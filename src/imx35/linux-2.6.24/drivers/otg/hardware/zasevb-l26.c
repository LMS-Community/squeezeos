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
 * otg/hardware/zasevb-l26.c - ZAS EVB OTG Peripheral and OTG Controller Drivers Module Initialization
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/zasevb/zasevb-l26.c|20070827195306|47636
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @defgroup ZASEVB Freescale ZAS EVB
 * @ingroup Hardware
 */
/*!
 * @file otg/hardware/zasevb-l26.c
 * @brief ZAS EVB USB Host Controller Driver
 *
 *
 * Linux ZASEVB OTG PCD/HCD/OCD/TCD Driver Initialization
 *
 * This file initializes all of the low level hardware drivers for the ZAS EVB.
 *
 * Notes.
 *
 *
 * @ingroup ZASEVB
 * @ingroup LINUXOS
 *
 */

#include <otg/pcd-include.h>
#include <linux/module.h>

#include <linux/pci.h>
#include <asm/arch/gpio.h>
#include <linux/clk.h>

#include "mxc-lnx.h"
#include "mxc-hardware.h"

MOD_AUTHOR ("sl@belcarra.com");
MOD_DESCRIPTION ("Belcarra MXC");
EMBED_LICENSE();


MOD_PARM_STR(serial_number_str, "Serial Number String", NULL);

/* OTG Driver
 */
otg_tag_t REMOVE_OCD;
struct ocd_instance *ocd_instance;
extern struct ocd_ops ocd_ops;

/* Transceiver Driver
 */
otg_tag_t REMOVE_TCD;
struct tcd_instance *REMOVE_tcd_instance;
extern struct tcd_ops tcd_ops;


/* Peripheral Driver
 */
otg_tag_t REMOVE_PCD;
struct pcd_instance *REMOVE_pcd_instance;
#if !defined(CONFIG_USB_HOST)
extern struct pcd_ops pcd_ops;
#else /* !defined(CONFIG_USB_HOST) */
irqreturn_t mxc_pcd_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        return IRQ_HANDLED;
}
#endif /* !defined(CONFIG_USB_HOST) */

/* Host Driver
 */
otg_tag_t HCD;
struct hcd_instance *hcd_instance;
#if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
extern struct hcd_ops hcd_ops;

#else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
irqreturn_t hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs)
{
        return IRQ_HANDLED;
}
#endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */

/* ************************************************************************************* */

#if defined(CONFIG_OTG_ZASEVB_ISP1301) || defined(CONFIG_OTG_ZASEVB_ISP1301_MODULE)
#define OTG_USE_I2C
#else
#define OTG_DISABLE_I2C
#endif
#ifdef OTG_USE_I2C
extern int i2c_mod_init(struct otg_instance *otg);
extern void i2c_mod_exit(struct otg_instance *otg);
#endif
extern int mxc_procfs_init(void);
extern void mxc_procfs_exit(void);

//extern void fs_mod_exit(void);

#if !defined(OTG_C99)
extern void pcd_global_init(void);
extern void fs_ocd_global_init(void);
extern void zasevb_tcd_global_init(void);
extern void fs_pcd_global_init(void);
#endif /* !defined(OTG_C99) */
void mxc_pcd_ops_init(void);

#if defined(CONFIG_OTG_GPTR)
extern int mxc_gptcr_mod_init (int divisor, int multiplier);
void mxc_gptcr_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */

#if defined(CONFIG_OTG_HRT)
extern int mxc_hrt_mod_init (struct otg_instance *otg, int divisor, int multiplier);
void mxc_hrt_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */


otg_tag_t ZAS;


/*!
 * zasevb_modexit() - This is used as module exit, and as cleanup if modinit fails.
 */
