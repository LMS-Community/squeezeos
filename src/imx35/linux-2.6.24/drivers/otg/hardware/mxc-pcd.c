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
 * otg/hardware/mxc-pcd.c -- Freescale USBOTG Peripheral Controller driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/mxc/mxc-pcd.c|20070822214154|21475
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/mxc-pcd.c
 * @brief Freescale USB Peripheral Controller Driver
 * This implements the Freescale USBOTG Peripheral Controller Driver.
 *
 * There is some processor specific code here to support the alternate processors
 * that implement this type of USB support.
 *
 * There is no board or platform level code here.
 *
 * @ingroup FSOTG
 * @ingroup PCD
 *
 *
 * Notes
 *
 * 1. EP_RDY is a toggle register, not R/W.
 *
 * 2. There is an EP_RDY_CLR register (only for TO2?)
 *
 * 3. EP_TOGGLE is RW for TO1 and toggle for TO2.
 *
 * 4. DMA transfers always transfer buffersize amounts of data.
 *
 * 5. DMA IN transfers for very short (2 byte) transfers can take longer than the
 *    data being sent causing done status change to fail.
 *
 * 6. DMA IN transfers must be monitored and the DMA enable forcibly reset if it
 *    does not clear by itself.
 *
 */

#include <otg/pcd-include.h>
#if defined(CONFIG_OTG_LNX)
#include "mxc-lnx.h"
#endif /* defined(CONFIG_OTG_LNX) */
#if defined(CONFIG_OTG_QNX)
#include "mx21.h"
#include "mxc-qnx.h"
#endif /* defined(CONFIG_OTG_QNX) */
#include "mxc-hardware.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */


//#define SHORT_DMA_IN            /* define this to force transfers to be maximum of 2*wMaxPacketSize */
//#define NO_DMA_IN               /* define this to use memcpy instead of DMA */

//#define SHORT_DMA_OUT           /* define this to force transfers to be maximum of 2*wMaxPacketSize */
//#define NO_DMA_OUT              /* define this to use memcpy instead of DMA */

//#define MXC_ACTIVE              /* active SOF checking */
//#define MXC_TRACK_TIMER

#define TRACE_VERBOSE 0
#define TRACE_VERY_VERBOSE 0

struct pcd_instance *mxc_pcd_instance;

void mxc_func_clock_on(void);                   /* defined in mx2-ocd.c */
void mxc_func_clock_off(void);                  /* defined in mx2-ocd.c */
void mxc_main_clock_on(void);
void mxc_main_clock_off(void);


u8 mxc_new_address, mxc_usb_address;            /* used to save and set USB address */
u16 mxc_funcrev;

#ifdef MXC_TRACK_TIMER
void mxc_pcd_hrt_callback (unsigned long arg)
{
        struct usbd_endpoint_instance *endpoint=(struct usbd_endpoint_instance *) arg;
        printk(KERN_INFO "\n");
        printk(KERN_INFO"%s: The endpoint_instance got time Out while waiting Done interrupt-----urbs in queue:%ld\n", __FUNCTION__,(endpoint->rdy).total);
}

#endif

/* ********************************************************************************************* */

/* The EP Toggle register has different ways of setting and clearing
 * depending on hardware version. The older hardware is R/W. The newer
 * hardware is write to toggle the current value.
 */


void mxc_halt_endpoint0_in(int pipe)
{

        u32 ep0_current, ep0_new;

        ep0_current = fs_rl(ep_word(0, pipe, 0));
        ep0_new =  (1 << 31) | (ep0_current);

        UNLESS (ep0_current == ep0_new) fs_wl(ep_word(0,pipe, 0), ep0_new);

}

/*! mxc_reset_toggles
 */
