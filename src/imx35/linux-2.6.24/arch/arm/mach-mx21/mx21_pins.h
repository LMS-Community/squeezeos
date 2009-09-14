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
#ifndef __ASM_ARCH_MXC_MX21_PINS_H__
#define __ASM_ARCH_MXC_MX21_PINS_H__

#ifndef __ASSEMBLY__

/*!
 * @name IOMUX/PAD Bit field definitions
 */

/*! @{ */

/*!
 * In order to identify pins more effectively, each mux-controlled pin's
 * enumerated value is constructed in the following way:
 *
 * -------------------------------------------------------------------
 * 31-29 | 28 - 24 |23 - 21| 20  | 19 - 18 | 17 - 10| 9 - 8 | 7 - 0
 * -------------------------------------------------------------------
 * IO_P  |  IO_I   |     RSVD    |  PAD_F  |  PAD_I | MUX_F | MUX_I
 * -------------------------------------------------------------------
 *
 * Bit 0 to 7 contains MUX_I used to identify the register
 * offset (0-based. base is IOMUX_module_base + 0xC) defined in the Section
 * "sw_pad_ctl & sw_mux_ctl details" of the IC Spec. Bit 8 to 9 is MUX_F which
 * contains the offset value defined WITHIN the same register (each IOMUX
 * control register contains four 8-bit fields for four different pins). The
 * similar field definitions are used for the pad control register.
 * For example, the MX21_PIN_A0 is defined in the enumeration:
 *    ( 73 << MUX_I) | (0 << MUX_F)|( 98 << PAD_I) | (0 << PAD_F)
 * It means the mux control register is at register offset 73. So the absolute
 * address is: 0xC+73*4=0x130   0 << MUX_F means the control bits are at the
 * least significant bits within the register. The pad control register offset
 * is: 0x154+98*4=0x2DC and also occupy the least significant bits within the
 * register.
 */

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * MUX control register index (0-based)
 */
#define MUX_I		0

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * field within IOMUX control register for control bits
 * (legal values are 0, 1, 2, 3)
 */
#define MUX_F		8

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * PAD control register index (0-based)
 */
#define PAD_I		10

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * field within PAD control register for control bits
 * (legal values are 0, 1, 2)
 */
#define PAD_F		18

#define _MX21_BUILD_PIN(gp,gi) (((gp) << MUX_IO_P) | ((gi) << MUX_IO_I))

/*! @} End IOMUX/PAD Bit field definitions */

/*!
 * This enumeration is constructed based on the Section
 * "sw_pad_ctl & sw_mux_ctl details" of the MX31 IC Spec. Each enumerated
 * value is constructed based on the rules described above.
 */
enum iomux_pins {
	MX21_PIN_LSCLK = _MX21_BUILD_PIN(0, 5),
	MX21_PIN_LD0 = _MX21_BUILD_PIN(0, 6),
	MX21_PIN_LD1 = _MX21_BUILD_PIN(0, 7),
	MX21_PIN_LD2 = _MX21_BUILD_PIN(0, 8),
	MX21_PIN_LD3 = _MX21_BUILD_PIN(0, 9),
	MX21_PIN_LD4 = _MX21_BUILD_PIN(0, 10),
	MX21_PIN_LD5 = _MX21_BUILD_PIN(0, 11),
	MX21_PIN_LD6 = _MX21_BUILD_PIN(0, 12),
	MX21_PIN_LD7 = _MX21_BUILD_PIN(0, 13),
	MX21_PIN_LD8 = _MX21_BUILD_PIN(0, 14),
	MX21_PIN_LD9 = _MX21_BUILD_PIN(0, 15),
	MX21_PIN_LD10 = _MX21_BUILD_PIN(0, 16),
	MX21_PIN_LD11 = _MX21_BUILD_PIN(0, 17),
	MX21_PIN_LD12 = _MX21_BUILD_PIN(0, 18),
	MX21_PIN_LD13 = _MX21_BUILD_PIN(0, 19),
	MX21_PIN_LD14 = _MX21_BUILD_PIN(0, 20),
	MX21_PIN_LD15 = _MX21_BUILD_PIN(0, 21),
	MX21_PIN_LD16 = _MX21_BUILD_PIN(0, 22),
	MX21_PIN_LD17 = _MX21_BUILD_PIN(0, 23),
	MX21_PIN_REV = _MX21_BUILD_PIN(0, 24),
	MX21_PIN_CLS = _MX21_BUILD_PIN(0, 25),
	MX21_PIN_PS = _MX21_BUILD_PIN(0, 26),
	MX21_PIN_SPL_SPR = _MX21_BUILD_PIN(0, 27),
	MX21_PIN_HSYNC = _MX21_BUILD_PIN(0, 28),
	MX21_PIN_VSYNC = _MX21_BUILD_PIN(0, 29),
	MX21_PIN_CONTRAST = _MX21_BUILD_PIN(0, 30),
	MX21_PIN_OE_ACD = _MX21_BUILD_PIN(0, 31),

