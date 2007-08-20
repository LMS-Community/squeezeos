/*
 *  linux/include/linux/cpufreq.h
 *
 *  Copyright (C) 2001 Russell King
 *
 * $Id: cpufreq.h 1763 2006-09-22 01:22:23Z dean $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LINUX_CPUFREQ_H
#define _LINUX_CPUFREQ_H

#include <linux/config.h>
#include <linux/notifier.h>

#ifndef CONFIG_SMP
#define cpufreq_current(cpu)	((void)(cpu), __cpufreq_cur)
#define cpufreq_max(cpu)	((void)(cpu), __cpufreq_max)
#else
/*
 * Should be something like:
 *
 * typedef struct {
 *	u_int current;
 *	u_int max;
 *	u_int min;
 * } __cacheline_aligned cpufreq_info_t;
 *
 * static cpufreq_info_t cpufreq_info;
 *
 * #define cpufreq_current(cpu)	(cpufreq_info[cpu].current)
 * #define cpufreq_max(cpu)	(cpufreq_info[cpu].max)
 *
 * Maybe we should find some other per-cpu structure to
 * bury this in?
 */
#error fill in SMP version
#endif

struct cpufreq_info {
	unsigned int old_freq;
	unsigned int new_freq;
};

/*
 * The max and min frequency rates that the registered device
 * can tolerate.  Never reference this structure directly -
 * always use cpu_updateminmax.
 */
struct cpufreq_minmax {
	unsigned int min;
	unsigned int max;
};

static inline
void cpufreq_updateminmax(void *arg, unsigned int min, unsigned int max)
{
	struct cpufreq_minmax *minmax = arg;

	if (minmax->min < min)
		minmax->min = min;
	if (minmax->max > max)
		minmax->max = max;
}

#define CPUFREQ_MINMAX		(0)
#define CPUFREQ_PRECHANGE	(1)
#define CPUFREQ_POSTCHANGE	(2)

int cpufreq_register_notifier(struct notifier_block *nb);
int cpufreq_unregister_notifier(struct notifier_block *nb);

int cpufreq_setmax(void);
int cpufreq_set(unsigned int khz);
unsigned int cpufreq_get(int cpu);
void cpufreq_init(unsigned int khz);

/*
 * The following are the architecture specific
 * parts of the cpu frequency subsystem.  Note that
 * cpu_validatespeed may not return an error.
 */
unsigned int cpufreq_validatespeed(unsigned int khz);
void cpufreq_setspeed(unsigned int khz);

#endif
