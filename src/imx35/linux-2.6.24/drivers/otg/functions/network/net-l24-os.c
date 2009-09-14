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
 * otg/functions/network/net-l24-os.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/net-l24-os.c|20070814184652|52070
 *
 *      Copyright (c) 2002-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */
/*
 * @file otg/functions/network/net-l24-os.c
 * @brief The Linux 2.4 OS specific upper edge (network interface)
 * implementation for the Network Function Driver.
 *
 * This file implements a standard Linux network driver interface and
 * the standard Linux 2.4 module init and exit functions.
 *
 * If compiled into the kernel, this driver can be used with NFSROOT to
 * provide the ROOT filesystem. Please note that the kernel NFSROOT support
 * (circa 2.4.20) can have problems if there are multiple interfaces. So
 * it is best to ensure that there are no other network interfaces compiled
 * in.
 * @ingroup NetworkFunction
 * @ingroup LINUXOS
 */


#include <otg/otg-compat.h>

#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

/* OS-specific #includes */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/in.h>
#include <linux/inetdevice.h>


/* Belcarra public interfaces */
#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>

/* network private interfaces */
#include "network.h"
#include "net-os.h"

#define TRACE_VERBOSE 0

MOD_AUTHOR ("sl@belcarra.com, tbr@belcarra.com, balden@belcarra.com");

EMBED_LICENSE();

MOD_DESCRIPTION ("USB Network Function");
EMBED_USBD_INFO ("network_fd 2.0-beta");

MOD_PARM_STR (local_dev_addr, "Local Device Address", NULL);
MOD_PARM_STR (remote_dev_addr, "Remote Device Address", NULL);
MOD_PARM_BOOL (override_mac, "Override MAC address", 0);

MOD_PARM_BOOL (personal_device, "Do not implement an infrastructure device", FALSE);
MOD_PARM_BOOL (infrastructure_device, "Implement an infrastructure device", FALSE);

int blan_mod_init(void);
void blan_mod_exit(void);

int basic_mod_init(void);
void basic_mod_exit(void);

int basic2_mod_init(void);
void basic2_mod_exit(void);

int ecm_mod_init(void);
void ecm_mod_exit(void);

int safe_mod_init(void);
void safe_mod_exit(void);

int eem_mod_init(void);
void eem_mod_exit(void);

#define NTT network_fd_trace_tag
otg_tag_t network_fd_trace_tag;
wait_queue_head_t usb_netif_wq;
#ifdef CONFIG_OTG_NET_NFS_SUPPORT
int usb_is_configured;
#endif

/* Module Parameters ************************************************************************* */


/* End of Module Parameters ****************************************************************** */

static u8 zeros[ETH_ALEN];

/* Prevent overlapping of bus administrative functions:
 *
 *      network_function_enable
 *      network_function_disable
 *      network_hard_start_xmit
 */
DECLARE_MUTEX(usbd_network_sem);

struct net_device Network_net_device;
struct net_device_stats Network_net_device_stats;  /* network device statistics */
struct usb_network_private Network_private;

void notification_schedule_bh (void);
int network_urb_sent_int (struct usbd_urb *urb, int urb_rc);

static u32 network_router_ip = 0;

extern void usbd_write_info_message(struct usbd_function_instance*, char *msg);


//_________________________________________________________________________________________________

/*
 * Synchronization
 *
 *
 * Notification bottom half
 *
 *  This is a scheduled task that will send an interrupt notification. Because it
 *  is called from the task scheduler context it needs to verify that the device is
 *  still usable.
 *
 *      static int network_send_int_blan(struct usbd_simple_instance *, int )
 *      static void notification_bh (void *)
 *      void notification_schedule_bh (void)
 *
 *
 * Netdevice functions
 *
 *   These are called by the Linux network layer. They must be protected by irq locks
 *   if necessary to prevent interruption by IRQ level events.
 *
 *      int network_init (struct net_device *net_device)
 *      void network_uninit (struct net_device *net_device)
 *      int network_open (struct net_device *net_device)
 *      int network_stop (struct net_device *net_device)
 *      struct net_device_stats *network_get_stats (struct net_device *net_device)
 *      int network_set_mac_addr (struct net_device *net_device, void *p)
 *      void network_tx_timeout (struct net_device *net_device)
 *      int network_set_config (struct net_device *net_device, struct ifmap *map)
 *      int network_stop (struct net_device *net_device)
 *      int network_hard_start_xmit (struct sk_buff *skb, struct net_device *net_device)
 *      int network_do_ioctl (struct net_device *net_device, struct ifreq *rp, int cmd)
 *
 *
 * Data bottom half functions
 *
 *  These are called from the bus bottom half handler.
 *
 *      static int network_recv (struct usb_network_private *, struct net_device *, struct sk_buff *)
 *      int network_recv_urb (struct usbd_urb *)
 *      int network_urb_sent (struct usbd_urb *, int )
 *
 *
 * Hotplug bottom half:
 *
 *  This is a scheduled task that will send do a hotplug call. Because it is
 *  called from the task scheduler context it needs to verify that the
 *  device is still usable.
 *
 *      static int hotplug_attach (u32 ip, u32 mask, u32 router, int attach)
 *      static void hotplug_bh (void *data)
 *      void net_os_hotplug (void)
 *
 *
 * Irq level functions:
 *
 *   These are called at interrupt time do process or respond to USB setup
 *   commands.
 *
 *      int network_device_request (struct usbd_device_request *)
 *      void network_event_handler (struct usbd_simple_instance *function, usbd_device_event_t event, int data)
 *
 *
 * Enable and disable functions:
 *
 *      void network_function_enable (struct usbd_simple_instance *, struct usbd_simple_instance *)
 *      void network_function_disable (struct usbd_simple_instance *function)
 *
 *
 * Driver initialization and exit:
 *
 *      static  int network_create (struct usb_network_private *)
 *      static  void network_destroy (struct usb_network_private *)
 *
 *      int network_modinit (void)
 *      void network_modexit (void)
 */


//_______________________________USB part Functions_________________________________________

/*! net_os_mutex_enter - enter mutex region
 * @param function_instance - pointer to function instance
 * @return  none
 */
void net_os_mutex_enter(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        down(&usbd_network_sem);
}

/* net_os_mutex_exit - exit mutex region
 * @param function_instance  pointer to function instance
 * @return none
 */
