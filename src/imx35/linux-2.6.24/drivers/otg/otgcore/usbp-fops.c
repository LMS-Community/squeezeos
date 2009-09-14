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
 * otg/otgcore/usbp-fops.c - USB Function support
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/usbp-fops.c|20070816060612|51897
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
 * @file otg/otgcore/usbp-fops.c
 * @brief Function driver related functions.
 *
 * This implements the functions used to implement USB Function drivers.
 *
 * @ingroup USBDCORE
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#define TRACE_VERBOSE   1

void urb_append(urb_link *hd, struct usbd_urb *urb);
struct usbd_function_driver *
usbd_find_function_internal(struct otg_list_node *usbd_function_drivers, const char *name);

//extern struct semaphore usbd_bus_sem;
extern otg_sem_t usbd_bus_sem;

int usbd_strings_init(struct usbd_bus_instance *bus, struct usbd_function_instance *function_instance);
void usbd_strings_exit(struct usbd_bus_instance *bus);
void usbd_free_string_descriptor (struct usbd_bus_instance *bus, u8 index);
extern void otg_write_info_message(void *, char *);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Simple Function Registration:
 *      usbd_register_simple_function()
 *      usbd_deregister_simple_function()
 *
 */

LIST_NODE_INIT(usbd_simple_drivers);            // list of all registered configuration function modules

/*!
 * @brief usbd_register_simple_function() - register a usbd function driver
 *
 * USBD Simple Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param simple_driver - driver to register
 * @param name  - driver name
 * @param privdata - driver private data
 * @return 0 on success, otherwise NULL
 *
 */
int
usbd_register_simple_function (struct usbd_simple_driver *simple_driver, const char * name, void *privdata)
{
        //TRACE_MSG1(USBD, "SIMPLE function: %s", name);
        RETURN_EINVAL_IF(otg_sem_wait(&usbd_bus_sem));
        simple_driver->driver.name = name;
        simple_driver->driver.privdata = privdata;
        simple_driver->driver.function_type = function_simple;
        LIST_ADD_TAIL (&simple_driver->driver.drivers, &usbd_simple_drivers);
        simple_driver->driver.flags |= FUNCTION_REGISTERED;
        otg_sem_post(&usbd_bus_sem);
        return 0;
}

/*!
 * @brief usbd_deregister_simple_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param simple_driver - driver to de-register
 */
