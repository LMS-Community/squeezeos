#ifndef _USB_NET_USBLAN_H
#define _USB_NET_USBLAN_H 1

#define USB_NEW_COMPLETE_T
//#define USB_ALLOC_URB(packets) usb_alloc_urb(packets, GFP_KERNEL)
#define USB_ALLOC_URB(packets,mem_flags) usb_alloc_urb(packets,mem_flags)

#define FILL_BULK_URB(URB,DEV,PIPE,TRANSFER_BUFFER,BUFFER_LENGTH,COMPLETE,CONTEXT) \
        usb_fill_bulk_urb(URB,DEV,PIPE,TRANSFER_BUFFER,BUFFER_LENGTH,(usb_complete_t)COMPLETE,CONTEXT)

#define FILL_CONTROL_URB(URB,DEV,PIPE,SETUP,TRANSFER_BUFFER,BUFFER_LENGTH,COMPLETE,CONTEXT) \
        usb_fill_control_urb(URB,DEV,PIPE,SETUP,TRANSFER_BUFFER,BUFFER_LENGTH,(usb_complete_t)COMPLETE,CONTEXT)

#define USB_SUBMIT_URB(urb) usb_submit_urb(urb, 0)
#define USB_ALTSETTING(ifc,alt_index) &(ifc->altsetting[alt_index].desc)
#define USB_IFC2EP(ifc,index) &(container_of(ifc, struct usb_host_interface, desc)->endpoint[index].desc)
#define USB_IFC2HOST(ifc) container_of(ifc, struct usb_host_interface, desc)
#define USB_DEV2CONFIG(dev,index) &(dev->config[index].desc)
#define USB_CONFIG2IFC(cfg, index) (container_of(cfg, struct usb_host_config, desc)->interface[index])
#define USB_CONFIG2DESC(cfg) &(cfg->desc)
#define USB_PROBE_DELAYED_SET NULL
#define USB_PROBE_ERROR -1
#define USB_PROBE_SET(driver,proc) { \
    int __usb_probe_proc(struct usb_interface *intf, \
	   const struct usb_device_id *id){\
           struct usb_device *usbdev = NULL;\
           if(!intf) { return USB_PROBE_ERROR; }\
           usbdev = interface_to_usbdev(intf); \
           return(!((*proc)(usbdev,0,id)) ? USB_PROBE_ERROR :0);\
       }\
    driver->probe = __usb_probe_proc; \
}



//Missing (obsolete??) XXX
//static void usb_inc_dev_use(struct usb_device * dev) {}
#define usb_inc_dev_use(dev) usb_get_dev(dev)
#define usb_dec_dev_use(dev) usb_put_dev(dev)
/*----------------------------------------------------------------------------*
 * New USB Structures                                                         *
 *----------------------------------------------------------------------------*/

/*
 * urb->transfer_flags:
 */
#define USB_DISABLE_SPD		0x0001
//#define URB_SHORT_NOT_OK	USB_DISABLE_SPD
#define USB_ISO_ASAP		0x0002
#define USB_ASYNC_UNLINK	0x0008
#define USB_QUEUE_BULK		0x0010
#define USB_NO_FSBR		0x0020
#define USB_ZERO_PACKET		0x0040  // Finish bulk OUTs always with zero length packet
#define URB_NO_INTERRUPT	0x0080	/* HINT: no non-error interrupt needed */
					/* ... less overhead for QUEUE_BULK */
#define USB_TIMEOUT_KILLED	0x1000	// only set by HCD!

/*
 * USB-status codes:
 * USB_ST* maps to -E* and should go away in the future
 */

#define USB_ST_NOERROR		0
#define USB_ST_CRC		(-EILSEQ)
#define USB_ST_BITSTUFF		(-EPROTO)
#define USB_ST_NORESPONSE	(-ETIMEDOUT)			/* device not responding/handshaking */
#define USB_ST_DATAOVERRUN	(-EOVERFLOW)
#define USB_ST_DATAUNDERRUN	(-EREMOTEIO)
#define USB_ST_BUFFEROVERRUN	(-ECOMM)
#define USB_ST_BUFFERUNDERRUN	(-ENOSR)
#define USB_ST_INTERNALERROR	(-EPROTO) 			/* unknown error */
#define USB_ST_SHORT_PACKET	(-EREMOTEIO)
#define USB_ST_PARTIAL_ERROR	(-EXDEV)			/* ISO transfer only partially completed */
#define USB_ST_URB_KILLED	(-ENOENT)			/* URB canceled by user */
#define USB_ST_URB_PENDING	(-EINPROGRESS)
#define USB_ST_REMOVED		(-ENODEV) 			/* device not existing or removed */
#define USB_ST_TIMEOUT		(-ETIMEDOUT)			/* communication timed out, also in urb->status**/
#define USB_ST_NOTSUPPORTED	(-ENOSYS)
#define USB_ST_BANDWIDTH_ERROR	(-ENOSPC)			/* too much bandwidth used */
#define USB_ST_URB_INVALID_ERROR  (-EINVAL)			/* invalid value/transfer type */
#define USB_ST_URB_REQUEST_ERROR  (-ENXIO)			/* invalid endpoint */
#define USB_ST_STALL		(-EPIPE) 			/* pipe stalled, also in urb->status*/

#endif
