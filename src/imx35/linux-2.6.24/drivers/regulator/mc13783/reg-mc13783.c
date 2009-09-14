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

/*!
 * brief PMIC regulators.
 */

enum {
	MC13783_SW1A = 0,	/*!< SW1A or SW1 */
	MC13783_SW1B,		/*!< SW1B */
	MC13783_SW2A,		/*!< SW2A or SW2 */
	MC13783_SW2B,		/*!< SW2B */
	MC13783_SW3,		/*!< SW3 */
	MC13783_VAUDIO,		/*!< VAUDIO */
	MC13783_VIOHI,		/*!< VIOHI */
	MC13783_VIOLO,		/*!< VIOLO */
	MC13783_VDIG,		/*!< VDIG */
	MC13783_VGEN,		/*!< VGEN */
	MC13783_VRFDIG,		/*!< VRFDIG */
	MC13783_VRFREF,		/*!< VRFREF */
	MC13783_VRFCP,		/*!< VRFCP */
	MC13783_VSIM,		/*!< VSIM */
	MC13783_VESIM,		/*!< VESIM */
	MC13783_VCAM,		/*!< VCAM */
	MC13783_VRFBG,		/*!< VRFBG */
	MC13783_VVIB,		/*!< VVIB */
	MC13783_VRF1,		/*!< VRF1 */
	MC13783_VRF2,		/*!< VRF2 */
	MC13783_VMMC1,		/*!< VMMC1 or VMMC */
	MC13783_VMMC2,		/*!< VMMC2 */
	MC13783_GPO1,		/*!< GPIO1 */
	MC13783_GPO2,		/*!< GPO2 */
	MC13783_GPO3,		/*!< GPO3 */
	MC13783_GPO4,		/*!< GPO4 */
} MC13783_regulator;

/*!
 * @enum regulator_voltage_sw
 * @brief PMIC regulator SW output voltage.
 */
enum {
	SW_0_9V = 0,		/*!< 0.900 V */
	SW_0_925V,		/*!< 0.925 V */
	SW_0_95V,		/*!< 0.950 V */
	SW_0_975V,		/*!< 0.975 V */
	SW_1V,			/*!< 1.000 V */
	SW_1_025V,		/*!< 1.025 V */
	SW_1_05V,		/*!< 1.050 V */
	SW_1_075V,		/*!< 1.075 V */
	SW_1_1V,		/*!< 1.100 V */
	SW_1_125V,		/*!< 1.125 V */
	SW_1_15V,		/*!< 1.150 V */
	SW_1_175V,		/*!< 1.175 V */
	SW_1_2V,		/*!< 1.200 V */
	SW_1_225V,		/*!< 1.225 V */
	SW_1_25V,		/*!< 1.250 V */
	SW_1_275V,		/*!< 1.275 V */
	SW_1_3V,		/*!< 1.300 V */
	SW_1_325V,		/*!< 1.325 V */
	SW_1_35V,		/*!< 1.350 V */
	SW_1_375V,		/*!< 1.375 V */
	SW_1_4V,		/*!< 1.400 V */
	SW_1_425V,		/*!< 1.425 V */
	SW_1_45V,		/*!< 1.450 V */
	SW_1_475V,		/*!< 1.475 V */
	SW_1_5V,		/*!< 1.500 V */
	SW_1_525V,		/*!< 1.525 V */
	SW_1_55V,		/*!< 1.550 V */
	SW_1_575V,		/*!< 1.575 V */
	SW_1_6V,		/*!< 1.600 V */
	SW_1_625V,		/*!< 1.625 V */
	SW_1_65V,		/*!< 1.650 V */
	SW_1_675V,		/*!< 1.675 V */
	SW_1_7V,		/*!< 1.700 V */
	SW_1_8V = 36,		/*!< 1.800 V */
	SW_1_85V = 40,		/*!< 1.850 V */
	SW_2V = 44,		/*!< 2_000 V */
	SW_2_1V = 48,		/*!< 2_100 V */
	SW_2_2V = 52,		/*!< 2_200 V */
} regulator_voltage_sw;

/*!
 * @enum regulator_voltage_violo
 * @brief PMIC regulator VIOLO output voltage.
 */
enum {
	VIOLO_1_2V = 0,		/*!< 1.2 V */
	VIOLO_1_3V,		/*!< 1.3 V */
	VIOLO_1_5V,		/*!< 1.5 V */
	VIOLO_1_8V,		/*!< 1.8 V */
} regulator_voltage_violo;

/*!
 * @enum regulator_voltage_vdig
 * @brief PMIC regulator VDIG output voltage.
 */
enum {
	VDIG_1_2V = 0,		/*!< 1.2 V */
	VDIG_1_3V,		/*!< 1.3 V */
	VDIG_1_5V,		/*!< 1.5 V */
	VDIG_1_8V,		/*!< 1.8 V */
} regulator_voltage_vdig;

/*!
 * @enum regulator_voltage_vgen
 * @brief PMIC regulator VGEN output voltage.
 */
enum {
	VGEN_1_2V = 0,		/*!< 1.2 V */
	VGEN_1_3V,		/*!< 1.3 V */
	VGEN_1_5V,		/*!< 1.5 V */
	VGEN_1_8V,		/*!< 1.8 V */
	VGEN_1_1V,		/*!< 1.1 V */
	VGEN_2V,		/*!< 2 V */
	VGEN_2_775V,		/*!< 2.775 V */
	VGEN_2_4V,		/*!< 2.4 V */
} regulator_voltage_vgen;

/*!
 * @enum regulator_voltage_vrfdig
 * @brief PMIC regulator VRFDIG output voltage.
 */
enum {
	VRFDIG_1_2V = 0,	/*!< 1.2 V */
	VRFDIG_1_5V,		/*!< 1.5 V */
	VRFDIG_1_8V,		/*!< 1.8 V */
	VRFDIG_1_875V,		/*!< 1.875 V */
} regulator_voltage_vrfdig;

/*!
 * @enum regulator_voltage_vrfref
 * @brief PMIC regulator VRFREF output voltage.
 */
enum {
	VRFREF_2_475V = 0,	/*!< 2.475 V */
	VRFREF_2_6V,		/*!< 2.600 V */
	VRFREF_2_7V,		/*!< 2.700 V */
	VRFREF_2_775V,		/*!< 2.775 V */
} regulator_voltage_vrfref;

/*!
 * @enum regulator_voltage_vrfcp
 * @brief PMIC regulator VRFCP output voltage.
 */
enum {
	VRFCP_2_7V = 0,		/*!< 2.700 V */
	VRFCP_2_775V,		/*!< 2.775 V */
} regulator_voltage_vrfcp;

/*!
 * @enum regulator_voltage_vsim
 * @brief PMIC linear regulator VSIM output voltage.
 */
enum {
	VSIM_1_8V = 0,		/*!< 1.8 V */
	VSIM_2_9V,		/*!< 2.90 V */
	VSIM_3V = 1,		/*!< 3 V */
} regulator_voltage_vsim;

/*!
 * @enum regulator_voltage_vesim
 * @brief PMIC regulator VESIM output voltage.
 */
enum {
	VESIM_1_8V = 0,		/*!< 1.80 V */
	VESIM_2_9V,		/*!< 2.90 V */
} regulator_voltage_vesim;

/*!
 * @enum regulator_voltage_vcam
 * @brief PMIC regulator VCAM output voltage.
 */
enum {
	VCAM_1_5V = 0,		/*!< 1.50 V */
	VCAM_1_8V,		/*!< 1.80 V */
	VCAM_2_5V,		/*!< 2.50 V */
	VCAM_2_55V,		/*!< 2.55 V */
	VCAM_2_6V,		/*!< 2.60 V */
	VCAM_2_75V,		/*!< 2.75 V */
	VCAM_2_8V,		/*!< 2.80 V */
	VCAM_3V,		/*!< 3.00 V */
} regulator_voltage_vcam;

/*!
 * @enum regulator_voltage_vvib
 * @brief PMIC linear regulator V_VIB output voltage.
 */
enum {
	VVIB_1_3V = 0,		/*!< 1.30 V */
	VVIB_1_8V,		/*!< 1.80 V */
	VVIB_2V,		/*!< 2 V */
	VVIB_3V,		/*!< 3 V */
} regulator_voltage_vvib;

/*!
 * @enum regulator_voltage_vmmc
 * @brief MC13783 PMIC regulator VMMC output voltage.
 */
