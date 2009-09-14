/* drivers/mxc/fab4/fab4-ir.c
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
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/ctype.h>

#include <asm/arch/gpio.h>

#include "psoc.h"


/*
 * The IR receiver is implemented in a PIC that supports i2c.
 * To get the ir code, read 6 bytes at address 0x47; the format is:
 *
 * struct S_I2C_READ {
 *   U8 status;   // when bit 0 is set: we got a new ir code since last read
 *   U32 rc_code; // most recent ir code
 *   U8 fw_rev;   // 0x05 on the FW you have
 *   U8 density;  // 0-64
 * } i2c_read;
 *
 * status bits are automatically cleared after read;
 * you can use ATTN line to avoid polling but you still need to check the status field to know the source of the event.
 *
 * Note: ir decoding and i2c code are not yet synchronized and you may miss some event if the master reads at the same time a rc code is received; that will be fixed later.
 *
 * I use this to test in RedBoot:
 * >i2c_init 0 100000 1 6
 * >i2c 0x47
 * ---> 0x00 0x00 0x00 0x00 0x00 0x05 // status is 0 -> nothing; last byte is fw revision (0x05)
 *
 * beam code "7"
 * >i2c 0x47
 * ---> 0x01 0x76 0x89 0xa8 0x57 0x02 // status is 0x01 -> rc_code = 0x7689a857
 * >i2c 0x47 // let's read again
 *---> 0x00 0x76 0x89 0xa8 0x57 0x02 // status is 0x00 because previous read command cleared it but previous code is still present in the buffer
 */

/*
 * Defines
 */

#define FAB4_IR_REG_STATUS	0x00
#define FAB4_IR_REG_RC_CODE3	0x01
#define FAB4_IR_REG_RC_CODE2	0x02
#define FAB4_IR_REG_RC_CODE1	0x03
#define FAB4_IR_REG_RC_CODE0	0x04
#define FAB4_IR_REG_FW_REV	0x05
#define FAB4_IR_REG_DENSITY	0x06

#define FAB4_IR_REG_CONTROL	0x00
#define FAB4_IR_REG_BITS0	0x01
#define FAB4_IR_REG_BITS1	0x02
#define FAB4_IR_REG_BITS2	0x03
#define FAB4_IR_REG_BITS3	0x04
#define FAB4_IR_REG_BIT_COUNT	0x05
#define FAB4_IR_REG_HDR_PRE	0x06
#define FAB4_IR_REG_HDR_POST	0x07
#define FAB4_IR_REG_BIT0_PRE	0x08
#define FAB4_IR_REG_BIT0_POST	0x09
#define FAB4_IR_REG_BIT1_PRE	0x0A
#define FAB4_IR_REG_BIT2_POST	0x0B


#undef USE_TIMER 
#define TIMER_INTERVAL (HZ/20)


struct fab4_ir_data {
	struct i2c_client	*client;
	struct input_dev	*input_dev;

	struct mutex		lock;
	struct work_struct	irq_work;
#ifdef USE_TIMER
	struct timer_list	timer;
#endif

	/* psoc programming */
	iomux_pin_name_t	xres;
	iomux_pin_name_t	sdata;
	iomux_pin_name_t	sclk;

	u8			*program;
	u8			*secure;
	u8			*chksum;

	unsigned int proximity_control : 2;	// 0: stopped, 1: use IR_LED1, 2: use IR_LED2, 3: use both IR_LED1 and IR_LED2
	unsigned int proximity_duty_cycle: 3;	// 0: 6.3%, 1: 8.4%, 2: 11.3%, 3: 15.2%, 4: 20.5%, 5: 27.6%, 6: 37.2%, 7: 50.0%
};


#define MAX_INTEL_HEX_DATA_LENGTH 64

struct intel_hex_record {
	u32 length;
	u32 address;
	u32 type;
	u8 data[MAX_INTEL_HEX_DATA_LENGTH];
};

/*
 * Management functions
 */

