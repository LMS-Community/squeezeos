/* $Header: /wwg/motif/wavplay.RCS/RCS/main.c,v 1.2 1997/04/15 02:14:14 wwg Exp $
 * Warren W. Gay VE3WWG		Sun Feb 16 20:12:22 1997#include <stdio.h>
 *
 * WAVPLAY SHELL MODE MAIN PROGRAM :
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
 * $Log: main.c,v $
 * Revision 1.2  1997/04/15 02:14:14  wwg
 * A number of small last minute fixes.
 *
 * Revision 1.1  1997/04/14 00:22:26  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)main.c $Revision: 1.2 $";

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <assert.h>
#include "wavplay.h"
#include "server.h"

int cmdopt_x = 0;					/* -x ; debug option */

/*
 * Report command usage:
 */
static void
usage(const char *cmd,OprMode opr_mode) {

	if ( opr_mode == OprRecord )
		printf("Usage:\t%s [options] output_file\n\n",cmd);
	else	printf("Usage:\t%s [options] [files...]\n\n",cmd);

	puts("Options:");
	printf("\t-%c\tThis info (or use --help)\n",OPF_HELP);
	printf("\t-%c\tQuiet mode (no messages)\n",OPF_QUIET);
	printf("\t-%c rate\tSampling rate\n",OPF_SAMPRATE);

	printf("\t-%c\tStereo (undoes -%c)\n",OPF_STEREO,OPF_MONO);
	printf("\t-%c\tMono (undoes -%c)\n",OPF_MONO,OPF_STEREO);

	if ( opr_mode == OprRecord )
		printf("\t-%c secs\tSet time limit\n",OPF_TIME);

	printf("\t-%c bits\tSet number of bits (8/16)\n",OPF_DATABITS);
	printf("\t-%c key\tSet IPC Key for lock\n",OPF_IPCKEY);
	printf("\t-%c\tRemove and recreate semaphore locks.\n",OPF_RESET);
	printf("\t-%c\tLock for play.\n",OPF_PLAY_LOCK);
	printf("\t-%c\tUnlock for play.\n",OPF_PLAY_UNLOCK);
	printf("\t-%c\tLock for record.\n",OPF_RECD_LOCK);
	printf("\t-%c\tUnlock for record.\n",OPF_RECD_UNLOCK);

	if ( opr_mode != OprRecord )
		printf("\t-%c\tDisplay info about wav file(s) only\n",OPF_INFO);

	printf("\n\t-%c\tDisplay version and Copyright info (or use --version)\n\n",OPF_VERSION);

	puts("\nWAV parameters are normally taken from the input file(s),\n"
		"but command line options can override them if required.");
}

/*
 * Report version information for this program:
 */
static void
version(void) {
	puts(	"\nwavplay/wavrec Version " WAVPLAY_VERSION "\n"
		"was written by and is\n"
		"Copyright (C) 1997 by Warren W. Gay VE3WWG\n"
		"---\n"
		"Special thanks to Erik de Castro Lopo (erikd@zip.com.au)\n"
		"for his contributed WAV header code\n"
		"(files wavfile.c and wavfile.h)\n"
		"---\n"
		"Thanks also go to Andre Fuechsel for his original\n"
		"version that inspired the current work (though\n"
		"this version does not contain his code)\n"
		"---\n"
		"Many thanks to the LINUX folks,\n"
		"the XFree86 folks,\n"
		"and the LessTif group\n"
		"for making this application possible.\n"
		"---\n"
		"This program comes with\n"
		"ABSOLUTELY NO WARRANTY.\n"
	);
}

/*
 * Error reporting function:
 */
void
v_erf(const char *format,va_list ap) {
	vfprintf(stderr,format,ap);
	fputc('\n',stderr);
}

/*
 * General printf() styled error print function:
 */
void
err(const char *format,...) {
	va_list ap;

	va_start(ap,format);
	v_erf(format,ap);
	va_end(ap);
}

/*
 * Determine the basename of the command, and the operation mode
 * for this run.
 */
static OprMode
getOprMode(const char *pathname,char **cmdBasename,int *p_ipcid) {
	char *basename = strrchr(pathname,'/');
	unsigned short x;

	/*
	 * Test if we are in server mode: If so, argv[0] is "WAVSVR=#", where
	 * # is the IPC ID of the private message queue to use for communications.
	 */
	*p_ipcid = -1;	

	if ( !strncmp(pathname,"WAVSVR=",7) ) {
		sscanf(pathname+7,"%d",p_ipcid);	/* Get server IPC ID of message queue */
		return OprServer;			/* We are in server mode */
	}

	/*
	 * Non server modes:
	 */
	if ( basename != NULL )
		*cmdBasename = basename + 1;		/* Return pointer to command's basename */
	else	*cmdBasename = (char *) pathname;	/* Otherwise, only basename was given */

	basename = *cmdBasename;

	for ( x=0; x < strlen(basename); ++x )
		if ( !strncmp(basename+x,"rec",3)
		||   !strncmp(basename+x,"Rec",3)
		||   !strncmp(basename+x,"REC",3) )
			return OprRecord;		/* This must be wavrecord command */

	return OprPlay;					/* Otherwise assume play mode */
}

