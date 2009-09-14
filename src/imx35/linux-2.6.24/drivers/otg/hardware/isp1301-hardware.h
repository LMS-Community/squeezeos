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
/*
 * otg/hardware/isp1301-hardware.h -- ISP1301 hardware specific defines
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/isp1301/isp1301-hardware.h|20061123215517|03742
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/isp1301-hardware.h
 * @brief Public Structures And Defines For ISP1301 Hardware.
 *
 * The Philips ISP1301 is an OTG Transceiver. There a additional
 * compatible parts such as the Maxim MAX3301E
 *
 * @ingroup ISP1301TCD
 */


/*!
 * @name ADR/PSW - C.f. 8.9.1
 *
 * The i2c address is either 0x2c or 0x2d depending on the level of the
 * ADR/PSW pin after device reset. Note that this pin can be programmed as
 * an output via the Mode Control 2 register, bit PSW_OE to enable an
 * @{
 */

#define ISP1301_I2C_ADDR_LOW                    0x2c
#define ISP1301_I2C_ADDR_HIGH                   0x2d
/*! @} */

/*!
 * @name ISP1301 Registers C.f. 11.1
 * @{
 */

#define ISP1301_VENDOR_ID                       0x00
#define ISP1301_PRODUCT_ID                      0x02
#define ISP1301_VERSION_ID                      0x14

#define ISP1301_VENDOR_ID_LOW                   0x00
#define ISP1301_PRODUCT_ID_LOW                  0x02
#define ISP1301_VERSION_ID_LOW                  0x14
#define ISP1301_VENDOR_ID_HIGH                  0x01
#define ISP1301_PRODUCT_ID_HIGH                 0x03
#define ISP1301_VERSION_ID_HIGH                 0x15

/* these are all single byte registers
 */
#define ISP1301_MODE_CONTROL_1                  0x04
#define ISP1301_MODE_CONTROL_1_SET              0x04
#define ISP1301_MODE_CONTROL_1_CLR              0x05

#define ISP1301_MODE_CONTROL_2                  0x12
#define ISP1301_MODE_CONTROL_2_SET              0x12
#define ISP1301_MODE_CONTROL_2_CLR              0x13

#define ISP1301_OTG_CONTROL                     0x06
#define ISP1301_OTG_CONTROL_SET                 0x06
#define ISP1301_OTG_CONTROL_CLR                 0x07

#define ISP1301_OTG_STATUS                      0x10

#define ISP1301_INTERRUPT_SOURCE                0x08

#define ISP1301_INTERRUPT_LATCH                 0x0a
#define ISP1301_INTERRUPT_LATCH_SET             0x0a
#define ISP1301_INTERRUPT_LATCH_CLR             0x0b

#define ISP1301_INTERRUPT_ENABLE_LOW            0x0c
#define ISP1301_INTERRUPT_ENABLE_LOW_SET        0x0c
#define ISP1301_INTERRUPT_ENABLE_LOW_CLR        0x0d

#define ISP1301_INTERRUPT_ENABLE_HIGH           0x0e
#define ISP1301_INTERRUPT_ENABLE_HIGH_SET       0x0e
#define ISP1301_INTERRUPT_ENABLE_HIGH_CLR       0x0f
/*! @} */


/*!
 * @name Mode Control 1 register - C.f. Table 17 and Table 18
 * @{
 */
#define ISP1301_UART_EN                         (1 << 6)
#define ISP1301_OE_INT_EN                       (1 << 5)
#define ISP1301_BDIS_ACON_EN                    (1 << 4)
#define ISP1301_TRANSP_EN                       (1 << 3)
#define ISP1301_DAT_SE0                         (1 << 2)
#define ISP1301_SUSPEND_REG                     (1 << 1)
#define ISP1301_SPEED_REG                       (1 << 0)
/*! @} */


/*!
 * @name Mode Control 2 register - C.f. Table 19 and Table 20
 * @{
 */
#define ISP1301_EN2V7                           (1 << 7)
#define ISP1301_PSW_OE                          (1 << 6)
#define ISP1301_AUDIO_EN                        (1 << 5)
#define ISP1301_TRANSP_BDIR                     (3 << 3)
#define ISP1301_BI_DI                           (1 << 2)
#define ISP1301_SPD_SUSP_CTRL                   (1 << 1)
#define ISP1301_GLOBAL_PWR_ON                   (1 << 0)
/*! @} */


