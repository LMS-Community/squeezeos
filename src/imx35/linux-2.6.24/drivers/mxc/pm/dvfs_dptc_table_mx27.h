/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file dptc.h
 *
 * @brief i.MX27 dptc table file.
 *
 * @ingroup PM_MX27
 */
#ifndef __DVFS_DPTC_TABLE_MX27_H__
#define __DVFS_DPTC_TABLE_MX27_H__

/*!
 * Default DPTC table definition
 */
#define NUM_OF_FREQS 1
#define NUM_OF_WP    17

static char *default_table_str = "WORKING POINT 17\n\
\n\
WP 0x1c\n\
WP 0x1b\n\
WP 0x1a\n\
WP 0x19\n\
WP 0x18\n\
WP 0x17\n\
WP 0x16\n\
WP 0x15\n\
WP 0x14\n\
WP 0x13\n\
WP 0x12\n\
WP 0x11\n\
WP 0x10\n\
WP 0xf\n\
WP 0xe\n\
WP 0xd\n\
WP 0xc\n\
\n\
DCVR 0xffe00000 0x18e2e85b 0xffe00000 0x25c4688a \n\
DCVR 0xffe00000 0x18e2e85b 0xffe00000 0x25c4688a \n\
DCVR 0xffe00000 0x1902e85b 0xffe00000 0x25e4688a \n\
DCVR 0xffe00000 0x1922e85b 0xffe00000 0x25e4688a \n\
DCVR 0xffe00000 0x1942ec5b 0xffe00000 0x2604688a \n\
DCVR 0xffe00000 0x1942ec5b 0xffe00000 0x26646c8a \n\
DCVR 0xffe00000 0x1962ec5b 0xffe00000 0x26c4708b \n\
DCVR 0xffe00000 0x1962ec5b 0xffe00000 0x26e4708b \n\
DCVR 0xffe00000 0x1982f05c 0xffe00000 0x2704748b \n\
DCVR 0xffe00000 0x19c2f05c 0xffe00000 0x2744748b \n\
DCVR 0xffe00000 0x1a02f45c 0xffe00000 0x2784788b \n\
DCVR 0xffe00000 0x1a42f45c 0xffe00000 0x27c47c8b \n\
DCVR 0xffe00000 0x1a82f85c 0xffe00000 0x2824808c \n\
DCVR 0xffe00000 0x1aa2f85c 0xffe00000 0x2884848c \n\
DCVR 0xffe00000 0x1ac2fc5c 0xffe00000 0x28e4888c \n\
DCVR 0xffe00000 0x1ae2fc5c 0xffe00000 0x2924888c \n\
DCVR 0xffe00000 0x1b23005d 0xffe00000 0x29648c8c \n\
";

#endif
