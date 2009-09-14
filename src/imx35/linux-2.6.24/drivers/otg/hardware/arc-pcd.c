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
 * otg/hardware/arc-pcd.c -- Freescale HS (ARC) USBOTG Peripheral Controller driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/arc/arc-pcd.c|20070918212334|07623
 *
 *      Copyright (c) 2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Shahrad Payandeh Lynne <sp@lbelcarra.com>,
 *      Stuart Lynne <sl@lbelcarra.com>,
 */
/*!
 * @file otg/hardware/arc-pcd.c
 * @brief USB Peripheral Controller Driver
 * This implements the Freescale HS (ARC) USBOTG Peripheral Controller Driver.
 *
 * @ingroup ARC
 * @ingroup PCD
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */

#include <otg/pcd-include.h>
#include <otg/otg-dev.h>
#include <linux/dma-mapping.h>
#include <asm/arch/arc_otg.h>
#include <linux/dmapool.h>
#include <linux/delay.h>
#include "arc-hardware.h"
#include "arc.h"

#define HAVE_PLATFORM_ARC_REMOTE_WAKEUP 1

#define TRACE_VERBOSE 1
#define UDC_MAX_ENDPOINTS 16
#define UDC_NAME "arc"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>

/* ep_qh_base store the base address before 2K align */
static struct ep_queue_head *ep_qh_base;
static struct arcotg_udc *udc_controller;
extern void fsl_platform_set_vbus_power(struct fsl_usb2_platform_data *pdata, int on);

/*! arc_read_setup_buffer
 */
static void arc_read_setup_buffer(struct pcd_instance *pcd, u8 * buffer_ptr)
{
        struct ep_queue_head    *qh = &udc_controller->ep_qh[0 * 2 + ARC_DIR_OUT];
        int                     timeout;

        dma_cache_maint(qh, sizeof(struct ep_queue_head), DMA_FROM_DEVICE);

        /* C.f 39.16.3.2.1 Setup Phase - Setup Packet Handling (2.3 hardware and later)
         */

        /* 1. Clear ENDPTSETUPSTAT */
        UOG_ENDPTSETUPSTAT |= (1 << 0);                 /* Clear recieve endpoint status */

        do {
                /* 2. set tripwire */
                UOG_USBCMD |= USB_CMD_SUTW;

                /* 3. read setup buffer */
                memcpy(buffer_ptr, (u8 *) qh->setup_buffer, 8);

                /* 4. check tripwire, loop if reset */
        } while (!(UOG_USBCMD & USB_CMD_SUTW));

        /* 5. reset tripwire */
        UOG_USBCMD &= ~USB_CMD_SUTW;

        // XXX This needs to be fixed, should not need to resort to timeout
        timeout = 10000000;
        while ((UOG_ENDPTSETUPSTAT & 1) && --timeout) { continue; }
        if (timeout == 0) printk(KERN_ERR "%s: TIMEOUT\n", __FUNCTION__);
}

/*! arc_read_rcv_buffer
 *
 * Recover number of bytes DMA'd to receive buffer, sync.
 */
int arc_read_rcv_buffer (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        struct arc_private_struct *privdata = endpoint->privdata;
        struct ep_td_struct     *curr_td = privdata->cur_dtd;
        struct ep_queue_head    *qh = privdata->cur_dqh;
        struct usbd_urb         *rx_urb = endpoint->rcv_urb;

        /* sync qh and td structures, note that urb-buffer was invalidated in arc_add_buffer_to_dtd() */
        dma_cache_maint(qh, sizeof(struct ep_queue_head), DMA_FROM_DEVICE);
        dma_cache_maint(curr_td, sizeof(struct ep_td_struct), DMA_FROM_DEVICE);

        if (rx_urb) {
                int length = rx_urb->buffer_length -
                        ((le32_to_cpu(curr_td->size_ioc_sts) & DTD_PACKET_SIZE) >> DTD_LENGTH_BIT_POS);

                if (TRACE_VERBOSE) TRACE_MSG4(pcd->TAG, "buffer_length: %d alloc_length: %d Len: %d (%x)" ,
                                rx_urb->buffer_length, rx_urb->alloc_length, length,
                                le32_to_cpu(curr_td->size_ioc_sts));
                return length;
        }
        TRACE_MSG1(pcd->TAG, "NO RCV URB (%x)" , le32_to_cpu(curr_td->size_ioc_sts));
        return 0;
}

/*! arc_udc_init
 */
void arc_udc_release (void)
{
        kfree(ep_qh_base);
        ep_qh_base = NULL;
        if (udc_controller) {
              if (udc_controller->ep_dtd) LKFREE(udc_controller->ep_dtd);
              LKFREE(udc_controller);
               udc_controller = NULL;
        }
}

/*! arc_udc_init
 */
static struct arcotg_udc *arc_udc_init (struct otg_dev *otg_dev)
{
        struct device           *device = otg_dev_get_drvdata(otg_dev);
        struct platform_device  *pdev = to_platform_device(device);

        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

        struct otg_instance     *otg = otg_dev->otg_instance;
        struct pcd_instance     *pcd = otg_dev->pcd_instance;
        struct arcotg_udc       *udc = NULL;
        int                     timeout;


        /* Setting up the udc structure */
        THROW_UNLESS((udc = (struct arcotg_udc *) CKMALLOC(sizeof(struct arcotg_udc))), error);

        /* Allocate queue */
        THROW_UNLESS((udc->ep_qh = (struct ep_queue_head *) KMALLOC_ALIGN(    USB_MAX_PIPES * sizeof(struct ep_queue_head),
                                        GFP_KERNEL | GFP_DMA, 2 * 1024, (void **)&ep_qh_base)), error);
        THROW_UNLESS(ep_qh_base, error);
        THROW_UNLESS((udc->ep_dtd = (struct ep_td_struct *) CKMALLOC(USB_MAX_PIPES * sizeof(struct ep_td_struct))), error);

        /* Stop, reset and wait for the UDC to reset */
        UOG_USBCMD &= ~USB_CMD_RUN_STOP;
        UOG_USBCMD |= USB_CMD_CTRL_RESET;

        timeout = 10000000;     // XXX This needs to be fixed, should not need to resort to timeout
        while ((UOG_USBCMD & USB_CMD_CTRL_RESET) && --timeout) { continue; }
        if (timeout == 0) {
                printk(KERN_DEBUG "%s: TIMEOUT\n", __FUNCTION__);
                return NULL;
        }