void net_os_mutex_exit(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        up(&usbd_network_sem);
}

/*! notification_bh - Bottom half handler to send a notification status
 *
 * Send a notification with open/close status
 *
 * It should not be possible for this to be called more than once at a time
 * as it is only called via schedule_task() which protects against a second
 * invocation.
 *
 * @param data pointer to usbd_function_instance
 * @return none
 */
STATIC void *notification_bh (void *data)
{
        struct usb_network_private *npd = (struct usb_network_private *) data;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;
        // XXX unsigned long flags;
        RETURN_NULL_UNLESS(npd);
        // XXX local_irq_save(flags);
        net_fd_send_int_notification(function_instance, npd->flags & NETWORK_OPEN, 0);
        // XXX local_irq_restore(flags);
        return NULL;
}

/* notification_schedule_bh - schedule a call for notification_bh
 *
 * @param function_instance  pointer to function instance
 * @reutrn none
 */
void net_os_send_notification_later(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        otg_workitem_start(npd->notification_workitem);
}

/*! net_os_xmit_done - called from USB part when a transmit completes, good or bad.
 *
 * @param function_instance pointer to function instance
 * @param data pointer passed to fd_ops->start_ximit()
 * @param tx_rc urb transmit result code, which is USBD_URB_ERROR, USBD_URB_CANCELLED or...
 * @return non-zero only if network does not exist, ow 0.
 */
int net_os_xmit_done(struct usbd_function_instance *function_instance, void *data, int tx_rc)
{
        struct sk_buff *skb = (struct sk_buff *) data;
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct net_device *net_device = npd ? npd->net_device : NULL;
        int rc = 0;

        //TRACE_MSG4(NTT, "function: %x npd: %x skb: %x", function_instance, npd, skb, net_device);

        RETURN_ZERO_UNLESS(skb);
        dev_kfree_skb_any(skb);

        RETURN_ZERO_UNLESS(net_device);
        if ( net_os_queue_stopped(function_instance))
                net_os_wake_queue(function_instance);

        RETURN_ZERO_UNLESS(npd);

        switch (tx_rc) {
        case USBD_URB_ERROR:
                npd->net_device_stats->tx_errors++;
                npd->net_device_stats->tx_dropped++;
                break;
        case USBD_URB_CANCELLED:
                npd->net_device_stats->tx_errors++;
                npd->net_device_stats->tx_carrier_errors++;
                break;
        default:
                break;
        }

        return 0;
}

/*! net_os_dealloc_buffer - deallocate data buffer
 * @param function_instance pointer to function instance
 * @param data  pointer to data buffer to deallocate
 * @param buffer * @return none
 * @return none
 */
void net_os_dealloc_buffer(struct usbd_function_instance *function_instance, void *data, void *buffer)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb = data;
        RETURN_UNLESS(skb);
        dev_kfree_skb_any(skb);
        RETURN_UNLESS(npd);
        npd->net_device_stats->rx_dropped++;
}

/*! net_os_alloc_buffer - allocate a buffer
 * @param function_instance  pointer to function instance
 * @param cp
 * @param n
 * @return allocated buffer of void pointer type
 */
void *net_os_alloc_buffer(struct usbd_function_instance *function_instance, u8 **cp, int n)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb;

        /* allocate skb of appropriate length, reserve 2 to align ip
         */
        RETURN_NULL_UNLESS ((skb = dev_alloc_skb(n+32)));
        //TRACE_MSG1(NTT, "skb: %x", skb);

        skb_reserve(skb, 4);
        while (((int)skb->data) & 0xf)
                skb_reserve(skb, 1);
        *cp = skb_put(skb, n);



        //TRACE_MSG7(NTT, "skb: %x head: %x data: %x tail: %x end: %x len: %d tail-data: %d",
        //                skb, skb->head, skb->data, skb->tail, skb->end, skb->len, skb->tail - skb->data);

        return (void*) skb;
}

/*! network_recv - function to process an received data URB
 *
 * Passes received data to the network layer. Passes skb to network layer.
 * @param function_instance  pointer to function instance
 * @param net_device  pointer to current net device
 * @param skb pointer to sk_buff struct
 *
 *
 * @return non-zero for failure.
 */
STATIC __inline__ int network_recv (struct usbd_function_instance *function_instance,
                struct net_device *net_device, struct sk_buff *skb)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int rc;

        RETURN_ZERO_UNLESS(npd);
#if 0
        printk(KERN_INFO"%s: len: %x head: %p data: %p tail: %p\n", __FUNCTION__,
                        skb->len, skb->head, skb->data, skb->tail);
        {
                u8 *cp = skb->data;
                int i;
                for (i = 0; i < skb->len; i++) {
                        if ((i%32) == 0) {
                                printk("\nrx[%2x] ", i);
                        }
                        printk("%02x ", *cp++);
                }
                printk("\n");
        }
#endif

#if 0
        TRACE_MSG4(NTT, "len: %x head: %p data: %p tail: %p",
                        skb->len, skb->head, skb->data, skb->tail);
        {
                int i;

                for (i = 0; i < skb->len; i += 8) {
                        u8 *cp = skb->data + i;
                        TRACE_MSG8(NTT, "[ %02x %02x %02x %02x %02x %02x %02x %02x]",
                                        cp[0], cp[1],
                                        cp[2], cp[3],
                                        cp[4], cp[5],
                                        cp[6], cp[7]
                                  );
                }
        }
#endif

        /* refuse if no device present
         */
        if (!netif_device_present (net_device)) {
                TRACE_MSG0(NTT,"device not present");
                printk(KERN_INFO"%s: device not present\n", __FUNCTION__);
                return -EINVAL;
        }

        /* refuse if no carrier
         */
        if (!netif_carrier_ok (net_device)) {
                TRACE_MSG0(NTT,"no carrier");
                printk(KERN_INFO"%s: no carrier\n", __FUNCTION__);
                return -EINVAL;
        }

        /* refuse if the net device is down
         */
        if (!(net_device->flags & IFF_UP)) {
                TRACE_MSG1(NTT,"USB Net interface is not up net_dev->flags: %x", net_device->flags);
//                printk(KERN_INFO"%s: USB Net interface is not up net_dev->flags: %x\n", __FUNCTION__, net_device->flags);
                //npd->net_device_stats->rx_dropped++;
                return -EINVAL;
        }

        skb->dev = net_device;
        skb->pkt_type = PACKET_HOST;
        skb->protocol = eth_type_trans (skb, net_device);
        skb->ip_summed = CHECKSUM_UNNECESSARY;

        /* pass it up to kernel networking layer
         */
        if ((rc = netif_rx (skb)))
                TRACE_MSG1(NTT,"netif_rx rc: %d", rc);

        npd->net_device_stats->rx_bytes += skb->len;
        npd->net_device_stats->rx_packets++;

        return 0;
}

