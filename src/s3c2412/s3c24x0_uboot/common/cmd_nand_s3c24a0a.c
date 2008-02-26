/*
 * (C) 2004 Samsung Electronics
 *        Young-jun Jang <yj03.jang@samsung.com>
 *        - add/modify nandw,nandr, nande, nandv
 *        - bad block management (bbcheck, bbmark)
 *        - yaffs image r/w (nandyw, nandyr)
 * (C) 2004 Samsung Electronics 
 *        SW.LEE <hitchcar@samsung.com>
 *        - add  nande, nandE
 *        - support 16Bit / 3 Addr cycle nand
 * (C) 2003 Samsung Electronics
 *        - getfree
 *
 * Driver for NAND support, Rick Bronson
 * borrowed heavily from:
 * (c) 1999 Machine Vision Holdings, Inc.
 * (c) 1999, 2000 David Woodhouse <dwmw2@infradead.org>
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/s3c24a0.h>
#include <configs/smdk24a0nand.h>
#include <linux/mtd/nand.h>

#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NAND)

#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ids.h>
#include <jffs2/jffs2.h>

#define BAD_CHECK	(1)
#define ECC_CHECK 	(0)

#define ROUND_DOWN(value,boundary)      ((value) & (~((boundary)-1)))


static u8 NF_CheckId(void);
static int NF_EraseBlock(u32 blockNum);
//static int NF_ReadPage(u32 block, u32 page, u8 *buffer);
static int NF_WritePage(u32 block, u32 page, u8 *buffer);
static int NF_WriteOob(u32 block, u32 page, u8 *buffer, int yaffs_option);
static int NF_IsBadBlock(u32 block);
static int NF_MarkBadBlock(u32 block, int mark_flag);
static void NF_Reset(void);
static void NF_Init(void);

#ifdef OREIAS
#define adjust_bank(x) {\
	if(x>=0x11e00000) \
		x=x+0x6200000;\
}
#endif


int s3c24x0_nand_read(uint blockIndex, int size, uint destAddress, int flag )
{
	char *buf = (char *)destAddress;
	char page_buf[NAND_PAGE_SIZE];
	char oob_buf[NAND_OOB_SIZE];
	int i, j;
	u32 src_addr = blockIndex * NAND_BLOCK_SIZE;

	NF_Reset();

	printf( "block readed : " );

	while (size > 0) {
		if ( flag != NF_RW_YAFFS ) {
			/* If this block is bad, go next block */
			if (is_bad_block(src_addr)) {
				src_addr += NAND_BLOCK_SIZE;
				continue;
			}
		}

		printf( "%X ", (src_addr>>14) );

		/* Read block */
		for (i = 0; i < NAND_PAGES_IN_BLOCK; i++) {
			nandll_read_page((char *)page_buf, src_addr);
			for (j = 0; j < NAND_PAGE_SIZE; j++) {
				*buf++ = page_buf[j];
			}
			size -= NAND_PAGE_SIZE;
			if (size <= 0) break;

			if ( flag == NF_RW_YAFFS ) {
				/* read oob */
				nandll_read_oob((char *)oob_buf, src_addr);
				for (j = 0; j < NAND_OOB_SIZE; j++) {
					*buf++ = oob_buf[j];
				}
				size -= NAND_OOB_SIZE;
				if (size <= 0) break;
			}

			src_addr += NAND_PAGE_SIZE;
		}

		//printf( "%X ", (src_addr>>14) );
	}
	printf( "\n" );

	return 0;
}

/*
 * s3c24x0_nand_write
 *
 * yaffs_option : only used when flag is yaffs mode.
 */
