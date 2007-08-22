/* $Header: /wwg/motif/xltwavplay/RCS/locks.c,v 1.1 1997/04/14 00:19:33 wwg Exp $
 * Warren W. Gay VE3WWG		Sat May 11 15:01:58 1996
 * 
 * MANAGE LOCKS ON THE AUDIO DEVICE:
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
 * $Log: locks.c,v $
 * Revision 1.1  1997/04/14 00:19:33  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)locks.c $Revision: 1.1 $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "wavplay.h"


union semun {
	ushort *array;
};


static int SemUndo = SEM_UNDO;
static ErrFunc v_erf;				/* Error reporting function */
static bTimedOut = 0;				/* True if SIGALRM is raised */

/*
 * SIGALRM catcher:
 */
static void
catch_sigalrm(int signo) {
	bTimedOut = 1;				/* Mark as timed out */
}

/*
 * Error reporting function for this source module:
 */
static void
err(const char *format,...) {
	va_list ap;

	if ( v_erf == NULL )
		return;				/* Only report error if we have function */
	va_start(ap,format);
	v_erf(format,ap);			/* Use caller's supplied function */
	va_end(ap);
}

/*
 * OpenDSPLocks  opens the existing set of 2 semaphores, or creates
 * and  initializes  the new set  if  it  does  not yet exist. This
 * program   chooses   not   to   release  the  IPC  resource  upon
 * termination,  since the  semphore creation and initialization is
 * not  an atomic operation, and can be prone to failure.  However,
 * it  the  IPC resource is removed by some other means (like ipcrm
 * command),  this  function  call  will create a new set using the
 * supplied IPC Key 'LockIPCKey'.
 *
 * NOTES:
 *
 * Two semphores are created in this set. Semaphore # 0 is intended
 * to be used as a /dev/dsp 'input lock' (play lock), and semaphore
 * # 1 is the /dev/dsp 'output lock' (record lock).  I doubt if the
 * record lock  is very useful, but there might be situations where
 * it is needed.
 *
 * Class F Reporting:
 */
int
OpenDSPLocks(key_t LockIPCKey,int SemUndoFlag,ErrFunc erf) {
	int ipc;
	int s;
	int e;
	int safety = 3;
	union semun u;
	static ushort init_sems[2] = { 1, 1 };
	
	v_erf = erf;					/* Set error reporting function */

	/*
	 * Set the SEM_UNDO status :
	 */
	SemUndo = SemUndoFlag ? SEM_UNDO : 0;

	/*
	 * If semaphore already exists, just use it :
	 */
	while ( (ipc = semget(LockIPCKey,2,0666)) < 0 && --safety >= 0 ) {	

		/*
		 * Failed to find it, try creating it :
		 */
		if ( (ipc = semget(LockIPCKey,2,IPC_CREAT|IPC_EXCL|0666)) < 0 && errno != EEXIST ) {
			err("Unable to create a semaphore set for key 0x%lX",
				sys_errlist[errno],
				LockIPCKey);
			return -1;	/* No system IPC resources? */
		}

		/*
		 * Set already exists- timing error? Try again.
		 */
		if ( ipc < 0 ) {
			sleep(1);	/* Allow creator time to init sems */
			continue;	/* Try again */
		}

		/*
		 * We created the semaphore set - initialize so that
		 * each semaphore has the value 1 :
		 */
		u.array = &init_sems[0];

		if ( (s = semctl(ipc,0,SETALL,u)) < 0 && errno == EIDRM ) {
			/*
			 * Another process removed our ipc resource :
			 */
			continue;	/* Try again */
		}

		if ( s < 0 ) {
			/*
			 * We failed to initialize!
			 */
			e = errno;			/* Save error */
			semctl(ipc,0,IPC_RMID,NULL);	/* Destroy bad sems */
			err("%s: Unable to initialize semaphore set values",sys_errlist[errno=e]);
			return -1;			/* Return err ind. */
		}
	}

	return ipc;	/* Return existing, or newly initialized sems */
}

/*
 * Lock the /dev/dsp device :
 *
 * playrecx :
 *	0	play lock
 *	1	record lock
 */
int
LockDSP(int ipc,int playrecx,ErrFunc erf,unsigned timeout_secs) {
	int s;
	int e;
	static struct sembuf sops[1] = { { 0, -1, SEM_UNDO } };

	v_erf = erf;					/* Set error reporting function */

	sops[0].sem_num = playrecx;
	sops[0].sem_flg = SemUndo;

	bTimedOut = 0;					/* Reset the timeout flag */

	if ( timeout_secs > 0 ) {
		signal(SIGALRM,catch_sigalrm);		/* Prepare to catch SIGALRM */
		alarm(timeout_secs);			/* Start the timer */
	}

	while ( (s = semop(ipc,sops,1)) < 0 && errno == EINTR )
		if ( bTimedOut ) {
			err("Timed out: locking the %s semaphore",playrecx?"Record":"Play");
			errno = EAGAIN;			/* Mark as timed out */
			s = -1;
			goto xit;
		}

	if ( s < 0 )
		err("%s: Locking the %s semaphore",sys_errlist[errno],playrecx?"Record":"Play");

	/*
	 * Exit this procedure:
	 */
xit:	e = errno;					/* Preserve errno for this exit */
	alarm(0);
	signal(SIGALRM,SIG_DFL);
	errno = e;					/* Restore errno */
	return s;
}

/*
 * Lock the /dev/dsp device :
 *
 * playrecx :
 *	0	play lock
 *	1	record lock
 */
int
UnlockDSP(int ipc,int playrecx,ErrFunc erf) {
	int s;
	static struct sembuf sops[1] = { { 0, +1, SEM_UNDO } };

	v_erf = erf;					/* Set error reporting function */

	sops[0].sem_num = playrecx;
	sops[0].sem_flg = SemUndo;

	while ( (s = semop(ipc,sops,1)) < 0 && errno == EINTR ) ;

	if ( s < 0 )
		err("%s: Unlocking the %s semaphore",
			sys_errlist[errno],
			playrecx?"Record":"Play");

	return s;
}

/* $Source: /wwg/motif/xltwavplay/RCS/locks.c,v $ */