/*! net_os_recv_buffer - forward a received URB, or clean up after a bad one.
 * @param function_instance pointer to function instance
 * @param    os_data     == NULL --> just accumulate stats (count 1 bad buff)
 * @param    os_buffer
 * @param    crc_bad  != 0     --> count a crc error and free data/skb
 * @param    length
 * @param    trim     --> amount to trim from valid skb
 * @return  non-zero for error
 */
int net_os_recv_buffer(struct usbd_function_instance *function_instance, void *os_data,
                void *os_buffer, int crc_bad, int length, int trim)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb = (struct sk_buff *) os_data;

        RETURN_EAGAIN_UNLESS(npd);

        //TRACE_MSG7(NTT, "skb: %x head: %x data: %x tail: %x end: %x len: %d tail-data: %d",
        //                skb, skb->head, skb->data, skb->tail, skb->end, skb->len, skb->tail - skb->data);

        //TRACE_MSG3(NTT, "npd: %x skb: %x flags: %04x", npd, skb, npd->flags);

        UNLESS (skb) {
                /* Lower layer never got around to allocating an skb, but
                 * needs to count a packet it can't forward.
                 */
                npd->net_device_stats->rx_frame_errors++;
                npd->net_device_stats->rx_errors++;
                TRACE_MSG0(NTT, "NO SKB");
                return -EAGAIN;
        }

#if 0
        /* There is an skb, either forward it or free it.
         */
        if (crc_bad) {
                npd->net_device_stats->rx_crc_errors++;
                npd->net_device_stats->rx_errors++;
                TRACE_MSG0(NTT, "BAD CRC");
                return -EAGAIN;
        }
#endif
        /* is the network up?
         */
        UNLESS (npd->net_device) {

                // Something wrong, free the skb
                //dev_kfree_skb_any (skb);
                // The received buffer didn't get forwarded, so...
                npd->net_device_stats->rx_dropped++;
                TRACE_MSG0(NTT, "NO NETWORK");
                return -EAGAIN;
        }

        /* Trim if necessary and pass up
         */
        if (trim)
                skb_trim(skb, skb->len - trim);

        // 6     6     2     46
        // 802.3 requires minimum 46 bytes of data,
        //

        return network_recv(function_instance, npd->net_device, skb);
}

extern void *config_bh (void *data);
STATIC void *hotplug_bh (void *data);

/*! net_os_enable - called to enable network device
 * @param function_instance pointer to function instance
 *
 * @return none
 *
 */
void net_os_enable(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = &Network_private;

        function_instance->privdata = npd;

        TRACE_MSG3(NTT,"npd: %p function: %p, f->p: %p", npd, function_instance, function_instance->privdata);

        memset(&Network_net_device_stats, 0, sizeof Network_net_device_stats);
        npd->function_instance = function_instance;
        npd->max_queued_frames = 10;
        npd->max_queued_bytes = 20000;

        #if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
         THROW_UNLESS((npd->config_otgtask = otg_task_init2("netcfg", config_bh, npd, NTT)), error);
        //npd->config_otgtask->debug = TRUE;
        otg_task_start(npd->config_otgtask);
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */

        #ifdef CONFIG_OTG_NETWORK_HOTPLUG
        THROW_UNLESS(npd->hotplug_workitem = otg_workitem_init("hotplug", hotplug_bh, npd, NTT), error);
        //npd->hotplug_workitem->debug = TRUE;
        #endif /* CONFIG_OTG_NETWORK_HOTPLUG */

        THROW_UNLESS(npd->notification_workitem = otg_workitem_init("netint", notification_bh, npd, NTT), error);
        //npd->notification_workitem->debug = TRUE;

        /* set the network device address from the local device address
         */
        //memcpy(npd->net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);

        CATCH(error) {
                TRACE_MSG0(NTT,"FAILED");

                #if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
                if (npd->config_otgtask) {
                        otg_task_exit(npd->config_otgtask);
                        npd->config_otgtask = NULL;
                }
                #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
                #ifdef CONFIG_OTG_NETWORK_HOTPLUG
                if (npd->hotplug_workitem) otg_workitem_exit(npd->hotplug_workitem);
                npd->hotplug_workitem = NULL;
                #endif /* CONFIG_OTG_NETWORK_HOTPLUG */
                if (npd->notification_workitem) otg_workitem_exit(npd->notification_workitem);
                npd->notification_workitem = NULL;
        }
}

/*! net_os_disable - called to disable netwok device
 * @param function_instance pointer to function instance
 * @return none
 *
 */
extern void net_os_disable(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd && npd->net_device);
        //npd->net_device->priv = NULL;
        //npd->net_device = NULL;
        function_instance->privdata = NULL;
        npd->function_instance = NULL;

        #if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
        if (npd->config_otgtask) {
                        otg_task_exit(npd->config_otgtask);
                        npd->config_otgtask = NULL;
        }
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
        #ifdef CONFIG_OTG_NETWORK_HOTPLUG
        if (npd->hotplug_workitem) otg_workitem_exit(npd->hotplug_workitem);
        npd->hotplug_workitem = NULL;
        #endif /* CONFIG_OTG_NETWORK_HOTPLUG */
        if (npd->notification_workitem) otg_workitem_exit(npd->notification_workitem);
        npd->notification_workitem = NULL;
}

/*! net_os_carrier_on - ???
 * @param  function_instance  pointer to function instance
 * @return none
 *
 */
void net_os_carrier_on(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);
        netif_carrier_on(npd->net_device);
        net_os_wake_queue(function_instance);
#ifdef CONFIG_OTG_NET_NFS_SUPPORT
        RETURN_UNLESS (usb_is_configured);
        wake_up(&usb_netif_wq);
        usb_is_configured = 1;
#endif
         //otg_up_work(npd->config_otgtask);
}

/*! net_os_carrier_off -???
 * @param function_instance  pointer to function instance
 *
 * @return none
 *
 */
