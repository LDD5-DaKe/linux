// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo Driver for the PWM Module in the DE1-SoC Computer System
 * Initializes the LEDs with a default pattern and then runs a test
 * animation on it.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>

// -----------------------------------------------------------------
// PWM channel definitions
static size_t __iomem *led_regs; // module intern pointer to the memory region

#define PWM_REG_BASE 0xFF203080UL
#define PWM_NUM_CHANNELS 8 // only map 8 led channels to the
#define PWM_REG_SIZE (PWM_NUM_CHANNELS * sizeof(uint32_t))

#define PWM_MAX 0x7FFU

// -----------------------------------------------------------------
// Constants for the timer/running led
#define NUM_LEDS 8
#define STARTUP_DELAY 3000
#define TIMER_DELAY 125

#define MULTIPLE_WRITE_DELAY 200

static int set_led(size_t __iomem *reg_base, int ledNr, uint32_t value);

// -----------------------------------------------------------------
// PWM Channel definitions for the character device
#define CDEV_PWM_REG_BASE (PWM_REG_BASE + PWM_NUM_CHANNELS * sizeof(u32))
#define CDEV_PWM_REG_SIZE (sizeof(u32))

#define PWM_CONVERSION_FACTOR (u32)(PWM_MAX / 100)
#define TO_PWM_VALUE(x) ((x)*PWM_CONVERSION_FACTOR)
#define FROM_PWM_VALUE(x) (u8)((x) / PWM_CONVERSION_FACTOR)

// -----------------------------------------------------------------
// Timer configuration
/**
 * callback to execute when the timer elapses
 * this function will create the pattern on the leds
 *
 */
static void on_timer_elapsed(struct timer_list *timer);

static DEFINE_TIMER(timer, on_timer_elapsed);

// -----------------------------------------------------------------
// character device configuration

struct ledpwm {
	u32 __iomem *led_regs;
	struct cdev cdev;
};

static struct ledpwm device_data;
static dev_t device_number;
static struct class *cdev_class;
static struct device *cdev_device;

static int ledpwm_open(struct inode *inode, struct file *filep)
{
	struct ledpwm *data = container_of(inode->i_cdev, struct ledpwm, cdev);
	filep->private_data = data;

	printk(KERN_INFO "In ledpwm_open");

	return 0;
}

static ssize_t ledpwm_read(struct file *filep, char __user *buf, size_t count,
			   loff_t *offp)
{
	u8 pwm_channel;
	struct ledpwm *data = filep->private_data;
	printk(KERN_INFO "In ledpwm_read, count: %d, offp: %lld", count, *offp);

	// if the byte has already been copied to the userspace
	if (*offp >= 1)
		return 0;

	// check buffer size
	if (count < 1)
		return -ETOOSMALL;

	// read the current led value, convert to scale 0 - 100
	pwm_channel = FROM_PWM_VALUE(ioread32(data->led_regs));

	// copy_to_user returns uncopied bytes, if not 0, an error occurred
	if (copy_to_user(buf, &pwm_channel, 1)) {
		printk(KERN_ERR "unable to copy data to userspace");
		return -EFAULT;
	}

	// increment the offset pointer
	*offp += 1;
	return 1;
}

static ssize_t ledpwm_write(struct file *filep, const char __user *buf,
			    size_t count, loff_t *offp)
{
	u8 userdata;

	size_t bytes_written = 0;
	struct ledpwm *data = filep->private_data;
	printk(KERN_INFO "In ledpwm_write, count: %d, offp: %lld", count,
	       *offp);

	for (bytes_written = 0; bytes_written < count;) {
		if (copy_from_user(&userdata, buf, 1)) {
			printk(KERN_ERR "unable to copy data from userspace");
			return -EFAULT;
		}

        set_led(data->led_regs, 0, TO_PWM_VALUE(userdata));

        // wait for 200ms if there are more bytes to write
		if (++bytes_written < count)
			mdelay(MULTIPLE_WRITE_DELAY);
	}

	return bytes_written;
}

// create the structure with the file pointers
static struct file_operations fops = {
	.open = ledpwm_open,
	.read = ledpwm_read,
	.write = ledpwm_write,
};

// -----------------------------------------------------------------
// misc functions

/**
 * Requests and maps the PWM I/O registers.
 * The ioremapped registers are stored in the static variable led_regs
 *
 * @param base base address of the register to remap
 *
 * @param size size of the memory region to remap (in bytes)
 *
 * @param memory [out] remapped memory region
 *
 * @return 0 if OK, an error code otherwise.
 *         necessary cleanup is already done
 */
static int map_registers(resource_size_t const base, resource_size_t const size,
			 size_t **memory)
{
	int ret = 0;

	if (!request_mem_region(base, size, "PWM Regs")) {
		printk(KERN_ERR "Unable to request mem region\n");
		return -EBUSY;
	}

	*memory = (size_t *)ioremap(base, size);
	if (!led_regs) {
		printk(KERN_ERR "Unable to ioremap PWM Registers\n");
		ret = -EBUSY;
		goto release;
	}

	goto end;

release:
	release_mem_region(PWM_REG_BASE, PWM_REG_SIZE);

end:
	return ret;
}

/**
 * unmap and release a mapped memory region
 *
 * @param base base address of the mapped region
 *
 * @param size size of the mapped region (in bytes)
 *
 * @param memory [inout] pointer to the remapped memory region, will be set to NULL
 */
