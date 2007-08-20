/*
 * Glue audio driver for the SA1111 compagnon chip & Philips WM8750 codec.
 *
 * Copyright (c) 2000 John Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2000-09-04	John Dorsey	SA-1111 Serial Audio Controller support
 * 				was initially added to the sa1100-wm8750.c
 * 				driver.
 *
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-wm8750.c driver.
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

#include <asm/semaphore.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/dma.h>
#include <asm/io.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/map.h>
#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-clock.h>
#include <asm/hardware/clock.h>

#include "elfin-audio.h"
#include <linux/l3/WM8750.h>

//#undef DEBUG
//#define DEBUG
#ifdef DEBUG
//#define DPRINTK( x... )  printk( ##x )
#define DPRINTK  printk
#else
#define DPRINTK( x... )
#endif

#define S_CLOCK_FREQ    384

static long audio_rate = 44100;

static struct clk *iis_clock;

static DECLARE_MUTEX(s3c2413_lock);

void WM8750_CODEC_Init(void);
void WM8750_SEND_CMD(unsigned char hb, unsigned char lb);

static void wm8750_power(void);
static void wm8750_dac_mute(unsigned char On_Off);
static void wm8750_out1_volume(void);
static void wm8750_out2_volume(void);


static unsigned int out1_volume = 0x0000; // mute
static unsigned int out2_volume = 0x5D5D; // level 93, maps to 0db


// mixer capabilities
#define REC_MASK 0
#define DEV_MASK (SOUND_MASK_VOLUME | SOUND_MASK_SPEAKER)


static int mixer_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	int val, nr = _IOC_NR(cmd), ret = 0;

	if (cmd == SOUND_MIXER_INFO) {
		struct mixer_info mi;

		strncpy(mi.id, "WM8750", sizeof(mi.id));
		strncpy(mi.name, "Wolfson WM8750", sizeof(mi.name));
		return copy_to_user((void *)arg, &mi, sizeof(mi)) ? -EFAULT : 0;
	}

	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		ret = get_user(val, (int *)arg);
		if (ret)
			goto out;

		switch (nr) {
		case SOUND_MIXER_VOLUME:
			out1_volume = val;
			if (val) {
				//wm8750_power();
				wm8750_out1_volume();
			}
			else {
				wm8750_out1_volume();
				//wm8750_power();
			}
			break;

		case SOUND_MIXER_SPEAKER:
			out2_volume = val;
			if (val) {
				//wm8750_power();
				wm8750_out2_volume();
			}
			else {
				wm8750_out2_volume();
				//wm8750_power();
			}
			break;

		default:
			ret = -EINVAL;
		}
	}

	if (ret == 0 && _IOC_DIR(cmd) & _IOC_READ) {
		int nr = _IOC_NR(cmd);
		ret = 0;

		switch (nr) {
		case SOUND_MIXER_VOLUME:     val = out1_volume;	break;
		case SOUND_MIXER_BASS:       val = 101;	break;
		case SOUND_MIXER_TREBLE:     val = 101;	break;
		case SOUND_MIXER_SPEAKER:    val = out2_volume;	break;
		case SOUND_MIXER_RECMASK:    val = REC_MASK;	break;
		case SOUND_MIXER_DEVMASK:    val = DEV_MASK;	break;
		case SOUND_MIXER_CAPS:       val = 0;		break;
		case SOUND_MIXER_STEREODEVS: val = 0;		break;
		default:	val = 0;     ret = -EINVAL;	break;
		}

		if (ret == 0)
			ret = put_user(val, (int *)arg);
	}
out:
	return ret;
}


static struct file_operations wm8750_mixer_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= mixer_ioctl,
};


static int iispsr_value(int sample_rate)
{
	int i;
	int prescaler = 0;
	unsigned long r0_sample_rate, r1_sample_rate = 0, r2_sample_rate;
	unsigned long fact0 = clk_get_rate(iis_clock)/S_CLOCK_FREQ;;
	
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


static int elfin_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	long val = 0;
	int ret = 0;

	switch (cmd) {
	case SNDCTL_DSP_STEREO:
		ret = get_user(val, (int *) arg);
		if (ret)
			break;
		/* the WM8750 is stereo only */
		ret = (val == 0) ? -EINVAL : 1;
		ret = put_user(ret, (int *) arg);
		break;

	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* the WM8750 is stereo only */
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
	/* Disable the I2S clock and L3 bus clock */
	clk_disable(iis_clock);
	clk_unuse(iis_clock);
	clk_put(iis_clock);

	/* power off codec */
	WM8750_SEND_CMD(WM8750_R25_PwrMgmt1_ADDR, 0);
	WM8750_SEND_CMD(WM8750_R26_PwrMgmt2_ADDR, 0);
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
	writel((readl(S3C2413_CLKDIVN) & ~(0xf << 12)),S3C2413_CLKDIVN);
    	//IIS Pre-scaler Setting
	writel((readl(S3C2413_IISPSR)&0),S3C2413_IISPSR);
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
	/* shaju IIS pins can be configured here */
	elfin_iis_config_pins();

	elfin_select_clk();
	elfin_init_iis();

	//SCLK -- GPB4
	s3c2413_gpio_cfgpin(S3C2413_GPB4, S3C2413_GPB4_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPB4, 0);
	s3c2413_gpio_setpin(S3C2413_GPB4, 0);

	//SDAT - GPB9
	s3c2413_gpio_cfgpin(S3C2413_GPB9, S3C2413_GPB9_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPB9, 0);
	s3c2413_gpio_setpin(S3C2413_GPB9, 0);

	//SCSB - GPH10
	s3c2413_gpio_cfgpin(S3C2413_GPH10, S3C2413_GPH10_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPH10, 0);
	s3c2413_gpio_setpin(S3C2413_GPH10, 0);

	WM8750_CODEC_Init();
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
	audio_dev_id = register_sound_dsp(&elfin_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&wm8750_mixer_fops, -1);

	printk(KERN_INFO "Sound: elfin 2413 WM8750: dsp id %d mixer id %d\n",
		audio_dev_id, mixer_dev_id);
	return 0;
}

