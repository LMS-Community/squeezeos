/* $Header: /wwg/motif/xltwavplay/RCS/server.c,v 1.3 1997/04/14 01:40:41 wwg Exp $
 * Warren W. Gay VE3WWG		Tue Feb 25 21:46:16 1997
 *
 * SERVER MODE FUNCTIONS:
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
 * $Log: server.c,v $
 * Revision 1.3  1997/04/14 01:40:41  wwg
 * Unlock the record semaphore, if Wav open failed,
 * since we've already locked it by this time!
 *
 * Revision 1.2  1997/04/14 01:36:21  wwg
 * WavOpenForWrite() now reports error message back to client
 *
 * Revision 1.1  1997/04/14 00:24:16  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)server.c $Revision: 1.3 $";

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <linux/soundcard.h>
#include "wavplay.h"
#include "server.h"

static void toclnt_errmsg(int msg_errno,const char *message,int flags);

int clntIPC = -1;				/* Client IPC ID for message queue */
int bExit = 0;					/* TRUE once the server is requested to exit */
DSPPROC svr_work_proc = NULL;			/* DSP/Server work process */

struct S_SVR svr;				/* Server state information */
static char ermsg[2048];			/* Captured error message */

/*
 * This function captures message text (only)
 */
static void
z_erf(const char *format,va_list ap) {
	vsprintf(ermsg,format,ap);
}

/*
 * This function formats and processes an error message:
 */
static void
x_erf(const char *format,va_list ap) {
	int msg_errno = errno;

	vsprintf(ermsg,format,ap);		/* Format the message */
	toclnt_errmsg(msg_errno,ermsg,0);

	fputs(ermsg,stderr);			/* Copy to stderr also */
	fputc('\n',stderr);
}

/*
 * Format a message for the client to popup:
 */
static void
ClntMsg(const char *format,...) {
	va_list ap;

	va_start(ap,format);
	x_erf(format,ap);
	va_end(ap);
}

/*
 * Service a client request:
 */
