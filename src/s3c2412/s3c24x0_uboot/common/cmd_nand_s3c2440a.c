/*
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

#define BAD_CHECK	(0)
#define ECC_CHECK 	(0)

#define ROUND_DOWN(value,boundary)      ((value) & (~((boundary)-1)))

u8 NF8_Spare_Data[16];
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


int s3c24x0_nand_write(uint targetBlock,uint targetSize, uint srcAddress, int flag, int yaffs_option )
{
	int i;
	int programError=0;
	u8 *srcPt,*saveSrcPt;
	u32 blockIndex;

	printf("NAND Flash writing\n");
	printf("Source base address      =0x%x\n",srcAddress);
	printf("Target start block number=%d\n",targetBlock);
	printf("Target size  (0x4000*n)  =0x%x\n",targetSize);
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

		for(i=0; i< NAND_PAGES_IN_BLOCK ;i++) {
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
			srcPt += NAND_PAGE_SIZE;
			if ( flag == NF_RW_YAFFS ) {
				if (!NF_WriteOob(blockIndex,i,srcPt, yaffs_option)) {
					programError = 1;
					break;
				}
				srcPt += NAND_OOB_SIZE;
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
	eraseblocks = size / NAND_BLOCK_SIZE;
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
       	eraseblocks = size / NAND_BLOCK_SIZE;
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
	ecc_option = (argv[4][0] == 'y') ? NF_USE_YAFFS_ECC : NF_USE_MTD_ECC;

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

	startblk = NAND_BLOCK_SIZE*simple_strtoul(argv[1], NULL, 16);
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
       "nandw targetblock 0~4096, targetsize memory addr \n"	\
);

U_BOOT_CMD(
    nandyw, 5,  1,  do_nandyw,
       "nandyw - HEX: targetblock targetsize mem_addr ecc_option(y/m)\n",
       "\n  - SMDK24X0 NAND Flash Write Command for Yaffs\n"        \
       "nandyw targetblock 0~0xfff(4095), targetsize memory addr \n"    \
       "       ecc_option : [y] : use yaffs ecc / [m] : use mtd ecc  \n"  \
);

U_BOOT_CMD(
	nande,	4,	1,	do_nande,
       "nande  startBlock size \n",
       "	- delete blocks while skipping bad block \n"		\
);

U_BOOT_CMD(
	nandE,	4,	1,	do_nandE,
       "nandE  startBlock size \n",
       "       - delete all blocks in given range regardless Bad Blcok\n"		\
);

U_BOOT_CMD(
	 nandr,	4,	1,	do_nandr,
	 "nandr - HEX: targetblock targetsize mem_adr \n",
	 "\n	-SMDK24X0 NAND Flash Read Program\n"	\
	 "nandw targetblock 0~4096, targetsize memory addr \n"	\
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


static u8 se8Buf[16]={
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff
};

static u32 se16Buf[32/2]={
        0xffffffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffffff,0xffffffff,0xffffffff,0xffffffff,
        0xffffffff,0xffffffff,0xffffffff,0xffffffff
};



static u8 seBuf[16]=
{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};


static int NF_MarkBadBlock(u32 block, int mark_flag)
{
	int i;
	u32 blockPage = block * NAND_PAGES_IN_BLOCK;
#ifdef S3C24X0_16BIT_NAND
	u16 *Buf = (unsigned short *) seBuf;
#endif

#ifdef S3C24X0_16BIT_NAND

	seBuf[0]=0x00;
	seBuf[1]=0x00;
	seBuf[2]=0xff;
	seBuf[3]=0xff;
	seBuf[5]=0xff;   // Bad blcok mark=0
#else
	seBuf[0]=0xff;
	seBuf[1]=0xff;
	seBuf[2]=0xff;
	//seBuf[3]=0xff;
	seBuf[5]=0x44;   // Bad blcok mark=0	
	if ( mark_flag == NF_BB_OFF ) seBuf[5] = 0xff;
#endif
	//  NF_CLRRnB();
	NF_nFCE_L();

	NFCMD = NAND_CMD_READOOB;
	NFCMD = NAND_CMD_SEQIN;

	NFADDR = 0;
	NFADDR = (blockPage)&0xff;
	NFADDR =(blockPage>>8)&0xff;
#ifdef NAND_3_ADDR_CYCLE
#else
	NFADDR=(blockPage>>16)&0xff;
#endif
#ifdef S3C24X0_16BIT_NAND
	for(i=0 ;i < NAND_OOB_SIZE / 2 ;i++){
		NFDATA16 = Buf[i];
	}

#else
	for(i=0 ;i < NAND_OOB_SIZE ;i++){
		NFDATA8 = seBuf[i];
	}
#endif

	NFCMD= NAND_CMD_PAGEPROG;
	for(i=0;i<10;i++); 
	NF_DETECT_RB();

	NFCMD=NAND_CMD_STATUS;
	for(i=0;i<3;i++); 

	if (NFDATA8 & 0x1) {
		NF_nFCE_H();
		printf("[Program error is occurred but ignored]\n");
	}
	else
	{
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
#ifdef S3C24X0_16BIT_NAND
	u16 *Buf = (unsigned short) seBuf;
	u16 *ptr16= (u16 *)buffer;
#else
	u8 *ptr8 = buffer;
#endif

	blockPage=block * NAND_PAGES_IN_BLOCK + page;
	
//	NF_SOFT_UnLock();
//	NF_RSTECC();    // Initialize ECC
//	NF_MECC_UnLock();
	NF_nFCE_L();
	NF_CLEAR_RB();

        NFCMD = NAND_CMD_READ0;
        NFCMD = NAND_CMD_SEQIN;
        NFADDR= 0;
        NFADDR= blockPage & 0xff;
        NFADDR= (blockPage>>8) & 0xff;
	
#ifdef NAND_3_ADDR_CYCLE
        /* Nothing */
