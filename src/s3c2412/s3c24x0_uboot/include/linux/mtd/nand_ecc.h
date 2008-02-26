/*
 *  drivers/mtd/nand_ecc.h
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * $Id: nand_ecc.h,v 1.1 2006/01/24 05:35:08 naushad Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file is the header for the ECC algorithm.
 */

#ifndef __NAND_ECC_H__
#define __NAND_ECC_H__

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
int nand_calculate_ecc(const u_char *dat, u_char *ecc_code);

/*
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data(u_char *dat, u_char *read_ecc, u_char *calc_ecc);

/*
 * nand_calculate_pageecc - Calculates the ecc for a page in steps 
 */
int nand_calculate_pageecc(const u_char *dat, u_char *ecc_buf,u32 pagesize);
#endif /* __NAND_ECC_H__ */
