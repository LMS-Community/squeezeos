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
 * otg/hardware/pcd.c - OTG Peripheral Controller Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/otglib/pcd.c|20070820053759|59156
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/pcd.c
 * @brief PCD only driver init.
 * Notes
 *
 * 1. The pcd file abstracts much of the UDC complexity as possible and
 * provides a common implementation that is shared to various extents by the
 * various pcd drivers.
 *
 * @ingroup PCD
 */

#include <otg/pcd-include.h>
#include <otg/usbp-func.h>

#define TRACE_VERBOSE   1
#define TRACE_VERY_VERBOSE   0

/* ******************************************************************************************* */
/* ******************************************************************************************* */

/*!
 * @brief  pcd_urb_finished_irq() - tell function that an urb has been finished.
 *
 * Used by a USB Bus driver to pass a sent urb back to the function
 * driver via the endpoints finished queue.
 * @param urb urb to process
 * @param rc result code
 *
 */
void pcd_urb_finished_irq(struct usbd_urb *urb, int rc)
{
        struct usbd_bus_instance *bus = (struct usbd_bus_instance *)urb->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)(bus ? bus->privdata : NULL);
        struct otg_instance *otg = bus ? bus->otg : NULL;
        //struct ocd_instance *ocd = otg ? otg->ocd : NULL;
        //struct ocd_ops *ocd_ops = otg ? otg->ocd_ops : NULL;

        UNLESS(urb && urb->bus) {
                //TRACE_MSG2(urb->bus->otg->pcd->TAG, "urb: %x bus: %x", urb, urb ? urb->bus : NULL);
                return;
        }
        urb->irq_flags = USBD_URB_FINISHED;
        urb->status = (usbd_urb_status_t)rc;

        /* fasttrack bus->ep0_urb */
        if (urb == bus->ep0_urb) {
                (urb = usbd_first_finished_urb_detached (urb->endpoint, &urb->endpoint->rdy));
                return;
        }


        TRACE_ELAPSED(pcd->TAG, "PCD 0. DONE", urb->ticks, (u32)urb);
        otg_get_ocd_info(pcd->otg, &urb->ticks, &urb->framenum);

        if (rc==USBD_URB_CANCELLED){
                usbd_unlink_urb(urb);
                usbd_do_urb_callback (urb, urb->status);
                return;
        }

        if (urb->flags & USBD_URB_FAST_RETURN) {
                #ifdef CONFIG_OTG_LATENCY_CHECK
                otg_tick_t ticks;
                u16 framenum;
                otg_tick_t urb_ticks = urb->ticks;
                otg_get_ocd_info(pcd->otg, &ticks, &framenum);
                #endif
                usbd_unlink_urb(urb);
                usbd_do_urb_callback (urb, urb->status);
                #ifdef CONFIG_OTG_LATENCY_CHECK
                /* elapsed time for the callback */
                TRACE_ELAPSED(pcd->TAG, "PCD 1. CB", ticks, (u32)urb);

                /* elapsed time for all processing since the urb was finished */
                TRACE_ELAPSED(pcd->TAG, "PCD 2. TOTAL", urb_ticks, (u32)urb);
                #endif
                return;
        }

        //TRACE_MSG1(pcd->TAG, "FINISH %p", urb);
        TRACE_MSG0(pcd->TAG, "otg_up_work()");
        otg_up_work(pcd->task);
}

/* ******************************************************************************************* */
/*! pcd_rcv_cancelled_irq - cancel current receive urb
 * Called by UDC driver to cancel the current receive urb.
 * @param endpoint - endpoint instance pointer
 */
void pcd_rcv_cancelled_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *rcv_urb;
        struct usbd_bus_instance *bus = endpoint->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

        TRACE_MSG1(pcd->TAG, "BUS_RCV CANCELLED: %p", (int) endpoint->rcv_urb);
        RETURN_IF (! (rcv_urb = endpoint->rcv_urb));
        TRACE_MSG1(pcd->TAG, "rcv_urb: %p", (int)endpoint->rcv_urb);
        pcd_urb_finished_irq (rcv_urb, USBD_URB_CANCELLED);
        endpoint->sent = endpoint->last = 0;
        endpoint->rcv_urb = NULL;
}


/*! pcd_tx_next_irq - get the current or next urb to transmit
 * Called by UDC driver to get current transmit urb or next available transmit urb.
 * @param endpoint - endpoint instance pointer
 * @return urb to transmit
 */
struct usbd_urb * pcd_tx_next_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_bus_instance *bus = endpoint->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct otg_instance *otg = pcd->otg;
        if (!endpoint->tx_urb) {

                if ( (endpoint->tx_urb = usbd_first_ready_urb(endpoint, &endpoint->rdy))) {
                        int i;
                        u8 *cp = endpoint->tx_urb->buffer;
                        if (otg)
                                otg_get_ocd_info(otg, &endpoint->tx_urb->ticks, &endpoint->tx_urb->framenum);

                        if (TRACE_VERBOSE)
                                TRACE_MSG3(pcd->TAG, "[%2x] NEXT TX: length: %d flags: %x",
                                                endpoint->bEndpointAddress,
                                                endpoint->tx_urb->actual_length, endpoint->tx_urb->flags);

                        if (TRACE_VERY_VERBOSE)
                                for (i = 0; i < endpoint->tx_urb->actual_length;  i+= 8)
                                        TRACE_MSG8(pcd->TAG, "SENT  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                                  );

                        endpoint->tx_urb->status = USBD_URB_ACTIVE;
                }
        }
        //TRACE_MSG1(endpoint->bus->otg->pcd->TAG, "BUS_TX NEXT TX_URB: %p", (int)endpoint->tx_urb);
        return endpoint->tx_urb;
}

/*! pcd_rcv_next_irq - get the current or next urb to receive into
 * Called by UDC driver to get current receive urb or next available receive urb.
 * @param endpoint - endpoint instance pointer
 * @return urb to receive
 */
struct usbd_urb * pcd_rcv_next_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_bus_instance *bus = endpoint->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct otg_instance *otg = pcd->otg;
        if (!endpoint->rcv_urb) {

                if ( (endpoint->rcv_urb = usbd_first_ready_urb(endpoint, &endpoint->rdy))) {
                        if (otg)
                                otg_get_ocd_info(otg, &endpoint->tx_urb->ticks, &endpoint->tx_urb->framenum);
                        endpoint->rcv_urb->status = USBD_URB_ACTIVE;
                }

        }
        //TRACE_MSG1(endpoint->bus->otg->pcd->TAG, "BUS_RCV_URB: %p", endpoint->rcv_urb);
        return endpoint->rcv_urb;
}

/* ******************************************************************************************* */
/*!
 * pcd_assign_endpoint()
 */
static int
pcd_assign_endpoint(struct pcd_instance *pcd, u8 index, u8 physicalEndpoint, u32 *used,
                struct usbd_endpoint_map *endpoint_map, u8 bmAttributes,
                u16 wMaxPacketSize_fs,
                u16 wMaxPacketSize_hs,
                u16 fs_transferSize,
                u16 hs_transferSize,
                u8 bInterval)
{
        RETURN_EINVAL_IF( *used & (1 << physicalEndpoint) );

        endpoint_map->index = index;
        endpoint_map->bEndpointAddress[0] = endpoint_map->bEndpointAddress[1] =
                physicalEndpoint | (USB_DIR_IN & bmAttributes);
        endpoint_map->physicalEndpoint[0] = endpoint_map->physicalEndpoint[1] =
                (physicalEndpoint * 2) + ((USB_DIR_IN & bmAttributes) ? 1 : 0);
        endpoint_map->wMaxPacketSize[0] = wMaxPacketSize_fs;
        endpoint_map->wMaxPacketSize[1] = wMaxPacketSize_hs;
        endpoint_map->transferSize[0] = fs_transferSize;
        endpoint_map->transferSize[1] = hs_transferSize;
        endpoint_map->bmAttributes[0] = endpoint_map->bmAttributes[1] = bmAttributes;
        endpoint_map->bInterval[0] = endpoint_map->bInterval[1] = bInterval;
        *used |= (1 << physicalEndpoint);

        TRACE_MSG6(pcd->TAG, "ASSIGN: index: %02x phys: %02x addr: %02x wMaxPacketSize: %02x:%02x",
                        physicalEndpoint,
                        endpoint_map->physicalEndpoint[0],
                        endpoint_map->bEndpointAddress[0],
                        endpoint_map->bEndpointAddress[1],
                        endpoint_map->wMaxPacketSize[0],
                        endpoint_map->wMaxPacketSize[1]
                        );
        //printk(KERN_INFO"%s: DDDD\n", __FUNCTION__);
        return 0;
}

