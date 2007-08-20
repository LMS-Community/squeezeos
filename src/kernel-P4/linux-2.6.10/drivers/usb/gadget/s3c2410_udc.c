/*
 * linux/drivers/usb/gadget/s3c24x0_udc.c
 * Samsung S3C24X0 on-chip full speed USB device controllers
 *
 * Copyright (C) 2005 for Samsung Electronics 
 * 		- by Jaswinder Singh Brar <jassi@samsung.com>
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

#include "s3c2410_udc.h"

#ifndef DEBUG_EP0
# define DEBUG_EP0(fmt,args...)
#endif
#ifndef DEBUG_SETUP
# define DEBUG_SETUP(fmt,args...)
#endif
#ifndef DEBUG
# define NO_STATES
# define DEBUG(fmt,args...)
#endif

//#define DEBUG_EP0 printk
//#define DEBUG_SETUP printk

#define	DRIVER_DESC		"Samsung USB Device Controller"
#define	DRIVER_VERSION		__DATE__

#ifndef _BIT			/* FIXME - what happended to _BIT in 2.6.7bk18? */
#define _BIT(x) (1<<(x))
#endif

struct elfin_udc	*the_controller;
static struct clk	*udc_clock;

static const char driver_name[] = "s3c2410_udc";
static const char driver_desc[] = DRIVER_DESC;
static const char ep0name[] = "ep0-control";

/*
  Local definintions.
*/

static char *state_names[] = {
	"WAIT_FOR_SETUP",
	"DATA_STATE_XMIT",
	"DATA_STATE_NEED_ZLP",
	"WAIT_FOR_OUT_STATUS",
	"DATA_STATE_RECV"
};

/*
  Local declarations.
*/
static int elfin_ep_enable(struct usb_ep *ep,
			     const struct usb_endpoint_descriptor *);
static int elfin_ep_disable(struct usb_ep *ep);
static struct usb_request *elfin_alloc_request(struct usb_ep *ep, int);
static void elfin_free_request(struct usb_ep *ep, struct usb_request *);
static void *elfin_alloc_buffer(struct usb_ep *ep, unsigned, dma_addr_t *,
				  int);
static void elfin_free_buffer(struct usb_ep *ep, void *, dma_addr_t,
				unsigned);
static int elfin_queue(struct usb_ep *ep, struct usb_request *, int);
static int elfin_dequeue(struct usb_ep *ep, struct usb_request *);
static int elfin_set_halt(struct usb_ep *ep, int);
static int elfin_fifo_status(struct usb_ep *ep);
static void elfin_fifo_flush(struct usb_ep *ep);
static void elfin_ep0_kick(struct elfin_udc *dev, struct elfin_ep *ep);
static void elfin_handle_ep0(struct elfin_udc *dev);

static void done(struct elfin_ep *ep, struct elfin_request *req,
		 int status);
static void stop_activity(struct elfin_udc *dev,
			  struct usb_gadget_driver *driver);
static int udc_enable(struct elfin_udc *dev);
static void udc_set_address(struct elfin_udc *dev, unsigned char address);
static void reconfig_usbd(void);

static __inline__ u32 usb_read(u32 port, u8 ind)
{
	__raw_writel(ind, S3C2410_UDC_INDEX_REG);
	return __raw_readl(port);
}

static __inline__ void usb_write(u32 val, u32 port, u8 ind)
{
	__raw_writel(ind, S3C2410_UDC_INDEX_REG);
	__raw_writel(val, port);
}

static __inline__ void usb_set(u32 val, u32 port, u8 ind)
{
	__raw_writel(ind, S3C2410_UDC_INDEX_REG);
	__raw_writel(__raw_readl(port) | val, port);
}

static __inline__ void usb_clear(u32 val, u32 port, u8 ind)
{
	__raw_writel(ind, S3C2410_UDC_INDEX_REG);
	__raw_writel(__raw_readl(port) & ~val, port);
}

static struct usb_ep_ops elfin_ep_ops = {
	.enable = elfin_ep_enable,
	.disable = elfin_ep_disable,

	.alloc_request = elfin_alloc_request,
	.free_request = elfin_free_request,

	.alloc_buffer = elfin_alloc_buffer,
	.free_buffer = elfin_free_buffer,

	.queue = elfin_queue,
	.dequeue = elfin_dequeue,

	.set_halt = elfin_set_halt,
	.fifo_status = elfin_fifo_status,
	.fifo_flush = elfin_fifo_flush,
};

/* Inline code */

static __inline__ int write_packet(struct elfin_ep *ep,
				   struct elfin_request *req, int max)
{
	u8 *buf;
	int length, count;
	u32 fifo = ep->fifo;

	buf = req->req.buf + req->req.actual;
	prefetch(buf);

	length = req->req.length - req->req.actual;
	length = min(length, max);
	req->req.actual += length;

	DEBUG("Write %d (max %d), fifo %p\n", length, max, fifo);

	count = length;

	while (count--)
	   __raw_writel(*buf++, fifo);

	return length;
}

#ifdef CONFIG_USB_GADGET_DEBUG_FILES

static const char proc_node_name[] = "driver/udc";

static int
udc_proc_read(char *page, char **start, off_t off, int count,
	      int *eof, void *_dev)
{
	char *buf = page;
	struct elfin_udc *dev = _dev;
	char *next = buf;
	unsigned size = count;
	unsigned long flags;
	int t;

	if (off != 0)
		return 0;

	local_irq_save(flags);

	/* basic device status */
	t = scnprintf(next, size,
		      DRIVER_DESC "\n"
		      "%s version: %s\n"
		      "Gadget driver: %s\n"
		      "\n",
		      driver_name, DRIVER_VERSION,
		      dev->driver ? dev->driver->driver.name : "(none)");
	size -= t;
	next += t;

	local_irq_restore(flags);
	*eof = 1;
	return count - size;
}

#define create_proc_files() 	create_proc_read_entry(proc_node_name, 0, NULL, udc_proc_read, dev)
#define remove_proc_files() 	remove_proc_entry(proc_node_name, NULL)

#else	/* !CONFIG_USB_GADGET_DEBUG_FILES */

#define create_proc_files() do {} while (0)
#define remove_proc_files() do {} while (0)

#endif	/* CONFIG_USB_GADGET_DEBUG_FILES */

/*
 * 	udc_disable - disable USB device controller
 */
static void udc_disable(struct elfin_udc *dev)
{
	DEBUG("%s, %p\n", __FUNCTION__, dev);

	udc_set_address(dev, 0);

	if (udc_clock) {
		clk_disable(udc_clock);
		clk_unuse(udc_clock);
		clk_put(udc_clock);
		udc_clock = NULL;
        }

	dev->ep0state = WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	dev->usb_address = 0;
}

/*
 * 	udc_reinit - initialize software state
 */
static void udc_reinit(struct elfin_udc *dev)
{
	u32 i;

	DEBUG("%s, %p\n", __FUNCTION__, dev);

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0state = WAIT_FOR_SETUP;

	/* basic endpoint records init */
	for (i = 0; i < S3C2410_MAX_ENDPOINTS; i++) {
		struct elfin_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->desc = 0;
		ep->stopped = 0;
		INIT_LIST_HEAD(&ep->queue);
		ep->pio_irqs = 0;
	}

	/* the rest was statically initialized, and is read-only */
}