int
Serve(SVRMSG *pmsg,int bWorkProc) {
	int len;
	SVRMSG msg;
	static char *argv[2] = { NULL, NULL };

	if ( pmsg->msg_type == ToSvr_Bits ) {
		/*
		 * Change bits override (only when not busy) :
		 */
		if ( bWorkProc )				/* In a work proc? */
			return 1;				/* Tell DSP play to exit */

		svr.opts.DataBits.optChar = 'b';
		svr.opts.DataBits.optValue = pmsg->u.tosvr_bits.DataBits;
		toclnt_settings(0,svr.wfile,&svr.opts);
		return 0;

	} else if ( pmsg->msg_type == ToSvr_Bye ) {
		/*
		 * Exit server:
		 */
		if ( cmdopt_x )
			fputs("->ToSvr_Bye\n",stderr);
		bExit = 1;					/* Server requested to exit */
		return 1;					/* Tell DSP play to exit */

	} else if ( pmsg->msg_type == ToSvr_Path ) {
		/*
		 * Register a pathname:
		 */
		if ( bWorkProc )				/* In a work proc? */
			return 1;				/* Tell DSP play to exit */

		if ( svr.wfile != NULL ) {
			WavClose(svr.wfile,x_erf);		/* Close last wav file */
			svr.wfile = NULL;
		}

		/*
		 * Reset any overrides:
		 */
		svr.opts.SamplingRate.optChar = 0;
		svr.opts.Channels.optChar = 0;
		svr.opts.DataBits.optChar = 0;

		if ( (len = pmsg->bytes) > sizeof svr.path )
			len = sizeof svr.path;
		
		if ( len > 0 )
			strncpy(svr.path,pmsg->u.tosvr_path.path,len)[len] = 0;
		else	*svr.path = 0;

		/*
		 * We send the same message back to the client so that it now
		 * can update the label widget with the pathname showing. Note that this
		 * message has the same size and format as ToSvr_Path, so we reuse it here:
		 */
		pmsg->msg_type = ToClnt_Path;			/* Send it back to client now */
		MsgToClient(clntIPC,pmsg,0);			/* So it can update its display */

		/*
		 * Now do a stat on this pathname:
		 */
		if ( !*svr.path )
			msg.u.toclnt_stat.errnox = ENOENT;	/* No path: then not found */
		else if ( stat(svr.path,&msg.u.toclnt_stat.sbuf) )
			msg.u.toclnt_stat.errnox = errno;	/* Pass back errno value */
		else	msg.u.toclnt_stat.errnox = 0;		/* stat() succeeded */

		msg.msg_type = ToClnt_Stat;			/* We're sending stat info back */
		msg.bytes = sizeof msg.u.toclnt_stat;		/* This substructure going back */
		MsgToClient(clntIPC,&msg,0);			/* Send back the response */

		if ( msg.u.toclnt_stat.errnox != 0 )
			return 0;				/* Don't try opening file if error */

		/*
		 * Wav info file requested:
		 */
		if ( !*svr.path )
			msg.u.toclnt_wavinfo.errnox = ENOENT;	/* No path: Not found */
		else if ( (svr.wfile = WavOpenForRead(svr.path,x_erf)) == NULL ) {
			msg.u.toclnt_wavinfo.errnox = errno;
			strncpy(msg.u.toclnt_wavinfo.errmsg,ermsg,sizeof msg.u.toclnt_wavinfo.errmsg)
				[sizeof msg.u.toclnt_wavinfo.errmsg - 1] = 0;
		} else	{
			WavReadOverrides(svr.wfile,&svr.opts);
			msg.u.toclnt_wavinfo.errnox = 0;
			msg.u.toclnt_wavinfo.wavinfo = svr.wfile->wavinfo;
		}
		msg.msg_type = ToClnt_WavInfo;			/* We're sending WAV Hdr info back */
		msg.bytes = sizeof msg.u.toclnt_wavinfo;	/* This substructure going back */
		MsgToClient(clntIPC,&msg,0);			/* Send back the response */

		toclnt_settings(0,svr.wfile,&svr.opts);		/* Send back other settings too */
		return 0;

	} else if ( pmsg->msg_type == ToSvr_Restore ) {

		/*
		 * Reset all overrides:
		 */
		svr.opts.SamplingRate.optChar = 0;
		svr.opts.Channels.optChar = 0;
		svr.opts.DataBits.optChar = 0;

rfrsh:		if ( svr.wfile != NULL ) {
			WavClose(svr.wfile,x_erf);
			svr.wfile = NULL;
updt:			if ( *svr.path != 0 && (svr.wfile = WavOpenForRead(svr.path,z_erf)) != NULL )
				WavReadOverrides(svr.wfile,&svr.opts);
		}
		toclnt_settings(0,svr.wfile,&svr.opts);
		return bWorkProc ? 1 : 0;

	} else if ( pmsg->msg_type == ToSvr_Chan ) {

		svr.opts.Channels.optChar = 'S';
		svr.opts.Channels.optValue = pmsg->u.tosvr_chan.Channels;
		goto rfrsh;

	} else if ( pmsg->msg_type == ToSvr_Play ) {
		/*
		 * Play a WAV file:
		 */
		if ( bWorkProc )				/* In a work proc? */
			return 0;				/* Tell DSP play to continue */

		/*
		 * Not playing yet, start playing:
		 */
		argv[0] = svr.path;				/* Point to pathname */

		if ( *argv[0] == 0 ) {
			if ( cmdopt_x )
				fputs("No pathname to play!\n",stderr);
			return 1;
		}

		/*
		 * Try to acquire the lock on the device:
		 */
		if ( svr.lockIPCID >= 0 && LockDSP(svr.lockIPCID,0,x_erf,PLAYLOCK_SECS) )
			return 0;				/* No lock acquired */

		svr_work_proc = ServerWorkProc;
		wavplay(&svr.opts,argv,x_erf);			/* Play file */
		svr_work_proc = NULL;

		/*
		 * Release the Play lock on the device:
		 */
		if ( svr.lockIPCID >= 0 )
			UnlockDSP(svr.lockIPCID,0,x_erf);
		return 0;

	} else if ( pmsg->msg_type == ToSvr_SamplingRate ) {

		if ( pmsg->u.tosvr_sampling_rate.SamplingRate >= DSP_MIN ) {
			svr.opts.SamplingRate.optChar = 's';		
			svr.opts.SamplingRate.optValue = (UInt32) pmsg->u.tosvr_sampling_rate.SamplingRate;
		} else	svr.opts.SamplingRate.optChar = 0;	/* Turn off override */
		goto rfrsh;					/* Refresh client settings */

	} else if ( pmsg->msg_type == ToSvr_Stop ) {
		return 1;					/* Tell DSP play/record to exit */

	} else if ( pmsg->msg_type == ToSvr_Pause ) {
		return 0;				/* Ignore this */

	} else if ( pmsg->msg_type == ToSvr_Record ) {
		/*
		 * Play a WAV file:
		 */
		if ( bWorkProc )			/* In a work proc? */
			return 0;			/* Yes, ignore this event */

		/*
		 * Not recording yet, start recording:
		 */
		if ( svr.wfile != NULL ) {
			WavClose(svr.wfile,x_erf);
			svr.wfile = NULL;
		}

		/*
		 * Tell client the pathname we're using for the recording:
		 */
		strcpy(svr.path,RECORD_PATH);		/* Always record to same place */

		msg.msg_type = ToClnt_Path;
		strcpy(msg.u.toclnt_path.path,svr.path);
		msg.bytes = strlen(msg.u.toclnt_path.path);
		MsgToClient(clntIPC,&msg,0);		/* Send back our path */

		/*
		 * Try to acquire the record lock on the device:
		 */
		if ( svr.lockIPCID >= 0 && LockDSP(svr.lockIPCID,1,x_erf,RECDLOCK_SECS) )
			return 0;				/* No lock acquired */

		/*
		 * Now attempt to open for write:
		 */
		svr.wfile = WavOpenForWrite(svr.path,
			pmsg->u.tosvr_record.Channels,
			pmsg->u.tosvr_record.SamplingRate,
			pmsg->u.tosvr_record.DataBits,
			999999,
			x_erf);

		/*
		 * Send back error info if we fail open:
		 */
		if ( svr.wfile == NULL ) {
			msg.u.toclnt_stat.errnox = errno == 0 ? EINVAL : errno;
			msg.msg_type = ToClnt_Stat;	/* We're sending stat info back */
			msg.bytes = sizeof msg.u.toclnt_stat;
			MsgToClient(clntIPC,&msg,0);

			/*
			 * Release the record lock on the device:
			 */
			if ( svr.lockIPCID >= 0 )
				UnlockDSP(svr.lockIPCID,1,x_erf);
			return 1;
		}

		/*
		 * Clear all overrides:
		 */
		svr.opts.SamplingRate.optChar = 0;
		svr.opts.Channels.optChar = 0;
		svr.opts.DataBits.optChar = 0;

		/*
		 * Start recording:
		 */
		svr_work_proc = ServerWorkProc;
		wavrecd(&svr.opts,NULL,x_erf);			/* Record file */
		svr_work_proc = NULL;

		/*
		 * Release the record lock on the device:
		 */
		if ( svr.lockIPCID >= 0 )
			UnlockDSP(svr.lockIPCID,1,x_erf);
		goto updt;					/* Update client */

	} else if ( pmsg->msg_type == ToSvr_Debug ) {

		cmdopt_x = pmsg->u.tosvr_debug.bDebugMode ? 1 : 0;
		return 0;					/* Continue biz as usual */

	} else if ( pmsg->msg_type == ToSvr_SemReset ) {
		/*
		 * Remove old semaphore set :
		 */
		if ( semctl(svr.lockIPCID,0,IPC_RMID,NULL) < 0 ) {
			toclnt_fatal(0,"%s: Unable to remove old locking semaphores.",sys_errlist[errno]);
			exit(13);
		}

		/*
		 * Open new semaphore set:
		 */
		if ( (svr.lockIPCID = OpenDSPLocks(svr.IPCKey,1,x_erf)) < 0 ) {
			toclnt_fatal(0,"%s:\nAttempting to obtain new locking\nsemaphores.",
				sys_errlist[errno]);
			exit(13);
		}

		/*
		 * Provide reset feedback to client program:
		 */
		ClntMsg("Old semaphores have been released.\n"
			"The new semaphore set for IPC Key 0x%08lX (%lu)\n"
			"is now IPC ID %d.",
			(unsigned long)svr.IPCKey,
			(unsigned long)svr.IPCKey,
			(int)svr.lockIPCID);

	} else	{
		ClntMsg("Unknown server request type %u\n",(unsigned)pmsg->msg_type);
		return 1;
	}

	return 1;						/* Unknown request */
}

