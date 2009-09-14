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
 * otg/otg/core-init-l24.c - OTG Peripheral Controller Driver Module Initialization
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/otgcore/core-init-lnx.c|20070808203850|03038
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * This is the linux 2.4 version.
 *
 */
/*!
 * @file otg/otgcore/core-init-lnx.c
 * @brief OTG Core Linux initialization.
 *
 * This file is the starting point for defining the Linux OTG Core
 * driver. It references and starts all of the other components that
 * must be "linked" into the OTGCORE mdoule.
 *
 * @ingroup LINUXOS
 */


#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#include <linux/platform_device.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12) */

EMBED_LICENSE(); // When supported by the OS, embed license information in the binary


#ifdef OTG_MALLOC_TEST
int otg_mallocs;
#endif /* OTG_MALLOC_TEST */

otg_tag_t CORE;
//int usbd_procfs_init (void);
//void usbd_procfs_exit (void);


/* Tables for OTG firmware
 */
#if defined(CONFIG_OTG_USB_PERIPHERAL) || defined (CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)
struct otg_firmware *otg_firmware_loaded = &otg_firmware_mn;
struct otg_firmware *otg_firmware_orig = &otg_firmware_mn;
#elif defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE)
struct otg_firmware *otg_firmware_loaded = &otg_firmware_df;
struct otg_firmware *otg_firmware_orig = &otg_firmware_df;
#else /* error */
//#abort "Missing USB or OTG configuration"
#endif


struct otg_firmware *otg_firmware_loading;


//struct otg_instance otg_instance_private = {
//};


/*! otg_get_state_name - get corresponding name of specific state value
 *
 * @param state - state value
 * @return state name
 */
char * otg_get_state_name(int state)
{
        struct otg_state *otg_state;
        if (!otg_firmware_loaded || (state >= otg_firmware_loaded->number_of_states))
                return "UNKNOWN_STATE";

        otg_state = otg_firmware_loaded->otg_states + state;
        return otg_state->name;
}


/* ************************************************************************************* */




#if defined (CONFIG_OTG_USBD_PM)
int usbd_load(char * arg) { return 0; }
int usbd_unload(char *arg) { return 0; }
OTG_EXPORT_SYMBOL(usbd_load);
OTG_EXPORT_SYMBOL(usbd_unload);
#endif
/*! otg_create -create an otg instance
 *
 * @return created otg intance pointer
 */
struct otg_instance *otg_create(void)
{
        int message_init = 0;
        int message_init_l24 = 0;
        //int trace_init = 0;
        //int trace_init_l24 = 0;
        struct otg_instance *otg = NULL;


        THROW_UNLESS((otg = CKMALLOC(sizeof(struct otg_instance))), error);

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //trace_init = otg_trace_init();
        //trace_init_l24 = otg_trace_init_l24();
        CORE = otg_trace_obtain_tag(NULL, "core-otg");
        otg->TAG = CORE;

        otg->function_name[0] = '\0';

        TRACE_MSG0(CORE,"--");
        otg->current_outputs = otg_firmware_loaded->otg_states;
        otg->previous_outputs = otg_firmware_loaded->otg_states;

        /* initialize otg-mesg and usbp
         */
        THROW_IF((message_init = otg_message_init(otg)), error);
        THROW_IF((message_init_l24 = otg_message_init_l24(otg)), error);
        THROW_IF(usbd_device_init(), error);


        otg_set_ocd_ops(otg, NULL);
        otg_set_tcd_ops(otg, NULL);
        otg_set_hcd_ops(otg, NULL);
        otg_set_pcd_ops(otg, NULL);

        return otg;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (message_init_l24 )otg_message_exit_l24(otg);
                if (message_init) otg_message_exit();
                CORE = otg_trace_invalidate_tag(CORE);
                if (otg) LKFREE(otg);
                return NULL;
        }
}
/*! otg_destroy - free otg instance and associated resources
 * @param otg - otg instance pointer
 * @return none
 */
void otg_destroy(struct otg_instance *otg)
{
        //usbd_procfs_exit ();
        usbd_device_exit();
        otg_message_exit_l24(otg);
        otg_message_exit();


        if (otg_firmware_loading) {
                lkfree(otg_firmware_loading->otg_states);
                lkfree(otg_firmware_loading->otg_tests);
                lkfree(otg_firmware_loading);
        }
        if (otg_firmware_loaded && (otg_firmware_loaded != otg_firmware_orig)) {
                lkfree(otg_firmware_loaded->otg_states);
                lkfree(otg_firmware_loaded->otg_tests);
                lkfree(otg_firmware_loaded);
        }

        CORE = otg_trace_invalidate_tag(CORE);
        otg->TAG = NULL;

//#if defined(LINUX24)
//        otg_trace_exit_l24();
//#endif
        //otg_trace_exit();

        LKFREE(otg);
}

OTG_EXPORT_SYMBOL(otg_create);
OTG_EXPORT_SYMBOL(otg_destroy);

/* ************************************************************************************* */
int otg_trace_modinit_lnx(void);
void otg_trace_modexit_lnx(void);
int usbd_device_modinit(void);
int usbd_device_modexit(void);


/* ************************************************************************************* */
//#if defined(LINUX26)
/*! otg_match - check if the device and driver match
 *
 * @param dev - pointer to device
 * @param drv - pointer device driver
 * @return int for match result
 */

static int otg_match (struct device *dev, struct device_driver *drv)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        return 0;
}

void otg_unregister(void)
{
        printk(KERN_INFO"%s: \n", __FUNCTION__);
        usbd_device_modexit();
        otg_trace_modexit_lnx();
        printk(KERN_INFO"%s: otg_mallocs: %d\n", __FUNCTION__, otg_mallocs);
}
int otg_register(void)
{
        int bus_registered = 0;
        int driver_registered = 0;

        RETURN_EINVAL_IF(otg_trace_modinit_lnx());

        THROW_IF(usbd_device_modinit(), error);


        CATCH(error) {
                return -EINVAL;
        }
        return 0;
}


/* ************************************************************************************* */
extern u64 otg_events[64];
extern otg_tag_t otg_tags[64];
extern char * otg_msgs[64];
extern u8 otg_head, otg_tail;

//#endif

/*! otg_modinit - linux module initialization
 */
static int otg_modinit (void)
{
        otg_led_init(LED1);
        otg_led_init(LED2);
        return otg_register();
}
module_init (otg_modinit);


#if OTG_EPILOGUE  /* Set nonzero in <otg-module.h> when -DMODULE is in force */
/*! otg_modexit - This is *only* used for drivers compiled and used as a module.
 */
static void otg_modexit (void)
{
        otg_unregister();
}
#endif



#ifdef OTG_SKYE_LED
void otg_led(int led, int flag)
{
        if (led)
                mxc_set_gpio_dataout(led, flag);
}

void otg_led_init(int led)
{
        if (led)
                mxc_set_gpio_direction(led,0);
}
#else /* OTG_SKYE_LED */
void otg_led(int led, int flag)
{
}

void otg_led_init(int led)
{
}
#endif /* OTG_SKYE_LED */
OTG_EXPORT_SYMBOL(otg_led);
OTG_EXPORT_SYMBOL(otg_led_init);


MOD_EXIT(otg_modexit);
OTG_EXPORT_SYMBOL(otg_get_state_name);


#ifdef OTG_MALLOC_TEST
OTG_EXPORT_SYMBOL(otg_mallocs);
#endif /* OTG_MALLOC_TEST */

#endif /* defined(CONFIG_OTG_LNX) */
