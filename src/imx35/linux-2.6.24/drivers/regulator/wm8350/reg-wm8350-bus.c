/*
 * wm8350_bus.c  --  Power Management Driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood
 *         liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    23rd Jan 2007   Initial version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/regulator/wm8350/wm8350.h>
#include <linux/regulator/wm8350/wm8350-bus.h>
#include <linux/regulator/wm8350/wm8350-pmic.h>
#include <linux/regulator/wm8350/wm8350-gpio.h>
#include <linux/regulator/wm8350/wm8350-comparator.h>
#include <linux/regulator/wm8350/wm8350-supply.h>
#include <linux/regulator/wm8350/wm8350-audio.h>
#include <linux/delay.h>

#define WM8350_BUS_VERSION "0.4"
#define WM8350_UNLOCK_KEY 0x0013
#define WM8350_LOCK_KEY 0x0000

#define WM8350_CLOCK_CONTROL_1                  0x28
#define WM8350_AIF_TEST                         0x74

/* debug */
#define WM8350_BUS_DEBUG 0
#if WM8350_BUS_DEBUG
#define dbg(format, arg...) printk(format, ## arg)
#define dump(regs, src) do { \
	int i; \
	u16 *src_ = src; \
	for (i = 0; i < regs; i++) \
		dbg(" 0x%4.4x", *src_++); \
	dbg("\n"); \
} while (0);
#else
#define dbg(format, arg...)
#define dump(bytes, src)
#endif

#define WM8350_LOCK_DEBUG 0
#if WM8350_LOCK_DEBUG
#define ldbg(format, arg...) printk(format, ## arg)
#else
#define ldbg(format, arg...)
#endif

#define BYTE_SWAP16(x) (x >> 8 | x << 8)  // lg replace with generic

/*
 * WM8350 can run in 1 of 4 configuration modes.
 * Each mode has different default register values.
 */
#if defined(CONFIG_PMIC_WM8350_MODE_0)
static const u16 wm8350_reg_map[] = WM8350_REGISTER_DEFAULTS_0;
#elif defined(CONFIG_PMIC_WM8350_MODE_1)
static const u16 wm8350_reg_map[] = WM8350_REGISTER_DEFAULTS_1;
#elif defined(CONFIG_PMIC_WM8350_MODE_2)
static const u16 wm8350_reg_map[] = WM8350_REGISTER_DEFAULTS_2;
#elif defined(CONFIG_PMIC_WM8350_MODE_3)
static const u16 wm8350_reg_map[] = WM8350_REGISTER_DEFAULTS_3;
#else
#error Invalid WM8350 configuration
#endif

/*
 * WM8350 Register IO access map
 */
static const struct wm8350_reg_access wm8350_reg_io_map[] = WM8350_ACCESS;

/*
 * WM8350 Device IO
 */
static DEFINE_MUTEX(io_mutex);
static DEFINE_MUTEX(reg_lock_mutex);
static DEFINE_MUTEX(auxadc_mutex);

#ifdef CONFIG_I2C
static int wm8350_read_i2c_device(struct wm8350 *wm8350, char reg,
	int bytes, char *dest)
{
	int ret;

	ret = i2c_master_send(wm8350->i2c_client, &reg, 1);
	if (ret < 0)
		return ret;
	return i2c_master_recv(wm8350->i2c_client, dest, bytes);
}

static int wm8350_write_i2c_device(struct wm8350 *wm8350, char reg,
	int bytes, char *src)
{
	/* we add 1 byte for device register */
	u8 msg[(WM8350_MAX_REGISTER << 1) + 1];

	if (bytes > ((WM8350_MAX_REGISTER << 1) + 1))
		return -EINVAL;

	msg[0] = reg;
	memcpy(&msg[1], src, bytes);
	return i2c_master_send(wm8350->i2c_client, msg, bytes + 1);
}
#endif

#ifdef CONFIG_SPI
static int wm8350_read_spi_device(struct wm8350 *wm8350, char reg,
	int bytes, char *dest)
{
	int ret;
	u8 tx_msg[4], rx_msg[4];

	/* don't support incremental write with SPI */
	if (bytes != 2)
		return -EIO;
		
	tx_msg[0] = 0x80;
	tx_msg[1] = reg; 
	tx_msg[2] = 0;
	tx_msg[3] = 0;

	ret = spi_write_then_read(wm8350->spi_device, tx_msg, 4, rx_msg, 4);
	if (ret < 0) {
		printk(KERN_ERR "%s: io failure %d\n", __func__, ret);
		return 0;
	}
		
	*dest++ = rx_msg[2];
	*dest = rx_msg[3];
	return 0; 
}

static int wm8350_write_spi_device(struct wm8350 *wm8350, char reg,
	int bytes, char *src)
{
	u8 msg[4];

	/* don't support incremental write with SPI */
	if (bytes != 2)
		return -EIO;

	msg[0] = 0;
	msg[1] = reg; 
	msg[2] = *src++;
	msg[3] = *src;
	return spi_write(wm8350->spi_device, msg, 4);
}
#endif

/* mask in WM8350 read bits */
static inline void wm8350_mask_read(u8 reg, int bytes, u16 *buf)
{
 	int i;

 	for (i = reg; i < reg + (bytes >> 1); i++)
 		*buf++ &= wm8350_reg_io_map[i].readable;
}

/* mask in WM8350 write bits */
static inline void wm8350_mask_write(u8 reg, int bytes, u16 *buf)
{
 	int i;

 	for (i = reg; i < reg + (bytes >> 1); i++)
 		*buf++ &= wm8350_reg_io_map[i].writable;
}

/* WM8350 is big endian, swap if necessary */
static inline void wm8350_endian_swap(u8 reg, int bytes, u16 *buf)
{
#ifdef  __LITTLE_ENDIAN
 	int i;
 	u16 tmp;

 	for (i = reg; i < reg + (bytes >> 1); i++) {
 		tmp = BYTE_SWAP16(*buf);
 		*buf++ = tmp;
 	}
#endif
}

