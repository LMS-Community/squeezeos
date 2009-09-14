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
 * otg/otgcore/usbp-bops.c - USB Device Prototype
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/usbp-bops.c|20070816060612|46629
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/otgcore/usbp-bops.c
 * @brief Bus Interface related functions.
 *
 * This implements the functions used to implement Peripheral Controller Drivers (PCD.)
 *
 * @ingroup USBDCORE
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
//#include <otg/otg-task.h>
#include <otg/otg-trace.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-pcd.h>

int usbd_procfs_init (struct usbd_bus_instance *);
void usbd_procfs_exit (struct usbd_bus_instance *);


struct usbd_simple_driver *usbp_find_simple_driver(char *);
struct usbd_simple_instance *usbp_alloc_simple_instance(struct usbd_simple_driver *, struct usbd_bus_instance *);
void usbp_dealloc_simple_instance(struct usbd_simple_instance *simple_instance);

void usbp_alloc_simple_configuration_descriptor(struct usbd_simple_instance *simple_instance, int cfg_size, int hs);

struct usbd_composite_driver *usbp_find_composite_driver(char *);
struct usbd_composite_instance *usbp_alloc_composite_instance(struct usbd_composite_driver *, struct usbd_bus_instance *);

void usbp_dealloc_composite_instance(struct usbd_composite_instance *composite_instance);

void usbp_alloc_composite_configuration_descriptor(struct usbd_composite_instance *composite_instance, int cfg_size, int hs);

void *usbd_device_reset(void *data);
void *usbd_device_suspended(void *data);
void *usbd_device_resumed(void *data);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Note that all of the list functions overlapped operation using the usbd_bus_sem;
 *
 *      usbd_device_bh
 *
 *      usbd_register_bus
 *      usbd_deregister_bus
 *      usbd_enable_function
 *      usbd_disable_function
 */
//DECLARE_MUTEX(usbd_bus_sem);
otg_sem_t usbd_bus_sem;

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Bus Interface Management:
 *      usbd_register_bus()
 *      usbd_deregister_bus()
 */

/*! urb_link_init() - initialize urb linke
 *
 * Initialize an urb_link to be a single element list.
 * If the urb_link is being used as a distinguished list head
 * the list is empty when the head is the only link in the list.
 *
 * @param ul link to urb
 */
static INLINE void urb_link_init (urb_link * ul)
{
        ul->prev = ul->next = ul;
}

struct usbd_endpoint_map ep0_endpoint_map_array[2];

#if defined(OTG_C99)
struct usbd_interface_instance ep0_instance = {
        .function = {
                .name = "EP0",
                .function_type = function_ep0, },
        .endpoint_map_array = ep0_endpoint_map_array,
};
#else /* defined(OTG_C99) */
struct usbd_interface_instance ep0_instance;
#endif /* defined(OTG_C99) */


/*!
 * usbd_register_bus() - called by a USB BUS INTERFACE driver to register a bus driver
 *
 * Used by a USB Bus interface driver to register itself with the usb device layer.
 *
 * @param driver
 * @param ep0_wMaxPacketSize
 * @return non-zero if error
 */
struct usbd_bus_instance *usbd_register_bus (struct usbd_bus_driver *driver, int ep0_wMaxPacketSize)
{
        int i;
        struct usbd_bus_instance *bus = NULL;

        RETURN_NULL_IF(otg_sem_wait(&usbd_bus_sem));

        THROW_UNLESS((bus = CKMALLOC (sizeof (struct usbd_bus_instance))), error);
        THROW_UNLESS((bus->reset_bh = otg_workitem_init("busreset", usbd_device_reset, bus, USBD)), error);
        THROW_UNLESS((bus->resume_bh = otg_workitem_init("busact", usbd_device_resumed, bus, USBD)), error);
        THROW_UNLESS((bus->suspend_bh = otg_workitem_init("busidle", usbd_device_suspended, bus, USBD)), error);


        //bus->reset_bh->debug = TRUE;
        //bus->resume_bh->debug = TRUE;
        //bus->suspend_bh->debug = TRUE;

        bus->driver = driver;
        bus->endpoints = bus->driver->max_endpoints;
        bus->bmAttributes = bus->driver->bmAttributes;
        bus->bMaxPower = bus->driver->bMaxPower;
        TRACE_MSG2(USBD, "bmAttributes: %02x bMaxPower: %02x", bus->bmAttributes, bus->bMaxPower);


        THROW_IF(!(bus->endpoint_array = CKMALLOC(sizeof (struct usbd_endpoint_instance) * bus->endpoints)), error);

        for (i = 0; i < bus->endpoints; i++) {
                struct usbd_endpoint_instance *endpoint = bus->endpoint_array + i;
                //endpoint->physical_endpoint = i;
                urb_link_init (&endpoint->rdy);
                endpoint->bus = bus;
                //init_MUTEX(&endpoint->sem);
        }

        bus->ep0 = &ep0_instance;
        bus->ep0->function.bus = bus;

        #if !defined(OTG_C99)
        bus->ep0->function.name = "EP0";
        bus->ep0->function.function_type = function_ep0;
        bus->ep0->endpoint_map_array = ep0_endpoint_map_array;
        #endif /* defined(OTG_C99) */

        for (i = 0; i < 2; i++) {
                bus->ep0->endpoint_map_array[i].wMaxPacketSize[0] =
                        bus->ep0->endpoint_map_array[i].wMaxPacketSize[1] = ep0_wMaxPacketSize;
                bus->ep0->endpoint_map_array[i].endpoint = bus->endpoint_array;
        }

        bus->device_state = STATE_CREATED;
        bus->status = USBD_OPENING;
        TRACE_MSG1(USBD, "bus->status: %d", bus->status);

        //new_usbd_bus_instance = bus;

        #if defined(CONFIG_OTG_TRACE)
	THROW_IF(usbd_procfs_init (bus), error);
        #endif /* defined(CONFIG_OTG_TRACE) */

        CATCH(error) {
                if (bus) {
                        if (bus->reset_bh) otg_workitem_exit(bus->reset_bh);
                        if (bus->resume_bh) otg_workitem_exit(bus->resume_bh);
                        if (bus->suspend_bh) otg_workitem_exit(bus->suspend_bh);
                        LKFREE(bus);
                }
                bus->endpoints = 0;
                bus =  NULL;
        }
        otg_sem_post(&usbd_bus_sem);
        return bus;
}

/*!
 * usbd_deregister_bus() - called by a USB BUS INTERFACE driver to deregister a bus driver
 *
 * Used by a USB Bus interface driver to de-register itself with the usb device
 * layer.
 *
 * @param bus
 */
void usbd_deregister_bus (struct usbd_bus_instance *bus)
{
        while (otg_sem_wait(&usbd_bus_sem));

        #if defined(CONFIG_OTG_TRACE)
        usbd_procfs_exit (bus);
        #endif /* defined(CONFIG_OTG_TRACE) */
        //new_usbd_bus_instance = NULL;

        if (bus->reset_bh) otg_workitem_exit(bus->reset_bh);
        if (bus->resume_bh) otg_workitem_exit(bus->resume_bh);
        if (bus->suspend_bh) otg_workitem_exit(bus->suspend_bh);
        LKFREE (bus->arg);
        LKFREE (bus->endpoint_array);
        LKFREE (bus);
        //bus->endpoints = 0;
        otg_sem_post(&usbd_bus_sem);
}

OTG_EXPORT_SYMBOL(usbd_register_bus);
OTG_EXPORT_SYMBOL(usbd_deregister_bus);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * bus_function_enable() - enable a usbd function driver for use
 *
 * This will copy descriptors from the function driver and get them ready
 * for use.
 * @param bus
 * @param function
 * @param serial_number
 * @return non-zero if error
 */
