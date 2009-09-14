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
 * otg/hardware/zasevb-isp1301.c -- ZASEVB ISP1301 Transceiver Controller driver
 * @(#) sp/root@belcarra.com/debian-black.(none)|otg/platform/zasevb/zasevb-isp1301.c|20070822230929|14023
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/zasevb-isp1301.c
 * @brief ZAS EVB USB ISP1301 Transceiver Driver
 *
 * This is a wrapper for the ISP1301 driver to support it on the ZASEVB.
 *
 * Notes
 *
 * 1. Ensure that S2.1 and S2.2 are both in the ON position.
 *
 * 2. The ISP1301 uses the system I2C bus, this is done throught the i2c-l26.c library.
 * This library must be opened with the correct adapter name.
 *
 * 3. The ISP1301 uses a GPIO for interrupts. The GPIO must be properly setup using
 * iomux().
 *
 * 4. The ISP1301 and USBOTG HWMODE must both be setup to match. Currently it
 * appears that only the SEO-SEO / DAT-Bidirectional combination works correctly.
 *
 * @ingroup ZASEVB
 * @ingroup TCD
 *
 */


#include <otg/pcd-include.h>

#if defined(CONFIG_OTG_ZASEVB_ISP1301) || defined(_OTG_DOXYGEN)

#include <asm/arch/gpio.h>

#include "mxc-lnx.h"
#include "mxc-hardware.h"

#include "isp1301.h"
#include "isp1301-hardware.h"

#include <linux/delay.h>

#ifndef PLL2_BASE_ADDR
#define PLL2_BASE_ADDR                          0x5004C000
#endif
#define PLL2_DP_CTL                             IO_ADDRESS(PLL2_BASE_ADDR + 0x00)
#define PLL2_DP_CONFIG                          IO_ADDRESS(PLL2_BASE_ADDR + 0x04)
#define PLL2_DP_OP                              IO_ADDRESS(PLL2_BASE_ADDR + 0x08)
#define PLL2_DP_MFD                             IO_ADDRESS(PLL2_BASE_ADDR + 0x0c)
#define PLL2_DP_MFN                             IO_ADDRESS(PLL2_BASE_ADDR + 0x10)
#define PLL2_DP_HFSOP                           IO_ADDRESS(PLL2_BASE_ADDR + 0x1c)
#define PLL2_DP_HFSMFD                          IO_ADDRESS(PLL2_BASE_ADDR + 0x20)
#define PLL2_DP_HFSMFN                          IO_ADDRESS(PLL2_BASE_ADDR + 0x24)



#if defined(CONFIG_MACH_ARGONPLUSEVB) || defined(CONFIG_ARCH_ARGONPLUSEVB) || defined(CONFIG_ARCH_ARGONLV)
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


void mxc_set_transceiver_mode(int mode);
void mxc_main_clock_on(void);
void mxc_disable_interrupts (void);
void isp1301_exit(void);
int mxc_iomux_gpio_isp1301_set (struct otg_instance *otg, int);
int mxc_iomux_gpio_isp1301_reset (struct otg_instance *otg);



#if 0   //removing gpios

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

        TRACE_MSG0(otg->tcd->TAG, "ZASEVB GPIO INTERRUPT: SCHEDULE WORK");
        isp1301_bh_wakeup(REMOVE_tcd_instance->otg, FALSE);

        return IRQ_HANDLED;
}

/*!
 * zasevb_isp1301_bh()- call isp1301 bottom half handler
 * @param arg
 * This is a wrapper to the isp1301 bottom half handler, it
 * re-enables the gpio interrupt after processing complete.
 */
void zasevb_isp1301_bh(void *arg)
{
        TRACE_MSG0(otg->tcd->TAG, "ZASEVB GPIO INTERRUPT: ISP1301_BH");
        isp1301_bh(arg);
        TRACE_MSG0(otg->tcd->TAG, "ZASEVB GPIO INTERRUPT: REENABLE");
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);
}

#endif

/* ********************************************************************************************* */
extern int zasevb_tcd_mod_init (struct otg_instance *otg);
extern void zasevb_tcd_mod_exit (struct otg_instance *otg);
//struct tcd_instance *zasevb_tcd_instance;
#if !defined(OTG_C99)
struct tcd_ops tcd_ops;
/*!
 * zasevb_tcd_global_init() - non c99 global initializer
 */
