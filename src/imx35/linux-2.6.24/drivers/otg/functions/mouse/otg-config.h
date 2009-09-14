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
# @(#) balden@belcarra.com|otg/functions/mouse/otg-config.h|20060419204257|26035
 *
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 */

/*
 * tristate "  Random Mouse Function"
 * This implements a simple Mouse driver that sends HID packets with
 * small random movements. This is a good function for initial testing
 * of Peripheral Controller Drivers as it provides for a complicated
 * enumeration but a simple uni-directional (send Interrupt only) data
 * transport.
 */
/* #define CONFIG_OTG_MOUSE */

/*
 * bool "Build an interface module"
 * default "y"
 * This module needs a composite function module, such as
 * mouse_cf or generic_cf to be activated
 */
#define CONFIG_OTG_MOUSE_INTERFACE

/*
 * bool "Build a simple OTG 2.x style composite function with mouse"
 * default "n"
 * This module provides a very simple composite function to drive
 * the mouse_if module (see above).  The generic_cf module provides
 * a much richer composite function supporting multiple functions.
 */
#define CONFIG_OTG_MOUSE_COMPOSITE

/*
 * bool "Build an OTG 1.x style function module/driver"
 * depends on OTG && OTG_MOUSE
 * default "n"
 * This is the mouse function driver from previous releases. It
 * does not use the composite/interface mechanism. If this module
 * is used, neither of the other two mouse modules is required.
 */
#define CONFIG_OTG_MOUSE_TRADITIONAL

/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 * This parameter is not used by the mouse_if module because the
 * composite layer is responsible for setting this parameter
 */
#define CONFIG_OTG_MOUSE_VENDORID 0x15ec

/*
 * hex "ProductID (hex value)"
 * default "0xf003"
 * Parameters such as PRODUCTID, VENDORID are now the responsibility
 * of the composite driver rather than the interface driver. For this
 * reason, the module parameters vendor_id, etc are not available in
 * the interface versions. Therefore, appropriate values should be set
 * in the kernel configuration. These configuration parameters
 * are referenced in both mouse_cf and generic_cf.
 */
#define CONFIG_OTG_MOUSE_PRODUCTID 0xf003

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_MOUSE_BCDDEVICE 0x0100

/*
 * string "iManufacturer (string)"
 * default "Belcarra"
 */
#define CONFIG_OTG_MOUSE_MANUFACTURER "Belcarra"

/*
 * string "iProduct (string)"
 * default "Random Mouse Device"
 */
#define CONFIG_OTG_MOUSE_PRODUCT_NAME "Random Mouse Device"

/*
 * string "MOUSE Bulk Only iInterface (string)"
 * default "MOUSE Data Intf"
 */
#define CONFIG_OTG_MOUSE_COMM_INTF "MOUSE Data Intf"

/*
 * string "Data Interface iConfiguration (string)"
 * default "MOUSE Configuration"
 */
#define CONFIG_OTG_MOUSE_DESC "MOUSE Configuration"

/*
 * bool "  MOUSE BH Test"
 * default n
 * Implement the Mouse send packet in a bottom half handler,
 * this will delay responses to test the PCD works correctly
 * when the host polls before a send data urb is queued.
 */
#define CONFIG_OTG_MOUSE_BH

/*
 * int "Number of packets (zero is continous)"
 * default 10
 * Number of Mouse packets to send, will run
 * forever if set to zero.
 */
#define CONFIG_OTG_MOUSE_PACKETS 10

/*
 * bool "  MOUSE Stall Test"
 * default n
 * Periodically stall the INTERRUPT endpoint.
 * Used for testing.
 */
#define CONFIG_OTG_MOUSE_STALL

/*
 * int "Number of Stalls"
 * default 1
 * Number of Stall tests to perform.
 */
#define CONFIG_OTG_MOUSE_STALL_COUNT

/*
 * int 'Polling Interval '
 * default 1
 * Sets interrupt endpoint interval.
 */
#define CONFIG_OTG_MOUSE_INTERVAL 1
