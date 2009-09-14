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
 * otg/functions/mouse/mouse-fd.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/mouse/mouse-fd.c|20061218212925|31740
 *
 *      Copyright (c) 2003-2004 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @defgroup MouseSimple Mouse Simple Function
 * @ingroup SimpleFunctions
 */

/*!
 * @file otg/functions/mouse/mouse-fd.c
 * @brief Mouse Function Driver protocol implementation.
 *
 * This file implements the Mouse HID protocols and will generate
 * random mouse data on the INTERRUPT endpoint.
 *
 * The primary purpose of this driver is to demonstrate how to
 * implement a simple function driver and to provide a simple
 * uni-directional driver for testing USB Device peripheral drivers.
 *
 * The mouse driver has several other characteristics to allow testing of
 * other features of USB Device peripheral drivers:
 *
 *      - ep0 ZLP handling
 *
 *      - ep0 delayed CONTROL READ
 *
 * To verify that ep0 ZLP processing is being handling correctly this
 * driver has a hard coded Product name that is returned as a 32 byte
 * string. This forces most any implementations that have an endpoint
 * zero packetsize of 8, 16 or 32 to send a Zero Length Packet to
 * terminate the string transfer when the Product name is requested
 * by the host.
 *
 * The delayed CONTROL READ test verifies that that USB Device peripheral
 * drivers will correctly NAK until data is provide for device requests.
 * This is done by (optionally) delaying the HID report via a bottom half
 * handler. The device request returns normally and the peripheral driver
 * must properly recognize that while the device request had a non-zero
 * wLength field, there is currently no queued urb.
 *
 * When the bottom half handler is scheduled the HID report urb will be
 * queued on endpoint zero and then returned to the host.
 *
 *
 * @ingroup MouseSimple
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-hid.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

/*!@struct mouse_private mouse-fd.c  otg/functions/mouse/mouse-fd.c
 * @brief  The mouse_private structure is used to collect all of the mouse driver
 * global variables into one place.
 */
struct mouse_private {
#ifdef CONFIG_OTG_MOUSE_BH
        struct WORK_STRUCT notification_bh;
#endif /* CONFIG_OTG_MOUSE_BH */
        int usb_driver_registered;              /*!< non-zero if usb function registered */
        unsigned char connected;                /*!< non-zero if connected to host (configured) */
        unsigned int writesize;                 /*!< packetsize * 4 */
        struct usbd_urb *tx_urb;                /*!< saved copy of current tx urb */
        int wLength;
        int x;
        int y;
        int last_x;
        int last_y;
        int n;
};

#ifndef OTG_C99
extern void mouse_global_init(void);
#endif /* OTG_C99 */

#define MOUSE mouse_fd_trace_tag
extern otg_tag_t MOUSE;

struct mouse_private mouse_private;

/*
 * ep0 testing.... ensure that this is exactly 16 bytes
 */
#undef CONFIG_OTG_MOUSE_PRODUCT_NAME
#define CONFIG_OTG_MOUSE_PRODUCT_NAME "Belcarra  Mouse"

/*!
 * @name Mouse Descriptors
 *
 * MouseHIDReport
 * MOUSE Configuration
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */

/*! @name Mouse Descriptors
 */
/* @{*/

#define BULK_INT        0x00    /*!< Interrupt endpoint number */
#define ENDPOINTS       0x01    /*!< Number of endpoints required */

/*! List of required endpoint numbers
 */
static u8 mouse_index[] = { BULK_INT, };

/*! List of requested endpoints
 */
static struct usbd_endpoint_request mouse_endpoint_requests[ENDPOINTS+1] = {
        { BULK_INT, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT, 16, 64, 2, },
        { 0, },
};

/*! This is the HID report returned for the HID Device Request.
 *  There is no pre-defined descriptor type for this.
 */
