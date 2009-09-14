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
 * otg/hardware/mxc-l26.c - Linux 2.6 Freescale USBOTG aware Host Controller Driver (HCD)
 * @(#) tt/root@belcarra.com/debian286.bbb|otg/platform/mxc/mxc-l26.c|20070907214828|35671
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *      Tony Tang    <tt@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc-l26.c
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
 *
 * @ingroup FSOTG
 * @ingroup LINUXOS
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

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>

#include "mxc-hcd.h"

/* ********************************************************************************************* */
/* ********************************************************************************************* */
extern struct usb_operations usb_hcd_operations;
extern void mxc_hcd_schedule_irq(struct mxc_hcd *mxc_hcd);
extern void mxc_hcd_finish_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, struct urb *urb, int killed);
extern void mxc_host_clock_on(void);
extern void mxc_host_clock_off(void);
extern fs_data_buff *rel_data_buff(struct mxc_hcd *mxc_hcd, fs_data_buff *db);
extern void rel_etd_irq(struct mxc_hcd *mxc_hcd, int etdn);
extern void mxc_hcd_hw_rh_port_feature(struct mxc_hcd *mxc_hcd, u16 wValue, u16 wIndex, int set_flag);
extern char *port_feature_name[];
int mxc_host_gpio (void);

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! Transfer Request Management
 */

/*! mxc_get_request - get unused mxc_request or allocate a new one
 * @param mxc_hcd
 * @return requested mxc_req
 */
struct mxc_req * mxc_get_request(struct mxc_hcd *mxc_hcd)
{
        struct mxc_req          *mxc_req = NULL;
        struct mxc_req          *mxc_req_tmp = NULL;
        unsigned long           flags;

        /* attempt to find first entry in list */

        local_irq_save(flags);
        list_for_each_entry(mxc_req_tmp, &mxc_hcd->unused, queue) {
                /* delete from list */
                mxc_req = mxc_req_tmp;
                list_del(&mxc_req->queue);
                break;
        }
        local_irq_restore(flags);

        /* otherwise attempt to allocate and initialize */

        UNLESS(mxc_req) {
               RETURN_NULL_UNLESS((mxc_req = CKMALLOC(sizeof(struct mxc_req))));
               INIT_LIST_HEAD(&mxc_req->queue);
               local_irq_save(flags);
               mxc_hcd->allocated_count++;
               local_irq_restore(flags);
        }
        return mxc_req;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! mxc_hcd_urq_enqueue - enqueue an mxc request
 * @param hcd
 * @param ep
 * @param urb
 * @param mem_flags
 *
 * New Style L2.6.10 USB Core urb enqueue.
 */
int mxc_hcd_urb_enqueue(struct usb_hcd *hcd, struct usb_host_endpoint *ep, struct urb *urb, gfp_t mem_flags)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct usb_device       *udev = urb->dev;
        //struct hcd_dev          *hdev = (struct hcd_dev *) udev->hcpriv;

        //int                     dev_addr = usb_pipedevice(urb->pipe);
        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipein(pipe) ? 0 : 1;
        int                     type = usb_pipetype(pipe);
        int                     endpoint = usb_pipeendpoint(pipe);
        unsigned long           flags;
	int			address = usb_pipedevice(urb->pipe) ? (usb_pipedevice(urb->pipe) % MXC_MAX_USB_ADDRESS + 1) : 0;

        struct mxc_req          *mxc_req = NULL;

        TRACE_MSG5(HCD, "hcd: %x urb: %x endpoint: %02x is_out: %d type: %0d", hcd, urb, endpoint, is_out, type);

        /* data address needs to be on a 32-bit boundary. */

        RETURN_EPIPE_IF (0x3 & (u32)urb->transfer_buffer);

        /* get request */

        RETURN_ENOMEM_UNLESS((mxc_req = mxc_get_request(mxc_hcd)));

        /* initialize request */

        mxc_req->urb = urb;
        mxc_req->etdn = -1;
        mxc_req->etd_urb_state = EPQ_NOTUSED;
        mxc_req->remaining = 0;
        memset((void *)&mxc_req->sdp_etd, 0, sizeof(mxc_req->sdp_etd));
        mxc_req->setup_dma = mxc_req->transfer_dma = 0;
        urb->hcpriv = mxc_req;
	ep->hcpriv = (void *) pipe;

        /* XXX MXC DMA can only transfer full buffers, so we need to provide
         * an intermediate buffer for it to use.
         */
        mxc_req->bounce_buffer = NULL;
        if (!is_out && (urb->transfer_buffer_length % 128)) {

                int length = ((urb->transfer_buffer_length + 128) / 128) * 128;

                TRACE_MSG2(HCD, "bounce buffer length: %d -> %d", urb->transfer_buffer_length, length);

                UNLESS((mxc_req->bounce_buffer = CKMALLOC(length + 32))) {
                        local_irq_save(flags);
                        list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
                        local_irq_restore(flags);
                        spin_unlock(&urb->lock);
                        return -ENOMEM;
                }

                mxc_req->transfer_buffer_save = urb->transfer_buffer;
                mxc_req->bounce_addr = mxc_req->bounce_buffer;
                urb->transfer_buffer = mxc_req->bounce_addr;

                //if (is_out)
                //        memcpy(mxc_req->bounce_addr, mxc_req->transfer_buffer_save, urb->transfer_buffer_length);
        }

        /* map setup_packet and transfer_buffer */

        if (urb->setup_packet)
                mxc_req->setup_dma = dma_map_single ( mxc_hcd->hcd.self.controller,
                                urb->setup_packet, sizeof (struct usb_ctrlrequest), DMA_TO_DEVICE);

