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
 * otg/hardware/l26-ocd.c -- Generic Linux 2.6 Timer
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/otglib/l26-ocd.c|20070612232808|58041
 *
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/l26-ocd.c
 * @brief Generic L26 OCD Driver.
 *
 * Provides ticks via gettimeofday() and optional a OTG timer
 * using high resolution timer.
 *
 * CONFIG_HIGH_RES_TIMERS=y
 *
 *     Optionally provides accurate OTG timer using Linux High
 *     Resolution Timer.
 *
 * CONFIG_HIGH_RES_TIMERS=n
 *
 *     Start timer is a dummy if High Resolution Timers not
 *     enabled.
 *
 * @ingroup LINUXOS
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>


#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>

/* Other includes*/
#include <linux/pci.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/dma.h>
//#include <asm/hrtime.h>

#include <otg/otg-trace.h>
#include <otg/otg-dev.h>
#include <otg/otg-api.h>
#include <otg/otg-ocd.h>

/* ********************************************************************************************* */

#ifdef CONFIG_HIGH_RES_TIMERS

struct l26_hr_timer {
        struct timer_list       timer_list;
        BOOL                    active;
        u32                     usec_set;
        u32                     jiffy_per_sec;
};

/*!
 * l26_hrt_callback() - called to queue an a timer event to otg queue
 * @param arg - pointer of otg_instance type
 */

void l26_hrt_callback (unsigned long arg)
{
        struct ocd_instance     *ocd = (struct ocd_instance *) arg;
        struct otg_instance     *otg = ocd->otg;
        struct l26_hr_timer     *hr_timer = ocd->privdata;

        RETURN_UNLESS(hr_timer->active);
        hr_timer->active = FALSE;
        //TRACE_MSG3 (ocd->TAG, "usec: %d expires: %8u arch_cycle_expires: %8u",
        //                hr_timer->usec_set, hr_timer.expires, hr_timer.arch_cycle_expires);

        otg_queue_event(otg, TMOUT, ocd->TAG, "HRT TMOUT");
}


#if 0
/*!
 * l26_hrt_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds).
 *
 * XXX There may be a floor or minimum that can be effectively set.
 * XXX We have seen an occasional problem with US(25) for discharge for example.
 *
 * @param otg
 * @param usec
 * @return 0 on success
 */
int l26_hrt_start_timer(struct otg_instance *otg, int usec)
{
        struct ocd_instance     *ocd = otg->ocd;
        struct l26_hr_timer     *hr_timer = ocd->privdata;

        TRACE_MSG1(ocd->TAG, "usec: %d", usec);

        hr_timer->usec_set = usec;
        //TRACE_MSG1 (otg->ocd->TAG, "usec: %d", usec);

        hr_timer->active = FALSE;
        TRACE_MSG1(ocd->TAG, "resetting active: %d", hr_timer->active);

        del_timer(&hr_timer->timer_list);
        RETURN_ZERO_UNLESS(usec);

        hr_timer->active = TRUE;
        TRACE_MSG1(ocd->TAG, "setting active: %d", hr_timer->active);

        if (hr_timer->usec_set >= 1000000) {
                hr_timer->timer_list.expires = jiffies + ((hr_timer->usec_set/1000000)*hr_timer->jiffy_per_sec);
                hr_timer->timer_list.arch_cycle_expires = get_arch_cycles(jiffies);
                TRACE_MSG4 (otg->ocd->TAG, "usec: %u jiffies: %8u expires: %8u arch_cycle_expires: %8u LONG",
                                usec, jiffies, hr_timer->timer_list.expires, hr_timer->timer_list.arch_cycle_expires);
        }
        else {
                hr_timer->timer_list.expires = jiffies;
                hr_timer->timer_list.arch_cycle_expires = get_arch_cycles(jiffies);

                if (hr_timer->usec_set < 100) {
                        TRACE_MSG1(otg->ocd->TAG, "usec: %d set to minimum 100", hr_timer->usec_set);
                        hr_timer->usec_set = 100;
                }
                hr_timer->timer_list.arch_cycle_expires += nsec_to_arch_cycle(hr_timer->usec_set * 1000);

                //TRACE_MSG2(ocd->TAG, "arch_cycle_expires: %d arch_cycles_per_jiffy: %d",
                //                hr_timer->timer_list.arch_cycle_expires, hr_timer->timer_list.arch_cycles_per_jiffy);

                while (hr_timer->timer_list.arch_cycle_expires >= arch_cycles_per_jiffy) {
                        hr_timer->timer_list.expires++;
                        hr_timer->timer_list.arch_cycle_expires -= arch_cycles_per_jiffy;
                }
                TRACE_MSG4 (ocd->TAG, "usec: %u jiffies: %8u expires: %8u arch_cycle_expires: %8u SHORT",
                                usec, jiffies, hr_timer->timer_list.expires, hr_timer->timer_list.arch_cycle_expires);
        }

        add_timer(&hr_timer->timer_list);
        return 0;
}
#endif
#endif /* CONFIG_HIGH_RES_TIMERS */
/* ********************************************************************************************* */
/*!
 * l26_ocd_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds).
 *
 * @param otg
 * @param usec
 */
