/*
 * Author: MontaVista Software, Inc.
 *       <source@mvista.com>
 *
 * Based on the OMAP devices.c
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>

#include <asm/hardware.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_power.h>
#include <asm/arch/mmc.h>

#include <asm/arch/spba.h>
#include "iomux.h"
#include <asm/arch/sdma.h>
#include <asm/arch/iim.h>

#ifdef CONFIG_ARCH_MXC91321
#include "sdma_script_code_pass2.h"
#else
#include "sdma_script_code.h"
#endif

#if 0
int board_device_enable(u32 device_id);
int board_device_disable(u32 device_id);

int mxc_device_enable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_enable(device_id);
	}

	return ret;
}

int mxc_device_disable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_disable(device_id);
	}
	return ret;
}

EXPORT_SYMBOL(mxc_device_enable);
EXPORT_SYMBOL(mxc_device_disable);
#endif

#ifndef CONFIG_MXC_DPTC
extern struct dptc_wp dptc_wp_allfreq[DPTC_WP_SUPPORTED];
#endif

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
	sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_patched_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = ap_2_bp_patched_ADDR;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = bp_2_ap_patched_ADDR;
	sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_patched_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_patched_ADDR;
	sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;
	sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_patched_ADDR;
	sdma_script_addr->mxc_sdma_start_addr = (unsigned short *)sdma_code;
	sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr =
	    uartsh_2_mcu_patched_ADDR;
	sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_patched_ADDR;
#ifdef CONFIG_ARCH_MXC91321
	sdma_script_addr->mxc_sdma_utra_addr = sdma_utra_ADDR;
#else
	sdma_script_addr->mxc_sdma_utra_addr = -1;
#endif
	sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
	sdma_script_addr->mxc_sdma_ram_code_start_addr = RAM_CODE_START_ADDR;
	sdma_script_addr->mxc_sdma_dptc_dvfs_addr = -1;
	sdma_script_addr->mxc_sdma_firi_2_mcu_addr = firi_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_app_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_shp_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_ata_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_firi_addr = mcu_2_firi_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = -1;
	sdma_script_addr->mxc_sdma_ata_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_uartsh_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_shp_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_uart_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_app_2_per_addr = -1;
}

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_MXC_HMP4E) || defined(CONFIG_MXC_HMP4E_MODULE)
static struct platform_device hmp4e_device = {
	.name = "mxc_hmp4e",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		}
};

static inline void mxc_init_hmp4e(void)
{
	u32 addr = IO_ADDRESS(IIM_BASE_ADDR + MXC_IIMHWV1);

	if (__raw_readl(addr) & MXC_IIMHWV1_HANTRO_DISABLE)
		__raw_writel(__raw_readl(addr) &
			     ~MXC_IIMHWV1_HANTRO_DISABLE, addr);

	if (__raw_readl(addr) & MXC_IIMHWV1_HANTRO_DISABLE) {
		printk(KERN_INFO "MPEG4 encoder fuse error\n");
		return;
	}

	platform_device_register(&hmp4e_device);
}
#else
static inline void mxc_init_hmp4e(void)
{
}
#endif

#if defined(CONFIG_W1_MASTER_MXC) || defined(CONFIG_W1_MASTER_MXC_MODULE)
static struct mxc_w1_config mxc_w1_data = {
	.search_rom_accelerator = 0,
};

static struct platform_device mxc_w1_devices = {
	.name = "mxc_w1",
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_w1_data,
		},
	.id = 0
};

static void mxc_init_owire(void)
{
	(void)platform_device_register(&mxc_w1_devices);
}
#else
static inline void mxc_init_owire(void)
{
}
#endif

#if defined(CONFIG_RTC_MXC) || defined(CONFIG_RTC_MXC_MODULE)
static struct resource rtc_resources[] = {
	{
	 .start = RTC_BASE_ADDR,
	 .end = RTC_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_RTC,
	 .flags = IORESOURCE_IRQ,
	 },
};
static struct platform_device mxc_rtc_device = {
	.name = "mxc_rtc",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(rtc_resources),
	.resource = rtc_resources,
};
static void mxc_init_rtc(void)
{
	(void)platform_device_register(&mxc_rtc_device);
}
#else
static inline void mxc_init_rtc(void)
{
}
#endif

#if defined(CONFIG_MXC_WATCHDOG) || defined(CONFIG_MXC_WATCHDOG_MODULE)

static struct resource wdt_resources[] = {
	{
	 .start = WDOG1_BASE_ADDR,
	 .end = WDOG1_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = WDOG2_BASE_ADDR,
	 .end = WDOG2_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_WDOG2,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_wdt_device = {
	.name = "mxc_wdt",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(wdt_resources),
	.resource = wdt_resources,
};

static void mxc_init_wdt(void)
{
	(void)platform_device_register(&mxc_wdt_device);
}
#else
static inline void mxc_init_wdt(void)
{
}
#endif

#if defined(CONFIG_MXC_IPU) || defined(CONFIG_MXC_IPU_MODULE)
static struct mxc_ipu_config mxc_ipu_data = {
	.rev = 1,
};

static struct resource ipu_resources[] = {
	{
	 .start = IPU_CTRL_BASE_ADDR,
	 .end = IPU_CTRL_BASE_ADDR + SZ_4K,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_IPU,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_ipu_device = {
	.name = "mxc_ipu",
	.id = -1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_ipu_data,
		},
	.num_resources = ARRAY_SIZE(ipu_resources),
	.resource = ipu_resources,
};

static void mxc_init_ipu(void)
{
	platform_device_register(&mxc_ipu_device);
}
#else
static inline void mxc_init_ipu(void)
{
}
#endif

/* MMC device data */