static char MouseHIDReport[52] = {
    0x05, 0x01,                    /*!< USAGE_PAGE (Generic Desktop)            */
    0x09, 0x02,                    /*!< USAGE (Mouse)                           */
    0xa1, 0x01,                    /*!< COLLECTION (Application)                */
    0x09, 0x01,                    /*!<   USAGE (Pointer)                       */
    0xa1, 0x00,                    /*!<   COLLECTION (Physical)                 */
    0x05, 0x09,                    /*!<     USAGE_PAGE (Button)                 */
    0x19, 0x01,                    /*!<     USAGE_MINIMUM (Button 1)            */
    0x29, 0x03,                    /*!<     USAGE_MAXIMUM (Button 3)            */
    0x15, 0x00,                    /*!<     LOGICAL_MINIMUM (0)                 */
    0x25, 0x01,                    /*!<     LOGICAL_MAXIMUM (1)                 */
    0x95, 0x03,                    /*!<     REPORT_COUNT (3)                    */
    0x75, 0x01,                    /*!<     REPORT_SIZE (1)                     */
    0x81, 0x02,                    /*!<     INPUT (Data,Var,Abs)                */
    0x95, 0x01,                    /*!<     REPORT_COUNT (1)                    */
    0x75, 0x05,                    /*!<     REPORT_SIZE (5)                     */
    0x81, 0x03,                    /*!<     INPUT (Cnst,Var,Abs)                */
    0x05, 0x01,                    /*!<     USAGE_PAGE (Generic Desktop)        */
    0x09, 0x30,                    /*!<     USAGE (X)                           */
    0x09, 0x31,                    /*!<     USAGE (Y)                           */
    0x09, 0x38,                    /*!<     USAGE (WHEEL)                       */
    0x15, 0x81,                    /*!<     LOGICAL_MINIMUM (-127)              */
    0x25, 0x7f,                    /*!<     LOGICAL_MAXIMUM (127)               */
    0x75, 0x08,                    /*!<     REPORT_SIZE (8)                     */
    0x95, 0x03,                    /*!<     REPORT_COUNT (3)                    */
    0x81, 0x06,                    /*!<     INPUT (Data,Var,Rel)                */
    0xc0,                          /*!<   END_COLLECTION                        */
    0xc0                           /*!< END_COLLECTION                        */
};

#if !defined(OTG_C99)

/*! struct hid_descriptor mouse_fd_hid -  This is the HID desccriptor.
  */
static struct hid_descriptor mouse_fd_hid;

/*!  staruct usbd_geeric_class_descriptor * mouse_fd_hid_descriptor -  List of class descriptors
  */
static struct usbd_generic_class_descriptor *mouse_fd_hid_descriptors[] = {
        (struct usbd_generic_class_descriptor *)&mouse_fd_hid, };

/*!  struct usbd_interface_descriptor mouse_date_alternate_descriptor -  Data Interface Alternate description
  */
static struct usbd_interface_descriptor  mouse_data_alternate_descriptor;

/*!  struct usbd_alternate_description mouse_date_alternate_descriptions -  Interface Descriptions
  */
static struct usbd_alternate_description mouse_data_alternate_descriptions[1];

/*!  struct usbd_interface_description mouse_interfaces - List of Interface description(s)
  */
static struct usbd_interface_description mouse_interfaces[1];

/*!  struct usbd_configuration_descriptor mouse_configuration_descriptor - Configuration description(s)
 */
static struct usbd_configuration_descriptor  mouse_configuration_descriptor;

/*!  struct usbd_configuration_description mouse_fd_configuration - Mouse Configuration Description
  */
struct usbd_configuration_description mouse_fd_configuration_description[1];

/*!  struct usbd_device_descriptor mouse_fd_device_descriptor - Device Description
  */
static struct usbd_device_descriptor mouse_fd_device_descriptor;

#ifdef CONFIG_OTG_HIGH_SPEED
/*!  struct usbd_device_qualifier_descriptor mouse_device_qualifier_descriptor - High Speed Device Description
  */