static inline void wm8350_cache_mask(struct wm8350 *wm8350, u8 reg,
	int bytes, u16 *dest)
{
	int i;
	u16 mask;

	for (i = reg; i < reg + (bytes >> 1); i++) {
 		*dest &= wm8350_reg_io_map[i].vol;
 		mask = wm8350->reg_cache[reg] & ~wm8350_reg_io_map[i].vol;
 		*dest |= mask;
 		dest++;
 	}
}

static int wm8350_read(struct wm8350 *wm8350, u8 reg, int num_regs, u16 *dest)
{
	int i, end = reg + num_regs, ret = 0, bytes = num_regs << 1;

	if (wm8350->read_dev == NULL)
		return -ENODEV;

	if ((reg + num_regs - 1) > WM8350_MAX_REGISTER) {
		printk(KERN_ERR "wm8350: invalid reg %x\n", reg + num_regs - 1);
		return -EINVAL;
	}

	dbg("%s R%d(0x%2.2x) %d regs ", __FUNCTION__, reg, reg, num_regs);

#if WM8350_BUS_DEBUG
	/* we can _safely_ read any register, but warn if read not supported */
	for (i = reg; i < end; i++) {
		if (!wm8350_reg_io_map[i].readable)
			printk(KERN_WARNING "wm8350: reg R%d is not readable\n", i);
	}
#endif
	/* if any volatile registers are required, then read back all */
	for (i = reg; i < end; i++) {
		if (wm8350_reg_io_map[i].vol) {
			dbg("volatile read ");
			ret = wm8350->read_dev(wm8350, reg,
				bytes, (char*)dest);
			wm8350_endian_swap(reg, bytes, dest);
			wm8350_cache_mask(wm8350, reg, bytes, dest);
			wm8350_mask_read(reg, bytes, dest);
			dump(num_regs, dest);
			return ret;
		}
	}

	/* no volatiles, then cache is good */
	dbg("cache read ");
	memcpy(dest, &wm8350->reg_cache[reg], bytes);
	dump(num_regs, dest);
	return ret;
}

static inline int is_reg_locked(struct wm8350 *wm8350, u8 reg)
{
	if (reg == WM8350_SECURITY ||
		wm8350->reg_cache[WM8350_SECURITY] == WM8350_UNLOCK_KEY)
		return 0;

	if ((reg == WM8350_GPIO_CONFIGURATION_I_O) ||
		(reg >= WM8350_GPIO_FUNCTION_SELECT_1 &&
		reg <= WM8350_GPIO_FUNCTION_SELECT_4) ||
		(reg >= WM8350_BATTERY_CHARGER_CONTROL_1 &&
		reg <= WM8350_BATTERY_CHARGER_CONTROL_3))
		return 1;
	return 0;
}

static int wm8350_write(struct wm8350 *wm8350, u8 reg, int num_regs, u16 *src)
{
	int ret, i, end = reg + num_regs, bytes = num_regs << 1;

	if (wm8350->write_dev == NULL)
		return -ENODEV;

	if ((reg + num_regs - 1) > WM8350_MAX_REGISTER) {
		printk(KERN_ERR "wm8350: invalid reg %x\n", reg + num_regs - 1);
		return -EINVAL;
	}

	wm8350_mask_write(reg, bytes, src);
	memcpy(&wm8350->reg_cache[reg], src, bytes);
	dbg("%s R%d(0x%2.2x) %d regs ", __FUNCTION__, reg, reg, num_regs);
	dump(num_regs, src);

	wm8350_endian_swap(reg, bytes, src);

	/* it's generally not a good idea to write to RO or locked registers */
	for (i = reg; i < end; i++) {
		if (!wm8350_reg_io_map[i].writable) {
			printk(KERN_ERR "wm8350: attempted write to read only reg R%d\n", i);
			return -EINVAL;
		}

		if (is_reg_locked(wm8350, i)) {
			printk(KERN_ERR "wm8350: attempted write to locked reg R%d\n", i);
			return -EINVAL;
		}
	}

	/* write registers and update cache if successful */
	ret = wm8350->write_dev(wm8350, reg, bytes, (char*)src);
	return ret;
}

/*
 * Safe read, modify, write methods
 */
