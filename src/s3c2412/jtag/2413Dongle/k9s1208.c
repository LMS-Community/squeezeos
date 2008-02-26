#include <stdio.h>
#include "pin2413.h"
#include "Jtag.h"
#include "K9S1208.h"
#include "sjf2413.h"
#include "mem_rdwr.h"

#define BAD_CHECK	(1)
#define ECC_CHECK	(0)

//*************** JTAG dependent functions ***************
void K9S1208_JtagInit(void);
static void NF_CMD(U8 cmd);
static void NF_ADDR(U8 addr);
static void NF_nFCE_L(void);
static void NF_nFCE_H(void);
static U8 NF_RDDATA(void);
static void NF_WRDATA(U8 data);
static void NF_WAITRB(void);
//*************** H/W dependent functions ***************
static U16 NF_CheckId(void);
static int NF_EraseBlock(U32 blockNum, U8 force);
static int NF_ReadPage(U32 block,U32 page,U8 *buffer,U8 *spareBuf);
static int NF_WritePage(U32 block,U32 page,U8 *buffer,U8 *spareBuf);
//buffer size is 512 bytes
static int NF_IsBadBlock(U32 block);
static int NF_MarkBadBlock(U32 block);
static void NF_Reset(void);
static void NF_Init(void);
//*******************************************************

void K9S1208_PrintBlock(void);
void K9S1208_Program(void);
void K9S1208_FindFailed(void);
void K9S1208_TestBlock(void);
void K9S1208_MarkBlockFailed(void);
void K9S1208_MarkBlockOk(void);


static U32 targetBlock;	    // Block number (0 ~ 4095)
static U32 targetSize;	    // Total byte size 
static U8 blockBuf[0x4000];

static void *function[][2]=
{
    (void *)K9S1208_Program,		"K9S1208 Program     ",
    (void *)K9S1208_PrintBlock,		"K9S1208 Read block  ",
	(void *)K9S1208_FindFailed,		"K9S1208 Find failed blocks",
	(void *)K9S1208_TestBlock,		"K9S1208 Test block",
	(void *)K9S1208_MarkBlockFailed,"K9S1208 Mark block failed ",
	(void *)K9S1208_MarkBlockOk,	"K9S1208 Mark block ok ",
    (void *)1,			    	    "Exit                ",
    0,0
};


void K9S1208_Menu(void)
{
    int i;
    U16 id;

    printf("\n[K9S1208 NAND Flash JTAG Programmer]\n");
    K9S1208_JtagInit();
    NF_Init();

    id=NF_CheckId();
    
	if(id!=0xec76)
    {
	printf("ERROR: K9S1208 is not detected. Detected ID=0x%x.\n",id);
	return;
    }
    else
    {
    	printf("K9S1208 is detected. ID=0x%x\n",id);
    }

    while(1)
    {

	i=0;
    	while(1)
	{   //display menu
	    printf("%2d:%s",i,function[i][1]);
	    i++;
	    if((int)(function[i][0])==0)
	    {
		printf("\n");
		break;
	    }
	    if((i%3)==0)
		printf("\n");
	}

	printf("Select the function to test :");
	scanf("%d",&i);
	if( i>=0 && (i<((sizeof(function)/8)-2)) ) 
	    ( (void (*)(void)) (function[i][0]) )();  
	else
	    break; //Exit menu
    }
}



