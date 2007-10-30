/*
 * USB Driver
 * 
 * Based on linux/drivers/usb/usb-skeleton.c (Thanks Greg)
 *
 * Copyright (c) 2001 Greg Kroah-Hartman (greg@kroah.com)
 *
 * Copyright (c) 2002-2003 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Revision History:
 *
 * 2002-05-XX Seungbum Lim <shawn@mizi.com>
 * initial release for Linuette
 * 
 * 2002-08-27 Chan Gyun Jeong <cgjeong@mizi.com>
 * add /proc/driver/usb-smlinux for connect/disconnect notifications
 *
 * 2002-09-18 Chan Gyun Jeong <cgjeong@mizi.com>
 * catch up with usb-skeleton 0.7
 *
 * 2003-02-07 Chan Gyun Jeong <cgjeong@mizi.com>
 * add wait queue at writing bulk packets instead of returning error, 
 * thanks to pain and hwang
 *
 * 2003-03-03 Chan Gyun Jeong <cgjeong@mizi.com>
 * fix the bug which causes data loss when closing the device.
 * remove gcc 3.2 compiler warnings, clean up and version 0.2
 *
 * 2003-03-18 Chan Gyun Jeong <cgjeong@mizi.com>
 * adjust the maximum number of the devices to be attached and 
 * support for multiple /proc files
 *
 */

#include <linux/config.h>
//#ifdef CONFIG_MODVERSIONS
//#define MODVERSIONS
//#include <linux/modversions.h>
//#endif
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/usb.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif

/* Use our own dbg macro */
#undef dbg
#ifdef CONFIG_USB_DEBUG
#define dbg(arg...) \
	do { \
		printk("%s: ", __FILE__); \
		printk(arg); \
		printk("\n"); \
	} while (0)
#else
#define dbg(arg...) do { } while (0);
#endif

/* Version Information */
#define DRIVER_VERSION 	"v0.1"
#define DRIVER_AUTHOR 	"SEC"
#define DRIVER_DESC 	"USB Character Host Driver"

/* Define these values to match your device */
#define USB_ITSY_VENDOR_ID		0x049f          /* Compaq Itsy */
#define USB_ITSY_PRODUCT_ID		0x505A

#define USB_I519_VENDOR_ID		0x04e8          /* SPH-i519 */
#define USB_I519_PRODUCT_ID		0x661d

#define USB_CHAR_DEV_CLASS		0xff
#define USB_CHAR_DEV_SUBCLASS		0xff
#define USB_CHAR_DEV_PROTOCOL		0x01

#define MY_USB_DEVICE(vend,prod) \
	match_flags: USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_INFO, \
	idVendor: (vend), \
	idProduct: (prod), \
	bDeviceClass: (USB_CHAR_DEV_CLASS), \
	bDeviceSubClass: (USB_CHAR_DEV_SUBCLASS), \
	bDeviceProtocol: (USB_CHAR_DEV_PROTOCOL)

/* table of devices that work with this driver */
static struct usb_device_id smlinux_table [] = {
	//{ MY_USB_DEVICE(USB_ITSY_VENDOR_ID, USB_ITSY_PRODUCT_ID) },
	{ USB_DEVICE(USB_ITSY_VENDOR_ID, USB_ITSY_PRODUCT_ID) },
//	{ MY_USB_DEVICE(USB_I519_VENDOR_ID, USB_I519_PRODUCT_ID) },
	{ }					/* Terminating entry */
};

#ifdef MODULE_DEVICE_TABLE
MODULE_DEVICE_TABLE (usb, smlinux_table);
#endif



/* Get a minor range for your devices from the usb maintainer */
#define USB_SMLINUX_MINOR_BASE	200	
//#define USB_SMLINUX_MINOR_BASE	192	

/* we can have up to this number of device plugged in at once */
#define MAX_DEVICES		8

/* Structure to hold all of our device specific stuff */
struct usb_smlinux {
	struct usb_device *	udev;			/* save off the usb device pointer */
	struct usb_interface *	interface;		/* the interface for this device */
	devfs_handle_t		devfs;			/* devfs device node */
	unsigned char		minor;			/* the starting minor number for this device */

	unsigned char *		bulk_in_buffer;		/* the buffer to receive data */
	int			bulk_in_size;		/* the size of the receive buffer */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */

	unsigned char *		bulk_out_buffer;	/* the buffer to send data */
	int			bulk_out_size;		/* the size of the send buffer */
	struct urb *		write_urb;		/* the urb used to send data */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */

	struct tq_struct	tqueue;			/* task queue for line discipline waking up */
	int			open_count;		/* number of times this port has been opened */
	struct semaphore	sem;			/* locks this structure */
	wait_queue_head_t	write_wq;		/* wait queue for write */
};


/* the global usb devfs handle */
extern devfs_handle_t usb_devfs_handle;

static unsigned char pszMe[] = "usb-smlinux : ";

/* local function prototypes */
static ssize_t smlinux_read	(struct file *file, char *buffer, size_t count, loff_t *ppos);
static ssize_t smlinux_write	(struct file *file, const char *buffer, size_t count, loff_t *ppos);
static int smlinux_ioctl		(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int smlinux_open		(struct inode *inode, struct file *file);
static int smlinux_release		(struct inode *inode, struct file *file);
	
static void * smlinux_probe	(struct usb_device *dev, unsigned int ifnum, const struct usb_device_id *id);
static void smlinux_disconnect	(struct usb_device *dev, void *ptr);

static void smlinux_write_bulk_callback	(struct urb *urb);


/* array of pointers to our devices that are currently connected */
static struct usb_smlinux		*minor_table[MAX_DEVICES];

/* lock to protect the minor_table structure */
static DECLARE_MUTEX (minor_table_mutex);

/* file operations needed when we register this driver */
static struct file_operations smlinux_fops = {
	owner:		THIS_MODULE,
	read:		smlinux_read,
	write:		smlinux_write,
	ioctl:		smlinux_ioctl,
	open:		smlinux_open,
	release:	smlinux_release,
};      


/* usb specific object needed to register this driver with the usb subsystem */
static struct usb_driver smlinux_driver = {
	name:		"smlinux",
	probe:		smlinux_probe,
	disconnect:	smlinux_disconnect,
	fops:		&smlinux_fops,
	minor:		USB_SMLINUX_MINOR_BASE,
	id_table:	smlinux_table,
};



#ifdef CONFIG_USB_DEBUG
/**
 *	usb_smlinux_debug_data
 */
static inline void usb_smlinux_debug_data (const char *function, int size, const unsigned char *data)
{
	int i;

	if (!debug)
		return;
	
	printk (KERN_DEBUG __FILE__": %s - length = %d, data = ", 
		function, size);
	for (i = 0; i < size; ++i) {
		printk ("%.2x ", data[i]);
	}
	printk ("\n");
}
#endif


/**
 *	smlinux_delete
 */
static inline void smlinux_delete (struct usb_smlinux *dev)
{
	minor_table[dev->minor] = NULL;
	if (dev->bulk_in_buffer != NULL)
		kfree (dev->bulk_in_buffer);
	if (dev->bulk_out_buffer != NULL)
		kfree (dev->bulk_out_buffer);
	if (dev->write_urb != NULL)
		usb_free_urb (dev->write_urb);
	kfree (dev);
}


/**
 *	smlinux_open
 */
static int smlinux_open (struct inode *inode, struct file *file)
{
	struct usb_smlinux *dev = NULL;
	int subminor;
	int retval = 0;
	
	dbg(__FUNCTION__);

	subminor = MINOR (inode->i_rdev) - USB_SMLINUX_MINOR_BASE;
	if ((subminor < 0) ||
	    (subminor >= MAX_DEVICES)) {
		return -ENODEV;
	}

	/* increment our usage count for the module */
	MOD_INC_USE_COUNT;

	/* lock our minor table and get our local data for this minor */
	down (&minor_table_mutex);
	dev = minor_table[subminor];
	if (dev == NULL) {
		up (&minor_table_mutex);
		MOD_DEC_USE_COUNT;
		return -ENODEV;
	}

	/* lock this device */
	down (&dev->sem);

	/* unlock the minor table */
	up (&minor_table_mutex);

	/* increment our usage count for the driver */
	++dev->open_count;

	/* save our object in the file's private structure */
	file->private_data = dev;

	/* unlock this device */
	up (&dev->sem);

	return retval;
}


/**
 *	smlinux_release
 */
static int smlinux_release (struct inode *inode, struct file *file)
{
	struct usb_smlinux *dev;
	int retval = 0;

	dev = (struct usb_smlinux *)file->private_data;
	if (dev == NULL) {
		dbg ("%s - object is NULL", __FUNCTION__);
		return -ENODEV;
	}

	dbg("%s - minor %d", __FUNCTION__, dev->minor);

	/* lock our minor table */
	down (&minor_table_mutex);

	/* lock our device */
	down (&dev->sem);

	if (dev->open_count <= 0) {
		dbg ("%s - device not opened", __FUNCTION__);
		retval = -ENODEV;
		goto exit_not_opened;
	}

	if (dev->udev == NULL) {
		/* the device was unplugged before the file was released */
		up (&dev->sem);
		smlinux_delete (dev);
		up (&minor_table_mutex);
		MOD_DEC_USE_COUNT;
		return 0;
	}

	/* decrement our usage count for the device */
	--dev->open_count;
	if (dev->open_count <= 0) {
		/* shuting down ongoing bulk writes might cause data loss */
#if 0
		/* shutdown any bulk writes that might be going on */
		usb_unlink_urb (dev->write_urb);
#endif
		dev->open_count = 0;
	}

	/* decrement our usage count for the module */
	MOD_DEC_USE_COUNT;

exit_not_opened:
	up (&dev->sem);
	up (&minor_table_mutex);

	return retval;
}


/**
 *	smlinux_read
 */
static ssize_t smlinux_read (struct file *file, char *buffer, size_t count, loff_t *ppos)
{
	struct usb_smlinux *dev;
	int retval = 0;

	dev = (struct usb_smlinux *)file->private_data;
	
	dbg("%s - minor %d, count = %d", __FUNCTION__, dev->minor, count);

	/* lock this object */
	down (&dev->sem);

	/* verify that the device wasn't unplugged */
	if (dev->udev == NULL) {
		up (&dev->sem);
		return -ENODEV;
	}
	
	/* do an immediate bulk read to get data from the device */
	retval = usb_bulk_msg (dev->udev,
			       usb_rcvbulkpipe (dev->udev, 
						dev->bulk_in_endpointAddr),
			       dev->bulk_in_buffer, dev->bulk_in_size,
			       &count, HZ*10);

	/* if the read was successful, copy the data to userspace */
	if (!retval) {
		if (copy_to_user (buffer, dev->bulk_in_buffer, count))
			retval = -EFAULT;
		else
			retval = count;
	}
	
	/* unlock the device */
	up (&dev->sem);
	return retval;
}


/**
 *	smlinux_write
 */
static ssize_t smlinux_write (struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
	struct usb_smlinux *dev;
	ssize_t bytes_written = 0;
	int retval = 0;
	DECLARE_WAITQUEUE(wait, current);

	dev = (struct usb_smlinux *)file->private_data;

	dbg("%s - minor %d, count = %d", __FUNCTION__, dev->minor, count);

	/* lock this object */
	down (&dev->sem);

	/* verify that the device wasn't unplugged */
	if (dev->udev == NULL) {
		retval = -ENODEV;
		goto exit;
	}

	/* verify that we actually have some data to write */
	if (count == 0) {
		dbg("%s - write request of 0 bytes", __FUNCTION__);
		goto exit;
	}

	/* see if we are already in the middle of a write */
	add_wait_queue(&dev->write_wq, &wait);
	set_current_state(TASK_INTERRUPTIBLE);
	if (dev->write_urb->status == -EINPROGRESS) {
		dbg ("%s - already writing", __FUNCTION__);
		if (file->f_flags & O_NONBLOCK) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&dev->write_wq, &wait);
			retval = -EAGAIN;
			goto exit;
		}
		schedule();
		if (signal_pending(current)) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&dev->write_wq, &wait);
			retval = -ERESTARTSYS;
			goto exit;
		}
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&dev->write_wq, &wait);

	/* we can only write as much as 1 urb will hold */
	bytes_written = (count > dev->bulk_out_size) ? 
				dev->bulk_out_size : count;

	/* copy the data from userspace into our urb */
	if (copy_from_user(dev->write_urb->transfer_buffer, buffer, 
			   bytes_written)) {
		retval = -EFAULT;
		goto exit;
	}

