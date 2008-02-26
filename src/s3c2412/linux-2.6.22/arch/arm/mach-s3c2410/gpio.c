/* linux/arch/arm/mach-s3c2410/gpio.c
 *
 * Copyright (c) 2004-2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 GPIO support
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/arch/regs-gpio.h>

int s3c2410_gpio_irqfilter(unsigned int pin, unsigned int on,
			   unsigned int config)
{
	void __iomem *reg_filter;
	void __iomem *reg_extint;
	unsigned long flags;
	unsigned long val;

	/* note, not all of these pins are available on the
	 * s3c2410 or the s3c2440 */

	if (pin >= S3C2410_GPG8 && pin <= S3C2410_GPG12) {
		pin -= (S3C2410_GPG8 - 16);
	} else if (pin >= S3C2410_GPF0 && pin <= S3C2410_GPF7) {
		pin -= (S3C2410_GPF0 - 0);
	} else if (pin >= S3C2410_GPG0 && pin <= S3C2410_GPG7) {
		pin -= (S3C2410_GPG0 - 8);
	} else
		return -EINVAL;

	config &= 0xff;

	reg_filter = S3C24XX_EINFLT0 + ((pin >> 2) * 4);
	reg_extint = S3C24XX_EXTINT0 + ((pin >> 3) * 4);

	pin &= 7;

	local_irq_save(flags);

	/* update filter width and clock source */

	val = __raw_readl(reg_filter);
	val &= ~(0xff << ((pin & 3) * 8));
	val |= config << ((pin & 3) * 8);
	__raw_writel(val, reg_filter);

	/* update filter enable */

	val = __raw_readl(reg_extint);
	val &= ~(1 << ((pin * 4) + 3));
	val |= on << ((pin * 4) + 3);
	__raw_writel(val, reg_extint);

	local_irq_restore(flags);

	return 0;
}

EXPORT_SYMBOL(s3c2410_gpio_irqfilter);
