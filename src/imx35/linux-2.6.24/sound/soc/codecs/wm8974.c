/*
 * wm8974.c -- WM8974 ALSA SoC audio driver
 *
 * Copyright 2008 Logitech
 *	Richard Titmuss <richard_titmuss@logitech.com>
 *
 * Based on wm8350.c and main line wm8974.c:
 *
 * Copyright (C) 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lg@opensource.wolfsonmicro.com>
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
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "wm8974.h"

#define AUDIO_NAME "WM8974"
#define WM8974_VERSION "0.1"


/*
 * wm8974 register cache
 * We can't read the WM8974 register space  so we cache
 * them instead.
 */
static const u16 wm8974_reg[WM8974_CACHEREGNUM] = {
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0050, 0x0000, 0x0140, 0x0000,
	0x0000, 0x0000, 0x0000, 0x00ff,
	0x0000, 0x0000, 0x0100, 0x00ff,
	0x0000, 0x0000, 0x012c, 0x002c,
	0x002c, 0x002c, 0x002c, 0x0000,
	0x0032, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0038, 0x000b, 0x0032, 0x0000,
	0x0008, 0x000c, 0x0093, 0x00e9,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0003, 0x0010, 0x0000, 0x0000,
	0x0000, 0x0002, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0039, 0x0000,
	0x0000,
};

static int wm8974_mute(struct snd_soc_dai *dai, int mute);

/*
 * read wm8974 register cache
 */
static inline unsigned int wm8974_read_reg_cache(struct snd_soc_codec * codec,
						 unsigned int reg)
{
	u16 *cache = codec->reg_cache;

	if (reg == WM8974_RESET)
		return 0;
	if (reg >= WM8974_CACHEREGNUM)
		return -1;
	return cache[reg];
}

/*
 * write wm8974 register cache
 */
static inline void wm8974_write_reg_cache(struct snd_soc_codec *codec,
					  u16 reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;

	if (reg >= WM8974_CACHEREGNUM)
		return;
	cache[reg] = value;
}

/*
 * write to the WM8974 register space
 */
static int wm8974_write_reg(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	u8 data[2];
	
        /* data is
	 *   D15..D9 WM8974 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	data[1] = value & 0x00ff;

	wm8974_write_reg_cache (codec, reg, value);
        if (codec->mach_write(codec->control_data, (long)data, 2) == 2)
                return 0;
	else
                return -EIO;
}


static const char *wm8974_companding[] = {"Off", "NC", "u-law", "A-law" };
static const char *wm8974_deemp[] = {"None", "32kHz", "44.1kHz", "48kHz" };
static const char *wm8974_eqmode[] = {"Capture", "Playback" };
static const char *wm8974_bw[] = {"Narrow", "Wide" };
static const char *wm8974_eq1[] = {"80Hz", "105Hz", "135Hz", "175Hz" };
static const char *wm8974_eq2[] = {"230Hz", "300Hz", "385Hz", "500Hz" };
static const char *wm8974_eq3[] = {"650Hz", "850Hz", "1.1kHz", "1.4kHz" };
static const char *wm8974_eq4[] = {"1.8kHz", "2.4kHz", "3.2kHz", "4.1kHz" };
static const char *wm8974_eq5[] = {"5.3kHz", "6.9kHz", "9kHz", "11.7kHz" };
static const char *wm8974_alc[] = {"ALC", "Limiter" };
  
static const struct soc_enum wm8974_enum[] = {
	SOC_ENUM_SINGLE(WM8974_COMP, 1, 4, wm8974_companding), /* adc */
	SOC_ENUM_SINGLE(WM8974_COMP, 3, 4, wm8974_companding), /* dac */
	SOC_ENUM_SINGLE(WM8974_DAC,  4, 4, wm8974_deemp),
	SOC_ENUM_SINGLE(WM8974_EQ1,  8, 2, wm8974_eqmode),
  
	SOC_ENUM_SINGLE(WM8974_EQ1,  5, 4, wm8974_eq1),
	SOC_ENUM_SINGLE(WM8974_EQ2,  8, 2, wm8974_bw),
	SOC_ENUM_SINGLE(WM8974_EQ2,  5, 4, wm8974_eq2),
	SOC_ENUM_SINGLE(WM8974_EQ3,  8, 2, wm8974_bw),
 
	SOC_ENUM_SINGLE(WM8974_EQ3,  5, 4, wm8974_eq3),
	SOC_ENUM_SINGLE(WM8974_EQ4,  8, 2, wm8974_bw),
	SOC_ENUM_SINGLE(WM8974_EQ4,  5, 4, wm8974_eq4),
	SOC_ENUM_SINGLE(WM8974_EQ5,  8, 2, wm8974_bw),
 
	SOC_ENUM_SINGLE(WM8974_EQ5,  5, 4, wm8974_eq5),
	SOC_ENUM_SINGLE(WM8974_ALC3,  8, 2, wm8974_alc),
};
 