        /* Setup UDC mode and disable lock out mode*/
        UOG_USBMODE |= USB_MODE_CTRL_MODE_DEVICE | USB_MODE_SETUP_LOCK_OFF;

        UOG_EPLISTADDR = virt_to_phys(udc->ep_qh);
        UOG_EPLISTADDR &= USB_EP_LIST_ADDRESS_MASK;

        /* Setup transceiver type, N.B. this must be done in one assignment */
        UOG_PORTSC1 = (UOG_PORTSC1 & ~PORTSCX_PHY_TYPE_SEL) | pdata->xcvr_type;

        #if !defined(CONFIG_OTG_HIGH_SPEED)
        UOG_PORTSC1 |= (0x01000000);
        #endif
	fsl_platform_set_vbus_power(pdata, 0);

        CATCH(error) {
                if (ep_qh_base) kfree(ep_qh_base);
                if (udc) {
                        if (udc->ep_dtd) LKFREE(udc->ep_dtd);
                        LKFREE(udc);
                }
                udc = NULL;
        }
        return udc;
}

/*! arc_udc_run
 */
static int arc_udc_run (void)
{
        /* enable the interrupt sources we want */
        UOG_USBINTR |= USB_INTR_INT_EN | USB_INTR_ERR_INT_EN | USB_INTR_PTC_DETECT_EN |
                USB_INTR_RESET_EN | USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;

        /* Set to peripheral mode */
        UOG_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;

        /* set interrupt threshold to zero, run udc */
        UOG_USBCMD &= ~USB_CMD_ITC;
        UOG_USBCMD |= USB_CMD_ITC_NO_THRESHOLD | USB_CMD_RUN_STOP;
        return 0;
}

/*! arc_udc_stop
 */
void arc_udc_stop (void)
{
        /* Disable all interrupts */
        UOG_USBINTR = 0;
        UOG_USBCMD &= ~USB_CMD_RUN_STOP;
}

/*! arc_pcd_start
 */
void arc_pcd_start (struct pcd_instance *pcd)
{
        u32 id = (u32) UOG_ID;
        TRACE_MSG1(pcd->TAG, "Initialize UDC and start id: %08x", id);
        arc_udc_run ();
}

/*! arc_pcd_stop
 */
void arc_pcd_stop(struct pcd_instance *pcd)
{
        TRACE_MSG0(pcd->TAG, "Stop");
        arc_udc_stop ();
}

/*! arc_pcd_disable
 */
void arc_pcd_disable(struct pcd_instance *pcd)
{
        TRACE_MSG0(pcd->TAG, "Disabled");
}

/*! arc_pcd_framenum() - get current framenum
 */
static u16 arc_pcd_framenum (struct pcd_instance *pcd)
{
        u16                     frame = (UOG_FRINDEX & USB_FRINDEX_MASKS);
        frame &= ~(0x7);
        frame >>= 3;
        return (int)( frame );
}

/* arc_pcd_ticks - get current ticks
 */
#define MXC_GPT_GPTCNT          (IO_ADDRESS(GPT1_BASE_ADDR + 0x24))
otg_tick_t arc_pcd_ticks (struct pcd_instance *pcd)
{
        unsigned long           ticks = 0;
        ticks = __raw_readl(MXC_GPT_GPTCNT);
        return (otg_tick_t) ticks;
}

/* arc_pcd_elapsed - return micro-seconds between two tick values
 */
otg_tick_t arc_pcd_elapsed(otg_tick_t *t1, otg_tick_t *t2)
{
        otg_tick_t              ticks = (((*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1)));
        return (otg_tick_t) ticks / 17;  // 185 vs 189
}

/*****************************************************************************************/
/*! arc_add_buffer_to_dtd
 *
 * C.f. 39.16.5.3 - case 1: Link list is empty
 */
