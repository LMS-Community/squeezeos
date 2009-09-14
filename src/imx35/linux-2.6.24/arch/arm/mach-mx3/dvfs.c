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
 * @file mach-mx3/dvfs.c
 *
 * @brief Driver for the Freescale Semiconductor MXC DVFS module.
 *
 * The DVFS driver
 * driver is designed as a character driver which interacts with the MXC DVFS
 * hardware. Upon initialization, the DVFS driver initializes the DVFS hardware
 * sets up driver nodes attaches to the DVFS interrupt and initializes internal
 * data structures. When the DVFS interrupt occurs the driver checks the cause
 * of the interrupt (lower frequency, increase frequency or emergency) and changes
 * the CPU voltage according to translation table that is loaded into the driver.
 * The driver read method is used to read the log buffer.
 * Driver ioctls are used to change driver parameters and enable/disable the
 * DVFS operation.
 *
 * @ingroup PM_MX31
 */

/* Define to enable debug messages */
#undef DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/sdma.h>
#include <asm/arch/dvfs.h>

#include "iomux.h"
#include "crm_regs.h"

/*!
 * Read the LBFL bits from the CCM
 */
#define GET_LBFL(pmcr0)	\
(pmcr0 & MXC_CCM_PMCR0_LBFL) >> MXC_CCM_PMCR0_LBFL_OFFSET

/*!
 * Read the LBCF bits from the CCM
 */
#define GET_LBCF(pmcr0)	\
(pmcr0 & MXC_CCM_PMCR0_LBCF_MASK) >> MXC_CCM_PMCR0_LBCF_OFFSET

/*!
 * Set DVSUP bits
 */
#define SET_DVSUP(pmcr0, x) \
pmcr0 = ((pmcr0 & ~MXC_CCM_PMCR0_DVSUP_MASK) | x << MXC_CCM_PMCR0_DVSUP_OFFSET)

/*!
 * Set DFSUP0 bits
 */
#define SET_DFSUP0(pmcr0, x) \
pmcr0 = ((pmcr0 & ~MXC_CCM_PMCR0_DFSUP0) | x << MXC_CCM_PMCR0_DFSUP0_OFFSET)

/*!
 * Set DFSUP1 bits
 */
#define SET_DFSUP1(pmcr0, x) \
pmcr0 = ((pmcr0 & ~MXC_CCM_PMCR0_DFSUP1) | x << MXC_CCM_PMCR0_DFSUP1_OFFSET)

/*
 * Set UDSC bit
 */
#define SET_UDSC(pmcr0) pmcr0 |= MXC_CCM_PMCR0_UDSC

/*
 * Clear UDSC bit
 */
#define CLEAR_UDSC(pmcr0) pmcr0 &= ~MXC_CCM_PMCR0_UDSC

#define SET_VSCNT(pmcr0, x) \
pmcr0 = ((pmcr0 & ~MXC_CCM_PMCR0_VSCNT_MASK) | x << MXC_CCM_PMCR0_VSCNT_OFFSET)

/*
 * Clear DPVCR bit
 */
#define CLEAR_DPVCR(pmcr0) pmcr0 &= ~MXC_CCM_PMCR0_DPVCR

/*
 * Set DPVCR bit
 */
#define SET_DPVCR(pmcr0) pmcr0 |= MXC_CCM_PMCR0_DPVCR

/*
 * Set DPVCR bit
 */
#define SET_DPVV(pmcr0) pmcr0 |= MXC_CCM_PMCR0_DPVV

dvfs_states_table *dvfs_states_tbl;

/*!
 * The dvfs_dptc_params structure holds all the internal DPTC driver parameters
 * (current working point, current frequency, translation table and DPTC
 * log buffer).
 */
static dvfs_dptc_params_s *dvfs_dptc_params;

/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_SPINLOCK(mxc_crm_lock);

/*!
 * This function is called to read the contents of a CCM_MCU register
 *
 * @param reg_offset the CCM_MCU register that will read
 *
 * @return the register contents
 */
static unsigned long mxc_ccm_get_reg(unsigned int reg_offset)
{
	unsigned long reg;

	reg = __raw_readl(reg_offset);
	return reg;
}

