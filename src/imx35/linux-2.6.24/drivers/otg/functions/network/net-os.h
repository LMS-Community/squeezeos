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
 * otg/functions/network/net-os.h
 * @(#) sl@belcarra.com|otg/functions/network/net-os.h|20060908200949|09138
 *
 *      Copyright (c) 2002-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */

/*!
 * @file otg/functions/network/net-os.h
 * @brief Structures and function definitions required for implementation
 * of the OS specific upper edge (network interface) for the Network
 * Function Driver.
 *
 * A USB network driver is composed of two pieces:
 *    1) An OS specific piece that handles creating and operating
 *       a network device for the given OS.
 *    2) A USB specific piece that interfaces either with the host
 *       usbcore layer, or with the device usbdcore layer.
 *
 * If the USB piece interfaces with the host usbcore layer you get
 * a network class driver.  If the USB piece interfaces with the usbdcore
 * layer you get a network function driver.
 *
 * This file describes the functions exported by the various net-*-os.c
 * files (implementing (1)) for use in net-fd.c (2).
 *
 *
 * @ingroup NetworkFunction
 */

#ifndef NET_OS_H
#define NET_OS_H 1

/*
 * net_os_mutex_enter - enter mutex region
 */
extern void net_os_mutex_enter(struct usbd_function_instance *);

/*
 * net_os_mutex_exit - exit mutex region
 */
extern void net_os_mutex_exit(struct usbd_function_instance *);

/*
 * net_os_send_notification_later - schedule a callback to the USB part to send
 *                                  an INT notification.
 */
extern void net_os_send_notification_later(struct usbd_function_instance *);

/*
 * net_os_xmit_done - called from USB part when a transmit completes, good or bad.
 *                    tx_rc is SEND_FINISHED_ERROR, SEND_CANCELLED or...
 *                    buff_ctx is the pointer passed to fd_ops->start_xmit().
 * Return non-zero only if network does not exist, ow 0.
 */
extern int net_os_xmit_done(struct usbd_function_instance *function, void *buff_ctx, int tx_rc);

/*
 *
 */
extern void *net_os_alloc_buffer(struct usbd_function_instance *, u8 **cp, int n);
extern void net_os_dealloc_buffer(struct usbd_function_instance *function_instance, void *data, void *buffer);

/*
 * net_os_recv_buffer - forward a received URB, or clean up after a bad one.
 *      buff_ctx == NULL --> just accumulate stats (count 1 bad buff)
 *      crc_bad != 0     --> count a crc error and free buff_ctx/skb
 *      trim             --> amount to trim from valid buffer (i.e. shorten
 *                           from amount requested in net_os_alloc_buff(..... n).
 *                    This will be called from interrupt context.
 */
extern int net_os_recv_buffer(struct usbd_function_instance *, void *os_data,
                void *os_buffer, int crc_bad, int length, int trim);

/*
 * net_os_config - schedule a network hotplug event if the OS supports hotplug.
 */
extern void net_os_config(struct usbd_function_instance *);

/*
 * net_os_settime - schedule a network hotplug event if the OS supports hotplug.
 */
extern void net_os_settime(struct usbd_function_instance *, u32);

/*
 * net_os_hotplug - schedule a network hotplug event if the OS supports hotplug.
 */
extern void net_os_hotplug(struct usbd_function_instance *);

/*! net_os_enable
 *
 */
extern void net_os_enable(struct usbd_function_instance *function);

/*! net_os_disable
 *
 */
extern void net_os_disable(struct usbd_function_instance *function);

/*! net_os_carrier_on
 *
 */
extern void net_os_carrier_on(struct usbd_function_instance *);

/*! net_os_carrier_off
 *
 */
extern void net_os_carrier_off(struct usbd_function_instance *);

/*! net_os_queue_stopped
 *
 */
int net_os_queue_stopped(struct usbd_function_instance *function_instance);

/*! net_os_wake_queue
 *
 */
void net_os_wake_queue(struct usbd_function_instance *function_instance);

/*! net_os_stop_queue
 *
 */
void net_os_stop_queue(struct usbd_function_instance *function_instance);
#endif
