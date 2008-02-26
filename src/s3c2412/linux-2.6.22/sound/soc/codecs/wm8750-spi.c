

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>

#include <linux/spi/spi.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "wm8750.h"

static struct snd_soc_device *wm8750_socdev;

struct wm8750_spi {
	struct snd_soc_device	*socdev;
	struct spi_device	*dev;
	struct spi_message	message;
	struct spi_transfer	transfer;
};

static int wm8750_spi_write(void *ctrl, const char *data, int size)
{
	struct wm8750_spi *spi = ctrl;

	spi_message_init(&spi->message);
	spi_message_add_tail(&spi->transfer, &spi->message);

	spi->transfer.tx_buf = data;
	spi->transfer.len = size;
	spi->transfer.bits_per_word = 8;

	return spi_sync(spi->dev, &spi->message);
}

static int __devinit wm8750_spi_probe(struct spi_device *spi)
{
	struct snd_soc_device *socdev = wm8750_socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct wm8750_spi *spidata;
	int ret;

	dev_info(&spi->dev, "probing device\n");

	spidata = kmalloc(sizeof(struct wm8750_spi), GFP_KERNEL);
	if (spidata == NULL) {
		dev_err(&spi->dev, "failed to allocate state\n");
		kfree(codec);
		return -ENOMEM;
	}

	spidata->dev = spi;
	spidata->socdev = socdev;
	codec->control_data = spi;

	dev_set_drvdata(&spi->dev, spi);

	ret = wm8750_init(socdev);
	if (ret < 0) {
		dev_err(&spi->dev, "failed to initialise codec\n");
		kfree(spi);
		kfree(codec);
		return ret;
	}

	dev_info(&spi->dev, "CODEC initialised\n");

	return 0;
}

static int __devexit wm8750_spi_remove(struct spi_device *spi)
{
	struct wm8750_spi *spidata = dev_get_drvdata(&spi->dev);
	struct snd_soc_codec *codec = spidata->socdev->codec;

	dev_info(&spi->dev, "removing device\n");

	kfree(codec->reg_cache);
	kfree(spi);

	return 0;
}

static struct spi_driver wm8750_spi_driver = {
	.driver = {
		.name		= "WM8750",
		.owner		= THIS_MODULE,
	},
	.probe		= wm8750_spi_probe,
	.remove		= __devexit_p(wm8750_spi_remove),
};


static int wm8750_probe_spi(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret;

	codec = wm8750_probe(pdev);
	if (codec == NULL) {
		dev_err(&pdev->dev, "No memory for CODEC device\n");
		return -ENODEV;
	}

	wm8750_socdev = socdev;

	codec->hw_write = wm8750_spi_write;

	ret = spi_register_driver(&wm8750_spi_driver);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register spi driver\n");
		return ret;
	} else {
	}

	return 0;
}

static int wm8750_remove_spi(struct platform_device *pdev)
{
	int ret;

	ret = wm8750_remove(pdev);
	if (ret)
		return ret;

	spi_unregister_driver(&wm8750_spi_driver);
	wm8750_free(pdev);

	return 0;
}


struct snd_soc_codec_device soc_codec_dev_wm8750_spi = {
	.probe = 	wm8750_probe_spi,
	.remove = 	wm8750_remove_spi,
	.suspend = 	wm8750_suspend,
	.resume =	wm8750_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_wm8750_spi);

MODULE_DESCRIPTION("ASoC WM8750 SPI access driver");
MODULE_AUTHOR("Ben Dooks");
MODULE_LICENSE("GPL");
