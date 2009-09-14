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
 * otg/otg/usbp-cdc.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/usbp-cdc.h|20061218212925|24522
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/otg/usbp-cdc.h
 * @brief CDC class descriptor structure definitions.
 *
 * @ingroup USBDAPI
 */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif

/*!
 * @name Class-Specific Request Codes
 * C.f. CDC Table 46
 */
 /*! @{ */

#define CDC_CLASS_REQUEST_SEND_ENCAPSULATED     0x00
#define CDC_CLASS_REQUEST_GET_ENCAPSULATED      0x01

#define CDC_CLASS_REQUEST_SET_COMM_FEATURE      0x02
#define CDC_CLASS_REQUEST_GET_COMM_FEATURE      0x03
#define CDC_CLASS_REQUEST_CLEAR_COMM_FEATURE    0x04

#define CDC_CLASS_REQUEST_SET_LINE_CODING       0x20
#define CDC_CLASS_REQUEST_GET_LINE_CODING       0x21

#define CDC_CLASS_REQUEST_SET_CONTROL_LINE_STATE 0x22
#define CDC_CLASS_REQUEST_SEND_BREAK            0x23
/*! @} */

/*!
 * @name Notification codes
 * c.f. CDC Table 68
 */
 /*! @{ */

#define CDC_NOTIFICATION_NETWORK_CONNECTION         0x00
#define CDC_NOTIFICATION_RESPONSE_AVAILABLE         0x01
#define CDC_NOTIFICATION_AUX_JACK_HOOK_STATE        0x08
#define CDC_NOTIFICATION_RING_DETECT                0x09
#define CDC_NOTIFICATION_SERIAL_STATE               0x20
#define CDC_NOTIFICATION_CALL_STATE_CHANGE          0x28
#define CDC_NOTIFICATION_LINE_STATE_CHANGE          0x29
#define CDC_NOTIFICATION_CONNECTION_SPEED_CHANGE    0x2a
/*! @} */


/*!
 * @name ACM - Line Coding structure
 * C.f. CDC Table 50
 */
 /*! @{ */

/*! @struct cdc_acm_line_coding  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief acm line code wrapper
 *
 */
PACKED0 struct PACKED1 cdc_acm_line_coding {
        u32 dwDTERate;
        u8 bCharFormat;
        u8 bParityType;
        u8 bDataBits;
};

 /*!@} */

/*!
 * @name ACM - Line State
 * C.f. CDC Table 51
 */
 /*! @{ */
/*! create a type for u16  */
typedef u16 cdc_acm_bmLineState;
#define CDC_LINESTATE_D1_RTS            (1 << 1)
#define CDC_LINESTATE_D0_DTR            (1 << 0)
/*! @} */

/*!
 * Serial State - C.f. 6.3.5
 */
/*! @struct acm_serial_state usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief acm serial state wrapper
 */
PACKED0 struct PACKED1 acm_serial_state {
        u16     uart_state_bitmap;
};


/*!
* @name ACM - UART State
 * C.f. Table 69 - UART State Bitmap Values
 */
/*! @{ */
/*! create a type for u16 */
typedef u16 cdc_acm_bmUARTState;
#define CDC_UARTSTATE_BOVERRUN        (1 << 6)
#define CDC_UARTSTATE_BPARITY         (1 << 5)
#define CDC_UARTSTATE_BFRAMING        (1 << 4)
#define CDC_UARTSTATE_BRINGSIGNAL     (1 << 3)
#define CDC_UARTSTATE_BBREAK          (1 << 2)
#define CDC_UARTSTATE_BTXCARRIER_DSR  (1 << 1)
#define CDC_UARTSTATE_BRXCARRIER_DCD  (1 << 0)
/*! @} */

/*! @struct cdc_notification_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
  *
  *  @brief USB Notification descriptor struct
  */
PACKED0 struct PACKED1 cdc_notification_descriptor {
        u8 bmRequestType;
        u8 bNotification;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u8 data[2];
};




/*!
 * @name Communications Class Types
 *
 * c.f. CDC  USB Class Definitions for Communications Devices
 * c.f. WMCD USB CDC Subclass Specification for Wireless Mobile Communications Devices
 *
 */
