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
 * otg/hardware/l26-ocd-dev.c -- Generic Linux 2.6 timer Dev driver wrapper
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/otglib/l26-ocd-dev.c|20070612232808|61728
 *
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/l26-ocd-dev.c
 * @brief Generic L26 OCD Driver.
 *
 * OTG-DEV wrapper for l26-ocd.c
 *
 * @ingroup LINUXOS
 * @ingroup OTGDEV
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
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/dma.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-dev.h>
#include <otg/otg-ocd.h>

extern struct ocd_ops l26_ocd_ops;

/* ********************************************************************************************* */


#ifdef CONFIG_HIGH_RES_TIMERS
void l26_ocd_remove(struct otg_dev *otg_dev);
int l26_ocd_probe(struct otg_dev *otg_dev);

static void
l26_ocd_dev_remove(struct otg_dev *otg_dev)
{
        l26_ocd_remove(otg_dev);
}

static int
l26_ocd_dev_probe(struct otg_dev *otg_dev)
{
        return l26_ocd_probe(otg_dev);
}

#endif /* CONFIG_HIGH_RES_TIMERS */

/* ********************************************************************************************* */
static struct otg_dev_driver arc_ocd_driver = {
        .name =         "l26-ocd-dev",
        .id =           OTG_DRIVER_OCD,
        #ifdef CONFIG_HIGH_RES_TIMERS
        .probe =        l26_ocd_dev_probe,
        .remove =       l26_ocd_dev_remove,
        #endif /* CONFIG_HIGH_RES_TIMERS */
        .ops =          &l26_ocd_ops,
};

/* ********************************************************************************************* */

/*! l26_ocd_dev_module_init() - module init
 */
int l26_ocd_dev_module_init (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return otg_dev_register_driver(otg_device_driver, &arc_ocd_driver);
}

/*!
 * l26_ocd_dev_module_exit() - module exit
 */
void l26_ocd_dev_module_exit (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        otg_dev_unregister_driver (otg_device_driver, &arc_ocd_driver);
}
