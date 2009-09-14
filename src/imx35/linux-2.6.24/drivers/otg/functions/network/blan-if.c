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
 * otg/functions/network/blan.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/blan-if.c|20070814215823|43564
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
 * @file otg/functions/network/blan-if.c
 * @brief This file implements the required descriptors to implement
 * a BLAN network device with a single interface.
 *
 * The BLAN network driver implements the BLAN protocol descriptors.
 *
 * The BLAN protocol is designed to support smart devices that want
 * to create a virtual network between them host and other similiar
 * devices.
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

#define TRACE_VERBOSE 0
#define TRACE_VERY_VERBOSE 0
#undef DEBUG_CRC_SEEN

#if defined(CONFIG_OTG_NETWORK_BLAN) || defined(_OTG_DOXYGEN)

/* USB BLAN  Configuration ******************************************************************** */

/*
 * BLAN Ethernet Configuration
 */

/*! @name Descriptors - Communication Interface Class descriptors
 *
 * @{
 */
static struct usbd_class_header_function_descriptor blan_class_1 = {
        .bFunctionLength = 0x05, /* Length */
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype = USB_ST_HEADER,
        .bcdCDC =  __constant_cpu_to_le16(0x0110) /*Version */
};
static struct usbd_class_mdlm_descriptor blan_class_2 = {
        .bFunctionLength = 0x15,
        .bDescriptorType = USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype = USB_ST_MDLM,
        .bcdVersion = __constant_cpu_to_le16(0x0100),
        .bGUID =  {
            0x74, 0xf0, 0x3d, 0xbd, 0x1e, 0xc1, 0x44, 0x70,  /* bGUID */
            0xa3, 0x67, 0x71, 0x34, 0xc9, 0xf5, 0x54, 0x37,  /* bGUID */ },
};
static struct usbd_class_blan_descriptor blan_class_3 = {
        .bFunctionLength = 0x07,
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype =  USB_ST_MDLMD,
        .bGuidDescriptorType =  0x01,
        .bmNetworkCapabilities =  0x00,
        .bmDataCapabilities =  0x00,
        .bPad =  0x00,
};
static struct usbd_class_ethernet_networking_descriptor blan_class_4 = {
        .bFunctionLength = 0x0d,
        .bDescriptorType = USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype = USB_ST_ENF,
        .iMACAddress =  0x00,
        .bmEthernetStatistics =  0x00,
        .wMaxSegmentSize =  0x05ea, /* 1514 maximum frame size */
        .wNumberMCFilters =   0x00,
        .bNumberPowerFilters =  0x00 ,
};
static struct usbd_class_network_channel_descriptor blan_class_5 = {
        .bFunctionLength =  0x07,
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype = USB_ST_NCT,
        .bEntityId =  0,
        .iName =  0,
        .bChannelIndex =  0,
        .bPhysicalInterface =  0,
};


static struct usbd_generic_class_descriptor *blan_comm_class_descriptors[] = {
        (struct usbd_generic_class_descriptor *) &blan_class_1,
        (struct usbd_generic_class_descriptor *) &blan_class_2,
        (struct usbd_generic_class_descriptor *) &blan_class_3,
        (struct usbd_generic_class_descriptor *) &blan_class_4,
        (struct usbd_generic_class_descriptor *) &blan_class_5, };

