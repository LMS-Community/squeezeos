/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>

#include "crm_regs.h"

/*
 * Define reference clock inputs
 */
#define CKIL_CLK_FREQ               32768
#define CKIH_CLK_FREQ               26000000
#define MXC_PLL_REF_CLK             MXC_CKIH_FREQ
#define AHB_FREQ_MAX		    133000000

static struct clk ckih_clk;
static struct clk mcu_pll_clk;
static struct clk usb_pll_clk;
static struct clk turbo_pll_clk;
static struct clk ipg_clk;
static struct clk ahb_clk;

static int cpu_clk_set_wp(int wp);

static int _clk_enable(struct clk *clk)
{
	u32 reg;
	reg = __raw_readl(clk->enable_reg);
	reg |= 3 << clk->enable_shift;
	__raw_writel(reg, clk->enable_reg);

	return 0;
}

static void _clk_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(clk->enable_reg);
	reg &= ~(3 << clk->enable_shift);
	__raw_writel(reg, clk->enable_reg);
}

static void _clk_pll_recalc(struct clk *clk)
{
	u32 mfi = 0, mfn = 0, mfd, pdf, ref_clk, reg = 0;
	u64 temp;

	if (clk == &mcu_pll_clk) {
		reg = __raw_readl(MXC_CCM_MPCTL);
		mfi = (reg & MXC_CCM_MCUPCTL_MFI_MASK) >>
		    MXC_CCM_MCUPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = (reg & MXC_CCM_MCUPCTL_MFN_MASK);
	} else if (clk == &usb_pll_clk) {
		reg = __raw_readl(MXC_CCM_UPCTL);
		mfi = (reg & MXC_CCM_USBPCTL_MFI_MASK) >>
		    MXC_CCM_USBPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = reg & MXC_CCM_USBPCTL_MFN_MASK;
	} else if (clk == &turbo_pll_clk) {
		reg = __raw_readl(MXC_CCM_TPCTL);
		mfi = (reg & MXC_CCM_TPCTL_MFI_MASK) >>
		    MXC_CCM_TPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = reg & MXC_CCM_TPCTL_MFN_MASK;
	} else {
		BUG();
	}

	pdf = (reg & MXC_CCM_PCTL_PDF_MASK) >> MXC_CCM_PCTL_PDF_OFFSET;
	mfd = (reg & MXC_CCM_PCTL_MFD_MASK) >> MXC_CCM_PCTL_MFD_OFFSET;
	ref_clk = clk->parent->rate;

	if (mfn < 1024) {
		temp = (unsigned long long)2 *ref_clk * mfn;
		do_div(temp, mfd + 1);
		temp = (unsigned long long)2 *ref_clk * mfi + temp;
		do_div(temp, pdf + 1);
	} else {
		temp = (unsigned long long)2 *ref_clk * (2048 - mfn);
		do_div(temp, mfd + 1);
		temp = (unsigned long long)2 *ref_clk * mfi - temp;
		do_div(temp, pdf + 1);
	}

	clk->rate = temp;
}

#define PDR0(mask, off) ((__raw_readl(MXC_CCM_MPDR0) & mask) >> off)
#define PDR1(mask, off) ((__raw_readl(MXC_CCM_MPDR1) & mask) >> off)
#define PDR2(mask, off) ((__raw_readl(MXC_CCM_MPDR2) & mask) >> off)

/*
 * The core operating points 212, 177 and 133MHZ are currently not implemented.
 * These operating points are used only when the user really wants to
 * lower frequency fast when working with TPLL otherwise it should not be used
 * since these points operate at 1.6V and hence will not save power
 */
/*
 * The table is populated by: Example sample calculation shown in row 2
 *********************************************************************
 * BRMM	   Active PLL	AHB	     FORMULA			CORE *
 *           (MHz)     (MHz)                                    (MHz)*
 *********************************************************************
 *  0        399        133	  Core clk = PLL      	 	 399 *
 *-------------------------------------------------------------------*
 *  1        399        133    ((2 * t_PLL) + (t_ahb))/3 =           *
 *                             ((2 * 2.5nsec) + (7.5nsec))/3 =       *
 *                                    4.16nsec                       *
 *                                Core clk = 1/4.16nsec =            *
 *                                    ~240Mhz                    240 *
 *-------------------------------------------------------------------*
 *  2        399        133      (t_PLL + t_ahb)/2               200 *
 *-------------------------------------------------------------------*
 *  3        399        133    ((t_PLL) + (2 * t_ahb))/3         171 *
 *-------------------------------------------------------------------*
 *  4        399        133       Core clk = AHB		 133 *
 *-------------------------------------------------------------------*
 *  0        532        133	  Core clk = PLL      	 	 532 *
 *-------------------------------------------------------------------*
 *  1        532        133    ((2 * t_PLL) + (t_ahb))/3         266 *
 *-------------------------------------------------------------------*
 *  2        532        133      (t_PLL + t_ahb)/2               212 *
 *-------------------------------------------------------------------*
 *  3        532        133    ((t_PLL) + (2 * t_ahb))/3         177 *
 *-------------------------------------------------------------------*
 *  4        532        133       Core clk = AHB		 133 *
 *-------------------------------------------------------------------*
 */
