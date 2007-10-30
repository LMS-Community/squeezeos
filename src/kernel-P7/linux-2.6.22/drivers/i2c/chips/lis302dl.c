/* drivers/i2c/chips/lis302dl.c
 *
 * Copyright 2007 Logitech.
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
#include <linux/proc_fs.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <linux/seq_file.h>
#include <linux/debugfs.h>

/* registers */
#define LIS302DL_REG_WHO_AM_I          0x0F /* r */
#define LIS302DL_REG_CTRL_REG1         0x20 /* rw */
#define LIS302DL_REG_CTRL_REG2         0x21 /* rw */
#define LIS302DL_REG_CTRL_REG3         0x22 /* rw */
#define LIS302DL_REG_HP_FILTER_RESET   0x23 /* r */
#define LIS302DL_REG_STATUS_REG        0x27 /* r */
#define LIS302DL_REG_OUT_X             0x29 /* r */
#define LIS302DL_REG_OUT_Y             0x2B /* r */
#define LIS302DL_REG_OUT_Z             0x2D /* r */
#define LIS302DL_REG_FF_WU_CFG_1       0x30 /* rw */
#define LIS302DL_REG_FF_WU_SRC_1       0x31 /* r */
#define LIS302DL_REG_FF_WU_THS_1       0x32 /* rw */
#define LIS302DL_REG_FF_WU_DURATION_1  0x33 /* rw */
#define LIS302DL_REG_FF_WU_CFG_2       0x34 /* rw */
#define LIS302DL_REG_FF_WU_SRC_2       0x35 /* r */
#define LIS302DL_REG_FF_WU_THS_2       0x36 /* rw */
#define LIS302DL_REG_FF_WU_DURATION_2  0x37 /* rw */
#define LIS302DL_REG_CLICK_CFG         0x38 /* rw */
#define LIS302DL_REG_CLICK_SRC         0x39 /* r */
#define LIS302DL_REG_CLICK_THSY_X      0x3B /* rw */
#define LIS302DL_REG_CLICK_THSZ        0x3C /* rw */
#define LIS302DL_REG_CLICK_TIMELIMIT   0x3D /* rw */
#define LIS302DL_REG_CLICK_LATENCY     0x3E /* rw */
#define LIS302DL_REG_CLICK_WINDOW      0x3F /* rw */


struct lis302dl_data {
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	struct dentry		*dbg_dentry;

	unsigned int		 threshold;
	unsigned int		 duration;
	unsigned int		 resume_en : 1;

	struct mutex		 lock;
	struct work_struct	 irq_work;
};



/* Register access code */

static inline int lis302dl_read_value(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline int lis302dl_write_value(struct i2c_client *client, u8 reg,
				       u16 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}


#ifdef CONFIG_LIS302DL_DEBUG
static int lis302dl_debug_init(struct seq_file *s, void *unused)
{
	struct lis302dl_data *data = s->private;
	int i;

	static const u8 regs[] = {
		0x0F,
		0x20, 0x21, 0x22, 0x23, 0x27, 0x29, 0x2B, 0x2D,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
	};

	mutex_lock(&data->lock);

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		seq_printf(s, "r[%02x]=%02x\n",	regs[i],
			   lis302dl_read_value(data->client, regs[i]));
	}

	mutex_unlock(&data->lock);

	return 0;
}

static int lis032dl_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, lis302dl_debug_init, inode->i_private);
}