int wm8350_clear_bits(struct wm8350 *wm8350, u16 reg, u16 mask)
{
	u16 data;
	int err;

	mutex_lock(&io_mutex);
	err = wm8350_read(wm8350, reg, 1, &data);
	if (err) {
		printk(KERN_ERR "wm8350: read from reg R%d failed\n", reg);
		goto out;
	}

	data &= ~mask;
	err = wm8350_write(wm8350, reg, 1, &data);
	if (err)
		printk(KERN_ERR "wm8350: write to reg R%d failed\n", reg);
out:
	mutex_unlock(&io_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(wm8350_clear_bits);

int wm8350_set_bits(struct wm8350 *wm8350, u16 reg, u16 mask)
{
	u16 data;
	int err;

	mutex_lock(&io_mutex);
	err = wm8350_read(wm8350, reg, 1, &data);
	if (err) {
		printk(KERN_ERR "wm8350: read from reg R%d failed\n", reg);
		goto out;
	}

	data |= mask;
	err = wm8350_write(wm8350, reg, 1, &data);
	if (err)
		printk(KERN_ERR "wm8350: write to reg R%d failed\n", reg);
out:
	mutex_unlock(&io_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(wm8350_set_bits);

u16 wm8350_reg_read(struct wm8350 *wm8350, int reg)
{
	u16 data;
	int err;

	mutex_lock(&io_mutex);
	err = wm8350_read(wm8350, reg, 1, &data);
	if (err)
		printk(KERN_ERR "wm8350: read from reg R%d failed\n", reg);

	mutex_unlock(&io_mutex);
	return data;
}
EXPORT_SYMBOL_GPL(wm8350_reg_read);

int wm8350_reg_write(struct wm8350 *wm8350, int reg, u16 val)
{
	int ret;
	u16 data = val;

	mutex_lock(&io_mutex);
	ret = wm8350_write(wm8350, reg, 1, &data);
	if (ret)
		printk(KERN_ERR "wm8350: write to reg R%d failed\n", reg);
	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_reg_write);

int wm8350_block_read(struct wm8350 *wm8350, int start_reg, int regs,
	u16 *dest)
{
	int err = 0;

	mutex_lock(&io_mutex);
	err = wm8350_read(wm8350, start_reg, regs, dest);
	if (err)
		printk(KERN_ERR "wm8350: block read starting from R%d failed\n",
			start_reg);
	mutex_unlock(&io_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(wm8350_block_read);

int wm8350_block_write(struct wm8350 *wm8350, int start_reg, int regs,
	u16 *src)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = wm8350_write(wm8350, start_reg, regs, src);
	if (ret)
		printk(KERN_ERR "wm8350: block write starting at R%d failed\n",
			start_reg);
	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_block_write);

int wm8350_reg_lock(struct wm8350 *wm8350)
{
	u16 key = WM8350_LOCK_KEY;
	int ret;

	ldbg ("%s\n", __FUNCTION__);
	mutex_lock(&io_mutex);
	ret = wm8350_write(wm8350, WM8350_SECURITY, 1, &key);
	if (ret)
		printk(KERN_ERR "wm8350: lock failed\n");
	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_reg_lock);

int wm8350_reg_unlock(struct wm8350 *wm8350)
{
	u16 key = WM8350_UNLOCK_KEY;
	int ret;

	ldbg ("%s\n", __FUNCTION__);
	mutex_lock(&io_mutex);
	ret = wm8350_write(wm8350, WM8350_SECURITY, 1, &key);
	if (ret)
		printk(KERN_ERR "wm8350: unlock failed\n");
	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_reg_unlock);

/*
 * For Switching between SPI and I2C IO
 */
int wm8350_set_io(struct wm8350 *wm8350, int io, wm8350_hw_read_t read_dev,
	wm8350_hw_write_t write_dev)
{
	mutex_lock(&io_mutex);
	switch (io) {
	case WM8350_IO_I2C:
#ifdef CONFIG_I2C
		wm8350->read_dev = wm8350_read_i2c_device;
		wm8350->write_dev = wm8350_write_i2c_device;
		break;
#else
		printk(KERN_ERR "wm8350: I2C not selected.\n");
		wm8350->read_dev = NULL;
		wm8350->write_dev = NULL;
		mutex_unlock(&io_mutex);
		return -EINVAL;
#endif
	case WM8350_IO_SPI:
#ifdef CONFIG_SPI
		wm8350->read_dev = wm8350_read_spi_device;
		wm8350->write_dev = wm8350_write_spi_device;
		break;
#else
		printk(KERN_ERR "wm8350: SPI not selected.\n");
		wm8350->read_dev = NULL;
		wm8350->write_dev = NULL;
		mutex_unlock(&io_mutex);
		return -EINVAL;
#endif
	default:
		printk(KERN_ERR "wm8350: invalid IO mechanism\n");
		wm8350->read_dev = NULL;
		wm8350->write_dev = NULL;
		mutex_unlock(&io_mutex);
		return -EINVAL;
	}
	mutex_unlock(&io_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_set_io);

/*
 * Cache is always host endian.
 */
int wm8350_create_cache(struct wm8350 *wm8350)
{
	int i, ret = 0;
	u16 value;

	if (wm8350->read_dev == NULL)
		return -ENODEV;

	wm8350->reg_cache =
		kzalloc(sizeof(u16) * (WM8350_MAX_REGISTER + 1), GFP_KERNEL);
	if (wm8350->reg_cache == NULL)
		return -ENOMEM;

	/* TODO: check if we are virgin state so we don't have to do this */
	/* refresh cache with chip regs as some registers can survive reboot */
	for (i = 0; i < WM8350_MAX_REGISTER; i++) {
		/* audio register range */
		if (wm8350_reg_io_map[i].readable &&
			(i < WM8350_CLOCK_CONTROL_1 || i > WM8350_AIF_TEST)) {
			ret = wm8350->read_dev(wm8350, i, 2, (char*)&value);
			if (ret < 0) {
				printk(KERN_ERR
				"wm8350: failed to create cache\n");
				goto out;
			}
			wm8350_endian_swap(i, 2, &value);
			wm8350_mask_read(i, 2, &value);
			wm8350->reg_cache[i] = value;
		} else
			wm8350->reg_cache[i] = wm8350_reg_map[i];
	}

out:
	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_create_cache);


static void wm8350_irq_call_worker(struct wm8350 *wm8350, int irq)
{
	mutex_lock(&wm8350->work_mutex);

	if (wm8350->irq[irq].handler)
		wm8350->irq[irq].handler(wm8350, irq, wm8350->irq[irq].data);
	else {
		mutex_unlock(&wm8350->work_mutex);
		printk(KERN_ERR "wm8350: irq %d nobody cared. now masked.\n",
			irq);
		wm8350_mask_irq(wm8350, irq);
		return;
	}
	mutex_unlock(&wm8350->work_mutex);
}

void wm8350_irq_worker(struct work_struct *work)
{
	u16 level_one, status1, status2, comp, oc, gpio, uv;
	struct wm8350 *wm8350 =
		container_of(work, struct wm8350, work);

	/* read this in 1 block read */
	/* read 1st level irq sources and then read required 2nd sources */
	level_one = wm8350_reg_read(wm8350, WM8350_SYSTEM_INTERRUPTS)
		& ~wm8350_reg_read(wm8350, WM8350_SYSTEM_INTERRUPTS_MASK);
	uv = wm8350_reg_read(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS)
		& ~wm8350_reg_read(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK);
	oc = wm8350_reg_read(wm8350, WM8350_OVER_CURRENT_INTERRUPT_STATUS)
		& ~wm8350_reg_read(wm8350, WM8350_OVER_CURRENT_INTERRUPT_STATUS_MASK);
	status1 = wm8350_reg_read(wm8350, WM8350_INTERRUPT_STATUS_1)
		& ~wm8350_reg_read(wm8350, WM8350_INTERRUPT_STATUS_1_MASK);
	status2 = wm8350_reg_read(wm8350, WM8350_INTERRUPT_STATUS_2)
		& ~wm8350_reg_read(wm8350, WM8350_INTERRUPT_STATUS_2_MASK);
	comp = wm8350_reg_read(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS)
		& ~wm8350_reg_read(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK);
	gpio = wm8350_reg_read(wm8350, WM8350_GPIO_INTERRUPT_STATUS)
		& ~wm8350_reg_read(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK);

	/* over current */
	if (level_one & WM8350_OC_INT) {
		if (oc & WM8350_OC_LS_EINT) /* limit switch */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_OC_LS);
	}

	/* under voltage */
	if (level_one & WM8350_UV_INT) {
		if (uv & WM8350_UV_DC1_EINT) /* DCDC1 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC1);
		if (uv & WM8350_UV_DC2_EINT) /* DCDC2 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC2);
		if (uv & WM8350_UV_DC3_EINT) /* DCDC3 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC3);
		if (uv & WM8350_UV_DC4_EINT) /* DCDC4 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC4);
		if (uv & WM8350_UV_DC5_EINT) /* DCDC5 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC5);
		if (uv & WM8350_UV_DC6_EINT) /* DCDC6 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_DC6);
		if (uv & WM8350_UV_LDO1_EINT) /* LDO1 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_LDO1);
		if (uv & WM8350_UV_LDO2_EINT) /* LDO2 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_LDO2);
		if (uv & WM8350_UV_LDO3_EINT) /* LDO3 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_LDO3);
		if (uv & WM8350_UV_LDO4_EINT) /* LDO4 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_UV_LDO4);
	}

	/* charger, RTC */
	if (status1) {
		if (status1 & WM8350_CHG_BAT_HOT_EINT) /* battery too hot */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_BAT_HOT);
		if (status1 & WM8350_CHG_BAT_COLD_EINT) /* battery too cold */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_BAT_COLD);
		if (status1 & WM8350_CHG_BAT_FAIL_EINT) /* battery fail */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_BAT_FAIL);
		if (status1 & WM8350_CHG_TO_EINT) /* charger timeout */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_TO);
		if (status1 & WM8350_CHG_END_EINT) /* fast charge current */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_END);
		if (status1 & WM8350_CHG_START_EINT) /* charging started */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_START);
		if (status1 & WM8350_CHG_FAST_RDY_EINT) /* fast charge ready */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_FAST_RDY);
		if (status1 & WM8350_CHG_VBATT_LT_3P9_EINT) /* battery voltage < 3.9 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P9);
		if (status1 & WM8350_CHG_VBATT_LT_3P1_EINT) /* battery voltage < 3.1 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_VBATT_LT_3P1);
		if (status1 & WM8350_CHG_VBATT_LT_2P85_EINT) /* battery voltage < 2.85 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CHG_VBATT_LT_2P85);

		if (status1 & WM8350_RTC_ALM_EINT) /* alarm */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_RTC_ALM);
		if (status1 & WM8350_RTC_SEC_EINT) /* second rollover */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_RTC_SEC);
		if (status1 & WM8350_RTC_PER_EINT) /* periodic */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_RTC_PER);
	}

	/* current sink, system, aux adc */
	if (status2) {
		if (status2 & WM8350_CS1_EINT) /* current sink 1 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CS1);
		if (status2 & WM8350_CS2_EINT) /* current sink 2 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CS2);

		if (status2 & WM8350_SYS_HYST_COMP_FAIL_EINT) /* comp fail */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_SYS_HYST_COMP_FAIL);
		if (status2 & WM8350_SYS_CHIP_GT115_EINT) /* chip > 115 C */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_SYS_CHIP_GT115);
		if (status2 & WM8350_SYS_CHIP_GT140_EINT) /* chip > 140 C */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_SYS_CHIP_GT140);
		if (status2 & WM8350_SYS_WDOG_TO_EINT) /* heartbeat missed */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_SYS_WDOG_TO);

		if (status2 & WM8350_AUXADC_DATARDY_EINT) /* data ready */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_AUXADC_DATARDY);
		if (status2 & WM8350_AUXADC_DCOMP4_EINT) /* exceeds comp 4 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_AUXADC_DCOMP4);
		if (status2 & WM8350_AUXADC_DCOMP3_EINT) /* exceeds comp 3 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_AUXADC_DCOMP3);
		if (status2 & WM8350_AUXADC_DCOMP2_EINT) /* exceeds comp 2 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_AUXADC_DCOMP2);
		if (status2 & WM8350_AUXADC_DCOMP1_EINT) /* exceeds comp 1 */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_AUXADC_DCOMP1);

		if (status2 & WM8350_USB_LIMIT_EINT) /* usb limit */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_USB_LIMIT);
	}

	/* wake, codec, ext */
	if (comp) {
		if (comp & WM8350_WKUP_OFF_STATE_EINT) /* wake from off */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_OFF_STATE);
		if (comp & WM8350_WKUP_HIB_STATE_EINT) /* wake from hibernate */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_HIB_STATE);
		if (comp & WM8350_WKUP_CONV_FAULT_EINT) /* wake from fault */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_CONV_FAULT);
		if (comp & WM8350_WKUP_WDOG_RST_EINT) /* wake from reset */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_WDOG_RST);
		if (comp & WM8350_WKUP_GP_PWR_ON_EINT) /* power on changed */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_GP_PWR_ON);
		if (comp & WM8350_WKUP_ONKEY_EINT) /* on key > specified time */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_ONKEY);
		if (comp & WM8350_WKUP_GP_WAKEUP_EINT) /* wake from GPIO */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_WKUP_GP_WAKEUP);

		if (comp & WM8350_CODEC_JCK_DET_L_EINT) /* left chn Jack detect */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CODEC_JCK_DET_L);
		if (comp & WM8350_CODEC_JCK_DET_R_EINT) /* right chn Jack detect */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CODEC_JCK_DET_R);
		if (comp & WM8350_CODEC_MICSCD_EINT) /* mic detect */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CODEC_MICSCD);
		if (comp & WM8350_CODEC_MICD_EINT) /* mic short circuit */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_CODEC_MICD);

		if (comp & WM8350_EXT_USB_FB_EINT) /* usb connect */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_EXT_USB_FB);
		if (comp & WM8350_EXT_WALL_FB_EINT) /* wall adaptor connect */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_EXT_WALL_FB);
		if (comp & WM8350_EXT_BAT_FB_EINT) /* battery insertion */
			wm8350_irq_call_worker(wm8350, WM8350_IRQ_EXT_BAT_FB);
	}

	if (level_one & WM8350_GP_INT) { /* gpio */
		int i;

		for (i = 0; i < 12; i++) {
			if (gpio & (1 << i))
				wm8350_irq_call_worker(wm8350, WM8350_IRQ_GPIO(i));
		}
	}
}
EXPORT_SYMBOL_GPL(wm8350_irq_worker);

