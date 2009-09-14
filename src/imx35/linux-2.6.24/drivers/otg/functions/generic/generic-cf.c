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
 * otg/functions/generic/generic-cf.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/generic/generic-cf.c|20070425221028|63290
 *
 *      Copyright (c) 2003-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/functions/generic/generic-cf.c
 * @brief Generic Configuration Function Driver
 *
 * This implements a generic composite function
 *
 *         - idVendor              16 bit word
 *         - idProduct             16 bit word
 *         - bcd_device            16 bit word
 *
 *         - bDeviceClass          byte
 *         - device_sub-class      byte
 *         - bcdDevice             byte
 *
 *         - vendor                string
 *         - iManufacturer         string
 *         - product               string
 *         - serial                string
 *
 *         - power                 byte
 *         - remote_wakeup         bit
 *
 *         - functions             string
 *
 *
 * The functions string would contain a list of names, the first would
 * be the composite function driver, the rest would be the interface
 * functions. For example:
 *
 *
 *         "cmouse-cf mouse-if"
 *         "mcpc-cf dun-if dial-if obex-if dlog-if"
 *
 * There are also a set of pre-defined configurations that
 * can be loaded singly or in toto.
 *
 * @ingroup GenericFunction
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>

#include "generic.h"
//#include "generic.c"

/* Module Parameters ******************************************************** */
/* !
 * @name XXXXX MODULE Parameters
 */
/* ! @{ */


MOD_AUTHOR ("sl@belcarra.com");
EMBED_LICENSE();
MOD_DESCRIPTION ("Generic Composite Function");


MOD_PARM_STR (driver_name, "Driver Name", NULL);

MOD_PARM_INT(idVendor, "Device Descriptor idVendor field", 0);
MOD_PARM_INT(idProduct, "Device Descriptor idProduct field", 0);
MOD_PARM_INT(bcddevice, "Device Descriptor bcdDevice field", 0);

MOD_PARM_INT (bDeviceClass, "Device Descriptor bDeviceClass field", 0);
MOD_PARM_INT (bDeviceSubClass, "Device Descriptor bDeviceSubClass field", 0);
MOD_PARM_INT (bDeviceProtocol, "Device Descriptor bDeviceProtocol field", 0);
MOD_PARM_INT (bcdDevice, "Device Descriptor bcdDevice field", 0);

MOD_PARM_STR (iConfiguration, "Configuration Descriptor iConfiguration field", NULL);
MOD_PARM_STR (iManufacturer, "Device Descriptor iManufacturer field", NULL);
MOD_PARM_STR (iProduct, "Device Descriptor iProduct field", NULL);
MOD_PARM_STR (iSerialNumber, "Device Descriptor iSerialNumber field", NULL);

MOD_PARM_STR (class_name, "Class Function Name", NULL);
MOD_PARM_STR (interface_names, "Interface Function Names List", NULL);

MOD_PARM_STR (config_name, "Pre-defined Configuration", NULL);
MOD_PARM_BOOL (load_all, "Load all pre-defined Configurations", 1);
MOD_PARM_BOOL (override, "Override idVendor and idProduct", 0);

/* Setup the pre-defined configuration name from configuration
 * option if available.
 *
 * N.B. This list needs to be synchronized with both pre-defined
 * configurations (see below) and the Kconfig list
 */
char *generic_predefined_configuration = "";


/* ! *} */


/* Setup the pre-defined configuration name from configuration
 * option if available.
 *
 * N.B. This list needs to be synchronized with both pre-defined
 * configurations (see below) and the Kconfig list
 */
#ifdef CONFIG_OTG_GENERIC_CONFIG_MOUSE
static char default_predefined_configuration[] = "hid";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_NET_BLAN)
static char default_predefined_configuration[] = "mdlm-blan";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_NET_SAFE)
static char default_predefined_configuration[] = "mdlm-safe";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_NET_CDC)
static char default_predefined_configuration[] = "cdc-ecm";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_NET_EEM)
static char default_predefined_configuration[] = "cdc-eem";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_ACM_TTY)
static char default_predefined_configuration[] = "serial";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_MSC)
static char default_predefined_configuration[] = "mass";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_HID2)
static char default_predefined_configuration[] = "hid2";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_HID2_NON_IAD)
static char default_predefined_configuration[] = "hid2-non_iad";
#elif defined(CONFIG_OTG_GENERIC_CONFIG_MSC_HID)
static char default_predefined_configuration[] = "msc-hid";
#else
static char default_predefined_configuration[] = "";
#endif