static void zasevb_modexit (void)
{
        struct otg_instance *otg = ocd_instance->otg;
        //struct pcd_instance *pcd = (struct pcd_instance *)otg->pcd;
        //struct usbd_bus_instance *bus= pcd->bus;

        TRACE_MSG0(ZAS, "Modules exit!");

        if (otg) otg_exit(otg);

        mxc_procfs_exit();

        /* Disable GPT
         */
        #if defined(CONFIG_OTG_GPTR)
        mxc_gptcr_mod_exit();
        #endif /* defined(CONFIG_OTG_GPTR) */

        #if defined(CONFIG_OTG_HRT)
        mxc_hrt_mod_exit();
        #endif /* defined(CONFIG_OTG_GPTR) */

        #ifdef OTG_USE_I2C
        TRACE_MSG0(ZAS, "0. I2C");
        i2c_mod_exit(otg);
        #endif

        #if !defined(CONFIG_USB_HOST)
        if (pcd_ops.mod_exit) pcd_ops.mod_exit(otg);
        REMOVE_pcd_instance = otg_set_pcd_ops(otg, NULL);
        #else /* !defined(CONFIG_USB_HOST) */
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__);
        #endif /* !defined(CONFIG_USB_HOST) */


        #if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
        if (hcd_ops.mod_exit) hcd_ops.mod_exit(otg);
        hcd_instance = otg_set_hcd_ops(otg, NULL);
        //HCD = otg_trace_invalidate_tag(HCD);
        #else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
        printk(KERN_INFO"%s: HCD DRIVER N/A\n", __FUNCTION__);
        #endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */


        if (tcd_ops.mod_exit) tcd_ops.mod_exit(otg);
        printk(KERN_INFO"%s: set_tcd_ops\n", __FUNCTION__);
        REMOVE_tcd_instance = otg_set_tcd_ops(otg, NULL);
        //REMOVE_TCD = otg_trace_invalidate_tag(REMOVE_TCD);

        if (ocd_ops.mod_exit) ocd_ops.mod_exit(otg);
        ocd_instance = otg_set_ocd_ops(otg, NULL);


        ZAS = otg_trace_invalidate_tag(ZAS);


        otg_destroy(otg);
}

extern void mxc_pcd_ops_init(void);

/*!
 * zasevb_modinit() - linux module initialization
 *
 * This needs to initialize the hcd, pcd and tcd drivers. This includes tcd and possibly hcd
 * for some architectures.
 *
 */
static int zasevb_modinit (void)
{
        struct otg_instance *otg = NULL;

        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
        struct clk *clk = clk_get(NULL, "usb_clk");
        clk_enable(clk);
        clk_put(clk);
        #endif

        THROW_UNLESS((otg = otg_create()), error);

        mxc_pcd_ops_init();
        #if !defined(OTG_C99)
        pcd_global_init();
        fs_ocd_global_init();
        zasevb_tcd_global_init();
        fs_pcd_global_init();
        #endif /* !defined(OTG_C99) */

        ZAS = otg_trace_obtain_tag(otg, "zas");

        mxc_procfs_init();

        TRACE_MSG0(ZAS, "1. ZAS");

        #if 0
        /* ZAS EVB Platform setup
         */
        TRACE_MSG4(ZAS, "BCTRL Version: %04x Status: %04x 1: %04x 2: %04x",
                        readw(PBC_BASE_ADDRESS ),
                        readw(PBC_BASE_ADDRESS + PBC_BSTAT),
                        readw(PBC_BASE_ADDRESS + PBC_BCTRL1_SET),
                        readw(PBC_BASE_ADDRESS + PBC_BCTRL2_SET));

        #endif

        /* ZAS EVB Clock setup
         */

#if defined(CONFIG_ARCH_ARGONPLUS) || defined(CONFIG_ARCH_ARGONLV)
#define ZASEVB_MULTIPLIER       12
#define ZASEVB_DIVISOR          775             // ~10.
#else
#define ZASEVB_MULTIPLIER       12
#define ZASEVB_DIVISOR 155
#endif

        TRACE_MSG0(ZAS, "2. Setup GPT");

        THROW_UNLESS(ocd_instance = otg_set_ocd_ops(otg, &ocd_ops), error);
        REMOVE_OCD = ocd_instance->TAG;
        // XXX THROW_IF((ocd_ops.mod_init ? ocd_ops.mod_init() : 0), error);

        #if defined(CONFIG_OTG_GPTR)
        mxc_gptcr_mod_init(ZASEVB_DIVISOR, ZASEVB_MULTIPLIER);
        #endif /* defined(CONFIG_OTG_GPTR) */

        #if defined(CONFIG_OTG_HRT)
        mxc_hrt_mod_init(otg, ZASEVB_DIVISOR, ZASEVB_MULTIPLIER);
        #endif /* defined(CONFIG_OTG_GPTR) */


        #if !defined(CONFIG_USB_HOST)
        TRACE_MSG0(ZAS, "3. PCD");
        THROW_UNLESS(REMOVE_pcd_instance = otg_set_pcd_ops(otg, &pcd_ops), error);
        REMOVE_PCD = REMOVE_pcd_instance->TAG;
        // XXX THROW_IF((pcd_ops.mod_init ? pcd_ops.mod_init() : 0), error);
        #else /* !defined(CONFIG_USB_HOST) */
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__);
        #endif /* !defined(CONFIG_USB_HOST) */


        TRACE_MSG0(ZAS, "4. TCD");
        THROW_UNLESS(REMOVE_tcd_instance = otg_set_tcd_ops(otg, &tcd_ops), error);
        REMOVE_TCD = REMOVE_tcd_instance->TAG;
        // XXX THROW_IF((tcd_ops.mod_init ? tcd_ops.mod_init() : 0), error);
