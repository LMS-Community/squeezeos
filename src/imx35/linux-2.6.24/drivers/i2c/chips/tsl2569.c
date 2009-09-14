/*
 *  tsl2569.c - Linux kernel modules for ambient light sensor
 *
 *  Copyright (C) 2008 Logitech
 *       Richard Titmuss <richard_titmuss@logitech.com>
 *
 *  Some parts based on tsl2550.c:
 *  Copyright (C) 2007 Rodolfo Giometti <giometti@linux.it>
 *  Copyright (C) 2007 Eurotech S.p.A. <info@eurotech.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define TSL2569_DRV_NAME	"tsl2569"
#define DRIVER_VERSION		"1.0"

/*
 * Defines
 */

#define TSL2569_REG_CONTROL		0x80
#define TSL2569_REG_TIMING		0x81
#define TSL2569_REG_PARTNO		0x8A
#define TSL2569_REG_ADC0		0x9C
#define TSL2569_REG_ADC1		0x9E

#define TSL2569_PARTNO			0xB0

#define TSL2569_POWER_DOWN		0x00
#define TSL2569_POWER_UP		0x03

/*
 * Structs
 */

struct tsl2569_data {
	struct i2c_client *client;
	struct mutex update_lock;

	unsigned int power_state : 1;
	unsigned int gain : 1;  // 0: 1X , 1: 16X
	unsigned int integration : 2; // 0:13.7mS, 1:100mS, 2:402mS, 3:Manual
	unsigned int manual_cycle_state : 1; // 0: manual cycle stopped, 1: manual cycle started

	unsigned int factor;
};

/*
 * Management functions
 */

static int tsl2569_set_power_state(struct i2c_client *client, int state)
{
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	if (state == 0) {
		ret = i2c_smbus_write_byte_data(client, TSL2569_REG_CONTROL, TSL2569_POWER_UP);
	} else {
		ret = i2c_smbus_write_byte_data(client, TSL2569_REG_CONTROL, TSL2569_POWER_DOWN);
	}

	data->power_state = state;

	return ret;
}

static int tsl2569_get_adc_value(struct i2c_client *client, u8 cmd)
{
	int ret = 0;
	u8 values[2];

	ret = i2c_smbus_read_i2c_block_data(client, cmd, 2, values);
	if (ret < 0)
		return ret;

	ret = (values[1] << 8) | values[0];	
	return ret;
}

static int tsl2569_set_timing(struct i2c_client *client)
{
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	u8 value;
	value = ((data->gain & 0x01) << 4) 
		| (data->integration & 0x03);

	printk("tsl2569: set timing: (%d,%d): %X\n", data->gain, data->integration, value);

	ret = i2c_smbus_write_byte_data(client, TSL2569_REG_TIMING, value);
	return ret;	
}

static int tsl2569_integ_cycle(struct i2c_client *client, u8 cmd)
{
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	u8 value;
	value = ((data->gain & 0x01) << 4) 
		| ((cmd & 0x01) << 3)
		| (data->integration & 0x03);
	printk("tsl2569: integration cycle: (%d,%d,%d): %X\n", data->gain, data->integration, cmd, value);

	ret = i2c_smbus_write_byte_data(client, TSL2569_REG_TIMING, value);
	return ret;
}

/*
 * sysfs access functions
 */

// rjuchli: Forward Declarations for TAOS Inc. Code 
unsigned int CalculateLux(unsigned int iFactor, unsigned int iGain, unsigned int tInt, unsigned int ch0, unsigned int ch1, int iType);

static int tsl2569_get_lux_value(struct i2c_client *client, u8 cmd)
{
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ch0, ch1;
	unsigned int lux;

	ch0 = tsl2569_get_adc_value(client, TSL2569_REG_ADC0);
	if (ch0 < 0)
		return ch0;

	ch1 = tsl2569_get_adc_value(client, TSL2569_REG_ADC1);
	if (ch1 < 0)
		return ch1;

	/*
	 * Calculating the Lux Value using the current parameters
	 * from the data structure
	 */
	lux = CalculateLux(data->factor, data->gain, data->integration, ch0, ch1, 0);

	return lux;
}


