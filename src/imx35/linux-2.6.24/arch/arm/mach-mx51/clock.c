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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/spba.h>

#include "crm_regs.h"

static unsigned long pll_base[] = {
	(unsigned long)MXC_DPLL1_BASE,
	(unsigned long)MXC_DPLL2_BASE,
	(unsigned long)MXC_DPLL3_BASE,
};

static struct clk pll1_main_clk;
static struct clk pll1_sw_clk;
static struct clk pll2_sw_clk;
static struct clk pll3_sw_clk;
static struct clk lp_apm_clk;

extern void propagate_rate(struct clk *tclk);
extern void board_ref_clk_rate(unsigned long *ckil, unsigned long *osc,
			       unsigned long *ckih, unsigned long *ckih2);

static int _clk_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(clk->enable_reg);
	reg |= MXC_CCM_CCGR_CG_MASK << clk->enable_shift;
	__raw_writel(reg, clk->enable_reg);

	return 0;
}

static void _clk_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(clk->enable_reg);
	reg &= ~(MXC_CCM_CCGR_CG_MASK << clk->enable_shift);
	__raw_writel(reg, clk->enable_reg);
}

/*
 * For the 4-to-1 muxed input clock
 */
static inline u32 _get_mux(struct clk *parent, struct clk *m0,
			   struct clk *m1, struct clk *m2, struct clk *m3)
{
	if (parent == m0)
		return 0;
	else if (parent == m1)
		return 1;
	else if (parent == m2)
		return 2;
	else if (parent == m3)
		return 3;
	else
		BUG();

	return 0;
}

static inline unsigned long _get_pll_base(struct clk *pll)
{
	if (pll == &pll1_main_clk)
		return pll_base[0];
	else if (pll == &pll2_sw_clk)
		return pll_base[1];
	else if (pll == &pll3_sw_clk)
		return pll_base[2];
	else
		BUG();

	return 0;
}

static struct clk ckih_clk = {
	.name = "ckih",
	.flags = RATE_PROPAGATES,
};

static struct clk ckih2_clk = {
	.name = "ckih2",
	.flags = RATE_PROPAGATES,
};

static struct clk osc_clk = {
	.name = "osc",
	.flags = RATE_PROPAGATES,
};

static struct clk ckil_clk = {
	.name = "ckil",
	.flags = RATE_PROPAGATES,
};

static void _fpm_recalc(struct clk *clk)
{
	clk->rate = ckil_clk.rate * 512;
	if ((__raw_readl(MXC_CCM_CCR) & MXC_CCM_CCR_FPM_MULT_MASK) != 0)
		clk->rate *= 2;

}

static int _fpm_enable(struct clk *clk)
{
	u32 reg = __raw_readl(MXC_CCM_CCR);
	reg |= MXC_CCM_CCR_FPM_EN;
	__raw_writel(reg, MXC_CCM_CCR);
	return 0;
}

static void _fpm_disable(struct clk *clk)
{
	u32 reg = __raw_readl(MXC_CCM_CCR);
	reg &= ~MXC_CCM_CCR_FPM_EN;
	__raw_writel(reg, MXC_CCM_CCR);
}

static struct clk fpm_clk = {
	.name = "fpm_clk",
	.parent = &ckil_clk,
	.recalc = _fpm_recalc,
	.enable = _fpm_enable,
	.disable = _fpm_disable,
	.flags = RATE_PROPAGATES,
};

static void _fpm_div2_recalc(struct clk *clk)
{
	clk->rate = clk->parent->rate / 2;
}

static struct clk fpm_div2_clk = {
	.name = "fpm_div2_clk",
	.parent = &fpm_clk,
	.recalc = _fpm_div2_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_pll_recalc(struct clk *clk)
{
	long mfi, mfn, mfd, pdf, ref_clk, mfn_abs;
	unsigned long dp_op, dp_mfd, dp_mfn, dp_ctl, pll_hfsm, dbl;
	unsigned long pllbase;
	s64 temp;

	pllbase = _get_pll_base(clk);

	dp_ctl = __raw_readl(pllbase + MXC_PLL_DP_CTL);
	pll_hfsm = dp_ctl & MXC_PLL_DP_CTL_HFSM;
	dbl = dp_ctl & MXC_PLL_DP_CTL_DPDCK0_2_EN;

	if (pll_hfsm == 0) {
		dp_op = __raw_readl(pllbase + MXC_PLL_DP_OP);
		dp_mfd = __raw_readl(pllbase + MXC_PLL_DP_MFD);
		dp_mfn = __raw_readl(pllbase + MXC_PLL_DP_MFN);
	} else {
		dp_op = __raw_readl(pllbase + MXC_PLL_DP_HFS_OP);
		dp_mfd = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFD);
		dp_mfn = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFN);
	}
	pdf = dp_op & MXC_PLL_DP_OP_PDF_MASK;
	mfi = (dp_op & MXC_PLL_DP_OP_MFI_MASK) >> MXC_PLL_DP_OP_MFI_OFFSET;
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = dp_mfd & MXC_PLL_DP_MFD_MASK;
	mfn = mfn_abs = dp_mfn & MXC_PLL_DP_MFN_MASK;
	/* Sign extend to 32-bits */
	if (mfn >= 0x04000000) {
		mfn |= 0xFC000000;
		mfn_abs = -mfn;
	}

	ref_clk = 2 * clk->parent->rate;
	if (dbl != 0)
		ref_clk *= 2;

	ref_clk /= (pdf + 1);
	temp = (u64) ref_clk * mfn_abs;
	do_div(temp, mfd + 1);
	if (mfn < 0)
		temp = -temp;
	temp = (ref_clk * mfi) + temp;

	clk->rate = temp;
}

static int _clk_pll_enable(struct clk *clk)
{
	u32 reg;
	u32 pllbase;

	pllbase = _get_pll_base(clk);
	reg = __raw_readl(pllbase + MXC_PLL_DP_CTL) | MXC_PLL_DP_CTL_UPEN;
	__raw_writel(reg, pllbase + MXC_PLL_DP_CTL);

	/* Wait for lock */
	while (!(__raw_readl(pllbase + MXC_PLL_DP_CTL) & MXC_PLL_DP_CTL_LRF)) ;

	return 0;
}

static void _clk_pll_disable(struct clk *clk)
{
	u32 reg;
	u32 pllbase;

	pllbase = _get_pll_base(clk);
	reg = __raw_readl(pllbase + MXC_PLL_DP_CTL) & ~MXC_PLL_DP_CTL_UPEN;
	__raw_writel(reg, pllbase + MXC_PLL_DP_CTL);
}

