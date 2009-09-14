/*
 * fab4-wm8974.c
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
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
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

#include "../codecs/wm8974.h"

void gpio_activate_audio_ports(int ssi_port);
extern const char wm8974_codec[SND_SOC_CODEC_NAME_SIZE];
extern const char wm8974_hifi_dai[SND_SOC_CODEC_NAME_SIZE];


static void fab4_init_dam(int ssi_port, int dai_port)
{
	/* WM8974 uses SSI1 or SSI2 via AUDMUX port dai_port for audio */

	/* reset port ssi_port & dai_port */
	DAM_PTCR(ssi_port) = 0;
	DAM_PDCR(ssi_port) = 0;
	DAM_PTCR(dai_port) = 0;
	DAM_PDCR(dai_port) = 0;

	/* set to synchronous */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_SYN;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_SYN;

	/* set Rx sources ssi_port <--> dai_port */
	DAM_PDCR(ssi_port) |= AUDMUX_PDCR_RXDSEL(dai_port);
	DAM_PDCR(dai_port) |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, dai_port);

	/* set Tx Clock direction and source dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, dai_port);
}

static int fab4_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int fab4_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct mxc_audio_platform_data *dev_data = pcm_link->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	struct snd_soc_dai *codec_dai = pcm_link->codec_dai;
	unsigned int rate = params_rate(params);
	unsigned int channels = params_channels(params);
	unsigned int mclkdiv;
	u32 dai_format;

	fab4_init_dam(dev_data->src_port, dev_data->ext_port);

	switch (rate) {
	case 8000:
		mclkdiv = WM8974_MCLKDIV_12;
		break;
	case 11025:
		mclkdiv = WM8974_MCLKDIV_8;
		break;
	case 16000:
		mclkdiv = WM8974_MCLKDIV_6;
		break;
	case 22050:
		mclkdiv = WM8974_MCLKDIV_4;
		break;
	case 32000:
		mclkdiv = WM8974_MCLKDIV_3;
		break;
	case 44100:
		mclkdiv = WM8974_MCLKDIV_2;
		break;
	case 48000:
		mclkdiv = WM8974_MCLKDIV_2;
		break;
	case 88200:
		mclkdiv = WM8974_MCLKDIV_1;
		break;
	case 96000:
		mclkdiv = WM8974_MCLKDIV_1;
		break;
	default:
		return -EINVAL;
	}
	
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;
	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	codec_dai->ops->set_fmt(codec_dai, dai_format);

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set i.MX active slot mask */
	cpu_dai->ops->set_tdm_slot(cpu_dai, 0xfffffffe, 2);

	/* set the SSI system clock as input (unused) */
	cpu_dai->ops->set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	/* set codec MCLK and BCLK division for sample rate */
	if (dev_data->audio_set_clk) {
		(dev_data->audio_set_clk)(rate);
	}
	codec_dai->ops->set_clkdiv(codec_dai, WM8974_BCLKDIV, WM8974_BCLKDIV_4);
	codec_dai->ops->set_clkdiv(codec_dai, WM8974_MCLKDIV, mclkdiv);

	return 0;
}

static void fab4_shutdown(struct snd_pcm_substream *substream)
{
}

/*
 * fab4 WM8974 DAI operations.
 */
static struct snd_soc_ops fab4_hifi_ops = {
	.startup = fab4_startup,
	.shutdown = fab4_shutdown,
	.hw_params = fab4_hifi_hw_params,
};

static int fab4_pcm_new(struct snd_soc_pcm_link *pcm_link)
{
	int ret;

	pcm_link->audio_ops = &fab4_hifi_ops;

	ret = snd_soc_pcm_new(pcm_link, 1, 1);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to create hifi pcm\n", __func__);
		return ret;
	}

	printk(KERN_INFO "fab4 WM8974 Audio Driver\n");

	return 0;
}

static int fab4_pcm_free(struct snd_soc_pcm_link *pcm_link)
{
	return 0;
}

static const struct snd_soc_pcm_link_ops fab4_pcm_ops = {
	.new = fab4_pcm_new,
	.free = fab4_pcm_free,
};

