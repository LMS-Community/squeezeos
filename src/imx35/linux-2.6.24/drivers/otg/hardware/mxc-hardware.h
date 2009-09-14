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
 * otg/hardware/mxc-hardware.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/mxc/mxc-hardware.h|20061218212926|59377
 *
 * 	Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @defgroup FSOTG Freescale MXC USBOTG Support
 * @ingroup Hardware
 */
/*!
 * @file otg/hardware/mxc-hardware.h
 * @brief Hardware defines for Freescale USBOTG Hardware
 *
 * This supports the Freescale MXC implementation of the Trans Dimension
 * USBOTG.
 *
 * This is used on:
 *
 *      - i.MX21
 *      - SCM-A11
 *      - Argon+
 *      - Zeus
 *
 * @ingroup FSOTG
 */


/*!
 * @name OTG_SYS_CTRL
 * C.f. 23.8 USB Control Register
 */
 /*! @{ */

//#define OTG_SYS_CTRL                          (OTG_SYS_BASE+0x00)

#define SYS_CTRL_I2C_WU_INT_STAT                (1 << 27)
#define SYS_CTRL_OTG_WU_INT_STAT                (1 << 26)
#define SYS_CTRL_HOST_WU_INT_STAT               (1 << 25)
#define SYS_CTRL_FNT_WU_INT_STAT                (1 << 24)

#define SYS_CTRL_I2C_WU_INT_EN                  (1 << 19)
#define SYS_CTRL_OTG_WU_INT_EN                  (1 << 18)
#define SYS_CTRL_HOST_WU_INT_EN                 (1 << 17)
#define SYS_CTRL_FNT_WU_INT_EN                  (1 << 16)

#define SYS_CTRL_OTG_BYP_VAL                    (0x3 << 11)
#define SYS_CTRL_HOST1_BYP_VAL                  (0x3 << 9)

#define SYS_CTRL_OTG_PWR_MASK                   (1 << 6)
#define SYS_CTRL_HOST1_PWR_MASK                 (1 << 6)
#define SYS_CTRL_HOST2_PWR_MASK                 (1 << 4)
#define SYS_CTRL_USB_BYP                        (1 << 2)
#define SYS_CTRL_HOST1_TXEN_OE                  (1 << 1)
/*! @} */

/*!
 * @name C.f. 23.9 USBOTG Module Registers
 */
 /*! @{ */

#define OTG_CORE_HWMODE                         (OTG_CORE_BASE+0x00)            //  32bit core hardware mode reg

#define XCVR_D_D                                0x00
//#define XCVR_SE0_D_OLD                          0x01            // XXX should be 0x02
//#define XCVR_D_SE0_OLD                          0x02            // XXX should be 0x01
#define XCVR_SE0_D_NEW                          0x02            // XXX should be 0x02
#define XCVR_D_SE0_NEW                          0x01            // XXX should be 0x01
#define XCVR_SE0_SE0                            0x03

#define MODULE_ANASDBEN                         (1 << 14)
#define MODULE_OTGXCVR                          (0x3 << 6)
#define MODULE_HOSTXCVR                         (0x3 << 4)
#define MODULE_CRECFG                           (0x3)
#define MODULE_CRECFG_HHNP                      (0x0)
#define MODULE_CRECFG_HOST                      (0x1)
#define MODULE_CRECFG_FUNC                      (0x2)
#define MODULE_CRECFG_SHNP                      (0x3)

#define OTG_CORE_CINT_STAT                      (OTG_CORE_BASE+0x04)            //  32bit core int status reg

#define MODULE_FCINTDSPEN                       (1 << 6)

#define MODULE_ASHNPINT                         (1 << 5)
#define MODULE_ASFCINT                          (1 << 4)
#define MODULE_ASHCINT                          (1 << 3)
#define MODULE_HNPINT                           (1 << 2)
#define MODULE_FCINT                            (1 << 1)
#define MODULE_HCINT                            (1)

#define OTG_CORE_CINT_STEN                      (OTG_CORE_BASE+0x08)            //  32bit core int enable reg
#define OTG_CORE_CINT_STEN_CLR                  (OTG_CORE_BASE+0x3c) 		//  32bit core int enable clear reg

#define MODULE_ASHNPINT_EN                      (1 << 5)
#define MODULE_ASFCINT_EN                       (1 << 4)
#define MODULE_ASHCINT_EN                       (1 << 3)
#define MODULE_HNPINT_EN                        (1 << 2)
#define MODULE_FCINT_EN                         (1 << 1)
#define MODULE_HCINT_EN                         (1)

#define OTG_CORE_CLK_CTRL                       (OTG_CORE_BASE+0x0C)            //  32bit core clock control reg

#define MODULE_FUNC_CLK                         (1 << 2)
#define MODULE_HOST_CLK                         (1 << 1)
#define MODULE_MAIN_CLK                         (1)

#define OTG_CORE_RST_CTRL                       (OTG_CORE_BASE+0x10)            // 32bit core reset control reg

#define MODULE_RSTI2C                           (1 << 15)
#define MODULE_RSTCTRL                          (1 << 5)
#define MODULE_RSTFC                            (1 << 4)
#define MODULE_RSTFSIE                          (1 << 3)
#define MODULE_RSTRH                            (1 << 2)
#define MODULE_RSTHSIE                          (1 << 1)
#define MODULE_RSTHC                            (1)

