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
 * otg/otgcore/otg-trace-l24.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/otg-trace-lnx.c|20061218212925|12732
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *
 */
/*!
 * @file otg/otgcore/otg-trace-lnx.c
 * @brief OTG Core Linux Debug Trace Facility
 *
 * This allows access to the OTG Core Trace via /proc/trace_otg. Each time
 * this file is opened and read it will contain a snapshot of the most
 * recent trace messages.
 *
 * Reading the trace resets it, the next read will open a new snapshot.
 *
 * N.B. There is no protection from multiple reads.
 *
 * N.B. This is for debugging and is not normally enabled for production software.
 *
 * @ingroup OTGTRACE
 * @ingroup LINUXOS
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/otg-module.h>
#include <linux/vmalloc.h>
#include <stdarg.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) || defined(_OTG_DOXYGEN)


extern otg_trace_snapshot otg_snapshot1;
extern otg_trace_snapshot otg_snapshot2;

extern int otg_trace_exiting;
extern otg_trace_snapshot *otg_trace_current_traces;
extern otg_trace_snapshot *otg_trace_other_traces;
extern otg_pthread_mutex_t otg_trace_mutex;




/* *
 * otg_trace_proc_read_l24 - implement proc file system read.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Standard proc file system read function.
 */
static ssize_t otg_trace_proc_read_l24 (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;
        int oindex;
        int rc;

        otg_tick_t ticks = 0;

        unsigned char *cp;
        int skip = 0;

        otg_trace_t px;
        otg_trace_t ox;
        otg_trace_t vx;
        otg_trace_t *o, *p;

        RETURN_ENOMEM_IF (!(page = GET_KERNEL_PAGE()));

        //_MOD_INC_USE_COUNT;

        // Lock out tag changes while reading
        //DOWN(&otg_trace_sem_l24);
        //while(otg_sem_wait(&otg_trace_sem_l24));
        len = otg_trace_proc_read((char *)page, count, (int *)pos);

        if (len > count) {
                //printk(KERN_INFO"%s: EINVAL\n", __FUNCTION__);
                len = -EINVAL;
        }

        else if (len > 0 && copy_to_user (buf, (char *) page, len)) {

                printk(KERN_INFO"%s: EFAULT\n", __FUNCTION__);
                len = -EFAULT;
        }

        //free_page (page);
        //UP(&otg_trace_sem_l24);
        //otg_sem_post(&otg_trace_sem_l24);
        free_page (page);
        //_MOD_DEC_USE_COUNT;
        return len;
}

#define MAX_TRACE_CMD_LEN  64
#if 1
/*! otg_trace_proc_write - implement proc file system write.
 * @param buf - command message buffer
 * @param count- message length
 * @param pos - trace slot to write to
 * @return count
 *
 */
int otg_trace_proc_write (const char *buf, int count, int * pos)
{
#define MAX_TRACE_CMD_LEN  64
        char command[MAX_TRACE_CMD_LEN+1];
        size_t n = count;
        size_t l;
        char c;

        if (n > 0) {
                l = MIN(n,MAX_TRACE_CMD_LEN);
                if (copy_from_user (command, buf, l))
                        count = -EFAULT;
                else {
                        n -= l;
                        while (n > 0) {
                                if (copy_from_user (&c, buf + (count - n), 1)) {
                                        count = -EFAULT;
                                        break;
                                }
                                n -= 1;
                        }
                        // Terminate command[]
                        if (l > 0 && command[l-1] == '\n') {
                                l -= 1;
                        }
                        command[l] = 0;
                }
        }

        if (0 >= count) {
                return count;
        }


        return count;
}
#endif
/* *
 * otg_trace_proc_write_l24 - implement proc file system write.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Proc file system write function.
 */
