/*
 * (C) Copyright 2003
 * David Müller ELSOFT AG Switzerland. d.mueller@elsoft.ch
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/************************************************
 * NAME	    : s3c2440.h
 * Version  : 2004.
 *
 ************************************************/

#ifndef __S3C2440_H__
#define __S3C2440_H__

#define S3C24X0_UART_CHANNELS	3
#define S3C24X0_SPI_CHANNELS	2

/* S3C2440 only supports 512 Byte HW ECC */
#define S3C2440_ECCSIZE		512
#define S3C2440_ECCBYTES	3

typedef enum {
	S3C24X0_UART0,
	S3C24X0_UART1,
	S3C24X0_UART2
} S3C24X0_UARTS_NR;

/* S3C2440 device base addresses */
#define S3C24X0_MEMCTL_BASE		0x48000000
#define S3C24X0_USB_HOST_BASE		0x49000000
#define S3C24X0_INTERRUPT_BASE		0x4A000000
#define S3C24X0_DMA_BASE		0x4B000000
#define S3C24X0_CLOCK_POWER_BASE	0x4C000000
#define S3C24X0_LCD_BASE		0x4D000000
#define S3C2440_NAND_BASE		0x4E000000
#define S3C24X0_UART_BASE		0x50000000
#define S3C24X0_TIMER_BASE		0x51000000
#define S3C24X0_USB_DEVICE_BASE		0x52000140
#define USB_DEVICE_PHYS_ADR		0x52000000
#define S3C24X0_WATCHDOG_BASE		0x53000000
#define S3C24X0_I2C_BASE		0x54000000
#define S3C24X0_I2S_BASE		0x55000000
#define S3C24X0_GPIO_BASE		0x56000000
#define S3C24X0_RTC_BASE		0x57000000
#define S3C2440_ADC_BASE		0x58000000
#define S3C24X0_SPI_BASE		0x59000000
#define S3C2440_SDI_BASE		0x5A000000


/* include common stuff */
#include <s3c24x0.h>


static inline S3C24X0_MEMCTL * const S3C24X0_GetBase_MEMCTL(void)
{
	return (S3C24X0_MEMCTL * const)S3C24X0_MEMCTL_BASE;
}
static inline S3C24X0_USB_HOST * const S3C24X0_GetBase_USB_HOST(void)
{
	return (S3C24X0_USB_HOST * const)S3C24X0_USB_HOST_BASE;
}
static inline S3C24X0_INTERRUPT * const S3C24X0_GetBase_INTERRUPT(void)
{
	return (S3C24X0_INTERRUPT * const)S3C24X0_INTERRUPT_BASE;
}
static inline S3C24X0_DMAS * const S3C24X0_GetBase_DMAS(void)
{
	return (S3C24X0_DMAS * const)S3C24X0_DMA_BASE;
}
static inline S3C24X0_CLOCK_POWER * const S3C24X0_GetBase_CLOCK_POWER(void)
{
	return (S3C24X0_CLOCK_POWER * const)S3C24X0_CLOCK_POWER_BASE;
}
static inline S3C24X0_LCD * const S3C24X0_GetBase_LCD(void)
{
	return (S3C24X0_LCD * const)S3C24X0_LCD_BASE;
}
static inline S3C2440_NAND * const S3C2440_GetBase_NAND(void)
{
	return (S3C2440_NAND * const)S3C2440_NAND_BASE;
}
static inline S3C24X0_UART * const S3C24X0_GetBase_UART(S3C24X0_UARTS_NR nr)
{
	return (S3C24X0_UART * const)(S3C24X0_UART_BASE + (nr * 0x4000));
}
static inline S3C24X0_TIMERS * const S3C24X0_GetBase_TIMERS(void)
{
	return (S3C24X0_TIMERS * const)S3C24X0_TIMER_BASE;
}
static inline S3C24X0_USB_DEVICE * const S3C24X0_GetBase_USB_DEVICE(void)
{
	return (S3C24X0_USB_DEVICE * const)S3C24X0_USB_DEVICE_BASE;
}
static inline S3C24X0_WATCHDOG * const S3C24X0_GetBase_WATCHDOG(void)
{
	return (S3C24X0_WATCHDOG * const)S3C24X0_WATCHDOG_BASE;
}
static inline S3C24X0_I2C * const S3C24X0_GetBase_I2C(void)
{
	return (S3C24X0_I2C * const)S3C24X0_I2C_BASE;
}
static inline S3C24X0_I2S * const S3C24X0_GetBase_I2S(void)
{
	return (S3C24X0_I2S * const)S3C24X0_I2S_BASE;
}
static inline S3C24X0_GPIO * const S3C24X0_GetBase_GPIO(void)
{
	return (S3C24X0_GPIO * const)S3C24X0_GPIO_BASE;
}
static inline S3C24X0_RTC * const S3C24X0_GetBase_RTC(void)
{
	return (S3C24X0_RTC * const)S3C24X0_RTC_BASE;
}
#if 0
static inline S3C2440_ADC * const S3C2440_GetBase_ADC(void)
{
	return (S3C2440_ADC * const)S3C2440_ADC_BASE;
}
static inline S3C24X0_SPI * const S3C24X0_GetBase_SPI(void)
{
	return (S3C24X0_SPI * const)S3C24X0_SPI_BASE;
}
static inline S3C2440_SDI * const S3C2440_GetBase_SDI(void)
{
	return (S3C2440_SDI * const)S3C2440_SDI_BASE;
}
#endif