static struct clk pll1_main_clk = {
	.name = "pll1_main_clk",
	.parent = &osc_clk,
	.recalc = _clk_pll_recalc,
	.enable = _clk_pll_enable,
	.disable = _clk_pll_disable,
	.flags = RATE_PROPAGATES,
};

static int _clk_pll1_sw_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CCSR);

	if (parent == &pll1_main_clk) {
		reg &= ~MXC_CCM_CCSR_PLL1_SW_CLK_SEL;
	} else {
		reg |= MXC_CCM_CCSR_PLL1_SW_CLK_SEL;
		mux = _get_mux(parent, &lp_apm_clk, NULL, &pll2_sw_clk,
			       &pll3_sw_clk);
		reg = (reg & ~MXC_CCM_CCSR_STEP_SEL_MASK) |
		    (mux << MXC_CCM_CCSR_STEP_SEL_OFFSET);
	}
	__raw_writel(reg, MXC_CCM_CCSR);
	return 0;
}

static void _clk_pll1_sw_recalc(struct clk *clk)
{
	u32 reg, div;
	div = 1;
	reg = __raw_readl(MXC_CCM_CCSR);

	if (clk->parent == &pll2_sw_clk) {
		div = ((reg & MXC_CCM_CCSR_PLL2_PODF_MASK) >>
		       MXC_CCM_CCSR_PLL2_PODF_OFFSET) + 1;
	} else if (clk->parent == &pll3_sw_clk) {
		div = ((reg & MXC_CCM_CCSR_PLL3_PODF_MASK) >>
		       MXC_CCM_CCSR_PLL3_PODF_OFFSET) + 1;
	}
	clk->rate = clk->parent->rate / div;
}

/* pll1 switch clock */
static struct clk pll1_sw_clk = {
	.name = "pll1_sw_clk",
	.parent = &pll1_main_clk,
	.set_parent = _clk_pll1_sw_set_parent,
	.recalc = _clk_pll1_sw_recalc,
	.flags = RATE_PROPAGATES,
};

/* same as pll2_main_clk. These two clocks should always be the same */
static struct clk pll2_sw_clk = {
	.name = "pll2",
	.parent = &osc_clk,
	.recalc = _clk_pll_recalc,
	.enable = _clk_pll_enable,
	.disable = _clk_pll_disable,
	.flags = RATE_PROPAGATES,
};

/* same as pll3_main_clk. These two clocks should always be the same */
static struct clk pll3_sw_clk = {
	.name = "pll3",
	.parent = &osc_clk,
	.recalc = _clk_pll_recalc,
	.enable = _clk_pll_enable,
	.disable = _clk_pll_disable,
	.flags = RATE_PROPAGATES,
};

static struct clk gpc_dvfs_clk = {
	.name = "gpc_dvfs_clk",
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG12_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static int _clk_lp_apm_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	if (parent == &osc_clk)
		reg = __raw_readl(MXC_CCM_CCSR) & ~MXC_CCM_CCSR_LP_APM_SEL;
	else if (parent == &fpm_clk)
		reg = __raw_readl(MXC_CCM_CCSR) | MXC_CCM_CCSR_LP_APM_SEL;
	else
		return -EINVAL;

	__raw_writel(reg, MXC_CCM_CCSR);

	return 0;
}

static struct clk lp_apm_clk = {
	.name = "lp_apm",
	.parent = &osc_clk,
	.set_parent = _clk_lp_apm_set_parent,
	.flags = RATE_PROPAGATES,
};

static void _clk_arm_recalc(struct clk *clk)
{
	u32 cacrr, div;

	cacrr = __raw_readl(MXC_CCM_CACRR);
	div = (cacrr & MXC_CCM_CACRR_ARM_PODF_MASK) + 1;
	clk->rate = clk->parent->rate / div;
}

static int _clk_cpu_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

static struct clk ap_clk = {
	.name = "cpu_clk",
	.parent = &pll1_sw_clk,
	.recalc = _clk_arm_recalc,
	.set_rate = _clk_cpu_set_rate,
};

static int _clk_periph_apm_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll3_sw_clk, &lp_apm_clk, NULL);

	reg = __raw_readl(MXC_CCM_CBCMR) & ~MXC_CCM_CBCMR_PERIPH_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CBCMR_PERIPH_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CBCMR);

	return 0;
}

static struct clk periph_apm_clk = {
	.name = "periph_apm_clk",
	.parent = &pll1_sw_clk,
	.set_parent = _clk_periph_apm_set_parent,
	.flags = RATE_PROPAGATES,
};

/* TODO: Need to sync with GPC to determine if DVFS is in place so that
 * the DVFS_PODF divider can be applied in CDCR register.
 */
static void _clk_main_bus_recalc(struct clk *clk)
{
	clk->rate = clk->parent->rate;
}

static int _clk_main_bus_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	if (parent == &pll2_sw_clk) {
		reg = __raw_readl(MXC_CCM_CBCDR) &
		    ~MXC_CCM_CBCDR_PERIPH_CLK_SEL;
	} else if (parent == &periph_apm_clk) {
		reg = __raw_readl(MXC_CCM_CBCDR) | MXC_CCM_CBCDR_PERIPH_CLK_SEL;
	} else {
		return -EINVAL;
	}
	__raw_writel(reg, MXC_CCM_CBCDR);

	return 0;
}

static struct clk main_bus_clk = {
	.name = "main_bus_clk",
	.parent = &pll2_sw_clk,
	.set_parent = _clk_main_bus_set_parent,
	.recalc = _clk_main_bus_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_axi_a_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_AXI_A_PODF_MASK) >>
	       MXC_CCM_CBCDR_AXI_A_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static struct clk axi_a_clk = {
	.name = "axi_a_clk",
	.parent = &main_bus_clk,
	.recalc = _clk_axi_a_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_axi_b_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_AXI_B_PODF_MASK) >>
	       MXC_CCM_CBCDR_AXI_B_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static struct clk axi_b_clk = {
	.name = "axi_b_clk",
	.parent = &main_bus_clk,
	.recalc = _clk_axi_b_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_ahb_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_AHB_PODF_MASK) >>
	       MXC_CCM_CBCDR_AHB_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static struct clk ahb_clk = {
	.name = "ahb_clk",
	.parent = &main_bus_clk,
	.recalc = _clk_ahb_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_ipg_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_IPG_PODF_MASK) >>
	       MXC_CCM_CBCDR_IPG_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static struct clk ipg_clk = {
	.name = "ipg_clk",
	.parent = &ahb_clk,
	.recalc = _clk_ipg_recalc,
	.flags = RATE_PROPAGATES,
};

