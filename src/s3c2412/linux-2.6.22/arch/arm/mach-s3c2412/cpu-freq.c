/* linux/arch/arm/mach-s3c2412/cpu-freq.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@simtec.co.uk>
 *
 * S3C2412 CPU Frequency scalling
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
#include <asm/arch/regs-s3c2412-mem.h>

#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/cpu-freq.h>
#include <asm/plat-s3c24xx/clock.h>

#if 0
#undef s3c_freq_dbg
#define s3c_freq_dbg(x...) printk(KERN_INFO x)
#endif

static struct clk *xtal;
static struct clk *fclk;
static struct clk *hclk;
static struct clk *armclk;

/* HDIV: 1, 2, 3, 4, 6, 8 */

int s3c2412_cpufreq_calcdivs(struct s3c24xx_cpufreq_config *cfg)
{
	struct s3c24xx_cpufreq_info *drv = cfg->info;
	unsigned int hdiv, pdiv, armdiv;
	unsigned long hclk, fclk, armclk;
	unsigned long hclk_max;

	fclk = cfg->freq.fclk;
	armclk = cfg->freq.armclk;
	hclk_max = drv->max.hclk;

	/* We can't run hclk above armclk as at the best we have to
	 * have armclk and hclk in dvs mode. */

	if (hclk_max > armclk)
		hclk_max = armclk;

	s3c_freq_dbg("%s: fclk=%lu, armclk=%lu, hclk_max=%lu\n",
		     __func__, fclk, armclk, hclk_max);
	s3c_freq_dbg("%s: want f=%lu, arm=%lu, h=%lu, p=%lu\n",
		     __func__, cfg->freq.fclk, cfg->freq.armclk,
		     cfg->freq.hclk, cfg->freq.pclk);

	armdiv = fclk / armclk;

	if (armdiv < 1)
		armdiv = 1;
	if (armdiv > 2)
		armdiv = 2;

	cfg->divs.arm_divisor = armdiv;
	armclk = fclk / armdiv;

	hdiv = armclk / hclk_max;
	if (hdiv < 1)
		hdiv = 1;

	cfg->freq.hclk = hclk = armclk / hdiv;

	s3c_freq_dbg("%s: armclk %lu, hclk %lu, armdiv %d, hdiv %d\n",
		     __func__, armclk, hclk, armdiv, hdiv);

	if (hdiv > 4)
		goto invalid;

	pdiv = (hclk > drv->max.pclk) ? 2 : 1;
	
	if ((hclk / pdiv) > drv->max.pclk)
		pdiv++;

	cfg->freq.pclk = hclk / pdiv;

	s3c_freq_dbg("%s: pdiv %d\n", __func__, pdiv);

	if (pdiv > 2)
		goto invalid;

	pdiv *= hdiv;

	/* store the result, and then return */

	cfg->divs.h_divisor = hdiv * armdiv;
	cfg->divs.p_divisor = pdiv * armdiv;

	return 0;

 invalid:
	return -EINVAL;
}

static void s3c2412_cpufreq_setdivs(struct s3c24xx_cpufreq_config *cfg)
{
	unsigned long clkdiv;
	unsigned long olddiv;
	int dvs;

	olddiv = clkdiv = __raw_readl(S3C2410_CLKDIVN);

	/* clear off current clock info */

	clkdiv &= ~S3C2412_CLKDIVN_ARMDIVN;
	clkdiv &= ~S3C2412_CLKDIVN_HDIVN_MASK;
	clkdiv &= ~S3C2412_CLKDIVN_PDIVN;

	if (cfg->divs.arm_divisor == 2)
		clkdiv |= S3C2412_CLKDIVN_ARMDIVN;

	clkdiv |= ((cfg->divs.h_divisor / cfg->divs.arm_divisor) - 1);
	
	if (cfg->divs.p_divisor == (cfg->divs.h_divisor * 2))
		clkdiv |= S3C2412_CLKDIVN_PDIVN;

	s3c_freq_dbg("%s: div %08lx => %08lx\n", __func__, olddiv, clkdiv);
	__raw_writel(clkdiv, S3C2410_CLKDIVN);

	dvs = (cfg->freq.armclk <= cfg->freq.hclk);
	clk_set_parent(armclk, dvs ? hclk : fclk);
}

static void s3c2412_cpufreq_setrefresh(struct s3c24xx_cpufreq_config *cfg)
{
	struct s3c24xx_cpufreq_board *board = cfg->board;
	unsigned long refresh;

	WARN_ON(board == NULL);

	if (board == NULL)
		return;

	s3c_freq_dbg("%s: refresh %u ns, hclk %lu\n", __func__,
		     board->refresh, cfg->freq.hclk);

	/* Reduce both the refresh time (in ns) and the frequency (in MHz)
	 * by 10 each to ensure that we do not overflow 32 bit numbers. This
	 * should work for HCLK up to 133MHz and refresh period up to 30usec.
	 */

	refresh = (board->refresh / 10);
	refresh *= (cfg->freq.hclk / 100);
	refresh /= (1 * 1000 * 1000);	/* 10^6 */

	s3c_freq_dbg("%s: setting refresh 0x%08lx\n", __func__, refresh);
	__raw_writel(refresh, S3C2412_REFRESH);
}

/* default handlers for io timings until we add this properly */

static int s3c2412_iotiming_calc(struct s3c24xx_cpufreq_config *cfg,
				 struct s3c24xx_iotimings *iot)
{
	struct s3c24xx_cpufreq_board *board = cfg->board;
	int bank;
	int nr_used = 0;

	WARN_ON(board == NULL);

	if (board == NULL)
		return -EINVAL;

	for (bank = 0; bank < S3C2412_MAX_IO; bank++) {
		if (board->banks[bank] == NULL)
			continue;

		nr_used++;
	}

	/* Return an error if there are any IO banks defined, as we do not
	 * currently have any method for setting these. */

	return (nr_used > 0) ? -EINVAL : 0;
}

static void s3c2412_iotiming_set(struct s3c24xx_cpufreq_config *cfg,
				 struct s3c24xx_iotimings *iot)
{
}

/* set the default cpu frequency information, based on an 200MHz part
 * as we have no other way of detecting the speed rating in software.
 */

struct s3c24xx_cpufreq_info s3c2412_cpufreq_info = {
	.max		= {
		.fclk	= 200000000,
		.hclk	= 100000000,
		.pclk	=  50000000,
	},

	.calc_iotiming	= s3c2412_iotiming_calc,
	.set_iotiming	= s3c2412_iotiming_set,
	//.get_iotiming	= s3c2410_iotiming_get,

	.set_refresh	= s3c2412_cpufreq_setrefresh,
	.set_divs	= s3c2412_cpufreq_setdivs,
	.calc_divs	= s3c2412_cpufreq_calcdivs,
};

static int s3c2412_cpufreq_add(struct sys_device *sysdev)
{
	unsigned long fclk_rate;

	hclk = clk_get(NULL, "hclk");
	if (IS_ERR(hclk)) {
		printk(KERN_ERR "%s: cannot find hclk clock\n", __func__);
		return -ENOENT;
	}

	fclk = clk_get(NULL, "fclk");
	if (IS_ERR(fclk)) {
		printk(KERN_ERR "%s: cannot find fclk clock\n", __func__);
		return -ENOENT;
	}

	fclk_rate = clk_get_rate(fclk);
	if (fclk_rate > 200000000) {
		printk(KERN_INFO "%s: fclk is %ld MHz, assuming 266MHz capable part\n", __func__, fclk_rate / 1000000);
		s3c2412_cpufreq_info.max.fclk = 266000000;
		s3c2412_cpufreq_info.max.hclk = 133000000;
		s3c2412_cpufreq_info.max.pclk =  66000000;
	}

	armclk = clk_get(NULL, "armclk");
	if (IS_ERR(armclk)) {
		printk(KERN_ERR "%s: cannot find arm clock\n", __func__);
		return -ENOENT;
	}

	xtal = clk_get(NULL, "xtal");
	if (IS_ERR(xtal)) {
		printk(KERN_ERR "%s: cannot find xtal clock\n", __func__);
		return -ENOENT;
	}

	return s3c24xx_cpufreq_register(&s3c2412_cpufreq_info);
}

static struct sysdev_driver s3c2412_cpufreq_driver = {
	.add		= s3c2412_cpufreq_add,
	.suspend	= s3c24xx_cpufreq_suspend,
	.resume		= s3c24xx_cpufreq_resume,
};

static int s3c2412_cpufreq_init(void)
{
	return sysdev_driver_register(&s3c2412_sysclass,
				      &s3c2412_cpufreq_driver);
}

arch_initcall(s3c2412_cpufreq_init);
