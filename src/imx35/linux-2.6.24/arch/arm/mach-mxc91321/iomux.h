/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __MACH_MXC91321_IOMUX_H__
#define __MACH_MXC91321_IOMUX_H__

#include <asm/arch/gpio.h>
#include "mxc91321_pins.h"

/*!
 * @file mach-mxc91321/iomux.h
 *
 * @brief I/O Muxing control definitions and functions
 *
 * @ingroup GPIO_MXC91321
 */

/*
 * TODO: wait for MXC91321 IC Spec update. Now it is not consistent.
 */
enum iopad_pins {
	DUMMYDUMMY,
};

/*!
 * various IOMUX output functions
 */
enum iomux_output_config {
	OUTPUTCONFIG_GPIO = 0,	/*!< used as GPIO */
	OUTPUTCONFIG_FUNC,	/*!< used as function */
	OUTPUTCONFIG_ALT1,	/*!< used as alternate function 1 */
	OUTPUTCONFIG_ALT2,	/*!< used as alternate function 2 */
	OUTPUTCONFIG_ALT3,	/*!< used as alternate function 3 */
	OUTPUTCONFIG_ALT4,	/*!< used as alternate function 4 */
	OUTPUTCONFIG_ALT5,	/*!< used as alternate function 5 */
	OUTPUTCONFIG_ALT6	/*!< used as alternate function 6 */
};

/*!
 * various IOMUX input functions
 */
enum iomux_input_config {
	INPUTCONFIG_NONE = 0,	/*!< not configured for input */
	INPUTCONFIG_GPIO = 1 << 0,	/*!< used as GPIO */
	INPUTCONFIG_FUNC = 1 << 1,	/*!< used as function */
	INPUTCONFIG_ALT1 = 1 << 2,	/*!< used as alternate function 1 */
	INPUTCONFIG_ALT2 = 1 << 3	/*!< used as alternate function 2 */
};

/*!
 * various IOMUX pad functions
 */
enum iomux_pad_config {
	PAD_CTL_NOLOOPBACK = 0x0 << 9,
	PAD_CTL_LOOPBACK = 0x1 << 9,
	PAD_CTL_PKE_NONE = 0x0 << 8,
	PAD_CTL_PKE_ENABLE = 0x1 << 8,
	PAD_CTL_PUE_KEEPER = 0x0 << 7,
	PAD_CTL_PUE_PUD = 0x1 << 7,
	PAD_CTL_100K_PD = 0x0 << 5,
	PAD_CTL_100K_PU = 0x1 << 5,
	PAD_CTL_47K_PU = 0x2 << 5,
	PAD_CTL_22K_PU = 0x3 << 5,
	PAD_CTL_HYS_CMOS = 0x0 << 4,
	PAD_CTL_HYS_SCHMITZ = 0x1 << 4,
	PAD_CTL_ODE_CMOS = 0x0 << 3,
	PAD_CTL_ODE_OpenDrain = 0x1 << 3,
	PAD_CTL_DS_NORMAL = 0x0 << 1,
	PAD_CTL_DS_HIGH = 0x2 << 1,
	PAD_CTL_DS_MAX = 0x1 << 1,
	PAD_CTL_SRE_SLOW = 0x0 << 0,
	PAD_CTL_SRE_FAST = 0x1 << 0
};

/*!
 * various IOMUX general purpose functions
 */
enum iomux_gp_func {
	MUX_WCSI_DIVERSITY_MODE = 0x1 << 3,
	MUX_WCSI_PGP_REG_RX_WB_CORR_CTRL0 = 0x1 << 4,
	MUX_WCSI_PGP_REG_RX_WB_CORR_CTRL1 = 0x1 << 5,
	MUX_WCSI_PGP_REG_RX_ALT_WB_QSEL0 = 0x1 << 6,
	MUX_WCSI_PGP_REG_RX_ALT_WB_QSEL1 = 0x1 << 7,
	MUX_WCSI_PGP_REG_RX_ALT_WB_QSEL2 = 0x1 << 8,
	MUX_WCSI_PGP_REG_RX_ALT_WB_ISEL0 = 0x1 << 9,
	MUX_WCSI_PGP_REG_RX_ALT_WB_ISEL1 = 0x1 << 10,
	MUX_WCSI_PGP_REG_RX_ALT_WB_ISEL2 = 0x1 << 11,
	MUX_WCSI_PGP_REG_RX_ALT_WB_EN = 0x1 << 12,
	MUX_WCSI_PGP_REG_BBIF_MODE = 0x1 << 13,
	MUX_PGP_WCSI_TX_DIS_FRM_RANDOM = 0x1 << 14,
	MUX_PGP_USB2_VPIN = 0x1 << 15,
	MUX_PGP_UART3_RXD = 0x1 << 16,
	MUX_SDCTL_SPBA_SEL = 0x1 << 17,
	MUX_NFC_CE_SEL = 0x1 << 18,
	MUX_PGP_FIRI = 0x1 << 19,
	MUX_SDCTL_CSD1_SEL_B = 0x1 << 20,
	MUX_SDCTL_CSD0_SEL_B = 0x1 << 21
};

/*!
 * This function is used to configure a pin through the IOMUX module.
 *
 * @param  pin          pin number as defined in \b #iomux_pins
 * @param  out          output function as defined in \b #iomux_output_config
 * @param  in           input function as defined in \b #iomux_input_config
 */
void iomux_config_mux(enum iomux_pins pin,
		      enum iomux_output_config out, enum iomux_input_config in);

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin          a pin number as defined in \b #iomux_pins
 * @param  config       ORed value of elements defined in \b #iomux_pad_config
 */
void iomux_config_pad(enum iomux_pins pin, __u32 config);

/*!
 * This function enables/disables the general purpose function for a particular
 * signal.
 *
 * @param  gp   one signal as defined in \b enum \b #iomux_gp_func
 * @param  en   \b #true to enable; \b #false to disable
 */
void iomux_config_gpr(enum iomux_gp_func gp, bool en);

#endif				/* __MACH_MXC91321_IOMUX_H__ */
