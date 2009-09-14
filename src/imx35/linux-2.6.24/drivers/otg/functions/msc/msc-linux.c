/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * otg/function/msc/msc-linux.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/msc-linux.c|20070425221028|41479
 *
 *      Copyright (c) 2003-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Tony Tang <tt@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */

/*!
 * @file otg/functions/msc/msc-linux.c
 * @brief Mass Storage Driver private defines
 *
 *
 * This is a Mass Storage Class Function that uses the Bulk Only protocol.
 *
 * To use simply load with something like:
 *
 *      insmod msc_fd.o vendor_id=0xffff product_id=0xffff
 *
 * Notes:
 *
 * 1. Currently block I/O is done a page at a time. I have not determined if
 * it is possible to dispatch a multiple page generic request. It should at
 * least be possible to queue page requests.
 *
 * 2. Currently for READ operations we have a maximum of one outstanding
 * read block I/O and send urb. These are allowed to overlap so that we can
 * continue to read data while sending data to the host.
 *
 * 3. Currently for WRITE operations we have a maximum of one outstanding
 * recv urb and one outstanding write block I/O. These are allowed to
 * overlap so that we can continue to receive data from the host while
 * waiting for writing to complete.
 *
 * 4. It would be possible to allow multiple writes to be pending, to the
 * limit of the page cache, if desired.
 *
 * 5. It should be possible to allow multiple outstanding reads to be
 * pending, to the limit of the page cache, but this potentially could
 * require dealing with out of order completions of the reads. Essentially a
 * list of completed buffer heads would be required to hold any completed
 * buffer heads that cannot be sent prior to another, earlier request being
 * completed.
 *
 * 6. Currently ioctl calls are available to start and stop device i/o.
 *
 * 7. The driver can optionally generate trace messages showing each sectors
 * CRC as read or written with LBA. These can be compared by user programs to
 * ensure that the correct data was read and/or written.
 *
 *
 * TODO
 *
 * 1. error handling for block io, e.g. what if using with removable block
 * device (like CF card) and it is removed.
 *
 * 2. Currently we memcpy() data from between the urb buffer and buffer
 * head page. It should be possible to simply use the page buffer for the
 * urb.
 *
 * 3. Should we offer using fileio as an option? This would allow direct access
 * to a block device image stored in a normal file or direct access to (for example)
 * ram disks. It would require implementing a separate file I/O kernel thread to
 * do the actual I/O.
 *
 * 4. It may be interesting to support use of SCSI block device and pass the
 * scsi commands directly to that. This would allow vendor commands for real
 * devices to be passed through and executed with results being properly
 * returned to the host. [This is the intended design for the mass storage
 * specification.]
 *
 *
 * TODO FIXME Bus Interface Notes
 *
 *
 * @ingroup MSCFunction
 * @ingroup LINUXOS
 */


#include <otg/otg-compat.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <otg/otg-linux.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/random.h>
#include <linux/slab.h>

#if defined(LINUX26)
#include <linux/bio.h>
#include <linux/buffer_head.h>
#endif
#define CONFIG_OTG_MSC_BLOCK_TRACE 1
#include <linux/blkdev.h>
#include <linux/kdev_t.h>

#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
#include "crc.h"
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
#include "msc-io.h"


//#include "rbc.h"


/* Module Parameters ************************************************************************* */
EMBED_LICENSE();

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (major, "Device Major", 0);
MOD_PARM_INT (minor, "Device Minor", 0);


#define DEVICE_EJECTED          0x0001          // MEDIA_EJECTED
#define DEVICE_INSERTED         0x0002          // MEDIA_INSERT

#define DEVICE_OPEN             0x0004          // WR_PROTECT_OFF
#define DEVICE_WRITE_PROTECTED  0x0008          // WR_PROTECT_ON

#define DEVICE_CHANGE_ON        0x0010          // MEDIA_CHANGE_ON

#define DEVICE_PREVENT_REMOVAL  0x0020


#define DEF_NUMBER_OF_HEADS     0x10
#define DEF_SECTORS_PER_TRACK   0x20


DECLARE_MUTEX(msc_sem);

spinlock_t msc_lock;

/* MSC ********************************************************************** */

//XXX no use struct msc_private msc_private;

int msc_urb_sent (struct usbd_urb *tx_urb, int rc);


/* Block Device ************************************************************* */

/*! msc_open_blockdev - open the block device specified in msc->major, msc->minor
 *
 * Sets appropriate fields to show current status of block device.
 *
 * XXX TODO - this needs to be tested against RO and absent devices.
 *
 * @param function_instance - pointer to this function instance
 * @return none
 *
 */