void zasevb_tcd_global_init(void)
{
        ZERO(tcd_ops);

        tcd_ops.vbus = isp1301_vbus;
        tcd_ops.id = isp1301_id;

        tcd_ops.tcd_init_func = isp1301_tcd_init;
        tcd_ops.tcd_en_func = isp1301_tcd_en;
        tcd_ops.chrg_vbus_func = isp1301_chrg_vbus;
        tcd_ops.drv_vbus_func = isp1301_drv_vbus;
        tcd_ops.dischrg_vbus_func = isp1301_dischrg_vbus;
        tcd_ops.dp_pullup_func = isp1301_dp_pullup_func;
        tcd_ops.dm_pullup_func = isp1301_dm_pullup_func;
        tcd_ops.dp_pulldown_func = isp1301_dp_pulldown_func;
        tcd_ops.dm_pulldown_func = isp1301_dm_pulldown_func;

        tcd_ops.overcurrent_func = NULL;
        tcd_ops.dm_det_func = isp1301_dm_det_func;
        tcd_ops.dp_det_func = isp1301_dp_det_func;
        tcd_ops.cr_det_func = isp1301_cr_det_func;
        tcd_ops.peripheral_host_func = isp1301_peripheral_host_func;
        //tcd_ops.mx21_vbus_drain_func = isp1301_mx21_vbus_drain_func;
        tcd_ops.id_pulldown_func = isp1301_id_pulldown_func;
        tcd_ops.audio_func = isp1301_audio_func;
        tcd_ops.uart_func = isp1301_uart_func;
        tcd_ops.mono_func = isp1301_mono_func;

        tcd_ops.mod_init = zasevb_tcd_mod_init;
        tcd_ops.mod_exit = zasevb_tcd_mod_exit;
}
#else /* !defined(OTG_C99) */
struct tcd_ops tcd_ops = {

        //.vbus = isp1301_vbus,
        //.id = isp1301_id,

        .tcd_init_func = isp1301_tcd_init,
        .tcd_en_func = isp1301_tcd_en,
        .chrg_vbus_func = isp1301_chrg_vbus,
        .drv_vbus_func = isp1301_drv_vbus,
        .dischrg_vbus_func = isp1301_dischrg_vbus,
        .dp_pullup_func = isp1301_dp_pullup_func,
        .dm_pullup_func = isp1301_dm_pullup_func,
        .dp_pulldown_func = isp1301_dp_pulldown_func,
        .dm_pulldown_func = isp1301_dm_pulldown_func,

        .overcurrent_func = NULL,
        .dm_det_func = isp1301_dm_det_func,
        .dp_det_func = isp1301_dp_det_func,
        .cr_det_func = isp1301_cr_det_func,
        .peripheral_host_func = isp1301_peripheral_host_func,
        //.mx21_vbus_drain_func = isp1301_mx21_vbus_drain_func,
        .id_pulldown_func = isp1301_id_pulldown_func,
        .audio_func = isp1301_audio_func,
        .uart_func = isp1301_uart_func,
        .mono_func = isp1301_mono_func,

