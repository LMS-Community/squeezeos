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
 * otg/functions/network/net-ip.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/net-ip.c|20070425221028|12267
 *
 *      Copyright (c) 2002-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/net-ip.c
 * @brief Network Function Driver private defines
 *
 * This contains support for requesting and checking Time Protocol or RARP
 * requests and replies.
 *
 * @ingroup NetworkFunction
 */



/* Belcarra public interfaces */
#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>


#include "network.h"
#include "net-os.h"

#define NTT network_fd_trace_tag
extern otg_tag_t network_fd_trace_tag;

//#define OFFSET(s,e) (&((s *)0)->e)

#define ETH_ALEN                6
#define IP_ALEN                 4
//#define INADDR_BROADCAST        ((u32) 0xffffffff)
#define IPPROTO_UDP             17
#define IPVERSION               4
#define ETHERTYPE_IP            0x0800
#define ETHERTYPE_ARP           0x0806
#define ETHERTYPE_RARP          0x8035

#define ARP_ETHERTYPE           0x01
#define ARP_REQUEST             0x01
#define ARP_REPLY               0x02
#define RARP_REQUEST            0x03
#define RARP_REPLY              0x04

#define RARPD_REQUEST           0x08
#define RARPD_REPLY             0x09
#define RARPD_ERROR             0x0a

#define IP_RF                   0x8000
#define IP_DF                   0x4000
#define IP_MF                   0x2000
#define IP_OFFMASK              0x1fff

/* TOD protocol - RFC868 */
#define TP_PORT                 37
#define RFC868_EPOCH            0x232661280
#define RFC868_OFFSET_TO_EPOCH  0x83AA7E80      // RFC868 - seconds from midnight on 1 Jan 1900 GMT to Midnight 1 Jan 1970 GMT



u8 *mac_bcast_addr =            "\xff\xff\xff\xff\xff\xff";
u8 *mac_zero_addr =             "\x00\x00\x00\x00\x00\x00";

u8 *well_known_host_addr =      "\x02\x00\x00\x00\x00\x01";
u8 *well_known_peripheral_addr = "\x02\x00\x00\x00\x00\x00";

/*! @name Structure definitions for each frame section
 *
 * @{
 */
/*! @struct my_ether_header  net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief ether header wrapper
 */
struct my_ether_header {
        u8      ether_dhost[ETH_ALEN];
        u8      ether_shost[ETH_ALEN];
        u16     ether_type;
}  __attribute__ ((__packed__));


/* ********************************************************************************************** */
/*! @struct arp_header net-ip.c  "otg/functions/network/net-ip.c"
 *
 * @brief  arp_header struct
 */
struct arp_header {
        u16     arp_hard;
        u16     arp_prot;
        u8      arp_hsize;
        u8      arp_psize;
        u16     arp_op;
}  __attribute__ ((__packed__));

/*! @struct arp_message  net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief arp message struct
 */
struct arp_message {
        u8      arp_sha[ETH_ALEN];
        u32     arp_sip;
        u8      arp_tha[ETH_ALEN];
        u32     arp_tip;
}  __attribute__ ((__packed__));

/*! @struct arp_frame  net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief arp frame data structure
 */
struct arp_frame {
        struct my_ether_header  eh;
        struct arp_header       aph;
        struct arp_message      apm;
}  __attribute__ ((__packed__));


/* ********************************************************************************************** */
/*! @struct my_ip net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief ip packet structure
 */
struct my_ip {
        union {
                u8 ip_v;             // LSB 4 bits
                u8 ip_hl;            // MSB 4 bits
        }__attribute__ ((__packed__)) ip;
        u8      ip_tos;
        s16     ip_len;
        s16     ip_id;
        s16     ip_off;
        u8      ip_ttl;
        u8      ip_p;
        s16     ip_sum;
        u32     ip_src;
        u32     ip_dst;

}  __attribute__ ((__packed__));

/*! @struct my_udphdr  net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief udb header structure
 */
struct my_udphdr {
        u16     uh_sport;
        u16     uh_dport;
        u16     uh_ulen;
        u16     uh_sum;
}  __attribute__ ((__packed__));

/*! @struct tp_message net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief tp message data structure
 */
struct tp_message {
        u32     time;
}  __attribute__ ((__packed__));

