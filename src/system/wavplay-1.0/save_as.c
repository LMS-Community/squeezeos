/* $Header: /wwg/motif/xltwavplay/RCS/save_as.c,v 1.2 1997/04/14 01:45:53 wwg Exp $
 * Warren W. Gay VE3WWG		Thu Apr 10 21:36:07 1997
 *
 * 	Save record.wav As... Dialog Callback [OK button]
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
 * $Log: save_as.c,v $
 * Revision 1.2  1997/04/14 01:45:53  wwg
 * Now automatically adds the "save as.." pathname to the list
 * box, as a selection.
 *
 * Revision 1.1  1997/04/14 00:00:14  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)save_as.c $Revision: 1.2 $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>

#include "xmsprint.h"
#include "xltwavplay.h"
#include "wavplay.h"

/*
 * This function is called when the recorded.wav file should be saved
 * with a new pathname.
 */
void
SaveAsDlgOKCB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmListCallbackStruct *ptr = (XmListCallbackStruct *)call_data;
	char *path;						/* C string copy */
	struct stat dest_stat, src_stat;			/* stat() info on both paths */
	char bCopy = 0;						/* Move or Copy? */
	char buf[8192];						/* I/O Buffer */
	int ifd, ofd, n;					/* In/Out fds, n is byte count */

	/*
	 * See if the recording exists to save:
	 */
	if ( stat(RECORD_PATH,&src_stat) != 0 ) {
		if ( errno == ENOENT )
			ReportErrorf("No file %s to 'save'.",RECORD_PATH);
		else	ReportErrorf("%s: doing stat on recorded.wav",sys_errlist[errno]);
		return;
	}

	/*
	 * If per chance, recorded.wav is a symlink, then force a copy to the
	 * selected "Save As.." file.
	 */
	if ( S_ISLNK(src_stat.st_mode) )			/* Is recorded.wav a symlink? */
		bCopy = 1;					/* recording is a symlink to another file */

	/*
	 * Get ASCII rendition of the selected pathname to save into:
	 */
	XmStringGetLtoR(ptr->item,XmSTRING_DEFAULT_CHARSET,&path);

	/*
	 * See if the specified path exists already:
	 */
	if ( stat(path,&dest_stat) != 0 )			/* Stat save pathname.. */
		dest_stat.st_ctime = 0;				/* Mark as not found by stat() */
	else if ( !bCopy && dest_stat.st_dev == src_stat.st_dev ) {
		/*
		 * If here, we can entertain the possibility of just doing a mv
		 * to the destination pathname:
		 */
		if ( dest_stat.st_ino == src_stat.st_ino ) {
			ReportErrorf("You tried to save the file to itself (recorded.wav)");
			return;
		}
		/*
		 * Both are on same device, so delete destination file, and just move
		 * contents into place with link()
		 */
		if ( unlink(path) == 0 )
			dest_stat.st_ctime = 0;			/* File no longer exists */
		else	{
			ReportErrorf("%s: unlink(%s)\nWill attempt to do a copy instead, next.\n",path);
			bCopy = 1;
		}
	}

	/*
	 * If the destination file already exists, see if its a symbolic link:
	 */
	if ( !bCopy && dest_stat.st_ctime != 0 ) {		/* if not copying, and path exists.. */
		if ( S_ISLNK(dest_stat.st_mode) )		/* is path a symlink? */
			bCopy = 1;				/* Destination is a symlink, do copy */
		else if ( dest_stat.st_dev != src_stat.st_dev )	/* are the files on different devs? */
			bCopy = 1;				/* Different devices: force copy */
	}

	/*
	 * Move the file into place if we can.
	 */
	if ( !bCopy ) {
		if ( link(RECORD_PATH,path) == 0 ) {		/* mv by linking.. */
			if ( unlink(RECORD_PATH) != 0 )		/* And removing the old */
				ReportErrorf("%s: unlink(%s)",sys_errlist[errno]);
			goto rm;				/* Success (or mostly success) */
		} else	ReportErrorf("%s: link(%s,%s)\nWill try a copy next..",
				sys_errlist[errno],RECORD_PATH,path);
	}

	/*
	 * Copy the file, if control passes to here:
	 */
	if ( (ifd = open(RECORD_PATH,O_RDONLY,0)) < 0 ) {
		ReportErrorf("%s: opening %s for read.",sys_errlist[errno],RECORD_PATH);
		goto xit;
	}

	if ( (ofd = open(path,O_WRONLY|O_CREAT|O_TRUNC,0644)) < 0 ) {
		ReportErrorf("%s: opening %s for read.",sys_errlist[errno],path);
		close(ifd);
		goto xit;
	}

	while ( (n = read(ifd,buf,sizeof buf)) > 0 )
		if ( write(ofd,buf,n) < 0 ) {
			ReportErrorf("%s: writing file %s for copy.",sys_errlist[errno],path);
			close(ifd);
			close(ofd);
			unlink(path);
			goto xit;
		}

	if ( n < 0 ) {
		ReportErrorf("%s: reading file %s for copy.",sys_errlist[errno],RECORD_PATH);
		close(ifd);
		close(ofd);
		unlink(path);
		goto xit;
	}

	if ( fsync(ofd) != 0 )
		ReportErrorf("%s: fsync(%s)",sys_errlist[errno],path);
	if ( close(ofd) == 0 )
		unlink(RECORD_PATH);		/* Delete recorded.wav if copy successful */
	close(ifd);

	/*
	 * Success exit:
	 */
rm:	if ( stat(RECORD_PATH,&src_stat) == 0 )	/* If record.wav still exists.. */
		goto xit;			/* ..then don't unmanage this dialog yet */

	if ( XtIsManaged(wSaveAsDlg) )		/* Safety test.. */
		XtUnmanageChild(wSaveAsDlg);	/* Now pop this dialog down */

	/*
	 * Remove the recorded.wav pathname from the list box:
	 */
	if ( XmListItemExists(wSelectionsListBox,sRecorded_wav) != False )
		XmListDeleteItem(wSelectionsListBox,sRecorded_wav);

	/*
	 * If the new pathname exists, add it to the list box:
	 */
xit:	if ( stat(path,&dest_stat) == 0 ) {
		if ( XmListItemExists(wSelectionsListBox,ptr->item) == False ) /* path */
			XmListAddItem(wSelectionsListBox,ptr->item,0);
	}

	XtFree(path);				/* Free the allocated string and leave popped up */
}

/* $Source: /wwg/motif/xltwavplay/RCS/save_as.c,v $ */
