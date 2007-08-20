/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * $Date: 2005/12/14 09:43:26 $
 * $Revision: 1.4 $
 */

#ifndef _S3C2413_H_
#define _S3C2413_H_

#include <asm/arch/hardware.h>
#include <asm/arch/bitfield.h>

/* handy sizes */
#define SZ_1K		0x00000400
#define SZ_4K           0x00001000
#define SZ_8K           0x00002000
#define SZ_16K          0x00004000
#define SZ_32K          0x00008000
#define SZ_64K          0x00010000
#define SZ_128K         0x00020000
#define SZ_256K         0x00040000
#define SZ_512K         0x00080000

#define SZ_1M           0x00100000
#define SZ_2M           0x00200000
#define SZ_4M           0x00400000
#define SZ_8M           0x00800000
#define SZ_16M          0x01000000
#define SZ_32M          0x02000000
#define SZ_64M          0x04000000
#define SZ_128M         0x08000000
#define SZ_256M         0x10000000
#define SZ_512M         0x20000000

#define SZ_1G           0x40000000
#define SZ_2G           0x80000000

#define ROM_BASE0       0x00000000      /* base address of rom bank 0 */
#define DRAM_BASE0      0x30000000      /* base address of dram bank 0 */

#define DRAM_BASE       DRAM_BASE0
#define DRAM_SIZE       SZ_64M


/* total memory required by uboot */
#define UBOOT_SIZE      		(2*1024*1024)
#define MAX_UBOOT_BIN_SIZE      (256*1024)
#define MMU_TLB_SIZE            (16*1024)
#define HEAP_SIZE               (1*1024*1024)
#define STACK_SIZE              (512*1024)

/* base address for uboot */
#define UBOOT_BASE              (DRAM_BASE + DRAM_SIZE - UBOOT_SIZE)
//#define UBOOT_BASE            TEXT_BASE
#define MMU_TLB_BASE            (UBOOT_BASE + MAX_UBOOT_BIN_SIZE)
#define HEAP_BASE               (UBOOT_BASE + (512*1024))
#define STACK_BASE              (UBOOT_BASE + UBOOT_SIZE - STACK_SIZE)
#define RAM_SIZE                (UBOOT_BASE - DRAM_BASE)
#define RAM_BASE                DRAM_BASE


/*
 * GPIO
 */
#define GPACON          (*(volatile unsigned *)0x56000000)
#define GPADAT          (*(volatile unsigned *)0x56000004)

#define GPBCON          (*(volatile unsigned *)0x56000010)
#define GPBDAT          (*(volatile unsigned *)0x56000014)
#define GPBDN           (*(volatile unsigned *)0x56000018)
#define GPBUP           (*(volatile unsigned *)0x56000018)
#define GPBSLPCON       (*(volatile unsigned *)0x5600001c)

#define GPCCON          (*(volatile unsigned *)0x56000020)
#define GPCDAT          (*(volatile unsigned *)0x56000024)
#define GPCDN           (*(volatile unsigned *)0x56000028)
#define GPCUP           (*(volatile unsigned *)0x56000028)
#define GPCSLPCON       (*(volatile unsigned *)0x5600002c)

#define GPDCON          (*(volatile unsigned *)0x56000030)
#define GPDDAT          (*(volatile unsigned *)0x56000034)
#define GPDDN           (*(volatile unsigned *)0x56000038)
#define GPDUP           (*(volatile unsigned *)0x56000038)
#define GPDSLPCON       (*(volatile unsigned *)0x5600003c)

#define GPECON          (*(volatile unsigned *)0x56000040)
#define GPEDAT          (*(volatile unsigned *)0x56000044)
#define GPEDN           (*(volatile unsigned *)0x56000048)
#define GPEUP           (*(volatile unsigned *)0x56000048)
#define GPESLPCON       (*(volatile unsigned *)0x5600004c)

#define GPFCON          (*(volatile unsigned *)0x56000050)
#define GPFDAT          (*(volatile unsigned *)0x56000054)
#define GPFDN           (*(volatile unsigned *)0x56000058)

#define GPGCON          (*(volatile unsigned *)0x56000060)
#define GPGDAT          (*(volatile unsigned *)0x56000064)
#define GPGDN           (*(volatile unsigned *)0x56000068)
#define GPGUP           (*(volatile unsigned *)0x56000068)
#define GPGSLPCON       (*(volatile unsigned *)0x5600006c)

#define GPHCON          (*(volatile unsigned *)0x56000070)
#define GPHDAT          (*(volatile unsigned *)0x56000074)
#define GPHDN           (*(volatile unsigned *)0x56000078)
#define GPHUP           (*(volatile unsigned *)0x56000078)
#define GPHSLPCON       (*(volatile unsigned *)0x5600007c)