/*! @struct tp_frame  net-ip.c  "otg/functions/network/net-ip.c"
 *
 *  @brief  tp frame data structure
 */
struct tp_frame {
        struct my_ether_header  eh;
        struct my_ip            iph;
        struct my_udphdr        udph;
        struct tp_message       tp_message;
}  __attribute__ ((__packed__));

/* @} */
/* ********************************************************************************************** */
/*!  checksum - compute and return checksum for length length
+ *   @param buf - buffer
+ *   @param length - buffer length
+ *
+ *   @return checksum value
+ */

u16 checksum(void *buf, int length) {
        u32 sum;
        u16 *ip = (u16 *) buf;
        for (sum = 0; length > 1; length -= 2 )  sum += *ip++;

        // process extra byte if there is one
        if( length > 0 )
                sum += * (u8 *) ip;

        //  munge
        while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);

        return (u16) ~sum;
}

/* ********************************************************************************************** */
extern u32 ip_addr;
extern u32 router_ip;
extern u32 network_mask;
extern u32 dns_server_ip;
extern BOOL do_settime;
#if defined(LINUX24)
extern struct timeval net_fd_tv;
#else
extern struct timespec net_fd_tv;
#endif

static u32 ip_id = 1;


/* ********************************************************************************************** */

 /*! net_fd_check_tp_response - check for Time Protocol (UDP) response
 *  @param function_instance - function instance pointer
 *  @param buffer            - buffer area
 *  @param length            - buffer lenght
 *  @return BOOL value
 */
BOOL net_fd_check_tp_response(struct usbd_function_instance *function_instance, u8 *buffer, int length)
{
        struct usb_network_private *npd = function_instance->privdata;

        struct tp_frame         *frame = (struct tp_frame *)buffer;

        unsigned int            ip_len;

        struct my_ether_header  *eh = (struct my_ether_header *) &frame->eh;
        struct my_ip            *iph  = (struct my_ip *) &frame->iph;
        struct my_udphdr        *udph = (struct my_udphdr *) &frame->udph;
        struct tp_message       *tp_message = (struct tp_message *) &frame->tp_message;


        //TRACE_MSG1(NTT, "length: %d", length);
        //TRACE_MSG2(NTT, "length: %d  sizeof(my_ether_header): %d", length, sizeof(struct my_ether_header));

        RETURN_FALSE_IF(length < (sizeof(struct my_ether_header) + sizeof(struct my_ip)));      /* sanity check */


        //TRACE_MSG6(NTT, "ether_dhost: %02x:%02x:%02x:%02x:%02x:%02x", eh->ether_dhost[0], eh->ether_dhost[1],
        //              eh->ether_dhost[2], eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5]);
        //TRACE_MSG6(NTT, "local_addr:  %02x:%02x:%02x:%02x:%02x:%02x", npd->local_dev_addr[0], npd->local_dev_addr[1],
        //              npd->local_dev_addr[2], npd->local_dev_addr[3], npd->local_dev_addr[4], npd->local_dev_addr[5]);
        RETURN_FALSE_IF(memcmp(eh->ether_dhost, npd->local_dev_addr, ETH_ALEN));        /* check if broadcast */

        //TRACE_MSG2(NTT, "ether_type: %04x %04x", ETHERTYPE_IP, ntohs(eh->ether_type));
        RETURN_FALSE_IF(ETHERTYPE_IP != ntohs(eh->ether_type));                         /* check ethernet frame type */

        //TRACE_MSG1(NTT, "ip_v: %04x", iph->ip.ip_v & 0xf0 >> 4 );

        RETURN_FALSE_IF(IPVERSION != (iph->ip.ip_v & 0xf0) >> 4);                          /* check for ipv4 */

        //TRACE_MSG3(NTT, "ip_hl: %04x %04x %04x", iph->ip.ip_v & 0xf, 5, (length - sizeof(struct my_ether_header) ));

        RETURN_FALSE_IF((iph->ip.ip_hl & 0x0f) < 5);                                       /* check ipv4 header length ok */
        RETURN_FALSE_IF( ((iph->ip.ip_hl & 0x0f) * 4) > (u8)(length - sizeof(struct my_ether_header)));

        //TRACE_MSG2(NTT, "ip_len: %04x %04x", iph->ip_len, (length - sizeof(struct my_ether_header)));
        RETURN_FALSE_IF(ntohs(iph->ip_len) > (length - sizeof(struct my_ether_header)));        /* check ipv4 total length ok */

        //TRACE_MSG2(NTT, "ip_p: %04x %04x", iph->ip_p, IPPROTO_UDP);
        RETURN_FALSE_IF(IPPROTO_UDP != iph->ip_p);                                      /* check for UDP */

        udph = (struct my_udphdr *) (((u8*) iph) + ((iph->ip.ip_hl & 0x0f) * 4));          /* check for TP port 37 */
        //TRACE_MSG2(NTT, "uh_dport: %04x uh_sport: %04x", udph->uh_dport, udph->uh_sport);
        RETURN_FALSE_IF((TP_PORT != ntohs(udph->uh_dport)) || ((TP_PORT != ntohs(udph->uh_sport))));

        //TRACE_MSG3(NTT, "found port: uh_ulen: %x %x sizeof: %x",
        //              udph->uh_ulen, ntohs(udph->uh_ulen), sizeof(struct tp_message));

        RETURN_FALSE_UNLESS(ntohs(udph->uh_ulen));                                      /* check if zero length payload */
        //TRACE_MSG0(NTT, "uh_ulen ok");

        /* get time, convert to Unix time - seconds since midnight 1 jan 1970
         */
        npd->rfc868time = tp_message->time;
        //net_os_settime(function_instance, ntohl(tp_message->time));

        TRACE_MSG0(NTT, "found");
        return TRUE;
}


