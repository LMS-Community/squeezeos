/* linux/include/asm-arm/arch-s3c2413/regs-spi.h
 *
 * Copyright (c) 2004 Fetron GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2413 SPI register definition
 *
 *  Changelog:
 *    20-04-2004     KF      Created file
 *    04-10-2004     BJD     Removed VA address (no longer mapped)
 *			     tidied file for submission
 */

#ifndef __ASM_ARCH_REGS_SPI_H
#define __ASM_ARCH_REGS_SPI_H


#define S3C2413_SPCON	(0x00)

#define S3C2413_SPCON_RXFIFORB_14	(3<<14)	/* Rx Fifo Remaining Bytes -- 14 */
#define S3C2413_SPCON_RXFIFORB_10	(2<<14)	/* Rx Fifo Remaining Bytes -- 10 */
#define S3C2413_SPCON_RXFIFORB_6	(1<<14)	/* Rx Fifo Remaining Bytes -- 6 */
#define S3C2413_SPCON_RXFIFORB_2	(0<<14)	/* Rx Fifo Remaining Bytes -- 2 */

#define S3C2413_SPCON_TXFIFORB_14	(3<<12)	/* Tx Fifo Remaining Bytes -- 14 */
#define S3C2413_SPCON_TXFIFORB_10	(2<<12)	/* Tx Fifo Remaining Bytes -- 10 */
#define S3C2413_SPCON_TXFIFORB_6	(1<<12)	/* Tx Fifo Remaining Bytes -- 6 */
#define S3C2413_SPCON_TXFIFORB_2	(0<<12)	/* Tx Fifo Remaining Bytes -- 2 */

#define S3C2413_SPCON_RXFIFORST		(1<<11)	/* Rx Fifo Reset */
#define S3C2413_SPCON_TXFIFORST		(1<<10)	/* Tx Fifo Reset */

#define S3C2413_SPCON_RXFIFOEN		(1<<9)	/* RX Fifo Enable */
#define S3C2413_SPCON_TXFIFOEN		(1<<8)	/* Tx Fifo Enable */

#define S3C2413_SPCON_DIRC_TX		(0<<7)	/* Transfer Direction Select -- Tx */
#define S3C2413_SPCON_DIRC_RX		(1<<7)	/* Transfer Direction Select -- Rx */

#define S3C2413_SPCON_SMOD_DMA_BUF_RX 	(3<<5)	/* Buffer DMA Rx mode */
#define S3C2413_SPCON_SMOD_DMA_BUF_TX 	(2<<5)	/* Buffer DMA Tx mode */
#define S3C2413_SPCON_SMOD_INT	  	(1<<5)	/* interrupt mode */
#define S3C2413_SPCON_SMOD_POLL   	(0<<5)	/* polling mode */

#define S3C2413_SPCON_DISSCK	  	(0<<4)	/* Disable SCK */
#define S3C2413_SPCON_ENSCK	  	(1<<4)	/* Enable SCK */

#define S3C2413_SPCON_SLV	  	(0<<3)	/* Master/Slave select
						   0: slave, 1: master */
#define S3C2413_SPCON_MSTR	  	(1<<3)	/* Master/Slave select
						   0: slave, 1: master */
#define S3C2413_SPCON_CPOL_HIGH	  	(0<<2)	/* Clock polarity select */
#define S3C2413_SPCON_CPOL_LOW	  	(1<<2)	/* Clock polarity select */

#define S3C2413_SPCON_CPHA_FMTA	  	(0<<1)	/* Clock Phase Select */
#define S3C2413_SPCON_CPHA_FMTB	  	(1<<1)	/* Clock Phase Select */

#define S3C2413_SPCON_NRML	  	(0<<0)	/* Tx normal data mode */
#define S3C2413_SPCON_TAGD	  	(1<<0)	/* Tx auto garbage data mode */


#define S3C2413_SPSTA	 (0x04)