/*
 * SysFS support
 */

static ssize_t tsl2569_show_factor(struct device* dev, 
		struct device_attribute *attr, char *buf)
{
	struct tsl2569_data* data = i2c_get_clientdata(to_i2c_client(dev));
	return sprintf(buf, "%i\n", data->factor);
}

static ssize_t tsl2569_store_factor(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) 
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);

	// Make sure it stays within unsigned int
	if(val < 0 || val > 65536) 
		return -EINVAL;

	data->factor = val;

	printk("Store Factor: %d\n", data->factor);

	return count;
}

static DEVICE_ATTR(factor, S_IWUSR | S_IRUGO,
		   tsl2569_show_factor, tsl2569_store_factor);

static ssize_t tsl2569_show_power_state(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tsl2569_data *data = i2c_get_clientdata(to_i2c_client(dev));

	return sprintf(buf, "%u\n", data->power_state);
}

static ssize_t tsl2569_store_power_state(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 1)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	ret = tsl2569_set_power_state(client, val);
	mutex_unlock(&data->update_lock);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(power_state, S_IWUSR | S_IRUGO,
		   tsl2569_show_power_state, tsl2569_store_power_state);

static ssize_t tsl2569_show_adc(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret, ch0, ch1;

	/* No LUX data if not operational */
	if (!data->power_state)
		return -EBUSY;

	mutex_lock(&data->update_lock);

	ch0 = tsl2569_get_adc_value(client, TSL2569_REG_ADC0);
	if (ch0 < 0)
		return ch0;

	ch1 = tsl2569_get_adc_value(client, TSL2569_REG_ADC1);
	if (ch1 < 0)
		return ch1;

	ret = 0; // FIXME tsl2569_calculate_lux(ch0, ch1);
	if (ret < 0)
		return ret;

	ret = sprintf(buf, "%d,%d\n", ch0, ch1);

	mutex_unlock(&data->update_lock);

	return ret;
}

static DEVICE_ATTR(adc, S_IRUGO,
		   tsl2569_show_adc, NULL);


static ssize_t tsl2569_show_lux(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret, lux;

	/* No LUX data if not operational */
	if (!data->power_state)
		return -EBUSY;

	mutex_lock(&data->update_lock);

	lux = tsl2569_get_lux_value(client, TSL2569_REG_ADC0);
	if (lux < 0)
		return lux;

	ret = sprintf(buf, "%d\n", lux);

	mutex_unlock(&data->update_lock);

	return ret;
}

static DEVICE_ATTR(lux, S_IRUGO,
		   tsl2569_show_lux, NULL);

static ssize_t tsl2569_show_gain(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);

	ret = sprintf(buf, "%d\n", data->gain);

	mutex_unlock(&data->update_lock);
	return ret;
}

static ssize_t tsl2569_store_gain(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 1)
		return -EINVAL;

	data->gain = val;

	mutex_lock(&data->update_lock);
	
	// Update the Timing Register
	ret = tsl2569_set_timing(client);
	if(ret < 0) 
		return ret;

	printk("Store Gain: %d\n", data->gain);

	mutex_unlock(&data->update_lock);

	return count;
}
	

static DEVICE_ATTR(gain, S_IWUSR | S_IRUGO,
		tsl2569_show_gain, tsl2569_store_gain);


static ssize_t tsl2569_show_integration(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);

	ret = sprintf(buf, "%d\n", data->integration);

	mutex_unlock(&data->update_lock);
	return ret;
}

static ssize_t tsl2569_store_integration(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 3)
		return -EINVAL;

	data->integration = val;

	mutex_lock(&data->update_lock);
	
	// Update the Timing Register
	ret = tsl2569_set_timing(client);
	if(ret < 0) 
		return ret;

	printk("Store Integration: %d\n", data->integration);

	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(integration, S_IWUSR | S_IRUGO,
		tsl2569_show_integration, tsl2569_store_integration);

