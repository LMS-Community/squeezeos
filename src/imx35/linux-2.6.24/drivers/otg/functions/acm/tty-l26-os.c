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
 * otg/functions/acm/tty-l24-os.c
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/functions/acm/tty-l26-os.c|20070921192720|19575
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */


/*!
 * @file otg/functions/acm/tty-l26-os.c
 * @brief ACM Function Driver private defines
 *
 * This is the OS portion of the TTY version of the
 * ACM driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |  <--------
 *    Implementation  +----------+
 *
 *
 *    Function        +----------+
 *    Descriptors     | tty-if   |
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |
 *                    +----------+
 *
 *
 *
 * @ingroup ACMFunction
 * @ingroup LINUXOS
 */


#include <otg/otg-compat.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/serial.h>

#if defined(CONFIG_DEVFS_FS)
#include <linux/devfs_fs_kernel.h>
#endif /* defined(CONFIG_DEVFS_FS) */

/* XXX
 * Set this to first kernel version that does not have flip bufs
 */
#define LINUX_KERNEL_VERSION_NO_FLIP_BUF KERNEL_VERSION(2,6,17)

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include <linux/capability.h>
#include <otg/otg-trace.h>
#include "acm.h"
#include "tty.h"

/* ACM ***************************************************************************************** */

#define ACM_TTY_MAJOR   166
#define ACM_TTY_MINOR   0
#define ACM_TTY_MINORS  4

/*
 * All functions take one of struct tty or struct usbd_function_instance to
 * identify the port and/or function.
 *
 * Each of the following tables provides the linkage of the various
 * components for each minor device entry:
 *
 *      tty_table - the tty struct, provide by tty layer above us, managed by tty_l26_open/tty_l26_close
 *      function_table - the acm interface instance managed by tty_fd_enable/tty_fd_disable
 *
 * The tty struct driver_data field points at the private data structure for
 * the os layer and tty layer:
 *
 *      os_private
 *
 * The function instance privdata filed points at the private data structure
 * for the function instance:
 *
 *      acm_private
 *
 *
 *
 * N.B.
 *
 * 1. There are several combinations
 *
 *                      enabled         reset   configured       host closed host opened
 *
 *      local closed
 *
 *      local opened
 *
 *
 */

static struct tty_struct **tty_table;
static struct usbd_function_instance **function_table;
static u8 tty_minor_count;

#if defined(LINUX26)

//static struct tty_struct **tty_table;
//static struct usbd_function_instance **function_table;
//static u8 tty_minor_count;
#define TTYINDEX(tty) tty->index

#else /* defined(LINUX26) */

//static struct tty_struct ** tty_table ;
//static struct usbd_function_instance ** function_table;


#define TTYINDEX(tty) minor(tty->device)

#endif /* defined(LINUX26) */


/* ******************************************************************************************* */

void * tty_l26_wakeup_writers(void *data);
void tty_l26_recv_flip(void *data);
void * tty_l26_call_hangup(void *data);
int tty_l26_block_until_ready(struct tty_struct *tty, struct file *filp);
int tty_l26_loop_xmit_chars(struct usbd_function_instance *function_instance, minor_chan_t chan,
                int count, int from_user, const unsigned char *buf);
void tty_l26_overflow_send(struct usbd_function_instance *function_instance, minor_chan_t, BOOL force);
int tty_l26_os_recv_space_available(struct usbd_function_instance *function_instance, minor_chan_t);
int tty_l26_os_recv_chars(struct usbd_function_instance *function_instance, u8 *cp, int n);
unsigned int tty_l26_tiocm(struct usbd_function_instance *function_instance, minor_chan_t);
int tty_l26_overflow_xmit_chars(struct usbd_function_instance *function_instance, minor_chan_t chan,
                int count, int from_user, const unsigned char *buf);

void * tty_l26_recv_start(void *data);
void tty_l26_recv_start_bh(struct usbd_function_instance *function_instance);
void tty_l26_os_line_coding(struct usbd_function_instance *function_instance);

/* ******************************************************************************************* */
/* ******************************************************************************************* */
/* Linux TTY Functions
 */
/*!
 * @brief tty_l26_open
 *
 * Called by TTY layer to open device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
STATIC int tty_l26_open(struct tty_struct *tty, struct file *filp)
{

        int index = TTYINDEX(tty);
        minor_chan_t chan = CHAN(index);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        int used;
        BOOL nonblocking;
        int rc = 0;

        TRACE_MSG1(TTY,"acm: %x", acm);

//static struct tty_struct *tty_table;

        tty_table[index] = tty;

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;
        }

        otg_pthread_mutex_lock(&acm->mutex);
        UNLESS(tty->driver_data) {
                THROW_UNLESS((os_private = CKMALLOC(sizeof(struct os_private))), error);
                os_private->index = index;
                tty->driver_data = os_private;
                init_waitqueue_head(&os_private->open_wait);
                init_waitqueue_head(&os_private->tiocm_wait);
                init_waitqueue_head(&os_private->send_wait);

                 os_private->wakeup_workitem = otg_workitem_init("wakeup", tty_l26_wakeup_writers, tty, TTY);
                 os_private->hangup_workitem = otg_workitem_init("hangup", tty_l26_call_hangup, tty, TTY);
                 os_private->recv_workitem = otg_workitem_init("receive", tty_l26_recv_start, tty, TTY);
        }
        otg_pthread_mutex_unlock(&acm->mutex);


        nonblocking = BOOLEAN(filp->f_flags & O_NONBLOCK);      /* O_NONBLOCK is same as O_NDELAY */

        TRACE_MSG2(TTY,"f_flags: %08x NONBLOCK: %x", filp->f_flags, nonblocking);

        /* Lock and increment used counter, save current value.
         * Check for exclusive open at the same time.
         */
        otg_pthread_mutex_lock(&acm->mutex);

        /* The value of os_private->used controls MOD_{INC/DEC}_USE_COUNT, so
         * it has to be incremented unconditionally, and no early return
         * made until after USE_COUNT has been adjusted to match.
         */
        used = atomic_post_inc(&os_private->used);
#ifdef MCEL
        filp->f_flags |= O_EXCL;  // QQQ Can we persuade MCEL to add this to their app?
        if (nonblocking && acm && !(acm->connected)) {
                // QQQ Is MCEL actually using this "feature"?  (See below for printk)
                rc = -EINVAL;
        }
        else
#endif
#if 0
        if (filp->f_flags & O_EXCL) {
                /* This is intended to be an exclusive open, so
                 * make sure no one has the device open already, and
                 * set the exclusive flag so no one can open it later.
                 */
                if (used > 0) {
                        // Someone already has it.
                        rc = -EBUSY;
                }
                else {
                        os_private->flags |= TTYFD_EXCLUSIVE;
                        set_bit(TTY_EXCLUSIVE, &tty->flags);
                }
        }
        if ((os_private->flags & TTYFD_EXCLUSIVE) && !capable(CAP_SYS_TTY_CONFIG))
        {
                // Only the superuser can do a normal open of an O_EXCL tty
                rc = -EBUSY;
        }
