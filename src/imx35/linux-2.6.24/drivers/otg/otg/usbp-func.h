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
 * otg/otg/usbp-func.h - USB Device Function Driver Interface
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/usbp-func.h|20070220211522|09278
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */

/*!
 * @file otg/otg/usbp-func.h
 * @brief Function Driver related structures and definitions.
 *
 * This file contains the USB Function Driver and USB Interface Driver
 * definitions.
 *
 * The USBOTG support allows for implementation of composite devices.
 *
 * From USB 2.0, C.f. 5.2.3:
 *
 *      A Device that has multiple interfaces controlled independantly of
 *      each other is referred to as a composite device. A composite device
 *      has only a single device.
 *
 * In this implemenation the function portion of the device is split into
 * two types of drivers:
 *
 *      - USB Interface Driver
 *      - USB Function Driver
 *
 *
 * USB Interface Driver
 *
 * An USB Interface Driver specifies the Interface related descriptors and
 * implements the data handling processes for each of the data endpoints
 * associated with the interface (or interfaces) required for the function.
 * It typically will also implement the upper edge interface to the OS.
 *
 * The USB Interface Driver will also handle non-standard device requests
 * and feature requests that are for the interface or an endpoint associated
 * with one of the interfaces associated with the driver.
 *
 * Each interface implemented by an USB Interface Driver will have a
 * interface instance that stores a pointer to the parent USB Function
 * Driver and has an array of pointers to endpoint instances for the data
 * endpoints associated with the interface.
 *
 *
 * USB Function Driver
 *
 * A USB Function Driver specifies the device and configuration descriptors.
 * A configuration descriptor is assembled from the interface descriptors
 * from the USB Interface Driver (or drivers) that it is making available to
 * the USB Host.
 *
 * The USB Function Driver handles non-standard device requests and feature
 * requests for the device.
 *
 * Each function driver has a function instance that maintains an array
 * of pointers to configuration instances and a pointer to the device
 * descriptor.
 *
 * Each configuration instance maintains an array of pointers to the
 * interface instances for that configuration and a pointer to the
 * complete configuration descriptor.
 *
 * @ingroup USBDAPI
 */

#ifndef USBP_FUNC_H
#define USBP_FUNC_H

/*!
 * This file contains the USB Device Core Common definitions. It also contains definitions for
 * functions that are commonly used by both Function Drivers and Bus Interface Drivers.
 *
 * Specifically:
 *
 *      o public functions exported by the USB Core layer
 *
 *      o common structures and definitions
 *
 */


/*!
 * @var typedef enum usbd_function_types usbd_function_types_t
 *
 *  @brief  USB Function Driver types
 */
typedef enum usbd_function_types {
        function_ep0,                // combined driver
        function_simple,                // combined driver
        function_interface,             // supports interface
        function_class,                 // supports class
        function_composite,             // supports configuration
} usbd_function_types_t;


/*!@struct   usbd_function_operations  usbp-func.h  "otg/usbp-func.h"
 * USB Function Driver structures
 *
 * Descriptors:
 *   struct usbd_endpoint_description
 *   struct usbd_interface_description
 *   struct usbd_configuration_description
 *
 * Driver description:
 *   struct usbd_function_driver
 *   struct usbd_function_operations
 *
 */

struct usbd_function_operations {

        int (*function_enable) (struct usbd_function_instance *);
        void (*function_disable) (struct usbd_function_instance *);

        /*
         * All of the following may be called from an interrupt context
         */
        void (*event_handler) (struct usbd_function_instance *, usbd_device_event_t, int);
        int (*device_request) (struct usbd_function_instance *, struct usbd_device_request *);

        // XXX should these return anything?
        int (*set_configuration) (struct usbd_function_instance *, int configuration);
        int (*set_interface) (struct usbd_function_instance *, int wIndex, int altsetting);
        int (*suspended) (struct usbd_function_instance *);
        int (*resumed) (struct usbd_function_instance *);
        int (*reset) (struct usbd_function_instance *);

        void * (*get_descriptor)(struct usbd_function_instance *, int descriptor_type, int descriptor_index);
        int * (*set_descriptor)(struct usbd_function_instance *, int descriptor_type, int descriptor_index, void *);