int
bus_function_enable (struct usbd_bus_instance *bus, struct usbd_function_instance *function, char *serial_number)
{
        struct usbd_simple_instance *simple_instance = NULL;
        struct usbd_simple_driver *simple_driver = NULL;
        struct usbd_composite_instance *composite_instance = NULL;
        struct usbd_composite_driver *composite_driver = NULL;

        struct usbd_device_description *device_description = NULL;
        //struct usbd_device_descriptor *device_descriptor = NULL;
#ifdef CONFIG_OTG_HIGH_SPEED
        struct usbd_device_qualifier_descriptor *device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */
        //struct usbd_otg_descriptor *otg_descriptor;
        int i;
        int rc = 0;

        TRACE_MSG1(USBD, "function: %x", function);

        /* point functions at the bus
         */
        function->bus = bus;

        if (function_simple == function->function_type) {
                simple_instance = (struct usbd_simple_instance *)function;
                simple_driver = (struct usbd_simple_driver *)simple_instance->function.function_driver;
                simple_driver->driver.flags |= FUNCTION_ENABLED;
                /* call enable function
                 */
                if (simple_driver->driver.fops->function_enable)
                        simple_driver->driver.fops->function_enable (function);

                /* update device descriptor
                 */
                RETURN_ZERO_IF(!(device_description = simple_driver->device_description));
        }
        if (function_composite == function->function_type) {

                composite_instance = (struct usbd_composite_instance *)function;
                composite_driver = (struct usbd_composite_driver *)composite_instance->function.function_driver;

                TRACE_MSG0(USBD, "enable composite");

                /* call enable functions
                 */
                if (composite_driver->driver.fops->function_enable)
                        composite_driver->driver.fops->function_enable (function);
                composite_driver->driver.flags |= FUNCTION_ENABLED;

                TRACE_MSG0(USBD, "enable class");
                if (composite_instance->class_instance) {
                        struct usbd_class_instance *class_instance = composite_instance->class_instance;
                        struct usbd_class_driver *class_driver =
                                (struct usbd_class_driver *)class_instance->function.function_driver;

                        composite_instance->class_instance->function.bus = bus;
                        if (class_driver->driver.fops->function_enable)
                                class_driver->driver.fops->function_enable
                                        ((struct usbd_function_instance *)class_instance);
                        class_driver->driver.flags |= FUNCTION_ENABLED;
                }
                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interfaces_array = composite_instance->interfaces_array;
                        struct usbd_interface_instance *interface_instance = interfaces_array + i;
                        struct usbd_interface_driver *interface_driver;
                        CONTINUE_UNLESS(interface_instance);
                        TRACE_MSG1(USBD, "enable interface[%d]", i);
                        interface_driver = (struct usbd_interface_driver *)interface_instance->function.function_driver;
                        interface_instance->function.bus = bus;
                        if (interface_driver->driver.fops->function_enable)
                                interface_driver->driver.fops->function_enable
                                        ((struct usbd_function_instance *)interface_instance);
                        interface_driver->driver.flags |= FUNCTION_ENABLED;
                }

                /* update device descriptor
                 */
                RETURN_ZERO_IF(!(device_description = composite_driver->device_description));
        }


        //RETURN_ZERO_IF(!(device_descriptor = device_description->device_descriptor));

        device_description->bMaxPacketSize0 = bus->driver->maxpacketsize;
        #if 0
        if (strlen (serial_number))
                device_descriptor->iSerialNumber = usbd_alloc_string (serial_number);
        else
                device_descriptor->iSerialNumber = usbd_alloc_string (device_description->iSerialNumber);
        #endif

        #if 0
        // XXX not used
        if (function_simple == function->function_type) {

                if ((otg_descriptor = device_description->otg_descriptor)) {
                        otg_descriptor->bmAttributes = usbd_otg_bmattributes(function);
                        simple_driver->otg_descriptor = otg_descriptor;
                }

                //simple_instance->device_descriptor = device_descriptor;

        }
        #endif
        #if 0
        // XXX not used
        if (function_composite == function->function_type) {
                if ((otg_descriptor = device_description->otg_descriptor)) {
                        otg_descriptor->bmAttributes = usbd_otg_bmattributes(function);
                        composite_driver->otg_descriptor = otg_descriptor;
                }

        }
        #endif
        return rc;
}

/*!
 * bus_function_disable() - disable a usbd function driver for use
 *
 * This will copy descriptors from the function driver and get them ready
 * for use.
 * @param bus
 * @param function
 * @return non-zero if error
 */
int
bus_function_disable (struct usbd_bus_instance *bus, struct usbd_function_instance *function)
{
        struct usbd_simple_instance *simple_instance = NULL;
        struct usbd_simple_driver *simple_driver = NULL;
        struct usbd_composite_instance *composite_instance = NULL;
        struct usbd_composite_driver *composite_driver = NULL;

        struct usbd_device_description *device_description = NULL;
        //struct usbd_device_descriptor *device_descriptor = NULL;
#ifdef CONFIG_OTG_HIGH_SPEED
        struct usbd_device_qualifier_descriptor *device_qualifier_descriptor;
#endif /* CONFIG_OTG_HIGH_SPEED */
        //struct usbd_otg_descriptor *otg_descriptor;
        int i;
        int rc = 0;

        TRACE_MSG1(USBD, "function: %x", function);

        /* point functions at the bus
         */
        function->bus = bus;

        if (function_simple == function->function_type) {
                simple_instance = (struct usbd_simple_instance *)function;
                simple_driver = (struct usbd_simple_driver *)simple_instance->function.function_driver;
                simple_driver->driver.flags &= ~FUNCTION_ENABLED;
                /* call disable function
                 */
                if (simple_driver->driver.fops->function_disable)
                        simple_driver->driver.fops->function_disable (function);

                /* update device descriptor
                 */
                RETURN_ZERO_IF(!(device_description = simple_driver->device_description));
        }
        if (function_composite == function->function_type) {

                composite_instance = (struct usbd_composite_instance *)function;
                composite_driver = (struct usbd_composite_driver *)composite_instance->function.function_driver;

                TRACE_MSG0(USBD, "disable composite");

                /* call disable functions
                 */
                if (composite_driver->driver.fops->function_disable)
                        composite_driver->driver.fops->function_disable (function);
                composite_driver->driver.flags &= ~FUNCTION_ENABLED;

                TRACE_MSG0(USBD, "disable class");
                if (composite_instance->class_instance) {
                        struct usbd_class_instance *class_instance = composite_instance->class_instance;
                        struct usbd_class_driver *class_driver =
                                (struct usbd_class_driver *)class_instance->function.function_driver;

                        composite_instance->class_instance->function.bus = bus;
                        if (class_driver->driver.fops->function_disable)
                                class_driver->driver.fops->function_disable
                                        ((struct usbd_function_instance *)class_instance);
                        class_driver->driver.flags |= FUNCTION_ENABLED;
                }
                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interfaces_array = composite_instance->interfaces_array;
                        struct usbd_interface_instance *interface_instance = interfaces_array + i;
                        struct usbd_interface_driver *interface_driver;
                        CONTINUE_UNLESS(interface_instance);
                        TRACE_MSG1(USBD, "disable interface[%d]", i);
                        interface_driver = (struct usbd_interface_driver *)interface_instance->function.function_driver;
                        interface_instance->function.bus = bus;
                        if (interface_driver->driver.fops->function_disable)
                                interface_driver->driver.fops->function_disable
                                        ((struct usbd_function_instance *)interface_instance);
                        interface_driver->driver.flags |= FUNCTION_ENABLED;
                }
        }
        return rc;
}

/*!
 * Function Management:
 *      usbd_enable_function()
 *      usbd_disable_function()
 *
 */

/*!
 * usbd_enable_function() - called to enable the desired function
 *
 * Used by a USB Bus interface driver to create a virtual device.
 *
 * @param bus
 * @param arg
 * @param serial_number
 * @return non-zero if error
 */