#endif
        otg_pthread_mutex_unlock(&acm->mutex);

        // OK, now it's safe to make an early return if we are failing.
        if (rc) {
#ifdef MCEL
                // This can dissappear when the "feature" above does.
                if (-EINVAL == rc)
                        //printk(KERN_INFO "\nusb cable not connected!\n");
#endif
                return rc;
        }


        /* To truly emulate the old dual-device approach of having a non-blocking
         * device (e.g cu0) and a blocking device (e.g. tty0) we would need to
         * track blocking and non-blocking opens separately.  We don't.  This
         * may lead to funny behavior in the multiple open case.
         */
        UNLESS (used) {

                // First open.
                TRACE_MSG2(TTY, "FIRST OPEN nonblocking: %x exclusive: %x", nonblocking, os_private->flags & TTYFD_EXCLUSIVE);
                tty->low_latency = 1;
                if (function_instance) {
                        acm_open(function_instance, chan);
                        acm_set_local(function_instance, 1);
                }
                os_private->flags |= TTYFD_OPENED;
        }


        /* All done if configured
         */
        //RETURN_ZERO_UNLESS(acm_ready(function_instance));

        /* All done if non blocking open
         */
        RETURN_ZERO_IF(nonblocking);

        /* Block open - wait until ready
         */
        TRACE_MSG0(TTY, "BLOCKING - wait until ready");
        return tty_l26_block_until_ready(tty, filp);

        /* The tty layer calls tty_l26_close() even if this open fails,
         * so any cleanup (rc != 0) will be done there.
         */
        //TRACE_MSG3(acm->trace_tag,"used: %d MOD_IN_USE: %d configured: %d",
        //                atomic_read(&os_private->used), MOD_IN_USE, acm->flags & ACM_CONFIGURED);


        CATCH(error) {
                return -ENOMEM;
        }

}

/*!
 * @brief tty_l26_close
 *
 * Called by TTY layer to close device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
STATIC void tty_l26_close(struct tty_struct *tty, struct file *filp)
{
        int index = TTYINDEX(tty);
        minor_chan_t chan = CHAN(index);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        int used;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        TRACE_MSG0(TTY, "entered");

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;
        }


        // lock and decrement used counter, save result
        used = atomic_pre_dec(&os_private->used);

        // finished unless this is the last close

        RETURN_IF (used > 0);

        TRACE_MSG0(TTY,"FINAL CLOSE");

        /* reset so no further operations can be started.
         */
        //tty->driver_data = NULL;

        if (function_instance) {

                /* send any overflow data if neccessary
                 */
                tty_l26_overflow_send(function_instance, chan, TRUE);

                /* tell acm layer to close
                 */
                acm_close(function_instance, chan);
        }

        //tty->driver_data = function_instance[index];

        /* This should never happen if this is the last close,
         * but it can't hurt to check.
         */
        //if (os_private->open_wait_count)
        wake_up_interruptible(&os_private->open_wait);

        if (os_private->flags & TTYFD_EXCLUSIVE) {
                os_private->flags &= ~TTYFD_EXCLUSIVE;
                if (tty)
                        clear_bit(TTYFD_EXCLUSIVE, &tty->flags);
        }

        // XXX wait

   otg_workitem_exit(os_private->wakeup_workitem);
   otg_workitem_exit(os_private->hangup_workitem);
   otg_workitem_exit(os_private->recv_workitem);

        if (os_private) LKFREE(os_private);
        // XXX moved up tty->driver_data = NULL;
        tty->driver_data = NULL;

        TRACE_MSG0(TTY,"FINISHED");

}

/* ******************************************************************************************* */


/*!
 * @brief tty_l26_block_until_ready
 *
 * Called from tty_l26_open to implement blocking open. Wait for DTR indication
 * from acm-fd.
 *
 * Called by TTY layer to open device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
int tty_l26_block_until_ready(struct tty_struct *tty, struct file *filp)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        int rc = 0;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        for (;;) {
                /* check if the file has been closed */
                if (tty_hung_up_p(filp)) {
                        TRACE_MSG0(TTY,"tty_hung_up_p()");
                        return -ERESTARTSYS;
                }
                /* check for pending signals */
                if (signal_pending(current)) {
                        TRACE_MSG0(TTY,"signal_pending()");
                        return -ERESTARTSYS;
                }
                /* check if the module is unloading */
                if (os_private->exiting) {
                        TRACE_MSG0(TTY,"module exiting()");
                        return -ENODEV;
                }
                /* check for what we want, do we have DCD (RTS from host POV)?
                 */
                if (acm_get_d1_rts(function_instance)) {
                        // OK, there's somebody on the other end, let's go...
                        TRACE_MSG0(TTY,"found CAR");
                        return 0;
                }
                /* sleep */
                interruptible_sleep_on(&os_private->open_wait);
        }
        /* NOT REACHED */
}

/* Transmit Function - called by tty layer ****************************************************** */

/*!
 * @brief tty_l26_chars_in_buffer
 *
 * Called by TTY layer to determine amount of pending write data.
 *
 * @param tty - pointer to tty data structure
 * @return amount of pending write data
 */
STATIC int tty_l26_chars_in_buffer(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        minor_chan_t chan = CHAN(index);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        int rc;

        TRACE_MSG1(TTY,"tty: %x", (int)tty);

        rc = acm_chars_in_buffer(function_instance, chan) + atomic_read(&os_private->tty_overflow_used);

        TRACE_MSG1(TTY, "chars in buffer: %d", rc);

        return rc;
}

#if !defined(LINUX26)
/*! tty_l24_write
 *
 * Called by TTY layer to write data.
 * @param tty - pointer to tty data structure
 * @param from_user - true if from user memory space
 * @param buf - pointer to data to send
 * @param  count - number of bytes want to send.
 * @return count - number of bytes actually sent.
 */
int tty_l24_write(struct tty_struct *tty, int from_user, const unsigned char *buf, int count)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        minor_chan_t chan = CHAN(index);

        TRACE_MSG4(TTY,"tty: %x -> count: %d loopback: %d from_usr: %d", tty, count, os_private->tiocm & TIOCM_LOOP, from_user);

        RETURN_ZERO_UNLESS(acm);

        /* loopback mode
         */
        if (os_private->tiocm & TIOCM_LOOP)
                return tty_l26_loop_xmit_chars(function_instance, chan, count, from_user, buf);

        /* overflow mode
         */
        if (atomic_read(&os_private->tty_overflow_used) || (tty_l26_chars_in_buffer(tty) && (count < 64)))
                return tty_l26_overflow_xmit_chars(function_instance, chan, count, from_user, buf);

        /* straight through mode
         */
        return acm_xmit_chars(function_instance, chan, count, from_user, buf);
}
#endif /* !defined(LINUX26) */

#if defined(LINUX26)
/*! tty_l26_write
 *
 * Called by TTY layer to write data.
 * @param tty - pointer to tty data structure
 * @param from_user - true if from user memory space
 * @param buf - pointer to data to send
 * @param  count - number of bytes want to send.
 * @return count - number of bytes actually sent.
 */
