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

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

// -----------------------------------------------------------------
// device tree configuration
static const struct of_device_id ledpwm_of_match[] = {
	{
		.compatible = "ldd,ledpwm",
	},
	{}
};

MODULE_DEVICE_TABLE(of, ledpwm_of_match);

static int ledpwm_probe(struct platform_device *pdev);
static int ledpwm_remove(struct platform_device *pdev);

static struct platform_driver ledpwm_driver = {
	.driver = {
		.name = "ledpwm",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ledpwm_of_match),
	},
	.probe = ledpwm_probe,
	.remove = ledpwm_remove,
};

module_platform_driver(ledpwm_driver);

// -----------------------------------------------------------------
// PWM Channel definitions for the character device
#define PWM_MAX 0x7FFU
#define CDEV_MAX_USERDATA 100
#define MULTIPLE_WRITE_DELAY 200

#define TO_PWM_VALUE(x) ((x)*PWM_MAX / CDEV_MAX_USERDATA)
#define FROM_PWM_VALUE(x) (u8)((x)*CDEV_MAX_USERDATA / PWM_MAX)

static int set_led(size_t __iomem *reg_base, uint32_t value);

// -----------------------------------------------------------------
// character device configuration
struct ledpwm_dev {
	u32 __iomem *led_regs;
	struct miscdevice misc;
};

static ssize_t ledpwm_read(struct file *filep, char __user *buf, size_t count,
			   loff_t *offp);
static ssize_t ledpwm_write(struct file *filep, const char __user *buf,
			    size_t count, loff_t *offp);

// create the structure with the file pointers
static struct file_operations const ledpwm_fops = {
	.read = ledpwm_read,
	.write = ledpwm_write,
};

static ssize_t ledpwm_read(struct file *filep, char __user *buf, size_t count,
			   loff_t *offp)
{
	u8 pwm_channel;
	struct ledpwm_dev *data;

	data = container_of(filep->private_data, struct ledpwm_dev, misc);

	dev_info(data->misc.parent, "In %s, count: %d, offp: %lld\n", __func__,
		 count, *offp);

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
		dev_err(data->misc.parent,
			"unable to copy data to userspace\n");
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
	struct ledpwm_dev *data;

	data = container_of(filep->private_data, struct ledpwm_dev, misc);

	if (!data)
		return -EFAULT;

	dev_info(data->misc.parent, "In %s, count: %d, offp: %lld\n", __func__,
		 count, *offp);

	for (bytes_written = 0; bytes_written < count;) {
		// load the current byte from the userspace
		// (bulk transfer would be more efficient)
		if (copy_from_user(&userdata, &(buf[bytes_written]), 1)) {
			dev_err(data->misc.parent,
				"unable to copy data from userspace\n");
			return -EFAULT;
		}

		// verify that the received userdata is valid
		if (userdata > CDEV_MAX_USERDATA) {
			dev_err(data->misc.parent,
				"received invalid userdata: %d, will skip this value",
				userdata);
			set_led(data->led_regs, 0);
			return -EINVAL;
		}

		// set the value
		set_led(data->led_regs, TO_PWM_VALUE(userdata));

		// wait for 200ms if there are more bytes to write
		if (++bytes_written < count)
			mdelay(MULTIPLE_WRITE_DELAY);
	}

	return bytes_written;
}

// -----------------------------------------------------------------
// misc functions

/**
 * Set the value (brightness) of a single LED PWM Channel
 *
 * @param ledNr Index of the LED, must be between 0 and PWM_NUM_CHANNELS
 *
 * @param value Duty Cycle of the PWM, must be between 0 and PWM_MAX
 *
 * @return -EINVAL if ledNr or value are invalid, 0 otherwise
 */
int set_led(u32 __iomem *reg_base, uint32_t value)
{
	if (reg_base == NULL)
		return -EINVAL;

	if (value > PWM_MAX)
		return -EINVAL;

	iowrite32(value, reg_base);

	return 0;
}

// -----------------------------------------------------------------
// module load/unload functions

int ledpwm_probe(struct platform_device *pdev)
{
	struct ledpwm_dev *ledpwm;
	struct resource *regs;
	int status;

	ledpwm =
		devm_kzalloc(&pdev->dev, sizeof(struct ledpwm_dev), GFP_KERNEL);
	if (!ledpwm) {
		dev_err(&pdev->dev, "could not allocate memory");
		return -ENOMEM;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "could not allocate memory");
		return -EFAULT;
	}

	ledpwm->led_regs = devm_ioremap_resource(&pdev->dev, regs);
	if (!ledpwm->led_regs) {
		dev_err(&pdev->dev, "error remapping the iomemory");
		return -EFAULT;
	}

	ledpwm->misc.name = "ledpwm";
	ledpwm->misc.minor = MISC_DYNAMIC_MINOR;
	ledpwm->misc.fops = &ledpwm_fops;
	ledpwm->misc.parent = &pdev->dev;
	status = misc_register(&ledpwm->misc);
	if (status) {
		dev_err(&pdev->dev, "unable to register the device");
		return status;
	}

	return 0;
}

int ledpwm_remove(struct platform_device *pdev)
{
	struct ledpwm_dev *ledpwm;

	ledpwm = platform_get_drvdata(pdev);
	misc_deregister(&ledpwm->misc);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the LED PWM component of the DE1-SoC Computer");
MODULE_AUTHOR("Alexander Daum <alexander.daum@mailbox.org>");
MODULE_AUTHOR("Matthias Kern <kern_matthias@gmx.at>");