/*!
 * This function is called to modify the contents of a CCM_MCU register
 *
 * @param reg_offset the CCM_MCU register that will read
 * @param mask       the mask to be used to clear the bits that are to be modified
 * @param data       the data that should be written to the register
 */
static void mxc_ccm_modify_reg(unsigned int reg_offset, unsigned int mask,
			       unsigned int data)
{
	unsigned long flags;
	unsigned long reg;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	reg = __raw_readl(reg_offset);
	reg = (reg & (~mask)) | data;
	__raw_writel(reg, reg_offset);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * Set the div_3_clk clock to maximum.
 * With the ARM clocked at 532, this setting yields
 * a DIV_3_CLK of 2.03 kHz.
 *
 * @param val   div3ck value
 */
static void dvfs_set_div3ck(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR0, MXC_CCM_LTR0_DIV3CK_MASK,
			   val << MXC_CCM_LTR0_DIV3CK_OFFSET);
}

/*!
 * Sets frequency decrease threshold. Decrease frequency change request
 * will be sent if DVFS counter value will be less than this value.
 *
 * @param val   threshold value
 */
static void dvfs_set_dnthr(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR0, MXC_CCM_LTR0_DNTHR_MASK,
			   val << MXC_CCM_LTR0_DNTHR_OFFSET);
}

/*!
 * Set frequency increase threshold. Increase frequency change request
 * will be sent if DVFS counter value will be more than this value.
 *
 * @param val   threshold value
 */
static void dvfs_set_upthr(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR0, MXC_CCM_LTR0_UPTHR_MASK,
			   val << MXC_CCM_LTR0_UPTHR_OFFSET);
}

/*!
 * Set panic threshold. Panic frequency change request
 * will be sent if DVFS counter value will be more than this value.
 *
 * @param val   threshold value
 */
static void dvfs_set_pncthr(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR1, MXC_CCM_LTR1_PNCTHR_MASK,
			   val << MXC_CCM_LTR1_PNCTHR_OFFSET);
}

/*!
 * Set UPCNT value.
 * UPCNT defines the amount of times the up threshold should be exceeded
 * before DVFS will trigger frequency increase request.
 *
 * @param val   threshold value
 */
static void dvfs_set_upcnt(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR1, MXC_CCM_LTR1_UPCNT_MASK,
			   val << MXC_CCM_LTR1_UPCNT_OFFSET);
}

/*!
 * Set DNCNT value.
 * DNCNT defines the amount of times the up threshold should be exceeded
 * before DVFS will trigger frequency decrease request.
 *
 * @param val   threshold value
 */
static void dvfs_set_dncnt(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR1, MXC_CCM_LTR1_DNCNT_MASK,
			   val << MXC_CCM_LTR1_DNCNT_OFFSET);
}

/*!
 * Set load tracking buffer source to ld_add
 */
static void dvfs_set_ltbrsr(void)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR1, MXC_CCM_LTR1_LTBRSR,
			   MXC_CCM_LTR1_LTBRSR);
}

#ifdef CONFIG_MXC_DVFS_SDMA
/*!
 * Take the original MSB of ld_add - ld_add[5:2] for load tracking register
 */
static void dvfs_clear_ltbrsh(void)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR1, MXC_CCM_LTR1_LTBRSH, 0);
}
#endif

/*!
 * Set EMAC value.
 * EMAC defines how many samples are included in EMA calculation
 *
 * @param val   EMAC value
 */
static void dvfs_set_emac(unsigned long val)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR2, MXC_CCM_LTR2_EMAC_MASK,
			   val << MXC_CCM_LTR2_EMAC_OFFSET);
}

/*!
 * Set general purpose weights of LTR2 register
 *
 * @param x   general purpose bit number
 * @param w   weight value
 */
static void dvfs_set_ltr2_wsw(unsigned long x, unsigned long w)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR2, MXC_CCM_LTR2_WSW_MASK(x),
			   w << MXC_CCM_LTR2_WSW_OFFSET(x));
}

/*!
 * Set general purpose weights of LTR3 register
 *
 * @param x   general purpose bit number
 * @param w   weight value
 */
static void dvfs_set_ltr3_wsw(unsigned long x, unsigned long w)
{
	mxc_ccm_modify_reg(MXC_CCM_LTR3, MXC_CCM_LTR3_WSW_MASK(x),
			   w << MXC_CCM_LTR3_WSW_OFFSET(x));
}

