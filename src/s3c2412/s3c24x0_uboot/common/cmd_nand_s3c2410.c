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
 *
 *History:
 * 2005-08-17:Jicheng.liao<jicheng.liao@samsung.com>
 *       :added this file according to cmd_nand_s3c2440a.c
 *
 * History :
 * 2005-09-27 : Jack fangdb<dongbiao.fang@samsung.com>
 *       : add file cmd_nand_s3c2410.c
 */ 

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <s3c2410.h>

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

#define BAD_CHECK	(0)
#define ECC_CHECK 	(0)

#ifdef CONFIG_OMAP1510
void archflashwp(void *archdata, int wp);
#endif

#define ROUND_DOWN(value,boundary)      ((value) & (~((boundary)-1)))

/*
 * Definition of the out of band configuration structure
 */
struct nand_oob_config {
	int ecc_pos[6];		/* position of ECC bytes inside oob */
	int badblock_pos;	/* position of bad block flag inside oob -1 = inactive */
	int eccvalid_pos;	/* position of ECC valid flag inside oob -1 = inactive */
} oob_config = { {0}, 0, 0};

#undef	NAND_DEBUG
#undef	PSYCHO_DEBUG

/* ****************** WARNING *********************
 * When ALLOW_ERASE_BAD_DEBUG is non-zero the erase command will
 * erase (or at least attempt to erase) blocks that are marked
 * bad. This can be very handy if you are _sure_ that the block
 * is OK, say because you marked a good block bad to test bad
 * block handling and you are done testing, or if you have
 * accidentally marked blocks bad.
 *
 * Erasing factory marked bad blocks is a _bad_ idea. If the
 * erase succeeds there is no reliable way to find them again,
 * and attempting to program or erase bad blocks can affect
 * the data in _other_ (good) blocks.
 */
#define	 ALLOW_ERASE_BAD_DEBUG 0

#define CONFIG_MTD_NAND_ECC  /* enable ECC */
#define CONFIG_MTD_NAND_ECC_JFFS2

/* bits for nand_rw() `cmd'; or together as needed */
#define NANDRW_READ	0x01
#define NANDRW_WRITE	0x00
#define NANDRW_JFFS2	0x02

#define NAND_PAGES_IN_BLOCK     (32)
#define NAND_BLOCK_SIZE         (512*NAND_PAGES_IN_BLOCK)

u8 NF8_Spare_Data[16];
#define FAIL 0
#define OK 1



/*
 * Function Prototypes
 */

/************************************************
 *
 * Funcitons --- H/W dependent --- getfree 12/05
 *
 ************************************************/

static u8 NF_CheckId(void);
static int NF_EraseBlock(u32 blockNum);
static int NF_ReadPage(u32 block, u32 page, u8 *buffer);
static int NF_WritePage(u32 block, u32 page, u8 *buffer);
static int NF_IsBadBlock(u32 block);
static int NF_MarkBadBlock(u32 block);
static void NF_Reset(void);
static void NF_Init(void);
static int Oneblockread(int block, int page, u32 **dest);

/***  End of H/W dependent Functions ***/

/* 
 * Oneblockread : read page * 32 = one block 
 *
 */

int Oneblockread(int block, int page, u32 **dest)
{
	int i,j;
	u8 buffer[512];   /* buffer size is 512 byte */
	u32 *pbuffer;

	u32 *mem = *dest;

	for(i=0; i<32; i++) {
		pbuffer = (u32 *) buffer;

		NF_ReadPage(block,i,buffer);

		for(j=0;j<(512/4);j++)
		{
			*mem++ = *pbuffer++;
			*dest = *dest + 1;
		}
	}

	return 0;
}	

/* 
 * 
 * K9S1208_read : 
 *
 */

int 
K9S1208_read(uint targetBlock,uint targetPage,uint targetSize, uint srcAddress )
{
	int j = 0;
	u8 id;
	u32 block,page;
	u32 *ptr;

	u32  totalblock = targetSize / 0x4000;

	if ( (targetSize % 0x4000) > 0) totalblock++;

	ptr = ( u32 *) srcAddress;


	printf("\n[SMC(K9S1208V0M) NAND Flash 0x%8X block read]\n",totalblock);	


	NF_Init();
	id=NF_CheckId();
	printf("ID=%x(0x76)\n",id);
	if(id!=0x76) 	return 0;

	block=targetBlock ;
	page=targetPage;

	if (block <= 0 || totalblock <= 0 )
		printf(" Wrong block number 0x%08X \n",(int)block);

	while(1) {
	     Oneblockread (block+j ,0 ,&ptr) ;
		 printf(".");
		 totalblock--;
		 j++;
		 if (totalblock <= 0 ) break;
	};
	
	printf("\n");
	return 0;
}


