/*
 * wm8350-bus.h  --  Power Supply Driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    8th Feb 2007   Initial version.
 *
 */

#ifndef __LINUX_REGULATOR_WM8350_WDT_H_
#define __LINUX_REGULATOR_WM8350_WDT_H_

#include <linux/device.h>

#define WM8350_IRQ_SYS_WDOG_TO		24

struct wm8350_wdg {
	struct device dev;
};
#define to_wm8350_wdg_device(d) container_of(d, struct wm8350_wdg, dev)


#endif
