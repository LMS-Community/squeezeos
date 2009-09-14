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

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/flash.h>
#include <asm/io.h>
#include "mxc_nd2.h"

#define DVR_VER "2.4"

/* Global address Variables */
static u32 nfc_axi_base, nfc_ip_base;

struct mxc_mtd_s {
	struct mtd_info mtd;
	struct nand_chip nand;
	struct mtd_partition *parts;
	struct device *dev;
};

static struct mxc_mtd_s *mxc_nand_data;

/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY   2000

struct nand_info {
	bool bSpareOnly;
	bool bStatusRequest;
	u16 colAddr;
};

static struct nand_info g_nandfc_info;

#ifdef CONFIG_MTD_NAND_MXC_SWECC
static int hardware_ecc = 0;
#else
static int hardware_ecc = 1;
#endif

static int g_page_mask;

static struct clk *nfc_clk;

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_512 = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{0, 4}}
};

static struct nand_ecclayout nand_hw_eccoob_2k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{2, 4}}
};

static struct nand_ecclayout nand_hw_eccoob_4k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{2, 4}}
};

/*!
 * @defgroup NAND_MTD NAND Flash MTD Driver for MXC processors
 */

/*!
 * @file mxc_nd2.c
 *
 * @brief This file contains the hardware specific layer for NAND Flash on
 * MXC processor
 *
 * @ingroup NAND_MTD
 */

#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "RedBoot", "cmdlinepart", NULL };
#endif

static wait_queue_head_t irq_waitq;

static irqreturn_t mxc_nfc_irq(int irq, void *dev_id)
{
	/* Disable Interuupt */
	raw_write(raw_read(REG_NFC_INTRRUPT) | NFC_INT_MSK, REG_NFC_INTRRUPT);
	wake_up(&irq_waitq);

	return IRQ_HANDLED;
}

static u8 mxc_main_xfer_buf[4096] ____cacheline_aligned;

/*
 * Functions to handle 32-bit aligned memcpy.
 */
static void nfc_memcpy(void *dst, const void *src, int len)
{
	volatile u16 *d = (volatile u16 *)dst;
	volatile u16 *s = (volatile u16 *)src;
	int wc;

	switch ((u32) dst & 3) {
	case 2:
		wc = len / 2;
		/* adjust alignment */
		*d = *s;
		memcpy((void *)(d + 1), (const void *)(s + 1), len - 4);
		*(d + wc - 1) = *(s + wc - 1);
		break;

	case 1:
	case 3:
		memcpy((void *)mxc_main_xfer_buf, (const void *)src,
		       (len + 3) & (~3));
		memcpy((void *)d, (const void *)mxc_main_xfer_buf, len);
		break;
	case 0:
		memcpy((void *)d, (const void *)s, len);
	}
}

/*
 * Functions to transfer data to/from spare erea.
 */
static void
copy_spare(struct mtd_info *mtd, void *pbuf, void *pspare, int len, bool bfrom)
{
	u16 ooblen = mtd->oobsize;
	u8 i, count, size;

	count = mtd->writesize >> 9;
	size = (ooblen / count >> 1) << 1;

	if (bfrom) {
		for (i = 0; i < count - 1; i++)
			nfc_memcpy((pbuf + i * size), (pspare + i * SPARE_LEN),
				   size);

		nfc_memcpy((pbuf + i * size), (pspare + i * SPARE_LEN),
			   len - i * size);
	} else {
		for (i = 0; i < count - 1; i++)
			nfc_memcpy((pspare + i * SPARE_LEN), (pbuf + i * size),
				   size);

		nfc_memcpy((pspare + i * SPARE_LEN), (pbuf + i * size),
			   len - i * size);
	}
}

/*!
 * This function polls the NFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @param       maxRetries     number of retry attempts (separated by 1 us)
 * @param       useirq         True if IRQ should be used rather than polling
 */
