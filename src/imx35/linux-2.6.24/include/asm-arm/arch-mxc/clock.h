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

#ifndef __ASM_ARCH_MXC_CLOCK_H__
#define __ASM_ARCH_MXC_CLOCK_H__

/*!
 * @defgroup CLOCKS Clock Control API
 */
/*!
 * @file arch-mxc/clock.h
 *
 * @brief API for setting up and retrieving clocks.
 *
 * This file contains API for setting up and retrieving clocks.
 *
 * @ingroup CLOCKS
 */
#ifndef __ASSEMBLY__

#include <linux/list.h>

struct module;

/*!
 * MXC specific struct for defining and controlling a single clock.
 *
 * This structure should not be accessed directly. Use clk_xxx API all
 * clock accesses.
 */
struct clk {
	struct list_head node;
	struct module *owner;
	const char *name;
	int id;
	/*! Source clock this clk depends on */
	struct clk *parent;
	/*! Secondary clock to enable/disable with this clock */
	struct clk *secondary;
	/*! Current clock rate */
	unsigned long rate;
	/*! Reference count of clock enable/disable */
	s8 usecount;
	/*! Register bit position for clock's enable/disable control. */
	u8 enable_shift;
	/*! Register address for clock's enable/disable control. */
	u32 enable_reg;
	u32 flags;
	/*! Function ptr to recalculate the clock's rate based on parent
	   clock's rate */
	void (*recalc) (struct clk *);
	/*! Function ptr to set the clock to a new rate. The rate must match a
	   supported rate returned from round_rate. Leave blank if clock is not
	   programmable */
	int (*set_rate) (struct clk *, unsigned long);
	/*! Function ptr to round the requested clock rate to the nearest
	   supported rate that is less than or equal to the requested rate. */
	unsigned long (*round_rate) (struct clk *, unsigned long);
	/*! Function ptr to enable the clock. Leave blank if clock can not
	   be gated. */
	int (*enable) (struct clk *);
	/*! Function ptr to disable the clock. Leave blank if clock can not
	   be gated. */
	void (*disable) (struct clk *);
	/*! Function ptr to set the parent clock of the clock. */
	int (*set_parent) (struct clk *, struct clk *);
};

int clk_register(struct clk *clk);
void clk_unregister(struct clk *clk);
int clk_get_usecount(struct clk *clk);
int clk_set_pll_dither(struct clk *clk, unsigned int pll_ppm);

/* Clock flags */
#define RATE_PROPAGATES		(1 << 0)	/* Program children too */
#define ALWAYS_ENABLED		(1 << 1)	/* Clock cannot be disabled */
#define RATE_FIXED		(1 << 2)	/* Fixed clock rate */

#endif				/* __ASSEMBLY__ */
#endif				/* __ASM_ARCH_MXC_CLOCK_H__ */
