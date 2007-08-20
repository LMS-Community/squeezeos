/*
 * Common audio handling for the SA11x0 processor
 *
 * Copyright (C) 2000, 2001 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 *
 * This module handles the generic buffering/DMA/mmap audio interface for
 * codecs connected to the SA1100 chip.  All features depending on specific
 * hardware implementations like supported audio formats or samplerates are
 * relegated to separate specific modules.
 *
 *
 * History:
 *
 * 2000-05-21	Nicolas Pitre	Initial release.
 *
 * 2000-06-10	Erik Bunce	Add initial poll support.
 *
 * 2000-08-22	Nicolas Pitre	Removed all DMA stuff. Now using the
 * 				generic SA1100 DMA interface.
 *
 * 2000-11-30	Nicolas Pitre	- Validation of opened instances;
 * 				- Power handling at open/release time instead
 * 				  of driver load/unload;
 *
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-uda1341.c driver.
 *
 * 2001-07-22	Nicolas Pitre	- added mmap() and realtime support
 * 				- corrected many details to better comply
 * 				  with the OSS API
 *
 * 2001-10-19	Nicolas Pitre	- brought DMA registration processing
 * 				  into this module for better ressource
 * 				  management.  This also fixes a bug
 * 				  with the suspend/resume logic.
 *
 * 2001-12-20	Nicolas Pitre	- moved DMA buffer handling out of the
 * 				  generic code (since only this module
 * 				  used it anyway) and reimplemented it
 * 				  locally for much better efficiency.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>
#include <asm/arch/dma.h>
#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-clock.h>

#include "elfin-audio.h"


#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif

#define SHAJU
#ifdef SHAJU
#define rGPBCON   S3C2413_GPBCON    	//Configure the pins of port B
#define rGPBDAT    S3C2413_GPBDAT		//The data for port B
#define rGPECON   S3C2413_GPECON		//The data for port B


#define rCLKSRC    S3C2413_CLKSRC		//The data for port B
#define rCLKCON    S3C2413_CLKCON		//The data for port B
#define rCLKDIVN    S3C2413_CLKDIVN		//The data for port B
#define rIISPSR    S3C2413_IISPSR		//The data for port B
#define rIISFIC    S3C2413_IISFIC		//The data for port B
#define rIISCON    S3C2413_IISCON		//The data for port B
#define rIISMOD    S3C2413_IISMOD		//The data for port B



#define L3C (1<<4)	//GPB4 = L3CLOCK
#define L3D (1<<3)	//GPB3 = L3DATA 
#define L3M (1<<2)	//GPB2 = L3MODE

#define S_CLOCK_FREQ    384

unsigned long iorem_base,dma_base,temp_base,iorem_phy;

static int iispsr_value(int sample_rate)
{
    int i;
    unsigned long fact0 = 50*1000*1000 / S_CLOCK_FREQ;
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






void set_IIS(void)
{
 	writel( (0<<5) | (0<<4) |(1<<2) | (0<<1)|0,rIISCON);
    writel( ((0<<10)|0<<8) | (0<<7)| (0<<5) | (2<<3)| (0<<1) | (0<<0),rIISMOD);
   writel(  0|(1<<15),rIISFIC) ;
    writel( 0|(0<<15),rIISFIC) ;
	
	printk("IISCON is %x\n",readl(rIISCON));
//writel(readl(rIISCON)| 0x1,rIISCON);

}

void select_PCLK(void)
{

	writel((readl(rCLKCON)| (1 << 26)| (1<<13)),rCLKCON);
	writel((readl(rCLKSRC)| (1 <<9)),rCLKSRC);
	writel((readl(rCLKDIVN) & ~(0xf << 12)),rCLKDIVN);
    //IIS Pre-scaler Setting
	writel((readl(rIISPSR)&0),rIISPSR);
	

 writel((readl(rIISPSR)|(1<<15) |(5<<8)) ,rIISPSR);
//	writel((readl(rIISPSR)|(1<<15) |(8<<8)) ,rIISPSR);
	printk("CLKCON is %x\n",readl(rCLKCON));
}




void _WrL3Addr(unsigned char data)
{	 
 	int i,j;

    	writel(((readl( rGPBDAT) & ~(L3D | L3M | L3C)) | (L3C)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H

				udelay(1);
    	//for(j=0;j<4;j++);	 //tsu(L3) > 190ns

	//GPB[4:2]=L3C:L3D:L3M
    	for(i=0;i<8;i++)	//LSB first
    	{
	  	if(data & 0x1)	//If data's LSB is 'H'
	  	{
			writel((readl(rGPBDAT)&~L3C),rGPBDAT);
			writel((readl(rGPBDAT)|L3D),rGPBDAT);
				udelay(1);
			//for(j=0;j<4;j++);	        //tcy(L3) > 500ns
			writel((readl(rGPBDAT)|L3C),rGPBDAT);
			writel((readl(rGPBDAT)|L3D),rGPBDAT);
				udelay(1);
			//for(j=0;j<4;j++);	        //tcy(L3) > 500ns
	  	}
	  	else		//If data's LSB is 'L'
	  	{
			writel((readl(rGPBDAT)&~L3C),rGPBDAT);
			writel((readl(rGPBDAT)&~L3D),rGPBDAT);
			//for(j=0;j<4;j++);	       //tcy(L3) > 500ns
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
			writel((readl(rGPBDAT)|L3C),rGPBDAT);
			writel((readl(rGPBDAT)&~L3D),rGPBDAT);
			//for(j=0;j<4;j++);	       //tcy(L3) > 500ns		
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
	  	}
	  	data >>= 1;
    	}

    	writel(((readl( rGPBDAT) & ~(L3D | L3M | L3C)) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
}


void _WrL3Data(unsigned char data,int halt)
{
 	int i,j;

    	if(halt)
    	{
    	writel(((readl( rGPBDAT) & ~(L3D | L3M | L3C)) | L3C ) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
	  //	for(j=0;j<4;j++);		//tstp(L3) > 190ns
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
    	}

    	writel(((readl( rGPBDAT) & ~(L3D | L3M | L3C)) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
    //	for(j=0;j<4;j++);		//tsu(L3)D > 190ns

	//GPB[4:2]=L3C:L3D:L3M
    	for(i=0;i<8;i++)
    	{
	  	if(data & 0x1)	//if data's LSB is 'H'
	  	{
			writel((readl(rGPBDAT)&~L3C),rGPBDAT);
			writel((readl(rGPBDAT)|L3D),rGPBDAT);

	     		//for(j=0;j<4;j++);			//tcy(L3) > 500ns
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
			writel((readl(rGPBDAT)|(L3D|L3C)),rGPBDAT);
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
	     		//for(j=0;j<4;j++);		 	//tcy(L3) > 500ns
	  	}
	  	else		//If data's LSB is 'L'
	  	{
			writel((readl(rGPBDAT)&~L3C),rGPBDAT);
			writel((readl(rGPBDAT)&~L3D),rGPBDAT);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
				udelay(1);
			writel((readl(rGPBDAT)|L3C),rGPBDAT);
			writel((readl(rGPBDAT)&~L3D),rGPBDAT);
				udelay(1);
	     		//for(j=0;j<4;j++);		//tcy(L3) > 500ns
	  	}
		data >>= 1;		//For check next bit
    	}

    	writel(((readl( rGPBDAT) & ~(L3D | L3M | L3C)) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
}


void IIS_Port_Init(void)
{

    //----------------------------------------------------------
    //PORT B GROUP
    //Ports  :   GPB2         GPB4           GPB3
    //Signal :  L3MODE     L3CLOCK    L3DATA
    //Setting:  OUTPUT     OUTPUT      OUTPUT
    //            [11:10]      [13:12]      [15:14]
    //Binary :  01             01              01
    //----------------------------------------------------------
    //rGPBCON = rGPBCON & ~(0x3<<4) & ~(0x3<<6) & ~(0x3<<8);
    //rGPBCON |= (0x1<<4) |(0x1<<6) |(0x1<<8);

	writel((readl(rGPBCON)& ~(0x3 << 4)&~(0x3 << 6) & ~(0x3 << 8)), rGPBCON );
	writel((readl(rGPBCON) | (1 << 4 | 1 << 6 |1 <<8) ), rGPBCON );



    //-------------------------------------------------------------------------------
    //PORT E GROUP
    //Ports  :     GPE4          GPE3             GPE2           GPE1            GPE0
    //Signal :    I2S DO           I2S DI         CDCLK        I2S CLK        I2S LRCLK
    //Binary :   10,                10,             10,              10,              10,            10
    //-------------------------------------------------------------------------------
   // rGPECON = rGPECON & ~(0x3ff) | 0x2aa;
	writel(((readl(rGPECON)&~(0x3ff))| 0x2aa),rGPECON);
}

#define rDISRC2       (dma_base + 0x00)
#define rDISRCC2      (dma_base + 0x04)
#define rDIDST2       (dma_base + 0x08)
#define rDIDSTC2      (dma_base + 0x0C)
#define rDCON2        (dma_base + 0x10)
#define rDSTAT2       (dma_base + 0x14)
#define rDCSRC2       (dma_base + 0x18)
#define rDCDST2       (dma_base + 0x1C)
#define rDMASKTRIG2   (dma_base + 0x20)
#define rDMAREQSEL2   (dma_base + 0x24)



void start_dma(void)
{
	printk("in start_dma\n");
	//writel(0x2100000,rDISRC2); 
	writel(iorem_phy,rDISRC2); 
	writel((0<<1)|(0<<0),rDISRCC2); 
	writel(0x55000010,rDIDST2); 
	writel((1<<1)| (1<<0),rDIDSTC2); 
	writel((1<<31)| (0<<30)|(1<<29)|(0<<28)|(0<<27)|(0<<22)|(2<<20)|0x4ff00,rDCON2); 
	writel((4 << 1)|(1<<0 ),rDMAREQSEL2);
	writel((0 << 2)|(1<<1 )|(0<<0),rDMASKTRIG2);

}
void start_iis(void)
{

	
	printk("in startiis _dma3\n");

writel(readl(rIISCON)| 0x1,rIISCON);

}


void chk_progress(void)
{

printk("curr tc is2 %x\n", readl(rDSTAT2) );
printk("cuu src curr dst %x %x\n",readl(rDCSRC2),readl(rDCDST2));
printk("src is %x \n",readl(rDISRC2));
if((readl(rDSTAT2)) < 0x30000)
{
//printk("******stop******\n");
	//writel((readl(rCLKCON)& ~(1 << 26)& ~(1<<13)),rCLKCON);
}

}





void Init1341(int mode)
{
 	
	//----------------------------------------------------------
	//PORT B GROUP
	//Ports  :   GPB3  		GPB4  		GPB2
	//Signal :   L3DATA      L3CLOCK 	L3MODE 
	//Setting:  OUTPUT      OUTPUT 		OUTPUT
	//	          [7:6]		[9:8] 		[5:4] 
	//Binary :     01 	      01	      01
	//---------------------------------------------------------- 
    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
		
		writel((readl(rGPBCON)& ~(0x3<<4) & ~(0x3<<6) & ~(0x3<<8)),rGPBCON);
		writel((readl(rGPBCON)| (0x1<<4) |(0x1<<6) |(0x1<<8) ),rGPBCON);
 
	//L3 Interface
    _WrL3Addr(0x14 + 2);     //STATUS (000101xx+10)
 	_WrL3Data(0x50,0);	 //0,1,01, 000,0 : Status 0,Reset, 384fs,IIS-bus,no DC-filtering
	
	_WrL3Addr(0x14 + 2);     //STATUS (000101xx+10)
    _WrL3Data(0x81,0);	 //bit[7:0] => 1,0,0,0, 0,0,01 

	//Record Sound via MIC-In
    if(mode == 1)
    {
	  	_WrL3Addr(0x14 + 2);    //STATUS (000101xx+10)
		_WrL3Data(0xa2,0);	//bit[7:0] => 1,0,1,0,0,0,10
		
		_WrL3Addr(0x14 + 0);    //DATA0 (000101xx+00)
       	_WrL3Data(0xc2, 0);	//1100 0,010  : Extended addr(3bits), 010 
        _WrL3Data(0xf2, 0);	//111,100,10 : DATA0, MIC Amplifier Gain 21dB, input channel 2 select (input channel 1 off)              
    }
}


#endif //shaju































#define AUDIO_NAME		"sa1100-audio"
#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	8192


#define AUDIO_ACTIVE(state)	((state)->rd_ref || (state)->wr_ref)

#define SPIN_ADDR		(dma_addr_t)FLUSH_BASE_PHYS
#define SPIN_SIZE		2048



void audio_dma_callback(s3c2413_dma_chan_t *dma_ch, void *buf_id,
        int size, s3c2413_dma_buffresult_t result);

/*
 * DMA processing
 */



