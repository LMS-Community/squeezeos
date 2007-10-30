#ifndef __MEM_RDWR_H__
#define __MEM_RDWR_H__

#include "def.h"

void MRW_Menu(void);

//************** JTAG dependent functions **************
void MRW_JtagInit(void);
int S2413_Addr2Bank(U32 addr);
void S2413_Assert_nGCS(U32 addr);
void S2413_Deassert_nGCS(U32 addr);

U16 MRW_Rd16(U32 addr);
void MRW_Wr16(U32 addr,U16 data);


U16 MRW_Rd16Q(U32 addr);
void MRW_Wr16Q(U32 addr,U16 data);

//specialized functions for Flash program speed-up.
void MRW_Wr16QQ(U32 addr,U16 data);
//*******************************************************


#endif //__MEM_RDWR_H__