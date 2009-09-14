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
 * otg/msc_fd/trace.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/trace.c|20070425221028|00673
 *
 *      Copyright (c) 2003-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *
 */

#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

#include <asm/atomic.h>
#include <asm/io.h>

#include <linux/proc_fs.h>

#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/cache.h>



#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>

#include <usbp-chap9.h>
#include <usbp-mem.h>
#include <usbp-func.h>
#include "msc-scsi.h"
#include "msc.h"
#include "trace.h"


static struct msc_private *msc_private;
int msc_trace_first;
int msc_trace_next;
msc_trace_t *msc_traces;

extern int msc_interrupts;


#if defined(CONFIG_OTG_MSC_REGISTER_TRACE) && defined(CONFIG_PROC_FS) || defined(_OTG_DOXYGEN)

msc_trace_t *MSC_TRACE_NEXT(msc_trace_types_t msc_trace_type)
{
        msc_trace_t *p;

        p = msc_traces + msc_trace_next;

        if (msc_private) {
                p->ticks = usbd_ticks(msc_private->function);
                p->sofs = usbd_framenum(msc_private->function);
        }
        p->interrupts = msc_interrupts;
        p->msc_trace_type = msc_trace_type;

        msc_trace_next++;
        msc_trace_next = (msc_trace_next == TRACE_MAX) ? 0 : msc_trace_next;

        if (msc_trace_next == msc_trace_first) {
                msc_trace_first++;
                msc_trace_first = (msc_trace_first == TRACE_MAX) ? 0 : msc_trace_first;
        }

        return p;
}

/* Proc Filesystem *************************************************************************** */

/* *
 * msc_trace_proc_read - implement proc file system read.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Standard proc file system read function.
 */
