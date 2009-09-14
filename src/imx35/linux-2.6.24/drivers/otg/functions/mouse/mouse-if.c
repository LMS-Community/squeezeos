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
 * otg/functions/mouse/mouse-if.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/mouse/mouse-if.c|20070810225217|38889
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
 * @defgroup MouseInterface Mouse Interface Function
 * @ingroup InterfaceFunctions
 */
/*!
 * @file otg/functions/mouse/mouse-if.c
 * @brief Mouse Interface Function Driver protocol implementation.
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
 *      - ep0 delayed CONTROL READ
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
 * The following configuration options are available:
 *
 *     - CONFIG_OTG_MOUSE_PACKETS is used to set the number of mouse
 *       data reports to send. Set to zero for a continous stream of data.
 *
 *     - CONFIG_OTG_MOUSE_INTERVAL sets the polling interval for the
 *       interrupt endpoint
 *
 *     - CONFIG_OTG_BH configures a separate bottom half handler to
 *       respond to HID device requests
 *
 *
 * @ingroup MouseInterface
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-hid.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>


#define MOUSE mouse_if_trace_tag
extern otg_tag_t MOUSE;

#ifdef CONFIG_OTG_MOUSE_BH
atomic_t mouse_if_bh_active;
#endif /* CONFIG_OTG_MOUSE_BH */

static int Mouse_Interfaces_Active = 0;

/*!@struct mouse_if_private  mouse-if.c otg/functions/mouse/mouse-if.c
 * @brief The mouse_if_private structure is used to collect all of the mouse driver
 * global variables into one place.
 */
struct mouse_if_private {

#ifdef CONFIG_OTG_MOUSE_BH
        struct WORK_STRUCT notification_bh;
#endif /* CONFIG_OTG_MOUSE_BH */

        u8 interface_index;
        u8 bEndpointAddress;

        unsigned int writesize;       /*!< packetsize * 4 */
        int x;
        int y;
        int last_x;
        int last_y;
        int n;
        int mouse_count;

        int wLength;
        u16 pending_report;
        u8 idle;
        u8 duration;
        u16 protocol;

        int mouse_interface_number;
};

/*!
 * @name Mouse Descriptors
 *
 * MouseHIDReport
 * MOUSE Configuration
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 *
 * @{
 */

/*!
 * MouseHIDReport
 * This is the HID report returned for the HID Device Request.
 * There is no pre-defined descriptor type for this.
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


#define BULK_INT        0x00    /*!< Interrupt endpoint number */
#define ENDPOINTS       0x01    /*!< Number of endpoints required */

/*! List of required endpoint numbers
 */
static u8 mouse_if_index[] = { BULK_INT, };

/*! List of requested endpoints
 */
#ifndef CONFIG_OTG_MOUSE_INTERVAL
#define CONFIG_OTG_MOUSE_INTERVAL 1
#endif /*CONFIG_OTG_MOUSE_INTERVAL*/
static struct usbd_endpoint_request mouse_if_endpoint_requests[ENDPOINTS+1] = {
        { BULK_INT, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT, 16, 64,  CONFIG_OTG_MOUSE_INTERVAL, },
        { 0, },
};


#if defined(CONFIG_OTG_NOC99)

/*! This is the HID desccriptor.
 */
static struct hid_descriptor mouse_if_hid;

/*! List of class descriptors
 */
static struct usbd_generic_class_descriptor *mouse_if_hid_descriptors[] = {
        (struct usbd_generic_class_descriptor *)&mouse_if_hid, };


/*! Interface Descriptions
 */
static struct usbd_alternate_description mouse_if_data_alternate_descriptions[1];

/*! List of Interface description(s)
 */
static struct usbd_interface_description mouse_interfaces[1];



/*! mouse_if_global_init -
 */
