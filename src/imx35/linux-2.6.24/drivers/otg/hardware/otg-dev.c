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
 * otg/hardware/otg-dev.c -- Generic DEV driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/otglib/otg-dev.c|20070918212346|28546
 *
 *      Copyright (c) 2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/otg-dev.c
 * @brief Generic DEV Driver.
 *
 * This supports using a single DEV device to implement various USB OTG
 * subsidiary drivers.
 *
 * The hardware driver is split into drivers for the following.
 *
 * Required:
 *
 *      1. dev
 *
 * One or more of the following:
 *
 *      2. ocd
 *      3. pcd
 *      4. hcd
 *      5. tcd
 *
 * The dev driver implements a standard dev driver structure which will
 * be used to register with the linux dev support. During the probe function
 * the otg_dev_probe() function should be called to create an otg_dev
 * structure for the device. This is used to keep track of all of the additional
 * drivers.
 *
 * Each of the optional drivers will use otg_dev_register_driver() to let the
 * dev layer know what optional drivers are available.
 *
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

//#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>

#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */

#include <otg/otg-trace.h>
#include <otg/usbp-chap9.h>
#include <otg/otg-api.h>
#include <otg/otg-dev.h>
#include <otg/otg-utils.h>
#include <otg/otg-hcd.h>
#include <otg/otg-ocd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-tcd.h>


#define TRACE_VERBOSE 0

/* ********************************************************************************************* */
static struct otg_dev       *otg_devs;

/* ********************************************************************************************* */
/*!
 * otg_dev_isr() - interrupt service handler
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
irqreturn_t otg_dev_isr(int irq, void *data, struct pt_regs *r)
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
irqreturn_t otg_dev_isr(int irq, void *data)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
{
        struct otg_interrupt    *otg_interrupt; // = data;
        struct device           *device; // = otg_interrupt->device;
        struct otg_dev          *otg_dev; // = dev_get_drvdata(device);
        struct otg_instance     *otg; // = otg_dev->otg_instance;
        int                     irqmask; // = 1 << otg_interrupt->irq;
        int                     i;

        otg_interrupt = data;
        device = otg_interrupt->device;
        otg_dev = dev_get_drvdata(device);
        irqmask = 1 << otg_interrupt->irq;

        // XXX need to determine what pt_regs *r can tell us, it may
        // allow sub-driver isr functions to tell if they should do something
        // so we may want to send that down

        /* XXX spinlock */

        RETURN_IRQ_HANDLED_UNLESS(otg_dev);
        otg = otg_dev->otg_instance;
        otg->interrupts++;

        if (TRACE_VERBOSE)
                TRACE_MSG0(otg_dev->DEV, "---------------------------------------- Start");

        for (i = OTG_DRIVER_TCD; i < OTG_DRIVER_TYPES; i++) {
                struct otg_dev_driver *otg_dev_driver = otg_dev->otg_device_driver->drivers[i];

                CONTINUE_UNLESS(otg_dev_driver);
                CONTINUE_UNLESS(otg_dev_driver->isr);
                CONTINUE_UNLESS(otg_dev_driver->irqs & irqmask);
                RETURN_IRQ_HANDLED_IF_IRQ_HANDLED (otg_dev_driver->isr(otg_dev, data, irqmask));
                //TRACE_MSG2(otg_dev->DEV, "not handled by %s %d", otg_dev_driver->name, i);
        }

        /* XXX spinlock */
        TRACE_MSG0(otg_dev->DEV, "---------------------------------------- IRQ_NONE ----");
        return IRQ_NONE;
}

/* ********************************************************************************************* */

/*!
 * otg_dev_free_dev
 */
void otg_dev_free_dev(struct device *device, struct otg_dev *otg_dev)
{
        #if 0
        int                     region;
        unsigned long           resource_start;
        unsigned long           resource_len;

        RETURN_UNLESS(otg_dev);

        for (region = 0; region < DEVICE_COUNT_RESOURCE; region++) {

                CONTINUE_UNLESS(otg_dev->pci_regions & (1 << region));

                if (otg_dev->regs[region]) iounmap(otg_dev->regs[region]);

                resource_start = pci_resource_start(pci_dev, region);
                resource_len = pci_resource_len(pci_dev, region);
                release_mem_region(resource_start, resource_len);
        }
        #endif
}

/*! otg_free_irqs
 */
void otg_free_irqs(struct otg_dev *otg_dev, struct device *device, int num_resources)
{
        struct platform_device *platform_device = to_platform_device(device);
        struct otg_interrupt *otg_interrupts = otg_dev->otg_interrupts;
        int                     resource;

        for (resource = 0; resource < MIN(platform_device->num_resources, num_resources); resource++) {

                struct resource *resources = platform_device->resource + resource;
                struct otg_interrupt *otg_interrupt = otg_interrupts + resource;


                CONTINUE_UNLESS(resources->flags == IORESOURCE_IRQ);

                free_irq(resources->start, otg_interrupt);
        }
}


