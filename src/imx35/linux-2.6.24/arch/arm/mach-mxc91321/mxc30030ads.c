/*
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/device.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/spi/spi.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/flash.h>
#endif

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/keypad.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/dsp_bringup.h>

#include "board-mxc30030ads.h"
#include "crm_regs.h"
#include "iomux.h"

/*!
 * @file mach-mxc91321/mxc30030ads.c
 *
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup MSL_MXC91321
 */

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);

static char command_line[COMMAND_LINE_SIZE];

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

static unsigned short mxckpd_keycodes[] = {
	KEY_9, KEY_F1, KEY_LEFTSHIFT, KEY_KPASTERISK, KEY_8, KEY_2, KEY_4,
	KEY_6, KEY_7, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
	KEY_5, KEY_F9, KEY_0, KEY_F10, KEY_F11, KEY_VOLUMEUP, KEY_VOLUMEDOWN,
	KEY_F12, KEY_RO, KEY_1, KEY_3, KEY_DOWN, KEY_UP, KEY_RIGHT,
	KEY_LEFT, KEY_ENTER,
};

static struct resource mxc_kpp_resources[] = {
	[0] = {
	       .start = MXC_INT_KPP,
	       .end = MXC_INT_KPP,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct keypad_data evb_8_by_4_keypad = {
	.rowmax = 8,
	.colmax = 4,
	.irq = MXC_INT_KPP,
	.learning = 1,
	.delay = 2,
	.matrix = mxckpd_keycodes
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_kpp_resources),
	.resource = mxc_kpp_resources,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &evb_8_by_4_keypad,
		},
};

static void mxc_init_keypad(void)
{
	(void)platform_device_register(&mxc_keypad_device);
}
#else
static inline void mxc_init_keypad(void)
{
}
#endif

#if defined(CONFIG_CS89x0) || defined(CONFIG_CS89x0_MODULE)
/*! Null terminated portlist used to probe for the CS8900A device on ISA Bus
 * Add 3 to reset the page window before probing (fixes eth probe when deployed
 * using nand_boot)
 */
unsigned int netcard_portlist[] = { CS8900A_BASE_ADDRESS + 3, 0 };

EXPORT_SYMBOL(netcard_portlist);
/*!
 * The CS8900A has 4 IRQ pins, which is software selectable, CS8900A interrupt
 * pin 0 is used for interrupt generation.
 */
unsigned int cs8900_irq_map[] = { CS8900AIRQ, 0, 0, 0 };

EXPORT_SYMBOL(cs8900_irq_map);

static int __init mxc_init_enet(void)
{
	iomux_config_mux(PIN_GPIO23, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
	return 0;
}
#else
static inline int mxc_init_enet(void)
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
	.width = 2,
	.parts = mxc_nor_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nor_partitions),
};

static struct resource mxc_flash_resource = {
	.start = 0xa0000000,
	.end = 0xa0000000 + 0x02000000 - 1,
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
	 .name = "IPL-SPL",
	 .offset = 0,
	 .size = 128 * 1024},
	{
	 .name = "nand.kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 4 * 1024 * 1024},
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
	if (__raw_readl(MXC_CCM_RCSR) & MXC_CCM_RCSR_NF16B) {
		mxc_nand_data.width = 2;
	}
	(void)platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif

static struct spi_board_info mxc_spi_board_info[] __initdata = {
	{
	 .modalias = "pmic_spi",
	 .irq = MXC_INT_EXT_INT1,
	 .max_speed_hz = 4000000,
	 .bus_num = 2,
	 .chip_select = 0,
	 },
};

/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{

	mxc_cpu_init();

	/* Store command line for use on mxc_board_init */
	strcpy(command_line, *cmdline);

#ifdef CONFIG_DISCONTIGMEM
	do {
		int nid;
		mi->nr_banks = MXC_NUMNODES;
		for (nid = 0; nid < mi->nr_banks; nid++) {
			SET_NODE(mi, nid);
		}
	} while (0);
#endif
}

/*!
 * This function is used to enable 26 mhz clock on CKO1. This pad
 * is connected to PMIC in order to derive all clocks
 * needed to make sound work (bit clock and frame sync clock)
 */
void mxc_init_pmic_clock(void)
{
	mxc_ccm_modify_reg(MXC_CCM_COSR,
			   (MXC_CCM_COSR_CKO1EN | MXC_CCM_COSR_CKO1S_MASK |
			    MXC_CCM_COSR_CKO1DV_MASK),
			   MXC_CCM_COSR_CKO1EN | 0x01);
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	mxc_cpu_common_init();
	/* Enable 26 mhz clock on CKO1 for PMIC audio */
	mxc_init_pmic_clock();

	mxc_gpio_init();
	mxc_init_enet();
	mxc_init_keypad();
	mxc_init_nor_mtd();
	mxc_init_nand_mtd();

	/* Assign USBPLL to be used by SSI1/2 */
	mxc_set_clocks_pll(SSI1_BAUD, USBPLL);
	mxc_set_clocks_pll(SSI2_BAUD, USBPLL);

	spi_register_board_info(mxc_spi_board_info,
				ARRAY_SIZE(mxc_spi_board_info));

	/* Search for dsp specific parameters from kernel's command line */
	if (dsp_parse_cmdline((const char *)command_line) != 0) {
		dsp_startapp_request();
	}
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MXC30030ADS data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MXC30030ADS, "Freescale MXC300-30 ADS")
    MAINTAINER("Freescale Semiconductor, Inc.")
    /*       physical memory    physical IO        virtual IO     */
    BOOT_MEM(PHYS_OFFSET, AIPS1_BASE_ADDR, AIPS1_BASE_ADDR_VIRT)
    BOOT_PARAMS(PHYS_OFFSET + 0x100)
    FIXUP(fixup_mxc_board)
    MAPIO(mxc_map_io)
    INITIRQ(mxc_init_irq)
    INIT_MACHINE(mxc_board_init)
    .timer = &mxc_timer,
MACHINE_END
