/* $Header: /wwg/motif/xltwavplay/RCS/file.c,v 1.2 1997/04/14 00:49:07 wwg Exp $
 * Warren W. Gay VE3WWG		Sun Feb 16 20:43:59 1997
 *
 * WAV FILE OPERATIONS:
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
 * $Log: file.c,v $
 * Revision 1.2  1997/04/14 00:49:07  wwg
 * Added ioctl(fd,SNDCTL_DSP_SYNC) calls
 *
 * Revision 1.1  1997/04/14 00:12:15  wwg
 * Initial revision
 *
 */
static char rcsid[] = "@(#)file.c $Revision: 1.2 $";

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
#include <sys/ioctl.h>
#include <assert.h>
#include <linux/soundcard.h>
#include "wavplay.h"

static ErrFunc v_erf;				/* This module's error reporting function */
static char emsg[2048];

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

static void
_v_erf(const char *format,va_list ap) {
	vsprintf(emsg,format,ap);		/* Capture message into emsg[] */
}

/*
 * Internal routine to allocate WAVFILE structure:
 */
static WAVFILE *
wavfile_alloc(const char *Pathname) {
	WAVFILE *wfile = (WAVFILE *) malloc(sizeof (WAVFILE));

	if ( wfile == NULL ) {
		err("%s: Allocating WAVFILE structure",sys_errlist[ENOMEM]);
		return NULL;
	}

	memset(wfile,0,sizeof *wfile);

	if ( (wfile->Pathname = strdup(Pathname)) == NULL ) {
		free(wfile);
		err("%s: Allocating storage for WAVFILE.Pathname",sys_errlist[ENOMEM]);
		return NULL;
	}

	wfile->fd = -1;				/* Initialize fd as not open */
	wfile->wavinfo.Channels = Mono;
	wfile->wavinfo.DataBits = 8;

	return wfile;
}

/*
 * Internal routine to release WAVFILE structure:
 * No errors reported.
 */
static void
wavfile_free(WAVFILE *wfile) {
	if ( wfile->Pathname != NULL )
		free(wfile->Pathname);
	free(wfile);
}

/*
 * Open a WAV file for reading: returns (WAVFILE *)
 *
 * The opened file is positioned at the first byte of WAV file data, or
 * NULL is returned if the open is unsuccessful.
 */
WAVFILE *
WavOpenForRead(const char *Pathname,ErrFunc erf) {
	WAVFILE *wfile = wavfile_alloc(Pathname);
	int e;					/* Saved errno value */
	UInt32 offset;				/* File offset */
	Byte ubuf[4];				/* 4 byte buffer */
	UInt32 dbytes;				/* Data byte count */
						/* wavfile.c values : */
	int channels;				/* Channels recorded in this wav file */
	u_long samplerate;			/* Sampling rate */
	int sample_bits;			/* data bit size (8/12/16) */
	u_long samples;				/* The number of samples in this file */
	u_long datastart;			/* The offset to the wav data */

	v_erf = erf;				/* Set error reporting function */

	if ( wfile == NULL )
		return NULL;			/* Insufficient memory (class B msg) */

	/*
	 * Open the file for reading:
	 */
	if ( (wfile->fd = open(wfile->Pathname,O_RDONLY)) < 0 ) {
		err("%s:\nOpening WAV file %s",
			sys_errlist[errno],
			wfile->Pathname);
		goto errxit;
	}

	if ( lseek(wfile->fd,0L,SEEK_SET) != 0L ) {
		err("%s:\nRewinding WAV file %s",
			sys_errlist[errno],
			wfile->Pathname);
		goto errxit;		/* Wav file must be seekable device */
	}

	if ( (e = WaveReadHeader(wfile->fd,&channels,&samplerate,&sample_bits,&samples,&datastart,_v_erf)) != 0 ) {
		err("%s:\nReading WAV header from %s",
			emsg,
			wfile->Pathname);
		goto errxit;
	}

	/*
	 * Copy WAV data over to WAVFILE struct:
	 */
	if ( channels == 2 )
		wfile->wavinfo.Channels = Stereo;
	else	wfile->wavinfo.Channels = Mono;

	wfile->wavinfo.SamplingRate = (UInt32) samplerate;
	wfile->wavinfo.Samples = (UInt32) samples;
	wfile->wavinfo.DataBits = (UInt16) sample_bits;
	wfile->wavinfo.DataStart = (UInt32) datastart;
	wfile->rw = 'R';			/* Read mode */

	offset = wfile->wavinfo.DataStart - 4;

	/*
	 * Seek to byte count and read dbytes:
	 */
	if ( lseek(wfile->fd,offset,SEEK_SET) != offset ) {
		err("%s:\nSeeking to WAV data in %s",sys_errlist[errno],wfile->Pathname);
		goto errxit;			/* Seek failure */
	}

	if ( read(wfile->fd,ubuf,4) != 4 ) {
		err("%s:\nReading dbytes from %s",sys_errlist[errno],wfile->Pathname);
		goto errxit;
	}

	/*
	 * Put little endian value into 32 bit value:
	 */
	dbytes = ubuf[3];
	dbytes = (dbytes << 8) | ubuf[2];
	dbytes = (dbytes << 8) | ubuf[1];
	dbytes = (dbytes << 8) | ubuf[0];

	wfile->wavinfo.DataBytes = dbytes;

	/*
	 * Open succeeded:
	 */
	return wfile;				/* Return open descriptor */

	/*
	 * Return error after failed open:
	 */
errxit:	e = errno;				/* Save errno */
	free(wfile->Pathname);			/* Dispose of copied pathname */
	free(wfile);				/* Dispose of WAVFILE struct */
	errno = e;				/* Restore error number */
	return NULL;				/* Return error indication */
}

