/*
 * linux/drivers/ide/arm/mxc_ide.c
 *
 * Based on Simtec BAST IDE driver
 * Copyright (c) 2003-2004 Simtec Electronics
 *  Ben Dooks <ben@simtec.co.uk>
 *
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *
 */

/*!
 * @file mxc_ide.c
 *
 * @brief ATA driver
 *
 * @ingroup ATA
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/ide.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <asm/mach-types.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/hardware.h>
#include <asm/arch/dma.h>
#include <linux/platform_device.h>
#include <linux/regulator/regulator.h>

#include "mxc_ide.h"

extern void gpio_ata_active(void);
extern void gpio_ata_inactive(void);
static int mxc_ide_config_drive(ide_drive_t * drive, u8 xfer_mode);
static void mxc_ide_dma_callback(void *arg, int error, unsigned int count);

static struct clk *ata_clk;

/* List of registered interfaces */
static ide_hwif_t *ifs[1];

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 5 PIO modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t0, t1, t2_8, t2_16, t2i, t4, t9, tA;
} pio_specs[] = {
	[0] = {
	.t0 = 600,.t1 = 70,.t2_8 = 290,.t2_16 = 165,.t2i = 0,.t4 =
		    30,.t9 = 20,.tA = 50},[1] = {
	.t0 = 383,.t1 = 50,.t2_8 = 290,.t2_16 = 125,.t2i = 0,.t4 =
		    20,.t9 = 15,.tA = 50},[2] = {
	.t0 = 240,.t1 = 30,.t2_8 = 290,.t2_16 = 100,.t2i = 0,.t4 =
		    15,.t9 = 10,.tA = 50},[3] = {
	.t0 = 180,.t1 = 30,.t2_8 = 80,.t2_16 = 80,.t2i = 0,.t4 =
		    10,.t9 = 10,.tA = 50},[4] = {
	.t0 = 120,.t1 = 25,.t2_8 = 70,.t2_16 = 70,.t2i = 0,.t4 =
		    10,.t9 = 10,.tA = 50}
};

#define NR_PIO_SPECS (sizeof pio_specs / sizeof pio_specs[0])

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 3 MDMA modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t0M, tD, tH, tJ, tKW, tM, tN, tJNH;
} mdma_specs[] = {
	[0] = {
	.t0M = 480,.tD = 215,.tH = 20,.tJ = 20,.tKW = 215,.tM = 50,.tN =
		    15,.tJNH = 20},[1] = {
	.t0M = 150,.tD = 80,.tH = 15,.tJ = 5,.tKW = 50,.tM = 30,.tN =
		    10,.tJNH = 15},[2] = {
	.t0M = 120,.tD = 70,.tH = 10,.tJ = 5,.tKW = 25,.tM = 25,.tN =
		    10,.tJNH = 10}
};

#define NR_MDMA_SPECS (sizeof mdma_specs / sizeof mdma_specs[0])

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 6 UDMA modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t2CYC, tCYC, tDS, tDH, tDVS, tDVH, tCVS, tCVH, tFS_min, tLI_max,
	    tMLI, tAZ, tZAH, tENV_min, tSR, tRFS, tRP, tACK, tSS, tDZFS;
} udma_specs[] = {
	[0] = {
	.t2CYC = 235,.tCYC = 114,.tDS = 15,.tDH = 5,.tDVS = 70,.tDVH =
		    6,.tCVS = 70,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 50,.tRFS = 75,.tRP = 160,.tACK = 20,.tSS =
		    50,.tDZFS = 80},[1] = {
	.t2CYC = 156,.tCYC = 75,.tDS = 10,.tDH = 5,.tDVS = 48,.tDVH =
		    6,.tCVS = 48,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 30,.tRFS = 70,.tRP = 125,.tACK = 20,.tSS =
		    50,.tDZFS = 63},[2] = {
	.t2CYC = 117,.tCYC = 55,.tDS = 7,.tDH = 5,.tDVS = 34,.tDVH =
		    6,.tCVS = 34,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 47},[3] = {
		/*
		 * change tDVS from 20 to 30 since the "drive ready,
		 * seek complete, bad crc error"
		 */
	.t2CYC = 86, .tCYC = 39, .tDS = 7, .tDH = 5, .tDVS = 30, .tDVH =
		    6,.tCVS = 20,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 35},[4] = {
	.t2CYC = 57,.tCYC = 25,.tDS = 5,.tDH = 5,.tDVS = 7,.tDVH =
		    6,.tCVS = 7,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 50,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 25},[5] = {
	.t2CYC = 38,.tCYC = 17,.tDS = 4,.tDH = 5,.tDVS = 5,.tDVH =
		    6,.tCVS = 10,.tCVH = 10,.tFS_min = 0,.tLI_max =
		    75,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 50,.tRP = 85,.tACK = 20,.tSS =
		    50,.tDZFS = 40}
};

#define NR_UDMA_SPECS (sizeof udma_specs / sizeof udma_specs[0])

/*!
 * Calculate values for the ATA bus timing registers and store
 * them into the hardware.
 *
 * @param       mode        Selects PIO, MDMA or UDMA modes
 *
 * @param       speed       Specifies the sub-mode number
 *
 * @return      EINVAL      speed out of range, or illegal mode
 */