#if defined(CONFIG_MMC_MXC) || defined(CONFIG_MMC_MXC_MODULE)

extern unsigned int sdhc_get_card_det_status(struct device *dev);
extern int sdhc_init_card_det(int id);

static struct mxc_mmc_platform_data mmc_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30,
	.min_clk = 150000,
	.max_clk = 25000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
};

/*!
 * Resource definition for the SDHC1
 */
static struct resource mxcsdhc1_resources[] = {
	[0] = {
	       .start = MMC_SDHC1_BASE_ADDR,
	       .end = MMC_SDHC1_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_MMC_SDHC1,
	       .end = MXC_INT_MMC_SDHC1,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*!
 * Resource definition for the SDHC2
 */
static struct resource mxcsdhc2_resources[] = {
	[0] = {
	       .start = MMC_SDHC2_BASE_ADDR,
	       .end = MMC_SDHC2_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_MMC_SDHC2,
	       .end = MXC_INT_MMC_SDHC2,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Device Definition for MXC SDHC1 */
static struct platform_device mxcsdhc1_device = {
	.name = "mxcmci",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc1_resources),
	.resource = mxcsdhc1_resources,
};

/*! Device Definition for MXC SDHC2 */
static struct platform_device mxcsdhc2_device = {
	.name = "mxcmci",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc2_resources),
	.resource = mxcsdhc2_resources,
};

static inline void mxc_init_mmc(void)
{
	int cd_irq;

	cd_irq = sdhc_init_card_det(0);
	if (cd_irq) {
		mxcsdhc1_device.resource[2].start = cd_irq;
		mxcsdhc1_device.resource[2].end = cd_irq;
	}
	cd_irq = sdhc_init_card_det(1);
	if (cd_irq) {
		mxcsdhc2_device.resource[2].start = cd_irq;
		mxcsdhc2_device.resource[2].end = cd_irq;
	}

	spba_take_ownership(SPBA_SDHC1, SPBA_MASTER_A | SPBA_MASTER_C);
	(void)platform_device_register(&mxcsdhc1_device);
	spba_take_ownership(SPBA_SDHC2, SPBA_MASTER_A | SPBA_MASTER_C);
	(void)platform_device_register(&mxcsdhc2_device);
}
#else
static inline void mxc_init_mmc(void)
{
}
#endif

/*!
 * This is platform device structure for adding SCC
 */
#if defined(CONFIG_MXC_SECURITY_SCC) || defined(CONFIG_MXC_SECURITY_SCC_MODULE)
static struct platform_device mxc_scc_device = {
	.name = "mxc_scc",
	.id = 0,
};

static void mxc_init_scc(void)
{
	platform_device_register(&mxc_scc_device);
}
#else
static inline void mxc_init_scc(void)
{
}
#endif

#if defined(CONFIG_SND_MXC_PMIC) || defined(CONFIG_SND_MXC_PMIC_MODULE)
static struct mxc_audio_platform_data mxc_audio_data = {
	.ssi_num = 2,
	.src_port = 0,
};

static struct platform_device mxc_alsa_device = {
	.name = "mxc_alsa",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_audio_data,
		},

};

static void mxc_init_audio(void)
{
	platform_device_register(&mxc_alsa_device);
}
#else

static void mxc_init_audio(void)
{
}

#endif

#if defined(CONFIG_MXC_SSI) || defined(CONFIG_MXC_SSI_MODULE)
/*!
 * Resource definition for the SSI
 */
static struct resource mxcssi2_resources[] = {
	[0] = {
	       .start = SSI2_BASE_ADDR,
	       .end = SSI2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
};

static struct resource mxcssi1_resources[] = {
	[0] = {
	       .start = SSI1_BASE_ADDR,
	       .end = SSI1_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
};

/*! Device Definition for MXC SSI */
static struct platform_device mxc_ssi1_device = {
	.name = "mxc_ssi",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_audio_data,
		},
	.num_resources = ARRAY_SIZE(mxcssi1_resources),
	.resource = mxcssi1_resources,
};

static struct platform_device mxc_ssi2_device = {
	.name = "mxc_ssi",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_audio_data,
		},
	.num_resources = ARRAY_SIZE(mxcssi2_resources),
	.resource = mxcssi2_resources,
};

static void mxc_init_ssi(void)
{
	platform_device_register(&mxc_ssi1_device);
	platform_device_register(&mxc_ssi2_device);
}

#else
static void mxc_init_ssi(void)
{
}
#endif

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO1_BASE_ADDR),
	 .irq = MXC_INT_GPIO1,
	 .virtual_irq_start = MXC_GPIO_INT_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO2_BASE_ADDR),
	 .irq = MXC_INT_GPIO2,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN,
	 },
};