	MX21_PIN_SD2_D0 = _MX21_BUILD_PIN(1, 4),
	MX21_PIN_SD2_D1 = _MX21_BUILD_PIN(1, 5),
	MX21_PIN_SD2_D2 = _MX21_BUILD_PIN(1, 6),
	MX21_PIN_SD2_D3 = _MX21_BUILD_PIN(1, 7),
	MX21_PIN_SD2_CMD = _MX21_BUILD_PIN(1, 8),
	MX21_PIN_SD2_CLK = _MX21_BUILD_PIN(1, 9),
	MX21_PIN_CSI_D0 = _MX21_BUILD_PIN(1, 10),
	MX21_PIN_CSI_D1 = _MX21_BUILD_PIN(1, 11),
	MX21_PIN_CSI_D2 = _MX21_BUILD_PIN(1, 12),
	MX21_PIN_CSI_D3 = _MX21_BUILD_PIN(1, 13),
	MX21_PIN_CSI_D4 = _MX21_BUILD_PIN(1, 14),
	MX21_PIN_CSI_MCLK = _MX21_BUILD_PIN(1, 15),
	MX21_PIN_CSI_PIXCLK = _MX21_BUILD_PIN(1, 16),
	MX21_PIN_CSI_D5 = _MX21_BUILD_PIN(1, 17),
	MX21_PIN_CSI_D6 = _MX21_BUILD_PIN(1, 18),
	MX21_PIN_CSI_D7 = _MX21_BUILD_PIN(1, 19),
	MX21_PIN_CSI_VSYNC = _MX21_BUILD_PIN(1, 20),
	MX21_PIN_CSI_HSYNC = _MX21_BUILD_PIN(1, 21),
	MX21_PIN_USB_BYP = _MX21_BUILD_PIN(1, 22),
	MX21_PIN_USB_PWR = _MX21_BUILD_PIN(1, 23),
	MX21_PIN_USB_OC = _MX21_BUILD_PIN(1, 24),
	MX21_PIN_USBH_ON = _MX21_BUILD_PIN(1, 25),
	MX21_PIN_USBH1_FS = _MX21_BUILD_PIN(1, 26),
	MX21_PIN_USBH1_OE = _MX21_BUILD_PIN(1, 27),
	MX21_PIN_USBH1_TXDM = _MX21_BUILD_PIN(1, 28),
	MX21_PIN_USBH1_TXDP = _MX21_BUILD_PIN(1, 29),
	MX21_PIN_USBH1_RXDM = _MX21_BUILD_PIN(1, 30),
	MX21_PIN_USBH1_RXDP = _MX21_BUILD_PIN(1, 31),

