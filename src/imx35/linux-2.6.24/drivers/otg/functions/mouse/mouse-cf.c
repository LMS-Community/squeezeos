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
 * otg/functions/mouse/mouse-cf.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/mouse/mouse-cf.c|20061218212925|53990
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
 * @defgroup MouseComposite Mouse Composite Function
 * @ingroup CompositeFunctions
 */

/*!
 * @file otg/functions/mouse/mouse-cf.c
 * @brief Mouse Configuration Function Driver
 *
 * This implements the composite function portion of the composite
 * random mouse driver.
 *
 * It defines the device descriptor charateristics, selects the
 * single class function and a list of one or more interface functions.
 *
 * The primary purpose of this driver is to demonstrate how to
 * implement a simple composite function driver and to provide a simple
 * uni-directional driver for testing USB Device peripheral drivers.
 *
 * The mouse driver has several other characteristics to allow testing of
 * other features of USB Device peripheral drivers:
 *
 *      - ep0 ZLP handling
 *
 * To verify that ep0 ZLP processing is being handling correctly this
 * driver has a hard coded Product name that is returned as a 32 byte
 * string. This forces most any implementations that have an endpoint
 * zero packetsize of 8, 16 or 32 to send a Zero Length Packet to
 * terminate the string transfer when the Product name is requested
 * by the host.
 *
 * There is no request or data handling required for mouse. So this
 * only implements required descriptors.
 *
 * @ingroup MouseComposite
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

/*
 * ep0 testing.... ensure that this is exactly 16 bytes
 */
#undef CONFIG_OTG_MOUSE_PRODUCT_NAME
#define CONFIG_OTG_MOUSE_PRODUCT_NAME "Belcarra  Mouse"

static const char *mouse_usb_strings[4] = {
        "Mouse-cf String Test 1",
        "Mouse-cf String Test 3",
        "Mouse-cf String Test 5",
        NULL,
};

static u8 mouse_usb_string_indexes[4] = {
        1, 3, 5, 0,
};


#if !defined(OTG_C99)

/*! @name mouse descriptors
 * @{ */

/*! Configuration description(s)
 */
static struct usbd_configuration_descriptor  mouse_configuration_descriptor;

/*! Mouse Configuration Description
 */
struct usbd_configuration_description mouse_cf_configuration_description[1];

/*! Device Description
 */
static struct usbd_device_descriptor mouse_cf_device_descriptor;

#ifdef CONFIG_OTG_HIGH_SPEED
/*! High Speed Device Description
 */
static struct usbd_device_qualifier_descriptor mouse_device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */

/*! OTG Descriptor
 */
static struct usbd_otg_descriptor mouse_otg_descriptor;

/*! Device Description
 */
struct usbd_device_description mouse_cf_device_description;
/* @} */

/*! mouse_cf_glocal_init - initialize global mouse configuration instance
 */

void mouse_cf_global_init(void)
{
        /*! Configuration description(s)
         */
        ZERO(mouse_configuration_descriptor);
        mouse_configuration_descriptor.bLength = 0x09;
        mouse_configuration_descriptor.bDescriptorType = USB_DT_CONFIGURATION;
        mouse_configuration_descriptor.wTotalLength = 0x00;

        /*! Mouse Configuration Description
         */
        ZERO(mouse_cf_configuration_description);
        //mouse_cf_configuration_description[0].configuration_descriptor = &mouse_configuration_descriptor;
        mouse_cf_configuration_description[0].iConfiguration = "USB Random Mouse Configuration";

        #if 0
        /*! Device Description
         */
        ZERO(mouse_cf_device_descriptor);
        mouse_cf_device_descriptor.bLength = sizeof(struct usbd_device_descriptor);
        mouse_cf_device_descriptor.bDescriptorType = USB_DT_DEVICE;
        mouse_cf_device_descriptor.bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION);
        mouse_cf_device_descriptor.bDeviceClass = 0x00;
        mouse_cf_device_descriptor.bDeviceSubClass = 0x00;
        mouse_cf_device_descriptor.bDeviceProtocol = 0x00;
        mouse_cf_device_descriptor.bMaxPacketSize0 = 0x00;
        mouse_cf_device_descriptor.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID);
        mouse_cf_device_descriptor.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID);
        mouse_cf_device_descriptor.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE);

#ifdef CONFIG_OTG_HIGH_SPEED
        /*! High Speed Device Description
         */
        ZERO(mouse_device_qualifier_descriptor);
        mouse_device_qualifier_descriptor.bLength = sizeof(struct usbd_device_qualifier_descriptor);
        mouse_device_qualifier_descriptor.bDescriptorType = USB_DT_DEVICE_QUALIFIER;
        mouse_device_qualifier_descriptor.bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION);
        mouse_device_qualifier_descriptor.bDeviceClass = 0x00;
        mouse_device_qualifier_descriptor.bDeviceSubClass = 0x00;
        mouse_device_qualifier_descriptor.bDeviceProtocol = 0x00;
        mouse_device_qualifier_descriptor.bMaxPacketSize0 = 0x00;
