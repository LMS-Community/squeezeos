/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef _MXC_IDE_H_
#define _MXC_IDE_H_

/*!
 * @defgroup ATA ATA/IDE Driver
 */

/*!
 * @file mxc_ide.h
 *
 * @brief MXC ATA/IDE hardware register and bit definitions.
 *
 * @ingroup ATA
 */

#define IDE_ARM_IO   IO_ADDRESS((ATA_BASE_ADDR + 0xA0) )
#define IDE_ARM_CTL  IO_ADDRESS((ATA_BASE_ADDR + 0xD8) )
#define IDE_ARM_IRQ  MXC_INT_ATA

/*
 * Interface control registers
 */

#define MXC_IDE_FIFO_DATA_32    IO_ADDRESS((ATA_BASE_ADDR + 0x18) )
#define MXC_IDE_FIFO_DATA_16    IO_ADDRESS((ATA_BASE_ADDR + 0x1C) )
#define MXC_IDE_FIFO_FILL       IO_ADDRESS((ATA_BASE_ADDR + 0x20) )
#define MXC_IDE_ATA_CONTROL     IO_ADDRESS((ATA_BASE_ADDR + 0x24) )
#define MXC_IDE_INTR_PENDING    IO_ADDRESS((ATA_BASE_ADDR + 0x28) )
#define MXC_IDE_INTR_ENABLE     IO_ADDRESS((ATA_BASE_ADDR + 0x2C) )
#define MXC_IDE_INTR_CLEAR      IO_ADDRESS((ATA_BASE_ADDR + 0x30) )
#define MXC_IDE_FIFO_ALARM      IO_ADDRESS((ATA_BASE_ADDR + 0x34) )

/*
 * Control register bit definitions
 */

#define MXC_IDE_CTRL_FIFO_RST_B      0x80
#define MXC_IDE_CTRL_ATA_RST_B       0x40
#define MXC_IDE_CTRL_FIFO_TX_EN      0x20
#define MXC_IDE_CTRL_FIFO_RCV_EN     0x10
#define MXC_IDE_CTRL_DMA_PENDING     0x08
#define MXC_IDE_CTRL_DMA_ULTRA       0x04
#define MXC_IDE_CTRL_DMA_WRITE       0x02
#define MXC_IDE_CTRL_IORDY_EN        0x01

/*
 * Interrupt registers bit definitions
 */

#define MXC_IDE_INTR_ATA_INTRQ1      0x80
#define MXC_IDE_INTR_FIFO_UNDERFLOW  0x40
#define MXC_IDE_INTR_FIFO_OVERFLOW   0x20
#define MXC_IDE_INTR_CTRL_IDLE       0x10
#define MXC_IDE_INTR_ATA_INTRQ2      0x08

/*
 * timing registers
 */

#define MXC_IDE_TIME_OFF        IO_ADDRESS((ATA_BASE_ADDR + 0x00) )
#define MXC_IDE_TIME_ON         IO_ADDRESS((ATA_BASE_ADDR + 0x01) )
#define MXC_IDE_TIME_1          IO_ADDRESS((ATA_BASE_ADDR + 0x02) )
#define MXC_IDE_TIME_2w         IO_ADDRESS((ATA_BASE_ADDR + 0x03) )

#define MXC_IDE_TIME_2r         IO_ADDRESS((ATA_BASE_ADDR + 0x04) )
#define MXC_IDE_TIME_AX         IO_ADDRESS((ATA_BASE_ADDR + 0x05) )
#define MXC_IDE_TIME_PIO_RDX    IO_ADDRESS((ATA_BASE_ADDR + 0x06) )
#define MXC_IDE_TIME_4          IO_ADDRESS((ATA_BASE_ADDR + 0x07) )

#define MXC_IDE_TIME_9          IO_ADDRESS((ATA_BASE_ADDR + 0x08) )
#define MXC_IDE_TIME_M          IO_ADDRESS((ATA_BASE_ADDR + 0x09) )
#define MXC_IDE_TIME_JN         IO_ADDRESS((ATA_BASE_ADDR + 0x0A) )
#define MXC_IDE_TIME_D          IO_ADDRESS((ATA_BASE_ADDR + 0x0B) )

#define MXC_IDE_TIME_K          IO_ADDRESS((ATA_BASE_ADDR + 0x0C) )
#define MXC_IDE_TIME_ACK        IO_ADDRESS((ATA_BASE_ADDR + 0x0D) )
#define MXC_IDE_TIME_ENV        IO_ADDRESS((ATA_BASE_ADDR + 0x0E) )
#define MXC_IDE_TIME_RPX        IO_ADDRESS((ATA_BASE_ADDR + 0x0F) )