static int set_ata_bus_timing(int speed, enum ata_mode mode)
{
	/* get the bus clock cycle time, in ns */
	int T = 1 * 1000 * 1000 * 1000 / clk_get_rate(ata_clk);
	mxc_ide_time_cfg_t cfg0, cfg1, cfg2, cfg3, cfg4, cfg5;
	/* every mode gets the same t_off and t_on */

	GET_TIME_CFG(&cfg0, MXC_IDE_TIME_OFF);
	cfg0.bytes.field1 = 3;
	cfg0.bytes.field2 = 3;
	SET_TIME_CFG(&cfg0, 3, MXC_IDE_TIME_OFF);

	switch (mode) {
	case PIO:
		if (speed < 0 || speed >= NR_PIO_SPECS) {
			return -EINVAL;
		}
		cfg0.bytes.field3 = (pio_specs[speed].t1 + T) / T;
		cfg0.bytes.field4 = (pio_specs[speed].t2_8 + T) / T;

		cfg1.bytes.field1 = (pio_specs[speed].t2_8 + T) / T;
		cfg1.bytes.field2 = (pio_specs[speed].tA + T) / T + 2;
		cfg1.bytes.field3 = 1;
		cfg1.bytes.field4 = (pio_specs[speed].t4 + T) / T;

		GET_TIME_CFG(&cfg2, MXC_IDE_TIME_9);
		cfg2.bytes.field1 = (pio_specs[speed].t9 + T) / T;

		SET_TIME_CFG(&cfg0, 0x0C, MXC_IDE_TIME_OFF);
		SET_TIME_CFG(&cfg1, 0x0F, MXC_IDE_TIME_2r);
		SET_TIME_CFG(&cfg2, 0x01, MXC_IDE_TIME_9);
		break;
	case MDMA:
		if (speed < 0 || speed >= NR_MDMA_SPECS) {
			return -EINVAL;
		}
		GET_TIME_CFG(&cfg2, MXC_IDE_TIME_9);
		GET_TIME_CFG(&cfg3, MXC_IDE_TIME_K);

		cfg2.bytes.field2 = (mdma_specs[speed].tM + T) / T;
		cfg2.bytes.field3 = (mdma_specs[speed].tJNH + T) / T;
		cfg2.bytes.field4 = (mdma_specs[speed].tD + T) / T;

		cfg3.bytes.field1 = (mdma_specs[speed].tKW + T) / T;

		SET_TIME_CFG(&cfg2, 0x0E, MXC_IDE_TIME_9);
		SET_TIME_CFG(&cfg3, 0x01, MXC_IDE_TIME_K);
		break;
	case UDMA:
		if (speed < 0 || speed >= NR_UDMA_SPECS) {
			return -EINVAL;
		}

		GET_TIME_CFG(&cfg3, MXC_IDE_TIME_K);

		cfg3.bytes.field2 = (udma_specs[speed].tACK + T) / T;
		cfg3.bytes.field3 = (udma_specs[speed].tENV_min + T) / T;
		cfg3.bytes.field4 = (udma_specs[speed].tRP + T) / T + 2;

		cfg4.bytes.field1 = (udma_specs[speed].tZAH + T) / T;
		cfg4.bytes.field2 = (udma_specs[speed].tMLI + T) / T;
		cfg4.bytes.field3 = (udma_specs[speed].tDVH + T) / T + 1;
		cfg4.bytes.field4 = (udma_specs[speed].tDZFS + T) / T;

		cfg5.bytes.field1 = (udma_specs[speed].tDVS + T) / T;
		cfg5.bytes.field2 = (udma_specs[speed].tCVH + T) / T;
		cfg5.bytes.field3 = (udma_specs[speed].tSS + T) / T;
		cfg5.bytes.field4 = (udma_specs[speed].tCYC + T) / T;

		SET_TIME_CFG(&cfg3, 0x0E, MXC_IDE_TIME_K);
		SET_TIME_CFG(&cfg4, 0x0F, MXC_IDE_TIME_ZAH);
		SET_TIME_CFG(&cfg5, 0x0F, MXC_IDE_TIME_DVS);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * Placeholder for code to make any hardware tweaks
 * necessary to select a drive.  Currently we are
 * not aware of any.
 */
static void mxc_ide_selectproc(ide_drive_t * drive)
{
}

/*!
 * Hardware-specific interrupt service routine for the ATA driver,
 * called mainly just to dismiss the interrupt at the hardware, and
 * to indicate whether there actually was an interrupt pending.
 *
 * The generic IDE related interrupt handling is done by the IDE layer.
 */
static int mxc_ide_ack_intr(struct hwif_s *hw)
{
	unsigned char status = ATA_RAW_READ(MXC_IDE_INTR_PENDING);
	unsigned char enable = ATA_RAW_READ(MXC_IDE_INTR_ENABLE);

	/*
	 * The only interrupts we can actually dismiss are the FIFO conditions.
	 * INTRQ comes from the bus, and must be dismissed according to IDE
	 * protocol, which will be done by the IDE layer, even when DMA
	 * is invovled (DMA can see it, but can't dismiss it).
	 */
	ATA_RAW_WRITE(status, MXC_IDE_INTR_CLEAR);

	if (status & enable & ~MXC_IDE_INTR_ATA_INTRQ2) {
		printk(KERN_ERR "mxc_ide_ack_intr: unexpected interrupt, "
		       "status=0x%02X\n", status);
	}

	return status ? 1 : 0;
}

/*!
 * Decodes the specified transfer mode and sets both timing and ultra modes
 *
 * @param       drive       Specifies the drive
 *
 * @param       xfer_mode   Specifies the desired transfer mode
 *
 * @return      EINVAL      Illegal mode specified
 */
static void mxc_ide_set_speed(ide_drive_t *drive, u8 xfer_mode)
{
	mxc_ide_private_t *priv = (mxc_ide_private_t *) HWIF(drive)->hwif_data;

	switch (xfer_mode) {
	case XFER_UDMA_7:
	case XFER_UDMA_6:
	case XFER_UDMA_5:
	case XFER_UDMA_4:
	case XFER_UDMA_3:
	case XFER_UDMA_2:
	case XFER_UDMA_1:
	case XFER_UDMA_0:
		priv->ultra = 1;
		set_ata_bus_timing(xfer_mode - XFER_UDMA_0, UDMA);
		break;
	case XFER_MW_DMA_2:
	case XFER_MW_DMA_1:
	case XFER_MW_DMA_0:
		priv->ultra = 0;
		set_ata_bus_timing(xfer_mode - XFER_MW_DMA_0, MDMA);
		break;
	case XFER_PIO_4:
	case XFER_PIO_3:
	case XFER_PIO_2:
	case XFER_PIO_1:
	case XFER_PIO_0:
		set_ata_bus_timing(xfer_mode - XFER_PIO_0, PIO);
		break;
	}

	return;
}

/*!
 * Called when the IDE layer is disabling DMA on a drive.
 *
 * @param       drive       Specifies the drive
 */
static void mxc_ide_dma_off_quietly(ide_drive_t * drive)
{
	drive->using_dma = 0;
}

/*!
 * Called by the IDE layer when something goes wrong
 *
 * @param       drive       Specifies the drive
 *
 */
static void mxc_ide_resetproc(ide_drive_t * drive)
{
	pr_info("%s: resetting ATA controller\n", __func__);

	ATA_RAW_WRITE(0x00, MXC_IDE_ATA_CONTROL);
	udelay(100);
	ATA_RAW_WRITE(MXC_IDE_CTRL_ATA_RST_B, MXC_IDE_ATA_CONTROL);
	udelay(100);
}

/*!
 * Turn on DMA for a drive.
 *
 * @param       drive       Specifies the drive
 *
 * @return      0 if successful
 */
static int mxc_ide_dma_on(ide_drive_t * drive)
{
	int rc = 0;

	/* consult the list of known "bad" drives */
	if (__ide_dma_bad_drive(drive))
		return 1;

	/*
	 * Go to UDMA3 mode.  Beyond that you'll no doubt
	 * need an Ultra-100 cable (the 80 pin type with
	 * ground leads between all the signals)
	 */
	rc = mxc_ide_config_drive(drive, XFER_UDMA_3);
	pr_info("%s: enabling UDMA3 mode %s\n", drive->name,
		rc == 0 ? "success" : "failed");
	drive->using_dma = 1;

	blk_queue_max_hw_segments(drive->queue, MXC_IDE_DMA_BD_NR);
	blk_queue_max_hw_segments(drive->queue, MXC_IDE_DMA_BD_NR);
	blk_queue_max_segment_size(drive->queue, MXC_IDE_DMA_BD_SIZE_MAX);

	HWIF(drive)->dma_host_on(drive);
	return 0;
}

/*!
 * Turn on DMA if the drive can handle it.
 *
 * @param       drive       Specifies the drive
 *
 * @return      0 if successful
 */
static int mxc_ide_dma_check(ide_drive_t * drive)
{
	struct hd_driveid *id = drive->id;
	if (id && (id->capability & 1)) {
		/*
		 * Enable DMA on any drive that has
		 * UltraDMA (mode 0/1/2/3/4/5/6) enabled
		 */
		if ((id->field_valid & 4) && ((id->dma_ultra >> 8) & 0x7f))
			return HWIF(drive)->ide_dma_on(drive);
		/*
		 * Enable DMA on any drive that has mode2 DMA
		 * (multi or single) enabled
		 */
		if (id->field_valid & 2)	/* regular DMA */
			if ((id->dma_mword & 0x404) == 0x404 ||
			    (id->dma_1word & 0x404) == 0x404)
				return HWIF(drive)->ide_dma_on(drive);

		/* Consult the list of known "good" drives */
		return HWIF(drive)->ide_dma_on(drive);
	}
	HWIF(drive)->dma_off_quietly(drive);
	return 0;
}

/*!
 * The DMA is done, and the drive is done.  We'll check the BD array for
 * errors, and unmap the scatter-gather list.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 means all is well, others means DMA signalled an error ,
 */
static int mxc_ide_dma_end(ide_drive_t * drive)
{
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int dma_stat = priv->dma_stat;

	BUG_ON(drive->waiting_for_dma == 0);
	drive->waiting_for_dma = 0;

	/*
	 * We'll unmap the sg table regardless of status.
	 */
	dma_unmap_sg(priv->dev, hwif->sg_table, hwif->sg_nents,
		     hwif->sg_dma_direction);

	return dma_stat;
}

/*!
 * The end-of-DMA interrupt handler
 *
 * @param       drive       Specifies the drive
 *
 * @return      ide_stopped or ide_started
 */
static ide_startstop_t mxc_ide_dma_intr(ide_drive_t * drive)
{
	u8 stat, dma_stat;
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	u8 fifo_fill;

	if (!rq)
		return ide_stopped;

	fifo_fill = ATA_RAW_READ(MXC_IDE_FIFO_FILL);
	BUG_ON(fifo_fill);

	dma_stat = hwif->ide_dma_end(drive);
	stat = hwif->INB(IDE_STATUS_REG);	/* get drive status */
	if (OK_STAT(stat, DRIVE_READY, drive->bad_wstat | DRQ_STAT)) {
		if (dma_stat == MXC_DMA_DONE) {
			ide_end_request(drive, 1, rq->nr_sectors);
			return ide_stopped;
		}
		printk(KERN_ERR "%s: mxc_ide_dma_intr: bad DMA status (0x%x)\n",
		       drive->name, dma_stat);
	}

	return ide_error(drive, "mxc_ide_dma_intr", stat);
}

/*!
 * Directs the IDE INTRQ signal to DMA or to the CPU
 *
 * @param       hwif       Specifies the IDE controller
 *
 * @param       which       \b INTRQ_DMA or \b INTRQ_MCU
 *
 */
static void mxc_ide_set_intrq(ide_hwif_t * hwif, intrq_which_t which)
{
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	unsigned char enable = ATA_RAW_READ(MXC_IDE_INTR_ENABLE);

	switch (which) {
	case INTRQ_DMA:
		enable &= ~MXC_IDE_INTR_ATA_INTRQ2;
		enable |= MXC_IDE_INTR_ATA_INTRQ1;
		break;
	case INTRQ_MCU:
		enable &= ~MXC_IDE_INTR_ATA_INTRQ1;
		enable |= MXC_IDE_INTR_ATA_INTRQ2;
		break;
	}
	priv->enable = enable;

	ATA_RAW_WRITE(enable, MXC_IDE_INTR_ENABLE);
}

/*!
 * Helper routine to configure drive speed
 *
 * @param       drive       Specifies the drive
 *
 * @param       xfer_mode   Specifies the desired transfer mode
 *
 * @return      0 = success, non-zero otherwise
 */
static int mxc_ide_config_drive(ide_drive_t * drive, u8 xfer_mode)
{
	int err;
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) (hwif->hwif_data);
	u8 prev = priv->enable;

	mxc_ide_set_speed(drive, xfer_mode);

	/*
	 * Work around an ADS hardware bug:
	 *
	 * OK, ide_config_drive_speed() is the right thing to call but it's
	 * going to leave an interrupt pending.  We have to jump through hoops
	 * because the ADS board doesn't correctly terminate INTRQ.  Instead
	 * of pulling it low, it pulls it high (asserted), so when the drive
	 * tri-states it, the MX31 sees it as an interrupt.  So we'll disable
	 * the hardware interrupt here, and restore it a bit later, after we've
	 * selected the drive again and let it drive INTRQ low for a bit.
	 *
	 * On later ADS boards, when presumably the correct INTRQ termination
	 * is in place, these extra hoops won't be necessary, but they
	 * shouldn't hurt, either.
	 */
	priv->enable = 0;
	ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);

	err = ide_config_drive_speed(drive, xfer_mode);

	if (ATA_RAW_READ(MXC_IDE_INTR_PENDING) &
	    (MXC_IDE_INTR_ATA_INTRQ2 | MXC_IDE_INTR_ATA_INTRQ1)) {
		/*
		 * OK, the 'bad thing' is happening, so we'll clear nIEN to
		 * get the drive to drive INTRQ, then we'll read status to
		 * clear any pending interrupt.  This sequence gets us by
		 * the spurious and stuck interrupt we see otherwise.
		 */
		SELECT_DRIVE(drive);
		udelay(2);
		hwif->OUTB(0, IDE_CONTROL_REG);
		udelay(2);
		hwif->INB(IDE_STATUS_REG);
		udelay(2);
	}

	priv->enable = prev;
	ATA_RAW_WRITE(prev, MXC_IDE_INTR_ENABLE);

	return err;
}