#define OTG_CORE_FRM_INTVL                      (OTG_CORE_BASE+0x14)            //  32bit core frame interval reg

#define MODULE_RESET_FRAME                      (1 << 15)

#define OTG_CORE_FRM_REMAIN                     (OTG_CORE_BASE+0x18)            //  32bit core frame remaining reg

#define OTG_CORE_HNP_CSTAT                      (OTG_CORE_BASE+0x1C)            //  32bit core HNP current state reg

#define MODULE_HNPDAT                           (1 << 30)
#define MODULE_VBUSBSE                          (1 << 29)
#define MODULE_VBUSABSV                         (1 << 28)
#define MODULE_VBUSGTAVV                        (1 << 27)

#define MODULE_ARMTHNPE                         (1 << 25)
#define MODULE_BHNPEN                           (1 << 24)

#define MODULE_SLAVE                            (1 << 22)
#define MODULE_MASTER                           (1 << 21)
#define MODULE_BGEN                             (1 << 20)
#define MODULE_CMPEN                            (1 << 19)
#define MODULE_ISBDEV                           (1 << 18)
#define MODULE_ISADEV                           (1 << 17)

#define MODULE_SWVBUSPUL                        (1 << 15)

#define MODULE_SWAUTORST                        (1 << 12)
#define MODULE_SWPUDP                           (1 << 11)

#define MODULE_SWPDDM                           (1 << 9)
#define MODULE_HNPSTATE                         (0x1f << 8)
#define MODULE_CLRERROR                         (1 << 3)
#define MODULE_ADROPBUS                         (1 << 2)
#define MODULE_ABBUSREQ                         (1 << 1)

#define HNP_A_IDLE                              (0x10 << 8)
#define HNP_A_MASTER                            (0x11 << 8)
#define HNP_A_SLAVE                             (0x12 << 8)
#define HNP_A_WAIT_VPULSE                       (0x13 << 8)

#define HNP_A_WAIT_DPULSE                       (0x14 << 8)
#define HNP_A_WAIT_CONN_A                       (0x15 << 8)
#define HNP_A_WAIT_CONN_B                       (0x16 << 8)
#define HNP_A_WAIT_VRISE                        (0x17 << 8)
#define HNP_A_SUSPEND                           (0x18 << 8)
#define HNP_A_WAIT_VFALL                        (0x19 << 8)
#define HNP_A_VBUS_ERR                          (0x1a << 8)
#define HNP_CONN_DEBOUNCE                       (0x1b << 8)
#define HNP_A_WAIT_ABREQ                        (0x1c << 8)
#define HNP_B_IDLE                              (0x00 << 8)
#define HNP_B_MASTER                            (0x01 << 8)
#define HNP_B_SLAVE                             (0x02 << 8)
#define HNP_B_SRP_INIT_V                        (0x03 << 8)
#define HNP_B_SRP_INIT_D                        (0x04 << 8)
#define HNP_B_PRE_WAIT_CONN_AB_SHORT_DB         (0x05 << 8)
#define HNP_B_WAIT_CONN_A                       (0x06 << 8)
#define HNP_B_PRE_SLAVE                         (0x0a << 8)


#define OTG_CORE_HNP_TIMER1                     (OTG_CORE_BASE+0x20)            //  32bit core HNP timer 1 reg
#define OTG_CORE_HNP_TIMER2                     (OTG_CORE_BASE+0x24)            //  32bit core HNP timer 2 reg
#define OTG_CORE_HNP_T3PCR                      (OTG_CORE_BASE+0x28)            //  32bit core HNP timer 3 pulse ctrl

#define HNP_DATAPULSE                           (1 << 5)
#define HNP_VBUSPULSE                           (1 << 4)

#define OTG_CORE_HINT_STAT                      (OTG_CORE_BASE+0x2C)            //  32bit core HNP int status reg

#define HNP_I2COTGINT                           (1 << 15)
#define HNP_AWAITBTO                            (1 << 8)                        // Hardware HNP Only
#define HNP_AIDLEBDTO                           (1 << 7)                        // Hardware HNP Only
#define HNP_SRPSUCFAIL                          (1 << 6)                        // Hardware HNP Only
#define HNP_SRPINT                              (1 << 5)
#define HNP_VBUSERROR                           (1 << 4)                        // Hardware HNP Only
#define HNP_ABSEVAILD                           (1 << 3)
#define HNP_ABUSVALID                           (1 << 2)
#define HNP_MASSLVCHG                           (1 << 1)                        // Hardware HNP Only
#define HNP_IDCHANGE                            (1)

#define OTG_CORE_HINT_STEN                      (OTG_CORE_BASE+0x30)            //  32bit core HNP int enable reg

#define HNP_I2COTGINT_EN                        (1 << 15)
#define HNP_AWAITBTO_EN                         (1 << 8)
#define HNP_AIDLEBDTO_EN                        (1 << 7)
#define HNP_SRPSUCFAIL_EN                       (1 << 6)
#define HNP_SRPINT_EN                           (1 << 5)
#define HNP_VBUSERROR_EN                        (1 << 4)
#define HNP_ABSEVAILD_EN                        (1 << 3)
#define HNP_ABUSVALID_EN                        (1 << 2)
#define HNP_MASSLVCHG_EN                        (1 << 1)
#define HNP_IDCHANGE_EN                         (1)


