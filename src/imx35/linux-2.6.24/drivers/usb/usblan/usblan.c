/*
 * USB Host to USB Device Network Function Driver
 *
 *      Copyright (c) 2002, 2003 Belcarra
 *      Copyright (c) 2001 Lineo
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>, Bruce Balden <balden@belcarra.com>
 *
 * Some algorithms adopted from usbnet.c:
 *
 *      Copyright (C) 2000-2001 by David Brownell <dbrownell@users.sourceforge.net>
 *
 */

#ifdef MODULE
#include <linux/module.h>
#endif
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
//#include <net/arp.h>
#include <linux/rtnetlink.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/pkt_sched.h>
#include <linux/random.h>
#include <linux/version.h>

#include <linux/skbuff.h>
#include <net/arp.h>
#include <linux/atmdev.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/inetdevice.h>
#include <linux/workqueue.h>



//#include "linux-pch.h"

#include <linux/usb.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include "usblan-compat.h"
#include "usblan.h"

#define PACKED_ENUM enum
#define PACKED_ENUM_EXTRA
#define PACKED1 __attribute__((packed))
#define PACKED2
#define PACKED0


#if defined(USBLAN_LOCAL_CONFIG)
#include "./usblan-config.h"
#endif

#define MIN(a,b) (((a) < (b))?(a):(b))
#define MAX(a,b) (((a) > (b))?(a):(b))

#define UNLESS(x) if (!(x))
#define THROW(x) goto x
#define CATCH(x) while(0) x:
#define THROW_IF(e, x) if (e) { goto x; }
#define THROW_UNLESS(e, x) UNLESS (e) { goto x; }
#define BREAK_IF(x) if (x) { break; }
#define CONTINUE_IF(x) if (x) { continue; }
#define RETURN_IF(x,y) if (y) { return x; }


#define mutex_lock(x)   down(x)
#define mutex_unlock(x) up(x)


#define DRIVER_VERSION "2.0.0"
#define DRIVER_AUTHOR "sl@belcarra.com"
#define DRIVER_DESC "Linux USBLAN driver"

#ifdef MODULE
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

// XXX This needs to be corrected down to the last version of the
// kernel that did NOT have MODULE_LICENSE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,15)
MODULE_LICENSE("GPL");
#endif
#endif

#define STATIC

/* Module Parameters ************************************************************************* */

#define MAX_INTERFACES 1

#define MAX_RCV_SKBS 10
#define TIMEOUT_JIFFIES (4*HZ)
#define MAX_PACKET      32768
#define MIN_PACKET      sizeof(struct ethhdr)


#define TX_QLEN 2
#define RX_QLEN 2

static          DECLARE_MUTEX(usbd_mutex); // lock for global changes
static          LIST_HEAD(usbd_list); // a list for all active devices


typedef enum usbdnet_device_type {
        usbdnet_unknown, usbdnet_basic, usbdnet_cdc, usbdnet_safe, usbdnet_blan, usbdnet_rndis
} usbdnet_device_type_t;

char * usbdnet_device_names[] = {
        "UNKNOWN", "BASIC", "CDC", "SAFE", "BLAN", "RNDIS",
};



__u8 SAFE_VERSION[2] = {
        0x00, 0x01,                                      /* BCD Version */
};
__u8 SAFE_GUID[16] = {
        0x5d, 0x34, 0xcf, 0x66, 0x11, 0x18, 0x11, 0xd6,  /* bGUID */
        0xa2, 0x1a, 0x00, 0x01, 0x02, 0xca, 0x9a, 0x7f,  /* bGUID */
};

__u8 BLAN_VERSION[2] = {
        0x00, 0x01,                                      /* BCD Version */
};
__u8 BLAN_GUID[16] = {
        0x74, 0xf0, 0x3d, 0xbd, 0x1e, 0xc1, 0x44, 0x70,  /* bGUID */
        0xa3, 0x67, 0x71, 0x34, 0xc9, 0xf5, 0x54, 0x37,  /* bGUID */
};

// data detail
#define DATA_CRC        0x01
#define DATA_PADBEFORE  0x02
#define DATA_PADAFTER   0x04
#define DATA_FERMAT     0x08

// cdc notifications
#define CDC_NOTIFICATION                0xa1
#define CDC_NOTIFICATION_NETWORK        0x00
#define CDC_NOTIFICATION_SPEEDCHANGE    0x2a

#define USB_DT_CS_INTERFACE             0x24

#define MDLM_FUNCTIONAL                 0x12
#define MDLM_DETAIL                     0x13

#define MDLM_SAFE_GUID                  0x00
#define MDLM_BLAN_GUID                  0x01


#if 0
STATIC void usblan_test_kalloc(char *msg)
{
        struct urb *foo;
        printk(KERN_INFO "%s: %s\n",__FUNCTION__,msg);
        if (NULL != (foo = USB_ALLOC_URB(0,GFP_ATOMIC))) {
                usb_free_urb(foo);
        }
        printk(KERN_INFO "%s: #%08x\n",__FUNCTION__,(u32)(void*)foo);
}
#endif



/* struct private
 *
 * This structure contains the network interface and additional per USB device information.
 *
 * A pointer to this structure is used in two three places:
 *
 *      net->priv
 *      urb->context
 *      skb->cb.priv
 */
struct private {

        // general
        struct usb_device *usbdev;  // usb core layer provides this for the probed device
        struct semaphore mutex;     // lock for changes to this structure
        struct list_head list;      // to maintain a list of these devices ()
        wait_queue_head_t *wait;
        wait_queue_head_t *ctrl_wait;

        struct tasklet_struct bh;
        struct WORK_STRUCT crc_task;
        struct WORK_STRUCT ctrl_task;
        struct WORK_STRUCT reset_task;
        struct WORK_STRUCT unlink_task;

        int intf_count;
        int intf_max;

        // network
        struct net_device net;
        struct net_device_stats stats;
        unsigned char   dev_addr[ETH_ALEN];

        // queues
        struct sk_buff_head rxq;
        struct sk_buff_head txq;
        struct sk_buff_head unlink;
        struct sk_buff_head done;

        //
        int             crc32;      // append and check for appended 32bit CRC
        int             padded;     // pad bulk transfers such that (urb->transfer_buffer_length % data_ep_out_size) == 1
        int             addr_set;   // set_address
        int             sawCRC;

        int             timeouts;

        usbdnet_device_type_t   usbdnet_device_type;

        //struct usb_interface *data_interface;
        //struct usb_interface *comm_interface;

        struct usb_ctrlrequest  ctrl_request;
        struct urb              *ctrl_urb;
        int             configuration_number;
        int             bConfigurationValue;

        int             comm_interface;
        int             comm_bInterfaceNumber;
        int             comm_bAlternateSetting;
        int             comm_ep_in;
        int             comm_ep_in_size;

        int             data_interface;

        int             data_bInterfaceNumber;
        int             data_bAlternateSetting;

        int             nodata_bInterfaceNumber;
        int             nodata_bAlternateSetting;

        int             data_ep_in;
        int             data_ep_out;
        int             data_ep_in_size;
        int             data_ep_out_size;

        u8              CRCInUse;
        u8              CDC;
        u8              bmNetworkCapabilities;
        u8              bmDataCapabilities;
        u8              bPad;
};

/* struct skb_cb
 *
 * This defines how we use the skb->cb data area. It allows us to get back to the private
 * data structure and track the current state of skb in our done queue. There is a pointer
 * to the active urb so that it can be cancelled (e.g. tx_timeout).
 *
 *      skb->cb
 */
typedef enum skb_state {
        unknown = 0,
        tx_start,                   // an skb in priv->txq
        tx_done,                    // a transmitted skb in priv->done
        rx_start,                   // an skb in priv->rxq
        rx_done,                    // a received skb in priv->done
        rx_cleanup,                 // a received skb being thrown out due to an error condition
} skb_state_t;

struct skb_cb {
        struct private *priv;
        struct urb     *urb;
        skb_state_t     state;
        unsigned long int jiffies;
};


static struct usb_driver usblan_driver;


struct usb_mdlm_detail_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u8      bGuidDescriptorType;
} __attribute__ ((packed));

struct usb_safe_detail_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u8      bGuidDescriptorType;
        u8      bmNetworkCapabilities;
        u8      bmDataCapabilities;
} __attribute__ ((packed));

struct usb_blan_detail_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u8      bGuidDescriptorType;
        u8      bmNetworkCapabilities;
        u8      bmDataCapabilities;
        u8      bPad;
} __attribute__ ((packed));

struct usb_class_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u8      data[0];
} __attribute__ ((packed));

struct usb_guid_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u8      bGuidDescriptorType;
        u8      data[0];
} __attribute__ ((packed));

struct usb_notification_descriptor {
        u8      bmRequestType;
        u8      bNotification;
        u16     wValue;
        u16     wIndex;
        u16     wLength;
        u8      data[2];
} __attribute__ ((packed));

struct usb_speedchange_descriptor {
        u8      bmRequestType;
        u8      bNotification;
        u16     wValue;
        u16     wIndex;
        u16     wLength;
        u32     data[2];
} __attribute__ ((packed));

struct usb_mdlm_functional_descriptor {
        u8      bLength;
        u8      bDescriptorType;
        u8      bDescriptorSubType;
        u16     bcdVersion;
        u8      bGuid[16];
} __attribute__ ((packed));



/*
 * default MAC address to use
 */
static unsigned char default_addr[ETH_ALEN];

/* Module Parameters ************************************************************************* */

#define VENDOR_SPECIFIC_CLASS                   0xff
#define VENDOR_SPECIFIC_SUBCLASS                0xff
#define VENDOR_SPECIFIC_PROTOCOL                0xff

#define MTU                                     1500+100


#if defined(CONFIG_USBD_USBLAN_VENDOR) && !defined(CONFIG_USBD_USBLAN_PRODUCT)
#abort "USBLAN_VENDOR defined without USBLAN_PRODUCT"
#endif

#define CDC_DEVICE_CLASS                        0x02 // Device descriptor Class

#define CDC_INTERFACE_CLASS                     0x02 // CDC interface descriptor Class
#define CDC_INTERFACE_SUBCLASS                  0x06 // CDC interface descriptor SubClass
#define RNDIS_INTERFACE_SUBCLASS                0x02 // CDC interface descriptor SubClass
#define MDLM_INTERFACE_SUBCLASS                 0x0a // CDC interface descriptor SubClass

#define DATA_INTERFACE_CLASS                    0x0a // Data interface descriptor Class

#define DEFAULT_PROTOCOL                        0x00
#define VENDOR_SPECIFIC_PROTOCOL                0xff

#define LINEO_INTERFACE_CLASS                   0xff // Lineo private interface descriptor Class

#define LINEO_INTERFACE_SUBCLASS_SAFENET        0x01 // Lineo private interface descriptor SubClass
#define LINEO_INTERFACE_SUBCLASS_SAFESERIAL     0x02

#define LINEO_SAFENET_CRC                       0x01 // Lineo private interface descriptor Protocol
#define LINEO_SAFENET_CRC_PADDED                0x02 // Lineo private interface descriptor Protocol

#define LINEO_SAFESERIAL_CRC                    0x01
#define LINEO_SAFESERIAL_CRC_PADDED             0x02


#define NETWORK_ADDR_HOST       0xac100005      /* 172.16.0.0 */
#define NETWORK_ADDR_CLIENT     0xac100006      /* 172.16.0.0 */
#define NETWORK_MASK            0xfffffffc


static __u32    vendor_id;         // no default
static __u32    product_id;        // no default
static __u32    class = LINEO_INTERFACE_CLASS;
static __u32    subclass = LINEO_INTERFACE_SUBCLASS_SAFENET;

static __u32    echo_fcs;       // no default
static __u32    noisy_fcs;      // no default
static __u32    echo_rx;        // no default
static __u32    echo_tx;        // no default

#ifdef MODULE
MODULE_PARM_DESC(vendor_id, "User specified USB idVendor");
MODULE_PARM_DESC(product_id, "User specified USB idProduct");
MODULE_PARM_DESC(class, "User specified USB Class");
MODULE_PARM_DESC(subclass, "User specified USB SubClass");
MODULE_PARM(vendor_id, "i");
MODULE_PARM(product_id, "i");
MODULE_PARM(class, "i");
MODULE_PARM(subclass, "i");

MODULE_PARM_DESC(echo_tx, "echo TX urbs");
MODULE_PARM_DESC(echo_rx, "echo RCV urbs");
MODULE_PARM_DESC(echo_fcs, "BAD FCS");
MODULE_PARM_DESC(noisy_fcs, "BAD FCS info");
MODULE_PARM(echo_tx, "i");
MODULE_PARM(echo_rx, "i");
MODULE_PARM(echo_fcs, "i");
MODULE_PARM(noisy_fcs, "i");
#endif

//match_flags: DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_CLASS ,

#define MY_USB_DEVICE(vend,prod) \
match_flags: USB_DEVICE_ID_MATCH_DEVICE , \
idVendor: (vend), \
idProduct: (prod),\
bDeviceClass: (CDC_INTERFACE_CLASS),

