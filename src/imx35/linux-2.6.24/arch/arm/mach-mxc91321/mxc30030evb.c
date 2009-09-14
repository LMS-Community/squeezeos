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
#include <linux/serial_8250.h>
#ifdef CONFIG_KGDB
#include <linux/kgdb.h>
#endif
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/irq.h>
#include <linux/clk.h>
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
#include <asm/arch/dsp_bringup.h>
#include <asm/arch/spba.h>

#include "board-mxc30030evb.h"
#include "crm_regs.h"
#include "iomux.h"

/*!
 * @file mach-mxc91321/mxc30030evb.c
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
extern void gpio_pmic_power_off(void);
extern int mxc_clocks_init(void);
extern void __init early_console_setup(char *);

static char command_line[COMMAND_LINE_SIZE];

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/* Keypad keycodes for the EVB 8x8
 * keypad.  POWER and PTT keys don't generate
 * any interrupts via this driver so they are
 * not support. Change any keys as u like!
 */
static u16 keymapping[64] = {
	KEY_SELECT, KEY_LEFT, KEY_DOWN, KEY_RIGHT,
	KEY_UP, KEY_F12, KEY_END, KEY_BACK,
	KEY_F1, KEY_SENDFILE, KEY_HOME, KEY_F6,
	KEY_VOLUMEUP, KEY_F8, KEY_F9, KEY_F10,
	KEY_3, KEY_2, KEY_1, KEY_4,
	KEY_VOLUMEDOWN, KEY_7, KEY_5, KEY_6,
	KEY_9, KEY_LEFTSHIFT, KEY_8, KEY_0,
	KEY_KPASTERISK, KEY_RECORD, KEY_Q, KEY_W,
	KEY_A, KEY_S, KEY_D, KEY_E,
	KEY_F, KEY_R, KEY_T, KEY_Y,
	KEY_TAB, KEY_F7, KEY_CAPSLOCK, KEY_Z,
	KEY_X, KEY_C, KEY_V, KEY_G,
	KEY_B, KEY_H, KEY_N, KEY_M,
	KEY_J, KEY_K, KEY_U, KEY_I,
	KEY_SPACE, KEY_F2, KEY_DOT, KEY_ENTER,
	KEY_L, KEY_BACKSPACE, KEY_P, KEY_O,
};

static struct resource mxc_kpp_resources[] = {
	[0] = {
	       .start = MXC_INT_KPP,
	       .end = MXC_INT_KPP,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct keypad_data evb_8_by_8_keypad = {
	.rowmax = 8,
	.colmax = 8,
	.irq = MXC_INT_KPP,
	.learning = 0,
	.delay = 2,
	.matrix = keymapping
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_kpp_resources),
	.resource = mxc_kpp_resources,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &evb_8_by_8_keypad,
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

#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
/*!
 * The serial port definition structure. The fields contain:
 * {UART, CLK, PORT, IRQ, FLAGS}
 */
static struct plat_serial8250_port serial_platform_data[] = {
	{
	 .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTA),
	 .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTA),
	 .irq = MXC_INT_EXT_INT6,
	 .uartclk = 14745600,
	 .regshift = 1,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	 },
	{
	 .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTB),
	 .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTB),
	 .irq = MXC_INT_EXT_INT7,
	 .uartclk = 14745600,
	 .regshift = 1,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	 },
	{}
};

/*!
 * REVISIT: document me
 */
static struct platform_device serial_device = {
	.name = "serial8250",
	.id = 0,
	.dev = {
		.platform_data = serial_platform_data,
		},
};

/*!
 * REVISIT: document me
 */