/*! net_fd_send_tp_request - allocate and send Time Protocol (UDP) request
 *
 * @param function_instance - function instance
 * @return none
  */
void net_fd_send_tp_request(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct tp_frame        tp_frame;
        struct tp_frame        *tx = &tp_frame;
        //struct tp_message      *todpmsg = &tp_frame.tp_message;


        char buffer[6];

        //TRACE_MSG2(NTT, "sizeof(my_ip) sizeof(tp_frame): %d XXX", sizeof(struct my_ip), sizeof(struct tp_frame));

        memset(&tp_frame, 0, sizeof(struct tp_frame));

        /* ethernet frame - provisionally setup with client mac address */
        tx->eh.ether_type = htons(ETHERTYPE_IP);
        memset(tx->eh.ether_dhost, 0xff, ETH_ALEN);
        memcpy(buffer, npd->local_dev_addr, ETH_ALEN);
        memcpy(tx->eh.ether_shost, buffer, ETH_ALEN);
        memcpy(tx->eh.ether_shost, npd->local_dev_addr, ETH_ALEN);

        /* ip header */
        tx->iph.ip.ip_v = (IPVERSION << 4) | 5;
        tx->iph.ip_src = htonl(npd->ip_addr);
        tx->iph.ip_dst = htonl(npd->router_ip);
        tx->iph.ip_p = IPPROTO_UDP;


        /* udp - checksum will be added later */
        tx->udph.uh_sport = htons(TP_PORT);
        tx->udph.uh_dport = htons(TP_PORT);
        tx->udph.uh_ulen = htons(sizeof(struct my_udphdr) + sizeof(struct tp_message));

        //TRACE_MSG6(NTT, "port: %04x %04x %04x:%04x %04x:%04x", tx->udph.uh_sport, tx->udph.uh_dport,
        //              TP_PORT, htons(TP_PORT), 0x1234, htons(0x1234));

        /* udp checksum - we can check sum entire ip packet as rest is zero */
        tx->udph.uh_sum = checksum( &(tx->iph),
                        sizeof(struct my_ip) + sizeof(struct my_udphdr) + sizeof(struct tp_message));

        /* ip - done here after udp checksum has */
        tx->iph.ip.ip_hl |= sizeof(struct my_ip) >> 2; // XXX
        tx->iph.ip.ip_v |= IPVERSION << 4; // XXX

        tx->iph.ip_id = htons(ip_id++);
        tx->iph.ip_ttl = 0x80;

        tx->iph.ip_len = htons(sizeof(struct my_ip) + sizeof(struct my_udphdr) + sizeof(struct tp_message));
        tx->iph.ip_sum = checksum(&(tx->iph), sizeof(struct my_ip));

        net_fd_start_xmit(function_instance, (u8 *) tx, sizeof(tp_frame), NULL);

        TRACE_MSG1(NTT, "EXIT: tp_frame: %p", &tp_frame);
}

