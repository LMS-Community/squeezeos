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
 * otg/otg/usbp-pcd.h - OTG Peripheral Controller Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/usbp-pcd.h|20070820053712|09860
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
 * @file otg/otg/usbp-pcd.h
 * @brief Peripheral Controller Driver Related Structures and Definitions.
 *
 * @ingroup PCD
 */

/*!
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct usbd_pcd_ops {

        /* mandatory */
        u8 bmAttributes;
        int max_endpoints;
        int ep0_packetsize;
        u8 high_speed_capable;
        u32 capabilities;                               /* UDC Capabilities - see usbd-bus.h for details */
        u8 bMaxPower;
        char *name;
        u8 ports;


        /* 3. called by ot_init() if defined */
        int (* serial_init) (struct pcd_instance *);    /* get device serial number if available */

        /* 4. called from usbd_enable_function_irq() - REQUIRED */
        int (*request_endpoints)                        /* process endpoint_request list and return endpoint_map */
                (struct pcd_instance *,
                 struct usbd_endpoint_map *,            /* showing allocated endpoints with attributes, addresses */
                 int, struct usbd_endpoint_request*);   /* and other associated information */

        /* 5. called from usbd_enable_function_irq() - REQUIRED */
        int (*set_endpoints)                            /* setup required endpoints in hardware */
                (struct pcd_instance *, int , struct usbd_endpoint_map *);

        /* 6. called by xxx() if defined */
        void (*enable) (struct pcd_instance *);         /* enable the UDC */

        /* 7. called by xxx() if defined */
        void (*disable) (struct pcd_instance *);        /* disable the UDC */

        /* 8. called by xxx() if defined */
        void (*start) (struct pcd_instance *);          /* called for DEVICE_CREATE to start the UDC */
        void (*stop) (struct pcd_instance *);           /* called for DEVICE_DESTROY to stop the UDC */
        void (*disable_ep)
                (struct pcd_instance *,
                 unsigned int ep,                       /* called for DEVICE_DESTROTY to disable endpoint zero */
                struct usbd_endpoint_instance *endpoint);

        /* 9. called by xxx() if defined */
        void (*set_address)
                (struct pcd_instance *,
                unsigned char address);                 /* called for DEVICE_RESET and DEVICE_ADDRESS_ASSIGNED to */


        /* 10. called by xxx() if defined */
        void (*setup_ep)
                (struct pcd_instance *,
                unsigned int ep,                        /* called for DEVICE_CONFIGURED to setup specified endpoint for use */
                struct usbd_endpoint_instance *endpoint);

        /* 11. */
        void * (*alloc_buffer)(struct pcd_instance*, int);
        void (*free_buffer)(struct pcd_instance*, void *);

        /* 12. called by xxx() if defined */
        void (*reset_ep)
                (struct pcd_instance *,
                unsigned int ep);                       /* called for DEVICE_RESET to reset endpoint zero */


        /* 13. called by usbd_send_urb() - REQUIRED */
        void (*start_endpoint_in)                       /* start an IN urb */
                (struct pcd_instance *,
                struct usbd_endpoint_instance *);

        /* 14. called by usbd_start_recv() - REQUIRED */
        void (*start_endpoint_out)                      /* start an OUT urb */
                (struct pcd_instance *,
                struct usbd_endpoint_instance *);

        /* 15. called by usbd_cancel_urb_irq() */
        void (*cancel_in_irq)
                (struct pcd_instance *,
                struct usbd_urb *urb);                  /* cancel active urb for IN endpoint */
        void (*cancel_out_irq)
                (struct pcd_instance *,
                struct usbd_urb *urb);                  /* cancel active urb for OUT endpoint */


        /* 16. called from ep0_recv_setup() for GET_STATUS request */
        int (*endpoint_halted)
                (struct pcd_instance *,
                 struct usbd_endpoint_instance *);      /* Return non-zero if requested endpoint is halted */
        int (*halt_endpoint)
                (struct pcd_instance *,
                 struct usbd_endpoint_instance *,
                 int);                                  /* Return non-zero if requested endpoint clear fails */

        int (*device_feature)
                (struct pcd_instance *,
                 int,
                 int);                                  /* Return non-zero if requested endpoint clear fails */

        int (*remote_wakeup)
                (struct pcd_instance *);                /* Return non-zero if remote wakeup fails */

        /* 17 */
        void (*startup_events) (struct pcd_instance *);   /* perform UDC specific USB events */

        /* 18. root hub operations
        */
        int (*hub_status) (struct pcd_instance *);
        int (*port_status) (struct pcd_instance *, int);
        int (*hub_feature) (struct pcd_instance *, int, int);
        int (*port_feature) (struct pcd_instance *, int, int, int);
        int (*get_ports_status) (struct pcd_instance *);
        int (*wait_for_change) (struct pcd_instance *);

        /* 19. configuration changes
         */
        int (*vbus_status) (struct pcd_instance *);
        int (*softcon) (struct pcd_instance *, int);
        u16 (*framenum) (struct pcd_instance *);

        otg_tick_t (*ticks)(struct pcd_instance *);
        otg_tick_t (*elapsed)(otg_tick_t *, otg_tick_t *);

};

extern struct usbd_pcd_ops usbd_pcd_ops;

/*! bus_set_speed
 */
void bus_set_speed(struct usbd_bus_instance *bus, int hs);



/*! pcd_rcv_complete_irq - complete a receive operation
 * Called by UDC driver to show completion of outstanding I/O, this will
 * update the urb length and if necessary will finish the urb passing it to the
 * bottom half which will in turn call the function driver callback.
 */