static __devinitdata struct usb_device_id id_table[] = {

        // RNDIS devices Vend   Prod    bDeviceClass      bInterfaceClass      bInterfaceSubClass

        {MY_USB_DEVICE(0x15ec, 0xf001)},                // Belcarra Network Demo
        {MY_USB_DEVICE(0x12b9, 0xf001)},                // Belcarra Network Demo


#if 1
#if defined(CONFIG_USB_USBLAN_VENDORID) && defined(CONFIG_USB_USBLAN_PRODUCTID)
        // A configured driver
        {MY_USB_DEVICE(CONFIG_USB_USBLAN_VENDORID, CONFIG_USB_USBLAN_PRODUCTID)},
#endif
#endif

        // extra null entry for module vendor_id/produc parameters and terminating entry
        {}, {},
};


#ifdef MODULE
MODULE_DEVICE_TABLE(usb, id_table);
#endif

#define ECHO_FCS
#define ECHO_RCV

#undef ECHO_TX_SKB
#define ECHO_TX_URB

__u32           crc32_table[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
        0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
        0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
        0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
        0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
        0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
        0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
        0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
        0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
        0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define CRC32_INITFCS     0xffffffff             // Initial FCS value
#define CRC32_GOODFCS     0xdebb20e3             // Good final FCS value

#define CRC32_FCS(fcs, c) (((fcs) >> 8) ^ crc32_table[((fcs) ^ (c)) & 0xff])

/* fcs_memcpy32 - memcpy and calculate fcs
 * Perform a memcpy and calculate fcs using ppp 32bit CRC algorithm.
 */
static __u32 __inline__
fcs_memcpy32(unsigned char *dp, unsigned char *sp, int len, __u32 fcs)
{
        for (; len-- > 0; fcs = CRC32_FCS(fcs, *dp++ = *sp++));
        return fcs;
}

/* fcs_pad32 - pad and calculate fcs
 * Pad and calculate fcs using ppp 32bit CRC algorithm.
 */
static __u32 __inline__
fcs_pad32(unsigned char *dp, int len, __u32 fcs)
{
        for (; len-- > 0; fcs = CRC32_FCS(fcs, *dp++ = '\0'));
        return fcs;
}

/* fcs_compute32 - memcpy and calculate fcs
 * Perform a memcpy and calculate fcs using ppp 32bit CRC algorithm.
 */
static __u32 __inline__
fcs_compute32(unsigned char *sp, int len, __u32 fcs)
{
        for (; len-- > 0; fcs = CRC32_FCS(fcs, *sp++));
        return fcs;
}

void
wait_for_sync(struct WORK_STRUCT *tq)
{

        // wait for pending bottom halfs to exit
        while (PENDING_WORK_ITEM((*tq))){
                SCHEDULE_TIMEOUT(1);
        }

        tq->data = 0;

        while (PENDING_WORK_ITEM((*tq))) {
                SCHEDULE_TIMEOUT(1);
        }
}


#define RETRYTIME       2

void
skb_bad_crc(struct sk_buff *skb, __u32 fcs)
{
#ifdef ECHO_FCS
        if (noisy_fcs) {
                printk(KERN_INFO"%s: BAD FCS len: %4d crc: %08x last: %02x %02x %02x %02x\n", __FUNCTION__,
                                skb->len, fcs, skb->data[skb->len - 4],
                                skb->data[skb->len - 3], skb->data[skb->len - 2], skb->data[skb->len - 1]
                      );
        }
        if (echo_fcs) {
                int             i;
                unsigned char  *cp = skb->data;
                printk(KERN_INFO "%s: FAILED skb: %p head: %p data: %p tail: %p len: %d", __FUNCTION__,
                                skb, skb->head, skb->data, skb->tail, skb->len);
                for (i = 0; i < skb->len; i++) {
                        if ((i % 32) == 0) {
                                printk("\nrcv[%02x]: ", i);
                        }
                        printk("%02x ", cp[i]);
                }
                printk("\n");
        }
#endif
}

#if 0
static void
dump_skb(struct sk_buff *skb, char *msg)
{
        int             i;
        unsigned char  *cp = skb->data;

        printk(KERN_INFO "\n%s", msg);
        for (i = 0; i < skb->len; i++) {
                if (!(i % 32)) {
                        printk("\n[%02x] ", i);
                }
                printk("%02x ", cp[i]);
        }
        printk("\n");
}
#endif

/* ********************************************************************************************* */
#if defined(LONG_STRING_OF_ZEROES_HACK)

/* fermat
 *
 * This is a hack designed to help some broken hardware that cannot successfully
 * transmit long strings of zero's without causing the host port to signal a
 * status change and drop the connection.
 *
 */

typedef unsigned char BYTE;
typedef struct fermat {
        int length;
        BYTE power[256];
} FERMAT;

STATIC void fermat_init(void);
STATIC void fermat_encode(BYTE *data, int length);
STATIC void fermat_decode(BYTE *data, int length);

STATIC int fermat_setup(FERMAT *p, int seed){
        int i = 0;
        unsigned long x,y;
        y = 1;
        do{
                x = y;
                p->power[i] = ( x == 256 ? 0 : x);
                y = ( seed * x ) % 257;
                i += 1;
        }while( y != 1);
        p->length = i;
        return i;
}

STATIC void fermat_xform(FERMAT *p, BYTE *data, int length){
        BYTE *pw = p->power;
        int   i, j;
        BYTE * q ;
        for(i = 0, j=0, q = data; i < length; i++, j++, q++){
                if(j>=p->length){
                        j = 0;
                }
                *q ^= pw[j];
        }
}

static FERMAT default_fermat;
static const int primitive_root = 5;
STATIC void fermat_init(){
        (void) fermat_setup(&default_fermat, primitive_root);
}

// Here are the public official versions.
// Change the primitive_root above to another primitive root
// if you need better scatter. Possible values are 3 and 7


STATIC void fermat_encode(BYTE *data, int length){
        fermat_xform(&default_fermat, data, length);
}

STATIC void fermat_decode(BYTE *data, int length){
        fermat_xform(&default_fermat, data, length);
}
#endif


/* ********************************************************************************************* */

STATIC void     defer_skb(struct private *priv, struct sk_buff *skb, struct skb_cb *cb,
			  skb_state_t state, struct sk_buff_head *list);
STATIC int      unlink_urbs(struct sk_buff_head *q);
STATIC void     urb_tx_complete(struct urb *urb);
STATIC void     urb_rx_complete(struct urb *urb);
#if 0
STATIC void     urb_dead_complete(struct urb *urb);
#endif


/* Network Configuration *********************************************************************** */

/* sock_ioctl - perform an ioctl call to inet device
 */
static int sock_ioctl(u32 cmd, struct ifreq *ifreq)
{
        int rc = 0;
        mm_segment_t save_get_fs = get_fs();
        //printk(KERN_INFO"%s: cmd: %x\n", __FUNCTION__, cmd);
        set_fs(get_ds());
        rc = devinet_ioctl(cmd, ifreq);
        set_fs(save_get_fs);
        return rc;
}

/* sock_addr - setup a socket address for specified interface
 */
static int sock_addr(char * ifname, u32 cmd, u32 s_addr)
{
        struct ifreq ifreq;
        struct sockaddr_in *sin = (void *) &(ifreq.ifr_ifru.ifru_addr);

        //printk(KERN_INFO"%s: ifname: %s addr: %x\n", __FUNCTION__, ifname, ntohl(s_addr));

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname);

        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = s_addr;

        return sock_ioctl(cmd, &ifreq);
}


/* sock_flags - set flags for specified interface
 */
static int sock_flags(char * ifname, u16 oflags, u16 sflags, u16 rflags)
{
        int rc = 0;
        struct ifreq ifreq;

        //printk(KERN_INFO"%s: ifname: %s oflags: %x s_flags: %x r_flags: %x\n", __FUNCTION__, ifname, oflags, sflags, rflags);

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname);

        oflags |= sflags;
        oflags &= ~rflags;
        ifreq.ifr_flags = oflags;

        //printk(KERN_INFO"%s: -> ifr_flags: %x \n", __FUNCTION__, ifreq.ifr_flags);

        THROW_IF ((rc = sock_ioctl(SIOCSIFFLAGS, &ifreq)), error);

        //printk(KERN_INFO"%s: <- ifr_flags: %x \n", __FUNCTION__, ifreq.ifr_flags);

        CATCH(error) {
                printk(KERN_INFO"%s: ifconfig: cannot get/set interface flags (%d)\n", __FUNCTION__, rc);
                return rc;
        }
        return rc;
}

/* network_attach - configure interface
 *
 * This will use socket calls to configure the interface to the supplied
 * ip address and make it active.
 */
STATIC int network_attach(struct net_device *net, u32 host_ip, u32 mask, int attach)
{
        int err = 0;

        //printk(KERN_INFO"%s: net: %p host_ip: %08x mask: %08x attach: %d\n", __FUNCTION__, net, host_ip, mask, attach);
        if (attach) {
                u16 oflags = net ? net->flags : 0;

                /* setup host_ip address, netwask, and broadcast address */
                if (host_ip) {
                        THROW_IF ((err = sock_addr(net->name, SIOCSIFADDR, htonl(host_ip))), error);
                        if (mask) {
                                THROW_IF ((err = sock_addr(net->name, SIOCSIFNETMASK, htonl(mask))), error);
                                THROW_IF ((err = sock_addr(net->name, SIOCSIFBRDADDR, htonl(host_ip | ~mask))), error);
                        }
                        /* bring the interface up */
                        THROW_IF ((err = sock_flags(net->name, oflags, IFF_UP, 0)), error);
                }


        }
        else {
                u16 oflags = net ? net->flags : 0;
                /* bring the interface down */
                THROW_IF ((err = sock_flags(net->name, oflags, 0, IFF_UP)), error);
        }

        CATCH(error) {
                printk(KERN_INFO"%s: ifconfig: cannot configure interface (%d)\n", __FUNCTION__, err);
                return err;
        }
        return 0;
}


/* ********************************************************************************************* */

/* Network Support Functions - these are called by the network layer *************************** */

/* net_get_stats - network device get stats function
 * Retreive network device stats structure.
 */
STATIC struct net_device_stats *
net_get_stats(struct net_device *net)
{
        struct private *priv = (struct private *) net->priv;
        //printk(KERN_INFO "%s:\n", __FUNCTION__);
        return &priv->stats;
}

/* net_set_mac_addr - network device set mac address function
 */
STATIC int
net_set_mac_address(struct net_device *net, void *p)
{
        struct private *priv = (struct private *) net->priv;
        struct sockaddr *addr = p;

        //printk(KERN_INFO "%s:\n", __FUNCTION__);
        if (netif_running(net)) {
                return -EBUSY;
        }
        memcpy(net->dev_addr, addr->sa_data, net->addr_len);
        priv->addr_set = 1;
        return 0;
}

/* net_change_mtu - network device set config function
 * Set MTU, if running we can only change it to something less
 * than or equal to MTU when PVC opened.
 */
STATIC int
net_change_mtu(struct net_device *net, int mtu)
{
        //printk(KERN_INFO "%s:\n", __FUNCTION__);
        if ((mtu < sizeof(struct ethhdr)) || (mtu > (MAX_PACKET - 4))) {
                return -EINVAL;
        }
        if (netif_running(net)) {
                if (mtu > net->mtu) {
                        return -EBUSY;
                }
        }
        net->mtu = mtu;
        return 0;
}

/* net_open - called by network layer to open network interface
 */
STATIC int
net_open(struct net_device *net)
{
        struct private *priv = (struct private *) net->priv;

        //printk(KERN_INFO "%s: priv#%08x net#%08x\n",__FUNCTION__,(u32)(void*)priv,(u32)(void*)net);
        mutex_lock(&priv->mutex);
        //printk(KERN_INFO "%s: AAA usbdev#%08x dataIF=%d alt=%d\n",__FUNCTION__,(u32)(void*)priv->usbdev, priv->data_bInterfaceNumber, priv->data_bAlternateSetting);

        // enable traffic
        //printk(KERN_INFO"%s: setting data interface bInterfaceNumber: %d bAlternateSetting: %d\n", __FUNCTION__,
        //                priv->data_bInterfaceNumber, priv->data_bAlternateSetting);

#if defined(LINUX24)
        if (usb_set_interface( priv->usbdev, priv->data_bInterfaceNumber, priv->data_bAlternateSetting)) {
                err("usb_set_interface() failed");
        }
#endif

        // XXX find interface ip / netmask, if netmask is 255.255.255.252
        // then send ip and ip+1 to device as suggested addresses




        //printk(KERN_INFO "%s: BBB\n",__FUNCTION__);

        // tell the network layer to enable transmit queue
        netif_start_queue(net);
        //printk(KERN_INFO "%s: BBB\n",__FUNCTION__);

        // call the bottom half to schedule some receive urbs
        tasklet_schedule(&priv->bh);
        //printk(KERN_INFO "%s: CCC\n",__FUNCTION__);

        mutex_unlock(&priv->mutex);
        //printk(KERN_INFO "%s: DDD\n",__FUNCTION__);
        return 0;
}

