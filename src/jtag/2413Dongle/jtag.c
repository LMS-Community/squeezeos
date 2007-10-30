
/*****************************************************************************/
/*	    [[ JTAG PIN assignment	]]                                   */
/*---------------------------------------------------------------------------*/
/* JTAG Pin          Parallel Port Pin                                       */
/*---------------------------------------------------------------------------*/
/*   TCK---------------->DATA0   (2)                                         */
/*   TDI---------------->DATA1   (2)                                         */
/*   TMS---------------->DATA2   (2)                                         */
/*   TDO---------------->STATUS7 (11)                                        */
/*****************************************************************************/  


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "def.h"
#include "jtag.h"
#include "ppt.h"

//No delay
//#define JTAG_DELAY()		    //delay=0 
#define JTAG_DELAY()  Delay(1)	    //delay=delayLoopCount

void JTAG_Reset(void);
void JTAG_RunTestldleState( void );


void JTAG_RunTestldleState( void )
{
	JTAG_Reset();

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY(); // Why 3 times?
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status

}


void  JTAG_ShiftIRState(char *wrIR)
{
	int size;
	int i;
	int tdi;

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	// Select-DR-Scan 
	
	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Select-IR-Scan 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Capture-IR 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-IR 

	size=strlen(wrIR);
	
	for( i=0;i<(size-1);i++)
	{
	    tdi= (wrIR[i] ==HIGH) ? TDI_H:TDI_L;
	    JTAG_SET(tdi|TMS_L|TCK_L);JTAG_DELAY();
	    JTAG_SET(tdi|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-IR 
	}
	tdi=(wrIR[i] ==HIGH) ? TDI_H:TDI_L; //i=3
	JTAG_SET(tdi|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(tdi|TMS_H|TCK_H);JTAG_DELAY(); 	//Exit1-IR

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Update-IR

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Run-Test/Idle
}


void  JTAG_ShiftDRState(char *wrDR, char *rdDR)
{
	int size;
	int i;
	int tdi;

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Select-DR-Scan 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Capture-DR 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-DR 

	size=strlen(wrDR);

	for(i=0;i<(size-1);i++)
	{
	    tdi=(wrDR[i]==HIGH) ? TDI_H:TDI_L;
	    JTAG_SET(tdi|TMS_L|TCK_L);JTAG_DELAY();
	    JTAG_SET(tdi|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-DR 
	    rdDR[i]=JTAG_GET_TDO();
	}

	tdi=(wrDR[i]==HIGH) ? TDI_H:TDI_L;	//i=S3C24A0_MAX_CELL_INDEX
	JTAG_SET(tdi|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(tdi|TMS_H|TCK_H);JTAG_DELAY(); 	//Exit1-DR
	rdDR[i] = JTAG_GET_TDO();
	

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Update-DR

	//Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Update-DR
}


void  JTAG_ShiftDRStateNoTdo(char *wrDR)
{
	int size;
	int i;
	int tdi;

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Select-DR-Scan 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Capture-DR 

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-DR 

	size=strlen(wrDR);

	for(i=0;i<(size-1);i++)
	{
	    tdi=(wrDR[i]==HIGH) ? TDI_H:TDI_L;
	    JTAG_SET(tdi|TMS_L|TCK_L);JTAG_DELAY();
	    JTAG_SET(tdi|TMS_L|TCK_H);JTAG_DELAY(); 	//Shift-DR 
//	    rdDR[i]=JTAG_GET_TDO();
	}


	tdi=(wrDR[i]==HIGH) ? TDI_H:TDI_L;	//i=S3C2413_MAX_CELL_INDEX
	JTAG_SET(tdi|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(tdi|TMS_H|TCK_H);JTAG_DELAY(); 	//Exit1-DR
	//rdDR[i] = JTAG_GET_TDO();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); 	//Update-DR

	//Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); 	//Update-DR
}

void JTAG_Reset(void)
{
	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();
	
	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY();
}


int Core_ReadId(void)
{
	int i;
	char id[33];
	U32 id32;
	
	JTAG_Reset();

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY(); // Why 4 times?
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status  

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-DR Scan Status
	
	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-IR Scan Status

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Capture-IR Status

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Shift-IR Status


	//ARM926EJ IDCODE Instruction "1110"

	JTAG_SET(TDI_L|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_L|TMS_L|TCK_H);JTAG_DELAY(); // '0'

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // '1'

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // '1'

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // '1', //Exit1-IR

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Update_IR

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-DR-Scan

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Capture-DR

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Shift-DR

// 	Read IDcode..
	for( i=0 ; i<32 ; i++)
	{
	    JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	    JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Shift-DR
	    id[i]=(char)JTAG_GET_TDO();
	}

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); //Exit1_DR
	id[i]=(char)JTAG_GET_TDO();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Update_DR

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY(); // Why 3 times?	
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle


	id32=0;                          
	for(i=31 ;i>=0 ;i--)
	{
	    if(id[i]==HIGH)
		id32|=(1<<i);
	}

	if(id32 == 0x0792609d) {
		printf("> ARM926EJ(ID=0x%08x) is detected.\n",id32);
		printf("> You cannot program memory read/write\n");
		printf("> If you want to program memory read/write, OM[4:0] shoud be set '10111'\n");
		return 1;
	}
	else {
		return 0;
	}

}



void S3C2413_ReadId(void)
{
	int i;
	char id[33];
	U32 id32;
	
	JTAG_Reset();

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY(); // Why 4 times?
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle Status  

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-DR Scan Status
	
	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-IR Scan Status

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Capture-IR Status

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Shift-IR Status


	//S3C2413 IDCODE Instruction "10"

	JTAG_SET(TDI_L|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_L|TMS_L|TCK_H);JTAG_DELAY(); // '0'

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // '1', //Exit1-IR

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Update_IR

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Select-DR-Scan

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Capture-DR

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Shift-DR

// 	Read IDcode..
	for( i=0 ; i<32 ; i++)
	{
	    JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	    JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); //Shift-DR
	    id[i]=(char)JTAG_GET_TDO();
	}

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); //Exit1_DR
	id[i]=(char)JTAG_GET_TDO();

	JTAG_SET(TDI_H|TMS_H|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_H|TCK_H);JTAG_DELAY(); // Update_DR

	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY(); // Why 3 times?	
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle
	JTAG_SET(TDI_H|TMS_L|TCK_L);JTAG_DELAY();
	JTAG_SET(TDI_H|TMS_L|TCK_H);JTAG_DELAY(); // Run-Test/Idle


	id32=0;                          
	for(i=31 ;i>=0 ;i--)
	{
	    if(id[i]==HIGH)
		id32|=(1<<i);
	}

	switch(id32)
	{
	case 0x00c8509d: //S3C2413X is detected.   ??
	    printf("> S3C2413(ID=0x%08x) is detected.\n",id32);
		printf("> You can program memory read/wirte\n");
	    break;
	default:
	    break;
	}
}



int delayLoopCount;

void Delay(int count) // unit = 100ns
{
    
    int i,j;
    for(i=0 ; i<count ; i++)
        for(j=0;j<delayLoopCount;j++);
}
