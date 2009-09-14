/*
 *  
 * (c) Copyright © 2003-2006, Marvell International Ltd. 
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */

#ifndef	__WLAN_THREAD_H_
#define	__WLAN_THREAD_H_

#include	<linux/kthread.h>

typedef struct
{
    struct task_struct *task;
    wait_queue_head_t waitQ;
    pid_t pid;
    void *priv;
} wlan_thread;

static inline void
wlan_activate_thread(wlan_thread * thr)
{
        /** Record the thread pid */
    thr->pid = current->pid;

        /** Initialize the wait queue */
    init_waitqueue_head(&thr->waitQ);
}

static inline void
wlan_deactivate_thread(wlan_thread * thr)
{
    ENTER();

    LEAVE();
}

static inline void
wlan_create_thread(int (*wlanfunc) (void *), wlan_thread * thr, char *name)
{
    thr->task = kthread_run(wlanfunc, thr, "%s", name);
}

static inline int
wlan_terminate_thread(wlan_thread * thr)
{
    ENTER();

    /* Check if the thread is active or not */
    if (!thr->pid) {
        PRINTM(INFO, "Thread does not exist\n");
        return -1;
    }
    kthread_stop(thr->task);

    LEAVE();
    return 0;
}

#endif
