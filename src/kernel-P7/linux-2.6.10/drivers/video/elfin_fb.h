#ifndef __S3C24XXFB_H
#define __S3C24XXFB_H
/*
 * linux/drivers/video/s3c24xxfb.h
 *
 * $Id: elfin_fb.h,v 1.1 2006/04/07 01:16:43 ihlee215 Exp $
 *
 * Copyright (c)2005 rahul tanwar <rahul.tanwar@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C24XX LCD Controller Frame Buffer Driver
 *	    based on skeletonfb.c, sa1100fb.h
 *
 */

#define MIN_XRES	64
#define MIN_YRES	64

struct elfinfb_rgb {
    struct fb_bitfield		red;
    struct fb_bitfield		green;
    struct fb_bitfield		blue;
    struct fb_bitfield		transp;
};

static struct elfinfb_rgb rgb_8 = {
      .red = {.offset = 0, .length = 4,},
      .green = {.offset = 0, .length = 4,},
      .blue = {.offset = 0, .length = 4,},
      .transp = {.offset = 0, .length = 0,},
};

static struct elfinfb_rgb xxx_tft_rgb_16 = {
      .red = {.offset = 11, .length = 5,},
      .green = {.offset = 5, .length = 6,},
      .blue = {.offset = 0, .length = 5,},
      .transp = {.offset = 0, .length = 0,},
};

static struct elfinfb_rgb xxx_tft_rgb_24 = {
      .red = {.offset = 16, .length = 8,},
      .green = {.offset = 8, .length = 8,},
      .blue = {.offset = 0, .length = 8,},
      .transp = {.offset = 0, .length = 0,},
};
static struct elfinfb_rgb xxx_tft_rgb_32 = {
      .red={.offset=16, .length=8,},
      .green={.offset=8, .length=8,},
      .blue={.offset=0, .length=8,},
      .transp={.offset= 0, .length=8,},   
};

struct s3c24xxfb_info {
	struct fb_info		fb;
	struct device		*dev;

       u_int			max_bpp;
       u_int			max_xres;
       u_int			max_yres;
	   
	/* raw memory addresses */
	dma_addr_t		map_dma_f1;	/* physical */
	u_char *		map_cpu_f1;	/* virtual */
	u_int			map_size_f1;

	/* addresses of pieces placed in raw buffer */
	u_char *		screen_cpu_f1;	/* virtual address of frame buffer */
	dma_addr_t		screen_dma_f1;	/* physical address of frame buffer */

	/* raw memory addresses */
	dma_addr_t		map_dma_f2;	/* physical */
	u_char *		map_cpu_f2;	/* virtual */
	u_int			map_size_f2;

	/* addresses of pieces placed in raw buffer */
	u_char *		screen_cpu_f2;	/* virtual address of frame buffer */
	dma_addr_t		screen_dma_f2;	/* physical address of frame buffer */

	u32 pseudo_pal[16];

	/* vertical sync interrupt */
	unsigned int		vsync_cnt;
	wait_queue_head_t	vsync_wait;
	unsigned long		irq_flags;
};

int s3c24xxfb_init(void);

#if 1
#define dprintk(msg...)	printk("s3c24xxfb: " msg)
#else
#define dprintk(msg...) while (0) { }
#endif


#endif
