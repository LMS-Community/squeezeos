/* linux/include/asm/arch-s3c2410/s3c24xxfb.h
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 *
 * Inspired by pxafb.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *  Changelog:
 *	07-Sep-2004	RTP	Created file
 *	03-Nov-2004	BJD	Updated and minor cleanups      
*/

#ifndef __ASM_ARM_S3C24XXFB_H
#define __ASM_ARM_S3C24XXFB_H

#include <asm/arch/regs-lcd.h>

struct s3c24xxfb_mach_info {
	
	/* Screen size */
	int		width;
	int		height;

	/* Screen info */
	int		xres;
	int		yres;
	int		bpp;

	unsigned long	pixclock;

	int hsync_len;
	int left_margin;
	int right_margin;
	int vsync_len;
	int upper_margin;
	int lower_margin;
	int sync;
	int cmap_grayscale:1, cmap_inverse:1, cmap_static:1, unused:29;
	
	/* lcd configuration registers */
	unsigned long	lcdcon1;
	unsigned long	lcdcon2;
	unsigned long	lcdcon3;
	unsigned long	lcdcon4;
	unsigned long	lcdcon5;

	/* GPIOs */
	unsigned long	gpcup;
	unsigned long	gpcup_mask;
	unsigned long	gpccon;
	unsigned long	gpccon_mask;
	unsigned long	gpdup;
	unsigned long	gpdup_mask;
	unsigned long	gpdcon;
	unsigned long	gpdcon_mask;
	
	/* lpc3600 control register */
	unsigned long	lpcsel;

	/* backlight info */
	int		backlight_min;
	int		backlight_max;
	int		backlight_default;
	
	/* Utility fonctions */
	void		(*backlight_power)(int);
	void		(*lcd_power)(int);
	void		(*set_brightness)(int);
};

#endif /* __ASM_ARM_S3C24XXFB_H */