/*
 * Main program:
 */
int
main(int argc,char **argv) {
	WavPlayOpts wavopts;			/* WAV Play options */
	char *cmd_name;				/* Basename of the command used */
	int optch;				/* option flag character */
	int rc;					/* Return code */
	int fd;					/* Temporary file descriptor */
	double d;				/* Temporary double value */
	static char cmdopts[] = {
		OPF_INFO, OPF_HELP, OPF_QUIET, OPF_SAMPRATE,':', OPF_STEREO, OPF_MONO,
		OPF_TIME,':', OPF_DATABITS,':', OPF_IPCKEY,':', OPF_RESET,
		OPF_PLAY_LOCK, OPF_PLAY_UNLOCK, OPF_RECD_LOCK, OPF_RECD_UNLOCK,
		OPF_DEBUG, OPF_VERSION, 0 };
#ifndef USE_GETOPT_STD
	int optx;				/* Option index */
	static struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },	/* --help	== -h */
		{ "version", no_argument, NULL, 'V' },	/* --version	== -V */
		{ NULL, 0, 0, 0 }
	};
#endif
	/*
	 * Initialize all wavplay options:
	 */
	memset(&wavopts,0,sizeof wavopts);	/* Zero this structure */
	wavopts.IPCKey = AUDIOLCK;		/* Default IPC Key for lock */
	wavopts.Mode = getOprMode(argv[0],&cmd_name,&clntIPC);
	wavopts.Channels.optChar = 0;
	wavopts.ipc = -1;			/* Semaphore ipc ID */
	wavopts.DataBits.optValue = 16;		/* Default to 16 bits */
	wavopts.Channels.optValue = Stereo;
	wavopts.SamplingRate.optValue = 8000;

	/*
	 * Parse command line options:
	 */
#ifndef USE_GETOPT_STD
	while( (optch = getopt_long(argc,argv,cmdopts,long_opts,&optx)) != -1 )
#else
	while( (optch = getopt(argc,argv,cmdopts)) != -1 )
