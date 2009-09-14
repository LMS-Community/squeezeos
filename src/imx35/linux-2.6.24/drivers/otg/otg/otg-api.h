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
 * otg/otg-api.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-api.h|20070125083535|47004
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*
 * Doxygen group definitions - N.B. These much each be in
 * their own separate comment...
 */

/*!
 * @defgroup FunctionDrivers USB Function Drivers
 * @brief These are the USB Function Drivers for Belcarra USBOTG.
 */
    /*!
     * @defgroup CompositeFunctions Composite USB Functions
     * @ingroup FunctionDrivers
     * @brief These are the USB Composite Function Drivers for Belcarra USBOTG.
     */
    /*!
     * @defgroup InterfaceFunctions Interface USB Functions
     * @ingroup FunctionDrivers
     * @brief These are the USB Interface Function Drivers for Belcarra USBOTG.
     */
    /*!
     * @defgroup ClassFunctions Class USB Functions
     * @ingroup FunctionDrivers
     * @brief These are the USB Class Function Drivers for Belcarra USBOTG.
     */
    /*!
     * @defgroup SimpleFunctions Simple USB Functions
     * @ingroup FunctionDrivers
     * @brief These are the USB Simple Function Drivers for Belcarra USBOTG.
     */


/*!
 * @defgroup USBDOTG USBOTG
 */
    /*!
     * @defgroup USBDAPI USBD Function Driver API
     * @ingroup USBDOTG
     * @brief This is the USBD API.
     */
    /*!
     * @defgroup OTGAPI OTG Driver API
     * @ingroup USBDOTG
     * @brief This is the OTG State Machine API.
     */
    /*!
     * @defgroup USBDCORE USBD Core
     * @ingroup USBDOTG
     * @brief This implements the USBD API.
     */
    /*!
     * @defgroup OTGCORE OTG State Machine
     * @ingroup USBDOTG
     * @brief This implements the OTG State Machine API.
     */
    /*!
     * @defgroup OTGINIT OTG Core Init
     * @ingroup USBDOTG
     * @brief This implements the OTG initialization.
     */
    /*!
     * @defgroup OTGMESG OTG Core Mesg
     * @ingroup USBDOTG
     * @brief This implements the OTG messaging facility.
     */
    /*!
     * @defgroup OTGTRACE OTG Core Trace
     * @ingroup USBDOTG
     * @brief This implements the OTG trace facility.
     */
#if 0
    /*!
     * @defgroup OTGFW OTG Firmware
     * @ingroup USBDOTG
     * @brief This implements the OTG State Machine Firmware.
     */
#endif
/*!
 * @defgroup OSPort OS Support
 */
    /*!
     * @defgroup OSAPI OTG OS Support API
     * @ingroup OSPort
     * @brief This defines the OTG OS API.
     */
    /*!
     * @defgroup LINUXAPI Linux OTG OS Support API
     * @ingroup OSPort
     * @brief This defines the Linux version of the OTG OS API.
     */
    /*!
     * @defgroup LINUXOS Linux OTG OS Support
     * @ingroup OSPort
     * @brief This implements the Linux OTG OS API.
     */

/*!
 * @defgroup Platform Platform
 */
/*!
 * @defgroup Hardware Platform Specific Hardware
 * @ingroup Platform
 * @brief The Platform specific hardware drivers.
 */
    /*!
     * @defgroup PCD Peripheral Controller Driver
     * @ingroup Platform
     * @brief The Peripheral Controller Driver controls the USB Device Controller hardware.
     */
    /*!
     * @defgroup TCD Transceiver Controller Driver
     * @ingroup Platform
     * @brief The Transceiver Controller Driver controls the USB and/or OTG Transceiver Controller hardware.
     */
    /*!
     * @defgroup HCD Host Controller Driver
     * @ingroup Platform
     * @brief The Host Controller Driver controls the USB Host Controller hardware.
     */
    /*!
     * @defgroup OCD OTG Controller Driver
     * @ingroup Platform
     * @brief The OTG Controller Driver controls the OTG Transceiver Controller hardware that is common to
     * both PCD and HCD.
     */