/*!
 * Unmask DVFS interrupt
 */
static void dvfs_unmask_dvfs_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_FSVAIM, 0);
}

/*!
 * Mask DVFS interrupt
 */
static void dvfs_mask_dvfs_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_FSVAIM,
			   MXC_CCM_PMCR0_FSVAIM);
}

/*!
 * Mask DVFS log buffer interrupt
 */
static void dvfs_mask_dvfs_lb_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_LBMI,
			   MXC_CCM_PMCR0_LBMI);
}

#ifdef CONFIG_MXC_DVFS_SDMA
/*!
 * Unmask DVFS log buffer interrupt
 */
static void dvfs_unmask_dvfs_lb_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_LBMI, 0);
}
#endif

#ifndef CONFIG_MXC_DVFS_SDMA
/*!
 * Sets DVFIS bit. MCU will get DVFS interrupt
 */
static void dvfs_set_dvfis(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DVFIS,
			   MXC_CCM_PMCR0_DVFIS);
}
#endif

/*!
 * Enable DVFS hardware
 */
static void dvfs_enable_dvfs(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DVFEN,
			   MXC_CCM_PMCR0_DVFEN);
}

/*!
 * Disable DVFS hardware
 */
static void dvfs_disable_dvfs(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DVFEN, 0);
}

#ifndef CONFIG_MXC_DVFS_SDMA
/*!
 * Read the FSVAI bits from the CCM.
 *
 * @return  FSVAI bits value
 */
static unsigned long dvfs_get_fsvai(void)
{
	return ((mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_FSVAI_MASK)
		>> MXC_CCM_PMCR0_FSVAI_OFFSET);
}
#endif

/*!
 * Read the DFSUP0 bit from the CCM
 *
 * @return  DFSUP0 bit value
 */
static unsigned long dvfs_get_dfsup0(void)
{
	return ((mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DFSUP0)
		>> MXC_CCM_PMCR0_DFSUP0_OFFSET);
}

/*!
 * Read the DFSUP1 bits from the CCM
 *
 * @return  DFSUP1 bit value
 */
static unsigned long dvfs_get_dfsup1(void)
{
	return ((mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DFSUP1)
		>> MXC_CCM_PMCR0_DFSUP1_OFFSET);
}

/*!
 * Read the DVSUP bits from the CCM
 *
 * @return  DVFSUP bits value
 */
unsigned long dvfs_get_dvsup(void)
{
	return ((mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DVSUP_MASK)
		>> MXC_CCM_PMCR0_DVSUP_OFFSET);
}

/*!
 * Set SW general purpose SW bits.
 *
 * @param x    status bits. Each of 4 LSB's corresponds to
 *             1 SW general purpose bit status (on/off).
 */
void dvfs_set_dvgp(unsigned long x)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR1, MXC_CCM_PMCR1_DVGP_MASK, x & 0xf);
}

/*!
 * This function sets the weight of general purpose signals
 * @param   gp_id   number of general purpose bit
 * @param   weight  the weight of the general purpose bit
 */
static void set_gp_weight(int gp_id, unsigned char weight)
{
	if (gp_id < 9) {
		dvfs_set_ltr3_wsw(gp_id, weight & 0x7);
	} else if (gp_id < 16) {
		dvfs_set_ltr2_wsw(gp_id, weight & 0x7);
	}
}

/*!
 * This function returns the ARM clock value in Hz.
 *
 * @param       reg     pll value
 * @param       pdr0     post-divider 0 value
 *
 * @return      clock value in Hz
 */