int usbd_enable_function(struct usbd_bus_instance *bus, char *arg, char *serial_number)
{
        struct usbd_composite_driver *composite_driver = NULL;
        struct usbd_composite_instance *composite_instance = NULL;

        struct usbd_simple_driver *simple_driver = NULL;
        struct usbd_simple_instance *simple_instance = NULL;

        //struct usbd_configuration_descriptor *configuration_descriptor = NULL;
        struct usbd_endpoint_map *endpoint_map_array = NULL;
        struct usbd_endpoint_request *requestedEndpoints = NULL;

        //struct otg_list_node *lhd = NULL;
        int len = 0;
        int i;
        int rc = -EINVAL;
        int epn;
        int endpointsRequested = 0;
        //int endpoints;

        TRACE_STRING(USBD, "serial_number: %s", serial_number);

        RETURN_EINVAL_IF(otg_sem_wait(&usbd_bus_sem));

        if (arg && bus->arg)
                LKFREE(bus->arg);

        if (arg) {
                bus->arg = LSTRDUP(arg);
                len = strlen(arg);
        }

        for (i = 1; i < bus->endpoints; i++) {
                struct usbd_endpoint_instance *endpoint = bus->endpoint_array + i;
                endpoint->active_urb = NULL;
        }

        TRACE_STRING(USBD, "arg: %s", arg);

        //PREPARE_WORK_ITEM(bus->reset_bh, usbd_device_reset, bus);
        //PREPARE_WORK_ITEM(bus->suspend_bh, usbd_device_suspended, bus);
        //PREPARE_WORK_ITEM(bus->resume_bh, usbd_device_resumed, bus);
        bus->status = USBD_OK;
        TRACE_MSG1(USBD, "bus->status: %d", bus->status);

        /* find requested function
         */
        if ((simple_driver = usbp_find_simple_driver(arg))) {

                TRACE_MSG0(USBD, "simple");

                // allocate simple_instance
                //
                THROW_UNLESS((simple_instance = usbp_alloc_simple_instance (simple_driver, bus)), error);
                bus->function_instance = (struct usbd_function_instance *)simple_instance;

                #if 0
                simple_driver->iManufacturer =
                        usbd_realloc_string (&simple_instance->function, strindex_product,
                                        simple_driver->device_description->iManufacturer);
                #else
                simple_driver->iManufacturer = usbd_alloc_string (&simple_instance->function,
                                simple_driver->device_description->iManufacturer);
                #endif
                simple_driver->iProduct = usbd_alloc_string (&simple_instance->function,
                                simple_driver->device_description->iProduct);

                if (serial_number && strlen (serial_number))
                        simple_driver->iSerialNumber = usbd_alloc_string (&simple_instance->function, serial_number);
                else
                        simple_driver->iSerialNumber = usbd_alloc_string (&simple_instance->function,
                                        simple_driver->device_description->iSerialNumber);

                bus_function_enable (bus, &simple_instance->function, serial_number);

                requestedEndpoints = simple_driver->requestedEndpoints;
                endpointsRequested = simple_driver->endpointsRequested;
                endpoint_map_array = simple_instance->endpoint_map_array;
        }

        else if ((composite_driver = usbp_find_composite_driver(arg))) {

                TRACE_MSG0(USBD, "composite");
                THROW_UNLESS((composite_instance = usbp_alloc_composite_instance (composite_driver, bus)), error);

                TRACE_MSG1(USBD, "composite: composite_instance: %x", composite_instance);

                bus->function_instance = (struct usbd_function_instance *)composite_instance;

                /* CLASS MISC devices must have SubClass set to 0x2 and Protocol set to 0x1
                 */
                if ((USB_CLASS_MISC == composite_driver->device_description->bDeviceClass) ) {
                        composite_driver->device_description->bDeviceSubClass = 0x2;
                        composite_driver->device_description->bDeviceProtocol = 0x1;
                }

                #if 0
                composite_driver->iManufacturer =
                        usbd_realloc_string (&composite_instance->function, strindex_product,
                                        composite_driver->device_description->iManufacturer);
                #else
                composite_driver->iManufacturer = usbd_alloc_string (&composite_instance->function,
                                composite_driver->device_description->iManufacturer);
                #endif
                composite_driver->iProduct = usbd_alloc_string (&composite_instance->function,
                                composite_driver->device_description->iProduct);
                if (serial_number && strlen (serial_number))
                        composite_driver->iSerialNumber = usbd_alloc_string (&composite_instance->function, serial_number);
                else
                        composite_driver->iSerialNumber = usbd_alloc_string (&composite_instance->function,
                                        composite_driver->device_description->iSerialNumber);

                //TRACE_MSG3(USBD, "iManufacturer: %d iProduct: %d iSerialNumber: %d", composite_driver->iManufacturer,
                //                composite_driver->iProduct, composite_driver->iSerialNumber);

                TRACE_MSG0(USBD, "composite - enable");
                bus_function_enable (bus, &composite_instance->function, serial_number);

                requestedEndpoints = composite_instance->requestedEndpoints;
                endpointsRequested = composite_instance->endpointsRequested;
                endpoint_map_array = composite_instance->endpoint_map_array;
        }
        else
                THROW(error);


        /* Request endpoints from bus interface driver
         */
        THROW_IF(bus->driver->bops->request_endpoints( bus, endpoint_map_array, endpointsRequested, requestedEndpoints), error);

        /* create configuration descriptors
         */
        #ifdef CONFIG_OTG_HIGH_SPEED
        for (i = 0; i < 2; i++)
        #else /* CONFIG_OTG_HIGH_SPEED */
        for (i = 0; i < 1; i++)
        #endif /* CONFIG_OTG_HIGH_SPEED */
        {
                if (simple_instance)
                        usbp_alloc_simple_configuration_descriptor (simple_instance,
                                        simple_instance->configuration_size, i);
                else if (composite_instance)
                        usbp_alloc_composite_configuration_descriptor (composite_instance,
                                        composite_instance->configuration_size, i);
        }

        /* set the endpoints in the peripheral driver
         */
        THROW_IF(bus->driver->bops->set_endpoints( bus, endpointsRequested, endpoint_map_array), error);

        /* iterate across the logical endpoint map to copy appropriate information
         * into the physical endpoint instance array
         */
        for (epn = 0; epn < endpointsRequested; epn++) {

                struct usbd_endpoint_map *endpoint_map = endpoint_map_array + epn;
                int physicalEndpoint = endpoint_map->physicalEndpoint[0];
                struct usbd_endpoint_instance *endpoint = bus->endpoint_array + physicalEndpoint;
                int hs;

                endpoint_map->endpoint = endpoint;

                for (hs = 0; hs < 2; hs++) {

                        endpoint->physicalEndpoint[hs] = endpoint_map->physicalEndpoint[0];
                        endpoint->bEndpointAddress[hs] = endpoint_map->bEndpointAddress[hs];
                        endpoint->bmAttributes[hs] = endpoint_map->bmAttributes[hs];
                        endpoint->transferSize[hs] = endpoint_map->transferSize[hs];
                        endpoint->wMaxPacketSize[hs] = endpoint_map->wMaxPacketSize[hs];

                        switch(endpoint->bEndpointAddress[hs] & USB_ENDPOINT_DIR_MASK) {
                        case USB_DIR_IN:
                                endpoint->tx_transferSize = endpoint_map->transferSize[hs];
                                endpoint->last = 0;
                                endpoint->tx_urb = NULL;
                                break;

                        case USB_DIR_OUT:
                                endpoint->rcv_transferSize = endpoint_map->transferSize[hs];
                                endpoint->rcv_urb = NULL;
                                break;
                        }

                        TRACE_MSG4(USBD, "endpoint[%d]: %x wMaxPacketSize: %02x %02x",
                                hs, endpoint,
                                endpoint->wMaxPacketSize[hs],
                                endpoint_map->wMaxPacketSize[hs]);
                }

        }
        bus->ep0->endpoint_map_array->endpoint = bus->endpoint_array;

        /* endpoint zero has no wMaxPacketSize set yet... so alloc buffer separately */
        THROW_UNLESS(bus->ep0_urb = usbd_alloc_urb((struct usbd_function_instance *) bus->ep0, 0, 0, NULL), error);
        THROW_UNLESS((bus->ep0_urb->buffer = (u8 *)KMALLOC (512)), error);

        rc = 0;
        CATCH(error) {
                if (simple_instance)
                        usbp_dealloc_simple_instance(simple_instance);
                if (composite_instance)
                        usbp_dealloc_composite_instance(composite_instance);

        }
        otg_sem_post(&usbd_bus_sem);
        return rc;
}

#if 0
/*!
 * usbd_enable_function_irq () -
 *
 * @param bus
 * @param arg
 * @param serial_number
 */
int usbd_enable_function_irq (struct usbd_bus_instance *bus, char *arg, char *serial_number)
{
        return usbd_enable_function(bus, arg, serial_number);
}
#endif

/*!
 * usbd_function_disable() - disable a usbd function driver
 * @param function
 */
void usbd_function_disable (struct usbd_function_instance *function)
{
        //int configuration;
        struct usbd_function_driver *function_driver = function->function_driver;
        /*
         * XXX composite or class?
         */
        TRACE_MSG3(USBD, "DISABLE: function_instance: %x function_driver: %x function_disable: %x",
                        function, function_driver,
                        function_driver ? function_driver->fops->function_disable : NULL
                        );
        /*
        if (function_driver && function->function_driver->fops->function_disable)
                function->function_driver->fops->function_disable (function);
        */


        function->bus = NULL;
        function_driver->flags &= ~FUNCTION_ENABLED;
}


/*!
 * usbd_disable_function() - called to disable the current function
 *
 * Used by a USB Bus interface driver to destroy a virtual device.
 *
 * @param bus
 */
void usbd_disable_function (struct usbd_bus_instance *bus)
{
	TRACE_MSG0(USBD, "DISABLE-");

        // prevent any more bottom half scheduling
        while (otg_sem_wait(&usbd_bus_sem));
        bus->status = USBD_CLOSING;
        TRACE_MSG1(USBD, "bus->status: %d", bus->status);
        otg_sem_post(&usbd_bus_sem);


        if (bus->function_instance) {

                TRACE_MSG0(USBD, "disable function");
                usbd_function_disable (bus->function_instance);
                bus_function_disable (bus, bus->function_instance);
                usbp_dealloc_composite_instance((struct usbd_composite_instance *)bus->function_instance);

                if (bus->ep0_urb->buffer) LKFREE(bus->ep0_urb->buffer);
                LKFREE (bus->ep0_urb);
                bus->ep0_urb = NULL;

                while (otg_sem_wait(&usbd_bus_sem));
                bus->function_instance = NULL;
                otg_sem_post(&usbd_bus_sem);
        }
        bus->status = USBD_CLOSED;
        TRACE_MSG1(USBD, "bus->status: %d", bus->status);

        /* XXX wait for reset, resume and suspend bh's */
        // XXX
}

OTG_EXPORT_SYMBOL(usbd_enable_function);
OTG_EXPORT_SYMBOL(usbd_disable_function);

/* ************************************************************************** */
/* ************************************************************************** */
/*!
 * @brief usbd_device_reset() - tell function driver it has been reset
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param data - bus instance pointer
 * @return non-zero if errror
 */
