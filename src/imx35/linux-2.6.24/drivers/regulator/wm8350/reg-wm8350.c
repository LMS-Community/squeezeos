/*
 * wm8350_pmu.c  --  Power Management Driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood
 *         liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    23rd Jan 2007   Initial version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regulator/wm8350/wm8350.h>
#include <linux/regulator/wm8350/wm8350-bus.h>
#include <linux/regulator/wm8350/wm8350-pmic.h>
#include <linux/regulator/regulator-drv.h>

#define WM8350_PMIC_VERSION	"0.4"

/* debug */
#define WM8350_DEBUG 0
#if WM8350_DEBUG
#define dbg(format, arg...) printk(format, ## arg)
#else
#define dbg(format, arg...)
#endif

/* hundredths of uA, 405 = 4.05 uA */
static const int isink_cur[] = {
	405, 482, 573, 681, 810, 963, 1146, 1362, 1620, 1927, 2291, 2725,
	3240, 3853, 4582, 5449, 6480, 7706, 9164, 10898, 12960, 15412, 18328,
	21796, 25920, 30824, 36656, 43592, 51840, 61648, 73313, 87184,
	103680, 123297, 146626, 174368, 207360, 246594, 293251, 348737,
	414720, 493188, 586503, 697473, 829440, 986376, 1173005, 1394946,
	1658880, 1972752, 2346011, 2789892, 3317760, 3945504, 4692021,
	5579785, 6635520, 7891008, 9384042, 11159570, 13271040, 15782015,
	18768085, 22319140
};

static int get_isink_val(int uA)
{
	int i, huA = uA * 100;

	for (i = ARRAY_SIZE(isink_cur) - 1; i >= 0 ; i--) {
		if (huA > isink_cur[i])
			return i;
	}
	return 0;
}