static int __init mxc_init_extuart(void)
{
	/*!
	 * 16C652 UART-A and UART-B interrupt pins are connected to
	 * e_GPIO24 and e_GPIO25. They are routed to ED_INT6 and ED_INT7
	 * by configuring the IOMUX and EDIO.
	 */
	/* UART-A */
	iomux_config_mux(PIN_GPIO24, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
	set_irq_type(serial_platform_data[0].irq, IRQF_TRIGGER_HIGH);
	/* UART-B */
	iomux_config_mux(PIN_GPIO25, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
	set_irq_type(serial_platform_data[1].irq, IRQF_TRIGGER_HIGH);
	return platform_device_register(&serial_device);
}
#else
static inline int mxc_init_extuart(void)
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

#if defined(CONFIG_BACKLIGHT_MXC)
static struct platform_device mxcbl_devices[] = {
#if defined(CONFIG_BACKLIGHT_MXC_PMIC) || defined(CONFIG_BACKLIGHT_MXC_PMIC_MODULE)
	{
	 .name = "mxc_pmic_bl",
	 .id = 0,
	 .dev = {
		 .platform_data = (void *)-1,	/* DISP # for this backlight */
		 },
	 },
	{
	 .name = "mxc_pmic_bl",
	 .id = 1,
	 .dev = {
		 .platform_data = (void *)0,	/* DISP # for this backlight */
		 },
	 },
#endif
#if defined(CONFIG_BACKLIGHT_MXC_IPU) || defined(CONFIG_BACKLIGHT_MXC_IPU_MODULE)
	{
	 .name = "mxc_ipu_bl",
	 .id = 0,
	 .dev = {
		 .platform_data = (void *)3,	/* DISP # for this backlight */
		 },
	 },
#endif
};
static inline void mxc_init_bl(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(mxcbl_devices); i++) {
		platform_device_register(&mxcbl_devices[i]);
	}
}
#else
static inline void mxc_init_bl(void)
{
}
#endif

#if defined(CONFIG_MXC_FIR) || defined(CONFIG_MXC_FIR_MODULE)
/*!
 * Resource definition for the FIR
 */
static struct resource mxcir_resources[] = {
	[0] = {
	       .start = UART4_BASE_ADDR,
	       .end = UART4_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_UART4_FIRI_OR,
	       .end = MXC_INT_UART4_FIRI_OR,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = FIRI_BASE_ADDR,
	       .end = FIRI_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[3] = {
	       .start = MXC_INT_UART4_FIRI_OR,
	       .end = MXC_INT_UART4_FIRI_OR,
	       .flags = IORESOURCE_IRQ,
	       },
	[4] = {
	       .start = MXC_INT_UART4_FIRI_OR,
	       .end = MXC_INT_UART4_FIRI_OR,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mxc_ir_platform_data ir_data = {
	.uart_ir_mux = 0,
	.ir_rx_invert = MXC_IRDA_RX_INV,
	.ir_tx_invert = MXC_IRDA_TX_INV,
};

/*! Device Definition for MXC FIR */
static struct platform_device mxcir_device = {
	.name = "mxcir",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &ir_data,
		},
	.num_resources = ARRAY_SIZE(mxcir_resources),
	.resource = mxcir_resources,
};

static inline void mxc_init_ir(void)
{
	spba_take_ownership(SPBA_UART4, SPBA_MASTER_A | SPBA_MASTER_C);

	if (cpu_is_mxc91321_rev(CHIP_REV_1_2) > 0) {
		ir_data.uart_ir_mux = 1;
	}
	/* Get clock for UART4 */
	ir_data.uart_clk = clk_get(NULL, "uart_clk.3");
	(void)platform_device_register(&mxcir_device);
}
#else
static inline void mxc_init_ir(void)
{
}
#endif

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
#ifdef CONFIG_KGDB_8250
	int i;
	for (i = 0;
	     i <
	     (sizeof(serial_platform_data) / sizeof(serial_platform_data[0]));
	     i += 1)
		kgdb8250_add_platform_port(i, &serial_platform_data[i]);
#endif

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

/*
 * Default power-off for MXC91321
 */
static void mxc_power_off(void)
{
	gpio_pmic_power_off();
}

#if defined(CONFIG_MXC_PMIC_MC13783) && defined(CONFIG_SND_MXC_PMIC)
static void __init mxc_init_pmic_audio(void)
{
	struct clk *pll_clk;
	struct clk *ssi_clk;
	struct clk *ckih_clk;
	struct clk *cko_clk;

	/* Enable 26 mhz clock on CKO1 for PMIC audio */
	ckih_clk = clk_get(NULL, "ckih");
	cko_clk = clk_get(NULL, "cko1_clk");
	if (IS_ERR(ckih_clk) || IS_ERR(cko_clk)) {
		printk(KERN_ERR "Unable to set CKO1 output to CKIH\n");
	} else {
		clk_set_parent(cko_clk, ckih_clk);
		clk_set_rate(cko_clk, clk_get_rate(ckih_clk));
		clk_enable(cko_clk);
	}
	clk_put(ckih_clk);
	clk_put(cko_clk);

	/* Assign USBPLL to be used by SSI1/2 */
	pll_clk = clk_get(NULL, "usb_pll");
	ssi_clk = clk_get(NULL, "ssi_clk.0");
	clk_set_parent(ssi_clk, pll_clk);
	clk_enable(ssi_clk);
	clk_put(ssi_clk);

	ssi_clk = clk_get(NULL, "ssi_clk.1");
	clk_set_parent(ssi_clk, pll_clk);
	clk_enable(ssi_clk);
	clk_put(ssi_clk);
	clk_put(pll_clk);
}
#else
static void __inline mxc_init_pmic_audio(void)
{
}
#endif

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	mxc_cpu_common_init();
	mxc_clocks_init();
	early_console_setup(saved_command_line);

	/* Enable 26 mhz clock on CKO1 for PMIC audio */
	mxc_init_pmic_audio();

	pm_power_off = mxc_power_off;

	mxc_gpio_init();
	mxc_init_keypad();
	mxc_init_extuart();
	mxc_init_enet();
	mxc_init_nor_mtd();
	mxc_init_nand_mtd();

	spi_register_board_info(mxc_spi_board_info,
				ARRAY_SIZE(mxc_spi_board_info));

	mxc_init_fb();
	mxc_init_bl();
	mxc_init_ir();
	/* Search for dsp specific parameters from kernel's command line */
	if (dsp_parse_cmdline((const char *)command_line) != 0) {
		dsp_startapp_request();
	}
}

#define MPDR0_REG(brmm, max, tpsel)       \
        (MXC_CCM_MPDR0_BRMM_##brmm | MXC_CCM_MPDR0_MAX_PDF_##max | \
         MXC_CCM_MPDR0_TPSEL_##tpsel)

/* working point(wp): 0 - 399MHz; 1 - 532MHz */
/* for ahb clock 133MHz */

static struct cpu_wp cpu_wp_133[] = {
	{
	 .pll_rate = 399000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = MPDR0_REG(0, 3, 0),},
	{
	 .pll_rate = 532000000,
	 .cpu_rate = 532000000,
	 .pdr0_reg = MPDR0_REG(0, 4, 1),},
};

struct cpu_wp *get_cpu_wp(int *wp)
{
	*wp = 2;
	return cpu_wp_133;
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MXC30030EVB data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MXC30030EVB, "Freescale MXC300-30 EVB")
	/* maintainer: Freescale Semiconductor, Inc. */
#ifdef CONFIG_SERIAL_8250_CONSOLE
	.phys_io        = CS4_BASE_ADDR,
	.io_pg_offst    = ((CS4_BASE_ADDR_VIRT) >> 18) & 0xfffc,
#else
	.phys_io        = AIPS1_BASE_ADDR,
	.io_pg_offst    = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
#endif
	.boot_params    = PHYS_OFFSET + 0x100,
	.fixup          = fixup_mxc_board,
	.map_io         = mxc_map_io,
	.init_irq       = mxc_init_irq,
	.init_machine   = mxc_board_init,
	.timer          = &mxc_timer,
MACHINE_END