/* ********************************************************************************************** */

/*! net_fd_check_rarp_reply - check for RARP (UDP) response
 *
 * @param function_instance - function instance
 * @param buffer            - buffer pointer
 * @param length            - buffer length
 * @return BOOL value
 */
BOOL net_fd_check_rarpd_reply(struct usbd_function_instance *function_instance, u8 *buffer, int length)
{
        struct usb_network_private *npd = function_instance->privdata;

        struct arp_frame        *frame = (struct arp_frame *)buffer;

        //unsigned int            ip_len;


        struct my_ether_header  *eh = (struct my_ether_header *) &frame->eh;
        struct arp_header       *aph = (struct arp_header *) &frame->aph;
        struct arp_message      *apm = (struct arp_message *) &frame->apm;


        //TRACE_MSG2(NTT, "length: %d  sizeof(my_ether): %d", length, sizeof(struct arp_frame));
        RETURN_FALSE_IF(length < (sizeof(struct arp_frame)));           /* sanity check */

        //TRACE_MSG6(NTT, "ether_dhost: %02x:%02x:%02x:%02x:%02x:%02x",
        //                eh->ether_dhost[0], eh->ether_dhost[1],
        //              eh->ether_dhost[2], eh->ether_dhost[3],
        //                eh->ether_dhost[4], eh->ether_dhost[5]);

        //TRACE_MSG6(NTT, "local_addr:  %02x:%02x:%02x:%02x:%02x:%02x",
        //                well_known_peripheral_addr[0], well_known_peripheral_addr[1],
        //                well_known_peripheral_addr[2], well_known_peripheral_addr[3],
        //                well_known_peripheral_addr[4], well_known_peripheral_addr[5]);

        RETURN_FALSE_IF(memcmp(eh->ether_dhost, well_known_peripheral_addr, ETH_ALEN));         /* check if broadcast */

        //TRACE_MSG2(NTT, "ether_type: %04x %04x", ETHERTYPE_RARP, ntohs(eh->ether_type));
        RETURN_FALSE_IF(ETHERTYPE_RARP != ntohs(eh->ether_type));               /* ethernet frame type RARP */


        //TRACE_MSG2(NTT, "arp_hard: %04x %04x", ARP_ETHERTYPE, ntohs(aph->arp_hard));
        RETURN_FALSE_IF(ARP_ETHERTYPE != ntohs(aph->arp_hard));                 /* check hard type */

        //TRACE_MSG2(NTT, "arp_op: %04x %04x", RARPD_REPLY, ntohs(aph->arp_op));
        RETURN_FALSE_IF(RARPD_REPLY != ntohs(aph->arp_op));                     /* check op for RARP request */

        //TRACE_MSG2(NTT, "arp_prot: %04x %04x", ETHERTYPE_IP, ntohs(aph->arp_prot));
        RETURN_FALSE_IF(ETHERTYPE_IP != ntohs(aph->arp_prot));                  /* check prot */

        //TRACE_MSG2(NTT, "hsize: %04x psize: %04x", aph->arp_hsize, aph->arp_psize);
        RETURN_FALSE_IF(ETH_ALEN != aph->arp_hsize);                            /* check hard size */
        RETURN_FALSE_IF(4 != aph->arp_psize);                                   /* check prot size */

        /* check if for us */
        //TRACE_MSG1(NTT, "arp_tip: %08x", apm->arp_tip);
        //RETURN_FALSE_IF(apm->arp_tip != htonl(ip_server(oldFrame)));


        // XXX what about locally set addr
        // XXX UNLESS(memcmp(npd->local_dev_addr, mac_zero_addr, ETH_ALEN))
        //        memcpy(npd->local_dev_addr, apm->arp_tha, ETH_ALEN);
        TRACE_MSG0(NTT, "sos908");
        if(!(npd->override_MAC)) memcpy(npd->local_dev_addr, apm->arp_tha, ETH_ALEN);
        //TRACE_MSG6(NTT, "arp_tha:     %02x:%02x:%02x:%02x:%02x:%02x", apm->arp_tha[0], apm->arp_tha[1],
        //              apm->arp_tha[2], apm->arp_tha[3], apm->arp_tha[4], apm->arp_tha[5]);

        npd->ip_addr = ntohl(apm->arp_tip);
        npd->router_ip = ntohl(apm->arp_sip);

        //TRACE_MSG1(NTT, "arp_tip:     %08x", apm->arp_tip);

        //net_os_config(function_instance);
        //net_os_hotplug(function_instance);

        //net_fd_send_tp_request(function_instance);


        TRACE_MSG0(NTT, "found");
        return TRUE;
}

 /*! net_fd_check_rarp_request - check for RARP (UDP) response
 *
 * @param function_instance - function instance
 * @param buffer            - buffer pointer
 * @param length            - buffer length
 * @return BOOL value
 */
