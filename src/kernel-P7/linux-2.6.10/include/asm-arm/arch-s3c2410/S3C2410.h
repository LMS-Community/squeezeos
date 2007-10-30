/*
 * linux/include/asm-arm/arch-s3c2440/S3C2440.h
 *
 * Definition of constants related to the S3C2440 microprocessor
 * This file is based on the S3C2440 User Manual 2002,01,23.
 *
 * Copyright (C) 2003  Samsung Electronics 
 *
 * Oct 27 '03 Kwanghyun La <nala.la@samsung.com> from laputa
   - add register & bit set macro for timer1 TCFG1
 *
 * 2004-02-19 : SW.LEE <hitchcar@samsung.com>
   -- update for S3C2440A  
 *
 * 2004-02-23 : Kwanghyun La <laputa : nala.la@samsung.com> 
 * - added for AC97 of s3c2440a
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C2440_H_
#define _S3C2440_H_

#include "hardware.h"
#include "bitfield.h"

/*
 * Memory Controller (Page 5-15)
 *
 * Register
   BWSCON	Bus Width & Wait Status Control	[word, R/W, 0x000000]
   BANKCON0	boot ROM Control		[word, R/W, 0x0700]
   BANKCON1	BANK1 Control			[word, R/W, 0x0700]
   BANKCON2	BANK2 Control			[word, R/W, 0x0700]
   BANKCON3	BANK3 Control			[word, R/W, 0x0700]
   BANKCON4	BANK4 Control			[word, R/W, 0x0700]
   BANKCON5	BANK5 Control			[word, R/W, 0x0700]
   BANKCON6	BANK6 Control			[word, R/W, 0x1808]
   BANKCON7	BANK7 Control			[word, R/W, 0x1808]
   REFRESH	DRAM/SDRAM Refresh Control	[word, R/W, 0xac0000]
   BANKSIZE	Flexible Bank Size		[word, R/W, 0x0]
   MRSRB6	Mode register set for SDRAM	[word, R/W]
   MRSRB7	Mode register set for SDRAM	[word, R/W]
 *
 */

//#define bMEMCON(Nb)	__REG(0x48000000 + (Nb))	jassi
#define bMEMCON(Nb)	(S3C2440_VA_MEMCTRL + (Nb))
#define BWSCON		bMEMCON(0)
#define bBANKCON(Nb)	bMEMCON((Nb+1)*4)
#define BANKCON0	bBANKCON(0)
#define BANKCON1	bBANKCON(1)
#define BANKCON2	bBANKCON(2)
#define BANKCON3	bBANKCON(3)
#define BANKCON4	bBANKCON(4)
#define BANKCON5	bBANKCON(5)
#define BANKCON6	bBANKCON(6)
#define BANKCON7	bBANKCON(7)
#define REFRESH		bMEMCON(0x24)
#define BANKSIZE	bMEMCON(0x28)
#define MRSRB6		bMEMCON(0x2C)
#define MRSRB7		bMEMCON(0x30)

#define fBWSCON_ST(Nb)	Fld(1,((Nb)*4)+3)	/* Using UB/LB for Bank (Nb), init: 0 */
#define fBWSCON_WS(Nb)	Fld(1,((Nb)*4)+2)	/* WAIT enable for Bank (Nb), init: 0 */
#define fBWSCON_DW(Nb)	Fld(2,(Nb)*4)		/* data bus width for Bank (Nb), init: 0 */
#define fBWSCON_DW0	Fld(2,1)	/* initital state is undef */
#define BWSCON_ST7	FMsk(fBWSCON_ST(7))
#define BWSCON_WS7	FMsk(fBWSCON_WS(7))
#define BWSCON_DW7	FMsk(fBWSCON_DW(7))
#define BWSCON_ST6	FMsk(fBWSCON_ST(6))
#define BWSCON_WS6	FMsk(fBWSCON_WS(6))
#define BWSCON_DW6	FMsk(fBWSCON_DW(6))
#define BWSCON_ST5	FMsk(fBWSCON_ST(5))
#define BWSCON_WS5	FMsk(fBWSCON_WS(5))
#define BWSCON_DW5	FMsk(fBWSCON_DW(5))
#define BWSCON_ST4	FMsk(fBWSCON_ST(4))
#define BWSCON_WS4	FMsk(fBWSCON_WS(4))
#define BWSCON_DW4	FMsk(fBWSCON_DW(4))
#define BWSCON_ST3	FMsk(fBWSCON_ST(3))
#define BWSCON_WS3	FMsk(fBWSCON_WS(3))
#define BWSCON_DW3	FMsk(fBWSCON_DW(3))
#define BWSCON_ST2	FMsk(fBWSCON_ST(2))
#define BWSCON_WS2	FMsk(fBWSCON_WS(2))
#define BWSCON_DW2	FMsk(fBWSCON_DW(2))
#define BWSCON_ST1	FMsk(fBWSCON_ST(1))
#define BWSCON_WS1	FMsk(fBWSCON_WS(1))
#define BWSCON_DW1	FMsk(fBWSCON_DW(1))
#define BWSCON_DW0	FMsk(fBWSCON_DW0)
#define BWSCON_DW_8	0x0		/* set bus width to  8bit */
#define BWSCON_DW_16	0x1		/* set bus width to 16bit */
#define BWSCON_DW_32	0x2		/* set bus width to 32bit */
#define BWSCON_DW(x,y)	FInsrt((y), fBWSCON_DW(x))

#define BWSCON_DW3_16   FInsrt(0x01,fBWSCON_DW(3))
#define	fBANKCON_Tacs	Fld(2,13)	/* Address set-up before nBCSn, init: 0 */
#define	BANKCON_Tacs	FMsk(fBANKCON_Tacs)
#define BANKCON_Tacs0	FInsrt(0x0, fBANKCON_Tacs)	/* 0 clock */
#define BANKCON_Tacs1	FInsrt(0x1, fBANKCON_Tacs)	/* 1 clock */
#define BANKCON_Tacs2	FInsrt(0x2, fBANKCON_Tacs)	/* 2 clock */
#define BANKCON_Tacs4	FInsrt(0x3, fBANKCON_Tacs)	/* 4 clock */

#define	fBANKCON_Tcos	Fld(2,11)	/* Chip selection set-up nOE, init: 0 */
#define	BANKCON_Tcos	FMsk(fBANKCON_Tcos)
#define BANKCON_Tcos0	FInsrt(0x0, fBANKCON_Tcos)	/* 0 clock */
#define BANKCON_Tcos1	FInsrt(0x1, fBANKCON_Tcos)	/* 1 clock */
#define BANKCON_Tcos2	FInsrt(0x2, fBANKCON_Tcos)	/* 2 clock */
#define BANKCON_Tcos4	FInsrt(0x3, fBANKCON_Tcos)	/* 4 clock */

#define	fBANKCON_Tacc	Fld(3,8)	/* Access cycle, init: 0x7 */
#define BANKCON_Tacc	FMsk(fBANKCON_Tacc)
#define BANKCON_Tacc1	FInsrt(0x0, fBANKCON_Tacc)	/* 1 clock */
#define BANKCON_Tacc2	FInsrt(0x1, fBANKCON_Tacc)	/* 2 clock */
#define BANKCON_Tacc3	FInsrt(0x2, fBANKCON_Tacc)	/* 3 clock */
#define BANKCON_Tacc4	FInsrt(0x3, fBANKCON_Tacc)	/* 4 clock */
#define BANKCON_Tacc6	FInsrt(0x4, fBANKCON_Tacc)	/* 6 clock */
#define BANKCON_Tacc8	FInsrt(0x5, fBANKCON_Tacc)	/* 8 clock */
#define BANKCON_Tacc10	FInsrt(0x6, fBANKCON_Tacc)	/* 10 clock */
#define BANKCON_Tacc14	FInsrt(0x7, fBANKCON_Tacc)	/* 14 clock */

#define	fBANKCON_Toch	Fld(2,6)	/* Chip selection hold on nOE, init: 0 */
#define	BANKCON_Toch	FMsk(fBANKCON_Toch)
#define BANKCON_Toch0	FInsrt(0x0, fBANKCON_Toch)	/* 0 clock */
#define BANKCON_Toch1	FInsrt(0x1, fBANKCON_Toch)	/* 1 clock */
#define BANKCON_Toch2	FInsrt(0x2, fBANKCON_Toch)	/* 2 clock */
#define BANKCON_Toch4	FInsrt(0x3, fBANKCON_Toch)	/* 4 clock */

#define	fBANKCON_Tcah	Fld(2,4)	/* Address holding time after nBCSn, init: 0 */
#define	BANKCON_Tcah	FMsk(fBANKCON_Tcah)
#define BANKCON_Tcah0	FInsrt(0x0, fBANKCON_Tcah)	/* 0 clock */
#define BANKCON_Tcah1	FInsrt(0x1, fBANKCON_Tcah)	/* 1 clock */
#define BANKCON_Tcah2	FInsrt(0x2, fBANKCON_Tcah)	/* 2 clock */
#define BANKCON_Tcah4	FInsrt(0x3, fBANKCON_Tcah)	/* 4 clock */

#define	fBANKCON_Tacp	Fld(2,2)	/* Page mode access cycle @ Page mode, init: 0 */
#define	BANKCON_Tacp	FMsk(fBANKCON_Tacp)
#define BANKCON_Tacp2	FInsrt(0x0, fBANKCON_Tacp)	/* 2 clock */
#define BANKCON_Tacp3	FInsrt(0x1, fBANKCON_Tacp)	/* 3 clock */
#define BANKCON_Tacp4	FInsrt(0x2, fBANKCON_Tacp)	/* 4 clock */
#define BANKCON_Tacp6	FInsrt(0x3, fBANKCON_Tacp)	/* 6 clock */

#define	fBANKCON_PMC	Fld(2,0)	/* Page mode configuration, init: 0 */
#define	BANKCON_PMC	FMsk(fBANKCON_PMC)
#define BANKCON_PMC1	FInsrt(0x0, fBANKCON_PMC)	/* normal (1 data) */
#define BANKCON_PMC4	FInsrt(0x1, fBANKCON_PMC)	/* 4 data */
#define BANKCON_PMC8	FInsrt(0x2, fBANKCON_PMC)	/* 8 data */
#define BANKCON_PMC16	FInsrt(0x3, fBANKCON_PMC)	/* 16 data */

#define fBANKCON_MT	Fld(2,15)	/* memory type for BANK6 and BANK7 */
#define BANKCON_MT	FMsk(fBANKCON_MT)
#define BANKCON_MT_ROM	FInsrt(0x0, fBANKCON_MT)	/* ROM or SRAM */
#define BANKCON_MT_EDO	FInsrt(0x2, fBANKCON_MT)	/* EDO DRAM */
#define BANKCON_MT_SDRM	FInsrt(0x3, fBANKCON_MT)	/* Sync. DRAM */

#define fBANKCON_Trcd	Fld(2,4)	/* RAS to CAS delay, init: 0 */
#define BANKCON_Trcd	FMsk(fBANKCON_Trcd)
#define BANKCON_Trcd1	FInsrt(0x0, fBANKCON_Trcd)	/* 1 clock */
#define BANKCON_Trcd2	FInsrt(0x1, fBANKCON_Trcd)	/* 2 clock */
#define BANKCON_Trcd3	FInsrt(0x2, fBANKCON_Trcd)	/* 3 clock */
#define BANKCON_Trcd4	FInsrt(0x3, fBANKCON_Trcd)	/* 4 clock */

#define fBANKCON_Tcas	Fld(1,3)	/* CAS pulse width, init: 0 */
#define BANKCON_Tcas	FMsk(fBANKCON_Tcas)
#define BANKCON_Tcas1	FInsrt(0x0, fBANKCON_Tcas)	/* 1 clock */
#define BANKCON_Tcas2	FInsrt(0x1, fBANKCON_Tcas)	/* 2 clock */

#define fBANKCON_Tcp	Fld(1,2)	/* CAS pre-charge, init: 0 */
#define BANKCON_Tcp	FMsk(fBANKCON_Tcp)
#define BANKCON_Tcp1	FInsrt(0x0, fBANKCON_Tcp)	/* 1 clock */
#define BANKCON_Tcp2	FInsrt(0x1, fBANKCON_Tcp)	/* 2 clock */

#define fBANKCON_CAN	Fld(2,0)	/* Column address number, init: 0 */
#define BANKCON_CAN	FMsk(fBANKCON_CAN)
#define BANKCON_CAN8	FInsrt(0x0, fBANKCON_CAN)	/* 8-bit */
#define BANKCON_CAN9	FInsrt(0x1, fBANKCON_CAN)	/* 9-bit */
#define BANKCON_CAN10	FInsrt(0x2, fBANKCON_CAN)	/* 10-bit */
#define BANKCON_CAN11	FInsrt(0x3, fBANKCON_CAN)	/* 11-bit */

#define fBANKCON_STrcd	Fld(2,2)	/* RAS to CAS delay, init: 0x2 */
#define BANKCON_STrcd	FMsk(fBANKCON_STrcd)
#define BANKCON_STrcd2	FInsrt(0x0, fBANKCON_STrcd)	/* 2 clock */
#define BANKCON_STrcd3	FInsrt(0x1, fBANKCON_STrcd)	/* 3 clock */
#define BANKCON_STrcd4	FInsrt(0x2, fBANKCON_STrcd)	/* 4 clock */

#define fBANKCON_SCAN	Fld(2,0)	/* Column address number, init: 0 */
#define BANKCON_SCAN	FMsk(fBANKCON_SCAN)
#define BANKCON_SCAN8	FInsrt(0x0, fBANKCON_SCAN)	/* 8-bit */
#define BANKCON_SCAN9	FInsrt(0x1, fBANKCON_SCAN)	/* 9-bit */
#define BANKCON_SCAN10	FInsrt(0x2, fBANKCON_SCAN)	/* 10-bit */

#define REFRESH_REFEN	(1 << 23)	/* DRAM/SDRAM Refresh Enable, init: 0x1 */

#define REFRESH_TREFMD	(1 << 22)	/* DRAM/SDRAM Refresh Mode, init: 0 */
#define REFRESH_TREFMD_Auto	(0 << 22)	/* CBR/Auto Refresh */
#define REFRESH_TREFMD_Self	(1 << 22)	/* Self Refresh */

#define fREFRESH_Trp	Fld(2,20)	/* DRAM/SDRAM RAS pre-charge, init: 0x2 */
#define REFRESH_Trp	FMsk(fREFRESH_Trp)
#define REFRESH_Trp15	FInsrt(0x0, fBANKCON_Trp)	/* DRAM : 1.5 clocks */
#define REFRESH_Trp25	FInsrt(0x1, fBANKCON_Trp)	/* DRAM : 2.5 clocks */
#define REFRESH_Trp35	FInsrt(0x2, fBANKCON_Trp)	/* DRAM : 3.5 clocks */
#define REFRESH_Trp45	FInsrt(0x3, fBANKCON_Trp)	/* DRAM : 4.5 clocks */
#define REFRESH_Trp2	FInsrt(0x0, fBANKCON_Trp)	/* SDRAM : 2 clocks */
#define REFRESH_Trp3	FInsrt(0x1, fBANKCON_Trp)	/* SDRAM : 3 clocks */
#define REFRESH_Trp4	FInsrt(0x2, fBANKCON_Trp)	/* SDRAM : 4 clocks */

#define fREFRESH_Trc	Fld(2,18)	/* SDRAM RC minimum time, init: 0x3 */
#define REFRESH_Trc	FMsk(fREFRESH_Trc)
#define REFRESH_Trc4	FInsrt(0x0, fBANKCON_Trc)	/* 4 clocks */
#define REFRESH_Trc5	FInsrt(0x1, fBANKCON_Trc)	/* 5 clocks */
#define REFRESH_Trc6	FInsrt(0x2, fBANKCON_Trc)	/* 6 clocks */
#define REFRESH_Trc7	FInsrt(0x3, fBANKCON_Trc)	/* 7 clocks */

