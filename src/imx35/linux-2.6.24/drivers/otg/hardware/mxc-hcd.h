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
 * otg/hardware/mxc-hcd.h - Freescale USBOTG aware Host Controller Driver (HCD)
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/mxc/mxc-hcd.h|20070614183949|39353
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *      Tony Tang <tt@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc-hcd.h
 * @brief Freescale USB Host Controller Driver
 *
 * This is a complete and self contained Linux 2.6 USB Host Driver.
 *
 * It also conforms to the requirements for use as an OTG HCD driver
 * in the Belcarra OTG Stack.
 *
 * The hardware is an integrated design, so it also works in conjunction
 * with the mxc OCD and PCD drivers to share the hardware under the direction
 * of the OTG State Machine.
 *
 *
 * @ingroup FSOTG
 */

/* ********************************************************************************************* */

/*! Data Structures Overview
 *
 * Urbs point at:
 *
 *      struct urb {
 *              struct  usb_device *udev;
 *      }
 *
 * Udev points at hdev (stuct hcd_dev) via:
 *
 *      struct usb_device {
 *              void *hcpriv;
 *      }
 *
 * Hdev is a simple structure for each "device" on the bus that the host has
 * enumerated. It contains an array of void *ep[32] which contains pointers
 * to active requests indexed using the endpoint number and direction. This
 * allows us to find an active request using endpoint information provided
 * by the hcd layer:
 *
 *      struct hcd_dev {        // usb_device.hcpriv points to this
 *
 *              struct list_head        dev_list;       // on this hcd
 *              struct list_head        urb_list;       // pending on this dev
 *              void                    *ep[32];        // per-configuration hc/hcd state such as qh or ed
 *      }
 *
 *
 * The host drivers are not allowed to use the urb->urb_list so we will keep
 * our own list of inactive requests using the mxc_req structure. Note that
 * we track the epnum (derived from endpoint information provided by the
 * hcd layer) and etdn (the hardware slot number) for the request if it
 * is schedule:
 *
 *      struct mxc_req {
 *              struct urb              *urb;
 *              struct list_head        queue;
 *
 *              int                     epnum;
 *              int                     etdn;
 *      };
 *
 *
 *
 * The mxc_hcd structure contains:
 *
 *      struct mxc_hcd {
 *              struct list_head        unused;                         // unused requests
 *              struct list_head        inactive;                       // in-active requests
 *              struct mxc_req          *active[NUM_ETDS];               // active requests by ETD
 *      };
 *
 * Requests are allocated on the fly when needed, but saved in the unused
 * queue when finished.
 *
 * Requests that are pending are in the inactive queue. The scheduler will
 * attempt to start any inactive requests when new requests are queued or
 * old requests are finished.
 *
 * Requests can stay on the inactive queue for two reasons, first if there is already
 * an active request for that device and endpoint (by verifying ep[] list is null) or
 * if there are no available resources (x/y buffers or etd slots.)
 *
 * The mxc_hcd structure will have a list of NUM_ETD pointer to active
 * requests. If the array is NULL then the ETD is not being used. The active
 * array is indexed by the ETD (hardware) number that the request is
 * scheduled on.
 *
 *
 * The primary reason for the inactive queue is that while the TDI design
 * handles all of the details about a transfer until it is complete, it can
 * only handle a limited number (NUM_ETDS) of requests at a time. So if there
 * are more requests to valid device/endpoint combinations than there are ETD's
 * then some will have to wait.
 *
 * Generally the maximum number of ETD's that can be used for any device is 1 (for the
 * control endpoint) plus the number of endpoints defined in the device's configuration
 * descriptor.
 *
 * It may be necessary to de-activate previously started requests periodically
 * if there is more work than can be handled. This will require three things:
 *
 *      - setup of etd registers in mxc request
 *      - start mxc request based on etd in mxc request
 *      - code to de-activate by saving etd in mxc request and place at end of inactive list
 *
 * IFF we can safely stop the ETD then the above would safely get it restarted.
 *
 * It may be safest to stop ETD's from SOF interrupt when (presumably) there is
 * no active tranfers.
 *
 */

/*! Driver Overview
 *
 *  Data Structures
 *  ETD Management
 *  Transfer Request Management
 *  Data Buffer Management
 *  Data Tranfer Handling
 *  Real Root Hub Support
 *  MXC Host Interrupt Handler
 *  OTG Support Functions
 *  Linux 2.6 USB Device Support
 *  Linux 2.6 Device Driver Support
 *  Module init/exit
 *
 */

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Data Structures
 */