#define OTG_CORE_CPUEPSEL_STAT                  (OTG_CORE_BASE+0x34)
#define OTG_CORE_INTERRUPT_STEN                 (OTG_CORE_BASE+0x3c)

/*! @} */


/*!
 * @name C.f. 23.11.11 Host Registers
 */
 /*! @{ */


#define OTG_HOST_CONTROL                        (OTG_HOST_BASE+0x00)     //  32bit host controller config reg

#define HOST_CONTROL_HCRESET                    (1 << 31)
#define HOST_CONTROL_RMTWUEN                    (1 << 4)

#define HOST_CONTROL_HCUSBSTE_RESET             (0 << 2)
#define HOST_CONTROL_HCUSBSTE_RESUME            (1 << 2)
#define HOST_CONTROL_HCUSBSTE_OPERATIONAL       (2 << 2)
#define HOST_CONTROL_HCUSBSTE_SUSPEND           (3 << 2)

#define HOST_CONTROL_CTLBLKSR_11                (0)
#define HOST_CONTROL_CTLBLKSR_21                (1)
#define HOST_CONTROL_CTLBLKSR_41                (2)
#define HOST_CONTROL_CTLBLKSR_81                (3)


#define OTG_HOST_SINT_STAT                      (OTG_HOST_BASE+0x08)     //  32bit host system int status reg

#define HOST_PSCINT                             (1 << 6)                // Port Status Change
#define HOST_FMOFINT                            (1 << 5)                // Frame Number Overflow
#define HOST_HERRINT                            (1 << 4)                // Host Error
#define HOST_RESDETINT                          (1 << 3)                // Resume Detected
#define HOST_SOFINT                             (1 << 2)                // Start of Frame
#define HOST_DONEINT                            (1 << 1)                // Done Register
#define HOST_SORINT                             (1)                     // Schedule Overrun

#define OTG_HOST_SINT_STEN                      (OTG_HOST_BASE+0x0C)     //  32bit host system int enable reg

#define HOST_PSCINT_EN                          (1 << 6)                // Port Status Change
#define HOST_FMOFINT_EN                         (1 << 5)                // Frame Number Overflow
#define HOST_HERRINT_EN                         (1 << 4)                // Host Error
#define HOST_RESDETINT_EN                       (1 << 3)                // Resume Detected
#define HOST_SOFINT_EN                          (1 << 2)                // Start of Frame
#define HOST_DONEINT_EN                         (1 << 1)                // Done Register
#define HOST_SORINT_EN                          (1)                     // Schedule Overrun

#define OTG_HOST_XINT_STAT                      (OTG_HOST_BASE+0x18)     //  32bit host X buf int status reg
#define OTG_HOST_YINT_STAT                      (OTG_HOST_BASE+0x1C)     //  32bit host Y buf int status reg

#define OTG_HOST_XYINT_STEN                     (OTG_HOST_BASE+0x20)     //  32bit host XY buf int enable reg
#define OTG_HOST_XFILL_STAT                     (OTG_HOST_BASE+0x28)     //  32bit host X filled status reg
#define OTG_HOST_YFILL_STAT                     (OTG_HOST_BASE+0x2C)     //  32bit host Y filled status reg

#define OTG_HOST_ETD_EN                         (OTG_HOST_BASE+0x40)     //  32bit host ETD enables reg
#define OTG_HOST_DIR_ROUTE                      (OTG_HOST_BASE+0x48)     //  32bit host direct routing reg
#define OTG_HOST_IINT                           (OTG_HOST_BASE+0x4C)     //  32bit host immediate interrupt reg

#define OTG_HOST_EP_DSTAT                       (OTG_HOST_BASE+0x50)     //  32bit host endpoints done status
#define OTG_HOST_ETD_DONE                       (OTG_HOST_BASE+0x54)     //  32bit host ETD done reg

#define OTG_HOST_FRM_NUM                        (OTG_HOST_BASE+0x60)     //  32bit host frame number reg
#define OTG_HOST_LSP_THRESH                     (OTG_HOST_BASE+0x64)     //  32bit host low speed threshold reg

#define OTG_HOST_ROOTHUB_DESCA                  (OTG_HOST_BASE+0x68)     //  32bit host root hub descriptor A
#define OTG_HOST_ROOTHUB_DESCB                  (OTG_HOST_BASE+0x6C)     //  32bit host root hub descriptor B
/* @} */

/*!
 * @name C.f. 23.11.29
 */
/*! @{ */
#define OTG_HOST_ROOTHUB_STATUS                 (OTG_HOST_BASE+0x70)     //  32bit host root hub status reg

#define ROOTHUB_STATUS_CLRMTWUE                 (1 << 31)               // Clear Remote Wakeup Enable
#define ROOTHUB_STATUS_OVRCURCHG                (1 << 17)               // Over Current Indicator Change
#define ROOTHUB_STATUS_LOCPWRSC                 (1 << 16)               // Local Power Status Change
#define ROOTHUB_STATUS_DEVCONWUE                (1 << 15)               // Device Connect Wakeup Enable
#define ROOTHUB_STATUS_OVRCURI                  (1 << 1)                // Over Current Indicator
#define ROOTHUB_STATUS_LOCPWRS                  (1)                     // Local Power Status
/*! @} */