unsigned long dvfs_get_clock(unsigned long reg, unsigned long pdr0)
{
	unsigned long pll, ret_val = 0;
	signed long mcu_pdf;
	signed long pdf, mfd, mfi, mfn, ref_clk;
	struct clk *pll_clk;
	struct clk *parent_clk;

	pll_clk = clk_get(NULL, "mcu_pll");
	parent_clk = clk_get(NULL, "ckih");
	if (parent_clk == clk_get_parent(pll_clk)) {
		ref_clk = clk_get_rate(parent_clk);
	} else {		/* parent is ckil/fpm */
		parent_clk = clk_get(NULL, "ckil");
		ref_clk = clk_get_rate(parent_clk) * 1024;
	}

	pdf = (signed long)
	    ((reg & MXC_CCM_PCTL_PD_MASK) >> MXC_CCM_PCTL_PD_OFFSET);
	mfd = (signed long)
	    ((reg & MXC_CCM_PCTL_MFD_MASK) >> MXC_CCM_PCTL_MFD_OFFSET);

	mfi = (signed long)((reg & MXC_CCM_PCTL_MFI_MASK) >>
			    MXC_CCM_PCTL_MFI_OFFSET);
	mfi = (mfi <= 5) ? 5 : mfi;
	mfn = (signed long)(reg & MXC_CCM_PCTL_MFN_MASK);
	mfn = (mfn < 0x200) ? mfn : (mfn - 0x400);

	pll = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
	    (pdf + 1);

	mcu_pdf = pdr0 & MXC_CCM_PDR0_MCU_PODF_MASK;
	ret_val = pll / (mcu_pdf + 1);

	return ret_val;
}

/*!
 * This function is called for module initialization.
 * It initializes the driver data structures and sets up the DVFS hardware.
 * It sets default values for DVFS thresholds and counters. The default
 * values was chosen from a set of different reasonable values. They was tested
 * and the default values in the driver gave the best results.
 * More work should be done to find optimal values.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return   0 to indicate success else returns a negative number.
 *
 */
int __init init_dvfs_controller(dvfs_dptc_params_s * params)
{
	int i;
	int res = 0;

	/* Configure 2 MC13783 DVFS pins */
	mxc_request_iomux(MX31_PIN_DVFS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_DVFS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE);

	/* Configure MC13783 voltage ready input pin */
	mxc_request_iomux(MX31_PIN_GPIO1_5, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_FUNC);

	/* Mask DVFS interrupts */
	dvfs_mask_dvfs_int();
	dvfs_mask_dvfs_lb_int();

	params->dvfs_log_buffer = (char *)
	    dma_alloc_coherent(NULL, DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8,
			       &params->dvfs_log_buffer_phys, GFP_KERNEL);

	if (params->dvfs_log_buffer <= 0) {
		printk(KERN_ERR "DVFS failed allocating log buffer\n");
		res = -ENOMEM;
	} else {
		memset(params->dvfs_log_buffer, 0,
		       DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8);
	}

	/* Set general purpose weights to 0 */
	for (i = 0; i < 16; i++) {
		set_gp_weight(i, 0);
	}

	/*
	 * Set the div_3_clk clock to maximum.
	 * With the ARM clocked at 532, this setting yields
	 * a DIV_3_CLK of 2.03 kHz.
	 */
	dvfs_set_div3ck(3);

#ifndef CONFIG_MXC_DVFS_SDMA
	/*
	 * ARM will get DVFS interrupt
	 */
	dvfs_set_dvfis();
#endif

	/*
	 * UPCNT defines the amount of times the up threshold should be exceeded
	 * before DVFS will trigger frequency increase request.
	 */
	dvfs_set_upcnt(0x33);

	/*
	 * DNCNT defines the amount of times the up threshold should be exceeded
	 * before DVFS will trigger frequency decrease request.
	 */
	dvfs_set_dncnt(0x33);

	/* EMAC defines how many samples are included in EMA calculation */
	dvfs_set_emac(0x20);

	/* Initialize frequencies tables for DPM usage */
	dvfs_states_tbl = kmalloc(sizeof(dvfs_states_table), GFP_KERNEL);
	memset(dvfs_states_tbl, 0, sizeof(dvfs_states_table));

	params->dvfs_mode = DVFS_HW_MODE;

	dvfs_dptc_params = params;

	if ((cpu_is_mx31_rev(CHIP_REV_2_0) > 0) || cpu_is_mx32()) {
		/* Rev2.0 changes */

		/* Disable PLL restart on DVFS switch */
		mxc_ccm_modify_reg(MXC_CCM_PMCR1, MXC_CCM_PMCR1_PLLRDIS,
				   MXC_CCM_PMCR1_PLLRDIS);

		/* Enable automatic EMI handshake */
		mxc_ccm_modify_reg(MXC_CCM_PMCR1, MXC_CCM_PMCR1_EMIRQ_EN,
				   MXC_CCM_PMCR1_EMIRQ_EN);

		/* Init SR PLL for 399MHz */
		mxc_ccm_modify_reg(MXC_CCM_SRPCTL, 0xffffffff, 0x00331c23);
	}

	if (res == 0) {
		printk(KERN_INFO "DVFS controller initialized\n");
	}

	return res;
}