/* 
 * getfree 12/05 
 * K9S1208_write : 
 *
 */
 
int K9S1208_write(uint targetBlock,uint targetSize, uint srcAddress )
{
	//    unsigned long interrupt_reservoir;

	int i;
	int programError=0;
	u8 *srcPt,*saveSrcPt;
	u32 blockIndex;

	printf("\n[SMC(K9S1208V0M) NAND Flash writing program]\n");

	NF_Init();

	printf("source base address(0x1010000x)=0x%x\n",srcAddress);
	printf("target start block number      =%d\n",targetBlock);
	printf("target size        (0x4000*n)  =0x%x\n",targetSize);

	srcPt=(u8 *)srcAddress;
	blockIndex=targetBlock;
	while(1)
	{
		saveSrcPt=srcPt;	

#if BAD_CHECK       
		if(NF_IsBadBlock(blockIndex))	// 1:bad 0:good
		{
			blockIndex++;   // for next block
			continue;
		}
#endif
		if(!NF_EraseBlock(blockIndex))
		{
			blockIndex++;   // for next block
			printf(" Error->  Erase Block %d  \n",(int)blockIndex);
			continue;
		}


		for(i=0;i<32;i++)
		{
			if(!NF_WritePage(blockIndex,i,srcPt))// block num, page num, buffer
			{
				programError=1;
				break;
			}
#if ECC_CHECK    
			if(!NF_ReadPage(blockIndex,i,srcPt))
			{
				printf("ECC Error(block=%d,page=%d!!!\n",(int)blockIndex,i);
			}
#endif	    
			srcPt+=512;	// Increase buffer addr one pase size
			/* getfree 1209 : print dot each block  */
			printf(".");
			if((u32)srcPt>=(srcAddress+targetSize)) // Check end of buffer
				break;	// Exit for loop
		}
		if(1==(blockIndex%4))
		    printf("\n");//change line after writing one block
		if(programError==1)
		{
			blockIndex++;
			srcPt=saveSrcPt;
			programError=0;
			continue;
		}
		if((u32)srcPt>=(srcAddress+targetSize))
			break;	// Exit while loop
		blockIndex++;
	}

	printf("\n\n");
	return 0;

}

/*
 * getfree 12/17
 * do_nandw : compare with original do_nandw in armboot
 *
 */
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

	K9S1208_write(startblk,size,memadr );

	return 0;
}


/*
 * getfree 12/17
 * do_nandr : compare with original do_nandr in armboot
 *
 */
extern int nandll_read_blocks(unsigned long dst_addr, unsigned long src_addr, int size);


