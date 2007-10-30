/*
 * nand_cp_2413.c: Copy from nand to destination
 *
 * Copright (C) 2005 Samsung Electronics
 *      SW.LEE<hitchcar@samsung.com>
 *      -- added Large and small block READ function for s3c2413
 *      -- This file only dedicated for S3C2413.
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
#include <asm/arch/s3c2413.h>
#include "memory.h"
#include <configs/smdk2413nand.h>
#include <linux/mtd/nand.h>

//#define UBOOT_REAL_SIZE		(0x20000)
#define UBOOT_REAL_SIZE		(0x30000)


#define nand_clear_RnB()	  	(NFSTAT |= (1 << 4))
#define nand_wait()             	{ while(!(NFSTAT & (1 << 4))); }
#define  nand_select() { \
		if (NFCONF & (1<<31)) NFCONT &= ~(1<<1); \
		else NFCONT &= ~(1<<2); \
	}

#define  nand_deselect() { \
		if (NFCONF & (1<<31)) NFCONT |= (1<<1); \
		else NFCONT |= (1<<2); \
	}

int nandll_read_page(unsigned char *buf, unsigned long block, 
			u32 page, struct nand_info *ni)
{
	int i;
	u32  *ptr32 = (u32 *)buf;
	u32 blockPage;

	blockPage = block * ni->cpages_in_block + page;

	/* chip Enable */
	nand_select();
	nand_clear_RnB();
	NFCMD = NAND_CMD_READ0;

	if (ni->cpage2Kis) {
		NFADDR = 0;	/* colum is 0 */ 
		NFADDR = 0; 
		NFADDR = (blockPage) & 0xff;
		NFADDR = (blockPage >> 8) & 0xff;
	//	if (ni->cchip_size > 128) 
			NFADDR = (blockPage >> 16) & 0xff;
		NFCMD = NAND_CMD_READSTART; 
	}
	else {
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR= (blockPage>>8) & 0xff;
		NFADDR=((blockPage>>16)&0xff);
	}
	nand_wait();
	for(i=0; i < ni->cpage_size /4 ; i++) {
		*ptr32++ = NFDATA32;
	}
	
	/* chip Disable */
	nand_deselect();
	return 0;
}

/* low level nand read function */
int nandll_read_oob(char *buf, unsigned long block,struct nand_info *ni)
{
	int i, oob_size;
	u32 *ptr32 = (u32 *)buf;
	u32 blockPage;

	blockPage = block * ni->cpages_in_block;

	oob_size = (ni->cpage_size / 512) * 16; 

	/* Chip Enable */
	nand_select();
	nand_clear_RnB();

	if (ni->cpage2Kis) {
		NFCMD = NAND_CMD_READ0;

		NFADDR = 0;
		NFADDR = (1 << 3);
		NFADDR = (blockPage) & 0xff;
		NFADDR = (blockPage >> 8) & 0xff;
	//	if (ni->cchip_size > 128) 
			NFADDR = (blockPage >> 16) & 0xff;
		NFCMD = NAND_CMD_READSTART; /* extended command for Large block */
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR= (blockPage>>8) & 0xff;
		NFADDR=((blockPage>>16)&0xff);
	}
	nand_wait();

	for(i=0; i < oob_size/4 ; i++)
		*ptr32++ = NFDATA32;
	
	/* Chip Disable */
	nand_deselect();
	return 0;
}

int is_bad_block(unsigned long block,struct nand_info *ni)
{
	char oob_buf[64];
	u32 badblockpos;
                                                                                              
	nandll_read_oob((char *)oob_buf, block ,ni);

	badblockpos = ni->cpage2Kis?0:5;

	if (oob_buf[badblockpos] != 0xFF)
		return 1;
	else 	
		return 0;
}


/*
 * Read data from NAND.
 */
int
nandll_read_blocks_internal(unsigned long dst_addr, u32 src_addr, 
			int size,struct nand_info *ni)
{
        char *buf = (char *)dst_addr;
	u32 block = src_addr,page,j;

	while (size > 0) {
		if (is_bad_block(block,ni)) { // skip bad block
			block++;
			continue;
		}
		/* Read block */
		for (page = 0; page < ni->cpages_in_block; page++) {
			nandll_read_page((char *)buf, block,page,ni);
			buf+=ni->cpage_size;
			size -= ni->cpage_size;
			if (size == 0) break;
		}
		block++;
	}
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

void nand_error_led(void)
{
	while (1) {
		int i;
             	GPFDAT = 0xf0; /* LED 0 0 0 0 */
               	for (i = 0; i < 10000; i++);
        	       GPFDAT = 0x00; /* LED 1 1 1 1 */
	}
}
void nand_arch_init(void)
{
	BANK_CFG |= (0x3<<2);
	GPACON &= ~(0x1f<<17);
    	GPACON |= (0x1f<<17);
	NFCONF |= 0x7770;
	NFCONT |= 0xf7;
}
void nand_config( struct nand_info *ni)
{
        u8 pMID, pDID, n4thcycle;
        u16  nBuff, i;
        
	nand_arch_init();

//	nand_reset();
        n4thcycle = nBuff = 0;
        
	nand_select();
        nand_clear_RnB();
        NFCMD = NAND_CMD_READID;
        NFADDR =  0x00;
        for ( i = 0; i < 200; i++ );
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
		nand_error_led();
        }

//	printf (" nand id %x %x %x %x\n",pMID,pDID,nBuff,n4thcycle);
	/* 0x36 is normal NAND flash */
        if (pDID >= 0xA0) {
                ni->cpage2Kis = 1;
        }
	else {
		ni->cpage2Kis = 0;
	}

	ni->manfid = pMID;
	ni->devid = pDID;

	if (ni->cpage2Kis) {
                ni->cpage_size = 2048;
                ni->cpages_in_block = 64;
        }
        else {
                ni->cpage_size = 512;
                ni->cpages_in_block = 32;
        }
	ni->coob_size = (ni->cpage_size/512)*16;
	ni->cblock_size = ni->cpage_size * ni->cpages_in_block;
}

int nandll_read_blocks(unsigned long dst_addr, u32 src_addr, int size)
{
	struct nand_info ni;
	nand_config(&ni);
	return nandll_read_blocks_internal(dst_addr,src_addr,size,&ni);
}
int copy_uboot_to_ram(void)
{
	nandll_read_blocks(UBOOT_BASE, 0x0,UBOOT_REAL_SIZE);
	return 0;
}