BOOL net_fd_check_rarpd_request(struct usbd_function_instance *function_instance, u8 *buffer, int length)
{
        struct usb_network_private *npd = function_instance->privdata;

        struct arp_frame        *frame = (struct arp_frame *)buffer;

        //unsigned int            ip_len;


        struct my_ether_header  *eh = (struct my_ether_header *) &frame->eh;
        struct arp_header       *aph = (struct arp_header *) &frame->aph;
        //struct arp_message      *apm = (struct arp_message *) &frame->apm;


        //TRACE_MSG2(NTT, "length: %d  sizeof(my_ether): %d", length, sizeof(struct arp_frame));
        RETURN_FALSE_IF(length < (sizeof(struct arp_frame)));           /* sanity check */

        //TRACE_MSG6(NTT, "ether_dhost: %02x:%02x:%02x:%02x:%02x:%02x", eh->ether_dhost[0], eh->ether_dhost[1],
        //              eh->ether_dhost[2], eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5]);
        //TRACE_MSG6(NTT, "local_addr:  %02x:%02x:%02x:%02x:%02x:%02x", npd->local_dev_addr[0], npd->local_dev_addr[1],
        //              npd->local_dev_addr[2], npd->local_dev_addr[3], npd->local_dev_addr[4], npd->local_dev_addr[5]);

        //TRACE_MSG2(NTT, "ether_type: %04x %04x", ETHERTYPE_RARP, ntohs(eh->ether_type));

        RETURN_FALSE_IF(memcmp(eh->ether_dhost, mac_bcast_addr, ETH_ALEN));     /* check if broadcast */
        RETURN_FALSE_IF(ETHERTYPE_RARP != ntohs(eh->ether_type));               /* ethernet frame type RARP */


        TRACE_MSG2(NTT, "arp_hard: %04x %04x", ARP_ETHERTYPE, ntohs(aph->arp_hard));
        RETURN_FALSE_IF(ARP_ETHERTYPE != ntohs(aph->arp_hard));                 /* check hard type */

        TRACE_MSG2(NTT, "arp_op: %04x %04x", RARPD_REPLY, ntohs(aph->arp_op));
        RETURN_FALSE_IF(RARPD_REQUEST != ntohs(aph->arp_op));           /* check op for RARP request */

        TRACE_MSG2(NTT, "arp_prot: %04x %04x", ETHERTYPE_IP, ntohs(aph->arp_prot));
        RETURN_FALSE_IF(ETHERTYPE_IP != ntohs(aph->arp_prot));          /* check prot */

        //TRACE_MSG2(NTT, "hsize: %04x psize: %04x", aph->arp_hsize, aph->arp_psize);
        RETURN_FALSE_IF(ETH_ALEN != aph->arp_hsize);                    /* check hard size */
        RETURN_FALSE_IF(4 != aph->arp_psize);                           /* check prot size */

        /* check if for us */
        //TRACE_MSG1(NTT, "arp_tip: %08x", apm->arp_tip);
        //THROW_IF(apm->arp_tip != htonl(ip_server(oldFrame)));
        //

        //UNLESS(memcmp(npd->local_dev_addr, mac_zero_addr, ETH_ALEN))
        //      memcpy(npd->local_dev_addr, apm->arp_tha, ETH_ALEN);

        //TRACE_MSG6(NTT, "arp_tha:     %02x:%02x:%02x:%02x:%02x:%02x", apm->arp_tha[0], apm->arp_tha[1],
        //              apm->arp_tha[2], apm->arp_tha[3], apm->arp_tha[4], apm->arp_tha[5]);

        //npd->ip_addr = ntohl(apm->arp_tip);
        //npd->router_ip = ntohl(apm->arp_sip);

        //TRACE_MSG1(NTT, "arp_tip:     %08x", apm->arp_tip);

        //net_os_config(function_instance);
        //net_os_hotplug(function_instance);

        //net_fd_send_tp_request(function_instance);


        TRACE_MSG0(NTT, "found");
        return TRUE;
}

