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
 * otg/functions/acm/acm-fd.c
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/functions/acm/acm.c|20070912103628|42749
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @file otg/functions/acm/acm.c
 * @brief ACM Function Driver private defines
 *
 * This is the ACM Protocol specific portion of the
 * ACM driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |
 *    Implementation  +----------+
 *
 *
 *    Function        +----------+
 *    Descriptors     | tty-if   |
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |      <-----
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |
 *                    +----------+
 *
 * @ingroup ACMFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/wait.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>
#include <otg/usbp-mcpc.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include "acm.h"

otg_tag_t acm_trace_tag;

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! acm_urb_sent_ep0 - called to indicate ep0 URB transmit finished
 *
 * @param urb pointer to struct usbd_urb
 * @param urb_rc result
 * @return non-zero for error
 */
int acm_urb_sent_ep0 (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        //otg_trace_msg(TTY, __FUNCTION__,
        //                "URB SENT acm: %x urb: %x urb_rc: %d length: %d",
        //                (u32)acm, (u32)urb, urb_rc, urb->actual_length);

        TRACE_MSG4(TTY,"URB SENT acm: %x urb: %x urb_rc: %d length: %d", (u32)acm, (u32)urb, urb_rc, urb->actual_length);

        RETURN_EINVAL_UNLESS(acm);

        TRACE_MSG1(TTY,"urb: %x", urb);
        TRACE_MSG1(TTY, "length: %d", urb->actual_length);

        urb->function_privdata = NULL;
        usbd_free_urb(urb);
        return 0;
}

/*! acm_line_coding_urb_received - callback for sent URB
 * @brief Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb
 * @param urb_rc
 * @return non-zero for failure.
 */
int acm_line_coding_urb_received (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        TRACE_MSG5(TTY,"URB RECEIVED acm: %x urb: %x urb_rc: %d length: %d %d", acm, urb, urb_rc,
                        urb->actual_length, sizeof(struct cdc_acm_line_coding));

        RETURN_EINVAL_UNLESS(acm);
        RETURN_EINVAL_IF (USBD_URB_OK != urb_rc);
        RETURN_EINVAL_IF (urb->actual_length < sizeof(struct cdc_acm_line_coding));

        // something changed, copy and notify

        memcpy(&acm->line_coding, urb->buffer, sizeof(struct cdc_acm_line_coding));

        // XXX notify application if baudrate has changed
        // we need an os layer function to call here.... it should decode line_coding
        // and if possible notify posix layer that things have changed.

        usbd_free_urb(urb);

        if (acm->ops->line_coding)
                acm->ops->line_coding(function_instance);

        return 0;
}


/*!
 * @brief  acm_set_link_urb_received - callback for sent URB
 * @param urb
 * @param urb_rc result
 * @return non-zero for error
 */
