/* arch/arm/mach-s3c2440/pll-16934400.c
 *
 * Copyright (c) 2006-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@arm.linux.org.uk>
 *
 * S3C2440 CPU PLL tables (16.93444MHz Crystal)
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
#include <linux/sysdev.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/cpu-freq.h>

static struct s3c24xx_pllval s3c2440_plls_169344[] __initdata = {
	{ 78019200,	PLLVAL(121,5,3)	}, 	/* FVco 624.153600 */
	{ 84067200,	PLLVAL(131,5,3)	}, 	/* FVco 672.537600 */
	{ 90115200,	PLLVAL(141,5,3)	}, 	/* FVco 720.921600 */
	{ 96163200,	PLLVAL(151,5,3)	}, 	/* FVco 769.305600 */
	{ 102135600,	PLLVAL(185,6,3)	}, 	/* FVco 817.084800 */
	{ 108259200,	PLLVAL(171,5,3)	}, 	/* FVco 866.073600 */
	{ 114307200,	PLLVAL(127,3,3)	}, 	/* FVco 914.457600 */
	{ 120234240,	PLLVAL(134,3,3)	}, 	/* FVco 961.873920 */
	{ 126161280,	PLLVAL(141,3,3)	}, 	/* FVco 1009.290240 */
	{ 132088320,	PLLVAL(148,3,3)	}, 	/* FVco 1056.706560 */
	{ 138015360,	PLLVAL(155,3,3)	}, 	/* FVco 1104.122880 */
	{ 144789120,	PLLVAL(163,3,3)	}, 	/* FVco 1158.312960 */
	{ 150100363,	PLLVAL(187,9,2)	}, 	/* FVco 600.401454 */
	{ 156038400,	PLLVAL(121,5,2)	}, 	/* FVco 624.153600 */
	{ 162086400,	PLLVAL(126,5,2)	}, 	/* FVco 648.345600 */
	{ 168134400,	PLLVAL(131,5,2)	}, 	/* FVco 672.537600 */
	{ 174048000,	PLLVAL(177,7,2)	}, 	/* FVco 696.192000 */
	{ 180230400,	PLLVAL(141,5,2)	}, 	/* FVco 720.921600 */
	{ 186278400,	PLLVAL(124,4,2)	}, 	/* FVco 745.113600 */
	{ 192326400,	PLLVAL(151,5,2)	}, 	/* FVco 769.305600 */
	{ 198132480,	PLLVAL(109,3,2)	}, 	/* FVco 792.529920 */
	{ 204271200,	PLLVAL(185,6,2)	}, 	/* FVco 817.084800 */
	{ 210268800,	PLLVAL(141,4,2)	}, 	/* FVco 841.075200 */
	{ 216518400,	PLLVAL(171,5,2)	}, 	/* FVco 866.073600 */
	{ 222264000,	PLLVAL(97,2,2)	}, 	/* FVco 889.056000 */
	{ 228614400,	PLLVAL(127,3,2)	}, 	/* FVco 914.457600 */
	{ 234259200,	PLLVAL(158,4,2)	}, 	/* FVco 937.036800 */
	{ 240468480,	PLLVAL(134,3,2)	}, 	/* FVco 961.873920 */
	{ 246960000,	PLLVAL(167,4,2)	}, 	/* FVco 987.840000 */
	{ 252322560,	PLLVAL(141,3,2)	}, 	/* FVco 1009.290240 */
	{ 258249600,	PLLVAL(114,2,2)	}, 	/* FVco 1032.998400 */
	{ 264176640,	PLLVAL(148,3,2)	}, 	/* FVco 1056.706560 */
	{ 270950400,	PLLVAL(120,2,2)	}, 	/* FVco 1083.801600 */
	{ 276030720,	PLLVAL(155,3,2)	}, 	/* FVco 1104.122880 */
	{ 282240000,	PLLVAL(92,1,2)	}, 	/* FVco 1128.960000 */
	{ 289578240,	PLLVAL(163,3,2)	}, 	/* FVco 1158.312960 */
	{ 294235200,	PLLVAL(131,2,2)	}, 	/* FVco 1176.940800 */
	{ 300200727,	PLLVAL(187,9,1)	}, 	/* FVco 600.401454 */
	{ 306358690,	PLLVAL(191,9,1)	}, 	/* FVco 612.717380 */
	{ 312076800,	PLLVAL(121,5,1)	}, 	/* FVco 624.153600 */
	{ 318366720,	PLLVAL(86,3,1)	}, 	/* FVco 636.733440 */
	{ 324172800,	PLLVAL(126,5,1)	}, 	/* FVco 648.345600 */
	{ 330220800,	PLLVAL(109,4,1)	}, 	/* FVco 660.441600 */
	{ 336268800,	PLLVAL(131,5,1)	}, 	/* FVco 672.537600 */
	{ 342074880,	PLLVAL(93,3,1)	}, 	/* FVco 684.149760 */
	{ 348096000,	PLLVAL(177,7,1)	}, 	/* FVco 696.192000 */
	{ 355622400,	PLLVAL(118,4,1)	}, 	/* FVco 711.244800 */
	{ 360460800,	PLLVAL(141,5,1)	}, 	/* FVco 720.921600 */
	{ 366206400,	PLLVAL(165,6,1)	}, 	/* FVco 732.412800 */
	{ 372556800,	PLLVAL(124,4,1)	}, 	/* FVco 745.113600 */
	{ 378201600,	PLLVAL(126,4,1)	}, 	/* FVco 756.403200 */
	{ 384652800,	PLLVAL(151,5,1)	}, 	/* FVco 769.305600 */
	{ 391608000,	PLLVAL(177,6,1)	}, 	/* FVco 783.216000 */
	{ 396264960,	PLLVAL(109,3,1)	}, 	/* FVco 792.529920 */
	{ 402192000,	PLLVAL(87,2,1)	}, 	/* FVco 804.384000 */
};

static int s3c2440_plls169344_add(struct sys_device *dev)
{
	struct clk *xtal_clk;
	unsigned long xtal;

	xtal_clk = clk_get(NULL, "xtal");
	if (IS_ERR(xtal_clk))
		return PTR_ERR(xtal_clk);

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	if (xtal == 169344000) {
		printk(KERN_INFO "Registering PLL table for 16.9344MHz crystal\n");
		return s3c24xx_plls_register(s3c2440_plls_169344,
					     ARRAY_SIZE(s3c2440_plls_169344));
	}

	return 0;
}

static struct sysdev_driver s3c2440_plls169344_drv = {
	.add	= s3c2440_plls169344_add,
};

static int __init s3c2440_pll_16934400(void)
{
	return sysdev_driver_register(&s3c2440_sysclass,
				      &s3c2440_plls169344_drv);

}

arch_initcall(s3c2440_pll_16934400);
