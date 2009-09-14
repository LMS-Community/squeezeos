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
 * otg/functions/network/ecm.c - Network Function Driver
 * @(#) balden@belcarra.com|otg/functions/network/ecm-if.c|20070326205353|49276
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
 * @file otg/functions/network/ecm-if.c
 * @brief This file implements the required descriptors to implement
 * a basic network device with two interfaces.
 *
 * The CDC ECM network driver implements the CDC Network Configuration.  Two
 * interfaces with two BULK data endpoints and an INTERRUPT endpoint.
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

#if defined(CONFIG_OTG_NETWORK_ECM) || defined(_OTG_DOXYGEN)

/* USB CDC Configuration ******************************************************************** */

/*! @name CDC ECM descriptors group
 *
 * @{
 */
static struct usbd_class_header_function_descriptor ecm_class_1 = {
    bFunctionLength:    0x05,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_HEADER,
    bcdCDC:             __constant_cpu_to_le16(0x0110),
};


static struct usbd_class_ethernet_networking_descriptor ecm_class_2 = {
    bFunctionLength:    0x0D,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_ENF,
    iMACAddress:        0x00,
    bmEthernetStatistics: 0x00,
    wMaxSegmentSize:    __constant_cpu_to_le16(0x05ea),
    wNumberMCFilters:   0,
    bNumberPowerFilters: 0,
};

static struct usbd_class_union_function_descriptor ecm_class_3 =
{
     bFunctionLength:    0x05,
     bDescriptorType:    USB_DT_CLASS_SPECIFIC,
     bDescriptorSubtype: USB_ST_UF,
     bMasterInterface:   0x00,
     bSlaveInterface:    { 1 },                         // This will be updated

};

static usbd_class_descriptor_t *ecm_comm_class_descriptors[] = {
        (struct usbd_generic_class_descriptor *) &ecm_class_1,
        (struct usbd_generic_class_descriptor *) &ecm_class_2,
        (struct usbd_generic_class_descriptor *) &ecm_class_3,
};

/*!
 * @}
 */

/*!@name  Data Alternate Interface Description List
 *
 * @{
 */
static struct usbd_alternate_description ecm_comm_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra USBLAN - CDC/ECM",
                .bInterfaceClass = USB_CLASS_COMM,
                .bInterfaceSubClass =  COMMUNICATIONS_ENCM_SUBCLASS,
                .bInterfaceProtocol = 0,
                .classes = sizeof (ecm_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  ecm_comm_class_descriptors,
                .endpoints =  1,
                .endpoint_index = cdc_int_endpoint_index,
        },
};

static struct usbd_alternate_description ecm_data_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra USBLAN - CDC/ECM No Data",
                .bInterfaceClass = USB_CLASS_DATA,      // XXX
                .bInterfaceSubClass =  0,               // XXX
                .bInterfaceProtocol = 0,                // XXX
        },
        {
                .iInterface = "Belcarra USBLAN - CDC/ECM Data",
                .bInterfaceClass = USB_CLASS_DATA,      // XXX
                .bInterfaceSubClass =  0x0,             // XXX
                .bInterfaceProtocol = 0x0,              // XXX
                .endpoints =  2,
                .endpoint_index = cdc_data_endpoint_index,
        },
};

/*!
 * @}
 */


/* CDC Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description ecm_interfaces[] = {
        {
                .alternates = sizeof (ecm_comm_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = ecm_comm_alternate_descriptions,
        },
        {
                .alternates = sizeof (ecm_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = ecm_data_alternate_descriptions,
        },
};

/* ********************************************************************************************* */
/*! net_fd_start_xmit_nocrc
 * @brief - start sending a buffer
 *
 * @param function_instance - pointer to this function instance
 * @param buffer buffer containing data to send
 * @param len  - length of data to send
 * @param data instance data
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit_nocrc (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        int in_pkt_sz;
        u8 *cp;
        u32 crc;
        #ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
        int xmit_index = 0;
        int endpoint_index = BULK_IN_A;
        #else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
        int xmit_index = (otg_atomic_read(&npd->xmit_urbs_started[0]) <= otg_atomic_read(&npd->xmit_urbs_started[1]))
                ? 0 : 1;
        int endpoint_index = (xmit_index) ? BULK_IN_B : BULK_IN_A;
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */

        TRACE_MSG7(NTT,"npd: %p function: %p flags: %04x len: %d endpoint_index: %d xmit_started: %d %d",
                        npd, function_instance, npd->flags, len, endpoint_index,
                        otg_atomic_read(&npd->xmit_urbs_started[0]), otg_atomic_read(&npd->xmit_urbs_started[1]));

        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));

        /* allocate urb 5 bytes larger than required */
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, len + 5 + in_pkt_sz, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }

        urb->actual_length = len;

        memcpy (urb->buffer, buffer, len);
        urb->function_privdata = data;
        urb->actual_length = len;
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