static void _clk_mcu_recalc(struct clk *clk)
{
	u32 brmm;
	u32 pdr0 = __raw_readl(MXC_CCM_MPDR0);
	u32 max_clk = ahb_clk.rate;
	u32 div;

	brmm = pdr0 & MXC_CCM_MPDR0_BRMM_MASK;
	div = (pdr0 & MXC_CCM_MPDR0_MAX_PDF_MASK)
	    >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	div++;

	BUG_ON(unlikely(brmm >= 5));

	if (brmm == 0) {
		clk->rate = clk->parent->rate;
	} else if (brmm == 1) {
		/* mcu_clk = (3n/(2+n))*max_clk */
		clk->rate = (3 * div * max_clk) / (div + 2);
	} else if (brmm == 2) {
		/* mcu_clk = (2n/(1+n))*max_clk */
		clk->rate = (2 * div * max_clk) / (div + 1);
	} else if (brmm == 3) {
		/* mcu_clk = (3n/(1+2n))*max_clk */
		clk->rate = (3 * div * max_clk) / (2 * div + 1);
	} else if (brmm == 4) {
		clk->rate = max_clk;
	}
}

static int _clk_cpu_set_rate(struct clk *clk, unsigned long rate)
{
	if (rate != 399000000 && rate != 532000000) {
		printk(KERN_ERR "Wrong rate %lu in _clk_cpu_set_rate\n", rate);
		return -EINVAL;
	}

	cpu_clk_set_wp(rate / ahb_clk.rate - 3);

	return 0;
}

static void _clk_ahb_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR0(MXC_CCM_MPDR0_MAX_PDF_MASK, MXC_CCM_MPDR0_MAX_PDF_OFFSET);
	clk->rate = clk->parent->rate / (pdf + 1);
}

static void _clk_ipg_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR0(MXC_CCM_MPDR0_IPG_PDF_MASK, MXC_CCM_MPDR0_IPG_PDF_OFFSET);
	clk->rate = clk->parent->rate / (pdf + 1);
}

static void _clk_nfc_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR0(MXC_CCM_MPDR0_NFC_PDF_MASK, MXC_CCM_MPDR0_NFC_PDF_OFFSET);
	clk->rate = clk->parent->rate / (pdf + 1);
}

static void _clk_usb_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR1(MXC_CCM_MPDR1_USB_PDF_MASK, MXC_CCM_MPDR1_USB_PDF_OFFSET);
	clk->rate = clk->parent->rate / (pdf + 1);
}

static void _clk_csi_recalc(struct clk *clk)
{
	u32 pdf, pdr0;
	u32 prepdf = 1;

	pdr0 = __raw_readl(MXC_CCM_MPDR0);
	pdf =
	    (pdr0 & MXC_CCM_MPDR0_CSI_PDF_MASK) >> MXC_CCM_MPDR0_CSI_PDF_OFFSET;
	if (pdr0 & MXC_CCM_MPDR0_CSI_PRE) {
		prepdf = 2;
	}

	/* hardware requirement */
	BUG_ON(unlikely(clk->parent->rate > 288000000 && prepdf != 2));

	/* Take care of the fractional part of the post divider */
	if (pdr0 & MXC_CCM_MPDR0_CSI_FPDF) {
		clk->rate = (2 * clk->parent->rate) / (prepdf * (2 * pdf + 3));
	} else {
		clk->rate = clk->parent->rate / (prepdf * (pdf + 1));
	}
}

static unsigned long _clk_csi_round_rate(struct clk *clk, unsigned long rate)
{
	u32 div = 0;

	if (rate == 0 || (div = clk->parent->rate / rate) == 0) {
		return -EINVAL;
	}
	if (clk->parent->rate % rate)
		div++;

	if (div > 512)
		div = 512;

	return clk->parent->rate / div;
}

static int _clk_csi_set_rate(struct clk *clk, unsigned long rate)
{
	u32 reg;
	u32 div = clk->parent->rate / rate;

	if (((clk->parent->rate / div) != rate) || div > 512)
		return -EINVAL;

	/* Set CSI clock divider */
	reg = __raw_readl(MXC_CCM_MPDR0) & ~MXC_CCM_MPDR0_CSI_PDF_MASK;
	reg |= (div - 1) << MXC_CCM_MPDR0_CSI_PDF_OFFSET;
	__raw_writel(reg, MXC_CCM_MPDR0);

	clk->rate = rate;
	return 0;
}

static void _clk_ssi1_recalc(struct clk *clk)
{
	u32 pdf, prepdf;

	pdf = PDR1(MXC_CCM_MPDR1_SSI1_PDF_MASK, MXC_CCM_MPDR1_SSI1_PDF_OFFSET);
	prepdf = PDR1(MXC_CCM_MPDR1_SSI1_PREPDF_MASK,
		      MXC_CCM_MPDR1_SSI1_PREPDF_OFFSET);
	clk->rate = clk->parent->rate / (prepdf + 1) / (pdf + 1);
}

