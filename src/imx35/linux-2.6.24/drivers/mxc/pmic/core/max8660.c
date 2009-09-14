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

/*!
 * @file max8660.c
 * @brief Driver for max8660
 *
 * @ingroup pmic
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/i2c.h>
#include <asm/arch/clock.h>
#include <asm/uaccess.h>
#include <asm/arch/pmic_external.h>
#include "max8660.h"

/* I2C bus id and device address of mcu */
#define I2C1_BUS	0
#define MAX8660_I2C_ADDR				0x68

static struct i2c_client *max8660_i2c_client;

    /* reg names for max8660
       REG_MAX8660_OUTPUT_ENABLE_1,
       REG_MAX8660_OUTPUT_ENABLE_2,
       REG_MAX8660_VOLT__CHANGE_1,
       REG_MAX8660_V3_TARGET_VOLT_1,
       REG_MAX8660_V3_TARGET_VOLT_2,
       REG_MAX8660_V4_TARGET_VOLT_1,
       REG_MAX8660_V4_TARGET_VOLT_2,
       REG_MAX8660_V5_TARGET_VOLT_1,
       REG_MAX8660_V5_TARGET_VOLT_2,
       REG_MAX8660_V6V7_TARGET_VOLT,
       REG_MAX8660_FORCE_PWM
     */

    /* save down the reg values for the device is write only */
static u8 max8660_reg_value_table[] =
    { 0x0, 0x0, 0x0, 0x17, 0x17, 0x1F, 0x1F, 0x04, 0x04, 0x0, 0x0
};
int max8660_get_buffered_reg_val(int reg_name, u8 *value)
{

	/* outof range */
	if (reg_name < REG_MAX8660_OUTPUT_ENABLE_1
	    || reg_name > REG_MAX8660_FORCE_PWM) {
		return -1;
	}
	*value =
	    max8660_reg_value_table[reg_name - REG_MAX8660_OUTPUT_ENABLE_1];
	return 0;
}
int max8660_save_buffered_reg_val(int reg_name, u8 value)
{

	/* outof range */
	if (reg_name < REG_MAX8660_OUTPUT_ENABLE_1
	    || reg_name > REG_MAX8660_FORCE_PWM) {
		return -1;
	}
	max8660_reg_value_table[reg_name - REG_MAX8660_OUTPUT_ENABLE_1] = value;
	return 0;
}

int max8660_write_reg(u8 reg, u8 value)
{
	if (i2c_smbus_write_byte_data(max8660_i2c_client, reg, value) < 0) {
		return -1;
	}
	return 0;
}

/*!
 * max8660 I2C attach function
 *
 * @param adapter            struct i2c_client *
 * @return  Always 0 because max8660 is write-only and can not be detected
 */
static int max8660_probe(struct i2c_client *client)
{
	max8660_i2c_client = client;
	return 0;
}

/*!
 * max8660 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  0
 */
static int max8660_remove(struct i2c_client *client)
{
	return 0;
}
static struct i2c_driver max8660_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "max8660",},
	.probe = max8660_probe,
	.remove = max8660_remove,
};

/* called by pmic core when init*/
int max8660_init(void)
{
	int err;
	err = i2c_add_driver(&max8660_i2c_driver);
	if (err) {
		return err;
	}
	return 0;
}
void max8660_exit(void)
{
	i2c_del_driver(&max8660_i2c_driver);
}