#define BYTES2MAXP(x)	(x / 8)
#define MAXP2BYTES(x)	(x * 8)

#define FShft(Field)            ((Field) & 0x0000FFFF)
#define FInsrt(Value, Field)    (((unsigned long) Value) << FShft (Field))
#define Fld(Size, Shft)         (((Size) << 16) + (Shft))

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static int udc_enable(struct elfin_udc *dev)
{
	u32 val;

	DEBUG("%s, %p\n", __FUNCTION__, dev);

	dev->gadget.speed = USB_SPEED_UNKNOWN;

	val = usb_read(S3C2410_MISCCR, 0);
	//val &= ~((1<<3)|(1<<13));       // USB Device in Normal mode
	val &= ~(1<<3);       // USB Device in Normal mode
	usb_write(val, S3C2410_MISCCR, 0);

	val = FInsrt(0x78, Fld(8,12)) | FInsrt(0x02, Fld(6,4)) | FInsrt(0x03, Fld(2,0));
        __raw_writel(val, S3C2410_UPLLCON);

	/* Enable Clock */
	udc_clock = clk_get(NULL, "usb-device");
	if (!udc_clock) {
		printk("failed to get udc clock source\n");
		return -ENOENT;
	}
	clk_use(udc_clock);
	clk_enable(udc_clock);

	//val = (1<<0);
	//usb_write(val, S3C2410_UDC_EP_INT_REG, 0);	//clear
	//val = (1<<2);   //UD_USBINTE_RESET
	//usb_write(val, S3C2410_UDC_USB_INT_REG, 0);	//clear

	/* Set MAXP values for each */
	usb_write(dev->ep[0].ep.maxpacket>>3, S3C2410_UDC_MAXP_REG, 0);

	usb_write(dev->ep[1].ep.maxpacket>>3, S3C2410_UDC_MAXP_REG, 1);

	usb_write(dev->ep[2].ep.maxpacket>>3, S3C2410_UDC_MAXP_REG, 2);

	usb_write(dev->ep[3].ep.maxpacket>>3, S3C2410_UDC_MAXP_REG, 3);

	usb_write(dev->ep[4].ep.maxpacket>>3, S3C2410_UDC_MAXP_REG, 4);

	return 0;
}

/*
  Register entry point for the peripheral controller driver.
*/
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct elfin_udc *dev = the_controller;
	int retval;

	DEBUG("%s: %s\n", __FUNCTION__, driver->driver.name);

	if (!driver
	    || driver->speed != USB_SPEED_FULL
	    || !driver->bind
	    || !driver->unbind || !driver->disconnect || !driver->setup)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;

	/* first hook up the driver ... */
	dev->driver = driver;
	dev->gadget.dev.driver = &driver->driver;

	device_add(&dev->gadget.dev);
	retval = driver->bind(&dev->gadget);
	if (retval) {
		printk("%s: bind to driver %s --> error %d\n", dev->gadget.name,
		       driver->driver.name, retval);
		device_del(&dev->gadget.dev);

		dev->driver = 0;
		dev->gadget.dev.driver = 0;
		return retval;
	}

	reconfig_usbd();
	enable_irq(IRQ_USBD);

	printk("Registered gadget driver '%s'\n", driver->driver.name);

	return 0;
}

EXPORT_SYMBOL(usb_gadget_register_driver);

/*
  Unregister entry point for the peripheral controller driver.
*/
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct elfin_udc *dev = the_controller;
	unsigned long flags;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	spin_lock_irqsave(&dev->lock, flags);
	dev->driver = 0;
	stop_activity(dev, driver);
	spin_unlock_irqrestore(&dev->lock, flags);

	driver->unbind(&dev->gadget);
	device_del(&dev->gadget.dev);

	disable_irq(IRQ_USBD);

	printk("Unregistered gadget driver '%s'\n", driver->driver.name);
	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);

/*-------------------------------------------------------------------------*/

/** Write request to FIFO (max write == maxp size)
 *  Return:  0 = still running, 1 = completed, negative = errno
 */
static int write_fifo(struct elfin_ep *ep, struct elfin_request *req)
{
	u32 max;
	unsigned count;
	int is_last, is_short;

	max = le16_to_cpu(ep->desc->wMaxPacketSize);

	count = write_packet(ep, req, max);

	/* last packet is usually short (or a zlp) */
	if (unlikely(count != max))
		is_last = is_short = 1;
	else {
		if (likely(req->req.length != req->req.actual)
		    || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
		/* interrupt/iso maxpacket may not fill the fifo */
		is_short = unlikely(max < ep_maxpacket(ep));
	}

	/* requests complete when all IN data is in the FIFO */
	if (is_last) {
		if(!ep_index(ep)){	// EP0 must not come here
			BUG();
			//usb_set(S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_DE, 
			//		S3C2410_UDC_EP0_CSR_REG, 0);
		}else{
			usb_set(S3C2410_UDC_ICSR1_PKTRDY, ep->csr1, ep_index(ep));
		}
		done(ep, req, 0);
		return 1;
	} else{
		usb_set(S3C2410_UDC_ICSR1_PKTRDY, ep->csr1, ep_index(ep));
	}

	DEBUG_EP0("%s: wrote %s %d bytes%s%s %d left %p\n", __FUNCTION__,
	      ep->ep.name, count,
	      is_last ? "/L" : "", is_short ? "/S" : "",
	      req->req.length - req->req.actual, req);

	return 0;
}

/** Read to request from FIFO (max read == bytes in fifo)
 *  Return:  0 = still running, 1 = completed, negative = errno
 */
static int read_fifo(struct elfin_ep *ep, struct elfin_request *req)
{
	u32 csr;
	u8 *buf;
	unsigned bufferspace, count, is_short;
	u32 fifo = ep->fifo;

	/* make sure there's a packet in the FIFO. */
	csr = usb_read(ep->csr1, ep_index(ep));
	if (!(csr & S3C2410_UDC_OCSR1_PKTRDY)) {
		DEBUG("%s: Packet NOT ready!\n", __FUNCTION__);
		return -EINVAL;
	}

	buf = req->req.buf + req->req.actual;
	prefetchw(buf);
	bufferspace = req->req.length - req->req.actual;

	/* read all bytes from this packet */
	count = (( (usb_read(S3C2410_UDC_OUT_FIFO_CNT2_REG, ep_index(ep)) & 0xff ) << 8) | (usb_read(S3C2410_UDC_OUT_FIFO_CNT1_REG, ep_index(ep)) & 0xff));
	req->req.actual += min(count, bufferspace);

	is_short = (count < ep->ep.maxpacket);
	DEBUG("read %s %02x, %d bytes%s req %p %d/%d\n",
	      ep->ep.name, csr, count,
	      is_short ? "/S" : "", req, req->req.actual, req->req.length);

	while (likely(count-- != 0)) {
		u8 byte = (u8) __raw_readl(fifo);

		if (unlikely(bufferspace == 0)) {
			/* this happens when the driver's buffer
			 * is smaller than what the host sent.
			 * discard the extra data.
			 */
			if (req->req.status != -EOVERFLOW)
				printk("%s overflow %d\n", ep->ep.name, count);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = byte;
			bufferspace--;
		}
	}

	usb_clear(S3C2410_UDC_OCSR1_PKTRDY, ep->csr1, ep_index(ep));

	/* completion */
	if (is_short || req->req.actual == req->req.length) {
		done(ep, req, 0);
		return 1;
	}

	/* finished that packet.  the next one may be waiting... */
	return 0;
}

/*
 *	done - retire a request; caller blocked irqs
 */
static void done(struct elfin_ep *ep, struct elfin_request *req, int status)
{
	unsigned int stopped = ep->stopped;

	DEBUG("%s, %p\n", __FUNCTION__, ep);
	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN)
		DEBUG("complete %s req %p stat %d len %u/%u\n",
		      ep->ep.name, &req->req, status,
		      req->req.actual, req->req.length);

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;

	spin_unlock(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	spin_lock(&ep->dev->lock);

	ep->stopped = stopped;
}

/*
 * 	nuke - dequeue ALL requests
 */
void nuke(struct elfin_ep *ep, int status)
{
	struct elfin_request *req;

	DEBUG("%s, %p\n", __FUNCTION__, ep);

	/* called with irqs blocked */
	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct elfin_request, queue);
		done(ep, req, status);
	}
}

