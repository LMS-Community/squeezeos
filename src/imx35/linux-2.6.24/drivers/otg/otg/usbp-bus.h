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
 * otg/otg/usbp-bus.h - USB Device Bus Interface Driver Interface
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/otg/usbp-bus.h|20070810200302|45685
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/otg/usbp-bus.h
 * @brief Bus interface Defines for USB Device Core Layaer
 *
 * This file contains the USB Peripheral Driver definitions.
 *
 * This is the interface between the bottom of the USB Core and the top of
 * the Bus Interace Drivers and is comprised of:
 *
 *      o public functions exported by the USB Core layer
 *
 *      o structures and functions passed by the Function Driver to the USB
 *      Core layer
 *
 * USB Peripheral Controller Drivers are structured such that the upper edge
 * implements interfaces to the USB Peripheral Core layer to provide low
 * level USB services and the lower edge interfaces to the actual USB
 * Hardware (typically called the USB Device Controller or UDC.)
 *
 * The peripheral controller layer primarily deals with endpoints.
 * There is one control endpoint and one or more data endpoints.
 *
 * The control endpoint is special, with received data processed with
 * the built in EP0 function. Which in turn will pass requests to
 * interface functions as appropriate.
 *
 * Data endpoints are controlled by interface driver and have a
 * function callback specified by the interface driver to perform
 * whatever needs to be done when data is sent or received.
 *
 *
 * @ingroup USBDAPI
 */

/*
struct usbd_endpoint_map;
struct usbd_endpoint_instance;
struct usbd_bus_instance;
struct usbd_urb;
*/

#define USBD usbd_trace_tag
extern otg_tag_t USBD;

/*!
 * USB Bus Interface Driver structures
 *
 * Driver description:
 *
 *  struct usbd_bus_operations
 *  struct usbd_bus_driver
 *
 */

extern int usbd_maxstrings;


/*!
 * exported to otg
 */
struct usbd_ops {
        int (*admin) (char *);
        char * (*function_name) (int);
};



/*! @struct usbd_bus_operations  usbp-bus.h  "otg/usbp-bus.h"
 *
 *  @brief Operations that the slave layer or function driver can use to interact with the bus interface driver.
 * The send_urb() function is used by the usb-device endpoint 0 driver and
 * function drivers to submit data to be sent.
 *
 * The cancel_urb() function is used by the usb-device endpoint 0 driver and
 * function drivers to remove previously queued data to be sent.
 *
 * The endpoint_halted() function is used by the ep0 control function to
 * check if an endpoint is halted.
 *
 * The endpoint_feature() function is used by the ep0 control function to
 * set/reset device features on an endpoint.
 *
 * The device_event() function is used by the usb device core to tell the
 * bus interface driver about various events.
 */
struct usbd_bus_operations {
        //int (*xbus_serial_number) (struct usbd_bus_instance *, char *);

        int (*start_endpoint_in) (struct usbd_bus_instance *, struct usbd_endpoint_instance *);
        int (*start_endpoint_out) (struct usbd_bus_instance *, struct usbd_endpoint_instance *);
        int (*cancel_urb_irq) (struct usbd_urb *);
        int (*device_feature) (struct usbd_bus_instance *, int, int);
        int (*endpoint_halted) (struct usbd_bus_instance *, int);
        int (*halt_endpoint) (struct usbd_bus_instance *, int, int);
        int (*set_configuration) (struct usbd_bus_instance *, int);
        int (*set_address) (struct usbd_bus_instance *, int);
        int (*event_handler) (struct usbd_bus_instance *, usbd_device_event_t, int);
        int (*request_endpoints) (struct usbd_bus_instance *, struct usbd_endpoint_map *,
                        int, struct usbd_endpoint_request *);
        int (*set_endpoints) (struct usbd_bus_instance *, int , struct usbd_endpoint_map *);
        int (*framenum) (struct usbd_bus_instance *);
        //u64 (*ticks) (void);
        //u64 (*elapsed) (u64 *, u64 *);
        //u64 (*interrupts) (void);
};

