/* drivers/i2c/chips/clearpad.c
 *
 * Copyright 2008 Logitech
 *	Richard Titmuss <richard_titmuss@logitech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>


static struct i2c_driver clearpad_driver;

struct clearpad_data {
	struct i2c_client	*client;
	struct input_dev	*input_dev;

	struct mutex		lock;
	struct work_struct	irq_work;

	char			product_id[0x1F];
	s32			max_x;
	s32			max_y;
	u8			resolution;
};

#define RMI_ADDR_STATUS      0x00
#define RMI_ADDR_Z           0x01
#define RMI_ADDR_XH          0x02
#define RMI_ADDR_XL          0x03
#define RMI_ADDR_YH          0x04
#define RMI_ADDR_YL          0x05
#define RMI_ADDR_DELTA_X     0x06
#define RMI_ADDR_DELTA_Y     0x07
#define RMI_ADDR_GESTURE     0x08
#define RMI_ADDR_MAGNITUDE   0x09


#define work_to_data(_w) container_of(_w, struct clearpad_data, irq_work)

static void clearpad_irq_work(struct work_struct *work)
{
	struct clearpad_data *data = work_to_data(work);
	struct i2c_client *client = data->client;
	u8 values[I2C_SMBUS_BLOCK_MAX];
	int status, width, pressure, gesture, flick;
	int x, y, xmag, ymag;

	i2c_smbus_read_i2c_block_data(client, 0x00, 0x0A, values);

	status = (values[RMI_ADDR_STATUS] & 0x07);
	gesture = (values[RMI_ADDR_GESTURE] & 0x80) >> 7;

	/* number of fingers */
	input_report_abs(data->input_dev, ABS_MISC, status);

	if (status >= 1 ) {
		width = (values[RMI_ADDR_STATUS] & 0xf0) >> 4;
		pressure = values[RMI_ADDR_Z];
		x = ((values[RMI_ADDR_XH] & 0x1f) << 8) | values[RMI_ADDR_XL];
		y = ((values[RMI_ADDR_YH] & 0x1f) << 8) | values[RMI_ADDR_YL];

		/* button down on finger contact */
		input_report_abs(data->input_dev, ABS_X, data->max_x - x);
		input_report_abs(data->input_dev, ABS_Y, y);
		input_report_abs(data->input_dev, ABS_PRESSURE, pressure);
		input_report_abs(data->input_dev, ABS_TOOL_WIDTH, width);

		input_report_rel(data->input_dev, REL_X, (s8) values[RMI_ADDR_DELTA_X]);
		input_report_rel(data->input_dev, REL_Y, (s8) values[RMI_ADDR_DELTA_Y]);

		//printk("%d,%d F%d W%d P%d\n", x,y, status, width, pressure);
	}

	if (gesture) {
		flick = (values[RMI_ADDR_GESTURE] & 0x10) >> 4;
		
		if (flick) {
			xmag = ((s8)(values[RMI_ADDR_MAGNITUDE] << 4)) >> 4;
			ymag = ((s8)values[RMI_ADDR_MAGNITUDE]) >> 4;

			input_report_rel(data->input_dev, REL_RX, xmag);
			input_report_rel(data->input_dev, REL_RY, ymag);

			//printk("FLICK %d,%d\n", xmag, ymag);
		}
	}

	input_sync(data->input_dev);

	enable_irq(client->irq);
}


static irqreturn_t clearpad_irq(int irq, void *dev_id)
{
	struct clearpad_data *data = dev_id;

	schedule_work(&data->irq_work);
	disable_irq(irq);

	return IRQ_HANDLED;
}


