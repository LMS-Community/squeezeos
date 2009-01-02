/* drivers/video/backlight/vgg243271.c
 *
 * VGG243271 (ILI9320) LCD controller driver.
 *
 * Copyright 2008 Logitech
 *	Richard Titmuss <richard_titmuss@logitech.com>
 *
 * Based on:
 * VGG2432A4 (ILI9320) LCD controller driver
 *
 * Copyright 2007 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/lcd.h>
#include <linux/module.h>

#include <linux/spi/spi.h>

#include <video/ili9320.h>

#include "ili9320.h"

/* Device initialisation sequences */

static struct ili9320_reg vgg_init1[] = {
	{
		.address = ILI9320_POWER1,
		.value	 = ILI9320_POWER1_AP(0) | ILI9320_POWER1_BT(0),
	}, {
		.address = ILI9320_POWER2,
		.value	 = ILI9320_POWER2_VC(7) | ILI9320_POWER2_DC0(0) | ILI9320_POWER2_DC1(0),
	}, {
		.address = ILI9320_POWER3,
		.value	 = ILI9320_POWER3_VRH(0),
	}, {
		.address = ILI9320_POWER4,
		.value	 = ILI9320_POWER4_VREOUT(0),
	},
};

static struct ili9320_reg vgg_init2[] = {
	{
		.address = ILI9320_POWER1,
		.value   = ILI9320_POWER1_AP(1) | ILI9320_POWER1_APE | ILI9320_POWER1_BT(4) | ILI9320_POWER1_SAP,
	}, {
		.address = ILI9320_POWER2,
		.value   = ILI9320_POWER2_VC(7) | ILI9320_POWER2_DC0(2) | ILI9320_POWER2_DC1(2),
	}
};

static struct ili9320_reg vgg_gamma[] = {
	{
		.address = ILI9320_GAMMA1,
		.value	 = 0x0000,
	}, {
		.address = ILI9320_GAMMA2,
		.value   = 0x0506,
	}, {
		.address = ILI9320_GAMMA3,
		.value	 = 0x0104,
	}, {
		.address = ILI9320_GAMMA4,
		.value	 = 0x0207,
	}, {
		.address = ILI9320_GAMMA5,
		.value	 = 0x000F,
	}, {
		.address = ILI9320_GAMMA6,
		.value	 = 0x0306,
	}, {
		.address = ILI9320_GAMMA7,
		.value	 = 0x0102,
	}, {
		.address = ILI9320_GAMMA8,
		.value	 = 0x0707,
	}, {
		.address = ILI9320_GAMMA9,
		.value	 = 0x0702,
	}, {
		.address = ILI9320_GAMMA10,
		.value	 = 0x1604,
	}

};

static struct ili9320_reg vgg_init0[] = {
	[0]	= {
		/* set direction and scan mode gate */
		.address = ILI9320_DRIVER,
		.value	 = ILI9320_DRIVER_SS,
	}, {
		.address = ILI9320_DRIVEWAVE,
		.value	 = ILI9320_DRIVEWAVE_MUSTSET | ILI9320_DRIVEWAVE_EOR | ILI9320_DRIVEWAVE_BC,
	}, {
		.address = ILI9320_ENTRYMODE,
		.value	 = ILI9320_ENTRYMODE_ID(3) | ILI9320_ENTRYMODE_BGR,
	}, {
		.address = ILI9320_RESIZING,
		.value	 = 0x0,
	},
};


static int vgg243271_lcd_init(struct ili9320 *lcd,
			      struct ili9320_platdata *cfg)
{
	unsigned int addr;
	int ret;

	/* Set internal timing */
	ret = ili9320_write(lcd, 0x00e3, 0x3008);
	if (ret)
		goto err_initial;

	ret = ili9320_write(lcd, 0x00e7, 0x0012);
	if (ret)
		goto err_initial;

	ret = ili9320_write(lcd, 0x00ef, 0x1231);
	if (ret)
		goto err_initial;

	/* must wait at-lesat 10ms after starting */
	mdelay(15);

	ret = ili9320_write_regs(lcd, vgg_init0, ARRAY_SIZE(vgg_init0));
	if (ret != 0)
		goto err_initial;

	ili9320_write(lcd, ILI9320_DISPLAY2, cfg->display2);
	ili9320_write(lcd, ILI9320_DISPLAY3, cfg->display3);
	ili9320_write(lcd, ILI9320_DISPLAY4, cfg->display4);

	ili9320_write(lcd, ILI9320_RGB_IF1, cfg->rgb_if1);
	ili9320_write(lcd, ILI9320_FRAMEMAKER, 0x0);
	ili9320_write(lcd, ILI9320_RGB_IF2, cfg->rgb_if2);