int s3c24x0_nand_write(uint targetBlock,uint targetSize, uint srcAddress, int flag, int yaffs_option )
{
	int i;
	int programError=0;
	u8 *srcPt,*saveSrcPt;
	u32 blockIndex;

	printf("NAND Flash Write\n");
	printf("Source base address        = 0x%x\n",srcAddress);
	printf("Target start block number  =  %d\n",targetBlock);
	printf("Target size    (0x4000*n)  =0x%x\n",targetSize);
	printf("block writed :\n");

	srcPt=(u8 *)srcAddress;
	blockIndex=targetBlock;
	while(1) {
		saveSrcPt=srcPt;	
#if BAD_CHECK       
		if(NF_IsBadBlock(blockIndex)){
			blockIndex++;   // for next block
			continue;
		}
#endif
		if(!NF_EraseBlock(blockIndex)) {
			blockIndex++;   // for next block
			printf(" Error->  Erase Block %d  \n",(int)blockIndex);
			continue;
		}

		for(i=0; i < NAND_PAGES_IN_BLOCK ; i++) {
			if(!NF_WritePage(blockIndex,i,srcPt)){
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
			if((u32)srcPt>=(srcAddress+targetSize)) { // Check end of buffer
				break;	// Exit for loop
			}
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

int do_nandE    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int startblk, size, eraseblocks, i;

	if ( argc != 3 ){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	printf("StartBlock %d (0x%x) : Size %d (0x%x) \n",startblk,startblk,size, size);
	eraseblocks = size / NAND_BLOCK_SIZE;
	printf("Total Erase Blocks %d (0x%x) \n",eraseblocks, eraseblocks);
	udelay(10000);
	NF_Init();
	NF_Reset();
	for ( i=0 ; i < eraseblocks;  i++) {
		NF_EraseBlock(startblk);
		startblk++;
	}
	return 0;
}
                                                                                                      
int do_nandw (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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
	s3c24x0_nand_write( startblk, size, memadr, NF_RW_NORMAL, 0 );

	return 0;
}


int do_nandr    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong  startblk, size, memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);

	NF_Init();
	NF_Reset();
	s3c24x0_nand_read( startblk, size, memadr, NF_RW_NORMAL );
	return 0;
}


int do_nande    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	ulong blk_start, blk_end;

	if (argc != 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	blk_start = simple_strtoul( argv[1], NULL, 16 );
	blk_end   = simple_strtoul( argv[2], NULL, 16 );

	for (i=blk_start; i<=blk_end; i++) {
		if ( !NF_EraseBlock(i) ) {
			printf( "nand flash erase error : at block %x\n", i );
		}
	}

	printf( "nand flash erase complete\n" );

	return 0;
}

int do_bbmark    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong blk;

	if (argc != 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	blk = simple_strtoul( argv[1], NULL, 16 );

	if (blk<=0 || blk>=4096) {
		printf( "out of block range\n" );
		return 1;
	}

	if ( !NF_EraseBlock(blk) ) {
		printf( "erase error\n" );
	}

	if ( strcmp(argv[2], "on") == 0 ) {
		NF_MarkBadBlock(blk, NF_BB_ON);
	} else {
		NF_MarkBadBlock(blk, NF_BB_OFF);
	}

	return 0;
}

int do_bbcheck    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i, bb_cnt;

	printf( "Bad block list : \n" );
	for (bb_cnt=0, i=0; i<4096; i++) {
		if ( NF_IsBadBlock(i) ) {
			bb_cnt++;
		}
	}

	if ( bb_cnt > 0 ) {
		printf( "\n Total bad block count : %d \n", bb_cnt );
	} else {
		printf( "No bad block\n" );
	}

	return 0;
}

/* show page data */
int do_nandv    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int blk, page, i, j;
	u8 oob_buf[16];
	u8 page_buf[512];

	if (argc != 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	blk  = simple_strtoul( argv[1], NULL, 16 );
	page = simple_strtoul( argv[2], NULL, 10 );

	if (blk<0 || blk>=4096) {
		printf( "out of block range\n" );
		return 1;
	}
	if (page<0 || page>=32) {
		printf( "out of page range\n" );
		return 1;
	}

	nandll_read_page((u8*)page_buf, (blk*NAND_BLOCK_SIZE)+(page*NAND_PAGE_SIZE));
	for (i=0; i<32; i++)
	{
		printf( "%03X : ", i*16 );
		for (j=0; j<16; j++)
		{
			printf( "%02X", page_buf[i*16+j] );
			if ( j%4 == 3 ) printf( " " );
		}
		printf( "\n" );
	}

	nandll_read_oob((u8*)oob_buf, (blk*NAND_BLOCK_SIZE) + (page*NAND_PAGE_SIZE));
	printf( "oob : " );
	for (j=0; j<16; j++)
	{
		printf( "%02X", oob_buf[j] );
		if ( j%4 == 3 ) printf( " " );
	}
	printf( "\n" );

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
	ecc_option = (argv[4][0] == 'y') ? NF_USE_ECC : NF_USE_MTD_ECC;

	// yaffs image don't use 1st block
	startblk++;

	NF_Init();
	NF_Reset();
	s3c24x0_nand_write(startblk, size, memadr, NF_RW_YAFFS, ecc_option );

	return 0;
}

int do_nandyr    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong   startblk, size, memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);

	// yaffs image don't use 1st block
	startblk++;

	NF_Init();
	NF_Reset();
	s3c24x0_nand_read(startblk, size, memadr, NF_RW_YAFFS );
	return 0;
}

/* getfree added U_BOOT_CMD TABLE 2003.12.05 */

U_BOOT_CMD(
    nandw,  4,  1,  do_nandw,
       "nandw - HEX: targetblock targetsize mem_addr \n",
       "\n  - SMDK24A0 NAND Flash Write Command\n"      \
       "nandw targetblock 0~0xfff(4095), targetsize memory addr \n" \
);

U_BOOT_CMD(
     nandr, 4,  1,  do_nandr,
     "nandr - HEX: targetblock targetsize mem_adr \n",
     "\n    -SMDK24A0 NAND Flash Read Command\n"    \
     "nandw targetblock 0~0xfff(4095), targetsize memory addr \n"   \
);

/* add command (nande, bbmark, bbcheck) by jyj. (2004.8.10) */

U_BOOT_CMD(
    nande,  3,  1,  do_nande,
       "nande - [block_start] [block_end] \n",
       "\n  - SMDK24A0 NAND Flash Erase Command"        \
       "\n         block range : 0 ~ 0xfff(4095) as hex value\n"    \
);

U_BOOT_CMD(
    bbmark, 3,  1,  do_bbmark,
       "bbmark [block] [on/off] \n",
       "\n  - SMDK24A0 NAND Flash Mark Bad Block Command"       \
       "\n         block range : 0 ~ 0xfff(4095) as hex value\n"    \
       "\n         on : mark bad block  /  off : unmark bad block \n"   \
);

U_BOOT_CMD(
    bbcheck,    3,  1,  do_bbcheck,
       "bbcheck \n",
       "\n  - SMDK24A0 NAND Flash Show Bad Block Command"       \
);

U_BOOT_CMD(
    nandv,  3,  1,  do_nandv,
       "nandv [block] [page] \n",
       "\n  - SMDK24A0 NAND Flash Show Block Data Command"      \
       "\n         block range : 0 ~ 0xfff(4095) as hex value\n"    \
       "\n         page  range : 0 ~ 31\n"  \
);

U_BOOT_CMD(
    nandyw, 5,  1,  do_nandyw,
       "nandyw - HEX: targetblock targetsize mem_addr ecc_option(y/m)\n",
       "\n  - SMDK24A0 NAND Flash Write Command for Yaffs\n"        \
       "nandyw targetblock 0~0xfff(4095), targetsize memory addr \n"    \
       "       ecc_option : [y] : use yaffs ecc / [m] : use mtd ecc  \n"  \
);

U_BOOT_CMD(
    nandyr, 4,  1,  do_nandyr,
       "nandyr - HEX: targetblock targetsize mem_addr \n",
       "\n  - SMDK24A0 NAND Flash Read Command for Yaffs\n"     \
       "nandyr targetblock 0~0xfff(4095), targetsize memory addr \n"    \
);

U_BOOT_CMD(
        nandE,  4,      1,      do_nandE,
       "nandE  startBlock size \n",
       "       - delete all blocks in given range regardless Bad Blcok\n"               \
);


#define NF_nFCE_L()     {NFCONT&=~(1<<7);}
#define NF_nFCE_H()     {NFCONT|=(1<<7);}
#define NF_RSTECC()     {NFCONT|=(1<<8);}
//#define NF_TRANSRnB()    {while(!(NFSTAT&(1<<13)));}
#define NF_TRANSRnB()    {while(!(NFSTAT&(1<<10)));}
#define NF_CLRRnB()    {(NFSTAT|=(1<<13));}
#define NF_MECC_Lock()  { NFCONT|=(1<<9);}
#define NF_MECC_UnLock()  { NFCONT&=~(1<<9);}


