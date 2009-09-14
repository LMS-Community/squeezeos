/*
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/spi/spi.h>

#include <asm/hardware.h>

#include <asm/arch/spba.h>
#include "iomux.h"
#include <asm/arch/sdma.h>
#include "sdma_script_code.h"
#include <asm/arch/mxc_scc2_driver.h>

extern void gpio_spdif_active(void);

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
	sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = -1;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = -1;
	sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_ADDR;
	sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;
	sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_start_addr = (unsigned short *)sdma_code;
	sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr = uartsh_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
	sdma_script_addr->mxc_sdma_ram_code_start_addr = RAM_CODE_START_ADDR;
	sdma_script_addr->mxc_sdma_dptc_dvfs_addr = dptc_dvfs_ADDR;
	sdma_script_addr->mxc_sdma_firi_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_app_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_shp_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_ata_addr = mcu_2_ata_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = -1;
	sdma_script_addr->mxc_sdma_ata_2_mcu_addr = ata_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_uartsh_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_shp_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_uart_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_app_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_spdif_addr = mcu_2_spdif_marley_ADDR;
}

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

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

#if defined(CONFIG_RTC_DRV_MXC_V2) || defined(CONFIG_RTC_DRV_MXC_V2_MODULE)
static struct resource rtc_resources[] = {
	{
	 .start = SRTC_BASE_ADDR,
	 .end = SRTC_BASE_ADDR + 0x40,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_SRTC_NTZ,
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

#if defined(CONFIG_MXC_IPU_V3) || defined(CONFIG_MXC_IPU_V3_MODULE)
static struct mxc_ipu_config mxc_ipu_data = {
	.rev = 1,
};

static struct resource ipu_resources[] = {
	{
	 .start = IPU_CTRL_BASE_ADDR,
	 .end = IPU_CTRL_BASE_ADDR + SZ_512M,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_IPU_SYN,
	 .flags = IORESOURCE_IRQ,
	 },
	{
	 .start = MXC_INT_IPU_ERR,
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
	uint32_t reg_value;
	uint8_t *UMID_base;
	uint32_t *MAP_base;
	uint8_t i;
	uint32_t partition_no;
	uint32_t scc_partno;
	void *scm_ram_base;
	void *scc_base;

	scc_base = ioremap((uint32_t) SCC_BASE_ADDR, 0x140);
	if (scc_base == NULL) {
		printk(KERN_ERR "FAILED TO MAP IRAM REGS\n");
		return;
	}
	scm_ram_base = ioremap((uint32_t) IRAM_BASE_ADDR, IRAM_SIZE);
	if (scm_ram_base == NULL) {
		printk(KERN_ERR "FAILED TO MAP IRAM\n");
		return;
	}

	for (partition_no = 0; partition_no < 9; partition_no++) {
		reg_value = ((partition_no << SCM_ZCMD_PART_SHIFT) &
			     SCM_ZCMD_PART_MASK) | ((0x03 <<
						     SCM_ZCMD_CCMD_SHIFT)
						    & SCM_ZCMD_CCMD_MASK);
		__raw_writel(reg_value, scc_base + SCM_ZCMD_REG);

		while ((__raw_readl(scc_base + SCM_STATUS_REG) &
			SCM_STATUS_SRS_READY) != SCM_STATUS_SRS_READY) ;

		__raw_writel(0, scc_base + (SCM_SMID0_REG + 8 * partition_no));

		reg_value = __raw_readl(scc_base + SCM_PART_OWNERS_REG);

		if (((reg_value >> (2 * (partition_no))) & 3) != 3) {
			printk(KERN_ERR "FAILED TO ACQUIRE IRAM PARTITION\n");
			iounmap(scm_ram_base);
			return;
		}

		MAP_base = scm_ram_base + (partition_no * 0x2000);
		UMID_base = (uint8_t *) MAP_base + 0x10;

		for (i = 0; i < 16; i++)
			UMID_base[i] = 0;

		MAP_base[0] = SCM_PERM_NO_ZEROIZE | SCM_PERM_HD_SUP_DISABLE |
		    SCM_PERM_HD_READ | SCM_PERM_HD_WRITE |
		    SCM_PERM_TH_READ | SCM_PERM_TH_WRITE;

	}

	/*Freeing 2 partitions for SCC2*/
	scc_partno = 9 - (SCC_IRAM_SIZE / SZ_8K);
	for (partition_no = scc_partno; partition_no < 9; partition_no++) {
		reg_value = ((partition_no << SCM_ZCMD_PART_SHIFT) &
			     SCM_ZCMD_PART_MASK) | ((0x03 <<
						     SCM_ZCMD_CCMD_SHIFT)
						    & SCM_ZCMD_CCMD_MASK);
		__raw_writel(reg_value, scc_base + SCM_ZCMD_REG);

			 while ((__raw_readl(scc_base + SCM_STATUS_REG) &
				 SCM_STATUS_SRS_READY) != SCM_STATUS_SRS_READY);
	}
	iounmap(scm_ram_base);
	iounmap(scc_base);
	printk(KERN_INFO "IRAM READY\n");

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
	.spi_version = 23,
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
	.spi_version = 23,
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