static int wm8350_isink_set_current(struct regulator *reg, int uA)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int isink = reg->id;
	u16 val;

	switch (isink) {
	case WM8350_ISINK_A:
		val = wm8350_reg_read(wm8350, WM8350_CURRENT_SINK_DRIVER_A) &
			~WM8350_CS1_ISEL_MASK;
		wm8350_reg_write(wm8350, WM8350_CURRENT_SINK_DRIVER_A, val |
			WM8350_CS1_ENABLE | get_isink_val(uA));
		break;
	case WM8350_ISINK_B:
		val = wm8350_reg_read(wm8350, WM8350_CURRENT_SINK_DRIVER_B) &
			~WM8350_CS1_ISEL_MASK;
		wm8350_reg_write(wm8350, WM8350_CURRENT_SINK_DRIVER_B, val |
			WM8350_CS2_ENABLE | get_isink_val(uA));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int wm8350_isink_get_current(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int isink = reg->id;
	u16 val;

	switch (isink) {
	case WM8350_ISINK_A:
		val = wm8350_reg_read(wm8350, WM8350_CURRENT_SINK_DRIVER_A) &
			WM8350_CS1_ISEL_MASK;
		break;
	case WM8350_ISINK_B:
		val = wm8350_reg_read(wm8350, WM8350_CURRENT_SINK_DRIVER_B) &
			WM8350_CS1_ISEL_MASK;
		break;
	default:
		return 0;
	}

	return val * 1000;
}

/* turn on ISINK followed by DCDC */
static int wm8350_isink_enable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int isink = reg->id;

	/* TODO: add ISINK's C,D,E */
	switch (isink) {
	case WM8350_ISINK_A:
		switch (pmic->isink_A_dcdc) {
		case WM8350_DCDC_2:
		case WM8350_DCDC_5:
			wm8350_set_bits(wm8350, WM8350_POWER_MGMT_7,
				WM8350_CS1_ENA);
			wm8350_set_bits(wm8350, WM8350_DCDC_LDO_REQUESTED,
				1 << (pmic->isink_A_dcdc - WM8350_DCDC_1));
			break;
		default:
			return -EINVAL;
		}
		break;
	case WM8350_ISINK_B:
		switch (pmic->isink_B_dcdc) {
		case WM8350_DCDC_2:
		case WM8350_DCDC_5:
			wm8350_set_bits(wm8350, WM8350_POWER_MGMT_7,
				WM8350_CS2_ENA);
			wm8350_set_bits(wm8350, WM8350_DCDC_LDO_REQUESTED,
				1 << (pmic->isink_B_dcdc - WM8350_DCDC_1));
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8350_isink_disable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int isink = reg->id;

	/* TODO: add ISINK's C,D,E */
	switch (isink) {
	case WM8350_ISINK_A:
		switch (pmic->isink_A_dcdc) {
		case WM8350_DCDC_2:
		case WM8350_DCDC_5:
			wm8350_clear_bits(wm8350, WM8350_DCDC_LDO_REQUESTED,
				1 << (pmic->isink_A_dcdc - WM8350_DCDC_1));
			wm8350_clear_bits(wm8350, WM8350_POWER_MGMT_7,
				WM8350_CS1_ENA);
			break;
		default:
			return -EINVAL;
		}
		break;
	case WM8350_ISINK_B:
		switch (pmic->isink_B_dcdc) {
		case WM8350_DCDC_2:
		case WM8350_DCDC_5:
			wm8350_clear_bits(wm8350, WM8350_DCDC_LDO_REQUESTED,
				1 << (pmic->isink_B_dcdc - WM8350_DCDC_1));
			wm8350_clear_bits(wm8350, WM8350_POWER_MGMT_7,
				WM8350_CS2_ENA);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

int wm8350_isink_set_flash(struct wm8350_pmic *pmic, int isink, u16 mode,
	u16 trigger, u16 duration, u16 on_ramp, u16 off_ramp, u16 drive)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);

	switch (isink) {
	case WM8350_ISINK_A:
		wm8350_reg_write(wm8350, WM8350_CSA_FLASH_CONTROL,
			(mode ? WM8350_CS1_FLASH_MODE : 0) |
			(trigger ? WM8350_CS1_TRIGSRC : 0) |
			duration | on_ramp | off_ramp | drive);
		break;
	case WM8350_ISINK_B:
		wm8350_reg_write(wm8350, WM8350_CSB_FLASH_CONTROL,
			(mode ? WM8350_CS2_FLASH_MODE : 0) |
			(trigger ? WM8350_CS2_TRIGSRC : 0) |
			duration | on_ramp | off_ramp | drive);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_isink_set_flash);

int wm8350_isink_trigger_flash(struct wm8350_pmic *pmic, int isink)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	u16 val;

	switch (isink) {
	case WM8350_ISINK_A:
		val = wm8350_reg_read(wm8350, WM8350_CSA_FLASH_CONTROL) &
			~WM8350_CS1_DRIVE;
		wm8350_reg_write(wm8350, WM8350_CSA_FLASH_CONTROL, val |
			WM8350_CS1_DRIVE);
		break;
	case WM8350_ISINK_B:
		val = wm8350_reg_read(wm8350, WM8350_CSA_FLASH_CONTROL) &
			~WM8350_CS2_DRIVE;
		wm8350_reg_write(wm8350, WM8350_CSA_FLASH_CONTROL, val |
			WM8350_CS2_DRIVE);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int wm8350_dcdc_set_voltage(struct regulator *reg, int uV)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg, dcdc = reg->id, mV = uV / 1000;
	u16 val;

	dbg("%s %d mV %d\n", __FUNCTION__, dcdc, mV);

	switch (dcdc) {
	case WM8350_DCDC_1:
		volt_reg = WM8350_DCDC1_CONTROL;
		break;
	case WM8350_DCDC_3:
		volt_reg = WM8350_DCDC3_CONTROL;
		break;
	case WM8350_DCDC_4:
		volt_reg = WM8350_DCDC4_CONTROL;
		break;
	case WM8350_DCDC_6:
		volt_reg = WM8350_DCDC6_CONTROL;
		break;
	case WM8350_DCDC_2:
	case WM8350_DCDC_5:
	default:
		return -EINVAL;
	}

	/* all DCDC's have same mV bits */
	val = wm8350_reg_read(wm8350, volt_reg) & ~WM8350_DC1_VSEL_MASK;
	wm8350_reg_write(wm8350, volt_reg, val | WM8350_DC1_VSEL(mV));
	return 0;
}

static int wm8350_dcdc_get_voltage(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg, dcdc = reg->id;
	u16 val;

	switch (dcdc) {
	case WM8350_DCDC_1:
		volt_reg = WM8350_DCDC1_CONTROL;
		break;
	case WM8350_DCDC_3:
		volt_reg = WM8350_DCDC3_CONTROL;
		break;
	case WM8350_DCDC_4:
		volt_reg = WM8350_DCDC4_CONTROL;
		break;
	case WM8350_DCDC_6:
		volt_reg = WM8350_DCDC6_CONTROL;
		break;
	case WM8350_DCDC_2:
	case WM8350_DCDC_5:
	default:
		return -EINVAL;
	}

	/* all DCDC's have same mV bits */
	val = wm8350_reg_read(wm8350, volt_reg) & WM8350_DC1_VSEL_MASK;
	return WM8350_DC6_VSEL_V(val) * 1000;
}

int wm8350_dcdc_set_image_voltage(struct wm8350_pmic *pmic, int dcdc, int mV,
	int mode, int signal)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg;

	dbg("%s %d mV %d\n", __FUNCTION__, dcdc, mV);

	switch (dcdc) {
	case WM8350_DCDC_1:
		volt_reg = WM8350_DCDC1_LOW_POWER;
		break;
	case WM8350_DCDC_3:
		volt_reg = WM8350_DCDC3_LOW_POWER;
		break;
	case WM8350_DCDC_4:
		volt_reg = WM8350_DCDC4_LOW_POWER;
		break;
	case WM8350_DCDC_6:
		volt_reg = WM8350_DCDC6_LOW_POWER;
		break;
	case WM8350_DCDC_2:
	case WM8350_DCDC_5:
	default:
		return -EINVAL;
	}

	/* all DCDC's have same mV bits */
	wm8350_reg_write(wm8350, volt_reg, WM8350_DC1_VSEL(mV) | mode | signal);
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_dcdc_set_image_voltage);

static int wm8350_ldo_set_voltage(struct regulator *reg, int uV)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg, ldo = reg->id, mV = uV / 1000;
	u16 val;

	dbg("%s %d mV %d\n", __FUNCTION__, ldo, mV);

	switch (ldo) {
	case WM8350_LDO_1:
		volt_reg = WM8350_LDO1_CONTROL;
		break;
	case WM8350_LDO_2:
		volt_reg = WM8350_LDO2_CONTROL;
		break;
	case WM8350_LDO_3:
		volt_reg = WM8350_LDO3_CONTROL;
		break;
	case WM8350_LDO_4:
		volt_reg = WM8350_LDO4_CONTROL;
		break;
	default:
		return -EINVAL;
	}

	/* all LDO's have same mV bits */
	val = wm8350_reg_read(wm8350, volt_reg) & ~WM8350_LDO1_VSEL_MASK;
	wm8350_reg_write(wm8350, volt_reg, val | WM8350_LDO1_VSEL(mV));
	return 0;
}

static int wm8350_ldo_get_voltage(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg, ldo = reg->id;
	u16 val;

	switch (ldo) {
	case WM8350_LDO_1:
		volt_reg = WM8350_LDO1_CONTROL;
		break;
	case WM8350_LDO_2:
		volt_reg = WM8350_LDO2_CONTROL;
		break;
	case WM8350_LDO_3:
		volt_reg = WM8350_LDO3_CONTROL;
		break;
	case WM8350_LDO_4:
		volt_reg = WM8350_LDO4_CONTROL;
		break;
	default:
		return -EINVAL;
	}

	/* all LDO's have same mV bits */
	val = wm8350_reg_read(wm8350, volt_reg) & WM8350_LDO1_VSEL_MASK;
	return WM8350_LDO1_VSEL_V(val) * 1000;
}

int wm8350_ldo_set_image_voltage(struct wm8350_pmic *pmic, int ldo, int mV,
	int mode, int signal)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int volt_reg;

	dbg("%s %d mV %d\n", __FUNCTION__, ldo, mV);

	switch (ldo) {
	case WM8350_LDO_1:
		volt_reg = WM8350_LDO1_LOW_POWER;
		break;
	case WM8350_LDO_2:
		volt_reg = WM8350_LDO2_LOW_POWER;
		break;
	case WM8350_LDO_3:
		volt_reg = WM8350_LDO3_LOW_POWER;
		break;
	case WM8350_LDO_4:
		volt_reg = WM8350_LDO4_LOW_POWER;
		break;
	default:
		return -EINVAL;
	}

	/* all LDO's have same mV bits */
	wm8350_reg_write(wm8350, volt_reg, WM8350_LDO1_VSEL(mV) | mode |
			 signal);
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_ldo_set_image_voltage);

int wm8350_dcdc_set_slot(struct wm8350_pmic *pmic, int dcdc, u16 start,
	u16 stop, u16 fault)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int slot_reg;
	u16 val;

	dbg("%s %d start %d stop %d\n", __FUNCTION__, dcdc, start, stop);

	/* slot valid ? */
	if (start > 15 || stop > 15)
		return -EINVAL;

	switch (dcdc) {
	case WM8350_DCDC_1:
		slot_reg = WM8350_DCDC1_TIMEOUTS;
		break;
	case WM8350_DCDC_2:
		slot_reg = WM8350_DCDC2_TIMEOUTS;
		break;
	case WM8350_DCDC_3:
		slot_reg = WM8350_DCDC3_TIMEOUTS;
		break;
	case WM8350_DCDC_4:
		slot_reg = WM8350_DCDC4_TIMEOUTS;
		break;
	case WM8350_DCDC_5:
		slot_reg = WM8350_DCDC5_TIMEOUTS;
		break;
	case WM8350_DCDC_6:
		slot_reg = WM8350_DCDC6_TIMEOUTS;
		break;
	default:
		return -EINVAL;
	}

	val = wm8350_reg_read(wm8350, slot_reg) &
		~(WM8350_DC1_ENSLOT_MASK | WM8350_DC1_SDSLOT_MASK |
		WM8350_DC1_ERRACT_MASK);
	wm8350_reg_write(wm8350, slot_reg,
		val | (start << WM8350_DC1_ENSLOT_SHIFT) |
		(stop << WM8350_DC1_SDSLOT_SHIFT) |
		(fault << WM8350_DC1_ERRACT_SHIFT));

	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_dcdc_set_slot);

