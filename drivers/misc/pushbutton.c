// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo Driver for the PUSHBUTTON Module in the DE1-SoC Computer System
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>

#include <linux/kfifo.h>


#define FIFO_SIZE 8
#define BUTTONS_MASK 0xF

struct pushbutton_regs {
    u32 data;
    u32 RESERVED;
    u32 interrupt_mask;
    u32 edge_capture;
};

// -----------------------------------------------------------------
// device tree configuration
static const struct of_device_id pushbutton_of_match[] = {
	{
		.compatible = "ldd,pushbutton",
	},
	{}
};

MODULE_DEVICE_TABLE(of, pushbutton_of_match);

static int pushbutton_probe(struct platform_device *pdev);
static int pushbutton_remove(struct platform_device *pdev);

static struct platform_driver pushbutton_driver = {
	.driver = {
		.name = "pushbutton",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(pushbutton_of_match),
	},
	.probe = pushbutton_probe,
	.remove = pushbutton_remove,
};

module_platform_driver(pushbutton_driver);

// -----------------------------------------------------------------
// PUSHBUTTON Channel definitions for the character device

// -----------------------------------------------------------------
// character device configuration
struct pushbutton_dev {
	struct pushbutton_regs __iomem *button_regs;
	struct miscdevice misc;
    struct mutex mutex;
    struct kfifo fifo;
};

static ssize_t pushbutton_read(struct file *filep, char __user *buf, size_t count,
			   loff_t *offp);
static ssize_t pushbutton_open(struct inode *inode, struct file *filp);
static ssize_t pushbutton_release(struct inode *inode, struct file *filp);

// create the structure with the file pointers
static struct file_operations const pushbutton_fops = {
	.read = pushbutton_read,
    .open = pushbutton_open,
    .release = pushbutton_release,
};

static ssize_t pushbutton_read(struct file *filep, char __user *buf, size_t count,
			   loff_t *offp)
{
	u8 pwm_channel;
	struct pushbutton_dev *data;

	data = container_of(filep->private_data, struct pushbutton_dev, misc);

	dev_info(data->misc.parent, "In %s, count: %d, offp: %lld\n", __func__,
		 count, *offp);


    // TODO
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

static ssize_t pushbutton_open(struct inode *inode, struct file *filep) {
	struct pushbutton_dev *data = container_of(filep->private_data, struct pushbutton_dev, misc);
    if (mutex_lock_interruptible(&data->mutex)) {
        return -ERESTARTSYS;
    }
    return 0;
}

static ssize_t pushbutton_release(struct inode *inode, struct file *filep) {
	struct pushbutton_dev *data = container_of(filep->private_data, struct pushbutton_dev, misc);
    mutex_unlock(&data->mutex);
    return 0;
}


// -----------------------------------------------------------------
// misc functions


// -----------------------------------------------------------------
// module load/unload functions

int pushbutton_probe(struct platform_device *pdev)
{
	struct pushbutton_dev *pushbutton;
	struct resource *regs;
	int status;

	pushbutton =
		devm_kzalloc(&pdev->dev, sizeof(struct pushbutton_dev), GFP_KERNEL);
	if (!pushbutton) {
		dev_err(&pdev->dev, "could not allocate memory");
		return -ENOMEM;
	}


	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "could not allocate memory");
		return -EFAULT;
	}

	pushbutton->button_regs = devm_ioremap_resource(&pdev->dev, regs);
	if (!pushbutton->button_regs) {
		dev_err(&pdev->dev, "error remapping the iomemory");
		return -EFAULT;
	}

    if(kfifo_alloc(&pushbutton->fifo, FIFO_SIZE, GFP_KERNEL)) {
        return -ENOMEM;
    };

    mutex_init(&pushbutton->mutex);

	pushbutton->misc.name = "pushbutton_event";
	pushbutton->misc.minor = MISC_DYNAMIC_MINOR;
	pushbutton->misc.fops = &pushbutton_fops;
	pushbutton->misc.parent = &pdev->dev;
	status = misc_register(&pushbutton->misc);
	if (status) {
		dev_err(&pdev->dev, "unable to register the device");
        kfifo_free(&pushbutton->fifo);
		return status;
	}

    iowrite32(0xF, &pushbutton->button_regs->interrupt_mask);

	platform_set_drvdata(pdev, pushbutton);

	return 0;
}

int pushbutton_remove(struct platform_device *pdev)
{
	struct pushbutton_dev *pushbutton;

	pushbutton = platform_get_drvdata(pdev);
	misc_deregister(&pushbutton->misc);
	platform_set_drvdata(pdev, NULL);

    kfifo_free(&pushbutton->fifo);
    mutex_destroy(&pushbutton->mutex);

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the LED PUSHBUTTON component of the DE1-SoC Computer");
MODULE_AUTHOR("Alexander Daum <alexander.daum@mailbox.org>");
MODULE_AUTHOR("Matthias Kern <kern_matthias@gmx.at>");
