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
 * otg/functions/network/eem.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/eem-if.c|20070315225839|19712
 *
 *      Copyright (c) 2005-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/eem-if.c
 * @brief This file implements the required descriptors to implement
 * a eem network device with a single interface.
 *
 * The EEM network driver implements a very simple descriptor set,
 * a single interface with two BULK data endpoints.
 *
 * The EEM protocol implements a different data encapsulation than
 * the other procotols (CDC ECM, MDLM-BLAN etc.) which can support
 * data streaming.
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include "network.h"
#include "net-os.h"

#define NTT network_fd_trace_tag

#if defined(CONFIG_OTG_NETWORK_EEM) || defined(_OTG_DOXYGEN)

/* USB EEM Configuration ******************************************************************** */

/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description eem_data_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra - CDC/EEM",
                .bInterfaceClass = USB_CLASS_COMM,
                .bInterfaceSubClass = COMMUNICATIONS_EEM_SUBCLASS,
                .bInterfaceProtocol = COMMUNICATIONS_EEM_PROTOCOL,
                .endpoints =  2,
                .endpoint_index = cdc_data_endpoint_index,
        },
};


/* EEM Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description eem_interfaces[] = {
        {
                .alternates = sizeof (eem_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = eem_data_alternate_descriptions,
        },
};


/* ********************************************************************************************* */

/*! net_fd_urb_sent_zle - callback for completed BULK ZLE URB
 *
 * Handles notification that an ZLE urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int net_fd_urb_sent_zle (struct usbd_urb *urb, int urb_rc)
{
	struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        unsigned long flags;
        void *buf;
        int rc = -EINVAL;
        int in_pkt_sz;

        TRACE_MSG6(NTT,"urb: %p urb_rc: %d length: %d queue: %d avg: %d samples: %d",
			urb, urb_rc, urb->actual_length, otg_atomic_read(&npd->queued_frames),
			npd->avg_queue_frames / npd->samples, npd->samples);

	usbd_free_urb (urb);
	rc = 0;
	return rc;
}

/*! net_fd_urb_sent_eem_bulk - callback for completed BULK xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int net_fd_urb_sent_eem_bulk (struct usbd_urb *urb, int urb_rc)
{
	struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        int rc;
        int in_pkt_sz;
        int endpoint_index = BULK_IN_A;

        rc = net_fd_urb_sent_bulk(urb, urb_rc);

	#ifdef CONFIG_OTG_NETWORK_EEM_STREAM
	/* see if we should send ZLE */
	if (otg_atomic_read(&npd->queued_frames) || (npd->network_type != network_eem))
		return rc;

	/* Nothing left to send so stop the host transfer by sending a ZLE */
        UNLESS((usbd_get_device_status(function_instance) == USBD_OK))
		return rc;

        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));
	if ((urb = usbd_alloc_urb (function_instance, endpoint_index, in_pkt_sz, net_fd_urb_sent_zle ))) {
		npd->eem_send_zle_data = 0;
                #ifdef CONFIG_OTG_NETWORK_EEM_ZLE
		urb->actual_length = 2 + (npd->eem_send_zle_data ? 1 : 0);
		TRACE_MSG2(NTT, "Sending ZLE: length: %d %d", urb->actual_length, npd->eem_send_zle_data);
		urb->buffer[0] = 0;
		urb->buffer[1] = 0;
		urb->buffer[3] = 0;
                #else /* CONFIG_OTG_NETWORK_EEM_ZLE */
                urb->actual_length = 0;
		TRACE_MSG2(NTT, "Sending ZLP: length: %d %d", urb->actual_length, npd->eem_send_zle_data);
                urb->flags |= USBD_URB_SENDZLP;
                #endif /* CONFIG_OTG_NETWORK_EEM_ZLE */

		if ((rc = usbd_start_in_urb (urb))) {
			TRACE_MSG1(NTT,"FAILED: %d", rc);
			printk(KERN_ERR"%s: FAILED: %d\n", __FUNCTION__, rc);
			urb->function_privdata = NULL;
			usbd_free_urb (urb);
		}
	}
	#endif /* CONFIG_OTG_NETWORK_EEM_STREAM */
        return rc;
}