int wm8350_register_irq(struct wm8350 *wm8350, int irq,
	void (*handler)(struct wm8350 *, int, void*), void *data)
{
	if (irq < 0 || irq > WM8350_NUM_IRQ || !handler)
		return -EINVAL;

	if (wm8350->irq[irq].handler)
		return -EBUSY;

	mutex_lock(&wm8350->work_mutex);
	wm8350->irq[irq].handler = handler;
	wm8350->irq[irq].data = data;
	mutex_unlock(&wm8350->work_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_register_irq);

int wm8350_free_irq(struct wm8350 *wm8350, int irq)
{
	if (irq < 0 || irq > WM8350_NUM_IRQ)
		return -EINVAL;

	mutex_lock(&wm8350->work_mutex);
	wm8350->irq[irq].handler = NULL;
	mutex_unlock(&wm8350->work_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_free_irq);

int wm8350_mask_irq(struct wm8350 *wm8350, int irq)
{
	switch (irq) {
	case WM8350_IRQ_CHG_BAT_HOT:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_HOT_EINT);
	case WM8350_IRQ_CHG_BAT_COLD:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_COLD_EINT);
	case WM8350_IRQ_CHG_BAT_FAIL:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_FAIL_EINT);
	case WM8350_IRQ_CHG_TO:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_TO_EINT);
	case WM8350_IRQ_CHG_END:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_END_EINT);
	case WM8350_IRQ_CHG_START:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_START_EINT);
	case WM8350_IRQ_CHG_FAST_RDY:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_FAST_RDY_EINT);
	case WM8350_IRQ_RTC_PER:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_PER_EINT);
	case WM8350_IRQ_RTC_SEC:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_SEC_EINT);
	case WM8350_IRQ_RTC_ALM:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_ALM_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_3P9:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_3P9_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_3P1:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_3P1_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_2P85:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_2P85_EINT);
	case WM8350_IRQ_CS1:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_CS1_EINT);
	case WM8350_IRQ_CS2:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_CS2_EINT);
	case WM8350_IRQ_USB_LIMIT:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_USB_LIMIT_EINT);
	case WM8350_IRQ_AUXADC_DATARDY:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DATARDY_EINT);
	case WM8350_IRQ_AUXADC_DCOMP4:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP4_EINT);
	case WM8350_IRQ_AUXADC_DCOMP3:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP3_EINT);
	case WM8350_IRQ_AUXADC_DCOMP2:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP2_EINT);
	case WM8350_IRQ_AUXADC_DCOMP1:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP1_EINT);
	case WM8350_IRQ_SYS_HYST_COMP_FAIL:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_HYST_COMP_FAIL_EINT);
	case WM8350_IRQ_SYS_CHIP_GT115:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_CHIP_GT115_EINT);
	case WM8350_IRQ_SYS_CHIP_GT140:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_CHIP_GT140_EINT);
	case WM8350_IRQ_SYS_WDOG_TO:
		return wm8350_set_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_WDOG_TO_EINT);
	case WM8350_IRQ_UV_LDO4:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO4_EINT);
	case WM8350_IRQ_UV_LDO3:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO3_EINT);
	case WM8350_IRQ_UV_LDO2:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO2_EINT);
	case WM8350_IRQ_UV_LDO1:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO1_EINT);
	case WM8350_IRQ_UV_DC6:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC6_EINT);
	case WM8350_IRQ_UV_DC5:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC5_EINT);
	case WM8350_IRQ_UV_DC4:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC4_EINT);
	case WM8350_IRQ_UV_DC3:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC3_EINT);
	case WM8350_IRQ_UV_DC2:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC2_EINT);
	case WM8350_IRQ_UV_DC1:
		return wm8350_set_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC1_EINT);
	case WM8350_IRQ_OC_LS:
		return wm8350_set_bits(wm8350, WM8350_OVER_CURRENT_INTERRUPT_STATUS_MASK,
			WM8350_IM_OC_LS_EINT);
	case WM8350_IRQ_EXT_USB_FB:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_USB_FB_EINT);
	case WM8350_IRQ_EXT_WALL_FB:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_WALL_FB_EINT);
	case WM8350_IRQ_EXT_BAT_FB:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_BAT_FB_EINT);
	case WM8350_IRQ_CODEC_JCK_DET_L:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_JCK_DET_L_EINT);
	case WM8350_IRQ_CODEC_JCK_DET_R:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_JCK_DET_R_EINT);
	case WM8350_IRQ_CODEC_MICSCD:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_MICSCD_EINT);
	case WM8350_IRQ_CODEC_MICD:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_MICD_EINT);
	case WM8350_IRQ_WKUP_OFF_STATE:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_OFF_STATE_EINT);
	case WM8350_IRQ_WKUP_HIB_STATE:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_HIB_STATE_EINT);
	case WM8350_IRQ_WKUP_CONV_FAULT:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_CONV_FAULT_EINT);
	case WM8350_IRQ_WKUP_WDOG_RST:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_OFF_STATE_EINT);
	case WM8350_IRQ_WKUP_GP_PWR_ON:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_GP_PWR_ON_EINT);
	case WM8350_IRQ_WKUP_ONKEY:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_ONKEY_EINT);
	case WM8350_IRQ_WKUP_GP_WAKEUP:
		return wm8350_set_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_GP_WAKEUP_EINT);
	case WM8350_IRQ_GPIO(0):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP0_EINT);
	case WM8350_IRQ_GPIO(1):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP1_EINT);
	case WM8350_IRQ_GPIO(2):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP2_EINT);
	case WM8350_IRQ_GPIO(3):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP3_EINT);
	case WM8350_IRQ_GPIO(4):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP4_EINT);
	case WM8350_IRQ_GPIO(5):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP5_EINT);
	case WM8350_IRQ_GPIO(6):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP6_EINT);
	case WM8350_IRQ_GPIO(7):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP7_EINT);
	case WM8350_IRQ_GPIO(8):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP8_EINT);
	case WM8350_IRQ_GPIO(9):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP9_EINT);
	case WM8350_IRQ_GPIO(10):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP10_EINT);
	case WM8350_IRQ_GPIO(11):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP11_EINT);
	case WM8350_IRQ_GPIO(12):
		return wm8350_set_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP12_EINT);
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_mask_irq);