/* net_stop - called by network layer to stop network interface
 */
STATIC int
net_stop(struct net_device *net)
{
        struct private *priv = (struct private *) net->priv;

        DECLARE_WAIT_QUEUE_HEAD(unlink_wakeup);
        DECLARE_WAITQUEUE(wait, current);

        //printk(KERN_INFO "%s:\n",__FUNCTION__);

        mutex_lock(&priv->mutex);

        // tell the network layer to disable the transmit queue
        netif_stop_queue(net);


        // disable (if possible) network traffic
        if (priv->nodata_bInterfaceNumber >= 0)  {

                //printk(KERN_INFO"%s: setting nodata interface bInterfaceNumber: %d bAlternateSetting: %d\n", __FUNCTION__,
                //                priv->nodata_bInterfaceNumber, priv->nodata_bAlternateSetting);

                if (usb_set_interface( priv->usbdev, priv->nodata_bInterfaceNumber, priv->nodata_bAlternateSetting)) {
                        err("usb_set_interface() failed");
                }
        }


        // setup a wait queue - this also acts as a flag to prevent bottom half from allocating more urbs
        add_wait_queue(&unlink_wakeup, &wait);
        priv->wait = &unlink_wakeup;

        // move the tx and rx urbs into the done queue
        unlink_urbs(&priv->txq);
        unlink_urbs(&priv->rxq);

        // wait for done queue to empty
        while (skb_queue_len(&priv->rxq) || skb_queue_len(&priv->txq) || skb_queue_len(&priv->done)) {
                current->state = TASK_UNINTERRUPTIBLE;
                schedule_timeout(TIMEOUT_JIFFIES * 1000);
        }
        priv->wait = 0;

        // cleanup
        remove_wait_queue(&unlink_wakeup, &wait);

        mutex_unlock(&priv->mutex);
        return 0;
}


/* net_tx_timeout - called by network layer to cancel outstanding skbs
 */
STATIC void
net_tx_timeout(struct net_device *net)
{
        //struct private *priv = (struct private *) net->priv;
        //unsigned char  *cp;
        //struct urb     *urb;

        //unsigned char   deadbeef[] = "DEADBEEF";

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //unlink_urbs(&priv->txq);
        //tasklet_schedule(&priv->bh);

        //if (priv->unlink_task.sync == 0) {
        //        schedule_task(&priv->unlink_task);
        //}

        return;
#if 0
        // Attempt to send a short usb packet to the device, this will ensure that
        // any partially completed bulk transfer will be terminated.

        if (!(cp = kmalloc(sizeof(deadbeef), GFP_KERNEL))) {
                return;
        }
        if (!(urb = USB_ALLOC_URB(0,GFP_ATOMIC))) {
                kfree(cp);
                return;
        }
        memcpy(cp, deadbeef, sizeof(deadbeef));

        FILL_BULK_URB(urb, priv->usbdev, usb_sndbulkpipe(priv->usbdev, priv->data_ep_out),
                        cp, sizeof(deadbeef), urb_dead_complete, NULL);

        urb->transfer_flags = USB_QUEUE_BULK | USB_ASYNC_UNLINK | USB_NO_FSBR;

        //usb_endpoint_running(priv->usbdev, usb_pipeendpoint(priv->data_ep_out), usb_pipeout(priv->data_ep_out));
        //usb_settoggle(priv->usbdev, usb_pipeendpoint(priv->data_ep_out), usb_pipeout(priv->data_ep_out), 0);

        if (usb_submit_urb(urb)) {
                kfree(cp);
                urb->transfer_buffer = NULL;
                usb_free_urb(urb);
        }
#endif
}

/* net_hard_start_xmit - called by network layer to transmit skb
 */
STATIC int
net_hard_start_xmit(struct sk_buff *skb, struct net_device *net)
{
        struct private *priv = (struct private *) net->priv;
        struct urb     *urb;
        struct skb_cb  *cb;
        struct sk_buff *skb2;
        int             flags = in_interrupt()? GFP_ATOMIC : GFP_KERNEL;
        __u32           fcs;
        int             length;
        int             pad;

        //printk(KERN_INFO "%s:\n",__FUNCTION__);

        echo_tx = 0;
        // debug
        if (echo_tx) {
                int             i;
                unsigned char  *cp = skb->data;
                printk(KERN_INFO"%s: skb: %p %d\n", __FUNCTION__, skb, skb->len);
                for (i = 0; i < skb->len; i++) {
                        if ((i % 32) == 0) {
                                printk("\ntx[%3x]: ", i);
                        }
                        printk("%02x ", cp[i]);
                }
                printk("\n");
        }

        // allocate urb
        THROW_IF (priv->wait || !(urb = USB_ALLOC_URB(0,flags)), free_skb_only);

        // calculate length required
        if (priv->bmDataCapabilities & (DATA_PADBEFORE | DATA_PADAFTER)) {
                // we need to pad so that after appending the CRC we have a multiple of packetsize
                length = priv->data_ep_out_size *  (((skb->len + 4 + 1) / priv->data_ep_out_size) + 1);
        }
        else {
                // require a minimum of one full packet
                length = MAX(priv->data_ep_out_size, skb->len + 4 + 1);
        }

        // allocate a new skb, copy data to it computing FCS,
        // the extra bytes are for the CRC and optional pad byte
        THROW_IF (!(skb2 = alloc_skb(length + 4, flags)), free_urb_and_skb); // XXX +4 ?

        if (priv->bmDataCapabilities & DATA_CRC) {

                fcs = fcs_memcpy32(skb_put(skb2, skb->len), skb->data, skb->len, CRC32_INITFCS);

                //dump_skb(skb, "skb");
                dev_kfree_skb_any(skb);
                skb = skb2;

                if (priv->bmDataCapabilities & DATA_PADBEFORE) {
                        if ((pad = (length - skb->len - 4)) > 0) {
                                // pad to required length less four (CRC), copy fcs and append pad byte if required
                                fcs = fcs_pad32(skb_put(skb, pad), pad, fcs);
                        }
                }

                fcs = ~fcs;
                *skb_put(skb, 1) = fcs & 0xff;
                *skb_put(skb, 1) = (fcs >> 8) & 0xff;
                *skb_put(skb, 1) = (fcs >> 16) & 0xff;
                *skb_put(skb, 1) = (fcs >> 24) & 0xff;

                if (priv->bmDataCapabilities & DATA_PADAFTER) {
                        while ((skb->len % priv->bPad) || !(skb->len % priv->data_ep_out_size)) skb_put(skb, 1);
                }

                // append a byte if required, we overallocated by one to allow for this
                else if (!(skb->len % priv->data_ep_out_size)) {
                        *skb_put(skb, 1) = 0;
                }

                //dump_skb(skb, "skb with CRC");

        }
        else {
                memcpy(skb_put(skb2, skb->len), skb->data, skb->len);

                //dump_skb(skb, "skb");
                dev_kfree_skb_any(skb);
                skb = skb2;

                if (!(skb->len % priv->data_ep_out_size)) {
                        *skb_put(skb, 1) = 0;
                }
        }

        // hand urb off to usb layer

        cb = (struct skb_cb *) skb->cb;
        cb->urb = urb;
        cb->priv = priv;
        cb->state = tx_start;
        cb->jiffies = jiffies;

        // urb->context ends up with pointer to skb
        FILL_BULK_URB(urb, priv->usbdev, usb_sndbulkpipe(priv->usbdev, priv->data_ep_out), skb->data, skb->len,
                        urb_tx_complete, skb);

        urb->transfer_flags = USB_QUEUE_BULK | USB_ASYNC_UNLINK | USB_NO_FSBR;
#if defined(LINUX24)
        urb->timeout = 2;
#endif /* XXX FIX ME  */


        // submit the urb and restart (or not) the network device queue
        //printk(KERN_INFO"%s: skb: %p %d urb %p len: %d\n", __FUNCTION__, skb, skb->len, urb, urb->transfer_buffer_length);

        if (USB_SUBMIT_URB(urb)) {
                netif_start_queue(net);
                priv->stats.tx_dropped++;
                usb_free_urb(urb);
                THROW(free_urb_and_skb);
        }
        skb_queue_tail(&priv->txq, skb);
        if (priv->txq.qlen < TX_QLEN) {
                netif_start_queue(net);
        }
        else {
                net->trans_start = jiffies;
        }
        CATCH(free_skb_only) {
                CATCH(free_urb_and_skb) {
                        usb_free_urb(urb);
                }
                dev_kfree_skb_any(skb);
                return NET_XMIT_DROP;
        }
        return NET_XMIT_SUCCESS;
}


/* Receive Related ***************************************************************************** */

/* rx_submit - queue an urb to receive data
 */
STATIC void
rx_submit(struct private *priv, struct urb *urb, int gpf)
{
        struct sk_buff *skb;
        struct skb_cb  *cb;
        unsigned long   flags;
        int             size;

        //printk(KERN_INFO "%s:\n",__FUNCTION__);

        //usblan_test_kalloc("AAA");
        size = ((priv->net.mtu + 14 + 4 + priv->data_ep_in_size) / priv->data_ep_in_size) * priv->data_ep_in_size;

        //printk(KERN_INFO"%s: ep_in: %d ep_in_size: %d size: %d urb#%08x\n", __FUNCTION__,
        //      priv->data_ep_in, priv->data_ep_in_size, size, (u32)(void*)urb);

        //TBR-32-bit if (!(skb = alloc_skb(size + 2, gpf))) {
        if (!(skb = alloc_skb(size + 4, gpf))) {
                //printk(KERN_INFO "%s: alloc_skb() failed.\n",__FUNCTION__);
                dbg("no rx skb");
                tasklet_schedule(&priv->bh);
                THROW(free_urb_only);
                return;
        }
        //TBR-32-bit skb_reserve(skb, 2);
        skb_reserve(skb, 4);
        //usblan_test_kalloc("BBB");

        cb = (struct skb_cb *) skb->cb;
        cb->priv = priv;
        cb->urb = urb;
        cb->state = rx_start;
        //usblan_test_kalloc("CCC");

        //printk(KERN_INFO"%s: AAA\n", __FUNCTION__);
        // urb->context ends up with pointer to skb
        FILL_BULK_URB(urb, priv->usbdev, usb_rcvbulkpipe(priv->usbdev, priv->data_ep_in), skb->data, size, urb_rx_complete, skb);
        //usblan_test_kalloc("DDD");

        urb->transfer_flags |= USB_QUEUE_BULK;

        //printk(KERN_INFO"%s: BBB\n", __FUNCTION__);
        spin_lock_irqsave(&priv->rxq.lock, flags);
        //printk(KERN_INFO"%s: CCC\n", __FUNCTION__);
        if (netif_running(&priv->net)) {

                //printk(KERN_INFO"%s: DDD urb %p\n", __FUNCTION__, urb);
                //usblan_test_kalloc("EEE");

                //printk(KERN_INFO "%s: starting URB #%08x\n",__FUNCTION__,(u32)(void*)urb);

                spin_unlock_irqrestore(&priv->rxq.lock, flags);
                if (USB_SUBMIT_URB(urb)) {
                        //printk(KERN_INFO "%s: submit failed freeing URB: %p\n", __FUNCTION__, urb);
                        //usblan_test_kalloc("FFF");
                        tasklet_schedule(&priv->bh);
                        THROW(free_urb_and_skb);
                }
                else {
                        //printk(KERN_INFO"%s: FFF skb %p\n", __FUNCTION__, skb);
                        __skb_queue_tail(&priv->rxq, skb);
                }
                //printk(KERN_INFO"%s: GGG urb %p\n", __FUNCTION__, urb);
                //usblan_test_kalloc("GGG");
        }
        else {
                spin_unlock_irqrestore(&priv->rxq.lock, flags);
                //printk(KERN_INFO "%s: network stopped, freeing urb: %p\n", __FUNCTION__, urb);
                THROW(free_urb_and_skb);
        }

        CATCH(free_urb_only) {

                CATCH(free_urb_and_skb) {
                        printk(KERN_INFO"%s: error freeing skb %p\n", __FUNCTION__, skb);
                        dev_kfree_skb_any(skb);
                }
                printk(KERN_INFO"%s: error freeing urb %p\n", __FUNCTION__, urb);
                usb_free_urb(urb);
        }
}

/* urb_rx_complete - called by usb core layer when urb has been received
 */
