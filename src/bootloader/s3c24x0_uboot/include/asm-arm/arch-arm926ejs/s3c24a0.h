/*
 * linux/include/arch-s3c24a0/S3C24A0.h
 *
 *  Author:     Janghoon Lyu
 *  Created:    July 02, 2003
 *  Copyright:  MIZI Research, Inc.
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * $Date: 2004/06/23 11:01:20 $
 * $Revision: 1.5 $
 *
 */

#ifndef _S3C24A0_H_
#define _S3C24A0_H_

#include <asm/arch/hardware.h>
#include <asm/arch/bitfield.h>


/* handy sizes */
#define SZ_1K                           0x00000400
#define SZ_4K                           0x00001000
#define SZ_8K                           0x00002000
#define SZ_16K                          0x00004000
#define SZ_32K                          0x00008000
#define SZ_64K                          0x00010000
#define SZ_128K                         0x00020000
#define SZ_256K                         0x00040000
#define SZ_512K                         0x00080000

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000

#define SZ_1G                           0x40000000
#define SZ_2G                           0x80000000




#define ROM_BASE0               0x00000000      /* base address of rom bank 0 */
#define ROM_BASE1               0x04000000      /* base address of rom bank 1 */
#define DRAM_BASE0              0x10000000      /* base address of dram bank 0 */
#define DRAM_BASE1              0x20000000      /* base address of dram bank 1 */

#define DRAM_BASE               DRAM_BASE0
#define DRAM_SIZE               SZ_64M


/* total memory required by uboot */
#define UBOOT_SIZE               (2*1024*1024)
/* size of vivi binary */
#define MAX_UBOOT_BIN_SIZE       (256*1024)
#define MMU_TLB_SIZE            (16*1024)
#define HEAP_SIZE               (1*1024*1024)
#define STACK_SIZE              (512*1024)

/* base address for uboot */
#define UBOOT_BASE               (DRAM_BASE + DRAM_SIZE - UBOOT_SIZE)
#define MMU_TLB_BASE            (UBOOT_BASE + MAX_UBOOT_BIN_SIZE)
#define HEAP_BASE               (UBOOT_BASE + (512*1024))
#define STACK_BASE              (UBOOT_BASE + UBOOT_SIZE - STACK_SIZE)

#define RAM_SIZE                (UBOOT_BASE - DRAM_BASE)
#define RAM_BASE                DRAM_BASE


/*
 * GPIO
 */

#define GPCON_U			__REG(0x44800000)
#define GPCON_M			__REG(0x44800004)
#define GPCON_L			__REG(0x44800008)
#define GPDAT			__REG(0x4480000C)
#define GPUP			__REG(0x44800010)


/*
 * Memory controller
 */

#define SROM_BW			__REG(0x40C20000)
#define SROM_BC0		__REG(0x40C20004)
#define SROM_BC1		__REG(0x40C20008)
#define SROM_BC2		__REG(0x40C2000C)
#define rSROM_BW                        (*(volatile unsigned *)0x40C20000)
#define rSROM_BC0                       (*(volatile unsigned *)0x40C20004)
#define rSROM_BC1                       (*(volatile unsigned *)0x40C20008)
#define rSROM_BC2                       (*(volatile unsigned *)0x40C2000C)


#define SROM_TACS		Fld(2, 14)
#define SROM_Tacs(clk)		((clk) << FShft(SROM_TACS))
#define SROM_Tacs_0clks		(0x0)
#define SROM_Tacs_2clks		(0x1)
#define SROM_Tacs_4clks		(0x2)
#define SROM_Tacs_8clks		(0x3)

#define SROM_TCOS		Fld(2, 12)
#define SROM_Tcos(clk)		((clk) << FShft(SROM_TCOS))
#define SROM_Tcos_0clks		(0x0)
#define SROM_Tcos_2clks		(0x1)
#define SROM_Tcos_4clks		(0x2)
#define SROM_Tcos_8clks		(0x3)