/* PENDING BIT */
#define BIT_EINT0		(0x1)
#define BIT_EINT1		(0x1<<1)
#define BIT_EINT2		(0x1<<2)
#define BIT_EINT3		(0x1<<3)
#define BIT_EINT4_7		(0x1<<4)
#define BIT_EINT8_23		(0x1<<5)
#define BIT_BAT_FLT		(0x1<<7)
#define BIT_TICK		(0x1<<8)
#define BIT_WDT			(0x1<<9)
#define BIT_TIMER0		(0x1<<10)
#define BIT_TIMER1		(0x1<<11)
#define BIT_TIMER2		(0x1<<12)
#define BIT_TIMER3		(0x1<<13)
#define BIT_TIMER4		(0x1<<14)
#define BIT_UART2		(0x1<<15)
#define BIT_LCD			(0x1<<16)
#define BIT_DMA0		(0x1<<17)
#define BIT_DMA1		(0x1<<18)
#define BIT_DMA2		(0x1<<19)
#define BIT_DMA3		(0x1<<20)
#define BIT_SDI			(0x1<<21)
#define BIT_SPI0		(0x1<<22)
#define BIT_UART1		(0x1<<23)
#define BIT_USBD		(0x1<<25)
#define BIT_USBH		(0x1<<26)
#define BIT_IIC			(0x1<<27)
#define BIT_UART0		(0x1<<28)
#define BIT_SPI1		(0x1<<29)
#define BIT_RTC			(0x1<<30)
#define BIT_ADC			(0x1<<31)
#define BIT_ALLMSK		(0xFFFFFFFF)


/* Memory control */
#define BWSCON			(*(volatile unsigned *)0x48000000)
#define BANKCON0		(*(volatile unsigned *)0x48000004)
#define BANKCON1		(*(volatile unsigned *)0x48000008)
#define BANKCON2		(*(volatile unsigned *)0x4800000C)
#define BANKCON3		(*(volatile unsigned *)0x48000010)
#define BANKCON4		(*(volatile unsigned *)0x48000014)
#define BANKCON5		(*(volatile unsigned *)0x48000018)
#define BANKCON6		(*(volatile unsigned *)0x4800001C)
#define BANKCON7		(*(volatile unsigned *)0x48000020)
#define REFRESH			(*(volatile unsigned *)0x48000024)
#define BANKSIZE		(*(volatile unsigned *)0x48000028)
#define MRSRB6			(*(volatile unsigned *)0x4800002C)
#define MRSRB7			(*(volatile unsigned *)0x48000030)


