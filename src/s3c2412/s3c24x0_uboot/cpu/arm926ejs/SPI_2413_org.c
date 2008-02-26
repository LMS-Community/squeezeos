
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#if defined(CONFIG_S3C2460x)
#include "asm/arch/s3c2460.h"
#elif defined(CONFIG_S3C2413)
#include <spi.h>
#include <s3c2413.h>
#endif

#if (CONFIG_COMMANDS & CFG_CMD_BOARD_TEST)



unsigned char SPI_BUFFER0[25];


unsigned char spi0TxStr[10],spi0RxStr[10];
unsigned int endSpi0Tx;

unsigned int spi0_rGPECON, spi0_rGPEDAT,spi0_rGPEDN;
unsigned int spi1_rGPGCON, spi1_rGPGDAT,spi1_rGPGDN;
int bufCnt;
int EFlag,FFlag;



void SPI_Baud_Rate_Set(unsigned int ch, unsigned int BaudRate)
{
	unsigned int PrescalerVal;


	//SystemCLK();
	//PrescalerVal = (unsigned int)(PCLK/2/BaudRate-1);

	    PrescalerVal = (unsigned int)(get_PCLK()/2/BaudRate - 1);
	   printf("get_PCLK = 0x%x\n",get_PCLK());
	    printf("PrescalerVal = 0x%x\n",PrescalerVal);
	if(ch) {
		rSPPRE1 = PrescalerVal;
		printf("BaudRate [%x]\trSPPRE1 [0x%08x]\n",BaudRate,rSPPRE1);
		}
	else {
		rSPPRE0 = PrescalerVal;
		printf("BaudRate [%x]\trSPPRE0 [0x%08x]\n",BaudRate,rSPPRE0);
		}
}

void SPI_Port_Init(unsigned int WhichMstr)
{
	spi0_rGPECON=rGPECON;
	spi0_rGPEDAT=rGPEDAT;
	spi0_rGPEDN=rGPEDN;

	spi1_rGPGCON=rGPGCON;
	spi1_rGPGDAT=rGPGDAT;
	spi1_rGPGDN=rGPGDN;


	if (WhichMstr==0)	// SPI0 is Master
	{
		rGPEDN |= (0x7<<11);		// Pull-down Disable
		rGPECON &= ~(0x3f<<22);	//
		rGPECON |= (0x2a<<22); 	// GPE13,12,11 = SPICLK0,SPIMOSI0,SPIMISO0


		rGPGCON &= ~(0x3<<4);
		rGPGCON |= (0x3<<4);		// GPIO => nSS0

		rGPGDAT |= (1<<2);		// nSS0 = 'High'
		rGPGCON &= ~(0x3<<4);
		rGPGCON |= (0x1<<4);		// nSS0 is Output

		rSPCON0 = 0x8;
		rSPPIN0 = 0x2;
		rSPPRE0 = 0;
		rSPFIC0 =0;

	}
	else if(WhichMstr==1)		// SPI1 is Master

	{

		rGPGDN |= ((0x7<<5)|(0x3<<2));		// Pull-down Disable
		rGPGCON &= ~(0x3f<<10);	//
		rGPGCON |= (0x3f<<10);	// GPG7,6,5 = SPICLK1,SPIMOSI1,SPIMISO1

		rGPGCON &= ~(0xc<<4);
		rGPGCON |= (0xc<<4);		// GPIO => nSS1

		rGPGDAT |= (1<<3);		// nSS1 = 'High'
		rGPGCON &= ~(0x3<<6);
		rGPGCON |= (0x1<<6);		// nSS1 is Output

		rSPCON1 = 0x8;
		rSPPIN1 = 0x2;
		rSPPRE1 = 0;
		rSPFIC1 =0;
	}
	else
	{
		rGPEDN |= (0x7<<11);
		rGPECON &= ~(0x3f<<22);	//
		rGPECON |=  (0x5<<24); 	//
		rGPGDAT |=  (0x3<12);

	}
}

void SPI_Port_Return(void)
{
	rGPECON=spi0_rGPECON;
	rGPEDAT=spi0_rGPEDAT;
	rGPEDN=spi0_rGPEDN;

	rGPGCON=spi1_rGPGCON;
	rGPGDAT=spi1_rGPGDAT;
	rGPGDN=spi1_rGPGDN;
}

void Master_nSS_Con0(int Set)
{
	rGPGDAT=(rGPGDAT&~(1<<2))|(Set<<2);
	if(Set) printf("=> Chip Select nSS0 Disassert\n");
	else printf("=> Chip Select nSS0 Assert\n");
}


void Master_nSS_Con1(int Set)
{
	rGPGDAT=(rGPGDAT&~(1<<3))|(Set<<3);
	if(Set) printf("Chip Select nSS1 Disassert");
	else printf("Chip Select nSS1 Assert");
}

void	SPI_nRESET_Init(void)
{
		rGPGDN |= (0x1<<2);			// Pull-down Disable

		rGPGCON &= ~(0x3<<22);	//output setting
		rGPGCON &= ~(0x1<<22);	//

		rGPGDAT |= (1<<2);		// nSS0 = 'High'


}

void SPI_nRESET_Con(int value)
{
	rGPGDAT=(rGPGDAT&~(1<<2))|(value<<2);
	if(value) printf("=> Chip nRESET HIGH\n");
	else printf("=>Chip nRESET LOW\n");
}

void SPI_Init(void)
{
	SPI_Port_Init(0);
	SPI_Baud_Rate_Set(0, 1000000);			// SPI Channel 0~10MHz

	SPI_nRESET_Init();

	rSPCON0=(0<<5)|(1<<4)|(1<<3)|(0<<2)|(0<<1)|(0<<0);//Polling,en-SCK,master,low,A,normal
	rSPPIN0=(0<<2)|(1<<1)|(0<<0);//dis-ENMUL,SBO,release
}

void SPI_Direction(unsigned char Set)
{
	rSPCON0 = (rSPCON0&~(1<<7))|(Set<<7);
}




#endif