/*!
 * @name C.f. 23.11.30
 */
/*! @{ */
#define OTG_HOST_PORT_STATUS_1                  (OTG_HOST_BASE+0x74)     //  32bit host port 1 status bits
#define OTG_HOST_PORT_STATUS_2                  (OTG_HOST_BASE+0x78)     //  32bit host port 2 status bits
#define OTG_HOST_PORT_STATUS_3                  (OTG_HOST_BASE+0x7c)     //  32bit host port 3 status bits

#define PORT_STATUS_PRTRSTSC                    (1 << 20)               // Port Reset Status Change
#define PORT_STATUS_OVRCURIC                    (1 << 19)               // Port Over Current Indicator Change
#define PORT_STATUS_PRTSTATSC                   (1 << 18)               // Port Suspend Status Change
#define PORT_STATUS_PRTENBLSC                   (1 << 17)               // Port Enable Status Change
#define PORT_STATUS_CONNECTSC                   (1 << 16)               // Connect Status Change

#define PORT_STATUS_LSDEVCON                    (1 << 9)                // Low Speed Device Attached
#define PORT_STATUS_PRTPWRST                    (1 << 8)                // Port Power Status

#define PORT_STATUS_PRTRSTST                    (1 << 4)                // Port Reset Status
#define PORT_STATUS_PRTOVRCURI                  (1 << 3)                // Port Over Current Indicator
#define PORT_STATUS_PRTSUSPST                   (1 << 2)                // Port Suspend Status
#define PORT_STATUS_PRTENABST                   (1 << 1)                // Port Enable Status
#define PORT_STATUS_CURCONST                    (1)                     // Current Connect Status


/* flags are the same for the interrupt, control and bulk transfer descriptors */
// R1: sec 23.11.10.2,3 pg 23-44,47
#define ETD_GET_COMPCODE(flags)   ((flags >> 12) & 0xf)
// R1: sec 23.11.10 Table 23-23 pg 23-40
#define ETD_CC_NOERROR            0x0
#define ETD_CC_CRCERR             0x1
#define ETD_CC_BITSTUFFERR        0x2
#define ETD_CC_DATATOGERR         0x3
#define ETD_CC_STALL              0x4
#define ETD_CC_DEVNOTRESPONDING   0x5
#define ETD_CC_PIDFAILURE         0x6
// Reserved                       0x7
#define ETD_CC_DATAOVERRUN        0x8
#define ETD_CC_DATAUNDERRUN       0x9
#define ETD_CC_ACK                0xA
#define ETD_CC_NAK                0xB
#define ETD_CC_BUFFEROVERRUN      0xC
#define ETD_CC_BUFFERUNDERRUN     0xD
#define ETD_CC_SCHEDOVERRUN       0xE
#define ETD_CC_NOTACCESSED        0xF

// R1: sec 23.11.10.2,3 pg 23-44,47 (contd)
#define ETD_GET_ERRORCNT(flags)   (((flags) >> 8) & 0xf)
#define ETD_GET_DATATOGL(flags)   (((flags) >> 6) & 0x3)
#define ETD_SET_DATATOGL(v)       (((v) & 0x3) << 6)
#define ETD_GET_DELAYINT(flags)   (((flags) >> 3) & 0x7)
#define ETD_SET_DELAYINT(v)       (((v) & 0x7) << 3)
#define ETD_GET_BUFROUND(flags)   (((flags) >> 2) & 0x1)
#define ETD_SET_BUFROUND(v)       (((v) & 0x1) << 2)
#define ETD_GET_DIRPID(flags)     ((flags) & 0x3)
#define ETD_SET_DIRPID(v)         ((v) & 0x3)
#define ETD_FLIP_DIRPID(flags)    ((flags) ^ 0x3) /* xor with 0x3 flips 2<->1 (IN<->OUT) */
#define ETD_DIRPID_SETUP          0x0  // OUT
#define ETD_DIRPID_OUT            0x1
#define ETD_DIRPID_IN             0x2
#define ETD_DIRPID_RESERVED       0x3

// For non-isoc td.w3.val -
#define ETD_GET_BUFSIZE(w3)       (((w3) >> 21) & 0xfff)
#define ETD_SET_BUFSIZE(v)        (((v) & 0xfff) << 21)
#define ETD_GET_TOTBYECNT(w3)     ((w3) & 0xfffff)
#define ETD_SET_TOTBYECNT(v)      ((v) & 0xfffff)

/* isoc flags shares the COMPCODE and DELAYINT bitfields with the other TDs,
   but the remaining bits are different. */
#define ETD_GET_AUTOISO(flags)    (((flags) >> 11) & 0x1)
#define ETD_SET_AUTOISO(v)        (((v) & 0x1) << 11)
#define ETD_GET_FRAMECNT(flags)   (((flags) >> 8) & 0x1)
#define ETD_SET_FRAMECNT(v)       (((v) & 0x1) << 8)
// For isoc td.w3.val -
/* pkt1 and pkt0 use the COMPCODE bitfield, and add a PKTLEN field. */
#define ETD_GET_PKTLEN(pkt)       ((pkt) & 0x3ff)
#define ETD_SET_PKTLEN(v)         ((v) & 0x3ff)


