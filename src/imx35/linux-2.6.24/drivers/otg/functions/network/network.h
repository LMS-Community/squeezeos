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
 * otg/functions/network/network.h - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/network.h|20070814184652|05358
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
 * @defgroup NetworkFunction Network CDC Interface Function
 * @ingroup InterfaceFunctions
 */
/*!
 * @file otg/functions/network/network.h
 * @brief Network Function Driver private defines
 *
 * This defines the internal and private structures required
 * by the Network Function Driver.
 *
 * @ingroup NetworkFunction
 */

/*! This is a test
 */
#ifndef NETWORK_FD_H
#define NETWORK_FD_H 1

#undef CONFIG_OTG_NETWORK_DOUBLE_OUT
//#define CONFIG_OTG_NETWORK_DOUBLE_OUT
#undef CONFIG_OTG_NETWORK_DOUBLE_IN
//#define CONFIG_OTG_NETWORK_DOUBLE_IN

#undef CONFIG_OTG_NETWORK_HS

#undef  CONFIG_OTG_NETWORK_XMIT_OS
#undef  CONFIG_OTG_NETWORK_XMIT_OS

#include <otg/otg-trace.h>

#define NTT network_fd_trace_tag
extern otg_tag_t network_fd_trace_tag;

#ifdef CONFIG_OTG_NETWORK_START_SINGLE
#define NETWORK_START_URBS 1
#else
#define NETWORK_START_URBS 2
#endif


// Some platforms need to get rid of "static" when using GDB or looking at ksyms
//#define STATIC
#define STATIC static

typedef enum network_config_status {
        config_unknown,
        config_attached,
        config_detached
} network_config_status_t;

typedef enum network_hotplug_status {
        hotplug_unknown,
        hotplug_attached,
        hotplug_detached
} network_hotplug_status_t;

typedef enum network_type {
        network_unknown,
        network_ecm,
        network_eem,
        network_blan,
        network_safe,
        network_basic,
        network_basic2,
} network_type_t;

typedef enum eem_data_type {
        eem_no_data,
        eem_frame_crc_data,
        eem_frame_no_crc_data,
        eem_echo_data,
        eem_echo_response_data
} eem_data_type_t;

#if 0
struct usb_network_params {
        // enabling switchs
        u32 cdc;
        u32 basic;
        u32 basic2;
        u32 safe;
        u32 blan;
        // capability flags
        u32 cdc_capable;
        u32 basic_capable;
        u32 basic2_capable;
        u32 safe_capable;
        u32 blan_capable;
        // overrides
        u32 vendor_id;
        u32 product_id;
        char *local_mac_address_str;
        char *remote_mac_address_str;
        // other switches
        u32 ep0test;
        u32 zeroconf;
};
#endif

#define CONFIG_OTG_NETWORK_ALLOW_SETID           1

//#define NETWORK_CREATED         0x01
//#define NETWORK_REGISTERED      0x02

#define NETWORK_INFRASTRUCTURE  0x01
#define NETWORK_DESTROYING      0x04
#define NETWORK_ENABLED         0x08
#define NETWORK_CONFIGURED      0x10
#define NETWORK_OPEN            0x20


#define MCCI_ENABLE_CRC         0x03
#define BELCARRA_GETMAC         0x04

#define BELCARRA_SETTIME        0x04
#define BELCARRA_SETIP          0x05
#define BELCARRA_SETMSK         0x06
#define BELCARRA_SETROUTER      0x07
#define BELCARRA_SETDNS         0x08
#define BELCARRA_PING           0x09
#define BELCARRA_SETFERMAT      0x0a
#define BELCARRA_HOSTNAME       0x0b


// RFC868 - seconds from midnight on 1 Jan 1900 GMT to Midnight 1 Jan 1970 GMT
#define RFC868_OFFSET_TO_EPOCH  0x83AA7E80

#define NET_ETH_ALEN            6

typedef int (*net_recv_urb_proc_t) (struct usbd_urb *urb, int rc);
typedef int (*net_start_xmit_proc_t) (struct usbd_function_instance *, u8 *, int, void*);
typedef int (*net_start_recv_proc_t) (struct usbd_function_instance *);

