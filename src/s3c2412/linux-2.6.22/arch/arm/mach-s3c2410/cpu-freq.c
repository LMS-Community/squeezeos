/* linux/arch/arm/mach-s3c2410/cpu-freq.c
 *
 * Copyright (c) 2006,2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 CPU Frequency scalling
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

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/regs-clock.h>

#include <asm/plat-s3c24xx/cpu.h>
#include <asm/plat-s3c24xx/cpu-freq.h>
#include <asm/plat-s3c24xx/clock.h>

int s3c2410_cpufreq_setdivs(struct s3c24xx_cpufreq_config *cfg)
{
	return -EINVAL;
}

int s3c2410_cpufreq_calcdivs(struct s3c24xx_cpufreq_config *cfg)
{
	return -EINVAL;
}

int s3c2410_cpufreq_setrefresh(struct s3c24xx_cpufreq_config *cfg)
{
	return -EINVAL;
}

struct s3c24xx_cpufreq_info s3c2410_cpufreq_info = {
	.max		= {
		.fclk	= 266000000,
		.hclk	= 133000000,
		.pclk	=  66000000,
	},

	.set_iotiming	= s3c2410_iotiming_set,
	.get_iotiming	= s3c2410_iotiming_get,

	.set_refresh	= s3c2410_cpufreq_setrefresh,
	.set_divs	= s3c2410_cpufreq_setdivs,
	.calc_divs	= s3c2410_cpufreq_calcdivs,
};

static int s3c2410_cpufreq_add(struct sys_device *sysdev)
{
	return s3c24xx_cpufreq_register(&s3c2410_cpufreq_info);
}

static struct sysdev_driver s3c2410_cpufreq_driver = {
	.add		= s3c2410_cpufreq_add,
	.suspend	= s3c24xx_cpufreq_suspend,
	.resume		= s3c24xx_cpufreq_resume,
};

static int s3c2410_cpufreq_init(void)
{
	return sysdev_driver_register(&s3c2410_sysclass,
				      &s3c2410_cpufreq_driver);
}

arch_initcall(s3c2410_cpufreq_init);