enum {
	VMMC_1_6V = 0,		/*!< 1.60 V */
	VMMC_1_8V,		/*!< 1.80 V */
	VMMC_2V,		/*!< 2.00 V */
	VMMC_2_6V,		/*!< 2.60 V */
	VMMC_2_7V,		/*!< 2.70 V */
	VMMC_2_8V,		/*!< 2.80 V */
	VMMC_2_9V,		/*!< 2.90 V */
	VMMC_3V,		/*!< 3.00 V */
} regulator_voltage_vmmc;

/*!
 * @enum regulator_voltage_vrf
 * @brief PMIC regulator VRF output voltage.
 */
enum {
	VRF_1_5V = 0,		/*!< 1.500 V */
	VRF_1_875V,		/*!< 1.875 V */
	VRF_2_7V,		/*!< 2.700 V */
	VRF_2_775V,		/*!< 2.775 V */
} regulator_voltage_vrf;

/*!
 * @enum regulator_voltage_sw3
 * @brief PMIC Switch mode regulator SW3 output voltages.
 */
enum {
	SW3_5V = 0,		/*!< 5.0 V */
	SW3_5_5V = 3,		/*!< 5.5 V */
} regulator_voltage_sw3;

/*!
     * The \b TPmicDVSTransitionSpeed enum defines the rate with which the
     * voltage transition occurs.
     */
enum {
	ESysDependent,
	E25mVEach4us,
	E25mVEach8us,
	E25mvEach16us
} DVS_transition_speed;

/*
 * Reg Regulator Mode 0
 */
#define VAUDIO_EN_LSH	0
#define VAUDIO_EN_WID	1
#define VAUDIO_EN_ENABLE	1
#define VAUDIO_EN_DISABLE	0
#define VIOHI_EN_LSH	3
#define VIOHI_EN_WID	1
#define VIOHI_EN_ENABLE	1
#define VIOHI_EN_DISABLE	0
#define VIOLO_EN_LSH	6
#define VIOLO_EN_WID	1
#define VIOLO_EN_ENABLE	1
#define VIOLO_EN_DISABLE	0
#define VDIG_EN_LSH	9
#define VDIG_EN_WID	1
#define VDIG_EN_ENABLE	1
#define VDIG_EN_DISABLE	0
#define VGEN_EN_LSH	12
#define VGEN_EN_WID	1
#define VGEN_EN_ENABLE	1
#define VGEN_EN_DISABLE	0
#define VRFDIG_EN_LSH	15
#define VRFDIG_EN_WID	1
#define VRFDIG_EN_ENABLE	1
#define VRFDIG_EN_DISABLE	0
#define VRFREF_EN_LSH	18
#define VRFREF_EN_WID	1
#define VRFREF_EN_ENABLE	1
#define VRFREF_EN_DISABLE	0
#define VRFCP_EN_LSH	21
#define VRFCP_EN_WID	1
#define VRFCP_EN_ENABLE	1
#define VRFCP_EN_DISABLE	0

/*
 * Reg Regulator Mode 1
 */
#define VSIM_EN_LSH	0
#define VSIM_EN_WID	1
#define VSIM_EN_ENABLE	1
#define VSIM_EN_DISABLE	0
#define VESIM_EN_LSH	3
#define VESIM_EN_WID	1
#define VESIM_EN_ENABLE	1
#define VESIM_EN_DISABLE	0
#define VCAM_EN_LSH	6
#define VCAM_EN_WID	1
#define VCAM_EN_ENABLE	1
#define VCAM_EN_DISABLE	0
#define VRFBG_EN_LSH	9
#define VRFBG_EN_WID	1
#define VRFBG_EN_ENABLE	1
#define VRFBG_EN_DISABLE	0
#define VVIB_EN_LSH	11
#define VVIB_EN_WID	1
#define VVIB_EN_ENABLE	1
#define VVIB_EN_DISABLE	0
#define VRF1_EN_LSH	12
#define VRF1_EN_WID	1
#define VRF1_EN_ENABLE	1
#define VRF1_EN_DISABLE	0
#define VRF2_EN_LSH	15
#define VRF2_EN_WID	1
#define VRF2_EN_ENABLE	1
#define VRF2_EN_DISABLE	0
#define VMMC1_EN_LSH	18
#define VMMC1_EN_WID	1
#define VMMC1_EN_ENABLE	1
#define VMMC1_EN_DISABLE	0
#define VMMC2_EN_LSH	21
#define VMMC2_EN_WID	1
#define VMMC2_EN_ENABLE	1
#define VMMC2_EN_DISABLE	0

/*
 * Reg Regulator Setting 0
 */
#define VIOLO_LSH		2
#define VIOLO_WID		2
#define VDIG_LSH		4
#define VDIG_WID		2
#define VGEN_LSH		6
#define VGEN_WID		3
#define VRFDIG_LSH		9
#define VRFDIG_WID		2
#define VRFREF_LSH		11
#define VRFREF_WID		2
#define VRFCP_LSH		13
#define VRFCP_WID		1
#define VSIM_LSH		14
#define VSIM_WID		1
#define VESIM_LSH		15
#define VESIM_WID		1
#define VCAM_LSH		16
#define VCAM_WID		3

/*
 * Reg Regulator Setting 1
 */
#define VVIB_LSH		0
#define VVIB_WID		2
#define VRF1_LSH		2
#define VRF1_WID		2
#define VRF2_LSH		4
#define VRF2_WID		2
#define VMMC1_LSH		6
#define VMMC1_WID		3
#define VMMC2_LSH		9
#define VMMC2_WID		3

/*
 * Reg Switcher 0
 */
#define SW1A_LSH		0
#define SW1A_WID		6
#define SW1A_DVS_LSH	6
#define SW1A_DVS_WID	6
#define SW1A_STDBY_LSH	12
#define SW1A_STDBY_WID	6

/*
 * Reg Switcher 1
 */
#define SW1B_LSH		0
#define SW1B_WID		6
#define SW1B_DVS_LSH	6
#define SW1B_DVS_WID	6
#define SW1B_STDBY_LSH	12
#define SW1B_STDBY_WID	6

/*
 * Reg Switcher 2
 */
#define SW2A_LSH		0
#define SW2A_WID		6
#define SW2A_DVS_LSH	6
#define SW2A_DVS_WID	6
#define SW2A_STDBY_LSH	12
#define SW2A_STDBY_WID	6

/*
 * Reg Switcher 3
 */
#define SW2B_LSH		0
#define SW2B_WID		6
#define SW2B_DVS_LSH	6
#define SW2B_DVS_WID	6
#define SW2B_STDBY_LSH	12
#define SW2B_STDBY_WID	6

/*
 * Reg Switcher 4
 */
#define SW1A_MODE_LSH		0
#define SW1A_MODE_WID		2
#define SW1A_DVS_SPEED_LSH		6
#define SW1A_DVS_SPEED_WID		2
#define SW1B_MODE_LSH		10
#define SW1B_MODE_WID		2
#define SW1B_DVS_SPEED_LSH		14
#define SW1B_DVS_SPEED_WID		2

/*
 * Reg Switcher 5
 */
#define SW2A_MODE_LSH		0
#define SW2A_MODE_WID		2
#define SW2A_DVS_SPEED_LSH		6
#define SW2A_DVS_SPEED_WID		2
#define SW2B_MODE_LSH		10
#define SW2B_MODE_WID		2
#define SW2B_DVS_SPEED_LSH		14
#define SW2B_DVS_SPEED_WID		2
#define SW3_LSH			18
#define SW3_WID			2
#define SW3_EN_LSH			20
#define SW3_EN_WID			2
#define SW3_EN_ENABLE		1
#define SW3_EN_DISABLE		0

/*
 * Reg Regulator Misc.
 */
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

#define NUM_MC13783_REGULATORS 33
#define dvs_speed E25mvEach16us