/*! @struct usb_network_private network.h  otg/functions/network/networks.h
 */

struct usb_network_private {

        //struct net_device_stats stats;  /* network device statistics */

        int flags;
        int altsetting;
        struct net_device *net_device;
        struct net_device_stats *net_device_stats;
        struct usbd_function_instance *function_instance;
        struct usbd_simple_driver *simple_driver;
        //struct usb_network_params *params;
        //struct net_usb_services *fd_ops;
        unsigned int maxtransfer;
        //rwlock_t rwlock;

        network_config_status_t config_status;
        network_hotplug_status_t hotplug_status;
        network_type_t network_type;

        int state;

        int mtu;
        int use_crc;
        int seen_crc;
        int seen_crc_error;
#if defined(CONFIG_OTG_NETWORK_BLAN_FERMAT)
        int fermat;
#endif

        unsigned int stops;
        unsigned int restarts;

        int max_recv_urbs;
        otg_atomic_t recv_urbs_started[2];
        otg_atomic_t xmit_urbs_started[2];

        unsigned int max_queued_frames;
        unsigned int max_queued_bytes;

        otg_atomic_t queued_frames;
        otg_atomic_t queued_bytes;

        time_t avg_queue_frames;

        time_t jiffies;
        unsigned long samples;

        int have_interrupt;

        int data_notify;

        struct usbd_urb *int_urb;

        #if 0
        struct OLD_WORK_ITEM notification_bh;
        #if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
        struct OLD_WORK_ITEM config_bh;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
        #ifdef CONFIG_HOTPLUG
        struct OLD_WORK_ITEM hotplug_bh;
        #endif /* CONFIG_HOTPLUG */
        #else
        struct otg_workitem *notification_workitem;

        #if !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC)
        struct otg_task *blan_recv_task;
        #endif /* !defined(CONFIG_OTG_NETWORK_BLAN_CRC) && !defined(CONFIG_OTG_NETWORK_SAFE_CRC) */

        #if defined(CONFIG_OTG_NETWORK_CONNECT)
        struct otg_workitem *connect_workitem;
        #endif /* defined(CONFIG_OTG_NETWORK_CONNECT)  */
        #if defined(CONFIG_OTG_NETWORK_DISCONNECT)
        struct otg_workitem *disconnect_workitem;
        #endif /* defined(CONFIG_OTG_NETWORK_DISCONNECT)  */

        #if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
        struct otg_workitem *config_workitem;
        struct otg_task *config_otgtask;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
        #ifdef CONFIG_HOTPLUG
        struct otg_workitem *hotplug_workitem;
        #endif /* CONFIG_HOTPLUG */
        #endif

        u8 local_dev_addr[NET_ETH_ALEN];
        BOOL local_dev_set;
        u8 remote_dev_addr[NET_ETH_ALEN];
        BOOL remote_dev_set;


        BOOL eem_fragment_valid;        // a fragment of eem packet exists
        u8 eem_fragment_data;           // the eem packet fragment

        void *eem_os_data;              // partially filled network buffer
        u8 *eem_os_buffer;              // partially filled network buffer
        int eem_frame_length;           // frame length for allocated network buffer
        int eem_os_buffer_used;         // amount of data in network buffer

        eem_data_type_t eem_data_type;

        u8 eem_bmCRC;                   // bmCRC flag
        u32 eem_crc;                    // crc for previous data if bmCRC set

        BOOL eem_send_zle_data;         // sent ZLE command byte, still need it's data byte

        u32 ip_addr;
        u32 router_ip;
        u32 network_mask;
        u32 dns_server_ip;

        u32 rfc868time;

        char * local_dev_addr_str;
        char * remote_dev_addr_str;
        BOOL   override_MAC;
        int infrastructure_device;

        void *privdata;
        net_recv_urb_proc_t     net_recv_urb;
        net_start_xmit_proc_t   net_start_xmit;
        net_start_recv_proc_t   net_start_recv;