int acm_set_link_urb_received (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG4(TTY,"URB RECEIVED acm: %x urb: %x urb_rc: %d length: %d", acm, urb, urb_rc, urb->actual_length);

        usbd_free_urb(urb);
        return 0;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * acm_start_out_urb - called to queue out urb on ep0 to receive device request OUT data
 * @param function
 * @param len
 * @param callback
 * @return non-zero if error
 */
int acm_start_out_urb_ep0(struct usbd_function_instance *function, int len, int (*callback) (struct usbd_urb *, int))
{
        struct acm_private *acm = function->privdata;
        struct usbd_urb *urb;
        RETURN_ZERO_UNLESS(len);
        RETURN_ENOMEM_UNLESS((urb = usbd_alloc_urb_ep0(function, len, callback)));
        urb->function_privdata = acm;
        urb->function_instance = function;
        RETURN_ZERO_UNLESS (usbd_start_out_urb(urb));
        usbd_free_urb(urb);          // de-alloc if error
        return -EINVAL;
}

/*!
 * acm_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero if error
 */
int acm_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct acm_private *acm = interface_instance->function.privdata;

        TRACE_SETUP(TTY, request);

        // determine the request direction and process accordingly
        switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {

        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_CLASS:
                TRACE_MSG1(TTY, "H2D CLASS bRequest: %02x", request->bRequest);
                switch (request->bRequest) {
                        /* CDC */
                case CDC_CLASS_REQUEST_SEND_ENCAPSULATED: break;
                case CDC_CLASS_REQUEST_SET_COMM_FEATURE: break;
                case CDC_CLASS_REQUEST_CLEAR_COMM_FEATURE: break;

                case CDC_CLASS_REQUEST_SET_LINE_CODING:                 /* 0x20 */
                           return acm_start_out_urb_ep0(function, le16_to_cpu(request->wLength), acm_line_coding_urb_received);

                case CDC_CLASS_REQUEST_SET_CONTROL_LINE_STATE: {        /* 0x22 */
                        unsigned int prev_bmLineState = acm->bmLineState;
                        acm->bmLineState = le16_to_cpu(request->wValue);

                        // schedule writers or hangup IFF open
                        TRACE_MSG5(TTY, "acm: %x interface: %x set control state, bmLineState: %04x previous: %04x changed: %04x",
                                        acm, interface_instance,
                                        acm->bmLineState, prev_bmLineState, acm->bmLineState ^ prev_bmLineState);

                        /* make sure there really is a state change in D0 */
                        if ((acm->bmLineState ^ prev_bmLineState) & CDC_LINESTATE_D0_DTR) {

                                TRACE_MSG1(TTY, "DTR state changed -> %x",
                                                (acm->bmLineState & CDC_LINESTATE_D0_DTR));

                                if (acm->bmLineState & CDC_LINESTATE_D0_DTR) {
                                        if (acm->flags & ACM_OPENED)
                                                acm->ops->schedule_wakeup_writers(function);

                                        //acm_schedule_recv_restart(function);
                                        //acm->ops->recv_start_bh(function);
                                        acm_start_recv_urbs(function);

                                }
                                #if 0
                                else {
                                        if (acm->flags & ACM_OPENED)
                                                acm->ops->schedule_hangup(function);

                                        acm_flush(function);
                                }
                                #endif

                                /* wake up blocked opens */
                                //acm->ops->wakeup_opens(function);

                                /* wake up blocked ioctls */
                                acm->ops->wakeup_state(function);

                        }

                        /* make sure there really is a state change in D1 */
                        if ((acm->bmLineState ^ prev_bmLineState) & CDC_LINESTATE_D1_RTS) {

                                TRACE_MSG1(TTY, "DCD state changed -> %x",
                                                (acm->bmLineState & CDC_LINESTATE_D1_RTS));

                                /* wake up blocked opens */
                                acm->ops->wakeup_opens(function);

                                /* wake up blocked ioctls */
                                acm->ops->wakeup_state(function);

                        }

                        /* check for hangup - both D0 and D1 not set */
                        if ((acm->bmLineState ^ prev_bmLineState) & (CDC_LINESTATE_D0_DTR | CDC_LINESTATE_D1_RTS)) {
                                UNLESS ((acm->bmLineState & (CDC_LINESTATE_D0_DTR | CDC_LINESTATE_D1_RTS)))
                                        // XXX check clocal
                                        //if (acm->flags & ACM_OPENED)
                                        if ((acm->flags & ACM_OPENED) && !(acm->flags & ACM_LOCAL))
                                                acm->ops->schedule_hangup(function);
                        }


                        /* send notification if we have DCD */
                        TRACE_MSG1(TTY, "bmUARTState: %04x sending (DCD|DSR) notification", acm->bmUARTState);

                        acm_send_int_notification(function, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
                        break;
                }

                case CDC_CLASS_REQUEST_SEND_BREAK: break;

                default:
                        return -EINVAL;
                }
                return 0;

        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_CLASS:
                TRACE_MSG1(TTY, "D2H CLASS bRequest: %02x", request->bRequest);
                switch (request->bRequest) {
                                                /* CDC */
                case CDC_CLASS_REQUEST_GET_ENCAPSULATED: break;
                case CDC_CLASS_REQUEST_GET_COMM_FEATURE: break;
                case CDC_CLASS_REQUEST_GET_LINE_CODING: {
                        struct usbd_urb *urb;

                        // XXX should check wLength
                        RETURN_ENOMEM_IF (!(urb = usbd_alloc_urb_ep0(function, sizeof(struct cdc_acm_line_coding),
                                                        acm_urb_sent_ep0)));
                        urb->function_instance = function;
                        urb->function_privdata = acm;

                        memcpy(urb->buffer, &acm->line_coding, sizeof(struct cdc_acm_line_coding));

                        urb->actual_length = sizeof(struct cdc_acm_line_coding);

                        TRACE_MSG1(TTY, "sending line coding urb: %p",(u32)(void*)urb);
                        RETURN_ZERO_UNLESS(usbd_start_in_urb(urb));
                        usbd_free_urb(urb);
                        TRACE_MSG0(TTY, "(send failed)--> -EINVAL");
                        return -EINVAL;
                }
                default:
                        TRACE_MSG1(TTY, "UNKNOWN D2H CLASS bRequest: %02x", request->bRequest);
                        return -EINVAL;
                }
                return 0;

                                                   /* MCPC */
        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:
                TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x", request->bRequest);
                switch (request->bRequest) {
                                                   /* MCPC */
                case MCPC_REQ_ACTIVATE_MODE:
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_ACTIVATE_MODE", request->bRequest);
                        return 0;

                case MCPC_REQ_SET_LINK:
                        return acm_start_out_urb_ep0(function, le16_to_cpu(request->wLength), acm_set_link_urb_received);

                case MCPC_REQ_CLEAR_LINK:
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_CLEAR_LINK", request->bRequest);
                        return 0;

                case MCPC_REQ_MODE_SPECIFIC_REQUEST:
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_SPECIFIC_REQUEST", request->bRequest);
                        return 0;
                default:
                        TRACE_MSG1(TTY, "UNKNOWN H2D CLASS bRequest: %02x XXXX", request->bRequest);
                        return -EINVAL;
                }
                break;

        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_VENDOR:
                TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x", request->bRequest);
                switch (request->bRequest) {
                case MCPC_REQ_GET_MODETABLE:
                        TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x MCPC_REQ_GET_MODETABLE", request->bRequest);
                        return 0;
                case MCPC_REQ_MODE_SPECIFIC_REQUEST:
                        TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x MCPC_REQ_SPECIFIC_REQUEST", request->bRequest);
                        return 0;
                default:
                        TRACE_MSG1(TTY, "UNKNOWN D2H Vendor bRequest: %02x", request->bRequest);
                        return -EINVAL;
                }
                break;

        default:
                TRACE_MSG2(TTY, "unknown bmRequestType: %02x bRequest: %02x", request->bmRequestType, request->bRequest);
                return -EINVAL;
        }
        return 0;
}

/*! @brief acm_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function_instance  The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */
void acm_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        TRACE_MSG1(TTY, "CLEARED bEndpointAddress: %02x", bEndpointAddress);
}