/*!
 * Update tables of frequencies for DPM usage
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DVFS &
 *                                   DPTC translation table.
 */
void dvfs_update_freqs_table(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr)
{
	int freq;
	dvfs_state *table;
	unsigned long pll, pdr0;
	int i;

	if (dvfs_states_tbl->freqs != 0) {
		kfree(dvfs_states_tbl->freqs);
	}

	dvfs_states_tbl->freqs = kmalloc(sizeof(unsigned int) *
					 dvfs_dptc_tables_ptr->dvfs_state_num,
					 GFP_KERNEL);

	dvfs_states_tbl->num_of_states = dvfs_dptc_tables_ptr->dvfs_state_num;

	table = dvfs_dptc_tables_ptr->table;

	/* State 0 */
	pll = table[0].pll_up;
	pdr0 = table[0].pdr0_up;
	freq = dvfs_get_clock(table[0].pll_up, table[0].pdr0_up);
	dvfs_states_tbl->freqs[0] = freq;

	for (i = 0; i < dvfs_dptc_tables_ptr->dvfs_state_num - 1; i++) {

		if (table[i].pll_sw_down == 1) {
			pll = table[i].pll_down;
		}
		pdr0 = table[i].pdr0_down;

		freq = dvfs_get_clock(pll, pdr0);
		dvfs_states_tbl->freqs[i + 1] = freq;
	}
}

/*!
 * This function enables the DVFS module. this function updates the DVFS
 * thresholds, updates the MC13783, unmasks the DVFS interrupt and enables
 * the DVFS module
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DVFS module was enabled else returns -EINVAL.
 *
 */
int start_dvfs(dvfs_dptc_params_s * params)
{
	int i;
	int dnthr, upthr, pncthr;

	/* Check if DVFS module isn't already active */
	if (params->dvfs_is_active == FALSE) {

		for (i = 12; i <= 15; i++) {
			set_gp_weight(i, 7);
		}

		if (params->dvfs_mode == DVFS_HW_MODE) {
			dnthr = DVFS_DNTHR;
			upthr = DVFS_UPTHR;
			pncthr = DVFS_PNCTHR;
		} else {
			dnthr = 0;
			upthr = 63;
			pncthr = 63;
		}

		/*
		 * Frequency decrease threshold. Decrease frequency change request
		 * will be sent if DVFS counter value will be less than this value.
		 */
		dvfs_set_dnthr(dnthr);
		/*
		 * Frequency increase threshold. Increase frequency change request
		 * will be sent if DVFS counter value will be more than this value.
		 */
		dvfs_set_upthr(upthr);
		/*
		 * Panic threshold. Panic frequency change request
		 * will be sent if DVFS counter value will be more than this value.
		 */
		dvfs_set_pncthr(pncthr);

		dvfs_unmask_dvfs_int();

#ifdef CONFIG_MXC_DVFS_SDMA
		if (params->dvfs_mode == DVFS_PRED_MODE) {
			params->chars_in_buffer = 0;
			init_waitqueue_head(&params->dvfs_pred_wait);
			params->read_ptr = params->dvfs_log_buffer;
			dvfs_unmask_dvfs_lb_int();
			dvfs_clear_ltbrsh();
		}
#endif

		dvfs_set_ltbrsr();

		dvfs_enable_dvfs();

		params->dvfs_is_active = TRUE;

		return 0;
	}

	/* DVFS module already active return error */
	return -EINVAL;
}

/*!
 * This function sets frequency according to fsvai bits (increase/decrease).
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    fsvai     value of fsvai bits (increase/decrease/emergency)
 */
