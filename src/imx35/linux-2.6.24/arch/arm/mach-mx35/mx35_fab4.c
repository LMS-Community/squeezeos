/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
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
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/mx35_fab4.h>

#include "board-mx35_fab4.h"
#include "crm_regs.h"
#include "iomux.h"

/*!
 * @file mach-mx35/mx35_fab4.c
 *
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup MSL_MX35
 */

unsigned int mx35_fab4_board_io;

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

/* MTD NAND flash */

#if defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)	\
|| defined(CONFIG_MTD_NAND_MXC_V2) || defined(CONFIG_MTD_NAND_MXC_V2_MODULE)

static struct mtd_partition mxc_nand_partitions_pa2[] = {
	{
	 .name = "redboot",
	 .offset = 0,
	 .size = 2 * 64 * 2048
	},
	{
	 .name = "ubi",
	 .offset = 2 * 64 * 2048,
	 .size = 1016 * 64 * 2048
	},
};

static struct mtd_partition mxc_nand_partitions_pa3[] = {
	{
	 .name = "redboot",
	 .offset = 0,
	 .size = 4 * 64 * 2048
	},
	{
	 .name = "ubi",
	 .offset = 4 * 64 * 2048,
	 .size = 1014 * 64 * 2048
	},
};

static struct flash_platform_data mxc_nand_data = {
	.parts = mxc_nand_partitions_pa3,
	.nr_parts = ARRAY_SIZE(mxc_nand_partitions_pa3),
	.width = 1,
};

static struct platform_device mxc_nand_mtd_device = {
	.name = "mxc_nandv2_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_nand_data,
		},
};

static void mxc_init_nand_mtd(void)
{
	if (__raw_readl(MXC_CCM_RCSR) & MXC_CCM_RCSR_NF16B)
		mxc_nand_data.width = 2;

	if (system_rev <= 2) {
		mxc_nand_data.parts = mxc_nand_partitions_pa2;
		mxc_nand_data.nr_parts = ARRAY_SIZE(mxc_nand_partitions_pa2);
	}

	platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif

static struct mxc_lcd_platform_data lcd_data = {
	.io_reg = "LCD"
};

static struct platform_device lcd_dev = {
	.name = "lcd_claa",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = (void *)&lcd_data,
		},
};

static void mxc_init_lcd(void)
{
	platform_device_register(&lcd_dev);
}

#if defined(CONFIG_FB_MXC_SYNC_PANEL) || defined(CONFIG_FB_MXC_SYNC_PANEL_MODULE)
static const char fb_default_mode[] = "Wintek-480x272";

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
#if defined(CONFIG_BACKLIGHT_MXC_IPU) || defined(CONFIG_BACKLIGHT_MXC_IPU_MODULE)
	{
	 .name = "mxc_ipu_bl",
	 .id = 0,
	 .dev = {
		 .platform_data = (void *)3,	/* DISP # for this backlight */
		 },
	 }
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

static struct resource ir_resources[] = {
	[0] = {
	       .start = MX35_PIN_ATA_DA2,
	       .end = MX35_PIN_ATA_DA2,
	       .flags = IORESOURCE_MEM,
	       .name = "xres",
	       },
	[1] = {
	       .start = MX35_PIN_ATA_DA1,
	       .end = MX35_PIN_ATA_DA1,
	       .flags = IORESOURCE_MEM,
	       .name = "sdata",
	       },
	[2] = {
	       .start = MX35_PIN_ATA_DA0,
	       .end = MX35_PIN_ATA_DA0,
	       .flags = IORESOURCE_MEM,
	       .name = "sclk",
	       },
};

