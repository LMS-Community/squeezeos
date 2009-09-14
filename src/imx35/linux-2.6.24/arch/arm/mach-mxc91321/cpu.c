/*
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*!
 * @file mach-mxc91321/cpu.c
 *
 * @brief This file contains the CPU initialization code.
 *
 * @ingroup MSL_MXC91321
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/hardware/cache-l2x0.h>

/*!
 * CPU initialization. It is called by fixup_mxc_board()
 */
void __init mxc_cpu_init(void)
{
	/* Disable Peripheral Port Remap register */
	asm("mov r0, #0				\n\
	 mcr p15, 0, r0, c15, c2, 4");

	if (!system_rev) {
		mxc_set_system_rev(0x91321, CHIP_REV_1_2);
	}
}

/*!
 * Post CPU init code
 *
 * @return 0 always
 */
static int __init post_cpu_init(void)
{
	void *l2_base;
	volatile unsigned long aips_reg;

	/* Initialize L2 cache */
	l2_base = ioremap(L2CC_BASE_ADDR, SZ_4K);
	if (l2_base) {
		l2x0_init(l2_base, 0x00030024, 0x00000000);
	}

	/*
	 * S/W workaround: Clear the off platform peripheral modules
	 * Supervisor Protect bit for SDMA to access them.
	 */
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));

	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));

	return 0;
}

postcore_initcall(post_cpu_init);