void *usbd_device_reset(void *data)
{
        struct usbd_bus_instance *bus = (struct usbd_bus_instance *) data;
        struct usbd_function_instance *function = bus->function_instance;
        BOOL rc;

        TRACE_MSG0(USBD, "DEVICE_RESET AAAA");

        RETURN_NULL_UNLESS(bus->device_state == STATE_CONFIGURED);
        RETURN_NULL_UNLESS(bus->status == USBD_OK);

        /* Call all function drivers reset() operation and if available any
         * interface drivers reset() operations.
         */
        rc = function->function_driver->fops->reset && function->function_driver->fops->reset (function);

        TRACE_MSG0(USBD, "DEVICE_RESET BBBB");
        TRACE_MSG1(USBD, "type: %d", function->function_type);


        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_class_instance *class_instance = composite_instance->class_instance;
                int i;

                TRACE_MSG1(USBD, "interfaces: %d", composite_instance->interface_functions);
                TRACE_MSG0(USBD, "DEVICE_RESET CCCC");
                rc = class_instance &&
                        class_instance->function.function_driver->fops->reset &&
                        class_instance->function.function_driver->fops->reset ((struct usbd_function_instance *)class_instance);

                TRACE_MSG0(USBD, "DEVICE_RESET DDDD");
                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;
                        rc = interface_instance &&
                                interface_instance->function.function_driver->fops->reset &&
                                interface_instance->function.function_driver->fops->reset
                                        ((struct usbd_function_instance *)interface_instance);
                }
                TRACE_MSG0(USBD, "DEVICE_RESET EEEE");
        }
        TRACE_MSG0(USBD, "DEVICE_RESET FFFF");
        return NULL;
}

/*!
 * @brief device_suspended() - tell function driver it has been suspended
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param data - bus instance pointer
 * @return non-zero if errror
 */
void *usbd_device_suspended(void *data)
{
        struct usbd_bus_instance *bus = (struct usbd_bus_instance *) data;
        struct usbd_function_instance *function = bus->function_instance;
        BOOL rc;

        TRACE_MSG2(USBD, "device_state: %d status: %d",bus->device_state, bus->status);

        RETURN_NULL_UNLESS(bus->device_state == STATE_SUSPENDED);
        RETURN_NULL_UNLESS(bus->status == USBD_OK);

        /* Call all function drivers suspended() operation and if available any
         * interface drivers suspended() operations.
         */
        rc = function->function_driver->fops->suspended && function->function_driver->fops->suspended (function);

        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_class_instance *class_instance = composite_instance->class_instance;
                int i;
                rc = class_instance &&
                        class_instance->function.function_driver->fops->suspended &&
                        class_instance->function.function_driver->fops->suspended
                                ((struct usbd_function_instance *)class_instance);
                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;
                        rc = interface_instance &&
                                interface_instance->function.function_driver->fops->suspended &&
                                interface_instance->function.function_driver->fops->suspended
                                        ((struct usbd_function_instance *)interface_instance);
                }
        }
        return NULL;
}

/*!
 * @brief device_resumed() - tell function driver it has been resumed
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param data - bus instance pointer
 * @return non-zero if errror
 */
void *usbd_device_resumed(void *data)
{
        struct usbd_bus_instance *bus = (struct usbd_bus_instance *) data;
        struct usbd_function_instance *function = bus->function_instance;
        BOOL rc;

        TRACE_MSG1(USBD, "type: %d", function->function_type);

        RETURN_NULL_UNLESS(bus->device_state == STATE_CONFIGURED);
        RETURN_NULL_UNLESS(bus->status == USBD_OK);

        /* Call all function drivers resumed() operation and if available any
         * interface drivers resumed() operations.
         */
        rc = function->function_driver->fops->resumed && function->function_driver->fops->resumed (function);

        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_class_instance *class_instance = composite_instance->class_instance;
                int i;
                rc = class_instance &&
                        class_instance->function.function_driver->fops->resumed &&
                        class_instance->function.function_driver->fops->resumed
                                ((struct usbd_function_instance *)class_instance);
                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;
                        rc = interface_instance &&
                                interface_instance->function.function_driver->fops->resumed &&
                                interface_instance->function.function_driver->fops->resumed
                                        ((struct usbd_function_instance *)interface_instance);
                }
        }
        return NULL;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Device Request Processing:
 *      usbd_device_request()
 *
 */

/*!
 * @name C.f. 9.4 Standard Requests and table 9.3
 *
 * Encode valid requests into a bitmap for each recipient type for
 * both directions. This is needed because there are some requests
 * that appear to be a standard request but are not described in
 * Chapter nine. For example the mouse driver hid request.
 *
 * So we only process the actual set of standard requests that are
 * defined in chapter nine and even if it appears to be standard but
 * is not in chapter nine then we pass it to the function driver.
 *
 */

#define STD(x) (1<<(x+1))

/*! @{ */
/*! h2d_standard_requests and d2h_standard_requests
 *
 * These tables list all of the valid Chapter Nine requests. Any request
 * not listed in these tables will NOT be processed by the EP0 function and
 * will instead be passed to the appropriate function driver.
 */
u32 h2d_standard_requests[4] = {
        // 0 - Device
        STD(USB_REQ_CLEAR_FEATURE) |
        STD(USB_REQ_SET_FEATURE) |
        STD(USB_REQ_SET_ADDRESS) |
        STD(USB_REQ_SET_DESCRIPTOR) |
        STD(USB_REQ_SET_CONFIGURATION) ,
        // 1 - Interface
        STD(USB_REQ_CLEAR_FEATURE) |
        STD(USB_REQ_SET_FEATURE) |
        STD(USB_REQ_SET_INTERFACE) ,
        // 2 - Endpoint
        STD(USB_REQ_CLEAR_FEATURE) |
        STD(USB_REQ_SET_FEATURE) ,
        // 3 - Other
        0,
};

u32 d2h_standard_requests[4] = {
        // 0 - Device
        STD(USB_REQ_GET_STATUS) |
        STD(USB_REQ_GET_DESCRIPTOR) |
        STD(USB_REQ_GET_CONFIGURATION),
        // 1 - Interface
        STD(USB_REQ_GET_STATUS) |
        STD(USB_REQ_GET_INTERFACE) ,
        // 2 - Endpoint
        STD(USB_REQ_GET_STATUS) |
        STD(USB_REQ_SYNCH_FRAME) ,
        // 3 - Other
        0,
};

/* @} */


/*!
 * @brief devreq_set_configuration() - process a received urb
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param function
 * @param wValue
 * @return non-zero if errror
 */
static int devreq_set_configuration(struct usbd_function_instance *function, int wValue)
{
        struct usbd_bus_instance *bus = function->bus;
        BOOL rc;

        // c.f. 9.4.7 - the top half of wValue is reserved
        // c.f. 9.4.7 - zero is the default or addressed state, in our case this
        // is the same is configuration zero, but will be fixed in usbd.c when used.

        u8 ConfigurationValue = wValue & 0x7f;

        //RETURN_EINVAL_IF(ConfigurationValue > bNumConfigurations);
        ///* A ConfigurationValue of zero is for the default configuration (i.e.) the
        // * first one. A non-zero value needs to be decremented to allow it to be used
        // * as an index.
        // */
        //ConfigurationValue = !ConfigurationValue ? 0 : ConfigurationValue -1;


        /* XXX check configuration value is correct, look in pre-built descriptors
         * RETURN_EINVAL_UNLESS(configuration == ...)
         */


        /* propagate event */
        //XXX pcd_bus_event_handler_irq (function->bus, ConfigurationValue ?  DEVICE_CONFIGURED : DEVICE_DE_CONFIGURED, 0);

        if (bus->driver->bops->set_configuration)  bus->driver->bops->set_configuration (bus, ConfigurationValue);

        TRACE_MSG1(USBD, "type: %d", function->function_type);

        /* Call all function drivers set_configuration() operation and if available any
         * interface drivers set_configuration() operations.
         */
        if (function_simple == function->function_type) {

                struct usbd_simple_instance *simple_instance = (struct usbd_simple_instance *)function;

                /* save ConfigurationValue for GET_CONFIGURATION request
                 */
                simple_instance->ConfigurationValue = wValue & 0x7f;//ConfigurationValue;

        }

        rc = function->function_driver->fops->set_configuration &&
                function->function_driver->fops->set_configuration (function, wValue);

        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_class_instance *class_instance = composite_instance->class_instance;
                int i;

                /* save ConfigurationValue for GET_CONFIGURATION request, call
                 * the reset function for the class and all interface functins.
                 */
                composite_instance->ConfigurationValue = wValue & 0x7f;//ConfigurationValue;

                rc = class_instance &&
                        class_instance->function.function_driver->fops->set_configuration &&
                        class_instance->function.function_driver->fops->set_configuration
                                ((struct usbd_function_instance *)class_instance, wValue);

                for (i = 0; i < composite_instance->interface_functions; i++) {
                        struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;
                        rc = interface_instance &&
                                interface_instance->function.function_driver->fops->set_configuration &&
                                interface_instance->function.function_driver->fops->set_configuration
                                        ((struct usbd_function_instance *)interface_instance, wValue);
                }
        }
        return 0;
}

/*!
 *  @brief find_interface_instance()
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param composite_instance
 * @param wIndex
 * @return interface_instance
 */
static struct usbd_interface_instance*
find_interface_instance(struct usbd_composite_instance *composite_instance, int wIndex)
{
        int i;
        TRACE_MSG1(USBD, "wIndex: %d", wIndex);
        for (i = 0; i < composite_instance->interface_functions; i++) {
                struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;
                struct usbd_interface_driver *interface_driver =
                        (struct usbd_interface_driver *) interface_instance->function.function_driver;

                TRACE_MSG3(USBD, "wIndex: %d interface->wIndex: %d, interface->interfaces: %d",
                                wIndex, interface_instance->wIndex, interface_driver->interfaces);

                CONTINUE_UNLESS((wIndex >= interface_instance->wIndex) &&
                                (wIndex < interface_instance->wIndex + interface_driver->interfaces) );

                TRACE_MSG2(USBD, "found interface: %d altSetting: %d",
                                interface_instance->wIndex, interface_instance->altsetting);
                return interface_instance;
        }
        TRACE_MSG0(USBD, "ERROR");
        return NULL;
}

