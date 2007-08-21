#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/S3C2440.h>

#include "s3c2411fb.h"


#if 1
#define H_FP	10		/* front porch */
#define H_SW	0		/* Hsync width  */
#define H_BP	16		/* Back porch */

#define V_FP	0		/* front porch */
#define V_SW	0		/* Vsync width */
#define V_BP	0		/* Back porch */

#define H_RESOLUTION	320	/* x resolition */
#define V_RESOLUTION	240	/* y resolution  */
#define VFRAME_FREQ     30	/* frame rate freq. */
#else
#define H_FP	5		/* front porch */
#define H_SW	2		/* Hsync width  */
#define H_BP	0		/* Back porch */

#define V_FP	9		/* front porch */
#define V_SW	2		/* Vsync width */
#define V_BP	8		/* Back porch */

#define H_RESOLUTION	320	/* x resolition - from AP.. yongkal*/
#define V_RESOLUTION	240	/* y resolution  - from AP.. yongkal*/
#define VFRAME_FREQ     70	/* frame rate freq. - from AP.. yongkal*/

#endif

#define LCD_PIXEL_CLOCK (VFRAME_FREQ *(H_FP+H_SW+H_BP+H_RESOLUTION) * (V_FP+V_SW+V_BP+V_RESOLUTION))
#define PIXEL_CLOCK     VFRAME_FREQ * LCD_PIXEL_CLOCK	/*  vclk = frame * pixel_count - from AP .. yongkal*/
#define PIXEL_BPP       16	/*  RGB 5-6-5 format for SMDK24A0 EVAL BOARD */

static struct elfinfb_rgb rgb_8 = {
      red: {offset: 0, length:4,},
      green: {offset: 0, length:4,},
	  blue: {offset: 0, length:4,},
      transp: {offset: 0, length:0,},
};

static struct elfinfb_rgb def_rgb_16 = {
      red: {offset: 11, length:5,},
      green: {offset: 5, length:6,},
      blue: {offset: 0, length:5,},
      transp: {offset: 0, length:0,},
};

static struct elfinfb_rgb xxx_tft_rgb_16 = {
      red: {offset: 11, length:5,},
      green: {offset: 5, length:6,},
      blue: {offset: 0, length:5,},
      transp: {offset: 0, length:0,},
};

static struct elfinfb_mach_info lcd_std_info __initdata = {
      pixclock:PIXEL_CLOCK,
      bpp:PIXEL_BPP,
      xres:H_RESOLUTION,
      yres:V_RESOLUTION,
                                                                                           
      hsync_len: H_SW, vsync_len:V_SW,
      left_margin: H_FP, upper_margin:V_FP,
      right_margin: H_BP, lower_margin:V_BP,
                                                                                           
      sync: 0, cmap_static:1,
      reg:{
#if 1		
//from yongkal
              lcdcon1:S3C2413_LCDCON1_CMODE | S3C2413_LCDCON1_STN8 | S3C2413_LCDCON1_STN16BPP,
              lcdcon2:S3C2413_LCDCON2_VBPD(0) | S3C2413_LCDCON2_VFPD(0) | S3C2413_LCDCON2_VSPW(0),
              lcdcon3:S3C2413_LCDCON3_WDLY(0x10) | S3C2413_LCDCON3_LINEBLANK(0x10),
              lcdcon4:S3C2413_LCDCON4_MVAL(0x10) | S3C2413_LCDCON4_WLH(0x10),
              lcdcon5:S3C2413_LCDCON5_FRM565 | S3C2413_LCDCON5_HWSWP,
		 	  lcdcon6:0x10803000,
			  lcdcon8:(0x15<<26) | (0x0<<18)| (0x0<<10) | (0x0<<2) | (0<<1),
//to yongkal
#else 		
              lcdcon1:LCD1_BPP_16T | LCD1_PNR_TFT,
              lcdcon2:LCD2_VBPD(V_BP) | LCD2_VFPD(V_FP) |
         LCD2_VSPW(V_SW),
              lcdcon3:LCD3_HBPD(H_BP) | LCD3_HFPD(H_FP),
              lcdcon4:LCD4_HSPW(H_SW),
              lcdcon5:LCD5_FRM565 | LCD5_INVVLINE | LCD5_INVVFRAME | LCD5_HWSWP |
         LCD5_PWREN,
#endif
         },
};