void usbd_deregister_simple_function (struct usbd_simple_driver *simple_driver)
{
        RETURN_UNLESS (simple_driver);
        TRACE_MSG1(USBD, "SIMPLE function: %s", simple_driver->driver.name);
        while (otg_sem_wait(&usbd_bus_sem));
        if (simple_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (simple_driver->driver.flags & FUNCTION_REGISTERED) {
                simple_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (simple_driver->driver.drivers);
        }
        otg_sem_post(&usbd_bus_sem);
}

/*!
 * @brief usbp_find_simple_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name - by which to look for driver
 * @return function instance
 *
 */
struct usbd_simple_driver *
usbp_find_simple_driver (char *name)
{
        return (struct usbd_simple_driver *) usbd_find_function_internal(&usbd_simple_drivers, name);
}

OTG_EXPORT_SYMBOL(usbd_register_simple_function);
OTG_EXPORT_SYMBOL(usbd_deregister_simple_function);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Function Registration:
 *      usbd_register_class_function_function()
 *      usbd_register_interface_function()
 *      usbd_register_composite_function()
 *      usbd_deregister_class_function()
 *      usbd_deregister_interface_function()
 *      usbd_deregister_composite_function()
 */

LIST_NODE_INIT(usbd_interface_drivers);         // list of all registered interface function modules
LIST_NODE_INIT(usbd_class_drivers);             // list of all registered composite function modules
LIST_NODE_INIT(usbd_composite_drivers);             // list of all registered composite function modules

/*!
 * @brief usbd_register_interface_function() - register a usbd function driver
 *
 * USBD Interface Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param interface_driver - driver to register
 * @param name - driver name
 * @param privdata - driver's private data
 * @return 0 on finish, or NULL
 *
 */
int
usbd_register_interface_function(struct usbd_interface_driver *interface_driver, const char * name, void *privdata)
{
        //TRACE_MSG1(USBD, "INTERFACE function: %s", name);
        RETURN_EINVAL_IF(otg_sem_wait(&usbd_bus_sem));
        interface_driver->driver.name = name;
        interface_driver->driver.privdata = privdata;
        interface_driver->driver.function_type = function_interface;
        LIST_ADD_TAIL (&interface_driver->driver.drivers, &usbd_interface_drivers);
        interface_driver->driver.flags |= FUNCTION_REGISTERED;
        otg_sem_post(&usbd_bus_sem);
        return 0;
}

/*!
 * @brief usbd_register_class_function() - register a usbd function driver
 *
 * USBD composite Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param class_driver- class driver to register
 * @param name - class driver name
 * @param privdata -class driver private data
 * @return 0 on finish, NULL on failure
 *
 */
int
usbd_register_class_function(struct usbd_class_driver *class_driver, const char * name, void *privdata)
{
        //TRACE_MSG1(USBD, "CLASS function: %s", name);
        RETURN_EINVAL_IF(otg_sem_wait(&usbd_bus_sem));
        class_driver->driver.name = name;
        class_driver->driver.privdata = privdata;
        class_driver->driver.function_type = function_class;
        LIST_ADD_TAIL (&class_driver->driver.drivers, &usbd_class_drivers);
        class_driver->driver.flags |= FUNCTION_REGISTERED;
        otg_sem_post(&usbd_bus_sem);
        return 0;
}

/*!
 * usbd_register_composite_function() - register a usbd function driver
 *
 * USBD composite Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param composite_driver - function driver to register
 * @param name - function driver's register name
 * @param class_name - function driver's class name
 * @param interface_function_names
 * @param privdata - function driver's private data
 * @return 0 on success, or NULL
 *
 */
int
usbd_register_composite_function (struct usbd_composite_driver *composite_driver, const char* name,
                const char* class_name, const char **interface_function_names, void *privdata)
{
        TRACE_MSG1(USBD, "COMPOSITE function: %s", name);
        RETURN_EINVAL_IF(otg_sem_wait(&usbd_bus_sem));
        composite_driver->driver.name = name;
        composite_driver->driver.privdata = privdata;
        composite_driver->driver.function_type = function_composite;
        composite_driver->class_name = class_name;
        composite_driver->interface_function_names = interface_function_names;
        LIST_ADD_TAIL (&composite_driver->driver.drivers, &usbd_composite_drivers);
        composite_driver->driver.flags |= FUNCTION_REGISTERED;
        otg_sem_post(&usbd_bus_sem);
        return 0;
}

/*!
 * usbd_deregister_interface_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param interface_driver - driver to de-register
 */
void usbd_deregister_interface_function (struct usbd_interface_driver *interface_driver)
{
        RETURN_UNLESS (interface_driver);
        //TRACE_MSG1(USBD, "INTERFACE function: %s", interface_driver->driver.name);
        while (otg_sem_wait(&usbd_bus_sem));
        if (interface_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (interface_driver->driver.flags & FUNCTION_REGISTERED) {
                interface_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (interface_driver->driver.drivers);
        }
        otg_sem_post(&usbd_bus_sem);
}

/*!
 * @brief usbd_deregister_class_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param class_driver - pointer to class driver to de-register
 */
void usbd_deregister_class_function (struct usbd_class_driver *class_driver)
{
        RETURN_UNLESS (class_driver);
        //TRACE_MSG1(USBD, "CLASS function: %s", class_driver->driver.name);
        while (otg_sem_wait(&usbd_bus_sem));
        if (class_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (class_driver->driver.flags & FUNCTION_REGISTERED) {
                class_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (class_driver->driver.drivers);
        }
        otg_sem_post(&usbd_bus_sem);
}

/*!
 * @brief usbd_deregister_composite_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param composite_driver
 */
void usbd_deregister_composite_function (struct usbd_composite_driver *composite_driver)
{
        RETURN_UNLESS (composite_driver);
        //TRACE_MSG1(USBD, "COMPOSITE function: %s", composite_driver->driver.name);
        while (otg_sem_wait(&usbd_bus_sem));
        if (composite_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (composite_driver->driver.flags & FUNCTION_REGISTERED) {
                composite_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (composite_driver->driver.drivers);
        }
        otg_sem_post(&usbd_bus_sem);
}

/*!
 * @brief usbp_find_interface_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name - by which to search
 * @return found interface driver
 *
 */
struct usbd_interface_driver *usbp_find_interface_driver(const char *name)
{
	#if defined(CONFIG_OTG_TRACE)
	TRACE_STRING(USBD, "name: %s", (char *)name);
	#endif

        return (struct usbd_interface_driver *)  usbd_find_function_internal(&usbd_interface_drivers, name);
}


/*!
 * @brief usbp_find_class_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name - driver's name by which to search
 * @return function instance
 *
 */
struct usbd_class_driver *usbp_find_class_driver(const char *name)
{
        TRACE_MSG1(USBD, "%s", name);
        return (struct usbd_class_driver *)  usbd_find_function_internal(&usbd_class_drivers, name);
}

/*!
 * @brief usbp_find_composite_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * This will typically only be called within usbd_bus_sem protected area.
 *
 * @param name - by which to search
 * @return  usbd_composite_driver instance
 *
 */
struct usbd_composite_driver *usbp_find_composite_driver(char *name)
{
        TRACE_MSG1(USBD, "%s", name);
        return (struct usbd_composite_driver *) usbd_find_function_internal(&usbd_composite_drivers, name);
}


OTG_EXPORT_SYMBOL(usbd_register_interface_function);
OTG_EXPORT_SYMBOL(usbd_register_composite_function);
OTG_EXPORT_SYMBOL(usbd_register_class_function);
OTG_EXPORT_SYMBOL(usbd_deregister_interface_function);
OTG_EXPORT_SYMBOL(usbd_deregister_class_function);
OTG_EXPORT_SYMBOL(usbd_deregister_composite_function);


/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * @brief copy_descriptor() - copy data into buffer
 *
 * @param cp - buffer to receive
 * @param data - source to copy
 * @return bLength - copied data length
 */
static int copy_descriptor (u8 *cp, void *data)
{
        RETURN_ZERO_UNLESS (data);
        memcpy (cp, data, ((struct usbd_generic_descriptor *)data)->bLength);
        return ((struct usbd_generic_descriptor *)data)->bLength;
}


/*!
 * @brief usbp_alloc_simple_configuration_descriptor() -
 *
 * @param simple_instance
 * @param cfg_size
 * @param hs
 * @return pointer to configuration descriptor
 */
void usbp_alloc_simple_configuration_descriptor(struct usbd_simple_instance *simple_instance, int cfg_size, int hs)
{
        struct usbd_simple_driver *simple_driver = (struct usbd_simple_driver *)
                simple_instance->function.function_driver;
        struct usbd_bus_instance *bus = simple_instance->function.bus;


        struct usbd_configuration_description *configuration_description;
        struct usbd_configuration_descriptor *configuration_descriptor = NULL;


        //int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);
        u8 *cp = NULL;

        //int i, j, k, l;
        int i, k, l;

        //RETURN_NULL_UNLESS((configuration_descriptor = CKMALLOC(cfg_size)));

        configuration_description = simple_driver->configuration_description;

        configuration_descriptor = simple_instance->configuration_descriptor[hs];

        cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);

        /* Compose the configuration descriptor
         * First copy the function driver configuration configuration descriptor.
         */
        cp = (u8 *)configuration_descriptor;
        //cfg_size = copy_descriptor(cp + 0, configuration_description->configuration_descriptor);
        cfg_size = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bLength = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bDescriptorType = USB_DT_CONFIGURATION;

        /* copy peripheral controller / platform dependant values into configuration */
        configuration_descriptor->bmAttributes = USB_BMATTRIBUTE_RESERVED |
                (bus->bmAttributes & (USB_BMATTRIBUTE_SELF_POWERED | USB_BMATTRIBUTE_REMOTE_WAKEUP));

        /* Self Powered devices must set bMaxPower to 1 unit */
        configuration_descriptor->bMaxPower =
                ((bus->bmAttributes & USB_BMATTRIBUTE_SELF_POWERED) || !bus->bMaxPower) ? 1: bus->bMaxPower;

        configuration_descriptor->iConfiguration = usbd_alloc_string (&simple_instance->function,
                        configuration_description->iConfiguration);


        for (i = 0; i < simple_driver->interfaces; i++) {

                struct usbd_interface_description *interface_description = simple_driver->interface_list + i;

                TRACE_MSG3(USBD, "INTERFACE[%02d] interface_description: %x alternates: %d", i,
                                interface_description, interface_description->alternates);

                for (k = 0; k < interface_description->alternates; k++) {

                        struct usbd_alternate_description *alternate_description = interface_description->alternate_list + k;

                        struct usbd_interface_descriptor *interface_descriptor;

                        TRACE_MSG2(USBD, "ALTERNATE[%02d:%02d]", i, k);

                        /* copy alternate interface descriptor, update descriptor fields
                         */
                        interface_descriptor = (struct usbd_interface_descriptor *)(cp + cfg_size);

                        //cfg_size += copy_descriptor(cp + cfg_size, alternate_description->interface_descriptor);
                        cfg_size += sizeof(struct usbd_interface_descriptor);

                        interface_descriptor->bLength = sizeof(struct usbd_interface_descriptor);
                        interface_descriptor->bDescriptorType = USB_DT_INTERFACE;
                        interface_descriptor->bInterfaceNumber = i;
                        interface_descriptor->bAlternateSetting = k;
                        interface_descriptor->bNumEndpoints = alternate_description->endpoints;

                        interface_descriptor->bInterfaceClass = alternate_description->bInterfaceClass;
                        interface_descriptor->bInterfaceSubClass = alternate_description->bInterfaceSubClass;
                        interface_descriptor->bInterfaceProtocol = alternate_description->bInterfaceProtocol;
                        // XXX
                        interface_descriptor->iInterface = usbd_alloc_string (&simple_instance->function,
                                        alternate_description->iInterface);

                        TRACE_MSG6(USBD, "COPY  ALT[%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                        i, k, alternate_description->classes, alternate_description->endpoints,
                                        alternate_description->iInterface, cfg_size);

                        /* copy class descriptors
                         */
                        for (l = 0; l < alternate_description->classes; l++)
                                cfg_size += copy_descriptor(cp + cfg_size, *(alternate_description->class_list + l));

                        /* copy endpoint descriptors, update descriptor fields
                         */
                        for (l = 0; l < alternate_description->endpoints; l++) {
                                int index = alternate_description->endpoint_index[l];
                                struct usbd_endpoint_map *endpoint_map = simple_instance->endpoint_map_array + index;
                                struct usbd_endpoint_descriptor *endpoint_descriptor =
                                        (struct usbd_endpoint_descriptor *)(cp + cfg_size);

                                cfg_size += sizeof(struct usbd_endpoint_descriptor);
                                endpoint_descriptor->bLength = sizeof(struct usbd_endpoint_descriptor);;
                                endpoint_descriptor->bDescriptorType = USB_DT_ENDPOINT;
                                endpoint_descriptor->bEndpointAddress = endpoint_map->bEndpointAddress[hs];
                                endpoint_descriptor->bmAttributes = endpoint_map->bmAttributes[hs] & 0x3;
                                endpoint_descriptor->wMaxPacketSize = cpu_to_le16(endpoint_map->wMaxPacketSize[hs]);
                                endpoint_descriptor->bInterval = endpoint_map->bInterval[hs];
                        }
                }
        }
        TRACE_MSG2(USBD, "cfg: %x size: %d", cp, cfg_size);

        configuration_descriptor->wTotalLength = cpu_to_le16(cfg_size);
        configuration_descriptor->bNumInterfaces = simple_driver->interfaces;
        configuration_descriptor->bConfigurationValue = 1;

        #if 0
        for (i = 0; i < cfg_size; i+= 8) {
                TRACE_MSG8(USBD, "%02x %02x %02x %02x %02x %02x %02x %02x",
                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                          );
        }
        #endif
}

/*!
 * @brief usbp_dealloc_simple_instance() -
 *
 * @param simple_instance
 */
void usbp_dealloc_simple_instance(struct usbd_simple_instance *simple_instance)
{
        int i;

        usbd_strings_exit(simple_instance->function.bus);

        for (i = 0; i < 2; i++)
                if (simple_instance->configuration_descriptor[i])
                        LKFREE(simple_instance->configuration_descriptor[i]);

        if (simple_instance->endpoint_map_array) LKFREE(simple_instance->endpoint_map_array);

        //if (simple_instance->requestedEndpoints) LKFREE(simple_instance->requestedEndpoints);
        //if (simple_instance->interface_list) LKFREE(simple_instance->interface_list);

        LKFREE(simple_instance);
}

/*!
 * @brief usbp_alloc_simple_instance() - create a usbp_simple_instance
 *
 * @param simple_driver - usbd_simple_driver instance pointer
 * @param bus - usbd_bus_instance pointer
 * @return usbd_simple_instance
 */
struct usbd_simple_instance *
usbp_alloc_simple_instance(struct usbd_simple_driver *simple_driver,
                struct usbd_bus_instance *bus)
{
        struct usbd_simple_instance *simple_instance = NULL;
        int i, k, l;
        int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);
        struct usbd_configuration_descriptor *configuration_descriptor[2] = { NULL, NULL, };
        struct usbd_endpoint_map *endpoint_map_array = NULL;
        u8 *altsettings = NULL;

        THROW_UNLESS((simple_instance = CKMALLOC(sizeof(struct usbd_simple_instance))), error);
        simple_instance->function.bus = bus;
        THROW_IF(usbd_strings_init(bus, &simple_instance->function), error);

        /* copy fields
         */
        simple_instance->function.function_driver = (struct usbd_function_driver *)simple_driver;
        simple_instance->function.function_type = function_simple;
        simple_instance->function.name = simple_driver->driver.name;
        simple_instance->function.bus = bus;

        /*
         * Find and total descriptor sizes.
         */
        for (i = 0; i < simple_driver->interfaces; i++) {

                struct usbd_interface_description *interface_description = simple_driver->interface_list + i;

                for (k = 0; k < interface_description->alternates; k++) {

                        struct usbd_alternate_description *alternate_description =
                                interface_description->alternate_list + k;

                        /* size interface descriptor
                         */
                        cfg_size += sizeof(struct usbd_interface_descriptor);;

                        TRACE_MSG6(USBD, "SIZE ALT[%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                        i, k, alternate_description->classes,
                                        alternate_description->endpoints, alternate_description->iInterface, cfg_size);

                        /* size class descriptors
                         */
                        for (l = 0; l < alternate_description->classes; l++) {
                                struct usbd_generic_class_descriptor *class_descriptor =
                                        *(alternate_description->class_list + l);
                                cfg_size += class_descriptor->bLength;
                        }

                        cfg_size += alternate_description->endpoints * sizeof(struct usbd_endpoint_descriptor);

                        #if 0
                        /* size endpoint descriptors (in theory could be endpoints * sizeof())
                         */
                        for (l = 0; l < alternate_description->endpoints; l++) {
                                struct usbd_endpoint_descriptor *endpoint_descriptor =
                                        *(alternate_description->endpoint_list + l);
                                cfg_size += endpoint_descriptor->bLength;
                        }
                        #endif

                }
        }
        TRACE_MSG1(USBD, "cfg_size: %d", cfg_size);

        /* allocate configuration descriptor
         */
        #ifdef CONFIG_OTG_HIGH_SPEED
        for (i = 0; i < 2; i++)
                #else /* CONFIG_OTG_HIGH_SPEED */
                for (i = 0; i < 1; i++)
                        #endif /* CONFIG_OTG_HIGH_SPEED */
                {
                        THROW_UNLESS((configuration_descriptor[i] = (struct usbd_configuration_descriptor *)
                                                CKMALLOC (cfg_size + 4)), error);
                        simple_instance->configuration_descriptor[i] = configuration_descriptor[i];
                }
        simple_instance->configuration_size = cfg_size;

        /* allocate endpoint map array and altsetting
         */
        THROW_UNLESS((endpoint_map_array = (struct usbd_endpoint_map *) CKMALLOC (sizeof(struct usbd_endpoint_map) *
                                        simple_driver->endpointsRequested)), error);
        simple_instance->endpoint_map_array = endpoint_map_array;

        THROW_UNLESS((altsettings = (u8 *)CKMALLOC(sizeof (u8) * simple_instance->interfaces)), error);
        simple_instance->altsettings = altsettings;

        TRACE_MSG1(USBD, "request endpoints: %x", simple_driver->endpointsRequested);

        /* request endpoints, allocate configuration descriptor, updating endpoint descriptors with correct
         * information from endpoint map allocated above and set the endpoints
         */

        THROW_IF(bus->driver->bops->set_endpoints( bus, simple_driver->endpointsRequested,
                                simple_instance->endpoint_map_array), error);

        return simple_instance;

        CATCH(error) {
                TRACE_MSG0(USBD, "FAILED");
                for (i = 0; i < 2; i++) if (configuration_descriptor[i]) LKFREE(configuration_descriptor[i]);
                if (altsettings) LKFREE(altsettings);
                if (endpoint_map_array) LKFREE(endpoint_map_array);
                if (simple_instance) {
                        usbd_strings_exit(bus);
                        usbp_dealloc_simple_instance(simple_instance);
                }
                return NULL;
        }
}
/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * @brief usbp_dealloc_composite_instance() - free composite instance and associated resources
 *
 * @param composite_instance
 */
void usbp_dealloc_composite_instance(struct usbd_composite_instance *composite_instance)
{
        int i;
        for (i = 0; i < 2; i++)
                if (composite_instance->configuration_descriptor[i])
                        LKFREE(composite_instance->configuration_descriptor[i]);

        usbd_strings_exit(composite_instance->function.bus);
        if (composite_instance->interfaces_array) LKFREE(composite_instance->interfaces_array);
        if (composite_instance->endpoint_map_array) LKFREE(composite_instance->endpoint_map_array);
        if (composite_instance->requestedEndpoints) LKFREE(composite_instance->requestedEndpoints);
        if (composite_instance->interface_list) LKFREE(composite_instance->interface_list);

        LKFREE(composite_instance);
}


/*!
 * @brief usbp_alloc_composite_configuration_descriptor() -
 *
 * @param composite_instance
 * @param cfg_size
 * @param hs
 * @return pointer to configuration descriptor
 */
void usbp_alloc_composite_configuration_descriptor(struct usbd_composite_instance *composite_instance, int cfg_size, int hs)
{
        struct usbd_composite_driver *composite_driver = (struct usbd_composite_driver*)
                composite_instance->function.function_driver;
        struct usbd_bus_instance *bus = composite_instance->function.bus;
        struct usbd_configuration_description *configuration_description = composite_driver->configuration_description;

        struct usbd_configuration_descriptor *configuration_descriptor = composite_instance->configuration_descriptor[hs];
        u8 *cp = NULL;

        int i, j, k, l, m;

        /* Compose the configuration descriptor
         * First copy the function driver configuration configuration descriptor.
         */
        cp = (u8 *)configuration_descriptor;

        TRACE_MSG4(USBD, "bDeviceClass: %02x bus->bmAttributes: %02x descriptor[%d] %x",
                        composite_driver->device_description->bDeviceClass,
                        bus->bmAttributes,
                        hs, composite_instance->configuration_descriptor[hs]);

        //cfg_size = copy_descriptor(cp + 0, configuration_description->configuration_descriptor);
        cfg_size = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bLength = sizeof(struct usbd_configuration_descriptor);

        /* copy peripheral controller / platform dependant values into configuration */
        configuration_descriptor->bDescriptorType = USB_DT_CONFIGURATION;

        /* Self Powered devices must set bMaxPower to 1 unit */
        configuration_descriptor->bmAttributes = USB_BMATTRIBUTE_RESERVED |
                (bus->bmAttributes & (USB_BMATTRIBUTE_SELF_POWERED | USB_BMATTRIBUTE_REMOTE_WAKEUP));

        /* Self Powered devices must set bMaxPower to 1 unit */
        configuration_descriptor->bMaxPower =
                ((bus->bmAttributes & USB_BMATTRIBUTE_SELF_POWERED) || !bus->bMaxPower) ? 1: bus->bMaxPower;

        configuration_descriptor->iConfiguration = usbd_alloc_string (&composite_instance->function,
                        configuration_description->iConfiguration);

        for (i = 0; i < composite_instance->interface_functions; i++) {

                struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array +i;
                struct usbd_interface_driver *interface_driver = (struct usbd_interface_driver *)
                        interface_instance->function.function_driver;

                //TRACE_MSG6(USBD, "INTERFACE[%02d] interfaces: %d endpoints: %d bFunctionClass: %d hs: %d %s", i,
                //                interface_driver->interfaces, interface_driver->endpointsRequested,
                //                interface_driver->bFunctionClass, hs,
                //                interface_driver->driver.name);

                TRACE_MSG8(USBD, "INTERFACE[%02d:%02d] %x endpoints: %d endpoint_map_array: %x "
                                "bFunctionClass: %d hs: %d %s",
                                i, interface_driver->interfaces,
                                interface_instance,
                                interface_driver->endpointsRequested,
                                interface_instance->endpoint_map_array,
                                interface_driver->bFunctionClass,
                                hs,
                                interface_driver->driver.name);

                /* insert Interface Association Descriptor if required
                 */
                if ((USB_CLASS_MISC == composite_driver->device_description->bDeviceClass)) {

                        struct usbd_interface_association_descriptor *iad =
                                (struct usbd_interface_association_descriptor *) (cp + cfg_size);

                        TRACE_MSG4(USBD, "      IAD[%02d] Class: %d SubClass: %d Protocol: %d", i,
                                        interface_driver->bFunctionClass, interface_driver->bFunctionSubClass,
                                        interface_driver->bFunctionProtocol);

                        cfg_size += sizeof(struct usbd_interface_association_descriptor);
                        iad->bLength = sizeof(struct usbd_interface_association_descriptor);
                        iad->bDescriptorType = USB_DT_INTERFACE_ASSOCIATION;
                        iad->bFirstInterface = interface_instance->wIndex;
                        iad->bInterfaceCount = interface_driver->interfaces;
                        iad->bFunctionClass = interface_driver->bFunctionClass;
                        iad->bFunctionSubClass = interface_driver->bFunctionSubClass;
                        iad->bFunctionProtocol = interface_driver->bFunctionProtocol;
                        iad->iFunction = usbd_alloc_string (&composite_instance->function,
                                        interface_driver->iFunction);
                }

                /* iterate across interfaces and alternates for this for this function
                 * XXX this will need to be modified if interface functions are allowed
                 * to define multiple interfaces.
                 */

                for (j = 0; j < interface_driver->interfaces; j++) {

                        struct usbd_interface_description *interface_description = interface_driver->interface_list + j;

                        TRACE_MSG5(USBD, "INTERFACE[%02d:%02d] interface_description: %x alternates: %d wIndex: %d", i, j,
                                        interface_description, interface_description->alternates,
                                        interface_instance->wIndex);

                        for (k = 0; k < interface_description->alternates; k++) {

                                struct usbd_alternate_description *alternate_description =
                                        interface_description->alternate_list + k;

                                struct usbd_interface_descriptor *interface_descriptor;

                                TRACE_MSG3(USBD, "ALTERNATE[%02d:%02d:%02d]", i, j, k);

                                /* copy alternate interface descriptor, update descriptor fields
                                 */
                                interface_descriptor = (struct usbd_interface_descriptor *)(cp + cfg_size);

                                //cfg_size += copy_descriptor(cp + cfg_size, alternate_description->interface_descriptor);
                                cfg_size += sizeof(struct usbd_interface_descriptor);

                                //interface_descriptor->bInterfaceNumber = i;     // XXX see note above
                                interface_descriptor->bLength = sizeof(struct usbd_interface_descriptor);
                                interface_descriptor->bDescriptorType = USB_DT_INTERFACE;
                                interface_descriptor->bInterfaceNumber = interface_instance->wIndex + j;
                                interface_descriptor->bAlternateSetting = k;
                                interface_descriptor->bNumEndpoints = alternate_description->endpoints;

                                interface_descriptor->bInterfaceClass = alternate_description->bInterfaceClass;
                                interface_descriptor->bInterfaceSubClass = alternate_description->bInterfaceSubClass;
                                interface_descriptor->bInterfaceProtocol = alternate_description->bInterfaceProtocol;

                                interface_descriptor->iInterface = usbd_alloc_string (&composite_instance->function,
                                                alternate_description->iInterface);

                                TRACE_MSG7(USBD, "COPY  ALT[%02d:%02d:%02d] classes: %d endpoints: %d %s size: %d", i, j, k,
                                                alternate_description->classes, alternate_description->endpoints,
                                                alternate_description->iInterface, cfg_size);

                                /* copy class descriptors
                                 */
                                for (l = 0; l < alternate_description->classes; l++) {
                                        struct usbd_generic_class_descriptor *generic_descriptor =
                                                (struct usbd_generic_class_descriptor *)(cp + cfg_size);

                                        cfg_size += copy_descriptor(cp + cfg_size, *(alternate_description->class_list + l));

                                        TRACE_MSG7(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] type: %02x:%02x:%02x",
                                                        i, j, k, l,
                                                        generic_descriptor->bLength,
                                                        generic_descriptor->bDescriptorType,
                                                        generic_descriptor->bDescriptorSubtype
                                                  );

                                        /* update
                                         *      call management functional descriptor
                                         *      union functional descriptor
                                         * and update interface numbers
                                         */
                                        CONTINUE_UNLESS (USB_DT_CLASS_SPECIFIC == generic_descriptor->bDescriptorType);
                                        switch (generic_descriptor->bDescriptorSubtype) {
                                        case USB_ST_CMF: {
                                                struct usbd_call_management_functional_descriptor *cmfd =
                                                        (struct usbd_call_management_functional_descriptor *)
                                                        generic_descriptor;
                                                cmfd->bDataInterface += interface_instance->wIndex + j;
                                                TRACE_MSG5(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] CMF bDataInterface %02x",
                                                                i, j, k, l, cmfd->bDataInterface);
                                                break;
                                        }
                                        case USB_ST_UF: {
                                                struct usbd_union_functional_descriptor *ufd =
                                                        (struct usbd_union_functional_descriptor *)
                                                        generic_descriptor;
                                                ufd->bMasterInterface = interface_instance->wIndex + j;
                                                TRACE_MSG6(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] UFD "
                                                                "bMasterInterface %02x "
                                                                "bSlaveInterface %02x",
                                                                i, j, k, l,
                                                                ufd->bMasterInterface,
                                                                ufd->bSlaveInterface[0]
                                                          );

                                                for (m = 0; m < interface_driver->interfaces - 1; m++) {


                                                        TRACE_MSG7(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] UFD "
                                                                        "bSlaveInterface[%02x] %02x ->%02x",
                                                                        i, j, k, l, m,
                                                                        ufd->bSlaveInterface[m],
                                                                        ufd->bSlaveInterface[m] + ufd->bMasterInterface
                                                                  );
                                                        ufd->bSlaveInterface[m] += ufd->bMasterInterface;
                                                }
                                                break;
                                        }
                                        default: break;
                                        }
                                }

                                /* copy endpoint descriptors, update descriptor fields
                                 */
                                for (l = 0; l < alternate_description->endpoints; l++) {
                                        int index = alternate_description->endpoint_index[l];
                                        struct usbd_endpoint_map *endpoint_map = interface_instance->endpoint_map_array + index;
                                        struct usbd_endpoint_descriptor *endpoint_descriptor =
                                                (struct usbd_endpoint_descriptor *)(cp + cfg_size);

                                        TRACE_MSG4(USBD, "REQUEST[%02d:%02d] index: %d  endpoint_map: %x",
                                                        l, alternate_description->endpoints, index, endpoint_map);

                                        cfg_size += sizeof(struct usbd_endpoint_descriptor);
                                        endpoint_descriptor->bLength = sizeof(struct usbd_endpoint_descriptor);;
                                        endpoint_descriptor->bDescriptorType = USB_DT_ENDPOINT;
                                        endpoint_descriptor->bEndpointAddress = endpoint_map->bEndpointAddress[hs];
                                        endpoint_descriptor->bmAttributes = endpoint_map->bmAttributes[hs] & 0x3;
                                        endpoint_descriptor->wMaxPacketSize = cpu_to_le16(endpoint_map->wMaxPacketSize[hs]);
                                        endpoint_descriptor->bInterval = endpoint_map->bInterval[hs];
                                }
                        }
                }
        }

        #if defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE)
        {
                struct usbd_otg_descriptor *otg_descriptor = (struct usbd_otg_descriptor *)(cp + cfg_size);
                otg_descriptor->bLength = sizeof(struct usbd_otg_descriptor);
                otg_descriptor->bDescriptorType = USB_DT_OTG;
                otg_descriptor->bmAttributes = bus->driver->bmAttributes & (USB_OTG_HNP_SUPPORTED | USB_OTG_SRP_SUPPORTED);
                cfg_size += sizeof(struct usbd_otg_descriptor);
        }
        #endif /* defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE) */

        configuration_descriptor->wTotalLength = cpu_to_le16(cfg_size);
        configuration_descriptor->bNumInterfaces = composite_instance->interfaces;
        configuration_descriptor->bConfigurationValue = 1;

        TRACE_MSG2(USBD, "cfg: %x size: %d", cp, cfg_size);
        #if 0
        for (i = 0; i < cfg_size; i+= 8) {
                TRACE_MSG8(USBD, "%02x %02x %02x %02x %02x %02x %02x %02x",
                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                          );
        }
        #endif
}