static int mc13783_vaudio_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VAUDIO_EN, VAUDIO_EN_ENABLE);
	register_mask = BITFMASK(VAUDIO_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vaudio_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VAUDIO_EN, VAUDIO_EN_DISABLE);
	register_mask = BITFMASK(VAUDIO_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_viohi_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOHI_EN, VIOHI_EN_ENABLE);
	register_mask = BITFMASK(VIOHI_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_viohi_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOHI_EN, VIOHI_EN_DISABLE);
	register_mask = BITFMASK(VIOHI_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_violo_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0, register1 = 0;
	int voltage, mV = uV / 1000;

	if ((mV >= 1200) && (mV < 1300))
		voltage = VIOLO_1_2V;
	else if ((mV >= 1300) && (mV < 1500))
		voltage = VIOLO_1_3V;
	else if ((mV >= 1500) && (mV < 1800))
		voltage = VIOLO_1_5V;
	else
		voltage = VIOLO_1_8V;

	register_val = BITFVAL(VIOLO, voltage);
	register_mask = BITFMASK(VIOLO);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_violo_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VIOLO);

	switch (voltage) {
	case VIOLO_1_2V:
		mV = 1200;
		break;
	case VIOLO_1_3V:
		mV = 1300;
		break;
	case VIOLO_1_5V:
		mV = 1500;
		break;
	case VIOLO_1_8V:
		mV = 1800;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_violo_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOLO_EN, VIOLO_EN_ENABLE);
	register_mask = BITFMASK(VIOLO_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_violo_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VIOLO_EN, VIOLO_EN_DISABLE);
	register_mask = BITFMASK(VIOLO_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vdig_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1200) && (mV < 1300))
		voltage = VDIG_1_2V;
	else if ((mV >= 1300) && (mV < 1500))
		voltage = VDIG_1_3V;
	else if ((mV >= 1500) && (mV < 1800))
		voltage = VDIG_1_5V;
	else
		voltage = VDIG_1_8V;

	register_val = BITFVAL(VDIG, voltage);
	register_mask = BITFMASK(VDIG);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vdig_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VDIG);

	switch (voltage) {
	case VDIG_1_2V:
		mV = 1200;
		break;
	case VDIG_1_3V:
		mV = 1300;
		break;
	case VDIG_1_5V:
		mV = 1500;
		break;
	case VDIG_1_8V:
		mV = 1800;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vdig_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VDIG_EN, VDIG_EN_ENABLE);
	register_mask = BITFMASK(VDIG_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vdig_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VDIG_EN, VDIG_EN_DISABLE);
	register_mask = BITFMASK(VDIG_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vgen_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;
	int vgenid = reg->id;

	printk(KERN_INFO "VGEN ID is %d\n", vgenid);

	if ((mV >= 1100) && (mV < 1200))
		voltage = VGEN_1_1V;
	else if ((mV >= 1200) && (mV < 1300))
		voltage = VGEN_1_2V;
	else if ((mV >= 1300) && (mV < 1500))
		voltage = VGEN_1_3V;
	else if ((mV >= 1500) && (mV < 1800))
		voltage = VGEN_1_5V;
	else if ((mV >= 1800) && (mV < 2000))
		voltage = VGEN_1_8V;
	else if ((mV >= 2000) && (mV < 2400))
		voltage = VGEN_2V;
	else if ((mV >= 2400) && (mV < 2775))
		voltage = VGEN_2_4V;
	else
		voltage = VGEN_2_775V;

	register_val = BITFVAL(VGEN, voltage);
	register_mask = BITFMASK(VGEN);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vgen_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VGEN);

	switch (voltage) {
	case VGEN_1_2V:
		mV = 1200;
		break;
	case VGEN_1_3V:
		mV = 1300;
		break;
	case VGEN_1_5V:
		mV = 1500;
		break;
	case VGEN_1_8V:
		mV = 1800;
		break;
	case VGEN_1_1V:
		mV = 1100;
		break;
	case VGEN_2V:
		mV = 2000;
		break;
	case VGEN_2_775V:
		mV = 2775;
		break;
	case VGEN_2_4V:
		mV = 2400;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vgen_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN_EN, VGEN_EN_ENABLE);
	register_mask = BITFMASK(VGEN_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vgen_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VGEN_EN, VGEN_EN_DISABLE);
	register_mask = BITFMASK(VGEN_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfdig_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1200) && (mV < 1500))
		voltage = VRFDIG_1_2V;
	else if ((mV >= 1500) && (mV < 1300))
		voltage = VRFDIG_1_5V;
	else if ((mV >= 1800) && (mV < 1875))
		voltage = VRFDIG_1_8V;
	else
		voltage = VRFDIG_1_875V;

	register_val = BITFVAL(VRFDIG, voltage);
	register_mask = BITFMASK(VRFDIG);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfdig_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VRFDIG);

	switch (voltage) {
	case VRFDIG_1_2V:
		mV = 1200;
		break;
	case VRFDIG_1_5V:
		mV = 1500;
		break;
	case VRFDIG_1_8V:
		mV = 1800;
		break;
	case VRFDIG_1_875V:
		mV = 1875;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vrfdig_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFDIG_EN, VRFDIG_EN_ENABLE);
	register_mask = BITFMASK(VRFDIG_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfdig_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFDIG_EN, VRFDIG_EN_DISABLE);
	register_mask = BITFMASK(VRFDIG_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfref_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 2475) && (mV < 2600))
		voltage = VRFREF_2_475V;
	else if ((mV >= 2600) && (mV < 2700))
		voltage = VRFREF_2_6V;
	else if ((mV >= 2700) && (mV < 2775))
		voltage = VRFREF_2_7V;
	else
		voltage = VRFREF_2_775V;

	register_val = BITFVAL(VRFREF, voltage);
	register_mask = BITFMASK(VRFREF);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfref_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VRFREF);

	switch (voltage) {
	case VRFREF_2_475V:
		mV = 2475;
		break;
	case VRFREF_2_6V:
		mV = 2600;
		break;
	case VRFREF_2_7V:
		mV = 2700;
		break;
	case VRFREF_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vrfref_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFREF_EN, VRFREF_EN_ENABLE);
	register_mask = BITFMASK(VRFREF_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfref_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFREF_EN, VRFREF_EN_DISABLE);
	register_mask = BITFMASK(VRFREF_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfcp_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 2700) && (mV < 2775))
		voltage = VRFCP_2_7V;
	else
		voltage = VRFCP_2_775V;

	register_val = BITFVAL(VRFCP, voltage);
	register_mask = BITFMASK(VRFCP);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfcp_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VRFCP);

	switch (voltage) {
	case VRFCP_2_7V:
		mV = 2700;
		break;
	case VRFCP_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vrfcp_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFCP_EN, VRFCP_EN_ENABLE);
	register_mask = BITFMASK(VRFCP_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrfcp_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VRFCP_EN, VRFCP_EN_DISABLE);
	register_mask = BITFMASK(VRFCP_EN);
	register1 = REG_REGULATOR_MODE_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vsim_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1800) && (mV < 2900))
		voltage = VSIM_1_8V;
	else
		voltage = VSIM_2_9V;

	register_val = BITFVAL(VSIM, voltage);
	register_mask = BITFMASK(VSIM);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vsim_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VSIM);

	switch (voltage) {
	case VSIM_1_8V:
		mV = 1800;
		break;
	case VSIM_2_9V:
		mV = 1900;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vsim_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VSIM_EN, VSIM_EN_ENABLE);
	register_mask = BITFMASK(VSIM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vsim_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VSIM_EN, VSIM_EN_DISABLE);
	register_mask = BITFMASK(VSIM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vesim_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1800) && (mV < 2900))
		voltage = VESIM_1_8V;
	else
		voltage = VESIM_2_9V;

	register_val = BITFVAL(VESIM, voltage);
	register_mask = BITFMASK(VESIM);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vesim_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VESIM);

	switch (voltage) {
	case VESIM_1_8V:
		mV = 1800;
		break;
	case VESIM_2_9V:
		mV = 1900;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vesim_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VESIM_EN, VESIM_EN_ENABLE);
	register_mask = BITFMASK(VESIM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vesim_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VESIM_EN, VESIM_EN_DISABLE);
	register_mask = BITFMASK(VESIM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vcam_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1500) && (mV < 1800))
		voltage = VCAM_1_5V;
	else if ((mV >= 1800) && (mV < 2500))
		voltage = VCAM_1_8V;
	else if ((mV >= 2500) && (mV < 2550))
		voltage = VCAM_2_5V;
	else if ((mV >= 2550) && (mV < 2600))
		voltage = VCAM_2_55V;
	if ((mV >= 2600) && (mV < 2750))
		voltage = VCAM_2_6V;
	else if ((mV >= 2750) && (mV < 2800))
		voltage = VCAM_2_75V;
	else if ((mV >= 2800) && (mV < 3000))
		voltage = VCAM_2_8V;
	else
		voltage = VCAM_3V;

	register_val = BITFVAL(VCAM, voltage);
	register_mask = BITFMASK(VCAM);
	register1 = REG_REGULATOR_SETTING_0;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vcam_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_0,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VCAM);

	switch (voltage) {
	case VCAM_1_5V:
		mV = 1500;
		break;
	case VCAM_1_8V:
		mV = 1800;
		break;
	case VCAM_2_5V:
		mV = 2500;
		break;
	case VCAM_2_55V:
		mV = 2550;
		break;
	case VCAM_2_6V:
		mV = 2600;
		break;
	case VCAM_2_75V:
		mV = 2750;
		break;
	case VCAM_2_8V:
		mV = 2800;
		break;
	case VCAM_3V:
		mV = 3000;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vcam_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VCAM_EN, VCAM_EN_ENABLE);
	register_mask = BITFMASK(VCAM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vcam_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VCAM_EN, VCAM_EN_DISABLE);
	register_mask = BITFMASK(VCAM_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vvib_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mV = uV / 1000;

	if ((mV >= 1300) && (mV < 1800))
		voltage = VVIB_1_3V;
	else if ((mV >= 1800) && (mV < 2000))
		voltage = VVIB_1_8V;
	else if ((mV >= 2000) && (mV < 3000))
		voltage = VVIB_2V;
	else
		voltage = VVIB_3V;

	register_val = BITFVAL(VVIB, voltage);
	register_mask = BITFMASK(VVIB);
	register1 = REG_REGULATOR_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vvib_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_1,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, VVIB);

	switch (voltage) {
	case VVIB_1_3V:
		mV = 1300;
		break;
	case VVIB_1_8V:
		mV = 1800;
		break;
	case VVIB_2V:
		mV = 2000;
		break;
	case VVIB_3V:
		mV = 3000;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vvib_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VVIB_EN, VVIB_EN_ENABLE);
	register_mask = BITFMASK(VVIB_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vvib_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(VVIB_EN, VVIB_EN_DISABLE);
	register_mask = BITFMASK(VVIB_EN);
	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrf_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, rf = reg->id, mV = uV / 1000;

	if ((mV >= 1500) && (mV < 1875))
		voltage = VRF_1_5V;
	else if ((mV >= 1875) && (mV < 2700))
		voltage = VRF_1_875V;
	else if ((mV >= 2700) && (mV < 2775))
		voltage = VRF_2_7V;
	else
		voltage = VRF_2_775V;

	switch (rf) {
	case MC13783_VRF1:
		register_val = BITFVAL(VRF1, voltage);
		register_mask = BITFMASK(VRF1);
		break;
	case MC13783_VRF2:
		register_val = BITFVAL(VRF2, voltage);
		register_mask = BITFMASK(VRF2);
		break;
	default:
		return -EINVAL;
	}

	register1 = REG_REGULATOR_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrf_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, rf = reg->id, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_1,
				  &register_val, PMIC_ALL_BITS));

	switch (rf) {
	case MC13783_VRF1:
		voltage = BITFEXT(register_val, VRF1);
		break;
	case MC13783_VRF2:
		voltage = BITFEXT(register_val, VRF2);
		break;
	default:
		return -EINVAL;
	};

	switch (voltage) {
	case VRF_1_5V:
		mV = 1500;
		break;
	case VRF_1_875V:
		mV = 1875;
		break;
	case VRF_2_7V:
		mV = 2700;
		break;
	case VRF_2_775V:
		mV = 2775;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vrf_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int vrf = reg->id;

	switch (vrf) {
	case MC13783_VRF1:
		register_val = BITFVAL(VRF1_EN, VRF1_EN_ENABLE);
		register_mask = BITFMASK(VRF1_EN);
		break;
	case MC13783_VRF2:
		register_val = BITFVAL(VRF2_EN, VRF2_EN_ENABLE);
		register_mask = BITFMASK(VRF2_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vrf_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int vrf = reg->id;

	switch (vrf) {
	case MC13783_VRF1:
		register_val = BITFVAL(VRF1_EN, VRF1_EN_DISABLE);
		register_mask = BITFMASK(VRF1_EN);
		break;
	case MC13783_VRF2:
		register_val = BITFVAL(VRF2_EN, VRF2_EN_DISABLE);
		register_mask = BITFMASK(VRF2_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vmmc_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int voltage, mmc = reg->id, mV = uV / 1000;

	printk(KERN_INFO "VMMC ID is %d\n", mmc);

	if ((mV >= 1600) && (mV < 1800))
		voltage = VMMC_1_6V;
	else if ((mV >= 1800) && (mV < 2000))
		voltage = VMMC_1_8V;
	else if ((mV >= 2000) && (mV < 2600))
		voltage = VMMC_2V;
	else if ((mV >= 2600) && (mV < 2700))
		voltage = VMMC_2_6V;
	else if ((mV >= 2700) && (mV < 2800))
		voltage = VMMC_2_7V;
	else if ((mV >= 2800) && (mV < 2900))
		voltage = VMMC_2_8V;
	else if ((mV >= 2900) && (mV < 3000))
		voltage = VMMC_2_9V;
	else
		voltage = VMMC_3V;

	switch (mmc) {
	case MC13783_VMMC1:
		register_val = BITFVAL(VMMC1, voltage);
		register_mask = BITFMASK(VMMC1);
		break;
	case MC13783_VMMC2:
		register_val = BITFVAL(VMMC2, voltage);
		register_mask = BITFMASK(VMMC2);
		break;
	default:
		return -EINVAL;
	}

	register1 = REG_REGULATOR_SETTING_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vmmc_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mmc = reg->id, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_REGULATOR_SETTING_1,
				  &register_val, PMIC_ALL_BITS));

	switch (mmc) {
	case MC13783_VMMC1:
		voltage = BITFEXT(register_val, VMMC1);
		break;
	case MC13783_VMMC2:
		voltage = BITFEXT(register_val, VMMC2);
		break;
	default:
		return -EINVAL;
	}

	switch (voltage) {
	case VMMC_1_6V:
		mV = 1600;
		break;
	case VMMC_1_8V:
		mV = 1800;
		break;
	case VMMC_2V:
		mV = 2000;
		break;
	case VMMC_2_6V:
		mV = 2600;
		break;
	case VMMC_2_7V:
		mV = 2700;
		break;
	case VMMC_2_8V:
		mV = 2800;
		break;
	case VMMC_2_9V:
		mV = 2900;
		break;
	case VMMC_3V:
		mV = 3000;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_vmmc_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int vmmc = reg->id;

	switch (vmmc) {
	case MC13783_VMMC1:
		register_val = BITFVAL(VMMC1_EN, VMMC1_EN_ENABLE);
		register_mask = BITFMASK(VMMC1_EN);
		break;
	case MC13783_VMMC2:
		register_val = BITFVAL(VMMC2_EN, VMMC2_EN_ENABLE);
		register_mask = BITFMASK(VMMC2_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_vmmc_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int vmmc = reg->id;

	switch (vmmc) {
	case MC13783_VMMC1:
		register_val = BITFVAL(VMMC1_EN, VMMC1_EN_DISABLE);
		register_mask = BITFMASK(VMMC1_EN);
		break;
	case MC13783_VMMC2:
		register_val = BITFVAL(VMMC2_EN, VMMC2_EN_DISABLE);
		register_mask = BITFMASK(VMMC2_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_REGULATOR_MODE_1;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_gpo_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int gpo = reg->id;

	switch (gpo) {
	case MC13783_GPO1:
		register_val = BITFVAL(GPO1_EN, GPO1_EN_ENABLE);
		register_mask = BITFMASK(GPO1_EN);
		break;
	case MC13783_GPO2:
		register_val = BITFVAL(GPO2_EN, GPO2_EN_ENABLE);
		register_mask = BITFMASK(GPO2_EN);
		break;
	case MC13783_GPO3:
		register_val = BITFVAL(GPO3_EN, GPO3_EN_ENABLE);
		register_mask = BITFMASK(GPO3_EN);
		break;
	case MC13783_GPO4:
		register_val = BITFVAL(GPO4_EN, GPO4_EN_ENABLE);
		register_mask = BITFMASK(GPO4_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_POWER_MISCELLANEOUS;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_gpo_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;
	int gpo = reg->id;

	switch (gpo) {
	case MC13783_GPO1:
		register_val = BITFVAL(GPO1_EN, GPO1_EN_DISABLE);
		register_mask = BITFMASK(GPO1_EN);
		break;
	case MC13783_GPO2:
		register_val = BITFVAL(GPO2_EN, GPO2_EN_DISABLE);
		register_mask = BITFMASK(GPO2_EN);
		break;
	case MC13783_GPO3:
		register_val = BITFVAL(GPO3_EN, GPO3_EN_DISABLE);
		register_mask = BITFMASK(GPO3_EN);
		break;
	case MC13783_GPO4:
		register_val = BITFVAL(GPO4_EN, GPO4_EN_DISABLE);
		register_mask = BITFMASK(GPO4_EN);
		break;
	default:
		return -EINVAL;
	};

	register1 = REG_POWER_MISCELLANEOUS;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw3_set_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0, register1 = 0;
	int voltage, mV = uV / 1000;

	if ((mV >= 5000) && (mV < 5500))
		voltage = SW3_5V;
	else
		voltage = SW3_5_5V;

	register_val = BITFVAL(SW3, voltage);
	register_mask = BITFMASK(SW3);
	register1 = REG_SWITCHERS_5;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw3_get_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0;

	CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_5,
				  &register_val, PMIC_ALL_BITS));
	voltage = BITFEXT(register_val, SW3);

	if (voltage == SW3_5_5V)
		mV = 5500;
	else
		mV = 5000;

	return mV * 1000;
}

static int mc13783_sw3_enable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(SW3_EN, SW3_EN_ENABLE);
	register_mask = BITFMASK(SW3_EN);
	register1 = REG_SWITCHERS_5;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw3_disable(struct regulator *reg)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1;

	register_val = BITFVAL(SW3_EN, SW3_EN_DISABLE);
	register_mask = BITFMASK(SW3_EN);
	register1 = REG_SWITCHERS_5;

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw_set_normal_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1 = 0;
	int voltage, sw = reg->id, mV = uV / 1000;

	if ((mV >= 900) && (mV < 925))
		voltage = SW_0_9V;
	else if ((mV >= 925) && (mV < 950))
		voltage = SW_0_925V;
	else if ((mV >= 950) && (mV < 975))
		voltage = SW_0_95V;
	else if ((mV >= 975) && (mV < 1000))
		voltage = SW_0_975V;
	else if ((mV >= 1000) && (mV < 1025))
		voltage = SW_1V;
	else if ((mV >= 1025) && (mV < 1050))
		voltage = SW_1_025V;
	else if ((mV >= 1050) && (mV < 1075))
		voltage = SW_1_05V;
	else if ((mV >= 1075) && (mV < 1100))
		voltage = SW_1_075V;
	else if ((mV >= 1100) && (mV < 1125))
		voltage = SW_1_1V;
	else if ((mV >= 1125) && (mV < 1150))
		voltage = SW_1_125V;
	else if ((mV >= 1150) && (mV < 1175))
		voltage = SW_1_15V;
	else if ((mV >= 1175) && (mV < 1200))
		voltage = SW_1_175V;
	else if ((mV >= 1200) && (mV < 1225))
		voltage = SW_1_2V;
	else if ((mV >= 1225) && (mV < 1250))
		voltage = SW_1_225V;
	else if ((mV >= 1250) && (mV < 1275))
		voltage = SW_1_25V;
	else if ((mV >= 1275) && (mV < 1300))
		voltage = SW_1_275V;
	else if ((mV >= 1300) && (mV < 1325))
		voltage = SW_1_3V;
	else if ((mV >= 1325) && (mV < 1350))
		voltage = SW_1_325V;
	else if ((mV >= 1350) && (mV < 1375))
		voltage = SW_1_35V;
	else if ((mV >= 1375) && (mV < 1400))
		voltage = SW_1_375V;
	else if ((mV >= 1400) && (mV < 1425))
		voltage = SW_1_4V;
	else if ((mV >= 1425) && (mV < 1450))
		voltage = SW_1_425V;
	else if ((mV >= 1450) && (mV < 1475))
		voltage = SW_1_45V;
	else if ((mV >= 1475) && (mV < 1500))
		voltage = SW_1_475V;
	else if ((mV >= 1500) && (mV < 1525))
		voltage = SW_1_5V;
	else if ((mV >= 1525) && (mV < 1550))
		voltage = SW_1_525V;
	else if ((mV >= 1550) && (mV < 1575))
		voltage = SW_1_55V;
	else if ((mV >= 1575) && (mV < 1600))
		voltage = SW_1_575V;
	else if ((mV >= 1600) && (mV < 1625))
		voltage = SW_1_6V;
	else if ((mV >= 1625) && (mV < 1650))
		voltage = SW_1_625V;
	else if ((mV >= 1650) && (mV < 1675))
		voltage = SW_1_65V;
	else if ((mV >= 1675) && (mV < 1700))
		voltage = SW_1_675V;
	else if ((mV >= 1700) && (mV < 1800))
		voltage = SW_1_7V;
	else if ((mV >= 1800) && (mV < 1850))
		voltage = SW_1_8V;
	else if ((mV >= 1850) && (mV < 2000))
		voltage = SW_1_85V;
	else if ((mV >= 2000) && (mV < 2100))
		voltage = SW_2V;
	else if ((mV >= 2100) && (mV < 2200))
		voltage = SW_2_1V;
	else
		voltage = SW_2_2V;

	switch (sw) {
	case MC13783_SW1A:
		register1 = REG_SWITCHERS_0;
		register_val = BITFVAL(SW1A, voltage);
		register_mask = BITFMASK(SW1A);
		break;
	case MC13783_SW1B:
		register1 = REG_SWITCHERS_1;
		register_val = BITFVAL(SW1B, voltage);
		register_mask = BITFMASK(SW1B);
		break;
	case MC13783_SW2A:
		register1 = REG_SWITCHERS_2;
		register_val = BITFVAL(SW2A, voltage);
		register_mask = BITFMASK(SW2A);
		break;
	case MC13783_SW2B:
		register1 = REG_SWITCHERS_3;
		register_val = BITFVAL(SW2B, voltage);
		register_mask = BITFMASK(SW2B);
		break;
	default:
		return -EINVAL;
	}

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw_get_normal_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id;

	switch (sw) {
	case MC13783_SW1A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1A);
		break;
	case MC13783_SW1B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1B);
		break;
	case MC13783_SW2A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_2,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2A);
		break;
	case MC13783_SW2B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_3,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2B);
		break;
	default:
		return -EINVAL;
	}

	switch (voltage) {
	case SW_0_9V:
		mV = 900;
		break;
	case SW_0_925V:
		mV = 925;
		break;
	case SW_0_95V:
		mV = 950;
		break;
	case SW_0_975V:
		mV = 975;
		break;
	case SW_1V:
		mV = 1000;
		break;
	case SW_1_025V:
		mV = 1025;
		break;
	case SW_1_05V:
		mV = 1050;
		break;
	case SW_1_075V:
		mV = 1075;
		break;
	case SW_1_1V:
		mV = 1100;
		break;
	case SW_1_125V:
		mV = 1125;
		break;
	case SW_1_15V:
		mV = 1150;
		break;
	case SW_1_175V:
		mV = 1175;
		break;
	case SW_1_2V:
		mV = 1200;
		break;
	case SW_1_225V:
		mV = 1225;
		break;
	case SW_1_25V:
		mV = 1250;
		break;
	case SW_1_275V:
		mV = 1275;
		break;
	case SW_1_3V:
		mV = 1300;
		break;
	case SW_1_325V:
		mV = 1325;
		break;
	case SW_1_35V:
		mV = 1350;
		break;
	case SW_1_375V:
		mV = 1375;
		break;
	case SW_1_4V:
		mV = 1400;
		break;
	case SW_1_425V:
		mV = 1425;
		break;
	case SW_1_45V:
		mV = 1450;
		break;
	case SW_1_475V:
		mV = 1475;
		break;
	case SW_1_5V:
		mV = 1500;
		break;
	case SW_1_525V:
		mV = 1525;
		break;
	case SW_1_55V:
		mV = 1550;
		break;
	case SW_1_575V:
		mV = 1575;
		break;
	case SW_1_6V:
		mV = 1600;
		break;
	case SW_1_625V:
		mV = 1625;
		break;
	case SW_1_65V:
		mV = 1650;
		break;
	case SW_1_675V:
		mV = 1675;
		break;
	case SW_1_7V:
		mV = 1700;
		break;
	case SW_1_8V:
		mV = 1800;
		break;
	case SW_1_85V:
		mV = 1850;
		break;
	case SW_2V:
		mV = 2000;
		break;
	case SW_2_1V:
		mV = 2100;
		break;
	case SW_2_2V:
		mV = 2200;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_sw_set_dvs_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0, register2_val =
	    0, register2_mask = 0;
	unsigned int register1 = 0, register2 = 0;
	int voltage, sw = reg->id, mV = uV / 1000;

	if ((mV >= 900) && (mV < 925))
		voltage = SW_0_9V;
	else if ((mV >= 925) && (mV < 950))
		voltage = SW_0_925V;
	else if ((mV >= 950) && (mV < 975))
		voltage = SW_0_95V;
	else if ((mV >= 975) && (mV < 1000))
		voltage = SW_0_975V;
	else if ((mV >= 1000) && (mV < 1025))
		voltage = SW_1V;
	else if ((mV >= 1025) && (mV < 1050))
		voltage = SW_1_025V;
	else if ((mV >= 1050) && (mV < 1075))
		voltage = SW_1_05V;
	else if ((mV >= 1075) && (mV < 1100))
		voltage = SW_1_075V;
	else if ((mV >= 1100) && (mV < 1125))
		voltage = SW_1_1V;
	else if ((mV >= 1125) && (mV < 1150))
		voltage = SW_1_125V;
	else if ((mV >= 1150) && (mV < 1175))
		voltage = SW_1_15V;
	else if ((mV >= 1175) && (mV < 1200))
		voltage = SW_1_175V;
	else if ((mV >= 1200) && (mV < 1225))
		voltage = SW_1_2V;
	else if ((mV >= 1225) && (mV < 1250))
		voltage = SW_1_225V;
	else if ((mV >= 1250) && (mV < 1275))
		voltage = SW_1_25V;
	else if ((mV >= 1275) && (mV < 1300))
		voltage = SW_1_275V;
	else if ((mV >= 1300) && (mV < 1325))
		voltage = SW_1_3V;
	else if ((mV >= 1325) && (mV < 1350))
		voltage = SW_1_325V;
	else if ((mV >= 1350) && (mV < 1375))
		voltage = SW_1_35V;
	else if ((mV >= 1375) && (mV < 1400))
		voltage = SW_1_375V;
	else if ((mV >= 1400) && (mV < 1425))
		voltage = SW_1_4V;
	else if ((mV >= 1425) && (mV < 1450))
		voltage = SW_1_425V;
	else if ((mV >= 1450) && (mV < 1475))
		voltage = SW_1_45V;
	else if ((mV >= 1475) && (mV < 1500))
		voltage = SW_1_475V;
	else if ((mV >= 1500) && (mV < 1525))
		voltage = SW_1_5V;
	else if ((mV >= 1525) && (mV < 1550))
		voltage = SW_1_525V;
	else if ((mV >= 1550) && (mV < 1575))
		voltage = SW_1_55V;
	else if ((mV >= 1575) && (mV < 1600))
		voltage = SW_1_575V;
	else if ((mV >= 1600) && (mV < 1625))
		voltage = SW_1_6V;
	else if ((mV >= 1625) && (mV < 1650))
		voltage = SW_1_625V;
	else if ((mV >= 1650) && (mV < 1675))
		voltage = SW_1_65V;
	else if ((mV >= 1675) && (mV < 1700))
		voltage = SW_1_675V;
	else if ((mV >= 1700) && (mV < 1800))
		voltage = SW_1_7V;
	else if ((mV >= 1800) && (mV < 1850))
		voltage = SW_1_8V;
	else if ((mV >= 1850) && (mV < 2000))
		voltage = SW_1_85V;
	else if ((mV >= 2000) && (mV < 2100))
		voltage = SW_2V;
	else if ((mV >= 2100) && (mV < 2200))
		voltage = SW_2_1V;
	else
		voltage = SW_2_2V;

	switch (sw) {
	case MC13783_SW1A:
		register1 = REG_SWITCHERS_0;
		register_val = BITFVAL(SW1A_DVS, voltage);
		register_mask = BITFMASK(SW1A_DVS);
		register2 = REG_SWITCHERS_4;
		register2_val = BITFVAL(SW1A_DVS_SPEED, dvs_speed);
		register2_mask = BITFMASK(SW1A_DVS_SPEED);
		break;
	case MC13783_SW1B:
		register1 = REG_SWITCHERS_1;
		register_val = BITFVAL(SW1B_DVS, voltage);
		register_mask = BITFMASK(SW1B_DVS);
		register2 = REG_SWITCHERS_4;
		register2_val = BITFVAL(SW1B_DVS_SPEED, dvs_speed);
		register2_mask = BITFMASK(SW1B_DVS_SPEED);
		break;
	case MC13783_SW2A:
		register1 = REG_SWITCHERS_2;
		register_val = BITFVAL(SW2A_DVS, voltage);
		register_mask = BITFMASK(SW2A_DVS);
		register2 = REG_SWITCHERS_5;
		register2_val = BITFVAL(SW2A_DVS_SPEED, dvs_speed);
		register2_mask = BITFMASK(SW2A_DVS_SPEED);
		break;
	case MC13783_SW2B:
		register1 = REG_SWITCHERS_3;
		register_val = BITFVAL(SW2B_DVS, voltage);
		register_mask = BITFMASK(SW2B_DVS);
		register2 = REG_SWITCHERS_5;
		register2_val = BITFVAL(SW2B_DVS_SPEED, dvs_speed);
		register2_mask = BITFMASK(SW2B_DVS_SPEED);
		break;
	default:
		return -EINVAL;
	}

	CHECK_ERROR(pmic_write_reg(register1, register_val, register_mask));
	CHECK_ERROR(pmic_write_reg(register2, register2_val, register2_mask));

	return 0;
}

static int mc13783_sw_get_dvs_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id;

	switch (sw) {
	case MC13783_SW1A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1A_DVS);
		break;
	case MC13783_SW1B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1B_DVS);
		break;
	case MC13783_SW2A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_2,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2A_DVS);
		break;
	case MC13783_SW2B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_3,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2B_DVS);
		break;
	default:
		return -EINVAL;
	}

	switch (voltage) {
	case SW_0_9V:
		mV = 900;
		break;
	case SW_0_925V:
		mV = 925;
		break;
	case SW_0_95V:
		mV = 950;
		break;
	case SW_0_975V:
		mV = 975;
		break;
	case SW_1V:
		mV = 1000;
		break;
	case SW_1_025V:
		mV = 1025;
		break;
	case SW_1_05V:
		mV = 1050;
		break;
	case SW_1_075V:
		mV = 1075;
		break;
	case SW_1_1V:
		mV = 1100;
		break;
	case SW_1_125V:
		mV = 1125;
		break;
	case SW_1_15V:
		mV = 1150;
		break;
	case SW_1_175V:
		mV = 1175;
		break;
	case SW_1_2V:
		mV = 1200;
		break;
	case SW_1_225V:
		mV = 1225;
		break;
	case SW_1_25V:
		mV = 1250;
		break;
	case SW_1_275V:
		mV = 1275;
		break;
	case SW_1_3V:
		mV = 1300;
		break;
	case SW_1_325V:
		mV = 1325;
		break;
	case SW_1_35V:
		mV = 1350;
		break;
	case SW_1_375V:
		mV = 1375;
		break;
	case SW_1_4V:
		mV = 1400;
		break;
	case SW_1_425V:
		mV = 1425;
		break;
	case SW_1_45V:
		mV = 1450;
		break;
	case SW_1_475V:
		mV = 1475;
		break;
	case SW_1_5V:
		mV = 1500;
		break;
	case SW_1_525V:
		mV = 1525;
		break;
	case SW_1_55V:
		mV = 1550;
		break;
	case SW_1_575V:
		mV = 1575;
		break;
	case SW_1_6V:
		mV = 1600;
		break;
	case SW_1_625V:
		mV = 1625;
		break;
	case SW_1_65V:
		mV = 1650;
		break;
	case SW_1_675V:
		mV = 1675;
		break;
	case SW_1_7V:
		mV = 1700;
		break;
	case SW_1_8V:
		mV = 1800;
		break;
	case SW_1_85V:
		mV = 1850;
		break;
	case SW_2V:
		mV = 2000;
		break;
	case SW_2_1V:
		mV = 2100;
		break;
	case SW_2_2V:
		mV = 2200;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static int mc13783_sw_set_stby_voltage(struct regulator *reg, int uV)
{
	unsigned int register_val = 0, register_mask = 0;
	unsigned int register1 = 0;
	int voltage, sw = reg->id, mV = uV / 1000;

	if ((mV >= 900) && (mV < 925))
		voltage = SW_0_9V;
	else if ((mV >= 925) && (mV < 950))
		voltage = SW_0_925V;
	else if ((mV >= 950) && (mV < 975))
		voltage = SW_0_95V;
	else if ((mV >= 975) && (mV < 1000))
		voltage = SW_0_975V;
	else if ((mV >= 1000) && (mV < 1025))
		voltage = SW_1V;
	else if ((mV >= 1025) && (mV < 1050))
		voltage = SW_1_025V;
	else if ((mV >= 1050) && (mV < 1075))
		voltage = SW_1_05V;
	else if ((mV >= 1075) && (mV < 1100))
		voltage = SW_1_075V;
	else if ((mV >= 1100) && (mV < 1125))
		voltage = SW_1_1V;
	else if ((mV >= 1125) && (mV < 1150))
		voltage = SW_1_125V;
	else if ((mV >= 1150) && (mV < 1175))
		voltage = SW_1_15V;
	else if ((mV >= 1175) && (mV < 1200))
		voltage = SW_1_175V;
	else if ((mV >= 1200) && (mV < 1225))
		voltage = SW_1_2V;
	else if ((mV >= 1225) && (mV < 1250))
		voltage = SW_1_225V;
	else if ((mV >= 1250) && (mV < 1275))
		voltage = SW_1_25V;
	else if ((mV >= 1275) && (mV < 1300))
		voltage = SW_1_275V;
	else if ((mV >= 1300) && (mV < 1325))
		voltage = SW_1_3V;
	else if ((mV >= 1325) && (mV < 1350))
		voltage = SW_1_325V;
	else if ((mV >= 1350) && (mV < 1375))
		voltage = SW_1_35V;
	else if ((mV >= 1375) && (mV < 1400))
		voltage = SW_1_375V;
	else if ((mV >= 1400) && (mV < 1425))
		voltage = SW_1_4V;
	else if ((mV >= 1425) && (mV < 1450))
		voltage = SW_1_425V;
	else if ((mV >= 1450) && (mV < 1475))
		voltage = SW_1_45V;
	else if ((mV >= 1475) && (mV < 1500))
		voltage = SW_1_475V;
	else if ((mV >= 1500) && (mV < 1525))
		voltage = SW_1_5V;
	else if ((mV >= 1525) && (mV < 1550))
		voltage = SW_1_525V;
	else if ((mV >= 1550) && (mV < 1575))
		voltage = SW_1_55V;
	else if ((mV >= 1575) && (mV < 1600))
		voltage = SW_1_575V;
	else if ((mV >= 1600) && (mV < 1625))
		voltage = SW_1_6V;
	else if ((mV >= 1625) && (mV < 1650))
		voltage = SW_1_625V;
	else if ((mV >= 1650) && (mV < 1675))
		voltage = SW_1_65V;
	else if ((mV >= 1675) && (mV < 1700))
		voltage = SW_1_675V;
	else if ((mV >= 1700) && (mV < 1800))
		voltage = SW_1_7V;
	else if ((mV >= 1800) && (mV < 1850))
		voltage = SW_1_8V;
	else if ((mV >= 1850) && (mV < 2000))
		voltage = SW_1_85V;
	else if ((mV >= 2000) && (mV < 2100))
		voltage = SW_2V;
	else if ((mV >= 2100) && (mV < 2200))
		voltage = SW_2_1V;
	else
		voltage = SW_2_2V;

	switch (sw) {
	case MC13783_SW1A:
		register1 = REG_SWITCHERS_0;
		register_val = BITFVAL(SW1A_STDBY, voltage);
		register_mask = BITFMASK(SW1A_STDBY);
		break;
	case MC13783_SW1B:
		register1 = REG_SWITCHERS_1;
		register_val = BITFVAL(SW1B_STDBY, voltage);
		register_mask = BITFMASK(SW1B_STDBY);
		break;
	case MC13783_SW2A:
		register1 = REG_SWITCHERS_2;
		register_val = BITFVAL(SW2A_STDBY, voltage);
		register_mask = BITFMASK(SW2A_STDBY);
		break;
	case MC13783_SW2B:
		register1 = REG_SWITCHERS_3;
		register_val = BITFVAL(SW2B_STDBY, voltage);
		register_mask = BITFMASK(SW2B_STDBY);
		break;
	default:
		return -EINVAL;
	}

	return (pmic_write_reg(register1, register_val, register_mask));
}

static int mc13783_sw_get_stby_voltage(struct regulator *reg)
{
	unsigned int register_val = 0;
	int voltage = 0, mV = 0, sw = reg->id;

	switch (sw) {
	case MC13783_SW1A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_0,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1A_STDBY);
		break;
	case MC13783_SW1B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_1,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW1B_STDBY);
		break;
	case MC13783_SW2A:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_2,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2A_STDBY);
		break;
	case MC13783_SW2B:
		CHECK_ERROR(pmic_read_reg(REG_SWITCHERS_3,
					  &register_val, PMIC_ALL_BITS));
		voltage = BITFEXT(register_val, SW2B_STDBY);
		break;
	default:
		return -EINVAL;
	}

	switch (voltage) {
	case SW_0_9V:
		mV = 900;
		break;
	case SW_0_925V:
		mV = 925;
		break;
	case SW_0_95V:
		mV = 950;
		break;
	case SW_0_975V:
		mV = 975;
		break;
	case SW_1V:
		mV = 1000;
		break;
	case SW_1_025V:
		mV = 1025;
		break;
	case SW_1_05V:
		mV = 1050;
		break;
	case SW_1_075V:
		mV = 1075;
		break;
	case SW_1_1V:
		mV = 1100;
		break;
	case SW_1_125V:
		mV = 1125;
		break;
	case SW_1_15V:
		mV = 1150;
		break;
	case SW_1_175V:
		mV = 1175;
		break;
	case SW_1_2V:
		mV = 1200;
		break;
	case SW_1_225V:
		mV = 1225;
		break;
	case SW_1_25V:
		mV = 1250;
		break;
	case SW_1_275V:
		mV = 1275;
		break;
	case SW_1_3V:
		mV = 1300;
		break;
	case SW_1_325V:
		mV = 1325;
		break;
	case SW_1_35V:
		mV = 1350;
		break;
	case SW_1_375V:
		mV = 1375;
		break;
	case SW_1_4V:
		mV = 1400;
		break;
	case SW_1_425V:
		mV = 1425;
		break;
	case SW_1_45V:
		mV = 1450;
		break;
	case SW_1_475V:
		mV = 1475;
		break;
	case SW_1_5V:
		mV = 1500;
		break;
	case SW_1_525V:
		mV = 1525;
		break;
	case SW_1_55V:
		mV = 1550;
		break;
	case SW_1_575V:
		mV = 1575;
		break;
	case SW_1_6V:
		mV = 1600;
		break;
	case SW_1_625V:
		mV = 1625;
		break;
	case SW_1_65V:
		mV = 1650;
		break;
	case SW_1_675V:
		mV = 1675;
		break;
	case SW_1_7V:
		mV = 1700;
		break;
	case SW_1_8V:
		mV = 1800;
		break;
	case SW_1_85V:
		mV = 1850;
		break;
	case SW_2V:
		mV = 2000;
		break;
	case SW_2_1V:
		mV = 2100;
		break;
	case SW_2_2V:
		mV = 2200;
		break;
	default:
		return -EINVAL;
	}

	return mV * 1000;
}