/*! @{ */

#define CLASS_BCD_VERSION               0x0110
/*! @} */

/*! @name c.f. CDC 4.1 Table 14 */

/*! @{ */

#define AUDIO_CLASS                     0x01
#define COMMUNICATIONS_DEVICE_CLASS     0x02
/*! @} */

/*! @name c.f. CDC 4.2 Table 15
 */
 /*! @{ */
#define COMMUNICATIONS_INTERFACE_CLASS  0x02
/*! @} */

/*! @name c.f. CDC 4.3 Table 16
 */
 /*! @{ */
#define COMMUNICATIONS_NO_SUBCLASS      0x00
#define COMMUNICATIONS_DLCM_SUBCLASS    0x01
#define COMMUNICATIONS_ACM_SUBCLASS     0x02
#define COMMUNICATIONS_TCM_SUBCLASS     0x03
#define COMMUNICATIONS_MCCM_SUBCLASS    0x04
#define COMMUNICATIONS_CCM_SUBCLASS     0x05
#define COMMUNICATIONS_ENCM_SUBCLASS    0x06
#define COMMUNICATIONS_ANCM_SUBCLASS    0x07

#define AUDIO_CONTROL_SUBCLASS          0x01
#define AUDIO_STREAMING_SUBCLASS        0x02
/*! @} */

/*! @name c.f. WMCD 5.1
 */
 /*! @{ */
#define COMMUNICATIONS_WHCM_SUBCLASS    0x08
#define COMMUNICATIONS_DMM_SUBCLASS     0x09
#define COMMUNICATIONS_MDLM_SUBCLASS    0x0a
#define COMMUNICATIONS_OBEX_SUBCLASS    0x0b
#define COMMUNICATIONS_EEM_SUBCLASS     0x0c
/*! @} */

/*! @name c.f. CDC 4.6 Table 18
 */
 /*! @{ */
#define DATA_INTERFACE_CLASS            0x0a
/*! @} */

/*! @name c.f. CDC 4.7 Table 19
 */
 /*! @{ */
#define COMMUNICATIONS_NO_PROTOCOL      0x00
#define COMMUNICATIONS_EEM_PROTOCOL     0x07
/*! @} */

/*!
 * @name bDescriptorSubtypes
 *
 * c.f. CDC 5.2.3 Table 25
 * c.f. WMCD 5.3 Table 5.3
 */
 /*! @{ */

#define USB_ST_HEADER                   0x00
#define USB_ST_CMF                      0x01
#define USB_ST_ACMF                     0x02
#define USB_ST_DLMF                     0x03
#define USB_ST_TRF                      0x04
#define USB_ST_TCLF                     0x05
#define USB_ST_UF                       0x06
#define USB_ST_CSF                      0x07
#define USB_ST_TOMF                     0x08
#define USB_ST_USBTF                    0x09
#define USB_ST_NCT                      0x0a
#define USB_ST_PUF                      0x0b
#define USB_ST_EUF                      0x0c
#define USB_ST_MCMF                     0x0d
#define USB_ST_CCMF                     0x0e
#define USB_ST_ENF                      0x0f
#define USB_ST_ATMNF                    0x10

#define USB_ST_WHCM                     0x11
#define USB_ST_MDLM                     0x12
#define USB_ST_MDLMD                    0x13
#define USB_ST_DMM                      0x14
#define USB_ST_OBEX                     0x15
#define USB_ST_CS                       0x16
#define USB_ST_CSD                      0x17
#define USB_ST_TCM                      0x18
/*! @} */

/*!
 * @name Call Management - bmCapabilities
 *
 * c.f. CDC 5.2.3.2 Table 27
 * c.f. WMCD
 */
#define USB_CMFD_DATA_CLASS              0x02
#define USB_CMFD_CALL_MANAGEMENT         0x01
/*! @} */

/*!
 * @name Abstract Control Management - bmCapabilities
 *
 * c.f. CDC 5.2.3.2 Table 28
 * c.f. WMCD
 */
#define USB_ACMFD_NETWORK                 0x08
#define USB_ACMFD_SEND_BREAK              0x04
#define USB_ACMFD_CONFIG                  0x02
#define USB_ACMFD_COMM_FEATURE            0x01
/*! @} */