/*!
 * @file otg/otg/otg-api.h
 * @brief Core Defines for USB OTG Core Layaer
 *
 * @ingroup OTGAPI
 */




/*!
 * @name OTGCORE OTG API Definitions
 * This contains the OTG API structures and definitions.
 * @{
 */

#include <otg/otg-fw.h>

#if defined(CONFIG_OTG_USB_PERIPHERAL) || defined (CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)
#include <otg/otg-fw-mn.h>
#elif defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE)
#include <otg/otg-fw-df.h>
#else /* error */
//#abort "Missing USB or OTG configuration"
#endif


struct otg_instance;
typedef void (*otg_output_proc_t) (struct otg_instance *, u8);

extern struct otg_firmware *otg_firmware_loaded;
extern struct otg_firmware *otg_firmware_orig;
extern struct otg_firmware *otg_firmware_loading;


char * otg_get_state_name(int state);

typedef u64 otg_current_t;

/*!
 * @struct otg_instance otg-api.h "otg/otg-api.h"
 *
 * This tracks the current OTG configuration and state
 */
struct otg_instance {

        u16                     state;                  /*!< current state      */
        int                     previous;               /*!< previous state     */

        int                     active;                 /*!< used as semaphore  */

        u32                     current_inputs;         /*!< input settings     */
        otg_current_t           outputs;                /*!< output settings    */

        otg_tick_t              tickcount;              /*!< when we transitioned to current state      */
        otg_current_t           bconn;                  /*!< when b_conn was set        */

        struct otg_state       *current_outputs;       /*!< output table entry for current state        */
        struct otg_state       *previous_outputs;      /*!< output table entry for current state        */

        otg_output_proc_t       otg_output_ops[MAX_OUTPUTS]; /*!< array of functions mapped to output numbers */

        int                     (*start_timer) (struct otg_instance *, int); /*!< OCD function to start timer */
        otg_tick_t              (*ticks) (void);        /*!< OCD function to return ticks */
        otg_tick_t              (*elapsed) ( otg_tick_t *, otg_tick_t *); /*!< OCD function to return elapsed ticks */

        struct hcd_instance*    hcd;                    /*!< pointer to hcd instance */
        struct ocd_instance*    ocd;                    /*!< pointer to ocd instance */
        struct pcd_instance*    pcd;                    /*!< pointer to pcd instance */
        struct tcd_instance*    tcd;                    /*!< pointer to tcd instance */

        struct hcd_ops*         hcd_ops;
        struct ocd_ops*         ocd_ops;
        struct pcd_ops*         pcd_ops;
        struct tcd_ops*         tcd_ops;


        char                    function_name[OTGADMIN_MAXSTR]; /*!< current function */
        char                    serial_number[OTGADMIN_MAXSTR]; /*!< current serial number */

        u32 interrupts;                                 /*!< track number of interrupts */

        //#if 0
        struct otg_tasklet      *tasklet;
        //#else
        struct otg_task         *task;
        //otg_sem_t               event;
        //#endif
        otg_sem_t               command;
        struct otg_task         *message_task;

        otg_pthread_mutex_t     mutex;

        otg_tag_t               TAG;

        void                    *privdata;
};

typedef int (*otg_event_t) (struct otg_instance *, int, char *);
/*!
 * @struct otg_event_info  otg-api.h "otg/otg-api.h"
 */

struct otg_event_info {
        char *name;              /*< otg event name */
        otg_event_t event;       /*< otg event      */
};

//typedef u16 (*framenum_t)(struct otg_instance *);
typedef u16 (*framenum_t)(struct otg_instance *otg);

/* 1 - used by external functions to pass in administrative commands as simple strings
 */
extern int otg_status(int, u32, u32, char *, int);

/* 2
 */
struct otg_instance *otg_create(void);
void otg_destroy(struct otg_instance *otg);


/* 3 - used internally by any OTG stack component to single event from non-interrupt context,
 */
