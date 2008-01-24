/*
 *  
 * (C) Samsung Electronics 
 *       SW.LEE <hitchcar@samsung.com>
 *       - add  nandE, 16Bit NAND
 *       - delete dummy code for S3C2440A
 * (C) Samsung Electrocnis
 *       getfree 
 *
 * Driver for NAND support, Rick Bronson
 * borrowed heavily from:
 * (c) 1999 Machine Vision Holdings, Inc.
 * (c) 1999, 2000 David Woodhouse <dwmw2@infradead.org>
 * 
 * History:
 *	2005-08-05: SW.LEE<hitchcar@samsung.com>
 *		: added RnB Clear in WriteOob function
 *		  to make CMD STATUS 0
 *		  tested in S3C2442
 *      2005-08-9 : SW.LEE<hitchcar@samsung.com>
 *		 supported Large block for s3c2442
 *      2006-01-20 :Naushad <naushad@samsung.com>
 *		Added Software and Hardware ECC Supports in write
 *		Added Same placement scheme as in kernel MTD/nand driver 
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/s3c2413.h>

#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ids.h>
#include <linux/mtd/nand_ecc.h>

#define BAD_CHECK	(1)
#define ECC_CHECK 	(0)

#define ROUND_DOWN(value,boundary)      ((value) & (~((boundary)-1)))

u8 NF8_Spare_Data[16];
static u8 eccbuf[24];
#define FAIL 0
#define OK 1

static u8 NF_CheckId(void);
static int NF_EraseBlock(u32 blockNum);
static int NF_ReadPage(u32 block, u32 page, u8 *buffer);
static int NF_WritePage(u32 block, u32 page, u8 *buffer);
static int NF_IsBadBlock(u32 block);
static int NF_MarkBadBlock(u32 block, int mark_flag);
static void NF_Reset(void);
static void NF_Init(void);
static int NF_WriteOob(u32 block, u32 page, u8 *buffer, int yaffs_option);
int s3c24x0_nand_init(void);
void nand_config( struct nand_info *ni);
////////////////////////////////
void yamutest_main(void);
int yamutest_menu(void);
//////////////////////////////////////////////

struct nand_info cnand_info;

/* This is used for padding purposes in nand_write_oob */
int cpage2Kis;
int cpage_size; /* Current Page Size */
int cpages_in_block;
int coob_size;
int cblock_size;
//int cchip_size;

/*
 * Board Specific Stuff 
 */

#define TACLS           7       // 1-clk(0ns) 
#define TWRPH0          7       // 3-clk(25ns)
#define TWRPH1          7       // 1-clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns

#define NF_SOFT_UnLock()            {NFCONT&=~(1<<16);}
#define NF_MECC_UnLock()            {NFCONT&=~(1<<7);}
#define NF_MECC_Lock()              {NFCONT|=(1<<7);}
#define NF_SECC_UnLock()            {NFCONT&=~(1<<6);}
#define NF_SECC_Lock()              {NFCONT|=(1<<6);}

#define NF_RSTMECC()                 {NFCONT|=(1<<5);}
#define NF_RSTSECC()                 {NFCONT|=(1<<4);}
#define NF_CLEAR_RB()               {NFSTAT |= (1<<4);}
#define NF_DETECT_RB()              {while(!(NFSTAT&(1<<4)));}

void NF_nFCE_L(void)
{
	if(NFCONF & (1<<31)) NFCONT &= ~(1<<1);
	else	NFCONT &= ~(1<<2);
}

void NF_nFCE_H(void)
{ 
	if(NFCONF & (1<<31)) NFCONT |= (1<<1);
	else	NFCONT |= (1<<2);
}

/*
 * NF_Init : 
 */