static struct usbd_device_qualifier_descriptor mouse_device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */

/*! struct usbd_otg_descriptor  mouse_otg_descriptor - OTG Descriptor
  */
static struct usbd_otg_descriptor mouse_otg_descriptor;

/*! Device Description
 */
struct usbd_device_description mouse_fd_device_description;
/* mouse_global_init - initialize global variables for mouse instance
 */
void mouse_global_init(void)
{
        /*! This is the HID desccriptor.
         */
        ZERO(mouse_fd_hid);
        mouse_fd_hid.bLength = 0x09;
        mouse_fd_hid.bDescriptorType = 0x21;
        mouse_fd_hid.bcdHID = __constant_cpu_to_le16(0x110);
        mouse_fd_hid.bCountryCode = 0x00;
        mouse_fd_hid.bNumDescriptors = 0x01;
        mouse_fd_hid.bReportType = 0x22;
        mouse_fd_hid.wItemLength = __constant_cpu_to_le16(0x34);


        /*! Data Interface Alternate description
         */
        ZERO(mouse_data_alternate_descriptor);
        mouse_data_alternate_descriptor.bLength = 0x09;
        mouse_data_alternate_descriptor.bDescriptorType = USB_DT_INTERFACE;
        mouse_data_alternate_descriptor.bInterfaceNumber = 0x00;
        mouse_data_alternate_descriptor.bAlternateSetting = 0x00;
        mouse_data_alternate_descriptor.bNumEndpoints = 0x01;
        mouse_data_alternate_descriptor.bInterfaceClass = 0x03;
        mouse_data_alternate_descriptor.bInterfaceSubClass = 0x01;
        mouse_data_alternate_descriptor.bInterfaceProtocol = 0x02;
        mouse_data_alternate_descriptor.iInterface = 0x00;

        /*! Interface Descriptions
         */
        ZERO(mouse_data_alternate_descriptions);
        mouse_data_alternate_descriptions[0].iInterface = "Random Mouse Interface - Interrupt";
        //mouse_data_alternate_descriptions[0].interface_descriptor =  &mouse_data_alternate_descriptor;
        mouse_data_alternate_descriptions[0].classes =
                sizeof (mouse_fd_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *);
        mouse_data_alternate_descriptions[0].class_list = mouse_fd_hid_descriptors;
        mouse_data_alternate_descriptions[0].endpoints = 0x01;
        mouse_data_alternate_descriptions[0].endpoint_index =  mouse_index,

        /*! List of Interface description(s)
         */
        ZERO(mouse_interfaces);
        mouse_interfaces[0].alternates = sizeof (mouse_data_alternate_descriptions) / sizeof (struct usbd_alternate_description);
        mouse_interfaces[0].alternate_list = mouse_data_alternate_descriptions;


        /*! Configuration description(s)
         */
        ZERO(mouse_configuration_descriptor);
        mouse_configuration_descriptor.bLength = 0x09;
        mouse_configuration_descriptor.bDescriptorType = USB_DT_CONFIGURATION;
        mouse_configuration_descriptor.wTotalLength = 0x00;
        mouse_configuration_descriptor.bNumInterfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description);
        mouse_configuration_descriptor.bConfigurationValue = 0x01;
        mouse_configuration_descriptor.iConfiguration = 0x00;
        mouse_configuration_descriptor.bmAttributes = 0;
        mouse_configuration_descriptor.bMaxPower = 0;

        /*! Mouse Configuration Description
         */
        ZERO(mouse_fd_configuration_description);
        //mouse_fd_configuration_description[0].configuration_descriptor = &mouse_configuration_descriptor;
        mouse_fd_configuration_description[0].iConfiguration = "USB Random Mouse Configuration";
        //mouse_fd_configuration_description[0].interfaces =
         //       sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description);
       // mouse_fd_configuration_description[0].interface_list = mouse_interfaces;

        /*! Device Description
         */
        ZERO(mouse_fd_device_description);
        //mouse_fd_device_description.device_descriptor = &mouse_fd_device_descriptor;