#endif /* CONFIG_OTG_HIGH_SPEED */
        #endif

        /*! OTG Descriptor
         */
        ZERO(mouse_otg_descriptor);
        mouse_otg_descriptor.bLength = sizeof(struct usbd_otg_descriptor);
        mouse_otg_descriptor.bDescriptorType = USB_DT_OTG;
        mouse_otg_descriptor.bmAttributes = 0;

        /*! Device Description
         */
        ZERO(mouse_cf_device_description);
        //mouse_cf_device_description.device_descriptor = &mouse_cf_device_descriptor;
#ifdef CONFIG_OTG_HIGH_SPEED
        mouse_cf_device_description.device_qualifier_descriptor = &mouse_device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */
        mouse_cf_device_description.bDeviceClass = 0x00;
        mouse_cf_device_description.bDeviceSubClass = 0x00;
        mouse_cf_device_description.bDeviceProtocol = 0x00;
        mouse_cf_device_description.bMaxPacketSize0 = 0x00;
        mouse_cf_device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID);
        mouse_cf_device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID);
        mouse_cf_device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE);
        mouse_cf_device_description.otg_descriptor = &mouse_otg_descriptor;
        mouse_cf_device_description.iManufacturer = CONFIG_OTG_MOUSE_MANUFACTURER;
        mouse_cf_device_description.iProduct = CONFIG_OTG_MOUSE_PRODUCT_NAME;
#if !defined(CONFIG_OTG_NO_SERIAL_NUMBER) && defined(CONFIG_OTG_SERIAL_NUMBER_STR)
        mouse_cf_device_description.iSerialNumber = CONFIG_OTG_SERIAL_NUMBER_STR;
#endif
}

#else /* defined(OTG_C99) */


/*! @var struct usbd_configuartion_descriptor mouse_configuration_description(s)
 */
static struct usbd_configuration_descriptor  mouse_configuration_descriptor = {
        .bLength = 0x09,
        .bDescriptorType = USB_DT_CONFIGURATION,
};

/*! @var struct usbd_configuration_description mouse _configuration_description
 */
struct usbd_configuration_description mouse_cf_configuration_description[] = {
      { //.configuration_descriptor = &mouse_configuration_descriptor,
              .iConfiguration = "USB Random Mouse Configuration",
      },
};

#if 0
/*! Device Description
 */
static struct usbd_device_descriptor mouse_cf_device_descriptor = {
        .bLength = sizeof(struct usbd_device_descriptor),
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION),
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 0x00,
        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID),
        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID),
        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE),
};

#ifdef CONFIG_OTG_HIGH_SPEED
/*! High Speed Device Description
 */
static struct usbd_device_qualifier_descriptor mouse_device_qualifier_descriptor = {
        .bLength = sizeof(struct usbd_device_qualifier_descriptor),
        .bDescriptorType = USB_DT_DEVICE_QUALIFIER,
        .bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION),
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 0x00,
};
#endif /* CONFIG_OTG_HIGH_SPEED */
#endif

#if 0
/*! OTG Descriptor
 */
static struct usbd_otg_descriptor mouse_otg_descriptor = {
        .bLength = sizeof(struct usbd_otg_descriptor),
        .bDescriptorType = USB_DT_OTG,
        .bmAttributes = 0,
};
#endif

/*! @var struct usbd_device_description mouse_cf_device_description
 */
struct usbd_device_description mouse_cf_device_description = {
        //.device_descriptor = &mouse_cf_device_descriptor,
#ifdef CONFIG_OTG_HIGH_SPEED
        .device_qualifier_descriptor = &mouse_device_qualifier_descriptor,
#endif /* CONFIG_OTG_HIGH_SPEED */
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 0x00,
        .idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID),
        .idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID),
        .bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE),
        //.otg_descriptor = &mouse_otg_descriptor,
        .iManufacturer = CONFIG_OTG_MOUSE_MANUFACTURER,
        .iProduct = CONFIG_OTG_MOUSE_PRODUCT_NAME,
#if !defined(CONFIG_OTG_NO_SERIAL_NUMBER) && defined(CONFIG_OTG_SERIAL_NUMBER_STR)
        .iSerialNumber = CONFIG_OTG_SERIAL_NUMBER_STR,
#endif
};
#endif /* defined(OTG_C99) */


/*! @} */


/* MOUSE ***************************************************************************************** */

/* USB Device Functions ************************************************************************ */

/*! mouse_cf_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int mouse_cf_function_enable (struct usbd_function_instance *function)
{
        // XXX MODULE LOCK HERE
        return 0;
}

/*! mouse_cf_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
static void mouse_cf_function_disable (struct usbd_function_instance *function)
{
        // XXX MODULE UNLOCK HERE
}


/* ********************************************************************************************* */
#if !defined(OTG_C99)
/*! struct usbd_functions mouse_function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops;

/*!  struct usbd_composite_driver mouse_composite_driver - USB Device Core function driver definition
  */
struct usbd_composite_driver mouse_composite_driver;

/*! mouse_cf_ops_init - initialze operations table for mouse configuration called by usbd core
 */

