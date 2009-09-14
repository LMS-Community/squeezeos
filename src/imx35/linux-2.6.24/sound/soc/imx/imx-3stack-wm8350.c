/*
 * imx-3stack-wm8350.c  --  i.MX 3Stack Driver for Wolfson WM8350 Codec
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *    19th Jun 2007   Initial version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regulator/wm8350/wm8350.h>
#include <linux/regulator/wm8350/wm8350-audio.h>
#include <linux/regulator/wm8350/wm8350-bus.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc.h>

#include "imx-ssi.h"
#include "imx-pcm.h"

void gpio_activate_audio_ports(void);

/* SSI BCLK and LRC master */
#define WM8350_SSI_MASTER	1

struct imx_3stack_pcm_state {
	int lr_clk_active;
	int playback_active;
	int capture_active;
};

struct _wm8350_audio {
	unsigned int channels;
	snd_pcm_format_t format;
	unsigned int rate;
	unsigned int sysclk;
	unsigned int bclkdiv;
	unsigned int clkdiv;
	unsigned int lr_rate;
};

/* in order of power consumption per rate (lowest first) */
static const struct _wm8350_audio wm8350_audio[] = {
	/* 16bit mono modes */

	{1, SNDRV_PCM_FORMAT_S16_LE, 8000, 12288000,
	WM8350_BCLK_DIV_48, WM8350_DACDIV_6, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 16000, 12288000,
	 WM8350_BCLK_DIV_24, WM8350_DACDIV_6, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 32000, 12288000,
	 WM8350_BCLK_DIV_12, WM8350_DACDIV_3, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 48000, 12288000,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_2, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 96000, 24576000,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_2, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 11025, 11289600,
	 WM8350_BCLK_DIV_32, WM8350_DACDIV_4, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 22050, 11289600,
	 WM8350_BCLK_DIV_16, WM8350_DACDIV_4, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 44100, 11289600,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_2, 32,},
	{1, SNDRV_PCM_FORMAT_S16_LE, 88200, 22579200,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_2, 32,},

	/* 16 bit stereo modes */
	{2, SNDRV_PCM_FORMAT_S16_LE, 8000, 12288000,
	 WM8350_BCLK_DIV_48, WM8350_DACDIV_6, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 16000, 12288000,
	 WM8350_BCLK_DIV_24, WM8350_DACDIV_3, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 32000, 12288000,
	 WM8350_BCLK_DIV_12, WM8350_DACDIV_1_5, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 48000, 12288000,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_1, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 96000, 24576000,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_1, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 11025, 11289600,
	 WM8350_BCLK_DIV_32, WM8350_DACDIV_4, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 22050, 11289600,
	 WM8350_BCLK_DIV_16, WM8350_DACDIV_2, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 44100, 11289600,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_1, 32,},
	{2, SNDRV_PCM_FORMAT_S16_LE, 88200, 22579200,
	 WM8350_BCLK_DIV_8, WM8350_DACDIV_1, 32,},

	/* 24bit stereo modes */
	{2, SNDRV_PCM_FORMAT_S24_LE, 48000, 12288000,
	 WM8350_BCLK_DIV_4, WM8350_DACDIV_1, 64,},
	{2, SNDRV_PCM_FORMAT_S24_LE, 96000, 24576000,
	 WM8350_BCLK_DIV_4, WM8350_DACDIV_1, 64,},
	{2, SNDRV_PCM_FORMAT_S24_LE, 44100, 11289600,
	 WM8350_BCLK_DIV_4, WM8350_DACDIV_1, 64,},
	{2, SNDRV_PCM_FORMAT_S24_LE, 88200, 22579200,
	 WM8350_BCLK_DIV_4, WM8350_DACDIV_1, 64,},
};

#if WM8350_SSI_MASTER
static int imx_3stack_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_codec *codec = pcm_link->codec;
	struct wm8350 *wm8350 = codec->control_data;
	struct imx_3stack_pcm_state *state = pcm_link->private_data;

	/* In master mode the LR clock can come from either the DAC or ADC.
	 * We use the LR clock from whatever stream is enabled first.
	 */

	if (!state->lr_clk_active) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			wm8350_clear_bits(wm8350, WM8350_CLOCK_CONTROL_2,
					  WM8350_LRC_ADC_SEL);
		else
			wm8350_set_bits(wm8350, WM8350_CLOCK_CONTROL_2,
					WM8350_LRC_ADC_SEL);
	}
	state->lr_clk_active++;
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		state->capture_active = 1;
	else
		state->playback_active = 1;
	return 0;
}
#else
#define imx_3stack_startup NULL
#endif