#ifdef CONFIG_USB_DEBUG
	usb_smlinux_debug_data (__FUNCTION__, bytes_written, 
			     dev->write_urb->transfer_buffer);
#endif

	/* set up our urb */
	FILL_BULK_URB(dev->write_urb, dev->udev, 
		      usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
		      dev->write_urb->transfer_buffer, bytes_written,
		      smlinux_write_bulk_callback, dev);

	/* send the data out the bulk port */
	retval = usb_submit_urb(dev->write_urb);
	if (retval) {
		err("%s - failed submitting write urb, error %d",
		    __FUNCTION__, retval);
	} else {
		retval = bytes_written;
	}

exit:
	/* unlock the device */
	up (&dev->sem);

	return retval;
}


/**
 *	smlinux_ioctl
 */
static int smlinux_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct usb_smlinux *dev;

	dev = (struct usb_smlinux *)file->private_data;

	/* lock this object */
	down (&dev->sem);

	/* verify that the device wasn't unplugged */
	if (dev->udev == NULL) {
		up (&dev->sem);
		return -ENODEV;
	}

	dbg("%s - minor %d, cmd 0x%.4x, arg %ld", __FUNCTION__,
	    dev->minor, cmd, arg);


	/* fill in your device specific stuff here */
	
	/* unlock the device */
	up (&dev->sem);
	
	/* return that we did not understand this ioctl call */
	return -ENOTTY;
}


