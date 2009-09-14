/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * otg/otgcore/otg-compat.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/linux/otg-compat.h|20070425221118|15928
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 */
/*!
 * @file otg/otg/otg-compat.h
 * @brief Common include file for Linux to determine and include appropriate OS compatibility file.
 *
 *
 * @ingroup LINUXAPI
 */
#ifndef _OTG_COMPAT_H
#define _OTG_COMPAT_H 1


#include <otg/otg-utils.h>
//#include <otg/otg-trace.h>


#if defined(_WIN32_WCE)
#define OTG_WINCE       _WIN32_WCE
#endif /* defined(_WIN32_WCE) */

        /* What operating system are we running under? */

        /* Recursively include enough information to determine which release */

        #if (__GNUC__ >=3)
        #define GCC3
        #else
        #define GCC2
        #endif
        #include <linux/kernel.h>
        #include <linux/version.h>

#if defined(__GNUC__)
        #define OTG_LINUX
        #ifndef CONFIG_OTG_NOC99
        #define OTG_C99
        #else
        #endif

        #if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,2)
        #define LINUX26
        #elif LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5)
        #define LINUX24
        #else /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */
        #define LINUX24
        #define LINUX_OLD
        #warning "Early unsupported release of Linux kernel"
        #endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */

        /* We are running under a supported version of Linux */
        #include <otg/otg-linux.h>


#elif defined(OTG_WINCE)

        /* We are running under a supported version of WinCE */
        #include <otg/otg-wince.h>
        #include <otg/otg-wince-ex.h>

#else /* defined(OTG_WINCE) */

        #error "Operating system not recognized"

#endif /* defined(OTG_WINCE) */

#if !defined(OTG_C99)
#else /* !defined(OTG_C99) */
#endif /* !defined(OTG_C99) */

/* include otg-os.h - this will confirm that the otg-xxx.h file
 * properly defined the os primitives.
 */
#include <otg/otg-os.h>
#endif /* _OTG_COMPAT_H */
