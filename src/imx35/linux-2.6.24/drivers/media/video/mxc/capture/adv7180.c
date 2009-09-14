/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file adv7180.c
 *
 * @brief Analog Device ADV7180 video decoder functions
 *
 * @ingroup Camera
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/wait.h>
#include <linux/videodev2.h>
#include <linux/workqueue.h>
#include "asm-arm/arch-mxc/pmic_external.h"
#include "mxc_v4l2_capture.h"

extern void gpio_sensor_active(void);
extern PMIC_STATUS pmic_gpio_set_bit_val(t_mcu_gpio_reg reg, unsigned int bit,
					 unsigned int val);
struct i2c_client *adv7180_i2c_client;

static int adv7180_probe(struct i2c_client *adapter);
static int adv7180_detach(struct i2c_client *client);

static struct i2c_driver adv7180_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "adv7180",
		   },
	.probe = adv7180_probe,
	.remove = adv7180_detach,
};

/*! Structure initialized by adv7180_interface() and used to configure the
 *  CSI.
 */
static sensor_interface *interface_param;

/*! List of input video formats supported. The video formats is corresponding
 * with v4l2 id in video_fmt_t
 */
typedef enum {
	ADV7180_NTSC = 0,	/*!< Locked on (M) NTSC video signal. */
	ADV7180_PAL,		/*!< (B, G, H, I, N)PAL video signal. */
	ADV7180_NOT_LOCKED,	/*!< Not locked on a signal. */
} video_fmt_idx;

/*! Number of video standards supported (including 'not locked' signal). */
#define ADV7180_STD_MAX		(ADV7180_PAL + 1)

/*! Video format structure. */
typedef struct {
	int v4l2_id;		/*!< Video for linux ID. */
	char name[16];		/*!< Name (e.g., "NTSC", "PAL", etc.) */
	u16 raw_width;		/*!< Raw width. */
	u16 raw_height;		/*!< Raw height. */
	u16 active_width;	/*!< Active width. */
	u16 active_height;	/*!< Active height. */
} video_fmt_t;

/*! Description of video formats supported.
 *
 *  PAL: raw=720x625, active=720x576.
 *  NTSC: raw=720x525, active=720x480.
 */
static video_fmt_t video_fmts[] = {
	{			/*! NTSC */
	 .v4l2_id = V4L2_STD_NTSC,
	 .name = "NTSC",
	 .raw_width = 720,	/* SENS_FRM_WIDTH */
	 .raw_height = 288,	/* SENS_FRM_HEIGHT */
	 .active_width = 720,	/* ACT_FRM_WIDTH */
	 .active_height = (480 / 2),	/* ACT_FRM_WIDTH */
	 },
	{			/*! (B, G, H, I, N) PAL */
	 .v4l2_id = V4L2_STD_PAL,
	 .name = "PAL",
	 .raw_width = 720,
	 .raw_height = (576 / 2) + 24 * 2,
	 .active_width = 720,
	 .active_height = (576 / 2),
	 },
	{			/*! Unlocked standard */
	 .v4l2_id = V4L2_STD_ALL,
	 .name = "Autodetect",
	 .raw_width = 720,
	 .raw_height = (576 / 2) + 24 * 2,
	 .active_width = 720,
	 .active_height = (576 / 2),
	 },

};

/*!* Standard index of ADV7180. */
static video_fmt_idx video_idx = ADV7180_PAL;

/*! @brief This mutex is used to provide mutual exclusion.
 *
 *  Create a mutex that can be used to provide mutually exclusive
 *  read/write access to the globally accessible data structures
 *  and variables that were defined above.
 */
static DECLARE_MUTEX(mutex);

#define IF_NAME                 "adv7180"
#define ADV7180_INPUT_CTL              0x00     /* Input Control */
#define ADV7180_STATUS_1               0x10     /* Status #1 */
#define ADV7180_BRIGHTNESS             0x0a     /* Brightness */
#define ADV7180_IDENT                  0x11     /* IDENT */
#define ADV7180_VSYNC_FIELD_CTL_1      0x31     /* VSYNC Field Control #1 */
#define ADV7180_MANUAL_WIN_CTL         0x3d     /* Manual Window Control */
#define ADV7180_SD_SATURATION_CB       0xe3     /* SD Saturation Cb */
#define ADV7180_SD_SATURATION_CR       0xe4     /* SD Saturation Cr */

/***********************************************************************
 * I2C transfert.
 ***********************************************************************/

/*! Read one register from a ADV7180 i2c slave device.
 *
 *  @param *reg		register in the device we wish to access.
 *
 *  @return		       0 if success, an error code otherwise.
 */