static struct s3c2413_dma_client s3c2413play_dma_client = {
	.name		= "s3c2413-play",
};
static struct s3c2413_dma_client s3c2413rec_dma_client = {
	.name		= "s3c2413-rec",
};

static int s3c24xx_iis_dma_init(audio_stream_t *s,int mode)
{
	int err = 0;

	if(mode == 0) //play 
	{

		s3c2413_dma_devconfig(s->dma,
			      S3C2413_DMASRC_MEM, 0x03,
			      S3C2413_PA_IIS + S3C2413_IISFIFO_TX);

		s3c2413_dma_config(s->dma,
			   2, S3C2413_DCON_SYNC_PCLK|S3C2413_DCON_HANDSHAKE,S3C2413_REQSEL_IIS_TX | S3C2413_REQSEL_HWTRIG);

		s3c2413_dma_set_buffdone_fn(s->dma,
				    audio_dma_callback,s);
        s3c2413_dma_setflags(s->dma, S3C2413_DMAF_AUTOSTART);

	}
	/* attach capture channel */
	if(mode ==1)
	{

		s3c2413_dma_config(s->dma,
			   2, S3C2413_DCON_HANDSHAKE |
			   S3C2413_DCON_SYNC_PCLK,S3C2413_REQSEL_IIS_RX | S3C2413_REQSEL_HWTRIG);

	s3c2413_dma_devconfig(s->dma,
			      S3C2413_DMASRC_HW, 0x3,
			      S3C2413_PA_IIS + S3C2413_IISFIFO_RX);

	s3c2413_dma_set_buffdone_fn(s->dma,
				    audio_dma_callback,s);
        s3c2413_dma_setflags(s->dma, S3C2413_DMAF_AUTOSTART);
	}
	return 0;
}