struct usbd_urb * pcd_rcv_complete_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad);
struct usbd_urb * pcd_rcv_finished_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad);
void pcd_rcv_cancelled_irq (struct usbd_endpoint_instance *endpoint);
struct usbd_urb * pcd_tx_next_irq (struct usbd_endpoint_instance *endpoint);
struct usbd_urb * pcd_rcv_next_irq (struct usbd_endpoint_instance *endpoint);
void pcd_urb_finished_irq(struct usbd_urb *urb, int rc);


/*!
 * pcd_configure_device is used by function drivers (usually the control endpoint)
 * to change the device configuration.
 *
 * usbd_device_event is used by bus interface drivers to tell the higher layers that
 * certain events have taken place.
 */
//void pcd_bus_event_handler (struct usbd_bus_instance *, usbd_device_event_t, int);
void pcd_bus_event_handler_irq (struct usbd_bus_instance *, usbd_device_event_t, int);



/*! pcd_tx_complete_irq - complete a transmit operation
 * Called by UDC driver to show completion of an outstanding I/O, this will update the urb sent
 * information and if necessary will finish the urb passing it to the bottom half which will in
 * turn call the function driver callback.
 */
struct usbd_urb * pcd_tx_complete_irq (struct usbd_endpoint_instance *endpoint, int restart);


/*! pcd_tx_cancelled_irq - finish pending tx_urb with USBD_URB_CANCELLED
 * Called by UDC driver to cancel the current transmit urb.
 */
static __inline__ void pcd_tx_cancelled_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *tx_urb;
        struct usbd_bus_instance *bus;
        struct pcd_instance *pcd;
        RETURN_IF (! (tx_urb = endpoint->tx_urb));
        bus = tx_urb->bus;
        pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG1(pcd->TAG, "BUS_TX CANCELLED: %p", (int) endpoint->tx_urb);
        pcd_urb_finished_irq (tx_urb, USBD_URB_CANCELLED);
        endpoint->sent = endpoint->last = 0;
        endpoint->tx_urb = NULL;
}


/*! pcd_tx_sendzlp - test if we need to send a ZLP
 * This has a side-effect of reseting the USBD_URB_SENDZLP flag.
 */
static __inline__ int pcd_tx_sendzlp (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *tx_urb = endpoint->tx_urb;
        struct usbd_bus_instance *bus;
        struct pcd_instance *pcd;

        RETURN_FALSE_UNLESS(tx_urb);                                    // no urb

        bus = tx_urb->bus;
        pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG3(pcd->TAG, "sent: %d actual: %d flags: %02x", endpoint->sent, tx_urb->actual_length, tx_urb->flags);

        RETURN_FALSE_IF(!endpoint->sent && tx_urb->actual_length);      // nothing sent yet and there is data to send
        RETURN_FALSE_IF(tx_urb->actual_length > endpoint->sent);        // still data to send
        RETURN_FALSE_UNLESS(tx_urb->flags & USBD_URB_SENDZLP);          // flag not set

        TRACE_MSG0(pcd->TAG, "SENDZLP");
        tx_urb->flags &= ~USBD_URB_SENDZLP;
        return TRUE;
}


/*! pcd_rcv_complete_irq - complete a receive
 * Called from rcv interrupt to complete.
 */
static __inline__ void pcd_rcv_fast_complete_irq (struct usbd_endpoint_instance *endpoint, struct usbd_urb *rcv_urb)
{
        //TRACE_MSG1(PCD, "BUS_RCV FAST COMPLETE: %d", rcv_urb->actual_length);
        pcd_urb_finished_irq (rcv_urb, USBD_URB_OK);
}

/*! pcd_recv_setup - process a device request
 * Note that we verify if a receive urb has been queued for H2D with non-zero wLength
 * and return -EINVAL to stall if the upper layers have not properly tested for and
 * setup a receive urb in this case.
 */
static __inline__ int pcd_recv_setup_irq (struct pcd_instance *pcd, struct usbd_device_request *request)
{
        struct usbd_bus_instance *bus = pcd->bus;
        //struct usbd_endpoint_instance *endpoint = bus->endpoint_array + 0;
        //TRACE_SETUP (pcd->TAG, request);
        return usbd_device_request_irq(bus, request);         // fail if already failed
        #if 0
        RETURN_EINVAL_IF (usbd_device_request_irq(bus, request));         // fail if already failed
        TRACE_MSG2(pcd->TAG, "BUS_RECV SETUP: RCV URB: %x TX_URB: %x", (int)endpoint->rcv_urb, (int)endpoint->tx_urb);
        RETURN_ZERO_IF ( (request->bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_DEVICE2HOST);
        RETURN_ZERO_IF (!le16_to_cpu (request->wLength));
        RETURN_EINVAL_IF (!endpoint->rcv_urb);
        return 0;
        #endif
}

/*! pcd_recv_setup_emulate_irq - emulate a device request
 * Called by the UDC driver to inject a SETUP request. This is typically used
 * by drivers for UDC's that do not pass all SETUP requests to us but instead give us
 * a configuration change interrupt.
 */
int pcd_recv_setup_emulate_irq(struct usbd_bus_instance *, u8, u8, u16, u16, u16);

/*! pcd_ep0_reset_irq - reset ep0 endpoint
 */
static void __inline__ pcd_ep0_reset_endpoint_irq (struct usbd_endpoint_instance *endpoint)
{
        //TRACE_MSG0(PCD, "BUS_EP0 RESET");
        pcd_tx_cancelled_irq (endpoint);
        pcd_rcv_cancelled_irq (endpoint);
        endpoint->sent = endpoint->last = 0;
}

/*! pcd_check_device_feature - verify that feature is set or clear
 * Check current feature setting and emulate SETUP Request to set or clear
 * if required.
 */
void pcd_check_device_feature(struct usbd_bus_instance *, int , int );