void msc_open_blockdev(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata;
        int rc;
        struct inode *inode;
        request_queue_t *qqq;

        spin_lock_init(&msc_lock);
        down(&msc_sem);
        msc->block_dev_state = DEVICE_EJECTED;

        TRACE_MSG2(MSC, "OPEN BLOCKDEV: Major: %x Minor: %x", msc->major, msc->minor);

        /*
         * Check device information and verify access to the block device.
         */
        THROW_UNLESS(msc->major, ejected);

        msc->dev = MKDEV(msc->major, msc->minor);

#if defined(LINUX26)

        THROW_UNLESS((msc->bdev = bdget(msc->dev)), ejected);



        if ((rc = blkdev_get(msc->bdev, FMODE_READ | FMODE_WRITE, 0))) {

                TRACE_MSG0(MSC,"OPEN BLOCKDEV: cannot open RW");
                THROW_IF ((rc = blkdev_get(msc->bdev, FMODE_READ, 0)), ejected);
                msc->block_dev_state |= DEVICE_WRITE_PROTECTED;
        }


        printk(KERN_INFO"%s: bdev: %x\n", __FUNCTION__, msc->bdev);
        printk(KERN_INFO"%s: bd_part: %x\n", __FUNCTION__, msc->bdev->bd_part);
        printk(KERN_INFO"%s: bd_disk: %x\n", __FUNCTION__, msc->bdev->bd_disk);
        printk(KERN_INFO"%s: capacity: %d\n", __FUNCTION__, msc->bdev->bd_disk->capacity);

        #else /* defined(LINUX26) */   /*the same as linux2.4*/
        THROW_IF (!(msc->bdev = bdget(kdev_t_to_nr(msc->dev))), ejected);
        if ((rc = blkdev_get(msc->bdev, FMODE_READ | FMODE_WRITE, 0, BDEV_RAW))) {

                TRACE_MSG0(MSC,"OPEN BLOCKDEV: cannot open RW");
                THROW_IF ((rc = blkdev_get(msc->bdev, FMODE_READ, 0, BDEV_RAW)), ejected);
                msc->block_dev_state |= DEVICE_WRITE_PROTECTED;
        }

#endif /* defined(LINUX26) */

        msc->io_state = MSC_INACTIVE;
        msc->block_dev_state &= ~DEVICE_EJECTED;
        msc->block_dev_state |= DEVICE_INSERTED | DEVICE_CHANGE_ON;

        TRACE_MSG1(MSC,"OPEN BLOCKDEV: opened block_dev_state: %x", msc->block_dev_state);

        /*
         * Note that capacity must return the LBA of the last addressable block
         * c.f. RBC 4.4, RBC 5.3 and notes below RBC Table 6
         *
         * The blk_size array contains the number of addressable blocks or N,
         * capacity is therefore N-1.
         */

#if defined(LINUX26)
        TRACE_MSG0(MSC,"Linux 2.6");
        msc->capacity = msc->bdev->bd_disk->capacity;
        msc->block_size = bdev_hardsect_size(msc->bdev);
        msc->max_blocks = PAGE_SIZE / msc->block_size;


#else /* defined(LINUX26) */
        TRACE_MSG0(MSC,"Linux 2.4");
        msc->capacity = (blk_size[MAJOR(msc->dev)][MINOR(msc->dev)] << 1) - 1;
        msc->block_size = get_hardsect_size(msc->dev);
        msc->max_blocks = PAGE_SIZE / msc->block_size;

        TRACE_MSG2(MSC,"blk_size: %x %d",
                        blk_size[MAJOR(msc->dev)][MINOR(msc->dev)] << 1,
                        blk_size[MAJOR(msc->dev)][MINOR(msc->dev)] << 1);
#endif /* defined(LINUX26) */

        TRACE_MSG2(MSC,"capacity: %x %d", msc->capacity, msc->capacity);
        TRACE_MSG2(MSC,"block_size: %x %d", msc->block_size, msc->block_size);
        TRACE_MSG2(MSC,"max_blocks: %x %d", msc->max_blocks, msc->max_blocks);

        /* setup generic buffer_head
         * XXX do we need two pages? it should be possible to have a single page
         * for both read and write, in fact do we need a read_bh and write_bh?
         * XXX ensure the page (or pages) get deallocated
         */
#if defined(LINUX26)
        printk(KERN_INFO "BIO,start\n");
        msc->write_pending = msc->read_pending = 0;
        bio_init(&msc->write_bio);
        bio_init(&msc->read_bio);
        msc->write_bio.bi_private = msc->read_bio.bi_private = function_instance;

        msc->read_bio.bi_io_vec=&msc->rbio_vec;
        msc->rbio_vec.bv_page = alloc_page(GFP_NOIO);     // XXX ensure that this gets de-allocated
        msc->rbio_vec.bv_len = 0;
        msc->rbio_vec.bv_offset = 0;
        msc->read_bio.bi_vcnt = 1;
        msc->read_bio.bi_idx = 0;
        msc->read_bio.bi_size = 0;
        msc->read_bio.bi_bdev = msc->bdev;
        msc->read_bio.bi_sector = 0;

        msc->write_bio.bi_io_vec=&msc->wbio_vec;
        msc->wbio_vec.bv_page = alloc_page(GFP_NOIO);     // XXX ensure that this gets de-allocated
        msc->wbio_vec.bv_len = 0;
        msc->wbio_vec.bv_offset = 0;
        msc->write_bio.bi_vcnt = 1;
        msc->write_bio.bi_idx = 0;
        msc->write_bio.bi_size = 0;
        msc->write_bio.bi_bdev = msc->bdev;
        msc->write_bio.bi_sector = 0;
        printk(KERN_INFO "BIO,end\n");

#else
        msc->write_pending = msc->read_pending = 0;
        msc->write_bh.b_private = msc->read_bh.b_private = function_instance;
        msc->read_bh.b_page = alloc_page(GFP_NOIO);     // XXX ensure that this gets de-allocated
        msc->write_bh.b_page = alloc_page(GFP_NOIO);    // XXX ensure that this gets de-allocated
        msc->read_bh.b_data = page_address(msc->read_bh.b_page);
        msc->write_bh.b_data = page_address(msc->write_bh.b_page);

        msc->write_bh.b_rdev = msc->read_bh.b_rdev = msc->dev;
#endif /* defined(LINUX26) */

                qqq=bdev_get_queue(msc->bdev);
                if(!(qqq->queue_lock)) qqq->queue_lock=&msc_lock;

        CATCH(ejected) {
                TRACE_MSG1(MSC,"OPEN BLOCKDEV: EJECTED block_dev_state: %x", msc->block_dev_state);
                printk(KERN_INFO"%s: Cannot get device %d %d\n", __FUNCTION__, msc->major, msc->minor);
        }
        up(&msc_sem);
}

/*! msc_close_blockdev - close the device for host
 * @param function_instance - pointer to this function instance
 * @return none
 */

void msc_close_blockdev(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata;

        RETURN_IF(msc->block_dev_state == DEVICE_EJECTED);

        // XXX this should be a wait for read_pending/write_pending to go to ZERO

#if defined(LINUX26)

        {
                static DECLARE_WAIT_QUEUE_HEAD(msc1_wq);
                printk(KERN_INFO"%s: read_pending:%x,write_pending:%x\n", __FUNCTION__,msc->read_pending,msc->write_pending);
                wait_event_interruptible(msc1_wq, ((msc->read_pending+msc->write_pending)==0));
        }

#else /* defined(LINUX26) */

        while (msc->read_pending || msc->write_pending) {
                printk(KERN_INFO"%s: sleeping on read or write bh\n", __FUNCTION__);
                sleep_on_timeout(&msc->msc_wq, 20);
        }

#endif /* defined(LINUX26) */

        down(&msc_sem);
        msc->block_dev_state = DEVICE_EJECTED;

#if defined(LINUX26)
        if (msc->bdev)
                blkdev_put(msc->bdev);
#else /* defined(LINUX26) */
        if (msc->bdev)
                blkdev_put(msc->bdev, BDEV_RAW);
#endif /* defined(LINUX26) */

#if defined(LINUX26)
        __free_page(msc->rbio_vec.bv_page);
        msc->rbio_vec.bv_page = NULL;
        __free_page(msc->wbio_vec.bv_page);
        msc->wbio_vec.bv_page = NULL;

#else /* defined(LINUX26) */
        __free_page((void *)&msc->read_bh.b_page);
        msc->read_bh.b_page = NULL;
        __free_page((void *)&msc->write_bh.b_page);
        msc->write_bh.b_page = NULL;
#endif /* defined(LINUX26) */
        up(&msc_sem);
}


/* READ 10 COMMAND - read and send data to the host ******************************************** */

int msc_start_reading_block_data(struct usbd_function_instance *function);

#if defined(LINUX26)

static int  msc_block_read_finished(struct bio *bio,unsigned int bytes_done,int err);

#else /* defined(LINUX26) */

void msc_block_read_finished(struct buffer_head *bh, int flag);

#endif /* defined(LINUX26) */

