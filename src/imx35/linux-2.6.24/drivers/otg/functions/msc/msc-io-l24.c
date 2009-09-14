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
 * otg/function/msc/msc-io-l24.c - MSC IO
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/msc-io-l24.c|20070425221028|61154
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Tony Tang <tt@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/msc/msc-io-l24.c
 * @brief
 *
 * NOTES
 *
 * TODO
 *
 * 1. implement prevent removal command.
 *
 * @ingroup MSCFunction
 * @ingroup LINUXOS
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include <linux/poll.h>
#include <linux/sched.h>
//#include <linux/devfs_fs_kernel.h>

#include <linux/kdev_t.h>
#include <linux/blkdev.h>

#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#include "crc.h"
#include "msc-io.h"

extern void msc_open_blockdev (struct usbd_function_instance *function_instance);
extern void msc_close_blockdev (struct usbd_function_instance *function_instance);
// XXX no use extern int msc_connection_blockdev (struct usbd_function_instance *function_instance);
//XXX No use extern struct msc_private msc_private;

#define DEVICE_EJECTED  0x0001  //MEDIA_EJECTED
#define DEVICE_INSERTED 0x0002  //MEDIA_INSERT
#define DEVICE_MOUNTED  0x0001  //DEVICE_MOUNTED
#define DEVICE_UNMOUNTED 0x0002 //DEVICE_UNMOUNTED

#if 0
void msc_io_wait(struct usbd_function_instance *function_instance, u32 flag)
{
        unsigned long flags;
        msc->io_state |= MSC_IOCTL_WAITING | flag;
        TRACE_MSG1(MSC, "SLEEPING io_state: %x", msc->io_state);
        interruptible_sleep_on(&msc->ioctl_wq);
}

void msc_io_wakup(struct usbd_function_instance *function_instance)
{
        unsigned long flags;
        local_irq_save(flags);
        if (msc->io_state & MSC_IOCTL_WAITING) {
                msc->io_state &= ~MSC_IOCTL_WAITING;
                TRACE_MSG0(MSC, "WAKEUP");
                wake_up_interruptible(&msc->ioctl_wq);
        }
        local_irq_restore(flags);
}
#endif

extern struct usbd_function_instance *proc_function_instance;

/*! int  msc_io_ioctl_internal (struct inode* , unsigned int, unsigned long)
 * @brief process io control command
 *
 * @param inode
 * @param cmd - ip command
 * @param arg - command argument
 * @return  0 for success
 *
 *
 * XXX Can we get function_instance passed in as arg??? XXX
 */