static void wait_op_done(int maxRetries, bool useirq)
{

	if (useirq) {
		if ((raw_read(REG_NFC_OPS_STAT) & NFC_OPS_STAT) == 0) {
			/* Enable Interuupt */
			raw_write(raw_read(REG_NFC_INTRRUPT) & ~NFC_INT_MSK,
				  REG_NFC_INTRRUPT);
			wait_event(irq_waitq,
				   (raw_read(REG_NFC_OPS_STAT) & NFC_OPS_STAT));
			WRITE_NFC_IP_REG((raw_read(REG_NFC_OPS_STAT) &
					  ~NFC_OPS_STAT), REG_NFC_OPS_STAT);
		}
	} else {
		while (1) {
			maxRetries--;
			if (raw_read(REG_NFC_OPS_STAT) & NFC_OPS_STAT) {
				WRITE_NFC_IP_REG((raw_read(REG_NFC_OPS_STAT) &
						  ~NFC_OPS_STAT),
						 REG_NFC_OPS_STAT);
				break;
			}
			udelay(1);
		}
		if (maxRetries <= 0) {
			DEBUG(MTD_DEBUG_LEVEL0, "%s(%d): INT not set\n",
			      __FUNCTION__, __LINE__);
		}
	}
}

static void send_addr(u16 addr, bool useirq);
/*!
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_cmd(u16 cmd, bool useirq)
{
	DEBUG(MTD_DEBUG_LEVEL3, "send_cmd(0x%x, %d)\n", cmd, useirq);

#ifdef NFC_AUTO_MODE_ENABLE
	switch (cmd) {
	case NAND_CMD_READ0:
	case NAND_CMD_READOOB:
		raw_write(NAND_CMD_READ0, REG_NFC_FLASH_CMD);
		break;

	case NAND_CMD_SEQIN:
	case NAND_CMD_ERASE1:
		raw_write(cmd, REG_NFC_FLASH_CMD);
		break;

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE2:
	case NAND_CMD_READSTART:
		raw_write(raw_read(REG_NFC_FLASH_CMD) | cmd << NFC_CMD_1_SHIFT,
			  REG_NFC_FLASH_CMD);

		if (cmd == NAND_CMD_ERASE2) {
			raw_write(NFC_AUTO_ERASE, REG_NFC_OPS);
		} else if (cmd == NAND_CMD_PAGEPROG) {
			NFC_SET_RBA(0);
			raw_write(NFC_AUTO_PROG, REG_NFC_OPS);
		} else if (cmd == NAND_CMD_READSTART) {
			NFC_SET_RBA(0);
			raw_write(NFC_AUTO_READ, REG_NFC_OPS);
		}

		wait_op_done(TROP_US_DELAY, useirq);
		break;

	case NAND_CMD_READID:
		raw_write(cmd, REG_NFC_FLASH_CMD);
		ACK_OPS;
		raw_write(NFC_CMD, REG_NFC_OPS);
		wait_op_done(TROP_US_DELAY, useirq);
		send_addr(0, false);
		break;

	case NAND_CMD_STATUS:
		raw_write(NFC_AUTO_STATE, REG_NFC_OPS);
		break;
	}
	DEBUG(MTD_DEBUG_LEVEL3, "AutoMode:CMD REG value is 0x%x \n",
	      raw_read(REG_NFC_FLASH_CMD));
#else
	raw_write(cmd, REG_NFC_FLASH_CMD);
	ACK_OPS;
	raw_write(NFC_CMD, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, useirq);
#endif
}

/*!
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_addr(u16 addr, bool useirq)
{
	DEBUG(MTD_DEBUG_LEVEL3, "send_addr(0x%x %d)\n", addr, useirq);
	raw_write((addr << NFC_FLASH_ADDR_SHIFT), REG_NFC_FLASH_ADDR);

	ACK_OPS;		/* defined only for V3 */
	raw_write(NFC_ADDR, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, useirq);
}

/*!
 * This function requests the NFC to initate the transfer
 * of data currently in the NFC RAM buffer to the NAND device.
 *
 * @param	buf_id	      Specify Internal RAM Buffer number (0-3)
 */
static void send_prog_page(u8 buf_id)
{
#ifndef NFC_AUTO_MODE_ENABLE
	DEBUG(MTD_DEBUG_LEVEL3, "%s\n", __FUNCTION__);

	NFC_SET_RBA(buf_id);

	ACK_OPS;		/* defined only for V3 */

	raw_write(NFC_INPUT, REG_NFC_OPS);
	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, true);
#endif
}

/*!
 * This function requests the NFC to initated the transfer
 * of data from the NAND device into in the NFC ram buffer.
 *
 * @param  	buf_id		Specify Internal RAM Buffer number (0-3)
 */