/*! msc_scsi_read_10 - process a read(10) command
 *
 * We have received a READ(10) CBW, if transfer length is non-zero
 * initiate a generic block i/o otherwise send a CSW.
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_read_10(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = function_instance->privdata;
        struct msc_scsi_read_10_command *command = (struct msc_scsi_read_10_command *)&msc->command.CBWCB;

        /*
         * save the CBW information and setup for transmitting data read from the block device
         */

        msc->lba = be32_to_cpu(command->LogicalBlockAddress);
        TRACE_MSG1(MSC,"sos msc->lba:%d",msc->lba);
        msc->TransferLength_in_blocks = be16_to_cpu(command->TransferLength);
        msc->TransferLength_in_bytes = msc->TransferLength_in_blocks * msc->block_size;
        msc->command_state = MSC_DATA_IN_READ;
        msc->io_state = MSC_INACTIVE;

        /*Case 1: If Host Expect no data, while device has data, then report phase error directly */

        if((msc->command.dCBWDataTransferLength==0) && (msc->TransferLength_in_blocks>0)){
                usbd_halt_endpoint(function_instance, BULK_IN);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = MSC_CBW_PHASE_ERROR;
                return 0;
        }

        /*Case 2:simply send the CSW if the host didn't actually ask for a non-zero length.*/

        if((msc->command.dCBWDataTransferLength==0) && (msc->TransferLength_in_blocks==0)){
                return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
        }

        /*Case 3: If Host Expect data, while device has no data, then device stalls BulkIn and  put to CBW_PASSED state */
        if((msc->command.dCBWDataTransferLength>0) && (msc->TransferLength_in_blocks==0)){

                usbd_halt_endpoint(function_instance, BULK_IN);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = MSC_CBW_PASSED;
                return 0;
        }

        /* Case 10 Host expects data OUT, but  Device reads data: Stall BulkOut and CBW phase error */
        if ((msc->TransferLength_in_blocks) && (msc->command.bmCBWFlags & 0x80)==0){
                usbd_halt_endpoint(function_instance, BULK_OUT);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = MSC_CBW_PHASE_ERROR;
                return 0;
        }
         /* Host expects data and Device has to read data: Start reading blocks to send */

        if ((msc->TransferLength_in_blocks) && (msc->command.bmCBWFlags & 0x80)){
                return msc_start_reading_block_data(function_instance);

        }
         return 0;
/*
        return (msc->TransferLength_in_blocks) ?  msc_start_reading_block_data(function_instance) :
                msc_start_sending_csw(function_instance, USB_MSC_PASSED);

*/
}


/*! msc_start_reading_block_data - start reading data
 *
 * Generate a generic block io request to read some data to transmit.
 *
 * This function is initially called by msc_scsi_read_10() but can also be called
 * by msc_urb_sent() if the amount of data requested was too large.
 *
 * The function msc_block_read_finished() will be called to actually send the data
 * to the host when the I/O request is complete.
 *
 */
int msc_start_reading_block_data(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata;
        int TransferLength_in_blocks = MIN(msc->max_blocks, msc->TransferLength_in_blocks);
        unsigned long flags;
        char *btemp_data;
        request_queue_t *qqq;
        /*TRACE_MSG2(MSC,"START READING BLOCK DATA lba:    %x blocks: %d %d ",
          msc->lba, msc->TransferLength_in_blocks);
         */
        if(msc->TransferLength_in_blocks<=0)
                printk(KERN_INFO "msc->TransferLength_in_blocks111=0\n");
        /*else
          printk(KERN_INFO "START READING BLOCK DATA lba111:    %x blocks: %d \n",msc->lba, msc->TransferLength_in_blocks);
         */
        /* ensure that device state is ok
         */
        if (msc->block_dev_state != DEVICE_INSERTED) {
                TRACE_MSG0(MSC,"START READ MEDIA NOT PRESENT");
                return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_MEDIA_NOT_PRESENT,
                                msc->lba, USB_MSC_FAILED);
        }

        // XXX an ioctl has requested that pending io be aborted
        local_irq_save(flags);
        if (msc->io_state & MSC_ABORT_IO) {
                msc->io_state &= ~MSC_ABORT_IO;
                TRACE_MSG0(MSC,"BLOCK READ ABORTED");
                local_irq_restore(flags);
                return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR,
                                msc->lba, USB_MSC_FAILED);
        }
        local_irq_restore(flags);

        /* sanity check lba against capacity
         */
        if (msc->lba >= msc->capacity) {
                TRACE_MSG2(MSC, "START READ LBA out of range: lba: %d capacity: %d", msc->lba, msc->capacity);
                return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE, msc->lba, USB_MSC_FAILED);
        }

        /* setup buffer head - msc_block_read_finished() will be called when block i/o is finished
         */

#if defined(LINUX26)
        if(TransferLength_in_blocks<=0)
                printk(KERN_INFO "TransferLength_in_blocks==0");

        /*else
          printk(KERN_INFO "Start reading blocks: %x\n", TransferLength_in_blocks);*/


        msc->rbio_vec.bv_len = TransferLength_in_blocks * msc->block_size;;
        msc->rbio_vec.bv_offset = 0;
        msc->read_bio.bi_vcnt = 1;
        msc->read_bio.bi_idx = 0;
        msc->read_bio.bi_size = TransferLength_in_blocks * msc->block_size;
        msc->read_bio.bi_bdev = msc->bdev;
        msc->read_bio.bi_sector = msc->lba;
        msc->read_bio.bi_end_io = msc_block_read_finished;
        msc->io_state |= MSC_BLOCKIO_PENDING;
        msc->read_pending=1;


        //btemp_data = page_address(msc->read_bio.bi_io_vec->bv_page);

        btemp_data=__bio_kmap_atomic( &msc->read_bio,0,KM_USER0);

        memset(btemp_data, 0x0, msc->read_bio.bi_size);
        __bio_kunmap_atomic(btemp_data,KM_USER0);

        //generic_make_request(READ, &msc->read_bio);
        if (msc->read_bio.bi_size){

                /* printk(KERN_INFO "Start reading bio->size: %x\n", msc->read_bio.bi_size); */
                submit_bio(READ, &msc->read_bio);
                generic_unplug_device(bdev_get_queue(msc->bdev));
        }
        else
        {
                printk(KERN_INFO "msc->read_bio.bi_size=0\n");
        }


#else /* defined(LINUX26) */

        msc->read_bh.b_end_io = msc_block_read_finished;
        msc->read_bh.b_size = TransferLength_in_blocks * msc->block_size;
        msc->read_bh.b_rsector = msc->lba;
        msc->read_bh.b_state = (1UL << BH_Mapped) | (1UL << BH_Lock);
        msc->io_state |= MSC_BLOCKIO_PENDING;
        msc->read_pending=1;
        memset(msc->read_bh.b_data, 0x0, msc->read_bh.b_size);
        generic_make_request(READ, &msc->read_bh);
        generic_unplug_device(blk_get_queue(msc->read_bh.b_rdev));

#endif /* defined(LINUX26) */
        return 0;
}