#define fREFRESH_Tchr	Fld(2,18)	/* DRAM CAS hold time, init: 0 */
#define REFRESH_Tchr	FMsk(fREFRESH_Tchr)
#define REFRESH_Tchr1	FInsrt(0x0, fBANKCON_Tchr)	/* 1 clock */
#define REFRESH_Tchr2	FInsrt(0x1, fBANKCON_Tchr)	/* 2 clocks */
#define REFRESH_Tchr3	FInsrt(0x2, fBANKCON_Tchr)	/* 3 clocks */
#define REFRESH_Tchr4	FInsrt(0x3, fBANKCON_Tchr)	/* 4 clocks */

#define fREFRESH_RC	Fld(11,0)	/* DRAM/SDRAM Refresh Counter, init: 0 */
#define REFRESH_RC	FMsk(fREFRESH_RC)
#define REFRESH_RC_VALUE(refresh_period, HCLK) \
			(F1stBit(Fld(1,11)) + 1 - (HCLK)*(refresh_period))

#define fBANKSIZE_SCLK	Fld(1,4) /* SCLK is enable only during SDRAM access cycle
				    for reducing power cosumption.
				    When SDRAM isn't be accessed, SCLK is 'L' level.
				    0 = SCLK is always active
				    1 = SCLK is active only during the access
				    init: 0 */
#define BANKSIZE_SCLK	FMsk(fBANKSIZE_SCLK)

#define fBANKSIZE_MAP	Fld(2,0)	/* BANK6/7 memory map, init: 0 */
#define BANKSIZE_MAP	FMsk(fBANKSIZE_MAP)
#define BANKSIZE_MAP32	FInsrt(0x0, fBANKCON_MAP)	/* 32M/32M */
#define BANKSIZE_MAP2	FInsrt(0x4, fBANKCON_MAP)	/*  2M/ 2M */
#define BANKSIZE_MAP4	FInsrt(0x5, fBANKCON_MAP)	/*  4M/ 4M */
#define BANKSIZE_MAP8	FInsrt(0x6, fBANKCON_MAP)	/*  8M/ 8M */
#define BANKSIZE_MAP16	FInsrt(0x7, fBANKCON_MAP)	/* 16M/16M */

#define fMRSR_WBL	Fld(1,9)	/* Write burst length */
#define MRSR_WBL	FMsk(fMRSR_WBL)
#define	MRSR_WBL_Burst	FInsrt(0x0, fBANKCON_WBL)	/* Burst(Fixed) */

#define fMRSR_TM	Fld(2,7)	/* Test Mode */
#define MRSR_TM		FMsk(fMRSR_TM)
#define	MRSR_TM_Set	FInsrt(0x0, fBANKCON_TM)	/* Mode Register set(Fixed) */

#define fMRSR_CL	Fld(3,4)	/* CAS Latency */
#define MRSR_CL		FMsk(fMRSR_CL)
#define	MRSR_CL1	FInsrt(0x0, fBANKCON_CL)	/* 1 clock */
#define	MRSR_CL2	FInsrt(0x2, fBANKCON_CL)	/* 2 clocks */
#define	MRSR_CL3	FInsrt(0x3, fBANKCON_CL)	/* 3 clocks */

#define fMRSR_BT	Fld(1,3)	/* Burst Type */
#define MRSR_BT		FMsk(fMRSR_BT)
#define	MRSR_BT_Seq	FInsrt(0x0, fBANKCON_BT)	/* sequential(Fixed) */

#define fMRSR_BL	Fld(3,0)	/* Burst Length */
#define MRSR_BL		FMsk(fMRSR_BL)
#define	MRSR_BL1	FInsrt(0x0, fBANKCON_BL)	/* 1 (Fixed) */


/* Clock and Power Management */
#define CLK_CTL_BASE		0x4C000000
#define bCLKCTL(Nb)		__REG(CLK_CTL_BASE + (Nb))
/* Offset */
#define oLOCKTIME		0x00	/* R/W, PLL lock time count register */
#define oMPLLCON		0x04	/* R/W, MPLL configuration register */
#define oUPLLCON		0x08	/* R/W, UPLL configuration register */
#define oCLKCON			0x0C	/* R/W, Clock generator control reg. */
#define oCLKSLOW		0x10	/* R/W, Slow clock control register */
#define oCLKDIVN		0x14	/* R/W, Clock divider control */
/* Registers */
#define LOCKTIME		bCLKCTL(oLOCKTIME)
#define MPLLCON			bCLKCTL(oMPLLCON)
#define UPLLCON			bCLKCTL(oUPLLCON)
#define CLKCON			bCLKCTL(oCLKCON)
#define CLKSLOW			bCLKCTL(oCLKSLOW)
#define CLKDIVN			bCLKCTL(oCLKDIVN)

//CLOCK SLOW CONTROL REGISTER ITEM 
//it's turn off the M/UPLL for power management 
#define	UCLK_OFF 		(1<<7)    // 0:UCLK ON : ISERTED AUTOMATICALLY, 1: UCLK OFF
#define MPLL_OFF 		(1<<5)	   // 0:PLL TURN ON 1:PLL IS TURN OFF
#define SLOW_BIT 		(1<<4)    // 0:FCLK=MPLL 1:SLOW MODE

/* Clock Divider Control Bit Field*/
#define DIVN_UPLL            	(1<<3)   
#define HDIVN_MASK		(3<<1)
#define PDIVN_MASK              (1) 

/* Fields */
#define fPLL_MDIV		Fld(8,12)
#define fPLL_PDIV		Fld(6,4)
#define fPLL_SDIV		Fld(2,0)
/* bits */
#define CLKCON_AC97             (1<<20)
#define CLKCON_SPI		(1<<18)
#define CLKCON_IIS		(1<<17)
#define CLKCON_IIC		(1<<16)
#define CLKCON_ADC		(1<<15)
#define CLKCON_RTC		(1<<14)
#define CLKCON_GPIO		(1<<13)
#define CLKCON_UART2		(1<<12)
#define CLKCON_UART1		(1<<11)
#define CLKCON_UART0		(1<<10)
#define CLKCON_SDI		(1<<9)
#define CLKCON_PWM		(1<<8)
#define CLKCON_USBD		(1<<7)
#define CLKCON_USBH		(1<<6)
#define CLKCON_LCDC		(1<<5)
#define CLKCON_NAND		(1<<4)
#define CLKCON_POWEROFF		(1<<3)
#define CLKCON_IDLE		(1<<2)


/*
 * I/O Port (Page 9-8)
 *
 * Register
   GPACON	Port A Control				[word, R/W, 0x7ffff]
   GPADAT	Port A Data				[word, R/W, ?]
   GPBCON	Port B Control				[word, R/W, 0x0]
   GPBDAT	Port B Data				[word, R/W, ?]
   GPBUP	Pull-up Control B (low active)		[word, R/W, 0x0]
   GPCCON	Port C Control				[word, R/W, 0x0]
   GPCDAT	Port C Data				[word, R/W, ?]
   GPCUP	Pull-up Control C (low active)		[word, R/W, 0x0]
   GPDCON	Port D Control				[word, R/W, 0x0]
   GPDDAT	Port D Data				[word, R/W, ?]
   GPDUP	Pull-up Control D (low active) 		[word, R/W, 0xF000]
   GPECON	Port E Control				[word, R/W, 0x0]
   GPEDAT	Port E Data				[word, R/W, ?]
   GPEUP	Pull-up Control E (low active)		[word, R/W, 0x0]
   GPFCON	Port F Control				[word, R/W, 0x0]
   GPFDAT	Port F Data				[word, R/W, ?]
   GPFUP	Pull-up Control F (low active)		[word, R/W, 0x0]
   GPGCON	Port G Control				[word, R/W, 0x0]
   GPGDAT	Port G Data				[word, R/W, ?]
   GPGUP	Pull-up Control G (low active)		[word, R/W, 0xF800]
   GPHCON	Port H Control				[word, R/W, 0x0]
   GPHDAT	Port H Data				[word, R/W, ?]
   GPHUP	Pull-up Control H (low active)		[word, R/W, 0x0]
   MISCCR	Miscellaneous Control			[word, R/W, 0x10330]
   DCLKCON	DCLK0/1 Control				[word, R/W, 0x0]
   EXTINT0	External Interrupt 0 Control		[word, R/W, 0x0]
   EXTINT1	External Interrupt 0 Control		[word, R/W, 0x0]
   EXTINT2	External Interrupt 0 Control		[word, R/W, 0x0]
   EINFLT0	Reserved
   EINFLT1	Reserved
   EINFLT2	External Interrupt 2 Filter		[word, R/W, 0x0]
   EINFLT3	External Interrupt 3 Filter		[word, R/W, 0x0]
   EINTMASK	External Interrupt Mask			[word, R/W, 0x00fffff0]
   EINTPEND	External Interrupt Pending		[word, R/W, 0x0]
   GSTATUS0	General Status (External Pin status)	[word, R,   ?]
 
   s3c2440
   GSTATUS1	General Status (Chip ID)		[word, R/W, 0x32410000]
   S3C240A
   GSTATUS1	General Status (Chip ID)		[word, R/W, 0x32440001]
 *
 */
#define GPIO_CTL_BASE		0x56000000
//#define bGPIO(p)		__REG(GPIO_CTL_BASE + (p))	jassi
#define bGPIO(p)		(GPIO_CTL_BASE + (p))
#define GPACON			bGPIO(0x00)
#define GPADAT			bGPIO(0x04)
#define GPBCON			bGPIO(0x10)
#define GPBDAT			bGPIO(0x14)
#define GPBUP			bGPIO(0x18)
#define GPCCON			bGPIO(0x20)
#define GPCDAT			bGPIO(0x24)
#define GPCUP			bGPIO(0x28)
#define GPDCON			bGPIO(0x30)
#define GPDDAT			bGPIO(0x34)
#define GPDUP			bGPIO(0x38)
#define GPECON			bGPIO(0x40)
#define GPEDAT			bGPIO(0x44)
#define GPEUP			bGPIO(0x48)
#define GPFCON			bGPIO(0x50)
#define GPFDAT			bGPIO(0x54)
#define GPFUP			bGPIO(0x58)
#define GPGCON			bGPIO(0x60)
#define GPGDAT			bGPIO(0x64)
#define GPGUP			bGPIO(0x68)
#define GPHCON			bGPIO(0x70)
#define GPHDAT			bGPIO(0x74)
#define GPHUP			bGPIO(0x78)

#define MISCCR			bGPIO(0x80)
#define DCLKCON			bGPIO(0x84)
#define EXTINT0			bGPIO(0x88)
#define EXTINT1			bGPIO(0x8c)
#define EXTINT2			bGPIO(0x90)
#define EINTFLT0		bGPIO(0x94)
#define EINTFLT1		bGPIO(0x98)
#define EINTFLT2		bGPIO(0x9c)
#define EINTFLT3		bGPIO(0xa0)
#define EINTMASK		bGPIO(0xa4)
#define EINTPEND		bGPIO(0xa8)
#define GSTATUS0		bGPIO(0xac)
#define GSTATUS1		bGPIO(0xb0)
#define GSTATUS2		bGPIO(0xb4)
#define GSTATUS3		bGPIO(0xb8)
#define GSTATUS4		bGPIO(0xbc)
#define DSC0			bGPIO(0xc4)
#define DSC1			bGPIO(0xc8)
#define MSLCON			bGPIO(0xcc)
	
/* Miscellaneous */
    
#define MISCCR_OFFREFRESH       (1 << 19)	/* 0:Self refresh retain disable 1:Self refresh retain enable */    
#define MISCCR_SCLK1_DIS	(1 << 18)       /* SCLK0 0:SCLK0 = SCLK, 1:SCLK0 = 0 */
#define MISCCR_SCLK1_EN		(0 << 18)       /* SCLK0 0:SCLK0 = SCLK, 1:SCLK0 = 0 */
#define MISCCR_SCLK0_DIS	(1 << 17)       /* SCLK0 0:SCLK0 = SCLK, 1:SCLK0 = 0 */
#define MISCCR_SCLK0_EN		(0 << 17)       /* SCLK0 0:SCLK0 = SCLK, 1:SCLK0 = 0 */
#define MISCCR_nRSTCON		(1 << 16)	/* nRSTOUT software control */
#define MISCCR_USB1_SUSPEND	(1 << 13)	/* set USB port 1 to Sleep */
#define MISCCR_USB0_SUSPEND	(1 << 12)	/* set USB port 0 to Sleep */
#define fMISCCR_CLKSEL(x)	Fld(3, 4*((x)+1))
#define MISCCR_CLKSEL(x)	FMsk(fMISCCR_CLKSEL(x))
				/* select ? CLK with CLKOUTx pad */

#ifdef CONFIG_ARCH_S3C2440A
#define MISCCR_CLKSEL_MPLL(x)	FInsrt(0x0, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_UPLL(x)	FInsrt(0x1, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_FCLK(x)	FInsrt(0x2, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_HCLK(x)	FInsrt(0x3, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_PCLK(x)	FInsrt(0x4, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_DCLK(x)	FInsrt(0x5, fMISCCR_CLKSEL(x))
#else
#define MISCCR_CLKSEL_HCLK(x)	FInsrt(0x3, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_PCLK(x)	FInsrt(0x4, fMISCCR_CLKSEL(x))
#define MISCCR_CLKSEL_DCLK(x)	FInsrt(0x5, fMISCCR_CLKSEL(x))
#endif
    
#define MISCCR_USBPAD		(1 << 3)	/* use pads related USB for
						   0: USB slave, 1: USB host */
#define MISCCR_HZSTOP		(1 << 2)	/* 0: HZ@stop
						   1: previous state of PAD */
#define MISCCR_SPUCR1		(1 << 1)	/* DATA[31:16] port pull-up */
#define MISCCR_SPUCR0		(1 << 0)	/* DATA[15:0] port pull-up */

/* DCLK control register */
#define fDCLKCMP(x)		Fld(4,8+16*(x))	/* DCLK Compare value clock */
#define mDCLKCMP(x)		FMsk(fDCLKCMP(x))
#define DCLKCMP(x, y)		Finsrt((y), fDCLKCMP(x))
#define fDCLKDIV(x)		Fld(4,4+16*(x))	/* DCLK divide value */
#define mDCLKDIV(x)		FMsk(fDCLKDIV(x))
#define DCLKDIV(x, y)		Finsrt((y), fDCLKDIV(x))
#define DCLKSEL_PCLK(x)		(0 << (1+16*(x))
				 /* Select PCLK as DCLK Source Clock */
#define DCLKSEL_USB(x)		(1 << (1+16*(x))
				 /* Select USBCLK as DCLK Source Clock */
#define DCLK1CMP		mDCLKCMP(1)
#define DCLK1DIV		mDCLKDIV(1)
#define DCLK1SEL_PCLK		DCLKSEL_PCLK(1)
#define DCLK1SEL_USB		DCLKSEL_USB(1)
#define DCLK0CMP		mDCLKCMP(0)
#define DCLK0DIV		mDCLKDIV(0)
#define DCLK0SEL_PCLK		DCLKSEL_PCLK(0)
#define DCLK0SEL_USB		DCLKSEL_USB(0)

/* General Status */
#define GSTAT0_nWAIT		(1 << 3)	/* Status of nWAIT pin */
#define GSTAT0_NCON1		(1 << 2)	/* Status of NCON1 pin */
#define GSTAT0_NCON0		(1 << 1)	/* Status of NCON0 pin */
#define GSTAT0_BATT_FLT		(1 << 0)	/* Status of BATT_FLT pin */


#define GPCON(x)		__REG2(0x56000000, (x) * 0x10)
#define GPDAT(x)		__REG2(0x56000004, (x) * 0x10)
#define GPUP(x)			__REG2(0x56000008, (x) * 0x10)

#define GPIO_OFS_SHIFT		0
#define GPIO_PORT_SHIFTT	8
#define GPIO_PULLUP_SHIFT	16 
#define GPIO_MODE_SHIFT		24
#define GPIO_OFS_MASK		0x000000ff
#define GPIO_PORT_MASK		0x0000ff00
#define GPIO_PULLUP_MASK	0x00ff0000
#define GPIO_MODE_MASK		0xff000000
#define GPIO_MODE_IN		(0 << GPIO_MODE_SHIFT)
#define GPIO_MODE_OUT		(1 << GPIO_MODE_SHIFT)
#define GPIO_MODE_ALT0		(2 << GPIO_MODE_SHIFT)
#define GPIO_MODE_ALT1		(3 << GPIO_MODE_SHIFT)
#define GPIO_PULLUP_EN		(0 << GPIO_PULLUP_SHIFT)
#define GPIO_PULLUP_DIS		(1 << GPIO_PULLUP_SHIFT) 