void set_freq(dvfs_dptc_params_s * params, int fsvai)
{
	int dvsup;
	unsigned long pdr0;
	unsigned long pll;
	int curr_freq_index;
	int pll_switch;
	int dfsup0, dfsup1;
	dvfs_state *table;
	unsigned long pmcr0;
	int dptc_enable = 0;

	pmcr0 = mxc_ccm_get_reg(MXC_CCM_PMCR0);
	dptc_enable = pmcr0 & MXC_CCM_PMCR0_DPTEN;

	/* Read DVSUP - current frequency index */
	dvsup = dvfs_get_dvsup();
	curr_freq_index = dvsup;

	table = params->dvfs_dptc_tables_ptr->table;

#ifdef CONFIG_MXC_DVFS_SDMA
	table = sdma_phys_to_virt((unsigned long)table);
#endif

	if (dptc_enable) {
		CLEAR_DPVCR(pmcr0);
	}
	if (fsvai == DVFS_DECREASE) {
		/* On DVFS_DECREASE event the frequency will be changed according
		 * to the next state setting
		 */
		CLEAR_UDSC(pmcr0);
		dvsup++;
		SET_VSCNT(pmcr0, 1);
		pll_switch = table[curr_freq_index].pll_sw_down;
		pdr0 = table[curr_freq_index].pdr0_down;
		pll = table[curr_freq_index].pll_down;

	} else {
		/* On DVFS_INCREASE and on DVFS_PANIC event the highest
		 * frequency will be set
		 */
		SET_UDSC(pmcr0);
		dvsup = 0;
		SET_VSCNT(pmcr0, table[curr_freq_index].vscnt);
		pll_switch = table[curr_freq_index].pll_sw_up;
		pdr0 = table[curr_freq_index].pdr0_up;
		pll = table[curr_freq_index].pll_up;
	}

	/* DVSUP (new frequency index) setup */
	SET_DVSUP(pmcr0, dvsup);

	/* DFSUP defines if PLL switch is required and which PLL source
	 * will be used after the switch.
	 */
	dfsup0 = dvfs_get_dfsup0();
	dfsup1 = dvfs_get_dfsup1();

	if (pll_switch == 0) {
		/* Update only pdr0 */
		dfsup0 = 1;
	}

	if (pll_switch == 1) {
		/* Update pll and pdr0 */
		dfsup1 = (~dfsup1) & 0x1;
		dfsup0 = 0;
	}

	SET_DFSUP0(pmcr0, dfsup0);
	SET_DFSUP1(pmcr0, dfsup1);

	mxc_ccm_modify_reg(MXC_CCM_PMCR0, 0xffffffff, pmcr0);
	/* software wait for voltage ramp-up */
	udelay(100);
	mxc_ccm_modify_reg(MXC_CCM_PDR0, 0xffffffff, pdr0);

	if (pll_switch == 1) {
		if (dfsup1 == 1) {
			/* MPCTL will be updated */
			mxc_ccm_modify_reg(MXC_CCM_MPCTL, 0xffffffff, pll);
		} else {
			/* SRPCTL will be updated */
			mxc_ccm_modify_reg(MXC_CCM_SRPCTL, 0xffffffff, pll);
		}
	}

	if (dptc_enable) {
		pmcr0 = mxc_ccm_get_reg(MXC_CCM_PMCR0);
		SET_DPVCR(pmcr0);
		SET_DPVV(pmcr0);
		mxc_ccm_modify_reg(MXC_CCM_PMCR0, 0xffffffff, pmcr0);
	}

	pr_debug(KERN_INFO "ARM frequency: %dMHz CKIH frequency: %dMHz(%d)\n",
		 (int)mxc_get_clocks(CPU_CLK) / 1000000,
		 (int)mxc_get_clocks(CKIH_CLK) / 1000000, (int)jiffies);
}

/*!
 * This function disables the DVFS module.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DVFS module was enabled else returns -EINVAL.
 */
int stop_dvfs(dvfs_dptc_params_s * params)
{
	unsigned long dvsup;
	unsigned long dfsup1;
	/* Check if DPTC module isn't already disabled */
	if (params->dvfs_is_active != FALSE) {
		/* Mask interrupts */
		dvfs_mask_dvfs_int();
		dvfs_mask_dvfs_lb_int();

		params->dvfs_is_active = FALSE;

		/* Set maximal frequency */
		dvsup = dvfs_get_dvsup();
		if (dvsup != 0) {
			set_freq(params, DVFS_INCREASE);
		}

		/* If current PLL is SRPCTL, move to MPCTL */
		dfsup1 = dvfs_get_dfsup1();
		if (dfsup1 != 1) {
			set_freq(params, DVFS_INCREASE);
		}

		/* Disable DVFS */
		dvfs_disable_dvfs();

		return 0;
	}

	/* DVFS module already disabled return error */
	return -EINVAL;
}

