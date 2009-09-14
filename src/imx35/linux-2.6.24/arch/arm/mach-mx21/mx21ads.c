/*
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/serial_8250.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/flash.h>
#endif

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/arch/memory.h>
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/mach/keypad.h>
#include "gpio_mux.h"

/*!
 * @file mx21ads.c
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup System
 */

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern void gpio_nand_active(void);
extern struct sys_timer mxc_timer;

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/*!
 * This array is used for mapping mx27 ADS keypad scancodes to input keyboard
 * keycodes.
 */
static u16 mxckpd_keycodes[(MAXROW * MAXCOL)] = {
	KEY_KP9, KEY_LEFTSHIFT, KEY_0, KEY_KPASTERISK, KEY_RECORD, KEY_POWER,
	KEY_KP8, KEY_9, KEY_8, KEY_7, KEY_KP5, KEY_VOLUMEDOWN,
	KEY_KP7, KEY_6, KEY_5, KEY_4, KEY_KP4, KEY_VOLUMEUP,
	KEY_KP6, KEY_3, KEY_2, KEY_1, KEY_KP3, KEY_DOWN,
	KEY_BACK, KEY_RIGHT, KEY_ENTER, KEY_LEFT, KEY_HOME, KEY_KP2,
	KEY_END, KEY_F2, KEY_UP, KEY_F1, KEY_F4, KEY_KP1,
};

static struct keypad_data evb_6_by_6_keypad = {
	.rowmax = 6,
	.colmax = 6,
	.irq = INT_KPP,
	.learning = 0,
	.delay = 2,
	.matrix = mxckpd_keycodes,
};

static struct resource mxc_kpp_resources[] = {
	[0] = {
	       .start = INT_KPP,
	       .end = INT_KPP,
	       .flags = IORESOURCE_IRQ,
	       }
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_kpp_resources),
	.resource = mxc_kpp_resources,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &evb_6_by_6_keypad,
		},
};

static void mxc_init_keypad(void)
{
	printk("mx27ads.c: registering mxc keypad device...");
	(void)platform_device_register(&mxc_keypad_device);
}
#else
static inline void mxc_init_keypad(void)
{
}
#endif

/* MTD NOR flash */

#if defined(CONFIG_MTD_MXC) || defined(CONFIG_MTD_MXC_MODULE)

static struct mtd_partition mxc_nor_partitions[] = {
	{
	 .name = "Bootloader",
	 .size = 512 * 1024,
	 .offset = 0x00000000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
	{
	 .name = "nor.Kernel",
	 .size = 2 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = 0},
	{
	 .name = "nor.userfs",
	 .size = 14 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = 0},
	{
	 .name = "nor.rootfs",
	 .size = 12 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = MTD_WRITEABLE},
	{
	 .name = "FIS directory",
	 .size = 12 * 1024,
	 .offset = 0x01FE0000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
	{
	 .name = "Redboot config",
	 .size = MTDPART_SIZ_FULL,
	 .offset = 0x01FFF000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
};

static struct flash_platform_data mxc_flash_data = {
	.map_name = "cfi_probe",
	.width = 4,
	.parts = mxc_nor_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nor_partitions),
};

static struct resource mxc_flash_resource = {
	.start = 0xC8000000,
	.end = 0xC8000000 + 0x02000000 - 1,
	.flags = IORESOURCE_MEM,
};

static struct platform_device mxc_nor_mtd_device = {
	.name = "mxc_nor_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_flash_data,
		},
	.num_resources = 1,
	.resource = &mxc_flash_resource,
};

static void mxc_init_nor_mtd(void)
{
	(void)platform_device_register(&mxc_nor_mtd_device);
}
#else
static void mxc_init_nor_mtd(void)
{
}
#endif

/* MTD NAND flash */

#if defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)

static struct mtd_partition mxc_nand_partitions[4] = {
	{
	 .name = "nand.bootloader",
	 .offset = 0,
	 .size = 1024 * 1024},
	{
	 .name = "nand.kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 5 * 1024 * 1024},
	{
	 .name = "nand.rootfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 22 * 1024 * 1024},
	{
	 .name = "nand.userfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL},
};

static struct flash_platform_data mxc_nand_data = {
	.parts = mxc_nand_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nand_partitions),
	.width = 1,
};

static struct platform_device mxc_nand_mtd_device = {
	.name = "mxc_nand_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_nand_data,
		},
};

static void mxc_init_nand_mtd(void)
{
	gpio_nand_active();
	(void)platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif

#if defined(CONFIG_FB_MXC_SYNC_PANEL) || defined(CONFIG_FB_MXC_SYNC_PANEL_MODULE)
static const char fb_default_mode[] = "Sharp-QVGA";

/* mxc lcd driver */
static struct platform_device mxc_fb_device = {
	.name = "mxc_sdc_fb",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &fb_default_mode,
		.coherent_dma_mask = 0xFFFFFFFF,
		},
};

static void mxc_init_fb(void)
{
	(void)platform_device_register(&mxc_fb_device);
}
#else
static inline void mxc_init_fb(void)
{
}
#endif

#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
/*!
 * The serial port definition structure. The fields contain:
 * {UART, CLK, PORT, IRQ, FLAGS}
 */
static struct plat_serial8250_port serial_platform_data[] = {
	{
	 .membase = (void __iomem *)(CS1_BASE_ADDR_VIRT + 0x00200000),
	 .mapbase = (unsigned long)(CS1_BASE_ADDR + 0x00200000),
	 .irq = IOMUX_TO_IRQ(MX21_PIN_TIN),
	 .uartclk = 3686400,
	 .regshift = 1,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	 },
};

/*!
 * REVISIT: document me
 */
static struct platform_device serial_device = {
	.name = "serial8250",
	.id = 0,
	.dev = {
		.platform_data = &serial_platform_data[0],
		},
};

/*!
 * REVISIT: document me
 */
static int __init mxc_init_extuart(void)
{
	gpio_config_mux(MX21_PIN_TIN, GPIO_MUX_GPIO);
	set_irq_type(serial_platform_data[0].irq, IRQT_RISING);
	return platform_device_register(&serial_device);
}
#else
static inline int mxc_init_extuart(void)
{
	return 0;
}
#endif

void mxc_init_enet(void)
{
	gpio_config_mux(MX21_PIN_UART3_RTS, GPIO_MUX_GPIO);
	set_irq_type(CS8900AIRQ, IRQT_RISING);
}

static void mxc_board_init(void)
{
	mxc_gpio_init();

	mxc_init_keypad();
	mxc_init_enet();
	mxc_init_nor_mtd();
	mxc_init_nand_mtd();
	mxc_init_extuart();
	mxc_init_fb();
}

static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
}

/* *INDENT-OFF* */
MACHINE_START(MX21ADS, "Freescale i.MX21 ADS")
	/* maintainer: Freescale Semiconductor, Inc. */
#ifdef CONFIG_SERIAL_8250_CONSOLE
	.phys_io        = CS1_BASE_ADDR,
	.io_pg_offst    = ((CS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
#else
	.phys_io        = AIPI_BASE_ADDR,
	.io_pg_offst    = ((AIPI_BASE_ADDR_VIRT) >> 18) & 0xfffc,
#endif
	.boot_params    = PHYS_OFFSET + 0x100,
	.fixup          = fixup_mxc_board,
	.map_io         = mxc_map_io,
	.init_irq       = mxc_init_irq,
	.init_machine   = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
