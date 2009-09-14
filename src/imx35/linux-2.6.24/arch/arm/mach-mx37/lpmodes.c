/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mx37_lpmodes.c
 *
 * @brief Driver for the Freescale Semiconductor MXC low power modes setup.
 *
 * MX37 is designed to play and video with minimal power consumption.
 * This driver enables the platform to enter and exit audio and video low
 * power modes.
 *
 * @ingroup PM
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <asm/arch/clock.h>
#include <asm/arch/hardware.h>
#include <linux/regulator/regulator-platform.h>
#include "crm_regs.h"
#include <asm/arch/mxc_uart.h>


#define ARM_LP_CLK  200000000

static int org_cpu_rate;
int lp_video_mode;
int lp_audio_mode;
static struct device *lpmode_dev;

void enter_lp_video_mode(void)
{
	int ret = 0;

	struct clk *p_clk;
	struct clk *tclk;
	struct clk *vmode_parent_clk;
	struct regulator *gp_core;

	/* Set the periph_apm_clk parent to be lp_apm. */
	clk_set_parent(clk_get(NULL, "periph_apm_clk"),
							clk_get(NULL, "lp_apm"));
	tclk = clk_get(NULL, "main_bus_clk");
	vmode_parent_clk = clk_get(NULL, "pll2");
	p_clk = clk_get_parent(tclk);

	if (p_clk != vmode_parent_clk) {
		clk_set_parent(tclk, vmode_parent_clk);

		clk_set_rate(clk_get(NULL, "axi_a_clk"), 133000000);
		clk_set_rate(clk_get(NULL, "axi_b_clk"), 66500000);
		clk_set_rate(clk_get(NULL, "axi_c_clk"), 166000000);
		clk_set_rate(clk_get(NULL, "emi_core_clk"), 133000000);
		clk_set_rate(clk_get(NULL, "nfc_clk"), 26600000);
		clk_set_rate(clk_get(NULL, "ahb_clk"), 133000000);
	}

	/* Set UART clock's parent to be PLL2. */
	tclk = clk_get(NULL, "uart_main_clk");
	if (clk_get_parent(tclk) != vmode_parent_clk) {
		clk_set_parent(tclk, vmode_parent_clk);
		clk_set_rate(tclk, 66500000);
	}

	/* move VPU clock to source from the emi_core_clk */
	tclk = clk_get(NULL, "vpu_clk");
	vmode_parent_clk = clk_get(NULL, "emi_core_clk");
	if (clk_get_parent(tclk) != vmode_parent_clk)
		clk_set_parent(tclk, vmode_parent_clk);

	tclk = clk_get(NULL, "vpu_core_clk");
	if (clk_get_parent(tclk) != vmode_parent_clk)
		clk_set_parent(tclk, vmode_parent_clk);

	tclk = clk_get(NULL, "arm_axi_clk");
	if (clk_get_parent(tclk) != vmode_parent_clk)
		clk_set_parent(tclk, vmode_parent_clk);

	tclk = clk_get(NULL, "ddr_clk");
	vmode_parent_clk = clk_get(NULL, "axi_c_clk");
	if (clk_get_parent(tclk) != vmode_parent_clk)
		clk_set_parent(tclk, vmode_parent_clk);

	/* disable PLL3 */
	tclk = clk_get(NULL, "pll3");
	clk_disable(tclk);

	tclk = clk_get(NULL, "cpu_clk");
	org_cpu_rate = clk_get_rate(tclk);

	ret = clk_set_rate(tclk, ARM_LP_CLK);
	if (ret != 0)
		printk(KERN_DEBUG "cannot set CPU clock rate\n");

	/* Set the voltage to 0.8v for the GP domain. */

	gp_core = regulator_get(NULL, "DCDC1");

	ret = regulator_set_voltage(gp_core, 850000);
	if (ret < 0)
		printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!\n");
	lp_video_mode = 1;
}