void lutsetting(void);		//yongkal
void frcsetting(void);		//yongkal

extern struct clk *clk_get(struct device *dev, const char *id);
extern int clk_enable(struct clk *clk);
extern void clk_put(struct clk *clk);
extern void clk_disable(struct clk *clk);
extern unsigned long clk_get_rate(struct clk *clk);

void lutsetting(void)		//yongkal
{
	__raw_writel(0x01820200 ,S3C2413_REDLUT0);
	__raw_writel(0x203860A1 ,S3C2413_REDLUT1);
	__raw_writel(0xA3058A12 ,S3C2413_REDLUT2);
	__raw_writel(0x224078E1 ,S3C2413_REDLUT3);
	__raw_writel(0x62A50992 ,S3C2413_REDLUT4);
	__raw_writel(0x9A3260B9 ,S3C2413_REDLUT5);
	__raw_writel(0xF9E3A70D ,S3C2413_REDLUT6);

	
	__raw_writel(0x80C10100 ,S3C2413_GREENLUT0);
	__raw_writel(0x101C3050 ,S3C2413_GREENLUT1);
	__raw_writel(0xD182C509 ,S3C2413_GREENLUT2);
	__raw_writel(0x11203C70 ,S3C2413_GREENLUT3);
	__raw_writel(0xB15284C9 ,S3C2413_GREENLUT4);
	__raw_writel(0xCD19305C ,S3C2413_GREENLUT5);
	__raw_writel(0x7CF1D386 ,S3C2413_GREENLUT6);
	__raw_writel(0x88D12140 ,S3C2413_GREENLUT7);
	__raw_writel(0x509D3254 ,S3C2413_GREENLUT8);
	__raw_writel(0xD58AD529 ,S3C2413_GREENLUT9);
	__raw_writel(0x3160BD72 ,S3C2413_GREENLUT10);
	__raw_writel(0xB3568CD9 ,S3C2413_GREENLUT11);
	__raw_writel(0xDD3970DD ,S3C2413_GREENLUT12);
	__raw_writel(0xFDF3D78E ,S3C2413_GREENLUT13);
 
	__raw_writel(0x01820200 ,S3C2413_BLUELUT0);
	__raw_writel(0x203860A1 ,S3C2413_BLUELUT1);
	__raw_writel(0xA3058A12 ,S3C2413_BLUELUT2);
	__raw_writel(0x224078E1 ,S3C2413_BLUELUT3);
	__raw_writel(0x62A50992 ,S3C2413_BLUELUT4);
	__raw_writel(0x9A3260B9 ,S3C2413_BLUELUT5);
	__raw_writel(0xF9E3A70D ,S3C2413_BLUELUT6);
}
void frcsetting(void)		//yongkal
{
	__raw_writel(0x00000000, S3C2413_FRCPAT0);
	__raw_writel(0x80000000, S3C2413_FRCPAT1);
	__raw_writel(0x80000000, S3C2413_FRCPAT2);
	__raw_writel(0x80000000, S3C2413_FRCPAT3);
	__raw_writel(0x00080000, S3C2413_FRCPAT4);
	__raw_writel(0x80000200, S3C2413_FRCPAT5);
	__raw_writel(0x80008000, S3C2413_FRCPAT6);
	__raw_writel(0x80008000, S3C2413_FRCPAT7);
	__raw_writel(0x00000000, S3C2413_FRCPAT8);
	__raw_writel(0x80000000, S3C2413_FRCPAT9);
	__raw_writel(0x00000000, S3C2413_FRCPAT10);
	__raw_writel(0x80000000, S3C2413_FRCPAT11);
	__raw_writel(0x00000000, S3C2413_FRCPAT12);
	__raw_writel(0x80000000, S3C2413_FRCPAT13);
	__raw_writel(0x80808080, S3C2413_FRCPAT14);
	__raw_writel(0x80808080, S3C2413_FRCPAT15);
	__raw_writel(0x00000000, S3C2413_FRCPAT16);
	__raw_writel(0x81000000, S3C2413_FRCPAT17);
	__raw_writel(0x00000000, S3C2413_FRCPAT18);
	__raw_writel(0x82000000, S3C2413_FRCPAT19);
	__raw_writel(0x00000000, S3C2413_FRCPAT20);
	__raw_writel(0x82000000, S3C2413_FRCPAT21);
	__raw_writel(0x84208420, S3C2413_FRCPAT22);
	__raw_writel(0x84208420, S3C2413_FRCPAT23);
	__raw_writel(0x00000000, S3C2413_FRCPAT24);
	__raw_writel(0x84000000, S3C2413_FRCPAT25);
	__raw_writel(0x00000000, S3C2413_FRCPAT26);
	__raw_writel(0x88000000, S3C2413_FRCPAT27);
	__raw_writel(0x00000000, S3C2413_FRCPAT28);
	__raw_writel(0x88800000, S3C2413_FRCPAT29);
	__raw_writel(0x88888888, S3C2413_FRCPAT30);
	__raw_writel(0x88888888, S3C2413_FRCPAT31);
	__raw_writel(0x00000000, S3C2413_FRCPAT32);
	__raw_writel(0x88880000, S3C2413_FRCPAT33);
	__raw_writel(0x00000000, S3C2413_FRCPAT34);
	__raw_writel(0x91200000, S3C2413_FRCPAT35);
	__raw_writel(0x00000000, S3C2413_FRCPAT36);
	__raw_writel(0x92000000, S3C2413_FRCPAT37);
	__raw_writel(0xA248A248, S3C2413_FRCPAT38);
	__raw_writel(0xA248A248, S3C2413_FRCPAT39);
	__raw_writel(0x00000000, S3C2413_FRCPAT40);
	__raw_writel(0x92000000, S3C2413_FRCPAT41);
	__raw_writel(0x00000000, S3C2413_FRCPAT42);
	__raw_writel(0x92928000, S3C2413_FRCPAT43);
	__raw_writel(0x00000000, S3C2413_FRCPAT44);
	__raw_writel(0x92480000, S3C2413_FRCPAT45);
	__raw_writel(0xA8A8A8A8, S3C2413_FRCPAT46);
	__raw_writel(0xA8A8A8A8, S3C2413_FRCPAT47);
	__raw_writel(0x00000000, S3C2413_FRCPAT48);
	__raw_writel(0x94A00000, S3C2413_FRCPAT49);
	__raw_writel(0x00000000, S3C2413_FRCPAT50);
	__raw_writel(0xA5000000, S3C2413_FRCPAT51);
	__raw_writel(0x00000000, S3C2413_FRCPAT52);
	__raw_writel(0xA5200000, S3C2413_FRCPAT53);
	__raw_writel(0xAA54AA54, S3C2413_FRCPAT54);
	__raw_writel(0xAA54AA54, S3C2413_FRCPAT55);
	__raw_writel(0x00000000, S3C2413_FRCPAT56);
	__raw_writel(0xAA800000, S3C2413_FRCPAT57);
	__raw_writel(0x00000000, S3C2413_FRCPAT58);
	__raw_writel(0xAA800000, S3C2413_FRCPAT59);
	__raw_writel(0x00000000, S3C2413_FRCPAT60);
	__raw_writel(0xAAAA8000, S3C2413_FRCPAT61);
	__raw_writel(0xAAAAAAAA, S3C2413_FRCPAT62);
	__raw_writel(0xAAAAAAAA, S3C2413_FRCPAT63);
}

