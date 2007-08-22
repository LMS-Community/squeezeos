/* $Header: /wwg/motif/xltwavplay/RCS/wavfile.c,v 1.1 1997/04/14 00:14:38 wwg Exp $
 * Copyright: wavfile.c (c) Erik de Castro Lopo  erikd@zip.com.au
 *
 * wavfile.c - Functions for reading and writing MS-Windoze .WAV files.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *	
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *	
 *	This code was originally written to manipulate Windoze .WAV files
 *	under i386 Linux. Please send any bug reports or requests for 
 *	enhancements to : erikd@zip.com.au
 *	
 * $Log: wavfile.c,v $
 * Revision 1.1  1997/04/14 00:14:38  wwg
 * Initial revision
 *
 *
 *	change log:
 *	16.02.97 -	Erik de Castro Lopo		
 *	Ported from MS-Windows to Linux for Warren W. Gay's
 *	wavplay project.
 */	
static char rcsid[] = "@(#)wavfile.c $Revision: 1.1 $";

#include  	<stdio.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<unistd.h>
#include  	<string.h>

#include "wavplay.h"

#define		BUFFERSIZE   		1024
#define		PCM_WAVE_FORMAT   	1

#define		TRUE			1
#define		FALSE			0

typedef  struct
{	u_long     dwSize ;
	u_short    wFormatTag ;
	u_short    wChannels ;
	u_long     dwSamplesPerSec ;
	u_long     dwAvgBytesPerSec ;
	u_short    wBlockAlign ;
	u_short    wBitsPerSample ;
} WAVEFORMAT ;

typedef  struct
{	char    	RiffID [4] ;
	u_long    	RiffSize ;
	char    	WaveID [4] ;
	char    	FmtID  [4] ;
	u_long    	FmtSize ;
	u_short   	wFormatTag ;
	u_short   	nChannels ;
	u_long		nSamplesPerSec ;
	u_long		nAvgBytesPerSec ;
	u_short		nBlockAlign ;
	u_short		wBitsPerSample ;
	char		DataID [4] ;
	u_long		nDataBytes ;
} WAVE_HEADER ;

/*=================================================================================================*/

char*  findchunk (char* s1, char* s2, size_t n) ;

/*=================================================================================================*/


static  WAVE_HEADER  waveheader =
{	{ 'R', 'I', 'F', 'F' },
		0,
	{ 'W', 'A', 'V', 'E' },
	{ 'f', 'm', 't', ' ' },
		16,								/* FmtSize*/
		PCM_WAVE_FORMAT,						/* wFormatTag*/
		0,								/* nChannels*/
		0,
		0,
		0,
		0,
	{ 'd', 'a', 't', 'a' },
		0
} ; /* waveheader*/

static ErrFunc v_erf;				/* wwg: Error reporting function */

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

int  WaveWriteHeader (int wavefile, int channels, u_long samplerate, int sampbits, u_long samples, ErrFunc erf)
{ 	u_long		databytes ;
	u_short		blockalign ;

	v_erf = erf;				/* wwg: Set error reporting function */

	if ( wavefile < 0 ) {
		err("Invalid file descriptor");
		return WW_BADOUTPUTFILE ;
	}

	sampbits   = (sampbits == 16) ? 16 : 8 ;

	blockalign = ((sampbits == 16) ? 2 : 1) * channels ;
	databytes  = samples * (u_long) blockalign ;

	waveheader.RiffSize 	   = sizeof (WAVE_HEADER) + databytes - 8 ;
	waveheader.wFormatTag      = PCM_WAVE_FORMAT ;
	waveheader.nChannels       = channels ;
	waveheader.nSamplesPerSec  = samplerate ;
	waveheader.nAvgBytesPerSec = samplerate * (u_long) blockalign ;
	waveheader.nBlockAlign     = blockalign ;
	waveheader.wBitsPerSample  = sampbits ;
	waveheader.nDataBytes      = databytes;

	if (write (wavefile, &waveheader, sizeof (WAVE_HEADER)) != sizeof (WAVE_HEADER)) {
		err("%s",sys_errlist[errno]);	/* wwg: report the error */
		return  WW_BADWRITEHEADER ;
	}

  return 0 ;
} ; /* WaveWriteHeader*/

