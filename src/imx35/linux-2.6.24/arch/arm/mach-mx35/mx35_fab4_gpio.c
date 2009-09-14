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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include "board-mx35_fab4.h"
#include "crm_regs.h"
#include "iomux.h"

/*!
 * @file mach-mx35/mx35_fab4_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO_MX35
 */

/*!
 * This system-wise GPIO function initializes the pins during system startup.
 * All the statically linked device drivers should put the proper GPIO
 * initialization code inside this function. It is called by \b fixup_mx31ads()
 * during system startup. This function is board specific.
 */
void mx35_fab4_gpio_init(void)
{
	/* I2C_ATTN1# */
	mxc_request_iomux(MX35_PIN_I2C2_CLK, MUX_CONFIG_ALT5);
	mxc_iomux_set_input(MUX_IN_GPIO2_IN_26, INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX35_PIN_I2C2_CLK, 0);
	mxc_set_gpio_direction(MX35_PIN_I2C2_CLK, 1);

	/* I2C_ATTN2# */
	mxc_request_iomux(MX35_PIN_I2C2_DAT, MUX_CONFIG_ALT5);
	mxc_iomux_set_input(MUX_IN_GPIO2_IN_27, INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX35_PIN_I2C2_DAT, 0);
	mxc_set_gpio_direction(MX35_PIN_I2C2_DAT, 1);

	/* DAC1_SMUTE : muted on boot */
	/* output, ATA_INTRQ/GPIO2_29 */
	mxc_request_iomux(MX35_PIN_ATA_INTRQ, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_ATA_INTRQ, 0);
	mxc_set_gpio_dataout(MX35_PIN_ATA_INTRQ, 1);
	mxc_set_gpio_direction(MX35_PIN_ATA_INTRQ, 0);

	/* OSC_SEL1 */
	/* output, TX4_RX1/GPIO1_11 */
	mxc_request_iomux(MX35_PIN_TX4_RX1, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_TX4_RX1, 0);
	mxc_set_gpio_dataout(MX35_PIN_TX4_RX1, 0);
	mxc_set_gpio_direction(MX35_PIN_TX4_RX1, 0);

	/* OSC_SEL2 */
	/* output, TX5_RX0/GPIO1_10 */
	mxc_request_iomux(MX35_PIN_TX5_RX0, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_TX5_RX0, 0);
	mxc_set_gpio_dataout(MX35_PIN_TX5_RX0, 0);
	mxc_set_gpio_direction(MX35_PIN_TX5_RX0, 0);

	/* HDP_EN : disabled on boot, or the analogue outs pop */
	/* output, ATA_RESET#/GPIO2_11 */
	mxc_request_iomux(MX35_PIN_ATA_RESET_B, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_ATA_RESET_B, 0);
	mxc_set_gpio_dataout(MX35_PIN_ATA_RESET_B, 0);
	mxc_set_gpio_direction(MX35_PIN_ATA_RESET_B, 0);

	/* HDP_DET */
	/* input, ATA_DIOW/GPIO2_9 */
	mxc_request_iomux(MX35_PIN_ATA_DIOW, MUX_CONFIG_ALT5);
	mxc_iomux_set_input(MUX_IN_GPIO2_IN_9, INPUT_CTL_PATH1);
	if (system_rev <= 2) {
		/* pull-up missing on PA2 */
		mxc_iomux_set_pad(MX35_PIN_ATA_DIOW, PAD_CTL_100K_PU);
	}
	else {
		mxc_iomux_set_pad(MX35_PIN_ATA_DIOW, 0);
	}
	mxc_set_gpio_direction(MX35_PIN_ATA_DIOW, 1);

	/* WIFI_PD# */
	/* output, ATA_DIOR/GPIO2_8 */
	mxc_request_iomux(MX35_PIN_ATA_DIOR, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_ATA_DIOR, 0);
	mxc_set_gpio_dataout(MX35_PIN_ATA_DIOR, 0);
	mxc_set_gpio_direction(MX35_PIN_ATA_DIOR, 0);

	/* WIFI_RESET# */
	/* output, ATA_CS1/GPIO2_7 */
	mxc_request_iomux(MX35_PIN_ATA_CS1, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_ATA_CS1, 0);
	mxc_set_gpio_dataout(MX35_PIN_ATA_CS1, 0);
	mxc_set_gpio_direction(MX35_PIN_ATA_CS1, 0);

	/* FAIL_SAFE# */
	/* input, FST/GPIO1_8 */
	mxc_request_iomux(MX35_PIN_FST, MUX_CONFIG_ALT5);
	mxc_iomux_set_input(MUX_IN_GPIO1_IN_8, INPUT_CTL_PATH1);
	mxc_iomux_set_pad(MX35_PIN_FST, 0);
	mxc_set_gpio_direction(MX35_PIN_FST, 1);

	/* LCDP_DISP */
	// FIXME we may not want this enabled (high)
	// output, D3_SPL/GPIO1_5
	mxc_request_iomux(MX35_PIN_D3_SPL, MUX_CONFIG_ALT5);
	mxc_iomux_set_pad(MX35_PIN_D3_SPL, 0);
	mxc_set_gpio_dataout(MX35_PIN_D3_SPL, 1);
	mxc_set_gpio_direction(MX35_PIN_D3_SPL, 0);


	/* Power up WLAN */
	/* FIXME move this to wlan sdi driver */
	mxc_set_gpio_dataout(MX35_PIN_ATA_CS1, 0); /* reset is active */
	mxc_set_gpio_dataout(MX35_PIN_ATA_DIOR, 1); /* power-down is inactive */
	msleep(50); /* wait 50 ms */
	mxc_set_gpio_dataout(MX35_PIN_ATA_CS1, 1); /* reset is inactive */
	msleep(1000); /* wait 1000 ms, to allow the wifi to boot */
}

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
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		mxc_request_iomux(MX35_PIN_RXD1, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_TXD1, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_RTS1, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CTS1, MUX_CONFIG_FUNC);

		mxc_iomux_set_pad(MX35_PIN_RXD1,
				  PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX35_PIN_TXD1,
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);
		mxc_iomux_set_pad(MX35_PIN_RTS1,
				  PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX35_PIN_CTS1,
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);

		break;
		/* UART 2 IOMUX Configs */
	case 1:
		mxc_request_iomux(MX35_PIN_TXD2, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_RXD2, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_RTS2, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CTS2, MUX_CONFIG_FUNC);
		mxc_iomux_set_pad(MX35_PIN_RXD2,
				  PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX35_PIN_TXD2,
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);
		mxc_iomux_set_pad(MX35_PIN_RTS2,
				  PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX35_PIN_CTS2,
				  PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);
		break;
		/* UART 3 IOMUX Configs */
	case 2:
		mxc_request_iomux(MX35_PIN_FEC_TX_CLK, MUX_CONFIG_ALT2);
		mxc_request_iomux(MX35_PIN_FEC_RX_CLK, MUX_CONFIG_ALT2);
		mxc_request_iomux(MX35_PIN_FEC_COL, MUX_CONFIG_ALT2);
		mxc_request_iomux(MX35_PIN_FEC_RX_DV, MUX_CONFIG_ALT2);

		mxc_iomux_set_input(MUX_IN_UART3_UART_RTS_B, INPUT_CTL_PATH2);
		mxc_iomux_set_input(MUX_IN_UART3_UART_RXD_MUX, INPUT_CTL_PATH3);
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
	case 0:
		mxc_request_gpio(MX35_PIN_RXD1);
		mxc_request_gpio(MX35_PIN_TXD1);
		mxc_request_gpio(MX35_PIN_RTS1);
		mxc_request_gpio(MX35_PIN_CTS1);

		mxc_free_iomux(MX35_PIN_RXD1, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_TXD1, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_RTS1, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CTS1, MUX_CONFIG_GPIO);
		break;
	case 1:
		mxc_request_gpio(MX35_PIN_RXD2);
		mxc_request_gpio(MX35_PIN_TXD2);
		mxc_request_gpio(MX35_PIN_RTS2);
		mxc_request_gpio(MX35_PIN_CTS2);

		mxc_free_iomux(MX35_PIN_RXD2, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_TXD2, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_RTS2, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CTS2, MUX_CONFIG_GPIO);
		break;
	case 2:
		mxc_request_gpio(MX35_PIN_FEC_TX_CLK);
		mxc_request_gpio(MX35_PIN_FEC_RX_CLK);
		mxc_request_gpio(MX35_PIN_FEC_COL);
		mxc_request_gpio(MX35_PIN_FEC_RX_DV);

		mxc_free_iomux(MX35_PIN_FEC_TX_CLK, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_FEC_RX_CLK, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_FEC_COL, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_FEC_RX_DV, MUX_CONFIG_GPIO);

		mxc_iomux_set_input(MUX_IN_UART3_UART_RTS_B, INPUT_CTL_PATH0);
		mxc_iomux_set_input(MUX_IN_UART3_UART_RXD_MUX, INPUT_CTL_PATH0);
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