static void send_read_page(u8 buf_id)
{
#ifndef NFC_AUTO_MODE_ENABLE
	DEBUG(MTD_DEBUG_LEVEL3, "%s(%d)\n", __FUNCTION__, buf_id);

	NFC_SET_RBA(buf_id);

	ACK_OPS;		/* defined only for V3 */

	raw_write(NFC_OUTPUT, REG_NFC_OPS);
	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, true);
#endif
}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(void)
{
	u8 val = 0;

	/* NFC buffer 0 is used for device ID output */
	/* Set RBA bits for BUFFER0 */
	NFC_SET_RBA(val);

	/* defined only for V3 */
	ACK_OPS;

	/* Read ID into main buffer */
	raw_write(NFC_ID, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, true);

}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(void)
{
#ifdef NFC_AUTO_MODE_ENABLE
	u16 status;
	do {
		status = (raw_read(NFC_CONFIG1) & 0x00FF0000) >> 16;
	} while ((status & 0x40) == 0);
	return status;
#else
	volatile u16 *mainBuf = MAIN_AREA1;
	u8 val = 1;
	u16 ret;

	/* Set RBA bits for BUFFER1 */
	NFC_SET_RBA(val);

	/* defined only for V3 */
	ACK_OPS;
	/* Read status into main buffer */
	raw_write(NFC_STATUS, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, true);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	ret = *mainBuf;

	return ret;
#endif
}

/*!
 * This functions is used by upper layer to checks if device is ready
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return  0 if device is busy else 1
 */
static int mxc_nand_dev_ready(struct mtd_info *mtd)
{
	/*
	 * For V1/V2 NFC this function returns always  1.
	 */
	if (CHECK_NFC_RB)
		return 1;
	else
		return 0;
}

static void mxc_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	raw_write((raw_read(REG_NFC_ECC_EN) | NFC_ECC_EN), REG_NFC_ECC_EN);
	return;
}

/*
 * Function to record the ECC corrected/uncorrected errors resulted
 * after a page read. This NFC detects and corrects upto to 4 symbols
 * of 9-bits each.
 */
static int mxc_check_ecc_status(struct mtd_info *mtd)
{
	u32 ecc_stat, err;
	int no_subpages = 1;
	int ret = 0;
	u8 ecc_bit_mask, err_limit;

	ecc_bit_mask = (IS_4BIT_ECC ? 0x7 : 0xf);
	err_limit = (IS_4BIT_ECC ? 0x4 : 0x8);

	no_subpages = mtd->writesize >> 9;

	ecc_stat = GET_NFC_ECC_STATUS();
	do {
		err = ecc_stat & ecc_bit_mask;
		if (err > err_limit) {
			return -1;
		} else {
			ret += err;
		}
		ecc_stat >>= 4;
	} while (--no_subpages);

	return ret;
}

/*
 * Function to correct the detected errors. This NFC corrects all the errors
 * detected. So this function is not required.
 */
static int mxc_nand_correct_data(struct mtd_info *mtd, u_char * dat,
				 u_char * read_ecc, u_char * calc_ecc)
{
	panic("Shouldn't be called here: %d\n", __LINE__);
	return 0;		//FIXME
}

/*
 * Function to calculate the ECC for the data to be stored in the Nand device.
 * This NFC has a hardware RS(511,503) ECC engine together with the RS ECC
 * CONTROL blocks are responsible for detection  and correction of up to
 * 4 symbols of 9 bits each in 528 byte page.
 * So this function is not required.
 */