/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description blan_alternate_descriptions[] = {
        {
                .iInterface =  "Belcarra USBLAN - MDLM/BLAN",
                .bInterfaceClass = COMMUNICATIONS_INTERFACE_CLASS,
                .bInterfaceSubClass =  COMMUNICATIONS_MDLM_SUBCLASS,
                .bInterfaceProtocol = COMMUNICATIONS_NO_PROTOCOL,
                .classes = sizeof (blan_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  blan_comm_class_descriptors,
                .endpoints = ENDPOINTS,
                .endpoint_index =  net_fd_endpoint_index,
        },
};

/*! @} */
/* Interface descriptions and descriptors
 */
/*! Interface Description List
 */
static struct usbd_interface_description blan_interfaces[] = {
        {
                .alternates =  sizeof (blan_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list =  blan_alternate_descriptions,
        },
};

/* ********************************************************************************************* */

/*! net_fd_start_xmit_mdlm
 * @brief - start sending a buffer
 *
 * @param function_instance - function instance pointer
 * @param buffer
 * @param len
 * @param data
 *
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit_mdlm (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        u32 crc;

        #ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
        int xmit_index = 0;
        int endpoint_index = BULK_IN_A;
        #else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
        int xmit_index = (otg_atomic_read(&npd->xmit_urbs_started[0]) <= otg_atomic_read(&npd->xmit_urbs_started[1]))
                ? 0 : 1;
        int endpoint_index = (xmit_index) ? BULK_IN_B : BULK_IN_A;
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */

        int in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, xmit_index, usbd_high_speed(function_instance));

        if (TRACE_VERBOSE)
                TRACE_MSG8(NTT,"npd: %p flags: %04x len: %d endpoint_index: %d "
                                "xmit_index: %d xmit_started: %d %d in_pkt_sz: %d",
                                npd, npd->flags, len, endpoint_index, xmit_index, otg_atomic_read(&npd->xmit_urbs_started[0]),
                                otg_atomic_read(&npd->xmit_urbs_started[1]), in_pkt_sz);

        #if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        /* allocate urb 5 bytes larger than required */
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, len + 5 + 4 + in_pkt_sz, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                return -ENOMEM;
        }

        urb->actual_length = len;

        /* copy and crc len bytes */
        crc = crc32_copy(urb->buffer, buffer, len, CRC32_INIT);

        if ((urb->actual_length % in_pkt_sz) == (in_pkt_sz - 4)) {
                /* no longer in Kconfig - change undef to define to active padbyte */
                #undef CONFIG_OTG_NETWORK_PADBYTE
                #ifdef CONFIG_OTG_NETWORK_PADBYTE
                // add a pad byte if required to ensure a short packet, usbdnet driver
                // will correctly handle pad byte before or after CRC, but the MCCI driver
                // wants it before the CRC.
                crc = crc32_pad(urb->buffer + urb->actual_length, 1, crc);
                urb->actual_length++;
                #else /* CONFIG_OTG_NETWORK_PADBYTE */
                urb->flags |= USBD_URB_SENDZLP;
                TRACE_MSG2(NTT,"setting ZLP: urb: %p flags: %x", urb, urb->flags);
                #endif /* CONFIG_OTG_NETWORK_PADBYTE */
        }
        crc = ~crc;
        urb->buffer[urb->actual_length++] = crc & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 8) & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 16) & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 24) & 0xff;

        #if defined(CONFIG_OTG_NETWORK_BLAN_FERMAT)
        if (npd->fermat) fermat_encode(urb->buffer, urb->actual_length);
        #endif

        #else /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/

        /* allocate urb with no buffer */
        #ifdef CONFIG_OTG_NETWORK_XMIT_OS
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, 0, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                return -ENOMEM;
        }
        urb->actual_length = len;
        urb->buffer = buffer;
        #else /* CONFIG_OTG_NETWORK_XMIT_OS */
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, len + 5 + 4 + in_pkt_sz, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }
        urb->actual_length = len;
        memcpy (urb->buffer, buffer, len);
        #endif /* CONFIG_OTG_NETWORK_XMIT_OS */

        urb->flags |= ((urb->actual_length % in_pkt_sz) == 0) ? USBD_URB_SENDZLP : 0;

        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/

        if (TRACE_VERBOSE)
                TRACE_MSG3(NTT,"urb: %p buf: %p priv: %p", urb, data, urb->function_privdata);
        urb->function_privdata = data;

        otg_atomic_add(urb->actual_length, &npd->queued_bytes);
        otg_atomic_inc(&npd->xmit_urbs_started[xmit_index]);
        if ((rc = usbd_start_in_urb (urb))) {

                TRACE_MSG1(NTT,"FAILED: %d", rc);
                printk(KERN_ERR"%s: FAILED: %d\n", __FUNCTION__, rc);
                urb->function_privdata = NULL;
                otg_atomic_sub(urb->actual_length, &npd->queued_bytes);
                otg_atomic_dec(&npd->xmit_urbs_started[xmit_index]);
                usbd_free_urb (urb);

                return rc;
        }
        return 0;
}

/* ********************************************************************************************* */

/*! net_fd_recv_urb_mdlm
 * @brief callback to process a received URB
 *
 * @param urb - pointer to received urb
 * @param rc - dummy parameter
 * @return non-zero for failure.
 */