/**
 *	smlinux_write_bulk_callback
 */
static void smlinux_write_bulk_callback (struct urb *urb)
{
	struct usb_smlinux *dev = (struct usb_smlinux *)urb->context;

	dbg("%s - minor %d", __FUNCTION__, dev->minor);

	wake_up_interruptible(&dev->write_wq);

	if ((urb->status != -ENOENT) && 
	    (urb->status != -ECONNRESET)) {
		dbg("%s - nonzero write bulk status received: %d",
		    __FUNCTION__, urb->status);
		return;
	}

	return;
}


#ifdef CONFIG_PROC_FS
static int usb_smlinux_read_proc(char *page, char **start, off_t off,
				  int count, int *eof, void *data)
{
	char *p = page;
	int len;
	struct usb_smlinux *dev = (struct usb_smlinux *)data;

	p += sprintf(p, "minor no.\t: %u\n"
			"bulk in size\t: %d\n"
			"bulk in ep adr\t: 0x%x\n"
			"bulk out size\t: %d\n"
			"bulk out ep adr\t: 0x%x\n"
			"open count\t: %d\n",
			dev->minor,
			dev->bulk_in_size, dev->bulk_in_endpointAddr,
			dev->bulk_out_size, dev->bulk_out_endpointAddr,
			dev->open_count);

	len = (p - page) - off;
	if (len < 0) len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif


/**
 *	smlinux_probe
 *
 *	Called by the usb core when a new device is connected that it thinks
 *	this driver might be interested in.
 */
static void * smlinux_probe(struct usb_device *udev, unsigned int ifnum, const struct usb_device_id *id)
{
	struct usb_smlinux *dev = NULL;
	struct usb_interface *interface;
	struct usb_interface_descriptor *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int minor;
	int buffer_size;
	int i;
	char name[10];

	/* select a "subminor" number (part of a minor number) */
	down (&minor_table_mutex);
	for (minor = 0; minor < MAX_DEVICES; ++minor) {
		if (minor_table[minor] == NULL)
			break;
	}
	if (minor >= MAX_DEVICES) {
		info ("Too many devices plugged in, can not handle this device.");
		goto exit;
	}

	/* allocate memory for our device state and intialize it */
	dev = kmalloc (sizeof(struct usb_smlinux), GFP_KERNEL);
	if (dev == NULL) {
		err ("Out of memory");
		goto exit;
	}
	memset (dev, 0x00, sizeof (*dev));
	minor_table[minor] = dev;

	interface = &udev->actconfig->interface[ifnum];

	init_MUTEX (&dev->sem);
	dev->udev = udev;
	dev->interface = interface;
	dev->minor = minor;

	init_waitqueue_head(&dev->write_wq);

	/* set up the endpoint information */
	/* check out the endpoints */
	iface_desc = &interface->altsetting[0];
	for (i = 0; i < iface_desc->bNumEndpoints; ++i) { /* 2 endpoints in case of gamepark board */
		endpoint = &iface_desc->endpoint[i];

		if ((endpoint->bEndpointAddress & USB_DIR_IN) &&   /* INT and BULK endpoint */
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			/* we found a bulk in endpoint */
			buffer_size = endpoint->wMaxPacketSize;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc (buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				err("Couldn't allocate bulk_in_buffer");
				goto error;
			}
			printk("%sBulk IN\n",pszMe);
			printk("  wMaxPacketSize : %d\n",dev->bulk_in_size);
			printk("  EndpointAddress : %u\n",dev->bulk_in_endpointAddr);
		}
		
		if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == 0x00) && /* OUT and BULK endpoint */
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			/* we found a bulk out endpoint */
			dev->write_urb = usb_alloc_urb(0);
			if (!dev->write_urb) {
				err("No free urbs available");
				goto error;
			}
#if 0
			buffer_size = endpoint->wMaxPacketSize;
#else
			buffer_size = PAGE_ALIGN(endpoint->wMaxPacketSize * 500);
#endif
			dev->bulk_out_size = buffer_size;
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_out_buffer = kmalloc (buffer_size, GFP_KERNEL);
			if (!dev->bulk_out_buffer) {
				err("Couldn't allocate bulk_out_buffer");
				goto error;
			}
			FILL_BULK_URB(dev->write_urb, udev, 
				      usb_sndbulkpipe(udev, 
						      endpoint->bEndpointAddress),
				      dev->bulk_out_buffer, buffer_size,
				      smlinux_write_bulk_callback, dev);
			 printk("%sBulk OUT\n",pszMe);
			 printk("  wMaxPacketSize : %d\n",dev->bulk_out_size);
			 printk("  EndpointAddress : %u\n",dev->bulk_out_endpointAddr);

		}
	}

	/* initialize the devfs node for this device and register it */
	sprintf(name, "smlinux%d", dev->minor);
	
	dev->devfs = devfs_register (usb_devfs_handle, name,
				     DEVFS_FL_DEFAULT, USB_MAJOR,
				     USB_SMLINUX_MINOR_BASE + dev->minor,
				     S_IFCHR | S_IRUSR | S_IWUSR | 
				     //S_IRGRP | S_IWGRP | S_IROTH, 
				     S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 
				     &smlinux_fops, NULL);

	/* let the user know what node this device is now attached to */
	info ("USB smlinux device now attached %d", dev->minor);
	goto exit;
	
