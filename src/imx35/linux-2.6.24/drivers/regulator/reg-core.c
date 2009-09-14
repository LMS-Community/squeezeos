/*
 * regulator.c  --  Voltage/Current Regulator framework.
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <liam.girdwood@wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/regulator/regulator.h>
#include <linux/regulator/regulator-drv.h>
#include <linux/regulator/regulator-platform.h>

static DEFINE_MUTEX(list_mutex);
static LIST_HEAD(regulators);

struct regulator_load {
	struct device *dev;
	struct list_head list;
	int uA_load;
	struct device_attribute dev_attr;
};

#define to_regulator(cd) \
	container_of(cd, struct regulator, cdev)

static struct regulator_load *get_regulator_load(struct device *dev)
{
	struct regulator_load *load = NULL;
	struct regulator *regulator;

	list_for_each_entry(regulator, &regulators, list) {
		list_for_each_entry(load, &regulator->user_list, list) {
			if (load->dev == dev)
				return load;
		}
	}
	return NULL;
}

#if 0
static int constraint_check_state(struct regulator *regulator)
{
	if (!regulator->constraints)
		return -ENODEV;
	if (!regulator->constraints->valid_ops_mask & REGULATOR_CHANGE_STATUS)
		return -EPERM;
	return 0;
}
#endif

static int constraint_check_voltage(struct regulator *regulator, int uV)
{
	if (!regulator->constraints)
		return -ENODEV;
	if (!regulator->constraints->valid_ops_mask & REGULATOR_CHANGE_VOLTAGE)
		return -EPERM;
	if (uV > regulator->constraints->max_uV ||
	    uV < regulator->constraints->min_uV)
		return -EINVAL;
	return 0;
}

static int constraint_check_current(struct regulator *regulator, int uA)
{
	if (!regulator->constraints)
		return -ENODEV;
	if (!regulator->constraints->valid_ops_mask & REGULATOR_CHANGE_CURRENT)
		return -EPERM;
	if (uA > regulator->constraints->max_uA ||
	    uA < regulator->constraints->min_uA)
		return -EINVAL;
	return 0;
}

static int constraint_check_mode(struct regulator *regulator, int mode)
{
	if (!regulator->constraints)
		return -ENODEV;
	if (!regulator->constraints->valid_ops_mask & REGULATOR_CHANGE_MODE)
		return -EPERM;
	if (!regulator->constraints->valid_modes_mask & mode)
		return -EINVAL;
	return 0;
}

static int constraint_check_drms(struct regulator *regulator)
{
	if (!regulator->constraints)
		return -ENODEV;
	if (!regulator->constraints->valid_ops_mask & REGULATOR_CHANGE_DRMS)
		return -EPERM;
	return 0;
}

static ssize_t dev_load_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct regulator_load *load;

	load = get_regulator_load(dev);
	if (load == NULL)
		return 0;

	return sprintf(buf, "%d\n", load->uA_load);
}

static ssize_t regulator_uV_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);

	return sprintf(buf, "%d\n", regulator_get_voltage(regulator));
}

static ssize_t regulator_uV_store(struct class_device *cdev,
				  const char *buf, size_t count)
{
	struct regulator *regulator = to_regulator(cdev);
	int mV, ret;

	mV = simple_strtoul(buf, NULL, 10);

	ret = regulator_set_voltage(regulator, mV * 1000);
	if (ret == 0)
		printk(KERN_INFO "set voltage %d mV\n", mV);
	else
		printk(KERN_INFO "set voltage failed\n");

	return count;
}

static ssize_t regulator_uA_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);

	return sprintf(buf, "%d\n", regulator_get_current(regulator));
}

static ssize_t regulator_mode_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);
	int mode = regulator_get_mode(regulator);

	switch (mode) {
	case REGULATOR_MODE_FAST:
		return sprintf(buf, "fast\n");
	case REGULATOR_MODE_NORMAL:
		return sprintf(buf, "normal\n");
	case REGULATOR_MODE_IDLE:
		return sprintf(buf, "idle\n");
	case REGULATOR_MODE_STANDBY:
		return sprintf(buf, "standby\n");
	}
	return sprintf(buf, "unknown\n");
}

static ssize_t regulator_state_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);
	int state = regulator_is_enabled(regulator);

	if (state > 0)
		return sprintf(buf, "enabled\n");
	else if (state == 0)
		return sprintf(buf, "disabled\n");
	else
		return sprintf(buf, "unknown\n");
}

static ssize_t
regulator_constraint_uA_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);

	if (!regulator->constraints)
		return sprintf(buf, "no constraints\n");

	return sprintf(buf, "%d %d\n", regulator->constraints->min_uA,
		       regulator->constraints->max_uA);
}

static ssize_t
regulator_constraint_uV_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);

	if (!regulator->constraints)
		return sprintf(buf, "no constraints\n");

	return sprintf(buf, "%d %d\n", regulator->constraints->min_uV,
		       regulator->constraints->max_uV);
}

static ssize_t
regulator_constraint_modes_show(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);
	int count = 0;

	if (!regulator->constraints)
		return sprintf(buf, "no constraints\n");

	if (regulator->constraints->valid_modes_mask & REGULATOR_MODE_FAST)
		count = sprintf(buf, "fast ");
	if (regulator->constraints->valid_modes_mask & REGULATOR_MODE_NORMAL)
		count += sprintf(buf + count, "normal ");
	if (regulator->constraints->valid_modes_mask & REGULATOR_MODE_IDLE)
		count += sprintf(buf + count, "idle ");
	if (regulator->constraints->valid_modes_mask & REGULATOR_MODE_STANDBY)
		count += sprintf(buf + count, "standby");
	count += sprintf(buf + count, "\n");
	return count;
}

static ssize_t regulator_total_dev_load(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);
	struct regulator_load *load;
	int uA = 0;

	list_for_each_entry(load, &regulator->user_list, list) {
		uA += load->uA_load;
	}
	return sprintf(buf, "%d\n", uA);
}

static ssize_t regulator_enabled_use_count(struct class_device *cdev, char *buf)
{
	struct regulator *regulator = to_regulator(cdev);
	return sprintf(buf, "%d\n", regulator->use_count);
}

static ssize_t regulator_ctl(struct class_device *cdev,
			     const char *buf, size_t count)
{
	struct regulator *regulator = to_regulator(cdev);
	if (buf[0] == '0') {
		printk(KERN_WARNING "disable regulator.\n");
		if (regulator_disable(regulator))
			printk(KERN_ERR "disable regulator failed.\n");
	} else {
		printk(KERN_WARNING "enable regulator.\n");
		if (regulator_enable(regulator))
			printk(KERN_ERR "enable regulator failed.\n");
	}
	return count;
}

static struct class_device_attribute regulator_dev_attrs[] = {
	__ATTR(uV, 0666, regulator_uV_show, regulator_uV_store),
	__ATTR(uA, 0444, regulator_uA_show, NULL),
	__ATTR(mode, 0444, regulator_mode_show, NULL),
	__ATTR(state, 0444, regulator_state_show, NULL),
	__ATTR(uA_limits, 0444, regulator_constraint_uA_show, NULL),
	__ATTR(uV_limits, 0444, regulator_constraint_uV_show, NULL),
	__ATTR(valid_modes, 0444, regulator_constraint_modes_show, NULL),
	__ATTR(total_uA_load, 0444, regulator_total_dev_load, NULL),
	__ATTR(enabled_count, 0444, regulator_enabled_use_count, NULL),
	__ATTR(ctl, 0666, NULL, regulator_ctl),
	__ATTR_NULL,
};

static void regulator_dev_release(struct class_device *class_dev)
{
}

struct class regulator_class = {
	.name = "regulator",
	.release = regulator_dev_release,
	.class_dev_attrs = regulator_dev_attrs,
};

static struct regulator_load *create_load_dev(struct regulator *regulator,
					      struct device *dev)
{
	struct regulator_load *load;
	char buf[32];
	int err;

	load = kzalloc(sizeof(*load), GFP_KERNEL);
	if (load == NULL)
		return NULL;

	sprintf(buf, "uA_load-%s", regulator->name);
	list_add(&load->list, &regulator->user_list);
	load->dev = dev;
	load->dev_attr.attr.name = kstrdup(buf, GFP_KERNEL);
	load->dev_attr.attr.owner = THIS_MODULE;
	load->dev_attr.attr.mode = 0444;
	load->dev_attr.show = dev_load_show;
	err = device_create_file(dev, &load->dev_attr);
	if (err < 0) {
		printk(KERN_WARNING "%s: could not add regulator load"
		       " sysfs\n", __func__);
		goto err_out;
	}
	err = sysfs_create_link(&regulator->cdev.kobj, &dev->kobj,
				kobject_name(&dev->kobj));
	if (err) {
		printk
		    (KERN_WARNING "%s : could not add device link %s err %d\n",
		     __func__, kobject_name(&dev->kobj), err);
		goto err_out;
	}
	return load;
      err_out:
	kfree(load->dev_attr.attr.name);
	kfree(load);
	return NULL;
}

struct regulator *regulator_get(struct device *dev, const char *id)
{
	struct regulator *r, *regulator = ERR_PTR(-ENOENT);
	struct regulator_load *load = NULL;

	if (id == NULL)
		return regulator;

	mutex_lock(&list_mutex);

	list_for_each_entry(r, &regulators, list) {
		if (strcmp(id, r->name) == 0 && try_module_get(r->owner)) {
			regulator = r;
			goto found;
		}
	}
	printk
	    (KERN_WARNING "regulator: Unable to get requested regulator: %s\n",
	     id);
	mutex_unlock(&list_mutex);
	return regulator;
      found:
	if (dev) {
		load = create_load_dev(regulator, dev);
		if (load == NULL) {
			regulator = ERR_PTR(-ENOMEM);
			module_put(regulator->owner);
		}
	}

	mutex_unlock(&list_mutex);
	return regulator;
}

EXPORT_SYMBOL_GPL(regulator_get);

void regulator_put(struct regulator *regulator, struct device *dev)
{
	struct regulator_load *load, *l;

	if (regulator == NULL || IS_ERR(regulator))
		return;

	if (!dev)
		goto put;

	sysfs_remove_link(&regulator->cdev.kobj, kobject_name(&dev->kobj));
	list_for_each_entry_safe(load, l, &regulator->user_list, list) {
		device_remove_file(dev, &load->dev_attr);
		list_del(&load->list);
		kfree(load->dev_attr.attr.name);
		kfree(load);
		goto put;
	}
      put:
	module_put(regulator->owner);
	mutex_unlock(&list_mutex);
}

EXPORT_SYMBOL_GPL(regulator_put);

static int __regulator_disable(struct regulator *regulator);

static int __regulator_enable(struct regulator *regulator)
{
	int ret = 0;

	if (regulator->use_count == 0) {

		if (regulator->parent) {
			ret = __regulator_enable(regulator->parent);

			if (ret < 0) {
				__regulator_disable(regulator->parent);
				goto out;
			}
		}

		if (regulator->ops->enable) {
			ret = regulator->ops->enable(regulator);
			if (ret < 0) {
				__regulator_disable(regulator);
				goto out;
			}
		}
	}
	regulator->use_count++;
      out:
	return ret;
}

int regulator_enable(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);
	ret = __regulator_enable(regulator);
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_enable);

static int __regulator_disable(struct regulator *regulator)
{
	int ret = 0;

	if (regulator->use_count > 0 && !(--regulator->use_count)) {
		if (regulator->ops->disable) {
			ret = regulator->ops->disable(regulator);
			if (ret < 0)
				goto out;
		}
		if (regulator->parent)
			__regulator_disable(regulator->parent);
	}
      out:
	return ret;
}

int regulator_disable(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);
	ret = __regulator_disable(regulator);
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_disable);

int regulator_is_enabled(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->is_enabled) {
		ret = -EINVAL;
		goto out;
	}

	ret = regulator->ops->is_enabled(regulator);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_is_enabled);

int regulator_set_voltage(struct regulator *regulator, int uV)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->set_voltage) {
		ret = -EINVAL;
		goto out;
	}

	/* constraints check */
	ret = constraint_check_voltage(regulator, uV);
	if (ret < 0)
		goto out;

	ret = regulator->ops->set_voltage(regulator, uV);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_set_voltage);