static const struct file_operations lis302dl_debug_fops = {
	.open		= lis032dl_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static inline void lis302dl_debug_create(struct lis302dl_data *data)
{
	data->dbg_dentry = debugfs_create_file(data->client->dev.bus_id,
					       S_IRUGO, NULL, data,
					       &lis302dl_debug_fops);
}

static inline void lis302dl_debug_remove(struct lis302dl_data *data)
{
	debugfs_remove(data->dbg_dentry);
}

#else
#define lis302dl_debug_create(_d) (0)
#define lis302dl_debug_remove(_d) do { } while (0)
#endif

#define work_to_data(_w) container_of(_w, struct lis302dl_data, irq_work)

static void lis302dl_irq_work(struct work_struct *work)
{
	struct lis302dl_data *data = work_to_data(work);
	struct i2c_client *client = data->client;
	s8 x, y, z;

	mutex_lock(&data->lock);

	/* clear irq */
	lis302dl_read_value(client, LIS302DL_REG_FF_WU_SRC_1);

	x = lis302dl_read_value(client, LIS302DL_REG_OUT_X);
	y = lis302dl_read_value(client, LIS302DL_REG_OUT_Y);
	z = lis302dl_read_value(client, LIS302DL_REG_OUT_Z);

	mutex_unlock(&data->lock);

	dev_dbg(&client->dev, "x=%d, y=%d, z=%d\n", x, y, z);

	input_report_abs(data->input_dev, ABS_X, x);
	input_report_abs(data->input_dev, ABS_Y, y);
	input_report_abs(data->input_dev, ABS_Z, z);
	input_sync(data->input_dev);

	enable_irq(client->irq);
}


static irqreturn_t lis302dl_irq(int irq, void *dev_id)
{
	struct lis302dl_data *data = dev_id;

	schedule_work(&data->irq_work);
	disable_irq(irq);

	return IRQ_HANDLED;
}

static void lis302dl_init_client(struct i2c_client *client)
{
	struct lis302dl_data *data = i2c_get_clientdata(client);

	/* PD=active, FS=0, Zen=enabled, Yen=enabled, Xen=enabled */
	lis302dl_write_value(client, LIS302DL_REG_CTRL_REG1, 0x47);

	/* HPF enabled on FF_WU1, fcutoff=2Hz */
	lis302dl_write_value(client, LIS302DL_REG_CTRL_REG2, 0x04);

	/* I1CFG=FF_WU_1 */
	lis302dl_write_value(client, LIS302DL_REG_CTRL_REG3, 0x01);

	/* threshold, duration */
	lis302dl_write_value(client, LIS302DL_REG_FF_WU_THS_1, data->threshold);
	lis302dl_write_value(client, LIS302DL_REG_FF_WU_DURATION_1, data->duration);

	/* reset HPF */
	lis302dl_read_value(client, LIS302DL_REG_HP_FILTER_RESET);

	/* LIR, ZHIE, YHIE, XHIE */
	lis302dl_write_value(client, LIS302DL_REG_FF_WU_CFG_1, 0x6A);

	INIT_WORK(&data->irq_work, lis302dl_irq_work);
}

static ssize_t lis302dl_threshold_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->threshold);
}

static ssize_t lis302dl_threshold_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);
	char *ptr;
	unsigned long val;

	val = simple_strtoul(buf, &ptr, 10);
	if (ptr == buf)
		return -EINVAL;

	if (val != data->threshold) {
		data->threshold = val;

		mutex_lock(&data->lock);

		lis302dl_write_value(client, LIS302DL_REG_FF_WU_THS_1, data->threshold);

		mutex_unlock(&data->lock);
	}

	return count;
}

static DEVICE_ATTR(threshold, 0644, lis302dl_threshold_show, lis302dl_threshold_store);

static ssize_t lis302dl_duration_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->duration);
}

static ssize_t lis302dl_duration_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);
	char *ptr;
	unsigned long val;

	val = simple_strtoul(buf, &ptr, 10);
	if (ptr == buf)
		return -EINVAL;

	if (val != data->duration) {
		data->duration = val;

		mutex_lock(&data->lock);

		lis302dl_write_value(client, LIS302DL_REG_FF_WU_DURATION_1, data->duration);

		mutex_unlock(&data->lock);
	}
	
	return count;
}

static DEVICE_ATTR(duration, 0644, lis302dl_duration_show, lis302dl_duration_store);


#ifdef CONFIG_PM
static ssize_t lis302dl_resume_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->resume_en);
}

static ssize_t lis302dl_resume_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis302dl_data *data = i2c_get_clientdata(client);
	char *ptr;
	unsigned long val;

	val = simple_strtoul(buf, &ptr, 10);
	if (ptr == buf)
		return -EINVAL;

	if (val && !data->resume_en) {
		data->resume_en = 1;
		enable_irq_wake(client->irq);
	} else if (!val && data->resume_en) {
		data->resume_en = 0;
		disable_irq_wake(client->irq);
	}

	return count;
}