static ssize_t tsl2569_show_integ_cycle(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);

	ret = sprintf(buf, "%d\n", data->manual_cycle_state);

	mutex_unlock(&data->update_lock);
	return ret;
}

static ssize_t tsl2569_store_integ_cycle(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tsl2569_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 1)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	if ( data->integration != 3 ) {
		// Integration not set to manual
		return -EINVAL;
	}
	
	// does this change the state?
	if(val != data->manual_cycle_state) {

		data->manual_cycle_state = val;

		// Update the Timing Register
		ret = tsl2569_integ_cycle(client, val);
		if(ret < 0) 
			return ret;

		printk("Manual Cycle: %d\n", data->manual_cycle_state);
	}

	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(manual_cycle_state, S_IWUSR | S_IRUGO,
		tsl2569_show_integ_cycle, tsl2569_store_integ_cycle);


static struct attribute *tsl2569_attributes[] = {
	&dev_attr_power_state.attr,
	&dev_attr_adc.attr,
	&dev_attr_lux.attr,
	&dev_attr_gain.attr,
	&dev_attr_integration.attr,
	&dev_attr_manual_cycle_state.attr,
	&dev_attr_factor.attr,
	NULL
};

static const struct attribute_group tsl2569_attr_group = {
	.attrs = tsl2569_attributes,
};

/*
 * Initialization function
 */

static int tsl2569_init_client(struct i2c_client *client)
{
	struct tsl2569_data *data = i2c_get_clientdata(client);
	int err;

	/* Verify the part number */
	if ((i2c_smbus_read_byte_data(client, TSL2569_REG_PARTNO) & 0xF0) != TSL2569_PARTNO)
		return -ENODEV;

	/* Power up the device, and then read the status back */
	err = i2c_smbus_write_byte_data(client, TSL2569_REG_CONTROL, TSL2569_POWER_UP);
	if (err < 0)
		return err;

	mdelay(1);
	if ((i2c_smbus_read_byte_data(client, TSL2569_REG_CONTROL) & 0x03) != TSL2569_POWER_UP)
		return -ENODEV;

	data->power_state = 1;

	/* Initialize Gain / Integration time with default values */
	data->gain = 0; // 1X gain is default
	data->integration = 2; // 402ms is default
	data->factor = 1;

	return 0;
}

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver tsl2569_driver;
static int __devinit tsl2569_probe(struct i2c_client *client)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tsl2569_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct tsl2569_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->update_lock);

	/* Initialize the TSL2569 chip */
	err = tsl2569_init_client(client);
	if (err)
		goto exit_kfree;

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &tsl2569_attr_group);
	if (err)
		goto exit_kfree;

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);

	return 0;

exit_kfree:
	kfree(data);
exit:
	return err;
}

static int __devexit tsl2569_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &tsl2569_attr_group);

	/* Power down the device */
	tsl2569_set_power_state(client, 0);

	kfree(i2c_get_clientdata(client));

	return 0;
}