int regulator_get_voltage(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->get_voltage) {
		ret = -EINVAL;
		goto out;
	}

	ret = regulator->ops->get_voltage(regulator);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_get_voltage);

int regulator_set_current(struct regulator *regulator, int uA)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->set_current) {
		ret = -EINVAL;
		goto out;
	}

	/* constraints check */
	ret = constraint_check_current(regulator, uA);
	if (ret < 0)
		goto out;

	ret = regulator->ops->set_current(regulator, uA);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_set_current);

int regulator_get_current(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->get_current) {
		ret = -EINVAL;
		goto out;
	}

	ret = regulator->ops->get_current(regulator);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_get_current);

int regulator_set_mode(struct regulator *regulator, unsigned int mode)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->set_mode) {
		ret = -EINVAL;
		goto out;
	}

	/* constraints check */
	ret = constraint_check_mode(regulator, mode);
	if (ret < 0)
		goto out;

	ret = regulator->ops->set_mode(regulator, mode);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_set_mode);

unsigned int regulator_get_mode(struct regulator *regulator)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->get_mode) {
		ret = -EINVAL;
		goto out;
	}

	ret = regulator->ops->get_mode(regulator);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_get_mode);

unsigned int regulator_get_optimum_mode(struct regulator *regulator,
					int input_uV, int output_uV,
					int load_uA)
{
	int ret;

	mutex_lock(&regulator->mutex);

	/* sanity check */
	if (!regulator->ops->get_optimum_mode) {
		ret = -EINVAL;
		goto out;
	}

	ret = regulator->ops->get_optimum_mode(regulator,
					       input_uV, output_uV, load_uA);
      out:
	mutex_unlock(&regulator->mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_get_optimum_mode);

int regulator_register_client(struct regulator *regulator,
			      struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&regulator->notifier, nb);
}

