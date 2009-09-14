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
 * otg/hardware/mxc-hcd.c - Freescale USBOTG aware Host Controller Driver (HCD)
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/platform/mxc/mxc-hcd.c|20070918011422|08412
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *      Tony Tang    <tt@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc-hcd.c
 * @brief Freescale USB Host Controller Driver
 *
 * This is a complete and self contained Linux 2.6 USB Host Driver.
 *
 * It also conforms to the requirements for use as an OTG HCD driver
 * in the Belcarra OTG Stack.
 *
 * The hardware is an integrated design, so it also works in conjunction
 * with the mxc OCD and PCD drivers to share the hardware under the direction
 * of the OTG State Machine.
 *
 * The root hub is managed locally, specifically we clear all relevant status
 * changes etc. The MXC USB interrupt is level triggered so this is required
 * to stop interrupts.
 *
 * The actual OTG port status changes are propagated to the upper layers
 * through a virtual root hub which will allow the root hub support in the
 * upper layers to operate properly.
 *
 * @ingroup FSOTG
 * @ingroup HCD
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>

#include <core/hcd.h>

#include <otg/usbp-hub.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>
#include <otg/otg-tcd.h>

#include <otg/otg-hcd.h>
#include <asm/arch/gpio.h>
#include "mxc-lnx.h"
#include "mxc-hardware.h"
#include <asm/memory.h>

#include "mxc-hcd.h"

/* ********************************************************************************************* */
/* ********************************************************************************************* */
void mxc_hcd_giveback_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, int status);

/* ********************************************************************************************* */
/* ********************************************************************************************* */

// For debugging...
static char *dirpid_name[4] = { "SETUP", "OUT", "IN", "WRONG-PID"};
//
char *etd_urb_state_name[] = {
        "COMPLETED",            "SETUP_STATUS",         "SETUP_DATA",           "SETUP_PKT",
        "BULK_START",           "BULK_WZLP",            "BULK_WZLP_START",      "INTERRUPT_START",
        "ISOC",                 "WRONG-STATE",
};

static char *format_name[4] = { "CONTROL", "ISO", "BULK", "INT", };

/*!
 * @struct cc_name
 * @brief Map CC values to names
 */
static char *cc_name[] = {
        "no error", "CRC error", "bitstuff error", "data toggle error",
        "stall", "device not responding", "PID failure" "reserved",
        "data overrun", "data underrun", "ACK", "NAK", "buffer overrun",
        "buffer underrun", "schedule overrun" "not accessed"
};


/*!
 * comp_code_to_status() - map condition cod3 to errno value
 * @param cc condition code
 * @return errno value
 */
static int comp_code_to_status(int cc)
{
        if (cc) TRACE_MSG2(HCD,"NON-ZERO CC: %d %s", cc,cc_name[cc]);
        switch (cc) {
        case ETD_CC_NOERROR:           return 0;
        case ETD_CC_CRCERR:            return -EILSEQ;
        case ETD_CC_BITSTUFFERR:       return -EPROTO;
        case ETD_CC_DATATOGERR:        return -EPROTO;  // I guess, nothing else looks better.
        case ETD_CC_STALL:             return -EPIPE;
        case ETD_CC_DEVNOTRESPONDING:  return -ETIMEDOUT;
        case ETD_CC_PIDFAILURE:        return -EPROTO;
        case ETD_CC_DATAOVERRUN:       return -EOVERFLOW;
        case ETD_CC_DATAUNDERRUN:      return -EREMOTEIO;
        case ETD_CC_ACK:               return -EPROTO;  // For lack of anything better
        case ETD_CC_NAK:               return -EPROTO;  // For lack of anything better
        case ETD_CC_BUFFEROVERRUN:     return -ECOMM;
        case ETD_CC_BUFFERUNDERRUN:    return -ENOSR;
        case ETD_CC_SCHEDOVERRUN:      return -ENOSPC;
        case ETD_CC_NOTACCESSED:       return -EPROTO;  // For lack of anything better
        default:                       return -EINVAL;
        }
}
/* ********************************************************************************************* */
/*! ETD Management
 */

/*!
 * rel_etd_irq() - release etd
 * @param mxc_hcd
 * @param etdn
 * {get,rel}_etd maintain a free list of ETDs by saving the index of the next free ETD in the
 * etd->xbufsrtad field.  The list is terminated by 0x0000ffff.  List manipulation needs to be
 * protected by irq_lock, since ETDs are released in the interrupt handler, but allocated at
 * regular priority.
 */
void rel_etd_irq(struct mxc_hcd *mxc_hcd, int etdn)
{
        TRACE_MSG1(HCD,"*****************etdn:%d",etdn);
        fs_wl(etd_word(etdn, 0), 0);
        fs_wl(etd_word(etdn, 1), 0);
        fs_wl(etd_word(etdn, 2), 0);
        fs_wl(etd_word(etdn, 3), 0);
        mxc_hcd->active[etdn] = NULL;
        //mxc_hcd->active_count--;
}

/*!
 * get_etd_irq() - get etd
 * @param mxc_hcd
 * @param mxc_req
 * @return int
 * {get,rel}_etd maintain a free list of ETDs by saving the index of the next free ETD in the
 * etd->xbufsrtad field.  The list is terminated by 0x0000ffff.  List manipulation needs to be
 * protected by irq_lock, since ETDs are released in the interrupt handler, but allocated at
 * regular priority.
 */
