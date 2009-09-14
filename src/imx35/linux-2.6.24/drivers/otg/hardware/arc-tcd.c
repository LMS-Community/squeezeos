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
 * otg/hardware/arc-tcd.c -- USB Transceiver Controller driver
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/arc/arc-tcd.c|20070614183949|64278
 *
 *      Copyright (c) 2006-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Shahrad Payandeh Lynne <sp@lbelcarra.com>,
 *      Stuart Lynne <sl@lbelcarra.com>,
 *
 */
/*!
 * @file otg/hardware/arc-tcd.c
 * @brief Belcarra Freescale ARC Device driver.
 *
 * This interfaces to the generic Freescale platform transceiver driver.
 *
 *
 * @ingroup ARC
 * @ingroup TCD
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */

#include <otg/otg-compat.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-dev.h>
#include <otg/otg-hcd.h>
#include <otg/otg-tcd.h>
#include <otg/otg-ocd.h>
#include <otg/otg-pcd.h>
#include <asm/arch/arc_otg.h>
#include "arc-hardware.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>

extern void fsl_platform_set_vbus_power(struct fsl_usb2_platform_data *pdata, int on);

/* ********************************************************************************************* */
/*! arc_tcd_init() - used to enable
 * @param otg - otg_instance pointer
 * @param flag -
 *
 * Enable MX31 OTG interrupts.
 *
 */
void arc_tcd_init(struct otg_instance *otg, u8 flag)
{
        struct otg_dev *otg_dev = otg->privdata;
        struct tcd_instance *tcd = otg->tcd;

        switch (flag) {
        case SET:
                break;
        case RESET:
                break;
        }
        otg_event(otg, OCD_OK, otg->tcd->TAG, "MX31 TCD OK");
}

/*! arc_tcd_en() - used to enable
 * @param otg - otg_instance pointer
 * @param flag -
 *
 * Sample OTG physical inputs and pass to OTG state machine.
 *
 */
void arc_tcd_en(struct otg_instance *otg, u8 flag)
{
        struct otg_dev *otg_dev = otg->privdata;
        struct tcd_instance     *tcd = otg->tcd;

        // XXX
        u32     vbus = UOG_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS;
        TRACE_MSG2(tcd->TAG, "UOG_PORTSC1: %x, vbus: %x", UOG_PORTSC1, vbus);

        switch (flag) {
        case PULSE:
        case SET:
                switch (vbus) {
                case 0:
                        otg_event(otg, VBUS_VLD_ | B_SESS_VLD_ | A_SESS_VLD_, otg->tcd->TAG, "MX31 VBUS INVALID");
                        break;
                case 1:
                        otg_event(otg, VBUS_VLD | B_SESS_VLD, otg->tcd->TAG, "MX31 TCD EN");
                        break;
                default:
                        break;
                }
        case RESET:
                break;
        default:
                break;

        }
}


/*! arc_dp_pullup_func - used to enable or disable peripheral connecting to bus
 *
 * C.f. 5.1.6, 5.1.7, 5.2.4 and 5.2.5
 *
 *                              host    peripheral
 *              d+ pull-up      clr     set
 *              d+ pull-down    set     clr
 *
 *              d- pull-up      clr     clr
 *              d- pull-down    set     set
 *
 */


/*! arc_dp_pullup_func
 * @param otg - otg_instance pointer
 * @param flag -
 *
 * Enable or disable pullup.
 */
void arc_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct otg_dev *otg_dev = otg->privdata;

        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "MX31 PULLUP SET");
                UOG_USBCMD |= USB_CMD_RUN_STOP;
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "MX31 PULLUP RESET");
                UOG_USBCMD &= ~USB_CMD_RUN_STOP;
                break;
        }
}

struct tcd_ops tcd_ops = {
        .tcd_init_func = arc_tcd_init,
        .tcd_en_func = arc_tcd_en,
        .dp_pullup_func = arc_dp_pullup_func,
};

/* ********************************************************************************************* */
/* ********************************************************************************************* */

int arc_transceiver_callback(struct otg_instance *otg, arc_belcarra_transceiver_event_t event)
{
        struct otg_dev *otg_dev = otg->privdata;
        struct device   *device = otg_dev_get_drvdata(otg_dev);
        struct pcd_instance *pcd_instance = otg_dev->pcd_instance;

        switch (event) {
        case arc_vbus_valid:
                TRACE_MSG1(otg->tcd->TAG, "VBUS VALID event: %d", event);
                otg_event(otg, VBUS_VLD | B_SESS_VLD, otg->tcd->TAG, "MX31 VBUS VALID");
                break;
        case arc_vbus_invalid:
                TRACE_MSG1(otg->tcd->TAG, "VBUS INVALID event: %d", event);
                otg_event(otg, VBUS_VLD_ | B_SESS_VLD_ | A_SESS_VLD_, otg->tcd->TAG, "MX31 VBUS INVALID");
                break;
        }
        return 0;
}

/*!
 * arc_tcd_remove() - called to remove hardware
 * @param otg_dev - otg device
 */
static void
arc_tcd_remove(struct otg_dev *otg_dev)
{
        struct device           *device = otg_dev_get_drvdata(otg_dev);
        struct platform_device  *pdev = to_platform_device(device);

        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

	if (pdata->platform_uninit)
        pdata->platform_uninit(pdata);
}

/*!
 * arc_tcd_probe() - called to probe hardware
 * @param otg_dev - otg device
 *
 * This function should do minimal, one time only hardware recognition,
 * resource reservation and minimal setup. Typically to get to known
 * disabled state. It should not start the hardware.
 *
 */
static int
arc_tcd_probe(struct otg_dev *otg_dev)
{
        struct device           *device = otg_dev_get_drvdata(otg_dev);
        struct platform_device  *pdev = to_platform_device(device);

        struct fsl_usb2_platform_data *pdata = (struct fsl_usb2_platform_data*)pdev->dev.platform_data;

        struct otg_instance     *otg = otg_dev->otg_instance;
        struct tcd_instance     *tcd = otg->tcd;

        RETURN_ENODEV_IF(strcmp(pdev->name, "arc_udc"));
        RETURN_ZERO_IF (pdata->platform_init(pdev));

        fsl_platform_set_vbus_power(pdata, 0);

        return 0;
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */

/* arc tcd otg_dev_driver structure
 */
static struct otg_dev_driver arc_tcd_driver = {
        .name =         "arc_udc",
        .id =           OTG_DRIVER_TCD,

        .probe =        arc_tcd_probe,
        .remove =       arc_tcd_remove,

        .ops = &tcd_ops,
};

/* ********************************************************************************************* */

/*! arc_tcd_module_init() - module init
 */
int arc_tcd_module_init (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return otg_dev_register_driver(otg_device_driver, &arc_tcd_driver);
}

/*!
 * arc_tcd_module_exit() - module exit
 */
void arc_tcd_module_exit (struct otg_device_driver *otg_device_driver)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        otg_dev_unregister_driver (otg_device_driver, &arc_tcd_driver);
}
