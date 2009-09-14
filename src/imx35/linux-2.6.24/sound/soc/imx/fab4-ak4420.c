/*
 * fab4-ak4420.c
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
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc.h>
#include <asm/arch/gpio.h>

#include "imx-pcm.h"
#include "imx-ssi.h"


extern void gpio_activate_audio_ports(int ssi_port);
extern const char ak4420_codec[SND_SOC_CODEC_NAME_SIZE];
extern const char ak4420_dai[SND_SOC_CODEC_NAME_SIZE];


static void fab4_init_dam(int ssi_port, int dai_port)
{
	/* AK4420 uses SSI1 or SSI2 via AUDMUX port dai_port for audio */

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

	/* set Tx frame direction and source  ssi_port --> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, ssi_port);

	/* set Tx Clock direction and source ssi_port--> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, ssi_port);
}

static int fab4_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct mxc_audio_platform_data *dev_data = pcm_link->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	unsigned int channels = params_channels(params);
	unsigned int rate = params_rate(params);
	u32 dai_format;

	fab4_init_dam(dev_data->src_port, dev_data->ext_port);

	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_IF |
	    SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_SYNC;

	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set i.MX active slot mask */
	cpu_dai->ops->set_tdm_slot(cpu_dai,
				   channels == 1 ? 0xfffffffe : 0xfffffffc,
				   channels);

	/* set the SSI system clock as input (unused) */
	cpu_dai->ops->set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	/* set BCLK division for sample rate */
	switch (rate) {
	case 96000:
	case 88200:
		cpu_dai->ops->set_clkdiv(cpu_dai, IMX_SSI_TX_DIV_PM, 0);
		break;
	default:
		cpu_dai->ops->set_clkdiv(cpu_dai, IMX_SSI_TX_DIV_PM, 1);
		break;
	}

	if (dev_data->audio_set_clk) {
		(dev_data->audio_set_clk)(rate);
	}

	return 0;
}

static int fab4_hifi_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_machine *machine = pcm_link->machine;
	struct resource	*smute, *hdp_en;

	smute = platform_get_resource_byname(machine->pdev, IORESOURCE_MEM, "smute");
	if (smute) {
		mxc_set_gpio_dataout(smute->start, 0); /* unmute */
	}

	hdp_en = platform_get_resource_byname(machine->pdev, IORESOURCE_MEM, "hdp_en");
	if (hdp_en) {
		mxc_set_gpio_dataout(hdp_en->start, 1); /* headphone amp on */
	}

	return 0;
}

static void fab4_hifi_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_machine *machine = pcm_link->machine;
	struct resource	*smute;

	smute = platform_get_resource_byname(machine->pdev, IORESOURCE_MEM, "smute");
	mxc_set_gpio_dataout(smute->start, 1); /* mute */
}

/*
 * fab4 ak4420 HiFi DAI opserations.
 */
static struct snd_soc_ops fab4_hifi_ops = {
	.hw_params = fab4_hifi_hw_params,
	.prepare = fab4_hifi_prepare,
	.shutdown = fab4_hifi_shutdown,
};

static int hifi_pcm_new(struct snd_soc_pcm_link *pcm_link)
{
	int ret;
	pcm_link->audio_ops = &fab4_hifi_ops;
	ret = snd_soc_pcm_new(pcm_link, 1, 0);
	if (ret < 0) {
		pr_err("%s: Failed to create hifi pcm\n", __func__);
		return ret;
	}

	return 0;
}

struct snd_soc_pcm_link_ops hifi_pcm = {
	.new = hifi_pcm_new,
};

static int fab4_mach_probe(struct snd_soc_machine
				 *machine)
{
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;
	int ret;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);
	codec = pcm_link->codec;

	snd_soc_dapm_set_policy(machine, SND_SOC_DAPM_POLICY_STREAM);

	/* register card with ALSA upper layers */
	ret = snd_soc_register_card(machine);
	if (ret < 0) {
		pr_err("%s: failed to register sound card\n", __func__);
		return ret;
	}

	return 0;
}

struct snd_soc_machine_ops fab4_mach_ops = {
	.mach_probe = fab4_mach_probe,
};

static ssize_t show_headphone(struct device_driver *dev, char *buf)
{
#if 0
	unsigned int value;

	if (value == 0)
		strcpy(buf, "speaker\n");
	else
		strcpy(buf, "headphone\n");
#endif
	strcpy(buf, "fixme\n");

	return strlen(buf);
}

DRIVER_ATTR(headphone, S_IRUGO | S_IWUSR, show_headphone, NULL);

/*
 * This function will register the snd_soc_pcm_link drivers.
 * It also registers devices for platform DMA, I2S, SSP and registers an
 * I2C driver to probe the codec.
 */
static int __init fab4_ak4420_probe(struct platform_device *pdev)
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
	machine->longname = "ak4420";
	machine->ops = &fab4_mach_ops;
	pdev->dev.driver_data = machine;

	/* register card */
	ret = snd_soc_new_card(machine, 1, SNDRV_DEFAULT_IDX1,
			       SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		pr_err("%s: failed to create stereo sound card\n", __func__);
		goto err;
	}

	/* fab4 ak4420 hifi interface */
	if (dev_data->src_port == 1)
		ssi_port = imx_ssi_1;
	else
		ssi_port = imx_ssi_3;
	hifi = snd_soc_pcm_link_new(machine,
				    "fab4-ak4420", &hifi_pcm,
				    imx_pcm, ak4420_codec,
				    ak4420_dai, ssi_port);
	if (hifi == NULL) {
		pr_err("Failed to create HiFi PCM link\n");
		goto err;
	}

	ret = snd_soc_pcm_link_attach(hifi);
	if (ret < 0)
		goto link_err;

	hifi->private_data = dev_data;

	/* Configure audio port 3 */
	gpio_activate_audio_ports(dev_data->src_port);

	return ret;

      link_err:
	snd_soc_machine_free(machine);
      err:
	kfree(machine);
	return ret;
}

static int __devexit fab4_ak4420_remove(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;

	kfree(machine);
	return 0;
}

#ifdef CONFIG_PM
static int fab4_ak4420_suspend(struct platform_device
				     *pdev, pm_message_t state)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;
	return snd_soc_suspend(machine, state);
}

static int fab4_ak4420_resume(struct platform_device
				    *pdev)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;
	return snd_soc_resume(machine);
}

#else
#define fab4_ak4420_suspend NULL
#define fab4_ak4420_resume  NULL
#endif

static struct platform_driver fab4_ak4420_driver = {
	.probe = fab4_ak4420_probe,
	.remove = __devexit_p(fab4_ak4420_remove),
	.suspend = fab4_ak4420_suspend,
	.resume = fab4_ak4420_resume,
	.driver = {
		   .name = "fab4-ak4420",
		   .owner = THIS_MODULE,
		   },
};

static int __init fab4_asoc_init(void)
{
	return platform_driver_register(&fab4_ak4420_driver);
}

static void __exit fab4_asoc_exit(void)
{
	platform_driver_unregister(&fab4_ak4420_driver);
}

module_init(fab4_asoc_init);
module_exit(fab4_asoc_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC ak4420 fab4");
MODULE_AUTHOR("Richard Titmuss");
MODULE_LICENSE("GPL");