void net_os_carrier_off(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);
        net_os_stop_queue(function_instance);
        netif_carrier_off(npd->net_device);
        //otg_up_work(npd->config_otgtask);
}

/*! net_os_queue_stopped --???
 * @param function_instance
 *
 * @return result code
 *
 */
int net_os_queue_stopped(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int rc;

        RETURN_ZERO_UNLESS(npd);
        rc = netif_queue_stopped(npd->net_device);
        //TRACE_MSG3(NTT, "stopped: %d stops: %d restarts: %d", rc, npd->stops, npd->restarts);
        return rc;
}

/*! net_os_wake_queue -???
 * @param function_instance function instance pointer
 * @return none
 *
 */
void net_os_wake_queue(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);
        //TRACE_MSG3(NTT, "stopped: %d stops: %d restarts: %d", netif_queue_stopped(npd->net_device), npd->stops, npd->restarts);
        netif_wake_queue (npd->net_device);
        npd->restarts++;
}

/*! net_os_stop_queue - ???
 * @param function_instance
 * @return none
 *
 */
void net_os_stop_queue(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);
        //TRACE_MSG3(NTT, "stopped: %d stops: %d restarts: %d", netif_queue_stopped(npd->net_device), npd->stops, npd->restarts);
        netif_stop_queue(npd->net_device);
        npd->stops++;
}

//_______________________________Network Layer Functions____________________________________

/*
 * In general because the functions are called from an independant layer it is necessary
 * to verify that the device is still ok and to lock interrupts out to prevent in-advertant
 * closures while in progress.
 */

/*! network_init - Initializes the specified network device.
 *
 * @param net_device  net_device pointer
 * @return non-zero for failure.
 */
/*! hexdigit -
 *
 * Converts characters in [0-9A-F] to 0..15, characters in [a-f] to 42..47, and all others to 0.
 *
 * @param c - character to convert
 * @return the converted decimal value
 */

extern  u8 hexdigit (char c);
extern void set_address(char *mac_address_str, u8 *dev_addr);


STATIC int network_init (struct net_device *net_device)
{

        TRACE_MSG0(NTT,"no-op");
        set_address(MODPARM(local_dev_addr), net_device->dev_addr);
        return 0;
}


/*! network_uninit   Uninitializes the specified network device.
 * @param net_device network device pointer
 * @return none
 */
STATIC void network_uninit (struct net_device *net_device)
{
        //TRACE_MSG0(NTT,"no-op");
        return;
}


/*! network_open  Opens the specified network device.
 *
 * @param net_device network device pointer
 * @return non-zero for failure.
 */
STATIC int network_open (struct net_device *net_device)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

        // XXX unsigned long flags;

        RETURN_ZERO_UNLESS(function_instance);
        RETURN_ZERO_UNLESS(npd);

        if (npd){

                npd->flags |= NETWORK_OPEN;
                memcpy(net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);

        }
        net_os_wake_queue (function_instance);

        // XXX local_irq_save(flags);
        net_fd_send_int_notification(function_instance, 1, 0);
        // XXX local_irq_restore(flags);

#ifdef CONFIG_OTG_NET_NFS_SUPPORT
        if (!usb_is_configured) {
                if (!in_interrupt()) {
                        printk(KERN_ERR"Please replug USB cable and then ifconfig host interface.\n");
                        interruptible_sleep_on(&usb_netif_wq);
                }
                else {
                        printk(KERN_ERR"Warning! In interrupt\n");
                }
        }
#endif
        return 0;
}


/*! network_stop  Stops the specified network device.
 *
 * @param net_device - pointer to network device
 *
 * @return non-zero for failure.
 */
STATIC int network_stop (struct net_device *net_device)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;
        // XXX unsigned long flags;


        //TRACE_MSG0(NTT,"-");

        RETURN_ZERO_UNLESS(npd);
        npd->flags &= ~NETWORK_OPEN;
        // XXX local_irq_save(flags);
        net_fd_send_int_notification(function_instance, 0, 0);
        // XXX local_irq_restore(flags);

        return 0;
}


/*! network_get_stats  Gets statistics from the specified network device.
 *
 * @param net_device - pointer to network device
 *
 * @returns pointer to net_device_stats structure with the required information.
 */
STATIC struct net_device_stats *network_get_stats (struct net_device *net_device)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

        //if (npd)
        //        return &npd->stats;
        //else
        RETURN_NULL_UNLESS(npd && npd->net_device_stats);
        return npd->net_device_stats;  /* network device statistics */
}


/*! network_set_mac_addr  Sets the MAC address of the specified network device. Fails if the device is in use.
 *
 * @param net_device pointer to network device
 * @param p  - pointer to sockaddr to asign to this device
 *
 * @return non-zero for failure.
 */
STATIC int network_set_mac_addr (struct net_device *net_device, void *p)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;
        struct sockaddr *addr = p;
        // XXX unsigned long flags;

        TRACE_MSG0(NTT,"--");

        RETURN_EBUSY_IF(netif_running (net_device));
        RETURN_ZERO_UNLESS(npd);
        // XXX local_irq_save(flags);
        memcpy(net_device->dev_addr, addr->sa_data, net_device->addr_len);
        memcpy(npd->local_dev_addr, npd->net_device->dev_addr, ETH_ALEN);
        // XXX local_irq_restore(flags);
        return 0;
}


/*! network_tx_timeout  Tells the specified network device that its current transmit attempt has timed out.
 * @param net_device  pointer to network device
 * @return none
 */
STATIC void network_tx_timeout (struct net_device *net_device)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

#if 0
        npd->net_device_stats.tx_errors++;
        npd->net_device_stats.tx_dropped++;
        usbd_cancel_urb_irq (npd->bus, NULL);  // QQSV
#endif
#if 0
        // XXX attempt to wakeup the host...
        if ((npd->network_type == network_blan) && (npd->flags & NETWORK_OPEN)) {
                notification_schedule_bh();
        }
#endif
}


/** network_set_config  Sets the specified network device's configuration. Fails if the device is in use.
 *
 * @param net_device   pointer to network device
 * @param map           An ifmap structure containing configuration values.
 *                      Those values which are non-zero/non-null update the corresponding fields
 *                      in net_device.
 *
 * @returns non-zero for failure.
 */