int wm8350_ldo_set_slot(struct wm8350_pmic *pmic, int ldo, u16 start,
	u16 stop)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int slot_reg;
	u16 val;

	dbg("%s %d start %d stop %d\n", __FUNCTION__, ldo, start, stop);

	/* slot valid ? */
	if (start > 15 || stop > 15)
		return -EINVAL;

	switch (ldo) {
	case WM8350_LDO_1:
		slot_reg = WM8350_LDO1_TIMEOUTS;
		break;
	case WM8350_LDO_2:
		slot_reg = WM8350_LDO2_TIMEOUTS;
		break;
	case WM8350_LDO_3:
		slot_reg = WM8350_LDO3_TIMEOUTS;
		break;
	case WM8350_LDO_4:
		slot_reg = WM8350_LDO4_TIMEOUTS;
		break;
	default:
		return -EINVAL;
	}

	val = wm8350_reg_read(wm8350, slot_reg) & ~WM8350_LDO1_SDSLOT_MASK;
	wm8350_reg_write(wm8350, slot_reg, val | ((start << 10) | (stop << 6)));
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_ldo_set_slot);

int wm8350_dcdc25_set_mode(struct wm8350_pmic *pmic, int dcdc, u16 mode,
	u16 ilim, u16 ramp, u16 feedback)
{
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	u16 val;

	dbg("%s %d mode: %s %s\n", __FUNCTION__, dcdc,
		mode ? "normal" : "boost", ilim ? "low" : "normal");

	switch (dcdc) {
	case WM8350_DCDC_2:
		val = wm8350_reg_read(wm8350, WM8350_DCDC2_CONTROL)
			 & ~(WM8350_DC2_MODE_MASK | WM8350_DC2_ILIM_MASK |
			 WM8350_DC2_RMP_MASK | WM8350_DC2_FBSRC_MASK);
		wm8350_reg_write(wm8350, WM8350_DCDC2_CONTROL, val |
			(mode << WM8350_DC2_MODE_SHIFT) |
			(ilim << WM8350_DC2_ILIM_SHIFT) |
			(ramp << WM8350_DC2_RMP_SHIFT) |
			(feedback << WM8350_DC2_FBSRC_SHIFT));
		break;
	case WM8350_DCDC_5:
		val = wm8350_reg_read(wm8350, WM8350_DCDC5_CONTROL)
			 & ~(WM8350_DC5_MODE_MASK | WM8350_DC5_ILIM_MASK |
			 WM8350_DC5_RMP_MASK | WM8350_DC5_FBSRC_MASK);
		wm8350_reg_write(wm8350, WM8350_DCDC5_CONTROL, val |
			(mode << WM8350_DC5_MODE_SHIFT) |
			(ilim << WM8350_DC5_ILIM_SHIFT) |
			(ramp << WM8350_DC5_RMP_SHIFT) |
			(feedback << WM8350_DC5_FBSRC_SHIFT));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_dcdc25_set_mode);

static int wm8350_dcdc_enable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int dcdc = reg->id;
	u16 shift;

	dbg("%s %d --> %s\n", __FUNCTION__, dcdc, enable ? "on" : "off");

	if (dcdc < WM8350_DCDC_1 || dcdc > WM8350_DCDC_6)
		return -EINVAL;

	shift = dcdc - WM8350_DCDC_1;
	wm8350_set_bits(wm8350, WM8350_DCDC_LDO_REQUESTED, 1 << shift);
	return 0;
}

