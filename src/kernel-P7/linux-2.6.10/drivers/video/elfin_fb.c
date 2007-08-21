/*
 * linux/drivers/video/s3c24xxfb.c
 *
 * $Id: elfin_fb.c,v 1.1 2006/04/07 01:16:43 ihlee215 Exp $
 * 
 * Copyright (c) 2005 rahul tanwar <rahul.tanwar@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C24XX LCD Controller Frame Buffer Driver
 *	    based on skeletonfb.c, sa1100fb.c
 *
 * ChangeLog:
 * 
 * 2005-02-24: Sean Choi <sh428.choi@samsung.com>
 *	- add LCD Backlight enable for SMDK2460 board
 *
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>

#include <asm/arch/s3c24xxfb.h> 
#include <asm/hardware/clock.h>

#include "elfin_fb.h"
#include "asm/arch/regs-clock.h" //panna


#define DEFAULT_BACKLIGHT_LEVEL		2

extern struct s3c24xxfb_mach_info mach_info; 

extern void set_brightness(int);
extern int s3c24xxfb_ioctl(struct inode *inode, struct file *file,
			   u_int cmd, u_long arg,
			   struct fb_info *info);

extern int __init s3c24xxfb_map_video_memory(struct s3c24xxfb_info *fbi);
extern void Init_LDI(void);
extern int s3c24xxfb_init_registers(struct s3c24xxfb_info *fbi);


struct s3c24xxfb_info info;

/* backlight and power control functions */

static int backlight_level = DEFAULT_BACKLIGHT_LEVEL;
static int backlight_power = 1;
static int lcd_power	   = 1;

static  void s3c24xxfb_lcd_power(int to)
{
	lcd_power = to;

	if (mach_info.lcd_power)
		(mach_info.lcd_power)(to);
}

static inline void s3c24xxfb_backlight_power(int to)
{	
	backlight_power = to;
	
	if (mach_info.backlight_power)
		(mach_info.backlight_power)(to);
}

 void s3c24xxfb_backlight_level(int to)
{
	backlight_level = to;

	if (mach_info.set_brightness)
		(mach_info.set_brightness)(to);
}

/*
 *	s3c24xxfb_check_var():
 *	Get the video params out of 'var'. If a value doesn't fit, round it up,
 *	if it's too big, return -EINVAL.
 *
 */
static int s3c24xxfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	//dprintk("check_var(var=%p, info=%p)\n", var, info);

	switch (var->bits_per_pixel) {
		case 8:
			var->red	= rgb_8.red;
	              var->green	=  rgb_8.green;
	              var->blue	=  rgb_8.blue;
	              var->transp  =  rgb_8.transp;
			break;
			
		case 16:
			var->red	= xxx_tft_rgb_16.red;
	              var->green	=  xxx_tft_rgb_16.green;
	              var->blue	=  xxx_tft_rgb_16.blue;
	              var->transp  =  xxx_tft_rgb_16.transp;
			break;
			
		case 24:
			var->red	= xxx_tft_rgb_24.red;
	              var->green	=  xxx_tft_rgb_24.green;
	              var->blue	=  xxx_tft_rgb_24.blue;
	              var->transp  =  xxx_tft_rgb_24.transp;
			break;
			
		case 32:
			var->red	= xxx_tft_rgb_32.red;
	              var->green   =  xxx_tft_rgb_32.green;
	              var->blue     =  xxx_tft_rgb_32.blue;
	              var->transp  =  xxx_tft_rgb_32.transp;  
			break;
	}
		
	return 0;
}

/*
 *      s3c24xxfb_set_par - Optional function. Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 */
static int s3c24xxfb_set_par(struct fb_info *info)
{
	struct s3c24xxfb_info *fbi = (struct s3c24xxfb_info *)info;
	struct fb_var_screeninfo *var = &info->var;

	/* We support only 16BPP true color */
	fbi->fb.fix.visual	    = FB_VISUAL_TRUECOLOR;
	fbi->fb.fix.line_length     = (var->width*var->bits_per_pixel)/8;
	return 0;
}