static int get_etd_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req)
{
        int etdn;
        for (etdn = 0; etdn < NUM_ETDS; etdn++)
                UNLESS (mxc_hcd->active[etdn]) {
                        mxc_hcd->active[etdn] = mxc_req;
                        //mxc_hcd->active_count++;
                        return etdn;
                }
        TRACE_MSG0(HCD,"error");
        return -ENOMEM;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Data Buffer Management
 */

/*!
 * @name DataBuffs
 *
 * There is no predetermined size for data buffers, so
 * this structure was chosen as an arbitrary, but generally
 * adequate size.
 */
/*! @{ */

/*!
 * get_data_buff() - get data buff
 * Get the next available free data_buff, return NULL if none available.
 */
static __inline__ fs_data_buff *get_data_buff(struct mxc_hcd *mxc_hcd)
{
        unsigned long flags;
        fs_data_buff *db = NULL;
        u16 ndx;
        local_irq_save(flags);
        if (0xffff != (ndx = mxc_hcd->free_buffs)) {
                mxc_hcd->free_buffs = mxc_hcd->buff_list[ndx];
                db = ndx + (fs_data_buff *)OTG_DATA_BASE;
        }
        local_irq_restore(flags);
        return db;
}

/*!
 * rel_data_buff() - release data buff
 * Release db to the pool of available data_buffs.
 */
fs_data_buff *rel_data_buff(struct mxc_hcd *mxc_hcd, fs_data_buff *db)
{
        unsigned long flags;
        u16 ndx;
        RETURN_NULL_UNLESS (db);
        local_irq_save(flags);
        ndx = db - (fs_data_buff *)OTG_DATA_BASE;
        mxc_hcd->buff_list[ndx] = mxc_hcd->free_buffs;
        mxc_hcd->free_buffs = ndx;
        local_irq_restore(flags);
        return NULL;
}

/*!
 * data_buff_boff() - get data buffer offset given address
 */
static __inline__ u16 data_buff_boff(fs_data_buff *db)
{
        return((u16)(((void *) db) - ((void *) OTG_DATA_BASE)));
}

/*!
 * data_buff_addr() - get data buffer address given offset
 * Return the address of the fs_data_buffer that is boff bytes from the start of data buffer memory.
 */
static __inline__ fs_data_buff *data_buff_addr(u16 boff)
{
        return(boff + ((void *) OTG_DATA_BASE));
}
/*! @} */

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! Data Tranfer Handling
 */

u8 dmabuf[128];

/*!
 * mxc_hcd_start_etd_irq() - start an etd
 * @param etdn
 * @param len
 * @param dma
 * @param out
 *
 * This will start a request by enabling the specified etd.
 *
 * XXX I don't know why this doesn't work correctly unless DMA is enabled.
 * But enabling DMA with NULL data address is not a greate idea... so
 * we check and give a dummy buffer for it to use.
 *
 */
static void mxc_hcd_start_etd_irq(int etdn, int len, u32 dma, int out)
{
        TRACE_MSG4(HCD, "etdn: %d len: %d dma: %x out: %d", etdn, len, dma, out);

        fs_wl(OTG_DMA_ETD_MSA(etdn), dma ? dma : (int)virt_to_phys(dmabuf));

        /* check for DMA alignment - downgrade if neccessary */
        if (dma & 0xf) {
                fs_andl(OTG_DMA_ETD_BURST4, ~ETD_MASK(etdn));
                TRACE_MSG2(HCD, "RESET ETD BURST4[%2d]: %08x", ETD_MASK(etdn), fs_rl(OTG_DMA_ETD_BURST4));
        }
        else {
                fs_orl(OTG_DMA_ETD_BURST4, ETD_MASK(etdn));
                TRACE_MSG2(HCD, "SET ETD BURST4[%2d]: %08x", ETD_MASK(etdn), fs_rl(OTG_DMA_ETD_BURST4));
        }


        if (len || out) fs_wl(OTG_DMA_ETD_EN, ETD_MASK(etdn));

        fs_orl(OTG_HOST_IINT, ETD_MASK(etdn));                       // Don't wait until SOF, interrupt ASAP.
        fs_orl(OTG_HOST_ETD_DONE, ETD_MASK(etdn));                   // Interrupt on ETD done (forget DMA interrupt).
        fs_wl(OTG_HOST_ETD_EN, ETD_MASK(etdn));
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */


/*! mxc_hcd_start_req_irq - start a request
 * @param mxc_hcd
 * @param mxc_req
 *
 * @return non-zero if no resources available
 *
 * This function will attempt to find the resources to perform
 * a request and will start it if the required resources are
 * available.
 *
 * A non-zero return value signals a failure to find resources.
 *
 * Requests that are not possible to start for other reasons are
 * not signalled back to the caller, the urb is simply given
 * back to the hcd driver.
 *
 * XXX it is possible that too many receive urbs could tie up all
 * existing etds. It would be possible to dequeue a receive urb
 * temporarily and then restart.
 */
static int mxc_hcd_start_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req)
{
        struct urb              *urb = mxc_req->urb;
        struct usb_device       *udev = urb->dev;
        /*struct hcd_dev          *hdev = (struct hcd_dev *) udev->hcpriv; */

	int			address = usb_pipedevice(urb->pipe) ?
					  (usb_pipedevice(urb->pipe) % MXC_MAX_USB_ADDRESS + 1) : 0;
        int                     is_out = usb_pipein(urb->pipe) ? 0 : 1;
        int                     endpoint = usb_pipeendpoint(urb->pipe);
        u32                     dma = mxc_req->transfer_dma;
        int                     len = urb->transfer_buffer_length;

        u32                     other;

        int                     pid;
        int                     toggle;

        transfer_descriptor     td;
        int                     fmt;
        int                     dir;

        endpoint_transfer_descriptor *sdp = NULL;

        TRACE_MSG2(HCD, "mxc_req: %x urb: %x", mxc_req, urb);
#if 0
        if (is_out) {
                int i;
                u8 *cp = urb->transfer_buffer;
                TRACE_MSG1(HCD, "NEXT TX: length: %d", urb->actual_length);
                for (i = 0; i < urb->actual_length;  i+= 8)
                        TRACE_MSG8(HCD, "SENT  %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]);

        }
        if (usb_pipetype(urb->pipe) == PIPE_CONTROL) {
                u8 *cp = (u8 *) urb->setup_packet;
                TRACE_MSG8(HCD, "SETUP %02x %02x %02x %02x %02x %02x %02x %02x",
                                cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
        }
#endif

        /* verify that urb status is ok */

        if (urb->status != -EINPROGRESS) {
                TRACE_MSG1(HCD, "bad status for urb: %08x status: %d, urb rejected.", urb->status);
                list_del(&mxc_req->queue);
                mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, -EPIPE);
                return 0;
        }

        /* verify that this device endpoint is inactive */

        if (mxc_hcd->ep[address][EPNUM(endpoint, is_out)]) {
                TRACE_MSG3(HCD, "endpoint active for urb: %08x status: %d endpoint: %d out: %d.", urb->status, endpoint, is_out);
                return 0;
        }

        /* verify we have hardware resources */

        RETURN_ENOMEM_IF(mxc_hcd->active_count > NUM_ETDS);

        /* 1. Allocate resources:
         *    a. X,Y buffers
         *    b. ETD (& matching DMA)
         * 2. Set the registers
         */
        mxc_req->etdn = get_etd_irq(mxc_hcd, mxc_req);
        mxc_req->x = get_data_buff(mxc_hcd);
        mxc_req->y = get_data_buff(mxc_hcd);
        mxc_req->epnum = EPNUM(endpoint, is_out);

        UNLESS (mxc_req->x && mxc_req->y && (mxc_req->etdn != -1)) {
                printk(KERN_INFO"%s: NO RESOURCES\n", __FUNCTION__);
                TRACE_MSG0(HCD, "NO RESOURCES");
                if (mxc_req->x) mxc_req->x = rel_data_buff(mxc_hcd, mxc_req->x);
                if (mxc_req->y) mxc_req->y = rel_data_buff(mxc_hcd, mxc_req->y);
                mxc_req->x = mxc_req->y = NULL;
                mxc_req->etdn = -1;
                return -ENOMEM;  // no sense in trying anymore...
        }

        mxc_hcd->active_count++;

        /* set hcd_dev->ep[] so we can find specific request from hcd_dev, indexed by EPNUM(endpoint,is_out)
         * set mxc_hcd->active[] so we can find requests from hardware POV, indexed by etdn
         */
        mxc_hcd->ep[address][mxc_req->epnum] = mxc_req;
        memset((void *)&td, 0, sizeof(td));
        td.w1.bufsrtad.y = data_buff_boff(mxc_req->y);
        td.w1.bufsrtad.x = data_buff_boff(mxc_req->x);

        /* we have request, we have urb, we have resources */

        TRACE_MSG5(HCD,"urb: %08lx ep: %d %s len: %d etdn: %d",
                        (u32)(void*)urb, endpoint, (is_out?"OUT":"IN"), urb->actual_length, mxc_req->etdn);

#if 0
        if (usb_endpoint_halted(urb->dev, endpoint, is_out)) {
                TRACE_MSG4(HCD,"urb for halted endpoint urb: %08lx dev: %d ep: %d dir: %c",
                                urb, dev_addr, endpoint, (is_out?'O':'I'));
                urb->actual_length = 0;
                mxc_hcd_giveback_urb(urb,-EPIPE);
                return -EINVAL;
        }
#endif

        /* Check pid, setup pid and out flags */
        switch ( (usb_pipetype(urb->pipe) == PIPE_CONTROL) ? USB_PID_SETUP : ((is_out ? USB_PID_OUT : USB_PID_IN))) {
        default:
        case USB_PID_SETUP:
                pid = ETD_DIRPID_SETUP;
                is_out = 1;
                break;
        case USB_PID_OUT:
                pid = ETD_DIRPID_OUT;
                break;
        case USB_PID_IN:
                pid = ETD_DIRPID_IN;
                break;
        }

        TRACE_MSG7(HCD, "len: %d endpoint: %02x pid: %d format: %d dev_addr: %d %s %s",
                        len, endpoint, pid, usb_pipetype(urb->pipe), usb_pipedevice(urb->pipe),
                        dirpid_name[pid], format_name[usb_pipetype(urb->pipe)]);

        switch (usb_pipetype(urb->pipe)) {
        default:
        case PIPE_CONTROL:
                TRACE_MSG0(HCD, "PIPE_CONTROL");
                // XXX send set feature otg enable IFF set configuration and
                // to first device and previously otg descriptor was seen.
                // overload toggle with data phase direction (setup is always toggle 0)
                // toggle == TRUE --> OUT  (HOST2DEVICE --> OUT)

                toggle = ((((struct usb_ctrlrequest *) urb->setup_packet)->bRequestType & USB_ENDPOINT_DIR_MASK) ==
                                USB_DIR_OUT);

                //other = (u32) (void *) urb->setup_packet;

                td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len));

                fmt = ETD_FORMAT_CONTROL;
                dir = ETD_DIRECT_TD00;

                /* build setup data/status phase ETD for use after completion of 8 byte setup packet
                 */
                dma = mxc_req->setup_dma;
                sdp = &mxc_req->sdp_etd;
                memset((void*)sdp, 0, sizeof(endpoint_transfer_descriptor));

                /* toggle is constant 0 for setup packet, and 1 for data and status phases,
                 * so toggle was overloaded to handle data phase direction.  If len == 0, there
                 * is no data phase, so direction is IN.
                 */
                sdp->td.w1.val = td.w1.val;
                sdp->td.w2.cb.flags = ETD_SET_DATATOGL((0x2|1)) |  // toggle is always 1, for data or status phases.
                        ETD_SET_DELAYINT(0) |           // when done, interrupt ASAP (wait for 0 frames)
                        ETD_SET_BUFROUND(1) |           // Allow short recv xfers
                        ETD_SET_DIRPID(((toggle && len > 0)? ETD_DIRPID_OUT:ETD_DIRPID_IN));
                sdp->td.w2.cb.reserved = 0;
                sdp->td.w2.cb.rtrydelay = 1;            // Number of frames to wait before retry on failure.
                sdp->td.w3.val = ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len);

                /* build SETUP ETD for initial 8 byte setup packet
                 */
                len = 8;
                td.w2.cb.flags = ETD_SET_DATATOGL((0x2|0)) |  // starting toggle is here, not TOGCRY, and is always 0.
                        ETD_SET_DELAYINT(0) |           // when done, interrupt ASAP (wait for 0 frames)
                        ETD_SET_BUFROUND(1) |           // Allow short recv xfers
                        ETD_SET_DIRPID(pid);
                td.w2.cb.reserved = 0;
                td.w2.cb.rtrydelay = 1;                 // Number of frames to wait before retry on failure.
                td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len));

                mxc_req->etd_urb_state = ETD_URB_SETUP_START;

                //TRACE_MSG3(HCD,"SETUP pkt, len: %d : %08x : %08x",len,*((u32*)(void*)data),*(1+(u32*)(void*)data));

                /* Complete the setup data phase ETD.
                 */
                sdp->epd = ETD_SET_MAXPKTSIZ ( usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe))) |
                        ETD_SET_TOGCRY(0) |
                        ETD_SET_FORMAT(ETD_FORMAT_BULK) |
                        ETD_SET_SPEED(((((urb->pipe) >> 26) & 1)? ETD_SPEED_LOW : ETD_SPEED_FULL)) |
                        ETD_SET_DIRECT(ETD_DIRECT_TD00) |
                        ETD_SET_ENDPNT(endpoint) |
                        ETD_SET_ADDRESS(usb_pipedevice(urb->pipe));

                break;

        case PIPE_BULK:
                TRACE_MSG0(HCD, "PIPE_BULK");
                toggle = usb_gettoggle(urb->dev, endpoint, is_out);
                other = (urb->transfer_flags & URB_ZERO_PACKET);
                td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len));
                fmt = ETD_FORMAT_BULK;
                dir = ETD_DIRECT_TD00;

                TRACE_MSG1(HCD,"BULK, starting toggle %x",toggle);
                td.w2.cb.flags = ETD_SET_DATATOGL((0x2|toggle)) |  // starting toggle is here, not TOGCRY
                        ETD_SET_DELAYINT(0) |           // when done, interrupt ASAP (wait for 0 frames)
                        ETD_SET_BUFROUND(1) |           // Allow short recv xfers
                        ETD_SET_DIRPID(pid);
                td.w2.cb.reserved = 0;
                td.w2.cb.rtrydelay = 0;                 // Number of frames to wait before retry on failure.

                /* Set the state to indicate if a trailing ZLP is required.
                 */
                mxc_req->etd_urb_state = (other && is_out && !(len % usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe)))) ?
                                ETD_URB_BULKWZLP_START : ETD_URB_BULK_START;
                break;

        case PIPE_INTERRUPT:
                TRACE_MSG0(HCD, "PIPE_INTERRUPT");
                toggle = usb_gettoggle(urb->dev, endpoint, is_out);
                other = urb->interval;
                td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len));
                fmt = ETD_FORMAT_INTERRUPT;
                dir = ETD_DIRECT_TD00;

                TRACE_MSG1(HCD,"INTERRUPT, starting toggle %x",toggle);
                td.w2.intr.flags = ETD_SET_DATATOGL((0x2|toggle)) |  // starting toggle is here, not TOGCRY
                        ETD_SET_DELAYINT(0) |           // when done, interrupt ASAP (wait for 0 frames)
                        ETD_SET_BUFROUND(1) |           // Allow short recv xfers
                        ETD_SET_DIRPID(pid);

                td.w2.intr.relpolpos = (fs_rl(OTG_HOST_FRM_NUM) + 1) & 0xff;  // Start next frame

                td.w2.intr.polinterv = other & 0xff;
                mxc_req->etd_urb_state = ETD_URB_INTERRUPT_START;
                break;

        case PIPE_ISOCHRONOUS:
                TRACE_MSG0(HCD, "PIPE_ISOCHRONOUS");
                toggle = 0;
                other = urb->interval;
                td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(len));

                fmt = ETD_FORMAT_ISOC;
                dir = is_out ? ETD_DIRECT_OUT : ETD_DIRECT_IN;
                td.w2.isoc.startfrm = (fs_rl(OTG_HOST_FRM_NUM) + 1) & 0xffff;  // next frame
                if (len > 1023) {
                        // Two frames needed, send 1023 in the first, remainder in the second.
                        td.w2.isoc.flags = ETD_SET_DELAYINT(0) | ETD_SET_AUTOISO(0) | ETD_SET_FRAMECNT(1);
                        td.w3.isoc.pkt0 = ETD_SET_PKTLEN(1023);
                        td.w3.isoc.pkt1 = ETD_SET_PKTLEN(len-1023);
                }
                else {
                        // Only one frame needed, send it all at once.
                        td.w2.isoc.flags = ETD_SET_DELAYINT(0) | ETD_SET_AUTOISO(0) | ETD_SET_FRAMECNT(0);
                        td.w3.isoc.pkt0 = ETD_SET_PKTLEN(len);
                        td.w3.isoc.pkt1 = 0;
                }
                mxc_req->etd_urb_state = ETD_URB_ISOC_START;
                break;
        }

        /* copy etd into hardware registers */

        fs_wl(etd_word(mxc_req->etdn, 0),
                        ETD_SET_MAXPKTSIZ ( usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe))) |
                        ETD_SET_TOGCRY(0) |
                        ETD_SET_FORMAT(fmt) |
                        ETD_SET_SPEED(((((urb->pipe) >> 26) & 1)? ETD_SPEED_LOW : ETD_SPEED_FULL)) |
                        ETD_SET_DIRECT(dir) |
                        ETD_SET_ENDPNT(endpoint) |
                        ETD_SET_ADDRESS(usb_pipedevice(urb->pipe)));

        fs_wl(etd_word(mxc_req->etdn, 1), td.w1.val);
        fs_wl(etd_word(mxc_req->etdn, 2), td.w2.val);
        fs_wl(etd_word(mxc_req->etdn, 3), td.w3.val);

        TRACE_MSG5(HCD, "ETD[%02d] %08x %08x %08x %08x", mxc_req->etdn,
                        fs_rl(etd_word(mxc_req->etdn, 0)), fs_rl(etd_word(mxc_req->etdn, 1)),
                        fs_rl(etd_word(mxc_req->etdn, 2)), fs_rl(etd_word(mxc_req->etdn, 3)));

        /* housekeeping before actual start */

        urb->actual_length = 0;
        list_del(&mxc_req->queue);

        mxc_hcd_start_etd_irq(mxc_req->etdn, len, dma, is_out);

        return 0;       // allow scheduler to continue
}