#define SROM_TACC		Fld(3, 8)
#define SROM_Tacc(clk)		((clk) << FShft(SROM_TACC))
#define SROM_Tacc_2clks		(0x0)
#define SROM_Tacc_3clks		(0x1)
#define SROM_Tacc_4clks		(0x2)
#define SROM_Tacc_10clks	(0x3)
#define SROM_Tacc_12clks	(0x4)
#define SROM_Tacc_14clks	(0x5)
#define SROM_Tacc_16clks	(0x6)
#define SROM_Tacc_20clks	(0x7)

#define SROM_TCOH		Fld(2, 6)
#define SROM_Tcoh(clk)		((clk) << FShft(SROM_TCOH))
#define SROM_Tcoh_0clks		(0x0)
#define SROM_Tcoh_2clks		(0x1)
#define SROM_Tcoh_4clks		(0x2)
#define SROM_Tcoh_8clks		(0x3)

#define SROM_TCAH		Fld(2, 4)
#define SROM_Tcah(clk)		((clk) << FShft(SROM_TCAH))
#define SROM_Tcah_0clks		(0x0)
#define SROM_Tcah_2clks		(0x1)
#define SROM_Tcah_4clks		(0x2)
#define SROM_Tcah_8clks		(0x3)

#define SROM_TACP		Fld(2, 2)
#define SROM_Tacp(clk)		((clk) << FShft(SROM_TACP))
#define SROM_Tacp_2clks		(0x0)
#define SROM_Tacp_4clks		(0x1)
#define SROM_Tacp_8clks		(0x2)
#define SROM_Tacp_10clks	(0x3)

#define SROM_PMC		Fld(2, 0)
#define SROM_Pmc(data)		((data) << FShft(SROM_PMC))
#define SROM_Pmc_1data		(0x0)
#define SROM_Pmc_4data		(0x1)
#define SROM_Pmc_8data		(0x2)	
#define SROM_Pmc_16data		(0x3)


#define P1BANKCFG		__REG(0x40C40000)
#define P1BANKCON		__REG(0x40C40004)
#define P1REFRESH		__REG(0x40C40008)
#define P1TIMEOUT		__REG(0x40C4000C)
#define P2BANKCFG		__REG(0x40C60000)
#define P2BANKCON		__REG(0x40C60004)
#define P2REFRESH		__REG(0x40C60008)
#define P2TIMEOUT		__REG(0x40C6000C)

#define SDRAM_DS		Fld(2, 29)
#define SDRAM_DrvStrn(val)	((val) << FShft(SDRAM_DS))
#define SDRAM_DrvStrn_Full	(0x0)
#define SDRAM_DrvStrn_Half	(0x1)
#define SDRAM_DrvStrn_Weak	(0x2)
#define SDRAM_DrvStrn_RFU	(0x3)

#define SDRAM_TCSR		Fld(2, 27)
#define SDRAM_Tcsr(tp)		((tp) << FShft(SDRAM_TCSR))
#define SDRAM_Tcsr_4670		(0x0)
#define SDRAM_Tcsr_1645		(0x1)
#define SDRAM_Tcsr_2515		(0x2)
#define SDRAM_Tcsr_7185		(0x3)

#define SDRAM_PASR		Fld(3, 24)
#define SDRAM_Pasr(no)		((no) << FShft(SDRAM_PASR))
#define SDRAM_Pasr_4banks	(0x0)
#define SDRAM_Pasr_2banks	(0x1)
#define SDRAM_Pasr_1banks	(0x2)

#define SDRAM_PWRDN		(1 << 20)

#define SDRAM_TRAS		Fld(4, 16)
#define SDRAM_Tras(clk)		((clk-1) << FShft(SDRAM_TRAS))

#define SDRAM_TRC		Fld(4, 12)
#define SDRAM_Trc(clk)		((clk-1) << FShft(SDRAM_TRC))

#define SDRAM_TRCD		Fld(2, 10)
#define SDRAM_Trcd(clk)		((clk-1) << FShft(SDRAM_TRCD))