int tty_l26_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        minor_chan_t chan = CHAN(index);
        int total_sent_chars= 0;
        int sent_chars;


        TRACE_MSG3(TTY,"sos909 tty: %x -> count: %d loopback: %d from_usr: %d", tty, count, os_private->tiocm & TIOCM_LOOP);

        #if 0
        {
                int i;
                const char *cp = buf;
                for (i = 0; i < count; i += 8) {
                        TRACE_MSG8(TTY, " SEND [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                        );

                }

        }
        #else

                //if (count>0) printk(KERN_INFO "808 tty =>USB: Num:%d\n",count);
        #endif
        RETURN_ZERO_UNLESS(acm);

        /* loopback mode
         */
        if (os_private->tiocm & TIOCM_LOOP)
                return tty_l26_loop_xmit_chars(function_instance, chan, count, 0, buf);

                /* overflow mode
                 */
                if (atomic_read(&os_private->tty_overflow_used) || (tty_l26_chars_in_buffer(tty) && (count < 64)))
                return tty_l26_overflow_xmit_chars(function_instance, chan, count, 0, buf);

        /* straight through mode
         */

        while ((sent_chars = acm_xmit_chars(function_instance, chan, count - total_sent_chars, 0, buf + total_sent_chars))) {
                total_sent_chars += sent_chars;
        }
        return total_sent_chars;
}

#endif /* defined(LINUX26) */

/*! tty_l26_write_room
 *
 * Called by TTY layer get amount of write room available.
 *
 * @param tty - pointer to tty data structure
 * @return amount of write room available
 */
STATIC int tty_l26_write_room(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        minor_chan_t chan = CHAN(index);
        int rc;

        TRACE_MSG1(TTY,"tty: %x", (int)tty);
        /* loopback mode
         */
        if (os_private->tiocm & TIOCM_LOOP)
                return tty_l26_os_recv_space_available(function_instance, chan);

        RETURN_ZERO_UNLESS(acm);
#if 0
        rc = acm_write_room(function_instance, chan) +
                (MIN(TTY_OVERFLOW_SIZE, acm->writesize) - os_private->tty_overflow_used, chan);

        TRACE_MSG1(TTY, "write room: %d", rc);

        return rc;
#else

        rc = acm_write_room(function_instance, chan);
        if(rc) rc = (MIN(TTY_OVERFLOW_SIZE, acm->writesize) - atomic_read(&os_private->tty_overflow_used));

        TRACE_MSG1(TTY, "write room: %d", rc);

        return rc;
#endif


}

static int throttle_count = 0;
static int unthrottle_count = 0;

/*! tty_l26_throttle
 *
 * Called by TTY layer to throttle (do not allow received data.)
 *
 * @return amount of write room available
 */
STATIC void tty_l26_throttle(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        minor_chan_t chan = CHAN(index);

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG1(TTY,"tty: %x", (int)tty);
        os_private->flags |= TTYFD_THROTTLED;
        acm_throttle(function_instance, chan);

}

/*! tty_l26_unthrottle
 *
 * Called by TTY layer to unthrottle (allow received data.)
 *
 * @return amount of write room available
 */
STATIC void tty_l26_unthrottle(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        minor_chan_t chan = CHAN(index);

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG1(TTY,"tty: %x", (int)tty);
        os_private->flags &= ~TTYFD_THROTTLED;
        acm_unthrottle(function_instance, chan);

        /* possible recv was waiting
         */
        //acm_restart_recv(function_instance);
        //tty_l26_recv_start_bh(function_instance);


        /* This function is called while the TTY_DONT_FLIP flag is still
         * set, so there is no point trying to push the flip buffer.  Just
         * try to queue some recv urbs, and keep trying until we do manage
         * to get some queued.
         */
        //tty_schedule_flip(tty);

}


unsigned int tty_l26_tiocm(struct usbd_function_instance *function_instance, minor_chan_t chan);

static int tty_l26_tiocmset(struct tty_struct *tty, struct file *file,
                unsigned int set, unsigned int clear);
/*! tty_l26_fd_ioctl
 *
 * Used by TTY layer to execute IOCTL command.
 *
 * Called by TTY layer to open device.
 *
 * Unhandled commands must return -ENOIOCTLCMD for correct operation.
 *
 * @param tty
 * @param file
 * @param cmd
 * @param arg
 * @returns non-zero for error
 */
STATIC int tty_l26_fd_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
        int index = TTYINDEX(tty);
        minor_chan_t chan = CHAN(index);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];

        unsigned int mask;
        unsigned int newctrl;
        int rc;

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //TRACE_MSG3(TTY,"tty: %x cmd: %08x arg: %08x", (int)tty, cmd, arg);
        TRACE_MSG3(TTY,"entered: %8x dir: %x size: %x", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));

        switch(chan) {

        case data_chan:
                break;
        case command_chan:
        default:
                break;

        }

        switch(cmd) {
#if !defined(LINUX26)
        case TIOCMGET:
                //TRACE_MSG1(TTY, "TIOCMGET: tiocm: %08x", tiocm);
                os_private->tiocm = tty_l26_tiocm(function_instance, chan);
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &os_private->tiocm, sizeof(int)));
                return 0;

        case TIOCMBIS:
        case TIOCMBIC:
        case TIOCMSET: {
                u32 tiocm = 0, set = 0, clear = 0;

                TRACE_MSG0(TTY, "TIOCMXXX: copying tiocm arguement");

                TRACE_MSG1(TTY, "access: %d", access_ok(VERIFY_READ, (void *)arg, sizeof(int)));
                RETURN_EINVAL_UNLESS(access_ok(VERIFY_READ, (void *)arg, sizeof(int)));


                //RETURN_EINVAL_IF (copy_from_user(&tiocm, (void *)arg, sizeof(int)));
                rc = copy_from_user((void *)&tiocm, (void *)arg, sizeof(int));
                TRACE_MSG2(TTY, "copy_from_user: %d tiocm: %08x", rc, tiocm);

                switch (cmd) {
                case TIOCMBIS:
                        set = tiocm;
                        break;
                case TIOCMBIC:
                        clear = tiocm;
                        break;
                case TIOCMSET:
                        set = tiocm;
                        clear = ~set;
                        break;
                }

                return tty_l26_tiocmset(tty, NULL, set, clear);
        }

#endif /* !defined(LINUX26) */

        case TIOCGSERIAL:
                TRACE_MSG0(TTY, "TIOCGSERIAL");
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &os_private->serial_struct, sizeof(struct serial_struct)));
                return 0;

        case TIOCSSERIAL:
                TRACE_MSG0(TTY, "TIOCSSERIAL");
                //RETURN_EFAULT_IF (copy_from_user(&os_private->serial_struct, (void *)arg, sizeof(struct serial_struct)));
                return -EINVAL;

        case TIOCSERCONFIG:
        case TIOCSERGETLSR: /* Get line status register */
        case TIOCSERGSTRUCT:
                TRACE_MSG0(TTY, "TIOCSER*");
                return -EINVAL;

                /*
                 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
                 * - mask passed in arg for lines of interest
                 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
                 * Caller should use TIOCGICOUNT to see which one it was
                 *
                 * Note: CTS is always true, RI is always false.
                 * DCD and DSR always follow DTR signal from host.
                 */
        case TIOCMIWAIT:
                TRACE_MSG0(TTY, "TIOCGMIWAIT");
                while (1) {

                        u32 saved_tiocm = os_private->tiocm;
                        u32 changed_tiocm;

                        interruptible_sleep_on(&os_private->tiocm_wait);

                        /* see if a signal did it */
                        if (signal_pending(current))
                                return -ERESTARTSYS;

                        os_private->tiocm = tty_l26_tiocm(function_instance, chan);
                        changed_tiocm = saved_tiocm ^ os_private->tiocm;
                        RETURN_ZERO_IF ( (changed_tiocm | TIOCM_CAR) | (changed_tiocm | TIOCM_DSR) );
                        return -EIO;
                        /* loop */
                }
                /* NOTREACHED */

                /*
                 * Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
                 * Return: write counters to the user passed counter struct
                 * NB: both 1->0 and 0->1 transitions are counted except for
                 *     RI where only 0->1 is counted.
                 */
        case TIOCGICOUNT:
                TRACE_MSG0(TTY, "TIOCGICOUNT");
                if (copy_to_user((void *)arg, &os_private->tiocgicount, sizeof(int)))
                        return -EFAULT;
                return 0;

        case TCSETS:
                TRACE_MSG0(TTY, "TCSETS: ");
                return -ENOIOCTLCMD;
        case TCFLSH:
                TRACE_MSG0(TTY, "TCFLSH: x");
                return -ENOIOCTLCMD;

        case TCGETS:
                TRACE_MSG0(TTY, "TCGETS: ");
                return -ENOIOCTLCMD;

        default:
                TRACE_MSG1(TTY, "unknown cmd: %08x", cmd);
                return -ENOIOCTLCMD;
        }
        return -ENOIOCTLCMD;


}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,20)
#define _TERMIOS_ ktermios
#else
#define _TERMIOS_ termios
#endif