int net_fd_recv_urb_mdlm(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        void *os_data = NULL;
        void *os_buffer = NULL;
        int crc_bad = 0;
        int trim = 0;
        int len;
        u32 crc;
        u32 temmp;
        len = urb->actual_length;
        trim = 0;

        //TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);
        //RETURN_EINVAL_IF (urb->status == USBD_URB_OK);

        os_data = urb->function_privdata;

        //TRACE_MSG2(NTT, "os_data: %x os_buffer: %x", os_data, os_buffer);


        #if defined(CONFIG_OTG_NETWORK_BLAN_PADAFTER)
        {
                /* This version simply checks for a correct CRC along the
                 * entire packet. Some UDC's have trouble with some packet
                 * sizes, this allows us to add pad bytes after the CRC.
                 */

                u8 *src = urb->buffer;
                int copied;

                // XXX this should work, but the MIPS optimizer seems to get it wrong....
                //copied = (len < urb->wMaxPacketSize) ? 0 : ((len / urb->wMaxPacketSize) - 1) * urb->wMaxPacketSize;

                if (len < urb->wMaxPacketSize*2)
                        copied = 0;
                else {
                        int pkts = ((len - urb->wMaxPacketSize) / urb->wMaxPacketSize);
                        copied = (pkts - 1) * urb->wMaxPacketSize;
                }

                len -= copied;
                crc = CRC32_INIT;
                for (; copied-- > 0 ; crc = COMPUTE_FCS (crc, *os_buffer++ = *src++));

                for (; (len-- > 0) && (CRC32_GOOD != crc); crc = COMPUTE_FCS (crc, *os_buffer++ = *src++));

                trim = len + 4;

                if (CRC32_GOOD != crc) {
                        TRACE_MSG1(NTT,"AAA frame: %03x", urb->framenum);
                        THROW_IF(npd->seen_crc, crc_error);
                }
                else
                        npd->seen_crc = 1;
        }
        //#else /* defined(CONFIG_OTG_NETWORK_BLAN_PADAFTER) */
        #elif defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        /*
         * The CRC can be sent in two ways when the size of the transfer
         * ends up being a multiple of the packetsize:
         *
         *                                           |
         *                <data> <CRC><CRC><CRC><CRC>|<???>     case 1
         *                <data> <NUL><CRC><CRC><CRC>|<CRC>     case 2
         *           <data> <NUL><CRC><CRC><CRC><CRC>|          case 3
         *     <data> <NUL><CRC><CRC><CRC>|<CRC>     |          case 4
         *                                           |
         *
         * This complicates CRC checking, there are four scenarios:
         *
         *      1. length is 1 more than multiple of packetsize with a trailing byte
         *      2. length is 1 more than multiple of packetsize
         *      3. length is multiple of packetsize
         *      4. none of the above
         *
         * Finally, even though we always compute CRC, we do not actually throw
         * things away until and unless we have previously seen a good CRC.
         * This allows backwards compatibility with hosts that do not support
         * adding a CRC to the frame.
         *
         */

        // test if 1 more than packetsize multiple
        if (1 == (len % urb->wMaxPacketSize)) {
                u8 *cp = urb->buffer + len - 1 - 4;
                // copy and CRC up to the packetsize boundary
                crc = crc32_nocopy(urb->buffer, len - 1, CRC32_INIT);

                if (TRACE_VERBOSE)
                        TRACE_MSG7(NTT,"A CRC nocopy: %08x %08x len: %d CRC: %02x %02x %02x %02x",
                                        CRC32_GOOD, crc, len, cp[0], cp[1], cp[2], cp[3]);

                // if the CRC is good then this is case 1
                if (CRC32_GOOD != crc) {

                        crc = crc32_nocopy(urb->buffer + len - 1, 1, crc);

                        if (CRC32_GOOD != crc) {
                                //crc_errors[len%64]++;
                                TRACE_MSG3(NTT,"A CRC error %08x %08x %03x", CRC32_GOOD, crc, urb->framenum);
                                printk(KERN_INFO"%s: A CRC\n", __FUNCTION__);
                                npd->seen_crc_error = 1;
                                THROW_IF(npd->seen_crc, crc_error);
                        }
                        else
                                npd->seen_crc = 1;
                }
                else
                        npd->seen_crc = 1;

        }
        else {
                u8 *cp = urb->buffer + len - 4;
                crc = crc32_nocopy(urb->buffer, len, CRC32_INIT);
                if (TRACE_VERBOSE)
                        TRACE_MSG7(NTT,"B CRC nocopy: %08x %08x len: %d CRC: %02x %02x %02x %02x",
                                        CRC32_GOOD, crc, len, cp[0], cp[1], cp[2], cp[3]);

                if (CRC32_GOOD != crc) {
                        //crc_errors[len%64]++;
                        TRACE_MSG3(NTT,"B CRC error %08x %08x %03x", CRC32_GOOD, crc, urb->framenum);
                        if (TRACE_VERBOSE) {

                                TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);

                                TRACE_NRECV(NTT, 32, urb->buffer);
                                TRACE_MSG0(NTT, "--");
                                TRACE_RECV(NTT, urb->actual_length, urb->buffer);
                        }
                        printk(KERN_INFO"%s: B CRC\n", __FUNCTION__);
                        npd->seen_crc_error = 1;
                        THROW_IF(npd->seen_crc, crc_error);
                }
                else
                        npd->seen_crc = 1;

                // XXX shorten by 4 bytes?

        }
        // trim IFF we are paying attention to crc
        if (npd->seen_crc)
                trim = 4;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/

        if (net_fd_recv_buffer(function_instance, urb->buffer, len, os_data, crc_bad, trim)) {
                TRACE_MSG0(NTT, "FAILED");
                net_os_dealloc_buffer(function_instance, os_data, os_buffer);
        }

        // catch a simple error, just increment missed error and general error
        CATCH(error) {
                //TRACE_MSG4(NTT,"CATCH(error) urb: %p status: %d len: %d function: %p",
                //                urb, urb->status, urb->actual_length, function_instance);
                // catch a CRC error
                CATCH(crc_error) {
                        crc_bad = 1;
                        npd->seen_crc_error = 1;
                }
        }
        return 0;
}



