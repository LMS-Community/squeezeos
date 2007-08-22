/* $Header: /wwg/motif/xltwavplay/RCS/menu.c,v 1.1 1997/04/14 00:04:22 wwg Exp $
 * Warren W. Gay VE3WWG		Thu Feb 13 21:10:04 1997
 *
 * Menu related functions:
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
 * $Log: menu.c,v $
 * Revision 1.1  1997/04/14 00:04:22  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)menu.c $Revision: 1.1 $";

#include <stdio.h>
#include <stdarg.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleB.h>
#include "xltwavplay.h"

void
CreateMenu(
  Widget wMenuBar,
  Widget *wPulldown,
  char *namePulldown,
  Widget *wCascade,
  char *nameCascade,
  ...) {		/* char *classname,char menu_type,XtCallbackProc callback,Widget *pWidget,... */
	Arg al[32];
	Cardinal ac;
	va_list ap;
	Widget wChoice;
	/* Varargs repeating group: */
	char *nameChoice;			/* Class name of the item to select */
	char menu_type;				/* 'M' for normal menu, 'T'/'t' for toggle item */
	XtCallbackProc callback;		/* Callback procedure */
	Widget *wPointer;			/* Pointer to widget to return */
	Boolean tf;				/* Boolean value for 'T' and 't' types: */

	va_start(ap,nameCascade);

	ac = 0;
	*wPulldown = XmCreatePulldownMenu(wMenuBar,namePulldown,al,ac);

	ac = 0;
	XtSetArg(al[ac],XmNsubMenuId,*wPulldown); ++ac;
	*wCascade = XmCreateCascadeButton(wMenuBar,nameCascade,al,ac);

	nameChoice = va_arg(ap,char *);

	while ( nameChoice != NULL ) {
		menu_type = va_arg(ap,char);		/* Get 'M' or 'T' */
		callback = va_arg(ap,XtCallbackProc);	/* Get callback address or NULL ptr */
		wPointer = va_arg(ap,Widget *);		/* Get address so we can return widget */

		switch ( menu_type ) {
		case 'M' :
			ac = 0;
			*wPointer = wChoice = XmCreatePushButton(*wPulldown,nameChoice,al,ac);
			if ( callback != NULL )
				XtAddCallback(wChoice,XmNactivateCallback,callback,NULL);
			break;
		case 'T' :				/* Default is TRUE */
		case 't' :				/* Default is FALSE */
			tf = menu_type == 'T' ? TRUE : FALSE;
		        ac = 0;
	                XtSetArg(al[ac],XmNindicatorOn,TRUE); ++ac;
	                XtSetArg(al[ac],XmNset,tf); ++ac;			
                        XtSetArg(al[ac],XmNindicatorType,XmN_OF_MANY); ++ac;
			*wPointer = wChoice = XmCreateToggleButton(*wPulldown,nameChoice,al,ac);
			if ( callback != NULL )
				XtAddCallback(wChoice,XmNvalueChangedCallback,callback,NULL);
			break;
		default :
			abort();			/* This should never happen */
		}
		XtManageChild(wChoice);

		nameChoice = va_arg(ap,char *);
	}
	XtManageChild(*wCascade);
	va_end(ap);
}

/* $Source: /wwg/motif/xltwavplay/RCS/menu.c,v $ */