/*! msc_block_read_finished - called by generic request
 *
 * Called when block i/o read is complete, send the data to the host if possible.
 *
 * The function msc_urb_sent() will be called when the data is sent to
 * either send additional data or the CSW as appropriate.
 *
 * If more data is required then call msc_start_reading_block_data() to
 * issue another block i/o to get more data.
 *
 * These means that there can be two outstanding actions when this
 * function completes:
 *
 *      1. a transmit urb may be pending, sending the most recently
 *         read data to the host
 *      2. another block i/o may be pending to read additional data
 *
 * This leads to a race condition, if the block i/o finished before the urb
 * transmit, then we must simply exit. The msc_in_read_10_urb_sent()
 * function will ensure that this is restarted.
 */

#if defined(LINUX26)

static int  msc_block_read_finished(struct bio *bio,unsigned int bytes_done,int err)
{
        struct usbd_function_instance *function_instance = bio->bi_private;
        struct msc_private *msc = function_instance->privdata;
        //int TransferLength_in_blocks = bh->b_size / msc->block_size;

        int TransferLength_in_blocks;

        struct usbd_urb *tx_urb;
        unsigned long flags;
        int rc;
        int i;
        int bytes_done1;
        char *btemp_data;

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        u32 crc;
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */

        if(bio->bi_size)
                return 1;

        //msc = bh->b_private;
        TransferLength_in_blocks = bytes_done / msc->block_size;

        TRACE_MSG1(MSC," bio BLOCK READ FINISHED size: %x", bytes_done);

        wake_up_interruptible(&msc->ioctl_wq);

        /*
         * Race condition here, if we have not finished sending the
         * previous tx_urb then we want to simply remmber the parameters ,exit and let
         * msc_in_read_10_urb_sent() call us again.
         *
         * Ensure that we do not reset BLOCKIO flags if SEND PENDING and
         * that we do reset BLOCKIO if not SEND PENDING
         */
        {
                unsigned long flags;
                local_irq_save(flags);
                if (msc->io_state & MSC_SEND_PENDING) {
                        TRACE_MSG0(MSC,"BLOCK READ SEND PENDING");
                        msc->io_state |= MSC_BLOCKIO_FINISHED;
                        msc->bytes_done = bytes_done;
                        msc->err = err;
                        local_irq_restore(flags);
                        return 0 ;
                }
                msc->io_state &= ~(MSC_BLOCKIO_PENDING | MSC_BLOCKIO_FINISHED);
                local_irq_restore(flags);
        }

        /* verify that the I/O completed
         */
        if (err) {
                TRACE_MSG0(MSC,"BLOCK READ FAILED");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);
                return 0;
        }

        bytes_done1=bytes_done;

        /* allocate a tx_urb and copy into it
         */
        //THROW_IF(!(function = msc->function), error);

        /* To see if enough data has been read. We only send as many as Host expects and don't read more*/

        if ((msc->data_transferred_in_bytes+bytes_done)>msc->command.dCBWDataTransferLength){
                bytes_done1=msc->command.dCBWDataTransferLength - msc->data_transferred_in_bytes;
        }

        THROW_IF(!(tx_urb = usbd_alloc_urb (function_instance, BULK_IN, bytes_done1, msc_urb_sent)), error);
        TRACE_MSG2(MSC,"bio BLOCK READ FINISHED urb: %p bytes_done:%d ", (int)tx_urb, bytes_done1);

        tx_urb->function_privdata = function_instance;
        tx_urb->actual_length = tx_urb->buffer_length = bytes_done1;


        //btemp_data = page_address(msc->read_bio.bi_io_vec->bv_page);
        btemp_data=__bio_kmap_atomic(&msc->read_bio,0,KM_USER0);


#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        TRACE_MSG0(MSC,"sos CONFIG_OTG_MSC_BLOCK_TRACE");
        /* debug output trace - dump rlba and computed crc
         */
        for (i = 0; i < bytes_done / msc->block_size; i++) {
                crc = crc32_compute(btemp_data + (i * msc->block_size), msc->block_size, CRC32_INIT);
                TRACE_RLBA(bio->bi_sector + i, crc);
        }
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */

        memcpy(tx_urb->buffer, btemp_data, bytes_done1);

        __bio_kunmap_atomic(btemp_data,KM_USER0);



        msc->read_pending=0;

        {
                unsigned long flags;
                local_irq_save(flags);

                msc->io_state |= MSC_SEND_PENDING;
                msc->TransferLength_in_bytes -= bytes_done1;
                msc->TransferLength_in_blocks -= TransferLength_in_blocks;
                msc->lba += TransferLength_in_blocks;

                if ((!msc->TransferLength_in_blocks) || (bytes_done>bytes_done1)) {
                        TRACE_MSG0(MSC,"bio BLOCK READ FINISHED - IO FINISHED");
                        // set flag so that CSW can be sent
                        msc->io_state |= MSC_DATA_IN_READ_FINISHED;
                }
                local_irq_restore(flags);
        }

        /* dispatch urb - msc_urb_sent() will be called when urb is finished
         */
        if ((rc = usbd_start_in_urb (tx_urb))) {
                TRACE_MSG0(MSC,"BLOCK READ FINISHED FAILED");
                usbd_free_urb (tx_urb);
                THROW(error);
        }

        /* if more data is required then call msc_start_reading_block_data() to
         * issue another block i/o to get more data.
         */
        if (!(msc->io_state & MSC_DATA_IN_READ_FINISHED)) {
                TRACE_MSG0(MSC,"BLOCK READ SEND RESTARTING");
                msc_start_reading_block_data(function_instance);
        }

        CATCH(error) {
                TRACE_MSG0(MSC,"BLOCK READ FINISHED ERROR");
                // stall?
        }
        return 0;
}