/*
 * Apply command line option overrides to the interpretation of the input
 * wav file:
 *
 */
void
WavReadOverrides(WAVFILE *wfile,WavPlayOpts *wavopts) {

	/*
	 * Override sampling rate: -s sampling_rate
	 */
	if ( wavopts->SamplingRate.optChar != 0 ) {
		wfile->wavinfo.SamplingRate = wavopts->SamplingRate.optValue;
		wfile->wavinfo.bOvrSampling = 1;
	}

	/*
	 * Override mono/stereo mode: -S / -M
	 */
	if ( wavopts->Channels.optChar != 0 ) {
		wfile->wavinfo.Channels = wavopts->Channels.optValue;
		wfile->wavinfo.bOvrMode = 1;
	}
	
	/*
	 * Override the sample size in bits: -b bits
	 */
	if ( wavopts->DataBits.optChar != 0 ) {
		wfile->wavinfo.DataBits = wavopts->DataBits.optValue;
		wfile->wavinfo.bOvrBits = 1;
	}

	/*
	 * Override # of samples if -t seconds option given:
	 */
	if ( wavopts->Seconds != 0 )
		wfile->wavinfo.Samples = wavopts->Seconds * wfile->wavinfo.SamplingRate;
}

/*
 * Close a WAVFILE
 */
int
WavClose(WAVFILE *wfile,ErrFunc erf) {
	int e = 0;				/* Returned error code */
	int channels;				/* Channels recorded in this wav file */
	u_long samplerate;			/* Sampling rate */
	int sample_bits;			/* data bit size (8/12/16) */
	u_long samples;				/* The number of samples in this file */
	u_long datastart;			/* The offset to the wav data */

	v_erf = erf;				/* Set error reporting function */

	if ( wfile == NULL ) {
		err("%s: WAVFILE pointer is NULL!",sys_errlist[EINVAL]);
		errno = EINVAL;
		return -1;
	}

	/*
	 * If the wav file was open for write, update the actual number
	 * of samples written to the file:
	 */
	if ( wfile->rw == 'W' ) {
		if ( (e = WaveReadHeader(wfile->fd,&channels,&samplerate,&sample_bits,&samples,&datastart,_v_erf)) != 0 )
			err("%s:\nReading WAV header from %s",emsg,wfile->Pathname);
		else if ( lseek(wfile->fd,(long)(datastart-4),SEEK_SET) != (long)(datastart-4) )
			err("%s:\nSeeking in WAV header file %s",sys_errlist[errno],wfile->Pathname);
		else if ( write(wfile->fd,&wfile->wavinfo.Samples,sizeof wfile->wavinfo.Samples) != sizeof wfile->wavinfo.Samples )
			err("%s:\nWriting in WAV header file %s",sys_errlist[errno],wfile->Pathname);
		/* Else succeeded ok */
	}

	if ( close(wfile->fd) < 0 ) {
		err("%s:\nClosing WAV file",sys_errlist[errno]);
		e = errno;			/* Save errno value to return */
	}

	wavfile_free(wfile);			/* Release WAVFILE structure */

	if ( (errno = e) != 0 )
		return -1;			/* Failed exit */
	return 0;				/* Successful exit */
}