static void _clk_ipg_per_recalc(struct clk *clk)
{
	u32 reg, prediv1, prediv2, podf;

	if (clk->parent == &main_bus_clk || clk->parent == &lp_apm_clk) {
		/* the main_bus_clk is the one before the DVFS engine */
		reg = __raw_readl(MXC_CCM_CBCDR);
		prediv1 = ((reg & MXC_CCM_CBCDR_PERCLK_PRED1_MASK) >>
			   MXC_CCM_CBCDR_PERCLK_PRED1_OFFSET) + 1;
		prediv2 = ((reg & MXC_CCM_CBCDR_PERCLK_PRED2_MASK) >>
			   MXC_CCM_CBCDR_PERCLK_PRED2_OFFSET) + 1;
		podf = ((reg & MXC_CCM_CBCDR_PERCLK_PODF_MASK) >>
			MXC_CCM_CBCDR_PERCLK_PODF_OFFSET) + 1;
		clk->rate = clk->parent->rate / (prediv1 * prediv2 * podf);
	} else if (clk->parent == &ipg_clk) {
		clk->rate = ipg_clk.rate;
	} else {
		BUG();
	}
}

static int _clk_ipg_per_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CBCMR);
	mux = _get_mux(parent, &main_bus_clk, &lp_apm_clk, &ipg_clk, NULL);
	if (mux == 2) {
		reg |= MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL;
	} else {
		reg &= ~MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL;
		if (mux == 0)
			reg &= ~MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL;
		else
			reg |= MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL;
	}
	__raw_writel(reg, MXC_CCM_CBCMR);

	return 0;
}

static struct clk ipg_perclk = {
	.name = "ipg_perclk",
	.parent = &lp_apm_clk,
	.recalc = _clk_ipg_per_recalc,
	.set_parent = _clk_ipg_per_set_parent,
	.flags = RATE_PROPAGATES,
};

static struct clk sdma_clk[] = {
	{
	 .name = "sdma_ahb_clk",
	 .parent = &ahb_clk,
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG15_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "sdma_ipg_clk",
	 .parent = &ipg_clk,
	 },
};

static struct clk ipu_clk = {
	.name = "ipu_clk",
	.parent = &axi_a_clk,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG5_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk ipu_di_clk = {
	.name = "ipu_di_clk",
	.parent = &osc_clk,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG6_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static int _clk_tve_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_CSCMR1);

	if (parent == &pll3_sw_clk) {
		reg &= ~(MXC_CCM_CSCMR1_TVE_CLK_SEL);
	} else if (parent == &osc_clk) {
		reg |= MXC_CCM_CSCMR1_TVE_CLK_SEL;
		reg &= MXC_CCM_CSCMR1_TVE_EXT_CLK_SEL;
	} else if (parent == &ckih_clk) {
		reg |= MXC_CCM_CSCMR1_TVE_CLK_SEL;
		reg |= MXC_CCM_CSCMR1_TVE_EXT_CLK_SEL;
	} else {
		BUG();
	}

	__raw_writel(reg, MXC_CCM_CSCMR1);
	return 0;
}

static void _clk_tve_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if ((reg & MXC_CCM_CSCMR1_TVE_CLK_SEL) == 0) {
		reg = __raw_readl(MXC_CCM_CDCDR) &
		    MXC_CCM_CDCDR_TVE_CLK_PRED_MASK;
		div = (reg >> MXC_CCM_CDCDR_TVE_CLK_PRED_OFFSET) + 1;
		clk->rate = clk->parent->rate / div;
	} else {
		clk->rate = clk->parent->rate;
	}
}

static unsigned long _clk_tve_round_rate(struct clk *clk, unsigned long rate)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (reg & MXC_CCM_CSCMR1_TVE_CLK_SEL)
		return -EINVAL;

	div = clk->parent->rate / rate;
	if (div > 8)
		div = 8;
	else if (div == 0)
		div++;
	return clk->parent->rate / div;
}

static int _clk_tve_set_rate(struct clk *clk, unsigned long rate)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (reg & MXC_CCM_CSCMR1_TVE_CLK_SEL)
		return -EINVAL;

	div = clk->parent->rate / rate;
	if (div == 0)
		div++;
	if (((clk->parent->rate / div) != rate) || (div > 8))
		return -EINVAL;

	div--;
	reg = __raw_readl(MXC_CCM_CDCDR) & ~MXC_CCM_CDCDR_TVE_CLK_PRED_MASK;
	reg |= div << MXC_CCM_CDCDR_TVE_CLK_PRED_OFFSET;
	__raw_writel(reg, MXC_CCM_CDCDR);
	clk->rate = rate;
	return 0;
}