/* MU device data */

#if defined(CONFIG_MXC_MU) || defined(CONFIG_MXC_MU_MODULE)
#include <asm/arch/mxc_mu.h>

static struct resource mxc_mu_resources[] = {
	[0] = {
	       .start = MXC_INT_MU_RX_OR,
	       .flags = IORESOURCE_IRQ,
	       },
	[1] = {
	       .start = MXC_INT_MU_TX_OR,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = MXC_INT_MU0,
	       .flags = IORESOURCE_IRQ,
	       },
	[3] = {
	       .start = MXC_INT_MU1,
	       .flags = IORESOURCE_IRQ,
	       },
	[4] = {
	       .start = MXC_INT_MU2,
	       .flags = IORESOURCE_IRQ,
	       },
	[5] = {
	       .start = MXC_INT_MU3,
	       .flags = IORESOURCE_IRQ,
	       }
};

/*!
 * This is platform device structure for adding MU
 */
static struct platform_device mxc_mu_device = {
	.name = "mxc_mu",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(mxc_mu_resources),
	.resource = mxc_mu_resources,
};

static inline void mxc_init_mu(void)
{
	(void)platform_device_register(&mxc_mu_device);
}
#else
static inline void mxc_init_mu(void)
{
}
#endif

/* SPI controller and device data */
#if defined(CONFIG_SPI_MXC) || defined(CONFIG_SPI_MXC_MODULE)

#ifdef CONFIG_SPI_MXC_SELECT1
/*!
 * Resource definition for the CSPI1
 */