#define GSTATUS0        (*(volatile unsigned *)0x560000BC)
#define GSTATUS1        (*(volatile unsigned *)0x560000C0)
#define GSTATUS2        (*(volatile unsigned *)0x560000C4)
#define GSTATUS3        (*(volatile unsigned *)0x560000C8)
#define GSTATUS4        (*(volatile unsigned *)0x560000CC)
#define GSTATUS5        (*(volatile unsigned *)0x560000D0)




#define GPJCON          (*(volatile unsigned *)0x560000d0)
#define GPJDAT          (*(volatile unsigned *)0x560000d4)
#define GPJDN           (*(volatile unsigned *)0x560000d8)
#define GPJUP           (*(volatile unsigned *)0x560000d8)
#define GPJSLPCON       (*(volatile unsigned *)0x560000dc)

/*
 * EBI(External Bus Interface)
 */
#define EBIPR           __REG(0x48800000)
#define BANK_CFG        __REG(0x48800004)

/*
 * SSMC 
 */
#define SMBIDCYR0       __REG(0x4F000000)
#define SMBIDCYR1       __REG(0x4F000020)
#define SMBIDCYR2       __REG(0x4F000040)
#define SMBIDCYR3       __REG(0x4F000060)
#define SMBIDCYR4       __REG(0x4F000080)
#define SMBIDCYR5       __REG(0x4F0000A0)

#define SMBWSTRDR0      __REG(0x4F000004)
#define SMBWSTRDR1      __REG(0x4F000024)
#define SMBWSTRDR2      __REG(0x4F000044)
#define SMBWSTRDR3      __REG(0x4F000064)
#define SMBWSTRDR4      __REG(0x4F000084)
#define SMBWSTRDR5      __REG(0x4F0000A4)

#define SMBWSTWRR0      __REG(0x4F000008)
#define SMBWSTWRR1      __REG(0x4F000028)
#define SMBWSTWRR2      __REG(0x4F000048)
#define SMBWSTWRR3      __REG(0x4F000068)
#define SMBWSTWRR4      __REG(0x4F000088)
#define SMBWSTWRR5      __REG(0x4F0000A8)

#define SMBWSTOENR0     __REG(0x4F00000C)
#define SMBWSTOENR1     __REG(0x4F00002C)
#define SMBWSTOENR2     __REG(0x4F00004C)
#define SMBWSTOENR3     __REG(0x4F00006C)
#define SMBWSTOENR4     __REG(0x4F00008C)
#define SMBWSTOENR5     __REG(0x4F0000AC)

#define SMBWSTWENR0     __REG(0x4F000010)
#define SMBWSTWENR1     __REG(0x4F000030)
#define SMBWSTWENR2     __REG(0x4F000050)
#define SMBWSTWENR3     __REG(0x4F000070)
#define SMBWSTWENR4     __REG(0x4F000090)
#define SMBWSTWENR5     __REG(0x4F0000B0)

#define SMBCR0          __REG(0x4F000014)
#define SMBCR1          __REG(0x4F000034)
#define SMBCR2          __REG(0x4F000054)
#define SMBCR3          __REG(0x4F000074)
#define SMBCR4          __REG(0x4F000094)
#define SMBCR5          __REG(0x4F0000B4)

#define SMBSR0          __REG(0x4F000018)
#define SMBSR1          __REG(0x4F000038)
#define SMBSR2          __REG(0x4F000058)
#define SMBSR3          __REG(0x4F000078)
#define SMBSR4          __REG(0x4F000098)
#define SMBSR5          __REG(0x4F0000B8)

#define SMBWSTBDR0      __REG(0x4F00001C)
#define SMBWSTBDR1      __REG(0x4F00003C)
#define SMBWSTBDR2      __REG(0x4F00005C)
#define SMBWSTBDR3      __REG(0x4F00007C)
#define SMBWSTBDR4      __REG(0x4F00009C)
#define SMBWSTBDR5      __REG(0x4F0000BC)

#define SSMCSR          __REG(0x4F000200)
#define SSMCCR          __REG(0x4F000204)

/*
 * Memory controller
 */
#define MDROM_BW		__REG(0x48000000)
#define MDROM_BC0		__REG(0x48000004)
#define MDROM_BC1		__REG(0x48000008)
#define MDROM_BC2		__REG(0x4800000C)
#define rSROM_BW         (*(volatile unsigned *)0x48000000)
#define rSROM_BC0        (*(volatile unsigned *)0x48000004)
#define rSROM_BC1        (*(volatile unsigned *)0x48000008)
#define rSROM_BC2        (*(volatile unsigned *)0x4800000C)

