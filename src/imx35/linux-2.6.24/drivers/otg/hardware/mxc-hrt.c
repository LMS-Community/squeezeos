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
 * otg/hardware/mxc-hrt.c -- Freescale High Resolution timer
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/mxc/mxc-hrt.c|20070614183950|63070
 *
 *      Copyright (c) 2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *      Shahrad Payandeh <sp@belcarra.com>
 *
 */

/*!
 * @file otg/hardware/mxc-hrt.c
 * @brief Freecale GPT Timer implementation.
 *
 * Use the Linux High Resolution Timers to implement OTG Timer.
 *
 * @ingroup FSOTG
 * @ingroup LINUXOS
 * @ingroup OCD
 *
 */

#include <otg/pcd-include.h>

#if defined (CONFIG_OTG_HRT) || defined(_OTG_DOXYGEN)

#include <linux/pci.h>
#include <linux/hrtimer.h>
#include <asm/arch/gpio.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include "mxc-lnx.h"
#include "mxc-hardware.h"

/* ********************************************************************************************* */
static struct hrtimer g_mxc_hrt_timer;	/*hrt timer */
static struct otg_instance *g_otg;	/* save the OTG instance */
static otg_tick_t g_mxc_hrt_divisor = 0;
static otg_tick_t g_mxc_hrt_multiplier = 0;
static BOOL g_mxc_hr_active = FALSE;

/*!
 * mxc_hrt_callback() - called to queue an a timer event to otg queue
 * @param arg - pointer of hrtimer type
 */
static int mxc_hrt_callback(struct hrtimer *arg)
{
	TRACE_MSG1(g_otg->ocd->TAG, "checking active: %d", g_mxc_hr_active);
	if (!g_mxc_hr_active) {
		return HRTIMER_NORESTART;
                }

	g_mxc_hr_active = FALSE;
	TRACE_MSG1(g_otg->ocd->TAG, "resetting active: %d", g_mxc_hr_active);

	otg_queue_event(g_otg, TMOUT, g_otg->ocd->TAG, "TMOUT");
	return HRTIMER_NORESTART;
}

/*!
 * mxc_hrt_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds).
 *
 * XXX There may be a floor or minimum that can be effectively set.
 * XXX We have seen an occasional problem with US(25) for discharge for example.
 *
 * @param otg
 * @param usec
 * @return 0 on success
 */
int mxc_hrt_start_timer(struct otg_instance *otg, int usec)
{
	ktime_t expires;

        TRACE_MSG1(otg->ocd->TAG, "usec: %d", usec);

	g_mxc_hr_active = FALSE;
	TRACE_MSG1(otg->ocd->TAG, "resetting active: %d", g_mxc_hr_active);

	hrtimer_cancel(&g_mxc_hrt_timer);

	if (usec == 0) {
		goto cancel;
                }

	g_mxc_hr_active = TRUE;
	TRACE_MSG1(otg->ocd->TAG, "setting active: %d", g_mxc_hr_active);

	if (usec < 100) {
		TRACE_MSG1(otg->ocd->TAG, "usec: %d set to minimum 100", usec);
		usec = 100;
        }
	hrtimer_init(&g_mxc_hrt_timer, CLOCK_REALTIME, HRTIMER_REL);
	g_mxc_hrt_timer.function = mxc_hrt_callback;
	expires = ktime_set(usec / 1000000, (usec % 1000000) * 1000);

	hrtimer_start(&g_mxc_hrt_timer, expires, HRTIMER_REL);

      cancel:
        return 0;
}

/*!
 * mxc_hrt_trace_ticks() - get current ticks
 * GPT3 is setup as free running clock at 266 Mhz - 1/266 = .0037
 */
otg_tick_t mxc_hrt_trace_ticks(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return ((tv.tv_sec * USEC_PER_SEC) + tv.tv_usec);
}

/*!
 * mxc_hrt_trace_elapsed() - return micro-seconds between two tick values
 *
 * GPT3 is setup as free running clock at 266 Mhz - 1/266 = .003759
 *
 * MPLL = 266, FCLK = 266, BCLK = 88
 *
 */
otg_tick_t mxc_hrt_trace_elapsed(otg_tick_t * t1, otg_tick_t * t2)
{
        otg_tick_t ticks = (*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1);
        return ticks;
}

/*!
 * mxc_hrt_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 * @param otg
 * @param divisor
 * @param multiplier
 * @return 0 for finish, or other on error
 */
int mxc_hrt_mod_init(struct otg_instance *otg, int divisor, int multiplier)
{
	g_mxc_hrt_divisor = divisor;
	g_mxc_hrt_multiplier = multiplier;

	hrtimer_init(&g_mxc_hrt_timer, CLOCK_REALTIME, HRTIMER_ABS);
	g_mxc_hrt_timer.function = mxc_hrt_callback;
	g_otg = otg;

        return 0;
}

/*!
 * mxc_hrt_mod_exit() - de-initialize
 */
void mxc_hrt_mod_exit(void)
{
	hrtimer_cancel(&g_mxc_hrt_timer);
}

#endif  //CONFIG_OTG_HRT