static int imx_3stack_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	struct snd_soc_dai *codec_dai = pcm_link->codec_dai;
	struct imx_3stack_pcm_state *state = pcm_link->private_data;
	int i, found = 0;
	snd_pcm_format_t format = params_format(params);
	unsigned int rate = params_rate(params);
	unsigned int channels = params_channels(params);
	u32 dai_format;

	/* only need to do this once as capture and playback are sync */
	if (state->lr_clk_active > 1)
		return 0;

	/* find the correct audio parameters */
	for (i = 0; i < ARRAY_SIZE(wm8350_audio); i++) {
		if (rate == wm8350_audio[i].rate &&
		    format == wm8350_audio[i].format &&
		    channels == wm8350_audio[i].channels) {
			found = 1;
			break;
		}
	}
	if (!found) {
		printk(KERN_ERR "%s: invalid params\n", __func__);
		return -EINVAL;
	}

#if WM8350_SSI_MASTER
	/* codec FLL input is 32768 kHz from MCLK */
	codec_dai->ops->set_pll(codec_dai, 0, 32768, wm8350_audio[i].sysclk);
#else
	/* codec FLL input is rate from DAC LRC */
	codec_dai->ops->set_pll(codec_dai, 0, rate, wm8350_audio[i].sysclk);
#endif

#if WM8350_SSI_MASTER
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;
	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	codec_dai->ops->set_fmt(codec_dai, dai_format);

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set 32KHZ as the codec system clock for DAC and ADC */
	codec_dai->ops->set_sysclk(codec_dai, WM8350_MCLK_SEL_PLL_32K,
				   wm8350_audio[i].sysclk, SND_SOC_CLOCK_IN);

#else
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_SYNC;
	if (channels == 2)
		format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	codec_dai->ops->set_fmt(codec_dai, dai_format);

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set DAC LRC as the codec system clock for DAC and ADC */
	codec_dai->ops->set_sysclk(codec_dai, WM8350_MCLK_SEL_PLL_DAC,
				   wm8350_audio[i].sysclk, SND_SOC_CLOCK_IN);
#endif

	/* set i.MX active slot mask */
	cpu_dai->ops->set_tdm_slot(cpu_dai,
				   channels == 1 ? 0xfffffffe : 0xfffffffc,
				   channels);

	/* set the SSI system clock as input (unused) */
	cpu_dai->ops->set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	/* set codec BCLK division for sample rate */
	codec_dai->ops->set_clkdiv(codec_dai, WM8350_BCLK_CLKDIV,
				   wm8350_audio[i].bclkdiv);

	/* DAI is synchronous and clocked with DAC LRCLK & ADC LRC */
	codec_dai->ops->set_clkdiv(codec_dai,
				   WM8350_DACLR_CLKDIV,
				   wm8350_audio[i].lr_rate);
	codec_dai->ops->set_clkdiv(codec_dai,
				   WM8350_ADCLR_CLKDIV,
				   wm8350_audio[i].lr_rate);

	/* now configure DAC and ADC clocks */
	codec_dai->ops->set_clkdiv(codec_dai,
				   WM8350_DAC_CLKDIV, wm8350_audio[i].clkdiv);

	codec_dai->ops->set_clkdiv(codec_dai,
				   WM8350_ADC_CLKDIV, wm8350_audio[i].clkdiv);

	return 0;
}

static void imx_3stack_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_dai *codec_dai = pcm_link->codec_dai;
	struct snd_soc_codec *codec = pcm_link->codec;
	struct imx_3stack_pcm_state *state = pcm_link->private_data;
	struct wm8350 *wm8350 = codec->control_data;

	/* disable the PLL if there are no active Tx or Rx channels */
	if (!codec_dai->active)
		codec_dai->ops->set_pll(codec_dai, 0, 0, 0);
	state->lr_clk_active--;

	/*
	 * We need to keep track of active streams in master mode and
	 * switch LRC source if necessary.
	 */

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		state->capture_active = 0;
	else
		state->playback_active = 0;

	if (state->capture_active)
		wm8350_set_bits(wm8350, WM8350_CLOCK_CONTROL_2,
				WM8350_LRC_ADC_SEL);
	else if (state->playback_active)
		wm8350_clear_bits(wm8350, WM8350_CLOCK_CONTROL_2,
				  WM8350_LRC_ADC_SEL);
}