        if (urb->transfer_buffer && urb->transfer_buffer_length)
                mxc_req->transfer_dma = dma_map_single ( mxc_hcd->hcd.self.controller,
                                urb->transfer_buffer, urb->transfer_buffer_length,
                                usb_pipein (urb->pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);


        TRACE_MSG5(HCD, "setup_packet: %x %x transfer_buffer: %x %x %d",
                        urb->setup_packet, mxc_req->setup_dma,
                        urb->transfer_buffer, mxc_req->transfer_dma,
                        urb->transfer_buffer_length);

        /* in case of unlink-during-submit */

        spin_lock(&urb->lock);

        if (urb->status != -EINPROGRESS) {
                printk(KERN_INFO"%s: EINPROGRESS\n", __FUNCTION__);
                TRACE_MSG2(HCD, "status != EINPROGRESS hcd: %x urb: %x", hcd, urb);
                local_irq_save(flags);
                list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
                local_irq_restore(flags);
                spin_unlock(&urb->lock);
                return -ENOMEM;
        }
        spin_unlock(&urb->lock);

        local_irq_save(flags);
        mxc_hcd->request_count++;
        list_add_tail(&mxc_req->queue, &mxc_hcd->inactive);
        local_irq_restore(flags);

        /* if ep[EPNUM(endpoint,is_out)] is NULL then nothing is currently scheduled
         * for this device to that endpoint
         */
        UNLESS (mxc_hcd->ep[address][EPNUM(endpoint, is_out)]) {
                TRACE_MSG2(HCD, "SCHEDULING  hcd: %x urb: %x", hcd, urb);
                local_irq_save(flags);
                mxc_hcd_schedule_irq(mxc_hcd);
                local_irq_restore(flags);
        }
        return 0;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! mxc_hcd_giveback_req - give an urb back to the hcd driver
 * @param mxc_hcd
 * @param mxc_req
 * @param status
 *
 * This function will set the status of an urb IFF it is currently EINPROGRESS
 * and then use usb_hcd_giveback_req() to pass control of the urb back to the
 * hcd driver.
 */
void mxc_hcd_giveback_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, int status)
{
        struct urb              *urb = mxc_req->urb;
        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipeout(pipe);

        TRACE_MSG2(HCD, "urb: %x status: %x", mxc_req->urb, status);

        /* unmap setup_packet and transfer_buffer */

        if (urb->setup_packet)
                dma_unmap_single (mxc_hcd->hcd.self.controller, mxc_req->setup_dma,
                                sizeof (struct usb_ctrlrequest), DMA_TO_DEVICE);

        if (urb->transfer_buffer && urb->transfer_buffer_length)
                dma_unmap_single (mxc_hcd->hcd.self.controller,
                                mxc_req->transfer_dma, urb->transfer_buffer_length,
                                usb_pipein (urb->pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

        if (mxc_req->bounce_buffer) {

                if (!is_out)
                        memcpy(mxc_req->transfer_buffer_save, mxc_req->bounce_addr, urb->transfer_buffer_length);

                urb->transfer_buffer = mxc_req->transfer_buffer_save;
                LKFREE(mxc_req->bounce_buffer);
                mxc_req->bounce_buffer = NULL;
        }

#if 0
        if (urb->transfer_buffer) {
                int i;
                u8 *cp = urb->transfer_buffer;

                TRACE_MSG1(HCD, "NEXT TX: length: %d", urb->actual_length);

                for (i = 0; i < urb->actual_length;  i+= 8)

                        TRACE_MSG8(HCD, "BUF:  %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                  );
        }
#endif


        /* Call the upper layer completion routine.  Decrement the ref count
         * (release our interest in this urb).
         */
        if (urb->status == -EINPROGRESS) urb->status = status;

        urb->hcpriv = NULL;
        usb_hcd_giveback_urb(&mxc_hcd->hcd, mxc_req->urb, NULL /* XXX */);
        list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
}


/*! mxc_hcd_urb_dequeue -
 * @param hcd
 * @param urb
 * @return error if not found
 *
 * Used by hcd driver to dequeue a previously enqueued urb that has not
 * completed.
 *
 * This should find and stop the request and then use the giveback
 * procedure to pass control of the urb back to the hcd layer.
 */
int mxc_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct mxc_req          *mxc_req;
        struct mxc_req          *mxc_req_save;

        unsigned long           flags;
        int                     etdn;

        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipeout(pipe);
        int                     type = usb_pipetype(pipe);
        int                     endpoint = usb_pipeendpoint(pipe);

        TRACE_MSG5(HCD, "hcd: %x urb: %x endpoint: %02x is_out: %d type: %0d", hcd, urb, endpoint, is_out, type);

        // XXX
        // mxc_req = urb->hcpriv;

        local_irq_save(flags);

        /* search through inactive list */

        list_for_each_entry_safe(mxc_req, mxc_req_save, &mxc_hcd->inactive, queue) {
                CONTINUE_IF(mxc_req->urb != urb);

                TRACE_MSG1(HCD, "urb: %x found inactive", urb);

                /* found it - delete from active list, restore irq, giveback and return */

                list_del(&mxc_req->queue);
                mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, 0);  // check if 0 is correct status to return
                local_irq_restore(flags);
                return 0;
        }

        /* search through active array */

        for (etdn = 0; etdn < NUM_ETDS; etdn++) {
                mxc_req = mxc_hcd->active[etdn];
                CONTINUE_UNLESS(mxc_req);
                CONTINUE_IF(mxc_req->urb != urb);

                /* found it - finish it (and giveback), restore irq and return */
                TRACE_MSG1(HCD, "urb: %x found active", urb);

                mxc_hcd_finish_req_irq(mxc_hcd, mxc_req, mxc_req->urb, TRUE); // XXX This should work - need to test
                local_irq_restore(flags);
                return 0;
        }


        /* didn't find it - restore irq, return error */

        TRACE_MSG1(HCD, "urb: %x not found", urb);
        local_irq_restore(flags);

        return -EINVAL;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! OTG Support Functions
 */

/*! mxc_hcd_bh_init - MXC Hardware bottom half and OTG Init
 * Called as a "bottom-half" to do usbcore registration once all the HW is usable.
 * @param arg - hacd instance
 */
static void mxc_hcd_bh_init(void *arg)
{
        struct hcd_instance *hcd = (struct hcd_instance *) arg;
        otg_event(hcd->otg, HCD_OK, HCD, "HCD_INIT SET HCD_OK");
        TRACE_MSG0(HCD,"finished");
}

static void mxc_hcd_bh_exit(void *arg)
{
        struct hcd_instance *hcd = (struct hcd_instance *) arg;
        otg_event(hcd->otg, HCD_OK, HCD, "HCD_INIT RESET (EXIT) HCD_OK");
}


/*! hcd_init_func - per host controller common initialization
 *
 * This is called to initialize / de-initialize the HCD, all except the last
 * stage of registering the root hub, because that needs to wait until rh_hcd_en_func()
 *
 * We start work items to do this.
 * @param otg - otg instance
 * @param flag - initialize /de-initialize flag
 *
 */
void hcd_init_func (struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;

        switch (flag) {
        case SET:
                // Schedule BH for mxc_hcd_bh_init...
                TRACE_MSG0(HCD, "HCD_INIT: SET");
                //PREPARE_WORK_ITEM(hcd->bh, mxc_hcd_bh_init, hcd);
                //SCHEDULE_WORK(hcd->bh);
                mxc_hcd_bh_init((void *)hcd);
                TRACE_MSG0(HCD, "mxc_hcd_bh_init() schedule finished");
                break;

        case RESET:
                TRACE_MSG0(HCD, "HCD_INIT: RESET");
                //PREPARE_WORK_ITEM(hcd->bh, mxc_hcd_bh_exit, hcd);
                //SCHEDULE_WORK(hcd->bh);
                mxc_hcd_bh_exit((void *)hcd);
                break;
        }
}

/*!
 * mxc_hcd_en_func() - otg hcd enable output function
 * @param otg
 * @param flag
 */
void mxc_hcd_en_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        struct tcd_instance *tcd = otg->tcd;
        int i;
        u32 hwmode = fs_rl(OTG_CORE_HWMODE);
        unsigned long flags;

        /* puts OTG capable port into a state where host is enabled */

        local_irq_save(flags);
        //fs_andl(OTG_CORE_HWMODE, 0xfffffff0);                  // clear

        switch (flag) {
        case SET:
                TRACE_MSG0(HCD, "SET");
                //if (hwmode & MODULE_CRECFG_HOST) {
                //        printk(KERN_INFO"%s: warning FUNC STILL SET\n", __FUNCTION__);
                //}
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);
                fs_orl(OTG_CORE_HWMODE, MODULE_CRECFG_HOST);           // set to software hnp
                hcd_instance->active = TRUE;
                if (!tcd->id) {
                        TRACE_MSG1(HCD, "FRM_INTRVL: %8x current", fs_rl(OTG_CORE_FRM_INTVL));
                        //fs_wl(OTG_CORE_FRM_INTVL, 4000 | MODULE_RESET_FRAME);
                        TRACE_MSG1(HCD, "FRM_INTRVL: %8x reset", fs_rl(OTG_CORE_FRM_INTVL));

                }

                break;
        case RESET:
                TRACE_MSG0(HCD, "RESET");
                hcd_instance->active = FALSE;
                fs_andl(OTG_CORE_HWMODE, ~MODULE_CRECFG_HOST);           // set to software hnp
                break;
        }

        local_irq_restore(flags);
        TRACE_MSG1(HCD, "HWMODE: %08x", fs_rl(OTG_CORE_HWMODE));
}

u32 otg_core_frm_intvl;

/*!
 * mxc_hcd_rh_func() - otg hcd root hub output function
 * @param otg
 * @param flag
 */
void mxc_hcd_rh_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct tcd_instance *tcd = otg->tcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        int i;
        u32 mask;
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);
        //printk(KERN_INFO"%s:\n", __FUNCTION__);

        switch (flag) {
        case SET:
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);

                local_irq_save(flags);

                /* reset the host core
                 */
                fs_wl_clr(HCD, OTG_CORE_RST_CTRL, MODULE_RSTRH | MODULE_RSTHSIE | MODULE_RSTHC);
                while (fs_rl(OTG_CORE_RST_CTRL));

                mxc_host_clock_on();


                for (i = 0; i < NUM_DATA_BUFFS; i++)
                        (void) rel_data_buff(mxc_hcd, ((fs_data_buff *)OTG_DATA_BASE)+i);
