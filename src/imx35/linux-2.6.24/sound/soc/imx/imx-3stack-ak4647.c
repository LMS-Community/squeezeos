/*
 * imx-3stack-ak4647.c  --  SoC audio for imx_3stack
 *
 * Copyright 2008 Freescale  Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc.h>
#include <asm/arch/pmic_external.h>

#include "imx-pcm.h"
#include "imx-ssi.h"

#define AK4647_SSI_MASTER	1

#define AK4647_DEFAULT	0
#define AK4647_HP	1
#define AK4647_MIC	2
#define AK4647_LINE_IN	3
#define AK4647_LINE_OUT	4
#define AK4647_OFF	5
#define AK4647_SPK_OFF	0
#define AK4647_SPK_ON	1

extern void gpio_activate_audio_ports(void);
extern const char ak4647_codec[SND_SOC_CODEC_NAME_SIZE];
extern const char ak4647_hifi_dai[SND_SOC_CODEC_NAME_SIZE];

static void headphone_detect_handler(struct work_struct *work);
static DECLARE_WORK(hp_event, headphone_detect_handler);
static struct snd_soc_machine *imx_3stack_mach;
static int ak4647_jack_func;
static int ak4647_spk_func;

static void ak4647_ext_control(void)
{
	int spk = 0, mic = 0, hp = 0, line_in = 0, line_out = 0;

	/* set up jack connection */
	switch (ak4647_jack_func) {
	case AK4647_DEFAULT:
		hp = 1;
		mic = 1;
	case AK4647_HP:
		hp = 1;
		break;
	case AK4647_MIC:
		mic = 1;
		break;
	case AK4647_LINE_IN:
		line_in = 1;
		break;
	case AK4647_LINE_OUT:
		line_out = 1;
		break;
	}

	if (ak4647_spk_func == AK4647_SPK_ON)
		spk = 1;

	snd_soc_dapm_set_endpoint(imx_3stack_mach, "Mic1 Jack", mic);
	snd_soc_dapm_set_endpoint(imx_3stack_mach, "Line In Jack", line_in);
	snd_soc_dapm_set_endpoint(imx_3stack_mach, "Line Out Jack", line_out);
	snd_soc_dapm_set_endpoint(imx_3stack_mach, "Headphone Jack", hp);
	snd_soc_dapm_set_endpoint(imx_3stack_mach, "Ext Spk", spk);
	snd_soc_dapm_sync_endpoints(imx_3stack_mach);
}

static void imx_3stack_init_dam(int ssi_port, int dai_port)
{
	/* AK4647 uses SSI1 or SSI2 via AUDMUX port dai_port for audio */

	/* reset port ssi_port & dai_port */
	DAM_PTCR(ssi_port) = 0;
	DAM_PDCR(ssi_port) = 0;
	DAM_PTCR(dai_port) = 0;
	DAM_PDCR(dai_port) = 0;

	/* set to synchronous */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_SYN;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_SYN;

#if AK4647_SSI_MASTER
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

static int imx_3stack_hifi_startup(struct snd_pcm_substream *substream)
{
	/* check the jack status at stream startup */
	ak4647_ext_control();
	return 0;
}

static int imx_3stack_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct mxc_audio_platform_data *dev_data = pcm_link->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	struct snd_soc_dai *codec_dai = pcm_link->codec_dai;
	unsigned int channels = params_channels(params);
	unsigned int rate = params_rate(params);
	u32 dai_format;

	imx_3stack_init_dam(dev_data->src_port, dev_data->ext_port);

#if AK4647_SSI_MASTER
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;
#else
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_SYNC;
#endif
	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	codec_dai->ops->set_fmt(codec_dai, dai_format);

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set i.MX active slot mask */
	cpu_dai->ops->set_tdm_slot(cpu_dai,
				   channels == 1 ? 0xfffffffe : 0xfffffffc,
				   channels);

	/* set the SSI system clock as input (unused) */
	cpu_dai->ops->set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	codec_dai->ops->set_sysclk(codec_dai, 0, rate, 0);

	/* set codec BCLK division for sample rate */
	codec_dai->ops->set_clkdiv(codec_dai, 0, 0);

	return 0;
}

/*
 * imx_3stack ak4647 HiFi DAI opserations.
 */
static struct snd_soc_ops imx_3stack_hifi_ops = {
	.startup = imx_3stack_hifi_startup,
	.hw_params = imx_3stack_hifi_hw_params,
};

