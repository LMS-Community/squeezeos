/*
 * wm8350-rtc.h  --  RTC driver for Wolfson WM8350 PMIC
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

#ifndef __LINUX_REGULATOR_WM8350_RTC_H
#define __LINUX_REGULATOR_WM8350_RTC_H

#include <linux/rtc.h>

/*
 * Register values.
 */
#define WM8350_RTC_SECONDS_MINUTES              0x10
#define WM8350_RTC_HOURS_DAY                    0x11
#define WM8350_RTC_DATE_MONTH                   0x12
#define WM8350_RTC_YEAR                         0x13
#define WM8350_ALARM_SECONDS_MINUTES            0x14
#define WM8350_ALARM_HOURS_DAY                  0x15
#define WM8350_ALARM_DATE_MONTH                 0x16
#define WM8350_RTC_TIME_CONTROL                 0x17

/*
 * R16 (0x10) - RTC Seconds/Minutes
 */
#define WM8350_RTC_MINS_MASK                    0x7F00  /* RTC_MINS - [14:8] */
#define WM8350_RTC_MINS_SHIFT                        8  /* RTC_MINS - [14:8] */
#define WM8350_RTC_SECS_MASK                    0x007F  /* RTC_SECS - [6:0] */
#define WM8350_RTC_SECS_SHIFT                        0  /* RTC_SECS - [6:0] */

/*
 * R17 (0x11) - RTC Hours/Day
 */
#define WM8350_RTC_DAY_MASK                     0x0700  /* RTC_DAY - [10:8] */
#define WM8350_RTC_DAY_SHIFT                         8  /* RTC_DAY - [10:8] */
#define WM8350_RTC_HPM_MASK                     0x0020  /* RTC_HPM */
#define WM8350_RTC_HPM_SHIFT                         5  /* RTC_HPM */
#define WM8350_RTC_HRS_MASK                     0x001F  /* RTC_HRS - [4:0] */
#define WM8350_RTC_HRS_SHIFT                         0  /* RTC_HRS - [4:0] */

/* Bit values for R21 (0x15) */
#define WM8350_RTC_DAY_SUN                           1  /* 1 = Sunday */
#define WM8350_RTC_DAY_MON                           2  /* 2 = Monday */
#define WM8350_RTC_DAY_TUE                           3  /* 3 = Tuesday */
#define WM8350_RTC_DAY_WED                           4  /* 4 = Wednesday */
#define WM8350_RTC_DAY_THU                           5  /* 5 = Thursday */
#define WM8350_RTC_DAY_FRI                           6  /* 6 = Friday */
#define WM8350_RTC_DAY_SAT                           7  /* 7 = Saturday */

#define WM8350_RTC_HPM_AM                            0  /* 0 = AM */
#define WM8350_RTC_HPM_PM                            1  /* 1 = PM */


/*
 * R18 (0x12) - RTC Date/Month
 */
#define WM8350_RTC_MTH_MASK                     0x1F00  /* RTC_MTH - [12:8] */
#define WM8350_RTC_MTH_SHIFT                         8  /* RTC_MTH - [12:8] */
#define WM8350_RTC_DATE_MASK                    0x003F  /* RTC_DATE - [5:0] */
#define WM8350_RTC_DATE_SHIFT                        0  /* RTC_DATE - [5:0] */

/* Bit values for R22 (0x16) */
#define WM8350_RTC_MTH_JAN                           1  /* 1 = January */
#define WM8350_RTC_MTH_FEB                           2  /* 2 = February */
#define WM8350_RTC_MTH_MAR                           3  /* 3 = March */
#define WM8350_RTC_MTH_APR                           4  /* 4 = April */
#define WM8350_RTC_MTH_MAY                           5  /* 5 = May */
#define WM8350_RTC_MTH_JUN                           6  /* 6 = June */
#define WM8350_RTC_MTH_JUL                           7  /* 7 = July */
#define WM8350_RTC_MTH_AUG                           8  /* 8 = August */
#define WM8350_RTC_MTH_SEP                           9  /* 9 = September */
#define WM8350_RTC_MTH_OCT                          10  /* 10 = October */
#define WM8350_RTC_MTH_NOV                          11  /* 11 = November */
#define WM8350_RTC_MTH_DEC                          12  /* 12 = December */
#define WM8350_RTC_MTH_JAN_BCD                    0x01  /* 1 = January */
#define WM8350_RTC_MTH_FEB_BCD                    0x02  /* 2 = February */
#define WM8350_RTC_MTH_MAR_BCD                    0x03  /* 3 = March */
#define WM8350_RTC_MTH_APR_BCD                    0x04  /* 4 = April */
#define WM8350_RTC_MTH_MAY_BCD                    0x05  /* 5 = May */
#define WM8350_RTC_MTH_JUN_BCD                    0x06  /* 6 = June */
#define WM8350_RTC_MTH_JUL_BCD                    0x07  /* 7 = July */
#define WM8350_RTC_MTH_AUG_BCD                    0x08  /* 8 = August */
#define WM8350_RTC_MTH_SEP_BCD                    0x09  /* 9 = September */
#define WM8350_RTC_MTH_OCT_BCD                    0x10  /* 10 = October */
#define WM8350_RTC_MTH_NOV_BCD                    0x11  /* 11 = November */
#define WM8350_RTC_MTH_DEC_BCD                    0x12  /* 12 = December */