static char *generic_config_name(void){
        if(MODPARM(config_name) && strlen(MODPARM(config_name))) return MODPARM(config_name);
        printk(KERN_INFO"%s: %s\n", __FUNCTION__, default_predefined_configuration);
        return default_predefined_configuration;
}

static int preset_config_name(void){
        return (strlen(generic_config_name()) > 0);
}


/* Pre-defined configurations *********************************************** */

static struct generic_config generic_cf_configs[] = {
        #if defined(CONFIG_OTG_ACM) ||  defined(CONFIG_OTG_ACM_MODULE) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "serial", }, },
                .interface_names = "tty-if",
                .configuration_description = {
                        .iConfiguration = "acm-tty",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_SERIAL),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " Serial", },

        },
        {
                .composite_driver = { .driver = { .name = "modem", }, },
                .interface_names = "tty-if",
                .configuration_description = {
                        .iConfiguration = "acm-tty",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_MODEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " Serial", },

        },
        #endif /* defined(CONFIG_OTG_ACM) */
        #if defined(CONFIG_OTG_NETWORK_EEM) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "cdc-eem", }, },
                .interface_names = "cdc-eem-if",
                .configuration_description = {
                        .iConfiguration = "cdc-eem",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = COMMUNICATIONS_EEM_SUBCLASS,
                        .bDeviceProtocol = COMMUNICATIONS_EEM_PROTOCOL,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCEEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " CDC-EEM", }
        },
        #endif /* defined(CONFIG_OTG_NETWORK_EEM) */
        #if defined(CONFIG_OTG_NETWORK_ECM) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "cdc-ecm", }, },
                .interface_names = "cdc-ecm-if",
                .configuration_description = {
                        .iConfiguration = "cdc-ecm",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCECM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " CDC-ECM", },
        },
        #endif /* defined(CONFIG_OTG_NETWORK_CDC) */
        #if defined(CONFIG_OTG_MOUSE) || defined(CONFIG_OTG_MOUSE_MODULE) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "hid", }, },
                .interface_names = "mouse-if",
                .configuration_description = {
                        .iConfiguration = "mouse",
                },
                .device_description = {
                        .bDeviceClass = 0,
                        .bDeviceSubClass = 0,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " Mouse", },
        },
        #endif /* defined(CONFIG_OTG_MOUSE) */
        #if defined(CONFIG_OTG_MSC) || defined(CONFIG_OTG_MSC_MODULE) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "mass", }, },
                .interface_names = "msc-if",
                .configuration_description = {
                        .iConfiguration = "msc",
                },
                .device_description = {
                        .bDeviceClass = 0,
                        .bDeviceSubClass = 0,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_MASS),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " Mass Storage", },
        },
        #endif /* defined(CONFIG_OTG_MSC)  */

        /* deprecated - use cdc-eem */
        #if defined(CONFIG_OTG_NETWORK_BLAN) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "network", }, },
                .interface_names = "net-blan-if",
                .configuration_description = {
                        .iConfiguration = "net-blan",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCEEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " MDLM-BLAN", },
        },
        {
                .composite_driver = { .driver = { .name = "mdlm-blan", }, },
                .interface_names = "net-blan-if",
                .configuration_description = {
                        .iConfiguration = "net-blan",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCEEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " MDLM-BLAN", },
        },
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN) */

        #if defined(CONFIG_OTG_NETWORK_SAFE) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "mdlm-safe", }, },
                .interface_names = "net-safe-if",
                .configuration_description = {
                        .iConfiguration = "net-safe",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCEEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " MDLM-SAFE", },
        },
        #endif /* defined(CONFIG_OTG_NETWORK_SAFE) */

        #if defined(CONFIG_OTG_NETWORK_BASIC) || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "net-basic", }, },
                .interface_names = "net-basic-if",
                .configuration_description = {
                        .iConfiguration = "net-basic",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_CDCEEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " NET-BASIC", },
        },
        #endif /* defined(CONFIG_OTG_NETWORK_BASIC) */

        #if defined(CONFIG_OTG_MOUSE) || defined(CONFIG_OTG_MOUSE_MODULE)
        {
                .composite_driver.driver.name = "hid2-non-iad",
                .interface_names = "mouse-if:mouse-if",
                .configuration_description.iConfiguration = "mouse2",
                .device_description.bDeviceClass = 0,
                .device_description.bDeviceSubClass = 0,
                .device_description.bDeviceProtocol = 0,
                .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID2),
                .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .device_description.iProduct = CONFIG_OTG_GENERIC_MANUFACTURER " USB HID (Mouse2)",
        },
        {
                .composite_driver.driver.name = "hid2",
                .interface_names = "mouse-if:mouse-if",
                .configuration_description.iConfiguration = "mouse2",
                .device_description.bDeviceClass = USB_CLASS_MISC,
                .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID2),
                .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .device_description.iProduct = CONFIG_OTG_GENERIC_MANUFACTURER " USB HID (Mouse2) IAD",
        },
        #endif /* defined(CONFIG_OTG_MOUSE) */

        #if (defined(CONFIG_OTG_MSC_MODULE) || defined(CONFIG_OTG_MSC)) && \
        (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE))
        {
                .composite_driver.driver.name = "hid-msc",
                .interface_names = "mouse-if:msc-if",
                .configuration_description.iConfiguration = "mouse-msc",
                .device_description.bDeviceClass = USB_CLASS_MISC,
                .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID_MSC),
                .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .device_description.iProduct = CONFIG_OTG_GENERIC_MANUFACTURER " HID-MSC Composite IAD",
        },
        #endif /* defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_MOUSE)) */

        #if defined(CONFIG_OTG_NETWORK_BLAN) && (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE))
        {
                .composite_driver.driver.name = "hid-blan",
                .interface_names = "mouse-if:net-blan-if",
                .configuration_description.iConfiguration = "hid-blan",
                .device_description.bDeviceClass = USB_CLASS_MISC,
                .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID_BLAN),
                .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .device_description.iProduct = CONFIG_OTG_GENERIC_MANUFACTURER " HID-BLAN Composite IAD",
        },
        #endif /* defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_MOUSE)) */
        #if defined(CONFIG_OTG_NETWORK_ECM) && (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE))
        {
                .composite_driver.driver.name = "hid-ecm",
                .interface_names = "mouse-if:cdc-ecm-if",
                .configuration_description.iConfiguration = "hid-ecm",
                .device_description.bDeviceClass = USB_CLASS_MISC,
                .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID_ECM),
                .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .device_description.iProduct = CONFIG_OTG_GENERIC_MANUFACTURER " HID-BLAN Composite IAD",
        },
        #endif /* defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_MOUSE)) */

        #if ( \
                        (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE)) && \
                        (defined(CONFIG_OTG_ACM) ||  defined(CONFIG_OTG_ACM_MODULE))) \
                || defined(_OTG_DOXYGEN)
        {
                .composite_driver = { .driver = { .name = "hid-serial", }, },
                .interface_names = "hid:tty-if",
                .configuration_description = {
                        .iConfiguration = "hid-acm-tty",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID_SERIAL),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " HID-Serial", },

        },
        {
                .composite_driver = { .driver = { .name = "hid-modem", }, },
                .interface_names = "hid:tty-if",
                .configuration_description = {
                        .iConfiguration = "hid-acm-tty",
                },
                .device_description = {
                        .bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
                        .bDeviceSubClass = 2,
                        .bDeviceProtocol = 0,
                        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID_HID_MODEM),
                        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                        .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                        .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME " HID-Serial", },

        },
        #endif /* defined(CONFIG_OTG_ACM) && defined(CONFIG_OTG_MOUSE) */


        {  },
};

