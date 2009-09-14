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
 * otg/otg/otg-pci.h -- Generic PCI driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-pci.h|20061218212925|55364
 *
 *      Copyright (c) 2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/otg/otg-pci.h
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
 * otg_pci_register_driver() and otg_pci_unregister_driver() are used
 * to register the base PCI hardware driver information. Which should
 * result in the hardware getting probed.
 *
 * otg_register_driver() and otg_unregister_driver() are used to register
 * the ocd, pcd, hcd, tcd drivers.
 *
 * The Device PCI driver should ensure that the basic hardware and interrupt
 * setup is performed during it's probe. Then call otg_pci_prove().
 *
 */

#define OTG_DRIVER_TCD  0
#define OTG_DRIVER_PCD  1
#define OTG_DRIVER_HCD  2
#define OTG_DRIVER_OCD  3
#define OTG_DRIVER_TYPES 4


struct otg_pci;


/*! @struct otg_pci_driver  otg-pci.h  "otg/otp-pci.h"
 *
 *  @brief  base pci hardware driver struct providing interface
 *          to OS pci
 */
struct otg_pci_driver {
        char                    *name;
        int                     id;


        irqreturn_t             (*isr)(struct otg_pci *, void *);

        int (*probe)(struct otg_pci *dev, int id);
        void (*remove)(struct otg_pci *dev, int id);

#ifdef CONFIG_PM
        void (*suspend)(struct otg_pci *dev, u32 state, int id);       /* Device suspended */
        void (*resume)(struct otg_pci *dev, int id);        /* Device woken up */
#endif /* CONFIG_PM */

};

/*! otg_pci_device_driver
 */
struct otg_pci_device_driver {

        int                     pci_regions;
        char                    *name;
        irqreturn_t             (*isr)(struct otg_pci *, void *);

        /* sub-drivers supporting various OTG functions from the hardware */
        struct otg_pci_driver       *drivers[OTG_DRIVER_TYPES];
};


struct otg_pci {
        /* base hardware PCI driver and interrupt handler */
        struct pci_dev          *pci_dev;

        struct otg_pci_device_driver      *otg_pci_device_driver;

        struct otg_pci          *next;

        spinlock_t              lock;


        /* PCI mapping information */
        //unsigned long           resource_start;
        //unsigned long           resource_len;
        //void                    *base;

        void                    *regs[DEVICE_COUNT_RESOURCE];

        int                     pci_regions;

        int                     id;

        /* OTG Driver data */
        void                    *drvdata;

        otg_tag_t               PCI;

        char                    procfs[32];
        u32                     chiprev;

        struct otg_instance     *otg_instance;

        //struct ocd_instance     *ocd_instance;
        //struct pcd_instance     *pcd_instance;
        //struct tcd_instance     *tcd_instance;
        //struct hcd_instance     *hcd_instance;

        u32                     interrupts;
};



int __devinit otg_pci_probe (struct pci_dev *pci_dev, const struct pci_device_id *id, struct otg_pci_device_driver *otg_pci_device_driver);
void __devexit otg_pci_remove (struct pci_dev *pci_dev);

extern int otg_pci_register_driver(struct otg_pci_device_driver*, struct otg_pci_driver *);
extern void otg_pci_unregister_driver(struct otg_pci_device_driver*, struct otg_pci_driver *);

extern void otg_pci_set_drvdata(struct otg_pci *, void *);
extern void * otg_pci_get_drvdata(struct otg_pci *);


/* ********************************************************************************************** */

static u32 inline otg_readl(struct otg_pci *dev, int region, u32 reg)
{
        return readl(dev->regs[region] + reg);
}

static u32 inline otg_readlv(struct otg_pci *dev, int region, u32 reg, char *msg)
{
        u32 data = otg_readl(dev, region, reg);
        TRACE_MSG4(dev->PCI, "[%08x:%04x] %08x %s", dev->regs[region], reg, data, msg);
        return data;
}

static void inline otg_writel(struct otg_pci *dev, int region, u32 data, u32 reg)
{
        writel(data, dev->regs[region] + reg);
}

static void inline otg_writelv(struct otg_pci *dev, int region, u32 data, u32 reg, char *msg)
{
        otg_writel(dev, region, data, reg);
        TRACE_MSG4(dev->PCI, "[%08x:%04x] %08x %s", dev->regs[region], reg, data, msg);
}


static u16 inline otg_readw(struct otg_pci *dev, int region, u32 reg)
{
        u16 data = readw(dev->regs[region] + reg);
        TRACE_MSG2(dev->PCI, "[%04x] %04x", reg, data);
        //printk(KERN_INFO"%s: [%04x] %04x\n", __FUNCTION__, reg, data);
        return data;
}

static void inline otg_writew(struct otg_pci *dev, int region, u16 data, u32 reg)
{
        TRACE_MSG2(dev->PCI, "[%04x] %04x", reg, data);
        //printk(KERN_INFO"%s: [%04x] %04x\n", __FUNCTION__, reg, data);
        writew(data, dev->regs[region] + reg);
}

static u8 inline otg_readb(struct otg_pci *dev, int region, u32 reg)
{
        u8 data = readb(dev->regs[region] + reg);
        TRACE_MSG2(dev->PCI, "[%04x] %02x", reg, data);
        //printk(KERN_INFO"%s: [%04x] %02x\n", __FUNCTION__, reg, data);
        return data;
}

static void inline otg_writeb(struct otg_pci *dev, int region, u8 data, u32 reg)
{
        TRACE_MSG2(dev->PCI, "[%04x] %02x", reg, data);
        //printk(KERN_INFO"%s: [%04x] %02x\n", __FUNCTION__, reg, data);
        writeb(data, dev->regs[region] + reg);
}

/* ********************************************************************************************* */

static u32 inline otg_readl0(struct otg_pci *dev, u32 reg)
{
        return otg_readl(dev, 0, reg);
}
static void inline otg_writel0(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writel(dev, 0, data, reg);
}

static u32 inline otg_readl1(struct otg_pci *dev, u32 reg)
{
        return otg_readl(dev, 1, reg);
}
static void inline otg_writel1(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writel(dev, 1, data, reg);
}

static u32 inline otg_readl2(struct otg_pci *dev, u32 reg)
{
        return otg_readl(dev, 2, reg);
}
static void inline otg_writel2(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writel(dev, 2, data, reg);
}

static u32 inline otg_readl3(struct otg_pci *dev, u32 reg)
{
        return otg_readl(dev, 3, reg);
}
static void inline otg_writel3(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writel(dev, 3, data, reg);
}

static u32 inline otg_readl3v(struct otg_pci *dev, u32 reg)
{
        return otg_readlv(dev, 3, reg, "");
}

static u32 inline otg_readl3m(struct otg_pci *dev, u32 reg, char *msg)
{
        return otg_readlv(dev, 3, reg, msg);
}

static void inline otg_writel3m(struct otg_pci *dev, u32 data, u32 reg, char *msg)
{
        otg_writelv(dev, 3, data, reg, msg);
}

static void inline otg_writel3v(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writelv(dev, 3, data, reg, "");
}

static u32 inline otg_readl4(struct otg_pci *dev, u32 reg)
{
        return otg_readl(dev, 4, reg);
}
static void inline otg_writel4(struct otg_pci *dev, u32 data, u32 reg)
{
        otg_writel(dev, 4, data, reg);
}

/* End of FILE */