static void inline
mxc_reset_toggles(void)
{
        //u32 toggle = fs_rl(OTG_FUNC_EP_TOGGLE);
        if (mxc_funcrev == 0x11) {
                /* TO1 - RW register, simply or the bit and write
                 */
                fs_wl(OTG_FUNC_EP_TOGGLE, 0);
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG2(mxc_pcd_instance->TAG, "EP_TOGGLES: %08x -> %08x (1.1)", toggle, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
        else {
                /* TO2 - RC - check and then write the bit to toggle to the other value if required
                 */
                fs_wl(OTG_FUNC_EP_TOGGLE, fs_rl(OTG_FUNC_EP_TOGGLE));
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG2(mxc_pcd_instance->TAG,  "EP_TOGGLES: %08x -> %08x (2.0)", toggle, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
}

/*! mxc_reset_toggle_ep
 */
static void inline
mxc_reset_toggle_ep(int ep_num)
{
        //u32 toggle = fs_rl(OTG_FUNC_EP_TOGGLE);
        if (mxc_funcrev == 0x11) {
                /* TO1 - RW register, simply or the bit and write
                 */
                fs_andl(OTG_FUNC_EP_TOGGLE, ~ep_num);
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG3(mxc_pcd_instance->TAG, "EP_TOGGLE[%d]: %08x -> %08x (1.1)",
                //                        toggle, ep_num, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
        else {
                /* TO2 - RC - check and then write the bit to toggle to the other value if required
                 */
                if (fs_rl(OTG_FUNC_EP_TOGGLE) & ep_num) fs_wl(OTG_FUNC_EP_TOGGLE, ep_num);
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG3(mxc_pcd_instance->TAG,  "EP_TOGGLE[%d]: %08x -> %08x (2.0)",
                //                        toggle, ep_num, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
}

/*! mxc_set_toggle_ep
 */
static void inline
mxc_set_toggle_ep(int ep_num)
{
        //u32 toggle = fs_rl(OTG_FUNC_EP_TOGGLE);
        if (mxc_funcrev == 0x11) {
                /* TO1 - RW register, simply or the bit and write
                 */
                fs_orl(OTG_FUNC_EP_TOGGLE, ep_num);
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG2(mxc_pcd_instance->TAG, "EP OR: %08x -> %08x (1.1)", toggle, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
        else {
                /* TO2 - RC - check and then write the bit to toggle to the other value if required
                 */
                UNLESS (fs_rl(OTG_FUNC_EP_TOGGLE) & ep_num) fs_wl(OTG_FUNC_EP_TOGGLE, ep_num);
                //if (TRACE_VERY_VERBOSE)
                //        TRACE_MSG2(mxc_pcd_instance->TAG, "EP TOGGLE: %08x -> %08x (2.0)", toggle, fs_rl(OTG_FUNC_EP_TOGGLE));
        }
}
/* ********************************************************************************************* */
/*
 * N.B. EP_RDY is a toggle not RW register.
 *
 * XXX need to check what TO1 does. I think it is RW register.
 */

/*! mxc_reset_ep_rdy
 */
static void /* inline */
mxc_reset_ep_rdy(int ep_num)
{
        u32 ep_rdy;
        RETURN_UNLESS ((ep_rdy = fs_rl(OTG_FUNC_EP_RDY)) & ep_num);
        if (mxc_funcrev == 0x11) {
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "Toggle: ep_rdy: %08x", fs_rl(OTG_FUNC_EP_RDY_CLR));
                fs_wl(OTG_FUNC_EP_RDY, ep_num);                 // toggle ready bit
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "Toggle: ep_rdy: %08x", fs_rl(OTG_FUNC_EP_RDY_CLR));
        }
        else {
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "Clear: ep_rdy: %08x", fs_rl(OTG_FUNC_EP_RDY_CLR));
                fs_wl(OTG_FUNC_EP_RDY_CLR, ep_num);    // clear ready bit
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "Clear: ep_rdy: %08x", fs_rl(OTG_FUNC_EP_RDY_CLR));
        }
}

/*! mxc_set_ep_rdy
 *
 * N.B. EP_RDY is a toggle not RW register.
 */
static void /* inline */
mxc_set_ep_rdy(int ep_num)
{
        u32 ep_rdy;;
        RETURN_IF ((ep_rdy = fs_rl(OTG_FUNC_EP_RDY)) & ep_num);
        fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_EP_RDY, ep_num);                // set ready bit
}
/* ********************************************************************************************* */
/*
 * These track IN endpoint transfers and enable the SOF interrupt
 * when (if) there is a transfer pending in any IN endpoint
 */
#ifdef MXC_ACTIVE
static u32 mxc_pcd_active;
#endif /* MXC_ACTIVE */

/*! mxc_sof_active
 */
void mxc_sof_active(u32 ep_num)
{
        #ifdef MXC_ACTIVE
        /* enable SOF interrupt if nothing is currently active */
        UNLESS (mxc_pcd_active)
                fs_orl(OTG_FUNC_SINT_STEN, SYSTEM_SOFDETINT_EN);        // XXX ORL - ok
        mxc_pcd_active |= ep_num;

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG3(mxc_pcd_instance->TAG, "ep_num: %08x active: %08x sint_sten: %08x",
                                ep_num, mxc_pcd_active, fs_rl(OTG_FUNC_SINT_STEN));
        #endif /* MXC_ACTIVE */
}

/*! mxc_sof_cancel
 */
void mxc_sof_cancel(u32 ep_num)
{
        #ifdef MXC_ACTIVE
        /* disable SOF interrupt if nothing is now active */
        mxc_pcd_active &= ~ep_num;
        UNLESS (mxc_pcd_active)
                fs_wl(OTG_FUNC_SINT_STEN_CLR, SYSTEM_SOFDETINT_EN);

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG3(mxc_pcd_instance->TAG, "ep_num: %08x active: %08x sint_sten: %08x",
                                ep_num, mxc_pcd_active, fs_rl(OTG_FUNC_SINT_STEN));
        #endif /* MXC_ACTIVE */
}


/* ********************************************************************************************* */

/*!
 * mxc_ep_config() - configure and enable an endpoint to send or receive data
 * First setup the endpoint descriptor words, and then enable the endpoint and done interrupt
 * @param epn - endpoint number
 * @param dir - direction IN or OUT
 * @param ep_num -
 * @param wMaxPacketSize
 * @param buffersize
 * @param format
 * @param ttlbtecnt
 * @param stall
 *
 * Adjust buffersize to match transfersize for IN transfers less than 2 *
 * packet size. Note that this requires that the x/y buffers are
 * sequentially allocated. Effectively this makes the DMA only transfer the
 * exact amount of the transfer.
 */
static void inline
mxc_ep_config(int epn, int dir, u32 ep_num, int wMaxPacketSize, int buffersize, int format, int ttlbtecnt, int stall)
{
        u32 ep_en = fs_rl(OTG_FUNC_EP_EN);
        u32 ep0_current = fs_rl(ep_word(epn, dir, 0));
        u32 ep1_current = fs_rl(ep_word(epn, dir, 1));
        u32 ep3_current = fs_rl(ep_word(epn, dir, 3));

        u32 ep0_new =  ((stall ? 1 : 0) << 31) | (wMaxPacketSize << 16) | ((format & 3) << 14);
        u32 ep1_new =  ttlbtecnt ? (data_y_buf(epn, dir) << 16) | data_x_buf(epn, dir): 0;
        u32 ep3_new =  ((buffersize - 1) << 21) | (ttlbtecnt & 0x1fffff);


        if (TRACE_VERBOSE) {
                TRACE_MSG7(mxc_pcd_instance->TAG, "epn[%02x:%02x] ep_num: %08x sz: %d fmt: %x cnt: %d stall: %d",
                                epn, dir, ep_num, wMaxPacketSize, format, ttlbtecnt, stall);
                TRACE_MSG7(mxc_pcd_instance->TAG, "ep_en: %08x ep0[%08x:%08x] ep1[%08x:%08x] ep3[%08x:%08x]", ep_en,
                                ep0_current, ep0_new, ep1_current, ep1_new, ep3_current, ep3_new);
                TRACE_MSG2(mxc_pcd_instance->TAG, "xfill: %08x yfill: %08x",
                                fs_rl(OTG_FUNC_XFILL_STAT), fs_rl(OTG_FUNC_YFILL_STAT));
        }

        /* disable interrupts to ensure that all changes to endpoint
         * configuration are completed with minimal latency
         */
        otg_disable_interrupts();

        UNLESS (ep0_current == ep0_new) fs_wl(ep_word(epn, dir, 0), ep0_new);
        UNLESS (ep1_current == ep1_new) fs_wl(ep_word(epn, dir, 1), ep1_new);
        UNLESS (ep3_current == ep3_new) fs_wl(ep_word(epn, dir, 3), ep3_new);
        UNLESS(ep_en & ep_num) fs_orl(OTG_FUNC_EP_EN, ep_num);    // enable the endpoint for use // XXX or required mx21

        fs_orl(OTG_FUNC_IINT, ep_num);              // issue done status interrupts immediately
        fs_orl(OTG_FUNC_EP_DEN, ep_num);            // enable endpoint done interrupt

        otg_enable_interrupts();

        /* interrupts enabled again */

        if (TRACE_VERBOSE) {
                TRACE_MSG7(mxc_pcd_instance->TAG, "epn[%02x:%02x] %08x %08x %08x TOG: %08x EP_EN: %08x", epn, dir,
                                fs_rl(ep_word(epn, dir, 0)), fs_rl(ep_word(epn, dir, 1)),
                                fs_rl(ep_word(epn, dir, 3)), fs_rl(OTG_FUNC_EP_TOGGLE), fs_rl(OTG_FUNC_EP_EN));

                TRACE_MSG7(mxc_pcd_instance->TAG, "epn[%02x:%02x] ep_num: %08x sz: %d fmt: %x cnt: %d stall: %d",
                                epn, dir, ep_num, wMaxPacketSize, format, ttlbtecnt, stall);
        }
#if 0
        printk (KERN_INFO"%s: epn[%02x:%02x] %08x %08x %08x TOG: %08x EP_EN: %08x\n", __FUNCTION__,
                        epn, dir, fs_rl(ep_word(epn, dir, 0)), fs_rl(ep_word(epn, dir, 1)),
                        fs_rl(ep_word(epn, dir, 3)), fs_rl(OTG_FUNC_EP_TOGGLE), fs_rl(OTG_FUNC_EP_EN));
        printk (KERN_INFO"%s: epn[%02x:%02x] ep_num: %08x sz: %d fmt: %x cnt: %d stall: %d\n", __FUNCTION__,
                        epn, dir, ep_num, wMaxPacketSize, format, ttlbtecnt, stall);
#endif
}

/*!
 * mxc_ep_config_nodma() - configure and enable an endpoint to send or receive data
 * Non-DMA setup, optional stall, ready etc.
 * @param epn - endpoint number
 * @param dir
 * @param stall - stall endpoint
 * @param wMaxPacketSize
 * @param format
 * @param ttlbtecnt
 * @param rdy1
 * @param rdy2
 */
static void
mxc_ep_config_nodma(int epn, int dir, int stall, int wMaxPacketSize, int format, int ttlbtecnt, int rdy1, int rdy2)
{
        u32 ep_num = ep_num_dir(epn, dir);

        if (TRACE_VERBOSE)
                TRACE_MSG5(mxc_pcd_instance->TAG, "EP CONFIG NODMA: epn[%02x] num: %02x stall: %d r1: %d r2: %d",
                                epn, ep_num, stall, rdy1, rdy2);
        if (rdy1) {
                if (TRACE_VERBOSE)
                        TRACE_MSG3(mxc_pcd_instance->TAG, "EP CONFIG NODMA: epn: %02x num: %02x checking RDY: %02x",
                                        epn, ep_num, rdy1 && fs_rl(OTG_FUNC_EP_RDY) & ep_num);
                RETURN_IF (rdy1 && (fs_rl(OTG_FUNC_EP_RDY) & ep_num));
        }

        //UNLESS(epn)
        //        fs_orl(OTG_FUNC_XYINT_STEN, ep_num);        // enable x or y buffer interrupt

        mxc_ep_config(epn, dir, ep_num, wMaxPacketSize, wMaxPacketSize, format, ttlbtecnt, stall);
        RETURN_UNLESS(rdy2);

        mxc_set_ep_rdy(ep_num);

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG4(mxc_pcd_instance->TAG, "IINT: %08x DEN: %08x EN: %08x RDY: %08x", fs_rl(OTG_FUNC_IINT),
                                fs_rl(OTG_FUNC_EP_DEN), fs_rl(OTG_FUNC_EP_EN), fs_rl(OTG_FUNC_EP_RDY));
}

/*!
 * mxc_pcd_start_ep0_setup() - enable ep0 for receiving SETUP request
 * Unless already ready, configure ep0 to receive.
 * @param endpoint - endpoint instance
 */
static void inline
mxc_pcd_start_ep0_setup (struct usbd_endpoint_instance *endpoint)
{
        if (TRACE_VERY_VERBOSE)
                TRACE_MSG0(mxc_pcd_instance->TAG, "START EP0: config");

        // XXX rdy1 must be FALSE for OUT and TRUE for IN
        mxc_ep_config_nodma(0, USB_DIR_OUT, 0, endpoint->wMaxPacketSize[0], EP_FORMAT_CONTROL,
                        endpoint->wMaxPacketSize[0], 0, 1);
        mxc_ep_config_nodma(0, USB_DIR_IN, 0, endpoint->wMaxPacketSize[0], EP_FORMAT_CONTROL, 0, 1, 0);
}

/*!
 * mxc_sendzlp() - start sending a buffer on a specified endpoint
 * Toggle XFILL_STAT or YFILL_STAT bit, then call mxc_ep_config_nodma().
 * @param epn - endpoint number
 * @param endpoint - endpoint instance
 * @param wMaxPacketSize
 */
static void inline
mxc_sendzlp (int epn, struct usbd_endpoint_instance *endpoint, int wMaxPacketSize)
{
        u32 ep_num_in = ep_num_in(epn);

        if (TRACE_VERBOSE)
                TRACE_MSG0(mxc_pcd_instance->TAG, "ZLP");
        if (!(fs_rl(OTG_FUNC_XFILL_STAT) & ep_num_in))
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_XFILL_STAT, ep_num_in);
        else if (!(fs_rl(OTG_FUNC_YFILL_STAT) & ep_num_in))
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_YFILL_STAT, ep_num_in);

        mxc_ep_config_nodma(epn, USB_DIR_IN, 0, endpoint->wMaxPacketSize[0], endpoint->bmAttributes[0] & 0x3, 0, 1, 1);
}

/*!
 * mxc_stop_ep() - stop endpoint
 * @param epn - endpoint number
 * @param dir - direction IN or OUT
 */
void mxc_stop_ep(int epn, int dir)
{
        int ep_num = ep_num_dir(epn, dir);

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG7(mxc_pcd_instance->TAG, "--> ep_rdy: %08x dma_ep: %08x iint: %08x int: %08x %08x fill: %08x %08x",
                                fs_rl(OTG_FUNC_EP_RDY), fs_rl(OTG_DMA_EP_CH_CLR), fs_rl(OTG_FUNC_IINT),
                                fs_rl(OTG_FUNC_XINT_STAT), fs_rl(OTG_FUNC_YINT_STAT),
                                fs_rl(OTG_FUNC_XFILL_STAT), fs_rl(OTG_FUNC_YFILL_STAT));

        mxc_reset_ep_rdy(ep_num);

        fs_wl(OTG_DMA_EP_CH_CLR, ep_num);

        fs_wl_set(mxc_pcd_instance->TAG,OTG_FUNC_IINT_CLR, ep_num);

        if (fs_rl(OTG_FUNC_XINT_STAT) & ep_num) {
                fs_wl_clr(mxc_pcd_instance->TAG, OTG_FUNC_XINT_STAT, ep_num);
                fs_wl_clr(mxc_pcd_instance->TAG, OTG_FUNC_XFILL_STAT, ep_num);
        }
        if (fs_rl(OTG_FUNC_YINT_STAT) & ep_num) {
                fs_wl_clr(mxc_pcd_instance->TAG, OTG_FUNC_YINT_STAT, ep_num);
                fs_wl_clr(mxc_pcd_instance->TAG, OTG_FUNC_YFILL_STAT, ep_num);
        }
}

/* ********************************************************************************************* */
/*!
 * mxc_pcd_setup_ep() - setup endpoint
 * @param pcd - pcd instance
 * @param epn - endpoint number
 * @param endpoint - endpoint instance
 */
static void inline
mxc_pcd_setup_ep (struct pcd_instance *pcd, unsigned int epn, struct usbd_endpoint_instance *endpoint)
{
        u32 ep_num;
        int dirr,epnn;

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG2(mxc_pcd_instance->TAG, "epn[%d] START EPN: config %02x", epn, endpoint->bEndpointAddress[0]);

        /* check if not endpoint zero and we have an assigned address */
        RETURN_UNLESS(epn && endpoint->bEndpointAddress[0]);

        dirr=endpoint->bEndpointAddress[0] & USB_DIR_IN;
        epnn=endpoint->bEndpointAddress[0] & 0x7f;
        ep_num= ep_num_dir(epnn, dirr);

        /* Start from DATA0 */
        mxc_reset_toggle_ep(ep_num);

        mxc_ep_config_nodma(endpoint->bEndpointAddress[0] & 0x7f, endpoint->bEndpointAddress[0] & USB_DIR_IN, 0,
                        endpoint->wMaxPacketSize[0], endpoint->bmAttributes[0] & 0x3, 0, 0, 0);


}

/* ********************************************************************************************* */
/*!
 * mxc_pcd_start_endpoint_out() - start receive
 * @param pcd - pcd instance
 * @param endpoint - endpoint number
 */
static void
mxc_pcd_start_endpoint_out(struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *rcv_urb = pcd_rcv_next_irq(endpoint);
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        u32 ep_num = ep_num_dir(epn, USB_DIR_OUT);
        u32 dma_num = dma_num_dir(epn, USB_DIR_OUT);
        UNLESS (rcv_urb) {
                UNLESS (epn) mxc_pcd_start_ep0_setup(endpoint);  // if EP0 then restart for receive setup
                return;
        }


        #if defined(SHORT_DMA_OUT) || defined(NO_DMA_OUT)
        endpoint->last = MIN(rcv_urb->buffer_length, 2 * endpoint->wMaxPacketSize[0]);
        #else /* defined(SHORT_DMA_OUT) || defined(NO_DMA_OUT) */
        endpoint->last = rcv_urb->buffer_length;
        #endif /* defined(SHORT_DMA_OUT) || defined(NO_DMA_OUT) */

        if (TRACE_VERBOSE)
                TRACE_MSG7(mxc_pcd_instance->TAG,
                                "epn[%02x] EP CONFIG DMA: num: %02x buffer: %p length: %d dir: %d rcv_urb: %x last: %d",
                                epn, ep_num, rcv_urb->buffer, rcv_urb->buffer_length,
                                USB_DIR_OUT, rcv_urb, endpoint->last);

        mxc_ep_config(epn, USB_DIR_OUT, ep_num, endpoint->wMaxPacketSize[0],
                        endpoint->wMaxPacketSize[0], endpoint->bmAttributes[0] & 0x3, endpoint->last, 0);

        #if !defined(NO_DMA_OUT)
        CACHE_SYNC_RCV (rcv_urb->buffer + rcv_urb->actual_length, endpoint->last);
        fs_wl(OTG_DMA_EPN_MSA(dma_num), virt_to_phys(rcv_urb->buffer + rcv_urb->actual_length));
        fs_wl_set(mxc_pcd_instance->TAG, OTG_DMA_EP_EN, ep_num);
        #endif /* NO_DMA_OUT */
        mxc_set_ep_rdy(ep_num);
        if (TRACE_VERY_VERBOSE)
                TRACE_MSG2(mxc_pcd_instance->TAG, "epn[%02x] EP_RDY: %08x", epn, fs_rl(OTG_FUNC_EP_RDY));
}

/*!
 * mxc_pcd_stop_out() - process interrupt for received buffer
 * @param pcd - pcd instance
 * @param epn - endpoint number
 * @param endpoint - endpoint instance
 */
static void
mxc_pcd_stop_out (struct pcd_instance *pcd, int epn, struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *rcv_urb = pcd_rcv_next_irq(endpoint);
        u32 ep3 = fs_rl(ep_word(epn, USB_DIR_OUT, 3)) & 0x1fffff;      // XXX fix mask
        //u32 ep_num = ep_num_out(epn);
        #if defined(NO_DMA_OUT)
        int wMaxPacketSize = endpoint->wMaxPacketSize[0];
        #endif /* defined(NO_DMA_OUT) */
        int last;
        int xfer;

        mxc_stop_ep(epn, USB_DIR_OUT);

        RETURN_UNLESS (rcv_urb); // XXX should reset fill bits...
        last = endpoint->last;

        xfer = last - ep3;

        if (TRACE_VERBOSE)
                TRACE_MSG6(mxc_pcd_instance->TAG, "epn{%02x] actual: %d rcv_urb: %x last: %d ep3: %04x xfer: %d",
                                epn, rcv_urb->actual_length, rcv_urb, last, ep3, xfer);

        #if defined(NO_DMA_OUT)
        if (TRACE_VERBOSE)
                TRACE_MSG7(mxc_pcd_instance->TAG, "--> ep_rdy: %08x dma_ep: %08x iint: %08x int: %08x %08x fill: %08x %08x",
                                fs_rl(OTG_FUNC_EP_RDY), fs_rl(OTG_DMA_EP_CH_CLR), fs_rl(OTG_FUNC_IINT),
                                fs_rl(OTG_FUNC_XINT_STAT), fs_rl(OTG_FUNC_YINT_STAT),
                                fs_rl(OTG_FUNC_XFILL_STAT), fs_rl(OTG_FUNC_YFILL_STAT));

        if (xfer /*&& (fs_rl(OTG_FUNC_XFILL_STAT) &  ep_num)*/) {

                if (TRACE_VERBOSE)
                        TRACE_MSG3(mxc_pcd_instance->TAG, "DATA_X: dst: %x src: %x len: %d",
                                        rcv_urb->buffer + rcv_urb->actual_length,
                                        (void *)data_x_address(epn, USB_DIR_OUT), wMaxPacketSize);

                memcpy(rcv_urb->buffer + rcv_urb->actual_length, (void *)data_x_address(epn, USB_DIR_OUT), wMaxPacketSize);

                //pcd_rcv_complete_irq (endpoint, MIN(xfer, wMaxPacketSize), 0);
                //fs_wl(OTG_FUNC_XFILL_STAT, ep_num);     // XXX this should be necessary
        }
        if ((xfer > wMaxPacketSize) /*&& (fs_rl(OTG_FUNC_YFILL_STAT) & ep_num)*/) {

                if (TRACE_VERBOSE)
                        TRACE_MSG3(mxc_pcd_instance->TAG, "DATA_Y: dst: %x src: %x len: %d",
                                        rcv_urb->buffer + rcv_urb->actual_length + wMaxPacketSize,
                                        (void *)data_y_address(epn, USB_DIR_OUT), wMaxPacketSize);

                memcpy(rcv_urb->buffer + rcv_urb->actual_length + wMaxPacketSize,
                                (void *)data_y_address(epn, USB_DIR_OUT), wMaxPacketSize);

                //pcd_rcv_complete_irq (endpoint, MIN(xfer - wMaxPacketSize, wMaxPacketSize), 0);
                //fs_wl(OTG_FUNC_YFILL_STAT, ep_num);     // XXX this should be necessary
        }
        if (TRACE_VERBOSE)
                TRACE_MSG7(mxc_pcd_instance->TAG, "<-- ep_rdy: %08x dma_ep: %08x iint: %08x int: %08x %08x fill: %08x %08x",
                                fs_rl(OTG_FUNC_EP_RDY), fs_rl(OTG_DMA_EP_CH_CLR), fs_rl(OTG_FUNC_IINT),
                                fs_rl(OTG_FUNC_XINT_STAT), fs_rl(OTG_FUNC_YINT_STAT),
                                fs_rl(OTG_FUNC_XFILL_STAT), fs_rl(OTG_FUNC_YFILL_STAT));
        #endif /* NO_DMA_OUT */

        //pcd_rcv_finished_irq (endpoint, xfer, 0);
        pcd_rcv_complete_irq (endpoint, xfer, 0);

        if (!epn) mxc_sendzlp(epn, endpoint, endpoint->wMaxPacketSize[0]);        // ZLP
        else mxc_pcd_start_endpoint_out(pcd, endpoint);                        // restart
}
/*!
 * mxc_pcd_cancel_out_irq() - cancel OUT urb
 * @param pcd
 * @param urb
 */
static void
mxc_pcd_cancel_out_irq (struct pcd_instance *pcd,struct usbd_urb *urb)
{
        struct usbd_endpoint_instance *endpoint = urb->endpoint;
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        if (TRACE_VERBOSE)
                TRACE_MSG3(mxc_pcd_instance->TAG, "urb: %x endpoint: %x epn: %d", urb, endpoint, epn);
        mxc_stop_ep(epn, USB_DIR_OUT);
}
/* ********************************************************************************************* */
/*!
 * mxc_pcd_start_endpoint_in() - start transmit
 * Get next tx_urb (completing old one if there is one) and start sending it.
 * @param pcd - pcd instance
 * @param endpoint - endpoint instance
 */
static void
mxc_pcd_start_endpoint_in(struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *tx_urb;
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        u32 ep_num = ep_num_dir(epn, USB_DIR_IN);
        u32 dma_num = dma_num_dir(epn, USB_DIR_IN);
        int wMaxPacketSize = endpoint->wMaxPacketSize[0];
        //int i;
        int remaining;
#ifdef  MXC_TRACK_TIMER
        if (timer_pending(&(endpoint->pcd_hr_timer))){
                TRACE_MSG0(mxc_pcd_instance->TAG, "Trying to reconfiure EP before previous finished");
                printk(KERN_INFO"%s:Trying to reconfiure EP before previous finished\n", __FUNCTION__);
        }
#endif

        //printk(KERN_INFO"%s:\n", __FUNCTION__);

        if (TRACE_VERBOSE)
                TRACE_MSG3(mxc_pcd_instance->TAG, "urb: %x sent: %d last: %d", endpoint->tx_urb, endpoint->sent, endpoint->last);
        tx_urb = pcd_tx_complete_irq(endpoint, 0);

        UNLESS (tx_urb) {
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "START IN: %02x finished", endpoint->bEndpointAddress[0]);
                UNLESS (epn) {                                  // if not EP0 we are done
                        if (mxc_new_address != mxc_usb_address) {
                                mxc_usb_address = mxc_new_address;
                                if (TRACE_VERY_VERBOSE)
                                        TRACE_MSG1(mxc_pcd_instance->TAG, "START IN: SETTING ADDRESS %x", mxc_usb_address);
                                fs_wl(OTG_FUNC_DEV_ADDR, mxc_usb_address);
                        }
                        mxc_pcd_start_ep0_setup(endpoint);          // if EP0 then restart for receive setup
                }
                return;
        }

        if (pcd_tx_sendzlp(endpoint)) {                                                  // check if we need to send ZLP
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "START IN: ZLP tx_urb: %x", tx_urb);
                mxc_sendzlp(epn, endpoint, wMaxPacketSize);        // ZLP
                return;
        }

        if (TRACE_VERY_VERBOSE)
                TRACE_MSG6(mxc_pcd_instance->TAG, "START IN: %02x tx_urb: %x actual: %d sent: %d last: %d %s", endpoint->bEndpointAddress[0], tx_urb,
                                tx_urb->actual_length, endpoint->sent,
                                endpoint->last, (tx_urb->flags & USBD_URB_SENDZLP) ? "ZLP": "");


        /* check if xfil/yfil bits are set, clear if necessary
         */
        if (epn && fs_rl(OTG_FUNC_XFILL_STAT) & ep_num) {
                fs_wl(OTG_FUNC_XFILL_STAT, ep_num);
                if (TRACE_VERY_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "CLEAR XFILL: %08x", fs_rl(OTG_FUNC_XFILL_STAT));
        }
        if (epn && fs_rl(OTG_FUNC_YFILL_STAT) & ep_num) {
                fs_wl(OTG_FUNC_YFILL_STAT, ep_num);
                if (TRACE_VERY_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "CLEAR YFILL: %08x", fs_rl(OTG_FUNC_YFILL_STAT));
        }

        /* reset ready status
         */
        // XXX mxc_reset_ep_rdy(ep_num);

        /* Three cases
         *     1. Complete transfer less than 2 buffers - non DMA
         *     2. Multiples of 2 buffers - DMA
         *     3. Remainder after sending all full sized buffers, same as 1, non DMA
         */
        remaining = tx_urb->actual_length - endpoint->sent;

        if (remaining < (2*wMaxPacketSize)) {
                endpoint->last = remaining;
                if (TRACE_VERY_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "LAST: %d SHORT", endpoint->last);
        }
        else {
                #ifdef NO_DMA_IN
                endpoint->last = 2*wMaxPacketSize;
                #else /* NO_DMA_IN */
                endpoint->last = (remaining / (2*wMaxPacketSize)) * (2*wMaxPacketSize);
                #endif /* NO_DMA_IN */
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "LAST: %d EVEN", endpoint->last);
        }

        #ifdef NO_DMA_IN
        if (endpoint->last <= (2*wMaxPacketSize)) {

                if (TRACE_VERBOSE) {
                        TRACE_MSG2(mxc_pcd_instance->TAG, "XCNT: %d memcpy: %d", endpoint->last,
                                        MAX(32, (endpoint->last + 3) & ~3));
                        TRACE_NSEND(mxc_pcd_instance->TAG, endpoint->last, tx_urb->buffer + endpoint->sent);
                }

                /* N.b. - copies must be 32bit */
                memcpy((void *)data_x_address(epn, USB_DIR_IN),
                                tx_urb->buffer + endpoint->sent,
                                MAX(32, (endpoint->last + 3) & ~3));
                fs_wl(OTG_FUNC_XFILL_STAT, ep_num);
                fs_wl(OTG_FUNC_YFILL_STAT, ep_num);
                mxc_ep_config(epn, USB_DIR_IN, ep_num, wMaxPacketSize, wMaxPacketSize, USB_DIR_IN, endpoint->last, 0);

                if (TRACE_VERBOSE)
                        TRACE_MSG6(mxc_pcd_instance->TAG,
                                        "EP CONFIG NODMA: epn: %02x num: %02x ttlbtecnt: %d dir: %d XFILL: %08x YFILL: %08x",
                                        epn, ep_num, endpoint->last, USB_DIR_IN,
                                        fs_rl(OTG_FUNC_XFILL_STAT), fs_rl(OTG_FUNC_YFILL_STAT));
        }
        else {
                #endif
                /* configure endpoint first to setup buffersize correctly, we set it to
                 * packetsize or the actual transfer size IFF less than twice packetsize.
                 * Note that this requires sequential x/y buffer allocation.
                 */
                CACHE_SYNC_TX (tx_urb->buffer + endpoint->sent, endpoint->last);

                mxc_ep_config(epn, USB_DIR_IN, ep_num, wMaxPacketSize,
                                (endpoint->last > wMaxPacketSize) ?  /* XXX 2*wMaxPacketSize ??? */
                                wMaxPacketSize : endpoint->last,
                                endpoint->bmAttributes[0] & 0x3,
                                endpoint->last, 0);

                fs_wl(OTG_DMA_EPN_MSA(dma_num), virt_to_phys(tx_urb->buffer + endpoint->sent));
                fs_wl_set(mxc_pcd_instance->TAG, OTG_DMA_EP_EN, ep_num);

                if (TRACE_VERBOSE)
                        TRACE_MSG5(mxc_pcd_instance->TAG, "EP CONFIG DMA: buf: %08x epn: %02x num: %02x ttlbtecnt: %d dir: %d",
                                        (void *)virt_to_phys(tx_urb->buffer), epn, ep_num, endpoint->last, USB_DIR_IN);
                #ifdef NO_DMA_IN
        }
        #endif
#ifdef MXC_TRACK_TIMER
        // setup a tracking timer
        init_timer (&(endpoint->pcd_hr_timer));
        endpoint->pcd_hr_timer.expires = jiffies + 1000;
        endpoint->pcd_hr_timer.function = mxc_pcd_hrt_callback;
        endpoint->pcd_hr_timer.data = (unsigned long) endpoint;
        endpoint->pcd_hr_timer.arch_cycle_expires = 0;
        add_timer(&endpoint->pcd_hr_timer);
#endif
        mxc_sof_active(ep_num);
        mxc_set_ep_rdy(ep_num);
}

