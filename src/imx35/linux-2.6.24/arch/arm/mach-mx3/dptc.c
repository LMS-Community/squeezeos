/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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

struct dptc_wp dptc_wp_allfreq_26ckih[DPTC_WP_SUPPORTED] = {
	/* 532MHz */
	/* dcvr0      dcvr1       dcvr2       dcvr3       regulator voltage */
	/* wp0 */
	{0xffc00000, 0x95c00000, 0xffc00000, 0xe5800000, SW_SW1A, SW1A_1_625V},
	{0xffc00000, 0x95e3e8e4, 0xffc00000, 0xe5b6fda0, SW_SW1A, SW1A_1_6V},
	{0xffc00000, 0x95e3e8e4, 0xffc00000, 0xe5b6fda0, SW_SW1A, SW1A_1_575V},
	{0xffc00000, 0x95e3e8e8, 0xffc00000, 0xe5f70da4, SW_SW1A, SW1A_1_55V},
	{0xffc00000, 0x9623f8e8, 0xffc00000, 0xe6371da8, SW_SW1A, SW1A_1_525V},
	/* wp5 */
	{0xffc00000, 0x966408f0, 0xffc00000, 0xe6b73db0, SW_SW1A, SW1A_1_5V},
	{0xffc00000, 0x96e428f4, 0xffc00000, 0xe7776dbc, SW_SW1A, SW1A_1_475V},
	{0xffc00000, 0x976448fc, 0xffc00000, 0xe8379dc8, SW_SW1A, SW1A_1_45V},
	{0xffc00000, 0x97e46904, 0xffc00000, 0xe977ddd8, SW_SW1A, SW1A_1_425V},
	{0xffc00000, 0x98a48910, 0xffc00000, 0xeab81de8, SW_SW1A, SW1A_1_4V},
	/* wp10 */
	{0xffc00000, 0x9964b918, 0xffc00000, 0xebf86df8, SW_SW1A, SW1A_1_375V},
	{0xffc00000, 0xffe4e924, 0xffc00000, 0xfff8ae08, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0xffe5192c, 0xffc00000, 0xfff8fe1c, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0xffe54938, 0xffc00000, 0xfff95e2c, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0xffe57944, 0xffc00000, 0xfff9ae44, SW_SW1A, SW1A_1_35V},
	/* wp15 */
	{0xffc00000, 0xffe5b954, 0xffc00000, 0xfffa0e58, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0xffe5e960, 0xffc00000, 0xfffa6e70, SW_SW1A, SW1A_1_35V},
};

struct dptc_wp dptc_wp_allfreq_26ckih_TO_2_0[DPTC_WP_SUPPORTED] = {
	/* Mx31 TO 2.0  Offset table */
	/* 532MHz  */
	/* dcvr0      dcvr1       dcvr2       dcvr3       regulator voltage */
	/* wp0 */
	{0xffc00000, 0x9E265978, 0xffc00000, 0xE4371D9C, SW_SW1A, SW1A_1_625V},
	{0xffc00000, 0x9E665978, 0xffc00000, 0xE4772D9C, SW_SW1A, SW1A_1_6V},
	{0xffc00000, 0x9EA65978, 0xffc00000, 0xE4772DA0, SW_SW1A, SW1A_1_575V},
	{0xffc00000, 0x9EE66978, 0xffc00000, 0xE4B73DA0, SW_SW1A, SW1A_1_55V},
	{0xffc00000, 0x9F26697C, 0xffc00000, 0xE4F73DA0, SW_SW1A, SW1A_1_525V},
	/* wp5 */
	{0xffc00000, 0x9F66797C, 0xffc00000, 0xE5774DA4, SW_SW1A, SW1A_1_5V},
	{0xffc00000, 0x9FE6797C, 0xffc00000, 0xE5F75DA4, SW_SW1A, SW1A_1_475V},
	{0xffc00000, 0xA026897C, 0xffc00000, 0xE6776DA4, SW_SW1A, SW1A_1_45V},
	{0xffc00000, 0xA0A6897C, 0xffc00000, 0xE6F77DA8, SW_SW1A, SW1A_1_425V},
	{0xffc00000, 0xA0E69980, 0xffc00000, 0xE7B78DAC, SW_SW1A, SW1A_1_4V},
	/* wp10 */
	{0xffc00000, 0xA1669980, 0xffc00000, 0xE8379DAC, SW_SW1A, SW1A_1_375V},
	{0xffc00000, 0xA1A6A980, 0xffc00000, 0xE8F7ADB0, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0xA226B984, 0xffc00000, 0xE9F7CDB0, SW_SW1A, SW1A_1_325V},
	{0xffc00000, 0xA2A6C984, 0xffc00000, 0xEAB7DDB4, SW_SW1A, SW1A_1_3V},
	{0xffc00000, 0xA326C988, 0xffc00000, 0xEBB7FDB8, SW_SW1A, SW1A_1_275V},
	/* wp15 */
	{0xffc00000, 0xA3A6D988, 0xffc00000, 0xECB80DBC, SW_SW1A, SW1A_1_25V},
	{0xffc00000, 0xA426E988, 0xffc00000, 0xEDB82DC0, SW_SW1A, SW1A_1_225V},
};

