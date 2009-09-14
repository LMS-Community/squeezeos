/*
 * wm8350-battery.h  --  Power Supply Driver for Wolfson WM8350 PMIC
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

#ifndef __LINUX_REGULATOR_WM8350_SUPPLY_H_
#define __LINUX_REGULATOR_WM8350_SUPPLY_H_

#include <linux/mutex.h>
#include <linux/power_supply.h>

/*
 * Charger registers
 */
#define WM8350_BATTERY_CHARGER_CONTROL_1        0xA8
#define WM8350_BATTERY_CHARGER_CONTROL_2        0xA9
#define WM8350_BATTERY_CHARGER_CONTROL_3        0xAA

/*
 * R168 (0xA8) - Battery Charger Control 1
 */
#define WM8350_CHG_ENA_R168                     0x8000  /* CHG_ENA */
#define WM8350_CHG_THR                          0x2000  /* CHG_THR */
#define WM8350_CHG_EOC_SEL_MASK                 0x1C00  /* CHG_EOC_SEL - [12:10] */
#define WM8350_CHG_TRICKLE_TEMP_CHOKE           0x0200  /* CHG_TRICKLE_TEMP_CHOKE */
#define WM8350_CHG_TRICKLE_USB_CHOKE            0x0100  /* CHG_TRICKLE_USB_CHOKE */
#define WM8350_CHG_RECOVER_T                    0x0080  /* CHG_RECOVER_T */
#define WM8350_CHG_END_ACT                      0x0040  /* CHG_END_ACT */
#define WM8350_CHG_FAST                         0x0020  /* CHG_FAST */
#define WM8350_CHG_FAST_USB_THROTTLE            0x0010  /* CHG_FAST_USB_THROTTLE */
#define WM8350_CHG_NTC_MON                      0x0008  /* CHG_NTC_MON */
#define WM8350_CHG_BATT_HOT_MON                 0x0004  /* CHG_BATT_HOT_MON */
#define WM8350_CHG_BATT_COLD_MON                0x0002  /* CHG_BATT_COLD_MON */
#define WM8350_CHG_CHIP_TEMP_MON                0x0001  /* CHG_CHIP_TEMP_MON */

/*
 * R169 (0xA9) - Battery Charger Control 2
 */
#define WM8350_CHG_ACTIVE                       0x8000  /* CHG_ACTIVE */
#define WM8350_CHG_PAUSE                        0x4000  /* CHG_PAUSE */
#define WM8350_CHG_STS_MASK                     0x3000  /* CHG_STS - [13:12] */
#define WM8350_CHG_TIME_MASK                    0x0F00  /* CHG_TIME - [11:8] */
#define WM8350_CHG_MASK_WALL_FB                 0x0080  /* CHG_MASK_WALL_FB */
#define WM8350_CHG_TRICKLE_SEL                  0x0040  /* CHG_TRICKLE_SEL */
#define WM8350_CHG_VSEL_MASK                    0x0030  /* CHG_VSEL - [5:4] */
#define WM8350_CHG_ISEL_MASK                    0x000F  /* CHG_ISEL - [3:0] */
#define WM8350_CHG_STS_OFF                      0x0000
#define WM8350_CHG_STS_TRICKLE                  0x1000
#define WM8350_CHG_STS_FAST                     0x2000

/*
 * R170 (0xAA) - Battery Charger Control 3
 */
#define WM8350_CHG_THROTTLE_T_MASK              0x0060  /* CHG_THROTTLE_T - [6:5] */
#define WM8350_CHG_SMART                        0x0010  /* CHG_SMART */
#define WM8350_CHG_TIMER_ADJT_MASK              0x000F  /* CHG_TIMER_ADJT - [3:0] */

/*
 * Charger Interrupts
 */