void elfinfb_lcd_on(int on)
{
	struct clk *clock;
	unsigned long val;

#ifdef CONFIG_ARCH_SMDK2413
	clock = clk_get(NULL, "gpio");
	clk_enable(clock);
	clk_put(clock);

	__raw_writel(0xaaaa56a9, S3C2413_GPCCON);				//yongkal
	__raw_writel(0xffff, S3C2413_GPCDN);				//yongkal
	__raw_writel(0xaaaaaaaa, S3C2413_GPDCON);				//yongkal
	__raw_writel(0xffff, S3C2413_GPDDN);				//yongkal

	s3c2413_gpio_cfgpin(S3C2413_GPG4, S3C2413_GPG4_OUTP); //S3C2440_GPG4_LCDPWREN ??
	s3c2413_gpio_pullup(S3C2413_GPG4, 1);

	s3c2413_gpio_cfgpin(S3C2413_GPB0, S3C2413_GPB0_OUTP); //BACKLIGHT_PWM yongkal
	s3c2413_gpio_pullup(S3C2413_GPB0, 1);
	s3c2413_gpio_cfgpin(S3C2413_GPG5, S3C2413_GPG4_OUTP); //nDIS_OFF=Enable CSTN yongkal
	s3c2413_gpio_pullup(S3C2413_GPG5, 1);
	
	clock = clk_get(NULL, "lcd");
	clk_disable(clock);

	val = __raw_readl(S3C2413_LCDCON1);
	val &= ~S3C2413_LCDCON1_ENVID;	/* LCD control signal OFF */
	__raw_writel(val, S3C2413_LCDCON1);

	val = __raw_readl(S3C2413_GPGDAT);
	val &= ~(1 << 4);
	__raw_writel(val, S3C2413_GPGDAT);

	if(on){
	   __raw_writel(__raw_readl(S3C2413_GPBDAT)|(1<<0), S3C2413_GPBDAT);		//yongkal

	   __raw_writel(__raw_readl(S3C2413_GPGDAT)|(1<<5), S3C2413_GPGDAT);		//yongkal

	   __raw_writel(__raw_readl(S3C2413_GPGDAT)|(1<<4), S3C2413_GPGDAT);		//modify by yongkal

	   val = __raw_readl(S3C2413_LCDCON1);
	   val |= S3C2413_LCDCON1_ENVID;	/* LCD control signal ON */
	   __raw_writel(val, S3C2413_LCDCON1);

	   mdelay(200);            /* to give for the stable time with DCDC  */
	   clk_enable(clock);
	}
	clk_put(clock);
#endif
}

