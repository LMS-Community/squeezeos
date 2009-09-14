/*
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>

#include "iomux.h"

/*!
 * @file mach-mx51/mx51_3stack_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */

void gpio_activate_audio_ports(void);

/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	/*
	 * Configure the IOMUX control registers for the UART signals
	 * and enable the UART transceivers
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		mxc_request_iomux(MX51_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART1_RXD, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_request_iomux(MX51_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART1_TXD, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_request_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART1_RTS, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH);
		mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RTS_B_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_request_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART1_CTS, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH);
		break;
		/* UART 2 IOMUX Configs */
	case 1:
		mxc_request_iomux(MX51_PIN_UART2_RXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART2_RXD, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_request_iomux(MX51_PIN_UART2_TXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART2_TXD, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		/* UART2_RTS */
		mxc_request_iomux(MX51_PIN_EIM_D26, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX51_PIN_EIM_D26, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RTS_B_SELECT_INPUT,
				    INPUT_CTL_PATH2);
		/* UART2_CTS */
		mxc_request_iomux(MX51_PIN_EIM_D25, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX51_PIN_EIM_D25, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		break;
	case 2:
		/* UART 3 IOMUX Configs */
		mxc_request_iomux(MX51_PIN_UART3_RXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART3_RXD, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_request_iomux(MX51_PIN_UART3_TXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART3_TXD, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		/* UART3_RTS */
		mxc_request_iomux(MX51_PIN_EIM_D27, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX51_PIN_EIM_D27, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RTS_B_SELECT_INPUT,
				    INPUT_CTL_PATH2);
		/* UART3_CTS */
		mxc_request_iomux(MX51_PIN_EIM_D24, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX51_PIN_EIM_D24, PAD_CTL_HYS_NONE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_uart_active);

/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
	switch (port) {
		/* UART 1 */
	case 0:
		mxc_request_gpio(MX51_PIN_UART1_RXD);
		mxc_request_gpio(MX51_PIN_UART1_TXD);
		mxc_request_gpio(MX51_PIN_UART1_RTS);
		mxc_request_gpio(MX51_PIN_UART1_CTS);

		mxc_free_iomux(MX51_PIN_UART1_RXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_UART1_TXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_GPIO);
		break;
		/* UART 2 */
	case 1:
		mxc_request_gpio(MX51_PIN_UART2_RXD);
		mxc_request_gpio(MX51_PIN_UART2_TXD);

		mxc_free_iomux(MX51_PIN_UART2_RXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_UART2_TXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_EIM_D26, IOMUX_CONFIG_ALT4);
		mxc_free_iomux(MX51_PIN_EIM_D25, IOMUX_CONFIG_ALT4);
		break;
	case 2:
		/* UART 3 */
		mxc_request_gpio(MX51_PIN_UART3_RXD);
		mxc_request_gpio(MX51_PIN_UART3_TXD);
		mxc_request_gpio(MX51_PIN_EIM_D27);
		mxc_request_gpio(MX51_PIN_EIM_D24);

		mxc_free_iomux(MX51_PIN_UART3_RXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_UART3_TXD, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_EIM_D27, IOMUX_CONFIG_GPIO);
		mxc_free_iomux(MX51_PIN_EIM_D24, IOMUX_CONFIG_GPIO);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_uart_inactive);

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{

}

EXPORT_SYMBOL(config_uartdma_event);

/*!
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 */
void gpio_spi_active(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1 */
		mxc_request_iomux(MX51_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MISO, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_CSPI1_RDY, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_RDY, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST);
		break;
	case 1:
		/* SPI2 */
		mxc_request_iomux(MX51_PIN_NANDF_RB2, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB2, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB3, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB3, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB4, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB4, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB7, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB7, PAD_CTL_DRV_VOT_HIGH |
				  PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_PUE_KEEPER | PAD_CTL_100K_PU |
				  PAD_CTL_ODE_OPENDRAIN_NONE |
				  PAD_CTL_DRV_HIGH);
		break;
	case 2:
		/* SPI3 */
		mxc_request_iomux(MX51_PIN_USBH1_NXT, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX51_PIN_USBH1_NXT, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_USBH1_DIR, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX51_PIN_USBH1_DIR, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_USBH1_CLK, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX51_PIN_USBH1_CLK, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);

		mxc_request_iomux(MX51_PIN_USBH1_DATA5, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX51_PIN_USBH1_DATA5, PAD_CTL_HYS_ENABLE |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_spi_active);

/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1 */
		break;
	case 1:
		/* SPI2 */
		mxc_request_iomux(MX51_PIN_NANDF_RB2, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB2, PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB3, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB3, PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB4, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB4, PAD_CTL_PUE_KEEPER |
				  PAD_CTL_PKE_ENABLE);

		mxc_request_iomux(MX51_PIN_NANDF_RB7, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB7, PAD_CTL_DRV_VOT_HIGH |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
				  PAD_CTL_100K_PU);
		break;
	case 2:
		/* SPI3 */
		break;
	default:
		break;
	}

}

EXPORT_SYMBOL(gpio_spi_inactive);

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_active(void)
{

}

EXPORT_SYMBOL(gpio_owire_active);

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_inactive(void)
{

}

EXPORT_SYMBOL(gpio_owire_inactive);

/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		/*i2c1 sda */
		mxc_request_iomux(MX51_PIN_CSPI1_MOSI,
				  IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				    INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH |
				  PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_VOT_LOW | PAD_CTL_DDR_INPUT_CMOS);

		/*i2c1 scl */
		mxc_request_iomux(MX51_PIN_CSPI1_SCLK,
				  IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				    INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH |
				  PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_VOT_LOW | PAD_CTL_DDR_INPUT_CMOS);
		break;
	case 1:
		mxc_request_iomux(MX51_PIN_GPIO1_2,
				  IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_GPIO1_3,
				  IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);

		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
				    INPUT_CTL_PATH3);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				    INPUT_CTL_PATH3);

		mxc_iomux_set_pad(MX51_PIN_GPIO1_2,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH |
				  PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_VOT_LOW | PAD_CTL_DDR_INPUT_CMOS);
		mxc_iomux_set_pad(MX51_PIN_GPIO1_3,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH |
				  PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE |
				  PAD_CTL_DRV_VOT_LOW | PAD_CTL_DDR_INPUT_CMOS);

		break;
	case 2:
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_i2c_active);

/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		/*i2c1 sda */
		mxc_request_iomux(MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI,
				  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE);
		/*i2c1 scl */
		mxc_request_iomux(MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK,
				  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE);
		break;
	case 1:
		mxc_request_iomux(MX51_PIN_GPIO1_2, IOMUX_CONFIG_ALT0);
		mxc_request_iomux(MX51_PIN_GPIO1_3, IOMUX_CONFIG_ALT0);

		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
				    INPUT_CTL_PATH0);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				    INPUT_CTL_PATH0);

		mxc_iomux_set_pad(MX51_PIN_GPIO1_2,
				  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE);
		mxc_iomux_set_pad(MX51_PIN_GPIO1_3,
				  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE);
		break;
	case 2:
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_i2c_inactive);

