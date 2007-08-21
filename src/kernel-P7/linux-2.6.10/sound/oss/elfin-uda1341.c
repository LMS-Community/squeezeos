/*
 * Glue audio driver for the SA1111 compagnon chip & Philips UDA1341 codec.
 *
 * Copyright (c) 2000 John Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2000-09-04	John Dorsey	SA-1111 Serial Audio Controller support
 * 				was initially added to the sa1100-uda1341.c
 * 				driver.
 *
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-uda1341.c driver.
 *
 * 2001-09-23	Russell King	Remove old L3 bus driver.
 *
 * 2001-12-19	Nicolas Pitre	Moved SA1111 SAC support to this file where
 * 				it actually belongs (formerly in dma-sa1111.c
 * 				from John Dorsey).
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/l3/l3.h>
#include <linux/l3/uda1341.h>

#include <asm/semaphore.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/dma.h>
#include <asm/io.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-clock.h>
#include <asm/hardware/clock.h>
#include "elfin-audio.h"

//#undef DEBUG
//#define DEBUG
#ifdef DEBUG
//#define DPRINTK( x... )  printk( ##x )
#define DPRINTK  printk
#else
#define DPRINTK( x... )
#endif




#define S_CLOCK_FREQ    384

static long audio_rate=44100;



static struct uda1341 *uda1341;
static struct clk	*iis_clock;


static DECLARE_MUTEX(s3c2413_lock);

static struct l3_adapter l3_elfin_adapter = {
	.owner	= THIS_MODULE,
	.name	= "l3-s3c2413",
	.lock	= &s3c2413_lock,
};


static int
mixer_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	return uda1341_mixer_ctl(uda1341, cmd, (void *)arg);
}

static struct file_operations uda1341_mixer_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= mixer_ioctl,
};



static int iispsr_value(int sample_rate)
{
	int i;
	unsigned long fact0 = clk_get_rate(iis_clock)/S_CLOCK_FREQ;;


	unsigned long r0_sample_rate, r1_sample_rate = 0, r2_sample_rate;
	int prescaler = 0;

	DPRINTK("requested sample_rate = %d\n", sample_rate);

	for(i = 1; i < 32; i++) {
		r1_sample_rate = fact0 / i;
		if(r1_sample_rate < sample_rate)
			break;
	}

	r0_sample_rate = fact0 / (i + 1);
	r2_sample_rate = fact0 / (i - 1);

	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i + 1, r0_sample_rate, abs(r0_sample_rate - sample_rate));
	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i, r1_sample_rate, abs(r1_sample_rate - sample_rate));
	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i - 1, r2_sample_rate, abs(r2_sample_rate - sample_rate));

	prescaler = i;
	if(abs(r0_sample_rate - sample_rate) <
	   abs(r1_sample_rate - sample_rate))
		prescaler = i + 1;
	if(abs(r2_sample_rate - sample_rate) <
	   abs(r1_sample_rate - sample_rate))
		prescaler = i - 1;

	prescaler = max_t(int, 0, (prescaler - 1));

	DPRINTK("selected prescale value = %d, freq = %ld, error = %d\n",
	       prescaler, fact0 / (prescaler + 1),
	       abs((fact0 / (prescaler + 1)) - sample_rate));

	return prescaler;
}

static long audio_set_dsp_speed(long val)
{
	unsigned long tmp;
	int prescaler = 0;

	tmp =  clk_get_rate(iis_clock)/S_CLOCK_FREQ; 
	 
	DPRINTK("requested = %ld, limit = %ld\n", val, tmp);
	if(val > (tmp >> 1))
		return -1;
	prescaler = iispsr_value(val);
	tmp  = (prescaler << 8) | 1 << 15 ; /*prescale enable*/
	writel(tmp,S3C2413_IISPSR);
	audio_rate = val;
	DPRINTK("return audio_rate = %ld\n", (unsigned long)audio_rate);

	return audio_rate;
}



static int
elfin_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	audio_state_t *state = file->private_data;
	//struct sa1111_dev *sadev = state->data;
	long val = 0;
	int ret = 0;

	switch (cmd) {
	case SNDCTL_DSP_STEREO:
		ret = get_user(val, (int *) arg);
		if (ret)
			break;
		/* the UDA1341 is stereo only */
		ret = (val == 0) ? -EINVAL : 1;
		ret = put_user(ret, (int *) arg);
		break;

	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* the UDA1341 is stereo only */
		ret = put_user(2, (long *) arg);
		break;

	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret)
			break;
 		audio_set_dsp_speed( val);
		break;
		/* fall through */

	case SOUND_PCM_READ_RATE:
		ret = put_user(val, (long *) arg); /* very ugly hack - Alex Lange (chicken@handhelds.org) */
		break;

	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:
		/* we can do 16-bit only */
		ret = put_user(AFMT_S16_LE, (long *) arg);
		break;

	default:
		/* Maybe this is meant for the mixer (as per OSS Docs) */
		ret = mixer_ioctl(inode, file, cmd, arg);
		break;
	}

	return ret;


}

static void elfin_audio_shutdown(void *dummy)
{

	uda1341_close(uda1341);
	/* Disable the I2S clock and L3 bus clock */
	clk_disable(iis_clock);
	clk_unuse(iis_clock);
	clk_put(iis_clock);


}