static struct regulator_ops mc13783_vaudio_ops = {
	.enable = mc13783_vaudio_enable,
	.disable = mc13783_vaudio_disable,
};

static struct regulator_ops mc13783_viohi_ops = {
	.enable = mc13783_viohi_enable,
	.disable = mc13783_viohi_disable,
};

static struct regulator_ops mc13783_violo_ops = {
	.set_voltage = mc13783_violo_set_voltage,
	.get_voltage = mc13783_violo_get_voltage,
	.enable = mc13783_violo_enable,
	.disable = mc13783_violo_disable,
};

static struct regulator_ops mc13783_vdig_ops = {
	.set_voltage = mc13783_vdig_set_voltage,
	.get_voltage = mc13783_vdig_get_voltage,
	.enable = mc13783_vdig_enable,
	.disable = mc13783_vdig_disable,
};

static struct regulator_ops mc13783_vgen_ops = {
	.set_voltage = mc13783_vgen_set_voltage,
	.get_voltage = mc13783_vgen_get_voltage,
	.enable = mc13783_vgen_enable,
	.disable = mc13783_vgen_disable,
};

static struct regulator_ops mc13783_vrfdig_ops = {
	.set_voltage = mc13783_vrfdig_set_voltage,
	.get_voltage = mc13783_vrfdig_get_voltage,
	.enable = mc13783_vrfdig_enable,
	.disable = mc13783_vrfdig_disable,
};

