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
 * otg/functions/network/net-fd.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/net-fd.c|20070814184652|40728
 *
 *      Copyright (c) 2002-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/net-fd.c
 * @brief The lower edge (USB Device Function) implementation of
 * the Network Function Driver. This performs the core protocol
 * handling and data encpasulation.
 *
 * This implements the lower edge (USB Device) layer of the Network Function
 * Driver. Specifically the data encapsulation, envent and protocol handlers.
 *
 * This network function driver intended to interoperate with
 * Belcarra's USBLAN Class drivers.
 *
 * These are available for Windows, Linux and Mac OSX. For more
 * information and to download a copy for testing:
 *
 *      http://www.belcarra.com/usblan/
 *
 * Alternately it should be compatible with any CDC-ECM or CDC-EEM
 * Class driver.
 *
 * Experimental Streaming mode is available for EEM. This simply sends
 * all frames padded so that they do not terminate the transfer at the
 * receiving end. The bulk sent callback will send a ZLE frame IFF there
 * is not pending traffic. This will terminate the transfer at the host.
 *
 * The host should attempt to match it's receiving buffer to a size that
 * optimizes the amount of data without unduly impacting latency. For a
 * full speed host, something around 35*64 or 2240 bytes would be 1.9mS of
 * data and allow for about .1mS for servicing.
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>
//#include <linux/list.h>
//#include <linux/ctype.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include "network.h"
#include "net-os.h"
#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
#include "fermat.h"
#endif

#define TRACE_VERBOSE_SEND 0
#define TRACE_VERBOSE_RECV 0
#define TRACE_VERY_VERBOSE 0

static char * local_dev_addr_str;
static char * remote_dev_addr_str;
static BOOL   override_MAC;
static int infrastructure_device;

/* ********************************************************************************************** */
#if !defined (CONFIG_OTG_NETWORK_INTERVAL)
#define CONFIG_OTG_NETWORK_INTERVAL 1
#endif /* !defined (CONFIG_OTG_NETWORK_INTERVAL) */

/*! Endpoint Request List
 */

#if !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT_A, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_A, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_INTERVAL, },
        { 0, },
};
u8 net_fd_endpoint_index[ENDPOINTS] = { BULK_OUT_A, BULK_IN_A, INT_IN, };

#elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT_A, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_A, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_B, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_INTERVAL, },
        { 0, },
};
u8 net_fd_endpoint_index[ENDPOINTS] = { BULK_OUT_A, BULK_IN_A, BULK_OUT_B, BULK_IN_B, INT_IN, };


#elif !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT_A, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_A, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_OUT_B, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_INTERVAL, },
        { 0, },
};
u8 net_fd_endpoint_index[ENDPOINTS] = { BULK_OUT_A, BULK_IN_A, BULK_OUT_B, BULK_IN_B, INT_IN, };


#elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT_A, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_A, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_OUT_B, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_B, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_INTERVAL, },
        { 0, },
};
u8 net_fd_endpoint_index[ENDPOINTS] = { BULK_OUT_A, BULK_IN_A, BULK_OUT_B, BULK_IN_B, INT_IN, };

#endif /* defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT) */


/*! Endpoint Request List
 */
struct usbd_endpoint_request cdc_data_endpoint_requests[2+1] = {
        { BULK_OUT_A, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN_A, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { 0, },
};
u8 cdc_data_endpoint_index[2] = { BULK_OUT_A, BULK_IN_A, };

/*! Endpoint Request List
 */
struct usbd_endpoint_request cdc_int_endpoint_requests[1+1] = {
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_INTERVAL, },
        { 0, },
};

u8 cdc_int_endpoint_index[1] = { INT_IN, };


/* ********************************************************************************************** */

u32 *network_crc32_table;

/*! make_crc_table
 * @brief Generate the crc32 table
 *
 * @return non-zero if malloc fails
 */
