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
#include <s3c2440.h>

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
int NF_ReadPage(u32 block, u32 page, u8 *buffer);
static int NF_WritePage(u32 block, u32 page, u8 *buffer);
static int NF_IsBadBlock(u32 block);
static int NF_MarkBadBlock(u32 block, int mark_flag);
static void NF_Reset(void);
static void NF_Init(void);
static int NF_WriteOob(u32 block, u32 page, u8 *buffer, int yaffs_option);


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


/* This is used for padding purposes in nand_write_oob */
extern int cpage2Kis;
extern int cpage_size; /* Current Page Size */
extern int cpages_in_block;
extern int coob_size;
extern int cblock_size;


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
			printf(" Error->  Erase Block %d  \n",(int)blockIndex);
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

extern int nandll_read_blocks(unsigned long, unsigned long, int);

int do_nandr    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	startblk,size,memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = cblock_size*simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);
	NF_Init();
	NF_Reset();
	nandll_read_blocks(memadr,startblk,size);
	return 0;
}

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

#define NF_SOFT_UnLock()      	 	{NFCONT&=~(1<<12);}
#define NF_MECC_UnLock()      	 	{NFCONT&=~(1<<5);}
#define NF_MECC_Lock()      	  	{NFCONT|=(1<<5);}
#define NF_SECC_UnLock()  	     	{NFCONT&=~(1<<6);}
#define NF_SECC_Lock()      		{NFCONT|=(1<<6);}
#define NF_nFCE_L()                     {NFCONT&=~(1<<1);}
#define NF_nFCE_H()                     {NFCONT|=(1<<1);}
#define NF_RSTECC()                     {NFCONT|=(1<<4);}
#define NF_CLEAR_RB()                   {NFSTAT |= (1<<2);}
#define NF_DETECT_RB()                  {while(!(NFSTAT&(1<<2)));}

#define TACLS           7       // 1-clk(0ns) 
#define TWRPH0          7       // 3-clk(25ns)
#define TWRPH1          7       // 1-clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns

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
		printf("[block 0x%x is marked as a bad block]\n",block);
	} else {
		printf("[block 0x%x is marked as a good block]\n",block);
	}
	return 1;
} 


