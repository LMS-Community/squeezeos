
/************************************************************
* TFTP LCD Driver modified for pmg22 by works               *
* Contact : kwonkh@yamuco.kr                                *
************************************************************/

#include <linux/config.h>

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

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/s3c24xxfb.h>
#include <asm/hardware/clock.h>

#include "elfin_fb.h"
#include "asm/arch/regs-clock.h" //panna
#define S3C2413_FBIO_SETBRIGHTNESS	_IOW('F', 0x50, int)
#define DEFAULT_BACKLIGHT_LEVEL		2

extern void s3c24xxfb_backlight_level(int to);

#define H_FP	2		/* front porch */
#define H_SW	2		/* Hsync width */
#define H_BP	2		/* Back porch */

#define V_FP	2		/* front porch */
#define V_SW	2		/* Vsync width */
#define V_BP	1		/* Back porch */

#define H_RESOLUTION	240	/* horizon pixel  x resolition */
#define V_RESOLUTION	320	/* line cnt       y resolution */
#define VFRAME_FREQ     60	/* frame rate freq */

#define PIXEL_CLOCK	VFRAME_FREQ * LCD_PIXEL_CLOCK	/*  vclk = frame * pixel_count */
#define PIXEL_BPP	16	/*  RGB 5-6-5 format for SMDK24A0 EVAL BOARD */
#define PIXEL_BPP24	32

#define LCD_PIXEL_CLOCK (VFRAME_FREQ *(H_FP+H_SW+H_BP+H_RESOLUTION) * (V_FP+V_SW+V_BP+V_RESOLUTION))

#define MAX_DISPLAY_BRIGHTNESS		100
#define DEF_DISPLAY_BRIGHTNESS		50


struct s3c24xxfb_mach_info mach_info  = {
    .pixclock        	= PIXEL_CLOCK,
    .xres             	= H_RESOLUTION,
    .yres             	= V_RESOLUTION,

    .hsync_len        	= H_SW,
    .vsync_len        	= V_SW,
    .left_margin      	= H_FP,
    .upper_margin    	= V_FP,
    .right_margin     	= H_BP,
    .lower_margin     	= V_BP,

	.width				= 240,
	.height				= 320,

    .sync				= 0,
    .cmap_static		= 1,

	.gpcup 				= 0xffffffff,
	.gpcup_mask 		= 0,
	.gpccon				= 0xaaaaaaaa,
	.gpccon_mask		= 0x0,
	.gpdup				= 0xffffffff,
	.gpdup_mask			= 0x0,
	.gpdcon				= 0xaaaaaaaa,
	.gpdcon_mask		= 0x0,
	.lpcsel				= 2,

    .bpp              	=PIXEL_BPP,
    .lcdcon1            =LCD1_BPP_16T | LCD1_PNR_TFT,
    .lcdcon2            =LCD2_VBPD(V_BP) | LCD2_VFPD(V_FP) | LCD2_VSPW(V_SW),
    .lcdcon3            =LCD3_HBPD(H_BP) | LCD3_HFPD(H_FP),
    .lcdcon4            =LCD4_HSPW(H_SW) ,
    .lcdcon5            =LCD5_FRM565 | LCD5_INVVLINE | LCD5_INVVFRAME | LCD5_HWSWP |LCD5_PWREN,
};


/*
 * s3c2410fb_ioctl - non standard ioctls (atm, only level)
 */

int s3c24xxfb_ioctl(struct inode *inode, struct file *file,
			   u_int cmd, u_long arg,
			   struct fb_info *info)
{
	int value;

	switch(cmd)
	{
		case S3C2413_FBIO_SETBRIGHTNESS:
			if (copy_from_user(&value, (__user void *)arg,
					   sizeof(int)))
				return(-EFAULT);
			s3c24xxfb_backlight_level(value);
			return(0);
			break;
		default:
			return(-EINVAL);
	}
}


/*
 * s3c2413fb_map_video_memory():
 *	Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *	allow palette and pixel writes to occur without flushing the
 *	cache.  Once this area is remapped, all virtual memory
 *	access to the video memory should occur at the new region.
 */