static const struct snd_kcontrol_new wm8974_snd_controls[] = {
	SOC_SINGLE("Digital Loopback Switch", WM8974_COMP, 0, 1, 0),
 
	SOC_ENUM("DAC Companding", wm8974_enum[1]),
	SOC_ENUM("ADC Companding", wm8974_enum[0]),
 
	SOC_ENUM("Playback De-emphasis", wm8974_enum[2]),
	SOC_SINGLE("DAC Inversion Switch", WM8974_DAC, 0, 1, 0),
 
	SOC_SINGLE("PCM Volume", WM8974_DACVOL, 0, 127, 0),
 
	SOC_SINGLE("High Pass Filter Switch", WM8974_ADC, 8, 1, 0),
	SOC_SINGLE("High Pass Cut Off", WM8974_ADC, 4, 7, 0),
	SOC_SINGLE("ADC Inversion Switch", WM8974_COMP, 0, 1, 0),
 
	SOC_SINGLE("Capture Volume", WM8974_ADCVOL,  0, 127, 0),
 
	SOC_ENUM("Equaliser Function", wm8974_enum[3]),
	SOC_ENUM("EQ1 Cut Off", wm8974_enum[4]),
	SOC_SINGLE("EQ1 Volume", WM8974_EQ1,  0, 31, 1),
 
	SOC_ENUM("Equaliser EQ2 Bandwith", wm8974_enum[5]),
	SOC_ENUM("EQ2 Cut Off", wm8974_enum[6]),
	SOC_SINGLE("EQ2 Volume", WM8974_EQ2,  0, 31, 1),
 
	SOC_ENUM("Equaliser EQ3 Bandwith", wm8974_enum[7]),
	SOC_ENUM("EQ3 Cut Off", wm8974_enum[8]),
	SOC_SINGLE("EQ3 Volume", WM8974_EQ3,  0, 31, 1),
 
	SOC_ENUM("Equaliser EQ4 Bandwith", wm8974_enum[9]),
	SOC_ENUM("EQ4 Cut Off", wm8974_enum[10]),
	SOC_SINGLE("EQ4 Volume", WM8974_EQ4,  0, 31, 1),
 
	SOC_ENUM("Equaliser EQ5 Bandwith", wm8974_enum[11]),
	SOC_ENUM("EQ5 Cut Off", wm8974_enum[12]),
	SOC_SINGLE("EQ5 Volume", WM8974_EQ5,  0, 31, 1),
 
	SOC_SINGLE("DAC Playback Limiter Switch", WM8974_DACLIM1,  8, 1, 0),
	SOC_SINGLE("DAC Playback Limiter Decay", WM8974_DACLIM1,  4, 15, 0),
	SOC_SINGLE("DAC Playback Limiter Attack", WM8974_DACLIM1,  0, 15, 0),
 
	SOC_SINGLE("DAC Playback Limiter Threshold", WM8974_DACLIM2,  4, 7, 0),
	SOC_SINGLE("DAC Playback Limiter Boost", WM8974_DACLIM2,  0, 15, 0),
 
	SOC_SINGLE("ALC Enable Switch", WM8974_ALC1,  8, 1, 0),
	SOC_SINGLE("ALC Capture Max Gain", WM8974_ALC1,  3, 7, 0),
	SOC_SINGLE("ALC Capture Min Gain", WM8974_ALC1,  0, 7, 0),
 
	SOC_SINGLE("ALC Capture ZC Switch", WM8974_ALC2,  8, 1, 0),
	SOC_SINGLE("ALC Capture Hold", WM8974_ALC2,  4, 7, 0),
	SOC_SINGLE("ALC Capture Target", WM8974_ALC2,  0, 15, 0),
 
	SOC_ENUM("ALC Capture Mode", wm8974_enum[13]),
	SOC_SINGLE("ALC Capture Decay", WM8974_ALC3,  4, 15, 0),
	SOC_SINGLE("ALC Capture Attack", WM8974_ALC3,  0, 15, 0),
 
	SOC_SINGLE("ALC Capture Noise Gate Switch", WM8974_NGATE,  3, 1, 0),
	SOC_SINGLE("ALC Capture Noise Gate Threshold", WM8974_NGATE,  0, 7, 0),
	
	SOC_SINGLE("Capture PGA ZC Switch", WM8974_INPPGA,  7, 1, 0),
	SOC_SINGLE("Capture PGA Volume", WM8974_INPPGA,  0, 63, 0),
 
	SOC_SINGLE("Speaker Playback ZC Switch", WM8974_SPKVOL,  7, 1, 0),
	SOC_SINGLE("Speaker Playback Switch", WM8974_SPKVOL,  6, 1, 1),
	SOC_SINGLE("Speaker Playback Volume", WM8974_SPKVOL,  0, 63, 0),
 
	SOC_SINGLE("Capture Boost(+20dB)", WM8974_ADCBOOST,  8, 1, 0),
	SOC_SINGLE("Mono Playback Switch", WM8974_MONOMIX, 6, 1, 0),
};

