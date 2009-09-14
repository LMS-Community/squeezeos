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
 * otg/hardware/arc-dev.c - MX31 Device driver
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/arc/arc-dev.c|20070921192720|26657
 *
 *      Copyright (c) 2006-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Shahrad Payandeh Lynne <sp@lbelcarra.com>,
 *      Stuart Lynne <sl@belcarra.com>
 */
/*!
 * @file otg/hardware/arc-dev.c
 * @brief Belcarra Freescale ARC Device driver.
 *
 * @ingroup ARC
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>
#include <otg/otg-api.h>
#include <otg/otg-dev.h>

#include <otg/otg-utils.h>

#include <asm/arch/gpio.h>
#include <asm/memory.h>
#include <asm/io.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */

//#include <asm/arch/arc_otg.h>

#define MX31_USB_NAME "arc_udc"

static const char driver_name[] = MX31_USB_NAME;


/* ********************************************************************************************* */
/* arc Proc FS
 */

/*! arc_dev_show - called to display information
 * @param s
 * @param unused
 */
static int arc_dev_show(struct seq_file *s, void *unused)
{
        int                     etdn;
        unsigned long           flags;
        u32                     u;

        seq_printf(s, "MX31 USB\n\n");

        seq_printf(s, "\nOTG Registers\n");

        seq_printf(s, "\n");

        return 0;
}

/*! arc_dev_open - open a file
 * @param inode
 * @param file
 */

static int arc_dev_open(struct inode *inode, struct file *file)
{
        return single_open(file, arc_dev_show, PDE(inode)->data);
}

/*!  struct file_operatons arc_dev_proc_ops */
static struct file_operations arc_dev_proc_ops = {
        .open           = arc_dev_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static const char proc_filename[] = "arcusb";

/* ********************************************************************************************* */
/*!
 * arc_udc_isr() - architecture isr wrapper
 * @param irq
 * @param data
 * @param r
 */
irqreturn_t arc_udc_isr(int irq, void *data)
{

//	printk(KERN_INFO"%s: --\n", __FUNCTION__);
	return otg_dev_isr(irq,data);
}


/* ********************************************************************************************* */
/*! arc_dev_remove - called to remove
 * @param device - device instance
 * @param otg_device_driver - otg driver instance
 */
static int __init_or_module
arc_dev_remove(struct device *device, struct otg_device_driver *otg_device_driver)
{
        struct otg_dev *otg_dev = dev_get_drvdata(device);

        otg_dev_remove(device, otg_device_driver);

        return 0;
}


static void arc_dev_release(struct device *dev)
{
}

/*! arc_dev_probe - called to initialize
 * @param device - device
 * @param otg_device_driver - otg driver instance
 */
static int
arc_dev_probe(struct device *device, struct otg_device_driver *otg_device_driver)
{
        struct platform_device  *platform_device = to_platform_device(device);
        int probe = -1;


        /* verify correct driver name
         */
        THROW_IF(strcmp(platform_device->name, driver_name), error);

        device->release = arc_dev_release;



        /* do generic device probe, this will allocate resources (e.g. irqs)
         * and call the sub-driver probe functions.
         */
        THROW_IF((probe = otg_dev_probe(device, otg_device_driver, NULL)), error);
	printk(KERN_INFO"%s: device: %p\n", __FUNCTION__, platform_device);

        return 0;

        CATCH(error) {
                return -ENODEV;
        }
}

/* ********************************************************************************************* */
/*! Linux 2.6 Device Driver Support
 *
 * The Linux 2.6 Core requires that host driver be registered as a device driver. These
 * implement the requirements for the Linux Device Driver support.
 *
 */

/* ********************************************************************************************* */
/*! arc_device_suspend - called to suspend udc
 * @param device - device
 * @param state -
 */
static int
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
arc_device_suspend(struct device *device, pm_message_t state)
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */
arc_device_suspend(struct device *device, u32 state, u32 phase)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */

{
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        return -EINVAL;
}

/*! arc_device_resume - called to resume udc
 * @param device - device
 */
static int
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
arc_device_resume(struct device *device)
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */
arc_device_resume(struct device *device, u32 phase)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return -EINVAL;
}

/* ********************************************************************************************* */
/*! arc_dev_driver - define a device_driver structure for this driver
 */
static struct device_driver arc_dev_driver = {

        .name =         MX31_USB_NAME,
        .bus =          &platform_bus_type,

        .suspend =      arc_device_suspend,
        .resume =       arc_device_resume,

};

/* ********************************************************************************************* */
/*! Module init/exit
 *
 * Register this a linux device driver and platform. This provides the necessary framework
 * expected by the Linux 2.6 USB Core.
 *
 */

/*! arc_dev_module_init
 * @param otg_device_driver - platform device driver structure
 * @param device_probe - platform probe function
 * @param device_remove - platform rmove function
 */
int arc_dev_module_init(struct otg_device_driver *otg_device_driver,
                int (*device_probe)(struct device *),
                int (*device_remove)(struct device *))
{
        int driver_registered = -1;
        struct proc_dir_entry *pde = NULL;

        printk(KERN_INFO"%s: platform: %s driver: %s\n",
                        __FUNCTION__, otg_device_driver->name, arc_dev_driver.name) ;

        /* Setup the otg_device_driver probe function to point back to this one,
         * and the actual driver probe to the provided function. This allows the
         * platform driver to provide the overall device structure.
         *
         * We also provide a wrapper ISR in case we need to do something before
         * or after the otg_dev_isr() function has run.
         */
        THROW_UNLESS(otg_device_driver, error);
        THROW_UNLESS(device_probe, error);
        arc_dev_driver.probe = device_probe;
        arc_dev_driver.remove = device_remove;
        otg_device_driver->probe = arc_dev_probe;
        otg_device_driver->remove = arc_dev_remove;
        otg_device_driver->isr = arc_udc_isr;

        /* register the arc-dev drivers
         */
        THROW_IF((driver_registered = driver_register(&arc_dev_driver)), error);
        THROW_UNLESS((pde = create_proc_entry(proc_filename, 0, NULL)), error);
        pde->proc_fops = &arc_dev_proc_ops;
        return 0;
        CATCH(error) {
                printk(KERN_INFO"%s: FAILED\n", __FUNCTION__);
                if (pde) remove_proc_entry(proc_filename, NULL);
                UNLESS (driver_registered) driver_unregister(&arc_dev_driver);
                return -EINVAL;
        }
}

/*! arc_dev_module_exit
 */
void arc_dev_module_exit(struct otg_device_driver *otg_device_driver)
{
        otg_device_driver->probe = NULL;
        otg_device_driver->isr = NULL;
        remove_proc_entry(proc_filename, NULL);
        driver_unregister(&arc_dev_driver);
}