static struct clk tve_clk = {
	.name = "tve_clk",
	.parent = &pll3_sw_clk,
	.set_parent = _clk_tve_set_parent,
	.enable_reg = MXC_CCM_CCGR2,
	.enable_shift = MXC_CCM_CCGR2_CG15_OFFSET,
	.recalc = _clk_tve_recalc,
	.round_rate = _clk_tve_round_rate,
	.set_rate = _clk_tve_set_rate,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static void _clk_uart_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CSCDR1);
	prediv = ((reg & MXC_CCM_CSCDR1_UART_CLK_PRED_MASK) >>
		  MXC_CCM_CSCDR1_UART_CLK_PRED_OFFSET) + 1;
	podf = ((reg & MXC_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_uart_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
		       &lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_UART_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_UART_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk uart_main_clk = {
	.name = "uart_main_clk",
	.parent = &pll2_sw_clk,
	.recalc = _clk_uart_recalc,
	.set_parent = _clk_uart_set_parent,
	.flags = RATE_PROPAGATES,
};

static struct clk uart1_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 0,
	 .parent = &uart_main_clk,
	 .secondary = &uart1_clk[1],
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG4_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "uart_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG3_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static struct clk uart2_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 1,
	 .parent = &uart_main_clk,
	 .secondary = &uart2_clk[1],
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG6_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "uart_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG5_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static struct clk uart3_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 2,
	 .parent = &uart_main_clk,
	 .secondary = &uart3_clk[1],
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG8_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "uart_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG7_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static struct clk spba_clk = {
	.name = "spba_clk",
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG0_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk gpt_clk[] = {
	{
	 .name = "gpt_clk",
	 .parent = &ipg_clk,
	 .id = 0,
	 .secondary = &gpt_clk[1],
	 .enable_reg = MXC_CCM_CCGR2,
	 .enable_shift = MXC_CCM_CCGR2_CG9_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "gpt_ipg_clk",
	 .id = 0,
	 .parent = &ipg_perclk,
	 .enable_reg = MXC_CCM_CCGR2,
	 .enable_shift = MXC_CCM_CCGR2_CG10_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "gpt_32k_clk",
	 .id = 0,
	 .parent = &ckil_clk,
	 },
};

static struct clk i2c_clk[] = {
	{
	 .name = "i2c_clk",
	 .id = 0,
	 .parent = &ipg_perclk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG9_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "i2c_clk",
	 .id = 1,
	 .parent = &ipg_perclk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG10_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "i2c_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG12_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static void _clk_cspi_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CSCDR2);
	prediv = ((reg & MXC_CCM_CSCDR2_CSPI_CLK_PRED_MASK) >>
		  MXC_CCM_CSCDR2_CSPI_CLK_PRED_OFFSET) + 1;
	if (prediv == 1)
		BUG();
	podf = ((reg & MXC_CCM_CSCDR2_CSPI_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR2_CSPI_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_cspi_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
		       &lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_CSPI_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_CSPI_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk cspi_main_clk = {
	.name = "cspi_main_clk",
	.parent = &pll3_sw_clk,
	.recalc = _clk_cspi_recalc,
	.set_parent = _clk_cspi_set_parent,
	.flags = RATE_PROPAGATES,
};

static struct clk cspi1_clk[] = {
	{
	 .name = "cspi_clk",
	 .id = 0,
	 .parent = &cspi_main_clk,
	 .secondary = &cspi1_clk[1],
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG10_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "cspi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG9_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static struct clk cspi2_clk[] = {
	{
	 .name = "cspi_clk",
	 .id = 1,
	 .parent = &cspi_main_clk,
	 .secondary = &cspi2_clk[1],
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG12_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "cspi_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG11_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static struct clk cspi3_clk[] = {
	{
	 .name = "cspi_clk",
	 .id = 2,
	 .parent = &cspi_main_clk,
	 .secondary = &cspi3_clk[1],
	 },
	{
	 .name = "cspi_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR4,
	 .enable_shift = MXC_CCM_CCGR4_CG13_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static int _clk_ssi_lp_apm_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &ckih_clk, &lp_apm_clk, &ckih2_clk, NULL);
	reg = __raw_readl(MXC_CCM_CSCMR1) &
	    ~MXC_CCM_CSCMR1_SSI_APM_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_SSI_APM_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk ssi_lp_apm_clk = {
	.name = "ssi_lp_apm_clk",
	.parent = &ckih_clk,
	.set_parent = _clk_ssi_lp_apm_set_parent,
};

static void _clk_ssi1_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CS1CDR);
	prediv = ((reg & MXC_CCM_CS1CDR_SSI1_CLK_PRED_MASK) >>
		  MXC_CCM_CS1CDR_SSI1_CLK_PRED_OFFSET) + 1;
	if (prediv == 1)
		BUG();
	podf = ((reg & MXC_CCM_CS1CDR_SSI1_CLK_PODF_MASK) >>
		MXC_CCM_CS1CDR_SSI1_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}
static int _clk_ssi1_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk,
		       &pll3_sw_clk, &ssi_lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_SSI1_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_SSI1_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk ssi1_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 0,
	 .parent = &pll3_sw_clk,
	 .set_parent = _clk_ssi1_set_parent,
	 .secondary = &ssi1_clk[1],
	 .recalc = _clk_ssi1_recalc,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG9_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "ssi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG8_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static void _clk_ssi2_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CS2CDR);
	prediv = ((reg & MXC_CCM_CS2CDR_SSI2_CLK_PRED_MASK) >>
		  MXC_CCM_CS2CDR_SSI2_CLK_PRED_OFFSET) + 1;
	if (prediv == 1)
		BUG();
	podf = ((reg & MXC_CCM_CS2CDR_SSI2_CLK_PODF_MASK) >>
		MXC_CCM_CS2CDR_SSI2_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_ssi2_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk,
		       &pll3_sw_clk, &ssi_lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_SSI2_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_SSI2_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk ssi2_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 1,
	 .parent = &pll3_sw_clk,
	 .set_parent = _clk_ssi2_set_parent,
	 .secondary = &ssi2_clk[1],
	 .recalc = _clk_ssi2_recalc,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG11_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
	{
	 .name = "ssi_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG10_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,
	 },
};

static void _clk_ssi_ext1_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	clk->rate = clk->parent->rate;
	reg = __raw_readl(MXC_CCM_CSCMR1);
	if ((reg & MXC_CCM_CSCMR1_SSI_EXT1_COM_CLK_SEL) == 0) {
		reg = __raw_readl(MXC_CCM_CS1CDR);
		prediv = ((reg & MXC_CCM_CS1CDR_SSI_EXT1_CLK_PRED_MASK) >>
			  MXC_CCM_CS1CDR_SSI_EXT1_CLK_PRED_OFFSET) + 1;
		if (prediv == 1)
			BUG();
		podf = ((reg & MXC_CCM_CS1CDR_SSI_EXT1_CLK_PODF_MASK) >>
			MXC_CCM_CS1CDR_SSI_EXT1_CLK_PODF_OFFSET) + 1;
		clk->rate = clk->parent->rate / (prediv * podf);
	}
}

static int _clk_ssi_ext1_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (parent == &ssi1_clk[0]) {
		reg |= MXC_CCM_CSCMR1_SSI_EXT1_COM_CLK_SEL;
	} else {
		reg &= ~MXC_CCM_CSCMR1_SSI_EXT1_COM_CLK_SEL;
		mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
			       &ssi_lp_apm_clk);
		reg = (reg & ~MXC_CCM_CSCMR1_SSI_EXT1_CLK_SEL_MASK) |
		    (mux << MXC_CCM_CSCMR1_SSI_EXT1_CLK_SEL_OFFSET);
	}

	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk ssi_ext1_clk = {
	.name = "ssi_ext1_clk",
	.parent = &pll3_sw_clk,
	.set_parent = _clk_ssi_ext1_set_parent,
	.recalc = _clk_ssi_ext1_recalc,
	.enable_reg = MXC_CCM_CCGR3,
	.enable_shift = MXC_CCM_CCGR3_CG14_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static void _clk_ssi_ext2_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	clk->rate = clk->parent->rate;
	reg = __raw_readl(MXC_CCM_CSCMR1);
	if ((reg & MXC_CCM_CSCMR1_SSI_EXT2_COM_CLK_SEL) == 0) {
		reg = __raw_readl(MXC_CCM_CS2CDR);
		prediv = ((reg & MXC_CCM_CS2CDR_SSI_EXT2_CLK_PRED_MASK) >>
			  MXC_CCM_CS2CDR_SSI_EXT2_CLK_PRED_OFFSET) + 1;
		if (prediv == 1)
			BUG();
		podf = ((reg & MXC_CCM_CS2CDR_SSI_EXT2_CLK_PODF_MASK) >>
			MXC_CCM_CS2CDR_SSI_EXT2_CLK_PODF_OFFSET) + 1;
		clk->rate = clk->parent->rate / (prediv * podf);
	}
}

static int _clk_ssi_ext2_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (parent == &ssi2_clk[0]) {
		reg |= MXC_CCM_CSCMR1_SSI_EXT2_COM_CLK_SEL;
	} else {
		reg &= ~MXC_CCM_CSCMR1_SSI_EXT2_COM_CLK_SEL;
		mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
			       &ssi_lp_apm_clk);
		reg = (reg & ~MXC_CCM_CSCMR1_SSI_EXT2_CLK_SEL_MASK) |
		    (mux << MXC_CCM_CSCMR1_SSI_EXT2_CLK_SEL_OFFSET);
	}

	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk ssi_ext2_clk = {
	.name = "ssi_ext2_clk",
	.parent = &pll3_sw_clk,
	.set_parent = _clk_ssi_ext2_set_parent,
	.recalc = _clk_ssi_ext2_recalc,
	.enable_reg = MXC_CCM_CCGR3,
	.enable_shift = MXC_CCM_CCGR3_CG15_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk iim_clk = {
	.name = "iim_clk",
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR0,
	.enable_shift = MXC_CCM_CCGR0_CG15_OFFSET,
	.disable = _clk_disable,
};

static struct clk tmax_clk[] = {
	{
	 .name = "tmax1_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG0_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &tmax_clk[1],
	 },
	{
	 .name = "tmax2_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG1_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &tmax_clk[2],
	 },
	{
	 .name = "tmax3_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR1,
	 .enable_shift = MXC_CCM_CCGR1_CG2_OFFSET,
	 .disable = _clk_disable,
	 },

};

static void _clk_usboh2_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CSCDR1);
	prediv = ((reg & MXC_CCM_CSCDR1_USBOH2_CLK_PRED_MASK) >>
		  MXC_CCM_CSCDR1_USBOH2_CLK_PRED_OFFSET) + 1;
	if (prediv == 1)
		BUG();
	podf = ((reg & MXC_CCM_CSCDR1_USBOH2_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_USBOH2_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_usboh2_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
		       &lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_USBOH2_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_USBOH2_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk usboh2_clk[] = {
	{
	 .name = "usboh2_clk",
	 .parent = &pll3_sw_clk,
	 .set_parent = _clk_usboh2_set_parent,
	 .recalc = _clk_usboh2_recalc,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR2,
	 .enable_shift = MXC_CCM_CCGR2_CG14_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &usboh2_clk[1],
	 },
	{
	 .name = "usb_ahb_clk",
	 .parent = &ahb_clk,
	 .secondary = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR2,
	 .enable_shift = MXC_CCM_CCGR2_CG13_OFFSET,
	 .disable = _clk_disable,
	 },
};

static void _clk_usb_phy_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	if (clk->parent == &pll3_sw_clk) {
		reg = __raw_readl(MXC_CCM_CDCDR);
		prediv = ((reg & MXC_CCM_CDCDR_USB_PHY_PRED_MASK) >>
			  MXC_CCM_CDCDR_USB_PHY_PRED_OFFSET) + 1;
		podf = ((reg & MXC_CCM_CDCDR_USB_PHY_PODF_MASK) >>
			MXC_CCM_CDCDR_USB_PHY_PODF_OFFSET) + 1;

		clk->rate = clk->parent->rate / (prediv * podf);
	} else
		clk->rate = clk->parent->rate;
}

static int _clk_usb_phy_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (parent == &osc_clk)
		reg &= ~MXC_CCM_CSCMR1_USB_PHY_CLK_SEL;
	else if (parent == &pll3_sw_clk)
		reg |= MXC_CCM_CSCMR1_USB_PHY_CLK_SEL;
	else
		BUG();

	__raw_writel(reg, MXC_CCM_CSCMR1);
	return 0;
}

static struct clk usb_phy_clk = {
	.name = "usb_phy_clk",
	.parent = &pll3_sw_clk,
	.set_parent = _clk_usb_phy_set_parent,
	.recalc = _clk_usb_phy_recalc,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR2,
	.enable_shift = MXC_CCM_CCGR2_CG0_OFFSET,
	.disable = _clk_disable,
};

static void _clk_esdhc1_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CSCDR1);
	prediv = ((reg & MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_MASK) >>
		  MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_OFFSET) + 1;
	podf = ((reg & MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_esdhc1_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
		       &lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) &
	    ~MXC_CCM_CSCMR1_ESDHC1_MSHC1_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_ESDHC1_MSHC1_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk esdhc1_clk[] = {
	{
	 .name = "esdhc_clk",
	 .id = 0,
	 .parent = &pll3_sw_clk,
	 .set_parent = _clk_esdhc1_set_parent,
	 .recalc = _clk_esdhc1_recalc,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG1_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esdhc1_clk[1],
	 },
	{
	 .name = "esdhc_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG0_OFFSET,
	 .disable = _clk_disable,
	 },
};

static void _clk_esdhc2_recalc(struct clk *clk)
{
	u32 reg, prediv, podf;

	reg = __raw_readl(MXC_CCM_CSCDR1);
	prediv = ((reg & MXC_CCM_CSCDR1_ESDHC2_MSHC2_CLK_PRED_MASK) >>
		  MXC_CCM_CSCDR1_ESDHC2_MSHC2_CLK_PRED_OFFSET) + 1;
	podf = ((reg & MXC_CCM_CSCDR1_ESDHC2_MSHC2_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_ESDHC2_MSHC2_CLK_PODF_OFFSET) + 1;

	clk->rate = clk->parent->rate / (prediv * podf);
}

static int _clk_esdhc2_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
		       &lp_apm_clk);
	reg = __raw_readl(MXC_CCM_CSCMR1) &
	    ~MXC_CCM_CSCMR1_ESDHC2_MSHC2_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_ESDHC2_MSHC2_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk esdhc2_clk[] = {
	{
	 .name = "esdhc_clk",
	 .id = 1,
	 .parent = &pll3_sw_clk,
	 .set_parent = _clk_esdhc2_set_parent,
	 .recalc = _clk_esdhc2_recalc,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG3_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esdhc2_clk[1],
	 },
	{
	 .name = "esdhc_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG2_OFFSET,
	 .disable = _clk_disable,
	 },
};

static int _clk_esdhc3_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_CSCMR1);
	if (parent == &esdhc1_clk[0])
		reg &= ~MXC_CCM_CSCMR1_ESDHC3_CLK_SEL;
	else if (parent == &esdhc2_clk[0])
		reg |= MXC_CCM_CSCMR1_ESDHC3_CLK_SEL;
	else
		BUG();

	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk esdhc3_clk[] = {
	{
	 .name = "esdhc_clk",
	 .id = 2,
	 .parent = &esdhc1_clk[0],
	 .set_parent = _clk_esdhc3_set_parent,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG5_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esdhc3_clk[1],
	 },
	{
	 .name = "esdhc_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR3,
	 .enable_shift = MXC_CCM_CCGR3_CG4_OFFSET,
	 .disable = _clk_disable,
	 },
};

static void _clk_emi_core_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_EMI_PODF_MASK) >>
	       MXC_CCM_CBCDR_EMI_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static int _clk_emi_core_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_CBCDR);
	if (parent == &ahb_clk)
		reg |= MXC_CCM_CBCDR_EMI_CLK_SEL;
	else if (parent == &main_bus_clk)
		reg &= ~MXC_CCM_CBCDR_EMI_CLK_SEL;
	else
		BUG();

	__raw_writel(reg, MXC_CCM_CBCDR);

	return 0;
}

static struct clk emi_core_clk = {
	.name = "emi_core_clk",
	.set_parent = _clk_emi_core_set_parent,
	.recalc = _clk_emi_core_recalc,
};

static void _clk_nfc_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CBCDR);
	div = ((reg & MXC_CCM_CBCDR_NFC_PODF_MASK) >>
	       MXC_CCM_CBCDR_NFC_PODF_OFFSET) + 1;
	clk->rate = clk->parent->rate / div;
}