	MX21_PIN_USBG_SDA = _MX21_BUILD_PIN(2, 5),
	MX21_PIN_USBG_SCL = _MX21_BUILD_PIN(2, 5),
	MX21_PIN_USBG_ON = _MX21_BUILD_PIN(2, 7),
	MX21_PIN_USBG_FS = _MX21_BUILD_PIN(2, 8),
	MX21_PIN_USBG_OE = _MX21_BUILD_PIN(2, 9),
	MX21_PIN_USBG_TXDM = _MX21_BUILD_PIN(2, 10),
	MX21_PIN_USBG_TXDP = _MX21_BUILD_PIN(2, 11),
	MX21_PIN_USBG_RXDM = _MX21_BUILD_PIN(2, 12),
	MX21_PIN_USBG_RXDP = _MX21_BUILD_PIN(2, 13),
	MX21_PIN_TOUT = _MX21_BUILD_PIN(2, 14),
	MX21_PIN_TIN = _MX21_BUILD_PIN(2, 15),
	MX21_PIN_SAP_FS = _MX21_BUILD_PIN(2, 16),
	MX21_PIN_SAP_RXD = _MX21_BUILD_PIN(2, 17),
	MX21_PIN_SAP_TXD = _MX21_BUILD_PIN(2, 18),
	MX21_PIN_SAP_CLK = _MX21_BUILD_PIN(2, 19),
	MX21_PIN_SSI1_FS = _MX21_BUILD_PIN(2, 20),
	MX21_PIN_SSI1_RXD = _MX21_BUILD_PIN(2, 21),
	MX21_PIN_SSI1_TXD = _MX21_BUILD_PIN(2, 22),
	MX21_PIN_SSI1_CLK = _MX21_BUILD_PIN(2, 23),
	MX21_PIN_SSI2_FS = _MX21_BUILD_PIN(2, 24),
	MX21_PIN_SSI2_RXD = _MX21_BUILD_PIN(2, 25),
	MX21_PIN_SSI2_TXD = _MX21_BUILD_PIN(2, 26),
	MX21_PIN_SSI2_CLK = _MX21_BUILD_PIN(2, 27),
	MX21_PIN_SSI3_FS = _MX21_BUILD_PIN(2, 28),
	MX21_PIN_SSI3_RXD = _MX21_BUILD_PIN(2, 29),
	MX21_PIN_SSI3_TXD = _MX21_BUILD_PIN(2, 30),
	MX21_PIN_SSI3_CLK = _MX21_BUILD_PIN(2, 31),

	MX21_PIN_I2C_DATA = _MX21_BUILD_PIN(3, 17),
	MX21_PIN_I2C_CLK = _MX21_BUILD_PIN(3, 18),
	MX21_PIN_CSPI2_SS2 = _MX21_BUILD_PIN(3, 19),
	MX21_PIN_CSPI2_SS1 = _MX21_BUILD_PIN(3, 20),
	MX21_PIN_CSPI2_SS0 = _MX21_BUILD_PIN(3, 21),
	MX21_PIN_CSPI2_SCLK = _MX21_BUILD_PIN(3, 22),
	MX21_PIN_CSPI2_MISO = _MX21_BUILD_PIN(3, 23),
	MX21_PIN_CSPI2_MOSI = _MX21_BUILD_PIN(3, 24),
	MX21_PIN_CSPI1_RDY = _MX21_BUILD_PIN(3, 25),
	MX21_PIN_CSPI1_SS2 = _MX21_BUILD_PIN(3, 26),
	MX21_PIN_CSPI1_SS1 = _MX21_BUILD_PIN(3, 27),
	MX21_PIN_CSPI1_SS0 = _MX21_BUILD_PIN(3, 28),
	MX21_PIN_CSPI1_SCLK = _MX21_BUILD_PIN(3, 29),
	MX21_PIN_CSPI1_MISO = _MX21_BUILD_PIN(3, 30),
	MX21_PIN_CSPI1_MOSI = _MX21_BUILD_PIN(3, 31),