void mouse_cf_ops_init(void)
{
        /*! function_ops - operations table for the USB Device Core
         */
        ZERO(mouse_function_ops);
        mouse_function_ops.function_enable = mouse_cf_function_enable;
        mouse_function_ops.function_disable = mouse_cf_function_disable;

        /*! composite_driver - USB Device Core function driver definition
         */
        ZERO(mouse_composite_driver);
        mouse_composite_driver.driver.name = "mouse-random-fd";                         /*! driver name */
        mouse_composite_driver.driver.fops = mouse_function_ops;                       /*! operations table */
        mouse_composite_driver.driver.usb_strings = mouse_usb_strings;                 /*! operations table */
        mouse_composite_driver.driver.usb_string_indexes = &mouse_usb_string_indexes;   /*! operations table */
        mouse_composite_driver.device_description = &mouse_cf_device_description;       /*! mouse device description */
        mouse_composite_driver.bNumConfigurations =
                sizeof (mouse_cf_configuration_description) / sizeof (struct usbd_configuration_description);
        mouse_composite_driver.configuration_description= mouse_cf_configuration_description;   /*! mouse configuration */
        //mouse_composite_driver.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID);
        //mouse_composite_driver.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID);
        //mouse_composite_driver.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE);

        //mouse_composite_driver.interfaces = 0;
        //mouse_composite_driver.interface_list = NULL;
        //mouse_composite_driver.endpointsRequested = 0;
        //mouse_composite_driver.requestedEndpoints = NULL;
}
#else /* defined(OTG_C99) */
/*! @var struct usbd_function_operation mouse_function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops = {
        .function_enable = mouse_cf_function_enable,
        .function_disable = mouse_cf_function_disable,
};

/*! mouse_composite_driver - USB Device Core function driver definition
 */
struct usbd_composite_driver mouse_composite_driver = {
        .driver = {
                .name = "mouse-random-cf",
                .fops = &mouse_function_ops,
                .usb_strings = mouse_usb_strings,
                .usb_string_indexes = mouse_usb_string_indexes,
        },
        .device_description = &mouse_cf_device_description,
        .bNumConfigurations = sizeof (mouse_cf_configuration_description) / sizeof (struct usbd_configuration_description),
        .configuration_description = mouse_cf_configuration_description,
};
#endif /* defined(OTG_C99) */


/* Module Parameters ******************************************************** */
/* !
 * @name XXXXX MODULE Parameters
 */
/* ! @{ */

MOD_AUTHOR ("sl@belcarra.com");
EMBED_LICENSE();
MOD_DESCRIPTION ("Belcarra Random Walk MOUSE Function");

static u32 vendor_id;           /*!< override built-in vendor ID */
static u32 product_id;          /*!< override built-in product ID */

MOD_PARM (vendor_id, "i");
MOD_PARM (product_id, "i");

MOD_PARM_DESC (vendor_id, "Device Vendor ID");
MOD_PARM_DESC (product_id, "Device Product ID");

otg_tag_t MOUSE;
/* ! *} */

/* USB Module init/exit ***************************************************** */
/*!  char *mouse_arg_list[] */
char *mouse_arg_list[] = {
        //"2=mouse-random-if",
        "mouse-random-if",
        //"mouse-random-if",
        //"mouse-random-if",
        //"mouse-random-if",
        NULL,
};

/*!
 * mouse_cf_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the mouse driver with the USB Device software.
 *
 * @return 0 on success
 *
 */
static int mouse_cf_modinit (void)
{
        int i;
        printk (KERN_INFO "%s: vendor_id: %04x product_id: %04x\n", __FUNCTION__, vendor_id, product_id);

        #if !defined(OTG_C99)
        mouse_cf_global_init();
        mouse_cf_ops_init();
        #endif /* defined(OTG_C99) */

        MOUSE = otg_trace_obtain_tag(NULL, "mouse-cf");
        TRACE_MSG2(MOUSE, "vendor_id: %04x product_id: %04x",vendor_id, product_id);

        //if (vendor_id)
        //        mouse_composite_driver.idVendor = cpu_to_le16(vendor_id);
        //if (product_id)
        //        mouse_composite_driver.idProduct = cpu_to_le16(product_id);


        // register as usb function driver
        TRACE_MSG0(MOUSE, "REGISTER COMPOSITE");

        THROW_IF (usbd_register_composite_function (&mouse_composite_driver,
                                        "mouse-random-cf", NULL, mouse_arg_list, NULL), error);

        TRACE_MSG0(MOUSE, "REGISTER FINISHED");

        CATCH(error) {
                otg_trace_invalidate_tag(MOUSE);
                return -EINVAL;
        }
        return 0;
}

module_init (mouse_cf_modinit);

#if OTG_EPILOGUE
/*!
 * mouse_cf_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 *
 * @param void
 * @return void
 */
static void mouse_cf_modexit (void)
{
        usbd_deregister_composite_function (&mouse_composite_driver);
        otg_trace_invalidate_tag(MOUSE);
}

module_exit (mouse_cf_modexit);
#endif

#endif /* defined(CONFIG_OTG_LNX) */
