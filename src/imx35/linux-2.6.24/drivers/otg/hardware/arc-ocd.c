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
 * otg/hardware/arc-ocd.c -- Generic Linux 2.6 Timer
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/arc/arc-ocd.c|20070612232808|00655
 *
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Shahrad Payandeh Lynne <sp@lbelcarra.com>,
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/arc-ocd.c
 * @brief Belcarra Freescale ARC Device driver.
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
 * @ingroup ARC
 * @ingroup OCD
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */


#include <otg/pcd-include.h>
#include <otg/otg-dev.h>

#include "arc-hardware.h"

//#warning NEED CONDITIONAL for imx-regs.h

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/time.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/leds.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>


#define MXC_GPT_GPTCNT          (IO_ADDRESS(GPT1_BASE_ADDR + 0x24))



/* ********************************************************************************************* */

/*!
 * arc_ocd_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds).
 *
 * @param otg
 * @param usec
 */
int arc_ocd_start_timer(struct otg_instance *otg, int usec)
{
        otg_event(otg, TMOUT, otg->ocd->TAG, "l26 Timer - FAKE TMOUT");
        return 0;
}

/* arc_ocd_ticks - get current ticks
 */
otg_tick_t arc_ocd_ticks (void)
{
        unsigned long ticks = 0;

        ticks = __raw_readl(MXC_GPT_GPTCNT);

        return (otg_tick_t) ticks;
}

/* arc_ocd_elapsed - return micro-seconds between two tick values
 */
otg_tick_t arc_ocd_elapsed(otg_tick_t *t1, otg_tick_t *t2)
{
        otg_tick_t ticks = (((*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1)));
        //return (otg_tick_t) ticks / 16; // 381 vs 367
        //return (otg_tick_t) (ticks * 2) / 31; // 201 vs 188
        return (otg_tick_t) ticks / 17;  // 185 vs 189
}


struct ocd_ops arc_ocd_ops = {
        .start_timer = arc_ocd_start_timer,
        .ticks = arc_ocd_ticks,
        .elapsed = arc_ocd_elapsed,
};


static void
arc_ocd_dev_remove(struct otg_dev *otg_dev)
{
}

static int
arc_ocd_dev_probe(struct otg_dev *otg_dev)
{
        return 0;
}


/* ********************************************************************************************* */
static struct otg_dev_driver arc_ocd_driver = {
        .name =         "arc_udc",
        .id =           OTG_DRIVER_OCD,

        .probe =        arc_ocd_dev_probe,
        .remove =       arc_ocd_dev_remove,

        .ops =          &arc_ocd_ops,
};

/* ********************************************************************************************* */

/*! arc_ocd_module_init() - module init
 */
int arc_ocd_module_init (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return otg_dev_register_driver(otg_device_driver, &arc_ocd_driver);
}

/*!
 * arc_ocd_module_exit() - module exit
 */
void arc_ocd_module_exit (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        otg_dev_unregister_driver (otg_device_driver, &arc_ocd_driver);
}
