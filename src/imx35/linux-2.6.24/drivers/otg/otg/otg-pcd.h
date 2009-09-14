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
 * otg/otg-pcd.h - OTG Peripheral Controller Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-pcd.h|20070819221238|02897
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
 * @file otg/otg/otg-pcd.h
 * @brief Defines common to On-The-Go Peripheral Controller Support
 *
 * This file defines the pcd_ops and pcd_instance structures.
 *
 * The pcd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The pcd_instance structure is used to maintain the global data
 * required by the peripheral controller drivers.
 *
 * @ingroup OTGAPI
 * @ingroup PCD
 */

/*!
 * @name PCD Peripheral Controller Driver
 * @{
 */

/*!
 * @struct pcd_ops  otg-pcd.h  "otg/otg-pcd.h"
 *
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct pcd_ops {

        int (*mod_init) (struct otg_instance *);                         /*!< PCD Module Initialization */
        void (*mod_exit) (struct otg_instance *);                        /*!< PCD Module Exit */

        otg_output_proc_t       pcd_init_func;          /*!< OTG calls to initialize or de-initialize the PCD */
        otg_output_proc_t       pcd_en_func;            /*!< OTG calls to enable or disable the PCD */
        otg_output_proc_t       remote_wakeup_func;     /*!< OTG calls to have PCD perform remote wakeup */

        framenum_t              framenum;               /*!< OTG calls to get current USB Frame number */

        otg_output_proc_t       tcd_en_func;            /*!< OTG calls to enable or disable the TCD */
        otg_output_proc_t       dp_pullup_func;         /*!< OTG calls to enable or disable D+ pullup (aka loc_conn) */
        otg_tick_t              (*ticks) (void);        /*!< called by OTG to get ticks, typically micro-seconds when available */
        otg_tick_t              (*elapsed) ( otg_tick_t *, otg_tick_t *);


        u8                      max_endpoints;
        //u32                     ep_in_mask;
        //u32                     ep_out_mask;
};

#define REMOVE_PCD pcd_trace_tag
extern otg_tag_t REMOVE_PCD;
extern struct pcd_instance *REMOVE_pcd_instance;
extern struct pcd_ops pcd_ops;

/*!
 * @struct pcd_instance  otg-pcd.h  "otg/otg-pcd.h"
 */
struct pcd_instance {
        struct otg_instance *otg;                       /*!< pointer to OTG Instance */
        otg_tag_t TAG;
        struct usbd_bus_instance *bus;                  /*!< pointer to usb bus instance */
        void * privdata;                                /*!< pointer to private data for PCD */
        int pcd_exiting;                                /*!< non-zero if OTG is unloading */
        //struct WORK_STRUCT bh;                        /*!< work structure for bottom half handler */
        struct otg_workitem *register_bh;               /*!< work structure for bottom half handler */
        struct otg_workitem *deregister_bh;             /*!< work structure for bottom half handler */
        struct otg_task *task;
        otg_pthread_mutex_t mutex;
        int active;
        u8 address;
        u8 new_address;
};


#if !defined(OTG_C99)
extern void pcd_global_init(void);
#endif /* !defined(OTG_C99) */
extern void pcd_init_func(struct otg_instance *, u8 );
extern void pcd_en_func(struct otg_instance *, u8 );
extern void pcd_remote_wakeup(struct otg_instance *, u8 );
extern void pcd_tcd_en_func(struct otg_instance *, u8 );
extern void pcd_dp_pullup_func(struct otg_instance *, u8 );
extern u16 pcd_framenum(struct otg_instance *);
extern otg_tick_t pcd_ticks(void);
extern otg_tick_t pcd_elapsed(otg_tick_t *, otg_tick_t *);

extern int pcd_request_endpoints(struct pcd_instance *, struct usbd_endpoint_map *, int, struct usbd_endpoint_request *);

extern struct pcd_ops pcd_ops;


/* @} */