/* USB HOST */
#define rHcRevision		(*(volatile unsigned *)0x49000000)
#define rHcControl		(*(volatile unsigned *)0x49000004)
#define rHcCommonStatus		(*(volatile unsigned *)0x49000008)
#define rHcInterruptStatus	(*(volatile unsigned *)0x4900000C)
#define rHcInterruptEnable	(*(volatile unsigned *)0x49000010)
#define rHcInterruptDisable	(*(volatile unsigned *)0x49000014)
#define rHcHCCA			(*(volatile unsigned *)0x49000018)
#define rHcPeriodCuttendED	(*(volatile unsigned *)0x4900001C)
#define rHcControlHeadED	(*(volatile unsigned *)0x49000020)
#define rHcControlCurrentED	(*(volatile unsigned *)0x49000024)
#define rHcBulkHeadED		(*(volatile unsigned *)0x49000028)
#define rHcBuldCurrentED	(*(volatile unsigned *)0x4900002C)
#define rHcDoneHead		(*(volatile unsigned *)0x49000030)
#define rHcRmInterval		(*(volatile unsigned *)0x49000034)
#define rHcFmRemaining		(*(volatile unsigned *)0x49000038)
#define rHcFmNumber		(*(volatile unsigned *)0x4900003C)
#define rHcPeriodicStart	(*(volatile unsigned *)0x49000040)
#define rHcLSThreshold		(*(volatile unsigned *)0x49000044)
#define rHcRhDescriptorA	(*(volatile unsigned *)0x49000048)
#define rHcRhDescriptorB	(*(volatile unsigned *)0x4900004C)
#define rHcRhStatus		(*(volatile unsigned *)0x49000050)
#define rHcRhPortStatus1	(*(volatile unsigned *)0x49000054)
#define rHcRhPortStatus2	(*(volatile unsigned *)0x49000058)


/* INTERRUPT getfree uncommented rINTMSK 2003.12.05 */
#define SRCPND			(*(volatile unsigned *)0x4A000000)
#define INTMOD			(*(volatile unsigned *)0x4A000004)
#define INTMSK			(*(volatile unsigned *)0x4A000008)
#define PRIORITY		(*(volatile unsigned *)0x4A00000C)
#define INTPND			(*(volatile unsigned *)0x4A000010)
#define INTOFFSET		(*(volatile unsigned *)0x4A000014)
#define SUBSRCPND		(*(volatile unsigned *)0x4A000018)
#define INTSUBMSK		(*(volatile unsigned *)0x4A00001C)