#ifdef CONFIG_OTG_HIGH_SPEED
        mouse_fd_device_description.device_qualifier_descriptor = &mouse_device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */
        mouse_fd_device_description.bDeviceClass = 0x00;
        mouse_fd_device_description.bDeviceSubClass = 0x00;
        mouse_fd_device_description.bDeviceProtocol = 0x00;
        mouse_fd_device_description.bMaxPacketSize0 = 0x00;
        mouse_fd_device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID);
        mouse_fd_device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID);
        mouse_fd_device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE);
        mouse_fd_device_description.otg_descriptor = &mouse_otg_descriptor;
        mouse_fd_device_description.iManufacturer = CONFIG_OTG_MOUSE_MANUFACTURER;
        mouse_fd_device_description.iProduct = CONFIG_OTG_MOUSE_PRODUCT_NAME;
#if !defined(CONFIG_OTG_NO_SERIAL_NUMBER) && defined(CONFIG_OTG_SERIAL_NUMBER_STR)
        mouse_fd_device_description.iSerialNumber = CONFIG_OTG_SERIAL_NUMBER_STR;
#endif
}

#else /* defined(OTG_C99) */

/*! struct hid_descriptor mouse_fd_hid -  This is the HID desccriptor.
 */
struct hid_descriptor mouse_fd_hid = {
        .bLength = 0x09,
        .bDescriptorType = 0x21,
        .bcdHID = __constant_cpu_to_le16(0x110),
        .bCountryCode = 0x00,
        .bNumDescriptors = 0x01,
        .bReportType = 0x22,
        .wItemLength = __constant_cpu_to_le16(0x34),
};

/*! struct usbd_generic_class_descriptor mouse_fd_hid_descriptor -  List of class descriptors
 */
static struct usbd_generic_class_descriptor *mouse_fd_hid_descriptors[] = {
        (struct usbd_generic_class_descriptor *)&mouse_fd_hid, };

/*! struct usbd_alternate_descriptor mouse_date_alternate_descriptor - Interface Descriptions
 */