/*! net_fd_urb_sent_eem_echo - callback for completed BULK xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int net_fd_urb_sent_eem_echo (struct usbd_urb *urb, int urb_rc)
{
	struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        int in_pkt_sz;
        unsigned long flags;

        TRACE_MSG2(NTT, "urb: %x rc: %d", urb, urb_rc);
        usbd_free_urb (urb);

        return 0;
}

/*! net_fd_start_xmit_eem
 * @brief  start sending data, Called to send a network frame via EEM.

 *
 * @param function_instance - function instance pointer
 * @param buffer - pointer to data to send
 * @param len - length of buffer
 * @param data - instance private data
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit_eem (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        int in_pkt_sz;
        u8 *cp;
        u32 crc = 0;
        int length;
        #ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
        int xmit_index = 0;
        int endpoint_index = BULK_IN_A;
        #else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
        int xmit_index = (otg_atomic_read(&npd->xmit_urbs_started[0]) <= otg_atomic_read(&npd->xmit_urbs_started[1]))
                ? 0 : 1;
        int endpoint_index = (xmit_index) ? BULK_IN_B : BULK_IN_A;
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */

        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));

        /* allocate urb 5 bytes larger than required */
        length = len + 5 + 4 + in_pkt_sz;
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, length, net_fd_urb_sent_eem_bulk )))
        {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }
        memset(urb->buffer, 0, length);
        urb->function_privdata = data;

        /* if EEM packet is multiple of packetsize we may need to append ZLE (two NULL bytes)
         */
        urb->actual_length = len + 2 + (npd->eem_send_zle_data ? 1 : 0) + 4;
	cp = npd->eem_send_zle_data ? urb->buffer + 1 : urb->buffer;
	npd->eem_send_zle_data = FALSE;

        /* EEM packet header - set bmCRC if EEM CRC is enabled */
        #if defined(CONFIG_OTG_NETWORK_EEM_CRC)
        /* little endian */
        cp[0] = (len + 4) & 0xff;
        cp[1] = 0x40 | (((len + 4) >> 8) & 0x3f);
	cp += 2;
        crc = crc32_copy(cp, buffer, len, CRC32_INIT);
        crc = ~crc;
	/* append CRC - network byte order */
	cp += len;
        *cp++ = crc & 0xff;
        *cp++ = (crc >> 8) & 0xff;
        *cp++ = (crc >> 16) & 0xff;
        *cp++ = (crc >> 24) & 0xff;
        #else /* !defined(CONFIG_OTG_NETWORK_EEM_CRC) */
        /* little endian */
        cp[0] = (len + 4) & 0xff;
        cp[1] = (((len + 4) >> 8) & 0x3f);
	cp += 2;
        memcpy (cp, buffer, len);
	/* append 0xdeadbeef sentinel - network byte order */
	cp += len;
        *cp++ = 0xde;
        *cp++ = 0xad;
        *cp++ = 0xbe;
        *cp++ = 0xef;
        #endif /* !defined(CONFIG_OTG_NETWORK_EEM_CRC) */

	#ifdef CONFIG_OTG_NETWORK_EEM_STREAM
	/* if actual length is odd then our padding will have start of ZLE but without data
	 * set flag and pad with nulls to packetsize
	 */
	npd->eem_send_zle_data = (urb->actual_length & 1);
	while (urb->actual_length % in_pkt_sz)
		urb->actual_length++;
	#else /* CONFIG_OTG_NETWORK_EEM_STREAM */
        /* send ZLP if neccessary */
        UNLESS ((urb->actual_length % in_pkt_sz)) {
                #ifdef CONFIG_OTG_NETWORK_EEM_ZLE
                TRACE_MSG0(NTT,"ZLE");
                urb->actual_length++;
                urb->actual_length++;
                #else /* CONFIG_OTG_NETWORK_EEM_ZLE */
                TRACE_MSG0(NTT,"ZLP");
                urb->flags |= USBD_URB_SENDZLP;
                #endif /* CONFIG_OTG_NETWORK_EEM_ZLE */
	}
	#endif /* CONFIG_OTG_NETWORK_EEM_STREAM */

	TRACE_MSG4(NTT, "len: %d crc: %08x actual_length: %d zle_data: %d", len, crc, urb->actual_length, npd->eem_send_zle_data);

        otg_atomic_add(urb->actual_length, &npd->queued_bytes);
        otg_atomic_inc(&npd->xmit_urbs_started[xmit_index]);
        if ((rc = usbd_start_in_urb (urb))) {
                TRACE_MSG1(NTT,"FAILED: %d", rc);
                urb->function_privdata = NULL;
                otg_atomic_sub(urb->actual_length, &npd->queued_bytes);
                otg_atomic_dec(&npd->xmit_urbs_started[xmit_index]);
                usbd_free_urb (urb);
                return rc;
        }
        return 0;
}