#ifdef CONFIG_SPI_MXC_SELECT3
/*!
 * Resource definition for the CSPI3
 */
static struct resource mxcspi3_resources[] = {
	[0] = {
	       .start = CSPI3_BASE_ADDR,
	       .end = CSPI3_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_CSPI3,
	       .end = MXC_INT_CSPI3,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI3 */
static struct mxc_spi_master mxcspi3_data = {
	.maxchipselect = 4,
	.spi_version = 23,
};

/*! Device Definition for MXC CSPI3 */
static struct platform_device mxcspi3_device = {
	.name = "mxc_spi",
	.id = 2,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi3_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi3_resources),
	.resource = mxcspi3_resources,
};
#endif				/* CONFIG_SPI_MXC_SELECT3 */

void __init mxc_init_spi(void)
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
#ifdef CONFIG_SPI_MXC_SELECT3
	if (platform_device_register(&mxcspi3_device) < 0)
		printk("Error: Registering the SPI Controller_3\n");
#endif				/* CONFIG_SPI_MXC_SELECT3 */
}
#else
void __init mxc_init_spi(void)
{
}
#endif

/* I2C controller and device data */
#if defined(CONFIG_I2C_MXC) || defined(CONFIG_I2C_MXC_MODULE)

#ifdef CONFIG_I2C_MXC_SELECT1
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
#endif

#ifdef CONFIG_I2C_MXC_SELECT2
/*!
 * Resource definition for the I2C2
 */
static struct resource mxci2c2_resources[] = {
	[0] = {
	       .start = I2C2_BASE_ADDR,
	       .end = I2C2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_I2C2,
	       .end = MXC_INT_I2C2,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c2_data = {
	.i2c_clk = 100000,
};
#endif

#ifdef CONFIG_I2C_MXC_SELECT3
/*!
 * Resource definition for the I2C3
 */
static struct resource mxci2c3_resources[] = {
	[0] = {
	       .start = I2C3_BASE_ADDR,
	       .end = I2C3_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_I2C3,
	       .end = MXC_INT_I2C3,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c3_data = {
	.i2c_clk = 100000,
};
#endif

/*! Device Definition for MXC I2C1 */
static struct platform_device mxci2c_devices[] = {
#ifdef CONFIG_I2C_MXC_SELECT1
	{
	 .name = "mxc_i2c",
	 .id = 0,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c1_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c1_resources),
	 .resource = mxci2c1_resources,},
#endif
#ifdef CONFIG_I2C_MXC_SELECT2
	{
	 .name = "mxc_i2c",
	 .id = 1,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c2_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c2_resources),
	 .resource = mxci2c2_resources,},
#endif
#ifdef CONFIG_I2C_MXC_SELECT3
	{
	 .name = "mxc_i2c",
	 .id = 2,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c3_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c3_resources),
	 .resource = mxci2c3_resources,},
#endif
};

static inline void mxc_init_i2c(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mxci2c_devices); i++) {
		if (platform_device_register(&mxci2c_devices[i]) < 0)
			dev_err(&mxci2c_devices[i].dev,
				"Unable to register I2C device\n");
	}
}
#else
static inline void mxc_init_i2c(void)
{
}
#endif