#ifdef OTG_USE_I2C
        TRACE_MSG0(ZAS, "0. I2C");
        i2c_mod_init(otg);
#endif


        #if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
        TRACE_MSG0(ZAS, "5. Host");
        THROW_UNLESS(hcd_instance = otg_set_hcd_ops(otg, &hcd_ops), error);
        HCD = hcd_instance->TAG;
        // XXX THROW_IF((hcd_ops.mod_init) ? hcd_ops.mod_init() : 0, error);
        #else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
        printk(KERN_INFO"%s: HCD DRIVER N/A\n", __FUNCTION__);
        #endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */



        TRACE_MSG0(ZAS, "6. Init & check");
        THROW_IF((ocd_ops.mod_init ? ocd_ops.mod_init(otg) : 0), error);

        #if !defined(CONFIG_USB_HOST)
        THROW_IF((pcd_ops.mod_init ? pcd_ops.mod_init(otg) : 0), error);
        #endif /* !defined(CONFIG_USB_HOST) */

        THROW_IF((tcd_ops.mod_init ? tcd_ops.mod_init(otg) : 0), error);

        #if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
        THROW_IF((hcd_ops.mod_init) ? hcd_ops.mod_init(otg) : 0, error);
        #endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */

        THROW_UNLESS(ocd_instance && (otg = ocd_instance->otg), error);


        TRACE_MSG0(ZAS, "7. otg_init");
        if (MODPARM(serial_number_str) && strlen(MODPARM(serial_number_str))) {

                TRACE_MSG1(ZAS, "serial_number_str: %s", MODPARM(serial_number_str));
                otg_serial_number (otg, MODPARM(serial_number_str));
        }
        otg_init(otg);

        return 0;

        CATCH(error) {
                //zasevb_modexit();
                return -EINVAL;
        }

        return 0;
}

#if 0
/* ************************************************************************************* */
static int __init_or_module
zasevb_remove(struct device *dev)
{
        return 0;
}


static int __init
zasevb_probe(struct device *dev)
{
}
static int
zasevb_suspend(struct device *dev, u32 state, u32 phase)
{
        return 0;
}

static int
zasevb_resume(struct device *dev, u32 phase)
{
        return 0;
}

static struct device_driver zasevb_driver = {

        .name =         (char *) "ZASEVB-MXC-USBOTG,
        .bus =          &otg_bus_type,

        .probe =        zasevb_probe,
        .remove =       zasevb_remove,

        .suspend =      zasevb_suspend,
        .resume =       zasevb_resume,

};


static int __init zasevb_init(void)
{
        if (usb_disabled())
                return -ENODEV;

        INFO("driver %s, %s\n", hcd_name, DRIVER_VERSION);
        return driver_register(&zasevb_driver);
}

static void __exit zasevb_exit(void)
{
        driver_unregister(&zasevb_driver);
}
#endif

module_init (zasevb_modinit);
module_exit (zasevb_modexit);
