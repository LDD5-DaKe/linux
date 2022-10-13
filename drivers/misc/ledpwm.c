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
#define PWM_NUM_CHANNELS 10
#define PWM_REG_SIZE (PWM_NUM_CHANNELS * sizeof(uint32_t))

#define PWM_MAX 0x7FFU

static void __iomem *led_regs = NULL;

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
		return -EBUSY;
	}

	led_regs = ioremap(PWM_REG_BASE, PWM_REG_SIZE);
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
 * Set the value (brightness) of a single LED PWM Channel
 * @param ledNr Index of the LED, must be between 0 and PWM_NUM_CHANNELS
 * @param value Duty Cycle of the PWM, must be between 0 and PWM_MAX
 * @return -EINVAL if ledNr or value are invalid, 0 otherwise
 */
static int set_led(int ledNr, uint32_t value)
{
	if (ledNr < 0 || ledNr >= PWM_NUM_CHANNELS) {
		return -EINVAL;
	}
	if (value > PWM_MAX) {
		return -EINVAL;
	}

	iowrite32(value, led_regs + ledNr * sizeof(uint32_t));

	return 0;
}

static int __init led_pwm_init(void)
{
	int ret = 0;
	int i;

	printk(KERN_INFO "Loading PWM Driver\n");
	ret = map_registers();
	if (ret != 0) {
		return ret;
	}

	for (i = 0; i < 8; i++) {
		set_led(i, PWM_MAX);
	}

	return ret;
}

static void __exit led_pwm_exit(void)
{
	int i;
	printk(KERN_INFO "Unloading PWM Driver\n");

	for (i = 0; i < PWM_NUM_CHANNELS; i++) {
		set_led(i, 0);
	}

	iounmap(led_regs);
	led_regs = NULL;
	release_mem_region(PWM_REG_BASE, PWM_REG_SIZE);
}

module_init(led_pwm_init);
module_exit(led_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for the LED PWM component of the DE1-SoC Computer");
MODULE_AUTHOR("Alexander Daum <alexander.daum@mailbox.org>");
