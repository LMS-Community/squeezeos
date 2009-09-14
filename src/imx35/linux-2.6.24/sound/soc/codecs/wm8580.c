/*
 * wm8580.c  --  WM8580 ALSA Soc Audio driver
 *
 * Copyright 2008 Freescale Semiconductor, Inc.
 * Copyright 2008 Wolfson Microelectronics PLC.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Notes:
 *  The WM8580 is a multichannel codec with S/PDIF support, featuring six
 *  DAC channels and two ADC channels.
 *
 *  Currently only the primary audio interface is supported - S/PDIF and
 *  the secondary audio interfaces are not.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include "wm8580.h"

#define AUDIO_NAME "WM8580"
#define WM8580_VERSION "0.1"

/*
 * Debug
 */

/* #define WM8580_DEBUG 1 */

#ifdef WM8580_DEBUG
#define dbg(format, arg...) \
	printk(AUDIO_NAME ": " format "\n", ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n", ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n", ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n", ## arg)

struct pll_state {
	unsigned int in;
	unsigned int out;
};

struct wm8580 {
	struct pll_state a;
	struct pll_state b;
};

/*
 * Register values.
 */
/* WM8580 register space */
#define WM8580_PLLA1                         0x00
#define WM8580_PLLA2                         0x01
#define WM8580_PLLA3                         0x02
#define WM8580_PLLA4                         0x03
#define WM8580_PLLB1                         0x04
#define WM8580_PLLB2                         0x05
#define WM8580_PLLB3                         0x06
#define WM8580_PLLB4                         0x07
#define WM8580_CLKSEL                        0x08
#define WM8580_PAIF1                         0x09
#define WM8580_PAIF2                         0x0A
#define WM8580_SAIF1                         0x0B
#define WM8580_PAIF3                         0x0C
#define WM8580_PAIF4                         0x0D
#define WM8580_SAIF2                         0x0E
#define WM8580_DAC_CONTROL1                  0x0F
#define WM8580_DAC_CONTROL2                  0x10
#define WM8580_DAC_CONTROL3                  0x11
#define WM8580_DAC_CONTROL4                  0x12
#define WM8580_DAC_CONTROL5                  0x13
#define WM8580_DIGITAL_ATTENUATION_DACL1     0x14
#define WM8580_DIGITAL_ATTENUATION_DACR1     0x15
#define WM8580_DIGITAL_ATTENUATION_DACL2     0x16
#define WM8580_DIGITAL_ATTENUATION_DACR2     0x17
#define WM8580_DIGITAL_ATTENUATION_DACL3     0x18
#define WM8580_DIGITAL_ATTENUATION_DACR3     0x19
#define WM8580_MASTER_DIGITAL_ATTENUATION    0x1C
#define WM8580_ADC_CONTROL1                  0x1D
#define WM8580_SPDTXCHAN0                    0x1E
#define WM8580_SPDTXCHAN1                    0x1F
#define WM8580_SPDTXCHAN2                    0x20
#define WM8580_SPDTXCHAN3                    0x21
#define WM8580_SPDTXCHAN4                    0x22
#define WM8580_SPDTXCHAN5                    0x23
#define WM8580_SPDMODE                       0x24
#define WM8580_INTMASK                       0x25
#define WM8580_GPO1                          0x26
#define WM8580_GPO2                          0x27
#define WM8580_GPO3                          0x28
#define WM8580_GPO4                          0x29
#define WM8580_GPO5                          0x2A
#define WM8580_INTSTAT                       0x2B
#define WM8580_SPDRXCHAN1                    0x2C
#define WM8580_SPDRXCHAN2                    0x2D
#define WM8580_SPDRXCHAN3                    0x2E
#define WM8580_SPDRXCHAN4                    0x2F
#define WM8580_SPDRXCHAN5                    0x30
#define WM8580_SPDSTAT                       0x31
#define WM8580_PWRDN1                        0x32
#define WM8580_PWRDN2                        0x33
#define WM8580_READBACK                      0x34
#define WM8580_RESET                         0x35

/* PLLB4 (register 7h) */
#define WM8580_PLLB4_MCLKOUTSRC_MASK   0x60
#define WM8580_PLLB4_MCLKOUTSRC_PLLA   0x20
#define WM8580_PLLB4_MCLKOUTSRC_PLLB   0x40
#define WM8580_PLLB4_MCLKOUTSRC_OSC    0x60

#define WM8580_PLLB4_CLKOUTSRC_MASK    0x180
#define WM8580_PLLB4_CLKOUTSRC_PLLACLK 0x080
#define WM8580_PLLB4_CLKOUTSRC_PLLBCLK 0x100
#define WM8580_PLLB4_CLKOUTSRC_OSCCLK  0x180

/* CLKSEL (register 8h) */
#define WM8580_CLKSEL_DAC_CLKSEL_MASK 0x03
#define WM8580_CLKSEL_DAC_CLKSEL_PLLA 0x01
#define WM8580_CLKSEL_DAC_CLKSEL_PLLB 0x02

/* AIF control 1 (registers 9h-bh) */
#define WM8580_AIF_RATE_MASK       0x7
#define WM8580_AIF_RATE_128        0x0
#define WM8580_AIF_RATE_192        0x1
#define WM8580_AIF_RATE_256        0x2
#define WM8580_AIF_RATE_384        0x3
#define WM8580_AIF_RATE_512        0x4
#define WM8580_AIF_RATE_768        0x5
#define WM8580_AIF_RATE_1152       0x6

#define WM8580_AIF_BCLKSEL_MASK   0x18
#define WM8580_AIF_BCLKSEL_64     0x00
#define WM8580_AIF_BCLKSEL_128    0x08
#define WM8580_AIF_BCLKSEL_256    0x10
#define WM8580_AIF_BCLKSEL_SYSCLK 0x18

#define WM8580_AIF_MS             0x20

#define WM8580_AIF_CLKSRC_MASK    0xc0
#define WM8580_AIF_CLKSRC_PLLA    0x40
#define WM8580_AIF_CLKSRC_PLLB    0x40
#define WM8580_AIF_CLKSRC_MCLK    0xc0

/* AIF control 2 (registers ch-eh) */
#define WM8580_AIF_FMT_MASK    0x03
#define WM8580_AIF_FMT_RIGHTJ  0x00
#define WM8580_AIF_FMT_LEFTJ   0x01
#define WM8580_AIF_FMT_I2S     0x02
#define WM8580_AIF_FMT_DSP     0x03

#define WM8580_AIF_LENGTH_MASK   0x0c
#define WM8580_AIF_LENGTH_16     0x00
#define WM8580_AIF_LENGTH_20     0x04
#define WM8580_AIF_LENGTH_24     0x08
#define WM8580_AIF_LENGTH_32     0x0c

#define WM8580_AIF_LRP         0x10
#define WM8580_AIF_BCP         0x20

/* Powerdown Register 1 (register 32h) */
#define WM8580_PWRDN1_PWDN     0x001
#define WM8580_PWRDN1_ALLDACPD 0x040

/* Powerdown Register 2 (register 33h) */
#define WM8580_PWRDN2_OSCPD   0x001
#define WM8580_PWRDN2_PLLAPD   0x002
#define WM8580_PWRDN2_PLLBPD   0x004
#define WM8580_PWRDN2_SPDIFPD  0x008
#define WM8580_PWRDN2_SPDIFTXD 0x010
#define WM8580_PWRDN2_SPDIFRXD 0x020

#define WM8580_REG_DATA_LENGTH   9
/*DAC muteall flag */
#define WM8580_MUTEALL    (1<<4)
/*Update flag */
#define WM8580_DA_UPDATE  (1<<8)

/*
 * wm8580 register cache
 * We can't read the WM8580 register space when we
 * are using 2 wire for device control, so we cache them instead.
 */
static const u16 wm8580_reg[] = {
	0x0121, 0x017e, 0x007d, 0x0014,	/*R3 */
	0x0121, 0x017e, 0x007d, 0x0194,	/*R7 */
	0x001c, 0x0002, 0x0002, 0x00c2,	/*R11 */
	0x0182, 0x0082, 0x000a, 0x0024,	/*R15 */
	0x0009, 0x0000, 0x00ff, 0x0000,	/*R19 */
	0x00ff, 0x00ff, 0x00ff, 0x00ff,	/*R23 */
	0x00ff, 0x00ff, 0x00ff, 0x00ff,	/*R27 */
	0x01f0, 0x0040, 0x0000, 0x0000,	/*R31(0x1F) */
	0x0000, 0x0000, 0x0031, 0x000b,	/*R35 */
	0x0039, 0x0000, 0x0010, 0x0032,	/*R39 */
	0x0054, 0x0076, 0x0098, 0x0000,	/*R43(0x2B) */
	0x0000, 0x0000, 0x0000, 0x0000,	/*R47 */
	0x0000, 0x0000, 0x005e, 0x003e,	/*R51(0x33) */
	0x0000, 0x0000		/*R53 */
};

static struct wm8580 *wm8580;

/*
 * read wm8580 register cache
 */
static inline unsigned int wm8580_read_reg_cache(struct snd_soc_codec *codec,
						 unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	BUG_ON(reg > ARRAY_SIZE(wm8580_reg));
	return cache[reg];
}

/*
 * write wm8580 register cache
 */
static inline void wm8580_write_reg_cache(struct snd_soc_codec *codec,
					  unsigned int reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;

	cache[reg] = value;
}

/*
 * write to the WM8580 register space
 */
static int wm8580_write(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	u16 data;

	BUG_ON(reg > ARRAY_SIZE(wm8580_reg));

	/* Registers are 9 bits wide */
	value &= 0x1ff;

	data = (reg << WM8580_REG_DATA_LENGTH) + value;

	wm8580_write_reg_cache(codec, reg, value);
	codec->mach_write(codec->control_data, (long)&data, 1);
	dbg("reg %d: value 0x%x", reg, value);
	return 0;
}

static unsigned int wm8580_read(struct snd_soc_codec *codec, unsigned int reg)
{
	u16 data;

	if (reg >= WM8580_SPDRXCHAN1 && reg <= WM8580_SPDRXCHAN5) {
		wm8580_write(codec, WM8580_READBACK, 0x10);
		data = reg << WM8580_REG_DATA_LENGTH;
		codec->mach_read(codec->control_data, (long)&data, 1);
		return (data & 0xff);
	} else
		return wm8580_read_reg_cache(codec, reg);

}

/*
 * Safe read, modify, write methods
 */
int wm8580_clear_bits(struct snd_soc_codec *codec, u8 reg, u16 mask)
{
	u16 value;
	int err;
	value = wm8580_read(codec, reg) & ~mask;

	err = wm8580_write(codec, reg, value);
	if (err)
		err("write to reg R%d failed\n", reg);
	return err;
}

EXPORT_SYMBOL_GPL(wm8580_clear_bits);

int wm8580_set_bits(struct snd_soc_codec *codec, u8 reg, u16 mask)
{
	u16 value;
	int err;

	value = wm8580_read(codec, reg) | mask;

	err = wm8580_write(codec, reg, value);
	if (err)
		err("write to reg R%d failed\n", reg);
	return err;
}

EXPORT_SYMBOL_GPL(wm8580_set_bits);

static const DECLARE_TLV_DB_SCALE(dac_tlv, -12750, 50, 1);

static int wm8580_out_vu(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int ret;
	u16 val;

	/* Clear the register cache so we write without VU set */
	wm8580_write_reg_cache(codec, reg, 0);

	ret = snd_soc_put_volsw(kcontrol, ucontrol);
	if (ret < 0)
		return ret;

	/* Now write again with the volume update bit set */
	val = wm8580_read_reg_cache(codec, reg);
	return wm8580_write(codec, reg, val | 0x0100);
}

#define SOC_WM8580_OUT_SINGLE_R_TLV(xname, reg, shift, max, invert, tlv_array) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		SNDRV_CTL_ELEM_ACCESS_READWRITE,  \
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw, \
	.get = snd_soc_get_volsw, .put = wm8580_out_vu, \
	.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }

static const struct snd_kcontrol_new wm8580_snd_controls[] = {
	SOC_WM8580_OUT_SINGLE_R_TLV("Left DAC1 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACL1,
				    0, 0xff, 0, dac_tlv),
	SOC_WM8580_OUT_SINGLE_R_TLV("Right DAC1 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACR1,
				    0, 0xff, 0, dac_tlv),
	SOC_WM8580_OUT_SINGLE_R_TLV("Left DAC2 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACL2,
				    0, 0xff, 0, dac_tlv),
	SOC_WM8580_OUT_SINGLE_R_TLV("Right DAC2 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACR2,
				    0, 0xff, 0, dac_tlv),
	SOC_WM8580_OUT_SINGLE_R_TLV("Left DAC3 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACL3,
				    0, 0xff, 0, dac_tlv),
	SOC_WM8580_OUT_SINGLE_R_TLV("Right DAC3 Playback Volume",
				    WM8580_DIGITAL_ATTENUATION_DACR3,
				    0, 0xff, 0, dac_tlv),

	SOC_SINGLE("DAC1 Deemphasis Switch", WM8580_DAC_CONTROL3, 0, 1, 0),
	SOC_SINGLE("DAC2 Deemphasis Switch", WM8580_DAC_CONTROL3, 1, 1, 0),
	SOC_SINGLE("DAC3 Deemphasis Switch", WM8580_DAC_CONTROL3, 2, 1, 0),

	SOC_SINGLE("DAC1 Left Invert Switch", WM8580_DAC_CONTROL4, 0, 1, 0),
	SOC_SINGLE("DAC1 Right Invert Switch", WM8580_DAC_CONTROL4, 1, 1, 0),
	SOC_SINGLE("DAC2 Left Invert Switch", WM8580_DAC_CONTROL4, 2, 1, 0),
	SOC_SINGLE("DAC2 Right Invert Switch", WM8580_DAC_CONTROL4, 3, 1, 0),
	SOC_SINGLE("DAC3 Left Invert Switch", WM8580_DAC_CONTROL4, 4, 1, 0),
	SOC_SINGLE("DAC3 Right Invert Switch", WM8580_DAC_CONTROL4, 5, 1, 0),

	SOC_SINGLE("DAC ZC Switch", WM8580_DAC_CONTROL5, 5, 1, 0),
	SOC_SINGLE("DAC1 Mute Switch", WM8580_DAC_CONTROL5, 0, 1, 0),
	SOC_SINGLE("DAC2 Mute Switch", WM8580_DAC_CONTROL5, 1, 1, 0),
	SOC_SINGLE("DAC3 Mute Switch", WM8580_DAC_CONTROL5, 2, 1, 0),
	SOC_SINGLE("All DACs Mute Switch", WM8580_DAC_CONTROL5, 4, 1, 0),

/*
	SOC_SINGLE("ADCL Mute Switch", WM8580_ADC_CONTROL1, 0, 1, 0),
	SOC_SINGLE("ADCR Mute Switch", WM8580_ADC_CONTROL1, 1, 1, 0),
	SOC_SINGLE("ADC High-Pass Filter Switch", WM8580_ADC_CONTROL1, 4, 1, 0),
*/
};