int l26_ocd_start_timer(struct otg_instance *otg, int usec)
{
        otg_event(otg, TMOUT, otg->ocd->TAG, "l26 Timer - FAKE TMOUT");
        return 0;
}

/* l26_ocd_ticks - get current ticks
 */
otg_tick_t l26_ocd_ticks (void)
{
        struct timeval tv;
        do_gettimeofday(&tv);
        #if 0
        return tv.tv_sec * 1000000 + tv.tv_usec;
        #else
        return tv.tv_sec << 20 | (tv.tv_usec & 0xfffff);
        #endif
}

/* l26_ocd_elapsed - return micro-seconds between two tick values
 */
otg_tick_t l26_ocd_elapsed(otg_tick_t *t1, otg_tick_t *t2)
{
        #if 0
        return (((t1 > t2) ? (t1 - t2) : (t2 - *t1)));
        #else
        otg_tick_t n1 = (*t1 >> 20) * 1000000 + (*t1 & 0xfffff);
        otg_tick_t n2 = (*t1 >> 20) * 1000000 + (*t2 & 0xfffff);
        return (((n1 > n2) ? (n1 - n2) : (n2 - n1)));
        #endif
}


struct ocd_ops l26_ocd_ops = {
        #ifdef xxxx_CONFIG_HIGH_RES_TIMERS
        .start_timer = l26_hrt_start_timer,
        #else /* CONFIG_HIGH_RES_TIMERS */
        .start_timer = l26_ocd_start_timer,
        #endif /* CONFIG_HIGH_RES_TIMERS */
        .ticks = l26_ocd_ticks,
        .elapsed = l26_ocd_elapsed,
};

/* ********************************************************************************************* */

#ifdef CONFIG_HIGH_RES_TIMERS
void
l26_ocd_remove(struct otg_dev *otg_dev)
{
        struct ocd_instance *ocd = otg_dev->ocd_instance;
        RETURN_UNLESS (ocd->privdata);
        LKFREE(ocd->privdata);
        ocd->privdata = NULL;
}

int
l26_ocd_probe(struct otg_dev *otg_dev)
{
        struct ocd_instance *ocd = otg_dev->ocd_instance;
        struct l26_timer *hr_timer = NULL;
        RETURN_EINVAL_UNLESS((ocd->privdata = CKMALLOC(sizeof(struct l26_hr_timer))));
        #if 0
        init_timer (hr_timer);
        hr_timer->timer_list.expires = jiffies + 10;
        hr_timer->timer_list.function = l26_hrt_callback;
        hr_timer->timer_list.data = (unsigned long) ocd_instance;
        hr_timer->timer_list.arch_cycle_expires = get_arch_cycles(jiffies);
        hr_timer->timer_list.arch_cycle_expires += nsec_to_arch_cycle(100 * 1000 * 1000);
        while (hr_timer->timer_list.arch_cycle_expires >= arch_cycles_per_jiffy) {
                hr_timer->timer_list.expires++;
                hr_timer->timer_list.arch_cycle_expires -= arch_cycles_per_jiffy;
        }
        #endif
        return 0;
}
#endif /* CONFIG_HIGH_RES_TIMERS */
