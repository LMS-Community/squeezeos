/* include/asm-arm/plat-s3c24xx/cpu-freq.h
 *
 * Copyright (c) 2006,2007 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX CPU frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/cpufreq.h>

struct s3c24xx_freq {
	unsigned long		fclk;
	unsigned long		armclk;
	unsigned long		hclk_tns;	/* in 10ths of ns */
	unsigned long		hclk;
	unsigned long		pclk;
};

/* wrapper 'struct cpufreq_freqs' so that any drivers receiving the
 * notification can use this information that is not provided by just
 * having the core frequency alone.
 *
 */

struct s3c24xx_cpufreq_freqs {
	struct cpufreq_freqs	freqs;
	struct s3c24xx_freq	old;
	struct s3c24xx_freq	new;
};

#define to_s3c_cpufreq(_cf) container_of(_cf, struct s3c24xx_cpufreq_freqs, freqs)

struct s3c24xx_divs {
	int			p_divisor;
	int			h_divisor;
	int			arm_divisor;
};

#define PLLVAL(_m, _p, _s) (((_m) << 12) | ((_p) << 4) | (_s))

struct s3c24xx_pllval {
	unsigned long		freq;
	unsigned long		pll_reg;
};

struct s3c24xx_plls {
	struct s3c24xx_pllval	*vals;
	int			 size;
};

struct s3c24xx_cpufreq_config {
	struct s3c24xx_freq	freq;
	struct s3c24xx_pllval	pll;
	struct s3c24xx_divs	divs;
	struct s3c24xx_cpufreq_info *info;
	struct s3c24xx_cpufreq_board *board;
};

/* We have 6 on the S3C2410, S3C2440, etc. 8 on the S3C2412 */
#define MAX_BANKS (8)
#define S3C2412_MAX_IO	(8)

/* s3c2410_banktiming
*/

struct s3c2410_banktiming {
	unsigned long		bankcon;	/* cache (for setting) */
	unsigned int		tacp;		/* */
	unsigned int		tacs;		/* Address => nCS */
	unsigned int		tcos;		/* nCS => nOE/nWE */
	unsigned int		tacc;		/* nOE/nWE assert */
	unsigned int		tcoh;		/* nCS hold afrer nOE/nWE */
	unsigned int		tcah;		/* Address hold after nCS */
};

struct s3c2410_iotiming {
	struct s3c2410_banktiming	banks[MAX_BANKS];
};

struct s3c24xx_iotimings {
	union {
		struct s3c2410_iotiming	io_2410;
	} timing;
};

struct s3c24xx_cpufreq_info {
	struct s3c24xx_freq	 max;		/* frequency limits */

	unsigned int		locktime_m;	/* lock-time in us */
	unsigned int		locktime_u;	/* lock-time in us */

	/* routines for this driver */

	/* update clocks at resume time */
	int		(*update_clks)(void);

	void		(*get_iotiming)(struct s3c24xx_cpufreq_config *cfg,
					struct s3c24xx_iotimings *timings);

	void		(*set_iotiming)(struct s3c24xx_cpufreq_config *cfg,
					struct s3c24xx_iotimings *timings);

	int		(*calc_iotiming)(struct s3c24xx_cpufreq_config *cfg,
					 struct s3c24xx_iotimings *timings);

	void		(*set_refresh)(struct s3c24xx_cpufreq_config *cfg);
	void		(*set_fvco)(struct s3c24xx_cpufreq_config *cfg);
	void		(*set_divs)(struct s3c24xx_cpufreq_config *cfg);
	int		(*calc_divs)(struct s3c24xx_cpufreq_config *cfg);

};

struct s3c24xx_cpufreq_board {
	unsigned int	refresh;	/* refresh period in ns */

	struct s3c2410_iotiming	*banks[MAX_BANKS];
};

extern int s3c24xx_cpufreq_register(struct s3c24xx_cpufreq_info *info);

extern int s3c24xx_plls_register(struct s3c24xx_pllval *plls,
				 unsigned int plls_no);

/* Board functions */

extern int s3c24xx_cpufreq_setboard(struct s3c24xx_cpufreq_board *board);

/* S3C2410 and compatible exported functions */

extern void s3c2410_cpufreq_setrefresh(struct s3c24xx_cpufreq_config *cfg);

extern int s3c2410_iotiming_calc(struct s3c24xx_cpufreq_config *cfg,
				 struct s3c24xx_iotimings *iot);

extern void s3c2410_iotiming_set(struct s3c24xx_cpufreq_config *cfg,
				 struct s3c24xx_iotimings *iot);

extern void s3c2410_iotiming_get(struct s3c24xx_cpufreq_config *cfg,
				 struct s3c24xx_iotimings *timings);

#define s3c_freq_dbg(x...) do { } while(0)

#define s3c24xx_cpufreq_suspend NULL
#define s3c24xx_cpufreq_resume NULL