#define WM8350_IRQ_CHG_BAT_HOT		0
#define WM8350_IRQ_CHG_BAT_COLD		1
#define WM8350_IRQ_CHG_BAT_FAIL		2
#define WM8350_IRQ_CHG_TO		3
#define WM8350_IRQ_CHG_END		4
#define WM8350_IRQ_CHG_START		5
#define WM8350_IRQ_CHG_FAST_RDY		6
#define WM8350_IRQ_CHG_VBATT_LT_3P9		10
#define WM8350_IRQ_CHG_VBATT_LT_3P1		11
#define WM8350_IRQ_CHG_VBATT_LT_2P85		12

/*
 * Charger Policy
 */
#define WM8350_CHG_TIME_MIN(x)	((((x - 30) / 15) & 0xf) << 8)
#define WM8350_CHG_TRICKLE_50mA		(0 << 6)
#define WM8350_CHG_TRICKLE_100mA		(1 << 6)
#define WM8350_CHG_4_05V		(0 << 4)
#define WM8350_CHG_4_10V		(1 << 4)
#define WM8350_CHG_4_15V		(2 << 4)
#define WM8350_CHG_4_20V		(3 << 4)
#define WM8350_CHG_FAST_LIMIT_mA(x)	((x / 50) & 0xf)
#define WM8350_CHG_EOC_mA(x)		(((x - 10) & 0x7) << 10)
#define WM8350_CHG_TRICKLE_3_1V		(0 << 13)
#define WM8350_CHG_TRICKLE_3_9V		(1 << 13)

/*
 * Supply Registers.
 */
#define WM8350_USB_VOLTAGE_READBACK             0x9C
#define WM8350_LINE_VOLTAGE_READBACK            0x9D
#define WM8350_BATT_VOLTAGE_READBACK            0x9E

/*
 * Supply Interrupts.
 */
#define WM8350_IRQ_USB_LIMIT		15
#define WM8350_IRQ_EXT_USB_FB		36
#define WM8350_IRQ_EXT_WALL_FB		37
#define WM8350_IRQ_EXT_BAT_FB		38

/*
 * Policy to control charger state machine.
 */
struct wm8350_charger_policy {
	struct mutex mutex;
	int enable;
	int fast_enable;
	
	/* charger state machine policy  - set in machine driver or by sysfs */
	int eoc_mA; /* end of charge current (mA)  */
	int charge_mV; /* charge voltage */
	int fast_limit_mA; /* fast charge current limit */
	int charge_timeout; /* charge timeout (mins) */
	int trickle_start_mV; /* trickle charge starts at mV */
	int trickle_charge_mA; /* trickle charge current */
};

struct wm8350_power {
	struct device dev;
	struct power_supply battery;
	struct power_supply usb;
	struct power_supply ac;
	struct wm8350_charger_policy policy;
};
#define to_wm8350_power_device(d) container_of(d, struct wm8350_power, dev)
#define to_wm8350_battery_supply(d) container_of(d, struct wm8350_power, battery)
#define to_wm8350_usb_supply(d) container_of(d, struct wm8350_power, usb)
#define to_wm8350_ac_supply(d) container_of(d, struct wm8350_power, ac)

#define wm8350_read_battery_uvolts(wm8350) \
	(wm8350_read_auxadc(wm8350, WM8350_AUXADC_BATT, 0, 0) * WM8350_AUX_COEFF)
#define wm8350_read_battery_mvolts(wm8350) \
	(wm8350_read_battery_uvolts(wm8350 / 1000)

#define wm8350_read_line_uvolts(wm8350) \
	(wm8350_read_auxadc(wm8350, WM8350_AUXADC_LINE, 0, 0) * WM8350_AUX_COEFF)
#define wm8350_read_line_mvolts(wm8350) \
	(wm8350_read_line_uvolts(wm8350 / 1000)
	
#define wm8350_read_usb_uvolts(wm8350) \
	(wm8350_read_auxadc(wm8350, WM8350_AUXADC_USB, 0, 0) * WM8350_AUX_COEFF)
#define wm8350_read_usb_mvolts(wm8350) \
	(wm8350_read_usb_uvolts(wm8350 / 1000)
	
int wm8350_charger_enable(struct wm8350_power *power, int enable);
int wm8350_fast_charger_enable(struct wm8350_power *power, int enable);

#endif /*WM8350SUPPLY_H_*/