#define MDRAM_RASBW0			Fld(2, 17)
#define MDRAM_RasBW0(clk)		((clk) << FShft(MDRAM_RASBW0))
#define MDRAM_RASBW1			Fld(2, 14)
#define MDRAM_RasBW1(clk)		((clk) << FShft(MDRAM_RASBW1))
#define MDRAM_RASBW_11bit		0x0
#define MDRAM_RASBW_12bit		0x1
#define MDRAM_RASBW_13bit		0x2
#define MDRAM_RASBW_14bit		0x3

#define MDRAM_CASBW0			Fld(2, 11)
#define MDRAM_CasBW0(clk)		((clk) << FShft(MDRAM_CASBW0))
#define MDRAM_CASBW1			Fld(2, 8)
#define MDRAM_CasBW1(clk)		((clk) << FShft(MDRAM_CASBW1))
#define MDRAM_CASBW_8bit		0x0
#define MDRAM_CASBW_9bit		0x1
#define MDRAM_CASBW_10bit		0x2
#define MDRAM_CASBW_11bit		0x3

#define MDRAM_ADDRCFG0			Fld(2, 6)
#define MDRAM_AddrCFG0(x)		((x) << FShft(MDRAM_ADDRCFG0))
#define MDRAM_ADDRCFG1			Fld(2, 4)
#define MDRAM_AddrCFG1(x)		((x) << FShft(MDRAM_ADDRCFG1))
#define BA_RAS_CAS				0x0
#define RAS_BA_CAS				0x1

#define mSDRAM					(1 << 2)
#define mDDRAM					(3 << 2)
#define MDRAM_BW_32bit			(0 << 0)
#define MDRAM_BW_16bit			(1 << 0)

#define MDRAM_BStop				Fld(1, 7)
#define MDRAM_BStop_CON(x)		((x) << FShft(MDRAM_BStop))
#define BStop_OFF				0x0
#define BStop_ON				0x1

#define MDRAM_WBUF				Fld(1, 6)
#define MDRAM_WBUF_CON(x)		((x) << FShft(MDRAM_WBUF))
#define WBUF_DISABLE			0x0
#define WBUF_ENABLE				0x1

#define MDRAM_AP				Fld(1, 5)
#define MDRAM_AP_CON(x)			((x) << FShft(MDRAM_ADDRCFG0))
#define AP_DISABLE				0x0
#define AP_ENABLE				0x1

#define MDRAM_PWRDN				(1 << 4)

#define MDRAM_TRAS				Fld(4, 20)
#define MDRAM_Tras(clk)			((clk-1) << FShft(MDRAM_TRAS))

#define MDRAM_TRC				Fld(4, 16)
#define MDRAM_Trc(clk)			((clk-1) << FShft(MDRAM_TRC))

#define MDRAM_CL				Fld(2, 4)
#define MDRAM_Cl(clk)			((clk) << FShft(MDRAM_CL))

#define MDRAM_TRCD				Fld(2, 2)
#define MDRAM_Trcd(clk)			((clk-1) << FShft(MDRAM_TRCD))

#define MDRAM_TRP				Fld(2, 0)
#define MDRAM_Trp(clk)			((clk-1) << FShft(MDRAM_TRP))

#define MDRAM_BANKADDR_EMRS		Fld(2, 30)
#define MDRAM_BA_EMRS(x)		((x) << FShft(MDRAM_BANKADDR_EMRS))

#define MDRAM_BANKADDR_MRS		Fld(2, 14)
#define MDRAM_BA_MRS(x)			((x) << FShft(MDRAM_BANKADDR_MRS))

#define MDRAM_CL_MRS			Fld(3, 4)
#define MDRAM_Cl_MRS(clk)		((clk) << FShft(MDRAM_CL_MRS))

/*
 * Nand flash controller
 */