static int wm8350_dcdc_disable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int dcdc = reg->id;
	u16 shift;

	dbg("%s %d --> %s\n", __FUNCTION__, dcdc, enable ? "on" : "off");

	if (dcdc < WM8350_DCDC_1 || dcdc > WM8350_DCDC_6)
		return -EINVAL;

	shift = dcdc - WM8350_DCDC_1;
	wm8350_clear_bits(wm8350, WM8350_DCDC_LDO_REQUESTED, 1 << shift);

	return 0;
}

static int wm8350_ldo_enable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int ldo = reg->id;
	u16 shift;

	dbg("%s %d --> %s\n", __FUNCTION__, ldo, enable ? "on" : "off");

	if (ldo < WM8350_LDO_1 || ldo > WM8350_LDO_4)
		return -EINVAL;

	shift = (ldo - WM8350_LDO_1) + 8;
	wm8350_set_bits(wm8350, WM8350_DCDC_LDO_REQUESTED, 1 << shift);
	return 0;
}

static int wm8350_ldo_disable(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int ldo = reg->id;
	u16 shift;

	dbg("%s %d --> %s\n", __FUNCTION__, ldo, enable ? "on" : "off");

	if (ldo < WM8350_LDO_1 || ldo > WM8350_LDO_4)
		return -EINVAL;

	shift = (ldo - WM8350_LDO_1) + 8;
	wm8350_clear_bits(wm8350, WM8350_DCDC_LDO_REQUESTED, 1 << shift);
	return 0;
}

static int force_continuous_enable(struct wm8350 *wm8350, int dcdc, int enable)
{
	int reg = 0, ret;

	switch (dcdc) {
	case WM8350_DCDC_1:
		reg = WM8350_DCDC1_FORCE_PWM;
		break;
	case WM8350_DCDC_3:
		reg = WM8350_DCDC3_FORCE_PWM;
		break;
	case WM8350_DCDC_4:
		reg = WM8350_DCDC4_FORCE_PWM;
		break;
	case WM8350_DCDC_6:
		reg = WM8350_DCDC6_FORCE_PWM;
		break;
	default:
		return -EINVAL;
	}

	/* LG FIXME - atm I can't seem to set the bits */
	return 0;

	wm8350_reg_unlock(wm8350);
	if (enable)
		ret = wm8350_set_bits(wm8350, reg, WM8350_DCDC1_FORCE_PWM_ENA);
	else
		ret = wm8350_clear_bits(wm8350, reg,
			WM8350_DCDC1_FORCE_PWM_ENA);
	wm8350_reg_lock(wm8350);
	return ret;
}

static int wm8350_dcdc_set_mode(struct regulator *reg,
	unsigned int mode)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int dcdc = reg->id;
	u16 val;

	if (dcdc < WM8350_DCDC_1 || dcdc > WM8350_DCDC_6)
		return -EINVAL;

	if (dcdc == WM8350_DCDC_2 || dcdc == WM8350_DCDC_5)
		return -EINVAL;

	val = 1 << (dcdc - WM8350_DCDC_1);

	switch (mode) {
	case REGULATOR_MODE_FAST:
		/* force continuous mode */
		wm8350_set_bits(wm8350, WM8350_DCDC_ACTIVE_OPTIONS, val);
		wm8350_clear_bits(wm8350, WM8350_DCDC_SLEEP_OPTIONS, val);
		force_continuous_enable(wm8350, dcdc, 1);
		break;
	case REGULATOR_MODE_NORMAL:
		/* active / pulse skipping */
		wm8350_set_bits(wm8350, WM8350_DCDC_ACTIVE_OPTIONS, val);
		wm8350_clear_bits(wm8350, WM8350_DCDC_SLEEP_OPTIONS, val);
		force_continuous_enable(wm8350, dcdc, 0);
		break;
	case REGULATOR_MODE_IDLE:
		/* standby mode */
		force_continuous_enable(wm8350, dcdc, 0);
		wm8350_clear_bits(wm8350, WM8350_DCDC_SLEEP_OPTIONS, val);
		wm8350_clear_bits(wm8350, WM8350_DCDC_ACTIVE_OPTIONS, val);
		break;
	case REGULATOR_MODE_STANDBY:
		/* LDO mode */
		force_continuous_enable(wm8350, dcdc, 0);
		wm8350_set_bits(wm8350, WM8350_DCDC_SLEEP_OPTIONS, val);
		break;
	}

	return 0;
}

