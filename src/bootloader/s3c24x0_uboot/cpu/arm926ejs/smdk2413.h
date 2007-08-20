/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * $Date: 2006/02/13 10:21:33 $
 * $Revision: 1.4 $
 *
 */
#include <config.h>

#if defined (CONFIG_S3C2413)
#include <asm/arch/s3c2413.h>
#ifndef S3C2413_SMDK_H
#define S3C2413_SMDK_H

#define MACH_TYPE      	(887)
#define UBOOT_MAGIC    	(0x43090000 | MACH_TYPE)
#define CONFIG_SERIAL_DEFAULT_BAUDRATE 115200

#if defined (CONFIG_S3C2411)
#define SDRAM_BANK_CFG	(MDRAM_RasBW0(MDRAM_RASBW_13bit) | MDRAM_RasBW1(MDRAM_RASBW_13bit) | \
						MDRAM_CasBW0(MDRAM_CASBW_9bit) | MDRAM_CasBW1(MDRAM_CASBW_9bit) | \
						MDRAM_AddrCFG0(BA_RAS_CAS) | MDRAM_AddrCFG1(BA_RAS_CAS) | \
						mSDRAM | MDRAM_BW_16bit)
#else
#define SDRAM_BANK_CFG	(MDRAM_RasBW0(MDRAM_RASBW_13bit) | MDRAM_RasBW1(MDRAM_RASBW_13bit) | \
						MDRAM_CasBW0(MDRAM_CASBW_9bit) | MDRAM_CasBW1(MDRAM_CASBW_9bit) | \
						MDRAM_AddrCFG0(BA_RAS_CAS) | MDRAM_AddrCFG1(BA_RAS_CAS) | \
						mSDRAM | MDRAM_BW_32bit)
#endif
#define SDRAM_BANK_VAL1 (MDRAM_BStop_CON(BStop_OFF) | MDRAM_WBUF_CON(WBUF_ENABLE) | \
						MDRAM_AP_CON(AP_DISABLE) | ~MDRAM_PWRDN)	
#define SDRAM_BANK_VAL2 (MDRAM_Tras(6) | MDRAM_Trc(8)| \
                        MDRAM_Cl(3) | MDRAM_Trcd(3) | MDRAM_Trp(3))
#define SDRAM_BANK_VAL3	(MDRAM_BA_EMRS(2) | (0<<21) | (0<<16) | \
						MDRAM_BA_MRS(0) | (0<<7) | MDRAM_Cl_MRS(3))
						
#define DDRAM_BANK_CFG	(MDRAM_RasBW0(MDRAM_RASBW_13bit) | MDRAM_RasBW1(MDRAM_RASBW_13bit) | \
						MDRAM_CasBW0(MDRAM_CASBW_10bit) | MDRAM_CasBW1(MDRAM_CASBW_10bit) | \
						MDRAM_AddrCFG0(BA_RAS_CAS) | MDRAM_AddrCFG1(BA_RAS_CAS) | \
						mDDRAM | MDRAM_BW_16bit)
#define DDRAM_BANK_VAL1 ((1<<30) | (1<<26) | MDRAM_BStop_CON(BStop_OFF) | \
						MDRAM_WBUF_CON(WBUF_ENABLE) | MDRAM_AP_CON(AP_DISABLE) | ~MDRAM_PWRDN)	
#define DDRAM_BANK_VAL2 (MDRAM_Tras(6) | MDRAM_Trc(8)| \
                        MDRAM_Cl(3) | MDRAM_Trcd(3) | MDRAM_Trp(3))
#define DDRAM_BANK_VAL3	(MDRAM_BA_EMRS(2) | (0<<21) | (0<<16) | \
						MDRAM_BA_MRS(0) | (0<<7) | MDRAM_Cl_MRS(3))

/* 133MHz 7.8us*HCLK=1037.4=0x40D */
//#define REFRESHRATES_VAL    0x40D
/* 130MHz 7.8us*HCLK=1014=0x3F6 */
//#define REFRESHRATES_VAL    0x3F6
/* 125MHz 7.8us*HCLK=975=0x3CF */
//#define REFRESHRATES_VAL    0x3CF
/* 120MHz 7.8us*HCLK=936=0x3A8 */
//#define REFRESHRATES_VAL    0x3A8
/* 110MHz 7.8us*HCLK=858=0x35A */
//#define REFRESHRATES_VAL    0x35A
/* 100MHz 7.8us*HCLK=780=0x30C */
#define REFRESHRATES_VAL    0x30C
/* 96MHz 7.8us*HCLK=748.8=0x2EC */
//#define REFRESHRATES_VAL    0x2EC

#define LockTime			0x0fff0fff

/* 266MHz */
//#define MPLL_VAL	    (PLL_MainDiv(125) | PLL_PreDiv(4) | PLL_PostDiv(1))
//#define UART_REF_CLK    (66500000)
/* 260MHz */
//#define MPLL_VAL    	(PLL_MainDiv(122) | PLL_PreDiv(4) | PLL_PostDiv(1))
//#define UART_REF_CLK    (65000000)
/* 250MHz */
//#define MPLL_VAL    	(PLL_MainDiv(117) | PLL_PreDiv(4) | PLL_PostDiv(1))
//#define UART_REF_CLK    (62500000)
/* 240MHz */
//#define MPLL_VAL	    (PLL_MainDiv(72) | PLL_PreDiv(2) | PLL_PostDiv(1))
//#define UART_REF_CLK    (60000000)
/* 220MHz */
//#define MPLL_VAL    	(PLL_MainDiv(47) | PLL_PreDiv(1) | PLL_PostDiv(1))
//#define UART_REF_CLK    (55000000)
/* 200MHz */
#define MPLL_VAL		(PLL_MainDiv(42) | PLL_PreDiv(1) | PLL_PostDiv(1))
#define UPLL_VAL		(PLL_MainDiv(64) | PLL_PreDiv(7) | PLL_PostDiv(0))
#define UART_REF_CLK	(50000000)
/* 192MHz */
//#define MPLL_VAL		(PLL_MainDiv(40) | PLL_PreDiv(1) | PLL_PostDiv(1))
//#define UART_REF_CLK	(48000000)

#define CLKDIV_VAL		(CLKDIV_USB48DIV | CLKDIV_HALFCLK | CLKDIV_ArmDiv(0) | \
						CLKDIV_PclkDiv(1) | CLKDIV_HclkDiv(1))	//HCLK/2, ARM:UCLK:PCLK = 1:2:4
//#define CLKDIV_VAL		(CLKDIV_USB48DIV | CLKDIV_HALFCLK | CLKDIV_ArmDiv(0) | \
						CLKDIV_PclkDiv(0) | CLKDIV_HclkDiv(3))	//HCLK/2, ARM:UCLK:PCLK = 1:4:4

#endif
#endif		/* THE END OF S3C2413 */