static int s3c24xxfb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	struct s3c24xxfb_info *fbi = (struct s3c24xxfb_info *)info;
	int bpp, m = 0;

	bpp = fbi->fb.var.bits_per_pixel;
	m = 1 << bpp;

	if (regno >= m) {
		return -EINVAL;
	}

	switch (bpp) {
	case 16:
		/* RGB 565 */
		fbi->pseudo_pal[regno] = ((red & 0xF800)
			| ((green & 0xFC00) >> 5)
			| ((blue & 0xF800) >> 11));
		break;
	}

	return 0;
}


#define S3C2413_LCDINTPND  S3C2413_LCDREG(0x24)
#define S3C2413_LCDSRCPND  S3C2413_LCDREG(0x28)
#define S3C2413_LCDINTMSK  S3C2413_LCDREG(0x2C)

#define S3C2413_LCDINT_FICNT	1<<0
#define S3C2413_LCDINT_FRSYNC	1<<1

static irqreturn_t s3c24xxfb_irq(int irq, void *dev_id, struct pt_regs *fp)
{
	unsigned long lcdirq = __raw_readl(S3C2413_LCDINTPND);

	if (lcdirq & S3C2413_LCDINT_FRSYNC) {
		/* Clear interrupts */
		__raw_writel(S3C2413_LCDINT_FRSYNC, S3C2413_LCDSRCPND);
		__raw_writel(S3C2413_LCDINT_FRSYNC, S3C2413_LCDINTPND);

		info.vsync_cnt++;
		wake_up_interruptible(&info.vsync_wait);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int s3c24xxfb_enable_irq(void)
{
	if (!test_and_set_bit(0, &(info.irq_flags))) {

		if (request_irq(IRQ_LCD, s3c24xxfb_irq, SA_INTERRUPT, "s3c24xxfb", NULL)) {
			printk(KERN_ERR "elfin_fb.c: Could not allocate IRQ_LCD %d\n", IRQ_LCD);

			clear_bit(0, &(info.irq_flags));
			return -EINVAL;
		}
	}

	/* Enable frame sync IRQ */
	__raw_writel(0xFD, S3C2413_LCDINTMSK);
	__raw_writel(S3C2413_LCDINT_FRSYNC, S3C2413_LCDSRCPND);
	__raw_writel(S3C2413_LCDINT_FRSYNC, S3C2413_LCDINTPND);

	return 0;
}

static int s3c24xxfb_disable_irq(void)
{
	if (test_and_clear_bit(0, &(info.irq_flags))) {
		free_irq(IRQ_LCD, NULL);
	}

	return 0;
}

/**
 *	s3c24xxfb_pan_display
 *	@var: frame buffer variable screen structure
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Pan (or wrap, depending on the `vmode' field) the display using the
 *	`xoffset' and `yoffset' fields of the `var' structure.
 *	If the values don't fit, return -EINVAL.
 *
 *	Returns negative errno on error, or zero on success.
 */
static int s3c24xxfb_pan_display(struct fb_var_screeninfo *var,
			     struct fb_info *fbinfo)
{
	unsigned long lcdsaddr1, lcdsaddr2, val, flags;
	unsigned long VideoPhysicalTemp = info.screen_dma_f1;
	unsigned int cnt;
	wait_queue_t __wait;
	int ret;

	/* Panning has been implemented for SDL double buffering,
	 * so we only use the yoffset.
	 */

	VideoPhysicalTemp += (var->xres * var->yoffset * var->bits_per_pixel) / 8;

	lcdsaddr1 =
	    LCDADDR_BANK(((unsigned long) VideoPhysicalTemp >> 22))
	    | LCDADDR_BASEU(((unsigned long) VideoPhysicalTemp >> 1));

	lcdsaddr2 = LCDADDR_BASEL(((unsigned long)
					    VideoPhysicalTemp +
					    (var->xres * var->yres * var->bits_per_pixel) / 8) >> 1);


	/* Users must not change the value of the LCDBASEU and LCDBASEL
	 * registers at the end of FRAME by refering to the LINECNT field
	 * in LCDCON1 register, for the LCD FIFO fetches the next frame
	 * data prior to the change in frame. To check the LINECNT,
	 * interrupts should be masked.
	 */
	local_irq_save(flags);

	do {
		val = __raw_readl(S3C2413_LCDCON1) >> 18;
	} while (val == 0);

	__raw_writel(lcdsaddr1, S3C2413_LCDSADDR1);
	__raw_writel(lcdsaddr2, S3C2413_LCDSADDR2);

	local_irq_restore(flags);


	/* Wait until the start of the next frame */
	ret = s3c24xxfb_enable_irq();
	if (ret) {
		return ret;
	}
        init_waitqueue_entry(&__wait, current);

	cnt = info.vsync_cnt;
	ret = wait_event_interruptible_timeout(info.vsync_wait, cnt != info.vsync_cnt, HZ/10);
	if (ret < 0) {
		return ret;
	}
	if (ret == 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 *      s3c24xxfb_blank
 *	@blank_mode: the blank mode we want.
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Blank the screen if blank_mode != 0, else unblank. Return 0 if
 *	blanking succeeded, != 0 if un-/blanking failed due to e.g. a
 *	video mode which doesn't support it. Implements VESA suspend
 *	and powerdown modes on hardware that supports disabling hsync/vsync:
 *	blank_mode == 2: suspend vsync
 *	blank_mode == 3: suspend hsync
 *	blank_mode == 4: powerdown
 *
 *	Returns negative errno on error, or zero on success.
 *
 */
static int s3c24xxfb_blank(int blank_mode, struct fb_info *info)
{
	dprintk("blank(mode=%d, info=%p)\n", blank_mode, info);
	
	switch (blank_mode) {
	case VESA_NO_BLANKING:	/* lcd on, backlight on */
		s3c24xxfb_lcd_power(1);
		s3c24xxfb_backlight_power(1);
		break;

	case VESA_VSYNC_SUSPEND: /* lcd on, backlight off */
	case VESA_HSYNC_SUSPEND:
		s3c24xxfb_lcd_power(1);
		s3c24xxfb_backlight_power(0);
		break;

	case VESA_POWERDOWN: /* lcd and backlight off */
		s3c24xxfb_lcd_power(0);
		s3c24xxfb_backlight_power(0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


/* sysfs export of baclight control */
static int s3c24xxfb_lcd_power_show(struct device *dev, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", lcd_power);
}
static int s3c24xxfb_lcd_power_store(struct device *dev,
					   const char *buf, size_t len)
{

	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 ||
	    strnicmp(buf, "1", 1) == 0) {
		s3c24xxfb_lcd_power(1);
	} else if (strnicmp(buf, "off", 3) == 0 ||
		   strnicmp(buf, "0", 1) == 0) {
		s3c24xxfb_lcd_power(0);
	} else {
		return -EINVAL;
	}

	return len;
}

static int s3c24xxfb_backlight_level_show(struct device *dev, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", backlight_level);
}

static int s3c24xxfb_backlight_power_show(struct device *dev, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", backlight_power);
}

static int s3c24xxfb_backlight_level_store(struct device *dev,
					   const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if (value < mach_info.backlight_min ||
	    value > mach_info.backlight_max)
		return -ERANGE;

	s3c24xxfb_backlight_level(value);
	return len;
}

static int s3c24xxfb_backlight_power_store(struct device *dev,
					   const char *buf, size_t len)
{
	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 ||
	    strnicmp(buf, "1", 1) == 0) {
		s3c24xxfb_backlight_power(1);
	} else if (strnicmp(buf, "off", 3) == 0 ||
		   strnicmp(buf, "0", 1) == 0) {
		s3c24xxfb_backlight_power(0);
	} else {
		return -EINVAL;
	}

	return len;
}

static DEVICE_ATTR(lcd_power, 0644,
		   s3c24xxfb_lcd_power_show,
		   s3c24xxfb_lcd_power_store);

static DEVICE_ATTR(backlight_level, 0644,
		   s3c24xxfb_backlight_level_show,
		   s3c24xxfb_backlight_level_store);

static DEVICE_ATTR(backlight_power, 0644,
		   s3c24xxfb_backlight_power_show,
		   s3c24xxfb_backlight_power_store);



struct fb_ops s3c24xxfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= s3c24xxfb_check_var,
	.fb_set_par	= s3c24xxfb_set_par,	
	.fb_blank	= s3c24xxfb_blank,
	.fb_pan_display	= s3c24xxfb_pan_display,	
	.fb_setcolreg	= s3c24xxfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,	
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= soft_cursor,	
	.fb_ioctl	= s3c24xxfb_ioctl,
};

/* Fake monspecs to fill in fbinfo structure */
/* Don't know if the values are important    */
struct fb_monspecs monspecs __initdata = {
	.hfmin	= 30000,
	.hfmax	= 70000,
	.vfmin	= 50,
	.vfmax	= 65,
};

static struct clk      *lcd_clock;

void s3c24xxfb_init_fbinfo(char * drv_name) {

       Init_LDI();
	strcpy(info.fb.fix.id, drv_name);
	
	info.fb.fix.type	    = FB_TYPE_PACKED_PIXELS;
	info.fb.fix.type_aux	    = 0;
	info.fb.fix.xpanstep	    = 0;
	info.fb.fix.ypanstep	    = 0;
	info.fb.fix.ywrapstep	    = 0;
	info.fb.fix.accel	    = FB_ACCEL_NONE;

	info.fb.var.nonstd	    = 0;
	info.fb.var.activate	    = FB_ACTIVATE_NOW;
	info.fb.var.height	    = mach_info.height;
	info.fb.var.width	    = mach_info.width;
	info.fb.var.accel_flags     = 0;
	info.fb.var.vmode	    = FB_VMODE_NONINTERLACED;

	info.fb.fbops		    = &s3c24xxfb_ops;
	info.fb.flags		    = FBINFO_FLAG_DEFAULT;
	info.fb.monspecs	    = monspecs;
	info.fb.pseudo_palette      = &info.pseudo_pal;


	info.fb.var.xres	    = mach_info.xres;
	info.fb.var.xres_virtual    = mach_info.xres;
	info.fb.var.yres	    = mach_info.yres;
	info.fb.var.yres_virtual    = mach_info.yres;

	info.fb.var.bits_per_pixel  = mach_info.bpp;

       info.fb.var.pixclock = mach_info.pixclock;
	info.fb.var.hsync_len = mach_info.hsync_len;
	info.fb.var.left_margin = mach_info.left_margin;
	info.fb.var.right_margin = mach_info.right_margin;
	info.fb.var.vsync_len = mach_info.vsync_len;
	info.fb.var.upper_margin = mach_info.upper_margin;
	info.fb.var.lower_margin = mach_info.lower_margin;
	info.fb.var.sync = mach_info.sync;
	info.fb.var.grayscale = mach_info.cmap_grayscale;

	info.fb.fix.smem_len	    = info.fb.var.xres * info.fb.var.yres *
				      info.fb.var.bits_per_pixel / 8;

	/* allocate room for SDL double buffer */
	info.fb.fix.smem_len *= 2;
}

int __init s3c24xxfb_probe(struct device *dev)
{
	char driver_name[]="s3c24xxfb";
	int ret;
	 
       s3c24xxfb_init_fbinfo(driver_name);
	
	s3c24xxfb_backlight_power(1);
	s3c24xxfb_lcd_power(1);

	s3c24xxfb_backlight_level(DEFAULT_BACKLIGHT_LEVEL);

	dprintk("dev FB init\n");

	if (!request_mem_region(S3C24XX_VA_LCD, SZ_1M, "s3c24xx-lcd"))
		return -EBUSY;
	
	dprintk("got LCD region\n");

	init_waitqueue_head(&info.vsync_wait);

	lcd_clock = clk_get(NULL, "lcd");
	if (!lcd_clock) {
		printk(KERN_INFO "failed to get lcd clock source\n");
		return -ENOENT;
	}
	clk_use(lcd_clock);
	clk_enable(lcd_clock);
	dprintk("got and enabled clock\n");

	/* maybe not required */
	msleep(5);

	/* Initialize video memory */
	ret = s3c24xxfb_map_video_memory(&info);
	if (ret) {
		printk("Failed to allocate video RAM: %d\n", ret);
		ret = -ENOMEM;
		goto failed;
	}
	dprintk("got video memory\n");

	ret = s3c24xxfb_init_registers(&info);
        s3c24xxfb_lcd_power(1);

	ret = s3c24xxfb_check_var(&info.fb.var, &info.fb);

	ret = register_framebuffer(&info.fb);
	if (ret < 0) {
		printk("Failed to register framebuffer device: %d\n", ret);
		goto failed;
	}

	/* create device files */

	device_create_file(dev, &dev_attr_backlight_power);
	device_create_file(dev, &dev_attr_backlight_level);
	device_create_file(dev, &dev_attr_lcd_power);

	printk(KERN_INFO "fb%d: %s frame buffer device\n",
		info.fb.node, info.fb.fix.id);

	return 0;
failed:
	release_mem_region(S3C24XX_VA_LCD, S3C24XX_SZ_LCD);
	return ret;
}

/* s3c24xxfb_stop_lcd
 *
 * shutdown the lcd controller 
*/

static void s3c24xxfb_stop_lcd(void)
{
	unsigned long flags;
	unsigned long tmp;

	s3c24xxfb_disable_irq();

	local_irq_save(flags);
	
	tmp = __raw_readl(S3C24XX_LCDCON1);
	__raw_writel(tmp & ~ENVID, S3C24XX_LCDCON1);
	
	local_irq_restore(flags);
}

/*
 *  Cleanup
 */
static void __exit s3c24xxfb_cleanup(void)
{
	s3c24xxfb_stop_lcd();
	msleep(1);

	if (lcd_clock) {
		clk_disable(lcd_clock);
		clk_unuse(lcd_clock);
		clk_put(lcd_clock);
		lcd_clock = NULL;
	}

	unregister_framebuffer(&info.fb);
	release_mem_region(S3C24XX_VA_LCD, S3C24XX_SZ_LCD);
}

#ifdef CONFIG_PM

/* suspend and resume support for the lcd controller */

static int s3c24xxfb_suspend(struct device *dev, u32 state, u32 level)
{
	if (level == SUSPEND_DISABLE || level == SUSPEND_POWER_DOWN) {
		s3c24xxfb_stop_lcd();

		/* sleep before disabling the clock, we need to ensure
		 * the LCD DMA engine is not going to get back on the bus
		 * before the clock goes off again (bjd) */

#if 0
		if (mach_info != NULL) {
			/* don't use our helper functions in this file,
			   becuase we want to save the original values for
			   when the system wakes up
			*/

			if (mach_info.lcd_power)
				(mach_info.lcd_power)(0);

			if (mach_info.backlight_power)
				(mach_info.backlight_power)(0);
		}
#endif

		msleep(1);
		clk_disable(lcd_clock);
	}

	return 0;
}

static int s3c24xxfb_resume(struct device *dev, u32 level)
{
	if (level == RESUME_ENABLE) {
		clk_enable(lcd_clock);
		msleep(1);

		s3c24xxfb_init_registers(&info);
		
		/* resume the backlight level */
		s3c24xxfb_backlight_power(backlight_power);
		s3c24xxfb_backlight_level(backlight_level);
	}

	return 0;
}

#else
#define s3c24xxfb_suspend NULL
#define s3c24xxfb_resume  NULL
#endif

static struct device_driver s3c24xxfb_driver = {
	.name		= "s3c2413-lcd",
	.bus		= &platform_bus_type,
	.probe		= s3c24xxfb_probe,
	.suspend	= s3c24xxfb_suspend,
	.resume		= s3c24xxfb_resume,
};

int __devinit s3c24xxfb_init(void)
{
	return driver_register(&s3c24xxfb_driver);
}

module_init(s3c24xxfb_init);
module_exit(s3c24xxfb_cleanup);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Framebuffer driver for the s3c24xx");
MODULE_LICENSE("GPL");