STATIC int make_crc_table(void)
{
        u32 n;
        RETURN_ZERO_IF(network_crc32_table);
        RETURN_ENOMEM_IF(!(network_crc32_table = (u32 *)ckmalloc(256*4)));
        for (n = 0; n < 256; n++) {
                int k;
                u32 c = n;
                for (k = 0; k < 8; k++) {
                        c =  (c & 1) ? (CRC32_POLY ^ (c >> 1)) : (c >> 1);
                }
                network_crc32_table[n] = c;
        }
        return 0;
}

/* ********************************************************************************************** */

/*! net_fd_urb_sent_int - callback for completed INT URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb - pointer to urb to send
 * @param urb_rc
 *
 * @return non-zero for failure.
 */
STATIC int net_fd_urb_sent_int (struct usbd_urb *urb, int urb_rc)
{
        //int rc = -EINVAL;
	struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;

        //TRACE_MSG3(NTT,"urb: %p npd: %p urb_rc: %d", urb, npd, urb_rc);

        npd->int_urb = NULL;
        usbd_free_urb (urb);
        return 0;
}

/*! net_fd_send_int_notification - send an interrupt notification response
 *
 * Generates a response urb on the notification (INTERRUPT) endpoint.
 *
 * This is called from either a scheduled task or from the process context
 * that calls network_open() or network_close().
 * This must be called with interrupts locked out as net_fd_event_handler can
 * change the NETWORK_CONFIGURED status
 *
 * @param function_instance - pointer to function instance
 * @param connected - connect status
 * @param data -
 *
 * @return none
 *
 */
void net_fd_send_int_notification(struct usbd_function_instance *function_instance, int connected, int data)
{
        struct usb_network_private *npd = function_instance ? function_instance->privdata : NULL;
        struct usbd_urb *urb;
        struct cdc_notification_descriptor *cdc;
        int rc;

        //TRACE_MSG3(NTT,"npd: %p function: %p flags: %04x", npd, function_instance, npd->flags);

        do {
                BREAK_IF(!function_instance);

                BREAK_IF(npd->network_type != network_blan);
                BREAK_IF(!npd->have_interrupt);

                BREAK_IF(!(npd->flags & NETWORK_CONFIGURED));

                TRACE_MSG3(NTT,"connected: %d network: %d %d", connected,
                                npd->network_type, network_blan);

                BREAK_IF(usbd_get_device_status(function_instance) != USBD_OK);

                BREAK_IF(!(urb = usbd_alloc_urb (function_instance, INT_IN,
                                                sizeof(struct cdc_notification_descriptor), net_fd_urb_sent_int)));

                urb->actual_length = sizeof(struct cdc_notification_descriptor);
                memset(urb->buffer, 0, sizeof(struct cdc_notification_descriptor));

                cdc = (struct cdc_notification_descriptor *)urb->buffer;

                cdc->bmRequestType = 0xa1;

                if (data) {
                        cdc->bNotification = 0xf0;
                        cdc->wValue = 1;
                }
                else {
                        cdc->bNotification = 0x00;
                        cdc->wValue = connected ? 0x01 : 0x00;
                }
                cdc->wIndex = 0x00; // XXX interface - check that this is correct


                npd->int_urb = urb;
                TRACE_MSG1(NTT,"int_urb: %p", urb);
                BREAK_IF (!(rc = usbd_start_in_urb (urb)));

                TRACE_MSG1(NTT,"usbd_start_in_urb failed err: %x", rc);
                npd->int_urb = NULL;
                usbd_free_urb (urb);

        } while(0);
}

/* ********************************************************************************************** */