void mouse_if_global_init(void)
{
        /*! This is the HID desccriptor.
         */
        ZERO(mouse_if_hid);
        mouse_if_hid.bLength = 0x09;
        mouse_if_hid.bDescriptorType = HID_DT_HID;
        mouse_if_hid.bcdHID = __constant_cpu_to_le16(0x101);
        mouse_if_hid.bCountryCode = 0x00;
        mouse_if_hid.bNumDescriptors = 0x01;
        mouse_if_hid.bReportType = HID_DT_REPORT;
        mouse_if_hid.wItemLength = __constant_cpu_to_le16(sizeof(MouseHIDReport));


        /*! Interface Descriptions
         */
        ZERO(mouse_if_data_alternate_descriptions);

        mouse_if_data_alternate_descriptions[0].iInterface = "Random Mouse Interface - Interrupt";
        //mouse_if_data_alternate_descriptions[0].interface_descriptor =  &mouse_if_data_alternate_descriptor;
        mouse_if_data_alternate_descriptions[0].classes =
                sizeof (mouse_if_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *);
        mouse_if_data_alternate_descriptions[0].class_list = mouse_if_hid_descriptors;
        mouse_if_data_alternate_descriptions[0].endpoints = 0x1;
        mouse_if_data_alternate_descriptions[0].endpoint_index = mouse_if_index;

        mouse_if_data_alternate_descriptions[0].bInterfaceClass = 0x03;
        mouse_if_data_alternate_descriptions[0].bInterfaceSubClass = 0x01;
        mouse_if_data_alternate_descriptions[0].bInterfaceProtocol = 0x02;

        /*! List of Interface description(s)
         */
        ZERO(mouse_interfaces);
        mouse_interfaces[0].alternates = sizeof (mouse_if_data_alternate_descriptions) / sizeof (struct usbd_alternate_description);
        mouse_interfaces[0].alternate_list = mouse_if_data_alternate_descriptions;
}
#else /* defined(CONFIG_OTG_NOC99) */
/*!
 * mouse_if_hid
 * This is the HID desccriptor.
 */
struct hid_descriptor mouse_if_hid = {
        .bLength = 0x09,
        .bDescriptorType = HID_DT_HID,
        .bcdHID = __constant_cpu_to_le16(0x101),

        .bCountryCode = 0x00,
        .bNumDescriptors = 0x01,
        .bReportType = HID_DT_REPORT,

        .wItemLength = __constant_cpu_to_le16(sizeof(MouseHIDReport)),
};

/*!
 * mouse_if_hid_descriptors
 * List of class descriptors
 */
static struct usbd_generic_class_descriptor *mouse_if_hid_descriptors[] = {
        (struct usbd_generic_class_descriptor *)&mouse_if_hid, };

/*!
 * mouse_if_data_alternate_descriptions
 * Interface Descriptions
 */