/*
 * Complain if VAR is out of range.
 */
#define DEBUG_VAR 1

#include "s3c2413fb.h"

static inline u_int palette_pbs(struct fb_var_screeninfo *var)
{
        int ret = 0;
        switch (var->bits_per_pixel) {
//#ifdef FBCON_HAS_CFB4
        case 4:
                ret = 0 << 12;
                break;
//#endif
//#ifdef FBCON_HAS_CFB8
        case 8:
                ret = 1 << 12;
                break;
//#endif
//#ifdef FBCON_HAS_CFB16
        case 16:
                ret = 2 << 12;
                break;
//#endif
        }
        return ret;
}

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
        chan &= 0xffff;
        chan >>= 16 - bf->length;
        return chan << bf->offset;
}

static int
elfinfb_setpalettereg(u_int regno, u_int red, u_int green, u_int blue,
                        u_int trans, struct fb_info *info)
{
        struct elfinfb_info *fbi = (struct elfinfb_info *) info;
        u_int val, ret = 1;
                                                                                           
        if (regno < fbi->palette_size) {
                val = ((blue >> 16) & 0x001f);
                val |= ((green >> 11) & 0x07e0);
                val |= ((red >> 5) & 0x0f800);
                val |= 1;       /* intensity bit */
                if (regno == 0)
                        val |= palette_pbs(&fbi->fb.var);
                                                                                           
                fbi->palette_cpu[regno] = val;
                ret = 0;
        }
        return ret;
}

static int
elfinfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		   u_int trans, struct fb_info *info)
{
	struct elfinfb_info *fbi = (struct elfinfb_info *)info;
	unsigned int val;
	int ret = 1;

	/*
	 * If inverse mode was selected, invert all the colours
	 * rather than the register number.  The register number
	 * is what you poke into the framebuffer to produce the
	 * colour you requested.
	 */
	if (fbi->cmap_inverse) {
		red   = 0xffff - red;
		green = 0xffff - green;
		blue  = 0xffff - blue;
	}

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (fbi->fb.var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
					7471 * blue) >> 16;

	switch (fbi->fb.fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/*
		 * 16-bit True Colour.  We encode the RGB value
		 * according to the RGB bitfield information.
		 */
		if (regno < 16) {
			u32 *pal = fbi->fb.pseudo_palette;

			val  = chan_to_field(red, &fbi->fb.var.red);
			val |= chan_to_field(green, &fbi->fb.var.green);
			val |= chan_to_field(blue, &fbi->fb.var.blue);

			pal[regno] = val;
			ret = 0;
		}
		break;

	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		ret = elfinfb_setpalettereg(regno, red, green, blue, trans, info);
		break;
	}

	return ret;
}

/*
 *  elfinfb_check_var():
 *    Get the video params out of 'var'. If a value doesn't fit, round it up,
 *    if it's too big, return -EINVAL.
 *
 *    Round up in the following order: bits_per_pixel, xres,
 *    yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 *    bitfields, horizontal timing, vertical timing.
 */