/*! net_fd_start_xmit - start sending a buffer
 *
 * Called with net_os_mutex_enter()d.
 *
 * @param function_instance - pointer to function instance
 * @param buffer - container to data to transmit
 * @param len - data length
 * @param data
 *
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;

        if (TRACE_VERBOSE_SEND)
                TRACE_MSG4(NTT, "os: %p buffer: %p len: %d flags: %04x", data, buffer, len, npd->flags);
        RETURN_EUNATCH_UNLESS(npd->flags & NETWORK_CONFIGURED);
        RETURN_EINVAL_IF(usbd_get_device_status(function_instance) != USBD_OK);

	//TRACE_MSG2(NTT, "queued: %d bytes: %d", npd->queued_frames, npd->queued_bytes);

        if (TRACE_VERBOSE_SEND) {
                TRACE_NSEND(NTT, 32, buffer);
                if (TRACE_VERY_VERBOSE) {
                        TRACE_SEND(NTT, len, buffer);
                }
        }

        UNLESS ((rc = npd->net_start_xmit (function_instance, buffer, len, data)))
                otg_atomic_inc(&npd->queued_frames);

        return rc;
}

/* ********************************************************************************************** */

int net_fd_check_tp_response(struct usbd_function_instance *function_instance, u8 *buffer, int length);
int net_fd_check_rarp_response(struct usbd_function_instance *function_instance, u8 *buffer, int length);

/*! net_fd_recv_buffer - forward a received URB, or clean up after a bad one.
 *
 * Common point for transmitting data to net-fd upper OS layer for pushing
 * to Host OS Network Layer.
 *
 * @param function_instance - pointer to functionn instance
 * @param os_buffer - pointer to os buffer
 * @param length - os buffer length
 * @param os_data -
 * @param crc_bad
 * @param trim
 * @return 0 for exception
 */
int net_fd_recv_buffer(struct usbd_function_instance *function_instance, u8 *os_buffer, int length,
                void *os_data, int crc_bad, int trim)
{
        struct usb_network_private *npd = function_instance->privdata;

        if (TRACE_VERBOSE_RECV) {

                TRACE_MSG6(NTT, "os_buffer: %x length: %d os_data: %x crc_bad: %d trim: %d flags: %04x",
                                os_buffer, length, os_data, crc_bad, trim, npd->flags);

                TRACE_NRECV(NTT, 32, os_buffer);
                TRACE_MSG0(NTT, "--");
                if (TRACE_VERY_VERBOSE) {
                        TRACE_RECV(NTT, length, os_buffer);
                }
        }

	#if defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
        /* check for RARPD request */
        if (net_fd_check_rarpd_request(function_instance, os_buffer, length)) {

                TRACE_MSG0(NTT, "RARPD REQUEST");

                /* send reply or ignore and send our own request */
                ((npd->flags & NETWORK_INFRASTRUCTURE) ?
                 net_fd_send_rarpd_reply : net_fd_send_rarpd_request) (function_instance);

                THROW(handled);
        }

        /* check for rarpd response IFF not infrastruture */
        UNLESS ((npd->flags & NETWORK_INFRASTRUCTURE) || !net_fd_check_rarpd_reply(function_instance, os_buffer, length)) {
                TRACE_MSG0(NTT, "RARPD REPLY RESPONSE");

                /* configure */
                net_os_config(function_instance);
                net_os_hotplug(function_instance);

                #if defined(CONFIG_OTG_NETWORK_RFC868_AUTO_CONFIG)
                net_fd_send_tp_request(function_instance);
                #endif /* defined(CONFIG_OTG_NETWORK_RFC868_AUTO_CONFIG) */
                THROW(handled);
        }

	#if defined(CONFIG_OTG_NETWORK_RFC868_AUTO_CONFIG)
        UNLESS ((npd->flags & NETWORK_INFRASTRUCTURE) || !net_fd_check_tp_response(function_instance, os_buffer, length)) {
                TRACE_MSG0(NTT, "TIME PROTOCOL RESPONSE");

                /* set time */
                net_os_settime(function_instance, ntohl(npd->rfc868time));
                THROW( handled);
        }

	#endif /* defined(CONFIG_OTG_NETWORK_RFC868_AUTO_CONFIG) */
	#endif /* defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */

        return (net_os_recv_buffer(function_instance, os_data, os_buffer, crc_bad, length, trim));

        CATCH(handled) {
                /* de-allocate os buffer */
                net_os_dealloc_buffer(function_instance, os_data, os_buffer);
                return 0;
        }
}