/* DMA */
#define rDISRC0			(*(volatile unsigned *)0x4B000000)
#define rDISRCC0		(*(volatile unsigned *)0x4B000004)
#define rDIDST0			(*(volatile unsigned *)0x4B000008)
#define rDIDSTC0		(*(volatile unsigned *)0x4B00000C)
#define rDCON0			(*(volatile unsigned *)0x4B000010)
#define rDSTAT0			(*(volatile unsigned *)0x4B000014)
#define rDCSRC0			(*(volatile unsigned *)0x4B000018)
#define rDCDST0			(*(volatile unsigned *)0x4B00001C)
#define rDMASKTRIG0		(*(volatile unsigned *)0x4B000020)
#define rDISRC1			(*(volatile unsigned *)0x4B000040)
#define rDISRCC1		(*(volatile unsigned *)0x4B000044)
#define rDIDST1			(*(volatile unsigned *)0x4B000048)
#define rDIDSTC1		(*(volatile unsigned *)0x4B00004C)
#define rDCON1			(*(volatile unsigned *)0x4B000050)
#define rDSTAT1			(*(volatile unsigned *)0x4B000054)
#define rDCSRC1			(*(volatile unsigned *)0x4B000058)
#define rDCDST1			(*(volatile unsigned *)0x4B00005C)
#define rDMASKTRIG1		(*(volatile unsigned *)0x4B000060)
#define rDISRC2			(*(volatile unsigned *)0x4B000080)
#define rDISRCC2		(*(volatile unsigned *)0x4B000084)
#define rDIDST2			(*(volatile unsigned *)0x4B000088)
#define rDIDSTC2		(*(volatile unsigned *)0x4B00008C)
#define rDCON2			(*(volatile unsigned *)0x4B000090)
#define rDSTAT2			(*(volatile unsigned *)0x4B000094)
#define rDCSRC2			(*(volatile unsigned *)0x4B000098)
#define rDCDST2			(*(volatile unsigned *)0x4B00009C)
#define rDMASKTRIG2		(*(volatile unsigned *)0x4B0000A0)
#define rDISRC3			(*(volatile unsigned *)0x4B0000C0)
#define rDISRCC3		(*(volatile unsigned *)0x4B0000C4)
#define rDIDST3			(*(volatile unsigned *)0x4B0000C8)
#define rDIDSTC3		(*(volatile unsigned *)0x4B0000CC)
#define rDCON3			(*(volatile unsigned *)0x4B0000D0)
#define rDSTAT3			(*(volatile unsigned *)0x4B0000D4)
#define rDCSRC3			(*(volatile unsigned *)0x4B0000D8)
#define rDCDST3			(*(volatile unsigned *)0x4B0000DC)
#define rDMASKTRIG3		(*(volatile unsigned *)0x4B0000E0)


/* CLOCK & POWER MANAGEMENT */
#define LOCKTIME		(*(volatile unsigned *)0x4C000000)
#define MPLLCON			(*(volatile unsigned *)0x4C000004)
#define UPLLCON			(*(volatile unsigned *)0x4C000008)
#define CLKCON			(*(volatile unsigned *)0x4C00000C)
#define CLKSLOW			(*(volatile unsigned *)0x4C000010)
#define CLKDIVN			(*(volatile unsigned *)0x4C000014)
#define CAMDIVN			(*(volatile unsigned *)0x4C000018)

#define CLKCON_USBD		(1<<7)

#define fPLL_MDIV               Fld(8,12)
#define fPLL_PDIV               Fld(6,4)
#define fPLL_SDIV               Fld(2,0)


#define NFDATA8			(*(volatile unsigned char *)0x4E000010)
#define NFDATA16		(*(volatile unsigned short *)0x4E000010)
#define NFDATA32		(*(volatile unsigned *)0x4E000010)

#define NFCONF                  __REG(0x4E000000)
#define NFCONT                  __REG(0x4E000004)
#define NFCMD                  __REG(0x4E000008)
#define NFADDR                  __REGb(0x4E00000C)
#define NFMECCD0                __REG(0x4E000014)
#define NFMECCD1                __REG(0x4E000018)
#define NFSECCD                 __REG(0x4E00001C)
#define NFSTAT                  __REG(0x4E000020)
#define NFESTAT0                __REG(0x4E000024)
#define NFESTAT1                __REG(0x4E000028)
#define NFMECC0                 __REG(0x4E00002C)
#define NFMECC1                 __REG(0x4E000030)
#define NFSECC                  __REG(0x4E000034)
#define NFSBLK                  __REG(0x4E000038)



/* UART */
#define rULCON0			(*(volatile unsigned *)0x50000000)
#define rUCON0			(*(volatile unsigned *)0x50000004)
#define rUFCON0			(*(volatile unsigned *)0x50000008)
#define rUMCON0			(*(volatile unsigned *)0x5000000C)
#define rUTRSTAT0		(*(volatile unsigned *)0x50000010)
#define rUERSTAT0		(*(volatile unsigned *)0x50000014)
#define rUFSTAT0		(*(volatile unsigned *)0x50000018)
#define rUMSTAT0		(*(volatile unsigned *)0x5000001C)
#define rUBRDIV0		(*(volatile unsigned *)0x50000028)

