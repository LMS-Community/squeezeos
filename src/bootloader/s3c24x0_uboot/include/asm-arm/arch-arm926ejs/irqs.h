#ifndef __S3C24A0A_IRQ_H_
#define __S3C24A0A_IRQ_H_

#include "../bits.h"

/* Interrupt controller */
#define IRQ_EINT0_2			(0)	/* External interrupt 0 ~ 2 */
#define IRQ_EINT3_6			(1)	/* External interrupt 3 ~ 6 */
#define IRQ_EINT7_10			(2)	/* External interrupt 7 ~ 10 */
#define IRQ_EINT11_14			(3)	/* External interrupt 11 ~ 14 */
#define IRQ_EINT15_18			(4)	/* External interrupt 15 ~ 18 */
#define IRQ_TIC				(5)	/* RTC time tick */
#define IRQ_DCTQ			(6)	/* DCTQ */
#define IRQ_MC				(7)	/* MC */
#define IRQ_ME				(8)	/* ME */
#define IRQ_KEYPAD			(9)	/* Keypad */
#define IRQ_TIMER0			(10)	/* Timer 0 */
#define IRQ_TIMER1			(11)	/* Timer 1 */
#define IRQ_TIMER2			(12)	/* Timer 2 */  
#define IRQ_TIMER3_4			(13)	/* Timer 3, 4 */
#define IRQ_LCD_POST			(14)	/* LCD/POST */
#define IRQ_CAM_C			(15)	/* Camera Codec */
#define IRQ_WDT_BATFLT			(16)	/* WDT/BATFLT */
#define IRQ_UART0			(17)	/* UART 0 */
#define IRQ_CAM_P			(18)	/* Camera Preview */
#define IRQ_MODEM			(19)	/* Modem */
#define IRQ_DMA				(20)	/* DMA channels for S-bus */
#define IRQ_SDI				(21)	/* SDI MMC */
#define IRQ_SPI0			(22)	/* SPI 0 */
#define IRQ_UART1			(23)	/* UART 1 */
#define IRQ_AC97_NFLASH			(24)	/* AC97/NFALASH */
#define IRQ_USBH			(26)	/* USB host */
#define IRQ_IIC				(27)	/* IIC */
#define IRQ_IRDA_MSTICK			(28)	/* IrDA/MSTICK */
#define IRQ_VLX_SPI1			(29)	/* SPI 1 */
#define IRQ_RTC				(30)	/* RTC alaram */
#define IRQ_ADC_PENUPDN			(31)	/* ADC EOC/Pen up/Pen down */
#if defined(CONFIG_S3C2460x)
#define IRQ_USBD			(29)	/* USB device */
#else
#define IRQ_USBD			(25)	/* USB device */
#endif
#define BIT_USBD			BIT25

#endif

