/*
 * (C) Copyright 2004 
 *  Samsung Electronics  : SW.LEE <hitchcar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SMDK2413_NAND_H
#define __SMDK2413_NAND_H

#define CFG_ENV_NAND_BLOCK     	      10
#define CFG_ENV_NAND_BLOCK_REDUND     11

#define NAND_OOB_SIZE           (16)
#define NAND_PAGES_IN_BLOCK     (32)
#define NAND_PAGE_SIZE          (512)

#define NAND_BLOCK_SIZE         (NAND_PAGE_SIZE*NAND_PAGES_IN_BLOCK)
#define NAND_BLOCK_CNT          (4096)
#define NAND_BLOCK_MASK         (NAND_BLOCK_SIZE - 1)
#define NAND_PAGE_MASK          (NAND_PAGE_SIZE - 1)

/* #define NAND_3_ADDR_CYCLE	1 */
/* #define S3C24X0_16BIT_NAND	1 */
/* mark bad block */
#define NF_BB_ON   1
#define NF_BB_OFF  0

/* NAND R/W status */
#define NF_RW_NORMAL   1   /* default */
#define NF_RW_YAFFS    2   /* yaffs image r/w */
    
/* NAND R/W ECC status */
#define NF_USE_YAFFS_ECC	1 
#define NF_USE_MTD_ECC		2
#define NF_USE_HW_ECC		3

#endif
