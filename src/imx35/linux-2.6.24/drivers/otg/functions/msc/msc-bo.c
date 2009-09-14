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
 * otg/function/msc/msc-bo.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/msc-bo.c|20070425221028|52337
 *
 *      Copyright (c) 2003-2004 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/msc/msc-bo.c
 * @brief Mass Storage Driver private defines
 *
 * This is a Mass Storage Class Function that uses the Bulk Only protocol.
 *
 *
 * @ingroup MSCFunction
 */

#include <otg/otg-compat.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/random.h>
#include <linux/slab.h>

#if defined(LINUX26)
#include <linux/buffer_head.h>
#endif
#include <linux/kdev_t.h>
#include <linux/blkdev.h>


#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#include "crc.h"

//#include "rbc.h"

# define CONFIG_OTG_SERIAL_NUMBER_STR   "69D38D2036248D3A983C5CDA63888"

/*!
 * Mass Storage Class - Bulk Only
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */

u8 msc_index[] = { BULK_OUT, BULK_IN, };

extern struct usbd_function_operations msc_function_ops;



static struct usbd_endpoint_request msc_if_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, PAGE_SIZE, PAGE_SIZE, 0, },
        { BULK_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, PAGE_SIZE, PAGE_SIZE, 0, },
        { 0, },
};


#ifndef OTG_C99

//#warning "C99"
static struct usbd_alternate_description msc_data_alternate_descriptions[1];

struct usbd_interface_description msc_interfaces[] = {
        { alternates:sizeof (msc_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                alternate_list:msc_data_alternate_descriptions,},
};

struct usbd_interface_driver msc_interface_driver;
/*! msc_global_init - initialize global variables for msc instance
 * @return void
 */

void msc_global_init(void){

        ZERO(msc_data_alternate_descriptions[0]);

        msc_data_alternate_descriptions[0].iInterface = CONFIG_OTG_MSC_INTF;
        msc_data_alternate_descriptions[0].bInterfaceClass = MASS_STORAGE_CLASS;
        msc_data_alternate_descriptions[0].bInterfaceSubClass =  MASS_STORAGE_SUBCLASS_SCSI;
        msc_data_alternate_descriptions[0].bInterfaceProtocol = MASS_STORAGE_PROTO_BULK_ONLY;
        msc_data_alternate_descriptions[0].endpoints = 0x02,
                msc_data_alternate_descriptions[0].endpoint_index =  msc_index;



        ZERO(msc_interface_driver);

        msc_interface_driver.driver.name = "msc-if";
        msc_interface_driver.driver.fops = &msc_function_ops;
        msc_interface_driver.interfaces = 0x1;
        msc_interface_driver.interface_list = msc_interfaces;
        msc_interface_driver.endpointsRequested = ENDPOINTS;
        msc_interface_driver.requestedEndpoints = msc_if_endpoint_requests;

}



#else /* OTG_C99 */
//#warning "Not C99"

static struct usbd_alternate_description msc_data_alternate_descriptions[] = {
        {
                .iInterface = "Belcarra Mass Storage - Bulk Only",
                .bInterfaceClass = MASS_STORAGE_CLASS,
                .bInterfaceSubClass =  MASS_STORAGE_SUBCLASS_SCSI,
                .bInterfaceProtocol = MASS_STORAGE_PROTO_BULK_ONLY,
                .endpoints = 0x02,
                .endpoint_index =  msc_index,
        },
};



struct usbd_interface_description msc_interfaces[] = {
        { alternates:sizeof (msc_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
alternate_list:msc_data_alternate_descriptions,},
};


/*! msc_interface_driver - USB Device Core function driver definition
 */
struct usbd_interface_driver msc_interface_driver;

struct usbd_interface_driver msc_interface_driver = {
        .driver = {
		.name = "msc-if",
                .fops = &msc_function_ops, },
//        .driver.name = "msc-if",                            /*! driver name */
//        .driver.fops = &msc_function_ops,                             /*!< operations table */
        .interfaces = 0x1,
        .interface_list = msc_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = msc_if_endpoint_requests,

        .bFunctionClass = MASS_STORAGE_CLASS,
        .bFunctionSubClass = MASS_STORAGE_SUBCLASS_SCSI,
        .bFunctionProtocol = MASS_STORAGE_PROTO_BULK_ONLY,
        .iFunction = "Belcarra Mass Storage - Bulk Only",
};






#endif /* OTG_C99 */