static int fab4_ir_set_proximity_control(struct i2c_client *client)
{
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;

	u8 value;
	value = ((data->proximity_duty_cycle & 0x07) << 4)
		| (data->proximity_control & 0x03);

	printk("fab4 ir: set proximity control: (%d,%d): %X\n", data->proximity_duty_cycle, data->proximity_control, value);

	ret = i2c_smbus_write_byte_data(client, FAB4_IR_REG_CONTROL, value);
	return ret;
}

static int fab4_ir_set_proximity_duty_cycle(struct i2c_client *client)
{
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;

	u8 value;
	value = ((data->proximity_duty_cycle & 0x07) << 4)
		| (data->proximity_control & 0x03);

	printk("fab4 ir: set proximity duty cycle: (%d,%d): %X\n", data->proximity_duty_cycle, data->proximity_control, value);

	ret = i2c_smbus_write_byte_data(client, FAB4_IR_REG_CONTROL, value);
	return ret;
}

/*
 * SysFS support
 */

static ssize_t fab4_ir_show_proximity_control(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->lock);

	ret = sprintf(buf, "%d\n", data->proximity_control);

	mutex_unlock(&data->lock);
	return ret;
}

static ssize_t fab4_ir_store_proximity_control(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 3)
		return -EINVAL;

	data->proximity_control = val;

	mutex_lock(&data->lock);
	
	// Update the proximity control
	ret = fab4_ir_set_proximity_control(client);
	if(ret < 0)
		return ret;

	printk("Store Proximity control: %d\n", data->proximity_control);

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(proximity_control, S_IWUSR | S_IRUGO,
		fab4_ir_show_proximity_control, fab4_ir_store_proximity_control);

static ssize_t fab4_ir_show_proximity_duty_cycle(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->lock);

	ret = sprintf(buf, "%d\n", data->proximity_duty_cycle);

	mutex_unlock(&data->lock);
	return ret;
}

static ssize_t fab4_ir_store_proximity_duty_cycle(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 7)
		return -EINVAL;

	data->proximity_duty_cycle = val;

	mutex_lock(&data->lock);
	
	// Update the proximity duty cycle
	ret = fab4_ir_set_proximity_duty_cycle(client);
	if(ret < 0)
		return ret;

	printk("Store Proximity Duty Cycle: %d\n", data->proximity_duty_cycle);

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(proximity_duty_cycle, S_IWUSR | S_IRUGO,
		fab4_ir_show_proximity_duty_cycle, fab4_ir_store_proximity_duty_cycle);

