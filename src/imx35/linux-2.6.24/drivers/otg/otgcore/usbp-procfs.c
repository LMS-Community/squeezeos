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
 * otg/otgcore/usbp-procfs.c - USB Device Core Layer
 * @(#) balden@belcarra.com|otg/otgcore/usbp-procfs.c|20070327230504|28330
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
 * @file otg/otgcore/usbp-procfs.c
 * @brief Implements /proc/usbd-functions, which displays descriptors for the current selected function.
 *
 *
 * @ingroup USBDCORE
 */

#include <otg/otg-compat.h>

//#ifdef LINUX24
//EXPORT_NO_SYMBOLS;
//#endif

#include <otg/usbp-chap9.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-trace.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-pcd.h>


#define MAX_INTERFACES 2

static struct usbd_bus_instance *procfs_usbd_bus_instance;
extern struct otg_list_node usbd_simple_drivers;            // list of all registered configuration function modules
extern struct otg_list_node usbd_interface_drivers;         // list of all registered interface function modules
extern struct otg_list_node usbd_class_drivers;             // list of all registered composite function modules
extern struct otg_list_node usbd_composite_drivers;         // list of all registered composite function modules


/*!
 * list_function() - list registered usbd_function_driver from start information
 *
 * @param usbd_function_drivers - usbd function drivers table
 * @param cp - buffer to store driver information
 * @param start - start position of list
 * @return information length
 */
int list_function_composite(struct otg_list_node *usbd_function_drivers, char *cp, int start)
{
        int total = 0;
        int count = 0;

        struct otg_list_node *lhd = NULL;


        LIST_FOR_EACH (lhd, usbd_function_drivers) {
                struct usbd_composite_driver *composite_driver =
                        (struct usbd_composite_driver *) LIST_ENTRY (lhd, struct usbd_function_driver, drivers);

                const char **names = composite_driver->interface_function_names;

                CONTINUE_IF(count++ < start);

                total += sprintf(cp + total, "\t%20s", composite_driver->driver.name);
                total += sprintf(cp + total, " %04x/%04x %02x/%02x/%02x ",
                                composite_driver->device_description->idVendor,
                                composite_driver->device_description->idProduct,
                                composite_driver->device_description->bDeviceClass,
                                composite_driver->device_description->bDeviceSubClass,
                                composite_driver->device_description->bDeviceProtocol
                                );
                while ( *names ) {
                        total += sprintf(cp + total, "%s", *names++);
                        if (*names)
                                total += sprintf(cp + total, ":");
                }


                if (composite_driver ->class_name)
                        total += sprintf(cp + total, "; %s", composite_driver->class_name);

                total += sprintf(cp + total, "\n");
                break;
        }
        return total;
}

/*!
 * list_function() - list driver's name from usbd_function_drivers table
 */
int list_function(struct otg_list_node *usbd_function_drivers, char *cp, char *msg)
{
        int total = sprintf(cp, "\n%s\n", msg);

        struct otg_list_node *lhd = NULL;
        LIST_FOR_EACH (lhd, usbd_function_drivers) {
                struct usbd_function_driver *function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                total += sprintf(cp + total, "\t%s\n", function_driver->name);
        }
        return total;
}


/* Proc Filesystem *************************************************************************** */
/*!
 * dohexdigit - change a value to hexdecimal char
 * @param cp - buffer to store transformed char
 * @param val - value to process
 *
 */
static void dohexdigit (char *cp, unsigned char val)
{
        if (val < 0xa) {
                *cp = val + '0';
        } else if ((val >= 0x0a) && (val <= 0x0f)) {
                *cp = val - 0x0a + 'a';
        }
}

/*!
 * dohex - using translate a unsigned char value to hex notation
 * @param cp - buffer to save hex notation
 * @param val - value to translate
 *
 */
static void dohexval (char *cp, unsigned char val)
{
        dohexdigit (cp++, val >> 4);
        dohexdigit (cp++, val & 0xf);
}

/*!
 * dump_descriptor - create a descriptor for string sp
 * @param buf - buffer to store descriptor
 * @param sp -
 * @return created descriptor length
 */
static int dump_descriptor (char *buf, char *sp)
{
        int num;
        int len = 0;

        RETURN_ZERO_UNLESS(sp);

        num = *sp;

        while (sp && num--) {
                dohexval (buf, *sp++);
                buf += 2;
                *buf++ = ' ';
                len += 3;
        }
        len++;
        *buf = '\n';
        return len;
}

/*!
 * dump_string_descriptor  - dispaly string descriptor information
 * @param function_instance - function instance pointer
 * @param buf - buffer to save information
 * @param i -
 * @param name -
 * @return information buffer length
 */
static int dump_string_descriptor (struct usbd_function_instance *function_instance, char *buf, int i, char *name)
{
        int k;
        int len = 0;

        struct usbd_string_descriptor *string_descriptor;


        RETURN_ZERO_UNLESS(i);

        string_descriptor = usbd_get_string_descriptor (function_instance, i);

        RETURN_ZERO_UNLESS (string_descriptor);

        len += sprintf((char *)buf+len, "  %-24s [%2d:%2d  ] ", name, i, string_descriptor->bLength);

        for (k = 0; k < (string_descriptor->bLength / 2) - 1; k++) {
                *(char *) (buf + len) = (char) string_descriptor->wData[k];
                len++;
        }
        len += sprintf ((char *) buf + len, "\n");

        return len;
}



/*! dump_device_descriptors - dump device descriptor
 * @param function_instance - usbd_function_instance pointer
 * @param buf - buffer for saving descriptor information
 * @param sp - device descriptor pointer
 * @return buffer length
 */
static int dump_device_descriptor(struct usbd_function_instance *function_instance, char *buf, char *sp)
{
        int total = 0;
        struct usbd_device_descriptor *device_descriptor = (struct usbd_device_descriptor *) sp;

        TRACE_MSG2(USBD, "buf: %x sp: %x", buf, sp);

        total = sprintf(buf + total,  "Device descriptor          [       ] ");
        total += dump_descriptor(buf + total, sp);

        total += sprintf(buf + total, "  bcdUSB                   [      2] %04x\n", device_descriptor->bcdUSB);
        total += sprintf(buf + total, "  bDevice[Class,Sub,Pro]   [      4] %02x %02x %02x\n",
                        device_descriptor->bDeviceClass, device_descriptor->bDeviceSubClass, device_descriptor->bDeviceProtocol);

        total += sprintf(buf + total, "  bMaxPacketSize0          [      7] %02x\n", device_descriptor->bMaxPacketSize0);

        total += sprintf(buf + total, "  idVendor                 [      8] %04x\n", device_descriptor->idVendor);
        total += sprintf(buf + total, "  idProduct                [     10] %04x\n", device_descriptor->idProduct);
        total += sprintf(buf + total, "  bcdDevice                [     12] %04x\n", device_descriptor->bcdDevice);

        total += sprintf(buf + total, "  bNumConfigurations       [     17] %02x\n", device_descriptor->bNumConfigurations);

        total += dump_string_descriptor(function_instance, buf + total, device_descriptor->iManufacturer, "iManufacturer");
        total += dump_string_descriptor(function_instance, buf + total, device_descriptor->iProduct, "iProduct");
        total += dump_string_descriptor(function_instance, buf + total, device_descriptor->iSerialNumber, "iSerialNumber");

        total += sprintf(buf + total, "\n");
        return total;
}

/*! dump_device_qualifier_descriptors - dump device descriptor
 * @param function_instance - usbd_function_instance pointer
 * @param buf - buffer for saving descriptor information
 * @param sp - device descriptor pointer
 * @return buffer length
 */
static int dump_device_qualifier_descriptor(struct usbd_function_instance *function_instance, char *buf, char *sp)
{
        int total = 0;
        struct usbd_device_qualifier_descriptor *device_qualifier_descriptor = (struct usbd_device_qualifier_descriptor *) sp;

        TRACE_MSG2(USBD, "buf: %x sp: %x", buf, sp);

        total = sprintf(buf + total,  "Device qualifier           [       ] ");
        total += dump_descriptor(buf + total, sp);

        total += sprintf(buf + total, "  bcdUSB                   [      2] %04x\n", device_qualifier_descriptor->bcdUSB);

        total += sprintf(buf + total, "  bDevice[Class,Sub,Pro]   [      4] %02x %02x %02x\n",
                        device_qualifier_descriptor->bDeviceClass,
                        device_qualifier_descriptor->bDeviceSubClass, device_qualifier_descriptor->bDeviceProtocol);

        total += sprintf(buf + total, "  bMaxPacketSize0          [      7] %02x\n",
                        device_qualifier_descriptor->bMaxPacketSize0);

        total += sprintf(buf + total, "  bNumConfigurations       [     17] %02x\n",
                        device_qualifier_descriptor->bNumConfigurations);

        total += sprintf(buf + total, "\n");
        return total;
}

/*! dump_config_descriptors - dump config descriptor information
 * @param function_instance -
 * @param buf
 * @param sp - configuartion descriptor pointer
 * @return information length
 */
static int dump_config_descriptor(struct usbd_function_instance *function_instance, char *buf, char *sp)
{
        struct usbd_configuration_descriptor *config = (struct usbd_configuration_descriptor *) sp;
        struct usbd_endpoint_descriptor *endpoint;
        struct usbd_interface_descriptor *interface;
        struct usbd_interface_association_descriptor *iad;

        int wTotalLength = le16_to_cpu(config->wTotalLength);
        int bConfigurationValue = config->bConfigurationValue;
        int interface_num;
        int class_num;
        int endpoint_num;
        int total;

        TRACE_MSG3(USBD, "buf: %x sp: %x length: %d", buf, sp, wTotalLength);

        interface_num = class_num = endpoint_num = 0;

        for (total = 0; wTotalLength > 0; ) {
                BREAK_UNLESS(sp[0]);
                switch (sp[1]) {
                case USB_DT_CONFIGURATION:
                case USB_DT_OTHER_SPEED_CONFIGURATION:
                        interface_num = class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "Configuration descriptor   [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_INTERFACE:
                        class_num = 0;
                        total += sprintf(buf + total, "\nInterface descriptor       [%d:%d:%d  ] ",
                                        bConfigurationValue, interface_num++, class_num);
                        break;
                case USB_DT_ENDPOINT:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "Endpoint descriptor        [%d:%d:%d:%d] ",
                                        bConfigurationValue, interface_num - 1, class_num, ++endpoint_num);
                        break;
                case USB_DT_OTG:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "OTG descriptor             [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_INTERFACE_ASSOCIATION:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "\nIAD descriptor             [%d:%d    ] ",
                                        bConfigurationValue, interface_num);
                        break;
                default:
                        endpoint_num = 0;
                        total += sprintf(buf + total, "  Class descriptor         [%d:%d:%d  ] ",
                                        bConfigurationValue, interface_num-1  , ++class_num);
                        break;
                }
                total += dump_descriptor(buf + total, sp);
                switch (sp[1]) {
                case USB_DT_CONFIGURATION:
                case USB_DT_OTHER_SPEED_CONFIGURATION:
                        config = (struct usbd_configuration_descriptor *)sp;
                        total += sprintf(buf + total, "  wTotalLength             [      4] %02x\n", config->wTotalLength);
                        total += sprintf(buf + total, "  bNumInterfaces           [      4] %02x\n", config->bNumInterfaces);
                        total += sprintf(buf + total, "  bConfigurationValue      [      5] %02x\n", config->bConfigurationValue);
                        total += dump_string_descriptor(function_instance, buf + total, config->iConfiguration, "iConfiguration");
                        total += sprintf(buf + total, "  bmAttributes             [      7] %02x%s%s\n",
                                        config->bmAttributes,
                                        config->bmAttributes & USB_BMATTRIBUTE_SELF_POWERED ? " Self-Powered" : "",
                                        config->bmAttributes & USB_BMATTRIBUTE_REMOTE_WAKEUP ? " Remote-Wakeup" : ""
                                        );
                        total += sprintf(buf + total, "  bMaxPower                [      8] %02x\n", config->bMaxPower);
                        break;
                case USB_DT_INTERFACE:
                        interface = (struct usbd_interface_descriptor *)sp;
                        total += sprintf(buf + total, "  bInterfaceNumber        [      2] %02x\n", interface->bInterfaceNumber);
                        total += sprintf(buf + total, "  bAlternateSetting        [      5] %02x\n",
                                        interface->bAlternateSetting);
                        total += sprintf(buf + total, "  bNumEndpoints            [      5] %02x\n", interface->bNumEndpoints);
                        total += sprintf(buf + total, "  bInterface[Class,Sub,Pro][      5] %02x %02x %02x\n",
                                        interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
                        total += dump_string_descriptor(function_instance, buf + total, interface->iInterface, "iInterface");
                        break;
                case USB_DT_ENDPOINT:
                        endpoint = (struct usbd_endpoint_descriptor *)sp;
                        break;
                case USB_DT_INTERFACE_ASSOCIATION:
                        iad = (struct usbd_interface_association_descriptor *)sp;
                        total += sprintf(buf + total, "  bFirstInterface          [      2] %02x\n", iad->bFirstInterface);
                        total += sprintf(buf + total, "  bInterfaceCount          [      3] %02x\n", iad->bInterfaceCount);
                        total += sprintf(buf + total, "  bFunction[Class,Sub,Pro] [      4] %02x %02x %02x\n",
                                        iad->bFunctionClass, iad->bFunctionSubClass, iad->bFunctionProtocol);
                        total += dump_string_descriptor(function_instance, buf + total, iad->iFunction, "iFunction");
                        break;
                default:
                        break;
                }
                wTotalLength -= sp[0];
                sp += sp[0];
        }
        total += sprintf(buf + total, "\n");
        return total;
}