static int elfin_audio_remove(void)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	return 0;
}

#ifdef CONFIG_PM
static int smdk2413_audio_suspend(u32 state)
{
	// FIXME wm8750 power management code needed
	return elfin_audio_suspend(&audio_state, state, SUSPEND_POWER_DOWN);
}

static int smdk2413_audio_resume(void)
{
	// FIXME wm8750 power management code needed
	return elfin_audio_resume(&audio_state, RESUME_ENABLE);
}
#endif

static struct device_driver elfin_iis_driver = {
    	.name       	= "elfin-iis",
    	.bus        	= &platform_bus_type,
    	.probe      	= elfin_audio_probe,
    	.remove     	= elfin_audio_remove,
#ifdef CONFIG_PM
    	.suspend    	= smdk2413_audio_suspend,
    	.resume     	= smdk2413_audio_resume,
#endif
};



int elfin_wm8750_init(void)
{
 	return driver_register(&elfin_iis_driver);
}

static void elfin_wm8750_exit(void)
{
	driver_unregister(&elfin_iis_driver);

}

//---------------------------------------------------------------------------------------------------------

#define SPI_SCSB(data)	s3c2413_gpio_setpin(S3C2413_GPH10, (data & 0x1))
#define SPI_SDA(data)	s3c2413_gpio_setpin(S3C2413_GPB9, (data & 0x1))
#define SPI_SCLK(data)	s3c2413_gpio_setpin(S3C2413_GPB4, (data & 0x1))


void WM8750_SEND_CMD(unsigned char hb, unsigned char lb) {
	char a;
	int data, temp;
	unsigned int mask;
	
	data = (int)hb;
	data =((data<<8)&0xff00)|(lb&0xff);

	mask=0x8000;
	for(a=0;a<16;a++)
	{
		temp = data&mask;
		if (temp) {
			SPI_SDA(1);
		}
		else {
			SPI_SDA(0);
		}
		udelay(1);
		SPI_SCLK(1);

		if (a==15) {
			udelay(1);
			SPI_SCSB(0);
		}

		udelay(1);
		SPI_SCLK(0);
		udelay(1);
		// maybe insert some delay!! to make 20nS

		mask = mask >> 1;
	}
	SPI_SDA(0);
	udelay(1);
	SPI_SCSB(1);
	udelay(5);
}


static void wm8750_power(void) {
	unsigned char r25h = 0, r25l = 0;
	unsigned char r26h = 0, r26l = 0;

	if (out1_volume || out2_volume) {
		r25h |= 0;
		r25l |= WM8750_R25_VMIDSEL_MASK_0; // 50KOHM playback
		r25l |= WM8750_R25_VREF_BIT;

		r26h |= WM8750_R26_DACL_BIT;
		r26l |= WM8750_R26_DACR_BIT;
	}

	if (out1_volume) {
		r26l |= WM8750_R26_LOUT1_BIT | WM8750_R26_ROUT1_BIT;
	}

	if (out2_volume) {
		r26l |= WM8750_R26_LOUT2_BIT | WM8750_R26_ROUT2_BIT;
	}

	WM8750_SEND_CMD(WM8750_R25_PwrMgmt1_ADDR | r25h, r25l);
	WM8750_SEND_CMD(WM8750_R26_PwrMgmt2_ADDR | r26h, r26l);
}


static void wm8750_out1_volume(void) {
	unsigned char r2h = 0, r2l = 0;
	unsigned char r3h = 0, r3l = 0;

	r2l = out1_volume;
	r3l = (out1_volume >> 8);

	// 0-100 -> 47-127
	r2l = (47 + ((r2l * 80) / 100)) & WM8750_R2_LOUT1VOL_MASK;
	r3l = (47 + ((r3l * 80) / 100)) & WM8750_R3_ROUT1VOL_MASK;
	printk("OUT1_VOLUME %x %x\n", r2l, r3l);

	r3h = WM8750_R3_RO1VU_BIT;

	WM8750_SEND_CMD(WM8750_R2_LOUT1Vol_ADDR | r2h, r2l);
	WM8750_SEND_CMD(WM8750_R3_ROUT1Vol_ADDR | r3h, r3l);
}