/*! @struct usbd_endpoint_instance  usbp-bus.h "otg/usbp-bus.h"
 *  @brief  Endpoint operation related data wrapper
 *
 * Per endpoint configuration data. Used to track which actual physical
 * endpoints.
 *
 * The bus layer maintains an array of these to represent all physical
 * endpoints.
 *
 */
struct usbd_endpoint_instance {
        int                             interface_wIndex;       // which interface owns this endpoint
        int                             physicalEndpoint[2];    // physical endpoint address - bus interface specific
        u8                              bEndpointAddress[2];    // logical endpoint address
        u8                              bmAttributes[2];        // endpoint type
        u16                             wMaxPacketSize[2];      // packet size for requested endpoint
        u16                             transferSize[2];        // packet size for requested endpoint

        u32                             feature_setting;        // save set feature information

        // control
        int                             state;                  // available for use by bus interface driver


        otg_pthread_mutex_t             mutex;
        struct urb_link                 rdy;                    // empty urbs ready to receive

        union {
                struct usbd_urb        *active_urb;             // active urb
                struct usbd_urb        *rcv_urb;                // alias
                struct usbd_urb        *tx_urb;                 // alias
        };

        u32                             rcv_transferSize;       // maximum transfer size from function driver
        int                             rcv_error;              // current bulk-in has an error
        u32                             tx_transferSize;        // maximum transfer size from function driver

        u32                             sent;                   // data already sent
        u32                             last;                   // data sent in last packet XXX do we need this
        struct timer_list               pcd_hr_timer;
        void                            *privdata;

        struct usbd_bus_instance        *bus;

        void                            *buffer;
        dma_addr_t                      dma;

};

/*! @name endpoint zero states
 * @{
 */
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4
#define DATA_STATE_PENDING_XMIT 5
#define WAIT_FOR_IN_STATUS      6
/*! @} */

/*! @struct usbd_bus_driver usbp-bus.h  "otg/usbp-bus.h"
 * @brief  Bus Interface data structure
 *
 * Keep track of specific bus interface.
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about each real bus interface found such that the
 * usb-device layer can create and maintain a usb-device structure.
 *
 * Note that bus interface registration is incumbent on finding specific
 * actual real bus interfaces. There will be a registration for each such
 * device found.
 *
 * The max_tx_endpoints and max_rx_endpoints are the maximum number of
 * possible endpoints that this bus interface can support. The default
 * endpoint 0 is not included in these counts.
 *
 */
struct usbd_bus_driver {
        char                            *name;
        u8                              max_endpoints;  // maximimum number of rx enpoints
        u8                              maxpacketsize;
        u8                              high_speed_capable;
        struct usbd_device_description   *device_description;
        struct usbd_bus_operations       *bops;
        void                            *privdata;
        //u32                             capabilities;
        u8                              bmAttributes;
        u8                              bMaxPower;
        u8                              ports;
};

#if 0
/*!
 * Bus state
 *
 * enabled or disabled
 */
typedef enum usbd_bus_state {
        usbd_bus_state_unknown,
        usbd_bus_state_disabled,
        usbd_bus_state_enabled
} usbd_bus_state_t;
#endif

/*!
 * @name UDC Capabilities
 * @{
 */
#define HAVE_CABLE_IRQ                  0x0001
#define REMOTE_WAKEUP_SUPPORTED         0x0002
#define ROOT_HUB                        0x0004

#define USB_SPEED_FULL                  0x0
#define USB_SPEED_HIGH                  0x1
#define USB_SPEED_UNKNOWN               0x2

/*! @} */

/*! @struct usbd_bus_instance usbp-bus.h "otg/usbp-bus.h"
 *  @brief  Bus Interface configuration structure
 *
 * This is allocated for each configured instance of a bus interface driver.
 *
 * It contains a pointer to the appropriate bus interface driver.
 *
 * The privdata pointer may be used by the bus interface driver to store private
 * per instance state information.
 */
struct usbd_bus_instance {

