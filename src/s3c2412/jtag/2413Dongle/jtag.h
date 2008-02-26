#ifndef __JTAG_H__
#define __JTAG_H__

#include "ppt.h"

#define LOW 			 '0'               
#define HIGH			 '1'
   

// Pin Connections
//   TCK   :DATA[0]   (2) 
//   TDI   :DATA[1]   (3) 
//   TMS   :DATA[2]   (4) 
//   TDO   :STATUS[7] (11)

#define TCK_H		0x01
#define TDI_H		0x02              
#define TMS_H		0x04
#define TCK_L		0x00
#define TDI_L		0x00              
#define TMS_L		0x00
#define JTAG_SET(value)	OutputPpt(value)

#define JTAG_GET_TDO()	( (InputPpt()&(1<<7)) ? LOW:HIGH )  //STATUS7 is read inverted. 		


// JTAG Instruction Definition for S3C24a0
#define EXTEST		"00"  //LSB...MSB
#define BYPASS		"11"
#define IDCODE		"10"
#define SAMPLE_PRELOAD	"01"

extern int delayLoopCount;

int Core_ReadId(void);
void S3C2413_ReadId(void);
void JTAG_ReadId(void);
void JTAG_RunTestldleState(void);
void JTAG_ShiftIRState(char *wrIR);
void JTAG_ShiftDRState(char *wrDR, char *rdDR);
void  JTAG_ShiftDRStateNoTdo(char *wrDR);
void Delay(int count);
#endif //__JTAG_H__



