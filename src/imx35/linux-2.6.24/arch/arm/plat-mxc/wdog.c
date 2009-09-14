/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file plat-mxc/wdog.c
 * @brief This file contains watchdog timer implementations.
 *
 * This file contains watchdog timer implementations for timer tick.
 *
 * @ingroup WDOG
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <asm/io.h>

/* Watchdog timers are not enabled by default */
#undef WDOG1_ENABLE		/* not defined by default */
#undef WDOG2_ENABLE		/* not defined by default */

#define WDOG1_TIMEOUT           4000	/* WDOG1 timeout in ms */
#define WDOG2_TIMEOUT           (WDOG1_TIMEOUT / 2)	/* WDOG2 timeout in ms */
#define WDOG_SERVICE_PERIOD     (WDOG2_TIMEOUT / 2)	/* time interval in ms to service WDOGs */

#if (WDOG1_TIMEOUT < WDOG2_TIMEOUT)
#error WDOG1_TIMEOUT must be greater than WDOG2_TIMEOUT!
#endif

#if (WDOG_SERVICE_PERIOD >= (WDOG2_TIMEOUT - 1000 / HZ))
#error WDOG_SERVICE_PERIOD is too large!
#endif

/* maximum timeout is 128s based on 2Hz clock */
#if ((WDOG1_TIMEOUT/1000) > 128)
#error WDOG time out has to be less than 128 seconds!
#endif

#define WDOG_WT                 0x8	/* WDOG WT starting bit inside WCR */
#define WCR_WOE_BIT             (1 << 6)
#define WCR_WDA_BIT             (1 << 5)
#define WCR_SRS_BIT             (1 << 4)
#define WCR_WRE_BIT             (1 << 3)
#define WCR_WDE_BIT             (1 << 2)
#define WCR_WDBG_BIT            (1 << 1)
#define WCR_WDZST_BIT           (1 << 0)

/*
 * WatchDog
 */
#define WDOG_WCR	0	/* 16bit watchdog control reg */
#define WDOG_WSR	2	/* 16bit watchdog service reg */
#define WDOG_WRSR	4	/* 16bit watchdog reset status reg */

/*!
 * The base addresses for the WDOG modules
 */
static unsigned long wdog_base[2] = {
	IO_ADDRESS(WDOG1_BASE_ADDR),
#ifdef WDOG2_BASE_ADDR
	IO_ADDRESS(WDOG2_BASE_ADDR),
#endif
};

/*!
 * The corresponding WDOG won't be serviced unless the corresponding global
 * variable is set to a non-zero value.
 */
volatile unsigned short g_wdog1_enabled, g_wdog2_enabled;

/* WDOG WCR register's WT value */
static int wdog_tmout[2] = { WDOG1_TIMEOUT, WDOG2_TIMEOUT };

static int g_wdog_count = 0;

/*!
 * This function provides the required service for the watchdog to avoid
 * the timeout.
 */
void mxc_kick_wd(void)
{
	if (++g_wdog_count >= ((WDOG_SERVICE_PERIOD / 1000) * HZ)) {
		g_wdog_count = 0;	/* reset */
		if (g_wdog1_enabled) {
			/* issue the service sequence instructions */
			__raw_writew(0x5555, wdog_base[0] + WDOG_WSR);
			__raw_writew(0xAAAA, wdog_base[0] + WDOG_WSR);
		}
		if (g_wdog2_enabled) {
			/* issue the service sequence instructions */
			__raw_writew(0x5555, wdog_base[1] + WDOG_WSR);
			__raw_writew(0xAAAA, wdog_base[1] + WDOG_WSR);
		}
	}
}

/*!
 * This is the watchdog initialization routine to setup the timeout
 * value and enable it.
 */
void mxc_wd_init(int port)
{
	u32 reg;
	unsigned short timeout = ((wdog_tmout[port] / 1000) * 2) << WDOG_WT;

	if (port == 0) {
		/* enable WD, suspend WD in DEBUG mode */
		reg = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WDA_BIT | WCR_WDE_BIT | WCR_WDBG_BIT;
	} else {
		/* enable WD, suspend WD in DEBUG, low power modes and WRE=1 */
		reg = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WRE_BIT | WCR_WDE_BIT | WCR_WDBG_BIT;
	}
	__raw_writew(reg, wdog_base[port] + WDOG_WCR);
}

void mxc_wd_reset(void)
{
	u16 reg;
	struct clk *clk;

	clk = clk_get(NULL, "wdog_clk");
	clk_enable(clk);

	reg = __raw_readw(wdog_base[0] + WDOG_WCR) & ~WCR_SRS_BIT;
	reg |= WCR_WDE_BIT;
	__raw_writew(reg, wdog_base[0] + WDOG_WCR);
}

/*!
 * This function is used to initialize the GPT to produce an interrupt
 * every 10 msec. It is called by the start_kernel() during system startup.
 */
static int __init wdog_init(void)
{

#ifdef WDOG1_BASE_ADDR
#ifdef WDOG1_ENABLE
	mxc_wd_init(0);
	g_wdog1_enabled = 1;
#else
	g_wdog1_enabled = __raw_readw(wdog_base[0] + WDOG_WCR) & WCR_WDE_BIT;
#endif
#endif

#ifdef WDOG2_BASE_ADDR
#ifdef WDOG2_ENABLE
	mxc_wd_init(1);
	g_wdog2_enabled = 1;
#else
	g_wdog2_enabled = __raw_readw(wdog_base[1] + WDOG_WCR) & WCR_WDE_BIT;
#endif
#endif

	mxc_kick_wd();

	if (g_wdog1_enabled)
		pr_info("MXC WDOG1 Enabled\n");
	if (g_wdog2_enabled)
		pr_info("MXC WDOG2 Enabled\n");
	return 0;
}

postcore_initcall(wdog_init);

EXPORT_SYMBOL(g_wdog1_enabled);
EXPORT_SYMBOL(g_wdog2_enabled);
EXPORT_SYMBOL(mxc_wd_init);
