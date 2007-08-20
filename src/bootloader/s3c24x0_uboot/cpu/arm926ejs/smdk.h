/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * $Date: 2005/11/17 07:05:46 $
 * $Revision: 1.13 $
 *
 */
#include <config.h>

#ifdef CONFIG_S3C2460x
#include <asm/arch/s3c2460.h>
#ifndef S3C2460_SMDK_H
#define S3C2460_SMDK_H
                                                                                        
/*
 * Architecture magic and machine type
 */
#define MACH_TYPE               193
#define UBOOT_MAGIC             (0x43090000 | MACH_TYPE)
#endif
                                                                                        
#elif defined(CONFIG_S3C24A0A)
#include <asm/arch/s3c24a0.h>
#ifndef S3C24a0_SMDK_H
#define S3C24A0_SMDK_H


/*
 * Architecture magic and machine type
 */
#define MACH_TYPE		193	
#define UBOOT_MAGIC		(0x43090000 | MACH_TYPE)
//shaju
#if defined (OREIAS)

#define         CONFIG_S3C24A0_CLK_192MHZ
#define		REFRESHRATES_VAL	0x2EC //7.8*HCL
#elif defined (FOX) || defined (FOXHOUND)
/*-------------  FOX -------------------*/
#define         CONFIG_S3C24A0_CLK_184MHZ
#define		REFRESHRATES_VAL	0x2C0
#else
#define         CONFIG_S3C24A0_CLK_204MHZ
#define		REFRESHRATES_VAL	360
#endif


#ifdef CONFIG_S3C24A0_CLK_220MHZ
#define UART_REF_CLK		(55000000)

#define SROM_BW_VAL	((0 << 9) | (1 << 6) | ( 1 << 3) | 0)
#define SROM_BANK0_VAL	(SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
			 SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
			 SROM_Pmc(0x0))
#define SROM_BANK1_VAL	(SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
			 SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
			 SROM_Pmc(0x0))
#define SROM_BANK2_VAL	(SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
			 SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
			 SROM_Pmc(0x0))

#define SDRAM_BANK1_VAL	(SDRAM_PWRDN | SDRAM_Tras(5) | SDRAM_Trc(8) | \
			 SDRAM_Trcd(3) | SDRAM_Trp(3) | SDRAM_Cl(3) | \
			 SDRAM_Den0(SDRAM_Den_256Mbit) | \
			 SDRAM_Dw_32bit)

/* 220MHz */
#define MPLL_VAL	(PLL_MainDiv(47) | PLL_PreDiv(1) | PLL_PostDiv(0))
/* 1:2:4 */
#define CLKDIV_VAL	(0x3)


#elif defined(CONFIG_S3C24A0_CLK_204MHZ)

#define SROM_BW_VAL     ((0 << 9) | (1 << 6) | ( 1 << 3) | 0)
#define SROM_BANK0_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK1_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK2_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))

#ifdef POSEIDON
#define SDRAM_BANK1_VAL (SDRAM_PWRDN | SDRAM_Tras(15) | SDRAM_Trc(15) | \
                         SDRAM_Trcd(3) | SDRAM_Trp(3) | SDRAM_Cl(3) | \
                         SDRAM_Den0(SDRAM_Den_128Mbit) | \
                         SDRAM_Dw_32bit)

/* Main Clock : 184Mhz */
#define MPLL_VAL        (PLL_MainDiv(53) | PLL_PreDiv(2) | PLL_PostDiv(0))
#define UART_REF_CLK            (45800000)
#else
#define SDRAM_BANK1_VAL (SDRAM_PWRDN | SDRAM_Tras(5) | SDRAM_Trc(8) | \
                         SDRAM_Trcd(3) | SDRAM_Trp(3) | SDRAM_Cl(3) | \
                         SDRAM_Den0(SDRAM_Den_256Mbit) | \
                         SDRAM_Dw_32bit)

#define UART_REF_CLK            (51000000)
#define MPLL_VAL        (PLL_MainDiv(60) | PLL_PreDiv(2) | PLL_PostDiv(0))
#endif
/* 204MHz */
/* 1:2:4 */
#define CLKDIV_VAL      (0x3)


/*---------------- Only FOX -------------------------------*/
#elif defined(CONFIG_S3C24A0_CLK_184MHZ)
#define SROM_BW_VAL     ((0 << 9) | (1 << 6) | ( 1 << 3) | 0)
#define SROM_BANK0_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK1_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK2_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))