STATIC void
urb_rx_complete(struct urb *urb)
{
        struct sk_buff *skb;
        struct skb_cb  *cb;
        struct private *priv;

        //printk(KERN_INFO "%s: urb %p len: %d status=%d\n", __FUNCTION__, urb, urb->actual_length,urb->status);

        if (!(skb = (struct sk_buff *) urb->context)) {
                printk(KERN_ERR "%s: skb NULL\n", __FUNCTION__);
        }
        if (!(cb = (struct skb_cb *) skb->cb)) {
                printk(KERN_ERR "%s: cb NULL\n", __FUNCTION__);
        }
        if (!(priv = cb->priv)) {
                printk(KERN_ERR "%s: priv NULL\n", __FUNCTION__);
        }


        switch (urb->status) {
        case 0:
                priv->timeouts = 0;

                if ((MIN_PACKET < urb->actual_length) && (urb->actual_length < MAX_PACKET)) {
                        cb->urb = NULL;
                        skb_put(skb, urb->actual_length);
                        defer_skb(priv, skb, cb, rx_done, &priv->rxq);
                        if (netif_running(&priv->net)) {
                                rx_submit(priv, urb, GFP_ATOMIC);
                        }
                        break;
                }

                /* FALLTHROUGH */

        case -EOVERFLOW:
                priv->stats.rx_over_errors++;
        case -EILSEQ:
        case -ECONNABORTED:
        case -ETIMEDOUT:
                priv->timeouts++;
                priv->stats.rx_dropped++;
                //printk(KERN_INFO"%s: RX_CLEANUP urb->status: %d timeout: %d\n", __FUNCTION__, urb->status, priv->timeouts);
                defer_skb(priv, skb, cb, rx_cleanup, &priv->rxq);

                // XXX provisional, this will attempt to force a reset for a device that
                // there have been multiple receive timeouts. This is a host
                // that is no longer responding to IN with a NAK. Typically this is
                // due to a device that has stopped operation without dropping the
                // usb control resistor to tell us.

                if (priv->timeouts > 20) {
                        priv->timeouts = 0;
                        //printk(KERN_INFO "%s: scheduling reset task\n", __FUNCTION__);

#if 0
                        if (priv->reset_task.sync == 0) {
                                schedule_task(&priv->reset_task);
#else
	                if(!PENDING_WORK_ITEM(priv->reset_task)){
                                SCHEDULE_WORK(priv->reset_task);
#endif
                        }
                }
                break;
        }
}


/* bh_rx_process - called by bottom half to process received skb
 */
STATIC inline void
bh_rx_process(struct private *priv, struct sk_buff *skb)
{
        __u32           fcs;

#if 0
        if (skb->len > (priv->net.mtu + 16 + 4 + 1 + 100)) {
                printk(KERN_INFO "%s: URB too large\n", __FUNCTION__);
                priv->stats.rx_length_errors++;
                THROW(crc_error);
                dev_kfree_skb(skb);
                priv->stats.rx_errors++;
                return;
        }
#endif
#if 0
        if (priv->bmDataCapabilities & DATA_FERMAT) {
                fermat_decode(skb->data, skb->len);
        }
#endif
        //printk(KERN_INFO "%s:\n",__FUNCTION__);
        if (priv->bmDataCapabilities & DATA_CRC) {

                // check if we need to check for extra byte
                if ((skb->len % priv->data_ep_in_size) == 1) {

                        // check fcs across length minus one bytes
                        if ((fcs = fcs_compute32(skb->data, skb->len - 1, CRC32_INITFCS)) == CRC32_GOODFCS) {
                                // success, trim extra byte and fall through
                                skb_trim(skb, skb->len - 1);

                                priv->sawCRC = 1;
                        }
                        // failed, check additional byte
                        else if ((fcs = fcs_compute32(skb->data + skb->len - 1, 1, fcs)) != CRC32_GOODFCS) {
                                // failed
                                printk(KERN_INFO "%s: CRC fail on extra byte\n", __FUNCTION__);
                                THROW_IF(priv->sawCRC, crc_error);
                                return;
                        }
                        // success fall through, possibly with corrected length
                }
                // normal check across full frame
                else if ((fcs = fcs_compute32(skb->data, skb->len, CRC32_INITFCS)) != CRC32_GOODFCS) {
                        printk(KERN_INFO "%s: CRC fail len: %d\n", __FUNCTION__, skb->len);
                        THROW_IF(priv->sawCRC, crc_error);
                        return;
                }

                // trim fcs
                skb_trim(skb, skb->len - 4);
        }

        // debug
        echo_rx = 0;
        if (echo_rx) {
                int             i;
                unsigned char  *cp = skb->data;

                for (i = 0; i < skb->len; i++) {
                        if ((i % 32) == 0) {
                                printk("\nrx[%3x]: ", i);
                        }
                        printk("%02x ", cp[i]);
                }
                printk("\n");
        }

        // push the skb up
        memset(skb->cb, 0, sizeof(struct skb_cb));
        skb->dev = &priv->net;
        skb->protocol = eth_type_trans(skb, &priv->net);
        skb->pkt_type = PACKET_HOST;
        skb->ip_summed = CHECKSUM_UNNECESSARY;
        priv->stats.rx_packets++;
        priv->stats.rx_bytes += skb->len;
        if (netif_rx(skb)) {
                printk(KERN_INFO "%s: submitting skb failed\n", __FUNCTION__);
        }

        CATCH(crc_error) {
                dev_kfree_skb_any(skb);
                priv->stats.rx_errors++;
        }
}


/* Transmit Related **************************************************************************** */

/* This is a version of usb_clear_halt() that doesn't read the status from
 * the device -- this is because some devices crash their internal firmware
 * when the status is requested after a halt
 */
STATIC int
local_clear_halt(struct usb_device *dev, int pipe)
{
        int             result;
        int             endp = usb_pipeendpoint(pipe) | (usb_pipein(pipe) << 7);

        if ((result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                                        USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, endp, NULL, 0, HZ * 3)))
        {
                return result;
        }

        // reset the toggles and endpoint flags
        // usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));
        // usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);

        return 0;
}

/* ctrl_task - called as kernel task to send clear halt message
 */
STATIC void
ctrl_task(void *data)
{
        struct private *priv = (struct private *) data;

        local_clear_halt(priv->usbdev, usb_sndbulkpipe(priv->usbdev, priv->data_ep_out));
        netif_wake_queue(&priv->net);
}

/* reset_task - called as kernel task to send reset the device
 */
STATIC void
reset_task(void *data)
{
        struct private *priv = (struct private *) data;

        if (usb_reset_device(priv->usbdev)) {
                printk(KERN_INFO "%s: reset failed\n", __FUNCTION__);
        }
}

/* urb_tx_complete - called by usb core layer when network skb urb has been transmitted
 */
STATIC void
urb_tx_complete(struct urb *urb)
{
        struct sk_buff *skb;

        //printk(KERN_INFO"%s: urb: %p\n", __FUNCTION__, urb);

        if (urb->status) {
                printk(KERN_INFO "%s: urb: %p status: %d\n", __FUNCTION__, urb, urb->status);
        }

        urb->dev = 0;

        if ((skb = urb->context)) {
                struct skb_cb  *cb;
                struct private *priv;

                if (!(cb = (struct skb_cb *) skb->cb)) {
                        printk(KERN_ERR "%s: cb NULL skb: %p\n", __FUNCTION__, skb);
                }
                if (!(priv = cb->priv)) {
                        printk(KERN_ERR "%s: priv NULL skb: %p cb: %p\n", __FUNCTION__, skb, cb);
                }

                if (!cb->urb) {
                        printk(KERN_ERR "%s: urb NULL skb: %p cb: %p\n", __FUNCTION__, skb, cb);
                }

                if (cb->urb != urb) {
                        printk(KERN_ERR "%s: urb not urb skb: %p cb: %p cb->urb: %p urb: %p\n", __FUNCTION__,
                                        skb, cb, cb->urb, urb);
                }

                if (urb->status == USB_ST_STALL) {
                        printk(KERN_ERR "%s: USB_ST_STALL\n", __FUNCTION__);
#if defined(LINUX24)
                        if (priv->ctrl_task.sync == 0) {
                                schedule_task(&priv->ctrl_task);
                        }
#else
                        if (!PENDING_WORK_ITEM(priv->ctrl_task)){
                                SCHEDULE_WORK(priv->ctrl_task);
                        }
#endif

                }
                defer_skb(priv, skb, cb, tx_done, &priv->txq);
        }
}

#if 0
/* urb_dead_complete - called by usb core layer when deadbeef urb has been transmitted
 */
STATIC void
urb_dead_complete(struct urb *urb)
{
        urb->dev = 0;

        if (urb->transfer_buffer) {
                kfree(urb->transfer_buffer);
                urb->transfer_buffer = NULL;
        }
        usb_free_urb(urb);
}
#endif


/* Bottom Half ********************************************************************************* */

/* defer_skb - put an skb on done list and schedule bottom half if necessary
 */
STATIC void
defer_skb(struct private *priv, struct sk_buff *skb, struct skb_cb *cb,
	  skb_state_t state, struct sk_buff_head *list)
{
        unsigned long   flags;

        if (!cb) {
                printk(KERN_ERR "%s: cb is NULL!\n", __FUNCTION__);
        }

        //if !(cb->urb) {
        //        printk(KERN_ERR"%s: urb is NULL!\n", __FUNCTION__);
        //}

        cb->state = state;

        if (!skb) {
                printk(KERN_ERR "%s: skb is NULL!\n", __FUNCTION__);
                return;
        }

        spin_lock_irqsave(&list->lock, flags);
        __skb_unlink(skb, list);
        spin_unlock(&list->lock);


        // link to done queue
        spin_lock(&priv->done.lock);
        __skb_queue_tail(&priv->done, skb);

        if (priv->done.qlen == 1) {
                tasklet_schedule(&priv->bh);
        }
        spin_unlock_irqrestore(&priv->done.lock, flags);
}


/* unlink_urbs - tell usb core layer that we want it to abandon attempts to send/receive urbs
 */
STATIC int
unlink_urbs(struct sk_buff_head *q)
{
        struct sk_buff *skb;
        int             count = 0;

        // move from the current queue to the unlink queue
        while ((skb = skb_dequeue(q))) {
                struct skb_cb  *cb;
                struct urb     *urb;
                struct private *priv;
                int             retval;

                if (!(cb = (struct skb_cb *) skb->cb)) {
                        printk(KERN_ERR "%s: cb NULL\n", __FUNCTION__);
                        continue;
                }

                if (!(urb = cb->urb)) {
                        printk(KERN_ERR "%s: urb NULL\n", __FUNCTION__);
                        continue;
                }

                if (!(priv = cb->priv)) {
                        printk(KERN_ERR "%s: priv NULL\n", __FUNCTION__);
                        continue;
                }

                // place them here until they can be processed after unlinking
                skb_queue_tail(&priv->unlink, skb);

                //printk(KERN_INFO "%s: unlinking skb: %p len: %d jiffs: %ld\n", __FUNCTION__,
                //                skb, skb->len, jiffies - cb->jiffies);

                // usb core layer will call rx_complete() with appropriate status so that we can remove
                urb->transfer_flags |= USB_ASYNC_UNLINK;
                if ((retval = usb_unlink_urb(urb)) < 0) {
                        dbg("unlink urb err, %d", retval);
                }
                else {
                        count++;
                }
        }
        return count;
}

/* unlink_task - called as kernel task to send crc message
 */
STATIC void
unlink_task(void *data)
{
        struct private *priv = (struct private *) data;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        unlink_urbs(&priv->txq);
        tasklet_schedule(&priv->bh);
}


/* bh - bottom half
 */
