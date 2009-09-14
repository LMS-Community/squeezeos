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
 * otg/functions/generic/inteltest-cl.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/generic/inteltest-cl.c|20070425221028|59534
 *
 *	Copyright (c) 2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @addgroup GenericClass Generic Class Function
 * @ingroup ClassFunctions
 */

/*!
 * @file otg/functions/generic/inteltest-cl.c
 * @brief Generic Class Function Driver
 *
 * This implements a inteltest NULL class function driver.
 *
 * @ingroup GenericClass
 */

#include <otg/otg-compat.h>

#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

//#include "inteltest.h"

/* Module Parameters ******************************************************** */
/* !
 * @name XXXXX MODULE Parameters
 */
/* ! @{ */

MOD_AUTHOR ("sl@belcarra.com");
EMBED_LICENSE();
MOD_DESCRIPTION ("Belcarra Generic NULL Class Function");

otg_tag_t GCLASS;

/* ! *} */

/* ********************************************************************************************* */
/*!
 * inteltest_cl_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero for failure, will cause endpoint zero stall
 */
static int inteltest_cl_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_urb *urb;
        u16 wLength = le16_to_cpu(request->wLength);


        TRACE_MSG5(GCLASS, "bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x wLength: %04x",
                        request->bmRequestType, request->bRequest, request->wValue,
                        request->wIndex, request->wLength);

        /* XXX it should be this, need to verify
         */
        RETURN_EINVAL_UNLESS(USB_REQ_RECIPIENT_DEVICE == (request->bmRequestType & USB_REQ_RECIPIENT_MASK));

        switch (ctrl->bRequest) {
        switch (request->bmRequestType & USB_REQ_DIRECTION_MASK) {
        case USB_REQ_DEVICE2HOST:

                switch (request->bRequest) {
                case 0x5c:      /* read test */
                        RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb_ep0 (function, wLength, NULL)));
                        RETURN_ZERO_UNLESS(rc || usbd_start_in_urb(urb));
                        break;
                }
                break;

        case USB_REQ_HOST2DEVICE:

                switch (request->bRequest) {
                case 0x5b:      /* write test */
                        RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb_ep0 (function, wLength, NULL)));
                        RETURN_ZERO_UNLESS(rc || usbd_start_out_urb(urb));
                        break;
                }
                break;
        }

        return -EINVAL;
}
/* ********************************************************************************************* */
#if !defined(OTG_C99)
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations inteltest_function_ops;

/*! inteltest_class_driver - USB Device Core function driver definition
 */
struct usbd_class_driver inteltest_class_driver;

#else /* defined(OTG_C99) */
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations inteltest_function_ops = {
        .device_request = inteltest_cl_device_request,           /*!< called for each received device request */
};

/*! inteltest_class_driver - USB Device Core function driver definition
 */
struct usbd_class_driver inteltest_class_driver = {
        .driver.name = "inteltest-class",                            /*!< driver name */
        .driver.fops = &inteltest_function_ops,                             /*!< operations table */
};
#endif /* defined(OTG_C99) */


/* USB Module init/exit ***************************************************** */

//#if OTG_EPILOGUE
/*!
 * inteltest_cl_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded
 * if compiled as a module. This function is never called if the
 * driver is linked into the kernel.
 * @return none
 */
static void inteltest_cl_modexit (void)
{
        usbd_deregister_class_function (&inteltest_class_driver);
        otg_trace_invalidate_tag(GCLASS);
}

module_exit (inteltest_cl_modexit);
//#endif

/*!
 * inteltest_cl_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the inteltest driver with the USB Device software.
 *
 */
static int inteltest_cl_modinit (void)
{
        #if !defined(OTG_C99)
        /*! function_ops - operations table for the USB Device Core
         */
        ZERO(inteltest_function_ops);
        inteltest_function_ops.device_request=inteltest_cl_device_request;          /*! called for each received device request */

        /*! class_driver - USB Device Core function driver definition
         */
        ZERO(inteltest_class_driver);
        inteltest_class_driver.driver.name = "inteltest-class";                            /*! driver name */
        inteltest_class_driver.driver.fops = &inteltest_function_ops;                             /*! operations table */
        #endif /* defined(OTG_C99) */

        GCLASS = otg_trace_obtain_tag(NULL, "inteltest-cf");

        // register as usb function driver
        TRACE_MSG0(GCLASS, "REGISTER CLASS");
        THROW_IF (usbd_register_class_function (&inteltest_class_driver, "inteltest-class", NULL), error);

        TRACE_MSG0(GCLASS, "REGISTER FINISHED");

        CATCH(error) {
                inteltest_cl_modexit();
                return -EINVAL;
        }
        return 0;
}

module_init (inteltest_cl_modinit);
#endif /* defined(CONFIG_OTG_LNX) */