static u_int audio_get_dma_pos(audio_stream_t *s)
{
	int dma_srcpos,dma_dstpos;
	audio_buf_t *b = &s->buffers[s->dma_tail];
	u_int offset;

	if (b->dma_ref) {
 		s3c2413_dma_getposition(s->dma, &dma_srcpos,&dma_dstpos);

		offset = dma_srcpos - b->dma_addr;


		if (offset >= s->fragsize)
			offset = s->fragsize - 4;
	} else if (s->pending_frags) {
		offset = b->offset;
	} else {
		offset = 0;
	}
	return offset;
}


static void audio_stop_dma(audio_stream_t *s)
{
	u_int pos;
	unsigned long flags;
	audio_buf_t *b;

	if (s->dma_spinref > 0 || !s->buffers)
		return;

	local_irq_save(flags);
	s->stopped = 1;
	pos = audio_get_dma_pos(s);
	s3c2413_dma_ctrl(s->dma, S3C2413_DMAOP_FLUSH);
	if (s->spin_idle) {
//shaju this may not be required
	//	DMA_START(s, SPIN_ADDR, SPIN_SIZE);
	//	DMA_START(s, SPIN_ADDR, SPIN_SIZE);
	//	s->dma_spinref = 2;
	} else
		s->dma_spinref = 0;
	local_irq_restore(flags);

	/* back up pointers to be ready to restart from the same spot */
	while (s->dma_head != s->dma_tail) {
		b = &s->buffers[s->dma_head];
		if (b->dma_ref) {
			b->dma_ref = 0;
			b->offset = 0;
		}
		s->pending_frags++;
		if (s->dma_head == 0)
			s->dma_head = s->nbfrags;
		s->dma_head--;
	}
	b = &s->buffers[s->dma_head];
	if (b->dma_ref) {
		b->offset = pos;
		b->dma_ref = 0;
	}
}