#define SDRAM_TRP		Fld(2, 8)
#define SDRAM_Trp(clk)		((clk-1) << FShft(SDRAM_TRP))

#define SDRAM_DEN1		Fld(2, 6)
#define SDRAM_Den1(bit)		((bit) << FShft(SDRAM_DEN1))
#define SDRAM_DEN0		Fld(2, 4)
#define SDRAM_Den0(bit)		((bit) << FShft(SDRAM_DEN0))
#define SDRAM_Den_16Mbit	(0x0)
#define SDRAM_Den_64Mbit	(0x1)
#define SDRAM_Den_128Mbit	(0x2)
#define SDRAM_Den_256Mbit	(0x3)

#define SDRAM_CL		Fld(2,2)
#define SDRAM_Cl(clk)		((clk) << FShft(SDRAM_CL))
#define SDRAM_AP		(1 << 1)
#define SDRAM_DW		(1 << 0)
#define SDRAM_Dw_32bit		(0 << 0)
#define SDRAM_Dw_16bit		(1 << 0)

/*
 * Nand flash controller
 */
#define NFCONF		__REG(0x40C00000)
#define NFCONT		__REG(0x40C00004)
#define NFCMD			__REGb(0x40C00008)
#define NFADDR		__REGb(0x40C0000C)
#define NFDATA8			__REGb(0x40C00010)
#define NFDATA16		__REGw(0x40C00010)
#define NFDATA32		__REG(0x40C00010)
#define NANDECCD0		__REG(0x40C00014)
#define NANDECCD1		__REG(0x40C00018)
#define NANDECCD2		__REG(0x40C0001C)
#define NANDECCD3		__REG(0x40C00020)
#define NANDOOBECC0		__REG(0x40C00024)
#define NANDOOBECC1		__REG(0x40C00028)
#define NANDSTAT		__REG(0x40C0002C)
#define NFSTAT			__REG(0x40C0002C)
#define NANDESTAT0		__REG(0x40C00030)
#define NANDESTAT1		__REG(0x40C00034)
#define NANDECC0		__REG(0x40C00038)
#define NANDECC1		__REG(0x40C0003C)
#define NANDOOBECC		__REG(0x40C00040)

#if 0
#define rNFCONF                         (*(volatile unsigned *)0x40C00000)
#define rNFCONT                         (*(volatile unsigned *)0x40C00004)
#define rNFCMMD                         (*(volatile unsigned *)0x40C00008)
#define rNFADDR                         (*(volatile unsigned *)0x40C0000C)
#define rNFDATA                         (*(volatile unsigned *)0x40C00010)
#define rNFMECCDATA0            (*(volatile unsigned *)0x40C00014)
#define rNFMECCDATA1            (*(volatile unsigned *)0x40C00018)
#define rNFMECCDATA2            (*(volatile unsigned *)0x40C0001C)
#define rNFMECCDATA3            (*(volatile unsigned *)0x40C00020)
#define rNFSECCDATA0            (*(volatile unsigned *)0x40C00024)
#define rNFSECCDATA1            (*(volatile unsigned *)0x40C00028)
#define rNFSTAT                         (*(volatile unsigned *)0x40C0002C)
#define rNFESTAT0                       (*(volatile unsigned *)0x40C00030)
#define rNFESTAT1                       (*(volatile unsigned *)0x40C00034)
#define rNFMECC0                        (*(volatile unsigned *)0x40C00038)
#define rNFMECC1                        (*(volatile unsigned *)0x40C0003C)
#define rNFSECC                         (*(volatile unsigned *)0x40C00040)
#define rNFSBLK                         (*(volatile unsigned *)0x40C00044)
#define rNFEBLK                         (*(volatile unsigned *)0x40C00048)
#endif