int wm8350_unmask_irq(struct wm8350 *wm8350, int irq)
{
	switch (irq) {
	case WM8350_IRQ_CHG_BAT_HOT:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_HOT_EINT);
	case WM8350_IRQ_CHG_BAT_COLD:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_COLD_EINT);
	case WM8350_IRQ_CHG_BAT_FAIL:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_BAT_FAIL_EINT);
	case WM8350_IRQ_CHG_TO:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_TO_EINT);
	case WM8350_IRQ_CHG_END:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_END_EINT);
	case WM8350_IRQ_CHG_START:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_START_EINT);
	case WM8350_IRQ_CHG_FAST_RDY:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_FAST_RDY_EINT);
	case WM8350_IRQ_RTC_PER:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_PER_EINT);
	case WM8350_IRQ_RTC_SEC:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_SEC_EINT);
	case WM8350_IRQ_RTC_ALM:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_RTC_ALM_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_3P9:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_3P9_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_3P1:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_3P1_EINT);
	case WM8350_IRQ_CHG_VBATT_LT_2P85:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_1_MASK,
			WM8350_IM_CHG_VBATT_LT_2P85_EINT);
	case WM8350_IRQ_CS1:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_CS1_EINT);
	case WM8350_IRQ_CS2:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_CS2_EINT);
	case WM8350_IRQ_USB_LIMIT:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_USB_LIMIT_EINT);
	case WM8350_IRQ_AUXADC_DATARDY:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DATARDY_EINT);
	case WM8350_IRQ_AUXADC_DCOMP4:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP4_EINT);
	case WM8350_IRQ_AUXADC_DCOMP3:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP3_EINT);
	case WM8350_IRQ_AUXADC_DCOMP2:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP2_EINT);
	case WM8350_IRQ_AUXADC_DCOMP1:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_AUXADC_DCOMP1_EINT);
	case WM8350_IRQ_SYS_HYST_COMP_FAIL:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_HYST_COMP_FAIL_EINT);
	case WM8350_IRQ_SYS_CHIP_GT115:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_CHIP_GT115_EINT);
	case WM8350_IRQ_SYS_CHIP_GT140:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_CHIP_GT140_EINT);
	case WM8350_IRQ_SYS_WDOG_TO:
		return wm8350_clear_bits(wm8350, WM8350_INTERRUPT_STATUS_2_MASK,
			WM8350_IM_SYS_WDOG_TO_EINT);
	case WM8350_IRQ_UV_LDO4:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO4_EINT);
	case WM8350_IRQ_UV_LDO3:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO3_EINT);
	case WM8350_IRQ_UV_LDO2:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO2_EINT);
	case WM8350_IRQ_UV_LDO1:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_LDO1_EINT);
	case WM8350_IRQ_UV_DC6:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC6_EINT);
	case WM8350_IRQ_UV_DC5:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC5_EINT);
	case WM8350_IRQ_UV_DC4:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC4_EINT);
	case WM8350_IRQ_UV_DC3:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC3_EINT);
	case WM8350_IRQ_UV_DC2:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC2_EINT);
	case WM8350_IRQ_UV_DC1:
		return wm8350_clear_bits(wm8350, WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK,
			WM8350_IM_UV_DC1_EINT);
	case WM8350_IRQ_OC_LS:
		return wm8350_clear_bits(wm8350, WM8350_OVER_CURRENT_INTERRUPT_STATUS_MASK,
			WM8350_IM_OC_LS_EINT);
	case WM8350_IRQ_EXT_USB_FB:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_USB_FB_EINT);
	case WM8350_IRQ_EXT_WALL_FB:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_WALL_FB_EINT);
	case WM8350_IRQ_EXT_BAT_FB:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_EXT_BAT_FB_EINT);
	case WM8350_IRQ_CODEC_JCK_DET_L:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_JCK_DET_L_EINT);
	case WM8350_IRQ_CODEC_JCK_DET_R:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_JCK_DET_R_EINT);
	case WM8350_IRQ_CODEC_MICSCD:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_MICSCD_EINT);
	case WM8350_IRQ_CODEC_MICD:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_CODEC_MICD_EINT);
	case WM8350_IRQ_WKUP_OFF_STATE:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_OFF_STATE_EINT);
	case WM8350_IRQ_WKUP_HIB_STATE:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_HIB_STATE_EINT);
	case WM8350_IRQ_WKUP_CONV_FAULT:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_CONV_FAULT_EINT);
	case WM8350_IRQ_WKUP_WDOG_RST:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_OFF_STATE_EINT);
	case WM8350_IRQ_WKUP_GP_PWR_ON:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_GP_PWR_ON_EINT);
	case WM8350_IRQ_WKUP_ONKEY:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_ONKEY_EINT);
	case WM8350_IRQ_WKUP_GP_WAKEUP:
		return wm8350_clear_bits(wm8350, WM8350_COMPARATOR_INTERRUPT_STATUS_MASK,
			WM8350_IM_WKUP_GP_WAKEUP_EINT);
	case WM8350_IRQ_GPIO(0):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP0_EINT);
	case WM8350_IRQ_GPIO(1):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP1_EINT);
	case WM8350_IRQ_GPIO(2):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP2_EINT);
	case WM8350_IRQ_GPIO(3):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP3_EINT);
	case WM8350_IRQ_GPIO(4):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP4_EINT);
	case WM8350_IRQ_GPIO(5):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP5_EINT);
	case WM8350_IRQ_GPIO(6):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP6_EINT);
	case WM8350_IRQ_GPIO(7):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP7_EINT);
	case WM8350_IRQ_GPIO(8):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP8_EINT);
	case WM8350_IRQ_GPIO(9):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP9_EINT);
	case WM8350_IRQ_GPIO(10):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP10_EINT);
	case WM8350_IRQ_GPIO(11):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP11_EINT);
	case WM8350_IRQ_GPIO(12):
		return wm8350_clear_bits(wm8350, WM8350_GPIO_INTERRUPT_STATUS_MASK,
			WM8350_IM_GP12_EINT);
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(wm8350_unmask_irq);

