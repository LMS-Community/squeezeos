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
 * @(#) balden@belcarra.com|otg/otg-config-std.h|20060419204305|26216
 *
 */



/*
 * config OTG_USB_PERIPHERAL
 * bool "USB Peripheral (only)"
 * Compile as a standard USB Peripheral.
 */
/* #define OTG_USB_PERIPHERAL */

/*
 * config OTG_USB_HOST
 * bool "USB Host (only)"
 * Compile as a standard USB Host.
 */
/* #define  OTG_USB_HOST */

/*
 * config OTG_USB_PERIPHERAL_OR_HOST
 * bool "USB Peripheral or Host"
 * Compile as a standard USB Peripheral and Host.
 * The transceiver driver must implement ID_GND to switch between
 * host and peripheral roles.
 */
/* #define OTG_USB_PERIPHERAL_OR_HOST */

/*
 * config OTG_BDEVICE_WITH_SRP
 * bool "SRP Capable B-Device (Only)"
 * Compile as a On-The-Go Peripheral-Only SRP capable device. This
 * is similiar to a Traditional USB Peripheral but enables
 * On-The-Go features such as SRP.
 */
/* #define OTG_BDEVICE_WITH_SRP */

/*
 * config OTG_DEVICE
 * bool "OTG Device - can act as A or B Device"
 * Implement full On-The-Go Device support for a platform that
 * supports implemenation of A and B Device.
 */
/* #define OTG_DEVICE */


/*
 * bool 'OTG Fast Tracing'
 * This option implements register trace to support
 * driver debugging.
 */
/* #define  CONFIG_OTG_TRACE */



/*
 * bool 'Built-in Minimal USB Device'
 * Compile in minimal USB Device only firmware.
 */
/* #define  CONFIG_OTG_FW_MN */

/*
 * bool 'Enable Auto-Start'
 * Automatically start and enable minimal USB Device.
 */
/* #define  CONFIG_OTG_TR_AUTO */

/*
 * bool 'Disable C99 initializers'
 * If your compiler does not allow a structure to be initialized as .element_name=value
 */
/* #define  CONFIG_OTG_NOC99 */

/*
 * boolean "USB Host - OTG Support"
 * The most notable feature of USB OTG is support for a
 * "Dual-Role" device, which can act as either a device
 * or a host.  The initial role choice can be changed
 * later, when two dual-role devices talk to each other.
 */
/* #define  CONFIG_USB_OTG */