extern void otg_queue_event(struct otg_instance *, otg_current_t, otg_tag_t, char *);
extern void otg_event(struct otg_instance *, otg_current_t, otg_tag_t, char *);
extern void otg_event_new(struct otg_instance *, otg_current_t, otg_tag_t, char *);

/* 5 - used by PCD/TCD drivers to signal various OTG Transceiver events
 */
void otg_event_set_irq(struct otg_instance *, int, int, u32, otg_tag_t, char *);


extern void otg_serial_number(struct otg_instance *otg, char *serial_number_str);
extern void otg_init (struct otg_instance *otg);
extern void otg_exit (struct otg_instance *otg);

/* message
 */
#if defined(CONFIG_OTG_LNX)
extern int otg_message_init_l24(struct otg_instance *);
extern void otg_message_exit_l24(struct otg_instance *);
#endif /* defined(OTG_LNX) */

#if defined(CONFIG_OTG_QNX)
extern int otg_message_init_qnx(struct otg_instance *);
extern void otg_message_exit_qnx(struct otg_instance *);
#endif /* defined(CONFIG_OTG_QNX) */

extern int otg_message_init(struct otg_instance *);
extern void otg_message_exit(void);

//extern int otg_write_message_irq(char *buf, int size);
extern int otg_write_message(struct otg_instance *otg, char *buf, int size);
extern int otg_read_message(char *buf, int size);
extern int otg_data_queued(void);
extern unsigned int otg_message_block(void);
//extern unsigned int otg_message_poll(struct file *, struct poll_table_struct *);


/* trace
 */
extern int otg_trace_init (void);
extern void otg_trace_exit (void);
#if defined(OTG_LINUX)
extern int otg_trace_init_l24(void);
extern void otg_trace_exit_l24(void);
#endif /* defined(OTG_LINUX) */

#if defined(OTG_WINCE)
extern int otg_trace_init_w42(void);
extern void otg_trace_exit_w42(void);
#endif /* defined(OTG_WINCE) */

extern int otgtrace_init (void);
extern void otgtrace_exit (void);
extern int otg_trace_proc_read (char *page, int count, int * pos);
extern int otg_trace_proc_write (const char *buf, int count, int * pos);

/* usbp
 */
extern int usbd_device_init (void);
extern void usbd_device_exit (void);


/*
 * otgcore/otg-mesg.c
 */

extern void otg_message(struct otg_instance *otg, char *buf);
extern void otg_mesg_get_status_update(struct otg_status_update *status_update);
extern void otg_mesg_get_firmware_info(struct otg_firmware_info *firmware_info);
extern int otg_mesg_set_firmware_info(struct otg_firmware_info *firmware_info);
extern struct otg_instance * mesg_otg_instance;


/*
 * otgcore/otg.c
 */
extern char * otg_get_state_names(int i);

/*
 * ops
 */

#define CORE core_trace_tag
extern otg_tag_t CORE;

extern struct hcd_instance * otg_set_hcd_ops(struct otg_instance *otg, struct hcd_ops *);
extern struct ocd_instance * otg_set_ocd_ops(struct otg_instance *otg, struct ocd_ops *);
extern struct pcd_instance * otg_set_pcd_ops(struct otg_instance *otg, struct pcd_ops *);
extern struct tcd_instance * otg_set_tcd_ops(struct otg_instance *otg, struct tcd_ops *);
extern int otg_set_usbd_ops(void *);

extern void otg_get_ocd_info(struct otg_instance *otg, otg_tick_t *ticks, u16 *d_framenum);
extern void otg_get_trace_info(struct otg_instance *otg, otg_trace_t *p);
extern int otg_tmr_id_gnd(void);
extern otg_tick_t otg_tmr_ticks(void);
extern otg_tick_t otg_tmr_elapsed(otg_tick_t *t1, otg_tick_t *t2);
extern u16 otg_tmr_framenum(void);
extern u32 otg_tmr_interrupts(void);

//extern void otg_write_info_message(struct usbd_bus_instance *, char *msg);


/* @} */