/* ******************************************************************************************* */
/*!  acm_open -
 * @param function_instance
 * @param chan
 * @return int
 */
int acm_open(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_EINVAL_UNLESS(acm);

        TRACE_MSG0(TTY,"OPEN");
        acm->flags |= ACM_OPENED;
        // XXX config option for old behavior
        acm->bmUARTState = /* CDC_UARTSTATE_BRXCARRIER_DCD | */ CDC_UARTSTATE_BTXCARRIER_DSR;

        //acm_schedule_recv_restart(function_instance);
       // acm->ops->recv_start_bh(function_instance);
        acm_start_recv_urbs(function_instance);

        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        return 0;
}

/*! acm_flush
 * @brief flush BLUK_IN, BLUK_OUT endpoints
 * @param function_instance
 * @return
 */
void acm_flush(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);

        RETURN_UNLESS(acm);
        atomic_set(&acm->bytes_received, 0);
        atomic_set(&acm->bytes_forwarded, 0);
        // XXX config option for old behavior
        acm->bmUARTState = /* CDC_UARTSTATE_BRXCARRIER_DCD |*/ CDC_UARTSTATE_BTXCARRIER_DSR;
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);

        acm->line_coding.dwDTERate = cpu_to_le32(0x1c200); // 115200
        acm->line_coding.bDataBits = 0x08;

        RETURN_IF(usbd_get_device_status(function_instance) == USBD_CLOSING);
        RETURN_IF(usbd_get_device_status(function_instance) != USBD_OK);
        RETURN_IF(usbd_get_device_state(function_instance) != STATE_CONFIGURED);
        usbd_flush_endpoint_index(function_instance, BULK_IN);
        usbd_flush_endpoint_index(function_instance, BULK_OUT);
}

/*!acm_close
 * @brief flush queued urbs and close connection
 * @param function_instance
 * @param  chan
 * @return 0 for success, EINVAL for error
 */
int acm_close(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;

        TRACE_MSG1(TTY,"acm: %x XXXXXX", (int)acm);
        RETURN_EINVAL_UNLESS(acm);

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;
        }

        otg_pthread_mutex_lock(&acm->mutex);

        acm->flags &= ~ACM_OPENED;

        //TRACE_MSG1(TTY,"queued: %d", (u32)(acm->queued_urbs));

        if (atomic_read(&acm->queued_urbs)) {
                acm->flags |= ACM_CLOSING;
                TRACE_MSG1(TTY, "setting ACM_CLOSING flags: %04x", acm->flags);
        }

        otg_pthread_mutex_unlock(&acm->mutex);

        /* XXX Should we wait here for data to flush...
         */
        RETURN_ZERO_IF(acm->flags & ACM_CLOSING);

        TRACE_MSG1(TTY, "normal close flags: %04x", acm->flags);

        /* Normal Close - no data pending
         */
        acm_flush(function_instance);
        acm->bmUARTState = 0;
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        return 0;
}


/*! acm_ready()
 * @param function_instance
 */
STATIC int acm_ready(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);

        TRACE_MSG6(TTY,"FLAGS: %08x CONFIGURED: %x OPENED: %x LOCAL: %x d0_dtr: %x d1_dcd: %x",
                        acm->flags,
                        BOOLEAN(acm->flags & ACM_CONFIGURED),
                        BOOLEAN(acm->flags & ACM_OPENED),
                        BOOLEAN(acm->flags & ACM_LOCAL),
                        acm->bmLineState & CDC_LINESTATE_D0_DTR,
                        acm->bmLineState & CDC_LINESTATE_D1_RTS);

        return (acm->flags & ACM_CONFIGURED) &&
                (acm->flags & ACM_OPENED) &&
                ((acm->flags & ACM_LOCAL) ? 1 : (acm->bmLineState & CDC_LINESTATE_D1_RTS) )
                ;

}

/*! acm_start_recv_urbs
 * @brief called to receive urbs
 * @param function_instance
 * @return number of urbs in queue
 */