/* ********************************************************************************************* */
int blan_start_recv(struct usbd_function_instance *function_instance);

/*! net_fd_recv_urb2 - callback to process a received URB
 *
 * @param urb - pointer to copy of received urb,
 * @param rc - receiving urb result code
 *
 * @return non-zero for failure.
 */
int net_fd_recv_urb2(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        int hs = usbd_high_speed(function_instance);
        int endpoint_index = urb->endpoint_index;

        #ifndef CONFIG_OTG_NETWORK_DOUBLE_OUT
        int recv_index = 0;
        #else /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        int recv_index = (endpoint_index == BULK_OUT_A) ? 0 : 1;
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        int alloc_length = usbd_endpoint_transferSize(function_instance, endpoint_index, hs);
        int status = urb->status;

        void *os_data;
        u8 *os_buffer;


        if (TRACE_VERY_VERBOSE) {

                TRACE_MSG4(NTT, "status: %d actual_length: %d bus status: %d device_state: %d",
                                urb->status, urb->actual_length,
                                usbd_get_device_status(function_instance), usbd_get_device_state(function_instance)
                                );

                TRACE_NRECV(NTT, 32, urb->buffer);
                TRACE_MSG0(NTT, "--");
                TRACE_RECV(NTT, urb->actual_length, urb->buffer);
        }


        otg_atomic_dec(&npd->recv_urbs_started[recv_index]);

        /* process the data */
        if (urb->status == USBD_URB_OK)
                npd->net_recv_urb(urb, rc);

        if (urb->status == USBD_URB_CANCELLED)
                net_os_dealloc_buffer(function_instance, urb->function_privdata, urb->buffer);

        /* disconnect os_data buffer from urb */
        urb->function_privdata = NULL;
        urb->buffer = NULL;
        urb->function_instance = NULL;
        urb->status = USBD_URB_OK;
        usbd_free_urb(urb);

        if ((USBD_OK == usbd_get_device_status(function_instance)) &&
                        (STATE_CONFIGURED == usbd_get_device_state(function_instance))) {

                blan_start_recv(function_instance);
        }
        else {
                TRACE_MSG0(NTT, "NOT RESTARTING");
        }

        return 0;
}

int blan_start_recv_endpoint(struct usbd_function_instance *function_instance,
                int endpoint_index, otg_atomic_t *recv_urbs_started, char *msg)
{
        struct usb_network_private *npd = function_instance->privdata;
        int i;
        int hs = usbd_high_speed(function_instance);
        int alloc_length = usbd_endpoint_transferSize(function_instance, endpoint_index, hs);

        if (TRACE_VERBOSE)
                TRACE_MSG4(NTT, "endpoint_index: %d recv_urbs_started: %d alloc_length: %d %s",
                                endpoint_index, otg_atomic_read(recv_urbs_started), alloc_length, msg);

        while (otg_atomic_read(recv_urbs_started) < npd->max_recv_urbs ) {
                u8 *os_buffer = NULL;
                void *os_data = NULL;
                struct usbd_urb *urb;

                #ifdef DEBUG_CRC_SEEN
                if (npd->seen_crc_error) {
                        TRACE_MSG0(NTT, "CRC ERROR NOT RESTARTING");
                        break;
                }
                #endif /* DEBUG_CRC_SEEN */

                /* get os buffer - os_buffer is data, os_data is the os data structure */
                os_data = net_os_alloc_buffer(function_instance, &os_buffer, alloc_length);

                /* allocate urb with no buffer */
                /*allocate urb without buffer */
                urb = usbd_alloc_urb(function_instance, endpoint_index, 0, net_fd_recv_urb2);

                /* start urb with buffer pointing at the os_buffer in os_data structure */
                if (os_buffer && urb) {

                        urb->function_privdata = os_data;
                        urb->buffer = os_buffer;
                        urb->flags |= npd->recv_urb_flags;
                        urb->alloc_length = urb->buffer_length = alloc_length;
                        if (usbd_start_out_urb(urb)) {
                                net_os_dealloc_buffer(function_instance, os_data, os_buffer);
                                urb->function_privdata = NULL;
                                urb->buffer = NULL;
                                usbd_free_urb(urb);
                        }
                        otg_atomic_inc(recv_urbs_started);
                        continue;
                }
                TRACE_MSG1(NTT, "recv_urbs_started: %d FAILED EARLY", otg_atomic_read(recv_urbs_started));

                if (os_buffer || os_data)
                        net_os_dealloc_buffer(function_instance, os_data, os_buffer);
                if (urb)
                        urb->buffer = NULL;
                        usbd_free_urb(urb);
                break;
        }
        if (TRACE_VERBOSE)
                TRACE_MSG1(NTT, "recv_urbs_started: %d", otg_atomic_read(recv_urbs_started));
        return 0;
}