	ret = ili9320_write_regs(lcd, vgg_init1, ARRAY_SIZE(vgg_init1));
	if (ret != 0)
		goto err_vgg;

	mdelay(300);

	ret = ili9320_write_regs(lcd, vgg_init2, ARRAY_SIZE(vgg_init2));
	if (ret != 0)
		goto err_vgg2;

	mdelay(100);

	ili9320_write(lcd, ILI9320_POWER3, 0x001c);

	mdelay(100);

	ili9320_write(lcd, ILI9320_POWER4, 0x1a00);
	ili9320_write(lcd, ILI9320_POWER7, 0x0025);

	mdelay(100);

	ili9320_write(lcd, ILI9320_FRAME_RATE_COLOUR, 0x0C);

	mdelay(100);

	ili9320_write(lcd, ILI9320_GRAM_HORIZ_ADDR, 0x00);
	ili9320_write(lcd, ILI9320_GRAM_VERT_ADD, 0x00);

	ret = ili9320_write_regs(lcd, vgg_gamma, ARRAY_SIZE(vgg_gamma));
	if (ret != 0)
		goto err_vgg3;

	ili9320_write(lcd, ILI9320_HORIZ_START, 0x0);
	ili9320_write(lcd, ILI9320_HORIZ_END, cfg->hsize - 1);
	ili9320_write(lcd, ILI9320_VERT_START, 0x0);
	ili9320_write(lcd, ILI9320_VERT_END, cfg->vsize - 1);

	ili9320_write(lcd, ILI9320_DRIVER2,
		      ILI9320_DRIVER2_GS |
		      ILI9320_DRIVER2_NL(((cfg->vsize - 240) / 8) + 0x1D));

	ili9320_write(lcd, ILI9320_BASE_IMAGE, 0x1);
	ili9320_write(lcd, ILI9320_VERT_SCROLL, 0x00);

	for (addr = ILI9320_PARTIAL1_POSITION; addr <= ILI9320_PARTIAL2_END;
	     addr++) {
		ili9320_write(lcd, addr, 0x0);
	}

	ili9320_write(lcd, ILI9320_INTERFACE1, 0x10);
	ili9320_write(lcd, ILI9320_INTERFACE2, cfg->interface2);

	lcd->display1 = (ILI9320_DISPLAY1_D(3) | ILI9320_DISPLAY1_DTE |
			 ILI9320_DISPLAY1_GON | ILI9320_DISPLAY1_BASEE);

	ili9320_write(lcd, ILI9320_DISPLAY1, lcd->display1);

	return 0;

 err_vgg3:
 err_vgg2:
 err_vgg:
 err_initial:
	return ret;
}

#ifdef CONFIG_PM
static int vgg243271_suspend(struct spi_device *spi, pm_message_t state)
{
	return ili9320_suspend(dev_get_drvdata(&spi->dev), state);
}

static int vgg243271_resume(struct spi_device *spi)
{
	return ili9320_resume(dev_get_drvdata(&spi->dev));
}
#else
#define vgg243271_suspend	NULL
#define vgg243271_resume 	NULL
#endif

static struct ili9320_client vgg243271_client = {
	.name	= "VGG243271",
	.init	= vgg243271_lcd_init,
};

/* Device probe */

static int __devinit vgg243271_probe(struct spi_device *spi)
{
	int ret;

	ret = ili9320_probe_spi(spi, &vgg243271_client);
	if (ret != 0) {
		dev_err(&spi->dev, "failed to initialise ili9320\n");
		return ret;
	}

	return 0;
}

static int __devexit vgg243271_remove(struct spi_device *spi)
{
	return ili9320_remove(dev_get_drvdata(&spi->dev));
}

static void vgg243271_shutdown(struct spi_device *spi)
{
	ili9320_shutdown(dev_get_drvdata(&spi->dev));
}

static struct spi_driver vgg243271_driver = {
	.driver = {
		.name		= "VGG243271",
		.owner		= THIS_MODULE,
	},
	.probe		= vgg243271_probe,
	.remove		= __devexit_p(vgg243271_remove),
	.shutdown	= vgg243271_shutdown,
	.suspend	= vgg243271_suspend,
	.resume		= vgg243271_resume,
};

/* Device driver initialisation */

static int __init vgg243271_init(void)
{
	return spi_register_driver(&vgg243271_driver);
}

static void __exit vgg243271_exit(void)
{
	spi_unregister_driver(&vgg243271_driver);
}

module_init(vgg243271_init);
module_exit(vgg243271_exit);

MODULE_AUTHOR("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("VGG243271 LCD Driver");
MODULE_LICENSE("GPLv2");


