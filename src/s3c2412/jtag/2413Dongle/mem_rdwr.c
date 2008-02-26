#include <stdio.h>
#include "pin2413.h"
#include "Jtag.h"
#include "mem_rdwr.h"
#include "sjf2413.h"

//*************************************************
//*************************************************
//**     JTAG dependent primitive functions      **
//************************************************* 
//*************************************************
void MRW_JtagInit(void)
{
    JTAG_RunTestldleState();
    JTAG_ShiftIRState(EXTEST);
    //The initial value of pins will be defined here.
}


int S2413_Addr2Bank(U32 addr)
{
    if(addr<0x4000000)
	return 0;
    else if(addr<0x8000000)
	return 1;
    else if(addr<0xc000000)
	return 2;

    printf("ERROR:address range overflow\n");
    return 0;
}


void S2413_Assert_nGCS(U32 addr)
{
    if(addr<0x8000000)
	{
	S2413_SetPin(GCSn0,LOW);
	//printf("cs=0\n");
	}
    else if(addr<0x10000000)
	{
	S2413_SetPin(GCSn1,LOW);
	//printf("cs=1\n");
	}
    else if(addr<0x18000000)
	{
	S2413_SetPin(GCSn2,LOW);
	//printf("cs=2\n");
	}	
    else if(addr<0x20000000)
	{
	S2413_SetPin(GCSn3,LOW);
	//printf("cs=1\n");
	}
    else if(addr<0x28000000)
	{
	S2413_SetPin(GCSn4,LOW);
	//printf("cs=2\n");
	}
    else if(addr<0x30000000)
	{
	S2413_SetPin(GCSn5,LOW);
	//printf("cs=1\n");
	}
    else if(addr<0x38000000)
	{
	S2413_SetPin(GCSn6,LOW);
	//printf("cs=2\n");
	}
	else if(addr<0x40000000)
	{
	S2413_SetPin(GCSn7,LOW);
	//printf("cs=2\n");
	}

}


void S2413_Deassert_nGCS(U32 addr)
{
	S2413_SetPin(GCSn0_CON,LOW);
	S2413_SetPin(GCSn1_CON,LOW);
	S2413_SetPin(GCSn2_CON,LOW);
	S2413_SetPin(GCSn3_CON,LOW);
	S2413_SetPin(GCSn4_CON,LOW);
	S2413_SetPin(GCSn5_7_CON,LOW);
	
    if(addr<0x8000000)
	{
	S2413_SetPin(GCSn0,HIGH);
	//printf("cs=0\n");
	}
    else if(addr<0x10000000)
	{
	S2413_SetPin(GCSn1,HIGH);
	//printf("cs=1\n");
	}
    else if(addr<0x18000000)
	{
	S2413_SetPin(GCSn2,HIGH);
	//printf("cs=2\n");
	}	
    else if(addr<0x20000000)
	{
	S2413_SetPin(GCSn3,HIGH);
	//printf("cs=1\n");
	}
    else if(addr<0x28000000)
	{
	S2413_SetPin(GCSn4,HIGH);
	//printf("cs=2\n");
	}
    else if(addr<0x30000000)
	{
	S2413_SetPin(GCSn5,HIGH);
	//printf("cs=1\n");
	}
    else if(addr<0x38000000)
	{
	S2413_SetPin(GCSn6,HIGH);
	//printf("cs=2\n");
	}
	else if(addr<0x40000000)
	{
	S2413_SetPin(GCSn7,HIGH);
	//printf("cs=2\n");
	}
}


//***********************************************************
// Fast Version                   
// xxxQ: fast version by removing nGCS_to_nWE/nOE setup time.
// xxxQQ: more fast version by omitting nGCS deassertion.
//***********************************************************

U16 MRW_Rd16(U32 addr)
{
	U16 temp;

	S2413_SetPin(DATA0_7_CON ,HIGH); 
	S2413_SetPin(DATA8_15_CON ,HIGH); 

	S2413_SetPin(ADDR0_CON,LOW); 
	S2413_SetPin(ADDR1_15_CON,LOW); 
	S2413_SetPin(ADDR16_CON,LOW); 
	S2413_SetPin(ADDR17_CON,LOW); 
	S2413_SetPin(ADDR18_CON,LOW); 
	S2413_SetPin(ADDR19_CON,LOW); 
	S2413_SetPin(ADDR20_CON,LOW); 
	S2413_SetPin(ADDR21_CON,LOW); 
	S2413_SetPin(ADDR22_CON,LOW);
    S2413_SetPin(ADDR23_CON,LOW); 
	S2413_SetPin(ADDR24_CON,LOW); 
	S2413_SetPin(ADDR25_CON,LOW);
	S2413_SetPin(ADDR26_CON,LOW);

	S2413_SetPin(NOE_CON,LOW); 

    S2413_SetAddr(addr);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

    S2413_Assert_nGCS(addr);
    S2413_SetPin(NOE,LOW); 
    JTAG_ShiftDRStateNoTdo(outCellValue); 

	JTAG_ShiftDRState(outCellValue,inCellValue); 
	temp= S2413_GetDataHW();

    S2413_SetPin(NOE,HIGH);
    S2413_Deassert_nGCS(addr);
	JTAG_ShiftDRStateNoTdo(outCellValue);
    return temp;
}