        .mod_init = zasevb_tcd_mod_init,
        .mod_exit = zasevb_tcd_mod_exit,
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */
void zasevb_tcd_mod_exit (struct otg_instance *otg);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#define ADAPTER_NAME "mxc_i2c"
#else
#define ADAPTER_NAME "MXC I2C Adapter"
#endif

/*!
 * zasevb_tcd_mod_init() - initial tcd setup
 * This performs the platform specific hardware setup for the MX2ADS.
 * @param otg - otg_instance pointer
 */
int zasevb_tcd_mod_init (struct otg_instance *otg)
{
        int i2c = 1;
        int gpio = 1;

        /* ------------------------------------------------------------------------ */
        #if defined(CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL)
        int hwmode = XCVR_D_SE0_NEW;
        int newmode = XCVR_D_D;
        isp1301_tx_mode_t tx_mode = vp_vm_unidirectional;       // SCMA11 ok
        printk (KERN_INFO"Current setting is DIFFERENTIAL UNIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
        #elif defined(CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL)
        int hwmode = XCVR_SE0_D_NEW;
        int newmode = XCVR_SE0_D_NEW;
        isp1301_tx_mode_t tx_mode = dat_se0_unidirectional;     // ArgonEVB ok
        printk (KERN_INFO"Current setting is SINGLE ENDED UNIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
        #elif defined(CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL)
        int hwmode = XCVR_D_D;
        int newmode = XCVR_D_D;
        isp1301_tx_mode_t tx_mode = vp_vm_bidirectional;       // ArgonEVB ok
        printk (KERN_INFO"Current setting is DIFFERENTIAL BIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
        #elif defined(CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL)
        int hwmode = XCVR_SE0_SE0;
        int newmode = XCVR_SE0_SE0;
        isp1301_tx_mode_t tx_mode = dat_se0_bidirectional;      //SCMA11 ok
        printk (KERN_INFO"Current setting is SINGLE ENDED BIDIRECTIONAL\n");

	/* ------------------------------------------------------------------------ */
        #else
        #error Please Configure Transceiver Mode
        #endif /* CONFIG_OTG_ZASEVB_.... */
        /* ------------------------------------------------------------------------ */

        TRACE_MSG0(otg->tcd->TAG, "1. I2C setup");

        THROW_IF ((i2c = i2c_configure(ADAPTER_NAME, ISP1301_I2C_ADDR_HIGH)), error);

        TRACE_MSG0(otg->tcd->TAG, "2. ISP1301 module setup");
        //        isp1301_mod_init(&zasevb_isp1301_bh);

        //TRACE_MSG0(otg->tcd->TAG, "3. SET TCD OPS");
        //printk(KERN_INFO"%s: set_tcd_ops\n", __FUNCTION__);
        //THROW_UNLESS(zasevb_tcd_instance = otg_set_tcd_ops(otg, &tcd_ops), error);

        TRACE_MSG0(otg->tcd->TAG, "4. ISP1301 device setup");

        mxc_iomux_gpio_isp1301_set (otg, hwmode);

	#ifdef CONFIG_ARCH_MXC91131
        writel (0x00000051, PLL2_DP_HFSOP);
        writel (0x00000051, PLL2_DP_OP);
	#endif /* CONFIG_ARCH_ZEUS */


        /* ------------------------------------------------------------------------ */
        TRACE_MSG0(otg->tcd->TAG, "7. SET HWMODE");
        isp1301_configure(otg, tx_mode, spd_susp_reg);
        mxc_main_clock_on();
        //mxc_host_clock_on();
        //mxc_func_clock_on();
        mxc_set_transceiver_mode(newmode);



#if 0   //Test reading registers

        int int_src1, int_lat;
        for (;;){
                int_src1 = i2c_readb(ISP1301_INTERRUPT_SOURCE);
                int_lat = i2c_readb(ISP1301_INTERRUPT_LATCH_CLR);
                printk(KERN_INFO"%s: interrupt source: %x latch: %x\n", __FUNCTION__, int_src1, int_lat);
                mdelay (1000);
                i2c_writeb(ISP1301_INTERRUPT_LATCH_CLR, int_lat);
                mdelay (1000);
        }

#endif

#if 0   //pulse on DP
        for (;;){
                mdelay (100);
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DP_PULLUP);
                mdelay (100);
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DP_PULLUP);
        }
#endif

        /* Success!
         */
        TRACE_MSG0(otg->tcd->TAG, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (i2c) i2c_close();
                //                UNLESS (gpio) gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }
        TRACE_MSG0(otg->tcd->TAG, "MX2_MOD_TCD_INIT FINISHED");
        return 0;
}

/*!
 * zasevb_tcd_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 * @param otg - otg_instance pointer
 */
void zasevb_tcd_mod_exit (struct otg_instance *otg)
{
        //struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(otg->tcd->TAG, "ZASEVB_TCD_MOD_EXIT");
        isp1301_mod_exit(otg);
        mxc_disable_interrupts();
        //      gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
        mxc_iomux_gpio_isp1301_reset(otg);
        //zasevb_tcd_instance = otg_set_tcd_ops(otg, NULL);
        isp1301_exit();
        printk(KERN_INFO"%s: call i2c_close()\n", __FUNCTION__);
        i2c_close();
}
#endif /* CONFIG_ZASEVB_ISP1301 */