static struct i2c_board_info mxc_i2c_board_info[] __initdata = {
	{
	 .driver_name = "clearpad",
	 .addr = 0x20,
	 .irq = IOMUX_TO_IRQ(MX35_PIN_I2C2_CLK),
	 },
	{
	  /* really a tmp100 */
	 .driver_name = "lm75",
	 .addr = 0x48,
	 },
	{
	 .driver_name = "tsl2569",
	 .addr = 0x39,
	 },
	{
	 .driver_name = "fab4-ir",
	 .addr = 0x47,
	 .platform_data = ir_resources,
	 .irq = IOMUX_TO_IRQ(MX35_PIN_I2C2_DAT),
	 },
	{
	 .driver_name = "wm8974-i2c",
	 .addr = 0x1A,
	 },
};

#if defined(CONFIG_FEC) || defined(CONFIG_FEC_MODULE)
unsigned int expio_intr_fec;

EXPORT_SYMBOL(expio_intr_fec);
#endif

#if defined(CONFIG_MMC_IMX_ESDHCI) || defined(CONFIG_MMC_IMX_ESDHCI_MODULE)
static struct mxc_mmc_platform_data mmc_data1 = {
	.ocr_mask = MMC_VDD_32_33,
	.min_clk = 400000,
	.max_clk = 52000000,
	.card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.clock_mmc = "sdhc_clk",
};

/*!
 * Resource definition for the SDHC1
 */
static struct resource mxcsdhc1_resources[] = {
	[0] = {
	       .start = MMC_SDHC1_BASE_ADDR,
	       .end = MMC_SDHC1_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_MMC_SDHC1,
	       .end = MXC_INT_MMC_SDHC1,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
		/* No card detect irq */
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Device Definition for MXC SDHC1 */
static struct platform_device mxcsdhc1_device = {
	.name = "mxsdhci",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data1,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc1_resources),
	.resource = mxcsdhc1_resources,
};

static struct mxc_mmc_platform_data mmc_data2 = {
	.ocr_mask = MMC_VDD_32_33,
	.min_clk = 400000,
	.max_clk = 52000000,
	.card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_get_card_wp_status,
	.clock_mmc = "sdhc_clk",
};

/*!
 * Resource definition for the SDHC2
 */
static struct resource mxcsdhc2_resources[] = {
	[0] = {
	       .start = MMC_SDHC2_BASE_ADDR,
	       .end = MMC_SDHC2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_MMC_SDHC2,
	       .end = MXC_INT_MMC_SDHC2,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = IOMUX_TO_IRQ(MX35_PIN_ATA_CS0),
	       .end = IOMUX_TO_IRQ(MX35_PIN_ATA_CS0),
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Device Definition for MXC SDHC2 */
static struct platform_device mxcsdhc2_device = {
	.name = "mxsdhci",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data2,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc2_resources),
	.resource = mxcsdhc2_resources,
};

static inline void mxc_init_mmc(void)
{
	/* WLAN */
	(void)platform_device_register(&mxcsdhc1_device);

	/* SD Card */
	(void)platform_device_register(&mxcsdhc2_device);
}
#else
static inline void mxc_init_mmc(void)
{
}
#endif


#ifdef CONFIG_SND_SOC_FAB4_AK4420
static void fab4_ak4420_set_clk(int sample_rate)
{
	switch (sample_rate) {
	case 96000:
	case 48000:
	case 32000:
	case 16000:
	case 8000:
		mxc_set_gpio_dataout(MX35_PIN_TX4_RX1, 1);
		break;

	default:
		mxc_set_gpio_dataout(MX35_PIN_TX4_RX1, 0);
		break;
	}
}

static struct resource ak4420_resources[] = {
	[0] = {
		.start = MX35_PIN_ATA_INTRQ,
		.end = MX35_PIN_ATA_INTRQ,
		.flags = IORESOURCE_MEM,
		.name = "smute",
	},
	[1] = {
		.start = MX35_PIN_ATA_RESET_B,
		.end = MX35_PIN_ATA_RESET_B,
		.flags = IORESOURCE_MEM,
		.name = "hdp_en",
	},
};

static struct mxc_audio_platform_data mxc_audio_data = {
	.ssi_num = 1,
	.src_port = 1,
	.ext_port = 4,
	.audio_set_clk = &fab4_ak4420_set_clk , /* OSC_SEL1 */
};

static struct platform_device mxc_alsa_ak4420_device = {
	.name = "fab4-ak4420",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_audio_data,
		},
	.num_resources = ARRAY_SIZE(ak4420_resources),
	.resource = ak4420_resources,
};

static void mxc_init_ak4420_audio(void)
{
	platform_device_register(&mxc_alsa_ak4420_device);
}
#else
static void mxc_init_ak4420_audio(void)
{
}
#endif


#ifdef CONFIG_SND_SOC_FAB4_WM8974
static void fab4_wm8974_set_clk(int sample_rate)
{
	switch (sample_rate) {
	case 96000:
	case 48000:
	case 32000:
	case 16000:
	case 8000:
		mxc_set_gpio_dataout(MX35_PIN_TX5_RX0, 1);
		break;

	default:
		mxc_set_gpio_dataout(MX35_PIN_TX5_RX0, 0);
		break;
	}
}

static struct mxc_audio_platform_data mxc_wm8974_data = {
	.ssi_num = 2,
	.src_port = 2,
	.ext_port = 5,
	.intr_id_hp = 0,
	.audio_set_clk = &fab4_wm8974_set_clk , /* OSC_SEL2 */
};

static struct platform_device mxc_alsa_wm8974_device = {
	.name = "fab4-wm8974",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_wm8974_data,
		},

};