/*!
 * This function turns on/off SW general purpose bits.
 * The argument's 4 LSBs represent the status of the bits.
 *
 * @param   arg  status of the SW general purpose bits
 *
 * @return 0 on success
 */
int set_sw_gp(unsigned char arg)
{
	/* Clear upper 4 bits */
	arg &= 0xf;
	dvfs_set_dvgp(arg);

	return 0;
}

#ifndef CONFIG_MXC_DVFS_SDMA
/*!
 * This function is the DVFS Interrupt handler.
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 */
void dvfs_irq(dvfs_dptc_params_s * params)
{
	int fsvai;
	unsigned long pmcr0;

	fsvai = dvfs_get_fsvai();
	pmcr0 = mxc_ccm_get_reg(MXC_CCM_PMCR0);

	if (pmcr0 & MXC_CCM_PMCR0_FSVAIM) {
		/* Do nothing. DVFS interrupt is masked */
		return;
	}

	if (fsvai == 0) {
		/* Do nothing. Freq change is not required */
		return;
	}

	if (!(pmcr0 & MXC_CCM_PMCR0_UPDTEN)) {
		/* Do nothing. DVFS didn't finish previous flow update */
		return;
	}

	if (((dvfs_get_dvsup() == 3) && (fsvai == DVFS_DECREASE)) ||
	    ((dvfs_get_dvsup() == 0) && ((fsvai == DVFS_INCREASE) ||
					 (fsvai == DVFS_EMERG)))) {
		/* Do nothing. DVFS is already at lowest (highest) state */
		return;
	}

	set_freq(params, fsvai);

}
#endif				/* CONFIG_MXC_DVFS_SDMA */

/*!
 * This function sets DVFS to monitor WFI signal
 *
 * @param   arg  0 - turn WFI off, 1 - turn WFI on
 * @return  0 on success, error code on fail
 */
int set_wfi(unsigned char arg)
{
	int res;

	if (arg == 0) {
		/* Disable */
		mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_WFIM,
				   MXC_CCM_PMCR0_WFIM);
		res = 0;
	} else if (arg == 1) {
		/* Enable */
		mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_WFIM, 0);
		res = 0;
	} else {
		res = -EINVAL;
	}

	return res;
}

/*!
 * This function changes the frequency if DVFS HW is disabled.
 * It gets id of the required state supported by DVFS table and
 * updates CCM.
 *
 * @param    dvfs_state_id  id of the DVFS state.
 * @return   0 on success, error code on fail
 */
int dvfs_set_state(int dvfs_state_id)
{
	int curr_freq_index;

	if (dvfs_dptc_params->dvfs_is_active != FALSE &&
	    dvfs_dptc_params->dvfs_mode != DVFS_PRED_MODE) {
		/* Frequency change is impossible when DVFS HW is enabled */
		return -EINVAL;
	}

	if (dvfs_state_id >= 0 &&
	    dvfs_state_id <
	    dvfs_dptc_params->dvfs_dptc_tables_ptr->dvfs_state_num) {
		curr_freq_index = dvfs_get_dvsup();
		if (curr_freq_index > dvfs_state_id) {
			/* printk("Increasing frequency to %d\n", dvfs_state_id); */
			if (dvfs_dptc_params->dvfs_is_active == FALSE) {
				dvfs_enable_dvfs();
			}
			set_freq(dvfs_dptc_params, DVFS_INCREASE);
			if (dvfs_dptc_params->dvfs_is_active == FALSE) {
				dvfs_disable_dvfs();
			}
			return dvfs_set_state(dvfs_state_id);
		} else if (curr_freq_index < dvfs_state_id) {
			/* printk("Decreasing frequency to %d\n", dvfs_state_id); */
			if (dvfs_dptc_params->dvfs_is_active == FALSE) {
				dvfs_enable_dvfs();
			}
			set_freq(dvfs_dptc_params, DVFS_DECREASE);
			if (dvfs_dptc_params->dvfs_is_active == FALSE) {
				dvfs_disable_dvfs();
			}
			return dvfs_set_state(dvfs_state_id);
		} else {
			return 0;
		}
	}

	return -EINVAL;
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("DVFS driver");
MODULE_LICENSE("GPL");
