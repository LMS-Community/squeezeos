
#include <stdio.h>
#include "def.h"
#include "pin2413.h"
#include "Jtag.h"


char outCellValue[S2413_MAX_CELL_INDEX+2];
char inCellValue[S2413_MAX_CELL_INDEX+2];
int  dataOutCellIndex[16];
int  dataInCellIndex[16];
int  addrCellIndex[27];

void S2413_InitCell(void)
{
    int i;
    dataOutCellIndex[0] = DATA0_OUT;
    dataOutCellIndex[1] = DATA1_OUT;
    dataOutCellIndex[2] = DATA2_OUT;
    dataOutCellIndex[3] = DATA3_OUT;
    dataOutCellIndex[4] = DATA4_OUT;
    dataOutCellIndex[5] = DATA5_OUT;
    dataOutCellIndex[6] = DATA6_OUT;
    dataOutCellIndex[7] = DATA7_OUT;	
    dataOutCellIndex[8] = DATA8_OUT;
    dataOutCellIndex[9] = DATA9_OUT;
    dataOutCellIndex[10] = DATA10_OUT;
    dataOutCellIndex[11] = DATA11_OUT;
    dataOutCellIndex[12] = DATA12_OUT;
    dataOutCellIndex[13] = DATA13_OUT;
    dataOutCellIndex[14] = DATA14_OUT;
    dataOutCellIndex[15] = DATA15_OUT;	

    dataInCellIndex[0] = DATA0_IN;  
    dataInCellIndex[1] = DATA1_IN;
    dataInCellIndex[2] = DATA2_IN;
    dataInCellIndex[3] = DATA3_IN;
    dataInCellIndex[4] = DATA4_IN;
    dataInCellIndex[5] = DATA5_IN;
    dataInCellIndex[6] = DATA6_IN;
    dataInCellIndex[7] = DATA7_IN;
    dataInCellIndex[8] = DATA8_IN;  
    dataInCellIndex[9] = DATA9_IN;
    dataInCellIndex[10] = DATA10_IN;
    dataInCellIndex[11] = DATA11_IN;
    dataInCellIndex[12] = DATA12_IN;
    dataInCellIndex[13] = DATA13_IN;
    dataInCellIndex[14] = DATA14_IN;
    dataInCellIndex[15] = DATA15_IN;

    addrCellIndex[0]=ADDR0;
    addrCellIndex[1]=ADDR1;
    addrCellIndex[2]=ADDR2;
    addrCellIndex[3]=ADDR3;
    addrCellIndex[4]=ADDR4;
    addrCellIndex[5]=ADDR5;
    addrCellIndex[6]=ADDR6;
    addrCellIndex[7]=ADDR7;
    addrCellIndex[8]=ADDR8;
    addrCellIndex[9]=ADDR9;
    addrCellIndex[10]=ADDR10;
    addrCellIndex[11]=ADDR11;
    addrCellIndex[12]=ADDR12;
    addrCellIndex[13]=ADDR13;
    addrCellIndex[14]=ADDR14;
    addrCellIndex[15]=ADDR15;
    addrCellIndex[16]=ADDR16;
    addrCellIndex[17]=ADDR17;
    addrCellIndex[18]=ADDR18;
    addrCellIndex[19]=ADDR19;
    addrCellIndex[20]=ADDR20;
    addrCellIndex[21]=ADDR21;
    addrCellIndex[22]=ADDR22;
    addrCellIndex[23]=ADDR23;
    addrCellIndex[24]=ADDR24;
    addrCellIndex[25]=ADDR25;
	addrCellIndex[26]=ADDR26;
    
    //outCellValue[] must be initialized by dummy values for JTAG_ShiftDRState();
    for(i=0;i<=S2413_MAX_CELL_INDEX;i++)
    {
	outCellValue[i]=HIGH;
	inCellValue[i]='u';
    }
    outCellValue[S2413_MAX_CELL_INDEX+1]='\0';
    inCellValue[S2413_MAX_CELL_INDEX+1]='\0';

    JTAG_RunTestldleState();
    JTAG_ShiftIRState(SAMPLE_PRELOAD);
    JTAG_ShiftDRState(outCellValue,inCellValue); //inCellValue[] is initialized.

    for(i=0;i<=S2413_MAX_CELL_INDEX;i++)
    {
	outCellValue[i]=inCellValue[i];	    //outCellValue[] is initialized.
    }

    //Memory control signal initialization.
    S2413_SetPin(DATA0_7_CON,HIGH);    //HIGH=input, LOW=output
	S2413_SetPin(DATA8_15_CON,HIGH);
	    
    S2413_SetPin(GCSn0,HIGH);
	S2413_SetPin(GCSn1,HIGH);
	S2413_SetPin(GCSn2,HIGH);
    S2413_SetPin(GCSn3,HIGH);
	S2413_SetPin(GCSn4,HIGH);
	S2413_SetPin(GCSn5,HIGH);    
	S2413_SetPin(GCSn6,HIGH);    
	S2413_SetPin(GCSn7,HIGH);    

    S2413_SetPin(NWE,HIGH); 
    S2413_SetPin(NOE,HIGH); 
    S2413_SetPin(NBE0,HIGH); 
    S2413_SetPin(NBE1,HIGH); 
    
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
    
    S2413_SetPin(RNB,HIGH); 
    S2413_SetPin(ALE,HIGH); 
	S2413_SetPin(CLE,HIGH);

}


void S2413_SetPin(int index, char value)
{
    outCellValue[index] = value;
}


char S2413_GetPin(int index)
{
    return inCellValue[index];
}


void S2413_SetAddr(U32 addr)
{
    int i;

    for(i=0;i<=25;i++)
    {
    	if(addr & (1<<i))
	    outCellValue[addrCellIndex[i]]=HIGH;
	else
	    outCellValue[addrCellIndex[i]]=LOW;
    }
}


void S2413_SetDataByte(U8 data)
{
    int i;

    for(i=0;i<8;i++)
    {
    	if(data & (1<<i))
	    outCellValue[dataOutCellIndex[i]]=HIGH;
	else
	    outCellValue[dataOutCellIndex[i]]=LOW;
    }
}


void S2413_SetDataHW(U16 data)
{
    int i;

    for(i=0;i<16;i++)
    {
    	if(data & (1<<i))
	    outCellValue[dataOutCellIndex[i]]=HIGH;
		else
	    outCellValue[dataOutCellIndex[i]]=LOW;
    }
}


void S2413_SetDataWord(U32 data)
{
    int i;

    for(i=0;i<32;i++)
    {
    	if(data & (1<<i))
	    outCellValue[dataOutCellIndex[i]]=HIGH;
	else
	    outCellValue[dataOutCellIndex[i]]=LOW;
    }
}



U8 S2413_GetDataByte(void)
{
    int	i;
    U8 data=0;

    for(i=0;i<8;i++)
    {
	if(inCellValue[dataInCellIndex[i]]==HIGH)
	{
	    data = (data | (1<<i));
	}
    }
    return data;
}


U16 S2413_GetDataHW(void)
{
    int	i;
    U16 data=0;

    for(i=0;i<16;i++)
    {
	if(inCellValue[dataInCellIndex[i]]==HIGH)
	{
	    data = (data | (1<<i));
	}
    }
    return data;
}


U32 S2413_GetDataWord(void)
{
    int	i;
    U32 data=0;

    for(i=0;i<32;i++)
    {
	if(inCellValue[dataInCellIndex[i]]==HIGH)
	{
	    data = (data | (1<<i));
	}
    }
    return data;
}