void exit_lp_video_mode(void)
{
	int ret = 0;
	static struct clk *tclk;
	struct regulator *gp_core;

	/*Set the voltage to 0.8v for the GP domain. */
	gp_core = regulator_get(NULL, "DCDC1");

	ret = regulator_set_voltage(gp_core, 1000000);
	if (ret < 0)
		printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!\n");

	tclk = clk_get(NULL, "cpu_clk");

	ret = clk_set_rate(tclk, org_cpu_rate);
	if (ret != 0)
		printk(KERN_DEBUG "cannot set CPU clock rate\n");

	/* enable PLL3 */
	tclk = clk_get(NULL, "pll3");
	clk_enable(tclk);
	lp_video_mode = 0;
}

void enter_lp_audio_mode(void)
{
	int ret = 0;

	struct clk *p_clk;
	struct clk *tclk;
	struct clk *amode_parent_clk;
	struct regulator *gp_core;
	struct regulator *lp_core;
	u32 uart_addr = IO_ADDRESS(UART1_BASE_ADDR);
    u32 reg;


	tclk = clk_get(NULL, "ipu_clk");
	if (clk_get_usecount(tclk) != 0) {
		printk(KERN_INFO
		"Cannot enter AUDIO LPM mode - display is still active\n");
		return;
	}

	tclk = clk_get(NULL, "cpu_clk");
	org_cpu_rate = clk_get_rate(tclk);

	ret = clk_set_rate(tclk, ARM_LP_CLK);
	if (ret != 0)
		printk(KERN_DEBUG "cannot set CPU clock rate\n");

	/* Set the voltage to 0.8v for the GP domain. */
	gp_core = regulator_get(NULL, "DCDC1");

	if (gp_core != NULL) {
		ret = regulator_set_voltage(gp_core, 850000);
		if (ret < 0)
			printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!!\n");
	}


	tclk = clk_get(NULL, "periph_apm_clk");
	amode_parent_clk = clk_get(NULL, "lp_apm");
	p_clk = clk_get_parent(tclk);

	if (p_clk != amode_parent_clk) {
		/* Make sure osc_clk is the parent of lp_apm. */
		clk_set_parent(amode_parent_clk, clk_get(NULL, "osc"));

		/* Set the parent of periph_apm_clk to be lp_apm */
		clk_set_parent(tclk, amode_parent_clk);
		amode_parent_clk = clk_get(NULL, "periph_apm_clk");

		/* Set the parent of main_bus_clk to be periph_apm_clk */
		clk_set_parent(clk_get(NULL, "main_bus_clk"), amode_parent_clk);

		clk_set_rate(clk_get(NULL, "axi_a_clk"), 24000000);
		clk_set_rate(clk_get(NULL, "axi_b_clk"), 24000000);
		clk_set_rate(clk_get(NULL, "axi_c_clk"), 24000000);
		clk_set_rate(clk_get(NULL, "emi_core_clk"), 24000000);
		clk_set_rate(clk_get(NULL, "nfc_clk"), 4800000);
		clk_set_rate(clk_get(NULL, "ahb_clk"), 24000000);
	}

	amode_parent_clk = clk_get(NULL, "lp_apm");

	/* Set UART clock's parent to be lp_apm. */
	tclk = clk_get(NULL, "uart_main_clk");
	if (clk_get_parent(tclk) != amode_parent_clk) {
		clk_disable(tclk);
		clk_set_parent(tclk, amode_parent_clk);
		clk_enable(tclk);
		clk_set_rate(tclk, 24000000);
		/* Set the UART baud registers.
		 * Set the RFDIV to divide by 2.
		 */
		reg = __raw_readl(uart_addr + MXC_UARTUFCR);
		reg &= ~MXC_UARTUFCR_RFDIV_MASK;
		reg |= 4 << MXC_UARTUFCR_RFDIV_OFFSET;
		__raw_writel(reg, uart_addr + MXC_UARTUFCR);

		/* Setup the BAUD dividers for input clock at 24MHz and output
		 * at 115200
		 */
		reg = 0x0000047F;
		__raw_writel(reg, uart_addr + MXC_UARTUBIR);
		reg = 0x09a8;
	__raw_writel(reg, uart_addr + MXC_UARTUBMR);
	}

	amode_parent_clk = clk_get(NULL, "emi_core_clk");

	tclk = clk_get(NULL, "arm_axi_clk");
	if (clk_get_parent(tclk) != amode_parent_clk)
		clk_set_parent(tclk, amode_parent_clk);

	tclk = clk_get(NULL, "ddr_clk");
	if (clk_get_parent(tclk) != amode_parent_clk)
		clk_set_parent(tclk, amode_parent_clk);

	tclk = clk_get(NULL, "vpu_clk");
	if (clk_get_parent(tclk) != amode_parent_clk)
		clk_set_parent(tclk, amode_parent_clk);

	tclk = clk_get(NULL, "vpu_core_clk");
	if (clk_get_parent(tclk) != amode_parent_clk)
		clk_set_parent(tclk, amode_parent_clk);

	/* disable PLL3 */
	tclk = clk_get(NULL, "pll3");
	clk_disable(tclk);

	/* disable PLL2 */
	tclk = clk_get(NULL, "pll2");
	clk_disable(tclk);

	/* Set the voltage to 1.0v for the LP domain. */
	lp_core = regulator_get(NULL, "DCDC4");

	if (lp_core != NULL) {
		ret = regulator_set_voltage(lp_core, 1000000);
		if (ret < 0)
			printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!!!\n");
	}
	lp_audio_mode = 1;
}