static struct resource mxcspi1_resources[] = {
	[0] = {
	       .start = CSPI1_BASE_ADDR,
	       .end = CSPI1_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_CSPI1,
	       .end = MXC_INT_CSPI1,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI1 */
static struct mxc_spi_master mxcspi1_data = {
	.maxchipselect = 4,
	.spi_version = 7,
};

/*! Device Definition for MXC CSPI1 */
static struct platform_device mxcspi1_device = {
	.name = "mxc_spi",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi1_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi1_resources),
	.resource = mxcspi1_resources,
};

#endif				/* CONFIG_SPI_MXC_SELECT1 */

#ifdef CONFIG_SPI_MXC_SELECT2
/*!
 * Resource definition for the CSPI2
 */
static struct resource mxcspi2_resources[] = {
	[0] = {
	       .start = CSPI2_BASE_ADDR,
	       .end = CSPI2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_CSPI2,
	       .end = MXC_INT_CSPI2,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI2 */
static struct mxc_spi_master mxcspi2_data = {
	.maxchipselect = 4,
	.spi_version = 7,
};

/*! Device Definition for MXC CSPI2 */
static struct platform_device mxcspi2_device = {
	.name = "mxc_spi",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi2_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi2_resources),
	.resource = mxcspi2_resources,
};
#endif				/* CONFIG_SPI_MXC_SELECT2 */

static inline void mxc_init_spi(void)
{
	/* SPBA configuration for CSPI2 - MCU is set */
	spba_take_ownership(SPBA_CSPI2, SPBA_MASTER_A);
#ifdef CONFIG_SPI_MXC_SELECT1
	if (platform_device_register(&mxcspi1_device) < 0)
		printk("Error: Registering the SPI Controller_1\n");
#endif				/* CONFIG_SPI_MXC_SELECT1 */
#ifdef CONFIG_SPI_MXC_SELECT2
	if (platform_device_register(&mxcspi2_device) < 0)
		printk("Error: Registering the SPI Controller_2\n");
#endif				/* CONFIG_SPI_MXC_SELECT2 */
}
#else
static inline void mxc_init_spi(void)
{
}
#endif

/* I2C controller and device data */
#if defined(CONFIG_I2C_MXC) || defined(CONFIG_I2C_MXC_MODULE)

/*!
 * Resource definition for the I2C1
 */
static struct resource mxci2c1_resources[] = {
	[0] = {
	       .start = I2C_BASE_ADDR,
	       .end = I2C_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_I2C,
	       .end = MXC_INT_I2C,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c1_data = {
	.i2c_clk = 100000,
};

/*! Device Definition for MXC I2C1 */
static struct platform_device mxci2c1_device = {
	.name = "mxc_i2c",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxci2c1_data,
		},
	.num_resources = ARRAY_SIZE(mxci2c1_resources),
	.resource = mxci2c1_resources,
};

static inline void mxc_init_i2c(void)
{
	if (platform_device_register(&mxci2c1_device) < 0)
		dev_err(&mxci2c1_device.dev, "Unable to register I2C device\n");
}
#else
static inline void mxc_init_i2c(void)
{
}
#endif

static struct platform_device mxc_dma_device = {
	.name = "mxc_dma",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
};

static inline void mxc_init_dma(void)
{
	(void)platform_device_register(&mxc_dma_device);
}

#ifndef CONFIG_MXC_DPTC
/*! Device Definition for DPTC */
static struct platform_device mxc_dptc_device = {
	.name = "mxc_dptc",
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &dptc_wp_allfreq,
		},
};

static inline void mxc_init_dptc(void)
{
	(void)platform_device_register(&mxc_dptc_device);
}
#endif

static int __init mxc_init_devices(void)
{
	mxc_init_wdt();
	mxc_init_ipu();
	mxc_init_mmc();
	mxc_init_mu();
	mxc_init_spi();
	mxc_init_i2c();
	mxc_init_ssi();
	mxc_init_rtc();
	mxc_init_owire();
	mxc_init_hmp4e();
	mxc_init_audio();
	mxc_init_dma();
	mxc_init_scc();
#ifndef CONFIG_MXC_DPTC
	mxc_init_dptc();
#endif

	/* SPBA configuration for SSI2 - SDMA and MCU are set */
	spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C | SPBA_MASTER_A);
	spba_take_ownership(SPBA_SAHARA, SPBA_MASTER_A);
	return 0;
}

arch_initcall(mxc_init_devices);