/*!
 * Masks drive interrupts temporarily at the hardware level
 *
 * @param       drive       Specifies the drive
 *
 * @param       mask        1 = disable interrupts, 0 = re-enable
 *
 */
static void mxc_ide_maskproc(ide_drive_t * drive, int mask)
{
	mxc_ide_private_t *priv =
	    (mxc_ide_private_t *) (HWIF(drive)->hwif_data);
	BUG_ON(!priv);

	if (mask) {
		ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);
	} else {
		ATA_RAW_WRITE(priv->enable, MXC_IDE_INTR_ENABLE);
	}
}

/*!
 * DMA completion callback routine.  This gets called after the DMA request
 * has completed or aborted.All we need to do here is return ownership of
 * the drive's INTRQ signal back to the CPU, which will immediately raise
 * the interrupt to the IDE layer.
 *
 * @param       arg         The drive we're servicing
 * @param	error	    The error number of DMA transfer.
 * @param	count	    The transfered length.
 */
static void mxc_ide_dma_callback(void *arg, int error, unsigned int count)
{
	ide_hwif_t *hwif = HWIF((ide_drive_t *) arg);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) (hwif->hwif_data);
	unsigned long fifo_fill;

	/*
	 * clean the fifo if the fill register is non-zero.
	 * If the fill register is non-zero, it is incorrect state.
	 */
	fifo_fill = ATA_RAW_READ(MXC_IDE_FIFO_FILL);
	while (fifo_fill) {
		ATA_RAW_READ(MXC_IDE_FIFO_DATA_32);
		fifo_fill = ATA_RAW_READ(MXC_IDE_FIFO_FILL);
	}

	priv->dma_stat = error;
	/*
	 * Redirect ata_intrq back to us instead of the DMA.
	 */
	mxc_ide_set_intrq(hwif, INTRQ_MCU);
}