/*! net_fd_start_echo_eem
 * @brief start sending data, called to send a network frame via EEM.
 *
 * @param function_instance - function instance pointer
 * @param buffer - pointer to data to send
 * @param len - length of data to send
 * @param data - pointer to instance private data
 * @param eem_command - eem command
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_echo_eem (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data, u8 eem_command)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        int in_pkt_sz;
        int xmit_index = 0;
        int endpoint_index = BULK_IN_A;
        u8 *cp;
        u32 crc = 0;

        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));

        //printk(KERN_INFO"%s:\n", __FUNCTION__);

        /* allocate urb 5 bytes larger than required */
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, len + 5 + 4 + in_pkt_sz, net_fd_urb_sent_eem_echo ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }
        urb->function_privdata = data;

        /* if EEM packet is multiple of packetsize we may need to append ZLE (two NULL bytes)
         */
        urb->actual_length = len + 2 + (npd->eem_send_zle_data ? 1 : 0) + 4;
	cp = npd->eem_send_zle_data ? urb->buffer + 1 : urb->buffer;
	npd->eem_send_zle_data = FALSE;

        /* EEM packet header - set bmCRC if EEM CRC is enabled, little endian */
        cp[0] = (len + 4) & 0xff;
        cp[1] = 0x80 | (eem_command << 11) | (((len + 4) >> 8) & 0x07);
	cp += 2;
        memcpy(cp, buffer, len);

	TRACE_MSG4(NTT, "len: %d crc: %08x actual_length: %d zle_data: %d", len, crc, urb->actual_length, npd->eem_send_zle_data);

        otg_atomic_add(urb->actual_length, &npd->queued_bytes);
        otg_atomic_inc(&npd->xmit_urbs_started[xmit_index]);
        if ((rc = usbd_start_in_urb (urb))) {
                TRACE_MSG1(NTT,"FAILED: %d", rc);
                urb->function_privdata = NULL;
                otg_atomic_sub(urb->actual_length, &npd->queued_bytes);
                otg_atomic_dec(&npd->xmit_urbs_started[xmit_index]);
                usbd_free_urb (urb);
                return rc;
        }
        return 0;
}


/*! net_fd_eem_recv_data
 *
 * @brief Accumulate data from an EEM Data Packet, when finished pass to OS layer to
 * push up to network layer.
 *
 * @param function_instance - function instance pointer
 * @param eem_data_type
 * @param frame_length - frame length
 * @param urb_buffer -
 * @param remainder
 * @param shortTransfer
 *
 *
 * @return Number of bytes consumed from urb buffer.
 */
