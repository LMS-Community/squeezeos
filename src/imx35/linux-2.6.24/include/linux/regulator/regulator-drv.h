/*
 * regulator-drv.h -- SoC Regulator support.
 *
 * Copyright (C) 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lg@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Regulator Driver Interface.
 */


#ifndef __LINUX_REGULATOR_DRV_H_
#define __LINUX_REGULATOR_DRV_H_

#include <linux/device.h>
#include <linux/regulator/regulator.h>

struct regulator_constraints;

/**
 * struct regulator_ops - regulator operations.
 *
 * This struct describes regulator operations.
 */
struct regulator_ops {

	/* get/set regulator voltage */
	int (*set_voltage)(struct regulator *, int uV);
	int (*get_voltage)(struct regulator *);

	/* get/set regulator current  */
	int (*set_current)(struct regulator *, int uA);
	int (*get_current)(struct regulator *);

	/* enable/disable regulator */
	int (*enable)(struct regulator *);
	int (*disable)(struct regulator *);
	int (*is_enabled)(struct regulator *);

	/* get/set regulator mode (defined in regulator.h) */
	int (*set_mode)(struct regulator *, unsigned int mode);
	unsigned int (*get_mode)(struct regulator *);

	/* get most efficient regulator mode for load */
	unsigned int (*get_optimum_mode)(struct regulator *, int input_uV,
		int output_uV, int load_uA);
};

/**
 * struct regulator
 *
 * Voltage / Current regulator.
 */
struct regulator {
	const char *name;
	int id;
	struct regulator_ops *ops;
	struct regulation_constraints *constraints;
	int use_count;

	struct list_head list;
	struct list_head user_list;
	struct blocking_notifier_head notifier;
	struct mutex mutex;
	struct module *owner;
	struct class_device cdev;

	struct regulator *parent; /* for tree */

	void *reg_data; /* regulator data */
	void *vendor; /* regulator vendor extensions */
};

/**
 * regulator_register - register regulator
 * @regulator: regulator source
 *
 * Called by regulator drivers to register a regulator.
 * Returns 0 on success.
 */
int regulator_register(struct regulator *regulator);

/**
 * regulator_unregister - unregister regulator
 * @regulator: regulator source
 *
 * Called by regulator drivers to unregister a regulator.
 */
void regulator_unregister(struct regulator *regulator);

/**
 * regulator_notifier_call_chain - call regulator event notifier
 * @regulator: regulator source
 * @event: notifier block
 * @data:
 *
 * Called by regulator drivers to notify clients a regulator event has
 * occurred.
 */
int regulator_notifier_call_chain(struct regulator *regulator,
	unsigned long event, void *data);

#endif
