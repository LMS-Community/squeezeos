/*
 * nand_cp_2442.c: Copy from nand to destination
 *
 * Copright (C) 2005 Samsung Electronics
 *      SW.LEE<hitchcar@samsung.com>
 *      -- added Large and small block READ function for s3c2442
 *      -- This file only dedicated for S3C2442.
 *      -- make use of GSTATUS4 as information register for NAND flash.
 * 
 * Copyright (C) Samsung Electronics
 *          SW.LEE <hitchcar@samsung.com>
 *     -- Correct mis-used register (Reset,TranRnB....)
 *     -- add 16Bit NAND
 *
 * MIZI Based Code
 *
 * Copyright (C) 2003	
 * Gianluca Costa <gcoata@libero.it>
 * 
 *  Under GPLver2
 */

#include <common.h>
#include <s3c2440.h>
#include "memory.h"
#include <configs/smdk2440nand.h>
#include <linux/mtd/nand.h>

#define nand_select()           (NFCONT &= ~(1 << 1))
#define nand_deselect()         (NFCONT |= (1 << 1))
#define nand_clear_RnB()  	(NFSTAT |= (1 << 2))
#define nand_wait()             { while(!(NFSTAT & 0x4)); } /* RnB_TransDectect */
#define UBOOT_REAL_SIZE		128*1024

/* Global variable used in cmd_nand_s3c2442.c */
int cpage2Kis;
int cpage_size;	/* Current Page Size */
int cpages_in_block;
int coob_size;
int cblock_size;


/* GSTATUS4 is scratch register and we will use it as temporary */
typedef struct  {
	u32 cpage_size:16;
	u32 cpages_in_block:8;
	u32 cpage2Kis:8;
} nandif_t __attribute__((packed));

	
int nandll_read_page(unsigned char *buf, unsigned long addr)
{
	int i;
	u32  *ptr32 = (u32 *)buf;
	nandif_t ni ;

        ni.cpage_size  = (GSTATUS4>>16) & 0xffff;
        ni.cpage2Kis =  (GSTATUS4) & 0xff;
        ni.cpages_in_block = (GSTATUS4>>8) & 0xff;

	if (addr & (ni.cpage_size - 1)) {
		return -1;      /* invalid alignment */
	}

	/* chip Enable */
	nand_select();
	nand_clear_RnB();
	NFCMD = NAND_CMD_READ0;

	if (ni.cpage2Kis) {
		/* Write Address */
		NFADDR = 0;	/* colum is 0 */ 
		NFADDR = 0; 
		NFADDR = (addr >> 11) & 0xff;
		NFADDR = (addr >> 19) & 0xff;
		/* extended command for Large block */
		NFCMD = NAND_CMD_READSTART;
	}
	else {
		NFADDR = addr & 0xff;
		NFADDR = (addr >> 9) & 0xff;
		NFADDR = (addr >> 17) & 0xff;
		NFADDR = (addr >> 25) & 0xff;
	}

	nand_wait();
	for(i=0; i < ni.cpage_size /4 ; i++) {
		*ptr32++ = NFDATA32;
	}
	
	/* chip Disable */
	nand_deselect();
	return 0;
}

/* low level nand read function */
int nandll_read_oob(char *buf, unsigned long addr)
{
	int i, oob_size;
	u32 *ptr32 = (u32 *)buf;
	nandif_t ni ;

        ni.cpage_size  = (GSTATUS4>>16) & 0xffff;
        ni.cpage2Kis =  (GSTATUS4) & 0xff;
        ni.cpages_in_block = (GSTATUS4>>8) & 0xff;
	oob_size = (ni.cpage_size / 512) * 16; 

	if ((addr & ( ni.cpage_size * ni.cpages_in_block - 1 ))) {
		return -1;      /* invalid alignment */
	}

	/* Chip Enable */
	nand_select();
	nand_clear_RnB();

	if (ni.cpage2Kis) {
		NFCMD = NAND_CMD_READ0;
		NFADDR = 0;
		NFADDR = (1 << 3);
		NFADDR = (addr >> 11) & 0xff;
		NFADDR = (addr >> 19) & 0xff;
		/* extended command for Large block */
		NFCMD = NAND_CMD_READSTART;
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		/* Write Address */
		NFADDR = addr & 0xff;
		NFADDR = (addr >> 9) & 0xff;
		NFADDR = (addr >> 17) & 0xff;
		NFADDR = (addr >> 25) & 0xff;
	}
	nand_wait();

	for(i=0; i < oob_size/4 ; i++)
		*ptr32++ = NFDATA32;
	
	/* Chip Disable */
	nand_deselect();
	return 0;
}