/*
 * This process is called during a "Playing/Recording" of a WAV file. If we return
 * TRUE, this process will stop.
 */
int
ServerWorkProc(DSPFILE *dfile) {
	SVRMSG msg;	
	int z;

	if ( MsgFromClient(clntIPC,&msg,IPC_NOWAIT) != 0 )
		return 0;			/* No messages from client */

	/*
	 * Shut the DSP up right away if we hit pause:
	 */
	if ( msg.msg_type == ToSvr_Pause )
		ioctl(dfile->fd,SNDCTL_DSP_RESET,0);

	/*
	 * If we get a Pause request, we block here indefinitely until
	 * another message from the client arrives:
	 */
	while ( msg.msg_type == ToSvr_Pause ) {
		/*
		 * During a "pause", we block on a client request message get:
		 */
		if ( MsgFromClient(clntIPC,&msg,0) )
			return -1;		/* Failed msg read */
	}

	/*
	 * Non pause message received:
	 */
	z = Serve(&msg,1);			/* Server from within work procedure */

	if ( z )				/* Shut up DSP if we're stopping */
		ioctl(dfile->fd,SNDCTL_DSP_RESET,0);
	return z;
}

/*
 * This is the main server loop:
 */
int
Server(key_t IPCKey) {
	SVRMSG msg;

	memset(&svr,0,sizeof svr);		/* Initialize server state */
	svr.opts.ipc = -1;
	svr.wfile = NULL;
	svr.IPCKey = IPCKey;
	svr.lockIPCID = OpenDSPLocks(svr.IPCKey,1,x_erf);

	toclnt_ready(0);			/* Tell client we're ready */

	while ( !bExit && !MsgFromClient(clntIPC,&msg,0) )
		Serve(&msg,0);

	exit(0);
}