/*!
 * otg_dev_probe() - otg dev probe function
 *
 */
int otg_dev_probe (struct device *device, struct otg_device_driver *otg_device_driver, void *privdata)
{
        struct platform_device *platform_device = to_platform_device(device);

        struct otg_dev_driver       *otg_dev_driver;
        struct otg_dev          *otg_dev = NULL;
        struct otg_interrupt    *otg_interrupts = NULL;
        int                     resource = 0;

        u8                      latency, limit;
        int                     i;

        struct otg_instance *otg = NULL;


        /* allocate otg structure */
        THROW_UNLESS((otg = otg_create()), error);

        /* allocate otg_dev structure and fill in standard fields */
        THROW_UNLESS((otg_dev = kmalloc(sizeof(struct otg_dev), GFP_KERNEL)), error);
        THROW_UNLESS((otg_interrupts =
                                kmalloc(sizeof(struct otg_interrupt) * platform_device->num_resources, GFP_KERNEL)), error);

        memset(otg_dev, 0, sizeof(struct otg_dev));
        otg_dev->num_resources = platform_device->num_resources;
        otg_dev->otg_interrupts = otg_interrupts;
        otg_dev->device = device;
        otg_dev->otg_instance = otg;
        otg_dev->DEV = otg_trace_obtain_tag(otg, "otg-dev");
        otg_dev->privdata = privdata;

        otg_dev->otg_device_driver = otg_device_driver;
        dev_set_drvdata(device, otg_dev);
        otg_dev_set_drvdata (otg_dev, device);

        for (resource = 0; resource < platform_device->num_resources; resource++) {

                struct resource *resources = platform_device->resource + resource;
                struct otg_interrupt *otg_interrupt = otg_interrupts + resource;

                CONTINUE_UNLESS(resources->flags == IORESOURCE_IRQ);

                TRACE_MSG2(otg_dev->DEV, "irq: %d start: %d", resource, resources->start);

                /* request irq - use hardware wrapper isr if available */

                otg_interrupt->device = device;
                otg_interrupt->irq = resource;

                printk(KERN_INFO"%s: REQUEST_IRQ resource: %d start: %d otg_interrupt: %x\n",
                                __FUNCTION__, resource, resources->start, otg_interrupt);
                THROW_IF(request_irq(resources->start,

                                        //(otg_dev->otg_device_driver->isr) ?
                                        //(otg_dev->otg_device_driver->isr) :
                                        otg_dev_isr,

                                        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
                                        SA_SHIRQ,
                                        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
                                        0,
                                        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
                                        otg_dev->otg_device_driver->name,
                                        otg_interrupt
                                    ), error);

        }

        if (otg_devs) {
                TRACE_MSG2(otg_dev->DEV, "otg_devs: %x new: %x", otg_devs, otg_dev);
                otg_dev->next = otg_devs;
        }

        otg_devs = otg_dev;

        for (i = 0; i < OTG_DRIVER_TYPES; i++) {
                struct otg_dev_driver *driver = otg_device_driver->drivers[i];

                /* check if we have a driver */
                CONTINUE_UNLESS(driver);

                /* register sub-driver with otg */
                switch (driver->id) {
                case OTG_DRIVER_TCD:
                        otg_dev->tcd_instance = otg_set_tcd_ops(otg_dev->otg_instance, driver->ops);
                        otg_dev->tcd_instance->privdata = otg_dev;
                        break;
                case OTG_DRIVER_PCD:
                        otg_dev->pcd_instance = otg_set_pcd_ops(otg_dev->otg_instance, driver->ops);
                        otg_dev->pcd_instance->privdata = otg_dev;
                        break;
                case OTG_DRIVER_HCD:
                        otg_dev->hcd_instance = otg_set_hcd_ops(otg_dev->otg_instance, driver->ops);
                        otg_dev->hcd_instance->privdata = otg_dev;
                        break;
                case OTG_DRIVER_OCD:
                        otg_dev->ocd_instance = otg_set_ocd_ops(otg_dev->otg_instance, driver->ops);
                        otg_dev->ocd_instance->privdata = otg_dev;
                        break;
                }

                /* call sub-driver probe() function */
                CONTINUE_UNLESS(driver->probe);
                CONTINUE_UNLESS(driver->probe(otg_dev));

        }

        /* start otg state machine */
        if (otg_device_driver->serial_number && strlen(otg_device_driver->serial_number)) {
                //TRACE_MSG1(ZAS, "serial_number: %s", otg_device_driver->serial_number);
                otg_serial_number (otg, otg_device_driver->serial_number);
        }
        otg->privdata = otg_dev;
        otg_init(otg);

        return 0;

        CATCH(error) {

                printk(KERN_INFO"%s: FAILED resource: %d\n", __FUNCTION__, resource);

                if (resource)
                        otg_free_irqs(otg_dev, device, resource);

                if (otg_interrupts)
                        kfree(otg_interrupts);

                dev_set_drvdata(device, NULL);

                otg_dev_free_dev(device, otg_dev);

                if (otg_dev) kfree(otg_dev);

                if (otg)
                        otg_destroy(otg);

                return -EINVAL;
        }
}