void gpio_pmic_active(void)
{
	mxc_request_iomux(MX51_PIN_GPIO1_5, IOMUX_CONFIG_GPIO
			  | IOMUX_CONFIG_SION);
	mxc_iomux_set_pad(MX51_PIN_GPIO1_5, PAD_CTL_SRE_SLOW |
			  PAD_CTL_ODE_OPENDRAIN_NONE |
			  PAD_CTL_DRV_MEDIUM |
			  PAD_CTL_100K_PU |
			  PAD_CTL_HYS_ENABLE |
			  PAD_CTL_DRV_VOT_HIGH | PAD_CTL_DDR_INPUT_CMOS);
	mxc_set_gpio_direction(MX51_PIN_GPIO1_5, 1);
}

EXPORT_SYMBOL(gpio_pmic_active);

/*!
 * This function activates DAM ports 4 & 5 to enable
 * audio I/O.
 */
void gpio_activate_audio_ports(void)
{
}

EXPORT_SYMBOL(gpio_activate_audio_ports);

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module)
{
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
int sdhc_get_card_det_status(struct device *dev)
{
	return 0;
}

EXPORT_SYMBOL(sdhc_get_card_det_status);

/*
 * Return the card detect pin.
 */
int sdhc_init_card_det(int id)
{
	return 0;
}

EXPORT_SYMBOL(sdhc_init_card_det);

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_lcd_active(void)
{
}

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
}

