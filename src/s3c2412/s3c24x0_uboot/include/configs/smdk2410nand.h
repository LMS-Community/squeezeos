/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 * Gary Jennejohn <gj@denx.de>
 * David Mueller <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Gianluca Costa <gcosta@libero.it>
 *
 * Configuation settings for the SAMSUNG SMDK2410 board with nand boot.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_SMDK2410NAND_H
#define __CONFIG_SMDK2410NAND_H

#define CFG_ENV_NAND_BLOCK     8

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
#define NF_USE_ECC     1 
#define NF_USE_MTD_ECC 2

#endif	/* __CONFIG_H */