static u8 seBuf[16]=
{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

/* 
 * NF_MarkBadBlock : 
 */

static int NF_MarkBadBlock(u32 block, int mark_flag)
{
	int i;
	u32 blockPage;
#ifdef S3C24X0_16BIT_NAND
	u16 *Buf = (unsigned short *) seBuf;
#endif

	blockPage=(block<<5);

#ifdef S3C24X0_16BIT_NAND
	seBuf[0]=0x00;
	seBuf[1]=0x00;
	seBuf[2]=0xff;
	seBuf[3]=0xff;
	seBuf[5]=0xff; 

#else
	seBuf[0]=0xff;
	seBuf[1]=0xff;
	seBuf[2]=0xff;
	seBuf[5]=0x44;   // 0xff is good block, else is bad block 
    if ( mark_flag == NF_BB_OFF ) seBuf[5] = 0xff;
#endif
	//  NF_CLRRnB();
	NF_nFCE_L();

	NFCMD = NAND_CMD_READOOB;
	NFCMD = NAND_CMD_SEQIN;   // Write 1st command

	NFADDR =0x0;                   // The mark of bad block is
	NFADDR=(blockPage)&0xff;      // marked 5th spare array
	NFADDR=(blockPage>>8)&0xff;   // in the 1st page.
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
	NF_TRANSRnB();   
	NFCMD=NAND_CMD_STATUS;
	for(i=0;i<3;i++);  
	if (NFDATA8&0x1) // Spare arrray write error
	{
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
	u32 blockPage = (block * NAND_PAGES_IN_BLOCK) + page;
#ifdef S3C24X0_16BIT_NAND
	u16 *Buf = (unsigned short) seBuf;
	u16 *ptr16=buffer;
#else
	u8 *ptr8 = buffer;
#endif

	// Port setting for SMC_WP
	GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x1<<2);
	GPDAT |=(1<<1);

	NF_RSTECC();    // Initialize ECC
	NF_MECC_UnLock();
	NF_CLRRnB();
	NF_nFCE_L();

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
		u8 *tptr;
		tptr = ptr8++;
#ifdef OREIAS
		adjust_bank(tptr);
#endif
	//	NFDATA8 = *ptr8++;
		NFDATA8 = *tptr;
	}
#endif
	NF_MECC_Lock();

#if ECC_CHECK  
	/* To Do */

	//ECC generation from main area ECC status.
	seBuf[10]=NFMECC0&0xff;
	seBuf[11]=(NFMECC0>>8)&0xff;
	seBuf[12]=(NFMECC0>>16)&0xff;
	seBuf[5]=0xff; 

	for(i=0 ; i < NAND_OOB_SIZE / 2; i++)
	{
		NFDATA16 = Buf[i];
	}
#endif

	NFCMD = NAND_CMD_PAGEPROG;
	for(i=0; i<10 ;i++);
	NF_TRANSRnB();
	NFCMD = NAND_CMD_STATUS;
	for(i=0; i<3; i++);
	if (NFDATA8 & 0x1) { /* Page Error */
		NF_nFCE_H();
		printf("[PROGRAM_ERROR:block#=%d]\n",block);
		NF_MarkBadBlock(block, NF_BB_ON);
		GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x3<<2);
		return 0;
	}
	else {
		NF_nFCE_H();
		GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x3<<2);
		return 1;
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
		u8 *tptr;
		tptr = ptr8++;
#ifdef OREIAS
		adjust_bank(tptr);
#endif
		oobBuf[i] = (*tptr);
	}
	if ( yaffs_option == NF_USE_MTD_ECC )
	{
		oobBuf[ 8] = 0xFF; oobBuf[ 9] = 0xFF; oobBuf[10] = 0xFF;
		oobBuf[13] = 0xFF; oobBuf[14] = 0xFF; oobBuf[15] = 0xFF;
	}
