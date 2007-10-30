/* linux/drivers/spi/busses/spi-s3c2413.c
 *
 * Copyright (C) 2006 Samsung Electronics Co. Ltd.
 *
 * S3C2413 SPI Controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/module.h>

#include "../spi.h"
#include "s3c2413-spi.h"

#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/hardware/clock.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-spi.h>

#undef debug
//#define debug
#ifdef debug
#define DEBUG	printk("%s :: %d\n",__FUNCTION__,__LINE__)
#else
#define DEBUG
#endif

//#define debug
#ifdef debug
void print_reg(struct s3c24xx_spi *spi)
{
	printk("SPCON%d = 0x%x\tAddr = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPCON),(spi->regs + S3C2413_SPCON));
	printk("SPSTA%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPSTA));
	printk("SPPIN%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPPIN));
	printk("SPPRE%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPPRE));
	printk("SPTDAT%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPTDAT));
	printk("SPRDAT%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPRDAT));
	printk("SPTXFIFO%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPTXFIFO));
	printk("SPRXFIFO%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPRXFIFO));
	printk("SPRDATB%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPRDATB));
	printk("SPFIC%d = 0x%x\n",spi->nr,readl(spi->regs + S3C2413_SPFIC));
}
#else
void print_reg(struct s3c24xx_spi *spi)
{
}
#endif

/* s3c24xx_spi_isbus0()
 *
 * return true is this the bus 0 device (spi0)
*/

static inline int s3c24xx_spi_isbus0(struct s3c24xx_spi *spi)
{
	struct platform_device *pdev = to_platform_device(spi->dev);
	DEBUG;

	return pdev->id == 0;
}

void set_nSS_master(int spi_nr)
{
	if(spi_nr == 0){
		s3c2413_gpio_cfgpin(S3C2413_GPG2, S3C2413_GPG2_OUTP);
	} else if(spi_nr == 1){
		s3c2413_gpio_cfgpin(S3C2413_GPG3, S3C2413_GPG3_OUTP);
	}
}

void set_nSS_slave(int spi_nr)
{
	if(spi_nr == 0){
		s3c2413_gpio_cfgpin(S3C2413_GPG2, S3C2413_GPG2_nSS0);
	} else if(spi_nr == 1){
		s3c2413_gpio_cfgpin(S3C2413_GPG3, S3C2413_GPG3_nSS1);
	}
	udelay(100);
}
static void enable_nSS(int spi_nr)
{
	if(spi_nr == 0){
		writel((readl(S3C2413_GPGDAT) & ~(1<<2)), S3C2413_GPGDAT);		
	}else if(spi_nr == 1){
		writel((readl(S3C2413_GPGDAT) & ~(1<<3)), S3C2413_GPGDAT);		
	}
	mdelay(1);
}

static void disable_nSS(int spi_nr)
{
	if(spi_nr == 0){
		writel((readl(S3C2413_GPGDAT) | (1<<2)), S3C2413_GPGDAT);		
	}else if(spi_nr == 1){
		writel((readl(S3C2413_GPGDAT) | (1<<3)), S3C2413_GPGDAT);		
	}
}

static int s3c24xx_spi_init(struct s3c24xx_spi *spi)
{
	DEBUG;

	/* initialise the gpio */
	if (s3c24xx_spi_isbus0(spi))
	{
		/* spi0 */
		s3c2413_gpio_cfgpin(S3C2413_GPE11, S3C2413_GPE11_SPIMISO0);
		s3c2413_gpio_cfgpin(S3C2413_GPE12, S3C2413_GPE12_SPIMOSI0);
		s3c2413_gpio_cfgpin(S3C2413_GPE13, S3C2413_GPE13_SPICLK0);

		set_nSS_master(spi->nr);
		disable_nSS(spi->nr);
	}
	else
	{
		/* spi1 */
		s3c2413_gpio_cfgpin(S3C2413_GPG5, S3C2413_GPG5_SPIMISO1);
		s3c2413_gpio_cfgpin(S3C2413_GPG6, S3C2413_GPG6_SPIMOSI1);
		s3c2413_gpio_cfgpin(S3C2413_GPG7, S3C2413_GPG7_SPICLK1);

		set_nSS_master(spi->nr);
		disable_nSS(spi->nr);
	}

	/* initialise spi registers */
	writel(S3C2413_SPCON_SMOD_POLL | S3C2413_SPCON_MSTR, spi->regs + S3C2413_SPCON);
	writel(0x2, spi->regs + S3C2413_SPPIN);		
	writel(0x9, spi->regs + S3C2413_SPPRE);	
	writel(0, spi->regs + S3C2413_SPFIC);	
	
	print_reg(spi);
	return 0;
}