#define SDRAM_BANK1_VAL (SDRAM_PWRDN | SDRAM_Tras(15) | SDRAM_Trc(15) | \
                         SDRAM_Trcd(3) | SDRAM_Trp(3) | SDRAM_Cl(3) | \
                         SDRAM_Den0(SDRAM_Den_256Mbit) | \
                         SDRAM_Dw_32bit)

/* Main Clock : 184Mhz */
#define MPLL_VAL        (PLL_MainDiv(53) | PLL_PreDiv(2) | PLL_PostDiv(0))
#define UART_REF_CLK    (45800000)
#define CLKDIV_VAL      (0x3)
/*for k2*/
#elif defined(CONFIG_S3C24A0_CLK_192MHZ)
#define SROM_BW_VAL     ((0 << 9) | (1 << 6) | ( 1 << 3) | 0)
#define SROM_BANK0_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK1_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))
#define SROM_BANK2_VAL  (SROM_Tacs(0x0) | SROM_Tcos(0x0) | SROM_Tacc(0x7) | \
                         SROM_Tcoh(0x0) | SROM_Tcah(0x0) | SROM_Tacp(0x0) | \
                         SROM_Pmc(0x0))

#define SDRAM_BANK1_VAL (SDRAM_PWRDN | SDRAM_Tras(15) | SDRAM_Trc(15) | \
                         SDRAM_Trcd(3) | SDRAM_Trp(3) | SDRAM_Cl(3) | \
                         SDRAM_Den0(SDRAM_Den_256Mbit) |\
			 SDRAM_Den1(SDRAM_Den_256Mbit) | \
                         SDRAM_Dw_32bit)

/* Main Clock : 184Mhz */
#define MPLL_VAL        (PLL_MainDiv(56) | PLL_PreDiv(2) | PLL_PostDiv(0))
#define UART_REF_CLK    (48000000)
#define CLKDIV_VAL      (0x3)


#elif defined(CONFIG_S3C24A0_CLK_84MHZ)
#define UART_REF_CLK	(42000000)
#define	SROM_BW_VAL	(0x00000048)
#define SROM_BANK0_VAL	(0x00000500)
#define SROM_BANK1_VAL	(0x00000500)
#define SROM_BANK2_VAL	(0x00000500)
#define SDRAM_BANK1_VAL	((0x1 << 20) | (0x4 << 16) | (0x6 << 12) | \
			 (0x2 << 10) | (0x2 << 8) | (0x0 << 6) | \
			 (0x3 << 4) | (0x3 << 2) | (0x0 << 1) | (0x0))
/* 84MHz */
#define MPLL_VAL	(PLL_MainDiv(76) | PLL_PreDiv(4) | PLL_PostDiv(1))
/* 1:1:2 */
#define CLKDIV_VAL	(0x1)
#else
#error Unknown clock setting
#endif


/* -------------   Multi-function GPIO Configuration ------*/
#if defined (POSEIDON)
#define GPCON_U_VAL     (0x03fbff5f)
#define GPCON_M_VAL     (0x0000d511)
#define GPCON_L_VAL     (0x001a596a)
#define GPUP_VAL        (0xffffffff)    /* disable all pull-up */
#elif defined (FOX) /* Real Board */
/* GPIO List : Thu Dec 02 */
#define GPCON_U_VAL     (0x02a9ff7f)
#define GPCON_M_VAL     (0x0000d555)
#define GPCON_L_VAL     (0x001a614a)
#define GPUP_VAL        (0xffffffff)    /* disable all pull-up */
#elif defined (FOXHOUND) /* Test Demo Board */
/* GPIO List : Thu Dec 02 */
#define GPCON_U_VAL     (0x02a9ffff)
#define GPCON_M_VAL     (0x0000d506)
#define GPCON_L_VAL     (0x001a646a)
#define GPUP_VAL        (0xffffffff)    /* disable all pull-up */
#elif defined (OREIAS) /* K2Board */
#define GPCON_U_VAL     (0x02a9ffff)
#define GPCON_M_VAL     (0x0000d566)
#define GPCON_L_VAL     (0x001915b9)
#define GPUP_VAL        (0xffffffff)    /* disable all pull-up */

#else

//#define GPCON_U_VAL     (0x02a9ffff)
//#define GPCON_M_VAL     (0x0000d566)
//#define GPCON_L_VAL     (0x001915b9)
//#define GPUP_VAL        (0xffffffff)    /* disable all pull-up */

#define GPCON_U_VAL	(0x02bfffff)
#define GPCON_M_VAL	(0x000055aa)
#define GPCON_L_VAL	(0x0029552a)
#define GPUP_VAL	(0xffffffff)	/* disable all pull-up */
#endif

#endif       	
#endif	/* THE END OF S3C24A0 */
