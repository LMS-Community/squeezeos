/* linux/include/asm/arch-s3c2413/regs-iis.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2413 IIS register definition
 *
 *  Changelog:
 *    19-06-2003     BJD     Created file
 *    26-06-2003     BJD     Finished off definitions for register addresses
 *    12-03-2004     BJD     Updated include protection
 */

#ifndef __ASM_ARCH_REGS_IIS_H
#define __ASM_ARCH_REGS_IIS_H

#define S3C2413_IISREG(x) ((x) + S3C2413_VA_IIS)

#define S3C2413_IISCON	 S3C2413_IISREG(0x00)

#define S3C2413_IISCON_LRINDEX	  (1<<11)
#define S3C2413_IISCON_FTXEMPT    (1<<10)
#define S3C2413_IISCON_FRXEMPT    (1<<9)
#define S3C2413_IISCON_FTXFULL    (1<<8)
#define S3C2413_IISCON_FRXFULL    (1<<7)
#define S3C2413_IISCON_TXDMAPAUSE (1<<6)
#define S3C2413_IISCON_RXDMAPAUSE (1<<5)
#define S3C2413_IISCON_TXCHPAUSE  (1<<4)
#define S3C2413_IISCON_RXCHPAUSE  (1<<3)
#define S3C2413_IISCON_TXDMACTIVE (1<<2)
#define S3C2413_IISCON_RXDMACTIVE (1<<1)
#define S3C2413_IISCON_I2SACTIVE  (1<<0)


#define S3C2413_IISMOD	  S3C2413_IISREG(0x04)

#define S3C2413_IISMOD_IMS_INTERNAL_MASTER	  (0<<10)
#define S3C2413_IISMOD_IMS_EXTERNAL_MASTER_DIV	  (1<<10)
#define S3C2413_IISMOD_IMS_EXTERNAL_MASTER_BYP	  (2<<10)
#define S3C2413_IISMOD_TXMODE	  (0<<8)
#define S3C2413_IISMOD_RXMODE	  (1<<8)
#define S3C2413_IISMOD_TXRXMODE	  (2<<8)

#define S3C2413_IISMOD_LR_LLOW	  (0<<7)
#define S3C2413_IISMOD_LR_RLOW	  (1<<7)
#define S3C2413_IISMOD_IIS	  (0<<5)
#define S3C2413_IISMOD_MSB	  (1<<5)
#define S3C2413_IISMOD_LSB	  (2<<5)
#define S3C2413_IISMOD_256FS	  (0<<3)
#define S3C2413_IISMOD_512FS	  (1<<3)
#define S3C2413_IISMOD_384FS	  (2<<3)
#define S3C2413_IISMOD_768FS	  (3<<3)

#define S3C2413_IISMOD_16FS	  (2<<1)
#define S3C2413_IISMOD_32FS	  (0<<1)
#define S3C2413_IISMOD_48FS	  (1<<1)
#define S3C2413_IISMOD_8BIT	  (1<<0)
#define S3C2413_IISMOD_16BIT  (0<<0)



#define S3C2413_IISFIC	S3C2413_IISREG(0x08)

#define S3C2413_IISFIC_TFLUSH	(1<<15)
#define S3C2413_IISFIC_RFLUSH	(1<<7)

#define S3C2413_IISPSR		S3C2413_IISREG(0x0c)

#define S3C2413_IISPSR_PSRAEN	  (1<<15)
#define S3C2413_IISPSR_PSVALA(x)	  (x<<8)

#define S3C2413_PA_IIS_FIFO_TX  0x55000010
#define S3C2413_PA_IIS_FIFO_RX  0x55000014

#endif /* __ASM_ARCH_REGS_IIS_H */