static struct usbd_alternate_description mouse_if_data_alternate_descriptions[] = {
        {
                /* This string is 31 bytes long which will result in 64 bytes string
                 * useful for testing zlp device requests
                 *
                 *             0123456789abcdef0123456789abcdef
                 *             |                             |
                 */
                .iInterface = "Belcarra Random Mouse Interface",
                .bInterfaceClass = 0x03,
                .bInterfaceSubClass = 0x01,
                .bInterfaceProtocol = 0x02,
                .classes = sizeof (mouse_if_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  mouse_if_hid_descriptors,
                .endpoints = sizeof (mouse_if_index) / sizeof(u8),
                .endpoint_index =  mouse_if_index,
        },
};

/*!
 * mouse_interfaces
 * List of Interface description(s)
 */
static struct usbd_interface_description mouse_interfaces[] = {
      { .alternates = sizeof (mouse_if_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = mouse_if_data_alternate_descriptions,},
};

#endif /* defined(CONFIG_OTG_NOC99) */

/*! @} */

/* MOUSE ***************************************************************************************** */
/*!
 * @name Transmit Function
 *
 * @{
 */

/*!
 * get_xy
 * Random walk array
 */
static int get_xy[8] = {
         0,  0,  1,  1,
         0,  0, -1, -1,
};



extern void mouse_if_send(struct usbd_function_instance *function);

/*!
 * @brief mouse_urb_sent() - called to indicate URB transmit finished
 * This function is the callback function for sent urbs.
 *
 * It will continue to queue new urbs  until either there is an error or the
 * maximum count is exceeded.
 *
 * @param tx_urb The urb to be sent.
 * @param rc result
 * @return int Return non-zero for failure.
 */
static int mouse_urb_sent (struct usbd_urb *tx_urb, int rc)
{
        struct usbd_function_instance *function = tx_urb->function_instance;
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;

        TRACE_MSG4(MOUSE, "mouse_count[%d]: count: %d status: %d state: %d",
                        interface_instance->wIndex, mouse->mouse_count,
                        usbd_get_device_status(function), usbd_get_device_state(function));
        // return non-zero to get tx_urb deallocated
        RETURN_EINVAL_IF(usbd_get_device_status(function) == USBD_CLOSING);
        RETURN_EINVAL_IF(usbd_get_device_status(function) != USBD_OK);
        RETURN_EINVAL_IF(usbd_get_device_state(function) != STATE_CONFIGURED);

        //TRACE_MSG2(MOUSE, "mouse_count[%d]: %d", interface_instance->wIndex, mouse->mouse_count);
        usbd_free_urb(tx_urb);
        #ifdef CONFIG_OTG_MOUSE_PACKETS
        RETURN_ZERO_IF (CONFIG_OTG_MOUSE_PACKETS && (mouse->mouse_count++ > CONFIG_OTG_MOUSE_PACKETS));
        #endif /* CONFIG_OTG_MOUSE_PACKETS */
        mouse_if_send(function);                // re-start
        return 0;
}
/*!
 * mouse_if_send() - send a mouse data urb with random data
 *
 * Queue a new data urb to send small mouse movement to host.
 *
 * @param function
 * @return void
 */
void mouse_if_send(struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        int new_x = 0;
        int new_y = 0;
        u8 random;

        struct usbd_urb *tx_urb;

        tx_urb = usbd_alloc_urb (function, BULK_INT, 4, mouse_urb_sent);
        RETURN_UNLESS(tx_urb);

        memset(tx_urb->buffer, 0, 4);

        /* when n is zero, generate new directional movement, and new n, from random data */
        UNLESS (mouse->n) {
                otg_get_random_bytes(&random, 1);

                mouse->last_x = MAX(-4, MIN(4, mouse->last_x + get_xy[random & 0x7]));
                mouse->last_y = MAX(-4, MIN(4, mouse->last_y + get_xy[(random >> 3) & 0x7]));
                mouse->n = (random>>6) & 0x3;

                new_x = mouse->x + mouse->last_x;
                new_y = mouse->y + mouse->last_y;

                if (Mouse_Interfaces_Active > 1) {
                        if (mouse->mouse_interface_number & 0x1)
                                tx_urb->buffer[1] = mouse->last_x;
                        else
                                tx_urb->buffer[2] = mouse->last_y;
                }
                else {
                        tx_urb->buffer[1] = mouse->last_x;
                        tx_urb->buffer[2] = mouse->last_y;

                }
                mouse->x = new_x;
                mouse->y = new_y;
        }
        /* when n is odd, simply send zero's */
        else if (mouse->n & 0x1) {
                mouse->n--;
        }
        /* when n is non-zero and not odd, send directional movement */
        else {
                mouse->n--;
                tx_urb->buffer[1] = mouse->last_x;
                tx_urb->buffer[2] = mouse->last_y;
                mouse->x = new_x;
                mouse->y = new_y;
        }
        tx_urb->actual_length = 4;
        TRACE_MSG0(MOUSE, "STARTING");
        RETURN_UNLESS(usbd_start_in_urb(tx_urb));
        TRACE_MSG0(MOUSE, "FAILED");
        usbd_free_urb(tx_urb);
}

/*! @} */

/* USB Device Functions ************************************************************************ */


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

