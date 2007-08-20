/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#ifndef __S3C2413FB_H__
#define __S3C2413FB_H__
#include <linux/fb.h>
#include <linux/pm.h>
                                                                                          
void LCD_on_seq(void);
int elfin_pm_callback(struct pm_dev *pm_dev, pm_request_t req, void *data);

struct elfinfb_rgb {
    struct fb_bitfield		red;
    struct fb_bitfield		green;
    struct fb_bitfield		blue;
    struct fb_bitfield		transp;
};

struct elfinfb_lcd_reg {
    unsigned long lcdcon1;
    unsigned long lcdcon2;
    unsigned long lcdcon3;
    unsigned long lcdcon4;
    unsigned long lcdcon5;
    unsigned long lcdcon6;
    unsigned long lcdcon7;
    unsigned long lcdcon8;
    unsigned long lcdsaddr1;
    unsigned long lcdsaddr2;
    unsigned long lcdsaddr3;
    unsigned long redlut;
    unsigned long greenlut;
    unsigned long bluelut;
    unsigned long dithmode;
    unsigned long tpal;
    unsigned long lcdintpnd;
    unsigned long lcdsrcpnd;
    unsigned long lcdintmsk;
    unsigned long lcdlpcsel;
    unsigned long tconsel;
	
};

struct elfinfb_mach_info {
    u_long		pixclock;

    u_short		xres;
    u_short		yres;

    u_char		bpp;
    u_char		hsync_len;
    u_char		left_margin;
    u_char		right_margin;

    u_char		vsync_len;
    u_char		upper_margin;
    u_char		lower_margin;
    u_char		sync;

    u_int		cmap_grayscale:1,
    			cmap_inverse:1,
			cmap_static:1,
		        unused:29;
    u_int		state;

    struct elfinfb_lcd_reg reg;
};

#define RGB_8	(0)
#define RGB_16	(1)
#define RGB_24  (2)

#define NR_RGB	3

struct elfinfb_info {
    struct fb_info		fb;
    struct device		*dev;
    signed int			currcon;

    struct elfinfb_rgb	*rgb[NR_RGB];

    u_int			max_bpp;
    u_int			max_xres;
    u_int			max_yres;

   /*
    * These are the addresses we mapped
    * the framebuffer memory region to.
    */
	/* raw memory addresses */
    dma_addr_t			map_dma;
    u_char *			map_cpu;
    u_int			map_size;

	/* addresses of pieces placed in raw buffer */
    u_char *			screen_cpu;/* virtual address of frame buffer */
    dma_addr_t			screen_dma;/* physical address of frame buffer */
    u16 *			palette_cpu;/* virtual address of palette memory */
    dma_addr_t			palette_dma;/* physical address of palette memory */
    u_int			palette_size;

    u_int			cmap_inverse:1,
    				cmap_static:1,
				unused:30;
	struct elfinfb_lcd_reg reg;
#ifdef CONFIG_PM
	struct pm_dev	*pm;
#endif
};

#define S3C2413_NAME		"S3C2413"

#define MIN_XRES	64
#define MIN_YRES	64

#endif
