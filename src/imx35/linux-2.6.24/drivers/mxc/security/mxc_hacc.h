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
 * @file mxc_hacc.h
 *
 * @brief The header file for Hash Acceleration (HAC) module.
 * This file contains all register defines and bit definition of HAC module.
 *
 * @ingroup MXC_Security
 */

#ifndef __MXC_HACC_H__
#define __MXC_HACC_H__

#include <asm/arch/mxc_security_api.h>
#include <asm/hardware.h>
#include <asm/errno.h>

/*
 * HAC Control register
 */
#define  HAC_CTL         IO_ADDRESS(HAC_BASE_ADDR + 0x00)

/*
 * HAC Start Address Register
 */
#define  HAC_START_ADDR  IO_ADDRESS(HAC_BASE_ADDR + 0x04)

/*
 * HAC Block Count Register
 */
#define  HAC_BLK_CNT     IO_ADDRESS(HAC_BASE_ADDR + 0x08)

/*
 * HAC Hash Register 4 Register
 */
#define  HAC_HSH4        IO_ADDRESS(HAC_BASE_ADDR + 0x0C)

/*
 * HAC Hash Register 3 Register
 */
#define  HAC_HSH3        IO_ADDRESS(HAC_BASE_ADDR + 0x10)

/*
 * HAC Hash Register 2 Register
 */
#define  HAC_HSH2        IO_ADDRESS(HAC_BASE_ADDR + 0x14)

/*
 * HAC Hash Register 1 Register
 */
#define  HAC_HSH1        IO_ADDRESS(HAC_BASE_ADDR + 0x18)

/*
 * HAC Hash Register 0 Register
 */
#define  HAC_HSH0        IO_ADDRESS(HAC_BASE_ADDR + 0x1C)

/*!
 * HAC Hash Done status
 */
#define         HAC_CTL_DONE            0x01

/*!
 * HAC  Hash Error Status
 */
#define         HAC_CTL_ERROR           0x02

/*!
 * HAC  Hash Accelerator Module Busy Status
 */
#define         HAC_CTL_BUSY            0x04

/*!
 * HAC  HAC Interrupt Mask Bit.
 */
#define         HAC_CTL_IMSK            0x08

/*!
 * HAC  Command to Stop hash processing
 */
#define         HAC_CTL_STOP            0x10

/*!
 * HAC  Command for software reset
 */
#define         HAC_CTL_SWRST           0x20

/*!
 * HAC  Command to start hash processing
 */
#define         HAC_CTL_START           0x40

/*!
 * HAC  Command to continue hash processing
 */
#define         HAC_CTL_CONTINUE        0x80

/*!
 * HAC  Command to do padding
 */
#define         HAC_CTL_PAD             0x100

/*!
 * HAC  Command not to do burst read
 */
#define         HAC_CTL_NO_BURST_READ   0x600

/*!
 * HAC  Command to do 4 word Burst read
 */
#define         HAC_CTL_4WORD_BURST     0x100

/*!
 * HAC  Command to do 8 word Burst read
 */
#define         HAC_CTL_8WORD_BURST     0x400

/*!
 * HAC  Command to do 16 word Burst read
 */
#define         HAC_CTL_16WORD_BURST    0x000

/*!
 * HAC  Command to configure the burst mode
 */
#define         HAC_CTL_BURST_MODE      0x800

/*!
 * Maximum block length that can be configured to the HAC module.
 */
#define         HAC_MAX_BLOCK_LENGTH    0x7FFFFF

#endif				/* __MXC_HACC_H__ */