/*!
 * @name OTG Control register - C.f. Table 21 and Table 22
 * @{
 */
#define ISP1301_VBUS_CHRG                       (1 << 7)
#define ISP1301_VBUS_DISCHRG                    (1 << 6)
#define ISP1301_VBUS_DRV                        (1 << 5)
#define ISP1301_ID_PULLDOWN                     (1 << 4)
#define ISP1301_DM_PULLDOWN                     (1 << 3)
#define ISP1301_DP_PULLDOWN                     (1 << 2)
#define ISP1301_DM_PULLUP                       (1 << 1)
#define ISP1301_DP_PULLUP                       (1 << 0)

#define ISP1301_VBUS_RESET (ISP1301_VBUS_CHRG | ISP1301_VBUS_DISCHRG | ISP1301_VBUS_DRV)

/*! @} */


/*!
 * @name OTG Status register - C.f. Table 23 and Table 24
 * @{
 */
#define ISP1301_B_SESS_VLD                      (1 << 7)
#define ISP1301_B_SESS_END                      (1 << 6)
/*! @} */


/*!
 * @name Interrupt Source Register
 * Interrupt Source register - C.f. Table 25 and Table 26
 * Interrupt Latch register - C.f. Table 27 and Table 28
 * Interrupt Enable Low register - C.f. Table 29 and Table 30
 * Interrupt Enable High register - C.f. Table 31 and Table 32
 * @{
 */
#define ISP1301_CR_INT                          (1 << 7)
#define ISP1301_BDIS_ACON                       (1 << 6)
#define ISP1301_ID_FLOAT                        (1 << 5)
#define ISP1301_DM_HI                           (1 << 4)
#define ISP1301_ID_GND                          (1 << 3)
#define ISP1301_DP_HI                           (1 << 2)
#define ISP1301_SESS_VLD                        (1 << 1)
#define ISP1301_VBUS_VLD                        (1 << 0)

//#define ISP1301_SE1                             (ISP1301_DM_HI | ISP1301_DP_HI)

/*! @} */

/*!
 * @name Maxim MAX3301E
 *
 * This part is compatible with the ISP1301 with minor differences:
 *
 * Mode Control 1 register is called Control Register 1. It is almost
 * identical except for:
 *
 *      bit     isp1301         max3301e
 *      3       transp_en       not used
 * @{
 */
#define MAX3301E_CONTROL_REGISTER_1             0x04
#define MAX3301E_CONTROL_REGISTER_1_SET         0x04
#define MAX3301E_CONTROL_REGISTER_1_CLR         0x05
/*! @} */

/*!
 * @name Mode Control 2
 * Mode Control 2 is called Special Function Register 1. Mostly the same
 * except for:
 *
 *      bit     isp1301         max3301e
 *      7       en2v7           gp_en
 *      6       psw_oe          sess_end
 *      5       audio_en        int_source
 * @{
 */
#define MAX3301E_SPECIAL_FUNCTION_1             0x12
#define MAX3301E_SPECIAL_FUNCTION_1_SET         0x12
#define MAX3301E_SPECIAL_FUNCTION_1_CLR         0x13
#define MAX3301E_GP_EN                          (1 << 7)
#define MAX3301E_SESS_END                       (1 << 6)
#define MAX3301E_INT_SOURCE                     (1 << 5)
/*! @} */

/*!
 * @name OTG Status
 * The OTG Status is not present, but B_SESS_END seems to be available
 * in the Mode Control 2 register bit 6.
 *
 * An additional register is available for MAX3301E specific features.
 *
 * Special Function Register 2
 * @{
 */

#define MAX3301E_SPECIAL_FUNCTION_2             0x16
#define MAX3301E_SPECIAL_FUNCTION_2_SET         0x16
#define MAX3301E_SPECIAL_FUNCTION_2_CLR         0x17
#define MAX3301E_SDWN                           (1 << 0)
#define MAX3301E_IRQ_MODE                       (1 << 1)
#define MAX3301E_XCVR_INPUT_DISC                (1 << 2)
#define MAX3301E_REQ_SET                        (1 << 3)
/*! @} */
