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
	MCU_SW1,
	MCU_SW2,
	MCU_SW3,
	MCU_SW4,
	MCU_LDO5,
	MCU_LDO6,
	MCU_LDO7,
	MCU_LCD,
	MCU_WIFI,
	MCU_HDD,
	MCU_GPS,
	MCU_CMOS,
	MCU_PLL
} MAX8660_regulator;

/*!
 * @enum t_pmic_regulator_voltage_mcu_sw34
 * @brief MCU PMIC Switch mode regulator SW34 output voltages.
 */
enum {
	MCU_SW34_0_725V = 0,
	MCU_SW34_0_75V,
	MCU_SW34_0_775V,
	MCU_SW34_0_8V,
	MCU_SW34_0_825V,
	MCU_SW34_0_85V,
	MCU_SW34_0_875V,
	MCU_SW34_0_9V,
	MCU_SW34_0_925V,
	MCU_SW34_0_95V,
	MCU_SW34_0_975V,
	MCU_SW34_1_0V,
	MCU_SW34_1_025V,
	MCU_SW34_1_05V,
	MCU_SW34_1_075V,
	MCU_SW34_1_1V,
	MCU_SW34_1_125V,
	MCU_SW34_1_15V,
	MCU_SW34_1_175V,
	MCU_SW34_1_2V,
	MCU_SW34_1_225V,
	MCU_SW34_1_25V,
	MCU_SW34_1_275V,
	MCU_SW34_1_3V,
	MCU_SW34_1_325V,
	MCU_SW34_1_35V,
	MCU_SW34_1_375V,
	MCU_SW34_1_4V,
	MCU_SW34_1_425V,
	MCU_SW34_1_45V,
	MCU_SW34_1_475V,
	MCU_SW34_1_5V,
	MCU_SW34_1_525V,
	MCU_SW34_1_55V,
	MCU_SW34_1_575V,
	MCU_SW34_1_6V,
	MCU_SW34_1_625V,
	MCU_SW34_1_65V,
	MCU_SW34_1_675V,
	MCU_SW34_1_7V,
	MCU_SW34_1_725V,
	MCU_SW34_1_75V,
	MCU_SW34_1_775V,
	MCU_SW34_1_8V
} t_pmic_regulator_voltage_mcu_sw34;

enum {
	MCU_LDO5_1_7V,
	MCU_LDO5_1_725V,
	MCU_LDO5_1_75V,
	MCU_LDO5_1_775V,
	MCU_LDO5_1_8V,
	MCU_LDO5_1_825V,
	MCU_LDO5_1_85V,
	MCU_LDO5_1_875V,
	MCU_LDO5_1_9V,
	MCU_LDO5_1_925V,
	MCU_LDO5_1_95V,
	MCU_LDO5_1_975V,
	MCU_LDO5_2_0V
} t_pmic_regulator_voltage_mcu_ldo5;

enum {
	MCU_LDO67_1_8V,
	MCU_LDO67_1_9V,
	MCU_LDO67_2_0V,
	MCU_LDO67_2_1V,
	MCU_LDO67_2_2V,
	MCU_LDO67_2_3V,
	MCU_LDO67_2_4V,
	MCU_LDO67_2_5V,
	MCU_LDO67_2_6V,
	MCU_LDO67_2_7V,
	MCU_LDO67_2_8V,
	MCU_LDO67_2_9V,
	MCU_LDO67_3_0V,
	MCU_LDO67_3_1V,
	MCU_LDO67_3_2V,
	MCU_LDO67_3_3V
} t_pmic_regulator_voltage_mcu_ldo67;

#define NUM_MAX8660_REGULATORS 7
#define NUM_MAX8660_CHILDREN_REGULATORS 6

#define SET_BIT_IN_BYTE(byte, pos) (byte |= (0x01 << pos))
#define CLEAR_BIT_IN_BYTE(byte, pos) (byte &= ~(0x01 << pos))