static struct regulator_ops mc13783_vrfref_ops = {
	.set_voltage = mc13783_vrfref_set_voltage,
	.get_voltage = mc13783_vrfref_get_voltage,
	.enable = mc13783_vrfref_enable,
	.disable = mc13783_vrfref_disable,
};

static struct regulator_ops mc13783_vrfcp_ops = {
	.set_voltage = mc13783_vrfcp_set_voltage,
	.get_voltage = mc13783_vrfcp_get_voltage,
	.enable = mc13783_vrfcp_enable,
	.disable = mc13783_vrfcp_disable,
};

static struct regulator_ops mc13783_vsim_ops = {
	.set_voltage = mc13783_vsim_set_voltage,
	.get_voltage = mc13783_vsim_get_voltage,
	.enable = mc13783_vsim_enable,
	.disable = mc13783_vsim_disable,
};

static struct regulator_ops mc13783_vesim_ops = {
	.set_voltage = mc13783_vesim_set_voltage,
	.get_voltage = mc13783_vesim_get_voltage,
	.enable = mc13783_vesim_enable,
	.disable = mc13783_vesim_disable,
};

static struct regulator_ops mc13783_vcam_ops = {
	.set_voltage = mc13783_vcam_set_voltage,
	.get_voltage = mc13783_vcam_get_voltage,
	.enable = mc13783_vcam_enable,
	.disable = mc13783_vcam_disable,
};

