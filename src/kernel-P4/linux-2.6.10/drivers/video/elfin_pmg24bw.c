
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

#define H_FP	12	//2		/* front porch */
#define H_SW	2				/* Hsync width */
#define H_BP	12	//2		/* Back porch */

#define V_FP	12		/* front porch */
#define V_SW	2		/* Vsync width */
#define V_BP	13	//1		/* Back porch */

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
//    .lcdcon2            =LCD2_VBPD(V_BP) | LCD2_VFPD(V_FP) | LCD2_VSPW(V_SW),
     .lcdcon2            =V_BP<<24 | V_FP<<6 | LCD2_VSPW(V_SW),
 //   .lcdcon3            =LCD3_HBPD(H_BP) | LCD3_HFPD(H_FP),
    .lcdcon3            =H_BP<<19 | H_FP,
    .lcdcon4            =LCD4_HSPW(H_SW) ,
    .lcdcon5            =LCD5_FRM565 | LCD5_INVVLINE | LCD5_INVVFRAME | LCD5_HWSWP |LCD5_PWREN | 0x01<<6,
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
#define LCD_RESET 	(1<<13)


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
					  gpdat &= ~LCD_RESET; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

#define LCD_RESET_Hi	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					  gpdat |= LCD_RESET; \
					  __raw_writel(gpdat, S3C2413_GPGDAT); \
					}

void LCD_DATA_Output(void)
{
	// GPB8 Output
	s3c2413_gpio_cfgpin(S3C2413_GPB8, S3C2413_GPB8_OUTP);
}

void LCD_DATA_Input(void)
{
	// GPB8 Input
	s3c2413_gpio_cfgpin(S3C2413_GPB8, S3C2413_GPB8_INP);
}

unsigned char LCD_DATA_READ(void)
{
	// GPB8 Read
	return s3c2413_gpio_getpin(S3C2413_GPB8);
}