int acm_start_recv_urbs(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        struct usbd_urb *urb;
        /*
         * Queue as many receive urbs as the OS layer has room for.  Return
         * the number in the queue (may be more than we queue here).
         */
        // XXX unsigned long flags;
        int num_in_queue = 0;
        int recv_space_available;
#if defined(CONFIG_OTG_ACM_PIPES_TEST)
        recv_space_available=512;
#else
        RETURN_ZERO_UNLESS(acm);
        RETURN_ZERO_IF((acm->flags & ACM_THROTTLED));

        UNLESS (acm_ready(function_instance)) {
                //printk(KERN_INFO"%s: NOT READY\n", __FUNCTION__);
                TRACE_MSG0(TTY, "START RECV: NOT READY");
                return -1;
        }


        /* if configured, opened, not throttled and we have carrier or local then
         * we can queue recv urbs.
         */


       // TRACE_MSG2(TTY,"START RECV: recv_urbs: %d space_avail: %d", atomic_read(&acm->recv_urbs),
         //               acm->ops->recv_space_available(function_instance, data_chan));

        //recv_space_available = acm->ops->recv_space_available(function_instance, data_chan);
#endif

#if 0
        while (((atomic_read(&acm->recv_urbs) + 1) * acm->readsize) <= recv_space_available) {
                struct usbd_urb *urb;

                BREAK_IF(!(urb = usbd_alloc_urb((struct usbd_function_instance *) function_instance,
                                                BULK_OUT, acm->readsize, acm_recv_urb)));
                atomic_inc(&acm->recv_urbs);
                urb->function_privdata = acm;
                TRACE_MSG3(TTY, "START RECV: %d urb: %p privdata: %p", atomic_read(&acm->recv_urbs), urb,acm);
                CONTINUE_UNLESS (usbd_start_out_urb(urb));
                atomic_dec(&acm->recv_urbs);
                TRACE_MSG1(TTY, "FAILED START RECV: %d", atomic_read(&acm->recv_urbs));
                usbd_free_urb(urb);
                break;
        }

        /* There needs to be at least one recv urb queued in order
         * to keep driving the push to the OS layer, so return how
         * many there are in case the OS layer must try again later.
         */
        num_in_queue = atomic_read(&acm->recv_urbs);
        return num_in_queue;
#else


        urb = usbd_alloc_urb((struct usbd_function_instance *) function_instance,
                                                BULK_OUT, 512, acm_recv_urb);
         atomic_inc(&acm->recv_urbs);
         urb->function_privdata = acm;
         TRACE_MSG3(TTY, "START RECV: %d urb: %p privdata: %p", atomic_read(&acm->recv_urbs), urb,acm);
         RETURN_ZERO_UNLESS (usbd_start_out_urb(urb));
         atomic_dec(&acm->recv_urbs);
         TRACE_MSG1(TTY, "FAILED START RECV: %d", atomic_read(&acm->recv_urbs));
         usbd_free_urb(urb);

        num_in_queue = atomic_read(&acm->recv_urbs);
        return 1;

#endif

}


#if 0
/*! @brief acm_start_recv_bh()
 * @param data
 * @return none
 */
STATIC void acm_start_recv_bh(void *data)
{
        struct usbd_function_instance *function_instance = data;
        RETURN_UNLESS(function_instance);

        /* keep trying to start urbs until we have enough or -1 (not ready)
         */
        while (!acm_start_recv_urbs(function_instance)) {
                struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
                TRACE_MSG0(TTY, "SLEEP START");
                interruptible_sleep_on_timeout(&acm->recv_wait, 1);
                TRACE_MSG0(TTY, "SLEEP FINISH");
        }
}

/*! @brief acm_restart_recv()
 * @param data
 * @return none
 */
void acm_restart_recv(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_UNLESS(function_instance);

        TRACE_MSG0(TTY, "SLEEP WAKEUP");
        wake_up_interruptible(&acm->recv_wait);
}

/*! acm_schedule_recv_restart()
 * @param function_instance
 */
void acm_schedule_recv_restart(struct usbd_function_instance *function_instance)
{
        unsigned long flags;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        //otg_led(LED1, TRUE);
        SET_WORK_ARG(acm->recv_bh, function_instance);
        SCHEDULE_WORK(acm->recv_bh);
        //otg_led(LED1, FALSE);
}
#endif

/* Transmit INTERRUPT ************************************************************************** */

/*!
 *@brief acm_send_int_notfication()
 *
 * Generates a response urb on the notification (INTERRUPT) endpoint.
 * CALLED from interrupt context.
 * @param function_instance
 * @param bnotification
 * @param data
 * @return non-zero if error
 */
STATIC int acm_send_int_notification(struct usbd_function_instance *function_instance, int bnotification, int data)
{
        struct usbd_urb *urb = NULL;
        struct cdc_notification_descriptor *notification;
        int rc = 0;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_EINVAL_UNLESS(acm);

        RETURN_ZERO_UNLESS((acm->flags & ACM_CONFIGURED));

        do {
                BREAK_IF(!(urb = usbd_alloc_urb((struct usbd_function_instance *)function_instance,
                                                INT_IN,
                                                sizeof(struct cdc_notification_descriptor),
                                                acm_urb_sent_int)));

                urb->function_privdata = acm;
                urb->function_instance = function_instance;

                memset(urb->buffer, 0, urb->buffer_length);
                urb->actual_length = sizeof(struct cdc_notification_descriptor);

                /* fill in notification structure */
                notification = (struct cdc_notification_descriptor *) urb->buffer;

                notification->bmRequestType = USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
                notification->bNotification = bnotification;

                switch (bnotification) {
                case CDC_NOTIFICATION_NETWORK_CONNECTION:
                        notification->wValue = data;
                        break;
                case CDC_NOTIFICATION_SERIAL_STATE:
                        notification->wLength = cpu_to_le16(2);
                        *((unsigned short *)notification->data) = cpu_to_le16(data);
                        break;
                }

                BREAK_IF(!(rc = usbd_start_in_urb (urb)));

                urb->function_privdata = NULL;
                usbd_free_urb (urb);

                TRACE_MSG1(TTY,"urb: %x", urb);

        } while(0);

        return rc;
}

/* Callback functions for TX urb completion (function chosen when urb is allocated) ***********/



/*! acm_closing
 */
int acm_closing(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm->flags & ACM_CLOSING);

        acm->flags &= ~ACM_CLOSING;

        /* No more data to send - complete close
         */
        acm_flush(function_instance);
        acm->bmUARTState = 0;
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);

        return 0;
}

