/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file sensor_clock.c
 *
 * @brief camera clock function
 *
 * @ingroup Camera
 */
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/clk.h>

/*
 * set_mclk_rate
 *
 * @param       p_mclk_freq  mclk frequence
 *
 */
void set_mclk_rate(uint32_t * p_mclk_freq)
{
	struct clk *clk;
	uint32_t freq = 0;

	clk = clk_get(NULL, "csi_clk");

	freq = clk_round_rate(clk, *p_mclk_freq);
	clk_set_rate(clk, freq);

	*p_mclk_freq = freq;

	clk_put(clk);
	pr_debug("mclk frequency = %d\n", *p_mclk_freq);
}

/* Exported symbols for modules. */
EXPORT_SYMBOL(set_mclk_rate);