#if 0
                for (i = 0; i < NUM_ETDS; i++)
                        rel_etd_irq(mxc_hcd,i);
#endif
                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCRESET | HOST_CONTROL_RMTWUEN |
                                HOST_CONTROL_HCUSBSTE_RESET | HOST_CONTROL_CTLBLKSR_11);


                TRACE_MSG0(HCD, "HW HCD_ENABLE_SET");
                fs_andl(OTG_CORE_HNP_CSTAT, ~(MODULE_MASTER | MODULE_SLAVE | MODULE_CMPEN |
                                        MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ));

                fs_rl(OTG_CORE_HNP_CSTAT);

                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_MASTER | MODULE_CMPEN | MODULE_BGEN | MODULE_ABBUSREQ);
                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_ARMTHNPE | MODULE_BHNPEN); // XXX

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                //hcd_hw_enable_interrupts(bus_hcpriv);

                mxc_hcd->int_mask = (HOST_PSCINT_EN | HOST_FMOFINT_EN | HOST_HERRINT_EN |
                                HOST_RESDETINT_EN | /* HOST_SOFINT_EN | */ HOST_DONEINT_EN | HOST_SORINT_EN);

                TRACE_MSG1(HCD, "HW HCD_ENABLE_SET: mask: %02x", mxc_hcd->int_mask);

                #if 1
                // R1: sec 23.11.15 pg 23-54
                fs_rl(OTG_CORE_HNP_CSTAT);
                fs_rl(OTG_HOST_CONTROL);
                fs_rl(OTG_HOST_ROOTHUB_STATUS);
                fs_rl(OTG_HOST_PORT_STATUS_1);
                fs_rl(OTG_HOST_PORT_STATUS_2);
                fs_rl(OTG_HOST_PORT_STATUS_3);

                fs_wl(OTG_HOST_SINT_STEN, mxc_hcd->int_mask);
                fs_rl(OTG_HOST_SINT_STEN);

                mxc_hcd->otg_port_enabled = TRUE;
                //printk(KERN_INFO"%s: is_b_host FALSE\n", __FUNCTION__);

                // XXX need to get ID_GND

                mxc_hcd->hcd.self.is_b_host = !tcd->id;

                if (!tcd->id)
                        fs_orl(OTG_CORE_HNP_CSTAT, MODULE_SWAUTORST); // XXX

                for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                        mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_POWER, i, TRUE);
                        //mxc_hcd->hub_port_change_status |= (1 << i);
                }

                #endif
                local_irq_restore(flags);

                break;

        case RESET:

                mxc_hcd->otg_port_enabled = FALSE;              // XXX check this
                mxc_hcd->hcd.self.is_b_host = TRUE;
                //printk(KERN_INFO"%s: is_b_host TRUE\n", __FUNCTION__);

                #if 0
                local_irq_save(flags);

                //printk(KERN_INFO"%s: RESET\n", __FUNCTION__);
                TRACE_MSG0(HCD, "HW HCD_ENABLE_RESET");

                //hcd_hw_disable_interrupts(bus_hcpriv);
                // R1: sec 23.11.15 pg 23-54
                fs_wl(OTG_HOST_SINT_STAT, 0);

                fs_andl(OTG_HOST_CONTROL, ~HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                // Shut down hardware if not already shut down....
                //hcd_hw_disable_interrupts(bus_hcpriv);

                mxc_host_clock_off();
                local_irq_restore(flags);
                #endif
                break;
        }
}