/*!
 * pcd_request_endpoints()
 * @param pcd
 * @param endpoint_map_array
 * @param endpointsRequested
 * @param requestedEndpoints
 *
 */
int
pcd_request_endpoints(struct pcd_instance *pcd, struct usbd_endpoint_map *endpoint_map_array, int endpointsRequested,
                struct usbd_endpoint_request *requestedEndpoints)
{
        //struct usbd_device_description *device_description;
        int i, in, out;
        u32 in_used = 0;
        u32 out_used = 0;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG1(pcd->TAG, "REQUEST ENDPOINTS: %d", endpointsRequested);
        for (in = out = 1, i = 0; i < endpointsRequested; i++) {
                struct usbd_endpoint_map *endpoint_map = endpoint_map_array + i;
                u8 index = requestedEndpoints[i].index;
                u8 bInterval = requestedEndpoints[i].bInterval;
                u8 bmAttributes = requestedEndpoints[i].bmAttributes;
                u16 fs_transferSize = requestedEndpoints[i].fs_requestedTransferSize;
                u16 hs_transferSize = requestedEndpoints[i].hs_requestedTransferSize;
                TRACE_MSG5(pcd->TAG, "REQUEST ENDPOINTS[%2x] index: %02x bm: %02x trans: %04x:%04x",
                                i, index, bmAttributes, fs_transferSize, hs_transferSize);


                switch(bmAttributes) {

                case USB_DIR_IN | USB_ENDPOINT_BULK:
                        RETURN_EINVAL_UNLESS(in < usbd_pcd_ops.max_endpoints);
                        CONTINUE_IF(!pcd_assign_endpoint(pcd, index, in++, &in_used, endpoint_map, bmAttributes,
                                                0x40, 0x200, fs_transferSize, hs_transferSize, bInterval));
                        THROW(error);

                case USB_DIR_IN | USB_ENDPOINT_INTERRUPT:
                case USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT:
                        RETURN_EINVAL_UNLESS(in < usbd_pcd_ops.max_endpoints);
                        CONTINUE_IF(!pcd_assign_endpoint(pcd, index, in++, &in_used, endpoint_map, bmAttributes,
                                                0x40, 0x40, fs_transferSize, hs_transferSize, bInterval));
                        THROW(error);

                case USB_DIR_OUT | USB_ENDPOINT_BULK:
                        RETURN_EINVAL_UNLESS(out < usbd_pcd_ops.max_endpoints);
                        CONTINUE_IF(!pcd_assign_endpoint(pcd, index, out++, &out_used, endpoint_map, bmAttributes,
                                                0x40, 0x200, fs_transferSize, hs_transferSize, bInterval));
                        THROW(error);
                case USB_DIR_OUT | USB_ENDPOINT_INTERRUPT:
                case USB_DIR_OUT | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT:
                        RETURN_EINVAL_UNLESS(out < usbd_pcd_ops.max_endpoints);
                        CONTINUE_IF(!pcd_assign_endpoint(pcd, index, out++, &out_used, endpoint_map, bmAttributes,
                                                0x40, 0x40, fs_transferSize, hs_transferSize, bInterval));
                        //CONTINUE_IF(!pcd_assign_endpoint(pcd, index, in++, &in_used, endpoint_map, bmAttributes,
                        //                        0x40, fs_transferSize, hs_transferSize, bInterval));
                        THROW(error);

                case USB_ENDPOINT_CONTROL:
                case USB_DIR_IN | USB_ENDPOINT_ISOCHRONOUS:
                case USB_DIR_OUT | USB_ENDPOINT_ISOCHRONOUS:
                        THROW(error);
                }
                THROW(error);
                CATCH(error) {
                        TRACE_MSG4(pcd->TAG, "FAILED num: %d bmAttributes: %02x transferSize: %04x:%04x",
                                        i, bmAttributes, fs_transferSize, hs_transferSize);

                        //printk(KERN_ERR"%s: FAILED num: %d bmAttributes: %02x transferSize: %02x\n",
                        //                __FUNCTION__, i, bmAttributes, transferSize);
                        return -EINVAL;
                }
        }
        return 0;
}

/* ******************************************************************************************* */
/*! bus_request_endpoints -
 * @param bus
 * @param endpoint_map_array
 * @param endpointsRequested
 * @param requestedEndpoints
 * @return 0 on finish, or -EINVAL
 */
int bus_request_endpoints(struct usbd_bus_instance *bus, struct usbd_endpoint_map *endpoint_map_array, int endpointsRequested,
                struct usbd_endpoint_request *requestedEndpoints)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int i;
        TRACE_MSG0(pcd->TAG, "BUS_REQUEST ENDPOINTS:");
        if (usbd_pcd_ops.request_endpoints(pcd, endpoint_map_array, endpointsRequested, requestedEndpoints)) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                return -EINVAL;
        }
        for (i = 0; i < endpointsRequested; i++) {
                struct usbd_endpoint_map *endpoint_map = endpoint_map_array + i;
                //TRACE_MSG8(pcd->TAG, "address: %02x:%02x physical: %02x:%02x request: %02x:%02x size: %04x:%04x",
                //                 endpoint_map->bEndpointAddress[0], endpoint_map->bEndpointAddress[1],
                //                 endpoint_map->physicalEndpoint[0], endpoint_map->physicalEndpoint[1],
                //                 endpoint_map->bmAttributes[0], endpoint_map->bmAttributes[1],
                //                 endpoint_map->wMaxPacketSize[0], endpoint_map->wMaxPacketSize[1]
                //                 );
                TRACE_MSG8(pcd->TAG, "REQUEST[%02d:%02d] endpoint_map: %x address: %04x physical: %04x request: %04x size: %04x:%04x",
                                i, endpointsRequested,
                                endpoint_map,
                                endpoint_map->bEndpointAddress[0] << 8 | endpoint_map->bEndpointAddress[1],
                                endpoint_map->physicalEndpoint[0] << 8 | endpoint_map->physicalEndpoint[1],
                                endpoint_map->bmAttributes[0] << 8 |  endpoint_map->bmAttributes[1],
                                endpoint_map->wMaxPacketSize[0],
                                endpoint_map->wMaxPacketSize[1]
                          );
        }
        TRACE_MSG0(pcd->TAG, "BUS_REQUEST ENDPOINTS: finished");
        return 0;
}

/*! bus_set_endpoints  - initialize pcd instance endpoints array
 * @param bus - usbd_bus_instance pointer
 * @param endpointsRequested - number of requested endpoints
 * @param endpoint_map_array - usbd_endpoint_map pointer
 */
int bus_set_endpoints(struct usbd_bus_instance *bus, int endpointsRequested, struct usbd_endpoint_map *endpoint_map_array)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG5(pcd->TAG, "SET ENDPOINTS bus: %p endpointsRequested: %d endpoint_map_array: %p pcd: %x pcd->otg: %x",
                        bus, endpointsRequested, endpoint_map_array, pcd, pcd ? pcd->otg : NULL);
        // XXX do we need to set bEndpointAddress
        return usbd_pcd_ops.set_endpoints ? usbd_pcd_ops.set_endpoints(pcd, endpointsRequested, endpoint_map_array) : 0;
}

/*! bus_set_configuration -
 * @param bus - bus instance pointer
 * @param config
 */
int bus_set_configuration(struct usbd_bus_instance *bus, int config)
{
        pcd_bus_event_handler_irq (bus, config? DEVICE_CONFIGURED : DEVICE_DE_CONFIGURED, 0);
        return 0;
}

/*! bus_set_address - called to set device address
 * @param bus - bus instance pointer
 * @param address - address to set
 */
int bus_set_address(struct usbd_bus_instance *bus, int address)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        pcd->new_address = address;
        pcd_bus_event_handler_irq (bus, DEVICE_ADDRESS_ASSIGNED, address);
        if (usbd_pcd_ops.set_address) usbd_pcd_ops.set_address (pcd, address);
        return 0;
}

/*! bus_set_speed - called to set bus high speed capability
 * @param bus - bus instance pointer
 * @param hs - high speed capability
 */