/* ********************************************************************************************** */

/*! net_fd_recv_urb - callback to process a received URB
 *
 * @param urb - pointer to copy of received urb,
 * @param rc - receiving urb result code
 *
 * @return non-zero for failure.
 */
int net_fd_recv_urb(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;

        TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);

#ifdef CONFIG_OTG_LATENCY_CHECK
        otg_get_ocd_info(NULL, &urb->bh_start_ticks, NULL);
       //urb->bh_start_ticks=__raw_readl(MXC_GPT_GPTCNT);
       //LTRACE_MSG2(NTT, "Callback urb  pointer:%x, ticks:%x",urb,urb->bh_start_ticks);

#endif
        #if 0
	TRACE_MSG8(NTT, "[%02x %02x %02x %02x %02x %02x %02x %02x]",
			urb->buffer[0], urb->buffer[1],
			urb->buffer[2], urb->buffer[3],
			urb->buffer[4], urb->buffer[5],
			urb->buffer[6], urb->buffer[7]
			);
	TRACE_MSG8(NTT, "[%02x %02x %02x %02x %02x %02x %02x %02x]",
			urb->buffer[0+8], urb->buffer[1+8],
			urb->buffer[2+8], urb->buffer[3+8],
			urb->buffer[4+8], urb->buffer[5+8],
			urb->buffer[6+8], urb->buffer[7+8]
			);
        #endif

        if (TRACE_VERBOSE_RECV)
                TRACE_NRECV(NTT, MIN(32, urb->actual_length), urb->buffer);

        if (urb->status == USBD_URB_OK)
                npd->net_recv_urb(urb, rc);

        urb->status = USBD_OK;
        return usbd_start_out_urb (urb);
}

/* ********************************************************************************************** */

/*! net_fd_device_request - process a received SETUP URB
 *
 * Processes a received setup packet and CONTROL WRITE data.
 * Results for a CONTROL READ are placed in urb->buffer.
 *
 * @param function_instance - pointer to function instance
 * @param request - pointer to usbd_device_request
 *
 * @return non-zero for failure.
 */
int net_fd_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request)
{
        //struct usb_network_private *npd = function_instance->privdata;
        //struct usbd_urb *urb;
        //int index;

        /* Verify that this is a USB Class request per CDC specification or a vendor request.
         */
        RETURN_ZERO_IF (!(request->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_VENDOR)));

        return -EINVAL;
}


/* ********************************************************************************************** */


/*! net_fd_endpoint_cleared -
 */
void net_fd_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        //TRACE_MSG1(NTT, "bEndpointAddress: %02x", bEndpointAddress);
}


/*! net_fd_urb_sent_bulk - callback for completed BULK xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int net_fd_urb_sent_bulk (struct usbd_urb *urb, int urb_rc)
{
	struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        void *buf;
        int rc = -EINVAL;
        int endpoint_index = urb->endpoint_index;


	npd->avg_queue_frames += otg_atomic_read(&npd->queued_frames);
	npd->samples++;

        //TRACE_MSG6(NTT,"urb: %p urb_rc: %d length: %d queue: %d avg: %d samples: %d",
	//		urb, urb_rc, urb->actual_length,
	//		npd->queued_frames,
	//		npd->avg_queue_frames / npd->samples,
	//		npd->samples);

	otg_atomic_dec(&npd->queued_frames);
        otg_atomic_sub(urb->actual_length, &npd->queued_bytes);
        #ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
        #else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
        //otg_atomic_dec(&npd->xmit_urbs_started[endpoint_index]);
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */
	do {

		BREAK_IF(!urb);
                buf = urb->function_privdata;
                urb->function_privdata = NULL;

                #ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
                TRACE_MSG6(NTT,"urb: %p buf: %p endpoint_index: %d buffer: %d alloc: %d actual: %d",
                                urb, buf, endpoint_index,
                                urb->buffer_length,
                                urb->alloc_length,
                                urb->actual_length
                                );
                #else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
                TRACE_MSG7(NTT,"urb: %p buf: %p endpoint_index: %d started: %d buffer: %d alloc: %d actual: %d",
                                urb, buf, endpoint_index,
                                otg_atomic_read(&npd->xmit_urbs_started[endpoint_index]),
                                urb->buffer_length,
                                urb->alloc_length,
                                urb->actual_length
                                );
                #endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */

                net_os_xmit_done(urb->function_instance, buf, urb_rc);
                #if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
                #else /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */
                #ifdef CONFIG_OTG_NETWORK_XMIT_OS
                urb->buffer = NULL;
                #endif /* CONFIG_OTG_NETWORK_XMIT_OS */
                #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */
                /* Now urb has it's own buffer*/
                usbd_free_urb (urb);
                rc = 0;

        } while (0);

	return rc;
}