#define rULCON1			(*(volatile unsigned *)0x50004000)
#define rUCON1			(*(volatile unsigned *)0x50004004)
#define rUFCON1			(*(volatile unsigned *)0x50004008)
#define rUMCON1			(*(volatile unsigned *)0x5000400C)
#define rUTRSTAT1		(*(volatile unsigned *)0x50004010)
#define rUERSTAT1		(*(volatile unsigned *)0x50004014)
#define rUFSTAT1		(*(volatile unsigned *)0x50004018)
#define rUMSTAT1		(*(volatile unsigned *)0x5000401C)
#define rUBRDIV1		(*(volatile unsigned *)0x50004028)

#define rULCON2			(*(volatile unsigned *)0x50008000)
#define rUCON2			(*(volatile unsigned *)0x50008004)
#define rUFCON2			(*(volatile unsigned *)0x50008008)
#define rUTRSTAT2		(*(volatile unsigned *)0x50008010)
#define rUERSTAT2		(*(volatile unsigned *)0x50008014)
#define rUFSTAT2		(*(volatile unsigned *)0x50008018)
#define rUBRDIV2		(*(volatile unsigned *)0x50008028)

#ifdef __BIG_ENDIAN
#define rUTXH0			(*(volatile unsigned char *)0x50000023)
#define rURXH0			(*(volatile unsigned char *)0x50000027)
#define rUTXH1			(*(volatile unsigned char *)0x50004023)
#define rURXH1			(*(volatile unsigned char *)0x50004027)
#define rUTXH2			(*(volatile unsigned char *)0x50008023)
#define rURXH2			(*(volatile unsigned char *)0x50008027)

#define WrUTXH0(ch)		(*(volatile unsigned char *)0x50000023)=(unsigned char)(ch)
#define RdURXH0()		(*(volatile unsigned char *)0x50000027)
#define WrUTXH1(ch)		(*(volatile unsigned char *)0x50004023)=(unsigned char)(ch)
#define RdURXH1()		(*(volatile unsigned char *)0x50004027)
#define WrUTXH2(ch)		(*(volatile unsigned char *)0x50008023)=(unsigned char)(ch)
#define RdURXH2()		(*(volatile unsigned char *)0x50008027)

#define UTXH0			(0x50000020+3)  /* byte_access address by DMA */
#define URXH0			(0x50000024+3)
#define UTXH1			(0x50004020+3)
#define URXH1			(0x50004024+3)
#define UTXH2			(0x50008020+3)
#define URXH2			(0x50008024+3)

#else /* Little Endian */
#define rUTXH0			(*(volatile unsigned char *)0x50000020)
#define rURXH0			(*(volatile unsigned char *)0x50000024)
#define rUTXH1			(*(volatile unsigned char *)0x50004020)
#define rURXH1			(*(volatile unsigned char *)0x50004024)
#define rUTXH2			(*(volatile unsigned char *)0x50008020)
#define rURXH2			(*(volatile unsigned char *)0x50008024)

#define WrUTXH0(ch)		(*(volatile unsigned char *)0x50000020)=(unsigned char)(ch)
#define RdURXH0()		(*(volatile unsigned char *)0x50000024)
#define WrUTXH1(ch)		(*(volatile unsigned char *)0x50004020)=(unsigned char)(ch)
#define RdURXH1()		(*(volatile unsigned char *)0x50004024)
#define WrUTXH2(ch)		(*(volatile unsigned char *)0x50008020)=(unsigned char)(ch)
#define RdURXH2()		(*(volatile unsigned char *)0x50008024)

#define UTXH0			(0x50000020)    /* byte_access address by DMA */
#define URXH0			(0x50000024)
#define UTXH1			(0x50004020)
#define URXH1			(0x50004024)
#define UTXH2			(0x50008020)
#define URXH2			(0x50008024)
#endif


