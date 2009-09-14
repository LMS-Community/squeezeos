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
 * otg/hardware/zasevb-mc13783.c -- Freescale mc13783 Connectivity driver
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/zasevb/zasevb-pmic.c|20070614183950|37596
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/hardware/zasevb-pmic.c
 * @brief mc13783 USB Transceiver Driver
 *
 * This is a wrapper for the Freescale MC13783 driver to support it on the ZASEVB.
 *
 * Notes
 *
 * 1. Ensure that S2.1 is ON and S2.2 is OFF.
 *
 * 2. Note that this driver has not been tested on the ZAS EVB.
 *
 * 3. The MC13783 Transceiver and USBOTG HWMODE must both be setup to match. Currently it
 * appears that only the SEO-SEO / DAT-Bidirectional combination works correctly.
 *
 * @ingroup ZASEVB
 * @ingroup TCD
 *
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
#if defined(CONFIG_OTG_ZASEVB_MC13783_PMIC) || defined(_OTG_DOXYGEN)

#include <asm/arch/gpio.h>

#include "mxc-lnx.h"
#include "mxc-hardware.h"

#include "isp1301.h"
#include "isp1301-hardware.h"

/*
 * These files are currently located in
 *      drivers/mxc/pmic
 */
#include <asm/arch/pmic_convity.h>      /* For PMIC Connectivity driver interface. */
#include <asm/arch/pmic_external.h>     /* For PMIC Protocol driver interface.     */
//#include <core/pmic_event.h>



/* ********************************************************************************************* */
extern PMIC_CONVITY_HANDLE pmic_handle;
extern int pmic_tcd_mod_init(struct otg_instance *otg);
extern void pmic_tcd_mod_exit(struct otg_instance *otg);
struct tcd_instance *pmic_tcd_instance;
/* ********************************************************************************************* */
int mxc_pmic_vbus(struct otg_instance *otg);
int mxc_pmic_id(struct otg_instance *otg);
void mxc_pmic_tcd_en(struct otg_instance *otg, u8 flag);
void mxc_pmic_tcd_init(struct otg_instance *otg, u8 flag);
void mxc_pmic_chrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_drv_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_dischrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_peripheral_host_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_cr_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_bdis_acon_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_id_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_audio_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_uart_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_mono_func(struct otg_instance *otg, u8 flag);
int mxc_pmic_mod_init(struct otg_instance *);
void mxc_pmic_mod_exit(struct otg_instance *);
void mxc_set_transceiver_mode(int);

int mxc_iomux_gpio_mc13783_set(int);
int mxc_iomux_gpio_mc13783_reset(void);

/* ********************************************************************************************* */
#if !defined(OTG_C99)
struct tcd_ops tcd_ops;
/*!
 * pmic_tcd_global_init() - non c99 global initializer
 */
void pmic_tcd_global_init(void)
{
        ZERO(tcd_ops);

        tcd_ops.id = mxc_pmic_id;
        tcd_ops.vbus = mxc_pmic_vbus;
        //tcd_ops.tcd_init_func = mx2_tcd_init;

        tcd_ops.tcd_init_func = mxc_pmic_tcd_init;
        tcd_ops.tcd_en_func = mxc_pmic_tcd_en;
        tcd_ops.chrg_vbus_func = mxc_pmic_chrg_vbus;
        tcd_ops.drv_vbus_func = mxc_pmic_drv_vbus;
        tcd_ops.dischrg_vbus_func = mxc_pmic_dischrg_vbus;
        tcd_ops.dp_pullup_func = mxc_pmic_dp_pullup_func;
        tcd_ops.dm_pullup_func = mxc_pmic_dm_pullup_func;
        tcd_ops.dp_pulldown_func = mxc_pmic_dp_pulldown_func;
        tcd_ops.dm_pulldown_func = mxc_pmic_dm_pulldown_func;

        tcd_ops.overcurrent_func = NULL;
        tcd_ops.dm_det_func = mxc_pmic_dm_det_func;
        tcd_ops.dp_det_func = mxc_pmic_dp_det_func;
        tcd_ops.cr_det_func = mxc_pmic_cr_det_func;
        //tcd_ops.charge_pump_func = mx2_charge_pump_func;
        //tcd_ops.bdis_acon_func = mxc_pmic_bdis_acon_func;
        tcd_ops.peripheral_host_func = mxc_pmic_peripheral_host_func;
        tcd_ops.mx21_vbus_drain_func = mxc_pmic_mx21_vbus_drain_func;
        tcd_ops.id_pulldown_func = mxc_pmic_id_pulldown_func;
        tcd_ops.audio_func = mxc_pmic_audio_func;
        tcd_ops.uart_func = mxc_pmic_uart_func;
        tcd_ops.mono_func = mxc_pmic_mono_func;

        tcd_ops.mod_init = pmic_tcd_mod_init;
        tcd_ops.mod_exit = pmic_tcd_mod_exit;
}
#else   /* !defined(OTG_C99) */
struct tcd_ops tcd_ops = {