static struct usbd_alternate_description mouse_data_alternate_descriptions[] = {
        {
                .iInterface = "Random Mouse Interface - Interrupt",
                .bInterfaceClass = 0x03,
                .bInterfaceSubClass = 0x01,
                .bInterfaceProtocol = 0x02,
                .classes = sizeof (mouse_fd_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  mouse_fd_hid_descriptors,
                .endpoints = sizeof (mouse_index) / sizeof(u8),
                .endpoint_index =  mouse_index,
        },
};

/*!  struct usbd_interfacd_description mouse_interfaces -  List of Interface description(s)
 */
static struct usbd_interface_description mouse_interfaces[] = {
      { .alternates = sizeof (mouse_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = mouse_data_alternate_descriptions,},
};


/*! struct usbd_configuration_descriptor mouse_configuration_descriptor - Configuration description(s)
 */
static struct usbd_configuration_descriptor  mouse_configuration_descriptor = {
        .bLength = 0x09,
        .bDescriptorType = USB_DT_CONFIGURATION,
        .wTotalLength = 0x00,
        .bNumInterfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description),
        .bConfigurationValue = 0x01,
        .iConfiguration = 0x00,
        .bmAttributes = 0,
        .bMaxPower = 0,
};

/*! struct usbd_configuration_description mouse_fd_configuration_description[] - Mouse Configuration Description
 */
struct usbd_configuration_description mouse_fd_configuration_description[] = {
      { //.configuration_descriptor = &mouse_configuration_descriptor,
              .iConfiguration = "USB Random Mouse Configuration",
      },
};

#if 0
/*! OTG Descriptor
 */
static struct usbd_otg_descriptor mouse_otg_descriptor = {
        .bLength = sizeof(struct usbd_otg_descriptor),
        .bDescriptorType = USB_DT_OTG,
        .bmAttributes = 0,
};
#endif

/*! struct usbd_device_description mouse_fd_device_description - Device Description
 */
struct usbd_device_description mouse_fd_device_description = {
        //.device_descriptor = &mouse_fd_device_descriptor,
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


/*@}*/

static int mouse_count;
static int mouse_stalls;

/* MOUSE ***************************************************************************************** */
/* Transmit Function *************************************************************************** */
/*!  int get_xy[] */
static int get_xy[8] = {
         0,  0,  1,  1,
         0,  0, -1, -1,
};

/*!
 * mouse_fd_send() - send a mouse data urb with random data
 * @param function
 * @return void
 */
void mouse_fd_send(struct usbd_function_instance *function)
{
        struct mouse_private *mouse = &mouse_private;
        int new_x = 0;
        int new_y = 0;
        u8 random;

        TRACE_MSG1(MOUSE, "mouse_count: %d", mouse_count);

        memset(mouse->tx_urb->buffer, 0, 4);
        if (!mouse->n) {

                get_random_bytes(&random, 1);

                mouse->last_x = MAX(-4, MIN(4, mouse->last_x + get_xy[random & 0x7]));
                mouse->last_y = MAX(-4, MIN(4, mouse->last_y + get_xy[(random >> 3) & 0x7]));
                mouse->n = (random>>6) & 0x3;

                new_x = mouse->x + mouse->last_x;
                new_y = mouse->y + mouse->last_y;

                mouse->tx_urb->buffer[1] = mouse->last_x;
                mouse->x = new_x;
                mouse->tx_urb->buffer[2] = mouse->last_y;
                mouse->y = new_y;

        }
        else if ((mouse->n)&1) {
                mouse->n--;
        }
        else {
                mouse->n--;
                mouse->tx_urb->buffer[1] = mouse->last_x;
                mouse->x = new_x;
                mouse->tx_urb->buffer[2] = mouse->last_y;
                mouse->y = new_y;
        }

        mouse->tx_urb->actual_length = 4;
        usbd_start_in_urb(mouse->tx_urb);
}

/*!
 * mouse_urb_sent() - called to indicate URB transmit finished
 * This function is the callback function for sent urbs.
 * It simply queues up another urb until the packets to be sent
 * configuration parameter is reached (or forever if zero.)
 * @param urb The urb to be sent.
 * @param rc result
 * @return int Return non-zero for failure.
 */
static int mouse_urb_sent (struct usbd_urb *urb, int rc)
{
        struct mouse_private *mouse = &mouse_private;
        struct usbd_function_instance *function = urb->function_instance;

        RETURN_ZERO_IF(usbd_get_device_status(function) == USBD_CLOSING);
        RETURN_ZERO_IF(usbd_get_device_status(function) != USBD_OK);
        RETURN_ZERO_IF(usbd_get_device_state(function) != STATE_CONFIGURED);

        TRACE_MSG2(MOUSE, "mouse_count: %d urb status: %d", mouse_count, urb->status);

        #ifndef CONFIG_OTG_MOUSE_STALL
        #ifdef CONFIG_OTG_MOUSE_PACKETS
        TRACE_MSG0(MOUSE," Normal routine");
        RETURN_ZERO_IF (CONFIG_OTG_MOUSE_PACKETS && (mouse_count++ > CONFIG_OTG_MOUSE_PACKETS));
        mouse_fd_send(function);                                                   // re-send
        return 0;
        #endif /* CONFIG_OTG_MOUSE_PACKETS */
        #else /* CONFIG_OTG_MOUSE_STALL */

        if (mouse_count++ < 7) {
                mouse_fd_send(function);   // re-send
        }
        else {
                usbd_halt_endpoint(function, BULK_INT);
                mouse_count = 0;
                TRACE_MSG0(MOUSE," MOUSE STALL");
        }
        #endif /* CONFIG_OTG_MOUSE_STALL */
        return 0;
}

/* USB Device Functions ************************************************************************ */

typedef enum mesg {
        mesg_unknown,
        mesg_configured,
        mesg_reset,
} mesg_t;
mesg_t mouse_last_mesg;

char * mouse_messages[3] = {
        "",
        "Mouse Configured",
        "Mouse Reset",
};
/*! mouse_check_mesg -
 * @param curr_mesg -
 */
void mouse_check_mesg(mesg_t curr_mesg)
{
        RETURN_UNLESS(mouse_last_mesg != curr_mesg);
        mouse_last_mesg = curr_mesg;
        otg_message(mouse_messages[curr_mesg]);
}

/*!
 * mouse_event_handler() - process a device event
 * This function is called to process USB Device Events.
 *
 * The DEVICE_CONFIGURED event causes the mouse_fd_send() to be called
 * to start the data flow.
 *
 * The DEVICE_RESET or DEVICE_DE_CONFIGURED events cause the outstanding
 * transmit urb to be cancelled.
 *
 * @param function the function instance
 * @param event the event
 * @param data
 * @return void
 *
 */
void mouse_event_handler (struct usbd_function_instance *function, usbd_device_event_t event, int data)
{
        struct mouse_private *mouse = &mouse_private;

        switch (event) {
        case DEVICE_CONFIGURED:

                TRACE_MSG0(MOUSE, "Mouse Configured");
                mouse_check_mesg(mesg_configured);
                mouse_count = 0;
                mouse->connected = 1;
                if (!(mouse->tx_urb = usbd_alloc_urb (function, BULK_INT, 4, mouse_urb_sent)))
                        printk(KERN_INFO"%s: alloc failed\n", __FUNCTION__);
                mouse_fd_send(function);                                           // start sending
                break;

        case DEVICE_RESET:
                TRACE_MSG0(MOUSE, "Mouse Reset");
        case DEVICE_DE_CONFIGURED:
                TRACE_MSG0(MOUSE, "Mouse De-Configured");
                mouse_check_mesg(mesg_reset);
                BREAK_IF(!mouse->connected);
                mouse->connected = 0;
                if (mouse->tx_urb) {
                        usbd_free_urb (mouse->tx_urb);
                        mouse->tx_urb = NULL;
                }
                break;
        default:
                break;
        }
}

/*! copy_report()
 * This function copies the Mouse HID report into the provided URB.
 * @param urb Destination
 * @param data Source
 * @param size Amount of data to copy.
 * @param max_buf Size of buffer
 * @return Non-zero if error.
 *
 */
static int copy_report (struct usbd_urb *urb, void *data, int size, int max_buf)
{
        int available;
        int length;

        RETURN_EINVAL_IF (!urb);
        RETURN_EINVAL_IF (!data);
        RETURN_EINVAL_IF (!(length = size));
        RETURN_EINVAL_IF ((available = max_buf - urb->actual_length) <= 0);

        length = (length < available) ? length : available;
        memcpy (urb->buffer + urb->actual_length, data, length);
        urb->actual_length += length;
        return 0;
}

/*!
 * mouse_fd_send_hid() - send an EP0 urb containing HID report
 * This is called to send the Mouse HID report.
 */
static int mouse_fd_send_hid (struct usbd_function_instance *function)
{
        struct mouse_private *mouse = &mouse_private;
        struct usbd_urb *urb = usbd_alloc_urb_ep0(function, mouse->wLength, NULL);
        int wMaxPacketSize = usbd_endpoint_zero_wMaxPacketSize(urb->function_instance, 0);

        TRACE_MSG1(MOUSE, "Send Hid wLength: %d", mouse->wLength);
        RETURN_EINVAL_IF (copy_report(urb, MouseHIDReport, sizeof(MouseHIDReport), mouse->wLength));
        RETURN_EINVAL_UNLESS (wMaxPacketSize);

        if (!(urb->actual_length % wMaxPacketSize) && (urb->actual_length < mouse->wLength))
                urb->flags |= USBD_URB_SENDZLP;

        RETURN_ZERO_IF(!usbd_start_in_urb(urb));
        usbd_free_urb(urb);
        return -EINVAL;
}

#ifdef CONFIG_OTG_MOUSE_BH
/*!
 * mouse_fd_hid_bh() - Bottom half handler to send a HID report
 * @param data
 */
static void mouse_fd_hid_bh (void *data)
{
        struct usbd_function_instance *function = mouse_private.function;
        mouse_fd_send_hid(function);
}

/*! mouse_schedule_bh - schedule a call for mouse_fd_hid_bh
 * @param void
 */
static int mouse_schedule_bh (struct usbd_function_instance *function)
{
        TRACE_MSG0(MOUSE, "Scheduling");
        SET_WORK_ARG(&mouse_private.notification_bh, (void)function)
        return (!schedule_task (&mouse_private.notification_bh)) ? EINVAL : 0;
}
#endif /* CONFIG_OTG_MOUSE_BH */

/*!
 * mouse_device_request - called to indicate urb has been received
 * @param function
 * @param request
 */
int mouse_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        /* verify that this is a usb class request per cdc-mouse specification or a vendor request.
         * determine the request direction and process accordingly
         */

        switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {

        case USB_REQ_HOST2DEVICE:
        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_CLASS:
        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:
                return 0;

        case USB_REQ_DEVICE2HOST :
        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_CLASS:
        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_VENDOR:

                switch (request->bRequest) {
                case USB_REQ_GET_DESCRIPTOR:
                        switch (le16_to_cpu(request->wValue)>>8) {
                        case HID_REPORT:
                                mouse_private.wLength = request->wLength;
                                #ifdef CONFIG_OTG_MOUSE_BH
                                return mouse_schedule_bh(function);
                                #else
                                return mouse_fd_send_hid(function);
                                #endif
                        }
                default: break;
                }
                break;

        default:
                break;
        }
        return -EINVAL;
}


/*! mouse_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int mouse_function_enable (struct usbd_function_instance *function)
{
        struct mouse_private *mouse = &mouse_private;

        // XXX MODULE LOCK HERE
        mouse->n = 0;
        mouse->x = 0;
        mouse->y = 0;
        mouse->last_x = 0;
        mouse->last_y = 0;
        mouse->writesize = usbd_endpoint_wMaxPacketSize(function, BULK_INT, 0);
        return 0;
}

/*! mouse_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
static void mouse_function_disable (struct usbd_function_instance *function)
{
        struct mouse_private *mouse = &mouse_private;
        mouse->writesize = 0;
        // XXX MODULE UNLOCK HERE
}

static void mouse_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress)
{

        TRACE_MSG1(MOUSE,"CLEARED bEndpointAddress: %02x", bEndpointAddress);

}


/* ********************************************************************************************* */
#if !defined(OTG_C99)
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops;

/*! mouse_simple_driver - USB Device Core function driver definition
 */
struct usbd_simple_driver mouse_simple_driver;
/*! mouse_ops_init - initialize mouse ops
 */
void mouse_ops_init(void)
{
        /*! function_ops - operations table for the USB Device Core
         */
        ZERO(mouse_function_ops);
        mouse_function_ops.event_handler = mouse_event_handler;              /*! called for each USB Device Event */
        mouse_function_ops.device_request = mouse_device_request;          /*! called for each received device request */
        mouse_function_ops.function_enable = mouse_function_enable;         /*! called to enable the function driver */
        mouse_function_ops.function_disable = mouse_function_disable;       /*! called to disable the function driver */
        mouse_function_ops.endpoint_cleared = mouse_endpoint_cleared;

        /*! simple_driver - USB Device Core function driver definition
         */
        ZERO(mouse_simple_driver);
        mouse_simple_driver.driver.name = "mouse-random";                            /*! driver name */
        mouse_simple_driver.driver.fops = &mouse_function_ops;                             /*! operations table */
        mouse_simple_driver.device_description = &mouse_fd_device_description;   /*! mouse device description */
        mouse_simple_driver.bNumConfigurations =
                sizeof (mouse_fd_configuration_description) / sizeof (struct usbd_configuration_description);
        mouse_simple_driver.configuration_description =
                mouse_fd_configuration_description;    /*! mouse configuration description */
        //mouse_simple_driver.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID);
        //mouse_simple_driver.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID);
        //mouse_simple_driver.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE);


        mouse_simple_driver.interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description);
        mouse_simple_driver.interface_list = mouse_interfaces;
        mouse_simple_driver.endpointsRequested = ENDPOINTS;
        mouse_simple_driver.requestedEndpoints = mouse_endpoint_requests;
}
#else /* defined(OTG_C99) */
/*! struct usbd_function_operations mouse_function_ops  - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops = {
        .event_handler = mouse_event_handler,                     /*!< called for each USB Device Event */
        .device_request = mouse_device_request,           /*!< called for each received device request */
        .function_enable = mouse_function_enable,         /*!< called to enable the function driver */
        .function_disable = mouse_function_disable,       /*!< called to disable the function driver */
        .endpoint_cleared = mouse_endpoint_cleared,
};

/*! struct usbd_simple_driver mouse_simple_driver - USB Device Core function driver definition
 */
struct usbd_simple_driver mouse_simple_driver = {
        .driver = {
                .name = "mouse-random",
                .fops = &mouse_function_ops, },
        .device_description = &mouse_fd_device_description,   /*!< mouse device description */
        .bNumConfigurations = sizeof (mouse_fd_configuration_description) / sizeof (struct usbd_configuration_description),
        .configuration_description = mouse_fd_configuration_description,    /*!< mouse configuration description */
        .interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = mouse_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = mouse_endpoint_requests,
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

/*!
 * mouse_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the mouse driver with the USB Device software.
 *
 * If the CONFIG_OTG_MOUSE_BH option is enabled it will also setup the mouse
 * bottom half handler.
 *
 */
static int mouse_modinit (void)
{
        printk (KERN_INFO "%s: vendor_id: %04x product_id: %04x\n", __FUNCTION__, vendor_id, product_id);

        #if !defined(OTG_C99)
        mouse_global_init();
        mouse_ops_init();
        #endif /* defined(OTG_C99) */

        MOUSE = otg_trace_obtain_tag(NULL, "mouse-fd");
        TRACE_MSG2(MOUSE, "vendor_id: %04x product_id: %04x",vendor_id, product_id);

        mouse_fd_hid.wItemLength = cpu_to_le16(0x34);      // XXX mips compiler bug.....

        // register as usb function driver
        THROW_IF (usbd_register_simple_function (&mouse_simple_driver, "mouse",  NULL), error);
        TRACE_MSG0(MOUSE, "ok");
        #ifdef CONFIG_OTG_MOUSE_BH
        mouse_private.notification_bh.routine = mouse_fd_hid_bh;
        mouse_private.notification_bh.data = NULL;
        #endif
        CATCH(error) {
                otg_trace_invalidate_tag(MOUSE);
                return -EINVAL;
        }
        return 0;
}

module_init (mouse_modinit);

#if OTG_EPILOGUE
/*!
 * mouse_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 *
 * @param void
 * @return void
 */
static void mouse_modexit (void)
{
        #ifdef CONFIG_OTG_MOUSE_BH
        while (PENDING_WORK_ITEM(mouse_private.notification_bh)) {
                printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
        #endif
        usbd_deregister_simple_function (&mouse_simple_driver);

        otg_trace_invalidate_tag(MOUSE);
}

module_exit (mouse_modexit);
#endif


#endif /* defined(CONFIG_OTG_LNX) */