/* Add non-DAPM controls */
static int wm8580_add_controls(struct snd_soc_codec *codec,
			       struct snd_card *card)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(wm8580_snd_controls); i++) {
		err = snd_ctl_add(card,
				  snd_soc_cnew(&wm8580_snd_controls[i],
					       codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}
static const struct snd_soc_dapm_widget wm8580_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC1", "Playback", WM8580_PWRDN1, 2, 1),
	SND_SOC_DAPM_DAC("DAC2", "Playback", WM8580_PWRDN1, 3, 1),
	SND_SOC_DAPM_DAC("DAC3", "Playback", WM8580_PWRDN1, 4, 1),

	SND_SOC_DAPM_OUTPUT("VOUT1L"),
	SND_SOC_DAPM_OUTPUT("VOUT1R"),
	SND_SOC_DAPM_OUTPUT("VOUT2L"),
	SND_SOC_DAPM_OUTPUT("VOUT2R"),
	SND_SOC_DAPM_OUTPUT("VOUT3L"),
	SND_SOC_DAPM_OUTPUT("VOUT3R"),

	SND_SOC_DAPM_ADC("ADC", "Capture", WM8580_PWRDN1, 1, 1),

	SND_SOC_DAPM_INPUT("AINL"),
	SND_SOC_DAPM_INPUT("AINR"),
};

static const char *audio_map[][3] = {
	{"VOUT1L", NULL, "DAC1"},
	{"VOUT1R", NULL, "DAC1"},

	{"VOUT2L", NULL, "DAC2"},
	{"VOUT2R", NULL, "DAC2"},

	{"VOUT3L", NULL, "DAC3"},
	{"VOUT3R", NULL, "DAC3"},

	{"ADC", NULL, "AINL"},
	{"ADC", NULL, "AINR"},

	/* terminator */
	{NULL, NULL, NULL},
};

static int wm8580_add_widgets(struct snd_soc_codec *codec,
			      struct snd_soc_machine *machine)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(wm8580_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec,
					 &wm8580_dapm_widgets[i]);
	}

	/* set up audio path audio_mapnects */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);
	}

	snd_soc_dapm_new_widgets(machine);
	return 0;
}