/* fab4 machine dapm widgets */
static const struct snd_soc_dapm_widget fab4_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

/* fab4 machine audio map */
static const char *audio_map[][3] = {
	/* speaker connected to codec pins SPKOUTP, SPKOUTN */
	{"Speaker", NULL, "SPKOUTP" },
	{"Speaker", NULL, "SPKOUTN" },

	/* mic connected to codec pins MICP, MICN */
	{"Mic", NULL, "MICP" },
	{"Mic", NULL, "MICN" },

	{ NULL, NULL, NULL },
};

static int mach_probe(struct snd_soc_machine *machine)
{
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;

	int i, ret;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	codec = pcm_link->codec;

	codec->ops->io_probe(codec, machine);

	/* add fab4 specific widgets */
	for (i = 0; i < ARRAY_SIZE(fab4_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec,
					 &fab4_dapm_widgets[i]);
	}

	/* set up fab4 specific audio path audio map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);
	}

	/* disable unused WM8974 codec pins */
	snd_soc_dapm_set_endpoint(machine, "AUX", 0);
	snd_soc_dapm_set_endpoint(machine, "MONOOUT", 0);

	/* connect and enable all speaker and mic */
	snd_soc_dapm_set_endpoint(machine, "Speaker", 1);
	snd_soc_dapm_set_endpoint(machine, "Mic", 1);

	snd_soc_dapm_set_policy(machine, SND_SOC_DAPM_POLICY_PATH);
	snd_soc_dapm_sync_endpoints(machine);

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

static int __devinit fab4_wm8974_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_machine *machine;
	struct mxc_audio_platform_data *dev_data = pdev->dev.platform_data;
	struct snd_soc_pcm_link *hifi;
	const char *ssi_port;
	int ret;

	machine = kzalloc(sizeof(struct snd_soc_machine), GFP_KERNEL);
	if (machine == NULL)
		return -ENOMEM;

	machine->owner = THIS_MODULE;
	machine->pdev = pdev;
	machine->name = "fab4";
	machine->longname = "wm8974";
	machine->ops = &machine_ops;
	pdev->dev.driver_data = machine;

	/* register card */
	ret =
	    snd_soc_new_card(machine, 1, SNDRV_DEFAULT_IDX1,
			     SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to create pcms\n", __func__);
		return ret;
	}

	/* WM8974 hifi interface */
	ret = -ENODEV;

	if (dev_data->src_port == 1)
		ssi_port = imx_ssi_1;
	else
		ssi_port = imx_ssi_3;

	hifi = snd_soc_pcm_link_new(machine, "fab4-wm8974",
				    &fab4_pcm_ops, imx_pcm,
				    wm8974_codec, wm8974_hifi_dai,
				    ssi_port);
	if (hifi == NULL) {
		printk(KERN_ERR "failed to create PCM link\n");
		snd_soc_machine_free(machine);
		return ret;
	}

	ret = snd_soc_pcm_link_attach(hifi);
	hifi->private_data = dev_data;
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to attach hifi pcm\n", __func__);
		snd_soc_machine_free(machine);
		return ret;
	}

	gpio_activate_audio_ports(dev_data->src_port);

	return ret;
}

static int __devexit fab4_wm8974_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	codec = pcm_link->codec;
	codec->ops->io_remove(codec, machine);

	snd_soc_machine_free(machine);
	return 0;
}

static struct platform_driver fab4_wm8974_audio_driver = {
	.probe = fab4_wm8974_audio_probe,
	.remove = __devexit_p(fab4_wm8974_audio_remove),
	.driver = {
		   .name = "fab4-wm8974",
		   .owner = THIS_MODULE,
		   },
};

static int __init fab4_wm8974_audio_init(void)
{
	return platform_driver_register(&fab4_wm8974_audio_driver);
}

static void __exit fab4_wm8974_audio_exit(void)
{
	platform_driver_unregister(&fab4_wm8974_audio_driver);
}

module_init(fab4_wm8974_audio_init);
module_exit(fab4_wm8974_audio_exit);

MODULE_DESCRIPTION("ALSA SoC wm8974 fab4");
MODULE_AUTHOR("Richard Titmuss");
MODULE_LICENSE("GPL");