static inline int adv7180_read(u8 reg)
{
	int val;
	val = i2c_smbus_read_byte_data(adv7180_i2c_client, reg);
	if (val < 0) {
		dev_dbg(&adv7180_i2c_client->dev,
			"%s:read reg error: reg=%2x \n", __func__, reg);
		return -1;
	}
	return val;
}

/*! Write one register of a ADV7180 i2c slave device.
 *
 *  @param *reg		register in the device we wish to access.
 *
 *  @return		       0 if success, an error code otherwise.
 */
static int adv7180_write_reg(u8 reg, u8 val)
{
	if (i2c_smbus_write_byte_data(adv7180_i2c_client, reg, val) < 0) {
		dev_dbg(&adv7180_i2c_client->dev,
			"%s:write reg error:reg=%2x,val=%2x\n", __func__,
			reg, val);
		return -1;
	}
	return 0;
}

/***********************************************************************
 * mxc_v4l2_capture interface.
 ***********************************************************************/

/*! Set video standard.
 *
 *  @param std		Video standard.
 *
 *  @return		       None.
 */
static void adv7180_set_std(v4l2_std_id std)
{

	dev_dbg(&adv7180_i2c_client->dev, "adv7180_set_std call \n");
	down(&mutex);
	if (std == V4L2_STD_PAL) {
		video_idx = ADV7180_PAL;
		ipu_csi_set_window_pos(0, 0);
	} else if (std == V4L2_STD_NTSC) {
		video_idx = ADV7180_NTSC;
		/* Get rid of the white dot line in NTSC signal input */
		ipu_csi_set_window_pos(0, 12);
	} else {
		video_idx = ADV7180_NOT_LOCKED;
		ipu_csi_set_window_pos(0, 0);
		dev_dbg(&adv7180_i2c_client->dev,
			"adv7180 set non-recognized std!\n");
	}
	up(&mutex);
}

/*! ADV7180 video decoder interface initialization.
 *
 *  @param *param	sensor_interface *.
 *  @param width	u32.
 *  @param height	u32.
 *
 *  @return		None.
 */
static void adv7180_interface(sensor_interface *param, u32 width, u32 height)
{
	param->clk_mode = IPU_CSI_CLK_MODE_CCIR656_PROGRESSIVE;
	param->ext_vsync = 0x00;
	param->Vsync_pol = 0x00;
	param->Hsync_pol = 0x01;	/*! Signal is inverted. */
	param->pixclk_pol = 0x00;
	param->data_pol = 0x00;
	param->data_width = IPU_CSI_DATA_WIDTH_8;
	param->width = width - 1;
	param->height = height - 1;
	param->active_width = video_fmts[video_idx].active_width;
	param->active_height = video_fmts[video_idx].active_height;
	param->pixel_fmt = IPU_PIX_FMT_UYVY;	/*! YUV422. */

	/*! Not used, ADV7180 has a dedicated clock */
	param->mclk = 27000000;
}

/*! ADV7180 video decoder set color configuration.
 *
 *  @param bright	Brightness.
 *  @param saturation	Saturation.
 *  @param red		Red.
 *  @param green	Green.
 *  @param blue		Blue.
 *
 *  @return		None.
 */
static void
adv7180_set_color(int bright, int saturation, int red, int green, int blue)
{
	u8 b = (u8) bright, s = (u8) saturation;

	adv7180_write_reg(ADV7180_BRIGHTNESS, b);
	adv7180_write_reg(ADV7180_SD_SATURATION_CB, s);
	adv7180_write_reg(ADV7180_SD_SATURATION_CR, s);
}

/*! ADV7180 video decoder get color configuration.
 *
 *  @param *bright	Brightness.
 *  @param *saturation	Saturation.
 *  @param *red		Red.
 *  @param *green	Green.
 *  @param *blue	Blue.
 *
 *  @return		None.
 */
static void
adv7180_get_color(int *bright, int *saturation, int *red, int *green, int *blue)
{
	int b, s;

	b = adv7180_read(ADV7180_BRIGHTNESS);
	s = adv7180_read(ADV7180_SD_SATURATION_CB);
	*bright = b;
	*saturation = s;
}

/*! ADV7180 video decoder configuration.
 *
 *  @param *frame_rate	Frame rate.
 *
 *  @return		sensor_interface pointer.
 */
sensor_interface *adv7180_config(int *frame_rate, int high_quality)
{
	down(&mutex);
	adv7180_interface(interface_param, video_fmts[video_idx].raw_width,
			  video_fmts[video_idx].raw_height);
	up(&mutex);

	return interface_param;
}

/*! ADV7180 Reset function.
 *
 *  @return		None.
 */
static sensor_interface *adv7180_soft_reset(void)
{
	int frame_rate;
	sensor_interface *s;

	s = adv7180_config(&frame_rate, 0);
	return s;
}

