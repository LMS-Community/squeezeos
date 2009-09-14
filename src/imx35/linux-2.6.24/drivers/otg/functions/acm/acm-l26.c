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
 * otg/functions/acm/acm-l24-os.c
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/functions/acm/acm-l26.c|20070911235624|22864
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/functions/acm/acm-l26.c
 * @brief ACM Function Driver private defines
 *
 * This is the linux module wrapper for the tty-if function driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |
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
 *    Loading         | acm-l26  |      <-----
 *                    +----------+
 *
 * @ingroup ACMFunction
 */


//#include <otg/osversion.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

MOD_AUTHOR ("sl@belcarra.com");

MOD_DESCRIPTION ("Belcarra TTY Function");
EMBED_LICENSE();

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

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include <linux/capability.h>
#include <otg/otg-trace.h>
#include "acm.h"
#include "tty.h"

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (max_queued_urbs, "Maximum TX Queued Urbs", 0);
MOD_PARM_INT (max_queued_bytes, "Maximum TX Queued Bytes", 0);

/*! acm_l26_modinit - module init
 *
 * This is called immediately after the module is loaded or during
 * the kernel driver initialization if linked into the kernel.
 *
 */
STATIC int acm_l26_modinit (void)
{
        BOOL tty_l26 = FALSE, tty_if = FALSE;

        /* register tty  and usb interface function drivers
         */
        TTY = otg_trace_obtain_tag(NULL, "acm");
        THROW_UNLESS(tty_l26 = BOOLEAN(!tty_l26_init("tty_if", 2)), error);
        THROW_UNLESS(tty_if = BOOLEAN(!tty_if_init()), error);

        CATCH(error) {
                if (tty_l26) tty_l26_exit();
                if (tty_if) tty_if_exit();
                otg_trace_invalidate_tag(TTY);
                return -EINVAL;
        }
        return 0;
}


/*! acm_l26_modexit - module cleanup
 *
 * This is called prior to the module being unloaded.
 */
STATIC void acm_l26_modexit (void)
{
        /* de-register as tty  and usb drivers */
        tty_l26_exit();
        tty_if_exit();
        otg_trace_invalidate_tag(TTY);
}

module_init (acm_l26_modinit);
module_exit (acm_l26_modexit);
