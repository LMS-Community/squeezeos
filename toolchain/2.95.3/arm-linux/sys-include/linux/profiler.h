/*
 * Itsy SA1100 Statistical Profiler
 * Copyright (c) Compaq Computer Corporation, 1999
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Author: Carl Waldspurger
 *
 * $Log: itsy_prof.h,v $
 * Revision 2.6  2000/11/22  10:27:07  kerr
 * Rename to be less Itsy-specific.
 *
 * Revision 2.5  1999/12/23  01:28:27  kerr
 * Updated Copyright information for entire source tree.
 *
 * Revision 2.4  1999/08/23  22:10:51  caw
 * Added documentation.
 *
 * Revision 2.3  1999/08/17 01:04:13  caw
 * Defined special sample values.
 *
 * Revision 2.2  1999/08/13 00:46:22  caw
 * Changed profiler_ctl, improving ITSY_PROF_{GET,SET}_CTL interface.
 * Added more documentation.
 *
 * Revision 2.1  1999/08/10  01:15:21  caw
 * Initial revision.
 *
 */

#ifndef	PROFILER_H
#define	PROFILER_H

/*
 * constants
 *
 */

/* valid sampling freq in [10, 1000] */
#define	PROFILER_FREQ_MIN	(10)
#define	PROFILER_FREQ_MAX	(1000)

/* sampling freq defaults */
#define	PROFILER_FREQ_DEFAULT	(256)
#define	PROFILER_RND_DEFAULT	(1)

/* special sample values */
#define	PROFILER_DEVICE_NONE	(0)
#define	PROFILER_INODE_NONE	(0)
#define	PROFILER_INODE_KERNEL	(1)
#define	PROFILER_PID_NONE	(0xffff)

/*
 * types
 *
 */

typedef unsigned char uchar;

/* profiling sample */
typedef struct {
  ulong  pid;	    /* process identifier */
  ulong  offset;    /* offset into executable file */
  ulong  inode;     /* executable file inode */
  ushort device;    /* executable file device */
  uchar  mode;	    /* processor mode */
  uchar  count;	    /* sample count */
}  __attribute__ ((packed)) profiler_sample;

/* statistics */
typedef struct {
  ulong interrupts; /* total interrupts */
  ulong samples;    /* total samples */
  ulong wakeups;    /* client wakeups */
  ulong user;       /* user samples */
  ulong kernel;     /* kernel samples */
  ulong nomap;      /* pc -> image, offset failed */
  ulong unused[2];  /* reserved for future use */
} profiler_stats;

/* control parameters */
typedef struct {
  ulong frequency;  /* samples per sec */
  ulong randomize;  /* boolean flag */
  ulong unused[4];  /* reserved for future use */
} profiler_ctl;

/* stream position (64 bits) */
typedef long long profiler_pos;

/*
 * ioctl interface
 *
 */

/* driver ioctl type */
#define	PROFILER_IOCTL_TYPE	'p'
#define	PROFILER_IOCTL_MAXNR	(6)

/* ioctl commands */
#define	PROFILER_GET_CTL	_IOR(PROFILER_IOCTL_TYPE,  0, profiler_ctl)
#define	PROFILER_SET_CTL	_IOW(PROFILER_IOCTL_TYPE,  1, profiler_ctl)

#define	PROFILER_GET_POSITION	_IOR(PROFILER_IOCTL_TYPE,  2, profiler_pos)
#define	PROFILER_SET_POSITION	_IOWR(PROFILER_IOCTL_TYPE, 3, profiler_pos)

#define	PROFILER_GET_DROPPED	_IOR(PROFILER_IOCTL_TYPE,  4, profiler_pos)
#define	PROFILER_SET_DROPPED	_IOW(PROFILER_IOCTL_TYPE,  5, profiler_pos)
#define	PROFILER_SET_DROP_FAIL	_IO(PROFILER_IOCTL_TYPE,   6)

#endif	/* PROFILER_H */