/*!
 * otg_dev_remove() - pci remove function
 */
void otg_dev_remove (struct device *device , struct otg_device_driver *otg_device_driver)
{
        struct platform_device *platform_device = to_platform_device(device);

        struct otg_dev *otg_dev = dev_get_drvdata(device);
        struct otg_instance *otg = otg_dev->otg_instance;
        int                     i;

        /* stop otg state machine */
        otg_exit(otg);

        /* release interrupts */
        otg_free_irqs(otg_dev, device, platform_device->num_resources);

        /* de-register sub-drivers from otg */
        for (i = 0; i < OTG_DRIVER_TYPES; i++) {
                struct otg_dev_driver *driver = otg_device_driver->drivers[i];
                CONTINUE_UNLESS(driver);
                CONTINUE_UNLESS(driver->remove);
                driver->remove(otg_dev);
                switch (driver->id) {
                case OTG_DRIVER_TCD:
                        otg_dev->tcd_instance = otg_set_tcd_ops(otg_dev->otg_instance, NULL);
                        break;
                case OTG_DRIVER_PCD:
                        otg_dev->pcd_instance = otg_set_pcd_ops(otg_dev->otg_instance, NULL);
                        break;
                case OTG_DRIVER_HCD:
                        otg_dev->hcd_instance = otg_set_hcd_ops(otg_dev->otg_instance, NULL);
                        break;
                case OTG_DRIVER_OCD:
                        otg_dev->ocd_instance = otg_set_ocd_ops(otg_dev->otg_instance, NULL);
                        break;
                }

        }

        /* finally destroy otg instance */
        otg_destroy(otg);

        /* cleanupj */
        dev_set_drvdata(device, NULL);

        otg_dev->otg_instance = NULL;
        kfree(otg_dev->otg_interrupts);


        #if 0
        if (otg_devs == otg_dev) {
                otg_devs = otg_dev->next;
        }
        else {
                struct otg_dev *link;
                for (link = otg_devs; link; link = otg_dev->next) {
                        if (link->next == otg_dev) {
                                link->next = otg_dev->next;
                                break;
                        }
                }
        }
        #endif

        otg_dev_free_dev(device, otg_dev);
        otg_trace_invalidate_tag(otg_dev->DEV);
        kfree(otg_dev);
}


/* ********************************************************************************************* */

/*!
 * otg_dev_register_driver() - sub-driver registration function
 */
int otg_dev_register_driver(struct otg_device_driver *otg_device_driver, struct otg_dev_driver *otg_dev_driver)
{
        struct otg_dev *otg_dev;

        otg_device_driver->drivers[otg_dev_driver->id] = otg_dev_driver;

        #if 0
        if (otg_dev_driver->probe) {
                for (otg_dev = otg_devs; otg_dev; otg_dev = otg_dev->next) {

                        if (otg_dev_driver->probe(otg_dev, otg_dev_driver->id)) {
                                otg_dev->otg_device_driver->drivers[otg_dev_driver->id] = NULL;
                                return -EINVAL;
                        }
                        return 0;
                }
        }
        #endif
        return 0;
}

/*!
 * otg_dev_unregister_driver() - sub-driver unregistration function
 */
void otg_dev_unregister_driver(struct otg_device_driver *otg_device_driver, struct otg_dev_driver *otg_dev_driver)
{
        struct otg_dev *otg_dev;

        #if 0
        if (otg_dev_driver->probe) {
                for (otg_dev = otg_devs; otg_dev; otg_dev = otg_dev->next) {
                        otg_dev_driver->remove(otg_dev, otg_dev_driver->id);
                        return;
                }
        }
        #endif
        otg_device_driver->drivers[otg_dev_driver->id] = NULL;
}

/*!
 * otg_dev_set_drvdata - set otg_dev data
 */

void otg_dev_set_drvdata(struct otg_dev *otg_dev, void *data)
{
        otg_dev->drvdata = data;
}
/*!
 * otg_dev_get_drvdata - get otg_dev data
 */
void * otg_dev_get_drvdata(struct otg_dev *otg_dev)
{
        return otg_dev->drvdata;
}

/* ********************************************************************************************* */

/* ********************************************************************************************* */




OTG_EXPORT_SYMBOL(otg_dev_probe);
OTG_EXPORT_SYMBOL(otg_dev_remove);


OTG_EXPORT_SYMBOL(otg_dev_register_driver);
OTG_EXPORT_SYMBOL(otg_dev_unregister_driver);

OTG_EXPORT_SYMBOL(otg_dev_set_drvdata);
OTG_EXPORT_SYMBOL(otg_dev_get_drvdata);