U16 MRW_Rd16Q(U32 addr)
{

	S2413_SetPin(DATA0_7_CON ,HIGH); 
	S2413_SetPin(DATA8_15_CON ,HIGH); 

	S2413_SetPin(FRE_CON,LOW); 

    S2413_SetAddr(addr);
	S2413_Assert_nGCS(addr);
    S2413_SetPin(NOE,LOW); 
    JTAG_ShiftDRStateNoTdo(outCellValue); 

	S2413_SetPin(NOE,HIGH);
    S2413_Deassert_nGCS(addr);
	JTAG_ShiftDRStateNoTdo(outCellValue);
	
    JTAG_ShiftDRState(outCellValue,inCellValue); 
    return S2413_GetDataHW();
}


void MRW_Wr16(U32 addr,U16 data)
{
	S2413_SetPin(DATA0_7_CON ,LOW); 
	S2413_SetPin(DATA8_15_CON ,LOW); 

	S2413_SetPin(ADDR0_CON,LOW); 

	S2413_SetPin(ADDR1_15_CON,LOW); 
	S2413_SetPin(ADDR16_CON,LOW); 
	S2413_SetPin(ADDR17_CON,LOW); 
	S2413_SetPin(ADDR18_CON,LOW); 
	S2413_SetPin(ADDR19_CON,LOW); 
	S2413_SetPin(ADDR20_CON,LOW); 
	S2413_SetPin(ADDR21_CON,LOW); 
	S2413_SetPin(ADDR22_CON,LOW);
    S2413_SetPin(ADDR23_CON,LOW); 
	S2413_SetPin(ADDR24_CON,LOW); 
	S2413_SetPin(ADDR25_CON,LOW);
	S2413_SetPin(ADDR26_CON,LOW);
 
	S2413_SetPin(NWE_CON,LOW);
	S2413_SetPin(NOE_CON,LOW);
	S2413_SetPin(GCSn0_CON,LOW);
	
	S2413_SetPin(NWE,HIGH);
	S2413_SetPin(NOE,HIGH);
	S2413_SetPin(GCSn0,HIGH);
	JTAG_ShiftDRStateNoTdo(outCellValue);

#if 1
    S2413_SetAddr(addr);
	S2413_Assert_nGCS(addr);
	S2413_SetPin(NWE,LOW);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

	S2413_SetDataHW(data);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

	S2413_SetPin(NOE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); 

    S2413_SetPin(NWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); //tCOH

#else
	S2413_SetAddr(addr);
	JTAG_ShiftDRStateNoTdo(outCellValue); 

	S2413_Assert_nGCS(addr);
	S2413_SetPin(NWE,LOW);
	JTAG_ShiftDRStateNoTdo(outCellValue); 

	S2413_SetDataHW(data);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

	S2413_SetPin(NWE,HIGH);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOH

#endif
	
	S2413_SetPin(DATA0_7_CON ,HIGH); 
	S2413_SetPin(DATA8_15_CON ,HIGH); 

    S2413_Deassert_nGCS(addr);
   	JTAG_ShiftDRStateNoTdo(outCellValue); 
}


void MRW_Wr16Q(U32 addr,U16 data)
{
	S2413_SetPin(DATA0_7_CON ,LOW); 
	S2413_SetPin(DATA8_15_CON ,LOW); 

	S2413_SetPin(NWE_CON,LOW);
	S2413_SetPin(NOE_CON,LOW);
	
	S2413_SetAddr(addr);
	S2413_Assert_nGCS(addr);
	S2413_SetPin(NWE,LOW);
	S2413_SetDataHW(data);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

    S2413_SetPin(NWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); //tCOH

    S2413_Deassert_nGCS(addr);
   	JTAG_ShiftDRStateNoTdo(outCellValue); 


}

void MRW_Wr16QQ(U32 addr,U16 data)
{
	S2413_SetPin(DATA0_7_CON ,LOW); 
	S2413_SetPin(DATA8_15_CON ,LOW); 

	S2413_SetPin(NWE_CON,LOW);
	S2413_SetPin(NOE_CON,LOW);
		
    S2413_SetAddr(addr);
	S2413_Assert_nGCS(addr);
	S2413_SetPin(NWE,LOW);
	S2413_SetDataHW(data);
	JTAG_ShiftDRStateNoTdo(outCellValue); //tCOS

    S2413_SetPin(NWE,HIGH);
    JTAG_ShiftDRStateNoTdo(outCellValue); //tCOH

}