/*! tty_l26_set_termios
 *
 * Used by TTY layer to set termios structure according to current status.
 *
 * @param tty - pointer to acm private data structure
 * @param termios_old - termios structure
 */
STATIC void tty_l26_set_termios(struct tty_struct *tty, struct _TERMIOS_ *termios_old)
{
        int index = TTYINDEX(tty);
        minor_chan_t chan = CHAN(index);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];

        unsigned int c_cflag = tty->termios->c_cflag;

        TRACE_MSG1(TTY,"tty: %x", (int)tty);

        switch(chan) {
        case data_chan:
                break;
        case command_chan:
        default:
                break;

        }

        /* see if CLOCAL has changed */
        if ((termios_old->c_cflag ^ tty->termios->c_cflag ) & CLOCAL)
                acm_set_local(function_instance, tty->termios->c_cflag & CLOCAL);

        /* save cflags */
        //os_private->c_cflag = c_cflag;

        /* send break?  */
        if ((termios_old->c_cflag & CBAUD) && !(c_cflag & CBAUD))
                acm_bBreak(function_instance);
}


/*! tty_l26_wait_until_sent
 *
 * Used by TTY layer to wait until pending data drains (is sent.)
 *
 * @param tty      - pointer to acm private data structure
 * @param timeout  - wait this amount of time, or forever if zero
 */
static void tty_l26_wait_until_sent(struct tty_struct *tty, int timeout)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        minor_chan_t chan = CHAN(index);

        TRACE_MSG0(TTY, "--");

        RETURN_UNLESS(acm_send_queued(function_instance));

        /* XXX This will require a wait structure and flag */
        interruptible_sleep_on(&os_private->send_wait);
}

/*! tty_l26_flush_chars
 *
 * Used by TTY layer to flush pending data to hardware.
 *
 * @param tty - pointer to acm private data structure
 */
static void tty_l26_flush_chars(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        minor_chan_t chan = CHAN(index);

        TRACE_MSG0(TTY, "--");

        /* queue data in overflow buffer */
        tty_l26_overflow_send(function_instance, chan, TRUE);
}

/*! tty_l26_flush_buffer
 *
 * Used by TTY layer to flush pending data to /dev/null (cancel.)
 *
 * @param tty - pointer to acm private data structure
 */
static void tty_l26_flush_buffer(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        minor_chan_t chan = CHAN(index);

        TRACE_MSG0(TTY, "--");

        /* clear overflow buffer */

        atomic_set(&os_private->tty_overflow_used, 0);
}


#if defined(LINUX26)
/*! tty_operations
 */
static struct tty_operations tty_ops = {

        .open =                  tty_l26_open,
        .close =                 tty_l26_close,
        .write =                 tty_l26_write,
        .write_room =            tty_l26_write_room,
        .ioctl =                 tty_l26_fd_ioctl,
        .throttle =              tty_l26_throttle,
        .unthrottle =            tty_l26_unthrottle,
        .chars_in_buffer =       tty_l26_chars_in_buffer,
        .set_termios =           tty_l26_set_termios,
        #if 0
        .wait_until_sent =       tty_l26_wait_until_sent,
        .flush_chars =           tty_l26_flush_chars,
        .flush_buffer =          tty_l26_flush_buffer,
        #endif
};

/*! tty_driver
 */
static struct tty_driver *tty_driver;

#else /* defined(LINUX26) */

static int tty_refcount;
static struct tty_struct *tty_table1[ACM_TTY_MINORS];
static struct termios *tty_termios[ACM_TTY_MINORS];
static struct termios *tty_termios_locked[ACM_TTY_MINORS];

static struct tty_driver tty_driver = {


        .open =                  tty_l26_open,
        .close =                 tty_l26_close,
        .write =                 tty_l24_write,
        .write_room =            tty_l26_write_room,
        .ioctl =                 tty_l26_fd_ioctl,
        .throttle =              tty_l26_throttle,
        .unthrottle =            tty_l26_unthrottle,
        .chars_in_buffer =       tty_l26_chars_in_buffer,
        .set_termios =           tty_l26_set_termios,

        .refcount =              &tty_refcount,
        .table =                 tty_table1,
        .termios =               tty_termios,
        .termios_locked =        tty_termios_locked,
};

#endif /* defined(LINUX26) */

/* Transmit Function - called by tty layer ****************************************************** */

/* ******************************************************************************************* */
/* ******************************************************************************************* */
/* TTY OS Functions
 */

#define LOOP_BUF 16
/*!
 * @brief tty_l26_loop_xmit_chars
 *
 * Implement data loop back, send xmit data back as received data.
 *
 * @param function_instance
 * @param chan
 * @param count - number of bytes to send
 * @param from_user - true if from user memory space
 * @param buf - pointer to data to send
 * @return count - number of bytes actually sent.
 */
int tty_l26_loop_xmit_chars(struct usbd_function_instance *function_instance, minor_chan_t chan,
                int count, int from_user, const unsigned char *buf)
{

        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty->driver_data;
        int received = 0;
        int rc=0;

        TRACE_MSG2(TTY,"acm[%x] %x", chan, acm);

        while (count > 0) {
                u8 copybuf[LOOP_BUF];
                int i;
                int space = tty_l26_os_recv_space_available(function_instance, chan);
                int recv = MIN (space, LOOP_BUF);
                recv = MIN(recv, count);

                //TRACE_MSG7(TTY, "buf: %8x buf+received: %8x received: %d from_user: %d count: %d space: %d recv: %d\n",
                //                buf, buf + received, received, from_user, count, space, recv);

                BREAK_UNLESS(recv);

                if (from_user)
                        rc=copy_from_user ((void *)copybuf, (void*)(buf + received), recv);
                else
                        memcpy ((void *)copybuf, (void*)(buf + received), recv);

                count -= recv;
                received += recv;
                TRACE_MSG3(TTY, "count: %d recv: %d received: %d", count, recv, received);
                tty_l26_os_recv_chars(function_instance, copybuf, recv);
        }
        return received;
}

