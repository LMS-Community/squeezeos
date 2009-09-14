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
#define DEBUG
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/proc-fns.h>
#include <asm/system.h>
#include <asm/cacheflush.h>
#include <asm/arch/clock.h>
#include "crm_regs.h"

/*!
 * @defgroup MSL_MX37 i.MX37 Machine Specific Layer (MSL)
 */

/*!
 * @file mach-mx37/system.c
 * @brief This file contains idle and reset functions.
 *
 * @ingroup MSL_MX37
 */

extern int mxc_jtag_enabled;
extern int lp_video_mode;
extern int lp_audio_mode;
static struct clk *srpg_clk;
static struct clk *cpu_clk;
static struct clk *pll1_main;
static struct clk *pll1_sw_clk;
static struct clk *lp_apm_clk;

/* set cpu low power mode before WFI instruction */
void mxc_cpu_lp_set(enum mxc_cpu_pwr_mode mode)
{
	u32 plat_lpc, gpc_pgr, arm_srpgcr, empgcr0, empgcr1, ccm_clpcr;
    u32 reg;
	/* always allow platform to issue a deep sleep mode request */
	plat_lpc = __raw_readl(MXC_ARM1176_PLAT_LPC) &
	    ~(MXC_ARM1176_PLAT_LPC_DSM);

	ccm_clpcr = __raw_readl(MXC_CCM_CLPCR) & ~(MXC_CCM_CLPCR_LPM_MASK);
	gpc_pgr = __raw_readl(MXC_GPC_PGR) & ~(MXC_GPC_PGR_ARMPG_MASK);
	arm_srpgcr = __raw_readl(MXC_SRPGC_ARM_SRPGCR) & ~(MXC_SRPGCR_PCR);
	empgcr0 = __raw_readl(MXC_EMPGC0_ARM_EMPGCR) & ~(MXC_EMPGCR_PCR);
	empgcr1 = __raw_readl(MXC_EMPGC1_ARM_EMPGCR) & ~(MXC_EMPGCR_PCR);

	switch (mode) {
	case WAIT_CLOCKED:
		break;
	case WAIT_UNCLOCKED:
		ccm_clpcr |= (0x1 << MXC_CCM_CLPCR_LPM_OFFSET);
		break;
	case WAIT_UNCLOCKED_POWER_OFF:
	case STOP_POWER_OFF:
		plat_lpc |= MXC_ARM1176_PLAT_LPC_DSM;
		if (mode == WAIT_UNCLOCKED_POWER_OFF)
			ccm_clpcr |= (0x1 << MXC_CCM_CLPCR_LPM_OFFSET);
		else
			ccm_clpcr |= (0x2 << MXC_CCM_CLPCR_LPM_OFFSET);

		gpc_pgr |= (0x1 << MXC_GPC_PGR_ARMPG_OFFSET);
		arm_srpgcr |= MXC_SRPGCR_PCR;
		empgcr0 |= MXC_EMPGCR_PCR;
		empgcr1 |= MXC_EMPGCR_PCR;

		if (tzic_enable_wake(1) != 0)
			return;
		break;
	case STOP_POWER_ON:
		ccm_clpcr |= (0x2 << MXC_CCM_CLPCR_LPM_OFFSET);
		break;
	default:
		printk(KERN_WARNING "UNKNOWN cpu power mode: %d\n", mode);
		return;
	}

	__raw_writel(plat_lpc, MXC_ARM1176_PLAT_LPC);
	__raw_writel(ccm_clpcr, MXC_CCM_CLPCR);
	__raw_writel(gpc_pgr, MXC_GPC_PGR);
	__raw_writel(arm_srpgcr, MXC_SRPGC_ARM_SRPGCR);
/*	 __raw_writel(empgcr0, MXC_EMPGC0_ARM_EMPGCR); //TODO: system crash */
	__raw_writel(empgcr1, MXC_EMPGC1_ARM_EMPGCR);

	flush_cache_all();

	if (lp_audio_mode || lp_video_mode) {
		if (pll1_sw_clk == NULL)
			pll1_sw_clk = clk_get(NULL, "pll1_sw_clk");
		if (lp_apm_clk == NULL)
			lp_apm_clk = clk_get(NULL, "lp_apm");

		if (cpu_clk == NULL)
			cpu_clk = clk_get(NULL, "cpu_clk");
		if (pll1_main == NULL)
			pll1_main = clk_get(NULL, "pll1_main_clk");

		/* Move the ARM to run off the 24MHz clock. Shutdown the PLL1 */
		/* Change the source of pll1_sw_clk to be the step_clk */
		clk_set_parent(pll1_sw_clk, lp_apm_clk);
		clk_disable(pll1_main);
	}
}

void mxc_pg_enable(struct platform_device *pdev)
{
	if (pdev == NULL)
		return;

	if (strcmp(pdev->name, "mxc_ipu") == 0) {
		__raw_writel(MXC_PGCR_PCR, MXC_PGC_IPU_PGCR);
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_IPU_PGSR);
	} else if (strcmp(pdev->name, "mxc_vpu") == 0) {
		__raw_writel(MXC_PGCR_PCR, MXC_PGC_VPU_PGCR);
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_VPU_PGSR);
	}

}
EXPORT_SYMBOL(mxc_pg_enable);

void mxc_pg_disable(struct platform_device *pdev)
{
	if (pdev == NULL)
		return;

	if (strcmp(pdev->name, "mxc_ipu") == 0) {
		__raw_writel(0x0, MXC_PGC_IPU_PGCR);
		if (__raw_readl(MXC_PGC_IPU_PGSR) & MXC_PGSR_PSR)
			dev_dbg(&pdev->dev, "power gating successful\n");
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_IPU_PGSR);
	} else if (strcmp(pdev->name, "mxc_vpu") == 0) {
		__raw_writel(0x0, MXC_PGC_VPU_PGCR);
		if (__raw_readl(MXC_PGC_VPU_PGSR) & MXC_PGSR_PSR)
			dev_dbg(&pdev->dev, "power gating successful\n");
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_VPU_PGSR);
	}
}
EXPORT_SYMBOL(mxc_pg_disable);

/* To change the idle power mode, need to set arch_idle_mode to a different
 * power mode as in enum mxc_cpu_pwr_mode.
 * May allow dynamically changing the idle mode.
 */
static int arch_idle_mode = WAIT_UNCLOCKED_POWER_OFF;

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	u32 reg;

	if (likely(!mxc_jtag_enabled)) {
		mxc_cpu_lp_set(arch_idle_mode);
		cpu_do_idle();

		if (lp_audio_mode || lp_video_mode) {
			/* Move ARM back to PLL from step clk. */
			clk_enable(pll1_main);
			/* Move the PLL1 back to the pll1_main_clk */
			clk_set_parent(pll1_sw_clk, pll1_main);
		}
	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	/* Assert SRS signal */
	mxc_wd_reset();
}