#endif

	NF_nFCE_L();

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
	NF_TRANSRnB();
	NFCMD = NAND_CMD_STATUS;
	for(i=0; i<3; i++);
	if (NFDATA8 & 0x1) { /* Page Error */
		NF_nFCE_H();
		printf("[Program error is occurred but ignored]\n");
	}
	else {
		NF_nFCE_H();
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


static void NF_Reset(void)
{
        int i;

        NF_CLRRnB();
        NF_nFCE_L();
        NFCMD = NAND_CMD_RESET;

        for(i=0;i<80;i++);
        NF_TRANSRnB();    
        NF_nFCE_H();
}


#define TECH              0x3f
#define TACLS           7//0  //1clk(0ns)
#define TWRPH0          7
#define TWRPH1          7//0  //1clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns

static void NF_Init(void)
{
#ifdef S3C24X0_16BIT_NAND
	NFCONF=(0<<22)|(TECH<<16)|(TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<3)|(1<<2)|(1<<1)|(0<<0);
#else

	NFCONF |= (TECH<<16)|(TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
#endif

	NFCONT |= 3; /* Enter into Software mode */

//	printf("NF_Init NFCONF 0x%08X  NFCONT 0x%08X\n", NFCONF, NFCONT);

}

/*
 * NF_EraseBlock
 */
static int NF_EraseBlock(u32 block)
{
	u32 blockPage;
	int i;

	// Port setting for SMC_WP
	GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x1<<2);
	GPDAT |=(1<<1);
	NFCONT &= ~(1<<2); /* Unlock */
	blockPage = block * NAND_PAGES_IN_BLOCK;
	NF_CLRRnB();
	NF_nFCE_L();
	NFCMD = NAND_CMD_ERASE1;
	NFADDR = blockPage&0xff; 
	NFADDR = (blockPage>>8)&0xff;
#ifdef  NAND_3_ADDR_CYCLE
#else
	NFADDR = (blockPage>>16)&0xff;
#endif

	NFCMD = NAND_CMD_ERASE2; 
	for(i=0;i<40;i++); 
	NF_TRANSRnB(); 
	NFCMD = NAND_CMD_STATUS;

	if (NFDATA8 & 0x1) // Erase error
	{
		NF_nFCE_H();
		printf("[ERASE_ERROR:block#=%d]\n",block);
		NF_MarkBadBlock(block,NF_BB_ON);
		GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x3<<2);
		return 0;
	}
	else {
		NF_nFCE_H();
		GPCON_L =  (GPCON_L & ~(0x3<<2))|(0x3<<2);
		return 1;
	}
}


static int NF_IsBadBlock(u32 block)
{
	int i;
	int blockPage;
#ifdef S3C24X0_16BIT_NAND
	u16 data;
#else
	u8 data;
#endif
	blockPage= block * NAND_PAGES_IN_BLOCK; 
	NF_CLRRnB();
	NF_nFCE_L();
	NFCMD = NAND_CMD_READOOB; 

#ifdef S3C24X0_16BIT_NAND
	NFADDR = 512 & 0x7; 		/* X16 A0~A2 are valid */
#else
	NFADDR = 517 & 0xf;          // Read the mark of bad block in spare array(M addr=5)
#endif
	NFADDR = blockPage&0xff;    // The mark of bad block is in 0 page
	NFADDR = (blockPage>>8)&0xff;   // For block number A[24:17]
#ifdef NAND_3_ADDR_CYCLE
#else
	NFADDR =(blockPage>>16)&0xff;  // For block number A[25]
#endif
	for(i=0;i<10;i++);  /* dummy check me */ 

	NF_TRANSRnB();  

#ifdef S3C24X0_16BIT_NAND
	data=NFDATA16;
#else
	data = NFDATA8;
#endif
	NF_nFCE_H();

#ifdef S3C24X0_16BIT_NAND
	if(data != 0xffff)
#else
		if(data != 0xff)
#endif
		{
			printf("[block 0x%x has been marked as a bad block(%x)]\n",block,data);
			return 1;
		}
		else
		{
			return 0;
		}
}

#endif /* (CONFIG_COMMANDS & CFG_CMD_NAND) */
