/* linux/include/asm-arm/arch-s3c2410/reset.h
 *
 * Copyright (c) 2007 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 CPU reset controls
*/

#ifndef __ASM_ARCH_RESET_H
#define __ASM_ARCH_RESET_H __FILE__

#include <linux/clk.h>

#include <asm/arch/regs-watchdog.h>

/* This allows the over-ride of the default reset code
*/

extern void (*s3c24xx_reset_hook)(void);

/* cause reset by using the watchdog */

static inline void arch_reset_wdt(void)
{
	struct clk *wdtclk;

	printk("arch_reset: attempting watchdog reset\n");

	__raw_writel(0, S3C2410_WTCON);	  /* disable watchdog, to be safe  */

	/* ensure that the watchdog clock is running before we try and
	 * enable the watchdog itself. */

	wdtclk = clk_get(NULL, "watchdog");
	if (!IS_ERR(wdtclk)) {
		clk_enable(wdtclk);
	} else
		printk(KERN_WARNING "%s: warning: cannot get watchdog clock\n", __func__);

	/* put initial values into count and data */
	__raw_writel(0x80, S3C2410_WTCNT);
	__raw_writel(0x80, S3C2410_WTDAT);

	/* set the watchdog to go and reset... */
	__raw_writel(S3C2410_WTCON_ENABLE|S3C2410_WTCON_DIV16|S3C2410_WTCON_RSTEN |
		     S3C2410_WTCON_PRESCALE(0x20), S3C2410_WTCON);

	/* wait for reset to assert... */
	mdelay(250);

	printk(KERN_ERR "%s: Watchdog failed to reset system\n", __func__);
}


#endif /* __ASM_ARCH_RESET_H */
