/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * (C) Copyright 2002 Hewlett-Packard Company
 * 
 * S3C2413 Bus Glue
 *
 * Based on fragments of previous driver by Rusell King et al.
 *
 * This file is licenced under the GPL.
 */
 
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/arch/hardware.h>
#include <asm/arch/regs-clock.h>

#ifndef CONFIG_ARCH_S3C2413 
#error "This file is S3C2413 bus glue.  CONFIG_S3C2413 must be defined."
#endif

extern int usb_disabled(void);

static void s3c2413_start_hc(struct platform_device *dev)
{
	u32 val;
	printk(KERN_DEBUG __FILE__
	       ": starting S3C2413 OHCI USB Controller\n");

	mdelay(1);

	val = __raw_readl(S3C2413_CLKCON);
	val |= S3C2413_CLKCON_USBH;
	__raw_writel(val, S3C2413_CLKCON);
	printk(KERN_DEBUG __FILE__
		   ": Clock to USB host has been enabled \n");
}

static void s3c2413_stop_hc(struct platform_device *dev)
{
	u32 clkcon;
	printk(KERN_DEBUG __FILE__
	       ": stopping S3C2413 OHCI USB Controller\n");
	clkcon = __raw_readl(S3C2413_CLKCON);
        clkcon &=  ~S3C2413_CLKCON_USBH;
	__raw_writel(clkcon, S3C2413_CLKCON);
}

static irqreturn_t usb_hcd_s3c2413_hcim_irq (int irq, void *__hcd, struct pt_regs * r)
{
	struct usb_hcd *hcd = __hcd;

	return usb_hcd_irq(irq, hcd, r);
}

void usb_hcd_s3c2413_remove (struct usb_hcd *, struct platform_device *);

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */


/**
 * usb_hcd_s3c2413_probe - initialize SA-1111-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_s3c2413_probe (const struct hc_driver *driver,
			  struct usb_hcd **hcd_out,
			  struct platform_device *dev)
{
	int retval;
	struct usb_hcd *hcd = 0;
	unsigned int *addr = NULL;

	if (!request_mem_region(dev->resource[0].start, 
				dev->resource[0].end - dev->resource[0].start + 1,
				 hcd_name)) {
		dbg("request_mem_region failed");
		return -EBUSY;
	}

	s3c2413_start_hc(dev);

	addr = ioremap(dev->resource[0].start, 
			dev->resource[0].end - dev->resource[0].start + 1);
	if (!addr) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err1;
        }

	hcd = driver->hcd_alloc ();
	if (hcd == NULL){
		dbg ("hcd_alloc failed");
		retval = -ENOMEM;
		goto err1;
	}

	if(dev->resource[1].flags != IORESOURCE_IRQ){
                pr_debug ("resource[1] is not IORESOURCE_IRQ");
                retval = -ENOMEM;
                goto err1;
        }

	hcd->driver = (struct hc_driver *) driver;
	hcd->description = driver->description;

	hcd->irq = dev->resource[1].start;
	hcd->regs = addr;
	hcd->self.controller = &dev->dev;

	retval = hcd_buffer_create (hcd);
	if (retval != 0) {
		dbg ("pool alloc fail");
		goto err1;
	}

	retval = request_irq (hcd->irq, usb_hcd_s3c2413_hcim_irq, SA_INTERRUPT,
			      hcd->description, hcd);
	if (retval != 0) {
		dbg("request_irq failed");
		retval = -EBUSY;
		goto err2;
	}

	printk ("%s (S3C2413) at 0x%p, irq %d\n",
		hcd->description, hcd->regs, hcd->irq);

	usb_bus_init (&hcd->self);
	hcd->self.op = &usb_hcd_operations;
	hcd->self.release = &usb_hcd_release;
	hcd->self.hcpriv = (void *) hcd;
	hcd->self.bus_name = "s3c2413-ahb";
	hcd->product_desc = "S3C2413 OHCI";

	INIT_LIST_HEAD (&hcd->dev_list);

	usb_register_bus (&hcd->self);

	if ((retval = driver->start (hcd)) < 0) 
	{
		usb_hcd_s3c2413_remove(hcd, dev);
		return retval;
	}

	*hcd_out = hcd;
	return 0;

 err2:
	hcd_buffer_destroy (hcd);
 err1:
	kfree(hcd);
	s3c2413_stop_hc(dev);
	release_mem_region(dev->resource[0].start, 
				dev->resource[0].end - dev->resource[0].start + 1);
	return retval;
}


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_s3c2413_remove - shutdown processing for SA-1111-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_s3c2413_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void usb_hcd_s3c2413_remove (struct usb_hcd *hcd, struct platform_device *dev)
{
	info ("remove: %s, state %x", hcd->self.bus_name, hcd->state);

	if (in_interrupt ())
		BUG ();

	hcd->state = USB_STATE_QUIESCING;

	dbg ("%s: roothub graceful disconnect", hcd->self.bus_name);
	usb_disconnect (&hcd->self.root_hub);

	hcd->driver->stop (hcd);
	hcd->state = USB_STATE_HALT;

	free_irq (hcd->irq, hcd);
	hcd_buffer_destroy (hcd);

	usb_deregister_bus (&hcd->self);

	s3c2413_stop_hc(dev);
	release_mem_region(dev->resource[0].start, 
				dev->resource[0].end - dev->resource[0].start + 1);
}

/*-------------------------------------------------------------------------*/

static int __devinit
ohci_s3c2413_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run (ohci)) < 0) {
		err ("can't start %s", ohci->hcd.self.bus_name);
		ohci_stop (hcd);
		return ret;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_s3c2413_hc_driver = {
	.description =		hcd_name,

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_s3c2413_start,
#ifdef	CONFIG_PM
	/* suspend:		ohci_s3c2413_suspend,  -- tbd */
	/* resume:		ohci_s3c2413_resume,   -- tbd */
#endif
	.stop =			ohci_stop,

	/*
         * memory lifecycle (except per-request)
         */
        .hcd_alloc =            ohci_hcd_alloc,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_USB_SUSPEND
	.hub_suspend =		ohci_hub_suspend,
	.hub_resume =		ohci_hub_resume,
#endif
};

/*-------------------------------------------------------------------------*/

static int ohci_hcd_s3c2413_drv_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = NULL;
	int ret;

	if (usb_disabled())
		return -ENODEV;

	ret = usb_hcd_s3c2413_probe(&ohci_s3c2413_hc_driver, &hcd, pdev);

	if (ret == 0)
		dev_set_drvdata(dev, hcd);

	return ret;
}

static int ohci_hcd_s3c2413_drv_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	usb_hcd_s3c2413_remove(hcd, pdev);

	dev_set_drvdata(dev, NULL);

	return 0;
}

static struct device_driver ohci_hcd_s3c2413_driver = {
	.name		= "s3c2413-ohci",
	.bus            = &platform_bus_type,
	.probe		= ohci_hcd_s3c2413_drv_probe,
	.remove		= ohci_hcd_s3c2413_drv_remove,
};

static int __init ohci_hcd_s3c2413_init (void)
{
	dbg (DRIVER_INFO " (S3C2413)");
	dbg ("block sizes: ed %d td %d",
		sizeof (struct ed), sizeof (struct td));

	return driver_register(&ohci_hcd_s3c2413_driver);
}

static void __exit ohci_hcd_s3c2413_cleanup (void)
{
	driver_unregister(&ohci_hcd_s3c2413_driver);
}

module_init (ohci_hcd_s3c2413_init);
module_exit (ohci_hcd_s3c2413_cleanup);