error:
	smlinux_delete (dev);
	dev = NULL;

exit:
#ifdef CONFIG_PROC_FS
	{
		char procfile[25];
		sprintf(procfile, "driver/usb-smlinux%d", dev->minor);
		create_proc_read_entry (procfile, 0, NULL, 
					usb_smlinux_read_proc, (void *)dev);
	}
#endif
	up (&minor_table_mutex);
	return dev; // null??
}


/**
 *	smlinux_disconnect
 *
 *	Called by the usb core when the device is removed from the system.
 */
static void smlinux_disconnect(struct usb_device *udev, void *ptr)
{
	struct usb_smlinux *dev;
	int minor;

	dev = (struct usb_smlinux *)ptr;
	
	down (&minor_table_mutex);
	down (&dev->sem);
		
	minor = dev->minor;

#ifdef CONFIG_PROC_FS
	{
		char procfile[25];
		sprintf(procfile, "driver/usb-smlinux%d", minor);
		remove_proc_entry (procfile, NULL);
	}
#endif

	/* remove our devfs node */
	devfs_unregister(dev->devfs);

	/* if the device is not opened, then we clean up right now */
	if (!dev->open_count) {
		up (&dev->sem);
		smlinux_delete (dev);
	} else {
		dev->udev = NULL;
		up (&dev->sem);
	}

	info("USB smlinux #%d now disconnected", minor);
	up (&minor_table_mutex);
}



/**
 *	usb_smlinux_init
 */
static int __init usb_smlinux_init(void)
{
	int result;

	/* register this driver with the USB subsystem */
	result = usb_register(&smlinux_driver);
	if (result < 0) {
		err("USB Linu@: usb_register failed. Error number %d",
		    result);
		return -1;
	}

	info(DRIVER_DESC " " DRIVER_VERSION);
	return 0;
}


/**
 *	usb_smlinux_exit
 */
static void __exit usb_smlinux_exit(void)
{
	/* deregister this driver with the USB subsystem */
	usb_deregister(&smlinux_driver);
}


module_init (usb_smlinux_init);
module_exit (usb_smlinux_exit);

#ifdef MODULE_AUTHOR
MODULE_AUTHOR(DRIVER_AUTHOR);
#endif
#ifdef MODULE_DESCRIPTION
MODULE_DESCRIPTION(DRIVER_DESC);
#endif
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