/*!
 * mxc_pcd_stop_in() - process interrupt for transmitted buffer
 * @param pcd - pcd instance
 * @param epn -endpoint number
 * @param endpoint - endpoint instance
 */
static void inline
mxc_pcd_stop_in (struct pcd_instance *pcd, int epn, struct usbd_endpoint_instance *endpoint)
{
        u32 ep_num = ep_num_dir(epn, USB_DIR_IN);
        //TRACE_MSG1(mxc_pcd_instance->TAG, "%02x", epn);
        mxc_stop_ep(epn, USB_DIR_IN);
        mxc_sof_cancel(ep_num);
#ifdef MXC_TRACK_TIMER
        del_timer(&endpoint->pcd_hr_timer);
#endif

        mxc_pcd_start_endpoint_in(pcd, endpoint);
}
/*!
 * mxc_pcd_cancel_in_irq() - cancel IN urb
 */
static void inline
mxc_pcd_cancel_in_irq (struct pcd_instance *pcd,struct usbd_urb *urb)
{
        struct usbd_endpoint_instance *endpoint = urb->endpoint;
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        if (TRACE_VERBOSE)
                TRACE_MSG3(mxc_pcd_instance->TAG, "urb: %x endpoint: %x epn: %d", urb, endpoint, epn);
        mxc_stop_ep(epn, USB_DIR_IN);
}