/*
 * Server -> Client
 *
 * Tell client that server is ready:
 * Returns 0 if message sent. flags can be IPC_NOWAIT.
 */
void
toclnt_ready(int flags) {
	SVRMSG msg;

	msg.msg_type = ToClnt_Ready;
	msg.bytes = 0;

	if ( MsgToClient(clntIPC,&msg,flags) != 0 ) {	/* Send to client */
		ClntMsg("%s: toclnt_ready(flags=0%o;ipc=%d)\n",sys_errlist[errno],flags,clntIPC);
		exit(13);
	}
}

/*
 * Server -> Client
 *
 * Tell client that server is ready:
 */
void
toclnt_bits(int flags,int bits) {
	SVRMSG msg;

	msg.msg_type = ToClnt_Bits;
	msg.bytes = sizeof msg.u.toclnt_bits;
	msg.u.toclnt_bits.DataBits = bits;

	if ( MsgToClient(clntIPC,&msg,flags) != 0 ) {	/* Send to client */
		ClntMsg("%s: toclnt_ready(flags=0%o;ipc=%d)\n",sys_errlist[errno],flags,clntIPC);
		exit(13);
	}
}

/*
 * Sever -> Client
 *
 * Tell client many of the current server settings:
 */
int
toclnt_settings(int flags,WAVFILE *wfile,WavPlayOpts *wavopts) {
	SVRMSG msg;
	int z;

	memset(&msg,0,sizeof msg);

	msg.msg_type = ToClnt_Settings;

	if ( wfile != NULL ) {
		/*
		 * We have a WAV file open:
		 */
		WavReadOverrides(wfile,wavopts);	/* Apply current overrides */
		msg.u.toclnt_settings.SamplingRate = wfile->wavinfo.SamplingRate;
		msg.u.toclnt_settings.Channels = wfile->wavinfo.Channels;
		msg.u.toclnt_settings.Samples = wfile->wavinfo.Samples;
		msg.u.toclnt_settings.DataBits = wfile->wavinfo.DataBits;
		msg.u.toclnt_settings.bOvrSampling = wfile->wavinfo.bOvrSampling;
		msg.u.toclnt_settings.bOvrMode = wfile->wavinfo.bOvrMode;
		msg.u.toclnt_settings.bOvrBits = wfile->wavinfo.bOvrBits;
	} else	{
		/*
		 * No WAV file open:
		 */
		if ( (msg.u.toclnt_settings.bOvrSampling = wavopts->SamplingRate.optChar ? 1 : 0) != 0 )
			msg.u.toclnt_settings.SamplingRate = wavopts->SamplingRate.optValue;
		else	msg.u.toclnt_settings.SamplingRate = 8000;
		if ( (msg.u.toclnt_settings.bOvrMode = wavopts->Channels.optChar ? 1 : 0) != 0 )
			msg.u.toclnt_settings.Channels = wavopts->Channels.optValue;
		else	msg.u.toclnt_settings.Channels = Mono;
		if ( (msg.u.toclnt_settings.bOvrBits = wavopts->DataBits.optChar ? 1 : 0) != 0 )
			msg.u.toclnt_settings.DataBits = wavopts->DataBits.optValue;
		else	msg.u.toclnt_settings.DataBits = 8;
	}

	/* For now: */
	strcpy(msg.u.toclnt_settings.WavType,"PCM");

	msg.bytes = sizeof msg.u.toclnt_settings;

	if ( (z = MsgToClient(clntIPC,&msg,flags)) != 0 && flags && errno != EAGAIN ) {
		toclnt_fatal(0,"%s: toclnt_settings(flags=0%o;ipc=%d)\n",sys_errlist[errno],flags,clntIPC);
		exit(13);
	}

	return z >= 0 ? 0 : -1;
}