static unsigned int wm8350_dcdc_get_mode(struct regulator *reg)
{
	struct wm8350_pmic *pmic = (struct wm8350_pmic *)reg->reg_data;
	struct wm8350 *wm8350 = to_wm8350_from_pmic(pmic);
	int dcdc = reg->id;
	u16 mask, sleep, active, force;
	int mode = REGULATOR_MODE_NORMAL;

	if (dcdc < WM8350_DCDC_1 || dcdc > WM8350_DCDC_6)
		return -EINVAL;

	if (dcdc == WM8350_DCDC_2 || dcdc == WM8350_DCDC_5)
		return -EINVAL;

	mask = 1 << (dcdc - WM8350_DCDC_1);
	active = wm8350_reg_read(wm8350, WM8350_DCDC_ACTIVE_OPTIONS) & mask;
	sleep = wm8350_reg_read(wm8350, WM8350_DCDC_SLEEP_OPTIONS) & mask;
	force = wm8350_reg_read(wm8350, WM8350_DCDC1_FORCE_PWM)
		& WM8350_DCDC1_FORCE_PWM_ENA;
	dbg("mask %x active %x sleep %x force %x", mask, active, sleep, force);

	if (active && !sleep) {
		if (force)
			mode = REGULATOR_MODE_FAST;
		else
			mode = REGULATOR_MODE_NORMAL;
	} else if (!active && !sleep)
		mode = REGULATOR_MODE_IDLE;
	else if (!sleep)
		mode = REGULATOR_MODE_STANDBY;

	return mode;
}

struct wm8350_dcdc_efficiency {
	int uA_load_min;
	int uA_load_max;
	unsigned int mode;
};

static const struct wm8350_dcdc_efficiency dcdc1_6_efficiency[] = {
	{0, 10000, REGULATOR_MODE_STANDBY},   /* 0 - 10mA - LDO */
	{10000, 100000, REGULATOR_MODE_IDLE}, /* 10mA - 100mA - Standby */
	{100000, 1000000, REGULATOR_MODE_NORMAL}, /* > 100mA - Active */
	{-1, -1, REGULATOR_MODE_NORMAL},
};

static const struct wm8350_dcdc_efficiency dcdc3_4_efficiency[] = {
	{0, 10000, REGULATOR_MODE_STANDBY},   /* 0 - 10mA - LDO */
	{10000, 100000, REGULATOR_MODE_IDLE}, /* 10mA - 100mA - Standby */
	{100000, 800000, REGULATOR_MODE_NORMAL}, /* > 100mA - Active */
	{-1, -1, REGULATOR_MODE_NORMAL},
};