/* ********************************************************************************************* */
/*!
 * mxc_pcd_stop_ep0_setup() - setup endpoint zero
 * @param pcd - pcd instance
 * @param endpoint - endpoint instance
 */
static void
mxc_pcd_stop_ep0_setup (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        static struct usbd_device_request request;
        //u8 *cp = (u8 *) &request;
        u32 ep0 = fs_rl(ep_word(0, USB_DIR_OUT, 0));
        u32 ep3 = fs_rl(ep_word(0, USB_DIR_OUT, 3));

        //fs_wl_set(mxc_pcd_instance->TAG,OTG_FUNC_XYINT_STEN_CLR, ep_num_both(0));
        fs_wl_set(mxc_pcd_instance->TAG,OTG_FUNC_IINT_CLR, ep_num_both(0));

        UNLESS (ep0 & EP0_SETUP) {                                      // check if SETUP flag set
                mxc_pcd_stop_out(pcd, 0, endpoint);
                return;
        }
        if (TRACE_VERBOSE)
                TRACE_MSG1(mxc_pcd_instance->TAG, "EP0 - SETUP: size: %x", ep3 & 0xffff);
        pcd_tx_cancelled_irq(endpoint);
        pcd_rcv_cancelled_irq(endpoint);

        /*
         * Ensure that toggle is set so that any data returned starts as DATA1
         * XXX It might be more appropriate to do this in start_endpoint_in().
         */
        mxc_set_toggle_ep(0x2);
        fs_memcpy((u8 *)&request, (u8 *)data_x_address(0, USB_DIR_OUT), 8);
        fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_XINT_STAT, ep_num_out(0));
        fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_XFILL_STAT, ep_num_out(0));

        if (pcd_recv_setup_irq(pcd, &request)) {                              // process setup packet
                if (TRACE_VERBOSE)
                        TRACE_MSG0(mxc_pcd_instance->TAG, "pcd_ep0: STALLING");
                //fs_rl(OTG_FUNC_XINT_STAT);
                //mxc_ep_config_nodma(0, USB_DIR_IN, 1, endpoint->wMaxPacketSize[0], EP_FORMAT_CONTROL, 0, 1, 1);
                mxc_halt_endpoint0_in(1);
                mxc_halt_endpoint0_in(0);
                return;
        }
        if (request.wLength) {
                // mxc_pcd_start_endpoint_in() will start xfer
                //mxc_pcd_start_ep0_setup(endpoint);                                  // restart endpoint
                return;
        }
        if ((request.bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_HOST2DEVICE) {
                //RETURN_IF (request.wLength);                            // mxc_pcd_start_endpoint_in() will start xfer
                mxc_sendzlp(0, endpoint, endpoint->wMaxPacketSize[0]);     // wLength zero, so send ZLP
                return;
        }
        mxc_pcd_start_ep0_setup(endpoint);                                  // restart endpoint
}
/* ********************************************************************************************* */
#ifdef MXC_ACTIVE
static u32 mxc_pcd_missed;
#endif /* MXC_ACTIVE */