STATIC void
bh(unsigned long data)
{
        struct private *priv = (struct private *) data;
        struct sk_buff *skb;

        //printk(KERN_INFO "%s: priv#%08x\n", __FUNCTION__, (u32)(void*)priv);

        if (!priv) {
                printk(KERN_INFO "%s: priv NULL\n", __FUNCTION__);
                return;
        }

        // process all skb's on the done queue
        while ((skb = skb_dequeue(&priv->done))) {

                struct skb_cb  *cb;

                //printk(KERN_INFO "%s: skb#%08x\n", __FUNCTION__, (u32)(void*)skb);

                if (!(cb = (struct skb_cb *) skb->cb)) {
                        printk(KERN_INFO "%s: cb NULL skb: %p\n", __FUNCTION__, skb);
                        continue;
                }

                switch (cb->state) {
                case rx_done:
                        // printk(KERN_INFO"%s: rx_done skb: %p\n", __FUNCTION__, skb);
                        bh_rx_process(priv, skb);
                        break;

                case tx_done:
                        // printk(KERN_INFO"%s: tx_done skb: %p\n", __FUNCTION__, skb);
                        {
                                struct urb     *urb;
                                if (!(urb = cb->urb)) {
                                        printk(KERN_INFO "%s: urb NULL skb: %p cb: %p\n", __FUNCTION__, skb, cb);
                                        dev_kfree_skb_any(skb);
                                        continue;
                                }

                                if (urb->status) {
                                        priv->stats.tx_errors++;
                                        //printk(KERN_INFO "%s: urb: %p skb: %p status: %d\n", __FUNCTION__,
                                        //                urb, skb, cb->urb->status);
                                        urb->transfer_buffer_length = 0;
                                        urb->transfer_buffer = NULL;
                                        //usb_free_urb(urb);
                                        //dev_kfree_skb_any(skb);
                                }
                                else {
                                        priv->stats.tx_packets++;
                                        priv->stats.tx_bytes += skb->len;
                                        usb_free_urb(urb);
                                        dev_kfree_skb_any(skb);
                                }
                                //usb_free_urb(urb);
                                //dev_kfree_skb_any(skb);
                                break;
                        }
                case rx_cleanup:
                        //printk(KERN_INFO"%s: rx_cleanup skb: %p\n", __FUNCTION__, skb);
                        {
                                struct urb     *urb;
                                if (!(urb = cb->urb)) {
                                        printk(KERN_INFO "%s: urb NULL skb: %p cb: %p\n", __FUNCTION__, skb, cb);
                                        dev_kfree_skb_any(skb);
                                        continue;
                                }

                                if (urb) {
                                        usb_free_urb(urb);
                                }
                                dev_kfree_skb_any(skb);
                                break;
                        }
                case unknown:
                case tx_start:
                case rx_start:
                        printk(KERN_INFO "%s: UNKNOWN\n", __FUNCTION__);
                        printk(KERN_INFO "%s: inconsistant cb state: %d\n", __FUNCTION__, cb->state);
                        break;
                }
        }

        //printk(KERN_INFO "%s: priv->wait#%08x\n", __FUNCTION__, (u32)(void*)priv->wait);

        // are we waiting for pending urbs to complete?
        if (priv->wait) {
                if (!(priv->txq.qlen + priv->rxq.qlen + priv->done.qlen + priv->unlink.qlen)) {
                        //printk(KERN_INFO "%s: wakeup\n", __FUNCTION__);
                        wake_up(priv->wait);
                }
        }

        // do we need to queue up receive urbs?
        else if (netif_running(&priv->net)) {

                while ((priv->rxq.qlen < RX_QLEN) && (priv->timeouts < 4)) {
                        struct urb     *urb;

                        //usblan_test_kalloc("bhR0");
                        //printk(KERN_INFO "%s: allocating receive urb ATOMIC#%08x KERNEL#%08x\n", __FUNCTION__,
                        //              GFP_ATOMIC,GFP_KERNEL);
                        // allocate an urb and use rx_submit to prepare and add it to the rxq
                        if ((urb = USB_ALLOC_URB(0,GFP_ATOMIC))) {
                                //usblan_test_kalloc("bhR1");
                                //printk(KERN_INFO "%s: allocated receive urb#%08x\n", __FUNCTION__,(u32)(void*)urb);
                                rx_submit(priv, urb, GFP_ATOMIC);
                                //usblan_test_kalloc("bhR2");
                        }
                        else {
                                // we failed, schedule another run to try again
                                //printk(KERN_INFO "%s: allocating receive urb failed\n", __FUNCTION__);
                                tasklet_schedule(&priv->bh);
                        }
                }

                if (priv->txq.qlen < TX_QLEN) {
                        //printk(KERN_INFO"%s: wake queue\n", __FUNCTION__);
                        netif_wake_queue(&priv->net);
                }
        }
}




/* USB Functions - Probe and Disconnect ******************************************************** */

#define BELCARRA_SETTIME        0x04
#define BELCARRA_SETIP          0x05
#define BELCARRA_SETMSK         0x06
#define BELCARRA_SETROUTER      0x07


static void vendor_callback(struct urb *urb)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //wake_up(priv->ctrl_wait);

        usb_free_urb(urb);
        //printk(KERN_INFO"%s: finish\n", __FUNCTION__);
}


int VendorOut(struct private *priv, u8 bRequest, u16 wValue, u16 wIndex)
{
        struct usb_ctrlrequest *ctrl_request = &priv->ctrl_request;
        int rc = 0;

        //DECLARE_WAIT_QUEUE_HEAD(vendor_wakeup);
        //DECLARE_WAITQUEUE(wait, current);

        //printk(KERN_INFO"%s: bRequest: %02x wValue: %04x wIndex: %04x\n", __FUNCTION__, bRequest, wValue, wIndex);

        // setup a wait queue - this also acts as a flag to prevent bottom half from allocating more urbs
        //add_wait_queue(&vendor_wakeup, &wait);
        //priv->ctrl_wait = &vendor_wakeup;

        mutex_lock(&priv->mutex);

        THROW_UNLESS((priv->ctrl_urb = USB_ALLOC_URB(0, GFP_ATOMIC)), error);

        // create urb
        ctrl_request->bRequestType = USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        ctrl_request->bRequest = bRequest;
        ctrl_request->wValue = wValue;
        ctrl_request->wIndex = wIndex;
        ctrl_request->wLength = 0;

        FILL_CONTROL_URB(priv->ctrl_urb, priv->usbdev,
                        usb_sndctrlpipe(priv->usbdev, 0),
                        (char *)&priv->ctrl_request,
                        NULL, 0, vendor_callback, priv
                        );

        // submit urb

        THROW_IF(USB_SUBMIT_URB(priv->ctrl_urb), error);

        // sleep
        //current->state = TASK_UNINTERRUPTIBLE;
        //schedule_timeout(TIMEOUT_JIFFIES * 1000);
        //priv->ctrl_wait = NULL;


        CATCH(error) {
                rc = -EINVAL;
        }

        //if (priv->ctrl_urb)
        //        usb_free_urb(priv->ctrl_urb);
        //priv->ctrl_urb = NULL;

        // cleanup
        //remove_wait_queue(&vendor_wakeup, &wait);
        mutex_unlock(&priv->mutex);
        return rc;
}

int VendorOutLong(struct private *priv, u16 bRequest, u32 data)
{
        //printk(KERN_INFO"%s: bRequest: %02x data: %04x\n", __FUNCTION__, bRequest, data);
        data = ntohl(data);
        return VendorOut(priv, bRequest, (u16)(data>>16), (u16)(data&0xffff));
}

void network_notify(struct private *priv, u32 host_ip, u32 mask, u32 client_ip)
{


        //printk(KERN_INFO"%s: client_ip: %08x mask: %08x host_ip: %x\n", __FUNCTION__, client_ip, mask, host_ip);
        THROW_IF(VendorOutLong(priv, BELCARRA_SETIP, client_ip), error );
        THROW_IF(VendorOutLong(priv, BELCARRA_SETMSK, mask), error );
        THROW_IF(VendorOutLong(priv, BELCARRA_SETROUTER, host_ip), error );

        //printk(KERN_INFO"%s: host_ip\n", __FUNCTION__);
        //printk(KERN_INFO"%s: finished\n", __FUNCTION__);

        CATCH(error) {
        }
}


#if 0
/* This is a version of usb_clear_halt() that doesn't read the status from
 * the device -- this is because some devices crash their internal firmware
 * when the status is requested after a halt
 */
STATIC int
set_crc(struct usb_device *dev, int pipe)
{
        return usb_control_msg(dev,
                        usb_sndctrlpipe(dev, 0),
                        0x3,
                        USB_TYPE_VENDOR | USB_RECIP_INTERFACE | USB_DIR_OUT,
                        0,
                        usb_pipeendpoint(pipe) | (usb_pipein(pipe) << 7),
                        NULL, 0, HZ * 3);
}
#endif

#if 0
/* crc_task - called as kernel task to send crc message
 */
STATIC void
crc_task(void *data)
{
        struct private *priv = (struct private *) data;
        set_crc(priv->usbdev, usb_sndctrlpipe(priv->usbdev, 0));
}
#endif


/* create_private - create private data structure and initialize network interface
 */
STATIC struct private *
create_private(int devnum)
{
        struct private *priv;
        struct net_device *net;

        if (!(priv = kmalloc(sizeof(struct private), GFP_KERNEL))) {
                return NULL;
        }

        memset(priv, 0, sizeof(struct private));

        net = &priv->net;
#ifdef MODULE
        SET_MODULE_OWNER(net);
#endif
        net->priv = priv;
        //printk(KERN_INFO "%s: priv#%08x net#%08x\n",__FUNCTION__,(u32)(void*)priv,(u32)(void*)net);
        // XXX usbb vs usbl

        switch(priv->usbdnet_device_type) {

        case usbdnet_blan:
                strcpy(net->name, "usbl%d");
                break;
        default:
                strcpy(net->name, "usb%d");
                break;
        }
        memcpy(priv->dev_addr, default_addr, ETH_ALEN);

        priv->dev_addr[ETH_ALEN - 1] = (unsigned char) devnum;

        memcpy(net->dev_addr, default_addr, sizeof(default_addr));
        net->dev_addr[ETH_ALEN - 1] = devnum;

        ether_setup(net);

        net->set_mac_address = net_set_mac_address;
        net->hard_start_xmit = net_hard_start_xmit;
        net->get_stats = net_get_stats;
        net->change_mtu = net_change_mtu;
        net->open = net_open;
        net->stop = net_stop;
        net->tx_timeout = net_tx_timeout;
        net->watchdog_timeo = TIMEOUT_JIFFIES;

        //printk(KERN_INFO"%s: finis\n", __FUNCTION__);
        return priv;
}

STATIC struct usb_device_id *
idp_search(int idVendor, int idProduct)
{
        struct usb_device_id *idp;

        //printk(KERN_INFO "%s: look for idVendor: %04x idProduct: %04x\n", __FUNCTION__, idVendor, idProduct);

        // search id_table for a match
        for (idp = id_table;; idp++) {

                //printk(KERN_INFO "%s: looking at idVendor: %04x idProduct: %04x\n", __FUNCTION__,
                //              idp->idVendor, idp->idProduct);

                // end of table
                BREAK_IF (!idp->idVendor && !idp->idProduct);

                // check for match
                if ((idp->idVendor == idVendor) && (idp->idProduct == idProduct)) {
                        //printk(KERN_INFO "%s: MATCH\n", __FUNCTION__);
                        return idp;
                }
        }
        return NULL;
}



/*
 * See if we have a CDC style communications interface:
 *
 *      1. Must have specified class, subclass and protocol
 *      2. May have an optional INTERRUPT endpoint.
 *
 */
STATIC int find_interface_comm(
                struct private *priv,
                struct usb_interface *comm_interface,
                int class, int subclass, int protocol)
{
        int     alternate_number;
        struct usb_interface_descriptor *interface_descriptor;

        //printk(KERN_INFO"%s: class: %x subclass: %x protocol: %x\n", __FUNCTION__, class, subclass, protocol );

        // iterate across interface alternate descriptors looking for suitable one
        for (alternate_number = 0; alternate_number < comm_interface->num_altsetting; alternate_number++) {

//                interface_descriptor = comm_interface->altsetting + alternate_number;
                interface_descriptor = USB_ALTSETTING(comm_interface,alternate_number);
                priv->comm_ep_in = 0;

                //printk(KERN_INFO"%s: alt: %x class: %x subclass: %x protocol: %x\n", __FUNCTION__,
                //                alternate_number,
                //                interface_descriptor->bInterfaceClass,
                //                interface_descriptor->bInterfaceSubClass,
                //                interface_descriptor->bInterfaceProtocol);

                // check for class, sub-class and too many endpoints (zero or one ok)
                CONTINUE_IF ((interface_descriptor->bInterfaceClass != class) ||
                                (interface_descriptor->bInterfaceSubClass != subclass) ||
                                (interface_descriptor->bInterfaceProtocol != protocol) ||
                                (1 < interface_descriptor->bNumEndpoints) );

                // if we have an endpoint check for validity
                if (interface_descriptor->bNumEndpoints) {
                        //struct usb_endpoint_descriptor *endpoint = interface_descriptor->endpoint;
                        struct usb_endpoint_descriptor *endpoint = USB_IFC2EP(interface_descriptor,0);
                        CONTINUE_IF (!(endpoint->bEndpointAddress & USB_DIR_IN) ||
                                        (endpoint->bmAttributes != USB_ENDPOINT_XFER_INT));
                        priv->comm_ep_in = endpoint->bEndpointAddress & 0x7f;
                        priv->comm_ep_in_size = endpoint->wMaxPacketSize;
                        //printk(KERN_INFO"%s: ep found %d\n", __FUNCTION__, priv->comm_ep_in);
                }

                priv->comm_bInterfaceNumber = interface_descriptor->bInterfaceNumber;
                priv->comm_bAlternateSetting = interface_descriptor->bAlternateSetting;

                //printk(KERN_INFO"%s: found %d %d\n", __FUNCTION__,
                //                priv->comm_bInterfaceNumber, priv->comm_bAlternateSetting);
                return 0;
        }
        return -1;
}


/*
 * See if we can have a CDC style data interface:
 *
 *      1. Must have specified class, subclass and protocol
 *      2. Must have (only) a BULK-IN and BULK-OUT endpoint.
 *      3. Optionally May have INTERRUPT endpoint.
 *
 */