        u32                     recv_urb_flags;
};

// XXX this needs to be co-ordinated with rndis.c maximum's
#define MAXFRAMESIZE 2000

#if !defined(CONFIG_OTG_MANUFACTURER)
        #define CONFIG_OTG_MANUFACTURER                "Belcarra"
#endif


#if !defined(CONFIG_OTG_SERIAL_NUMBER_STR)
        #define CONFIG_OTG_SERIAL_NUMBER_STR           ""
#endif

/*
 * Lineo specific
 */

#define VENDOR_SPECIFIC_CLASS           0xff
#define VENDOR_SPECIFIC_SUBCLASS        0xff
#define VENDOR_SPECIFIC_PROTOCOL        0xff

/*
 * Lineo Classes
 */
#define LINEO_CLASS                     0xff

#define LINEO_SUBCLASS_BASIC_NET          0x01
#define LINEO_SUBCLASS_BASIC_SERIAL       0x02

/*
 * Lineo Protocols
 */
#define LINEO_BASIC_NET_CRC             0x01
#define LINEO_BASIC_NET_CRC_PADDED      0x02

#define LINEO_BASIC_SERIAL_CRC          0x01
#define LINEO_BASIC_SERIAL_CRC_PADDED   0x02


/*
 * endpoint and interface indexes
 */


#if !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
#define BULK_OUT_A      0x00
#define BULK_IN_A       0x01
#define INT_IN          0x02
#define ENDPOINTS       0x03

#elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && !defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
#define BULK_OUT_A      0x00
#define BULK_IN_A       0x01
#define BULK_IN_B       0x02
#define INT_IN          0x03
#define ENDPOINTS       0x04

#elif !defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
#define BULK_OUT_A      0x00
#define BULK_IN_A       0x01
#define BULK_OUT_B      0x02
#define INT_IN          0x03
#define ENDPOINTS       0x04

#elif defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT)
#define BULK_OUT_A      0x00
#define BULK_IN_A       0x01
#define BULK_OUT_B      0x02
#define BULK_IN_B       0x03
#define INT_IN          0x04
#define ENDPOINTS       0x05

#endif /* defined(CONFIG_OTG_NETWORK_DOUBLE_IN) && defined(CONFIG_OTG_NETWORK_DOUBLE_OUT) */

#if 0
#ifndef CONFIG_OTG_NETWORK_HS
#define BULK_OUT        0x00
#define BULK_IN         0x01
#define INT_IN          0x02
#define ENDPOINTS       0x03
#else /* CONFIG_OTG_NETWORK_HS */
#define BULK_OUT_A      0x00
#define BULK_IN_A       0x01
#define BULK_OUT_B      0x02
#define BULK_IN_B       0x03
#define INT_IN          0x04
#define ENDPOINTS_HS    0x05
#endif /* CONFIG_OTG_NETWORK_HS */
#endif

#define COMM_INTF       0x00
#define DATA_INTF       0x01

extern struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1];
extern u8 net_fd_endpoint_index[ENDPOINTS];
extern u8 network_requested_endpoints[ENDPOINTS+1];
extern u16 network_requested_transferSizes[ENDPOINTS+1];

extern struct usbd_endpoint_request cdc_int_endpoint_requests[1+1];
extern struct usbd_endpoint_request cdc_data_endpoint_requests[2+1];


extern u8 cdc_data_endpoint_index[2];
extern u8 cdc_int_endpoint_index[1];

/* bmDataCapabilities */
#define BMDATA_CRC                      0x01
#define BMDATA_PADBEFORE                0x02
#define BMDATA_PADAFTER                 0x04
#define BMDATA_FERMAT                   0x08
#define BMDATA_HOSTNAME                 0x10

/* bmNetworkCapabilities */
#define BMNETWORK_SET_PACKET_OK         0x01
#define BMNETWORK_NONBRIDGED            0x02
#define BMNETWORK_DATA_NOTIFY_OK        0x04
#define BMNETWORK_NO_VENDOR             0x08