static void _clk_ssi2_recalc(struct clk *clk)
{
	u32 pdf, prepdf;

	pdf = PDR1(MXC_CCM_MPDR1_SSI2_PDF_MASK, MXC_CCM_MPDR1_SSI2_PDF_OFFSET);
	prepdf = PDR1(MXC_CCM_MPDR1_SSI2_PREPDF_MASK,
		      MXC_CCM_MPDR1_SSI2_PREPDF_OFFSET);
	clk->rate = clk->parent->rate / (prepdf + 1) / (pdf + 1);
}

static void _clk_firi_recalc(struct clk *clk)
{
	u32 pdf, prepdf;

	pdf = PDR1(MXC_CCM_MPDR1_FIRI_PDF_MASK, MXC_CCM_MPDR1_FIRI_PDF_OFFSET);
	prepdf = PDR1(MXC_CCM_MPDR1_FIRI_PREPDF_MASK,
		      MXC_CCM_MPDR1_FIRI_PREPDF_OFFSET);
	clk->rate = clk->parent->rate / (prepdf + 1) / (pdf + 1);
}

static void __calc_pre_post_dividers(u32 div, u32 * pre, u32 * post)
{
	u32 min_pre, temp_pre, old_err, err;

	if (div >= 256) {
		*pre = 8;
		*post = 32;
	} else if (div >= 32) {
		min_pre = (div - 1) / 32 + 1;
		old_err = 8;
		for (temp_pre = 8; temp_pre >= min_pre; temp_pre--) {
			err = div % temp_pre;
			if (err == 0) {
				*pre = temp_pre;
				break;
			}
			err = temp_pre - err;
			if (err < old_err) {
				old_err = err;
				*pre = temp_pre;
			}
		}
		*post = (div + *pre - 1) / *pre;
	} else if (div <= 8) {
		*pre = div;
		*post = 1;
	} else {
		*pre = 1;
		*post = div;
	}
}

static unsigned long _clk_firi_round_rate(struct clk *clk, unsigned long rate)
{
	u32 pre, post;
	u32 div = clk->parent->rate / rate;
	if (clk->parent->rate % rate)
		div++;

	__calc_pre_post_dividers(div, &pre, &post);

	return clk->parent->rate / (pre * post);

}

static int _clk_firi_set_rate(struct clk *clk, unsigned long rate)
{
	u32 reg;
	u32 div, pre, post;

	div = clk->parent->rate / rate;

	if ((clk->parent->rate / div) != rate)
		return -EINVAL;

	__calc_pre_post_dividers(div, &pre, &post);

	/* Set FIRI clock divider */
	reg = __raw_readl(MXC_CCM_MPDR1) &
	    ~(MXC_CCM_MPDR1_FIRI_PREPDF_MASK | MXC_CCM_MPDR1_FIRI_PDF_MASK);
	reg |= (pre - 1) << MXC_CCM_MPDR1_FIRI_PREPDF_OFFSET;
	reg |= (post - 1) << MXC_CCM_MPDR1_FIRI_PDF_OFFSET;
	__raw_writel(reg, MXC_CCM_MPDR1);

	clk->rate = rate;
	return 0;
}

static int _clk_usb_pll_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) | MXC_CCM_MCR_UPE;
	__raw_writel(reg, MXC_CCM_MCR);

	while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_UPL) == 0) ;

	return 0;
}

static void _clk_usb_pll_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) & ~MXC_CCM_MCR_UPE;
	__raw_writel(reg, MXC_CCM_MCR);
}

static int _clk_mcu_pll_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) | MXC_CCM_MCR_MPEN;
	__raw_writel(reg, MXC_CCM_MCR);

	while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_MPL) == 0) ;
	return 0;
}

static void _clk_mcu_pll_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) & ~MXC_CCM_MCR_MPEN;
	__raw_writel(reg, MXC_CCM_MCR);
}

static int _clk_turbo_pll_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) | MXC_CCM_MCR_TPE;
	__raw_writel(reg, MXC_CCM_MCR);

	while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_TPL) == 0) ;
	return 0;
}

static void _clk_turbo_pll_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MCR) & ~MXC_CCM_MCR_TPE;
	__raw_writel(reg, MXC_CCM_MCR);
}

static int _clk_sdhc1_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR2) & ~MXC_CCM_MPDR2_SDHC1DIS;
	__raw_writel(reg, MXC_CCM_MPDR2);

	return 0;
}

static void _clk_sdhc1_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR2) | MXC_CCM_MPDR2_SDHC1DIS;
	__raw_writel(reg, MXC_CCM_MPDR2);
}

static void _clk_sdhc1_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR2(MXC_CCM_MPDR2_SDHC1_PDF_MASK,
		   MXC_CCM_MPDR2_SDHC1_PDF_OFFSET);
	/* Not allowed in hardware */
	BUG_ON(unlikely(pdf == 0));

	clk->rate = clk->parent->rate / (pdf + 1);
}

static int _clk_sdhc2_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR2) & ~MXC_CCM_MPDR2_SDHC2DIS;
	__raw_writel(reg, MXC_CCM_MPDR2);

	return 0;
}

static void _clk_sdhc2_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR2) | MXC_CCM_MPDR2_SDHC2DIS;
	__raw_writel(reg, MXC_CCM_MPDR2);
}