#define NFCONF          (*(volatile unsigned *)0x4E000000)
#define NFCONT          (*(volatile unsigned *)0x4E000004)
#define NFCMD           (*(volatile unsigned *)0x4E000008)
#define NFADDR          (*(volatile unsigned *)0x4E00000C)
#define NFDATA          (*(volatile unsigned *)0x4E000010)
#define NFDATA8         (*(volatile unsigned char *)0x4E000010)
#define NFDATA16        (*(volatile unsigned short *)0x4E000010)
#define NFDATA32        (*(volatile unsigned long *)0x4E000010)
#define NFMECCD0        (*(volatile unsigned *)0x4E000014)
#define NFMECCD1        (*(volatile unsigned *)0x4E000018)
#define NFSECCD         (*(volatile unsigned *)0x4E00001C)
#define NFSBLK          (*(volatile unsigned *)0x4E000020)
#define NFEBLK          (*(volatile unsigned *)0x4E000024)
#define NFSTAT          (*(volatile unsigned *)0x4E000028)
#define NFECCERR0       (*(volatile unsigned *)0x4E00002C)
#define NFECCERR1       (*(volatile unsigned *)0x4E000030)
#define NFMECC0         (*(volatile unsigned *)0x4E000034)
#define NFMECC1         (*(volatile unsigned *)0x4E000038)
#define NFSECC          (*(volatile unsigned *)0x4E00003C)
#define NFMLCBITPT      (*(volatile unsigned *)0x4E000040)

#define NANDCONF_TACLS		Fld(3, 12)
#define NANDCONF_Tacls(dur)	((dur) << FShft(NANDCONF_TACLS))
#define NANDCONF_TWRPH0		Fld(3, 8)
#define NANDCONF_Twrph0(dur)	((dur) << FShft(NANDCONF_TWRPH0))
#define NANDCONF_TWRPH1		Fld(3, 4)
#define NANDCONF_Twrph1(dur)	((dur) << FShft(NANDCONF_TWRPH1))
#define NANDCONF_nCE		(1 << 3)
#define NANDCONT_nCE		(1 << 7)	/* chip enable/disable */
#define NFSTAT_RnB			(1 << 10)

/*
 * Interrupt
 */
#define SRCPND			__REG(0x4A000000)
#define INTMOD			__REG(0x4A000004)
#define INTMSK			__REG(0x4A000008)
#define INTPRIORITY		__REG(0x4A00000C)
#define INTPND			__REG(0x4A000010)
#define INTOFFSET		__REG(0x4A000014)
#define SUBSRCPND		__REG(0x4A000018)
#define INTSUBMSK		__REG(0x4A00001C)
#define INTVECTMODE		__REG(0x4A000020)
#define INTVAR			__REG(0x4A00002C)

#define EXTINTCR0		__REG(0x56000098)
#define EXTINTCR1		__REG(0x5600009C)
#define EXTINTCR2		__REG(0x560000A0)
#define EINTMASK		__REG(0x560000B4)
#define EINTPEND		__REG(0x560000B8)

/*
 * Watchdog timer
 */
#define WTCON			__REG(0x53000000)
#define WTDAT			__REG(0x53000004)
#define WTCNT			__REG(0x53000008)


#define CLKCON_USBD		(1<< 16)


/*
 * Clock and power management
 */
#define LOCKTIME		__REG(0x4C000000)
#define MPLLCON			__REG(0x4C000004)
#define UPLLCON			__REG(0x4C000008)
#define CLKCON			__REG(0x4C00000C)
#define CLKSRC			__REG(0x4C00001C)
#define CLKDIV			__REG(0x4C000014)
#define PWRMAN			__REG(0x4C000020)
#define SOFTRESET		__REG(0x4C000030)

#define PLL_MDIV			Fld(8, 12)
#define PLL_MainDiv(val)	((val) << FShft(PLL_MDIV))
#define PLL_PDIV			Fld(6, 4)
#define PLL_PreDiv(val)		((val) << FShft(PLL_PDIV))
#define PLL_SDIV			Fld(2, 0)
#define PLL_PostDiv(val)	((val) << FShft(PLL_SDIV))
#define GET_MDIV(x)			FExtr(x, PLL_MDIV)
#define GET_PDIV(x)			FExtr(x, PLL_PDIV)
#define GET_SDIV(x)			FExtr(x, PLL_SDIV)

#define fPLL_MDIV               Fld(8,12)
#define fPLL_PDIV               Fld(6,4)
#define fPLL_SDIV               Fld(2,0)




#define CLKDIV_USB48DIV		(1 << 6)
#define CLKDIV_HALFCLK		(1 << 5)
#define CLKDIV_ARMDIV		Fld(1, 3)
#define CLKDIV_ArmDiv(val)	((val) << FShft(CLKDIV_ARMDIV))
#define CLKDIV_HCLKDIV		Fld(2, 0)
#define CLKDIV_HclkDiv(val)	((val) << FShft(CLKDIV_HCLKDIV))
#define CLKDIV_PCLKDIV		Fld(1, 2)
#define CLKDIV_PclkDiv(val)	((val) << FShft(CLKDIV_PCLKDIV))

/*
 * UART
 */