/* PLL divisors */
struct _pll_div {
	u32 prescale:1;
	u32 postscale:1;
	u32 freqmode:2;
	u32 n:4;
	u32 k:24;
};

/* The size in bits of the pll divide */
#define FIXED_PLL_SIZE (1 << 22)

/* PLL rate to output rate divisions */
static struct {
	unsigned int div;
	unsigned int freqmode;
	unsigned int postscale;
} post_table[] = {
	{
	2, 0, 0}, {
	4, 0, 1}, {
	4, 1, 0}, {
	8, 1, 1}, {
	8, 2, 0}, {
	16, 2, 1}, {
	12, 3, 0}, {
	24, 3, 1}
};

static int pll_factors(struct _pll_div *pll_div, unsigned int target,
		       unsigned int source)
{
	u64 Kpart;
	unsigned int K, Ndiv, Nmod;
	int i;

	dbg("wm8580: PLL %dHz->%dHz\n", source, target);

	/* Scale the output frequency up; the PLL should run in the
	 * region of 90-100MHz.
	 */
	for (i = 0; i < ARRAY_SIZE(post_table); i++) {
		if (target * post_table[i].div >= 90000000 &&
		    target * post_table[i].div <= 100000000) {
			pll_div->freqmode = post_table[i].freqmode;
			pll_div->postscale = post_table[i].postscale;
			target *= post_table[i].div;
			break;
		}
	}

	if (i == ARRAY_SIZE(post_table)) {
		err("Unable to scale output frequency " "%u\n", target);
		return -EINVAL;
	}

	Ndiv = target / source;

	if (Ndiv < 5) {
		source /= 2;
		pll_div->prescale = 1;
		Ndiv = target / source;
	} else
		pll_div->prescale = 0;

	if ((Ndiv < 5) || (Ndiv > 13)) {
		err("N=%d outside supported range\n", Ndiv);
		return -EINVAL;
	}

	pll_div->n = Ndiv;
	Nmod = target % source;
	Kpart = FIXED_PLL_SIZE * (long long)Nmod;

	do_div(Kpart, source);

	K = Kpart & 0xFFFFFFFF;

	pll_div->k = K;

	dbg("PLL %x.%x prescale %d freqmode %d postscale %d\n",
	    pll_div->n, pll_div->k, pll_div->prescale, pll_div->freqmode,
	    pll_div->postscale);

	return 0;
}