/* add non dapm controls */
static int wm8974_add_controls(struct snd_soc_codec *codec, 
	struct snd_card *card)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(wm8974_snd_controls); i++) {
		err = snd_ctl_add(card,
				snd_soc_cnew(&wm8974_snd_controls[i], 
					codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

/*
 * DAPM Controls
 */
 
/* Speaker Output Mixer */
static const struct snd_kcontrol_new wm8974_speaker_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line Bypass Switch", WM8974_SPKMIX, 1, 1, 0),
	SOC_DAPM_SINGLE("Aux Playback Switch", WM8974_SPKMIX, 5, 1, 0),
	SOC_DAPM_SINGLE("PCM Playback Switch", WM8974_SPKMIX, 0, 1, 0),
};

/* Mono Output Mixer */
static const struct snd_kcontrol_new wm8974_mono_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line Bypass Switch", WM8974_MONOMIX, 1, 1, 0),
	SOC_DAPM_SINGLE("Aux Playback Switch", WM8974_MONOMIX, 2, 1, 0),
	SOC_DAPM_SINGLE("PCM Playback Switch", WM8974_MONOMIX, 0, 1, 0),
};

/* AUX Input boost vol */
static const struct snd_kcontrol_new wm8974_aux_boost_controls =
	SOC_DAPM_SINGLE("Aux Volume", WM8974_ADCBOOST, 0, 7, 0);

/* Mic Input boost vol */
static const struct snd_kcontrol_new wm8974_mic_boost_controls =
	SOC_DAPM_SINGLE("Mic Volume", WM8974_ADCBOOST, 4, 7, 0);

/* Capture boost switch */
static const struct snd_kcontrol_new wm8974_capture_boost_controls =
	SOC_DAPM_SINGLE("Capture Boost Switch", WM8974_INPPGA,  6, 1, 0);

/* Aux In to PGA */
static const struct snd_kcontrol_new wm8974_aux_capture_boost_controls =
	SOC_DAPM_SINGLE("Aux Capture Boost Switch", WM8974_INPPGA,  2, 1, 0);

/* Mic P In to PGA */
static const struct snd_kcontrol_new wm8974_micp_capture_boost_controls =
	SOC_DAPM_SINGLE("Mic P Capture Boost Switch", WM8974_INPPGA,  0, 1, 0);

/* Mic N In to PGA */
static const struct snd_kcontrol_new wm8974_micn_capture_boost_controls =
	SOC_DAPM_SINGLE("Mic N Capture Boost Switch", WM8974_INPPGA,  1, 1, 0);

static const struct snd_soc_dapm_widget wm8974_dapm_widgets[] = {
	SND_SOC_DAPM_MIXER("Speaker Mixer", WM8974_POWER3, 3, 0,
			   &wm8974_speaker_mixer_controls[0],
			   ARRAY_SIZE(wm8974_speaker_mixer_controls)),
	SND_SOC_DAPM_MIXER("Mono Mixer", WM8974_POWER3, 3, 0,
			   &wm8974_mono_mixer_controls[0],
			   ARRAY_SIZE(wm8974_mono_mixer_controls)),
	SND_SOC_DAPM_DAC("DAC", "Playback", WM8974_POWER3, 0, 0),
	SND_SOC_DAPM_ADC("ADC", "Capture", WM8974_POWER2, 0, 0),
	SND_SOC_DAPM_PGA("Aux Input", WM8974_POWER1, 6, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SpkN Out", WM8974_POWER3, 5, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SpkP Out", WM8974_POWER3, 6, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Mono Out", WM8974_POWER3, 7, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Mic PGA", WM8974_POWER2, 2, 0, NULL, 0),
 
	SND_SOC_DAPM_PGA("Aux Boost", SND_SOC_NOPM, 0, 0,
			 &wm8974_aux_boost_controls, 1),
	SND_SOC_DAPM_PGA("Mic Boost", SND_SOC_NOPM, 0, 0,
			 &wm8974_mic_boost_controls, 1),
	SND_SOC_DAPM_SWITCH("Capture Boost", SND_SOC_NOPM, 0, 0,
			    &wm8974_capture_boost_controls),
 
	SND_SOC_DAPM_MIXER("Boost Mixer", WM8974_POWER2, 4, 0, NULL, 0),
	
	SND_SOC_DAPM_MICBIAS("Mic Bias", WM8974_POWER1, 4, 0),
	
	SND_SOC_DAPM_INPUT("MICN"),
	SND_SOC_DAPM_INPUT("MICP"),
	SND_SOC_DAPM_INPUT("AUX"),
	SND_SOC_DAPM_OUTPUT("MONOOUT"),
	SND_SOC_DAPM_OUTPUT("SPKOUTP"),
	SND_SOC_DAPM_OUTPUT("SPKOUTN"),
};

static const char *audio_map[][3] = {
	/* Mono output mixer */
	{"Mono Mixer", "PCM Playback Switch", "DAC"},
	{"Mono Mixer", "Aux Playback Switch", "Aux Input"},
	{"Mono Mixer", "Line Bypass Switch", "Boost Mixer"},
 
	/* Speaker output mixer */
	{"Speaker Mixer", "PCM Playback Switch", "DAC"},
	{"Speaker Mixer", "Aux Playback Switch", "Aux Input"},
	{"Speaker Mixer", "Line Bypass Switch", "Boost Mixer"},

	/* Outputs */
	{"Mono Out", NULL, "Mono Mixer"},
	{"MONOOUT", NULL, "Mono Out"},
	{"SpkN Out", NULL, "Speaker Mixer"},
	{"SpkP Out", NULL, "Speaker Mixer"},
	{"SPKOUTN", NULL, "SpkN Out"},
	{"SPKOUTP", NULL, "SpkP Out"},
  
	/* Boost Mixer */
	{"ADC", NULL, "Boost Mixer"},
	//{"Capture Boost Switch", "Aux Capture Boost Switch", "AUX"},
	//{"Boost Mixer", "Aux Volume", "Aux Boost"},
	// FIXME
	{"Capture Boost", NULL /*"Capture Switch"*/, "Boost Mixer"},
	{"Boost Mixer", NULL /*"Mic Volume"*/, "Mic Boost"},
	{"Boost Mixer", NULL /*"Mic Volume"*/, "Mic PGA"},
  
	/* Inputs */
	{"Mic Boost", NULL, "MICP"},
	{"Mic PGA", NULL, "MICN"},
	{"Capture Boost", NULL, "Mic PGA"},
	{"Aux Input", NULL, "AUX"},
	
	/* terminator */
	{NULL, NULL, NULL},
};

static int wm8974_add_widgets(struct snd_soc_codec *codec, 
	struct snd_soc_machine *machine)
{
	int i;

	for(i = 0; i < ARRAY_SIZE(wm8974_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec, 
			&wm8974_dapm_widgets[i]);
	}

	/* set up audio path audio_mapnects */
	for(i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine, audio_map[i][0],
			audio_map[i][1], audio_map[i][2]);
	}

	snd_soc_dapm_new_widgets(machine);
	return 0;
}

