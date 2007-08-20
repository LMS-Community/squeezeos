/*
 *  linux/include/asm-arm/arch-clps711x/time.h
 *
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <asm/system.h>
#include <asm/leds.h>
#include <asm/hardware/clps7111.h>

/*
 * IRQ handler for the timer
 */
static void p720t_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	do_leds();
	do_timer(regs);
	do_profile(regs);
}

/*
 * gettimeoffset() returns time since last timer tick, in usecs.
 *
 * 'LATCH' is hwclock ticks (see CLOCK_TICK_RATE in timex.h) per jiffy.
 * 'tick' is usecs per jiffy.
 */
static unsigned long p720t_gettimeoffset(void)
{
	unsigned long hwticks;
	hwticks = LATCH - (clps_readl(TC2D) & 0xffff);	/* since last underflow */
	return (hwticks * tick) / LATCH;
}

/*
 * Set up timer interrupt, and return the current time in seconds.
 */
extern __inline__ void setup_timer(void)
{
	unsigned int syscon;

	gettimeoffset = p720t_gettimeoffset;

	syscon = clps_readl(SYSCON1);
	syscon |= SYSCON1_TC2S | SYSCON1_TC2M;
	clps_writel(syscon, SYSCON1);

	clps_writel(LATCH-1, TC2D); /* 512kHz / 100Hz - 1 */

	timer_irq.handler = p720t_timer_interrupt;

	/* 
	 * Make irqs happen for the system timer
	 */
	setup_arm_irq(IRQ_TC2OI, &timer_irq);

	xtime.tv_sec = clps_readl(RTCDR);
}