static DEVICE_ATTR(resume, 0644, lis302dl_resume_show, lis302dl_resume_store);

static inline void lis302dl_attach_pm(struct lis302dl_data *data)
{
	struct i2c_client *client = data->client;
	int err;

	err = device_create_file(&client->dev, &dev_attr_resume);
	if (err)
		dev_err(&client->dev, "cannot attach resume attribute\n");
}
#else
static inline void lis302dl_attach_pm(struct lis302dl_data *data)
{
}
#endif


static int __devinit lis302dl_probe(struct i2c_client *client)
{
	struct lis302dl_data *data;
	struct input_dev *input_dev;
	int err = 0;

	dev_info(&client->dev, "LIS302DL, IRQ %d\n", client->irq);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "No SMBUS byte data capability\n");
		goto error0;
	}

	if (!(data = kmalloc(sizeof(struct lis302dl_data), GFP_KERNEL))) {
		dev_err(&client->dev, "No space for state\n");
		err = -ENOMEM;
		goto error0;
	}

	memset(data, 0, sizeof(struct lis302dl_data));
	data->threshold = 0x14; // 350mg default

	mutex_init(&data->lock);

	i2c_set_clientdata(client, data);
	data->client = client;

	if (lis302dl_read_value(client, LIS302DL_REG_WHO_AM_I) != 0x3B) {
		dev_err(&client->dev, "Device is not a LIS302DL\n");
		err = -ENODEV;
		goto error1;
	}

	/* Initialize the LIS302DL chip */
	lis302dl_init_client(client);

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		dev_err(&client->dev, "Could not allocate input device\n");
		goto error1;
	}

	input_dev->evbit[0] = BIT(EV_ABS);
	input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_Z);
	input_dev->absmin[ABS_X] = -127;
	input_dev->absmax[ABS_X] = 127;
	input_dev->absfuzz[ABS_X] = 4;
	input_dev->absflat[ABS_X] = 0;
	input_dev->absmin[ABS_Y] = -127;
	input_dev->absmax[ABS_Y] = 127;
	input_dev->absfuzz[ABS_Y] = 4;
	input_dev->absflat[ABS_Y] = 0;
	input_dev->absmin[ABS_Z] = -127;
	input_dev->absmax[ABS_Z] = 127;
	input_dev->absfuzz[ABS_Z] = 4;
	input_dev->absflat[ABS_Z] = 0;

	input_dev->name = "lis302dl";
	input_dev->dev.parent = &client->dev;

	data->input_dev = input_dev;

	err = input_register_device(input_dev);
	if (err != 0) {
		dev_err(&client->dev, "Failed to register input device\n");
		input_free_device(input_dev);
		goto error1;
	}

	err = request_irq(client->irq, lis302dl_irq,
			  IRQF_TRIGGER_HIGH | IRQF_SAMPLE_RANDOM,
			  client->dev.bus_id, data);

	if (err != 0) {
		dev_err(&client->dev, "Failed to register IRQ %d\n",
			client->irq);
		goto error2;
	}

	if ((err = device_create_file(&client->dev, &dev_attr_threshold)) ||
	    (err = device_create_file(&client->dev, &dev_attr_duration))) {
		dev_err(&client->dev, "cannot attach resume attribute\n");
	}

	lis302dl_attach_pm(data);
	lis302dl_debug_create(data);

	return 0;

 error2:
	input_unregister_device(input_dev);
 error1:
	kfree(data);
 error0:
	return err;
}

static int __devexit lis302dl_remove(struct i2c_client *client)
{
	struct lis302dl_data *data = i2c_get_clientdata(client);
	int err;

	lis302dl_debug_remove(data);

	if (data->resume_en)
		disable_irq_wake(client->irq);

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

static struct i2c_driver lis302dl_driver = {
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "lis302dl",
	},
	.probe		= lis302dl_probe,
	.remove		= __exit_p(lis302dl_remove),
};

static int __init lis302dl_init(void)
{
	return i2c_add_driver(&lis302dl_driver);
}


static void __exit lis302dl_exit(void)
{
	i2c_del_driver(&lis302dl_driver);
}


MODULE_AUTHOR ("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("lis302dl driver");

module_init(lis302dl_init)
module_exit(lis302dl_exit)
