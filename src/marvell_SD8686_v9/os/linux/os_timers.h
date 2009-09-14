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

#ifndef	_OS_TIMERS_H
#define _OS_TIMERS_H

typedef struct __WLAN_DRV_TIMER
{
    struct timer_list tl;
    void (*timer_function) (void *context);
    void *function_context;
    UINT time_period;
    BOOLEAN timer_is_periodic;
    BOOLEAN timer_is_canceled;
} __ATTRIB_PACK__ WLAN_DRV_TIMER, *PWLAN_DRV_TIMER;

static inline void
TimerHandler(unsigned long fcontext)
{
    PWLAN_DRV_TIMER timer = (PWLAN_DRV_TIMER) fcontext;

    timer->timer_function(timer->function_context);

    if (timer->timer_is_periodic == TRUE) {
        mod_timer(&timer->tl, jiffies + ((timer->time_period * HZ) / 1000));
    }
}

static inline void
InitializeTimer(PWLAN_DRV_TIMER timer,
                void (*TimerFunction) (void *context), void *FunctionContext)
{
    // first, setup the timer to trigger the WlanTimerHandler proxy
    init_timer(&timer->tl);
    timer->tl.function = TimerHandler;
    timer->tl.data = (u32) timer;

    // then tell the proxy which function to call and what to pass it       
    timer->timer_function = TimerFunction;
    timer->function_context = FunctionContext;
    timer->timer_is_canceled = FALSE;
}

static inline void
SetTimer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = FALSE;
    timer->tl.expires = jiffies + (MillisecondPeriod * HZ) / 1000;
    add_timer(&timer->tl);
    timer->timer_is_canceled = FALSE;
}

static inline void
ModTimer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = FALSE;
    mod_timer(&timer->tl, jiffies + (MillisecondPeriod * HZ) / 1000);
    timer->timer_is_canceled = FALSE;
}

static inline void
SetPeriodicTimer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = TRUE;
    timer->tl.expires = jiffies + (MillisecondPeriod * HZ) / 1000;
    add_timer(&timer->tl);
    timer->timer_is_canceled = FALSE;
}

#define	FreeTimer(x)	do {} while (0)

static inline void
CancelTimer(WLAN_DRV_TIMER * timer)
{
    del_timer(&timer->tl);
    timer->timer_is_canceled = TRUE;
}

#endif /* _OS_TIMERS_H */
