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
 * otg/functions/acm/tty.h
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/functions/acm/tty.h|20070911235625|36999
 *
 *      Copyright (c) 2003-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/acm/tty.h
 * @brief ACM Function Driver private defines
 *
 *
 * This is an ACM Function Driver. The upper edge is exposed
 * to the hosting OS as a Posix type character device. The lower
 * edge implements the USB Device Stack API.
 *
 * These are emulated and set by the tty driver as appropriate
 * to model a virutal serial port. The
 *
 *      TIOCM_RNG       RNG (Ring)                              not used
 *      TIOCM_LE        DSR (Data Set Ready / Line Enable)
 *      TIOCM_DSR       DSR (Data Set Ready)
 *      TIOCM_CAR       DCD (Data Carrier Detect)
 *      TIOCM_CTS       CTS (Clear to Send)
 *      TIOCM_CD        TIOCM_CAR
 *      TIOCM_RI        TIOCM_RNG
 *
 * These are set by the application:
 *
 *      TIOCM_DTR       DTR (Data Terminal Ready)
 *      TIOCM_RTS       RTS (Request to Send)
 *
 *      TIOCM_LOOP      Set into loopback mode
 *      TIOCM_OUT1      Not used.
 *      TIOCM_OUT2      Not used.
 *
 *
 * The following File and  termio c_cflags are used:
 *
 *      O_NONBLOCK      don't block in tty_open()
 *      O_EXCL          don't allow more than one open
 *
 *      CLOCAL          ignore DTR status
 *      CBAUD           send break if set to B0
 *
 * Virtual NULL Modem
 *
 * Peripheral to Host via Serial State Notification (write ioctls)
 *
 * Application             TIOCM           Null Modem      ACM (DCE)       Notificaiton    Host (DTE)
 * -------------------------------------------------------------------------------------------------------------
 * Request to send         TIOCM_RTS       RTS ->  CTS     CTS             Not Available   Clear to Send (N/A)
 * Data Terminal Ready     TIOCM_DTR       DTR ->  DSR     DSR             bTxCarrier      Data Set Ready
 *                                         DTR ->  DCD     DCD             bRXCarrier      Carrier Detect
 * Ring Indicator          TIOCM_OUT1      OUT1 -> RI      RI              bRingSignal     Ring Indicator
 * Overrun                 TIOCM_OUT2      OUT2 -> Overrun Overrun         bOverrun        Overrun
 * Send Break              B0              B0   -> Break   Break           bBreak          Break Received
 * -------------------------------------------------------------------------------------------------------------
 *
 *
 * Host to Peripheral via Set Control Line State
 *
 * Host (DTE)              Line State      ACM (DCE)       Null Modem      TIOCM           Peripheral (DTE)
 * -------------------------------------------------------------------------------------------------------------
 * Data Terminal Ready     D0              DTR             DTR ->  DSR     TIOCM_DSR       Data Set Ready
 *                                                         DTR ->  DCD     TIOCM_CAR       Carrier Detect
 * Request To Send         D1              RTS             RTS ->  CTS     TIOCM_CTS       Clear to Send
 * -------------------------------------------------------------------------------------------------------------
 *
 *
 * @ingroup TTYFunction
 * @ingroup LINUXAPI
 */

extern struct usbd_function_driver tty_function_driver;


#define TTYFD_OPENED      (1 << 0)                /*! OPENED flag */
#define TTYFD_CLOCAL      (1 << 1)                /*! CLOCAL flag */
//#define TTYFD_LOOPBACK    (1 << 2)               /*! LOOPBACK flag */
#define TTYFD_EXCLUSIVE   (1 << 3)                /*! EXCLUSIVE flag */
#define TTYFD_THROTTLED   (1 << 4)                /*! THROTTLED flag */

#define TTY_OVERFLOW_SIZE       4096

/*! @struct os_private tty.h "otg/functions/acm/tty.h"
  */
struct os_private {

        //struct tty_driver *tty_driver;          /*!< tty structure */
        int tty_driver_registered;              /*!< non-zero if tty_driver registered */
        int usb_driver_registered;              /*!< non-zero if usb function registered */

        int index;

        struct otg_workitem *wakeup_workitem;
        struct otg_workitem *hangup_workitem;
        u32 flags;                              /*!< flags */

        u32 tiocm;                              /*!< tiocm settings */

        struct serial_struct serial_struct;     /*!< serial structure used for TIOCSSERIAL and TIOCGSERIAL */

        wait_queue_head_t tiocm_wait;
        u32 tiocgicount;

        //u32 c_cflag;

        wait_queue_head_t open_wait;            /*! wait queue for blocking open*/
        int exiting;                            /*! True if module exiting */

        wait_queue_head_t send_wait;            /*! wait queue for blocking open*/
        int sending;

        struct otg_workitem *recv_workitem;
        atomic_t used;

        u8 tty_overflow_buffer[TTY_OVERFLOW_SIZE];
        atomic_t tty_overflow_used;
        int minor_num;

        //u8 tty_inflow_buffer[TTY_FLIPBUF_SIZE];
        //int tty_inflow_used;
};

/*! @name modules initialization and unload functions */

//@{
//
int tty_l26_init(char *, int);
void tty_l26_exit(void);

int tty_if_init(void);
void tty_if_exit(void);

int dun_if_init(void);
void dun_if_exit(void);

int atcom_if_init(void);
void atcom_if_exit(void);

int obex_if_init(void);
void obex_if_exit(void);

//@}