static struct clk nfc_clk = {
	.name = "nfc_clk",
	.parent = &emi_core_clk,
	.recalc = _clk_nfc_recalc,
};

static int _clk_spdif_xtal_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	mux = _get_mux(parent, &osc_clk, &ckih_clk, &ckih2_clk, NULL);
	reg = __raw_readl(MXC_CCM_CSCMR1) & ~MXC_CCM_CSCMR1_SPDIF_CLK_SEL_MASK;
	reg |= mux << MXC_CCM_CSCMR1_SPDIF_CLK_SEL_OFFSET;
	__raw_writel(reg, MXC_CCM_CSCMR1);

	return 0;
}

static struct clk spdif_xtal_clk = {
	.name = "spdif_xtal_clk",
	.parent = &osc_clk,
	.set_parent = _clk_spdif_xtal_set_parent,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG15_OFFSET,
	.disable = _clk_disable,
};

static int _clk_spdif0_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CSCMR2);
	reg |= MXC_CCM_CSCMR2_SPDIF0_COM;
	if (parent != &ssi1_clk[0]) {
		reg &= ~MXC_CCM_CSCMR2_SPDIF0_COM;
		mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
			       &spdif_xtal_clk);
		reg = (reg & ~MXC_CCM_CSCMR2_SPDIF0_CLK_SEL_MASK) |
		    (mux << MXC_CCM_CSCMR2_SPDIF0_CLK_SEL_OFFSET);
	}
	__raw_writel(reg, MXC_CCM_CSCMR2);

	return 0;
}