static void _clk_sdhc2_recalc(struct clk *clk)
{
	u32 pdf;

	pdf = PDR2(MXC_CCM_MPDR2_SDHC2_PDF_MASK,
		   MXC_CCM_MPDR2_SDHC2_PDF_OFFSET);
	/* Not allowed in hardware */
	BUG_ON(unlikely(pdf == 0));

	clk->rate = clk->parent->rate / (pdf + 1);
}

static int _clk_usb_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1);
	reg &= ~MXC_CCM_MPDR1_USB_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);

	return 0;
}

static void _clk_usb_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1);
	reg |= MXC_CCM_MPDR1_USB_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);
}

static int _clk_csi_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR0);
	reg &= ~MXC_CCM_MPDR0_CSI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR0);

	return 0;
}

static void _clk_csi_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR0);
	reg |= MXC_CCM_MPDR0_CSI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR0);
}

/*
 * Set parent for those modules with 2 bit selection: sdhc1/2, csi
 */
static inline int __clk_set_parent2(struct clk *clk, struct clk *parent,
				    u32 mask, u32 offset)
{
	u32 mux = 0, reg;

	if (unlikely(clk->parent == parent)) {
		return 0;
	}

	if (parent == &usb_pll_clk) {
		mux = 0;
	} else if (parent == &mcu_pll_clk) {
		mux = 1;
	} else if (parent == &turbo_pll_clk) {
		mux = 2;
	} else if (parent == &ckih_clk) {
		mux = 3;
	} else {
		BUG();
	}

	reg = __raw_readl(MXC_CCM_MCR) & ~mask;
	reg |= mux << offset;
	__raw_writel(reg, MXC_CCM_MCR);

	return 0;
}

/*
 * Set parent for those modules with 1 bit selection: SSI1/2, FIR
 */
static inline int __clk_set_parent1(struct clk *clk, struct clk *parent,
				    u32 select)
{
	u32 reg;

	if (unlikely(clk->parent == parent)) {
		return 0;
	}

	reg = __raw_readl(MXC_CCM_MCR);
	if (parent == &mcu_pll_clk) {
		reg &= ~select;
	} else if (parent == &usb_pll_clk) {
		reg |= select;
	} else {
		BUG();
	}
	__raw_writel(reg, MXC_CCM_MCR);

	return 0;
}

static int _clk_ssi1_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent1(clk, parent, MXC_CCM_MCR_SSIS1);
}

static int _clk_ssi2_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent1(clk, parent, MXC_CCM_MCR_SSIS2);
}

static int _clk_firi_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent1(clk, parent, MXC_CCM_MCR_FIRS);
}

static int _clk_csi_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent2(clk, parent, MXC_CCM_MCR_CSIS_MASK,
				 MXC_CCM_MCR_CSIS_OFFSET);
}

static int _clk_sdhc1_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent2(clk, parent, MXC_CCM_MCR_SDHC1S_MASK,
				 MXC_CCM_MCR_SDHC1S_OFFSET);
}

static int _clk_sdhc2_set_parent(struct clk *clk, struct clk *parent)
{
	return __clk_set_parent2(clk, parent, MXC_CCM_MCR_SDHC2S_MASK,
				 MXC_CCM_MCR_SDHC2S_OFFSET);
}

static int _clk_ssi1_baud_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) & ~MXC_CCM_MPDR1_SSI1_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);

	return 0;
}

static void _clk_ssi1_baud_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) | MXC_CCM_MPDR1_SSI1_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);
}

static int _clk_ssi2_baud_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) & ~MXC_CCM_MPDR1_SSI2_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);

	return 0;
}

static void _clk_ssi2_baud_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) | MXC_CCM_MPDR1_SSI2_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);
}

static int _clk_firi_baud_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) & ~MXC_CCM_MPDR1_FIRI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);

	return 0;
}

static void _clk_firi_baud_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_MPDR1) | MXC_CCM_MPDR1_FIRI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);
}

static int _clk_pll_set_parent(struct clk *child, struct clk *parent)
{
	child->parent = parent;

	return 0;
}

static struct clk ckih_clk = {
	.name = "ckih",
	.rate = CKIH_CLK_FREQ,
	.flags = RATE_PROPAGATES,
};

static struct clk ckil_clk = {
	.name = "ckil",
	.rate = CKIL_CLK_FREQ,
	.flags = RATE_PROPAGATES,
};

static struct clk mcu_pll_clk = {
	.name = "mcu_pll",
	.parent = &ckih_clk,
	.recalc = _clk_pll_recalc,
	.set_parent = _clk_pll_set_parent,
	.enable = _clk_mcu_pll_enable,
	.disable = _clk_mcu_pll_disable,
	.flags = RATE_PROPAGATES,
};

static struct clk turbo_pll_clk = {
	.name = "turbo_pll",
	.parent = &ckih_clk,
	.recalc = _clk_pll_recalc,
	.set_parent = _clk_pll_set_parent,
	.enable = _clk_turbo_pll_enable,
	.disable = _clk_turbo_pll_disable,
	.flags = RATE_PROPAGATES,
};