STATIC int net_fd_eem_recv_data(struct usbd_function_instance *function_instance, eem_data_type_t eem_data_type,
                int frame_length,
                u8 *urb_buffer, int remainder, BOOL shortTransfer)
{
        struct usb_network_private *npd = function_instance->privdata;
        void *os_data;
        u8 *os_buffer;
        int os_buffer_used, copy_length;
        u32 eem_crc, valid_crc;
        BOOL crc_bad;

        // XXX return MIN(remainder, frame_length);

        /* Restart filling if in the middle of receiving an EEM Data Packet.
         */
        if (npd->eem_os_buffer) {
        //printk(KERN_INFO"%s: WWWW\n", __FUNCTION__);
                os_data = npd->eem_os_data;
                os_buffer = npd->eem_os_buffer;
                os_buffer_used = npd->eem_os_buffer_used;
                eem_data_type = npd->eem_data_type;
                eem_crc = npd->eem_crc;
                npd->eem_os_data = npd->eem_os_buffer = NULL;
                frame_length = npd->eem_frame_length;
        }
        /* Otherwise allocate a new os buffer
         */
        else {
        //printk(KERN_INFO"%s: XXXX\n", __FUNCTION__);
                UNLESS((os_data = net_os_alloc_buffer(function_instance, &os_buffer, frame_length))) {
                        return MIN(remainder, frame_length);
                }
                os_buffer_used = 0;
                eem_crc = CRC32_INIT;
        }

        /* Determine copy length and then copy using memcpy or crc copy as
         * appropriate.
         */
        copy_length = MIN(remainder, frame_length - os_buffer_used);

        //printk(KERN_INFO"%s: YYYY\n", __FUNCTION__);
        switch (eem_data_type) {
        case eem_frame_crc_data:
                eem_crc = crc32_copy(os_buffer + os_buffer_used, urb_buffer, copy_length/* - 1*/, eem_crc);
                valid_crc = CRC32_GOOD;
                break;
        case eem_frame_no_crc_data:
                memcpy (os_buffer + os_buffer_used, urb_buffer, copy_length);
                urb_buffer += copy_length - 4;
                eem_crc = *urb_buffer++ << 24;
                eem_crc |= *urb_buffer++ << 16;
                eem_crc |= *urb_buffer++ << 8;
                eem_crc |= *urb_buffer++;
                valid_crc = 0xdeadbeef;
                break;
        case eem_echo_data:
        case eem_echo_response_data:
                memcpy (os_buffer + os_buffer_used, urb_buffer, copy_length);
                eem_crc = valid_crc = 0;
                break;
        default:
                return copy_length;
        }
        //printk(KERN_INFO"%s: ZZZZ\n", __FUNCTION__);
        urb_buffer += copy_length;
        remainder -= copy_length;
        os_buffer_used += copy_length;

        //printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);

        /* Check for incomplete frame and short transfer. This is an
         * error, as we are expecting more data and EEM Data cannot
         * cross a bulk transfer boundard (short packet).
         */
        if (shortTransfer && (os_buffer_used < frame_length))
                return copy_length;

        //printk(KERN_INFO"%s: BBBB\n", __FUNCTION__);

        /* Save intermediate information if not complete.
         */
        if (os_buffer_used < frame_length) {
                npd->eem_os_data = os_data;
                npd->eem_os_buffer = os_buffer;
                npd->eem_frame_length = frame_length;
                npd->eem_os_buffer_used = os_buffer_used;
                npd->eem_data_type = eem_data_type;
                npd->eem_crc = eem_crc;
                return copy_length;
        }

        //printk(KERN_INFO"%s: CCCC\n", __FUNCTION__);

        switch (eem_data_type) {
        case eem_frame_crc_data:
        case eem_frame_no_crc_data:
                //printk(KERN_INFO"%s: DDDD\n", __FUNCTION__);
                /* Push the frame up as we have completed it. */
                if ((crc_bad = (valid_crc != eem_crc)))
                        TRACE_MSG3(NTT, "CRC error %08x %08x remainder: %d", eem_crc, valid_crc, remainder);
                //printk(KERN_INFO"%s: EEEE\n", __FUNCTION__);
                if (net_fd_recv_buffer(function_instance, os_buffer, copy_length, os_data, crc_bad, 4)) {
                        TRACE_MSG0(NTT, "FAILED");
                        net_os_dealloc_buffer(function_instance, os_data, os_buffer);
                }
                //printk(KERN_INFO"%s: FFFF\n", __FUNCTION__);
                break;
        case eem_echo_data:
                /* send data back as echo response */
                if (net_fd_start_echo_eem (function_instance, os_buffer, os_buffer_used, os_data, EEM_ECHO_RESPONSE)) {
                        TRACE_MSG0(NTT, "FAILED");
                        net_os_dealloc_buffer(function_instance, os_data, os_buffer);
                }
                break;
        case eem_echo_response_data:
                /* discard */
                TRACE_MSG1(NTT, "RESPONSE len: %d", copy_length);
                break;
        default:
                return copy_length;
        }
        //printk(KERN_INFO"%s: GGGG\n", __FUNCTION__);

        /* tell caller how much data we processed
         */
        return copy_length;
}