/*!
 * @name Class Descriptor Description Structures...
 * c.f. CDC 5.1
 * c.f. WCMC 6.7.2
 *
 * XXX add the other dozen class descriptor description structures....
 *
 */
 /*! @{ */
/*! @struct   usbd_header_functional_descriptor   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief    header functional descriptor
 */
PACKED0 struct PACKED1 usbd_header_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u16 bcdCDC;
};
/*! @struct    usbd_call_management_functional_descriptor   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief     call management functional descriptor
 */
PACKED0 struct PACKED1 usbd_call_management_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
        u8 bDataInterface;
};
/*! @struct  usbd_abstract_control_functional_descriptor usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  abstract control functional descriptor
 */
PACKED0 struct PACKED1 usbd_abstract_control_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
};
/*! @struct usbd_union_functional_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  union functional descriptor
 */
PACKED0 struct PACKED1 usbd_union_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bMasterInterface;
        u8 bSlaveInterface[4];
};
/*! @struct  usbd_ethernet_networking_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  ethernet networking descriptor
 */
PACKED0 struct PACKED1 usbd_ethernet_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        char *iMACAddress;
        u8 bmEthernetStatistics;
        u16 wMaxSegmentSize;
        u16 wNumberMCFilters;
        u8 bNumberPowerFilters;
};
/*! @struct   usbd_mobile_direct_line_model_descriptor   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief    mobile direct line model descriptor struct
 */PACKED0 struct PACKED1 usbd_mobile_direct_line_model_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u16 bcdVersion;
        u8 bGUID[16];
};
/*! @struct  usbd_mobile_direct_line_model_detail_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  usbd_mobile direct line model detail descriptor struct
 */
PACKED0 struct PACKED1 usbd_mobile_direct_line_model_detail_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bGuidDescriptorType;
        u8 bDetailData[4];
};
/*! @} */




/*!
 * @name Communications Class Dscriptor Structures
 * c.f. CDC 5.2 Table 25c
  */

/*! @{*/
/*! @struct  usbd_class_function_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   CDC class function descriptor
 */
PACKED0 struct PACKED1 usbd_class_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
};
/*! @struct   usbd_class_function_descriptor_generic   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   CDC calss function descriptor generic struct
 */
PACKED0 struct PACKED1 usbd_class_function_descriptor_generic {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
};
/*! @struct usbd_class_header_function_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  class header function descriptor
 */
PACKED0 struct PACKED1 usbd_class_header_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x00
        u16 bcdCDC;
};
/*! @struct  usbd_class_call_management_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class call management descriptor
 */
PACKED0 struct PACKED1 usbd_class_call_management_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x01
        u8 bmCapabilities;
        u8 bDataInterface;
};
/*! @struct  usbd_class_abstract_control_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class abstract control descriptor struct
 */
PACKED0 struct PACKED1  usbd_class_abstract_control_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x02
        u8 bmCapabilities;
};
/*! @struct  usbd_class_direct_line_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class direct line descriptor struct
 */
PACKED0 struct PACKED1  usbd_class_direct_line_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x03
};
/*! @struct    usbd_class_telephone_ringer_descriptor   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief     class telephone ringer descriptor struct
 */
PACKED0 struct PACKED1  usbd_class_telephone_ringer_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x04
        u8 bRingerVolSeps;
        u8 bNumRingerPatterns;
};
/*! @struct  usbd_class_telephone_call_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  class telephone call descriptor struct
 */
PACKED0 struct PACKED1  usbd_class_telephone_call_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x05
        u8 bmCapabilities;
};
/*! @struct  usbd_class_union_function_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class union function descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_union_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x06
        u8 bMasterInterface;
        u8 bSlaveInterface[4];
};
/*! @struct  usbd_class_country_selection_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class country selection descriptor struct
 */
PACKED0 struct PACKED1  usbd_class_country_selection_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x07
        u8 iCountryCodeRelDate;
        u16 wCountryCode0[4];
};