static int mxc_nand_calculate_ecc(struct mtd_info *mtd, const u_char * dat,
				  u_char * ecc_code)
{
	panic(KERN_ERR "Shouldn't be called here %d \n", __LINE__);
	return 0;		//FIXME
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char mxc_nand_read_byte(struct mtd_info *mtd)
{
	u_char retVal = 0;
	u16 col, rdWord;
	volatile u16 *mainBuf = MAIN_AREA0;
	volatile u16 *spareBuf = SPARE_AREA0;

	/* Check for status request */
	if (g_nandfc_info.bStatusRequest) {
		return (get_dev_status() & 0xFF);
	}

	/* Get column for 16-bit access */
	col = g_nandfc_info.colAddr >> 1;

	/* If we are accessing the spare region */
	if (g_nandfc_info.bSpareOnly) {
		rdWord = spareBuf[col];
	} else {
		rdWord = mainBuf[col];
	}

	/* Pick upper/lower byte of word from RAM buffer */
	if (g_nandfc_info.colAddr & 0x1) {
		retVal = (rdWord >> 8) & 0xFF;
	} else {
		retVal = rdWord & 0xFF;
	}

	/* Update saved column address */
	g_nandfc_info.colAddr++;

	return retVal;
}

/*!
  * This function reads word from the NAND Flash
  *
  * @param     mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 mxc_nand_read_word(struct mtd_info *mtd)
{
	u16 col, rdWord;
	volatile u16 *mainBuf = MAIN_AREA0;
	volatile u16 *spareBuf = SPARE_AREA0;

	/* Get column for 16-bit access */
	col = g_nandfc_info.colAddr >> 1;

	/* If we are accessing the spare region */
	if (g_nandfc_info.bSpareOnly) {
		rdWord = spareBuf[col];
	} else {
		rdWord = mainBuf[col];
	}

	/* Update saved column address */
	g_nandfc_info.colAddr += 2;

	return rdWord;
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @param     mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char mxc_nand_read_byte16(struct mtd_info *mtd)
{
	/* Check for status request */
	if (g_nandfc_info.bStatusRequest) {
		return (get_dev_status() & 0xFF);
	}

	return mxc_nand_read_word(mtd) & 0xFF;
}

/*!
 * This function writes data of length \b len from buffer \b buf to the NAND
 * internal RAM buffer's MAIN area 0.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void mxc_nand_write_buf(struct mtd_info *mtd,
			       const u_char * buf, int len)
{
	volatile uint32_t *base;
	panic("re-work needed\n");
	if (g_nandfc_info.colAddr >= mtd->writesize || g_nandfc_info.bSpareOnly) {
		base = (uint32_t *) SPARE_AREA0;
		copy_spare(mtd, (char *)buf, (char *)base, len, false);
		return;
	} else {
		g_nandfc_info.colAddr += len;
		base = (uint32_t *) MAIN_AREA0;
	}
	memcpy((void *)base, (void *)buf, len);
}

/*!
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void mxc_nand_read_buf(struct mtd_info *mtd, u_char * buf, int len)
{
	volatile uint32_t *base;

	if (g_nandfc_info.colAddr >= mtd->writesize || g_nandfc_info.bSpareOnly) {
		base = (uint32_t *) SPARE_AREA0;
		copy_spare(mtd, buf, (char *)base, len, true);
		return;
	} else {
		base = (uint32_t *) MAIN_AREA0;
		g_nandfc_info.colAddr += len;
	}
	nfc_memcpy((void *)buf, (void *)base, len);
}

/*!
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be verified
 * @param       len     length of the data to be verified
 *
 * @return      -EFAULT if error else 0
 *
 */
static int mxc_nand_verify_buf(struct mtd_info *mtd, const u_char * buf,
			       int len)
{
	volatile u32 *mainBuf = (u32 *) MAIN_AREA0;
	/* check for 32-bit alignment? */
	uint32_t *p = (uint32_t *) buf;

	for (; len > 0; len -= 4) {
		if (*p++ != *mainBuf++) {
			return -EFAULT;
		}
	}

	return 0;
}

/*!
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void mxc_nand_select_chip(struct mtd_info *mtd, int chip)
{
#ifdef CONFIG_MTD_NAND_MXC_FORCE_CE
	if (chip > 0) {
		DEBUG(MTD_DEBUG_LEVEL0,
		      "ERROR:  Illegal chip select (chip = %d)\n", chip);
		return;
	}

	if (chip == -1) {
		raw_write((raw_read(REG_NFC_CE) & ~NFC_CE), REG_NFC_CE);
		return;
	}

	raw_write((raw_read(REG_NFC_CE) | NFC_CE), REG_NFC_CE);

#endif

	switch (chip) {
	case -1:
		/* Disable the NFC clock */
		clk_disable(nfc_clk);
		break;
	case 0:
		/* Enable the NFC clock */
		clk_enable(nfc_clk);
		break;

	default:
		break;
	}
}

/*
 * Function to perform the address cycles.
 */
