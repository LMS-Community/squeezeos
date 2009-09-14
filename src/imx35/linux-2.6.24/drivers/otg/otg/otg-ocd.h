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
 * otg/otg-ocd.h - OTG Controller Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-ocd.h|20061218212925|27282
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
 * @file otg/otg/otg-ocd.h
 * @brief Defines common to On-The-Go OTG Controller Support
 *
 * This file defines the ocd_ops and ocd_instance structures.
 *
 * The ocd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The ocd_instance structure is used to maintain the global data
 * required by the OTG controller drivers.
 *
 * @ingroup OTGAPI
 * @ingroup OCD
 */

/*!
 * @name OCD OTG Controller Driver
 * @{
 */
/*! @struct ocd_instance otg-ocd.h  "otg/otg-ocd.h"
 */

struct ocd_instance {
        struct otg_instance *otg;
        otg_tag_t TAG;
        void * privdata;
};


typedef int (*otg_timer_callback_proc_t) (void *);

#define OCD_CAPABILITIES_DR          1 << 0
#define OCD_CAPABILITIES_PO          1 << 1
#define OCD_CAPABILITIES_TR          1 << 2
#define OCD_CAPABILITIES_HOST        1 << 3

#define OCD_CAPABILITIES_AUTO        1 << 4


/*!
 * @struct ocd_ops
 * The ocd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct ocd_ops {

        u32 capabilities;                               /* OCD Capabilities */

        /* Driver Initialization - by degrees
         */
        int (*mod_init) (struct otg_instance *);                         /*!< OCD Module Initialization */
        void (*mod_exit) (struct otg_instance *);                        /*!< OCD Module Exit */

        otg_output_proc_t       ocd_init_func;          /*!< OTG calls to initialize or de-initialize the OCD */

        int (*start_timer) (struct otg_instance *, int);/*!< called by OTG to start timer */
        otg_tick_t (*ticks) (void);         /*!< called by OTG to  fetch current ticks, typically micro-seconds when available */
        otg_tick_t (*elapsed) ( otg_tick_t *, otg_tick_t *);
                                        /*!< called by OTG to get micro-seconds elapsed between two ticks */
        //u32 interrupts;                                 /*!< called by OTG to get number of interrupts */
        //

        void                    *privdata;
};


#if 0
struct ocd_instance {
        struct otg_instance *otg;
        void * privdata;
};
#endif

#ifndef OTG_APPLICATION

#define REMOVE_OCD ocd_trace_tag
extern otg_tag_t REMOVE_OCD;
extern struct ocd_instance *REMOVE_ocd_instance;
extern struct ocd_ops ocd_ops;
#endif
/* @} */
