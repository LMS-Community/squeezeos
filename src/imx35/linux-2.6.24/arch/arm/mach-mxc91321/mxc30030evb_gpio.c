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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <linux/delay.h>

#include "board-mxc30030evb.h"
#include "iomux.h"

/*!
 * @file mach-mxc91321/mxc30030evb_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO_MXC91321
 */

/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	unsigned int pbc_bctrl1_set = 0;

	/*
	 * Configure the IOMUX control registers for the UART signals
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		if (no_irda == 1) {
			iomux_config_mux(PIN_UART_TXD1, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_RXD1, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_RTS1_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_CTS1_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			/*
			 * Enable the UART 1 Transceiver
			 */
			pbc_bctrl1_set |= PBC_BCTRL1_UENA;
		}
		break;
		/* UART 2 IOMUX Configs */
	case 1:
		if (no_irda == 1) {
			iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2);	/* TXD */
			iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2);	/* RXD */
			iomux_config_mux(PIN_USB_XRXD, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* RTS */
			iomux_config_mux(PIN_UART_CTS2_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_DSR2_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_DTR2_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_RI2_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_DCD2_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			/*
			 * Enable the UART 2 Transceiver
			 */
			pbc_bctrl1_set |= PBC_BCTRL1_UENCE;
		}
		break;
		/* UART 3 IOMUX Configs */
	case 2:
		if (no_irda == 1) {
			iomux_config_mux(PIN_UART_TXD3, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_RXD3, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_RTS3_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			iomux_config_mux(PIN_UART_CTS3_B, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			/*
			 * Enable the UART 3 Transceiver
			 */
			pbc_bctrl1_set |= PBC_BCTRL1_UENB;
		}
		break;
		/*
		 * UART 4 IOMUX Configs
		 * UART 4 is used for Irda
		 */
	case 3:
		if (no_irda == 0) {
			/*
			 * IOMUX configs for Irda pins
			 */
			iomux_config_mux(PIN_IRDA_TX4, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_NONE);
			iomux_config_mux(PIN_IRDA_RX4, INPUTCONFIG_NONE,
					 INPUTCONFIG_FUNC);

			iomux_config_mux(PIN_GPIO6, OUTPUTCONFIG_FUNC,
					 INPUTCONFIG_FUNC);
			gpio_config(0, 6, true, GPIO_INT_NONE);
			/* Clear the SD/Mode signal */
			gpio_set_data(0, 6, 0);

			/*
			 * Enable the Irda Transmitter
			 */
			pbc_bctrl1_set |= PBC_BCTRL1_IREN;
		}
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
	/*
	 * TODO: Configure the Pad registers for the UART pins
	 */
}

/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
	unsigned int pbc_bctrl1_clr = 0;

	/*
	 * Disable the UART Transceiver by configuring the GPIO pin
	 */
	switch (port) {
	case 0:
		/*
		 * Disable the UART 1 Transceiver
		 */
		pbc_bctrl1_clr |= PBC_BCTRL1_UENA;
		break;
	case 1:
		/*
		 * Disable the UART 2 Transceiver
		 */
		pbc_bctrl1_clr |= PBC_BCTRL1_UENCE;
		break;
	case 2:
		/*
		 * Disable the UART 3 Transceiver
		 */
		pbc_bctrl1_clr |= PBC_BCTRL1_UENB;
		break;
	case 3:
		/*
		 * Disable the Irda Transmitter
		 */
		pbc_bctrl1_clr |= PBC_BCTRL1_IREN;
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_clr, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
}

void gpio_power_key_active(void)
{
}
EXPORT_SYMBOL(gpio_power_key_active);

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
	switch (port) {
	case 3:
		/* Configure to receive UART 4 SDMA events */
		iomux_config_gpr(MUX_PGP_FIRI, false);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_uart_active);
EXPORT_SYMBOL(gpio_uart_inactive);
EXPORT_SYMBOL(config_uartdma_event);

/*!
 * Setup GPIO for a Keypad to be active
 */
void gpio_keypad_active(void)
{
	iomux_config_mux(PIN_KEY_COL0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_COL7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_KEY_ROW7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
}

EXPORT_SYMBOL(gpio_keypad_active);

/*!
 * Setup GPIO for a keypad to be inactive
 */
void gpio_keypad_inactive(void)
{
	/* The Keypad pins do not have/support GPIO mode */

}

EXPORT_SYMBOL(gpio_keypad_inactive);

/*!
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 */
void gpio_spi_active(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1_SS0 IOMux configuration */
		iomux_config_mux(PIN_CSPI1_CS_0, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI1 MISO IOMux configuration */
		iomux_config_mux(PIN_CSPI1_DI, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI1 MOSI IOMux configuration */
		iomux_config_mux(PIN_CSPI1_DO, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI1_SS1 IOMux configuration */
		iomux_config_mux(PIN_CSPI1_CS_1, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI1 CLK IOMux configuration */
		iomux_config_mux(PIN_CSPI1_CK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		break;
	case 1:
		/* SPI2_SS0 IOMux configuration */
		iomux_config_mux(PIN_CSPI2_CS_0, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI2 MISO IOMux configuration */
		iomux_config_mux(PIN_CSPI2_DI, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI2 MOSI IOMux configuration */
		iomux_config_mux(PIN_CSPI2_DO, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI2_SS1 IOMux configuration */
		iomux_config_mux(PIN_CSPI2_CS_1, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		/* SPI2 CLK IOMux configuration */
		iomux_config_mux(PIN_CSPI2_CK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
	/* The CSPI pins do not have/support GPIO mode */

}

/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		iomux_config_mux(PIN_I2C_CLK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_I2C_DAT, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
	/* The I2C pins do not have/support GPIO mode */

}

/*!
 * Setup GPIO for DVS PMIC Handshake in Low-level PM driver to be active
 *
 */
void gpio_pmhs_active(void)
{
	iomux_config_mux(PIN_GPIO9, OUTPUTCONFIG_ALT6, INPUTCONFIG_FUNC);
	iomux_config_mux(PIN_GPIO20, OUTPUTCONFIG_FUNC, INPUTCONFIG_ALT2);
}

EXPORT_SYMBOL(gpio_pmhs_active);

/*!
 * Setup GPIO for DVS PMIC Handshake in Low-level PM driver to be inactive
 *
 */
void gpio_pmhs_inactive(void)
{
	iomux_config_mux(PIN_GPIO20, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	/* The other pins do not have/support GPIO mode */

}

EXPORT_SYMBOL(gpio_pmhs_inactive);

/*!
 * This function configures and drives a gpio to tell PMIC to
 * power down.
 */
void gpio_pmic_power_off(void)
{
	local_irq_disable();

	/*
	 * Configure PIN_WDOG_B as an output, as a GPIO, and as LOW.
	 * This is on the cpu schematic as XH_WDOG_AP_B, which is
	 * connected to the PMIC WDI input pin.  When PMIC is in
	 * RUN mode, setting this line LOW will turn it off.
	 */

	/* Configure PIN_WDOG_B as an output */
	gpio_config(1, 23, true, GPIO_INT_NONE);

	/* Configure PIN_WDOG_B as a GPIO in the IOMUX */
	iomux_config_mux(PIN_WDOG_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);

	/* Configure PIN_WDOG_B as low */
	gpio_set_data(1, 23, 0);

	while (1) ;
}

/*!
 * This function configures the IOMux block for PMIC standard operations.
 *
 */
void gpio_pmic_active(void *irq_handler)
{
	iomux_config_mux(PIN_PM_INT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2);
}

EXPORT_SYMBOL(gpio_pmic_active);

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module)
{
	unsigned int pbc_bctrl1_set = 0;
	switch (module) {
	case 0:
		iomux_config_mux(PIN_MMC1_CLK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC1_CMD, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC1_DATA_0, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC1_DATA_1, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC1_DATA_2, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC1_DATA_3, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		pbc_bctrl1_set |= PBC_BCTRL1_MCP1;
		break;
	case 1:
		iomux_config_mux(PIN_MMC2_CLK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC2_CMD, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC2_DATA_0, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC2_DATA_1, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC2_DATA_2, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_mux(PIN_MMC2_DATA_3, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		pbc_bctrl1_set |= PBC_BCTRL1_MCP2;
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
	unsigned int pbc_bctrl1_set = 0;
	switch (module) {
	case 0:
		pbc_bctrl1_set &= PBC_BCTRL1_MCP1;
		break;
	case 1:
		pbc_bctrl1_set &= PBC_BCTRL1_MCP2;
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
unsigned int sdhc_get_card_det_status(struct device *dev)
{
	if (to_platform_device(dev)->id == 0) {
		return mxc_get_gpio_datain(PIN_GPIO37);
	} else {
		return 0;
	}
}

EXPORT_SYMBOL(sdhc_get_card_det_status);

/*
 * Return the card detect pin.
 */
int sdhc_init_card_det(int id)
{
	if (id == 0) {
		iomux_config_mux(PIN_GPIO37, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_FUNC);
		iomux_config_pad(PIN_GPIO37, PAD_CTL_PKE_NONE);
		return IOMUX_TO_IRQ(PIN_GPIO37);
	} else {
		return 0;	//SD2 not supported
	}
}

EXPORT_SYMBOL(sdhc_init_card_det);

/*!
 * Configure the IOMUX GPR register to receive shared SDMA FIRI events
 *
 */
void config_firidma_event(void)
{
	/* Configure to receive UART 4 SDMA events */
	iomux_config_gpr(MUX_PGP_FIRI, true);
}

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_lcd_active(void)
{
	u16 temp;

	temp = PBC_BCTRL1_LCDON;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
	u16 pbc_bctrl1_set = 0;

	pbc_bctrl1_set = (u16) PBC_BCTRL1_LCDON;
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET + 2);
}

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_sensor_active(void)
{
#ifdef CONFIG_MXC_CAMERA_MC521DA
	__raw_writew(0x100, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
#else
	__raw_writew(PBC_BCTRL1_SENSORON, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
#endif
}

EXPORT_SYMBOL(gpio_sensor_active);

void gpio_sensor_reset(bool flag)
{
	if (flag) {
		__raw_writew(0x200, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
	} else {
		__raw_writew(0x200, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
	}
}

EXPORT_SYMBOL(gpio_sensor_reset);

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_sensor_inactive(void)
{
#ifdef CONFIG_MXC_CAMERA_MC521DA
	__raw_writew(0x100, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
#else
	__raw_writew(PBC_BCTRL1_SENSORON, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
#endif
}

EXPORT_SYMBOL(gpio_sensor_inactive);

void slcd_gpio_config(void)
{
	/* PIN_GPIO31 is actually bit 21 in in GPIO module!!! */
	gpio_set_data(0, 21, 0);
	gpio_config(0, 21, true, GPIO_INT_NONE);
	iomux_config_mux(PIN_GPIO31, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	msleep(1);

	gpio_set_data(0, 21, 1);
	msleep(1);
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_active(void)
{
	/*
	 * Configure the IOMUX control register for 1-wire signals.
	 */
	iomux_config_pad(PIN_BATT_LINE, PAD_CTL_LOOPBACK);
	iomux_config_mux(PIN_BATT_LINE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_inactive(void)
{
	/*
	 * Configure the IOMUX control register for 1-wire signals.
	 */
	iomux_config_mux(PIN_BATT_LINE, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_owire_active);
EXPORT_SYMBOL(gpio_owire_inactive);

/*!
 * Setup IR to be used by UART and FIRI
 */
void gpio_firi_init(void)
{
	gpio_uart_active(3, 0);
	iomux_config_gpr(MUX_PGP_FIRI, true);
}

EXPORT_SYMBOL(gpio_firi_init);

/*!
 * Setup IR to be used by UART
 */
void gpio_firi_inactive(void)
{
	iomux_config_mux(PIN_IRDA_TX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE);
	iomux_config_mux(PIN_IRDA_RX4, INPUTCONFIG_NONE, INPUTCONFIG_FUNC);

	/* Set the SD/Mode signal */
	gpio_set_data(0, 6, 1);

	/* Clear the SD/Mode signal */
	gpio_set_data(0, 6, 0);
}

EXPORT_SYMBOL(gpio_firi_inactive);

/*!
 * Setup IR to be used by FIRI
 */
void gpio_firi_active(void *fir_cong_reg_base, unsigned int tpp_mask)
{
	unsigned int cr;

	iomux_config_mux(PIN_IRDA_TX4, OUTPUTCONFIG_ALT2, INPUTCONFIG_NONE);
	iomux_config_mux(PIN_IRDA_RX4, INPUTCONFIG_NONE, INPUTCONFIG_ALT2);

	cr = readl(fir_cong_reg_base);
	cr |= tpp_mask;
	writel(cr, fir_cong_reg_base);

	/* Clear the SD/Mode signal */
	gpio_set_data(0, 6, 0);

	/* Set the SD/Mode signal */
	gpio_set_data(0, 6, 1);

	cr = readl(fir_cong_reg_base);
	cr &= ~tpp_mask;
	writel(cr, fir_cong_reg_base);

	/* Clear the SD/Mode signal */
	gpio_set_data(0, 6, 0);

	cr = readl(fir_cong_reg_base);
	cr |= tpp_mask;
	writel(cr, fir_cong_reg_base);
}

EXPORT_SYMBOL(gpio_firi_active);