static struct generic_config generic_config = {
        .composite_driver = { .driver = { .name = "generic", }, },
        .device_description = {
                .idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
                .idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID),
                .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
                .iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
                .iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME, },
};




static otg_tag_t GENERIC;

/*!
 * generic_cf_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the generic driver with the USB Device software.
 *
 */
int generic_cf_modinit (void)
{
        /* load config or configs
         */

        #if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)
        GENERIC = otg_trace_obtain_tag(NULL, "generic-cf");
        #endif

        if (preset_config_name() || MODPARM(load_all)) {
                TRACE_MSG0(GENERIC, "PRESET");
                return generic_modinit(generic_cf_configs,
                                generic_config_name(),
                                MODPARM(override),
                                MODPARM(idVendor),
                                MODPARM(idProduct),
                                MODPARM(iSerialNumber),
                                GENERIC);
        }
        else {
                struct generic_config *config = &generic_config;

                TRACE_MSG0(GENERIC, "SEARCH");

                printk (KERN_INFO "%s: idVendor: %04x idProduct: %04x\n", __FUNCTION__, MODPARM(idVendor), MODPARM(idProduct));
                printk (KERN_INFO "%s: class_name: \"%s\" _interface_names: \"%s\"\n",
                                __FUNCTION__, MODPARM(class_name), MODPARM(interface_names));

                if (MODPARM(driver_name) && strlen(MODPARM(driver_name)))
                        config->composite_driver.driver.name = MODPARM(driver_name);

                if (MODPARM(class_name) && strlen(MODPARM(class_name)))
                        config->class_name = MODPARM(class_name);

                if (MODPARM(interface_names) && strlen(MODPARM(interface_names)))
                        config->interface_names = MODPARM(interface_names);

                if (MODPARM(iConfiguration) && strlen(MODPARM(iConfiguration)))
                        config->configuration_description.iConfiguration = MODPARM(iConfiguration);

                if (MODPARM(bDeviceClass))
                        config->device_description.bDeviceClass = MODPARM(bDeviceClass);

                if (MODPARM(bDeviceSubClass))
                        config->device_description.bDeviceSubClass = MODPARM(bDeviceSubClass);

                if (MODPARM(bDeviceProtocol))
                        config->device_description.bDeviceProtocol = MODPARM(bDeviceProtocol);

                if (MODPARM(idVendor))
                        config->device_description.idVendor = MODPARM(idVendor);
                else
                        config->device_description.idVendor = CONFIG_OTG_GENERIC_VENDORID;

                if (MODPARM(idProduct))
                        config->device_description.idProduct = MODPARM(idProduct);
                else
                        config->device_description.idProduct = CONFIG_OTG_GENERIC_PRODUCTID;

                if (MODPARM(bcdDevice))
                        config->device_description.bcdDevice = MODPARM(bcdDevice);
                else
                        config->device_description.bcdDevice = CONFIG_OTG_GENERIC_BCDDEVICE;

                if (MODPARM(iManufacturer) && strlen(MODPARM(iManufacturer)))
                        config->device_description.iManufacturer = MODPARM(iManufacturer);
                else
                        config->device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER;

                if (MODPARM(iProduct) && strlen(MODPARM(iProduct)))
                        config->device_description.iProduct = MODPARM(iProduct);
                else
                        config->device_description.iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME;

                if (MODPARM(iSerialNumber) && strlen(MODPARM(iSerialNumber))){
                        config->device_description.iSerialNumber = MODPARM(iSerialNumber);
                }
                if (MODPARM(interface_names))
                        config->interface_names = MODPARM(interface_names);

                generic_register(config, NULL, FALSE, 0, 0, NULL, GENERIC);
                return  0;
        }
}

module_init (generic_cf_modinit);

/*!
 * generic_cf_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 * @return void
 */
void generic_cf_modexit (void)
{
        generic_modexit(generic_cf_configs, GENERIC);

        if (generic_config.registered)
                usbd_deregister_composite_function (&generic_config.composite_driver);

        if (generic_config.interface_list)
                LKFREE(generic_config.interface_list);

	otg_trace_invalidate_tag(GENERIC);

}
#if OTG_EPILOGUE
module_exit (generic_cf_modexit);
#endif
