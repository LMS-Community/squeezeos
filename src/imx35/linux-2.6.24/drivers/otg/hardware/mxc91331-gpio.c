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
 * otg/hardware/mxc91331-gpio.c - Freescale USBOTG ArgonPlus gpio and iomux setting
 * @(#) sp/root@belcarra.com/debian-black.(none)|otg/platform/mxc/mxc91331-gpio.c|20070822230929|09899
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>,
 *      Shahrad Payandeh <sp@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc91331-gpio.c
 * @brief Freescale USB Host Controller Driver
 *
 * @ingroup FSOTG
 */

#include <linux/irq.h>
#include <otg/pcd-include.h>
#include <../arch/arm/mach-mxc91321/iomux.h>

#if defined(CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY) || defined(CONFIG_OTG_ZASEVB_MC13783_PMIC)
#include <asm/arch/gpio.h>
#include "mxc-lnx.h"
#include "mxc-hardware.h"
#include "isp1301.h"
#include "isp1301-hardware.h"
#endif

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
#include <asm/arch/gpio.h>
#include "mxc-lnx.h"
#include "mxc-hardware.h"
#include "isp1301.h"
#include "isp1301-hardware.h"
#endif

#define ZGPIO_PORT 0
#define ZGPIO_PIN  2


#if defined(CONFIG_OTG_ZASEVB_ISP1301) || defined(_OTG_DOXYGEN)

BOOL zasevb_int_disabled = FALSE;

/* ********************************************************************************************* */
/*!
 * zasevb_gpio_int_hndlr() - gpio interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 * @return interrupt handler status
 * This disables the gpio interrup and schedules the isp1301 bottom half handler.
 *
 */
static irqreturn_t zasevb_gpio_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        disable_irq(irq);
        zasevb_int_disabled = TRUE;
        TRACE_MSG0(REMOVE_TCD, "ZASEVB GPIO INTERRUPT: SCHEDULE WORK");
        isp1301_bh_wakeup(REMOVE_tcd_instance->otg, FALSE);

        return IRQ_HANDLED;
}

/*!
 * zasevb_isp1301_bh()- call isp1301 bottom half handler
 * @param arg
 * This is a wrapper to the isp1301 bottom half handler, it
 * re-enables the gpio interrupt after processing complete.
 */
void *zasevb_isp1301_bh(void *arg)
{
        TRACE_MSG0(REMOVE_TCD, "ZASEVB GPIO INTERRUPT: ISP1301_BH");
        isp1301_bh(arg);
        TRACE_MSG0(REMOVE_TCD, "ZASEVB GPIO INTERRUPT: REENABLE");

        if (zasevb_int_disabled) {
                zasevb_int_disabled = FALSE;
                enable_irq(IOMUX_TO_IRQ(PIN_GPIO2));
        }
        return 0;
}




/*!
 * This function set iomux settings depends on platform and usb mode.
 * @param otg    otg  instance
 * @param  usb_mode     setting usb mode.
 *
 */

int mxc_iomux_gpio_isp1301_set (struct otg_instance *otg, int usb_mode)
{

        int gpio = 1;

        printk (KERN_INFO"MXC gpio setting for isp1301\n");

        isp1301_mod_init(otg, &zasevb_isp1301_bh);

        TRACE_MSG0(otg->tcd->TAG, "5. IOMUX and GPIO Interrupt Configuration");
        iomux_config_mux(PIN_GPIO2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

        //Setting interrupt for ISP1301
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
        set_irq_type(IOMUX_TO_IRQ(PIN_GPIO2), IRQF_TRIGGER_FALLING);
        #else
        set_irq_type(IOMUX_TO_IRQ(PIN_GPIO2), IRQT_FALLING);
        #endif
        gpio = request_irq(IOMUX_TO_IRQ(PIN_GPIO2), zasevb_gpio_int_hndlr,
                        0, "ISP1301", (void *)&ocd_ops);
        THROW_IF(gpio, error);


        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);



        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (gpio) gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }

        return 0;
}

int mxc_iomux_gpio_isp1301_reset (void)
{
        free_irq(IOMUX_TO_IRQ(PIN_GPIO2), &ocd_ops);
        return 0;
}

#endif  //CONFIG_OTG_ZASEVB_ISP1301



#if defined(CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY) || defined(CONFIG_OTG_ZASEVB_MC13783_PMIC)
/*! mxc_iomux_gpio_mc1383_set - set mc13783 iomux setting
 * @param usb_mode
 */
int mxc_iomux_gpio_mc13783_set (int usb_mode)
{

        printk (KERN_INFO"MXC gpio setting for Atlas\n");

        printk(KERN_INFO"IOMUX setting for Argon+\n");
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);


        return 0;
}

/*! mxc_iomux_gpio_mc13783_reset -clear mc13783 iomux settings
 */

int mxc_iomux_gpio_mc13783_reset (void)
{

        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

        return 0;
}
#endif  //CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY


int mxc_host_gpio (void)
{
        iomux_config_mux(PIN_GPIO5,  OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);      // USB1_OC
        printk(KERN_INFO"%s: NONE/GPIO\n", __FUNCTION__);
        return 0;
}

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_reset);
#endif
#if defined(CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY) || defined(CONFIG_OTG_ZASEVB_MC13783_PMIC)
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_mc13783_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_mc13783_reset);
#endif

OTG_EXPORT_SYMBOL(mxc_host_gpio);
