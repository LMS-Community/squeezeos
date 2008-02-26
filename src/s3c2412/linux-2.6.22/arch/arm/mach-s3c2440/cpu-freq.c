/* linux/arch/arm/mach-s3c2440/cpu-freq.c
 *
 * Copyright (c) 2006,2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@simtec.co.uk>
 *
 * S3C2440 CPU Frequency scalling
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


#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/sysdev.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/regs-clock.h>

#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/cpu-freq.h>
#include <asm/plat-s3c24xx/clock.h>

static struct clk *xtal;

/* HDIV: 1, 2, 3, 4, 6, 8 */

int s3c2440_cpufreq_calcdivs(struct s3c24xx_cpufreq_config *cfg)
{
	struct s3c24xx_cpufreq_info *drv = cfg->info;
	unsigned int hdiv, pdiv;
	unsigned long hclk, fclk;
	unsigned long hclk_max;

	fclk = cfg->freq.fclk;
	hclk_max = drv->max.hclk;

	s3c_freq_dbg("%s: fclk is %lu, max hclk %lu\n",
		     __func__, fclk, hclk_max);

	for (hdiv = 1; hdiv < 9; hdiv++) {
		if (hdiv == 5 || hdiv == 7)
			hdiv++;
		
		hclk = (fclk / hdiv);
		if (hclk <= hclk_max)
			break;
	}

	s3c_freq_dbg("%s: hclk %lu, div %d\n", __func__, hclk, hdiv);

	if (hdiv > 8)
		goto invalid;

	pdiv = (hclk > drv->max.pclk) ? 2 : 1;
	
	if ((hclk / pdiv) > drv->max.pclk)
		pdiv++;

	s3c_freq_dbg("%s: pdiv %d\n", __func__, pdiv);

	if (pdiv > 2)
		goto invalid;

	pdiv *= hdiv;

	/* store the result, and then return */

	cfg->divs.h_divisor = hdiv;
	cfg->divs.p_divisor = pdiv;

	return 0;

 invalid:
	return -EINVAL;
}

#define CAMDIVN_HCLK_HALF (S3C2440_CAMDIVN_HCLK3_HALF | S3C2440_CAMDIVN_HCLK4_HALF)

static void s3c2440_cpufreq_setdivs(struct s3c24xx_cpufreq_config *cfg)
{
	unsigned long clkdiv, camdiv;

	clkdiv = __raw_readl(S3C2410_CLKDIVN);
	camdiv = __raw_readl(S3C2440_CAMDIVN);

	clkdiv &= ~S3C2440_CLKDIVN_HDIVN_MASK;
	camdiv &= ~(S3C2440_CAMDIVN_HCLK3_HALF | S3C2440_CAMDIVN_HCLK4_HALF);

	switch (cfg->divs.h_divisor) {
	case 1:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_1;
		break;

	case 2:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_2;
		break;
			
	case 6:
		camdiv |= S3C2440_CAMDIVN_HCLK3_HALF;
	case 3:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_4_8;
		break;

	case 8:
	case 4:
		break;

	default:
		BUG();
	}

	/* Write the divisors first with hclk intentionally halved so that
	 * when we write clkdiv we will under-frequency instead of over. We
	 * then make a short delay and remove the hclk halving if necessary.
	 */

	__raw_writel(camdiv | CAMDIVN_HCLK_HALF, S3C2440_CAMDIVN);
	__raw_writel(clkdiv, S3C2410_CLKDIVN);

	ndelay(20);
	__raw_writel(camdiv, S3C2440_CAMDIVN);
}

static void s3c2440_cpufreq_setrefresh(struct s3c24xx_cpufreq_config *cfg)
{
}

struct s3c24xx_cpufreq_info s3c2440_cpufreq_info = {
	.max		= {
		.fclk	= 400000000,
		.hclk	= 133333333,
		.pclk	=  66666666,
	},

	.calc_iotiming	= s3c2410_iotiming_calc,
	.set_iotiming	= s3c2410_iotiming_set,
	.get_iotiming	= s3c2410_iotiming_get,

	.set_refresh	= s3c2440_cpufreq_setrefresh,
	.set_divs	= s3c2440_cpufreq_setdivs,
	.calc_divs	= s3c2440_cpufreq_calcdivs,
};

static int s3c2440_cpufreq_add(struct sys_device *sysdev)
{
	xtal = clk_get(NULL, "xtal");
	if (IS_ERR(xtal)) {
		printk(KERN_ERR "%s: cannot find xtal clock\n", __func__);
		return -ENOENT;
	}

	return s3c24xx_cpufreq_register(&s3c2440_cpufreq_info);
}

static struct sysdev_driver s3c2440_cpufreq_driver = {
	.add		= s3c2440_cpufreq_add,
	.suspend	= s3c24xx_cpufreq_suspend,
	.resume		= s3c24xx_cpufreq_resume,
};

static int s3c2440_cpufreq_init(void)
{
	return sysdev_driver_register(&s3c2440_sysclass,
				      &s3c2440_cpufreq_driver);
}

arch_initcall(s3c2440_cpufreq_init);

static int s3c2442_cpufreq_init(void)
{
	return sysdev_driver_register(&s3c2442_sysclass,
				      &s3c2440_cpufreq_driver);
}

arch_initcall(s3c2442_cpufreq_init);
