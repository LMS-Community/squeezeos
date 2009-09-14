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
 * @(#) balden@belcarra.com|otg/functions/network/otg-config.h|20060419204258|52982
 *
 */


/*
 * tristate "  Network Function Driver"
 */
#define CONFIG_OTG_NETWORK


/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 */
#define CONFIG_OTG_NETWORK_VENDORID

/*
 * hex "ProductID (hex value)"
 * default "0xf001"
 */
#define CONFIG_OTG_NETWORK_PRODUCTID

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_NETWORK_BCDDEVICE

/*
 * string "iManufacturer (string)"
 * default "Belcarra"
 * This will be used as the iManufacturer string descriptor.
 */
#define CONFIG_OTG_NETWORK_MANUFACTURER

/*
 * string "iProduct (string)"
 * default "Belcarra Network Device"
 * This will be used as the iProduct string descriptor.
 */
#define CONFIG_OTG_NETWORK_PRODUCT_NAME

/*
 * bool 'Enable MDLM-BLAN networking'
 * BLAN supports non-infrastructure devices in virtual
 * bridged network environment.
 */
#define CONFIG_OTG_NETWORK_BLAN

/*
 * string "    iConfiguration (string)"
 * default "BLAN Net Cfg"
 * This will be used as the Configuration string descriptor
 * for the BLAN configuration.
 */
#define CONFIG_OTG_NETWORK_BLAN_DESC

/*
 * string "    iInterface (string)"
 * default "Comm/Data Intf"
 * This will be used as the Interface string descriptor
 * for the BLAN configuration.
 */
#define CONFIG_OTG_NETWORK_BLAN_INTF


/*
 * bool "        append 32bit CRC"
 * default TRUE
 * Setting this allows the host and device to verify that
 * all network frames have been successfully transferred.
 */
#define CONFIG_OTG_NETWORK_BLAN_CRC

/*
 * bool "        Automatically configure the network interface"
 * default TRUE
 * The driver will automatically configure the network interface
 * based on the IPADDR sent from the host to the device during
 * enumeration. This eliminates the need for hotplug.
 */
#define CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG


/*
 * bool "        Pad after"
 * default FALSE
 * This is used get around some buggy hardware, it
 * is generally not used. Say No.
 */
#define CONFIG_OTG_NETWORK_BLAN_PADAFTER

/*
 * bool "        Pad before"
 * default FALSE
 * This is used get around some buggy hardware, it
 * is generally not used. Say No.
 */
#define CONFIG_OTG_NETWORK_BLAN_PADBEFORE

/*
 * int "        Pad bytes"
 * default "8"
 * This is used get around some buggy hardware, it
 * is generally not used. Leave at default.
 */
#define CONFIG_OTG_NETWORK_BLAN_PADBYTES

/*
 * bool "        Fermat"
 * default FALSE
 * This is used get around some buggy hardware, it
 * is generally not used. Say No.
 */
#define CONFIG_OTG_NETWORK_BLAN_FERMAT

/*
 * bool "        Non-Bridged"
 * default FALSE
 * Tell host to not to enable bridge.
 */
#define CONFIG_OTG_NETWORK_BLAN_NONBRIDGED

/*
 * bool 'Enable MDLM-SAFE networking'
 * SAFE supports infrastructure devices but does not
 * require support for SET INTERFACE or interrupt endpoints
 */
#define CONFIG_OTG_NETWORK_SAFE

/*
 * string "    SAFE Data Interface iConfiguration (string)"
 * default "SAFE Net Cfg"
 * This will be used as the Configuration string descriptor
 * for the SAFE configuration.
 */
#define CONFIG_OTG_NETWORK_SAFE_DESC

/*
 * string "    SAFE Data Interface iInterface (string)"
 * default "Data Intf"
 * This will be used as the Interface string descriptor
 * for the SAFE configuration.
 */
#define CONFIG_OTG_NETWORK_SAFE_INTF

/*
 * bool "        append 32bit CRC"
 * default TRUE
 * Setting this allows the host and device to verify that
 * all network frames have been successfully transferred.
 */
#define CONFIG_OTG_NETWORK_SAFE_CRC

/*
 * bool "        Pad before"
 * default FALSE
 * This is used get around some buggy hardware, it
 * is generally not used. Say No.
 */
#define CONFIG_OTG_NETWORK_SAFE_PADBEFORE

/*
 * bool "        Bridged"
 * default FALSE
 * Tell host to enable bridge.
 */
#define CONFIG_OTG_NETWORK_SAFE_BRIDGED

/*
 * bool 'Enable CDC networking'
 * CDC implements the USB CDC Class Specification
 * to support infra-structure devices
 */
#define CONFIG_OTG_NETWORK_CDC

/*
 * string "    CDC iConfiguration (string)"
 * default "SAFE Net Cfg"
 * This will be used as the Configuration string descriptor
 * for the CDC configuration.
 */
#define CONFIG_OTG_NETWORK_CDC_DESC

/*
 * string "    CDC Comm Interface iInterface (string)"
 * default "Comm Intf"
 * This will be used as the Interface string descriptor
 * for the CDC configuration.
 */
#define CONFIG_OTG_NETWORK_CDC_COMM_INTF

/*
 * string "    CDC Data (diabled) iInterface (string)"
 * default "Data (Disabled) Intf"
 * This will be used as the Interface string descriptor
 * for the CDC no DATA interface.
 */
#define CONFIG_OTG_NETWORK_CDC_NODATA_INTF

/*
 * string "    CDC Data Interface iInterface (string)"
 * default "Data Intf"
 * This will be used as the Interface string descriptor
 * for the CDC Data Interface.
 */
#define CONFIG_OTG_NETWORK_CDC_DATA_INTF


/*
 * bool 'Enable Basic network'
 * Implement a very simple network configuration
 * with a single data interface.
 */
#define CONFIG_OTG_NETWORK_BASIC

/*
 * string "    BASIC Data Interface iConfiguration (string)"
 * default "BASIC Net Cfg"
 * This will be used as the Configuration string descriptor
 * for the BLAN configuration.
 */
#define CONFIG_OTG_NETWORK_BASIC_DESC

/*
 * string "    BASIC Data Interface iInterface (string)"
 * default "Data Intf"
 * This will be used as the Interface string descriptor
 * for the BLAN data interface.
 */
#define CONFIG_OTG_NETWORK_BASIC_INTF

/*
 * bool 'Enable Basic2 network'
 * Implement a very simple network configuration with
 * two interfaces.
 */
#define CONFIG_OTG_NETWORK_BASIC2

/*
 * string "    BASIC2 Data Interface iConfiguration (string)"
 * default "BASIC Net Cfg"
 * This will be used as the Configuration string descriptor
 * for the BASIC2 configuration.
 */
#define CONFIG_OTG_NETWORK_BASIC2_DESC

/*
 * string "    BASIC2 Comm Interface iInterface (string)"
 * default "Comm Intf"
 * This will be used as the Interface string descriptor
 * for the BASIC2 configuration.
 */
#define CONFIG_OTG_NETWORK_BASIC2_COMM_INTF

/*
 * string "    BASIC2 Data Interface iInterface (string)"
 * default "Data Intf"
 * This will be used as the Interface string descriptor
 * for the BASIC2 configuration.
 */
#define CONFIG_OTG_NETWORK_BASIC2_DATA_INTF
