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
 * otg/functions/network/basic2.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/basic2-if.c|20070220211521|10696
 *
 *      Copyright (c) 2002-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/basic2-if.c
 * @brief This file implements the required descriptors to implement
 * a basic network device with two interfaces.
 *
 * The BASIC2 network driver implements a very simple descriptor set.
 * Two interfaces with two BULK data endpoints and a optional
 * INTERRUPT endpoint.
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include "network.h"

#define NTT network_fd_trace_tag

#if defined(CONFIG_OTG_NETWORK_BASIC) || defined(_OTG_DOXYGEN)

/* USB BASIC Configuration ******************************************************************** */

/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description basic2_nodata_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra USBLAN - Basic2",
                .bInterfaceClass = LINEO_CLASS,
                .bInterfaceSubClass =  LINEO_SUBCLASS_BASIC_NET,
                .bInterfaceProtocol = 0,
                .endpoints =  1,
                .endpoint_index = cdc_int_endpoint_index,
        },
};
/*! @var  struct usbd_alternate_description basic2_data_alternate_descriptions
 *  @brief an array of alternate descriptions
 *
 */
  static struct usbd_alternate_description basic2_data_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra USBLAN - Basic2 Data",
                .bInterfaceClass = LINEO_CLASS,
                .bInterfaceSubClass =  LINEO_SUBCLASS_BASIC_NET,
                .bInterfaceProtocol = LINEO_BASIC_NET_CRC,
                .endpoints =  2,
                .endpoint_index = cdc_data_endpoint_index,
        },
};


/* BASIC Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description basic2_interfaces[] = {
        {
                .alternates = sizeof (basic2_nodata_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = basic2_data_alternate_descriptions,
        },
        {
                .alternates = sizeof (basic2_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = basic2_data_alternate_descriptions,
        },
};


/* ********************************************************************************************* */

/*! basic2_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int basic2_fd_function_enable (struct usbd_function_instance *function_instance)
{
        return net_fd_function_enable(function_instance,
                        network_blan, net_fd_recv_urb_mdlm,
                        net_fd_start_xmit_mdlm,
                        net_fd_start_recv_mdlm, 0
                        );
}


/* ********************************************************************************************* */
/*! basic2_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations basic2_fd_function_ops = {
        .function_enable = basic2_fd_function_enable,
        .function_disable = net_fd_function_disable,

        .device_request = net_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .endpoint_cleared = net_fd_endpoint_cleared,
        .set_configuration = net_fd_set_configuration,
        .set_interface = NULL,
        .reset = net_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};


/*! function driver description
 */
struct usbd_interface_driver basic2_interface_driver = {
        .driver = {
                .name = "net-basic2-if",
                .fops = &basic2_fd_function_ops, },
        .interfaces = sizeof (basic2_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = basic2_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = net_fd_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - Basic2",
};

/* ********************************************************************************************* */

/*!
 * @brief basic2_mod_init
 * @return none
 */
int basic2_mod_init (void)
{
        return usbd_register_interface_function (&basic2_interface_driver, "net-basic2-if", NULL);
}

/*!
 * basic2_mod_exit
 * @return void
 */

void basic2_mod_exit(void)
{
        usbd_deregister_interface_function (&basic2_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_BASIC */
/*!
 * @brief basic2_mod_init
 * @return none
 */
int basic2_mod_init (void)
{
        return 0;
}

void basic2_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_BASIC */
