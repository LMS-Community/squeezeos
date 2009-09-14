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
 * otg/functions/acm/acm.h
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/functions/acm/acm.h|20070911235624|28546
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @defgroup ACMFunction Serial ACM Interface Function
 * @ingroup InterfaceFunctions
 */

/*!
 * @file otg/functions/acm/acm.h
 * @brief ACM Function Driver private defines
 *
 * This is an ACM Function Driver. The upper edge is exposed
 * to the hosting OS as a Posix type character device. The lower
 * edge implements the USB Device Stack API.
 *
 * This driver implements the CDC ACM driver model and uses the CDC ACM
 * protocols.
 *
 * Note that it appears to be impossible to determine the end of a receive
 * bulk transfer larger than wMaxPacketsize. The host is free to send
 * wMaxPacketsize chars in a single transfer. This means that we cannot
 * queue receive urbs larger than wMaxPacketsize (typically 64 bytes.)
 *
 * This does not however prevent queuing transmit urbs with larger amounts
 * of data. It is interpreted at the receiving (host) end as a series of
 * wMaxPacketsize transfers but because there is no interpretation to the
 * amounts of data sent it does affect anything if we treat the data as a
 * single larger transfer.
 *
 * @ingroup ACMFunction
 */


// Endpoint indexes in acm_endpoint_requests[] and the endpoint map.
#define INT_IN          0x00
#define BULK_IN         0x01
#define BULK_OUT        0x02
#define ENDPOINTS       0x03

#define COMM_INTF       0x00
#define DATA_INTF       0x01



// Most hosts don't care about BMAXPOWER, but the UUT tests want it to be 1
#define BMAXPOWER   1
#define BMATTRIBUTE 0

#define STATIC
//#define STATIC static
//

/*! @name ACM Flags
 * @{
 */
#define ACM_OPENED      (1 << 0)        /*!< upper layer has opened the device port */
#define ACM_CONFIGURED  (1 << 1)        /*!< device is configured */
#define ACM_THROTTLED   (1 << 2)        /*!< upper layer doesn't want to receive data  */
//#define ACM_CARRIER     (1 << 3)        /*!< host has set DTR, i.e. host has opened com device */
#define ACM_LOOPBACK    (1 << 4)        /*!< upper layer wants local loopback */
#define ACM_LOCAL       (1 << 5)        /*!< upper layer specified LOCAL mode */

#define ACM_CLOSING     (1 << 6)        /*!< upper layer has asked us to close */

/*! @} */

/*! @name tty type
 * @{
 */
/*! @var typedef enum tty_type  tty_type_t
 */
typedef enum tty_type {
        tty_if,
        dun_if,
        obex_if,
        atcom_if,
} tty_type_t;


/*! @} */

/*! @name data channel
 * @{
 */
typedef enum minor_chan {
        data_chan,
        command_chan,
} minor_chan_t;

#define CHAN(x) ((x&1)?command_chan : data_chan)

/*! @} */

/*! @struct  acm_os_ops acm.h "otg/functions/acm/acm.h"
 *
 *  @brief  This structure lists the operations implemented in the upper os layer.
 */
struct acm_os_ops {
        /*! enable
         * Enable the function driver.
         */
        int (*enable)(struct usbd_function_instance *);

        /*! disable
         * Disable the function driver.
         */
        void (*disable)(struct usbd_function_instance *);

        /*! wakeup_opens
         * Wakeup processes waiting for DTR.
         */
        void (*wakeup_opens)(struct usbd_function_instance *);

        /*! wakeup_opens
         * Wakeup processes waiting for state change.
         */
        void (*wakeup_state)(struct usbd_function_instance *);

        /*! wakeup writers
         * Wakeup pending writes.
         */
        void (*schedule_wakeup_writers)(struct usbd_function_instance *);

        /*! recv_space_available
         * Check for amount of receive space that is available, controls
         * amount of receive urbs that will be queued.
         */
        int (*recv_space_available)(struct usbd_function_instance *, minor_chan_t);

        /*! recv_chars
         * Process chars received on specified interface.
         */
        int (*recv_chars)(struct usbd_function_instance *, u8 *cp, int n);


        /*! schedule_hangup
         * Schedule a work item that will perform a hangup.
         */
        void (*schedule_hangup)(struct usbd_function_instance *);

        /*! comm_feature
         * Tell function that comm feature has changed.
         */
        //void (*comm_feature)(struct usbd_function_instance *);

        /*! line_coding
         * Tell function that line coding has changed.
         */
        void (*line_coding)(struct usbd_function_instance *);

        /*! control_state
         * Tell function that control state has changed.
         */
        //void (*control_state)(struct usbd_function_instance *);

        /*! send_break
         * Tell function to send a break signal.
         */
        void (*send_break)(struct usbd_function_instance *);

        /*! schedule_recv_start_bh
         * Tell function to send a break signal.
         */
        void (*recv_start_bh)(struct usbd_function_instance *);

};