/*!
 * DMA set up.  This is called once per DMA request to the drive. It delivers
 * the scatter-gather list into the DMA channel and prepares both the ATA
 * controller and the DMA engine for the DMA
 * transfer.
 *
 * @param       drive     The drive we're servicing
 *
 * @return      0 on success, non-zero otherwise
 */
static int mxc_ide_dma_setup(ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	struct scatterlist *sg = hwif->sg_table;
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int dma_ultra = priv->ultra ? MXC_IDE_CTRL_DMA_ULTRA : 0;
	int dma_mode = 0;
	int chan;
	u8 ata_control;
	u8 fifo_fill;

	BUG_ON(!rq);
	BUG_ON(!priv);
	BUG_ON(drive->waiting_for_dma);

	/*Initialize the dma state */
	priv->dma_stat = MXC_DMA_TRANSFER_ERROR;

	/*
	 * Prepare the ATA controller for the DMA
	 */
	if (rq_data_dir(rq)) {
		chan = priv->dma_write_chan;
		ata_control = MXC_IDE_CTRL_FIFO_RST_B |
		    MXC_IDE_CTRL_ATA_RST_B |
		    MXC_IDE_CTRL_FIFO_TX_EN |
		    MXC_IDE_CTRL_DMA_PENDING |
		    dma_ultra | MXC_IDE_CTRL_DMA_WRITE;

		dma_mode = DMA_MODE_WRITE;
	} else {
		chan = priv->dma_read_chan;
		ata_control = MXC_IDE_CTRL_FIFO_RST_B |
		    MXC_IDE_CTRL_ATA_RST_B |
		    MXC_IDE_CTRL_FIFO_RCV_EN |
		    MXC_IDE_CTRL_DMA_PENDING | dma_ultra;

		dma_mode = DMA_MODE_READ;
	}

	/*
	 * Set up the DMA interrupt callback
	 */
	mxc_dma_callback_set(chan, mxc_ide_dma_callback, (void *)drive);

	/*
	 * If the ATA FIFO isn't empty, we shouldn't even be here
	 */
	fifo_fill = ATA_RAW_READ(MXC_IDE_FIFO_FILL);
	BUG_ON(fifo_fill);	// $$$ TODO: need better recovery here

	ide_map_sg(drive, rq);

	hwif->sg_dma_direction = rq_data_dir(rq) ? DMA_TO_DEVICE :
	    DMA_FROM_DEVICE;

	hwif->sg_nents = dma_map_sg(priv->dev, sg, hwif->sg_nents,
				    hwif->sg_dma_direction);
	BUG_ON(!hwif->sg_nents);
	BUG_ON(hwif->sg_nents > MXC_IDE_DMA_BD_NR);

	mxc_dma_sg_config(chan, sg, hwif->sg_nents, 0, dma_mode);

	ATA_RAW_WRITE(ata_control, MXC_IDE_ATA_CONTROL);
	ATA_RAW_WRITE(MXC_IDE_DMA_WATERMARK / 2, MXC_IDE_FIFO_ALARM);

	/*
	 * Route ata_intrq to the DMA engine, and not to us.
	 */
	mxc_ide_set_intrq(hwif, INTRQ_DMA);

	/*
	 * The DMA and ATA controller are ready to go.
	 * mxc_ide_dma_start() will start the DMA transfer,
	 * and mxc_ide_dma_exec_cmd() will tickle the drive, which
	 * actually initiates the DMA transfer on the ATA bus.
	 * The ATA controller is DMA slave for both read and write.
	 */
	BUG_ON(drive->waiting_for_dma);
	drive->waiting_for_dma = 1;

	return 0;
}