#if 1
/*! acm_urb_sent_zle - callback for completed BULK ZLE URB
 *
 * Handles notification that an ZLE urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int acm_urb_sent_zle (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        void *buf;
        int rc = -EINVAL;
        int in_pkt_sz;

        //TRACE_MSG4(TTY,"urb: %p urb_rc: %d length: %d queue: %d",
        //                urb, urb_rc, urb->actual_length, acm->queued_urbs);

        usbd_free_urb (urb);
        rc = 0;
        return acm_closing(function_instance);
}
#endif


/*! acm_urb_sent_bulk - called to indicate bulk URB transmit finished
 * @param urb pointer to struct usbd_urb
 * @param rc result
 * @return non-zero for error
 */
STATIC int acm_urb_sent_bulk (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, BULK_IN, usbd_high_speed(function_instance));
        struct usbd_urb *tx_urb;
        static int count=1;

        TRACE_MSG5(TTY,"acm: %x urb: %x length: %d flags: %x queued_bytes: %d",
                        acm, urb, urb->actual_length,
                        acm ? acm->flags : 0,
                        acm ? atomic_read(&acm->queued_bytes) : 0
                        );
        RETURN_ZERO_UNLESS(acm);

#if defined(CONFIG_OTG_ACM_PIPES_TEST)
                RETURN_EINVAL_UNLESS(tx_urb = usbd_alloc_urb ((struct usbd_function_instance *) function_instance,
                                                BULK_IN, count+1, acm_urb_sent_bulk));

                memset ((void *)tx_urb->buffer, count & 0xff, count);
                tx_urb->function_privdata = acm;
                tx_urb->actual_length = count;
                if (count <1024) count+=1;
                RETURN_ZERO_UNLESS(usbd_start_in_urb (tx_urb));
                tx_urb->function_privdata = NULL;
                usbd_free_urb (tx_urb);
                return 0;

#else

        atomic_sub(urb->actual_length, &acm->queued_bytes);
        atomic_dec(&acm->queued_urbs);

        TRACE_MSG3(TTY, "queued_bytes: %d queude_urbs: %d opened: %d",
                        atomic_read(&acm->queued_bytes), atomic_read(&acm->queued_urbs), BOOLEAN(acm->flags & ACM_OPENED));

        urb->function_privdata = NULL;
        usbd_free_urb (urb);

        /* wakeup upper layers to send more data */
        if (acm->flags & ACM_OPENED)
                acm->ops->schedule_wakeup_writers(function_instance);

        /* check if there are still pending send urbs */
        RETURN_ZERO_IF(atomic_read(&acm->queued_urbs));

        /* check if we should send a zlp, send ZLP */
        #if 1
        UNLESS (urb->actual_length % in_pkt_sz) {
                if ((urb = usbd_alloc_urb (function_instance, BULK_IN, 0, acm_urb_sent_zle ))) {
                        urb->actual_length = 0;
                        TRACE_MSG1(TTY, "Sending ZLP: length: %d", urb->actual_length);
                        urb->flags |= USBD_URB_SENDZLP;

                        if ((rc = usbd_start_in_urb (urb))) {
                                TRACE_MSG1(TTY,"FAILED: %d", rc);
                                printk(KERN_ERR"%s: FAILED: %d\n", __FUNCTION__, rc);
                                urb->function_privdata = NULL;
                                usbd_free_urb (urb);
                        }
                }
                return 0;
        }
        #endif
        return acm_closing(function_instance);
#endif
}

/*! acm_urb_sent_int - called to indicate int URB transmit finished
 * @param urb pointer to struct usbd_urb
 * @param rc result
 * @return non-zero for error
 */
int acm_urb_sent_int (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);

        TRACE_MSG1(TTY,"urb: %x", urb);
        TRACE_MSG1(TTY,"INT length=%d",urb->actual_length);

        urb->function_privdata = NULL;
        usbd_free_urb(urb);
        return 0;
}

/* USB Device Functions ************************************************************************ */


/*!acm_write_room
 * @brief  get available room for queuing receiving urbs
 * @param function_instance
 * @param chan
 * @return non-zero if error
 */
STATIC int acm_write_room(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        TRACE_MSG2(TTY,"acm: %x queued_bytes: %d", acm, acm ? atomic_read(&acm->queued_bytes) : 0);
        RETURN_ZERO_UNLESS(acm);

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;
        }

        return (acm->max_queued_urbs <= atomic_read(&acm->queued_urbs) ||
                        acm->max_queued_bytes <= atomic_read(&acm->queued_bytes) ) ?  0 : acm->writesize ;
}

/*!acm_chars_in_buffer
 * @brief get number of of queued chars
 * @param function_instance
 * @param chan
 * @return number of chars, zero if error
 */
STATIC int acm_chars_in_buffer(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        int rc;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        TRACE_MSG2(TTY,"acm: %x queued_bytes: %d", (int)acm, acm ? atomic_read(&acm->queued_bytes) : 0);
        RETURN_ZERO_UNLESS(acm);

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;
        }

        return atomic_read(&acm->queued_bytes);
}

static int throttle_count = 0;
static int unthrottle_count = 0;


/*! acm_xmit_chars
 * @brief - transmit data to host
 * @param function_instance
 * @param chan
 * @param count
 * @param from_user
 * @param buf
 * @return number of bytes sent.
 */
