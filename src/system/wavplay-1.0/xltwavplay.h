/* $Header: /wwg/motif/xltwavplay/RCS/xltwavplay.h,v 1.1 1997/04/14 00:55:28 wwg Exp $
 * Warren W. Gay VE3WWG		Thu Feb 13 21:08:14 1997
 *
 * X LessTif WAV Play :
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
 * $Log: xltwavplay.h,v $
 * Revision 1.1  1997/04/14 00:55:28  wwg
 * Initial revision
 *
 */
#ifndef _xltwavplay_h_
#define _xltwavplay_h_ "@(#)xltwavplay.h $Revision: 1.1 $"

extern void ReportErrorf(const char *format,...);

extern void CreateMenu(
	Widget wMenuBar,					// Menu bar widget
	Widget *wPulldown,					// Menu widget to create
	char *namePulldown,					// Class name of menu widget to create
	Widget *wCascade,					// Cascade button for pulldown menu to create
	char *nameCascade,					// Class name of cascade button to create
	...);							// Menu entries...

extern void SaveAsDlgOKCB(Widget w,XtPointer client_data,XtPointer call_data);

extern Widget wSaveAsDlg;					/* File "Save As..." dialog */
extern Widget wSelectionsListBox;				/* Selections list box */

extern XmString sRecorded_wav;					/* XmString() of RECORD_PATH */

#endif /* _xltwavplay_h_ */

/* $Source: /wwg/motif/xltwavplay/RCS/xltwavplay.h,v $ */
