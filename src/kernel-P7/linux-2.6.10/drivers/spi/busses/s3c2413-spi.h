/* ------------------------------------------------------------------------- */
/* 									     */
/* s3c2413.h - definitions of s3c24xx specific spi interface			     */
/* 									     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2006 Samsung Electronics Co. ltd. 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		     */
/* ------------------------------------------------------------------------- */

#ifndef _S3C24XX_SPI_H
#define _S3C24XX_SPI_H

#include <asm/dma.h>
#include <asm/arch/dma.h>

/* DMA channel to be used for the SPI interface. */
#define S3C2413_SPI0_DMA         1

/* DMA transfer unit (byte). */
#define S3C2413_DMA_XFER_BYTE   1

/* DMA configuration setup byte. */
#define S3C2413_DCON_SPI	(S3C2413_DCON_HANDSHAKE | \
                        	S3C2413_DCON_SYNC_PCLK)

/* DMA hardware configuration mode (DISRCC register). */
#define S3C2413_SPI_DMA_HWCFG    3

#define	DMA_BUFFER_SIZE		1500

/* spi controller state */
int req_dma_flag = 1;
enum s3c24xx_spi_state {
	STATE_IDLE,
	STATE_XFER_TX,
	STATE_XFER_RX,
	STATE_STOP
};

static struct s3c2413_dma_client s3c2413spi_dma_client = {
	.name		= "s3c2413-spi-dma",
};

struct s3c24xx_spi {
	spinlock_t		lock;
	struct semaphore 	sem;
	int 			nr;
	int			dma;
	dma_addr_t		dmabuf_addr;

	struct spi_msg		*msg;
	unsigned int		msg_num;
	unsigned int		msg_idx;
	unsigned int		msg_ptr;
	unsigned int		msg_rd_ptr;

	enum s3c24xx_spi_state	state;

	void __iomem		*regs;
	struct clk		*clk;
	struct device		*dev;
	struct resource		*irq;
	struct resource		*ioarea;
	struct spi_dev		spidev;
};


#endif /* _S3C24XX_SPI_H */

