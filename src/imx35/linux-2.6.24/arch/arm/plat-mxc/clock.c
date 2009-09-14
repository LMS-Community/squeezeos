/*
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 *  Based on arch/arm/plat-omap/clock.c
 *
 *  Copyright (C) 2004 - 2005 Nokia corporation
 *  Written by Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>
 *  Modified for omap shared clock framework by Tony Lindgren <tony@atomide.com>
 */
/*!
 * @file plat-mxc/clock.c
 *
 * @brief clk API implementation for MXC clocks.
 *
 * This file contains API defined in include/linux/clk.h for setting up and
 * retrieving clocks.
 *
 * @ingroup CLOCKS
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/semaphore.h>

#include <asm/arch/clock.h>

static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);
static DEFINE_SPINLOCK(clockfw_lock);

/*-------------------------------------------------------------------------
 * Standard clock functions defined in include/linux/clk.h
 *-------------------------------------------------------------------------*/

/*!
 * @brief Function to retrieve a clock by name.
 *
 * Note that we first try to use device id on the bus
 * and clock name. If this fails, we try to use "<name>.<id>". If this fails,
 * we try to use clock name only.
 *
 * The reference count to the clock's module owner ref count is incremented.
 *
 * @param dev 	Device structure to get bus device id.
 * @param id	Clock name string
 *
 * @return	Returns handle to clock on success or -ENOENT on failure.
 */
struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p, *clk = ERR_PTR(-ENOENT);
	int idno, len;
	char *str;

	if (id == NULL)
		return clk;

	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	mutex_lock(&clocks_mutex);

	list_for_each_entry(p, &clocks, node) {
		if (p->id == idno &&
		    strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			goto found;
		}
	}

	str = strrchr(id, '.');
	if (str) {
		len = str - id;
		str++;
		idno = simple_strtol(str, NULL, 10);
		list_for_each_entry(p, &clocks, node) {
			if (p->id == idno && (strlen(p->name) == len) &&
			    strncmp(id, p->name, len) == 0 &&
			    try_module_get(p->owner)) {
				clk = p;
				goto found;
			}
		}
	}

	list_for_each_entry(p, &clocks, node) {
		if (strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			goto found;
		}
	}

	printk(KERN_WARNING "clk: Unable to get requested clock: %s\n", id);

      found:
	mutex_unlock(&clocks_mutex);

	return clk;
}

EXPORT_SYMBOL(clk_get);

static void __clk_disable(struct clk *clk)
{
	if (clk->usecount > 0 && !(--clk->usecount)) {
		if (clk->disable != NULL) {
			clk->disable(clk);
		}
		if (likely((u32) clk->parent))
			__clk_disable(clk->parent);
		if (unlikely((u32) clk->secondary))
			__clk_disable(clk->secondary);
	}
}

static int __clk_enable(struct clk *clk)
{
	int ret = 0;

	if (clk->usecount++ == 0) {
		if (likely((u32) clk->parent)) {
			ret = __clk_enable(clk->parent);

			if (unlikely(ret != 0)) {
				goto err1;
			}
		}

		if (unlikely((u32) clk->secondary)) {
			ret = __clk_enable(clk->secondary);

			if (unlikely(ret != 0)) {
				goto err2;
			}
		}

		if (clk->enable) {
			ret = clk->enable(clk);
		}

		if (unlikely(ret != 0)) {
			goto err3;
		}
	}
	return 0;

      err3:
	if (clk->secondary)
		__clk_disable(clk->secondary);
      err2:
	if (clk->parent)
		__clk_disable(clk->parent);
      err1:
	clk->usecount--;
	return ret;
}

/*!
 * @brief Function to enable a clock.
 *
 * This function increments the reference count on the clock and enables the
 * clock if not already enabled. The parent clock tree is recursively enabled.
 *
 * @param clk 	Handle to clock to enable.
 *
 * @return	Returns 0 on success or error code on failure.
 */
int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret = 0;

	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	spin_lock_irqsave(&clockfw_lock, flags);

	ret = __clk_enable(clk);

	spin_unlock_irqrestore(&clockfw_lock, flags);

	return ret;
}

EXPORT_SYMBOL(clk_enable);

/*!
 * @brief Function to disable a clock.
 *
 * This function decrements the reference count on the clock and disables the
 * clock when reference count is 0. The parent clock tree is recursively
 * disabled.
 *
 * @param clk 	Handle to clock to disable.
 *
 */
void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return;

	spin_lock_irqsave(&clockfw_lock, flags);

	__clk_disable(clk);

	spin_unlock_irqrestore(&clockfw_lock, flags);
}

EXPORT_SYMBOL(clk_disable);

/*!
 * @brief Function to get the usage count for the requested clock.
 *
 * This function returns the reference count for the clock.
 *
 * @param clk 	Handle to clock to disable.
 *
 * @return Returns the usage count for the requested clock.
 */
int clk_get_usecount(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return 0;

	return clk->usecount;
}

EXPORT_SYMBOL(clk_get_usecount);

/*!
 * @brief Function to retrieve the clock rate for a clock.
 *
 * @param clk 	Handle to clock to retrieve.
 *
 * @return	Returns the clock's rate in Hz.
 *
 */
unsigned long clk_get_rate(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return 0;

	return clk->rate;
}

EXPORT_SYMBOL(clk_get_rate);

/*!
 * @brief Function to decrement the clock's module reference count.
 *
 * @param clk 	Handle to clock to put.
 *
 */
void clk_put(struct clk *clk)
{
	if (clk && !IS_ERR(clk))
		module_put(clk->owner);
}

EXPORT_SYMBOL(clk_put);