// ETD endpoint descriptor (etd->epd) bitfield access
// R1: sec 23.11.10.1 pg 23-41 (word0)
#define ETD_GET_SNDNAK(epd)    (((epd) >> 30) & 0x1)
#define ETD_SET_SNDNAK(v)      (((v) & 0x1) << 30)
#define ETD_GET_TOGCRY(epd)    (((epd) >> 28) & 0x1)
#define ETD_SET_TOGCRY(v)      (((v) & 0x1) << 28)
#define ETD_GET_HALTED(epd)    (((epd) >> 27) & 0x1)
#define ETD_SET_HALTED(v)      (((v) & 0x1) << 27)
#define ETD_GET_MAXPKTSIZ(epd) (((epd) >> 16) & 0x3ff)
#define ETD_SET_MAXPKTSIZ(v)   (((v) & 0x3ff) << 16)
#define ETD_GET_FORMAT(epd)    (((epd) >> 14) & 0x3)
#define ETD_SET_FORMAT(v)      (((v) & 0x3) << 14)
#define ETD_FORMAT_CONTROL     0x0
#define ETD_FORMAT_ISOC        0x1
#define ETD_FORMAT_BULK        0x2
#define ETD_FORMAT_INTERRUPT   0x3
#define ETD_GET_SPEED(epd)     (((epd) >> 13) & 0x1)
#define ETD_SET_SPEED(v)       (((v) & 0x1) << 13)
#define ETD_SPEED_FULL         0x0
#define ETD_SPEED_LOW          0x1
#define ETD_GET_DIRECT(epd)    (((epd) >> 11) & 0x3)
#define ETD_SET_DIRECT(v)      (((v) & 0x3) << 11)
/* xor with 3 flips 2<->1 */
#define ETD_FLIP_DIRECT(epd)   ((epd) ^ (0x3 << 11))
#define ETD_DIRECT_TD00        0x0
#define ETD_DIRECT_OUT         0x1
#define ETD_DIRECT_IN          0x2
#define ETD_DIRECT_TD11        0x3
#define ETD_GET_ENDPNT(epd)    (((epd) >> 7) & 0xf)
#define ETD_SET_ENDPNT(v)      (((v) & 0xf) << 7)
#define ETD_GET_ADDRESS(epd)   ((epd) & 0x7f)
#define ETD_SET_ADDRESS(v)     ((v) & 0x7f)

#if 0
// etd_urb_state[] values
#define ETD_URB_COMPLETED       0
#define ETD_URB_SETUP_STATUS    1
#define ETD_URB_SETUP_DATA      2
#define ETD_URB_SETUP_START     3
#define ETD_URB_BULK_START      4
#define ETD_URB_BULKWZLP        5
#define ETD_URB_BULKWZLP_START  6
#define ETD_URB_INTERRUPT_START 7
#define ETD_URB_ISOC_START      8
#endif
/*! @} */


/*!
 * @name C.f. 23.12.8 Function Registers
 */
/*! @{ */

#define OTG_FUNC_CMD_STAT                       (OTG_FUNC_BASE+0x00)            //  32bit func command status reg

#define COMMAND_SOFTRESET                       (1 << 7)
#define COMMAND_BADISOAP                        (1 << 3)
#define COMMAND_SUPDET                          (1 << 2)
#define COMMAND_RSMINPROG                       (1 << 1)
#define COMMAND_RESETDET                        (1)


#define OTG_FUNC_DEV_ADDR                       (OTG_FUNC_BASE+0x04)            //  32bit func device address reg

#define OTG_FUNC_SINT_STAT                      (OTG_FUNC_BASE+0x08)            //  32bit func system int status reg

#define SYSTEM_DONEREGINTDS                     (1 << 5)
#define SYSTEM_SOFDETINT                        (1 << 4)
#define SYSTEM_DONEREGINT                       (1 << 3)
#define SYSTEM_SUSPDETINT                       (1 << 2)
#define SYSTEM_RSMFININT                        (1 << 1)
#define SYSTEM_RESETINT                         (1)

#define OTG_FUNC_SINT_STEN                      (OTG_FUNC_BASE+0x0C)            //  32bit func system int enable reg

#define OTG_FUNC_SINT_STEN_CLR                  (OTG_FUNC_BASE+0x10C)            //  32bit func system int enable clear reg

#define SYSTEM_DONEREGINTDS_EN                  (1 << 5)
#define SYSTEM_SOFDETINT_EN                     (1 << 4)
#define SYSTEM_DONEREGINT_EN                    (1 << 3)
#define SYSTEM_SUSPDETINT_EN                    (1 << 2)
#define SYSTEM_RSMFININT_EN                     (1 << 1)
#define SYSTEM_RESETINT_EN                      (1)


#define OTG_FUNC_XINT_STAT                      (OTG_FUNC_BASE+0x10)            //  32bit func X buf int status reg
#define OTG_FUNC_YINT_STAT                      (OTG_FUNC_BASE+0x14)            //  32bit func Y buf int status reg

#define OTG_FUNC_XYINT_STEN                     (OTG_FUNC_BASE+0x18)            //  32bit func XY buf int enable reg
#define OTG_FUNC_XYINT_STEN_CLR                 (OTG_FUNC_BASE+0x118)            //  32bit func XY buf int enable clear reg

