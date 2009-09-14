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
 *
 * @(#) balden@belcarra.com|otg/functions/generic/otg-config.h|20060419204257|50674
 *
 */

/*
 * tristate "  Generic Function"
 * This acts as a generic function, allowing selection of class
 * and interface function drivers to create a specific configuration.
 */
#define CONFIG_OTG_GENERIC

/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 */
#define CONFIG_OTG_GENERIC_VENDORID

/*
 * hex "ProductID (hex value)"
 * default "0xf010"
 */
#define CONFIG_OTG_GENERIC_PRODUCTID

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_GENERIC_BCDDEVICE

/*
 * string "iManufacturer (string)"
 * default "Belcarra"
 */
#define CONFIG_OTG_GENERIC_MANUFACTURER

/*
 * string "iProduct (string)"
 * default "Generic Composite"
 */
#define CONFIG_OTG_GENERIC_PRODUCT_NAME

/*
 * bool 'none'
 * No pre-defined selection, use the OTG_GENERIC_CONFIG_NAME instead.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NONE

/*
 * bool 'mouse'
 * Configure the mouse driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MOUSE

/*
 * bool 'net-blan'
 * Configure the network driver in a single function non-composite BLAN configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_BLAN

/*
 * bool 'net-cdc'
 * Configure the network driver in a single function non-composite CDC configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_CDC

/*
 * bool 'net-safe'
 * Configure the network driver in a single function non-composite SAFE configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_SAFE

/*
 * bool 'acm-tty'
 * Configure the acm driver in a single function non-composite TTY configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_ACM_TTY

/*
 * bool 'msc'
 * Configure the msc driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC

/*
 * bool 'mouse2'
 * Configure the mouse driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MOUSE2

/*
 * bool 'msc-mouse'
 * Configure the msc and mouse driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_MOUSE

/*
 * bool 'msc-blan'
 * Configure the msc and network-blan driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_BLAN

/*
 * bool 'msc-cdc'
 * Configure the msc and network-cdc driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_CDC




/*
 * string "Composite Configuration (string)"
 * default ""
 * Name of predefined configuration to be enabled, note that
 * if this is not defined (empty string) then the first available
 * configuration will be used (assuming that the required interface
 * function drivers are available.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NAME