static void _clk_spdif0_recalc(struct clk *clk)
{
	u32 reg, pred, podf;

	if (clk->parent == &ssi1_clk[0]) {
		clk->rate = clk->parent->rate;
	} else {
		reg = __raw_readl(MXC_CCM_CDCDR);
		pred = ((reg & MXC_CCM_CDCDR_SPDIF0_CLK_PRED_MASK) >>
			MXC_CCM_CDCDR_SPDIF0_CLK_PRED_OFFSET) + 1;
		podf = ((reg & MXC_CCM_CDCDR_SPDIF0_CLK_PODF_MASK) >>
			MXC_CCM_CDCDR_SPDIF0_CLK_PODF_OFFSET) + 1;
		clk->rate = clk->parent->rate / (pred * podf);
	}
}

static struct clk spdif0_clk = {
	.name = "spdif_clk",
	.id = 0,
	.parent = &pll3_sw_clk,
	.set_parent = _clk_spdif0_set_parent,
	.recalc = _clk_spdif0_recalc,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG13_OFFSET,
	.disable = _clk_disable,
};

static int _clk_spdif1_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;

	reg = __raw_readl(MXC_CCM_CSCMR2);
	reg |= MXC_CCM_CSCMR2_SPDIF1_COM;
	if (parent != &ssi2_clk[0]) {
		reg &= ~MXC_CCM_CSCMR2_SPDIF1_COM;
		mux = _get_mux(parent, &pll1_sw_clk, &pll2_sw_clk, &pll3_sw_clk,
			       &spdif_xtal_clk);
		reg = (reg & ~MXC_CCM_CSCMR2_SPDIF1_CLK_SEL_MASK) |
		    (mux << MXC_CCM_CSCMR2_SPDIF1_CLK_SEL_OFFSET);
	}
	__raw_writel(reg, MXC_CCM_CSCMR2);

	return 0;
}

static void _clk_spdif1_recalc(struct clk *clk)
{
	u32 reg, pred, podf;

	if (clk->parent == &ssi2_clk[0]) {
		clk->rate = clk->parent->rate;
	} else {
		reg = __raw_readl(MXC_CCM_CDCDR);
		pred = ((reg & MXC_CCM_CDCDR_SPDIF1_CLK_PRED_MASK) >>
			MXC_CCM_CDCDR_SPDIF1_CLK_PRED_OFFSET) + 1;
		podf = ((reg & MXC_CCM_CDCDR_SPDIF1_CLK_PODF_MASK) >>
			MXC_CCM_CDCDR_SPDIF1_CLK_PODF_OFFSET) + 1;
		clk->rate = clk->parent->rate / (pred * podf);
	}
}

