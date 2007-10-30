/*
 * wm8750-i2c.c -- WM8750 ALSA SoC I2C Driver
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <richard@openedhand.com>
 *
 * Based on WM8753.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "wm8750.h"

/*
 * WM8731 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
 */
static unsigned short normal_i2c[] = { 0, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

static struct i2c_driver wm8750_i2c_driver;
static struct i2c_client client_template;

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */
static struct snd_soc_device *wm8750_socdev;

static int wm8750_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = wm8750_socdev;
	struct wm8750_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;

	if (addr != setup->i2c_address)
		return -ENODEV;

	client_template.adapter = adap;
	client_template.addr = addr;

	i2c = kmemdup(&client_template, sizeof(client_template), GFP_KERNEL);
	if (i2c == NULL) {
		kfree(codec);
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		err("failed to attach codec at addr %x\n", addr);
		goto err;
	}

	ret = wm8750_init(socdev);
	if (ret < 0) {
	err("failed to initialise WM8750\n");
		goto err;
	}
	return ret;

err:
	kfree(codec);
	kfree(i2c);
	return ret;
}

static int wm8750_i2c_detach(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	i2c_detach_client(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}

static int wm8750_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, wm8750_codec_probe);
}

/* corgi i2c codec control layer */
static struct i2c_driver wm8750_i2c_driver = {
	.driver = {
		.name = "WM8750 I2C Codec",
		.owner = THIS_MODULE,
	},
	.id =             I2C_DRIVERID_WM8750,
	.attach_adapter = wm8750_i2c_attach,
	.detach_client =  wm8750_i2c_detach,
	.command =        NULL,
};

static struct i2c_client client_template = {
	.name =   "WM8750",
	.driver = &wm8750_i2c_driver,
};

static int wm8750_probe_i2c(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct wm8750_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec;

	codec = wm8750_probe(pdev);
	if (codec == NULL) {
		dev_err(&pdev->dev, "No memory for CODEC device\n");
		return -ENODEV;
	}

	wm8750_socdev = socdev;

	/* initialise our interface */

	if (setup->i2c_address) {
		normal_i2c[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t)i2c_master_send;
		ret = i2c_add_driver(&wm8750_i2c_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add i2c driver");
	}

	return 0;
}


static int wm8750_remove_i2c(struct platform_device *pdev)
{
	int ret;

	ret = wm8750_remove(pdev);
	if (ret)
		return ret;

	i2c_del_driver(&wm8750_i2c_driver);
	wm8750_free(pdev);

	return 0;
}


struct snd_soc_codec_device soc_codec_dev_wm8750_i2c = {
	.probe = 	wm8750_probe_i2c,
	.remove = 	wm8750_remove_i2c,
	.suspend = 	wm8750_suspend,
	.resume =	wm8750_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_wm8750_i2c);

MODULE_DESCRIPTION("ASoC WM8750 I2C access driver");
MODULE_AUTHOR("Liam Girdwood");
MODULE_LICENSE("GPL");
