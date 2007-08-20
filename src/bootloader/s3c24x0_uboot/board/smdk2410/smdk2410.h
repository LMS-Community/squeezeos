/*
 * (C) Copyright 2002, 2003
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
 /****************************************************************************
 * Global routines used for SMDK2410 
 *****************************************************************************/

#include <s3c2410.h>

extern int  mem_test(unsigned long start, unsigned long ramsize,int mode);
static inline void NF_Reset(void);


#if (CONFIG_COMMANDS & CFG_CMD_NAND)
typedef enum {
	NFCE_LOW,
	NFCE_HIGH
} NFCE_STATE;

static inline void NF_Conf(u16 conf)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFCONF = conf;
}

/* wjluv add this for SMDK2440 */
static inline void NF_Cont(u16 cont)
{

	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFCONT = cont;
}

static inline void NF_Cmd(u8 cmd)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFCMD = cmd;
}

static inline void NF_CmdW(u8 cmd)
{
	NF_Cmd(cmd);
	udelay(1);
}

static inline void NF_Addr(u8 addr)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFADDR = addr;
}

static inline void NF_SetCE(NFCE_STATE s)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	/* wjluv changes this for SMDK2440 */
#if 0
	switch (s) {
		case NFCE_LOW:
			nand->NFCONF &= ~(1<<11);
			break;

		case NFCE_HIGH:
			nand->NFCONF |= (1<<11);
			break;
	}
#else

	switch(s){
		case NFCE_LOW:
			nand->NFCONT = ( (1<<4)|(0<<1)|(1<<0) );
			break;
		case NFCE_HIGH:
			nand->NFCONT |= (1<<1);
			break;
	}
#endif
}



static inline void NF_Init(void)
{
/* wjluv changes this for SMDK2440 */
#if 0
#define TACLS   0
#define TWRPH0  3 
#define TWRPH1  0 
	
	NF_Conf((1<<15)|(0<<14)|(0<<13)|(1<<12)|(1<<11)|(TACLS<<8)|(TWRPH0<<4)|(TWRPH1<<0));
	/*nand->NFCONF = (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11)|(TACLS<<8)|(TWRPH0<<4)|(TWRPH1<<0); */
	/* 1  1    1     1,   1      xxx,  r xxx,   r xxx */
	/* En 512B 4step ECCR nFCE=H tACLS   tWRPH0   tWRPH1 */
	NF_Reset();
#endif

}

static inline void NF_WaitRB(void)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	while (!(nand->NFSTAT & (1<<0)));
}

/* wjluv adds this for SMDK2440 */
static inline void NF_ClearRB(void)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFSTAT = 0x6;

}

static inline void NF_Reset(void)
{
	int i;
	NF_SetCE(NFCE_LOW);
	NF_ClearRB();
	NF_Cmd(0xff);
	for(i=0;i<10;i++);
	NF_WaitRB();
	NF_SetCE(NFCE_HIGH);
}

static inline void NF_Write(u8 data)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFDATA = data;
}

static inline u8 NF_Read(void)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	return(nand->NFDATA);
}

static inline void NF_Init_ECC(void)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	nand->NFCONF |= (1<<12);
}

static inline u32 NF_Read_ECC(void)
{
	S3C2410_NAND * const nand = S3C2410_GetBase_NAND();

	return(nand->NFECC);
}
#endif  // End of CONFIG_COMMANDS & CFG_CMD_NAND  