static void audio_reset(audio_stream_t *s)
{
	if (s->buffers) {
		audio_stop_dma(s);
		s->buffers[s->dma_head].offset = 0;
		s->buffers[s->usr_head].offset = 0;
		s->usr_head = s->dma_head;
		s->pending_frags = 0;
		sema_init(&s->sem, s->nbfrags);
	}
	s->active = 0;
	s->stopped = 0;
}


static void audio_process_dma(audio_stream_t *s)
{
	int ret;
#if 0
	if (s->stopped)
		goto spin;

	if (s->dma_spinref > 0 && s->pending_frags) {
		s->dma_spinref = 0;
		DMA_CLEAR(s);
	}
#endif
	while (s->pending_frags) {
		audio_buf_t *b = &s->buffers[s->dma_head];
		u_int dma_size = s->fragsize - b->offset;
	//	if (dma_size > MAX_DMA_SIZE)
	//		dma_size = CUT_DMA_SIZE;

		 s3c2413_dma_enqueue(s->dma, (void *) b,
                        b->dma_addr+b->offset,dma_size);


		b->dma_ref++;
		b->offset += dma_size;
		if (b->offset >= s->fragsize) {
			s->pending_frags--;
			if (++s->dma_head >= s->nbfrags)
				s->dma_head = 0;
		
		}
	}
#if 0
spin:
	if (s->spin_idle) {
		int spincnt = 0;
		while (DMA_START(s, SPIN_ADDR, SPIN_SIZE) == 0)
			spincnt++;
		/*
		 * Note: if there is still a data buffer being
		 * processed then the ref count is negative.  This
		 * allows for the DMA termination to be accounted in
		 * the proper order.  Of course dma_spinref can't be
		 * greater than 0 if dma_ref is not 0 since we kill
		 * the spinning above as soon as there is real data to process.
		 */
		if (s->buffers && s->buffers[s->dma_tail].dma_ref)
			spincnt = -spincnt;
		s->dma_spinref += spincnt;
	}
#endif
}




void audio_dma_callback(s3c2413_dma_chan_t *dma_ch, void *buf_id,
        int size, s3c2413_dma_buffresult_t result)

{
	audio_stream_t *s = dma_ch->callback_arg;
	audio_buf_t *b = &s->buffers[s->dma_tail];

	if (s->dma_spinref > 0) {
		s->dma_spinref--;
	} else if (!s->buffers) {
		printk(KERN_CRIT "elfin: received DMA IRQ for non existent buffers!\n");
		return;
	} else if (b->dma_ref && --b->dma_ref == 0 && b->offset >= s->fragsize) {
		/* This fragment is done */
		b->offset = 0;
		s->bytecount += s->fragsize;
		s->fragcount++;
		//s->dma_spinref = -s->dma_spinref;
		if (++s->dma_tail >= s->nbfrags)
			s->dma_tail = 0;
		if (!s->mapped)
			up(&s->sem);
		else
			s->pending_frags++;
		wake_up(&s->wq);
	}
	audio_process_dma(s);
}


/*
 * Buffer creation/destruction
 */

static void audio_discard_buf(audio_stream_t * s)
{
	DPRINTK("audio_discard_buf\n");

	/* ensure DMA isn't using those buffers */
	audio_reset(s);

	if (s->buffers) {
		int frag;
		for (frag = 0; frag < s->nbfrags; frag++) {
			if (!s->buffers[frag].master)
				continue;
			dma_free_coherent(s->dev,
					s->buffers[frag].master,
					s->buffers[frag].data,
					s->buffers[frag].dma_addr);
		}
		kfree(s->buffers);
		s->buffers = NULL;
	}
}


static int audio_setup_buf(audio_stream_t * s)
{
	int frag;
	int dmasize = 0;
	char *dmabuf = NULL;
	dma_addr_t dmaphys = 0;

	if (s->buffers)
		return -EBUSY;

	s->buffers = kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL);
	if (!s->buffers)
		goto err;
	memset(s->buffers, 0, sizeof(audio_buf_t) * s->nbfrags);

	for (frag = 0; frag < s->nbfrags; frag++) {
		audio_buf_t *b = &s->buffers[frag];

		/*
		 * Let's allocate non-cached memory for DMA buffers.
		 * We try to allocate all memory at once.
		 * If this fails (a common reason is memory fragmentation),
		 * then we allocate more smaller buffers.
		 */
		if (!dmasize) {
			dmasize = (s->nbfrags - frag) * s->fragsize;
			do {
			  	dmabuf = dma_alloc_coherent(s->dev, dmasize, &dmaphys, GFP_KERNEL);
				if (!dmabuf)
					dmasize -= s->fragsize;
			} while (!dmabuf && dmasize);
			if (!dmabuf)
				goto err;
			b->master = dmasize;
			memzero(dmabuf, dmasize);
		}

		b->data = dmabuf;
		b->dma_addr = dmaphys;
		DPRINTK("buf %d: start %p dma %#08x\n", frag, b->data,
			b->dma_addr);

		dmabuf += s->fragsize;
		dmaphys += s->fragsize;
		dmasize -= s->fragsize;
	}

	s->usr_head = s->dma_head = s->dma_tail = 0;
	s->bytecount = 0;
	s->fragcount = 0;
	sema_init(&s->sem, s->nbfrags);

	return 0;

