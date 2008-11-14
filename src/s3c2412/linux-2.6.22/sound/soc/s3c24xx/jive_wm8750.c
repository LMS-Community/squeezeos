/* sound/soc/s3c24xx/jive_wm8705.c
 *
 * Copyright 2007 Simtec Electronics
 *
 * Based on sound/soc/pxa/spitz.c
 *	Copyright 2005 Wolfson Microelectronics PLC.
 *	Copyright 2005 Openedhand Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-irq.h>

#include <asm/hardware.h>

#include "s3c24xx-pcm.h"
#include "s3c2412-i2s.h"

#include "../codecs/wm8750.h"

/* define the scenarios */
#define JIVE_AUDIO_OFF                  0
#define JIVE_SPEAKER                    1
#define JIVE_HEADPHONE                  2
#define JIVE_SPEAKER_HEADPHONE          3

static const char *audio_map[][2] = {
	[0]	= { "Headphone Jack", "LOUT1" },
	[1]	= { "Headphone Jack", "ROUT1" },
	[2]	= { "Internal Speaker", "LOUT2" },
	[3]	= { "Internal Speaker", "ROUT2" },
	[4]	= { "LINPUT1", "Line Input" },
	[5]	= { "RINPUT1", "Line Input" },
	[6]	= { NULL, NULL },
};

static const struct snd_soc_dapm_widget wm8750_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Internal Speaker", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const char *jive_scenarios[] = { "Off", "Speaker", "Headphone", "Speaker+Headphone" };

static const struct soc_enum jive_scenarios_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(jive_scenarios),jive_scenarios),
};

static int jive_scenario = 1; /* default speaker on */

static int jive_get_scenario(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        ucontrol->value.integer.value[0] = jive_scenario;
        return 0;
}

static void set_scenario_endpoints(struct snd_soc_codec *codec)
{
        switch(jive_scenario) {
        case JIVE_SPEAKER:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 0);
		snd_soc_dapm_set_endpoint(codec, "Internal Speaker", 1);
		break;
        case JIVE_HEADPHONE:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 1);
		snd_soc_dapm_set_endpoint(codec, "Internal Speaker", 0);
		break;
        case JIVE_SPEAKER_HEADPHONE:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 1);
		snd_soc_dapm_set_endpoint(codec, "Internal Speaker", 1);
		break;
	default:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 0);
		snd_soc_dapm_set_endpoint(codec, "Internal Speaker", 0);
		break;
	}

	snd_soc_dapm_sync_endpoints(codec);
}

static int jive_set_scenario(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

        if (jive_scenario == ucontrol->value.integer.value[0])
                return 0;

        jive_scenario = ucontrol->value.integer.value[0];
        set_scenario_endpoints(codec);
        return 1;
}


static int jive_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->socdev->codec;
	int val;

	s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_INP);
	val = (s3c2410_gpio_getpin(S3C2410_GPG14) > 0);
	s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_EINT22);

	snd_soc_dapm_set_endpoint(codec, "Line In", 1);
        set_scenario_endpoints(codec);

	return 0;
}

static inline int codec_set_dai_fmt(struct snd_soc_codec_dai *dai,
				    unsigned int format)
{
	return dai->dai_ops.set_fmt(dai, format);
}

static inline int codec_set_dai_sysclk(struct snd_soc_codec_dai *dai,
				       unsigned int clk,
				       unsigned int arg,
				       unsigned int arg2)
{
	return dai->dai_ops.set_sysclk(dai, clk, arg, arg2);
}

static inline int cpu_set_dai_fmt(struct snd_soc_cpu_dai *dai,
				  unsigned int format)
{
	return dai->dai_ops.set_fmt(dai, format);
}

static inline int cpu_set_dai_sysclk(struct snd_soc_cpu_dai *dai,
				     unsigned int clk,
				     unsigned int arg,
				     unsigned int arg2)
{
	return dai->dai_ops.set_sysclk(dai, clk, arg, arg2);
}

static inline int cpu_set_dai_clkdiv(struct snd_soc_cpu_dai *dai,
				     unsigned int clk,
				     unsigned int arg)
{
	return dai->dai_ops.set_clkdiv(dai, clk, arg);
}



