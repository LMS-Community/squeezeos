/* $Header: /wwg/motif/xltwavplay/RCS/xmsprint.h,v 1.5 1997/04/14 00:54:23 wwg Exp $
 * Warren W. Gay VE3WWG		Sat Mar  1 14:10:02 1997
 *
 * XmSprintf header file:
 *
 *
 * 	X LessTif WAV Play :
 * 
 * 	Copyright (C) 1997  Warren W. Gay VE3WWG
 * 
 * This  program is free software; you can redistribute it and/or modify it
 * under the  terms  of  the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 * 
 * This  program  is  distributed  in  the hope that it will be useful, but
 * WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details (see enclosed file COPYING).
 * 
 * You  should have received a copy of the GNU General Public License along
 * with this  program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Send correspondance to:
 * 
 * 	Warren W. Gay VE3WWG
 * 	5536 Montevideo Road #17
 *	Mississauga, Ontario L5N 2P4
 * 
 * Email:
 * 	wwg@ica.net			(current ISP of the month :-) )
 * 	bx249@freenet.toronto.on.ca	(backup)
 *
 * $Log: xmsprint.h,v $
 * Revision 1.5  1997/04/14 00:54:23  wwg
 * Added GNU license info
 *
 * Revision 1.4  1997/03/04 03:08:53  wwg
 * Added function XmSprintfText()
 *
 * Revision 1.3  1997/03/01 21:50:27  wwg
 * Put StrDate() function in here
 *
 * Revision 1.2  1997/03/01 19:28:44  wwg
 * Added XmSprintfLabel() function
 *
 * Revision 1.1  1997/03/01 19:13:18  wwg
 * Initial revision
 */
#ifndef _xmsprint_h_
#define _xmsprint_h_ "@(#)xmsprint.h $Revision: 1.5 $"

extern XmString XmSprintf(const char *format,...);
extern void XmSprintfLabel(Widget w,const char *format,...);
extern void XmSprintfText(Widget w,const char *format,...);

extern char *StrDate(time_t td);

#endif /* _xmsprint_h_ */

/* $Source: /wwg/motif/xltwavplay/RCS/xmsprint.h,v $ */