STATIC int find_interface_data(
                struct private *priv,
                struct usb_interface *data_interface,
                int class, int subclass, int protocol,
                int allow_interrupt)
{
        int     alternate_number;
        struct usb_interface_descriptor *interface_descriptor;

        //printk(KERN_INFO"%s: class: %x subclass: %x protocol: %x\n", __FUNCTION__, class, subclass, protocol );

        for (alternate_number = 0; alternate_number < data_interface->num_altsetting; alternate_number++) {

                //interface_descriptor = data_interface->altsetting + alternate_number;
                interface_descriptor = USB_ALTSETTING(data_interface,alternate_number);
                priv->data_ep_in = priv->data_ep_out = 0;

                //printk(KERN_INFO"%s: alt: %d class: %x subclass: %x protocol: %x endpoints: %d\n", __FUNCTION__,
                //                alternate_number,
                //                interface_descriptor->bInterfaceClass,
                //                interface_descriptor->bInterfaceSubClass,
                //                interface_descriptor->bInterfaceProtocol,
                //                interface_descriptor->bNumEndpoints);

                // check for class, sub-class and wrong number of endpoints (must be two)
                CONTINUE_IF ((interface_descriptor->bInterfaceClass != class) ||
                                (interface_descriptor->bInterfaceSubClass != subclass) ||
                                (interface_descriptor->bInterfaceProtocol != protocol) );

                CONTINUE_IF (allow_interrupt && !((1 < interface_descriptor->bNumEndpoints)
                                        && (3 >= interface_descriptor->bNumEndpoints)));

                CONTINUE_IF (!allow_interrupt && (2 != interface_descriptor->bNumEndpoints));

                // check endpoints for validity, must be BULK and we want one in each direction, no more, no less
                if (interface_descriptor->bNumEndpoints) {
                        int endpoint_number;
                        for (endpoint_number = 0; endpoint_number < interface_descriptor->bNumEndpoints; endpoint_number++) {
                                //struct usb_endpoint_descriptor *endpoint = interface_descriptor->endpoint + endpoint_number;
                                struct usb_endpoint_descriptor *endpoint = USB_IFC2EP(interface_descriptor,endpoint_number);

                                BREAK_IF (USB_ENDPOINT_XFER_BULK != endpoint->bmAttributes);

                                if (endpoint->bEndpointAddress & USB_DIR_IN) {
                                        priv->data_ep_in = endpoint->bEndpointAddress & 0x7f;
                                        priv->data_ep_in_size = endpoint->wMaxPacketSize;
                                }
                                else {
                                        priv->data_ep_out = endpoint->bEndpointAddress & 0x7f;
                                        priv->data_ep_out_size = endpoint->wMaxPacketSize;
                                }
                        }
                }

                CONTINUE_IF (!priv->data_ep_in || !priv->data_ep_out);

                //printk(KERN_INFO"%s: ep found %d %d\n", __FUNCTION__, priv->data_ep_in, priv->data_ep_out);

                priv->data_bInterfaceNumber = interface_descriptor->bInterfaceNumber;
                priv->data_bAlternateSetting = interface_descriptor->bAlternateSetting;

                //printk(KERN_INFO"%s: found %d %d\n", __FUNCTION__,
                //                priv->data_bInterfaceNumber, priv->data_bAlternateSetting);
                return 0;
        }
        //printk(KERN_INFO"%s: not found\n", __FUNCTION__);
        return -1;
}

/*
 * See if we can have a CDC style no-data interface:
 *
 *      1. Must have specified class, subclass and protocol
 *      2. Must have a zero endpoints.
 */
STATIC int find_interface_nodata(
                struct private *priv,
                struct usb_interface *data_interface,
                int class, int subclass, int protocol)
{
        int     alternate_number;
        struct usb_interface_descriptor *interface_descriptor;

        for (alternate_number = 0; alternate_number < data_interface->num_altsetting; alternate_number++) {

                //interface_descriptor = data_interface->altsetting + alternate_number;
                interface_descriptor = USB_ALTSETTING(data_interface,alternate_number);

                //printk(KERN_INFO"%s: alt: %d class: %d subclass: %d endpoints: %d\n", __FUNCTION__, alternate_number,
                //                interface_descriptor->bInterfaceClass,
                //                interface_descriptor->bInterfaceSubClass,
                //                interface_descriptor->bNumEndpoints);

                // check for class, sub-class, protocol and verify no endpoints
                CONTINUE_IF ((interface_descriptor->bInterfaceClass != class) ||
                                (interface_descriptor->bInterfaceSubClass != subclass) ||
                                (interface_descriptor->bInterfaceProtocol != protocol) ||
                                (interface_descriptor->bNumEndpoints));

                priv->nodata_bInterfaceNumber = interface_descriptor->bInterfaceNumber;
                priv->nodata_bAlternateSetting = interface_descriptor->bAlternateSetting;

                //printk(KERN_INFO"%s: found %d %d\n", __FUNCTION__,
                //                priv->nodata_bInterfaceNumber, priv->nodata_bAlternateSetting);

                return 0;
        }
        //printk(KERN_INFO"%s: not found\n", __FUNCTION__);
        return -1;
}

/*
 * See if we can have a MDLM style no-data interface:
 *
 *      1. Must have specified class, subclass and protocol
 *      2. Must have have BULK-IN and BULK-OUT endpoints
 *      3. May have INTERRUPT endpoint
 *
 */
STATIC int find_interface_mdlm(
                struct private *priv,
                struct usb_interface *mdlm_interface,
                int class,
                int subclass,
                int protocol,
                char *guid
                )
{
        struct usb_interface_descriptor *interface_descriptor;
        struct usb_class_descriptor *extra;
        int extralen;

        if (mdlm_interface->num_altsetting > 1) {
                printk(KERN_INFO"%s: too many intefaces num_altsetting: %d\n", __FUNCTION__, mdlm_interface->num_altsetting);
                return -1;
        }

        //interface_descriptor = mdlm_interface->altsetting;
        interface_descriptor = USB_ALTSETTING(mdlm_interface,0);

        //printk(KERN_INFO"%s: class: %d subclass: %d endpoints: %d\n", __FUNCTION__,
        //                interface_descriptor->bInterfaceClass,
        //                interface_descriptor->bInterfaceSubClass,
        //                interface_descriptor->bNumEndpoints
        //      );

        // check for class, sub-class, protocol and correct number of endpoints (two or three ok)
        if ((interface_descriptor->bInterfaceClass != class) ||
                        (interface_descriptor->bInterfaceSubClass != subclass) ||
                        (interface_descriptor->bInterfaceProtocol != protocol) ||
                        !((1 < interface_descriptor->bNumEndpoints) && (3 >= interface_descriptor->bNumEndpoints))
           )
        {
                return -1;
        }

        if (interface_descriptor->bNumEndpoints) {
                int endpoint_number;
                priv->comm_ep_in = priv->data_ep_in = priv->data_ep_out = 0;
                for (endpoint_number = 0; endpoint_number < interface_descriptor->bNumEndpoints; endpoint_number++) {
                        //struct usb_endpoint_descriptor *endpoint = interface_descriptor->endpoint + endpoint_number;
                        struct usb_endpoint_descriptor *endpoint = USB_IFC2EP(interface_descriptor,endpoint_number);

                        if (USB_ENDPOINT_XFER_BULK == endpoint->bmAttributes) {
                                if (endpoint->bEndpointAddress & USB_DIR_IN) {
                                        priv->data_ep_in = endpoint->bEndpointAddress & 0x7f;
                                        priv->data_ep_in_size = endpoint->wMaxPacketSize;
                                }
                                else {
                                        priv->data_ep_out = endpoint->bEndpointAddress & 0x7f;
                                        priv->data_ep_out_size = endpoint->wMaxPacketSize;
                                }
                        }
                        else if ((USB_ENDPOINT_XFER_INT == endpoint->bmAttributes) &&
                                        (endpoint->bEndpointAddress & USB_DIR_IN))
                        {
                                priv->comm_ep_in = endpoint->bEndpointAddress & 0x7f;
                                priv->comm_ep_in_size = endpoint->wMaxPacketSize;
                        }
                        else {
                                return -1;
                        }
                }
        }

        if (!priv->data_ep_in || !priv->data_ep_out) {
                return -1;
        }

        extra = (struct usb_class_descriptor *)USB_IFC2HOST(interface_descriptor)->extra;
        extralen = USB_IFC2HOST(interface_descriptor)->extralen;
        while (extra && (extralen > 0)) {

                u8 *cp = (u8 *) extra;

                //printk(KERN_INFO"%s: %p extralen: %02x bLength: %02x bDescriptorType: %02x bDescriptorSubType: %02x\n",
                //                __FUNCTION__, extra, extralen,
                //                extra->bLength, extra->bDescriptorType, extra->bDescriptorSubType);

                BREAK_IF(!extra->bLength);

                // check for CS descriptors (0x24)
                switch(extra->bDescriptorType) {


                case USB_DT_CS_INTERFACE:

                        //printk(KERN_INFO"%s: CS bDescriptorSubType: %02x\n", __FUNCTION__, extra->bDescriptorSubType);

                        // then check for MDLM Functional or Detail descriptors
                        switch (extra->bDescriptorSubType) {

                                // check for bGuid match
                        case MDLM_FUNCTIONAL:
                                {
                                        struct usb_mdlm_functional_descriptor *functional =
                                                (struct usb_mdlm_functional_descriptor *) extra;
                                        //u8 *bGuid = (u8 *)&functional->bGuid;
                                        //printk(KERN_INFO"%s: FUNCTIONAL bGUID %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                        //                __FUNCTION__,
                                        //                bGuid[0],bGuid[1],bGuid[2],bGuid[3],
                                        //                bGuid[4],bGuid[5],bGuid[6],bGuid[7]
                                        //                );

                                        //printk(KERN_INFO"%s: FUNCTIONAL bGUID %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                        //                __FUNCTION__,
                                        //                bGuid[8],bGuid[9],bGuid[10],bGuid[11],
                                        //                bGuid[12],bGuid[13],bGuid[14],bGuid[15]
                                        //                );

                                        RETURN_IF( -1, memcmp(guid, functional->bGuid, sizeof(functional->bGuid) != 0 ));
                                        break;
                                }

                                // find details
                        case MDLM_DETAIL:
                                {
                                        struct usb_mdlm_detail_descriptor *mdlm = (struct usb_mdlm_detail_descriptor *) extra;

                                        //printk(KERN_INFO"%s: DETAIL bGuidDescriptorType\n", __FUNCTION__);

                                        // figure out what type of detail record it is
                                        switch (mdlm->bGuidDescriptorType) {
                                        case MDLM_SAFE_GUID:
                                                {
                                                        struct usb_safe_detail_descriptor *safe =
                                                                (struct usb_safe_detail_descriptor *) extra;
                                                        //printk(KERN_INFO"%s: SAFE bmDataCapabilities: %02x\n",
                                                        //                __FUNCTION__, safe->bmDataCapabilities);
                                                        priv->bmNetworkCapabilities = safe->bmNetworkCapabilities;
                                                        priv->bmDataCapabilities = safe->bmDataCapabilities;
                                                        break;
                                                }
                                        case MDLM_BLAN_GUID:
                                                {
                                                        struct usb_blan_detail_descriptor *blan =
                                                                (struct usb_blan_detail_descriptor *) extra;

                                                        //printk(KERN_INFO"%s: BLAN bmDataCapabilities: %02x\n",
                                                        //                __FUNCTION__, blan->bmDataCapabilities);
                                                        priv->bmNetworkCapabilities = blan->bmNetworkCapabilities;
                                                        priv->bmDataCapabilities = blan->bmDataCapabilities;
                                                        priv->bPad = blan->bPad;
                                                        break;
                                                }
                                        default:
                                                break;
                                        }
                                }

                        default:
                                break;
                        }

                default:
                        break;
                }

                extralen -= extra->bLength;
                cp = cp + extra->bLength;
                extra = (struct usb_class_descriptor *) cp;
        }

        priv->data_bInterfaceNumber = interface_descriptor->bInterfaceNumber;
        priv->data_bAlternateSetting = interface_descriptor->bAlternateSetting;

        //printk(KERN_INFO"%s: found %d %d\n", __FUNCTION__, priv->data_bInterfaceNumber, priv->data_bAlternateSetting);
        return 0;
}

/*
 * See if we can have a CDC interface:
 *
 *      1. class        CDC_INTERFACE_CLASS
 *      2. subclass     CDC_INTERFACE_SUBCLASS
 *      3. protocol     DEFAULT_PROTOCOL
 *
 */
STATIC int verify_ethernet_comm_interface(
                struct usb_device *device,
                struct private *priv,
                int comm_number, struct usb_interface *comm_interface,
                int data_number, struct usb_interface *data_interface
                )
{
        //printk(KERN_INFO"verify_ethernet_comm_interface:\n");

        if (find_interface_comm(priv, comm_interface, CDC_INTERFACE_CLASS, CDC_INTERFACE_SUBCLASS, DEFAULT_PROTOCOL)) {
                //printk(KERN_INFO"verify_ethernet_comm_interface: no comm\n");
                return -1;
        }

        if (find_interface_data(priv, data_interface, DATA_INTERFACE_CLASS, 0, DEFAULT_PROTOCOL, 0)) {
                //printk(KERN_INFO"verify_ethernet_comm_interface: no data\n");
                return -1;
        }

        if (find_interface_nodata(priv, data_interface, DATA_INTERFACE_CLASS, 0, DEFAULT_PROTOCOL)) {
                //printk(KERN_INFO"verify_ethernet_comm_interface: no nodata\n");
        }

        //printk(KERN_INFO"verify_ethernet_comm_interface: found\n");

        priv->comm_interface = comm_number;
        priv->data_interface = data_number;
        priv->usbdnet_device_type = usbdnet_cdc;

        return 0;
}