#define PORTA_OFS		0
#define PORTB_OFS		1
#define PORTC_OFS		2
#define PORTD_OFS		3
#define PORTE_OFS		4
#define PORTF_OFS		5
#define PORTG_OFS		6
#define PORTH_OFS		7

#define MAKE_GPIO_NUM(p, o)	((p << GPIO_PORT_SHIFTT) | (o << GPIO_OFS_SHIFT))

#define GRAB_MODE(x)		(((x) & GPIO_MODE_MASK) >> GPIO_MODE_SHIFT)
#define GRAB_PULLUP(x)		(((x) & GPIO_PULLUP_MASK) >> GPIO_PULLUP_SHIFT)
#define GRAB_PORT(x)		(((x) & GPIO_PORT_MASK) >> GPIO_PORT_SHIFTT)
#define GRAB_OFS(x)		(((x) & GPIO_OFS_MASK) >> GPIO_OFS_SHIFT)

#define set_gpio_ctrl(x) \
	({ GPCON(GRAB_PORT((x))) &= ~(0x3 << (GRAB_OFS((x))*2)); \
	   GPCON(GRAB_PORT(x)) |= (GRAB_MODE(x) << (GRAB_OFS((x))*2)); \
	   GPUP(GRAB_PORT((x))) &= ~(1 << GRAB_OFS((x))); \
	   GPUP(GRAB_PORT((x))) |= (GRAB_PULLUP((x)) << GRAB_OFS((x))); })
#define set_gpio_pullup(x) \
	({ GPUP(GRAB_PORT((x))) &= ~(1 << GRAB_OFS((x))); \
	   GPUP(GRAB_PORT((x))) |= (GRAB_PULLUP((x)) << GRAB_OFS((x))); })
#define set_gpio_pullup_user(x, v) \
	({ GPUP(GRAB_PORT((x))) &= ~(1 << GRAB_OFS((x))); \
	   GPUP(GRAB_PORT((x))) |= ((v) << GRAB_OFS((x))); })
#define set_gpio_mode(x) \
	({ GPCON(GRAB_PORT((x))) &= ~(0x3 << (GRAB_OFS((x))*2)); \
	   GPCON(GRAB_PORT((x))) |= (GRAB_MODE((x)) << (GRAB_OFS((x))*2)); })
#define set_gpio_mode_user(x, v) \
	({ GPCON(GRAB_PORT((x))) & = ~(0x3 << (GRAB_OFS((x))*2)); \
	   GPCON(GRAB_PORT((x))) |= ((v) << (GRAB_OFS((x))*2)); })
#define set_gpioA_mode(x) \
	({ GPCON(GRAB_PORT((x))) &= ~(0x1 << GRAB_OFS((x))); \
	   GPCON(GRAB_PORT((x))) |= (GRAB_MODE((x)) << GRAB_OFS((x))); })
#define read_gpio_bit(x)	((GPDAT(GRAB_PORT((x))) & (1<<GRAB_OFS((x)))) >> GRAB_OFS((x)))
#define read_gpio_reg(x)	(GPDAT(GRAB_PORT((x)))
#define write_gpio_bit(x, v) \
	({ GPDAT(GRAB_PORT((x))) &= ~(0x1 << GRAB_OFS((x))); \
	   GPDAT(GRAB_PORT((x))) |= ((v) << GRAB_OFS((x))); })
#define write_gpio_reg(x, v)	(GPDAT(GRAB_PORT((x))) = (v))
	


/*
 *  * Don't use write_gpio_bit & write_gpio_reg in Open-drain I/O type
 *   *                                                      -by SW.LEE
 *    */
	
#define write_gpio_bit_clear(x) \
            ({ GPDAT(GRAB_PORT((x))) &= ~(0x1 << GRAB_OFS((x)));  })

#define write_gpio_bit_set(x) \
         ({ GPDAT(GRAB_PORT((x))) |=  (0x1 << GRAB_OFS((x)));  })

#define write_gpio_bit(x, v) \
           ({ GPDAT(GRAB_PORT((x))) &= ~(0x1 << GRAB_OFS((x))); \
             GPDAT(GRAB_PORT((x))) |= ((v) << GRAB_OFS((x))); })
#define write_gpio_reg(x, v)    (GPDAT(GRAB_PORT((x))) = (v))


#define GPIO_A0				MAKE_GPIO_NUM(PORTA_OFS, 0)
#define GPIO_A1				MAKE_GPIO_NUM(PORTA_OFS, 1)
#define GPIO_A2				MAKE_GPIO_NUM(PORTA_OFS, 2)
#define GPIO_A3				MAKE_GPIO_NUM(PORTA_OFS, 3)
#define GPIO_A4				MAKE_GPIO_NUM(PORTA_OFS, 4)
#define GPIO_A5				MAKE_GPIO_NUM(PORTA_OFS, 5)
#define GPIO_A6				MAKE_GPIO_NUM(PORTA_OFS, 6)
#define GPIO_A7				MAKE_GPIO_NUM(PORTA_OFS, 7)
#define GPIO_A8				MAKE_GPIO_NUM(PORTA_OFS, 8)
#define GPIO_A9				MAKE_GPIO_NUM(PORTA_OFS, 9)
#define GPIO_A10			MAKE_GPIO_NUM(PORTA_OFS, 10)
#define GPIO_A11			MAKE_GPIO_NUM(PORTA_OFS, 11)
#define GPIO_A12			MAKE_GPIO_NUM(PORTA_OFS, 12)
#define GPIO_A13			MAKE_GPIO_NUM(PORTA_OFS, 13)
#define GPIO_A14			MAKE_GPIO_NUM(PORTA_OFS, 14)
#define GPIO_A15			MAKE_GPIO_NUM(PORTA_OFS, 15)
#define GPIO_A16			MAKE_GPIO_NUM(PORTA_OFS, 16)
#define GPIO_A17			MAKE_GPIO_NUM(PORTA_OFS, 17)
#define GPIO_A18			MAKE_GPIO_NUM(PORTA_OFS, 18)
#define GPIO_A19			MAKE_GPIO_NUM(PORTA_OFS, 19)
#define GPIO_A20			MAKE_GPIO_NUM(PORTA_OFS, 20)
#define GPIO_A21			MAKE_GPIO_NUM(PORTA_OFS, 21)
#define GPIO_A22			MAKE_GPIO_NUM(PORTA_OFS, 22)

#define GPIO_B0				MAKE_GPIO_NUM(PORTB_OFS, 0)
#define GPIO_B1				MAKE_GPIO_NUM(PORTB_OFS, 1)
#define GPIO_B2				MAKE_GPIO_NUM(PORTB_OFS, 2)
#define GPIO_B3				MAKE_GPIO_NUM(PORTB_OFS, 3)
#define GPIO_B4				MAKE_GPIO_NUM(PORTB_OFS, 4)
#define GPIO_B5				MAKE_GPIO_NUM(PORTB_OFS, 5)
#define GPIO_B6				MAKE_GPIO_NUM(PORTB_OFS, 6)
#define GPIO_B7				MAKE_GPIO_NUM(PORTB_OFS, 7)
#define GPIO_B8				MAKE_GPIO_NUM(PORTB_OFS, 8)
#define GPIO_B9				MAKE_GPIO_NUM(PORTB_OFS, 9)
#define GPIO_B10			MAKE_GPIO_NUM(PORTB_OFS, 10)

#define GPIO_C0				MAKE_GPIO_NUM(PORTC_OFS, 0)
#define GPIO_C1				MAKE_GPIO_NUM(PORTC_OFS, 1)
#define GPIO_C2				MAKE_GPIO_NUM(PORTC_OFS, 2)
#define GPIO_C3				MAKE_GPIO_NUM(PORTC_OFS, 3)
#define GPIO_C4				MAKE_GPIO_NUM(PORTC_OFS, 4)
#define GPIO_C5				MAKE_GPIO_NUM(PORTC_OFS, 5)
#define GPIO_C6				MAKE_GPIO_NUM(PORTC_OFS, 6)
#define GPIO_C7				MAKE_GPIO_NUM(PORTC_OFS, 7)
#define GPIO_C8				MAKE_GPIO_NUM(PORTC_OFS, 8)
#define GPIO_C9				MAKE_GPIO_NUM(PORTC_OFS, 9)
#define GPIO_C10			MAKE_GPIO_NUM(PORTC_OFS, 10)
#define GPIO_C11			MAKE_GPIO_NUM(PORTC_OFS, 11)
#define GPIO_C12			MAKE_GPIO_NUM(PORTC_OFS, 12)
#define GPIO_C13			MAKE_GPIO_NUM(PORTC_OFS, 13)
#define GPIO_C14			MAKE_GPIO_NUM(PORTC_OFS, 14)
#define GPIO_C15			MAKE_GPIO_NUM(PORTC_OFS, 15)

#define GPIO_D0				MAKE_GPIO_NUM(PORTD_OFS, 0)
#define GPIO_D1				MAKE_GPIO_NUM(PORTD_OFS, 1)
#define GPIO_D2				MAKE_GPIO_NUM(PORTD_OFS, 2)
#define GPIO_D3				MAKE_GPIO_NUM(PORTD_OFS, 3)
#define GPIO_D4				MAKE_GPIO_NUM(PORTD_OFS, 4)
#define GPIO_D5				MAKE_GPIO_NUM(PORTD_OFS, 5)
#define GPIO_D6				MAKE_GPIO_NUM(PORTD_OFS, 6)
#define GPIO_D7				MAKE_GPIO_NUM(PORTD_OFS, 7)
#define GPIO_D8				MAKE_GPIO_NUM(PORTD_OFS, 8)
#define GPIO_D9				MAKE_GPIO_NUM(PORTD_OFS, 9)
#define GPIO_D10			MAKE_GPIO_NUM(PORTD_OFS, 10)
#define GPIO_D11			MAKE_GPIO_NUM(PORTD_OFS, 11)
#define GPIO_D12			MAKE_GPIO_NUM(PORTD_OFS, 12)
#define GPIO_D13			MAKE_GPIO_NUM(PORTD_OFS, 13)
#define GPIO_D14			MAKE_GPIO_NUM(PORTD_OFS, 14)
#define GPIO_D15			MAKE_GPIO_NUM(PORTD_OFS, 15)

#define GPIO_E0				MAKE_GPIO_NUM(PORTE_OFS, 0)
#define GPIO_E1				MAKE_GPIO_NUM(PORTE_OFS, 1)
#define GPIO_E2				MAKE_GPIO_NUM(PORTE_OFS, 2)
#define GPIO_E3				MAKE_GPIO_NUM(PORTE_OFS, 3)
#define GPIO_E4				MAKE_GPIO_NUM(PORTE_OFS, 4)
#define GPIO_E5				MAKE_GPIO_NUM(PORTE_OFS, 5)
#define GPIO_E6				MAKE_GPIO_NUM(PORTE_OFS, 6)
#define GPIO_E7				MAKE_GPIO_NUM(PORTE_OFS, 7)
#define GPIO_E8				MAKE_GPIO_NUM(PORTE_OFS, 8)
#define GPIO_E9				MAKE_GPIO_NUM(PORTE_OFS, 9)
#define GPIO_E10			MAKE_GPIO_NUM(PORTE_OFS, 10)
#define GPIO_E11			MAKE_GPIO_NUM(PORTE_OFS, 11)
#define GPIO_E12			MAKE_GPIO_NUM(PORTE_OFS, 12)
#define GPIO_E13			MAKE_GPIO_NUM(PORTE_OFS, 13)
#define GPIO_E14			MAKE_GPIO_NUM(PORTE_OFS, 14)
#define GPIO_E15			MAKE_GPIO_NUM(PORTE_OFS, 15)

#define GPIO_F0				MAKE_GPIO_NUM(PORTF_OFS, 0)
#define GPIO_F1				MAKE_GPIO_NUM(PORTF_OFS, 1)
#define GPIO_F2				MAKE_GPIO_NUM(PORTF_OFS, 2)
#define GPIO_F3				MAKE_GPIO_NUM(PORTF_OFS, 3)
#define GPIO_F4				MAKE_GPIO_NUM(PORTF_OFS, 4)
#define GPIO_F5				MAKE_GPIO_NUM(PORTF_OFS, 5)
#define GPIO_F6				MAKE_GPIO_NUM(PORTF_OFS, 6)
#define GPIO_F7				MAKE_GPIO_NUM(PORTF_OFS, 7)

#define GPIO_G0				MAKE_GPIO_NUM(PORTG_OFS, 0)
#define GPIO_G1				MAKE_GPIO_NUM(PORTG_OFS, 1)
#define GPIO_G2				MAKE_GPIO_NUM(PORTG_OFS, 2)
#define GPIO_G3				MAKE_GPIO_NUM(PORTG_OFS, 3)
#define GPIO_G4				MAKE_GPIO_NUM(PORTG_OFS, 4)
#define GPIO_G5				MAKE_GPIO_NUM(PORTG_OFS, 5)
#define GPIO_G6				MAKE_GPIO_NUM(PORTG_OFS, 6)
#define GPIO_G7				MAKE_GPIO_NUM(PORTG_OFS, 7)
#define GPIO_G8				MAKE_GPIO_NUM(PORTG_OFS, 8)
#define GPIO_G9				MAKE_GPIO_NUM(PORTG_OFS, 9)
#define GPIO_G10			MAKE_GPIO_NUM(PORTG_OFS, 10)
#define GPIO_G11			MAKE_GPIO_NUM(PORTG_OFS, 11)
#define GPIO_G12			MAKE_GPIO_NUM(PORTG_OFS, 12)
#define GPIO_G13			MAKE_GPIO_NUM(PORTG_OFS, 13)
#define GPIO_G14			MAKE_GPIO_NUM(PORTG_OFS, 14)
#define GPIO_G15			MAKE_GPIO_NUM(PORTG_OFS, 15)

#define GPIO_H0				MAKE_GPIO_NUM(PORTH_OFS, 0)
#define GPIO_H1				MAKE_GPIO_NUM(PORTH_OFS, 1)
#define GPIO_H2				MAKE_GPIO_NUM(PORTH_OFS, 2)
#define GPIO_H3				MAKE_GPIO_NUM(PORTH_OFS, 3)
#define GPIO_H4				MAKE_GPIO_NUM(PORTH_OFS, 4)
#define GPIO_H5				MAKE_GPIO_NUM(PORTH_OFS, 5)
#define GPIO_H6				MAKE_GPIO_NUM(PORTH_OFS, 6)
#define GPIO_H7				MAKE_GPIO_NUM(PORTH_OFS, 7)
#define GPIO_H8				MAKE_GPIO_NUM(PORTH_OFS, 8)
#define GPIO_H9				MAKE_GPIO_NUM(PORTH_OFS, 9)
#define GPIO_H10			MAKE_GPIO_NUM(PORTH_OFS, 10)