/* PWM TIMER */
#define rTCFG0			(*(volatile unsigned *)0x51000000)
#define rTCFG1			(*(volatile unsigned *)0x51000004)
#define rTCON			(*(volatile unsigned *)0x51000008)
#define rTCNTB0			(*(volatile unsigned *)0x5100000C)
#define rTCMPB0			(*(volatile unsigned *)0x51000010)
#define rTCNTO0			(*(volatile unsigned *)0x51000014)
#define rTCNTB1			(*(volatile unsigned *)0x51000018)
#define rTCMPB1			(*(volatile unsigned *)0x5100001C)
#define rTCNTO1			(*(volatile unsigned *)0x51000020)
#define rTCNTB2			(*(volatile unsigned *)0x51000024)
#define rTCMPB2			(*(volatile unsigned *)0x51000028)
#define rTCNTO2			(*(volatile unsigned *)0x5100002C)
#define rTCNTB3			(*(volatile unsigned *)0x51000030)
#define rTCMPB3			(*(volatile unsigned *)0x51000034)
#define rTCNTO3			(*(volatile unsigned *)0x51000038)
#define rTCNTB4			(*(volatile unsigned *)0x5100003C)
#define rTCNTO4			(*(volatile unsigned *)0x51000040)


/* WATCH DOG TIMER */
#define rWTCON			(*(volatile unsigned *)0x53000000)
#define rWTDAT			(*(volatile unsigned *)0x53000004)
#define rWTCNT			(*(volatile unsigned *)0x53000008)


/* IIC */
#define rIICCON			(*(volatile unsigned *)0x54000000)
#define rIICSTAT		(*(volatile unsigned *)0x54000004)
#define rIICADD			(*(volatile unsigned *)0x54000008)
#define rIICDS			(*(volatile unsigned *)0x5400000C)


/* IIS */
#define rIISCON			(*(volatile unsigned *)0x55000000)
#define rIISMOD			(*(volatile unsigned *)0x55000004)
#define rIISPSR			(*(volatile unsigned *)0x55000008)
#define rIISFCON		(*(volatile unsigned *)0x5500000C)

#ifdef __BIG_ENDIAN
#define IISFIF			((volatile unsigned short *)0x55000012)
#else /*  little endian */
#define IISFIF			((volatile unsigned short *)0x55000010)
#endif


/* I/O PORT */
#define GPACON			(*(volatile unsigned *)0x56000000)
#define GPADAT			(*(volatile unsigned *)0x56000004)

#define GPBCON			(*(volatile unsigned *)0x56000010)
#define GPBDAT			(*(volatile unsigned *)0x56000014)
#define GPBUP			(*(volatile unsigned *)0x56000018)

#define GPCCON			(*(volatile unsigned *)0x56000020)
#define GPCDAT			(*(volatile unsigned *)0x56000024)
#define GPCUP			(*(volatile unsigned *)0x56000028)

#define GPDCON			(*(volatile unsigned *)0x56000030)
#define GPDDAT			(*(volatile unsigned *)0x56000034)
#define GPDUP			(*(volatile unsigned *)0x56000038)

#define GPECON			(*(volatile unsigned *)0x56000040)
#define GPEDAT			(*(volatile unsigned *)0x56000044)
#define GPEUP			(*(volatile unsigned *)0x56000048)

#define GPFCON			(*(volatile unsigned *)0x56000050)
#define GPFDAT			(*(volatile unsigned *)0x56000054)
#define GPFUP			(*(volatile unsigned *)0x56000058)

#define GPGCON			(*(volatile unsigned *)0x56000060)
#define GPGDAT			(*(volatile unsigned *)0x56000064)
#define GPGUP			(*(volatile unsigned *)0x56000068)

#define GPHCON			(*(volatile unsigned *)0x56000070)
#define GPHDAT			(*(volatile unsigned *)0x56000074)
#define GPHUP			(*(volatile unsigned *)0x56000078)

#define GPJCON			(*(volatile unsigned *)0x560000d0)
#define GPJDAT			(*(volatile unsigned *)0x560000d4)
#define GPJUP			(*(volatile unsigned *)0x560000d8)


