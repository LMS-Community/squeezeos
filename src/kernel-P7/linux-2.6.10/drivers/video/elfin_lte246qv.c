
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

void delayLoop(int count);
void Init_LDI(void);
void WriteLDI_LTE246(int address, int data);
void WriteLDI_LTE246_Index(int address);

extern void s3c24xxfb_backlight_level(int to);

#define H_FP	23		/* front porch */
#define H_SW	3		/* Hsync width */
#define H_BP	7		/* Back porch */

#define V_FP	7		/* front porch */
#define V_SW	0		/* Vsync width */
#define V_BP	6		/* Back porch */

#define H_RESOLUTION	240	/* horizon pixel  x resolition */
#define V_RESOLUTION	320	/* line cnt       y resolution */
#define VFRAME_FREQ     115	/* frame rate freq */

#define PIXEL_CLOCK	VFRAME_FREQ * LCD_PIXEL_CLOCK	/*  vclk = frame * pixel_count */
#define PIXEL_BPP	16	/*  RGB 5-6-5 format for SMDK24A0 EVAL BOARD */
#define PIXEL_BPP24	32	

#define LCD_PIXEL_CLOCK (VFRAME_FREQ *(H_FP+H_SW+H_BP+H_RESOLUTION) * (V_FP+V_SW+V_BP+V_RESOLUTION))

#define MAX_DISPLAY_BRIGHTNESS		100
#define DEF_DISPLAY_BRIGHTNESS		50
struct s3c24xxfb_mach_info mach_info  = {
      .pixclock        =PIXEL_CLOCK,
      .xres             =H_RESOLUTION,
      .yres             =V_RESOLUTION,
                                                                                           
      .hsync_len          = H_SW, 
      .vsync_len          =V_SW,
      .left_margin        = H_FP, 
      .upper_margin    =V_FP,
      .right_margin      = H_BP, 
      .lower_margin     =V_BP,

	.width=		240,
	.height=	320,
	
      .sync= 0, 
      .cmap_static=1,

	.gpcup =    0xffffffff,
	.gpcup_mask = 0,
	.gpccon=     0xaaaaaaaa,
	.gpccon_mask= 0x0,		
	.gpdup=      0xffffffff,
	.gpdup_mask= 0x0,		
	.gpdcon=     0xaaaaaaaa,
	.gpdcon_mask=  0x0,		
	.lpcsel=      2,

       .bpp              =PIXEL_BPP24,
      .lcdcon1             =LCD1_BPP_24T | LCD1_PNR_TFT,
      .lcdcon2             =LCD2_VBPD(V_BP) | LCD2_VFPD(V_FP) | LCD2_VSPW(V_SW),
      .lcdcon3             =LCD3_HBPD(H_BP) | LCD3_HFPD(H_FP),
      .lcdcon4             =LCD4_HSPW(H_SW) ,
      .lcdcon5             =LCD5_FRM565 | LCD5_INVVCLK | LCD5_INVVLINE | LCD5_INVVFRAME |LCD5_INVVDEN ,
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
		memset(fbi->map_cpu_f1, 0xf0, fbi->map_size_f1);

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
 * s3c2413fb_init_registers - Initialise all LCD-related registers
 */

int s3c24xxfb_init_registers(struct s3c24xxfb_info *fbi)
{	
       unsigned long lcdsaddr1, lcdsaddr2, lcdsaddr3;
       struct clk *lcd_clock;
	u_int half_screen_size, yres;
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

       /* 24bpp */
	lcdsaddr2 = LCDADDR_BASEL(((unsigned long)
					    VideoPhysicalTemp +
					    (var->xres * 4 * (var->yres)))
					   >> 1);

	lcdsaddr3 = LCDADDR_OFFSET(0) | (LCDADDR_PAGE(var->xres)*2);

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
	val &=(~0x1f);
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

#define LCD_DEN		(1<<2)
#define LCD_DSERI	(1<<12)
#define LCD_DCLK	(1<<13)
#define LCD_RESET     (1<<11)


#define LCD_DEN_Lo	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					   gpdat &= ~LCD_DEN; \
					   __raw_writel(gpdat, S3C2413_GPGDAT); \
					 }

#define LCD_DEN_Hi	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					   gpdat |= LCD_DEN; \
					   __raw_writel(gpdat, S3C2413_GPGDAT); \
					 }