err:
	printk(AUDIO_NAME ": unable to allocate audio memory\n ");
	audio_discard_buf(s);
	return -ENOMEM;
}


/*
 * Driver interface functions
 */


void check_buffer1(unsigned long addr)
{
	int i;
	printk("copy chk in audio write \n");
	for(i=0; i< 0x4*1024;i=i+4)
	{
		printk("%x \n",*(unsigned long *)(addr+i));

	}


}


static int audio_write(struct file *file, const char *buffer,
		       size_t count, loff_t * ppos)
{
#if 0
	unsigned char *ptr;

ptr = kmalloc(20*1024,GFP_KERNEL);
	if (copy_from_user((unsigned char *)ptr, buffer, count)) {
			printk("audio_write FAULT\n");
			return -EFAULT;
		}
	printk("audio_write:kmalloc");
	check_buffer1((unsigned long)ptr);
	
	#endif

	if (copy_from_user((unsigned char *)iorem_base, buffer, count)) {
			printk("audio_write FAULT\n");
			return -EFAULT;
		}
	iorem_base+=count;
return count;

#if 0
	if (s->mapped)
		return -ENXIO;
	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;

	while (count > 0) {
		audio_buf_t *b = &s->buffers[s->usr_head];

		/* Wait for a buffer to become free */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&s->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&s->sem))
				break;
		}

		/* Feed the current buffer */
		chunksize = s->fragsize - b->offset;
		if (chunksize > count)
			chunksize = count;
		DPRINTK("write %d to %d\n", chunksize, s->usr_head);
		if (copy_from_user(b->data + b->offset, buffer, chunksize)) {
			up(&s->sem);
			return -EFAULT;
		}
		buffer += chunksize;
		count -= chunksize;
		b->offset += chunksize;
		if (b->offset < s->fragsize) {
			up(&s->sem);
			break;
		}

		/* Update pointers and send current fragment to DMA */
		b->offset = 0;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		s->active = 1;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;
	DPRINTK("audio_write: return=%d\n", ret);
	return ret;
#endif


}


static void audio_prime_rx(audio_state_t *state)
{
	audio_stream_t *is = state->input_stream;
	unsigned long flags;

	local_irq_save(flags);
	is->pending_frags = is->nbfrags;
	sema_init(&is->sem, 0);
	is->active = 1;
	audio_process_dma(is);
	local_irq_restore(flags);
}


static int audio_read(struct file *file, char *buffer,
		      size_t count, loff_t * ppos)
{
	char *buffer0 = buffer;
	audio_state_t *state = file->private_data;
	audio_stream_t *s = state->input_stream;
	int chunksize, ret = 0;
	unsigned long flags;

	DPRINTK("audio_read: count=%d\n", count);

	if (s->mapped)
		return -ENXIO;

	if (!s->active) {
		if (!s->buffers && audio_setup_buf(s))
			return -ENOMEM;
		audio_prime_rx(state);
	}

	while (count > 0) {
		audio_buf_t *b = &s->buffers[s->usr_head];

		/* Wait for a buffer to become full */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&s->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&s->sem))
				break;
		}

		/* Grab data from the current buffer */
		chunksize = s->fragsize - b->offset;
		if (chunksize > count)
			chunksize = count;
		DPRINTK("read %d from %d\n", chunksize, s->usr_head);
		if (copy_to_user(buffer, b->data+ b->offset, chunksize)) {
			up(&s->sem);
			return -EFAULT;
		}
		buffer += chunksize;
		count -= chunksize;
		b->offset += chunksize;
		if (b->offset < s->fragsize) {
			up(&s->sem);
			break;
		}

		/* Update pointers and return current fragment to DMA */
		b->offset = 0;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;
	DPRINTK("audio_read: return=%d\n", ret);
	return ret;
}

static int audio_sync(struct file *file)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *s = state->output_stream;
	audio_buf_t *b;
	u_int shiftval = 0;
	unsigned long flags;
	DECLARE_WAITQUEUE(wait, current);

	DPRINTK("audio_sync\n");

	if (!(file->f_mode & FMODE_WRITE) || !s->buffers || s->mapped)
		return 0;

	/*
	 * Send current buffer if it contains data.  Be sure to send
	 * a full sample count.
	 */
	b = &s->buffers[s->usr_head];
	if (b->offset &= ~3) {
		down(&s->sem);
		/*
		 * HACK ALERT !
		 * To avoid increased complexity in the rest of the code
		 * where full fragment sizes are assumed, we cheat a little
		 * with the start pointer here and don't forget to restore
		 * it later.
		 */
		shiftval = s->fragsize - b->offset;
		b->offset = shiftval;
		b->dma_addr -= shiftval;
		s->bytecount -= shiftval;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	/* Let's wait for all buffers to complete */
	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&s->wq, &wait);
	while (s->pending_frags &&
	       s->dma_tail != s->usr_head &&
	       !signal_pending(current)) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&s->wq, &wait);

	/* undo the pointer hack above */
	if (shiftval) {
		local_irq_save(flags);
		b->dma_addr += shiftval;
		/* ensure sane DMA code behavior if not yet processed */
		if (b->offset != 0)
			b->offset = s->fragsize;
		local_irq_restore(flags);
	}

	return 0;
}

