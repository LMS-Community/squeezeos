/*
 * Copyright 2008 Logitech. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mx35_fab4.h>

extern struct mxc_gpio_port mxc_gpio_ports[];

enum gpio_reg {
	GPIO_DR = 0x00,
	GPIO_GDIR = 0x04,
	GPIO_PSR = 0x08,
	GPIO_ICR1 = 0x0C,
	GPIO_ICR2 = 0x10,
	GPIO_IMR = 0x14,
	GPIO_ISR = 0x18,
};

#define MUX_I		0
#define PAD_I		10
#define RSVD_I		21

#define PIN_TO_MUX_MASK	((1<<(PAD_I - MUX_I)) - 1)
#define PIN_TO_PAD_MASK	((1<<(RSVD_I - PAD_I)) - 1)

#define PIN_TO_IOMUX_MUX(pin) ((pin >> MUX_I) & PIN_TO_MUX_MASK)
#define PIN_TO_IOMUX_PAD(pin) ((pin >> PAD_I) & PIN_TO_PAD_MASK)

static ssize_t fab4_gpio_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int i;
	struct mxc_fab4_gpio_platform_data *fab4_gpio_data = dev->platform_data;

	for (i=0; i<fab4_gpio_data->nr_pins; i++) {
		int datain;
		u32 gpio, base;
		int dr, gdir, psr;
		u32 pin;
		int input;

		if (strcmp(attr->attr.name, fab4_gpio_data->pins[i].name) != 0)
			continue;

		/* read gpio using offical api */
		pin = fab4_gpio_data->pins[i].gpio;
		datain = mxc_get_gpio_datain(pin);

		/* read gpio state from registers */
		gpio = IOMUX_TO_GPIO(pin);
		base = mxc_gpio_ports[GPIO_TO_PORT(gpio)].base;

		dr = (__raw_readl(base + GPIO_DR) >> GPIO_TO_INDEX(gpio)) & 1;
		gdir = (__raw_readl(base + GPIO_GDIR) >> GPIO_TO_INDEX(gpio)) & 1;
		psr = (__raw_readl(base + GPIO_PSR) >> GPIO_TO_INDEX(gpio)) & 1;

		//input = __raw_readl(IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x7AC + (pin << 2));

		return snprintf(buf, PAGE_SIZE, "name:\t%s\ndatain:\t%d\ngdir:\t%s\ndr:\t%d\npsr:\t%d\ninput:\t%d\n", attr->attr.name, datain, (gdir==0)?"input":"output", dr, psr, input);
	}

	return 0;
}

static ssize_t fab4_gpio_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int i;
	struct mxc_fab4_gpio_platform_data *fab4_gpio_data = dev->platform_data;
	unsigned long val;
	char *ptr;

	val = simple_strtoul(buf, &ptr, 10);

	for (i=0; i<fab4_gpio_data->nr_pins; i++) {
		if (strcmp(attr->attr.name, fab4_gpio_data->pins[i].name) != 0)
			continue;

		/* set gpio using offical api */
		mxc_set_gpio_dataout(fab4_gpio_data->pins[i].gpio, val);

		return count;
	}

	return count;
}

static int fab4_probe(struct platform_device *pdev)
{
	int i, err;
	struct device *dev = &pdev->dev;
	struct mxc_fab4_gpio_platform_data *fab4_gpio_data = dev->platform_data;

	for (i=0; i<fab4_gpio_data->nr_pins; i++) {
		struct device_attribute *attr;

		attr = kmalloc(sizeof(struct device_attribute), GFP_KERNEL);

		attr->attr.name = fab4_gpio_data->pins[i].name;
		attr->attr.owner = THIS_MODULE;
		attr->attr.mode = S_IRUGO | S_IWUSR;
		attr->show = fab4_gpio_show;
		attr->store = fab4_gpio_store;

		fab4_gpio_data->pins[i].attr = attr;

		if ((err = device_create_file(dev, attr))) {
			dev_err(dev, "cannot attach resume attribute\n");
			kfree(attr);
		}
	}

	return 0;
}

static int fab4_remove(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct mxc_fab4_gpio_platform_data *fab4_gpio_data = dev->platform_data;

	for (i=0; i<fab4_gpio_data->nr_pins; i++) {
		if (fab4_gpio_data->pins[i].attr) {
			device_remove_file(dev, fab4_gpio_data->pins[i].attr);

			kfree(fab4_gpio_data->pins[i].attr);
			fab4_gpio_data->pins[i].attr = NULL;
		}
	}

	return 0;
}

static struct platform_driver fab4_gpio_driver = {
	.probe = fab4_probe,
	.remove = fab4_remove,
	.driver = {
		.name = "fab4_gpio",
	},
};

static int __init fab4_gpio_init(void)
{
	return platform_driver_register(&fab4_gpio_driver);
}

static void __exit fab4_gpio_exit(void)
{
	platform_driver_unregister(&fab4_gpio_driver);
}


MODULE_AUTHOR ("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("Fab4 GPIO debug driver");
MODULE_LICENSE("GPL");

module_init(fab4_gpio_init)
module_exit(fab4_gpio_exit)