/*
 * Open a WAV file for writing:
 */
WAVFILE *
WavOpenForWrite(const char *Pathname,Chan chmode,UInt32 sample_rate,UInt16 bits,UInt32 samples,ErrFunc erf) {
	WAVFILE *wfile = wavfile_alloc(Pathname);
	int e;					/* Saved errno value */

	v_erf = erf;				/* Set error reporting function */

	if ( wfile == NULL )
		return NULL;			/* ENOMEM (class_b msg) */

	wfile->rw = 'W';			/* Mark as for writing */
	wfile->wavinfo.SamplingRate = sample_rate;
	wfile->wavinfo.Channels = chmode;	/* Mono / Stereo */
	wfile->wavinfo.Samples = samples;
	wfile->wavinfo.DataBits = bits;

	/*
	 * Open/create the file for writing:
	 */
	if ( (wfile->fd = open(wfile->Pathname,O_RDWR|O_TRUNC|O_CREAT,0666)) < 0 ) {
		err("%s:\nOpening %s for WAV writing",
			sys_errlist[errno],
			wfile->Pathname);
		return NULL;			/* Open error */
	}

	/*
	 * Write out a WAV file header:
	 */
	e = WaveWriteHeader(wfile->fd,
		wfile->wavinfo.Channels == Mono ? 1 : 2,
		wfile->wavinfo.SamplingRate,
		wfile->wavinfo.DataBits,
		wfile->wavinfo.Samples,
		_v_erf);			/* Capture error messages to emsg[] */

	if ( e != 0 ) {
		wavfile_free(wfile);
		err("%s:\nWriting WAV header to %s",
			emsg,
			wfile->Pathname);
		return NULL;
	}

	/*
	 * Return successfuly opened file:
	 */
	return wfile;
}

/*
 * Open /dev/dsp for reading or writing:
 */
DSPFILE *
OpenDSP(WAVFILE *wfile,int omode,ErrFunc erf) {
	int e;					/* Saved errno value */
	int t;					/* Work int */
	unsigned long ul;			/* Work unsigned long */
	DSPFILE *dfile = (DSPFILE *) malloc(sizeof (DSPFILE));

	v_erf = erf;				/* Set error reporting function */

	if ( dfile == NULL ) {
		err("%s: Opening DSP device",sys_errlist[errno=ENOMEM]);
		return NULL;
	}

	memset(dfile,0,sizeof *dfile);
	dfile->dspbuf = NULL;

	/*
	 * Open the device driver:
	 */
	if ( (dfile->fd = open(AUDIODEV,omode,0)) < 0 ) {
		err("%s:\nOpening audio device %s",
			sys_errlist[errno],
			AUDIODEV);
		goto errxit;
	}

	/*
	 * Determine the audio device's block size:
	 */
	if ( ioctl(dfile->fd,SNDCTL_DSP_GETBLKSIZE,&dfile->dspblksiz) < 0 ) {
		err("%s: Optaining DSP's block size",sys_errlist[errno]);
		goto errxit;
	}

	/*
	 * Check the range on the buffer sizes:
	 */
	if ( dfile->dspblksiz < 4096 || dfile->dspblksiz > 65536 ) {
		err("%s: Audio block size (%d bytes)",
			sys_errlist[errno=EINVAL],
			(int)dfile->dspblksiz);
		goto errxit;
	} 

	/*
	 * Allocate a buffer to do the I/O through:
	 */
	if ( (dfile->dspbuf = (char *) malloc(dfile->dspblksiz)) == NULL ) {
		err("%s: For DSP I/O buffer",sys_errlist[errno]);
		goto errxit;
	}

	/*
	 * Set the data bit size:
	 */
	t = wfile->wavinfo.DataBits;
        if ( ioctl(dfile->fd,SNDCTL_DSP_SAMPLESIZE,&t) < 0 ) {
		err("%s: Setting DSP to %u bits",sys_errlist[errno],(unsigned)t);
		goto errxit;
	}

	/*
	 * Set the mode to be Stereo or Mono:
	 */
	t = wfile->wavinfo.Channels == Stereo ? 1 : 0;
	if ( ioctl(dfile->fd,SNDCTL_DSP_STEREO,&t) < 0 ) {
		err("%s: Unable to set DSP to %s mode",
			sys_errlist[errno],
			t?"Stereo":"Mono");
		goto errxit;
	}		
      
	/*
	 * Set the sampling rate:
	 */
	ul = wfile->wavinfo.SamplingRate;
	if ( ioctl(dfile->fd,SNDCTL_DSP_SPEED,&ul) < 0 ) {
		err("Unable to set audio sampling rate",sys_errlist[errno]);
		goto errxit;
	}

	/*
	 * Return successfully opened device:
	 */
	return dfile;				/* Return file descriptor */

	/*
	 * Failed to open/initialize properly:
	 */
errxit:	e = errno;				/* Save the errno value */
	if ( dfile->fd >= 0 )
		close(dfile->fd);		/* Close device */
	if ( dfile->dspbuf != NULL )
		free(dfile->dspbuf);
	free(dfile);
	errno = e;				/* Restore error code */
	return NULL;				/* Return error indication */
}