#else
        NFADDR=((blockPage>>16)&0xff);
#endif
#ifdef S3C24X0_16BIT_NAND
	for(i=0; i < NAND_PAGE_SIZE/2 ; i++){
		NFDATA16 = *ptr16++;
	}
#else
	for(i=0; i < NAND_PAGE_SIZE ; i++) {
		NFDATA8 = *ptr8++;
	}
#endif

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
	int i;
	u32 blockPage = (block * NAND_PAGES_IN_BLOCK) + page;
#ifdef S3C24X0_16BIT_NAND
	u16 *ptr16=buffer;
#else
	u8 *ptr8 = buffer;
	u8 oobBuf[NAND_OOB_SIZE];

	for (i=0; i<NAND_OOB_SIZE; i++)
	{
		oobBuf[i] = (*ptr8++);
	}
	if ( yaffs_option == NF_USE_MTD_ECC )
	{
		oobBuf[ 8] = 0xFF; oobBuf[ 9] = 0xFF; oobBuf[10] = 0xFF;
		oobBuf[13] = 0xFF; oobBuf[14] = 0xFF; oobBuf[15] = 0xFF;
	}
#endif
	//	NF_SOFT_UnLock();
	//	NF_RSTECC();    // Initialize ECC
	//	NF_MECC_UnLock();
	NF_nFCE_L();

	NF_CLEAR_RB();	/* to make CMD_STATUS have 0 */

	NFCMD = NAND_CMD_READOOB;
	NFCMD = NAND_CMD_SEQIN;
	NFADDR= 0;
	NFADDR= blockPage & 0xff;
	NFADDR= (blockPage>>8) & 0xff;
#ifdef NAND_3_ADDR_CYCLE
	/* Nothing */
#else
	NFADDR=((blockPage>>16)&0xff);
#endif

#ifdef S3C24X0_16BIT_NAND
	for(i=0; i < NAND_OOB_SIZE/2 ; i++){
		NFDATA16 = *ptr16++;
	}
#else
	for(i=0; i < NAND_OOB_SIZE ; i++) {
		NFDATA8 = oobBuf[i];
	}
#endif

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
	u32 blockPage = block * NAND_PAGES_IN_BLOCK;
	int i;
	NF_SOFT_UnLock();
	
	NF_nFCE_L();
	NF_CLEAR_RB();

	NFCMD = NAND_CMD_ERASE1;
	NFADDR = blockPage&0xff;
	NFADDR = (blockPage>>8)&0xff;
#ifdef  NAND_3_ADDR_CYCLE
#else
	NFADDR = (blockPage>>16)&0xff;
#endif

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

	blockPage = block * NAND_PAGES_IN_BLOCK;

	NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	NFCMD = NAND_CMD_READOOB;

#ifdef S3C24X0_16BIT_NAND
	NFADDR = ( NAND_PAGE_SIZE + 0) & 0x7;             /* X16 A0~A2 are valid */
#else
	NFADDR = ( NAND_PAGE_SIZE + 0) & 0xf;
#endif

	NFADDR = blockPage&0xff;    // The mark of bad block is in 0 page
	NFADDR = (blockPage>>8)&0xff;   // For block number A[24:17]
#ifdef NAND_3_ADDR_CYCLE
#else
	NFADDR =(blockPage>>16)&0xff;  // For block number A[25]
#endif
	for(i=0;i<10;i++);  /* dummy check me */
	NF_DETECT_RB();

	for (i = 0 ; i < NAND_PAGE_SIZE /2 ;i++){
		data=NFDATA16;
		if ( data != 0xffff ) break;
	}
	NF_nFCE_H();
	if (i != (NAND_PAGE_SIZE/2)) {
		printf("[block %d is bad block(%x)]\n",block,data);
	}
}


static int NF_IsBadBlock(u32 block)
{
	int i;
	unsigned int blockPage;
	u16 data;

	blockPage = block * NAND_PAGES_IN_BLOCK;

	NF_RSTECC();
	NF_nFCE_L();
	NF_CLEAR_RB();

	NFCMD = NAND_CMD_READOOB;

#ifdef S3C24X0_16BIT_NAND
	NFADDR = 512 & 0x7;             /* X16 A0~A2 are valid */
#else
	NFADDR = 517 & 0xf;
#endif

	NFADDR = blockPage&0xff;    // The mark of bad block is in 0 page
	NFADDR = (blockPage>>8)&0xff;   // For block number A[24:17]
#ifdef NAND_3_ADDR_CYCLE
#else
	NFADDR =(blockPage>>16)&0xff;  // For block number A[25]
#endif
	for(i=0;i<10;i++);  /* dummy check me */
	NF_DETECT_RB();

	data=NFDATA16;

	NF_nFCE_H();

#ifdef S3C24X0_16BIT_NAND
	if(data != 0xffff)
#else
		if(data != 0xff)
#endif
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