#define GPIO_MODE_TOUT			GPIO_MODE_ALT0
#define GPIO_MODE_nXBACK		GPIO_MODE_ALT0
#define GPIO_MODE_nXBREQ		GPIO_MODE_ALT0
#define GPIO_MODE_nXDACK		GPIO_MODE_ALT0
#define GPIO_MODE_nXDREQ		GPIO_MODE_ALT0
#define GPIO_MODE_LEND			GPIO_MODE_ALT0
#define GPIO_MODE_VCLK			GPIO_MODE_ALT0
#define GPIO_MODE_VLINE			GPIO_MODE_ALT0
#define GPIO_MODE_VFRAME		GPIO_MODE_ALT0
#define GPIO_MODE_VM			GPIO_MODE_ALT0
#define GPIO_MODE_LCDVF			GPIO_MODE_ALT0
#define GPIO_MODE_VD			GPIO_MODE_ALT0
#define GPIO_MODE_IICSDA		GPIO_MODE_ALT0
#define GPIO_MODE_IICSCL		GPIO_MODE_ALT0
#define GPIO_MODE_SPICLK		GPIO_MODE_ALT0
#define GPIO_MODE_SPIMOSI		GPIO_MODE_ALT0
#define GPIO_MODE_SPIMISO		GPIO_MODE_ALT0
#define GPIO_MODE_SDDAT			GPIO_MODE_ALT0
#define GPIO_MODE_SDCMD			GPIO_MODE_ALT0
#define GPIO_MODE_SDCLK			GPIO_MODE_ALT0
#define GPIO_MODE_I2SSDO		GPIO_MODE_ALT0
#define GPIO_MODE_I2SSDI		GPIO_MODE_ALT0
#define GPIO_MODE_CDCLK			GPIO_MODE_ALT0
#define GPIO_MODE_I2SSCLK		GPIO_MODE_ALT0
#define GPIO_MODE_I2SLRCK		GPIO_MODE_ALT0
#define GPIO_MODE_I2SSDI_ABNORMAL	GPIO_MODE_ALT1
#define GPIO_MODE_nSS			GPIO_MODE_ALT1
#define GPIO_MODE_EINT			GPIO_MODE_ALT0
#define GPIO_MODE_nYPON			GPIO_MODE_ALT1
#define GPIO_MODE_YMON			GPIO_MODE_ALT1
#define GPIO_MODE_nXPON			GPIO_MODE_ALT1
#define GPIO_MODE_XMON			GPIO_MODE_ALT1
#define GPIO_MODE_UART			GPIO_MODE_ALT0	
#define GPIO_MODE_TCLK_ABNORMAL		GPIO_MODE_ALT1
#define GPIO_MODE_SPICLK_ABNORMAL	GPIO_MODE_ALT1
#define GPIO_MODE_SPIMOSI_ABNORMAL	GPIO_MODE_ALT1
#define GPIO_MODE_SPIMISO_ABNORMAL	GPIO_MODE_ALT1
#define GPIO_MODE_LCD_PWRDN		GPIO_MODE_ALT1

/* S3C2440A AC97 SUPPORT */
#define GPIO_MODE_AC_SDATA_OUT		GPIO_MODE_ALT1
#define GPIO_MODE_AC_SDATA_IN		GPIO_MODE_ALT1
#define GPIO_MODE_AC_nRESET		GPIO_MODE_ALT1
#define GPIO_MODE_AC_BIT_CLK		GPIO_MODE_ALT1
#define GPIO_MODE_AC_SYNC		GPIO_MODE_ALT1
#define AC97_CTL_BASE			0x5b000000

/* UART */
#define UART_CTL_BASE		0x50000000
#define UART0_CTL_BASE		UART_CTL_BASE
#define UART1_CTL_BASE		UART_CTL_BASE + 0x4000
#define UART2_CTL_BASE		UART_CTL_BASE + 0x8000
#define bUART(x, Nb)		__REG(UART_CTL_BASE + (x)*0x4000 + (Nb))
/* Offset */
#define oULCON			0x00	/* R/W, UART line control register */
#define oUCON			0x04	/* R/W, UART control register */
#define oUFCON			0x08	/* R/W, UART FIFO control register */
#define oUMCON			0x0C	/* R/W, UART modem control register */
#define oUTRSTAT		0x10	/* R  , UART Tx/Rx status register */
#define oUERSTAT		0x14	/* R  , UART Rx error status register */
#define oUFSTAT			0x18	/* R  , UART FIFO status register */
#define oUMSTAT			0x1C	/* R  , UART Modem status register */
#define oUTXHL			0x20	/*   W, UART transmit(little-end) buffer */
#define oUTXHB			0x23	/*   W, UART transmit(big-end) buffer */
#define oURXHL			0x24	/* R  , UART receive(little-end) buffer */
#define oURXHB			0x27	/* R  , UART receive(big-end) buffer */
#define oUBRDIV			0x28	/* R/W, Baud rate divisor register */
/* Registers */
#define ULCON0			bUART(0, oULCON)
#define UCON0			bUART(0, oUCON)
#define UFCON0			bUART(0, oUFCON)
#define UMCON0			bUART(0, oUMCON)
#define UTRSTAT0		bUART(0, oUTRSTAT)
#define UERSTAT0		bUART(0, oUERSTAT)
#define UFSTAT0			bUART(0, oUFSTAT)
#define UMSTAT0			bUART(0, oUMSTAT)
#define UTXH0			bUART(0, oUTXHL)
#define URXH0			bUART(0, oURXHL)
#define UBRDIV0			bUART(0, oUBRDIV)
#define ULCON1			bUART(1, oULCON)
#define UCON1			bUART(1, oUCON)
#define UFCON1			bUART(1, oUFCON)
#define UMCON1			bUART(1, oUMCON)
#define UTRSTAT1		bUART(1, oUTRSTAT)
#define UERSTAT1		bUART(1, oUERSTAT)
#define UFSTAT1			bUART(1, oUFSTAT)
#define UMSTAT1			bUART(1, oUMSTAT)
#define UTXH1			bUART(1, oUTXHL)
#define URXH1			bUART(1, oURXHL)
#define UBRDIV1			bUART(1, oUBRDIV)
#define ULCON2			bUART(2, oULCON)
#define UCON2			bUART(2, oUCON)
#define UFCON2			bUART(2, oUFCON)
#define UMCON2			bUART(2, oUMCON)
#define UTRSTAT2		bUART(2, oUTRSTAT)
#define UERSTAT2		bUART(2, oUERSTAT)
#define UFSTAT2			bUART(2, oUFSTAT)
#define UMSTAT2			bUART(2, oUMSTAT)
#define UTXH2			bUART(2, oUTXHL)
#define URXH2			bUART(2, oURXHL)
#define UBRDIV2			bUART(2, oUBRDIV)
/* ... */
#define UTRSTAT_TX_EMPTY	(1 << 2)
#define UTRSTAT_RX_READY	(1 << 0)
#define UART_ERR_MASK		0xF 

#define ULCON_IR		(1 << 6)	/* use Infra-Red mode */

#define fULCON_PAR		Fld(3,3)	/* what parity mode? */
#define ULCON_PAR		FMsk(fULCON_PAR)
#define ULCON_PAR_NONE		FInsrt(0x0, fULCON_PAR) /* No Parity */
#define ULCON_PAR_ODD		FInsrt(0x4, fULCON_PAR) /* Odd Parity */
#define ULCON_PAR_EVEN		FInsrt(0x5, fULCON_PAR) /* Even Parity */
#define ULCON_PAR_1		FInsrt(0x6, fULCON_PAR) /* Parity force/checked as 1 */
#define ULCON_PAR_0		FInsrt(0x7, fULCON_PAR) /* Parity force/checked as 0 */

#define ULCON_STOP		(1 << 2)	/* The number of stop bits */
#define ULCON_ONE_STOP		(0 << 2)	/* 1 stop bit */
#define ULCON_TWO_STOP		(1 << 2)	/* 2 stop bit */

#define fULCON_WL		Fld(2, 0)	/* word length */
#define ULCON_WL		FMsk(fULCON_WL)
#define ULCON_WL5		FInsrt(0x0, fULCON_WL)	/* 5 bits */
#define ULCON_WL6		FInsrt(0x1, fULCON_WL)	/* 6 bits */
#define ULCON_WL7		FInsrt(0x2, fULCON_WL)	/* 7 bits */
#define ULCON_WL8		FInsrt(0x3, fULCON_WL)	/* 8 bits */

#define ULCON_CFGMASK		(ULCON_IR | ULCON_PAR | ULCON_WL)

#ifdef CONFIG_ARCH_S3C2440A
#define UCON_FCLK_DIVN(n)	(((n) & 0xf) << 12)	/* vider calue when FCLK/n */ 
#define fUCONCLK                Fld(2,10)	        /* word length */	   
#define UCON_CLK                FMsk(fUCONCLK)
#define UCON_CLK_FCLKN          FInsrt(0x3, fUCONCLK)  /* FCLK/ n baud rate clock use */
#define UCON_CLK_UEXTCLK	FInsrt(0x1, fUCONCLK)	/* UexternCLK for UART baud rate */
#define UCON_CLK_PCLK		FInsrt(0x0, fUCONCLK)	/* PCLK for UART baud rate */
#else
#define UCON_CLK_PCLK		(0 << 10)	/* PCLK for UART baud rate */
#define UCON_CLK_UCLK		(1 << 10)	/* UCLK for UART baud rate */	   
#endif
#define UCON_TX_INT_TYPE	(1 << 9)	/* TX Interrupt request type */
#define UCON_TX_INT_PLS		(0 << 9)	/* Pulse */
#define UCON_TX_INT_LVL		(1 << 9)	/* Level */

#define UCON_RX_INT_TYPE	(1 << 8)	/* RX Interrupt request type */
#define UCON_RX_INT_PLS		(0 << 8)	/* Pulse */
#define UCON_RX_INT_LVL		(1 << 8)	/* Level */

#define UCON_RX_TIMEOUT		(1 << 7)	/* RX timeout enable */
#define UCON_RX_ERR_INT		(1 << 6)	/* RX error status interrupt enable */
#define UCON_LOOPBACK		(1 << 5)	/* to enter the loop-back mode */
#define UCON_BRK_SIG		(1 << 4)	/* to send a break during 1 frame time */

#define fUCON_TX	Fld(2,2)		/* function to write Tx data
						   to the UART Tx buffer */
#define UCON_TX		FMsk(fUCON_TX)
#define UCON_TX_DIS	FInsrt(0x0, fUCON_TX)	/* Disable */
#define UCON_TX_INT	FInsrt(0x1, fUCON_TX)	/* Interrupt or polling */
#define UCON_TX_DMA0	FInsrt(0x2, fUCON_TX)	/* DMA0 request */
#define UCON_TX_DMA1	FInsrt(0x3, fUCON_TX)	/* DMA1 request */

#define fUCON_RX	Fld(2,0)		/* function to read data
						   from UART Rx buffer */
#define UCON_RX		FMsk(fUCON_RX)
#define UCON_RX_DIS	FInsrt(0x0, fUCON_RX)	/* Disable */
#define UCON_RX_INT	FInsrt(0x1, fUCON_RX)	/* Interrupt or polling */
#define UCON_RX_DMA0	FInsrt(0x2, fUCON_RX)	/* DMA0 request */
#define UCON_RX_DMA1	FInsrt(0x3, fUCON_RX)	/* DMA1 request */

#define fUFCON_TX_TR	Fld(2,6)	/* trigger level of transmit FIFO */
#define UFCON_TX_TR	FMsk(fUFCON_TX_TR)

#ifdef CONFIG_ARCH_S3C2440A
#define UFCON_TX_TR0    FInsrt(0x0, fUFCON_TX_TR)       /* Empty */
#define UFCON_TX_TR16   FInsrt(0x1, fUFCON_TX_TR)       /* 16-byte */
#define UFCON_TX_TR32   FInsrt(0x2, fUFCON_TX_TR)       /* 32-byte */
#define UFCON_TX_TR48   FInsrt(0x3, fUFCON_TX_TR)       /* 48-byte */
                                                                                
#else

#define UFCON_TX_TR0	FInsrt(0x0, fUFCON_TX_TR)	/* Empty */
#define UFCON_TX_TR4	FInsrt(0x1, fUFCON_TX_TR)	/* 4-byte */
#define UFCON_TX_TR8	FInsrt(0x2, fUFCON_TX_TR)	/* 8-byte */
#define UFCON_TX_TR12	FInsrt(0x3, fUFCON_TX_TR)	/* 12-byte */
#endif

#define fUFCON_RX_TR	Fld(2,4)	/* trigger level of receive FIFO */
#define UFCON_RX_TR	FMsk(fUFCON_RX_TR)

#ifdef CONFIG_ARCH_S3C2440A
#define UFCON_RX_TR1    FInsrt(0x0, fUFCON_RX_TR)       /* 1-byte */
#define UFCON_RX_TR8    FInsrt(0x1, fUFCON_RX_TR)       /* 8-byte */
#define UFCON_RX_TR16   FInsrt(0x2, fUFCON_RX_TR)       /* 16-byte */
#define UFCON_RX_TR32   FInsrt(0x3, fUFCON_RX_TR)       /* 32-byte */
                                                                                
#else

#define UFCON_RX_TR0	FInsrt(0x0, fUFCON_RX_TR)	/* Empty */
#define UFCON_RX_TR4	FInsrt(0x1, fUFCON_RX_TR)	/* 4-byte */
#define UFCON_RX_TR8	FInsrt(0x2, fUFCON_RX_TR)	/* 8-byte */
#define UFCON_RX_TR12	FInsrt(0x3, fUFCON_RX_TR)	/* 12-byte */
#endif

#define UFCON_TX_REQ	(1 << 2)	/* auto-cleared after resetting FIFO */
#define UFCON_RX_REQ	(1 << 1)	/* auto-cleared after resetting FIFO */
#define UFCON_FIFO_EN	(1 << 0)	/* FIFO Enable */

#define UMCON_AFC	(1 << 4)	/* Enable Auto Flow Control */
#define UMCON_SEND	(1 << 0)	/* when not-AFC,
						set nRTS 1:'L' 0:'H' level */

#define UTRSTAT_TR_EMP	(1 << 2)	/* 1: Transmitter buffer &
						shifter register empty */
#define UTRSTAT_TX_EMP	(1 << 1)	/* Transmit buffer reg. is empty */
#define UTRSTAT_RX_RDY	(1 << 0)	/* Receive buffer reg. has data */

#define UERSTAT_BRK	(1 << 3)	/* Break receive */
#define UERSTAT_FRAME	(1 << 2)	/* Frame Error */
#define UERSTAT_PARITY	(1 << 1)	/* Parity Error */
#define UERSTAT_OVERRUN	(1 << 0)	/* Overrun Error */

#ifdef CONFIG_ARCH_S3C2440A

                                                                                
#define UFSTAT_TX_FULL  (1 << 14)       /* Transmit FIFO is full */
#define fUFSTAT_TX_CNT  Fld(6,8)        /* Number of data in Tx FIFO */
#define UFSTAT_TX_CNT   FMsk(fUFSTAT_TX_CNT)
                                                                                
#define UFSTAT_RX_FULL  (1 << 6)        /* Receive FIFO is full */
#define fUFSTAT_RX_CNT  Fld(6,0)        /* Number of data in Rx FIFO */
#define UFSTAT_RX_CNT   FMsk(fUFSTAT_RX_CNT)
                                                                                
#define UART2_TXFIFO_CNT() FExtr(UFSTAT2,fUFSTAT_TX_CNT)
#define UART2_RXFIFO_CNT() FExtr(UFSTAT2,fUFSTAT_RX_CNT)
                                                                                
#else

#define UFSTAT_TX_FULL	(1 << 9)	/* Transmit FIFO is full */
#define UFSTAT_RX_FULL	(1 << 8)	/* Receive FIFO is full */

#define fUFSTAT_TX_CNT	Fld(4,4)	/* Number of data in Tx FIFO */
#define UFSTAT_TX_CNT	FMsk(fUFSTAT_TX_CNT)

#define fUFSTAT_RX_CNT	Fld(4,0)	/* Number of data in Rx FIFO */
#define UFSTAT_RX_CNT	FMsk(fUFSTAT_RX_CNT)

#endif

#define UMSTAT_dCTS	(1 << 4)	/* indicate that the nCTS input Delta CTS */
#define UMSTAT_CTS	(1 << 0)	/* CTS(Clear to Send) signal */

#define UTXH_DATA	0x000000FF	/* Transmit data for UARTn */
#define URXH_DATA	0x000000FF	/* Receive data for UARTn */
#define UBRDIVn		0x0000FFFF	/* Baud rate division value (> 0) */
/* UBRDIVn = (int)(PCLK/(bsp * 16)-1 or	UBRDIVn = (int)(UCLK/(bsp * 16)-1 */