/*!
 * @brief tty_l26_overflow_send
 *
 * Check if there is data in overflow buffer and send if neccessary.
 *
 * @param function_instance - pointer to acm private data structure
 * @param chan
 * @param force - force the send
 * @return none
 */
void tty_l26_overflow_send(struct usbd_function_instance *function_instance, minor_chan_t chan, int force)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty->driver_data;
        int chars_in_buffer;
        int count;

        TRACE_MSG2(TTY,"acm[%x] %x", chan, acm);

        otg_pthread_mutex_lock(&acm->mutex);

        chars_in_buffer = acm_chars_in_buffer(function_instance, chan);

        TRACE_MSG3(TTY, "overflow_used: %d chars_in_buffer: %d force: %d",
                        atomic_read(&os_private->tty_overflow_used), chars_in_buffer, force);

        if (TRUE || !chars_in_buffer || (atomic_read(&os_private->tty_overflow_used) > 200)) {

                count = acm_xmit_chars(function_instance, chan, atomic_read(&os_private->tty_overflow_used),
                                0, os_private->tty_overflow_buffer);

                TRACE_MSG2(TTY, "overflow_used: %d count: %d", atomic_read(&os_private->tty_overflow_used), count);

                if((count==0) && (atomic_read(&os_private->tty_overflow_used)>0))
                {
                }else{
                        atomic_set(&os_private->tty_overflow_used, 0);
                }

        }
        otg_pthread_mutex_unlock(&acm->mutex);
}


/*!
 * @brief tty_l26_overflow_xmit_chars
 *
 * Implement overflow buffer. The TTY layer implements echo with single
 * character writes which can quickly exhaust the urbs allowed for sending.
 * This results in data being lost.
 *
 * This function is called to accumulate small amounts of data when there is
 * already an bulk in urb in the queue.
 *
 * The urb sent function calls wakeup writers which will call
 * tty_l26_overflow_send which will send the data.
 *
 * @param function_instance - pointer to acm private data structure
 * @param chan
 * @param count - number of bytes to send
 * @param from_user - true if from user memory space
 * @param buf - pointer to data to send
 * @return count - number of bytes actually sent.
 */
int tty_l26_overflow_xmit_chars(struct usbd_function_instance *function_instance, minor_chan_t chan,
                int count, int from_user, const unsigned char *buf)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty->driver_data;
        int overflow_room;
        int write_room = acm_write_room(function_instance, chan);
        int sent = 0;
        int rc=0;

        TRACE_MSG4(TTY,"acm[%x] %x count: %d from_user:%d", chan, acm, count, from_user);

        RETURN_ZERO_UNLESS(acm);

        do {
                otg_pthread_mutex_lock(&acm->mutex);

                if ((overflow_room = MIN(TTY_OVERFLOW_SIZE, acm->writesize) - atomic_read(&os_private->tty_overflow_used)) > 0) {

                        int tosend = MIN(overflow_room, count);
                        if (from_user)
                                rc=copy_from_user ((void *)os_private->tty_overflow_buffer + atomic_read(&os_private->tty_overflow_used),
                                                (void*)buf, tosend);
                        else
                                memcpy ((void *)os_private->tty_overflow_buffer + atomic_read(&os_private->tty_overflow_used),
                                                (void*)buf, tosend);

                        atomic_add(tosend, &os_private->tty_overflow_used);
                        count -= tosend;
                        buf += tosend;
                        sent += tosend;
                }
                else
                {
                        count = 0;
                }
                otg_pthread_mutex_unlock(&acm->mutex);

                /* start sending from overflow if neccessary
                 */
                tty_l26_overflow_send(function_instance, chan, count ? TRUE : FALSE);
        } while (count);

        return sent;
}
/*! tty_l26_setbit - set specific bit
 * @brief set specific bit and output message
 *
 * @param result - new value after setbit
 * @param mask   - bit to set
 * @param value - bool to decide whether to set
 * @param msg - message to output
 * @return new value after set bit
 */

static int tty_l26_setbit(u32 result, u32 mask, BOOL value, char *msg)
{
        UNLESS (value)
                return result;

        result |= mask;

        TRACE_MSG3(TTY, "result: %04x mask: %04x %s", result, mask, msg);
        return result;
}

/*!
 * @brief tty_l26_tiocm
 * @param function_instance
 * @param chan
 * @return - computed tiocm value
 */
unsigned int tty_l26_tiocm(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        u32 tiocm = 0;
        TRACE_MSG2(TTY,"acm[%x] %x", chan, (int)function_instance ? function_instance->privdata : NULL );
        switch(chan) {
        case data_chan:
                tiocm = tty_l26_setbit(tiocm, TIOCM_DTR, acm_get_bRxCarrier(function_instance), "TIOCM_DTR (bRxCarrier)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_RTS, !acm_get_throttled(function_instance), "TIOCM_RTS (TRUE)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_CTS, acm_get_d0_dtr(function_instance), "TIOCM_CTS (TRUE)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_CAR, acm_get_d1_rts(function_instance), "TIOCM_CAR (D1 RTS)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_RI, FALSE, "TIOCM_RING");
                tiocm = tty_l26_setbit(tiocm, TIOCM_DSR, acm_get_d0_dtr(function_instance), "TIOCM_DSR (D0 DTR)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_OUT1, FALSE, "TIOCM_OUT1 (FALSE)");
                tiocm = tty_l26_setbit(tiocm, TIOCM_OUT2, FALSE, "TIOCM_OUT2 (FALSE)");
                return tiocm;

        case command_chan:
        default:
                return 0;
        }
}

#if defined(LINUX26) || defined(LINUX24)

/*! tty_l26_tiocmget
 *
 *              Peripheral      Host            Internal
 * TIOCM_DTR    bRxCarrier
 * TIOCM_RTS    Always TRUE
 * TIOCM_CTS                    Always TRUE
 * TIOCM_CAR                    D1
 * TIOCM_RI                     Always FALSE
 * TIOCM_DSR                    D0
 * TIOCM_OUT1   bRingSignal
 * TIOCM_OUT2   bOverrrun
 * TIOCM_LOOP                                   Loopback
 */
static int tty_l26_tiocmget(struct tty_struct *tty, struct file *file)
{

        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned int result = 0;
        unsigned int msr = 0;
        unsigned int mcr = 0;

        TRACE_MSG1(TTY, "acm: %x", (int)acm);
        RETURN_ZERO_UNLESS(acm);

        return tty_l26_tiocm(function_instance, data_chan);

}

/*! tty_l26_tiocsget
 *              Peripheral      Host            Internal
 * TIOCM_DTR    bRxCarrier
 * TIOCM_RTS    Always TRUE
 * TIOCM_OUT1   bRingSignal
 * TIOCM_OUT2   bOverrun
 * TIOCM_LOOP                                   Loopback
 *
 */
static int tty_l26_tiocmset(struct tty_struct *tty, struct file *file,
                unsigned int set, unsigned int clear)
{
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned int result = 0;
        unsigned int msr = 0;
        unsigned int mcr = 0;

        TRACE_MSG3(TTY,"acm: %x set: %04x clear: %04x", (int)acm, set, clear);
        RETURN_ZERO_UNLESS(acm);


        /* Set DTR -> DTR */
        if (set & TIOCM_DTR)
                acm_set_bRxCarrier(function_instance, 1);
        if (clear & TIOCM_DTR)
                acm_set_bRxCarrier(function_instance, 0);

        /* Set Loop back */
        if (set & TIOCM_LOOP)
                acm_set_loopback(function_instance, 1);
        if (clear & TIOCM_LOOP)
                acm_set_loopback(function_instance, 0);

        /* OUT1->Ring */
        if ((set & TIOCM_OUT1))
                acm_bRingSignal(function_instance);

        /* OUT2->Overrun */
        if ((set & TIOCM_OUT2))
                acm_bOverrun(function_instance);

        /* !RTS->Break */
        if (set & TIOCM_RTS)
                acm_set_throttle(function_instance, 0);
        if (clear & TIOCM_RTS)
                acm_set_throttle(function_instance, 1);

        return 0;
}

/*! tty_l26_break_ctl
 */
static int tty_l26_break_ctl(struct tty_struct *tty, int flag)
{
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);

        if (flag)
                acm_bBreak(function_instance);
        return 0;
}