#define MISCCR			(*(volatile unsigned *)0x56000080)
#define DCLKCON			(*(volatile unsigned *)0x56000084)
#define EXTINT0			(*(volatile unsigned *)0x56000088)
#define EXTINT1			(*(volatile unsigned *)0x5600008C)
#define EXTINT2			(*(volatile unsigned *)0x56000090)
#define EINTFLT0		(*(volatile unsigned *)0x56000094)
#define EINTFLT1		(*(volatile unsigned *)0x56000098)
#define EINTFLT2		(*(volatile unsigned *)0x5600009C)
#define EINTFLT3		(*(volatile unsigned *)0x560000A0)
#define EINTMASK		(*(volatile unsigned *)0x560000A4)
#define EINTPEND		(*(volatile unsigned *)0x560000A8)
#define GSTATUS0		(*(volatile unsigned *)0x560000AC)
#define GSTATUS1		(*(volatile unsigned *)0x560000B0)
#define GSTATUS2		(*(volatile unsigned *)0x560000B4)
#define GSTATUS3		(*(volatile unsigned *)0x560000B8)
#define GSTATUS4		(*(volatile unsigned *)0x560000BC)
#define DSC0			(*(volatile unsigned *)0x560000C4)
#define DSC1			(*(volatile unsigned *)0x560000C8)

/* RTC */
#ifdef __BIG_ENDIAN
#define rRTCCON			(*(volatile unsigned char *)0x57000043)
#define rTICNT			(*(volatile unsigned char *)0x57000047)
#define rRTCALM			(*(volatile unsigned char *)0x57000053)
#define rALMSEC			(*(volatile unsigned char *)0x57000057)
#define rALMMIN			(*(volatile unsigned char *)0x5700005B)
#define rALMHOUR		(*(volatile unsigned char *)0x5700005F)
#define rALMDATE		(*(volatile unsigned char *)0x57000063)
#define rALMMON			(*(volatile unsigned char *)0x57000067)
#define rALMYEAR		(*(volatile unsigned char *)0x5700006B)
#define rRTCRST			(*(volatile unsigned char *)0x5700006F)
#define rBCDSEC			(*(volatile unsigned char *)0x57000073)
#define rBCDMIN			(*(volatile unsigned char *)0x57000077)
#define rBCDHOUR		(*(volatile unsigned char *)0x5700007B)
#define rBCDDATE		(*(volatile unsigned char *)0x5700007F)
#define rBCDDAY			(*(volatile unsigned char *)0x57000083)
#define rBCDMON			(*(volatile unsigned char *)0x57000087)
#define rBCDYEAR		(*(volatile unsigned char *)0x5700008B)
#else /*  little endian */
#define rRTCCON			(*(volatile unsigned char *)0x57000040)
#define rTICNT			(*(volatile unsigned char *)0x57000044)
#define rRTCALM			(*(volatile unsigned char *)0x57000050)
#define rALMSEC			(*(volatile unsigned char *)0x57000054)
#define rALMMIN			(*(volatile unsigned char *)0x57000058)
#define rALMHOUR		(*(volatile unsigned char *)0x5700005C)
#define rALMDATE		(*(volatile unsigned char *)0x57000060)
#define rALMMON			(*(volatile unsigned char *)0x57000064)
#define rALMYEAR		(*(volatile unsigned char *)0x57000068)
#define rRTCRST			(*(volatile unsigned char *)0x5700006C)
#define rBCDSEC			(*(volatile unsigned char *)0x57000070)
#define rBCDMIN			(*(volatile unsigned char *)0x57000074)
#define rBCDHOUR		(*(volatile unsigned char *)0x57000078)
#define rBCDDATE		(*(volatile unsigned char *)0x5700007C)
#define rBCDDAY			(*(volatile unsigned char *)0x57000080)
#define rBCDMON			(*(volatile unsigned char *)0x57000084)
#define rBCDYEAR		(*(volatile unsigned char *)0x57000088)
#endif




#endif /*__S3C2440_H__*/
