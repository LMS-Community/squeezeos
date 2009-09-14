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
 * @(#) balden@belcarra.com|otg/functions/msc/crc.h|20060419204258|03869
 *
 *      Copyright (c) 2003-2004 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */


extern unsigned int *msc_crc32_table;

#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation

#define COMPUTE_FCS(val, c) (((val) >> 8) ^ msc_crc32_table[((val) ^ (c)) & 0xff])

int make_crc_table(void);
void free_crc_table(void);
unsigned int crc32_compute(unsigned char *src, int len, unsigned int val);
