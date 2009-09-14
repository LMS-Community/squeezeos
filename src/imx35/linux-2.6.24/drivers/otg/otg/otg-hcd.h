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
 * otg/otg-hcd.h - OTG Host Controller Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-hcd.h|20061218212925|63115
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */
/*!
 * @file otg/otg/otg-hcd.h
 * @brief Defines common to On-The-Go Host Controller Support
 *
 * This file defines the hcd_ops and hcd_instance structures.
 *
 * The hcd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The hcd_instance structure is used to maintain the global data
 * required by the host controller drivers.
 *
 * @ingroup OTGAPI
 * @ingroup HCD
 */

/*!
 * @name HCD Host Controller Driver
 * @{
 */
struct hcd_instance;

/*!
 * @struct hcd_ops
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct hcd_ops {

        /* Driver Initialization - by degrees
         */
        int (*mod_init) (struct otg_instance *);                         /*!< HCD Module Initialization */
        void (*mod_exit) (struct otg_instance *);                        /*!< HCD Module Exit */


        /* mandatory */
        int max_ports;                                  /*!< maximum number of ports available */
        u32 capabilities;                               /*!< UDC Capabilities - see usbd-bus.h for details */
        char *name;                                     /*!< name of controller */


        otg_output_proc_t       hcd_init_func;          /*!< OTG calls to initialize or de-initialize the HCD */
        otg_output_proc_t       hcd_en_func;            /*!< OTG calls to enable or disable the HCD */
        otg_output_proc_t       hcd_rh_func;            /*!< OTG calls to enable HCD Root Hub */
        otg_output_proc_t       loc_sof_func;           /*!< OTG calls to into a_host or b_host state - attempt to use port */
        otg_output_proc_t       loc_suspend_func;       /*!< OTG calls to suspend bus */
        otg_output_proc_t       remote_wakeup_en_func;  /*!< OTG calls to issue SET FEATURE REMOTE WAKEUP */
        otg_output_proc_t       hnp_en_func;            /*!< OTG calls to issues SET FEATURE B_HNP_ENABLE */

        framenum_t		framenum;               /*!< OTG calls to get current USB Frame number */
};

/*!
 * @struct hcd_instance otg-hcd.h  "otg/otg-hcd.h"
 */
struct hcd_instance {
        struct otg_instance *otg;                       /*!< pointer to OTG Instance */
        otg_tag_t TAG;
        void * privdata;                                /*!< pointer to private data for PCD */
        //struct WORK_STRUCT bh;                          /*!< work structure for bottom half handler */
        int active;
};

#define HCD hcd_trace_tag
extern otg_tag_t HCD;
extern struct hcd_ops hcd_ops;
extern struct hcd_instance *hcd_instance;
#if !defined(OTG_C99)
extern void fs_hcd_global_init(void);
#endif /* !defined(OTG_C99) */
extern void hcd_init_func(struct otg_instance *, u8 );
extern void hcd_en_func(struct otg_instance *, u8 );

/* @} */