int is_bad_block(unsigned long addr)
{
	char oob_buf[64];
        nandif_t ni ;
                                                                                              
        ni.cpage2Kis =  (GSTATUS4) & 0xff;

	nandll_read_oob((char *)oob_buf, addr);

	if(ni.cpage2Kis){
		if (oob_buf[0] != 0xFF) return 1;
		else 			return 0;
	}

	if (oob_buf[5] != 0xFF)
		return 1;
	else 	return 0;
}


/*
 * Read data from NAND.
 */
int
nandll_read_blocks(unsigned long dst_addr, unsigned long src_addr, int size)
{
        char *buf = (char *)dst_addr;
        u8	 page_buf[2048];	/* Check stack overflow */
        int i, j, block_size;
	nandif_t ni ;

        ni.cpage_size  = (GSTATUS4>>16) & 0xffff;
        ni.cpage2Kis =  (GSTATUS4) & 0xff;
        ni.cpages_in_block = (GSTATUS4>>8) & 0xff;
	block_size = ni.cpage_size * ni.cpages_in_block;

	if ((src_addr & (ni.cpage_size * ni.cpages_in_block - 1)) 
		|| (size & (ni.cpage_size -1 )))
                return -1;      /* invalid alignment */
	
	while (size > 0) {
#if 1
		/* If this block is bad, go next block */
		if (is_bad_block(src_addr)) {
			src_addr += block_size;
			continue;
		}
#endif
		/* Read block */
		for (i = 0; i < ni.cpages_in_block; i++) {
			nandll_read_page((char *)page_buf, src_addr);
			for (j = 0; j < ni.cpage_size; j++)
				*buf++ = page_buf[j];
			src_addr += ni.cpage_size;
			size -= ni.cpage_size;
			if (size == 0) break;
		}
	}
	GPFDAT = 0xf0;
        return 0;
}

void nand_reset(void)
{
	nand_select();
        nand_clear_RnB();
	NFCMD = 0xff;
	nand_wait();
	nand_deselect();
}

void fill_nandinfo(void)
{
	nandif_t ni;
	ni.cpage_size  = (GSTATUS4>>16) & 0xffff;
	ni.cpage2Kis =  (GSTATUS4) & 0xff;
	ni.cpages_in_block = (GSTATUS4>>8) & 0xff;

	cpage2Kis = ni.cpage2Kis;
	cpage_size = ni.cpage_size;	/* Current Page Size */
	cpages_in_block = ni.cpages_in_block;
	coob_size = (ni.cpage_size/512)*16;
	cblock_size = ni.cpage_size * ni.cpages_in_block;
}

void nand_config(void)
{
        u8 pMID, pDID, n4thcycle;
        u16  nBuff, i;
	nandif_t ni;
        
	NFCONF = 0xFFF0; /* Max TACLS, TWRPH0, TWRPH1 */
	NFCONT = (1 << 0) ;   /* Activate NAND Controller */
	/* Copy uboot to ram. 128k is enough. */

//	nand_reset();
        cpage2Kis = 0;
        n4thcycle = nBuff = 0;
        
	nand_select();
        nand_clear_RnB();
        NFCMD = NAND_CMD_READID;
        NFADDR =  0x00;
        for ( i = 0; i < 100; i++ );
        /* tREA is necessary to Get a MID. */
        for (i = 0; i < 5; i++) {
                pMID = NFDATA8;
                if (0xEC == pMID)
                        break;
        }
        pDID = NFDATA8;
        nBuff =  NFDATA8;
        n4thcycle =  NFDATA8;
        nand_deselect();
        
        if (pMID != (u8)0xEC){
                while (1) {
                        GPFDAT = 0xf0; /* LED 0 0 0 0 */
                        for (i = 0; i < 10000; i++);
                        GPFDAT = 0x00; /* LED 1 1 1 1 */
                }
        }

	/* 0x36 is normal NAND flash */
        if (pDID >= 0xA0) {
                ni.cpage2Kis = 1;
        }
	else {
		ni.cpage2Kis = 0;
	}

	if (ni.cpage2Kis) {
                ni.cpage_size = 2048;
                ni.cpages_in_block = 64;
        }
        else {
                ni.cpage_size = 512;
                ni.cpages_in_block = 32;
        }

	GSTATUS4 = ((ni.cpage_size <<16) & 0xffff0000 ) |
	           ((ni.cpages_in_block <<8)& 0xff00) | 
	           ((ni.cpage2Kis  )& 0xff );
}

int copy_uboot_to_ram(void)
{
	nand_config();
	nandll_read_blocks(UBOOT_BASE, 0x0,UBOOT_REAL_SIZE);
	GPFDAT = 0xf0;
	return 0;
}