int __init s3c24xxfb_map_video_memory(struct s3c24xxfb_info *fbi)
{
	dprintk("map_video_memory(fbi=%p)\n", fbi);

	fbi->map_size_f1 = PAGE_ALIGN(fbi->fb.fix.smem_len + PAGE_SIZE);
	fbi->map_cpu_f1 = dma_alloc_writecombine(fbi->dev, fbi->map_size_f1,
					      &fbi->map_dma_f1, GFP_KERNEL);

	fbi->map_size_f1 = fbi->fb.fix.smem_len;

	if (fbi->map_cpu_f1) {
		/* prevent initial garbage on screen */
		dprintk("map_video_memory: clear %p:%08x\n",
			fbi->map_cpu_f1, fbi->map_size_f1);
		memset(fbi->map_cpu_f1, 0x0, fbi->map_size_f1);

		fbi->screen_dma_f1 = fbi->map_dma_f1;
		fbi->fb.screen_base = fbi->map_cpu_f1;
		fbi->fb.fix.smem_start = fbi->screen_dma_f1;

		dprintk("map_video_memory: dma=%08x cpu=%p size=%08x\n",
			fbi->map_dma_f1, fbi->map_cpu_f1, fbi->fb.fix.smem_len);
	}

	return fbi->map_cpu_f1 ? 0 : -ENOMEM;
}

static void modify_gpio(unsigned long reg,
			       unsigned long set, unsigned long mask)
{
	unsigned long tmp;

	tmp = __raw_readl(reg) & ~mask;
	__raw_writel(tmp | set, reg);

	dprintk("%08lx set to %08x\n", reg, __raw_readl(reg));
}

/*
 * s3c2410fb_init_registers - Initialise all LCD-related registers
 */

int s3c24xxfb_init_registers(struct s3c24xxfb_info *fbi)
{
    unsigned long lcdsaddr1, lcdsaddr2, lcdsaddr3;
    struct clk *lcd_clock;
	u_long flags;

	unsigned long val, VideoPhysicalTemp = fbi->screen_dma_f1;
	struct fb_var_screeninfo *var= &fbi->fb.var;

	local_irq_save(flags);

	lcd_clock = clk_get(NULL, "lcd");

	mach_info.lcdcon1 = mach_info.lcdcon1 & ~LCD1_ENVID;
	mach_info.lcdcon1 |= LCD1_CLKVAL((int)((clk_get_rate(lcd_clock) / LCD_PIXEL_CLOCK/2) - 1));

	mach_info.lcdcon2 = (mach_info.lcdcon2 & ~LCD2_LINEVAL_MSK)
	    | LCD2_LINEVAL(var->yres - 1);

	/* TFT LCD only ! */
	mach_info.lcdcon3 = (mach_info.lcdcon3 & ~LCD3_HOZVAL_MSK)
	    | LCD3_HOZVAL(var->xres - 1);

	lcdsaddr1 =
	    LCDADDR_BANK(((unsigned long) VideoPhysicalTemp >> 22))
	    | LCDADDR_BASEU(((unsigned long) VideoPhysicalTemp >> 1));

	/* 16bpp */
	lcdsaddr2 = LCDADDR_BASEL(((unsigned long)
					    VideoPhysicalTemp +
					    (var->xres * 2 * (var->yres)))
					   >> 1);

	lcdsaddr3 = LCDADDR_OFFSET(0) | (LCDADDR_PAGE(var->xres));


	modify_gpio(S3C2413_GPCDN, mach_info.gpcup, mach_info.gpcup_mask);
	modify_gpio(S3C2413_GPCCON, mach_info.gpccon, mach_info.gpccon_mask);
	modify_gpio(S3C2413_GPDDN, mach_info.gpdup, mach_info.gpdup_mask);
	modify_gpio(S3C2413_GPDCON, mach_info.gpdcon, mach_info.gpdcon_mask);

	__raw_writel(mach_info.lcdcon1, S3C2413_LCDCON1);
	__raw_writel(mach_info.lcdcon2, S3C2413_LCDCON2);
	__raw_writel(mach_info.lcdcon3, S3C2413_LCDCON3);
	__raw_writel(mach_info.lcdcon4, S3C2413_LCDCON4);
	__raw_writel(mach_info.lcdcon5, S3C2413_LCDCON5);
	__raw_writel(lcdsaddr1, S3C2413_LCDSADDR1);
	__raw_writel(lcdsaddr2, S3C2413_LCDSADDR2);
	__raw_writel(lcdsaddr3, S3C2413_LCDSADDR3);

	val = __raw_readl(S3C2413_LPCSEL);
	val &= ~0x7;
	val |= (1<<4);
	__raw_writel(val, S3C2413_LPCSEL);
	__raw_writel(0, S3C2413_TPAL);

	val = __raw_readl(S3C2413_LCDCON1);
	val |= S3C2413_LCDCON1_ENVID;	/* LCD control signal ON */
	__raw_writel(val, S3C2413_LCDCON1);

	local_irq_restore(flags);

	return 0;
}



/**************************************************
 Code to initialize the LCD for 2413 SMDK Eval board
 through SPI controlled by GPIO's.
*************************************************/

#define LCD_NCS		(1<<7)
#define LCD_DATA	(1<<8)
#define LCD_CLK		(1<<3)
#define LCD_RESET 	(0)