/*
 * imx_3stack WM8350 HiFi DAI opserations.
 */
static struct snd_soc_ops imx_3stack_hifi_ops = {
	.startup = imx_3stack_startup,
	.shutdown = imx_3stack_shutdown,
	.hw_params = imx_3stack_hifi_hw_params,
};

/* need to refine these */
static struct wm8350_audio_platform_data imx_3stack_wm8350_setup = {
	.vmid_discharge_msecs = 1000,
	.drain_msecs = 30,
	.cap_discharge_msecs = 700,
	.vmid_charge_msecs = 700,
	.vmid_s_curve = WM8350_S_CURVE_SLOW,
	.dis_out4 = WM8350_DISCHARGE_SLOW,
	.dis_out3 = WM8350_DISCHARGE_SLOW,
	.dis_out2 = WM8350_DISCHARGE_SLOW,
	.dis_out1 = WM8350_DISCHARGE_SLOW,
	.vroi_out4 = WM8350_TIE_OFF_500R,
	.vroi_out3 = WM8350_TIE_OFF_500R,
	.vroi_out2 = WM8350_TIE_OFF_500R,
	.vroi_out1 = WM8350_TIE_OFF_500R,
	.vroi_enable = 0,
	.codec_current_d0 = WM8350_CODEC_ISEL_1_0,
	.codec_current_d3 = WM8350_CODEC_ISEL_0_5,
	.codec_current_charge = WM8350_CODEC_ISEL_1_5,
};

static void imx_3stack_init_dam(int ssi_port, int dai_port)
{
	/* WM8350 uses SSI1 or SSI2 via AUDMUX port dai_port for audio */

	/* reset port ssi_port & dai_port */
	DAM_PTCR(ssi_port) = 0;
	DAM_PDCR(ssi_port) = 0;
	DAM_PTCR(dai_port) = 0;
	DAM_PDCR(dai_port) = 0;

	/* set to synchronous */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_SYN;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_SYN;

#if WM8350_SSI_MASTER
	/* set Rx sources ssi_port <--> dai_port */
	DAM_PDCR(ssi_port) |= AUDMUX_PDCR_RXDSEL(dai_port);
	DAM_PDCR(dai_port) |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, dai_port);

	/* set Tx Clock direction and source dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, dai_port);
#else
	/* set Rx sources ssi_port <--> dai_port */
	DAM_PDCR(ssi_port) |= AUDMUX_PDCR_RXDSEL(dai_port);
	DAM_PDCR(dai_port) |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  ssi_port --> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, ssi_port);

	/* set Tx Clock direction and source ssi_port--> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, ssi_port);
#endif

}
static int imx_3stack_pcm_new(struct snd_soc_pcm_link *pcm_link)
{
	struct imx_3stack_pcm_state *state;
	int ret;

	state = kzalloc(sizeof(struct imx_3stack_pcm_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	pcm_link->audio_ops = &imx_3stack_hifi_ops;
	pcm_link->private_data = state;

	ret = snd_soc_pcm_new(pcm_link, 1, 1);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to create hifi pcm\n", __func__);
		kfree(state);
		return ret;
	}

	printk(KERN_INFO "i.MX 3STACK WM8350 Audio Driver");

	return 0;
}

static int imx_3stack_pcm_free(struct snd_soc_pcm_link *pcm_link)
{
	kfree(pcm_link->private_data);
	return 0;
}

static const struct snd_soc_pcm_link_ops imx_3stack_pcm_ops = {
	.new = imx_3stack_pcm_new,
	.free = imx_3stack_pcm_free,
};

/* imx_3stack machine dapm widgets */
static const struct snd_soc_dapm_widget imx_3stack_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("SiMIC", NULL),
	SND_SOC_DAPM_MIC("Mic1 Jack", NULL),
	SND_SOC_DAPM_MIC("Mic2 Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out Jack", NULL),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
};