        TRACE_MSG3(MOUSE, "max_buf: %d actual: %d available: %d", max_buf, urb->actual_length, available);
        length = (length < available) ? length : available;
        memcpy (urb->buffer + urb->actual_length, data, length);
        urb->actual_length += length;
        return 0;
}

/*!
 * mouse_if_send_hid_descriptor() - send an EP0 urb containing HID descriptor
 * This is called to send the Mouse HID report.
 *
 * This will satisfy a device request from the host.
 *
 * @param function  - pointer to function instance
 * @return int
 */
static int mouse_if_send_hid_descriptor (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        struct usbd_urb *urb = usbd_alloc_urb_ep0(function, mouse->wLength, NULL);
        int hs = usbd_high_speed(function);
        int wMaxPacketSize = usbd_endpoint_zero_wMaxPacketSize(function, hs);

        TRACE_MSG2(MOUSE, "Send Hid wLength: %d wMaxPacketSize: %d", mouse->wLength, wMaxPacketSize);
        TRACE_MSG0(MOUSE, "AAAA");
        RETURN_EINVAL_IF (copy_report(urb, (u8 *)&mouse_if_hid, sizeof(mouse_if_hid), mouse->wLength));
        TRACE_MSG1(MOUSE, "BBBB actual: %d", urb->actual_length);
        RETURN_EINVAL_UNLESS (wMaxPacketSize);
        TRACE_MSG0(MOUSE, "BBBB");

        if (!(urb->actual_length % wMaxPacketSize) && (urb->actual_length < mouse->wLength))
                urb->flags |= USBD_URB_SENDZLP;

        RETURN_ZERO_IF(!usbd_start_in_urb(urb));
        TRACE_MSG0(MOUSE, "CCCC");
        usbd_free_urb(urb);
        return -EINVAL;
}

/*!
 * mouse_if_send_hid_report() - send an EP0 urb containing HID report
 * This is called to send the Mouse HID report.
 *
 * This will satisfy a device request from the host.
 *
 * @param function  - pointer to function instance
 * @return int
 */
static int mouse_if_send_hid_report (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        struct usbd_urb *urb = usbd_alloc_urb_ep0(function, mouse->wLength, NULL);
        int hs = usbd_high_speed(function);
        int wMaxPacketSize = usbd_endpoint_zero_wMaxPacketSize(function, hs);

        TRACE_MSG1(MOUSE, "Send Hid wLength: %d", mouse->wLength);
        TRACE_MSG0(MOUSE, "AAAA");
        RETURN_EINVAL_IF (copy_report(urb, MouseHIDReport, sizeof(MouseHIDReport), mouse->wLength));
        TRACE_MSG0(MOUSE, "BBBB");
        RETURN_EINVAL_UNLESS (wMaxPacketSize);
        TRACE_MSG0(MOUSE, "BBBB");

        if (!(urb->actual_length % wMaxPacketSize) && (urb->actual_length < mouse->wLength))
                urb->flags |= USBD_URB_SENDZLP;

        RETURN_ZERO_IF(!usbd_start_in_urb(urb));
        TRACE_MSG0(MOUSE, "CCCC");
        usbd_free_urb(urb);
        return -EINVAL;
}

#ifdef CONFIG_OTG_MOUSE_BH
/*!
 * @brief mouse_if_hid_bh() - Bottom half handler to send a HID report
 * Bottom half handler.
 *
 * Ssne device request response from bottom half. This tests
 * delayed device request IN responses.
 *
 * @param data
 * @return none
 */
static void mouse_if_hid_bh (void *data)
{
        struct usbd_function_instance *function = (struct usbd_function_instance *)data;
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        mouse_if_send_hid_report(function);
        atomic_inc(&mouse_if_bh_active);
}

/*! mouse_schedule_bh - schedule a call for mouse_if_hid_bh
 * This will schedule the mouse bottom half handler.
 *
 * @param void
 * @return 0 on success
 */
static int mouse_schedule_bh (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;

        TRACE_MSG0(MOUSE, "Scheduling");
        TRACE_MSG0(MOUSE, "AAAA");
        PREPARE_WORK_ITEM(mouse->notification_bh, mouse_if_hid_bh, (void *)function);
        atomic_set(&mouse_if_bh_active, 1);
        SCHEDULE_WORK(mouse->notfication_bh);
        TRACE_MSG0(MOUSE, "BBBB");
        return 0;
}
#endif /* CONFIG_OTG_MOUSE_BH */

/*!
 * @brief mouse_report_received() - called to indicate URB transmit finished
 * This function is the callback function for sent urbs.
 * It simply queues up another urb until the packets to be sent
 * configuration parameter is reached (or forever if zero.)
 * @param rx_urb The urb to be sent.
 * @param rc result
 * @return int Return non-zero for failure.
 */
int mouse_report_received (struct usbd_urb *rx_urb, int rc)
{
        struct usbd_function_instance *function;
        struct usbd_interface_instance *interface_instance;
        struct mouse_if_private *mouse;
        function = rx_urb->function_instance;
        interface_instance = (struct usbd_interface_instance *)function;
        mouse = interface_instance->function.privdata;
        TRACE_MSG3(MOUSE, "REPORT[%02x]: len: %d buf[0]: %x",
                        mouse->pending_report, rx_urb->actual_length, rx_urb->buffer[0]);
        usbd_free_urb(rx_urb);
        mouse->pending_report = 0;
        return 0;
}


/*!
 * mouse_if_device_request - called to process a request to endpoint or interface
 * Process a device request.
 *
 * The HID host sends various device requests. We only support a limited
 * set.
 *
 * @param function
 * @param request
 * @return non-zero for failure, will cause endpoint zero stall
 */
static int mouse_if_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        struct usbd_urb *urb = NULL;

