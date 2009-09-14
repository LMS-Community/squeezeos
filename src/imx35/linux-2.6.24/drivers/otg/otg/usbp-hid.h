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
 * otg/otg/usbp-hid.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/usbp-hid.h|20070810225414|35914
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif

/*!
 * @file otg/otg/usbp-hid.h
 * @brief HID class descriptor structure definitions.
 *
 * @ingroup USBDAPI
 */

/*!
 * @name HID requests
 */
 /*! @{ */
#define USB_REQ_GET_REPORT              0x01
#define USB_REQ_GET_IDLE                0x02
#define USB_REQ_GET_PROTOCOL            0x03
#define USB_REQ_SET_REPORT              0x09
#define USB_REQ_SET_IDLE                0x0A
#define USB_REQ_SET_PROTOCOL            0x0B
/*! @} */


/*!
 * @name HID - Class Descriptors
 * C.f. 7.1.1
 */
 /*! @{ */

#define HID_DT_HID      0x21
#define HID_DT_REPORT   0x22
#define HID_DT_PHYSICAL 0x23

/*! @} */

/*!
 * @name HID - Report Types
 * C.f. 7.1.1
 */
 /*! @{ */

#define HID_INPUT       0x01
#define HID_OUTPUT      0x02
#define HID_FEATURE     0x03

/*! @} */

/*!
 * @name HID Descriptor
 * C.f. E.8
 */
 /*! @{ */

/*! @struct hid_descriptor  usbp-hid.h "otg/usbp-hid.h"
 *
 *  @brief hid class descriptor struct
 */
PACKED0 struct PACKED1 hid_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u16 bcdHID;
        u8 bCountryCode;
        u8 bNumDescriptors;
        u8 bReportType;
        u16 wItemLength;
};

/*! @} */


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif
