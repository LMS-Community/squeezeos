/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/regulator/regulator-platform.h>
#include <linux/regulator/regulator-drv.h>
#include <asm/ioctl.h>
#include <linux/platform_device.h>
#include <asm/arch/pmic_status.h>
#include <asm/arch/pmic_external.h>

enum {
	MC13892_SW1 = 0,
	MC13892_SW2,
	MC13892_SW3,
	MC13892_SW4,
	MC13892_SWBST,
	MC13892_VIOHI,
	MC13892_VPLL,
	MC13892_VDIG,
	MC13892_VSD,
	MC13892_VUSB2,
	MC13892_VVIDEO,
	MC13892_VAUDIO,
	MC13892_VCAM,
	MC13892_VGEN1,
	MC13892_VGEN2,
	MC13892_VGEN3,
	MC13892_VUSB,
	MC13892_GPO1,
	MC13892_GPO2,
	MC13892_GPO3,
	MC13892_GPO4,
	MC13892_REG_NUM,
} MC13892_regulator;

enum {
	VDIG_1_05V = 0,
	VDIG_1_25V,
	VDIG_1_65V,
	VDIG_1_80V,
} regulator_voltage_vdig;

enum {
	VPLL_1_05V = 0,
	VPLL_1_25V,
	VPLL_1_65V,
	VPLL_1_80V,
} regulator_voltage_vpll;

enum {
	VGEN1_1_2V = 0,
	VGEN1_1_5V,
	VGEN1_1_8V,
	VGEN1_2_775V,
} regulator_voltage_vgen1;

enum {
	VGEN2_1_2V = 0,
	VGEN2_1_5V,
	VGEN2_1_6V,
	VGEN2_1_8V,
	VGEN2_2_7V,
	VGEN2_2_8V,
	VGEN2_2_9V,
	VGEN2_3_0V,
} regulator_voltage_vgen2;

enum {
	VGEN3_1_8V = 0,
	VGEN3_2_9V,
} regulator_voltage_vgen3;

enum {
	VSD_1_8V = 0,
	VSD_2_0V,
	VSD_2_6V,
	VSD_2_7V,
	VSD_2_8V,
	VSD_2_9V,
	VSD_3_0V,
	VSD_3_15V,
} regulator_voltage_vsd;

enum {
	VCAM_1_8V,
	VCAM_2_5V,
	VCAM_2_6V,
	VCAM_2_75V,
} regulator_voltage_vcam;

enum {
	VAUDIO_2_3V,
	VAUDIO_2_5V,
	VAUDIO_2_7V,
	VAUDIO_2_775V,
} regulator_voltage_vaudio;

enum {
	VUSB2_2_4V,
	VUSB2_2_6V,
	VUSB2_2_7V,
	VUSB2_2_775V,
} regulator_voltage_vusb2;

enum {
	VVIDEO_2_7V,
	VVIDEO_2_775V,
	VVIDEO_2_5V,
	VVIDEO_2_6V,
} regulator_voltage_vvideo;

#define VAUDIO_LSH	4
#define VAUDIO_WID	2
#define VAUDIO_EN_LSH	15
#define VAUDIO_EN_WID	1
#define VAUDIO_EN_ENABLE	1
#define VAUDIO_EN_DISABLE	0

#define VUSB2_LSH	11
#define VUSB2_WID	2
#define VUSB2_EN_LSH	18
#define VUSB2_EN_WID	1
#define VUSB2_EN_ENABLE	1
#define VUSB2_EN_DISABLE	0

#define VVIDEO_LSH	2
#define VVIDEO_WID	2
#define VVIDEO_EN_LSH	12
#define VVIDEO_EN_WID	1
#define VVIDEO_EN_ENABLE	1
#define VVIDEO_EN_DISABLE	0

#define SWBST_EN_LSH	20
#define SWBST_EN_WID	1
#define SWBST_EN_ENABLE	1
#define SWBST_EN_DISABLE	0

#define VIOHI_EN_LSH	3
#define VIOHI_EN_WID	1
#define VIOHI_EN_ENABLE	1
#define VIOHI_EN_DISABLE	0

#define VDIG_LSH	4
#define VDIG_WID	2
#define VDIG_EN_LSH	9
#define VDIG_EN_WID	1
#define VDIG_EN_ENABLE	1
#define VDIG_EN_DISABLE	0

#define VPLL_LSH	9
#define VPLL_WID	2
#define VPLL_EN_LSH	15
#define VPLL_EN_WID	1
#define VPLL_EN_ENABLE	1
#define VPLL_EN_DISABLE	0

#define VGEN1_LSH	0
#define VGEN1_WID	2
#define VGEN1_EN_LSH	0
#define VGEN1_EN_WID	1
#define VGEN1_EN_ENABLE	1
#define VGEN1_EN_DISABLE	0

#define VGEN2_LSH	6
#define VGEN2_WID	3
#define VGEN2_EN_LSH	12
#define VGEN2_EN_WID	1
#define VGEN2_EN_ENABLE	1
#define VGEN2_EN_DISABLE	0

#define VGEN3_LSH	14
#define VGEN3_WID	1
#define VGEN3_EN_LSH	0
#define VGEN3_EN_WID	1
#define VGEN3_EN_ENABLE	1
#define VGEN3_EN_DISABLE	0

#define VSD_LSH	6
#define VSD_WID	3
#define VSD_EN_LSH	18
#define VSD_EN_WID	1
#define VSD_EN_ENABLE	1
#define VSD_EN_DISABLE	0

#define VCAM_LSH	16
#define VCAM_WID	2
#define VCAM_EN_LSH	6
#define VCAM_EN_WID	1
#define VCAM_EN_ENABLE	1
#define VCAM_EN_DISABLE	0

#define SW1_LSH		0
#define SW1_WID		5
#define SW1_DVS_LSH	5
#define SW1_DVS_WID	5
#define SW1_STDBY_LSH	10
#define SW1_STDBY_WID	5

#define SW2_LSH		0
#define SW2_WID		5
#define SW2_DVS_LSH	5
#define SW2_DVS_WID	5
#define SW2_STDBY_LSH	10
#define SW2_STDBY_WID	5

#define SW3_LSH		0
#define SW3_WID		5
#define SW3_STDBY_LSH	10
#define SW3_STDBY_WID	5

#define SW4_LSH		0
#define SW4_WID		5
#define SW4_STDBY_LSH	10
#define SW4_STDBY_WID	5

#define VUSB_EN_LSH	3
#define VUSB_EN_WID	1
#define VUSB_EN_ENABLE	1
#define VUSB_EN_DISABLE	0

#define GPO1_EN_LSH	6
#define GPO1_EN_WID	1
#define GPO1_EN_ENABLE	1
#define GPO1_EN_DISABLE	0

#define GPO2_EN_LSH	8
#define GPO2_EN_WID	1
#define GPO2_EN_ENABLE	1
#define GPO2_EN_DISABLE	0

#define GPO3_EN_LSH	10
#define GPO3_EN_WID	1
#define GPO3_EN_ENABLE	1
#define GPO3_EN_DISABLE	0

#define GPO4_EN_LSH	12
#define GPO4_EN_WID	1
#define GPO4_EN_ENABLE	1
#define GPO4_EN_DISABLE	0

static int mc13892_get_sw_hi_bit(int sw)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int reg = 0;

	switch (sw) {
	case MC13892_SW1:
		reg = REG_SW_0;
		break;
	case MC13892_SW2:
		reg = REG_SW_1;
		break;
	case MC13892_SW3:
		reg = REG_SW_2;
		break;
	case MC13892_SW4:
		reg = REG_SW_3;
		break;
	default:
		return -EINVAL;
	}

	CHECK_ERROR(pmic_read_reg(reg, &register_val, PMIC_ALL_BITS));
	return register_val & 0x800000;
}

static int mc13892_get_voltage_value(int hi, int mV)
{
	int voltage;

	if (mV < 600)
		mV = 600;
	if (mV > 1850)
		mV = 1850;

	if (hi == 0) {
		if (mV > 1375)
			mV = 1375;
		voltage = (mV - 600) / 25;
	} else {
		if (mV < 1100)
			mV = 1100;
		voltage = (mV - 1100) / 25;
	}

	return voltage;
}

static int mc13892_get_voltage_mV(int hi, int voltage)
{
	int mV;

	if (hi == 0)
		mV = voltage * 25 + 600;
	else
		mV = voltage * 25 + 1100;

	return mV;
}