void bus_set_speed(struct usbd_bus_instance *bus, int hs)
{
        bus->high_speed = hs;
}

/* ******************************************************************************************* */

/*! pcd_search_endpoint_index - find endpoint map index given endpoint address
 * @param bus - usbd_bus_instance pointer
 * @param bEndpointAddress - endbpoint address
 * @return found endpoint index
 */
int pcd_search_endpoint_index(struct usbd_bus_instance *bus, int bEndpointAddress)
{
        int i;
        for (i = 0; i < bus->endpoints; i++)
                BREAK_IF (bus->endpoint_array[i].bEndpointAddress[bus->high_speed] == bEndpointAddress);
        return i;
}

/*! pcd_search_endpoint - find endpoint given endpoint address
 * @param bus - bus instance pointer
 * @param bEndpointAddress - endpoint address
 * @return found endpoint instance pointer
 */
struct usbd_endpoint_instance * pcd_search_endpoint(struct usbd_bus_instance *bus, int bEndpointAddress)
{
        int i = pcd_search_endpoint_index(bus, bEndpointAddress);
        //TRACE_MSG2(pcd->TAG, "BUS_SEARCH ENDPOINT:  addr: %02x index: %d", bEndpointAddress, i);
        RETURN_NULL_UNLESS(i < bus->endpoints);
        return &(bus->endpoint_array[i]);
}

/*! bus_endpoint_halted - check if endpoint halted
 * Used by the USB Device Core to check endpoint halt status.
 *
 * Return actual halted status if available or'd with endpoint->feature_setting set value.
 * Assume not halted if udc driver does not provide an endpoint halted function.
 * @param bus - bus instance pointer
 * @param bEndpointAddress - endpoint to check halt status
 * @return check result
 */
int bus_endpoint_halted (struct usbd_bus_instance *bus, int bEndpointAddress)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int endpoint_index = pcd_search_endpoint_index(bus, bEndpointAddress);
        struct usbd_endpoint_instance *endpoint = pcd_search_endpoint(bus, bEndpointAddress);
        int halted;

        TRACE_MSG1(pcd->TAG, "BUS_ENDPOINT HALTED:  endpoint: %p", (int) endpoint);
        RETURN_EINVAL_UNLESS(endpoint);

        /* return true IFF endpoint was previously halted by host OR if hardware
         * has been stalled.
         */

        halted = usbd_pcd_ops.endpoint_halted ? usbd_pcd_ops.endpoint_halted(pcd, endpoint) : 0;
        TRACE_MSG2(pcd->TAG, "BUS_ENDPOINT STALLED %d FEATURE: %08x",
                        halted, bus->endpoint_array[endpoint_index].feature_setting);

        return halted || (endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)? 1 : 0);
}

/*! bus_halt_endpoint - handle set/clear feature requests
 * Used by the USB Device Core to set/clear endpoint halt status.
 *
 * We assume that if the udc driver does not implement anything then
 * we should just return zero for ok.
 *
 * XXX should do endpoint instance as arg
 * @param bus - bus instance pointer
 * @param bEndpointAddress - endpoint address
 * @param flag - set/clear flag
 *
 */
int bus_halt_endpoint (struct usbd_bus_instance *bus, int bEndpointAddress,  int flag)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        //int endpoint_index = pcd_search_endpoint_index(bus, bEndpointAddress);
        struct usbd_endpoint_instance *endpoint = pcd_search_endpoint(bus, bEndpointAddress);

        TRACE_MSG3(pcd->TAG, "BUS_ENDPOINT HALT:  endpoint: %p bEndpointAddress: %02x flag: %d",
                        endpoint, bEndpointAddress, flag);

        RETURN_EINVAL_UNLESS(endpoint);

        endpoint->feature_setting = flag ?
                (endpoint->feature_setting | FEATURE(USB_ENDPOINT_HALT)) :
                (endpoint->feature_setting & ~FEATURE(USB_ENDPOINT_HALT));

        if (flag)
                usbd_flush_endpoint(endpoint);

        return usbd_pcd_ops.halt_endpoint ? usbd_pcd_ops.halt_endpoint(pcd, endpoint, flag) : 0;
}

/*! bus_device_feature - handle set/clear feature requests
 * Used by the USB Device Core to set/clear device feature requests
 *
 * We assume that if the udc driver does not implement anything then
 * we should just return -EINVAL.
 *
 * @param bus - bus instance pointer
 * @param selector - selector
 * @param flag - set/clear flag
 *
 */
int bus_device_feature (struct usbd_bus_instance *bus, int selector, int flag)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

        TRACE_MSG2(pcd->TAG, "BUS_DEVICE_FEATURE selector: %d flag: %d", selector, flag);

        return usbd_pcd_ops.device_feature ? usbd_pcd_ops.device_feature(pcd, selector, flag) : -EINVAL;
}

/*! pcd_disable_endpoints - disable udc and all endpoints
 * @param bus - bus instance pointer
 */
static void pcd_disable_endpoints (struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int epn;
        TRACE_MSG0(pcd->TAG, "--");
        RETURN_IF (!bus || !bus->endpoint_array);
        for (epn = 0; epn < usbd_pcd_ops.max_endpoints; epn++) {
                struct usbd_endpoint_instance *endpoint = (bus->endpoint_array + epn);
                TRACE_MSG2(pcd->TAG, "epn: %d endpoint: %p", epn, endpoint);
                CONTINUE_IF (!endpoint);
                usbd_flush_endpoint (endpoint);
        }
}


/*! bus_event_handler - handle generic bus event
 * Called by usb core layer to inform bus of an event.
 * @param bus - bus instance pointer
 * @param event - usbd_device_event
 * @param data -
 */
int bus_event_handler (struct usbd_bus_instance *bus, usbd_device_event_t event, int data)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int epn;
        //struct usbd_endpoint_map *endpoint_map_array = bus->function_instance->endpoint_map_array;
        int hs = bus->high_speed;

        TRACE_MSG1(pcd->TAG, "BUS_EVENT %d", event);
        switch (event) {
        case DEVICE_UNKNOWN:
                break;

        case DEVICE_INIT:
                TRACE_MSG0(pcd->TAG, "EVENT INIT");
                break;

        case DEVICE_CREATE:
                TRACE_MSG0(pcd->TAG, "EVENT CREATE");
                pcd_disable_endpoints (bus);
                if (usbd_pcd_ops.start) usbd_pcd_ops.start (pcd);
                break;

        case DEVICE_HUB_CONFIGURED:
                TRACE_MSG0(pcd->TAG, "EVENT HUB_CONFIGURED");
                //bi_connect (bus, NULL);
                break;

        case DEVICE_RESET:
                TRACE_MSG0(pcd->TAG, "EVENT RESET");

                if (usbd_pcd_ops.set_address) usbd_pcd_ops.set_address (pcd, 0);
                if (usbd_pcd_ops.reset_ep) usbd_pcd_ops.reset_ep (pcd, 0);

                pcd_disable_endpoints (bus);
                otg_queue_event(pcd->otg, (otg_current_t)(BUS_RESET | BUS_SUSPENDED_), pcd->TAG, "DEVICE_RESET");

                break;

        case DEVICE_ADDRESS_ASSIGNED:
                TRACE_MSG1(pcd->TAG, "EVENT ADDRESSED: %d", data);
                otg_queue_event(pcd->otg, ADDRESSED, pcd->TAG, "ADDRESSED");

                break;

        case DEVICE_CONFIGURED:
                TRACE_MSG0(pcd->TAG, "EVENT CONFIGURED");
                otg_queue_event(pcd->otg, CONFIGURED, pcd->TAG, "CONFIGURED");
                // iterate across the physical endpoint instance array to enable the endpoints
                if (usbd_pcd_ops.setup_ep)
                        for (epn = 1; epn < bus->endpoints; epn++)
                                if (bus->endpoint_array[epn].physicalEndpoint[hs])
                                        usbd_pcd_ops.setup_ep (pcd, epn, bus->endpoint_array + epn);
                break;

        case DEVICE_DE_CONFIGURED:
                TRACE_MSG0(pcd->TAG, "EVENT DE-CONFIGURED");
                otg_queue_event(pcd->otg, CONFIGURED_, pcd->TAG, "DE-CONFIGURED");
                break;

        case DEVICE_SET_INTERFACE:
                TRACE_MSG0(pcd->TAG, "EVENT SET INTERFACE");
                break;

        case DEVICE_SET_FEATURE:
                TRACE_MSG0(pcd->TAG, "EVENT SET FEATURE");
                break;

        case DEVICE_CLEAR_FEATURE:
                TRACE_MSG0(pcd->TAG, "EVENT CLEAR FEATURE");
                break;

        case DEVICE_BUS_INACTIVE:
                TRACE_MSG0(pcd->TAG, "EVENT INACTIVE");
                TRACE_MSG1(pcd->TAG, "EVENT INACTIVE: state: %x", bus->device_state);
                otg_queue_event(pcd->otg, BUS_SUSPENDED, pcd->TAG, "DEVICE_BUS_INACTIVE");
                break;

        case DEVICE_BUS_ACTIVITY:
                TRACE_MSG0(pcd->TAG, "EVENT ACTIVITY");
                otg_queue_event(pcd->otg, BUS_SUSPENDED_, pcd->TAG, "DEVICE_BUS_ACTIVITY");
                break;

        case DEVICE_POWER_INTERRUPTION:
                TRACE_MSG0(pcd->TAG, "POWER INTERRUPTION");
                break;

        case DEVICE_HUB_RESET:
                TRACE_MSG0(pcd->TAG, "HUB RESET");
                break;

        case DEVICE_DESTROY:
                //printk(KERN_INFO"%s: DESTROY\n", __FUNCTION__);
                TRACE_MSG0(pcd->TAG, "DEVICE DESTROY");
                //bi_disconnect (bus, NULL);
                pcd_disable_endpoints (bus);
                if (usbd_pcd_ops.stop) usbd_pcd_ops.stop (pcd);
                break;

        case DEVICE_CLOSE:
                TRACE_MSG0(pcd->TAG, "DEVICE CLOSE");
                break;
        default:
                TRACE_MSG1(pcd->TAG, "DEVICE UNKNOWN: %d", event);
                break;
        }
        TRACE_MSG1(pcd->TAG, "BUS_EVENT %d finished", event);
        //TRACE_MSG2(pcd->TAG, "EVENT bNumInterfaces: %x alternates: %x", bus->bNumInterfaces, bus->alternates);
        return 0;
}