/*
 * Close the DSP device:
 */
int
CloseDSP(DSPFILE *dfile,ErrFunc erf) {
	int fd;

	v_erf = erf;				/* Set error reporting function */

	if ( dfile == NULL ) {
		err("%s: DSPFILE is not open",sys_errlist[errno=EINVAL]);
		return -1;
	}

	fd = dfile->fd;
	if ( dfile->dspbuf != NULL )
		free(dfile->dspbuf);
	free(dfile);
	
	if ( close(fd) ) {
		err("%s: Closing DSP fd %d",sys_errlist[errno],fd);
		return -1;
	}

	return 0;
}

/*
 * Play DSP from WAV file:
 */
int
PlayDSP(DSPFILE *dfile,WAVFILE *wfile,DSPPROC work_proc,ErrFunc erf) {
	UInt32 byte_count = (UInt32) wfile->wavinfo.Samples;
	int bytes;
	int n;
	int byte_modulo;

	v_erf = erf;				/* Set error reporting function */

	/*
	 * Check that the WAVFILE is open for reading:
	 */
	if ( wfile->rw != 'R' ) {
		err("%s: WAVFILE must be open for reading",sys_errlist[errno=EINVAL]);
		return -1;
	}

	/*
	 * First determine how many bytes are required for each channel's sample:
	 */
	switch ( wfile->wavinfo.DataBits ) {
	case 8 :
		byte_count = 1;
		break;
	case 16 :
		byte_count = 2;
		break;
	default :
		err("%s: Cannot process %u bit samples",
			sys_errlist[errno=EINVAL],
			(unsigned)wfile->wavinfo.DataBits);
		return -1;
	}

	/*
	 * Allow for Mono/Stereo difference:
	 */
	if ( wfile->wavinfo.Channels == Stereo )
		byte_count *= 2;		/* Twice as many bytes for stereo */
	else if ( wfile->wavinfo.Channels != Mono ) {
		err("%s: DSPFILE control block is corrupted (chan_mode)",
			sys_errlist[errno=EINVAL]);
		return -1;
	}		

	byte_modulo = byte_count;				/* This many bytes per sample */
	byte_count  = wfile->wavinfo.Samples * byte_modulo;	/* Total bytes to process */

	if ( ioctl(dfile->fd,SNDCTL_DSP_SYNC,0) != 0 )
		err("%s: ioctl(%d,SNDCTL_DSP_SYNC,0)",sys_errlist[errno]);

	for ( ; byte_count > 0 && wfile->wavinfo.DataBytes > 0; byte_count -= (UInt32) n ) {

		bytes = (int) ( byte_count > dfile->dspblksiz ? dfile->dspblksiz : byte_count );

		if ( bytes > wfile->wavinfo.DataBytes )	/* Size bigger than data chunk? */
			bytes = wfile->wavinfo.DataBytes;	/* Data chunk only has this much left */

		if ( (n = read(wfile->fd,dfile->dspbuf,bytes)) != bytes ) {
			if ( n >= 0 )
				err("Unexpected EOF reading samples from WAV file",sys_errlist[errno=EIO]);
			else	err("Reading samples from WAV file",sys_errlist[errno]);
			goto errxit;
		}

		if ( write(dfile->fd,dfile->dspbuf,n) != n ) {
			err("Writing samples to audio device",sys_errlist[errno]);
			goto errxit;
		}

		wfile->wavinfo.DataBytes -= (UInt32) bytes;	/* We have fewer bytes left to read */

		/*
		 * The work procedure function is called when operating
		 * in server mode to check for more server messages:
		 */
		if ( work_proc != NULL && work_proc(dfile) )	/* Did work_proc() return TRUE? */
			break;					/* Yes, quit playing */
	}
			
	if ( ioctl(dfile->fd,SNDCTL_DSP_SYNC,0) != 0 )
		err("%s: ioctl(%d,SNDCTL_DSP_SYNC,0)",sys_errlist[errno]);

	return 0;	/* All samples played successfully */

errxit:	return -1;	/* Indicate error return */
}