/*! net_fd_send_rarpd_request - allocate and send RARP request
 *
 * @param function_instance - function instance
 * @return none
 */
void net_fd_send_rarpd_request(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct arp_frame        arp_frame;
        //struct arp_header       *aph = (struct arp_header *) &arp_frame.aph;
        //struct arp_message      *apm = (struct arp_message *) &arp_frame.apm;
        struct arp_frame        *tx = &arp_frame;

        //TRACE_MSG1(NTT, "sizeof(arp_frame): %d XXX", sizeof(struct arp_frame));

        memset(tx, 0, sizeof(struct arp_frame));

        /* copy original frame */
        //memcpy(tx, rx, sizeof(struct arp_frame));

        /* set mac addresses */
        memset(tx->eh.ether_dhost, 0xff, ETH_ALEN);
        memcpy(tx->eh.ether_shost, well_known_peripheral_addr, ETH_ALEN);

        /* set target ethernet addr */
        memset(tx->apm.arp_tha, 0xff, ETH_ALEN+4);
        memcpy(tx->apm.arp_sha, npd->local_dev_addr, ETH_ALEN);

        tx->eh.ether_type = htons(ETHERTYPE_RARP);
        tx->aph.arp_prot = htons(ETHERTYPE_IP );
        tx->aph.arp_hard = htons(ARP_ETHERTYPE );
        tx->aph.arp_op = htons(RARPD_REQUEST);
        tx->aph.arp_hsize = ETH_ALEN;
        tx->aph.arp_psize = 4;
        tx->apm.arp_sip = htonl(0);
        tx->apm.arp_tip = htonl(0);

        // checksum

        net_fd_start_xmit(function_instance, (u8 *) tx, sizeof(arp_frame), NULL);

        TRACE_MSG1(NTT, "frame: %p exit", &arp_frame);
}


 /*! net_fd_send_rarpd_reply - allocate and send RARP request
 * @param function_instance - function instance
 * @return none
 */
void net_fd_send_rarpd_reply(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct arp_frame        arp_frame;
        //struct arp_header       *aph = (struct arp_header *) &arp_frame.aph;
        //struct arp_message      *apm = (struct arp_message *) &arp_frame.apm;
        struct arp_frame        *tx = &arp_frame;

        //TRACE_MSG1(NTT, "sizeof(arp_frame): %d XXX", sizeof(struct arp_frame));

        memset(tx, 0, sizeof(struct arp_frame));

        /* copy original frame */
        //memcpy(tx, rx, sizeof(struct arp_frame));

        /* set mac addresses */
        memset(tx->eh.ether_dhost, 0xff, ETH_ALEN);
        memcpy(tx->eh.ether_shost, npd->local_dev_addr, ETH_ALEN);

        /* set target ethernet addr */
        memset(tx->apm.arp_tha, 0xff, ETH_ALEN+4);
        memcpy(tx->apm.arp_sha, npd->local_dev_addr, ETH_ALEN);

        tx->eh.ether_type = htons(ETHERTYPE_RARP);
        tx->aph.arp_prot = htons(ETHERTYPE_IP );
        tx->aph.arp_hard = htons(ARP_ETHERTYPE );
        tx->aph.arp_op = htons(RARPD_REQUEST);
        tx->aph.arp_hsize = ETH_ALEN;
        tx->aph.arp_psize = 4;
        tx->apm.arp_sip = htonl(0);
        tx->apm.arp_tip = htonl(0);

        // checksum

        net_fd_start_xmit(function_instance, (u8 *) tx, sizeof(arp_frame), NULL);

        TRACE_MSG1(NTT, "frame: %p exit", &arp_frame);
}

/* End of FILE */
