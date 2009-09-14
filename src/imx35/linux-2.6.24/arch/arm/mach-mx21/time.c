/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/* System Timer Interrupt reconfigured to run in free-run mode.
 * Author: Vitaly Wool
 * Copyright 2004 MontaVista Software Inc.
 */

/*!
 * @file time.c
 * @brief This file contains OS tick implementations.
 *
 * This file contains OS tick implementations.
 *
 * @ingroup Timers
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach/time.h>

#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>

#ifndef __noinstrument
#define __noinstrument
#endif

/* OS tick defines */
#define MXC_GPT_INT_TICK		INT_GPT
#define MXC_GPT_CLOCK_TICK		GPT1_CLK
#define MXC_GPT_TCMP_TICK		MXC_GPT_TCMP(MXC_TIMER_GPT1)
#define MXC_GPT_TSTAT_TICK		MXC_GPT_TSTAT(MXC_TIMER_GPT1)
#define MXC_GPT_TCTL_TICK		MXC_GPT_TCTL(MXC_TIMER_GPT1)
#define MXC_GPT_TPRER_TICK		MXC_GPT_TPRER(MXC_TIMER_GPT1)
#define MXC_GPT_TCN_TICK		MXC_GPT_TCN(MXC_TIMER_GPT1)
/* High resolution timer defines */
#define MXC_GPT_INT_HRT			INT_GPT2
#define MXC_GPT_CLOCK_HRT		GPT2_CLK
#define MXC_GPT_TCMP_HRT		MXC_GPT_TCMP(MXC_TIMER_GPT2)
#define MXC_GPT_TSTAT_HRT		MXC_GPT_TSTAT(MXC_TIMER_GPT2)
#define MXC_GPT_TCTL_HRT		MXC_GPT_TCTL(MXC_TIMER_GPT2)
#define MXC_GPT_TPRER_HRT		MXC_GPT_TPRER(MXC_TIMER_GPT2)
#define MXC_GPT_TCN_HRT			MXC_GPT_TCN(MXC_TIMER_GPT2)

/*!
 * This is the timer interrupt service routine to do required tasks.
 *
 * @param  irq          GPT interrupt source number (not used)
 * @param  dev_id       this parameter is not used
 * @param  regs         pointer to a structure containing the processor
 *                      registers and state prior to servicing the interrupt
 * @return always returns \b IRQ_HANDLED as defined in
 *         include/linux/interrupt.h.
 */
static irqreturn_t mxc_timer_interrupt(int irq, void *dev_id,
				       struct pt_regs *regs)
{
	unsigned int next_match;

	write_seqlock(&xtime_lock);

	do {
		timer_tick(regs);
		next_match = __raw_readl(MXC_GPT_TCMP_TICK) + LATCH;
		__raw_writel(GPT_TSTAT_COMP, MXC_GPT_TSTAT_TICK);
		__raw_writel(next_match, MXC_GPT_TCMP_TICK);
	} while ((signed long)(next_match - __raw_readl(MXC_GPT_TCN_TICK)) <=
		 0);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

/*!
 * This function is used to obtain the number of microseconds since the last
 * timer interrupt. Note that interrupts is disabled by do_gettimeofday().
 *
 * @return the number of microseconds since the last timer interrupt.
 */
static unsigned long __noinstrument mxc_gettimeoffset(void)
{
	long ticks_to_match, elapsed, usec;

	/* Get ticks before next timer match */
	ticks_to_match =
	    __raw_readl(MXC_GPT_TCMP_TICK) - __raw_readl(MXC_GPT_TCN_TICK);

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed * (tick_nsec / 1000)) / LATCH;

	return usec;
}

/*!
 * The OS tick timer interrupt structure.
 */
static struct irqaction timer_irq = {
	.name = "MXC Timer Tick",
	.flags = IRQF_DISABLED | IRQF_TIMER,
	.handler = mxc_timer_interrupt
};

/*!
 * This function is used to initialize the GPT to produce an interrupt
 * every 10 msec. It is called by the start_kernel() during system startup.
 */
void __init mxc_init_time(void)
{
	u32 reg, v;

	mxc_clks_enable(MXC_GPT_CLOCK_TICK);
	__raw_writel(0, MXC_GPT_TCTL_TICK);
	__raw_writel(GPT_TCTL_SWR, MXC_GPT_TCTL_TICK);

	while ((__raw_readl(MXC_GPT_TCTL_TICK) & GPT_TCTL_SWR) != 0)
		mb();

	reg = GPT_TCTL_FRR | GPT_TCTL_COMPEN | GPT_TCTL_SRC_PER1;

	__raw_writel(reg, MXC_GPT_TCTL_TICK);

	v = mxc_get_clocks(MXC_GPT_CLOCK_TICK);
	__raw_writel((v / CLOCK_TICK_RATE) - 1, MXC_GPT_TPRER_TICK);

	if ((v % CLOCK_TICK_RATE) != 0) {
		pr_info("\nWARNING: Can't generate CLOCK_TICK_RATE at %d Hz\n",
			CLOCK_TICK_RATE);
	}
	pr_info("Actual CLOCK_TICK_RATE is %d Hz\n",
		v / ((__raw_readl(MXC_GPT_TPRER_TICK) & 0x7FF) + 1));

	reg = __raw_readl(MXC_GPT_TCN_TICK);
	reg += LATCH;
	__raw_writel(reg, MXC_GPT_TCMP_TICK);

	setup_irq(MXC_GPT_INT_TICK, &timer_irq);

	reg = __raw_readl(MXC_GPT_TCTL_TICK) | GPT_TCTL_TEN;
	__raw_writel(reg, MXC_GPT_TCTL_TICK);

#ifdef CONFIG_KFI
	os_timer_initialized = 1;
#endif
}

struct sys_timer mxc_timer = {
	.init = mxc_init_time,
	.offset = mxc_gettimeoffset,
};
