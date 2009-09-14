/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_rtic.h
 *
 * @brief The header file for Run-Time Integrity Checker (RTIC) module.
 * This file contains all register defines and bit definition of RTIC module.
 *
 * @ingroup MXC_Security
 */

#ifndef __MXC_RTIC_H__
#define __MXC_RTIC_H__

#include <asm/arch/mxc_security_api.h>
#include <asm/arch/hardware.h>
#include <asm-generic/errno-base.h>

/*
 * RTIC status register
 */
#define  RTIC_STATUS           IO_ADDRESS(RTIC_BASE_ADDR + 0x00)

/*
 * RTIC Command Register
 */
#define  RTIC_COMMAND          IO_ADDRESS(RTIC_BASE_ADDR + 0x04)

/*
 * RTIC Control Register
 */
#define  RTIC_CONTROL          IO_ADDRESS(RTIC_BASE_ADDR + 0x08)

/*
 * RTIC Delay Timer/DMA Throttle
 */
#define  RTIC_DMA              IO_ADDRESS(RTIC_BASE_ADDR + 0x0C)

/*
 * RTIC Memory A Address 1
 */
#define  RTIC_MEMAADDR1        IO_ADDRESS(RTIC_BASE_ADDR + 0x10)

/*
 * RTIC Memory A Length 1
 */
#define  RTIC_MEMALEN1         IO_ADDRESS(RTIC_BASE_ADDR + 0x14)

/*
 * RTIC Memory A Address 2
 */
#define  RTIC_MEMAADDR2        IO_ADDRESS(RTIC_BASE_ADDR + 0x18)
/*
 * RTIC Memory A Length 2
 */
#define  RTIC_MEMALEN2         IO_ADDRESS(RTIC_BASE_ADDR + 0x1C)

/*
 * RTIC Memory B Address 1
 */
#define  RTIC_MEMBADDR1        IO_ADDRESS(RTIC_BASE_ADDR + 0x30)

/*
 * RTIC Memory B Length 1
 */
#define  RTIC_MEMBLEN1         IO_ADDRESS(RTIC_BASE_ADDR + 0x34)

/*
 * RTIC Memory B Address 2
 */
#define  RTIC_MEMBADDR2        IO_ADDRESS(RTIC_BASE_ADDR + 0x38)

/*
 * RTIC Memory B Length 2
 */
#define  RTIC_MEMBLEN2         IO_ADDRESS(RTIC_BASE_ADDR + 0x3C)

/*
 *  RTIC Memory C Address 1
 */
#define  RTIC_MEMCADDR1        IO_ADDRESS(RTIC_BASE_ADDR + 0x50)

/*
 *  RTIC Memory C Length 1
 */
#define  RTIC_MEMCLEN1         IO_ADDRESS(RTIC_BASE_ADDR + 0x54)

/*
 *  RTIC Memery C Address 2
 */
#define  RTIC_MEMCADDR2        IO_ADDRESS(RTIC_BASE_ADDR + 0x58)

/*
 * RTIC Memory C Length 2
 */
#define  RTIC_MEMCLEN2         IO_ADDRESS(RTIC_BASE_ADDR + 0x5C)

/*
 * RTIC Memory D Address 1
 */
#define  RTIC_MEMDADDR1        IO_ADDRESS(RTIC_BASE_ADDR + 0x70)

/*
 * RTIC Memory D Length 1
 */
#define  RTIC_MEMDLEN1         IO_ADDRESS(RTIC_BASE_ADDR + 0x74)

/*
 * RTIC Memory D Address 2
 */
#define  RTIC_MEMDADDR2        IO_ADDRESS(RTIC_BASE_ADDR + 0x78)

/*
 * RTIC Memory D Length 2
 */
#define  RTIC_MEMDLEN2         IO_ADDRESS(RTIC_BASE_ADDR + 0x7C)

/*
 * RTIC Fault address Register.
 */
#define  RTIC_FAULTADDR       IO_ADDRESS(RTIC_BASE_ADDR + 0x90)

/*
 * RTIC Watchdog Timeout
 */
#define  RTIC_WDTIMER         IO_ADDRESS(RTIC_BASE_ADDR + 0x94)

/*
 * RTIC  Memory A Hash Result Word A
 */
#define  RTIC_MEMAHASHRES0    IO_ADDRESS(RTIC_BASE_ADDR + 0xA0)

/*
 * RTIC  Memory B Hash Result Word A
 */
#define  RTIC_MEMBHASHRES0    IO_ADDRESS(RTIC_BASE_ADDR + 0xC0)

/*
 * RTIC  Memory C Hash Result Word A
 */
#define  RTIC_MEMCHASHRES0    IO_ADDRESS(RTIC_BASE_ADDR + 0xE0)
/*
 * RTIC  Memory D Hash Result Word A
 */