static u32 mxc_pcd_missing;
static u32 mxc_pcd_fixed;

/*!
 * mxc_pcd_int_hndlr() - interrupt handle
 * @return interrupt handled status
 */
irqreturn_t mxc_pcd_int_hndlr (void)
{
        struct pcd_instance *pcd = mxc_pcd_instance;
        otg_tick_t ticks = (pcd->otg && pcd->otg->ocd_ops && pcd->otg->ocd_ops->ticks) ?  pcd->otg->ocd_ops->ticks () : 0;
        //u32 cmd_stat, sint_stat, ep_dstat, ep_stats, mask;
        u32 cmd_stat, sint_stat, ep_dstat, ep_stats;
        //u32 sint_sten = fs_rl(OTG_FUNC_SINT_STEN);

        int i;
        BOOL ep0_int = FALSE;
        BOOL epn_int = FALSE;
        BOOL other_int = FALSE;

        if (!pcd)
                return IRQ_HANDLED;


        /* Reset, clear and process any System interrupts
         */
        if ((sint_stat = fs_rl(OTG_FUNC_SINT_STAT)))
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_SINT_STAT, sint_stat);

        cmd_stat = fs_rl(OTG_FUNC_CMD_STAT);

        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");

        //if (TRACE_VERY_VERBOSE)
        TRACE_MSG6(mxc_pcd_instance->TAG, "sint: %08x cmd: %08x EP_EN: %08x EP_RDY: %08x EP_DSTAT: %08x FIXED: %d",
                        sint_stat, cmd_stat, fs_rl(OTG_FUNC_EP_EN), fs_rl(OTG_FUNC_EP_RDY),
                        fs_rl(OTG_FUNC_EP_DSTAT), mxc_pcd_fixed);


        if ((cmd_stat & COMMAND_RESETDET) || (sint_stat & SYSTEM_RESETINT)) {
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "RESETDET: %08x", sint_stat);
                //printk(KERN_INFO"%s: RESETDET\n", __FUNCTION__);
                pcd_bus_event_handler_irq (pcd->bus, DEVICE_RESET, 0);
                //mxc_func_clock_on();
                fs_wl(OTG_FUNC_DEV_ADDR, 0);
                mxc_new_address = mxc_usb_address = 0;
                fs_wl(OTG_DMA_EP_CH_CLR, 0xffffffff);

                //fs_wl(OTG_FUNC_XYINT_STEN_CLR, 0xffffffff);
                fs_wl(OTG_FUNC_IINT_CLR, 0xffffffff);
                fs_wl(OTG_FUNC_EP_DEN_CLR, 0xffffffff);  // XXX this fails,

                mxc_reset_toggles();

                //fs_wl_clr(mxc_pcd_instance->TAG, OTG_FUNC_EP_RDY, fs_rl(OTG_FUNC_EP_RDY));
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_EP_RDY_CLR, fs_rl(OTG_FUNC_EP_RDY));

                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_EP_DSTAT, fs_rl(OTG_FUNC_EP_DSTAT));

                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_XINT_STAT, fs_rl(OTG_FUNC_XINT_STAT));
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_YINT_STAT, fs_rl(OTG_FUNC_YINT_STAT));
                if (pcd->bus)
                        mxc_pcd_start_ep0_setup(pcd->bus->endpoint_array);
                other_int = TRUE;
        }
        if ((cmd_stat & COMMAND_RSMINPROG) || (sint_stat & SYSTEM_RSMFININT)) {
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "RSMINPRO: %08x", sint_stat);
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_SINT_STAT,SYSTEM_RSMFININT);
                mxc_func_clock_on();
                pcd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_ACTIVITY, 0);
                other_int = TRUE;
        }
        if ((cmd_stat & COMMAND_SUPDET) || (sint_stat & SYSTEM_SUSPDETINT)) {
                //int timeout = 0;
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "SUSPENDDET: %08x", sint_stat);
                fs_wl(OTG_FUNC_CMD_STAT, COMMAND_SUPDET);  /* tell hardware to suspend */
                fs_wl(OTG_FUNC_SINT_STAT,SYSTEM_SUSPDETINT);
                mxc_func_clock_off();
                pcd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_INACTIVE, 0);
                other_int = TRUE;
        }

        while ((ep_dstat = fs_rl(OTG_FUNC_EP_DSTAT))) {

                ep_stats = (ep_dstat | fs_rl(OTG_FUNC_XINT_STAT) | fs_rl(OTG_FUNC_YINT_STAT));

                /* clear the done status flags */
                if (TRACE_VERBOSE)
                        TRACE_MSG1(mxc_pcd_instance->TAG, "EP STATS: %08x", ep_stats);
                //printk (KERN_INFO"%s: EP STATS: %08x\n", __FUNCTION__, ep_stats);
                fs_wl_set(mxc_pcd_instance->TAG, OTG_FUNC_EP_DSTAT, ep_dstat);

                /* check ep0 first - special handling for control endpoint, need to check dstat, y/x stats */
                if (ep_stats & EP_OUT) {
                        ep0_int = TRUE;
                        mxc_pcd_stop_ep0_setup(pcd, pcd->bus->endpoint_array);
                }
                if (ep_stats & EP_IN) {
                        ep0_int = TRUE;
                        mxc_pcd_stop_in(pcd, 0, pcd->bus->endpoint_array);
                }

                /* now check all of the data endpoints, only if in dstat */
                for (i = 1, ep_dstat >>= 2; ep_dstat && (i < 16); i++, ep_dstat >>= 2) {
                        if (ep_dstat & EP_OUT) {
                                epn_int = TRUE;
                                mxc_pcd_stop_out(pcd, i, pcd->bus->endpoint_array + (2 * i));
                        }
                        if (ep_dstat & EP_IN) {
                                //epn_int = TRUE;
                                mxc_pcd_stop_in(pcd, i, pcd->bus->endpoint_array + (2 * i) + 1);
                        }
                }
        }

        if (epn_int)
                TRACE_ELAPSED(mxc_pcd_instance->TAG, "EPN 0. INT", ticks, 0);

        return IRQ_HANDLED;
}