static int audio_mmap(struct file *file, struct vm_area_struct *vma)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *s;
	unsigned long size, vma_addr;
	int i, ret;

	if (vma->vm_pgoff != 0)
		return -EINVAL;

	if (vma->vm_flags & VM_WRITE) {
		if (!state->wr_ref)
			return -EINVAL;;
		s = state->output_stream;
	} else if (vma->vm_flags & VM_READ) {
		if (!state->rd_ref)
			return -EINVAL;
		s = state->input_stream;
	} else return -EINVAL;

	if (s->mapped)
		return -EINVAL;
	size = vma->vm_end - vma->vm_start;
	if (size != s->fragsize * s->nbfrags)
		return -EINVAL;
	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;
	vma_addr = vma->vm_start;
	for (i = 0; i < s->nbfrags; i++) {
		audio_buf_t *buf = &s->buffers[i];
		if (!buf->master)
			continue;
		ret = remap_pfn_range(vma, vma_addr, buf->dma_addr, buf->master, vma->vm_page_prot);
		if (ret)
			return ret;
		vma_addr += buf->master;
	}
	s->mapped = 1;

	return 0;
}

static unsigned int audio_poll(struct file *file,
			       struct poll_table_struct *wait)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *is = state->input_stream;
	audio_stream_t *os = state->output_stream;
	unsigned int mask = 0;

	DPRINTK("audio_poll(): mode=%s%s\n",
		(file->f_mode & FMODE_READ) ? "r" : "",
		(file->f_mode & FMODE_WRITE) ? "w" : "");

	if (file->f_mode & FMODE_READ) {
		/* Start audio input if not already active */
		if (!is->active) {
			if (!is->buffers && audio_setup_buf(is))
				return -ENOMEM;
			audio_prime_rx(state);
		}
		poll_wait(file, &is->wq, wait);
	}

	if (file->f_mode & FMODE_WRITE) {
		if (!os->buffers && audio_setup_buf(os))
			return -ENOMEM;
		poll_wait(file, &os->wq, wait);
	}

	if (file->f_mode & FMODE_READ)
		if (( is->mapped && is->bytecount > 0) ||
		    (!is->mapped && sema_count(&is->sem) > 0))
			mask |= POLLIN | POLLRDNORM;

	if (file->f_mode & FMODE_WRITE)
		if (( os->mapped && os->bytecount > 0) ||
		    (!os->mapped && sema_count(&os->sem) > 0))
			mask |= POLLOUT | POLLWRNORM;

	DPRINTK("audio_poll() returned mask of %s%s\n",
		(mask & POLLIN) ? "r" : "",
		(mask & POLLOUT) ? "w" : "");

	return mask;
}

static loff_t audio_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

static int audio_set_fragments(audio_stream_t *s, int val)
{
	if (s->active)
		return -EBUSY;
	if (s->buffers)
		audio_discard_buf(s);
	s->nbfrags = (val >> 16) & 0x7FFF;
	val &= 0xffff;
	if (val < 4)
		val = 4;
	if (s->sa1111_dma && val < 11)
		val = 11;
	if (val > 15)
		val = 15;
	s->fragsize = 1 << val;
	if (s->nbfrags < 2)
		s->nbfrags = 2;
	if (s->nbfrags * s->fragsize > 128 * 1024)
		s->nbfrags = 128 * 1024 / s->fragsize;
	if (audio_setup_buf(s))
		return -ENOMEM;
	return val|(s->nbfrags << 16);
}

void check_buffer(void)
{
	int i;
	for(i=0; i< 0x20000;i=i+4)
	{
		printk("%x \n",*(unsigned long *)(temp_base+i));

	}


}


