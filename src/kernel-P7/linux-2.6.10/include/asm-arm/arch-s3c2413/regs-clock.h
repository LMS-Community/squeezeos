/* linux/include/asm/arch-s3c2413/regs-clock.h
 *
 * Copyright (c) 2003,2004 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2413 clock register definitions
 *
 *  Changelog:
 *    18-Aug-2004 Ben Dooks      Added 2413 definitions
 *    08-Aug-2004 Herbert Pötzl  Added CLKCON definitions
 *    19-06-2003  Ben Dooks      Created file
 *    12-03-2004  Ben Dooks      Updated include protection
 *    29-Sep-2004 Ben Dooks	 Fixed usage for assembly inclusion
 *    10-Feb-2005 Ben Dooks	 Fixed CAMDIVN address (Guillaume Gourat)
 */

#ifndef __ASM_ARM_REGS_CLOCK
#define __ASM_ARM_REGS_CLOCK "$Id: regs-clock.h,v 1.2 2005/12/14 06:11:13 yongkal Exp $"

#define S3C2413_CLKREG(x) ((x) + S3C2413_VA_CLKPWR)

#define S3C2413_PLLVAL(_m,_p,_s) ((_m) << 12 | ((_p) << 4) | ((_s)))

#define S3C2413_LOCKTIME    S3C2413_CLKREG(0x00)
#define S3C2413_MPLLCON	    S3C2413_CLKREG(0x04)
#define S3C2413_UPLLCON	    S3C2413_CLKREG(0x08)
#define S3C2413_CLKCON	    S3C2413_CLKREG(0x0C)
#define S3C2413_CLKSLOW     S3C2413_CLKREG(0x10)
#define S3C2413_CLKDIVN	    S3C2413_CLKREG(0x14)

#define S3C2413_OSCSET	    S3C2413_CLKREG(0x18)
#define S3C2413_CLKSRC	    S3C2413_CLKREG(0x1c)
#define S3C2413_PWRCFG  	S3C2413_CLKREG(0x24)
#define S3C2413_ENDIAN  	S3C2413_CLKREG(0x2c)
#define S3C2413_SWRSTCON  	S3C2413_CLKREG(0x30)
#define S3C2413_RSTCON  	S3C2413_CLKREG(0x34)
#define S3C2413_INFORM0  	S3C2413_CLKREG(0x70)
#define S3C2413_INFORM1  	S3C2413_CLKREG(0x74)
#define S3C2413_INFORM2  	S3C2413_CLKREG(0x78)
#define S3C2413_INFORM3  	S3C2413_CLKREG(0x7c)

#define S3C2413_CLKCON_POWER	(1<<3) //?

#define S3C2413_CLKCON_DMA0	     (1<<0)
#define S3C2413_CLKCON_DMA1	     (1<<1)
#define S3C2413_CLKCON_DMA2	     (1<<2)
#define S3C2413_CLKCON_DMA3	     (1<<3)

#define S3C2413_CLKCON_NAND	     (1<<4)
#define S3C2413_CLKCON_LCDC	     (1<<5)
#define S3C2413_CLKCON_USBH	     (1<<6)

#define S3C2413_CLKCON_SDRAM     (1<<8)
#define S3C2413_CLKCON_HCLKx2     (1<<9)
#define S3C2413_CLKCON_HCLKx1_2     (1<<10)

#define S3C2413_CLKCON_USBD48M     (1<<11)
#define S3C2413_CLKCON_USBH48M     (1<<12)

#define S3C2413_CLKCON_IISCLK	     (1<<13)
#define S3C2413_CLKCON_UART	     (1<<14)
#define S3C2413_CLKCON_CAMIF     (1<<15)
#define S3C2413_CLKCON_USBD	     (1<<16)
#define S3C2413_CLKCON_PWM	     (1<<17)
#define S3C2413_CLKCON_SDI	     (1<<18)

#define S3C2413_CLKCON_UART0	     (1<<19)
#define S3C2413_CLKCON_UART1	     (1<<20)
#define S3C2413_CLKCON_UART2	     (1<<21)
#define S3C2413_CLKCON_GPIO		     (1<<22)
#define S3C2413_CLKCON_RTC		     (1<<23)
#define S3C2413_CLKCON_ADC		     (1<<24)
#define S3C2413_CLKCON_IIC		     (1<<25)
#define S3C2413_CLKCON_IIS		     (1<<26)
#define S3C2413_CLKCON_SPI		     (1<<27)
#define S3C2413_CLKCON_WDT		     (1<<28)

