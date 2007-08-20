/*
 * (C) Copyright 2004 
 *  Samsung Electronics  : SW.LEE <hitchcar@samsung.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 * Gary Jennejohn <gj@denx.de>
 * David Mueller <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Gianluca Costa <gcosta@libero.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/************************************************************
 * Select your PKG version and Memory type in the board.
 * Note : If you select one of board type, others should be commented.
 ************************************************************/
#define CONFIG_SMDK2460_496_DDR		/* 496 PKG and DDR (default) */
//#define CONFIG_SMDK2460_496_SDRAM		/* 496 PKG and SDRAM */
//#define CONFIG_SMDK2460_416_DDR 		/* 416 PKG and DDR */
//#define CONFIG_SMDK2460_416_SDRAM		/* 416 PKG and SDRAM */

/************************************************************
 * Does the board have DISCONTIGUOUS MEMORY layout?
 ************************************************************/
#if defined(CONFIG_SMDK2460_496_DDR) || defined(CONFIG_SMDK2460_496_SDRAM)
	#define CONFIG_DISCONTIGMEM		1	/*Memory is not contiguous*/
	#define adjust_bank1(x) {\
		if((x>=0x11e00000) && (x<0x20000000))\
			x=x+0xe200000;\
	}
#elif defined(CONFIG_SMDK2460_416_DDR)
	#define CONFIG_DISCONTIGMEM		1	/*Memory is not contiguous*/
	#define adjust_bank1(x) {\
		if((x>=0x21e00000) && (x<0x30000000)) \
			x=x+0xe200000;\
	}
#endif 

#if defined(CONFIG_SMDK2460_496_DDR) || defined(CONFIG_SMDK2460_496_SDRAM)
#define CFG_KERNEL_BOOT       0x10008000
#else
#define CFG_KERNEL_BOOT       0x20008000
#endif

#define CFG_MACH_ID           560

/*
 * If we are developing, we might want to start armboot from ram
 * so we MUST NOT initialize critical regs like mem-timing ...
 */
#define CONFIG_INIT_CRITICAL		/* undef for developing */

/*
 * High Level Configuration Options
 * (easy to change)
 */

#define CONFIG_S3C2460x_SMDK	1	/* on a SAMSUNG SMDK2460x Board  */
/* #define CONFIG_S3C24A0A_JTAG_BOOT 1 */
#define CONFIG_S3C2460x
#define CONFIG_S3C2440A_USE_NAND  1

#define CONFIG_S3C2460x_NAND_BOOT 1      /* boot from nandflash          */
#define CONFIG_PM 			/* Power Management */

/* input clock of PLL */
#define CONFIG_SYS_CLK_FREQ	12000000 /* the SMDK2410 has 12MHz input clock */

#define USE_926EJ_MMU		1
#define CONFIG_USE_IRQ          1
/*
 * Size of malloc() pool
 */
#define CFG_MALLOC_LEN		(CFG_ENV_SIZE + 128*1024)

/*
 * Hardware drivers
 */
#define CONFIG_DRIVER_CS8900	1	/* we have a CS8900 on-board */
//#define CS8900_BASE		(0x04000300 | (1<<25))	/* SMDK2460x */
#define CS8900_BASE		(0x07000300 | (1<<22))		/* SMDK2460A */
#define CS8900_BUS16		1 /* the Linux driver does accesses as shorts */

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_UART0          1	/* we use SERIAL 0 on SMDK2460 */
//#undef CONFIG_SERIAL0

//#define CONFIG_SERIAL_UART1          1	/* we use SERIAL 1 on SMDK2460 */
#undef CONFIG_SERIAL1

//#define CONFIG_SERIAL_UART2          1	/* we use SERIAL 2 on SMDK2460 */
#undef CONFIG_SERIAL2

/************************************************************
 * RTC
 ************************************************************/
#define	CONFIG_RTC_S3C24X0	1

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_BAUDRATE		115200

/***********************************************************
 * Command definition
 ***********************************************************/
#define CONFIG_COMMANDS \
		       (CONFIG_CMD_DFL	 | \
			CFG_CMD_CACHE	 | \
			CFG_CMD_NAND	 |  \
                /*        CFG_CMD_JFFS2	 |*/ \
			/*CFG_CMD_EEPROM |*/ \
			/*CFG_CMD_I2C	 |*/ \
			/*CFG_CMD_USB	 |*/ \
			CFG_CMD_REGINFO  | \
			CFG_CMD_DATE	 | \
			CFG_CMD_ELF      | \
			CFG_CMD_USBD     | \
                        CFG_CMD_ENV ) 

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define CONFIG_BOOTDELAY 1
/*#define CONFIG_BOOTARGS    	"root=ramfs devfs=mount console=ttySA0,9600" */