#endif
		switch ( optch ) {

		case OPF_PLAY_LOCK :		/* -l ; lock play lock request */
		case OPF_PLAY_UNLOCK :		/* -u ; unlock play lock request */
			wavopts.PlayLock.optChar = (char) optch;
			break;

		case OPF_RECD_LOCK :		/* -L ; lock record lock request */
		case OPF_RECD_UNLOCK :		/* -U ; unlock record lock request */
			wavopts.RecdLock.optChar = (char) optch;
			break;

		case OPF_RESET:			/* -R ; reset both locks request */
			wavopts.bResetLocks = 1;
			break;

		case OPF_IPCKEY:		/* -k IPCkey ; Specify the IPC key of the semaphores */
			wavopts.IPCKey = (key_t) atol(optarg);
			break;

		case OPF_QUIET:			/* -q ; Quiet mode of operation */
			wavopts.bQuietMode = 1;
			break;

		case OPF_SAMPRATE:		/* -s samp_rate ; Specify the sampling rate (Hz) */
			wavopts.SamplingRate.optChar = optch;
			wavopts.SamplingRate.optValue = atoi(optarg);
			break;

		case OPF_STEREO:		/* -S ; sets stereo (undoes -B) */
			wavopts.Channels.optChar = optch;
			wavopts.Channels.optValue = Stereo;
			break;

		case OPF_MONO:			/* -M ; sets mono (undoes -S) */
			wavopts.Channels.optChar = optch;
			wavopts.Channels.optValue = Stereo;
			break;

		case OPF_INFO:			/* -i ; info mode */
			wavopts.bInfoMode = optch;
			break;

		case OPF_TIME:			/* -t secs ; limit play to n seconds */
			if ( sscanf(optarg,"%lf",&d) != 1 ) {
				err("Invalid argument: -t %s\n",optarg);
				exit(1);
			}
			wavopts.Seconds = (UInt32) d;
			break;

		case OPF_DATABITS:		/* -b bits ; Samples are n bits each */
			wavopts.DataBits.optChar = optch;
			wavopts.DataBits.optValue = atoi(optarg);
			break;

		case OPF_DEBUG :
			cmdopt_x = 1;		/* -x ; debug turned on */
			break;

		case OPF_HELP:			/* -h ; A plea for help */
			usage(cmd_name,wavopts.Mode);
			exit(0);
			
		case OPF_VERSION:
			version();
			exit(0);
			
		default:			/* A mishap on the command line */
			usage(cmd_name,wavopts.Mode);
			exit(1);
		}

	/*
	 * If in server mode, go wait for instructions:
	 */
	if ( wavopts.Mode == OprServer )
		return Server(wavopts.IPCKey);	/* wait for X Window client to speak */

	/*
	 * Check the arguments (non-server mode) :
	 */
	assert(wavopts.Mode == OprRecord || wavopts.Mode == OprPlay);

	/*
	 * Apply quiet mode by redirecting output:
	 */
	if ( wavopts.bQuietMode != 0 ) {
		if ( (fd = open("/dev/null",O_WRONLY,0)) > 0 ) {
			close(1);
			close(2);
			dup2(fd,1);
			dup2(fd,2);
			close(fd);
		} else	fprintf(stderr,"%s: cannot go quiet due to /dev/null\n",
				sys_errlist[errno]);
	}

	/*
	 * Check if -i used in record mode:
	 */
	if ( wavopts.Mode == OprRecord && wavopts.bInfoMode ) {
		err("-i option does not make sense in record mode.");
		exit(1);
	}

	/*
	 * Check data bits option:
	 */
	if ( wavopts.DataBits.optChar != 0 )
		switch ( wavopts.DataBits.optValue ) {
		case 8 :
		case 16 :
			break;				/* 8/16 bits are ok */
		default :
			err("Cannot do %u bit samples: but can do 8 or 16.\n",
				wavopts.DataBits.optValue);
			exit(1);
		}

	/*
	 * Obtain semaphores for locking if IPCKey != 0 :
	 */
	if ( wavopts.IPCKey && (wavopts.ipc = OpenDSPLocks(wavopts.IPCKey,
	     !(wavopts.PlayLock.optChar | wavopts.RecdLock.optChar),v_erf)) < 0 ) {
		err("%s: Unable to get audio locking semaphores.",sys_errlist[errno]);
		exit(1);
	}

	/*
	 * If -r was given above, then reset the semaphore locks:
	 */
	if ( wavopts.ipc >= 0 && wavopts.bResetLocks ) {
		/*
		 * Remove old semaphore set :
		 */
		if ( semctl(wavopts.ipc,0,IPC_RMID,NULL) < 0 ) {
			err("%s: Unable to remove old locking semaphores.",sys_errlist[errno]);
			exit(1);
		}

		/*
		 * Cancel -u or -U if given :
		 */
		if ( wavopts.PlayLock.optChar == OPF_PLAY_UNLOCK )
			wavopts.PlayLock.optChar = 0;		/* Cancel the unecessary -u option */

		if ( wavopts.RecdLock.optChar == OPF_RECD_UNLOCK )
			wavopts.RecdLock.optChar = 0;		/* Cancel the unecessary -U option */

		/*
		 * Get replacement semaphores :
		 */
		if ( wavopts.IPCKey && (wavopts.ipc = OpenDSPLocks(wavopts.IPCKey,0,v_erf)) < 0 ) {
			err("%s: Unable to get audio locking semaphores.",sys_errlist[errno]);
			exit(1);
		}
	}

	/*
	 * -u play unlock request :
	 */
	if ( wavopts.PlayLock.optChar == OPF_PLAY_UNLOCK && UnlockDSP(wavopts.ipc,0,v_erf) ) {
		err("%s: -%c unlock request failed.",sys_errlist[errno],OPF_PLAY_UNLOCK);
		exit(1);
	}

	/*
	 * -U record unlock request :
	 */
	if ( wavopts.RecdLock.optChar == OPF_RECD_UNLOCK && UnlockDSP(wavopts.ipc,1,v_erf) ) {
		err("%s: -%c unlock request failed.",sys_errlist[errno],OPF_RECD_UNLOCK);
		exit(1);
	}

	/*
	 * -l play lock request :
	 */
	if ( wavopts.PlayLock.optChar == OPF_PLAY_LOCK && LockDSP(wavopts.ipc,0,v_erf,0) ) {
		err("%s: -%c lock request failed.",sys_errlist[errno],OPF_PLAY_LOCK);
		exit(1);
	}

	/*
	 * -L record lock request :
	 */
	if ( wavopts.RecdLock.optChar == OPF_RECD_LOCK && LockDSP(wavopts.ipc,1,v_erf,0) ) {
		err("%s: -%c lock request failed.",sys_errlist[errno],OPF_RECD_LOCK);
		exit(1);
	}

	/*
	 * If any of -l, -u, -L or -U are given, then we exit(0) here
	 */
	if ( wavopts.PlayLock.optChar || wavopts.RecdLock.optChar != 0 )
		exit(0);		/* Just lock/unlock functions */

	/*
	 * Record or Play :
	 */
	if ( wavopts.Mode == OprRecord ) {
		/*
		 * Set default RECORD options :
		 */
		if ( !wavopts.Seconds )
			wavopts.Seconds = 10;			/* Default is 10 seconds */

		if ( !wavopts.Channels.optChar ) {
			wavopts.Channels.optChar = 1;
			wavopts.Channels.optValue = Mono;	/* Default to mono */
		}

		if ( !wavopts.SamplingRate.optChar ) {
			wavopts.SamplingRate.optChar = 1;
			wavopts.SamplingRate.optValue = 22050;	/* Default to 22050 Hz */
		}

		if ( !wavopts.DataBits.optChar ) {
			wavopts.DataBits.optChar = 1;
			wavopts.DataBits.optChar = 8;		/* Default to 8 bits */
		}
	}

	/*
	 * Now do the record/play session
	 */
	rc = recplay(&wavopts,&argv[optind],v_erf);
	return rc;
}

/* $Source: /wwg/motif/wavplay.RCS/RCS/main.c,v $ */