static void NF_Init(void)
{
	BANK_CFG |= (0x3<<2);
	GPACON &= ~(0x1f<<17);
        GPACON |= (0x1f<<17);

        NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
       // NFCONT = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0);
	NFCONT = (0<<17)|(0<<16)|(0<<10)|(0<<9)|(0<<8)|(1<<7)|(1<<6)|(1<<5)|(1<<4)|(0x3<<1)|(1<<0);

	if( NF_CheckId() != cnand_info.devid)
	{
		printf("\nNand Flash is Changed : Probing Again ...");
		s3c24x0_nand_init();
	}
	if (cpage2Kis) 
		NFCONF |= (1 << 3);
	else
		NFCONF &= ~(1 << 3);
}


static struct nand_oobinfo nand_hwecc_oob64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 4,
	.eccpos = {60,61,62,63},
	.oobfree = { {2,58} }
};

static struct nand_oobinfo nand_swecc_oob64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47, 
		48, 49, 50, 51, 52, 53, 54, 55, 
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};
static struct nand_oobinfo yaffs_oobinfo = {
        .useecc = 1,
        .eccbytes = 6,
        .eccpos = {8, 9, 10, 13, 14, 15}
};


int s3c24x0_nand_init(void)
{
	printf("Nand Flash Probe...\n");
	nand_config(&cnand_info);

	cpage2Kis = cnand_info.cpage2Kis;
	cpage_size = cnand_info.cpage_size;
	cpages_in_block = cnand_info.cpages_in_block;
	coob_size = cnand_info.coob_size;
	cblock_size = cnand_info.cblock_size;
	//cchip_size = cnand_info.cchip_size;

	printf("Nand Flash Configuration:\nPage Size  :%d Bytes\nOob Size   :%d Bytes\nBlock Size :%d KB\n",cpage_size,coob_size,cblock_size >> 10);
	//printf("Total Memory Size: %d MBytes\n",cchip_size);
	return 0;

}

int s3c24x0_nand_read(uint srcBlock,uint readSize, uint dstAddress)
{
	u8 *dstPt ;
	u32 blockIndex,i;

	blockIndex = srcBlock;
	dstPt = (u8 *)dstAddress;

	while(1) {
#if BAD_CHECK       
		if(NF_IsBadBlock(blockIndex)) {	// 1:bad 0:good
			printf("Skipping Bad Block %d (0x%x)\n",blockIndex,blockIndex);
			blockIndex++;  
			continue;
		}
#endif
		for(i=0; i< cpages_in_block ;i++) {
			if(!NF_ReadPage(blockIndex,i,dstPt)) {
				printf("Error in Reading Block %d (0x%x) Page %d\n",blockIndex,blockIndex,i);
			}
			dstPt += cpage_size;
			if((u32)dstPt >= (dstAddress+readSize)) // Check end of buffer
				break;
		}
		blockIndex++;   // for next block
		if((u32)dstPt >= (dstAddress+readSize)) // Check end of buffer
			break;
	}
	return 0;

}

