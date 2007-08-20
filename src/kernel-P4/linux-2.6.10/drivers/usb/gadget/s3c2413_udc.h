/*
 * linux/drivers/usb/gadget/elfin_udc.h
 */

#ifndef __elfin_H_
#define __elfin_H_

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/hardware.h>
#include <asm/hardware/clock.h>
#include <asm/arch/regs-udc.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-clock.h>

#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>

// Max packet size
#define EP0_FIFO_SIZE		8
#define EP_FIFO_SIZE		64

#define S3C2413_MAX_ENDPOINTS	5

#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4

/* ********************************************************************************************* */
/* IO
 */

typedef enum ep_type {
	ep_control, ep_bulk_in, ep_bulk_out, ep_interrupt
} ep_type_t;

struct elfin_ep {
	struct usb_ep ep;
	struct elfin_udc *dev;

	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	unsigned long pio_irqs;

	u8 stopped;
	u8 bEndpointAddress;
	u8 bmAttributes;

	ep_type_t ep_type;
	u32 fifo;
	u32 csr1;
	u32 csr2;
};

struct elfin_request {
	struct usb_request req;
	struct list_head queue;
};

struct elfin_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct device *dev;
	spinlock_t lock;

	int ep0state;
	struct elfin_ep ep[S3C2413_MAX_ENDPOINTS];

	unsigned char usb_address;

	unsigned req_pending:1, req_std:1, req_config:1;
};

extern struct elfin_udc *the_controller;

#define ep_is_in(EP) 		(((EP)->bEndpointAddress&USB_DIR_IN)==USB_DIR_IN)
#define ep_index(EP) 		((EP)->bEndpointAddress&0xF)
#define ep_maxpacket(EP) 	((EP)->ep.maxpacket)

#endif
