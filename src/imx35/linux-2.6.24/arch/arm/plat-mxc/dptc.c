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
 * @file dptc.c
 *
 * @brief Driver for the Freescale Semiconductor MXC DPTC module.
 *
 * The DPTC driver is designed to control the MXC DPTC hardware.
 * hardware. Upon initialization, the DPTC driver initializes the DPTC hardware
 * sets up driver nodes attaches to the DPTC interrupt and initializes internal
 * data structures. When the DPTC interrupt occurs the driver checks the cause
 * of the interrupt (lower frequency, increase frequency or emergency) and changes
 * the CPU voltage according to translation table that is loaded into the driver.
 * The driver read method is used to read the log buffer.
 * Driver ioctls are used to change driver parameters and enable/disable the
 * DVFS operation.
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
#include <asm/arch/gpio.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_power.h>
#include <asm/arch/hardware.h>

#if defined(CONFIG_ARCH_MX3)
#include "../mach-mx3/crm_regs.h"
#elif defined(CONFIG_ARCH_MXC91321)
#include "../mach-mxc91321/crm_regs.h"
#else
#include "../mach-mx27/crm_regs.h"
#endif

static int dptc_is_active;
static int turbo_mode_active;

static int curr_wp;
static u32 ptvai;
static struct delayed_work dptc_work;
static struct dptc_wp *dptc_wp_allfreq;
static struct device *dptc_dev;
static struct clk *cpu_clk;

DEFINE_SPINLOCK(mxc_dptc_lock);

enum {
	DPTC_PTVAI_NOCHANGE = 0x0,
	DPTC_PTVAI_DECREASE,
	DPTC_PTVAI_INCREASE,
	DPTC_PTVAI_EMERG,
};

static void update_dptc_wp(u32 wp)
{
	t_pmic_regulator regulator;
	t_regulator_voltage voltage;

	regulator = dptc_wp_allfreq[wp].regulator;
	voltage = (t_regulator_voltage) dptc_wp_allfreq[wp].voltage;

	__raw_writel(dptc_wp_allfreq[wp].dcvr0, MXC_CCM_DCVR0);
	__raw_writel(dptc_wp_allfreq[wp].dcvr1, MXC_CCM_DCVR1);
	__raw_writel(dptc_wp_allfreq[wp].dcvr2, MXC_CCM_DCVR2);
	__raw_writel(dptc_wp_allfreq[wp].dcvr3, MXC_CCM_DCVR3);

	if (cpu_is_mx31()) {
		pmic_power_regulator_set_voltage(SW_SW1A, voltage);
	} else if (cpu_is_mxc91321()) {
		pmic_power_switcher_set_dvs(SW_SW1A, voltage);
	} else {
		pmic_power_regulator_set_voltage(SW_SW1A, voltage);
	}

	pr_debug("dcvr0-3: 0x%x, 0x%x, 0x%x, 0x%x; vol: %d\n",
		 dptc_wp_allfreq[wp].dcvr0,
		 dptc_wp_allfreq[wp].dcvr1,
		 dptc_wp_allfreq[wp].dcvr2,
		 dptc_wp_allfreq[wp].dcvr3, dptc_wp_allfreq[wp].voltage);
}

static irqreturn_t dptc_irq(int irq, void *dev_id)
{
	u32 pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	ptvai = (pmcr0 & MXC_CCM_PMCR0_PTVAI_MASK) >>
	    MXC_CCM_PMCR0_PTVAI_OFFSET;

	pr_debug("dptc_irq: ptvai = 0x%x (0x%x)!!!!!!!\n", ptvai, pmcr0);

	/* disable DPTC and mask its interrupt */
	pmcr0 = (pmcr0 & ~(MXC_CCM_PMCR0_DPTEN)) | MXC_CCM_PMCR0_PTVAIM;
	__raw_writel(pmcr0, MXC_CCM_PMCR0);

	schedule_delayed_work(&dptc_work, 0);

	return IRQ_RETVAL(1);
}