void gpio_fec_active(void)
{
	mxc_request_iomux(MX35_PIN_FEC_TX_CLK, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_CLK, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_DV, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_COL, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TX_EN, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_MDC, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_MDIO, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TX_ERR, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_ERR, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_CRS, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA2, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA2, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA3, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA3, MUX_CONFIG_FUNC);

#define FEC_PAD_CTL_COMMON (PAD_CTL_DRV_3_3V|PAD_CTL_PUE_PUD| \
			PAD_CTL_ODE_CMOS|PAD_CTL_DRV_NORMAL|PAD_CTL_SRE_SLOW)
	mxc_iomux_set_pad(MX35_PIN_FEC_TX_CLK, FEC_PAD_CTL_COMMON |
			  PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
			  PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_CLK,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_DV,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_COL,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA0,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA0,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TX_EN,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_MDC,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_MDIO,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_22K_PU);
	mxc_iomux_set_pad(MX35_PIN_FEC_TX_ERR,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_ERR,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_CRS,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA1,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA1,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA2,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA2,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA3,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_SCHMITZ |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA3,
			  FEC_PAD_CTL_COMMON | PAD_CTL_HYS_CMOS |
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
#undef FEC_PAD_CTL_COMMON
}

EXPORT_SYMBOL(gpio_fec_active);

