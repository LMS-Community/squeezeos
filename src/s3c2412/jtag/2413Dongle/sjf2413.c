/********************************************************************************** 
 The SJF2413 is based on SJF2410, which was written by Inwook Kong(purnnamu).  
 The main difference between SJF2413 and SJF2410 is BSC(Boundary Scan Cell)Index.
 Refer to S3C2413_jtag_bsdl file.
 Thanks to Inwook Kong. 
 
 Revision history
 2005.10.27: ver 0.1 (Boaz)
  - SEC JTAG FLASH Program for S3C2413 & SMDK2413 B/D.
  - K9S1208, AMD 29LV800BB StrataFlash programming is supported.

**********************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "def.h"
#include "pin2413.h"
#include "jtag.h"
#include "ppt.h"
#include "k9s1208.h"
#include "am29f800.h"
#include "mem_rdwr.h"

FILE *stream;
U32 imageSize;

char srcFileName[256];
void OpenImageFile(char *filename);
void OpenPpt(void);
void ErrorBanner(void);

static void *function[]=
{
    "K9S1208 prog    ",
    "AM29LV800 Prog  ",
    "Exit            ",
    0
};

void main(int argc,char *argv[])
{
    char num=0;
    int i;

	
    printf("\n");
    printf("+------------------------------------+\n");
    printf("|     SEC JTAG FLASH(SJF) v 0.0      |\n");
    printf("|     (S3C2413 & SMDK2413 B/D)       |\n");
    printf("+------------------------------------+\n");
    printf("Usage: SJF /f:<filename> /d=<delay>\n");

    delayLoopCount=100;
    srcFileName[0]='\0';
    for(i=1;i<argc;i++)
    {
		switch(argv[i][1])
		{
		case 'f':
			strcpy(srcFileName,&(argv[i][3]));
			break;
		case 'd':
			delayLoopCount=atoi(&argv[i][3]);
			break;
		default:
			printf("ERROR: unknown option /%c is detected.\n",argv[i][1]);
			break;
		}
    }

    OpenPpt();

   if(srcFileName[0]!='\0')
	OpenImageFile(srcFileName);

	if(Core_ReadId())
		return;

	S3C2413_ReadId();

    S2413_InitCell();
	
    printf("\n[SJF Main Menu]\n");
    i=0;
    while(1)
    {   //display menu
	printf("%2d:%s",i,function[i]);
	i++;
	if((int)(function[i])==0)
	{
	    printf("\n");
	    break;
	}
	if((i%4)==0)
	    printf("\n");
    }
    
    printf("Select the function to test:");
    scanf("%d",&i);
    switch(i)
    {
    case 0:
	if(srcFileName[0]==0)
	{
	    printf("ERROR:Source file name is not valid.\n");
	    return;
	}
       	K9S1208_Menu();
	break;

    case 1:
	if(srcFileName[0]==0)
	{
	    printf("ERROR:Source file name is not valid.\n");
	    return;
	} 
       	ProgramAM29F800();
	break;

    default:
	return;
	break; //Exit menu
    }
    return;
}



void OpenImageFile(char *filename)
{
    U32 fileEnd,fileStart;
 
	stream = fopen(filename,"rb");
//	stream = fopen("2413loader8bit.bin","rb");
    if(stream==NULL)
    {
	printf("\nERROR:can't find the file.\n");
	exit(0);
    }

    fseek(stream,0L,SEEK_END);
    fileEnd=ftell(stream);
    fseek(stream,0L,SEEK_SET);
    fileStart=ftell(stream);

    imageSize=fileEnd-fileStart;  /*fileend == peof+1 */
	printf("Image size:%x\n",imageSize);
}


int LoadImageFile(U8 *buf,int size)
{
    int i,readSize=size;
    for(i=0;i<size;i++)
    {
	if(feof(stream))
	{
	    readSize=i;
	    for(;i<size;i++)buf[i]=0;
	    break;
	}
	buf[i] = fgetc(stream);
    }
    return readSize;
}


void OpenPpt(void)
{
    if(!InstallGiveIo())
    {
        printf("ERROR: Couldn't open giveio.sys\n");
        exit(0);
    }

    validPpt = GetValidPpt();
    if(!validPpt)
    {
	printf("ERROR: Unable to find a parallel port\n");
	exit(0);
    }
    SetPptCompMode();	
}