static struct regulator_ops mc13783_vvib_ops = {
	.set_voltage = mc13783_vvib_set_voltage,
	.get_voltage = mc13783_vvib_get_voltage,
	.enable = mc13783_vvib_enable,
	.disable = mc13783_vvib_disable,
};

static struct regulator_ops mc13783_vrf_ops = {
	.set_voltage = mc13783_vrf_set_voltage,
	.get_voltage = mc13783_vrf_get_voltage,
	.enable = mc13783_vrf_enable,
	.disable = mc13783_vrf_disable,
};

static struct regulator_ops mc13783_vmmc_ops = {
	.set_voltage = mc13783_vmmc_set_voltage,
	.get_voltage = mc13783_vmmc_get_voltage,
	.enable = mc13783_vmmc_enable,
	.disable = mc13783_vmmc_disable,
};

static struct regulator_ops mc13783_gpo_ops = {
	.enable = mc13783_gpo_enable,
	.disable = mc13783_gpo_disable,
};

static struct regulator_ops mc13783_sw3_ops = {
	.set_voltage = mc13783_sw3_set_voltage,
	.get_voltage = mc13783_sw3_get_voltage,
	.enable = mc13783_sw3_enable,
	.disable = mc13783_sw3_disable,
};

static struct regulator_ops mc13783_sw_normal_ops = {
	.set_voltage = mc13783_sw_set_normal_voltage,
	.get_voltage = mc13783_sw_get_normal_voltage,
};