        //.id = mxc_pmic_id,
        //.vbus = mxc_pmic_vbus,
        //.tcd_init_func = mx2_tcd_init,

        .tcd_init_func = mxc_pmic_tcd_init,
        .tcd_en_func = mxc_pmic_tcd_en,
        .chrg_vbus_func = mxc_pmic_chrg_vbus,
        .drv_vbus_func = mxc_pmic_drv_vbus,
        .dischrg_vbus_func = mxc_pmic_dischrg_vbus,
        .dp_pullup_func = mxc_pmic_dp_pullup_func,
        .dm_pullup_func = mxc_pmic_dm_pullup_func,
        .dp_pulldown_func = mxc_pmic_dp_pulldown_func,
        .dm_pulldown_func = mxc_pmic_dm_pulldown_func,

        .overcurrent_func = NULL,
        .dm_det_func = mxc_pmic_dm_det_func,
        .dp_det_func = mxc_pmic_dp_det_func,
        .cr_det_func = mxc_pmic_cr_det_func,
        //.charge_pump_func = mx2_charge_pump_func,
        //tcd_ops.bdis_acon_func = mxc_pmic_bdis_acon_func,
        .peripheral_host_func = mxc_pmic_peripheral_host_func,
        .mx21_vbus_drain_func = mxc_pmic_mx21_vbus_drain_func,
        .id_pulldown_func = mxc_pmic_id_pulldown_func,
        .audio_func = mxc_pmic_audio_func,
        .uart_func = mxc_pmic_uart_func,
        .mono_func = mxc_pmic_mono_func,

        .mod_init = pmic_tcd_mod_init,
        .mod_exit = pmic_tcd_mod_exit,
};
#endif  /* !defined(OTG_C99) */

/* ********************************************************************************************* */
void pmic_tcd_mod_exit(struct otg_instance *otg);

/*!
 * pmic_tcd_mod_init() - initial tcd setup
 * This performs the platform specific hardware setup for the MX2ADS.
 * @param otg - otg_instance pointer
 *
 */