/*!
 * Setup pins for SLCD to be active
 *
 */
void slcd_gpio_config(void)
{
}

/*!
 * Switch to the specified sensor
 *
 */
void gpio_sensor_select(int sensor)
{
}

/*!
 * Setup GPIO for sensor to be active
 *
 */
void gpio_sensor_active(void)
{
}

EXPORT_SYMBOL(gpio_sensor_active);

/*!
 * Setup GPIO for sensor to be inactive
 *
 */
void gpio_sensor_inactive(void)
{
}

EXPORT_SYMBOL(gpio_sensor_inactive);

/*!
 * Setup GPIO for ATA interface
 *
 */
void gpio_ata_active(void)
{
}

EXPORT_SYMBOL(gpio_ata_active);

/*!
 * Restore ATA interface pins to reset values
 *
 */
void gpio_ata_inactive(void)
{
}

EXPORT_SYMBOL(gpio_ata_inactive);

/*!
 * Setup GPIO for Keypad  to be active
 *
 */
void gpio_keypad_active(void)
{
	/*
	 * Configure the IOMUX control register for keypad signals.
	 */
	mxc_request_iomux(MX51_PIN_KEY_COL0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0);
}

EXPORT_SYMBOL(gpio_keypad_active);

/*!
 * Setup GPIO for Keypad to be inactive
 *
 */
void gpio_keypad_inactive(void)
{
	mxc_request_gpio(MX51_PIN_KEY_COL0);
	mxc_request_gpio(MX51_PIN_KEY_COL1);
	mxc_request_gpio(MX51_PIN_KEY_COL2);
	mxc_request_gpio(MX51_PIN_KEY_COL3);
	mxc_request_gpio(MX51_PIN_KEY_COL4);
	mxc_request_gpio(MX51_PIN_KEY_COL5);
	mxc_request_gpio(MX51_PIN_KEY_ROW0);
	mxc_request_gpio(MX51_PIN_KEY_ROW1);
	mxc_request_gpio(MX51_PIN_KEY_ROW2);
	mxc_request_gpio(MX51_PIN_KEY_ROW3);

	mxc_free_iomux(MX51_PIN_KEY_COL0, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_COL1, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_COL2, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_COL3, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_COL4, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_COL5, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_ROW0, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_ROW1, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_ROW2, IOMUX_CONFIG_GPIO);
	mxc_free_iomux(MX51_PIN_KEY_ROW3, IOMUX_CONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_keypad_inactive);

/*
 * USB OTG HS port
 */
int gpio_usbotg_hs_active(void)
{
	return 0;
}

EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void)
{
}

EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

/*!
 * Setup GPIO for PCMCIA interface
 *
 */
void gpio_pcmcia_active(void)
{
}

EXPORT_SYMBOL(gpio_pcmcia_active);

/*!
 * Setup GPIO for pcmcia to be inactive
 */
void gpio_pcmcia_inactive(void)
{
}

EXPORT_SYMBOL(gpio_pcmcia_inactive);

/*!
 * Setup GPIO for fec to be active
 */
void gpio_fec_active(void)
{
}

EXPORT_SYMBOL(gpio_fec_active);

/*!
 * Setup GPIO for fec to be inactive
 */
void gpio_fec_inactive(void)
{
}

EXPORT_SYMBOL(gpio_fec_inactive);

void gpio_spdif_active(void)
{
}

EXPORT_SYMBOL(gpio_spdif_active);

void gpio_spdif_inactive(void)
{

}

EXPORT_SYMBOL(gpio_spdif_inactive);
