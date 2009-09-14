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
 * otg/msc_fd/msc.h - Mass Storage Class
 * @(#) sl@belcarra.com|otg/functions/msc/msc-fd.h|20060702220757|39666
 *
 *      Copyright (c) 2003-2004 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */

#ifndef MSC_FD_H
#define MSC_FD_H 1

extern int msc_dispatch_query_urb(struct usbd_function_instance *, struct usbd_urb *, int , int );
extern int msc_start_sending_csw(struct usbd_function_instance *, u8 );
extern int msc_dispatch_query_urb_zlp(struct usbd_function_instance *, u32 sensedata, u32 info, int status);
extern int msc_start_sending_csw_failed(struct usbd_function_instance *, u32 sensedata, u32 info, int status);
extern int msc_start_recv_urb(struct usbd_function_instance *, int size);

#define NOCHK 0
#define NOZLP 1
#define SENDZLP 2

#if 1
/*! @name MSC activity trace functions */
/*! @{*/

static __inline__ void TRACE_SENSE(unsigned int sense, unsigned int info)
{
        TRACE_MSG2(MSC, "-->             SENSE: %06x INFO: %08x", sense, info);
}
static __inline__ void TRACE_RLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "<--             rlba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_SLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "-->             slba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_TLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "-->             tlba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_TAG(unsigned int tag, unsigned int frame)
{
        TRACE_MSG2(MSC, "-->             TAG: %8x FRAME: %03x", tag, frame);
}
static __inline__ void TRACE_CBW(char *msg, int val, int check)
{
        TRACE_MSG3(MSC,  " -->            %s %02x check: %d", msg, val, check);
}
//static __inline__ void TRACE_RECV(unsigned char *cp)
//{
//        TRACE_MSG8(MSC, "<--             recv [%02x %02x %02x %02x %02x %02x %02x %02x]",
//                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
//}
//static __inline__ void TRACE_SENT(unsigned char *cp)
//{
//        TRACE_MSG8(MSC, "-->             sent [%02x %02x %02x %02x %02x %02x %02x %02x]",
//                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
//}

/*! @} */

#endif


#endif /* MSC_H */