void gpio_fec_inactive(void)
{
	mxc_request_gpio(MX35_PIN_FEC_TX_CLK);
	mxc_request_gpio(MX35_PIN_FEC_RX_CLK);
	mxc_request_gpio(MX35_PIN_FEC_RX_DV);
	mxc_request_gpio(MX35_PIN_FEC_COL);
	mxc_request_gpio(MX35_PIN_FEC_RDATA0);
	mxc_request_gpio(MX35_PIN_FEC_TDATA0);
	mxc_request_gpio(MX35_PIN_FEC_TX_EN);
	mxc_request_gpio(MX35_PIN_FEC_MDC);
	mxc_request_gpio(MX35_PIN_FEC_MDIO);
	mxc_request_gpio(MX35_PIN_FEC_TX_ERR);
	mxc_request_gpio(MX35_PIN_FEC_RX_ERR);
	mxc_request_gpio(MX35_PIN_FEC_CRS);
	mxc_request_gpio(MX35_PIN_FEC_RDATA1);
	mxc_request_gpio(MX35_PIN_FEC_TDATA1);
	mxc_request_gpio(MX35_PIN_FEC_RDATA2);
	mxc_request_gpio(MX35_PIN_FEC_TDATA2);
	mxc_request_gpio(MX35_PIN_FEC_RDATA3);
	mxc_request_gpio(MX35_PIN_FEC_TDATA3);

	mxc_free_iomux(MX35_PIN_FEC_TX_CLK, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RX_CLK, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RX_DV, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_COL, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RDATA0, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TDATA0, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TX_EN, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_MDC, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_MDIO, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TX_ERR, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RX_ERR, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_CRS, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RDATA1, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TDATA1, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RDATA2, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TDATA2, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_RDATA3, MUX_CONFIG_GPIO);
	mxc_free_iomux(MX35_PIN_FEC_TDATA3, MUX_CONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_fec_inactive);

/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{

#define PAD_CONFIG (PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD | PAD_CTL_ODE_OpenDrain)

	switch (i2c_num) {
	case 0:
		mxc_request_iomux(MX35_PIN_I2C1_CLK, MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_I2C1_DAT, MUX_CONFIG_SION);

		mxc_iomux_set_pad(MX35_PIN_I2C1_CLK, PAD_CONFIG);
		mxc_iomux_set_pad(MX35_PIN_I2C1_DAT, PAD_CONFIG);
		break;
	case 1:
		mxc_request_iomux(MX35_PIN_I2C2_CLK, MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_I2C2_DAT, MUX_CONFIG_SION);

		mxc_iomux_set_pad(MX35_PIN_I2C2_CLK, PAD_CONFIG);
		mxc_iomux_set_pad(MX35_PIN_I2C2_DAT, PAD_CONFIG);

		break;
	case 2:
		mxc_request_iomux(MX35_PIN_TX3_RX2, MUX_CONFIG_ALT1);
		mxc_request_iomux(MX35_PIN_TX2_RX3, MUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX35_PIN_TX3_RX2, PAD_CONFIG);
		mxc_iomux_set_pad(MX35_PIN_TX2_RX3, PAD_CONFIG);
		break;
	default:
		break;
	}

#undef PAD_CONFIG

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
		break;
	case 1:
		break;
	case 2:
		mxc_request_iomux(MX35_PIN_TX3_RX2, MUX_CONFIG_GPIO);
		mxc_request_iomux(MX35_PIN_TX2_RX3, MUX_CONFIG_GPIO);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_i2c_inactive);

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
		mxc_request_iomux(MX35_PIN_CSPI1_MOSI, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CSPI1_MISO, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CSPI1_SS0, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CSPI1_SS1, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CSPI1_SCLK, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_CSPI1_SPI_RDY, MUX_CONFIG_FUNC);

		mxc_iomux_set_pad(MX35_PIN_CSPI1_MOSI,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PD | PAD_CTL_DRV_NORMAL);
		mxc_iomux_set_pad(MX35_PIN_CSPI1_MISO,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PD | PAD_CTL_DRV_NORMAL);
		mxc_iomux_set_pad(MX35_PIN_CSPI1_SS0,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PU | PAD_CTL_ODE_CMOS |
				  PAD_CTL_DRV_NORMAL);
		mxc_iomux_set_pad(MX35_PIN_CSPI1_SS1,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PU | PAD_CTL_ODE_CMOS |
				  PAD_CTL_DRV_NORMAL);
		mxc_iomux_set_pad(MX35_PIN_CSPI1_SCLK,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PD | PAD_CTL_DRV_NORMAL);
		mxc_iomux_set_pad(MX35_PIN_CSPI1_SPI_RDY,
				  PAD_CTL_DRV_3_3V | PAD_CTL_HYS_SCHMITZ |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
				  PAD_CTL_100K_PU | PAD_CTL_DRV_NORMAL);
		break;
	case 1:
		/* SPI2 */
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
		mxc_request_gpio(MX35_PIN_CSPI1_MOSI);
		mxc_request_gpio(MX35_PIN_CSPI1_MISO);
		mxc_request_gpio(MX35_PIN_CSPI1_SS0);
		mxc_request_gpio(MX35_PIN_CSPI1_SS1);
		mxc_request_gpio(MX35_PIN_CSPI1_SCLK);
		mxc_request_gpio(MX35_PIN_CSPI1_SPI_RDY);

		mxc_free_iomux(MX35_PIN_CSPI1_MOSI, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CSPI1_MISO, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CSPI1_SS0, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CSPI1_SS1, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CSPI1_SCLK, MUX_CONFIG_GPIO);
		mxc_free_iomux(MX35_PIN_CSPI1_SPI_RDY, MUX_CONFIG_GPIO);
		break;
	case 1:
		/* SPI2 */
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_spi_inactive);

