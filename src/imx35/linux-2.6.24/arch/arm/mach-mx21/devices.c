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

#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/mmc.h>

 /*!
  * @file devices.c
  * @brief device configurations including nor/nand/watchdog for mx27.
  *
  * @ingroup MSL
  */

/* FIXME: SDHC is not supported yet! */

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_MXC_WATCHDOG) || defined(CONFIG_MXC_WATCHDOG_MODULE)

static struct resource wdt_resources[] = {
	{
	 .start = WDOG_BASE_ADDR,
	 .end = WDOG_BASE_ADDR + 0x30,
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
	mxc_clks_enable(WDOG_CLK);
	(void)platform_device_register(&mxc_wdt_device);
}
#else
static inline void mxc_init_wdt(void)
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
	       .start = SDHC1_BASE_ADDR,
	       .end = SDHC1_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_SDHC1,
	       .end = INT_SDHC1,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
	[3] = {
	       .start = MXC_SDIO1_CARD_IRQ,
	       .end = MXC_SDIO1_CARD_IRQ,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*!
 * Resource definition for the SDHC2
 */
static struct resource mxcsdhc2_resources[] = {
	[0] = {
	       .start = SDHC2_BASE_ADDR,
	       .end = SDHC2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_SDHC2,
	       .end = INT_SDHC2,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
	[3] = {
	       .start = MXC_SDIO2_CARD_IRQ,
	       .end = MXC_SDIO2_CARD_IRQ,
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

	/*cd_irq = sdhc_init_card_det(0);
	   if (cd_irq) {
	   mxcsdhc1_device.resource[2].start = cd_irq;
	   mxcsdhc1_device.resource[2].end = cd_irq;
	   } */
	cd_irq = sdhc_init_card_det(1);
	if (cd_irq) {
		mxcsdhc2_device.resource[2].start = cd_irq;
		mxcsdhc2_device.resource[2].end = cd_irq;
	}

	/*(void)platform_device_register(&mxcsdhc1_device); */
	(void)platform_device_register(&mxcsdhc2_device);
}
#else
static inline void mxc_init_mmc(void)
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
	       .start = INT_I2C,
	       .end = INT_I2C,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c1_data = {
	.clk = I2C_CLK,
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

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR),
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x100,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN,
	 },
	{
	 .num = 2,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x200,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 2,
	 },
	{
	 .num = 3,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x300,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 3,
	 },
	{
	 .num = 4,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x400,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 4,
	 },
	{
	 .num = 5,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x500,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 5,
	 },
};

static int __init mxc_init_devices(void)
{
	mxc_init_wdt();
	mxc_init_mmc();
	mxc_init_i2c();
	return 0;
}

arch_initcall(mxc_init_devices);