#define OTG_FUNC_XFILL_STAT                     (OTG_FUNC_BASE+0x1C)            //  32bit func X filled status reg
#define OTG_FUNC_YFILL_STAT                     (OTG_FUNC_BASE+0x20)            //  32bit func Y filled status reg

#define OTG_FUNC_EP_EN                          (OTG_FUNC_BASE+0x24)            //  32bit func endpoints enable reg
#define OTG_FUNC_EP_EN_CLR                          (OTG_FUNC_BASE+0x124)            //  32bit func endpoints enable clear reg

#define OTG_FUNC_EP_RDY                         (OTG_FUNC_BASE+0x28)            //  32bit func endpoints ready reg
#define OTG_FUNC_EP_RDY_CLR                     (OTG_FUNC_BASE+0x3C)            //  32bit func endpoints ready clear reg

#define OTG_FUNC_IINT                           (OTG_FUNC_BASE+0x2C)            //  32bit func immediate interrupt reg
#define OTG_FUNC_IINT_CLR                       (OTG_FUNC_BASE+0x12C)           //  32bit func immediate interrupt clear reg

#define OTG_FUNC_EP_DSTAT                       (OTG_FUNC_BASE+0x30)            //  32bit func endpoints done status

#define OTG_FUNC_EP_DEN                         (OTG_FUNC_BASE+0x34)            //  32bit func endpoints done enable
#define OTG_FUNC_EP_DEN_CLR                     (OTG_FUNC_BASE+0x134)            //  32bit func endpoints done clear enable

#define OTG_FUNC_EP_TOGGLE                      (OTG_FUNC_BASE+0x38)            //  32bit func endpoints toggle bits
#define OTG_FUNC_FRM_NUM                        (OTG_FUNC_BASE+0x3C)            //  32bit func frame number reg






#define EP0_STALL                               (1 << 31)
#define EP0_SETUP                               (1 << 30)
#define EP0_OVERRUN                             (1 << 29)
#define EP0_AUTOISO                             (1 << 27)

#define EP_FORMAT_CONTROL     0x0
#define EP_FORMAT_ISOC        0x1
#define EP_FORMAT_BULK        0x2
#define EP_FORMAT_INTERRUPT   0x3

/* generic endpoint register manipulations
 */
#define EP_OUT                                  0x1
#define EP_IN                                   0x2
#define EP_BOTH                                 0x3

#define ETD_MASK(n)                             (1 << n)
#define ep_num_both(n)                          (EP_BOTH << n)
#define ep_num_dir(n, dir)                      ((dir ? EP_IN : EP_OUT) << (n*2))
#define ep_num_out(n)                           ep_num_dir(n, USB_DIR_OUT)
#define ep_num_in(n)                            ep_num_dir(n, USB_DIR_IN)


/*! @} */

/*!
 * @name C.f. 23.13.3 DMA Registers
 */
/*! @{ */
#define OTG_DMA_REV_NUM                         (OTG_DMA_BASE+0x000)     //  32bit dma revision number reg
#define OTG_DMA_DINT_STAT                       (OTG_DMA_BASE+0x004)     //  32bit dma int status reg
#define OTG_DMA_DINT_STEN                       (OTG_DMA_BASE+0x008)     //  32bit dma int enable reg
#define OTG_DMA_ETD_ERR                         (OTG_DMA_BASE+0x00C)     //  32bit dma ETD error status reg
#define OTG_DMA_EP_ERR                          (OTG_DMA_BASE+0x010)     //  32bit dma EP error status reg
#define OTG_DMA_ETD_EN                          (OTG_DMA_BASE+0x020)     //  32bit dma ETD DMA enable reg
#define OTG_DMA_EP_EN                           (OTG_DMA_BASE+0x024)     //  32bit dma EP DMA enable reg
#define OTG_DMA_ETD_ENXREQ                      (OTG_DMA_BASE+0x028)     //  32bit dma ETD DMA enable Xtrig req
#define OTG_DMA_EP_ENXREQ                       (OTG_DMA_BASE+0x02C)     //  32bit dma EP DMA enable Ytrig req
#define OTG_DMA_ETD_ENXYREQ                     (OTG_DMA_BASE+0x030)     //  32bit dma ETD DMA enble XYtrig req
#define OTG_DMA_EP_ENXYREQ                      (OTG_DMA_BASE+0x034)     //  32bit dma EP DMA enable XYtrig req
#define OTG_DMA_ETD_BURST4                      (OTG_DMA_BASE+0x038)     //  32bit dma ETD DMA enble burst4 reg
#define OTG_DMA_EP_BURST4                       (OTG_DMA_BASE+0x03C)     //  32bit dma EP DMA enable burst4 reg

#define OTG_DMA_MISC_CTRL                       (OTG_DMA_BASE+0x040)     //  32bit dma EP misc control reg
#define OTG_DMA_MISC_ARBMODE                            (1 << 1)


#define OTG_DMA_ETD_CH_CLR                      (OTG_DMA_BASE+0x048)     //  32bit dma ETD clear channel reg
#define OTG_DMA_EP_CH_CLR                       (OTG_DMA_BASE+0x04c)     //  32bit dma EP clear channel reg