int  WaveReadHeader  (int wavefile, int* channels, u_long* samplerate, int* samplebits, u_long* samples, u_long* datastart,ErrFunc erf)
{	static  WAVEFORMAT  waveformat ;
	static	char   buffer [ BUFFERSIZE ] ;		/* Function is not reentrant.*/
	char*   ptr ;
	u_long  databytes ;

	v_erf = erf;					/* wwg: Set error reporting function */

	if (lseek (wavefile, 0L, SEEK_SET)) {
		err("%s",sys_errlist[errno]);		/* wwg: Report error */
		return  WR_BADSEEK ;
	}

	read (wavefile, buffer, BUFFERSIZE) ;

	if (findchunk (buffer, "RIFF", BUFFERSIZE) != buffer) {
		err("Bad format: Cannot find RIFF file marker");	/* wwg: Report error */
		return  WR_BADRIFF ;
	}

	if (! findchunk (buffer, "WAVE", BUFFERSIZE)) {
		err("Bad format: Cannot find WAVE file marker");	/* wwg: report error */
		return  WR_BADWAVE ;
	}

	ptr = findchunk (buffer, "fmt ", BUFFERSIZE) ;

	if (! ptr) {
		err("Bad format: Cannot find 'fmt' file marker");	/* wwg: report error */
		return  WR_BADFORMAT ;
	}

	ptr += 4 ;	/* Move past "fmt ".*/
	memcpy (&waveformat, ptr, sizeof (WAVEFORMAT)) ;

	if (waveformat.dwSize != (sizeof (WAVEFORMAT) - sizeof (u_long))) {
		err("Bad format: Bad fmt size");			/* wwg: report error */
		return  WR_BADFORMATSIZE ;
	}

	if (waveformat.wFormatTag != PCM_WAVE_FORMAT) {
		err("Only supports PCM wave format");			/* wwg: report error */
		return  WR_NOTPCMFORMAT ;
	}

	ptr = findchunk (buffer, "data", BUFFERSIZE) ;

	if (! ptr) {
		err("Bad format: unable to find 'data' file marker");	/* wwg: report error */
		return  WR_NODATACHUNK ;
	}

	ptr += 4 ;	/* Move past "data".*/
	memcpy (&databytes, ptr, sizeof (u_long)) ;

	/* Everything is now cool, so fill in output data.*/

	*channels   = waveformat.wChannels ;
	*samplerate = waveformat.dwSamplesPerSec ;
	*samplebits = waveformat.wBitsPerSample ;
	*samples    = databytes / waveformat.wBlockAlign ;
	
	*datastart  = ((u_long) (ptr + 4)) - ((u_long) (&(buffer[0]))) ;

	if (waveformat.dwSamplesPerSec != waveformat.dwAvgBytesPerSec / waveformat.wBlockAlign) {
		err("Bad file format");			/* wwg: report error */
		return  WR_BADFORMATDATA ;
	}

	if (waveformat.dwSamplesPerSec != waveformat.dwAvgBytesPerSec / waveformat.wChannels / ((waveformat.wBitsPerSample == 16) ? 2 : 1)) {
		err("Bad file format");			/* wwg: report error */
		return  WR_BADFORMATDATA ;
	}

  return  0 ;
} ; /* WaveReadHeader*/

/*===========================================================================================*/

#if 0
char*  WaveFileError (int  errno)
{	switch (errno)
	{	case	WW_BADOUTPUTFILE	: return "Bad output file.\n" ;
		case	WW_BADWRITEHEADER 	: return "Not able to write WAV header.\n" ;
		
		case	WR_BADALLOC			: return "Not able to allocate memory.\n" ;
		case	WR_BADSEEK        	: return "fseek failed.\n" ;
		case	WR_BADRIFF        	: return "Not able to find 'RIFF' file marker.\n" ;
		case	WR_BADWAVE        	: return "Not able to find 'WAVE' file marker.\n" ;
		case	WR_BADFORMAT      	: return "Not able to find 'fmt ' file marker.\n" ;
		case	WR_BADFORMATSIZE  	: return "Format size incorrect.\n" ;
		case	WR_NOTPCMFORMAT		: return "Not PCM format WAV file.\n" ;
		case	WR_NODATACHUNK		: return "Not able to find 'data' file marker.\n" ;
		case	WR_BADFORMATDATA	: return "Format data questionable.\n" ;
		default           			:  return "No error\n" ;
	} ;
	return	NULL ;	
} ; /* WaveFileError*/
#endif
/*===========================================================================================*/

char* findchunk  (char* pstart, char* fourcc, size_t n)
{	char	*pend ;
	int		k, test ;

	pend = pstart + n ;

	while (pstart < pend)
	{ 	if (*pstart == *fourcc)       /* found match for first char*/
		{	test = TRUE ;
			for (k = 1 ; fourcc [k] != 0 ; k++)
				test = (test ? ( pstart [k] == fourcc [k] ) : FALSE) ;
			if (test)
				return  pstart ;
			} ; /* if*/
		pstart ++ ;
		} ; /* while lpstart*/

	return  NULL ;
} ; /* findchuck*/

/* $Source: /wwg/motif/xltwavplay/RCS/wavfile.c,v $ */
