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
 * otg/msc_fd/crc.c - crc table
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/msc/crc.c|20070425221028|06858
 *
 *      Copyright (c) 2003-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>


#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <otg/otg-compat.h>

//#include <usbd-chap9.h>
//#include <usbd-mem.h>
//#include <usbd.h>
//#include <usbd-func.h>

#include "crc.h"

unsigned int *msc_crc32_table;

/**
 * Generate the crc32 table
 *
 * return non-zero if malloc fails
 */
int make_crc_table(void)
{
        unsigned int n;
        if (msc_crc32_table) return 0;
        if (!(msc_crc32_table = (unsigned int *)ckmalloc(256*4))) return -EINVAL;
        for (n = 0; n < 256; n++) {
                int k;
                unsigned int c = n;
                for (k = 0; k < 8; k++) {
                        c =  (c & 1) ? (CRC32_POLY ^ (c >> 1)) : (c >> 1);
                }
                msc_crc32_table[n] = c;
        }
        return 0;
}
/*! free_crc_table - free ckmalloced resource for crc-table
 * @return void
 */
void free_crc_table(void)
{
        if (msc_crc32_table) {
                lkfree(msc_crc32_table);
                msc_crc32_table = NULL;
        }
}

/*! crc32_compute - compute frame check sequence number
 * @param src  source data
 * @param len length of source data
 * @param val FCS value
 * @return FCS result
 */

unsigned int crc32_compute(unsigned char *src, int len, unsigned int val)
{
        for (; len-- > 0; val = COMPUTE_FCS (val, *src++));
        return val;
}