/*!
 * alloc_function_interface_function() -
 *
 * @param interface_driver
 * @return pointer to interface instance
 */
struct usbd_interface_instance *
alloc_function_interface_function(struct usbd_interface_driver *interface_driver)
{
        struct usbd_interface_instance *function_instance = NULL;
        RETURN_NULL_UNLESS((function_instance = CKMALLOC(sizeof(struct usbd_interface_instance))));
        function_instance->function.function_type = function_interface;
        function_instance->function.name = interface_driver->driver.name;
        return function_instance;
}


/*!
 * alloc_function_class_function() -
 *
 * @param class_driver
 * @return pointer to class instance
 */
struct usbd_class_instance *
alloc_function_class_function(struct usbd_class_driver *class_driver)
{
        struct usbd_class_instance *function_instance = NULL;
        RETURN_NULL_UNLESS((function_instance = CKMALLOC(sizeof(struct usbd_class_instance))));
        function_instance->function.function_type = function_class;
        function_instance->function.name = class_driver->driver.name;
        return function_instance;
}

/*!
 * alloc_usb_string
 */
void alloc_usb_strings(struct usbd_function_instance *function, u8 *index, const char **str)
{
        TRACE_MSG4(USBD, "function: %x index: %x str: %x %d", function, index, str, str ? *str : NULL);

        for ( ; str && *str; str++, index++) {

                TRACE_MSG2(USBD, "str: %s index: %d", *str, *index);

                if (*index)
                        usbd_realloc_string (function, *index, *str);
                else
                        usbd_alloc_string (function, *str);
        }
}