static struct clk usb_pll_clk = {
	.name = "usb_pll",
	.parent = &ckih_clk,
	.recalc = _clk_pll_recalc,
	.enable = _clk_usb_pll_enable,
	.disable = _clk_usb_pll_disable,
	.flags = RATE_PROPAGATES,
};

static struct clk ahb_clk = {
	.name = "ahb_clk",
	.parent = &mcu_pll_clk,	/* could be turbo pll also */
	.recalc = _clk_ahb_recalc,
	.set_parent = _clk_pll_set_parent,
	.flags = RATE_PROPAGATES,
};

static struct clk mcu_clk = {
	.name = "cpu_clk",
	.parent = &mcu_pll_clk,	/* two parents? another one is hclk ?? */
	.recalc = _clk_mcu_recalc,
	.set_rate = _clk_cpu_set_rate,
	.set_parent = _clk_pll_set_parent,
};

static struct clk cspi1_clk = {
	.name = "cspi_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_CSPI1_OFFSET,
	.disable = _clk_disable,
};

static struct clk cspi2_clk = {
	.name = "cspi_clk",
	.id = 1,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_CSPI2_OFFSET,
	.disable = _clk_disable,
};

static struct clk ipg_clk = {
	.name = "ipg_clk",
	.parent = &ahb_clk,
	.recalc = _clk_ipg_recalc,
	.set_parent = _clk_pll_set_parent,
	.flags = RATE_PROPAGATES,
};

static struct clk gpt_clk = {
	.name = "gpt_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_GPTMCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk pwm_clk = {
	.name = "pwm_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_PWMMCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk epit1_clk = {
	.name = "epit_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_EPIT1MCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk epit2_clk = {
	.name = "epit_clk",
	.id = 1,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_EPIT2MCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk nfc_clk = {
	.name = "nfc_clk",
	.id = 0,
	.parent = &ahb_clk,
	.recalc = _clk_nfc_recalc,
	.set_parent = _clk_pll_set_parent,
};

static struct clk ipu_clk = {
	.name = "ipu_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_IPU_OFFSET,
	.disable = _clk_disable,
};

static struct clk kpp_clk = {
	.name = "kpp_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_KPP_OFFSET,
	.disable = _clk_disable,
};

static struct clk usb_clk[] = {
	{
	 .name = "usb_clk",
	 .parent = &usb_pll_clk,
	 .secondary = &usb_clk[1],
	 .recalc = _clk_usb_recalc,
	 .enable = _clk_usb_enable,
	 .disable = _clk_usb_disable,},
	{
	 .name = "usb_ipg_clk",
	 .parent = &ipg_clk,
	 .secondary = &usb_clk[2],
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_MCGR1,
	 .enable_shift = MXC_CCM_MCGR1_USBOTG_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "usb_ahb_clk",
	 .parent = &ahb_clk,},
};

static struct clk csi_clk = {
	.name = "csi_clk",
	.parent = &usb_pll_clk,
	.recalc = _clk_csi_recalc,
	.set_parent = _clk_csi_set_parent,
	.round_rate = _clk_csi_round_rate,
	.set_rate = _clk_csi_set_rate,
	.enable = _clk_csi_enable,
	.disable = _clk_csi_disable,
};

static struct clk uart1_clk = {
	.name = "uart_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_UART1_OFFSET,
	.disable = _clk_disable,
};

static struct clk uart2_clk = {
	.name = "uart_clk",
	.id = 1,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_UART2_OFFSET,
	.disable = _clk_disable,
};

static struct clk uart3_clk = {
	.name = "uart_clk",
	.id = 2,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_UART3_OFFSET,
	.disable = _clk_disable,
};

static struct clk uart4_clk = {
	.name = "uart_clk",
	.id = 3,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_UART4_OFFSET,
	.disable = _clk_disable,
};

static struct clk i2c_clk = {
	.name = "i2c_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_I2C_OFFSET,
	.disable = _clk_disable,
};

static struct clk owire_clk = {
	.name = "owire_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_OWIRE_OFFSET,
	.disable = _clk_disable,
};

static struct clk sdhc1_clk[] = {
	{
	 .name = "sdhc_clk",
	 .id = 0,
	 .parent = &usb_pll_clk,
	 .secondary = &sdhc1_clk[1],
	 .set_parent = _clk_sdhc1_set_parent,
	 .recalc = _clk_sdhc1_recalc,
	 .enable = _clk_sdhc1_enable,
	 .disable = _clk_sdhc1_disable,},
	{
	 .name = "sdhc_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_MCGR1,
	 .enable_shift = MXC_CCM_MCGR1_SDHC1_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,},
};

static struct clk sdhc2_clk[] = {
	{
	 .name = "sdhc_clk",
	 .id = 1,
	 .parent = &usb_pll_clk,
	 .secondary = &sdhc2_clk[1],
	 .set_parent = _clk_sdhc2_set_parent,
	 .recalc = _clk_sdhc2_recalc,
	 .enable = _clk_sdhc2_enable,
	 .disable = _clk_sdhc2_disable,},
	{
	 .name = "sdhc_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_MCGR1,
	 .enable_shift = MXC_CCM_MCGR1_SDHC2_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,},
};

static struct clk ssi1_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 0,
	 .parent = &usb_pll_clk,
	 .secondary = &ssi1_clk[1],
	 .recalc = _clk_ssi1_recalc,
	 .set_parent = _clk_ssi1_set_parent,
	 .enable = _clk_ssi1_baud_enable,
	 .disable = _clk_ssi1_baud_disable,},
	{
	 .name = "ssi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_MCGR0,
	 .enable_shift = MXC_CCM_MCGR0_SSI1_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,}
};

