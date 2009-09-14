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
 * otg/otg/otg-dev.h -- Generic PCI driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-dev.h|20070918212334|33755
 *
 *      Copyright (c) 2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/otg/otg-dev.h
 * @brief Generic PCI Driver.
 *
 * This is the base PCI driver for supporting the PCI devices. It is
 * setup to allow the various OTG drivers to register and be handled
 * separately.
 *
 * Multiple devices are supported.
 *
 *
 *      TCD     PCD     HCD     OCD
 *      |       |       |       |
 *      -------------------------
 *           device PCI
 *      -------------------------
 *           otg PCI
 *      -------------------------
 *            OS PCI
 *      -------------------------
 *           hardware
 *
 *
 * It will probe the hardware and support registration for the sub-device
 * drivers (ocd, hcd and pcd).
 *
 * otg_dev_register_driver() and otg_dev_unregister_driver() are used
 * to register the base PCI hardware driver information. Which should
 * result in the hardware getting probed.
 *
 * otg_register_driver() and otg_unregister_driver() are used to register
 * the ocd, pcd, hcd, tcd drivers.
 *
 * The Device PCI driver should ensure that the basic hardware and interrupt
 * setup is performed during it's probe. Then call otg_dev_probe().
 *
 */

#define OTG_DRIVER_TCD  0
#define OTG_DRIVER_PCD  1
#define OTG_DRIVER_HCD  2
#define OTG_DRIVER_OCD  3
#define OTG_DRIVER_TYPES 4


struct otg_dev;

struct otg_device_driver;

/*! otg_dev_driver
 * Functions used to start an otg sub driver.
 */
struct otg_dev_driver {
        char                    *name;
        int                     id;

        u32                     irqs;
        irqreturn_t             (*isr)(struct otg_dev *, void *, u32);

        int                     (*probe)(struct otg_dev *);
        void                    (*remove)(struct otg_dev *);

#ifdef CONFIG_PM
        void                    (*suspend)(struct otg_dev *dev, u32 state);       /* Device suspended */
        void                    (*resume)(struct otg_dev *dev);        /* Device woken up */
#endif /* CONFIG_PM */

        void                    *ops;

};

struct otg_interrupt {
        struct device           *device;
        struct platform_device   *platform_device;
        int                     irq;
};

/*! otg_device_driver
 */
struct otg_device_driver {

        char                    *name;

        /* device driver support */
        //int                     (*device_otg_probe)(struct device *, struct otg_device_driver *);
        //int                     (*device_otg_remove)(struct device *, struct otg_device_driver *);

        int                     (*probe)(struct device *, struct otg_device_driver *);
        int                     (*remove)(struct device *, struct otg_device_driver *);

        /* platform device driver support */
        int                     (*platform_otg_probe)(struct platform_device *, struct otg_device_driver *);
        int                     (*platform_otg_remove)(struct platform_device *, struct otg_device_driver *);

        /* interrupts */
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
        irqreturn_t             (*isr) (int irq, void *data, struct pt_regs *r);
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
        irqreturn_t             (*isr) (int irq, void *data);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */


        /* sub-drivers supporting various OTG functions from the hardware */
        struct otg_dev_driver   *drivers[OTG_DRIVER_TYPES];

        char                    *serial_number;
};


struct otg_dev {
        /* base hardware PCI driver and interrupt handler */
        struct device           *device;
        struct platform_device  *platform_device;

        struct otg_device_driver   *otg_device_driver;

        struct otg_dev          *next;

        spinlock_t              lock;

        /* OTG Driver data */
        void                    *drvdata;

        /* interrupt support */
        int                     num_resources;
        struct otg_interrupt    *otg_interrupts;

        /* otg trace support */
        otg_tag_t               DEV;

        char                    procfs[32];

        struct otg_instance     *otg_instance;

        struct ocd_instance     *ocd_instance;
        struct pcd_instance     *pcd_instance;
        struct tcd_instance     *tcd_instance;
        struct hcd_instance     *hcd_instance;

        u32                     interrupts;

        void                    *privdata;
};


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
extern irqreturn_t otg_dev_isr(int irq, void *data, struct pt_regs *r);
extern irqreturn_t otg_pdev_isr(int irq, void *data, struct pt_regs *r);
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */
extern irqreturn_t otg_dev_isr(int irq, void *data);
extern irqreturn_t otg_pdev_isr(int irq, void *data);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) */

extern int otg_dev_probe (struct device *, struct otg_device_driver *, void *privdata);
extern void otg_dev_remove (struct device *, struct otg_device_driver *);

extern int otg_pdev_probe (struct platform_device *, struct otg_device_driver *, void *privdata);
extern void otg_pdev_remove (struct platform_device *, struct otg_device_driver *);

extern int otg_dev_register_driver(struct otg_device_driver*, struct otg_dev_driver *);
extern void otg_dev_unregister_driver(struct otg_device_driver*, struct otg_dev_driver *);

extern void otg_dev_set_drvdata(struct otg_dev *, void *);
extern void * otg_dev_get_drvdata(struct otg_dev *);



/* End of FILE */
