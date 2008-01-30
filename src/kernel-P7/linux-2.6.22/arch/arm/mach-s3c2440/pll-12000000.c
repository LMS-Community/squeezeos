/* arch/arm/mach-s3c2440/pll-1200000.c
 *
 * Copyright (c) 2006,2007 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@arm.linux.org.uk>
 *
 * S3C2440 CPU PLL tables (12MHz Crystal)
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

static struct s3c24xx_pllval s3c2440_plls_12[] __initdata = {
	{ 75000000,	PLLVAL(0x75,3,3) }, 	/* FVco 600.000000 */
	{ 80000000,	PLLVAL(0x98,4,3) }, 	/* FVco 640.000000 */
	{ 90000000,	PLLVAL(0x70,2,3) }, 	/* FVco 720.000000 */
	{ 100000000,	PLLVAL(0x5c,1,3) }, 	/* FVco 800.000000 */
	{ 110000000,	PLLVAL(0x66,1,3) }, 	/* FVco 880.000000 */
	{ 120000000,	PLLVAL(0x70,1,3) }, 	/* FVco 960.000000 */
	{ 150000000,	PLLVAL(0x75,3,2) }, 	/* FVco 600.000000 */
	{ 160000000,	PLLVAL(0x98,4,2) }, 	/* FVco 640.000000 */
	{ 170000000,	PLLVAL(0x4d,1,2) }, 	/* FVco 680.000000 */
	{ 180000000,	PLLVAL(0x70,2,2) }, 	/* FVco 720.000000 */
	{ 190000000,	PLLVAL(0x57,1,2) }, 	/* FVco 760.000000 */
	{ 200000000,	PLLVAL(0x5c,1,2) }, 	/* FVco 800.000000 */
	{ 210000000,	PLLVAL(0x84,2,2) }, 	/* FVco 840.000000 */
	{ 220000000,	PLLVAL(0x66,1,2) }, 	/* FVco 880.000000 */
	{ 230000000,	PLLVAL(0x6b,1,2) }, 	/* FVco 920.000000 */
	{ 240000000,	PLLVAL(0x70,1,2) }, 	/* FVco 960.000000 */
	{ 300000000,	PLLVAL(0x75,3,1) }, 	/* FVco 600.000000 */
	{ 310000000,	PLLVAL(0x93,4,1) }, 	/* FVco 620.000000 */
	{ 320000000,	PLLVAL(0x98,4,1) }, 	/* FVco 640.000000 */
	{ 330000000,	PLLVAL(0x66,2,1) }, 	/* FVco 660.000000 */
	{ 340000000,	PLLVAL(0x4d,1,1) }, 	/* FVco 680.000000 */
	{ 350000000,	PLLVAL(0xa7,4,1) }, 	/* FVco 700.000000 */
	{ 360000000,	PLLVAL(0x70,2,1) }, 	/* FVco 720.000000 */
	{ 370000000,	PLLVAL(0xb1,4,1) }, 	/* FVco 740.000000 */
	{ 380000000,	PLLVAL(0x57,1,1) }, 	/* FVco 760.000000 */
	{ 390000000,	PLLVAL(0x7a,2,1) }, 	/* FVco 780.000000 */
	{ 400000000,	PLLVAL(0x5c,1,1) }, 	/* FVco 800.000000 */
};

static int s3c2440_plls12_add(struct sys_device *dev)
{
	struct clk *xtal_clk;
	unsigned long xtal;

	xtal_clk = clk_get(NULL, "xtal");
	if (IS_ERR(xtal_clk))
		return PTR_ERR(xtal_clk);

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	if (xtal == 12000000) {
		printk("Registering PLL table for 12MHz crystal\n");
		return s3c24xx_plls_register(s3c2440_plls_12,
					     ARRAY_SIZE(s3c2440_plls_12));
	}

	return 0;
}

static struct sysdev_driver s3c2440_plls12_drv = {
	.add	= s3c2440_plls12_add,
};

static int __init s3c2440_pll_12mhz(void)
{
	return sysdev_driver_register(&s3c2440_sysclass, &s3c2440_plls12_drv);

}

arch_initcall(s3c2440_pll_12mhz);