#endif /* defined(LINUX26) */
/* ********************************************************************************************** */
/* ********************************************************************************************* */

/*!
 * @brief tty_l26_call_hangup
 *
 * Bottom half handler to safely send hangup signal to process group.
 *
 * @param data
 * @return none
 */
void * tty_l26_call_hangup(void *data)
{
        struct tty_struct *tty = data;
        struct os_private *os_private = tty ? tty->driver_data : NULL;

        /* XXX Do we need to protect this
         */
        RETURN_NULL_UNLESS(tty && os_private);

        TRACE_MSG3(TTY,"tty: %x c_cflag: %02x CLOCAL: %d", tty,
                        tty->termios->c_cflag,
                        BOOLEAN(tty->termios->c_cflag & CLOCAL)
                  );


        //if (tty && !(os_private->c_cflag & CLOCAL))
        tty_hangup(tty);

        wake_up_interruptible(&os_private->open_wait);

        TRACE_MSG0(TTY,"exited");
        return NULL;
}

/*!
 * @brief tty_l26_os_schedule_hangup
 *
 * Schedule hangup bottom half handler.
 *
 * @param function_instance
 * @return none

 */
void tty_l26_os_schedule_hangup(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        RETURN_UNLESS(os_private);

        TRACE_MSG0(TTY, "entered");
        //tty_l26_schedule(tty, &os_private->hqueue);
        otg_workitem_start(os_private->hangup_workitem);
}


/*! tty_l26_wakeup_writers
 *
 * Bottom half handler to wakeup pending writers.
 *
 * @param data - pointer to acm private data structure
 */
void * tty_l26_wakeup_writers(void *data)
{
        struct tty_struct *tty = data;
        struct os_private *os_private = tty ? tty->driver_data : NULL;
        int index = os_private->index;
        minor_chan_t chan = CHAN(index);
        struct usbd_function_instance *function_instance = function_table[index];


        unsigned long flags;

        TRACE_MSG1(TTY,"tty: %x", tty);

        /* XXX Do we need to protect this
         */
        RETURN_NULL_UNLESS(os_private);

        //TRACE_MSG2(TTY,"used: %d MOD_IN_USE: %d", atomic_read(&os_private->used), MOD_IN_USE);

        RETURN_NULL_UNLESS(acm_ready(function_instance));
        RETURN_NULL_UNLESS(atomic_read(&os_private->used));
        RETURN_NULL_UNLESS(tty);

        /* start sending from overflow buffer if necessary
         */
        tty_l26_overflow_send(function_instance, chan, FALSE);

        if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup)
                (tty->ldisc.write_wakeup)(tty);

        wake_up_interruptible(&tty->write_wait);
        return NULL;
}

/*!
 * @brief tty_l26_os_wakeup_opens
 *
 * Called by acm_fd to wakeup processes blocked in open waiting for DTR.
 *
 * @param function_instance
 * @return none
 */
void tty_l26_os_wakeup_opens(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        TRACE_MSG1(TTY,"acm: %x", acm);

        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(os_private);

        //if (os_private->open_wait_count > 0)
        wake_up_interruptible(&os_private->open_wait);
}

/*!
 * @brief  tty_l26_os_schedule_wakeup_writers
 *
 * Called by acm-fd to schedule a wakeup writes bottom half handler.
 *
 * @param function_instance - pointer to acm private data structure
 * @return none
 */
void tty_l26_os_schedule_wakeup_writers(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(os_private);

        //tty_l26_schedule(tty, &os_private->wqueue);
        otg_workitem_start(os_private->wakeup_workitem);
        // verify ok
        wake_up_interruptible(&os_private->send_wait);
}

/*!
 * @brief tty_l26_os_wakeup_state
 *
 * Called by acm_fd to wakeup processes blocked waiting for state change
 *
 * @param function_instance - pointer to acm private data structure
 * @return none
 */
void tty_l26_os_wakeup_state(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(os_private);

        TRACE_MSG0(TTY, "entered");
        wake_up_interruptible(&os_private->tiocm_wait);
}


/* ********************************************************************************************* */

/*!
 * @brief tty_l26_os_recv_space_available
 *
 * Used by acm-fd to determine receive space available. This will determine
 * how many receive urbs can be queued as there are never more receive urbs
 * pending than there is currently room for data to be received.
 *
 * @param function_instance
 * @param chan
 * @return count - number of bytes available
 */
int tty_l26_os_recv_space_available(struct usbd_function_instance *function_instance, minor_chan_t chan)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : NULL;
        int room_len = 0;
        int rc;

        //TRACE_MSG1(TTY,"acm: %x", (int)acm);

        RETURN_ZERO_UNLESS(tty);
#if 1
        //room_len=tty_buffer_request_room(tty, TTY_FLIPBUF_SIZE);
        TRACE_MSG1(TTY,"OS space room:%d",room_len);
        printk(KERN_INFO"%s: space room:%d, TTY_FLIPBUF_SIZE:%d\n", __FUNCTION__,room_len,TTY_FLIPBUF_SIZE);
        return room_len;

#else

#if LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF
        UNLESS (!tty->flip.count || test_bit(TTY_THROTTLED, &tty->flags) || test_bit(TTY_DONT_FLIP, &tty->flags)) {
                TRACE_MSG2(TTY, "FLIPPING count: %d available: %d", tty->flip.count, TTY_FLIPBUF_SIZE - tty->flip.count);
                tty_flip_buffer_push(tty);
        }
        TRACE_MSG7(TTY, "FLIP count: %d available: %d read_cnt: %d minimum_to_wake: %d  %s %s %s",
                        tty->flip.count,
                        TTY_FLIPBUF_SIZE - tty->flip.count,
                        tty->read_cnt, tty->minimum_to_wake,
                        tty->flags & TTY_THROTTLED ? "THROTTLED" : "",
                        tty->flags & TTY_DONT_FLIP ? "DONT_FLIP" : "",
                        tty->real_raw ? "REAL RAW" : ""
                  );
        /* not only do we have to test the local flip buffer for space, but if
         * we are in raw mode check the n_tty buffer availability.
         *
         * In n_tty_receive_buf(), when in raw mode, the data is saved into
         * a 4kb wrap around ring buffer. So we must ensure that we never
         * attempt push more data to that layer than will fit.
         */
        return MIN( (tty->real_raw ?  (N_TTY_BUF_SIZE - acm->readsize - tty->read_cnt) : TTY_FLIPBUF_SIZE ),
                        (TTY_FLIPBUF_SIZE - acm->readsize) - tty->flip.count);