        void (*endpoint_cleared)(struct usbd_function_instance*, int wIndex);

};


/*!@struct   xusbd_alternate_instance  usbp-func.h  "otg/usbp-func.h"
 * function driver definitions
 */
struct xusbd_alternate_instance {
        struct usbd_interface_descriptor *interface_descriptor;
        int                             classes;
        struct usbd_generic_class_descriptor    **class_list;
        int                             endpoints;
};


/*!
 * @struct usbd_alternate_description  usbp-func.h "otg/usbp-func.h"
 *
 *  @brief  usb device description structures
 */

struct usbd_alternate_description {
        //struct usbd_interface_descriptor *interface_descriptor;
        char                            *iInterface;
        u8                              bInterfaceClass;
        u8                              bInterfaceSubClass;
        u8                              bInterfaceProtocol;

        // list of CDC class descriptions for this alternate interface
        u8                              classes;
        struct usbd_generic_class_descriptor    **class_list;

        // list of endpoint descriptions for this alternate interface
        u8                              endpoints;

        // list of indexes into endpoint request map for each endpoint descriptor
        u8                              *endpoint_index;

                                                        // XXX This should be here
        u8                              new_endpointsRequested;
        struct usbd_endpoint_request    *new_requestedEndpoints;
};

/*! create a type for usbd_generic_class_defscriptor
 * @brief typedef struct usbd_generic_class_descriptor usbd_calss_descriptor_t
 *
 */

typedef struct usbd_generic_class_descriptor usbd_class_descriptor_t;

/*!@var typedef struct usbd_endpoint_descriptor      usbd_endpoint_descriptor_t;
 *
 * @brief create a type for usbd_endpoint_descriptor
 */
typedef struct usbd_endpoint_descriptor      usbd_endpoint_descriptor_t;

/*! @struct usbd_interface_description usbp-func.h  "otg/usbp-func.h"
 */

struct usbd_interface_description {
        // list of alternate interface descriptions for this interface
        u8                              alternates;        /*< number of alternates */
        struct usbd_alternate_description *alternate_list; /*< alternate description list */
};

/*! @struct usbd_configuration_description usbp-func.h "otg/usbp-func.h"*/

struct usbd_configuration_description {
        char                            *iConfiguration; /*<  configuration index string */
        u8 bmAttributes;                                 /*<  bmAttributes  */
        u8 bMaxPower;                                    /*< bMaxPower - maximum power allowed */
};

/*! @struct usbd_device_description usbp-func.h "otg/usbp-func.h"
 */

struct usbd_device_description {
        //struct usbd_device_descriptor   *device_descriptor;
        //struct usbd_otg_descriptor      *otg_descriptor;

        u16                             idVendor;
        u16                             idProduct;
        u16                             bcdDevice;
        u8                              bDeviceClass;
        u8                              bDeviceSubClass;
        u8                              bDeviceProtocol;

        u8                              bMaxPacketSize0;

        u8                              *iManufacturer;
        u8                              *iProduct;
        u8                              *iSerialNumber;
};
/*! @struct usbd_instances_instance usbp-func.h "otg/usbp-func.h"*/

struct usbd_interfaces_instance {
        u8                              alternates;
        struct usbd_alternate_instance  *alternates_instance_array;
};

/*! @struct usbd_function_driver usbp-func.h  "otg/usbp-func.h"
 *
 *  @brief Function Driver data structure * Function driver and its configuration descriptors.
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about the function driver for the usb-device layer
 * to use the function drivers configuration data and to configure this
 * function driver an active configuration.
 *
 * Note that each function driver registers itself on a speculative basis.
 * Whether a function driver is actually configured will depend on the USB
 * HOST selecting one of the function drivers configurations.
 *
 * This may be done multiple times WRT to either a single bus interface
 * instance or WRT to multiple bus interface instances. In other words a
 * multiple configurations may be selected for a specific bus interface. Or
 * the same configuration may be selected for multiple bus interfaces.
 *
 */
