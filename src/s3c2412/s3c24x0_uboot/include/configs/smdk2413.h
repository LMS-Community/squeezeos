/*
 * (C) Samsung Electronics SW.LEE <hitchcar@samsung.com>
 *    - add new definition for S3C2440A
 *  
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 * Gary Jennejohn <gj@denx.de>
 * David Mueller <d.mueller@elsoft.ch>
 *
 * Configuation settings for the SAMSUNG SMDK2440 board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * If we are developing, we might want to start armboot from ram
 * so we MUST NOT initialize critical regs like mem-timing ...
 */
#define CONFIG_INIT_CRITICAL		/* undef for developing */

#define CONFIG_PM   /* Power Management */

/*
 * High Level Configuration Options
 * (easy to change)
 */

/* CPU uses external FIN value without PLL system */

#define CONFIG_ARM926EJ		1	/* This is an ARM926EJS Core	*/
#define	CONFIG_S3C2413		1	/* in a SAMSUNG S3C2413 SoC     */
#define CONFIG_S3C2413_SMDK	1	/* on a SAMSUNG SMDK2413 Board  */

//#define	CONFIG_S3C2411		1	/* in a SAMSUNG S3C2411 SoC     */
//#define CONFIG_S3C2411_SMDK	1	/* on a SAMSUNG SMDK2411 Board  */

//#define CONFIG_DEBUG_LL		1

/* Instead of using NOR , we want to use NAND */
#define CONFIG_S3C2413_USE_NAND 1

#define CONFIG_S3C2413_NAND_BOOT
//#define CONFIG_S3C2413_JTAG_BOOT	/* for boot using JTAG debugger */

#define CONFIG_SYS_CLK_FREQ	12000000


#define USE_926EJ_MMU		1
#define CONFIG_USE_IRQ      1
/*
 * Size of malloc() pool
 */
#define CFG_MALLOC_LEN		(CFG_ENV_SIZE + 128*1024)

/*
 * Hardware drivers
 */
#define CONFIG_DRIVER_CS8900	1	/* we have a CS8900 on-board */
#define CS8900_BUS16			1 /* the Linux driver does accesses as shorts */
#if defined (CONFIG_S3C2411)
#define CS8900_BASE				(0x0a000600)
#define CONFIG_SERIAL_UART1 	1
#else
#define CS8900_BASE				(0x2a000600)
#define CONFIG_SERIAL_UART0 	1
#endif



/*
 * select serial console configuration
 */
#define UART_NR	          0

/************************************************************
 * RTC
 ************************************************************/
#define	CONFIG_RTC_S3C24X0	1

/* allow to overwrite serial and ethaddr */
/* #define CONFIG_ENV_OVERWRITE */

#define CONFIG_BAUDRATE		115200

/***********************************************************
 * Command definition
 ***********************************************************/
#define CONFIG_COMMANDS \
			(CONFIG_CMD_DFL	 | \
			CFG_CMD_CACHE	 | \
			CFG_CMD_NAND	 | \
			/*CFG_CMD_EEPROM |*/ \
			/*CFG_CMD_I2C	 |*/ \
			/*CFG_CMD_USB	 |*/ \
			CFG_CMD_REGINFO  | \
			/*CFG_CMD_DATE	 |*/ \
			CFG_CMD_USBD  |\
			CFG_CMD_ELF | \
			CFG_CMD_BOARD_TEST)

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */


#include <cmd_confdefs.h>


/*
 * !!! WARNING !!!
 *
 * If you update these defines you MUST also update the values in
 * tools/env/fw_env.h.
 */
#define CONFIG_BOOTDELAY	1
#define CONFIG_BOOTARGS    	"root=/dev/mtdblock1 console=ttySAC0,115200 mem=64M init=/linuxrc"
#define CONFIG_BOOTCOMMAND	"nandr $(kernelblock) 1d0000 30008000; setenv bootargs $(bootargs) mtdset=$(mtdset); bootm"

#define CONFIG_EXTRA_ENV_SETTINGS "sw6=echo Factory reset; blink; nande b00 1400000; blink\0" \
                                  "kernelblock=c\0" \
                                  "mtdset=0\0"

#define CONFIG_CMDLINE_TAG

#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	115200		/* speed to run kgdb serial port */
/* what's this ? it's not used anywhere */
#define CONFIG_KGDB_SER_INDEX	1		/* which serial port to use */
#endif

/******    KERNEL PARAMETER *************/
#define CFG_KERNEL_BOOT       0x30008000
#define CFG_MACH_ID           1490
/****************************************/

/*
 * Miscellaneous configurable options
 */
#define	CFG_LONGHELP				/* undef to save memory		*/
#define	CFG_PROMPT		"JIVE# "	/* Monitor Command Prompt	*/
#define	CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define	CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x30000000	/* memtest works on	*/
#define CFG_MEMTEST_END		0x33F00000	/* 63 MB in DRAM	*/

#undef  CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define	CFG_LOAD_ADDR		0x33000000	/* default load address	*/

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
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1	   /* we have 1 bank of DRAM */
#define PHYS_SDRAM_1		0x30000000 /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE	0x04000000 /* 64 MB */

#define PHYS_FLASH_1		0x00000000 /* Flash Bank #1 */

//#define USBD_DOWN_ADDR		PHYS_SDRAM_1
#define USBD_DOWN_ADDR	CFG_KERNEL_BOOT
#define CFG_FLASH_BASE		PHYS_FLASH_1

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
#if 0
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + 0x0F0000) /* addr of environment */
#endif
#define CFG_ENV_ADDR		0x33EF0000 /* addr of environment */
#endif
#ifdef CONFIG_AMD_LV400
#define PHYS_FLASH_SIZE		0x00080000 /* 512KB */
#define CFG_MAX_FLASH_SECT	(11)	/* max number of sectors on one chip */
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + 0x070000) /* addr of environment */
#endif

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(5*CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(5*CFG_HZ) /* Timeout for Flash Write */

#define	CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_SIZE		0x4000	/* Total Size of Environment Sector */

#define DETECT_KEY		/* Enables key detection at u-boot boot */

#include "smdk2413nand.h"

#endif	/* __CONFIG_H */