/*!
 * @brief  devreq_set_interface_altsetting() -
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param function
 * @param wIndex
 * @param wValue
 * @return non-zero if errror
 */
static int devreq_set_interface_altsetting(struct usbd_function_instance *function, int wIndex, int wValue)
{
        //struct usbd_bus_instance *bus = function->bus;
        BOOL rc;

        TRACE_MSG2(USBD, "Interface/wIndex: %02x altsetting/wValue: %02x", wIndex, wValue);

        // XXX pcd_bus_event_handler_irq (bus, DEVICE_SET_INTERFACE, 0);

        /* Call function driver set_interface() operation and if available any
         * interface drivers set_interface() operations.
         */
        if (function_simple == function->function_type) {
                struct usbd_simple_instance *simple_instance = (struct usbd_simple_instance *)function;
                RETURN_EINVAL_IF(wIndex > simple_instance->interfaces);
                simple_instance->altsettings[wIndex] = (u8) wValue;
                rc = simple_instance->function.function_driver->fops->set_interface &&
                        simple_instance->function.function_driver->fops->set_interface (function, wIndex, wValue);
        }

        /* Note that composite and class drivers does not receive set_interface information
         */
        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_interface_instance *interface_instance;
                RETURN_EINVAL_UNLESS((interface_instance = find_interface_instance(composite_instance, wIndex)));

                rc = interface_instance->function.function_driver->fops->set_interface &&
                        interface_instance->function.function_driver->fops->set_interface
                        ((struct usbd_function_instance *)interface_instance, wIndex, wValue);
        }
        return 0;
}



/*!
 * @brief do_endpoint_cleared - clear endpoint
 *
 * Set or clear endpoint.
 *
 * Call bus interface halt_endpoint to set or clear.
 *
 * Call interface function endpoint clear to indicate clear.
 *
 * Return non-zero to indicate failure.
 * @param function
 * @param bEndpointAddress
 * @param setclear_flag
 * @return int
 */
static int do_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress, int setclear_flag)
{
        u16 endpoint_index;
        //int endpoints;
        //int i;
        struct usbd_endpoint_instance *endpoint;

        TRACE_MSG2(USBD, "bEndpointAddress: %04x setclear: %x", bEndpointAddress, setclear_flag);

        /* Find and verify the endpoint map index as well.  Then clear the
         * endpoint using the bus interface driver halt_endpoint()
         * operation, if that is ok, then inform the function driver using
         * the endpoint_cleared() operation.
         */
        RETURN_EINVAL_UNLESS((endpoint_index = usbd_find_endpoint_index(function->bus, bEndpointAddress))
                        < function->bus->endpoints);
        TRACE_MSG1(USBD, "endpoint_index: %x", endpoint_index);

        RETURN_EINVAL_UNLESS((endpoint = function->bus->endpoint_array + endpoint_index));

        RETURN_EINVAL_IF(function->bus->driver->bops->halt_endpoint(function->bus, bEndpointAddress, setclear_flag));
        TRACE_MSG0(USBD, "halt_endpoint OK");

        RETURN_ZERO_IF(setclear_flag);

        if (function_simple == function->function_type) {
                struct usbd_simple_instance *simple_instance = (struct usbd_simple_instance *)function;
                TRACE_MSG2(USBD, "SIMPLE bEndpointAddress: %0dx interfaces: %02d", bEndpointAddress, simple_instance->interfaces);
                //RETURN_EINVAL_IF(wIndex > simple_instance->interfaces);
                if (simple_instance->function.function_driver->fops->endpoint_cleared)
                        simple_instance->function.function_driver->fops->endpoint_cleared (function, endpoint_index);
        }

        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                struct usbd_interface_instance *interface_instance;
                TRACE_MSG2(USBD, "COMPOSITE endpoint->interface_wIndex: %0d find: %x",
                                endpoint->interface_wIndex,
                                find_interface_instance(composite_instance, endpoint->interface_wIndex));

                RETURN_EINVAL_UNLESS((interface_instance =
                                        find_interface_instance(composite_instance, endpoint->interface_wIndex)));
                TRACE_MSG1(USBD, "interface_instance: %x", interface_instance);

                if (interface_instance->function.function_driver->fops->endpoint_cleared)
                        interface_instance->function.function_driver->fops->endpoint_cleared
                                ((struct usbd_function_instance *)interface_instance, bEndpointAddress);
        }
        return 0;
}


/*!
 *  @brief do_non_standard_device_request - process a device request
 *
 * Process a received device request. If not a Chapter nine request pass it
 * to the other loaded function driver device_request() function.
 *
 * @param function
 * @param request
   @return  int  Return non-zero to indicate failure.
 */
static int do_non_standard_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_class_instance *class_instance;
        struct usbd_interface_instance *interface_instance;
        u8 bmRequestType = request->bmRequestType;
        //u16 wValue = le16_to_cpu(request->wValue);
        u16 wIndex = le16_to_cpu(request->wIndex);
        //u16 wLength = le16_to_cpu(request->wLength);

        u16 endpoint_index;
        //int endpoints;
        //int i;

        if (function_simple == function->function_type) {
                struct usbd_simple_instance *simple_instance = (struct usbd_simple_instance *)function;
                return simple_instance->function.function_driver->fops->device_request ?
                        simple_instance->function.function_driver->fops->device_request(function, request) : 0;
        }

        if (function_composite == function->function_type) {
                struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;
                int i;

                /* direct request to simple, interface or composite driver as appropriate
                 */
                switch(bmRequestType & USB_REQ_RECIPIENT_MASK) {
                case USB_REQ_RECIPIENT_ENDPOINT:

                        /* Need to find interface function responsible for endpoint and and give
                         * it the device request.
                         */
                        RETURN_EINVAL_UNLESS((endpoint_index = usbd_find_endpoint_index(function->bus, wIndex))
                                        < function->bus->endpoints);

                        wIndex = function->bus->endpoint_array[endpoint_index].interface_wIndex;

                        /* FALL THROUGH */

                case USB_REQ_RECIPIENT_INTERFACE:

                        /* Determine interface function responsible for interface and give it
                         * the device request.
                         */
                        RETURN_EINVAL_UNLESS((interface_instance = find_interface_instance(composite_instance, wIndex)));

                        return interface_instance->function.function_driver->fops->device_request ?
                                (interface_instance->function.function_driver->fops->device_request
                                ((struct usbd_function_instance *)interface_instance, request)) :
                                        -EINVAL;
                        /*
                         * XXX It may be necessary to allow all failed requests above
                         * to fall through to here....
                         */
                case USB_REQ_RECIPIENT_DEVICE:
                case USB_REQ_RECIPIENT_OTHER:
                default:
                        /* Attempt to find someone who will handle this request ...
                         * As long as each interface properly fails requests it does
                         * not understand there is a reasonably good chance that it
                         * will get to the correct handler.
                         */
                        RETURN_ZERO_UNLESS((class_instance = composite_instance->class_instance) &&
                                class_instance->function.function_driver->fops->device_request ?
                                class_instance->function.function_driver->fops->device_request
                                        ((struct usbd_function_instance *)class_instance, request) : -EINVAL);

                        for (i = 0; i < composite_instance->interface_functions; i++) {
                                struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array + i;

                                TRACE_MSG2(USBD, "interface_instance: %x %s",
                                                interface_instance,
                                                interface_instance ? interface_instance->function.name : "");

                                RETURN_ZERO_UNLESS(interface_instance &&
                                                interface_instance->function.function_driver->fops->device_request ?
                                                (interface_instance->function.function_driver->fops->device_request
                                                ((struct usbd_function_instance *)interface_instance, request)) :
                                                -EINVAL);
                        }
                        return -EINVAL;
                }
        }
        return -EINVAL; /* never used */
}

/*!
 *  @brief devreq_get_device_feature_settings() - process a received urb
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param function
 * @return non-zero if errror
 */
int devreq_get_device_feature_settings(struct usbd_function_instance *function)
{
        struct usbd_bus_instance *bus = function->bus;
        return bus->device_feature_settings;
}


/*!
 * @brief devreq_device_feature - handle set/clear feature requests for device
 * Used by the USB Device Core to set/clear endpoint halt status.
 *
 * We assume that if the udc driver does not implement anything then
 * we should just return zero for ok.

 * @param bus
 * @param features
 * @param flag
 * @return int
 */
