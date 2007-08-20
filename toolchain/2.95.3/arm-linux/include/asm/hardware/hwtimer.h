/*
 * Code to multiplex the SA1100 Timer 
 * Copyright (c) Compaq Computer Corporation, 1999
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Author: Deborah A. Wallach
 *
 */

#ifndef _LINUX_HWTIMER_H
#define _LINUX_HWTIMER_H

struct hwtimer_list {
	struct hwtimer_list *next;
	unsigned long expires;
	unsigned long data;
	void (*function)(int, void *, struct pt_regs *, unsigned long);
};

extern void add_hwtimer(struct hwtimer_list * timer, int delay);
extern int  del_hwtimer(struct hwtimer_list * timer);
extern inline void init_hwtimer(struct hwtimer_list * timer);

#endif