static struct i2c_driver tsl2569_driver = {
	.driver = {
		.name	= TSL2569_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= tsl2569_probe,
	.remove	= __devexit_p(tsl2569_remove),
};

static int __init tsl2569_init(void)
{
	return i2c_add_driver(&tsl2569_driver);
}

static void __exit tsl2569_exit(void)
{
	i2c_del_driver(&tsl2569_driver);
}

MODULE_AUTHOR("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("TSL2569 ambient light sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(tsl2569_init);
module_exit(tsl2569_exit);


// ======================================================================
// Lux Calculation Code follows taken directly out of the 
// specifications by TAOS, Inc.
// - rjuchli
// ======================================================================

//****************************************************************************
//
// Copyright 2004-2008 TAOS, Inc.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Module Name:
// lux.cpp
//
//****************************************************************************

#define LUX_SCALE 16 // scale by 2^16
#define RATIO_SCALE 9 // scale ratio by 2^9

//---------------------------------------------------
// Integration time scaling factors
//---------------------------------------------------

#define CH_SCALE 10 // scale channel values by 2^10
#define CHSCALE_TINT0 0x7517 // 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1 0x0fe7 // 322/81 * 2^CH_SCALE

//---------------------------------------------------
// T Package coefficients
//---------------------------------------------------
// For Ch1/Ch0=0.00 to 0.35:
// Lux=0.00763*Ch0-0.01031*Ch1
//
// For Ch1/Ch0=0.35 to 0.50:
// Lux=0.00817*Ch0-0.01188*Ch1
//
// For Ch1/Ch0=0.50 to 0.60:
// Lux=0.00723*Ch0-0.01000*Ch1
//
// For Ch1/Ch0=0.60 to 0.72:
// Lux=0.00573*Ch0-0.00750*Ch1
//
// For Ch1/Ch0=0.72 to 0.85:
// Lux=0.00216*Ch0-0.00254*Ch1
//
// For Ch1/Ch0>0.85:
// Lux/Ch0=0
//
//--------------------------------------------------

#define K1T 0x00b3 // 0.35 * 2^RATIO_SCALE
#define B1T 0x01f4 // 0.00763 * 2^LUX_SCALE
#define M1T 0x02a4 // 0.01031 * 2^LUX_SCALE

#define K2T 0x0100 // 0.50 * 2^RATIO_SCALE
#define B2T 0x0217 // 0.00817 * 2^LUX_SCALE
#define M2T 0x030a // 0.01188 * 2^LUX_SCALE

#define K3T 0x0133 // 0.60 * 2^RATIO_SCALE
#define B3T 0x01da // 0.00723 * 2^LUX_SCALE
#define M3T 0x028f // 0.01000 * 2^LUX_SCALE

#define K4T 0x0171 // 0.72 * 2^RATIO_SCALE
#define B4T 0x0177 // 0.00573 * 2^LUX_SCALE
#define M4T 0x01ec // 0.00750 * 2^LUX_SCALE

#define K5T 0x01b3 // 0.85 * 2^RATIO_SCALE
#define B5T 0x008d // 0.00216 * 2^LUX_SCALE
#define M5T 0x00a6 // 0.00254 * 2^LUX_SCALE

#define K6T 0x01b3 // 0.85 * 2^RATIO_SCALE
#define B6T 0x0000 // 0.00000 * 2^LUX_SCALE
#define M6T 0x0000 // 0.00000 * 2^LUX_SCALE


//---------------------------------------------------
// CS Package coefficients
//---------------------------------------------------
// For Ch1/Ch0=0.00 to 0.35:
// Lux=0.00713*Ch0-0.00975*Ch1
//
// For Ch1/Ch0=0.35 to 0.45:
// Lux=0.00813*Ch0-0.01250*Ch1
//
// For Ch1/Ch0=0.45 to 0.52:
// Lux=0.00935*Ch0-0.01521*Ch1
//
// For Ch1/Ch0=0.52 to 0.67:
// Lux=0.00394*Ch0-0.00482*Ch1
//
// For Ch1/Ch0=0.67 to 0.85:
// Lux=0.00337*Ch0-0.00396*Ch1
//
// For Ch1/Ch0>0.85:
// Lux/Ch0=0
//
//-------------------------------------------------
#define K1C 0x00b3 // 0.35 * 2^RATIO_SCALE
#define B1C 0x01d3 // 0.00713 * 2^LUX_SCALE
#define M1C 0x027f // 0.00975 * 2^LUX_SCALE

#define K2C 0x00e6 // 0.45 * 2^RATIO_SCALE
#define B2C 0x0214 // 0.00813 * 2^LUX_SCALE
#define M2C 0x0333 // 0.01250 * 2^LUX_SCALE

#define K3C 0x010a // 0.52 * 2^RATIO_SCALE
#define B3C 0x0265 // 0.00935 * 2^LUX_SCALE
#define M3C 0x03e5 // 0.01521 * 2^LUX_SCALE

#define K4C 0x0157 // 0.67 * 2^RATIO_SCALE
#define B4C 0x0102 // 0.00394 * 2^LUX_SCALE
#define M4C 0x013c // 0.00482 * 2^LUX_SCALE

#define K5C 0x01b3 // 0.85 * 2^RATIO_SCALE
#define B5C 0x00dd // 0.00337 * 2^LUX_SCALE
#define M5C 0x0104 // 0.00396 * 2^LUX_SCALE

#define K6C 0x01b3 // 0.85 * 2^RATIO_SCALE
#define B6C 0x0000 // 0.00000 * 2^LUX_SCALE
#define M6C 0x0000 // 0.00000 * 2^LUX_SCALE


// lux equation approximation without floating point calculations
//////////////////////////////////////////////////////////////////////////////
// Routine: unsigned int CalculateLux(unsigned int ch0, unsigned int ch0, int iType)
//
// Description: Calculate the approximate illuminance (lux) given the raw
// channel values of the TSL2568. The equation if implemented
// as a piece-wise linear approximation.
//
// Arguments: unsigned int iGain - gain, where 0:1X, 1:16X
// unsigned int tInt - integration time, where 0:13.7mS, 1:100mS, 2:402mS,
// 3:Manual
// unsigned int ch0 - raw channel value from channel 0 of TSL2568
// unsigned int ch1 - raw channel value from channel 1 of TSL2568
// unsigned int iType - package type (0:T, 1:CS)
//
// Return: unsigned int - the approximate illuminance (lux)
//
//////////////////////////////////////////////////////////////////////////////
unsigned int CalculateLux(unsigned int iFactor, unsigned int iGain, unsigned int tInt, unsigned int ch0, unsigned int ch1, int iType)
{
	//------------------------------------------------------------------------
	// first, scale the channel values depending on the gain and integration time
	// 16X, 402mS is nominal.
	// scale if integration time is NOT 402 msec
	unsigned long chScale;
	unsigned long channel1;
	unsigned long channel0;
	unsigned long ratio1 = 0;
	unsigned long ratio;
	unsigned int b, m;
	unsigned long temp;
	unsigned long lux;
	switch (tInt)
	{
		case 0: // 13.7 msec
			chScale = CHSCALE_TINT0;
			break;
		case 1: // 101 msec
			chScale = CHSCALE_TINT1;
			break;
		default: // assume no scaling
			chScale = (1 << CH_SCALE);
			break;
	}
	// scale if gain is NOT 16X
	if (!iGain) chScale = chScale << 4; // scale 1X to 16X

	// scale the channel values
	channel0 = (ch0 * chScale) >> CH_SCALE;
	channel1 = (ch1 * chScale) >> CH_SCALE;
	//-----------------------------------------------------------------

	// find the ratio of the channel values (Channel1/Channel0)
	// protect against divide by zero
	if (channel0 != 0) ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;
	// round the ratio value
	ratio = (ratio1 + 1) >> 1;

	// is ratio <= eachBreak ?
	switch (iType)
	{
		case 0: // T package
			if ((ratio >= 0) && (ratio <= K1T))
				{b=B1T; m=M1T;}
			else if (ratio <= K2T)
				{b=B2T; m=M2T;}
			else if (ratio <= K3T)
				{b=B3T; m=M3T;}
			else if (ratio <= K4T)
				{b=B4T; m=M4T;}
			else if (ratio <= K5T)
				{b=B5T; m=M5T;}
			else if (ratio > K6T)
				{b=B6T; m=M6T;}
			break;
		case 1: // CS package
			if ((ratio >= 0) && (ratio <= K1C))
				{b=B1C; m=M1C;}
			else if (ratio <= K2C)
				{b=B2C; m=M2C;}
			else if (ratio <= K3C)
				{b=B3C; m=M3C;}
			else if (ratio <= K4C)
				{b=B4C; m=M4C;}
			else if (ratio <= K5C)
				{b=B5C; m=M5C;}
			else if (ratio > K6C)
				{b=B6C; m=M6C;}
			break;
	}

	temp = ((channel0 * b * iFactor) - (channel1 * m * iFactor));

	// scale CS or T package
	// round lsb (2^(LUX_SCALE-1))
	temp += (1 << (LUX_SCALE-1));
	// strip off fractional portion
	lux = temp >> LUX_SCALE;
	return(lux);
}