STATIC int network_set_config (struct net_device *net_device, struct ifmap *map)
{
        RETURN_EBUSY_IF(netif_running (net_device));
        if (map->base_addr)
                net_device->base_addr = map->base_addr;
        if (map->mem_start)
                net_device->mem_start = map->mem_start;
        if (map->irq)
                net_device->irq = map->irq;
        return 0;
}


/*! network_change_mtu  Sets the specified network device's MTU. Fails if the new value is larger and
 * the device is in use.
 *
 * @param net_device pointer to network device
 * @param mtu - transmit unit limit
 * @return non-zero for failure.
 */
STATIC int network_change_mtu (struct net_device *net_device, int mtu)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

        RETURN_EBUSY_IF(netif_running (net_device));
        RETURN_ZERO_UNLESS(npd);
        RETURN_EBUSY_IF(mtu > npd->mtu);
        npd->mtu = mtu;
        return 0;
}

//_________________________________________________________________________________________________
//                                      network_hard_start_xmit

/*! net_os_start_xmit - start sending an skb, with usbd_network_sem already held.
 * @param skb - ???
 * @param net_device pointer to network device
 *
 * @return non-zero (1) if busy. QQSV - this code always returns 0.
 */
STATIC __inline__ int net_os_start_xmit (struct sk_buff *skb, struct net_device *net_device)
{
        struct usb_network_private *npd = (struct usb_network_private *) net_device->priv;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;
        int rc;

        RETURN_ZERO_UNLESS(npd);

        if (!netif_carrier_ok(net_device)) {
                dev_kfree_skb_any (skb);
                npd->net_device_stats->tx_dropped++;
                return 0;
        }

        // stop queue, it will be restarted only when we are ready for another skb
        net_os_stop_queue(function_instance);
        //npd->stopped++;

        // Set the timestamp for tx timeout
        net_device->trans_start = jiffies;

        // XXX request IN, should start a timer to resend this.
        // XXX net_fd_send_int_notification(function_instance, 0, 1);

        if (TRACE_VERBOSE)
                TRACE_MSG8(NTT,"SKB: %p head: %p data: %p tail: %p end: %p headroom: %d len: %d tailroom: %d",
                                skb, skb->head, skb->data, skb->tail, skb->end,
                                (skb->data - skb->head), (skb->tail - skb->data), (skb->end - skb->tail));

        switch ((rc = net_fd_start_xmit(function_instance, skb->data, skb->len, (void*)skb))) {
        case 0:
                TRACE_MSG1(NTT,"OK: %d", rc);
                npd->net_device_stats->tx_packets++;
                npd->net_device_stats->tx_bytes += skb->len;
                if ((atomic_read(&npd->queued_frames) < npd->max_queued_frames) &&
                                (atomic_read(&npd->queued_bytes) < npd->max_queued_bytes))
                        net_os_wake_queue (function_instance);
                break;

        case -EINVAL:
        case -EUNATCH:
                TRACE_MSG1(NTT,"not attached, send failed: %d", rc);
                printk(KERN_ERR"%s: not attached, send failed: %d\n", __FUNCTION__, rc);
                npd->net_device_stats->tx_errors++;
                npd->net_device_stats->tx_carrier_errors++;
                net_os_wake_queue(function_instance);
                break;

        case -ENOMEM:
                TRACE_MSG1(NTT,"no mem, send failed: %d", rc);
                printk(KERN_ERR"%s: no mem, send failed: %d\n", __FUNCTION__, rc);
                npd->net_device_stats->tx_errors++;
                npd->net_device_stats->tx_fifo_errors++;
                net_os_wake_queue(function_instance);
                break;

        case -ECOMM:
                TRACE_MSG2(NTT,"comm failure, send failed: %d %p", rc, net_device);
                printk(KERN_ERR"%s: comm failure, send failed: %d %p\n", __FUNCTION__, rc, net_device);
                // Leave the IF queue stopped.
                npd->net_device_stats->tx_dropped++;
                break;

        }
        if (0 != rc) {
                dev_kfree_skb_any (skb);
                // XXX this is what we should do, blows up on some 2.4.20 kernels
                // return(NET_XMIT_DROP);
                return 0;
        }
        return 0;
}

/*!
 * network_hard_start_xmit - start sending an skb.  Called by the OS network layer.
 * @param skb - ???
 * @param net_device pointer to network device
 *
 * @return non-zero (1) if busy. QQSV - this code always returns 0.
 */
STATIC int network_hard_start_xmit (struct sk_buff *skb, struct net_device *net_device)
{
        int rc = 0;
        //down(&usbd_network_sem);
        rc = net_os_start_xmit(skb, net_device);
        //up(&usbd_network_sem);
        return rc;
}


/*! network_do_ioctl - perform an ioctl call
 *
 * Carries out IOCTL commands for the specified network device.
 *
 * @param net_device  network device pointer
 * @param rp            Points to an ifreq structure containing the IOCTL parameter(s).
 * @param cmd           The IOCTL command.
 *
 * @return non-zero for failure.
 */
STATIC int network_do_ioctl (struct net_device *net_device, struct ifreq *rp, int cmd)
{
        return -ENOIOCTLCMD;
}

//_________________________________________________________________________________________________
/// @var network_device Network_net_device
///
struct net_device Network_net_device = {
        .get_stats = network_get_stats,
        .tx_timeout = network_tx_timeout,
        .do_ioctl = network_do_ioctl,
        .set_config = network_set_config,
        .set_mac_address = network_set_mac_addr,
        .hard_start_xmit = network_hard_start_xmit,
        .change_mtu = network_change_mtu,
        .init = network_init,
        .uninit = network_uninit,
        .open = network_open,
        .stop = network_stop,
        .priv = NULL,
        .name =  "usbl0",
};


//_________________________________________________________________________________________________
/*! net_os_settime - set os time
 *
 * @param function_instance  function_insance pointer
 * @param rfc868seconds
 * @return none
 */
void net_os_settime(struct usbd_function_instance *function_instance, u32 rfc868seconds)
{
        #if defined(LINUX24)
        struct timeval net_fd_tv;
        #else
        struct timespec net_fd_tv;
        #endif
        /* wIndex and wLength contain RFC868 time - seconds since midnight 1
         * jan 1900 and convert to Unix time - seconds since midnight 1 jan
         * 1970
         */
        TRACE_MSG1(NTT, "RFC868 seconds: %d", rfc868seconds);
        memset(&net_fd_tv, 0, sizeof(net_fd_tv));
        net_fd_tv.tv_sec = rfc868seconds;
        net_fd_tv.tv_sec -= RFC868_OFFSET_TO_EPOCH;
        do_settimeofday(&net_fd_tv);
}