int do_nandr    (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	startblk,startpage = 0,size,memadr;
	if (argc !=  4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	startblk = NAND_BLOCK_SIZE*simple_strtoul(argv[1], NULL, 16);
	size  = simple_strtoul(argv[2], NULL, 16);
	memadr = simple_strtoul(argv[3], NULL, 16);

	NF_Init();
	nandll_read_blocks(memadr,startblk,size);
	return 0;
}

/* getfree added U_BOOT_CMD TABLE 2003.12.05 */

U_BOOT_CMD(
	nandw,	4,	1,	do_nandw,
       "nandw - HEX: targetblock targetsize mem_addr \n",
       "\n	- SMDK2410 NAND Flash Write Program\n"		\
       "nandw targetblock 0~4096, targetsize memory addr \n"	\
);

U_BOOT_CMD(
	 nandr,	4,	1,	do_nandr,
	 "nandr - HEX: targetblock targetsize mem_adr \n",
	 "\n	-SMDK2410 NAND Flash Read Program\n"	\
	 "nandw targetblock 0~4096, targetsize memory addr \n"	\
);

/* End Of U_BOOT_CMD */

/* getfree added below source from armboot/common/cmd_nand.c */

// block0: reserved for boot strap
// block1~4095: used for OS image
// badblock SE: xx xx xx xx xx 00 ....
// good block SE: ECC0 ECC1 ECC2 FF FF FF ....

/* 
 * Following macros are used for SMDK2440
 *
 */

#define NF_MECC_UnLock()         
#define NF_MECC_Lock()         
#define NF_SECC_UnLock()        //(NFCONF &=~(1<<12)) 
#define NF_SECC_Lock()        
#define NF_CMD(cmd)             {NFCMD=cmd;}
#define NF_ADDR(addr)           {NFADDR=addr;} 
#define NF_nFCE_L()             (NFCONF &= ~0x800)
#define NF_nFCE_H()              
#define NF_RSTECC()             (NFCONF|=(1<<12))
#define NF_RDDATA()             (NFDATA8)
#define NF_RDDATA8()            (NFDATA8)
#define NF_WRDATA(data)         {NFDATA8=data;}
#define NF_WRDATA8(data)        {NFDATA8=data;}

// RnB Signal
#define NF_CLEAR_RB()                   
#define NF_DETECT_RB()                  {while(!(NFSTAT&1));}

#define ID_K9S1208V0M   0xec76
#define ID_K9K2G16U0M   0xecca

// HCLK=100Mhz
#define TACLS           7       // 1-clk(0ns) 
#define TWRPH0          7       // 3-clk(25ns)
#define TWRPH1          7       // 1-clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns


static u8 se8Buf[16]={
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff
};

/* 
 * NF_MarkBadBlock : 
 *
 */

static int NF_MarkBadBlock(u32 block)
{
    int i;
    u32 blockPage;
    blockPage=(block<<5);


    se8Buf[0]=0xff;
    se8Buf[1]=0xff;
    se8Buf[2]=0xff;
    //seBuf[3]=0xff;
    se8Buf[5]=0x44;   // Bad blcok mark=0


  //  NF_CLRRnB();
    NF_nFCE_L();


    NF_CMD(0x50);
    NF_CMD(0x80);   // Write 1st command

    NF_ADDR(0x0);                   // The mark of bad block is
    NF_ADDR((blockPage)&0xff);      // marked 5th spare array
    NF_ADDR((blockPage>>8)&0xff);   // in the 1st page.
    NF_ADDR((blockPage>>16)&0xff);  //

    for(i=0;i<16;i++)
    {
        //NF_WRDATA(0,seBuf[i]);        // Write spare array
        NF_WRDATA(se8Buf[i]);    // Write spare array
    }

    NF_CMD(0x10);   // Write 2nd command

    for(i=0;i<10;i++);  //tWB = 100ns. ///???????


    NF_CMD(0x70);

    for(i=0;i<3;i++);  //twhr=60ns////??????

    if (NF_RDDATA()&0x1) // Spare arrray write error
    {
        NF_nFCE_H();
        printf("[Program error is occurred but ignored]\n");
    }
    else
    {
        NF_nFCE_H();
    }

    printf("[block #%d is marked as a bad block]\n",block);
    return 1;
} 

static int NF8_MarkBadBlock(u32 block)
{
	return 0;
}

static int NF_ReadPage(u32 block, u32 page, u8 *buffer){
	unsigned long addr=0;
	addr=block*NAND_BLOCK_SIZE+page*NAND_PAGE_SIZE;
    nandll_read_page(buffer, addr);	
	return 0;
}

static int NF_WritePage(u32 block,u32 page,u8 *buffer)
{
	int i;
	u32 blockPage, Mecc, Secc;
	u8 *bufPt=buffer;

	NF_RSTECC();    // Initialize ECC
	NF_MECC_UnLock();

	blockPage=(block<<5)+page;

        NF_nFCE_L();
        NF_CMD(0x0); 	//??????
        NF_CMD(0x80);   // Write 1st command
        NF_ADDR(0);                         // Column 0
        NF_ADDR(blockPage&0xff);            //
        NF_ADDR((blockPage>>8)&0xff);   // Block & page num.
        NF_ADDR((blockPage>>16)&0xff);  //


        for(i=0;i<512;i++) {
            NF_WRDATA8(*bufPt++);   // Write one page to NFM from buffer
         }

	    NF_MECC_Lock();
        // Get ECC data.
        // Spare data for 8bit
        // byte  0     1    2     3     4          5               6      7            8         9
        // ecc  [0]   [1]  [2]    x     x     [Bad marking]        SECC0  SECC1
        Mecc = NFECC;
        se8Buf[0]=(u8)(Mecc&0xff);
        se8Buf[1]=(u8)((Mecc>>8) & 0xff);
        se8Buf[2]=(u8)((Mecc>>16) & 0xff);
        //se8Buf[3]=(u8)((Mecc>>24) & 0xff);
        se8Buf[5]=0xff;         // Marking good block

        NF_SECC_UnLock();
        //Write extra data(ECC, bad marking)
        for(i=0;i<4;i++) {
                NF_WRDATA8(se8Buf[i]);  // Write spare array(Main ECC)
                NF8_Spare_Data[i]=se8Buf[i];
        }
	    NF_SECC_Lock();
        for(i=4;i<16;i++) {
                NF_WRDATA8(se8Buf[i]);  // Write spare array(Spare ECC and Mark)
                NF8_Spare_Data[i]=se8Buf[i];
        }
        NF_CLEAR_RB();
        NF_CMD(0x10);    // Write 2nd command

	    NF_DETECT_RB();
	
        NF_CMD(0x70);   // Read status command   

        for(i=0;i<3;i++);  //twhr=60ns

	   if (NF_RDDATA()&0x1) {// Page write error
            NF_nFCE_H();
            printf("[PROGRAM_ERROR:block#=%d]\n",block);
            NF8_MarkBadBlock(block);
            return FAIL;
	   } else {
		NF_nFCE_H();
		return OK;
	  }
}



/*
 * NF_CheckId
 *
 */
static u8 NF_CheckId(void)
{
    int i;
    u8 id,id1, id2,id3,id4;

    NF_nFCE_L();

    NF_CMD(0x90);
    NF_ADDR(0x0);

    for(i=0;i<10;i++); //wait tWB(100ns)////?????

//    id=NF_RDDATA()<<8;        // Maker code(K9S1208V:0xec)
//    id|=NF_RDDATA();  // Devide code(K9S1208V:0x76)

    id1=NF_RDDATA();    // Maker code(K9S1208V:0xec)
    id2=NF_RDDATA();    // Devide code(K9S1208V:0x76)
    id3=NF_RDDATA();
    id4=NF_RDDATA();

    NF_nFCE_H();
    return id2;
}

/*
 *
 * NF_Reset :
 *
 */

void NF_Reset(void)
{
    int i;

        NF_nFCE_L();

        NF_CLEAR_RB();
        NF_CMD(0xFF);   //reset command
        for(i=0;i<10;i++);  //tWB = 100ns. //??????
        NF_DETECT_RB();

        NF_nFCE_H();

}

/*
 *
 * NF_Init : 
 *
 */
static void NF_Init(void)
{
    NFCONF |=0xf842;// (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
	NF_Reset();
}

/*
 *
 * Check10 
 *
 */
char Check10(int data, char seq) // Check '10' pattern
{
    if(data & (0x2<< seq*2) )
	return 1;
    else
	return 0;
}

/*
 *
 * NF_EraseBlock
 *
 */
static int NF_EraseBlock(u32 block)
{
	u32 blockPage=(block<<5);
	int i;

#if BAD_CHECK
	if(NF8_IsBadBlock(block))
		return FAIL;
#endif
	NF_nFCE_L();

    NF_CMD(0x60);   // Erase one block 1st command, Block Addr:A9-A25
    // Address 3-cycle
    NF_ADDR(blockPage&0xff);            // Page number=0
    NF_ADDR((blockPage>>8)&0xff);
    NF_ADDR((blockPage>>16)&0xff);

	NF_CLEAR_RB();
	NF_CMD(0xd0);   // Erase one blcok 2nd command
	NF_DETECT_RB();

	NF_CMD(0x70);   // Read status command

	if (NF_RDDATA()&0x1) // Erase error
	{
		NF_nFCE_H();
		printf("[ERASE_ERROR:block#=%d]\n",block);
		//NF8_MarkBadBlock(block);
		return FAIL;
	}
	else
	{
		NF_nFCE_H();
		return OK;
	}
}

/*
 *
 * NF_IsBadBlock :
 *
 */

static int NF_IsBadBlock(u32 block)
{
    int i;
    unsigned int blockPage;
    u8 data;

    blockPage=(block<<5);       // For 2'nd cycle I/O[7:5]

    //  NF_RSTECC();    // Reset ECC
    //NF_CLRRnB();
    NF_nFCE_L();
    NF_CMD(0x50);               // Spare array read command
    NF_ADDR(517&0xf);           // Read the mark of bad block in spare array(M addr=5)
    NF_ADDR(blockPage&0xff);    // The mark of bad block is in 0 page
    NF_ADDR((blockPage>>8)&0xff);   // For block number A[24:17]
    NF_ADDR((blockPage>>16)&0xff);  // For block number A[25]

   for(i=0;i<10;i++);   // wait tWB(100ns) //?????

    //NF_TRANSRnB();      // Wait tR(max 12us)

    data=NF_RDDATA();

    NF_nFCE_H();

    if(data!=0xff)
    {
        printf("[block %d has been marked as a bad block(%x)]\n",block,data);
        return 1;
    }
    else
    {
        return 0;
    }
}
///////////////////////fangdb add 

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


#endif /* (CONFIG_COMMANDS & CFG_CMD_NAND) */
