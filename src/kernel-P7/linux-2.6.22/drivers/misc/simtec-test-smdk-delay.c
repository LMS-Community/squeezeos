/* drivers/misc/simtec-test-smdk-delay.c
 *
 * Copyright 2008 Simtec Electronics
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>

#include <asm/arch/regs-gpio.h>

#define DELAY (1)
#define VAL (1<<2)

static void do_test(void)
{
	unsigned long flags;
	unsigned long gpgdat;
	int loop;

	local_irq_save(flags);

	gpgdat = __raw_readl(S3C2410_GPGDAT);

	for (loop = 0; loop < 2000; loop++) {
		gpgdat ^= VAL;
		__raw_writel(gpgdat, S3C2410_GPGDAT);
		udelay(DELAY);
		gpgdat ^= VAL;
		__raw_writel(gpgdat, S3C2410_GPGDAT);
		udelay(DELAY);
	}

	local_irq_restore(flags);
}


static int nb_call(struct notifier_block *nb, unsigned long val, void *data)
{
	if (val == CPUFREQ_POSTCHANGE) {
		printk("notified...\n");
		do_test();
	}

	return 0;
}

static struct notifier_block nb = {
	.notifier_call	= nb_call,

};

/* we use GPG2 to output */


static int smdkdelay_init(void)
{
	printk("CPUFreq test\n");

	if (!machine_is_smdk2412()) {
		printk(KERN_INFO "machine is not smdk2412\n");
		return 0;
	}

	s3c2410_gpio_setpin(S3C2410_GPG2, 0);
	s3c2410_gpio_cfgpin(S3C2410_GPG2, S3C2410_GPIO_OUTPUT);

	do_test();

	return cpufreq_register_notifier(&nb, CPUFREQ_TRANSITION_NOTIFIER);
}

module_init(smdkdelay_init);