/* Interrupts */
#define INT_CTL_BASE		0x4A000000
#define INTBASE			__REG(INT_CTL_BASE)
#define bINTCTL(Nb)		__REG(INT_CTL_BASE + (Nb))
/* Offset */
#define oSRCPND			0x00
#define oINTMOD			0x04
#define oINTMSK			0x08
#define oPRIORITY		0x0C
#define oINTPND			0x10
#define oINTOFFSET		0x14
#define oSUBSRCPND		0x18
#define oINTSUBMSK		0x1C
/* Registers */
#define SRCPND			bINTCTL(oSRCPND)
#define INTMOD			bINTCTL(oINTMOD)
#define INTMSK			bINTCTL(oINTMSK)
#define PRIORITY		bINTCTL(oPRIORITY)
#define INTPND			bINTCTL(oINTPND)
#define INTOFFSET		bINTCTL(oINTOFFSET)
#define SUBSRCPND		bINTCTL(oSUBSRCPND)
#define INTSUBMSK		bINTCTL(oINTSUBMSK)

#define INT_ADCTC		(1 << 31)	/* ADC EOC interrupt */
#define INT_RTC			(1 << 30)	/* RTC alarm interrupt */
#define INT_SPI1		(1 << 29)	/* UART1 transmit interrupt */
#define INT_UART0		(1 << 28)	/* UART0 transmit interrupt */
#define INT_IIC			(1 << 27)	/* IIC interrupt */
#define INT_USBH		(1 << 26)	/* USB host interrupt */
#define INT_USBD		(1 << 25)	/* USB device interrupt */
#define INT_RESERVED24		(1 << 24)
#define INT_UART1		(1 << 23)	/* UART1 receive interrupt */
#define INT_SPI0		(1 << 22)	/* SPI interrupt */
#define INT_MMC			(1 << 21)	/* MMC interrupt */
#define INT_DMA3		(1 << 20)	/* DMA channel 3 interrupt */
#define INT_DMA2		(1 << 19)	/* DMA channel 2 interrupt */
#define INT_DMA1		(1 << 18)	/* DMA channel 1 interrupt */
#define INT_DMA0		(1 << 17)	/* DMA channel 0 interrupt */
#define INT_LCD			(1 << 16)	/* reserved for future use */
#define INT_UART2		(1 << 15)	/* UART 2 interrupt  */
#define INT_TIMER4		(1 << 14)	/* Timer 4 interrupt */
#define INT_TIMER3		(1 << 13)	/* Timer 3 interrupt */
#define INT_TIMER2		(1 << 12)	/* Timer 2 interrupt */
#define INT_TIMER1		(1 << 11)	/* Timer 1 interrupt */
#define INT_TIMER0		(1 << 10)	/* Timer 0 interrupt */
#define INT_WDT			(1 << 9)	/* Watch-Dog timer interrupt */
#define INT_TICK		(1 << 8)	/* RTC time tick interrupt  */
#define INT_nBAT_FLT		(1 << 7)
#define INT_RESERVED6		(1 << 6)	/* Reserved for future use */
#define INT_EINT8_23		(1 << 5)	/* External interrupt 8 ~ 23 */
#define INT_EINT4_7		(1 << 4)	/* External interrupt 4 ~ 7 */
#define INT_EINT3		(1 << 3)	/* External interrupt 3 */
#define INT_EINT2		(1 << 2)	/* External interrupt 2 */
#define INT_EINT1		(1 << 1)	/* External interrupt 1 */
#define INT_EINT0		(1 << 0)	/* External interrupt 0 */

#define INT_ADC			(1 << 10)
#define INT_TC			(1 << 9)
#define INT_ERR2		(1 << 8)
#define INT_TXD2		(1 << 7)
#define INT_RXD2		(1 << 6)
#define INT_ERR1		(1 << 5)
#define INT_TXD1		(1 << 4)
#define INT_RXD1		(1 << 3)
#define INT_ERR0		(1 << 2)
#define INT_TXD0		(1 << 1)
#define INT_RXD0		(1 << 0)

/* Real Time Clock */
/* Registers */
#define bRTC(Nb)		__REG(0x57000000 + (Nb))
#define RTCCON			bRTC(0x40)
#define TICNT			bRTC(0x44)
#define RTCALM			bRTC(0x50)
#define ALMSEC			bRTC(0x54)
#define ALMMIN			bRTC(0x58)
#define ALMHOUR			bRTC(0x5c)
#define ALMDATE			bRTC(0x60)
#define ALMMON			bRTC(0x64)
#define ALMYEAR			bRTC(0x68)
#define RTCRST			bRTC(0x6c)
#define BCDSEC			bRTC(0x70)
#define BCDMIN			bRTC(0x74)
#define BCDHOUR			bRTC(0x78)
#define BCDDATE			bRTC(0x7c)
#define BCDDAY			bRTC(0x80)
#define BCDMON			bRTC(0x84)
#define BCDYEAR			bRTC(0x88)
/* Fields */
#define fRTC_SEC		Fld(7,0)
#define fRTC_MIN		Fld(7,0)
#define fRTC_HOUR		Fld(6,0)
#define fRTC_DATE		Fld(6,0)
#define fRTC_DAY		Fld(3,0)
#define fRTC_MON		Fld(5,0)
#define fRTC_YEAR		Fld(8,0)
/* Mask */
#define Msk_RTCSEC		FMsk(fRTC_SEC)
#define Msk_RTCMIN		FMsk(fRTC_MIN)
#define Msk_RTCHOUR		FMsk(fRTC_HOUR)
#define Msk_RTCDAY		FMsk(fRTC_DAY)
#define Msk_RTCDATE		FMsk(fRTC_DATE)
#define Msk_RTCMON		FMsk(fRTC_MON)
#define Msk_RTCYEAR		FMsk(fRTC_YEAR)
/* bits */
#define RTCCON_EN		(1 << 0) /* RTC Control Enable */
#define RTCCON_CLKSEL		(1 << 1) /* BCD clock as XTAL 1/2^25 clock */
#define RTCCON_CNTSEL		(1 << 2) /* 0: Merge BCD counters */
#define RTCCON_CLKRST		(1 << 3) /* RTC clock count reset */

/* RTC Alarm */
#define RTCALM_GLOBAL		(1 << 6) /* Global alarm enable */
#define RTCALM_YEAR		(1 << 5) /* Year alarm enable */
#define RTCALM_MON		(1 << 4) /* Month alarm enable */
#define RTCALM_DAY		(1 << 3) /* Day alarm enable */
#define RTCALM_HOUR		(1 << 2) /* Hour alarm enable */
#define RTCALM_MIN		(1 << 1) /* Minute alarm enable */
#define RTCALM_SEC		(1 << 0) /* Second alarm enable */
#define RTCALM_EN		(RTCALM_GLOBAL | RTCALM_YEAR | RTCALM_MON |\
				RTCALM_DAY | RTCALM_HOUR | RTCALM_MIN |\
				RTCALM_SEC)
#define RTCALM_DIS		(~RTCALM_EN)

/* PWM Timer */
#define bPWM_TIMER(Nb)		__REG(0x51000000 + (Nb))
#define bPWM_BUFn(Nb,x)		bPWM_TIMER(0x0c + (Nb)*0x0c + (x))
/* Registers */
#define TCFG0			bPWM_TIMER(0x00)
#define TCFG1			bPWM_TIMER(0x04)
#define TCON			bPWM_TIMER(0x08)
#define TCNTB0			bPWM_BUFn(0,0x0)
#define TCMPB0			bPWM_BUFn(0,0x4)
#define TCNTO0			bPWM_BUFn(0,0x8)
#define TCNTB1			bPWM_BUFn(1,0x0)
#define TCMPB1			bPWM_BUFn(1,0x4)
#define TCNTO1			bPWM_BUFn(1,0x8)
#define TCNTB2			bPWM_BUFn(2,0x0)
#define TCMPB2			bPWM_BUFn(2,0x4)
#define TCNTO2			bPWM_BUFn(2,0x8)
#define TCNTB3			bPWM_BUFn(3,0x0)
#define TCMPB3			bPWM_BUFn(3,0x4)
#define TCNTO3			bPWM_BUFn(3,0x8)
#define TCNTB4			bPWM_BUFn(4,0x0)
#define TCNTO4			bPWM_BUFn(4,0x4)
/* Fields */
#define fTCFG0_DZONE		Fld(8,16)	/* the dead zone length (= timer 0) */
#define fTCFG0_PRE1		Fld(8,8)	/* prescaler value for time 2,3,4 */
#define fTCFG0_PRE0		Fld(8,0)        /* prescaler value for time 0,1 */
#define fTCON_TIMER0	Fld(5,0)
#define fTCON_TIMER1	Fld(4,8)
#define fTCON_TIMER2	Fld(4,12)
#define fTCON_TIMER3	Fld(4,16)

/* bits */
#define TCFG0_DZONE(x)		FInsrt((x), fTCFG0_DZONE)
#define TCFG0_PRE1(x)		FInsrt((x), fTCFG0_PRE1)
#define TCFG0_PRE0(x)		FInsrt((x), fTCFG0_PRE0)


/* 031027 timer config register set 
 * 5-MUX & DMA mode selecton register
 */
#define TCFG1_DMACH(n)      ( (n+1)   << 20)     // n : dma channel no if n is 0 timer0 or  1 timer1 ...
#define TCFG1_MUX4(n)       (((n)>>2) << 16)     // n prescale value if n is 2, 1/2
#define TCFG1_MUX3(n)       (((n)>>2) << 12)     // if n is 4 ,prescale value is 1/4
#define TCFG1_MUX2(n)       (((n)>>2) <<  8)     // if n is 8 ,prescale value is 1/8
#define TCFG1_MUX1(n)       (((n)>>2) <<  4)     // if n is 16 , prescale value is 1/16
#define TCFG1_MUX0(n)       (((n)>>2) <<  0)     // if over 16 , External TCLK0i



#define TCON_4_AUTO	(1 << 22)	/* auto reload on/off for Timer 4 */
#define TCON_4_UPDATE	(1 << 21)	/* manual Update TCNTB4 */
#define TCON_4_ONOFF	(1 << 20)	/* 0: Stop, 1: start Timer 4 */
#define COUNT_4_ON	(TCON_4_ONOFF*1)
#define COUNT_4_OFF	(TCON_4_ONOFF*0)
#define TCON_3_AUTO     (1 << 19)       /* auto reload on/off for Timer 3 */
#define TCON_3_INVERT   (1 << 18)       /* 1: Inverter on for TOUT3 */
#define TCON_3_MAN      (1 << 17)       /* manual Update TCNTB3,TCMPB3 */
#define TCON_3_ONOFF    (1 << 16)       /* 0: Stop, 1: start Timer 3 */
#define TCON_2_AUTO     (1 << 15)       /* auto reload on/off for Timer 3 */
#define TCON_2_INVERT   (1 << 14)       /* 1: Inverter on for TOUT3 */
#define TCON_2_MAN      (1 << 13)       /* manual Update TCNTB3,TCMPB3 */
#define TCON_2_ONOFF    (1 << 12)       /* 0: Stop, 1: start Timer 3 */
#define TCON_1_AUTO     (1 << 11)       /* auto reload on/off for Timer 3 */
#define TCON_1_INVERT   (1 << 10)       /* 1: Inverter on for TOUT3 */
#define TCON_1_MAN      (1 << 9)       /* manual Update TCNTB3,TCMPB3 */
#define TCON_1_ONOFF    (1 << 8)       /* 0: Stop, 1: start Timer 3 */
#define COUNT_1_ON      (TCON_1_ONOFF*1)
#define COUNT_1_OFF     (TCON_1_ONOFF*0)
#define TCON_0_AUTO     (1 << 3)       /* auto reload on/off for Timer 3 */
#define TCON_0_INVERT   (1 << 2)       /* 1: Inverter on for TOUT3 */
#define TCON_0_MAN      (1 << 1)       /* manual Update TCNTB3,TCMPB3 */
#define TCON_0_ONOFF    (1 << 0)       /* 0: Stop, 1: start Timer 3 */
#define COUNT_0_ON	(TCON_0_ONOFF*1)
#define COUNT_0_OFF	(TCON_0_ONOFF*0)

#define TIMER3_ATLOAD_ON        (TCON_3_AUTO*1)
#define TIMER3_ATLAOD_OFF       FClrBit(TCON, TCON_3_AUTO)
#define TIMER3_IVT_ON   (TCON_3_INVERT*1)
#define TIMER3_IVT_OFF  (FClrBit(TCON, TCON_3_INVERT))
#define TIMER3_MANUP    (TCON_3_MAN*1)
#define TIMER3_NOP      (FClrBit(TCON, TCON_3_MAN))
#define TIMER3_ON       (TCON_3_ONOFF*1)
#define TIMER3_OFF      (FClrBit(TCON, TCON_3_ONOFF))
#define TIMER2_ATLOAD_ON        (TCON_2_AUTO*1)
#define TIMER2_ATLAOD_OFF       FClrBit(TCON, TCON_2_AUTO)
#define TIMER2_IVT_ON   (TCON_2_INVERT*1)
#define TIMER2_IVT_OFF  (FClrBit(TCON, TCON_2_INVERT))
#define TIMER2_MANUP    (TCON_2_MAN*1)
#define TIMER2_NOP      (FClrBit(TCON, TCON_2_MAN))
#define TIMER2_ON       (TCON_2_ONOFF*1)
#define TIMER2_OFF      (FClrBit(TCON, TCON_2_ONOFF))
#define TIMER1_ATLOAD_ON        (TCON_1_AUTO*1)
#define TIMER1_ATLAOD_OFF       FClrBit(TCON, TCON_1_AUTO)
#define TIMER1_IVT_ON   (TCON_1_INVERT*1)
#define TIMER1_IVT_OFF  (FClrBit(TCON, TCON_1_INVERT))
#define TIMER1_MANUP    (TCON_1_MAN*1)
#define TIMER1_NOP      (FClrBit(TCON, TCON_1_MAN))
#define TIMER1_ON       (TCON_1_ONOFF*1)
#define TIMER1_OFF      (FClrBit(TCON, TCON_1_ONOFF))
#define TIMER0_ATLOAD_ON        (TCON_0_AUTO*1)
#define TIMER0_ATLAOD_OFF       FClrBit(TCON, TCON_0_AUTO)
#define TIMER0_IVT_ON   (TCON_0_INVERT*1)
#define TIMER0_IVT_OFF  (FClrBit(TCON, TCON_0_INVERT))
#define TIMER0_MANUP    (TCON_0_MAN*1)
#define TIMER0_NOP      (FClrBit(TCON, TCON_0_MAN))
#define TIMER0_ON       (TCON_0_ONOFF*1)
#define TIMER0_OFF      (FClrBit(TCON, TCON_0_ONOFF))

#define TCON_TIMER1_CLR   FClrFld(TCON, fTCON_TIMER1);
#define TCON_TIMER2_CLR   FClrFld(TCON, fTCON_TIMER2);
#define TCON_TIMER3_CLR   FClrFld(TCON, fTCON_TIMER3);


/*
 * LCD Controller (Page 15-23)
 *
 * Register
   LCDCON1	LCD Control 1				[word, R/W, 0x00000000]
   LCDCON2	LCD Control 2				[word, R/W, 0x00000000]
   LCDCON3	LCD Control 3				[word, R/W, 0x00000000]
   LCDCON4	LCD Control 4				[word, R/W, 0x00000000]
   LCDCON5	LCD Control 5				[word, R/W, 0x00000000]
   LCDADDR1	STN/TFT: Frame Buffer Start Addr1	[word, R/W, 0x00000000]
   LCDADDR2	STN/TFT: Frame Buffer Start Addr2	[word, R/W, 0x00000000]
   LCDADDR3	STN/TFT: Virtual Screen Address Set	[word, R/W, 0x00000000]
   REDLUT	STN: Red Lookup Table			[word, R/W, 0x00000000]
   GREENLUT	STN: Green Lookup Table			[word, R/W, 0x00000000]
   BLUELUT	STN: Blue Lookup Table			[word, R/W, 0x0000]
   DP1_2	STN: Dithering Pattern Duty 1/2		[word, R/W]
   DP4_7	STN: Dithering Pattern Duty 4/7		[word, R/W]
   DP3_5	STN: Dithering Pattern Duty 3/5		[word, R/W]
   DP2_3	STN: Dithering Pattern Duty 2/3		[word, R/W]
   DP5_7	STN: Dithering Pattern Duty 5/7		[word, R/W]
   DP3_4	STN: Dithering Pattern Duty 3/4		[word, R/W]
   DP4_5	STN: Dithering Pattern Duty 4/5		[word, R/W]
   DP6_7	STN: Dithering Pattern Duty 6/7		[word, R/W]
   DITHMODE	STN: Dithering Mode			[word, R/W, 0x00000000]
   TPAL		TFT: Temporary Pallete			[word, R/W, 0x00000000]
 *
 */