#define NANDCONF_TACLS		Fld(3, 12)
#define NANDCONF_Tacls(dur)	((dur) << FShft(NANDCONF_TACLS))
#define NANDCONF_TWRPH0		Fld(3, 8)
#define NANDCONF_Twrph0(dur)	((dur) << FShft(NANDCONF_TWRPH0))
#define NANDCONF_TWRPH1		Fld(3, 4)
#define NANDCONF_Twrph1(dur)	((dur) << FShft(NANDCONF_TWRPH1))
#define NANDCONF_nCE		(1 << 3)

#define NANDCONT_nCE		(1 << 7)	/* chip enable/disable */

#define NFSTAT_RnB		(1 << 10)

/*
 * Interrupt
 */

#define SRCPND			__REG(0x40200000)
#define INTMOD			__REG(0x40200004)
#define INTMSK			__REG(0x40200008)
#define INTPRIORITY		__REG(0x4020000C)
#define INTPND			__REG(0x40200010)
#define INTOFFSET		__REG(0x40200014)
#define SUBSRCPND		__REG(0x40200018)
#define INTSUBMSK		__REG(0x4020001C)
#define INTVECTMODE		__REG(0x40200020)
#define INTVAR			__REG(0x4020002C)

#define EXTINTCR0		__REG(0x44800018)
#define EXTINTCR1		__REG(0x4480001c)
#define EXTINTCR2		__REG(0x44800020)
#define EINTMASK		__REG(0x44800034)
#define EINTPEND		__REG(0x44800038)

/*
 * Watchdog timer
 */

#define WTCON			__REG(0x44100000)
#define WTDAT			__REG(0x44100004)
#define WTCNT			__REG(0x44100008)

/*
 * Clock and power management
 */

#define CLKCON_USBD		(1<< 15)

#define fPLL_MDIV               Fld(8,12)
#define fPLL_PDIV               Fld(6,4)
#define fPLL_SDIV               Fld(2,0)

#define LOCKTIME		__REG(0x40000000)
#define XTALWSET		__REG(0x40000004)
#define MPLLCON			__REG(0x40000010)
#define UPLLCON			__REG(0x40000014)
#define CLKCON			__REG(0x40000020)
#define CLKSRC			__REG(0x40000024)
#define CLKDIV			__REG(0x40000028)
#define PWRMAN			__REG(0x40000030)
#define SOFTRESET		__REG(0x40000038)

#define PLL_MDIV		Fld(8, 12)
#define PLL_MainDiv(val)	((val) << FShft(PLL_MDIV))
#define PLL_PDIV		Fld(6, 4)
#define PLL_PreDiv(val)		((val) << FShft(PLL_PDIV))
#define PLL_SDIV		Fld(2, 0)
#define PLL_PostDiv(val)	((val) << FShft(PLL_SDIV))
#define GET_MDIV(x)		FExtr(x, PLL_MDIV)
#define GET_PDIV(x)		FExtr(x, PLL_PDIV)
#define GET_SDIV(x)		FExtr(x, PLL_SDIV)

#define CLKDIV_HCLKDIV		Fld(1, 1)
#define CLKDIV_HclkDiv(val)	((val) << FShft(CLKDIV_HCLKDIV))
#define CLKDIV_PCLKDIV		Fld(1, 0)
#define CLKDIV_PclkDiv(val)	((val) << FShft(CLKDIV_PCLKDIV))

/*
 * UART
 */
#define ULCON0			__REG(0x44400000)
#define UCON0			__REG(0x44400004)
#define UFCON0			__REG(0x44400008)
#define UMCON0			__REG(0x4440000C)
#define UTRSTAT0		__REG(0x44400010)
#define UERSTAT0		__REG(0x44400014)
#define UFSTAT0			__REG(0x44400018)
#define UMSTAT0			__REG(0x4440001C)
#define UTXH0			__REG(0x44400020)
#define URXH0			__REG(0x44400024)
#define UBRDIV0			__REG(0x44400028)

