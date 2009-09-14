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

/*!
 * @file clock.c
 * @brief API for setting up and retrieving clocks.
 *
 * This file contains API for setting up and retrieving clocks.
 *
 * @ingroup CLOCKS
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include "crm_regs.h"
/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_SPINLOCK(mxc_crm_lock);

/*!
 * g_emma_clock_map is defined to control the emma_clock .
 * emma_clock will be disabled until emma_prp_clk and emma_pp_clk are closed.
 */
#define MXC_CLK_EMMA_PRP	0
#define MXC_CLK_EMMA_PP		1
static int g_emma_clock_map = 0;

#define	MXC_REG_BIT_MOD(r, off, en)	(r = (r & (~(1 << off))) | (en << off))

/*!
 * This function enables the emma clock.
 * @param	source each bit of source indicate the clock status of mdoule
 *		which is using emma clk
 * @return 	none
 */
static void inline __enable_emma_clk(unsigned long source)
{
	unsigned long reg;

	if (g_emma_clock_map == 0) {
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
		MXC_REG_BIT_MOD(reg, CCM_PCCR0_EMMA_OFFSET, 1);
		MXC_REG_BIT_MOD(reg, CCM_PCCR0_HCLK_EMMA_OFFSET, 1);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
	}
	g_emma_clock_map |= (1 << source);
}

/*!
 * This function disables the emma clock.
 * @param	source each bit of source indicate the clock status of mdoule
 *		which is using emma clk
 * @return 	none
 */
static void inline __disable_emma_clk(unsigned long source)
{
	unsigned long reg;

	g_emma_clock_map &= ~(1 << source);
	if (g_emma_clock_map == 0) {
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
		MXC_REG_BIT_MOD(reg, CCM_PCCR0_EMMA_OFFSET, 0);
		MXC_REG_BIT_MOD(reg, CCM_PCCR0_HCLK_EMMA_OFFSET, 0);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
	}
}

/*!
 * This function returns the PLL output value in Hz based on pll.
 * @param       pll     PLL as defined in enum plls
 * @return      PLL value in Hz.
 */
unsigned long mxc_pll_clock(enum plls pll)
{
	unsigned long mfi = 0, mfn = 0, mfd = 0, pdf = 0;
	unsigned long ref_clk = 0, pll_out = 0;
	volatile unsigned long reg, cscr;

	cscr = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
	if (pll == MCUPLL) {
		if ((cscr & CCM_CSCR_MCU) != 0) {
			ref_clk = CKIH_CLK_FREQ;
		} else {
			ref_clk = CKIL_CLK_FREQ;
		}
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_MPCTL0);
		pdf = (reg & CCM_MPCTL0_PD_MASK) >> CCM_MPCTL0_PD_OFFSET;
		mfd = (reg & CCM_MPCTL0_MFD_MASK) >> CCM_MPCTL0_MFD_OFFSET;
		mfi = (reg & CCM_MPCTL0_MFI_MASK) >> CCM_MPCTL0_MFI_OFFSET;
		mfn = (reg & CCM_MPCTL0_MFN_MASK) >> CCM_MPCTL0_MFN_OFFSET;
	} else if (pll == SERIALPLL) {
		if ((cscr & CCM_CSCR_SP) != 0) {
			ref_clk = CKIH_CLK_FREQ;
		} else {
			ref_clk = CKIL_CLK_FREQ;
		}
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_SPCTL0);
		pdf = (reg & CCM_SPCTL0_PD_MASK) >> CCM_SPCTL0_PD_OFFSET;
		mfd = (reg & CCM_SPCTL0_MFD_MASK) >> CCM_SPCTL0_MFD_OFFSET;
		mfi = (reg & CCM_SPCTL0_MFI_MASK) >> CCM_SPCTL0_MFI_OFFSET;
		mfn = (reg & CCM_SPCTL0_MFN_MASK) >> CCM_SPCTL0_MFN_OFFSET;
	} else {
		printk(KERN_ERR "Wrong PLL: %d\n", pll);
		BUG();		/* oops */
	}

	mfi = (mfi <= 5) ? 5 : mfi;
	pll_out = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
	    (pdf + 1);
	return pll_out;
}