static void dptc_workqueue_handler(struct work_struct *work)
{
	u32 pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	if (!cpu_is_mxc91321() || (turbo_mode_active == 1)) {
		switch (ptvai) {
		case DPTC_PTVAI_DECREASE:
			curr_wp++;
			break;
		case DPTC_PTVAI_INCREASE:
		case DPTC_PTVAI_EMERG:
			curr_wp--;
			if (curr_wp < 0) {
				/* already max voltage */
				curr_wp = 0;
				printk(KERN_WARNING
				       "dptc: already maximum voltage\n");
			}
			break;

			/* Unknown interrupt cause */
		default:
			BUG();
		}

		if (curr_wp > DPTC_WP_SUPPORTED || curr_wp < 0) {
			panic("Can't support this working point: %d\n",
			      curr_wp);
		}
		update_dptc_wp(curr_wp);

		/* enable DPTC and unmask its interrupt */
		pmcr0 = (pmcr0 & ~(MXC_CCM_PMCR0_PTVAIM)) | MXC_CCM_PMCR0_DPTEN;
		__raw_writel(pmcr0, MXC_CCM_PMCR0);
	}
}

/* Start DPTC unconditionally */
static int start_dptc(void)
{
	u32 pmcr0, flags;
	unsigned long cpu_rate;

	spin_lock_irqsave(&mxc_dptc_lock, flags);

	cpu_rate = clk_get_rate(cpu_clk);

	if (cpu_is_mx27() && cpu_rate < 399000000) {
		goto err;
	} else if (cpu_rate < 532000000) {
		goto err;
	}

	pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	/* enable DPTC and unmask its interrupt */
	pmcr0 = ((pmcr0 & ~(MXC_CCM_PMCR0_PTVAIM)) | MXC_CCM_PMCR0_DPTEN) |
	    (MXC_CCM_PMCR0_DPVCR | MXC_CCM_PMCR0_DPVV);

	__raw_writel(pmcr0, MXC_CCM_PMCR0);

	spin_unlock_irqrestore(&mxc_dptc_lock, flags);

	pr_info("DPTC has been started \n");

	return 0;

      err:
	spin_unlock_irqrestore(&mxc_dptc_lock, flags);
	pr_info("Core is not running in Turbo mode\n");
	pr_info("DPTC is not enabled\n");
	return -1;
}

/* Stop DPTC unconditionally */
static void stop_dptc(void)
{
	u32 pmcr0;

	pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	/* disable DPTC and mask its interrupt */
	pmcr0 = ((pmcr0 & ~(MXC_CCM_PMCR0_DPTEN)) | MXC_CCM_PMCR0_PTVAIM) &
	    (~MXC_CCM_PMCR0_DPVCR);

	__raw_writel(pmcr0, MXC_CCM_PMCR0);

	/* Restore Turbo Mode voltage to highest wp */
	update_dptc_wp(0);
	curr_wp = 0;

	pr_info("DPTC has been stopped\n");
}

/*
  this function does not change the working point. It can be
 called from an interrupt context.
*/
void dptc_suspend(void)
{
	u32 pmcr0;

	if (!dptc_is_active)
		return;

	pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	/* disable DPTC and mask its interrupt */
	pmcr0 = ((pmcr0 & ~(MXC_CCM_PMCR0_DPTEN)) | MXC_CCM_PMCR0_PTVAIM) &
	    (~MXC_CCM_PMCR0_DPVCR);

	__raw_writel(pmcr0, MXC_CCM_PMCR0);
}

/*!
 * This function is called to put the DPTC in a low power state.
 *
 */
void dptc_disable(void)
{
	if (!dptc_is_active)
		return;

	stop_dptc();
	dptc_is_active = 0;
	turbo_mode_active = 0;
}

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 */
void dptc_enable(void)
{
	if (dptc_is_active)
		return;
	start_dptc();
	dptc_is_active = 1;
	turbo_mode_active = 1;
}

static ssize_t dptc_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	if (dptc_is_active)
		return sprintf(buf, "DPTC is enabled\n");
	else
		return sprintf(buf, "DPTC is disabled\n");
}

static ssize_t dptc_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t size)
{
	if (strstr(buf, "0") != NULL) {
		dptc_disable();
	} else if (strstr(buf, "1") != NULL) {
		dptc_enable();
	}

	return size;
}

static DEVICE_ATTR(enable, 0644, dptc_show, dptc_store);

/*!
 * This is the probe routine for the DPTC driver.
 *
 * @param   pdev   The platform device structure
 *
 * @return         The function returns 0 on success
 *
 */