/*!
 * DMA timeout notification.  This gets called when the IDE layer above
 * us times out waiting for a request.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 to attempt recovery, otherwise, an additional tick count
 *              to keep waiting
 */
static int mxc_ide_dma_timer_expiry(ide_drive_t * drive)
{
	printk(KERN_ERR "%s: fifo_fill=%d\n", drive->name,
	       readb(MXC_IDE_FIFO_FILL));

	mxc_ide_resetproc(NULL);

	if (drive->waiting_for_dma) {
		HWIF(drive)->ide_dma_end(drive);
	}

	return 0;
}

/*!
 * Called by the IDE layer to start a DMA request on the specified drive.
 * The request has already been prepared by \b mxc_ide_dma_setup().  All
 * we need to do is pass the command through while specifying our timeout
 * handler.
 *
 * @param       drive       The drive we're servicing
 *
 * @param       cmd         The IDE command for the drive
 *
 */
static void mxc_ide_dma_exec_cmd(ide_drive_t * drive, u8 cmd)
{
	ide_execute_command(drive, cmd, mxc_ide_dma_intr, 2 * WAIT_CMD,
			    mxc_ide_dma_timer_expiry);
}

/*!
 * Called by the IDE layer to start the DMA controller.  The request has
 * already been prepared by \b mxc_ide_dma_setup().  All we do here
 * is tickle the DMA channel.
 *
 * @param       drive       The drive we're servicing
 *
 */
static void mxc_ide_dma_start(ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int chan = rq_data_dir(rq) ? priv->dma_write_chan : priv->dma_read_chan;

	BUG_ON(chan < 0);

	/*
	 * Tickle the DMA channel.  This starts the channel, but it is likely
	 * that DMA will yield and go idle before the DMA request arrives from
	 * the drive.  Nonetheless, at least the context will be hot.
	 */
	mxc_dma_enable(chan);
}

/*!
 * There is a race between the DMA interrupt and the timeout interrupt.  This
 * gets called during the IDE layer's timeout interrupt to see if the DMA
 * interrupt has also occured or is pending.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      1 means there is a DMA interrupt pending, 0 otherwise
 */