/*!
 * @brief usbp_alloc_composite_instance() - register a usbd function driver
 *
 * USBD Composite Function drivers call this to register with the USBD Core layer.
 *
 * This takes a list of interface functions (by name) to use. Each of these
 * must be found and the interface descriptor and endpoint request information
 * must be copied.
 *
 * @param composite_driver
 * @param bus
 *
 * @return function instance or  NULL on failure.
 *
 */
struct usbd_composite_instance *
usbp_alloc_composite_instance (struct usbd_composite_driver *composite_driver, struct usbd_bus_instance *bus)
{
        struct usbd_composite_instance *composite_instance = NULL;
        //struct usbd_class_instance *class_instnace = NULL;
        //struct usbd_class_driver *class_driver = NULL;
        struct usbd_interface_instance *interfaces_array = NULL;
        struct usbd_endpoint_request *requestedEndpoints = NULL;
        struct usbd_endpoint_map *endpoint_map_array = NULL;
        struct usbd_interface_description *interface_list = NULL;

        u32 interfaces_used;

        const char ** interface_function_names = composite_driver->interface_function_names;

        int i = 0, j = 0, k, l;
        int endpoint_number, interface_number, function_number = 0;
        //u8 *cp;

        int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);

        THROW_UNLESS((composite_instance = CKMALLOC(sizeof(struct usbd_composite_instance))), error);
        composite_instance->function.bus = bus;
        THROW_IF(usbd_strings_init(bus, &composite_instance->function), error);

        /* copy fields
         */
        composite_instance->function.function_driver = (struct usbd_function_driver *)composite_driver;
        composite_instance->function.function_type = function_composite;
        composite_instance->function.name = composite_driver->driver.name;
        composite_instance->function.bus = bus;
        alloc_usb_strings((struct usbd_function_instance*)composite_instance, composite_driver->driver.usb_string_indexes,
                        composite_driver->driver.usb_strings);

        /* count and list interfaces, then allocate interfaces_array.
         */
        for (endpoint_number = interface_number = function_number = 0;
                        *(interface_function_names + function_number);
                        function_number++)
        {
                const char *interface_name = *(interface_function_names + function_number);
                char *cp;
                struct usbd_interface_driver *interface_driver;
                int interfaces;

                if ((cp = strchr(interface_name, '=')))
                        interface_name = cp + 1;


                TRACE_MSG1(USBD, "interface_name: \"%s\"", interface_name ? interface_name : "");

                THROW_UNLESS(interface_driver = usbp_find_interface_driver(interface_name), error);

                TRACE_MSG5(USBD, "INTERFACE[%02d] interfaces: %d endpoints: %d bFunctionClass: %d %s",
                                endpoint_number, interface_driver->interfaces,
                                interface_driver->endpointsRequested, interface_driver->bFunctionClass,
                                interface_driver->driver.name);

                interfaces = interface_driver->interfaces;

                TRACE_MSG6(USBD, "FUNCTION[%02d] interfaces: %d:%d endpoints: %d:%d %20s",
                                function_number, interface_driver->interfaces, interface_number,
                                interface_driver->endpointsRequested, endpoint_number, interface_name);

                interface_number += interface_driver->interfaces;
                endpoint_number += interface_driver->endpointsRequested;

                if ((USB_CLASS_MISC == composite_driver->device_description->bDeviceClass) ) {
                        cfg_size += sizeof(struct usbd_interface_association_descriptor);
                        TRACE_MSG2(USBD, "FUNCTION[%02d] IAD size: %d", function_number, cfg_size);
                }

                /* iterate across interfaces for this function and their alternates to get descriptors sizes
                 */
                for (j = 0; j < interface_driver->interfaces; j++) {

                        struct usbd_interface_description *interface_description = interface_driver->interface_list + j;

                        for (k = 0; k < interface_description->alternates; k++) {

                                struct usbd_alternate_description *alternate_description =
                                        interface_description->alternate_list + k;

                                /* size interface descriptor
                                 */
                                cfg_size += sizeof(struct usbd_interface_descriptor);;

                                TRACE_MSG7(USBD, "SIZE ALT[%02d:%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                                function_number, j, k, alternate_description->classes,
                                                alternate_description->endpoints, alternate_description->iInterface, cfg_size);

                                /* size class descriptors
                                 */
                                for (l = 0; l < alternate_description->classes; l++) {
                                        struct usbd_generic_class_descriptor *class_descriptor =
                                                *(alternate_description->class_list + l);
                                        cfg_size += class_descriptor->bLength;
                                }

                                cfg_size += alternate_description->endpoints * sizeof(struct usbd_endpoint_descriptor);

                                #if 0
                                /* size endpoint descriptors (in theory could be endpoints * sizeof())
                                 */
                                for (l = 0; l < alternate_description->endpoints; l++) {
                                        struct usbd_endpoint_descriptor *endpoint_descriptor =
                                                *(alternate_description->endpoint_list + l);
                                        cfg_size += endpoint_descriptor->bLength;
                                }
                                #endif
                        }
                }
        }

        TRACE_MSG2(USBD, "interfaces: %d endpoints: %d", interface_number, endpoint_number);

        /* Allocate and save information.
         *
         * Save endpoints, bNumInterfaces and configuration size, Allocate
         * configuration descriptor, allocate enough memory for all
         * interface instances plus one extra for class instance if
         * required, allocate endpoint map array and endpoint requests.
         */
        /*
           THROW_UNLESS((configuration_descriptor =
           (struct usbd_configuration_descriptor *) CKMALLOC (cfg_size + 4)), error);
           composite_instance->configuration_descriptor = configuration_descriptor;
         */

        THROW_UNLESS((interfaces_array = (struct usbd_interface_instance *)
                                CKMALLOC (sizeof(struct usbd_interface_instance) *
                                        (function_number + (composite_driver->class_name ? 1 : 0)))), error);

        THROW_UNLESS((endpoint_map_array = (struct usbd_endpoint_map *)
                                CKMALLOC (sizeof(struct usbd_endpoint_map) * endpoint_number)), error);

        THROW_UNLESS((requestedEndpoints = (struct usbd_endpoint_request *)
                                CKMALLOC (sizeof(struct usbd_endpoint_request) * endpoint_number)), error);

        THROW_UNLESS((interface_list = (struct usbd_interface_description *)
                                CKMALLOC (sizeof(struct usbd_interface_description) * interface_number)), error);


        #ifdef CONFIG_OTG_HIGH_SPEED
        for (i = 0; i < 2; i++)
                #else /* CONFIG_OTG_HIGH_SPEED */
                for (i = 0; i < 1; i++)
                        #endif /* CONFIG_OTG_HIGH_SPEED */
                {
                        THROW_UNLESS((composite_instance->configuration_descriptor[i] =
                                                (struct usbd_configuration_descriptor *) CKMALLOC (cfg_size + 4)),
                                        error);
                }

        composite_instance->interfaces_array = interfaces_array;
        composite_instance->endpoint_map_array = endpoint_map_array;
        composite_instance->requestedEndpoints = requestedEndpoints;
        composite_instance->interface_list = interface_list;

        composite_instance->configuration_size = cfg_size;                      // total configuration size
        composite_instance->interface_functions = function_number;              // number of interface function drivers
        composite_instance->interfaces = interface_number;                      // number of interfaces
        composite_instance->endpointsRequested = endpoint_number;               // number of endpoints
        composite_instance->endpoints = endpoint_number;

        /* Setup interface_instance array.
         *
         * Find all interfaces and compose the interface list, count the
         * endpoints and allocte the endpoint request array. Also find and
         * total descriptor sizes.  Compose the endpoint request array and
         * configuration descriptor, cannot request endpoints yet as the bus
         * interface driver is not loaded, so we cannot compose
         * configuration descriptor here.
         */
        for (interfaces_used = endpoint_number = interface_number = function_number = 0;
                        function_number < composite_instance->interface_functions;
                        function_number++)
        {
                struct usbd_interface_instance *interface_instance = interfaces_array + function_number;
                const char *interface_name = *(interface_function_names + function_number);
                struct usbd_interface_driver *interface_driver;
                const char *cp;
                int wIndex = 0;

                if ((cp = strchr(interface_name, '='))) {
                        const char *sp = interface_name;
                        interface_name = cp + 1;
                        for (; sp < cp; sp++) {
                                BREAK_UNLESS(isdigit(*sp));
                                wIndex = wIndex * 10 + *sp - '0';
                        }
                }
                else {
                        for (wIndex = 0; wIndex < 32; wIndex++) {
                                BREAK_UNLESS(interfaces_used & (1 << wIndex));
                        }
                        THROW_IF(wIndex == 32, error);
                }

                THROW_UNLESS(interface_driver = usbp_find_interface_driver(interface_name), error);

                interface_instance->endpoint_map_array = composite_instance->endpoint_map_array + endpoint_number;
                interface_instance->function.function_driver = (struct usbd_function_driver *)interface_driver;
                interface_instance->function.function_type = function_interface;
                interface_instance->function.name = interface_driver->driver.name;
                interface_instance->function.bus = bus;
                interface_instance->wIndex = wIndex;
                interface_instance->lIndex = j;

                alloc_usb_strings((struct usbd_function_instance*)interface_instance,
                                interface_driver->driver.usb_string_indexes,
                                interface_driver->driver.usb_strings);

                for (i = 0; i < interface_driver->interfaces; i++) {
                        THROW_IF(interfaces_used & (1 << (wIndex + 1)), error);
                        interfaces_used |= (1 << (wIndex + i));
                }

                TRACE_MSG6(USBD, "INTERFACE FUNCTION[%02d] %x interfaces: %d endpointsRequested: %d "
                                "wIndex: %d endpoint_map_array: %x",
                                function_number, interface_instance, interface_driver->interfaces,
                                interface_driver->endpointsRequested, wIndex,
                                interface_instance->endpoint_map_array);

                /* copy requested endpoint information
                 */
                for (k = 0; k < interface_driver->endpointsRequested; k++, endpoint_number++) {

                        TRACE_MSG5(USBD, "REQUEST[%2d %d:%d %d:%d]",
                                        function_number, j, interface_number, k, endpoint_number);

                        TRACE_MSG3(USBD, "%x %x %d", requestedEndpoints + endpoint_number,
                                        interface_driver->requestedEndpoints + j,
                                        sizeof(struct usbd_endpoint_request));

                        TRACE_MSG5(USBD, "REQUEST[%2d:%2d:%2d] bm: %02x addr: %02x", function_number,
                                        k, endpoint_number,
                                        interface_driver->requestedEndpoints[k].bmAttributes,
                                        interface_driver->requestedEndpoints[k].bEndpointAddress
                                  );

                        memcpy(requestedEndpoints + endpoint_number, interface_driver->requestedEndpoints + k,
                                        sizeof(struct usbd_endpoint_request));

                        requestedEndpoints[endpoint_number].interface_num += wIndex;
                }

                interface_number += interface_driver->interfaces;
        }

        if (composite_driver->class_name) {

                struct usbd_class_instance *class_instance = (struct usbd_class_instance *)(interfaces_array + function_number);
                struct usbd_class_driver *class_driver;

                THROW_UNLESS(class_driver = usbp_find_class_driver(composite_driver->class_name), error);

                class_instance->function.function_driver = (struct usbd_function_driver *)composite_driver;
                class_instance->function.function_type = function_composite;
                class_instance->function.name = composite_driver->driver.name;
                class_instance->function.bus = bus;
                alloc_usb_strings((struct usbd_function_instance *)class_instance, class_driver->driver.usb_string_indexes,
                                class_driver->driver.usb_strings);
        }

        return composite_instance;

        CATCH(error) {
                TRACE_MSG0(USBD, "FAILED");
                //if (configuration_descriptor) LKFREE(configuration_descriptor);
                if (composite_instance) {
                        usbd_strings_exit(bus);
                        for (i = 0; i < 2; i++) if (composite_instance->configuration_descriptor[i])
                                LKFREE(composite_instance->configuration_descriptor[i]);
                }

                if (requestedEndpoints) LKFREE(requestedEndpoints);
                if (interfaces_array) LKFREE(interfaces_array);
                if (interface_list) LKFREE(interface_list);
                return NULL;
        }
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Function Registration:
 *      usbd_register_function_function()
 *      usbd_register_interface_function()
 *      usbd_register_composite_function()
 *      usbd_deregister_function()
 *      usbd_interface_deregister_function()
 *      usbd_class_deregister_function()
 *      usbd_composite_deregister_function()
 *      usbd_find_function()
 *      usbd_find_interface_function()
 *
 */