#if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
/*! blan_start_recv - start recv urb(s)
 *
 *@param function_instance - pointer to this function instance
 *@return none
 */
int blan_start_recv(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        TRACE_MSG0(NTT, "DIRECT");
        blan_start_recv_endpoint(function_instance, BULK_OUT_A, &npd->recv_urbs_started[0], "BULK_OUT_A");
        #ifdef CONFIG_OTG_NETWORK_DOUBLE_OUT
        blan_start_recv_endpoint(function_instance, BULK_OUT_B, &npd->recv_urbs_started[1], "BULK_OUT_B");
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        return 0;
}
#else /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */

/*! blan_start_recv_task - start recv urb(s)
 *
 * For high speed devices we can optimize using FAST_RECV flag to get urb handed
 * directly to recv urb function from ISR. But then we do not want to allocate new
 * recv urbs, so must implement a task for that.
 *
 *@param data - pointer to this function instance
 *@return none
 */
void * blan_start_recv_task(otg_task_arg_t data)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *)data;
        struct usb_network_private *npd = function_instance->privdata;
        TRACE_MSG0(NTT, "--");
        blan_start_recv_endpoint(function_instance, BULK_OUT_A, &npd->recv_urbs_started[0], "BULK_OUT_A");
        #ifdef CONFIG_OTG_NETWORK_DOUBLE_OUT
        blan_start_recv_endpoint(function_instance, BULK_OUT_B, &npd->recv_urbs_started[1], "BULK_OUT_B");
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        return NULL;
}
/*! blan_start_recv - start recv urb(s)
 *
 *@param function_instance - pointer to this function instance
 *@return none
 */
int blan_start_recv(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        TRACE_MSG0(NTT, "SCHEDULE");
        otg_up_work(npd->blan_recv_task);
        return 0;
}
#endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */

/*! net_fd_start_recv_mdlm - start recv urb(s)
 *
 * For high speed devices we can optimize using FAST_RECV flag to get urb handed
 * directly to recv urb function from ISR. But then we do not want to allocate new
 * recv urbs, so must implement a task for that.
 *
 *@param data - pointer to this function instance
 *@return none
 */
int net_fd_start_recv_mdlm (struct usbd_function_instance *function_instance)
{
        blan_start_recv(function_instance);
        return 0;
}

/* ********************************************************************************************* */


/*! net_fd_function_enable
 * @brief  enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 *
 * @param function_instance - pointer ot instance of this function
 * @return int indicating function driver enable operation result
 */

int blan_fd_function_enable (struct usbd_function_instance *function_instance)
{
        struct usbd_class_network_channel_descriptor *channel = &blan_class_5 ;
        struct usb_network_private *npd = NULL;
        u32 recv_urb_flags;
#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
        struct usbd_class_ethernet_networking_descriptor *ethernet = &blan_class_4;
        char address_str[14];
        snprintf(address_str, 13, "%02x%02x%02x%02x%02x%02x",
                        local_dev_addr[0], local_dev_addr[1], local_dev_addr[2],
                        local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#else
        char address_str[20];
        sprintf(address_str, "%02x%02x%02x%02x%02x%02x",
                        local_dev_addr[0], local_dev_addr[1], local_dev_addr[2],
                        local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#endif
        ethernet->iMACAddress = usbd_alloc_string(address_str);
#endif
        struct usbd_class_ethernet_networking_descriptor *ethernet = &blan_class_4;
        char address_str[20];

        sprintf(address_str, "%02x%02x%02x%02x%02x%02x", 0, 0, 0, 0, 0, 0);
        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);

        //channel->iName = usbd_alloc_string(function_instance, system_utsname.nodename);

        //TRACE_MSG3(NTT, "name: %s strings index imac: %d name: %d",
        //                system_utsname.nodename, ethernet->iMACAddress, channel->iName);

        #if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        recv_urb_flags = 0;
        #else /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */
        recv_urb_flags = USBD_URB_FAST_RETURN | USBD_URB_FAST_FINISH;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC) */


        THROW_IF(net_fd_function_enable(function_instance,
                        network_blan, net_fd_recv_urb_mdlm,
                        net_fd_start_xmit_mdlm,
                        blan_start_recv, recv_urb_flags
                        ), error);

        THROW_UNLESS((npd = function_instance->privdata), error);

        #if !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        THROW_UNLESS((npd->blan_recv_task = otg_task_init2("blanrcv", blan_start_recv_task, function_instance, NTT)), error);
        //npd->blan_recv_task->taskdebug = TRUE;
        otg_task_start(npd->blan_recv_task);
        #endif /* !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC) */

        return 0;

        CATCH(error) {
                return -EINVAL;
        }
}