/*! mxc_hcd_schedule_irq -
 * @param mxc_hcd
 *
 * This function scans inactive list for work and will start
 * anything it can find assuming no conflicts and that there
 * are resources available.
 *
 * This is called from the enqueue function and the hcd interrupt handler.
 *
 */
void mxc_hcd_schedule_irq(struct mxc_hcd *mxc_hcd)
{
        struct mxc_req          *mxc_req;
        struct mxc_req          *mxc_req_save;

        /* iterate across list of active urbs, normally this will be a very
         * short list.... stop if non-zero which means there are no more
         * resources.
         *
         * N.B. use _safe version of macro because we are deleting entries.
         */
        list_for_each_entry_safe(mxc_req, mxc_req_save, &mxc_hcd->inactive, queue) {
                BREAK_IF(mxc_hcd_start_req_irq(mxc_hcd, mxc_req));
        }
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * mxc_hcd_finish_req_irq() - called to complete transfer
 * @param mxc_hcd
 * @param mxc_req
 * @param urb
 * @param killed
 *
 * This function will finish a request and give the urb back to the hcd driver.
 *
 * Control Transfers may be restarted to do the status phase done and/or
 * send a ZLP.
 *
 * Bulk transfers may be restarted to send a ZLP.
 *
 */
void mxc_hcd_finish_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, struct urb *urb, int killed)
{
        int                     cc = 0;
        int                     next_toggle = 0;
        int                     format = 0;
        u32                     remaining = 0;
        fs_data_buff            *x;
        fs_data_buff            *y;
        endpoint_transfer_descriptor res;

        struct usb_device       *dev = urb->dev;
        struct usb_bus          *bus = dev->bus;
        struct usb_device       *udev = urb->dev;
//        struct hcd_dev          *hdev = (struct hcd_dev *) udev->hcpriv;
        int                     is_out = usb_pipeout(urb->pipe);
	int			address = usb_pipedevice(urb->pipe) ?
					(usb_pipedevice(urb->pipe) % MXC_MAX_USB_ADDRESS + 1) : 0;
        int                     endpoint = usb_pipeendpoint(urb->pipe);
        TRACE_MSG6(HCD, "urb: %x mxc_req: %x etdn: %d state: %d %s %s",
                        urb, mxc_req, mxc_req->etdn, mxc_req->etd_urb_state,
                        etd_urb_state_name[mxc_req->etd_urb_state], (killed?"KILLED":""));

        /* Disable/Abort DMA.  R1: sec 23.13.16 pg 23-103
         * (supposed to auto-clear on DMA completion, but this shouldn't hurt).
         */
        fs_wl(OTG_DMA_ETD_CH_CLR, ETD_MASK(mxc_req->etdn));

        if (fs_rl(OTG_HOST_ETD_EN) & ETD_MASK(mxc_req->etdn))
                fs_andl(OTG_HOST_ETD_EN, ~ETD_MASK(mxc_req->etdn));             // This register IS write 0 to disable.  Really.

        if ( fs_rl(OTG_HOST_ETD_DONE) & ETD_MASK(mxc_req->etdn))
                fs_andl(OTG_HOST_ETD_DONE, ~ETD_MASK(mxc_req->etdn));           // This register IS write 0 to disable.  Really.

                                                        // Hardware should have disabled ETD, but it doesn't hurt to check.
        if (fs_rl(OTG_HOST_ETD_EN) & ETD_MASK(mxc_req->etdn))
                fs_wl(OTG_HOST_ETD_EN, ETD_MASK(mxc_req->etdn));                // This register IS write 1 to clear.  Really.

        if (fs_rl(OTG_HOST_XINT_STAT) & ETD_MASK(mxc_req->etdn))
                fs_wl(OTG_HOST_XINT_STAT, ETD_MASK(mxc_req->etdn));             // This register IS write 1 to clear.  Really.

        if (fs_rl(OTG_HOST_YINT_STAT) & ETD_MASK(mxc_req->etdn))
                fs_wl(OTG_HOST_YINT_STAT, ETD_MASK(mxc_req->etdn));             // This register should be write 1 to clear.


        if (fs_rl(OTG_HOST_XYINT_STEN) & ETD_MASK(mxc_req->etdn))
                fs_andl(OTG_HOST_XYINT_STEN, ~ETD_MASK(mxc_req->etdn));         // This register should be write 0 to disable.

        if (fs_rl(OTG_HOST_XFILL_STAT) & ETD_MASK(mxc_req->etdn))
                fs_wl(OTG_HOST_XFILL_STAT, ETD_MASK(mxc_req->etdn));            // This register IS write 1 to clear.  Really.

        if (fs_rl(OTG_HOST_YFILL_STAT) & ETD_MASK(mxc_req->etdn))
                fs_wl(OTG_HOST_YFILL_STAT, ETD_MASK(mxc_req->etdn));            // This register should be write 1 to clear.

        // Extract results
        res.epd = fs_rl(etd_word(mxc_req->etdn, 0));
        res.td.w1.val = fs_rl(etd_word(mxc_req->etdn, 1));
        res.td.w2.val = fs_rl(etd_word(mxc_req->etdn, 2));
        res.td.w3.val = fs_rl(etd_word(mxc_req->etdn, 3));

        fs_wl(OTG_HOST_EP_DSTAT, ETD_MASK(mxc_req->etdn));

        //TRACE_MSG1(HCD,"OTG_HOST_ETD_DONE: %08x", fs_rl(OTG_HOST_ETD_DONE));

        //if ((ETD_URB_BULK_START == mxc_req->etd_urb_state || (ETD_URB_BULKWZLP_START == mxc_req->etd_urb_state)) &&
        //                (ETD_GET_DIRPID(res.td.w2.cb.flags) != ETD_DIRPID_IN)
        //    )
                TRACE_MSG4(HCD,"RES: epd: %08x w1: %08x w2: %08x w3: %08x", res.epd,res.td.w1.val,res.td.w2.val,res.td.w3.val);

        if (ETD_GET_HALTED(res.epd))
                TRACE_MSG0(HCD,"endpoint HALTED");

        format = ETD_GET_FORMAT(res.epd);

        switch (mxc_req->etd_urb_state) {
        case ETD_URB_SETUP_START:
                /* Just finished the setup packet.
                 * Go to data or status phase of setup packet while we still have the ETD and data buffers allocated.
                 */
                if (!killed && ETD_CC_NOERROR == (cc = ETD_GET_COMPCODE(res.td.w2.cb.flags))) {

                        endpoint_transfer_descriptor *sdp = &mxc_req->sdp_etd;
                        int len = ETD_GET_TOTBYECNT(sdp->td.w3.val);
                        void *data = mxc_req->urb->transfer_buffer;
                        u32 dma = mxc_req->transfer_dma;
                        mxc_req->etd_urb_state = ((len > 0) ? ETD_URB_SETUP_DATA : ETD_URB_SETUP_STATUS);

                        // Copy into the active ETD and start it.
                        fs_wl(etd_word(mxc_req->etdn, 0), sdp->epd);
                        fs_wl(etd_word(mxc_req->etdn, 1), sdp->td.w1.val);
                        fs_wl(etd_word(mxc_req->etdn, 2), sdp->td.w2.val);
                        fs_wl(etd_word(mxc_req->etdn, 3), sdp->td.w3.val);
                                                                                        // watch out for ETD_DIRPID_SETUP

                        mxc_hcd_start_etd_irq(mxc_req->etdn, len, dma,
                                        (ETD_GET_DIRPID(sdp->td.w2.cb.flags) != ETD_DIRPID_IN));

                        TRACE_MSG1(HCD,"SETUP %s Phase started",((len > 0) ? "DATA" : "STATUS"));
                        return;
                }

                /* Something is wrong, finish the URB as is. */
                remaining = ETD_GET_TOTBYECNT(fs_rl(etd_word(mxc_req->etdn, 3)));
                format = PIPE_CONTROL;
                break;

        case ETD_URB_SETUP_DATA:
                /* Go to status phase of setup packet while we still have the ETD and data buffers allocated.
                 * Save the data phase length for after status.
                 */
                if (!killed && ETD_CC_NOERROR == (cc = ETD_GET_COMPCODE(res.td.w2.cb.flags))) {
                        endpoint_transfer_descriptor *sdp = &mxc_req->sdp_etd;
                        mxc_req->remaining = ETD_GET_TOTBYECNT(res.td.w3.val);
                        mxc_req->etd_urb_state = ETD_URB_SETUP_STATUS;

                        /* Rebuild data phase sdp for status phase with opposite direction, and 0 length.
                         */

                        sdp->td.w2.cb.flags = ETD_FLIP_DIRPID(sdp->td.w2.cb.flags);
                        sdp->td.w3.val = (ETD_SET_BUFSIZE(sizeof(fs_data_buff)-1) | ETD_SET_TOTBYECNT(0));

                        /* Copy into the active ETD and start it.
                         */
                        fs_wl(etd_word(mxc_req->etdn, 0), sdp->epd);
                        fs_wl(etd_word(mxc_req->etdn, 1), sdp->td.w1.val);
                        fs_wl(etd_word(mxc_req->etdn, 2), sdp->td.w2.val);
                        fs_wl(etd_word(mxc_req->etdn, 3), sdp->td.w3.val);
                        mxc_hcd_start_etd_irq(mxc_req->etdn, 0, 0,
                                        (ETD_GET_DIRPID(sdp->td.w2.cb.flags) != ETD_DIRPID_IN));
                        TRACE_MSG0(HCD,"SETUP STATUS Phase started");
                        return;
                }

                /* Something is wrong, finish the URB as is. */
                remaining = ETD_GET_TOTBYECNT(res.td.w3.val);
                format = PIPE_CONTROL;
                break;

        case ETD_URB_SETUP_STATUS:
                cc = ETD_GET_COMPCODE(res.td.w2.cb.flags);
                remaining = mxc_req->remaining;
                mxc_req->remaining = 0;
                format = PIPE_CONTROL;
                break;

        case ETD_URB_BULKWZLP_START:
                /* Need a trailing ZLP.
                 */
                next_toggle = ETD_GET_TOGCRY(res.epd);
                if (!killed && ETD_CC_NOERROR == ETD_GET_COMPCODE(res.td.w2.cb.flags)) {
                        // Since TOTBYECNT should be 0, re-enabling the same ETD ought to give a ZLP.
                        mxc_req->etd_urb_state = ETD_URB_BULKWZLP;
                                                                                // Save the data phase length for after ZLP.
                        // mxc_hcd->sdp_data[mxc_req->etdn] = (void *) ETD_GET_TOTBYECNT(res.td.w3.val);
                        // FIXME - verify toggle OK

                        mxc_hcd_start_etd_irq(mxc_req->etdn, 0, 0, TRUE/*always OUT*/);

                        TRACE_MSG0(HCD,"ZLP started");
                        return;
                }
                // Something is wrong, finish the URB as is.
                remaining = ETD_GET_TOTBYECNT(res.td.w3.val);
                format = PIPE_BULK;
                break;

        case ETD_URB_BULKWZLP:
                /* Trailing ZLP now finished. */
                cc = ETD_GET_COMPCODE(res.td.w2.cb.flags);
                next_toggle = ETD_GET_TOGCRY(res.epd);
                /* remaining must be zero */
                remaining = 0;
                format = PIPE_BULK;
                break;

        case ETD_URB_BULK_START:
                cc = ETD_GET_COMPCODE(res.td.w2.cb.flags);
                next_toggle = ETD_GET_TOGCRY(res.epd);
                remaining = ETD_GET_TOTBYECNT(res.td.w3.val);
                format = PIPE_BULK;
                break;

        case ETD_URB_INTERRUPT_START:
                cc = ETD_GET_COMPCODE(res.td.w2.intr.flags);
                remaining = ETD_GET_TOTBYECNT(res.td.w3.val);
		next_toggle = usb_gettoggle(urb->dev,usb_pipeendpoint(urb->pipe),usb_pipeout(urb->pipe)) ? 0 : 1;
                format = PIPE_INTERRUPT;
                break;

        case ETD_URB_ISOC_START:
                remaining = ETD_GET_PKTLEN(res.td.w3.isoc.pkt0);
                cc = ETD_GET_COMPCODE(res.td.w3.isoc.pkt0);
                if (ETD_GET_FRAMECNT(res.td.w2.isoc.flags)) {
                        // There were two packets in this transfer
                        remaining += ETD_GET_PKTLEN(res.td.w3.isoc.pkt1);
                }
                format = PIPE_ISOCHRONOUS;
        }

        mxc_req->etd_urb_state = ETD_URB_COMPLETED;

        /* Convert completion code to HW independent value.  */

        if (killed) {
                cc = -ENOENT;
                remaining = 0;
        }
        else
                cc = comp_code_to_status(cc);

        /* Return ETD and X,Y buffers to free list.  */

        y = data_buff_addr(res.td.w1.bufsrtad.y);
        x = data_buff_addr(res.td.w1.bufsrtad.x);
        y = rel_data_buff(mxc_hcd, y);
        x = rel_data_buff(mxc_hcd, x);

        /* clear ep and active entries */

        mxc_hcd->ep[address][mxc_req->epnum] = NULL;
        rel_etd_irq(mxc_hcd, mxc_req->etdn);
        mxc_hcd->active_count--;

        /* Forward results to the layers above.  */

        UNLESS (urb) {
                printk(KERN_INFO"%s: active_urb[%d] NULL", __FUNCTION__, mxc_req->etdn);
                TRACE_MSG1(HCD,"active_urb[%d] NULL", mxc_req->etdn);
                return;
        }

        TRACE_MSG5(HCD,"id: %d status: %d tlen: %d rem: %d buffer: %p",
                         mxc_req->etdn, cc, urb->transfer_buffer_length, remaining, urb->transfer_buffer);
#if 0
        if (urb->transfer_buffer) {
                int i;
                u8 *cp = urb->transfer_buffer;
                urb->actual_length = urb->transfer_buffer_length - remaining;

                TRACE_MSG1(HCD, "NEXT TX: length: %d", urb->actual_length);

                for (i = 0; i < urb->actual_length;  i+= 8)

                        TRACE_MSG8(HCD, "RECV  %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                  );
        }
#endif

        if (-ENOENT == cc) {
                // Cancelled.
                urb->actual_length = 0;
        }
        else {
                switch (format) {
                case PIPE_CONTROL:
                        urb->actual_length = urb->transfer_buffer_length - remaining;

                        if (urb->setup_packet) {
                                u8 *cp = urb->setup_packet;
                                TRACE_MSG8(HCD, "SETUP  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                        }


                        // XXX Should this be moved?
                        //
                        // XXX We need to figure out a new way to ensure first device. Perhaps active_count == 1.
                        //
                        //if ((dev == mxc_hcd->first_dev) && !cc && urb->setup_packet)
                        {

                                struct usb_ctrlrequest *request = (struct usb_ctrlrequest*) urb->setup_packet;
                                switch (request->bRequest) {
                                case USB_REQ_SET_ADDRESS:
                                        otg_queue_event(hcd_instance->otg, ADDRESSED, HCD, "HCD COMPLETE ADDRESSED");
                                        break;
                                case USB_REQ_SET_CONFIGURATION:
                                        otg_queue_event(hcd_instance->otg, CONFIGURED, HCD, "HCD COMPLETE CONFIGURED");
                                        break;
                                // XXX check for GET_CONFIGURATION and otg descriptor here
                                }
                        }
                        break;

                case PIPE_BULK:
                case PIPE_INTERRUPT:
                        if (0 == cc) {
                                // QQSV needed for BULK, what about interrupt?
                                TRACE_MSG4(HCD,"dev: %d ep: %d dir: %d TOGGLE: %d",urb->dev->devnum,
                                                              usb_pipeendpoint(urb->pipe),usb_pipeout(urb->pipe),next_toggle);
                                usb_settoggle(urb->dev,usb_pipeendpoint(urb->pipe),usb_pipeout(urb->pipe),next_toggle);
                        }
                        urb->actual_length = urb->transfer_buffer_length - remaining;
                        //hcd_trace_mem(HCD,__FUNCTION__,"BULK/INTERRUPT urb",urb->actual_length,urb->transfer_buffer);
                        break;

                case PIPE_ISOCHRONOUS:
                        urb->actual_length = urb->transfer_buffer_length - remaining;  // QQSV
                        break;

                default: // Paranoia.
                        urb->actual_length = 0;
                        break;
                }
        }

        /* give the urb back to the hcd driver, put the request back into the unused queue */
        mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, cc);
        //list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
        return;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! MXC Root Hub Support
 *
 * We track all changes to the MXC root hub and push the OTG port changes through
 * to the upper layers IFF we are in a_host or b_host states.
 *
 * This requires that we implement some of the root hub state machine to get ports
 * connected etc even if we are not currently pushing up and conversly emulate
 * the port connect changes and reset/enable for the root hub implementation in
 * the usb core layer.
 *
 */
char *port_feature_name[] = {
        "PORT_CONNECTION",     "PORT_ENABLE",         "PORT_SUSPEND",        "FEATURE 0x03",
        "PORT_RESET",          "FEATURE 0x05",        "FEATURE 0x06",        "FEATURE 0x07",
        "PORT_POWER",          "PORT_LOW_SPEED",      "FEATURE 0x0a",        "FEATURE 0x0b",
        "FEATURE 0x0c",        "FEATURE 0x0d",        "FEATURE 0x0e",        "FEATURE 0x0f",
        "C_PORT_CONNECTION",   "C_PORT_ENABLE",       "C_PORT_SUSPEND",      "C_PORT_OVER_CURRENT",
        "C_PORT_RESET",        "PORT_TEST",
};

/*!
 * mxc_hcd_hw_rh_port_feature() - get rh port feature
 * @param mxc_hcd
 * @param wValue is feature selector
 * @param wIndex is port number (1-N)
 * @param set_flag
 */
void mxc_hcd_hw_rh_port_feature(struct mxc_hcd *mxc_hcd, u16 wValue, u16 wIndex, int set_flag)
{
        char buf[64];
        sprintf(buf, "port %d feature %s %s", wIndex, port_feature_name[wValue], set_flag ? "SET" : "RESET");

        if (wIndex == 1)
                TRACE_MSG5(HCD, "-> port: %d %s (%d) %s port_stat: %8x", wIndex, port_feature_name[wValue],
                                wValue, set_flag ? "SET" : "RESET",
                                fs_rl(fs_host_port_stat(wIndex)));

        RETURN_UNLESS(mxc_hcd->otg_port_enabled);
        if (set_flag) {
                // SET feature
                switch (wValue) {
                case PORT_SUSPEND:
                        otg_queue_event(hcd_instance->otg, BUS_SUSPENDED, HCD, "HUB_PORT_SUSPEND (mx21 hw)");
                        fs_wl(fs_host_port_stat(wIndex), 1 << wValue);
                        break;
                case PORT_RESET:
                case PORT_POWER:
                        fs_wl(fs_host_port_stat(wIndex), 1 << wValue);
                        break;
                default:
                        // Error, but ignore.
                        TRACE_MSG2(HCD,"SET port %d invalid feature %d", wIndex, wValue);
                        break;
                }
        }
        else {
                // CLEAR feature (valid features from USB2.0 11.24.2.2 pg 423).
                switch (wValue) {
                case PORT_SUSPEND:              // Cause a Host initiated resume, or no-op if already active.
                        wValue++;               // yes, really. this clears PORT_SUSPEND
                        fs_wl(fs_host_port_stat(wIndex), 1 << wValue);
                        otg_queue_event(hcd_instance->otg, BUS_SUSPENDED_, HCD, "HUB_PORT_SUSPEND/ (mx21 hw)");
                        break;

                case PORT_POWER:                // Put port in powered-off state.
                        wValue++;               // yes, really. this clears PORT_POWER
                        fs_wl(fs_host_port_stat(wIndex), 1 << wValue);
                        break;

                case PORT_ENABLE:               // Disable port.
                        //wValue--;
                        break;
                default:
                        break;
                }
                switch (wValue) {
                case PORT_POWER:                // Put port in powered-off state.
                case PORT_SUSPEND:              // Cause a Host initiated resume, or no-op if already active.
                case PORT_ENABLE:               // Disable port.
                case PORT_INDICATOR:            // ind_selector gives which indicator to clear
                case C_PORT_SUSPEND:            // clear the PORT_SUSPEND change bit
                case C_PORT_CONNECTION:         // clear the PORT_CONNECTION change bit
                case C_PORT_RESET:              // clear the PORT_RESET change bit
                case C_PORT_ENABLE:             // clear the PORT_ENABLE change bit
                case C_PORT_OVER_CURRENT:       // clear the PORT_OVERCURRENT change bit
                        fs_wl(fs_host_port_stat(wIndex), 1 << wValue);
                        break;
                default:
                        // Error, but ignore.
                        TRACE_MSG2(HCD,"CLEAR port %d invalid feature %d", wIndex, wValue);
                        break;
                }
        }

        if (wIndex == 1)
                TRACE_MSG4(HCD, "<- port: %d %s %s port_stat: %8x", wIndex, port_feature_name[wValue], set_flag ? "SET" : "RESET",
                                fs_rl(fs_host_port_stat(wIndex)));
}

/*!
 * MXC_PORT_CHANGED() - send otg event information to state machine
 * @param name
 * @param cs
 * @param changed
 * @param status
 * @param set
 * @param reset
 */
void MXC_PORT_CHANGED(char *name, u32 cs, u32 changed, u32 status, otg_current_t set, otg_current_t reset)
{
        RETURN_UNLESS(cs & changed);
        TRACE_MSG2(HCD, "%s%s", name, (cs & (1 << status)) ? "" : "/");
        if (cs & (1 << status) && set)
                otg_queue_event(hcd_instance->otg, set, HCD, name);
        if (!(cs & (1 << status)) && reset)
                otg_queue_event(hcd_instance->otg, reset, HCD, name);
}


/*!
 * mxc_hcd_update_shadow_rh() - Update shadow _status_ info (change info already updated).
 * @param mxc_hcd
 * @param port_num - 1-N based port number
 * @param cs
 *
 * Process change status for OTG port. Track changes into virtual hub change
 * status so that virtual root hub implementation can see them when we want
 * them to see them.
 *
 * N.B. The port reset processing is done here so that the OTG state machine
 * can oversee things. We emulate it for the upper layers when we want them
 * to see the actual connection.
 *
 */
static void mxc_hcd_update_shadow_rh(struct mxc_hcd *mxc_hcd, int port_num, u32 cs)
{
        TRACE_MSG2(HCD,"port: %d cs: %08x", port_num, cs);

        /* PORT_CONNECTION (0) bit has changed (either direction).
         * PORT_STATUS_CONNECTSC (16) - PORT_CONNECTION (0) changed
         *
         * When set will initiate a PORT_RESET, which should result in PORT_ENABLE.
         *
         * When reset this needs to clear virtual hub connection if otg port.
         */
        MXC_PORT_CHANGED("PORT_CONNECTION", cs, PORT_STATUS_CONNECTSC, PORT_CONNECTION,
                        HUB_PORT_CONNECT, HUB_PORT_CONNECT_ );

        if (cs & (1 << C_PORT_CONNECTION)) {
                if (cs & (1 << PORT_CONNECTION)) {
                        if (cs & (1 << PORT_LOW_SPEED)) TRACE_MSG1(HCD,"port: %d low speed device", port_num);

                        // initiate reset, which should enable port
                        mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_RESET, port_num, TRUE);
                }
                else {

                        mxc_hcd->virt_port_status[port_num - 1] &= ~(1 << PORT_CONNECTION);
                        mxc_hcd->virt_port_status[port_num - 1] &= ~(1 << PORT_ENABLE);
                        mxc_hcd->virt_port_status[port_num - 1] |= (1 << C_PORT_CONNECTION);
                        //mxc_hcd->virt_port_status[port_num - 1] |= (1 << C_PORT_ENABLE);
                        mxc_hcd->virt_hub_port_change_status |= (1 << port_num);

                        TRACE_MSG2(HCD,"port: %d disconnection virt_port_status: %08x",
                                        port_num, mxc_hcd->virt_port_status[port_num - 1]);
                }
        }


        /* PORT_RESET (4) status change, end of reset, PORT_ENABLE may (should) be on.
         *
         * This should be because of PORT_RESET started due to PORT_CONNECT and
         * should have resulted in PORT_ENABLE.
         *
         * If PORT_ENABLE is true then we can set PORT_CONNECT in virtual hub if otg port.
         *
         */
        MXC_PORT_CHANGED("PORT_RESET", cs, PORT_STATUS_PRTRSTSC, PORT_RESET, BUS_RESET, BUS_RESET_);
        if (cs & (1 << C_PORT_RESET)) {
                TRACE_MSG1(HCD,"port: %d PORT_RESET complete (s.b. enabled)", port_num);
                if (cs & (1 << PORT_ENABLE)) {
                        TRACE_MSG1(HCD,"port: %d enabled", port_num);

                        /* If OTG Port set Port Connection in virtual root hub, this starts
                         * the sequeuence of events required to get virtual
                         * root hub to emulate a connection.
                         */
                        mxc_hcd->virt_port_status[port_num - 1] |= (1 << PORT_CONNECTION);
                        mxc_hcd->virt_port_status[port_num - 1] |= (1 << C_PORT_CONNECTION);
                        mxc_hcd->virt_hub_port_change_status |= (1 << port_num);

                        TRACE_MSG2(HCD,"port: %d reset virt_port_status: %08x",
                                        port_num, mxc_hcd->virt_port_status[port_num - 1]);
                }
                else
                        TRACE_MSG1(HCD,"port: %d NOT enabled!!!", port_num);

        }


        /* PORT_ENABLE (1) has been __cleared__ by some HW event
         * PORT_STATUS_PRTENBLSC (17) - PORT_ENABLE (1) changed
         *
         * This will be set only on port error.
         *
         * This needs to clear virtual hub connection if otg port.
         */
        MXC_PORT_CHANGED("PORT_ENABLE", cs, PORT_STATUS_PRTENBLSC, PORT_ENABLE, (u64) 0, (u64) 0);
        if (cs & (1 << C_PORT_ENABLE)) {
                TRACE_MSG1(HCD,"port: %d disabled (by HW)", port_num);
                if ((cs & (1 << PORT_ENABLE))) {

                }
                else {

                        /* Reset Port Connection in virtual root hub
                         */
                        TRACE_MSG2(HCD,"port: %d lost connection virt_port_status: %08x",
                                        port_num, mxc_hcd->virt_port_status[port_num - 1]);

                        mxc_hcd->virt_port_status[port_num - 1] &= ~(1 << PORT_CONNECTION);
                        mxc_hcd->virt_port_status[port_num - 1] &= ~(1 << PORT_ENABLE);
                        //mxc_hcd->virt_port_status[port_num - 1] |= (1 << C_PORT_CONNECTION);
                        mxc_hcd->virt_port_status[port_num - 1] |= (1 << C_PORT_ENABLE);
                        mxc_hcd->virt_hub_port_change_status |= (1 << port_num);

                        TRACE_MSG2(HCD,"port: %d enable lost virt_port_status: %08x",
                                        port_num, mxc_hcd->virt_port_status[port_num - 1]);
                }
        }

        /* PORT_OVER_CURRENT (3) bit has changed (either direction).
         * PORT_STATUS_OVRCURIC (19) - PORT_OVER_CURRENT (3) changed
         *
         * XXX This will need to clear virtual hub.
         */
        MXC_PORT_CHANGED("PORT_OVER_CURRENT", cs, PORT_STATUS_OVRCURIC, PORT_OVER_CURRENT, (u64) 0, (u64) 0);
        if (cs & (1 << C_PORT_OVER_CURRENT)) {
                if (cs & (1 << PORT_OVER_CURRENT)) {
                        TRACE_MSG1(HCD,"port: %d over current ON", port_num);
                }
                else {
                        TRACE_MSG1(HCD,"port: %d over current OFF", port_num);
                }
        }

        /* PORT_SUSPEND (2) Resume sequence has completed....
         * PORT_STATUS_PRTSTATSC (18) - PORT_SUSPEND (2) changed
         */
        MXC_PORT_CHANGED("PORT_SUSPEND", cs, PORT_STATUS_PRTSTATSC, PORT_SUSPEND,
                        BUS_SUSPENDED, BUS_SUSPENDED_);

        if (cs & (1 << C_PORT_SUSPEND)) {
                // FIXME - figure this out when we get to suspend/resume....
        }

        //TRACE_MSG5(HCD, "port: %d shadow port_change_status: ((%08x & ~%04x) | %04x) hpcs: %04x",
        //                port_num, mxc_hcd->real_port_change_status[port_num - 1],
        //                mxc_hcd->real_hub_port_change_status);
}

/* ********************************************************************************************** */

/*! mxc_hcd_rh_portstatus_bh()
 *
 * Track changes in portstatus. This is run as a bottom-half handler when
 * changes in the port status registers are detected in the interrupt
 * handler.
 *
 */
static void mxc_hcd_rh_portstatus_bh(void *data)
{
        struct mxc_hcd *mxc_hcd = (struct mxc_hcd *) data;
        u8 change_data;
        struct urb *int_urb;

        u32 real_hub_port_change_status;
        u32 cs;
        unsigned long flags;

        int i;

        TRACE_MSG3(HCD, "port_change_status: %08x %08x %08x", mxc_hcd->real_port_change_status[0],
                        mxc_hcd->real_port_change_status[1], mxc_hcd->real_port_change_status[2]);

        /* check for changes in real port status */

        RETURN_UNLESS (cs = mxc_hcd->real_port_change_status[0] | fs_rl(fs_host_port_stat(1))); // OTG Port

        /* clear real port status and update the OTG port to the upper layers */

        mxc_hcd->real_port_change_status[0] = 0;
        mxc_hcd_update_shadow_rh(mxc_hcd, 1, cs);
}

/*! mxc_hcd_rh_int_hndler() - handle port status interrupt
 * @param mxc_hcd
 *
 * This finds, saves and clears port status changes, then schedules
 * the bottom-half handler to process them.
 *
 * XXX Current MXC hardware reports apparantly spurious changes to port status
 * for the non-OTG ports. These must be properly cleared.
 *
 */
void mxc_hcd_rh_int_hndlr(struct mxc_hcd *mxc_hcd)
{
        u32 roothub_status = fs_rl(OTG_HOST_ROOTHUB_STATUS);
        int port_num;

        TRACE_MSG1(HCD, "HOST_PSCINT ROOTHUB_STATUS: %08x", roothub_status);

        if (roothub_status & ROOTHUB_STATUS_OVRCURCHG) {
                fs_wl(OTG_HOST_ROOTHUB_STATUS, ROOTHUB_STATUS_OVRCURCHG);
                roothub_status = fs_rl(OTG_HOST_ROOTHUB_STATUS);
                TRACE_MSG1(HCD, "HOST_PSCINT ROOTHUB_STATUS: %08x CLEARED", roothub_status);
        }

        //printk(KERN_INFO"%s: cs: %08x %08x %08x\n", __FUNCTION__, fs_rl(fs_host_port_stat(1)),
        //                fs_rl(fs_host_port_stat(2)), fs_rl(fs_host_port_stat(3)));

        /* Clear the interrupt by clearing the port(s) in question.
         * Reflect any changes in the shadow values.
         */
        for (port_num = 1; port_num <= mxc_hcd->bNbrPorts; port_num++) {

                u32 cs = fs_rl(fs_host_port_stat(port_num));
                u32 cm = cs & 0xFFFF0000;

                CONTINUE_UNLESS (cm);
                if (port_num == 1)
                        TRACE_MSG3(HCD, "port: %d cs: %08x cm: %08x", port_num, cs, cm);

                if (port_num == 1) {
                        mxc_hcd->real_hub_port_change_status |= (1 << port_num);
                        mxc_hcd->real_port_change_status[port_num - 1] |= cm;
                }
                if ( (fs_rl(OTG_HOST_ROOTHUB_STATUS) & 0x2) &&
                        (cs & 0x8) )    //Over current in port and hub occurs
                {
                        printk (KERN_INFO"USB port overcurrent is happened !?\n");
                }

                /* clear port status */
                if (cm) {
                        //printk(KERN_INFO"%s: clearing[%d] %08x\n", __FUNCTION__, port_num, cm);
                        fs_wl(fs_host_port_stat(port_num), cm);
                        cs = fs_rl(fs_host_port_stat(port_num));
                        cm = cs & 0xFFFF0000;
                        //printk(KERN_INFO"%s: port[%d] cs: %08x cm: %08x CLEARED\n", __FUNCTION__, port_num, cs, cm);
                        TRACE_MSG3(HCD, "port: %d cs: %08x cm: %08x CLEARED", port_num, cs, cm);
                }
        }

        TRACE_MSG3(HCD, "port_change_status: %08x %08x %08x", mxc_hcd->real_port_change_status[0],
                        mxc_hcd->real_port_change_status[1], mxc_hcd->real_port_change_status[2]);

        /* Tell the RH bottom-half to scan for changes in shadow data. (schedule bottom half handler)
         */
        PREPARE_WORK_ITEM(mxc_hcd->rh_bh, mxc_hcd_rh_portstatus_bh, mxc_hcd);
        SCHEDULE_WORK(mxc_hcd->rh_bh);
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! MXC Host Interrupt Handler
 *
 * Handle a host controller interrupt. This can be for either a transfer complete
 * or root hub / port change etc.
 */

static int num_host_interrupts = 0;
#define MAX_HOST_INTERRUPTS  0
#define MAX_PSC_INTERRUPTS  0

/*!
 * mxc_hcd_hw_int_hndlr() - interrupt handler for hcd controller
 * @param irq
 * @param dev_id
 * @param regs
 */




irqreturn_t mxc_hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs)
{
        struct mxc_hcd  *mxc_hcd = hcd_instance->privdata;      // XXX this should come from dev_id

        u32 host_sint;                                  // C.f. 23.11.11 Host Interrupt Register

        int loop_count = 0;
        struct usb_hcd *hcd = (struct usb_hcd *) mxc_hcd;
        int start = hcd->state;


        /* XXX - what is this.... */
        if (OTG_USBDMA == irq) {
                // HOST DMA error interrupt
                u32 err_stat = fs_rl(OTG_DMA_ETD_ERR);
                TRACE_MSG1(HCD,"host DMA interrupt, error status %08x",err_stat);
                if (0 != err_stat) {
                        // Disable the matching DMA channels and clear the interrupt
                        fs_wl(OTG_DMA_ETD_CH_CLR, err_stat);
                        fs_wl(OTG_DMA_ETD_ERR, err_stat);
                }
                // local_irq_restore(flags);
                return IRQ_HANDLED;
        }

        /* FIXME - should check mxc_hcd->mm->otg.OTG_Module_Interrupt_Status & (0x1 << 3) | 0x1;
         * for Host (async + regular) Interrupt - enable clock on async.
         */
#if 1
        if (unlikely(start == HC_STATE_HALT ||
            !test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags)))
                return IRQ_NONE;
#endif
        while ((host_sint = fs_rl(OTG_HOST_SINT_STAT) & mxc_hcd->int_mask)) {

                num_host_interrupts++;
                if ((loop_count++ > 100) || (MAX_HOST_INTERRUPTS && (num_host_interrupts > MAX_HOST_INTERRUPTS))) {
                        printk(KERN_INFO"%s: DISABLED\n", __FUNCTION__);
                        fs_wl(OTG_HOST_SINT_STEN, 0);
                        fs_andl(OTG_CORE_CINT_STEN, ~(/*(0x1 << 3) |*/ 0x1));
                        return IRQ_HANDLED;
                }

                TRACE_MSG1(HCD, "host_sint: %08x", host_sint);

                while (HOST_DONEINT & host_sint) {
                        u32 etds_done;

                        BREAK_IF(loop_count++ > 100);

                        TRACE_MSG1(HCD,"HOST_DONEINT: %08x", fs_rl(OTG_HOST_EP_DSTAT));
                        fs_wl(OTG_HOST_SINT_STAT, HOST_DONEINT);
                        host_sint = fs_rl(OTG_HOST_SINT_STAT) & mxc_hcd->int_mask;

                        /* while DSTAT is non-zero call finish urb on highest bit set
                         */
                        while ((etds_done = fs_rl(OTG_HOST_EP_DSTAT))) {
                                int                     etdn = fls(etds_done) - 1;
                                struct mxc_req          *mxc_req;

                                mxc_req = mxc_hcd->active[etdn];

                                BREAK_IF(loop_count++ > 100);

                                UNLESS(mxc_req) {
                                        printk(KERN_INFO"%s: ERROR NO REQUEST %d etds_done: %08x etdn: %d\n",
                                                        __FUNCTION__, num_host_interrupts, etds_done, fls(etds_done) - 1);
                                        fs_wl(OTG_HOST_EP_DSTAT, ETD_MASK(etdn));
                                        continue;
                                }
                                mxc_hcd_finish_req_irq(mxc_hcd, mxc_req, mxc_req->urb, FALSE);
                        }
                        /* assuming something was done, we may be able to start
                         * something if there is anything to start...
                         */
                        mxc_hcd_schedule_irq(mxc_hcd);
                }

                host_sint = fs_rl(OTG_HOST_SINT_STAT) & mxc_hcd->int_mask;

                /* Start Of Frame.
                 * Note: this interrupt seems to happen even if disabled when there is a frame number overflow.
                 */
                if (host_sint & HOST_SOFINT) {

                        mxc_hcd->sof_count += 1;

                        TRACE_MSG2(HCD, "HOST_SOFINT: %d sof_count: %d", fs_rl(OTG_HOST_FRM_NUM), mxc_hcd->sof_count);

                        if (0 == (mxc_hcd->sof_count & (4096 - 1))) {
                                TRACE_MSG1(HCD, "SOF %08lx", mxc_hcd->sof_count);
                        }
                }

                /* Frame Number Overflow.
                 */
                if (host_sint & HOST_FMOFINT)
                        TRACE_MSG0(HCD,"Frame Number Overflow");

                /* Port Status Change.
                 */
                if (host_sint & HOST_PSCINT)
                        mxc_hcd_rh_int_hndlr(mxc_hcd);

                /* Overrun
                 */
                if (host_sint & (HOST_SORINT | HOST_HERRINT)) {
                        if (host_sint & HOST_SORINT)
                                TRACE_MSG0(HCD,"Scheduling Overrun");
                        if (host_sint & HOST_HERRINT)
                                TRACE_MSG0(HCD,"Host Scheduling Error");
                }
                /* Resume Detected.
                 */
                if (host_sint & HOST_RESDETINT)
                        TRACE_MSG0(HCD,"Resume Detected");

                /* Clear by writing back the ones we've checked for since the last read.
                 */
                fs_wl(OTG_HOST_SINT_STAT, host_sint & mxc_hcd->int_mask);
        }
#if 1
        set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);

        if (unlikely(hcd->state == HC_STATE_HALT))
                usb_hc_died (hcd);
#endif


        return IRQ_HANDLED;
}