/*! net_fd_start_xmit_ecm
 * @brief - start sending to host
 * @param function_instance
 * @param buffer data storage area
 * @param len - length of data to transmit
 * @param data - instance private data
 *
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above)
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit_ecm (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        int in_pkt_sz;
        u32 crc;
        //#ifndef CONFIG_OTG_NETWORK_DOUBLE_IN
        int xmit_index = 0;
        int endpoint_index = BULK_IN_A;
        //#else /* CONFIG_OTG_NETWORK_DOUBLE_IN */
        //int xmit_index = (otg_atomic_read(&npd->xmit_urbs_started[0]) <= otg_atomic_read(&npd->xmit_urbs_started[1]))
        //        ? 0 : 1;
        //int endpoint_index = (xmit_index) ? BULK_IN_B : BULK_IN_A;
        //#endif /* CONFIG_OTG_NETWORK_DOUBLE_IN */

        TRACE_MSG7(NTT,"npd: %p function: %p flags: %04x len: %d endpoint_index: %d xmit_started: %d %d",
                        npd, function_instance, npd->flags, len, endpoint_index,
                        otg_atomic_read(&npd->xmit_urbs_started[0]), otg_atomic_read(&npd->xmit_urbs_started[1]));

        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));

        //TRACE_MSG0(NTT,"SIMPLE_CRC");
        // allocate urb 5 bytes larger than required
        if (!(urb = usbd_alloc_urb (function_instance, endpoint_index, len + 5 + 4 + in_pkt_sz, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, endpoint_index, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }
        urb->actual_length = len;

        #if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)

        // copy and crc len bytes
        crc = crc32_copy(urb->buffer, buffer, len, CRC32_INIT);

        if ((urb->actual_length % in_pkt_sz) == (in_pkt_sz - 4)) {

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

        memcpy (urb->buffer, buffer, len);
        if ((urb->actual_length % in_pkt_sz) == (in_pkt_sz)) {
                /* no longer in Kconfig - change undef to define to active padbyte */
                #undef CONFIG_OTG_NETWORK_PADBYTE
                #ifdef CONFIG_OTG_NETWORK_PADBYTE
                urb->actual_length++;
                #else /* CONFIG_OTG_NETWORK_PADBYTE */
                urb->flags |= USBD_URB_SENDZLP;
                TRACE_MSG2(NTT,"setting ZLP: urb: %p flags: %x", urb, urb->flags);
                #endif /* CONFIG_OTG_NETWORK_PADBYTE */
        }


        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/

        //TRACE_MSG3(NTT,"urb: %p buf: %p priv: %p", urb, data, urb->function_privdata);
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

/*! net_fd_recv_urb_ecm
 * @brief - callback to process a received URB
 *
 * @param urb - received urb
 * @param rc dummy
 * @return non-zero for failure.
 */
int net_fd_recv_urb_ecm(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = function_instance->privdata;
        void *os_data = NULL;
        u8 *os_buffer, *net_frame;
        int crc_bad = 0;
        int trim = 0;
        int len;
        u32 crc;
        #ifndef CONFIG_OTG_NETWORK_DOUBLE_OUT
        int endpoint_index = BULK_OUT_A;
        #else /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        int endpoint_index = BULK_OUT_A;
        #endif /* CONFIG_OTG_NETWORK_DOUBLE_OUT */
        int out_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, endpoint_index, usbd_high_speed(function_instance));
        len = urb->actual_length;
        trim = 0;

        TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);
        //RETURN_EINVAL_IF (urb->status == USBD_URB_OK);

        THROW_UNLESS((os_data = net_os_alloc_buffer(function_instance, &os_buffer, len)), error);
        net_frame = os_buffer;

        //TRACE_MSG2(NTT, "os_data: %x os_buffer: %x", os_data, os_buffer);


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
        if (1 == (len % out_pkt_sz)) {
		#if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
                // copy and CRC up to the packetsize boundary
                crc = crc32_copy(os_buffer, urb->buffer, len - 1, CRC32_INIT);
                os_buffer += len - 1;

                // if the CRC is good then this is case 1
                if (CRC32_GOOD != crc) {

                        crc = crc32_copy(os_buffer, urb->buffer + len - 1, 1, crc);
                        os_buffer += 1;

                        if (CRC32_GOOD != crc) {
                                //crc_errors[len%64]++;
                                TRACE_MSG2(NTT,"A CRC error %08x %03x", crc, urb->framenum);
                                THROW_IF(npd->seen_crc, crc_error);
                        }
                        else
                                npd->seen_crc = 1;
                }
                else
                        npd->seen_crc = 1;

		#endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/
        }
        else {

		#if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
                crc = crc32_copy(os_buffer, urb->buffer, len, CRC32_INIT);
                os_buffer += len;

                if (CRC32_GOOD != crc) {
                        //crc_errors[len%64]++;
                        TRACE_MSG2(NTT,"B CRC error %08x %03x", crc, urb->framenum);
                        THROW_IF(npd->seen_crc, crc_error);
                }
                else
                        npd->seen_crc = 1;
		#else /* !defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/
                memcpy (os_buffer, urb->buffer, len);
		#endif /* !defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/
        }
        // trim IFF we are paying attention to crc
        #if defined(CONFIG_OTG_NETWORK_BLAN_CRC) || defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        if (npd->seen_crc)
                trim = 4;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_CRC) ...*/


        if (net_fd_recv_buffer(function_instance, net_frame, len, os_data, crc_bad, trim)) {
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
                }
        }
        return 0;
        //return usbd_start_out_urb (urb);
}