static int clearpad_init_client(struct i2c_client *client)
{
	struct clearpad_data *data = i2c_get_clientdata(client);
	u8 values[I2C_SMBUS_BLOCK_MAX];
	u8 reg;

	/* Check Manufacturer id */
	i2c_smbus_write_byte_data(client, 0xFF, 0x04);
	if (i2c_smbus_read_byte_data(client, 0xE1) != 0x01) {
		return 0;
	}

	/* Check for 2-D sensor pad */
	i2c_smbus_write_byte_data(client, 0xFF, 0x03);
	if (i2c_smbus_read_byte_data(client, 0x10) == 0x00) {
		return 0;
	}

	/* Check Product ID */
	i2c_smbus_write_byte_data(client, 0xFF, 0x02);

	i2c_smbus_read_i2c_block_data(client, 0x10, 16, values);
	memcpy(data->product_id, values, sizeof(data->product_id));

	/* Check Sensor Query Registers */
	i2c_smbus_write_byte_data(client, 0xFF, 0x10);

	i2c_smbus_read_i2c_block_data(client, 0x00, 8, values);

	data->max_x = (values[4] & 0x1F) << 8 | (values[5]);
	data->max_y = (values[6] & 0x1F) << 8 | (values[7]);
	data->resolution = values[8];
	
	printk(KERN_INFO "Clearpad: %s (%d,%d)\n", data->product_id, data->max_x, data->max_y);

	/* Enable Attention IRQ */
	i2c_smbus_write_byte_data(client, 0xFF, 0x00);
	reg = i2c_smbus_read_byte_data(client, 0x01);
	reg |= 0x07;
	i2c_smbus_write_byte_data(client, 0x01, reg);

	/* Restore to Data page */
	i2c_smbus_write_byte_data(client, 0xFF, 0x04);

	INIT_WORK(&data->irq_work, clearpad_irq_work);

	return 1;
}


int clearpad_probe(struct i2c_client *client)
{
	struct clearpad_data *data;
	struct input_dev *input_dev;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "No SMBUS byte data capability\n");
		goto error0;
	}

	if (!(data = kzalloc(sizeof(struct clearpad_data), GFP_KERNEL))) {
		dev_err(&client->dev, "No space for state\n");
		err = -ENOMEM;
		goto error0;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	mutex_init(&data->lock);

	/* Initialize the ClearPad */
	if (!clearpad_init_client(client)) {
		goto error1;
	}

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		dev_err(&client->dev, "Could not allocate input device\n");
		goto error1;
	}

	input_dev->name = "Synaptics ClearPad";
	input_dev->phys = data->product_id;
	input_dev->dev.parent = &client->dev;

	input_dev->evbit[0] = BIT(EV_ABS) | BIT(EV_REL);
	set_bit(REL_X, input_dev->relbit);
	set_bit(REL_Y, input_dev->relbit);
	set_bit(REL_RX, input_dev->relbit);
	set_bit(REL_RY, input_dev->relbit);
	input_set_abs_params(input_dev, ABS_X, 0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 0xFF, 0, 0);
	input_set_abs_params(input_dev, ABS_TOOL_WIDTH, 0, 0x0F, 0, 0);
	input_set_abs_params(input_dev, ABS_MISC, 0, 0x03, 0, 0);

	data->input_dev = input_dev;

	err = input_register_device(input_dev);
	if (err != 0) {
		dev_err(&client->dev, "Failed to register input device\n");
		input_free_device(input_dev);
		goto error1;
	}

	err = request_irq(client->irq, clearpad_irq,
			  IRQF_TRIGGER_LOW | IRQF_SAMPLE_RANDOM,
			  client->dev.bus_id, data);

	if (err != 0) {
		dev_err(&client->dev, "Failed to register IRQ %d\n",
			client->irq);
		goto error2;
	}

	return 0;

 error2:
	input_unregister_device(input_dev);
 error1:
	kfree(data);
 error0:
	return err;
}


int clearpad_remove(struct i2c_client *client)
{
	struct clearpad_data *data = i2c_get_clientdata(client);
	int err;

	free_irq(client->irq, data);

	input_unregister_device(data->input_dev);

	/* Try to detach the client from i2c space */
	if ((err = i2c_detach_client(client))) {
		dev_err(&client->dev,
			"Client deregistration failed, not detached.\n");
		return err;
	}

	kfree(client);

	return 0;
}


static struct i2c_driver clearpad_driver = {
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "clearpad",
	},
	.probe	= clearpad_probe,
	.remove	= clearpad_remove,
};


static int __init clearpad_init(void)
{
	return i2c_add_driver(&clearpad_driver);
}


static void __exit clearpad_exit(void)
{
	i2c_del_driver(&clearpad_driver);
}


MODULE_AUTHOR ("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("Synatics ClearPad driver");

module_init(clearpad_init)
module_exit(clearpad_exit)
