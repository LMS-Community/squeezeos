/* linux/include/asm-arm/arch-s3c2413/regs-ebi.h
 *
 * Copyright (c) 2005 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2413 External Bus Interface register definitions
 *
 *  Changelog:
 *	31-Oct-2005  rahul.tanwar@samsung.com  Initial include for Linux
 *
*/

#ifndef __ASM_ARM_EBIREGS_H
#define __ASM_ARM_EBIREGS_H 

#ifndef S3C2413_EBIREG
#define S3C2413_EBIREG(x) (S3C2413_VA_EBI + (x))
#endif

/* bus priority decision */
#define S3C2413_EBIPR	    S3C2413_EBIREG(0x0000)

/* Bank configuration register */
#define S3C2413_BANK_CFG	S3C2413_EBIREG(0x0004)

#define S3C2413_BANK_CFG_BANK_2_3_NAND		        (3<<2)

#endif /* __ASM_ARM_EBIREGS_H */
