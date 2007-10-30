/*
 *  linux/include/linux/l3/l3.h
 *
 *  Copyright (C) 2001 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Derived from i2c.h by Simon G. Vogl
 */
#ifndef L3_H
#define L3_H

struct l3_msg {
	unsigned char	addr;	/* slave address	*/
	unsigned char	flags;		
#define L3_M_RD		0x01
#define L3_M_NOADDR	0x02
	unsigned short	len;	/* msg length		*/
	unsigned char	*buf;	/* pointer to msg data	*/
};

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/list.h>

struct l3_adapter;

struct l3_algorithm {
	/* textual description */
	char name[32];

	/* perform bus transactions */
	int (*xfer)(struct l3_adapter *, struct l3_msg msgs[], int num);
};

struct semaphore;

/*
 * l3_adapter is the structure used to identify a physical L3 bus along
 * with the access algorithms necessary to access it.
 */
struct l3_adapter {
	/*
	 * This name is used to uniquely identify the adapter.
	 * It should be the same as the module name.
	 */
	char			name[32];

	/*
	 * the algorithm to access the bus
	 */
	struct l3_algorithm	*algo;

	/*
	 * Algorithm specific data
	 */
	void			*algo_data;

	/*
	 * This may be NULL, or should point to the module struct
	 */
	struct module		*owner;

	/*
	 * private data for the adapter
	 */
	void			*data;

	/*
	 * Our lock.  Unlike the i2c layer, we allow this to be used for
	 * other stuff, like the i2c layer lock.  Some people implement
	 * i2c stuff using the same signals as the l3 bus.
	 */
	struct semaphore	*lock;

	/*
	 * List of all adapters.
	 */
	struct list_head	adapters;
};

extern int l3_add_adapter(struct l3_adapter *);
extern int l3_del_adapter(struct l3_adapter *);
extern void l3_put_adapter(struct l3_adapter *);
extern struct l3_adapter *l3_get_adapter(const char *name);

extern int l3_write(struct l3_adapter *, int, const char *, int);
extern int l3_read(struct l3_adapter *, int, char *, int);

#endif

#endif /* L3_H */