/* ********************************************************************************************* */
/*!
 * mxc_pcd_set_address() - set the USB address for this device
 * @param pcd
 * @param address
 */
static void inline
mxc_pcd_set_address (struct pcd_instance *pcd, u8 address)
{
        mxc_usb_address = 0;
        mxc_new_address = address;      /* this will be used in the interrupt handler */
}

/* ********************************************************************************************* */
/*!
 * mxc_pcd_en_func() - enable
 * This is called to enable / disable the PCD and USBD stack.
 * @param otg - otg instance
 * @param flag - SET or RESET
 */
static void
mxc_pcd_en_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;
        u32 hwmode = fs_rl(OTG_CORE_HWMODE);
        int epn;

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->pcd->TAG, "PCD_EN: SET" );
                otg_pthread_mutex_lock(&pcd->mutex);

                fs_andl(OTG_CORE_HWMODE, ~MODULE_CRECFG_HOST);           // set to FUNCTION hnp

                TRACE_MSG1(otg->pcd->TAG, "PCD_EN: clr hwmode: %08x", fs_rl(OTG_CORE_HWMODE));

                fs_orl(OTG_CORE_HWMODE, MODULE_CRECFG_FUNC);           // set to software hnp

                TRACE_MSG2(otg->pcd->TAG, "PCD_EN: set hwmode: %08x want %08x",
                                fs_rl(OTG_CORE_HWMODE),
                                MODULE_CRECFG_FUNC);           // set to software hnp

                mxc_main_clock_on();
                mxc_func_clock_on();

                if (pcd->bus){
                        mxc_pcd_start_ep0_setup(pcd->bus->endpoint_array);
                        for (epn = 1; epn < bus->endpoints; epn++)
                                mxc_pcd_setup_ep (pcd, epn, bus->endpoint_array + epn);
                }
                /* C.f. L3 12.1 - enable Slave, Band Gap enable, and Comparator enable.
                 * C.f. SCM-11 version of the USBOTG specification, ABBusReq enables the charge pump
                 */
                fs_andl(OTG_CORE_HNP_CSTAT, ~(MODULE_MASTER | MODULE_SLAVE |
                                        MODULE_CMPEN | MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ));

                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_SLAVE | MODULE_CMPEN | MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ);
                //fs_rl(OTG_CORE_HNP_CSTAT);

                fs_orl(OTG_FUNC_SINT_STEN, ( SYSTEM_DONEREGINT_EN |
                                        SYSTEM_SUSPDETINT_EN | SYSTEM_RSMFININT_EN | SYSTEM_RESETINT_EN));

                fs_orl(OTG_CORE_CINT_STEN, MODULE_FCINT_EN | MODULE_ASFCINT_EN);
                otg->pcd->active = TRUE;
                otg_pthread_mutex_lock(&pcd->mutex);

                break;

        case RESET:
                TRACE_MSG1(otg->pcd->TAG, "PCD_EN: RESET hwmode: %08x", hwmode);

                otg_pthread_mutex_lock(&pcd->mutex);
                otg->pcd->active = FALSE;
                mxc_func_clock_on();
                fs_andl(OTG_FUNC_SINT_STEN, 0);
                fs_andl(OTG_CORE_CINT_STEN, ~(MODULE_FCINT_EN | MODULE_ASFCINT_EN));

                mxc_func_clock_off();

                /* reset the function core
                 */
                fs_wl_set(otg->pcd->TAG, OTG_CORE_RST_CTRL, MODULE_RSTFSIE | MODULE_RSTFC);
                while (fs_rl(OTG_CORE_RST_CTRL));
                fs_orl(OTG_CORE_HWMODE,MODULE_CRECFG_SHNP);             // set to software hnp
                otg_pthread_mutex_lock(&pcd->mutex);

                pcd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
                pcd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);

                break;
        }
        TRACE_MSG4(otg->pcd->TAG, "HWMODE: %08x CINT_STEN: %08x SINT_STEN: %08x HNP_CSTAT: %08x",
                        fs_rl(OTG_CORE_HWMODE), fs_rl(OTG_CORE_CINT_STEN),
                        fs_rl(OTG_FUNC_SINT_STEN), fs_rl(OTG_CORE_HNP_CSTAT) );
}

#if CONFIG_OTG_REMOTE_WAKEUP
/*!
 * mxc_remote_wakeup() - perform remote wakeup.
 * Initiate a remote wakeup to the host.
 * @param otg - otg instance
 * @param flag - SET or RESET
 */
static void
mxc_remote_wakeup(struct otg_instance *otg, u8 flag)
{
        switch(flag) {
        case PULSE:
                TRACE_MSG1(otg->pcd->TAG, "MX2_REMOTE_WAKEUP: OTG_FUNC_CMD_STAT: %08x ", fs_rl(OTG_FUNC_CMD_STAT) );
                otg_pthread_mutex_lock(&otg->pcd->mutex);
                mxc_func_clock_on();
                fs_wl(OTG_FUNC_CMD_STAT, COMMAND_RSMINPROG);
                otg_pthread_mutex_unlock(&otg->pcd->mutex);
                TRACE_MSG1(otg->pcd->TAG, "MX2_REMOTE_WAKEUP: OTG_FUNC_CMD_STAT: %08x RESUMED", fs_rl(OTG_FUNC_CMD_STAT) );
        default:
                break;
        }
}
#endif /* CONFIG_OTG_REMOTE_WAKEUP */