#define LCD_NCS_Lo	{ gpdat = __raw_readl(S3C2413_GPBDAT); \
					  gpdat &= ~LCD_NCS; \
					  __raw_writel(gpdat, S3C2413_GPBDAT); \
					}

#define LCD_NCS_Hi	{ gpdat = __raw_readl(S3C2413_GPBDAT); \
					  gpdat |= LCD_NCS; \
					  __raw_writel(gpdat, S3C2413_GPBDAT); \
					}

#define LCD_CLK_Lo	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					  gpdat &= ~LCD_CLK; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

#define LCD_CLK_Hi	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					  gpdat |= LCD_CLK; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

#define LCD_DATA_Lo	{ gpdat = __raw_readl(S3C2413_GPBDAT); \
					  gpdat &= ~LCD_DATA; \
					  __raw_writel(gpdat, S3C2413_GPBDAT); \
					}

#define LCD_DATA_Hi	{ gpdat = __raw_readl(S3C2413_GPBDAT); \
					  gpdat |= LCD_DATA; \
					  __raw_writel(gpdat, S3C2413_GPBDAT); \
					}

#define LCD_RESET_Lo { gpdat = __raw_readl(S3C2413_GPGDAT); \
					  gpdat &= ~0x20; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

#define LCD_RESET_Hi	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					  gpdat |= 0x20; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