void exit_lp_audio_mode(void)
{
	struct regulator *gp_core;
	struct regulator *lp_core;
	struct clk *tclk;
	struct clk *rmode_parent_clk;
	int ret;

	/* Set the voltage to 1.2v for the LP domain. */
	lp_core = regulator_get(NULL, "DCDC4");

	if (lp_core != NULL) {
		ret = regulator_set_voltage(lp_core, 1200000);
		if (ret < 0)
			printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!!!\n");
	}

	/* Set the voltage to 1.0v for the GP domain. */
	gp_core = regulator_get(NULL, "DCDC1");

	ret = regulator_set_voltage(gp_core, 1000000);
	if (ret < 0)
		printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!\n");

	tclk = clk_get(NULL, "cpu_clk");

	ret = clk_set_rate(tclk, org_cpu_rate);
	if (ret != 0)
		printk(KERN_DEBUG "cannot set CPU clock rate\n");

	clk_enable(clk_get(NULL, "pll3"));

	clk_enable(clk_get(NULL, "pll2"));

	clk_enable(clk_get(NULL, "ahbmux1_clk"));

	rmode_parent_clk = clk_get(NULL, "pll2");

	/* Set the dividers before setting the parent clock.*/
	clk_set_rate(clk_get(NULL, "axi_a_clk"), 4800000);
	clk_set_rate(clk_get(NULL, "axi_b_clk"), 4000000);
	clk_set_rate(clk_get(NULL, "axi_c_clk"), 6000000);


	clk_set_rate(clk_get(NULL, "emi_core_clk"), 4800000);
	clk_set_rate(clk_get(NULL, "ahb_clk"), 4800000);

	/* Set the parent of main_bus_clk to be pll2 */
	clk_set_parent(clk_get(NULL, "main_bus_clk"), rmode_parent_clk);

	tclk = clk_get(NULL, "ddr_clk");
	if (clk_get_parent(tclk) != clk_get(NULL, "axi_c_clk"))
		clk_set_parent(tclk, clk_get(NULL, "axi_c_clk"));

	tclk = clk_get(NULL, "arm_axi_clk");
	if (clk_get_parent(tclk) != clk_get(NULL, "emi_core_clk"))
		clk_set_parent(tclk, clk_get(NULL, "emi_core_clk"));

	/* Set the parent of periph_apm_clk to be pll1 */
	clk_set_parent(clk_get(NULL, "periph_apm_clk"),
						clk_get(NULL, "pll1_sw_clk"));

	/* Set UART clock's parent to be lp_apm. */
	tclk = clk_get(NULL, "uart_main_clk");
	if (clk_get_parent(tclk) != rmode_parent_clk) {
		u32 uart_addr = IO_ADDRESS(UART1_BASE_ADDR);
	    u32 reg;

		/*Set the divider before setting the parent. */
		clk_set_rate(tclk, 2400000);
		clk_set_parent(tclk, rmode_parent_clk);

		/*Set the RFDIV to divide by 6. */
		reg = __raw_readl(uart_addr + MXC_UARTUFCR);
		reg &= ~MXC_UARTUFCR_RFDIV_MASK;
		__raw_writel(reg, uart_addr + MXC_UARTUFCR);

		/*Setup the BAUD dividers for input clock at 24MHz and
		 * output at 115200
		 */
		reg = 0x0000047F;
		__raw_writel(reg, uart_addr + MXC_UARTUBIR);
		reg = 0x1b0e;
		__raw_writel(reg, uart_addr + MXC_UARTUBMR);
	}

	lp_audio_mode = 0;
}