static ssize_t otg_trace_proc_write_l24 (struct file *file, const char *buf, size_t count, loff_t * pos)
{
        char command[MAX_TRACE_CMD_LEN+1];
        size_t l = MIN(count, MAX_TRACE_CMD_LEN);

        RETURN_ZERO_UNLESS(count);
        RETURN_EINVAL_IF (copy_from_user (command, buf, l));


        RETURN_EINVAL_IF(copy_from_user (command, buf, l));

        return otg_trace_proc_write(command, l, (int *)pos);
#if 0
        else {
                n -= l;
                while (n > 0) {
                        if (copy_from_user (&c, buf + (count - n), 1)) {
                                count = -EFAULT;
                                break;
                        }
                        n -= 1;
                }
                // Terminate command[]
                if (l > 0 && command[l-1] == '\n') {
                        l -= 1;
                }
                command[l] = 0;
        }
#endif
}

/* Module init ************************************************************** */



static struct file_operations otg_trace_proc_operations_functions = {
        read: otg_trace_proc_read_l24,
        write: otg_trace_proc_write_l24,
};


int otg_trace_modinit(void);
void otg_trace_modexit(void);
void otg_trace_init_snapshot(otg_trace_snapshot *tss);

otg_trace_snapshot *test;

/*! rel_snapshot - release trace snapshot
 *
 * @param tss - pointer to otg_trace_snapshot
 * @return NULL
 */

otg_trace_snapshot *rel_snapshot_lnx(otg_trace_snapshot *tss)
{
        RETURN_NULL_UNLESS(tss);
        if (tss->traces) {
                vfree(tss->traces);
                tss->traces = NULL;
        }
        return NULL;
}

/*! otg_trace_snapshot - allocate otg trace snapshot memory resource
 *
 * @param tss - pointer to otg_trace_snapshot
 * @return otg_trace_snapshot point on success
 */

otg_trace_snapshot *get_snapshot_lnx(otg_trace_snapshot *tss)
{
        UNLESS ((tss->traces = vmalloc(sizeof(otg_trace_t) * TRACE_MAX))) {
//                printk(KERN_ERR "Cannot allocate memory for OTG trace log.\n");
                return rel_snapshot_lnx(tss);
        }
        otg_trace_init_snapshot(tss);
        return tss;
}


/*! otg_trace_modinit_lnx
 *
 * Return non-zero if not successful.
 */
int otg_trace_modinit_lnx (void)
{
        struct proc_dir_entry *p;

        otg_trace_current_traces = otg_trace_other_traces = NULL;

        UNLESS ((otg_trace_current_traces = get_snapshot_lnx(&otg_snapshot1)) &&
                        (otg_trace_other_traces = get_snapshot_lnx(&otg_snapshot2)))
        {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                otg_trace_current_traces = rel_snapshot_lnx(otg_trace_current_traces);
                otg_trace_other_traces = rel_snapshot_lnx(otg_trace_other_traces);
                return -ENOMEM;
        }

        RETURN_EINVAL_IF(otg_trace_modinit());

        //RETURN_EINVAL_IF(otg_sem_init_unlocked((&otg_trace_sem_l24)));

        // create proc filesystem entries
        if ((p = create_proc_entry ("trace_otg", 0, 0)) == NULL)
                printk(KERN_INFO"%s PROC FS failed\n", "trace_otg");
        else
                p->proc_fops = &otg_trace_proc_operations_functions;

        return 0;
}

/*! otg_trace_modexit_lnx - remove procfs entry, free trace data space.
 */
void otg_trace_modexit_lnx (void)
{
        otg_trace_t *p;
        unsigned long flags;
        otg_trace_snapshot *c,*o;

        remove_proc_entry ("trace_otg", NULL);

        c = otg_trace_current_traces;
        o = otg_trace_other_traces;

        otg_trace_modexit();

        rel_snapshot_lnx(c);
        rel_snapshot_lnx(o);
}

#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */
//int otg_trace_init_l24 (void) {return 0;}
//void otg_trace_exit_l24 (void) {}
void otg_trace_modexit_lnx (void) {}
int otg_trace_modinit_lnx (void) {return 0;}
#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

#endif /* defined(CONFIG_OTG_LNX) */