/*!
 * mxc_loc_sof_func() - otg loc sof output function
 * @param otg
 * @param flag
 */
void mxc_loc_sof_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        //struct tcd_instance *tcd = otg->tcd;
        int i;
        u32 hwmode = fs_rl(OTG_CORE_HWMODE);
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);
        //printk(KERN_INFO"%s:\n", __FUNCTION__);

        switch (flag) {
        case SET:
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);

                local_irq_save(flags);

                #if 0
                /* reset the host core
                 */
                fs_wl_clr(HCD, OTG_CORE_RST_CTRL, MODULE_RSTRH | MODULE_RSTHSIE | MODULE_RSTHC);
                while (fs_rl(OTG_CORE_RST_CTRL));

                mxc_host_clock_on();

                for (i = 0; i < NUM_DATA_BUFFS; i++)
                        (void) rel_data_buff(mxc_hcd, ((fs_data_buff *)OTG_DATA_BASE)+i);

                for (i = 0; i < NUM_ETDS; i++)
                        rel_etd_irq(mxc_hcd,i);

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCRESET | HOST_CONTROL_RMTWUEN |
                                HOST_CONTROL_HCUSBSTE_RESET | HOST_CONTROL_CTLBLKSR_11);


                TRACE_MSG0(HCD, "HW HCD_ENABLE_SET");
                fs_andl(OTG_CORE_HNP_CSTAT, ~(MODULE_MASTER | MODULE_SLAVE | MODULE_CMPEN |
                                        MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ));
                fs_rl(OTG_CORE_HNP_CSTAT);

                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_MASTER | MODULE_CMPEN | MODULE_BGEN | MODULE_ABBUSREQ);
                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_ARMTHNPE | MODULE_BHNPEN); // XXX

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                //hcd_hw_enable_interrupts(bus_hcpriv);

                mxc_hcd->int_mask = (HOST_PSCINT_EN | HOST_FMOFINT_EN | HOST_HERRINT_EN |
                                HOST_RESDETINT_EN | /* HOST_SOFINT_EN | */ HOST_DONEINT_EN | HOST_SORINT_EN);

                TRACE_MSG1(HCD, "HW HCD_ENABLE_SET: mask: %02x", mxc_hcd->int_mask);


                // R1: sec 23.11.15 pg 23-54
                fs_rl(OTG_CORE_HNP_CSTAT);
                fs_rl(OTG_HOST_CONTROL);
                fs_rl(OTG_HOST_ROOTHUB_STATUS);
                fs_rl(OTG_HOST_PORT_STATUS_1);
                fs_rl(OTG_HOST_PORT_STATUS_2);
                fs_rl(OTG_HOST_PORT_STATUS_3);

                fs_wl(OTG_HOST_SINT_STEN, mxc_hcd->int_mask);
                fs_rl(OTG_HOST_SINT_STEN);

                mxc_hcd->otg_port_enabled = TRUE;

                for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                        mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_POWER, i, TRUE);
                        //mxc_hcd->hub_port_change_status |= (1 << i);
                }

                #endif
                local_irq_restore(flags);

                break;

        case RESET:
                local_irq_save(flags);

                //printk(KERN_INFO"%s: RESET\n", __FUNCTION__);
                TRACE_MSG0(HCD, "HW HCD_ENABLE_RESET");

                //hcd_hw_disable_interrupts(bus_hcpriv);
                // R1: sec 23.11.15 pg 23-54
                fs_wl(OTG_HOST_SINT_STAT, 0);

                fs_andl(OTG_HOST_CONTROL, ~HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                // Shut down hardware if not already shut down....
                //hcd_hw_disable_interrupts(bus_hcpriv);

                mxc_host_clock_off();
                local_irq_restore(flags);
                break;
        }

}
/*! mxc_hcd_loc_suspend_func -
 * @param otg
 * @param on
 */