EXPORT_SYMBOL_GPL(regulator_register_client);

int regulator_unregister_client(struct regulator *regulator,
				struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&regulator->notifier, nb);
}

EXPORT_SYMBOL_GPL(regulator_unregister_client);

int regulator_notifier_call_chain(struct regulator *regulator,
				  unsigned long event, void *data)
{
	return blocking_notifier_call_chain(&regulator->notifier, event, data);
}

EXPORT_SYMBOL_GPL(regulator_notifier_call_chain);

int regulator_register(struct regulator *regulator)
{
	static atomic_t regulator_no = ATOMIC_INIT(0);
	int ret;

	if (regulator == NULL || IS_ERR(regulator))
		return -EINVAL;

	if (regulator->name == NULL || regulator->ops == NULL)
		return -EINVAL;

	mutex_lock(&list_mutex);

	mutex_init(&regulator->mutex);
	regulator->parent = NULL;
	INIT_LIST_HEAD(&regulator->user_list);
	BLOCKING_INIT_NOTIFIER_HEAD(&regulator->notifier);

	regulator->cdev.class = &regulator_class;
	class_device_initialize(&regulator->cdev);
	snprintf(regulator->cdev.class_id, sizeof(regulator->cdev.class_id),
		 "regulator-%ld-%s",
		 (unsigned long)atomic_inc_return(&regulator_no) - 1,
		 regulator->name);

	ret = class_device_add(&regulator->cdev);
	if (ret == 0)
		list_add(&regulator->list, &regulators);
	mutex_unlock(&list_mutex);
	return ret;
}