#define S3C2413_SPSTA_RXFIFOAF 		(1<<11)	/* Rx FIFO Almost Full */
#define S3C2413_SPSTA_TXFIFOAE	 	(1<<10)	/* Tx FIFO Almost Empty */
#define S3C2413_SPSTA_RXFIFOFERR 	(1<<9)	/* Rx FIFO Full Error */
#define S3C2413_SPSTA_TXFIFOEERR 	(1<<8)	/* Tx FIFO Empty Error */
#define S3C2413_SPSTA_RXFIFOFULL 	(1<<7)	/* Rx FIFO Full */
#define S3C2413_SPSTA_RXFIFONEMPTY 	(1<<6)	/* Rx FIFO Not Empty */
#define S3C2413_SPSTA_RXFIFOEMPTY 	(0<<6)	/* Rx FIFO Empty */
#define S3C2413_SPSTA_TXFIFONFULL 	(1<<5)	/* Tx FIFO Not Full */
#define S3C2413_SPSTA_TXFIFOEMPTY 	(1<<4)	/* Tx FIFO Empty */
#define S3C2413_SPSTA_REDY_ORG	  	(1<<3)	/* Data Pre Tx/Rx ready */
#define S3C2413_SPSTA_DCOL	  	(1<<2)	/* Data Collision Error */
#define S3C2413_SPSTA_RESERVED	  	(0<<1)  /* Reserved */
#define S3C2413_SPSTA_READY	  	(1<<0)	/* Data Tx/Rx ready */


#define S3C2413_SPPIN	 (0x08)

#define S3C2413_SPPIN_FDCKEN	  	(1<<3)	/* Feedback Clock Enable */
#define S3C2413_SPPIN_ENMUL	  	(1<<2)	/* Multi Master Error detect */
#define S3C2413_SPPIN_CS		(1<<1)  /* Chip Select Output 
						0: CS active, 1: CS inactive*/
#define S3C2413_SPPIN_KEEP	  	(1<<0)	/* Master Out keep */


#define S3C2413_SPPRE	 (0x0C)
#define S3C2413_SPTDAT	 (0x10)
#define S3C2413_SPRDAT	 (0x14)

#define S3C2413_SPTXFIFO (0x18)
#define S3C2413_SPRXFIFO (0x1C)
#define S3C2413_SPRDATB	 (0x20)

#define S3C2413_SPFIC	 (0x24)

#define S3C2413_SPFIC_RXFIFODMACTL_ALMF  (2<<10) /* Rx FIFO DMA control -- FIFO almost full */
#define S3C2413_SPFIC_RXFIFODMACTL_NEMPT (1<<10) /* Rx FIFO DMA control -- FIFO not empty */
#define S3C2413_SPFIC_RXFIFODMACTL_DIS 	 (0<<10) /* Rx FIFO DMA control -- FIFO disable */

#define S3C2413_SPFIC_TXFIFODMACTL_ALME	 (2<<8)  /* Tx FIFO DMA control -- FIFO almost empty */
#define S3C2413_SPFIC_TXFIFODMACTL_EMPT  (1<<8)  /* Tx FIFO DMA control -- FIFO empty   */
#define S3C2413_SPFIC_TXFIFODMACTL_DIS	 (0<<8)  /* Tx FIFO DMA control -- FIFO disable */

#define S3C2413_SPFIC_RESERVED_7  	(0<<7)  /* Reserved */
#define S3C2413_SPFIC_RESERVED_6  	(0<<6)  /* Reserved */

#define S3C2413_SPFIC_RXFIFOAFIE	(1<<5)  /* Rx FIFO almost full interrupt enable */
#define S3C2413_SPFIC_TXFIFOAEIE	(1<<4)  /* Tx FIFO almost empty interrupt enable */

#define S3C2413_SPFIC_RXFIFOFEIE	(1<<3)  /* Rx FIFO full error interrupt enable */
#define S3C2413_SPFIC_TXFIFOEEIE	(1<<2)  /* Tx FIFO empty error interrupt enable */

#define S3C2413_SPFIC_RXFIFOFLIE	(1<<1)  /* Rx FIFO full interrupt enable */
#define S3C2413_SPFIC_TXFIFOEMIE	(1<<0)  /* Tx FIFO empty interrupt enable */

#define S3C2413_PA_SPTDAT	 0x59000010
#define S3C2413_PA_SPRDAT	 0x59000014
#define S3C2413_PA_SPTXFIFO	 0x59000018
#define S3C2413_PA_SPRXFIFO	 0x5900001C
#define S3C2413_PA_SPRDATB	 0x59000020

#endif /* __ASM_ARCH_REGS_SPI_H */
