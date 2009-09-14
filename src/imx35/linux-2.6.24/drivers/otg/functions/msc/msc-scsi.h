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
 * otg/msc/msc-scsi.h - mass storage protocol library header
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/msc-scsi.h|20061218212925|54647
 *
 *      Copyright(c) 2004-2006, Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 */

/*
 *
 * Documents
 *      Universal Serial Bus Mass Storage Class - Specification Overview
 *      Universal Serial Bus Mass Storage Class - Bulk-Only Transport
 *      T10/1240-D - Information technology - Reduced Block Commands
 *
 * Notes
 *
 * 1. Reduced Block Command set C.f. Table 2 RBC
 *
 *                                              Command Support
 *      Name                            OpCode  Fixed   Removable       Reference
 *      Format Unit                     04h     O       O               RBC
 *      Inquiry                         12h     M       M               SPC-2
 *      Mode Select (6)                 15h     M       M               SPC-2
 *      Mode Sense (6)                  1Ah     M       M               SPC-2
 *      Perisstent Reserve In           5Eh     O       O               SPC-2
 *      Persistent Reserve Out          5Fh     O       O               SPC-2
 *      Prevent/Allow Medium Removal    1Eh     N/A     M               SPC-2
 *      Read (10)                       28h     M       M               RBC
 *      Read Capacity                   25h     M       M               RBC
 *      Reelase (6)                     17h     O       O               SPC-2
 *      Request Sense                   03h     O       O               SPC-2
 *      Reserve (6)                     16h     O       O               SPC-2
 *      Start Stop Unit                 1Bh     M       M               RBC
 *      Synchronize Cache               35h     O       O               RBC
 *      Test Unit Ready                 00h     M       M               SPC-2
 *      Verify (10)                     2Fh     M       M               RBC
 *      Write (10)                      2Ah     M       M               RBC
 *      Write Buffer                    3Bh     M       O               SPC-2
 *
 * 2. Other commands seen?
 *                                                      Sh      MV      FS
 *      SCSI_REZERO_UNIT                     0x01       no
 *      SCSI_READ_6                          0x08                       yes
 *      SCSI_WRITE_6                         0x0a                       yes
 *      SCSI_SEND_DIAGNOSTIC                 0x1d       no              yes
 *      SCSI_READ_FORMAT_CAPACITY            0x23       yes             yes
 *      SCSI_WRITE_AND_VERIFY                0x2e       no
 *      SCSI_SEEK_10                         0x2b       no
 *      SCSI_MODE_SELECT_10                  0x55       no      yes     yes
 *      SCSI_READ_12                         0xa8       no              yes
 *      SCSI_WRITE_12                        0xaa       no              yes
 *
 *
 * 3. Status - C.f. Chapter 5 - RBC
 *
 *      Check Condition                 02h
 *
 *
 * 4. Sense Keys - C.f. Chapter 5 - RBC
 *
 *      Not Ready                       02h
 *      Media Error                     03h
 *      Illegal Request                 05h
 *      Unit Attention                  06h
 *
 * 5. ASC/ASCQ - C.f. Chapter 5 - RBC
 *
 *      Logical Unit Not Ready                                          04h
 *      Logical Unit Not Ready, Format in Progress                      04h,04h
 *      Invalid Command Operation Code                                  20h
 *      Invalide Field in CDB                                           24h
 *      Format Command Failed                                           31h,01h
 *      Status Notification / Power Management Class Event              38h,02h
 *      Status Notification / Media Class Event                         38h,04h
 *      Status Notification / Device Busy Class Event                   38h,06h
 *      Low Power Condition Active                                      5Eh,00
 *      Power Condition Change to Active                                5Eh,41h
 *      Power Condition Change to Idle                                  5Eh,42h
 *      Power Condition Change to Standby                               5Eh,43h
 *      Power Condition Change to Sleep                                 5Eh,45h
 *      Power Condition Change to Device Control                        5Eh,47h
 *
 * 6. ASCQ - C.f. Chapter 5 - RBC
 *
 *
 *
 * 7. Sense Keys C.f. Chapter 5 - RBC
 *
 * Command                      Status                  Sense Key      ASC/ASCQ
 *
 * 5.1 Format Unit
 *   Format progress            Check Condition         Not Ready,      Logical Unit Note Ready, Format in Progress
 *   Sueccsful completion       Check Condition         Unit Attention, Status Notification / Media Class Event
 *   Failure                    Check Condition         Media error,    Format Command Failed
 *
 * 5.2 Read (10)
 *
 * 5.3 Read Capacity
 *   No Media                   Check Condition         Not Ready,      Logical Unit Not Ready
 *
 * 5.4 Start Stop Unit
 *   Power Consumption          Check Condition         Illegal Request,Low Power Condition Active
 *
 * 5.5 Synchronize Cache        Check Condition         Illegal Request,Invalid Command Operation Code
 *
 * 5.6 Write (10
 *
 * 5.7 Verify
 *
 * 5.8 Mode
 *
 * 6.1 Inquiry
 *
 * 6.2 Mode Select (6)
 *   Cannot Save                Check Condition         Illegal Request,Invalid Field in CDB
 *
 * 6.3 Mode Sense (6)
 *
 * 6.4 Prevent Allow Medium Removal
 *
 * 6.5 Request Sense
 *
 * 6.6 Test Unit Ready
 *
 * 6.7 Write Buffer
 *
 * 7.1 Unit Attention
 *   Power Contition change     Check Condition         Unit Attention, Power Condition Change Notification
 *
 * 7.4.1 Event Status Sense
 *
 *   Power Management Event     Check Condition         Unit Attention, Event Status Notification / Power Managment
 *   Media Class Event          Check Condition         Unit Attention, Event Status Notification / Media Class
 *   Device Busy Event          Check Condition         Unit Attention, Event Status Notification / Device Busy Class
 *
 * 7.4.6 Removable Medium Device Initial Response
 *   Ready                      Check Condition         Unit Attention, Event Status Notification / Media Class Event
 *   Power                      Check Condition         Unit Attention, Event Status Notification / Power Management Class Event
 */