static struct regulator_ops mc13783_sw_dvs_ops = {
	.set_voltage = mc13783_sw_set_dvs_voltage,
	.get_voltage = mc13783_sw_get_dvs_voltage,
};

static struct regulator_ops mc13783_sw_stby_ops = {
	.set_voltage = mc13783_sw_set_stby_voltage,
	.get_voltage = mc13783_sw_get_stby_voltage,
};

struct regulation_constraints violo_regulation_constraints = {
	.min_uV = mV_to_uV(1200),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vdig_regulation_constraints = {
	.min_uV = mV_to_uV(1200),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vgen_regulation_constraints = {
	.min_uV = mV_to_uV(1100),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vrfdig_regulation_constraints = {
	.min_uV = mV_to_uV(1200),
	.max_uV = mV_to_uV(1875),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vrfref_regulation_constraints = {
	.min_uV = mV_to_uV(2475),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vrfcp_regulation_constraints = {
	.min_uV = mV_to_uV(2700),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vsim_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(2900),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vesim_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(2900),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vcam_regulation_constraints = {
	.min_uV = mV_to_uV(1500),
	.max_uV = mV_to_uV(3000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vvib_regulation_constraints = {
	.min_uV = mV_to_uV(1300),
	.max_uV = mV_to_uV(3000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vrf_regulation_constraints = {
	.min_uV = mV_to_uV(1500),
	.max_uV = mV_to_uV(2775),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints vmmc_regulation_constraints = {
	.min_uV = mV_to_uV(1600),
	.max_uV = mV_to_uV(3000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints sw3_regulation_constraints = {
	.min_uV = mV_to_uV(5000),
	.max_uV = mV_to_uV(5500),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints sw_regulation_constraints = {
	.min_uV = mV_to_uV(900),
	.max_uV = mV_to_uV(2200),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct mc13783_regulator {
	struct regulator regulator;
};

static struct mc13783_regulator reg_mc13783[NUM_MC13783_REGULATORS] = {
	{
	 .regulator = {
		       .name = "VAUDIO",
		       .id = MC13783_VAUDIO,
		       .ops = &mc13783_vaudio_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "VIOHI",
		       .id = MC13783_VIOHI,
		       .ops = &mc13783_viohi_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "VIOLO",
		       .id = MC13783_VIOLO,
		       .ops = &mc13783_violo_ops,
		       .constraints = &violo_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VDIG",
		       .id = MC13783_VDIG,
		       .ops = &mc13783_vdig_ops,
		       .constraints = &vdig_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VGEN",
		       .id = MC13783_VGEN,
		       .ops = &mc13783_vgen_ops,
		       .constraints = &vgen_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VRFDIG",
		       .id = MC13783_VRFDIG,
		       .ops = &mc13783_vrfdig_ops,
		       .constraints = &vrfdig_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VRFREF",
		       .id = MC13783_VRFREF,
		       .ops = &mc13783_vrfref_ops,
		       .constraints = &vrfref_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VRFCP",
		       .id = MC13783_VRFCP,
		       .ops = &mc13783_vrfcp_ops,
		       .constraints = &vrfcp_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VSIM",
		       .id = MC13783_VSIM,
		       .ops = &mc13783_vsim_ops,
		       .constraints = &vsim_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VESIM",
		       .id = MC13783_VESIM,
		       .ops = &mc13783_vesim_ops,
		       .constraints = &vesim_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VCAM",
		       .id = MC13783_VCAM,
		       .ops = &mc13783_vcam_ops,
		       .constraints = &vcam_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VVIB",
		       .id = MC13783_VVIB,
		       .ops = &mc13783_vvib_ops,
		       .constraints = &vvib_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VRF1",
		       .id = MC13783_VRF1,
		       .ops = &mc13783_vrf_ops,
		       .constraints = &vrf_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VRF2",
		       .id = MC13783_VRF2,
		       .ops = &mc13783_vrf_ops,
		       .constraints = &vrf_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VMMC1",
		       .id = MC13783_VMMC1,
		       .ops = &mc13783_vmmc_ops,
		       .constraints = &vmmc_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "VMMC2",
		       .id = MC13783_VMMC2,
		       .ops = &mc13783_vmmc_ops,
		       .constraints = &vmmc_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO1",
		       .id = MC13783_GPO1,
		       .ops = &mc13783_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO2",
		       .id = MC13783_GPO2,
		       .ops = &mc13783_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO3",
		       .id = MC13783_GPO3,
		       .ops = &mc13783_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPO4",
		       .id = MC13783_GPO4,
		       .ops = &mc13783_gpo_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW3",
		       .id = MC13783_SW3,
		       .ops = &mc13783_sw3_ops,
		       .constraints = &sw3_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1A_NORMAL",
		       .id = MC13783_SW1A,
		       .ops = &mc13783_sw_normal_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1B_NORMAL",
		       .id = MC13783_SW1B,
		       .ops = &mc13783_sw_normal_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2A_NORMAL",
		       .id = MC13783_SW2A,
		       .ops = &mc13783_sw_normal_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2B_NORMAL",
		       .id = MC13783_SW2B,
		       .ops = &mc13783_sw_normal_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1A_DVS",
		       .id = MC13783_SW1A,
		       .ops = &mc13783_sw_dvs_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1B_DVS",
		       .id = MC13783_SW1B,
		       .ops = &mc13783_sw_dvs_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2A_DVS",
		       .id = MC13783_SW2A,
		       .ops = &mc13783_sw_dvs_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2B_DVS",
		       .id = MC13783_SW2B,
		       .ops = &mc13783_sw_dvs_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1A_STBY",
		       .id = MC13783_SW1A,
		       .ops = &mc13783_sw_stby_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW1B_STBY",
		       .id = MC13783_SW1B,
		       .ops = &mc13783_sw_stby_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2A_STBY",
		       .id = MC13783_SW2A,
		       .ops = &mc13783_sw_stby_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2B_STBY",
		       .id = MC13783_SW2B,
		       .ops = &mc13783_sw_stby_ops,
		       .constraints = &sw_regulation_constraints,
		       },
	 },
};

/*
 * Init and Exit
 */

int reg_mc13783_probe(void)
{
	int ret11 = 0;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(reg_mc13783); i++) {
		ret11 = regulator_register(&reg_mc13783[i].regulator);
		regulator_set_platform_constraints(reg_mc13783[i].regulator.
						   name,
						   reg_mc13783[i].regulator.
						   constraints);
		if (ret11 < 0) {
			printk(KERN_ERR "%s: failed to register %s err %d\n",
			       __func__, reg_mc13783[i].regulator.name, ret11);
			i--;
			for (; i >= 0; i--)
				regulator_unregister(&reg_mc13783[i].regulator);

			return ret11;
		}
	}

	printk(KERN_INFO "MC13783 regulator successfully probed\n");

	return 0;
}

EXPORT_SYMBOL(reg_mc13783_probe);

/* Module information */
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MC13783 Regulator driver");
MODULE_LICENSE("GPL");