static int elfinfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct elfinfb_info *fbi = (struct elfinfb_info *)info;

	if (var->xres < MIN_XRES)
		var->xres = MIN_XRES;
	if (var->yres < MIN_YRES)
		var->yres = MIN_YRES;
	if (var->xres > fbi->max_xres)
		var->xres = fbi->max_xres;
	if (var->yres > fbi->max_yres)
		var->yres = fbi->max_yres;
	var->xres_virtual =
		max(var->xres_virtual, var->xres);
	var->yres_virtual =
		max(var->yres_virtual, var->yres);

        /*
	 * Setup the RGB parameters for this display.
         */
	if (var->bits_per_pixel == 16) {
		var->red.offset   = 11; var->red.length   = 5;
		var->green.offset = 5;  var->green.length = 6;
		var->blue.offset  = 0;  var->blue.length  = 5;
		var->transp.offset = var->transp.length = 0;
	} else {
		var->red.offset = var->green.offset = var->blue.offset = var->transp.offset = 0;
		var->red.length   = 8;
		var->green.length = 8;
		var->blue.length  = 8;
		var->transp.length = 0;
	}

	return 0;
}

static int elfinfb_activate_var(struct fb_var_screeninfo *var,
				  struct elfinfb_info *fbi)
{
	struct clk *hclk;
	struct elfinfb_lcd_reg new_regs;
	u_int half_screen_size, yres;
	u_long flags, hclk_freq;

	unsigned long val, VideoPhysicalTemp = fbi->screen_dma;

	hclk = clk_get(NULL, "hclk");
	hclk_freq = clk_get_rate(hclk);
	clk_put(hclk);

	local_irq_save(flags);

	lutsetting();
	frcsetting();

	new_regs.lcdcon1 = fbi->reg.lcdcon1 & ~S3C2413_LCDCON1_ENVID;
//	new_regs.lcdcon1 |= LCD1_CLKVAL((int)((hclk_freq / LCD_PIXEL_CLOCK/2) - 1));
//	new_regs.lcdcon1 |= LCD1_CLKVAL((int)(hclk_freq/LCD_PIXEL_CLOCK));		//yongkal
	new_regs.lcdcon1 |= S3C2413_LCDCON1_CLKVAL(35);		//yongkal

	new_regs.lcdcon2 = (fbi->reg.lcdcon2 & ~LCD2_LINEVAL_MSK)
	    | S3C2413_LCDCON2_LINEVAL(var->yres);	//yongkal

	/* TFT LCD only ! */
	new_regs.lcdcon3 = (fbi->reg.lcdcon3 & ~LCD3_HOZVAL_MSK)
	    | S3C2413_LCDCON3_HOZVAL(var->xres);	//yongkal

	new_regs.lcdcon4 = fbi->reg.lcdcon4;
	new_regs.lcdcon5 = fbi->reg.lcdcon5;
	new_regs.lcdcon6 = fbi->reg.lcdcon6;			//yongkal
	new_regs.lcdcon8 = fbi->reg.lcdcon8;			//yongkal

	new_regs.lcdsaddr1 =
	    S3C2413_LCDBANK(((unsigned long) VideoPhysicalTemp >> 22))
	    | S3C2413_LCDBASEU(((unsigned long) VideoPhysicalTemp >> 1));		//modified by yongkal

	/* 16bpp */
	new_regs.lcdsaddr2 = S3C2413_LCDBASEL(((unsigned long)
					    VideoPhysicalTemp +
					    (var->xres * 2 * (var->yres))) >> 1);				//modified by yongkal

	new_regs.lcdsaddr3 = S3C2413_OFFSIZE(0) | (S3C2413_PAGEWIDTH(var->xres));

	yres = var->yres;

	half_screen_size = var->bits_per_pixel;
        half_screen_size = half_screen_size * var->xres * var->yres / 16;
                                                                                           
        fbi->reg.lcdcon1 = new_regs.lcdcon1;
        fbi->reg.lcdcon2 = new_regs.lcdcon2;
        fbi->reg.lcdcon3 = new_regs.lcdcon3;
        fbi->reg.lcdcon4 = new_regs.lcdcon4;
        fbi->reg.lcdcon5 = new_regs.lcdcon5;
        fbi->reg.lcdcon6 = new_regs.lcdcon6;			//yongkal	
        fbi->reg.lcdcon8 = new_regs.lcdcon8;			//yongkal
        fbi->reg.lcdsaddr1 = new_regs.lcdsaddr1;
        fbi->reg.lcdsaddr2 = new_regs.lcdsaddr2;
        fbi->reg.lcdsaddr3 = new_regs.lcdsaddr3;
                                                                                           