#ifndef _MSCSCSIPROTO_H_
#define _MSCSCSIPROTO_H_


/*
 * Class Specific Requests - C.f. MSC BO Chapter 3
 */

#define MSC_BULKONLY_RESET      0xff
#define MSC_BULKONLY_GETMAXLUN  0xfe


/*
 * Class Code
 */

#define MASS_STORAGE_CLASS                  0x08

/*
 * MSC - Specification Overview
 *
 * SubClass Codes - C.f MSC Table 2.1
 */

#define MASS_STORAGE_SUBCLASS_RBC           0x01
#define MASS_STORAGE_SUBCLASS_SFF8020I      0x02
#define MASS_STORAGE_SUBCLASS_QIC157        0x03
#define MASS_STORAGE_SUBCLASS_UFI           0x04
#define MASS_STORAGE_SUBCLASS_SFF8070I      0x05
#define MASS_STORAGE_SUBCLASS_SCSI          0x06

/*
 * Protocol - C.f MSC Table 3.1
 */

#define MASS_STORAGE_PROTO_CBI_WITH_COMP    0x00
#define MASS_STORAGE_PROTO_CBI_NO_COMP      0x01
#define MASS_STORAGE_PROTO_BULK_ONLY        0x50

/*
 * SCSI Command
*/

#define SCSI_TEST_UNIT_READY                    0x00
#define SCSI_REQUEST_SENSE                      0x03
#define SCSI_FORMAT_UNIT                        0x04
#define SCSI_INQUIRY                            0x12
#define SCSI_MODE_SELECT                        0x15    // aka MODE_SELECT_6
#define SCSI_MODE_SENSE                         0x1a    // aka MODE_SENSE_6
#define SCSI_START_STOP                         0x1b
#define SCSI_PREVENT_ALLOW_MEDIA_REMOVAL        0x1e
#define SCSI_READ_FORMAT_CAPACITY               0x23
#define SCSI_READ_CAPACITY                      0x25
#define SCSI_READ_10                            0x28
#define SCSI_WRITE_10                           0x2a
#define SCSI_VERIFY                             0x2f