static int jive_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct s3c2412_rate_calc div;
	unsigned int clk = 0;
	int ret = 0;

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 48000:
	case 96000:
		clk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		clk = 11289600;
		break;
	}

	s3c2412_iis_calc_rate(&div, NULL, params_rate(params),
			      s3c2412_get_iisclk());

	/* set codec DAI configuration */
	ret = codec_set_dai_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = cpu_set_dai_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
			      SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */
	ret = codec_set_dai_sysclk(codec_dai, WM8750_SYSCLK, clk,
				   SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = cpu_set_dai_clkdiv(cpu_dai, S3C2412_DIV_RCLK, div.fs_div);
	if (ret < 0)
		return ret;

	ret = cpu_set_dai_clkdiv(cpu_dai, S3C2412_DIV_PRESCALER, div.clk_div - 1);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops jive_ops = {
	.startup	= jive_startup,
	.hw_params	= jive_hw_params,
};

static const struct snd_kcontrol_new wm8750_jive_controls[] = {
        SOC_ENUM_EXT("Endpoint", jive_scenarios_enum[0],
                jive_get_scenario, jive_set_scenario),
};

static int jive_wm8750_init(struct snd_soc_codec *codec)
{
	int i, err;

	/* These endpoints are not being used. */
	snd_soc_dapm_set_endpoint(codec, "LINPUT2", 0);
	snd_soc_dapm_set_endpoint(codec, "RINPUT2", 0);
	snd_soc_dapm_set_endpoint(codec, "LINPUT3", 0);
	snd_soc_dapm_set_endpoint(codec, "RINPUT3", 0);
	snd_soc_dapm_set_endpoint(codec, "OUT3", 0);
	snd_soc_dapm_set_endpoint(codec, "MONO", 0);

	/* Add jive specific controls */
	for (i = 0; i < ARRAY_SIZE(wm8750_jive_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&wm8750_jive_controls[i], codec, NULL));
		if (err < 0)
			return err;
	}

	/* Add jive specific widgets */
	for (i = 0; i < ARRAY_SIZE(wm8750_dapm_widgets); i++) {
		snd_soc_dapm_new_control(codec, &wm8750_dapm_widgets[i]);
	}

	/* Set up jive specific audio path audio_map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		printk(KERN_INFO "mapping %s => %s\n", audio_map[i][0], audio_map[i][1]);
		snd_soc_dapm_connect_input(codec, audio_map[i][0], NULL,
					   audio_map[i][1]);
	}

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

static struct snd_soc_dai_link jive_dai = {
	.name		= "wm8750",
	.stream_name	= "WM8750",
	.cpu_dai	= &s3c2412_i2s_dai,
	.codec_dai	= &wm8750_dai,
	.init		= jive_wm8750_init,
	.ops		= &jive_ops,
};

/* jive audio machine driver */
static struct snd_soc_machine snd_soc_machine_jive = {
	.name		= "Jive",
	.dai_link	= &jive_dai,
	.num_links	= 1,
};

struct wm8750_setup_reg jive_wm8750_initregs[] = {
	{ .reg =  0, .val = 0x0097, },
	{ .reg =  1, .val = 0x0097, },
	{ .reg =  2, .val = 0x0079, },
	{ .reg =  3, .val = 0x0079, },
	{ .reg =  5, .val = 0x0000, },
	{ .reg =  7, .val = 0x0002, },
	{ .reg =  8, .val = 0x0022, },
	{ .reg = 10, .val = 0x00ff, },
	{ .reg = 11, .val = 0x00ff, },
	{ .reg = 23, .val = 0x00c3, },
	{ .reg = 24, .val = 0x0010, },
	{ .reg = 25, .val = 0x0000, },
	{ .reg = 34, .val = 0x0100, },
	{ .reg = 37, .val = 0x0100, },
	{ .reg = 40, .val = 0x0079, },
	{ .reg = 41, .val = 0x0079, },
};

/* jive audio private data */
static struct wm8750_setup_data jive_wm8750_setup = {
	.regs		= jive_wm8750_initregs,
	.regs_size	= ARRAY_SIZE(jive_wm8750_initregs),
};

/* jive audio subsystem */
static struct snd_soc_device jive_snd_devdata = {
	.machine	= &snd_soc_machine_jive,
	.platform	= &s3c24xx_soc_platform,
	.codec_dev	= &soc_codec_dev_wm8750_spi,
	.codec_data	= &jive_wm8750_setup,
};

static struct platform_device *jive_snd_device;

static int __init jive_init(void)
{
	int ret;

	printk("JIVE WM8750 Audio support\n");

	if (!machine_is_jive())
		return 0;

	jive_snd_device = platform_device_alloc("soc-audio", -1);
	if (!jive_snd_device)
		return -ENOMEM;

	platform_set_drvdata(jive_snd_device, &jive_snd_devdata);
	jive_snd_devdata.dev = &jive_snd_device->dev;
	ret = platform_device_add(jive_snd_device);

	if (ret)
		platform_device_put(jive_snd_device);

	return ret;
}

static void __exit jive_exit(void)
{
	platform_device_unregister(jive_snd_device);
}

module_init(jive_init);
module_exit(jive_exit);

MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_DESCRIPTION("ALSA SoC Jive");
MODULE_LICENSE("GPL");