/* ********************************************************************************************* */
#ifdef CONFIG_OTG_NETWORK_START_SINGLE
#define NETWORK_START_URBS 1
#else
#define NETWORK_START_URBS 2
#endif

/*! net_fd_start_recv_ecm - start recv urb(s)
 *
 * @param function_instance - pointer to this function instance
 * @return none
 */
int net_fd_start_recv_ecm(struct usbd_function_instance *function_instance)
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

/*! ecm_fd_function_enable
 * @brief - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 *
 * @param function_instance - function instance pointer
 * @return int - 0 for success
 */

int ecm_fd_function_enable (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd;
        struct usbd_class_ethernet_networking_descriptor *ethernet = &ecm_class_2;

        char address_str[14];

        /* do normal network function enable first - this creates network private data */
        RETURN_EINVAL_IF(net_fd_function_enable(function_instance, network_ecm,
                                net_fd_recv_urb_ecm,
                                net_fd_start_xmit_ecm,
                                net_fd_start_recv_ecm, 0
                                ));

        npd = function_instance->privdata;

        /* if successful then lets allocate iMACAddress string */

        TRACE_MSG6(NTT, "local: %02x:%02x:%02x:%02x:%02x:%02x",
                        npd->local_dev_addr[0], npd->local_dev_addr[1], npd->local_dev_addr[2],
                        npd->local_dev_addr[3], npd->local_dev_addr[4], npd->local_dev_addr[5]);

        TRACE_MSG6(NTT, "remote: %02x:%02x:%02x:%02x:%02x:%02x",
                        npd->remote_dev_addr[0], npd->remote_dev_addr[1], npd->remote_dev_addr[2],
                        npd->remote_dev_addr[3], npd->remote_dev_addr[4], npd->remote_dev_addr[5]);

        snprintf(address_str, 13, "%02x%02x%02x%02x%02x%02x",
                        npd->remote_dev_addr[0], npd->remote_dev_addr[1], npd->remote_dev_addr[2],
                        npd->remote_dev_addr[3], npd->remote_dev_addr[4], npd->remote_dev_addr[5]);

        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);

        return 0;
}


/*! ecm_fd_set_configuration
 * @brief - called to indicate set configuration request was received
 * @param function_instance
 * @param configuration
 * @return int
 */
int ecm_fd_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usb_network_private *npd = function_instance->privdata;

        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        //net_check_mesg(mesg_configured);
        if (npd->eem_os_buffer)
                net_os_dealloc_buffer(function_instance, npd->eem_os_data, npd->eem_os_buffer);

        npd->eem_os_data = npd->eem_os_buffer = NULL;
        //npd->flags |= NETWORK_CONFIGURED;
	npd->altsetting = 0;
        TRACE_MSG3(NTT, "CONFIGURED: %d ipaddr: %08x flags: %04x", configuration, npd->ip_addr, npd->flags);
        return 0;
}



/*!ecm_fd_set_interface
 *
 * @brief called to indicate set interface request was received
 * @param function_instance
 * @param wIndex
 * @param altsetting
 * @return int
 */
int ecm_fd_set_interface (struct usbd_function_instance *function_instance, int wIndex, int altsetting)
{
        struct usb_network_private *npd = function_instance->privdata;

        TRACE_MSG2(NTT, "SET INTERFACE[%02x] altsetting: %02x", wIndex, altsetting);
	npd->altsetting = altsetting;
        TRACE_MSG3(NTT, "ipaddr: %08x flags: %04x altsetting: %02x",
                        npd->ip_addr, npd->flags, npd->altsetting);
	return (altsetting ? net_fd_start : net_fd_stop) (function_instance);
}



/* ********************************************************************************************* */
/*! ecm_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations ecm_fd_function_ops = {
        .function_enable = ecm_fd_function_enable,
        .function_disable = net_fd_function_disable,

        .device_request = net_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .set_configuration = ecm_fd_set_configuration,
        .set_interface = ecm_fd_set_interface,
        .reset = net_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};


/*! function driver description
 */
struct usbd_interface_driver ecm_interface_driver = {
        .driver = {
                .name = "cdc-ecm-if",
                .fops = &ecm_fd_function_ops, },
        .interfaces = sizeof (ecm_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = ecm_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = net_fd_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - CDC/ECM",
};

/* ********************************************************************************************* */

/*! emc_mod_init
 * @brief initialize ecm interface module
 * @return int
 */
int ecm_mod_init (void)
{
        return usbd_register_interface_function (&ecm_interface_driver, "cdc-ecm-if", NULL);
}
/*! ecm_mod_exit
 * @brief cleanup  system resource
 * @return void
 */

void ecm_mod_exit(void)
{
        usbd_deregister_interface_function (&ecm_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_ECM */
/*!
 * @brief ecm_mod_init
 * @return int
 */
int ecm_mod_init (void)
{
        return 0;
}

void ecm_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_ECM */