int s3c24x0_nand_write(uint targetBlock,uint targetSize, uint srcAddress, int flag, int yaffs_option )
{
	int i;
	int programError=0;
	u8 *srcPt,*saveSrcPt;
	u32 blockIndex;

	printf("NAND Flash writing\n");
	printf("Source base address    =0x%x\n",srcAddress);
	printf("Target start block num =%d\n",targetBlock);
	printf("Target size            =0x%x\n",targetSize);
	printf("block writed :\n");

	srcPt=(u8 *)srcAddress;
	blockIndex=targetBlock;
	while(1) {
		saveSrcPt=srcPt;	
#if BAD_CHECK       
		if(NF_IsBadBlock(blockIndex)) {	// 1:bad 0:good
			blockIndex++;   // for next block
			continue;
		}
#endif
		if(!NF_EraseBlock(blockIndex)) {
			blockIndex++;   // for next block
			printf(" Error->  Erase Block %d (0x%x)  \n",(int)blockIndex,blockIndex);
			continue;
		}

		for(i=0; i< cpages_in_block ;i++) {
			if(!NF_WritePage(blockIndex,i,srcPt)) {
				// block num, page num, buffer
				programError=1;
				break;
			}
#if ECC_CHECK    

			if ( flag != NF_RW_YAFFS ) { 
				if(!NF_ReadPage(blockIndex,i,srcPt)) {
					printf("ECC Error(block=%d,page=%d!!!\n",(int)blockIndex,i);
				}
			}
#endif	    
			if (( flag == NF_RW_YAFFS) && (yaffs_option == NF_USE_MTD_ECC )) {
				// Software ECC Calculation
				nand_calculate_pageecc(srcPt,eccbuf,cpage_size);
			}
					
			srcPt += cpage_size;
			if ( flag == NF_RW_YAFFS ) {
				if (!NF_WriteOob(blockIndex,i,srcPt, yaffs_option)) {
					programError = 1;
				break;
				}
				srcPt += coob_size;
			}
			//printf(".");
			if((u32)srcPt>=(srcAddress+targetSize)) // Check end of buffer
				break;	// Exit for loop
		}
		if(programError==1) {
			blockIndex++;
			srcPt=saveSrcPt;
			programError=0;
			continue;
		}

		printf( "%X ", blockIndex );

		if (!( blockIndex % 16 )) printf("\n");
		if((u32)srcPt>=(srcAddress+targetSize))
			break;	// Exit while loop
		blockIndex++;
	}

	printf("\n\n");
	return 0;
}


int do_nandbb(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int startblk, numblks, i;

	if ( argc != 3 ){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	startblk = simple_strtoul(argv[1], NULL, 16);
	numblks  = simple_strtoul(argv[2], NULL, 16);
	printf("StartBlock %d (0x%x) : NumBlock %d (0x%x) \n",startblk,startblk,numblks, numblks);

	for (i=startblk; i<startblk+numblks; i++) {
		NF_IsBadBlock(i); // prints bad blocks
	}

	return 0;
}


int do_nandmb(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int blk;

	if ( argc != 2 ){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	blk = simple_strtoul(argv[1], NULL, 16);
	printf("Mark block %d (0x%x) as BAD\n", blk, blk);

	NF_Init();
	NF_Reset();

	NF_MarkBadBlock(blk, NF_BB_ON);
	NF_IsBadBlock(blk);

	return 0;
}


static int NF_CheckBadNande(u32 block);
int do_nande    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int startblk, size,eraseblocks,i;

	if ( argc != 3 ){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	printf("StartBlock %d (0x%x) : Size %d (0x%x) \n",startblk,startblk,size, size);
	eraseblocks = size / cblock_size;
	printf("Total Erase Blocks %d (0x%x) \n",eraseblocks, eraseblocks);

	NF_Init();
	NF_Reset();
	for ( i=0 ; i < eraseblocks;  i++) {
		NF_EraseBlock(startblk);
		NF_CheckBadNande(startblk);
		startblk++;
	}
	return 0;
}


int do_nandE    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int startblk, size,eraseblocks,i;

	if ( argc != 3 ){
                printf ("Usage:\n%s\n", cmdtp->usage);
                return 1;
        }
        startblk = simple_strtoul(argv[1], NULL, 16);
        size  = simple_strtoul(argv[2], NULL, 16);
  	printf("StartBlock %d (0x%x) : Size %d (0x%x) \n",startblk,startblk,size, size);
       	eraseblocks = size / cblock_size;
        printf("Total Erase Blocks %d (0x%x) \n",eraseblocks, eraseblocks);

	NF_Init();
	NF_Reset();
        for ( i=0 ; i < eraseblocks;  i++) {
                NF_EraseBlock(startblk);
                startblk++;
        }
	return 0;
}
#if (CONFIG_COMMANDS & CFG_CMD_BOARD_TEST)
int nandw_upgrade    (ulong startblk,ulong size, ulong memadr)
{
	NF_Init();
	NF_Reset();
	s3c24x0_nand_write(startblk,size,memadr, NF_RW_NORMAL, 0 );
	return 0;
}
#endif
int do_nandw    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	startblk,size,memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);
	NF_Init();
	NF_Reset();
	s3c24x0_nand_write(startblk,size,memadr, NF_RW_NORMAL, 0 );
	return 0;
}