#define SCSI_READ_6                             0x08
#define SCSI_WRITE_6                            0x0a
#define SCSI_RESERVE                            0x16
#define SCSI_RELEASE                            0x17
#define SCSI_SEND_DIAGNOSTIC                    0x1d
#define SCSI_SYNCHRONIZE_CACHE                  0x35
#define SCSI_MODE_SENSE_10                      0x5a

#define SCSI_REZERO_UNIT                        0x01
#define SCSI_REASSIGN_BLOCKS                    0x07
#define SCSI_COPY                               0x18
#define SCSI_RECEIVE_DIAGNOSTIC_RESULTS         0x1c
#define SCSI_WRITE_AND_VERIFY                   0x2e
#define SCSI_PREFETCH                           0x34
#define SCSI_READ_DEFECT_DATA                   0x37
#define SCSI_COMPARE                            0x39
#define SCSI_COPY_AND_VERIFY                    0x3a
#define SCSI_WRITE_BUFFER                       0x3b
#define SCSI_READ_BUFFER                        0x3c
#define SCSI_READ_LONG                          0x3e
#define SCSI_WRITE_LONG                         0x3f
#define SCSI_CHANGE_DEFINITION                  0x40
#define SCSI_WRITE_SAME                         0x41
#define SCSI_LOG_SELECT                         0x4c
#define SCSI_LOG_SENSE                          0x4d
#define SCSI_XD_WRITE                           0x50
#define SCSI_XP_WRITE                           0x51
#define SCSI_XD_READ                            0x52
#define SCSI_MODE_SELECT_10                     0x55
#define SCSI_RESERVE_10                         0x56
#define SCSI_RELEASE_10                         0x57
#define SCSI_MODE_SELECT_10                     0x55
#define SCSI_XD_WRITE_EXTENDED                  0x80
#define SCSI_REBUILD                            0x81
#define SCSI_REGENERATE                         0x82

#define SCSI_SEEK_10                            0x2b
#define SCSI_WRITE_AND_VERIFY                   0x2e
#define SCSI_WRITE_12                           0xaa
#define SCSI_READ_12                            0xa8

/*
 * Private
 */
#define SCSI_PRIVATE_PCS                        0xff

/*
 * SCSI Command Parameter
 */

#define CBW_SIGNATURE           0x43425355  /* USBC */
#define CSW_SIGNATURE           0x53425355  /* USBS */

#define PRODUCT_REVISION_LEVEL  "1.00"

/*
 * Command Block Status Values - C.f MSC BO Table 5.3
 */

#define USB_MSC_PASSED  0x00            // good
#define USB_MSC_FAILED  0x01            // bad
#define USB_MSC_PHASE_ERROR     0x02    // we want to be reset

#define NODATAPHASE       1
#define HASDATAPHASE      0



/*
 * SCSI Sense
 *      SenseKey
 *      AdditionalSenseCode
 *      SenseCodeQualifier
 */

#define SCSI_ERROR_CURRENT                      0x70
#define SCSI_ERROR_DEFERRED                     0x07

/*
 *  SCSI Sense Keys
 */

#define SK_NO_SENSE            0x00
#define SK_RECOVERED_ERROR     0x01
#define SK_NOT_READY           0x02
#define SK_MEDIA_ERROR         0x03
#define SK_HARDWARE_ERROR      0x04
#define SK_ILLEGAL_REQUEST     0x05
#define SK_UNIT_ATTENTION      0x06
#define SK_DATA_PROTECT        0x07
#define SK_BLANK_CHECK         0x08
#define SK_COPY_ABORTED        0x0a
#define SK_ABORTED_COMMAND     0x0b
#define SK_VOLUME_OVERFLOW     0x0d
#define SK_MISCOMPARE          0x0e

/*
 * 5. ASC/ASCQ - C.f. Chapter 5 - RBC
 */

#define SK(SenseKey,ASC,ASCQ) ((SenseKey<<16) | (ASC << 8) | (ASCQ))

#define SCSI_SENSEKEY_NO_SENSE                          SK(SK_NO_SENSE,         0x00,0x00)      // 0x000000

#define SCSI_FAILURE_PREDICTION_THRESHOLD_EXCEEDED      SK(SK_RECOVERED_ERROR,  0x5d,0x00)      // 0x015d00

#define SCSI_SENSEKEY_LOGICAL_UNIT_NOT_READY            SK(SK_NOT_READY,        0x04,0x00)      // 0x020400
#define SCSI_SENSEKEY_FORMAT_IN_PROGRESS                SK(SK_NOT_READY,        0x04,0x04)      // 0x020404
#define SCSI_SENSEKEY_MEDIA_NOT_PRESENT                 SK(SK_NOT_READY,        0x3a,0x00)      // 0x023a00

#define SCSI_SENSEKEY_WRITE_ERROR                       SK(SK_MEDIA_ERROR,      0x0c,0x02)      // 0x030c02
#define SCSI_SENSEKEY_UNRECOVERED_READ_ERROR            SK(SK_MEDIA_ERROR,      0x11,0x00)      // 0x031100
#define SCSI_FORMAT_COMMAND_FAILED                      SK(SK_MEDIA_ERROR,      0x31,0x01)      // 0x033101

#define SCSI_SENSEKEY_COMMUNICATION_FAILURE             SK(SK_HARDWARE_ERROR,   0x08,0x00)       // 0x040800

#define SCSI_SENSEKEY_INVALID_COMMAND                   SK(SK_ILLEGAL_REQUEST,  0x20,0x00)      // 0x052000
#define SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE        SK(SK_ILLEGAL_REQUEST,  0x21,0x00)      // 0x052100
#define SCSI_SENSEKEY_INVALID_FIELD_IN_CDB              SK(SK_ILLEGAL_REQUEST,  0x24,0x00)      // 0x052400
#define SCSI_SENSEKEY_LOGICAL_UNIT_NOT_SUPPORTED        SK(SK_ILLEGAL_REQUEST,  0x25,0x00)      // 0x052500
#define SCSI_SENSEKEY_SAVING_PARAMETERS_NOT_SUPPORTED   SK(SK_ILLEGAL_REQUEST,  0x39,0x00)      // 0x053900
#define SCSI_MEDIA_REMOVAL_PREVENTED                    SK(SK_ILLEGAL_REQUEST,  0x53,0x02)      // 0x055302

#define SCSI_SENSEKEY_NOT_READY_TO_READY_CHANGE         SK(SK_UNIT_ATTENTION,   0x28,0x00)      // 0x062800
#define SCSI_SENSEKEY_RESET_OCCURRED                    SK(SK_UNIT_ATTENTION,   0x29,0x00)      // 0x062900

#define SCSI_SENSEKEY_STATUS_NOTIFICATION_POWER_CLASS   SK(SK_UNIT_ATTENTION,   0x38,0x02)      // 0x063802
#define SCSI_SENSEKEY_STATUS_NOTIFICATION_MEDIA_CLASS   SK(SK_UNIT_ATTENTION,   0x38,0x04)      // 0x063804
#define SCSI_SENSEKEY_STATUS_NOTIFICATION_DEVICE_BUSY   SK(SK_UNIT_ATTENTION,   0x38,0x06)      // 0x063806

#define SCSI_SENSEKEY_LOW_POWER_CONDITION_ACTIVE        SK(SK_UNIT_ATTENTION,   0x5e,0x00)      // 0x065e00
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_ACTIVE  SK(SK_UNIT_ATTENTION,   0x5e,0x41)      // 0x065e41
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_IDLE    SK(SK_UNIT_ATTENTION,   0x5e,0x42)      // 0x065e42
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_STANDBY SK(SK_UNIT_ATTENTION,   0x5e,0x43)      // 0x065e43
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_SLEEP   SK(SK_UNIT_ATTENTION,   0x5e,0x45)      // 0x065e45
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_DEVICE  SK(SK_UNIT_ATTENTION,   0x5e,0x47)      // 0x065e47