void K9S1208_Program(void)
{
    int i;
    int programError=0;
    U32 blockIndex;
    int noLoad=0;
    U8 spareBuf[16]=
	{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    U8 *srcPt;
    U32 progSize=0;

    printf("\n[SMC(K9S1208V0M) NAND Flash Writing Program]\n");
    
    printf("\nSource size:0h~%xh\n",imageSize-1);
    printf("\nAvailable target block number: 0~4095\n");
    printf("Input target block number:");
    scanf("%d",&targetBlock);
    targetSize=((imageSize+0x4000-1)/0x4000)*0x4000;
    printf("target start block number     =%d\n",targetBlock);
    printf("target size        (0x4000*n) =0x%x\n",targetSize);
    printf("STATUS:");
    blockIndex=targetBlock;
    while(1)
    {
	if(noLoad==0)
	{
	    LoadImageFile(blockBuf,0x4000);
	}
	noLoad=0;
	
#if BAD_CHECK       
	if(NF_IsBadBlock(blockIndex) && blockIndex!=0 )	// 1:bad 0:good
        {
	    blockIndex++;   // for next block
	    noLoad=1;
	    continue;
	}
#endif
	if(!NF_EraseBlock(blockIndex, 0))
	{
	    blockIndex++;   // for next block
	    noLoad=1;
	    continue;
	}

	printf("E");
	srcPt=blockBuf;

	for(i=0;i<32;i++)
	{
	    if(!NF_WritePage(blockIndex,i,srcPt,NULL/*spareBuf*/))// block num, page num, buffer
	    {
	        programError=1;
	        break;
	    }

	    srcPt+=512;	// Increase buffer addr one pase size
	    printf("[P]");
	}
	printf("\n");

        if(programError==1)
	{
	    blockIndex++;
	    noLoad=1;
	    programError=0;
	    continue;
	}
	progSize+=0x4000;
	if(progSize>=imageSize)
	    break;	// Exit while loop
	blockIndex++;
    }
}




void K9S1208_PrintBlock(void)// Printf one page
{
    int i;
    U16 id;
    U32 block,page;
    U8	buffer[512+16];

    printf("\n[SMC(K9S1208) NAND Flash block read]\n");	
    
    NF_Init();
    id=NF_CheckId();
    printf("ID=%x(0xec76)\n",id);
    if(id!=0xec76)
	return;

    printf("Input target block number:");
    scanf("%d",&block);
    printf("Input target page number:");   
    scanf("%d",&page);
    
    NF_ReadPage(block,page,buffer,buffer+512);
    
    printf("block=%d,page=%d:",block,page);
    for(i=0;i<512;i++)
    {
        if(i%16==0)
	    printf("\n%3xh:",i);
        printf("%02x ",buffer[i]);
    }
    printf("\nS.A.:",i);

    for(i=512;i<512+16;i++)
    {
        printf("%02x ",buffer[i]);
    }

    printf("\n");    	
}


void K9S1208_FindFailed(void) {
	U16 id;
	U32 block;

	printf("\n[SMC(K9S1208) NAND Flash finding failed blocks]\n");	

	NF_Init();
    id=NF_CheckId();
    printf("ID=%x(0xec76)\n",id);
    if(id!=0xec76)
	return;

	for (block=0; block<4096; block++) {
		if(NF_IsBadBlock(block) && block!=0 ) {	// 1:bad 0:good
			printf("Bad block %d\n", block);
		}
	}
	printf("\n");
}


void K9S1208_MarkBlockFailed(void) {
    int i;
    U16 id;
    U32 block;
    U8	buffer[512+16];

    printf("\n[SMC(K9S1208) NAND Flash mark block failed]\n");	
    
    NF_Init();
    id=NF_CheckId();
    printf("ID=%x(0xec76)\n",id);
    if(id!=0xec76)
	return;

    printf("Input target block number:");
    scanf("%d",&block);

	if (block == 0) {
		printf("CANNOT MARK BLOCK 0 AS BAD\n");
		return;
	}

	NF_MarkBadBlock(block);
}


void K9S1208_MarkBlockOk(void) {
    int i;
    U16 id;
    U32 block,page;
    U8	buffer[32 * 512];
	U8  spare[32 * 16];

    printf("\n[SMC(K9S1208) NAND Flash mark block ok]\n");	
    
    NF_Init();
    id=NF_CheckId();
    printf("ID=%x(0xec76)\n",id);
    if(id!=0xec76)
	return;

    printf("Input target block number:");
    scanf("%d",&block);

	if(!NF_IsBadBlock(block, 0)) {
		printf("Block is already ok\n");
		return;
	}


	for (page=0; page<32; page++) {
		printf("R");
	    NF_ReadPage(block, page, buffer + (page * 512), spare + (page * 16));
	}
	printf("\n");

	printf("Erasing block %d\n", block);
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}

	for (page=0; page<32; page++) {
		printf("W", block, page);
	    NF_ReadPage(block, page, buffer + (page * 512), NULL);
	}
	printf("\n");

}