static ssize_t fab4_ir_show_proximity_density(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;
	u8 values[7];

	mutex_lock(&data->lock);

// TODO: Fix reading density register direct
//	ret = i2c_smbus_read_byte_data(client, FAB4_IR_REG_DENSITY);
	ret = i2c_smbus_read_i2c_block_data(client, FAB4_IR_REG_STATUS, 0x07, values);
	if(ret < 0)
		return ret;

	ret = sprintf(buf, "%d\n", values[6]);

	mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_ATTR(proximity_density, S_IWUSR | S_IRUGO,
		fab4_ir_show_proximity_density, NULL);


static ssize_t fab4_ir_show_fw_rev(struct device  *dev,
			struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int ret;
	u8 values[6];

	mutex_lock(&data->lock);

// TODO: Fix reading fw revision register direct
//	ret = i2c_smbus_read_byte_data(client, FAB4_IR_REG_FW_REV);
	ret = i2c_smbus_read_i2c_block_data(client, FAB4_IR_REG_STATUS, 0x06, values);
	if(ret < 0)
		return ret;

	ret = sprintf(buf, "%d\n", values[5]);

	mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_ATTR(fw_rev, S_IWUSR | S_IRUGO,
		fab4_ir_show_fw_rev, NULL);

static struct attribute *fab4_ir_attributes[] = {
	&dev_attr_proximity_control,
	&dev_attr_proximity_duty_cycle,
	&dev_attr_proximity_density,

	&dev_attr_fw_rev,
	NULL
};

static const struct attribute_group fab4_ir_attr_group = {
	.attrs = fab4_ir_attributes,
};


static int intel_hex_parse_byte(char **ptr, u32 *cksum)
{
	char c[3];
	int v;

	c[0] = *((*ptr)++);
	c[1] = *((*ptr)++);
	c[2] = '\0';

	sscanf(c, "%x", &v);

	*cksum += v;
	return v;
}

static char *intel_hex_parse_record(char *ptr, struct intel_hex_record *record)
{
	int i;
	u32 cksum = 0;

	while (isspace(*ptr)) {
		ptr++;
	}

	if (*ptr++ != ':') {
		return NULL;
	}

	record->length = intel_hex_parse_byte(&ptr, &cksum);
	record->address = (intel_hex_parse_byte(&ptr, &cksum) << 8);
	record->address |= intel_hex_parse_byte(&ptr, &cksum);
	record->type = intel_hex_parse_byte(&ptr, &cksum);
	for (i=0; i<record->length; i++) {
		record->data[i] = intel_hex_parse_byte(&ptr, &cksum);
	}
	intel_hex_parse_byte(&ptr, &cksum); /* checksum */
	if (cksum & 0xFF) {
		printk(KERN_ERR "Invalid checksum");
		return NULL;
	}

	return ptr;
}

static int intel_hex_parse_firmware(struct fab4_ir_data *data, const struct firmware *fw)
{
	struct intel_hex_record record;
	char *cptr, *cend;
	u8 *dptr, *dend;
	int extended;

	cptr = fw->data;
	cend = fw->data + fw->size;

	dptr = data->program;
	dend = dptr + (64*64);

	while (cptr && cptr < cend) {
		cptr = intel_hex_parse_record(cptr, &record);

		switch (record.type) {
		case 0: /* Data */
			if (dptr + record.length > dend) {
				return -1;
			}
			memcpy(dptr, record.data, record.length);
			dptr += record.length;
			break;

		case 1: /* End record */
			return 0;

		case 4: /* Extended */
			extended = (record.data[0] << 8) | record.data[1];
			if (extended == 0x0010) {
				dptr = data->secure;
				dend = dptr + 64;
			}
			else if (extended == 0x0020) {
				dptr = data->chksum;
				dend = dptr + 2;
			}
			else {
				return -1;
			}
			break;

		default:
			return -1;
		}
	}

	return -1;
}

static void psoc_reset(struct fab4_ir_data *data)
{
	mxc_set_gpio_dataout(data->xres, 1);
	udelay(40);
	mxc_set_gpio_dataout(data->xres, 0);
	udelay(40);
}

static void psoc_clock(struct fab4_ir_data *data)
{
	mxc_set_gpio_dataout(data->sclk, 1);
	udelay(PSOC_CLOCK_UDELAY);
	mxc_set_gpio_dataout(data->sclk, 0);
	udelay(PSOC_CLOCK_UDELAY);
}

static void psoc_write_vector(struct fab4_ir_data *data, u32 vector)
{
	int i;
	u32 mask = 0x200000;

	mxc_set_gpio_direction(data->sdata, 0);

	for (i=0; i<22; i++) {
		mxc_set_gpio_dataout(data->sdata, vector & mask);
		mask >>= 1;

		psoc_clock(data);
	}
}

static void psoc_write_block(struct fab4_ir_data *data, u8 *block)
{
	int addr;

	for (addr=0; addr<64; addr++) {
		psoc_write_vector(data, 0x240007
				  | (addr << 11)
				  | (block[addr] << 3));
	}
}

static u8 psoc_read_byte(struct fab4_ir_data *data, u8 addr)
{
	int i;
	u32 cmd, mask = 0x400;
	u8 val = 0;

	cmd = 0x580 | addr;

	mxc_set_gpio_direction(data->sdata, 0);
	for (i=0; i<11; i++) {
		mxc_set_gpio_dataout(data->sdata, cmd & mask);
		psoc_clock(data);

		mask >>= 1;
	}

	mxc_set_gpio_direction(data->sdata, 1);
	psoc_clock(data);

	for (i=0; i<8; i++) {
		psoc_clock(data);

		val <<= 1;
		val |= mxc_get_gpio_datain(data->sdata);
	}

	psoc_clock(data);

	mxc_set_gpio_dataout(data->sdata, 1);
	mxc_set_gpio_direction(data->sdata, 0);
	psoc_clock(data);

	return val;
}

static int psoc_wait_and_poll(struct fab4_ir_data *data)
{
	int i;

	/* Clock Z to device */
	mxc_set_gpio_direction(data->sdata, 1);
	psoc_clock(data);

	/* Wait for HIGH to LOW transition */
	i = 0;
	while (mxc_get_gpio_datain(data->sdata) != 0) {
		i += PSOC_WAIT_MDELAY;
		if (i >= PSOC_WAIT_MTIMEOUT) {
			return -1;
		}

		msleep(PSOC_WAIT_MDELAY);
	}

	/* Apply a bit stream of 40 zero bits */
	mxc_set_gpio_dataout(data->sdata, 0);
	mxc_set_gpio_direction(data->sdata, 1);

	for (i=0; i<40; i++) {
		psoc_clock(data);
	}

	return 0;
}

static void psoc_send_vectors(struct fab4_ir_data *data, u32 vectors[], size_t len)
{
	int i;

	for (i=0; i<len; i++) {
		if (vectors[i] == WAIT_AND_POLL) {
			psoc_wait_and_poll(data);
		}
		else {
			psoc_write_vector(data, vectors[i]);
		}
	}
}

static void fab4_prog_firmware(const struct firmware *fw, void *context)
{
	struct fab4_ir_data *data = (struct fab4_ir_data *) context;
	struct i2c_client *client = data->client;

	u16 silicon_id, chksum;
	int block;
	unsigned long flags;

	if (fw == NULL) {
		printk(KERN_ERR "FAB4 IR: fw request failed\n");
		return;
	}

	// Disable interrupts as long as mcu is in programming mode.
	disable_irq(client->irq);

	data->program = kzalloc(64*64, GFP_KERNEL);
	data->secure = kzalloc(64, GFP_KERNEL);
	data->chksum = kzalloc(2, GFP_KERNEL);

	if (intel_hex_parse_firmware(data, fw) != 0) {
		printk(KERN_ERR "FAB4 IR: fw invalid\n");
		goto done;
	}

	mxc_set_gpio_direction(data->xres, 0);
	mxc_set_gpio_direction(data->sclk, 0);

	/* to enter the programming mode the reset and initialize vector
	 * must be completed in 125us. to ensure the timing we are bad and
	 * disable irq's here.
	 */

	/* reset target */
 	local_irq_save(flags);
	psoc_reset(data);

	/* initialize target */
	psoc_send_vectors(data, psoc_init, ARRAY_SIZE(psoc_init));
 	local_irq_restore(flags);

	/* verify silicon id */
	psoc_send_vectors(data, psoc_id_setup, ARRAY_SIZE(psoc_id_setup));
	silicon_id = psoc_read_byte(data, 0x78) << 8;
	silicon_id |= psoc_read_byte(data, 0x79);

	if (silicon_id != 0x19 /* CY8C21323 */) {
		printk(KERN_ERR "FAB4 IR: invalid silicon id: %x\n", silicon_id);
		goto done;
	}

	/* checksum */
	psoc_send_vectors(data, psoc_checksum_setup, ARRAY_SIZE(psoc_checksum_setup));
	chksum = psoc_read_byte(data, 0x79) << 8;
	chksum |= psoc_read_byte(data, 0x78);

	if (chksum == ((data->chksum[0] << 8) | data->chksum[1])) {
		printk(KERN_INFO "FAB4 IR: fw checksum ok %x\n", chksum);
		goto done;
	}

	printk(KERN_INFO "FAB4 IR: programming fw (checksum was %x)\n", chksum);

	/* bulk erase */
	psoc_send_vectors(data, psoc_bluk_erase, ARRAY_SIZE(psoc_bluk_erase));

	/* program */
	for (block=0; block<64; block++) {
		psoc_write_block(data, data->program + (block * 64));

		psoc_write_vector(data, 0x27D007 | (block << 3));
		psoc_send_vectors(data, psoc_program, ARRAY_SIZE(psoc_program));
	}

	/* secure */
	psoc_write_block(data, data->secure);
	psoc_send_vectors(data, psoc_secure, ARRAY_SIZE(psoc_secure));

	/* checksum */
	psoc_send_vectors(data, psoc_checksum_setup, ARRAY_SIZE(psoc_checksum_setup));
	chksum = psoc_read_byte(data, 0x79) << 8;
	chksum |= psoc_read_byte(data, 0x78);

	if (chksum != ((data->chksum[0] << 8) | data->chksum[1])) {
		printk(KERN_ERR "FAB4 IR: fw checksum failed %x\n", chksum);

		// FIXME retry programming?
		goto done;
	}

	printk(KERN_INFO "FAB4 IR: fw programming ok (checksum %x)\n", chksum);

 done:
	mxc_set_gpio_direction(data->sclk, 1);
	mxc_set_gpio_direction(data->sdata, 1);

	psoc_reset(data);
	mxc_set_gpio_direction(data->xres, 1);

	kfree(data->program);
	kfree(data->secure);
	kfree(data->chksum);

	// After switching mcu back from programming to regular mode it takes a
	//  while until it can answer i2c read or write requests again.
	// Delay enabling interrupts until mcu is ready. 4ms is an empiric value.
	//  (3ms is too short and results in i2c 'bus busy' error messages.)
	mdelay(4);
	enable_irq(client->irq);
}


#define irq_work_to_data(_w) container_of(_w, struct fab4_ir_data, irq_work)

static void fab4_ir_work(struct work_struct *work)
{
	struct fab4_ir_data *data = irq_work_to_data(work);
	struct i2c_client *client = data->client;
	u32 code;
	u8 values[6];

	mutex_lock(&data->lock);

	i2c_smbus_read_i2c_block_data(client, 0x00, 0x06, values);

	mutex_unlock(&data->lock);

	// If bit 0 is set a new RC code is available
	if (values[0] & 0x01) {
		code = (values[1] << 24) | (values[2] << 16) |
			(values[3] << 8) | (values[4]);

//		printk("fab4 ir: new RC code received: 0x%X\n", code);

		input_event(data->input_dev, EV_MSC, MSC_RAW, code);
		input_sync(data->input_dev);
	}

	// Check if we either have a remoteness transition (user moved away) or proximity detected
	// Always send out this event even if it most likly was only an IR remote command
	if ( ((values[0] & 0x02) || (values[0] & 0x04)) /* && (values[0] & 0x01) == 0 */) {
		/*
		RJ: Is a more complex version required? ie. more data in the actual code sent?
		code = ((values[0]&0x10) >> 4) | // proximity(0x10,4) to bit 0
			((values[0]&0x04) >> 2) | // remoteness trans (0x04,3) to bit 1
			((values[0]&0x02) << 1); // proximity trans (0x02,1) to bit 2
		*/

		code = (values[0]&0x10) >> 4;
		
		input_event(data->input_dev, EV_SW, SW_TABLET_MODE, code);
		input_sync(data->input_dev);
	}

#ifndef USE_TIMER
	enable_irq(client->irq);
#endif
}


#ifdef USE_TIMER
static void fab4_ir_timer(unsigned long ptr)
{
	struct fab4_ir_data *data = (struct fab4_ir_data *) ptr;

	schedule_work(&data->irq_work);

	data->timer.expires = jiffies + TIMER_INTERVAL;
	add_timer(&data->timer);
}
#else
static irqreturn_t fab4_ir_irq(int irq, void *dev_id)
{
	struct fab4_ir_data *data = dev_id;

	schedule_work(&data->irq_work);
	disable_irq(irq);

	return IRQ_HANDLED;
}
#endif


int fab4_ir_probe(struct i2c_client *client)
{
	struct fab4_ir_data *data;
	struct input_dev *input_dev;
	struct resource *res = client->dev.platform_data;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "No SMBUS byte data capability\n");
		goto error0;
	}

	if (!(data = kzalloc(sizeof(struct fab4_ir_data), GFP_KERNEL))) {
		dev_err(&client->dev, "No space for state\n");
		err = -ENOMEM;
		goto error0;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
	data->xres = res[0].start;
	data->sdata = res[1].start;
	data->sclk = res[2].start;

	if (mxc_request_gpio(data->xres) != 0 ||
	    mxc_request_gpio(data->sdata) != 0 ||
	    mxc_request_gpio(data->sclk) != 0) {
		printk(KERN_ERR "Failed to cliam XRES, SDATA or SCLK\n");
	}


	mutex_init(&data->lock);

	INIT_WORK(&data->irq_work, fab4_ir_work);

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		dev_err(&client->dev, "Could not allocate input device\n");
		goto error1;
	}

	input_dev->name = "FAB4 IR";
	input_dev->dev.parent = &client->dev;

	input_set_capability(input_dev, EV_MSC, MSC_RAW);

	//RJ: Adding EV_SW, SW_TABLET_MODE (Proximity) to capabilities
	input_set_capability(input_dev, EV_SW, SW_TABLET_MODE);

	data->input_dev = input_dev;

	err = input_register_device(input_dev);
	if (err != 0) {
		dev_err(&client->dev, "Failed to register input device\n");
		input_free_device(input_dev);
		goto error1;
	}

#ifndef USE_TIMER
	err = request_irq(client->irq, fab4_ir_irq,
			  IRQF_TRIGGER_LOW | IRQF_SAMPLE_RANDOM,
			  client->dev.bus_id, data);

	if (err != 0) {
		dev_err(&client->dev, "Failed to register IRQ %d\n",
			client->irq);
		goto error2;
	}
#else
	init_timer(&data->timer);

	data->timer.expires = jiffies + TIMER_INTERVAL;
	data->timer.function = fab4_ir_timer;
	data->timer.data = (unsigned long) data;
	add_timer(&data->timer);
#endif

	/* verify PSoC firmware */
	request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				"ir_controller_21323.hex", &client->dev,
				data, fab4_prog_firmware);

	/* Init proximity */
	data->proximity_control = 0x03;
	data->proximity_duty_cycle = 0x04;

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &fab4_ir_attr_group);
	if (err)
		goto error1;

	return 0;

 error3:
#ifdef USE_TIMER
	del_timer(&data->timer);
#else
	free_irq(client->irq, data);
#endif
 error2:
	input_unregister_device(input_dev);
 error1:
	kfree(data);
 error0:
	return err;
}


int fab4_ir_remove(struct i2c_client *client)
{
	struct fab4_ir_data *data = i2c_get_clientdata(client);
	int err;

	sysfs_remove_group(&client->dev.kobj, &fab4_ir_attr_group);

#ifdef USE_TIMER
	del_timer(&data->timer);
#else
	free_irq(client->irq, data);
#endif

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


static struct i2c_driver fab4_ir_driver = {
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "fab4-ir",
	},
	.probe	= fab4_ir_probe,
	.remove	= fab4_ir_remove,
};


static int __init fab4_ir_init(void)
{
	return i2c_add_driver(&fab4_ir_driver);
}


static void __exit fab4_ir_exit(void)
{
	i2c_del_driver(&fab4_ir_driver);
}


MODULE_AUTHOR ("Richard Titmuss <richard_titmuss@logitech.com>");
MODULE_DESCRIPTION("FAB4 IR driver");

module_init(fab4_ir_init)
module_exit(fab4_ir_exit)