/*!
 * @brief usbd_find_function_internal() - search a usbd_function_driver instance by name
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param usbd_function_drivers - registerd usbd_function_driver table
 * @param name - name to search
 * @return function instance
 *
 */
struct usbd_function_driver *
usbd_find_function_internal(struct otg_list_node *usbd_function_drivers, const char *name)
{
        int len = name ? strlen(name) : 0;
        struct otg_list_node *lhd = NULL;

	#if defined(CONFIG_OTG_TRACE)
	TRACE_STRING(USBD, "name:: %s", (char *)name);
	#endif

        LIST_FOR_EACH (lhd, usbd_function_drivers) {

                struct usbd_function_driver *function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);

                CONTINUE_IF(len && strncmp(function_driver->name, name, len));

		#if defined(CONFIG_OTG_TRACE)
                TRACE_MSG1(USBD, "FOUND: %s", function_driver->name);
		#endif

                return function_driver;
        }
        return NULL;
}


/*!
 * usbd_function_name() - return name of nth function driver
 * @param n index to function name
 * @return pointer to name
 */
char * usbd_function_name(int n)
{
        struct usbd_function_driver *function_driver = NULL;
        struct otg_list_node *lhd = NULL;

        TRACE_MSG1(USBD, "n: %d", n);
        LIST_FOR_EACH (lhd, &usbd_simple_drivers) {
                function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                TRACE_MSG2(USBD, "simple name: [%d] %s", n, function_driver->name);
                CONTINUE_IF(n--);
                TRACE_MSG1(USBD, "simple: %s", function_driver->name);
                return (char *)function_driver->name;
        };

        LIST_FOR_EACH (lhd, &usbd_composite_drivers) {
                function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                TRACE_MSG2(USBD, "composite name: [%d] %s", n, function_driver->name);
                CONTINUE_IF(n--);
                TRACE_MSG1(USBD, "composite: %s", function_driver->name);
                return (char *)function_driver->name;
        };

        return NULL;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * String Descriptors:
 *      usbd_alloc_string_descriptor()
 *      usbd_free_string_descriptor()
 *      usbd_get_string_descriptor()
 */

#define LANGID_ENGLISH          "\011"
#define LANGID_US_ENGLISH       "\004"
#define LANGIDs  LANGID_US_ENGLISH LANGID_ENGLISH

/*!
 * usbd_alloc_string_zero() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 *
 * @param str string to allocate
 * @param bus
 * @return the slot number
 */
static u8 usbd_alloc_string_zero (struct usbd_bus_instance *bus, char *str)
{
        u8 bLength;
        //u16 *wData;
        struct usbd_langid_descriptor *langid = NULL;
        unsigned char *cp;

        RETURN_ZERO_IF(bus->usb_strings[0] != NULL);

        TRACE_MSG2(USBD, "LANGID: %02x %02x", str[0], str[1]);

        bLength = 4;

        RETURN_ZERO_IF(!(langid = (struct usbd_langid_descriptor *) CKMALLOC (bLength)));


        langid->bLength = bLength;                              // set descriptor length
        langid->bDescriptorType = USB_DT_STRING;                // set descriptor type
        langid->bData[0] = str[1];
        langid->bData[1] = str[0];

        bus->usb_strings[0] = (struct usbd_string_descriptor *)langid;    // save in string index array

        TRACE_MSG4(USBD, "LANGID String: %02x %02x %02x %02x",
                        langid->bLength, langid->bDescriptorType, langid->bData[0], langid->bData[1]);
        cp = (char *) langid;
        TRACE_MSG4(USBD, "LANGID String: %02x %02x %02x %02x", cp[0], cp[1], cp[2], cp[3]);

        return 0;
}

/*!
 * usbd_strings_init() - initialize usb strings pool
 */
int usbd_strings_init(struct usbd_bus_instance *bus, struct usbd_function_instance *function_instance)
{
        bus->usbd_maxstrings = 50; // XXX Need configuration parameter...

        RETURN_EINVAL_IF (!(bus->usb_strings = CKMALLOC (
                                        sizeof (struct usbd_string_descriptor *) * bus->usbd_maxstrings)));

        #if defined(CONFIG_OTG_LANGID)
        {
                char langid_str[4];
                u16 langid;
                TRACE_MSG1(USBD, "LANGID: %04x", CONFIG_OTG_LANGID);
                langid = cpu_to_le16(CONFIG_OTG_LANGID);
                memset(langid_str, 0, sizeof(langid_str));
                langid_str[0] = langid & 0xff;
                langid_str[1] = (langid >> 8) & 0xff;
                if (usbd_alloc_string_zero(bus, langid_str)) {
                        LKFREE (function_instance->bus->usb_strings);
                        return -EINVAL;
                }
        }
        #else /* defined(CONFIG_OTG_LANGID) */

        if (usbd_alloc_string_zero(ubs, LANGIDs) != 0) {
                LKFREE (function_instance->bus->usb_strings);
                return -EINVAL;
        }
        #endif /* defined(CONFIG_OTG_LANGID) */
        return 0;
}

/*!
 * usbd_strings_exit() - de-initialize usb strings pool
 */
void usbd_strings_exit(struct usbd_bus_instance *bus)
{
        int i;
        RETURN_IF (!bus->usb_strings);
        for (i = 0; i < bus->usbd_maxstrings; i++)
                usbd_free_string_descriptor(bus, i);
        LKFREE (bus->usb_strings);
        bus->usb_strings = NULL;
}


/*!
 * usbd_get_string_descriptor() - find and return a string descriptor
 *
 * Find an indexed string and return a pointer to a it.
 * @param function_instance
 * @param index index of string
 * @return pointer to string descriptor or NULL
 */
struct usbd_string_descriptor *usbd_get_string_descriptor (struct usbd_function_instance *function_instance, u8 index)
{
        RETURN_NULL_IF (index >= function_instance->bus->usbd_maxstrings);
        return function_instance->bus->usb_strings[index];
}

/*!
 * usbd_realloc_string() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 * @param function_instance
 * @param index
 * @param str
 * @return new index
 */
u8 usbd_realloc_string (struct usbd_function_instance *function_instance, u8 index, const char *str)
{
        const char *save = str;
        struct usbd_string_descriptor *string;
        u8 bLength;
        u16 *wData;

        RETURN_EINVAL_IF(!str || !strlen (str));

        // XXX should have semaphores...

        if ((index < function_instance->bus->usbd_maxstrings) && (string = function_instance->bus->usb_strings[index])) {
                function_instance->bus->usb_strings[index] = NULL;
                LKFREE (string);
        }

        bLength = USBD_STRING_SIZEOF (struct usbd_string_descriptor) + 2 * strlen (str);

        RETURN_ENOMEM_IF(!(string = (struct usbd_string_descriptor *)CKMALLOC (bLength)));

        string->bLength = bLength;
        string->bDescriptorType = USB_DT_STRING;

        for (wData = string->wData; *str;)
                *wData++ = cpu_to_le16 ((u16) (*str++));

        // store in string index array
        function_instance->bus->usb_strings[index] = string;

        TRACE_MSG2(USBD, "STRING[%d] %s", index, save);
        return index;
}

/*!
 * @brief  strtest() - check if a string exists in string descriptor table
 *
 * @param string - string descriptor table
 * @param str  - string to test
 * @return non-zero if match
 */
int strtest(struct usbd_string_descriptor *string, const char *str)
{
        int i;
        for (i = 0; i < string->bLength; i++ ) {
                CONTINUE_IF ( (*str == (string->wData[i] & 0xff)));
                return 1;
        }
        return 0;
}


/*!
 * usbd_alloc_string() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 * @param function_instance
 * @param str
 * @return index
 *
 * XXX rename to usbd_alloc_string_descriptor
 */
u8 usbd_alloc_string (struct usbd_function_instance *function_instance, const char *str)
{
        //const char *save = str;
        //int i, j;
        int i;
        struct usbd_string_descriptor *string = NULL;
        u8 bLength;
        u16 *wData;

        TRACE_MSG2(USBD, "function_instance: %x str: %x", function_instance, str);

        RETURN_ZERO_IF(!str || !strlen (str));

        /* find an empty string descriptor slot
         * "0" is consumed by the language ID, "1" is the optional IP address
         * and "2" is for the product string (as required by the PST group)
         */
        for (i = 3; i < function_instance->bus->usbd_maxstrings; i++) {

                if (function_instance->bus->usb_strings[i]) {
                        UNLESS (strtest(function_instance->bus->usb_strings[i], str))
                                return i;
                        continue;
                }

                bLength = USBD_STRING_SIZEOF (struct usbd_string_descriptor) + 2 * strlen (str);

                RETURN_ZERO_IF(!(string = (struct usbd_string_descriptor *)CKMALLOC (bLength)));

                string->bLength = bLength;
                string->bDescriptorType = USB_DT_STRING;

                for (wData = string->wData; *str;)
                        *wData++ = cpu_to_le16((u16) (*str++));

                // store in string index array
                function_instance->bus->usb_strings[i] = string;
                //TRACE_MSG2(USBD, "STRING[%d] %s", i, save);
                return i;
        }
        return 0;
}


/*!
 * usbd_free_string_descriptor() - deallocate a string descriptor
 *
 * Find and remove an allocated string.
 * @param bus
 * @param index
 *
 */
void usbd_free_string_descriptor (struct usbd_bus_instance *bus, u8 index)
{
        struct usbd_string_descriptor *string;

        if ((index < bus->usbd_maxstrings) && (string = bus->usb_strings[index])) {
                bus->usb_strings[index] = NULL;
                LKFREE (string);
        }
}

OTG_EXPORT_SYMBOL(usbd_realloc_string);
OTG_EXPORT_SYMBOL(usbd_alloc_string);
OTG_EXPORT_SYMBOL(usbd_free_string_descriptor);
OTG_EXPORT_SYMBOL(usbd_get_string_descriptor);
//OTG_EXPORT_SYMBOL(usbd_maxstrings);


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Device Information:
 *      usbd_high_speed()
 *      usbd_get_bMaxPower()
 *      usbd_endpoint_wMaxPacketsize()
 *      usbd_endpoint_wMaxPacketsize_ep0()
 *      usbd_endpoint_bEndpointAddress()
 *
 */


/*!
 * usbd_high_speed() - return high speed status
 * @return true if operating at high speed
 */
BOOL usbd_high_speed(struct usbd_function_instance *function)
{
        // XXX TODO - per function modifications for composite devices
        return BOOLEAN(function->bus->high_speed);
}

/*!
 * usbd_get_bMaxPower() - process a received urb
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param function
 * @return non-zero if errror
 */
int usbd_get_bMaxPower(struct usbd_function_instance *function)
{
        struct usbd_bus_instance *bus = function->bus;
        return bus->driver->bMaxPower;
}

/*! usbd_endpoint_map()
 *
 * @param function
 * @return pointer to endpoint map
 */
struct usbd_endpoint_map *usbd_endpoint_map(struct usbd_function_instance *function)
{
        struct usbd_endpoint_map *endpoint_map = NULL;
        RETURN_NULL_UNLESS(function);
        switch(function->function_type) {
        case function_simple:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_simple_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_composite:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_composite_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_interface:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_interface_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_ep0:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_interface_instance *)function)->endpoint_map_array));
                return endpoint_map;
        default:
                return NULL;
        }
}

/*!
 * usbd_endpoint_wMaxPacketSize() - get maximum packet size for endpoint
 * @param function
 * @param endpoint_index
 * @param hs highspeed flag
 * @return endpoint size
 */
int usbd_endpoint_wMaxPacketSize(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        //TRACE_MSG4(USBD, "function: %x index: %d endpoint_map: %x wMaxPacketSize: %02x",
        //                function, endpoint_index, endpoint_map, endpoint_map[endpoint_index].wMaxPacketSize[hs]);
        //return le16_to_cpu(endpoint_map[endpoint_index].wMaxPacketSize[hs]);
        return endpoint_map[endpoint_index].wMaxPacketSize[hs];
}

/*!
 * usbd_endpoint_zero_wMaxPacketSize() - get maximum packet size for endpoint zero
 * @param function
 * @param hs highspeed flag
 * @return endpoint size
 */
int usbd_endpoint_zero_wMaxPacketSize(struct usbd_function_instance *function, int hs)
{
        struct usbd_endpoint_map *endpoint_map;
        RETURN_ZERO_IF(!(endpoint_map = function->bus->ep0->endpoint_map_array));
        //return le16_to_cpu(endpoint_map[0].wMaxPacketSize[hs]);
        return endpoint_map[0].wMaxPacketSize[hs];
}

/*!
 * usbd_endpoint_bEndpointAddress() - get endpoint addrsess
 * @param function
 * @param endpoint_index
 * @param hs high speed flag
 * @return endpoint address
 */
int usbd_endpoint_bEndpointAddress(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        //TRACE_MSG3(USBD, "function: %x index: %d endpoint_map: %x", function, endpoint_index, endpoint_map);
        return endpoint_map[endpoint_index].bEndpointAddress[hs];
}

OTG_EXPORT_SYMBOL(usbd_endpoint_map);
OTG_EXPORT_SYMBOL(usbd_endpoint_wMaxPacketSize);
OTG_EXPORT_SYMBOL(usbd_endpoint_zero_wMaxPacketSize);
OTG_EXPORT_SYMBOL(usbd_endpoint_bEndpointAddress);
OTG_EXPORT_SYMBOL(usbd_high_speed);

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * @name STRUCT
 * @brief
 * Structure member address manipulation macros.
 *
 * These are used by client code (code using the urb_link routines), since
 * the urb_link structure is embedded in the client data structures.
 *
 * Note: a macro offsetof equivalent to member_offset is defined in stddef.h
 *       but this is kept here for the sake of portability.
 *
 * p2surround returns a pointer to the surrounding structure given
 * type of the surrounding structure, the name memb of the structure
 * member pointed at by ptr.  For example, if you have:
 *
 *      struct foo {
 *          int x;
 *          float y;
 *          char z;
 *      } thingy;
 *
 *      char *cp = &thingy.z;
 *
 * then
 *      &thingy == p2surround(struct foo, z, cp)
 *
 * @{
 */

#define _cv_(ptr)                 ((char*)(void*)(ptr))
#define member_offset(type,memb)  (_cv_(&(((type*)0)->memb))-(char*)0)
#define p2surround(type,memb,ptr) ((type*)(void*)(_cv_(ptr)-member_offset(type,memb)))

/*!
 * @name list
 * @brief List support functions
 *
 * @{
 */

/*!
 * @brief  urb_link() - return first urb_link in list
 *
 * Return the first urb_link in a list with a distinguished
 * head "hd", or NULL if the list is empty.  This will also
 * work as a predicate, returning NULL if empty, and non-NULL
 * otherwise.
 *
 * Called from interrupt.
 *
 * @param hd
 * @return link to urb
 */
static INLINE urb_link *first_urb_link (urb_link * hd)
{
        urb_link *nx;
        return (!hd || !(nx = hd->next) || (nx == hd)) ? NULL : nx;
}

/*!
 * @brief  _usbd_first_urb() - return first urb in list
 *
 * Return the first urb in a list with a distinguished
 * head "hd", or NULL if the list is empty.
 *
 * Called from interrupt.
 *
 * @param hd linked list of urbs
 * @return pointer to urb
 */
struct usbd_urb *_usbd_first_urb(urb_link * hd)
{
        urb_link *nx = first_urb_link (hd);
        struct usbd_urb *urb = nx ? p2surround (struct usbd_urb, link, nx) : NULL;
        return urb;
}

#if 0
/*!
 * @brief  _usbd_unlink_urb() - return first urb in list
 *
 * Return the first urb in a list with a distinguished
 * head "hd", or NULL if the list is empty.
 *
 * Called from interrupt.
 *
 * @param urb linked list of urbs
 * @return pointer to urb
 */
void INLINE _usbd_unlink_urb(struct usbd_urb *urb)
{
        urb_link *ul = &urb->link;
        ul->next->prev = ul->prev;
        ul->prev->next = ul->next;
        ul->prev = ul->next = ul;
}
#endif

/*!
 * @brief  usbd_first_urb_detached() - detach an return first urb
 *
 * Detach and return the first urb in a list with a distinguished
 * head "hd", or NULL if the list is empty.
 * @param endpoint  pointer to endpoint instance
 * @param hd linked list of urbs
 * @return pointer to urb or NULL
 */
struct usbd_urb *usbd_first_urb_detached (struct usbd_endpoint_instance *endpoint, urb_link * hd)
{
        struct usbd_urb *urb;
        otg_pthread_mutex_lock(&endpoint->mutex);         /* lock mutex */
        if ((urb = _usbd_first_urb (hd))) {
                usbd_unlink_urb(urb);
                hd->total-=1;
        }
        otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */
        return urb;
}

/* @} */


/*!
 * @brief  usbd_first_finished_urb_detached() - detach an return first urb
 *
 * Detach and return the first urb in a list with a distinguished
 * head "hd", or NULL if the list is empty.
 * @param endpoint  pointer to endpoint instance
 * @param hd linked list of urbs
 * @return pointer to urb or NULL
 */
struct usbd_urb *usbd_first_finished_urb_detached (struct usbd_endpoint_instance *endpoint, urb_link * hd)
{
        struct usbd_urb *urb;

        otg_pthread_mutex_lock(&endpoint->mutex);         /* lock mutex */
        if ((urb = _usbd_first_urb(hd))) {
                //TRACE_MSG0(USBD,"enter1");
                if  (urb->irq_flags == USBD_URB_FINISHED) {
                        //TRACE_MSG0(USBD,"enter2");
                        usbd_unlink_urb(urb);
                        hd->total-=1;
                        otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */
                        //TRACE_MSG1(USBD,"return urb:%x",urb);
                        return urb;
                }
        }
        //TRACE_MSG0(USBD,"return NULL");
        otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */
        return NULL;
}

/*!
 * @brief  usbd_first_ready_urb() - detach an return first urb
 *
 * Find the first ready urb, mark as active and return pointer.
 *
 * N.B. must be called with interrupts locked.
 * @param endpoint  pointer to endpoint instance
 * @param hd linked list of urbs
 * @return pointer to urb or NULL
 */
struct usbd_urb *usbd_first_ready_urb(struct usbd_endpoint_instance *endpoint, urb_link * hd)
{
        //struct usbd_urb *urb;
        urb_link *ul;
        otg_pthread_mutex_lock(&endpoint->mutex);         /* lock mutex */
        for (ul = hd->next; ul != hd; ) {
                struct usbd_urb *urb = p2surround (struct usbd_urb, link, ul);
                ul = urb->link.next;
                CONTINUE_UNLESS(urb->irq_flags & USBD_URB_READY);
                urb->irq_flags |= USBD_URB_ACTIVE;
                //hd->total-=1;
                otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */

                return urb;
        }
        otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */
        return NULL;
}

/* ************************************************************************** */
/*!
 * Device I/O:
 *      usbd_urb_finished()
 *      usbd_first_urb_detached()
 *      usbd_find_endpoint_address()
 */

/*!
 * @brief  urb_append() - append a linked list of urbs to another linked list
 * @param hd link to urb list
 * @param urb to append to list
 */
void urb_append (urb_link * hd, struct usbd_urb *urb)
{
        struct usbd_endpoint_instance *endpoint = urb->endpoint;
        urb_link *new;
        urb_link *pul;
        RETURN_UNLESS (hd && urb);
        otg_pthread_mutex_lock(&endpoint->mutex);         /* lock mutex */
        new = &urb->link;
        pul = hd->prev;
        new->prev->next = hd;
        hd->prev = new->prev;
        new->prev = pul;
        pul->next = new;
        hd->total+=1;
        otg_pthread_mutex_unlock(&endpoint->mutex);      /* unlock mutex */
}


/* ********************************************************************************************* */
/*!
 * Device Control:
 *
 *      XXX usbd_request_endpoints()
 *      XXX usbd_configure_endpoints()
 *
 *      usbd_get_device_state()
 *      usbd_get_device_status()
 *      usbd_framenum()
 *      usbd_ticks()
 *      usbd_elapsed()
 */

/*!
 * usbd_flush_endpoint() - flush urbs from endpoint
 *
 * Iterate across the approrpiate tx or rcv list and cancel any outstanding urbs.
 *
 * @param endpoint flush urbs on this endpoint
 */
void usbd_flush_endpoint (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *urb;

        if (TRACE_VERBOSE)
                if (endpoint->bEndpointAddress)
                        TRACE_MSG1(USBD, "bEndpointAddress: %02x", endpoint->bEndpointAddress);

        while ((urb = endpoint->active_urb))
                usbd_cancel_urb(urb);

        for (; (urb = usbd_first_urb_detached (endpoint, &endpoint->rdy)); usbd_cancel_urb(urb))
        TRACE_MSG1(USBD, "cancelled urb:%x", urb);
}


/*!
 * usbd_flush_endpoint_address() - flush endpoint
 * @param function
 * @param endpoint_index
 */
void usbd_flush_endpoint_index (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;

        RETURN_UNLESS(function && endpoint_map);
        RETURN_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));
        usbd_flush_endpoint (endpoint);
}

/*!
 * usbd_flush_endpoint_address() - flush endpoint
 * @param function
 * @param endpoint_index
 */
int usbd_endpoint_urb_num(struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;
        if (!(function && endpoint_map)) return -1;
        if (!(endpoint = endpoint_map[endpoint_index].endpoint)) return -1;
        return ((endpoint->rdy).total);
}



/*!
 * usbd_get_device_state() - return device state
 * @param function
 * @return current device state
 */
usbd_device_state_t usbd_get_device_state(struct usbd_function_instance *function)
{
        return (function && function->bus) ? function->bus->device_state : STATE_UNKNOWN;
}

/*!
 * usbd_get_device_status() - return device status
 * @param function
 * @return current device status
 */
usbd_device_status_t usbd_get_device_status(struct usbd_function_instance *function)
{
        return (function && function->bus) ?  function->bus->status : USBD_UNKNOWN;
}

OTG_EXPORT_SYMBOL(usbd_first_ready_urb);
OTG_EXPORT_SYMBOL(usbd_first_finished_urb_detached);
OTG_EXPORT_SYMBOL(usbd_get_device_state);
OTG_EXPORT_SYMBOL(usbd_get_device_status);
OTG_EXPORT_SYMBOL(usbd_flush_endpoint);
OTG_EXPORT_SYMBOL(usbd_flush_endpoint_index);
OTG_EXPORT_SYMBOL(usbd_endpoint_urb_num);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Endpoint I/O:
 *      usbd_alloc_urb()
 *      usbd_start_in_urb()
 *      usbd_start_out_urb()
 *      usbd_cancel_urb()
 *
 */

/*!
 * usbd_alloc_urb() - allocate an URB appropriate for specified endpoint
 *
 * Allocate an urb structure. The usb device urb structure is used to
 * contain all data associated with a transfer, including a setup packet for
 * control transfers.
 *
 * @param function
 * @param endpoint_index
 * @param length
 * @param notify
 * @return urb
 */
struct usbd_urb *usbd_alloc_urb (struct usbd_function_instance *function, int endpoint_index,
                int length, usbd_urb_notification *notify)
{
        struct usbd_urb *urb = NULL;
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;
        int hs = function->bus->high_speed;


        RETURN_NULL_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));

        THROW_IF (!(urb = (struct usbd_urb *)CKMALLOC (sizeof (struct usbd_urb))), error);
        urb->endpoint = endpoint;
        urb->endpoint_index = endpoint_index;
        urb->wMaxPacketSize = endpoint->wMaxPacketSize[hs];
        urb->bus = function->bus;
        urb->function_instance = function;
        urb->link.prev = urb->link.next = &urb->link;
        urb->notify = notify;
        urb->dma_addr = (dma_addr_t)NULL;
        urb->alloc_length = urb->actual_length = urb->buffer_length = 0;

        if (length) {

                urb->buffer_length = length;

                /* For receive we always overallocate to ensure that receiving another
                 * full sized packet when we are only expecting a short packet will
                 * not overflow the buffer. Endpoint zero alloc's don't specify direction
                 * so always overallocate.
                 */
                UNLESS (endpoint->bEndpointAddress[hs] && (endpoint->bEndpointAddress[hs] & USB_ENDPOINT_DIR_MASK))
                        length = ((length / urb->wMaxPacketSize) + 3) * urb->wMaxPacketSize;

                THROW_IF(!(urb->buffer = (u8 *)KMALLOC (length)), error);
                urb->alloc_length = length;
        }

        if (TRACE_VERBOSE)
                TRACE_MSG7(USBD, "[%2x] urb:%x endpoint_index: %d "
                                "length: %d wMaxPacketSize: %d buffer_length: %d alloc_length: %d",
                                endpoint->bEndpointAddress[hs],
                                urb, endpoint_index, length, urb->wMaxPacketSize, urb->buffer_length, urb->alloc_length);

        CATCH(error) {

                #if defined(OTG_LINUX)
                #endif /* defined(OTG_LINUX) */
                #if defined(OTG_WINCE)
                DEBUGMSG(ZONE_INIT, (_T("usbd_alloc_urb: FAILED\n")));
                #endif /* defined(OTG_LINUX) */
                usbd_free_urb(urb);
                urb = NULL;
                TRACE_MSG0(USBD, "CATCH a error\n");
        }
        return urb;
}

/*!
 * usbd_halt_endpoint() -
 *
 * @param function function that owns endpoint
 * @param endpoint_index endpoint number
 * @return non-zero if endpoint is halted.
 */
int usbd_halt_endpoint (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;
        int hs = function->bus->high_speed;

        RETURN_ZERO_UNLESS(function && endpoint_map);
        RETURN_ZERO_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));

        return function->bus->driver->bops->halt_endpoint (function->bus, endpoint->bEndpointAddress[hs], 1);
}


/*!
 * usbd_alloc_urb_ep0() - allocate an urb for endpoint zero
 * @param function
 * @param length
 * @param callback
 * @return urb
 */
struct usbd_urb *usbd_alloc_urb_ep0 (struct usbd_function_instance *function, int length,
                int (*callback) (struct usbd_urb *, int))
{
        return usbd_alloc_urb((struct usbd_function_instance *)function->bus->ep0, 0, length, callback);
}

/*!
 * usbd_free_urb() - deallocate an URB and associated buffer
 *
 * Deallocate an urb structure and associated data.
 * @param urb
 */
void usbd_free_urb (struct usbd_urb *urb)
{
#if 0
        RETURN_IF (!urb);
        if (urb->buffer) LKFREE ((void *) urb->buffer);
        LKFREE (urb);
#else
        RETURN_UNLESS(urb);
        RETURN_IF(urb == urb->bus->ep0_urb);
        if (urb->buffer) LKFREE(urb->buffer);
        LKFREE (urb);
#endif

}
/*! usbd_start_in_urb - called to send urb
 * @param urb -pointer of usbd_urb to send
 * @return send result or -EAGAIN on error
 */
int usbd_start_in_urb (struct usbd_urb *urb)
{
        int hs = urb->bus->high_speed;
        struct usbd_bus_instance *bus = urb->bus;
        struct usbd_endpoint_instance *endpoint= urb->endpoint;

        if (TRACE_VERBOSE)
                TRACE_MSG6(USBD, "[%2x] urb: %x index: %d bus: %x status: %d length: %d",
                                urb->endpoint->bEndpointAddress[hs], urb, urb->endpoint_index, urb->bus, urb->bus->status,
                                urb->actual_length);

        if (urb->endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)) {
                urb->status = USBD_URB_STALLED;
                TRACE_MSG1(USBD, "[%2x] USBD_URB_STALLED", urb->endpoint->bEndpointAddress[hs]);
                return -EAGAIN;
        }
        if (urb->endpoint->bEndpointAddress[hs] && (USBD_OK != urb->bus->status)) {
                urb->status = USBD_URB_NOT_READY;
                TRACE_MSG1(USBD, "[%2x] USBD_URB_NOT_READY", urb->endpoint->bEndpointAddress[hs]);
                return -EAGAIN;
        }
        //TRACE_MSG2(USBD, "endpoint: %02x length: %d", urb->endpoint->bEndpointAddress[hs], urb->actual_length);
        urb->status = USBD_URB_QUEUED;
        urb->flags |= USBD_URB_IN;
        urb->irq_flags |= USBD_URB_READY;

        /* N.B. Race condition possible after this, once appended to the ready queue it
         * is possible for an interrupt handler to start using *and* complete this urb
         * so it cannot be referenced after this point.
         */
        urb_append (&(urb->endpoint->rdy), urb);

        /* Use saved values for bus and endpoint to call appropriate start out function
         */
        return bus->driver->bops->start_endpoint_in(bus, endpoint);
}

/*!
 * usbd_start_out_urb() - recycle a received urb
 *
 * Used by a USB Function interface driver to recycle an urb.
 *
 * @param urb to process
 * @return non-zero if error
 */
int usbd_start_out_urb (struct usbd_urb *urb)
{
        int hs = urb->bus->high_speed;
        struct usbd_bus_instance *bus = urb->bus;
        struct usbd_endpoint_instance *endpoint= urb->endpoint;

        if (TRACE_VERBOSE)
                TRACE_MSG6(USBD, "[%2x] urb: %x index: %d bus: %x status: %d length: %d",
                                urb->endpoint->bEndpointAddress[hs],
                                urb, urb->endpoint_index, urb->bus, urb->bus->status,
                                urb->buffer_length);
        if (urb->endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)) {
                urb->status = USBD_URB_STALLED;
                TRACE_MSG1(USBD, "[%2x] USBD_URB_STALLED", urb->endpoint->bEndpointAddress[hs]);
                return -EAGAIN;
        }
        if (urb->endpoint->bEndpointAddress[hs] && (USBD_OK != urb->bus->status)) {
                urb->status = USBD_URB_NOT_READY;
                TRACE_MSG1(USBD, "[%2x] USBD_URB_NOT_READY", urb->endpoint->bEndpointAddress[hs]);
                return -EAGAIN;
        }

        urb->actual_length = 0;
        urb->status = USBD_URB_QUEUED;
        urb->flags |= USBD_URB_OUT;
        urb->irq_flags = USBD_URB_READY;

        /* N.B. Race condition possible after this, once appended to the ready queue it
         * is possible for an interrupt handler to start using *and* complete this urb
         * so it cannot be referenced after this point.
         */
        urb_append (&(urb->endpoint->rdy), urb);

        /* Use saved values for bus and endpoint to call appropriate start out function
         */
        return bus->driver->bops->start_endpoint_out(bus, endpoint);
}

/*!
 * usbd_cancel_urb() - cancel an urb being sent
 *
 * @param urb to process
 * @return non-zero if error
 */
int usbd_cancel_urb (struct usbd_urb *urb)
{
        RETURN_ZERO_UNLESS (urb && urb->bus && urb->bus->driver && urb->bus->driver->bops);
        return urb->bus->driver->bops->cancel_urb_irq (urb);
}


/*!
 * usbd_endpoint_halted() - return halt status
 *
 * @param function function that owns endpoint
 * @param endpoint_index endpoint number
 * @return non-zero if endpoint is halted.
 */
int usbd_endpoint_halted (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_instance *endpoint;
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        RETURN_ZERO_UNLESS(function && endpoint_map);
        RETURN_ZERO_UNLESS((endpoint = endpoint_map[endpoint_index].endpoint));
        return function->bus->driver->bops->endpoint_halted (function->bus,
                        endpoint->bEndpointAddress[function->bus->high_speed]);
}


OTG_EXPORT_SYMBOL(usbd_alloc_urb);
OTG_EXPORT_SYMBOL(usbd_alloc_urb_ep0);
OTG_EXPORT_SYMBOL(usbd_free_urb);

OTG_EXPORT_SYMBOL(usbd_start_in_urb);
OTG_EXPORT_SYMBOL(usbd_start_out_urb);
OTG_EXPORT_SYMBOL(usbd_cancel_urb);
OTG_EXPORT_SYMBOL(usbd_halt_endpoint);

/* ********************************************************************************************* */

/*!
 * usbd_function_get_privdata() - get private data pointer
 * @param function
 * @return void * pointer to private data
 */
void *usbd_function_get_privdata(struct usbd_function_instance *function)
{
        return(function->privdata);
}

/*!
 * usbd_function_set_privdata() - set private data structure in function
 * @param function
 * @param privdata
 */
void usbd_function_set_privdata(struct usbd_function_instance *function, void *privdata)
{
        function->privdata = privdata;
}

/*!
 * usbd_endpoint_transferSize() - get transferSize for endpoint
 * @param function
 * @param endpoint_index
 * @param hs highspeed flag
 * @return transfer size
 */
int usbd_endpoint_transferSize(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        RETURN_ZERO_UNLESS(function && endpoint_map);
        return endpoint_map[endpoint_index].transferSize[hs];
}

/*!
 * usbd_endpoint_update() - update endpoint address and size
 * @param function
 * @param endpoint_index
 * @param endpoint descriptor
 * @param hs high speed flag
 */
void usbd_endpoint_update(struct usbd_function_instance *function, int endpoint_index,
                struct usbd_endpoint_descriptor *endpoint, int hs)
{
        endpoint->bEndpointAddress = usbd_endpoint_bEndpointAddress(function, endpoint_index, hs);
        endpoint->wMaxPacketSize = usbd_endpoint_wMaxPacketSize(function, endpoint_index, hs);
}

/*!
 * usbd_otg_bmattributes() - return attributes
 * @param function
 * @return endpoint attributes
 */
int usbd_otg_bmattributes(struct usbd_function_instance *function)
{
        // XXX TODO - per function modifications for composite devices
        return function->bus->bmAttributes;
}



/*!
 * usbd_write_info_message() -
 *
 * Send a message to the otg management application.
 * @param function - function instance pointer
 * @param msg
 */
void usbd_write_info_message(struct usbd_function_instance *function, char *msg)
{
        struct usbd_bus_instance *bus = function->bus;
        RETURN_UNLESS(bus);
        otg_write_info_message(bus->privdata, msg);
}

OTG_EXPORT_SYMBOL(usbd_function_get_privdata);
OTG_EXPORT_SYMBOL(usbd_function_set_privdata);
OTG_EXPORT_SYMBOL(usbd_endpoint_transferSize);
OTG_EXPORT_SYMBOL(usbd_otg_bmattributes);
OTG_EXPORT_SYMBOL(usbd_endpoint_update);
OTG_EXPORT_SYMBOL(usbd_write_info_message);