/* imx_3stack machine audio map */
static const char *audio_map[][3] = {

	/* SiMIC --> IN1LN (with automatic bias) via SP1 */
	{"IN1RP", NULL, "Mic Bias"},
	{"Mic Bias", NULL, "SiMIC"},

	/* Mic 1 Jack --> IN1LN and IN1LP (with automatic bias) */
	{"IN1LN", NULL, "Mic Bias"},
	{"IN1LP", NULL, "Mic1 Jack"},
	{"Mic Bias", NULL, "Mic1 Jack"},

	/* Mic 2 Jack --> IN1RN and IN1RP (with automatic bias) */
	{"IN1RN", NULL, "Mic2 Jack"},
	{"IN1RP", NULL, "Mic Bias"},
	{"Mic Bias", NULL, "Mic2 Jack"},

	/* Line in Jack --> AUX (L+R) */
	{"IN3R", NULL, "Line In Jack"},
	{"IN3L", NULL, "Line In Jack"},

	/* Out1 --> Headphone Jack */
	{"Headphone Jack", NULL, "OUT1R"},
	{"Headphone Jack", NULL, "OUT1L"},

	/* Out1 --> Line Out Jack */
	{"Line Out Jack", NULL, "OUT2R"},
	{"Line Out Jack", NULL, "OUT2L"},

	{NULL, NULL, NULL},
};

#ifdef CONFIG_PM
static int imx_3stack_wm8350_audio_suspend(struct platform_device *dev,
					   pm_message_t state)
{

	int ret = 0;

	return ret;
}

static int imx_3stack_wm8350_audio_resume(struct platform_device *dev)
{

	int ret = 0;

	return ret;
}

#else
#define imx_3stack_wm8350_audio_suspend	NULL
#define imx_3stack_wm8350_audio_resume	NULL
#endif

static void imx_3stack_jack_handler(struct wm8350 *wm8350, int irq, void *data)
{
	struct snd_soc_machine *machine =
	    (struct snd_soc_machine *)&wm8350->audio;
	u16 reg;

	/* debounce for 200ms */
	schedule_timeout_interruptible(msecs_to_jiffies(200));
	reg = wm8350_reg_read(wm8350, WM8350_JACK_PIN_STATUS);
	if (reg & WM8350_JACK_R_LVL) {
		snd_soc_dapm_set_endpoint(machine, "Line Out Jack", 0);
		snd_soc_dapm_set_endpoint(machine, "Headphone Jack", 1);
	} else {
		snd_soc_dapm_set_endpoint(machine, "Headphone Jack", 0);
		snd_soc_dapm_set_endpoint(machine, "Line Out Jack", 1);
	}
	snd_soc_dapm_sync_endpoints(machine);
}

int mach_probe(struct snd_soc_machine *machine)
{
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;
	struct wm8350 *wm8350 = to_wm8350_from_audio(machine);

	int i, ret;
	u16 reg;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	codec = pcm_link->codec;
	codec->control_data = wm8350;
	codec->platform_data = &imx_3stack_wm8350_setup;
	codec->ops->io_probe(codec, machine);

	/* set unused imx_3stack WM8350 codec pins */
	snd_soc_dapm_set_endpoint(machine, "OUT3", 0);
	snd_soc_dapm_set_endpoint(machine, "OUT4", 0);
	snd_soc_dapm_set_endpoint(machine, "IN2R", 0);
	snd_soc_dapm_set_endpoint(machine, "IN2L", 0);
	snd_soc_dapm_set_endpoint(machine, "OUT2L", 0);
	snd_soc_dapm_set_endpoint(machine, "OUT2R", 0);

	/* Add imx_3stack specific widgets */
	for (i = 0; i < ARRAY_SIZE(imx_3stack_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec,
					 &imx_3stack_dapm_widgets[i]);
	}

	/* set up imx_3stack specific audio path audio map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);
	}

	/* connect and enable all imx_3stack WM8350 jacks (for now) */
	snd_soc_dapm_set_endpoint(machine, "SiMIC", 1);
	snd_soc_dapm_set_endpoint(machine, "Mic1 Jack", 1);
	snd_soc_dapm_set_endpoint(machine, "Mic2 Jack", 1);
	snd_soc_dapm_set_endpoint(machine, "Line In Jack", 1);

	snd_soc_dapm_set_policy(machine, SND_SOC_DAPM_POLICY_STREAM);
	snd_soc_dapm_sync_endpoints(machine);

	/* enable slow clock gen for jack detect */
	reg = wm8350_reg_read(wm8350, WM8350_POWER_MGMT_4);
	wm8350_reg_write(wm8350, WM8350_POWER_MGMT_4, reg | WM8350_TOCLK_ENA);
	/* enable jack detect */
	reg = wm8350_reg_read(wm8350, WM8350_JACK_DETECT);
	wm8350_reg_write(wm8350, WM8350_JACK_DETECT, reg | WM8350_JDR_ENA);
	wm8350_register_irq(wm8350, WM8350_IRQ_CODEC_JCK_DET_R,
			    imx_3stack_jack_handler, NULL);
	wm8350_unmask_irq(wm8350, WM8350_IRQ_CODEC_JCK_DET_R);

	/* register card with ALSA upper layers */
	ret = snd_soc_register_card(machine);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to register sound card\n",
		       __FUNCTION__);
		snd_soc_machine_free(machine);
		return ret;
	}
	return 0;

}

