/****************************************************************
 NAME: am29f800.c
 DESC: am29lv800 Flash Programming code through 2413 JTAG 
 HISTORY: 
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "def.h"

#include "pin2413.h"
#include "jtag.h"
#include "sjf2413.h"

#include "mem_rdwr.h"

#define	FAST_AMD_PROGRAM 	 1
#define TARGET_ADDR_29LV800      0x0  // nGCS0, 128MB area

static int AM29F800_ProgFlash(U32 realAddr,U16 data);
static void AM29F800_EraseSector(int targetAddr);
static int AM29F800_CheckId(void);
static int BlankCheck(int targetAddr,int targetSize);
static int _WAIT(void);

static void InputTargetOffset(void);
static void GetSectorSize(void);


// Because KS32C41000_A1 is connected to AM29LV800_A0,
// the addr parameter has to be a WORD address, so called in AMD specification.

#if  FAST_STRATA_PROGRAM 
#define _WR(addr,data)	    MRW_Wr16QQ(addr,data)
#define _RD(addr)	    MRW_Rd16Q(addr)
#else 
#define _WR(addr,data)	    MRW_Wr16(addr,data)
#define _RD(addr)	    MRW_Rd16(addr)
#endif

#define _RESET()	    _WR(0x0,0xf0f0)
#define BADDR2WADDR(addr)   (addr>>1)  
    //If you want to access a real address, use BADDR2WADDR macro.
    //For example, MRW_Wr16Q(BADDR2WADDR(realAddr),data,0,0x3)....

static U32 srcAddress;
static U32 srcOffset;

static U32 targetAddress;
static U32 targetOffset;
static U32 targetSize;

static U32 sectorOffset;
static U32 sectorSize;
static U32 totalSize;

void ProgramAM29F800(void)
{
    int i;

    printf("\n[AM29F800 Writing Program]\n");
    printf("NOTE: AM29LV800BB needs 4 step sequences for 1 half-word data.\n");
    printf("      So,the program time is twice of Starata flash(2 step sequences).\n");

    MRW_JtagInit();

    printf("[Check AM29LV800]\n");
    if(!AM29F800_CheckId())
    {
	printf("ID Check Error!!!\n");
	return;
    }

    targetSize=imageSize;

    srcAddress=(U32)malloc(targetSize+4);
    if(srcAddress==0)return;
    LoadImageFile((U8 *)srcAddress,targetSize);

    srcOffset=0;
    
    targetAddress=TARGET_ADDR_29LV800;
    InputTargetOffset();
    
    totalSize=0;
    sectorOffset=targetOffset;

    printf("\n");

    while(totalSize<imageSize)
    {
        GetSectorSize(); //sectorSize is updated.

	printf("Erase the sector:0x%x.\n",targetAddress+sectorOffset);
    
	AM29F800_EraseSector(targetAddress+sectorOffset);

	printf("Start of the sector data writing.\n");

	for(i=0x0;i<sectorSize;i+=2) 
	{
	    AM29F800_ProgFlash(targetAddress+sectorOffset+i,*( (U16 *)(srcAddress+srcOffset+i) )  );
	    if((i%0x100)==0)printf("%x ",i);
	    totalSize+=2;
	    if(totalSize>=targetSize)break;
	}
	
	printf("\nEnd of the sector data writing!!!\n\n");

	_RESET();
	sectorOffset+=sectorSize;
	srcOffset+=sectorSize;
    }
/*
    printf("\nVerifying Start.\n");
    for(i=0x0;i<targetSize;i+=2) 
    {
	if(*( (U16 *)(i+targetAddress+targetOffset) )!=*( (U16 *)(srcAddress+i) )  )
	{    
	    printf("%x=verify error\n",i+targetAddress+targetOffset);
	    return;
	}
	if((i%0x1000)==0)printf("%x ",i);
    }
    printf("\nVerifying End!!!\n");
*/
}


static void InputTargetOffset(void)
{
    printf("\nImage Size:0h~%xh\n",targetSize);
    printf("\nAvailable Target Offset:\n"); 
    printf("    0x0, 0x4000, 0x6000, 0x8000,0x10000,0x20000,0x30000,0x40000,\n");
    printf("0x50000,0x60000,0x70000,0x80000,0x90000,0xa0000,0xb0000,0xc0000,\n");
    printf("0xd0000,0xe0000,0xf0000\n");

    printf("Input target offset:");
    scanf("%x",&targetOffset);
}

static void GetSectorSize(void)
{
    if(sectorOffset<0x4000){sectorSize=0x4000;}
    else if(sectorOffset<0x6000)sectorSize=0x2000;
    else if(sectorOffset<0x8000)sectorSize=0x2000;
    else if(sectorOffset<0x10000)sectorSize=0x8000; 
    else sectorSize=0x10000;

    printf("SectorOffset=0x%x\n",sectorOffset);
    printf("SectorSize  =0x%x\n",sectorSize);
}




static int AM29F800_CheckId(void)
{
    U16 manId,devId;

    _RESET();
    
    _WR(0x555,0xaaaa);
    _WR(0x2aa,0x5555);
    _WR(0x555,0x9090);
    manId=_RD(0x0);

    _RESET(); // New 5V AM29F800 needs this command. 
    _WR(0x555,0xaaaa);
    _WR(0x2aa,0x5555);
    _WR(0x555,0x9090);
    devId=_RD(0x1);

    _RESET();   

    printf("Manufacture ID=%4x(0x0001), Device ID(0x225B)=%4x\n",manId,devId);

    if(manId==0x0001 && devId==0x225b)return 1;
    else return 0;
}




void AM29F800_EraseSector(int targetAddr)
{
        printf("Sector Erase is started!\n");

        _RESET();

        _WR(0x555,0xaaaa);
        _WR(0x2aa,0x5555);
        _WR(0x555,0x8080);
        _WR(0x555,0xaaaa);
        _WR(0x2aa,0x5555);
        _WR(BADDR2WADDR(targetAddr),0x3030);
        
	_WAIT(); 
        
	_RESET();
}


int _WAIT(void) //Check if the bit6 toggle ends.
{
        volatile U16 flashStatus,old;

	old=_RD(BADDR2WADDR(0x0));

        while(1)
	{
	    flashStatus=_RD(BADDR2WADDR(0x0));
            if( (old&0x40) == (flashStatus&0x40) )break;
            if( flashStatus&0x20 )
	    {
		//printf("[DQ5=1:%x]\n",flashStatus);
		old=_RD(BADDR2WADDR(0x0));
		flashStatus=_RD(BADDR2WADDR(0x0));
		if( (old&0x40) == (flashStatus&0x40) )
		    return 0;
		else return 1;
	    }
	    //printf(".");
	    old=flashStatus;
        }
        //printf("!\n");
	return 1;
}




int AM29F800_ProgFlash(U32 realAddr,U16 data)
{
        _WR(0x555,0xaaaa);
        _WR(0x2aa,0x5555);
        _WR(0x555,0xa0a0);

        _WR(BADDR2WADDR(realAddr),data);
	//return _WAIT(); //not needed at JTAG access
	return 1;
}