static int __devinit mxc_dptc_probe(struct platform_device *pdev)
{
	int res = 0;
	u32 pmcr0 = __raw_readl(MXC_CCM_PMCR0);
	struct clk *ckih_clk;

	dptc_dev = &pdev->dev;
	dptc_wp_allfreq = pdev->dev.platform_data;
	if (dptc_wp_allfreq == NULL) {
		ckih_clk = clk_get(NULL, "ckih");
		if (cpu_is_mx31() &
		    (mxc_cpu_is_rev(CHIP_REV_2_0) < 0) &
		    (clk_get_rate(ckih_clk) == 27000000))
			printk(KERN_ERR "DPTC: DPTC not supported on TO1.x \
					& ckih = 27M\n");
		else
			printk(KERN_ERR "DPTC: Pointer to DPTC table is NULL\
					not started\n");
		return -1;
	}

	INIT_DELAYED_WORK(&dptc_work, dptc_workqueue_handler);

	/* request the DPTC interrupt */
	res =
	    request_irq(MXC_INT_CCM, dptc_irq, IRQF_DISABLED, "mxc-dptc", NULL);
	if (res) {
		printk(KERN_ERR "DPTC: Unable to attach to DPTC interrupt");
		return res;
	}

	if (cpu_is_mx31()) {
		/* 256 system clock count; ARM interrupt; enable all 2 rf circuits */
		pmcr0 = (pmcr0 & ~(MXC_CCM_PMCR0_DCR)) | MXC_CCM_PMCR0_PTVIS |
		    MXC_CCM_PMCR0_DRCE3 | MXC_CCM_PMCR0_DRCE1;

	} else if (cpu_is_mxc91321()) {
		pmcr0 = (pmcr0 | MXC_CCM_PMCR0_DRCE3) | MXC_CCM_PMCR0_DRCE2 |
		    MXC_CCM_PMCR0_DRCE1;
	} else if (cpu_is_mx27()) {
		pmcr0 =
		    (pmcr0 & ~(MXC_CCM_PMCR0_DCR)) | MXC_CCM_PMCR0_DRCE3 |
		    MXC_CCM_PMCR0_DRCE1;
	}

	__raw_writel(pmcr0, MXC_CCM_PMCR0);

	res = sysfs_create_file(&dptc_dev->kobj, &dev_attr_enable.attr);
	if (res) {
		printk(KERN_ERR
		       "DPTC: Unable to register sysdev entry for dptc");
		return res;
	}

	if (res != 0) {
		printk(KERN_ERR "DPTC: Unable to start");
		return res;
	}

	curr_wp = 0;
	update_dptc_wp(curr_wp);

	cpu_clk = clk_get(NULL, "cpu_clk");

	pmcr0 = __raw_readl(MXC_CCM_PMCR0);

	return 0;
}

/*!
 * This function is called to put DPTC in a low power state.
 *
 * @param   pdev  the device structure
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_dptc_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (dptc_is_active)
		stop_dptc();

	return 0;
}

/*!
 * This function is called to resume the MU from a low power state.
 *
 * @param   dev   the device structure
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_dptc_resume(struct platform_device *pdev)
{
	if (dptc_is_active)
		start_dptc();

	return 0;
}

static struct platform_driver mxc_dptc_v2_driver = {
	.driver = {
		   .name = "mxc_dptc",
		   },
	.probe = mxc_dptc_probe,
	.suspend = mxc_dptc_suspend,
	.resume = mxc_dptc_resume,
};

/*!
 * This function is called to resume the MU from a low power state.
 *
 * @param   dev   the device structure used to give information on which MU
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */

static int __init dptc_init(void)
{
	if (cpu_is_mx31()) {
		if (platform_driver_register(&mxc_dptc_v2_driver) != 0) {
			printk(KERN_ERR "mxc_dptc_v2_driver register failed\n");
			return -ENODEV;
		}

		printk(KERN_INFO "DPTC driver module loaded\n");
	}
	return 0;
}

static void __exit dptc_cleanup(void)
{
	stop_dptc();

	/* release the DPTC interrupt */
	free_irq(MXC_INT_CCM, NULL);

	sysfs_remove_file(&dptc_dev->kobj, &dev_attr_enable.attr);

	/* Unregister the device structure */
	platform_driver_unregister(&mxc_dptc_v2_driver);

	printk("DPTC driver module unloaded\n");
}

module_init(dptc_init);
module_exit(dptc_cleanup);

EXPORT_SYMBOL(dptc_disable);
EXPORT_SYMBOL(dptc_enable);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("DPTC driver");
MODULE_LICENSE("GPL");