EXPORT_SYMBOL_GPL(regulator_register);

void regulator_unregister(struct regulator *regulator)
{
	if (regulator == NULL || IS_ERR(regulator))
		return;

	mutex_lock(&list_mutex);
	list_del(&regulator->list);
	if (regulator->parent)
		sysfs_remove_link(&regulator->cdev.kobj, "source");
	class_device_unregister(&regulator->cdev);
	mutex_unlock(&list_mutex);
}

EXPORT_SYMBOL_GPL(regulator_unregister);

int regulator_set_platform_source(struct regulator *regulator,
				  struct regulator *parent)
{
	int err;

	if (regulator == NULL || IS_ERR(regulator))
		return -EINVAL;

	if (parent == NULL || IS_ERR(parent))
		return -EINVAL;

	mutex_lock(&list_mutex);
	regulator->parent = parent;
	err = sysfs_create_link(&regulator->cdev.kobj, &parent->cdev.kobj,
				"source");
	if (err)
		printk(KERN_WARNING
		       "%s : could not add device link %s err %d\n", __func__,
		       kobject_name(&parent->cdev.kobj), err);
	mutex_unlock(&list_mutex);

	return 0;
}

EXPORT_SYMBOL_GPL(regulator_set_platform_source);

struct regulator *regulator_get_platform_source(struct regulator *regulator)
{
	if (regulator == NULL || IS_ERR(regulator))
		return ERR_PTR(-EINVAL);
	return regulator->parent;
}