/*
 * Tell client about fatal server error:
 */ 
int
toclnt_fatal(int flags,const char *format,...) {
	SVRMSG msg;
	va_list ap;
	char buf[2048];

	msg.msg_type = ToClnt_Fatal;		/* Fatal server error */
	msg.bytes = sizeof msg.u.toclnt_fatal;	/* Message bytes */
	msg.u.toclnt_fatal.errnox = errno;	/* Pass back errno value */

	va_start(ap,format);
	vsprintf(buf,format,ap);
	va_end(ap);

	strncpy(msg.u.toclnt_fatal.msg,buf,sizeof msg.u.toclnt_fatal.msg)[sizeof msg.u.toclnt_fatal.msg-1] = 0;

	return MsgToClient(clntIPC,&msg,flags);	/* Send message to client */
}

/*
 * Send an error message back to the client process:
 */
static void
toclnt_errmsg(int msg_errno,const char *message,int flags) {
	SVRMSG msg;

	msg.msg_type = ToClnt_ErrMsg;
	msg.bytes = sizeof msg.u.toclnt_errmsg;
	msg.u.toclnt_errmsg.errnox = msg_errno;
	strncpy(msg.u.toclnt_errmsg.msg,message,sizeof msg.u.toclnt_errmsg.msg)
		[sizeof msg.u.toclnt_errmsg.msg - 1] = 0;

	if ( MsgToClient(clntIPC,&msg,flags) != 0 ) {	/* Send to client */
		fprintf(stderr,"%s: toclnt_errmsg(...)\n",sys_errlist[errno]);
		exit(13);
	}
}

/* $Source: /wwg/motif/xltwavplay/RCS/server.c,v $ */