/**
 * elfin_in_epn - handle IN interrupt
 */
static void elfin_in_epn(struct elfin_udc *dev, u32 ep_idx)
{
	u32 csr;
	struct elfin_ep *ep = &dev->ep[ep_idx];
	struct elfin_request *req;

	csr = usb_read(ep->csr1, ep_index(ep));
	DEBUG("%s: %d, csr %x\n", __FUNCTION__, ep_idx, csr);

	if (csr & S3C2410_UDC_ICSR1_SENTSTL) {
		printk("S3C2410_UDC_ICSR1_SENTSTL\n");
		usb_set(S3C2410_UDC_ICSR1_SENTSTL /*| S3C2410_UDC_ICSR1_SENDSTL */ ,
			ep->csr1, ep_index(ep));
		return;
	}

	if (!ep->desc) {
		DEBUG("%s: NO EP DESC\n", __FUNCTION__);
		return;
	}

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next, struct elfin_request, queue);

	DEBUG("req: %p\n", req);

	if (!req)
		return;

	write_fifo(ep, req);
}

/* ********************************************************************************************* */
/* Bulk OUT (recv)
 */

static void elfin_out_epn(struct elfin_udc *dev, u32 ep_idx)
{
	struct elfin_ep *ep = &dev->ep[ep_idx];
	struct elfin_request *req;

	DEBUG("%s: %d\n", __FUNCTION__, ep_idx);

	if (ep->desc) {
		u32 csr;
		csr = usb_read(ep->csr1, ep_index(ep));

		while ((csr = usb_read(ep->csr1, ep_index(ep))) 
				& (S3C2410_UDC_OCSR1_PKTRDY | S3C2410_UDC_OCSR1_SENTSTL)) {

			DEBUG("%s: %x\n", __FUNCTION__, csr);

			if (csr & S3C2410_UDC_OCSR1_SENTSTL) {
				DEBUG("%s: stall sent, flush fifo\n",
				      __FUNCTION__);
			/* usb_set(USB_OUT_CSR1_FIFO_FLUSH, ep->csr1, ep_index(ep)); */
			} else if (csr & S3C2410_UDC_OCSR1_PKTRDY) {
				if (list_empty(&ep->queue))
					req = 0;
				else
					req = list_entry(ep->queue.next,
						       struct elfin_request, queue);

				if (!req) {
					printk("%s: NULL REQ %d\n",
					       __FUNCTION__, ep_idx);
					break;
				} else {
					read_fifo(ep, req);
				}
			}
		}
	} else {
		/* Throw packet away.. */
		printk("%s: No descriptor?!?\n", __FUNCTION__);
	}
}

static void stop_activity(struct elfin_udc *dev,
			  struct usb_gadget_driver *driver)
{
	int i;

	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = 0;
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < S3C2410_MAX_ENDPOINTS; i++) {
		struct elfin_ep *ep = &dev->ep[i];
		ep->stopped = 1;
		nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	if (driver) {
		spin_unlock(&dev->lock);
		driver->disconnect(&dev->gadget);
		spin_lock(&dev->lock);
	}

	/* re-init driver-visible data structures */
	udc_reinit(dev);
}

static void reconfig_usbd(void)
{
	u32 val;

	usb_write(0, S3C2410_UDC_PWR_REG, 0);

	/* EP0 */
	val = EP0_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 0);

	val = (1<<6)|(1<<7);	//EP0_CSR_SOPKTRDY | EP0_CSR_SSE;
	usb_write(val, S3C2410_UDC_EP0_CSR_REG, 0);

	/* EP3 -- INT */
	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 3);

	val = (1<<6)|(1<<3);	//UD_ICSR1_FFLUSH | UD_ICSR1_CLRDT;
	usb_write(val, S3C2410_UDC_IN_CSR1_REG, 3);

	//val = (1<<5)|(1<<4);	//UD_ICSR2_MODEIN | UD_ICSR2_DMAIEN; jassi
	val = (1<<5);	//UD_ICSR2_MODEIN; 
	usb_write(val, S3C2410_UDC_IN_CSR2_REG, 3);

	/* EP2 -- IN */
	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 2);

	val = (1<<6)|(1<<3);	//UD_ICSR1_FFLUSH | UD_ICSR1_CLRDT;
	usb_write(val, S3C2410_UDC_IN_CSR1_REG, 2);

	//val = (1<<5)|(1<<4);	//UD_ICSR2_MODEIN | UD_ICSR2_DMAIEN; jassi
	val = (1<<5);	//UD_ICSR2_MODEIN; 
	usb_write(val, S3C2410_UDC_IN_CSR2_REG, 2);

	/* EP1 -- OUT */
	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 1);

	val = (1<<6)|(1<<3);	//UD_ICSR1_FFLUSH | UD_ICSR1_CLRDT;
	usb_write(val, S3C2410_UDC_IN_CSR1_REG, 1);

	usb_write(0, S3C2410_UDC_IN_CSR2_REG, 1);	// output mode

	val = (1<<4)|(1<<7);	//UD_OCSR1_FFLUSH | UD_OCSR1_CLRDT;
	usb_write(val, S3C2410_UDC_OUT_CSR1_REG, 1);

	val = (1<<5);		//UD_OCSR2_DMAIEN;
	usb_write(val, S3C2410_UDC_OUT_CSR2_REG, 1);

	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 3);

	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 2);

	val = EP_FIFO_SIZE>>3;
	usb_write(val, S3C2410_UDC_MAXP_REG, 1);

	//val = (1<<0)|(1<<1)|(1<<2);	//UD_INTE_EP0 | UD_INTE_EP2 | UD_INTE_EP1;
	val = (1<<0)|(1<<1)|(1<<2)|(1<<3); //EP0 | EP1 | EP2 | EP3
	usb_write(val, S3C2410_UDC_EP_INT_EN_REG, 0);
	usb_write(val & ~(1<<0), S3C2410_UDC_EP_INT_REG, 0);	//clear

	val = (1<<0)|(1<<2);	//UD_USBINTE_RESET | UD_USBINTE_SUSPND;
	usb_write(val, S3C2410_UDC_USB_INT_EN_REG, 0);
	usb_write(val, S3C2410_UDC_USB_INT_REG, 0);	//clear
}