static void arc_add_buffer_to_dtd (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint,
                int dir, int len, int offset)
{
        struct otg_instance     *otg = pcd->otg;
        struct usbd_urb         *urb = endpoint->active_urb;
        struct arc_private_struct *privdata = endpoint->privdata;

        u8                      hs = pcd->bus->high_speed;
        u8                      physicalEndpoint = endpoint->physicalEndpoint[hs];
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      epnum = bEndpointAddress & 0x3f;
        u16                     wMaxPacketSize = endpoint->wMaxPacketSize[hs];
        struct ep_queue_head    *dQH = &udc_controller->ep_qh[2 * epnum + dir];
        struct ep_td_struct     *dtd = &(udc_controller->ep_dtd[2 * epnum + dir]);

        u32                     mask = 0;
        int                     timeout1 = 0;
        int                     timeout2 = 0;

        u32                     endptstat = -1;
        u32                     endptprime = -1;
        u32                     endptcomplete = -1;

        TRACE_MSG6(pcd->TAG, "[%2d] USBCMD: %08x ENDPTPRIME: %08x COMPLETE: %08x STATUS: %08x %s",
                        2*epnum+dir, UOG_USBCMD, UOG_ENDPTPRIME, UOG_ENDPTCOMPLETE,
                        (u32)dQH->size_ioc_int_sts, (dir == ARC_DIR_OUT) ? "OUT" : "IN");

        if (urb && urb->buffer) {

                TRACE_MSG4(pcd->TAG, "buffer: %x length: %d alloc: %d dir: %d ",
                                urb->buffer, urb->actual_length, urb->buffer_length, dir);

                /* flush cache for IN */
                if ((dir == ARC_DIR_IN) && urb->actual_length)
                        dma_cache_maint(urb->buffer, urb->actual_length, DMA_TO_DEVICE);

                /* invalidate cache for OUT */
                else if ((dir == ARC_DIR_OUT) && urb->buffer_length)
                        dma_cache_maint(urb->buffer, urb->alloc_length, DMA_FROM_DEVICE);
        }

        /* Set size and interrupt on each dtd, Clear reserved field,
         * set pointers and flush from cache, and save in cur_dqh for dtd_releases()
         */
        memset(dtd, 0, sizeof(struct ep_td_struct));
        dtd->size_ioc_sts = cpu_to_le32(((len << DTD_LENGTH_BIT_POS) | DTD_IOC | DTD_STATUS_ACTIVE));
        dtd->size_ioc_sts &= cpu_to_le32(~DTD_RESERVED_FIELDS);
        dtd->buff_ptr0 = cpu_to_le32(endpoint->active_urb ?  (u32) (virt_to_phys (endpoint->active_urb->buffer + offset)) : 0);
        dtd->next_td_ptr = cpu_to_le32(DTD_NEXT_TERMINATE);
        dtd->next_td_virt = NULL;
        dma_cache_maint(dtd, sizeof(struct ep_td_struct), DMA_TO_DEVICE);
        privdata->cur_dqh = dQH;

        /* Case 1 - Step 1 - Write dQH next pointer and dQH terminate bit to 0 as single DWord */
        dQH->next_dtd_ptr = cpu_to_le32( virt_to_phys((void *)dtd) & EP_QUEUE_HEAD_NEXT_POINTER_MASK);

        /* Case 1 - Step 2 - Clear active and halt bit */
        dQH->size_ioc_int_sts &= le32_to_cpu(~(EP_QUEUE_HEAD_STATUS_ACTIVE | EP_QUEUE_HEAD_STATUS_HALT));

        dma_cache_maint(dQH, sizeof(struct ep_queue_head), DMA_TO_DEVICE);

        /* Case 1 - Step 3 - Prime endpoint by writing ENDPTPRIME */
        mask = (dir == ARC_DIR_OUT) ?  (1 << epnum) : (1 << (epnum + 16));

        /* Verify that endpoint PRIME is not set, wait if necessary. */
        for (timeout1 = 0; (UOG_ENDPTPRIME & mask) && (timeout1 ++ < 100); udelay(1));

        /* ep0 needs extra tests */
        UNLESS(epnum) {
                /* C.f. 39.16.3.2.2 Data Phase */
                UOG_ENDPTPRIME |= mask;
                for (timeout2 = 0; timeout2++ < 100; ) {
                        endptprime = UOG_ENDPTPRIME;    // order may be important
                        endptstat = UOG_ENDPTSTAT;      // we check stat after prime
                        BREAK_IF(endptstat & mask);
                        BREAK_UNLESS(endptprime & mask);
                }
                if (!(endptstat & mask) && !(endptprime & mask)) {
                        TRACE_MSG2(pcd->TAG, "[%2d] ENDPTSETUPSTAT: %04x PREMATURE FAILUURE", 2*epnum+dir, UOG_ENDPTSETUPSTAT);
                }
                TRACE_MSG6(pcd->TAG, "[%2d] ENDPTPRIME %08x ENPTSTAT: %08x mask: %08x timeout: %d:%d SET",
                                2*epnum+dir, UOG_ENDPTPRIME, UOG_ENDPTSTAT, mask, timeout1, timeout2);;
        }
        /* epn general case */
        else {
                /* Hit PRIME bit until STATUS bit is set. */
                UOG_ENDPTPRIME |= mask;
                //for (timeout2 = 0; !(UOG_ENDPTSTAT & mask) && (timeout2++ < 100); /* UOG_ENDPTPRIME |= mask */);

                for (timeout2 = 0; timeout2++ < 100; ) {
                        endptprime = UOG_ENDPTPRIME;    // order may be important
                        endptstat = UOG_ENDPTSTAT;      // we check stat after prime
                        endptcomplete = UOG_ENDPTCOMPLETE;
                        BREAK_IF(endptstat & mask);
                        BREAK_IF(endptcomplete & mask);
                        UNLESS(endptprime & mask) {
                                TRACE_MSG8(pcd->TAG, "[%2d] ENDPTPRIME %08x:%08x ENPTSTAT: %08x:%08x COMPLETE: %08x "
                                                "mask: %08x timeout: %x NOT SET",
                                                2*epnum+dir, endptprime, UOG_ENDPTPRIME, endptstat, UOG_ENDPTSTAT,
                                                UOG_ENDPTCOMPLETE,
                                                mask, (timeout1 << 8) | timeout2);;
                                udelay(1);
                                endptstat = UOG_ENDPTSTAT;      // we check stat after prime
                                BREAK_IF(endptstat & mask);
                                UOG_ENDPTPRIME |= mask;
                        }
                }
                TRACE_MSG8(pcd->TAG, "[%2d] ENDPTPRIME %08x:%08x ENPTSTAT: %08x:%08x COMPLETE: %08x "
                                "mask: %08x timeout: %x SET",
                                2*epnum+dir, endptprime, UOG_ENDPTPRIME, endptstat, UOG_ENDPTSTAT,
                                UOG_ENDPTCOMPLETE,
                                mask, (timeout1 << 8) | timeout2);;
        }

}

/* arc_dtd_releases
 */