static ssize_t msc_trace_proc_read (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;
        int oindex;
        int previous;

        MOD_INC_USE_COUNT;
        // get a page, max 4095 bytes of data...
        if (!(page = get_free_page (GFP_KERNEL))) {
                MOD_DEC_USE_COUNT;
                return -ENOMEM;
        }

        len = 0;
        oindex = index = (*pos)++;

        if (index == 0)
                len += sprintf ((char *) page + len, " Index     Ints     Ticks\n");


        index += msc_trace_first;
        if (index >= TRACE_MAX)
                index -= TRACE_MAX;

        previous = (index) ? (index - 1) : (TRACE_MAX - 1);


        if (
                        ((msc_trace_first < msc_trace_next) && (index >= msc_trace_first) && (index < msc_trace_next)) ||
                        ((msc_trace_first > msc_trace_next) && ((index < msc_trace_next) || (index >= msc_trace_first)))
           )
        {

                u64 ticks = 0;

                msc_trace_t *p = msc_traces + index;
                unsigned char *cp;
                unsigned int *ip;
                int skip = 0;

                if (oindex > 0) {
                        msc_trace_t *o = msc_traces + previous;

                        if (o->ticks)
                                ticks = (p->ticks > o->ticks) ? (p->ticks - o->ticks) : (o->ticks - p->ticks) ;

                        if (o->interrupts != p->interrupts)
                                skip++;

                }

                //printk(KERN_INFO"index: %d interrupts: %d\n", index, p->interrupts);
                len += sprintf ((char *) page + len, "%s%6d %8d ", skip?"\n":"", index, p->interrupts);

                if (ticks > 1024*1024)
                        len += sprintf ((char *) page + len, "%8dM ", ticks>>20);
                else
                        len += sprintf ((char *) page + len, "%8d  ", ticks);

                len += sprintf ((char *) page + len, "%6d  ", (int)p->sofs);

                switch (p->msc_trace_type) {
                case msc_trace_msg:
                        len += sprintf ((char *) page + len, " --                   ");
                        len += sprintf ((char *) page + len, p->trace.msg.msg);
                        break;

                case msc_trace_w:
                        len += sprintf ((char *) page + len, " -->                  ");
                        len += sprintf ((char *) page + len, "[%8x] W %s", p->trace.msg32.val, p->trace.msg32.msg);
                        break;

                case msc_trace_r:
                        len += sprintf ((char *) page + len, "<--                   ");
                        len += sprintf ((char *) page + len, "[%8x] R %s", p->trace.msg32.val, p->trace.msg32.msg);
                        break;

                case msc_trace_msg32:
                        len += sprintf ((char *) page + len, " --                   ");
                        len += sprintf ((char *) page + len, p->trace.msg32.msg, p->trace.msg32.val);
                        break;

                case msc_trace_msg16:
                        len += sprintf ((char *) page + len, " --                   ");
                        len += sprintf ((char *) page + len, p->trace.msg16.msg, p->trace.msg16.val0, p->trace.msg16.val1);
                        break;

                case msc_trace_msg8:
                        len += sprintf ((char *) page + len, " --                   ");
                        len += sprintf ((char *) page + len, p->trace.msg8.msg,
                                        p->trace.msg8.val0, p->trace.msg8.val1, p->trace.msg8.val2, p->trace.msg8.val3);
                        break;

                case msc_trace_setup:
                        cp = (unsigned char *)&p->trace.setup;
                        len += sprintf ((char *) page + len,
                                        " --           request [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                        break;

                case msc_trace_recv:
                case msc_trace_sent:
                        cp = (unsigned char *)&p->trace.sent;
                        len += sprintf ((char *) page + len,
                                        "%s             %s [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                        ( p->msc_trace_type == msc_trace_recv)?"<-- ":" -->",
                                        ( p->msc_trace_type == msc_trace_recv)?"recv":"sent",
                                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                        break;
                case msc_trace_rlba:
                        ip = (unsigned int *)&p->trace.ints;
                        len += sprintf ((char *) page + len,
                                        "%s             %s [%8x %08x]",
                                        "<-- ", "rlba", ip[0], ip[1]);
                        break;
                case msc_trace_slba:
                case msc_trace_tlba:
                        ip = (unsigned int *)&p->trace.ints;
                        len += sprintf ((char *) page + len,
                                        "%s             %s [%8x %08x]", " -->",
                                        ( p->msc_trace_type == msc_trace_tlba)?"tlba":"slba",
                                        ip[0], ip[1]);
                        break;

                case msc_trace_tag:
                        ip = (unsigned int *)&p->trace.ints;
                        len += sprintf ((char *) page + len,
                                        "%s             TAG: %8x FRAME: %03x", " -->",
                                        ip[0], ip[1]);
                        break;

                case msc_trace_sense:
                        ip = (unsigned int *)&p->trace.ints;
                        len += sprintf ((char *) page + len,
                                        "%s             SENSE: %06x INFO: %08x", " -->",
                                        ip[0], ip[1]);
                        break;

                case msc_trace_cbw:
                        len += sprintf ((char *) page + len, " -->                  ");
                        len += sprintf ((char *) page + len, "%s %02x", p->trace.msg32.msg, p->trace.msg32.val);
                        break;
                }
                len += sprintf ((char *) page + len, "\n");
        }

        if ((len > count) || (len == 0))
                len = -EINVAL;
        else if (len > 0 && copy_to_user (buf, (char *) page, len))
                len = -EFAULT;

        free_page (page);
        MOD_DEC_USE_COUNT;
        return len;
}

/* *
 * msc_trace_proc_write - implement proc file system write.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Proc file system write function, used to signal monitor actions complete.
 * (Hotplug script (or whatever) writes to the file to signal the completion
 * of the script.)  An ugly hack.
 */
static ssize_t msc_trace_proc_write (struct file *file, const char *buf, size_t count, loff_t * pos)
{
        return count;
}

static struct file_operations msc_trace_proc_operations_functions = {
        read:msc_trace_proc_read,
        write:msc_trace_proc_write,
};


/**
 * msc_trace_init
 *
 * Return non-zero if not successful.
 */
int msc_trace_init (char *name, struct msc_private *msc)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        if (!(msc_traces = vmalloc(sizeof(msc_trace_t) * TRACE_MAX))) {
                printk(KERN_ERR"%s: malloc failed %p %d\n", __FUNCTION__, msc_traces, sizeof(msc_trace_t) * TRACE_MAX);
                return -EINVAL;
        }
        memset(msc_traces, 0, sizeof(msc_trace_t) * TRACE_MAX);

        {
                struct proc_dir_entry *p;

                // create proc filesystem entries
                if ((p = create_proc_entry (name, 0, 0)) == NULL) {
                        printk(KERN_INFO"BITRACE PROC FS failed\n");
                }
                else {
                        p->proc_fops = &msc_trace_proc_operations_functions;
                }
        }
        printk(KERN_INFO"%s: OK\n", __FUNCTION__);
        msc_private = msc;
        return 0;
}

/**
 * udc_release_io - release UDC io region
 */
void msc_trace_exit (char *name)
{
        msc_private = NULL;
        {
                unsigned long flags;
                local_irq_save (flags);
                remove_proc_entry (name, NULL);
                if (msc_traces) {
                        msc_trace_t *p = msc_traces;
                        msc_traces = NULL;
                        vfree(p);
                }
                local_irq_restore (flags);
        }
}


#else
int msc_trace_init (void)
{
        return 0;
}

void msc_trace_exit (char *)
{
        return;
}
#endif