static void mxc_init_wm8974_audio(void)
{
	platform_device_register(&mxc_alsa_wm8974_device);
}
#else
static void mxc_init_wm8974_audio(void)
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
	mxc_cpu_init();

#ifdef CONFIG_DISCONTIGMEM
	do {
		int nid;
		mi->nr_banks = MXC_NUMNODES;
		for (nid = 0; nid < mi->nr_banks; nid++)
			SET_NODE(mi, nid);
	} while (0);
#endif
}


/* PLATFORM DEVICES REVIEWED BELOW THIS POINT */



#ifdef CONFIG_MXC_FAB4_DEBUG_GPIO

/* GPIO debug  initialization. */
static struct mxc_fab4_gpio_pins fab4_gpio_pins[] = {
	{
		.name = "FAIL_SAFE#",
		.gpio = MX35_PIN_FST,
	},
	{
		.name = "SD_CD#",
		.gpio = MX35_PIN_ATA_CS0,
	},
	{
		.name = "SD_WP#",
		.gpio = MX35_PIN_ATA_DMARQ,
	},
	{
		.name = "I2C_ATTN1#",
		.gpio = MX35_PIN_I2C2_CLK,
	},
	{
		.name = "I2C_ATTN2#",
		.gpio = MX35_PIN_I2C2_DAT,
	},
	{
		.name = "IR_SENSOR#",
		.gpio = MX35_PIN_CAPTURE,
	},
	{
		.name = "IR_LED",
		.gpio = MX35_PIN_GPIO1_1,
	},
	{
		.name = "OSC_SEL2",
		.gpio = MX35_PIN_TX5_RX0,
	},
	{
		.name = "OSC_SEL1",
		.gpio = MX35_PIN_TX4_RX1,
	},
	{
		.name = "HDP_EN",
		.gpio = MX35_PIN_ATA_RESET_B,
	},
	{
		.name = "LCD_DISP",
		.gpio = MX35_PIN_D3_SPL,
	},
	{
		.name = "HDP_DET",
		.gpio = MX35_PIN_ATA_DIOW,
	},
	{
		.name = "WIFI_PD#",
		.gpio = MX35_PIN_ATA_DIOR,
	},
	{
		.name = "WIFI_RESET#",
		.gpio = MX35_PIN_ATA_CS1,
	},
	{
		.name = "DAC1_SMUTE",
		.gpio = MX35_PIN_ATA_INTRQ,
	},
};

static struct mxc_fab4_gpio_platform_data fab4_gpio_data = {
	.pins = fab4_gpio_pins,
	.nr_pins = ARRAY_SIZE(fab4_gpio_pins),
};