static void arc_dtd_releases (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint, int dir)
{
        struct arc_private_struct *privdata = endpoint->privdata;
        dma_cache_maint(privdata->cur_dtd, sizeof(struct ep_td_struct), (dir == ARC_DIR_OUT) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
        dma_cache_maint(privdata->cur_dqh, sizeof(struct ep_queue_head), (dir == ARC_DIR_OUT) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
}


/* arc_pcd_start_endpoint_in - start transmit
 */
void arc_pcd_start_endpoint_in (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        struct otg_instance     *otg = pcd->otg;
        struct usbd_urb         *tx_urb;
        u8                      hs = pcd->bus->high_speed;
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      epnum = bEndpointAddress & 0x3f;

        UNLESS(epnum) {
                TRACE_MSG1(pcd->TAG, "[ 0] ENDPTSETUPSTAT: %04x", UOG_ENDPTSETUPSTAT);
                RETURN_IF (UOG_ENDPTSETUPSTAT & 0x1);
        }
        if ((tx_urb = endpoint->tx_urb)) {
                endpoint->last = pcd_tx_sendzlp(endpoint) ?  0 : tx_urb->actual_length;
                TRACE_MSG4(pcd->TAG, "[%2d] urb length: %d sent: %d %s",
                                epnum, tx_urb->actual_length, endpoint->sent, endpoint->last ? "" : "ZLP");
                arc_add_buffer_to_dtd (pcd, endpoint, ARC_DIR_IN, endpoint->last, endpoint->sent);
        }
}

/* arc_pcd_start_endpoint_out - start receive
 */
void arc_pcd_start_endpoint_out (struct pcd_instance *pcd,struct usbd_endpoint_instance *endpoint)
{
        //struct usbd_urb         *rcv_urb;
        //u8                      hs = pcd->bus->high_speed;
        //u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        //u8                      epnum = bEndpointAddress & 0x3f;

        struct usbd_urb         *rcv_urb;
        u8                      hs;
        u8                      bEndpointAddress;
        u8                      epnum;

        UNLESS (pcd) printk(KERN_INFO"%s: pcd: %p\n", __FUNCTION__, pcd);
        UNLESS (pcd) printk(KERN_INFO"%s: pcd->bus: %p\n", __FUNCTION__, pcd->bus);
        hs = pcd->bus->high_speed;

        UNLESS (endpoint) printk(KERN_INFO"%s: endpoint: %p\n", __FUNCTION__, endpoint);
        bEndpointAddress = endpoint->bEndpointAddress[hs];
        epnum = bEndpointAddress & 0x3f;

        UNLESS(epnum) {
                //otg_led(LED2, 0);
                TRACE_MSG1(pcd->TAG, "[ 0] ENDPTSETUPSTAT: %04x", UOG_ENDPTSETUPSTAT);
                RETURN_IF (UOG_ENDPTSETUPSTAT & 0x1);
                //otg_led(LED2, 1);
        }
        if ((rcv_urb = pcd_rcv_next_irq(endpoint))) {
                TRACE_MSG3(pcd->TAG, "[%2d] urb length: %d actual: %d length: %d", epnum,
                                rcv_urb->actual_length, rcv_urb->buffer_length);
                arc_add_buffer_to_dtd (pcd, endpoint, ARC_DIR_OUT, rcv_urb->buffer_length, 0);
        }
}

/*! arc_pcd_setup_ep - setup endpoint
 * @param pcd -
 * @param ep -
 * @param endpoint
 * @return none
 */
void arc_pcd_setup_ep (struct pcd_instance *pcd, unsigned int ep, struct usbd_endpoint_instance *endpoint)
{
        struct usbd_bus_instance *bus = pcd->bus;
        struct ep_queue_head    *dQH = NULL;

        u8                      hs = pcd->bus->high_speed;
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      bmAttributes = endpoint->bmAttributes[hs];
        u8                      epnum = bEndpointAddress & 0x3f;
        u16                     wMaxPacketSize = endpoint->wMaxPacketSize[hs];
        u8                      dir = (bEndpointAddress & 0x80) ? 1 : 0;
        u8                      type = bmAttributes & 0x3;

        int                     timeout;
        unsigned int            tmp = 0;

        struct arc_private_struct      *privdata;

        dQH = &udc_controller->ep_qh[2 * epnum + dir];
        UNLESS(endpoint->privdata) {
                endpoint->privdata = (struct arc_private_struct *) CKMALLOC (sizeof(struct arc_private_struct));
        }
        RETURN_UNLESS(privdata = endpoint->privdata);

        /* Set the packet size */
        switch (type) {
        case 0:                 /* Control */
                tmp = (wMaxPacketSize << EP_QUEUE_HEAD_MAX_PKT_LEN_POS) | EP_QUEUE_HEAD_IOS;
                break;

        case 2:                 /* Bulk */
        case 3:                 /* Interrupt */
                tmp = wMaxPacketSize << EP_QUEUE_HEAD_MAX_PKT_LEN_POS;
                tmp |= EP_QUEUE_HEAD_ZLT_SEL;
                break;
        }
        dQH->max_pkt_length = cpu_to_le32(tmp);

        dma_cache_maint(dQH, sizeof(struct ep_queue_head), DMA_TO_DEVICE);

        /* Set the control register of endpoint */
        tmp = dir ?
                (EPCTRL_TX_ENABLE | ((u32)(type) << EPCTRL_TX_EP_TYPE_SHIFT) | (epnum ? EPCTRL_TX_DATA_TOGGLE_RST : 0)) :
                (EPCTRL_RX_ENABLE | ((u32)(type) << EPCTRL_RX_EP_TYPE_SHIFT) | (epnum ? EPCTRL_RX_DATA_TOGGLE_RST : 0));

        USBOTG_REG32(0x1c0 + epnum*4) |= tmp;

        /* wait for correct writing */
        timeout = 10000000;     // XXX This needs to be fixed, should not need to resort to timeout
        while ( ( !(USBOTG_REG32(0x1c0 + epnum*4) & (tmp & (EPCTRL_TX_ENABLE | EPCTRL_RX_ENABLE)) ) ) && --timeout) { continue; }
        if (timeout == 0)
                printk(KERN_INFO "%s: TIMEOUT\n", __FUNCTION__);

        TRACE_MSG4(pcd->TAG, "[%2x] Control register for size: %d is %x (%x)",
                        2*epnum+dir, wMaxPacketSize, tmp, USBOTG_REG32(0x1c0 + epnum*4));

        if (!epnum) {
                dir = 1;
                dQH = &udc_controller->ep_qh[2 * epnum + dir];
                tmp = (wMaxPacketSize << EP_QUEUE_HEAD_MAX_PKT_LEN_POS) | EP_QUEUE_HEAD_IOS;
                dQH->max_pkt_length = le32_to_cpu(tmp);
                dma_cache_maint(dQH, sizeof(struct ep_queue_head), DMA_FROM_DEVICE);
                tmp = USBOTG_REG32(0x1c0 + epnum*4);
                tmp |= EPCTRL_TX_ENABLE;
                tmp |= ((unsigned int)(type) << EPCTRL_TX_EP_TYPE_SHIFT);
                USBOTG_REG32(0x1c0 + epnum*4) = tmp;

                // XXX This needs to be fixed, should not need to resort to timeout, wait for correct writing
                timeout = 10000000;
                while ( ( !(USBOTG_REG32(0x1c0 + epnum*4) & (tmp & (EPCTRL_TX_ENABLE | EPCTRL_RX_ENABLE)) ) ) && --timeout)
                        continue;
                if (timeout == 0) printk(KERN_INFO "%s: TIMEOUT\n", __FUNCTION__);
        }
        privdata->cur_dtd = &(udc_controller->ep_dtd[2 * epnum + dir]);
        privdata->cur_dqh = &udc_controller->ep_qh[2 * epnum + dir];
}

/* arc_pcd_disable_ep -
 */
void arc_pcd_disable_ep(struct pcd_instance *pcd, unsigned int ep, struct usbd_endpoint_instance *endpoint)
{
        TRACE_MSG1(pcd->TAG, "endpoint->privdata: %p", endpoint->privdata);
        RETURN_UNLESS (endpoint->privdata);
        LKFREE(endpoint->privdata);
        endpoint->privdata = NULL;
}


/*! arc_pcd_cancel_irq
 */
void arc_pcd_cancel_irq (struct pcd_instance *pcd, struct usbd_urb *urb)
{
        struct usbd_endpoint_instance *endpoint = urb->endpoint;

        u8                      hs = pcd->bus->high_speed;
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      epnum = bEndpointAddress & 0x3f;
        u8                      dir = (bEndpointAddress & 0x80) ? 1 : 0;

        u32                     mask = (dir == ARC_DIR_OUT) ?  (1 << epnum) : (1 << (epnum + 16));

        TRACE_MSG5(pcd->TAG, "[%2d] USBCMD: %08x ENDPTPRIME: %08x COMPLETE: %08x mask: %08x FLUSH",
                        2*epnum+dir, UOG_USBCMD, UOG_ENDPTPRIME, UOG_ENDPTCOMPLETE, mask);


        UOG_ENDPTFLUSH = mask;

        TRACE_MSG5(pcd->TAG, "[%2d] USBCMD: %08x ENDPTPRIME: %08x COMPLETE: %08x mask: %08x FLUSH",
                        2*epnum+dir, UOG_USBCMD, UOG_ENDPTPRIME, UOG_ENDPTCOMPLETE, mask);

        TRACE_MSG5(pcd->TAG, "[%2d] USBCMD: %08x ENDPTPRIME: %08x COMPLETE: %08x mask: %08x FLUSH",
                        2*epnum+dir, UOG_USBCMD, UOG_ENDPTPRIME, UOG_ENDPTCOMPLETE, mask);

}


extern void fsl_platform_test_mode_select(struct fsl_usb2_platform_data *pdata, int on);
/* arc_pcd_device_feature - device feature
 */
int arc_pcd_device_feature(struct pcd_instance *pcd, int selector, int flag)
{
        u32 tmp = UOG_PORTSC1 | (selector << 16);
        UOG_PORTSC1 = tmp;
        return 0;
}

/*
 * arc_endpoint_halted() - is endpoint halted
 */
static int
arc_endpoint_halted (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
        u8                      hs = pcd->bus->high_speed;
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      epnum = bEndpointAddress & 0x3f;
        u8                      dir = (bEndpointAddress & 0x80) ? 1 : 0;
	u32			tmp = USBOTG_REG32(0x1c0 + epnum*4);
        switch (dir) {
        case USB_DIR_IN:
		TRACE_MSG3(pcd->TAG, "epn: %d dir: %d ENDPTCTRLn: %04x USB_DIR_IN", epnum, dir, tmp);
		return BOOLEAN( tmp & EPCTRL_TX_EP_STALL );

	case USB_DIR_OUT:
		TRACE_MSG3(pcd->TAG, "epn: %d dir: %d ENDPTCTRLn: %04x USB_DIR_OUT", epnum, dir, tmp);
		return BOOLEAN( tmp & EPCTRL_RX_EP_STALL );
	}
	return 0;
}

/* arc_halt_endpoint - halt endpoint
 */
int arc_halt_endpoint(struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint, int flag)
{
        u8                      hs = pcd->bus->high_speed;
        u8                      bEndpointAddress = endpoint->bEndpointAddress[hs];
        u8                      epnum = bEndpointAddress & 0x3f;
        u8                      dir = (bEndpointAddress & 0x80) ? 1 : 0;
	u32			tmp = USBOTG_REG32(0x1c0 + epnum*4);
        u32                     mask = (dir == USB_DIR_IN) ? EPCTRL_TX_EP_STALL : EPCTRL_RX_EP_STALL;

        tmp = flag ? (tmp | mask) : (tmp & ~mask);
	USBOTG_REG32(0x1c0 + epnum*4) = tmp;

        TRACE_MSG5(pcd->TAG, "epn: %d dir: %d flag: %d ENDPTCTRLn: %04x USB_DIR_%s STALLING",
                        epnum, dir, flag, tmp, (dir == USB_DIR_IN) ? "IN" : "OUT");
	return 0;
}


#if HAVE_PLATFORM_ARC_REMOTE_WAKEUP
extern void fsl_platform_perform_remote_wakeup(struct fsl_usb2_platform_data *pdata);

/*! arc_remote_wakeup
 */
static int arc_remote_wakeup(struct pcd_instance *pcd)
{
        struct usbd_bus_instance *bus = pcd->bus;
        struct otg_dev          *otg_dev = pcd->privdata;
        struct device           *device = otg_dev_get_drvdata(otg_dev);
        struct platform_device  *pdev = to_platform_device(device);
        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

        printk(KERN_INFO"%s:\n", __FUNCTION__);
        fsl_platform_perform_remote_wakeup(pdata);
        return 0;
}
#endif

/*! arc_vbus_status() - enable
 * This is called to return status of the PCD and USBD stack.
 * Start or stop the UDC.
 */
static int
arc_vbus_status (struct pcd_instance *pcd)
{
        u32                     vbus = UOG_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS;
        TRACE_MSG2(pcd->TAG, "UOG_PORTSC1: %x, vbus: %x", UOG_PORTSC1, vbus);
        return BOOLEAN(vbus);
}

/*! arc_softcon
 * @param pcd - pcd pointer
 * @param flag -
 *
 * Enable or disable pullup.
 */
int arc_softcon(struct pcd_instance *pcd, int flag)
{
        TRACE_MSG1(pcd->TAG, "ARC PULLUP %s", flag ? "SET": "RESET");
        UOG_USBCMD = flag ? (UOG_USBCMD | USB_CMD_RUN_STOP) : (UOG_USBCMD & ~USB_CMD_RUN_STOP);
        return 0;
}


/* ********************************************************************************************* */

struct usbd_pcd_ops usbd_pcd_ops = {
        .bmAttributes =
                #ifdef CONFIG_OTG_SELF_POWERED
                USB_BMATTRIBUTE_SELF_POWERED |
                #endif /* CONFIG_OTG_SELF_POWERED */
                #ifdef CONFIG_OTG_REMOTE_WAKEUP
                USB_BMATTRIBUTE_REMOTE_WAKEUP |
                #endif /* CONFIG_OTG_REMOTE_WAKEUP */
                //USB_OTG_HNP_SUPPORTED | USB_OTG_SRP_SUPPORTED |
                USB_BMATTRIBUTE_RESERVED,
        #ifndef CONFIG_OTG_SELF_POWERED
        .bMaxPower = CONFIG_OTG_BMAXPOWER,
        #else /* CONFIG_OTG_SELF_POWERED */
        .bMaxPower = 1,
        #endif /* CONFIG_OTG_SELF_POWERED */
        .max_endpoints =  0,
        .high_speed_capable = TRUE,
        .ep0_packetsize =  64,
        .name =  UDC_NAME,
        .start = arc_pcd_start,
        .stop = arc_pcd_stop,
        .disable = arc_pcd_disable,
        .disable_ep = arc_pcd_disable_ep,
        .start_endpoint_in = arc_pcd_start_endpoint_in,
        .start_endpoint_out = arc_pcd_start_endpoint_out,
        .request_endpoints = pcd_request_endpoints,
        .setup_ep = arc_pcd_setup_ep,
        .cancel_in_irq = arc_pcd_cancel_irq,
        .cancel_out_irq = arc_pcd_cancel_irq,
        .device_feature = arc_pcd_device_feature,
	.halt_endpoint = arc_halt_endpoint,
	.endpoint_halted = arc_endpoint_halted,
	.remote_wakeup = arc_remote_wakeup,
	.vbus_status = arc_vbus_status,
	.softcon = arc_softcon,
        .ticks = arc_pcd_ticks,
        .elapsed = arc_pcd_elapsed,
        .framenum = arc_pcd_framenum,
};

/* ********************************************************************************************* */
/*! arc_ep0_irq
 */
static irqreturn_t arc_ep0_irq(struct pcd_instance *pcd)
{
        struct usbd_endpoint_instance *endpoint = pcd->bus->endpoint_array + 0;
        struct usbd_urb         *urb = endpoint->active_urb;

        /* SETUP - Device request received */
        if (UOG_ENDPTSETUPSTAT & 0x1) {
                struct usbd_device_request      request;
                TRACE_MSG2(pcd->TAG, "EP0 SETUP active: %p %x", urb, urb ? urb->flags : 0);

                /* Complete any outstanding transfers */
                if (urb && (urb->flags & USBD_URB_IN)) pcd_tx_complete_irq(endpoint, 0);
                if (urb && (urb->flags & USBD_URB_OUT)) pcd_rcv_complete_irq(endpoint, 0, 0);

                /* Read setup - reset UOG_ENDPTSETUPSTAT */
                arc_read_setup_buffer(pcd, (u8 *) &request);

                if (pcd_recv_setup_irq(pcd, &request)) {
                        /* endpoint requested not handled - need to stall */
                        TRACE_MSG0(pcd->TAG, "REQUEST NOT HANDLED - STALLED");
                        USBOTG_REG32(0x1c0 + 0*4) |= EPCTRL_TX_EP_STALL | EPCTRL_RX_EP_STALL;
                        return IRQ_HANDLED;
                }
                /* Finish if no data phase */
                UNLESS (request.wLength) {
                        TRACE_MSG0(pcd->TAG, "REQUEST HANDLED OK - NO DATA PHASE");
                        arc_add_buffer_to_dtd (pcd, endpoint, ARC_DIR_IN, 0, 0);
                }
                return IRQ_HANDLED;
        }
        /* TX/IN Complete */
        if (UOG_ENDPTCOMPLETE & 0x10000) {
                TRACE_MSG2(pcd->TAG, "EP0 IN COMPLETE active: %p %x", urb, urb ? urb->flags : 0);

                /* set address here if previous request was SET_ADDRESS */
                if (pcd->new_address) {
                        UOG_DEVICEADDR = pcd->new_address << USB_DEVICE_ADDRESS_BIT_POS;
                        pcd->new_address = 0;
                }

                /* sync and clear complete bit */
                arc_dtd_releases (pcd, endpoint, ARC_DIR_IN);
                UOG_ENDPTCOMPLETE = 0x10000;

                /* finish urb, reset zlp flag if it's there, we always send zlp */
                if (pcd_tx_complete_irq(endpoint, 0)) {
                        TRACE_MSG2(pcd->TAG, "EP0 IN COMPLETE ZLP check: %p %x", urb, urb ? urb->flags : 0);
                        if (pcd_tx_sendzlp(endpoint)) {
                                pcd_tx_complete_irq(endpoint, 0);
                                TRACE_MSG2(pcd->TAG, "EP0 IN COMPLETE ZLP active: %p %x", urb, urb ? urb->flags : 0);
                        }
                }
                arc_add_buffer_to_dtd (pcd, endpoint, ARC_DIR_OUT, 0, 0);
                return IRQ_HANDLED;
        }
        /* RX/OUT Complete */
        if (UOG_ENDPTCOMPLETE & 0x1) {
                struct arc_private_struct *privdata = endpoint->privdata;
                struct ep_td_struct     *curr_td = privdata->cur_dtd;
                int length;

                /* sync and clear complete bit */
                arc_dtd_releases (pcd, endpoint, ARC_DIR_OUT);
                length = ((le32_to_cpu(curr_td->size_ioc_sts) & DTD_PACKET_SIZE) >> DTD_LENGTH_BIT_POS);
                UOG_ENDPTCOMPLETE = 0x1;

                TRACE_MSG3(pcd->TAG, "EP0 OUT COMPLETE active: %p %x length: %d", urb, urb ? urb->flags : 0, length);
                UNLESS (urb && (urb->flags & USBD_URB_OUT)) {
                        TRACE_MSG0(pcd->TAG, "EP0 OUT COMPLETE - ZLP received");
                        return IRQ_HANDLED;
                }
                length = arc_read_rcv_buffer (pcd, endpoint);
                pcd_rcv_complete_irq(endpoint, length, 0);
                arc_add_buffer_to_dtd (pcd, endpoint, ARC_DIR_IN, 0, 0);
                return IRQ_HANDLED;
        }
        return IRQ_NONE;
}

/*! arc_epn_irq
 */
static irqreturn_t arc_epn_irq(struct pcd_instance *pcd)
{
        u32 endptcomplete;
        /* N.B. need to loop until nothing left to do, endpoints can complete while
         * working on other endpoints.
         */
        while ((endptcomplete = (UOG_ENDPTCOMPLETE & 0xfffefffe))) {

                /* We have endptcomplete bits, immediately reset so that subsequent
                 * completions will be seen when we loop. Note that we mask ep0 status.
                 */
                UOG_ENDPTCOMPLETE = endptcomplete;

                /* Loop while non-zero - N.B. clz requires pre-test as it is undefined on zero */
                while (endptcomplete) {
                        int bit = 31 - __builtin_clz(endptcomplete);    /* highest non-zero bit */
                        int ep = bit & 0xf;
                        int dir = (bit > 15) ? ARC_DIR_IN : ARC_DIR_OUT;
                        struct usbd_endpoint_instance *endpoint = pcd->bus->endpoint_array + (2 * ep + dir);
                        struct arc_private_struct *privdata = endpoint->privdata;
                        struct ep_td_struct *cur_dtd = privdata->cur_dtd;
                        int errors = le32_to_cpu(cur_dtd->size_ioc_sts) & DTD_ERROR_MASK;
                        int remain_len;
                        TRACE_MSG6(pcd->TAG, "[%2d:%2d] mask: %08x  endptcomplete: %08x %s %s", bit, ep,
                                        (1 << bit), endptcomplete, (ARC_DIR_IN) ? "IN" : "OUT", ep ? "EPN" : "EP0");
                        switch (dir) {
                        case ARC_DIR_IN: /* Transmitting */
                                arc_dtd_releases (pcd, endpoint, ARC_DIR_IN);
                                if ((pcd_tx_complete_irq(endpoint, 0)))
                                        arc_pcd_start_endpoint_in(pcd, endpoint);
                                break;

                        case ARC_DIR_OUT: /* Receiving */
                                remain_len = arc_read_rcv_buffer (pcd, endpoint);
                                arc_dtd_releases (pcd, endpoint, ARC_DIR_OUT);
                                if ( errors & DTD_STATUS_HALTED ) {
                                        TRACE_MSG4(pcd->TAG, "[%2d:%2d] Current dtd is halted size_ioc_sts: %08x errors: %02x",
                                                        bit, ep, le32_to_cpu(cur_dtd->size_ioc_sts), errors);
                                        cur_dtd->size_ioc_sts &= cpu_to_le32(errors);
                                        dma_cache_maint(cur_dtd, sizeof(struct ep_td_struct), DMA_TO_DEVICE);
                                }

                                if ((pcd_rcv_complete_irq(endpoint, remain_len, 0))) {

                                        UNLESS(pcd && pcd->bus && endpoint) {
                                                printk(KERN_INFO"%s: NULL pcd: %p bus: %p endpoint: %p\n",
                                                                __FUNCTION__, pcd, pcd ? pcd->bus : NULL, endpoint);
                                                break;
                                        }
                                        arc_pcd_start_endpoint_out(pcd, endpoint);
                                }
                                break;
                        }
                        endptcomplete &= ~(1 << bit);
                }
        }
        return IRQ_HANDLED;
}

/*! arc_pcd_isr
 */
static irqreturn_t arc_pcd_isr(struct otg_dev *otg_dev, void *data, u32 mask)
{
        u32 irq_src;
        struct otg_instance     *otg = otg_dev ? otg_dev->otg_instance : NULL;
        struct pcd_instance     *pcd = otg_dev ? otg_dev->pcd_instance : NULL;
        struct usbd_bus_instance *bus = pcd->bus;
        int                     timeout;
        irqreturn_t             status = IRQ_NONE;

        otg_led(LED2, 1);
        otg->interrupts++;
        if (TRACE_VERBOSE) {
                TRACE_MSG0(pcd->TAG, "--");
                TRACE_MSG5(pcd->TAG, "UOG_USBINTR: %08x UOG_ENDPTSTAT: %08x "
                        "UOG_ENDPTSETUPSTAT: %04x, UOG_ENDPTCOMPLETE: %08x, UOG_PORTSC1: %08x",
                        UOG_USBINTR, UOG_ENDPTSTAT, UOG_ENDPTSETUPSTAT, UOG_ENDPTCOMPLETE, UOG_PORTSC1);
        }

        /* verify controller is running */
        RETURN_IRQ_NONE_UNLESS(UOG_USBCMD & USB_CMD_RUN_STOP);

        /* get interrupt source and reset in USBSTS register */
        irq_src = UOG_USBSTS & UOG_USBINTR;
        UOG_USBSTS &= irq_src;
        /* USB */
        if (irq_src & USB_STS_INT) {
                if ( (UOG_ENDPTSETUPSTAT & 0x1) || (UOG_ENDPTCOMPLETE & 0x1) || (UOG_ENDPTCOMPLETE & 0x10000))
                        status = arc_ep0_irq (pcd);
                if ( (UOG_ENDPTCOMPLETE & 0xfffefffe) )
                        status = arc_epn_irq (pcd);
        }
        /* SOF */
        if (irq_src & USB_STS_SOF) {
                // TRACE_MSG0(pcd->TAG, "SOF interrupt");
                status = IRQ_HANDLED;
        }
        /* port changed */
        if (irq_src & USB_STS_PORT_CHANGE) {
                u32 speed;
                TRACE_MSG0(pcd->TAG, "PORT CHANGE interrupt");
                if (!(UOG_PORTSC1 & PORTSCX_PORT_RESET)) {
                        speed = (UOG_PORTSC1 & PORTSCX_PORT_SPEED_MASK);
                        switch (speed) {
                        case PORTSCX_PORT_SPEED_HIGH:
                                TRACE_MSG0(pcd->TAG, "High Speed");
                                bus->high_speed = TRUE;
                                break;
                        case PORTSCX_PORT_SPEED_FULL:
                                TRACE_MSG0(pcd->TAG, "Full Speed");
                                bus->high_speed = FALSE;
                                break;
                        case PORTSCX_PORT_SPEED_LOW:
                                TRACE_MSG0(pcd->TAG, "Low Speed");
                                break;
                        default:
                                TRACE_MSG0(pcd->TAG, "Unknown Speed");
                                break;
                        }
                        status = IRQ_HANDLED;
                }
                if ((UOG_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS)) {
                        TRACE_MSG0(pcd->TAG, "Cable is attached");
                        otg_event(otg, VBUS_VLD | B_SESS_VLD, otg->pcd->TAG, "ARC VBUS VALID");
                        status = IRQ_HANDLED;
                }
                if (!(UOG_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS)) {
                        TRACE_MSG0(pcd->TAG, "Cable is detached");
                        otg_event(otg, VBUS_VLD_ | B_SESS_VLD_ | A_SESS_VLD_, otg->pcd->TAG, "ARC VBUS INVALID");
                        status = IRQ_HANDLED;
                }
        }
        /* reset  */
        if (irq_src & USB_STS_RESET) {
                u32 temp;
                TRACE_MSG0(pcd->TAG, "RESET interrupt");

                /* Reset address, endpoint status and complete */
                UOG_DEVICEADDR &= ~USB_DEVICE_ADDRESS_MASK;
                UOG_ENDPTSTAT = UOG_ENDPTSTAT;
                UOG_ENDPTCOMPLETE = UOG_ENDPTCOMPLETE;

                timeout = 10000000;     // XXX
                /* Wait until all endptprime bits cleared */
                while (UOG_ENDPTPRIME && --timeout) { continue; }
                if (timeout == 0) printk(KERN_ERR "%s: TIMEOUT\n", __FUNCTION__);

                UOG_ENDPTFLUSH = 0xFFFFFFFF;

                if (UOG_PORTSC1 & PORTSCX_PORT_RESET) {
                        TRACE_MSG0(pcd->TAG, "Bus reset");
                }
                else {
                        TRACE_MSG0(pcd->TAG, "Controller reset");
                        arc_udc_run ();
                }
                pcd_bus_event_handler_irq (pcd->bus, DEVICE_RESET, 0);
                pcd_bus_event_handler_irq (pcd->bus, DEVICE_ADDRESS_ASSIGNED, 0);
                status = IRQ_HANDLED;
        }
        /* SUSPEND / RESUME */
        if (irq_src & USB_STS_SUSPEND) {
                if (UOG_PORTSC1 & PORTSCX_PORT_SUSPEND) {
                        TRACE_MSG0(pcd->TAG, "SUSPEND interrupt");
                        pcd_check_device_feature(pcd->bus, USB_DEVICE_REMOTE_WAKEUP, 1);
                        pcd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_INACTIVE, 0);
                }
                else {
                        TRACE_MSG0(pcd->TAG, "RESUME interrupt");
                        pcd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_ACTIVITY, 0);
                        pcd_check_device_feature(pcd->bus, USB_DEVICE_REMOTE_WAKEUP, 0);
                }
                status = IRQ_HANDLED;
        }
        if (irq_src & (USB_STS_ERR | USB_STS_SYS_ERR)) {
                TRACE_MSG0(pcd->TAG, "Error in interrupt");
                status = IRQ_HANDLED;
        }
        if (TRACE_VERBOSE) TRACE_MSG6(pcd->TAG, "UOG_USBINTR: %08x UOG_ENDPTSTAT: %08x "
                        "UOG_ENDPTSETUPSTAT: %04x, UOG_ENDPTCOMPLETE: %08x, UOG_PORTSC1: %08x %s",
                        UOG_USBINTR, UOG_ENDPTSTAT, UOG_ENDPTSETUPSTAT, UOG_ENDPTCOMPLETE, UOG_PORTSC1,
                        (status == IRQ_HANDLED) ? "IRQ_HANDLED" : "IRQ_NONE");

        otg_led(LED2, 0);
        return status;
}

