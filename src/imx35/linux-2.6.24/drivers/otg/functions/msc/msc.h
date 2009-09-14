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
 * otg/function/msc/msc.h - Mass Storage Class
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/msc.h|20061218212925|08467
 *
 *      Copyright (c) 2003-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *      Tony Tang <tt@belcarra.com>
 *
 */
/*!
 * @defgroup MSCFunction Mass Storage Interface Function
 * @ingroup InterfaceFunctions
 */
/*!
 * @file otg/functions/msc/msc.h
 * @brief Mass Storage Driver private defines
 *
 *
 * @ingroup MSCFunction
 */

#ifndef MSC_H
#define MSC_H 1

extern otg_tag_t msc_fd_trace_tag;
#define MSC msc_fd_trace_tag

/*
 * Command/Data/Status Flow
 * C.f. 5 - Figure 1
 */

typedef enum msc_state {
        MSC_READY,
        MSC_DATA_OUT_WRITE,
        MSC_DATA_OUT_WRITE_FINISHED,
        MSC_DATA_IN_READ,
        MSC_DATA_IN_READ_FINISHED,
        MSC_STATUS,
        MSC_QUERY,
        MSC_WAITFOR_RESET,
        MSC_CBW_PASSED,
        MSC_CBW_PHASE_ERROR,
        MSC_CBW_FAILED,
        MSC_WAITFOR_CLEAR,
        MSC_UNKNOWN,
} msc_state_t;


/*
 * Device Transfer state
 * C.F. Table 6.1
 */

typedef enum msc_device_state {
        MSC_DEVICE_DN,                  // The device intends to transfer no data
        MSC_DEVICE_DI,                  // The device intends to send data to the host
        MSC_DEVICE_DO,                  // The device intents to received data from the host
} msc_device_state_t;

#define MSC_INACTIVE            0x0000
#define MSC_BLOCKIO_PENDING     0x0001
#define MSC_BLOCKIO_FINISHED    0x0002
#define MSC_RECV_PENDING        0x0010
#define MSC_RECV_FINISHED       0x0020
#define MSC_SEND_PENDING        0x0040
#define MSC_SEND_FINISHED       0x0080
#define MSC_IOCTL_WAITING       0x0100          // there is an ioctl call waiting for I/O completion
#define MSC_ABORT_IO            0x0200          // please abort current i/o

#if 0
struct SPC_inquiry_cdb {
        u8            OperationCode; /* 12H */
        u8            EnableVPD:1;
        u8            CmdSupportData:1;
        u8            Reserved0:6;
        u8            PageCode;
        u8            Reserved1;
        u8            AllocationLen;
        u8            Control;
} __attribute__((packed));
#endif
/*! @struct msc_private   msc.h "otg/functions/msc/msc.h"
 *
 *  @brief  msc private data wrapper
 */
struct msc_private {

        unsigned char           connected;              // non-zero if connected to host (configured)

        struct usbd_urb         *rcv_urb_finished;

#if defined(LINUX26)
        struct bio      read_bio;
        struct bio_vec  rbio_vec;
        struct bio      write_bio;
        struct bio_vec  wbio_vec;
        unsigned int bytes_done;
        int err;

#else /* defined(LINUX26) */
        struct buffer_head      read_bh;
        struct buffer_head      write_bh;
        int                     uptodate;
#endif /* defined(LINUX26) */

        u8                     read_pending;
        u8                     write_pending;
        u8 caseflag;

        msc_device_state_t      device_state;
        msc_state_t             command_state;          // current command state
        u16                     io_state;               // current IO state
        u8                      endpoint_state;         // Bulk in and Bulk out state bit 0 IN, bit 1 OUT. The rest are reserved

        struct msc_command_block_wrapper   command;

        u32                     lba;                    // next lba to read/write from
        u32                     transfer_blocks;
        u32                     TransferLength_in_blocks;       // amount of transfer remaining
        u32                     TransferLength_in_bytes;        // amount of transfer remaining
        u32                     data_transferred_in_bytes;       // amount of data actually transferred

        int                     major;
        int                     minor;
        //kdev_t                  dev;
        dev_t                   dev;
        struct gendisk          *disk;
        struct block_device     *bdev;
        u32                     block_size;
        u32                     capacity;
        u32                     max_blocks;



        wait_queue_head_t       msc_wq;
        wait_queue_head_t       ioctl_wq;

        u32                     status;
        u32                     block_dev_state;
        u32                     sensedata;
        u32                     info;
};


/*
 * MSC Configuration
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */

#define BULK_OUT        0x00
#define BULK_IN         0x01
#define ENDPOINTS       0x02

/* endpoint state*/

#define BULK_IN_HALTED    0x01
#define BULK_OUT_HALTED   0x02


extern struct usbd_function_operations function_ops;
extern struct usbd_function_driver function_driver;


#endif