static struct clk ssi2_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 1,
	 .parent = &usb_pll_clk,
	 .secondary = &ssi2_clk[1],
	 .recalc = _clk_ssi2_recalc,
	 .set_parent = _clk_ssi2_set_parent,
	 .enable = _clk_ssi2_baud_enable,
	 .disable = _clk_ssi2_baud_disable,},
	{
	 .name = "ssi_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_MCGR1,
	 .enable_shift = MXC_CCM_MCGR1_SSI2_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,}
};

static struct clk firi_clk[] = {
	{
	 .name = "firi_clk",
	 .id = 0,
	 .parent = &usb_pll_clk,
	 .secondary = &firi_clk[1],
	 .recalc = _clk_firi_recalc,
	 .round_rate = _clk_firi_round_rate,
	 .set_rate = _clk_firi_set_rate,
	 .set_parent = _clk_firi_set_parent,
	 .enable = _clk_firi_baud_enable,
	 .disable = _clk_firi_baud_disable,},
	{
	 .name = "firi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable_reg = MXC_CCM_MCGR0,
	 .enable_shift = MXC_CCM_MCGR0_FIRI_OFFSET,
	 .enable = _clk_enable,
	 .disable = _clk_disable,}
};

static struct clk rtic_clk = {
	.name = "rtic_clk",
	.id = 0,
	.parent = &ahb_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_RTIC_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk sdma_clk[] = {
	{
	 .name = "sdma_ahb_clk",
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_MCGR2,
	 .enable_shift = MXC_CCM_MCGR2_SDMA_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "sdma_ipg_clk",
	 .parent = &ipg_clk,}
};

static struct clk mpeg4_clk = {
	.name = "mpeg4_clk",
	.id = 0,
	.parent = &ahb_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_MPEG4_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk emi_clk = {
	.name = "emi_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_EMI_OFFSET,
	.disable = _clk_disable,
};

static struct clk mu_clk = {
	.name = "mu_clk",
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_MU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk rtc_clk = {
	.name = "rtc_clk",
	.id = 0,
	.parent = &ckil_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_RTC_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk edio_clk = {
	.name = "edio_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_EDIO_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk wdog_clk = {
	.name = "wdog_clk",
	.id = 0,
	.parent = &ckil_clk,
	.enable_reg = MXC_CCM_MCGR0,
	.enable_shift = MXC_CCM_MCGR0_WDOGMCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk iim_clk = {
	.name = "iim_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_IIM_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk sim1_clk = {
	.name = "sim_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_SIM1_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk sim2_clk = {
	.name = "sim_clk",
	.id = 1,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_SIM2_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk gemk_clk = {
	.name = "gemk_clk",
	.id = 0,
	.parent = &ahb_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR1,
	.enable_shift = MXC_CCM_MCGR1_GEM_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk rtr_clk = {
	.name = "rtr_clk",
	.id = 0,
	.parent = &ckil_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_RTRMCU_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk ect_clk = {
	.name = "ect_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_ECT_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk smc_clk = {
	.name = "smc_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_SMC_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk spba_clk = {
	.name = "spba_clk",
	.parent = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_SPBA_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static struct clk sahara_clk = {
	.name = "sahara_clk",
	.id = 0,
	.parent = &ahb_clk,
	.secondary = &ipg_clk,
	.enable_reg = MXC_CCM_MCGR2,
	.enable_shift = MXC_CCM_MCGR2_SAHARA_OFFSET,
	.enable = _clk_enable,
	.disable = _clk_disable,
};

static unsigned long _clk_cko1_round_rate(struct clk *clk, unsigned long rate)
{
	u32 div;

	div = clk->parent->rate / rate;
	if (clk->parent->rate % rate)
		div++;

	if (div > 8) {
		div = 16;
	} else if (div > 4) {
		div = 8;
	} else if (div > 2) {
		div = 4;
	}

	return clk->parent->rate / div;
}

static int _clk_cko1_set_rate(struct clk *clk, unsigned long rate)
{
	u32 reg;
	u32 div;

	div = clk->parent->rate / rate;

	if ((clk->parent->rate / div) != rate)
		return -EINVAL;

	if (div == 16) {
		div = 4;
	} else if (div == 8) {
		div = 3;
	} else if (div == 4) {
		div = 2;
	} else if (div == 2) {
		div = 1;
	} else if (div == 1) {
		div = 0;
	} else {
		return -EINVAL;
	}

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO1DV_MASK;
	reg |= div << MXC_CCM_COSR_CKO1DV_OFFSET;
	__raw_writel(reg, MXC_CCM_COSR);

	clk->rate = rate;
	return 0;
}

static void _clk_cko1_recalc(struct clk *clk)
{
	u32 div;

	div = __raw_readl(MXC_CCM_COSR) & MXC_CCM_COSR_CKO1DV_MASK >>
	    MXC_CCM_COSR_CKO1DV_OFFSET;

	clk->rate = clk->parent->rate / (1 << div);
}

static int _clk_cko1_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO1S_MASK;

	if (parent == &mcu_pll_clk) {
		reg |= 0 << MXC_CCM_COSR_CKO1S_OFFSET;
	} else if (parent == &ckih_clk) {
		reg |= 1 << MXC_CCM_COSR_CKO1S_OFFSET;
	} else if (parent == &usb_pll_clk) {
		reg |= 2 << MXC_CCM_COSR_CKO1S_OFFSET;
	} else {
		return -EINVAL;
	}

	__raw_writel(reg, MXC_CCM_COSR);

	return 0;
}

static int _clk_cko1_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) | MXC_CCM_COSR_CKO1EN;
	__raw_writel(reg, MXC_CCM_COSR);

	return 0;
}

static void _clk_cko1_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO1EN;
	__raw_writel(reg, MXC_CCM_COSR);
}

static unsigned long _clk_cko2_round_rate(struct clk *clk, unsigned long rate)
{
	u32 div;

	div = clk->parent->rate / rate;
	if (clk->parent->rate % rate)
		div++;

	if (div > 8) {
		div = 16;
	} else if (div > 4) {
		div = 8;
	} else if (div > 2) {
		div = 4;
	}

	return clk->parent->rate / div;
}

static int _clk_cko2_set_rate(struct clk *clk, unsigned long rate)
{
	u32 reg;
	u32 div;

	div = clk->parent->rate / rate;

	if ((clk->parent->rate / div) != rate)
		return -EINVAL;

	if (div == 16) {
		div = 4;
	} else if (div == 8) {
		div = 3;
	} else if (div == 4) {
		div = 2;
	} else if (div == 2) {
		div = 1;
	} else if (div == 1) {
		div = 0;
	} else {
		return -EINVAL;
	}

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO2DV_MASK;
	reg |= div << MXC_CCM_COSR_CKO2DV_OFFSET;
	__raw_writel(reg, MXC_CCM_COSR);

	clk->rate = rate;

	return 0;
}

static void _clk_cko2_recalc(struct clk *clk)
{
	u32 div;

	div = __raw_readl(MXC_CCM_COSR) & MXC_CCM_COSR_CKO2DV_MASK >>
	    MXC_CCM_COSR_CKO2DV_OFFSET;

	clk->rate = clk->parent->rate / (1 << div);
}

static int _clk_cko2_set_parent(struct clk *clk, struct clk *parent)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO2S_MASK;

	if (parent == &mcu_pll_clk) {
		reg |= 0 << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &usb_pll_clk) {
		reg |= 2 << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &nfc_clk) {
		reg |= 8 << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &ahb_clk) {
		reg |= 9 << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &ipg_clk) {
		reg |= 0xB << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &turbo_pll_clk) {
		reg |= 0x10 << MXC_CCM_COSR_CKO2S_OFFSET;
	} else if (parent == &mcu_clk) {
		reg |= 0xD << MXC_CCM_COSR_CKO2S_OFFSET;
	} else {
		return -EINVAL;
	}

	__raw_writel(reg, MXC_CCM_COSR);

	return 0;
}

static int _clk_cko2_enable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) | MXC_CCM_COSR_CKO2EN;
	__raw_writel(reg, MXC_CCM_COSR);

