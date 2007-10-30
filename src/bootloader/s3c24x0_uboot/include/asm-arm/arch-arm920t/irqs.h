/*
 * Copyright (C) SAMSUNG MOBILE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * History
 *
 * 2002-05-20 : 
 *    	- Initial code
 * 2003-10-27 : SW.LEE <hitchcar@sec.samsung.com>
 * 	- add camera register
 */

#include <config.h>

#ifndef __S3C24X0_IRQS_H_
#define __S3C24X0_IRQS_H_

/* Interrupt Controller */
#define IRQ_EINT0		0	/* External interrupt 0 */
#define IRQ_EINT1		1	/* External interrupt 1 */
#define IRQ_EINT2		2	/* External interrupt 2 */
#define IRQ_EINT3		3	/* External interrupt 3 */
#define IRQ_EINT4_7		4	/* External interrupt 4 ~ 7 */
#define IRQ_EINT8_23		5	/* External interrupt 8 ~ 23 */
#define IRQ_CAM                 6   /* Only for S3C2440  */
#define IRQ_BAT_FLT		7
#define IRQ_TICK		8	/* RTC time tick interrupt  */
#ifdef CONFIG_S3C2440A
#define IRQ_WDT_AC97		9	/* S3C2440A watch-dog & AC97 dound interrupt */
#else
#define IRQ_WDT			9	/* Watch-Dog timer interrupt */
#endif
#define IRQ_TIMER0		10	/* Timer 0 interrupt */
#define IRQ_TIMER1		11	/* Timer 1 interrupt */
#define IRQ_TIMER2		12	/* Timer 2 interrupt */
#define IRQ_TIMER3		13	/* Timer 3 interrupt */
#define IRQ_TIMER4		14	/* Timer 4 interrupt */
#define IRQ_UART2		15	/* UART 2 interrupt  */
#define IRQ_LCD			16	/* reserved for future use */
#define IRQ_DMA0		17	/* DMA channel 0 interrupt */
#define IRQ_DMA1		18	/* DMA channel 1 interrupt */
#define IRQ_DMA2		19	/* DMA channel 2 interrupt */
#define IRQ_DMA3		20	/* DMA channel 3 interrupt */
#define IRQ_SDI			21	/* SD Interface interrupt */
#define IRQ_SPI0		22	/* SPI interrupt */
#define IRQ_UART1		23	/* UART1 receive interrupt */
#define IRQ_NFCON               24  /* Only For S3C2440 NAND interrupt */
#define IRQ_USBD		25	/* USB device interrupt */
#define IRQ_USBH		26	/* USB host interrupt */
#define IRQ_IIC			27	/* IIC interrupt */
#define IRQ_UART0		28	/* UART0 transmit interrupt */
#define IRQ_SPI1		29	/* UART1 transmit interrupt */
#define IRQ_RTC			30	/* RTC alarm interrupt */
#define IRQ_ADCTC		31	/* ADC EOC interrupt */

#endif