static struct platform_device fab4_gpio_device = {
	.name = "fab4_gpio",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &fab4_gpio_data,
		},
};

static inline void fab4_gpio_debug_init(void)
{
	platform_device_register(&fab4_gpio_device);
}
#else
static inline void fab4_gpio_debug_init(void)
{
}
#endif


/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	const char *rest;

	mxc_cpu_common_init();

	mxc_clocks_init();
	early_console_setup(saved_command_line);
	mxc_gpio_init();
	mx35_fab4_gpio_init();
	mxc_init_nand_mtd();

	mxc_init_lcd();
	mxc_init_fb();
	mxc_init_bl();

	i2c_register_board_info(0, mxc_i2c_board_info,
				ARRAY_SIZE(mxc_i2c_board_info));

	mxc_init_mmc();

	mxc_init_ak4420_audio();

	if (system_rev >= 3) {
		/* Changed to use i2c from PA3/PB1 build */
		mxc_init_wm8974_audio();
	}

	fab4_gpio_debug_init();

	switch (__raw_readl(MXC_CCM_RCSR) & 0xF) {
	case 0x0:
		rest = "Power On";
		break;
	case 0x4:
		rest = "External";
		break;
	case 0x8:
		rest = "Watchdog timeout";
		break;
	case 0x2:
		rest = "JTAG";
		break;
	default:
		rest = "unknown";
		break;
	}
	printk(KERN_INFO "Reset status: %s\n", rest);
}

#define PLL_PCTL_REG(brmo, pd, mfd, mfi, mfn)		\
		(((brmo) << 31) + (((pd) - 1) << 26) + (((mfd) - 1) << 16) + \
		((mfi)  << 10) + mfn)

/* For 24MHz input clock */
#define PLL_665MHZ		PLL_PCTL_REG(1, 1, 48, 13, 41)
#define PLL_532MHZ		PLL_PCTL_REG(1, 1, 12, 11, 1)
#define PLL_399MHZ		PLL_PCTL_REG(0, 1, 16, 8, 5)

/* working point(wp): 0,1 - 133MHz; 2,3 - 266MHz; 4,5 - 399MHz;*/
/* auto input clock table */
static struct cpu_wp cpu_wp_auto[] = {
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 133000000,
	 .pdr0_reg = (0x2 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 133000000,
	 .pdr0_reg = (0x6 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 266000000,
	 .pdr0_reg = (0x1 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 266000000,
	 .pdr0_reg = (0x5 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = (0x0 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = (0x6 << MXC_CCM_PDR0_AUTO_MUX_DIV_OFFSET),},
};

/* consumer input clock table */
static struct cpu_wp cpu_wp_con[] = {
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 133000000,
	 .pdr0_reg = (0x6 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 133000000,
	 .pdr0_reg = (0xE << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 266000000,
	 .pdr0_reg = (0x2 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 266000000,
	 .pdr0_reg = (0xA << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = (0x1 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = (0x9 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 532000000,
	 .pdr0_reg = (0x0 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 532000000,
	 .pdr0_reg = (0x8 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
	{
	 .pll_reg = PLL_665MHZ,
	 .pll_rate = 665000000,
	 .cpu_rate = 665000000,
	 .pdr0_reg = (0x7 << MXC_CCM_PDR0_CON_MUX_DIV_OFFSET),},
};

struct cpu_wp *get_cpu_wp(int *wp)
{
	if (cpu_is_mx35_rev(CHIP_REV_2_0) >= 1) {
                *wp = 9;
                return cpu_wp_con;
        } else {
		if (__raw_readl(MXC_CCM_PDR0) & MXC_CCM_PDR0_AUTO_CON) {
			*wp = 9;
			return cpu_wp_con;
		} else {
			*wp = 6;
			return cpu_wp_auto;
		}
	}
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_FAB4 data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(FAB4, "Logitech Fab4 Board")
	/* Maintainer: Logitech */
	.phys_io = AIPS1_BASE_ADDR,
	.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = fixup_mxc_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	.init_machine = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