void Write_LDI(unsigned char address, unsigned char data)
{
	int j;
	unsigned long DELAY = 5;
	unsigned long gpdat;

	LCD_NCS_Hi		//      EN = High                                       CS high
	LCD_CLK_Hi
	LCD_DATA_Hi
	udelay(DELAY);

	LCD_NCS_Lo
	udelay(DELAY);
	//LCD_CLK_Lo
	//udelay(DELAY);
	//LCD_CLK_Hi
	//udelay(DELAY);
	//LCD_CLK_Lo
	//udelay(DELAY);
	//LCD_CLK_Hi
	//udelay(DELAY);
	LCD_CLK_Lo
	udelay(DELAY);

	for (j = 7; j >= 0; j--) {
		if ((address >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	for (j = 7; j >= 0; j--) {
		if ((data >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi	// CLOCK = High
		udelay(DELAY);

		if (j != 0) {
			LCD_CLK_Lo
			udelay(DELAY);	// CLOCK = Low
		}
	}

	LCD_NCS_Hi		// EN = High
	udelay(DELAY);
}

void SetGPIOforLDI(void)
{
	unsigned long gpdn;
	unsigned long gpdat;
	unsigned long gpcon;

	//nCS, DATA
	gpdn = __raw_readl(S3C2413_GPBDN);
	gpdn |= (0x3<<7);
	__raw_writel(gpdn, S3C2413_GPBDN);

	//CLK
	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn  |= (0x1<<3);
	__raw_writel(gpdn, S3C2413_GPGDN);

	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn  |= (0x1<<5);
	__raw_writel(gpdn, S3C2413_GPGDN);

	//backlight
	gpdn = __raw_readl(S3C2413_GPBDN);
	gpdn |= (0x1<<6);
	__raw_writel(gpdn, S3C2413_GPBDN);

	//nCS, DATA
	gpdat = __raw_readl(S3C2413_GPBDAT);
	gpdat |= (0x3<<7);
	__raw_writel(gpdat, S3C2413_GPBDAT);
	//CLK
	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat |= (0x1<<3);
	__raw_writel(gpdat, S3C2413_GPGDAT);
	//backlight
	gpdat = __raw_readl(S3C2413_GPBDAT);
	gpdat |= (0x1<<6);
	__raw_writel(gpdat, S3C2413_GPBDAT);
	//nCS, DATA
	gpcon = __raw_readl(S3C2413_GPBCON);
	gpcon &= ~(0xf<<14);
	gpcon |= (0x5<<14);
	__raw_writel(gpcon, S3C2413_GPBCON);
	//CLK
	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon &= ~(0x3<<6);
	gpcon |= (0x1<<6);
	__raw_writel(gpcon, S3C2413_GPGCON);
	
	//backlight
	gpcon = __raw_readl(S3C2413_GPBCON);
	gpcon &= ~(0x3<<12);
	gpcon |= (0x1<<12);
	__raw_writel(gpcon, S3C2413_GPBCON);

	//nReset(G5)
	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon &= ~(0x0c<<8);
	gpcon |= (0x4<<8);
	__raw_writel(gpcon, S3C2413_GPGCON);

	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat |= (0x1<<5);
	//gpdat &=~(0x1<<5);
	__raw_writel(gpdat, S3C2413_GPGDAT);


}


 void Init_LDI(void)
{
	unsigned long gpdat;

	SetGPIOforLDI();
	mdelay(100);

	LCD_NCS_Hi
	LCD_CLK_Hi
	LCD_DATA_Hi
  	LCD_RESET_Lo
	mdelay(10);
	LCD_RESET_Hi
	mdelay(10);


	Write_LDI(0x01,0x10); // Start oscillation
	Write_LDI(0x00,0xA0); // Standby mode cancel
	Write_LDI(0x03,0x01); // Software reset operation
	mdelay(10);

	Write_LDI(0x03,0x00); // Software reset operation cancel
	Write_LDI(0x2B,0x04); // Oscillator frequency adjust setting
	Write_LDI(0x28,0x18); // DC/DC clock frequency adjust setting
	Write_LDI(0x1A,0x05); // Step up circuit frequency adjust setting
	Write_LDI(0x25,0x05); // Step up factor in step up circuit 2 setting
	Write_LDI(0x19,0x00); // VR1 and VR2 regulator factor setting

	//############# void Power_on_Set(void) ################//

	Write_LDI(0x1C,0x73); // Step up circuit operating current setting
	Write_LDI(0x24,0x74); // V18 and VCOM regulator current setting
	Write_LDI(0x1E,0x01); // Extra step up circuit1 operation
	Write_LDI(0x18,0xC1); // VR1 and VR2 regulator on
	mdelay(10);

	Write_LDI(0x18,0xE1); // VCL turn on
	Write_LDI(0x18,0xF1); // VGH and VGL turn on
	mdelay(60);

	Write_LDI(0x18,0xF5); // DDVDH turn on
	mdelay(60);

	Write_LDI(0x1B,0x09); // VS/VDH turn on and set
	mdelay(10);

	Write_LDI(0x1F,0x11); // VCOM amplitude voltage setting
	Write_LDI(0x20,0x0E); // VCOMH voltage setting
	Write_LDI(0x1E,0x81); // VCOM operation start
	mdelay(10);

	Write_LDI(0x9D,0x02);
	Write_LDI(0xC0,0x00);
	Write_LDI(0xC1,0x01); // BGR bit = ¡°0¡±
	Write_LDI(0x0E,0x00);
	Write_LDI(0x0F,0x00);
	Write_LDI(0x10,0x00);
	Write_LDI(0x11,0x00);
	Write_LDI(0x12,0x00);
	Write_LDI(0x13,0x00);
	Write_LDI(0x14,0x00);
	Write_LDI(0x15,0x00);
	Write_LDI(0x16,0x00);
	Write_LDI(0x17,0x00);
	Write_LDI(0x34,0x01);
	Write_LDI(0x35,0x00);
	Write_LDI(0x4B,0x00);
	Write_LDI(0x4C,0x00);
	Write_LDI(0x4E,0x00);
	Write_LDI(0x4F,0x00);
	Write_LDI(0x50,0x00);
	Write_LDI(0x02,0x07);

	Write_LDI(0x3C,0x00);
	Write_LDI(0x3D,0x00);
	Write_LDI(0x3E,0x01);
	Write_LDI(0x3F,0x3F);
	Write_LDI(0x40,0x02);
	Write_LDI(0x41,0x02);

	Write_LDI(0x42,0x00);
	Write_LDI(0x43,0x00);
	Write_LDI(0x44,0x00);
	Write_LDI(0x45,0x00);
	Write_LDI(0x46,0xEF);
	Write_LDI(0x47,0x00);
	Write_LDI(0x48,0x00);
	Write_LDI(0x49,0x01);
	Write_LDI(0x4A,0x3F);

	Write_LDI(0x1D,0x08); // Gate scan direction setting
	Write_LDI(0x86,0x00);
	Write_LDI(0x87,0x30);
	Write_LDI(0x88,0x02);
	Write_LDI(0x89,0x05);
	Write_LDI(0x8D,0x01); // Register set-up mode for one line clock number
	Write_LDI(0x8B,0x30); // One line SYSCLK number in one-line scan
	Write_LDI(0x33,0x01); // N line inversion setting
	Write_LDI(0x37,0x01); // Scanning method setting
	Write_LDI(0x76,0x00);

	//############# void Gamma_Set(void) ################//

	Write_LDI(0x8F,0x10);
	Write_LDI(0x90,0x67);
	Write_LDI(0x91,0x07);
	Write_LDI(0x92,0x65);
	Write_LDI(0x93,0x07);
	Write_LDI(0x94,0x01);
	Write_LDI(0x95,0x76);
	Write_LDI(0x96,0x56);
	Write_LDI(0x97,0x00);
	Write_LDI(0x98,0x00);
	Write_LDI(0x99,0x00);
	Write_LDI(0x9A,0x00);

	//############# void Display_On(void) ################//

	Write_LDI(0x3B,0x01);
  mdelay(40);

	Write_LDI(0x00,0x20);
	mdelay(10);
}