static int audio_ioctl(struct inode *inode, struct file *file,
		       uint cmd, ulong arg)
{

	if(cmd ==10 )
{
printk("dma iis started\n");
	//check_buffer();
	start_dma();
	//start_iis();
	return 0;
}
//if(cmd ==11 )
//chk_progress();

return 0;
#if 0
	audio_state_t *state = file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	long val;

	/* dispatch based on command */
	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);

	case SNDCTL_DSP_GETBLKSIZE:
		if (file->f_mode & FMODE_WRITE)
			return put_user(os->fragsize, (int *)arg);
		else
			return put_user(is->fragsize, (int *)arg);

	case SNDCTL_DSP_GETCAPS:
		val = DSP_CAP_REALTIME|DSP_CAP_TRIGGER|DSP_CAP_MMAP;
		if (is && os)
			val |= DSP_CAP_DUPLEX;
		return put_user(val, (int *)arg);

	case SNDCTL_DSP_SETFRAGMENT:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		if (file->f_mode & FMODE_READ) {
			int ret = audio_set_fragments(is, val);
			if (ret < 0)
				return ret;
			ret = put_user(ret, (int *)arg);
			if (ret)
				return ret;
		}
		if (file->f_mode & FMODE_WRITE) {
			int ret = audio_set_fragments(os, val);
			if (ret < 0)
				return ret;
			ret = put_user(ret, (int *)arg);
			if (ret)
				return ret;
		}
		return 0;

	case SNDCTL_DSP_SYNC:
		return audio_sync(file);

	case SNDCTL_DSP_SETDUPLEX:
		return 0;

	case SNDCTL_DSP_POST:
		return 0;

	case SNDCTL_DSP_GETTRIGGER:
		val = 0;
		if (file->f_mode & FMODE_READ && is->active && !is->stopped)
			val |= PCM_ENABLE_INPUT;
		if (file->f_mode & FMODE_WRITE && os->active && !os->stopped)
			val |= PCM_ENABLE_OUTPUT;
		return put_user(val, (int *)arg);

	case SNDCTL_DSP_SETTRIGGER:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		if (file->f_mode & FMODE_READ) {
			if (val & PCM_ENABLE_INPUT) {
				unsigned long flags;
				if (!is->active) {
					if (!is->buffers && audio_setup_buf(is))
						return -ENOMEM;
					audio_prime_rx(state);
				}
				local_irq_save(flags);
				is->stopped = 0;
				audio_process_dma(is);
				local_irq_restore(flags);
			} else {
				audio_stop_dma(is);
			}
		}
		if (file->f_mode & FMODE_WRITE) {
			if (val & PCM_ENABLE_OUTPUT) {
				unsigned long flags;
				if (!os->buffers && audio_setup_buf(os))
					return -ENOMEM;
				local_irq_save(flags);
				if (os->mapped && !os->pending_frags) {
					os->pending_frags = os->nbfrags;
					sema_init(&os->sem, 0);
					os->active = 1;
				}
				os->stopped = 0;
				audio_process_dma(os);
				local_irq_restore(flags);
			} else {
				audio_stop_dma(os);
			}
		}
		return 0;

	case SNDCTL_DSP_GETOPTR:
	case SNDCTL_DSP_GETIPTR:
	    {
		count_info inf = { 0, };
		audio_stream_t *s = (cmd == SNDCTL_DSP_GETOPTR) ? os : is;
		int bytecount, offset;
		unsigned long flags;

		if ((s == is && !(file->f_mode & FMODE_READ)) ||
		    (s == os && !(file->f_mode & FMODE_WRITE)))
			return -EINVAL;
		if (s->active) {
			local_irq_save(flags);
			offset = audio_get_dma_pos(s);
			inf.ptr = s->dma_tail * s->fragsize + offset;
			bytecount = s->bytecount + offset;
			s->bytecount = -offset;
			inf.blocks = s->fragcount;
			s->fragcount = 0;
			local_irq_restore(flags);
			if (bytecount < 0)
				bytecount = 0;
			inf.bytes = bytecount;
		}
		return copy_to_user((void *)arg, &inf, sizeof(inf));
	    }

	case SNDCTL_DSP_GETOSPACE:
	case SNDCTL_DSP_GETISPACE:
	    {
		audio_buf_info inf = { 0, };
		audio_stream_t *s = (cmd == SNDCTL_DSP_GETOSPACE) ? os : is;

		if ((s == is && !(file->f_mode & FMODE_READ)) ||
		    (s == os && !(file->f_mode & FMODE_WRITE)))
			return -EINVAL;
		if (!s->buffers && audio_setup_buf(s))
			return -ENOMEM;
		inf.bytes = sema_count(&s->sem) * s->fragsize;
		inf.bytes -= s->buffers[s->usr_head].offset;
		inf.fragments = inf.bytes / s->fragsize;
		inf.fragsize = s->fragsize;
		inf.fragstotal = s->nbfrags;
		return copy_to_user((void *)arg, &inf, sizeof(inf));
	    }

	case SNDCTL_DSP_NONBLOCK:
		file->f_flags |= O_NONBLOCK;
		return 0;

	case SNDCTL_DSP_RESET:
		if (file->f_mode & FMODE_READ) {
			audio_reset(is);
		}
		if (file->f_mode & FMODE_WRITE) {
			audio_reset(os);
		}
		return 0;

	default:
		/*
		 * Let the client of this module handle the
		 * non generic ioctls
		 */
		return state->client_ioctl(inode, file, cmd, arg);
	}
#endif
	return 0;
}

static int audio_release(struct inode *inode, struct file *file)
{

	return 0;
	audio_state_t *state = file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;

	DPRINTK("audio_release\n");

	down(&state->sem);

	if (file->f_mode & FMODE_READ) {
		audio_discard_buf(is);
		s3c2413_dma_free(is->dma,&s3c2413rec_dma_client);
		is->dma_spinref = 0;
		
		state->rd_ref = 0;
	}

	if (file->f_mode & FMODE_WRITE) {
		audio_sync(file);
		audio_discard_buf(os);
		s3c2413_dma_free(os->dma,&s3c2413play_dma_client);
			os->dma_spinref = 0;
		state->wr_ref = 0;
	}

	if (!AUDIO_ACTIVE(state)) {
	       if (state->hw_shutdown)
		       state->hw_shutdown(state->data);
	}

	up(&state->sem);
	return 0;
}


#ifdef CONFIG_PM

int elfin_audio_suspend(audio_state_t *s, u32 state, u32 level)
{
	if (level == SUSPEND_DISABLE || level == SUSPEND_POWER_DOWN) {
		audio_stream_t *is = s->input_stream;
		audio_stream_t *os = s->output_stream;
		int stopstate;

		if (is  ) {
			stopstate = is->stopped;
			audio_stop_dma(is);
			s3c2413_dma_ctrl(is->dma, S3C2413_DMAOP_FLUSH);
			is->dma_spinref = 0;
			is->stopped = stopstate;
		}
		if (os ) {
			stopstate = os->stopped;
			audio_stop_dma(os);
			s3c2413_dma_ctrl(os->dma, S3C2413_DMAOP_FLUSH);
			os->dma_spinref = 0;
			os->stopped = stopstate;
		}
		if (AUDIO_ACTIVE(s) && s->hw_shutdown)
			s->hw_shutdown(s->data);
	}
	return 0;
}