static int mxc_ide_dma_test_irq(ide_drive_t * drive)
{
	unsigned char status = ATA_RAW_READ(MXC_IDE_INTR_PENDING);

	/*
	 * We need to test the interrupt status without dismissing any.
	 */

	return status & (MXC_IDE_INTR_ATA_INTRQ1
			 | MXC_IDE_INTR_ATA_INTRQ2) ? 1 : 0;
}

/*!
 * Called to turn off DMA on this drive's controller, such as when a DMA error
 * occured and the IDE layer wants to fail back to PIO mode.  We won't do
 * anything special, leaving the DMA channel allocated for future attempts.
 *
 * @param       drive       The drive we're servicing
 */
static void mxc_ide_dma_host_off(ide_drive_t * drive)
{
}

/*!
 * Called to turn on DMA on this drive's controller.
 *
 * @param       drive       The drive we're servicing
 */
static void mxc_ide_dma_host_on(ide_drive_t * drive)
{
}

/*!
 * Called for special handling on timeouts.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static void mxc_ide_dma_timeout(ide_drive_t *drive)
{
	return;
}

/*!
 * Called for special handling on lost irq's.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static void mxc_ide_dma_lost_irq(ide_drive_t *drive)
{
	return;
}

/*!
 * Called once per controller to set up DMA
 *
 * @param       hwif       Specifies the IDE controller
 *
 */
static void mxc_ide_dma_init(ide_hwif_t * hwif)
{
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;

	hwif->dmatable_cpu = NULL;
	hwif->dmatable_dma = 0;
	hwif->set_dma_mode = mxc_ide_set_speed;
	hwif->set_pio_mode = mxc_ide_set_speed;
	hwif->resetproc = mxc_ide_resetproc;

	/*
	 * Allocate and setup the DMA channels
	 */
	priv->dma_read_chan = mxc_dma_request(MXC_DMA_ATA_RX, "MXC ATA RX");
	if (priv->dma_read_chan < 0) {
		printk(KERN_ERR "%s: couldn't get RX DMA channel\n",
		       hwif->name);
		goto err_out;
	}

	priv->dma_write_chan = mxc_dma_request(MXC_DMA_ATA_TX, "MXC ATA TX");
	if (priv->dma_write_chan < 0) {
		printk(KERN_ERR "%s: couldn't get TX DMA channel\n",
		       hwif->name);
		goto err_out;
	}

	pr_info("%s: read chan=%d , write chan=%d \n",
		hwif->name, priv->dma_read_chan, priv->dma_write_chan);

	set_ata_bus_timing(0, UDMA);

	/*
	 * All ready now
	 */
	hwif->ultra_mask = 0x7f;
	hwif->mwdma_mask = 0x07;
	hwif->swdma_mask = 0x07;

	hwif->dma_off_quietly = mxc_ide_dma_off_quietly;
	hwif->ide_dma_on = mxc_ide_dma_on;
	hwif->dma_setup = mxc_ide_dma_setup;
	hwif->dma_exec_cmd = mxc_ide_dma_exec_cmd;
	hwif->dma_start = mxc_ide_dma_start;
	hwif->ide_dma_end = mxc_ide_dma_end;
	hwif->ide_dma_test_irq = mxc_ide_dma_test_irq;
	hwif->dma_host_off = mxc_ide_dma_host_off;
	hwif->dma_host_on = mxc_ide_dma_host_on;
	hwif->dma_timeout = mxc_ide_dma_timeout;
	hwif->dma_lost_irq = mxc_ide_dma_lost_irq;

	hwif->hwif_data = (void *)priv;
	return;

      err_out:
	if (priv->dma_read_chan >= 0)
		mxc_dma_free(priv->dma_read_chan);
	if (priv->dma_write_chan >= 0)
		mxc_dma_free(priv->dma_write_chan);
	kfree(priv);
	return;
}

/*!
 * MXC-specific IDE registration helper routine.  Among other things,
 * we tell the IDE layer where our standard IDE drive registers are,
 * through the \b io_ports array.  The IDE layer sends commands to and
 * reads status directly from attached IDE drives through these drive
 * registers.
 *
 * @param   base   Base address of memory mapped IDE standard drive registers
 *
 * @param   aux    Address of the auxilliary ATA control register
 *
 * @param   irq    IRQ number for our hardware
 *
 * @param   hwifp  Pointer to hwif structure to be updated by the IDE layer
 *
 * @param   priv   Pointer to private structure
 *
 * @return  ENODEV if no drives are present, 0 otherwise
 */
static int __init
mxc_ide_register(unsigned long base, unsigned int aux, int irq,
		 ide_hwif_t ** hwifp, mxc_ide_private_t * priv)
{
	int i = 0;
	hw_regs_t hw;
	ide_hwif_t *hwif = &ide_hwifs[0];
	const int regsize = 4;

	memset(&hw, 0, sizeof(hw));

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw.io_ports[i] = (unsigned long)base;
		base += regsize;
	}
	hw.io_ports[IDE_CONTROL_OFFSET] = aux;
	hw.irq = irq;
	hw.ack_intr = &mxc_ide_ack_intr;

	*hwifp = hwif;
	ide_register_hw(&hw, NULL, 0, hwifp);

	hwif->selectproc = &mxc_ide_selectproc;
	hwif->maskproc = &mxc_ide_maskproc;
	hwif->hwif_data = (void *)priv;
	hwif->pio_mask = ATA_PIO4;
	mxc_ide_set_intrq(hwif, INTRQ_MCU);

	/*
	 * The IDE layer will set hwif->present if we have devices attached,
	 * if we don't, discard the interface reset the ports.
	 */
	if (!hwif->present) {
		pr_info("ide%d: Bus empty, interface released.\n", hwif->index);
		for (i = IDE_DATA_OFFSET; i <= IDE_CONTROL_OFFSET; ++i)
			hwif->io_ports[i] = 0;
		hwif->chipset = ide_unknown;
		hwif->noprobe = 1;
		return -ENODEV;
	}

	mxc_ide_dma_init(hwif);

	for (i = 0; i < MAX_DRIVES; i++) {
		mxc_ide_dma_check(hwif->drives + i);
	}

	return 0;
}