/*! bus_start_endpoint_in - called to send urb
 * @param bus - bus instance pointer
 * @param endpoint - endpoint pointer
 * @return 0 on finish
 */
int bus_start_endpoint_in (struct usbd_bus_instance *bus, struct usbd_endpoint_instance *endpoint)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

        RETURN_ZERO_IF (!endpoint);
        pcd->otg->interrupts++;

        /* call pcd_start_endpoint_in IFF we didn't previously have a tx urb */
        otg_pthread_mutex_lock(&pcd->mutex);
        if (!endpoint->tx_urb && pcd_tx_next_irq (endpoint))
                usbd_pcd_ops.start_endpoint_in (pcd, endpoint);
        otg_pthread_mutex_unlock(&pcd->mutex);
        return 0;
}

/*! bus_start_endpoint_out -called to receive urb
 * @param bus - bus instance pointer
 * @param endpoint - endpoint instance pointer
 * @return 0 on finish
 */
int bus_start_endpoint_out (struct usbd_bus_instance *bus, struct usbd_endpoint_instance *endpoint)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        RETURN_ZERO_IF (!endpoint);
        pcd->otg->interrupts++;

        /* call pcd_start_endpoint_OUT IFF we didn't previously have a rcv urb */
        otg_pthread_mutex_lock(&pcd->mutex);
        if (!endpoint->rcv_urb && pcd_rcv_next_irq (endpoint))
                usbd_pcd_ops.start_endpoint_out (pcd, endpoint);
        otg_pthread_mutex_unlock(&pcd->mutex);
        return 0;
}

/*! bus_cancel_urb_irq - cancel sending an urb
 * Used by the USB Device Core to cancel an urb.
 * @param urb - urb to cancel sending operation
 */
int bus_cancel_urb_irq (struct usbd_urb *urb)
{
        struct usbd_bus_instance *bus = urb->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct usbd_endpoint_instance *endpoint;
        RETURN_EINVAL_IF (!urb);
        TRACE_MSG1(pcd->TAG, "BUS_CANCEL URB: %x", (int)urb);
        endpoint = urb->endpoint;
        switch (urb->endpoint->bEndpointAddress[urb->bus->high_speed] & USB_ENDPOINT_DIR_MASK) {
        case USB_DIR_IN:
                // is this the active urb?
                if (urb->endpoint->tx_urb == urb) {
                        urb->endpoint->tx_urb = NULL;
                        //TRACE_MSG1(pcd->TAG, "CANCEL IN URB: %02x", urb->endpoint->bEndpointAddress[urb->bus->high_speed]);
                        if (usbd_pcd_ops.cancel_in_irq) usbd_pcd_ops.cancel_in_irq (pcd, urb);
                }
                pcd_urb_finished_irq (urb, USBD_URB_CANCELLED);
                break;

        case USB_DIR_OUT:
                // is this the active urb?
                if (urb->endpoint->rcv_urb == urb) {
                        urb->endpoint->rcv_urb = NULL;
                        //TRACE_MSG1(pcd->TAG, "CANCEL OUT URB: %02x", urb->endpoint->bEndpointAddress[urb->bus->high_speed]);
                        if (usbd_pcd_ops.cancel_out_irq) usbd_pcd_ops.cancel_out_irq (pcd, urb);
                }
                //TRACE_MSG0(pcd->TAG, "CANCEL RECV URB");
                pcd_urb_finished_irq (urb, USBD_URB_CANCELLED);
                break;
        }
        endpoint->last = endpoint->sent = 0;
        return 0;
}

/*! pcd_rcv_finished_irq--
 * @param endpoint - endpoint instance pointer
 * @param len -
 * @param urb_bad
 */
struct usbd_urb * pcd_rcv_finished_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad)
{
        struct usbd_urb *rcv_urb;

        // if we had an urb then update actual_length, dispatch if neccessary
        if (otg_likely ( (int) (rcv_urb = endpoint->rcv_urb))) {
                struct usbd_bus_instance *bus = rcv_urb->bus;
                struct pcd_instance *pcd;
                pcd = (struct pcd_instance *)bus->privdata;

                //TRACE_MSG5(pcd->TAG, "BUS_RCV COMPLETE: finished buffer: %x actual: %d len: %d bad: %d: status: %d",
                //                rcv_urb->buffer, rcv_urb->actual_length, len, urb_bad, rcv_urb->status);

                if (!urb_bad && !endpoint->rcv_error && (rcv_urb->bus->status == USBD_OK)) {

#if 0
                        int i;
                        u8 *cp = rcv_urb->buffer;
                        for (i = 0; i < rcv_urb->actual_length + len; i+= 8) {

                                TRACE_MSG8(pcd->TAG, "RECV  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                          );
                        }
#endif
                        // increment the received data size
                        rcv_urb->actual_length += len;

                        if (rcv_urb->actual_length && !len)
                                rcv_urb->flags |= USBD_URB_SENDZLP;

                        endpoint->rcv_urb = NULL;
                        rcv_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum (bus->otg) : 0;
                        TRACE_MSG2(pcd->TAG, "BUS_RCV COMPLETE: urb: %p framenum: %x", rcv_urb, (int) rcv_urb->framenum);
                        pcd_urb_finished_irq (rcv_urb, USBD_URB_OK);
                        rcv_urb = NULL;
                }
                else {
                        rcv_urb->actual_length = 0;
                        //endpoint->rcv_error = 1;
                }
        }

        // if we don't have an urb see if we can get one
        return pcd_rcv_next_irq (endpoint);
}

/*! pcd_rcv_complete_irq
 */
struct usbd_urb * pcd_rcv_complete_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad)
{
        struct usbd_urb *rcv_urb;