/* ********************************************************************************************* */
/*!
 * mxc_pcd_start() - start the UDC
 * @param pcd
 */
static void
mxc_pcd_start (struct pcd_instance *pcd)
{
        //fs_rl(OTG_FUNC_SINT_STEN);
        fs_wl(OTG_DMA_MISC_CTRL, OTG_DMA_MISC_ARBMODE);         /* round-robin mode */
}

/*!
 * mxc_pcd_stop() - stop the UDC
 * @param pcd
 */
static void
mxc_pcd_stop (struct pcd_instance *pcd)
{
        TRACE_MSG0(pcd->TAG, "UDC STOP");
}

/*!
 * mxc_pcd_set_endpoints() - setup the physical endpoints for the endpoint map
 * @param pcd
 * @param endpointsRequested
 * @param endpoint_map_array
 */
static int
mxc_pcd_set_endpoints (struct pcd_instance *pcd, int endpointsRequested, struct usbd_endpoint_map *endpoint_map_array)
{
        TRACE_MSG0(pcd->TAG, "Enable system control interrupts");
        #ifdef MX2_OTG_XCVR_DEVAD
        fs_wl(OTG_SYS_CTRL, SYS_CTRL_I2C_WU_INT_EN | SYS_CTRL_OTG_WU_INT_EN | SYS_CTRL_HOST_WU_INT_EN |
                        SYS_CTRL_FNT_WU_INT_EN | 0x1);
        #else /* MX2_OTG_XCVR_DEVAD */
        fs_wl(OTG_SYS_CTRL, SYS_CTRL_OTG_WU_INT_EN | SYS_CTRL_HOST_WU_INT_EN | SYS_CTRL_FNT_WU_INT_EN | 0x1);
        #endif /* MX2_OTG_XCVR_DEVAD */
        return 0;
}

/*!
 * mxc_pcd_framenum() - get current framenum
 */
static u16
mxc_pcd_framenum (struct otg_instance *otg)
{
        return fs_rl(OTG_FUNC_FRM_NUM);
}

/*
 * mxc_endpoint_halted() - is endpoint halted
 */
static int
mxc_endpoint_halted (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        int dir = (endpoint->bEndpointAddress[0] & 0x80) ? 1 : 0;

        TRACE_MSG2(pcd->TAG, "epn: %02x dir: %x", epn, dir);
        return fs_rl(ep_word(epn, dir, 0))  & (1 << 31) ? TRUE : FALSE;
}


/* mxc_halt_endpoint - halt endpoint
 */
int mxc_halt_endpoint(struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint, int flag)
{
        int epn = endpoint->bEndpointAddress[0] & 0x7f;
        int dir = (endpoint->bEndpointAddress[0] & 0x80) ? 1 : 0;
        u32 ep_num = ep_num_dir(epn, dir);
        u32 temp=~(1<<31);
        //u32 toggle = fs_rl(OTG_FUNC_EP_TOGGLE);
        u32 ep0_current, ep0_new;

        /*
         * Ensure that toggle is reset so that any data returned starts as DATA0
         * XXX It might be more appropriate to do this in start_endpoint_in().
         */
        mxc_reset_toggle_ep(ep_num);
        // XXX fs_wl(OTG_DMA_EP_CH_CLR, ep_num); // XXX causes problems

        ep0_current = fs_rl(ep_word(epn, dir, 0));
        ep0_new =  ((flag ? 1 : 0) << 31) | (ep0_current & temp);

        UNLESS (ep0_current == ep0_new) fs_wl(ep_word(epn, dir, 0), ep0_new);
        return 0;
}

/* mxc_pcd_disable -
 */
void mxc_pcd_disable(struct pcd_instance *pcd)
{
        TRACE_MSG0(pcd->TAG, "--");
}

/* mxc_pcd_disable_ep -
 */
void mxc_pcd_disable_ep(struct pcd_instance *pcd, unsigned int ep, struct usbd_endpoint_instance * ep_instance)
{
        TRACE_MSG0(pcd->TAG, "--");
}

/* ********************************************************************************************* */

#if defined(CONFIG_OTG_NOC99)
struct usbd_pcd_ops usbd_pcd_ops;

extern pcd_ops pcd_ops;

#if 0
#if defined(CONFIG_OTG_LNX) && defined(LINUX26)
int mxc_pcd_mod_init_l26(void);
void mxc_pcd_mod_exit_l26(void);
struct pcd_ops pcd_ops = { mxc_pcd_mod_init_l26, mxc_pcd_mod_exit_l26, };
#else /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */

int mxc_pcd_mod_init(struct otg_instance *otg);
void mxc_pcd_mod_exit(struct otg_instance *otg);
struct pcd_ops pcd_ops = { mxc_pcd_mod_init, mxc_pcd_mod_exit, };
#endif /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */
#endif

/*!
 * mxc_pcd_global_init() - initialize globals
 */
void
mxc_pcd_global_init(void)
{
        ZERO(usbd_pcd_ops);
        usbd_pcd_ops.bmAttributes =
                USB_ABMTTRIBUTE_RESERVED |
                #ifdef CONFIG_OTG_SELF_POWERED
                USB_BMATTRIBUTE_SELF_POWERED |
                #endif /* CONFIG_OTG_SELF_POWERED */
                #ifdef CONFIG_OTG_REMOTE_WAKEUP
                USB_BMATTRIBUTE_REMOTE_WAKEUP |
                #endif /* CONFIG_OTG_REMOTE_WAKEUP */
                USB_OTG_HNP_SUPPORTED | USB_OTG_SRP_SUPPORTED;
        #ifndef CONFIG_OTG_SELF_POWERED
        usbd_pcd_ops.bMaxPower = CONFIG_OTG_BMAXPOWER;
        #endif /* CONFIG_OTG_SELF_POWERED */
        usbd_pcd_ops.max_endpoints =  UDC_MAX_ENDPOINTS;
        usbd_pcd_ops.ep0_packetsize =  EP0_PACKETSIZE;
        //usbd_pcd_ops.capabilities = REMOTE_WAKEUP_SUPPORTED;
        usbd_pcd_ops.name =  UDC_NAME;
        usbd_pcd_ops.start = mxc_pcd_start;
        usbd_pcd_ops.stop = mxc_pcd_stop;
        usbd_pcd_ops.disable = mxc_pcd_disable;
        usbd_pcd_ops.disable_ep = mxc_pcd_disable_ep;
        usbd_pcd_ops.start_endpoint_in = mxc_pcd_start_endpoint_in;
        usbd_pcd_ops.start_endpoint_out = mxc_pcd_start_endpoint_out;
        usbd_pcd_ops.request_endpoints = pcd_request_endpoints;
        usbd_pcd_ops.set_endpoints = mxc_pcd_set_endpoints;
        usbd_pcd_ops.set_address = mxc_pcd_set_address;
        usbd_pcd_ops.setup_ep = mxc_pcd_setup_ep;
        usbd_pcd_ops.halt_endpoint = mxc_halt_endpoint;
        usbd_pcd_ops.endpoint_halted = mxc_endpoint_halted;

        usbd_pcd_ops.cancel_in_irq = mxc_pcd_cancel_in_irq;
        usbd_pcd_ops.cancel_out_irq = mxc_pcd_cancel_out_irq;
        usbd_pcd_ops.max_endpoints = 8;

}
/* ********************************************************************************************* */
#else /* defined(CONFIG_OTG_NOC99) */
struct usbd_pcd_ops usbd_pcd_ops;
struct
usbd_pcd_ops usbd_pcd_ops = {
        .bmAttributes =
                USB_BMATTRIBUTE_RESERVED
                #ifdef CONFIG_OTG_SELF_POWERED
                | USB_BMATTRIBUTE_SELF_POWERED
                #endif /* CONFIG_OTG_SELF_POWERED */
                #ifdef CONFIG_OTG_REMOTE_WAKEUP
                | USB_BMATTRIBUTE_REMOTE_WAKEUP
                #endif /* CONFIG_OTG_REMOTE_WAKEUP */
                | USB_OTG_HNP_SUPPORTED | USB_OTG_SRP_SUPPORTED,
        #ifndef CONFIG_OTG_SELF_POWERED
        .bMaxPower = CONFIG_OTG_BMAXPOWER;
        #endif /* CONFIG_OTG_SELF_POWERED */
        .max_endpoints =  UDC_MAX_ENDPOINTS,
                .high_speed_capable = FALSE,
                .ep0_packetsize =  EP0_PACKETSIZE,
                //.capabilities = REMOTE_WAKEUP_SUPPORTED,
                .name =  UDC_NAME,
                .start = mxc_pcd_start,
                .stop = mxc_pcd_stop,
                .disable = mxc_pcd_disable,
                .disable_ep = mxc_pcd_disable_ep,
                .start_endpoint_in = mxc_pcd_start_endpoint_in,
                .start_endpoint_out = mxc_pcd_start_endpoint_out,
                .request_endpoints = pcd_request_endpoints,
                .set_endpoints = mxc_pcd_set_endpoints,
                .set_address = mxc_pcd_set_address,
                .setup_ep = mxc_pcd_setup_ep,
                .halt_endpoint = mxc_halt_endpoint,
                .endpoint_halted = mxc_endpoint_halted,

                .cancel_in_irq = mxc_pcd_cancel_in_irq,
                .cancel_out_irq = mxc_pcd_cancel_out_irq,

                .max_endpoints = 8,
};