static int max8660_regulator_on(int regulator)
{
	u8 reg_mask = 0;
	int reg_num;

	if (regulator > MCU_LDO7 || regulator < MCU_SW1)
		return -1;

	switch (regulator) {
	case MCU_SW1:
		SET_BIT_IN_BYTE(reg_mask, 0);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_SW2:
		SET_BIT_IN_BYTE(reg_mask, 1);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_SW3:
		SET_BIT_IN_BYTE(reg_mask, 0);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_1;
		break;
	case MCU_SW4:
		SET_BIT_IN_BYTE(reg_mask, 2);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_1;
		break;
	case MCU_LDO5:
		SET_BIT_IN_BYTE(reg_mask, 3);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_LDO6:
		SET_BIT_IN_BYTE(reg_mask, 1);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_2;
		break;
	case MCU_LDO7:
		SET_BIT_IN_BYTE(reg_mask, 2);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_2;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(reg_num, reg_mask, reg_mask));

	return 0;
}

static int max8660_regulator_off(int regulator)
{
	u8 reg_mask = 0;
	unsigned int reg_read = 0;
	int reg_num;

	if (regulator > MCU_LDO7 || regulator < MCU_SW1)
		return PMIC_NOT_SUPPORTED;

	switch (regulator) {
	case MCU_SW1:
		SET_BIT_IN_BYTE(reg_mask, 0);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_SW2:
		SET_BIT_IN_BYTE(reg_mask, 1);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_SW3:
		SET_BIT_IN_BYTE(reg_mask, 0);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_1;
		break;
	case MCU_SW4:
		SET_BIT_IN_BYTE(reg_mask, 2);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_1;
		break;
	case MCU_LDO5:
		SET_BIT_IN_BYTE(reg_mask, 3);
		reg_num = REG_MCU_POWER_CTL;
		break;
	case MCU_LDO6:
		SET_BIT_IN_BYTE(reg_mask, 1);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_2;
		break;
	case MCU_LDO7:
		SET_BIT_IN_BYTE(reg_mask, 2);
		reg_num = REG_MAX8660_OUTPUT_ENABLE_2;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(reg_num, 0, reg_mask));

	/* handle sw3,4 */
	switch (regulator) {
	case MCU_SW3:
		reg_mask = 0;
		SET_BIT_IN_BYTE(reg_mask, 2);
		CHECK_ERROR(pmic_read_reg
			    (REG_MCU_POWER_CTL, &reg_read, reg_mask));

		/* check if hw pin enable sw34 */
		if (0 != reg_read) {

			/* keep sw4 on */
			reg_mask = 0;
			SET_BIT_IN_BYTE(reg_mask, 2);
			CHECK_ERROR(pmic_write_reg
				    (REG_MAX8660_OUTPUT_ENABLE_1, reg_mask,
				     reg_mask));

			/* disable hw pin to actually turn off sw3 */
			reg_mask = 0;
			SET_BIT_IN_BYTE(reg_mask, 2);
			CHECK_ERROR(pmic_write_reg
				    (REG_MCU_POWER_CTL, 0, reg_mask));
		}
		break;
	case MCU_SW4:
		reg_mask = 0;
		SET_BIT_IN_BYTE(reg_mask, 2);
		CHECK_ERROR(pmic_read_reg
			    (REG_MCU_POWER_CTL, &reg_read, reg_mask));

		/* check if hw pin enable sw34 */
		if (0 != reg_read) {

			/* keep sw3 on */
			reg_mask = 0;
			SET_BIT_IN_BYTE(reg_mask, 0);
			CHECK_ERROR(pmic_write_reg
				    (REG_MAX8660_OUTPUT_ENABLE_1, reg_mask,
				     reg_mask));

			/* disable hw pin to actually turn off sw4 */
			reg_mask = 0;
			SET_BIT_IN_BYTE(reg_mask, 2);
			CHECK_ERROR(pmic_write_reg
				    (REG_MCU_POWER_CTL, 0, reg_mask));
		}
		break;
	default:
		break;
	}
	return 0;
}

static int max8660_sw1_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_SW1);
}

static int max8660_sw1_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_SW1);
}

static int max8660_sw2_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_SW2);
}

static int max8660_sw2_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_SW2);
}

static int max8660_sw3_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_SW3);
}

static int max8660_sw3_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_SW3);
}

static int max8660_sw3_set_voltage(struct regulator *reg, int uV)
{
	u8 reg_mask = 0;
	int mV = uV / 1000;
	int volt;

	if (mV < 725 || mV > 1800)
		return -1;

	/* convert to reg value */
	volt = (mV - 725) / 25;

	/* hold on */
	SET_BIT_IN_BYTE(reg_mask, 0);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, 0, reg_mask));

	/* set volt */
	CHECK_ERROR(pmic_write_reg(REG_MAX8660_V3_TARGET_VOLT_1, volt, 0xff));

	/* start ramp */
	SET_BIT_IN_BYTE(reg_mask, 0);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, reg_mask, reg_mask));

	return 0;
}