int do_nandyw    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong   startblk,size,memadr;
	int ecc_option;

	if (argc !=  5) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);

	switch(argv[4][0]) {
		case 'y': ecc_option = NF_USE_YAFFS_ECC;	
			  break;
		case 'm': ecc_option = NF_USE_MTD_ECC;	
			  break;
		case 'h': ecc_option = NF_USE_HW_ECC;	
			  break;
		default : 
			  printf("Invalid ECC Option! Usage : %s",cmdtp->usage);
			  return 1;
	}

	if((ecc_option == NF_USE_YAFFS_ECC) && (cpage_size != 512)) {
		printf("Yaffs ECC option is supported only for small page devices\r\n");
		return 1;
	}  
	

	// yaffs image don't use 1st block
	startblk++;

	NF_Init();
	NF_Reset();
	s3c24x0_nand_write(startblk, size, memadr, NF_RW_YAFFS, ecc_option );

	return 0;
}

//extern int nandll_read_blocks(unsigned long, unsigned long, int,struct nand_info *);

int do_nandr    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	startblk,size,memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	//startblk = cblock_size*simple_strtoul(argv[1], NULL, 16);
	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);
	NF_Init();
	NF_Reset();
	s3c24x0_nand_read(startblk,size,memadr);
	return 0;
}

U_BOOT_CMD(
	nandbb,	4,	1,	do_nandbb,
       "nandbb HEX: targetblock numblocks \n",
       "\n	- SMDK24X0 NAND Flash Bad Block Check\n"	\
       "nandbb targetblock numblocks \n"	\
);

U_BOOT_CMD(
	nandmb,	4,	1,	do_nandmb,
       "nandmb HEX: targetblock \n",
       "\n	- SMDK24X0 NAND Flash Mark Bad Block\n"	\
       "nandmb targetblock \n"	\
);

U_BOOT_CMD(
	nandw,	4,	1,	do_nandw,
       "nandw HEX: targetblock targetsize mem_addr \n",
       "\n	- SMDK24X0 NAND Flash Write Program\n"		\
       "nandw targetblock targetsize memory addr \n"	\
);

U_BOOT_CMD(
    nandyw, 5,  1,  do_nandyw,
       "nandyw - HEX: targetblock targetsize mem_addr ecc_option(y/m/h)\n",
       "\n  - SMDK24X0 NAND Flash Write Command for Yaffs\n"        \
       "nandyw targetblock targetsize memory addr ecc_option(y/m/h)\n"    \
       "   ecc_option : [y] : use yaffs ecc (only in 512 byte/page devices)\n"\
       "              : [m] : use mtd ecc (software ecc) /  \n"  \
       "              : [h] : use hardware ecc  \n"  \
);

U_BOOT_CMD(
	nande,	4,	1,	do_nande,
       "nande  startBlock size \n",
       "	- delete blocks while skipping bad block \n"		\
);

U_BOOT_CMD(
	nandE,	4,	1,	do_nandE,
       "nandE  startBlock size \n",
       "       - delete all blocks in given range regardless Bad Blcok\n"\
);

U_BOOT_CMD(
	 nandr,	4,	1,	do_nandr,
	 "nandr - HEX: targetblock targetsize mem_adr \n",
	 "\n	-SMDK24X0 NAND Flash Read Program\n"	\
	 "nandw targetblock, targetsize memory addr \n"	\
);