#define ULCON1			__REG(0x44404000)
#define UCON1			__REG(0x44404004)
#define UFCON1			__REG(0x44404008)
#define UMCON1			__REG(0x4440400C)
#define UTRSTAT1		__REG(0x44404010)
#define UERSTAT1		__REG(0x44404014)
#define UFSTAT1			__REG(0x44404018)
#define UMSTAT1			__REG(0x4440401C)
#define UTXH1			__REG(0x44404020)
#define URXH1			__REG(0x44404024)
#define UBRDIV1			__REG(0x44404028)

#define UTRSTAT_TX_EMPTY	(1 << 2)
#define UTRSTAT_RX_READY	(1 << 0)
#define UART_ERR_MASK		0xF 

/*
 * PWM timer
 */
#define TCFG0			__REG(0x44000000)
#define TCFG1			__REG(0x44000004)
#define TCON			__REG(0x44000008)
#define TCNTB0			__REG(0x4400000C)
#define TCMPB0			__REG(0x44000010)
#define TCNTO0			__REG(0x44000014)
#define TCNTB1			__REG(0x44000018)
#define TCMPB1			__REG(0x4400001C)
#define TCNTO1			__REG(0x44000020)
#define TCNTB2			__REG(0x44000024)
#define TCMPB2			__REG(0x44000028)
#define TCNTO2			__REG(0x4400002C)
#define TCNTB3			__REG(0x44000030)
#define TCMPB3			__REG(0x44000034)
#define TCNTO3			__REG(0x44000038)
#define TCNTB4			__REG(0x4400003C)
#define TCNTO4			__REG(0x44000040)

/* Fields */
#define fTCFG0_DZONE		Fld(8,16)       /* the dead zone length (= timer 0) */
#define fTCFG0_PRE1		Fld(8,8)        /* prescaler value for time 2,3,4 */
#define fTCFG0_PRE0		Fld(8,0)        /* prescaler value for time 0,1 */
#define fTCFG1_MUX4		Fld(4,16)
/* bits */
#define TCFG0_DZONE(x)		FInsrt((x), fTCFG0_DZONE)
#define TCFG0_PRE1(x)		FInsrt((x), fTCFG0_PRE1)
#define TCFG0_PRE0(x)		FInsrt((x), fTCFG0_PRE0)
#define TCON_4_AUTO		(1 << 22)       /* auto reload on/off for Timer 4 */
#define TCON_4_UPDATE		(1 << 21)       /* manual Update TCNTB4 */
#define TCON_4_ONOFF		(1 << 20)       /* 0: Stop, 1: start Timer 4 */
#define COUNT_4_ON		(TCON_4_ONOFF*1)
#define COUNT_4_OFF		(TCON_4_ONOFF*0)
#define TCON_3_AUTO		(1 << 19)       /* auto reload on/off for Timer 3 */
#define TIMER3_ATLOAD_ON	(TCON_3_AUTO*1)
#define TIMER3_ATLAOD_OFF	FClrBit(TCON, TCON_3_AUTO)
#define TCON_3_INVERT		(1 << 18)       /* 1: Inverter on for TOUT3 */
#define TIMER3_IVT_ON		(TCON_3_INVERT*1)
#define TIMER3_IVT_OFF		(FClrBit(TCON, TCON_3_INVERT))
#define TCON_3_MAN		(1 << 17)       /* manual Update TCNTB3,TCMPB3 */
#define TIMER3_MANUP		(TCON_3_MAN*1)
#define TIMER3_NOP		(FClrBit(TCON, TCON_3_MAN))
#define TCON_3_ONOFF		(1 << 16)       /* 0: Stop, 1: start Timer 3 */
#define TIMER3_ON		(TCON_3_ONOFF*1)
#define TIMER3_OFF		(FClrBit(TCON, TCON_3_ONOFF))
/* macros */
#define GET_PRESCALE_TIMER4(x)	FExtr((x), fTCFG0_PRE1)
#define GET_DIVIDER_TIMER4(x)	FExtr((x), fTCFG1_MUX4)

/* USB Device */
#define USB_DEVICE_PHYS_ADR	0x44a00000


#endif /* _S3C24A0_H_ */
