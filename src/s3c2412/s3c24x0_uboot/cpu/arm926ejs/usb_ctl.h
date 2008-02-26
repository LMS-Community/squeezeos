/*
 *	Copyright (C)  Compaq Computer Corporation, 1998, 1999
 *	Copyright (C)  Extenex Corporation 2001
 *	Copyright (C)  MIZI Research Inc. 2002
 *
 *  usb_ctl.h
 *
 *  PRIVATE interface used to share info among components of the S3C2400 USB
 *  core: usb_ctl, usb_ep0, usb_recv and usb_send. Clients of the USB core
 *  should use s3c2400_usb.h.
 *
 *  bushi <bushi@mizi.com>
 */

#ifndef _USB_CTL_H
#define _USB_CTL_H

//#include <asm/dma.h>  /* dmach_t */
//#define TCNT	256

/*
 * These states correspond to those in the USB specification v1.0
 * in chapter 8, Device Framework.
 */
enum { USB_STATE_NOTATTACHED=0, USB_STATE_ATTACHED=1,USB_STATE_POWERED=2,
	   USB_STATE_DEFAULT=3, USB_STATE_ADDRESS=4, USB_STATE_CONFIGURED=5,
	   USB_STATE_SUSPENDED=6};

struct usb_stats_t {
	 unsigned long ep0_fifo_write_failures;
	 unsigned long ep0_bytes_written;
	 unsigned long ep0_fifo_read_failures;
	 unsigned long ep0_bytes_read;
};

struct usb_info_t
{
	 char * client_name;
	 //kwg dmach_t dmach_tx, dmach_rx;
	 int state;
	 unsigned char address;
	 struct usb_stats_t stats;
};

/* in usb_ctl.c */
extern struct usb_info_t usbd_info;


/* in usb_ep0.c */
extern unsigned int ep0_state;
#define EP0_STATE_IDLE		0
#define EP0_STATE_TRANSFER	1
#define EP0_STATE_RECEIVER	2

extern __u8 set_configuration;
extern __u8 set_interface;
extern __u8 device_status;
extern __u8 ep0_status;
extern __u8 ep_bulk_in_status;
extern __u8 ep_bulk_out_status;


/*
 * Function Prototypes
 */
enum { kError=-1, kEvSuspend=0, kEvReset=1,
	   kEvResume=2, kEvAddress=3, kEvConfig=4, kEvDeConfig=5 };
int usbctl_next_state_on_event( int event );

/* endpoint zero */
extern void ep0_reset(void);
extern void ep0_int_hndlr(void);

extern int  ep1_recv(void);
extern int  ep1_init(int chn);
extern void ep1_int_hndlr(int status);
extern void ep1_reset(void);
extern void ep1_stall(void);

extern void ep2_reset(void);
extern int  ep2_init(int chn);
extern void ep2_int_hndlr(int status);
extern void ep2_stall(void);


extern void ep2_dma_callback(void *buf_id, int size);
extern void ep1_dma_callback(void *buf_id, int size);

#endif /* _USB_CTL_H */