static int NF_WritePage(u32 block,u32 page,u8 *buffer)
{
	int i;
	u32 blockPage;
	u32 *ptr32 = (u32 *)buffer;
	u32 mecc;

	blockPage =block * cpages_in_block + page;

	NF_RSTECC();
	NF_MECC_UnLock();

	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= 0;
		NFADDR= blockPage & 0xff;
		NFADDR=(blockPage>>8) & 0xff;
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

	/* 
         * 2442 uses 28 bit ECC for both 512 and  2048 sized pages
         */

	mecc = NFMECC0;
	eccbuf[0] = mecc & 0xff;
	eccbuf[1] = (mecc >> 8) & 0xff;
	eccbuf[2] = (mecc >> 16) & 0xff;
	eccbuf[3] = (mecc >> 24) & 0xff;

	// The 4 remaining bits are set to 1.
	eccbuf[3] |= 0x0f; 
	
	NFCMD = NAND_CMD_PAGEPROG;
	for(i=0; i<10 ;i++);
	NF_DETECT_RB();
	NFCMD = NAND_CMD_STATUS;
	for(i=0;i<3;i++); 
	if (NFDATA8 & 0x1) {// Page write error
		NF_nFCE_H();
		printf("[PROGRAM_ERROR:block#=%d]\n",block);
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
	//	NF_SOFT_UnLock();
	//	NF_RSTECC();    // Initialize ECC
	//	NF_MECC_UnLock();
	NF_nFCE_L();
	NF_CLEAR_RB();	/* to make CMD_STATUS have 0 */

	if(cpage2Kis) {
		NFCMD = NAND_CMD_SEQIN;
		NFADDR= 0;
		NFADDR= (1 << 3);
		NFADDR= blockPage & 0xff;
		NFADDR=(blockPage>>8) & 0xff;
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
 * NF_Init : 
 */
static void NF_Init(void)
{
#define FULL_INIT 1

#ifdef FULL_INIT
        NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
        // TACLS                [14:12] CLE&ALE duration = HCLK*TACLS.
        // TWRPH0               [10:8]  TWRPH0 duration = HCLK*(TWRPH0+1)
        // TWRPH1               [6:4]   TWRPH1 duration = HCLK*(TWRPH1+1)
        // AdvFlash(R)  [3]             Advanced NAND, 0:256/512, 1:1024/2048
        // PageSize(R)  [2]             NAND memory page size
        //                                              when [3]==0, 0:256, 1:512 bytes/page.
        //                                              when [3]==1, 0:1024, 1:2048 bytes/page.
        // AddrCycle(R) [1]             NAND flash addr size
        //                                              when [3]==0, 0:3-addr, 1:4-addr.
        //                                              when [3]==1, 0:4-addr, 1:5-addr.
        // BusWidth(R/W) [0]    NAND bus width. 0:8-bit, 1:16-bit.

        NFCONT = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0);
        // Lock-tight   [13]    0:Disable lock, 1:Enable lock.
        // Soft Lock    [12]    0:Disable lock, 1:Enable lock.
        // EnablillegalAcINT[10]        Illegal access interupt control. 0:Disable, 1:Enable
        // EnbRnBINT    [9]             RnB interrupt. 0:Disable, 1:Enable
        // RnB_TrandMode[8]             RnB transition detection config. 0:Low to High, 1:High to Low
        // SpareECCLock [6]             0:Unlock, 1:Lock
        // MainECCLock  [5]             0:Unlock, 1:Lock
        // InitECC(W)   [4]             1:Init ECC decoder/encoder.
        // Reg_nCE              [1]             0:nFCE=0, 1:nFCE=1.
        // NANDC Enable [0]             operating mode. 0:Disable, 1:Enable.
#else
	NFCONF |= 0xFFF0; /* Max TACLS, TWRPH0, TWRPH1 */
	NFCONT |= (1 << 0) ;   /* Activate NAND Controller */
#endif

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
		//NFADDR = 0x0;
	//	NFADDR = 0x0;
		NFADDR = blockPage & 0xff;
		NFADDR = (blockPage>>8) & 0xff;
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
		printf("[ERASE_ERROR:block#=%d]\n",block);
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
	u16 data;

	blockPage = block * cpages_in_block;

	NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_READ0;
		NFADDR = cpage_size & 0xff;
		NFADDR = (cpage_size >>8) & 0x0f;
		NFADDR = blockPage & 0xff;
		NFADDR = (blockPage >> 8)  & 0xff;
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
		printf("[block %d is bad block(%x)]\n",block,data);
	}
}


static int NF_IsBadBlock(u32 block)
{
	int i;
	unsigned int blockPage;
	u8 data;

	blockPage = block * cpages_in_block ; 

	NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	if (cpage2Kis) {
		NFCMD = NAND_CMD_READ0;
		NFADDR = cpage_size & 0xff;
		NFADDR = (cpage_size >> 8) & 0x0f;    // The mark of bad block is in 0 page
		NFADDR = blockPage  & 0xff;   // For block number A[24:17]
		NFADDR = (blockPage >> 8) & 0xff;  // For block number A[25]
	        NFCMD = NAND_CMD_READSTART;
	}
	else {
		NFCMD = NAND_CMD_READOOB;
		NFADDR = 517 & 0xf;
		NFADDR = blockPage & 0xff;    // The mark of bad block is in 0 page
		NFADDR = (blockPage >> 8) & 0xff;   // For block number A[24:17]
		NFADDR = (blockPage >> 16) & 0xff;  // For block number A[25]
	}

	for(i=0;i<10;i++);  /* dummy check me */
	NF_DETECT_RB();

	data=(u8)NFDATA16;

	NF_nFCE_H();

	if(data != 0xff)
	{
		printf("[block %d has been marked as a bad block(%x)]\n",block,data);
		return 1;
	}
	else
	{
		return 0;
	}
}

#endif /* (CONFIG_COMMANDS & CFG_CMD_NAND) */