//_________________________________________________________________________________________________

#if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
int local_devinet_ioctl(unsigned int cmd, void __user *data);
/*! sock_ioctl - perform an ioctl call to inet device
 * @param cmd - io control command
 * @param ifreq -
 * @return result code
 */
static int sock_ioctl(u32 cmd, struct ifreq *ifreq)
{
        int rc = 0;
        mm_segment_t save_get_fs = get_fs();
        TRACE_MSG1(NTT, "cmd: %x", cmd);
        set_fs(get_ds());
        rc = local_devinet_ioctl(cmd, ifreq);
        set_fs(save_get_fs);
        return rc;
}

/*! sock_addr - setup a socket address for specified interface
 * @param ifname - interface name
 * @param cmd - ioctl command
 * @param s_addr socket address
 * @return io control operation result code
 */
static int sock_addr(char * ifname, u32 cmd, u32 s_addr)
{
        struct ifreq ifreq;
        struct sockaddr_in *sin = (void *) &(ifreq.ifr_ifru.ifru_addr);

        TRACE_MSG2(NTT, "ifname: %s addr: %x", ifname, ntohl(s_addr));

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname);

        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = s_addr;

        return sock_ioctl(cmd, &ifreq);
}


/*! sock_flags - set flags for specified interface
 * @param ifname - interface name
 * @param oflags
 * @param sflags
 * @param rflags
 * @return int as result code
 */
static int sock_flags(char * ifname, u16 oflags, u16 sflags, u16 rflags)
{
        int rc = 0;
        struct ifreq ifreq;

        TRACE_MSG4(NTT, "ifname: %s oflags: %x s_flags: %x r_flags: %x", ifname, oflags, sflags, rflags);

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname);

        oflags |= sflags;
        oflags &= ~rflags;
        ifreq.ifr_flags = oflags;

        TRACE_MSG1(NTT, "-> ifr_flags: %x ", ifreq.ifr_flags);

        THROW_IF ((rc = sock_ioctl(SIOCSIFFLAGS, &ifreq)), error);

        TRACE_MSG1(NTT, "<- ifr_flags: %x ", ifreq.ifr_flags);

        CATCH(error) {
                TRACE_MSG1(NTT, "ifconfig: cannot get/set interface flags (%d)", rc);
                return rc;
        }
        return rc;
}

/*! network_attach - called to configure interface dnd evice is attached/deatched
 *
 * This will use socket calls to configure the interface to the supplied
 * ip address and make it active.
 *
 * @param function_instance  function instance pointer
 * @param ip  ip address
 * @param make network maks
 * @param router router ip address
 * @return 0 on success
 */
STATIC int network_attach(struct usbd_function_instance *function_instance, u32 ip, u32 mask, u32 router, int attach)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int err = 0;


        RETURN_ZERO_UNLESS(npd);

        if (attach) {
                u16 oflags = (npd && npd->net_device) ? npd->net_device->flags : 0;
                TRACE_MSG5(NTT, "ATTACH npd: %x ip: %08x mask: %08x router: %08x attach: %d", npd, ip, mask, router, attach);
                usbd_write_info_message(function_instance, "NETWORK: ATTACH");

                if (npd->net_device)
                        memcpy(npd->net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);

                /* setup ip address, netwask, and broadcast address */
                if (ip) {
                        char buf[40];
                        THROW_IF ((err = sock_addr("usbl0", SIOCSIFADDR, htonl(ip))), error);
                        if (mask) {
                                THROW_IF ((err = sock_addr("usbl0", SIOCSIFNETMASK, htonl(mask))), error);
                                THROW_IF ((err = sock_addr("usbl0", SIOCSIFBRDADDR, htonl(ip | ~mask))), error);
                        }
                        /* bring the interface up */
                        THROW_IF ((err = sock_flags("usbl0", oflags, IFF_UP, 0)), error);

                        snprintf(buf, sizeof(buf), "NETWORK: usbl0 %d.%d.%d.%d",
                                        ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip  & 0xff);

                        usbd_write_info_message(function_instance, buf);
                }
        }
        else {
                u16 oflags = (npd && npd->net_device) ? npd->net_device->flags : 0;
                TRACE_MSG5(NTT, "DETACH npd: %x ip: %08x mask: %08x router: %08x attach: %d", npd, ip, mask, router, attach);
                usbd_write_info_message(function_instance, "NETWORK: DETACH");
                /* bring the interface down */
                THROW_IF ((err = sock_addr("usbl0", SIOCSIFADDR, htonl(0))), error);
                THROW_IF ((err = sock_flags("usbl0", oflags, 0, IFF_UP)), error);
        }

        /* XXX needs to be fixed for multi-interface */
        network_router_ip = npd->router_ip;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                TRACE_MSG1(NTT, "ifconfig: cannot configure interface (%d)", err);
                return err;
        }
        return 0;
}

/*! config_bh() - configure network
 *
 * Check connected status and load/unload as appropriate.
 * @param data pointer to function instance
 * @return none
 */
void *config_bh (void *data)
{
        struct usb_network_private *npd = (struct usb_network_private *) data;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

        RETURN_NULL_UNLESS(npd);

        TRACE_MSG5(NTT, "function: %x enabled: %x status: %x state: %x npd->config_status: %d",
                        function_instance,
                        npd->flags & NETWORK_ENABLED,
                        usbd_get_device_status(function_instance),
                        usbd_get_device_state(function_instance),
                        npd->config_status);

#if 1
        while (TRUE) {

                if (npd->flags & NETWORK_CONFIGURED)

                {
                        TRACE_MSG0(NTT,"call npd->net_start_recv");
                        npd->net_start_recv(function_instance);
                        TRACE_MSG2(NTT,"BUS state: %d status: %d", usbd_get_device_state(function_instance),
                                        usbd_get_device_status(function_instance));
                        if ((config_attached != npd->config_status) && npd->ip_addr) {
                                network_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 1);
                                TRACE_MSG0(NTT,"ATTACH");
                                npd->config_status = config_attached;
                                continue;
                        }
                }
                else
                {
                        if (config_detached != npd->config_status) {

                                network_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 0);
                                TRACE_MSG0(NTT,"DETACH");
                                npd->config_status = config_detached;
                                continue;
                        }

                }
                break;
        }
        //sleep(1);
        return NULL;