struct dptc_wp dptc_wp_allfreq_27ckih_TO_2_0[DPTC_WP_SUPPORTED] = {
	/* Mx31 TO 2.0  Offset table */
	/* 532MHz  */
	/* dcvr0      dcvr1       dcvr2       dcvr3       regulator voltage */
	/* wp0 */
	{0xffc00000, 0x9864E920, 0xffc00000, 0xDBB50D1C, SW_SW1A, SW1A_1_625V},
	{0xffc00000, 0x98A4E920, 0xffc00000, 0xDBF51D1C, SW_SW1A, SW1A_1_6V},
	{0xffc00000, 0x98E4E920, 0xffc00000, 0xDBF51D20, SW_SW1A, SW1A_1_575V},
	{0xffc00000, 0x9924F920, 0xffc00000, 0xDC352D20, SW_SW1A, SW1A_1_55V},
	{0xffc00000, 0x9924F924, 0xffc00000, 0xDC752D20, SW_SW1A, SW1A_1_525V},
	/* wp5 */
	{0xffc00000, 0x99650924, 0xffc00000, 0xDCF53D24, SW_SW1A, SW1A_1_5V},
	{0xffc00000, 0x99E50924, 0xffc00000, 0xDD754D24, SW_SW1A, SW1A_1_475V},
	{0xffc00000, 0x9A251924, 0xffc00000, 0xDDF55D24, SW_SW1A, SW1A_1_45V},
	{0xffc00000, 0x9AA51924, 0xffc00000, 0xDE756D28, SW_SW1A, SW1A_1_425V},
	{0xffc00000, 0x9AE52928, 0xffc00000, 0xDF357D2C, SW_SW1A, SW1A_1_4V},
	/* wp10 */
	{0xffc00000, 0x9B652928, 0xffc00000, 0xDFB58D2C, SW_SW1A, SW1A_1_375V},
	{0xffc00000, 0x9BA53928, 0xffc00000, 0xE0759D30, SW_SW1A, SW1A_1_35V},
	{0xffc00000, 0x9C254928, 0xffc00000, 0xE135BD30, SW_SW1A, SW1A_1_325V},
	{0xffc00000, 0x9CA55928, 0xffc00000, 0xE1F5CD34, SW_SW1A, SW1A_1_3V},
	{0xffc00000, 0x9D25592C, 0xffc00000, 0xE2F5ED38, SW_SW1A, SW1A_1_275V},
	/* wp15 */
	{0xffc00000, 0x9DA5692C, 0xffc00000, 0xE3F5FD38, SW_SW1A, SW1A_1_25V},
	{0xffc00000, 0x9E25792C, 0xffc00000, 0xE4F61D3C, SW_SW1A, SW1A_1_225V},
};