static int wm8974_set_clkdiv(struct snd_soc_dai *codec_dai, 
	int div_id, int div)
{
	struct snd_soc_codec *codec = codec_dai->codec;
        u16 reg;

	switch (div_id) {
	case WM8974_OPCLKDIV:
		reg = wm8974_read_reg_cache(codec, WM8974_GPIO) & 0x1cf;
		wm8974_write_reg(codec, WM8974_GPIO, reg | div);
		break;
	case WM8974_MCLKDIV:
		reg = wm8974_read_reg_cache(codec, WM8974_CLOCK) & 0x1f;
		wm8974_write_reg(codec, WM8974_CLOCK, reg | div);
		break;
	case WM8974_ADCCLK:
		reg = wm8974_read_reg_cache(codec, WM8974_ADC) & 0x1f7;
		wm8974_write_reg(codec, WM8974_ADC, reg | div);
		break;
	case WM8974_DACCLK:
		reg = wm8974_read_reg_cache(codec, WM8974_DAC) & 0x1f7;
		wm8974_write_reg(codec, WM8974_DAC, reg | div);
		break;
	case WM8974_BCLKDIV:
		reg = wm8974_read_reg_cache(codec, WM8974_CLOCK) & 0x1e3;
		wm8974_write_reg(codec, WM8974_CLOCK, reg | div);
		break;
	default:
		return -EINVAL;
	}
 
	return 0;
}