static int gpio_set_dir(struct wm8350 *wm8350, int gpio, int dir)
{
	int ret;

	wm8350_reg_unlock(wm8350);
	if (dir == WM8350_GPIO_DIR_OUT)
		ret = wm8350_clear_bits(wm8350,
			WM8350_GPIO_CONFIGURATION_I_O, 1 << gpio);
	else
		ret = wm8350_set_bits(wm8350,
			WM8350_GPIO_CONFIGURATION_I_O, 1 << gpio);
	wm8350_reg_lock(wm8350);
	return ret;
}

static int gpio_set_debounce(struct wm8350 *wm8350, int gpio, int db)
{
	if (db == WM8350_GPIO_DEBOUNCE_ON)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_DEBOUNCE, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_DEBOUNCE, 1 << gpio);
}

static int gpio_set_func(struct wm8350 *wm8350, int gpio, int func)
{
	u16 reg;

	wm8350_reg_unlock(wm8350);
	switch (gpio) {
	case 0:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_1)
			& ~WM8350_GP0_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_1,
			reg | ((func & 0xf) << 0));
		break;
	case 1:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_1)
			& ~WM8350_GP1_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_1,
			reg | ((func & 0xf) << 4));
		break;
	case 2:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_1)
			& ~WM8350_GP2_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_1,
			reg | ((func & 0xf) << 8));
		break;
	case 3:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_1)
			& ~WM8350_GP3_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_1,
			reg | ((func & 0xf) << 12));
		break;
	case 4:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_2)
			& ~WM8350_GP4_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_2,
			reg | ((func & 0xf) << 0));
		break;
	case 5:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_2)
			& ~WM8350_GP5_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_2,
			reg | ((func & 0xf) << 4));
		break;
	case 6:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_2)
			& ~WM8350_GP6_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_2,
			reg | ((func & 0xf) << 8));
		break;
	case 7:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_2)
			& ~WM8350_GP7_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_2,
			reg | ((func & 0xf) << 12));
		break;
	case 8:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3)
			& ~WM8350_GP8_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3,
			reg | ((func & 0xf) << 0));
		break;
	case 9:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3)
			& ~WM8350_GP9_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3,
			reg | ((func & 0xf) << 4));
		break;
	case 10:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3)
			& ~WM8350_GP10_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3,
			reg | ((func & 0xf) << 8));
		break;
	case 11:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_3)
			& ~WM8350_GP11_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_3,
			reg | ((func & 0xf) << 12));
		break;
	case 12:
		reg = wm8350_reg_read(wm8350, WM8350_GPIO_FUNCTION_SELECT_4)
			& ~WM8350_GP12_FN_MASK;
		wm8350_reg_write(wm8350, WM8350_GPIO_FUNCTION_SELECT_4,
			reg | ((func & 0xf) << 0));
		break;
	default:
		wm8350_reg_lock(wm8350);
		return -EINVAL;
	}

	wm8350_reg_lock(wm8350);
	return 0;
}