static void s3c24xx_spi_free(struct s3c24xx_spi *spi)
{
	DEBUG;
	if (spi->clk != NULL && !IS_ERR(spi->clk)) {
		clk_disable(spi->clk);
		clk_unuse(spi->clk);
		clk_put(spi->clk);
		spi->clk = NULL;
	}

	if (spi->regs != NULL) {
		iounmap(spi->regs);
		spi->regs = NULL;
	}

	if (spi->ioarea != NULL) {
		release_resource(spi->ioarea);
		kfree(spi->ioarea);
		spi->ioarea = NULL;
	}
}


static int s3c24xx_spi_dma_init(struct s3c24xx_spi *spi, int mode)
{
	if(mode == 0) //TX 
	{
		s3c2413_dma_devconfig(spi->dma, S3C2413_DMASRC_MEM, 
			S3C2413_SPI_DMA_HWCFG, S3C2413_PA_SPTXFIFO);	

		s3c2413_dma_config(spi->dma, S3C2413_DMA_XFER_BYTE, 
			S3C2413_DCON_SPI, S3C2413_REQSEL_SPI_0_TX | S3C2413_REQSEL_HWTRIG);

        	s3c2413_dma_setflags(spi->dma, S3C2413_DMAF_AUTOSTART);
	}
	if(mode ==1) //RX
	{
		s3c2413_dma_config(spi->dma, S3C2413_DMA_XFER_BYTE, 
			S3C2413_DCON_SPI, S3C2413_REQSEL_SPI_0_RX | S3C2413_REQSEL_HWTRIG);

		s3c2413_dma_devconfig(spi->dma, S3C2413_DMASRC_HW, 
			S3C2413_SPI_DMA_HWCFG, S3C2413_PA_SPRXFIFO);	

        	s3c2413_dma_setflags(spi->dma, S3C2413_DMAF_AUTOSTART);
	}
	return 0;
}

static inline void s3c24xx_setup_fifo(struct s3c24xx_spi *spi)
{
	struct spi_msg *msg = spi->msg;
	unsigned int spfic=0, spcon=0, spsta=0; 


	/* Clear Tx/Rx FIFO Error */
	spsta = readl(spi->regs + S3C2413_SPSTA);
	if(spsta & S3C2413_SPSTA_RXFIFOFERR){
		writel(S3C2413_SPSTA_RXFIFOFERR, (spi->regs + S3C2413_SPSTA));
	}
	if(spsta & S3C2413_SPSTA_TXFIFOEERR){
		writel(S3C2413_SPSTA_TXFIFOEERR, (spi->regs + S3C2413_SPSTA));
	}

	/* Wait till REDY_org bit gets set */
	while(!(readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_REDY_ORG)){}

	/* Enable Rx/Tx FIFO */
	spcon = readl(spi->regs + S3C2413_SPCON);
	if(msg->wbuf)
		spcon |= S3C2413_SPCON_TXFIFOEN;
	if(msg->rbuf)
		spcon |= S3C2413_SPCON_RXFIFOEN;
	writel(spcon, spi->regs + S3C2413_SPCON);

	/* Reset Rx/Tx FIFO */
	if(msg->wbuf)
		spcon |= S3C2413_SPCON_TXFIFORST;
	if(msg->rbuf)
		spcon |= S3C2413_SPCON_RXFIFORST;
	writel(spcon, spi->regs + S3C2413_SPCON);
	while(readl(spi->regs + S3C2413_SPCON) & 
		(S3C2413_SPCON_RXFIFORST | S3C2413_SPCON_TXFIFORST)){}
}

