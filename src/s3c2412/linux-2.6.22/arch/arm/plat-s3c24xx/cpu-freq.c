/* linux/arch/arm/plat-s3c24xx/cpu-freq.c
 *
 * Copyright (c) 2006,2007,2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX CPU Frequency scalling
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
#include <linux/clk.h>
#include <linux/err.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/clock.h>
#include <asm/plat-s3c24xx/cpu-freq.h>

#include <asm/arch/regs-clock.h>

#define cpufreq_dbg(x...) printk(x)

/* note, cpufreq support deals in kHz, no Hz */

static struct cpufreq_driver s3c24xx_driver;
static struct s3c24xx_cpufreq_info *drv_info;
static struct s3c24xx_cpufreq_board *board_info;
static struct s3c24xx_iotimings s3c24xx_iotiming;
static struct s3c24xx_plls s3c24xx_plls;

static struct clk *_clk_mpll;
static struct clk *clk_fclk;
static struct clk *clk_hclk;
static struct clk *clk_pclk;
static struct clk *clk_arm;

static void s3c24xx_cpufreq_getcur(struct s3c24xx_cpufreq_config *cfg)
{
	unsigned long fclk, pclk, hclk, armclk;

	cfg->freq.fclk = fclk = clk_get_rate(clk_fclk);
	cfg->freq.hclk = hclk = clk_get_rate(clk_hclk);
	cfg->freq.pclk = pclk = clk_get_rate(clk_pclk);
	cfg->freq.armclk = armclk = clk_get_rate(clk_arm);

	cfg->pll.pll_reg = __raw_readl(S3C2410_MPLLCON);
	cfg->pll.freq = fclk;

	cfg->divs.arm_divisor = fclk / armclk;
	cfg->divs.h_divisor = fclk / hclk;
	cfg->divs.p_divisor = fclk / pclk;
}

static inline void s3c24xx_cpufreq_calc(struct s3c24xx_cpufreq_config *cfg)
{
	unsigned long pll = cfg->pll.freq;

	cfg->freq.fclk = pll;
	cfg->freq.hclk = pll / cfg->divs.h_divisor;
	cfg->freq.pclk = pll / cfg->divs.p_divisor;

	/* convert hclk into 10ths of nanoseconds for io calcs */
	cfg->freq.hclk_tns = 1000000000 / (cfg->freq.hclk / 10);
}

static inline int closer(unsigned int target, unsigned int n, unsigned int c)
{
	int diff_cur = abs(target - c);
	int diff_new = abs(target - n);

	return (diff_new < diff_cur);
}

/* todo - is checking for an exact match, followed by a best match the
 * most optimal approach, or should we just try and do it in one go?
 */
static int s3c24xx_cpufreq_findpll(struct s3c24xx_pllval *pll,
				   unsigned int target_freq)
{
	struct s3c24xx_plls *plls = &s3c24xx_plls;
	struct s3c24xx_pllval *ptr;
	struct s3c24xx_pllval *got;	
	unsigned int best;
	int count;

	/* first, look for an exact frequency */

	got = NULL;

	for (ptr = plls->vals, count = plls->size; count > 0; count--, ptr++) {
		if (ptr->freq == target_freq) {
			got = ptr;
			goto found;
		}
	}

	/* look for best match */
	best = 0;

	for (ptr = plls->vals, count = plls->size; count > 0; count--, ptr++) {
		if (ptr->freq > target_freq)
			continue;

		if (closer(target_freq, ptr->freq, best)) {
			got = ptr;
			best = got->freq;
		}
	}

	if (got == NULL)
		return -ENOENT;

	printk(KERN_DEBUG "cpufreq: closest to %d is %d\n", target_freq, best);

 found:
	printk(KERN_INFO "%s: found %p (%ld,0x%08lx)\n", __func__, got,
	       got->freq, got->pll_reg);
	*pll = *got;
	return 0;
}


static void s3c24xx_cpufreq_show(const char *pfx, 
				 struct s3c24xx_cpufreq_config *cfg)
{
	s3c_freq_dbg("%s: Fvco=%lu, F=%lu, A=%lu (%u), H=%lu (%u), P=%lu (%u)\n",
	       pfx, cfg->pll.freq, cfg->freq.fclk,
	       cfg->freq.armclk, cfg->divs.arm_divisor,
	       cfg->freq.hclk, cfg->divs.h_divisor,
	       cfg->freq.pclk, cfg->divs.p_divisor);
}

/* functions to wrapper the driver info calls to do the cpu specific work */

static void s3c24xx_cpufreq_setio(struct s3c24xx_cpufreq_config *cfg)
{
	(drv_info->set_iotiming)(cfg, &s3c24xx_iotiming);
}

static int s3c24xx_cpufreq_calcio(struct s3c24xx_cpufreq_config *cfg)
{
	return (drv_info->calc_iotiming)(cfg, &s3c24xx_iotiming);
}

