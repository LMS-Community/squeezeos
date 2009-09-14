/*
 * wm8350-bus.h  --  Power Supply Driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    8th Feb 2007   Initial version.
 *
 */

#ifndef __LINUX_REGULATOR_WM8350_BUS_H_
#define __LINUX_REGULATOR_WM8350_BUS_H_

#include <linux/regulator/wm8350/wm8350-pmic.h>
#include <linux/regulator/wm8350/wm8350-supply.h>
#include <linux/regulator/wm8350/wm8350-rtc.h>
#include <linux/regulator/wm8350/wm8350-wdt.h>

/* number of WM8350 interrupts */
#define WM8350_NUM_IRQ	79

/* WM8350 IO control types */
#define WM8350_IO_I2C       0
#define WM8350_IO_SPI       1
#define WM8350_IO_CUSTOM    2

extern struct bus_type wm8350_bus_type;

struct wm8350;
typedef int (*wm8350_hw_read_t)(struct wm8350 *wm8350, char reg, int size, char *dest);
typedef int (*wm8350_hw_write_t)(struct wm8350 *wm8350, char reg, int size, char *src);


struct wm8350_irq {
	void (*handler)(struct wm8350 *, int, void *);
	void *data;
};

struct wm8350_platform_data {
	unsigned int pmic_irq;
	unsigned int pmic_irq_type;
	unsigned int rtc_per_irq;
	unsigned int rtc_per_irq_type;
	int isink_A_dcdc;
	int isink_B_dcdc;
};

struct wm8350 {
	/* device IO */
	unsigned short i2c_address;
	struct i2c_client *i2c_client;
	struct spi_device *spi_device;
	wm8350_hw_read_t read_dev;
	wm8350_hw_write_t write_dev;
	u16 *reg_cache;
	struct wm8350_platform_data *pdata;

	/* clients */
	struct wm8350_pmic pmic;
	struct wm8350_wdg wdg;
	struct wm8350_rtc rtc;
	struct snd_soc_machine *audio;
	struct wm8350_power power;
	
	/* irq handlers and workq */
	int nirq;
	struct work_struct work;
	struct mutex work_mutex;
	struct wm8350_irq irq[WM8350_NUM_IRQ];
};
#define to_wm8350_from_pmic(d) container_of(d, struct wm8350, pmic)
#define to_wm8350_from_wdg(d) container_of(d, struct wm8350, wdg)
#define to_wm8350_from_rtc(d) container_of(d, struct wm8350, rtc)
#define to_wm8350_from_power(d) container_of(d, struct wm8350, power)
#define to_wm8350_from_audio(d) container_of(d, struct wm8350, audio)

/*
 * WM8350 device IO
 */
int wm8350_clear_bits(struct wm8350 *wm8350, u16 reg, u16 mask);
int wm8350_set_bits(struct wm8350 *wm8350, u16 reg, u16 mask);
u16 wm8350_reg_read(struct wm8350 *wm8350, int reg);
int wm8350_reg_write(struct wm8350 *wm8350, int reg, u16 val);
int wm8350_set_io(struct wm8350 *wm8350, int io, wm8350_hw_read_t read_dev,
	wm8350_hw_write_t write_dev);
int wm8350_reg_lock(struct wm8350 *wm8350);
int wm8350_reg_unlock(struct wm8350 *wm8350);
int wm8350_block_read(struct wm8350 *wm8350, int reg, int size,
	u16 *dest);
int wm8350_block_write(struct wm8350 *wm8350, int reg, int size,
	u16 *src);
int wm8350_create_cache(struct wm8350 *wm8350);

/*
 * Client device registration.
 */
int wm8350_device_register_wdg(struct wm8350 *wm8350);
int wm8350_device_register_pmic(struct wm8350 *wm8350);
int wm8350_device_register_rtc(struct wm8350 *wm8350);
int wm8350_device_register_power(struct wm8350 *wm8350);

/*
 * IRQ
 */
int wm8350_register_irq(struct wm8350 *wm8350, int irq,
	void (*handler)(struct wm8350 *, int, void *), void *data);
int wm8350_free_irq(struct wm8350 *wm8350, int irq);
int wm8350_mask_irq(struct wm8350 *wm8350, int irq);
int wm8350_unmask_irq(struct wm8350 *wm8350, int irq);
void wm8350_irq_worker(struct work_struct *work);

#endif