#else
        if ((npd->flags & NETWORK_ENABLED ) &&
                        (function_instance && (USBD_OK == usbd_get_device_status(function_instance))
                         && (STATE_CONFIGURED == usbd_get_device_state(function_instance)) && npd->ip_addr))
        {
                TRACE_MSG2(NTT,"BUS state: %d status: %d", usbd_get_device_state(function_instance),
                                usbd_get_device_status(function_instance));


                if (config_attached != npd->config_status) {
                        TRACE_MSG0(NTT,"ATTACH");
                        npd->config_status = config_attached;
                        network_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 1);
                }
                return NULL;
        }


        if (config_detached != npd->config_status) {
                TRACE_MSG0(NTT,"DETACH");
                npd->config_status = config_detached;
                network_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 0);
        }
        return NULL;
#endif
}

/*!
 * net_os_config - schedule a call to config bottom half
 *
 * @param function_instance  pointer to function instance
 *
 * @return none
 */
void net_os_config(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        TRACE_MSG0(NTT, "CONFIG");
        RETURN_UNLESS(npd);
        //do_settime = FALSE;
        otg_up_work(npd->config_otgtask);
}
#else /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
void net_os_config(struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(NTT, "NOT ENABLED");
        // Do nothing, config not enabled.
}
#endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
//______________________________________ Hotplug Functions ________________________________________

#ifdef CONFIG_OTG_NETWORK_HOTPLUG

#define AGENT "network_fd"
#ifdef CONFIG_OTG_NETWORK_HOTPLUG_PATH
static char hotplug_path[] =  CONFIG_OTG_NETWORK_HOTPLUG_PATH;
#else
static char hotplug_path[]="";
#endif

/*! hotplug_attach - call hotplug
 *
 * @param function_instance  function  instance pointer
 * @param ip  ip address
 * @param mask   maks addrss
 * @param router router address
 * @param attach  operation indicator for attach/deatch
 * @return 0 on success
 */