static struct resource tve_resources[] = {
	{
	 .start = TVE_BASE_ADDR,
	 .end = TVE_BASE_ADDR + SZ_4K - 1,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_TVOUT,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_tve_device = {
	.name = "tve",
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(tve_resources),
	.resource = tve_resources,
};

void __init mxc_init_tve(void)
{
	platform_device_register(&mxc_tve_device);
}

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO1_BASE_ADDR),
	 .irq_0_15 = MXC_INT_GPIO1_LOW,
	 .irq_16_31 = MXC_INT_GPIO1_HIGH,
	 .virtual_irq_start = MXC_GPIO_INT_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO2_BASE_ADDR),
	 .irq_0_15 = MXC_INT_GPIO2_LOW,
	 .irq_16_31 = MXC_INT_GPIO2_HIGH,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 1,
	 },
	{
	 .num = 2,
	 .base = IO_ADDRESS(GPIO3_BASE_ADDR),
	 .irq_0_15 = MXC_INT_GPIO3_LOW,
	 .irq_16_31 = MXC_INT_GPIO3_HIGH,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 2,
	 },
};

#if defined(CONFIG_MXC_VPU) || defined(CONFIG_MXC_VPU_MODULE)
static struct resource vpu_resources[] = {
	{
	 .start = VPU_IRAM_BASE_ADDR,
	 .end = VPU_IRAM_BASE_ADDR + VPU_IRAM_SIZE,
	 .flags = IORESOURCE_MEM,
	 },
};

/*! Platform Data for MXC VPU */
static struct platform_device mxcvpu_device = {
	.name = "mxc_vpu",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(vpu_resources),
	.resource = vpu_resources,
};

static inline void mxc_init_vpu(void)
{
	if (platform_device_register(&mxcvpu_device) < 0)
		printk(KERN_ERR "Error: Registering the VPU.\n");
}
#else
static inline void mxc_init_vpu(void)
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

static struct resource spdif_resources[] = {
	{
	 .start = SPDIF_BASE_ADDR,
	 .end = SPDIF_BASE_ADDR + 0x50,
	 .flags = IORESOURCE_MEM,
	 },
};

static struct mxc_spdif_platform_data mxc_spdif_data = {
	.spdif_tx = 1,
	.spdif_rx = 0,
	.spdif_clk_44100 = 0,
	.spdif_clk_48000 = 3,
	.spdif_clk = NULL,
	.spdif_core_clk = NULL,
};

static struct platform_device mxc_alsa_spdif_device = {
	.name = "mxc_alsa_spdif",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_spdif_data,
		},
	.num_resources = ARRAY_SIZE(spdif_resources),
	.resource = spdif_resources,
};

static inline void mxc_init_spdif(void)
{
	struct clk *ckih_clk;
	ckih_clk = clk_get(NULL, "ckih");
	mxc_spdif_data.spdif_core_clk = clk_get(NULL, "spdif_xtal_clk");
	clk_set_parent(mxc_spdif_data.spdif_core_clk, ckih_clk);
	clk_put(ckih_clk);
	clk_put(mxc_spdif_data.spdif_core_clk);
	gpio_spdif_active();
	platform_device_register(&mxc_alsa_spdif_device);
}

static struct platform_device mx37_lpmode_device = {
	.name = "mx37_lpmode",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
};

static inline void mx37_init_lpmode(void)
{
	(void)platform_device_register(&mx37_lpmode_device);
}

static int __init mxc_init_devices(void)
{
	mxc_init_wdt();
	mxc_init_ipu();
	mxc_init_spi();
	mxc_init_i2c();
	mxc_init_rtc();
	mxc_init_owire();
	mxc_init_scc();
	mxc_init_dma();
	mxc_init_vpu();
	mxc_init_spdif();
	mxc_init_tve();
	mx37_init_lpmode();
	/* SPBA configuration for SSI2 - SDMA and MCU are set */
	spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C | SPBA_MASTER_A);
	return 0;
}

arch_initcall(mxc_init_devices);