	__raw_writel(fbi->reg.lcdcon1, S3C2413_LCDCON1);
	__raw_writel(fbi->reg.lcdcon2, S3C2413_LCDCON2);
	__raw_writel(fbi->reg.lcdcon3, S3C2413_LCDCON3);
	__raw_writel(fbi->reg.lcdcon4, S3C2413_LCDCON4);
	__raw_writel(fbi->reg.lcdcon5, S3C2413_LCDCON5);
	__raw_writel(fbi->reg.lcdcon6, S3C2413_LCDCON6);	//yongkal
	__raw_writel(fbi->reg.lcdcon8, S3C2413_LCDCON8);	//yongkal
	__raw_writel(fbi->reg.lcdsaddr1, S3C2413_LCDSADDR1);
	__raw_writel(fbi->reg.lcdsaddr2, S3C2413_LCDSADDR2);
	__raw_writel(fbi->reg.lcdsaddr3, S3C2413_LCDSADDR3);

#if 0
	val = __raw_readl(S3C2413_LPCSEL);
	val &= ~0x7;
	val |= (1<<4);
	__raw_writel(val, S3C2413_LPCSEL);
	__raw_writel(0, S3C2413_TPAL);
#endif													//yongkal why???

	val = __raw_readl(S3C2413_LCDCON1);
	val |= S3C2413_LCDCON1_ENVID;	/* LCD control signal ON */
	__raw_writel(val, S3C2413_LCDCON1);

	local_irq_restore(flags);
    
	return 0;
}

/*
 * elfinfb_set_par():
 *	Set the user defined part of the display for the specified console
 */
static int elfinfb_set_par(struct fb_info *info)
{
	struct elfinfb_info *fbi = (struct elfinfb_info *)info;
	struct fb_var_screeninfo *var = &info->var;
	unsigned long palette_mem_size;

	if (var->bits_per_pixel == 16)
		fbi->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	else if (!fbi->cmap_static)
		fbi->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else {
		/*
		 * Some people have weird ideas about wanting static
		 * pseudocolor maps.  I suspect their user space
		 * applications are broken.
		 */
		fbi->fb.fix.visual = FB_VISUAL_STATIC_PSEUDOCOLOR;
	}

	fbi->fb.fix.line_length = var->xres_virtual *
				  var->bits_per_pixel / 8;
	fbi->palette_size = var->bits_per_pixel == 8 ? 256 : 16;

	palette_mem_size = fbi->palette_size * sizeof(u16);

	fbi->palette_cpu = (u16 *)(fbi->map_cpu + PAGE_SIZE - palette_mem_size);
	fbi->palette_dma = fbi->map_dma + PAGE_SIZE - palette_mem_size;
/* jassi
	if (fbi->fb.var.bits_per_pixel == 16)
		fb_dealloc_cmap(&fbi->fb.cmap);
	else
		fb_alloc_cmap(&fbi->fb.cmap, 1<<fbi->fb.var.bits_per_pixel, 0);
*/
	elfinfb_activate_var(var, fbi);

	return 0;
}

/*
 * Formal definition of the VESA spec:
 *  On
 *  	This refers to the state of the display when it is in full operation
 *  Stand-By
 *  	This defines an optional operating state of minimal power reduction with
 *  	the shortest recovery time
 *  Suspend
 *  	This refers to a level of power management in which substantial power
 *  	reduction is achieved by the display.  The display can have a longer
 *  	recovery time from this state than from the Stand-by state
 *  Off
 *  	This indicates that the display is consuming the lowest level of power
 *  	and is non-operational. Recovery from this state may optionally require
 *  	the user to manually power on the monitor
 *
 *  Now, the fbdev driver adds an additional state, (blank), where they
 *  turn off the video (maybe by colormap tricks), but don't mess with the
 *  video itself: think of it semantically between on and Stand-By.
 *
 *  So here's what we should do in our fbdev blank routine:
 *
 *  	VESA_NO_BLANKING (mode 0)	Video on,  front/back light on
 *  	VESA_VSYNC_SUSPEND (mode 1)  	Video on,  front/back light off
 *  	VESA_HSYNC_SUSPEND (mode 2)  	Video on,  front/back light off
 *  	VESA_POWERDOWN (mode 3)		Video off, front/back light off
 *
 *  This will match the matrox implementation.
 */