struct snd_soc_machine_ops machine_ops = {
	.mach_probe = mach_probe,
};

static int __devinit imx_3stack_wm8350_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = platform_get_drvdata(pdev);
	struct snd_soc_pcm_link *hifi;
	int ret;
	struct mxc_audio_platform_data *tmp;

	machine->owner = THIS_MODULE;
	machine->pdev = pdev;
	machine->name = "i.MX_3STACK";
	machine->longname = "WM8350";
	machine->ops = &machine_ops;

	/* register card */
	ret =
	    snd_soc_new_card(machine, 1, SNDRV_DEFAULT_IDX1,
			     SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to create pcms\n", __func__);
		return ret;
	}

	/* WM8350 hifi interface */
	ret = -ENODEV;
	tmp = pdev->dev.platform_data;

	if (tmp->src_port == 2)
		hifi = snd_soc_pcm_link_new(machine, "imx_3stack-hifi",
					    &imx_3stack_pcm_ops, imx_pcm,
					    wm8350_codec, wm8350_hifi_dai,
					    imx_ssi_3);
	else
		hifi = snd_soc_pcm_link_new(machine, "imx_3stack-hifi",
					    &imx_3stack_pcm_ops, imx_pcm,
					    wm8350_codec, wm8350_hifi_dai,
					    imx_ssi_1);
	if (hifi == NULL) {
		printk("failed to create HiFi PCM link\n");
		snd_soc_machine_free(machine);
		return ret;
	}
	ret = snd_soc_pcm_link_attach(hifi);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to attach hifi pcm\n", __func__);
		snd_soc_machine_free(machine);
		return ret;
	}
	gpio_activate_audio_ports();
	imx_3stack_init_dam(tmp->src_port, tmp->ext_port);

	return ret;

}

static int __devexit
imx_3stack_wm8350_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = platform_get_drvdata(pdev);
	struct wm8350 *wm8350 = to_wm8350_from_audio(machine);
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	codec = pcm_link->codec;
	codec->ops->io_remove(codec, machine);
	wm8350_mask_irq(wm8350, WM8350_IRQ_CODEC_JCK_DET_R);
	wm8350_free_irq(wm8350, WM8350_IRQ_CODEC_JCK_DET_R);

	snd_soc_machine_free(machine);
	return 0;
}

const char imx_3stack_audio[32] = {
	"wm8350-imx-3stack-audio"
};

/* EXPORT_SYMBOL_GPL(imx_3stack_audio); */

static struct platform_driver imx_3stack_wm8350_audio_driver = {
	.probe = imx_3stack_wm8350_audio_probe,
	.remove = __devexit_p(imx_3stack_wm8350_audio_remove),
	.suspend = imx_3stack_wm8350_audio_suspend,
	.resume = imx_3stack_wm8350_audio_resume,
	.driver = {
		   .name = imx_3stack_audio,
		   },
};

static int __init imx_3stack_wm8350_audio_init(void)
{
	return platform_driver_register(&imx_3stack_wm8350_audio_driver);
}

static void __exit imx_3stack_wm8350_audio_exit(void)
{
	platform_driver_unregister(&imx_3stack_wm8350_audio_driver);
}

module_init(imx_3stack_wm8350_audio_init);
module_exit(imx_3stack_wm8350_audio_exit);

MODULE_AUTHOR("Liam Girdwood");
MODULE_DESCRIPTION("PMIC WM8350 Driver for i.MX 3STACK");
MODULE_LICENSE("GPL");