#define  RTIC_MEMDHASHRES0     IO_ADDRESS(RTIC_BASE_ADDR + 0x100)
/*
 * RTIC  Memory Integrity Status
 */
#define  RTIC_STAT_MEMBLK       0x100

/*
 * RTIC  Address Error
 */
#define  RTIC_STAT_ADDR_ERR     0x200

/*
 * RTIC  Length Error
 */
#define  RTIC_STAT_LEN_ERR      0x400

/*
 * RTIC  Clear Interrupts command
 */
#define  RTIC_CMD_CLRIRQ        0x01

/*
 * RTIC  SW reset command
 */
#define  RTIC_CMD_SW_RESET      0x02

/*
 * RTIC  Hash Once command
 */
#define  RTIC_CMD_HASH_ONCE     0x04

/*
 * RTIC  Run Time Check Command
 */
#define  RTIC_CMD_RUN_TIME_CHK  0x08

/*
 * RTIC  IRQ EN
 */
#define  RTIC_CTL_IRQ_EN        0x01

/*
 * RTIC  Hash ALGO 256 Memory block A enable
 */
#define  RTIC_CTL_HASHALGO_MEMA_BLK_EN   0x1000

/*
 * RTIC  Hash ALGO 256 Memory block B Enable
 */
#define  RTIC_CTL_HASHALGO_MEMB_BLK_EN   0x2000

/*
 * RTIC  Hash ALGO 256 Memory block C Enable
 */
#define  RTIC_CTL_HASHALGO_MEMC_BLK_EN   0x4000

/*
 * RTIC  Hash ALGO 256 Memory block D Enable
 */
#define  RTIC_CTL_HASHALGO_MEMD_BLK_EN   0x8000

/*
 * RTIC  Hash Once Memory block A enable
 */
#define  RTIC_CTL_HASHONCE_MEMA_BLK_EN   0x10

/*
 * RTIC  Hash Once Memory block B Enable
 */
#define  RTIC_CTL_HASHONCE_MEMB_BLK_EN   0x20

/*
 * RTIC  Hash Once Memory block C Enable
 */
#define  RTIC_CTL_HASHONCE_MEMC_BLK_EN   0x40

/*
 * RTIC  Hash Once Memory block D Enable
 */
#define  RTIC_CTL_HASHONCE_MEMD_BLK_EN   0x80

/*
 * RTIC  Run Time Memory blk A Enable
 */
#define  RTIC_CTL_RUNTIME_MEMA_BLK_EN   0x100

/*
 * RTIC  Run Time Memory blk B Enable
 */
#define  RTIC_CTL_RUNTIME_MEMB_BLK_EN   0x200

/*
 * RTIC  Run Time Memory blk C Enable
 */
#define  RTIC_CTL_RUNTIME_MEMC_BLK_EN   0x400

/*
 * RTIC  Run Time Memory blk D Enable
 */
#define  RTIC_CTL_RUNTIME_MEMD_BLK_EN   0x800

/*!
 * Maximum block length that can be configured to the RTIC module.
 */
#define  RTIC_MAX_BLOCK_LENGTH          0xFFFFFFFC

/*!
 * This define is used to clear DMA Burst in RTIC Control register.
 */
#define  DMA_BURST_CLR          0xE

//#ifndef CONFIG_ARCH_MX27
/*!
 * This define will clear Hash Once DMA Throttle Programmable timer.
 */
#define HASH_ONCE_DMA_THROTTLE_CLR     (0xFF << 16)

//#endif                                /* CONFIG_ARCH_MX27 */

/*!
 * This define is used to configure DMA Burst read as 1 word.
 */
#define  DMA_1_WORD     0x00

/*!
 * This define is used to configure DMA Burst read as 2 words.
 */
#define  DMA_2_WORD     0x02

/*!
 * This define is used to configure DMA Burst read as 4 words.
 */
#define  DMA_4_WORD     0x04

/*!
 * This define is used to configure DMA Burst read as 8 words.
 */
#define  DMA_8_WORD     0x06

/*!
 * This define is used to configure DMA Burst read as 16 words.
 */
#define  DMA_16_WORD    0x08

/*!
 * This define is used for checking RTIC is busy or not.
 */
#define  RTIC_BUSY      0x01

#ifndef  CONFIG_ARCH_MX27
/*!
 * This define is used for checking RUN Time Disable bit is set or not.
 */
#define  RTIC_RUN_TIME_DISABLE          0x10

#endif				/*  CONFIG_ARCH_MX27 */

/*!
 * This define is used to check Done bit set in RTIC Status register.
 */
#define  RTIC_DONE      0x02
#endif				/* __MXC_RTIC_H__ */
