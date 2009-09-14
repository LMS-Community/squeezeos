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
 * otg/functions/generic/generic-cf.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/generic/generic.c|20070425221028|47281
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/generic/generic.c
 * @brief Generic Configuration Function Driver
 *
 * This implements a generic composite function
 *
 *         - idVendor              16 bit word
 *         - idProduct             16 bit word
 *         - bcd_device            16 bit word
 *
 *         - bDeviceClass          byte
 *         - device_sub-class      byte
 *         - bcdDevice             byte
 *
 *         - vendor                string
 *         - iManufacturer         string
 *         - product               string
 *         - serial                string
 *
 *         - power                 byte
 *         - remote_wakeup         bit
 *
 *         - functions             string
 *
 *
 * The functions string would contain a list of names, the first would
 * be the composite function driver, the rest would be the interface
 * functions. For example:
 *
 *
 *         "cmouse-cf mouse-if"
 *         "mcpc-cf dun-if dial-if obex-if dlog-if"
 *
 * There are also a set of pre-defined configurations that
 * can be loaded singly or in toto.
 *
 * @ingroup GenericComposite
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>

#include "generic.h"

//static otg_tag_t GENERIC;


/* USB Module init/exit ***************************************************** */

/*! generic_cf_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int generic_cf_function_enable (struct usbd_function_instance *function)
{
        //TRACE_MSG0(GENERIC,"--");
        return 0;
}

/*! generic_cf_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
/*!@name generic composite functions
 * @{
 */
static void generic_cf_function_disable (struct usbd_function_instance *function)
{
        //TRACE_MSG0(GENERIC,"--");
}


/*! function_ops - operations table for the USB Device Core
 *  */
static struct usbd_function_operations generic_function_ops = {
        .function_enable = generic_cf_function_enable,
        .function_disable = generic_cf_function_disable,
};



/*! @} */

/*!
 * generic_register()
 *
 */
void generic_register(struct generic_config *config, const char *match, BOOL override, u16 idVendor, u16 idProduct,
		char *iSerialNumber, otg_tag_t tag)
{
        char *cp, *sp, *lp, *np;
        const char **interface_list = NULL;
        int interfaces = 0;
        char interface_name_list[256];

        TRACE_MSG5(tag, "Driver: \"%s\" idVendor: %04x idProduct: %04x interface_names: \"%s\" match: \"%s\"",
                        config->composite_driver.driver.name, idVendor, idProduct, config->interface_names,
                        match ? match : "");

        printk(KERN_INFO"Driver: \"%s\" idVendor: %04x idProduct: %04x interface_names: \"%s\" match: \"%s\"\n",
                        config->composite_driver.driver.name, idVendor, idProduct, config->interface_names,
                        match ? match : "");


        RETURN_IF (match && strlen(match) && strcmp(match, config->composite_driver.driver.name));

        TRACE_MSG2(tag, "loading %s interface_names: %s", match, config->interface_names);


        /* decompose interface names to construct interface_list
         */
        RETURN_UNLESS (config->interface_names && strlen(config->interface_names));

        /* count interface names and allocate _interface_names array
         */
        strncpy(interface_name_list,config->interface_names,sizeof(interface_name_list));
        interface_name_list[sizeof(interface_name_list)-1]=0;
        for (cp = sp = interface_name_list, interfaces = 0; cp && *cp; ) {
                for (; *cp && *cp == ':'; cp++);        // skip white space
                sp = cp;                                // save start of token
                for (; *cp && *cp != ':'; cp++);        // find end of token
                BREAK_IF (sp == cp);
                if (*cp) cp++;
                interfaces++;
        }

        THROW_UNLESS(interfaces, error);

        config->interfaces=interfaces;
        TRACE_MSG1(tag, "interfaces: %d", interfaces);

        THROW_UNLESS((interface_list = (const char **) CKMALLOC (sizeof (char *) * (interfaces + 1))), error);

        for (cp = sp = interface_name_list, interfaces = 0; cp && *cp; interfaces++) {
                for (; *cp && *cp == ':'; cp++);        // skip white space
                sp = cp;                                // save start of token
                for (; *cp && *cp != ':'; cp++);        // find end of token
                BREAK_IF (sp == cp);
                lp = cp;
                if (*cp) cp++;
                np = CKMALLOC(lp - sp + 2);
                strncpy(np, sp, lp - sp);
                interface_list[interfaces] = np;

                TRACE_MSG3(tag, "INTERFACE[%2d] %x \"%s\"",
                                interfaces, interface_list[interfaces], interface_list[interfaces]);
        }

        config->composite_driver.device_description = &config->device_description;
        config->composite_driver.configuration_description = &config->configuration_description;
        config->composite_driver.driver.fops = &generic_function_ops;
        config->interface_list = interface_list;

        if (override) {
                TRACE_MSG2(tag, "Overide idVendor: %04x idProduct: %04x", idVendor, idProduct);
                config->composite_driver.device_description->idVendor = idVendor;
                config->composite_driver.device_description->idProduct = idProduct;
        }

        if ((iSerialNumber) && strlen(iSerialNumber)){
                 config->composite_driver.device_description->iSerialNumber = iSerialNumber;
                               // config->device_description.iSerialNumber = iSerialNumber;
                        }

        THROW_IF (usbd_register_composite_function (
                                &config->composite_driver,
                                config->composite_driver.driver.name,
                                config->class_name,
                                config->interface_list, NULL), error);

        config->registered++;

        TRACE_MSG0(tag, "REGISTER FINISHED");

        CATCH(error) {
                TRACE_MSG0(tag, "REGISTER FAILED");
        //        otg_trace_invalidate_tag(GENERIC);
        }

}

/*!
 * generic_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the generic driver with the USB Device software.
 *
 */
int generic_modinit (struct generic_config *generic_configs,
		char *generic_config_name, BOOL override, u16 idVendor, u16 idProduct, char *iSerialNumber, otg_tag_t tag)
{
	int i;


        TRACE_MSG5(tag, "config_name: \"%s\"  Serial: \"%s\" override: %d idVendor: %04x idProduct: %04x",
                        generic_config_name ? generic_config_name : "",
                        iSerialNumber ? iSerialNumber : "",
			override, idVendor, idProduct);

	/* search for named config
	 */
	for (i = 0; ; i++) {
		struct generic_config *config = generic_configs + i;
                TRACE_MSG2(tag, "i: %d interface_names: %d", i, config->interface_names);
		BREAK_UNLESS(config->interface_names);
                generic_register(config, generic_config_name, override, idVendor, idProduct, iSerialNumber, tag);
	}
	return 0;
}

/*!
 * generic_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 *
 * @param generic_configs
 * @param tag
 * @return void
 */
void generic_modexit (struct generic_config *generic_configs, otg_tag_t tag)
{
	int i,j;
	//otg_trace_invalidate_tag(GENERIC);
	for (i = 0; ; i++) {
		struct generic_config *config = generic_configs + i;

		/* finished?
		 */
                BREAK_UNLESS(config->interface_names);

		/* continue if not registered
		 */
                CONTINUE_UNLESS(config->registered);

		/* de-register this composite function
		 */
                usbd_deregister_composite_function (&config->composite_driver);

                for (j=0;j<config->interfaces;j++) LKFREE ((char *) config->interface_list[j]);

                if (config->interface_list)
                        LKFREE(config->interface_list);
        }

	/*
        if (generic_config.registered)
                usbd_deregister_composite_function (&generic_config.composite_driver);

        if (generic_config.interface_list)
                LKFREE(generic_config.interface_list);
	*/
}

#if OTG_EPILOGUE
#endif