        /* if we had an urb then update actual_length, dispatch if neccessary */
        if (otg_likely ( (int) (rcv_urb = endpoint->rcv_urb))) {
                struct usbd_bus_instance *bus = rcv_urb->bus;
                struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

                #if 0
                TRACE_MSG5(pcd->TAG, "BUS_RCV COMPLETE: buffer: %d actual: %d len: %d bad: %d: status: %d",
                                rcv_urb->buffer_length, rcv_urb->actual_length, len, urb_bad, rcv_urb->status);
                #endif

                if (TRACE_VERBOSE)
                        TRACE_MSG7(pcd->TAG, "[%2x] alloc: %d buffer: %d actual: %d packet: %d transfer: %d len: %d",
                                        endpoint->bEndpointAddress,
                                        rcv_urb->alloc_length, rcv_urb->buffer_length, rcv_urb->actual_length,
                                        endpoint->wMaxPacketSize[bus->high_speed], endpoint->rcv_transferSize, len);


                // check the urb is ok, are we adding data less than the packetsize
                if (!urb_bad && !endpoint->rcv_error && (rcv_urb->bus->status == USBD_OK)
                                /*&& (len <= endpoint->wMaxPacketSize)*/)
                {

                        /* increment the received data size */
                        rcv_urb->actual_length += len;

                        /* if the current received data is short (less than full packetsize) which
                         * indicates the end of the bulk transfer, we have received the maximum
                         * transfersize, or if we do not have enough room to receive another packet
                         * then pass this data up to the function driver
                         */

                        if (
                                        (
                                         ((rcv_urb->actual_length % endpoint->wMaxPacketSize[bus->high_speed])) ||
                                         (rcv_urb->actual_length >= rcv_urb->buffer_length) ||
                                         (rcv_urb->actual_length + endpoint->wMaxPacketSize[bus->high_speed] >
                                          rcv_urb->alloc_length)))
                        {
                                //TRACE_MSG2(pcd->TAG, "BUS_RCV COMPLETE: finished length: %d framenum: %x",
                                //                rcv_urb->actual_length, (int) rcv_urb->framenum);

                                /* N.B. we don't reset endpoint->rcv_urb until finish urb is run
                                 */
                                rcv_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum (bus->otg) : 0;
                                pcd_urb_finished_irq (rcv_urb, USBD_URB_OK);
                                endpoint->rcv_urb = rcv_urb = NULL;
                                endpoint->last = 0;
                        }
                }
                else {
                        rcv_urb->actual_length = 0;
                }
        }

        /* if we don't have an urb see if we can get one */
        return pcd_rcv_next_irq (endpoint);
}

/*! pcd_tx_complete_irq
 */
struct usbd_urb * pcd_tx_complete_irq (struct usbd_endpoint_instance *endpoint, int restart)
{
        struct usbd_urb *tx_urb;

        //TRACE_MSG2(pcd->TAG, "tx_urb: %x restart: %d", endpoint->tx_urb, restart);

        // if we have a tx_urb advance or reset, finish if complete


        if ( (tx_urb = endpoint->tx_urb)) {

                struct usbd_bus_instance *bus = tx_urb->bus;
                struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

                //printk(KERN_INFO"%s: tx_urb: %x endpoint->tx_urb: %x\n", __FUNCTION__, tx_urb, endpoint->tx_urb);

                //printk(KERN_INFO"%s: finishing tx_urb\n", __FUNCTION__);

                if (TRACE_VERBOSE)
                        TRACE_MSG6(pcd->TAG, "[%2x] BUS_TX CURRENT TX_URB: %p actual: %d sent: %d last: %d: flags: %x",
                                        endpoint->bEndpointAddress,
                                        (int)endpoint->tx_urb, tx_urb->actual_length, endpoint->sent,
                                        endpoint->last, tx_urb->flags);

                if (otg_likely (!restart)) {
                        endpoint->sent += endpoint->last;
                }
                else {
                        TRACE_MSG1(pcd->TAG, "[%2x] RESTARTING", endpoint->bEndpointAddress);
                }
                endpoint->last = 0;

                if ( (endpoint->sent >= tx_urb->actual_length) && ! (tx_urb->flags & USBD_URB_SENDZLP) ) {
                        endpoint->tx_urb = NULL;
                        endpoint->last = endpoint->sent = 0;
                        tx_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum (bus->otg) : 0;
                        if (TRACE_VERBOSE)
                                TRACE_MSG3(pcd->TAG, "[%2x] BUS_TX COMPLETE: finished tx_urb: %p framenum: %x",
                                                endpoint->bEndpointAddress,
                                                (int)tx_urb, (int)tx_urb->framenum);
                        pcd_urb_finished_irq (tx_urb, USBD_URB_OK);
                }
        }
        return pcd_tx_next_irq (endpoint);
}

/* ******************************************************************************************* */

/*! pcd_disable - Stop using the USB Device Controller
 * Stop using the USB Device Controller.
 * Shutdown and free dma channels, de-register the interrupt handler.
 */
void pcd_disable (struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int hs = bus->high_speed;
        int epn;

        TRACE_MSG0(pcd->TAG, "BUS_UDC DISABLE");
        if (usbd_pcd_ops.disable_ep) usbd_pcd_ops.disable_ep (pcd, 0, bus->endpoint_array);

        pcd_disable_endpoints (bus);

        if (usbd_pcd_ops.disable_ep)
                for (epn = 1; epn < bus->endpoints; epn++)
                        if (bus->endpoint_array[epn].physicalEndpoint[hs])
                                usbd_pcd_ops.disable_ep (pcd, epn, bus->endpoint_array + epn);

        if (usbd_pcd_ops.disable) usbd_pcd_ops.disable (pcd);
}

/* ************************************************************************************* */
/*! bus_framenum
 */
int bus_framenum (struct usbd_bus_instance *bus)
{
        //struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        return (pcd_ops.framenum) ? pcd_ops.framenum (bus->otg) : 0;
}

#if 0
/*! bus_interrupts
 */
u64 bus_interrupts (void)
{
        return pcd_instance->otg->interrupts;
}
#endif

/*! pcd_ticks
 */
otg_tick_t pcd_ticks (void)
{
        return usbd_pcd_ops.ticks ? usbd_pcd_ops.ticks (NULL) : 0;
}

/*! pcd_elapsed
 */
otg_tick_t pcd_elapsed (otg_tick_t *t1, otg_tick_t *t2)
{
        return usbd_pcd_ops.elapsed ? (usbd_pcd_ops.elapsed (t1, t2)) : 0;
}

/*! pcd_recv_setup_emulate_irq - emulate a device request
 */
int pcd_recv_setup_emulate_irq(struct usbd_bus_instance *bus, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct usbd_device_request request;

        //printk(KERN_INFO"bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x\n",
        //                bmRequestType, bRequest, wValue, wIndex);
        TRACE_MSG4(pcd->TAG, "bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x",
                        bmRequestType, bRequest, wValue, wIndex);
        request.bmRequestType = bmRequestType;
        request.bRequest = bRequest;
        request.wValue = cpu_to_le16(wValue);
        request.wIndex = cpu_to_le16(wIndex);
        request.wLength = cpu_to_le16(wLength);
        return pcd_recv_setup_irq(pcd, &request);
}

/*! pcd_check_device_feature - verify that feature is set or clear
 * Check current feature setting and emulate SETUP Request to set or clear
 * if required.
 * @param bus -
 * @param feature -
 * @param flag
 */
void pcd_check_device_feature(struct usbd_bus_instance *bus, int feature, int flag)
{
        int status = bus->device_feature_settings & (1 << feature);
        //TRACE_MSG2(pcd->TAG, "BUS_CHECK FEATURE: feature: %x flag: %x", feature, flag);
        if (!status && flag)
                pcd_recv_setup_emulate_irq(bus, USB_REQ_HOST2DEVICE, USB_REQ_SET_FEATURE, feature, 0, 0);
        else if (status && !flag)
                pcd_recv_setup_emulate_irq(bus, USB_REQ_HOST2DEVICE, USB_REQ_CLEAR_FEATURE, feature, 0, 0);
        //TRACE_MSG1(pcd->TAG, "BUS_CHECK FEATURE: features; %08x", bus->device_feature_settings);
}

/* ******************************************************************************************* */
#if defined(CONFIG_OTG_NOC99)
struct usbd_bus_operations bus_ops;
/*!
 * pcd_global_init - initialize pcd related bus operations
 */