	return 0;
}

static void _clk_cko2_disable(struct clk *clk)
{
	u32 reg;

	reg = __raw_readl(MXC_CCM_COSR) & ~MXC_CCM_COSR_CKO2EN;
	__raw_writel(reg, MXC_CCM_COSR);
}

static struct clk cko1_clk = {
	.name = "cko1_clk",
	.recalc = _clk_cko1_recalc,
	.set_rate = _clk_cko1_set_rate,
	.round_rate = _clk_cko1_round_rate,
	.set_parent = _clk_cko1_set_parent,
	.enable = _clk_cko1_enable,
	.disable = _clk_cko1_disable,
};

static struct clk cko2_clk = {
	.name = "cko2_clk",
	.recalc = _clk_cko2_recalc,
	.set_rate = _clk_cko2_set_rate,
	.round_rate = _clk_cko2_round_rate,
	.set_parent = _clk_cko2_set_parent,
	.enable = _clk_cko2_enable,
	.disable = _clk_cko2_disable,
};

static struct clk *mxc_clks[] = {
	&ckih_clk,
	&ckil_clk,
	&mcu_pll_clk,
	&turbo_pll_clk,
	&usb_pll_clk,
	&mcu_clk,
	&ahb_clk,
	&ipg_clk,
	&cko1_clk,
	&cko2_clk,
	&cspi1_clk,
	&cspi2_clk,
	&gpt_clk,
	&pwm_clk,
	&epit1_clk,
	&epit2_clk,
	&nfc_clk,
	&ipu_clk,
	&kpp_clk,
	&usb_clk[0],
	&usb_clk[1],
	&usb_clk[2],
	&csi_clk,
	&uart1_clk,
	&uart2_clk,
	&uart3_clk,
	&uart4_clk,
	&i2c_clk,
	&owire_clk,
	&sdhc1_clk[0],
	&sdhc1_clk[1],
	&sdhc2_clk[0],
	&sdhc2_clk[1],
	&ssi1_clk[0],
	&ssi1_clk[1],
	&ssi2_clk[0],
	&ssi2_clk[1],
	&firi_clk[0],
	&firi_clk[1],
	&rtic_clk,
	&sdma_clk[0],
	&sdma_clk[1],
	&mpeg4_clk,
	&emi_clk,
	&mu_clk,
	&rtc_clk,
	&wdog_clk,
	&iim_clk,
	&edio_clk,
	&sim1_clk,
	&sim2_clk,
	&gemk_clk,
	&rtr_clk,
	&ect_clk,
	&smc_clk,
	&spba_clk,
	&sahara_clk,
};

static int cpu_curr_wp;
static struct cpu_wp *cpu_wp_tbl;
static int cpu_wp_nr;

extern void propagate_rate(struct clk *tclk);

int mxc_clocks_init(void)
{
	u32 reg;
	struct clk **clkp;

	for (clkp = mxc_clks; clkp < mxc_clks + ARRAY_SIZE(mxc_clks); clkp++) {
		clk_register(*clkp);
	}

	/* Turn off all possible clocks */
	reg = __raw_readl(MXC_CCM_MPDR0);
	reg |= MXC_CCM_MPDR0_CSI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR0);