/*!
 * usbd_device_proc_read - implement proc file system read.
 * @param page
 * @param count
 * @param pos
 *
 * Standard proc file system read function.
 *
 * We let upper layers iterate for us, *pos will indicate which device to return
 * statistics for.
 */
int usbd_device_proc_read (char *page, size_t count, int * pos)
{
        int len = 0;
        int index;

        u8 config_descriptor[512];
        int config_size;

        struct usbd_function_instance *function_instance =
                procfs_usbd_bus_instance && procfs_usbd_bus_instance->function_instance ?
                procfs_usbd_bus_instance->function_instance : NULL;

        struct pcd_instance *pcd_instance =  (struct pcd_instance *)
                procfs_usbd_bus_instance && procfs_usbd_bus_instance->privdata ?
                procfs_usbd_bus_instance->privdata : NULL;

        struct otg_instance *otg_instance = pcd_instance ?  pcd_instance->otg : NULL;

        //struct list_head *lhd;

        len = 0;
        index = (*pos)++;

        switch(index) {

        case 0:
                #if defined(CONFIG_USB_PERIPHERAL)
                len += sprintf ((char *) page + len, "USB Peripheral\n");
                #elif defined(CONFIG_USB_PERIPHERAL_OR_HOST)
                len += sprintf ((char *) page + len, "USB Peripheral or Host\n");
                #elif defined(CONFIG_USB_WIRED_DEVICE)
                len += sprintf ((char *) page + len, "USB Wired Device\n");
                #elif defined(CONFIG_USB_WIRED_DEVICE_OR_HOST)
                len += sprintf ((char *) page + len, "USB Wired Device or Host\n");
                #elif defined(CONFIG_OTG_BDEVICE_WITH_SRP)
                len += sprintf ((char *) page + len, "SRP Capable B-Device (only)\n");
                #elif defined(CONFIG_OTG_DEVICE)
                len += sprintf ((char *) page + len, "OTG Device\n");
                #endif
                len += sprintf ((char *) page + len, "usb-device list\n");
                break;

        case 1:
                if ( function_instance) {
                        //int configuration = index;
                        //struct usbd_function_driver *function_driver;
                        int i;


                        if ((config_size = usbd_get_descriptor(function_instance, config_descriptor,
                                                        sizeof(config_descriptor),
                                                        USB_DT_DEVICE, 0)) > index) {
                                len += dump_device_descriptor(function_instance, (char *)page + len, config_descriptor );
                        }

                        if ((config_size = usbd_get_descriptor(function_instance, config_descriptor,
                                                        sizeof(config_descriptor),
                                                        USB_DT_CONFIGURATION, 0)) > index) {
                                len += dump_config_descriptor(function_instance, (char *)page + len, config_descriptor );
                        }

                        #ifdef CONFIG_OTG_HIGH_SPEED
                        len += sprintf ((char *) page + len, "High Speed\n");
                        if ((config_size = usbd_get_descriptor(function_instance, config_descriptor,
                                                        sizeof(config_descriptor),
                                                        USB_DT_DEVICE_QUALIFIER, 0)) > index) {
                                len += dump_device_qualifier_descriptor(function_instance,
                                                (char *)page + len, config_descriptor );
                        }
                        len += sprintf ((char *) page + len, "Other Speed Descriptor\n");
                        if ((config_size = usbd_get_descriptor(function_instance, config_descriptor,
                                                        sizeof(config_descriptor),
                                                        USB_DT_OTHER_SPEED_CONFIGURATION, 0)) > index) {
                                len += dump_config_descriptor(function_instance, (char *)page + len, config_descriptor );
                        }
                        #endif /* CONFIG_OTG_HIGH_SPEED */

                        for (i = 0; i < procfs_usbd_bus_instance->usbd_maxstrings; i++) {

                                struct usbd_string_descriptor *string_descriptor
                                        = usbd_get_string_descriptor (function_instance, i);
                                struct usbd_langid_descriptor *langid_descriptor =
                                        (struct usbd_langid_descriptor *) string_descriptor;
                                int k;

                                CONTINUE_UNLESS (string_descriptor);

                                switch (i) {
                                case 0:
                                        len += sprintf((char *)page+len,
                                                        "LangID                     [ 0     ] %2x:%2x %02x%02x\n",
                                                        langid_descriptor->bLength, langid_descriptor->bDescriptorType,
                                                        langid_descriptor->bData[0], langid_descriptor->bData[1]
                                                        );
                                        break;
                                default:
                                        len += sprintf((char *)page+len, "String                     [%2d:%2d  ] ",
                                                        i, string_descriptor->bLength);

                                        // bLength = sizeof(struct usbd_string_descriptor) + 2*strlen(str)-2;

                                        for (k = 0; k < (string_descriptor->bLength / 2) - 1; k++) {
                                                *(char *) (page + len) = (char) string_descriptor->wData[k];
                                                len++;
                                        }
                                        len += sprintf ((char *) page + len, "\n");
                                        break;
                                }
                        }
                }
                else
                        len += list_function(&usbd_simple_drivers, (char *)page + len, "Not Enabled");

                break;

        case 2:
                len += list_function(&usbd_simple_drivers, (char *)page + len, "Simple Drivers");
                break;
        case 3:
                len += list_function(&usbd_class_drivers, (char *)page + len, "Class Drivers");
                break;
        case 4:
                len += list_function(&usbd_interface_drivers, (char *)page + len, "Interface Drivers");
                break;

        case 5:
                if (otg_instance && otg_instance->function_name && strlen(otg_instance->function_name))
                        len += sprintf((char *)page + len, "\nActive: %s\n\n", otg_instance->function_name);
                len += sprintf((char *)page + len, "\n%s\n", "Composite Drivers");
                break;

        default:
                len += list_function_composite(&usbd_composite_drivers, (char *)page + len, index - 6);
                break;
        }

        return len;
}