void mxc_hcd_loc_suspend_func(struct otg_instance *otg, u8 on)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        int i;
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);

        switch (on) {
        case SET:
                TRACE_MSG0(HCD, "OUTPUT: RH LOC_SUSPEND SET");
                break;

        case RESET:
                TRACE_MSG0(HCD, "OUTPUT: RH LOC_SUSPEND RESET");
                break;
        }
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * mxc_pcd_framenum() - get current framenum
 * @param otg - otg instance
 */
static u16
mxc_hcd_framenum (struct otg_instance *otg)
{
        //printk(KERN_INFO"%s: framenum: %04x\n", __FUNCTION__,    fs_rl(OTG_HOST_FRM_NUM) );
        return fs_rl(OTG_HOST_FRM_NUM);
}



#if !defined(OTG_C99)
#if defined(LINUX26)
int mxc_hcd_mod_init_l26(struct otg_instance *otg);
void mxc_hcd_mod_exit_l26(struct otg_instance *otg);
struct hcd_ops hcd_ops = { mxc_hcd_mod_init_l26, mxc_hcd_mod_exit_l26, };
#else /* defined(LINUX26) */
int mxc_hcd_mod_init_l24(struct otg_instance *otg);
void mxc_hcd_mod_exit_l24(struct otg_instance *otg);
struct hcd_ops hcd_ops = { mxc_hcd_mod_init_l24, mxc_hcd_mod_exit_l24, };
#endif /* defined(LINUX26) */

/*!
 * fs_hcd_global_init() - initialize global vars for non C99 systems
 */
void fs_hcd_global_init(void)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__);

        ZERO(hcd_ops);
        hcd_ops.name = "MX21 HCD";
        hcd_ops.max_ports = 1;
        hcd_ops.capabilities = 0;
                                                       // module
        hcd_ops.mod_init = mxc_hcd_mod_init;           // called for module init
#ifdef MODULE
        hcd_ops.mod_exit = mxc_hcd_mod_exit;           // called for module exit
#endif
                                                       // otg state machine
        hcd_ops.hcd_init_func = hcd_init_func;         // initialize when otg enabled
        hcd_ops.hcd_en_func = mxc_hcd_en_func;         // setup hardware as host
        hcd_ops.hcd_rh_func = mxc_hcd_rh_func;         // start root hub
        hcd_ops.loc_suspend_func = mxc_hcd_loc_suspend_func;        // enable port on hub
        hcd_ops.loc_sof_func = mxc_loc_sof_func;       // enable port on hub
        hcd_ops.framenum = mxc_hcd_framenum;
};

/* ********************************************************************************************* */
#else /* !defined(OTG_C99) */
int mxc_hcd_mod_init_l26(struct otg_instance *otg);
void mxc_hcd_mod_exit_l26(struct otg_instance *otg);
struct hcd_ops hcd_ops = {
        .mod_init = mxc_hcd_mod_init_l26,              // called for module init
#ifdef MODULE
        .mod_exit = mxc_hcd_mod_exit_l26,              // called for module exit
#endif

        .name = "MX21 HCD",
        .max_ports = 1,
        .capabilities = 0,
                                                // module
        .hcd_init_func = hcd_init_func,         // initialize when otg enabled
        .hcd_en_func = mxc_hcd_en_func,         // setup hardware as host
        .hcd_rh_func = mxc_hcd_rh_func,         // start root hub
        .loc_suspend_func = mxc_hcd_loc_suspend_func,  // enable port on hub
        .loc_sof_func = mxc_loc_sof_func,        // enable port on hub
        .framenum = mxc_hcd_framenum,
};
#endif /* !defined(OTG_C99) */


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Linux 2.6 USB Device Support
 * New style linux 2.6.10 USB Core support
 */

/*
 * From drivers/usb/host/hcd.c:
 *    disables the endpoint: cancels any pending urbs, then synchronizes with
 *    the hcd to make sure all endpoint state is gone from hardware. use for
 *    set_configuration, set_interface, driver removal, physical disconnect.
 */
/*! mxc_hcd_endpoint_disable -
 * @param hcd - usb_hcd instance
 * @param ep - endpoint
 */
void mxc_hcd_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *ep)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct mxc_req          *mxc_req;
        unsigned long           flags;
        int                     is_out = (ep->desc.bEndpointAddress & USB_DIR_IN) ? 0 : 1;
	unsigned int		pipe = (unsigned int) ep->hcpriv;
	int			address = usb_pipedevice(pipe) ?
					(usb_pipedevice(pipe) % MXC_MAX_USB_ADDRESS + 1) : 0;
        TRACE_MSG0(HCD, "--");
        local_irq_save(flags);

        if ((mxc_req = mxc_hcd->ep[address][EPNUM(ep->desc.bEndpointAddress, is_out)])) {
                //printk(KERN_INFO"%s: disabled\n", __FUNCTION__);
                rel_etd_irq(mxc_hcd, mxc_req->etdn);
                mxc_hcd->ep[address][EPNUM(ep->desc.bEndpointAddress, is_out)] = NULL;

                // XXX giveback?
                mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, 0);  // check if 0 is correct status to return
        }

        local_irq_restore(flags);
}

int mxc_rh_frame(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        return fs_rl(OTG_HOST_FRM_NUM);
}

/*! mxc_rh_descriptor -get root hub descriptor
 * @param mxc_hcd - mxc_hcd instance
 * @param desc - root hub descriptor pointer
 */
static void
mxc_rh_descriptor ( struct mxc_hcd *mxc_hcd, struct usbp_hub_descriptor *desc)
{
        u16             temp = 0;

        desc->bDescLength = 9;
        desc->bDescriptorType = 0x29;

        desc->bNbrPorts = mxc_hcd->bNbrPorts;
        desc->wHubCharacteristics = mxc_hcd->wHubCharacteristics;
        desc->bPwrOn2PwrGood = mxc_hcd->bPwrOn2PwrGood;
        desc->bHubContrCurrent = 0;
        desc->DeviceRemovable = mxc_hcd->DeviceRemovable;
        desc->PortPwrCtrlMask = mxc_hcd->PortPwrCtrlMask;
}


