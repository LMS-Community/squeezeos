/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file fsl_platform.h
 *
 * Header file to isolate code which might be platform-dependent
 */

#ifndef FSL_PLATFORM_H
#define FSL_PLATFORM_H

#ifdef __KERNEL__
#include "portable_os.h"
#endif

#if defined(FSL_PLATFORM_OTHER)

/* Have Makefile or other method of setting FSL_HAVE_* flags */

#elif defined(CONFIG_ARCH_MX3)	/* i.MX31 */

#define FSL_HAVE_SCC
#define FSL_HAVE_RTIC
#define FSL_HAVE_RNGA

#elif defined(CONFIG_ARCH_MX21)

#define FSL_HAVE_HAC
#define FSL_HAVE_RNGA
#define FSL_HAVE_SCC

#elif defined(CONFIG_ARCH_MX27)

#define FSL_HAVE_SAHARA2
#define SUBMIT_MULTIPLE_DARS
#define FSL_HAVE_RTIC
#define FSL_HAVE_SCC
#define USE_OLD_PTRS

#elif defined(CONFIG_ARCH_MX35)

#define FSL_HAVE_SCC
#define FSL_HAVE_RNGC
#define FSL_HAVE_RTIC

#elif defined(CONFIG_ARCH_MX37)

#define FSL_HAVE_SCC2
#define FSL_HAVE_RNGC
#define FSL_HAVE_RTIC2

#elif defined(CONFIG_ARCH_MXC91131)

#define FSL_HAVE_SCC
#define FSL_HAVE_RNGA
#define FSL_HAVE_HAC

#elif defined(CONFIG_ARCH_MXC91221)

#define FSL_HAVE_SCC
#define FSL_HAVE_RNGC
#define FSL_HAVE_RTIC2

#elif defined(CONFIG_ARCH_MXC91231)

#define FSL_HAVE_SAHARA2
#define USE_OLD_PTRS
#define FSL_HAVE_RTIC
#define FSL_HAVE_SCC
#define NO_OUTPUT_1K_CROSSING

#elif defined(CONFIG_ARCH_MXC91311)

#define FSL_HAVE_SCC
#define FSL_HAVE_RNGC

#elif defined(CONFIG_ARCH_MXC91321)

#define FSL_HAVE_SAHARA2
#define FSL_HAVE_RTIC
#define FSL_HAVE_SCC
#define NO_OUTPUT_1K_CROSSING
#define USE_OLD_PTRS

#elif defined(CONFIG_ARCH_MXC92323)

#define FSL_HAVE_SCC2
#define FSL_HAVE_SAHARA4
#define FSL_HAVE_RTIC2
#define NO_1K_CROSSING
#define NO_RESEED_WORKAROUND
#define NEED_CTR_WORKAROUND
#define USE_3WORD_BURST

#elif  defined(CONFIG_ARCH_MXC91331)

#define FSL_HAVE_SCC
#define FSL_HAVE_RNGA
#define FSL_HAVE_HAC
#define FSL_HAVE_RTIC

#elif defined(CONFIG_8548)

#define FSL_HAVE_SEC2x

#elif defined(CONFIG_MPC8374)

#define FSL_HAVE_SEC3x

#else

#error UNKNOWN_PLATFORM

#endif				/* platform checks */

#endif				/* FSL_PLATFORM_H */