#else /* defined(LINUX26) */
void msc_block_read_finished(struct buffer_head *bh, int uptodate)
{
        struct usbd_function_instance *function_instance = bh->b_private;
        struct msc_private *msc = function_instance->privdata;
        //int TransferLength_in_blocks = bh->b_size / msc->block_size;

        int TransferLength_in_blocks;

        struct usbd_urb *tx_urb;
        unsigned long flags;
        int rc;
#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        u32 crc;
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
        int i;

        //msc = bh->b_private;
        TransferLength_in_blocks = bh->b_size / msc->block_size;

        TRACE_MSG1(MSC,"BLOCK READ FINISHED size: %x", bh->b_size);

        wake_up_interruptible(&msc->ioctl_wq);

        /*
         * Race condition here, if we have not finished sending the
         * previous tx_urb then we want to simply exit and let
         * msc_in_read_10_urb_sent() call us again.
         *
         * Ensure that we do not reset BLOCKIO flags if SEND PENDING and
         * that we do reset BLOCKIO if not SEND PENDING
         */
        {
                unsigned long flags;
                local_irq_save(flags);
                if (msc->io_state & MSC_SEND_PENDING) {
                        TRACE_MSG0(MSC,"BLOCK READ SEND PENDING");
                        msc->io_state |= MSC_BLOCKIO_FINISHED;
                        msc->uptodate = uptodate;
                        local_irq_restore(flags);
                        return;
                }
                msc->io_state &= ~(MSC_BLOCKIO_PENDING | MSC_BLOCKIO_FINISHED);
                local_irq_restore(flags);
        }

        /* verify that the I/O completed
         */
        if (1 != uptodate) {
                TRACE_MSG0(MSC,"BLOCK READ FAILED");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);
                return;
        }

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        /* debug output trace - dump rlba and computed crc
         */
        for (i = 0; i < bh->b_size / msc->block_size; i++) {
                crc = crc32_compute(bh->b_data + (i * msc->block_size), msc->block_size, CRC32_INIT);
                TRACE_RLBA(bh->b_rsector + i, crc);
        }
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
        /* allocate a tx_urb and copy into it
         */
        //THROW_IF(!(function = msc->function), error);
        THROW_IF(!(tx_urb = usbd_alloc_urb (function_instance, BULK_IN, bh->b_size, msc_urb_sent)), error);
        TRACE_MSG1(MSC,"BLOCK READ FINISHED urb: %p", (int)tx_urb);

        tx_urb->function_privdata = function_instance;
        tx_urb->actual_length = tx_urb->buffer_length = bh->b_size;
        memcpy(tx_urb->buffer, bh->b_data, bh->b_size);

        msc->read_pending=0;

        {
                unsigned long flags;
                local_irq_save(flags);

                msc->io_state |= MSC_SEND_PENDING;
                msc->TransferLength_in_bytes -= bh->b_size;
                msc->TransferLength_in_blocks -= TransferLength_in_blocks;
                msc->lba += TransferLength_in_blocks;

                if (!msc->TransferLength_in_blocks) {
                        TRACE_MSG0(MSC,"BLOCK READ FINISHED - IO FINISHED");
                        // set flag so that CSW can be sent
                        msc->io_state |= MSC_DATA_IN_READ_FINISHED;
                }
                local_irq_restore(flags);
        }

        /* dispatch urb - msc_urb_sent() will be called when urb is finished
         */
        if ((rc = usbd_start_in_urb (tx_urb))) {
                TRACE_MSG0(MSC,"BLOCK READ FINISHED FAILED");
                usbd_free_urb (tx_urb);
                THROW(error);
        }

        /* if more data is required then call msc_start_reading_block_data() to
         * issue another block i/o to get more data.
         */
        if (!(msc->io_state & MSC_DATA_IN_READ_FINISHED)) {
                TRACE_MSG0(MSC,"BLOCK READ SEND RESTARTING");
                msc_start_reading_block_data(function_instance);
        }

        CATCH(error) {
                TRACE_MSG0(MSC,"BLOCK READ FINISHED ERROR");
                // stall?
        }
}

#endif /* defined(LINUX26) */


/*! msc_in_read_10_urb_sent - called by msc_urb_sent when state is MSC_DATA_IN_READ
 *
 * This will process a read_10 urb that has been sent. Specifically if any previous
 * read_10 block I/O has finished we recall the msc_block_read_finished() function
 * to transmit another read_10 urb.
 *
 * If there is no other pending read_10 to do we create and send a CSW.
 *
 * If there is more I/O to do or there is already an outstanding I/O we simply
 * return after freeing the URB.
 *
 * Return non-zero if urb was not disposed of.
 */
int msc_in_read_10_urb_sent(struct usbd_urb *tx_urb)
{
        struct usbd_function_instance *function_instance = tx_urb->function_privdata;
        struct msc_private *msc = function_instance->privdata;
        unsigned long flags;

        TRACE_MSG0(MSC,"URB SENT DATA IN");

        /*
         * Potential race condition here, we may need to restart blockio.
         */
        local_irq_save(flags);

        msc->io_state &= ~MSC_SEND_PENDING;
        msc->data_transferred_in_bytes += tx_urb->actual_length;

        TRACE_MSG1(MSC,"URB SENT DATA IN data transferred: %d", msc->data_transferred_in_bytes);

        if (!tx_urb->actual_length)
                msc->TransferLength_in_blocks = 0;

        /* XXX We should be checking urb status
         */

        usbd_free_urb (tx_urb);

        /*
         * Check to see if we need to send CSW.
         */
        if (MSC_DATA_IN_READ_FINISHED & msc->io_state) {
                /* Case 5: Host expect more data Hi>Di  Then stall BulkIn ,later send CSW*/
                if (msc->data_transferred_in_bytes < msc->command.dCBWDataTransferLength){
                        usbd_halt_endpoint(function_instance, BULK_IN);
                        msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                        msc->command_state = MSC_CBW_PASSED;
                        local_irq_restore(flags);
                        return 0;
                }

                /* Case 6:  Hi = Di */
                if (msc->TransferLength_in_bytes==0){
                        TRACE_MSG0(MSC,"URB SENT DATA IN - FINISHED SENDING CSW");
                        local_irq_restore(flags);
                        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
                }

                /*Case 7 Host expects less data Hi < Di, Then Stall after transfer exected length */

                if (msc->TransferLength_in_bytes>0){
                        usbd_halt_endpoint(function_instance, BULK_IN);
                        msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                        msc->command_state = MSC_CBW_PHASE_ERROR;
                        local_irq_restore(flags);
                        return 0;
                }
        }
        /*
         * Check to see if there is a block read needs to be finished.
         */
        if (MSC_BLOCKIO_FINISHED & msc->io_state) {
                TRACE_MSG0(MSC,"URB SENT DATA IN - RESTART");
                local_irq_restore(flags);
                // XXX uptodate?
#if defined(LINUX26)
                msc_block_read_finished(&msc->read_bio, msc->bytes_done, msc->err);
#else /* defined(LINUX26) */
                msc_block_read_finished(&msc->read_bh, msc->uptodate);
#endif /* defined(LINUX26) */
                return 0;
        }
        local_irq_restore(flags);
        return 0;
}


/* WRITE 10 - receive data from host and write to block device ********************************* */

int msc_start_receiving_data(struct usbd_function_instance *function);

#if defined(LINUX26)
static int msc_data_written(struct bio *bio, unsigned bytes_done,int err);
#else /* defined(LINUX26) */
void msc_data_written(struct buffer_head *bh, int flag);
#endif /* defined(LINUX26) */

void msc_data_test(struct buffer_head *bh, int flag);

/*! int msc_scsi_write_10(struct usbd_function_instance*, char* ,int )
 * @brief - process a write command
 *
 * Call either msc_start_receiving_data() or msc_start_sending_csw() as appropriate.
 * Normally msc_start_receiving_data() is called and it will use msc_start_recv_urb()
 * to setup a receive urb of the appropriate size.
 *
 * @param function_instance - pointer to this function instance
 * @param name
 * @param op
 * @return  non-zero if there is an error in the USB layer.
 */