/*! mxc_rh_status_data() - provide hub status data to usb core root hub
 *
 * This function is used by the linux 2.6 virtual root hub to poll for
 * changes.
 * @param hcd - usb_hcd instance
 * @param buf -
 *
 */
int mxc_rh_status_data(struct usb_hcd *hcd, char *buf)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);

        RETURN_ZERO_UNLESS(mxc_hcd->otg_port_enabled);
        RETURN_ZERO_UNLESS(mxc_hcd->virt_hub_port_change_status);

        *buf = mxc_hcd->virt_hub_port_change_status;
        mxc_hcd->virt_hub_port_change_status = 0;

        TRACE_MSG1(HCD, "changed buf: %02x", *buf);
        return 1;
}
/*! mxc_rh_control - called to process control endpoint request
 *
 * @param hcd -
 * @param typeReq
 * @param wValue
 * @param wIndex
 * @param buf
 * @param wLength
 * @return
 */
int mxc_rh_control( struct usb_hcd  *hcd, u16 typeReq, u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        unsigned long   flags;
        u32             status;

        TRACE_MSG4(HCD, "typeReq: %04x wValue: %04x wIndex: %04x wLength: %04x", typeReq, wValue, wIndex, wLength);

        switch (typeReq) {
        case ClearHubFeature:
        case SetHubFeature:
                TRACE_MSG0(HCD, "ClearHubFeature/SetHubFeature");
                //mxc_rh_hub_feature(mxc_hcd, wValue, wIndex, typeReq == SetHubFeature);
                return 0;

        case SetPortFeature: {
                u32 virt_port_status;
                TRACE_MSG0(HCD, "SetPortFeature");
                RETURN_EPIPE_IF (wIndex > mxc_hcd->bNbrPorts || wLength);
                virt_port_status = mxc_hcd->virt_port_status[wIndex - 1];
                /* Feature selector is wValue, wIndex is 1-origin
                 */
                // SET feature
                switch (wValue) {
                case PORT_RESET:
                        mxc_hcd->virt_port_status[wIndex - 1] |= (1 << C_PORT_RESET | (1 << wValue));
                        mxc_hcd->virt_hub_port_change_status |= (1 << wIndex);
                        break;
                case PORT_SUSPEND:
                case PORT_POWER:
                        mxc_hcd->virt_port_status[wIndex - 1] |= (1 << wValue);
                        break;
                }
                TRACE_MSG3(HCD, "[%d]: %08x -> %08x", wIndex, virt_port_status, mxc_hcd->virt_port_status[wIndex -1]);
                //printk(KERN_INFO"%s: SetPortFeature[%d]: wValue: %d %08x -> %08x %s\n",
                //                __FUNCTION__, wIndex, wValue, virt_port_status, mxc_hcd->virt_port_status[wIndex -1],
                //                port_feature_name[wValue]);
                return 0;
        }
        case ClearPortFeature: {
                u32 virt_port_status;
                TRACE_MSG0(HCD, "ClearPortFeature");
                RETURN_EPIPE_IF ((wIndex > mxc_hcd->bNbrPorts) || wLength);
                virt_port_status = mxc_hcd->virt_port_status[wIndex - 1];
                /* Feature selector is wValue, wIndex is 1-origin
                 */
                // CLEAR feature (valid features from USB2.0 11.24.2.2 pg 423).
                switch (wValue) {
                case PORT_ENABLE: // Disable port.
                case PORT_SUSPEND: // Cause a Host initiated resume, or no-op if already active.
                case PORT_POWER: // Put port in powered-off state.
                case C_PORT_CONNECTION: // clear the PORT_CONNECTION change bit
                case C_PORT_RESET: // clear the PORT_RESET change bit
                case C_PORT_ENABLE: // clear the PORT_ENABLE change bit
                case C_PORT_SUSPEND: // clear the PORT_SUSPEND change bit
                case C_PORT_OVER_CURRENT: // clear the PORT_OVERCURRENT change bit
                        mxc_hcd->virt_port_status[wIndex - 1] &= ~(1 << wValue);
                        break;
                }
                TRACE_MSG3(HCD, "[%d]: %08x -> %08x", wIndex, virt_port_status, mxc_hcd->virt_port_status[wIndex -1]);
                //printk(KERN_INFO"%s: ClearPortFeature[%d]: wValue: %d %08x -> %08x %s\n",
                //                __FUNCTION__, wIndex, wValue, virt_port_status, mxc_hcd->virt_port_status[wIndex -1],
                //                port_feature_name[wValue]);
                return 0;
        }

        case GetHubDescriptor:
                //printk(KERN_INFO"%s: GetHubDescriptor\n", __FUNCTION__);
                TRACE_MSG0(HCD, "GetHubDescriptor");
                mxc_rh_descriptor(mxc_hcd, (struct usbp_hub_descriptor *) buf);
                return 0;

        case GetHubStatus:
                //printk(KERN_INFO"%s: GetHubStatus\n", __FUNCTION__);
                //*(__le32 *) buf = cpu_to_le32(0);       // XXX this seems to be de rigeur for 2.6 root hubs...
                *(__le32 *) buf = cpu_to_le32(fs_rl(OTG_HOST_ROOTHUB_STATUS) & 0xffff);
                TRACE_MSG1(HCD, "GetHubStatus: %04x", *(__le32 *)buf);
                return 0;

        case GetPortStatus:
                //printk(KERN_INFO"%s: GetPortStatus\n", __FUNCTION__);
                TRACE_MSG0(HCD, "GetPortStatus");
                RETURN_EPIPE_IF ((wIndex > mxc_hcd->bNbrPorts));
                /* use saved port change status and reset it
                 */
                status = mxc_hcd->virt_port_status[wIndex - 1]; // | fs_rl(fs_host_port_stat(wIndex))
                *(__le32 *) buf = cpu_to_le32( status);

                TRACE_MSG3(HCD, "GetPortStatus: port: %d port_change_status: %08x buf: %08x",
                                wIndex, status, *(__le32 *) buf);

                //printk(KERN_INFO"%s: GetPortStatus port: %d status: %08x\n",
                //                __FUNCTION__, wIndex, status);

                /* Mini-state machine to progress to PORT_CONNECT in virtual root hub port */
                if (status & (1 << PORT_RESET)) {
                        mxc_hcd->virt_port_status[wIndex - 1] &= ~(1 << PORT_RESET);
                        mxc_hcd->virt_port_status[wIndex - 1] |=
                                (1 << C_PORT_RESET) |
                                (1 << C_PORT_ENABLE) |
                                (1 << PORT_ENABLE)
                                ;
                        mxc_hcd->virt_hub_port_change_status |= (1 << wIndex);
                        //printk(KERN_INFO"%s: GetPortStatus port: %d status: %08x CLEARED PORT_RESET\n",
                        //                __FUNCTION__, wIndex, mxc_hcd->virt_port_status[wIndex - 1]);
                }
                return 0;

        default:
                //printk(KERN_INFO"%s: Default\n", __FUNCTION__);
                /* "protocol stall" on error */
                return -EPIPE;
        }
}