void elfin_init_iis(void)
{
	u32 val =0;
	
	val = S3C2413_IISCON_TXDMACTIVE | S3C2413_IISCON_RXDMACTIVE ;
	writel(val,S3C2413_IISCON);  
	
	val =0;
	val =  S3C2413_IISMOD_IMS_INTERNAL_MASTER |S3C2413_IISMOD_TXRXMODE |S3C2413_IISMOD_IIS |S3C2413_IISMOD_384FS |  
			S3C2413_IISMOD_32FS | S3C2413_IISMOD_16BIT;
 	writel(val,S3C2413_IISMOD);

	val = 0 ;
	val =  (1<<15) ;	  
	writel(val,S3C2413_IISFIC);
	val =  (0<<15) ;	  
	writel(val,S3C2413_IISFIC);
	writel(readl(S3C2413_IISCON)| 0x1,S3C2413_IISCON);

}







//shaju...i dont know whats happening here..the document doesnt have these bits..copying the firmware code
static void elfin_select_clkdiv(void)
{

	writel((readl(S3C2413_CLKCON)| (1 << 26)| (1<<13)),S3C2413_CLKCON);
	//writel((readl(S3C2413_CLKSRC)| (1 <<9)),S3C2413_CLKSRC);
	writel((readl(S3C2413_CLKDIVN) & ~(0xf << 12)),S3C2413_CLKDIVN);
    //IIS Pre-scaler Setting
	writel((readl(S3C2413_IISPSR)&0),S3C2413_IISPSR);
	

// writel((readl(S3C2413_IISPSR)|(1<<15) |(5<<8)) ,S3C2413_IISPSR);



}


static void elfin_iis_config_pins(void)
{
 	s3c2413_gpio_cfgpin(S3C2413_GPE0, S3C2413_GPE0_I2SLRCK);
    s3c2413_gpio_cfgpin(S3C2413_GPE1, S3C2413_GPE1_I2SSCLK);
    s3c2413_gpio_cfgpin(S3C2413_GPE3, S3C2413_GPE3_I2SSDI);
    s3c2413_gpio_cfgpin(S3C2413_GPE4, S3C2413_GPE4_I2SSDO);

}


static int elfin_select_clk(void)
{
		iis_clock = clk_get(NULL, "iis");
		if (!iis_clock) {
			printk("failed to get iis clock source\n");
			return -ENOENT;
		}
	clk_use(iis_clock);
	clk_enable(iis_clock);
	 elfin_select_clkdiv();
	return 0;
}





static void elfin_audio_init(void *dummy)
{
/*shaju IIS pins can be configured here */

	elfin_iis_config_pins();
	
	DPRINTK("iis config\n");

	elfin_select_clk();
	DPRINTK("iis select clk\n");
	elfin_init_iis();	
	DPRINTK("iis init iis\n");

	uda1341_open(uda1341);
}

static audio_stream_t output_stream = {
	id:		"s3c2413 audio out ",
	dma:	2,
};
static audio_stream_t input_stream = {
	id:		"s3c2413 audio in ",
	dma:	3,
};

static audio_state_t audio_state = {
	.output_stream	= &output_stream,
	.input_stream	= &input_stream,
	.hw_init	= elfin_audio_init,
	.hw_shutdown	= elfin_audio_shutdown,
	.client_ioctl	= elfin_audio_ioctl,
	.sem		= __MUTEX_INITIALIZER(audio_state.sem),
};



static int elfin_audio_open(struct inode *inode, struct file *file)
{
        return elfin_audio_attach(inode, file, &audio_state);
}



/*
 * Missing fields of this structure will be patched with the call
 * to elfin_audio_attach().
 */



static struct file_operations elfin_audio_fops = {
	.owner	= THIS_MODULE,
	.open	= elfin_audio_open,
};


static int audio_dev_id, mixer_dev_id;

static int elfin_audio_probe(struct device *_dev)
{
/*shaju most imp..configure IIS pins for 2413*/
	struct uda1341_cfg cfg;
	int ret;
	ret = l3_add_adapter(&l3_elfin_adapter);
	if (ret)
		goto out;

	uda1341 = uda1341_attach("l3-bit-s3c2413-gpio");
	if (IS_ERR(uda1341)) {
		ret = PTR_ERR(uda1341);
		goto remove_l3;
	}
	cfg.fs     = 384;
	cfg.format = FMT_I2S;
	//uda1341_configure(uda1341, &cfg);


	audio_dev_id = register_sound_dsp(&elfin_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&uda1341_mixer_fops, -1);

	printk(KERN_INFO "Sound: elfin 2413 UDA1341: dsp id %d mixer id %d\n",
		audio_dev_id, mixer_dev_id);
	return 0;
remove_l3:
	uda1341_detach(uda1341);
	l3_del_adapter(&l3_elfin_adapter);

out:
	return ret;

}

static int elfin_audio_remove(void )
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	uda1341_detach(uda1341);
	l3_del_adapter(&l3_elfin_adapter);
	return 0;
}
#ifdef CONFIG_PM
static int smdk2413_audio_suspend( u32 state)
{
	return elfin_audio_suspend(&audio_state, state, SUSPEND_POWER_DOWN);
}

static int smdk2413_audio_resume()
{
	return elfin_audio_resume(&audio_state, RESUME_ENABLE);
}
#endif

static struct device_driver elfin_iis_driver = {
    .name       = "elfin-iis",
    .bus        = &platform_bus_type,
    .probe      = elfin_audio_probe,
    .remove     = elfin_audio_remove,
#ifdef CONFIG_PM
    .suspend    = smdk2413_audio_suspend,
    .resume     = smdk2413_audio_resume,
#endif
};



int elfin_uda1341_init(void)
{
 return driver_register(&elfin_iis_driver);
}

static void elfin_uda1341_exit(void)
{
	driver_unregister(&elfin_iis_driver);

}


module_init(elfin_uda1341_init);
module_exit(elfin_uda1341_exit);