/*! @struct acm_private acm.h "otg/functions/acm/acm.h"
 *
 * @brief encapsulation of acm instance global variables
 *
 */
struct acm_private {
        struct usbd_function_instance *function_instance;
        otg_tag_t trace_tag;

        tty_type_t tty_type;

        int index;
        int minors;

        u32 flags;

        otg_pthread_mutex_t mutex;
        otg_atomic_t recv_urbs;
        //otg_atomic_t used;
        otg_atomic_t queued_bytes;
        otg_atomic_t queued_urbs;

        BOOL hs;
        int writesize;
        int readsize;

        /*TBR debug receive flow control */
        otg_atomic_t bytes_received;
        otg_atomic_t bytes_forwarded;
        /*TBR end debug */

        struct cdc_acm_line_coding line_coding; // C.f. Table 50 - [GS]ET_LINE_CODING Device Request (to/from host)
        cdc_acm_bmUARTState bmUARTState;        // C.f. Table 51 - SET_CONTROL_LINE_STATE Device Request (from host)
        cdc_acm_bmLineState bmLineState;        // C.f. Table 69 - SERIAL_STATE Notification (to host)

        /*for tiocmget and tiocmset functions */
        int msr;
        int mcr;

        int max_queued_urbs;
        int max_queued_bytes;


        OLD_WORK_STRUCT recv_bh;
        wait_queue_head_t recv_wait;            /*! wait queue for blocking open*/


        //void *privdata;

        struct acm_os_ops *ops;


        //mcpc_mode_t mcpc_mode;
        u8 mode;                                // mode if mcpc_activated

        u8 modes;                               // sizeof received modetable
        u8 *modetable;                          // most recent received mode table

};


#define TTY acm_trace_tag
extern otg_tag_t acm_trace_tag;


/*! acm_fd_init - called to initialize the acm_fd library
 * This call initializs the acm_fd library and registers the function driver
 * with the USB Peripheral Stack.
 *
 */
int acm_fd_init (struct usbd_function_instance *, char *inf, u32 vendor_id, u32 product_id, u32 wmax_urbs, u32 wmax_bytes);

/*! acm_fd_exit - called before exiting
 * This call will cause the function driver to be de-registered.
 */
void acm_fd_exit (struct usbd_function_instance *);

/*! acm_open - device open
  * This is called the device is opened (first open only if non-exclusive opens allowed).
  */
int acm_open (struct usbd_function_instance *, minor_chan_t chan);

/*! acm_close - Device close
  * This is called when the device closed (last close only if non-exclusive opens allowed.)
  */
int acm_close (struct usbd_function_instance *, minor_chan_t chan);


/*! acm_flush - flush data urbs.
 * Cancel outstanding data urbs.
 */
void acm_flush (struct usbd_function_instance *);

 /*! acm_schedule_recv - queue as many data receive urbs as possible
  * This will schedule a bottom half hander that will will start as
  * many receive data urbs as are allowed given the amount of room
  * available in the upper layer. If no urbs are queued by the
  * bottom half handler it will re-schedule itself.
  */
void acm_schedule_recv_restart (struct usbd_function_instance *);

 /*! acm_throttle - set throttle flag for specified interface
  * Receive urbs will not be queued when throttled.
  */
void acm_throttle (struct usbd_function_instance *, minor_chan_t);

/*! acm_unthrottle - reset throttle flag for specified interface
  * Receive urbs are allowed to be queued. If no urbs are queued a
  * bottom half handler will be scheduled to queue them.
  */
 void acm_unthrottle (struct usbd_function_instance *, minor_chan_t);


/*!acm_xmit_chars - send data via specified interface
  * This will start a transmit urb to send the specified data. The
  * number of characters sent will be returned.
  */
int acm_xmit_chars (struct usbd_function_instance *, minor_chan_t chan, int count, int from_user, const unsigned char *buf);

/*! acm_write_room
  * Return amount of data that could be queued for sending.
  */
 int acm_write_room (struct usbd_function_instance *, minor_chan_t);

/*! acm_chars_in_buffer
  * Return number of chars in xmit buffer.
  */
int acm_chars_in_buffer (struct usbd_function_instance *, minor_chan_t);


/*! acm_send_int_notification - send notification via interrupt endpoint
  *  This can be used to queue network, serial state change notifications.
  */
 int acm_send_int_notification (struct usbd_function_instance *, int bnotification, int data);

/*! acm_wait_task - wait for task to complete.
 */
void acm_wait_task (struct usbd_function_instance *, OLD_WORK_ITEM *queue);
//void acm_wait_task (struct usbd_function_instance *, struct otg_workitem *work);
/*! acm_ready - return true if connected and carrier
 */
int acm_ready (struct usbd_function_instance *);

/*! acm_get_d1_rts
 * Get DSR status
 */
int acm_get_d1_rts (struct usbd_function_instance *);

