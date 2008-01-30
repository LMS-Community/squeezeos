/* linux/arch/arm/mach-s3c2410/iotiming.c
 *
 * Copyright (c) 2006,2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX CPU Frequency scalling - IO timing support
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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/cpufreq.h>

#include <asm/io.h>

#include <asm/arch/map.h>
#include <asm/arch/regs-mem.h>

#include <asm/plat-s3c24xx/cpu-freq.h>

#define print_ns(x) ((x) / 10), ((x) % 10)

static void s3c2410_print_timing(const char *pfx,
				 struct s3c24xx_iotimings *timings)
{
	struct s3c2410_banktiming *bt = timings->timing.io_2410.banks;
	int bank;

	for (bank = 0; bank < MAX_BANKS; bank++, bt++) {

		printk("%s Tacs=%d.%d, Tcos=%d.%d, Tacc=%d.%d,"
		       "Tcoh=%d.d, Tcah=%d.%d\n", pfx,
		       print_ns(bt->tacs),
		       print_ns(bt->tcos),
		       print_ns(bt->tacc),
		       print_ns(bt->tcoh),
		       print_ns(bt->tcah));
	}
}


static inline void __iomem *bank_reg(unsigned int bank)
{
	return S3C2410_BANKCON0 + (bank << 2);
}

static inline int bank_is_io(unsigned long bankcon)
{
	return !(bankcon & S3C2410_BANKCON_SDRAM);
}

static inline unsigned int to_div(unsigned int cyc, unsigned int hclk_tns)
{
	return cyc / hclk_tns; 
}

static unsigned int calc_0124(int cyc, unsigned long hclk_tns, 
			     unsigned long *v, int shift)
{
	int div = to_div(cyc, hclk_tns);
	unsigned long val;
	
	switch (div) {
	case 0:
		val = 0;
		break;
	case 1:
		val = 1;
		break;
	case 2:
		val = 2;
	case 3:
	case 4:
		val = 3;
	default:
		return -1;
	}

	*v |= val << shift;
	return 0;
}

int calc_tacp(int cyc, unsigned long hclk, unsigned long *v)
{
	return 0;
}

int calc_tacc(int cyc, unsigned long hclk_tns, unsigned long *v)
{
	int div = to_div(cyc, hclk_tns);
	unsigned long val;

	switch (div) {
	case 0:
		val = 0;
		break;

	case 1:
	case 2:
	case 3:
	case 4:
		val = div - 1;
		break;

	case 5:
	case 6:
		val = 4;
		break;

	case 7:
	case 8:
		val = 5;
		break;

	case 9:
	case 10:
		val = 6;
		break;

	case 11:
	case 12:
	case 13:
	case 14:
		val = 7;
		break;

	default:
		return -1;
	}

	*v |= val << 8;
	return 0;
}

static int s3c2410_calc_bank(struct s3c24xx_cpufreq_config *cfg,
			     struct s3c2410_banktiming *bt)
{
	unsigned long hclk = cfg->freq.hclk_tns;
	unsigned long res;
	int ret;

	res  = bt->bankcon;
	res &= ~(S3C2410_BANKCON_SDRAM|S3C2410_BANKCON_PMC16);

	/* tacp: 2,3,4,5 */
	/* tcah: 0,1,2,4 */
	/* tcoh: 0,1,2,4 */
	/* tacc: 1,2,3,4,6,7,10,14 (>4 for nwait) */
	/* tcos: 0,1,2,4 */
	/* tacs: 0,1,2,4 */
	
	ret  = calc_0124(bt->tcah, hclk, &res, S3C2410_BANKCON_Tcah_SHIFT);
	ret |= calc_0124(bt->tcoh, hclk, &res, S3C2410_BANKCON_Tcoh_SHIFT);
	ret |= calc_0124(bt->tcos, hclk, &res, S3C2410_BANKCON_Tcos_SHIFT);
	ret |= calc_0124(bt->tacs, hclk, &res, S3C2410_BANKCON_Tacs_SHIFT);

	ret |= calc_tacp(bt->tacp, hclk, &res);
	ret |= calc_tacc(bt->tacc, hclk, &res);

	if (ret)
		return -EINVAL;

	bt->bankcon = res;
	return 0;
}

static unsigned int tacc_tab[] = {
	[0]	= 1,
	[1]	= 2,
	[2]	= 3,
	[3]	= 4,
	[4]	= 6,
	[5]	= 9,
	[6]	= 10,
	[7]	= 14,
};

static unsigned int get_tacc(unsigned long hclk_tns,
			     unsigned long val)
{
	val &= 7;
	return hclk_tns * tacc_tab[val];
}

static unsigned int get_0124(unsigned long hclk_tns,
			     unsigned long val)
{
	val &= 3;
	return hclk_tns * ((val == 3) ? 4 : val);
}

void s3c2410_iotiming_getbank(struct s3c24xx_cpufreq_config *cfg,
			     struct s3c2410_banktiming *bt)
{
	unsigned long bankcon = bt->bankcon;
	unsigned long hclk = cfg->freq.hclk_tns;

	bt->tcah = get_0124(hclk, bankcon >> S3C2410_BANKCON_Tcah_SHIFT);
	bt->tcoh = get_0124(hclk, bankcon >> S3C2410_BANKCON_Tcoh_SHIFT);
	bt->tcos = get_0124(hclk, bankcon >> S3C2410_BANKCON_Tcos_SHIFT);
	bt->tacs = get_0124(hclk, bankcon >> S3C2410_BANKCON_Tacs_SHIFT);
	bt->tacc = get_tacc(hclk, bankcon >> S3C2410_BANKCON_Tacc_SHIFT);
}

int s3c2410_iotiming_calc(struct s3c24xx_cpufreq_config *cfg,
			  struct s3c24xx_iotimings *iot)
{
	struct s3c2410_iotiming *ot = &iot->timing.io_2410;
	unsigned long bankcon;
	int bank;
	int ret;
	
	for (bank = 0; bank < MAX_BANKS; bank++) {
		bankcon = __raw_readl(bank_reg(bank));
		ot->banks[bank].bankcon = bankcon;
		
		if (!bank_is_io(bankcon))
			continue;
		
		ret = s3c2410_calc_bank(cfg, &ot->banks[bank]);
		if (ret)
			goto err;
	}
	
	return 0;
 err:
	return ret;
}

void s3c2410_iotiming_set(struct s3c24xx_cpufreq_config *cfg,
			  struct s3c24xx_iotimings *iot)
{
	struct s3c2410_iotiming *ot = &iot->timing.io_2410;
	unsigned long bankcon;
	int bank;
	
	/* set the io timings from the specifier */
	
	for (bank = 0; bank < MAX_BANKS; bank++) {
		bankcon = ot->banks[bank].bankcon;
		
		if (!bank_is_io(bankcon))
			continue;
		
		__raw_writel(bankcon, bank_reg(bank));
	}
}

void s3c2410_iotiming_get(struct s3c24xx_cpufreq_config *cfg,
			  struct s3c24xx_iotimings *timings)
{
	struct s3c2410_iotiming *ot = &timings->timing.io_2410;
	unsigned long bankcon;
	int bank;
	
	/* set the io timings from the specifier */
	
	for (bank = 0; bank < MAX_BANKS; bank++) {
		bankcon = __raw_readl(bank_reg(bank));
		ot->banks[bank].bankcon = bankcon;
		
		if (!bank_is_io(bankcon))
			continue;

		s3c2410_iotiming_getbank(cfg, &ot->banks[bank]);
	}
}