/*
 * BLAN Data Plane
 */
//#define CONFIG_OTG_NETWORK_PADBYTES  8
//#define CONFIG_OTG_NETWORK_PADAFTER  1
//#undef CONFIG_OTG_NETWORK_PADBEFORE
//#define CONFIG_OTG_NETWORK_CRC       1



//extern struct usb_network_private Usb_network_private;
//extern u8 local_dev_addr[ETH_ALEN];
//extern u8 remote_dev_addr[ETH_ALEN];

extern struct usbd_function_operations net_fd_function_ops;

/*! @struct usbd_class_safe_networking_mdlm_descriptor network.h otg/functions/network/network.h
 */
struct usbd_class_safe_networking_mdlm_descriptor {
        u8 bFunctionLength;           // 0x06
        u8 bDescriptorType;           // 0x24
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;       // 0x00
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
} __attribute__ ((packed));

/*! @struct usbd_class_blan_networking_mdlm_descriptor network.h otg/functions/network/network.h
 */
struct usbd_class_blan_networking_mdlm_descriptor {
        u8 bFunctionLength;           // 0x07
        u8 bDescriptorType;           // 0x24
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;       // 0x01
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
        u8 bPad;
} __attribute__ ((packed));



#if 0
#define NETWORK_CREATED         0x01
#define NETWORK_REGISTERED      0x02
#define NETWORK_DESTROYING      0x04
#define NETWORK_ENABLED         0x08
#define NETWORK_ATTACHED        0x10
#define NETWORK_OPEN            0x20


#define MCCI_ENABLE_CRC         0x03
#define BELCARRA_GETMAC         0x04

#define BELCARRA_SETTIME        0x04
#define BELCARRA_SETIP          0x05
#define BELCARRA_SETMSK         0x06
#define BELCARRA_SETROUTER      0x07
#define BELCARRA_SETDNS         0x08
#define BELCARRA_PING           0x09
#define BELCARRA_SETFERMAT      0x0a
#define BELCARRA_HOSTNAME       0x0b
#define BELCARRA_DATA_NOTIFY    0x0c
#endif

#define RFC868_OFFSET_TO_EPOCH  0x83AA7E80      // RFC868 - seconds from midnight on 1 Jan 1900 GMT to Midnight 1 Jan 1970 GMT


extern u32 *network_crc32_table;

#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation

#define COMPUTE_FCS(val, c) (((val) >> 8) ^ network_crc32_table[((val) ^ (c)) & 0xff])


#if 0
#if !defined(CONFIG_OTG_NETWORK_CRC_DUFF4) && !defined(CONFIG_OTG_NETWORK_CRC_DUFF8)
/**
 * Copies a specified number of bytes, computing the 32-bit CRC FCS as it does so.
 *
 * dst   Pointer to the destination memory area.
 * src   Pointer to the source memory area.
 * len   Number of bytes to copy.
 * val   Starting value for the CRC FCS.
 *
 * Returns      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static u32 __inline__ crc32_copy (u8 *dst, u8 *src, int len, u32 val)
{
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = *src++));
        return val;
}

#endif /* DUFFn */
#endif



int net_fd_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request);
void net_fd_event_handler (struct usbd_function_instance *function_instance, usbd_device_event_t event, int data);
void net_fd_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress);
int net_fd_set_configuration (struct usbd_function_instance *function, int configuration);
//int net_fd_set_configuration_blan (struct usbd_function_instance *function, int configuration);
//int net_fd_set_configuration_cdc (struct usbd_function_instance *function, int configuration);
//int net_fd_set_interface_cdc (struct usbd_function_instance *function, int wIndex, int altsetting);
int net_fd_start (struct usbd_function_instance *function);
int net_fd_stop (struct usbd_function_instance *function);
int net_fd_reset (struct usbd_function_instance *function);
int net_fd_suspended (struct usbd_function_instance *function);
int net_fd_resumed (struct usbd_function_instance *function);

int net_fd_function_enable (struct usbd_function_instance *, network_type_t,
        net_recv_urb_proc_t , net_start_xmit_proc_t, net_start_recv_proc_t, u32 );