/*!
 * @struct fs_data_buff
 */
typedef struct fs_data_buff {
        u32 data[DATA_BUFF_SIZE/sizeof(u32)]; // Cannot safely write less than 32-bit quantities
} volatile fs_data_buff;


/*!
 * @struct mxc_req
 *
 * This is used to queue urb's so that they can be performed when there
 * are hardware resources available.
 *
 */
struct mxc_req {

        struct urb              *urb;                   // the urb for this request
        struct list_head        queue;                  // for inactive or unused queue

        int                     epnum;                  // array index for ep and active arrays
        int                     etdn;                   // if scheduled this is the etd we are using
        u8                      etd_urb_state;          // current state

        u32                     remaining;

        endpoint_transfer_descriptor sdp_etd;           // Setup data phase save area
        fs_data_buff            *x;
        fs_data_buff            *y;

        u32                     setup_dma;
        u32                     transfer_dma;

        void                    *bounce_buffer;
        void                    *bounce_addr;
        void                    *transfer_buffer_save;
};


/*! mxc_hcd
 */
#define MAX_HUB_PORTS           8
#define MXC_MAX_USB_ADDRESS     8
//#define OTG_PORT                2
struct mxc_hcd {
        struct usb_hcd          hcd;
        spinlock_t              lock;

        struct device           *dev;

        //u32                     port1;
        //u8                      ctrl1;
        //u8                      ctrl2;

        //u8                      otg_device_mask;
        BOOL                    otg_port_enabled;

        u32                     real_hub_port_change_status;
        u32                     real_port_change_status[MAX_HUB_PORTS];
        u32                     virt_hub_port_change_status;
        u32                     virt_port_status[MAX_HUB_PORTS];

        u32                     change_data;

        struct OLD_WORK_STRUCT  rh_bh;

        BOOL                    suspended;

        u32                     sof_count;
        u32                     int_mask;

        //int                     free_etds;

        u16                     free_buffs;
        u16                     buff_list[NUM_DATA_BUFFS];

        u32                     root_hub_desc_a;
        u32                     root_hub_desc_b;
        u8                      bNbrPorts;
        u8                      wHubCharacteristics;
        u8                      bPwrOn2PwrGood;
        u8                      DeviceRemovable;
        u8                      PortPwrCtrlMask;

        struct list_head        unused;                         // unused requests
        struct list_head        inactive;                       // in-active requests
        struct mxc_req          *active[NUM_ETDS];              // active requests by ETD

                                                                // Statistics
        int                     active_count;                   // number of active requests
        int                     allocated_count;                // number of allocated request structs
        u32                     request_count;                  // total number of enqueued requests

        //struct usb_device       *first_dev;
	void                    *ep[MXC_MAX_USB_ADDRESS + 1][32];
};

#define OUT(o,e)((o && e) ? 1 : 0)

#define EPQ_EMPTY         0
#define EPQ_RUNNING       1
#define EPQ_WAITING       2
#define EPQ_NOTUSED       3

struct usbp_hub_descriptor {
        __u8  bDescLength;
        __u8  bDescriptorType;
        __u8  bNbrPorts;
        __u16 wHubCharacteristics;
        __u8  bPwrOn2PwrGood;
        __u8  bHubContrCurrent;
                /* add 1 bit for hub status change; round to bytes */
        __u8  DeviceRemovable;
        __u8  PortPwrCtrlMask;
} __attribute__ ((packed));



static inline struct mxc_hcd *hcd_to_mxc(struct usb_hcd *hcd)
{
        return container_of(hcd, struct mxc_hcd, hcd);
}

/* convert epnum and out flag into index into 32 entry array */

#define EPNUM(e,o) (((e & 0xf) << 1) + o)

#define ETD_URB_COMPLETED       0
#define ETD_URB_SETUP_STATUS    1
#define ETD_URB_SETUP_DATA      2
#define ETD_URB_SETUP_START     3
#define ETD_URB_BULK_START      4
#define ETD_URB_BULKWZLP        5
#define ETD_URB_BULKWZLP_START  6
#define ETD_URB_INTERRUPT_START 7
#define ETD_URB_ISOC_START      8