static int wm8974_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = 0;
	u16 clk = wm8974_read_reg_cache(codec, WM8974_CLOCK) & 0x1fe;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		clk |= 0x0001;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}
 
	/* interface format */
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 0x0010;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= 0x0008;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= 0x0018;
		break;
	default:
		return -EINVAL;
	}

#if 1
	// FIXME no sound without this clock inversion
	iface |= 0x0080;
#else
	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x0180;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x0100;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x0080;
		break;
	default:
		return -EINVAL;
	}
#endif

	wm8974_write_reg(codec, WM8974_IFACE, iface);
	wm8974_write_reg(codec, WM8974_CLOCK, clk);

	return 0;
}

static void wm8974_pcm_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;

	/* mute early to avoid pops */
	wm8974_mute(pcm_link->codec_dai, 1);
}

static int wm8974_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_codec *codec = pcm_link->codec;
	u16 iface = wm8974_read_reg_cache(codec, WM8974_IFACE) & 0x19f;
	u16 adn = wm8974_read_reg_cache(codec, WM8974_ADD) & 0x1f1;

	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		iface |= 0x0020;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		iface |= 0x0040;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		iface |= 0x0060;
		break;
	}

	/* filter coefficient */
	switch (params_rate(params)) {
	case SNDRV_PCM_RATE_8000:
		adn |= 0x5 << 1;
		break;
	case SNDRV_PCM_RATE_11025:
		adn |= 0x4 << 1;
		break;
	case SNDRV_PCM_RATE_16000:
		adn |= 0x3 << 1;
		break;
	case SNDRV_PCM_RATE_22050:
		adn |= 0x2 << 1;
		break;
	case SNDRV_PCM_RATE_32000:
		adn |= 0x1 << 1;
		break;
	case SNDRV_PCM_RATE_44100:
	case SNDRV_PCM_RATE_48000:
	case SNDRV_PCM_RATE_88200:
	case SNDRV_PCM_RATE_96000:
		break;
	}

	wm8974_write_reg(codec, WM8974_IFACE, iface);
	wm8974_write_reg(codec, WM8974_ADD, adn);

	return 0;
}

static int wm8974_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = wm8974_read_reg_cache(codec, WM8974_DAC) & 0xffbf;
	u16 spk_reg = wm8974_read_reg_cache(codec, WM8974_SPKVOL) & 0xffbf;

	if (mute) {
		wm8974_write_reg(codec, WM8974_SPKVOL, spk_reg | 0x40);
		wm8974_write_reg(codec, WM8974_DAC, mute_reg | 0x40);
	}
	else {
		wm8974_write_reg(codec, WM8974_DAC, mute_reg);
		wm8974_write_reg(codec, WM8974_SPKVOL, spk_reg);
	}
        return 0;
}