static unsigned int get_mode(int uA,
	const struct wm8350_dcdc_efficiency *eff)
{
	int i = 0;

	while (eff[i].uA_load_min != -1) {
		if (uA >= eff[i].uA_load_min && uA <= eff[i].uA_load_max)
			return eff[i].mode;
	}
	return REGULATOR_MODE_NORMAL;
}

/* query the regulator for it's most efficient mode @ uV,uA
 * WM8350 regulator efficiency is pretty similar over
 * different input and output uV.
 */
static unsigned int wm8350_dcdc_get_optimum_mode(struct regulator *reg,
	int input_uV, int output_uV, int output_uA)
{
	int dcdc = reg->id, mode;

	switch (dcdc) {
	case WM8350_DCDC_1:
	case WM8350_DCDC_6:
		mode = get_mode(output_uA, dcdc1_6_efficiency);
		break;
	case WM8350_DCDC_3:
	case WM8350_DCDC_4:
		mode = get_mode(output_uA, dcdc3_4_efficiency);
		break;
	default:
		mode = REGULATOR_MODE_NORMAL;
		break;
	}
	return mode;
}

static struct regulator_ops wm8350_dcdc_ops = {
	.set_voltage	= wm8350_dcdc_set_voltage,
	.get_voltage	= wm8350_dcdc_get_voltage,
	.enable		= wm8350_dcdc_enable,
	.disable	= wm8350_dcdc_disable,
	.get_mode	= wm8350_dcdc_get_mode,
	.set_mode	= wm8350_dcdc_set_mode,
	.get_optimum_mode	= wm8350_dcdc_get_optimum_mode,
};

static struct regulator_ops wm8350_dcdc2_5_ops = {
	.enable		= wm8350_dcdc_enable,
	.disable	= wm8350_dcdc_disable,
};

static struct regulator_ops wm8350_ldo_ops = {
	.set_voltage	= wm8350_ldo_set_voltage,
	.get_voltage	= wm8350_ldo_get_voltage,
	.enable		= wm8350_ldo_enable,
	.disable	= wm8350_ldo_disable,
};

static struct regulator_ops wm8350_isink_ops = {
	.set_current	= wm8350_isink_set_current,
	.get_current	= wm8350_isink_get_current,
	.enable		= wm8350_isink_enable,
	.disable	= wm8350_isink_disable,
};

struct wm8350_regulator {
	struct regulator regulator;
	int irq;
};

static struct wm8350_regulator wm8350_reg[NUM_WM8350_REGULATORS] = {
{
	.regulator = {
		.name	= "DCDC1",
		.id	= WM8350_DCDC_1,
		.ops 	= &wm8350_dcdc_ops,
	},
	.irq	= WM8350_IRQ_UV_DC1,
},
{
	.regulator = {
		.name	= "DCDC2",
		.id	= WM8350_DCDC_2,
		.ops 	= &wm8350_dcdc2_5_ops,
	},
	.irq	= WM8350_IRQ_UV_DC2,
},
{
	.regulator = {
		.name	= "DCDC3",
		.id	= WM8350_DCDC_3,
		.ops 	= &wm8350_dcdc_ops,
	},
	.irq	= WM8350_IRQ_UV_DC3,
},
{
	.regulator = {
		.name	= "DCDC4",
		.id	= WM8350_DCDC_4,
		.ops 	= &wm8350_dcdc_ops,
	},
	.irq	= WM8350_IRQ_UV_DC4,
},
{
	.regulator = {
		.name	= "DCDC5",
		.id	= WM8350_DCDC_5,
		.ops 	= &wm8350_dcdc2_5_ops,
	},
	.irq	= WM8350_IRQ_UV_DC5,
},
{
	.regulator = {
		.name	= "DCDC6",
		.id	= WM8350_DCDC_6,
		.ops 	= &wm8350_dcdc_ops,
	},
	.irq	= WM8350_IRQ_UV_DC6,
},
{
	.regulator = {
		.name	= "LDO1",
		.id	= WM8350_LDO_1,
		.ops 	= &wm8350_ldo_ops,
	},
	.irq	= WM8350_IRQ_UV_LDO1,
},
{
	.regulator = {
		.name	= "LDO2",
		.id	= WM8350_LDO_2,
		.ops 	= &wm8350_ldo_ops,
	},
	.irq	= WM8350_IRQ_UV_LDO2,
},
{
	.regulator = {
		.name	= "LDO3",
		.id	= WM8350_LDO_3,
		.ops 	= &wm8350_ldo_ops,
	},
	.irq	= WM8350_IRQ_UV_LDO3,
},
{
	.regulator = {
		.name	= "LDO4",
		.id	= WM8350_LDO_4,
		.ops 	= &wm8350_ldo_ops,
	},
	.irq	= WM8350_IRQ_UV_LDO4,
},
{
	.regulator = {
		.name	= "ISINKA",
		.id	= WM8350_ISINK_A,
		.ops 	= &wm8350_isink_ops,
	},
	.irq	= WM8350_IRQ_CS1,
},
{
	.regulator = {
		.name	= "ISINKB",
		.id	= WM8350_ISINK_B,
		.ops 	= &wm8350_isink_ops,
	},
	.irq	= WM8350_IRQ_CS2,
},};