#else /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */
        UNLESS((TTY_FLIPBUF_SIZE != tty_buffer_request_room(tty, TTY_FLIPBUF_SIZE))
                        || test_bit(TTY_THROTTLED, &tty->flags)
                        ) {
                tty_flip_buffer_push(tty);
        }
        return MIN((tty->real_raw ? (N_TTY_BUF_SIZE - acm->readsize - tty->read_cnt) : TTY_FLIPBUF_SIZE),
                        (tty_buffer_request_room(tty, TTY_FLIPBUF_SIZE) - acm->readsize));

#endif /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */
#endif
}


/*!
 * @brief tty_l26_os_recv_chars
 *
 * Called by acm-fd when data has been received. This will
 * receive n bytes starting at cp.  This will never be
 * more than the last call to tty_l26_os_recv_space_available().
 * This will be called from interrupt context.
 *
 * @param function_instance
 * @param n - number of bytes to send
 * @param cp - pointer to data to send
 * @return non-zero if error
 */
int tty_l26_os_recv_chars(struct usbd_function_instance *function_instance, u8 *cp, int n)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        int buf_size;
        RETURN_EINVAL_UNLESS(tty);
#if 1
        otg_pthread_mutex_lock(&acm->mutex);
        buf_size=tty_buffer_request_room(tty,n);
        //printk(KERN_INFO"%s: USB got data and push to tty:%d, buf_size:%d\n", __FUNCTION__,n,buf_size);
        tty_insert_flip_string(tty, cp, buf_size);
        tty_flip_buffer_push(tty);
        otg_pthread_mutex_unlock(&acm->mutex);
        return 0;
#else

#if LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF
        if (TTY_FLIPBUF_SIZE < (tty->flip.count + n)) {
                TRACE_MSG3(TTY, "TTY_FLIP_BUF_SIZE: %d count: %d n: %d", TTY_FLIPBUF_SIZE, tty->flip.count, n);
                printk(KERN_INFO"%s: FLIP BUFFER OVERFLOW ERROR\n", __FUNCTION__);
                return -EINVAL;
        }
#endif /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */

#if 1
        if (tty->real_raw) {
                if ((N_TTY_BUF_SIZE - 1) < (tty->read_cnt + n)) {
                        TRACE_MSG3(TTY, "N_TTY_BUF_SIZE::%d, count: %d n: %d", N_TTY_BUF_SIZE, tty->read_cnt, n);
                        printk(KERN_INFO"%s: N_TTY_BUF_SIZE:%d, tty->read_cnt: %d  just rcv n: %d\n",  __FUNCTION__, N_TTY_BUF_SIZE, tty->read_cnt, n);
                        printk(KERN_INFO"%s: N_TTY BUFFER OVERFLOW ERROR\n", __FUNCTION__);
                        return -EINVAL;
                }

        }
#endif

        otg_pthread_mutex_lock(&acm->mutex);

#if LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF
        TRACE_MSG5(TTY, "tty: %x cb: %x fb: %x count: %d buf: %d",
                        tty, tty->flip.char_buf_ptr, tty->flip.flag_buf_ptr, tty->flip.count, tty->flip.buf_num);
#endif /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */


         TRACE_MSG2(TTY, " sos909 FLIP: %d buffer: %x", n, cp);
        #if 1

        {

                int i;

                for (i = 0; i < n; i += 8) {
                        TRACE_MSG8(TTY, "sos909 %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                  );
                }

        }
        #else


                int i;
                char prompt[1028];
                for (i = 0; i < n; i++) sprintf((prompt+3*i),"%02X ",cp[i]);
                //TRACE_MSG2(TTY,"808 Hardware to tty: num%d, %s\n",n,prompt);

                if (n>0) printk(KERN_INFO "808 tty<=USB: Num: %d|| %s\n\n",n,prompt);
        #endif

#if LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF
        memcpy(tty->flip.char_buf_ptr, cp, n);
        memset(tty->flip.flag_buf_ptr, TTY_NORMAL, n);
        tty->flip.count += n;
        tty->flip.char_buf_ptr += n;
        tty->flip.flag_buf_ptr += n;
        atomic_add(n, &acm->bytes_forwarded);
        TRACE_MSG5(TTY, "tty: %x cb: %x fb: %x count: %d buf: %d",
                        tty, tty->flip.char_buf_ptr, tty->flip.flag_buf_ptr, tty->flip.count, tty->flip.buf_num);

#else /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */
        n = tty_insert_flip_string(tty, cp, n);
#endif /* LINUX_VERSION_CODE < LINUX_KERNEL_VERSION_NO_FLIP_BUF */
        tty_flip_buffer_push(tty);
        //otg_led(LED1, FALSE);
        otg_pthread_mutex_unlock(&acm->mutex);
        return 0;
#endif

}


/* ********************************************************************************************* */

/*!
 * @brief tty_l26_recv_start
 *
 * Bottom half handler to restart acm recv queue...
 *
 * This is implemented here because the background scheduling may b3 OS
 * dependant.. but it must call the common acm_start_recv_urbs() function to
 * do the work.
 *
 * @param data
 * @return none
 */
void * tty_l26_recv_start(void *data)
{
        struct tty_struct *tty = data;
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct os_private *os_private = tty ? tty->driver_data : NULL;

       RETURN_NULL_UNLESS(function_instance);

        TRACE_MSG3(TTY,"tty: %x c_cflag: %02x CLOCAL: %d", tty,
                        tty->termios->c_cflag,
                        BOOLEAN(tty->termios->c_cflag & CLOCAL)
                  );

        /* keep trying to start urbs until we have enough or -1 (not ready)
         */
        while (!acm_start_recv_urbs(function_instance)) {
                struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
                TRACE_MSG0(TTY, "SLEEP START");
                interruptible_sleep_on_timeout(&acm->recv_wait, 1);
                TRACE_MSG0(TTY, "SLEEP FINISH");
        }

        TRACE_MSG0(TTY,"exited");
        return NULL;
}

/*!
 * @brief tty_l26_recv_start_bh
 *
 * Schedule hangup bottom half handler.
 *
 * @param function_instance
 * @return none

 */
void tty_l26_recv_start_bh(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        RETURN_UNLESS(os_private);

        TRACE_MSG0(TTY, "entered");

        otg_workitem_start(os_private->recv_workitem);
}

void tty_l26_os_line_coding(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;
        struct cdc_acm_line_coding *line_coding = &acm->line_coding;

        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        RETURN_UNLESS(os_private);

        TRACE_MSG0(TTY, "entered");

        os_private->serial_struct.custom_divisor = 1;
        os_private->serial_struct.baud_base = line_coding->dwDTERate;
        os_private->serial_struct.flags = line_coding->bCharFormat << 16 |
                line_coding->bParityType << 8 | line_coding->bDataBits;
}

/* ********************************************************************************************* */
/*!
 * @brief  tty_l26_os_enable
 *
 * Called by acm-fd when the function driver is enabled.
 * @param function_instance
 * @return int
 */
