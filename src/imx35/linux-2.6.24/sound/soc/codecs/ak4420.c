/*
 * ak4420.c
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

#define AUDIO_NAME "AK4420"
#define AK4420_VERSION "0.1"

#define AK4420_RATES (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
		      SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

static const struct snd_soc_pcm_stream ak4420_dai_playback = {
	.stream_name = "Playback",
	.channels_min = 2,
	.channels_max = 2,
	.rates = AK4420_RATES,
	.formats = SNDRV_PCM_FMTBIT_S24_LE,
};

/* dai ops, called by machine drivers */
static const struct snd_soc_dai_ops ak4420_dai_ops = {
};

/* audio ops, called by alsa */
static const struct snd_soc_ops ak4420_dai_audio_ops = {
};

static const struct snd_soc_codec_ops ak4420_codec_ops = {
};

static int ak4420_codec_probe(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);

	printk(KERN_INFO "AK4420 Audio Codec %s", AK4420_VERSION);

	codec->owner = THIS_MODULE;
	codec->ops = &ak4420_codec_ops;

	snd_soc_register_codec(codec);

	return 0;

}

static int ak4420_codec_remove(struct device *dev)
{
	struct snd_soc_codec *codec = to_snd_soc_codec(dev);
	kfree(codec->private_data);
	return 0;
}

static int ak4420_dai_probe(struct device *dev)
{
	struct snd_soc_dai *dai = to_snd_soc_dai(dev);

	dai->playback = &ak4420_dai_playback;
	snd_soc_register_codec_dai(dai);
	return 0;
}

const char ak4420_codec[SND_SOC_CODEC_NAME_SIZE] = "ak4420-codec";
EXPORT_SYMBOL_GPL(ak4420_codec);

static struct snd_soc_device_driver ak4420_codec_driver = {
	.type = SND_SOC_BUS_TYPE_CODEC,
	.driver = {
		   .name = ak4420_codec,
		   .owner = THIS_MODULE,
		   .bus = &asoc_bus_type,
		   .probe = ak4420_codec_probe,
		   .remove = __devexit_p(ak4420_codec_remove),
		   },
};

const char ak4420_dai[SND_SOC_CODEC_NAME_SIZE] = "ak4420-dai";
EXPORT_SYMBOL_GPL(ak4420_dai);

static struct snd_soc_device_driver ak4420_dai_driver = {
	.type = SND_SOC_BUS_TYPE_DAI,
	.driver = {
		   .name = ak4420_dai,
		   .owner = THIS_MODULE,
		   .bus = &asoc_bus_type,
		   .probe = ak4420_dai_probe,
		   },
};

static __init int ak4420_init(void)
{
	int ret = 0;

	ret = driver_register(&ak4420_codec_driver.driver);
	if (ret < 0) {
		return ret;
	}

	ret = driver_register(&ak4420_dai_driver.driver);
	if (ret < 0) {
		driver_unregister(&ak4420_codec_driver.driver);
		return ret;
	}

	return ret;
}

static __exit void ak4420_exit(void)
{
	driver_unregister(&ak4420_dai_driver.driver);
	driver_unregister(&ak4420_codec_driver.driver);
}

module_init(ak4420_init);
module_exit(ak4420_exit);

MODULE_DESCRIPTION("ASoC AK4420 driver");
MODULE_AUTHOR("Richard Titmuss");
MODULE_LICENSE("GPL");