/*! Return attributes of current video standard.
 *
 *  @return		None.
 */
static void adv7180_get_std(v4l2_std_id * std)
{
	int tmp;

	/* Read the AD_RESULT to get the detect output video standard */
	tmp = adv7180_read(ADV7180_STATUS_1) & 0x70;
	down(&mutex);
	if (tmp == 0x40)	/* PAL */
		*std = V4L2_STD_PAL;
	else if (tmp == 0)
		/*NTSC*/ *std = V4L2_STD_NTSC;
	else {
		*std = V4L2_STD_ALL;
		dev_dbg(&adv7180_i2c_client->dev,
			"Get invalid video standard! \n");
	}
	up(&mutex);
}

/*! Interface with 'mxc_v4l2_capture.c' */
struct camera_sensor camera_sensor_if = {
	.set_color = adv7180_set_color,
	.get_color = adv7180_get_color,
	.config = adv7180_config,
	.reset = adv7180_soft_reset,
	.set_std = adv7180_set_std,
	.get_std = adv7180_get_std,
};
EXPORT_SYMBOL(camera_sensor_if);

/***********************************************************************
 * I2C client and driver.
 ***********************************************************************/

/*! ADV7180 Reset function.
 *
 *  @return		None.
 */
static void adv7180_hard_reset(void)
{
	/*! Driver works fine without explicit register
	 * initialization. Furthermore, initializations takes about 2 seconds
	 * at startup...
	 */

	/*! Set YPbPr input on AIN1,4,5 and normal
	 * operations(autodection of all stds).
	 */
	adv7180_write_reg(ADV7180_INPUT_CTL, 0x09);
	/*! Datasheet recommend */
	adv7180_write_reg(ADV7180_VSYNC_FIELD_CTL_1, 0x02);
	adv7180_write_reg(ADV7180_MANUAL_WIN_CTL, 0xa2);
}

/*! ADV7180 I2C attach function.
 *
 *  @param *adapter	struct i2c_adapter *.
 *
 *  @return		Error code indicating success or failure.
 */

/*! ADV7180 I2C probe function.
 *
 *  @param *adapter	I2C adapter descriptor.
 *
 *  @return		Error code indicating success or failure.
 */
static int adv7180_probe(struct i2c_client *client)
{
	int rev_id;

	/*! Put device into normal operational mode. */
	pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 1, 1);
	pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 4, 1);

	adv7180_i2c_client = client;
	dev_dbg(&adv7180_i2c_client->dev,
		"%s:adv7180 probe i2c address is 0x%02X \n",
		__func__, adv7180_i2c_client->addr);
	/*! Read the revision ID of the tvin chip */
	rev_id = adv7180_read(ADV7180_IDENT);
	dev_dbg(&adv7180_i2c_client->dev,
		"%s:Analog Device adv7%2X0 detected! \n", __func__,
		rev_id);
	interface_param = (sensor_interface *)
	    kmalloc(sizeof(sensor_interface), GFP_KERNEL);
	if (!interface_param) {
		dev_dbg(&adv7180_i2c_client->dev, " kmalloc failed \n");
		return -1;
	}
	/*! ADV7180 initialization. */
	adv7180_hard_reset();

	return 0;
}

/*! ADV7180 I2C detach function.
 *
 *  @param *client	struct i2c_client *.
 *
 *  @return		Error code indicating success or failure.
 */
static int adv7180_detach(struct i2c_client *client)
{
	dev_dbg(&adv7180_i2c_client->dev,
		"%s:Removing %s video decoder @ 0x%02X from adapter %s \n",
		__func__, IF_NAME, client->addr << 1, client->adapter->name);
	kfree(interface_param);
	interface_param = NULL;

	/*! Put device into power down mode. */
	pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_2, 4, 0);
	/*! Disable TVIN module. */
	pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 1, 0);

	return 0;
}

/*! ADV7180 init function.
 *
 *  @return		Error code indicating success or failure.
 */
static __init int adv7180_init(void)
{
	u8 err = 0;

	/*! Configuration of i.MX35 CSI I/O muxes. */
	gpio_sensor_active();
	err = i2c_add_driver(&adv7180_i2c_driver);
	if (err != 0)
		dev_dbg(&adv7180_i2c_client->dev,
			"%s:driver registration failed, error=%d \n",
			__func__, err);

	return err;
}

extern void gpio_sensor_inactive(void);
/*! ADV7180 cleanup function.
 *
 *  @return		Error code indicating success or failure.
 */
static void __exit adv7180_clean(void)
{
	i2c_del_driver(&adv7180_i2c_driver);
	gpio_sensor_inactive();
}

module_init(adv7180_init);
module_exit(adv7180_clean);

MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Anolog Device ADV7180 video decoder driver");
MODULE_LICENSE("GPL");