static int mc13892_sw_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1 = 0;
	int voltage, sw = reg->id, mV = uV / 1000, hi;

	hi = mc13892_get_sw_hi_bit(sw);
	voltage = mc13892_get_voltage_value(hi, mV);

	switch (sw) {
	case MC13892_SW1:
		register1 = REG_SW_0;
		register_val = BITFVAL(SW1, voltage);
		register_mask = BITFMASK(SW1);
		break;
	case MC13892_SW2:
		register1 = REG_SW_1;
		register_val = BITFVAL(SW2, voltage);
		register_mask = BITFMASK(SW2);
		break;
	case MC13892_SW3:
		register1 = REG_SW_2;
		register_val = BITFVAL(SW3, voltage);
		register_mask = BITFMASK(SW3);
		break;
	case MC13892_SW4:
		register1 = REG_SW_3;
		register_val = BITFVAL(SW4, voltage);
		register_mask = BITFMASK(SW4);
		break;
	default:
		return -EINVAL;
	}

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_sw_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id, hi;

	switch (sw) {
	case MC13892_SW1:
		CHECK_ERROR(pmic_read_reg(REG_SW_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1);
		break;
	case MC13892_SW2:
		CHECK_ERROR(pmic_read_reg(REG_SW_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2);
		break;
	case MC13892_SW3:
		CHECK_ERROR(pmic_read_reg(REG_SW_2,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW3);
		break;
	case MC13892_SW4:
		CHECK_ERROR(pmic_read_reg(REG_SW_3,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW4);
		break;
	default:
		return -EINVAL;
	}

	hi = mc13892_get_sw_hi_bit(sw);
	mV = mc13892_get_voltage_mV(hi, voltage);

	return mV;
}

static int mc13892_sw_stby_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1 = 0;
	int voltage, sw = reg->id, mV = uV / 1000, hi;

	hi = mc13892_get_sw_hi_bit(sw);
	voltage = mc13892_get_voltage_value(hi, mV);

	switch (sw) {
	case MC13892_SW1:
		register1 = REG_SW_0;
		register_val = BITFVAL(SW1_STDBY, voltage);
		register_mask = BITFMASK(SW1_STDBY);
		break;
	case MC13892_SW2:
		register1 = REG_SW_1;
		register_val = BITFVAL(SW2_STDBY, voltage);
		register_mask = BITFMASK(SW2_STDBY);
		break;
	case MC13892_SW3:
		register1 = REG_SW_2;
		register_val = BITFVAL(SW3_STDBY, voltage);
		register_mask = BITFMASK(SW3_STDBY);
		break;
	case MC13892_SW4:
		register1 = REG_SW_3;
		register_val = BITFVAL(SW4_STDBY, voltage);
		register_mask = BITFMASK(SW4_STDBY);
		break;
	default:
		return -EINVAL;
	}

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_sw_stby_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id, hi;

	switch (sw) {
	case MC13892_SW1:
		CHECK_ERROR(pmic_read_reg(REG_SW_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1_STDBY);
		break;
	case MC13892_SW2:
		CHECK_ERROR(pmic_read_reg(REG_SW_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2_STDBY);
		break;
	case MC13892_SW3:
		CHECK_ERROR(pmic_read_reg(REG_SW_2,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW3_STDBY);
		break;
	case MC13892_SW4:
		CHECK_ERROR(pmic_read_reg(REG_SW_3,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW4_STDBY);
		break;
	default:
		return -EINVAL;
	}

	hi = mc13892_get_sw_hi_bit(sw);
	mV = mc13892_get_voltage_mV(hi, voltage);

	return mV;
}

static int mc13892_sw_dvs_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1 = 0;
	int voltage, sw = reg->id, mV = uV / 1000, hi;

	hi = mc13892_get_sw_hi_bit(sw);
	voltage = mc13892_get_voltage_value(hi, mV);

	switch (sw) {
	case MC13892_SW1:
		register1 = REG_SW_0;
		register_val = BITFVAL(SW1_DVS, voltage);
		register_mask = BITFMASK(SW1_DVS);
		break;
	case MC13892_SW2:
		register1 = REG_SW_1;
		register_val = BITFVAL(SW2_DVS, voltage);
		register_mask = BITFMASK(SW2_DVS);
		break;
	default:
		return -EINVAL;
	}

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_sw_dvs_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id, hi;

	switch (sw) {
	case MC13892_SW1:
		CHECK_ERROR(pmic_read_reg(REG_SW_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1_DVS);
		break;
	case MC13892_SW2:
		CHECK_ERROR(pmic_read_reg(REG_SW_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2_DVS);
		break;
	default:
		return -EINVAL;
	}

	hi = mc13892_get_sw_hi_bit(sw);
	mV = mc13892_get_voltage_mV(hi, voltage);

	return mV;
}

static int mc13892_swbst_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(SWBST_EN, SWBST_EN_ENABLE);
	register_mask = BITFMASK(SWBST_EN);
	register1 = REG_SW_5;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_swbst_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(SWBST_EN, SWBST_EN_DISABLE);
	register_mask = BITFMASK(SWBST_EN);
	register1 = REG_SW_5;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_viohi_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOHI_EN, VIOHI_EN_ENABLE);
	register_mask = BITFMASK(VIOHI_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_viohi_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOHI_EN, VIOHI_EN_DISABLE);
	register_mask = BITFMASK(VIOHI_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vusb_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VUSB_EN, VUSB_EN_ENABLE);
	register_mask = BITFMASK(VUSB_EN);
	register1 = REG_USB1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vusb_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VUSB_EN, VUSB_EN_DISABLE);
	register_mask = BITFMASK(VUSB_EN);
	register1 = REG_USB1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vdig_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1050) && (mV < 1250))
		voltage = VDIG_1_05V;
	else if ((mV >= 1250) && (mV < 1650))
		voltage = VDIG_1_25V;
	else if ((mV >= 1650) && (mV < 1800))
		voltage = VDIG_1_65V;
	else
		voltage = VDIG_1_80V;

	register_val = BITFVAL(VDIG, voltage);
	register_mask = BITFMASK(VDIG);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vdig_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VDIG);

	switch (voltage) {
	case VDIG_1_05V:
		mV = 1050;
		break;
	case VDIG_1_25V:
		mV = 1250;
		break;
	case VDIG_1_65V:
		mV = 1650;
		break;
	case VDIG_1_80V:
		mV = 1800;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vdig_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VDIG_EN, VDIG_EN_ENABLE);
	register_mask = BITFMASK(VDIG_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vdig_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VDIG_EN, VDIG_EN_DISABLE);
	register_mask = BITFMASK(VDIG_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vpll_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1050) && (mV < 1250))
		voltage = VPLL_1_05V;
	else if ((mV >= 1250) && (mV < 1650))
		voltage = VPLL_1_25V;
	else if ((mV >= 1650) && (mV < 1800))
		voltage = VPLL_1_65V;
	else
		voltage = VPLL_1_80V;

	register_val = BITFVAL(VPLL, voltage);
	register_mask = BITFMASK(VPLL);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vpll_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VPLL);

	switch (voltage) {
	case VPLL_1_05V:
		mV = 1050;
		break;
	case VPLL_1_25V:
		mV = 1250;
		break;
	case VPLL_1_65V:
		mV = 1650;
		break;
	case VPLL_1_80V:
		mV = 1800;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vpll_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VPLL_EN, VPLL_EN_ENABLE);
	register_mask = BITFMASK(VPLL_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vpll_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VPLL_EN, VPLL_EN_DISABLE);
	register_mask = BITFMASK(VPLL_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vaudio_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 2300) && (mV < 2500))
		voltage = VAUDIO_2_3V;
	else if ((mV >= 2500) && (mV < 2700))
		voltage = VAUDIO_2_5V;
	else if ((mV >= 2700) && (mV < 2775))
		voltage = VAUDIO_2_7V;
	else
		voltage = VAUDIO_2_775V;

	register_val = BITFVAL(VAUDIO, voltage);
	register_mask = BITFMASK(VAUDIO);
	register1 = REG_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vaudio_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_1, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VAUDIO);

	switch (voltage) {
	case VAUDIO_2_3V:
		mV = 2300;
		break;
	case VAUDIO_2_5V:
		mV = 2500;
		break;
	case VAUDIO_2_7V:
		mV = 2700;
		break;
	case VAUDIO_2_775V:
		mV = 2775;
		break;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vaudio_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VAUDIO_EN, VAUDIO_EN_ENABLE);
	register_mask = BITFMASK(VAUDIO_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vaudio_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VAUDIO_EN, VAUDIO_EN_DISABLE);
	register_mask = BITFMASK(VAUDIO_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vusb2_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 2400) && (mV < 2600))
		voltage = VUSB2_2_4V;
	else if ((mV >= 2600) && (mV < 2700))
		voltage = VUSB2_2_6V;
	else if ((mV >= 2700) && (mV < 2775))
		voltage = VUSB2_2_7V;
	else
		voltage = VUSB2_2_775V;

	register_val = BITFVAL(VUSB2, voltage);
	register_mask = BITFMASK(VUSB2);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vusb2_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VUSB2);

	switch (voltage) {
	case VUSB2_2_4V:
		mV = 2400;
		break;
	case VUSB2_2_6V:
		mV = 2600;
		break;
	case VUSB2_2_7V:
		mV = 2700;
		break;
	case VUSB2_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vusb2_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VUSB2_EN, VUSB2_EN_ENABLE);
	register_mask = BITFMASK(VUSB2_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vusb2_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VUSB2_EN, VUSB2_EN_DISABLE);
	register_mask = BITFMASK(VUSB2_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vvideo_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 2500) && (mV < 2600))
		voltage = VVIDEO_2_5V;
	else if ((mV >= 2600) && (mV < 2700))
		voltage = VVIDEO_2_6V;
	else if ((mV >= 2700) && (mV < 2775))
		voltage = VVIDEO_2_7V;
	else
		voltage = VVIDEO_2_775V;

	register_val = BITFVAL(VVIDEO, voltage);
	register_mask = BITFMASK(VVIDEO);
	register1 = REG_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vvideo_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_1, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VVIDEO);

	switch (voltage) {
	case VVIDEO_2_5V:
		mV = 2500;
		break;
	case VVIDEO_2_6V:
		mV = 2600;
		break;
	case VVIDEO_2_7V:
		mV = 2700;
		break;
	case VVIDEO_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vvideo_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VVIDEO_EN, VVIDEO_EN_ENABLE);
	register_mask = BITFMASK(VVIDEO_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vvideo_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VVIDEO_EN, VVIDEO_EN_DISABLE);
	register_mask = BITFMASK(VVIDEO_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vsd_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1800) && (mV < 2000))
		voltage = VSD_1_8V;
	else if ((mV >= 2000) && (mV < 2600))
		voltage = VSD_2_0V;
	else if ((mV >= 2600) && (mV < 2700))
		voltage = VSD_2_6V;
	else if ((mV >= 2700) && (mV < 2800))
		voltage = VSD_2_7V;
	else if ((mV >= 2800) && (mV < 2900))
		voltage = VSD_2_8V;
	else if ((mV >= 2900) && (mV < 3000))
		voltage = VSD_2_9V;
	else if ((mV >= 3000) && (mV < 3150))
		voltage = VSD_3_0V;
	else
		voltage = VSD_3_15V;

	register_val = BITFVAL(VSD, voltage);
	register_mask = BITFMASK(VSD);
	register1 = REG_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vsd_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_1, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VSD);

	switch (voltage) {
	case VSD_1_8V:
		mV = 1800;
		break;
	case VSD_2_0V:
		mV = 2000;
		break;
	case VSD_2_6V:
		mV = 2600;
		break;
	case VSD_2_7V:
		mV = 2700;
		break;
	case VSD_2_8V:
		mV = 2800;
		break;
	case VSD_2_9V:
		mV = 2900;
		break;
	case VSD_3_0V:
		mV = 3000;
		break;
	case VSD_3_15V:
		mV = 3150;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vsd_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VSD_EN, VSD_EN_ENABLE);
	register_mask = BITFMASK(VSD_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vsd_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VSD_EN, VSD_EN_DISABLE);
	register_mask = BITFMASK(VSD_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vcam_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1800) && (mV < 2500))
		voltage = VCAM_1_8V;
	else if ((mV >= 2500) && (mV < 2600))
		voltage = VCAM_2_5V;
	else if ((mV >= 2600) && (mV < 2750))
		voltage = VCAM_2_6V;
	else
		voltage = VCAM_2_75V;

	register_val = BITFVAL(VCAM, voltage);
	register_mask = BITFMASK(VCAM);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vcam_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VCAM);

	switch (voltage) {
	case VCAM_1_8V:
		mV = 1800;
		break;
	case VCAM_2_5V:
		mV = 2500;
		break;
	case VCAM_2_6V:
		mV = 2600;
		break;
	case VCAM_2_75V:
		mV = 2750;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vcam_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VCAM_EN, VCAM_EN_ENABLE);
	register_mask = BITFMASK(VCAM_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vcam_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VCAM_EN, VCAM_EN_DISABLE);
	register_mask = BITFMASK(VCAM_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen1_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1200) && (mV < 1500))
		voltage = VGEN1_1_2V;
	else if ((mV >= 1500) && (mV < 1800))
		voltage = VGEN1_1_5V;
	else if ((mV >= 1800) && (mV < 2775))
		voltage = VGEN1_1_8V;
	else
		voltage = VGEN1_2_775V;

	register_val = BITFVAL(VGEN1, voltage);
	register_mask = BITFMASK(VGEN1);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen1_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VGEN1);

	switch (voltage) {
	case VGEN1_1_2V:
		mV = 1200;
		break;
	case VGEN1_1_5V:
		mV = 1500;
		break;
	case VGEN1_1_8V:
		mV = 1800;
		break;
	case VGEN1_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vgen1_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN1_EN, VGEN1_EN_ENABLE);
	register_mask = BITFMASK(VGEN1_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen1_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN1_EN, VGEN1_EN_DISABLE);
	register_mask = BITFMASK(VGEN1_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen2_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1200) && (mV < 1500))
		voltage = VGEN2_1_2V;
	else if ((mV >= 1500) && (mV < 1600))
		voltage = VGEN2_1_5V;
	else if ((mV >= 1600) && (mV < 1800))
		voltage = VGEN2_1_6V;
	else if ((mV >= 1800) && (mV < 2700))
		voltage = VGEN2_1_8V;
	else if ((mV >= 2700) && (mV < 2800))
		voltage = VGEN2_2_7V;
	else if ((mV >= 2800) && (mV < 2900))
		voltage = VGEN2_2_8V;
	else if ((mV >= 2900) && (mV < 3000))
		voltage = VGEN2_2_9V;
	else
		voltage = VGEN2_3_0V;

	register_val = BITFVAL(VGEN2, voltage);
	register_mask = BITFMASK(VGEN2);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen2_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VGEN2);

	switch (voltage) {
	case VGEN2_1_2V:
		mV = 1200;
		break;
	case VGEN2_1_5V:
		mV = 1500;
		break;
	case VGEN2_1_6V:
		mV = 1600;
		break;
	case VGEN2_1_8V:
		mV = 1800;
		break;
	case VGEN2_2_7V:
		mV = 2700;
		break;
	case VGEN2_2_8V:
		mV = 2800;
		break;
	case VGEN2_2_9V:
		mV = 2900;
		break;
	case VGEN2_3_0V:
		mV = 3000;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vgen2_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN2_EN, VGEN2_EN_ENABLE);
	register_mask = BITFMASK(VGEN2_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen2_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN2_EN, VGEN2_EN_DISABLE);
	register_mask = BITFMASK(VGEN2_EN);
	register1 = REG_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen3_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1800) && (mV < 2900))
		voltage = VGEN3_1_8V;
	else
		voltage = VGEN3_2_9V;

	register_val = BITFVAL(VGEN3, voltage);
	register_mask = BITFMASK(VGEN3);
	register1 = REG_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen3_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SETTING_0, &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VGEN3);

	switch (voltage) {
	case VGEN3_1_8V:
		mV = 1800;
		break;
	case VGEN3_2_9V:
		mV = 2900;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13892_vgen3_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN3_EN, VGEN3_EN_ENABLE);
	register_mask = BITFMASK(VGEN3_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_vgen3_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN3_EN, VGEN3_EN_DISABLE);
	register_mask = BITFMASK(VGEN3_EN);
	register1 = REG_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_gpo_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int gpo = reg->id;

	switch (gpo) {
	case MC13892_GPO1:
		register_val = BITFVAL(GPO1_EN, GPO1_EN_ENABLE);
		register_mask = BITFMASK(GPO1_EN);
		break;
	case MC13892_GPO2:
		register_val = BITFVAL(GPO2_EN, GPO2_EN_ENABLE);
		register_mask = BITFMASK(GPO2_EN);
		break;
	case MC13892_GPO3:
		register_val = BITFVAL(GPO3_EN, GPO3_EN_ENABLE);
		register_mask = BITFMASK(GPO3_EN);
		break;
	case MC13892_GPO4:
		register_val = BITFVAL(GPO4_EN, GPO4_EN_ENABLE);
		register_mask = BITFMASK(GPO4_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_POWER_MISC;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13892_gpo_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int gpo = reg->id;

	switch (gpo) {
	case MC13892_GPO1:
		register_val = BITFVAL(GPO1_EN, GPO1_EN_DISABLE);
		register_mask = BITFMASK(GPO1_EN);
		break;
	case MC13892_GPO2:
		register_val = BITFVAL(GPO2_EN, GPO2_EN_DISABLE);
		register_mask = BITFMASK(GPO2_EN);
		break;
	case MC13892_GPO3:
		register_val = BITFVAL(GPO3_EN, GPO3_EN_DISABLE);
		register_mask = BITFMASK(GPO3_EN);
		break;
	case MC13892_GPO4:
		register_val = BITFVAL(GPO4_EN, GPO4_EN_DISABLE);
		register_mask = BITFMASK(GPO4_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_POWER_MISC;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static struct regulator_ops mc13892_sw_ops = {
	.set_voltage = mc13892_sw_set_voltage,
	.get_voltage = mc13892_sw_get_voltage,
};

struct regulation_constraints sw_constraints = {
	.min_uV = mV_to_uV(1100),
	.max_uV = mV_to_uV(1850),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_sw_stby_ops = {
	.set_voltage = mc13892_sw_stby_set_voltage,
	.get_voltage = mc13892_sw_stby_get_voltage,
};

struct regulation_constraints sw_stby_constraints = {
	.min_uV = mV_to_uV(600),
	.max_uV = mV_to_uV(1375),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_sw_dvs_ops = {
	.set_voltage = mc13892_sw_dvs_set_voltage,
	.get_voltage = mc13892_sw_dvs_get_voltage,
};

static struct regulator_ops mc13892_swbst_ops = {
	.enable = mc13892_swbst_enable,
	.disable = mc13892_swbst_disable,
};

static struct regulator_ops mc13892_viohi_ops = {
	.enable = mc13892_viohi_enable,
	.disable = mc13892_viohi_disable,
};

static struct regulator_ops mc13892_vusb_ops = {
	.enable = mc13892_vusb_enable,
	.disable = mc13892_vusb_disable,
};

static struct regulator_ops mc13892_vdig_ops = {
	.set_voltage = mc13892_vdig_set_voltage,
	.get_voltage = mc13892_vdig_get_voltage,
	.enable = mc13892_vdig_enable,
	.disable = mc13892_vdig_disable,
};

struct regulation_constraints vdig_regulation_constraints = {
	.min_uV = mV_to_uV(1050),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vpll_ops = {
	.set_voltage = mc13892_vpll_set_voltage,
	.get_voltage = mc13892_vpll_get_voltage,
	.enable = mc13892_vpll_enable,
	.disable = mc13892_vpll_disable,
};

struct regulation_constraints vpll_regulation_constraints = {
	.min_uV = mV_to_uV(1050),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vusb2_ops = {
	.set_voltage = mc13892_vusb2_set_voltage,
	.get_voltage = mc13892_vusb2_get_voltage,
	.enable = mc13892_vusb2_enable,
	.disable = mc13892_vusb2_disable,
};

struct regulation_constraints vusb2_regulation_constraints = {
	.min_uV = mV_to_uV(2400),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vvideo_ops = {
	.set_voltage = mc13892_vvideo_set_voltage,
	.get_voltage = mc13892_vvideo_get_voltage,
	.enable = mc13892_vvideo_enable,
	.disable = mc13892_vvideo_disable,
};

struct regulation_constraints vvideo_regulation_constraints = {
	.min_uV = mV_to_uV(2500),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vaudio_ops = {
	.set_voltage = mc13892_vaudio_set_voltage,
	.get_voltage = mc13892_vaudio_get_voltage,
	.enable = mc13892_vaudio_enable,
	.disable = mc13892_vaudio_disable,
};

struct regulation_constraints vaudio_regulation_constraints = {
	.min_uV = mV_to_uV(2300),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vsd_ops = {
	.set_voltage = mc13892_vsd_set_voltage,
	.get_voltage = mc13892_vsd_get_voltage,
	.enable = mc13892_vsd_enable,
	.disable = mc13892_vsd_disable,
};

struct regulation_constraints vsd_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(3150),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vcam_ops = {
	.set_voltage = mc13892_vcam_set_voltage,
	.get_voltage = mc13892_vcam_get_voltage,
	.enable = mc13892_vcam_enable,
	.disable = mc13892_vcam_disable,
};

struct regulation_constraints vcam_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(2750),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vgen1_ops = {
	.set_voltage = mc13892_vgen1_set_voltage,
	.get_voltage = mc13892_vgen1_get_voltage,
	.enable = mc13892_vgen1_enable,
	.disable = mc13892_vgen1_disable,
};

struct regulation_constraints vgen1_regulation_constraints = {
	.min_uV = mV_to_uV(1200),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vgen2_ops = {
	.set_voltage = mc13892_vgen2_set_voltage,
	.get_voltage = mc13892_vgen2_get_voltage,
	.enable = mc13892_vgen2_enable,
	.disable = mc13892_vgen2_disable,
};

struct regulation_constraints vgen2_regulation_constraints = {
	.min_uV = mV_to_uV(1200),
	.max_uV = mV_to_uV(3000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_vgen3_ops = {
	.set_voltage = mc13892_vgen3_set_voltage,
	.get_voltage = mc13892_vgen3_get_voltage,
	.enable = mc13892_vgen3_enable,
	.disable = mc13892_vgen3_disable,
};

struct regulation_constraints vgen3_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(2900),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator_ops mc13892_gpo_ops = {
	.enable = mc13892_gpo_enable,
	.disable = mc13892_gpo_disable,
};

struct mc13892_regulator {
	struct regulator regulator;
};

static struct mc13892_regulator reg_mc13892[] = {
	{
	 .regulator = {
		       .name = "SW1",
		       .id = MC13892_SW1,
		       .ops = &mc13892_sw_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2",
		       .id = MC13892_SW2,
		       .ops = &mc13892_sw_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW3",
		       .id = MC13892_SW3,
		       .ops = &mc13892_sw_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW4",
		       .id = MC13892_SW4,
		       .ops = &mc13892_sw_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1_STBY",
		       .id = MC13892_SW1,
		       .ops = &mc13892_sw_stby_ops,
		       .constraints = &sw_stby_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2_STBY",
		       .id = MC13892_SW2,
		       .ops = &mc13892_sw_stby_ops,
		       .constraints = &sw_stby_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW3_STBY",
		       .id = MC13892_SW3,
		       .ops = &mc13892_sw_stby_ops,
		       .constraints = &sw_stby_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW4_STBY",
		       .id = MC13892_SW4,
		       .ops = &mc13892_sw_stby_ops,
		       .constraints = &sw_stby_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1_DVS",
		       .id = MC13892_SW1,
		       .ops = &mc13892_sw_dvs_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2_DVS",
		       .id = MC13892_SW2,
		       .ops = &mc13892_sw_dvs_ops,
		       .constraints = &sw_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SWBST",
		       .id = MC13892_SWBST,
		       .ops = &mc13892_swbst_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "VIOHI",
		       .id = MC13892_VIOHI,
		       .ops = &mc13892_viohi_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "VPLL",
		       .id = MC13892_VPLL,
		       .ops = &mc13892_vpll_ops,
		       .constraints = &vpll_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VDIG",
		       .id = MC13892_VDIG,
		       .ops = &mc13892_vdig_ops,
		       .constraints = &vdig_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VSD",
		       .id = MC13892_VSD,
		       .ops = &mc13892_vsd_ops,
		       .constraints = &vsd_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VUSB2",
		       .id = MC13892_VUSB2,
		       .ops = &mc13892_vusb2_ops,
		       .constraints = &vusb2_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VVIDEO",
		       .id = MC13892_VVIDEO,
		       .ops = &mc13892_vvideo_ops,
		       .constraints = &vvideo_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VAUDIO",
		       .id = MC13892_VAUDIO,
		       .ops = &mc13892_vaudio_ops,
		       .constraints = &vaudio_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VCAM",
		       .id = MC13892_VCAM,
		       .ops = &mc13892_vcam_ops,
		       .constraints = &vcam_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VGEN1",
		       .id = MC13892_VGEN1,
		       .ops = &mc13892_vgen1_ops,
		       .constraints = &vgen1_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VGEN2",
		       .id = MC13892_VGEN2,
		       .ops = &mc13892_vgen2_ops,
		       .constraints = &vgen2_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VGEN3",
		       .id = MC13892_VGEN3,
		       .ops = &mc13892_vgen3_ops,
		       .constraints = &vgen3_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "USB",
		       .id = MC13892_VUSB,
		       .ops = &mc13892_vusb_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO1",
		       .id = MC13892_GPO1,
		       .ops = &mc13892_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO2",
		       .id = MC13892_GPO2,
		       .ops = &mc13892_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO3",
		       .id = MC13892_GPO3,
		       .ops = &mc13892_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO4",
		       .id = MC13892_GPO4,
		       .ops = &mc13892_gpo_ops,
		       },
	 },
};

int reg_mc13892_probe(void)
{
	int ret11 = 0;
	int i = 0;
	struct regulator reg;

	for (i = 0; i < ARRAY_SIZE(reg_mc13892); i++) {
		ret11 = regulator_register(&reg_mc13892[i].regulator);
		regulator_set_platform_constraints(reg_mc13892[i].regulator.
						   name,
						   reg_mc13892[i].regulator.
						   constraints);

		reg.id = reg_mc13892[i].regulator.id;
		if (reg_mc13892[i].regulator.ops->enable == NULL) {
			reg_mc13892[i].regulator.use_count = 1;
		} else {
			/*default set all regulator on */
			reg_mc13892[i].regulator.use_count = 1;
			reg_mc13892[i].regulator.ops->enable(&reg);
		}
		if (ret11 < 0) {
			printk(KERN_ERR "%s: failed to register %s err %d\n",
			       __func__, reg_mc13892[i].regulator.name, ret11);
			i--;
			for (; i >= 0; i--)
				regulator_unregister(&reg_mc13892[i].regulator);

			return ret11;
		}
	}

	printk(KERN_INFO "MC13892 regulator successfully probed\n");

	return 0;
}

EXPORT_SYMBOL(reg_mc13892_probe);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MC13892 Regulator driver");
MODULE_LICENSE("GPL");