	reg = __raw_readl(MXC_CCM_MPDR1);
	reg |= MXC_CCM_MPDR1_USB_DIS | MXC_CCM_MPDR1_SSI2_DIS |
	    MXC_CCM_MPDR1_SSI1_DIS | MXC_CCM_MPDR1_FIRI_DIS;
	__raw_writel(reg, MXC_CCM_MPDR1);

	reg = __raw_readl(MXC_CCM_MPDR2);
	reg |= MXC_CCM_MPDR2_SDHC1DIS | MXC_CCM_MPDR2_SDHC2DIS;
	__raw_writel(reg, MXC_CCM_MPDR2);

	__raw_writel(MXC_CCM_MCGR0_GPTMCU_MASK, MXC_CCM_MCGR0);
	__raw_writel(MXC_CCM_MCGR1_EMI_MASK, MXC_CCM_MCGR1);
	__raw_writel(0, MXC_CCM_MCGR2);

	cko1_clk.disable(&cko1_clk);
	cko2_clk.disable(&cko2_clk);

	usb_pll_clk.disable(&usb_pll_clk);
	turbo_pll_clk.disable(&turbo_pll_clk);

	/* This will propagate to all children and init all the clock rates */
	propagate_rate(&ckih_clk);
	propagate_rate(&ckil_clk);

	cpu_curr_wp = mcu_clk.rate / ahb_clk.rate - 3;
	cpu_wp_tbl = get_cpu_wp(&cpu_wp_nr);
	clk_enable(&gpt_clk);
	clk_enable(&emi_clk);
	clk_enable(&iim_clk);
	clk_enable(&spba_clk);
	clk_enable(&edio_clk);
	clk_enable(&wdog_clk);

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
	mcu_pll_clk.recalc(&mcu_pll_clk);
	ahb_clk.recalc(&ahb_clk);
	ipg_clk.recalc(&ipg_clk);

	return ipg_clk.rate;
}

#define MXC_MPDR0_MAX_MCU_MASK   (MXC_CCM_MPDR0_BRMM_MASK | \
                                 MXC_CCM_MPDR0_MAX_PDF_MASK | \
                                 MXC_CCM_MPDR0_TPSEL)

/*!
 * Setup cpu clock based on working point.
 * @param       wp      cpu freq working point (0 is the slowest)
 * @return              0 on success or error code on failure.
 */
static int cpu_clk_set_wp(int wp)
{
	struct clk *pll_old = &turbo_pll_clk;
	struct clk *pll_new = &mcu_pll_clk;
	struct cpu_wp *p;

	if (wp >= cpu_wp_nr || wp < 0) {
		printk(KERN_ERR "Wrong wp: %d for cpu_clk_set_wp\n", wp);
		return -EINVAL;
	}

	if (wp == cpu_curr_wp) {
		return 0;
	}

	p = &cpu_wp_tbl[wp];
	if (p->pll_rate > 400000000) {
		pll_old = &mcu_pll_clk;
		pll_new = &turbo_pll_clk;
	}

	clk_enable(pll_new);

	while ((__raw_readl(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DFSP) != 0) ;

	/* no switching is in progress, update MPDR0 */
	__raw_writel((__raw_readl(MXC_CCM_MPDR0) & ~MXC_MPDR0_MAX_MCU_MASK) |
		     p->pdr0_reg, MXC_CCM_MPDR0);

	mcu_clk.parent = pll_new;
	mcu_clk.rate = p->cpu_rate;
	clk_disable(pll_old);

	cpu_curr_wp = wp;

	return 0;
}