static int hifi_pcm_new(struct snd_soc_pcm_link *pcm_link)
{
	int ret;
	pcm_link->audio_ops = &imx_3stack_hifi_ops;
	ret = snd_soc_pcm_new(pcm_link, 1, 1);
	if (ret < 0) {
		pr_err("%s: Failed to create hifi pcm\n", __func__);
		return ret;
	}

	return 0;
}

struct snd_soc_pcm_link_ops hifi_pcm = {
	.new = hifi_pcm_new,
};

static int ak4647_get_jack(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4647_jack_func;
	return 0;
}

static int ak4647_set_jack(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	if (ak4647_jack_func == ucontrol->value.integer.value[0])
		return 0;

	ak4647_jack_func = ucontrol->value.integer.value[0];
	ak4647_ext_control();
	return 1;
}

static int ak4647_get_spk(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4647_spk_func;
	return 0;
}

static int ak4647_set_spk(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	if (ak4647_spk_func == ucontrol->value.integer.value[0])
		return 0;

	ak4647_spk_func = ucontrol->value.integer.value[0];
	ak4647_ext_control();
	return 1;
}

static int spk_amp_event(struct snd_soc_dapm_widget *w, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event))
		pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 0, 1);
	else
		pmic_gpio_set_bit_val(MCU_GPIO_REG_GPIO_CONTROL_1, 0, 0);
	return 0;
}

/* imx_3stack machine dapm widgets */
static const struct snd_soc_dapm_widget imx_3stack_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic1 Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", spk_amp_event),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
};

/* example machine audio_mapnections */
static const char *audio_map[][3] = {

	/* mic is connected to mic1 - with bias */
	{"Left Input", NULL, "Mic1 Jack"},

	/* Line in jack */
	{"Left Input", NULL, "Line In Jack"},
	{"Right Input", NULL, "Line In Jack"},

	/* Headphone jack */
	{"Headphone Jack", NULL, "HPL"},
	{"Headphone Jack", NULL, "HPR"},

	/* Line out jack */
	{"Line Out Jack", NULL, "ROUT"},
	{"Line Out Jack", NULL, "LOUT"},

	/* Ext Spk */
	{"Ext Spk", NULL, "ROUT"},
	{"Ext Spk", NULL, "LOUT"},

	{NULL, NULL, NULL},

};

static const char *jack_function[] = { "Hp-Mic", "Headphone", "Mic", "Line In",
	"Line Out", "Off"
};

static const char *spk_function[] = { "Off", "On" };

static const struct soc_enum ak4647_enum[] = {
	SOC_ENUM_SINGLE_EXT(6, jack_function),
	SOC_ENUM_SINGLE_EXT(2, spk_function),
};

static const struct snd_kcontrol_new ak4647_machine_controls[] = {
	SOC_ENUM_EXT("Jack Function", ak4647_enum[0], ak4647_get_jack,
		     ak4647_set_jack),
	SOC_ENUM_EXT("Speaker Function", ak4647_enum[1], ak4647_get_spk,
		     ak4647_set_spk),
};

static void headphone_detect_handler(struct work_struct *work)
{
	sysfs_notify(&imx_3stack_mach->pdev->dev.driver->kobj, NULL,
		     "headphone");

}

static irqreturn_t imx_headphone_detect_handler(int irq, void *dev_id)
{
	schedule_work(&hp_event);
	return IRQ_HANDLED;

}

static int imx_3stack_mach_probe(struct snd_soc_machine
				 *machine)
{
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;
	int i, ret;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);
	codec = pcm_link->codec;

	codec->ops->io_probe(codec, machine);

	snd_soc_dapm_set_endpoint(machine, "RIN2", 0);

	/* Add imx_3stack specific widgets */
	for (i = 0; i < ARRAY_SIZE(imx_3stack_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec,
					 &imx_3stack_dapm_widgets[i]);
	}

	for (i = 0; i < ARRAY_SIZE(ak4647_machine_controls); i++) {
		ret = snd_ctl_add(machine->card,
				  snd_soc_cnew(&ak4647_machine_controls[i],
					       codec, NULL));
		if (ret < 0)
			return ret;
	}

	/* set up imx_3stack specific audio path audio_mapnects */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine,
					   audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);
	}

	snd_soc_dapm_set_policy(machine, SND_SOC_DAPM_POLICY_STREAM);

	/* register card with ALSA upper layers */
	ret = snd_soc_register_card(machine);
	if (ret < 0) {
		pr_err("%s: failed to register sound card\n", __func__);
		return ret;
	}

	return 0;
}