static void mxc_do_addr_cycle(struct mtd_info *mtd, int column, int page_addr)
{
#ifdef NFC_AUTO_MODE_ENABLE

	if (page_addr != -1 && column != -1) {
		/* the column address */
		raw_write(column & 0x0000FFFF, NFC_FLASH_ADDR0);
		raw_write((raw_read(NFC_FLASH_ADDR0) |
			   ((page_addr & 0x0000FFFF) << 16)), NFC_FLASH_ADDR0);
		/* the row address */
		raw_write(((raw_read(NFC_FLASH_ADDR8) & 0xFFFF0000) |
			   ((page_addr & 0xFFFF0000) >> 16)), NFC_FLASH_ADDR8);
	} else if (page_addr != -1) {
		raw_write(page_addr, NFC_FLASH_ADDR0);
	}

	DEBUG(MTD_DEBUG_LEVEL3,
	      "AutoMode:the ADDR REGS value is (0x%x, 0x%x)\n",
	      raw_read(NFC_FLASH_ADDR0), raw_read(NFC_FLASH_ADDR8));
#else

	u32 page_mask = g_page_mask;

	if (column != -1) {
		send_addr(column & 0xFF, false);
		if (IS_2K_PAGE_NAND) {
			/* another col addr cycle for 2k page */
			send_addr((column >> 8) & 0xF, false);
		} else if (IS_4K_PAGE_NAND) {
			/* another col addr cycle for 4k page */
			send_addr((column >> 8) & 0x1F, false);
		}
	}
	if (page_addr != -1) {
		do {
			send_addr((page_addr & 0xff), false);
			page_mask >>= 8;
			page_addr >>= 8;
		} while (page_mask != 0);
	}
#endif
}

/*
 * Function to read a page from nand device.
 */
static void read_full_page(struct mtd_info *mtd, int page_addr)
{
	send_cmd(NAND_CMD_READ0, false);

	mxc_do_addr_cycle(mtd, 0, page_addr);

	if (IS_LARGE_PAGE_NAND) {
		send_cmd(NAND_CMD_READSTART, false);
		READ_PAGE();
	} else {
		send_read_page(0);
	}
}

/*!
 * This function is used by the upper layer to write command to NAND Flash for
 * different operations to be carried out on NAND Flash
 *
 * @param       mtd             MTD structure for the NAND Flash
 * @param       command         command for NAND Flash
 * @param       column          column offset for the page read
 * @param       page_addr       page to be read from NAND Flash
 */
static void mxc_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	bool useirq = true;

	DEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
	      command, column, page_addr);
	/*
	 * Reset command state information
	 */
	g_nandfc_info.bStatusRequest = false;

	/* Reset column address to 0 */
	g_nandfc_info.colAddr = 0;

	/*
	 * Command pre-processing step
	 */
	switch (command) {
	case NAND_CMD_STATUS:
		g_nandfc_info.bStatusRequest = true;
		break;

	case NAND_CMD_READ0:
		g_nandfc_info.bSpareOnly = false;
		useirq = false;
		break;

	case NAND_CMD_READOOB:
		g_nandfc_info.colAddr = column;
		g_nandfc_info.bSpareOnly = true;
		useirq = false;
		command = NAND_CMD_READ0;	/* only READ0 is valid */
		break;

	case NAND_CMD_SEQIN:
		if (column >= mtd->writesize) {
			g_nandfc_info.bSpareOnly = true;
		} else {
			g_nandfc_info.bSpareOnly = false;
		}
		useirq = false;
		break;

	case NAND_CMD_PAGEPROG:
		if (!g_nandfc_info.bSpareOnly) {
			if (IS_LARGE_PAGE_NAND) {
				PROG_PAGE();
			} else {
				send_prog_page(0);
			}
		} else {
			return;
		}
		break;

	case NAND_CMD_ERASE1:
		useirq = false;
		break;
	case NAND_CMD_ERASE2:
		useirq = false;
		break;
	}

	/*
	 * Write out the command to the device.
	 */
	send_cmd(command, useirq);

	mxc_do_addr_cycle(mtd, column, page_addr);

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (IS_LARGE_PAGE_NAND) {
			/* send read confirm command */
			send_cmd(NAND_CMD_READSTART, true);
			/* read for each AREA */
			READ_PAGE();
		} else {
			send_read_page(0);
		}
		break;

	case NAND_CMD_READID:
		send_read_id();
		break;
	}
}