static struct clk spdif1_clk = {
	.name = "spdif_clk",
	.id = 1,
	.parent = &pll3_sw_clk,
	.set_parent = _clk_spdif1_set_parent,
	.recalc = _clk_spdif1_recalc,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR5,
	.enable_shift = MXC_CCM_CCGR5_CG14_OFFSET,
	.disable = _clk_disable,
};

static int _clk_ddr_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;
	reg = __raw_readl(MXC_CCM_CBCMR);
	mux = _get_mux(parent, &axi_a_clk, &axi_b_clk, &emi_core_clk, &ahb_clk);
	reg = (reg & ~MXC_CCM_CBCMR_DDR_CLK_SEL_MASK) |
	    (mux << MXC_CCM_CBCMR_DDR_CLK_SEL_OFFSET);
	__raw_writel(reg, MXC_CCM_CBCMR);

	return 0;
}

static struct clk ddr_clk = {
	.name = "ddr_clk",
	.parent = &emi_core_clk,
	.set_parent = _clk_ddr_set_parent,
};

static int _clk_arm_axi_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;
	reg = __raw_readl(MXC_CCM_CBCMR);
	mux = _get_mux(parent, &axi_a_clk, &axi_b_clk, &emi_core_clk, &ahb_clk);
	reg = (reg & ~MXC_CCM_CBCMR_ARM_AXI_CLK_SEL_MASK) |
	    (mux << MXC_CCM_CBCMR_ARM_AXI_CLK_SEL_OFFSET);
	__raw_writel(reg, MXC_CCM_CBCMR);

	return 0;
}

static struct clk arm_axi_clk = {
	.name = "arm_axi_clk",
	.parent = &axi_a_clk,
	.set_parent = _clk_arm_axi_set_parent,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR0,
	.enable_shift = MXC_CCM_CCGR0_CG1_OFFSET,
	.disable = _clk_disable,
};

static int _clk_vpu_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;
	reg = __raw_readl(MXC_CCM_CBCMR);
	mux = _get_mux(parent, &axi_a_clk, &axi_b_clk, &emi_core_clk, &ahb_clk);
	reg = (reg & ~MXC_CCM_CBCMR_VPU_AXI_CLK_SEL_MASK) |
	    (mux << MXC_CCM_CBCMR_VPU_AXI_CLK_SEL_OFFSET);
	__raw_writel(reg, MXC_CCM_CBCMR);

	return 0;
}

static struct clk vpu_clk[] = {
	{
	 .name = "vpu_clk",
	 .parent = &axi_a_clk,
	 .set_parent = _clk_vpu_set_parent,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR5,
	 .enable_shift = MXC_CCM_CCGR5_CG4_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &vpu_clk[1],
	 },
	{
	 .name = "vpu_core_clk",
	 .parent = &axi_a_clk,
	 .set_parent = _clk_vpu_set_parent,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CCGR5,
	 .enable_shift = MXC_CCM_CCGR5_CG3_OFFSET,
	 .disable = _clk_disable,
	 },
};

static int _clk_lpsr_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg, mux;
	reg = __raw_readl(MXC_CCM_CLPCR);
	mux = _get_mux(parent, &ckil_clk, &fpm_clk, &fpm_div2_clk, NULL);
	reg = (reg & ~MXC_CCM_CLPCR_LPSR_CLK_SEL_MASK) |
	    (mux << MXC_CCM_CLPCR_LPSR_CLK_SEL_OFFSET);
	__raw_writel(reg, MXC_CCM_CLPCR);

	return 0;
}

static struct clk lpsr_clk = {
	.name = "lpsr_clk",
	.parent = &ckil_clk,
	.set_parent = _clk_lpsr_set_parent,
};

static void _clk_pgc_recalc(struct clk *clk)
{
	u32 reg, div;

	reg = __raw_readl(MXC_CCM_CSCDR1);
	div = (reg & MXC_CCM_CSCDR1_PGC_CLK_PODF_MASK) >>
	    MXC_CCM_CSCDR1_PGC_CLK_PODF_OFFSET;
	div = 1 >> div;
	clk->rate = clk->parent->rate / div;
}

static struct clk pgc_clk = {
	.name = "pgc_clk",
	.parent = &ipg_clk,
	.recalc = _clk_pgc_recalc,
};

/*usb OTG clock */

static struct clk usb_clk = {
	.name = "usb_clk",
	.rate = 60000000,
};
static struct clk usb_utmi_clk = {
	.name = "usb_utmi_clk",
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CSCMR1,
	.enable_shift = MXC_CCM_CSCMR1_USB_PHY_CLK_SEL_OFFSET,
	.disable = _clk_disable,
};

static struct clk rtc_clk = {
	.name = "rtc_clk",
	.parent = &ckil_clk,
	.secondary = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR4,
	.enable_shift = MXC_CCM_CCGR4_CG14_OFFSET,
	.disable = _clk_disable,
};

static struct clk ata_clk = {
	.name = "ata_clk",
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CCGR4,
	.enable_shift = MXC_CCM_CCGR4_CG0_OFFSET,
	.disable = _clk_disable,
};

static struct clk *mxc_clks[] = {
	&osc_clk,
	&ckih_clk,
	&ckih2_clk,
	&ckil_clk,
	&fpm_clk,
	&fpm_div2_clk,
	&pll1_main_clk,
	&pll1_sw_clk,
	&pll2_sw_clk,
	&pll3_sw_clk,
	&gpc_dvfs_clk,
	&lp_apm_clk,
	&ap_clk,
	&periph_apm_clk,
	&main_bus_clk,
	&axi_a_clk,
	&axi_b_clk,
	&ahb_clk,
	&ipg_clk,
	&ipg_perclk,
	&sdma_clk[0],
	&sdma_clk[1],
	&ipu_clk,
	&ipu_di_clk,
	&tve_clk,
	&uart_main_clk,
	&uart1_clk[0],
	&uart1_clk[1],
	&uart2_clk[0],
	&uart2_clk[1],
	&uart3_clk[0],
	&uart3_clk[1],
	&spba_clk,
	&i2c_clk[0],
	&i2c_clk[1],
	&i2c_clk[2],
	&gpt_clk[0],
	&gpt_clk[1],
	&gpt_clk[2],
	&cspi_main_clk,
	&cspi1_clk[0],
	&cspi1_clk[1],
	&cspi2_clk[0],
	&cspi2_clk[1],
	&cspi3_clk[0],
	&cspi3_clk[1],
	&ssi_lp_apm_clk,
	&ssi1_clk[0],
	&ssi1_clk[1],
	&ssi2_clk[0],
	&ssi2_clk[1],
	&ssi_ext1_clk,
	&ssi_ext2_clk,
	&iim_clk,
	&tmax_clk[0],
	&tmax_clk[1],
	&tmax_clk[2],
	&usboh2_clk[0],
	&usboh2_clk[1],
	&usb_phy_clk,
	&usb_utmi_clk,
	&usb_clk,
	&esdhc1_clk[0],
	&esdhc1_clk[1],
	&esdhc2_clk[0],
	&esdhc2_clk[1],
	&esdhc3_clk[0],
	&esdhc3_clk[1],
	&emi_core_clk,
	&nfc_clk,
	&spdif_xtal_clk,
	&spdif0_clk,
	&spdif1_clk,
	&ddr_clk,
	&arm_axi_clk,
	&vpu_clk[0],
	&vpu_clk[1],
	&lpsr_clk,
	&pgc_clk,
	&rtc_clk,
	&ata_clk,
};