#define bLCD_CTL(Nb)	__REG(0x4d000000 + (Nb))
#define LCDCON1		bLCD_CTL(0x00)
#define LCDCON2		bLCD_CTL(0x04)
#define LCDCON3		bLCD_CTL(0x08)
#define LCDCON4		bLCD_CTL(0x0c)
#define LCDCON5		bLCD_CTL(0x10)
#define LCDADDR1	bLCD_CTL(0x14)
#define LCDADDR2	bLCD_CTL(0x18)
#define LCDADDR3	bLCD_CTL(0x1c)
#define REDLUT		bLCD_CTL(0x20)
#define GREENLUT	bLCD_CTL(0x24)
#define BLUELUT		bLCD_CTL(0x28)
#define DITHMODE	bLCD_CTL(0x4c)
#define TPAL		bLCD_CTL(0x50)
#define LCDINTPND	bLCD_CTL(0x54)
#define LCDSRCPND	bLCD_CTL(0x58)
#define LCDINTMSK	bLCD_CTL(0x5c)
#define LCDLPCSEL	bLCD_CTL(0x60)
#define TCONSEL         bLCD_CTL(0x60) 

#define fLCD1_LINECNT	Fld(10,18)	/* the status of the line counter */
#define LCD1_LINECNT	FMsk(fLCD_LINECNT)

#define fLCD1_CLKVAL	Fld(10,8)	/* rates of VCLK and CLKVAL[9:0] */
#define LCD1_CLKVAL(x)	FInsrt((x), fLCD1_CLKVAL)
#define LCD1_CLKVAL_MSK	FMsk(fLCD1_CLKVAL)

#define LCD1_MMODE (1<<7)

#define fLCD1_PNR	Fld(2,5)	/* select the display mode */
#define LCD1_PNR_4D	FInsrt(0x0, fLCD1_PNR)	/* STN: 4-bit dual scan */
#define LCD1_PNR_4S	FInsrt(0x1, fLCD1_PNR)	/* STN: 4-bit single scan */
#define LCD1_PNR_8S	FInsrt(0x2, fLCD1_PNR)	/* STN: 8-bit single scan */
#define LCD1_PNR_TFT	FInsrt(0x3, fLCD1_PNR)	/* TFT LCD */

#define fLCD1_BPP	Fld(4,1)	/* select BPP(Bit Per Pixel) */
#ifdef CONFIG_ARCH_S3C2440A
#define LCD1_BPP_1S	FInsrt(0x0, fLCD1_BPP)  /* STN: 1bpp mono */
#define LCD1_BPP_2S	FInsrt(0x1, fLCD1_BPP)  /* STN: 2bpp 4-gray */
#define LCD1_BPP_4S	FInsrt(0x2, fLCD1_BPP)  /* STN: 4bpp 16-gray */
#define LCD1_BPP_8S	FInsrt(0x3, fLCD1_BPP)  /* STN: 8bpp color(256) */
#define LCD1_BPP_12PS	FInsrt(0x4, fLCD1_BPP)  /* STN: packed 12bpp color(4096) */
#define LCD1_BPP_12UPS	FInsrt(0x5, fLCD1_BPP)  /* STN: unpacked 12bpp color(4096) */
#define LCD1_BPP_16S	FInsrt(0x6, fLCD1_BPP)  /* STN: 16bpp color(4096) */
#define LCD1_BPP_1T	FInsrt(0x8, fLCD1_BPP)  /* TFT: 1bpp TFT COLOR  */
#define LCD1_BPP_2T	FInsrt(0x9, fLCD1_BPP)  /* TFT: 2bpp TFT COLOR */
#define LCD1_BPP_4T	FInsrt(0xa, fLCD1_BPP)  /* TFT: 4bpp TFT COLOR */
#define LCD1_BPP_8T	FInsrt(0xb, fLCD1_BPP)  /* TFT: 8bpp TFT COLOR */
#define LCD1_BPP_16T	FInsrt(0xc, fLCD1_BPP)  /* TFT: 16bpp TFT COLOR */
#define LCD1_BPP_24T	FInsrt(0xd, fLCD1_BPP)  /* TFT: 24bpp TFT COLOR */
#else
#define LCD1_BPP_1S	FInsrt(0x0, fLCD1_BPP)	/* STN: 1 bpp, mono */
#define LCD1_BPP_2S	FInsrt(0x1, fLCD1_BPP)	/* STN: 2 bpp, 4-grey */
#define LCD1_BPP_4S	FInsrt(0x2, fLCD1_BPP)	/* STN: 4 bpp, 16-grey */
#define LCD1_BPP_8S	FInsrt(0x3, fLCD1_BPP)	/* STN: 8 bpp, color */
#define LCD1_BPP_12S	FInsrt(0x4, fLCD1_BPP)	/* STN: 12 bpp, color */
#define LCD1_BPP_1T	FInsrt(0x8, fLCD1_BPP)	/* TFT: 1 bpp */
#define LCD1_BPP_2T	FInsrt(0x9, fLCD1_BPP)	/* TFT: 2 bpp */
#define LCD1_BPP_4T	FInsrt(0xa, fLCD1_BPP)	/* TFT: 4 bpp */
#define LCD1_BPP_8T	FInsrt(0xb, fLCD1_BPP)	/* TFT: 8 bpp */
#define LCD1_BPP_16T	FInsrt(0xc, fLCD1_BPP)	/* TFT: 16 bpp */
#endif

#define LCD1_ENVID	(1 << 0)	/* 1: Enable the video output */

#define fLCD2_VBPD	Fld(8,24)	/* TFT: (Vertical Back Porch)
	   # of inactive lines at the start of a frame,
	   after vertical synchronization period. */
#define LCD2_VBPD(x)	FInsrt((x), fLCD2_VBPD)

#define fLCD2_LINEVAL	Fld(10,14)	/* TFT/STN: vertical size of LCD */
#define LCD2_LINEVAL(x)	FInsrt((x), fLCD2_LINEVAL)
#define LCD2_LINEVAL_MSK	FMsk(fLCD2_LINEVAL)

#define fLCD2_VFPD	Fld(8,6)	/* TFT: (Vertical Front Porch)
	   # of inactive lines at the end of a frame,
	   before vertical synchronization period. */
//#define LCD2_VFPD	FMsk(fLCD2_VFPD)
#define LCD2_VFPD(x)	FInsrt((x), fLCD2_VFPD)

#define fLCD2_VSPW	Fld(6,0)	/* TFT: (Vertical Sync Pulse Width)
	   the VSYNC pulse's high level width
	   by counting the # of inactive lines */
//#define LCD2_VSPW	FMsk(fLCD2_VSPW)
#define LCD2_VSPW(x)	FInsrt((x), fLCD2_VSPW)

#define fLCD3_HBPD	Fld(7,19)	/* TFT: (Horizontal Back Porch)
	   # of VCLK periods between the falling edge of HSYNC
	   and the start of active data */
//#define LCD3_HBPD	FMsk(fLCD3_HBPD)
#define LCD3_HBPD(x)	FInsrt((x), fLCD3_HBPD)

#define fLCD3_WDLY	Fld(7,19)	/* STN: delay between VLINE and
	   VCLK by counting the # of the HCLK */
#define LCD3_WDLY	FMsk(fLCD3_WDLY)
#define LCD3_WDLY	FMsk(fLCD3_WDLY)
#define LCD3_WDLY_16	FInsrt(0x0, fLCD3_WDLY)	/* 16 clock */
#define LCD3_WDLY_32	FInsrt(0x1, fLCD3_WDLY)	/* 32 clock */
#define LCD3_WDLY_64	FInsrt(0x2, fLCD3_WDLY)	/* 64 clock */
#define LCD3_WDLY_128	FInsrt(0x3, fLCD3_WDLY)	/* 128 clock */

#define fLCD3_HOZVAL	Fld(11,8)	/* horizontal size of LCD */
#define LCD3_HOZVAL(x)	FInsrt((x), fLCD3_HOZVAL)
#define LCD3_HOZVAL_MSK	FMsk(fLCD3_HOZVAL)

#define fLCD3_HFPD	Fld(8,0)	/* TFT: (Horizontal Front Porch)
	   # of VCLK periods between the end of active date
	   and the rising edge of HSYNC */
#define LCD3_HFPD(x)	FInsrt((x), fLCD3_HFPD)

#define fLCD3_LINEBLNK	Fld(8,0)	/* STN: the blank time
	   in one horizontal line duration time.
	   the unit of LINEBLNK is HCLK x 8 */
#define LCD3_LINEBLNK	FInsrt(fLCD3_LINEBLNK)

#define fLCD4_MVAL	Fld(8,8)	/* STN: the rate at which the VM signal
	   will toggle if the MMODE bit is set logic '1' */
#define LCD4_MVAL(x)	FInsrt((x), fLCD4_MVAL)

#define fLCD4_HSPW	Fld(8,0)	/* TFT: (Horizontal Sync Pulse Width)
	   HSYNC pulse's high lvel width by counting the # of the VCLK */
#define LCD4_HSPW(x)	FInsrt((x), fLCD4_HSPW)

#define fLCD4_WLH	Fld(8,0)	/* STN: VLINE pulse's high level width
	   by counting the # of the HCLK */
#define LCD4_WLH(x)	FInsrt((x), fLCD4_WLH)
#define LCD4_WLH_16	FInsrt(0x0, fLCD4_WLH)	/* 16 clock */
#define LCD4_WLH_32	FInsrt(0x1, fLCD4_WLH)	/* 32 clock */
#define LCD4_WLH_64	FInsrt(0x2, fLCD4_WLH)	/* 64 clock */
#define LCD4_WLH_128	FInsrt(0x3, fLCD4_WLH)	/* 128 clock */

#define fLCD5_VSTAT	Fld(2,15)	/* TFT: Vertical Status (ReadOnly) */
#define LCD5_VSTAT	FMsk(fLCD5_VSTAT)
#define LCD5_VSTAT_VS	0x00	/* VSYNC */
#define LCD5_VSTAT_BP	0x01	/* Back Porch */
#define LCD5_VSTAT_AC	0x02	/* Active */
#define LCD5_VSTAT_FP	0x03	/* Front Porch */

#define fLCD5_HSTAT	Fld(2,13)	/* TFT: Horizontal Status (ReadOnly) */
#define LCD5_HSTAT	FMsk(fLCD5_HSTAT)
#define LCD5_HSTAT_HS	0x00	/* HSYNC */
#define LCD5_HSTAT_BP	0x01	/* Back Porch */
#define LCD5_HSTAT_AC	0x02	/* Active */
#define LCD5_HSTAT_FP	0x03	/* Front Porch */

#define LCD5_BPP24BL	(1 << 12)
#define LCD5_FRM565	(1 << 11)
#define LCD5_INVVCLK	(1 << 10)	/* STN/TFT :
	   1 : video data is fetched at VCLK falling edge
	   0 : video data is fetched at VCLK rising edge */
#define LCD5_INVVLINE	(1 << 9)	/* STN/TFT :
	   1 : VLINE/HSYNC pulse polarity is inverted */
#define LCD5_INVVFRAME	(1 << 8)	/* STN/TFT :
	   1 : VFRAME/VSYNC pulse polarity is inverted */
#define LCD5_INVVD	(1 << 7)	/* STN/TFT :
	   1 : VD (video data) pulse polarity is inverted */
#define LCD5_INVVDEN	(1 << 6)	/* TFT :
	   1 : VDEN signal polarity is inverted */
#define LCD5_INVPWREN	(1 << 5)
#define LCD5_INVLEND	(1 << 4)	/* TFT :
	   1 : LEND signal polarity is inverted */
#define LCD5_PWREN	(1 << 3)
#define LCD5_LEND	(1 << 2)	/* TFT,1 : Enable LEND signal */
#define LCD5_BSWP	(1 << 1)	/* STN/TFT,1 : Byte swap enable */
#define LCD5_HWSWP	(1 << 0)	/* STN/TFT,1 : HalfWord swap enable */

#define fLCDADDR_BANK	Fld(9,21)	/* bank location for video buffer */
#define LCDADDR_BANK(x)	FInsrt((x), fLCDADDR_BANK)

#define fLCDADDR_BASEU	Fld(21,0)	/* address of upper left corner */
#define LCDADDR_BASEU(x)	FInsrt((x), fLCDADDR_BASEU)

#define fLCDADDR_BASEL	Fld(21,0)	/* address of lower right corner */
#define LCDADDR_BASEL(x)	FInsrt((x), fLCDADDR_BASEL)