/*
 * See if we can have an RNDIS interface:
 *
 *      1. class        CDC_INTERFACE_CLASS
 *      2. subclass     RNDIS_INTERFACE_SUBCLASS
 *      3. protocol     VENDOR_SPECIFIC_PROTOCOL
 *
 */
STATIC int verify_rndis_comm_interface(
                struct usb_device *device,
                struct private *priv,
                int comm_number, struct usb_interface *comm_interface,
                int data_number, struct usb_interface *data_interface
                )
{

        //printk(KERN_INFO"verify_rndis_comm_interface:\n");

        if (find_interface_comm(priv, comm_interface, CDC_INTERFACE_CLASS, RNDIS_INTERFACE_SUBCLASS, VENDOR_SPECIFIC_PROTOCOL)) {
                //printk(KERN_INFO"verify_rndis_comm_interface: no comm\n");
                return -1;
        }

        if (find_interface_data(priv, data_interface, DATA_INTERFACE_CLASS, 0, DEFAULT_PROTOCOL, 0)) {
                //printk(KERN_INFO"verify_rndis_comm_interface: no data\n");
                return -1;
        }

        //if (find_interface_nodata(priv, data_interface, DATA_INTERFACE_CLASS, 0, DEFAULT_PROTOCOL)) {
        //        printk(KERN_INFO"verify_rndis_comm_interface: no nodata\n");
        //}

        //printk(KERN_INFO"verify_rndis_comm_interface: found\n");

        priv->comm_interface = comm_number;
        priv->data_interface = data_number;
        priv->usbdnet_device_type = usbdnet_rndis;

        return 0;
}


/*
 * See if we can have an MDLM-BLAN interface:
 *
 *      1. class        CDC_INTERFACE_CLASS
 *      2. subclass     MDLM_INTERFACE_SUBCLASS
 *      3. protocol     DEFAULT_PROTOCOL
 *
 */
STATIC int verify_blan_interface(
                struct usb_device *device,
                struct private *priv,
                struct usb_interface *data_interface
                )
{
        if (find_interface_mdlm(priv, data_interface,
                                CDC_INTERFACE_CLASS,
                                MDLM_INTERFACE_SUBCLASS,
                                DEFAULT_PROTOCOL,
                                BLAN_GUID))
        {
                printk(KERN_INFO"%s: not found\n", __FUNCTION__);
                return -1;
        }

        //printk(KERN_INFO"%s: found bmDataCapabilities: %02x bPad %02x\n",
        //                __FUNCTION__, priv->bmDataCapabilities, priv->bPad);

        priv->comm_interface = priv->data_interface = 0;
        priv->usbdnet_device_type = usbdnet_blan;
        return 0;
}

/*
 * See if we can have an MDLM-SAFE interface:
 *
 *      1. class        CDC_INTERFACE_CLASS
 *      2. subclass     MDLM_INTERFACE_SUBCLASS
 *      3. protocol     DEFAULT_PROTOCOL
 *
 */
STATIC int verify_safe_interface(
                struct usb_device *device,
                struct private *priv,
                struct usb_interface *data_interface
                )
{
        if (find_interface_mdlm(priv, data_interface,
                                CDC_INTERFACE_CLASS,
                                MDLM_INTERFACE_SUBCLASS,
                                DEFAULT_PROTOCOL,
                                SAFE_GUID))
        {
                printk(KERN_INFO"%s: not found\n", __FUNCTION__);
                return -1;
        }

        //printk(KERN_INFO"%s: found bmDataCapabilities: %02x\n", __FUNCTION__, priv->bmDataCapabilities);

        priv->comm_interface = priv->data_interface = 0;
        priv->usbdnet_device_type = usbdnet_safe;
        return 0;
}

/*
 * See if we can have a BASIC interface:
 *
 *      1. class        VENDOR_SPECIFIC_CLASS
 *      2. subclass     LINEO_INTERFACE_SUBCLASS_SAFENET
 *      3. protocol     LINEO_SAFENET_CRC
 *
 */
STATIC int verify_basic_interface(
                struct usb_device *device,
                struct private *priv,
                struct usb_interface *data_interface
                )
{
        if (find_interface_data(priv, data_interface,
                                VENDOR_SPECIFIC_CLASS,
                                LINEO_INTERFACE_SUBCLASS_SAFENET,
                                LINEO_SAFENET_CRC,
                                1))
        {
                printk(KERN_INFO"%s: not found\n", __FUNCTION__);
                return -1;
        }

        //printk(KERN_INFO"%s: found\n", __FUNCTION__);

        priv->comm_interface = priv->data_interface = 0;
        priv->usbdnet_device_type = usbdnet_basic;
        return 0;
}


/*
 * Iterate across configurations looking for one that we like.
 */
STATIC int find_valid_configuration(struct usb_device *usbdev, struct private *priv)
{
        int configuration_number;

        // We will try each and every possible configuration
        for ( configuration_number = 0;
                        configuration_number < usbdev->descriptor.bNumConfigurations;
                        configuration_number++ )
        {

                //struct usb_config_descriptor *configuration = usbdev->config + configuration_number;
                struct usb_config_descriptor *configuration = USB_DEV2CONFIG(usbdev,configuration_number);

                //printk(KERN_INFO"%s[%d] bConfigurationValue: %d bNumInterfaces: %d\n", __FUNCTION__,
                //                configuration_number, configuration->bConfigurationValue, configuration->bNumInterfaces);

                priv->configuration_number = configuration_number;
                priv->bConfigurationValue = configuration->bConfigurationValue;

                priv->comm_bInterfaceNumber = priv->comm_bAlternateSetting = -1;
                priv->data_bInterfaceNumber = priv->data_bAlternateSetting = -1;
                priv->nodata_bInterfaceNumber = priv->nodata_bAlternateSetting = -1;

                /*
                 * CDC and RNDIS devices have two interfaces,
                 * MDLM and simple devices have a single interface,
                 * we don't handle any devices with more or less than one or two intefaces.
                 */
                //printk(KERN_INFO"%s: interface(s) %d\n", __FUNCTION__, configuration->bNumInterfaces);
                switch(configuration->bNumInterfaces) {

                        /*
                         * Tests for devices with two interfaces, e.g. CDC and RNDIS.
                         *
                         * We cannot guarantee which order the comm and data interfaces will be so we have to check for
                         * comm/data or data/comm.
                         */
                case 2: {
                                //struct usb_interface *int0 = configuration->interface + 0;
                                //struct usb_interface *int1 = configuration->interface + 1;
                                struct usb_interface *int0 = USB_CONFIG2IFC(configuration,0);
                                struct usb_interface *int1 = USB_CONFIG2IFC(configuration,1);

                                if ( !verify_ethernet_comm_interface(usbdev, priv, 0, int0, 1, int1)) {
                                        //printk(KERN_INFO"%s: cdc 0 1\n", __FUNCTION__);
                                        return 0;
                                }
                                else if ( !verify_ethernet_comm_interface(usbdev, priv, 1, int1, 0, int0)) {
                                        //printk(KERN_INFO"%s: cdc 1 0\n", __FUNCTION__);
                                        return 0;
                                }
                                else if ( !verify_rndis_comm_interface(usbdev, priv, 0, int0, 1, int1)) {
                                        //printk(KERN_INFO"%s: rndis 0 1\n", __FUNCTION__);
                                        return 0;
                                }
                                else if ( !verify_rndis_comm_interface(usbdev, priv, 1, int1, 0, int0)) {
                                        //printk(KERN_INFO"%s: rndis 1 0\n", __FUNCTION__);
                                        return 0;
                                }
                        }
                        break;

                        /*
                         * tests for devices with a single interface, e.g. MDLM-SAFE MDLM-BLAN or BASIC
                         */
                case 1:

                        if ( !verify_blan_interface(usbdev, priv, USB_CONFIG2IFC(configuration,0))) {
                                //printk(KERN_INFO"%s[%d] bConfigurationValue: %d bNumInterfaces: %d\n", __FUNCTION__,
                                //       configuration_number, priv->bConfigurationValue, configuration->bNumInterfaces);
                                //printk(KERN_INFO"%s: mdlm-blan\n", __FUNCTION__);
                                return 0;
                        }
                        else if ( !verify_safe_interface(usbdev, priv, USB_CONFIG2IFC(configuration,0))) {
                                //printk(KERN_INFO"%s[%d] bConfigurationValue: %d bNumInterfaces: %d\n", __FUNCTION__,
                                //       configuration_number, priv->bConfigurationValue, configuration->bNumInterfaces);
                                //printk(KERN_INFO"%s: mdlm-safe\n", __FUNCTION__);
                                return 0;
                        }
                        else if ( !verify_basic_interface(usbdev, priv, USB_CONFIG2IFC(configuration,0))) {
                                //printk(KERN_INFO"%s: basic\n", __FUNCTION__);
                                return 0;
                        }
                        break;

                        /*
                         * currently we have no support for devices with zero or more than
                         * two interfaces....
                         */
                default:
                        break;
                }
                priv->configuration_number = priv->bConfigurationValue = 0;
        }

        // None of the configurations suited us.
        //printk(KERN_INFO"%s: nothing found\n", __FUNCTION__);
        return -1;
}

/*
 * check the active configuration to see if there are any claimed interfaces
 */
STATIC int verify_no_claimed_interfaces(struct usb_config_descriptor *act_config)
{
        struct usb_interface *interface;
        int             interface_number;

        // Go through all the interfaces and make sure none are
        // claimed by anybody else.
        //

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        if (!act_config) {
                printk(KERN_INFO"%s: NULL\n", __FUNCTION__);
                return 0;
        }

        //printk(KERN_INFO"%s: bNumInterfaces: %d\n", __FUNCTION__, act_config->bNumInterfaces);
        for (interface_number = 0; interface_number < act_config->bNumInterfaces; interface_number++) {


                interface = USB_CONFIG2IFC(act_config,interface_number);

#if 0
                if (usb_interface_claimed(interface)) {
                        printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                        return -1;
                }
#endif
        }
        //printk(KERN_INFO"%s: ok\n", __FUNCTION__);
        return 0;
}

#if defined(CONFIG_USB_USBLAN) || defined(CONFIG_USB_USBLAN_MODULE)
/********************************************************
 * In the 2.6 host system, the result of "probe" is a
 * ERROR value, (0 for success, negative for failure)
 * The actual value of priv is not passed to the caller
 * of probe (usb_probe_interface)
 *****************************************************/
STATIC int usblan_probe_return(struct private *priv){
        return ((priv == 0) ? -1 : 0);
}

#else

#error
/********************************************************
 *  In the 2.4 host system, the probe routine is called
 *  by usb_find_interface_driver. If probe() returns a
 *  non zero pointer, that value is passed along to
 *  usb_claim_interface()
 *******************************************************/
STATIC void * usblan_probe_return(struct private *priv){
        return priv;
}

#error

#endif

#if defined(CONFIG_USB_USBLAN) || defined(CONFIG_USB_USBLAN_MODULE)
STATIC int usblan_probe(struct usb_interface * udev, const struct usb_device_id *id){
        struct private *priv = NULL;
        struct usb_device_descriptor *device = NULL;
        struct usb_device *usbdev;

        /* find the usb device for the usb_interface */
        usbdev = interface_to_usbdev(udev);
        /* find the usb device descriptor  */
        device = &usbdev->descriptor;
        //We can now resume the old probe routine except when returning a value
#else
#error
STATIC void    *
usblan_probe(struct usb_device *usbdev, unsigned int ifnum, const struct usb_device_id *id)
{
        struct private *priv = NULL;
        struct usb_device_descriptor *device = &usbdev->descriptor;

#endif

        /* Begin the common portion of the probe routine */
        //printk(KERN_INFO "%s: probe\n", __FUNCTION__);

        /* do we have a valid device structure, is the device a CDC device, is the product_id and
         * vendor_id one that we are supposed to handle, has anyone else claimed any interfaces?
         */
        THROW_IF (device->bDeviceClass != CDC_DEVICE_CLASS, error);
        THROW_IF (!idp_search(device->idVendor, device->idProduct), error);
#if defined(LINUX24)
        THROW_IF (verify_no_claimed_interfaces(USB_CONFIG2DESC(usbdev->actconfig)), error);
#endif

        /* create a private structure to save state information about this usb device.
         */
        THROW_IF (!(priv = create_private(usbdev->devnum)), error);
        THROW_IF (find_valid_configuration(usbdev, priv), error);

        /*
         * There is a valid configuration.
         */
        //printk(KERN_INFO"%s: configuration_number: %d bConfigurationValue: %d\n", __FUNCTION__,
        //        priv->configuration_number, priv->bConfigurationValue);

        THROW_IF (register_netdev(&priv->net), free_priv);


#if !defined(CONFIG_USB_USBLAN) && !defined(CONFIG_USB_USBLAN_MODULE)
        // usb_set_config() is not available inside probe() for 2.6.
        //printk(KERN_INFO"%s: setting configuration bConfigurationValue: %d\n", __FUNCTION__, priv->bConfigurationValue);

        THROW_IF ( usb_set_configuration( usbdev, priv->bConfigurationValue ), unregister );
#else
        // Claim the interfaces we want before starting to operate on them...
        // manually claim the interfaces we want.
        switch (priv->usbdnet_device_type ) {
        case usbdnet_cdc:
        case usbdnet_rndis:
                //printk(KERN_INFO"%s: claiming comm interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->comm_interface);

                usb_driver_claim_interface( &usblan_driver,
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->comm_interface), priv);
                priv->intf_count += 1;

                /* FALL THROUGH */ // In order to claim the data interface too.
        case usbdnet_safe:
        case usbdnet_basic:
        case usbdnet_blan:
                //printk(KERN_INFO"%s: claiming data interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->data_interface);

                usb_driver_claim_interface( &usblan_driver,
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->data_interface),
                                priv );
                priv->intf_count += 1;

                break;
        case usbdnet_unknown:
                THROW(unregister);
        }
        priv->intf_max = priv->intf_count;