/* is_msgend
 *
 * returns TRUE if we reached the end of the current message
*/

static inline int is_msgend(struct s3c24xx_spi *spi)
{
	return spi->msg_ptr > spi->msg->len;
}

static inline void s3c24xx_spi_xfer_byte(struct s3c24xx_spi *spi)
{
	__u8 byte;

	if (spi->msg->wbuf)
	{
		byte = spi->msg->wbuf[spi->msg_ptr++];		
	}else{
		spi->msg_ptr++;
		byte = 0xff;
	}
	writeb(byte, spi->regs + S3C2413_SPTXFIFO);
}

static void poll_on_fifo(struct s3c24xx_spi *spi)
{
	cli();
	if(spi->msg->wbuf){
		while(!is_msgend(spi)) {
			if((readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFONFULL)
					&&(!is_msgend(spi))){
				s3c24xx_spi_xfer_byte(spi);
			}
			if (readl(spi->regs+S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFOEERR) {
				writel(S3C2413_SPSTA_TXFIFOEERR, (spi->regs + S3C2413_SPSTA));
			}
		}	
		while(!(readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFOEMPTY)){}
	}else{
		while(spi->msg_ptr<spi->msg->len){
			while(readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_RXFIFONEMPTY){
			    spi->msg->rbuf[spi->msg_ptr++] = readb(spi->regs + S3C2413_SPRXFIFO);
			}
			if (readl(spi->regs+S3C2413_SPSTA) & S3C2413_SPSTA_RXFIFOFERR) {
				writel(S3C2413_SPSTA_RXFIFOFERR, (spi->regs + S3C2413_SPSTA));
			}
		}
	}
	sti();
}

/* s3c24xx_spi_master_complete
 *
 * complete the message and wake up the caller, using the given return code,
 * or zero to mean ok.
*/

static inline void s3c24xx_spi_master_complete(struct s3c24xx_spi *spi, int ret)
{
	dev_dbg(spi->dev, "master_complete %d\n", ret);

	spi->msg_ptr = 0;
	spi->msg_rd_ptr = 0;
	spi->msg->flags = 0;
	spi->msg = NULL;
	spi->msg_idx ++;
	spi->msg_num = 0;
	if (ret)
		spi->msg_idx = ret;
}

static inline void s3c24xx_spi_stop(struct s3c24xx_spi *spi, int ret)
{
	unsigned long spcon;
	struct spi_msg *msg = spi->msg;

	DEBUG;
	spcon = S3C2413_SPCON_SMOD_POLL | S3C2413_SPCON_MSTR;

	dev_dbg(spi->dev, "STOP\n");

	writel(spcon, spi->regs + S3C2413_SPCON);
	writel(0, spi->regs + S3C2413_SPFIC);	
	
	if (msg->flags & SPI_M_MODE_MASTER){
		udelay(100);
		disable_nSS(spi->nr);
	} else {
		set_nSS_master(spi->nr);
		disable_nSS(spi->nr);
	}

	spi->state = STATE_IDLE;
	s3c24xx_spi_master_complete(spi, ret);
        up(&spi->sem);
	print_reg(spi);
}

/* s3c24xx_spi_message_start
 *
 * configure the spi controler and transmit start of a message onto the bus 
*/

static void s3c24xx_spi_message_start(struct s3c24xx_spi *spi)
{
	struct spi_msg *msg = spi->msg;
	unsigned long spcon=0, sppin=0, spfic=0;
	int count = 0;
	
	DEBUG;
	if(msg->flags & SPI_M_INT_MODE){
		spcon |= S3C2413_SPCON_SMOD_INT; 
	} else if(msg->flags & SPI_M_POLL_MODE){
		spcon |= S3C2413_SPCON_SMOD_POLL; 
	}else if((msg->flags & SPI_M_DMA_MODE) && !(msg->flags & SPI_M_USE_FIFO)){
		if(msg->wbuf)
			spcon |= S3C2413_SPCON_SMOD_DMA_BUF_TX; 
		if(msg->rbuf)
			spcon |= S3C2413_SPCON_SMOD_DMA_BUF_RX; 
	}

	s3c24xx_setup_fifo(spi);
	if (msg->wbuf){
		spcon |= S3C2413_SPCON_TXFIFOEN; 
		spcon |= S3C2413_SPCON_TXFIFORB_10;	/* FIFO remaining bytes in Tx */ 
		spcon |= S3C2413_SPCON_DIRC_TX;   	/* Direction of Transmission */ 
	}else if (msg->rbuf){
		spcon |= S3C2413_SPCON_RXFIFOEN; 
		spcon |= S3C2413_SPCON_RXFIFORB_2;	/* remaining bytes in Rx */ 
		spcon |= S3C2413_SPCON_DIRC_RX;		/* Direction of Transmission */ 
	}
	
	/* Set FIFO control register */
	spfic = readl(spi->regs + S3C2413_SPFIC);
	if(!(msg->flags & SPI_M_FIFO_POLL)){
		if(msg->wbuf){
			if(msg->flags & SPI_M_DMA_MODE){
				spfic |= S3C2413_SPFIC_TXFIFODMACTL_EMPT;
			}else{
				spfic |= S3C2413_SPFIC_TXFIFOEMIE;
				spfic |= S3C2413_SPFIC_TXFIFOEEIE;
				spfic |= S3C2413_SPFIC_TXFIFOAEIE;
			}
		}else if(msg->rbuf){
			if(msg->flags & SPI_M_DMA_MODE){
				spfic |= S3C2413_SPFIC_RXFIFODMACTL_NEMPT;
			}else{
				spfic |= S3C2413_SPFIC_RXFIFOFLIE;
				spfic |= S3C2413_SPFIC_RXFIFOFEIE;	
				spfic |= S3C2413_SPFIC_RXFIFOAFIE;
			}
		}
	}

	if (msg->flags & SPI_M_DMA_MODE) {
		if (msg->wbuf) {
			s3c24xx_spi_dma_init(spi, 0);
		}else if (spi->msg->rbuf) {
			s3c24xx_spi_dma_init(spi, 1);
		}
	}
	
	if(msg->flags & SPI_M_DMA_MODE){
		s3c2413_dma_enqueue(spi->dma, (void *) spi, spi->dmabuf_addr, spi->msg->len);
	}

	if (msg->flags & SPI_M_MODE_MASTER){
		enable_nSS(spi->nr);
		spcon |= (S3C2413_SPCON_ENSCK | S3C2413_SPCON_MSTR);
		sppin = S3C2413_SPPIN_FDCKEN | S3C2413_SPPIN_KEEP;
	} else {
		set_nSS_slave(spi->nr);
		sppin = S3C2413_SPPIN_CS;
	}	
	
	if (msg->flags & SPI_M_CPOL_ACTLOW)
		spcon |= S3C2413_SPCON_CPOL_LOW;
	
	if (msg->flags & SPI_M_CPHA_FORMATB)
		spcon |= S3C2413_SPCON_CPHA_FMTB;

	if (msg->wbuf == NULL)
		spcon |= S3C2413_SPCON_TAGD;

	writel(spcon, spi->regs + S3C2413_SPCON);
	writel(sppin, spi->regs + S3C2413_SPPIN);
	writel(spfic, spi->regs + S3C2413_SPFIC);

	dev_dbg(spi->dev, "START: %02lx to SPCON\n", spcon);
	print_reg(spi);	
	
	/* Write the first byte of data to the Tx FIFO when in interrupt mode */
	if(msg->flags & SPI_M_FIFO_POLL){
		poll_on_fifo(spi);
		if (msg->wbuf && (msg->flags & SPI_M_MODE_SLAVE)){
        		up(&spi->sem);
			return;
		}
		udelay(100);
		s3c24xx_spi_stop(spi, readl(spi->regs + S3C2413_SPSTA));
	}else if((msg->wbuf) && !(msg->flags & SPI_M_DMA_MODE)){
		s3c24xx_spi_xfer_byte(spi);
	}
}


/* spi_s3c_irq_nextbyte
 *
 * process an interrupt and work out what to do
 */

static void spi_s3c_irq_nextbyte(struct s3c24xx_spi *spi, unsigned long spsta)
{
	struct spi_msg *msg = spi->msg;
	unsigned long spfic=0;

	switch (spi->state) {

	case STATE_IDLE:
		dev_dbg(spi->dev, "%s: called in STATE_IDLE\n", __FUNCTION__);
		break;

	case STATE_STOP:
		udelay(200);
		s3c24xx_spi_stop(spi, 0);
		dev_dbg(spi->dev, "%s: called in STATE_STOP\n", __FUNCTION__);
		break;
	case STATE_XFER_TX:
		/* we are transfering data with the device... check for the
		 * end of the message, and if so, work out what to do
		 */
		while((readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFONFULL)
				&& (!is_msgend(spi))){
			s3c24xx_spi_xfer_byte(spi);
		}
		if (readl(spi->regs+S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFOEERR) {
			writel(S3C2413_SPSTA_TXFIFOEERR, (spi->regs + S3C2413_SPSTA));
		}
		if (is_msgend(spi)) {
			spfic = readl(spi->regs + S3C2413_SPFIC);
			spfic &= ~(S3C2413_SPFIC_TXFIFOAEIE);
			writel(spfic, spi->regs + S3C2413_SPFIC);
			spi->state = STATE_STOP;
		}
		break;
	case STATE_XFER_RX:
		while(readl(spi->regs + S3C2413_SPSTA) & S3C2413_SPSTA_RXFIFONEMPTY){
			spi->msg->rbuf[spi->msg_rd_ptr++] =  readb(spi->regs + S3C2413_SPRXFIFO);

		}
		if(spi->msg_rd_ptr >= spi->msg->len){
			udelay(100);
			s3c24xx_spi_stop(spi, 0);
		}
		if (spsta & S3C2413_SPSTA_RXFIFOFERR) {
			writel(S3C2413_SPSTA_RXFIFOFERR, (spi->regs + S3C2413_SPSTA));
		}
		break;
	default:
		dev_err(spi->dev, "%s: called with Invalid option\n", __FUNCTION__);
	}
	return;
}

/* s3c24xx_spi_irq
 *
 * top level IRQ servicing routine
*/

static irqreturn_t s3c24xx_spi_irq(int irqno, void *dev_id,
				   struct pt_regs *regs)
{
	struct s3c24xx_spi *spi = dev_id;
	unsigned long status;

	status = readl(spi->regs + S3C2413_SPSTA);

	if (status & S3C2413_SPSTA_DCOL) {
		dev_err(spi->dev, "Collision error detected\n");
	}
	/* pretty much this leaves us with the fact that we've
	 * transmitted or received whatever byte we last sent */

	spi_s3c_irq_nextbyte(spi, status);

 	return IRQ_HANDLED;
}

void s3c24xx_spi_dma_cb(s3c2413_dma_chan_t *dma_ch, void *buf_id,
        int size, s3c2413_dma_buffresult_t result)
{
	struct s3c24xx_spi *spi = (struct s3c24xx_spi *)buf_id;
	unsigned long status = 0;
	int count = 0;

	status = readl(spi->regs + S3C2413_SPSTA);
	pr_debug("DMA call back\n");
	if(spi->msg->wbuf)
		while(!(readl(spi->regs +S3C2413_SPSTA) & S3C2413_SPSTA_TXFIFOEMPTY)){}
	s3c24xx_spi_stop(spi, status); 
}

static int s3c24xx_spi_doxfer(struct s3c24xx_spi *spi, struct spi_msg msgs[], int num)
{
	unsigned long timeout,flags;
	int ret;

	spin_lock_irq(&spi->lock);

	spi->msg     = msgs;
	spi->msg_num = num;
	spi->msg_ptr = 0;
	spi->msg_rd_ptr = 0;
	spi->msg_idx = 0;
	if(spi->msg->flags & SPI_M_DMA_MODE){
		spi->dmabuf_addr = spi->spidev.dmabuf;
		pr_debug("spi->dmabuf_addr = 0x%x\n",spi->dmabuf_addr);
	}

	if(spi->msg->wbuf){
		spi->state   = STATE_XFER_TX;
	}else if(spi->msg->rbuf){
		spi->state   = STATE_XFER_RX;
	}else{
		dev_err(spi->dev,"Unknown functionality \n");
		return -ESRCH;
	}

	s3c24xx_spi_message_start(spi);

        down_interruptible(&spi->sem);
	spin_unlock_irq(&spi->lock);
	
	ret = spi->msg_idx;
#if 0
	timeout = wait_event_timeout(spi->wait, spi->msg_num == 0, HZ * spi->spidev.timeout);

	if (timeout == 0)
		dev_err(spi->dev, "timeout\n");
	else if (ret != num)
		dev_err(spi->dev, "incomplete xfer (%d)\n", ret);
#endif
 	return ret;
}

/* s3c24xx_spi_xfer
 *
 * first port of call from the spi bus code when an message needs
 * transfering across the spi bus.
*/

static int s3c24xx_spi_xfer(struct spi_dev *spi_dev,
			struct spi_msg msgs[], int num)
{
	struct s3c24xx_spi *spi = (struct s3c24xx_spi *)spi_dev->algo_data;
	int retry;
	int ret;

	DEBUG;
	for (retry = 0; retry < spi_dev->retries; retry++) {

		ret = s3c24xx_spi_doxfer(spi, msgs, num);

		if (ret != -EAGAIN)
			return ret;

		dev_dbg(spi->dev, "Retrying transmission (%d)\n", retry);

		udelay(100);
	}

	return -EREMOTEIO;
}

/* spi bus registration info */

static struct spi_algorithm s3c24xx_spi_algorithm = {
	.name			= "s3c2413-spi-algorithm",
	.master_xfer		= s3c24xx_spi_xfer,
};

static struct s3c24xx_spi s3c24xx_spi[2] = {
	[0] = {
		.lock	= SPIN_LOCK_UNLOCKED,
		.spidev	= {
			.algo			= &s3c24xx_spi_algorithm,
			.retries		= 2,
			.timeout		= 5,
		}
	},
	[1] = {
		.lock	= SPIN_LOCK_UNLOCKED,
		.spidev	= {
			.algo			= &s3c24xx_spi_algorithm,
			.retries		= 2,
			.timeout		= 5,
		}
	},
};

/* s3c24xx_spi_probe
 *
 * called by the bus driver when a suitable device is found
*/

static int s3c24xx_spi_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s3c24xx_spi *spi = &s3c24xx_spi[pdev->id];		
	struct resource *res;
	int ret;

	DEBUG;
	/* find the clock and enable it */
        sema_init(&spi->sem, 0);		
	spi->nr = pdev->id;
	spi->dev = dev;
	spi->clk = clk_get(dev, "spi");
	if (IS_ERR(spi->clk)) {
		dev_err(dev, "cannot get clock\n");
		ret = -ENOENT;
		goto out;
	}

	dev_dbg(dev, "clock source %p\n", spi->clk);

	clk_use(spi->clk);
	clk_enable(spi->clk);

	/* map the registers */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto out;
	}

	spi->ioarea = request_mem_region(res->start, (res->end-res->start)+1,
					 pdev->name);

	if (spi->ioarea == NULL) {
		dev_err(dev, "cannot request IO\n");
		ret = -ENXIO;
		goto out;
	}

	spi->regs = ioremap(res->start, (res->end-res->start)+1);

	if (spi->regs == NULL) {
		dev_err(dev, "cannot map IO\n");
		ret = -ENXIO;
		goto out;
	}

	dev_dbg(dev, "registers %p (%p, %p)\n", spi->regs, spi->ioarea, res);

	/* setup info block for the spi core */

	spi->spidev.algo_data = spi;
	spi->spidev.dev.parent = dev;
	spi->spidev.minor = spi->nr;
	init_MUTEX(&spi->spidev.bus_lock);

	/* initialise the spi controller */

	ret = s3c24xx_spi_init(spi);
	if (ret != 0)
		goto out;

	/* find the IRQ for this unit (note, this relies on the init call to
	 * ensure no current IRQs pending 
	 */

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(dev, "cannot find IRQ\n");
		ret = -ENOENT;
		goto out;
	}

	ret = request_irq(res->start, s3c24xx_spi_irq, SA_INTERRUPT,
			  pdev->name, spi);

	if (ret != 0) {
		dev_err(dev, "cannot claim IRQ\n");
		goto out;
	}

	spi->irq = res;
		
	dev_dbg(dev, "irq resource %p (%ld)\n", res, res->start);

	/* Need to call this only once */
	if (req_dma_flag){
		if (s3c2413_dma_request(S3C2413_SPI0_DMA, &s3c2413spi_dma_client, NULL)) {
		    dev_err(dev, "cannot get DMA channel %d\n", S3C2413_SPI0_DMA);
		    	ret = -EBUSY;
			goto out;
		}
		s3c2413_dma_set_buffdone_fn(S3C2413_SPI0_DMA, s3c24xx_spi_dma_cb);
		s3c2413_dma_set_opfn(S3C2413_SPI0_DMA, NULL);
		req_dma_flag = 0;
	}
	spi->dma = S3C2413_SPI0_DMA;

	ret = spi_attach_spidev(&spi->spidev);		
	if (ret < 0) {
		dev_err(dev, "failed to add adapter to spi core\n");
		goto out;
	}

	dev_set_drvdata(dev, spi);	

	dev_info(dev, "%s: S3C SPI adapter\n", spi->dev->bus_id);

 out:
	if (ret < 0)
		s3c24xx_spi_free(spi);

	return ret;
}