/*!
 * This function returns the mcu main clock frequency
 *
 * @return      mcu main clock value in Hz.
 */
static unsigned long mxc_mcu_main_clock(void)
{
	return mxc_pll_clock(MCUPLL);
}

/*!
 * This function returns the main clock values in Hz.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk)
{
	unsigned long pll, spll, ret_val = 0, hclk;
	unsigned long presc_pdf, ipg_pdf, nfc_pdf, usb_pdf;
	volatile unsigned long cscr =
	    __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
	volatile unsigned long pcdr0 =
	    __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
	volatile unsigned long pcdr1 =
	    __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);

	unsigned long fclk, ipgclk, perclk1, perclk2, perclk3, perclk4;
	unsigned long bclk_pdf;
	unsigned long perclk1_pdf, perclk2_pdf, perclk3_pdf, perclk4_pdf;
	unsigned long clk_src;
	unsigned long ssi1_pdf = 0;
	unsigned long ssi2_pdf = 0;
	presc_pdf = (cscr & CCM_CSCR_PRESC_MASK) >> CCM_CSCR_PRESC_OFFSET;
	bclk_pdf = (cscr & CCM_CSCR_BCLK_MASK) >> CCM_CSCR_BCLK_OFFSET;
	ipg_pdf = (cscr & CCM_CSCR_IPDIV) >> CCM_CSCR_IPDIV_OFFSET;
	perclk1_pdf =
	    (pcdr1 & CCM_PCDR1_PERDIV1_MASK) >> CCM_PCDR1_PERDIV1_OFFSET;
	perclk2_pdf =
	    (pcdr1 & CCM_PCDR1_PERDIV2_MASK) >> CCM_PCDR1_PERDIV2_OFFSET;
	perclk3_pdf =
	    (pcdr1 & CCM_PCDR1_PERDIV3_MASK) >> CCM_PCDR1_PERDIV3_OFFSET;
	perclk4_pdf =
	    (pcdr1 & CCM_PCDR1_PERDIV4_MASK) >> CCM_PCDR1_PERDIV4_OFFSET;

	pll = mxc_mcu_main_clock();
	spll = mxc_pll_clock(SERIALPLL);
	fclk = pll / (presc_pdf + 1);
	hclk = fclk / (bclk_pdf + 1);
	ipgclk = hclk / (ipg_pdf + 1);
	perclk1 = pll / (perclk1_pdf + 1);
	perclk2 = pll / (perclk2_pdf + 1);
	perclk3 = pll / (perclk3_pdf + 1);
	perclk4 = pll / (perclk4_pdf + 1);
	switch (clk) {
	case CPU_CLK:
		ret_val = fclk;
		break;
	case AHB_CLK:
		ret_val = hclk;
		break;
	case PERCLK1:
	case UART6_BAUD:
	case UART5_BAUD:
	case UART4_BAUD:
	case UART3_BAUD:
	case UART2_BAUD:
	case UART1_BAUD:
	case GPT6_CLK:
	case GPT5_CLK:
	case GPT4_CLK:
	case GPT3_CLK:
	case GPT2_CLK:
	case GPT1_CLK:
	case PWM_CLK:
		ret_val = perclk1;
		break;
	case PERCLK2:
	case SDHC2_CLK:
	case SDHC1_CLK:
	case CSPI3_CLK:
	case CSPI2_CLK:
	case CSPI1_CLK:
		ret_val = perclk2;
		break;
	case PERCLK3:
	case LCDC_CLK:
		ret_val = perclk3;
		break;
	case PERCLK4:
	case CSI_BAUD:
		ret_val = perclk4;
		break;
	case USB_CLK:
		usb_pdf = (cscr & CCM_CSCR_USB_MASK) >> CCM_CSCR_USB_OFFSET;
		ret_val = spll / (usb_pdf + 1);
		break;
	case SSI1_BAUD:
		ssi1_pdf = (pcdr0 & CCM_PCDR0_SSI1BAUDDIV_MASK) >>
		    CCM_PCDR0_SSI1BAUDDIV_OFFSET;
		clk_src = (cscr & CCM_CSCR_SSI1) >> CCM_CSCR_SSI1_OFFSET;
		if (clk_src)
			ret_val = pll / (ssi1_pdf + 1);
		else
			ret_val = spll / (ssi1_pdf + 1);
		break;
	case SSI2_BAUD:
		ssi1_pdf = (pcdr0 & CCM_PCDR0_SSI2BAUDDIV_MASK) >>
		    CCM_PCDR0_SSI2BAUDDIV_OFFSET;
		clk_src = (cscr & CCM_CSCR_SSI2) >> CCM_CSCR_SSI2_OFFSET;
		if (clk_src)
			ret_val = pll / (ssi2_pdf + 1);
		else
			ret_val = spll / (ssi2_pdf + 1);
		break;
	case NFC_CLK:
		nfc_pdf = (pcdr0 & CCM_PCDR0_NFCDIV_MASK) >>
		    CCM_PCDR0_NFCDIV_OFFSET;
		ret_val = hclk / (nfc_pdf + 1);
		break;
	default:
		ret_val = ipgclk;
		break;
	}
	return ret_val;
}

/*!
 * This function returns the parent clock values in Hz.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks_parent(enum mxc_clocks clk)
{
	unsigned long ret_val = 0;

	switch (clk) {
	case CSI_BAUD:
		ret_val = mxc_mcu_main_clock();
		break;
	default:
		break;
	}
	return ret_val;
}

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num)
{
	volatile unsigned long cscr;
	unsigned long flags;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	cscr = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);

	switch (clk) {
	case SSI1_BAUD:
		cscr = (cscr & (~CCM_CSCR_SSI1)) |
		    (pll_num << CCM_CSCR_SSI1_OFFSET);
		break;
	case SSI2_BAUD:
		cscr = (cscr & (~CCM_CSCR_SSI2)) |
		    (pll_num << CCM_CSCR_SSI2_OFFSET);
		break;
	default:
		pr_info("Can't choose its clock source: %d\n", clk);
		break;
	}
	__raw_writel(cscr, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return;
}

/*!
 * This function sets the divider value for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass
 *            in 2 times the expected division value to account for FP vals on certain
 *            platforms)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div)
{
	volatile unsigned long reg;
	unsigned long flags;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case SSI2_BAUD:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		reg = (reg & (~CCM_PCDR0_SSI2BAUDDIV_MASK)) |
		    ((div - 1) << CCM_PCDR0_SSI2BAUDDIV_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		reg = (reg & (~CCM_PCDR0_SSI1BAUDDIV_MASK)) |
		    ((div - 1) << CCM_PCDR0_SSI1BAUDDIV_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		break;
	case NFC_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		reg = (reg & (~CCM_PCDR0_NFCDIV_MASK)) |
		    ((div - 1) << CCM_PCDR0_NFCDIV_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		break;
	case PERCLK1:
	case UART4_BAUD:
	case UART3_BAUD:
	case UART2_BAUD:
	case UART1_BAUD:
	case GPT3_CLK:
	case GPT2_CLK:
	case GPT1_CLK:
	case PWM_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		reg = (reg & (~CCM_PCDR1_PERDIV1_MASK)) |
		    ((div - 1) << CCM_PCDR1_PERDIV1_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		break;
	case PERCLK2:
	case SDHC2_CLK:
	case SDHC1_CLK:
	case CSPI3_CLK:
	case CSPI2_CLK:
	case CSPI1_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		reg = (reg & (~CCM_PCDR1_PERDIV2_MASK)) |
		    ((div - 1) << CCM_PCDR1_PERDIV2_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		break;
	case PERCLK3:
	case LCDC_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		reg = (reg & (~CCM_PCDR1_PERDIV3_MASK)) |
		    ((div - 1) << CCM_PCDR1_PERDIV3_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		break;
	case PERCLK4:
	case CSI_BAUD:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		reg = (reg & (~CCM_PCDR1_PERDIV4_MASK)) |
		    ((div - 1) << CCM_PCDR1_PERDIV4_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR1);
		break;
	case USB_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		reg = (reg & (~CCM_CSCR_USB_MASK)) |
		    ((div - 1) << CCM_CSCR_USB_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		break;
	case IPG_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		reg = (reg & (~CCM_CSCR_IPDIV)) |
		    ((div - 1) << CCM_CSCR_IPDIV_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		break;
	case CPU_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		reg = (reg & (~CCM_CSCR_PRESC_MASK)) |
		    ((div - 1) << CCM_CSCR_PRESC_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		break;
	case AHB_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		reg = (reg & (~CCM_CSCR_BCLK_MASK)) |
		    ((div - 1) << CCM_CSCR_BCLK_OFFSET);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		break;
	default:
		break;
	}

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

static void __mxc_clks_config(enum mxc_clocks clk, int enable)
{
	unsigned long flags, reg_up = 0;
	volatile unsigned long reg0, reg1;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	reg0 = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
	reg1 = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR1);

	switch (clk) {
	case CSI_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_PERCLK4_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_CSI_OFFSET, enable);
		break;
	case DMA_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_DMA_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_DMA_OFFSET, enable);
		break;
	case BROM_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_BROM_OFFSET, enable);
		break;
	case EMMA_PRP_CLK:
		__enable_emma_clk(MXC_CLK_EMMA_PRP);
		goto clk_cfg_out;
		break;
	case EMMA_PP_CLK:
		__enable_emma_clk(MXC_CLK_EMMA_PP);
		goto clk_cfg_out;
		break;
	case LCDC_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_LCDC_OFFSET, enable);
		break;
	case SLCDC_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SLCDC_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_SLCDC_OFFSET, enable);
		break;
	case USB_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_HCLK_USBOTG_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_USBOTG_OFFSET, enable);
		break;
	case SSI1_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SSI1_BAUD_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SSI1_OFFSET, enable);
		break;
	case SSI2_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SSI2_BAUD_OFFSET, enable);
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SSI2_OFFSET, enable);
		break;
	case NFC_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_NFC_OFFSET, enable);
		break;
	case UART1_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_UART1_OFFSET, enable);
		break;
	case UART2_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_UART2_OFFSET, enable);
		break;
	case UART3_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_UART3_OFFSET, enable);
		break;
	case UART4_BAUD:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_UART4_OFFSET, enable);
		break;
	case WDOG_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_WDT_OFFSET, enable);
		reg_up = 1;
		break;
	case CSPI3_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_CSPI3_OFFSET, enable);
		break;
	case CSPI2_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_CSPI2_OFFSET, enable);
		break;
	case CSPI1_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_CSPI1_OFFSET, enable);
		break;
	case GPIO_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_GPIO_OFFSET, enable);
		break;
	case GPT3_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_GPT3_OFFSET, enable);
		reg_up = 1;
		break;
	case GPT2_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_GPT2_OFFSET, enable);
		reg_up = 1;
		break;
	case GPT1_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_GPT1_OFFSET, enable);
		reg_up = 1;
		break;
	case I2C1_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_I2C_OFFSET, enable);
		break;
	case KPP_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_KPP_OFFSET, enable);
		reg_up = 1;
		break;
	case OWIRE_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_OWIRE_OFFSET, enable);
		reg_up = 1;
		break;
	case PWM_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_PWM_OFFSET, enable);
		reg_up = 1;
		break;
	case RTC_CLK:
		MXC_REG_BIT_MOD(reg1, CCM_PCCR1_RTC_OFFSET, enable);
		reg_up = 1;
		break;
	case SDHC2_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SDHC2_OFFSET, enable);
		break;
	case SDHC1_CLK:
		MXC_REG_BIT_MOD(reg0, CCM_PCCR0_SDHC1_OFFSET, enable);
		break;
	default:
		goto clk_cfg_out;
	}
	if (reg_up == 0) {
		__raw_writel(reg0, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
	} else if (reg_up == 1) {
		__raw_writel(reg1, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR1);
	} else {
		__raw_writel(reg0, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR0);
		__raw_writel(reg1, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCCR1);
	}

      clk_cfg_out:
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to enable the individual module clocks
 *
 * @param       clk     as defined in enum mxc_clocks
 */