#define fLCDADDR_OFFSET	Fld(11,11)	/* Virtual screen offset size
					   (# of half words) */
#define LCDADDR_OFFSET(x)	FInsrt((x), fLCDADDR_OFFSET)

#define fLCDADDR_PAGE	Fld(11,0)	/* Virtual screen page width
					   (# of half words) */
#define LCDADDR_PAGE(x)	FInsrt((x), fLCDADDR_PAGE)

#define TPAL_LEN	(1 << 24)	/* 1 : Temp. Pallete Register enable */
#define fTPAL_VAL	Fld(24,0)	/* Temp. Pallete Register value */
#define TPAL_VAL(x)	FInsrt((x), fTPAL_VAL)
#define TPAL_VAL_RED(x)	FInsrt((x), Fld(8,16))
#define TPAL_VAL_GREEN(x)	FInsrt((x), Fld(8,8))
#define TPAL_VAL_BLUE(x)	FInsrt((x), Fld(8,0))

#define LCD_INT_FRSYN   2


/* ADC and Touch Screen Interface */
#define ADC_CTL_BASE		0x58000000
#define bADC_CTL(Nb)		__REG(ADC_CTL_BASE + (Nb))
/* Offset */
#define oADCCON			0x00	/* R/W, ADC control register */
#define oADCTSC			0x04	/* R/W, ADC touch screen ctl reg */
#define oADCDLY			0x08	/* R/W, ADC start or interval delay reg */
#define oADCDAT0		0x0c	/* R  , ADC conversion data reg */
#define oADCDAT1		0x10	/* R  , ADC conversion data reg */
#define oADC_UPDN         	0x14    /* R/W Stylus Up or Down Interrpt status register*/

/* Registers */
#define ADCCON			bADC_CTL(oADCCON)
#define ADCTSC			bADC_CTL(oADCTSC)
#define ADCDLY			bADC_CTL(oADCDLY)
#define ADCDAT0			bADC_CTL(oADCDAT0)
#define ADCDAT1			bADC_CTL(oADCDAT1)
#define PEN_UPDN		bADC_CTL(oADC_UPDN)
/* ADCUPDN register set */
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

/* DMA */
#define DMA_CTL_BASE	0x4b000000
#define bDMA_CTL(Nb,x)	__REG(DMA_CTL_BASE + (0x40*Nb) + (x))
/* DMA channel 0 */
#define DISRC0			bDMA_CTL(0, 0x00)
#define DISRCC0			bDMA_CTL(0, 0x04)
#define DIDST0			bDMA_CTL(0, 0x08)
#define DIDSTC0			bDMA_CTL(0, 0x0c)
#define DCON0			bDMA_CTL(0, 0x10)
#define DSTAT0			bDMA_CTL(0, 0x14)
#define DCSRC0			bDMA_CTL(0, 0x18)
#define DCDST0			bDMA_CTL(0, 0x1c)
#define DMTRIG0			bDMA_CTL(0, 0x20)
/* DMA channel 1 */
#define DISRC1			bDMA_CTL(1, 0x00)
#define DISRCC1			bDMA_CTL(1, 0x04)
#define DIDST1			bDMA_CTL(1, 0x08)
#define DIDSTC1			bDMA_CTL(1, 0x0c)
#define DCON1			bDMA_CTL(1, 0x10)
#define DSTAT1			bDMA_CTL(1, 0x14)
#define DCSRC1			bDMA_CTL(1, 0x18)
#define DCDST1			bDMA_CTL(1, 0x1c)
#define DMTRIG1			bDMA_CTL(1, 0x20)
/* DMA channel 2 */
#define DISRC2			bDMA_CTL(2, 0x00)
#define DISRCC2			bDMA_CTL(2, 0x04)
#define DIDST2			bDMA_CTL(2, 0x08)
#define DIDSTC2			bDMA_CTL(2, 0x0c)
#define DCON2			bDMA_CTL(2, 0x10)
#define DSTAT2			bDMA_CTL(2, 0x14)
#define DCSRC2			bDMA_CTL(2, 0x18)
#define DCDST2			bDMA_CTL(2, 0x1c)
#define DMTRIG2			bDMA_CTL(2, 0x20)
/* DMA channel 3 */
#define DISRC3			bDMA_CTL(3, 0x00)
#define DISRCC3			bDMA_CTL(3, 0x04)
#define DIDST3			bDMA_CTL(3, 0x08)
#define DIDSTC3			bDMA_CTL(3, 0x0c)
#define DCON3			bDMA_CTL(3, 0x10)
#define DSTAT3			bDMA_CTL(3, 0x14)
#define DCSRC3			bDMA_CTL(3, 0x18)
#define DCDST3			bDMA_CTL(3, 0x1c)
#define DMTRIG3			bDMA_CTL(3, 0x20)

/* DISRC, DIDST Control registers */
#define fDMA_BASE_ADDR		Fld(30, 0)      /* base address of src/dst data */
#define DMA_BASE_ADDR(x)	FInsrt(x, fDMA_BASE_ADDR)
#define LOC_SRC			(1 << 1)	/* select the location of source */
#define ON_AHB			(LOC_SRC*0)
#define ON_APB			(LOC_SRC*1)
#define ADDR_MODE		(1 << 0)       /* select the address increment */
#define ADDR_INC		(ADDR_MODE*0)
#define ADDR_FIX		(ADDR_MODE*1)

#if defined(CONFIG_ARCH_S3C2440)
#define CHK_INT			(1 << 2)  	/* TC reaches 0 & Autoreload perform */
#endif

/* DCON Definitions */
#define DCON_MODE		(1 << 31)	/* 0: demand, 1: handshake */
#define DEMAND_MODE		(DCON_MODE*0)
#define HS_MODE			(DCON_MODE*1)
#define DCON_SYNC		(1 << 30)       /* sync to 0:PCLK, 1:HCLK */
#define SYNC_PCLK		(DCON_SYNC*0)
#define SYNC_HCLK		(DCON_SYNC*1)
#define DCON_INT		(1 << 29)
#define POLLING_MODE		(DCON_INT*0)
#define INT_MODE		(DCON_INT*1)
#define DCON_TSZ		(1 << 28)	/* tx size 0: a unit, 1: burst */
#define TSZ_UNIT		(DCON_TSZ*0)
#define TSZ_BURST		(DCON_TSZ*1)
#define DCON_SERVMODE		(1 << 27)	/* 0: single, 1: whole service */
#define SINGLE_SERVICE		(DCON_SERVMODE*0)
#define WHOLE_SERVICE		(DCON_SERVMODE*1)
#define fDCON_HWSRC		Fld(3, 24)	/* select request source */
#define CH0_nXDREQ0		0
#define CH0_UART0		1
#define CH0_MMC			2
#define CH0_TIMER		3
#define CH0_USBEP1		4
#define CH0_I2SSDO		5	/* S3C2440A DMA SUPPORT*/
#define CH0_PCMIN		6	/* S3C2440A DMA SUPPORT*/
#define CH1_nXDREQ1		0
#define CH1_UART1		1
#define CH1_I2SSDI		2
#define CH1_SPI			3
#define CH1_USBEP2		4
#define CH1_PCMOUT		5	/* S3C2440A DMA SUPPORT*/	
#define	CH1_SDI			6	/* S3C2440A DMA SUPPORT*/
#define CH2_I2SSDO		0
#define CH2_I2SSDI		1
#define CH2_MMC			2
#define CH2_TIMER		3
#define CH2_USBEP3		4
#define CH2_PCMIN		5	/* S3C2440A DMA SUPPORT*/
#define CH2_MICIN		6	/* S3C2440A DMA SUPPORT*/
#define CH3_UART2		0
#define CH3_MMC			1
#define CH3_SPI			2
#define CH3_TIMER		3
#define CH3_USBEP4		4
#define CH3_MICIN		5	/* S3C2440A DMA SUPPORT*/
#define CH3_PCMOUT		6	/* S3C2440A DMA SUPPORT*/
#define HWSRC(x)		FInsrt(x, fDCON_HWSRC)
#define DCON_SWHW_SEL		(1 << 23)	/* DMA src 0: s/w 1: h/w */
#define DMA_SRC_SW		(DCON_SWHW_SEL*0)
#define DMA_SRC_HW		(DCON_SWHW_SEL*1)
#define DCON_RELOAD		(1 << 22)	/* set auto-reload */
#define SET_ATRELOAD		(DCON_RELOAD*0)
#define CLR_ATRELOAD		(DCON_RELOAD*1)
#define fDCON_DSZ		Fld(2, 20)
#define DSZ_BYTE		0
#define DSZ_HALFWORD		1
#define DSZ_WORD		2
#define DSZ(x)			FInsrt(x, fDCON_DSZ)
#define readDSZ(x)		FExtr(x, fDCON_DSZ)
#define fDCON_TC		Fld(20,0)
#define TX_CNT(x)		FInsrt(x, fDCON_TC)
/* STATUS Register Definitions  */
#define fDSTAT_ST		Fld(2,20)	/* Status of DMA Controller */
#define fDSTAT_TC		Fld(20,0)	/* Current value of transfer count */
#define DMA_STATUS(chan)	FExtr((DSTAT0 + (0x20 * chan)), fDSTAT_ST)
#define DMA_BUSY		(1 << 0)
#define DMA_READY		(0 << 0)
#define DMA_CURR_TC(chan)	FExtr((DSTAT0 + (0x20 * chan)), fDSTAT_TC)      
/* DMA Trigger Register Definitions */
#define DMASKTRIG_STOP		(1 << 2)	/* Stop the DMA operation */
#define DMA_STOP		(DMASKTRIG_STOP*1)
#define DMA_STOP_CLR		(DMASKTRIG_STOP*0)
#define DMASKTRIG_ONOFF		(1 << 1)	/* DMA channel on/off */
#define CHANNEL_ON		(DMASKTRIG_ONOFF*1)
#define CHANNEL_OFF		(DMASKTRIG_ONOFF*0)
#define DMASKTRIG_SW		(1 << 0)	/* Trigger DMA ch. in S/W req. mode */
#define DMA_SW_REQ_CLR		(DMASKTRIG_SW*0)
#define DMA_SW_REQ		(DMASKTRIG_SW*1)

/* IIS Bus Interface */
#define IIS_CTL_BASE		0x55000000
#define bIIS_CTL(Nb)		__REG(IIS_CTL_BASE + (Nb))
#define IISCON			bIIS_CTL(0x00)
#define IISMOD			bIIS_CTL(0x04)
#define IISPSR			bIIS_CTL(0x08)
#define IISFIFOC		bIIS_CTL(0x0c)
#define IISFIFOE		bIIS_CTL(0x10)


#define IISCON_CH_RIGHT (1 << 8)        /* Right channel */
#define IISCON_CH_LEFT  (0 << 8)        /* Left channel */
#define IISCON_TX_RDY   (1 << 7)        /* Transmit FIFO is ready(not empty) */
#define IISCON_RX_RDY   (1 << 6)        /* Receive FIFO is ready (not full) */
#define IISCON_TX_DMA   (1 << 5)        /* Transmit DMA service reqeust */
#define IISCON_RX_DMA   (1 << 4)        /* Receive DMA service reqeust */
#define IISCON_TX_IDLE  (1 << 3)        /* Transmit Channel idle */
#define IISCON_RX_IDLE  (1 << 2)        /* Receive Channel idle */
#define IISCON_PRESCALE (1 << 1)        /* IIS Prescaler Enable */
#define IISCON_EN       (1 << 0)        /* IIS enable(start) */

#define IISMOD_SEL_PCLK (0 << 9)        /* Master clock 0:PCLK in */
#define IISMOD_SEL_MPLL (1 << 9)        /* Master clock MPLL in*/
#define IISMOD_SEL_MA   (0 << 8)        /* Master mode (IISLRCK, IISCLK are Output) */
#define IISMOD_SEL_SL   (1 << 8)        /* Slave mode  (IISLRCK, IISCLK are Input) */
#define fIISMOD_SEL_TR  Fld(2, 6)       /* Transmit/Receive mode */
#define IISMOD_SEL_TR   FMsk(fIISMOD_SEL_TR)
#define IISMOD_SEL_NO   FInsrt(0x0, fIISMOD_SEL_TR)     /* No Transfer */
#define IISMOD_SEL_RX   FInsrt(0x1, fIISMOD_SEL_TR)     /* Receive */
#define IISMOD_SEL_TX   FInsrt(0x2, fIISMOD_SEL_TR)     /* Transmit */
#define IISMOD_SEL_BOTH FInsrt(0x3, fIISMOD_SEL_TR)     /* Tx & Rx */
#define IISMOD_CH_RIGHT (0 << 5)        /* high for right channel */
#define IISMOD_CH_LEFT  (1 << 5)        /* high for left channel */
#define IISMOD_FMT_IIS  (0 << 4)        /* IIS-compatible format */
#define IISMOD_FMT_MSB  (1 << 4)        /* MSB(left)-justified format */
#define IISMOD_BIT_8    (0 << 3)        /* Serial data bit/channel is 8 bit*/
#define IISMOD_BIT_16   (1 << 3)        /* Serial data bit/channel is 16 bit*/
#define IISMOD_FREQ_256 (0 << 2)        /* Master clock freq = 256 fs */
#define IISMOD_FREQ_384 (1 << 2)        /* Master clock freq = 384 fs */
#define fIISMOD_SFREQ   Fld(2, 0)       /* Serial bit clock frequency */
#define IISMOD_SFREQ    FMsk(fIISMOD_SFREQ)     /* fs = sampling frequency */
#define IISMOD_SFREQ_16 FInsrt(0x0, fIISMOD_SFREQ)      /* 16 fs */
#define IISMOD_SFREQ_32 FInsrt(0x1, fIISMOD_SFREQ)      /* 32 fs */
#define IISMOD_SFREQ_48 FInsrt(0x2, fIISMOD_SFREQ)      /* 48 fs */

#define fIISPSR_A       Fld(5, 5)       /* Prescaler Control A */
#define IISPSR_A(x)     FInsrt((x), fIISPSR_A)
#define fIISPSR_B       Fld(5, 0)       /* Prescaler Control B */
#define IISPSR_B(x)     FInsrt((x), fIISPSR_B)  

#define IISFCON_TX_NORM (0 << 15)       /* Transmit FIFO access mode: normal */
#define IISFCON_TX_DMA  (1 << 15)       /* Transmit FIFO access mode: DMA */
#define IISFCON_RX_NORM (0 << 14)       /* Receive FIFO access mode: normal */
#define IISFCON_RX_DMA  (1 << 14)       /* Receive FIFO access mode: DMA */
#define IISFCON_TX_EN   (1 << 13)        /* Transmit FIFO enable */
#define IISFCON_RX_EN   (1 << 12)        /* Recevice FIFO enable */
#define fIISFCON_TX_CNT Fld(6, 6)       /* Tx FIFO data count (Read-Only) */
#define IISFCON_TX_CNT  FMsk(fIISFCON_TX_CNT)
#define fIISFCON_RX_CNT Fld(6, 0)       /* Rx FIFO data count (Read-Only) */
#define IISFCON_RX_CNT  FMsk(fIISFCON_RX_CNT)

/* USB Device - Little Endian : 
 * (B) : byte(8 bit) access
 * (W) : word(32 bit) access
 */
#define bUD(Nb)		__REG(0x52000000 + (Nb))
#define UD_FUNC		bUD(0x140) // Function address  (B)
#define UD_PWR		bUD(0x144) // Power management (B)
#define UD_INT		bUD(0x148) // Endpoint interrupt pending/clear (B)
#define UD_USBINT	bUD(0x158) // USB interrupt pending/clear (B)
#define UD_INTE		bUD(0x15c) // Endpoint interrupt enable (B)
#define UD_USBINTE	bUD(0x16c) // USB interrupt enable (B)
#define UD_FRAMEL	bUD(0x170) // Frame number low-byte (B)
#define UD_FRAMEH	bUD(0x174) // Frame number high-byte (B)
#define UD_INDEX	bUD(0x178) // Index (B)
#define UD_FIFO0	bUD(0x1c0) // Endpoint 0 FIFO (B)
#define UD_FIFO1	bUD(0x1c4) // Endpoint 1 FIFO (B)
#define UD_FIFO2	bUD(0x1c8) // Endpoint 2 FIFO (B)
#define UD_FIFO3	bUD(0x1cc) // Endpoint 3 FIFO (B)
#define UD_FIFO4	bUD(0x1d0) // Endpoint 4 FIFO (B)
#define UD_DMACON1	bUD(0x200) // Endpoint 1 DMA control (B)
#define UD_DMAUC1	bUD(0x204) // Endpoint 1 DMA unit counter (B)
#define UD_DMAFC1	bUD(0x208) // Endpoint 1 DMA FIFO counter
#define UD_DMATCL1	bUD(0x20c) // Endpoint 1 DMA Transfer counter low-byte
#define UD_DMATCM1	bUD(0x210) // Endpoint 1 DMA Transfer counter middle-byte
#define UD_DMATCH1	bUD(0x214) // Endpoint 1 DMA Transfer counter high-byte
#define UD_DMACON2	bUD(0x218) // Endpoint 2 DMA control (B)
#define UD_DMAUC2	bUD(0x21c) // Endpoint 2 DMA unit counter (B)
#define UD_DMAFC2	bUD(0x220) // Endpoint 2 DMA FIFO counter
#define UD_DMATCL2	bUD(0x224) // Endpoint 2 DMA Transfer counter low-byte
#define UD_DMATCM2	bUD(0x228) // Endpoint 2 DMA Transfer counter middle-byte
#define UD_DMATCH2	bUD(0x22c) // Endpoint 2 DMA Transfer counter high-byte
#define UD_DMACON3	bUD(0x240) // Endpoint 3 DMA control (B)
#define UD_DMAUC3	bUD(0x244) // Endpoint 3 DMA unit counter (B)
#define UD_DMAFC3	bUD(0x248) // Endpoint 3 DMA FIFO counter
#define UD_DMATCL3	bUD(0x24c) // Endpoint 3 DMA Transfer counter low-byte
#define UD_DMATCM3	bUD(0x250) // Endpoint 3 DMA Transfer counter middle-byte
#define UD_DMATCH3	bUD(0x254) // Endpoint 3 DMA Transfer counter high-byte
#define UD_DMACON4	bUD(0x258) // Endpoint 4 DMA control (B)
#define UD_DMAUC4	bUD(0x25c) // Endpoint 4 DMA unit counter (B)
#define UD_DMAFC4	bUD(0x260) // Endpoint 4 DMA FIFO counter
#define UD_DMATCL4	bUD(0x264) // Endpoint 4 DMA Transfer counter low-byte
#define UD_DMATCM4	bUD(0x268) // Endpoint 4 DMA Transfer counter middle-byte
#define UD_DMATCH4	bUD(0x26c) // Endpoint 4 DMA Transfer counter high-byte
#define UD_MAXP		bUD(0x180) // Endpoint MAX Packet
#define UD_ICSR1	bUD(0x184) // EP In control status register 1 (B)
#define UD_ICSR2	bUD(0x188) // EP In control status register 2 (B)
#define UD_OCSR1	bUD(0x190) // EP Out control status register 1 (B)
#define UD_OCSR2	bUD(0x194) // EP Out control status register 2 (B)
#define UD_OFCNTL	bUD(0x198) // EP Out Write counter low-byte (B)
#define UD_OFCNTH	bUD(0x19c) // EP Out Write counter high-byte (B)

#define UD_FUNC_UD	(1 << 7)
#define fUD_FUNC_ADDR	Fld(7,0)	/* USB Device Addr. assigned by host */
#define UD_FUNC_ADDR	FMsk(fUD_FUNC_ADDR)

#define UD_PWR_ISOUP	(1<<7) // R/W
#define UD_PWR_RESET	(1<<3) // R
#define UD_PWR_RESUME	(1<<2) // R/W
#define UD_PWR_SUSPND	(1<<1) // R
#define UD_PWR_ENSUSPND	(1<<0) // R/W

#define UD_PWR_DEFAULT	0x00

#define UD_INT_EP4	(1<<4)	// R/W (clear only)
#define UD_INT_EP3	(1<<3)	// R/W (clear only)
#define UD_INT_EP2	(1<<2)	// R/W (clear only)
#define UD_INT_EP1	(1<<1)	// R/W (clear only)
#define UD_INT_EP0	(1<<0)	// R/W (clear only)

#define UD_USBINT_RESET	(1<<2) // R/W (clear only)
#define UD_USBINT_RESUM	(1<<1) // R/W (clear only)
#define UD_USBINT_SUSPND (1<<0) // R/W (clear only)

#define UD_INTE_EP4	(1<<4) // R/W
#define UD_INTE_EP3	(1<<3) // R/W
#define UD_INTE_EP2	(1<<2) // R/W
#define UD_INTE_EP1	(1<<1) // R/W
#define UD_INTE_EP0	(1<<0) // R/W

#define UD_USBINTE_RESET	(1<<2) // R/W
#define UD_USBINTE_SUSPND	(1<<0) // R/W

#define fUD_FRAMEL_NUM	Fld(8,0) // R
#define UD_FRAMEL_NUM	FMsk(fUD_FRAMEL_NUM)

#define fUD_FRAMEH_NUM	Fld(8,0) // R
#define UD_FRAMEH_NUM	FMsk(fUD_FRAMEH_NUM)

#define UD_INDEX_EP0	(0x00)
#define UD_INDEX_EP1	(0x01) // ??
#define UD_INDEX_EP2	(0x02) // ??
#define UD_INDEX_EP3	(0x03) // ??
#define UD_INDEX_EP4	(0x04) // ??

#define UD_ICSR1_CLRDT	(1<<6)   // R/W
#define UD_ICSR1_SENTSTL (1<<5)  // R/W (clear only)
#define UD_ICSR1_SENDSTL (1<<4)  // R/W
#define UD_ICSR1_FFLUSH (1<<3)  // W	(set only)
#define UD_ICSR1_UNDRUN  (1<<2)  // R/W (clear only)
#define UD_ICSR1_PKTRDY	 (1<<0)  // R/W (set only)

#define UD_ICSR2_AUTOSET (1<<7) // R/W
#define UD_ICSR2_ISO	 (1<<6)	// R/W
#define UD_ICSR2_MODEIN	 (1<<5) // R/W
#define UD_ICSR2_DMAIEN	 (1<<4) // R/W

#define UD_OCSR1_CLRDT	(1<<7) // R/W
#define UD_OCSR1_SENTSTL	(1<<6)	// R/W (clear only)
#define UD_OCSR1_SENDSTL	(1<<5)	// R/W
#define UD_OCSR1_FFLUSH		(1<<4) // R/W
#define UD_OCSR1_DERROR		(1<<3) // R
#define UD_OCSR1_OVRRUN		(1<<2) // R/W (clear only)
#define UD_OCSR1_PKTRDY		(1<<0) // R/W (clear only)

#define UD_OCSR2_AUTOCLR	(1<<7) // R/W
#define UD_OCSR2_ISO		(1<<6) // R/W
#define UD_OCSR2_DMAIEN		(1<<5) // R/W

#define fUD_FIFO_DATA	Fld(8,0) // R/W
#define UD_FIFO0_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO1_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO2_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO3_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO4_DATA	FMsk(fUD_FIFO_DATA)

#define UD_MAXP_8	(1<<0)
#define UD_MAXP_16	(1<<1)
#define UD_MAXP_32	(1<<2)
#define UD_MAXP_64	(1<<3)

#define fUD_OFCNT_DATA	Fld(8,0)
#define UD_OFCNTL_DATA	FMsk(fUD_OFCNT_DATA) //R
#define UD_OFCNTH_DATA	FMsk(fUD_OFCNT_DATA) //R

#define UD_DMACONx_INRUNOB	(1<<7) // R
#define fUD_DMACON_STATE	Fld(3,4) // R
#define UD_DMACONx_STATE	FMsk(fUD_DMACON_STATE) // R/W
#define UD_DMACONx_DEMEN	(1<<3) // R/W
#define UD_DMACONx_ORUN		(1<<2) // R/W
#define UD_DMACONx_IRUN		(1<<1) // R/W
#define UD_DMACONx_DMAMODE	(1<<0) // R/W

#define fUD_DMAUC_DATA	Fld(8,0)
#define UD_DMAUCx_DATA	FMsk(fUD_DMAUC_DATA)

#define fUD_DMAFC_DATA	Fld(8,0)
#define UD_DMAFCx_DATA	FMsk(fUD_DMAFC_DATA)

#define fUD_DMATC_DATA	Fld(8,0)
#define UD_DMATCL_DATA	FMsk(fUD_DMATC_DATA)
#define UD_DMATCM_DATA	FMsk(fUD_DMATC_DATA)
#define UD_DMATCH_DATA	FMsk(fUD_DMATC_DATA)

#define EP0_CSR_OPKRDY	(1<<0)
#define EP0_CSR_IPKRDY	(1<<1)
#define EP0_CSR_SENTSTL	(1<<2)
#define EP0_CSR_DE	(1<<3)
#define EP0_CSR_SE	(1<<4)
#define EP0_CSR_SENDSTL	(1<<5)
#define EP0_CSR_SOPKTRDY (1<<6)
#define EP0_CSR_SSE	(1<<7)


/* USB Host - OHCI registers */
#define USBHOST_CTL_BASE 0x49000000

/*
 * SPI (Page 22-6)
 *
 * Register
   SPCON0	SPI Control				[byte, R/W, 0x00]
   SPSTA0	SPI Status				[byte, R  , 0x01]
   SPPIN0	SPI Pin Control				[byte, R/W, 0x02]
   SPPRE0	SPI Baud Rate Prescaler			[byte, R/W, 0x00]
   SPTDAT0	SPI Tx Data				[byte, R/W, 0x00]
   SPRDAT0	SPI Rx Data				[byte, R  , 0x00]

   SPCON1	SPI Control				[byte, R/W, 0x00]
   SPSTA1	SPI Status				[byte, R  , 0x01]
   SPPIN1	SPI Pin Control				[byte, R/W, 0x02]
   SPPRE1	SPI Baud Rate Prescaler			[byte, R/W, 0x00]
   SPTDAT1	SPI Tx Data				[byte, R/W, 0x00]
   SPRDAT1	SPI Rx Data				[byte, R  , 0x00]
 *
 */
#define bSPI(Nb)	__REG(0x59000000 + (Nb))
#define SPCON0		bSPI(0x00)
#define SPSTA0		bSPI(0x04)
#define SPPIN0		bSPI(0x08)
#define SPPRE0		bSPI(0x0c)
#define SPTDAT0		bSPI(0x10)
#define SPRDAT0		bSPI(0x14)
#define SPCON1		bSPI(0x20 + 0x00)
#define SPSTA1		bSPI(0x20 + 0x04)
#define SPPIN1		bSPI(0x20 + 0x08)
#define SPPRE1		bSPI(0x20 + 0x0c)
#define SPTDAT1		bSPI(0x20 + 0x10)
#define SPRDAT1		bSPI(0x20 + 0x14)

#define fSPCON_SMOD	Fld(2,5)	/* SPI Mode Select */
#define SPCON_SMOD	FMsk(fSPCON_SMOD)
#define SPCON_SMOD_POLL	FInsrt(0x0, fSPCON_SMOD)	/* polling mode */
#define SPCON_SMOD_INT	FInsrt(0x1, fSPCON_SMOD)	/* interrupt mode */
#define SPCON_SMOD_DMA	FInsrt(0x2, fSPCON_SMOD)	/* DMA mode */
#define SPCON_ENSCK	(1 << 4)	/* Enable SCK */
#define SPCON_MSTR	(1 << 3)	/* Master/Slave select
					   0: slave, 1: master */
#define SPCON_CPOL	(1 << 2)	/* Clock polarity select
					   1: active low, 0: active high */
#define SPCON_CPOL_LOW	(1 << 2)
#define SPCON_CPOL_HIGH	(0 << 2)
#define SPCON_CPHA	(1 << 1)	/* Clock Phase Select
					   0: format A, 1: format B */
#define SPCON_CPHA_FMTA	(0 << 1)
#define SPCON_CPHA_FMTB	(1 << 1)
#define SPCON_TAGD	(1 << 0)	/* Tx auto garbage data mode enable
			in normal mode, you only want to receive data,
					you should tranmit dummy 0xFF data */

#define SPSTA_DCOL	(1 << 2)	/* Data Collision Error */
#define SPSTA_MULF	(1 << 1)	/* Multi Master Error */
#define SPSTA_READY	(1 << 0)	/* data Tx/Rx ready */

#define SPPIN_ENMUL	(1 << 2)	/* Multi Master Error detect Enable */
#define SPPIN_KEEP	(1 << 0)	/* Master Out keep */


/*
 * SDI (Secure Digital Interface, Page 19-4)
 *
 * Register
   SDICON	SDI Control				[ 5, R/W, 0x0]
   SDIPRE	SDI Baud Rate Prescaler			[ 8, R/W, 0x0]
   SDICARG	SDI Command Argument			[32, R/W, 0x0]
   SDICCON	SDI Command Control			[13, R/W, 0x0]
   SDICSTA	SDI Command Status			[13, R/W, 0x0]
   SDIRSP0	SDI Response Register 0			[32, R  , 0x0]
   SDIRSP1	SDI Response Register 1			[32, R  , 0x0]
   SDIRSP2	SDI Response Register 2			[32, R  , 0x0]
   SDIRSP3	SDI Response Register 3			[32, R  , 0x0]
   SDIDTIMER	SDI Data/Busy Timer 			[16, R/W, 0x2000]
   SDIBSIZE	SDI Block Size				[12, R/W, 0x0]
   SDIDCON	SDI Data Control			[22, R/W, 0x0]
   SDIDCNT	SDI Data Domain Counter			[24, R  , 0x0]
   SDIDSTA	SDI Data Status				[11, R/W, 0x0]
   SDIFSTA	SDI FIFO Status				[14, R  , 0x0]
   SDIDAT	SDI Data				[32, R/W, 0x0]
   SDIIMSK	SDI Interrupt Mask			[18, R/W, 0x0]
 *
 */
/* Moved SD Definitions to Standalone module files */

#define GPIO_SDDAT3	GPIO_E10
#define GPIO_SDDAT2	GPIO_E9
#define GPIO_SDDAT1	GPIO_E8
#define GPIO_SDDAT0	GPIO_E7
#define GPIO_SDCMD	GPIO_E6
#define GPIO_SDCLK	GPIO_E5

/* IRDA TXD ENABLE */
#ifdef CONFIG_ARCH_S3C2440A
#define GPIO_IRDA_TXDEN  GPIO_B1
#endif
                                                                                

/*
 * Power Management
 */
#define SCLKE           (1 << 19)
#define SCLK1           (1 << 18)
#define SCLK0           (1 << 17)
#define USBSPD1         (1 << 13)
#define USBSPD0         (1 << 12)
#define PMST_WDR        (1 << 2)
#define PMST_SMR        (1 << 1)
#define PMST_HWR        (1 << 0)

/*
 * Watchdog timer
 */
#define bWTCON(Nb)	__REG(0x53000000 + (Nb))
#define WTCON		bWTCON(0)
#define WTDAT		bWTCON(4)
#define WTCNT		bWTCON(8)

/*------------------------------------------
 *   S3C2440 New feature for camera 
 -----------------------------------------*/
  
  /*
   *   Clock
   */
  
  #define oCAMDIVN                0x18    /* R/W, Camera clock divider  */
  #define CAMDIVN                 bCLKCTL(oCAMDIVN)
  #define DVS_EN                  (1 <<12) /* Dynamic Voltage Scaling enabl 0:OFF(fclk=MPLL) 1:ON FCLK=HCLK*/
  #define CAMCLK_SET_DIV          (1 << 4)
  #define HCLK4_HALF              (1 << 9) /* S3C2440A */ 
  #define HCLK3_HALF              (1 << 8) /* S3C2440A */ 
  #define CAMCLK_DIV_MSK          (0xf)
  #define CLKCON_CAMIF            (1 <<19)
  #define DIVN_UPLL_EN            (1 << 3)
  
  /* GPIO
   *     GPJCON has interface to CAMDATA, CAMPCLK, CAMVSYNC,
   *      CAMHREF, CAMPCLKOUT, CAMRESET
   *      GPJCON starts from 0x560000d0
   */
  
  #define PORTJ_OFS          0x0d
  
  #define GPIO_J0            MAKE_GPIO_NUM(PORTJ_OFS, 0)
  #define GPIO_J1            MAKE_GPIO_NUM(PORTJ_OFS, 1)
  #define GPIO_J2            MAKE_GPIO_NUM(PORTJ_OFS, 2)
  #define GPIO_J3            MAKE_GPIO_NUM(PORTJ_OFS, 3)
  #define GPIO_J4            MAKE_GPIO_NUM(PORTJ_OFS, 4)
  #define GPIO_J5            MAKE_GPIO_NUM(PORTJ_OFS, 5)
  #define GPIO_J6            MAKE_GPIO_NUM(PORTJ_OFS, 6)
  #define GPIO_J7            MAKE_GPIO_NUM(PORTJ_OFS, 7)
  #define GPIO_J8            MAKE_GPIO_NUM(PORTJ_OFS, 8)
  #define GPIO_J9            MAKE_GPIO_NUM(PORTJ_OFS, 9)
  #define GPIO_J10           MAKE_GPIO_NUM(PORTJ_OFS, 10)
  #define GPIO_J11           MAKE_GPIO_NUM(PORTJ_OFS, 11)
  #define GPIO_J12           MAKE_GPIO_NUM(PORTJ_OFS, 12)
  
  #define GPJCON             bGPIO(0xd0)
  #define GPJDAT             bGPIO(0xd4)
  #define GPJUP              bGPIO(0xdc)
  
  
  
/*
 * NAND
 *
 */
  
#include "s3c2440_nand.h"

/*------------------------------------------
 *   S3C2440 : from EOS
 -----------------------------------------*/
/* Slow clock control register */
#define CLKSLOW_UCLK_ON         (1<<7)
#define CLKSLOW_MPLL_OFF        (1<<5)
#define CLKSLOW_SLOW_BIT        (1<<4)
#define CLKSLOW_SLOW_VAL        FMsk(Fld(3,0))
                                                                                                                             
/* Clock divider control register */
#define CLKDIVN_HDIVN           (3<<0)
#define CLKDIVN_PDIVN           (1<<0)
#define CLKDIVN_MASK            0x7

/* I/O Port */
/* DSC(Drive Strength Control) */
#define DSC0                    bGPIO(0xc4)     /* DSC0: 0x560000c4 */
#define DSC1                    bGPIO(0xc8)     /* DSC1: 0x560000c8 */
/* MSLCON(Memory Sleep Control Reg.) */
#define MSLCON                  bGPIO(0xcc)     /* MSLCON: 0x560000cc */
                                                                                                                             
/* I/O Port control register test-value */
#define GPGCON_LCD_PWREN        (1<<4)
                                                                                                                             
/* Miscellaneous */
/* from EOS: our files uses this register */
#define MISCCR_nEN_SCKE         (1<<19)
#define MISCCR_nEN_SCLK1        (1<<18)
#define MISCCR_nEN_SCLK0        (1<<17)

#endif // _S3C2440_H_