/*! default initialization
 */
int pcd_mod_init (struct otg_instance *otg);
void pcd_mod_exit (struct otg_instance *otg);

/*! arc_pcd_remove() - called to remove hardware
 * @param otg_dev - otg device
 */
static void
arc_pcd_remove(struct otg_dev *otg_dev)
{
        struct otg_instance     *otg = otg_dev->otg_instance;
        struct device           *device = otg_dev_get_drvdata(otg_dev);
        struct platform_device  *pdev = to_platform_device(device);
        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

        TRACE_MSG0(otg->pcd->TAG, "--");
        arc_udc_release ();
        udc_controller = NULL;
        pdata->platform_uninit(pdata);
//printk(KERN_INFO"%s: jumpin116 start--------\n ", __FUNCTION__);
        pcd_mod_exit(otg);
//printk(KERN_INFO"%s: jumpin116 end--------- \n", __FUNCTION__);
}

/*! arc_pcd_probe() - called to probe hardware
 * @param otg_dev - otg device
 *
 * This function should do minimal, one time only hardware recognition,
 * resource reservation and minimal setup. Typically to get to known
 * disabled state. It should not start the hardware.
 *
 */
static int arc_pcd_probe(struct otg_dev *otg_dev)
{
        struct device   *device = otg_dev_get_drvdata(otg_dev);
        struct otg_instance *otg = otg_dev->otg_instance;
        struct platform_device *pdev = to_platform_device(device);
        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

        usbd_pcd_ops.max_endpoints = UDC_MAX_ENDPOINTS;

        RETURN_ENODEV_IF(strcmp(pdev->name, "arc_udc"));
        UNLESS(pdata && pdata->platform_init) {
                printk(KERN_INFO"%s: NO TRANSCEIVER CONFIGURED\n", __FUNCTION__);
                return -ENODEV;
        }
        RETURN_ZERO_IF (pdata->platform_init(pdev));

        //fsl_platform_set_vbus_power(pdata, 0);
        RETURN_ENODEV_IF(pcd_mod_init(otg));
        udc_controller = (struct arcotg_udc *) arc_udc_init (otg_dev);
        return 0;
}

/*! arc_pcd_suspend
 */
static void arc_pcd_suspend(struct otg_dev *otg_dev, u32 state)
{
        /* NOTHING */
}

/*! arc_pcd_resume
 */
static void arc_pcd_resume(struct otg_dev *otg_dev)
{
        /* NOTHING */
}

/* ********************************************************************************************* */
static struct otg_dev_driver arc_pcd_driver = {
        .name =         "arc_udc",
        .id =           OTG_DRIVER_PCD,
        .probe =        arc_pcd_probe,
        .remove =       arc_pcd_remove,
        .suspend =      arc_pcd_suspend,
        .resume =       arc_pcd_resume,
        .irqs =         (1 << 1),               /* use irq at location #0 in udc_resources */
        .isr =          arc_pcd_isr,
        .ops =          &pcd_ops,
};

/*! arc_pcd_module_init() - module init
 */
int arc_pcd_module_init (struct otg_device_driver *otg_device_driver)
{
        return otg_dev_register_driver(otg_device_driver, &arc_pcd_driver);
}

/*! arc_pcd_module_exit() - module exit
 */
void arc_pcd_module_exit (struct otg_device_driver *otg_device_driver)
{
        otg_dev_unregister_driver (otg_device_driver, &arc_pcd_driver);
}