/*! mxc_rh_suspend - called to suspend root hub
 * @param hcd - usb_hcd instance
 */
int mxc_rh_suspend(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        return 0;
}
/*! mxc_rh_resume - called to resum root hub
 * @param hcd - usb_hcd instance
 * @return 0
 */
int mxc_rh_resume(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        return 0;
}

/*! mxc_hcd_stop - called to stop hcd
 * @param hcd - usb_hcd instance
 */
static void
mxc_hcd_stop(struct usb_hcd *hcd)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(HCD, "--");
        del_timer_sync(&hcd->rh_timer);
}

static int
mxc_hcd_start(struct usb_hcd *hcd)
{
	hcd->state = HC_STATE_RUNNING;
        return 0;
}

/*!  struct hcd_driver mxc_hc_driver */
static const struct hc_driver mxc_hc_driver = {
        .description =          "mxc-hcd",
	.product_desc = "Freescale On-Chip USB Host Controller",
	.hcd_priv_size = sizeof(struct mxc_hcd) - sizeof(struct usb_hcd),

        /*
         * generic hardware linkage
         */
        .flags =                HCD_USB11,

	.start = 		mxc_hcd_start,
	.stop = 		mxc_hcd_stop,

        /*
         * managing i/o requests and associated device resources
         */
        .urb_enqueue =          mxc_hcd_urb_enqueue,
        .urb_dequeue =          mxc_hcd_urb_dequeue,
        .endpoint_disable =     mxc_hcd_endpoint_disable,

        /*
         * periodic schedule support
         */
        .get_frame_number =     mxc_rh_frame,

        /*
         * root hub support
         */
        .hub_status_data =      mxc_rh_status_data,
        .hub_control =          mxc_rh_control,
        .bus_suspend =          mxc_rh_suspend,
        .bus_resume =           mxc_rh_resume,
};

/* ********************************************************************************************* */
/*! mxc_hcd_remove - called to remove hcd
 * @param dev
 */
static int
mxc_hcd_remove(struct platform_device *dev)
{
        struct mxc_hcd *hcd = platform_get_drvdata(dev);

	usb_remove_hcd(&hcd->hcd);

	usb_put_hcd(&hcd->hcd);

        return 0;
}

static void mxc_dev_release(struct platform_device *device)
{
        TRACE_MSG0(HCD, "--");
        //usb_hcd_release((struct usb_bus *) dev);
}

/*! mxc_hcd_probe - called to initialize hcd
 * @param dev - hcd device
 */
static int mxc_hcd_probe(struct platform_device *dev)
{
        struct platform_device  *platform_device = NULL;
        struct mxc_hcd *hcd;

	hcd = (struct mxc_hcd *) usb_create_hcd(&mxc_hc_driver, &dev->dev, dev->dev.bus_id);
	if (!hcd) {
		return -ENOMEM;
	}

        INIT_LIST_HEAD(&hcd->unused);
        INIT_LIST_HEAD(&hcd->inactive);

        platform_set_drvdata(dev, hcd);

        //usb_bus_init(&hcd->hcd.self);
        //hcd->hcd.self.op = &usb_hcd_operations;
        hcd->hcd.self.hcpriv = hcd;
        hcd->hcd.self.is_b_host = TRUE;
        hcd->hcd.self.otg_port = 1;
        hcd->root_hub_desc_a = fs_rl(OTG_HOST_ROOTHUB_DESCA);
        hcd->root_hub_desc_b = fs_rl(OTG_HOST_ROOTHUB_DESCB);
        hcd->bNbrPorts = hcd->root_hub_desc_a & 0xff;
        hcd->wHubCharacteristics = (hcd->root_hub_desc_a >> 8) & 0xff;
        hcd->bPwrOn2PwrGood = (hcd->root_hub_desc_a >> 24) & 0xff;
        hcd->DeviceRemovable = hcd->root_hub_desc_b & 0xff;
        hcd->PortPwrCtrlMask = (hcd->root_hub_desc_b >> 16) & 0xff;
        hcd->allocated_count = hcd->active_count = 0;

        TRACE_MSG2(HCD, "root_hub descA: %08x descB: %08x", hcd->root_hub_desc_a, hcd->root_hub_desc_b);

        TRACE_MSG5(HCD, "bNbrPorts: %02x wHubCharacteristics: %02x bPwrOn2PwrGood: %02x "
                        "DeviceRemovalbe: %02x PortPwrCtrlMask: %02x",
                        hcd->bNbrPorts,
                        hcd->wHubCharacteristics,
                        hcd->bPwrOn2PwrGood,
                        hcd->DeviceRemovable,
                        hcd->PortPwrCtrlMask
                        );

        //INIT_LIST_HEAD(&hcd->hcd.dev_list);

        //hcd->hcd.self.release = &mxc_hcd_release;

        hcd->hcd.driver = &mxc_hc_driver;
        hcd->hcd.irq = -1;
        //hcd->hcd.state = USB_STATE_HALT;

        spin_lock_init(&hcd->lock);

	THROW_IF(usb_add_hcd(&hcd->hcd, 0, IRQF_SHARED), error);;

        hcd_instance->privdata = &hcd->hcd;

        THROW_IF(mxc_host_gpio(), error);

        return 0;

        CATCH(error) {
                if (hcd) {
                        if (hcd) mxc_hcd_stop(&hcd->hcd);
			usb_put_hcd(&hcd->hcd);
                }
                return -ENODEV;
        }
}

