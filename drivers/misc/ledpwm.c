// LPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo Driver for the PWM Module in the DE1-SoC Computer System
 * Initializes the LEDs with a default pattern and then runs a test
 * animation on it.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/io.h>

#define PWM_REG_BASE 0xFF203080UL
#define PWM_REG_SIZE (10 * sizeof(uint32_t))

static void __iomem *led_regs;

/**
 * Requests and maps the PWM I/O registers.
 * The ioremapped registers are stored in the static variable led_regs
 *
 * @return 0 if OK, an error code otherwise.
 *         necessary cleanup is already done
 */
static int map_registers(void)
{
	int ret = 0;

	if (!request_mem_region(PWM_REG_BASE, PWM_REG_SIZE, "PWM Regs")) {
		printk(KERN_ERR "Unable to request mem region\n");
		return EBUSY;
	}

	led_regs = ioremap(PWM_REG_BASE, PWM_REG_SIZE);
	if (!led_regs) {
		printk(KERN_ERR "Unable to ioremap PWM Registers\n");
		goto release;
	}

	goto end;

release:
	release_mem_region(PWM_REG_BASE, PWM_REG_SIZE);

end:
	return ret;
}

static int __init led_pwm_init(void)
{
	int ret = 0;

	printk(KERN_INFO "Loading PWM Driver\n");
	ret = map_registers();
	if (ret != 0) {
		return ret;
	}

	iowrite32(0x7FF, led_regs);

	return ret;
}

static void __exit led_pwm_exit(void)
{
	printk(KERN_INFO "Unloading PWM Driver\n");

	iowrite32(0x00, led_regs);

	iounmap(led_regs);
	release_mem_region(PWM_REG_BASE, PWM_REG_SIZE);
}

module_init(led_pwm_init);
module_exit(led_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the LED PWM component of the DE1-SoC Computer");
MODULE_AUTHOR("Alexander Daum <alexander.daum@mailbox.org>");