void mxc_clks_enable(enum mxc_clocks clk)
{
	__mxc_clks_config(clk, 1);
}

/*!
 * This function is called to disable the individual module clocks
 *
 * @param       clk     as defined in enum mxc_clocks
 */
void mxc_clks_disable(enum mxc_clocks clk)
{
	__mxc_clks_config(clk, 0);
}

/*!
 * This function is used to modify PLL registers to generate the required
 * frequency.
 *
 * @param  pll_num  the PLL that you wish to modify
 * @param  mfi      multiplication factor integer part
 * @param  pdf      pre-division factor
 * @param  mfd      multiplication factor denominator
 * @param  mfn      multiplication factor numerator
 */
void mxc_pll_set(enum plls pll_num, unsigned int mfi, unsigned int pdf,
		 unsigned int mfd, unsigned int mfn)
{
	volatile unsigned long cscr;
	unsigned long flags;
	unsigned long new_pll = 0;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	if (pll_num == MCUPLL) {
		/* Change the Pll value      */
		new_pll = (mfi << CCM_MPCTL0_MFI_OFFSET) |
		    (mfn << CCM_MPCTL0_MFN_OFFSET) |
		    (mfd << CCM_MPCTL0_MFD_OFFSET) |
		    (pdf << CCM_MPCTL0_PD_OFFSET);

		__raw_writel(new_pll, IO_ADDRESS(CCM_BASE_ADDR) + CCM_MPCTL0);
		/* Swap to reference clock and disable PLL */
		cscr = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		cscr |= CCM_CSCR_MPLLRES;
		__raw_writel(cscr, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
	} else {
		if (pll_num == SERIALPLL) {
			/* Change the Pll value      */
			new_pll = (mfi << CCM_SPCTL0_MFI_OFFSET) |
			    (mfn << CCM_SPCTL0_MFN_OFFSET) |
			    (mfd << CCM_SPCTL0_MFD_OFFSET) |
			    (pdf << CCM_SPCTL0_PD_OFFSET);

			__raw_writel(new_pll,
				     IO_ADDRESS(CCM_BASE_ADDR) + CCM_SPCTL0);
			/* Swap to reference clock and disable PLL */
			cscr =
			    __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
			cscr |= CCM_CSCR_SPLLRES;
			__raw_writel(cscr,
				     IO_ADDRESS(CCM_BASE_ADDR) + CCM_CSCR);
		}
	}

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * Configure clock output on CKO pins
 *
 * @param   output  clock output pin
 * @param   clk     clock source to output
 * @param   div     CLKO divider
 *
 */
void mxc_set_clock_output(enum mxc_clk_out output, enum mxc_clocks clk, int div)
{
	unsigned long flags;
	volatile unsigned long reg;

	if (output != CKO) {
		return;
	}

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case CKIH_CLK:
		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_CCSR);
		reg = (reg & (~0x1f)) | 0x2;
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_CCSR);

		reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);
		reg = (reg & (~0x03c00000)) | 0x02000000 | ((div - 1) << 22);
		__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR) + CCM_PCDR0);

		break;
	default:
		break;
	};

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return;
}

EXPORT_SYMBOL(mxc_pll_set);
EXPORT_SYMBOL(mxc_pll_clock);
EXPORT_SYMBOL(mxc_get_clocks);
EXPORT_SYMBOL(mxc_set_clocks_pll);
EXPORT_SYMBOL(mxc_set_clocks_div);
EXPORT_SYMBOL(mxc_clks_disable);
EXPORT_SYMBOL(mxc_clks_enable);
EXPORT_SYMBOL(mxc_set_clock_output);
EXPORT_SYMBOL(mxc_get_clocks_parent);