int acm_xmit_chars(struct usbd_function_instance *function_instance, minor_chan_t chan,
                int count, int from_user, const unsigned char *buf)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        struct usbd_urb *urb;
        int in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, BULK_IN, usbd_high_speed(function_instance));
        int rc=0;

        TRACE_MSG4(TTY,"acm[%d]: %x flags: %x queued_bytes: %d", chan, acm,
                        acm ? acm->flags: 0, acm ? atomic_read(&acm->queued_bytes) : 0);
        RETURN_ZERO_UNLESS(acm);
        RETURN_ZERO_UNLESS((acm->flags & ACM_CONFIGURED) &&
                        ((acm->flags & ACM_LOCAL) ? 1 : (acm->bmLineState & CDC_LINESTATE_D1_RTS)));

        //TRACE_MSG5(TTY, "count: %d from_user: %d queued: %d max: %d writesize: %d",
        //                count, from_user, acm->queued_urbs, acm->max_queued_urbs, acm->writesize);

        switch(chan) {
        case data_chan:
                // sanity check and are we connect
                // XXX RETURN_ZERO_UNLESS(atomic_read(&acm->used));
                TRACE_MSG0(TTY,"DATA");
                RETURN_ZERO_UNLESS (count);
                RETURN_ZERO_UNLESS (acm->flags & ACM_CONFIGURED);       // XXX acm_ready() ??
                TRACE_MSG0(TTY,"connected OK");
                RETURN_ZERO_IF(acm->max_queued_urbs <= atomic_read(&acm->queued_urbs));
                TRACE_MSG0(TTY,"acm->max_queued_urbs OK");

                // allocate a write urb
                count = MIN(count, acm->writesize);

                RETURN_ZERO_UNLESS ((urb = usbd_alloc_urb ((struct usbd_function_instance *) function_instance,
                                                BULK_IN, count, acm_urb_sent_bulk)));

                if (from_user)
                        rc=copy_from_user ((void *)urb->buffer, (void *)buf, count);
                else
                        memcpy ((void *)urb->buffer, (void *)buf, count);


                #if 0
                {
                        u8 *cp = urb->buffer;
                        int len;
                        for (len = 0; len < count; len += 8, cp += 8) {
                                TRACE_MSG8(TTY, "SEND [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                                cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                        }
                }
                #endif

                urb->function_privdata = acm;
                urb->actual_length = count;

                #if 0
                /* force ZLP if multiple of packet size */
                UNLESS(count % in_pkt_sz) {
                        urb->flags |= USBD_URB_SENDZLP;
                }
                #endif

                atomic_add(count, &acm->queued_bytes);
                atomic_inc(&acm->queued_urbs);

                UNLESS (usbd_start_in_urb (urb)) return count;

                usbd_free_urb (urb);
                atomic_sub(count, &acm->queued_bytes);
                atomic_dec(&acm->queued_urbs);
                return 0;

        case command_chan:
                TRACE_MSG0(TTY,"COMMAND");
        default:
                break;
        }
        return 0;
}

/*! acm_recv_urb - callback for urb received
 * @param urb
 * @param rc
 * @return non-zero if error
 */
int acm_recv_urb (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance->privdata;

        /* Return 0 if urb has been accepted,
         * return 1 and expect caller to deal with urb release otherwise.
         */

        TRACE_MSG2(TTY, "acm: %x len: %d", acm, urb->actual_length);

        atomic_dec(&acm->recv_urbs);               // this could probably be atomic operation....

        if (USBD_URB_CANCELLED == rc) {
                TRACE_MSG1(TTY,"cancelled URB=%p",urb);
                return -EINVAL;
        }
        if (USBD_URB_OK != rc) {
                TRACE_MSG2(TTY,"rejected URB=%p rc=%d",urb,rc);
                usbd_free_urb(urb);

                //acm_schedule_recv_restart(function_instance);
               // acm->ops->recv_start_bh(function_instance);

                acm_start_recv_urbs(function_instance);
                return 0;
        }
#if defined(CONFIG_OTG_ACM_PIPES_TEST)
        acm_start_recv_urbs(function_instance);

#else
        /* loopback mode
         */
        if (acm->flags & ACM_LOOPBACK)
                acm_xmit_chars(function_instance, data_chan, urb->actual_length, 0, urb->buffer);

        /* acm_start_recv_urbs() will never queue more urbs than there is currently
         * room in the upper layer buffer for. So we are guaranteed that any data actually
         * received can be given to the upper layers without worrying if we will
         * actually have room.
         */
        else
                if ((rc = acm->ops->recv_chars(function_instance, urb->buffer, urb->actual_length)))
                        return rc; // XXX


        atomic_add(urb->actual_length, &acm->bytes_received);

        TRACE_MSG1(TTY,"bytes_received: %d", atomic_read(&acm->bytes_received));
        usbd_free_urb(urb);

        //acm_schedule_recv_restart(function_instance);
       // acm->ops->recv_start_bh(function_instance);
        acm_start_recv_urbs(function_instance);
        return 0;
#endif
}

/*! acm_wait_task - delay task to perform
 * @param function_instance
 * @param queue
 */
void acm_wait_task(struct usbd_function_instance *function_instance, OLD_WORK_ITEM *queue)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);


}