static int wm8580_set_dai_pll(struct snd_soc_dai *codec_dai,
			      int pll_id, unsigned int freq_in,
			      unsigned int freq_out)
{
	int offset;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8580 *wm8580 = codec->private_data;
	struct pll_state *state;
	struct _pll_div pll_div;
	unsigned int reg;
	unsigned int pwr_mask;
	int ret;

	memset((char *)&pll_div, 0, sizeof(struct _pll_div));

	switch (pll_id) {
	case WM8580_PLLA:
		state = &wm8580->a;
		offset = 0;
		pwr_mask = WM8580_PWRDN2_PLLAPD;
		break;
	case WM8580_PLLB:
		state = &wm8580->b;
		offset = 4;
		pwr_mask = WM8580_PWRDN2_PLLBPD;
		break;
	case WM8580_PLL_NONE:
		/* disable PLL */
		reg = wm8580_read(codec, WM8580_PWRDN2);
		wm8580_write(codec, WM8580_PWRDN2, reg | 0x06);
		return 0;
	default:
		return -ENODEV;
	}

	if (freq_in && freq_out) {
		ret = pll_factors(&pll_div, freq_out, freq_in);
		if (ret != 0)
			return ret;
	}

	state->in = freq_in;
	state->out = freq_out;

	/* Always disable the PLL - it is not safe to leave it running
	 * while reprogramming it.
	 */
	reg = wm8580_read(codec, WM8580_PWRDN2);
	wm8580_write(codec, WM8580_PWRDN2, reg | pwr_mask);

	if (!freq_in || !freq_out)
		return 0;

	wm8580_write(codec, WM8580_PLLA1 + offset, pll_div.k & 0x1ff);
	wm8580_write(codec, WM8580_PLLA2 + offset, (pll_div.k >> 9) & 0x1ff);
	wm8580_write(codec, WM8580_PLLA3 + offset,
		     (pll_div.k >> 18 & 0xf) | (pll_div.n << 4));

	reg = wm8580_read(codec, WM8580_PLLA4 + offset);
	reg &= ~0x3b;
	reg |= pll_div.prescale | pll_div.postscale << 1 |
	    pll_div.freqmode << 3;

	wm8580_write(codec, WM8580_PLLA4 + offset, reg);

	/* All done, turn it on */
	reg = wm8580_read(codec, WM8580_PWRDN2);
	wm8580_write(codec, WM8580_PWRDN2, reg & ~pwr_mask);

	return 0;
}

/*
 * Set PCM DAI bit size and sample rate.
 */
static int wm8580_paif_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_codec *codec = pcm_link->codec;
	int offset = 0;
	u16 paifb;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		offset = 0;
	else
		offset = 1;

	paifb = wm8580_read(codec, WM8580_PAIF3 + offset);

	paifb &= ~WM8580_AIF_LENGTH_MASK;
	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		dbg(KERN_CRIT "16 bit\n");
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		dbg(KERN_CRIT "20 bit\n");
		paifb |= WM8580_AIF_LENGTH_20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dbg(KERN_CRIT "24 bit\n");
		paifb |= WM8580_AIF_LENGTH_24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dbg(KERN_CRIT "32 bit\n");
		paifb |= WM8580_AIF_LENGTH_24;
		break;
	default:
		return -EINVAL;
	}

	wm8580_write(codec, WM8580_PAIF3 + offset, paifb);
	return 0;
}

static int wm8580_set_paif_dai_fmt(struct snd_soc_dai *codec_dai,
				   unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int aifa;
	unsigned int aifb;
	int can_invert_lrclk;

	aifa = wm8580_read(codec, WM8580_PAIF1);
	aifb = wm8580_read(codec, WM8580_PAIF3);

	aifb &= ~(WM8580_AIF_FMT_MASK | WM8580_AIF_LRP | WM8580_AIF_BCP);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		aifa &= ~WM8580_AIF_MS;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		aifa |= WM8580_AIF_MS;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		can_invert_lrclk = 1;
		aifb |= WM8580_AIF_FMT_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		can_invert_lrclk = 1;
		aifb |= WM8580_AIF_FMT_RIGHTJ;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		can_invert_lrclk = 1;
		aifb |= WM8580_AIF_FMT_LEFTJ;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		can_invert_lrclk = 0;
		aifb |= WM8580_AIF_FMT_DSP;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		can_invert_lrclk = 0;
		aifb |= WM8580_AIF_FMT_DSP;
		aifb |= WM8580_AIF_LRP;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;

	case SND_SOC_DAIFMT_IB_IF:
		if (!can_invert_lrclk)
			return -EINVAL;
		aifb |= WM8580_AIF_BCP;
		aifb |= WM8580_AIF_LRP;
		break;

	case SND_SOC_DAIFMT_IB_NF:
		aifb |= WM8580_AIF_BCP;
		break;

	case SND_SOC_DAIFMT_NB_IF:
		if (!can_invert_lrclk)
			return -EINVAL;
		aifb |= WM8580_AIF_LRP;
		break;

	default:
		return -EINVAL;
	}

	wm8580_write(codec, WM8580_PAIF1, aifa);
	wm8580_write(codec, WM8580_PAIF3, aifb);

	return 0;
}