#define FUNCTION_REGISTERED             0x1                     // set in flags if function is registered
#define FUNCTION_ENABLED                0x2                     // set in flags if function is enabled
struct usbd_function_driver {
        struct otg_list_node            drivers;              // linked list (was drivers in usbd_function_driver)
        const char                      *name;
        struct usbd_function_operations *fops;  // functions
        usbd_function_types_t           function_type;
        u32                             flags;
        void                            *privdata;
        const char                      **usb_strings;
        u8                              *usb_string_indexes;
};

/*!@struct usbd_function_instance  usbp-func.h "otg/usbp-func.h"
 *
 *  @brief function configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_function_instance {
        const char                      *name;
        struct usbd_bus_instance        *bus;
        usbd_function_types_t           function_type;
        struct usbd_function_driver     *function_driver;
        void                            *privdata;              // private data for the function

        //int                             usbd_maxstrings;
        //struct usbd_string_descriptor   **usb_strings;
};

/*! @struct usbd_simple_driver usbp-func.h "otg/usbp-func.h"
 *
 *  @brief Function Driver data structure *
 * Function driver and its configuration descriptors.
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about the function driver for the usb-device layer
 * to use the function drivers configuration data and to configure this
 * function driver an active configuration.
 *
 * Note that each function driver registers itself on a speculative basis.
 * Whether a function driver is actually configured will depend on the USB
 * HOST selecting one of the function drivers configurations.
 *
 * This may be done multiple times WRT to either a single bus interface
 * instance or WRT to multiple bus interface instances. In other words a
 * multiple configurations may be selected for a specific bus interface. Or
 * the same configuration may be selected for multiple bus interfaces.
 *
 */
struct usbd_simple_driver {

        struct usbd_function_driver     driver;

        // device & configuration descriptions
        struct usbd_device_description  *device_description;
        struct usbd_configuration_description *configuration_description;
        int                             bNumConfigurations;

        u8                              interfaces;         // XXX should be interfaces
        struct usbd_interface_description *interface_list;

        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        // constructed descriptors
        //struct usbd_device_descriptor   *device_descriptor;

        u8                              iManufacturer;
        u8                              iProduct;
        u8                              iSerialNumber;


        //struct usbd_otg_descriptor       *otg_descriptor;
        //struct usbd_configuration_instance *configuration_instance_array;

};

/*! @struct usbd_simple_instance  usbp-func.h "otg/usbp-func.h"
 *
 * @brief Simple configuration structure *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_simple_instance {
                                                                // common to all types
        struct usbd_function_instance   function;

                                                                // composite driver only
        int                             configuration;          // saved value from set configuration

        int                             interface_functions;    // number of names in interfaces_list
        int                             interfaces;             // accumulated total of all bNumInterfaces


        u8                              *altsettings;            // array[0..interfaces-1] of alternate settings

        struct usbd_endpoint_map        *endpoint_map_array;

        int                             configuration_size;
        struct usbd_configuration_descriptor  *configuration_descriptor[2];
        u8                              ConfigurationValue;     // current set configuration (zero is default)

};

/*!@struct usbd_class_instance usbp-func.h "otg/usbp-func.h"
 *  @brief class configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_class_instance {
        struct usbd_function_instance   function;

};

/*!@struct usbd_class_driver  usbp-func.h "otg/usbp-func.h" */

struct usbd_class_driver {
        struct usbd_function_driver     driver;
};



/*!   @struct usbd_interface_driver usbp-func.h "otg/usbp-func.h"
 *    @brief  interface configuration structure
 *
 *    This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */

struct usbd_interface_driver {
        struct usbd_function_driver     driver;

        u8                              interfaces;
        struct usbd_interface_description *interface_list;

                                                        // XXX This should moved to interface description
        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        u8 bFunctionClass;
        u8 bFunctionSubClass;
        u8 bFunctionProtocol;
        char *iFunction;
};

/*!@struct usbd_interface_instance   usbp-func.h "otg/usbp-func.h"*/

struct usbd_interface_instance {
        struct usbd_function_instance   function;

        int                             wIndex;                 // allocated interface number
        int                             lIndex;                 // logical interface number
        int                             altsetting;

        int                             endpoints;
        struct usbd_endpoint_map        *endpoint_map_array;
};