        struct otg_instance             *otg;
        struct usbd_bus_driver          *driver;

        int                             endpoints;
        struct usbd_endpoint_instance   *endpoint_array;        // array of available configured endpoints

        struct urb_link                 finished;               // processed urbs

        usbd_device_status_t            status;                 // device status
        //usbd_bus_state_t                bus_state;
        usbd_device_state_t             device_state;           // current USB Device state
        usbd_device_state_t             suspended_state;        // previous USB Device state

        struct usbd_interface_instance  *ep0;                   // ep0 configuration
        struct usbd_function_instance   *function_instance;

        u8                              high_speed;

        u32                             device_feature_settings;// save set feature information
        u8                              bmAttributes;
        u8                              bMaxPower;

        //struct WORK_STRUCT              reset_bh;               // runs as bottom half, equivalent to interrupt time
        //struct WORK_STRUCT              resume_bh;              // runs as bottom half, equivalent to interrupt time
        //struct WORK_STRUCT              suspend_bh;             // runs as bottom half, equivalent to interrupt time

        struct otg_workitem            *reset_bh;               // runs as bottom half, equivalent to interrupt time
        struct otg_workitem            *resume_bh;              // runs as bottom half, equivalent to interrupt time
        struct otg_workitem            *suspend_bh;             // runs as bottom half, equivalent to interrupt time

        //struct otg_task                 *task;

        void                            *privdata;              // private data for the bus interface
        char                            *arg;

        int                             usbd_maxstrings;
        struct usbd_string_descriptor   **usb_strings;
        struct usbd_urb                 *ep0_urb;
};


/*! bus driver registration
 *
 * Called by bus interface drivers to register themselves when loaded
 * or de-register when unloading.
 */
struct usbd_bus_instance *usbd_register_bus (struct usbd_bus_driver *, int);
void usbd_deregister_bus (struct usbd_bus_instance *);


/*! Enable/Disable Function
 *
 * Called by a bus interface driver to select and enable a specific function
 * driver.
 */
int usbd_enable_function (struct usbd_bus_instance *, char *, char *);
void usbd_disable_function (struct usbd_bus_instance *);


/*!
 * usbd_device_request - process a received urb
 * @urb: pointer to an urb structure
 *
 * Used by a USB Bus interface driver to pass received data in a URB to the
 * appropriate USB Function driver.
 *
 * This function must return 0 for success and -EINVAL if the request
 * is to be stalled.
 *
 * Not that if the SETUP is Host to Device with a non-zero wLength then there
 * *MUST* be a valid receive urb queued OR the request must be stalled.
 */
int usbd_device_request_irq (struct usbd_bus_instance*, struct usbd_device_request *);
int usbd_device_request (struct usbd_bus_instance*, struct usbd_device_request *);

/*!
 * Device I/O
 */
void usbd_urb_finished (struct usbd_urb *, int );
void usbd_urb_finished_irq (struct usbd_urb *, int );
//struct usbd_urb *usbd_first_urb_detached_irq (urb_link * hd);
//struct usbd_urb *usbd_first_urb_detached (urb_link * hd);
void usbd_do_urb_callback (struct usbd_urb *urb, int rc);


/*! flush endpoint
 */
void usbd_flush_endpoint_irq (struct usbd_endpoint_instance *);
void usbd_flush_endpoint (struct usbd_endpoint_instance *);
int usbd_find_endpoint_index(struct usbd_bus_instance *bus, int bEndpointAddress);

/*
 * urb lists
 */
//struct usbd_urb *usbd_first_urb (urb_link * hd);
//void usbd_unlink_urb (struct usbd_urb * urb);

struct usbd_urb *usbd_first_urb_detached (struct usbd_endpoint_instance *endpoint, urb_link * hd);
struct usbd_urb *usbd_first_finished_urb_detached (struct usbd_endpoint_instance *endpoint, urb_link * hd);
struct usbd_urb *usbd_first_ready_urb(struct usbd_endpoint_instance *endpoint, urb_link * hd);
