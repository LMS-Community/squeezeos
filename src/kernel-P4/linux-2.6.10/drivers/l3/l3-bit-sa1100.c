/*
 *  linux/drivers/l3/l3-bit-sa1100.c
 *
 *  Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is a combined I2C and L3 bus driver.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/l3/algo-bit.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/arch/assabet.h>

#define NAME "l3-bit-sa1100-gpio"

struct bit_data {
	unsigned int	sda;
	unsigned int	scl;
	unsigned int	l3_mode;
};

static int getsda(void *data)
{
	struct bit_data *bits = data;

	return GPLR & bits->sda;
}

#ifdef CONFIG_I2C_BIT_SA1100_GPIO
static void i2c_setsda(void *data, int state)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);
	if (state)
		GPDR &= ~bits->sda;
	else {
		GPCR = bits->sda;
		GPDR |= bits->sda;
	}
	local_irq_restore(flags);
}

static void i2c_setscl(void *data, int state)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);
	if (state)
		GPDR &= ~bits->scl;
	else {
		GPCR = bits->scl;
		GPDR |= bits->scl;
	}
	local_irq_restore(flags);
}

static int i2c_getscl(void *data)
{
	struct bit_data *bits = data;

	return GPLR & bits->scl;
}

static struct i2c_algo_bit_data i2c_bit_data = {
	.setsda		= i2c_setsda,
	.setscl		= i2c_setscl,
	.getsda		= getsda,
	.getscl		= i2c_getscl,
	.udelay		= 10,
	.mdelay		= 10,
	.timeout	= 100,
};

static struct i2c_adapter i2c_adapter = {
	.algo_data	= &i2c_bit_data,
};

#define LOCK	&i2c_adapter.bus_lock

static int __init i2c_init(struct bit_data *bits)
{
	i2c_bit_data.data = bits;
	return i2c_bit_add_bus(&i2c_adapter);
}

static void i2c_exit(void)
{
	i2c_bit_del_bus(&i2c_adapter);
}

#else
static DECLARE_MUTEX(l3_lock);
#define LOCK		&l3_lock
#define i2c_init(bits)	(0)
#define i2c_exit()	do { } while (0)
#endif

#ifdef CONFIG_L3_BIT_SA1100_GPIO
/*
 * iPAQs need the clock line driven hard high and low.
 */
static void l3_setscl(void *data, int state)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);
	if (state)
		GPSR = bits->scl;
	else
		GPCR = bits->scl;
	GPDR |= bits->scl;
	local_irq_restore(flags);
}

static void l3_setsda(void *data, int state)
{
	struct bit_data *bits = data;

	if (state)
		GPSR = bits->sda;
	else
		GPCR = bits->sda;
}

static void l3_setdir(void *data, int in)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);
	if (in)
		GPDR &= ~bits->sda;
	else
		GPDR |= bits->sda;
	local_irq_restore(flags);
}

static void l3_setmode(void *data, int state)
{
	struct bit_data *bits = data;

	if (state)
		GPSR = bits->l3_mode;
	else
		GPCR = bits->l3_mode;
}

static struct l3_algo_bit_data l3_bit_data = {
	.data		= NULL,
	.setdat		= l3_setsda,
	.setclk		= l3_setscl,
	.setmode	= l3_setmode,
	.setdir		= l3_setdir,
	.getdat		= getsda,
	.data_hold	= 1,
	.data_setup	= 1,
	.clock_high	= 1,
	.mode_hold	= 1,
	.mode_setup	= 1,
};

static struct l3_adapter l3_adapter = {
	.owner		= THIS_MODULE,
	.name		= NAME,
	.algo_data	= &l3_bit_data,
	.lock		= LOCK,
};

static int __init l3_init(struct bit_data *bits)
{
	l3_bit_data.data = bits;
	return l3_bit_add_bus(&l3_adapter);
}

static void __exit l3_exit(void)
{
	l3_bit_del_bus(&l3_adapter);
}
#else
#define l3_init(bits)	(0)
#define l3_exit()	do { } while (0)
#endif

static struct bit_data bit_data;

static int __init bus_init(void)
{
	struct bit_data *bit = &bit_data;
	unsigned long flags;
	int ret;

	if (machine_is_assabet() || machine_is_pangolin()) {
		bit->sda     = GPIO_GPIO15;
		bit->scl     = GPIO_GPIO18;
		bit->l3_mode = GPIO_GPIO17;
	}

	if (machine_is_h3600() || machine_is_h3100()) {
		bit->sda     = GPIO_GPIO14;
		bit->scl     = GPIO_GPIO16;
		bit->l3_mode = GPIO_GPIO15;
	}

	if (machine_is_stork()) {
		bit->sda     = GPIO_GPIO15;
		bit->scl     = GPIO_GPIO18;
		bit->l3_mode = GPIO_GPIO17;
	}

	if (!bit->sda)
		return -ENODEV;

	/*
	 * Default level for L3 mode is low.
	 * We set SCL and SDA high (i2c idle state).
	 */
	local_irq_save(flags);
	GPDR &= ~(bit->scl | bit->sda);
	GPCR = bit->l3_mode | bit->scl | bit->sda;
	GPDR |= bit->l3_mode;
	local_irq_restore(flags);

	if (machine_is_assabet()) {
		/*
		 * Release reset on UCB1300, ADI7171 and UDA1341.  We
		 * need to do this here so that we can communicate on
		 * the I2C/L3 buses.
		 */
		ASSABET_BCR_set(ASSABET_BCR_CODEC_RST);
		mdelay(1);
		ASSABET_BCR_clear(ASSABET_BCR_CODEC_RST);
		mdelay(1);
		ASSABET_BCR_set(ASSABET_BCR_CODEC_RST);
	}

	ret = i2c_init(bit);
	if (ret == 0 && bit->l3_mode) {
		ret = l3_init(bit);
		if (ret)
			i2c_exit();
	}

	return ret;
}

static void __exit bus_exit(void)
{
	l3_exit();
	i2c_exit();
}

module_init(bus_init);
module_exit(bus_exit);