static int max8660_sw3_get_voltage(struct regulator *reg)
{
	int uV;
	unsigned int reg_val = 0;

	CHECK_ERROR(pmic_read_reg
		    (REG_MAX8660_V3_TARGET_VOLT_1, &reg_val, 0xff));

	uV = 1000 * (reg_val * 25 + 725);

	return uV;
}

static int max8660_sw4_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_SW4);
}

static int max8660_sw4_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_SW4);
}

static int max8660_sw4_set_voltage(struct regulator *reg, int uV)
{
	u8 reg_mask = 0;
	int mV = uV / 1000;
	int volt;

	if (mV < 725 || mV > 1800)
		return -1;

	/* convert to reg value */
	volt = (mV - 725) / 25;

	/* hold on */
	SET_BIT_IN_BYTE(reg_mask, 4);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, 0, reg_mask));

	/* set volt */
	CHECK_ERROR(pmic_write_reg(REG_MAX8660_V4_TARGET_VOLT_1, volt, 0xff));

	/* start ramp */
	SET_BIT_IN_BYTE(reg_mask, 4);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, reg_mask, reg_mask));

	return 0;
}

static int max8660_sw4_get_voltage(struct regulator *reg)
{
	int uV;
	unsigned int reg_val = 0;

	CHECK_ERROR(pmic_read_reg
		    (REG_MAX8660_V4_TARGET_VOLT_1, &reg_val, 0xff));

	uV = 1000 * (reg_val * 25 + 725);

	return uV;
}

static int max8660_ldo5_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_LDO5);
}

static int max8660_ldo5_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_LDO5);
}

static int max8660_ldo5_set_voltage(struct regulator *reg, int uV)
{
	u8 reg_mask = 0;
	int mV = uV / 1000;
	int volt;

	if (mV < 1700 || mV > 2000)
		return -1;

	/* convert to reg value */
	volt = (mV - 1700) / 25;

	/* hold on */
	SET_BIT_IN_BYTE(reg_mask, 6);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, 0, reg_mask));

	/* set volt */
	CHECK_ERROR(pmic_write_reg(REG_MAX8660_V5_TARGET_VOLT_1, volt, 0xff));

	/* start ramp */
	SET_BIT_IN_BYTE(reg_mask, 6);
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_VOLT_CHANGE_CONTROL_1, reg_mask, reg_mask));

	return 0;
}

static int max8660_ldo5_get_voltage(struct regulator *reg)
{
	int uV;
	unsigned int reg_val = 0;

	CHECK_ERROR(pmic_read_reg
		    (REG_MAX8660_V5_TARGET_VOLT_1, &reg_val, 0xff));

	uV = 1000 * (reg_val * 25 + 1700);

	return uV;

}

static int max8660_ldo6_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_LDO6);
}

static int max8660_ldo6_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_LDO6);
}

static int max8660_ldo6_set_voltage(struct regulator *reg, int uV)
{
	int mV = uV / 1000;
	int volt;

	if (mV < 1800 || mV > 3300)
		return -1;

	/* convert to reg value */
	volt = (mV - 1800) / 100;

	/* set volt */
	CHECK_ERROR(pmic_write_reg(REG_MAX8660_V6V7_TARGET_VOLT, volt, 0x0f));

	return 0;
}

static int max8660_ldo6_get_voltage(struct regulator *reg)
{
	int uV;
	unsigned int reg_val = 0;

	CHECK_ERROR(pmic_read_reg
		    (REG_MAX8660_V6V7_TARGET_VOLT, &reg_val, 0x0f));

	uV = 1000 * (reg_val * 100 + 1800);

	return uV;
}

static int max8660_ldo7_enable(struct regulator *reg)
{
	return max8660_regulator_on(MCU_LDO7);
}

static int max8660_ldo7_disable(struct regulator *reg)
{
	return max8660_regulator_off(MCU_LDO7);
}