EXPORT_SYMBOL_GPL(regulator_get_platform_source);

int regulator_set_platform_constraints(const char *regulator_name,
				       struct regulation_constraints
				       *constraints)
{
	struct regulator *regulator;

	if (regulator_name == NULL)
		return -EINVAL;

	mutex_lock(&list_mutex);
	list_for_each_entry(regulator, &regulators, list) {
		if (!strcmp(regulator_name, regulator->name)) {
			mutex_lock(&regulator->mutex);
			regulator->constraints = constraints;
			mutex_unlock(&regulator->mutex);
			mutex_unlock(&list_mutex);
			return 0;
		}
	}
	mutex_unlock(&list_mutex);
	return -ENODEV;
}

EXPORT_SYMBOL_GPL(regulator_set_platform_constraints);

static void load_change(struct regulator *regulator,
			struct regulator_load *load, int uA)
{
	struct regulator_load *l;
	int current_uA = 0, output_uV, input_uV, err;
	unsigned int mode;

	load->uA_load = uA;
	err = constraint_check_drms(regulator);
	if (err < 0 || !regulator->ops->get_optimum_mode ||
	    !regulator->ops->get_voltage || !regulator->ops->set_mode)
		return;

	/* get output voltage */
	output_uV = regulator->ops->get_voltage(regulator);

	/* get input voltage */
	if (regulator->parent && regulator->parent->ops->get_voltage)
		input_uV =
		    regulator->parent->ops->get_voltage(regulator->parent);
	else
		input_uV = regulator->constraints->input_uV;

	/* calc total requested load */
	list_for_each_entry(l, &regulator->user_list, list)
	    current_uA += l->uA_load;

	mode = regulator->ops->get_optimum_mode(regulator, input_uV,
						output_uV, current_uA);
	err = constraint_check_mode(regulator, mode);
	if (err < 0)
		return;

	regulator->ops->set_mode(regulator, mode);
}

void regulator_drms_notify_load(struct regulator *regulator,
				struct device *dev, int uA)
{
	struct regulator_load *load;

	mutex_lock(&regulator->mutex);
	list_for_each_entry(load, &regulator->user_list, list) {
		if (load->dev == dev) {
			load_change(regulator, load, uA);
			break;
		}
	}
	mutex_unlock(&regulator->mutex);
}

EXPORT_SYMBOL_GPL(regulator_drms_notify_load);

void *regulator_get_drvdata(struct regulator *regulator)
{
	return regulator->reg_data;
}

EXPORT_SYMBOL_GPL(regulator_get_drvdata);

void regulator_set_drvdata(struct regulator *regulator, void *data)
{
	regulator->reg_data = data;
}

EXPORT_SYMBOL_GPL(regulator_set_drvdata);

static int __init regulator_init(void)
{
	return class_register(&regulator_class);
}

static void __exit regulator_exit(void)
{
	class_unregister(&regulator_class);
}

subsys_initcall(regulator_init);
module_exit(regulator_exit);

MODULE_AUTHOR("Liam Girdwood, liam.girdwood@wolfsonmicro.com, \
    www.wolfsonmicro.com ");
MODULE_DESCRIPTION("Regulator Interface");
MODULE_LICENSE("GPL");