/*
 *	elfin usb client interrupt handler.
 */
static irqreturn_t elfin_udc_irq(int irq, void *_dev, struct pt_regs *r)
{
	struct elfin_udc *dev = _dev;

	spin_lock(&dev->lock);

	u32 intr_out, intr_in;
	u32 intr_int, val;

	intr_out = intr_in = usb_read(S3C2410_UDC_EP_INT_REG, 0);
	intr_int = usb_read(S3C2410_UDC_USB_INT_REG, 0);

	/* We have only 3 usable eps now 	-jassi */
	intr_in &= (S3C2410_UDC_INT_EP3 | S3C2410_UDC_INT_EP2 | S3C2410_UDC_INT_EP0);
	intr_out &= S3C2410_UDC_INT_EP1;

	if (!intr_out && !intr_in && !intr_int){
		spin_unlock(&dev->lock);
		return IRQ_HANDLED;
	}

	DEBUG("%s (on state %s)\n", __FUNCTION__,
	      state_names[dev->ep0state]);
	DEBUG("intr_out = %x\n", intr_out);
	DEBUG("intr_in  = %x\n", intr_in);
	DEBUG("intr_int = %x\n", intr_int);

	if (intr_in) {
		if (intr_in & S3C2410_UDC_INT_EP0){
			//printk("EP0 ");
			usb_write(S3C2410_UDC_INT_EP0, S3C2410_UDC_EP_INT_REG, 0);
			elfin_handle_ep0(dev);
		}else if (intr_in & S3C2410_UDC_INT_EP2){
			//printk("EP2 ");
			usb_write(S3C2410_UDC_INT_EP2, S3C2410_UDC_EP_INT_REG, 2);
			elfin_in_epn(dev, 2);	// hard coded !!!   -jassi
		}else{
			//printk("EP3 ");
			usb_write(S3C2410_UDC_INT_EP3, S3C2410_UDC_EP_INT_REG, 3);
			elfin_in_epn(dev, 3);	// hard coded !!!   -jassi
		}
	}

	if (intr_out) {
		//printk("EP1 ");
		usb_write(S3C2410_UDC_INT_EP1, S3C2410_UDC_EP_INT_REG, 1);
		elfin_out_epn(dev, 1);		// hard coded !!!   -jassi
	}

	if (intr_int) {
		if (intr_int & S3C2410_UDC_USBINT_RESET) {
			reconfig_usbd();
			dev->gadget.speed = USB_SPEED_FULL;
			dev->ep0state = WAIT_FOR_SETUP;
			usb_write(S3C2410_UDC_USBINT_RESET, S3C2410_UDC_USB_INT_REG, 0);
			DEBUG_SETUP("RESET");
		}

		usb_write(intr_int, S3C2410_UDC_USB_INT_REG, 0);

		if (intr_int & S3C2410_UDC_USBINT_RESUME) {
			DEBUG_SETUP("USB resume\n");

			if (dev->gadget.speed != USB_SPEED_UNKNOWN
			    && dev->driver
			    && dev->driver->resume) {
				dev->driver->resume(&dev->gadget);
			}
		}

		if (intr_int & S3C2410_UDC_USBINT_SUSPEND) {
			if (dev->gadget.speed !=
				   USB_SPEED_UNKNOWN && dev->driver
				   && dev->driver->suspend) {
				dev->driver->suspend(&dev->gadget);
			}
		}

	}

	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

static int elfin_ep_enable(struct usb_ep *_ep,
			     const struct usb_endpoint_descriptor *desc)
{
	struct elfin_ep *ep;
	struct elfin_udc *dev;
	unsigned long flags;

	DEBUG("%s, %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct elfin_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress
	    || ep_maxpacket(ep) < le16_to_cpu(desc->wMaxPacketSize)) {
		DEBUG("%s, bad ep or descriptor\n", __FUNCTION__);
		return -EINVAL;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if (ep->bmAttributes != desc->bmAttributes
	    && ep->bmAttributes != USB_ENDPOINT_XFER_BULK
	    && desc->bmAttributes != USB_ENDPOINT_XFER_INT) {
		DEBUG("%s, %s type mismatch\n", __FUNCTION__, _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
	     && le16_to_cpu(desc->wMaxPacketSize) != ep_maxpacket(ep))
	    || !desc->wMaxPacketSize) {
		DEBUG("%s, bad %s maxpacket\n", __FUNCTION__, _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {
		DEBUG("%s, bogus device state\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);

	ep->stopped = 0;
	ep->desc = desc;
	ep->pio_irqs = 0;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	/* Reset halt state */
	elfin_set_halt(_ep, 0);

	spin_unlock_irqrestore(&ep->dev->lock, flags);

	DEBUG("%s: enabled %s\n", __FUNCTION__, _ep->name);
	return 0;
}

/** Disable EP
 */
static int elfin_ep_disable(struct usb_ep *_ep)
{
	struct elfin_ep *ep;
	unsigned long flags;

	DEBUG("%s, %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct elfin_ep, ep);
	if (!_ep || !ep->desc) {
		DEBUG("%s, %s not enabled\n", __FUNCTION__,
		      _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* Nuke all pending requests */
	nuke(ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;

	spin_unlock_irqrestore(&ep->dev->lock, flags);

	DEBUG("%s: disabled %s\n", __FUNCTION__, _ep->name);
	return 0;
}

static struct usb_request *elfin_alloc_request(struct usb_ep *ep,
						 int gfp_flags)
{
	struct elfin_request *req;

	DEBUG("%s, %p\n", __FUNCTION__, ep);

	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;

	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void elfin_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct elfin_request *req;

	DEBUG("%s, %p\n", __FUNCTION__, ep);

	req = container_of(_req, struct elfin_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

static void *elfin_alloc_buffer(struct usb_ep *ep, unsigned bytes,
				  dma_addr_t * dma, int gfp_flags)
{
	char *retval;

	DEBUG("%s (%p, %d, %d)\n", __FUNCTION__, ep, bytes, gfp_flags);

	retval = kmalloc(bytes, gfp_flags & ~(__GFP_DMA | __GFP_HIGHMEM));
	if (retval)
		*dma = virt_to_bus(retval);
	return retval;
}

static void elfin_free_buffer(struct usb_ep *ep, void *buf, dma_addr_t dma,
				unsigned bytes)
{
	DEBUG("%s, %p\n", __FUNCTION__, ep);
	kfree(buf);
}

/** Queue one request
 *  Kickstart transfer if needed
 */
static int elfin_queue(struct usb_ep *_ep, struct usb_request *_req,
			 int gfp_flags)
{
	struct elfin_request *req;
	struct elfin_ep *ep;
	struct elfin_udc *dev;
	unsigned long flags;

	DEBUG("\n\n\n%s, %p\n", __FUNCTION__, _ep);

	req = container_of(_req, struct elfin_request, req);
	if (unlikely
	    (!_req || !_req->complete || !_req->buf
	     || !list_empty(&req->queue))) {
		DEBUG("%s, bad params\n", __FUNCTION__);
		return -EINVAL;
	}

	ep = container_of(_ep, struct elfin_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG("%s, bad ep\n", __FUNCTION__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DEBUG("%s, bogus device state %p\n", __FUNCTION__, dev->driver);
		return -ESHUTDOWN;
	}

	DEBUG("%s queue req %p, len %d buf %p\n", _ep->name, _req, _req->length,
	      _req->buf);

	spin_lock_irqsave(&dev->lock, flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* kickstart this i/o queue? */
	DEBUG("Add to %d Q %d %d\n", ep_index(ep), list_empty(&ep->queue),
	      ep->stopped);
	if (list_empty(&ep->queue) && likely(!ep->stopped)) {
		u32 csr;

		if (unlikely(ep_index(ep) == 0)) {
			/* EP0 */
			list_add_tail(&req->queue, &ep->queue);
			elfin_ep0_kick(dev, ep);
			req = 0;
		} else if (ep_is_in(ep)) {
			csr = usb_read(ep->csr1, ep_index(ep));
			if(!(csr & S3C2410_UDC_ICSR1_PKTRDY)&&(write_fifo(ep, req) == 1))
				req = 0;
		} else {
			csr = usb_read(ep->csr1, ep_index(ep));
			if (read_fifo(ep, req) == 1)
				req = 0;
		}
	}

	/* pio or dma irq handler advances the queue. */
	if (likely(req != 0))
		list_add_tail(&req->queue, &ep->queue);

	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/* dequeue JUST ONE request */
static int elfin_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct elfin_ep *ep;
	struct elfin_request *req;
	unsigned long flags;

	DEBUG("%s, %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct elfin_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		spin_unlock_irqrestore(&ep->dev->lock, flags);
		return -EINVAL;
	}

	done(ep, req, -ECONNRESET);

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

/** Halt specific EP
 *  Return 0 if success
 */
static int elfin_set_halt(struct usb_ep *_ep, int value)
{
	return 0;
}

/** Return bytes in EP FIFO
 */
static int elfin_fifo_status(struct usb_ep *_ep)
{
	u32 csr;
	int count = 0;
	struct elfin_ep *ep;

	ep = container_of(_ep, struct elfin_ep, ep);
	if (!_ep) {
		DEBUG("%s, bad ep\n", __FUNCTION__);
		return -ENODEV;
	}

	DEBUG("%s, %d\n", __FUNCTION__, ep_index(ep));

	/* LPD can't report unclaimed bytes from IN fifos */
	if (ep_is_in(ep))
		return -EOPNOTSUPP;

	csr = usb_read(ep->csr1, ep_index(ep));
	if (ep->dev->gadget.speed != USB_SPEED_UNKNOWN ||
	    csr & S3C2410_UDC_OCSR1_PKTRDY) {
		count = ( ((usb_read(S3C2410_UDC_OUT_FIFO_CNT2_REG, ep_index(ep)) & 0xff) << 8) 
			| (usb_read(S3C2410_UDC_OUT_FIFO_CNT1_REG, ep_index(ep)) & 0xff));
	}

	return count;
}

/** Flush EP FIFO
 */
static void elfin_fifo_flush(struct usb_ep *_ep)
{
	struct elfin_ep *ep;

	ep = container_of(_ep, struct elfin_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG("%s, bad ep\n", __FUNCTION__);
		return;
	}
}

/****************************************************************/
/* End Point 0 related functions                                */
/****************************************************************/

/* return:  0 = still running, 1 = completed, negative = errno */
static int write_fifo_ep0(struct elfin_ep *ep, struct elfin_request *req)
{
	u32 max;
	unsigned count;
	int is_last;

	max = ep_maxpacket(ep);

	DEBUG_EP0("%s\n", __FUNCTION__);

	count = write_packet(ep, req, max);

	/* last packet is usually short (or a zlp) */
	if (unlikely(count != max))
		is_last = 1;
	else {
		if (likely(req->req.length != req->req.actual) || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
	}

	DEBUG_EP0("%s: wrote %s %d bytes%s %d left %p\n", __FUNCTION__,
		  ep->ep.name, count,
		  is_last ? "/L" : "", req->req.length - req->req.actual, req);

	/* requests complete when all IN data is in the FIFO */
	if (is_last) {
		return 1;
	}

	return 0;
}

static __inline__ int elfin_fifo_read(struct elfin_ep *ep,
					unsigned char *cp, int max)
{
	int bytes;
	int count;
	u32 fifo = ep->fifo;

	count = ( ((usb_read(S3C2410_UDC_OUT_FIFO_CNT2_REG, ep_index(ep)) & 0xff ) << 8) 
			| (usb_read(S3C2410_UDC_OUT_FIFO_CNT1_REG, ep_index(ep)) & 0xff) );

	if (count != max){
		printk("count=%d max=%d\n", count, max);
		count = max;
	}
	bytes = count;
	while (count--)
		*cp++ = (u8) __raw_readl(fifo);

	return bytes;
}

static __inline__ void elfin_fifo_write(struct elfin_ep *ep,
					  unsigned char *cp, int count)
{
	u32 fifo = ep->fifo;
	DEBUG_EP0("fifo_write: %d %d\n", ep_index(ep), count);
	while (count--)
		__raw_writel(*cp++, fifo);
}

static int read_fifo_ep0(struct elfin_ep *ep, struct elfin_request *req)
{
	u32 csr;
	u8 *buf;
	unsigned bufferspace, count, is_short;
	u32 fifo = ep->fifo;

	DEBUG_EP0("%s\n", __FUNCTION__);

	csr = usb_read(S3C2410_UDC_EP0_CSR_REG, ep_index(ep));
	if (!(csr & S3C2410_UDC_OCSR1_PKTRDY))
		return 0;

	buf = req->req.buf + req->req.actual;
	prefetchw(buf);
	bufferspace = req->req.length - req->req.actual;

	/* read all bytes from this packet */
	if (likely(csr & S3C2410_UDC_EP0_CSR_OPKRDY)) {
		count = 8;	// I do !!!
		req->req.actual += min(count, bufferspace);
	} else	{		// zlp 
		count = 0;
	}

	is_short = (count < ep->ep.maxpacket);
	DEBUG_EP0("read %s %02x, %d bytes%s req %p %d/%d\n",
		  ep->ep.name, csr, count,
		  is_short ? "/S" : "", req, req->req.actual, req->req.length);

	while (likely(count-- != 0)) {
		u8 byte = (u8) __raw_readl(fifo);

		if (unlikely(bufferspace == 0)) {
			/* this happens when the driver's buffer
			 * is smaller than what the host sent.
			 * discard the extra data.
			 */
			if (req->req.status != -EOVERFLOW)
				DEBUG_EP0("%s overflow %d\n", ep->ep.name,
					  count);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = byte;
			bufferspace--;
		}
	}

	/* completion */
	if (is_short || req->req.actual == req->req.length) {
		return 1;
	}

	return 0;
}

/**
 * udc_set_address - set the USB address for this device
 * @address:
 *
 * Called from control endpoint function after it decodes a set address setup packet.
 */
static void udc_set_address(struct elfin_udc *dev, unsigned char address)
{
	DEBUG_EP0("%s: %d\n", __FUNCTION__, address);
	dev->usb_address = address;
	usb_set((1<<7) | (address & 0x7f), S3C2410_UDC_FUNC_ADDR_REG, 0);
}

/*
 * DATA_STATE_RECV (OUT_PKT_RDY)
 *      - if error
 *              set S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE | S3C2410_UDC_EP0_CSR_SENDSTL bits
 *      - else
 *              set S3C2410_UDC_EP0_CSR_SOPKTRDY bit
 				if last set S3C2410_UDC_EP0_CSR_DE bit
 */
static int first_time = 1;

static void elfin_ep0_read(struct elfin_udc *dev)
{
	struct elfin_request *req;
	struct elfin_ep *ep = &dev->ep[0];
	int ret;

	if (!list_empty(&ep->queue))
		req = list_entry(ep->queue.next, struct elfin_request, queue);
	else
		BUG();	//logic ensures		-jassi

	if(req->req.length == 0) {
		usb_set((S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE),
				 S3C2410_UDC_EP0_CSR_REG, 0);
		dev->ep0state = WAIT_FOR_SETUP;
		first_time = 1;
		done(ep, req, 0);
		return;
	}

	if(!req->req.actual && first_time){	//for SetUp packet
		usb_set(S3C2410_UDC_EP0_CSR_SOPKTRDY, S3C2410_UDC_EP0_CSR_REG, 0);
		first_time = 0;
		return;
	}

	ret = read_fifo_ep0(ep, req);
	if (ret) {
		usb_set((S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE),
					 S3C2410_UDC_EP0_CSR_REG, 0);
		dev->ep0state = WAIT_FOR_SETUP;
		first_time = 1;
		done(ep, req, 0);
		return;
	}
	usb_set(S3C2410_UDC_EP0_CSR_SOPKTRDY, S3C2410_UDC_EP0_CSR_REG, 0);
}

/*
 * DATA_STATE_XMIT
 */
static int elfin_ep0_write(struct elfin_udc *dev)
{
	struct elfin_request *req;
	struct elfin_ep *ep = &dev->ep[0];
	int ret, need_zlp = 0;

	DEBUG_EP0("%s: %x\n", __FUNCTION__);

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next, struct elfin_request, queue);

	if (!req) {
		DEBUG_EP0("%s: NULL REQ\n", __FUNCTION__);
		return 0;
	}

	if (req->req.length == 0) {	
		usb_set((S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_DE),
				 S3C2410_UDC_EP0_CSR_REG, 0);
		dev->ep0state = WAIT_FOR_SETUP;
	   	done(ep, req, 0);
		return 1;
	}

	if (req->req.length - req->req.actual == EP0_FIFO_SIZE) {
		/* Next write will end with the packet size, */
		/* so we need Zero-length-packet */
		need_zlp = 1;
	}

	ret = write_fifo_ep0(ep, req);

	if ((ret == 1) && !need_zlp) {
		/* Last packet */
		DEBUG_EP0("%s: finished, waiting for status\n", __FUNCTION__);

		usb_set((S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_DE), 
				S3C2410_UDC_EP0_CSR_REG, 0);
		dev->ep0state = WAIT_FOR_SETUP;
	} else {
		DEBUG_EP0("%s: not finished\n", __FUNCTION__);
		usb_set(S3C2410_UDC_EP0_CSR_IPKRDY, S3C2410_UDC_EP0_CSR_REG, 0);
	}

	if (need_zlp) {
		DEBUG_EP0("%s: Need ZLP!\n", __FUNCTION__);
		dev->ep0state = DATA_STATE_NEED_ZLP;
	}

	if(ret)
	   done(ep, req, 0);

	return 1;
}

#if 0
static int elfin_handle_get_status(struct elfin_udc *dev,
				     struct usb_ctrlrequest *ctrl)
{
	struct elfin_ep *ep0 = &dev->ep[0];
	struct elfin_ep *qep;
	int reqtype = (ctrl->bRequestType & USB_RECIP_MASK);
	u16 val = 0;

	if (reqtype == USB_RECIP_INTERFACE) {
		/* This is not supported.
		 * And according to the USB spec, this one does nothing..
		 * Just return 0
		 */
		DEBUG_SETUP("GET_STATUS: USB_RECIP_INTERFACE\n");
	} else if (reqtype == USB_RECIP_DEVICE) {
		DEBUG_SETUP("GET_STATUS: USB_RECIP_DEVICE\n");
		val |= (1 << 0);	/* Self powered */
		/*val |= (1<<1); *//* Remote wakeup */
	} else if (reqtype == USB_RECIP_ENDPOINT) {
		int ep_num = (ctrl->wIndex & ~USB_DIR_IN);

		DEBUG_SETUP
		    ("GET_STATUS: USB_RECIP_ENDPOINT (%d), ctrl->wLength = %d\n",
		     ep_num, ctrl->wLength);

		if (ctrl->wLength > 2 || ep_num > 3)
			return -EOPNOTSUPP;

		qep = &dev->ep[ep_num];
		if (ep_is_in(qep) != ((ctrl->wIndex & USB_DIR_IN) ? 1 : 0)
		    && ep_index(qep) != 0) {
			return -EOPNOTSUPP;
		}

		/* Return status on next IN token */
		switch (qep->ep_type) {
		case ep_control:
			val =
			    (usb_read(qep->csr1, ep_index(qep)) & S3C2410_UDC_EP0_CSR_SENDSTL) ==
			    S3C2410_UDC_EP0_CSR_SENDSTL;
			break;
		case ep_bulk_in:
		case ep_interrupt:
			val = (usb_read(qep->csr1, ep_index(qep))
				 & S3C2410_UDC_ICSR1_SENDSTL) == S3C2410_UDC_ICSR1_SENDSTL;
			break;
		case ep_bulk_out:
			val = (usb_read(qep->csr1, ep_index(qep)) 
				& S3C2410_UDC_OCSR1_SENDSTL) == S3C2410_UDC_OCSR1_SENDSTL;
			break;
		}

		/* Back to EP0 index */

		DEBUG_SETUP("GET_STATUS, ep: %d (%x), val = %d\n", ep_num,
			    ctrl->wIndex, val);
	} else {
		DEBUG_SETUP("Unknown REQ TYPE: %d\n", reqtype);
		return -EOPNOTSUPP;
	}

	/* Clear "out packet ready" */
	usb_set((S3C2410_UDC_EP0_CSR_SOPKTRDY), S3C2410_UDC_EP0_CSR_REG, 0);
	/* Put status to FIFO */
	elfin_fifo_write(ep0, (u8 *) & val, sizeof(val));
	/* Issue "In packet ready" */
	usb_set((S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_DE),
			 S3C2410_UDC_EP0_CSR_REG, 0);

	return 0;
}
#endif

/*
 * WAIT_FOR_SETUP (OUT_PKT_RDY)
 *      - read data packet from EP0 FIFO
 *      - decode command
 *      - if error
 *              set S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE | S3C2410_UDC_EP0_CSR_SENDSTL bits
 *      - else
 *              set S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE bits
 */
static void elfin_ep0_setup(struct elfin_udc *dev, u32 csr)
{
	struct elfin_ep *ep = &dev->ep[0];
	struct usb_ctrlrequest ctrl;
	int i, bytes, is_in;

	DEBUG_SETUP("%s: %x\n", __FUNCTION__, csr);

	/* Nuke all previous transfers */
	nuke(ep, -EPROTO);

	/* read control req from fifo (8 bytes) */
	bytes = elfin_fifo_read(ep, (unsigned char *)&ctrl, 8);
	DEBUG_SETUP("Read CTRL REQ %d bytes\n", bytes);
	DEBUG_SETUP("CTRL.bRequestType = 0x%x (is_in %d)\n", ctrl.bRequestType,
		    ctrl.bRequestType & USB_DIR_IN);
	DEBUG_SETUP("CTRL.bRequest = 0x%x\n", ctrl.bRequest);
	DEBUG_SETUP("CTRL.wLength = 0x%x\n", ctrl.wLength);
	DEBUG_SETUP("CTRL.wValue = 0x%x (%d)\n", ctrl.wValue, ctrl.wValue >> 8);
	DEBUG_SETUP("CTRL.wIndex = 0x%x\n", ctrl.wIndex);

	/* Set direction of EP0 */
	if (likely(ctrl.bRequestType & USB_DIR_IN)) {
		ep->bEndpointAddress |= USB_DIR_IN;
		is_in = 1;
	} else {
		ep->bEndpointAddress &= ~USB_DIR_IN;
		is_in = 0;
	}

	dev->req_pending = 1;

	/* Handle some SETUP packets ourselves */
	switch (ctrl.bRequest) {
	case USB_REQ_SET_ADDRESS:
		if (ctrl.bRequestType != (USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			break;

		DEBUG_SETUP("USB_REQ_SET_ADDRESS (%d)\n", ctrl.wValue);
		udc_set_address(dev, ctrl.wValue);
		usb_set((S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE), S3C2410_UDC_EP0_CSR_REG, 0);
		return;
#if 0
	case USB_REQ_GET_STATUS: 
		DEBUG_SETUP("USB_REQ_GET_STATUS \n");
			if (elfin_handle_get_status(dev, &ctrl) == 0)
				return;
#endif
	default:
		DEBUG_SETUP(" DFAULT\n");
		break;
	}

	if (likely(dev->driver)) {
		/* device-2-host (IN) or no data setup command, process immediately */
		spin_unlock(&dev->lock);
		i = dev->driver->setup(&dev->gadget, &ctrl);
		spin_lock(&dev->lock);

		if (i < 0) {
			/* setup processing failed, force stall */
			printk("  --> ERROR: gadget setup FAILED (stalling), \
				setup returned %d\n", i);
			usb_set((S3C2410_UDC_EP0_CSR_SOPKTRDY | S3C2410_UDC_EP0_CSR_DE 
				|S3C2410_UDC_EP0_CSR_SENDSTL), S3C2410_UDC_EP0_CSR_REG, 0);
			/* ep->stopped = 1; */
			dev->ep0state = WAIT_FOR_SETUP;
		}
	}
}

/*
 * DATA_STATE_NEED_ZLP
 */
static void elfin_ep0_in_zlp(struct elfin_udc *dev, u32 csr)
{
	DEBUG_EP0("%s: %x\n", __FUNCTION__, csr);

	usb_set((S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_DE), 
			S3C2410_UDC_EP0_CSR_REG, 0);
	dev->ep0state = WAIT_FOR_SETUP;
}

/*
 * handle ep0 interrupt
 */
static void elfin_handle_ep0(struct elfin_udc *dev)
{
	struct elfin_ep *ep = &dev->ep[0];
	u32 csr;

 	csr = usb_read(S3C2410_UDC_EP0_CSR_REG, ep_index(ep));

	DEBUG_EP0("%s: csr = %x\n", __FUNCTION__, csr);

	/*
	 * if SENT_STALL is set
	 *      - clear the SENT_STALL bit
	 */
	if (csr & S3C2410_UDC_EP0_CSR_SENTSTL) {
		printk("%s: S3C2410_UDC_EP0_CSR_SENTSTL is set: %x\n", __FUNCTION__, csr);
		usb_write(0, S3C2410_UDC_EP0_CSR_REG, 0);
		nuke(ep, -ECONNABORTED);
		dev->ep0state = WAIT_FOR_SETUP;
		return;
	}

	/*
	 * if SETUP_END is set
	 *      - abort the last transfer
	 *      - set SERVICED_SETUP_END_BIT
	 */
	if (csr & S3C2410_UDC_EP0_CSR_SE) {
		DEBUG_EP0("%s: S3C2410_UDC_EP0_CSR_SE is set: %x\n", __FUNCTION__, csr);

		usb_clear(0xc0, S3C2410_UDC_EP0_CSR_REG, 0);
		usb_set(S3C2410_UDC_EP0_CSR_SSE, S3C2410_UDC_EP0_CSR_REG, 0);

		nuke(ep, 0);
		dev->ep0state = WAIT_FOR_SETUP;
	}

	/*
	 * if a transfer is in progress && IN_PKT_RDY and OUT_PKT_RDY are clear
	 *      - fill EP0 FIFO
	 *      - if last packet
	 *      -       set IN_PKT_RDY | DATA_END
	 *      - else
	 *              set IN_PKT_RDY
	 */
	if (!(csr & (S3C2410_UDC_EP0_CSR_IPKRDY | S3C2410_UDC_EP0_CSR_OPKRDY))) {
		switch (dev->ep0state) {
		case DATA_STATE_XMIT:
			DEBUG_EP0("continue with DATA_STATE_XMIT\n");
			elfin_ep0_write(dev);
			return;
		case DATA_STATE_NEED_ZLP:
			DEBUG_EP0("continue with DATA_STATE_NEED_ZLP\n");
			elfin_ep0_in_zlp(dev, csr);
			return;
		default:
			/* Stall? */
			DEBUG_EP0("Odd state!! state = %s\n",
				  state_names[dev->ep0state]);
			dev->ep0state = WAIT_FOR_SETUP;
			/* nuke(ep, 0); */
			/* usb_set(S3C2410_UDC_EP0_CSR_SENDSTL, ep->csr1, ep_index(ep)); */
			break;
		}
	}

	/*
	 * if S3C2410_UDC_EP0_CSR_OPKRDY is set
	 *      - read data packet from EP0 FIFO
	 *      - decode command
	 *      - if error
	 *              set SERVICED_OUT_PKT_RDY | DATA_END bits | SEND_STALL
	 *      - else
	 *              set SERVICED_OUT_PKT_RDY | DATA_END bits
	 */
	if (csr & S3C2410_UDC_EP0_CSR_OPKRDY) {
		switch (dev->ep0state) {
		case WAIT_FOR_SETUP:
			DEBUG_EP0("WAIT_FOR_SETUP\n");
			elfin_ep0_setup(dev, csr);
			break;

		case DATA_STATE_RECV:
			DEBUG_EP0("DATA_STATE_RECV\n");
			elfin_ep0_read(dev);
			break;

		default:
			/* send stall? */
			DEBUG_EP0("strange state!! 2. send stall? state = %d\n",
				  dev->ep0state);
			break;
		}
	}
}

static void elfin_ep0_kick(struct elfin_udc *dev, struct elfin_ep *ep)
{
	if (ep_is_in(ep)) {
		/* Clear "out packet ready" */
		usb_set(S3C2410_UDC_EP0_CSR_SOPKTRDY, S3C2410_UDC_EP0_CSR_REG, 0);
		udelay(5);	// time to settle things down

		dev->ep0state = DATA_STATE_XMIT;
		elfin_ep0_write(dev);
	} else {
		dev->ep0state = DATA_STATE_RECV;
		elfin_ep0_read(dev);
	}
}

/* ---------------------------------------------------------------------------
 * 	device-scoped parts of the api to the usb controller hardware
 * ---------------------------------------------------------------------------
 */

static int elfin_udc_get_frame(struct usb_gadget *_gadget)
{
	u32 frame1 = usb_read(S3C2410_UDC_FRAME_NUM1_REG, 0);/* Least significant 8 bits */
	u32 frame2 = usb_read(S3C2410_UDC_FRAME_NUM1_REG, 0);/* Most significant 3 bits */
	DEBUG("%s, %p\n", __FUNCTION__, _gadget);
	return ((frame2 & 0x07) << 8) | (frame1 & 0xff);
}

static int elfin_udc_wakeup(struct usb_gadget *_gadget)
{
	return -ENOTSUPP;
}

static const struct usb_gadget_ops elfin_udc_ops = {
	.get_frame = elfin_udc_get_frame,
	.wakeup = elfin_udc_wakeup,
	/* current versions must always be self-powered */
};

static void nop_release(struct device *dev)
{
	DEBUG("%s %s\n", __FUNCTION__, dev->bus_id);
}

static struct elfin_udc memory = {
	.usb_address = 0,

	.gadget = {
		   .ops = &elfin_udc_ops,
		   .ep0 = &memory.ep[0].ep,
		   .name = driver_name,
		   .dev = {
			   .bus_id = "gadget",
			   .release = nop_release,
			   },
		   },

	/* control endpoint */
	.ep[0] = {
		  .ep = {
			 .name = ep0name,
			 .ops = &elfin_ep_ops,
			 .maxpacket = EP0_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 0,
		  .bmAttributes = 0,

		  .ep_type = ep_control,
		  .fifo = S3C2410_UDC_EP0_FIFO_REG,
		  .csr1 = S3C2410_UDC_EP0_CSR_REG,
		  .csr2 = S3C2410_UDC_EP0_CSR_REG,
		  },

	/* first group of endpoints */
	.ep[1] = {
		  .ep = {
			 .name = "ep1-bulk",
			 .ops = &elfin_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 1,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .ep_type = ep_bulk_out,
		  .fifo = S3C2410_UDC_EP1_FIFO_REG,
		  .csr1 = S3C2410_UDC_OUT_CSR1_REG,
		  .csr2 = S3C2410_UDC_OUT_CSR2_REG,
		  },

	.ep[2] = {
		  .ep = {
			 .name = "ep2-bulk",
			 .ops = &elfin_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 2,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .ep_type = ep_bulk_in,
		  .fifo = S3C2410_UDC_EP2_FIFO_REG,
		  .csr1 = S3C2410_UDC_IN_CSR1_REG,
		  .csr2 = S3C2410_UDC_IN_CSR2_REG,
		  },

	.ep[3] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep3-int",
			 .ops = &elfin_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 3,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = S3C2410_UDC_EP3_FIFO_REG,
		  .csr1 = S3C2410_UDC_IN_CSR1_REG,
		  .csr2 = S3C2410_UDC_IN_CSR2_REG,
		  },
	.ep[4] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep4-int",
			 .ops = &elfin_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 4,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = S3C2410_UDC_EP4_FIFO_REG,
		  .csr1 = S3C2410_UDC_IN_CSR1_REG,
		  .csr2 = S3C2410_UDC_IN_CSR2_REG,
		  },
};

/*
 * 	probe - binds to the platform device
 */
static int elfin_udc_probe(struct device *_dev)
{
	struct elfin_udc *dev = &memory;
	int retval;

	DEBUG("%s: %p\n", __FUNCTION__, _dev);

	spin_lock_init(&dev->lock);
	dev->dev = _dev;

	device_initialize(&dev->gadget.dev);
	dev->gadget.dev.parent = _dev;

	dev->gadget.is_dualspeed = 1;	// Hack only
	dev->gadget.is_otg = 0;
	dev->gadget.is_a_peripheral = 0;
	dev->gadget.b_hnp_enable = 0;
	dev->gadget.a_hnp_support = 0;
	dev->gadget.a_alt_hnp_support = 0;

	the_controller = dev;
	dev_set_drvdata(_dev, dev);

	udc_reinit(dev);

	local_irq_disable();

	/* irq setup after old hardware state is cleaned up */
	retval =
	    request_irq(IRQ_USBD, elfin_udc_irq, SA_INTERRUPT, driver_name,
			dev);
	if (retval != 0) {
		DEBUG(KERN_ERR "%s: can't get irq %i, err %d\n", driver_name,
		      IRQ_USBD, retval);
		return -EBUSY;
	}

	disable_irq(IRQ_USBD);

	udc_enable(dev);
	reconfig_usbd();

	local_irq_enable();
	create_proc_files();

	return retval;
}

static int elfin_udc_remove(struct device *_dev)
{
	struct elfin_udc *dev = _dev->driver_data;

	DEBUG("%s: %p\n", __FUNCTION__, dev);

	udc_disable(dev);
	remove_proc_files();
	usb_gadget_unregister_driver(dev->driver);

	free_irq(IRQ_USBD, dev);

	dev_set_drvdata(_dev, 0);

	the_controller = 0;

	return 0;
}

/*-------------------------------------------------------------------------*/

static struct device_driver udc_driver = {
	.name = "s3c2410_udc",
	.bus = &platform_bus_type,
	.probe = elfin_udc_probe,
	.remove = elfin_udc_remove
};

static int __init udc_init(void)
{
	int ret;

	ret = driver_register(&udc_driver);
	if(!ret)
	   printk("Loaded %s version %s\n", driver_name, DRIVER_VERSION);

	return ret;
}

static void __exit udc_exit(void)
{
	driver_unregister(&udc_driver);
	printk("Unloaded %s version %s\n", driver_name, DRIVER_VERSION);
}

module_init(udc_init);
module_exit(udc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");
