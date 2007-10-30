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
#include <asm/arch/neponset.h>
#include <asm/hardware/sa1111.h>

#include "sa1100-audio.h"
#include "sa1111-sac.h"

#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif

#define SBI_SMCR	__CCREG(SA1111_SMCR)

static struct sa1111_driver sac_driver;

/*
 * L3 bus driver
 */
static inline unsigned char l3_sa1111_recv_byte(unsigned char addr)
{
	unsigned char dat;

	L3_CAR = addr;
	while ((SASR0 & SASR0_L3RD) == 0)
		mdelay(1);
	dat = L3_CDR;
	SASCR = SASCR_RDD;
	return dat;
}

static void l3_sa1111_recv_msg(struct l3_msg *msg)
{
	int len = msg->len;
	char *p = msg->buf;

	if (len > 1) {
		SACR1 |= SACR1_L3MB;
		while ((len--) > 1)
			*p++ = l3_sa1111_recv_byte(msg->addr);
	}
	SACR1 &= ~SACR1_L3MB;
	*p = l3_sa1111_recv_byte(msg->addr);
}

static inline void l3_sa1111_send_byte(unsigned char addr, unsigned char dat)
{
	L3_CAR = addr;
	L3_CDR = dat;
	while ((SASR0 & SASR0_L3WD) == 0)
		mdelay(1);
	SASCR = SASCR_DTS;
}

static void l3_sa1111_send_msg(struct l3_msg *msg)
{
	int len = msg->len;
	char *p = msg->buf;

	if (len > 1) {
		SACR1 |= SACR1_L3MB;
		while ((len--) > 1)
			l3_sa1111_send_byte(msg->addr, *p++);
	}
	SACR1 &= ~SACR1_L3MB;
	l3_sa1111_send_byte(msg->addr, *p);
}

static int l3_sa1111_xfer(struct l3_adapter *adap, struct l3_msg msgs[], int num)
{
	int i;

	for (i = 0; i < num; i++) {
		struct l3_msg *pmsg = &msgs[i];

		if (pmsg->flags & L3_M_RD)
			l3_sa1111_recv_msg(pmsg);
		else
			l3_sa1111_send_msg(pmsg);
	}

	return num;
}

static struct l3_algorithm l3_sa1111_algo = {
	.name	= "L3 SA1111 algorithm",
	.xfer	= l3_sa1111_xfer,
};

static DECLARE_MUTEX(sa1111_lock);

static struct l3_adapter l3_sa1111_adapter = {
	.owner	= THIS_MODULE,
	.name	= "l3-sa1111",
	.algo	= &l3_sa1111_algo,
	.lock	= &sa1111_lock,
};

/*
 * Definitions
 */
#define AUDIO_RATE_DEFAULT	22050

/*
 * Mixer interface
 */
static struct uda1341 *uda1341;

static int
mixer_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	return uda1341_mixer_ctl(uda1341, cmd, (void *)arg);
}

static struct file_operations uda1341_mixer_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= mixer_ioctl,
};


/*
 * SA1111 SAC DMA support
 */
 
static void reset_sac_dma(sa1111_dma_t *dma)
{
	dma->regs->SAD_CS = 0;
	mdelay(1);
	dma->dma_a = dma->dma_b = 0;
}