static int wm8580_set_paif_dai_tdm_slot(struct snd_soc_dai *codec_dai,
					unsigned int mask, int slots)
{
	return 0;
}

static int wm8580_set_paif_dai_tristate(struct snd_soc_dai *codec_dai,
					int tristate)
{
	return 0;
}

static int wm8580_set_dai_clkdiv(struct snd_soc_dai *codec_dai,
				 int div_id, int div)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 val;

	switch (div_id) {
	case WM8580_BCLK_CLKDIV:
		val = wm8580_read(codec, WM8580_PAIF1) &
		    ~WM8580_AIF_BCLKSEL_MASK;
		wm8580_write(codec, WM8580_PAIF1, val | div);
		val = wm8580_read(codec, WM8580_PAIF2) &
		    ~WM8580_AIF_BCLKSEL_MASK;
		wm8580_write(codec, WM8580_PAIF2, val | div);
		break;
	case WM8580_LRCLK_CLKDIV:
		val = wm8580_read(codec, WM8580_PAIF1) & ~0x07;
		wm8580_write(codec, WM8580_PAIF1, val | div);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8580_set_paif_dai_sysclk(struct snd_soc_dai *codec_dai,
				      int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int reg;

	switch (clk_id) {
	case WM8580_MCLK:
		reg = wm8580_read(codec, WM8580_PLLB4);
		reg &= ~WM8580_PLLB4_MCLKOUTSRC_MASK;

		switch (dir) {
		case WM8580_CLKSRC_MCLK:
			/* Input */
			break;

		case WM8580_CLKSRC_PLLA:
			reg |= WM8580_PLLB4_MCLKOUTSRC_PLLA;
			break;
		case WM8580_CLKSRC_PLLB:
			reg |= WM8580_PLLB4_MCLKOUTSRC_PLLB;
			break;

		case WM8580_CLKSRC_OSC:
			reg |= WM8580_PLLB4_MCLKOUTSRC_OSC;
			break;

		default:
			return -EINVAL;
		}
		wm8580_write(codec, WM8580_PLLB4, reg);
		break;

	case WM8580_DAC_CLKSEL:
		reg = wm8580_read(codec, WM8580_CLKSEL);
		reg &= ~WM8580_CLKSEL_DAC_CLKSEL_MASK;

		switch (dir) {
		case WM8580_CLKSRC_MCLK:
			break;

		case WM8580_CLKSRC_PLLA:
			reg |= WM8580_CLKSEL_DAC_CLKSEL_PLLA;
			break;

		case WM8580_CLKSRC_PLLB:
			reg |= WM8580_CLKSEL_DAC_CLKSEL_PLLB;
			break;

		default:
			return -EINVAL;
		}
		wm8580_write(codec, WM8580_CLKSEL, reg);
		break;

	case WM8580_CLKOUTSRC:
		reg = wm8580_read(codec, WM8580_PLLB4);
		reg &= ~WM8580_PLLB4_CLKOUTSRC_MASK;

		switch (dir) {
		case WM8580_CLKSRC_NONE:
			break;

		case WM8580_CLKSRC_PLLA:
			reg |= WM8580_PLLB4_CLKOUTSRC_PLLACLK;
			break;

		case WM8580_CLKSRC_PLLB:
			reg |= WM8580_PLLB4_CLKOUTSRC_PLLBCLK;
			break;

		case WM8580_CLKSRC_OSC:
			reg |= WM8580_PLLB4_CLKOUTSRC_OSCCLK;
			break;

		default:
			return -EINVAL;
		}
		wm8580_write(codec, WM8580_PLLB4, reg);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8580_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	if (mute)
		wm8580_set_bits(codec, WM8580_DAC_CONTROL5, WM8580_MUTEALL);
	else
		wm8580_clear_bits(codec, WM8580_DAC_CONTROL5, WM8580_MUTEALL);
	return 0;

}

static int wm8580_dapm_event(struct snd_soc_codec *codec, int event)
{
	u16 reg;

	switch (event) {
	case SNDRV_CTL_POWER_D0:	/* full On */
		/* set vmid to 10k and current to 1.0x */
		/* Make VMID high impedence */
		reg = wm8580_read(codec, WM8580_ADC_CONTROL1);
		wm8580_write(codec, WM8580_ADC_CONTROL1, reg | 0x100);

		break;
	case SNDRV_CTL_POWER_D1:	/* partial On */
	case SNDRV_CTL_POWER_D2:	/* partial On */
		/* set vmid to 40k for quick power up */
		/*if (codec->dapm_state == SNDRV_CTL_POWER_D3hot) {

		   } */
		break;
	case SNDRV_CTL_POWER_D3hot:	/* Off, with power */
		/* Power up and get individual control of the DACs */
		reg = wm8580_read(codec, WM8580_PWRDN1);
		wm8580_write(codec, WM8580_PWRDN1,
			     reg & ~(WM8580_PWRDN1_PWDN |
				     WM8580_PWRDN1_ALLDACPD));
		/* Make VMID high impedence */
		reg = wm8580_read(codec, WM8580_ADC_CONTROL1);
		wm8580_write(codec, WM8580_ADC_CONTROL1, reg & ~0x100);
		break;
	case SNDRV_CTL_POWER_D3cold:	/* Off, without power */
		/* Power off */
		wm8580_set_bits(codec, WM8580_PWRDN1, WM8580_PWRDN1_PWDN);

		break;
	}

	codec->dapm_state = event;
	return 0;
}

#define WM8580_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_pcm_stream wm8580_dai_playback = {
	.stream_name = "Playback",
	.channels_min = 1,
	.channels_max = 6,
	.rates = SNDRV_PCM_RATE_8000_192000,
	.formats = WM8580_FORMATS,
};

static const struct snd_soc_pcm_stream wm8580_dai_capture = {
	.stream_name = "Capture",
	.channels_min = 1,
	.channels_max = 2,
	.rates = SNDRV_PCM_RATE_8000_192000,
	.formats = WM8580_FORMATS,
};

/* dai ops, called by machine drivers */
static const struct snd_soc_dai_ops wm8580_dai_ops = {
	.digital_mute = wm8580_mute,
	.set_fmt = wm8580_set_paif_dai_fmt,
	.set_sysclk = wm8580_set_paif_dai_sysclk,
	.set_tristate = wm8580_set_paif_dai_tristate,
	.set_pll = wm8580_set_dai_pll,
	.set_tdm_slot = wm8580_set_paif_dai_tdm_slot,
	.set_clkdiv = wm8580_set_dai_clkdiv,
};

/* audio ops, called by alsa */
static const struct snd_soc_ops wm8580_dai_audio_ops = {
	.hw_params = wm8580_paif_hw_params,
};

static int wm8580_suspend(struct device *dev, pm_message_t state)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);

	wm8580_dapm_event(codec, SNDRV_CTL_POWER_D3cold);
	return 0;
}

