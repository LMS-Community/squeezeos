/* 
 * nand_cp.c: Copy from nand to destination
 * Copyright (C) Samsung Electronics
 *          SW.LEE <hitchcar@samsung.com>
 *     -- Correct mis-used register (Reset,TranRnB....)
 *     -- add 16Bit NAND
 *
 * MIZI Based Code
 *
 * Copyright (C) 2003	
 * Gianluca Costa <gcoata@libero.it>
 * for 2410 
 *  Under GPLver2
 */

#include <common.h>
#include <linux/mtd/nand.h>
#include "memory.h"
#ifndef CONFIG_SMDK2410
#include <s3c2440.h>
#include <configs/smdk2440nand.h>
#define nand_select()           (NFCONT &= ~(1 << 1))
#define nand_deselect()         (NFCONT |= (1 << 1))
#define nand_clear_RnB()  	(NFSTAT |= (1 << 2))
#define nand_wait()             { while(!(NFSTAT & 0x4)); } /* RnB_TransDectect */

int nandll_read_page(unsigned char *buf, unsigned long addr)
{
        int i;
	unsigned short *ptr16 = (unsigned short *)buf;

        if (addr & NAND_PAGE_MASK) {
                return -1;      /* invalid alignment */
        }

        /* chip Enable */
        nand_select();
        nand_clear_RnB();

        NFCMD = NAND_CMD_READ0;
        /* Write Address */
        NFADDR = addr & 0xff;
        NFADDR = (addr >> 9) & 0xff;
        NFADDR = (addr >> 17) & 0xff;
#ifdef NAND_3_ADDR_CYCLE
#else
        NFADDR = (addr >> 25) & 0xff;
#endif
        nand_wait();

        for(i=0; i < NAND_PAGE_SIZE/2; i++) {
                *ptr16 = NFDATA16;
                ptr16++;
        }
#if 0	
	for (i = 0; i < 8 ; i++)
		printf(" 0x%4x ", NFDATA16);
	printf("\n");
#endif
        /* chip Disable */
        nand_deselect();
        return 0;
}

/* low level nand read function */
int nandll_read_oob(char *buf, unsigned long addr)
{
	int i;
	unsigned long *ptr32 = (unsigned long *)buf;

	if ((addr & NAND_BLOCK_MASK)) {
		return -1;      /* invalid alignment */
	}

	/* Chip Enable */
	nand_select();
	nand_clear_RnB();

	NFCMD = NAND_CMD_READOOB;

	/* Write Address */
	NFADDR = addr & 0xff;
	NFADDR = (addr >> 9) & 0xff;
	NFADDR = (addr >> 17) & 0xff;
#ifdef NAND_3_ADDR_CYCLE
#else
	NFADDR = (addr >> 25) & 0xff;
#endif
	nand_wait();

	for(i=0; i < NAND_OOB_SIZE/4; i++) {
		*ptr32 = NFDATA32;
		ptr32++;
	}
	/* Chip Disable */
	nand_deselect();
	return 0;
}
#else //added by liao on 2005-08-17 for supporting s3c2410
#include <s3c2410.h>
#include <configs/smdk2410nand.h>
#define nand_select()           (NFCONF &= ~0x800)
#define nand_deselect()         (NFCONF |= 0x800)
#define nand_wait()             { while(!(NFSTAT & 0x1)); } /* RnB_TransDectect */

int nandll_read_page(unsigned char *buf, unsigned long addr)
{
        int i;

        if (addr & NAND_PAGE_MASK) {
                return -1;      /* invalid alignment */
        }

        /* chip Enable */
        nand_select();

        NFCMD = NAND_CMD_READ0;
        /* Write Address */
        NFADDR = addr & 0xff;
        NFADDR = (addr >> 9) & 0xff;
        NFADDR = (addr >> 17) & 0xff;
        NFADDR = (addr >> 25) & 0xff;
        nand_wait();

        for(i=0; i < NAND_PAGE_SIZE; i++) {
                *buf = NFDATA8;
                buf++;
        }
        /* chip Disable */
        nand_deselect();
        return 0;
}

/* low level nand read function */
int nandll_read_oob(char *buf, unsigned long addr)
{
	int i;

	if ((addr & NAND_BLOCK_MASK)) {
		return -1;      /* invalid alignment */
	}

	/* Chip Enable */
	nand_select();

	NFCMD = NAND_CMD_READOOB;

	/* Write Address */
	NFADDR = addr & 0xff;
	NFADDR = (addr >> 9) & 0xff;
	NFADDR = (addr >> 17) & 0xff;
	NFADDR = (addr >> 25) & 0xff;
	nand_wait();

	for(i=0; i < NAND_OOB_SIZE; i++) {
		*buf = NFDATA8;
		buf++;
	}
	/* Chip Disable */
	nand_deselect();
	return 0;
}

#endif


int is_bad_block(unsigned long addr)
{
        char oob_buf[16];

        nandll_read_oob((char *)oob_buf, addr);

#ifdef S3C24X0_16BIT_NAND
        if ( (oob_buf[0] != 0xFF) ||  (oob_buf[1] != 0xFF) )
                return 1;
#else
        if (oob_buf[5] != 0xFF)
                return 1;
#endif
        else
                return 0;
}

/*
 * Read data from NAND.
 */
int
nandll_read_blocks(unsigned long dst_addr, unsigned long src_addr, int size)
{
        char *buf = (char *)dst_addr;
        char page_buf[NAND_PAGE_SIZE];
        int i, j;
        
	if ((src_addr & NAND_BLOCK_MASK) || (size & NAND_PAGE_MASK))
                return -1;      /* invalid alignment */

        while (size > 0) {
                /* If this block is bad, go next block */
                if (is_bad_block(src_addr)) {
                        src_addr += NAND_BLOCK_SIZE;
                        continue;
                }

                /* Read block */
                for (i = 0; i < NAND_PAGES_IN_BLOCK; i++) {
                        nandll_read_page((char *)page_buf, src_addr);
                        for (j = 0; j < NAND_PAGE_SIZE; j++)
                                *buf++ = page_buf[j];
                        src_addr += NAND_PAGE_SIZE;
                        size -= NAND_PAGE_SIZE;
                        if (size == 0) break;
                }
        }
        return 0;
}

int copy_uboot_to_ram(void)
{
#ifndef CONFIG_SMDK2410
	NFCONF |= 0xFFF0; /* Max TACLS, TWRPH0, TWRPH1 */
	NFCONT |= (1 << 0) ;   /* Activate NAND Controller */
	/* Copy uboot to ram. 128k is enough. */
#else //added by liao on 2005-08-17 for support s3c2410	
   NFCONF=0xf842;
#endif	
	return nandll_read_blocks(UBOOT_BASE, 0x0, (8*NAND_BLOCK_SIZE));
}