#define SCSI_SENSEKEY_WRITE_PROTECTED                   SK(SK_DATA_PROTECT,     0x27,0x00)      // 0x072700


/*
 * Mode Page Code and Page Control
 */
#define SCSI_MODEPAGE_CONTROL_CURRENT           0x0
#define SCSI_MODEPAGE_CONTROL_CHANGEABLE        0x1
#define SCSI_MODEPAGE_CONTROL_DEFAULT           0x2
#define SCSI_MODEPAGE_CONTROL_SAVED             0x3

#define SCSI_MODEPAGE_UNIT_ATTENTION            0x00
#define SCSI_MODEPAGE_ERROR_RECOVERY            0x01
#define SCSI_MODEPAGE_DISCONNNECT_RECONNECT     0x02
#define SCSI_MODEPAGE_FORMAT                    0x03
#define SCSI_MODEPAGE_RIGID_DRIVE_GEOMETRY      0x04
#define SCSI_MODEPAGE_FLEXIBLE_DISK_PAGE        0x05    // check
#define SCSI_MODEPAGE_VERIFY_ERROR_RECOVERY     0x07
#define SCSI_MODEPAGE_CACHING                   0x08
#define SCSI_MODEPAGE_CONTROL_MODE              0x0a
#define SCSI_MODEPAGE_NOTCH_AND_PARTITION       0x0c
#define SCSI_MODEPAGE_POWER_CONDITION           0x0d
#define SCSI_MODEPAGE_XOR                       0x10
#define SCSI_MODEPAGE_CONTROL_MODE_ALIAS        0x1a
#define SCSI_MODEPAGE_REMOVABLE_BLOCK_ACCESS    0x1b// check
#define SCSI_MODEPAGE_INFORMATION_EXCEPTIONS    0x1c
#define SCSI_MODEPAGE_ALL_SUPPORTED             0x3f



/*
 * Command Block Wrapper / Command Status Wrapper
 */

/*! @struct msc_command_block_wrapper msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 * @brief  Command Block Wrapper
 */
 struct msc_command_block_wrapper {
        u32  dCBWSignature;
        u32  dCBWTag;
        u32  dCBWDataTransferLength;
        u8   bmCBWFlags;
        u8   bCBWLUN:4,
             Reserved:4;
        u8   bCBWCBLength:5,
             Reserved2:3;
        u8   CBWCB[16];
} __attribute__((packed));

/*! @struct msc_command_status_wrapper msc-scsi.h  "otg/functions/msc/msc-scsi.h"
 *
 *  @brief Command Status Wrapper
 */
struct msc_command_status_wrapper {
        u32  dCSWSignature;
        u32  dCSWTag;
        u32  dCSWDataResidue;
        u8   bCSWStatus;
} __attribute__((packed));

/*! @struct msc_scsi_inquiry_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief SCSI INQUIRY command wrapper
 */
struct msc_scsi_inquiry_command {
        u8   OperationCode;
        u8   EnableVPD:1,
             Reserved1:4,
             LogicalUnitNumber:3;
        u8   PageCode;
        u8   Reserved2;
        u8   AllocationLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
} __attribute__((packed));

/* ! @struct msc_scsi_inquiry_data msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief msc scsi inquiry data wrapper
 */
struct msc_scsi_inquiry_data {
        u8   PeripheralDeviceType:5,
             PeripheralQaulifier:3;
        u8   Reserved2:7,
             RMB:1;
        u8   ANSIVersion:3,
             ECMAVersion:3,
             ISOVersion:2;
        u8   ResponseDataFormat:4,
             Reserved3:4;
        u8   AdditionalLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   VendorInformation[8];
        u8   ProductIdentification[16];
        u8   ProductRevisionLevel[4];
} __attribute__((packed));

/*! @struct msc_scsi_read_format_capacity_command   msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  msc scsi read_format_capacity command wrapper
 *
 */
struct msc_scsi_read_format_capacity_command {
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u16  AllocationLength;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
} __attribute__((packed));

/*! @struct msc_scsi_read_format_capacity_data msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief msc scsi read_format_capacity data warpper
 *
 */

struct msc_scsi_read_format_capacity_data {
        struct{
                u8   Reserved1;
                u8   Reserved2;
                u8   Reserved3;
                u8   CapacityListLength;
        } __attribute__((packed)) CapacityListHeader;

        struct{
                u32  NumberofBlocks;
                u8   DescriptorCode:2,
                     Reserved1:6;
                u8   BlockLength[3];
        } __attribute__((packed)) CurrentMaximumCapacityDescriptor;

} __attribute__((packed));

/*! @struct msc_scsi_read_capacity_command  msc-scsi.h  "otg/functions/msc/msc-scsi.h"
 *
 *  @brief msc scsi read_capacity_command wrapper
 *
 */
struct msc_scsi_read_capacity_command {
        u8   OperationCode;
        u8   RelAdr:1,
             Reserved1:4,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u8   Reserved3;
        u8   PMI:1,
             Reserved4:7;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
} __attribute__((packed));

/*! @struct  msc_scsi_read_capacity_data msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief READ FORMAT CAPACITY DATA
 */
struct msc_scsi_read_capacity_data {
        u32   LastLogicalBlockAddress;
        u32   BlockLengthInBytes;
} __attribute__((packed));

/*! @struct msc_scsi_request_sense_command  msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief REQUEST SENSE
 */
struct msc_scsi_request_sense_command {
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   AllocationLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
} __attribute__((packed));

/*! @struct msc_scsi_request_sense_data msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief REQUEST SENSE DATA
 */
struct msc_scsi_request_sense_data {
        u8   ErrorCode:7,
             Valid:1;
        u8   Reserved1;
        u8   SenseKey:4,
             Reserved2:4;
        u32  Information;
        u8   AdditionalSenseLength;
        u8   Reserved3[4];
        u8   AdditionalSenseCode;
        u8   AdditionalSenseCodeQualifier;
        u8   Reserved4;
        u8   Reserved5[3];
} __attribute__((packed));

/* @struct msc_scsi_read_10_command  msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 * @brief msc scsi read_10 command wrapper
 */
struct msc_scsi_read_10_command {
        u8   OperationCode;
        u8   RelAdr:1,
             Reserved1:2,
             FUA:1,
             DPO:1,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u16  TransferLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed));

/*! @struct msc_scsi_mode_sense_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief msc_scsi_mode_sense_command wrapper
 * MODE SENSE
 */
struct msc_scsi_mode_sense_command {
        u8   OperationCode;
        u8   Reserved1:3,
             DBD:1,
             Reserved2:1,
             LogicalUnitNumber:3;
        u8   PageCode:6,
             PageControl:2;          // PC
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u16   ParameterListLength;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
} __attribute__((packed));

/*! @struct msc_mode_parameter_header msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  MODE PARAMETER HEADER
 */
struct msc_mode_parameter_header {
        u8   ModeDataLength;
        u8   MediumTypeCode;
        u8   Reserved1:4,
             DPOFUA:1,
             Reserved2:2,
             WriteProtect:1;
        u8   Reserved3;
} __attribute__((packed));

/*! @struct msc_read_write_error_recovery_page  msc-scsi.h  "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  MODE READ WRITE ERROR RECOVERY PAGE
 */
struct msc_read_write_error_recovery_page{
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u8   DCR:1,
             Reserved2:1,
             PER:1,
             Reserved3:1,
             RC:1,
             Reserved4:1,
             Reserved5:1,
             AWRE:1;
        u8   ReadRetryCount;
        u8   Reserved6[4];
        u8   WriteRetryCount;
        u8   Reserved7[3];
} __attribute__((packed));

/*! @struct msc_flexible_disk_page msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  FLEXIBLE DISK PAGE
 */