/*! blan_fd_function_disable - disable the function driver
 *
 * @param function_instance - pointer to function instance
 *
 * @return none
 *
 */
void blan_fd_function_disable (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;

        #if !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        if (npd->blan_recv_task) {
                otg_task_exit(npd->blan_recv_task);
                npd->blan_recv_task = NULL;
        }
        #endif /* !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC) */
        net_fd_function_disable(function_instance);
}


/*!
 * blan_fd_set_configuration - called to indicate set configuration request was received
 * @param function_instance
 * @param configuration
 * @return int
 */
int blan_fd_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usb_network_private *npd = function_instance->privdata;
        int hs = usbd_high_speed(function_instance);

        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;

        TRACE_MSG2(NTT, "CONFIGURED: %d ip_addr: %08x", configuration, npd->ip_addr);

        //net_check_mesg(mesg_configured);
        if (npd->eem_os_buffer)
                net_os_dealloc_buffer(function_instance, npd->eem_os_data, npd->eem_os_buffer);

        npd->max_recv_urbs = hs ? NETWORK_START_URBS * 3 : NETWORK_START_URBS;
        otg_atomic_clr(&npd->recv_urbs_started[0]);
        #ifdef CONFIG_OTG_NETWORK_DOUBLE_OUT
        otg_atomic_clr(&npd->recv_urbs_started[1]);
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        npd->eem_os_data = npd->eem_os_buffer = NULL;
        //npd->flags |= NETWORK_CONFIGURED;
	npd->altsetting = 0;

        if ((npd->flags & NETWORK_OPEN))
                net_os_send_notification_later(function_instance);

        TRACE_MSG1(NTT, "START RECV npd->flags: %04x", npd->flags);

        net_fd_start(function_instance);

        TRACE_MSG1(NTT, "CONFIGURED npd->flags: %04x", npd->flags);
        return 0;
}

/*!
 * @brief  blan_fd_reset - called to indicate bus has been reset
 * @param function_instance
 * @return int
 */
int blan_fd_reset (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        TRACE_MSG1(NTT,"RESET %08x",npd->ip_addr);
        return net_fd_stop (function_instance);
}


/*! blan_fd_urb_received_ep0 - callback for sent URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @return non-zero for failure.
 */
int blan_fd_urb_received_ep0 (struct usbd_urb *urb, int urb_rc)
{
        TRACE_MSG2(NTT,"urb: %p status: %d", urb, urb->status);

        RETURN_EINVAL_IF (USBD_URB_OK != urb->status);

        // TRACE_MSG1(NTT,"%s", urb->buffer); // QQSV  is this really a NUL-terminated string???

        return -EINVAL;         // caller will de-allocate
}
/*! blan_fd_device_request
 *  @brief process a received SETUP URB
 *
 * Processes a received setup packet and CONTROL WRITE data.
 * Results for a CONTROL READ are placed in urb->buffer.
 *
 * @param function_instance - pointer to this function instance
 * @param request - received request to interface or endpoint
 * @return non-zero for failure.
 */