static void clk_tree_init(void)
{
	u32 reg, dp_ctl;

	/* set pll1_main_clk parent */
	pll1_main_clk.parent = &osc_clk;
	dp_ctl = __raw_readl(pll_base[0] + MXC_PLL_DP_CTL);
	if ((dp_ctl & MXC_PLL_DP_CTL_REF_CLK_SEL_MASK) == 0)
		pll1_main_clk.parent = &fpm_clk;
	/* set pll2_sw_clk parent */
	pll2_sw_clk.parent = &osc_clk;
	dp_ctl = __raw_readl(pll_base[1] + MXC_PLL_DP_CTL);
	if ((dp_ctl & MXC_PLL_DP_CTL_REF_CLK_SEL_MASK) == 0)
		pll2_sw_clk.parent = &fpm_clk;
	/* set pll3_clk parent */
	pll3_sw_clk.parent = &osc_clk;
	dp_ctl = __raw_readl(pll_base[2] + MXC_PLL_DP_CTL);
	if ((dp_ctl & MXC_PLL_DP_CTL_REF_CLK_SEL_MASK) == 0)
		pll3_sw_clk.parent = &fpm_clk;

	/* set emi_core_clk parent */
	emi_core_clk.parent = &main_bus_clk;
	reg = __raw_readl(MXC_CCM_CBCDR);
	if ((reg & MXC_CCM_CBCDR_EMI_CLK_SEL) != 0)
		emi_core_clk.parent = &ahb_clk;

	/* set ipg_perclk parent */
	ipg_perclk.parent = &lp_apm_clk;
	reg = __raw_readl(MXC_CCM_CBCMR);
	if ((reg & MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL) != 0) {
		ipg_perclk.parent = &ipg_clk;
	} else {
		if ((reg & MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL) == 0)
			ipg_perclk.parent = &main_bus_clk;
	}

	/* set DDR clock parent */
	reg = __raw_readl(MXC_CCM_CBCMR) & MXC_CCM_CBCMR_DDR_CLK_SEL_MASK;
	reg >>= MXC_CCM_CBCMR_DDR_CLK_SEL_OFFSET;
	if (reg == 0) {
		ddr_clk.parent = &axi_a_clk;
	} else if (reg == 1) {
		ddr_clk.parent = &axi_b_clk;
	} else if (reg == 2) {
		ddr_clk.parent = &emi_core_clk;
	} else {
		ddr_clk.parent = &ahb_clk;
	}
}

int __init mxc_clocks_init(void)
{
	struct clk **clkp;

	for (clkp = mxc_clks; clkp < mxc_clks + ARRAY_SIZE(mxc_clks); clkp++)
		clk_register(*clkp);

	/* Turn off all possible clocks */
	__raw_writel(MXC_CCM_CCGR0_CG0_MASK | MXC_CCM_CCGR0_CG1_MASK |
		     MXC_CCM_CCGR0_CG2_MASK | MXC_CCM_CCGR0_CG3_MASK |
		     MXC_CCM_CCGR0_CG4_MASK | MXC_CCM_CCGR0_CG8_MASK |
		     MXC_CCM_CCGR0_CG9_MASK | MXC_CCM_CCGR0_CG10_MASK |
		     MXC_CCM_CCGR0_CG11_MASK | MXC_CCM_CCGR0_CG12_MASK |
		     MXC_CCM_CCGR0_CG13_MASK | MXC_CCM_CCGR0_CG14_MASK |
		     MXC_CCM_CCGR0_CG15_MASK, MXC_CCM_CCGR0);

	__raw_writel(0, MXC_CCM_CCGR1);
	__raw_writel(0, MXC_CCM_CCGR2);
	__raw_writel(0, MXC_CCM_CCGR3);
	__raw_writel(0, MXC_CCM_CCGR4);
	__raw_writel(MXC_CCM_CCGR5_CG2_MASK | MXC_CCM_CCGR5_CG7_MASK |
		     MXC_CCM_CCGR5_CG8_MASK | MXC_CCM_CCGR5_CG9_MASK |
		     MXC_CCM_CCGR5_CG10_MASK | MXC_CCM_CCGR5_CG11_MASK,
		     MXC_CCM_CCGR5);
	__raw_writel(MXC_CCM_CCGR6_CG4_MASK, MXC_CCM_CCGR6);

	/* Setup the parent based on the register values */
	clk_tree_init();
	/* This will propagate to all children and init all the clock rates */
	propagate_rate(&osc_clk);
	propagate_rate(&ckih_clk);
	propagate_rate(&ckih2_clk);
	propagate_rate(&ckil_clk);

	clk_enable(&gpt_clk[1]);
	clk_enable(&spba_clk);
	clk_enable(&tmax_clk[0]);
	clk_enable(&gpc_dvfs_clk);

	return 0;
}

/*!
 * Function to get timer clock rate early in boot process before clock tree is
 * initialized.
 *
 * @return	Clock rate for timer
 */
unsigned long __init clk_early_get_timer_rate(void)
{
	board_ref_clk_rate(&ckil_clk.rate, &osc_clk.rate, &ckih_clk.rate,
			   &ckih2_clk.rate);

	lp_apm_clk.rate = lp_apm_clk.parent->rate;
	ipg_perclk.recalc(&ipg_perclk);
	gpt_clk[1].enable(&gpt_clk[1]);

	return ipg_perclk.rate;
}