int wm8350_gpio_set_status(struct wm8350 *wm8350, int gpio, int status)
{
	if (status)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_PIN_STATUS, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_PIN_STATUS, 1 << gpio);
}
EXPORT_SYMBOL_GPL(wm8350_gpio_set_status);

int wm8350_gpio_get_status(struct wm8350 *wm8350, int gpio)
{
	return (wm8350_reg_read(wm8350, WM8350_GPIO_PIN_STATUS) &
		(1 << gpio)) ? 1: 0;
}
EXPORT_SYMBOL_GPL(wm8350_gpio_get_status);

static int gpio_set_pull_up(struct wm8350 *wm8350, int gpio, int up)
{
	if (up)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_PIN_PULL_UP_CONTROL, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_PIN_PULL_UP_CONTROL, 1 << gpio);
}

static int gpio_set_pull_down(struct wm8350 *wm8350, int gpio, int down)
{
	if (down)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_PULL_DOWN_CONTROL, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_PULL_DOWN_CONTROL, 1 << gpio);
}

static int gpio_set_polarity(struct wm8350 *wm8350, int gpio, int pol)
{
	if (pol == WM8350_GPIO_ACTIVE_HIGH)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_PIN_POLARITY_TYPE, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_PIN_POLARITY_TYPE, 1 << gpio);
}

static int gpio_set_invert(struct wm8350 *wm8350, int gpio, int invert)
{
	if (invert == WM8350_GPIO_INVERT_ON)
		return wm8350_set_bits(wm8350,
			WM8350_GPIO_INTERRUPT_MODE, 1 << gpio);
	else
		return wm8350_clear_bits(wm8350,
			WM8350_GPIO_INTERRUPT_MODE, 1 << gpio);
}

int wm8350_gpio_config(struct wm8350 *wm8350, int gpio, int dir, int func,
	int pol, int pull, int invert, int debounce)
{
	/* make sure we never pull up and down at the same time */
	if (pull == WM8350_GPIO_PULL_NONE) {
		if (gpio_set_pull_up(wm8350, gpio, 0))
			goto err;
		if (gpio_set_pull_down(wm8350, gpio, 0))
			goto err;
	} else if (pull == WM8350_GPIO_PULL_UP) {
		if (gpio_set_pull_down(wm8350, gpio, 0))
			goto err;
		if (gpio_set_pull_up(wm8350, gpio, 1))
			goto err;
	} else if (pull == WM8350_GPIO_PULL_DOWN) {
		if (gpio_set_pull_up(wm8350, gpio, 0))
			goto err;
		if (gpio_set_pull_down(wm8350, gpio, 1))
			goto err;
	}

	if (gpio_set_invert(wm8350, gpio, invert))
		goto err;
	if (gpio_set_polarity(wm8350, gpio, pol))
		goto err;
	if (gpio_set_debounce(wm8350, gpio, debounce))
		goto err;
	if (gpio_set_dir(wm8350, gpio, dir))
		goto err;
	return gpio_set_func(wm8350, gpio, func);

err:
	return -EIO;
}
EXPORT_SYMBOL_GPL(wm8350_gpio_config);