static int wm8580_resume(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);
	int i;

	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(wm8580_reg); i++) {
		if (i == WM8580_RESET)
			continue;
		wm8580_write(codec, i, *((u16 *) codec->reg_cache + i));
	}

	wm8580_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	/* charge wm8580 caps */
	if (codec->suspend_dapm_state == SNDRV_CTL_POWER_D0) {
		wm8580_dapm_event(codec, SNDRV_CTL_POWER_D3hot);
		codec->dapm_state = SNDRV_CTL_POWER_D0;
	}

	return 0;
}

static int wm8580_codec_io_probe(struct snd_soc_codec *codec,
				 struct snd_soc_machine *machine)
{

	/* Get the codec into a known state */
	wm8580_write(codec, WM8580_RESET, 0);

	/* charge output caps */
	codec->dapm_state = SNDRV_CTL_POWER_D3cold;
	wm8580_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	wm8580_add_controls(codec, machine->card);
	wm8580_add_widgets(codec, machine);

	/* read OUT1 & OUT2 volumes */
	/* need to backup volumes of all channels and write
	   last volume value again???? */

	/* set the update bits */
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACL1,
			WM8580_DA_UPDATE);
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACR1,
			WM8580_DA_UPDATE);
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACL2,
			WM8580_DA_UPDATE);
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACR2,
			WM8580_DA_UPDATE);
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACL3,
			WM8580_DA_UPDATE);
	wm8580_set_bits(codec, WM8580_DIGITAL_ATTENUATION_DACR3,
			WM8580_DA_UPDATE);

	return 0;
}

static int wm8580_codec_io_remove(struct snd_soc_codec *codec,
				  struct snd_soc_machine *machine)
{
	wm8580_dapm_event(codec, SNDRV_CTL_POWER_D3cold);
	return 0;
}

static const struct snd_soc_codec_ops wm8580_codec_ops = {
	.dapm_event = wm8580_dapm_event,
	.read = wm8580_read,
	.write = wm8580_write,
	.io_probe = wm8580_codec_io_probe,
	.io_remove = wm8580_codec_io_remove,
};

#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)

static struct spi_device *wm8580_spi_device;

 /*!
  * This function is called to transfer data to WM8580 on SPI.
  *
  * @param    spi        the SPI slave device(WM8580)
  * @param    buf        the pointer to the data buffer
  * @param    len        the length of the data to be transferred
  *
  * @return   Returns 0 on success -1 on failure.
  */