#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
static void mxc_low_erase(struct mtd_info *mtd)
{

	struct nand_chip *this = mtd->priv;
	unsigned int page_addr, addr;
	u_char status;

	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : mxc_low_erase:Erasing NAND\n");
	for (addr = 0; addr < this->chipsize; addr += mtd->erasesize) {
		page_addr = addr / mtd->writesize;
		mxc_nand_command(mtd, NAND_CMD_ERASE1, -1, page_addr);
		mxc_nand_command(mtd, NAND_CMD_ERASE2, -1, -1);
		mxc_nand_command(mtd, NAND_CMD_STATUS, -1, -1);
		status = mxc_nand_read_byte(mtd);
		if (status & NAND_STATUS_FAIL) {
			printk(KERN_ERR
			       "ERASE FAILED(block = %d,status = 0x%x)\n",
			       addr / mtd->erasesize, status);
		}
	}

}
#else
#define mxc_low_erase(x)
#endif

static int mxc_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	unsigned long timeo = jiffies;
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	send_cmd(NAND_CMD_STATUS, 1);

	while (time_before(jiffies, timeo)) {
#ifdef CONFIG_ARCH_MXC_HAS_NFC_V3
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd))
				break;
		} else
#endif
		{
			if (get_dev_status() & NAND_STATUS_READY)
				break;
		}
		cond_resched();
	}

	status = (int)(get_dev_status());
	return status;
}

static int mxc_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd)
{
	if (sndcmd) {
		read_full_page(mtd, page);
		sndcmd = 0;
	}

	copy_spare(mtd, chip->oob_poi, (char *)SPARE_AREA0, mtd->oobsize, true);
	return sndcmd;
}

static int mxc_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
	int status = 0;
	const uint8_t *buf = chip->oob_poi;
	int read_oob_col = 0;
	volatile uint16_t *p_addr = SPARE_AREA0;

	send_cmd(NAND_CMD_READ0, false);
	send_cmd(NAND_CMD_SEQIN, false);
	mxc_do_addr_cycle(mtd, read_oob_col, page);

	/* copy the oob data */
	copy_spare(mtd, (char *)p_addr, (char *)buf, mtd->oobsize, false);

	if (IS_LARGE_PAGE_NAND)
		PROG_PAGE();
	else
		send_prog_page(0);

	send_cmd(NAND_CMD_PAGEPROG, true);

	status = mxc_nand_wait(mtd, chip);
	if (status & NAND_STATUS_FAIL)
		return -EIO;
	return 0;
}

static int mxc_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t * buf)
{
	int stat;

	stat = mxc_check_ecc_status(mtd);
	if (stat == -1) {
		mtd->ecc_stats.failed++;
		printk(KERN_WARNING "UnCorrectable RS-ECC Error\n");
	} else {
		mtd->ecc_stats.corrected += stat;
		if (stat)
			pr_debug("%d Symbol Correctable RS-ECC Error\n", stat);
	}

	nfc_memcpy((void *)buf, (void *)MAIN_AREA0, mtd->writesize);
	copy_spare(mtd, (void *)chip->oob_poi, (void *)SPARE_AREA0,
		   mtd->oobsize, true);
	return 0;
}

/* Kevin: This is clean and solid */
static void mxc_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t * buf)
{
	memcpy((void *)MAIN_AREA0, buf, mtd->writesize);
	copy_spare(mtd, (char *)chip->oob_poi, (char *)SPARE_AREA0,
		   mtd->oobsize, false);
}

/* Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks. */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr smallpage_memorybased = {
	.options = NAND_BBT_SCAN2NDPAGE,
	.offs = 5,
	.len = 1,
	.pattern = scan_ff_pattern
};

static struct nand_bbt_descr largepage_memorybased = {
	.options = 0,
	.offs = 0,
	.len = 2,
	.pattern = scan_ff_pattern
};

/* Generic flash bbt decriptors
*/
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