struct snd_soc_machine_ops imx_3stack_mach_ops = {
	.mach_probe = imx_3stack_mach_probe,
};

static ssize_t show_headphone(struct device_driver *dev, char *buf)
{

	unsigned int value;

	pmic_gpio_get_designation_bit_val(0, &value);

	if (value == 0)
		strcpy(buf, "speaker\n");
	else
		strcpy(buf, "headphone\n");

	return strlen(buf);
}

DRIVER_ATTR(headphone, S_IRUGO | S_IWUSR, show_headphone, NULL);

/*
 * This function will register the snd_soc_pcm_link drivers.
 * It also registers devices for platform DMA, I2S, SSP and registers an
 * I2C driver to probe the codec.
 */
static int __init imx_3stack_ak4647_probe(struct platform_device *pdev)
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
	machine->name = "imx_3stack";
	machine->longname = "ak4647";
	machine->ops = &imx_3stack_mach_ops;
	pdev->dev.driver_data = machine;

	/* register card */
	imx_3stack_mach = machine;
	ret =
	    snd_soc_new_card(machine, 1, SNDRV_DEFAULT_IDX1,
			     SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		pr_err("%s: failed to create stereo sound card\n", __func__);
		goto err;
	}

	pmic_gpio_set_bit_val(MCU_GPIO_REG_RESET_2, 1, 0);
	msleep(1);
	pmic_gpio_set_bit_val(MCU_GPIO_REG_RESET_2, 1, 1);

	/* imx_3stack ak4647 hifi interface */
	if (dev_data->src_port == 1)
		ssi_port = imx_ssi_1;
	else
		ssi_port = imx_ssi_3;
	hifi =
	    snd_soc_pcm_link_new(machine,
				 "imx_3stack-hifi", &hifi_pcm,
				 imx_pcm, ak4647_codec,
				 ak4647_hifi_dai, ssi_port);
	if (hifi == NULL) {
		pr_err("Failed to create HiFi PCM link\n");
		goto err;
	}
	ret = snd_soc_pcm_link_attach(hifi);
	hifi->private_data = dev_data;
	if (ret < 0)
		goto link_err;

	/* Configure audio port 3 */
	gpio_activate_audio_ports();

	if (request_irq
	    (dev_data->intr_id_hp, imx_headphone_detect_handler, 0,
	     "headphone", machine))
		goto link_err;

	ret = driver_create_file(pdev->dev.driver, &driver_attr_headphone);
	if (ret < 0)
		goto sysfs_err;

	return ret;

      sysfs_err:
	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);
      link_err:
	snd_soc_machine_free(machine);
      err:
	kfree(machine);
	return ret;
}

static int __devexit imx_3stack_ak4647_remove(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;

	imx_3stack_mach = NULL;
	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);
	kfree(machine);
	return 0;
}

#ifdef CONFIG_PM
static int imx_3stack_ak4647_suspend(struct platform_device
				     *pdev, pm_message_t state)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;
	return snd_soc_suspend(machine, state);
}

static int imx_3stack_ak4647_resume(struct platform_device
				    *pdev)
{
	struct snd_soc_machine *machine = pdev->dev.driver_data;
	return snd_soc_resume(machine);
}

#else
#define imx_3stack_ak4647_suspend NULL
#define imx_3stack_ak4647_resume  NULL
#endif

static struct platform_driver imx_3stack_ak4647_driver = {
	.probe = imx_3stack_ak4647_probe,
	.remove = __devexit_p(imx_3stack_ak4647_remove),
	.suspend = imx_3stack_ak4647_suspend,
	.resume = imx_3stack_ak4647_resume,
	.driver = {
		   .name = "imx-3stack-ak4647",
		   .owner = THIS_MODULE,
		   },
};

static int __init imx_3stack_asoc_init(void)
{
	return platform_driver_register(&imx_3stack_ak4647_driver);
}

static void __exit imx_3stack_asoc_exit(void)
{
	platform_driver_unregister(&imx_3stack_ak4647_driver);
}

module_init(imx_3stack_asoc_init);
module_exit(imx_3stack_asoc_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC ak4647 imx_3stack");
MODULE_LICENSE("GPL");