#define LCD_DCLK_Lo	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat &= ~LCD_DCLK; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					 }

#define LCD_DCLK_Hi	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat |= LCD_DCLK; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					 }

#define LCD_DSERI_Lo  { gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat &= ~LCD_DSERI; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					  }

#define LCD_DSERI_Hi	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
							gpdat |= LCD_DSERI; \
							__raw_writel(gpdat, S3C2413_GPEDAT); \
						}

#define LCD_RESET_Lo  { gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat &= ~LCD_RESET; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					  }

#define LCD_RESET_Hi	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
							gpdat |= LCD_RESET; \
							__raw_writel(gpdat, S3C2413_GPEDAT); \
						}


void WriteLDI_LTE246(int address, int data)
{
 	unsigned char	dev_id_code=0x1D;
       int     j;
	unsigned char DELAY=100;
	unsigned long gpdat;
	
	
	LCD_DEN_Hi; 		//	EN = High					CS high
	LCD_DCLK_Hi;							//	SCL High
	LCD_DSERI_Hi;							//	Data Low

	delayLoop(DELAY);

	LCD_DEN_Lo; 		//	EN = Low				CS Low
	delayLoop(DELAY);
	
	for (j = 5; j >= 0; j--)
	{	
		LCD_DCLK_Lo;							//	SCL Low

		if ((dev_id_code >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	
	// RS = "0" : index data
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Lo;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	// Write
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Lo;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	for (j = 15; j >= 0; j--)
	{
		LCD_DCLK_Lo;							//	SCL Low

		if ((address >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	LCD_DSERI_Hi;
	delayLoop(DELAY);
	
	LCD_DEN_Hi; 				// EN = High
	delayLoop(DELAY*10);

	LCD_DEN_Lo; 		//	EN = Low				CS Low
	delayLoop(DELAY);
	
	for (j = 5; j >= 0; j--)
	{	
		LCD_DCLK_Lo;							//	SCL Low

		if ((dev_id_code >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	
	// RS = "1" instruction data
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Hi;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	// Write
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Lo;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	for (j = 15; j >= 0; j--)
	{
		LCD_DCLK_Lo;							//	SCL Low

		if ((data >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	
	LCD_DEN_Hi; 				// EN = High
	delayLoop(DELAY);

}

void SetGPIOforLDI(void)
{
	unsigned long gpdn;
	unsigned long gpdat;
	unsigned long gpcon;

	gpdn = __raw_readl(S3C2413_GPEDN);
	gpdn  |= (0xf<<11);
	__raw_writel(gpdn, S3C2413_GPEDN);

	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn  |= (0x1<<2);
	__raw_writel(gpdn, S3C2413_GPGDN);

	gpdat = __raw_readl(S3C2413_GPEDAT);
	gpdat  |= (0xe<<11);
	__raw_writel(gpdat, S3C2413_GPEDAT);

	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat  |= (0x1<<2);
	__raw_writel(gpdat, S3C2413_GPGDAT);

	gpcon = __raw_readl(S3C2413_GPECON);
	gpcon  = (gpcon & ~(0x3f<<22))|(0x15<<22);
	__raw_writel(gpcon, S3C2413_GPECON);

	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon  = (gpcon & ~(0x3<<4))|(0x1<<4);
	__raw_writel(gpcon, S3C2413_GPGCON);
}

void delayLoop(int count) 
{ 
    int j; 
    for(j = 0; j < count; j++)  ; 
}

void Init_LDI(void)
{
      unsigned long gpdat;
    
       SetGPIOforLDI();
	LCD_DEN_Hi;
	LCD_DCLK_Hi;
	LCD_DSERI_Hi;
	LCD_RESET_Lo;	
	
	delayLoop(1*180000);
	LCD_RESET_Hi;
	//delayLoop(50*180000);
	delayLoop(10*180000);
	
	//Power Setting Sequence
	WriteLDI_LTE246(0x11, 0x3f07);
	//WriteLDI_LTE246(0x11, 0x1007);
	WriteLDI_LTE246(0x14, 0x3147);
	//WriteLDI_LTE246(0x14, 0xbf09);
	WriteLDI_LTE246(0x10, 0x4d00);
	WriteLDI_LTE246(0x13, 0x0040);
	delayLoop(50*180000);
	WriteLDI_LTE246(0x13, 0x0060);
	delayLoop(20*180000);
	WriteLDI_LTE246(0x13, 0x0062);
	delayLoop(15*180000);
	WriteLDI_LTE246(0x13, 0x0063);
	delayLoop(20*180000);	
	WriteLDI_LTE246(0x13, 0x0073);
	delayLoop(50*180000);


	//Initializing Sequence
	WriteLDI_LTE246(0x01, 0x0127);
	WriteLDI_LTE246(0x02, 0x0300);
	WriteLDI_LTE246(0x03, 0x1030);
	WriteLDI_LTE246(0x07, 0x0000);
	WriteLDI_LTE246(0x08, 0x8808);
	WriteLDI_LTE246(0x09, 0x0001);
	WriteLDI_LTE246(0x0b, 0x3948);
	WriteLDI_LTE246(0x0c, 0x0110);
	WriteLDI_LTE246(0x30, 0x0300);
	WriteLDI_LTE246(0x31, 0x0303);
	WriteLDI_LTE246(0x32, 0x0303);
	WriteLDI_LTE246(0x33, 0x0000);
	WriteLDI_LTE246(0x34, 0x0300);
	WriteLDI_LTE246(0x35, 0x0303);
	WriteLDI_LTE246(0x36, 0x0303);
	WriteLDI_LTE246(0x37, 0x0000);
	WriteLDI_LTE246(0x38, 0x0f00);
	WriteLDI_LTE246(0x39, 0x0f00);

	
	delayLoop(100*180000);
	
	//Display On Sequence
	WriteLDI_LTE246(0x07, 0x0012);
	delayLoop(100*180000);
	WriteLDI_LTE246(0x07, 0x0017);	
	
	WriteLDI_LTE246(0x20, 0x0000);
	WriteLDI_LTE246(0x21, 0x0000);
	
	WriteLDI_LTE246_Index(0x22);

}

void WriteLDI_LTE246_Index(int address)
{
       unsigned long gpdat;
 	unsigned char	dev_id_code=0x1D;
       int     j;
	unsigned char DELAY=100;
		
	
	LCD_DEN_Hi; 		//	EN = High					CS high
	LCD_DCLK_Hi;							//	SCL High
	LCD_DSERI_Hi;							//	Data Low

	delayLoop(DELAY);

	LCD_DEN_Lo; 		//	EN = Low				CS Low
	delayLoop(DELAY);
	
	for (j = 5; j >= 0; j--)
	{	
		LCD_DCLK_Lo;							//	SCL Low

		if ((dev_id_code >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	
	// RS = "0" : index data
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Lo;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	// Write
	LCD_DCLK_Lo;			// CLOCK = Low
	LCD_DSERI_Lo;
	delayLoop(DELAY);
	LCD_DCLK_Hi;			// CLOCK = High
	delayLoop(DELAY);

	for (j = 15; j >= 0; j--)
	{
		LCD_DCLK_Lo;							//	SCL Low

		if ((address >> j) & 0x0001)	// DATA HIGH or LOW
		{
			LCD_DSERI_Hi;		
		}
		else
		{
			LCD_DSERI_Lo;
		}

		delayLoop(DELAY);

		LCD_DCLK_Hi;			// CLOCK = High
		delayLoop(DELAY);

	}
	LCD_DSERI_Hi;
	delayLoop(DELAY);
	
	LCD_DEN_Hi; 				// EN = High
	delayLoop(DELAY*10);


}