#define WM8974_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | \
		      SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
		      SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
		      SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | \
		      SNDRV_PCM_RATE_96000)

#define WM8974_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_pcm_stream wm8974_hifi_dai_playback = {
	.stream_name	= "Playback",
	.channels_min	= 1,
	.channels_max	= 1,
	.rates		= WM8974_RATES,
	.formats	= WM8974_FORMATS,
};

static const struct snd_soc_pcm_stream wm8974_hifi_dai_capture = {
	.stream_name	= "Capture",
	.channels_min	= 1,
	.channels_max	= 1,
	.rates		= WM8974_RATES,
	.formats	= WM8974_FORMATS,
};

/* dai ops, called by machine drivers */
static const struct snd_soc_dai_ops wm8974_hifi_dai_ops = {
	.digital_mute = wm8974_mute,
	.set_fmt = wm8974_set_dai_fmt,
	.set_clkdiv = wm8974_set_clkdiv,
};

/* audio ops, called by alsa */
static const struct snd_soc_ops wm8974_hifi_dai_audio_ops = {
	.shutdown = wm8974_pcm_shutdown,
	.hw_params = wm8974_pcm_hw_params,
};

static int wm8974_suspend(struct device *dev, pm_message_t state)
{
	return 0;
}

static int wm8974_resume(struct device *dev)
{
	return 0;
}

static int wm8974_codec_io_probe(struct snd_soc_codec *codec,
	struct snd_soc_machine *machine)
{
	struct wm8974* wm8974 = codec->control_data;
	u16 reg;
	
	snd_assert(wm8974 != NULL, return -EINVAL);

	/* reset codec */
	wm8974_write_reg(codec, WM8974_RESET, 0xFFFF);

	wm8974_write_reg(codec, WM8974_POWER1,
			 ( 1 << 4) | /* MICBEN */
			 ( 1 << 3) | /* BIASEN */
			 ( 1 << 2) | /* BUFIOEN */
			 ( 3 << 0)   /* VMIDSEL = 11 */
			 );

	wm8974_write_reg(codec, WM8974_POWER2,
			 ( 1 << 4) |  /* BOOSTEN */
			 ( 1 << 2) |  /* INPPGAEN */
			 ( 1 << 0)    /* ADCEN */
			 );

	wm8974_write_reg(codec, WM8974_POWER3,
			 ( 1 << 0) | /* DACEN */
			 ( 1 << 5) | /* SPKPEN */
			 ( 1 << 6) | /* SPKNEN */
			 ( 1 << 2)   /* SPKMIXEN */
			 );

	wm8974_write_reg(codec, WM8974_DAC,
			 ( 1 << 6) | /* DACMU */
			 ( 1 << 3)   /* DACOSR128 */
			 );

	wm8974_write_reg(codec, WM8974_ADC,
			 ( 1 << 8) |  /* HPFEN */
			 ( 1 << 7) |  /* HPFAPP */
			 ( 2 << 4) |  /* HPFCUT = 010 */
			 ( 1 << 3)    /* ADCOSR */
			 );

	wm8974_write_reg(codec, WM8974_ADCBOOST,
			 ( 1 << 8)    /* PGAPOOST */
			 );

	wm8974_write_reg(codec, WM8974_SPKMIX,
			 ( 1 << 0)   /* DAC2SPK */
			 );

	wm8974_write_reg(codec, WM8974_SPKVOL,
			 ( 1 << 6) | /* SPKMUTE */
			 ( 1 << 7)   /* SPKZC */
			 );

#if 1
	/* default volumes */
	reg = wm8974_read_reg_cache(codec, WM8974_INPPGA) & 0x1C0;
	wm8974_write_reg(codec, WM8974_INPPGA, reg |
			 ( 47 << 0 )  /* INPPGAVOL */
			 );

	reg = wm8974_read_reg_cache(codec, WM8974_SPKVOL) & 0x1C0;
	wm8974_write_reg(codec, WM8974_SPKVOL, reg |
			 ( 63 << 0)   /* SPKVOL */
			 );
#endif
	
	wm8974_add_controls(codec, machine->card);
	wm8974_add_widgets(codec, machine);
	
	return 0;
}

static int wm8974_codec_io_remove(struct snd_soc_codec *codec,
	struct snd_soc_machine *machine)
{
	return 0;	
}