void pcd_global_init(void)
{
        ZERO(bus_ops);
        bus_ops.start_endpoint_in = bus_start_endpoint_in;
        bus_ops.start_endpoint_out = bus_start_endpoint_out;
        bus_ops.cancel_urb_irq = bus_cancel_urb_irq;
        bus_ops.endpoint_halted = bus_endpoint_halted;
        bus_ops.halt_endpoint = bus_halt_endpoint;
        bus_ops.device_feature = bus_device_feature;
        bus_ops.set_configuration = bus_set_configuration;
        bus_ops.set_address = bus_set_address;
        bus_ops.event_handler = bus_event_handler;
        bus_ops.request_endpoints = bus_request_endpoints;
        bus_ops.set_endpoints = bus_set_endpoints;
        bus_ops.framenum = bus_framenum;
        //bus_ops.ticks = bus_ticks;
        //bus_ops.elapsed = bus_elapsed;
        //bus_ops.interrupts = bus_interrupts;
        //bus_ops.device_feature = bus_device_feature;
}

#else /* defined(CONFIG_OTG_NOC99) */
struct usbd_bus_operations bus_ops = {
        .start_endpoint_in = bus_start_endpoint_in,
        .start_endpoint_out = bus_start_endpoint_out,
        .cancel_urb_irq = bus_cancel_urb_irq,
        .endpoint_halted = bus_endpoint_halted,
        .halt_endpoint = bus_halt_endpoint,
        .device_feature = bus_device_feature,
        .set_configuration = bus_set_configuration,
        .set_address = bus_set_address,
        .event_handler = bus_event_handler,
        .request_endpoints = bus_request_endpoints,
        .set_endpoints = bus_set_endpoints,
        .framenum = bus_framenum,
        //.ticks = bus_ticks,
        //.elapsed = bus_elapsed,
        //.interrupts = bus_interrupts,
        //.device_feature = bus_device_feature,
};
#endif /* defined(CONFIG_OTG_NOC99) */


struct usbd_bus_driver bus_driver = {
bops: &bus_ops,
};


/* ******************************************************************************************* */

/*!
 * Bus Event Handling:
 *      pcd_bus_event_handler()
 *
 */
static usbd_device_state_t event_states[DEVICE_CLOSE] = {
        STATE_UNKNOWN, STATE_INIT, STATE_ATTACHED, STATE_POWERED,
        STATE_DEFAULT, STATE_ADDRESSED, STATE_CONFIGURED, STATE_UNKNOWN,
        STATE_UNKNOWN, STATE_UNKNOWN, STATE_ADDRESSED, STATE_SUSPENDED,
        STATE_UNKNOWN, STATE_POWERED, STATE_ATTACHED,
};

static usbd_device_status_t event_status[DEVICE_CLOSE+1] = {
        USBD_UNKNOWN,   // DEVICE_UNKNOWN
        USBD_OPENING,   // DEVICE_INIT
        USBD_OPENING,   // DEVICE_CREATE
        USBD_OPENING,   // DEVICE_HUB_CONFIGURED
        USBD_RESETING,  // DEVICE_RESET
        USBD_OK,        // DEVICE_ADDRESS_ASSIGNED
        USBD_OK,        // DEVICE_CONFIGURED
        USBD_OK,        // DEVICE_SET_INTERFACE
        USBD_OK,        // DEVICE_SET_FEATURE
        USBD_OK,        // DEVICE_CLEAR_FEATURE
        USBD_OK,        // DEVICE_DE_CONFIGURED
        USBD_SUSPENDED, // DEVICE_BUS_INACTIVE
        USBD_OK,        // DEVICE_BUS_ACTIVITY
        USBD_RESETING,  // DEVICE_POWER_INTERRUPTION
        USBD_RESETING,  // DEVICE_HUB_RESET
        USBD_CLOSING,   // DEVICE_DESTROY
        USBD_CLOSED,    // DEVICE_CLOSE
};

int usbd_device_reset(struct usbd_bus_instance *bus);
int usbd_device_suspended(struct usbd_bus_instance *bus);
int usbd_device_resumed(struct usbd_bus_instance *bus);


/*!
 * @brief pcd_bus_event_handler_irq() - called to respond to various usb events
 *
 * This is called from an interrupt context.
 *
 * Used by a Bus driver to indicate an event.
 * @param bus
 * @param event
 * @param data
 */
void pcd_bus_event_handler_irq (struct usbd_bus_instance *bus, usbd_device_event_t event, int data)
{
        usbd_device_state_t last_state;
        struct pcd_instance *pcd = (struct pcd_instance *)(bus ? bus->privdata : NULL);
        usbd_device_event_t real_event = event;

        RETURN_UNLESS(bus);

        event = (event == DEVICE_ADDRESS_ASSIGNED) && !data ? DEVICE_RESET : event;

        last_state = bus->device_state;

        if (TRACE_VERBOSE)
                switch (event) {
                case DEVICE_BUS_INACTIVE:
                        TRACE_MSG0(pcd->TAG, "BUS INACTIVE");
                        break;
                default:
                        TRACE_MSG0(pcd->TAG, "default");
                        break;
                case DEVICE_UNKNOWN:
                        TRACE_MSG0(pcd->TAG, "UNKNOWN");
                        break;
                case DEVICE_BUS_ACTIVITY:
                        TRACE_MSG0(pcd->TAG, "BUS ACTIVITY");
                        break;
                case DEVICE_CONFIGURED:
                        TRACE_MSG0(pcd->TAG, "CONFIGURED");
                        break;
                case DEVICE_SET_INTERFACE:
                        TRACE_MSG0(pcd->TAG, "SET INTEERFACE");
                        break;
                case DEVICE_SET_FEATURE:
                case DEVICE_CLEAR_FEATURE:
                        TRACE_MSG0(pcd->TAG, "SET/CLEAR FEATURE");
                        break;
                }
        /* get the next bus device state by table lookup of event, we need to save the
         * current state IFF we are suspending so that it can be restored on resume.
         */
        switch (event) {
        case DEVICE_BUS_INACTIVE:
                bus->suspended_state = bus->device_state;
                /* FALL THROUGH */
        default:
                bus->device_state = event_states[event];
                break;
        case DEVICE_UNKNOWN:
                break;
        case DEVICE_BUS_ACTIVITY:
                bus->device_state = bus->suspended_state;
                break;

        case DEVICE_SET_INTERFACE:
        case DEVICE_SET_FEATURE:
        case DEVICE_CLEAR_FEATURE:
                break;
        }

        /* set the bus status by table lookup of event.
         */
        switch (event) {
        case DEVICE_BUS_ACTIVITY:
        case DEVICE_BUS_INACTIVE:
                BREAK_IF(USBD_CLOSING != bus->status);
                /* FALL THROUGH */
        default:
                bus->status = event_status[event];
                //TRACE_MSG1(pcd->TAG, "bus->status: %x", bus->status);
                //TRACE_MSG0(PCD, "DEFAULT");
                break;
        }
        TRACE_MSG5(pcd->TAG, "DEVICE_STATE event: %d data: %d state: %d -> %d status: %d",
                        event, data, last_state, bus->device_state, bus->status);

        /* we need to reset things on some transitions
         */
        switch (bus->device_state) {
        default:
                // if we lost configuration then get rid of alternate settings
                #if 0
                if (bus->alternates) {
                        LKFREE(bus->alternates);
                        bus->alternates = NULL;
                }
                bus->bNumInterfaces = 0;
                bus->device_feature_settings = 0;
                bus->ConfigurationValue = 0;
                #endif
        case STATE_CONFIGURED:
                break;
        case STATE_SUSPENDED:
                // XXX Checkme - is suspend equivalent to end of session?
                // C.f. OTG 6.5.1-6.5.3 imply that these must be reset at the end of a session or reset
                //bus->device_feature_settings &=
                //        ~(FEATURE(USB_OTG_A_HNP_ENABLE) | FEATURE(USB_OTG_B_HNP_ENABLE) | FEATURE(USB_OTG_A_ALT_HNP_ENABLE));
                break;
        }
        /* Pass the event to the bus interface driver and then the function driver and
         * any interface function drivers.
         */
        bus->driver->bops->event_handler (bus, real_event, data);

        RETURN_UNLESS (bus->function_instance);

        // XXX if (bus->function_instance->function_driver->fops->event_handler)
        // XXX        bus->function_instance->function_driver->fops->event_handler(bus->function_instance, event, data);

        switch (event) {
        case DEVICE_BUS_ACTIVITY:
                TRACE_MSG0(pcd->TAG, "schedule RESUME_BH");
                otg_workitem_start(bus->resume_bh);
                break;
        case DEVICE_BUS_INACTIVE:
                TRACE_MSG0(pcd->TAG, "schedule SUSPEND_BH");
                otg_workitem_start(bus->suspend_bh);
                break;
        case DEVICE_RESET:
                TRACE_MSG0(pcd->TAG, "schedule RESET_BH");
                otg_workitem_start(bus->reset_bh);
                break;
        default:
                break;
        }
}
/* ******************************************************************************************* */
/* Prevent overlapp of bi administrative functions mainly:
 *      pcd_bus_register
 *      pcd_bus_deregister
 */
