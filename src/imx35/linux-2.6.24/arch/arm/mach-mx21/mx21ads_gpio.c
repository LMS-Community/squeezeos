/*
 * Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/device.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>

#include "gpio_mux.h"
#include "crm_regs.h"

static int g_uart_actived[4] = { 0, 0, 0, 0 };

/*!
 * @file mx21ads_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */

/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	if (port < 0 || port >= MXC_UART_NR) {
		pr_info("Wrong GPIO port number: %d\n", port);
		BUG();
	}

	if (g_uart_actived[port]) {
		pr_info("UART %d GPIO have been actived multi-times\n",
			port + 1);
		return;
	}
	g_uart_actived[port] = 1;

	switch (port) {
	case 0:
		gpio_request_mux(MX21_PIN_UART1_TXD, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART1_RXD, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART1_CTS, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART1_RTS, GPIO_MUX_PRIMARY);
		break;
	case 1:
		gpio_request_mux(MX21_PIN_UART2_TXD, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART2_RXD, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART2_CTS, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_UART2_RTS, GPIO_MUX_PRIMARY);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
	if (port < 0 || port >= MXC_UART_NR) {
		pr_info("Wrong GPIO port number: %d\n", port);
		BUG();
	}

	if (g_uart_actived[port] == 0) {
		pr_info("UART %d GPIO have not been actived \n", port + 1);
		return;
	}
	g_uart_actived[port] = 0;

	switch (port) {
	case 0:
		gpio_free_mux(MX21_PIN_UART1_TXD);
		gpio_free_mux(MX21_PIN_UART1_RXD);
		gpio_free_mux(MX21_PIN_UART1_CTS);
		gpio_free_mux(MX21_PIN_UART1_RTS);
		break;
	case 1:
		gpio_free_mux(MX21_PIN_UART2_TXD);
		gpio_free_mux(MX21_PIN_UART2_RXD);
		gpio_free_mux(MX21_PIN_UART2_CTS);
		gpio_free_mux(MX21_PIN_UART2_RTS);
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
	return;
}

/************************************************************************/
/* for i2c gpio                                                         */
/* PD17,PD18 -- Primary 						*/
/************************************************************************/
#define GPIO_I2C (( 1<<17 ) |( 1<<18) )
/*!
* Setup GPIO for an I2C device to be active
*
* @param  i2c_num         an I2C device
*/
void gpio_i2c_active(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		gpio_request_mux(MX21_PIN_I2C_CLK, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_I2C_DATA, GPIO_MUX_PRIMARY);
		break;
	default:
		printk("gpio_i2c_active no compatible I2C adapter\n");
		break;
	}
}

/*!
 *  * Setup GPIO for an I2C device to be inactive
 *   *
 *    * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		gpio_free_mux(MX21_PIN_I2C_CLK);
		gpio_free_mux(MX21_PIN_I2C_DATA);
		break;
	default:
		break;
	}
}

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
		gpio_request_mux(MX21_PIN_CSPI1_RDY, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI1_SS2, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI1_SS1, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI1_SS0, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI1_MISO, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI1_MOSI, GPIO_MUX_PRIMARY);
		break;
	case 1:
		/* SPI2 */
		gpio_request_mux(MX21_PIN_CSPI2_SS2, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI2_SS1, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI2_SS0, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI2_MISO, GPIO_MUX_PRIMARY);
		gpio_request_mux(MX21_PIN_CSPI2_MOSI, GPIO_MUX_PRIMARY);
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
	switch (cspi_mod) {
	case 0:
		/* SPI1 */
		gpio_free_mux(MX21_PIN_CSPI1_RDY);
		gpio_free_mux(MX21_PIN_CSPI1_SS2);
		gpio_free_mux(MX21_PIN_CSPI1_SS1);
		gpio_free_mux(MX21_PIN_CSPI1_SS0);
		gpio_free_mux(MX21_PIN_CSPI1_MISO);
		gpio_free_mux(MX21_PIN_CSPI1_MOSI);
		break;
	case 1:
		/* SPI2 */
		gpio_free_mux(MX21_PIN_CSPI2_SS2);
		gpio_free_mux(MX21_PIN_CSPI2_SS1);
		gpio_free_mux(MX21_PIN_CSPI2_SS0);
		gpio_free_mux(MX21_PIN_CSPI2_MISO);
		gpio_free_mux(MX21_PIN_CSPI2_MOSI);
		break;

	default:
		break;
	}
}

/*!
 * Setup GPIO for a nand flash device to be active
 *
 */
void gpio_nand_active(void)
{
	gpio_request_mux(MX21_PIN_NFALE, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFCE, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFCLE, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFRB, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFRE, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFWE, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFWP, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO1, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO2, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO3, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO4, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO5, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO6, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_NFIO7, GPIO_MUX_PRIMARY);
}