static const struct snd_soc_codec_ops wm8974_codec_ops = {
	.read		= wm8974_read_reg_cache,
	.write		= wm8974_write_reg,
	.io_probe	= wm8974_codec_io_probe,
	.io_remove	= wm8974_codec_io_remove,
};


#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

static struct i2c_client *wm8974_i2c_client;

static int wm8974_i2c_probe(struct i2c_client *client)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "No SMBUS byte data capability\n");
		return -EEXIST;
	}

	wm8974_i2c_client = client;
 
	return 0;
}

static int wm8974_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	return 0;
}

static struct i2c_driver wm8974_i2c_driver = {
	.driver = {
		.name = "wm8974-i2c",
		.owner = THIS_MODULE,
	},
	.probe	= wm8974_i2c_probe,
	.remove	= wm8974_i2c_remove,
};
#endif


static int wm8974_codec_probe(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);

	printk(KERN_INFO "WM8974 Audio Codec %s\n", WM8974_VERSION);

	codec->owner = THIS_MODULE;
	codec->ops = &wm8974_codec_ops;
	codec->reg_cache_size = ARRAY_SIZE(wm8974_reg);
	codec->reg_cache = kmemdup(wm8974_reg, sizeof(wm8974_reg), GFP_KERNEL);

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	codec->control_data = wm8974_i2c_client;
	codec->mach_write = i2c_master_send;
#endif

 	snd_soc_register_codec(codec);

	return 0;
}

static int wm8974_codec_remove(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);
	kfree(codec->private_data);
	return 0;
}

static int wm8974_hifi_dai_probe(struct device *dev)
{
	struct snd_soc_dai *dai = to_snd_soc_dai(dev);
		
	dai->ops = &wm8974_hifi_dai_ops;
	dai->audio_ops = &wm8974_hifi_dai_audio_ops;
	dai->capture = &wm8974_hifi_dai_capture;
	dai->playback = &wm8974_hifi_dai_playback;
	snd_soc_register_codec_dai(dai);
	return 0;
}

const char wm8974_codec[SND_SOC_CODEC_NAME_SIZE] = "wm8974-codec";
EXPORT_SYMBOL_GPL(wm8974_codec);

static struct snd_soc_device_driver wm8974_codec_driver = {
	.type	= SND_SOC_BUS_TYPE_CODEC,
	.driver	= {
		.name 		= wm8974_codec,
		.owner		= THIS_MODULE,
		.bus 		= &asoc_bus_type,
		.probe		= wm8974_codec_probe,
		.remove		= __devexit_p(wm8974_codec_remove),
		.suspend	= wm8974_suspend,
		.resume		= wm8974_resume,
	},
};

const char wm8974_hifi_dai[SND_SOC_CODEC_NAME_SIZE] = "wm8974-hifi-dai";
EXPORT_SYMBOL_GPL(wm8974_hifi_dai);

static struct snd_soc_device_driver wm8974_hifi_dai_driver = {
	.type	= SND_SOC_BUS_TYPE_DAI,
	.driver	= {
		.name 		= wm8974_hifi_dai,
		.owner		= THIS_MODULE,
		.bus 		= &asoc_bus_type,
		.probe		= wm8974_hifi_dai_probe,
	},
};

static __init int wm8974_init(void)
{
	int ret = 0;
	
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&wm8974_i2c_driver);
	if (ret != 0)
		goto error0;
#endif

	ret = driver_register(&wm8974_codec_driver.driver);
	if (ret < 0)
		goto error1;

	ret = driver_register(&wm8974_hifi_dai_driver.driver);
	if (ret < 0)
		goto error2;

	return ret;

 error2:
	driver_unregister(&wm8974_codec_driver.driver);
 error1:
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&wm8974_i2c_driver);
#endif
 error0:
	return ret;
}

static __exit void wm8974_exit(void)
{
	driver_unregister(&wm8974_hifi_dai_driver.driver);
	driver_unregister(&wm8974_codec_driver.driver);
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&wm8974_i2c_driver);
#endif
}

module_init(wm8974_init);
module_exit(wm8974_exit);

MODULE_DESCRIPTION("ASoC WM8974 driver");
MODULE_AUTHOR("Richard Titmuss");
MODULE_LICENSE("GPL");
