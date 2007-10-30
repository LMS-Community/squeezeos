/* $Header: /wwg/motif/xltwavplay/RCS/server.h,v 1.1 1997/04/14 01:00:22 wwg Exp $
 * Warren W. Gay VE3WWG		Wed Feb 26 21:58:08 1997
 *
 * SERVER.C HEADER FILE:
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
 * $Log: server.h,v $
 * Revision 1.1  1997/04/14 01:00:22  wwg
 * Initial revision
 *
 */
#ifndef _server_h_
#define _server_h_ "@(#)server.h $Revision: 1.1 $"

#define PLAYLOCK_SECS		3
#define RECDLOCK_SECS		3

extern int clntIPC;				/* Client IPC ID for message queue */
extern DSPPROC svr_work_proc;			/* Server work procedure */

extern void toclnt_ready(int flags);
extern void toclnt_bits(int flags,int bits);
extern int toclnt_fatal(int flags,const char *format,...);
extern int toclnt_settings(int flags,WAVFILE *wfile,WavPlayOpts *wavopts);

extern int Serve(SVRMSG *pmsg,int bWorkProc);
extern int ServerWorkProc(DSPFILE *dfile);
extern int Server(key_t IPCKey);

/*
 * This structure maintains the state of the server based upon incoming client
 * requests.
 */
struct S_SVR {
	char		path[1024];		/* Name of file to play/record */
	WavPlayOpts	opts;			/* Wav options */
	WAVFILE		*wfile;			/* Currently opened WAV file */
	int		lockIPCID;		/* Play/Record IPC ID of the locking semaphores */
	key_t		IPCKey;			/* Semaphore IPC Key */
};

extern struct S_SVR svr;

#endif /* _server_h */

/* $Source: /wwg/motif/xltwavplay/RCS/server.h,v $ */