#endif

        //printk(KERN_INFO"%s: %s\n", __FUNCTION__, usbdnet_device_names[priv->usbdnet_device_type]);

        /*
         * If we have a nodata interface interface use it, otherwise use the normal data interface.
         * MDLM and Safe devices never have a nodata interface.
         */
        switch (priv->usbdnet_device_type ) {
        case usbdnet_cdc:
        case usbdnet_rndis:

                //printk(KERN_INFO"%s: setting comm interface bInterfaceNumber: %d bAlternateSetting: %d\n", __FUNCTION__,
                //                priv->comm_bInterfaceNumber, priv->comm_bAlternateSetting);
                THROW_IF (usb_set_interface(usbdev, priv->comm_bInterfaceNumber, priv->comm_bAlternateSetting), unregister);

                if (priv->nodata_bInterfaceNumber >= 0)  {
                        //printk(KERN_INFO"%s: setting nodata interface bInterfaceNumber: %d bAlternateSetting: %d\n",
                        //                __FUNCTION__, priv->nodata_bInterfaceNumber, priv->nodata_bAlternateSetting);
                        THROW_IF (usb_set_interface( usbdev, priv->nodata_bInterfaceNumber, priv->nodata_bAlternateSetting),
                                        unregister);
                }
                else {
                        /* FALL THROUGH */

                        //printk(KERN_INFO"%s: setting data interface bInterfaceNumber: %d bAlternateSetting: %d\n",
                        //                __FUNCTION__, priv->data_bInterfaceNumber, priv->data_bAlternateSetting);
                        THROW_IF (usb_set_interface( usbdev, priv->data_bInterfaceNumber, priv->data_bAlternateSetting),
                                        unregister);
                }
                break;

        case usbdnet_safe:
        case usbdnet_blan:
        case usbdnet_basic:

                break;

        case usbdnet_unknown:
                THROW(unregister);
        }


#if !defined(CONFIG_USB_USBLAN) && !defined(CONFIG_USB_USBLAN_MODULE)
        // manually claim the interfaces we want.
        switch (priv->usbdnet_device_type ) {
        case usbdnet_cdc:
        case usbdnet_rndis:
                //printk(KERN_INFO"%s: claiming comm interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->comm_interface);

                usb_driver_claim_interface( &usblan_driver,
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->comm_interface), priv);
                                //&(usbdev->config[priv->configuration_number].interface[priv->comm_interface]), priv );
                priv->intf_count += 1;

                /* FALL THROUGH */ // In order to claim the data interface too.
        case usbdnet_safe:
        case usbdnet_basic:
        case usbdnet_blan:
                //printk(KERN_INFO"%s: claiming data interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->data_interface);

                usb_driver_claim_interface( &usblan_driver,
                                //&(usbdev->config[priv->configuration_number].interface[priv->data_interface]),
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->data_interface),
                                priv );
                priv->intf_count += 1;

                break;
        case usbdnet_unknown:
                THROW(unregister);
        }
        priv->intf_max = priv->intf_count;
#endif


        skb_queue_head_init(&priv->rxq);
        skb_queue_head_init(&priv->txq);
        skb_queue_head_init(&priv->unlink);
        skb_queue_head_init(&priv->done);

        //printk(KERN_INFO "%s: tx_size: %3d rx_size: %3d\n", __FUNCTION__, priv->data_ep_out_size, priv->data_ep_in_size);
        //printk(KERN_INFO "%s: tx_ep  : %3x rx_ep  : %3x\n", __FUNCTION__, priv->data_ep_out, priv->data_ep_in);

        priv->usbdev = usbdev;

#if defined(LINUX24)
        // ctrl task for clear halt operation
        priv->ctrl_task.routine = ctrl_task;
        priv->ctrl_task.data = priv;
        priv->ctrl_task.sync = 0;
#else
        PREPARE_WORK_ITEM(priv->ctrl_task, ctrl_task, priv);
#endif

        // reset task for reseting device
#if defined(LINUX26)
        PREPARE_WORK_ITEM(priv->reset_task, reset_task, priv);
#else
        priv->reset_task.routine = reset_task;
        priv->reset_task.data = priv;
        priv->reset_task.sync = 0;
#endif

        // unlink task for reseting device
#if defined(LINUX26)
        PREPARE_WORK_ITEM(priv->unlink_task, unlink_task, priv);
#else
        priv->unlink_task.routine = unlink_task;
        priv->unlink_task.data = priv;
        priv->unlink_task.sync = 0;
#endif

        // bottom half processing tasklet
        priv->bh.func = bh;
        priv->bh.data = (unsigned long) priv;

        // init mutex and list, add to device list
        init_MUTEX(&priv->mutex);
        INIT_LIST_HEAD(&priv->list);

        mutex_lock(&usbd_mutex);
        list_add(&priv->list, &usbd_list);
        mutex_unlock(&usbd_mutex);

        //printk(KERN_INFO "%s: success %s\n", __FUNCTION__, DRIVER_VERSION);

        switch (priv->usbdnet_device_type ) {
        case usbdnet_blan:
                network_notify(priv, NETWORK_ADDR_HOST, NETWORK_MASK, NETWORK_ADDR_CLIENT);
                network_attach(&priv->net, NETWORK_ADDR_HOST, NETWORK_MASK, 1);
                break;
        default:
                break;
        }


        // start as if the link is up
        netif_device_attach(&priv->net);

//#define NETWORK_ADDR_HOST       0xac100005      /* 172.16.0.0 */
//#define NETWORK_ADDR_CLIENT     0xac100006      /* 172.16.0.0 */
//#define NETWORK_MASK            0xfffffffc

#if defined(CONFIG_USB_USBLAN) || defined(CONFIG_USB_USBLAN_MODULE)
        usb_get_dev(usbdev);
#else
        usb_inc_dev_use(usbdev);
#endif

        usb_set_intfdata(udev, priv);

        CATCH(error) {
                printk(KERN_ERR"%s: caught error\n", __FUNCTION__);

                CATCH(free_priv) {

                        printk(KERN_ERR"%s: caught free_priv\n", __FUNCTION__);

                        CATCH(unregister) {

                                printk(KERN_ERR"%s: caught unregister\n", __FUNCTION__);

                                printk(KERN_ERR"%s: unregister\n", __FUNCTION__);
                                unregister_netdev(&priv->net);
                        }

                        printk(KERN_ERR"%s: free priv\n", __FUNCTION__);
                        kfree(priv);
                }
                printk(KERN_ERR"%s: return NULL\n", __FUNCTION__);
                return usblan_probe_return(NULL);
        }
        //printk(KERN_ERR"%s: return %p\n", __FUNCTION__, priv);
        return usblan_probe_return(priv);
}

#if defined(CONFIG_USB_USBLAN) || defined(CONFIG_USB_USBLAN_MODULE)
STATIC void usblan_disconnect(struct usb_interface *intf)
{
        struct private *priv = (struct private *) usb_get_intfdata(intf);
        struct usb_device *usbdev = interface_to_usbdev(intf);

        /* If we claimed more than one interface during probe, this will
           be called more than once. */
        if (priv->intf_count == priv->intf_max) {
                // This is the first interface to be disconnected, unregister the network interface
                unregister_netdev(&priv->net);

                // remove from device list
                mutex_lock(&usbd_mutex);
                mutex_lock(&priv->mutex);
                list_del(&priv->list);
                mutex_unlock(&usbd_mutex);
        }
        // release interfaces

        //printk(KERN_INFO"%s: releasing interface: %p from configuration: %d\n", __FUNCTION__, intf, priv->configuration_number);
        // XXX usb_driver_release_interface(&usblan_driver,intf);
        //printk(KERN_INFO"%s: releasing interface: ok\n", __FUNCTION__);
        priv->intf_count -= 1;

        if (priv->intf_count <= 0) {
                // disable
                usb_put_dev(usbdev);

                // destroy the network interface and free the private storage
                kfree(priv);
        }
}
#else
STATIC void     usblan_disconnect(struct usb_device *usbdev, void *ptr) {
        struct private *priv = (struct private *) ptr;

        // unregister the network interface
        unregister_netdev(&priv->net);

        // remove from device list
        mutex_lock(&usbd_mutex);
        mutex_lock(&priv->mutex);
        list_del(&priv->list);
        mutex_unlock(&usbd_mutex);

        // release interfaces


        switch (priv->usbdnet_device_type ) {
        case usbdnet_cdc:
        case usbdnet_rndis:
                //printk(KERN_INFO"%s: releasing comm interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->comm_interface);
                usb_driver_release_interface( &usblan_driver,
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->comm_interface));
//                              &(usbdev->config[priv->configuration_number].interface[priv->comm_interface]));
                /* FALL THROUGH */
        case usbdnet_safe:
        case usbdnet_basic:
        case usbdnet_blan:
                //printk(KERN_INFO"%s: releasing data interface: configuration: %d interface: %d\n", __FUNCTION__,
                //                priv->configuration_number, priv->data_interface);
                usb_driver_release_interface( &usblan_driver,
                                USB_CONFIG2IFC(USB_DEV2CONFIG(usbdev,priv->configuration_number),priv->data_interface));
//                              &(usbdev->config[priv->configuration_number].interface[priv->data_interface]));
        case usbdnet_unknown:
                break;
        }


        // disable
        usb_dec_dev_use(usbdev);  // QQSV

        // destroy the network interface and free the private storage
        kfree(priv);
}
#endif


static struct usb_driver usblan_driver = {
     name:        "usblan",
     probe:       usblan_probe,
     disconnect:  usblan_disconnect, // QQSV
     id_table:    id_table,
};


/* Module loading functions - modinit and modexit*********************************************** */

/*
 * usbdnet_modinit - module init
 *
 */
STATIC int __init usbdnet_modinit(void)
{
        //USB_PROBE_SET(usblan_driver, probe);
        //printk(KERN_INFO DRIVER_VERSION " " DRIVER_AUTHOR "XXX\n");


        info(DRIVER_DESC " " DRIVER_VERSION " " DRIVER_AUTHOR);
        //info(DRIVER_DESC);

        get_random_bytes(default_addr, ETH_ALEN);
        default_addr[0] = (default_addr[0] & 0xf0) | 0x02;

#if defined(LONG_STRING_OF_ZEROES_HACK)
        fermat_init();
#endif

        // if we have vendor_id / product_id parameters patch them into id list
        if (vendor_id && product_id) {
                int             i;
                //printk(KERN_INFO "%s: vendor_id: %x product_id: %x\n", __FUNCTION__, vendor_id, product_id);
                for (i = 0; i < (sizeof(id_table) / sizeof(struct usb_device_id) - 1); i++) {

                        if (id_table[i].idVendor == vendor_id && id_table[i].idProduct == product_id) {

                                //printk(KERN_INFO "%s: vendor_id: %x product_id: %x already in table\n",
                                //                __FUNCTION__, vendor_id, product_id);
                                break;
                        }
                        if (!id_table[i].idVendor && !id_table[i].idProduct) {
                                //printk(KERN_INFO "%s: vendor_id: %x product_id: %x inserted into table\n",
                                //                __FUNCTION__, vendor_id, product_id);
                                id_table[i].match_flags = USB_DEVICE_ID_MATCH_DEVICE;
                                id_table[i].idVendor = vendor_id;
                                id_table[i].idProduct = product_id;
                                id_table[i].bDeviceClass = class;
                                id_table[i].bDeviceSubClass = subclass;
                                break;
                        }
                }
        }

        // register us with the usb core layer
        if (usb_register(&usblan_driver)) {
                return -EINVAL;
        }

        return 0;
}


/*
 * function_exit - module cleanup
 *
 */
STATIC void __exit usbdnet_modexit(void) {
        // de-register from the usb core layer
        usb_deregister(&usblan_driver);
}

module_init(usbdnet_modinit);
module_exit(usbdnet_modexit);