#define CONFIG_ETHADDR		00:00:F0:7F:3F:AF
#define CONFIG_NETMASK          255.255.255.000
#define CONFIG_IPADDR		192.168.0.20
#define CONFIG_SERVERIP		192.168.0.10
#define CONFIG_GATEWAYIP 	192.168.0.1


/*#define CONFIG_BOOTFILE	"elinos-lart" */
#define CONFIG_CMDLINE_TAG        1      /* enable passing of ATAGs      */
#if defined(CONFIG_SMDK2460_496_DDR) || defined(CONFIG_SMDK2460_496_SDRAM)
#define CONFIG_BOOTCOMMAND      "nandr c 1c0000 10008000; bootm"		       
#elif defined(CONFIG_SMDK2460_416_DDR) || defined(CONFIG_SMDK2460_416_SDRAM)
#define CONFIG_BOOTCOMMAND      "nandr c 1c0000 20008000; bootm"		       
#endif

#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	115200		/* speed to run kgdb serial port */
/* what's this ? it's not used anywhere */
#define CONFIG_KGDB_SER_INDEX	1		/* which serial port to use */
#endif

/*
 * Miscellaneous configurable options
 */
#define	CFG_LONGHELP				/* undef to save memory		*/
#define	CFG_PROMPT		"SMDK2460x # "	/* Monitor Command Prompt	*/
#define	CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define	CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x10000000	/* memtest works on	*/
#define CFG_MEMTEST_END		0x13F00000	/* 63 MB in DRAM	*/

#undef  CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define	CFG_LOAD_ADDR		0x13000000	/* default load address	*/


/* the PWM TImer 4 uses a counter of 15625 for 10 ms, so we need */
/* it to wrap 100 times (total 1562500) to get 1 sec. */
#define	CFG_HZ			1562500

/* valid baudrates */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#if defined(CONFIG_SMDK2460_496_DDR) || defined(CONFIG_SMDK2460_496_SDRAM)
#define CONFIG_NR_DRAM_BANKS	2	   /* we have 1 bank of DRAM */
#define PHYS_DDRAM_1		0x10000000 /* SDRAM Bank #0 */
#define PHYS_DDRAM_1_SIZE	0x02000000 /* 32 MB */ 
#define PHYS_DDRAM_2		0x20000000 /* SDRAM Bank #1 */
#define PHYS_DDRAM_2_SIZE	0x02000000 /* 32 MB */

#elif defined(CONFIG_SMDK2460_416_DDR)
#define CONFIG_NR_DRAM_BANKS	2	   /* we have 1 bank of DRAM */
#define PHYS_DDRAM_1		0x20000000 /* SDRAM Bank #0 */
#define PHYS_DDRAM_1_SIZE	0x02000000 /* 32 MB */ 
#define PHYS_DDRAM_2		0x30000000 /* SDRAM Bank #1 */
#define PHYS_DDRAM_2_SIZE	0x02000000 /* 32 MB */

#elif defined(CONFIG_SMDK2460_416_SDRAM)
#define CONFIG_NR_DRAM_BANKS	1	   /* we have 1 bank of DRAM */
#define PHYS_DDRAM_1		0x20000000 /* SDRAM Bank #0 */
#define PHYS_DDRAM_1_SIZE	0x04000000 /* 64 MB */ 
#endif

#define PHYS_FLASH_1		0x00000000 /* Flash Bank #1 */
#define CFG_FLASH_BASE		PHYS_FLASH_1
#define USBD_DOWN_ADDR		PHYS_DDRAM_1
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

#if 0
#define CONFIG_AMD_LV400	1	/* uncomment this if you have a LV400 flash */
#else 
#define CONFIG_AMD_LV800	1	/* uncomment this if you have a LV800 flash */
#endif

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#ifdef CONFIG_AMD_LV800
#define PHYS_FLASH_SIZE		0x00100000 /* 1MB */
#define CFG_MAX_FLASH_SECT	(19)	/* max number of sectors on one chip */
#define CFG_ENV_ADDR		0x13EF0000  /* addr of environment */
#endif
#ifdef CONFIG_AMD_LV400
#define PHYS_FLASH_SIZE		0x00080000 /* 512KB */
#define CFG_MAX_FLASH_SECT	(11)	/* max number of sectors on one chip */
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + 0x070000) /* addr of environment */
#endif

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(5*CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(5*CFG_HZ) /* Timeout for Flash Write */

#define	CFG_ENV_IS_IN_FLASH	1  /* 2003.11.20 getfree */ 
#ifndef CFG_ENV_IS_IN_FLASH
# define CFG_ENV_IS_NOWHERE     1
#endif
#define CFG_ENV_SIZE		0x4000	/* Total Size of Environment Sector */

#include "smdk2460nand.h"

#endif	/* __CONFIG_H */