/*
 * R19 (0x13) - RTC Year
 */
#define WM8350_RTC_YHUNDREDS_MASK               0x3F00  /* RTC_YHUNDREDS - [13:8] */
#define WM8350_RTC_YHUNDREDS_SHIFT                   8  /* RTC_YHUNDREDS - [13:8] */
#define WM8350_RTC_YUNITS_MASK                  0x00FF  /* RTC_YUNITS - [7:0] */
#define WM8350_RTC_YUNITS_SHIFT                      0  /* RTC_YUNITS - [7:0] */

/*
 * R20 (0x14) - Alarm Seconds/Minutes
 */
#define WM8350_RTC_ALMMINS_MASK                 0x7F00  /* RTC_ALMMINS - [14:8] */
#define WM8350_RTC_ALMMINS_SHIFT                     8  /* RTC_ALMMINS - [14:8] */
#define WM8350_RTC_ALMSECS_MASK                 0x007F  /* RTC_ALMSECS - [6:0] */
#define WM8350_RTC_ALMSECS_SHIFT                     0  /* RTC_ALMSECS - [6:0] */

/* Bit values for R20 (0x14) */
#define WM8350_RTC_ALMMINS_DONT_CARE                -1  /* -1 = don't care */

#define WM8350_RTC_ALMSECS_DONT_CARE                -1  /* -1 = don't care */

/*
 * R21 (0x15) - Alarm Hours/Day
 */
#define WM8350_RTC_ALMDAY_MASK                  0x0F00  /* RTC_ALMDAY - [11:8] */
#define WM8350_RTC_ALMDAY_SHIFT                      8  /* RTC_ALMDAY - [11:8] */
#define WM8350_RTC_ALMHPM_MASK                  0x0020  /* RTC_ALMHPM */
#define WM8350_RTC_ALMHPM_SHIFT                      5  /* RTC_ALMHPM */
#define WM8350_RTC_ALMHRS_MASK                  0x001F  /* RTC_ALMHRS - [4:0] */
#define WM8350_RTC_ALMHRS_SHIFT                      0  /* RTC_ALMHRS - [4:0] */

/* Bit values for R21 (0x15) */
#define WM8350_RTC_ALMDAY_DONT_CARE                 -1  /* -1 = don't care */
#define WM8350_RTC_ALMDAY_SUN                        1  /* 1 = Sunday */
#define WM8350_RTC_ALMDAY_MON                        2  /* 2 = Monday */
#define WM8350_RTC_ALMDAY_TUE                        3  /* 3 = Tuesday */
#define WM8350_RTC_ALMDAY_WED                        4  /* 4 = Wednesday */
#define WM8350_RTC_ALMDAY_THU                        5  /* 5 = Thursday */
#define WM8350_RTC_ALMDAY_FRI                        6  /* 6 = Friday */
#define WM8350_RTC_ALMDAY_SAT                        7  /* 7 = Saturday */

#define WM8350_RTC_ALMHPM_AM                         0  /* 0 = AM */
#define WM8350_RTC_ALMHPM_PM                         1  /* 1 = PM */

#define WM8350_RTC_ALMHRS_DONT_CARE                 -1  /* -1 = don't care */

/*
 * R22 (0x16) - Alarm Date/Month
 */
#define WM8350_RTC_ALMMTH_MASK                  0x1F00  /* RTC_ALMMTH - [12:8] */
#define WM8350_RTC_ALMMTH_SHIFT                      8  /* RTC_ALMMTH - [12:8] */
#define WM8350_RTC_ALMDATE_MASK                 0x003F  /* RTC_ALMDATE - [5:0] */
#define WM8350_RTC_ALMDATE_SHIFT                     0  /* RTC_ALMDATE - [5:0] */