int devreq_device_feature (struct usbd_bus_instance *bus, int features, int flag)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        //u32 features = bus->device_feature_settings;

        TRACE_MSG0(USBD, "BUS_DEVICE FEATURE");

        /* if USB Host has enabled remote wakeup
         */
        if (features & FEATURE(USB_DEVICE_REMOTE_WAKEUP) )
                otg_queue_event(pcd->otg,
                                flag ?  REMOTE_WAKEUP_ENABLED : REMOTE_WAKEUP_ENABLED_, USBD,
                                flag ?  "DEVICE FEATURE - REMOTE_WAKEUP_ENABLED" :
                                "DEVICE FEATURE - REMOTE_WAKEUP_ENABLED/");

        /* not legal to clear HNP */
        if (flag) {

                /* if A-Device has enabled hnp
                 */
                if (features & FEATURE(USB_OTG_B_HNP_ENABLE) )
                        otg_queue_event(pcd->otg, HNP_ENABLED, USBD, "DEVICE FEATURE - B_HNP_ENABLE");

                /* if A-Device does not support HNP on this port
                 */
                else if ( features & FEATURE(USB_OTG_A_ALT_HNP_ENABLE) )
                        ; //otg_event(pcd->otg, not(a_hnp_support));

                /* if A-Device does support HNP
                 */
                else if (features & FEATURE(USB_OTG_A_HNP_SUPPORT))
                        ; //otg_event(pcd->otg, a_hnp_support);

        }
        return 0;
}

/*!
 * @brief copy_config() - copy data into buffer

 * @param cp
 * @param data
 * @param actual_length
 * @param max_buf
 * @return length
 */
static int copy_config (u8 *cp, void *data, int actual_length, int max_buf)
{
        int available = max_buf - actual_length;
        int length = MIN(*(u8 *)data, available);
        RETURN_ZERO_UNLESS (data);
        RETURN_ZERO_UNLESS (length);
        memcpy (cp, data, length);
        return length;
}

#if 0
/*!
 * @brief  copy_endpoint() - copy data into buffer
 * @param function
 * @param cp
 * @param endpoint
 * @param endpoint_index
 * @param actual_length
 * @param max_buf
 * @param hs
 * @return length
 */
static int copy_endpoint (struct usbd_function_instance *function, u8 *cp,
                struct usbd_endpoint_descriptor *endpoint, int endpoint_index, int actual_length, int max_buf, int hs)
{
        int available = max_buf - actual_length;
        int length = MIN(endpoint->bLength, available);
        struct usbd_endpoint_descriptor endpoint_copy;

        RETURN_ZERO_IF (!length);
        memcpy (&endpoint_copy, endpoint, endpoint->bLength);
        usbd_endpoint_update(function, endpoint_index, &endpoint_copy, hs);
        memcpy (cp, &endpoint_copy, length);
        return length;
}
#endif

/*!
 * @brief  usbd_get_descriptor() - copy a descriptor into buffer
 *
 * @param function
 * @param buffer
 * @param wLength
 * @param descriptor_type
 * @param index
 * @return non-zero for error.
 */
int usbd_get_descriptor (struct usbd_function_instance *function, u8 *buffer, int wLength, int descriptor_type, int index)
{
        struct usbd_bus_instance *bus = function->bus;
        struct usbd_function_driver *function_driver = bus->function_instance->function_driver;
        int actual_length = 0;

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //TRACE_MSG3(USBD, "descriptor_type: %d index: %d wLength: %d", descriptor_type, index, wLength);
        switch (descriptor_type) {
        case USB_DT_DEVICE: {
                struct usbd_device_descriptor device_descriptor;
                struct usbd_device_description *device_description;
                u8 iManufacturer;
                u8 iProduct;
                u8 iSerialNumber = 0;

                //printk(KERN_INFO"%s: DT_DEVICE\n", __FUNCTION__);
                //TRACE_MSG1(USBD, "DT_DEVICE function_type: %d", function->function_type);

                if (function_simple == function->function_type) {
                        struct usbd_simple_driver * simple_driver =
                                (struct usbd_simple_driver *)function_driver;

                        device_description = simple_driver->device_description;
                        iManufacturer = simple_driver->iManufacturer;
                        iProduct = simple_driver->iProduct;
                        iSerialNumber = simple_driver->iSerialNumber;
                }
                else if (function_composite == function->function_type) {

                        struct usbd_composite_driver * composite_driver =
                                (struct usbd_composite_driver *)function_driver;

                        device_description = composite_driver->device_description;

                        //TRACE_MSG6(USBD, "iManufacturer: %d iProduct: %d iSerialNumber: %d "
                        //                "idVendor: %04x idProduct: %04x bcdDevice: %04x",
                        //                composite_driver->iManufacturer, composite_driver->iProduct,
                        //                composite_driver->iSerialNumber, device_description->idVendor,
                        //                device_description->idProduct, device_description->bcdDevice
                        //                );

                        iManufacturer = composite_driver->iManufacturer;
                        iProduct = composite_driver->iProduct;
                        iSerialNumber = composite_driver->iSerialNumber;
                }
                else
                        return 0;

                //TRACE_MSG3(USBD, "function_driver: %x device_descriptor: %x %d",
                //                function_driver, device_descriptor, device_descriptor->bLength);

                RETURN_EINVAL_UNLESS(device_description);

                // correct the correct control endpoint 0 wLength packet size into the descriptor
                //device_descriptor = (struct usbd_device_descriptor *) buffer;

                device_descriptor.bLength = sizeof(struct usbd_device_descriptor);;
                device_descriptor.bDescriptorType = USB_DT_DEVICE;
                device_descriptor.bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION);
                device_descriptor.bDeviceClass = device_description->bDeviceClass;
                device_descriptor.bDeviceSubClass = device_description->bDeviceSubClass;
                device_descriptor.bDeviceProtocol = device_description->bDeviceProtocol;
                device_descriptor.bMaxPacketSize0 = device_description->bMaxPacketSize0;

                device_descriptor.idVendor = cpu_to_le16(device_description->idVendor);
                device_descriptor.idProduct = cpu_to_le16(device_description->idProduct);
                device_descriptor.bcdDevice = cpu_to_le16(device_description->bcdDevice);
                device_descriptor.iManufacturer = iManufacturer;
                device_descriptor.iProduct = iProduct;
                device_descriptor.iSerialNumber = iSerialNumber;
                device_descriptor.bNumConfigurations = 1;

                //actual_length += wLength;
                //memcpy(buffer, device_descriptor, wLength);

                /* copy device descriptor for this device */
                actual_length += copy_config (buffer + actual_length, &device_descriptor, actual_length, wLength);
                break;
        }

        #ifdef CONFIG_OTG_HIGH_SPEED
        case USB_DT_DEVICE_QUALIFIER:      {
                // c.f. 9.6.2 Device Qualifier
                struct usbd_device_qualifier_descriptor device_qualifier_descriptor;

                struct usbd_device_description *device_description;

                //printk(KERN_INFO"%s: DT_DEVICE_QUALIFIER\n", __FUNCTION__);

                if (function_simple == function->function_type) {
                        struct usbd_simple_driver * simple_driver =
                                (struct usbd_simple_driver *)function_driver;

                        device_description = simple_driver->device_description;
                }
                else if (function_composite == function->function_type) {

                        struct usbd_composite_driver * composite_driver =
                                (struct usbd_composite_driver *)function_driver;

                        device_description = composite_driver->device_description;
                }
                else
                        return 0;

                device_qualifier_descriptor.bLength = sizeof(struct usbd_device_qualifier_descriptor);;
                device_qualifier_descriptor.bDescriptorType = USB_DT_DEVICE_QUALIFIER;
                device_qualifier_descriptor.bcdUSB = __constant_cpu_to_le16(USB_BCD_VERSION);
                device_qualifier_descriptor.bDeviceClass = device_description->bDeviceClass;
                device_qualifier_descriptor.bDeviceSubClass = device_description->bDeviceSubClass;
                device_qualifier_descriptor.bDeviceProtocol = device_description->bDeviceProtocol;
                device_qualifier_descriptor.bMaxPacketSize0 = device_description->bMaxPacketSize0;
                device_qualifier_descriptor.bNumConfigurations = 1; // XXX
                device_qualifier_descriptor.bReserved = 0;

                // copy descriptor for this device
                actual_length += copy_config (buffer + actual_length, &device_qualifier_descriptor, actual_length, wLength);

                break;
        }

        case USB_DT_OTHER_SPEED_CONFIGURATION:
        #endif /* CONFIG_OTG_HIGH_SPEED */

        case USB_DT_CONFIGURATION: {
                //printk(KERN_INFO"%s: DT_CONFIGURATION\n", __FUNCTION__);
                #ifdef CONFIG_OTG_HIGH_SPEED
                int hs = bus->high_speed ?  descriptor_type == USB_DT_CONFIGURATION:
                        descriptor_type == USB_DT_OTHER_SPEED_CONFIGURATION;
                #else /* CONFIG_OTG_HIGH_SPEED */
                int hs = 0;
                #endif /* CONFIG_OTG_HIGH_SPEED */
                struct usbd_configuration_descriptor *configuration_descriptor;

                if (function_simple == function->function_type)
                        configuration_descriptor = ((struct usbd_simple_instance *)function)->configuration_descriptor[hs];
                else if (function_composite == function->function_type)
                        configuration_descriptor = ((struct usbd_composite_instance *)function)->configuration_descriptor[hs];
                else
                        return 0;

                actual_length  = MIN(le16_to_cpu(configuration_descriptor->wTotalLength), wLength);
                memcpy (buffer, configuration_descriptor, actual_length);

                /* set descriptor type to requested type */
                configuration_descriptor = (struct usbd_configuration_descriptor *)buffer;
                configuration_descriptor->bDescriptorType = descriptor_type;

                #if 0
                {
                        u8 * cp = buffer;

                        int i;
                        int size = configuration_descriptor->wTotalLength;

                        TRACE_MSG2(USBD, "cfg: %x size: %d", cp, size);
                        for (i = 0; i < size; i+= 8) {
                                TRACE_MSG8(USBD, "%02x %02x %02x %02x %02x %02x %02x %02x",
                                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                          );
                        }

                }
                #endif
                break;
        }

        case USB_DT_STRING: {
                struct usbd_string_descriptor *string_descriptor = NULL;
                //printk(KERN_INFO"%s: DT_STRING\n", __FUNCTION__);
                RETURN_EINVAL_IF (!(string_descriptor = usbd_get_string_descriptor (function, (u8)index)));
                actual_length += copy_config (buffer + actual_length, string_descriptor, actual_length, wLength);
                break;
        }

        case USB_DT_OTG: {
                struct usbd_otg_descriptor *otg_descriptor = (struct usbd_otg_descriptor *) buffer;
                actual_length += sizeof(struct usbd_otg_descriptor);
                otg_descriptor->bLength = sizeof(struct usbd_otg_descriptor);
                otg_descriptor->bDescriptorType = USB_DT_OTG;
                otg_descriptor->bLength = 0;    // XXX
                break;
        }
        default:
                return -EINVAL;
        }
        return actual_length;
}