/*
 * elfinfb_blank():
 *	Blank the display by setting all palette values to zero.  Note, the
 * 	16 bpp mode does not really use the palette, so this will not
 *      blank the display in all modes.
 */
static int elfinfb_blank(int blank, struct fb_info *info)
{
	return 0;
}

static struct fb_ops elfinfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= elfinfb_check_var,
	.fb_set_par	= elfinfb_set_par,
	.fb_setcolreg	= elfinfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_blank	= elfinfb_blank,
	.fb_cursor	= soft_cursor,
};

#ifdef CONFIG_PM
/*
 * Power management hooks.  Note that we won't be called from IRQ context,
 * unlike the blank functions above, so we may sleep.
 */
static int elfinfb_suspend(struct device *dev, u32 state, u32 level)
{
	return -1;
}

static int elfinfb_resume(struct device *dev, u32 level)
{
	return -1;
}
#else
#endif

/*
 * elfinfb_map_video_memory():
 *      Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *      allow palette and pixel writes to occur without flushing the
 *      cache.  Once this area is remapped, all virtual memory
 *      access to the video memory should occur at the new region.
 */
static int __init elfinfb_map_video_memory(struct elfinfb_info *fbi)	//OK
{
	/*
	 * We reserve one page for the palette, plus the size
	 * of the framebuffer.
	 */
	fbi->map_size = PAGE_ALIGN(fbi->fb.fix.smem_len + PAGE_SIZE);
	fbi->map_cpu = dma_alloc_writecombine(fbi->dev, fbi->map_size,
					      &fbi->map_dma, GFP_KERNEL);

	if (fbi->map_cpu) {
		/* prevent initial garbage on screen */
		memset(fbi->map_cpu, 0, fbi->map_size);
		fbi->fb.screen_base = fbi->map_cpu + PAGE_SIZE;
		fbi->screen_dma = fbi->map_dma + PAGE_SIZE;
		/*
		 * FIXME: this is actually the wrong thing to place in
		 * smem_start.  But fbdev suffers from the problem that
		 * it needs an API which doesn't exist (in this case,
		 * dma_writecombine_mmap)
		 */
		fbi->fb.fix.smem_start = fbi->screen_dma;
	}

	return fbi->map_cpu ? 0 : -ENOMEM;
}

static struct elfinfb_mach_info *__init
elfinfb_get_machine_info(struct elfinfb_info *fbi)
{
	struct elfinfb_mach_info *inf = NULL;

	inf = &lcd_std_info;
	fbi->reg = inf->reg;
	fbi->rgb[RGB_16] = &xxx_tft_rgb_16;

	return inf;
}

static struct fb_monspecs monspecs __initdata = {
	.hfmin = 30000, 
	.hfmax = 70000,
	.vfmin = 50,
	.vfmax = 65,
};

static struct elfinfb_info * __init elfinfb_init_fbinfo(struct device *dev)	
{
	struct elfinfb_info *fbi;
	void *addr;
	struct elfinfb_mach_info *inf ;

	/* Alloc the elfinfb_info and pseudo_palette in one step */
	fbi = kmalloc(sizeof(struct elfinfb_info) + sizeof(u32) * 16, GFP_KERNEL);
	if (!fbi)
		return NULL;

	memset(fbi, 0, sizeof(struct elfinfb_info));
	fbi->dev = dev;

	strcpy(fbi->fb.fix.id, "S3C2413");

	fbi->fb.fix.type	= FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux	= 0;
	fbi->fb.fix.xpanstep	= 0;
	fbi->fb.fix.ypanstep	= 0;
	fbi->fb.fix.ywrapstep	= 0;
	fbi->fb.fix.accel	= FB_ACCEL_NONE;

	fbi->fb.var.nonstd	= 0;
	fbi->fb.var.activate	= FB_ACTIVATE_NOW;
	fbi->fb.var.height	= -1;
	fbi->fb.var.width	= -1;
	fbi->fb.var.accel_flags	= 0;
	fbi->fb.var.vmode	= FB_VMODE_NONINTERLACED;