/*! @struct usbd_composite_driver usbp-func.h "otg/usbp-func.h"
 *   @brief  Composite configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_composite_driver {
        struct usbd_function_driver     driver;

        // device & configuration descriptions
        struct usbd_device_description  *device_description;
        struct usbd_configuration_description *configuration_description;
        int                             bNumConfigurations;

        const char                      **interface_function_names;      // list of interface function names
        const char                      *class_name;
        const char                      *windows_os;                    // windows 0xee string

        u8                              iManufacturer;
        u8                              iProduct;
        u8                              iSerialNumber;

        //struct usbd_otg_descriptor       *otg_descriptor;             // XXX unused

};

/*! @struct usbd_composite_instance   usbp-func.h "otg/usbp-func.h"*/
struct usbd_composite_instance {
                                                                // common to all types
        struct usbd_function_instance   function;
                                                                // composite driver only
        int                             configuration;          // saved value from set configuration

        int                             interface_functions;    // number of interface functions available
        int                             interfaces;             // accumulated total of all bNumInterfaces

        struct usbd_interface_description *class_list;
        struct usbd_class_instance      *class_instance;

        struct usbd_interface_instance   *interfaces_array;     // array of interface instances, one per interface function
        struct usbd_interface_description *interface_list;

        int                             endpoints;
        struct usbd_endpoint_map        *endpoint_map_array;
        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        int                             configuration_size;
        struct usbd_configuration_descriptor  *configuration_descriptor[2];
        u8                              ConfigurationValue;     // current set configuration (zero is default)
};
/*
union usbd_instance_union {
        struct usbd_function_instance *function_instance;
        struct usbd_simple_instance *simple_instance;
        struct usbd_composite_instance *composite_instance;
};
*/
/*!
 * @name Function Driver Registration
 *
 * Called by function drivers to register themselves when loaded
 * or de-register when unloading.
 * @{
 */
int usbd_register_simple_function (struct usbd_simple_driver *, const char *, void *);

int usbd_register_interface_function (struct usbd_interface_driver *, const char *, void *);
int usbd_register_class_function (struct usbd_class_driver *function_driver, const char*, void *);
int usbd_register_composite_function (struct usbd_composite_driver *function_driver, const char*, const char*, const char **, void *);

void usbd_deregister_simple_function (struct usbd_simple_driver *);
void usbd_deregister_interface_function (struct usbd_interface_driver *);
void usbd_deregister_class_function (struct usbd_class_driver *);
void usbd_deregister_composite_function (struct usbd_composite_driver *);

struct usbd_function_driver *usbd_find_function (const char *name);
struct usbd_interface_driver *usbd_find_interface_function(const char *name);
struct usbd_class_driver *usbd_find_class_function(const char *name);
struct usbd_composite_driver *usbd_find_composite_function(const char *name);

//struct usbd_function_instance * usbd_alloc_function (struct usbd_function_driver *, char *, void *);
//struct usbd_function_instance * usbd_alloc_interface_function (struct usbd_function_driver *, char *, void *);
//struct usbd_function_instance * usbd_alloc_class_function (struct usbd_function_driver *function_driver, char*, void *);
//struct usbd_function_instance * usbd_alloc_composite_function (struct usbd_function_driver *function_driver, char*, char*,
//                char **, void *);

//void usbd_dealloc_function (struct usbd_function_instance *);
//void usbd_dealloc_interface (struct usbd_function_instance *);
//void usbd_dealloc_class (struct usbd_function_instance *);

/*! @} */


/*!
 * @name String Descriptor database
 *
 * @{
 */

struct usbd_string_descriptor *usbd_get_string_descriptor (struct usbd_function_instance *, u8);
u8 usbd_realloc_string (struct usbd_function_instance *, u8, const char *);
u8 usbd_alloc_string (struct usbd_function_instance *, const char *);
//void usbd_free_string_descriptor(struct usbd_function_instance *, u8 );



//extern struct usbd_string_descriptor **usb_strings;
//void usbd_free_descriptor_strings (struct usbd_descriptor *);

/*! @} */

/*!
 * @name LANGID's
 *
 * @{
 */
#define LANGID_ENGLISH          "\011"
#define LANGID_US_ENGLISH       "\004"
#define LANGIDs  LANGID_US_ENGLISH LANGID_ENGLISH
/*! @} */