#define ULCON0			__REG(0x50000000)
#define UCON0			__REG(0x50000004)
#define UFCON0			__REG(0x50000008)
#define UMCON0			__REG(0x5000000C)
#define UTRSTAT0		__REG(0x50000010)
#define UERSTAT0		__REG(0x50000014)
#define UFSTAT0			__REG(0x50000018)
#define UMSTAT0			__REG(0x5000001C)
#define UTXH0			__REG(0x50000020)
#define URXH0			__REG(0x50000024)
#define UBRDIV0			__REG(0x50000028)

#define ULCON1			__REG(0x50004000)
#define UCON1			__REG(0x50004004)
#define UFCON1			__REG(0x50004008)
#define UMCON1			__REG(0x5000400C)
#define UTRSTAT1		__REG(0x50004010)
#define UERSTAT1		__REG(0x50004014)
#define UFSTAT1			__REG(0x50004018)
#define UMSTAT1			__REG(0x5000401C)
#define UTXH1			__REG(0x50004020)
#define URXH1			__REG(0x50004024)
#define UBRDIV1			__REG(0x50004028)

#define UTRSTAT_TX_EMPTY	(1 << 2)
#define UTRSTAT_RX_READY	(1 << 0)
#define UART_ERR_MASK		0xF 

/*
 * PWM timer
 */
#define TCFG0			__REG(0x51000000)
#define TCFG1			__REG(0x51000004)
#define TCON			__REG(0x51000008)
#define TCNTB0			__REG(0x5100000C)
#define TCMPB0			__REG(0x51000010)
#define TCNTO0			__REG(0x51000014)
#define TCNTB1			__REG(0x51000018)
#define TCMPB1			__REG(0x5100001C)
#define TCNTO1			__REG(0x51000020)
#define TCNTB2			__REG(0x51000024)
#define TCMPB2			__REG(0x51000028)
#define TCNTO2			__REG(0x5100002C)
#define TCNTB3			__REG(0x51000030)
#define TCMPB3			__REG(0x51000034)
#define TCNTO3			__REG(0x51000038)
#define TCNTB4			__REG(0x5100003C)
#define TCNTO4			__REG(0x51000040)

/* Fields */
#define fTCFG0_DZONE	Fld(8,16)       /* the dead zone length (= timer 0) */
#define fTCFG0_PRE1		Fld(8,8)        /* prescaler value for time 2,3,4 */
#define fTCFG0_PRE0		Fld(8,0)        /* prescaler value for time 0,1 */
#define fTCFG1_MUX4		Fld(4,16)

/* bits */
#define TCFG0_DZONE(x)		FInsrt((x), fTCFG0_DZONE)
#define TCFG0_PRE1(x)		FInsrt((x), fTCFG0_PRE1)
#define TCFG0_PRE0(x)		FInsrt((x), fTCFG0_PRE0)
#define TCON_4_AUTO			(1 << 22)       /* auto reload on/off for Timer 4 */
#define TCON_4_UPDATE		(1 << 21)       /* manual Update TCNTB4 */
#define TCON_4_ONOFF		(1 << 20)       /* 0: Stop, 1: start Timer 4 */
#define COUNT_4_ON			(TCON_4_ONOFF*1)
#define COUNT_4_OFF			(TCON_4_ONOFF*0)
#define TCON_3_AUTO			(1 << 19)       /* auto reload on/off for Timer 3 */
#define TIMER3_ATLOAD_ON	(TCON_3_AUTO*1)
#define TIMER3_ATLAOD_OFF	FClrBit(TCON, TCON_3_AUTO)
#define TCON_3_INVERT		(1 << 18)       /* 1: Inverter on for TOUT3 */
#define TIMER3_IVT_ON		(TCON_3_INVERT*1)
#define TIMER3_IVT_OFF		(FClrBit(TCON, TCON_3_INVERT))
#define TCON_3_MAN			(1 << 17)       /* manual Update TCNTB3,TCMPB3 */
#define TIMER3_MANUP		(TCON_3_MAN*1)
#define TIMER3_NOP			(FClrBit(TCON, TCON_3_MAN))
#define TCON_3_ONOFF		(1 << 16)       /* 0: Stop, 1: start Timer 3 */
#define TIMER3_ON			(TCON_3_ONOFF*1)
#define TIMER3_OFF			(FClrBit(TCON, TCON_3_ONOFF))

/* macros */
#define GET_PRESCALE_TIMER4(x)	FExtr((x), fTCFG0_PRE1)
#define GET_DIVIDER_TIMER4(x)	FExtr((x), fTCFG1_MUX4)

#define USB_DEVICE_PHYS_ADR	0x52000000

#endif /* _S3C2413_H_ */