int pmic_tcd_mod_init(struct otg_instance *otg)
{
        int gpio = 1;
        bool res;
        unsigned int reg_value, reg_mask = 0xfffff;
        int i;
#if 1
#ifdef CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL
        int hwmode = XCVR_D_D;
        int newmode = XCVR_D_D;
#elif CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL
        int hwmode = XCVR_D_SE0_NEW;
        int newmode = XCVR_D_D;
#elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL
        int hwmode = XCVR_SE0_D_NEW;
        int newmode = XCVR_SE0_D_NEW;
#elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL
        int hwmode = XCVR_SE0_SE0;
        int newmode = XCVR_SE0_SE0;
#else
#error Please Configure Transceiver Mode
#endif  /* CONFIG_OTG_ZASEVB_.... */
#endif

        printk(KERN_INFO "%s: AAAA22\n", __FUNCTION__);

        TRACE_MSG0(REMOVE_TCD, "1. mc13783 Connectivity");

        mxc_pmic_mod_init(otg);

        TRACE_MSG0(REMOVE_TCD, "2. Transceiver setup");

        switch (hwmode) {
        case XCVR_D_D:
        case XCVR_SE0_D_NEW:
        case XCVR_D_SE0_NEW:
                break;

        case XCVR_SE0_SE0:
                // this works with XCVR_SE0_SE0 if AP_GPIO_AP_C16 not configured
                //isp1301_configure(dat_se0_bidirectional, spd_susp_reg);        // XCVR_SEO_SE0
                // XXX configure mc13783 transceiver here
                break;
        }

        //isp1301_configure(vp_vm_bidirectional, spd_susp_reg);        // XCVR_D_D

        //TRACE_MSG0(REMOVE_TCD, "5. SET TCD OPS");
        //THROW_UNLESS(mc13783_tcd_instance = otg_set_tcd_ops(otg, &tcd_ops), error);

        mxc_iomux_gpio_mc13783_set(hwmode);

        switch (hwmode) {
        case XCVR_D_SE0_NEW:
                TRACE_MSG0(REMOVE_TCD, "D_D - vp_vm_bidirectional");
                printk(KERN_INFO "%s: D_D - Differential Unidirectional\n",
                                __FUNCTION__);
                pmic_convity_usb_otg_set_config(pmic_handle, USB_OTG_SE0CONN);
                pmic_convity_usb_set_xcvr(pmic_handle, USB_DIFFERENTIAL_UNIDIR);
                break;
        case XCVR_SE0_D_NEW:
                TRACE_MSG0(REMOVE_TCD, "SE0_D");
                printk(KERN_INFO "%s: SE0_D - Single Ended Unidirectional\n",
                                __FUNCTION__);
                pmic_convity_usb_set_xcvr(pmic_handle, USB_SINGLE_ENDED_UNIDIR);
                break;
        case XCVR_D_D:
                TRACE_MSG0(REMOVE_TCD, "D_SE0");
                printk(KERN_INFO "%s: D_SE0 - Differential Bidirectional\n",
                                __FUNCTION__);
                pmic_convity_usb_set_xcvr(pmic_handle, USB_DIFFERENTIAL_BIDIR);
                break;

        case XCVR_SE0_SE0:
                TRACE_MSG0(REMOVE_TCD, "SE0_SE0 - SEO_bidirectional");
                printk(KERN_INFO "%s: SE0_SE0 - Single Ended Bidirectional\n",
                                __FUNCTION__);
                pmic_convity_usb_set_xcvr(pmic_handle, USB_SINGLE_ENDED_BIDIR);
                break;
        }

        TRACE_MSG0(REMOVE_TCD, "7. SET HWMODE");

        mxc_set_transceiver_mode(newmode);
        pmic_convity_usb_otg_set_config(pmic_handle, USB_PULL_OVERRIDE);        // variable 1k5 and UDP/UDM pull-down are disconnected. (PULLOVER)
        pmic_convity_usb_otg_set_config(pmic_handle, USBXCVREN);        //USB transceiver is disabled (USBXCVREN)
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_OTG_SE0CONN);        //variable UDP is not automatically connected (SE0CONN)
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_DP150K_PU);
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_USBCNTRL);
        pmic_convity_set_output(pmic_handle, TRUE, FALSE);      //disable vbus TBD
        pmic_convity_set_output(pmic_handle, FALSE, FALSE);     //disable vusb TBD
        pmic_convity_set_output(pmic_handle, FALSE, TRUE);      //enable vusb  TBD


#if 1

	for (i = 0; i < 3; i++) {
		pmic_read_reg(REG_CHARGER + i, &reg_value, reg_mask);
		printk(KERN_INFO "Register %d = %8X\n", REG_CHARGER + i, reg_value);
        }
#endif

        /* Success! */

        TRACE_MSG0(REMOVE_TCD, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO "%s: failed\n", __FUNCTION__);
                //SHP
                //UNLESS (gpio) gpio_free_irq (3, GPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }
        TRACE_MSG0(REMOVE_TCD, "MX2_MOD_TCD_INIT FINISHED");
        return 0;
}

/*!
 * pmic_tcd_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 * @param otg - otg_instance pointer
 */
void pmic_tcd_mod_exit(struct otg_instance *otg)
{
        //struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(REMOVE_TCD, "MX2_MOD_TCD_EXIT");

        mxc_iomux_gpio_mc13783_reset();

        mxc_pmic_mod_exit(otg);
        pmic_tcd_instance = otg_set_tcd_ops(otg, NULL);

}
#endif  /* CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY */
