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
 * otg/hardware/arc.h - MX31 Device driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/arc/arc.h|20070801221240|58512
 *
 *	Copyright (c) 2006-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 */
/*!
 * @defgroup ARC Freescale HS (ARC)
 * @ingroup Hardware
 */
/*!
 * @file otg/hardware/arc.h
 * @brief Belcarra Freescale ARC Device driver.
 *
 * @ingroup ARC
 * @ingroup OTGDEV
 * @ingroup LINUXOS
 */



struct mx31_dtd_list_struct {
	struct list_head        td_queue;
	struct ep_td_struct	*td;
	int			primed;
	int			epnum;
	int			dir;
};

struct arc_private_struct {
	struct ep_queue_head 	*cur_dqh;
	struct ep_td_struct     *cur_dtd;
};

struct arcotg_udc {
	struct ep_queue_head *ep_qh;    /* Endpoints Queue-Head */
        struct ep_td_struct *ep_dtd;
};