static ssize_t lp_curr_mode(struct device *dev,
								struct device_attribute *attr, char *buf)
{
	if (lp_video_mode)
		return sprintf(buf, "in lp_video_mode\n");
	else if (lp_audio_mode)
		return sprintf(buf, "in lp_audio_mode\n");
    else
		return sprintf(buf, "in normal mode\n");
}

static ssize_t set_lp_mode(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t size)
{
	printk(KERN_DEBUG "In set_lp_mode() \n");

	if (strstr(buf, "enable_lp_video") != NULL) {
		if (!lp_video_mode)
			enter_lp_video_mode();
	} else if (strstr(buf, "disable_lp_video") != NULL) {
		if (lp_video_mode)
			exit_lp_video_mode();
	} else if (strstr(buf, "enable_lp_audio") != NULL) {
		if (!lp_audio_mode)
			enter_lp_audio_mode();
	} else if (strstr(buf, "disable_lp_audio") != NULL) {
		if (lp_audio_mode)
			exit_lp_audio_mode();
	}
	return size;
}

static DEVICE_ATTR(lp_modes, 0644, lp_curr_mode, set_lp_mode);


/*!
 * This is the probe routine for the lp_mode driver.
 *
 * @param   pdev   The platform device structure
 *
 * @return         The function returns 0 on success
 *
 */
static int __devinit mx37_lpmode_probe(struct platform_device *pdev)
{
	u32 res = 0;
	lpmode_dev = &pdev->dev;

	res = sysfs_create_file(&lpmode_dev->kobj, &dev_attr_lp_modes.attr);
	if (res) {
		printk(KERN_ERR
			"lpmode_dev: Unable to register sysdev entry for lpmode_dev");
		return res;
	}

	if (res != 0) {
		printk(KERN_ERR "lpmode_dev: Unable to start");
		return res;
	}
	lp_video_mode = 0;
	lp_audio_mode = 0;

	return 0;
}

static struct platform_driver mx37_lpmode_driver = {
	.driver = {
		   .name = "mx37_lpmode",
		   },
	.probe = mx37_lpmode_probe,
};

/*!
 * Initialise the mx37_lpmode_driver.
 *
 * @return  The function always returns 0.
 */

static int __init lpmode_init(void)
{
	if (platform_driver_register(&mx37_lpmode_driver) != 0) {
		printk(KERN_ERR "mx37_lpmode_driver register failed\n");
		return -ENODEV;
	}

	printk(KERN_INFO "LPMode driver module loaded\n");
	return 0;
}

static void __exit lpmode_cleanup(void)
{
	sysfs_remove_file(&lpmode_dev->kobj, &dev_attr_lp_modes.attr);

	/* Unregister the device structure */
	platform_driver_unregister(&mx37_lpmode_driver);
}

module_init(lpmode_init);
module_exit(lpmode_cleanup);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("LPMode driver");
MODULE_LICENSE("GPL");