int msc_scsi_write_10(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = function_instance->privdata;
        struct msc_scsi_write_10_command *command = (struct msc_scsi_write_10_command *)&msc->command.CBWCB;

        /* save the CBW and setup for receiving data to be written to the block device
         */
        msc->lba = be32_to_cpu(command->LogicalBlockAddress);
        msc->TransferLength_in_blocks = be16_to_cpu(command->TransferLength);
        msc->TransferLength_in_bytes = msc->TransferLength_in_blocks * msc->block_size;

        msc->command_state = MSC_DATA_OUT_WRITE;
        msc->io_state = MSC_INACTIVE;

        TRACE_MSG1(MSC,"RECV WRITE lba: %x", msc->lba);
        TRACE_MSG1(MSC,"RECV WRITE blocks: %d", msc->TransferLength_in_blocks);


         /*Case 3:  If Host Expect no data, device writes data Hn < Do then report phase error directly */

        if((msc->command.dCBWDataTransferLength==0) && (msc->TransferLength_in_blocks>0)){
                usbd_halt_endpoint(function_instance, BULK_IN);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = MSC_CBW_PHASE_ERROR;
                return 0;
        }

        /*Case9: Host sends data, but Device doesn't expect to rcv data*/

        if((msc->command.dCBWDataTransferLength>0) && (msc->TransferLength_in_blocks==0)){
                usbd_halt_endpoint(function_instance, BULK_OUT);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = USB_MSC_PASSED;
                return 0;
        }

        /* Case 8  Host expect data IN, but  Device writes data: Stall BulkIn and CBW phase error */
        if ((msc->TransferLength_in_blocks) && (msc->command.bmCBWFlags & 0x80)){
                usbd_halt_endpoint(function_instance, BULK_IN);
                msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                msc->command_state = MSC_CBW_PHASE_ERROR;
                return 0;
        }

        /*Case 1: Hn = Dn */

         if((msc->command.dCBWDataTransferLength==0) && (msc->TransferLength_in_blocks==0)){
                return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
        }

        return  msc_start_receiving_data(function_instance);

}


/*! msc_start_receiving_data(struct usbd_function_instance * )
 * @brief - called to initiate an urb to receive WRITE(10) data
 *
 * Initiate a receive urb to receive upto PAGE_SIZE WRITE(10) data. The
 * msc_recv_urb() function will call msc_recv_out_blocks() to actually
 * write the data.
 *
 * This is called from msc_scsi_write_10() to initiate the WRITE(10) and
 * called from msc_data_written() to start the next page sized receive
 * urb.
 *
 * @param function_instance
 * @return non-zero for error
 */
int msc_start_receiving_data(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata;
        u32 minmumbytes=0;
        /*
         * Calculating the length is most of the work we do :-)
         */
        int TransferLength_in_blocks = MIN(msc->max_blocks, msc->TransferLength_in_blocks);

        TRACE_MSG1(MSC,"START RECEIVING DATA lba: %x", msc->lba);
        TRACE_MSG1(MSC,"START RECEIVING DATA blocks: %d", msc->TransferLength_in_blocks);
        TRACE_MSG1(MSC,"START RECEIVING DATA blocks: %d", TransferLength_in_blocks);
        TRACE_MSG1(MSC,"START RECEIVING DATA bytes: %d", TransferLength_in_blocks * msc->block_size);

/* Ensure Host will send the length data*/
        minmumbytes=
MIN((msc->command.dCBWDataTransferLength - msc->data_transferred_in_bytes),TransferLength_in_blocks * msc->block_size);

        THROW_IF(msc->command_state != MSC_DATA_OUT_WRITE, error);
        THROW_IF(!msc->TransferLength_in_blocks, error);

        msc->io_state |= MSC_RECV_PENDING;

        //return msc_start_recv_urb(function_instance, TransferLength_in_blocks * msc->block_size);
        return msc_start_recv_urb(function_instance, minmumbytes);
        CATCH(error) {
                TRACE_MSG0(MSC,"START RECEIVING DATA ERROR");
                return -EINVAL;
        }
}


/*! void  msc_recv_out_blocks(struct usbd_urb )
 * @brief - process received WRITE(10) data by writing to block device
 *
 * We get here indirectly from msc_recv_urb() when state is MSC_DATA_OUT_WRITE.
 *
 * Dealloc the urb and call Initiate the generic request to write the
 * received data. The b_end_io function msc_data_written() will send the
 * CSW.
 *
 * @param rcv_urb - pointer to recv urb
 * @return none
 *
 */
void msc_recv_out_blocks(struct usbd_urb *rcv_urb)
{
        struct usbd_function_instance *function_instance = rcv_urb->function_privdata;
        struct msc_private *msc = function_instance->privdata;
        int TransferLength_in_bytes = rcv_urb->actual_length;
        int TransferLength_in_blocks = TransferLength_in_bytes / msc->block_size;
#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        u32 crc;
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
        int i;
#if defined(LINUX26)
        char *btemp_data;
#endif
        TRACE_MSG1(MSC,"RECV OUT            bytes: %d", TransferLength_in_bytes);
        TRACE_MSG1(MSC,"RECV OUT          iostate: %x", msc->io_state);
        TRACE_MSG1(MSC,"RECV OUT data transferred: %d", msc->data_transferred_in_bytes);
        //if(TransferLength_in_bytes)
        //printk(KERN_INFO "sos0 TransferLength_in_bytes: %x\n", TransferLength_in_bytes);
        //      else
        //printk(KERN_INFO "sos0 write TransferLength_in_bytes = 0\n");


        /*
         * Race condition here, we may get to here before the previous block
         * write completed. If so just exit and the msc_data_written()
         * function will recall us.
         */
        {
                unsigned long flags;
                local_irq_save(flags);
                if (msc->io_state & MSC_BLOCKIO_PENDING) {
                        TRACE_MSG0(MSC,"RECV OUT  BLOCKIO PENDING");
                        msc->io_state |= MSC_RECV_FINISHED;
                        msc->rcv_urb_finished = rcv_urb;
                        local_irq_restore(flags);
                        return;
                }
                msc->io_state &= ~(MSC_RECV_PENDING |MSC_RECV_FINISHED);
                local_irq_restore(flags);
        }

        msc->data_transferred_in_bytes += rcv_urb->actual_length;

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        for (i = 0; i < TransferLength_in_blocks; i++) {
                crc = crc32_compute(rcv_urb->buffer + (i * msc->block_size), msc->block_size, CRC32_INIT);
                TRACE_SLBA(msc->lba + i, crc);
        }
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */

        msc->write_pending=1;
#if defined(LINUX26)

        btemp_data=__bio_kmap_atomic( &msc->write_bio,0,KM_USER0);

        //btemp_data = page_address(msc->write_bio.bi_io_vec->bv_page);
        memcpy(btemp_data,rcv_urb->buffer,  TransferLength_in_bytes);

        __bio_kunmap_atomic(btemp_data,KM_USER0);
#else
        memcpy(msc->write_bh.b_data, rcv_urb->buffer, rcv_urb->actual_length);
#endif
        usbd_free_urb(rcv_urb);

        /* ensure media state is ok
         */
        if (msc->block_dev_state != DEVICE_INSERTED) {
                TRACE_MSG0(MSC,"START READ MEDIA NOT PRESENT");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_MEDIA_NOT_PRESENT, msc->lba, USB_MSC_FAILED);
                return;
        }

        // XXX an ioctl has requested that pending io be aborted
        if (msc->io_state & MSC_ABORT_IO) {
                unsigned long flags;
                local_irq_save(flags);
                msc->io_state &= ~MSC_ABORT_IO;
                TRACE_MSG0(MSC,"BLOCK READ ABORTED");
                local_irq_restore(flags);
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);
                return;
        }

        /* sanity check lba against capacity
         */
        if (msc->lba >= msc->capacity) {
                TRACE_MSG2(MSC, "START READ LBA out of range: lba: %d capacity: %d", msc->lba, msc->capacity);
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE, msc->lba, USB_MSC_FAILED);
                return;
        }

        /* additional sanity check for lba - ensure non-zero
         */
        if (!msc->TransferLength_in_blocks) {
                TRACE_MSG0(MSC,"START READ LBA TransferLength_in_blocks zero:");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE, msc->lba, USB_MSC_FAILED);
                return;
        }

        /* XXX additional sanity check required here - verify that the transfer
         * size agrees with what we where expecting, specifically I think
         * we need to verify that we have received a multiple of the block
         * size and either a full bulk transfer (so more will come) or
         * the actual exact amount that the host said it would send.
         * An error should generate a CSW with a USB_MSC_PHASE_ERROR
         */


        TRACE_MSG1(MSC,"RECV OUT              lba: %x", msc->lba);
        TRACE_MSG1(MSC,"RECV OUT   blocks    left: %d", msc->TransferLength_in_blocks);
        TRACE_MSG1(MSC,"RECV OUT   blocks current: %d", TransferLength_in_blocks);

        /* Initiate writing the data - msc_data_written() will be called
         * when finished.
         */
        //if(TransferLength_in_bytes)
        //printk(KERN_INFO "sos TransferLength_in_bytes: %x\n", TransferLength_in_bytes);
        //      else
        //printk(KERN_INFO"sos write TransferLength_in_bytes = 0\n");