/*!
 * Setup GPIO for LCD to be active
 */
void gpio_lcd_active(void)
{
	mxc_request_iomux(MX35_PIN_LD0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD2, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD3, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD4, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD5, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD6, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD7, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD8, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD9, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD10, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD11, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD12, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD13, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD14, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD15, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD16, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD17, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD18, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD19, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD20, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD21, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD22, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_LD23, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_D3_VSYNC, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_D3_HSYNC, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_D3_FPSHIFT, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_D3_DRDY, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_CONTRAST, MUX_CONFIG_FUNC);
}

EXPORT_SYMBOL(gpio_lcd_active);

/*!
 * Setup GPIO for LCD to be inactive
 */
void gpio_lcd_inactive(void)
{
}

EXPORT_SYMBOL(gpio_lcd_inactive);

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module)
{
	unsigned int pad_val;

	switch (module) {
	case 0:
		/* WLAN Module */
		mxc_request_iomux(MX35_PIN_SD1_CLK,
				  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD1_CMD,
				  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD1_DATA0,
				  MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD1_DATA1,
				  MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD1_DATA2,
				  MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD1_DATA3,
				  MUX_CONFIG_FUNC);

		pad_val = PAD_CTL_DRV_3_3V | PAD_CTL_HYS_CMOS |
			PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
			PAD_CTL_47K_PU | PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST;

		mxc_iomux_set_pad(MX35_PIN_SD1_CLK, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD1_CMD, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA0, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA1, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA2, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA3, pad_val);
		break;
	case 1:
		/* SD Card */
		mxc_request_iomux(MX35_PIN_SD2_CLK,
				  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD2_CMD,
				  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD2_DATA0, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD2_DATA1, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD2_DATA2, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD2_DATA3, MUX_CONFIG_FUNC);

		pad_val = PAD_CTL_DRV_MAX | PAD_CTL_PUE_PUD |
			PAD_CTL_PKE_ENABLE | PAD_CTL_22K_PU |
			PAD_CTL_SRE_FAST;

		mxc_iomux_set_pad(MX35_PIN_SD2_CLK, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_CMD, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA0, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA1, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA2, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA3, pad_val);

		/* Chip select and write protect */
		mxc_request_iomux(MX35_PIN_ATA_CS0, MUX_CONFIG_ALT5);
		mxc_iomux_set_input(MUX_IN_GPIO2_IN_6, INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX35_PIN_ATA_CS0, 0);
		mxc_set_gpio_direction(MX35_PIN_ATA_CS0, 1);

		mxc_request_iomux(MX35_PIN_ATA_DMARQ, MUX_CONFIG_ALT5);
		mxc_iomux_set_input(MUX_IN_GPIO2_IN_31, INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX35_PIN_ATA_DMARQ, 0);
		mxc_set_gpio_direction(MX35_PIN_ATA_DMARQ, 1);

		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
	switch (module) {
	case 0:
		/* WLAN Module */
		mxc_free_iomux(MX35_PIN_SD1_CLK,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD1_CMD,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD1_DATA0,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD1_DATA1,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD1_DATA2,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD1_DATA3,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);

		mxc_iomux_set_pad(MX35_PIN_SD1_CLK,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD1_CMD,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA0,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA1,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA2,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA3,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		break;
	case 1:
		/* SD Card */
		mxc_free_iomux(MX35_PIN_SD2_CLK,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD2_CMD,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD2_DATA0,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD2_DATA1,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD2_DATA2,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_free_iomux(MX35_PIN_SD2_DATA3,
			       MUX_CONFIG_FUNC | MUX_CONFIG_SION);

		mxc_iomux_set_pad(MX35_PIN_SD2_CLK,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD2_CMD,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA0,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA1,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA2,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA3,
				  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));

		/* Chip select and write protect */
		mxc_free_iomux(MX35_PIN_ATA_CS0, MUX_CONFIG_FUNC);
		mxc_free_iomux(MX35_PIN_ATA_DMARQ, MUX_CONFIG_FUNC);

		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
unsigned int sdhc_get_card_det_status(struct device *dev)
{
	if (to_platform_device(dev)->id == 0) {
		/* WLAN Module - always available */
		return 0;
	}
	else if (to_platform_device(dev)->id == 1) {
		/* SD Card */
		return mxc_get_gpio_datain(MX35_PIN_ATA_CS0);
	} else {
		return 0;
	}
}

EXPORT_SYMBOL(sdhc_get_card_det_status);

/*
 * Write protect for the card. If present the GPIO data would be set.
 */
unsigned int sdhc_get_card_wp_status(void)
{
	unsigned int ret;

	/* SD Card */
	ret = mxc_get_gpio_datain(MX35_PIN_ATA_DMARQ);
	return ret;
}

EXPORT_SYMBOL(sdhc_get_card_wp_status);

/*
 *  USB Host2
 */
int gpio_usbh2_active(void)
{
	mxc_request_iomux(MX35_PIN_I2C2_CLK, MUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX35_PIN_I2C2_CLK, 0x0040);

	mxc_request_iomux(MX35_PIN_I2C2_DAT, MUX_CONFIG_ALT2);
	mxc_iomux_set_input(MUX_IN_USB_UH2_USB_OC, INPUT_CTL_PATH1);
	mxc_iomux_set_pad(MX35_PIN_I2C2_DAT, 0x01c0);

	return 0;
}

EXPORT_SYMBOL(gpio_usbh2_active);

void gpio_usbh2_inactive(void)
{
	mxc_request_gpio(MX35_PIN_I2C2_DAT);
	mxc_free_iomux(MX35_PIN_I2C2_DAT, MUX_CONFIG_GPIO);
	mxc_request_gpio(MX35_PIN_I2C2_CLK);
	mxc_free_iomux(MX35_PIN_I2C2_CLK, MUX_CONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_usbh2_inactive);

/*
 *  USB OTG UTMI
 */
int gpio_usbotg_utmi_active(void)
{
	mxc_request_iomux(MX35_PIN_USBOTG_PWR, MUX_CONFIG_FUNC);
	mxc_iomux_set_pad(MX35_PIN_USBOTG_PWR, 0x0040);
	mxc_request_iomux(MX35_PIN_USBOTG_OC, MUX_CONFIG_FUNC);
	mxc_iomux_set_pad(MX35_PIN_USBOTG_OC, 0x01c0);

	return 0;
}

EXPORT_SYMBOL(gpio_usbotg_utmi_active);

void gpio_usbotg_utmi_inactive(void)
{
	mxc_request_gpio(MX35_PIN_USBOTG_PWR);
	mxc_free_iomux(MX35_PIN_USBOTG_PWR, MUX_CONFIG_GPIO);
	mxc_request_gpio(MX35_PIN_USBOTG_OC);
	mxc_free_iomux(MX35_PIN_USBOTG_OC, MUX_CONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_usbotg_utmi_inactive);

/*!
 * Setup GPIO for spdif tx/rx to be active
 */
void gpio_spdif_active(void)
{
	/* SPDIF OUT */
	mxc_free_iomux(MX35_PIN_CTS2, MUX_CONFIG_GPIO);

	mxc_request_iomux(MX35_PIN_CTS2, MUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX35_PIN_CTS2, PAD_CTL_PKE_NONE | PAD_CTL_PUE_PUD);
}

EXPORT_SYMBOL(gpio_spdif_active);

/*!
 * Setup GPIO for spdif tx/rx to be inactive
 */
void gpio_spdif_inactive(void)
{
	/* SPDIF OUT */
	mxc_free_iomux(MX35_PIN_CTS2, MUX_CONFIG_ALT1);

	/* turn off led */
	mxc_request_iomux(MX35_PIN_CTS2, MUX_CONFIG_ALT5);
	if (system_rev <= 2) {
		mxc_set_gpio_dataout(MX35_PIN_CTS2, 1);
	}
	else {
		mxc_set_gpio_dataout(MX35_PIN_CTS2, 0);
	}
	mxc_set_gpio_direction(MX35_PIN_CTS2, 0);
}

EXPORT_SYMBOL(gpio_spdif_inactive);

/*!
 * This function activates DAM ports 3 to enable
 * audio I/O.
 */
void gpio_activate_audio_ports(int ssi_port)
{
	unsigned int pad_val;
	u32 reg;

	if (ssi_port == 1) {
		/* Select osc_audio for SSI1 audio clock */
		reg = __raw_readl(MXC_CCM_ACMR);
		reg &= ~MXC_CCM_ACMR_SSI1_CLK_SEL_MASK;
		reg |= (0x01 << MXC_CCM_ACMR_SSI1_CLK_SEL_OFFSET);
		__raw_writel(reg, MXC_CCM_ACMR);

		mxc_request_iomux(MX35_PIN_STXD4, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SRXD4, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SCK4, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_STXFS4, MUX_CONFIG_FUNC);

		pad_val = PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD |
			PAD_CTL_PUE_PUD;

		mxc_iomux_set_pad(MX35_PIN_STXD4, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SRXD4, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SCK4, pad_val);
		mxc_iomux_set_pad(MX35_PIN_STXFS4, pad_val);
	}
	else {
		mxc_request_iomux(MX35_PIN_STXD5, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SRXD5, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SCK5, MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_STXFS5, MUX_CONFIG_FUNC);

		pad_val = PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD |
			PAD_CTL_PUE_PUD;
		mxc_iomux_set_pad(MX35_PIN_STXD5, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SRXD5, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SCK5, pad_val);
		mxc_iomux_set_pad(MX35_PIN_STXFS5, pad_val);
	}
}

EXPORT_SYMBOL(gpio_activate_audio_ports);