static int start_sac_dma(sa1111_dma_t *dma, dma_addr_t dma_ptr, size_t size)
{
	sac_regs_t *sac_regs = dma->regs;

	DPRINTK(" SAC DMA %cCS %02x at %08x (%d)\n",
		(sac_regs==&SADTCS)?'T':'R', sac_regs->SAD_CS, dma_ptr, size);

	/* The minimum transfer length requirement has not yet been
	 * verified:
	 */
	if( size < SA1111_SAC_DMA_MIN_XFER )
	  printk(KERN_WARNING "Warning: SAC xfers below %u bytes may be buggy!"
		 " (%u bytes)\n", SA1111_SAC_DMA_MIN_XFER, size);

	if( dma->dma_a && dma->dma_b ){
	  	DPRINTK("  neither engine available! (A %d, B %d)\n",
			dma->dma_a, dma->dma_b);
	  	return -1;
	}

	if( (dma->last_dma || dma->dma_b) && dma->dma_a == 0 ){
	  	if( sac_regs->SAD_CS & SAD_CS_DBDB ){
		  	DPRINTK("  awaiting \"done B\" interrupt, not starting\n");
			return -1;
		}
		sac_regs->SAD_SA = SA1111_DMA_ADDR((u_int)dma_ptr);
		sac_regs->SAD_CA = size;
		sac_regs->SAD_CS = SAD_CS_DSTA | SAD_CS_DEN;
		++dma->dma_a;
		DPRINTK("  with A [%02lx %08lx %04lx]\n", sac_regs->SAD_CS,
			sac_regs->SAD_SA, sac_regs->SAD_CA);
	} else {
	  	if( sac_regs->SAD_CS & SAD_CS_DBDA ){
		  	DPRINTK("  awaiting \"done A\" interrupt, not starting\n");
			return -1;
		}
		sac_regs->SAD_SB = SA1111_DMA_ADDR((u_int)dma_ptr);
		sac_regs->SAD_CB = size;
		sac_regs->SAD_CS = SAD_CS_DSTB | SAD_CS_DEN;
		++dma->dma_b;
		DPRINTK("  with B [%02lx %08lx %04lx]\n", sac_regs->SAD_CS,
			sac_regs->SAD_SB, sac_regs->SAD_CB);
	}

	/* Additional delay to avoid DMA engine lockup during record: */
	if (sac_regs == (sac_regs_t*)&SADRCS)
	  	mdelay(1);	/* NP : wouuuh! ugly... */

	return 0;
}

static irqreturn_t sac_dma_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	sa1111_dma_t *dma = dev_id;

	DPRINTK("irq %d, last DMA serviced was %c, CS %02x\n", irq,
		dma->last_dma?'B':'A', dma->regs->SAD_CS);

	/* Occasionally, one of the DMA engines (A or B) will
	 * lock up. We try to deal with this by quietly kicking
	 * the control register for the afflicted transfer
	 * direction.
	 *
	 * Note for the debugging-inclined: we currently aren't
	 * properly flushing the DMA engines during channel
	 * shutdown. A slight hiccup at the beginning of playback
	 * after a channel has been stopped can be heard as
	 * evidence of this. Programmatically, this shows up
	 * as either a locked engine, or a spurious interrupt. -jd
	 */

	if(irq==AUDXMTDMADONEA || irq==AUDRCVDMADONEA){

	  	if(dma->last_dma == 0){
		  	DPRINTK("DMA B has locked up!\n");
			dma->regs->SAD_CS = 0;
			mdelay(1);
			dma->dma_a = dma->dma_b = 0;
		} else {
		  	if(dma->dma_a == 0)
			  	DPRINTK("spurious SAC IRQ %d\n", irq);
			else {
			  	--dma->dma_a;

				/* Servicing the SAC DMA engines too quickly
				 * after they issue a DONE interrupt causes
				 * them to lock up.
				 */
				if(irq==AUDRCVDMADONEA || irq==AUDRCVDMADONEB)
				  	mdelay(1);
			}
		}

		dma->regs->SAD_CS = SAD_CS_DBDA | SAD_CS_DEN; /* w1c */
		dma->last_dma = 0;

	} else {

	  	if(dma->last_dma == 1){
		  	DPRINTK("DMA A has locked up!\n");
			dma->regs->SAD_CS = 0;
			mdelay(1);
			dma->dma_a = dma->dma_b = 0;
		} else {
		  	if(dma->dma_b == 0)
			  	DPRINTK("spurious SAC IRQ %d\n", irq);
			else {
			  	--dma->dma_b;

				/* See lock-up note above. */
				if(irq==AUDRCVDMADONEA || irq==AUDRCVDMADONEB)
				  	mdelay(1);
			}
		}

		dma->regs->SAD_CS = SAD_CS_DBDB | SAD_CS_DEN; /* w1c */
		dma->last_dma = 1;

	}

	/* NP: maybe this shouldn't be called in all cases? */
	dma->callback(dma->data);

	return IRQ_HANDLED;
}