static int max8660_ldo7_set_voltage(struct regulator *reg, int uV)
{
	int mV = uV / 1000;
	int volt;

	if (mV < 1800 || mV > 3300)
		return -1;

	/* convert to reg value */
	volt = (mV - 1800) / 100;

	/* set volt */
	CHECK_ERROR(pmic_write_reg
		    (REG_MAX8660_V6V7_TARGET_VOLT, (volt << 4), 0xf0));

	return 0;
}

static int max8660_ldo7_get_voltage(struct regulator *reg)
{
	int uV;
	unsigned int reg_val = 0;

	CHECK_ERROR(pmic_read_reg
		    (REG_MAX8660_V6V7_TARGET_VOLT, &reg_val, 0x0f));
	reg_val = (reg_val >> 4) & 0xf;

	uV = 1000 * (reg_val * 100 + 1800);

	return uV;
}

static struct regulator_ops max8660_sw1_ops = {
	.enable = max8660_sw1_enable,
	.disable = max8660_sw1_disable,
};

static struct regulator_ops max8660_sw2_ops = {
	.enable = max8660_sw2_enable,
	.disable = max8660_sw2_disable,
};

static struct regulator_ops max8660_sw3_ops = {
	.set_voltage = max8660_sw3_set_voltage,
	.get_voltage = max8660_sw3_get_voltage,
	.enable = max8660_sw3_enable,
	.disable = max8660_sw3_disable,
};

static struct regulator_ops max8660_sw4_ops = {
	.set_voltage = max8660_sw4_set_voltage,
	.get_voltage = max8660_sw4_get_voltage,
	.enable = max8660_sw4_enable,
	.disable = max8660_sw4_disable,
};

static struct regulator_ops max8660_ldo5_ops = {
	.set_voltage = max8660_ldo5_set_voltage,
	.get_voltage = max8660_ldo5_get_voltage,
	.enable = max8660_ldo5_enable,
	.disable = max8660_ldo5_disable,
};

static struct regulator_ops max8660_ldo6_ops = {
	.set_voltage = max8660_ldo6_set_voltage,
	.get_voltage = max8660_ldo6_get_voltage,
	.enable = max8660_ldo6_enable,
	.disable = max8660_ldo6_disable,
};

static struct regulator_ops max8660_ldo7_ops = {
	.set_voltage = max8660_ldo7_set_voltage,
	.get_voltage = max8660_ldo7_get_voltage,
	.enable = max8660_ldo7_enable,
	.disable = max8660_ldo7_disable,
};

struct regulation_constraints max8660_sw3_regulation_constraints = {
	.min_uV = mV_to_uV(725),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints max8660_sw4_regulation_constraints = {
	.min_uV = mV_to_uV(725),
	.max_uV = mV_to_uV(1800),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints max8660_ldo5_regulation_constraints = {
	.min_uV = mV_to_uV(1700),
	.max_uV = mV_to_uV(2000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints max8660_ldo6_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(3300),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct regulation_constraints max8660_ldo7_regulation_constraints = {
	.min_uV = mV_to_uV(1800),
	.max_uV = mV_to_uV(3300),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

struct max8660_regulator {
	struct regulator regulator;
};

static struct max8660_regulator reg_max8660[NUM_MAX8660_REGULATORS] = {
	{
	 .regulator = {
		       .name = "SW1",
		       .id = MCU_SW1,
		       .ops = &max8660_sw1_ops,
		       .use_count = 1,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW2",
		       .id = MCU_SW2,
		       .ops = &max8660_sw2_ops,
		       .use_count = 1,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW3",
		       .id = MCU_SW3,
		       .ops = &max8660_sw3_ops,
		       .constraints = &max8660_sw3_regulation_constraints,
		       .use_count = 1,
		       },
	 },
	{
	 .regulator = {
		       .name = "SW4",
		       .id = MCU_SW4,
		       .ops = &max8660_sw4_ops,
		       .constraints = &max8660_sw4_regulation_constraints,
		       .use_count = 1,
		       },
	 },
	{
	 .regulator = {
		       .name = "LDO5",
		       .id = MCU_LDO5,
		       .ops = &max8660_ldo5_ops,
		       .constraints = &max8660_ldo5_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "LDO6",
		       .id = MCU_LDO6,
		       .ops = &max8660_ldo6_ops,
		       .constraints = &max8660_ldo6_regulation_constraints,
		       },
	 },
	{
	 .regulator = {
		       .name = "LDO7",
		       .id = MCU_LDO7,
		       .ops = &max8660_ldo7_ops,
		       .constraints = &max8660_ldo7_regulation_constraints,
		       },
	 },
};

/* children regulator ops*/

/* lcd */
static int max8660_lcd_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 6, 1);
}

static int max8660_lcd_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 6, 0);
}

static struct regulator_ops max8660_lcd_ops = {
	.enable = max8660_lcd_enable,
	.disable = max8660_lcd_disable,
};

/* wifi */
static int max8660_wifi_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 5, 1);
}

