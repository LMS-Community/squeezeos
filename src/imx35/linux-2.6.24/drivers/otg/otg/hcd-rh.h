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
 * otg/hcd/hc_xfer_rh.h - Generic transfer level USBOTG aware Host Controller Driver (HCD)
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/hcd-rh.h|20061017072623|12102
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/otg/hcd-rh.h
 * @brief Implements Generic Root Hub function for Generic Host Controller Driver.
 *
 * @ingroup HCD
 */
#ifndef HC_XFER_RH_H
#define HC_XFER_RH_H 1

#define XHC_RH_STRING_CONFIGURATION 1
#define XHC_RH_STRING_INTERFACE     2
#define XHC_RH_STRING_SERIAL        3
#define XHC_RH_STRING_PRODUCT       4
#define XHC_RH_STRING_MANUFACTURER  5

/*===========================================================================*
 * Functions provided by the generic virtual root hub.
 *===========================================================================*/

/*===========================================================================*
 * For the generic transfer aware framework.
 *===========================================================================*/

/*
 * hcd_rh_submit_urb()
 * Returns: values as for the generic submit_urb() function.
 */
extern int hcd_rh_submit_urb(struct bus_hcpriv *bus_hcpriv, struct urb *urb, int mem_flags);

extern int hcd_rh_unlink_urb(struct bus_hcpriv *bus_hcpriv, struct urb *urb);

extern void hcd_rh_get_ops(struct bus_hcpriv *bus_hcpriv);

extern int hcd_rh_init(struct bus_hcpriv *bus_hcpriv);

extern void hcd_rh_exit(struct bus_hcpriv *bus_hcpriv);

/*===========================================================================*
 * For the hardware specific root hub component.
 *===========================================================================*/

extern irqreturn_t hcd_rh_int_hndlr(int irq, void *dev_id, struct pt_regs *regs);

#endif
