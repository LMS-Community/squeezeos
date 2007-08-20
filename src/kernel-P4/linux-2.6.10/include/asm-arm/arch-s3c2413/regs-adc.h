/* linux/include/asm/arch-s3c2413/regs-adc.h
 *
 * This program is free software; yosu can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2413 ADC registers
 *
*/

#ifndef __ASM_ARCH_REGS_ADC_H
#define __ASM_ARCH_REGS_ADC_H "regs-adc.h"

#define S3C2413_ADCREG(x) (S3C2413_VA_ADCTS + x)

#define S3C2413_ADCCON	   S3C2413_ADCREG(0x00)
#define S3C2413_ADCTSC	   S3C2413_ADCREG(0x04)
#define S3C2413_ADCDLY	   S3C2413_ADCREG(0x08)
#define S3C2413_ADCDAT0	   S3C2413_ADCREG(0x0C)
#define S3C2413_ADCDAT1	   S3C2413_ADCREG(0x10)
//#define S3C2413_ADCUPDN	   S3C2413_ADCREG(0x14)

/* ADCCON Register Bits */
#define S3C2413_ADCCON_ECFLG		(1<<15)
#define S3C2413_ADCCON_PRSCEN		(1<<14)
#define S3C2413_ADCCON_PRSCVL(x)	(((x)&0xFF)<<6)
#define S3C2413_ADCCON_PRSCVLMASK	(0xFF<<6)
#define S3C2413_ADCCON_SELMUX(x)	(((x)&0x7)<<3)
#define S3C2413_ADCCON_MUXMASK		(0x7<<3)
#define S3C2413_ADCCON_STDBM		(1<<2)
#define S3C2413_ADCCON_READ_START	(1<<1)
#define S3C2413_ADCCON_ENABLE_START	(1<<0)
#define S3C2413_ADCCON_STARTMASK	(0x3<<0)


/* ADCTSC Register Bits */
#define S3C2413_ADCTSC_YM_SEN		(1<<7)
#define S3C2413_ADCTSC_YP_SEN		(1<<6)
#define S3C2413_ADCTSC_XM_SEN		(1<<5)
#define S3C2413_ADCTSC_XP_SEN		(1<<4)
#define S3C2413_ADCTSC_PULL_UP_DISABLE	(1<<3)
#define S3C2413_ADCTSC_AUTO_PST		(1<<2)
#define S3C2413_ADCTSC_XY_PST(x)	(((x)&0x3)<<0)

/* ADCDAT0 Bits */
#define S3C2413_ADCDAT0_UPDOWN		(1<<15)
#define S3C2413_ADCDAT0_AUTO_PST	(1<<14)
#define S3C2413_ADCDAT0_XY_PST		(0x3<<12)
#define S3C2413_ADCDAT0_XPDATA_MASK	(0x03FF)

/* ADCDAT1 Bits */
#define S3C2413_ADCDAT1_UPDOWN		(1<<15)
#define S3C2413_ADCDAT1_AUTO_PST	(1<<14)
#define S3C2413_ADCDAT1_XY_PST		(0x3<<12)
#define S3C2413_ADCDAT1_YPDATA_MASK	(0x03FF)

/* ADC and Touch Screen Interface */
#if 0
/* Registers */
#define ADCCON			bADC_CTL(oADCCON)
#define ADCTSC			bADC_CTL(oADCTSC)
#define ADCDLY			bADC_CTL(oADCDLY)
#define ADCDAT0			bADC_CTL(oADCDAT0)
#define ADCDAT1			bADC_CTL(oADCDAT1)
#define PEN_UPDN		bADC_CTL(oADC_UPDN)
/* ADCUPDN register set */
#endif
#define ADC_TSC_UP		(1 << 1)  /* 0: No stylus up state 1: Stylus up state */
#define ADC_TSC_DN              (1 << 0)  /* 0: No Stylus down status 1:Style down status*/  
/* Field */
#define fADCCON_PRSCVL		Fld(8, 6)
#define fADCCON_INPUT		Fld(3, 3)
#define fTSC_XY_PST		Fld(2, 0)
#define fADC_DELAY		Fld(6, 0)
#define fDAT_UPDOWN		Fld(1, 15)
#define fDAT_AUTO_PST		Fld(1, 14)
#define fDAT_XY_PST		Fld(2, 12)
#define fPST_DATA               Fld(10, 0) 
#define fDAT_XPDATA		Fld(10, 0)
#define fDAT_YPDATA		Fld(10, 0)

#define PST_DAT_MSK     0x3FF
#define PST_DAT_VAL(x)  (FExtr(x, fPST_DATA) & PST_DAT_MSK)
#define XPDATA          PST_DAT_VAL(ADCDAT0)
#define YPDATA          PST_DAT_VAL(ADCDAT1)
                                                                                
/* ... */
#define ADC_IN0                 0
#define ADC_IN1                 1
#define ADC_IN2                 2
#define ADC_IN3                 3
#define ADC_IN4                 4
#define ADC_IN5                 5
#define ADC_IN6                 6
#define ADC_IN7                 7
#define ADC_BUSY		1
#define ADC_READY		0
#define NOP_MODE		0
#define X_AXIS_MODE		1
#define Y_AXIS_MODE		2
#define WAIT_INT_MODE		3
/* ... */
#define ADCCON_ECFLG		(1 << 15)
#define PRESCALE_ENDIS		(1 << 14)
#define PRESCALE_DIS		(PRESCALE_ENDIS*0)
#define PRESCALE_EN		(PRESCALE_ENDIS*1)
#define PRSCVL(x)		(x << 6)
#define ADC_INPUT(x)		(x << 3)
#define ADCCON_STDBM		(1 << 2)        /* 1: standby mode, 0: normal mode */
#define ADC_NORMAL_MODE		FClrBit(ADCCON, ADCCON_STDBM)
#define ADC_STANDBY_MODE	(ADCCON_STDBM*1)
#define ADCCON_READ_START	(1 << 1)
#define ADC_START_BY_RD_DIS	FClrBit(ADCCON, ADCCON_READ_START)
#define ADC_START_BY_RD_EN	(ADCCON_READ_START*1)
#define ADC_START		(1 << 0)
#define ADC_START_NOP           FClrBit(ADCCON, ADC_START) 
#define ADC_START_START         (ADC_START*1) 

#define UD_SEN			(1 << 8)
#define DOWN_INT		(UD_SEN*0)
#define UP_INT			(UD_SEN*1)
#define YM_SEN			(1 << 7)
#define YM_HIZ			(YM_SEN*0)
#define YM_GND			(YM_SEN*1)
#define YP_SEN			(1 << 6)
#define YP_EXTVLT		(YP_SEN*0)
#define YP_AIN			(YP_SEN*1)
#define XM_SEN			(1 << 5)
#define XM_HIZ			(XM_SEN*0)
#define XM_GND			(XM_SEN*1)
#define XP_SEN			(1 << 4)
#define XP_EXTVLT		(XP_SEN*0)
#define XP_AIN			(XP_SEN*1)
#define XP_PULL_UP		(1 << 3)
#define XP_PULL_UP_EN		(XP_PULL_UP*0)
#define XP_PULL_UP_DIS		(XP_PULL_UP*1)
#define AUTO_PST		(1 << 2)
#define CONVERT_MAN		(AUTO_PST*0)
#define CONVERT_AUTO		(AUTO_PST*1)
#define XP_PST(x)		(x << 0)

#endif /* __ASM_ARCH_REGS_ADC_H */