        TRACE_MSG6(MOUSE, "Interface[%02d] bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x wLength: %04x",
                        interface_instance->wIndex, request->bmRequestType, request->bRequest, request->wValue,
                        request->wIndex, request->wLength);

        /* HID only sends requests to interface.
         */
        TRACE_MSG0(MOUSE, "CHECK RECIP");
        RETURN_EINVAL_UNLESS(USB_REQ_RECIPIENT_INTERFACE == (request->bmRequestType & USB_REQ_RECIPIENT_MASK));


        TRACE_MSG0(MOUSE, "RECIP OK");

        switch (request->bmRequestType & USB_REQ_DIRECTION_MASK) {
        case USB_REQ_DEVICE2HOST:
                TRACE_MSG0(MOUSE, "DEVICE2HOST");
                switch (request->bRequest) {
                case USB_REQ_GET_DESCRIPTOR:
                        TRACE_MSG0(MOUSE, "GET DESCRIPTOR");
                        switch (le16_to_cpu(request->wValue)>>8) {
                        case HID_DT_HID:
                                mouse->wLength = request->wLength;
                                return mouse_if_send_hid_descriptor(function);

                        case HID_DT_REPORT:
                                TRACE_MSG0(MOUSE, "GET DESCRIPTOR HID REPORT");
                                mouse->wLength = le16_to_cpu(request->wLength);
                                #ifdef CONFIG_OTG_MOUSE_BH
                                return mouse_schedule_bh(function);
                                #else
                                return mouse_if_send_hid_report(function);
                                #endif
                        default:
                                TRACE_MSG0(MOUSE, "GET DESCRIPTOR HID UNKNOWN");
                                break;
                        }
                        return -EINVAL;
                case USB_REQ_GET_REPORT:
                        TRACE_MSG0(MOUSE, "GET REPORT"); break;
                case USB_REQ_GET_IDLE:
                        TRACE_MSG0(MOUSE, "GET IDLE"); break;
                case USB_REQ_GET_PROTOCOL:
                        TRACE_MSG0(MOUSE, "GET PROTOCOL"); break;
                        break;
                default:
                        TRACE_MSG0(MOUSE, "GET DEFAULT"); break;
                        return -EINVAL;
                }

                RETURN_EINVAL_UNLESS(request->wLength &&
                                ((urb = usbd_alloc_urb_ep0 (function, request->wLength, NULL))));

                switch (request->bRequest) {

                case USB_REQ_GET_REPORT:

                        switch (le16_to_cpu(request->wValue) >> 8) {
                        case HID_INPUT:
                                TRACE_MSG0(MOUSE, "GET REPORT HID_INPUT"); break;
                        case HID_OUTPUT:
                                TRACE_MSG0(MOUSE, "GET REPORT HID_OUTPUT"); break;
                        case HID_FEATURE:
                                TRACE_MSG0(MOUSE, "GET REPORT HID_FEATURE"); break;
                                break;
                        default:
                                TRACE_MSG0(MOUSE, "GET REPORT default"); break;
                                break;
                        }
                        // XXX create urb and send?
                        urb->actual_length = 1;
                        break;

                case USB_REQ_GET_IDLE:
                case USB_REQ_GET_PROTOCOL:
                        urb->actual_length = 1;
                        switch (request->bRequest) {
                        case USB_REQ_GET_IDLE:
                                urb->buffer[0] = cpu_to_le16(mouse->idle);
                                TRACE_MSG1(MOUSE, "GET IDLE: %x", urb->buffer[0]);
                                break;
                        case USB_REQ_GET_PROTOCOL:
                                urb->buffer[0] = cpu_to_le16(mouse->protocol);
                                TRACE_MSG1(MOUSE, "GET PROTOCOL: %x", urb->buffer[0]);
                                break;
                        }
                }

                RETURN_ZERO_UNLESS(usbd_start_in_urb(urb));

                /* only get here if error */
                usbd_free_urb(urb);
                TRACE_MSG0(MOUSE, "get failed");
                break;

        case USB_REQ_HOST2DEVICE:
                TRACE_MSG0(MOUSE, "HOST2DEVICE");
                switch (request->bRequest) {
                case USB_REQ_SET_REPORT:
                        TRACE_MSG0(MOUSE, "SET REPORT");
                        // Sample: 21 09 00 02 00 00 01 00
                        mouse->pending_report = le16_to_cpu(request->wValue);
                        RETURN_EINVAL_UNLESS(request->wLength &&
                                        (urb = usbd_alloc_urb_ep0(function, request->wLength, mouse_report_received)));

                        RETURN_ZERO_UNLESS (usbd_start_out_urb (urb));
                        /* failure */
                        usbd_free_urb (urb);
                        mouse->pending_report = 0;
                        return -EINVAL;

                case USB_REQ_SET_IDLE:
                        TRACE_MSG0(MOUSE, "SET IDLE");
                        mouse->idle = le16_to_cpu(request->wValue) >> 8;
                        mouse->duration = le16_to_cpu(request->wValue) & 0xff;
                        TRACE_MSG2(MOUSE, "SET IDLE: idle: %02x duration: %02x", mouse->idle, mouse->duration);
                        return 0;

                case USB_REQ_SET_PROTOCOL:
                        TRACE_MSG0(MOUSE, "SET PROTOCOL");
                        mouse->protocol = le16_to_cpu(request->wValue);
                        TRACE_MSG1(MOUSE, "SET PROTOCOL: %x", request->wValue);
                        return 0;
                default:
                        TRACE_MSG0(MOUSE, "SET UNKNOWN");
                        break;
                }
                break;
        }
        return -EINVAL;
}