/*!
 * This function is called to put the ATA in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which ATA
 *                device to suspend
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_ide_suspend(struct platform_device *pdev, pm_message_t state)
{
	ide_hwif_t *hwif = &ide_hwifs[0];
	mxc_ide_private_t *priv = (mxc_ide_private_t *) (hwif->hwif_data);

	/*
	 * Disable hardware interrupts.
	 * Be carefully, the system would halt if there is no pwr supply
	 */
	if (priv->attached == 1)
		ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);

	if (priv->dma_read_chan >= 0) {
		mxc_dma_disable(priv->dma_read_chan);
		mxc_dma_free(priv->dma_read_chan);
	}
	if (priv->dma_write_chan >= 0) {
		mxc_dma_disable(priv->dma_write_chan);
		mxc_dma_free(priv->dma_write_chan);
	}

	/*
	 * Turn off the power
	 */
	/* There is a HDD disk attached on the ata bus */
	if (priv->attached == 1) {
		/* power off the ATA pwr supply */
		if (priv->regulator_drive)
			regulator_disable(priv->regulator_drive);
		if (priv->regulator_io)
			regulator_disable(priv->regulator_io);
		gpio_ata_inactive();
	}

	return 0;
}

static int mxc_ide_resume(struct platform_device *pdev)
{
	ide_hwif_t *hwif = &ide_hwifs[0];
	mxc_ide_private_t *priv = (mxc_ide_private_t *) (hwif->hwif_data);

	/* There is a HDD disk attached on the ata bus */
	if (priv->attached == 1) {
		/* Select group B pins, and enable the interface */
		gpio_ata_active();
		/* power up the ATA pwr supply */
		if (priv->regulator_drive) {
			if (regulator_enable(priv->regulator_drive))
				printk(KERN_ERR "enable power_drive error.\n");
		}
		if (priv->regulator_io) {
			if (regulator_enable(priv->regulator_io))
				printk(KERN_ERR "enable power_io error.\n");
		}

		/* Deassert the reset bit to enable the interface */
		ATA_RAW_WRITE(MXC_IDE_CTRL_ATA_RST_B, MXC_IDE_ATA_CONTROL);

		/* Set initial timing and mode */
		set_ata_bus_timing(4, PIO);

		/* Reset the interface */
		mxc_ide_resetproc(NULL);
	}

	priv->dma_read_chan = mxc_dma_request(MXC_DMA_ATA_RX, "MXC ATA RX");
	if (priv->dma_read_chan < 0) {
		printk(KERN_ERR "%s: couldn't get RX DMA channel\n",
		       hwif->name);
		return -EAGAIN;
	}
	priv->dma_write_chan = mxc_dma_request(MXC_DMA_ATA_TX, "MXC ATA TX");
	if (priv->dma_write_chan < 0) {
		if (priv->dma_read_chan > 0)
			mxc_dma_free(priv->dma_read_chan);

		printk(KERN_ERR "%s: couldn't get TX DMA channel\n",
		       hwif->name);
		return -EAGAIN;
	}

	return 0;
}

/*!
 * Driver initialization routine.  Prepares the hardware for ATA activity.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions.
 *
 * @return  The function returns 0 on successful.
 *          Otherwise returns specific error code.
 */