static void wm8750_out2_volume(void) {
	unsigned char r40h = 0, r40l = 0;
	unsigned char r41h = 0, r41l = 0;

	r40l = out2_volume;
	r41l = (out2_volume >> 8);

	// 0-100 -> 47-127
	r40l = (47 + ((r40l * 80) / 100)) & WM8750_R40_LOUT2VOL_MASK;
	r41l = (47 + ((r41l * 80) / 100)) & WM8750_R41_ROUT2VOL_MASK;
	printk("OUT2_VOLUME %x %x\n", r40l, r41l);

	r41h = WM8750_R41_RO2VU_BIT;

	WM8750_SEND_CMD(WM8750_R40_LOUT2Vol_ADDR | r40h, r40l);
	WM8750_SEND_CMD(WM8750_R41_ROUT2Vol_ADDR | r41h, r41l);
}


static void wm8750_dac_mute(unsigned char On_Off) {
	unsigned char r5 = 0;

	if (On_Off) {
		r5 |= WM8750_R5_DACMU_BIT;
	}

	WM8750_SEND_CMD(WM8750_R5_ADCDACCON_ADDR, r5);
}


void WM8750_CODEC_Init(void)
{
	WM8750_SEND_CMD(WM8750_R15_Reset_ADDR,0x00);		// reset

	WM8750_SEND_CMD(WM8750_R0_LeftInVol_ADDR,0x97);		// Odb INPUT
	WM8750_SEND_CMD(WM8750_R1_RightInVol_ADDR,0x97);	// Odb INPUT
	
	WM8750_SEND_CMD(WM8750_R7_AudioIF_ADDR,0x02);		// I^2S,16bit,slave mode
	WM8750_SEND_CMD(WM8750_R8_SampleRate_ADDR,0x22);	// 44.1 kHz
	WM8750_SEND_CMD(WM8750_R12_BASSCon_ADDR,0x0f);		// bass control disabled
	WM8750_SEND_CMD(WM8750_R13_TrebleCon_ADDR,0x0f);	// treble control disabled
	WM8750_SEND_CMD(WM8750_R16_3DCon_ADDR,0x00);		// 3d enhancement disabled
	
	WM8750_SEND_CMD(WM8750_R17_ALC1_ADDR,0x7b);
	WM8750_SEND_CMD(WM8750_R18_ALC2_ADDR,0x00);
	WM8750_SEND_CMD(WM8750_R19_ALC3_ADDR,0x32);
	WM8750_SEND_CMD(WM8750_R20_NosieGate_ADDR,0x00);

	WM8750_SEND_CMD(WM8750_R21_LeftADCVol_ADDR|1,0xc3);	// 0db
	WM8750_SEND_CMD(WM8750_R22_RightADCVol_ADDR|1,0xc3);	// 0db

	WM8750_SEND_CMD(WM8750_R23_AdditionalCont1_ADDR,0x8c);	// AVDD 3.3V
	WM8750_SEND_CMD(WM8750_R24_AdditionalCont2_ADDR,WM8750_R24_ROUT2INV_BIT);
	WM8750_SEND_CMD(WM8750_R27_AdditionalCont3_ADDR,0x00);
	WM8750_SEND_CMD(WM8750_R31_ADCInputMode_ADDR,0x00);

	WM8750_SEND_CMD(WM8750_R32_ADCL_ADDR,0x00);
	WM8750_SEND_CMD(WM8750_R33_ADCR_ADDR,0x00);

	WM8750_SEND_CMD(WM8750_R34_LeftOutMix1_ADDR|1,0x00);	// OUTPUT mIXERS,LINPUT1, MIXER MUTE
	WM8750_SEND_CMD(WM8750_R35_LeftOutMix2_ADDR,0x50);	// miXER MUTE
	WM8750_SEND_CMD(WM8750_R36_RightOutMix1_ADDR,0x50);	//
	
	WM8750_SEND_CMD(WM8750_R37_RightOutMix2_ADDR|1,0x00);
	WM8750_SEND_CMD(WM8750_R38_MonoOutMix1_ADDR,0x50);
	WM8750_SEND_CMD(WM8750_R39_MonoOutMix2_ADDR,0x50);
	WM8750_SEND_CMD(WM8750_R40_LOUT2Vol_ADDR,0x79);
	WM8750_SEND_CMD(WM8750_R41_ROUT2Vol_ADDR,0x79);
	WM8750_SEND_CMD(WM8750_R42_MONOOUT_ADDR,0x79);
	
	wm8750_power();
	wm8750_out1_volume();
	wm8750_out2_volume();

	wm8750_dac_mute(0); // unmute
}

module_init(elfin_wm8750_init);
module_exit(elfin_wm8750_exit);