static void s3c24xx_cpufreq_setrefresh(struct s3c24xx_cpufreq_config *cfg)
{
	(drv_info->set_refresh)(cfg);
}

static void s3c24xx_cpufreq_setdivs(struct s3c24xx_cpufreq_config *cfg)
{
	(drv_info->set_divs)(cfg);
}

static int s3c24xx_cpufreq_calcdivs(struct s3c24xx_cpufreq_config *cfg)
{
	return (drv_info->calc_divs)(cfg);
}

static void s3c24xx_cpufreq_setfvco(struct s3c24xx_cpufreq_config *cfg)
{
	(drv_info->set_fvco)(cfg);
}

static inline void s3c24xx_cpufreq_updateclk(struct clk *clk, unsigned int freq)
{
	clk_set_rate(clk, freq);
}       

/* s3c24xx_cpufreq_target
 *
 * called by the cpufreq core to adjust the frequency that the CPU
 * is currently running at. Note, the target_freq is passed in Hz
 * to this call, whereas most other places kHz are required.
 */

static int s3c24xx_cpufreq_target(struct cpufreq_policy *policy,
				  unsigned int target_freq,
				  unsigned int relation)
{
	struct s3c24xx_cpufreq_freqs freqs;
	struct s3c24xx_cpufreq_config cpu_cur, cpu_new;
	unsigned long flags;

	s3c_freq_dbg("%s: policy %p, target %u, relation %u\n", 
	       __func__, policy, target_freq, relation);

	cpu_new.info = cpu_cur.info = drv_info;
	cpu_new.board = cpu_cur.board = board_info;
	
	s3c24xx_cpufreq_getcur(&cpu_cur);
	s3c24xx_cpufreq_show("cur", &cpu_cur);

	/* TODO - check for DMA currently outstanding */

	/* find the settings for our new frequency */

	if (s3c24xx_plls.size == 0) {
		cpu_new.pll = cpu_cur.pll;
	} else {
		if (s3c24xx_cpufreq_findpll(&cpu_new.pll, target_freq) < 0) {
			printk(KERN_ERR "no PLL setting for %d\n", target_freq);
			goto err_notpossible;
		}

		printk("found pll, rate %lu\n", cpu_new.pll.freq);
	}

	/* update our frequencies */

	cpu_new.freq.armclk = target_freq * 1000;
	cpu_new.freq.fclk = cpu_new.pll.freq;

	if (s3c24xx_cpufreq_calcdivs(&cpu_new) < 0) {
		printk(KERN_ERR "no divisors for %d\n", target_freq);
		goto err_notpossible;
	}

	s3c_freq_dbg("%s: got divs\n", __func__);

	s3c24xx_cpufreq_calc(&cpu_new);

	s3c_freq_dbg("%s: calculated frequencies for new\n", __func__);

	if (cpu_new.freq.hclk != cpu_cur.freq.hclk) {
		if (s3c24xx_cpufreq_calcio(&cpu_new) < 0) {
			printk(KERN_ERR "cannot get IO timing for %d\n",
			       target_freq);
			goto err_notpossible;
		}
	}

	s3c24xx_cpufreq_show("new", &cpu_new);

	/* setup our cpufreq parameters */

	memcpy(&freqs.old, &cpu_cur.freq, sizeof(freqs.old));
	memcpy(&freqs.new, &cpu_new.freq, sizeof(freqs.new));

	freqs.freqs.cpu = 0;
	freqs.freqs.old = cpu_cur.freq.armclk / 1000;
	freqs.freqs.new = cpu_new.freq.armclk / 1000;

	/* update f/h/p clock settings before we issue the change
	 * notification, so that drivers do not need to do anything
	 * special if they want to recalculate on CPUFREQ_PRECHANGE. */

	s3c24xx_cpufreq_updateclk(_clk_mpll, cpu_new.pll.freq);
	s3c24xx_cpufreq_updateclk(clk_fclk, cpu_new.freq.fclk);
	s3c24xx_cpufreq_updateclk(clk_hclk, cpu_new.freq.hclk);
	s3c24xx_cpufreq_updateclk(clk_pclk, cpu_new.freq.pclk);

	/* start the frequency change */

	cpufreq_notify_transition(&freqs.freqs, CPUFREQ_PRECHANGE);	

	/* If hclk is staying the same, then we do not need to
	 * re-write the IO or the refresh timings whilst we are changing
	 * speed. */

	local_irq_save(flags);

	/* is our memory clock slowing down? */
	if (cpu_new.freq.hclk < cpu_cur.freq.hclk) {
		s3c24xx_cpufreq_setrefresh(&cpu_new);
		s3c24xx_cpufreq_setio(&cpu_new);
	}

	/* todo - place the divisor settings properly. */
	s3c24xx_cpufreq_setdivs(&cpu_new);

	if (cpu_new.freq.fclk != cpu_cur.freq.fclk)
		s3c24xx_cpufreq_setfvco(&cpu_new);

	/* did our memory clock speed up */
	if (cpu_new.freq.hclk > cpu_cur.freq.hclk) {
		s3c24xx_cpufreq_setrefresh(&cpu_new);
		s3c24xx_cpufreq_setio(&cpu_new);
	}

	local_irq_restore(flags);

	/* notify everyone we've done this */
	cpufreq_notify_transition(&freqs.freqs, CPUFREQ_POSTCHANGE);

	return 0;

 err_notpossible:
	return -EINVAL;
}

