
/*
 * linux/drivers/i2c/i2c-s3c2413-bitbang.c
 *
 * Author: Richard Titmuss <richard_titmuss@logitech.com>
 *
 * An I2C adapter driver for the S3C2412 GPIO
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <asm/hardware.h>
#include <asm/arch-s3c2413/regs-gpio.h>


static void s3c2413_bitbang_setsda (void *data,int state)
{
	s3c2413_gpio_setpin(S3C2413_GPH4, (state > 0));
}

static void s3c2413_bitbang_setscl (void *data,int state)
{
	s3c2413_gpio_setpin(S3C2413_GPH5, (state > 0));
}

static int s3c2413_bitbang_getsda (void *data)
{
	return (s3c2413_gpio_getpin(S3C2413_GPH4) != 0);
}

static void s3c2413_bitbang_sdapin(void *data,int output) {
	if (output) {
		s3c2413_gpio_cfgpin(S3C2413_GPH4, S3C2413_GPH4_OUTP);
	}
	else {
		s3c2413_gpio_cfgpin(S3C2413_GPH4, S3C2413_GPH4_INP);
	}
}

static struct i2c_algo_bit_data bit_s3c2413_bitbang_data = {
	.setsda		= s3c2413_bitbang_setsda,
	.setscl		= s3c2413_bitbang_setscl,
	.getsda		= s3c2413_bitbang_getsda,
	.getscl		= NULL,
	.sdapin		= s3c2413_bitbang_sdapin,
	.udelay		= 20,
	.mdelay		= 20,
	.timeout	= HZ
};

static struct i2c_adapter s3c2413_bitbang_ops = {
	.owner			= THIS_MODULE,
	.id			= I2C_HW_B_S3C2413,
	.algo_data		= &bit_s3c2413_bitbang_data,
	.dev			= {
//		.name		= "S3C2413_Bitbang adapter driver",
	},
};

static int __init i2c_s3c2413_bitbang_init (void)
{
	s3c2413_gpio_cfgpin(S3C2413_GPH4, S3C2413_GPH4_OUTP);
	s3c2413_gpio_setpin(S3C2413_GPH4, 1);
	s3c2413_gpio_pullup(S3C2413_GPH4, 1);

	s3c2413_gpio_cfgpin(S3C2413_GPH5, S3C2413_GPH5_OUTP);
	s3c2413_gpio_setpin(S3C2413_GPH5, 1);
	s3c2413_gpio_pullup(S3C2413_GPH5, 1);

	return i2c_bit_add_bus(&s3c2413_bitbang_ops);
}

static void __exit i2c_s3c2413_bitbang_exit (void)
{
	i2c_bit_del_bus(&s3c2413_bitbang_ops);
}

MODULE_AUTHOR ("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION ("I2C-Bus adapter routines for S3C2413 Bitbang");
MODULE_LICENSE ("GPL");

module_init (i2c_s3c2413_bitbang_init);
module_exit (i2c_s3c2413_bitbang_exit);

