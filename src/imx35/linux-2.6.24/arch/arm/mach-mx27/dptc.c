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

/*!
 * @file dptc.c
 *
 * @brief DPTC table for the Freescale Semiconductor MXC DPTC module.
 *
 * @ingroup PM
 */

#include <asm/arch/pmic_power.h>
#include <asm/arch/hardware.h>

struct dptc_wp dptc_wp_allfreq[DPTC_WP_SUPPORTED] = {
	/* 532MHz */
	/* dcvr0      dcvr1       dcvr2       dcvr3       regulator voltage */
	/* wp0 */
	{0xffe00000, 0x18e2e85b, 0xffe00000, 0x25c4688a, SW_SW1A, SW1A_1_6V},
	{0xffe00000, 0x18e2e85b, 0xffe00000, 0x25c4688a, SW_SW1A, SW1A_1_575V},
	{0xffe00000, 0x1902e85b, 0xffe00000, 0x25e4688a, SW_SW1A, SW1A_1_55V},
	{0xffe00000, 0x1922e85b, 0xffe00000, 0x25e4688a, SW_SW1A, SW1A_1_525V},
	{0xffe00000, 0x1942ec5b, 0xffe00000, 0x2604688a, SW_SW1A, SW1A_1_5V},
	/* wp5 */
	{0xffe00000, 0x1942ec5b, 0xffe00000, 0x26646c8a, SW_SW1A, SW1A_1_475V},
	{0xffe00000, 0x1962ec5b, 0xffe00000, 0x26c4708b, SW_SW1A, SW1A_1_45V},
	{0xffe00000, 0x1962ec5b, 0xffe00000, 0x26e4708b, SW_SW1A, SW1A_1_425V},
	{0xffe00000, 0x1982f05c, 0xffe00000, 0x2704748b, SW_SW1A, SW1A_1_4V},
	{0xffe00000, 0x19c2f05c, 0xffe00000, 0x2744748b, SW_SW1A, SW1A_1_375V},
	/* wp10 */
	{0xffe00000, 0x1a02f45c, 0xffe00000, 0x2784788b, SW_SW1A, SW1A_1_35V},
	{0xffe00000, 0x1a42f45c, 0xffe00000, 0x27c47c8b, SW_SW1A, SW1A_1_325V},
	{0xffe00000, 0x1a82f85c, 0xffe00000, 0x2824808c, SW_SW1A, SW1A_1_3V},
	{0xffe00000, 0x1aa2f85c, 0xffe00000, 0x2884848c, SW_SW1A, SW1A_1_275V},
	{0xffe00000, 0x1ac2fc5c, 0xffe00000, 0x28e4888c, SW_SW1A, SW1A_1_25V},
	/* wp15 */
	{0xffe00000, 0x1ae2fc5c, 0xffe00000, 0x2924888c, SW_SW1A, SW1A_1_225V},
	{0xffe00000, 0x1b23005d, 0xffe00000, 0x29648c8c, SW_SW1A, SW1A_1_2V},
};