void LCD_CMD(unsigned short address, unsigned short data)
{

	int j;
	unsigned char start_byte;
	unsigned long DELAY = 5;
	unsigned long gpdat;

	//printk("LCD_CMD addr=%x data=%x\n", address, data);

	mdelay(5);//
	LCD_NCS_Hi		//      EN = High                                       CS high
	LCD_CLK_Hi
	LCD_DATA_Hi
	udelay(DELAY);
	LCD_NCS_Lo
	udelay(DELAY);
	LCD_CLK_Lo
	udelay(DELAY);

	//----------------------------------------------------------------
	//
	//			Send the Address
	//
	//----------------------------------------------------------------
	//1. start byte		- address
	start_byte = 0x74;		//01110100
	for (j = 7; j >= 0; j--) {
		if ((start_byte >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	//2. address setting
	for (j = 15; j >= 0; j--)
	{
		if ((address >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	LCD_NCS_Hi		// EN = High
	LCD_CLK_Hi
	LCD_DATA_Hi
	mdelay(5);//
	udelay(DELAY);
	LCD_NCS_Lo
	udelay(DELAY);
	LCD_CLK_Lo
	udelay(DELAY);
	//----------------------------------------------------------------
	//
	//			Send the data
	//
	//----------------------------------------------------------------
	//start byte		- data
	start_byte = 0x76;		//01110110
	for (j = 7; j >= 0; j--) {
		if ((start_byte >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	//data setting
	for (j = 15; j >= 0; j--) {
		if ((data >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}
	LCD_NCS_Hi		// EN = High
	LCD_CLK_Hi
	LCD_DATA_Hi
	udelay(DELAY);
	mdelay(5);//
}

unsigned short LCD_READ(unsigned short address)
{

	int j;
	unsigned char start_byte;
	unsigned long DELAY = 5;
	unsigned long gpdat;
	unsigned short data, mask;

	mdelay(5);//
	LCD_NCS_Hi		//      EN = High                                       CS high
	LCD_CLK_Hi
	LCD_DATA_Hi
	udelay(DELAY);
	LCD_NCS_Lo
	udelay(DELAY);
	LCD_CLK_Lo
	udelay(DELAY);

	//----------------------------------------------------------------
	//
	//			Send the Address
	//
	//----------------------------------------------------------------
	//1. start byte		- address
	start_byte = 0x74;		//01110100
	for (j = 7; j >= 0; j--) {
		if ((start_byte >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	//2. address setting
	for (j = 15; j >= 0; j--)
	{
		if ((address >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	LCD_NCS_Hi		// EN = High
	LCD_CLK_Hi
	LCD_DATA_Hi
	mdelay(5);//
	udelay(DELAY);
	LCD_NCS_Lo
	udelay(DELAY);
	LCD_CLK_Lo
	udelay(DELAY);
	//----------------------------------------------------------------
	//
	//			Send the data
	//
	//----------------------------------------------------------------
	//start byte		- data
	start_byte = 0x77;		//01110111
	for (j = 7; j >= 0; j--) {
		if ((start_byte >> j) & 0x01) LCD_DATA_Hi
		else LCD_DATA_Lo

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	for (j = 7; j >= 0; j--) {

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	LCD_DATA_Input();
	udelay(DELAY);

	//data setting
	data = 0;
	mask = 0x8000;
	for (j = 15; j >= 0; j--) {

		udelay(DELAY);
		LCD_CLK_Hi
		udelay(DELAY);

		if(LCD_DATA_READ()== 0x1) data |= mask;

		mask = mask >> 1;

		LCD_CLK_Lo
		udelay(DELAY);	// CLOCK = Low
	}

	LCD_DATA_Output();
	LCD_NCS_Hi		// EN = High
	LCD_CLK_Hi
	LCD_DATA_Hi
	udelay(DELAY);
	mdelay(5);//

	return ( data );
}

void SetGPIOforLDI(void)
{
	unsigned long gpdn;
	unsigned long gpdat;
	unsigned long gpcon;

	//---------------- LCD -----------------------------------------
	//CLK(G3)											-- PULL-DOWN
	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn |= (0x1<<3);
	__raw_writel(gpdn, S3C2413_GPGDN);

	//nCS(B7),DATA(B8)
	gpdn = __raw_readl(S3C2413_GPBDN);
	gpdn  |= (0x3<<7);
	__raw_writel(gpdn, S3C2413_GPBDN);

	//LCD PWR(XX), nRESET(G13)
	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn  |= (0x1<<13);
	__raw_writel(gpdn, S3C2413_GPGDN);

	//LCD_ENABLE(C4)
	gpdn = __raw_readl(S3C2413_GPCDN);
	gpdn  |= (0x1<<4);
	__raw_writel(gpdn, S3C2413_GPCDN);

	//backlight
	gpdn = __raw_readl(S3C2413_GPBDN);
	gpdn |= (0x1<<6);
	__raw_writel(gpdn, S3C2413_GPBDN);

	//CLK																-- DATA
	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat |= (0x1<<3);
	__raw_writel(gpdat, S3C2413_GPGDAT);
	//nCS, DATA
	gpdat = __raw_readl(S3C2413_GPBDAT);
	gpdat |= (0x3<<7);
	__raw_writel(gpdat, S3C2413_GPBDAT);
	//LCD_ENABLE
	gpdat = __raw_readl(S3C2413_GPCDAT);
	gpdat &= ~(0x1<<4);
	__raw_writel(gpdat, S3C2413_GPCDAT);
	//backlight
	gpdat = __raw_readl(S3C2413_GPBDAT);
	gpdat &= ~(0x1<<6);
	__raw_writel(gpdat, S3C2413_GPBDAT);

	//CLK																-- CONTROL REGISTER
	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon &= ~(0x3<<6);
	gpcon |= (0x1<<6);
	__raw_writel(gpcon, S3C2413_GPGCON);
	//nCS, DATA
	gpcon = __raw_readl(S3C2413_GPBCON);
	gpcon &= ~(0xF<<14);
	gpcon |= (0x5<<14);
	__raw_writel(gpcon, S3C2413_GPBCON);
	//LCD_ENABLE
	gpcon = __raw_readl(S3C2413_GPCCON);
	gpcon &= ~(0x3<<8);
	gpcon |= (0x2<<8);
	__raw_writel(gpcon, S3C2413_GPCCON);
	//backlight
	gpcon = __raw_readl(S3C2413_GPBCON);
	gpcon &= ~(0x3<<12);
	gpcon |= (0x1<<12);
	__raw_writel(gpcon, S3C2413_GPBCON);

	//LCD PWR(XX), nReset(G13)
	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon &= ~(0x03<<26);
	gpcon |= (0x1<<26);
	__raw_writel(gpcon, S3C2413_GPGCON);

	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat |= (0x1<<13);
	__raw_writel(gpdat, S3C2413_GPGDAT);

}


 void Init_LDI(void)
{
	unsigned long gpdat;
	unsigned short data;

	SetGPIOforLDI();
	mdelay(100);

	LCD_NCS_Hi
	LCD_CLK_Hi
	LCD_DATA_Hi
	LCD_RESET_Hi
	mdelay(10);
 	LCD_RESET_Lo
	mdelay(20);
	LCD_RESET_Hi
	mdelay(100);


	//;VGG243237-6UFLWA (2.4"CMO TFT + ILI9320 ) (SPI+18 bit RGB interface)
	//********START INITIAL Sequence ************/
	LCD_CMD(0X00E5,0X8000);// ;//Set the vcore voltage and setting is must
	LCD_CMD(0X0000,0X0001);// ;start internal OSC
	mdelay(20);
	//LCD_CMD(0X0007,0x0000);

	data = LCD_READ(0X0000);
	printk("device id = %x \n",data);


	LCD_CMD(0X0001,0X0100);// ;set SS and SM bit
	LCD_CMD(0X0002,0X0700);// ;set 1-line inversion
	//======================================= 0030
	LCD_CMD(0X0003,0X1030);// ;set GRAM write dicrection and BGR =1
	//=========================================
	LCD_CMD(0X0004,0X0000);//
	LCD_CMD(0X0008,0X020C);//
	LCD_CMD(0X0009,0X0000);//
	LCD_CMD(0X000A,0X0000);//
	//======================================== 0000
	LCD_CMD(0X000C,0X0110);// ;//RM=1, RGB interface, DM1,DM0=01: RGB DISPLAY CLOCK , RIM1,RIM0=00,18 bit
	//RGB interface
	//========================================
	LCD_CMD(0X000D,0X0000);//
	LCD_CMD(0X000F,0X0001);// ;DPL
	//*********** Power on sequence ***********
	LCD_CMD(0X0010,0X0000);// ;SAP,BT(3:0),AP,DSTB,SLP,STB
	LCD_CMD(0X0011,0X0007);// ;DC1(2:0), DC0(2:0), VC(2:0)
	LCD_CMD(0X0012,0X0000);// ;VREG1OUT voltage
	LCD_CMD(0X0013,0X0000);// ;VDV(4:0) for VCOM amplitude
	mdelay(300);//
	LCD_CMD(0X0010,0X17B0);// ;SAP,BT(3:0),AP,DSTB,SLP,STB
	LCD_CMD(0X0011,0X0037);// ;DC1(2:0), DC0(2:0), VC(2:0)
	mdelay(100);
	//======================================= 013E
	LCD_CMD(0X0012,0X013C);// ;VREG1OUT voltage
	//=======================================
	mdelay(100);//
	//======================================== 1F00
	LCD_CMD(0X0013,0X1C00);// ;VDV(4:0) for VCOM amplitude ;1F00
	//=======================================
	LCD_CMD(0X0029,0X000E);// ;VCM(4:0) for VOMH amplitude ;0013
	mdelay(100);//
	LCD_CMD(0X0020,0X0000);// ;GRAM Horizontal address
	LCD_CMD(0X0021,0X0000);// ;GRAM Vertical address
	//*********** Adjust the GAMMA curve*********
	LCD_CMD(0X0030,0X0000);
	LCD_CMD(0X0031,0X0505);
	LCD_CMD(0X0032,0X0004);
	LCD_CMD(0X0035,0X0006);
	LCD_CMD(0X0036,0X0707);
	LCD_CMD(0X0037,0X0105);
	LCD_CMD(0X0038,0X0002);
	LCD_CMD(0X0039,0X0707);
	LCD_CMD(0X003C,0X0704);
	LCD_CMD(0X003D,0X0807);
	//**************GRAM area set*********
	LCD_CMD(0X0050,0X0000); //;Horizontal GRAM start address
	LCD_CMD(0X0051,0X00EF);// ;Horizontal GRAM end address
	LCD_CMD(0X0052,0X0000);// ;Vertical GRAM start address
	LCD_CMD(0X0053,0X013F);// ;Vertical GRAM end address
	LCD_CMD(0X0060,0X2700);
	LCD_CMD(0X0061,0X0001);
	LCD_CMD(0X006A,0X0000);
	//;//Partial display control
	LCD_CMD(0X0080,0X0000);
	LCD_CMD(0X0081,0X0000);
	LCD_CMD(0X0082,0X0000);
	LCD_CMD(0X0083,0X0000);
	LCD_CMD(0X0084,0X0000);
	LCD_CMD(0X0085,0X0000);
	//;//*************Panel control*************
	LCD_CMD(0X0090,0X0010);
	LCD_CMD(0X0092,0X0000);
	//ÝÂúí(1)
	//VGG243237_6UFLWA_RGB_INITIAL.TXT
	LCD_CMD(0X0093,0X0003);
	LCD_CMD(0X0095,0X0110);
	LCD_CMD(0X0097,0X0000);
	LCD_CMD(0X0098,0X0000);
	LCD_CMD(0X0007,0X0173);
}