#if 0
#define OTG_DMA_ETD_CH_CLR                      (OTG_DMA_BASE+0x044)     //  32bit dma ETD clear channel reg
#define OTG_DMA_ETD_ERR                         (OTG_DMA_BASE+0x00C)     //  32bit dma ETD error status reg
#define OTG_DMA_ETD_EN                          (OTG_DMA_BASE+0x020)     //  32bit dma ETD DMA enable reg
#define OTG_DMA_ETD_ENXREQ                      (OTG_DMA_BASE+0x028)     //  32bit dma ETD DMA enable Xtrig req
#define OTG_DMA_ETD_ENXYREQ                     (OTG_DMA_BASE+0x030)     //  32bit dma ETD DMA enble XYtrig req
#define OTG_DMA_ETD_BURST4                      (OTG_DMA_BASE+0x038)     //  32bit dma ETD DMA enble burst4 reg



#define OTG_DMA_EP_CH_CLR                       (OTG_DMA_BASE+0x048)     //  32bit dma EP clear channel reg
#define OTG_DMA_EP_ERR                          (OTG_DMA_BASE+0x010)     //  32bit dma EP error status reg
#define OTG_DMA_EP_EN                           (OTG_DMA_BASE+0x024)     //  32bit dma EP DMA enable reg
#define OTG_DMA_EP_ENXREQ                       (OTG_DMA_BASE+0x02C)     //  32bit dma EP DMA enable Ytrig req
#define OTG_DMA_EP_ENXYREQ                      (OTG_DMA_BASE+0x034)     //  32bit dma EP DMA enable XYtrig req
#define OTG_DMA_ETD_BURST4                      (OTG_DMA_BASE+0x038)     //  32bit dma ETD DMA enable burst4 reg
#define OTG_DMA_EP_BURST4                       (OTG_DMA_BASE+0x03C)     //  32bit dma EP DMA enable burst4 reg

#endif


#define dma_num_dir(n, dir) (n * 2 + (dir ? 1 : 0))
#define dma_num_out(n) dma_num_dir(n, USB_DIR_OUT)
#define dma_num_in(n) dma_num_dir(n, USB_DIR_IN)

#define OTG_DMA_ETD_MSA(x)                      (OTG_DMA_BASE+0x100+x*4)
#define OTG_DMA_EPN_MSA(x)                      (OTG_DMA_BASE+0x180+x*4)
#define OTG_DMA_ETDN_BPTR(x)                    (OTG_DMA_BASE+0x280+x*4)
#define OTG_DMA_EPN_BPTR(x)                     (OTG_DMA_BASE+0x284+x*4)

/*! @} */

/* ********************************************************************************************** */
/*!
 * Warning: the MX21 memory mapped region appears to require 32-bit access only.
 * As of this writing, only the ETD region has been tested, but it does show the
 * following behavior:
 *
 * Step 1:   *(volatile u32 *)(void*)0xe40243f8  == 00000000
 * Step 2:   *(volatile u16 *)(void*)0xe40243f8 = 0x0084;
 * Step 3:   *(volatile u32 *)(void*)0xe40243f8  == 00840084
 * Step 4:   *(volatile u8 *)(void*)(0xe40243f8+3) = 0x01;
 * Step 5:   *(volatile u32 *)(void*)0xe40243f8  == 01010101
 *
 * Any access to less than 32 bits is replicated as many times as required
 * to make 32 bits, and the surrounding 32-bit region is changed.
 *
 * Just to add insult to injury, gcc 3.2.3 wants to store constants
 * that are small enough using byte instructions.  E.g.
 *
 *      mm->host.System_Interrupt_Enables = (SIEN_PSCIEN | SIEN_FMOFIEN | SIEN_HERRIEN | SIEN_RESDETIEN |
 *                                         SIEN_DONEIEN | SIEN_SORIEN);
 *
 * becomes:
 *
 *      ldrb    r2, [r3, #140]
 *      mov     r2, #123
 *      strb    r2, [r3, #140]
 *      ldrb    r2, [r3, #141]
 *      strb    r1, [r3, #141]
 *      ldrb    r2, [r3, #142]
 *      strb    r1, [r3, #142]
 *      ldrb    r2, [r3, #143]
 *      strb    r1, [r3, #143]
 *
 * which, given the 32-bit access restrictions above,
 * is not going to work very well.  Changing the original assignment to:
 *
 *      *&(mm->host.System_Interrupt_Enables) = (SIEN_PSCIEN | SIEN_FMOFIEN | SIEN_HERRIEN | SIEN_RESDETIEN |
 *                                           SIEN_DONEIEN | SIEN_SORIEN);
 *
 * which should be identical, from a language definition point of view,
 * produces:
 *
 *      mov     r2, #123
 *      str     r2, [r3, #140]
 *
 */


/*!
 * @name TransferDescriptors
 *
 * Note that the MX21 needs to run in little-endian mode for OTG, but
 * since the memory can only be accessed in 32-bit quantities, anything
 * smaller will be built in regular memory, then transfered one word at
 * a time to the memory-mapped registers, and that word access will flip
 * the order of bytes.  u16 quantities survive this flipping because they
 * get "pre-flipped" in regular memory.
 *
 *
 * An Endpoint Transfer Descriptor (ETD) consists of 4x32bit words.
 * The first word is the Endpoint Descriptor, the last three are
 * the transfer descriptor.  Transfer descriptors come in 3 formats,
 * CONTROL/BULK, INTERRUPT, and ISOCRONOUS.  Becasue of the memory
 * access restrictions to less than 32 bits described above, only
 * the words are mapped directly,  The smaller fields are assembled
 * into 32-bit words before being placed into the live ETD.
 *
 */
 /*! @{ */