static unsigned int s3c24xx_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(clk_arm) / 1000;
}

static inline struct clk *s3c24xx_cpufreq_clk_get(struct device *dev,
						  const char *name)
{
	struct clk *clk;

	clk = clk_get(dev, name);
	if (IS_ERR(clk)) {
		printk("cpufreq: failed to get clock '%s'\n", name);
	}

	return clk;
}

static int s3c24xx_cpufreq_init(struct cpufreq_policy *policy)
{
	cpufreq_dbg("%s: initialising\n", __func__);

	if (policy->cpu != 0)
		return -EINVAL;

	_clk_mpll = s3c24xx_cpufreq_clk_get(NULL, "mpll");
	clk_fclk = s3c24xx_cpufreq_clk_get(NULL, "fclk");
	clk_hclk = s3c24xx_cpufreq_clk_get(NULL, "hclk");
	clk_pclk = s3c24xx_cpufreq_clk_get(NULL, "pclk");
	clk_arm = s3c24xx_cpufreq_clk_get(NULL, "armclk");

	if (IS_ERR(clk_fclk) || IS_ERR(clk_hclk) || IS_ERR(clk_pclk) ||
	    IS_ERR(_clk_mpll) || IS_ERR(clk_arm)) {
		printk(KERN_ERR "could not get clock(s)\n");
		return -ENOENT;
	}

	printk(KERN_INFO "%s: clocks f=%lu,h=%lu,p=%lu,a=%lu\n", __func__,
	       clk_get_rate(clk_fclk) / 1000,
	       clk_get_rate(clk_hclk) / 1000,
	       clk_get_rate(clk_pclk) / 1000,
	       clk_get_rate(clk_arm) / 1000);

	policy->cur = s3c24xx_cpufreq_get(0);
	policy->min = policy->cpuinfo.min_freq = 0;
	policy->max = policy->cpuinfo.max_freq = drv_info->max.fclk;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;
	
	return 0;
}

static int s3c24xx_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return 0;
}


static struct cpufreq_driver s3c24xx_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s3c24xx_cpufreq_verify,
	.target		= s3c24xx_cpufreq_target,
	.get		= s3c24xx_cpufreq_get,
	.init		= s3c24xx_cpufreq_init,
	.name		= "s3c24xx",
};

int __init s3c24xx_cpufreq_register(struct s3c24xx_cpufreq_info *info)
{
	printk("S3C24XX CPU Frequency driver: registering %p\n", info);

	/* check our driver info has valid data */

	BUG_ON(info->set_iotiming == NULL);
	BUG_ON(info->calc_iotiming == NULL);
	BUG_ON(info->set_refresh == NULL);	
	BUG_ON(info->set_divs == NULL);
	BUG_ON(info->calc_divs == NULL);
	//BUG_ON(info->set_fvco == NULL);

	drv_info = info;

	/* Note, driver registering should probably update locktime */

	return 0;
}

int s3c24xx_cpufreq_setboard(struct s3c24xx_cpufreq_board *board)
{
	struct s3c24xx_cpufreq_board *ours;

	if (!board)
		return -EINVAL;

	/* Copy the board information so that each board can make this
	 * initdata. */

	ours = kzalloc(sizeof(struct s3c24xx_cpufreq_board), GFP_KERNEL);
	if (ours == NULL) {
		printk(KERN_ERR "%s: no memory\n", __func__);
		return -ENOMEM;
	}

	memcpy(ours, board, sizeof(struct s3c24xx_cpufreq_board));
	board_info = ours;

	return 0;
}

static int __init s3c24xx_cpufreq_initcall(void)
{
	int ret = 0;

	if (drv_info && board_info)
		ret = cpufreq_register_driver(&s3c24xx_driver);

	return ret;
}

late_initcall(s3c24xx_cpufreq_initcall);

int __init s3c24xx_plls_register(struct s3c24xx_pllval *plls,
				 unsigned int plls_no)
{
	unsigned int size = sizeof(struct s3c24xx_pllval) * plls_no;
	struct s3c24xx_pllval *vals;

	vals = kmalloc(size, GFP_KERNEL);
	if (vals) {
		memcpy(vals, plls, size);
		s3c24xx_plls.vals = vals;
		s3c24xx_plls.size = plls_no;

		printk(KERN_INFO "cpufreq: %d PLL entries\n", plls_no);
	} else
		printk(KERN_ERR "cpufreq: no memory for PLL tables\n");

	return vals ? 0 : -ENOMEM;
}