static sa1111_dma_t sac_tx = {
	.reset	= reset_sac_dma,
	.start	= start_sac_dma,
	.regs	= (sac_regs_t *) &SADTCS,
};

static sa1111_dma_t sac_rx = {
	.reset	= reset_sac_dma,
	.start	= start_sac_dma,
	.regs	= (sac_regs_t *) &SADRCS,
};


/*
 * Audio interface
 */
static void sa1111_audio_init(void *dummy)
{
	struct sa1111_dev *sadev = dummy;

#ifdef CONFIG_SA1100_JORNADA720
	if (machine_is_jornada720()) {
		/* LDD4 is speaker, LDD3 is microphone */
		PPSR &= ~(PPC_LDD3 | PPC_LDD4);
		PPDR |= PPC_LDD3 | PPC_LDD4;
		PPSR |= PPC_LDD4; /* enable speaker */
		PPSR |= PPC_LDD3; /* enable microphone */
	}
#endif
#ifdef CONFIG_ASSABET_NEPONSET
	if (machine_is_assabet()) {
		/* Select I2S audio (instead of AC-Link) */
		AUD_CTL = AUD_SEL_1341;
	}
#endif

	/* Select i2s mode */
	sa1111_select_audio_mode(sadev, SA1111_AUDIO_I2S);

	/* Enable the I2S clock and L3 bus clock: */
	sa1111_enable_device(sadev);

	/* Activate and reset the Serial Audio Controller */
	SACR0 |= (SACR0_ENB | SACR0_RST);
	mdelay(5);
	SACR0 &= ~SACR0_RST;

	/* For I2S, BIT_CLK is supplied internally. The "SA-1111
	 * Specification Update" mentions that the BCKD bit should
	 * be interpreted as "0 = output". Default clock divider
	 * is 22.05kHz.
	 *
	 * Select I2S, L3 bus. "Recording" and "Replaying"
	 * (receive and transmit) are enabled.
	 */
	SACR1 = SACR1_L3EN;

	/* Initialize the UDA1341 internal state */
	uda1341_open(uda1341);
}

static void sa1111_audio_shutdown(void *dummy)
{
	struct sa1111_dev *sadev = dummy;

	uda1341_close(uda1341);
	SACR0 &= ~SACR0_ENB;

	/* Disable the I2S clock and L3 bus clock */
	sa1111_disable_device(sadev);
}






static int
sa1111_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	audio_state_t *state = file->private_data;
	struct sa1111_dev *sadev = state->data;
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
		sa1111_set_audio_rate(sadev, val);
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

static audio_stream_t output_stream = {
	.sa1111_dma	= 1,
	.dma_regs	= (dma_regs_t *) &sac_tx,
};

static audio_stream_t input_stream = {
	.sa1111_dma	= 1,
	.dma_regs	= (dma_regs_t *) &sac_rx,
};

static audio_state_t audio_state = {
	.output_stream	= &output_stream,
	.input_stream	= &input_stream,
	.hw_init	= sa1111_audio_init,
	.hw_shutdown	= sa1111_audio_shutdown,
	.client_ioctl	= sa1111_audio_ioctl,
	.sem		= __MUTEX_INITIALIZER(audio_state.sem),
};

static int sa1111_audio_open(struct inode *inode, struct file *file)
{
        return sa1100_audio_attach(inode, file, &audio_state);
}

/*
 * Missing fields of this structure will be patched with the call
 * to sa1100_audio_attach().
 */
static struct file_operations sa1111_audio_fops = {
	.owner	= THIS_MODULE,
	.open	= sa1111_audio_open,
};

static int audio_dev_id, mixer_dev_id;