/* ********************************************************************************************* */
/*! acm_get_d0_dtr
 * @brief get d0 DTR status
 * Get DTR status.
 * @param function_instance
 * @return int
 */
int acm_get_d0_dtr(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);
        TRACE_MSG2(TTY,"bmLineState: %04x d0_dtr: %x", acm->bmLineState, (acm->bmLineState & CDC_LINESTATE_D0_DTR) ? 1 : 0);
        return (acm->bmLineState & CDC_LINESTATE_D0_DTR) ? 1 : 0;
}

/*!
 * @brief acm_get_d1_rts
 * Get DSR status
 * @param function_instance
 * @return int
 */
int acm_get_d1_rts(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);
        TRACE_MSG2(TTY,"bmLineState: %04x d1_rts: %x", acm->bmLineState, (acm->bmLineState & CDC_LINESTATE_D1_RTS) ? 1 : 0);
        return (acm->bmLineState & CDC_LINESTATE_D1_RTS) ? 1 : 0;
}


/* ********************************************************************************************* */
/*!
 * @brief acm_get_bRxCarrier
 * Get bRxCarrier (DCD) status
 * @param function_instance
 @ @return int
 */
int acm_get_bRxCarrier(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);
        TRACE_MSG2(TTY,"bmLineState: %04x bRxCarrier: %x",
                        acm->bmLineState, (acm->bmUARTState & CDC_UARTSTATE_BRXCARRIER_DCD) ? 1 : 0);
        return (acm->bmUARTState & CDC_UARTSTATE_BRXCARRIER_DCD) ? 1 : 0;
}

/*!
 * @brief acm_get_bTxCarrier
 * Get bTxCarrier (DSR) status
 * @param function_instance
 @ @return int
 */
int acm_get_bTxCarrier(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);
        TRACE_MSG2(TTY,"bmLineState: %04x bTxCarrier: %x",
                        acm->bmLineState, (acm->bmUARTState & CDC_UARTSTATE_BTXCARRIER_DSR) ? 1 : 0);
        return (acm->bmUARTState & CDC_UARTSTATE_BTXCARRIER_DSR) ? 1 : 0;
}


/*! @brief acm_set_bRxCarrier
 * Get bRxCarrier (DCD) status
 * @param function_instance
 * @param value
 * @return none
 */
void acm_set_bRxCarrier(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG2(TTY,"acm: %x value: %x", (int)acm, value);
        RETURN_UNLESS(acm);
        acm->bmUARTState &= ~CDC_UARTSTATE_BRXCARRIER_DCD;
        acm->bmUARTState |= value ? CDC_UARTSTATE_BRXCARRIER_DCD : 0;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
}

/*! @brief acm_set_bTxCarrier
 * Set bTxCarrier (DSR) status
 * @param function_instance
 * @param value
 * @return none
 */
void acm_set_bTxCarrier(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG2(TTY,"acm: %x value: %x", (int)acm, value);
        RETURN_UNLESS(acm);
        acm->bmUARTState &= ~CDC_UARTSTATE_BTXCARRIER_DSR;
        acm->bmUARTState |= value ? CDC_UARTSTATE_BTXCARRIER_DSR : 0;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
}


/* ********************************************************************************************* */

/*! acm_bRingSignal
 * Indicate Ring signal to host.
 */
void acm_bRingSignal(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BRINGSIGNAL;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BRINGSIGNAL;
}

/*! acm_bBreak
 * Indicate Break signal to host.
 */
void acm_bBreak(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BBREAK;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BBREAK;
}

/*! acm_bOverrun
 * Indicate Overrun signal to host.
 */
void acm_bOverrun(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BOVERRUN;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BOVERRUN;
}

/*! acm_set_local
 * Set LOCAL status
 * @param function_instance
 * @param value
 */
void acm_set_local(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        TRACE_MSG2(TTY,"acm: %x value: %x", acm, value);
        RETURN_UNLESS(acm);
        acm->flags &= ~ACM_LOCAL;
        acm->flags |= value ? ACM_LOCAL : 0;
        acm_start_recv_urbs(function_instance);
}

/*! @ brief acm_set_loopback
 * Set LOOPBACK status
 * @param function_instance
 * @param value
 */
void acm_set_loopback(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG2(TTY,"acm: %x value: %d", (int)acm, value);
        RETURN_UNLESS(acm);
        acm->flags &= ~ACM_LOOPBACK;
        acm->flags |= value ? ACM_LOOPBACK : 0;
        acm_start_recv_urbs(function_instance);
}


/*! @ brief acm_set_throttle
 * Set LOOPBACK status
 * @param function_instance
 * @param value
 */
void acm_set_throttle(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG2(TTY,"acm: %x value: %d", (int)acm, value);
        RETURN_UNLESS(acm);
        acm->flags &= ~ACM_THROTTLED;
        acm->flags |= value ? ACM_THROTTLED : 0;
        acm_start_recv_urbs(function_instance);
}

/*! @ brief acm_get_throttled
 * Set LOOPBACK status
 * @param function_instance
 */
int acm_get_throttled(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);

        return (acm->flags & ACM_THROTTLED) ? 1 : 0;
}


void acm_throttle (struct usbd_function_instance *function_instance, minor_chan_t xx){

        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"sos acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        acm->flags |= ACM_THROTTLED;
        usbd_flush_endpoint_index(function_instance,BULK_OUT);
        TRACE_MSG1(TTY,"sos acm111: %x", (int)acm);
}

