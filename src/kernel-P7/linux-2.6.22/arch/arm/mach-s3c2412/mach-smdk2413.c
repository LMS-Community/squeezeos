/* linux/arch/arm/mach-s3c2412/mach-smdk2413.c
 *
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Thanks to Dimity Andric (TomTom) and Steven Ryu (Samsung) for the
 * loans of SMDK2413 to work with.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/hardware.h>
#include <asm/hardware/iomd.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

//#include <asm/debug-ll.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-lcd.h>

#include <asm/arch/idle.h>
#include <asm/arch/udc.h>
#include <asm/arch/fb.h>

#include <asm/plat-s3c24xx/s3c2410.h>
#include <asm/plat-s3c24xx/s3c2412.h>
#include <asm/plat-s3c24xx/clock.h>
#include <asm/plat-s3c24xx/devs.h>
#include <asm/plat-s3c24xx/cpu.h>

#include <asm/plat-s3c24xx/common-smdk.h>

static struct map_desc smdk2413_iodesc[] __initdata = {
};

static struct s3c2410_uartcfg smdk2413_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
	/* IR port */
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x43,
		.ufcon	     = 0x51,
	}
};

static void smdk2413_udc_pullup(enum s3c2410_udc_cmd_e cmd)
{
	printk(KERN_DEBUG "udc: pullup(%d)\n",cmd);

	switch (cmd)
	{
		case S3C2410_UDC_P_ENABLE :
			s3c2410_gpio_setpin(S3C2410_GPF2, 1);
			break;
		case S3C2410_UDC_P_DISABLE :
			s3c2410_gpio_setpin(S3C2410_GPF2, 0);
			break;
		case S3C2410_UDC_P_RESET :
			break;
		default:
			break;
	}
}


static struct s3c2410_udc_mach_info smdk2413_udc_cfg __initdata = {
	.udc_command		= smdk2413_udc_pullup,
};

/* LCD timing and setup */

#define LCD_XRES	 (240)
#define LCD_YRES	 (320)
#define LCD_LEFT_MARGIN  (9)
#define LCD_RIGHT_MARGIN (2)
#define LCD_LOWER_MARGIN (0)
#define LCD_UPPER_MARGIN (5)
#define LCD_VSYNC	 (2)
#define LCD_HSYNC	 (2)

#define LCD_REFRESH	 (70)

#define LCD_HTOT (LCD_HSYNC + LCD_LEFT_MARGIN + LCD_XRES + LCD_RIGHT_MARGIN)
#define LCD_VTOT (LCD_VSYNC + LCD_LOWER_MARGIN + LCD_YRES + LCD_UPPER_MARGIN)

struct s3c2410fb_display smdk2413_displays[] = {
	[0] = {
		.width		= LCD_XRES,
		.height		= LCD_YRES,
		.xres		= LCD_XRES,
		.yres		= LCD_YRES,
		.xres_virtual	= LCD_XRES,
		.yres_virtual	= LCD_YRES,
		.left_margin	= LCD_LEFT_MARGIN,
		.right_margin	= LCD_RIGHT_MARGIN,
		.upper_margin	= LCD_UPPER_MARGIN,
		.lower_margin	= LCD_LOWER_MARGIN,
		.hsync_len	= LCD_HSYNC,
		.vsync_len	= LCD_VSYNC,

		.pixclock	= (1000000000000LL /
				   (LCD_REFRESH * LCD_HTOT * LCD_VTOT)),

		.bpp		= 16,
		.type		= (S3C2410_LCDCON1_TFT16BPP |
				   S3C2410_LCDCON1_TFT),

		.lcdcon5	= (S3C2410_LCDCON5_FRM565 |
				   S3C2410_LCDCON5_INVVLINE |
				   S3C2410_LCDCON5_INVVFRAME |
				   S3C2410_LCDCON5_INVVDEN |
				   S3C2410_LCDCON5_PWREN
			       ),
	}
};

#define S3C2410_GPCCON_MASK(x)	(3 << (x))
#define S3C2410_GPDCON_MASK(x)	(3 << (x))

struct s3c2410fb_mach_info smdk2413_lcd_cfg = {
	.displays	 = smdk2413_displays,
	.num_displays	 = ARRAY_SIZE(smdk2413_displays),
	.default_display = 0,

	/* Enable VD[2..7], VD[10..15], VD[18..23] and VCLK, syncs, VDEN
	 * and disable the pull down resistors on pins we are using for LCD
	 * data. */

	.gpcup		= (0xf << 1) | (0x3f << 10),

	.gpccon		= (S3C2410_GPC1_VCLK   | S3C2410_GPC2_VLINE |
			   S3C2410_GPC3_VFRAME | S3C2410_GPC4_VM |
			   S3C2410_GPC10_VD2   | S3C2410_GPC11_VD3 |
			   S3C2410_GPC12_VD4   | S3C2410_GPC13_VD5 |
			   S3C2410_GPC14_VD6   | S3C2410_GPC15_VD7),