#if 0
#if defined(CONFIG_OTG_LNX) && defined(LINUX26)
int mxc_pcd_mod_init_l26(struct otg_instance *otg);
void mxc_pcd_mod_exit_l26(struct otg_instance *otg);
struct pcd_ops pcd_ops = {
        .mod_init = mxc_pcd_mod_init_l26,               // called for module init
#ifdef MODULE
        .mod_exit = mxc_pcd_mod_exit_l26,               // called for module exit
#endif
        .pcd_en_func = mxc_pcd_en_func,
        .pcd_init_func = pcd_init_func,
        #ifdef CONFIG_OTG_REMOTE_WAKEUP
        .remote_wakeup_func = mxc_remote_wakeup,
        #endif /* CONFIG_OTG_REMOTE_WAKEUP */
        .framenum = mxc_pcd_framenum,
};

#else /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */
int mxc_pcd_mod_init(struct otg_instance *otg);
void mxc_pcd_mod_exit(struct otg_instance *otg);
struct pcd_ops pcd_ops = {
        .mod_init = mxc_pcd_mod_init,               // called for module init
        .mod_exit = mxc_pcd_mod_exit,               // called for module exit
        .pcd_en_func = mxc_pcd_en_func,
        .pcd_init_func = pcd_init_func,
        #ifdef CONFIG_OTG_REMOTE_WAKEUP
        .remote_wakeup_func = mxc_remote_wakeup,
        #endif /* CONFIG_OTG_REMOTE_WAKEUP */
        .framenum = mxc_pcd_framenum,
};
#endif /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */
#endif


#endif /* defined(CONFIG_OTG_NOC99) */

int mxc_pcd_mod_init_l26(struct otg_instance *otg);
void mxc_pcd_mod_exit_l26(struct otg_instance *otg);

/*!
 * mxc_pcd_ops_init() - initialize globals
 */
void
mxc_pcd_ops_init(void)
{
        ZERO(pcd_ops);
        pcd_ops.pcd_en_func = mxc_pcd_en_func;
        pcd_ops.pcd_init_func = pcd_init_func;
        #ifdef CONFIG_OTG_REMOTE_WAKEUP
        pcd_ops.remote_wakeup_func = mxc_remote_wakeup;
        #endif /* CONFIG_OTG_REMOTE_WAKEUP */
        pcd_ops.framenum = mxc_pcd_framenum;
        #if defined(CONFIG_OTG_LNX) && defined(LINUX26)
        pcd_ops.mod_init = mxc_pcd_mod_init_l26;               // called for module init
        #ifdef MODULE
        pcd_ops.mod_exit = mxc_pcd_mod_exit_l26;               // called for module exit
        #endif

        #else /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */
        pcd_ops.mod_init = mxc_pcd_mod_init;               // called for module init
        #ifdef MODULE
        pcd_ops.mod_exit = mxc_pcd_mod_exit;               // called for module exit
        #endif
        #endif /* defined(CONFIG_OTG_LNX) && defined(LINUX26) */
}

/* ********************************************************************************************* */

/*! default initialization
 */

int pcd_mod_init (struct otg_instance *otg);
void pcd_mod_exit (struct otg_instance *otg);

int mxc_pcd_mod_init(struct otg_instance *otg)
{
        u32 hwmode;

        mxc_pcd_instance = otg->pcd;
        TRACE_MSG0(otg->pcd->TAG, "--");
        pcd_mod_init(otg);

        mxc_main_clock_on();
        #if 0
        fs_clear_words((volatile u32 *)IO_ADDRESS(OTG_EP_BASE), (NUM_ETDS*16/4));
        fs_clear_words((volatile u32 *)IO_ADDRESS(OTG_EP_BASE), 0x200/4);
        fs_clear_words((volatile u32 *)IO_ADDRESS(OTG_DATA_BASE), 0x400/4);
        #endif


        hwmode = fs_rl(OTG_CORE_HWMODE);
        mxc_funcrev = hwmode >> 24;

        TRACE_MSG1(otg->pcd->TAG, "funcrev: %04x", mxc_funcrev);

        mxc_main_clock_off();

        return 0;
}

void mxc_pcd_mod_exit(struct otg_instance *otg)
{
        int i;


        TRACE_MSG0(otg->pcd->TAG, "--");
        otg_pthread_mutex_lock(&otg->pcd->mutex);
        for (i = 0; i < 16; i++) {
                mxc_stop_ep(i, USB_DIR_IN);
                mxc_stop_ep(i, USB_DIR_OUT);
        }
        otg_pthread_mutex_unlock(&otg->pcd->mutex);
        pcd_mod_exit(otg);
        mxc_pcd_instance = NULL;
}

/* ********************************************************************************************* */



/*! New style linux 2.6 initialization.
 */

#if defined(LINUX26)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
static void mxc_pcd_release(struct platform_device *dev)
#else
static void mxc_pcd_release(struct device *dev)
#endif
{
        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");
}

static int __init_or_module
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
mxc_pcd_remove(struct platform_device *dev)
#else
mxc_pcd_remove(struct device *dev)
#endif
{
        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");
        //mxc_pcd_mod_exit_l24(otg);
        //TRACE_MSG0(mxc_pcd_instance->TAG, "ok");
        return 0;
}


static int __init
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
mxc_pcd_probe(struct platform_device *dev)
#else
mxc_pcd_probe(struct device *dev)
#endif
{
        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
        #else
        dev->release = mxc_pcd_release;
        #endif
        //mxc_pcd_mod_init_l24(otg);
        return 0;
}
static int
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
mxc_pcd_suspend(struct platform_device *dev, pm_message_t state)
#else
mxc_pcd_suspend(struct device *dev, u32 state, u32 phase)
#endif
{
        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");
        return 0;
}

static int
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
mxc_pcd_resume(struct platform_device *dev)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
mxc_pcd_resume(struct platform_device *dev, pm_message_t state)
#else
mxc_pcd_resume(struct device *dev, u32 phase)
#endif
{
        //TRACE_MSG0(mxc_pcd_instance->TAG, "--");
        return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
static struct platform_driver mxc_pcd_driver = {
        .driver = {
                .name = "mxc-pcd",
        },
#else
static struct device_driver mxc_pcd_driver = {

                .name =         "mxc-pcd",
                .bus =          &platform_bus_type,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */

                .probe =        mxc_pcd_probe,
                .remove =       mxc_pcd_remove,

                .suspend =      mxc_pcd_suspend,
                .resume =       mxc_pcd_resume,

};
/* } FORMATTING */

static struct platform_device mxc_pcd_platform = {
        .name       = "mxc-pcd",
        .id         = 1,
};


int mxc_pcd_mod_init_l26(struct otg_instance *otg)
{
        u32 hwmode;
        int driver_registered = 0;
        int platform_registered = 0;

        //mxc_pcd_instance = otg->pcd;
        TRACE_MSG0(otg->pcd->TAG, "--");

        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
        THROW_IF((driver_registered = platform_driver_register(&mxc_pcd_driver)), error);
        #else
        THROW_IF((driver_registered = driver_register(&mxc_pcd_driver)), error);
        #endif
        THROW_IF((platform_registered = platform_registered = platform_device_register(&mxc_pcd_platform)), error);

        mxc_pcd_mod_init(otg);

        CATCH(error) {
                if (platform_registered) platform_device_unregister(&mxc_pcd_platform);
                #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
                if (driver_registered) platform_driver_unregister(&mxc_pcd_driver);
                #else
                if (driver_registered) driver_unregister(&mxc_pcd_driver);
                #endif
        }

        return 0;
}

void mxc_pcd_mod_exit_l26(struct otg_instance *otg)
{
        TRACE_MSG0(otg->pcd->TAG, "--");
        mxc_pcd_mod_exit(otg);
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
        platform_driver_unregister(&mxc_pcd_driver);
        #else
        driver_unregister(&mxc_pcd_driver);
        #endif
        platform_device_unregister(&mxc_pcd_platform);
        //otg->pcd->TAG = 0;
        //mxc_pcd_instance = NULL;
}


#endif /* defined(LINUX26) */

/* ********************************************************************************************* */