void K9S1208_TestBlock(void) {
    int i;
    U16 id;
    U32 block,page;
    U8	buffer[32 * 512];
	U8  spare[32 * 16];
	U8  data[512];
	U32 failed = 0;

    printf("\n[SMC(K9S1208) NAND Flash verify block]\n");	
    
    NF_Init();
    id=NF_CheckId();
    printf("ID=%x(0xec76)\n",id);
    if(id!=0xec76)
	return;

    printf("Input target block number:");
    scanf("%d",&block);


	printf("Saving block contents\n");
	for (page=0; page<32; page++) {
		printf("R");
	    NF_ReadPage(block, page, buffer + (page * 512), spare + (page * 16));
	}
	printf("\n");

	printf("Testing 0x00\n");
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}
	memset(data, 0x00, 512);
	for (page=0; page<32; page++) {
	    NF_WritePage(block, page, data, NULL);
		printf("W");
	}
	for (page=0; page<32; page++) {
	    NF_ReadPage(block, page, data, NULL);
		for (i=0; i<sizeof(data); i++) {
			if (data[i] != 0x00) {
				failed++;
				printf("byte failed block %d page %d byte %d\n", block, page, i);
			}
		}
		printf("R");
	}
	printf("\n");

	printf("Testing 0xFF\n");
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}
	memset(data, 0xFF, 512);
	for (page=0; page<32; page++) {
	    NF_WritePage(block, page, data, NULL);
		printf("W");
	}
	for (page=0; page<32; page++) {
	    NF_ReadPage(block, page, data, NULL);
		for (i=0; i<sizeof(data); i++) {
			if (data[i] != 0xFF) {
				failed++;
				printf("byte failed block %d page %d byte %d\n", block, page, i);
			}
		}
		printf("R");
	}
	printf("\n");

	printf("Testing 0xAA\n");
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}
	memset(data, 0xAA, 512);
	for (page=0; page<32; page++) {
	    NF_WritePage(block, page, data, NULL);
		printf("W");
	}
	for (page=0; page<32; page++) {
	    NF_ReadPage(block, page, data, NULL);
		for (i=0; i<sizeof(data); i++) {
			if (data[i] != 0xAA) {
				failed++;
				printf("byte failed block %d page %d byte %d\n", block, page, i);
			}
		}
		printf("R");
	}
	printf("\n");

	printf("Testing 0x55\n");
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}
	memset(data, 0x55, 512);
	for (page=0; page<32; page++) {
	    NF_WritePage(block, page, data, NULL);
		printf("W");
	}
	for (page=0; page<32; page++) {
	    NF_ReadPage(block, page, data, NULL);
		for (i=0; i<sizeof(data); i++) {
			if (data[i] != 0x55) {
				failed++;
				printf("byte failed block %d page %d byte %d\n", block, page, i);
			}
		}
		printf("R");
	}
	printf("\n");

	printf("Writing block contents\n");
	if (!NF_EraseBlock(block, 1)) {
		printf("FAILED to erase block %d.\n", block);
		return;
	}
	for (page=0; page<32; page++) {
		printf("W");
	    NF_WritePage(block, page, buffer + (page * 512), NULL);
	}
	printf("\n");

	if (failed) {
		printf("Total failures %d\n", failed);
		NF_MarkBadBlock(block);
	}
}