#define MXC_IDE_TIME_ZAH        IO_ADDRESS((ATA_BASE_ADDR + 0x10) )
#define MXC_IDE_TIME_MLIX       IO_ADDRESS((ATA_BASE_ADDR + 0x11) )
#define MXC_IDE_TIME_DVH        IO_ADDRESS((ATA_BASE_ADDR + 0x12) )
#define MXC_IDE_TIME_DZFS       IO_ADDRESS((ATA_BASE_ADDR + 0x13) )

#define MXC_IDE_TIME_DVS        IO_ADDRESS((ATA_BASE_ADDR + 0x14) )
#define MXC_IDE_TIME_CVH        IO_ADDRESS((ATA_BASE_ADDR + 0x15) )
#define MXC_IDE_TIME_SS         IO_ADDRESS((ATA_BASE_ADDR + 0x16) )
#define MXC_IDE_TIME_CYC        IO_ADDRESS((ATA_BASE_ADDR + 0x17) )

/*
 * other facts
 */
#define MXC_IDE_FIFO_SIZE	64	/* DMA FIFO size in halfwords */
#define MXC_IDE_DMA_BD_SIZE_MAX 0xFC00	/* max size of scatterlist segment */

/*! Private data for the drive structure. */
typedef struct {
	struct device *dev;	/*!< The device */
	int dma_read_chan;	/*!< DMA channel sdma api gave us for reads */
	int dma_write_chan;	/*!< DMA channel sdma api gave us for writes */
	int ultra;		/*!< Remember when we're in ultra mode */
	int dma_stat;		/*!< the state of DMA request */
	u8 enable;		/*!< Current hardware interrupt mask */
	u8 attached;		/*!< device attached or not */
	struct regulator *regulator_drive;
	struct regulator *regulator_io;
} mxc_ide_private_t;

/*! ATA transfer mode for set_ata_bus_timing() */
enum ata_mode {
	PIO,			/*!< Specifies PIO mode */
	MDMA,			/*!< Specifies MDMA mode */
	UDMA			/*!< Specifies UDMA mode */
};

/*!
 * The struct defines the interrupt type which will issued by interrupt singal.
 */
typedef enum {
	/*! Enable ATA_INTRQ on the CPU */
	INTRQ_MCU,

	/*! Enable ATA_INTRQ on the DMA engine */
	INTRQ_DMA
} intrq_which_t;

/*! defines the structure for timing configurations */

/*!
 *  This structure defines the bits in the ATA TIME_CONFIGx
 */
typedef union {
	unsigned long config;	/* the 32bits fields in TIME_CONFIGx */
	struct {
		unsigned char field1;	/* the 8bits field for 8bits accessing */
		unsigned char field2;	/* the 8bits field for 8bits accessing */
		unsigned char field3;	/* the 8bits field for 8bits accessing */
		unsigned char field4;	/* the 8bits field for 8bits accessing */
	} bytes;		/* the 8bits fields in TIME_CONFIGx */
} mxc_ide_time_cfg_t;

#ifdef MXC_ATA_USE_8_BIT_ACCESS	/* use 8 bit access */

/*!defines the macro for accessing the register */
#define ATA_RAW_WRITE(v, addr)	__raw_writeb(v, addr)
#define ATA_RAW_READ(addr)	__raw_readb(addr)

/*! Get the configuration of TIME_CONFIG0 */
#define GET_TIME_CFG(t, base)
/*! Set the configuration of TIME_CONFIG0.
 * And mask is the mask of valid fields.
 * base is the start address of this cofniguration.
 */
#define SET_TIME_CFG(t, mask, base)	\
	{	\
	if ((mask) & 1) \
		ATA_RAW_WRITE((t)->bytes.field1, (unsigned long)(base)); \
	if ((mask) & 2) \
		ATA_RAW_WRITE((t)->bytes.field2, (unsigned long)(base) + 1); \
	if ((mask) & 4) \
		ATA_RAW_WRITE((t)->bytes.field3, (unsigned long)(base) + 2); \
	if ((mask) & 8) \
		ATA_RAW_WRITE((t)->bytes.field4, (unsigned long)(base) + 3); \
	}

#else				/*MXC_ATA_USE_8_BIT_ACCESS */

/*!defines the macro for accessing the register */
#define ATA_RAW_WRITE(v, addr)	__raw_writel(v, addr)
#define ATA_RAW_READ(addr)	__raw_readl(addr)
/*! Get the configuration of TIME_CONFIG0 */
#define GET_TIME_CFG(t, base)	\
	{	\
	(t)->config = ATA_RAW_READ(base); \
	}

/*! Set the configuration of TIME_CONFIG0.
 * And mask is ignored. base is the start address of this configuration.
 */
#define SET_TIME_CFG(t, mask, base)	\
	{	\
	ATA_RAW_WRITE((t)->config, base);	\
	}
#endif				/*MXC_ATA_USE_8_BIT_ACCESS */

#endif				/* !_MXC_IDE_H_ */