/* Bit values for R22 (0x16) */
#define WM8350_RTC_ALMDATE_DONT_CARE                -1  /* -1 = don't care */

#define WM8350_RTC_ALMMTH_DONT_CARE                 -1  /* -1 = don't care */
#define WM8350_RTC_ALMMTH_JAN                        1  /* 1 = January */
#define WM8350_RTC_ALMMTH_FEB                        2  /* 2 = February */
#define WM8350_RTC_ALMMTH_MAR                        3  /* 3 = March */
#define WM8350_RTC_ALMMTH_APR                        4  /* 4 = April */
#define WM8350_RTC_ALMMTH_MAY                        5  /* 5 = May */
#define WM8350_RTC_ALMMTH_JUN                        6  /* 6 = June */
#define WM8350_RTC_ALMMTH_JUL                        7  /* 7 = July */
#define WM8350_RTC_ALMMTH_AUG                        8  /* 8 = August */
#define WM8350_RTC_ALMMTH_SEP                        9  /* 9 = September */
#define WM8350_RTC_ALMMTH_OCT                       10  /* 10 = October */
#define WM8350_RTC_ALMMTH_NOV                       11  /* 11 = November */
#define WM8350_RTC_ALMMTH_DEC                       12  /* 12 = December */
#define WM8350_RTC_ALMMTH_JAN_BCD                 0x01  /* 1 = January */
#define WM8350_RTC_ALMMTH_FEB_BCD                 0x02  /* 2 = February */
#define WM8350_RTC_ALMMTH_MAR_BCD                 0x03  /* 3 = March */
#define WM8350_RTC_ALMMTH_APR_BCD                 0x04  /* 4 = April */
#define WM8350_RTC_ALMMTH_MAY_BCD                 0x05  /* 5 = May */
#define WM8350_RTC_ALMMTH_JUN_BCD                 0x06  /* 6 = June */
#define WM8350_RTC_ALMMTH_JUL_BCD                 0x07  /* 7 = July */
#define WM8350_RTC_ALMMTH_AUG_BCD                 0x08  /* 8 = August */
#define WM8350_RTC_ALMMTH_SEP_BCD                 0x09  /* 9 = September */
#define WM8350_RTC_ALMMTH_OCT_BCD                 0x10  /* 10 = October */
#define WM8350_RTC_ALMMTH_NOV_BCD                 0x11  /* 11 = November */
#define WM8350_RTC_ALMMTH_DEC_BCD                 0x12  /* 12 = December */

/*
 * R23 (0x17) - RTC Time Control
 */
#define WM8350_RTC_BCD                          0x8000  /* RTC_BCD */
#define WM8350_RTC_BCD_MASK                     0x8000  /* RTC_BCD */
#define WM8350_RTC_BCD_SHIFT                        15  /* RTC_BCD */
#define WM8350_RTC_12HR                         0x4000  /* RTC_12HR */
#define WM8350_RTC_12HR_MASK                    0x4000  /* RTC_12HR */
#define WM8350_RTC_12HR_SHIFT                       14  /* RTC_12HR */
#define WM8350_RTC_DST                          0x2000  /* RTC_DST */
#define WM8350_RTC_DST_MASK                     0x2000  /* RTC_DST */
#define WM8350_RTC_DST_SHIFT                        13  /* RTC_DST */
#define WM8350_RTC_SET                          0x0800  /* RTC_SET */
#define WM8350_RTC_SET_MASK                     0x0800  /* RTC_SET */
#define WM8350_RTC_SET_SHIFT                        11  /* RTC_SET */
#define WM8350_RTC_STS                          0x0400  /* RTC_STS */
#define WM8350_RTC_STS_MASK                     0x0400  /* RTC_STS */
#define WM8350_RTC_STS_SHIFT                        10  /* RTC_STS */
#define WM8350_RTC_ALMSET                       0x0200  /* RTC_ALMSET */
#define WM8350_RTC_ALMSET_MASK                  0x0200  /* RTC_ALMSET */
#define WM8350_RTC_ALMSET_SHIFT                      9  /* RTC_ALMSET */
#define WM8350_RTC_ALMSTS                       0x0100  /* RTC_ALMSTS */
#define WM8350_RTC_ALMSTS_MASK                  0x0100  /* RTC_ALMSTS */
#define WM8350_RTC_ALMSTS_SHIFT                      8  /* RTC_ALMSTS */
#define WM8350_RTC_PINT                         0x0070  /* RTC_PINT - [6:4] */
#define WM8350_RTC_PINT_MASK                    0x0070  /* RTC_PINT - [6:4] */
#define WM8350_RTC_PINT_SHIFT                        4  /* RTC_PINT - [6:4] */
#define WM8350_RTC_DSW                          0x000F  /* RTC_DSW - [3:0] */
#define WM8350_RTC_DSW_MASK                     0x000F  /* RTC_DSW - [3:0] */
#define WM8350_RTC_DSW_SHIFT                         0  /* RTC_DSW - [3:0] */

