/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/proc-fns.h>
#include <asm/system.h>
#include <asm/arch/mxc.h>
#include "crm_regs.h"

/*!
 * @defgroup MSL_MXC91321 MXC91321 Machine Specific Layer (MSL)
 */

/*!
 * @file mach-mxc91321/system.c
 * @brief This file contains idle and reset functions.
 *
 * @ingroup MSL_MXC91321
 */

extern int mxc_jtag_enabled;

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	unsigned int reg;

	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	if (!mxc_jtag_enabled) {
		__raw_writel(((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_3))
			      | MXC_CCM_MCR_LPM_1), MXC_CCM_MCR);
		/*
		 * workaround ARM11 Platform defect TLSbo64855.
		 * Also see TLSbo78761.
		 */
		reg = __raw_readl(MXC_CCM_MPDR0);
		/* make sure ARM:AHB clock ratio is 1:1 */
		__raw_writel((reg & ~MXC_CCM_MPDR0_BRMM_MASK) | 0x4,
			     MXC_CCM_MPDR0);
		__raw_readl(MXC_CCM_MPDR0);
		__raw_readl(MXC_CCM_MPDR0);

		/* WFI */
		cpu_do_idle();

		/* restore original clock dividers */
		__raw_writel(reg, MXC_CCM_MPDR0);
	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	/* Assert Watchdog Reset signal */
	mxc_wd_reset();
}