#define S3C2413_PLLCON_MDIVSHIFT     12
#define S3C2413_PLLCON_PDIVSHIFT     4
#define S3C2413_PLLCON_SDIVSHIFT     0
#define S3C2413_PLLCON_MDIVMASK	     ((1<<(1+(19-12)))-1)
#define S3C2413_PLLCON_PDIVMASK	     ((1<<5)-1)
#define S3C2413_PLLCON_SDIVMASK	     3


#define S3C2413_PWRCFG_BATF_CFG_INT	(1<<0)
#define S3C2413_PWRCFG_BATF_CFG_IGNORE	(2<<0)
#define S3C2413_PWRCFG_BATF_CFG_SLEEP	(3<<0)

#define S3C2413_PWRCFG_STANDBYWFI_IDLE	(1<<6)
#define S3C2413_PWRCFG_STANDBYWFI_STOP	(2<<6)
#define S3C2413_PWRCFG_STANDBYWFI_SLEEP	(3<<6)
#define S3C2413_PWRCFG_STANDBYWFI_MASK	(3<<6)


/* DCLKCON register addresses in gpio.h */

#define S3C2413_DCLKCON_DCLK0EN	     (1<<0)
#define S3C2413_DCLKCON_DCLK0_PCLK   (0<<1)
#define S3C2413_DCLKCON_DCLK0_UCLK   (1<<1)
#define S3C2413_DCLKCON_DCLK0_DIV(x) (((x) - 1 )<<4)
#define S3C2413_DCLKCON_DCLK0_CMP(x) (((x) - 1 )<<8)

#define S3C2413_DCLKCON_DCLK1EN	     (1<<16)
#define S3C2413_DCLKCON_DCLK1_PCLK   (0<<17)
#define S3C2413_DCLKCON_DCLK1_UCLK   (1<<17)
#define S3C2413_DCLKCON_DCLK1_DIV(x) (((x) - 1) <<20)

#define S3C2413_CLKDIVN_HDIVN	     (1<<1)

#ifndef __ASSEMBLY__

static inline unsigned long
s3c2413_get_pll(unsigned long  pllval, unsigned long baseclk)
{
	unsigned long  mdiv, pdiv, sdiv;

	mdiv = (pllval & (0xff << 12))>>12;
	pdiv = (pllval & (0x3f << 4))>>4;
	sdiv = (pllval & (0x03 << 0))>>0;
	return (baseclk * (mdiv + 8)) / ((pdiv + 2) << sdiv);
}

#endif /* __ASSEMBLY__ */

#ifdef CONFIG_CPU_S3C2413

/* extra registers */

//#define S3C2413_CLKCON_CAMERA        (1<<19)
//#define S3C2413_CLKCON_AC97          (1<<20)

#define S3C2413_CLKDIVN_ARMDIV_MASK   (1<<3)
#define S3C2413_CLKDIVN_HALFHCLK   (1<<5)
#define S3C2413_CLKDIVN_PDIVN	     (1<<2)
#define S3C2413_CLKDIVN_HDIVN_MASK   (3<<0)
#define S3C2413_CLKDIVN_HDIVN_1      (0<<0)
#define S3C2413_CLKDIVN_HDIVN_2      (1<<0)
#define S3C2413_CLKDIVN_HDIVN_4_8    (2<<0)
#define S3C2413_CLKDIVN_HDIVN_3_6    (3<<0)
//#define S3C2413_CLKDIVN_UCLK         (1<<3)

//#define S3C2413_CAMDIVN_CAMCLK_MASK  (0xf<<0)
//#define S3C2413_CAMDIVN_CAMCLK_SEL   (1<<4)
//#define S3C2413_CAMDIVN_HCLK3_HALF   (1<<8)
//#define S3C2413_CAMDIVN_HCLK4_HALF   (1<<9)
//#define S3C2413_CAMDIVN_DVSEN        (1<<12)

#endif /* CONFIG_CPU_S3C2413 */


#endif /* __ASM_ARM_REGS_CLOCK */