static void pmic_uv_handler(struct wm8350 *wm8350, int irq, void *data)
{
	struct regulator *regulator = (struct regulator *)data;

	if (irq == WM8350_IRQ_CS1 || irq == WM8350_IRQ_CS2)
		regulator_notifier_call_chain(regulator,
			REGULATOR_EVENT_REGULATION_OUT, wm8350);
	else
		regulator_notifier_call_chain(regulator,
			REGULATOR_EVENT_UNDER_VOLTAGE, wm8350);
}

static int wm8350_probe(struct device *dev)
{
	struct wm8350_pmic *wm8350_pmic = to_wm8350_pmic_device(dev);
	struct wm8350 *wm8350 = to_wm8350_from_pmic(wm8350_pmic);
	int ret, i;

	printk(KERN_INFO "WM8350 PMIC driver version %s\n",
		WM8350_PMIC_VERSION);

	for (i = 0; i < ARRAY_SIZE(wm8350_reg); i++) {
		ret = regulator_register(&wm8350_reg[i].regulator);
		if (ret < 0) {
			printk(KERN_ERR "%s: failed to register %s err %d\n",
				__func__, wm8350_reg[i].regulator.name, ret);
			goto unwind_reg;
		}
		regulator_set_drvdata(&wm8350_reg[i].regulator, wm8350_pmic);
		ret = wm8350_register_irq(wm8350, wm8350_reg[i].irq,
			pmic_uv_handler, &wm8350_reg[i].regulator);
		if (ret < 0)
			goto unwind_irq;
		wm8350_unmask_irq(wm8350, wm8350_reg[i].irq);
	}
	return 0;
unwind_irq:
	regulator_unregister(&wm8350_reg[i].regulator);
unwind_reg:
	i--;
	for (; i >= 0; i--) {
		wm8350_mask_irq(wm8350, wm8350_reg[i].irq);
		wm8350_free_irq(wm8350, wm8350_reg[i].irq);
		regulator_unregister(&wm8350_reg[i].regulator);
	}
	return ret;
}

static int wm8350_remove(struct device *dev)
{
	struct wm8350_pmic *wm8350_pmic = to_wm8350_pmic_device(dev);
	struct wm8350 *wm8350 = to_wm8350_from_pmic(wm8350_pmic);
	int i;

	for (i = ARRAY_SIZE(wm8350_reg) - 1; i >= 0; i--) {
		wm8350_mask_irq(wm8350, wm8350_reg[i].irq);
		wm8350_free_irq(wm8350, wm8350_reg[i].irq);
		regulator_unregister(&wm8350_reg[i].regulator);
	}

	return 0;
}

struct device_driver wm8350_driver = {
	.name = "wm8350-pmic",
	.bus = &wm8350_bus_type,
	.owner = THIS_MODULE,
	.probe = wm8350_probe,
	.remove = wm8350_remove,
};

int wm8350_pmu_init(void)
{
	return driver_register(&wm8350_driver);
}

void wm8350_pmu_exit(void)
{
	driver_unregister(&wm8350_driver);
}

/* Module information */
MODULE_AUTHOR("Liam Girdwood, liam.girdwood@wolfsonmicro.com,\
www.wolfsonmicro.com");
MODULE_DESCRIPTION("WM8350 PMIC driver");
MODULE_LICENSE("GPL");