void net_fd_function_disable (struct usbd_function_instance *);

int net_fd_start_xmit (struct usbd_function_instance *, u8 *, int , void *);

void net_fd_send_int_notification(struct usbd_function_instance *, int connected, int data);

void net_fd_exit(void);
int net_fd_init(char *info_str, char *, char *, BOOL, BOOL, BOOL);
int net_fd_urb_sent_bulk (struct usbd_urb *urb, int urb_rc);

//int net_fd_start_xmit_eem (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data);
int net_fd_start_xmit_mdlm (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data);
//int net_fd_start_xmit_crc (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data);
//int net_fd_recv_urb_cdc(struct usbd_urb *urb, int rc);
int net_fd_recv_urb_mdlm(struct usbd_urb *urb, int rc);
//int net_fd_recv_urb_eem(struct usbd_urb *urb, int rc);

int net_fd_start_recv_mdlm (struct usbd_function_instance *function_instance);
int net_fd_start_recv_eem (struct usbd_function_instance *function_instance);
int net_fd_start_recv_ecm (struct usbd_function_instance *function_instance);

int net_fd_urb_sent_bulk (struct usbd_urb *urb, int urb_rc);
int net_fd_recv_buffer(struct usbd_function_instance *function_instance, u8 *os_buffer, int length,
                void *os_data, int crc_bad, int trim);

BOOL net_fd_check_rarpd_reply(struct usbd_function_instance *function_instance, u8 *buffer, int length);
BOOL net_fd_check_rarpd_request(struct usbd_function_instance *function_instance, u8 *buffer, int length);
BOOL net_fd_check_tp_response(struct usbd_function_instance *function_instance, u8 *buffer, int length);
void net_fd_send_rarpd_reply(struct usbd_function_instance *function_instance);
void net_fd_send_rarpd_request(struct usbd_function_instance *function_instance);
void net_fd_send_tp_request(struct usbd_function_instance *function_instance);

int net_fd_recv_urb(struct usbd_urb *urb, int rc);

/* crc */
/*
 * If the following are defined we implement the crc32_copy routine using
 * Duff's device. This will unroll the copy loop by either 4 or 8. Do not
 * use these without profiling to test if it actually helps on any specific
 * device.
 */
#undef CONFIG_OTG_NETWORK_CRC_DUFF4
#undef CONFIG_OTG_NETWORK_CRC_DUFF8

extern u32 *network_crc32_table;

#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation

#define COMPUTE_FCS(val, c) (((val) >> 8) ^ network_crc32_table[((val) ^ (c)) & 0xff])


/*! crc32_copy
 * Copies a specified number of bytes, computing the 32-bit CRC FCS as it does so.
 *
 * @param dst   Pointer to the destination memory area.
 * @param src   Pointer to the source memory area.
 * @param len   Number of bytes to copy.
 * @param val   Starting value for the CRC FCS.
 *
 * @return      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static u32 __inline__ crc32_copy (u8 *dst, u8 *src, int len, u32 val)
{
        //UNLESS (network_crc32_table)
        //        printk(KERN_INFO"%s:\n", __FUNCTION__);
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = *src++));
        return val;
}

/*! crc32_nocopy
 * Computes the 32-bit CRC FCS across a buffer.
 *
 * @param src   Pointer to the source memory area.
 * @param len   Number of bytes to copy.
 * @param val   Starting value for the CRC FCS.
 *
 * @return      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static u32 __inline__ crc32_nocopy (u8 *src, int len, u32 val)
{
        //UNLESS (network_crc32_table)
        //        printk(KERN_INFO"%s:\n", __FUNCTION__);
        for (; len-- > 0; val = COMPUTE_FCS (val, *src++));
        return val;
}


/*! crc32_pad - pad and calculate crc32
 *
 * @return CRC FCS
 */
static u32 __inline__ crc32_pad (u8 *dst, int len, u32 val)
{
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = '\0'));
        return val;
}



#endif /* NETWORK_FD_H */