STATIC int hotplug_attach(struct usbd_function_instance *function_instance, u32 ip, u32 mask, u32 router, int attach)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        static int count = 0;
        char *argv[3];
        char *envp[10];
        char ifname[20+12 + IFNAMSIZ];
        int i;
        char count_str[20];

        RETURN_EINVAL_UNLESS(npd);
        RETURN_EINVAL_IF(!hotplug_path[0]);

        argv[0] = hotplug_path;
        argv[1] = AGENT;
        argv[2] = 0;

        sprintf (ifname, "INTERFACE=%s", npd->net_device->name);
        sprintf (count_str, "COUNT=%d", count++);

        i = 0;
        envp[i++] = "HOME=/";
        envp[i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
        envp[i++] = ifname;


        if (attach) {
                unsigned char *cp;
                char ip_str[20+32];
                char mask_str[20+32];
                char router_str[20+32];
                u32 nh;

                nh = htonl(ip);
                cp = (unsigned char*) &nh;
                sprintf (ip_str, "IP=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                nh = htonl(mask);
                cp = (unsigned char*) &nh;
                sprintf (mask_str, "MASK=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                nh = htonl(router);
                cp = (unsigned char*) &nh;
                sprintf (router_str, "ROUTER=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                //TRACE_MSG3(NTT, "attach %s %s %s", ifname, ip_str, count_str);

                envp[i++] = "ACTION=attach";
                envp[i++] = ip_str;
                envp[i++] = mask_str;
                envp[i++] = router_str;

                TRACE_MSG3(NTT, "attach ip: %08x mask: %08x router: %08x", ip, mask, router);
        }
        else {
                //TRACE_MSG2(NTT,"detach %s %s", ifname, count_str);
                envp[i++] = "ACTION=detach";
                TRACE_MSG3(NTT, "detach ip: %08x mask: %08x router: %08x", ip, mask, router);
        }

        envp[i++] = count_str;
        envp[i++] = 0;

        return call_usermodehelper (argv[0], argv, envp,0);
}


/*! hotplug_bh - bottom half handler to call hotplug script to signal ATTACH or DETACH
 *
 * Check connected status and load/unload as appropriate.
 *
 * It should not be possible for this to be called more than once at a time
 * as it is only called via schedule_task() which protects against a second
 * invocation.
 *
 * @param data - pointer to data to pass to bottom half handler
 * @return none
 */
STATIC void *hotplug_bh (void *data)
{
        struct usb_network_private *npd = (struct usb_network_private *) data;
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) npd->function_instance;

        RETURN_NULL_UNLESS(npd);

        function_instance = (struct usbd_function_instance *)npd->function_instance;

        TRACE_MSG2(NTT, "function: %x npd->hotplug_status: %d", function_instance, npd->hotplug_status);


        if ((npd->flags & NETWORK_ENABLED ) && (function_instance && (USBD_OK == usbd_get_device_status(function_instance))
                                && (STATE_CONFIGURED == usbd_get_device_state(function_instance))))
        {
                TRACE_MSG2(NTT,"BUS state: %d status: %d", usbd_get_device_state(function_instance),
                                usbd_get_device_status(function_instance));
                if (hotplug_attached != npd->hotplug_status) {
                        TRACE_MSG0(NTT,"ATTACH");
                        npd->hotplug_status = hotplug_attached;
                        hotplug_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 1);
                }
                return NULL;
        }

        if (hotplug_detached != npd->hotplug_status) {
                TRACE_MSG0(NTT,"DETACH");
                npd->hotplug_status = hotplug_detached;
                hotplug_attach (function_instance, npd->ip_addr, npd->network_mask, npd->router_ip, 0);
        }
        return NULL;
}

/*!
 * net_os_hotplug - schedule a call to hotplug bottom half
 * @param function_instance function instance pointer
 * @return none
 */
void net_os_hotplug(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd =
                (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        #if 0
        SET_WORK_ARG(npd->hotplug_bh, function_instance);
        SCHEDULE_WORK(npd->hotplug_bh);
        #else
        otg_workitem_start(npd->hotplug_workitem);
        #endif
}
#else /* CONFIG_OTG_NETWORK_HOTPLUG */
void net_os_hotplug(struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(NTT, "NOT ENABLED");
        // Do nothing, hotplug not configured.
}
#endif /* CONFIG_OTG_NETWORK_HOTPLUG */

//_________________________________________________________________________________________________

/*! network_create - create and initialize network private structure
 *
 * @returns non-zero for failure.
 */
STATIC int network_create(void)
{
        TRACE_MSG0(NTT,"entered");

        /* Set some fields to generic defaults and register the network device with the kernel networking code */

        Network_private.net_device = &Network_net_device;
        Network_private.net_device_stats = &Network_net_device_stats;
        Network_net_device.priv = &Network_private;

        ether_setup(&Network_net_device);
        RETURN_EINVAL_IF (register_netdev(&Network_net_device));

        netif_stop_queue(&Network_net_device);
        netif_carrier_off(&Network_net_device);
        Network_net_device.flags &= ~IFF_UP;


        TRACE_MSG0(NTT,"finis");
        return 0;
}

/*! network_destroy - destroy network private struture
 *
 * Destroys the network interface referenced by the global variable @c Network_net_device.
 */
STATIC void network_destroy(void)
{
        netif_stop_queue(&Network_net_device);
        netif_carrier_off(&Network_net_device);
        unregister_netdev(&Network_net_device);

        Network_private.net_device = NULL;
        Network_private.net_device_stats = NULL;
        Network_net_device.priv = NULL;
}


//______________________________________module_init and module_exit________________________________
#if defined(CONFIG_PROC_FS) && \
        (defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG))

/*! network_proc_read_ip - implement proc file system read.
 *  Standard proc file system read function.
 *
 *  @param file
 *  @param buf -storage for read ip value
 *  @param count
 *  @param pos position from which to read
 *  @return read lenth
 */
static ssize_t network_proc_read_ip (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        char *bp;
        int index = (*pos)++;

        RETURN_ZERO_IF(index);

        // get a page, max 4095 bytes of data...
        RETURN_ENOMEM_UNLESS ((page = GET_KERNEL_PAGE()));

        bp = (char *) page;

        len = sprintf(bp, "%d.%d.%d.%d\n",
                        (network_router_ip >> 24) & 0xff,
                        (network_router_ip >> 16) & 0xff,
                        (network_router_ip >> 8) & 0xff,
                        (network_router_ip) & 0xff
                        );

        if (copy_to_user(buf, (char *) page, len))
                len = -EFAULT;

        free_page (page);
        return len;
}

/*! @var struct file_operations otg_message_proc_swithch_functions
 */
static struct file_operations otg_message_proc_switch_functions = {
      read: network_proc_read_ip,
};
#endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */



/*! network_modinit - driver intialization,
 *
 * @returns non-zero for failure.
 */
STATIC int network_modinit (void)
{
        #if defined(CONFIG_PROC_FS) && \
                (defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG))
        struct proc_dir_entry *p;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */

        BOOL blan = FALSE, basic = FALSE, basic2 = FALSE, ecm = FALSE, eem = FALSE,
             safe = FALSE, net_fd = FALSE, rnddis = FALSE, procfs = FALSE;

        NTT = otg_trace_obtain_tag(NULL, "network-if");
        UNLESS (blan = BOOLEAN(!blan_mod_init()))  blan_mod_exit();
        UNLESS (basic = BOOLEAN(!basic_mod_init())) basic_mod_exit();
        UNLESS (basic2 = BOOLEAN(!basic2_mod_init())) basic2_mod_exit();
        UNLESS (ecm = BOOLEAN(!ecm_mod_init())) ecm_mod_exit();
        UNLESS (safe = BOOLEAN(!safe_mod_init())) safe_mod_exit();
        THROW_UNLESS (eem = BOOLEAN(!eem_mod_init()), error);
        /* do we have something enabled? */
        THROW_UNLESS ((eem || blan || basic || basic2 || ecm || safe),error);

        /* verify that they didn't attempt to override both personal and infrastructure override */
        THROW_IF (MODPARM(personal_device) && MODPARM(infrastructure_device), error);

        /* check that we have remote address if in infrastructure mode */
        #if defined(OTG_NETWORK_INFRASTRUCTURE)
        THROW_UNLESS(MODPARM(personal_device) || MODPARM(remote_dev_addr), error);
        #else /* defined(OTG_NETWORK_INFRASTRUCTURE) */
        THROW_UNLESS(!MODPARM(infrastructure_device) || MODPARM(remote_dev_addr), error);
        #endif /* defined(OTG_NETWORK_INFRASTRUCTURE) */



        THROW_UNLESS (net_fd = BOOLEAN(!net_fd_init("network",
                                        MODPARM(local_dev_addr),
                                        MODPARM(remote_dev_addr),
                                        MODPARM(override_mac),
                                        MODPARM(personal_device),
                                        MODPARM(infrastructure_device)
                                        )), error);


        init_waitqueue_head(&usb_netif_wq);

        #if defined(CONFIG_PROC_FS) && \
                (defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG))
        if ((p = create_proc_entry ("network_ip", 0666, 0)))  {
                p->proc_fops = &otg_message_proc_switch_functions;
        }
        procfs = TRUE;
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */

        THROW_IF (network_create(), error);

        return 0;

        CATCH(error) {
                if (net_fd) net_fd_exit();

                #if defined(CONFIG_PROC_FS) && \
                        (defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG))
                if (procfs) remove_proc_entry("network_ip", NULL);
                #endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
                otg_trace_invalidate_tag(NTT);
                return -EINVAL;
        }
}

//_____________________________________________________________________________

/*! network_modexit - driver exit
 *
 * Cleans up the module. Deregisters the function driver and destroys the network object.
 */
STATIC void network_modexit (void)
{
        network_destroy();
        net_fd_exit();

        #ifdef CONFIG_PROC_FS
        remove_proc_entry("network_ip", NULL);
        #endif /* CONFIG_PROC_FS */

        blan_mod_exit();
        basic_mod_exit();
        basic2_mod_exit();
        ecm_mod_exit();
        safe_mod_exit();
        eem_mod_exit();
        //rndis_mod_exit();


        otg_trace_invalidate_tag(NTT);
}

//_________________________________________________________________________________________________

module_init (network_modinit);
module_exit (network_modexit);

#endif /* defined(CONFIG_OTG_LNX) */