int tty_l26_os_enable(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index;

        TRACE_MSG1(TTY,"acm: %x", acm);
        // XXX MODULE LOCK HERE

        // XXX should do something based on wIndex...
        for (index = 0; index < acm->minors; index++) {
                struct tty_struct *tty;
                struct os_private *os_private;
                CONTINUE_IF (function_table[index]);
                function_table[index] = function_instance;
                acm->index = index;
                tty = tty_table[index];
                os_private = tty ? tty->driver_data : NULL;

                TRACE_MSG1(TTY, "OS Flags: %x", os_private ? os_private->flags : 0);

                RETURN_ZERO_UNLESS(os_private && (os_private->flags & TTYFD_OPENED));
                acm->flags |= ACM_OPENED;
                tty_l26_os_schedule_wakeup_writers(function_instance);

                return 0;
        }
        // XXX MODULE LOCK HERE
        return -EINVAL;
}

/*!
 * @brief tty_l26_os_disable
 * @param function_instance
 * @return none
 *
 * Called by acm-fd when the function driver is disabled.
 */
void tty_l26_os_disable(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        TRACE_MSG1(TTY,"acm: %x", acm);
        function_table[index] = NULL;
        // XXX MODULE UNLOCK HERE
}

/*! \var struct  acm_os_ops tty_l26_os_ops
 * \brief acm OS operations table
 *
 */


struct acm_os_ops tty_l26_os_ops = {
        .enable = tty_l26_os_enable,
        .disable = tty_l26_os_disable,
        .wakeup_opens = tty_l26_os_wakeup_opens,
        .wakeup_state = tty_l26_os_wakeup_state,
        .schedule_wakeup_writers = tty_l26_os_schedule_wakeup_writers,
        .recv_space_available = tty_l26_os_recv_space_available,
        .recv_chars = tty_l26_os_recv_chars,
        .schedule_hangup = tty_l26_os_schedule_hangup,
        .recv_start_bh = tty_l26_recv_start_bh,
        //.comm_feature = tty_l26_os_comm_feature,
        .line_coding = tty_l26_os_line_coding,
        //.control_state = tty_l26_os_control_state,
};

/* ************************************************************************** */
/* ************************************************************************** */

/* USB Module init/exit ************************************************************************ */

/*!
 * @brief tty_l26_init - module init
 *
 * This is called immediately after the module is loaded or during
 * the kernel driver initialization if linked into the kernel.
 * @param name
 * @param minor_num
 * @return int
 */
int tty_l26_init (char *name, int minor_num)
{
        int i;

        TRACE_MSG2(TTY, "name: %s minor_num: %d", name, minor_num);


        THROW_UNLESS (tty_table = (struct tty_struct **)
                        CKMALLOC(sizeof(struct tty_struct *)* minor_num ), error) ;

        THROW_UNLESS (function_table = (struct usbd_function_instance **)
                        CKMALLOC(sizeof(struct usbd_function_instance *) * minor_num), error) ;

        tty_minor_count = minor_num;

#if defined(LINUX26)

        THROW_UNLESS((tty_driver = alloc_tty_driver(minor_num)), error);

        /* update init_termios and register as tty driver
         */
        tty_driver->owner = THIS_MODULE;
        tty_driver->driver_name = name;
        tty_driver->name = "acm";
        tty_driver->major = ACM_TTY_MAJOR;
        tty_driver->minor_start = 0;
        tty_driver->minor_num = minor_num;
        tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
        tty_driver->subtype = SERIAL_TYPE_NORMAL;

        tty_driver->flags = TTY_DRIVER_REAL_RAW;

        #if defined(TTY_DRIVER_NO_DEVFS)
        tty_driver->devfs_name = "tts/acm%d";
        tty_driver->flags |= TTY_DRIVER_NO_DEVFS;
//        #warning "TTY_DRIVER_NO_DEVFS------------"
        #endif /* defined(TTY_DRIVER_NO_DEVFS) */
        // XXX Probably not correct config name
        #if defined(TTY_DRIVER_DYNAMIC_DEV)
        tty_driver->flags |= TTY_DRIVER_DYNAMIC_DEV;
//        #warning "TTY_DRIVER_DYNAMIC_DEV------------"
        #endif /* defined(TTY_DRIVER_DYNAMIC_DEV) */
        tty_driver->init_termios = tty_std_termios;
        tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;

        tty_set_operations(tty_driver, &tty_ops);
        tty_driver->tiocmget= tty_l26_tiocmget;
        tty_driver->tiocmset= tty_l26_tiocmset;
        THROW_IF(tty_register_driver(tty_driver), error);

        for (i = 0; i < tty_minor_count; i++)
                tty_register_device(tty_driver, i, NULL);


       /* for (i = 0; i < minor_num; i++)
                devfs_mk_cdev(MKDEV(ACM_TTY_MAJOR, i), S_IFCHR|S_IRUGO, (i & 1) ? "atcom/%d" :  "acm/%d", i);*/

#else /* defined(LINUX26) */

        /* update init_termios and register as tty driver
         */
        tty_driver.magic = TTY_DRIVER_MAGIC;
        tty_driver.driver_name = name;
        tty_driver.name = "acm0";

        tty_driver.major = ACM_TTY_MAJOR;
        tty_driver.num =   ACM_TTY_MINORS;
        tty_driver.minor_start = 0;
        tty_driver.type = TTY_DRIVER_TYPE_SERIAL;
        tty_driver.subtype = SERIAL_TYPE_NORMAL;
        tty_driver.flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
        tty_driver.init_termios = tty_std_termios;
        tty_driver.init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;

        THROW_IF(tty_register_driver(&tty_driver), error);


        for (i = 0; i < minor_num; i++)
                tty_register_devfs(&tty_driver, 0, i);

        TRACE_MSG0(TTY,"Register ok");

#endif /* defined(LINUX26) */

        CATCH(error) {
                printk(KERN_ERR"%s: ERROR\n", __FUNCTION__);

#if defined(LINUX26)
                put_tty_driver(tty_driver);
#endif /* defined(LINUX26) */

                if (tty_table) LKFREE(tty_table);
                if (function_table) LKFREE(function_table);
                return -EINVAL;
        }
        return 0;
}


/*!
 *@brief tty_l26_exit - module cleanup
 *
 * This is called prior to the module being unloaded.
 */
void tty_l26_exit (void)
{
        unsigned long flags;
        int i;
        //struct usbd_urb *urb;

        /* Wake up any pending opens after setting the exiting flag. */
        //tty_l26_private.exiting = 1;
        //if (tty_l26_private.open_wait_count > 0)
        //wake_up_interruptible(&tty_l26_private.open_wait);

        /* verify no tasks are running */
        //acm_wait_task(acm->function_instance, &acm->recv_tqueue);
        //acm_wait_task(acm->function_instance, &tty_l26_private.wqueue);
        //acm_wait_task(acm->function_instance, &tty_l26_private.hqueue);

        /* de-register as tty  and usb drivers */
#if defined(LINUX26)

        printk(KERN_INFO "tty_l26_exit!\n");
        for (i = 0; i < tty_minor_count; i++)

                tty_unregister_device(tty_driver, i);

        // XXX blk_run_queues();
        tty_unregister_driver(tty_driver);

        put_tty_driver(tty_driver);

        printk(KERN_INFO "after tty_unregister_driver!\n");
#else
        run_task_queue(&tq_timer);
        tty_unregister_driver(&tty_driver);
#endif

        if (tty_table) LKFREE(tty_table);
        if (function_table) LKFREE(function_table);
        printk(KERN_INFO "tty_l26_exit!\n");


}
