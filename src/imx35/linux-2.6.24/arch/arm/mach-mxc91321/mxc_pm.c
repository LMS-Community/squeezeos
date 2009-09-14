/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup DPM_MXC91321 Power Management
 * @ingroup MSL_MXC91321
 */
/*!
 * @file mach-mxc91321/mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CCM_MCU, DPLL and DSM modules.
 *
 * @ingroup DPM_MXC91321
 */

/*
 * Include Files
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/sysdev.h>

#include <asm/arch/mxc_pm.h>
#include <asm/arch/system.h>
#include <asm/hardware.h>
#include <asm/arch/dptc.h>
#include <asm/arch/pmic_power.h>

#include "crm_regs.h"

#define CORE_TURBO	532000000
#define CORE_NORMAL	399000000

/*
 * GPIO calls
 */
extern void gpio_pmhs_active(void);
extern void gpio_pmhs_inactive(void);

extern int mxc_jtag_enabled;

/*
 * Clock structures
 */
static struct clk *mcu_clk;

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   armfreq    The desired ARM frequency
 * @param   ahbfreq    Not used. It is constant value at 133 MHz
 * @param   ipfreq     Not used. It is constant value at 66.5 MHz
 *
 * @returns             Returns 0 on success
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int ret_val = -1;

	if (armfreq == CORE_TURBO) {

#ifdef CONFIG_MXC_DPTC
		/*
		 * Adjust voltage
		 */
		dptc_set_turbo_mode(1);
#endif
	}

	/* Changing Frequency/Voltage */
	if (armfreq != clk_get_rate(mcu_clk)) {
		ret_val = clk_set_rate(mcu_clk, armfreq);
	}

	if (armfreq == CORE_TURBO) {
#ifdef CONFIG_MXC_DPTC
		/*
		 * Resume DPTC
		 */
		dptc_resume();
#else
		/*
		 * Resume DPTC
		 */
		dptc_enable();
#endif

	} else {
#ifdef CONFIG_MXC_DPTC
		/*
		 * Suspend DPTC and adjust voltage
		 */
		dptc_suspend();
		dptc_set_turbo_mode(0);
#else
		/*
		 * Suspend DPTC and adjust voltage
		 */
		dptc_disable();
#endif

	}
	return ret_val;
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param   mode    The desired low-power mode. Possible values are,
 *                  WAIT_MODE, DOZE_MODE, STOP_MODE or DSM_MODE
 *
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int crm_ctrl;
	unsigned long flags;

	if (mxc_jtag_enabled == 1) {
		/*
		 * If JTAG is connected then WFI is not enabled
		 * So return
		 */
		return;
	}

	/* Check parameter */
	if ((mode != WAIT_MODE) && (mode != STOP_MODE) && (mode != DSM_MODE) &&
	    (mode != DOZE_MODE)) {
		return;
	}

	local_irq_save(flags);

	switch (mode) {
	case WAIT_MODE:
		/*
		 * Handled by kernel using arch_idle()
		 * There are no LPM bits for WAIT mode which
		 * are set by software
		 */
		break;
	case DOZE_MODE:
		/* Writing '10' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_2));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	case STOP_MODE:
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	case DSM_MODE:
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_WBEN)),
			     MXC_CCM_MCR);
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_SBYOS)),
			     MXC_CCM_MCR);
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	}
	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	cpu_do_idle();

	local_irq_restore(flags);
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

static void pmic_voltage_init(void)
{
	t_regulator_cfg_param cfgp;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_get_config(cfgp.regulator, &cfgp.cfg);

	cfgp.cfg.voltage.sw1a = SW1A_1_5V;
	cfgp.cfg.voltage_lvs.sw1a = SW1A_1_6V;
	cfgp.cfg.voltage_stby.sw1a = SW1A_1V;
	cfgp.cfg.dvs_speed = DVS_4US;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_set_config(cfgp.regulator, &cfgp.cfg);
}

static ssize_t dvfs_cpu_clk_store(struct sys_device *dev, const char *buf,
				  size_t size)
{
	if (strstr(buf, "399000000") != NULL) {
		mxc_pm_dvfs(CORE_NORMAL, 0, 0);
	} else if (strstr(buf, "532000000") != NULL) {
		mxc_pm_dvfs(CORE_TURBO, 0, 0);
	} else {
		pr_info("\ncpu clock %s is not supported\n", buf);
	}

	return size;
}

static SYSDEV_ATTR(cpu_clk, 0200, NULL, dvfs_cpu_clk_store);

static struct sysdev_class dvfs_sysclass = {
	set_kset_name("dvfs"),
};

static struct sys_device dvfs_device = {
	.id = 0,
	.cls = &dvfs_sysclass,
};

static int dvfs_sysdev_ctrl_init(void)
{
	int err;

	err = sysdev_class_register(&dvfs_sysclass);
	if (!err)
		err = sysdev_register(&dvfs_device);
	if (!err) {
		err = sysdev_create_file(&dvfs_device, &attr_cpu_clk);
	}

	return err;
}

static void dvfs_sysdev_ctrl_exit(void)
{
	sysdev_remove_file(&dvfs_device, &attr_cpu_clk);
	sysdev_unregister(&dvfs_device);
	sysdev_class_unregister(&dvfs_sysclass);
}

/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
	int err;
	unsigned long pmcr0;

	pmic_voltage_init();

	mcu_clk = clk_get(NULL, "cpu_clk");

	while ((__raw_readl(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DFSP) != 0) ;

	/*
	 * Set DVSE bit to enable Dynamic Voltage Scaling
	 */
	pmcr0 = __raw_readl(MXC_CCM_PMCR0);
	pmcr0 = pmcr0 | MXC_CCM_PMCR0_DVSE | MXC_CCM_PMCR0_PMICHE |
	    MXC_CCM_PMCR0_EMIHE | MXC_CCM_PMCR0_DFSIM;
	__raw_writel(pmcr0, MXC_CCM_PMCR0);

	/*
	 * IOMUX configuration for GPIO9 and GPIO20
	 */
	gpio_pmhs_active();

	err = dvfs_sysdev_ctrl_init();
	if (err) {
		printk(KERN_ERR
		       "DVFS: Unable to register sysdev entry for dvfs");
		return err;
	}
	pr_info("Low-Level PM Driver module loaded\n");
	return 0;
}

/*!
 * This function is used to unload the module
 */
static void __exit mxc_pm_cleanup_module(void)
{
	gpio_pmhs_inactive();
	dvfs_sysdev_ctrl_exit();

	pr_info("Low-Level PM Driver module Unloaded\n");
}

module_init(mxc_pm_init_module);
module_exit(mxc_pm_cleanup_module);

EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_lowpower);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_intscale);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91321 Low-level PM Driver");
MODULE_LICENSE("GPL");