/*! acm_get_d0_dtr
 * Get DTR status.
 */
int acm_get_d0_dtr (struct usbd_function_instance *);


/*! acm_get_bRxCarrier
 * Get bRxCarrier (DCD) status
 */
int acm_get_bRxCarrier(struct usbd_function_instance *function_instance);

/*! acm_set_bRxCarrier
 * set bRxCarrier (DCD) status
 */
void acm_set_bRxCarrier (struct usbd_function_instance *, int);

/*! acm_get_bTxCarrier
 * Get bTxCarrier (DSR) status
 */
int acm_get_bTxCarrier (struct usbd_function_instance *);

/*! acm_set_bTxCarrier
 * Set bTxCarrier (DSR) status
 */
void acm_set_bTxCarrier(struct usbd_function_instance *function_instance, int value);

/*! acm_bRingSignal
 * Indicate Ring signal to host.
 */
void acm_bRingSignal (struct usbd_function_instance *);

/*! acm_bBreak
 * Indicate Break signal to host.
 */
void acm_bBreak (struct usbd_function_instance *);

/*! acm_bOverrun
 * Indicate Overrun signal to host.
 */
void acm_bOverrun (struct usbd_function_instance *);

/*! acm_set_throttle
 * Set LOOPBACK status
 */
void acm_set_throttle (struct usbd_function_instance *, int value);

/*! acm_get_throttled
 * Set LOOPBACK status
 */
int acm_get_throttled(struct usbd_function_instance *function_instance);


/*! acm_set_local
  * Set LOCAL status
  */
void acm_set_local (struct usbd_function_instance *, int value);

/*! set_loopback - set loopback mode
 * Sets LOOP flag, data received from the host will be immediately
 * returned without passing to the upper layer.
 */
void acm_set_loopback (struct usbd_function_instance *, int value);



/*! acm_start_recv_urbs
 */
void acm_restart_recv(struct usbd_function_instance *);
int acm_start_recv_urbs(struct usbd_function_instance *);


/*! acm_line_coding_urb_received - callback for sent URB
 * @param urb
 * @param rc
 * @return int value
 */
int acm_line_coding_urb_received (struct usbd_urb *urb, int urb_rc);

/*! acm_urb_sent_ep0 - called to indicate ep0 URB transmit finished
 * @param urb
 * @param rc
 * @return int value
 */
int acm_urb_sent_ep0 (struct usbd_urb *urb, int rc);

/*! acm_recv_urb
 * @param urb
 * @param rc
 * @return int value
 */
int acm_recv_urb (struct usbd_urb *urb, int rc);

/*! acm_urb_sent_int - called to indicate int URB transmit finished
 *  @param urb  usb request block pointer
 *  @param result code
 */
int acm_urb_sent_int (struct usbd_urb *urb, int rc);

/*! acm_function_enable - called by USB Device Core to enable the driver
 * @param function_instance The function instance for this driver to use.
 * @param tty_type
 * @param os_ops
 * @param mac_urbs
 * @param max_bytes
 * @return non-zero if error.
 */
int acm_function_enable (struct usbd_function_instance *function_instance, tty_type_t tty_type,
                struct acm_os_ops *os_ops, int max_urbs, int max_bytes);

/*! acm_function_disable - called by the USB Device Core to disable the driver
 * @param function */
void acm_function_disable (struct usbd_function_instance *function);

/*! acm_set_configuration - called to indicate urb has been received
  * @param function_instance
  * @param configuration
  */
int acm_set_configuration (struct usbd_function_instance *function_instance, int configuration);

/*! acm_set_interface - called to indicate urb has been received
 * @param function
+ * @param wIndex
+ * @param altsetting
  */
int acm_set_interface (struct usbd_function_instance *function, int wIndex, int altsetting);

/*! acm_reset - called to indicate urb has been received
 * @param function
 */
int acm_reset (struct usbd_function_instance *function);

/*! acm_suspended - called to indicate urb has been received
 * @param function
 */
int acm_suspended (struct usbd_function_instance *function);

/*! acm_resumed - called to indicate urb has been received
 * @param function
 */
int acm_resumed (struct usbd_function_instance *function);


/*! acm_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero if error
 */
int acm_device_request (struct usbd_function_instance *function, struct usbd_device_request *request);


/*! acm_endpoint_cleared - called by the USB Device Core when endpoint cleared
 *  @param function_instance The function instance for this driver
+ * @param bEndpointAddress  Endpoint address
+ * @return none
  */
void acm_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress);

/*! acm_send_queued - called by the USB Device Core when endpoint cleared
  * @param function_instance The function instance for this driver
  */
int acm_send_queued (struct usbd_function_instance *function_instance);



/*
 * otg-trace tag.
 */
extern otg_tag_t acm_fd_trace_tag;

#define MAX_QUEUED_BYTES        512
#define MAX_QUEUED_URBS         32     // Max for write
