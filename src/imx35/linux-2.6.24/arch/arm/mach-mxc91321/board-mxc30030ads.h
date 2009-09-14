/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARCH_MXC_BOARD_MXC30030ADS_H__
#define __ASM_ARCH_MXC_BOARD_MXC30030ADS_H__

/*
 * Include Files
 */
#include <asm/arch/mxc_uart.h>

/*!
 * @name MXC UART EVB board level configurations
 */
/*! @{ */
/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         MXC_UARTUCR3_INVT
/*!
 * Specifies if the Irda receive path is inverting
 */
#define MXC_IRDA_RX_INV         0
/* UART 1 configuration */
/*!
 * This define specifies if the UART port is configured to be in DTE or
 * DCE mode. There exists a define like this for each UART port. Valid
 * values that can be used are \b MODE_DTE or \b MODE_DCE.
 */
#define UART1_MODE              MODE_DCE
/*!
 * This define specifies if the UART is to be used for IRDA. There exists a
 * define like this for each UART port. Valid values that can be used are
 * \b IRDA or \b NO_IRDA.
 */
#define UART1_IR                NO_IRDA
/*!
 * This define is used to enable or disable a particular UART port. If
 * disabled, the UART will not be registered in the file system and the user
 * will not be able to access it. There exists a define like this for each UART
 * port. Specify a value of 1 to enable the UART and 0 to disable it.
 */
#define UART1_ENABLED           1
/*! @} */
/* UART 2 configuration */
#define UART2_MODE              MODE_DCE
#define UART2_IR                NO_IRDA
#define UART2_ENABLED           1
/* UART 3 configuration */
#define UART3_MODE              MODE_DCE
#define UART3_IR                NO_IRDA
#define UART3_ENABLED           1
/* UART 4 configuration */
#define UART4_MODE              MODE_DCE
#define UART4_IR                IRDA
#ifdef CONFIG_MXC_FIR_MODULE
#define UART4_ENABLED           0
#else
#define UART4_ENABLED           1
#endif

#define MXC_LL_UART_PADDR       UART3_BASE_ADDR
#define MXC_LL_UART_VADDR       SPBA0_IO_ADDRESS(UART3_BASE_ADDR)

/*!
 * @name  Defines Base address and IRQ used for CS8900A Ethernet Controller on MXC Boards
 */
/*! @{*/
/*! This is System IRQ used by CS8900A for interrupt generation taken from platform.h */
#define CS8900AIRQ              MXC_INT_EXT_INT5
/*! This is I/O Base address used to access registers of CS8900A on MXC ADS */
#define CS8900A_BASE_ADDRESS    (IO_ADDRESS(CS2_BASE_ADDR) + 0x300)
/*! @} */

#define MXC_PMIC_INT_LINE       MXC_INT_EXT_INT1

/*
 * Board specific REF, AHB and IPG frequencies
 */
#define AHB_FREQ                100000000
#define IPG_FREQ                50000000

#endif				/* __ASM_ARCH_MXC_BOARD_MXC30030EVB_H__ */
