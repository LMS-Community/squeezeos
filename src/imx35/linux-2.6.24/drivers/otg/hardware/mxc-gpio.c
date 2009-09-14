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
 * otg/hardware/mxc-gpio.c - Freescale USBOTG common gpio and iomux setting
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/mxc/mxc-gpio.c|20070612233038|29528
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc-gpio.c
 * @brief Freescale USB Host Controller Driver
 *
 * @ingroup FSOTG
 */

#include <otg/pcd-include.h>

#ifdef CONFIG_OTG_ZASEVB_ATLAS_CONNECTIVITY
#include <asm/arch/gpio.h>
#include "mxc-hardware.h"
#include "isp1301.h"
#include "isp1301-hardware.h"
#endif

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
#include <asm/arch/gpio.h>
#include "mxc-hardware.h"
#include "isp1301.h"
#include "isp1301-hardware.h"
#endif

#if defined(CONFIG_MACH_ARGONPLUSEVB) || defined(CONFIG_ARCH_ARGONPLUSEVB) || defined(CONFIG_MACH_ARGONPLUSODYSSEY) || defined(CONFIG_ARCH_ARGONLV)
#define ZGPIO_PORT 0
#define ZGPIO_PIN  2
#endif/* defined(CONFIG_MACH_ARGONPLUSEVB)*/

#if defined(CONFIG_MACH_SCMA11EVB) || defined(CONFIG_ARCH_SCMA11EVB)
#define ZGPIO_PORT 2
#define ZGPIO_PIN  12
#endif /* CONFIG_MACH_SCMA11EVB */

#ifdef CONFIG_ARCH_ZEUS
#define ZGPIO_PORT 1
#define ZGPIO_PIN 30
#endif /* CONFIG_ARCH_ZEUS */


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
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, FALSE);
        zasevb_int_disabled = TRUE;

        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: SCHEDULE WORK");
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        isp1301_bh_wakeup(FALSE);

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
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: ISP1301_BH");
        isp1301_bh(arg);
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: REENABLE");
        if (zasevb_int_disabled) {
                zasevb_int_disabled = FALSE;
                gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);
        }
        return NULL;
}




/*!
 * This function set iomux settings depends on platform and usb mode.
 *
 * @param otg           otg instance
 * @param  usb_mode     setting usb mode.
 *
 */

int mxc_iomux_gpio_isp1301_set (struct otg_instance *otg, int usb_mode)
{

        int gpio = 1;

        printk (KERN_INFO"MXC gpio setting for isp1301\n");

        isp1301_mod_init(otg, &zasevb_isp1301_bh);

        TRACE_MSG0(TCD, "5. IOMUX and GPIO Interrupt Configuration");

        #ifdef CONFIG_MACH_SCMA11EVB
        iomux_config_mux(AP_GPIO_AP_C12, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE); // XXX INPUTCONFIG_DEFAULT?
        #endif /* CONFIG_MACH_SCMA11EVB */

        #ifdef CONFIG_MACH_ARGONPLUSEVB
        iomux_config_mux(PIN_GPIO2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_MACH_ARGONPLUSEVB */

        #ifdef CONFIG_MACH_ZEUSEVB
        iomux_config_mux(SPI2_SS1_PIN, MUX0_OUT, GPIO_MUX1_IN);
        #endif /* CONFIG_MACH_ZEUSEVB */

        //Settung interrupt for ISP1301
        gpio_config(ZGPIO_PORT, ZGPIO_PIN, false, GPIO_INT_FALL_EDGE);
        gpio = gpio_request_irq(ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO, zasevb_gpio_int_hndlr,
                        SA_SHIRQ, "ISP1301", (void *) &ocd_ops);
        THROW_IF(gpio, error);
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);  // XXX this might not be needed


        #if defined(CONFIG_ARCH_SCMA11)
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        #if 0
        switch(hwmode) {
        case XCVR_SE0_D_NEW:
        case XCVR_D_SE0_NEW:
                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
                iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
                printk(KERN_INFO"%s: EXTRA IOMUX\n", __FUNCTION__);
                break;

        case XCVR_D_D:
        case XCVR_SE0_SE0:
                printk(KERN_INFO"%s: NO EXTRA IOMUX\n", __FUNCTION__);
                break;
        }
        #endif
        #endif /* CONFIG_ARCH_SCMA11 */

        #if defined(CONFIG_ARCH_ARGONPLUS) || defined(CONFIG_ARCH_ARGONLV)
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_ARCH_ARGONPLUS */


        #ifdef CONFIG_ARCH_ZEUS
        iomux_config_mux(USB_DAT_VP_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x00
        iomux_config_mux(USB_SE0_VM_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x01
        iomux_config_mux(USB_TXOE_B_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x09
        iomux_config_mux(USB_RXD_PIN,     MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x3c
        #endif


        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (gpio) gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }

        return 0;
}

/*! mxc_iomux_gpio_isp1301_reset - reset isp1301 gpio irq setting
 */
int mxc_iomux_gpio_isp1301_reset (void)
{
        gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
        return 0;
}

#endif  //CONFIG_OTG_ZASEVB_ISP1301



#ifdef CONFIG_OTG_ZASEVB_ATLAS_CONNECTIVITY
/*! mxc_iomux_gpio_atlas_set - set atlas gpio settings
 * @param usb_mode
 * @return 0
 */
int mxc_iomux_gpio_atlas_set (int usb_mode)
{

        printk (KERN_INFO"MXC gpio setting for Atlas\n");
        #if defined(CONFIG_ARCH_SCMA11)
        printk(KERN_INFO"IOMUX setting for  SCMA11\n");
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        #endif

        #if defined(CONFIG_MACH_ARGONPLUSEVB) || defined(CONFIG_ARCH_ARGONPLUSEVB) || defined(CONFIG_MACH_ARGONPLUSODYSSEY) || defined(CONFIG_ARCH_ARGONLV)
        printk(KERN_INFO"IOMUX setting for Argon+\n");
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_ARCH_ARGONPLUS */


        return 0;
}

/*! mxc_iomux_gpio_atlas_reset - reset atlas gpio settings
 * @param usb_mode
 * @return 0
 */
int mxc_iomux_gpio_atlas_reset (void)
{

        #if defined(CONFIG_ARCH_SCMA11)
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        #endif

        #if defined(CONFIG_MACH_ARGONPLUSEVB) || defined(CONFIG_ARCH_ARGONPLUSEVB)
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_ARCH_ARGONPLUS */


        return 0;
}
#endif  //CONFIG_OTG_ZASEVB_ATLAS_CONNECTIVITY

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_reset);
#endif
#ifdef CONFIG_OTG_ZASEVB_ATLAS_CONNECTIVITY
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_atlas_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_atlas_reset);
#endif