/*!
 * @brief mouse_if_set_configuration - device configured
 * Called when the host performs a set configuration.
 *
 * This will start sending data.
 *
 * @param function
 * @param configuration
 * @return int
 */
static int mouse_if_set_configuration (struct usbd_function_instance *function, int configuration)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        int hs = usbd_high_speed(function);


        TRACE_MSG3(MOUSE, "SET_CONFIGURATION MOUSE_IF[%d] %x cfg: %d ", interface_instance->wIndex, function, configuration);

        // XXX Need to differentiate between non-zero, zero and non-zero done twice

        function->privdata = mouse;;

        mouse->n = mouse->x = mouse->y = mouse->last_x = mouse->last_y = 0;

        mouse->bEndpointAddress = usbd_endpoint_bEndpointAddress(function, BULK_INT, hs);
        mouse->writesize = usbd_endpoint_wMaxPacketSize(function, BULK_INT, hs);

        mouse->mouse_count = 0;

        TRACE_MSG4(MOUSE, "MOUSE_IF[%2d] Configured: %d bEndpointAddress: %02x size: %02x",
                        interface_instance->wIndex, configuration, mouse->bEndpointAddress, mouse->writesize);

        mouse_if_send(function);                                           // start sending

        return 0;
}

/*!
 * @brief  mouse_if_reset - called to indicate bus has been reset
 * @param function_instance
 * @return int
 */
int mouse_if_reset (struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(MOUSE,"RESET");
	return 0;
}


/*!
 * @brief mouse_if_suspended - called to indicate bus has been suspended
 * @param function_instance
 * @return int
 */
int mouse_if_suspended (struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(MOUSE,"SUSPENDED");
	return 0;
}



/*!
 * @brief mouse_if_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * Clear an endpoint.
 *
 * @param function The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */
static void mouse_if_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress)
{
        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        //struct mouse_if_private *mouse = interface_instance->function.privdata;
        TRACE_MSG1(MOUSE,"CLEARED bEndpointAddress: %02x", bEndpointAddress);
        mouse_if_send(function);                                           // re-start sending
}



/* ********************************************************************************************* */
/*! mouse_if_function_enable - called by USB Device Core to enable the driver
 * Called by the USBD core layer to enable this driver.
 *
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int mouse_if_function_enable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = NULL;
        int hs = usbd_high_speed(function);

        RETURN_EINVAL_UNLESS((mouse = CKMALLOC(sizeof(struct mouse_if_private))));
        // XXX MODULE LOCK HERE
        interface_instance->function.privdata = (void *)mouse;
        mouse->writesize = usbd_endpoint_wMaxPacketSize(function, BULK_INT, hs);
        mouse->mouse_interface_number = Mouse_Interfaces_Active++;
        return 0;
}

/*! mouse_if_function_disable - called by the USB Device Core to disable the driver
 * Called by the USBD core layer to disable this driver.
 *
 * @param function The function instance for this driver
 */
static void mouse_if_function_disable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        interface_instance->function.privdata = NULL;
        Mouse_Interfaces_Active--;
        LKFREE(mouse);
        // XXX MODULE UNLOCK HERE
}

/* ********************************************************************************************* */
const char *mouse_usb_strings[4] = {
        "Mouse String Test 2",
        "Mouse String Test 4",
        "Mouse String Test 6",
        NULL,
};
u8 mouse_usb_string_indexes[4] = {
        2, 4, 6, 0,
};

#if defined(CONFIG_OTG_NOC99)
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops;

/*! mouse_interface_driver - USB Device Core function driver definition
 */
struct usbd_interface_driver mouse_interface_driver;

/*! mouse_if_ops_init - initialize mouse interface operations
 * @param void
 * @return none
 */
void mouse_if_ops_init(void)
{
        /*! function_ops - operations table for the USB Device Core
         */

        ZERO(mouse_function_ops);
        mouse_function_ops.set_configuration = mouse_if_set_configuration;
        mouse_function_ops.device_request = mouse_if_device_request;          /*! called for each received device request */
        mouse_function_ops.endpoint_cleared = mouse_if_endpoint_cleared;

        mouse_function_ops.function_enable = mouse_if_function_enable;
        mouse_function_ops.function_disable = mouse_if_function_disable;

        /*! interface_driver - USB Device Core function driver definition
         */
        ZERO(mouse_interface_driver);
        mouse_interface_driver.driver.name = "mouse-random-if";                            /*! driver name */
        mouse_interface_driver.driver.fops = &mouse_function_ops;                             /*! operations table */
        mouse_interface_driver.driver.usb_strings = mouse_usb_strings;                 /*! operations table */
        mouse_interface_driver.driver.usb_string_indexes = mouse_usb_string_indexes;   /*! operations table */

        mouse_interface_driver.interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description);
        mouse_interface_driver.interface_list = mouse_interfaces;
        mouse_interface_driver.endpointsRequested = ENDPOINTS;
        mouse_interface_driver.requestedEndpoints = mouse_if_endpoint_requests;
        mouse_interface_driver.iFunction = "Belcarra Random Mouse - 1.0 HID";

}