//DECLARE_MUTEX (usbd_bi_sem);
struct usbd_bus_instance *usbd_bus;


/*!
 * @brief pcd_device_bh() -  Bottom half handler to process sent or received urbs.
 * @param data - pcd_instance pointer
 */
void *pcd_device_bh (otg_task_arg_t data)
{
        struct pcd_instance *pcd = (struct pcd_instance *) data;
        struct usbd_bus_instance *bus = pcd->bus;
        struct usbd_endpoint_instance *endpoint;
        struct usbd_urb *urb;
        int i;
        RETURN_NULL_IF (!bus || !(endpoint = bus->endpoint_array));
        //otg_led(LED2, TRUE);
        for (i = 0; i < bus->endpoints; i++) {
                struct usbd_endpoint_instance *endpoint = &bus->endpoint_array[i];
                CONTINUE_UNLESS(endpoint);
                for (   ; (urb = usbd_first_finished_urb_detached (endpoint, &endpoint->rdy)); ) {
                        #ifdef CONFIG_OTG_LATENCY_CHECK
                        otg_tick_t ticks;
                        u16 framenum;
                        otg_tick_t urb_ticks = urb->ticks;

                        /* elapsed time to get BH started and find this urb */
                        TRACE_ELAPSED(pcd->TAG, "PCD 1. BH", urb->ticks, (u32)urb);
                        otg_get_ocd_info(pcd->otg, &ticks, &framenum);
                        #endif
                        usbd_do_urb_callback (urb, urb->status);

                        #ifdef CONFIG_OTG_LATENCY_CHECK
                        /* elapsed time for the callback */
                        TRACE_ELAPSED(pcd->TAG, "PCD 2. CB", ticks, (u32)urb);

                        /* elapsed time for all processing since the urb was finished */
                        TRACE_ELAPSED(pcd->TAG, "PCD 3. TOTAL", urb_ticks, (u32)urb);
                        #endif
                }
        }
        //otg_led(LED2, FALSE);
        return NULL;
}

/*! pcd_startup_events - called for start event processing
 * @param bus - bus instance pointer
 */
void pcd_startup_events(struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG0(pcd->TAG, "BI_STARTUP");
        if (usbd_pcd_ops.startup_events) {
                TRACE_MSG0(pcd->TAG, "BI_STARTUP_EVENTS - udc");
                usbd_pcd_ops.startup_events(pcd);
        }
        else {
                TRACE_MSG0(pcd->TAG, "BI_STARTUP_EVENTS - default");
                pcd_bus_event_handler_irq (bus, DEVICE_INIT, 0);
                pcd_bus_event_handler_irq (bus, DEVICE_CREATE, 0);
                pcd_bus_event_handler_irq (bus, DEVICE_HUB_CONFIGURED, 0);
                pcd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
        }
}

/*! pcd_register_bh -
 * @param data - pcd_instance pointer
 */
void *pcd_register_bh(void *data)
{
        struct pcd_instance *pcd = (struct pcd_instance *) data;
        struct otg_instance *otg = pcd->otg;
        struct usbd_bus_instance *bus = NULL;
        struct usbd_endpoint_instance *endpoint;
        BOOL usbd_enabled = FALSE;


        TRACE_MSG1(pcd->TAG, "BUS_REGISTER_BH: pcd: %p", (int)data);
        RETURN_NULL_UNLESS(pcd);

        TRACE_MSG0(pcd->TAG, "BUS_REGISTER_BH");

        bus_driver.name = usbd_pcd_ops.name;
        bus_driver.max_endpoints = usbd_pcd_ops.max_endpoints;
        bus_driver.maxpacketsize = usbd_pcd_ops.ep0_packetsize;
        bus_driver.high_speed_capable = usbd_pcd_ops.high_speed_capable;
        //bus_driver.capabilities = usbd_pcd_ops.capabilities;
        bus_driver.bMaxPower = usbd_pcd_ops.bMaxPower;
        bus_driver.ports = usbd_pcd_ops.ports;
        // XXX bus_driver.otg_bmAttributes = tcd_ops.bmAttributes;
        bus_driver.bmAttributes = usbd_pcd_ops.bmAttributes;


        TRACE_MSG1(pcd->TAG, "usbd_pcd_ops.bmAttributes: %x", usbd_pcd_ops.bmAttributes);

        /* register this bus interface driver and create the device driver instance */
        THROW_UNLESS ( (bus = usbd_register_bus (&bus_driver, usbd_pcd_ops.ep0_packetsize)), error);
        bus->otg = otg;

        bus->privdata = (void *) pcd;
        bus->high_speed = USB_SPEED_FULL;
        pcd->bus = bus;

        #if 0
        if (serial_number_str && strlen(serial_number_str)) {
                char *sp, *dp;
                int i;
                TRACE_MSG1(pcd->TAG, "prm serial_number_str: %s", serial_number_str);
                for (sp = serial_number_str, dp = otg->serial_number, i = 0;
                                *sp && (i < (sizeof(otg->serial_number) - 1)); i++, sp++)
                        if (isxdigit(*sp)) *dp++ = toupper(*sp);
        }
        #endif

        TRACE_STRING(pcd->TAG, "otg serial_number_str: %s", pcd->otg->serial_number);

        if (pcd->otg && pcd->otg->function_name)
                TRACE_STRING(pcd->TAG, "function_name: %s", pcd->otg->function_name);


        THROW_IF ((usbd_enabled = usbd_enable_function (bus, pcd->otg->function_name, pcd->otg->serial_number)), error);

        TRACE_MSG0(pcd->TAG, "BUS_REGISTER_BH: task start");
        THROW_UNLESS((pcd->task = otg_task_init2("usbd", pcd_device_bh, pcd, pcd->TAG)), error);
        //pcd->task->debug = TRUE;
        otg_task_start(pcd->task);

        /* setup endpoint zero */
        endpoint = bus->endpoint_array + 0;
        endpoint->bEndpointAddress[0] = endpoint->bEndpointAddress[1] = 0;
        endpoint->wMaxPacketSize[0] = endpoint->wMaxPacketSize[1] = usbd_pcd_ops.ep0_packetsize;
        endpoint->rcv_transferSize = 4096;       // XXX should this be higher

        if (usbd_pcd_ops.setup_ep) usbd_pcd_ops.setup_ep (pcd, 0, endpoint);

        TRACE_MSG0(pcd->TAG, "BUS_REGISTER_BH: startup");
        pcd_startup_events (bus);

        TRACE_MSG0(pcd->TAG, "BUS_REGISTER_BH: PCD_OK");
        otg_event(pcd->otg, PCD_OK, pcd->TAG, "PCD_REGISTER_BH PCD_OK");

        TRACE_MSG0(pcd->TAG, "BUS_REGISTER_BH: FINISHED");

        CATCH(error) {

                TRACE_MSG0(pcd->TAG, "PCD_REGISTER_BH: register failed");
                otg_event(pcd->otg, enable_otg_ | PCD_OK, pcd->TAG, "BUS_REGISTER_BH");

                if (pcd->task) {
                        otg_task_exit(pcd->task);
                        pcd->task = NULL;
                }

                if (usbd_enabled) {
                        usbd_disable_function (bus);
                }

                if (bus) {
                        usbd_deregister_bus (bus);
                        pcd->bus = NULL;
                }
        }
        return NULL;
}

/*! pcd_deregister_bh -
 * @param data - pcd_instance pointer
 */
void *pcd_deregister_bh(void *data)
{
        struct pcd_instance *pcd = (struct pcd_instance *) data;
        struct usbd_bus_instance *bus;
        //struct bus_data *data;

        TRACE_MSG2(pcd->TAG, "PCD_DEREGISTER_BH: pcd: %x bus: %x", pcd, pcd ? pcd->bus : NULL);

        //Disable OTG state
        otg_event(pcd->otg, enable_otg_, pcd->TAG, "BUS_DEREGISTER_BH");

        if (pcd && (bus = pcd->bus) /*&& (usbd_bus_state_enabled == bus->bus_state)*/) {

                //otg_up_admin(pcd->task);
                //otg_task_exit(bus->task);
                //bus->task = NULL;

                //TRACE_MSG0(pcd->TAG, "BUS_DEREGISTER_BH: task stop");
                //otg_task_exit(pcd->task);
                //pcd->task = NULL;

                if (usbd_pcd_ops.disable) usbd_pcd_ops.disable (pcd);

                if (bus->device_state == STATE_ATTACHED) {
                        pcd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
                        pcd_bus_event_handler_irq (bus, DEVICE_POWER_INTERRUPTION, 0);
                        pcd_bus_event_handler_irq (bus, DEVICE_HUB_RESET, 0);
                }

                pcd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);
                pcd_disable_endpoints (bus);
                pcd_disable (bus);

                TRACE_MSG0(pcd->TAG, "BUS_DEREGISTER_BH: task stop");
                if (pcd->task) otg_task_exit(pcd->task);
                pcd->task = NULL;

                usbd_disable_function (bus);

                //if (bus->serial_number_str)
                //        lkfree (pcd->bus->serial_number_str);

                usbd_deregister_bus (bus);
                pcd->bus = NULL;

        }
        TRACE_MSG0(pcd->TAG, "sending PCD_OK");
        otg_event(pcd->otg, PCD_OK, pcd->TAG, "PCD_DEREGISTER_BH PCD_OK");
        return NULL;
}

/* ************************************************************************************* */


/*! pcd_framenum() - get current framenum
 * @param otg
 */
u16
pcd_framenum (struct otg_instance *otg)
{
        struct pcd_instance     *pcd = otg ? otg->pcd : NULL;
        RETURN_ZERO_UNLESS(otg && pcd && usbd_pcd_ops.framenum);
        return usbd_pcd_ops.framenum(pcd);
}


/*!
 * pcd_remote_wakeup() - perform remote wakeup.
 * Initiate a remote wakeup to the host.
 * @param otg - otg instance
 * @param flag - SET or RESET
 */
void pcd_remote_wakeup(struct otg_instance *otg, u8 flag)
{
        struct pcd_instance     *pcd = otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;

        struct otg_dev          *otg_dev = otg->privdata;

        TRACE_MSG1(pcd->TAG, "device_feature_settings: %04x", bus->device_feature_settings);
        RETURN_UNLESS(bus->device_feature_settings & FEATURE(USB_DEVICE_REMOTE_WAKEUP));

        TRACE_MSG1(pcd->TAG, "remote wakeup enabled flag: %d", flag);

        RETURN_UNLESS (usbd_pcd_ops.remote_wakeup);

        usbd_pcd_ops.remote_wakeup(pcd);
}

/*!
 * pcd_tcd_en_func() - enable
 * This is called to enable / disable the PCD and USBD stack.
 * @param otg - otg instance
 * @param flag - SET or RESET
 *
 * Start or stop the UDC.
 *
 * SET Called after Vbus detected and before the pullup is enabled.
 *
 * RESET after pullup disabled to turn off.
 *
 */
void
pcd_tcd_en_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance     *pcd = otg->pcd;

        int                     vbus = FALSE;

        RETURN_UNLESS (usbd_pcd_ops.vbus_status);

        vbus = usbd_pcd_ops.vbus_status(pcd);

        TRACE_MSG1(pcd->TAG, "vbus: %d", vbus);

        switch (flag) {
        case PULSE:
        case SET:
                switch (vbus) {
                case 1:
                case 2:
                        otg_event(otg, VBUS_VLD | B_SESS_VLD, otg->pcd->TAG, "PCD TCD EN");
                        break;
                default:
                        break;
                }
                break;
        }
}

/*! pcd_dp_pullup_func
 * @param otg - otg_instance pointer
 * @param flag -
 *
 * Enable or disable pullup.
 */
void pcd_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct pcd_instance     *pcd = otg->pcd;

        RETURN_UNLESS(usbd_pcd_ops.softcon);
        usbd_pcd_ops.softcon(pcd, flag);
}


/*! pcd_en_func - enable
 *
 * This is called to enable / disable the PCD and USBD stack.
 * @param otg - otg instance pointer
 * @param flag - enable/disable flag
 */
void pcd_en_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;

        //TRACE_MSG0(pcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(pcd->TAG, "PCD_EN: SET");
                // check if we can see the UDC and register, then enable the function
                if (usbd_pcd_ops.enable) usbd_pcd_ops.enable (pcd);
                break;

        case RESET:
                TRACE_MSG0(pcd->TAG, "PCD_EN: RESET");
                pcd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
                //TRACE_MSG0(pcd->TAG, "PCD_EN: DESTROY");
                //pcd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);
                TRACE_MSG0(pcd->TAG, "PCD_EN: FINISHED");
                // XXX need to set a flag here
                break;
        }
}

/*! pcd_init_func - per peripheral controller common initialization
 *
 * This is called to initialize / de-initialize the PCD and USBD stack.
 *
 * We start work items to do this.
 * @param otg - otg instance pointer
 * @param flag - operation flag
 *
 */
void pcd_init_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = otg->pcd;
        //struct usbd_bus_instance *bus = pcd->bus;
        //struct bus_data *data = NULL;

        //TRACE_MSG0(pcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(pcd->TAG, "PCD_INIT: SET - start REGISTER BH");
                //printk(KERN_INFO"%s: SET pcd->register_bh: %p\n", __FUNCTION__, pcd->register_bh);
                otg_workitem_start(pcd->register_bh);
                //TRACE_MSG0(pcd->TAG, "BUS_REGISTER_SCHEDULE: finished");
                break;

        case RESET:
                TRACE_MSG0(pcd->TAG, "PCD_INIT: RESET - start DEREGISTER BH");
                otg_workitem_start(pcd->deregister_bh);
                //TRACE_MSG0(pcd->TAG, "BUS_DEREGISTER_SCHEDULE: finished");
        }
}

/*! pcd_mod_init - per peripheral controller common initialization
 *
 * This is called to initialize / de-initialize the PCD and USBD stack.
 *
 * We start work items to do this.
 * @param otg - otg instance pointer
 *
 */
int pcd_mod_init (struct otg_instance *otg)
{
        struct pcd_instance *pcd = otg->pcd;
        //struct usbd_bus_instance *bus = pcd->bus;


        THROW_UNLESS((pcd->register_bh = otg_workitem_init("regbh", pcd_register_bh, pcd, pcd->TAG)), error);
        THROW_UNLESS((pcd->deregister_bh = otg_workitem_init("deregbh", pcd_deregister_bh, pcd, pcd->TAG)), error);
        //pcd->register_bh->debug = TRUE;
        //pcd->deregister_bh->debug = TRUE;
        return 0;

        CATCH(error) {
                if (pcd->register_bh) otg_workitem_exit(pcd->register_bh);
                if (pcd->deregister_bh) otg_workitem_exit(pcd->deregister_bh);
                pcd->register_bh = pcd->deregister_bh = NULL;
                return -EINVAL;
        }
}

/*! pcd_mod_exit - per peripheral controller common initialization
 *
 * This is called to initialize / de-initialize the PCD and USBD stack.
 *
 * We start work items to do this.
 * @param otg - otg_instance pointer
 *
 */
void pcd_mod_exit (struct otg_instance *otg)
{
        struct pcd_instance *pcd = otg->pcd;
        //struct usbd_bus_instance *bus = pcd->bus;
        //

        if (pcd->task) otg_task_exit(pcd->task);
        if (pcd->register_bh) otg_workitem_exit(pcd->register_bh);
        if (pcd->deregister_bh)  otg_workitem_exit(pcd->deregister_bh);

        pcd->task = NULL;
        pcd->register_bh = pcd->deregister_bh = NULL;
}

struct pcd_ops pcd_ops = {
        .pcd_en_func = pcd_en_func,
        .pcd_init_func = pcd_init_func,
        #ifdef CONFIG_OTG_REMOTE_WAKEUP
        .remote_wakeup_func = pcd_remote_wakeup,
        #endif /* CONFIG_OTG_REMOTE_WAKEUP */
        .framenum = pcd_framenum,
        .tcd_en_func = pcd_tcd_en_func,
        .dp_pullup_func = pcd_dp_pullup_func,
        .ticks = pcd_ticks,
        .elapsed = pcd_elapsed,
};