/* s3c24xx_spi_remove
 *
 * called when device is removed from the bus
*/

static int s3c24xx_spi_remove(struct device *dev)
{
	struct s3c24xx_spi *spi = dev_get_drvdata(dev);
	
	DEBUG;
	if (spi != NULL) {
		spi_detach_spidev(&spi->spidev);
		s3c24xx_spi_free(spi);
		dev_set_drvdata(dev, NULL);	
	}

	return 0;
}

/* device driver for platform bus bits */

static struct device_driver s3c24xx_spi_driver = {
	.name		= "s3c2413-spi",
	.bus		= &platform_bus_type,
	.probe		= s3c24xx_spi_probe,
	.remove		= s3c24xx_spi_remove,
};
static char banner[] __initdata = KERN_INFO "Samsung s3c24xx SPI drivers, (c) 2006 Samsung Electronics Co Ltd.\n";

static int __init spi_adap_s3c_init(void)
{
	printk(banner);

	return driver_register(&s3c24xx_spi_driver);
}

static void __exit spi_adap_s3c_exit(void)
{
	driver_unregister(&s3c24xx_spi_driver);
}

module_init(spi_adap_s3c_init);
module_exit(spi_adap_s3c_exit);

MODULE_DESCRIPTION("S3C2413 SPI Bus driver");
MODULE_AUTHOR("Banajit Goswami, <banajit.g@samsung.com>");
MODULE_LICENSE("GPL");