static void unmap_registers(resource_size_t const base, resource_size_t size,
			    size_t **memory)
{
	iounmap((void *)*memory);
	*memory = (size_t *)NULL;
	release_mem_region(base, size);
}

/**
 * allocate + initialize character device + setup udev
 */
static int setup_char_dev(void)
{
	int status;

	// map the 9th pwm channel into the device data struct
	status = map_registers(CDEV_PWM_REG_BASE, CDEV_PWM_REG_SIZE,
			       &(device_data.led_regs));
	if (status != 0) {
		printk(KERN_ERR
		       "Unable to map pwm channel for the character device");
		goto exit;
	}

	// allocate character device
	status = alloc_chrdev_region(&device_number, 0, 1, "ledpwm");
	if (status < 0) {
		printk(KERN_ERR "Unable to allocate chardev region\n");
		goto release_mapped_region;
	}

	// init structure
	cdev_init(&device_data.cdev, &fops);
	device_data.cdev.owner = THIS_MODULE;

	// and add device
	status = cdev_add(&device_data.cdev, device_number, 1);
	if (status < 0) {
		printk(KERN_ERR "Unable to add cdev\n");
		goto release_chardev;
	}

	// create device file
	cdev_class = class_create(THIS_MODULE, "ldd5");
	if (IS_ERR(cdev_class)) {
		printk(KERN_ERR "Unable to create class\n");
		status = -EEXIST;
		goto remove_device;
	}

	cdev_device = device_create(cdev_class, NULL, device_number,
				    &device_data, "demo");

	if (IS_ERR(cdev_device)) {
		printk(KERN_ERR "Unable to create device\n");
		status = -EEXIST;
		goto remove_device_class;
	}

	// everything ok
	return 0;

remove_device_class:
	class_destroy(cdev_class);

remove_device:
	cdev_del(&device_data.cdev);

release_chardev:
	unregister_chrdev_region(device_number, 1);

release_mapped_region:
	unmap_registers(CDEV_PWM_REG_BASE, CDEV_PWM_REG_SIZE,
			&(device_data.led_regs));

exit:
	return status;
}

static void __exit remove_char_dev(void)
{
	// remove device file
	device_destroy(cdev_class, device_number);
	class_destroy(cdev_class);

	// release resources
	cdev_del(&device_data.cdev);
	unregister_chrdev_region(device_number, 1);

    // unmap the pwm channel
    unmap_registers(CDEV_PWM_REG_BASE, CDEV_PWM_REG_SIZE, &device_data.led_regs);
}

/**
 * Set the value (brightness) of a single LED PWM Channel
 *
 * @param ledNr Index of the LED, must be between 0 and PWM_NUM_CHANNELS
 *
 * @param value Duty Cycle of the PWM, must be between 0 and PWM_MAX
 *
 * @return -EINVAL if ledNr or value are invalid, 0 otherwise
 */
int set_led(size_t __iomem *reg_base, int ledNr, uint32_t value)
{
	if (reg_base == NULL)
		return -EINVAL;

	if (ledNr < 0 || ledNr >= PWM_NUM_CHANNELS)
		return -EINVAL;

	if (value > PWM_MAX)
		return -EINVAL;

	iowrite32(value, reg_base + ledNr * sizeof(u32));

	return 0;
}

void on_timer_elapsed(struct timer_list *timer)
{
	static u32 currPos; // will be initialized to 0

	// create the pattern for a running light
	u32 i = currPos;
	u32 currVal = PWM_MAX;

	// create a running led with a slight trail
	do {
		set_led(led_regs, i, currVal);
		// currVal will be 0 after 3 loops
		currVal >>= 4;
		i = (i - 1) % NUM_LEDS;
	} while (i != currPos);

	currPos = (currPos + 1) % NUM_LEDS;
	mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_DELAY));
}

// -----------------------------------------------------------------
// module load/unload functions

static int __init led_pwm_init(void)
{
	int status;
	int i;

	printk(KERN_INFO "Loading PWM Driver\n");

	// map the lower 8 pwm channels to be accessed by led_regs
	status = map_registers(PWM_REG_BASE, PWM_REG_SIZE, &led_regs);
	if (status != 0)
		goto exit;

	// setup the character device config
	status = setup_char_dev();
	if (status != 0)
		goto release_pwm_regs;

	for (i = 0; i < NUM_LEDS; i++)
		set_led(led_regs, i, PWM_MAX);

	// configure the timer, first callback after 3000 ms
	mod_timer(&timer, jiffies + msecs_to_jiffies(STARTUP_DELAY));

	return 0;

release_pwm_regs:
	unmap_registers(PWM_REG_BASE, PWM_REG_SIZE, &led_regs);

exit:
	return status;
}

static void __exit led_pwm_exit(void)
{
	int i;

	printk(KERN_INFO "Unloading PWM Driver\n");

	// stop the timer -> otherwise kernel panic :-)
	del_timer_sync(&timer);

	for (i = 0; i < PWM_NUM_CHANNELS; i++)
		set_led(led_regs, i, 0);

    remove_char_dev();
	unmap_registers(PWM_REG_BASE, PWM_REG_SIZE, &led_regs);
}

module_init(led_pwm_init);
module_exit(led_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the LED PWM component of the DE1-SoC Computer");
MODULE_AUTHOR("Alexander Daum <alexander.daum@mailbox.org>");
MODULE_AUTHOR("Matthias Kern <kern_matthias@gmx.at>");