/*! mxc_hcd_driver - define a platform_driver structure for this driver
 */
static struct platform_driver mxc_hcd_driver = {
	.driver = {
        .name =         "mxc-hcd",
	},

        .probe =        mxc_hcd_probe,
        .remove =       mxc_hcd_remove,
};

static void mxc_hcd_release(struct usb_bus *bus)
{
        TRACE_MSG0(HCD, "--");
        //usb_hcd_release((struct usb_bus *) dev);
}

/*! mxc_hcd_platform - define a platform_device structure for this driver
 */
static struct platform_device mxc_hcd_platform = {
        .name       = "mxc-hcd",
        .id         = 1,
};

/* ********************************************************************************************* */
/* ********************************************************************************************* */

extern char *etd_urb_state_name[];
/*! mxc_hcd_show - called to display hcd information
 * @param s
 * @param unused
 */
static int mxc_hcd_show(struct seq_file *s, void *unused)
{
        struct mxc_hcd          *mxc_hcd = hcd_instance->privdata;      // XXX this should come from dev_id
        struct mxc_req          *mxc_req;
        struct mxc_req          *mxc_req_save;

        int                     etdn;
        unsigned long           flags;


        seq_printf(s, "MXC HCD\n\n");

        for (etdn = 0; etdn < NUM_ETDS; etdn++) {

                local_irq_save(flags);

                if ((mxc_req = mxc_hcd->active[etdn])) {
                        seq_printf(s, "[%02x] Active urb: %08x epnum: %02x state: %s\n", etdn,
                                        mxc_req->urb,
                                        mxc_req->epnum,
                                        etd_urb_state_name[mxc_req->etd_urb_state]
                                  );
                }

                local_irq_restore(flags);

        }
        seq_printf(s, "\n");

        local_irq_save(flags);
        etdn = 0;
        list_for_each_entry(mxc_req, &mxc_hcd->inactive, queue) {
                seq_printf(s, "[%02x] Inactive urb: %08x epnum: %02x state: %s\n", etdn++,
                                mxc_req->urb,
                                mxc_req->epnum,
                                etd_urb_state_name[mxc_req->etd_urb_state]
                          );
        }
        local_irq_restore(flags);

        seq_printf(s, "\n");
        seq_printf(s, "Requests: Allocated: %d Active: %d Inactive: %d Total: %d\n",
                        mxc_hcd->allocated_count, mxc_hcd->active_count, etdn, mxc_hcd->request_count);

        seq_printf(s, "\n");
        return 0;
}

/*! mxc_hcd_open - open a hcd file
 * @param inode
 * @param file
 */

static int mxc_hcd_open(struct inode *inode, struct file *file)
{
        return single_open(file, mxc_hcd_show, PDE(inode)->data);
}
/*!  struct file_operatons mxc_hcd_proc_ops */
static struct file_operations mxc_hcd_proc_ops = {
        .open           = mxc_hcd_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static const char proc_filename[] = "mxchcd";


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Module init/exit
 *
 * Register this a linux device driver and platform. This provides the necessary framework
 * expected by the Linux 2.6 USB Core.
 *
 */
/*! mxc_hcd_mod_init_l26 -called to register hcd driver and platform
 * @param otg - otg instance
 */

int mxc_hcd_mod_init_l26(struct otg_instance *otg)
{
        u32 hwmode;
        int driver_registered = 0;
        int platform_registered = 0;
        struct proc_dir_entry *pde = NULL;

        printk(KERN_INFO"%s: MXC-HCD.C (dev)\n", __FUNCTION__);

        if (usb_disabled()) {
                //printk(KERN_INFO"%s: YYYYY\n", __FUNCTION__);
                return -ENOMEM;
        }

        THROW_IF((driver_registered = platform_driver_register(&mxc_hcd_driver)), error);

        THROW_IF((platform_registered = platform_device_register(&mxc_hcd_platform)), error);

        THROW_UNLESS((pde = create_proc_entry(proc_filename, 0, NULL)), error);
        pde->proc_fops = &mxc_hcd_proc_ops;

        CATCH(error) {
                //printk(KERN_INFO"%s: ZZZZ\n", __FUNCTION__);
                if (platform_registered) platform_device_unregister(&mxc_hcd_platform);
                if (driver_registered) platform_driver_unregister(&mxc_hcd_driver);
                if (pde) remove_proc_entry(proc_filename, NULL);
        }

        return 0;
}

#ifdef MODULE
/*! mxc_hcd_mod_exit_l26 - called to release hcd module
 * @param otg - otg instance
 */
void mxc_hcd_mod_exit_l26(struct otg_instance *otg)
{
        remove_proc_entry(proc_filename, NULL);

        platform_driver_unregister(&mxc_hcd_driver);

        platform_device_unregister(&mxc_hcd_platform);

}
#endif