	fbi->fb.fbops		= &elfinfb_ops;
	fbi->fb.flags		= FBINFO_DEFAULT;
	fbi->fb.monspecs        = monspecs;
	fbi->fb.node		= -1;

	addr = fbi;
	addr = addr + sizeof(struct elfinfb_info);
	fbi->fb.pseudo_palette	= addr;

	fbi->rgb[RGB_8]		= &rgb_8;
	fbi->rgb[RGB_16]	= &def_rgb_16;

	inf = elfinfb_get_machine_info(fbi);

	fbi->max_xres			= inf->xres;
	fbi->fb.var.xres		= inf->xres;
	fbi->fb.var.xres_virtual	= inf->xres;
	fbi->max_yres			= inf->yres;
	fbi->fb.var.yres		= inf->yres;
	fbi->fb.var.yres_virtual	= inf->yres;
	fbi->max_bpp			= inf->bpp;
	fbi->fb.var.bits_per_pixel	= inf->bpp;
	fbi->fb.var.pixclock		= inf->pixclock;
	fbi->fb.var.hsync_len		= inf->hsync_len;
	fbi->fb.var.left_margin		= inf->left_margin;
	fbi->fb.var.right_margin	= inf->right_margin;
	fbi->fb.var.vsync_len		= inf->vsync_len;
	fbi->fb.var.upper_margin	= inf->upper_margin;
	fbi->fb.var.lower_margin	= inf->lower_margin;
	fbi->fb.var.sync		= inf->sync;
	fbi->cmap_inverse		= inf->cmap_inverse;
	fbi->cmap_static		= inf->cmap_static;
	fbi->fb.fix.smem_len		= fbi->max_xres * fbi->max_yres *
					  fbi->max_bpp / 8;
	return fbi;
}



#include "s3c2413_ltd222_lcd_init.c"
int __init elfinfb_probe(struct device *dev)
{
	struct elfinfb_info *fbi;
	int ret = -1;

	dev_dbg(dev, "elfinfb_probe\n");

	InitLDI();

	fbi = elfinfb_init_fbinfo(dev);
	if (!fbi) {
		goto failed;
	}

	/* Initialize video memory */
	ret = elfinfb_map_video_memory(fbi);
	if (ret) {
		goto failed;
	}

	/* enable LCD controller clock */
//	ret = request_irq(IRQ_LCD, elfinfb_handle_irq, SA_INTERRUPT, "LCD", fbi);
//	if (ret) {
//		goto failed;
//	}
//	disable_irq(IRQ_LCD);

	/*
	 * This makes sure that our colour bitfield
	 * descriptors are correctly initialised.
	 */
	elfinfb_check_var(&fbi->fb.var, &fbi->fb);
	//elfinfb_set_par(&fbi->fb);	ok? jassi

	dev_set_drvdata(dev, fbi);

	ret = register_framebuffer(&fbi->fb);
	if (ret < 0) {
		dev_err(dev, "Failed to register framebuffer device: %d\n", ret);
		goto failed;
	}
	printk("S3C2413 Framebuffer registered ! \n");

#if 0	//def CONFIG_CPU_FREQ
	fbi->freq_transition.notifier_call = sa1100fb_freq_transition;
	fbi->freq_policy.notifier_call = sa1100fb_freq_policy;
	cpufreq_register_notifier(&fbi->freq_transition, CPUFREQ_TRANSITION_NOTIFIER);
	cpufreq_register_notifier(&fbi->freq_policy, CPUFREQ_POLICY_NOTIFIER);
#endif

	/*
	 * Ok, now enable the LCD controller
	 */
	elfinfb_lcd_on(1);

	return 0;

failed:
	dev_set_drvdata(dev, NULL);
	if (fbi)
		kfree(fbi);
	return ret;
}

static struct device_driver elfinfb_driver = {
	.name		= "s3c2413-lcd",
	.bus		= &platform_bus_type,
	.probe		= elfinfb_probe,
#ifdef CONFIG_PM
	.suspend	= elfinfb_suspend,
	.resume		= elfinfb_resume,
#endif
};

int __devinit elfinfb_init(void)
{
	if(fb_get_options("elfinfb", NULL))
		return -ENODEV;

	return driver_register(&elfinfb_driver);
}

module_init(elfinfb_init);

MODULE_DESCRIPTION("loadable framebuffer driver for S3C2440");
MODULE_LICENSE("GPL");