static int sac_probe(struct sa1111_dev *dev)
{
	struct uda1341_cfg cfg;
	int ret;

	if (!request_mem_region(dev->res.start, 512, SA1111_DRIVER_NAME(dev)))
		return -EBUSY;

	audio_state.data = dev;
	input_stream.dev = &dev->dev;
	output_stream.dev = &dev->dev;
	l3_sa1111_adapter.data = dev;

	ret = l3_add_adapter(&l3_sa1111_adapter);
	if (ret)
		goto out;

	uda1341 = uda1341_attach("l3-sa1111");
	if (IS_ERR(uda1341)) {
		ret = PTR_ERR(uda1341);
		goto remove_l3;
	}

	/*
	 * Acquire SAC interrupts
	 */
	ret = request_irq(dev->irq[0], sac_dma_irq, SA_INTERRUPT,
			  SA1111_DRIVER_NAME(dev), &sac_tx);
	if (ret)
		goto release_l3;
	ret = request_irq(dev->irq[1], sac_dma_irq, SA_INTERRUPT,
			  SA1111_DRIVER_NAME(dev), &sac_tx);
	if (ret)
		goto release_ITXA;
	ret = request_irq(dev->irq[2], sac_dma_irq, SA_INTERRUPT,
			  SA1111_DRIVER_NAME(dev), &sac_rx);
	if (ret)
		goto release_ITXB;
	ret = request_irq(dev->irq[3], sac_dma_irq, SA_INTERRUPT,
			  SA1111_DRIVER_NAME(dev), &sac_rx);
	if (ret)
		goto release_IRXA;
	
	cfg.fs     = 256;
	cfg.format = FMT_I2S;
	uda1341_configure(uda1341, &cfg);

	sa1111_set_audio_rate(dev, AUDIO_RATE_DEFAULT);

	/* register devices */
	audio_dev_id = register_sound_dsp(&sa1111_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&uda1341_mixer_fops, -1);

	printk(KERN_INFO "Sound: SA1111 UDA1341: dsp id %d mixer id %d\n",
		audio_dev_id, mixer_dev_id);
	return 0;

release_IRXA:
	free_irq(AUDRCVDMADONEA, &sac_rx);
release_ITXB:
	free_irq(AUDXMTDMADONEB, &sac_tx);
release_ITXA:
	free_irq(AUDXMTDMADONEA, &sac_tx);
release_l3:
	uda1341_detach(uda1341);
remove_l3:
	l3_del_adapter(&l3_sa1111_adapter);
out:
	release_mem_region(dev->res.start, 512);
	return ret;
}

static int sac_remove(struct sa1111_dev *dev)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	free_irq(AUDRCVDMADONEB, &sac_rx);
	free_irq(AUDRCVDMADONEA, &sac_rx);
	free_irq(AUDXMTDMADONEB, &sac_tx);
	free_irq(AUDXMTDMADONEA, &sac_tx);
	uda1341_detach(uda1341);
	l3_del_adapter(&l3_sa1111_adapter);

	release_mem_region(dev->res.start, 512);

	return 0;
}

static int sac_suspend(struct sa1111_dev *dev, u32 state)
{
	return sa1100_audio_suspend(&audio_state, state, SUSPEND_POWER_DOWN);
}

static int sac_resume(struct sa1111_dev *dev)
{
	return sa1100_audio_resume(&audio_state, RESUME_ENABLE);
}

static struct sa1111_driver sac_driver = {
	.drv = {
		.name		= "sa1111-uda1341",
	},
	.devid		= SA1111_DEVID_SAC,
	.probe		= sac_probe,
	.remove		= sac_remove,
	.suspend	= sac_suspend,
	.resume		= sac_resume,
};

static int sa1111_uda1341_init(void)
{
	return sa1111_driver_register(&sac_driver);
}

static void sa1111_uda1341_exit(void)
{
	sa1111_driver_unregister(&sac_driver);
}

module_init(sa1111_uda1341_init);
module_exit(sa1111_uda1341_exit);

MODULE_AUTHOR("John Dorsey, Nicolas Pitre");
MODULE_DESCRIPTION("Glue audio driver for the SA1111 companion chip & Philips UDA1341 codec.");