/*! @struct  usbd_class_telephone_operational_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief class telephone operational descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_telephone_operational_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x08
        u8 bmCapabilities;
};

/*! @struct  usbd_class_usbd_terminal_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class usbd terminal descriptor struct
 *
 */
PACKED0 struct PACKED1 usbd_class_usbd_terminal_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x09
        u8 bEntityId;
        u8 bInterfaceNo;
        u8 bOutInterfaceNo;
        u8 bmOptions;
        u8 bChild0[4];
};
/*! @struct  usbd_class_network_channel_descriptor usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class network channel descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_network_channel_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0a
        u8 bEntityId;
        u8 iName;
        u8 bChannelIndex;
        u8 bPhysicalInterface;
};
/*! @struct  usbd_class_protocol_unit_function_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class protocol unit function descriptor
 */
PACKED0 struct PACKED1 usbd_class_protocol_unit_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0b
        u8 bEntityId;
        u8 bProtocol;
        u8 bChild0[4];
};
/*! @struct  usbd_class_extension_unit_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class extension unit descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_extension_unit_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0c
        u8 bEntityId;
        u8 bExtensionCode;
        u8 iName;
        u8 bChild0[4];
};
/*! @struct  usbd_class_multi_channel_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  class multi_channel descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_multi_channel_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0d
        u8 bmCapabilities;
};
/*! @struct  usbd_class_capi_control_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  class capi control descriptor
 */
PACKED0 struct PACKED1 usbd_class_capi_control_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0e
        u8 bmCapabilities;
};
/*! @struct   usbd_class_ethernet_networking_descriptor   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief    class ethernet networking descriptor
 */
PACKED0 struct PACKED1 usbd_class_ethernet_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0f
        u8 iMACAddress;
        u32 bmEthernetStatistics;
        u16 wMaxSegmentSize;
        u16 wNumberMCFilters;
        u8 bNumberPowerFilters;
};
/*! @struct  usbd_class_atm_networking_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief class atm networking descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_atm_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x10
        u8 iEndSystermIdentifier;
        u8 bmDataCapabilities;
        u8 bmATMDeviceStatistics;
        u16 wType2MaxSegmentSize;
        u16 wType3MaxSegmentSize;
        u16 wMaxVC;
};

/*! @struct  usbd_class_mdlm_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  class mdlm  descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_mdlm_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x12
        u16 bcdVersion;
        u8 bGUID[16];
};

/*
PACKED0 struct PACKED1 usbd_class_mdlmd_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bDetailData[4];
};
*/
/*! @struct  usbd_class_blan_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   class blan descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_blan_descriptor {
        u8 bFunctionLength;           // 0x7
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
        u8 bPad;
};
/*! @struct  usbd_class_safe_descriptor  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief class safe descriptor struct
 */
PACKED0 struct PACKED1 usbd_class_safe_descriptor {
        u8 bFunctionLength;           // 0x6
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
};

/*! @} */




/*!
 * @name EEM
 * c.f. EEM 1.0
  */
/*! @{*/

/*! @struct  usbd_eem_packet_header usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   eem packet header wrapper
 */
PACKED0 struct PACKED1 usbd_eem_packet_header {
        u8 bmType:1;
        u16 eem: 15;
};
/*! @struct  usbd_eem_data_packet  usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief   eem data packet wrapper
 */
PACKED0 struct PACKED1 usbd_eem_data_packet {
        u8 bmType:1;
        u8 bmCRC:1;
        u16 length:14;
};

/*! @struct   usbd_eem_command_packet   usbp-cdc.h  "otg/usbp-cdc.h"
 *
 *  @brief  eem command packet wrapper
 */
PACKED0 struct PACKED1 usbd_eem_command_packet {
        u8 bmType:1;
        u8 bmReserved:4;
        u16 bmEEMCmdParam:11;
};

#define EEM_TYPE_DATA                   0x0
#define EEM_TYPE_COMMAND                0x1

#define EEM_ECHO                        0x0
#define EEM_ECHO_RESPONSE               0x1
#define EEM_SUSPEND_HINT                0x2
#define EEM_RESPONSE_HINT               0x3
#define EEM_RESPONSE_COMPLETE_HINT      0x4
#define EEM_TICKLE                      0x5




/*@}*/


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif
