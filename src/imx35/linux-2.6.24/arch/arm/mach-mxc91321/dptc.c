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
	{0x7fe00000, 0x46489d11, 0x820fe9f9, 0x6ccd55a7, SW_SW1A, SW1A_1_6V},
	{0x7fe00000, 0x46689d11, 0x826fedf9, 0x6d2d59a7, SW_SW1A, SW1A_1_575V},
	{0x7fe00000, 0x46a8a111, 0x82cff5fa, 0x6d6d5da8, SW_SW1A, SW1A_1_55V},
	{0x7fe00000, 0x46c8a111, 0x832ff9fa, 0x6dcd61a8, SW_SW1A, SW1A_1_525V},
	{0x7fe00000, 0x46e8a111, 0x838ffdfa, 0x6e0d65a8, SW_SW1A, SW1A_1_5V},
	/* wp5 */
	{0x7fe00000, 0x4708a511, 0x83affdfa, 0x6e4d65a8, SW_SW1A, SW1A_1_475V},
	{0x7fe00000, 0x4728a512, 0x841001fa, 0x6e8d69a8, SW_SW1A, SW1A_1_45V},
	{0x7fe00000, 0x4748a912, 0x845009fb, 0x6ecd6da9, SW_SW1A, SW1A_1_425V},
	{0x7fe00000, 0x4788a912, 0x849009fb, 0x6f0d71a9, SW_SW1A, SW1A_1_4V},
	{0x7fe00000, 0x47a8ad12, 0x84f00dfb, 0x6f4d75a9, SW_SW1A, SW1A_1_375V},
	/* wp10 */
	{0x7fe00000, 0x47c8ad12, 0x855015fb, 0x6fad79a9, SW_SW1A, SW1A_1_35V},
	{0x7fe00000, 0x4808b112, 0x859015fc, 0x6fed7da9, SW_SW1A, SW1A_1_325V},
	{0x7fe00000, 0x4828b512, 0x85f01dfc, 0x704d81aa, SW_SW1A, SW1A_1_3V},
	{0x7fe00000, 0x4868b513, 0x867021fc, 0x70ad85aa, SW_SW1A, SW1A_1_275V},
	{0x7fe00000, 0x48a8b913, 0x86d029fd, 0x710d89aa, SW_SW1A, SW1A_1_25V},
	/* wp15 */
	{0x7fe00000, 0x48c8bd13, 0x875031fd, 0x716d91ab, SW_SW1A, SW1A_1_225V},
	{0x7fe00000, 0x4928c113, 0x87d035fe, 0x71ed99ab, SW_SW1A, SW1A_1_2V},
};
