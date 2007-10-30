/*
 * L3 bus algorithm module.
 *
 *  Copyright (C) 2001 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Note that L3 buses can share the same pins as I2C buses, so we must
 *  _not_ generate an I2C start condition.  An I2C start condition is
 *  defined as a high-to-low transition of the data line while the clock
 *  is high.  Therefore, we must only change the data line while the
 *  clock is low.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/l3/l3.h>
#include <linux/l3/algo-bit.h>

#define setdat(adap,val)	adap->setdat(adap->data, val)
#define setclk(adap,val)	adap->setclk(adap->data, val)
#define setmode(adap,val)	adap->setmode(adap->data, val)
#define setdatin(adap)		adap->setdir(adap->data, 1)
#define setdatout(adap)		adap->setdir(adap->data, 0)
#define getdat(adap)		adap->getdat(adap->data)

/*
 * Send one byte of data to the chip.  Data is latched into the chip on
 * the rising edge of the clock.
 */
static void sendbyte(struct l3_algo_bit_data *adap, unsigned int byte)
{
	int i;

	for (i = 0; i < 8; i++) {
		setclk(adap, 0);
		udelay(adap->data_hold);
		setdat(adap, byte & 1);
		udelay(adap->data_setup);
		setclk(adap, 1);
		udelay(adap->clock_high);
		byte >>= 1;
	}
}

/*
 * Send a set of bytes to the chip.  We need to pulse the MODE line
 * between each byte, but never at the start nor at the end of the
 * transfer.
 */
static void sendbytes(struct l3_algo_bit_data *adap, const char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i) {
			udelay(adap->mode_hold);
			setmode(adap, 0);
			udelay(adap->mode);
		}
		setmode(adap, 1);
		udelay(adap->mode_setup);
		sendbyte(adap, buf[i]);
	}
}

/*
 * Read one byte of data from the chip.  Data is latched into the chip on
 * the rising edge of the clock.
 */
static unsigned int readbyte(struct l3_algo_bit_data *adap)
{
	unsigned int byte = 0;
	int i;

	for (i = 0; i < 8; i++) {
		setclk(adap, 0);
		udelay(adap->data_hold + adap->data_setup);
		setclk(adap, 1);
		if (getdat(adap))
			byte |= 1 << i;
		udelay(adap->clock_high);
	}

	return byte;
}

/*
 * Read a set of bytes from the chip.  We need to pulse the MODE line
 * between each byte, but never at the start nor at the end of the
 * transfer.
 */
static void readbytes(struct l3_algo_bit_data *adap, char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i) {
			udelay(adap->mode_hold);
			setmode(adap, 0);
		}
		setmode(adap, 1);
		udelay(adap->mode_setup);
		buf[i] = readbyte(adap);
	}
}

static int l3_xfer(struct l3_adapter *l3_adap, struct l3_msg msgs[], int num)
{
	struct l3_algo_bit_data *adap = l3_adap->algo_data;
	int i;

	/*
	 * If we share an I2C bus, ensure that it is in STOP mode
	 */
	setclk(adap, 1);
	setdat(adap, 1);
	setmode(adap, 1);
	setdatout(adap);
	udelay(adap->mode);

	for (i = 0; i < num; i++) {
		struct l3_msg *pmsg = &msgs[i];

		if (!(pmsg->flags & L3_M_NOADDR)) {
			setmode(adap, 0);
			udelay(adap->mode_setup);
			sendbyte(adap, pmsg->addr);
			udelay(adap->mode_hold);
		}

		if (pmsg->flags & L3_M_RD) {
			setdatin(adap);
			readbytes(adap, pmsg->buf, pmsg->len);
		} else {
			setdatout(adap);
			sendbytes(adap, pmsg->buf, pmsg->len);
		}
	}

	/*
	 * Ensure that we leave the bus in I2C stop mode.
	 */
	setclk(adap, 1);
	setdat(adap, 1);
	setmode(adap, 0);
	setdatin(adap);

	return num;
}

static struct l3_algorithm l3_bit_algo = {
	name:	"L3 bit-shift algorithm",
	xfer:	l3_xfer,
};

int l3_bit_add_bus(struct l3_adapter *adap)
{
	adap->algo = &l3_bit_algo;
	return l3_add_adapter(adap);
}

int l3_bit_del_bus(struct l3_adapter *adap)
{
	return l3_del_adapter(adap);
}

EXPORT_SYMBOL(l3_bit_add_bus);
EXPORT_SYMBOL(l3_bit_del_bus);