/*!
 * @brief  setclear() - set or reset a bit in a mask
 * @param features
 * @param flag
 * @param value
 */
static void setclear(u32 *features, u32 flag, u32 value)
{
        if (value) *features |= flag;
        else *features &= ~flag;
}

/*!
 * @brief  do_standard_device_request - process a device request
 *
 * Process a received device request. If not a Chapter nine request pass it
 * to the other loaded function driver device_request() function.
 *
 * @param function
 * @param request
 * @return non-zero to indicate failure.
 */
static int do_standard_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_simple_instance *simple_instance = (struct usbd_simple_instance *)function;
        struct usbd_composite_instance *composite_instance = (struct usbd_composite_instance *)function;

        struct usbd_bus_instance *bus = function->bus;
        u8 bRequest = request->bRequest;
        u8 bmRequestType = request->bmRequestType;
        u16 wValue = le16_to_cpu(request->wValue);
        u16 wIndex = le16_to_cpu(request->wIndex);
        u16 wLength = le16_to_cpu(request->wLength);
        //u16 endpoint_index;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        usbd_device_state_t device_state = usbd_get_device_state(function);


        switch (device_state) {
        case STATE_CREATED:
        case STATE_ATTACHED:
        case STATE_POWERED:
                printk(KERN_INFO"%s: bad device state: %d\n", __FUNCTION__, device_state);
                TRACE_MSG1(USBD, "bad device_state: %d", device_state);
                return -EINVAL;

        case STATE_INIT:
        case STATE_DEFAULT:
                switch (bRequest) {
                case USB_REQ_CLEAR_FEATURE:
                case USB_REQ_GET_INTERFACE:
                case USB_REQ_GET_STATUS:
                case USB_REQ_SET_DESCRIPTOR:
                case USB_REQ_SET_INTERFACE:
                case USB_REQ_SYNCH_FRAME:
                        printk(KERN_INFO"%s: bad request %d for this device state: %d\n", __FUNCTION__,
                                        bRequest, device_state);
                        TRACE_MSG2(USBD, "bad request: %d for this device_state: %d", bRequest, device_state);
                        return -EINVAL;

                case USB_REQ_GET_CONFIGURATION:
                case USB_REQ_SET_ADDRESS:
                        otg_queue_event(pcd->otg, DEVICE_REQUEST, USBD, "DEVICE REQUEST");
                case USB_REQ_SET_CONFIGURATION:
                case USB_REQ_GET_DESCRIPTOR:
                case USB_REQ_SET_FEATURE:
                        break;
                }
        case STATE_ADDRESSED:
        case STATE_CONFIGURED:
        case STATE_SUSPENDED:
                break;
        case STATE_UNKNOWN:
                TRACE_MSG1(USBD, "unknown device_state: %d", device_state);
                return -EINVAL;
        }

        /* Handle all requests that return data (direction bit set on bm RequestType).
         * N.B. no Chapter 9 requests send additional data.
         */
        if ((bmRequestType & USB_REQ_DIRECTION_MASK)) {
                struct usbd_urb *urb;
                int rc = 1;
                switch (bRequest) {
                case USB_REQ_CLEAR_FEATURE:
                case USB_REQ_SET_ADDRESS:
                case USB_REQ_SET_CONFIGURATION:
                case USB_REQ_SET_DESCRIPTOR:
                case USB_REQ_SET_FEATURE:
                case USB_REQ_SET_INTERFACE:
                case USB_REQ_SYNCH_FRAME:
                        printk(KERN_INFO"%s: bad direction\n", __FUNCTION__);
                        TRACE_MSG0(USBD, "bad direction");
                        return -EINVAL;
                }

                RETURN_EINVAL_IF(!wLength);

                /* allocate urb, no notify callback, urb will be automatically de-allocated
                 */
                RETURN_EINVAL_UNLESS((urb = bus->ep0_urb));
                urb->status = USBD_URB_OK;
		urb->irq_flags = 0;

                switch (bRequest) {

                case USB_REQ_GET_STATUS:
                        urb->actual_length = 2;
                        urb->buffer[0] = urb->buffer[1] = 0;

                        switch (bmRequestType & USB_REQ_RECIPIENT_MASK) {
                        case USB_REQ_RECIPIENT_DEVICE:

                                UNLESS (usbd_get_bMaxPower(function)) urb->buffer[0] |= USB_STATUS_SELFPOWERED;
                                urb->buffer[0] |=
                                        (devreq_get_device_feature_settings(function) & FEATURE(USB_DEVICE_REMOTE_WAKEUP)) ?
                                        USB_STATUS_REMOTEWAKEUP : 0;
                                TRACE_MSG2(USBD, "GET STATUS DEVICE status: %02x %02x",
                                                urb->buffer[0], urb->buffer[1]);
                                rc = 0;
                                break;
                        case USB_REQ_RECIPIENT_ENDPOINT:
                                urb->buffer[0] = function->bus->driver->bops->endpoint_halted (bus,wIndex);
                                rc = 0;
                                TRACE_MSG2(USBD, "GET STATUS ENDPOINT status: %02x %02x", urb->buffer[0], urb->buffer[1]);
                                break;
                        case USB_REQ_RECIPIENT_INTERFACE:
                                TRACE_MSG2(USBD, "GET STATUS INTERFACE status: %02x %02x", urb->buffer[0], urb->buffer[1]);
                                rc = 0;
                                break;
                        case USB_REQ_RECIPIENT_OTHER:
                                TRACE_MSG0(USBD, "GET STATUS OTHER");
                        default:
                                TRACE_MSG0(USBD, "GET STATUS BAD RECIPIENT");
                                rc = -EINVAL;
                        }
                        break;

                case USB_REQ_GET_DESCRIPTOR:
                        //printk(KERN_INFO"%s: GET DESCRIPTOR\n", __FUNCTION__);

                        TRACE_MSG2(USBD, "GET_DESCRIPTOR type : %04x, wLength: %d", wValue, wLength);

                        rc = usbd_get_descriptor (function, urb->buffer, wLength, wValue >> 8, wValue & 0xff);


                        //TRACE_MSG1(USBD, "get descriptor rc : %d", rc);
                        if (rc != -EINVAL) {
                                urb->actual_length = rc;
                                //printk(KERN_INFO"%s: length: %d\n", __FUNCTION__, rc);
                                rc = 0;
                        }
                        else {
                                //printk(KERN_INFO"%s: bad description\n", __FUNCTION__);
                                TRACE_MSG0(USBD, "bad description");
                        }
                        break;

                case USB_REQ_GET_CONFIGURATION:
                        urb->actual_length = 1;
                        if (function_simple == function->function_type)
                                urb->buffer[0] = simple_instance->ConfigurationValue;

                        else if (function_composite == function->function_type)
                                urb->buffer[0] = composite_instance->ConfigurationValue;

                        TRACE_MSG1(USBD, "GET CONFIGURATION: %d", urb->buffer[0]);
                        rc=0;   // XXX Always success???
                        break;

                case USB_REQ_GET_INTERFACE:

                        if (function_simple == function->function_type) {
                                rc = 1;
                                BREAK_IF(wIndex > simple_instance->interfaces);
                                urb->buffer[0] = simple_instance->altsettings[wIndex];
                                urb->actual_length = 1;
                                rc = 0;
                                TRACE_MSG1(USBD, "GET INTERFACE: %d simple", urb->buffer[0]);
                        }
                        else if (function_composite == function->function_type) {
                                struct usbd_interface_instance *interface_instance;
                                if ((interface_instance = find_interface_instance(composite_instance, wIndex))) {
                                        urb->buffer[0] = interface_instance->altsetting;
                                        urb->actual_length = 1;
                                        rc = 0;
                                        TRACE_MSG1(USBD, "GET INTERFACE: %d composite", urb->buffer[0]);
                                }
                                else {
                                        TRACE_MSG1(USBD, "GET INTERFACE: %d composite not found", urb->buffer[0]);

                                }
                        }
                        break;
                default:
                        //printk(KERN_INFO"%s: bad descriptor\n", __FUNCTION__);
                        TRACE_MSG0(USBD, "bad descriptor type");
                        rc = 1;
                }

                if (!(urb->actual_length % usbd_endpoint_zero_wMaxPacketSize(function, usbd_high_speed(function))) &&
                                (urb->actual_length < wLength))
                        urb->flags |= USBD_URB_SENDZLP;

                RETURN_ZERO_UNLESS(rc || usbd_start_in_urb(urb));
                /* only get here if error */
                usbd_free_urb(urb);
                //TRACE_MSG0(USBD, "get failed");
                TRACE_MSG1(USBD, "get failed rc:=%x",rc);
                return -EINVAL;
        }
        /* Handle the requests that do not return data.
         */
        else {
                int setclear_flag = USB_REQ_SET_FEATURE == bRequest;
                switch (bRequest) {
                case USB_REQ_CLEAR_FEATURE:
                case USB_REQ_SET_FEATURE:
                        TRACE_MSG4(USBD, "FEATURE: recipient: %d wIndex: %04x wValue: %04x setclear: %02x",
                                        bmRequestType & USB_REQ_RECIPIENT_MASK, wIndex, wValue, setclear_flag);
                        switch (bmRequestType & USB_REQ_RECIPIENT_MASK) {
                        case USB_REQ_RECIPIENT_DEVICE:
                                //switch (wValue) {
                                //        otg_event(pcd->otg, HNP_ENABLED, PCD, "DEVICE FEATURE - B_HNP_ENABLE");
                                //        break;
                                //}

                                switch (wValue) {
                                case USB_OTG_A_ALT_HNP_ENABLE:                  // C.f. OTG 6.5.3
                                case USB_OTG_A_HNP_SUPPORT:                     // C.f. OTG 6.5.2
                                case USB_OTG_B_HNP_ENABLE:                      // C.f. OTG 6.5.1
                                        TRACE_MSG0(USBD, "HNP");
                                        RETURN_EINVAL_UNLESS(setclear_flag);    // cleared by reset
                                        break;
                                case USB_DEVICE_REMOTE_WAKEUP:
                                        TRACE_MSG0(USBD, "REMOTE WAKEUP");
                                        break;
                                case USB_TEST_MODE:
                                        TRACE_MSG2(USBD, "TEST MODE selector: %04x %02x", wIndex, wIndex >> 8);
                                        switch (wIndex >> 8) {
                                        case USB_TEST_J:
                                        case USB_TEST_K:
                                        case USB_TEST_SE0_NAK:
                                        case USB_TEST_PACKET: // XXX it should be possible to do this generically here
                                        case USB_TEST_FORCE_ENABLE:
                                                return function->bus->driver->bops->device_feature(
                                                                        function->bus, wIndex >> 8, setclear_flag);
                                                break;
                                        default:
                                                break;
                                        }
                                        break;
                                default:
                                        return -EINVAL;
                                }
                                setclear(&bus->device_feature_settings, FEATURE(wValue), setclear_flag);
                                return devreq_device_feature(bus, FEATURE(wValue), setclear_flag);

                        case USB_REQ_RECIPIENT_ENDPOINT:
                                RETURN_EINVAL_UNLESS(USB_ENDPOINT_HALT == wValue);

                                setclear(&(bus->endpoint_array[wIndex].feature_setting),
                                                FEATURE(wValue), bRequest == USB_REQ_SET_FEATURE);

                                /* wIndex contains bEndpointAddress, also find and verify the endpoint map index as well.
                                 * Then clear the endpoint using the bus interface driver halt_endpoint() operation,
                                 * if that is ok, then inform the function driver using the endpoint_cleared() operation.
                                 */
                                return do_endpoint_cleared(function, wIndex, setclear_flag);


                        case USB_REQ_RECIPIENT_INTERFACE:
                        case USB_REQ_RECIPIENT_OTHER:
                        default:
                                TRACE_MSG0(USBD, "bad recipient");
                                return -EINVAL;
                        }

                case USB_REQ_SET_ADDRESS:
                        if (bus->driver->bops->set_address)  bus->driver->bops->set_address (bus, wValue);
                        // XXX pcd_bus_event_handler_irq (bus, DEVICE_ADDRESS_ASSIGNED, wValue);
                        return 0;

                case USB_REQ_SET_DESCRIPTOR:
                        TRACE_MSG0(USBD, "set descriptor not supported");
                        return -EINVAL;

                case USB_REQ_SET_CONFIGURATION:
                        return devreq_set_configuration(function, wValue);

                case USB_REQ_SET_INTERFACE:
                        return devreq_set_interface_altsetting(function, wIndex, wValue);

                case USB_REQ_GET_CONFIGURATION:
                case USB_REQ_GET_DESCRIPTOR:
                case USB_REQ_GET_INTERFACE:
                case USB_REQ_GET_STATUS:
                case USB_REQ_SYNCH_FRAME:
                        TRACE_MSG0(USBD, "unkown");
                        return -EINVAL;
                }
        }
        TRACE_MSG0(USBD, "not possible");
        return -EINVAL;
}