static u8 seBuf[64]=
{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

static int NF_MarkBadBlock(u32 block, int mark_flag)
{
	int i;
	u32 blockPage = block * cpages_in_block;

	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		seBuf[0]=0x44;   // Bad blcok mark=0	
		if ( mark_flag == NF_BB_OFF ) seBuf[0] = 0xff;
		NFCMD = NAND_CMD_SEQIN;
		NFADDR = cpage_size & 0xff;
		NFADDR = (cpage_size >> 8) & 0x0f;
		NFADDR = blockPage & 0xff;
		NFADDR= (blockPage>>8)& 0xff;
	//	if (cchip_size > 128) 
			NFADDR = (blockPage>>16) & 0xff;
	} 
	else {
		seBuf[5]=0x44;   // Bad blcok mark=0	
		if ( mark_flag == NF_BB_OFF ) seBuf[5] = 0xff;

		NFCMD = NAND_CMD_READOOB;
		NFCMD = NAND_CMD_SEQIN;
		NFADDR = 0;
		NFADDR = (blockPage)&0xff;
		NFADDR = (blockPage>>8)&0xff;
		NFADDR = (blockPage>>16)&0xff;
	}
	
	for(i=0 ;i < coob_size ;i++){
		NFDATA8 = seBuf[i];
	}

	NFCMD= NAND_CMD_PAGEPROG;

	for(i=0;i<10;i++); 
	NF_DETECT_RB();

	NFCMD=NAND_CMD_STATUS;
	for(i=0;i<3;i++); 

	if (NFDATA8 & 0x1) {
		NF_nFCE_H();
		printf("[Program error is occurred but ignored]\n");
	}
	else {
		NF_nFCE_H();
	}
	if ( mark_flag == NF_BB_ON) {
		printf("[block %d (0x%x) is marked as a bad block]\n",block,block);
	} else {
		printf("[block %d (0x%x) is marked as a good block]\n",block,block);
	}
	return 1;
} 

static int NF_ReadPage(u32 block,u32 page,u8 *buffer)
{
	int i;
	u32 blockPage;
	u32 *ptr32 = (u32 *)buffer;

	blockPage =block * cpages_in_block + page;


	NF_nFCE_L();
	NF_CLEAR_RB();

	NFCMD = NAND_CMD_READ0;

	if (cpage2Kis) {
		NFADDR = 0;	/* colum is 0 */ 
		NFADDR = 0; 
		NFADDR = (blockPage) & 0xff;
		NFADDR = (blockPage >> 8) & 0xff;
		//if (cchip_size > 128) 
			NFADDR = (blockPage >> 16) & 0xff;
		NFCMD = NAND_CMD_READSTART; 
	}
	else {
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR= (blockPage>>8) & 0xff;
		NFADDR=((blockPage>>16)&0xff);
	}

	NF_DETECT_RB();
	for(i=0; i < cpage_size /4 ; i++) {
		*ptr32++ = NFDATA32;
	}
	
	NF_nFCE_H();
	return OK;
}

static int NF_WritePage(u32 block,u32 page,u8 *buffer)
{
	int i;
	u32 blockPage;
	u32 *ptr32 = (u32 *)buffer;
	u32 mecc;
	u8 status;

	blockPage =block * cpages_in_block + page;

	NF_RSTMECC();
	NF_MECC_UnLock();

	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR=(blockPage>>8) & 0xff;
//		if (cchip_size > 128) 
			NFADDR=(blockPage>>16) & 0xff;
	}
	else {
		NFCMD = NAND_CMD_READ0;
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR= (blockPage>>8) & 0xff;
		NFADDR=((blockPage>>16)&0xff);
	}

	for(i=0; i < cpage_size / 4; i++) {
		NFDATA32 = *ptr32++;
	}

	// Calculate ECC
	NF_MECC_Lock();

	NFCMD = NAND_CMD_PAGEPROG;
	for(i=0; i<10 ;i++);

	/* 
         * 2442 uses 28 bit ECC for both 512 and  2048 sized pages
         */

	mecc = NFMECC0;
	
	eccbuf[0] = mecc & 0xff;
	eccbuf[1] = (mecc >> 8) & 0xff;
	eccbuf[2] = (mecc >> 16) & 0xff;
	eccbuf[3] = (mecc >> 24) & 0xff;
	
	NF_DETECT_RB();

	NFCMD = NAND_CMD_STATUS;
	for(i=0;i<10;i++); 
	status = NFDATA8;

	if (status & 0x1) {// Page write error
		NF_nFCE_H();
		printf("[PROGRAM_ERROR:block#=%d (0x%x)]\n",block,block);
		NF_MarkBadBlock(block, NF_BB_ON);
		return FAIL;
	} else {
		NF_nFCE_H();
		return OK;
	}
}

