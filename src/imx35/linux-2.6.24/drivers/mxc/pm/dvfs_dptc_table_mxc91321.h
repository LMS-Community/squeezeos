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
 * @brief MXC91321 dvfs & dptc table file.
 *
 * @ingroup PM_MXC91321
 */
#ifndef __DVFS_DPTC_TABLE_MXC91321_H__
#define __DVFS_DPTC_TABLE_MXC91321_H__

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
DCVR 0x7fe00000 0x82E10870 0xF53DCFD0 0xCA32ED04\n\
DCVR 0x7fe00000 0x83211874 0xF5BE0FDC 0xCAB32D10\n\
DCVR 0x7fe00000 0x8361287C 0xF5FE2FEC 0xCAB33D1C\n\
DCVR 0x7fe00000 0x8361388C 0xF5FE3004 0xCAF34D34\n\
DCVR 0x7fe00000 0x83613890 0xF63E4010 0xCAF35D3C\n\
DCVR 0x7fe00000 0x83614894 0xF63E5018 0xCB335D44\n\
DCVR 0x7fe00000 0x83A1489C 0xF67E6024 0xCB337D4C\n\
DCVR 0x7fe00000 0x83A158A0 0xF67E7030 0xCB337D54\n\
DCVR 0x7fe00000 0x83A158A4 0xF6BE8038 0xCB738D60\n\
DCVR 0x7fe00000 0x83E168A8 0xF6BEA044 0xCBB39D68\n\
DCVR 0x7fe00000 0x83E168B0 0xF6FEB050 0xCBB3BD74\n\
DCVR 0x7fe00000 0x83E178B8 0xF73EC05C 0xCBF3BD80\n\
DCVR 0x7fe00000 0x842188BC 0xF77EE06C 0xCBF3DD8C\n\
DCVR 0x7fe00000 0x842198C8 0xF77EF07C 0xCC33FD9C\n\
DCVR 0x7fe00000 0x8421A8CC 0xF7BF108C 0xCC73FDA8\n\
DCVR 0x7fe00000 0x8461B8D8 0xF7FF309C 0xCCB42DB8\n\
DCVR 0x7fe00000 0x84A1C8E0 0xF83F50B0 0xCCF44DC8\n\
";
#endif