int blan_fd_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb;
        int index;

        /* Verify that this is a USB Class request per CDC specification or a vendor request.
         */
        RETURN_ZERO_IF (!(request->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_VENDOR)));

        /* Determine the request direction and process accordingly
         */
        switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {

        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:

                switch (request->bRequest) {
                case MCCI_ENABLE_CRC:
                        //if (make_crc_table())
                        //        return -EINVAL;
                        //npd->encapsulation = simple_crc;
                        return 0;

                case BELCARRA_PING:
                        TRACE_MSG1(NTT,"H2D VENDOR IP: %08x", npd->ip_addr);
                        if ((npd->network_type == network_blan))
                                net_os_send_notification_later(function_instance);
                        break;

                #if !defined(CONFIG_OTG_NETWORK_BLAN_DO_NOT_SETTIME) || !defined(CONFIG_OTG_NETWORK_SAFE_DO_NOT_SETTIME)
                case BELCARRA_SETTIME:
                        npd->rfc868time = ntohl( request->wValue << 16 | request->wIndex);
                        net_os_settime(function_instance, npd->rfc868time);
                        break;
                #endif
                case BELCARRA_SETIP:
                        #ifdef OTG_BIG_ENDIAN
                        npd->ip_addr = ntohl( request->wValue | request->wIndex<< 16 );
                        #else /* OTG_BIG_ENDIAN */
                        npd->ip_addr = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "npd->ip_addr: %08x", npd->ip_addr);
                        // XXX need to get in correct order here
                        // XXX what about locally set mac addr
                        // XXX UNLESS(npd->local_dev_set) {
                        if (!(npd->override_MAC)) {
                                npd->local_dev_addr[0] = 0xfe | 0x02;           /* locally administered */
                                npd->local_dev_addr[1] = 0x00;
                                npd->local_dev_addr[2] = (npd->ip_addr >> 24) & 0xff;
                                npd->local_dev_addr[3] = (npd->ip_addr >> 16) & 0xff;
                                npd->local_dev_addr[4] = (npd->ip_addr >> 8) & 0xff;
                                npd->local_dev_addr[5] = (npd->ip_addr >> 0) & 0xff;
                         }
                        // XXX }
                        break;

                case BELCARRA_SETMSK:
                        #ifdef OTG_BIG_ENDIAN
                        npd->network_mask = ntohl( request->wValue | request->wIndex << 16);
                        #else /* OTG_BIG_ENDIAN */
                        npd->network_mask = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "npd->network_mask: %08x", npd->network_mask);
                        break;

                case BELCARRA_SETROUTER:
                        #ifdef OTG_BIG_ENDIAN
                        npd->router_ip = ntohl( request->wValue | request->wIndex << 16);
                        #else /* OTG_BIG_ENDIAN */
                        npd->router_ip = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "npd->router_ip: %08x", npd->router_ip);
                        break;

                case BELCARRA_SETDNS:
                        #ifdef OTG_BIG_ENDIAN
                        npd->dns_server_ip = ntohl( request->wValue | request->wIndex < 16);
                        #else /* OTG_BIG_ENDIAN */
                        npd->dns_server_ip = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        break;
                #ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
                case BELCARRA_SETFERMAT:
                        npd->fermat = 1;
                        break;
                #endif
                #ifdef CONFIG_OTG_NETWORK_BLAN_HOSTNAME
                case BELCARRA_HOSTNAME:
                        TRACE_MSG0(NTT,"HOSTNAME");
                        RETURN_EINVAL_IF(!(urb = usbd_alloc_urb_ep0(function_instance, le16_to_cpu(request->wLength),
                                                       blant_fd_urb_received_ep0) ));

                        RETURN_ZERO_IF(!usbd_start_out_urb(urb));                  // return if no error
                        usbd_free_urb(urb);                                  // de-alloc if error
                        return -EINVAL;
                #endif
                #ifdef CONFIG_OTG_NETWORK_BLAN_DATA_NOTIFY_OK
                case BELCARRA_DATA_NOTIFY:
                        TRACE_MSG0(NTT,"DATA NOTIFY");
                        npd->data_notify = 1;
                        return -EINVAL;
                #endif
                }
                switch (request->bRequest) {
                case BELCARRA_SETIP:
                case BELCARRA_SETMSK:
                case BELCARRA_SETROUTER:
                        TRACE_MSG5(NTT, "npd->network_mask: %08x npd->ip_addr: %08x npd->router_ip: %08x flags: %08x %s",
                                        npd->network_mask, npd->ip_addr, npd->router_ip, npd->flags,
                                        (npd->flags & NETWORK_CONFIGURED) ? "CONFIGURED" : "");

                        BREAK_UNLESS (npd->network_mask && npd->ip_addr && npd->router_ip && (npd->flags & NETWORK_CONFIGURED));
                        // let the os layer know, if it's interested.
                        #ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
                        net_os_config(function_instance);
                        net_os_hotplug(function_instance);
                        #endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
                        break;
                }
                return 0;
        default:
                break;
        }
        return -EINVAL;
}

/* ********************************************************************************************* */
/*! blan_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations blan_fd_function_ops = {
        .function_enable = blan_fd_function_enable,
        .function_disable = blan_fd_function_disable,

        .device_request = blan_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .set_configuration = blan_fd_set_configuration,
        .set_interface = NULL,
        .reset = blan_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};

/*! function driver description
 */