/*! net_fd_recv_urb_eem
 *
 * @brief callback to process a received URB
 * Process incoming stream of EEM packets.
 *
 * There are three special cases:
 *
 *      1. EEM Data Packet not complete at the end of a bulk transfer.
 *      2. EEM Command Packet not complete at the end of a bulk transfer.
 *      3. EEM Packets cannot cross short transfer boundary.
 *
 * N.B. EEM 5.1 says "EEM packets shall not be split across USB transfers."
 * But we cannot determine what the maximum host transfer size is, so the
 * only hard boundary is if the transfer is terminated with a short packet,
 * Zero Length EEM Packet (ZLE) or Zero Length Packet (ZLP).
 *
 * @param urb received urb
 * @param rc dummy
 * @return non-zero for failure.
 */
int net_fd_recv_urb_eem(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        u8 *urb_buffer = urb->buffer;
        int remainder = urb->actual_length;
        u32 temmp=0;
        int endpoint_index = BULK_IN_A;

        int maximumPacketSize = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index,
                        usbd_high_speed(function_instance));
        BOOL shortTransfer = (remainder % maximumPacketSize);

        TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);

        while (remainder) {

                u8 buf0;                // first byte of EEM packet header
                u8 buf1;                // second byte of EEM packet header
                eem_data_type_t eem_data_type = eem_no_data;

                int frame_length, consumed = 0;

                /* Are we in the middle of processing an EEM Data Packet?
                 * Process this data and continue.
                 */
                if (npd->eem_os_buffer) {
                        consumed -= net_fd_eem_recv_data(function_instance, 0, 0, urb_buffer, remainder, shortTransfer);
                        remainder -= consumed;
                        urb_buffer += consumed;
                        continue;
                }

                /* Get first byte of EEM packet, there may be a fragment left
                 * from previous transfer.
                 */
                if (npd->eem_fragment_valid) {
                        buf0 = npd->eem_fragment_data;
                        buf1 = *urb_buffer++;
                        npd->eem_fragment_valid = FALSE;
                }
                else {
                        buf0 = *urb_buffer++;
                        buf1 = *urb_buffer++;
                        remainder--;

                        /* Check if this is a fragment. save if found
                         */
                        if (shortTransfer && (remainder < 1)) {
                                return 0;
                        }
                        if (remainder < 1) {
                                npd->eem_fragment_valid = TRUE;
                                npd->eem_fragment_data = buf0;
                                return 0;
                        }
                }

		TRACE_MSG3(NTT, "remainder: %d EEM: %02x %02x", remainder, buf1, buf0);

                /* Check for a Zero Length EEM (ZLE).
                 * A null byte, following by any byte, Skip both and continue if found.
                 */
                UNLESS (buf0 || buf1) {
                        remainder--;
                        //TRACE_MSG1(NTT, "ZLE: %02x", buf1);
                        continue;
                }

                /* Check for an EEM Command Packet.
                 */
                if (buf1 & 0x80) {

                        u8 bmEEMCmd = (buf1 >> 3) & 0x7;
                        u8 bmEEMCmdParam = ((buf1 & 0x7) << 8) | buf0;
                        remainder --;

                        TRACE_MSG3(NTT, "bmEEMCmd: %d bmEEMCmdParam: %06x remainder: %d", bmEEMCmd, bmEEMCmdParam, remainder);

                        /* XXX Still todo...
                         */
                        switch (bmEEMCmd) {
                                /* host sent echo, get data and send back as echo response */
                        case EEM_ECHO:
                                eem_data_type = eem_echo_data;
                                break;

                                /* receiving response from echo we sent to host */
                        case EEM_ECHO_RESPONSE:
                                eem_data_type = eem_echo_response_data;
                                break;

                                /* host has timed out response complete hint command */
                        case EEM_TICKLE:
                                TRACE_MSG0(NTT, "EEM TICKLE");
                                break;

                                /* These are not allowed from Host */
                        case EEM_SUSPEND_HINT:
                        case EEM_RESPONSE_HINT:
                        case EEM_RESPONSE_COMPLETE_HINT:
                                TRACE_MSG1(NTT, "EEM COMMAND - BAD %02x", bmEEMCmd);
                                break;

                        }
                        /* if not echo or echo response we are finished */
                        CONTINUE_UNLESS((EEM_ECHO == bmEEMCmd) || (EEM_ECHO_RESPONSE == bmEEMCmd));

                        /* echo or echo response, get data length */
                        frame_length = ((buf1 & 0x07) << 8) | buf0;
                }
                /* otherwise must be EEM Data */
                else {
                        /* Must be an EEM Data Packet. Determine frame length,
                         * advance past header and process data.
                         */
                        u32 bmCRC = bmCRC = buf1 & 0x40;
                        frame_length = ((buf1 & 0x3f) << 8) | buf0;
                        remainder--;
                        eem_data_type = bmCRC ? eem_frame_crc_data : eem_frame_no_crc_data;
                }

                /* process received data - either frame or echo or echo response */
                consumed = net_fd_eem_recv_data(function_instance, eem_data_type, frame_length, urb_buffer,
                                remainder, shortTransfer);
                remainder -= consumed;
                urb_buffer += consumed;
        }