static int NF_WriteOob(u32 block,u32 page,u8 *buffer, int yaffs_option)
{
	int i,j;
	u32 blockPage = (block * cpages_in_block) + page;
	u8 oobBuf[64],ooblen;
	struct nand_oobinfo *oobinfo; 

	if(cpage2Kis) {
		if(yaffs_option == NF_USE_HW_ECC) 
			oobinfo = &nand_hwecc_oob64;
		else
			oobinfo = &nand_swecc_oob64;
		memset(oobBuf,0xff,coob_size);
		ooblen = coob_size  - oobinfo->eccbytes - 2;
		// Copy oobdata supplied
		for(i=0,j=0;j<ooblen;i++) {
			int from = oobinfo->oobfree[i][0];
			int len = oobinfo->oobfree[i][1];
			memcpy(&oobBuf[from],(buffer+j),len);
			j+= len;
		}
		// Place ecc bytes
		for(i=0;i<oobinfo->eccbytes;i++) {
			oobBuf[oobinfo->eccpos[i]] = eccbuf[i];
		}

	}else {
		for(i=0;i<coob_size;i++)
			oobBuf[i] = *(buffer + i);
		if(yaffs_option != NF_USE_YAFFS_ECC) {
			oobinfo = &yaffs_oobinfo;
			// Place ecc bytes
			for(i=0;i<oobinfo->eccbytes;i++) {
				oobBuf[oobinfo->eccpos[i]] = eccbuf[i];
			}
		}
	} 
	NF_nFCE_L();
	NF_CLEAR_RB();	/* to make CMD_STATUS have 0 */

	if(cpage2Kis) {
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= (1 << 3);
		NFADDR= blockPage & 0xff;
		NFADDR=(blockPage>>8) & 0xff;
		//if (cchip_size > 128) 
			NFADDR=(blockPage>>16) & 0xff;
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR= (blockPage>>8) & 0xff;
		NFADDR=((blockPage>>16)&0xff);
	}

	for(i=0; i < coob_size ; i++) {
		NFDATA8 = oobBuf[i];
	}

	NFCMD = NAND_CMD_PAGEPROG;
	for(i=0; i<10 ;i++);
	NF_DETECT_RB();
	for(i=0; i<1000 ;i++);
	NFCMD = NAND_CMD_STATUS;
	for(i=0; i<3; i++);
	{
		unsigned char ret = NFDATA8;
		if (ret & 0x1) { 
			NF_nFCE_H();
			printf("[Program error is occurred but ignored] %x\n", ret);
		}
		else {
			NF_nFCE_H();
		}
	}
	return 1;
}


static u8 NF_CheckId(void)
{
        int i;
        u8 maf_id,dev_id;

        NF_nFCE_L();
        NFCMD = NAND_CMD_READID;
        NFADDR = 0x0;

        for(i=0;i<10;i++);

        maf_id=NFDATA8;
        dev_id=NFDATA8;
        NF_nFCE_H();

        return dev_id;
}


void NF_Reset(void)
{
	int i;

	NF_nFCE_L();
	NF_CLEAR_RB();
	
 	NFCMD = NAND_CMD_RESET;

	for(i=0;i<10;i++);
	NF_DETECT_RB();
	NF_nFCE_H();
}
/*
 * NF_EraseBlock
 */
