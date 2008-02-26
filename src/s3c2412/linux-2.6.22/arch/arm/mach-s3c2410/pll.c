/* arch/arm/mach-s3c2410/pll.c
 *
 * Copyright (c) 2006,2007 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@arm.linux.org.uk>
 *
 * S3C2410 CPU PLL tables
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clock.h"
#include "cpu-freq.h"
#include "s3c2440-plls.h"

static struct s3c24xx_pllval pll_vals_12MHz[] = {
    { 34000,  PLLVAL(82,2,3)  },
    { 45000,  PLLVAL(82,1,3)  },
    { 51000,  PLLVAL(161,3,3) },
    { 48000,  PLLVAL(120,2,3) },
    { 56000,  PLLVAL(142,2,3) },
    { 68000,  PLLVAL(82,2,2)  },
    { 79000,  PLLVAL(71,1,2)  },
    { 85000,  PLLVAL(105,2,2) },
    { 90000,  PLLVAL(112,2,2) },
    { 101000, PLLVAL(127,2,2) },
    { 113000, PLLVAL(105,1,2) },
    { 118000, PLLVAL(150,2,2) },
    { 124000, PLLVAL(116,1,2) },
    { 135000, PLLVAL(82,2,1)  },
    { 147000, PLLVAL(90,2,1)  },
    { 152000, PLLVAL(68,1,1)  },
    { 158000, PLLVAL(71,1,1)  },
    { 170000, PLLVAL(77,1,1)  },
    { 180000, PLLVAL(82,1,1)  },
    { 186000, PLLVAL(85,1,1)  },
    { 192000, PLLVAL(88,1,1)  },
    { 203000, PLLVAL(161,3,1) },

    /* 2410A extras */

    { 210000, PLLVAL(132,2,1) },
    { 226000, PLLVAL(105,1,1) },
    { 266000, PLLVAL(125,1,1) },
    { 268000, PLLVAL(126,1,1) },
    { 270000, PLLVAL(127,1,1) },
};

static struct s3c24xx_plls pllvals_12MHz = {
	.vals	= pll_vals_12MHz,
	.size	= ARRAY_SIZE(pll_vals_12MHz),
};

/* TODO - make selection based on 2410 vs 2410A */
struct s3c24xx_pllvals *s3c2440_get_pll(void)
{
	return &pllvals_12MHz;
}