/* Bit values for R23 (0x17) */
#define WM8350_RTC_BCD_BINARY                        0  /* 0 = binary */
#define WM8350_RTC_BCD_BCD                           1  /* 1 = BCD */

#define WM8350_RTC_12HR_24HR                         0  /* 0 = 24-hour */
#define WM8350_RTC_12HR_12HR                         1  /* 1 = 12-hour */

#define WM8350_RTC_DST_DISABLED                      0  /* 0 = disabled */
#define WM8350_RTC_DST_ENABLED                       1  /* 1 = enabled */

#define WM8350_RTC_SET_RUN                           0  /* 0 = let RTC run */
#define WM8350_RTC_SET_SET                           1  /* 1 = request RTC set */

#define WM8350_RTC_STS_RUNNING                       0  /* 0 = RTC running */
#define WM8350_RTC_STS_STOPPED                       1  /* 1 = RTC stopped */

#define WM8350_RTC_ALMSET_RUN                        0  /* 0 = let RTC alarm run */
#define WM8350_RTC_ALMSET_SET                        1  /* 1 = request RTC alarm set */

#define WM8350_RTC_ALMSTS_RUNNING                    0  /* 0 = RTC alarm running */
#define WM8350_RTC_ALMSTS_STOPPED                    1  /* 1 = RTC alarm stopped */

#define WM8350_RTC_PINT_DISABLED                     0  /* 0 = disabled */
#define WM8350_RTC_PINT_SECS                         1  /* 1 = seconds rollover */
#define WM8350_RTC_PINT_MINS                         2  /* 2 = minutes rollover */
#define WM8350_RTC_PINT_HRS                          3  /* 3 = hours rollover */
#define WM8350_RTC_PINT_DAYS                         4  /* 4 = days rollover */
#define WM8350_RTC_PINT_MTHS                         5  /* 5 = months rollover */

#define WM8350_RTC_DSW_DISABLED                      0  /* 0 = disabled */
#define WM8350_RTC_DSW_1HZ                           1  /* 1 = 1Hz */
#define WM8350_RTC_DSW_2HZ                           2  /* 2 = 2Hz */
#define WM8350_RTC_DSW_4HZ                           3  /* 3 = 4Hz */
#define WM8350_RTC_DSW_8HZ                           4  /* 4 = 8Hz */
#define WM8350_RTC_DSW_16HZ                          5  /* 5 = 16Hz */
#define WM8350_RTC_DSW_32HZ                          6  /* 6 = 32Hz */
#define WM8350_RTC_DSW_64HZ                          7  /* 7 = 64Hz */
#define WM8350_RTC_DSW_128HZ                         8  /* 8 = 128Hz */
#define WM8350_RTC_DSW_256HZ                         9  /* 9 = 256Hz */
#define WM8350_RTC_DSW_512HZ                        10  /* 10 = 512Hz */
#define WM8350_RTC_DSW_1024HZ                       11  /* 11 = 1024Hz */

/*
 * R218 (0xDA) - RTC Tick Control
 */
// lg #define WM8350_RTC_TICK_ENA                     0x8000  /* RTC_TICK_ENA */
#define WM8350_RTC_TICKSTS                      0x4000  /* RTC_TICKSTS */
#define WM8350_RTC_CLKSRC                       0x2000  /* RTC_CLKSRC */
// lg #define WM8350_OSC32K_ENA                       0x1000  /* OSC32K_ENA */
#define WM8350_RTC_TRIM_MASK                    0x03FF  /* RTC_TRIM - [9:0] */

/*
 * RTC Interrupts.
 */
#define WM8350_IRQ_RTC_PER		7
#define WM8350_IRQ_RTC_SEC		8
#define WM8350_IRQ_RTC_ALM		9

struct wm8350_rtc {
	struct device dev;
	struct rtc_device *rtc;
	int pie_freq;
	int pie_enabled;
	int update_enabled;
	int alarm_enabled;
	int per_irq; /* platform specific period irq */
};
#define to_wm8350_rtc_device(d) container_of(d, struct wm8350_rtc, dev)
 

#endif