struct msc_flexible_disk_page {
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u16  TransferRate;
        u8   NumberofHeads;
        u8   SectorsperTrack;
        u16  DataBytesperSector;
        u16  NumberofCylinders;
        u8   Reserved2[9];
        u8   MotorOnDelay;
        u8   MotorOffDelay;
        u8   Reserved3[7];
        u16  MediumRotationRate;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed));

/*! @struct msc_removable_block_access_capabilities_page  msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  REMOVABLE BLOCK ACCESS CAPABILITIES PAGE
 */
struct msc_removable_block_access_capabilities_page {
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u8   Reserved2:6,
             SRFP:1,
             SFLP:1;
        u8   TLUN:3,
             Reserved3:3,
             SML:1,
             NCD:1;
        u8   Reserved4[8];
} __attribute__((packed));

/*!  @struct msc_timer_and_protect_page msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  TIMER AND PROTECT PAGE
 */
 struct msc_timer_and_protect_page {
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u8   Reserved2;
        u8   InactivityTimeMultiplier:4,
             Reserved3:4;
        u8   SWPP:1,
             DISP:1,
             Reserved4:6;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
} __attribute__((packed));

/*! @struct msc_mode_all_pages  msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief MODE ALL PAGES
 */
struct msc_mode_all_pages {
        struct msc_read_write_error_recovery_page              ReadWriteErrorRecoveryPage;
        struct msc_flexible_disk_page                          FlexibleDiskPage;
        struct msc_removable_block_access_capabilities_page    RemovableBlockAccessCapabilitiesPage;
        struct msc_timer_and_protect_page                      TimerAndProtectPage;
} __attribute__((packed));

/*! @struct msc_scsi_mode_sense_date  msc-scsi.h  "otg/functions/msc/msc-scsi.h"
 *
 *  @brief  SCSI MODE SENSE DATA
 */
struct msc_scsi_mode_sense_data {
        struct msc_mode_parameter_header   ModeParameterHeader;
        union{
            struct msc_read_write_error_recovery_page              ReadWriteErrorRecoveryPage;
            struct msc_flexible_disk_page                          FlexibleDiskPage;
            struct msc_removable_block_access_capabilities_page    RemovableBlockAccessCapabilitiesPage;
            struct msc_timer_and_protect_page                      TimerAndProtectPage;
            struct msc_mode_all_pages                              ModeAllPages;
        } __attribute__((packed)) ModePages;
} __attribute__((packed));

/*! @struct msc_scsi_test_unit_ready_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief TEST UNIT READY
 */
struct msc_scsi_test_unit_ready_command {
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed));

/*! @struct msc_scsi_prevent_allow_media_removal_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief PREVENT-ALLOW MEDIA REMOVAL
 */
struct msc_scsi_prevent_allow_media_removal_command {
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Prevent:2,
             Reserved4:6;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed));

/*! @struct msc_scsi_start_stop_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief START-STOP UNIT
 */
struct msc_scsi_start_stop_command {
        u8   OperationCode;
        u8   IMMED:1,
             Reserved1:4,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Start:1,
             LoEj:1,
             Reserved4:2,
             PowerConditions:4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed));

/*! @struct msc_scsi_write_10_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *  @brief WRITE 10  command wrapper
 */
struct msc_scsi_write_10_command {
        u8   OperationCode;
        u8   RelAdr:1,
             Reserved1:2,
             FUA:1,
             DPO:1,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u16  TransferLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed));

/*!  @struct msc_scsi_verify_command msc-scsi.h "otg/functions/msc/msc-scsi.h"
 *
 *   @brief msc_scsi VERIFY command wrapper
 */
struct msc_scsi_verify_command {
        u8   OperationCode;
        u8   RelAdr:1,
             ByteChk:1,
             Reserved1:1,
             Reserved2:1,
             DPO:1,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved3;
        u16  VerificationLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
} __attribute__((packed));

#endif  /* _MSCSCSIPROTO_H_ */