/*!
 * @name Well Known string indices
 *
 * Used by function drivers to set certain strings
 * @{
 */
#define STRINDEX_LANGID     0
#define STRINDEX_IPADDR     1
#define STRINDEX_PRODUCT    2
/*! @} */




/*!
 * @name Device Information
 *
 * @{
 */
BOOL usbd_high_speed(struct usbd_function_instance *instance);
int usbd_bmaxpower(struct usbd_function_instance *instance);
int usbd_endpoint_wMaxPacketSize(struct usbd_function_instance *instance, int index, BOOL hs);
int usbd_endpoint_zero_wMaxPacketSize(struct usbd_function_instance *instance, BOOL hs);
int usbd_endpoint_bEndpointAddress(struct usbd_function_instance *instance, int index, BOOL hs);
int usbd_get_bMaxPower(struct usbd_function_instance *);
/*! @} */


/*!
 * @name Device Control
 *
 * @{
 */
usbd_device_state_t usbd_get_device_state(struct usbd_function_instance *);
usbd_device_status_t usbd_get_device_status(struct usbd_function_instance *);
#if 0
int usbd_framenum(struct usbd_function_instance *);
otg_tick_t usbd_ticks(struct usbd_function_instance *);
otg_tick_t usbd_elapsed(struct usbd_function_instance *, otg_tick_t *, otg_tick_t *);
#endif
/*! @} */

/*!
 * @name Endpoint I/O
 *
 * @{
 */

//typedef int usbd_urb_notification(struct usbd_urb *urb, int rc);
struct usbd_urb *usbd_alloc_urb (struct usbd_function_instance *, int endpoint_index, int length, usbd_urb_notification *notify);
struct usbd_urb *usbd_alloc_urb_ep0 (struct usbd_function_instance *, int length, usbd_urb_notification *notify);
void usbd_free_urb (struct usbd_urb *urb);
int usbd_start_in_urb (struct usbd_urb *urb);
int usbd_start_out_urb (struct usbd_urb *);
int usbd_cancel_urb(struct usbd_urb *);
int usbd_halt_endpoint (struct usbd_function_instance *function, int endpoint_index);
int usbd_endpoint_halted (struct usbd_function_instance *function, int endpoint);

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
static void INLINE usbd_unlink_urb(struct usbd_urb *urb)
{
        urb_link *ul = &urb->link;
        ul->next->prev = ul->prev;
        ul->prev->next = ul->next;
        ul->prev = ul->next = ul;
}



/*! @} */


/*!
 * @name endpoint information
 *
 * Used by function drivers to get specific endpoint information
 * @{
 */

int usbd_endpoint_transferSize(struct usbd_function_instance *, int, int);
int usbd_endpoint_interface(struct usbd_function_instance *, int);
int usbd_interface_AltSetting(struct usbd_function_instance *, int);
int usbd_ConfigurationValue(struct usbd_function_instance *);
void usbd_endpoint_update(struct usbd_function_instance *, int , struct usbd_endpoint_descriptor *, int);
/*! @} */

//int usbd_remote_wakeup_enabled(struct usbd_function_instance *);

/*!
 * @name device information
 *
 * Used by function drivers to get device feature information
 * @{
 */
int usbd_otg_bmattributes(struct usbd_function_instance *);
/*! @} */

/*!
 * @name usbd_feature_enabled - return non-zero if specified feature has been enabled
 * @{
 */
int usbd_feature_enabled(struct usbd_function_instance *function, int f);
/*! @} */

/* DEPRECATED */
/*!
 * @name Access to function privdata (DEPRECATED)
 * @{
 */
void *usbd_function_get_privdata(struct usbd_function_instance *function);
void usbd_function_set_privdata(struct usbd_function_instance *function, void *privdata);
void usbd_flush_endpoint_index (struct usbd_function_instance *, int );
int usbd_endpoint_urb_num (struct usbd_function_instance *function, int endpoint_index);
int usbd_get_descriptor (struct usbd_function_instance *function, u8 *buffer, int max, int descriptor_type, int index);
/*! @} */

#endif /* USBP_FUNC_H */
