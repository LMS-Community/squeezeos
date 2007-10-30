/*
 *  linux/include/linux/l3/algo-bit.h
 *
 *  Copyright (C) 2001 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * L3 Bus bit-banging algorithm.  Derived from i2c-algo-bit.h by
 * Simon G. Vogl.
 */
#ifndef L3_ALGO_BIT_H
#define L3_ALGO_BIT_H 1

#include <linux/l3/l3.h>

struct l3_algo_bit_data {
	void (*setdat) (void *data, int state);
	void (*setclk) (void *data, int state);
	void (*setmode)(void *data, int state);
	void (*setdir) (void *data, int in);	/* set data direction */
	int  (*getdat) (void *data);

	void *data;

	/* bus timings (us) */
	int	data_hold;
	int	data_setup;
	int	clock_high;
	int	mode_hold;
	int	mode_setup;
	int	mode;
};

int l3_bit_add_bus(struct l3_adapter *);
int l3_bit_del_bus(struct l3_adapter *);

#endif