/*!
 * create a type for transfer_descriptor_w1
 * @brief  typedef struct transfer_descriptor_w1 transfer_descriptor_w1
 *All three types of TD have the same format for w1.
 */
typedef struct transfer_descriptor_w1 {
#if defined(LITTLE_ENDIAN)
        u16 x;
        u16 y;
#else
        u16 y;
        u16 x;
#endif
} __attribute__ ((packed)) volatile transfer_descriptor_w1;

/*!
 * create a type for control_bulk_transfer_descriptor_w2
 * @brief  typedef struct control_bulk_transfer_descriptor_w2 control_bulk_transfer_descriptor_w2
 * There are 3 different formats for w2.
 * C.f. R1: sec 23.11.10.2 pg 23-43
 */
typedef struct control_bulk_transfer_descriptor_w2 {
#if defined(LITTLE_ENDIAN)
        u8 rtrydelay;
        u8 reserved;
        u16 flags;
#else
        u16 flags;
        u8 reserved;
        u8 rtrydelay;
#endif
} __attribute__ ((packed)) volatile control_bulk_transfer_descriptor_w2;

/*!
 *create an alias for interrupt_transfer_descriptor_w2
 *  @brief typedef struct  interrupt_transfer_descriptor_w2  interrupt_transfer_descriptor_w2
  * C.f. R1: sec 23.11.10.3 pg 23-46
  * C.f. R1: sec 23.11.10.3 pg 23-46
  */
typedef struct interrupt_transfer_descriptor_w2 {
#if defined(LITTLE_ENDIAN)
        u8 polinterv;
        u8 relpolpos;
        u16 flags;
#else
        u16 flags;
        u8 relpolpos;
        u8 polinterv;
#endif
} __attribute__ ((packed)) volatile interrupt_transfer_descriptor_w2;

/*!
 * create a type for isoc_transfer_descriptor_w2
 *  @brief  typedef struct isoc_transfer_descriptor_w2  isoc_transfer_descriptor * C.f. R1: sec 23.11.10.4 pg 23-48
 */
typedef struct isoc_transfer_descriptor_w2 {
#if defined(LITTLE_ENDIAN)
        u16 startfrm;
        u16 flags;
#else
        u16 flags;
        u16 startfrm;
#endif
} __attribute__ ((packed)) volatile isoc_transfer_descriptor_w2;



/*!
 * create a type for isoc_transfer_descriptor_w3
 *  @brief  typedef struct isoc_transfer_descriptor_w3  isoc_transfer_descriptor
 *
 *  There are 2 different formats for w3.
 * All but isoc use a packet size and 20-bit bytecount, accessed through w3.val.
 */
typedef struct isoc_transfer_descriptor_w3 {
#if defined(LITTLE_ENDIAN)
        u16 pkt0;
        u16 pkt1;
#else
        u16 pkt1;
        u16 pkt0;
#endif
} __attribute__ ((packed)) volatile isoc_transfer_descriptor_w3;

/*!
 * @name transfer_descriptor
 */
/*! @{ */
/*! create an alias for transfer_descriptor
 *  @brief typedef  struct transfer_descriptor transfer_descriptor
 */
typedef struct transfer_descriptor {
        union {
                u32 val;
                transfer_descriptor_w1 bufsrtad;
        } volatile w1;
        union {
                u32 val;
                control_bulk_transfer_descriptor_w2 cb;
                interrupt_transfer_descriptor_w2 intr;
                isoc_transfer_descriptor_w2 isoc;
        } volatile w2;
        union {
                u32 val;
                isoc_transfer_descriptor_w3 isoc;
        } volatile w3;
} __attribute__ ((packed)) volatile transfer_descriptor;

/*! @} */

/*!
 * @name endpoint_transfer_descriptor
 * C.f. R1: sec 23.11.10 pg 23-41
 */
 /*! @{ */
/*! create a type for endpoint_transfer_descriptor
 *  @brief typedef struct endpoint_transfer_descriptor  endpoint_transfer_descriptor
 */
typedef struct endpoint_transfer_descriptor {
        u32 epd;
        transfer_descriptor td;
} __attribute__ ((packed)) volatile endpoint_transfer_descriptor;  // ETD

/*! @} */
/*!
 * fs_host_port_stat() - get host start port address for ports 1-3
 * @param n
 * @returns port address
 */
static u32 inline fs_host_port_stat(int n)
{
        switch (n) {
        default:
        case 1:
                return OTG_HOST_PORT_STATUS_1;
        case 2:
                return OTG_HOST_PORT_STATUS_2;
        case 3:
                return OTG_HOST_PORT_STATUS_3;
        }
}

/*! @name clock switch functions */
/* ********************************************************************************************** */

void fs_func_clock_on(void);
void fs_host_clock_on(void);
void fs_func_clock_off(void);
void fs_host_clock_off(void);
void fs_main_clock_on(void);
void fs_main_clock_off(void);
void fs_set_transceiver_mode(int mode);





/*! @} */