#if defined(LINUX26)

        msc->wbio_vec.bv_offset = 0;
        msc->write_bio.bi_vcnt = 1;
        msc->write_bio.bi_idx = 0;
        msc->write_bio.bi_size = TransferLength_in_bytes;
        msc->wbio_vec.bv_len = TransferLength_in_bytes;
        msc->write_bio.bi_bdev = msc->bdev;
        msc->write_bio.bi_sector = msc->lba;
        msc->write_bio.bi_end_io = msc_data_written;
        msc->io_state |= MSC_BLOCKIO_PENDING;
        msc->write_pending=1;
#if 0
        /* decrement counters and increment address
         */
        msc->TransferLength_in_bytes -= TransferLength_in_bytes;
        msc->TransferLength_in_blocks -= TransferLength_in_blocks;
        msc->lba += TransferLength_in_blocks;

        /* Check current status of TransferLength - if non-zero then we need
         * to queue another urb to receive more data.
         */
        //if (!msc->TransferLength_in_blocks)
        if ( msc->data_transferred_in_bytes > msc->command.dCBWDataTransferLength
                msc->command_state = MSC_DATA_OUT_WRITE_FINISHED;
        else
                msc_start_receiving_data(function_instance);
#else
         /* Check current status of TransferLength - if non-zero then we need
         * to queue another urb to receive more data.
         */
        //if (!msc->TransferLength_in_blocks)

        /*Case 13 Ho < Do */
        if((msc->data_transferred_in_bytes >= msc->command.dCBWDataTransferLength) &&
        (msc->TransferLength_in_bytes>TransferLength_in_bytes)) {
                msc->command_state = MSC_DATA_OUT_WRITE_FINISHED;
                TRACE_MSG0( MSC, "Case 13");
                msc->caseflag=13;
        }

        /*Case 11 Ho > Do */
        else if ( msc->TransferLength_in_blocks <=TransferLength_in_blocks){

                msc->command_state = MSC_DATA_OUT_WRITE_FINISHED;
                if(msc->command.dCBWDataTransferLength>msc->data_transferred_in_bytes) {
                        msc->write_bio.bi_size =  msc->TransferLength_in_bytes;
                        msc->wbio_vec.bv_len = msc->TransferLength_in_bytes;
                        msc->caseflag=11;
                        TRACE_MSG0( MSC, "Case 11");
                }else{
                        /* Case 12 Ho = Do */
                        msc->caseflag=12;
                        TRACE_MSG0( MSC, "Case 12");
                }
        }


        else {


         /* decrement counters and increment address
         */
                msc->TransferLength_in_bytes -= TransferLength_in_bytes;
                msc->TransferLength_in_blocks -= TransferLength_in_blocks;
                msc->lba += TransferLength_in_blocks;
                msc_start_receiving_data(function_instance);
        }


#endif

        /* initiate the block i/o request
         */
        if(msc->write_bio.bi_size){
                /*printk(KERN_INFO"write OUT bio->size: %x\n", msc->write_bio.bi_size);*/
                submit_bio(WRITE, &msc->write_bio);
                generic_unplug_device(bdev_get_queue(msc->bdev));
        }
        else
        {
                printk(KERN_INFO"write OUT bio->size=0\n");
        }



#else /* defined(LINUX26) */

        msc->write_bh.b_end_io = msc_data_written;

        /* set address in buffer head
         */
        msc->write_bh.b_size = TransferLength_in_bytes;
        msc->write_bh.b_rsector = msc->lba;

        /* decrement counters and increment address
         */
        msc->TransferLength_in_bytes -= TransferLength_in_bytes;
        msc->TransferLength_in_blocks -= TransferLength_in_blocks;
        msc->lba += TransferLength_in_blocks;

        msc->write_bh.b_state = (1UL << BH_Mapped) | (1UL << BH_Lock);

        msc->io_state |= MSC_BLOCKIO_PENDING;


        /* Check current status of TransferLength - if non-zero then we need
         * to queue another urb to receive more data.
         */
        if (!msc->TransferLength_in_blocks)
                msc->command_state = MSC_DATA_OUT_WRITE_FINISHED;
        else
                msc_start_receiving_data(function_instance);

        /* initiate the block i/o request
         */

        generic_make_request(WRITE, &msc->write_bh);
        generic_unplug_device(blk_get_queue(msc->write_bh.b_rdev));
#endif /* defined(LINUX26) */
}


/*! int msc_data_written(struct bio *, unsigned , int)
 * @brief - called by generic request
 *
 * Called when current block i/o read is finished, restart block i/o or send the CSW.
 *
 * @param bh -
 * @param uptodate -
 * @return 0
 */
#if defined(LINUX26)
static int msc_data_written(struct bio *bio, unsigned bytes_done,int err)
{
        struct usbd_function_instance *function_instance = bio->bi_private;
        struct msc_private *msc = function_instance->privdata;
        unsigned long flags;
        u8 io_state;
        if(bio->bi_size)
                return 1;
        /* printk(KERN_INFO "DATA WRITTEN bytes_done: %x\n", bytes_done);*/

        TRACE_MSG4(MSC,"DATA WRITTEN bytes_done: %x err: %x state: %x io: %x",
                        bytes_done, err, msc->command_state, msc->io_state);

        TRACE_MSG1(MSC,"DATA WRITTEN lba: %x", msc->lba);

        local_irq_save(flags);
        io_state = msc->io_state &= ~MSC_BLOCKIO_PENDING;
        msc->write_pending=0;
        local_irq_restore(flags);

        wake_up_interruptible(&msc->ioctl_wq);

        /*
         * verify that the I/O completed
         */
        if (err) {
                TRACE_MSG0(MSC,"BLOCK READ FAILED");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);
                // XXX CHECKME
                if (msc->rcv_urb_finished) {
                        usbd_free_urb(msc->rcv_urb_finished);
                        msc->rcv_urb_finished = NULL;
                }
                return 0;
        }

        /*
         * If there was a rcv_urb that was not processed then we need
         * to process it now. This is done by simply restarting msc_recv_out()
         */
        if ((io_state & MSC_RECV_FINISHED) && msc->rcv_urb_finished) {
                struct usbd_urb *rcv_urb = msc->rcv_urb_finished;
                msc->rcv_urb_finished = NULL;
                TRACE_MSG0(MSC,"DATA WRITTEN RECV FINISHED");
                //printk(KERN_INFO "previuos rcv urb process\n");
                msc_recv_out_blocks(rcv_urb);
        }
        /*
         * DATA_IN mode and no more data to write
         */
        if (MSC_DATA_OUT_WRITE_FINISHED == msc->command_state) {
                // finished, send CSW

                if (msc->caseflag==12){
                        TRACE_MSG0(MSC,"DATA WRITTEN send CSW");
                        msc_start_sending_csw(function_instance, USB_MSC_PASSED);
                        return 0;
                }

                if (msc->caseflag==11){
                        usbd_halt_endpoint(function_instance, BULK_OUT);
                        msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                        msc->command_state = MSC_CBW_PASSED;
                        return 0;
                }
                 if (msc->caseflag==13){
                        usbd_halt_endpoint(function_instance, BULK_IN);
                        msc->data_transferred_in_bytes=msc->TransferLength_in_bytes;
                        msc->command_state = MSC_CBW_PHASE_ERROR;
                        return 0;
                }
        }
        return 0;
}

#else /* defined(LINUX26) */
void msc_data_written(struct buffer_head *bh, int uptodate)
{
        struct usbd_function_instance *function_instance = bh->b_private;
        struct msc_private *msc = function_instance->privdata;
        unsigned long flags;
        u8 io_state;

        TRACE_MSG4(MSC,"DATA WRITTEN uptodate: %x b_state: %x state: %x io: %x",
                        uptodate, bh->b_state, msc->command_state, msc->io_state);

        TRACE_MSG1(MSC,"DATA WRITTEN lba: %x", msc->lba);

        local_irq_save(flags);
        io_state = msc->io_state &= ~MSC_BLOCKIO_PENDING;
        msc->write_pending=0;
        local_irq_restore(flags);

        wake_up_interruptible(&msc->ioctl_wq);

        /*
         * verify that the I/O completed
         */
        if (1 != uptodate) {
                TRACE_MSG0(MSC,"BLOCK READ FAILED");
                msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);
                // XXX CHECKME
                if (msc->rcv_urb_finished) {
                        usbd_free_urb(msc->rcv_urb_finished);
                        msc->rcv_urb_finished = NULL;
                }
                return;
        }

        /*
         * If there was a rcv_urb that was not processed then we need
         * to process it now. This is done by simply restarting msc_recv_out()
         */
        if ((io_state & MSC_RECV_FINISHED) && msc->rcv_urb_finished) {
                struct usbd_urb *rcv_urb = msc->rcv_urb_finished;
                msc->rcv_urb_finished = NULL;
                TRACE_MSG0(MSC,"DATA WRITTEN RECV FINISHED");
                msc_recv_out_blocks(rcv_urb);
        }
        /*
         * DATA_IN mode and no more data to write
         */
        if (MSC_DATA_OUT_WRITE_FINISHED == msc->command_state) {
                // finished, send CSW
                TRACE_MSG0(MSC,"DATA WRITTEN send CSW");
                msc_start_sending_csw(function_instance, USB_MSC_PASSED);
        }
}
#endif /* defined(LINUX26) */
extern struct usbd_interface_driver msc_interface_driver;


/* USB Module init/exit ***************************************************** */

void msc_io_exit_l24(void);

/*! int msc_os_int_l24(struct usbd_function_intance * )
 * @brief - called to initialize msc intance major and minor device number
 *
 * @param function_instance - pointer to this function instance
 * @return 0
 */
int msc_os_init_l24(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata;
        msc->major = MODPARM(major);
        msc->minor = MODPARM(minor);
        return 0;
}

extern void msc_global_init(void);
/*! int msc_modinit ( )
 * @brief - initialize msc global variables, and register interface function driver
 *
 *@return non-zero for error
 */
static int msc_modinit (void)
{
        int rc;

        printk(KERN_INFO "Copyright (c) 2004 Belcarra Technologies; www.belcarra.com; sl@belcarra.com\n");
        printk (KERN_INFO "%s vendor_id: %04x product_id: %04x major: %d minor: %d\n", __FUNCTION__,
                        MODPARM(vendor_id), MODPARM(product_id), MODPARM(major), MODPARM(minor));

#ifndef OTG_C99
#warning "C99"
        msc_global_init();
#endif
        MSC = otg_trace_obtain_tag(NULL, "msc-if");

        //if (vendor_id)
        //        msc_function_driver.idVendor = cpu_to_le16(vendor_id);
        //if (product_id)
        //        msc_function_driver.idProduct = cpu_to_le16(product_id);

        #if 0
        init_waitqueue_head(&msc->msc_wq);
        init_waitqueue_head(&msc->ioctl_wq);

        msc->block_dev_state = DEVICE_EJECTED;


        msc->command_state = MSC_READY;
        msc->io_state = MSC_INACTIVE;
        #endif

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        make_crc_table();
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */

        TRACE_MSG3(MSC,"PAGE_SHIFT: %x PAGE_SIZE: %x %d", PAGE_SHIFT, PAGE_SIZE, PAGE_SIZE);


        /* register as usb function driver
         */
        RETURN_EINVAL_IF(usbd_register_interface_function (&msc_interface_driver, "msc-if", NULL));

        CATCH(error) {
                otg_trace_invalidate_tag(MSC);
                return -EINVAL;
        }
        return 0;
}

/*! msc_modexit - module cleanup
 */
static void msc_modexit (void)
{
        #if 0
        /* destroy control io interface
         */
        msc_io_exit_l24();

        /* flush io and free page buffers
         */
        msc_close_blockdev(msc);
        #endif
        usbd_deregister_interface_function (&msc_interface_driver);

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        free_crc_table();
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
        otg_trace_invalidate_tag(MSC);
}


module_init (msc_modinit);
module_exit (msc_modexit);