static int NF_EraseBlock(u32 block)
{
	u32 blockPage = block * cpages_in_block;
	int i;
	NF_SOFT_UnLock();

	NF_nFCE_L();
	NF_CLEAR_RB();

	NFCMD = NAND_CMD_ERASE1;
	if (cpage2Kis) {
		NFADDR = blockPage & 0xff;
		NFADDR = (blockPage>>8) & 0xff;
		//if (cchip_size > 128) 
			NFADDR = (blockPage>>16) & 0xff;
	}
	else {
		NFADDR = blockPage&0xff;
		NFADDR = (blockPage>>8)&0xff;
		NFADDR = (blockPage>>16)&0xff;
	}
	NFCMD = NAND_CMD_ERASE2;
	for(i=0;i<40;i++);
	NF_DETECT_RB();
	NFCMD = NAND_CMD_STATUS;

	if (NFDATA8 & 0x1) {
		NF_nFCE_H();
		printf("[ERASE_ERROR:block#=%d (0x%x)]\n",block,block);
		NF_MarkBadBlock(block,NF_BB_ON);
		return FAIL;
	}
	else {
		NF_nFCE_H();
		return OK;
	}
}

static int NF_CheckBadNande(u32 block)
{
	int i;
	unsigned int blockPage;
	u16 data = 0;

	blockPage = block * cpages_in_block;

	//NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_READ0;
		NFADDR = cpage_size & 0xff;
		NFADDR = (cpage_size >>8) & 0x0f;
		NFADDR = blockPage & 0xff;
		NFADDR = (blockPage >> 8)  & 0xff;
		//if (cchip_size > 128) 
			NFADDR = (blockPage>>16) & 0xff;
		/* extended command for Large block */
		NFCMD = NAND_CMD_READSTART;
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		NFADDR = ( cpage_size + 0) & 0xf;
		NFADDR = blockPage&0xff;    // The mark of bad block is in 0 page
		NFADDR = (blockPage>>8)&0xff;   // For block number A[24:17]
		NFADDR =(blockPage>>16)&0xff;  // For block number A[25]
	}

	for(i=0;i<10;i++);  /* dummy check me */
	NF_DETECT_RB();

	for (i = 0 ; i < coob_size /2 ;i++){
		data=NFDATA16;
		if ( data != 0xffff ) break;
	}
	NF_nFCE_H();
	if (i != (coob_size/2)) {
		printf("[block %d (0x%x) is bad block(%x)]\n",block,block,data);
		return 1;
	}
	return 0;
}


static int NF_IsBadBlock(u32 block)
{
	int i;
	unsigned int blockPage;
	u8 data,oobbuf[64];

	blockPage = block * cpages_in_block ; 

	//NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_READ0;
		NFADDR= 0;
		NFADDR= (1 << 3);
		NFADDR = blockPage  & 0xff;   // For block number A[24:17]
		NFADDR = (blockPage >> 8) & 0xff;  // For block number A[25]
	//	if (cchip_size > 128) 
			NFADDR = (blockPage>>16) & 0xff;
	        NFCMD = NAND_CMD_READSTART;
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		NFADDR = 0;
		NFADDR = blockPage & 0xff;    // The mark of bad block is in 0 page
		NFADDR = (blockPage >> 8) & 0xff;   // For block number A[24:17]
		NFADDR = (blockPage >> 16) & 0xff;  // For block number A[25]
	}

	for(i=0;i<10;i++);  /* dummy check me */
	NF_DETECT_RB();

	//data=(u8)NFDATA16;
	for(i=0;i<coob_size;i++)
		oobbuf[i] = NFDATA8;

	if (cpage2Kis) 
		data = oobbuf[0];
	else
		data = oobbuf[5];

	NF_nFCE_H();

	if(data != 0xff)
	{
		printf("[block %d (0x%x) has been marked as a bad block(%x)]\n",block,block,data);
		return 1;
	}
	return 0;
}

#endif /* (CONFIG_COMMANDS & CFG_CMD_NAND) */