/*!
 * Setup GPIO for a nand flash device to be inactive
 *
 */
void gpio_nand_inactive(void)
{
	gpio_free_mux(MX21_PIN_NFALE);
	gpio_free_mux(MX21_PIN_NFCE);
	gpio_free_mux(MX21_PIN_NFCLE);
	gpio_free_mux(MX21_PIN_NFRB);
	gpio_free_mux(MX21_PIN_NFRE);
	gpio_free_mux(MX21_PIN_NFWE);
	gpio_free_mux(MX21_PIN_NFWP);
	gpio_free_mux(MX21_PIN_NFIO1);
	gpio_free_mux(MX21_PIN_NFIO2);
	gpio_free_mux(MX21_PIN_NFIO3);
	gpio_free_mux(MX21_PIN_NFIO4);
	gpio_free_mux(MX21_PIN_NFIO5);
	gpio_free_mux(MX21_PIN_NFIO6);
	gpio_free_mux(MX21_PIN_NFIO7);
}

/*!
 * Setup GPIO for LCDC device to be active
 *
 */
void gpio_lcdc_active(void)
{
	gpio_request_mux(MX21_PIN_LSCLK, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD0, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD1, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD2, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD3, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD4, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD5, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD6, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD7, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD8, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD9, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD10, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD11, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD12, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD13, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD14, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD15, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD16, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_LD17, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_REV, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_CLS, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_PS, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_SPL_SPR, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_HSYNC, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_VSYNC, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_CONTRAST, GPIO_MUX_PRIMARY);
	gpio_request_mux(MX21_PIN_OE_ACD, GPIO_MUX_PRIMARY);
}

/*!
 * Setup GPIO for LCDC device to be inactive
 *
 */
void gpio_lcdc_inactive(void)
{
	gpio_free_mux(MX21_PIN_LSCLK);
	gpio_free_mux(MX21_PIN_LD0);
	gpio_free_mux(MX21_PIN_LD1);
	gpio_free_mux(MX21_PIN_LD2);
	gpio_free_mux(MX21_PIN_LD3);
	gpio_free_mux(MX21_PIN_LD4);
	gpio_free_mux(MX21_PIN_LD5);
	gpio_free_mux(MX21_PIN_LD6);
	gpio_free_mux(MX21_PIN_LD7);
	gpio_free_mux(MX21_PIN_LD8);
	gpio_free_mux(MX21_PIN_LD9);
	gpio_free_mux(MX21_PIN_LD10);
	gpio_free_mux(MX21_PIN_LD11);
	gpio_free_mux(MX21_PIN_LD12);
	gpio_free_mux(MX21_PIN_LD13);
	gpio_free_mux(MX21_PIN_LD14);
	gpio_free_mux(MX21_PIN_LD15);
	gpio_free_mux(MX21_PIN_LD16);
	gpio_free_mux(MX21_PIN_LD17);
	gpio_free_mux(MX21_PIN_REV);
	gpio_free_mux(MX21_PIN_CLS);
	gpio_free_mux(MX21_PIN_PS);
	gpio_free_mux(MX21_PIN_SPL_SPR);
	gpio_free_mux(MX21_PIN_HSYNC);
	gpio_free_mux(MX21_PIN_VSYNC);
	gpio_free_mux(MX21_PIN_CONTRAST);
	gpio_free_mux(MX21_PIN_OE_ACD);
}

/*!
 * GPIO settings not required for keypad
 *
 */
void gpio_keypad_active(void)
{
}

/*!
 * GPIO settings not required for keypad
 *
 */
void gpio_keypad_inactive(void)
{
}

void board_power_lcd(int on)
{
	volatile unsigned short reg;

	reg = __raw_readw(CS1_BASE_ADDR_VIRT + 0x800000);
	if (on == 0)
		/* turn it off */
		reg &= ~0x0200;
	else
		/* turn it on */
		reg |= 0x0200;

	__raw_writew(reg, CS1_BASE_ADDR_VIRT + 0x800000);
}

EXPORT_SYMBOL(gpio_uart_active);
EXPORT_SYMBOL(gpio_uart_inactive);
EXPORT_SYMBOL(config_uartdma_event);
EXPORT_SYMBOL(gpio_i2c_active);
EXPORT_SYMBOL(gpio_i2c_inactive);
EXPORT_SYMBOL(gpio_spi_active);
EXPORT_SYMBOL(gpio_spi_inactive);
EXPORT_SYMBOL(gpio_nand_active);
EXPORT_SYMBOL(gpio_nand_inactive);
EXPORT_SYMBOL(gpio_lcdc_active);
EXPORT_SYMBOL(gpio_lcdc_inactive);
EXPORT_SYMBOL(gpio_keypad_active);
EXPORT_SYMBOL(gpio_keypad_inactive);
EXPORT_SYMBOL(board_power_lcd);
