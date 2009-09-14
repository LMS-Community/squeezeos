/*
 * wm8350-battery.h  --  Comparator Aux ADC for Wolfson WM8350 PMIC
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

#ifndef __LINUX_REGULATOR_WM8350_COMPARATOR_H_
#define __LINUX_REGULATOR_WM8350_COMPARATOR_H_

/*
 * Registers
 */
 
#define WM8350_DIGITISER_CONTROL_1              0x90
#define WM8350_DIGITISER_CONTROL_2              0x91
#define WM8350_AUX1_READBACK                    0x98
#define WM8350_AUX2_READBACK                    0x99
#define WM8350_AUX3_READBACK                    0x9A
#define WM8350_AUX4_READBACK                    0x9B
#define WM8350_CHIP_TEMP_READBACK               0x9F
#define WM8350_GENERIC_COMPARATOR_CONTROL       0xA3
#define WM8350_GENERIC_COMPARATOR_1             0xA4
#define WM8350_GENERIC_COMPARATOR_2             0xA5
#define WM8350_GENERIC_COMPARATOR_3             0xA6
#define WM8350_GENERIC_COMPARATOR_4             0xA7

/*
 * R144 (0x90) - Digitiser Control (1)
 */
// lg #define WM8350_AUXADC_ENA                       0x8000  /* AUXADC_ENA */
#define WM8350_AUXADC_CTC                       0x4000  /* AUXADC_CTC */
#define WM8350_AUXADC_POLL                      0x2000  /* AUXADC_POLL */
#define WM8350_AUXADC_HIB_MODE                  0x1000  /* AUXADC_HIB_MODE */
#define WM8350_AUXADC_SEL8                      0x0080  /* AUXADC_SEL8 */
#define WM8350_AUXADC_SEL7                      0x0040  /* AUXADC_SEL7 */
#define WM8350_AUXADC_SEL6                      0x0020  /* AUXADC_SEL6 */
#define WM8350_AUXADC_SEL5                      0x0010  /* AUXADC_SEL5 */
#define WM8350_AUXADC_SEL4                      0x0008  /* AUXADC_SEL4 */
#define WM8350_AUXADC_SEL3                      0x0004  /* AUXADC_SEL3 */
#define WM8350_AUXADC_SEL2                      0x0002  /* AUXADC_SEL2 */
#define WM8350_AUXADC_SEL1                      0x0001  /* AUXADC_SEL1 */

/*
 * R145 (0x91) - Digitiser Control (2)
 */
#define WM8350_AUXADC_MASKMODE_MASK             0x3000  /* AUXADC_MASKMODE - [13:12] */
#define WM8350_AUXADC_CRATE_MASK                0x0700  /* AUXADC_CRATE - [10:8] */
#define WM8350_AUXADC_CAL                       0x0004  /* AUXADC_CAL */
#define WM8350_AUX_RBMODE                       0x0002  /* AUX_RBMODE */
#define WM8350_AUXADC_WAIT                      0x0001  /* AUXADC_WAIT */

/*
 * R152 (0x98) - AUX1 Readback
 */
#define WM8350_AUXADC_SCALE1_MASK               0x6000  /* AUXADC_SCALE1 - [14:13] */
#define WM8350_AUXADC_REF1                      0x1000  /* AUXADC_REF1 */
#define WM8350_AUXADC_DATA1_MASK                0x0FFF  /* AUXADC_DATA1 - [11:0] */

/*
 * R153 (0x99) - AUX2 Readback
 */
#define WM8350_AUXADC_SCALE2_MASK               0x6000  /* AUXADC_SCALE2 - [14:13] */
#define WM8350_AUXADC_REF2                      0x1000  /* AUXADC_REF2 */
#define WM8350_AUXADC_DATA2_MASK                0x0FFF  /* AUXADC_DATA2 - [11:0] */

/*
 * R154 (0x9A) - AUX3 Readback
 */
#define WM8350_AUXADC_SCALE3_MASK               0x6000  /* AUXADC_SCALE3 - [14:13] */
#define WM8350_AUXADC_REF3                      0x1000  /* AUXADC_REF3 */
#define WM8350_AUXADC_DATA3_MASK                0x0FFF  /* AUXADC_DATA3 - [11:0] */

/*
 * R155 (0x9B) - AUX4 Readback
 */
#define WM8350_AUXADC_SCALE4_MASK               0x6000  /* AUXADC_SCALE4 - [14:13] */
#define WM8350_AUXADC_REF4                      0x1000  /* AUXADC_REF4 */
#define WM8350_AUXADC_DATA4_MASK                0x0FFF  /* AUXADC_DATA4 - [11:0] */