int wm8350_read_auxadc(struct wm8350 *wm8350, int channel, int scale, int vref)
{
	u16 reg, result = 0;
	int tries = 5;

	if (channel < WM8350_AUXADC_AUX1 || channel > WM8350_AUXADC_TEMP)
		return -EINVAL;
	if (channel >= WM8350_AUXADC_USB && channel <= WM8350_AUXADC_TEMP
		&& (scale != 0 || vref != 0))
		return -EINVAL;

	mutex_lock(&auxadc_mutex);

	/* Turn on the ADC */
	reg = wm8350_reg_read(wm8350, WM8350_POWER_MGMT_5);
	wm8350_reg_write(wm8350, WM8350_POWER_MGMT_5, reg | WM8350_AUXADC_ENA);

	if (scale || vref) {
		reg = scale << 13;
		reg |= vref << 12;
		wm8350_reg_write(wm8350, WM8350_AUX1_READBACK + channel, reg);
	}

	reg = wm8350_reg_read(wm8350, WM8350_DIGITISER_CONTROL_1);
	reg |= 1 << channel | WM8350_AUXADC_POLL;
	wm8350_reg_write(wm8350, WM8350_DIGITISER_CONTROL_1, reg);

	do {
		schedule_timeout_interruptible(1);
		reg = wm8350_reg_read(wm8350, WM8350_DIGITISER_CONTROL_1);
	} while (tries-- && (reg & WM8350_AUXADC_POLL));
	
	if (!tries)
		printk (KERN_ERR "wm8350: adc chn %d read timeout\n", channel);
	else
		result = wm8350_reg_read(wm8350, 
			WM8350_AUX1_READBACK + channel);

	/* Turn off the ADC */
	reg=wm8350_reg_read(wm8350, WM8350_POWER_MGMT_5);
	wm8350_reg_write(wm8350, WM8350_POWER_MGMT_5, reg & ~WM8350_AUXADC_ENA);

	mutex_unlock(&auxadc_mutex);
	return result & WM8350_AUXADC_DATA1_MASK;
}
EXPORT_SYMBOL_GPL(wm8350_read_auxadc);

static void wm8350_pmic_dev_release(struct device *dev){}

int wm8350_device_register_pmic(struct wm8350 *wm8350)
{
	int ret;

	strcpy(wm8350->pmic.dev.bus_id, "wm8350-pmic");
	wm8350->pmic.dev.bus = &wm8350_bus_type;
	wm8350->pmic.dev.parent = &wm8350->i2c_client->dev;
	wm8350->pmic.dev.release = wm8350_pmic_dev_release;

	ret = device_register(&wm8350->pmic.dev);
	if (ret < 0)
		printk(KERN_ERR "failed to register WM8350 PMIC device\n");

	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_device_register_pmic);

static void wm8350_rtc_dev_release(struct device *dev){}

int wm8350_device_register_rtc(struct wm8350 *wm8350)
{
	int ret;

	strcpy(wm8350->rtc.dev.bus_id, "wm8350-rtc");
	wm8350->rtc.dev.bus = &wm8350_bus_type;
	wm8350->rtc.dev.parent = &wm8350->i2c_client->dev;
	wm8350->rtc.dev.release = wm8350_rtc_dev_release;

	ret = device_register(&wm8350->rtc.dev);
	if (ret < 0)
		printk(KERN_ERR "failed to register WM8350 RTC device\n");

	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_device_register_rtc);

static void wm8350_wdg_dev_release(struct device *dev){}

int wm8350_device_register_wdg(struct wm8350 *wm8350)
{
	int ret;

	strcpy(wm8350->wdg.dev.bus_id, "wm8350-wdt");
	wm8350->wdg.dev.bus = &wm8350_bus_type;
	wm8350->wdg.dev.parent = &wm8350->i2c_client->dev;
	wm8350->wdg.dev.release = wm8350_wdg_dev_release;

	ret = device_register(&wm8350->wdg.dev);
	if (ret < 0)
		printk(KERN_ERR "failed to register WM8350 Watchdog device\n");

	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_device_register_wdg);

static void wm8350_power_dev_release(struct device *dev){}

int wm8350_device_register_power(struct wm8350 *wm8350)
{
	int ret;

	strcpy(wm8350->power.dev.bus_id, "wm8350-power");
	wm8350->power.dev.bus = &wm8350_bus_type;
	wm8350->power.dev.parent = &wm8350->i2c_client->dev;
	wm8350->power.dev.release = wm8350_power_dev_release;

	ret = device_register(&wm8350->power.dev);
	if (ret < 0)
		printk(KERN_ERR "failed to register WM8350 Power device\n");

	return ret;
}
EXPORT_SYMBOL_GPL(wm8350_device_register_power);

static int wm8350_bus_match(struct device *dev, struct device_driver *drv)
{
	if(!strcmp(dev->bus_id, drv->name))
		return 1;
	return 0;
}

static int wm8350_bus_suspend(struct device *dev, pm_message_t state)
{
	int ret = 0;

	if (dev->driver && dev->driver->suspend)
		ret = dev->driver->suspend(dev, state);

	return ret;
}

static int wm8350_bus_resume(struct device *dev)
{
	int ret = 0;

	if (dev->driver && dev->driver->resume)
		ret = dev->driver->resume(dev);

	return ret;
}

struct bus_type wm8350_bus_type = {
	.name		= "wm8350",
	.match		= wm8350_bus_match,
	.suspend	= wm8350_bus_suspend,
	.resume		= wm8350_bus_resume,
};
EXPORT_SYMBOL(wm8350_bus_type);

MODULE_AUTHOR("Liam Girdwood, liam.girdwood@wolfsonmicro.com, www.wolfsonmicro.com");
MODULE_DESCRIPTION("WM8350 PMIC Bus driver");
MODULE_LICENSE("GPL");