int elfin_audio_resume(audio_state_t *s, u32 level)
{
	if (level == RESUME_ENABLE) {
		audio_stream_t *is = s->input_stream;
		audio_stream_t *os = s->output_stream;

		if (AUDIO_ACTIVE(s) && s->hw_init)
			s->hw_init(s->data);
		if (os && os->dma_regs) {
			//DMA_RESET(os);
			audio_process_dma(os);
		}
		if (is && is->dma_regs) {
			//DMA_RESET(is);
			audio_process_dma(is);
		}
	}
	return 0;
}

EXPORT_SYMBOL(elfin_audio_suspend);
EXPORT_SYMBOL(elfin_audio_resume);
#else
#define elfin_audio_suspend() NULL
#define elfin_audio_resume() NULL 
#endif


static int elfin_audio_open(struct inode *inode, struct file *file)
{
	int err, need_tx_dma;
#if 0
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	int err, need_tx_dma;

	DPRINTK("audio_open\n");

	down(&state->sem);

	/* access control */
	err = -ENODEV;
	if ((file->f_mode & FMODE_WRITE) && !os)
		goto out;
	if ((file->f_mode & FMODE_READ) && !is)
		goto out;
	err = -EBUSY;
	if ((file->f_mode & FMODE_WRITE) && state->wr_ref)
		goto out;
	if ((file->f_mode & FMODE_READ) && state->rd_ref)
		goto out;
	err = -EINVAL;
	if ((file->f_mode & FMODE_READ) && state->need_tx_for_rx && !os)
		goto out;
	


/* request DMA channels */
	if (file->f_mode & FMODE_WRITE) {
		if(s3c2413_dma_request(os->dma, &s3c2413play_dma_client, NULL)) {
		printk(KERN_WARNING  "unable to get DMA channel.\n" );
		err = -EBUSY;
		goto out;
		}
		
		err = s3c24xx_iis_dma_init(os,0);
	}
	if (file->f_mode & FMODE_READ) {
	
			if(s3c2413_dma_request(is->dma, &s3c2413rec_dma_client, NULL)) {
			printk(KERN_WARNING  "unable to get DMA channel.\n" );
			err = -EBUSY;
			if (need_tx_dma)
				s3c2413_dma_free(is->dma,&s3c2413play_dma_client);
			goto out;
			}	
		err = s3c24xx_iis_dma_init(is,1);

	}

	/* now complete initialisation */
	if (!AUDIO_ACTIVE(state)) {
		if (state->hw_init)
			state->hw_init(state->data);
	}

	if ((file->f_mode & FMODE_WRITE)) {
		state->wr_ref = 1;
		audio_reset(os);
		os->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		os->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		os->mapped = 0;
		init_waitqueue_head(&os->wq);
	}
	if (file->f_mode & FMODE_READ) {
		state->rd_ref = 1;
		audio_reset(is);
		is->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		is->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		is->mapped = 0;
		init_waitqueue_head(&is->wq);
	}
#endif
	//file->private_data	= state;
	file->f_op->release	= audio_release;
	file->f_op->write	= audio_write;
	file->f_op->read	= audio_read;
	file->f_op->mmap	= audio_mmap;
	file->f_op->poll	= audio_poll;
	file->f_op->ioctl	= audio_ioctl;
	file->f_op->llseek	= audio_llseek;
	err = 0;
	printk("open success \n");

	iorem_base = dma_alloc_coherent(NULL,1*1024*1024,&iorem_phy, GFP_KERNEL);






// iorem_base = ioremap(0x2100000, 0x200000);
 dma_base = ioremap(S3C2413_PA_DMA, 0x200);
temp_base = iorem_base;
printk("temp_base %x iorembase%x \n",temp_base,iorem_base);
	dma_base = dma_base+2*0x40;
	printk("dma base %x\n",dma_base);

#ifdef SHAJU 
	IIS_Port_Init();
	 select_PCLK();
	 Init1341(0);
	set_IIS();
	start_iis();
#endif
//out:
//	up(&state->sem);
	return err;
}

//EXPORT_SYMBOL(elfin_audio_attach);




static struct file_operations elfin_audio_fops = {
	.owner	= THIS_MODULE,
	.open	= elfin_audio_open,
};


static int audio_dev_id, mixer_dev_id;


static struct device_driver elfin_iis_driver = {
    .name       = "elfin-iis",
    .bus        = &platform_bus_type,
    .probe      = NULL,
    .remove     = NULL,
};



int elfin_uda1341_init(void)
{

	audio_dev_id = register_sound_dsp(&elfin_audio_fops, -1);
	//mixer_dev_id = register_sound_mixer(&uda1341_mixer_fops, -1);
 return driver_register(&elfin_iis_driver);
}

static void elfin_uda1341_exit(void)
{
	driver_unregister(&elfin_iis_driver);

}



module_init(elfin_uda1341_init);
module_exit(elfin_uda1341_exit);



MODULE_AUTHOR("Nicolas Pitre");
MODULE_DESCRIPTION("Common audio handling for the S3C24xx processor");
MODULE_LICENSE("GPL");