/* ********************************************************************************************** */

/*! hexdigit -
 *
 * Converts characters in [0-9A-F] to 0..15, characters in [a-f] to 42..47, and all others to 0.
 *
 * @param c - character to convert
 * @return the converted decimal value
 */
 u8 hexdigit (char c)
{
        return isxdigit (c) ? (isdigit (c) ? (c - '0') : (c - 'A' + 10)) : 0;
}

/*! set_address -generate a device address from mac-address_str
 * @param mac_address_str - sting used to generate device address
 * @param dev_addr - pointer to generated device address
 */
void set_address(char *mac_address_str, u8 *dev_addr)
{
        int i;
        if (mac_address_str && strlen(mac_address_str)) {
                for (i = 0; i < NET_ETH_ALEN; i++) {
                        dev_addr[i] =
                                hexdigit (mac_address_str[i * 3]) << 4 |
                                hexdigit (mac_address_str[i * 3 + 1]);
                }
        }
        else {
                otg_get_random_bytes(dev_addr, NET_ETH_ALEN);
                dev_addr[0] = (dev_addr[0] & 0xfe) | 0x02;
                mac_address_str = "RANDOM";
        }
        TRACE_MSG7(NTT, "net addr: %02x:%02x:%02x:%02x:%02x:%02x %s",
                        dev_addr[0], dev_addr[1], dev_addr[2],
                        dev_addr[3], dev_addr[4], dev_addr[5],
                        mac_address_str);
}

/*! net_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 *
 * @param function_instance - pointer to this function instance
 * @param network_type
 * @param net_recv_urb
 * @param net_start_xmit
 * @param net_start_recv
 * @param recv_urb_flags
 *
 * @return int - non-zero for exception
 */

int net_fd_function_enable (struct usbd_function_instance *  function_instance, network_type_t network_type,
                net_recv_urb_proc_t net_recv_urb,
                net_start_xmit_proc_t net_start_xmit,
                net_start_recv_proc_t net_start_recv,
                u32 recv_urb_flags
                )
{
        struct usb_network_private *npd = NULL;

        /* This will link the usb_network_private structure into function_instance->privdata */
        net_os_enable(function_instance);
        npd = function_instance->privdata;

        npd->network_type = network_type;
        npd->net_recv_urb = net_recv_urb;
        npd->net_start_xmit = net_start_xmit;
        npd->net_start_recv = net_start_recv;
        npd->recv_urb_flags = recv_urb_flags;
        npd->override_MAC   = override_MAC;

        net_os_mutex_enter(function_instance);

        #if 1
        set_address(local_dev_addr_str, npd->local_dev_addr);

        TRACE_MSG7(NTT, "net npd->local_ addr: %02x:%02x:%02x:%02x:%02x:%02x org MAC string:%s",
                        npd->local_dev_addr[0], npd->local_dev_addr[1], npd->local_dev_addr[2],
                        npd->local_dev_addr[3], npd->local_dev_addr[4], npd->local_dev_addr[5],
                        local_dev_addr_str);
        npd->local_dev_set = TRUE;
        #else
        if (local_dev_addr_str && strlen(local_dev_addr_str)) {
                set_address(local_dev_addr_str, npd->local_dev_addr);
                npd->local_dev_set = TRUE;
        }
        #endif
        if (remote_dev_addr_str && strlen(remote_dev_addr_str)) {
                set_address(remote_dev_addr_str, npd->remote_dev_addr);
                npd->remote_dev_set = TRUE;
        }

        npd->have_interrupt = usbd_endpoint_bEndpointAddress(function_instance, INT_IN,
                        usbd_high_speed(function_instance)) ? 1 : 0;

        npd->flags |= NETWORK_ENABLED | (infrastructure_device ? NETWORK_INFRASTRUCTURE : 0);

        net_os_mutex_exit(function_instance);
        CATCH(error) {
                // XXX MODULE UNLOCK HERE
                return -EINVAL;
        }
        return 0;
}

