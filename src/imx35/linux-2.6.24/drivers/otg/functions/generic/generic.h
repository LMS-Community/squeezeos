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
 * otg/functions/generic/generic-cf.h
 * @(#) tt@belcarra.com|otg/functions/generic/generic.h|20070316204018|00274
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
 * @defgroup GenericComposite Generic Composite Function
 * @ingroup CompositeFunctions
 */

/*!
 * @file otg/functions/generic/generic.h
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


/*
 * N.B. This list needs to be synchronized with both the default pre-defined
 * configuration conditional settings (see above) and the Kconfig list
 */
/*! @struct generic_config  generic.h "otg/functions/generic/generic.h"
 */
struct generic_config {
        struct usbd_composite_driver composite_driver;
        const char *class_name;
        const char *interface_names;
        const char ** interface_list;
        struct usbd_configuration_description configuration_description;
        struct usbd_device_description device_description;
        int registered;
        int interfaces;
};

int generic_modinit (struct generic_config *generic_configs,
		char *generic_config_name, BOOL override, u16 idVendor, u16 idProduct, char *iSerialNumber,
                otg_tag_t tag);
void generic_modexit (struct generic_config *generic_configs,
                otg_tag_t tag);
void generic_register(struct generic_config *config, const char *match, BOOL override, u16 idVendor, u16 idProduct,
		char *iSerialNumber,
                otg_tag_t tag);