static int mxc_ide_probe(struct platform_device *pdev)
{
	int index = 0;
	mxc_ide_private_t *priv;
	struct mxc_ide_platform_data *ide_plat = pdev->dev.platform_data;
	struct regulator *regulator_drive, *regulator_io;

	pr_info("MXC: IDE driver, (c) 2004-2008 Freescale Semiconductor\n");

	if (!ide_plat)
		return -EINVAL;

	ata_clk = clk_get(NULL, "ata_clk");
	if (ata_clk) {
		if (clk_enable(ata_clk)) {
			printk(KERN_ERR "enable clk error.\n");
			goto ERR_NODEV;
		}
	} else {
		printk(KERN_ERR "MXC: IDE driver, can't get clk\n");
		goto ERR_NODEV;
	}

	ATA_RAW_WRITE(MXC_IDE_CTRL_ATA_RST_B, MXC_IDE_ATA_CONTROL);

	/* Select group B pins, and enable the interface */
	gpio_ata_active();

	/*
	 * Enable the power to HDD
	 */
	if (ide_plat->power_drive != NULL) {
		regulator_drive = regulator_get(&pdev->dev,
						ide_plat->power_drive);
		if (IS_ERR(regulator_drive)) {
			printk(KERN_ERR "MXC: IDE driver, can't get power\n");
			goto ERR_NODEV;
		} else {
			if (regulator_enable(regulator_drive)) {
				printk(KERN_ERR "enable regulator error.\n");
				goto ERR_NODEV;
			}
			msleep(100);
		}
	} else
		regulator_drive = NULL;

	if (ide_plat->power_io != NULL) {
		regulator_io = regulator_get(&pdev->dev, ide_plat->power_io);
		if (IS_ERR(regulator_io)) {
			printk(KERN_ERR "MXC: IDE driver, can't get power\n");
			goto ERR_NODEV;
		} else {
			if (regulator_enable(regulator_io)) {
				printk(KERN_ERR "enable regulator error.\n");
				goto ERR_NODEV;
			}
			msleep(100);
		}
	} else
		regulator_io = NULL;

	/* Set initial timing and mode */

	set_ata_bus_timing(4, PIO);

	/* Reset the interface */

	mxc_ide_resetproc(NULL);

	/*
	 * Enable hardware interrupts.
	 * INTRQ2 goes to us, so we enable it here, but we'll need to ignore
	 * it when DMA is doing the transfer.
	 */
	ATA_RAW_WRITE(MXC_IDE_INTR_ATA_INTRQ2, MXC_IDE_INTR_ENABLE);

	/*
	 * Allocate a private structure
	 */
	priv = (mxc_ide_private_t *) kmalloc(sizeof *priv, GFP_KERNEL);
	if (priv == NULL) {
		ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);
		if (regulator_drive) {
			regulator_disable(regulator_drive);
			regulator_put(regulator_drive, &pdev->dev);
		}
		if (regulator_io) {
			regulator_disable(regulator_io);
			regulator_put(regulator_io, &pdev->dev);
		}
		gpio_ata_inactive();
		goto ERR_NOMEM;
	}

	memset(priv, 0, sizeof *priv);
	priv->dev = NULL;	// dma_map_sg() ignores it anyway
	priv->dma_read_chan = -1;
	priv->dma_write_chan = -1;
	priv->attached = 1;

	/*
	 * Now register
	 */

	index =
	    mxc_ide_register(IDE_ARM_IO, IDE_ARM_CTL, IDE_ARM_IRQ, &ifs[0],
			     priv);
	if (index < 0) {
		printk(KERN_ERR "Unable to register the MXC IDE driver\n");
		ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);

		/*
		 * If there is no hdd disk, turn off the clock and power
		 */
		/* Poweroff the HDD */
		clk_disable(ata_clk);
		clk_put(ata_clk);
		priv->attached = 0;
		if (regulator_drive) {
			regulator_disable(regulator_drive);
			regulator_put(regulator_drive, &pdev->dev);
		}
		if (regulator_io) {
			regulator_disable(regulator_io);
			regulator_put(regulator_io, &pdev->dev);
		}
		gpio_ata_inactive();
		kfree(priv);
		goto ERR_NODEV;
	}
#ifdef ATA_USE_IORDY
	/* turn on IORDY protocol */

	udelay(25);
	ATA_RAW_WRITE(MXC_IDE_CTRL_ATA_RST_B | MXC_IDE_CTRL_IORDY_EN,
		      MXC_IDE_ATA_CONTROL);
#endif
	priv->regulator_drive = regulator_drive;
	priv->regulator_io = regulator_io;
	return 0;

      ERR_NODEV:
	return -ENODEV;
      ERR_NOMEM:
	return -ENOMEM;

}

/*!
 * Driver exit routine.  Clean up.
 *
 * @param   pdev  the device structure used to give information on which ATA
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxc_ide_remove(struct platform_device *pdev)
{
	ide_hwif_t *hwif = ifs[0];
	mxc_ide_private_t *priv;

	BUG_ON(!hwif);
	priv = (mxc_ide_private_t *) hwif->hwif_data;
	BUG_ON(!priv);

	ide_unregister(hwif->index);

	/*
	 * Disable hardware interrupts.
	 */
	if (priv->attached)
		ATA_RAW_WRITE(0, MXC_IDE_INTR_ENABLE);
	/*
	 * Deallocate DMA channels.  Free's BDs and everything.
	 */
	if (priv->dma_read_chan >= 0)
		mxc_dma_free(priv->dma_read_chan);
	if (priv->dma_write_chan >= 0)
		mxc_dma_free(priv->dma_write_chan);

	/*
	 * Turn off the clock
	 */
	clk_disable(ata_clk);
	clk_put(ata_clk);

	/*
	 * Disable the interface
	 * Free the pins
	 */
	gpio_ata_inactive();

	/*
	 * Turn off the power
	 */
	/* Disable power supply */
	if (priv->attached) {
		priv->attached = 0;
		if (priv->regulator_drive) {
			regulator_disable(priv->regulator_drive);
			regulator_put((priv->regulator_drive), &pdev->dev);
		}
		if (priv->regulator_io) {
			regulator_disable(priv->regulator_io);
			regulator_put((priv->regulator_io), &pdev->dev);
		}
	}

	/*
	 * Free the private structure.
	 */
	kfree(priv);
	hwif->hwif_data = NULL;

	return 0;
}

/*!
 * MXC IDE host operations structure.
 * These functions are registered with IDE/ATA Bus protocol driver.
 */
static struct platform_driver mxc_ide_driver = {
	.driver = {
		   .name = "mxc_ide",
		   },
	.probe = mxc_ide_probe,
	.remove = mxc_ide_remove,
	.suspend = mxc_ide_suspend,
	.resume = mxc_ide_resume,
};

/*!
 * This function is used to initialize the IDE driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the IDE callback functions with the core ATA driver.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxc_ide_init(void)
{
	return platform_driver_register(&mxc_ide_driver);
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxc_ide_exit(void)
{
	platform_driver_unregister(&mxc_ide_driver);
}

module_init(mxc_ide_init);
module_exit(mxc_ide_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION
    ("Freescale MXC IDE (based on Simtec BAST IDE driver by Ben Dooks <ben@simtec.co.uk>)");