#if defined(CONFIG_OTG_LNX)
/*! *
 * usbd_device_proc_read_lnx - implement proc file system read.
 * @param file
 * @param buf
 * @param count
 * @param pos
 *
 * Standard proc file system read function.
 *
 * We let upper layers iterate for us, *pos will indicate which device to return
 * statistics for.
 */
static ssize_t usbd_device_proc_read_lnx (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;

        u8 config_descriptor[512];
        int config_size;

        if (!(page = GET_KERNEL_PAGE())) {
                return -ENOMEM;
        }

        len = 0;
        index = (*pos)++;

        len = usbd_device_proc_read((char *)page, count, (int *)pos);

        if (len > count) {
                len = -EINVAL;
        }
        else if ((len > 0) && copy_to_user (buf, (char *) page, len)) {
                len = -EFAULT;
        }
        free_page (page);
        return len;
}

/* Module init ******************************************************************************* */

/*!
 * usbd_device_proc_operations_functions -
 */
static struct file_operations usbd_device_proc_operations_functions = {
        read:usbd_device_proc_read_lnx,
};
#endif /* defined(CONFIG_OTG_LNX) */

/*!
 * usbd_procfs_init () -
 */
int usbd_procfs_init (struct usbd_bus_instance *bus)
{
        #if defined(CONFIG_OTG_LNX)
        /* create proc filesystem entries */
        struct proc_dir_entry *p;
        RETURN_ENOMEM_UNLESS ((p = create_proc_entry ("usb-functions", 0, NULL)));
        p->proc_fops = &usbd_device_proc_operations_functions;
        #endif /* defined(CONFIG_OTG_LNX) */
        procfs_usbd_bus_instance = bus;
        return 0;
}

/*!
 * usbd_procfs_exit () -
 */
void usbd_procfs_exit (struct usbd_bus_instance *bus)
{
        // remove proc filesystem entry
        procfs_usbd_bus_instance = NULL;
        #if defined(CONFIG_OTG_LNX)
        remove_proc_entry ("usb-functions", NULL);
        #endif /* defined(CONFIG_OTG_LNX) */
}