	.gpccon_mask	= (S3C2410_GPCCON_MASK(1)  | S3C2410_GPCCON_MASK(2)  |
			   S3C2410_GPCCON_MASK(3)  | S3C2410_GPCCON_MASK(4)  |
			   S3C2410_GPCCON_MASK(10) | S3C2410_GPCCON_MASK(11) |
			   S3C2410_GPCCON_MASK(12) | S3C2410_GPCCON_MASK(13) |
			   S3C2410_GPCCON_MASK(14) | S3C2410_GPCCON_MASK(15)),

	.gpdup		= (0x3f << 2) | (0x3f << 10),

	.gpdcon		= (S3C2410_GPD2_VD10  | S3C2410_GPD3_VD11 |
			   S3C2410_GPD4_VD12  | S3C2410_GPD5_VD13 |
			   S3C2410_GPD6_VD14  | S3C2410_GPD7_VD15 |
			   S3C2410_GPD10_VD18 | S3C2410_GPD11_VD19 |
			   S3C2410_GPD12_VD20 | S3C2410_GPD13_VD21 |
			   S3C2410_GPD14_VD22 | S3C2410_GPD15_VD23 ),

	.gpdcon_mask	= (S3C2410_GPDCON_MASK(2)  | S3C2410_GPDCON_MASK(3) |
			   S3C2410_GPDCON_MASK(4)  | S3C2410_GPDCON_MASK(5) |
			   S3C2410_GPDCON_MASK(6)  | S3C2410_GPDCON_MASK(7) |
			   S3C2410_GPDCON_MASK(10) | S3C2410_GPDCON_MASK(11)|
			   S3C2410_GPDCON_MASK(12) | S3C2410_GPDCON_MASK(13)|
			   S3C2410_GPDCON_MASK(14) | S3C2410_GPDCON_MASK(15)),

	//.lpcsel		= 0xf90,
};

static struct platform_device *smdk2413_devices[] __initdata = {
	&s3c_device_usb,
	&s3c_device_lcd,
	&s3c_device_wdt,
	&s3c_device_i2c,
	&s3c_device_iis,
	&s3c_device_usbgadget,
};

static void __init smdk2413_fixup(struct machine_desc *desc,
				  struct tag *tags, char **cmdline,
				  struct meminfo *mi)
{
	if (tags != phys_to_virt(S3C2410_SDRAM_PA + 0x100)) {
		mi->nr_banks=1;
		mi->bank[0].start = 0x30000000;
		mi->bank[0].size = SZ_64M;
		mi->bank[0].node = 0;
	}
}

static void __init smdk2413_map_io(void)
{
	s3c24xx_init_io(smdk2413_iodesc, ARRAY_SIZE(smdk2413_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(smdk2413_uartcfgs, ARRAY_SIZE(smdk2413_uartcfgs));
}

static void __init smdk2413_machine_init(void)
{	/* Turn off suspend on both USB ports, and switch the
	 * selectable USB port to USB device mode. */

	s3c2410_gpio_setpin(S3C2410_GPF2, 0);
	s3c2410_gpio_cfgpin(S3C2410_GPF2, S3C2410_GPIO_OUTPUT);

	s3c2410_modify_misccr(S3C2410_MISCCR_USBHOST |
			      S3C2410_MISCCR_USBSUSPND0 |
			      S3C2410_MISCCR_USBSUSPND1, 0x0);


 	s3c24xx_udc_set_platdata(&smdk2413_udc_cfg);
	s3c24xx_fb_set_platdata(&smdk2413_lcd_cfg);

	platform_add_devices(smdk2413_devices, ARRAY_SIZE(smdk2413_devices));
	smdk_machine_init();
}

MACHINE_START(S3C2413, "S3C2413")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C2410_PA_UART,
	.io_pg_offst	= (((u32)S3C24XX_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C2410_SDRAM_PA + 0x100,

	.fixup		= smdk2413_fixup,
	.init_irq	= s3c24xx_init_irq,
	.map_io		= smdk2413_map_io,
	.init_machine	= smdk2413_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END

MACHINE_START(SMDK2412, "SMDK2412")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C2410_PA_UART,
	.io_pg_offst	= (((u32)S3C24XX_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C2410_SDRAM_PA + 0x100,

	.fixup		= smdk2413_fixup,
	.init_irq	= s3c24xx_init_irq,
	.map_io		= smdk2413_map_io,
	.init_machine	= smdk2413_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END

MACHINE_START(SMDK2413, "SMDK2413")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C2410_PA_UART,
	.io_pg_offst	= (((u32)S3C24XX_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C2410_SDRAM_PA + 0x100,

	.fixup		= smdk2413_fixup,
	.init_irq	= s3c24xx_init_irq,
	.map_io		= smdk2413_map_io,
	.init_machine	= smdk2413_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END
