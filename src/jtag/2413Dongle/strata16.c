/*************************************************************************************
 NAME: strata16.c
 DESC: Strata Flash Programming code through 2413 JTAG 
 *************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "def.h"

#include "pin2413.h"
#include "Jtag.h"
#include "K9S1208.h"
#include "sjf2413.h"

#include "mem_rdwr.h"

//I have not tested yet about fast programming. But, it will reduce the programming time.
//IF the problem is occurred, let FAST_ROM_PROGRAM  0.
#define	FAST_STRATA_PROGRAM 	 1

#define TARGET_ADDR_28F128      0x08000000  // nGCS1, 128MB area

static U32 srcAddr;

static U32 targetOffset; 
static U32 targetAddress; 
static U32 targetSize; 

static int error_erase=0;       // Read Status Register, SR.5
static int error_program=0;     // Read Status Register, SR.4


static int  Strata_ProgFlash(U32 realAddr,U32 data);
static void Strata_EraseSector(int targetAddr);
static int  Strata_CheckID(int targetAddr);
static int  Strata_CheckDevice(int targetAddr);
static int  Strata_CheckBlockLock(int targetAddr);
static int Strata_ClearBlockLock(int targetAddr); 
static int  Strata_BlankCheck(int targetAddr,int targetSize);
static int  _WAIT(void);

#if FAST_STRATA_PROGRAM	
    #define _WR(addr,data)  MRW_Wr16QQ(addr,data) //  omitting nGCS deassertion. 
	#define _RD(addr)       MRW_Rd16Q(addr)   
	#define _RESET()	 MRW_Wr16Q(targetAddress,0x00ff)
    
#else
    #define _WR(addr,data)  MRW_Wr16(addr,data) 
	#define _RD(addr)       MRW_Rd16(addr)   
	#define _RESET()	 MRW_Wr16(targetAddress,0x00ff)
#endif

void Program28F256K3(void)
{
	U32 i;
    U16 temp;

    MRW_JtagInit();

    targetAddress=TARGET_ADDR_28F128;
    targetSize=imageSize;

    printf("\nSource size = %xh\n",targetSize);
    printf("\nAvailable Target Offset Address: \n"); 
    printf("0x0,0x20000,0x40000, ..., 0x1ce0000\n");
    printf("Input target address offset [0x?] : ");
    scanf("%x",&targetOffset);

    printf("Target base address(0x08000000) = 0x%x\n",targetAddress);
    printf("Target offset      (0x0)        = 0x%x\n",targetOffset);
    printf("Target size        (0x20000*n)  = 0x%x\n",targetSize);

    if ( (Strata_CheckID(targetAddress) & 0xffff) != 0x0089 )       // ID number = 0x0089
    {
        printf("Identification check error !!\n");
		return ;
    }

    if ( (Strata_CheckDevice(targetAddress) & 0xffff) != 0x8803 )   // Device number=0x0018
    {
		
        printf("Device check error !!\n");
		return ;
    }


    printf("\nErase the sector from 0x%x.\n", targetAddress+targetOffset);

 
	 for(i=0;i<targetSize;i+=0x10000)
		 {
		 Strata_ClearBlockLock(targetAddress+targetOffset+i); 
	 	Strata_EraseSector(targetAddress+targetOffset+i);
		 }
#if 0 
    if(!Strata_BlankCheck(targetAddress+targetOffset,targetSize))
    {
        printf("Blank Check Error!!!\n");
        return;
    }
#else
	printf("\nBlank Check skip\n");
#endif

    printf("\nStart of the data writing...\n");

    srcAddr=(U32)malloc(targetSize+4);
    if(srcAddr==0)return;
    
    LoadImageFile((U8 *)srcAddr,targetSize);

    for (i=0; i<targetSize; i+=2) 
    {
        Strata_ProgFlash(i+targetAddress+targetOffset, *((U16 *)(srcAddr+i)));
        if(i%0x100==0xfc)
            printf("[%x]",(i+4));
    }
    printf("\nEnd of the data writing \n");

    _RESET();

#if 1
    printf("Verifying Start...\n");
    for (i=0; i<targetSize; i+=2) 
    {
	temp=_RD(i+targetAddress+targetOffset);
        if (temp != *((U16 *)(srcAddr+i))) 
        {
            printf("Verify error: src %08x = %08x\n", srcAddr+i, *((U16 *)(srcAddr+i)));
            printf("         dst %08x = %08x\n", i+targetAddress+targetOffset, temp);
            //return;
        }
    }
    printf("Verifying End!!!");
#else
	printf("verifying skip\n");
#endif

}

//==========================================================================================
int Strata_CheckID(int targetAddr) 
{
    _RESET();
    _WR(targetAddr,0x0090); 
	printf("Identification=%x\n",_RD(targetAddr));
    return _RD(targetAddr); // Read Identifier Code, including lower, higher 16-bit, 8MB, Intel Strate Flash ROM
                            // targetAddress must be the beginning location of a Block Address
}

//==========================================================================================
int Strata_CheckDevice(int targetAddr) 
{
//    _RESET();
    _WR(targetAddr, 0x0090);
	printf("DeviceID=%x\n",_RD(targetAddr+0x2));
    return _RD(targetAddr+0x2); // Read Device Code, including lower, higher 16-bit, 8MB, Intel Strate Flash ROM
                                // targetAddress must be the beginning location of a Block Address
}

//==========================================================================================
int Strata_CheckBlockLock(int targetAddr) 
{
    //_RESET();
    _WR(targetAddr, 0x0090);
    return _RD(targetAddr+0x8); 
    // Read Block Lock configuration, 
    // targetAddress must be the beginning location of a Block Address
}


//==========================================================================================
static int Strata_ClearBlockLock(int targetAddr) 
{
    U32 status,ReadStatus;
	unsigned long bSR7,bSR1;
    //_RESET();
    _WR(targetAddr, 0x0060);
    _WR(targetAddr, 0x00d0);

	_WR(targetAddr, 0x0090);
	status=_RD(targetAddr+0x4); 
	bSR7=status & (1<<7);
	bSR1=status & (1<<1);
	
	while(bSR1) 
	{
	_WR(targetAddr, 0x0090);
	status=_RD(targetAddr+0x4); 
	bSR1=status & (1<<1);
	if(!bSR1)break;
	}

	_WR(targetAddr, 0x0070);	// Read Status Register
	ReadStatus=_RD(targetAddr);	// realAddr is any valid address within the device
	bSR7=ReadStatus & (1<<7);
	while(!bSR7 ) 
	{
	_WR(targetAddr, 0x0070);		  // Read Status Register
	ReadStatus=_RD(targetAddr);
	bSR7=ReadStatus & (1<<7);
	}
    _RESET();
}

//==========================================================================================
void Strata_EraseSector(int targetAddress) 
{
    unsigned long ReadStatus;
    unsigned long bSR5;     // Erase and Clear Lock-bits Status, lower 16bit, 8MB Intel Strate Flash ROM
    unsigned long bSR7;     // Write State Machine Status, lower 16bit, 8MB Intel Strate Flash ROM

    //_RESET();
    _WR(targetAddress, 0x0020); // Block Erase, First Bus Cycle, targetAddress is the address withint the block
    _WR(targetAddress, 0x00d0); // Block Erase, Second Bus Cycle, targetAddress is the address withint the block
    
    //_RESET();
    _WR(targetAddress, 0x0070); // Read Status Register, First Bus Cycle, targetAddress is any valid address within the device
    ReadStatus=_RD(targetAddress);  // Read Status Register, Second Bus Cycle, targetAddress is any valid address within the device
    bSR7=ReadStatus & (1<<7);       // lower 16-bit 8MB Strata
    while(!bSR7) 
    {
        _WR(targetAddress, 0x0070);
        ReadStatus=_RD(targetAddress);
        bSR7=ReadStatus & (1<<7);
    }

    _WR(targetAddress, 0x0070); // When the block erase is complete, status register bit SR.5 should be checked. 
                    // If a block erase error is detected, the status register should be cleared before
                    // system software attempts correct actions.
    ReadStatus=_RD(targetAddress);  
    bSR5=ReadStatus & (1<<5);           // lower 16-bit 8MB Strata 
    if (bSR5==0) 
    {
        printf("Block_%x Erase O.K. \n",targetAddress);
    } 
    else 
    {
        //printf("Error in Block Erasure!!\n");
        _WR(targetAddress, 0x0050); // Clear Status Register
        error_erase=1;                  // But not major, is it casual ?
    }

    _RESET();   // write 0xffh(_RESET()) after the last opoeration to reset the device to read array mode.
}

int Strata_BlankCheck(int targetAddr,int targetSize) 
{
    int i,j;

	printf("\nblank check start........\n");
    for (i=0; i<targetSize; i+=2) 
    {
        j=_RD(i+targetAddr);
        if (j!=0xffff)      // In erasure it changes all block dta to 0xff
        {
            printf("E : %x = %x\n", (i+targetAddr), j);
            return 0;
        }
		if(i%0x100==0xfc)
            printf(".",(i+4));
		//printf("E : %x = %x\n", (i+targetAddr), j);
    }
	printf("\nblank check end..........\n");
    return 1;
}

//==========================================================================================
int Strata_ProgFlash(U32 realAddr,U16 data) 
	{
		//volatile U16 *ptargetAddr;
		unsigned int ReadStatus, status;
		//unsigned int bSR7,bSR1,bSR4;	  // Write State Machine Status, 8MB Intel Strate Flash ROM
		unsigned int bSR7;
		//ptargetAddr = (volatile U16 *)realAddr;


		_WR(realAddr, 0x0040);	// realAddr is any valid adress within the device
		//*ptargetAddr=data;			// 16 bit data
	   _WR(realAddr, data);
		
		_WR(realAddr, 0x0070);	// Read Status Register
		ReadStatus=_RD(realAddr);	// realAddr is any valid address within the device
		bSR7=ReadStatus & (1<<7);

		while(!bSR7 ) 
		{
			_WR(realAddr, 0x0070);		  // Read Status Register
			ReadStatus=_RD(realAddr);
			bSR7=ReadStatus & (1<<7);
		}
/*
		
		_WR(realAddr, 0x0070); 
		ReadStatus=_RD(realAddr);			  // Real Status Register
	
		if(ReadStatus&(1<<3))
		{
		printf("Voltage Range Error\n");
			_WR(realAddr, 0x0050);			// Clear Status Register	
		return 0;
		}	
		if(ReadStatus&(1<<1))
		{
		printf("Device Protect Error\n");
			_WR(realAddr, 0x0050);			// Clear Status Register	
		return 0;
		}	
		if(ReadStatus&(1<<4))
		{
		printf("Programming Error\n");
			_WR(realAddr, 0x0050);			// Clear Status Register	
		return 0;
		}	
*/	
	}