int msc_io_ioctl_internal(struct inode *inode, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        int i;
        int len;
        int flag;

        static char func_buf[32];
        struct otgmsc_mount mount;
        unsigned long flags;

        TRACE_MSG2(MSC, "cmd: %d connect: %d", cmd, msc->connected);
        memset(&mount, 0, sizeof(mount));
        switch (cmd) {

                /* Mount - make the specified major/minof device available for use
                 * by the USB Host, this does not require an active connection.
                 */
        case OTGMSC_START:
                TRACE_MSG0(MSC, "Mounting the device");
                RETURN_EINVAL_IF(copy_from_user(&mount, (void *)arg, _IOC_SIZE(cmd)));

                TRACE_MSG3(MSC, "major=%d minor=%d state=%d", mount.major, mount.minor, msc->block_dev_state);
                msc->major = mount.major;
                msc->minor = mount.minor;

                //msc->io_state = MSC_INACTIVE;
                inode->i_rdev=MKDEV(msc->major, msc->minor);
                msc_open_blockdev (function_instance);
                RETURN_EAGAIN_UNLESS(msc->command_state == MSC_READY);

                mount.status = (msc->block_dev_state == DEVICE_INSERTED) ? DEVICE_MOUNTED : DEVICE_UNMOUNTED;

                TRACE_MSG1(MSC, "Device is mounted status: %d", mount.status);

                // XXX Need to copy result back to user space
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
                TRACE_MSG0(MSC,  "Device mounted");
                return 0;

                /* Umount - make the currently mounted device unavailable to the USB Host,
                 * if there is pending block i/o block until it has finished.
                 * Note that if the driver is unloaded the waiting ioctl process
                 * must be woken up and informed with error code.
                 */
        case OTGMSC_STOP:

                TRACE_MSG0(MSC,  "Unmounting the device");

                RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));

                if (msc->command_state != MSC_READY) {
                        TRACE_MSG0(MSC, "SLEEPING");
                        interruptible_sleep_on(&msc->ioctl_wq);
                        TRACE_MSG0(MSC, "AWAKE");
                }

                RETURN_EAGAIN_UNLESS(msc->command_state == MSC_INACTIVE);

                // XXX Need to copy result back to user space
                msc->major = mount.major;
                msc->minor = mount.minor;
                msc_close_blockdev(function_instance);
                mount.status = DEVICE_UNMOUNTED;
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
                TRACE_MSG0(MSC,  "Device unmounted");
                return 0;


                /* Status - return the current mount status.
                 */
        case OTGMSC_STATUS:
                TRACE_MSG0(MSC, "Mount status");
                RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
                if (msc->block_dev_state == DEVICE_EJECTED)
                        mount.status = DEVICE_UNMOUNTED;
                else
                        mount.status = DEVICE_MOUNTED;

                // XXX Need to copy result back to user space
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
                return 0;

                /* Wait_Connect - if not already connected wait until connected,
                 * Note that if the driver is unloaded the waiting ioctl process
                 * must be woken up and informed with error code.
                 */
        case OTGMSC_WAIT_CONNECT:
                TRACE_MSG1(MSC, "Wait for connect: connected: %d", msc->connected);
                // XXX local_irq_save(flags);

                if (msc->connected == 0) {
                        TRACE_MSG0(MSC, "SLEEPING");
                        interruptible_sleep_on (&msc->ioctl_wq);
                        TRACE_MSG0(MSC, "AWAKE");
                }
                RETURN_EAGAIN_UNLESS(msc->connected);

                RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
                TRACE_MSG0(MSC,  "Device connected");
                return 0;

                /* Wait_DisConnect - if not already disconnected wait until disconnected,
                 * Note that if the driver is unloaded the waiting ioctl process
                 * must be woken up and informed with error code.
                 */
        case OTGMSC_WAIT_DISCONNECT:
                TRACE_MSG1(MSC, "Wait for disconnect: connected: %d", msc->connected);

                if (msc->connected == 1) {
                        TRACE_MSG0(MSC, "SLEEPING");
                        interruptible_sleep_on (&msc->ioctl_wq);
                        TRACE_MSG0(MSC, "AWAKE");
                }
                RETURN_EAGAIN_IF(msc->connected);
                RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
                TRACE_MSG0(MSC,  "Device disconnected");
                return 0;

        default:
                TRACE_MSG1(MSC,  "Unknown command: %x", cmd);
                TRACE_MSG1(MSC,  "OTGMSC_START: %x", OTGMSC_START);
                TRACE_MSG1(MSC,  "OTGMSC_WRITEPROTECT: %x", OTGMSC_WRITEPROTECT);
                TRACE_MSG1(MSC,  "OTGMSC_STOP: %x", OTGMSC_STOP);
                TRACE_MSG1(MSC,  "OTGMSC_STATUS: %x", OTGMSC_STATUS);
                TRACE_MSG1(MSC,  "OTGMSC_WAIT_CONNECT: %x", OTGMSC_WAIT_CONNECT);
                TRACE_MSG1(MSC,  "OTGMSC_WAIT_DISCONNECT: %x", OTGMSC_WAIT_DISCONNECT);
                return -EINVAL;
        }
        return 0;
}



/*! int msc_io_ioctl(struct inode* inode, struct file*, unsigned int, unsigned long)
 * @brief verify ioctl command and call msc_io_ioctl_internal to process ioctl command
 *
 * @param inode pointer to device node
 * @param filp  file pointer
 * @param cmd  command to process
 * @param arg  command argument
 * @return non-zero for error
 */
int msc_io_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        int i;
        int len;
        int flag;

        TRACE_MSG6(MSC, "cmd: %08x arg: %08x type: %02d nr: %02d dir: %02d size: %02d",
                        cmd, arg, _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_DIR(cmd), _IOC_SIZE(cmd));

        RETURN_EINVAL_UNLESS (_IOC_TYPE(cmd) == OTGMSC_MAGIC);
        RETURN_EINVAL_UNLESS (_IOC_NR(cmd) <= OTGMSC_MAXNR);

        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_READ) && !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)));
        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_WRITE) && !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd)));

        return msc_io_ioctl_internal(inode, cmd, arg);
}

/*! msc_io_proc_ioctl -process ioctl command
 *
 * @param inode - device node pointer
 * @param filp - file pointer
 * @param cmd - command to process
 * @param arg - command argument
 * @return non-zero for error
 */

int msc_io_proc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        return msc_io_ioctl(inode, filp, cmd, arg);
}


static struct file_operations msc_io_proc_switch_functions = {
ioctl:msc_io_proc_ioctl,
};


/*! int msc_io_init_l24(struct usbd_function_instance* )
 * @brief - create proc dircetory entry: /proc/msc_io
 *
 * @param function_instance - pointer to this function driver instance
 * @return 0 for success
 */
int msc_io_init_l24(struct usbd_function_instance *function_instance)
{
        struct proc_dir_entry *message = NULL;

        proc_function_instance = function_instance;

        THROW_IF (!(message = create_proc_entry ("msc_io", 0666, 0)), error);
        message->proc_fops = &msc_io_proc_switch_functions;
        CATCH(error) {
                printk(KERN_ERR"%s: creating /proc/msc_io failed\n", __FUNCTION__);
                if (message)
                        remove_proc_entry("msc_io", NULL);
                return -EINVAL;
        }
        return 0;
}

/*! void msc_io_exit_l24()
 * @brief -remove and exit proc operation
 * @return none
 */
void msc_io_exit_l24(void)
{
        remove_proc_entry("msc_io", NULL);
        proc_function_instance = NULL;
}