static int max8660_wifi_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 5, 0);
}

static struct regulator_ops max8660_wifi_ops = {
	.enable = max8660_wifi_enable,
	.disable = max8660_wifi_disable,
};

/* hdd */
static int max8660_hdd_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 4, 1);
}

static int max8660_hdd_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 4, 0);
}

static struct regulator_ops max8660_hdd_ops = {
	.enable = max8660_hdd_enable,
	.disable = max8660_hdd_disable,
};

/* gps */
static int max8660_gps_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 0, 1);
}

static int max8660_gps_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 0, 0);
}

static struct regulator_ops max8660_gps_ops = {
	.enable = max8660_gps_enable,
	.disable = max8660_gps_disable,
};

/* cmos */
static int max8660_cmos_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 4, 1);
}

static int max8660_cmos_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 4, 0);
}

static struct regulator_ops max8660_cmos_ops = {
	.enable = max8660_cmos_enable,
	.disable = max8660_cmos_disable,
};

/* pll */
static int max8660_pll_enable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 5, 1);
}

static int max8660_pll_disable(struct regulator *reg)
{
	return pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 5, 0);
}

static struct regulator_ops max8660_pll_ops = {
	.enable = max8660_pll_enable,
	.disable = max8660_pll_disable,
};

static struct max8660_regulator
    reg_max8660_children[NUM_MAX8660_CHILDREN_REGULATORS] = {
	{
	 .regulator = {
		       .name = "LCD",
		       .id = MCU_LCD,
		       .ops = &max8660_lcd_ops,
		       .parent = &reg_max8660[0].regulator,	/*SW1 */
		       },
	 },
	{
	 .regulator = {
		       .name = "WIFI",
		       .id = MCU_WIFI,
		       .ops = &max8660_wifi_ops,
		       .parent = &reg_max8660[3].regulator,	/*SW4 */
		       },
	 },
	{
	 .regulator = {
		       .name = "HDD",
		       .id = MCU_HDD,
		       .ops = &max8660_hdd_ops,
		       },
	 },
	{
	 .regulator = {
		       .name = "GPS",
		       .id = MCU_GPS,
		       .ops = &max8660_gps_ops,
		       .parent = &reg_max8660[0].regulator,	/*SW1 */
		       },

	 },
	/* hw not finished!! */
	{
	 .regulator = {
		       .name = "CMOS",
		       .id = MCU_CMOS,
		       .ops = &max8660_cmos_ops,
		       },

	 },
	/* hw not finished!! */
	{
	 .regulator = {
		       .name = "PLL",
		       .id = MCU_PLL,
		       .ops = &max8660_pll_ops,
		       },

	 },

};

/*
 * Init and Exit
 */
int reg_max8660_probe(void)
{
	int ret11 = 0;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(reg_max8660); i++) {
		ret11 = regulator_register(&reg_max8660[i].regulator);
		regulator_set_platform_constraints(reg_max8660[i].regulator.
						   name,
						   reg_max8660[i].regulator.
						   constraints);
		if (ret11 < 0) {
			i--;
			for (; i >= 0; i--)
				regulator_unregister(&reg_max8660[i].regulator);

			return ret11;
		}

	}

	/* for child regulators */
	for (i = 0; i < ARRAY_SIZE(reg_max8660_children); i++) {
		ret11 = regulator_register(&reg_max8660_children[i].regulator);
		regulator_set_platform_source(&reg_max8660_children[i].
					      regulator,
					      reg_max8660[i].regulator.parent);
		if (ret11 < 0) {
			i--;
			for (; i >= 0; i--)
				regulator_unregister(&reg_max8660_children[i].
						     regulator);

			return ret11;
		}

	}

	return 0;
}
EXPORT_SYMBOL(reg_max8660_probe);

/* Module information */
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MAX8660 Regulator driver");
MODULE_LICENSE("GPL");