struct usbd_interface_driver blan_interface_driver = {
        .driver = {
                .name =  "net-blan-if",
                .fops =  &blan_fd_function_ops, },
        .interfaces = sizeof (blan_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = blan_interfaces,
        .endpointsRequested =  ENDPOINTS,
        .requestedEndpoints =  net_fd_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        #if !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
        .iFunction = "Belcarra USBLAN - MDLM/BLAN",
        #elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
        .iFunction = "Belcarra USBLAN - MDLM/BLAN (DIN",
        #elif !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
        .iFunction = "Belcarra USBLAN - MDLM/BLAN (DOUT)",
        #elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
        .iFunction = "Belcarra USBLAN - MDLM/BLAN (DIN/DOUT)",
        #endif /* defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT) */
};

/* ********************************************************************************************* */

/*! int blan_mod_init()
 * @brief - set interface bmDataCapabilities, and register driver to initialize module
 * @return int for driver register result
 */
int blan_mod_init (void)
{
        struct usbd_class_ethernet_networking_descriptor *ethernet;
        struct usbd_class_network_channel_descriptor *channel;

        int len = 0;
        char buf[255];

        buf[0] = 0;

#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
        TRACE_MSG0(NTT,"fermat");
        fermat_init();
#endif

        //blan_alternate_descriptions[0].endpoints = Usb_network_private.have_interrupt ? 3 : 2;

#if 0
        // Update the iMACAddress field in the ethernet descriptor
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
                char address_str[14];
                snprintf(address_str, 13, "%02x%02x%02x%02x%02x%02x",
                                local_dev_addr[0], local_dev_addr[1], local_dev_addr[2],
                                local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#else
                char address_str[20];
                sprintf(address_str, "%02x%02x%02x%02x%02x%02x",
                                local_dev_addr[0], local_dev_addr[1], local_dev_addr[2],
                                local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#endif
                TRACE_MSG0(NTT,"alloc mac string");
                //if ((ethernet = &blan_class_4))
                //        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);
                TRACE_MSG0(NTT,"alloc mac string done");
        }
        TRACE_MSG0(NTT,"alloc channel string");
        //if ((channel = &blan_class_5))
        //        channel->iName = usbd_alloc_string(function_instance, system_utsname.nodename);
        TRACE_MSG0(NTT,"alloc channel string done");
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADBYTES
        blan_class_3.bPad = CONFIG_OTG_NETWORK_BLAN_PADBYTES;
        len += sprintf(buf + len, "PADBYTES: %02x ", blan_class_3.bPad);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADBEFORE
        blan_class_3.bmDataCapabilities |= BMDATA_PADBEFORE;
        len += sprintf(buf + len, "PADBEFORE: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADAFTER
        blan_class_3.bmDataCapabilities |= BMDATA_PADAFTER;
        len += sprintf(buf + len, "PADAFTER: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_CRC
        blan_class_3.bmDataCapabilities |= BMDATA_CRC;
        len += sprintf(buf + len, "CRC: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
        blan_class_3.bmDataCapabilities |= BMDATA_FERMAT;
        len += sprintf(buf + len, "FERMAT: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_HOSTNAME
        blan_class_3.bmDataCapabilities |= BMDATA_HOSTNAME;
        len += sprintf(buf + len, "HOSTNAME: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_NONBRIDGED
        #ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
        #warning BLAN_AUTO_CONFIG ignored when BLAN_NONBRIDGED enabled
        #endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
        blan_class_3.bmNetworkCapabilities |= BMNETWORK_NONBRIDGED | BMNETWORK_NO_VENDOR;
        len += sprintf(buf + len, "NONBRIDGE: %02x ", blan_class_3.bmNetworkCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_DATA_NOTIFY_OK
        blan_class_3.bmNetworkCapabilities |= BMNETWORK_DATA_NOTIFY_OK;
        len += sprintf(buf + len, "DATA NOTIFY: %02x ", blan_class_3.bmNetworkCapabilities);
#endif
#ifndef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
        blan_class_3.bmNetworkCapabilities |= BMNETWORK_NO_VENDOR;
        len += sprintf(buf + len, "NO VENDOR: %02x ", blan_class_3.bmNetworkCapabilities);
#endif
        if (strlen(buf))
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, buf);

        return usbd_register_interface_function (&blan_interface_driver, "net-blan-if", NULL);
}

/*!
 * @brief blan_mod_exit
 * @return none
 */
void blan_mod_exit(void)
{
        usbd_deregister_interface_function (&blan_interface_driver);
}

#else                  /* CONFIG_OTG_NETWORK_BLAN */
/*!
 * @brief  blan_mod_init
 * @return int
 */
int blan_mod_init (void)
{
        return 0;
}
/*!
 * @brief blan_mod_exit
 * @return none
 */
void blan_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_BLAN */