static inline int spi_rw(void *control_data, long data, int length)
{
	struct spi_transfer t = {
		.tx_buf = (const void *)data,
		.rx_buf = (void *)data,
		.len = length,
		.cs_change = 0,
		.delay_usecs = 0,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	if (spi_sync((struct spi_device *)control_data, &m) != 0
	    || m.status != 0)
		return 0;

	return (length - m.actual_length);
}

/*!
 * This function is called whenever the SPI slave device is detected.
 *
 * @param       spi     the SPI slave device
 *
 * @return      Returns 0 on SUCCESS and error on FAILURE.
 */
static int __devinit wm8580_spi_probe(struct spi_device *spi)
{
	spi->bits_per_word = 16;
	spi->mode = SPI_MODE_2;

	spi_setup(spi);

	wm8580_spi_device = spi;
	return 0;
}

static int __devinit wm8580_spi_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver wm8580_spi_driver = {
	.driver = {
		   .name = "wm8580_spi",
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
	.probe = wm8580_spi_probe,
	.remove = __devexit_p(wm8580_spi_remove),
};
#endif

static int wm8580_codec_probe(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);
	u16 data;

	info("WM8580 Audio Codec %s", WM8580_VERSION);

	codec->private_data = wm8580;

	codec->owner = THIS_MODULE;
	codec->ops = &wm8580_codec_ops;
	codec->reg_cache_size = ARRAY_SIZE(wm8580_reg);
	codec->reg_cache = kmemdup(wm8580_reg, sizeof(wm8580_reg), GFP_KERNEL);

	if (codec->reg_cache == NULL)
		return -ENOMEM;

#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
	codec->control_data = wm8580_spi_device;
	codec->mach_write = (hw_write_t) spi_rw;
	codec->mach_read = (hw_read_t) spi_rw;
#else
	/*Add I2C read/write here */
#endif

	snd_soc_register_codec(codec);

	/*Get WM8580 device revision number by reading register 2 */
	wm8580_write(codec, WM8580_READBACK, 0x10);
	data = 2 << WM8580_REG_DATA_LENGTH;
	codec->mach_read(codec->control_data, (long)&data, 1);
	info("device rev:%d", data & 0xff);

	return 0;

}

static int wm8580_codec_remove(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);
	kfree(codec->private_data);
	return 0;
}

static int wm8580_dai_probe(struct device *dev)
{
	struct snd_soc_dai *dai = to_snd_soc_dai(dev);

	dai->ops = &wm8580_dai_ops;
	dai->audio_ops = &wm8580_dai_audio_ops;
	dai->capture = &wm8580_dai_capture;
	dai->playback = &wm8580_dai_playback;
	snd_soc_register_codec_dai(dai);
	return 0;
}

const char wm8580_codec[SND_SOC_CODEC_NAME_SIZE] = "wm8580-codec";
EXPORT_SYMBOL_GPL(wm8580_codec);

static struct snd_soc_device_driver wm8580_codec_driver = {
	.type = SND_SOC_BUS_TYPE_CODEC,
	.driver = {
		   .name = wm8580_codec,
		   .owner = THIS_MODULE,
		   .bus = &asoc_bus_type,
		   .probe = wm8580_codec_probe,
		   .remove = __devexit_p(wm8580_codec_remove),
		   .suspend = wm8580_suspend,
		   .resume = wm8580_resume,
		   },
};

const char wm8580_dai[SND_SOC_CODEC_NAME_SIZE] = "wm8580-dai";
EXPORT_SYMBOL_GPL(wm8580_dai);

static struct snd_soc_device_driver wm8580_dai_driver = {
	.type = SND_SOC_BUS_TYPE_DAI,
	.driver = {
		   .name = wm8580_dai,
		   .owner = THIS_MODULE,
		   .bus = &asoc_bus_type,
		   .probe = wm8580_dai_probe,
		   },
};

static __init int wm8580_init(void)
{
	int ret = 0;

	dbg("Registering the WM8580 protocal Driver\n");
#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
	ret = spi_register_driver(&wm8580_spi_driver);
	if (ret < 0)
		return ret;
#endif
	ret = driver_register(&wm8580_codec_driver.driver);
	if (ret < 0) {
#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
		spi_unregister_driver(&wm8580_spi_driver);
#endif
		return ret;
	}
	ret = driver_register(&wm8580_dai_driver.driver);
	if (ret < 0) {
		driver_unregister(&wm8580_codec_driver.driver);
#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
		spi_unregister_driver(&wm8580_spi_driver);
#endif
		return ret;
	}
	wm8580 = kzalloc(sizeof(struct wm8580), GFP_KERNEL);
	if (wm8580 == NULL) {
		driver_unregister(&wm8580_dai_driver.driver);
		driver_unregister(&wm8580_codec_driver.driver);
#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
		spi_unregister_driver(&wm8580_spi_driver);
#endif
		return -ENOMEM;
	}
	return ret;
}

static __exit void wm8580_exit(void)
{
	driver_unregister(&wm8580_dai_driver.driver);
	driver_unregister(&wm8580_codec_driver.driver);
#if defined(CONFIG_SPI) || defined(CONFIG_SPI_MODULE)
	spi_unregister_driver(&wm8580_spi_driver);
#endif
}

module_init(wm8580_init);
module_exit(wm8580_exit);

MODULE_DESCRIPTION("ASoC WM8580 driver");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