//*************************************************
//*************************************************
//**           H/W dependent functions           **
//************************************************* 
//*************************************************

// NAND Flash Memory Commands
#define	SEQ_DATA_INPUT		(0x80)
#define	READ_ID				(0x90)
#define	RESET				(0xFF)
#define	READ_1_1			(0x00)
#define	READ_1_2			(0x01)
#define	READ_2				(0x50)
#define	PAGE_PROGRAM		(0x10)
#define	BLOCK_ERASE			(0x60)
#define	BLOCK_ERASE_CONFIRM	(0xD0)
#define	READ_STATUS			(0x70)


// block0: reserved for boot strap
// block1~4095: used for OS image
// badblock SE: xx xx xx xx xx 00 ....
// good block SE: ECC0 ECC1 ECC2 FF FF FF ....

#define WRITEVERIFY  (0)  //verifing is enable at writing.

#define ID_K9S1208V0M	0xec76

static U8 seBuf[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

// 1block=(512+16)bytes x 32pages
// 4096block

// A[23:14][13:9]
//  block   page

static int NF_EraseBlock(U32 block, U8 force)
{
    U32 blockPage=(block<<5);

#if BAD_CHECK
    if(!force && NF_IsBadBlock(block) && block!=0) //block #0 can't be bad block for NAND boot
	return 0;
#endif

    NF_nFCE_L();
    
    NF_CMD(0x60);   // Erase one block 1st command

    NF_ADDR(blockPage&0xff);	    // Page number=0
    NF_ADDR((blockPage>>8)&0xff);   
    NF_ADDR((blockPage>>16)&0xff);

    NF_CMD(0xd0);   // Erase one blcok 2nd command
    
    Delay(1); //wait tWB(100ns)

    NF_WAITRB();    // Wait tBERS max 3ms.
    NF_CMD(0x70);   // Read status command

    if (NF_RDDATA()&0x1) // Erase error
    {	
    	NF_nFCE_H();
	printf("[ERASE_ERROR:block#=%d]\n",block);
	NF_MarkBadBlock(block);
	return 0;
    }
    else 
    {
    	NF_nFCE_H();
        return 1;
    }
}


static int NF_IsBadBlock(U32 block)
{
    unsigned int blockPage;
    U8 data;
    
    
    blockPage=(block<<5);	// For 2'nd cycle I/O[7:5] 
    
    NF_nFCE_L();    
    NF_CMD(0x50);		// Spare array read command
    NF_ADDR(517&0xf);		// Read the mark of bad block in spare array(M addr=5) 
    NF_ADDR(blockPage&0xff);	// The mark of bad block is in 0 page
    NF_ADDR((blockPage>>8)&0xff);   // For block number A[24:17]
    NF_ADDR((blockPage>>16)&0xff);  // For block number A[25]

    Delay(1);		// wait tWB(100ns)
    
    NF_WAITRB();	// Wait tR(max 12us)
    
    data=NF_RDDATA();

    NF_nFCE_H();    

    if(data!=0xff)
    {
    	printf("[block %d:bad block(%x)]\n",block,data);
    	return 1;
    }
    else
    {
	printf(".");
    	return 0;
    }
}


static int NF_MarkBadBlock(U32 block)
{
    int i;
    U32 blockPage=(block<<5);
 
    seBuf[0]=0xff;
    seBuf[1]=0xff;    
    seBuf[2]=0xff;    
    seBuf[5]=0x44;   // Bad blcok mark=0
    
    NF_nFCE_L(); 
    NF_CMD(0x50);   
    NF_CMD(0x80);   // Write 1st command
    
    NF_ADDR(0x0);		    // The mark of bad block is 
    NF_ADDR(blockPage&0xff);	    // marked 5th spare array 
    NF_ADDR((blockPage>>8)&0xff);   // in the 1st page.
    NF_ADDR((blockPage>>16)&0xff);  
    
    for(i=0;i<16;i++)
    {
	NF_WRDATA(seBuf[i]);	// Write spare array
    }

    NF_CMD(0x10);   // Write 2nd command
    
    Delay(1);  //tWB = 100ns. 

    NF_WAITRB();      // Wait tPROG(200~500us)
  
    NF_CMD(0x70);
    
    Delay(1);	 //twhr=60ns//
    
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


static int NF_ReadPage(U32 block,U32 page,U8 *buffer,U8 *spareBuf)
{
    int i;
    unsigned int blockPage;
    U8 *bufPt=buffer;
    
    page=page&0x1f;
    blockPage=(block<<5)+page;
        
    NF_nFCE_L();    
    NF_CMD(0x00);   // Read command
    NF_ADDR(0);	    // Column = 0
    NF_ADDR(blockPage&0xff);	    //
    NF_ADDR((blockPage>>8)&0xff);   // Block & Page num.
    NF_ADDR((blockPage>>16)&0xff);  //

    Delay(1);	    //wait tWB(100ns)/////??????
    
    NF_WAITRB();    // Wait tR(max 12us)
    
    for(i=0;i<(512);i++)
    {
    	*bufPt++=NF_RDDATA();	// Read one page
    }

    if(spareBuf!=NULL)
    {
	for(i=0;i<16;i++)
	    spareBuf[i]=NF_RDDATA();	// Read spare array
    }

    NF_nFCE_H();    

    return 1;
}


static int NF_WritePage(U32 block,U32 page,U8 *buffer,U8 *spareBuf)
{
    int i;
    U32 blockPage=(block<<5)+page;
    U8 *bufPt=buffer;

    NF_nFCE_L(); 
    NF_CMD(0x0);
    NF_CMD(0x80);		    // Write 1st command
    NF_ADDR(0);			    // Column 0
    NF_ADDR(blockPage&0xff);	    //
    NF_ADDR((blockPage>>8)&0xff);   // Block & page num.
    NF_ADDR((blockPage>>16)&0xff);  //

    for(i=0;i<512;i++)
    {
	NF_WRDATA(*bufPt++);	// Write one page to NFM from buffer
    }  

    if(spareBuf!=NULL)
    {
	for(i=0;i<16;i++)
	{
	    NF_WRDATA(spareBuf[i]);	// Write spare array(ECC and Mark)
	}
    }

    NF_CMD(0x10);   // Write 2nd command
    
    Delay(1);	    //tWB = 100ns. 

    NF_WAITRB();    //wait tPROG 200~500us;
 
    NF_CMD(0x70);   // Read status command   
    
    Delay(1);	    //twhr=60ns
    
    if (NF_RDDATA()&0x1) // Page write error
    {	
    	NF_nFCE_H();
	printf("[PROGRAM_ERROR:block#=%d]\n",block);
	NF_MarkBadBlock(block);
	return 0;
    }
    else 
    {
    	NF_nFCE_H();
    #if (WRITEVERIFY==1)
	//return NF_VerifyPage(block,page,pPage);	
    #else
	return 1;
    #endif
    }
}



static U16 NF_CheckId(void)
{
    U16 id;
    
    NF_nFCE_L();
    
    NF_CMD(0x90);
    NF_ADDR(0x0);
    
    Delay(1);	//wait tWB(100ns)
    
    id=NF_RDDATA()<<8;	// Maker code(K9S1208V:0xec)
    id|=NF_RDDATA();	// Devide code(K9S1208V:0x76)
    
    NF_nFCE_H();
    
    return id;
}


static void NF_Reset(void)
{
    NF_nFCE_L();

    NF_CMD(0xFF);   //reset command

    Delay(1);	    //tWB = 100ns. 

    NF_WAITRB();    //wait 200~500us;
     
    NF_nFCE_H();    
}


static void NF_Init(void)
{
    NF_Reset();

    //NF_nFCE_L();
    NF_CMD(READ_1_1);        
    //NF_nFCE_H();
}


//*************************************************
//*************************************************
//**     JTAG dependent primitive functions      **
//************************************************* 
//*************************************************
void K9S1208_JtagInit(void)
{
    JTAG_RunTestldleState();
    JTAG_ShiftIRState(EXTEST);

	//Added to SJF2440 
	S2413_SetPin(CLE_CON,LOW);
    S2413_SetPin(ALE_CON,LOW);
	S2413_SetPin(GCSn3_CON,LOW); 
    S2413_SetPin(FRE_CON,LOW); 
    S2413_SetPin(FWE_CON,LOW); 
	
	S2413_SetPin(CLE,LOW); 
    S2413_SetPin(ALE,LOW); 
}


static void NF_CMD(U8 cmd)
{   
    
    //Command Latch Cycle
    S2413_SetPin(DATA0_7_CON ,LOW); //D[7:0]=output


	S2413_SetPin(GCSn3,LOW); 
    S2413_SetPin(FRE,HIGH); 
    S2413_SetPin(FWE,LOW); //Because tCLS=0, CLE & nFWE can be changed simultaneously.
    
	S2413_SetPin(ALE,LOW); 
    S2413_SetPin(CLE,HIGH); 
    
	S2413_SetDataByte(cmd);
    JTAG_ShiftDRStateNoTdo(outCellValue); 

    S2413_SetPin(FWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); 

#if 1
    S2413_SetPin(CLE,LOW);	
    S2413_SetPin(DATA0_7_CON,HIGH); //D[7:0]=input

    JTAG_ShiftDRStateNoTdo(outCellValue); 
#endif
}


static void NF_ADDR(U8 addr)
{
    //rNFADDR=addr;
    S2413_SetPin(DATA0_7_CON ,LOW); //D[7:0]=output

    S2413_SetPin(GCSn3,LOW); 
    S2413_SetPin(FRE,HIGH); 
    S2413_SetPin(FWE,LOW);
    S2413_SetPin(ALE,HIGH);
    S2413_SetPin(CLE,LOW);
    S2413_SetDataByte(addr);
    JTAG_ShiftDRStateNoTdo(outCellValue); 
    
    S2413_SetPin(FWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); 
    
#if 1
    S2413_SetPin(ALE,LOW);	
    S2413_SetPin(DATA0_7_CON,HIGH); //D[7:0]=input

    JTAG_ShiftDRStateNoTdo(outCellValue); 
#endif
}


static void NF_nFCE_L(void)
{
    S2413_SetPin(GCSn3_CON,LOW);
	S2413_SetPin(GCSn3,LOW); 
    JTAG_ShiftDRStateNoTdo(outCellValue); 
}


static void NF_nFCE_H(void)
{
    S2413_SetPin(GCSn3,HIGH); 
    JTAG_ShiftDRStateNoTdo(outCellValue); 
}


static U8 NF_RDDATA(void)
{
    S2413_SetPin(DATA0_7_CON,HIGH); //D[7:0]=input

    S2413_SetPin(FRE,LOW);
    JTAG_ShiftDRStateNoTdo(outCellValue); 
    
    S2413_SetPin(FRE,HIGH);
    JTAG_ShiftDRState(outCellValue,inCellValue); 
    return S2413_GetDataByte();
}

static void NF_WRDATA(U8 data)
{   
    S2413_SetPin(DATA0_7_CON ,LOW); //D[7:0]=output

    S2413_SetPin(FWE,LOW);
    S2413_SetDataByte(data);
    JTAG_ShiftDRStateNoTdo(outCellValue); 
    
    S2413_SetPin(FWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); 
}


static void NF_WAITRB(void)
{
    char c;

	while(1)
    {
	JTAG_ShiftDRState(outCellValue,inCellValue); 
	
	if( S2413_GetPin(RNB)==HIGH)
	break;
	}
}