static int mxc_nand_scan_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	g_page_mask = this->pagemask;

	if (IS_2K_PAGE_NAND) {
		NFC_SET_NFMS(1 << NFMS_NF_PG_SZ);
		this->ecc.layout = &nand_hw_eccoob_2k;
	} else if (IS_4K_PAGE_NAND) {
		NFC_SET_NFMS(1 << NFMS_NF_PG_SZ);
		this->ecc.layout = &nand_hw_eccoob_4k;
	} else {
		this->ecc.layout = &nand_hw_eccoob_512;
	}

	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = this->ecc.layout;

	/* jffs2 not write oob */
	mtd->flags &= ~MTD_OOB_WRITEABLE;

	/* use flash based bbt */
	this->bbt_td = &bbt_main_descr;
	this->bbt_md = &bbt_mirror_descr;

	/* update flash based bbt */
	this->options |= NAND_USE_FLASH_BBT;

	if (!this->badblock_pattern) {
		this->badblock_pattern = (mtd->writesize > 512) ?
		    &largepage_memorybased : &smallpage_memorybased;
	}

	/* Build bad block table */
	return nand_scan_bbt(mtd, this->badblock_pattern);
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and
 *                remove functions
 *
 * @return  The function always returns 0.
 */
static int __init mxcnd_probe(struct platform_device *pdev)
{
	struct nand_chip *this;
	struct mtd_info *mtd;
	struct flash_platform_data *flash = pdev->dev.platform_data;
	int nr_parts = 0, err = 0;

	nfc_axi_base = IO_ADDRESS(NFC_AXI_BASE_ADDR);
	nfc_ip_base = IO_ADDRESS(NFC_BASE_ADDR);

	/* Resetting  NFC */
	raw_write((raw_read(REG_NFC_RST) | NFC_RST), REG_NFC_RST);

	/* Allocate memory for MTD device structure and private data */
	mxc_nand_data = kzalloc(sizeof(struct mxc_mtd_s), GFP_KERNEL);
	if (!mxc_nand_data) {
		printk(KERN_ERR "%s: failed to allocate mtd_info\n",
		       __FUNCTION__);
		err = -ENOMEM;
		goto out;
	}

	memset((char *)&g_nandfc_info, 0, sizeof(g_nandfc_info));

	mxc_nand_data->dev = &pdev->dev;
	/* structures must be linked */
	this = &mxc_nand_data->nand;
	mtd = &mxc_nand_data->mtd;
	mtd->priv = this;
	mtd->owner = THIS_MODULE;

	/* 5 us command delay time */
	this->chip_delay = 5;
	this->priv = mxc_nand_data;
	this->dev_ready = mxc_nand_dev_ready;
	this->cmdfunc = mxc_nand_command;
	this->waitfunc = mxc_nand_wait;
	this->select_chip = mxc_nand_select_chip;
	this->read_byte = mxc_nand_read_byte;
	this->read_word = mxc_nand_read_word;
	this->write_buf = mxc_nand_write_buf;
	this->read_buf = mxc_nand_read_buf;
	this->verify_buf = mxc_nand_verify_buf;
	this->scan_bbt = mxc_nand_scan_bbt;

	/* NAND bus width determines access funtions used by upper layer */
	if (flash->width == 2) {
		this->read_byte = mxc_nand_read_byte16;
		this->options |= NAND_BUSWIDTH_16;
		NFC_SET_NFMS(1 << NFMS_NF_DWIDTH);
	}

	nfc_clk = clk_get(&pdev->dev, "nfc_clk");
	clk_enable(nfc_clk);	/* Enabled here to satisfy following reset command to succeed */

	/* Disable interrupt */
	raw_write((raw_read(REG_NFC_INTRRUPT) | NFC_INT_MSK), REG_NFC_INTRRUPT);

	init_waitqueue_head(&irq_waitq);
	err = request_irq(MXC_INT_NANDFC, mxc_nfc_irq, 0, "mxc_nd", NULL);
	if (err) {
		goto out_1;
	}

	if (hardware_ecc) {
		this->ecc.read_page = mxc_nand_read_page;
		this->ecc.write_page = mxc_nand_write_page;
		this->ecc.read_oob = mxc_nand_read_oob;
		this->ecc.write_oob = mxc_nand_write_oob;
		this->ecc.layout = &nand_hw_eccoob_512;
		this->ecc.calculate = mxc_nand_calculate_ecc;
		this->ecc.hwctl = mxc_nand_enable_hwecc;
		this->ecc.correct = mxc_nand_correct_data;
		this->ecc.mode = NAND_ECC_HW;
		this->ecc.size = 512;	/* RS-ECC is applied for both MAIN+SPARE not MAIN alone */
		this->ecc.bytes = 9;	/* used for both main and spare area */
		raw_write((raw_read(REG_NFC_ECC_EN) | NFC_ECC_EN),
			  REG_NFC_ECC_EN);
	} else {
		this->ecc.mode = NAND_ECC_SOFT;
		raw_write((raw_read(REG_NFC_ECC_EN) & ~NFC_ECC_EN),
			  REG_NFC_ECC_EN);
	}

	raw_write(raw_read(REG_NFC_SP_EN) & ~NFC_SP_EN, REG_NFC_SP_EN);

	/* Reset NAND */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	raw_write(NFC_SET_BLS(NFC_BLS_UNLCOKED), REG_NFC_BLS);

	/* Blocks to be unlocked */
	/* Start Address = 0X0, End Address   = 0xFFFF */
	UNLOCK_ADDR(0x0, 0xFFFF);

	/* Unlock Block Command for given address range */
	raw_write(NFC_SET_WPC(NFC_WPC_UNLOCK), REG_NFC_WPC);

	/* Scan to find existence of the device */
	if (nand_scan(mtd, 1)) {
		DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_ND: Unable to find any NAND device.\n");
		err = -ENXIO;
		goto out_1;
	}

	/* Register the partitions */
#ifdef CONFIG_MTD_PARTITIONS
	nr_parts =
	    parse_mtd_partitions(mtd, part_probes, &mxc_nand_data->parts, 0);
	if (nr_parts > 0)
		add_mtd_partitions(mtd, mxc_nand_data->parts, nr_parts);
	else if (flash->parts)
		add_mtd_partitions(mtd, flash->parts, flash->nr_parts);
	else
#endif
	{
		pr_info("Registering %s as whole device\n", mtd->name);
		add_mtd_device(mtd);
	}

	platform_set_drvdata(pdev, mtd);

	/* Erase all the blocks of a NAND -- depend on the config */
	mxc_low_erase(mtd);

	return 0;

      out_1:
	kfree(mxc_nand_data);
      out:
	return err;

}

 /*!
  * Dissociates the driver from the device.
  *
  * @param   pdev  the device structure used to give information on which
  *
  * @return  The function always returns 0.
  */