/*!
 * @brief Function to round the requested clock rate to the nearest supported
 * rate that is less than or equal to the requested rate. This is dependent on
 * the clock's current parent.
 *
 * @param clk 	Handle to clock to retrieve.
 * @param rate	Desired clock rate in Hz.
 *
 * @return	Returns the nearest supported rate in Hz.
 *
 */
long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || IS_ERR(clk) || !clk->round_rate)
		return 0;

	return clk->round_rate(clk, rate);
}

EXPORT_SYMBOL(clk_round_rate);

/* Propagate rate to children */
void propagate_rate(struct clk *tclk)
{
	struct clk *clkp;

	if (tclk == NULL || IS_ERR(tclk))
		return;

	pr_debug("mxc clock: finding children of %s-%d\n", tclk->name,
		 tclk->id);
	list_for_each_entry(clkp, &clocks, node) {
		if (likely(clkp->parent != tclk))
			continue;
		pr_debug("mxc clock: %s-%d: recalculating rate: old = %lu, ",
			 clkp->name, clkp->id, clkp->rate);
		if (likely((u32) clkp->recalc))
			clkp->recalc(clkp);
		else
			clkp->rate = tclk->rate;
		pr_debug("new = %lu\n", clkp->rate);
		propagate_rate(clkp);
	}
}

/*!
 * @brief Function to set the clock to the requested clock rate. The rate must
 * match a supported rate exactly based on what clk_round_rate returns.
 *
 * @param clk 	Handle to clock to retrieve.
 * @param rate	Desired clock rate in Hz.
 *
 * @return	Returns 0 on success, negative error code on failure.
 *
 */
int clk_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long flags;
	int ret = -EINVAL;

	if (clk == NULL || IS_ERR(clk) || clk->set_rate == NULL || rate == 0)
		return ret;

	spin_lock_irqsave(&clockfw_lock, flags);

	ret = clk->set_rate(clk, rate);
	if (unlikely((ret == 0) && (clk->flags & RATE_PROPAGATES)))
		propagate_rate(clk);

	spin_unlock_irqrestore(&clockfw_lock, flags);

	return ret;
}

EXPORT_SYMBOL(clk_set_rate);

/*!
 * @brief Function to set the clock's parent to another clock source.
 *
 * @param clk 	Handle to clock to retrieve.
 * @param parent Desired parent clock input for the clock.
 *
 * @return	Returns 0 on success, negative error code on failure.
 *
 */
int clk_set_parent(struct clk *clk, struct clk *parent)
{
	unsigned long flags;
	int ret = -EINVAL;

	if (clk == NULL || IS_ERR(clk) || parent == NULL ||
	    IS_ERR(parent) || clk->set_parent == NULL)
		return ret;

	spin_lock_irqsave(&clockfw_lock, flags);
	ret = clk->set_parent(clk, parent);
	if (ret == 0) {
		clk->parent = parent;
		if (clk->recalc) {
			clk->recalc(clk);
		} else {
			clk->rate = parent->rate;
		}
		if (unlikely(clk->flags & RATE_PROPAGATES))
			propagate_rate(clk);
	}
	spin_unlock_irqrestore(&clockfw_lock, flags);

	return ret;
}

EXPORT_SYMBOL(clk_set_parent);

/*!
 * @brief Function to retrieve the clock's parent clock source.
 *
 * @param clk 	Handle to clock to retrieve.
 *
 * @return	Returns parent clk on success, NULL on failure.
 *
 */
struct clk *clk_get_parent(struct clk *clk)
{
	struct clk *ret = NULL;

	if (clk == NULL || IS_ERR(clk))
		return ret;

	return clk->parent;
}

EXPORT_SYMBOL(clk_get_parent);

/*!
 * @brief Function to add a new clock to the clock tree.
 *
 * @param clk 	Handle to clock to add.
 *
 * @return	Returns 0 on success, negative error code on failure.
 *
 */
int clk_register(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	mutex_lock(&clocks_mutex);
	list_add(&clk->node, &clocks);
	mutex_unlock(&clocks_mutex);

	return 0;
}

EXPORT_SYMBOL(clk_register);

/*!
 * @brief Function to remove a clock from the clock tree.
 *
 * @param clk 	Handle to clock to add.
 *
 */
void clk_unregister(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return;

	mutex_lock(&clocks_mutex);
	list_del(&clk->node);
	mutex_unlock(&clocks_mutex);
}

EXPORT_SYMBOL(clk_unregister);

void mxc_dump_clocks(void)
{
	struct clk *clkp;
	list_for_each_entry(clkp, &clocks, node) {
		printk("name:\t%s-%d\n", clkp->name, clkp->id);
		printk("count:\t%d\n", clkp->usecount);
		printk("rate:\t%lu\n", clkp->rate);
		if (clkp->parent)
			printk("parent:\t%s-%d\n\n", clkp->parent->name,
			       clkp->parent->id);
		else
			printk("parent:\tnone\n\n");
	}
}

#ifdef CONFIG_PROC_FS
static int mxc_clock_read_proc(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{
	struct clk *clkp;
	char *p = page;
	int len;

	list_for_each_entry(clkp, &clocks, node) {
		p += sprintf(p, "%s-%d:\t\t%lu, %d",
			     clkp->name, clkp->id, clkp->rate, clkp->usecount);
		if (clkp->parent)
			p += sprintf(p, ", %s-%d\n", clkp->parent->name,
				     clkp->parent->id);
		else
			p += sprintf(p, "\n");
	}

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int __init mxc_setup_proc_entry(void)
{
	struct proc_dir_entry *res;

	res = create_proc_read_entry("cpu/clocks", 0, NULL,
				     mxc_clock_read_proc, NULL);
	if (!res) {
		printk(KERN_ERR "Failed to create proc/cpu/clocks\n");
		return -ENOMEM;
	}
	return 0;
}

late_initcall(mxc_setup_proc_entry);
#endif
