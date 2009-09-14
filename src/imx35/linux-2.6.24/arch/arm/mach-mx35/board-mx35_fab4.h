/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARCH_MXC_BOARD_MX35_FAB4_H__
#define __ASM_ARCH_MXC_BOARD_MX35_FAB4_H__

#ifdef CONFIG_MACH_FAB4

/*!
 * @defgroup BRDCFG_MX35 Board Configuration Options
 * @ingroup MSL_MX35
 */

/*!
 * @file mach-mx35/board-mx35_fab4.h
 *
 * @brief This file contains all the board level configuration options.
 *
 * It currently hold the options defined for Logitech MX35 Fab4 Platform.
 *
 * @ingroup BRDCFG_MX35
 */

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
#define UART3_IR                NO_IRDA
#define UART3_ENABLED           0

#define MXC_LL_UART_PADDR	UART1_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART1_BASE_ADDR)

#define MXC_PSEUDO_PARENT	MXC_INT_RESV0


#define AHB_FREQ                133000000
#define IPG_FREQ                66500000


extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern void mx35_fab4_gpio_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);
extern int mxc_clocks_init(void);
extern void __init early_console_setup(char *);
extern void gpio_tsc_active(void);
extern void gpio_tsc_inactive(void);
extern unsigned int sdhc_get_card_det_status(struct device *dev);
extern unsigned int sdhc_get_card_wp_status(void);

#endif				/* CONFIG_MACH_FAB4 */
#endif				/* __ASM_ARCH_MXC_BOARD_MX35_FAB4_H__ */