/*
 * Record DSP to WAV file: If samples == 0UL, then record continues until
 * a bRecordStopPosted becomes true (via SIGINT).
 */
int
RecordDSP(DSPFILE *dfile,WAVFILE *wfile,UInt32 samples,DSPPROC work_proc,ErrFunc erf) {
	UInt32 byte_count = (UInt32) wfile->wavinfo.Samples;
	UInt32 chunk;
	int bytes;
	int n;
	UInt32 bytes_per_sample = 0;
	UInt32 bytes_written = 0;

	v_erf = erf;				/* Set error reporting function */

	/*
	 * Check that the WAVFILE is open for writing:
	 */
	if ( wfile->rw != 'W' ) {
		err("WAVFILE must be open for writing",sys_errlist[errno=EINVAL]);
		return -1;
	}

	/*
	 * First determine how many bytes are required for each channel's sample:
	 */
	switch ( wfile->wavinfo.DataBits ) {
	case 8 :
		byte_count = 1;
		break;
	case 12 :
	case 16 :
		byte_count = 2;
		break;
	default :
		err("Cannot process %u bit samples",
			sys_errlist[errno=EINVAL],
			(unsigned)wfile->wavinfo.DataBits);
		return -1;
	}

	/*
	 * Allow for Mono/Stereo difference:
	 */
	if ( wfile->wavinfo.Channels == Stereo )
		byte_count *= 2;		/* Twice as many bytes for stereo */
	else if ( wfile->wavinfo.Channels != Mono ) {
		err("DSPFILE control block is corrupted (chan_mode)",sys_errlist[errno=EINVAL]);
		return -1;
	}		

	bytes_per_sample = byte_count;		/* Save for close later */

	if ( samples > 0 )
		byte_count *= wfile->wavinfo.Samples;	/* Total number of bytes to collect */
	else	{
		/* Compute a smallish sized chunk of the correct modulo bytes */
		chunk = dfile->dspblksiz > 4096 ? 4096 : dfile->dspblksiz;
		byte_count = (chunk + (byte_count-1)) & ~(byte_count-1);
	}

	while ( 1 ) {
		/*
		 * Determine how many samples to read:
		 */
		if ( samples > 0 )
			bytes = byte_count > dfile->dspblksiz ? dfile->dspblksiz : byte_count;
		else	bytes = byte_count;	/* Record until interrupted */

		/*
		 * Read a block of samples:
		 */
		if ( (n = read(dfile->fd,dfile->dspbuf,bytes)) < 0 ) {
			err("Reading DSP device",sys_errlist[errno]);
			goto errxit;
		} else if ( n == 0 )
			break;
			
		/*
		 * Write a block of samples to the file:
		 */
		if ( (bytes = write(wfile->fd,dfile->dspbuf,n)) < 0 ) {
			err("Writing WAV samples to %s",sys_errlist[errno],wfile->Pathname);
			goto errxit;
		} else if ( bytes != n ) {
			if ( bytes > 0 )
				bytes_written += bytes;
			err("Did not write all WAV successfully",sys_errlist[errno=EIO]);
			goto errxit;
		}

		bytes_written += bytes;

		if ( samples > 0 )
			if ( (byte_count -= (unsigned) n) < 1 )
				break;

		/*
		 * The work procedure function is called when operating
		 * in server mode to check for more server messages:
		 */
		if ( work_proc != NULL && work_proc(dfile) )	/* Did work_proc() return TRUE? */
			break;					/* Yes, quit recording */
	}

	wfile->wavinfo.Samples = bytes_written & ~(bytes_per_sample-1);
	return 0;	/* All samples played successfully */

errxit:	wfile->wavinfo.Samples = bytes_written & ~(bytes_per_sample-1);
	return -1;	/* Indicate error return */
}

/* $Source: /wwg/motif/xltwavplay/RCS/file.c,v $ */
