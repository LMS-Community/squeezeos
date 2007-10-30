/*
 * usb_send.c 
 *
 * S3C2410 USB send function
 * endpoint 2
 *
 * bushi<bushi@mizi.com>
 */

#include <common.h>
#include <command.h>
#include <s3c2440.h>
#include <s3c24x0_usb.h>
#include "usb_ctl.h"

/* error number from linux */
#define EINTR            4	/* Interrupted system call */
#define EAGAIN          11	/* Try again */
#define EBUSY           16	/* Device or resource bus */
#define ENODEV          19	/* No such device */


//#define USB_DEBUG 1

#ifdef USB_DEBUG
#define LOG(arg...) printf(__FILE__":"__FUNCTION__"(): " ##arg)
#else
#define LOG(arg...) (void)(0)
#endif

/* Enable DMA on the endpoint 2. */

static char *ep2_buf;
static int ep2_len;
static usb_callback_t ep2_callback;
#ifdef USE_USBD_DMA
static dma_addr_t ep2_dma;
static dma_addr_t ep2_curdmapos;
#else
static char *ep2_curbufpos;
#endif
static int ep2_curdmalen;
static int ep2_remain;
static int dmachn_tx;
static int tx_pktsize;

extern int DNW;

void ep2_dma_callback(void *buf_id, int size)
{
	LOG("\n");
#ifdef USE_USBD_DMA
	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
	UD_ICSR1 |= UD_ICSR1_PKTRDY;
#endif
}


/* set feature stall executing, async */
void ep2_stall(void)
{
	LOG("\n");

	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
	UD_ICSR1 |= UD_ICSR1_SENDSTL;	/* send stall, force stall */
}

static void ep2_start(void)
{
#ifndef USE_USBD_DMA
	int i;
#endif

	LOG("\n");

	if (!ep2_len) {
		LOG("!ep2_len\n");
		return;
	}

	ep2_curdmalen = tx_pktsize;	/* 64 */

	if (ep2_curdmalen > ep2_remain)
		ep2_curdmalen = ep2_remain;

	LOG("ep2_curdmalen = %d\n", ep2_curdmalen);

	for (i = 0; i < ep2_curdmalen; i++) {
		LOG("ep2_curbufpos[i] = 0x%02x\n", ep2_curbufpos[i]);
		UD_FIFO2 = (u_char) ep2_curbufpos[i] & UD_FIFO2_DATA;
	}
	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
}

static void ep2_done(int flag)
{
	int size = ep2_len - ep2_remain;

	LOG("ep2_len = %d, ep2_remain = %d\n", ep2_len, ep2_remain);

	if (ep2_len) {
		ep2_len = 0;
#ifndef  CONFIG_S3C2410_OYSTER
		if (ep2_callback)
			ep2_callback(flag, size);
#endif
	}
}

int ep2_init(int chn)
{
	desc_t *pd = s3c24x0_usb_get_descriptor_ptr();
	tx_pktsize = __le16_to_cpu(pd->b.ep2.wMaxPacketSize);	/* 64 */

	LOG("tx_pktsize = %d\n", tx_pktsize);

	dmachn_tx = chn;
	ep2_done(-EAGAIN);
	return 0;
}

void ep2_reset(void)
{
	desc_t *pd = s3c24x0_usb_get_descriptor_ptr();
	tx_pktsize = __le16_to_cpu(pd->b.ep2.wMaxPacketSize);	/* 64 */

	LOG("tx_pktsize = %d\n", tx_pktsize);

	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
	UD_ICSR1 &= ~(UD_ICSR1_SENDSTL);	// write 0 to clear

	ep2_done(-EINTR);
}

void ep2_int_hndlr(int udcsr)
{
	int status;

	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
	status = UD_ICSR1;

	LOG("status = 0x%08x\n", status);

	if (status & UD_ICSR1_SENTSTL) {
		if (DNW) {
			UD_INDEX = UD_INDEX_EP3;
		} else {
			UD_INDEX = UD_INDEX_EP2;
		}
		UD_ICSR1 &= ~UD_ICSR1_SENTSTL;	// clear_ep1_sent_stall;
		return;
	}
	LOG("[%d]\n", __LINE__);

	if (!(status & UD_ICSR1_PKTRDY)) {
		LOG("[%d]\n", __LINE__);
		ep2_curbufpos += ep2_curdmalen;
		ep2_remain -= ep2_curdmalen;

		if (ep2_remain != 0) {
			LOG("[%d]\n", __LINE__);
			ep2_start();
		} else {
			LOG("[%d]\n", __LINE__);
			ep2_done(0);
		}
	}
}

int s3c24x0_usb_send(char *buf, int len, usb_callback_t callback)
{
	int flags;

	LOG("[%d]\n", __LINE__);

	if (usbd_info.state != USB_STATE_CONFIGURED)
		return -ENODEV;

	if (ep2_len)
		return -EBUSY;

	//kwg local_irq_save(flags);
	ep2_buf = buf;
	ep2_len = len;
	ep2_curbufpos = ep2_buf;
	ep2_callback = callback;
	ep2_remain = len;
	ep2_start();
	//kwg local_irq_restore(flags);

	return 0;
}


void s3c24x0_usb_send_reset(void)
{
	LOG("\n");
	ep2_reset();
}

int s3c24x0_usb_xmitter_avail(void)
{
	LOG("\n");

	if (usbd_info.state != USB_STATE_CONFIGURED) {
		LOG("[%d]\n", __LINE__);
		return -ENODEV;
	}
	if (ep2_len) {
		LOG("[%d]\n", __LINE__);
		return -EBUSY;
	}
	return 0;
}