/*! net_fd_function_disable - disable the function driver
 *
 * @param function_instance - pointer to function instance
 *
 * @return none
 *
 */
void net_fd_function_disable (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        net_os_mutex_enter(function_instance);
        npd->flags &= ~NETWORK_ENABLED;
        net_os_mutex_exit(function_instance);

        TRACE_MSG0(NTT,"--");
	net_fd_stop (function_instance);

        if (npd->eem_os_buffer)
                net_os_dealloc_buffer(function_instance, npd->eem_os_data, npd->eem_os_buffer);

        /* this will disconnect function_instance->privdata */
        net_os_disable(function_instance);

}

/* ********************************************************************************************** */

extern void net_fd_send_rarp_request(struct usbd_function_instance *function_instance);

/*!
 * net_fd_start - start network
 * @param function_instance
 * @return int
 */
int net_fd_start (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        int numm;
        TRACE_MSG0(NTT,"entered");
        npd->flags |= NETWORK_CONFIGURED;

        if ((npd->network_type == network_blan) && (npd->flags & NETWORK_OPEN))
                net_os_send_notification_later(function_instance);

        net_os_carrier_on(function_instance);


        /* Let the OS layer know, if it's interested. */
        net_os_config(function_instance);
        net_os_hotplug(function_instance);

        /* send RARPD request if not an infrastructure device */
        #if 0
	#if defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
	UNLESS(infrastructure_device)
		net_fd_send_rarp_request(function_instance);
	#endif /* defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
        #endif
        TRACE_MSG1(NTT, "CONFIGURED npd->flags: %04x", npd->flags);
        return 0;
}

/*!
 * @brief  net_fd_stop - stop network
 * @param function_instance
 * @return int
 */
int net_fd_stop (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;

        TRACE_MSG0(NTT,"entered");

        // Return if argument is null.

        // XXX flush

        npd->flags &= ~NETWORK_CONFIGURED;
        npd->int_urb = NULL;

        // Disable our net-device.
        // Apparently it doesn't matter if we should do this more than once.

        net_os_carrier_off(function_instance);

        // If we aren't already tearing things down, do it now.
        if (!(npd->flags & NETWORK_DESTROYING)) {
                npd->flags |= NETWORK_DESTROYING;
                //npd->device = NULL;
        }

        npd->seen_crc = 0;

        // Let the OS layer know, if it's interested.
        net_os_config(function_instance);
        net_os_hotplug(function_instance);

        // XXX flush
        // Release any queued urbs
        TRACE_MSG1(NTT, "RESET npd->flags: %04x", npd->flags);
        return 0;
}

/*!
 * net_fd_set_configuration - called to indicate set configuration request was received
 * @param function_instance
 * @param configuration
 * @return int
 */