/*!
 * @brief  usbd_device_request_irq() - process a device request
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param bus
 * @param request
 * @return non-zero if errror
 */
int usbd_device_request_irq(struct usbd_bus_instance *bus, struct usbd_device_request *request)
{
        struct usbd_function_instance *function = bus->function_instance;
        u8 bRequest = request->bRequest;
        u8 bmRequestType = request->bmRequestType;

        /* Handle only USB Standard Requests (c.f. USB Spec table 9-2, D6..5 must be 0),
         * otherwise call the currently enabled function specific receive setup.
         */
	TRACE_SETUP(USBD, request);;
        if (!(bmRequestType & USB_REQ_TYPE_MASK) &&
                        ((bmRequestType & USB_DIR_IN ?  d2h_standard_requests : h2d_standard_requests)
                         [bmRequestType & 0x3] & STD(bRequest)))
                return do_standard_device_request(function, request);
        else
                return do_non_standard_device_request(function, request);

}
OTG_EXPORT_SYMBOL(usbd_device_request_irq);
OTG_EXPORT_SYMBOL(usbd_get_descriptor);

/* ************************************************************************** */
/*!
 * Device I/O:
 *      usbd_urb_finished()
 *      usbd_first_urb_detached()
 *      usbd_find_endpoint_address()
 */

/*!
 * @brief  usbd_do_urb_callback() - tell function that an urb has been transmitted.
 *
 * Must be called from an interrupt or with interrupts disabled.
 *
 * Used by a USB Bus driver to pass a sent urb back to the function
 * driver via the endpoints done queue.
 *
 * If there is no callback or if the URB call back returns non-zero then the
 * urb must be de-allocated here.
 *
 * @param urb
 * @param rc
 */
void usbd_do_urb_callback (struct usbd_urb *urb, int rc)
{
        TRACE_MSG1(USBD,"urb: %p", urb);
        RETURN_UNLESS (urb);
        if (!urb->notify || urb->notify(urb,rc))
                usbd_free_urb(urb);
}

/*!
 * @brief  usbd_find_endpoint_index()

 * Find the endpoint map index for the specified bEndpointAddress.

 * @param bus
 * @param bEndpointAddress
 */
int usbd_find_endpoint_index(struct usbd_bus_instance *bus, int bEndpointAddress)
{
        int i,rc1;
        rc1=0;
        for (i = 0; i < bus->endpoints; i++){
                rc1=i;
                BREAK_IF (bus->endpoint_array[i].bEndpointAddress[bus->high_speed] == bEndpointAddress);
        }
        return rc1;
}

OTG_EXPORT_SYMBOL(usbd_find_endpoint_index);
OTG_EXPORT_SYMBOL(usbd_do_urb_callback);


/* ************************************************************************** */
/* Device init ************************************************************** */

extern char * usbd_function_name(int n);
struct usbd_ops usbd_ops;
otg_tag_t USBD;

/*!
 * @brief usbd_device_init() - initialize
 */
int usbd_device_init (void)
{
        //USBD = otg_trace_obtain_tag(NULL, "usbd");
        TRACE_MSG0(USBD,"--");
        //otg_sem_init_unlocked("usbd_device", &usbd_bus_sem);
        usbd_ops.function_name = usbd_function_name;
        otg_set_usbd_ops(&usbd_ops);
        return 0;
}

/*!
 * @brief usbd_device_exit() - de-initialize
 */
void usbd_device_exit (void)
{
        otg_set_usbd_ops(NULL);
        //otg_trace_invalidate_tag(USBD);
}

/* Module init ************************************************************** */
/*!
 * @brief usbd_device_init() - initialize
 */
int usbd_device_modinit (void)
{
        USBD = otg_trace_obtain_tag(NULL, "usbd");
        otg_sem_init_unlocked("usbd_bus", &usbd_bus_sem);
        return 0;
}

/*!
 * @brief usbd_device_exit() - de-initialize
 */
void usbd_device_modexit (void)
{
        otg_trace_invalidate_tag(USBD);
}
