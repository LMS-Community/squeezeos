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
 * @file mxc_mu_reg.h
 *
 * @brief This file provides all the Register Addresses, Bit definitions.
 *
 * @ingroup MU
 */
#ifndef __MXC_MU_REG_H__

#define __MXC_MU_REG_H__

#include <asm/hardware.h>

/*
 * Address offsets of the MU MCU side transmit registers
 */
#define AS_MUMTR0			IO_ADDRESS(MU_BASE_ADDR + 0x000)	/* Transmit register 0 */
#define AS_MUMTR1			IO_ADDRESS(MU_BASE_ADDR + 0x004)	/* Transmit register 1 */
#define AS_MUMTR2			IO_ADDRESS(MU_BASE_ADDR + 0x008)	/* Transmit register 2 */
#define AS_MUMTR3			IO_ADDRESS(MU_BASE_ADDR + 0x00c)	/* Transmit register 3 */

/*
 * Address offsets of the MU MCU side receive registers
 */
#define AS_MUMRR0			IO_ADDRESS(MU_BASE_ADDR + 0x010)	/* Receive register 0 */
#define AS_MUMRR1			IO_ADDRESS(MU_BASE_ADDR + 0x014)	/* Receive register 1 */
#define AS_MUMRR2			IO_ADDRESS(MU_BASE_ADDR + 0x018)	/* Receive register 2 */
#define AS_MUMRR3			IO_ADDRESS(MU_BASE_ADDR + 0x01c)	/* Receive register 3 */

/*
 * Address offset of the MU MCU side status register
 */
#define AS_MUMSR			IO_ADDRESS(MU_BASE_ADDR + 0x020)	/* Status register */

/*
 * Address offset of the MU MCU side control register
 */
#define AS_MUMCR			IO_ADDRESS(MU_BASE_ADDR + 0x024)	/* Control register */

/*
 * Bit definitions of MSR
 */
#define AS_MUMSR_MGIP0			0x80000000
#define AS_MUMSR_MGIP1			0x40000000
#define AS_MUMSR_MGIP2			0x20000000
#define AS_MUMSR_MGIP3			0x10000000
#define AS_MUMSR_MRF0			0x08000000
#define AS_MUMSR_MRF1			0x04000000
#define AS_MUMSR_MRF2			0x02000000
#define AS_MUMSR_MRF3			0x01000000
#define AS_MUMSR_MTE0			0x00800000
#define AS_MUMSR_MTE1			0x00400000
#define AS_MUMSR_MTE2			0x00200000
#define AS_MUMSR_MTE3			0x00100000
#define AS_MUMSR_MFUP			0x00000100
#define AS_MUMSR_DRS			0x00000080
#define AS_MUMSR_DPM1			0x00000040
#define AS_MUMSR_DPM0			0x00000020
#define AS_MUMSR_MEP			0x00000010
#define AS_MUMSR_MNMIC			0x00000008
#define AS_MUMSR_MF2			0x00000004
#define AS_MUMSR_MF1			0x00000002
#define AS_MUMSR_MF0			0x00000001

/*
 * Bit definitions of MCR
 */
#define AS_MUMCR_MGIE0			0x80000000
#define AS_MUMCR_MGIE1			0x40000000
#define AS_MUMCR_MGIE2			0x20000000
#define AS_MUMCR_MGIE3			0x10000000
#define AS_MUMCR_MRIE0			0x08000000
#define AS_MUMCR_MRIE1			0x04000000
#define AS_MUMCR_MRIE2			0x02000000
#define AS_MUMCR_MRIE3			0x01000000
#define AS_MUMCR_MTIE0			0x00800000
#define AS_MUMCR_MTIE1			0x00400000
#define AS_MUMCR_MTIE2			0x00200000
#define AS_MUMCR_MTIE3			0x00100000
#define AS_MUMCR_MGIR0			0x00080000
#define AS_MUMCR_MGIR1			0x00040000
#define AS_MUMCR_MGIR2			0x00020000
#define AS_MUMCR_MGIR3			0x00010000
#define AS_MUMCR_MMUR			0x00000020
#define AS_MUMCR_DHR			0x00000010
#define AS_MUMCR_DNMI			0x00000008
#define AS_MUMCR_MDF2			0x00000004
#define AS_MUMCR_MDF1			0x00000002
#define AS_MUMCR_MDF0			0x00000001

#define BASE_NUM			1222

#endif				/* __MXC_MU_REG_H__ */
