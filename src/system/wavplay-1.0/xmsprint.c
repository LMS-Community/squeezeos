/* $Header: /wwg/motif/xltwavplay/RCS/xmsprint.c,v 1.7 1997/04/14 00:08:49 wwg Exp $
 * Warren W. Gay VE3WWG		Sat Mar  1 14:04:46 1997
 *
 * Format a string: XmString XmSprintf(format,...)
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
 * $Log: xmsprint.c,v $
 * Revision 1.7  1997/04/14 00:08:49  wwg
 * Added GNU license info.
 *
 * Revision 1.6  1997/04/14 00:02:46  wwg
 * Hopefully the final revision for the 1.0 release
 *
 * Revision 1.5  1997/03/04 03:08:53  wwg
 * Added function XmSprintfText()
 *
 * Revision 1.4  1997/03/01 21:50:27  wwg
 * Put StrDate() function in here
 *
 * Revision 1.3  1997/03/01 19:44:14  wwg
 * Added missing rcsid[]
 *
 * Revision 1.2  1997/03/01 19:28:44  wwg
 * Added XmSprintfLabel() function
 *
 * Revision 1.1  1997/03/01 19:13:36  wwg
 * Initial revision
 */
static char rcsid[] = "@(#)xmsprint.c $Revision: 1.7 $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "xmsprint.h"

/*
 * This function returns a formatted XmString
 */
XmString
XmSprintf(const char *format,...) {
	char buf[2048];
	va_list ap;

	/*
	 * First format the message:
	 */
	va_start(ap,format);
	vsprintf(buf,format,ap);
	va_end(ap);
	
	/*
	 * Now create the XmString to return:
	 */
	return XmStringCreateLtoR(buf,XmSTRING_DEFAULT_CHARSET);
}

/*
 * This function does a sprintf() into a label widget:
 */
void
XmSprintfLabel(Widget w,const char *format,...) {
	char buf[2048];
	va_list ap;
	XmString s;

	/*
	 * First format the message:
	 */
	va_start(ap,format);
	vsprintf(buf,format,ap);
	va_end(ap);
	
	/*
	 * Now create the XmString to return:
	 */
	s = XmStringCreateLtoR(buf,XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(w,XmNlabelString,s,NULL);
	XmStringFree(s);
}

/*
 * This function does a sprintf() into a text widget:
 */
void
XmSprintfText(Widget w,const char *format,...) {
	char buf[2048];
	va_list ap;

	/*
	 * First format the message:
	 */
	va_start(ap,format);
	vsprintf(buf,format,ap);
	va_end(ap);
	
	XmTextSetString(w,buf);					/* Set text in widget now */
	XmTextSetInsertionPosition(w,0);			/* Put cursor at start of field */
}

/*
 * Return an ascii string date, without the newline:
 */
char *
StrDate(time_t td) {
	char *cp;
	static char stbuf[64];

	strcpy(stbuf,ctime(&td));
	if ( (cp = strchr(stbuf,'\n')) != NULL )
		*cp = 0;					/* Stomp on the newline */	
	return stbuf;
}

/* $Source: /wwg/motif/xltwavplay/RCS/xmsprint.c,v $ */