	MX21_PIN_WB2 = _MX21_BUILD_PIN(4, 0),
	MX21_PIN_WB1 = _MX21_BUILD_PIN(4, 1),
	MX21_PIN_WB0 = _MX21_BUILD_PIN(4, 2),
	MX21_PIN_UART2_CTS = _MX21_BUILD_PIN(4, 3),
	MX21_PIN_UART2_RTS = _MX21_BUILD_PIN(4, 4),
	MX21_PIN_PWMO = _MX21_BUILD_PIN(4, 5),
	MX21_PIN_UART2_TXD = _MX21_BUILD_PIN(4, 6),
	MX21_PIN_UART2_RXD = _MX21_BUILD_PIN(4, 7),
	MX21_PIN_UART3_TXD = _MX21_BUILD_PIN(4, 8),
	MX21_PIN_UART3_RXD = _MX21_BUILD_PIN(4, 9),
	MX21_PIN_UART3_CTS = _MX21_BUILD_PIN(4, 10),
	MX21_PIN_UART3_RTS = _MX21_BUILD_PIN(4, 11),
	MX21_PIN_UART1_TXD = _MX21_BUILD_PIN(4, 12),
	MX21_PIN_UART1_RXD = _MX21_BUILD_PIN(4, 13),
	MX21_PIN_UART1_CTS = _MX21_BUILD_PIN(4, 14),
	MX21_PIN_UART1_RTS = _MX21_BUILD_PIN(4, 15),
	MX21_PIN_RTCK = _MX21_BUILD_PIN(4, 16),
	MX21_PIN_RESET_OUT = _MX21_BUILD_PIN(4, 17),
	MX21_PIN_SD1_D0 = _MX21_BUILD_PIN(4, 18),
	MX21_PIN_SD1_D1 = _MX21_BUILD_PIN(4, 19),
	MX21_PIN_SD1_D2 = _MX21_BUILD_PIN(4, 20),
	MX21_PIN_SD1_D3 = _MX21_BUILD_PIN(4, 21),
	MX21_PIN_SD1_CMD = _MX21_BUILD_PIN(4, 22),
	MX21_PIN_SD1_CLK = _MX21_BUILD_PIN(4, 23),

	MX21_PIN_NFRB = _MX21_BUILD_PIN(5, 0),
	MX21_PIN_NFCE = _MX21_BUILD_PIN(5, 1),
	MX21_PIN_NFWP = _MX21_BUILD_PIN(5, 2),
	MX21_PIN_NFCLE = _MX21_BUILD_PIN(5, 3),
	MX21_PIN_NFALE = _MX21_BUILD_PIN(5, 4),
	MX21_PIN_NFRE = _MX21_BUILD_PIN(5, 5),
	MX21_PIN_NFWE = _MX21_BUILD_PIN(5, 6),
	MX21_PIN_NFIO0 = _MX21_BUILD_PIN(5, 7),
	MX21_PIN_NFIO1 = _MX21_BUILD_PIN(5, 8),
	MX21_PIN_NFIO2 = _MX21_BUILD_PIN(5, 9),
	MX21_PIN_NFIO3 = _MX21_BUILD_PIN(5, 10),
	MX21_PIN_NFIO4 = _MX21_BUILD_PIN(5, 11),
	MX21_PIN_NFIO5 = _MX21_BUILD_PIN(5, 12),
	MX21_PIN_NFIO6 = _MX21_BUILD_PIN(5, 13),
	MX21_PIN_NFIO7 = _MX21_BUILD_PIN(5, 14),
	MX21_PIN_CLKO = _MX21_BUILD_PIN(5, 15),
	MX21_PIN_PF16 = _MX21_BUILD_PIN(5, 16),
	MX21_PIN_CS4 = _MX21_BUILD_PIN(5, 21),
	MX21_PIN_CS5 = _MX21_BUILD_PIN(5, 22),
};

#endif
#endif
