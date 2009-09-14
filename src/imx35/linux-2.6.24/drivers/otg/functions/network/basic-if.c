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
 * otg/functions/network/basic.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/basic-if.c|20070220211521|39075
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
 * @file otg/functions/network/basic-if.c
 * @brief This file implements the required descriptors to implement
 * a basic network device with a single interface.
 *
 * The BASIC network driver implements a very simple descriptor set.
 * A single interface with two BULK data endpoints and a optional
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
static struct usbd_alternate_description basic_data_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra USBLAN - MDLM/BLAN",
                .bInterfaceClass = LINEO_CLASS,
                .bInterfaceSubClass =  LINEO_SUBCLASS_BASIC_NET,
                .bInterfaceProtocol = LINEO_BASIC_NET_CRC,
                .endpoints =  ENDPOINTS,
                .endpoint_index = net_fd_endpoint_index,
        },
};


/* BASIC Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description basic_interfaces[] = {
        {
                .alternates = sizeof (basic_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = basic_data_alternate_descriptions,
        },
};


/* ********************************************************************************************* */

/*! int basic_fd_function_enable( struct usbd_function_instance)
 * @brief -enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 *
 * @param function_instance - pointer to this function intance
 * @return 0 for success
 */

int basic_fd_function_enable (struct usbd_function_instance *function_instance)
{
        return net_fd_function_enable(function_instance,
                        network_blan, net_fd_recv_urb_mdlm,
                        net_fd_start_xmit_mdlm,
                        net_fd_start_recv_mdlm, 0
                        );
}


/* ********************************************************************************************* */
/*! basic_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations basic_fd_function_ops = {
        .function_enable = basic_fd_function_enable,
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
struct usbd_interface_driver basic_interface_driver = {
        .driver = {
                .name = "net-basic-if",
                .fops = &basic_fd_function_ops, },
        .interfaces = sizeof (basic_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = basic_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = net_fd_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - Basic",
};

/* ********************************************************************************************* */

/*!int basic_mod_init()
 * @brief  initialize interface module throught registering driver
 * @return int
 */
int basic_mod_init (void)
{
        return usbd_register_interface_function (&basic_interface_driver, "net-basic-if", NULL);
}

/*!
* @brief basic_mod_exit
 * @return none
 */
void basic_mod_exit(void)
{
        usbd_deregister_interface_function (&basic_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_BASIC */
/*!
 * @brief basic_mod_init
 * @return int
 */
int basic_mod_init (void)
{
        return 0;
}

/*!
 * @brief  basic_mod_exit
 * @return none
 */
void basic_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_BASIC */