int net_fd_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usb_network_private *npd = function_instance->privdata;
        int hs = usbd_high_speed(function_instance);

        TRACE_MSG2(NTT, "CONFIGURED: %d ip_addr: %08x", configuration, npd->ip_addr);

        if (npd->eem_os_buffer)
                net_os_dealloc_buffer(function_instance, npd->eem_os_data, npd->eem_os_buffer);

        npd->eem_os_data = npd->eem_os_buffer = NULL;
        //npd->flags |= NETWORK_CONFIGURED;
	npd->altsetting = 0;
        npd->ip_addr = 0;
        npd->max_recv_urbs = hs ? NETWORK_START_URBS * 3 : NETWORK_START_URBS;

        return net_fd_start(function_instance);
}

/*!
 * @brief  net_fd_reset - called to indicate bus has been reset
 * @param function_instance
 * @return int
 */
int net_fd_reset (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;

        #ifndef CONFIG_OTG_NETWORK_DOUBLE_OUT
        int numm = 0;
        numm = usbd_endpoint_urb_num (function_instance, BULK_OUT_A);
        TRACE_MSG2(NTT,"RESET %08x urb_number: %d",npd->ip_addr, numm);
        #else /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        int numm_a = 0;
        int numm_b = 0;
        numm_a = usbd_endpoint_urb_num (function_instance, BULK_OUT_A);
        numm_b = usbd_endpoint_urb_num (function_instance, BULK_OUT_B);
        TRACE_MSG3(NTT,"RESET %08x urb_number: %d:%d",npd->ip_addr, numm_a, numm_b);
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        npd->ip_addr = 0;
	return net_fd_stop (function_instance);
}


/*!
 * @brief net_fd_suspended - called to indicate bus has been suspended
 * @param function_instance
 * @return int
 */
int net_fd_suspended (struct usbd_function_instance *function_instance)
{

        TRACE_MSG0(NTT, "SUSPENDED");
	return net_fd_stop (function_instance);
}


/*!
 * net_fd_resumed - called to indicate bus has been resumed
 * @param function_instance
 * @return int
 */
int net_fd_resumed (struct usbd_function_instance *function_instance)
{
        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;

        TRACE_MSG0(NTT, "RESUMED");
        return net_fd_start(function_instance);
}



/* ********************************************************************************************** */

#if 0
/*! macstrtest -
 */
static int macstrtest(char *mac_address_str)
{
        int l = 0;

        if (mac_address_str) {
                l = strlen(mac_address_str);
        }
        return ((l != 0) && (l != 12));
}
#endif

/*!
 * @brief net_fd_init - function driver usb part intialization
 *
 * @param info_str
 * @param local
 * @param remote
 * @param override_mac
 * @param override_personal
 * @param override_infrastructure
 * @return non-zero for failure.
 */
int net_fd_init(char *info_str, char *local, char *remote, BOOL override_mac, BOOL override_personal, BOOL override_infrastructure)
{
        local_dev_addr_str = local;
        remote_dev_addr_str = remote;
        if (override_mac) override_MAC= TRUE;
	#if defined(OTG_NETWORK_INFRASTRUCTURE)
	infrastructure_device = !override_personal;
	#else /* defined(OTG_NETWORK_INFRASTRUCTURE) */
	infrastructure_device = override_infrastructure;
	#endif /* defined(OTG_NETWORK_INFRASTRUCTURE) */

	TRACE_MSG3(NTT, "local: %s remote: %s Mode; %s Device",
			local_dev_addr_str ? local_dev_addr_str : "",
			remote_dev_addr_str ? remote_dev_addr_str : "",
			infrastructure_device ? "Infrastructure" : "Personal"
		  );

        return make_crc_table();
}

/*!
 * @brief net_fd_exit - driver exit
 *
 * Cleans up the module. Deregisters the function driver and destroys the network object.
 */
void net_fd_exit(void)
{
        if (network_crc32_table) {
                lkfree(network_crc32_table);
                network_crc32_table = NULL;
        }
}