/*
 * R156 (0x9C) - USB Voltage Readback
 */
#define WM8350_AUXADC_DATA_USB_MASK             0x0FFF  /* AUXADC_DATA_USB - [11:0] */

/*
 * R157 (0x9D) - LINE Voltage Readback
 */
#define WM8350_AUXADC_DATA_LINE_MASK            0x0FFF  /* AUXADC_DATA_LINE - [11:0] */

/*
 * R158 (0x9E) - BATT Voltage Readback
 */
#define WM8350_AUXADC_DATA_BATT_MASK            0x0FFF  /* AUXADC_DATA_BATT - [11:0] */

/*
 * R159 (0x9F) - Chip Temp Readback
 */
#define WM8350_AUXADC_DATA_CHIPTEMP_MASK        0x0FFF  /* AUXADC_DATA_CHIPTEMP - [11:0] */

/*
 * R163 (0xA3) - Generic Comparator Control
 */
#define WM8350_DCMP4_ENA                        0x0008  /* DCMP4_ENA */
#define WM8350_DCMP3_ENA                        0x0004  /* DCMP3_ENA */
#define WM8350_DCMP2_ENA                        0x0002  /* DCMP2_ENA */
#define WM8350_DCMP1_ENA                        0x0001  /* DCMP1_ENA */

/*
 * R164 (0xA4) - Generic comparator 1
 */
#define WM8350_DCMP1_SRCSEL_MASK                0xE000  /* DCMP1_SRCSEL - [15:13] */
#define WM8350_DCMP1_GT                         0x1000  /* DCMP1_GT */
#define WM8350_DCMP1_THR_MASK                   0x0FFF  /* DCMP1_THR - [11:0] */

/*
 * R165 (0xA5) - Generic comparator 2
 */
#define WM8350_DCMP2_SRCSEL_MASK                0xE000  /* DCMP2_SRCSEL - [15:13] */
#define WM8350_DCMP2_GT                         0x1000  /* DCMP2_GT */
#define WM8350_DCMP2_THR_MASK                   0x0FFF  /* DCMP2_THR - [11:0] */

/*
 * R166 (0xA6) - Generic comparator 3
 */
#define WM8350_DCMP3_SRCSEL_MASK                0xE000  /* DCMP3_SRCSEL - [15:13] */
#define WM8350_DCMP3_GT                         0x1000  /* DCMP3_GT */
#define WM8350_DCMP3_THR_MASK                   0x0FFF  /* DCMP3_THR - [11:0] */

/*
 * R167 (0xA7) - Generic comparator 4
 */
#define WM8350_DCMP4_SRCSEL_MASK                0xE000  /* DCMP4_SRCSEL - [15:13] */
#define WM8350_DCMP4_GT                         0x1000  /* DCMP4_GT */
#define WM8350_DCMP4_THR_MASK                   0x0FFF  /* DCMP4_THR - [11:0] */


/*
 * Interrupts.
 */
#define WM8350_IRQ_AUXADC_DATARDY		16
#define WM8350_IRQ_AUXADC_DCOMP4		17
#define WM8350_IRQ_AUXADC_DCOMP3		18
#define WM8350_IRQ_AUXADC_DCOMP2		19
#define WM8350_IRQ_AUXADC_DCOMP1		20
#define WM8350_IRQ_SYS_HYST_COMP_FAIL		21
#define WM8350_IRQ_SYS_CHIP_GT115		22
#define WM8350_IRQ_SYS_CHIP_GT140		23

/*
 * USB/2, LINE & BATT = ((VRTC * 2) / 4095)) * 10e6 uV
 * Where VRTC = 2.7 V
 */
#define WM8350_AUX_COEFF 1319

#define WM8350_AUXADC_AUX1	0
#define WM8350_AUXADC_AUX2	1
#define WM8350_AUXADC_AUX3	2
#define WM8350_AUXADC_AUX4	3
#define WM8350_AUXADC_USB	4
#define WM8350_AUXADC_LINE	5
#define WM8350_AUXADC_BATT	6
#define WM8350_AUXADC_TEMP	7

struct wm8350;

/*
 * AUX ADC Readback
 */
int wm8350_read_auxadc(struct wm8350 *wm8350, int channel, int scale, int vref);

#define wm8350_read_temp(wm8350) \
	wm8350_read_auxadc(wm8350, WM8350_AUXADC_TEMP, 0, 0)

#endif
