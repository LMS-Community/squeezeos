/*
 * MXC Power Management Routines
 *
 * Original code for the SA11x0:
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * Modified for the PXA250 by Nicolas Pitre:
 * Copyright (c) 2002 Monta Vista Software, Inc.
 *
 * Modified for the OMAP1510 by David Singleton:
 * Copyright (c) 2002 Monta Vista Software, Inc.
 *
 * Cleanup 2004 for OMAP1510/1610 by Dirk Behme <dirk.behme@de.bosch.com>
 *
 * Modified for the MXC by Vitaly Wool
 * Copyright (c) 2005 Monta Vista Software, Inc.
 *
 * Ported to MXC91321 by Armin Kuster
 * Copyright (c) 2006 MontaVista Software, Inc.
 *
 * Added .valid hook in mxc_suspend_ops
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/suspend.h>

#include <asm/arch/mxc_pm.h>

/*
 * TODO: whatta save?
 */

static int mxc_suspend_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_MEM:
		mxc_pm_lowpower(DSM_MODE);
		break;
	case PM_SUSPEND_STANDBY:
		mxc_pm_lowpower(STOP_MODE);
		break;
	default:
		return -1;
	}
	return 0;
}

/*
 * Called after processes are frozen, but before we shut down devices.
 */
static int mxc_suspend_prepare(void)
{
	return 0;
}

/*
 * Called after devices are re-setup, but before processes are thawed.
 */
static void mxc_suspend_finish(void)
{
	return;
}

static int mxc_pm_valid(suspend_state_t state)
{
	return (state > PM_SUSPEND_ON && state <= PM_SUSPEND_MAX);
}

struct platform_suspend_ops mxc_suspend_ops = {
	.valid = mxc_pm_valid,
	.prepare = mxc_suspend_prepare,
	.enter = mxc_suspend_enter,
	.finish = mxc_suspend_finish,
};

static int __init mxc_pm_init(void)
{
	printk(KERN_INFO "Power Management for Freescale MXC91321.\n");
	suspend_set_ops(&mxc_suspend_ops);

	return 0;
}

late_initcall(mxc_pm_init);