#else /* defined(CONFIG_OTG_NOC99) */
/*! function_ops - operations table for the USB Device Core
 * The set of interface function operations supported by this driver.
 */
static struct usbd_function_operations mouse_function_ops = {
        .set_configuration = mouse_if_set_configuration,
        .device_request = mouse_if_device_request,           /*!< called for each received device request */
        .endpoint_cleared = mouse_if_endpoint_cleared,
        .function_enable = mouse_if_function_enable,
        .function_disable = mouse_if_function_disable,
};

/*! mouse_interface_driver - USB Device Core function driver definition
 * The interface function configuration for this driver.
 */
struct usbd_interface_driver mouse_interface_driver = {
        .driver = {
                .name = "mouse-random-if",
                .fops = &mouse_function_ops,
                .usb_strings = mouse_usb_strings,
                .usb_string_indexes = mouse_usb_string_indexes,
        },
        .interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = mouse_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = mouse_if_endpoint_requests,

        .bFunctionClass = 0x03,
        .bFunctionSubClass = 0x01,
        .bFunctionProtocol = 0x02,
        .iFunction = "Belcarra Random Mouse - 1.0 HID",
};
#endif /* defined(CONFIG_OTG_NOC99) */


/*! Module Parameters ******************************************************** */

/*! @name XXXXX MODULE Parameters
 */
/*@{*/

MOD_AUTHOR ("sl@belcarra.com");
EMBED_LICENSE();
MOD_DESCRIPTION ("Belcarra Random Walk MOUSE Function");

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);

otg_tag_t MOUSE;

/*! @} */

/*! @name USB Module init/exit ***************************************************** */

 /*! @{
 */

/*!
 * mouse_if_modinit() - module init
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
 */
int mouse_if_modinit (void)
{
        #if defined(CONFIG_OTG_NOC99)
        mouse_if_global_init();
        mouse_if_ops_init();
        #endif /* defined(CONFIG_OTG_NOC99) */

        MOUSE = otg_trace_obtain_tag(NULL, "mouse-if");
        TRACE_MSG2(MOUSE, "vendor_id: %04x product_id: %04x", MODPARM(vendor_id), MODPARM(product_id));

        mouse_if_hid.wItemLength = cpu_to_le16(0x34);      // XXX mips compiler bug.....

        // register as usb function driver
        TRACE_MSG0(MOUSE, "REGISTER INTERFACE");
        TRACE_MSG2(MOUSE, "%s %d", mouse_interface_driver.driver.name, mouse_interface_driver.endpointsRequested);

        THROW_IF (usbd_register_interface_function ( &mouse_interface_driver, "mouse-if", NULL), error);

        TRACE_MSG2(MOUSE, "%s %d", mouse_interface_driver.driver.name, mouse_interface_driver.endpointsRequested);

        TRACE_MSG0(MOUSE, "REGISTER FINISHED");
        #ifdef CONFIG_OTG_MOUSE_BH
        mouse_if_private.notification_bh.routine = mouse_if_hid_bh;
        mouse_if_private.notification_bh.data = NULL;
        #endif
        CATCH(error) {
                otg_trace_invalidate_tag(MOUSE);
                return -EINVAL;
        }
        return 0;
}

module_init (mouse_if_modinit);

/*!
 * mouse_if_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 *
 * @return void
 */
void mouse_if_modexit (void)
{
        #ifdef CONFIG_OTG_MOUSE_BH
        while (atomic_read(&mouse_if_bh_active)) {
                printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
        #endif
        usbd_deregister_interface_function (&mouse_interface_driver);
        otg_trace_invalidate_tag(MOUSE);
}

#if OTG_EPILOGUE
module_exit (mouse_if_modexit);
#endif
/*! @} */