static int __exit mxcnd_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);

	clk_disable(nfc_clk);
	clk_put(nfc_clk);
	platform_set_drvdata(pdev, NULL);

	if (mxc_nand_data) {
		nand_release(mtd);
		free_irq(MXC_INT_NANDFC, NULL);
		kfree(mxc_nand_data);
	}

	return 0;
}

#ifdef CONFIG_PM
/*!
 * This function is called to put the NAND in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device information structure
 *
 * @param   state the power state the device is entering
 *
 * @return  The function returns 0 on success and -1 on failure
 */

static int mxcnd_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mtd_info *info = platform_get_drvdata(pdev);
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : NAND suspend\n");
	if (info)
		ret = info->suspend(info);

	/* Disable the NFC clock */
	clk_disable(nfc_clk);

	return ret;
}

/*!
 * This function is called to bring the NAND back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device information structure
 *
 * @return  The function returns 0 on success and -1 on failure
 */
static int mxcnd_resume(struct platform_device *pdev)
{
	struct mtd_info *info = platform_get_drvdata(pdev);
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : NAND resume\n");
	/* Enable the NFC clock */
	clk_enable(nfc_clk);

	if (info) {
		info->resume(info);
	}

	return ret;
}

#else
#define mxcnd_suspend   NULL
#define mxcnd_resume    NULL
#endif				/* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxcnd_driver = {
	.driver = {
		   .name = "mxc_nandv2_flash",
		   },
	.probe = mxcnd_probe,
	.remove = __exit_p(mxcnd_remove),
	.suspend = mxcnd_suspend,
	.resume = mxcnd_resume,
};

/*!
 * Main initialization routine
 * @return  0 if successful; non-zero otherwise
 */
static int __init mxc_nd_init(void)
{
	/* Register the device driver structure. */
	pr_info("MXC MTD nand Driver %s\n", DVR_VER);
	if (platform_driver_register(&mxcnd_driver) != 0) {
		printk(KERN_ERR "Driver register failed for mxcnd_driver\n");
		return -ENODEV;
	}
	return 0;
}

/*!
 * Clean up routine
 */
static void __exit mxc_nd_cleanup(void)
{
	/* Unregister the device structure */
	platform_driver_unregister(&mxcnd_driver);
}

module_init(mxc_nd_init);
module_exit(mxc_nd_cleanup);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC NAND MTD driver Version 2-3");
MODULE_LICENSE("GPL");