#if 0 //CONFIG_OTG_LATENCY_CHECK
        otg_get_ocd_info(NULL, &urb->goto_net_ticks, NULL);
        //urb->goto_net_ticks=__raw_readl(MXC_GPT_GPTCNT);
/*Calculate BH waiting time*/
 LTRACE_MSG4(NTT, "urb pointer:%x, rcv urb:%x, bh start:%x,goto net:%x",urb, urb->urb_rcved_ticks,urb->bh_start_ticks,urb->goto_net_ticks);
        if ( urb->bh_start_ticks < urb->urb_rcved_ticks ) temmp=(0xffffffff- urb->urb_rcved_ticks + urb->bh_start_ticks);
        else temmp=urb->bh_start_ticks - urb->urb_rcved_ticks;

        temmp=10000*temmp/LATCH; // latch=10MS, SO temmp should be in uS
        LTRACE_MSG4(NTT, "urbrcved=>bhstart time(uS):%d,  Start:%x, End:%x, LATCH:%x",temmp,urb->urb_rcved_ticks,urb->bh_start_ticks,LATCH);


/*Calculate BH process time*/
        if (urb->goto_net_ticks < urb->bh_start_ticks) temmp=(0xffffffff- urb->bh_start_ticks + urb->goto_net_ticks);
        else temmp=urb->goto_net_ticks - urb->bh_start_ticks;

        temmp=10000*temmp/LATCH; // latch=10MS, SO temmp should be in uS
        LTRACE_MSG4(NTT, "bh=>net time(uS):%d,  Start:%x, End:%x, LATCH:%x",temmp,urb->bh_start_ticks,urb->goto_net_ticks,LATCH);
