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
 * @file pmic/core/mc13892.c
 * @brief This file contains MC13892 specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 * @ingroup PMIC_CORE
 */

/*
 * Includes
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>

#include <asm/uaccess.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_status.h>

#include "pmic.h"

/*
 * Defines
 */

int pmic_i2c_24bit_read(struct i2c_client *client, unsigned int reg_num)
{
	char buf[3];
	int ret;

	memset(buf, 0, 3);
	ret = i2c_smbus_read_i2c_block_data(client, reg_num, 3, buf);

	if (ret == 3) {
		ret = buf[0] << 16 | buf[1] << 8 | buf[2];
		return ret;
	} else {
		printk(KERN_ERR "24bit read error, ret = %d\n", ret);
		return -1;	/* return -1 on failure */
	}
}

int pmic_i2c_24bit_write(struct i2c_client *client,
			 unsigned int reg_num, unsigned int reg_val)
{
	char buf[3];

	buf[0] = (reg_val >> 16) & 0xff;
	buf[1] = (reg_val >> 8) & 0xff;
	buf[2] = (reg_val) & 0xff;

	return i2c_smbus_write_i2c_block_data(client, reg_num, 3, buf);
}

int pmic_read(int reg_num, unsigned int *reg_val)
{
	if (mc13892_client == NULL)
		return PMIC_ERROR;

	*reg_val = pmic_i2c_24bit_read(mc13892_client, reg_num);

	if (*reg_val == -1)
		return PMIC_ERROR;

	return PMIC_SUCCESS;
}

int pmic_write(int reg_num, const unsigned int reg_val)
{
	if (mc13892_client == NULL)
		return PMIC_ERROR;

	return pmic_i2c_24bit_write(mc13892_client, reg_num, reg_val);
}

int pmic_init_registers(void)
{
	CHECK_ERROR(pmic_write(REG_INT_MASK0, 0xFFFFFF));
	CHECK_ERROR(pmic_write(REG_INT_MASK0, 0xFFFFFF));
	CHECK_ERROR(pmic_write(REG_INT_STATUS0, 0xFFFFFF));
	CHECK_ERROR(pmic_write(REG_INT_STATUS1, 0xFFFFFF));

	return PMIC_SUCCESS;
}

static unsigned int events_enabled0;
static unsigned int events_enabled1;

unsigned int pmic_get_active_events(unsigned int *active_events)
{
	unsigned int count = 0;
	unsigned int status0, status1;
	int bit_set;

	pmic_read(REG_INT_STATUS0, &status0);
	pmic_read(REG_INT_STATUS1, &status1);
	pmic_write(REG_INT_STATUS0, status0);
	pmic_write(REG_INT_STATUS1, status1);
	status0 &= events_enabled0;
	status1 &= events_enabled1;

	while (status0) {
		bit_set = ffs(status0) - 1;
		*(active_events + count) = bit_set;
		count++;
		status0 ^= (1 << bit_set);
	}
	while (status1) {
		bit_set = ffs(status1) - 1;
		*(active_events + count) = bit_set + 24;
		count++;
		status1 ^= (1 << bit_set);
	}

	return count;
}

#define EVENT_MASK_0			0x387fff
#define EVENT_MASK_1			0x1177eb

int pmic_event_unmask(type_event event)
{
	unsigned int event_mask = 0;
	unsigned int mask_reg = 0;
	unsigned int event_bit = 0;
	int ret;

	if (event < EVENT_1HZI) {
		mask_reg = REG_INT_MASK0;
		event_mask = EVENT_MASK_0;
		event_bit = (1 << event);
		events_enabled0 |= event_bit;
	} else {
		event -= 24;
		mask_reg = REG_INT_MASK1;
		event_mask = EVENT_MASK_1;
		event_bit = (1 << event);
		events_enabled1 |= event_bit;
	}

	if ((event_bit & event_mask) == 0) {
		pr_debug("Error: unmasking a reserved/unused event\n");
		return PMIC_ERROR;
	}

	ret = pmic_write_reg(mask_reg, 0, event_bit);

	pr_debug("Enable Event : %d\n", event);

	return ret;
}

int pmic_event_mask(type_event event)
{
	unsigned int event_mask = 0;
	unsigned int mask_reg = 0;
	unsigned int event_bit = 0;
	int ret;

	if (event < EVENT_1HZI) {
		mask_reg = REG_INT_MASK0;
		event_mask = EVENT_MASK_0;
		event_bit = (1 << event);
		events_enabled0 &= ~event_bit;
	} else {
		event -= 24;
		mask_reg = REG_INT_MASK1;
		event_mask = EVENT_MASK_1;
		event_bit = (1 << event);
		events_enabled1 &= ~event_bit;
	}

	if ((event_bit & event_mask) == 0) {
		pr_debug("Error: masking a reserved/unused event\n");
		return PMIC_ERROR;
	}

	ret = pmic_write_reg(mask_reg, event_bit, event_bit);

	pr_debug("Disable Event : %d\n", event);

	return ret;
}
