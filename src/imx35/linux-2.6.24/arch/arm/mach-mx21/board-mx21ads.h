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

#ifndef __ASM_ARCH_MXC_BOARD_MX21ADS_H__
#define __ASM_ARCH_MXC_BOARD_MX21ADS_H__

/*!
 * @defgroup BRDCFG Board Configuration Options
 * @ingroup MSL
 */

/*!
 * @file arch-mxc/board-mx21ads.h
 *
 * @brief This file contains all the board level configuration options.
 *
 * It currently hold the options defined for MX21 ADS Platform.
 *
 * @ingroup BRDCFG
 */

/*
 * Include Files
 */
#include <asm/arch/board.h>

/* Start of physical RAM */
#define PHYS_OFFSET	        UL(0xC0000000)

/* Size of contiguous memory for DMA and other h/w blocks */
#define CONSISTENT_DMA_SIZE	SZ_8M

/*!
 * @name MXC UART EVB board level configurations
 */
/*! @{ */
/*!
 * Specify the max baudrate for the MXC UARTs for your board, do not specify a max
 * baudrate greater than 1500000. This is used while specifying the UART Power
 * management constraints.
 */
#define MAX_UART_BAUDRATE       1500000
/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         0
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
#define UART3_MODE              MODE_DTE
#define UART3_IR                IRDA
#define UART3_ENABLED           1
/* UART 4 configuration */
#define UART4_MODE              MODE_DTE
#define UART4_IR                NO_IRDA
#define UART4_ENABLED           1	/* Enable UART 4 */

#define MXC_LL_EXTUART_PADDR	(CS1_BASE_ADDR + 0x200000)
#define MXC_LL_EXTUART_VADDR	CS1_IO_ADDRESS(MXC_LL_EXTUART_PADDR)
#define MXC_LL_EXTUART_16BIT_BUS

#define MXC_LL_UART_PADDR	UART1_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPI_IO_ADDRESS(UART1_BASE_ADDR)

/*!
 * @name Memory Size parameters
 */
/*! @{ */
/*!
 * Size of SDRAM memory
 */
#define SDRAM_MEM_SIZE          SZ_64M
/*!
 * Size of display buffer memory
 */
#define MXCLCDC_MEM_SIZE         SZ_8M
/*!
 * Size of memory available to kernel
 */
#define MEM_SIZE                (SDRAM_MEM_SIZE - MXCLCDC_MEM_SIZE)
/*! @} */

/*! @{ */
/*!
 * @name Keypad Configurations
 */
/*! @{ */
/*!
 * Maximum number of rows (0 to 7)
 */
#define MAXROW                          6
/*!
 * Maximum number of columns (0 to 7)
 */
#define MAXCOL                          6
/*! @} */

/*!
 * @name  Defines Base address and IRQ used for CS8900A Ethernet Controller on MXC Boards
 */
/*! @{*/
/*! This is System IRQ used by CS8900A for interrupt generation taken from platform.h */
#define CS8900AIRQ              IOMUX_TO_IRQ(MX21_PIN_UART3_RTS)
/*! This is I/O Base address used to access registers of CS8900A on MXC ADS */
#define CS8900A_BASE_ADDRESS    (IO_ADDRESS(CS1_BASE_ADDR))
/*! @} */

#endif				/* __ASM_ARCH_MXC_BOARD_MX21ADS_H__ */