#endif
        return 0;
}

/* ********************************************************************************************* */
#ifdef CONFIG_OTG_NETWORK_START_SINGLE
#define NETWORK_START_URBS 1
#else
#define NETWORK_START_URBS 2
#endif

/*! net_fd_start_recv_eem - start recv urb(s)
 *
 * @param function_instance - pointer to this function instance
 * @return none
 */
int net_fd_start_recv_eem(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        int i;
        int network_start_urbs = NETWORK_START_URBS;
        int hs = usbd_high_speed(function_instance);
        int endpoint_index = BULK_OUT_A;

        if (hs)
                network_start_urbs = NETWORK_START_URBS * 3;

        for (i = 0; i < network_start_urbs; i++) {
                struct usbd_urb *urb;
                BREAK_IF(!(urb = usbd_alloc_urb(function_instance, endpoint_index,
                                                usbd_endpoint_transferSize(function_instance, endpoint_index, hs),
                                                net_fd_recv_urb)));
                TRACE_MSG5(NTT,"i: %d urb: %p priv: %p npd: %p function: %p",
                                i, urb, urb->function_privdata, npd, function_instance);

                //urb->function_privdata = npd;
                if (usbd_start_out_urb(urb)) {
                        urb->function_privdata = NULL;
                        usbd_free_urb(urb);
                }
        }
        return 0;
}
/* ********************************************************************************************* */

/*! eem_fd_function_enable
 * @brief enable the function driver
 * Called for usbd_function_enable() from usbd_register_device()
 *
 * @param function_instance - function instance pointer
 * @return int to indicate operation result
 */

int eem_fd_function_enable (struct usbd_function_instance *function_instance)
{
        int rc;
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        rc = net_fd_function_enable(function_instance, network_eem, net_fd_recv_urb_eem,
                        net_fd_start_xmit_eem,
                        net_fd_start_recv_eem, 0);
        return rc;
}

/*!
 * eem_fd_set_configuration - called to indicate set configuration request was received
 * @param function_instance
 * @param configuration
 * @return int
 */
int eem_fd_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        char * test = "This is a test.";
        int rc;
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        rc = net_fd_set_configuration(function_instance, configuration);
        RETURN_EINVAL_IF(rc);
        net_fd_start_echo_eem (function_instance, test, strlen(test) + 1, NULL, EEM_ECHO);
        return 0;
}


/* ********************************************************************************************* */
/*! eem_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations eem_fd_function_ops = {
        .function_enable = eem_fd_function_enable,
        .function_disable = net_fd_function_disable,

        .device_request = net_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .set_configuration = eem_fd_set_configuration,
        .set_interface = NULL,
        .reset = net_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};


/*! function driver description
 */
struct usbd_interface_driver eem_interface_driver = {
        .driver = {
                .name = "cdc-eem-if",
                .fops = &eem_fd_function_ops, },
        .interfaces = sizeof (eem_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = eem_interfaces,
        .endpointsRequested = 2,
        .requestedEndpoints = cdc_data_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_EEM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_EEM_PROTOCOL,
        .iFunction = "Belcarra USBLAN - EEM",
};

/* ********************************************************************************************* */

/*! int eem_mod_init ()
 * @brief eem_mod_init
 * @return int
 */
int eem_mod_init (void)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return usbd_register_interface_function (&eem_interface_driver, "cdc-eem-if", NULL);
}

/*! eem_mod_exit()
* @brief eem_mod_exit
 * @return none
 */
void eem_mod_exit(void)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        usbd_deregister_interface_function (&eem_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_EEM */
/*!
 * @brief eem_mod_init
 * @return int
 */
int eem_mod_init (void)
{
        return 0;
}

/*!
 * @brief  eem_mod_exit
 * @return none
 */
void eem_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_EEM */