void acm_unthrottle (struct usbd_function_instance *function_instance, minor_chan_t xx)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        RETURN_UNLESS(acm);
        acm->flags &= ~ACM_THROTTLED;
        acm_start_recv_urbs(function_instance);

}

/*! @brief acm_function_enable - called by USB Device Core to enable the driver
 * @param function_instance The function instance for this driver to use.
 * @param tty_type
 * @param os_ops
 * @param max_urbs
 * @param max_bytes
 * @return non-zero if error.
 */
int acm_function_enable (struct usbd_function_instance *function_instance, tty_type_t tty_type,
                struct acm_os_ops *os_ops, int max_urbs, int max_bytes)
{
        struct acm_private *acm = NULL;

        RETURN_EINVAL_UNLESS((acm = CKMALLOC(sizeof(struct acm_private))));
        printk(KERN_INFO "acm_function_enable and acm = CKMALLOC()\n");
        function_instance->privdata = acm;

        acm->tty_type = tty_type;
        acm->ops = os_ops;
        acm->max_queued_urbs = max_urbs;
        acm->max_queued_bytes = max_bytes;

        switch(tty_type) {
        case tty_if:
        case dun_if:
                acm->minors = 2;
                break;
        default:
                acm->minors = 1;
                break;
        }
        acm->line_coding.dwDTERate = __constant_cpu_to_le32(0x1c200);
        acm->line_coding.bDataBits = 0x08;
        //PREPARE_WORK_ITEM(acm->recv_bh, acm_start_recv_bh, function_instance);

        init_waitqueue_head(&acm->recv_wait);

        TRACE_MSG1(TTY, "ENABLED writesize: %d", acm->writesize);

        return acm->ops->enable(function_instance);
}

/*! @brief acm_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
void acm_function_disable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct acm_private *acm = function ? function->privdata : NULL;

        TRACE_MSG0(TTY, "DISABLED");
        acm->ops->disable(function);
        function->privdata = NULL;
        printk(KERN_INFO "acm_function_disable and LKFREE(acm)\n");
        LKFREE(acm);
}

/*! acm_set_configuration
 * @brief called to configure this function instance
 * @param function_instance
 * @param configuration index to set
 * @return int
 */
int acm_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        struct usbd_urb *tx_urb;

        TRACE_MSG4(TTY, "CONFIGURED acm: %x wIndex:%d %x cfg: %d ", acm,
                        interface_instance->wIndex, function_instance, configuration);

        // XXX Need to differentiate between non-zero, zero and non-zero done twice

        /* speead and size cannot be set until now as we don't know waht speed we are
         * enumerated at
         */
        acm->hs = usbd_high_speed((struct usbd_function_instance *) function_instance);
        acm->readsize = usbd_endpoint_transferSize(function_instance, BULK_OUT, acm->hs);
        acm->writesize = usbd_endpoint_transferSize(function_instance, BULK_IN, acm->hs);
        acm->flags |= ACM_CONFIGURED;

        TRACE_MSG2(TTY, "configured readsize: %d writesize: %d", acm->readsize, acm->writesize);

#if defined(CONFIG_OTG_ACM_PIPES_TEST)

        acm_start_recv_urbs(function_instance);

        RETURN_EINVAL_UNLESS(tx_urb = usbd_alloc_urb ((struct usbd_function_instance *) function_instance,
                                                BULK_IN, 1, acm_urb_sent_bulk));

                        memset ((void *)tx_urb->buffer, 1, 1);
                tx_urb->function_privdata = acm;
                tx_urb->actual_length = 1;

                RETURN_ZERO_UNLESS(usbd_start_in_urb (tx_urb));
        usbd_free_urb (tx_urb);
        return -EINVAL;

#else /*defined(CONFIG_OTG_MSC_PIPES_TEST) */
        acm_start_recv_urbs(function_instance);
#endif



        return 0;
}

/*!acm_set_interface
 * @brief set interface from function instance
 * @param function_instance
 * @param wIndex
 * @param altsetting
 * @return int
 */
int acm_set_interface (struct usbd_function_instance *function_instance, int wIndex, int altsetting)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG0(TTY, "SET INTERFACE");
        return 0;
}

/*!
 * @brief acm_reset - called to indicate bus reset
 * @param function_instance
 * @return int
 */
int acm_reset (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;

        TRACE_MSG0(TTY, "RESET");

        /* if configured and open then schedule hangup
         */
        if ((acm->flags & ACM_OPENED) && (acm->flags & ACM_CONFIGURED && !(acm->flags & ACM_LOCAL)))
                acm->ops->schedule_hangup(function_instance);

        acm->flags &= ~ACM_CONFIGURED;
        TRACE_MSG0(TTY, "RESET continue");

        // XXX flush
        // Release any queued urbs
        return 0;
}

/*!
 * @brief acm_suspended - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int acm_suspended (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG0(TTY, "SUSPENDED");
        return 0;
}

/*!
 * acm_resumed - called to indicate urb has been received
 * @param function_instance the function instance for this driver
 * @return int
 */
int acm_resumed (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG0(TTY, "RESUMED");
        return 0;
}

/*! acm_send_queued
 * @brief to check how many urbs in queue waiting to send
 * @param function_instance The function instance for this driver
 * @return number of queued urbs
 */
int acm_send_queued (struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance->privdata;
        return atomic_read(&acm->queued_urbs);
}
