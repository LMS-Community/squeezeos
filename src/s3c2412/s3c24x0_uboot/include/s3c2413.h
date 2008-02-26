//=============================================================================
// File Name : 2413addr.h
// Function  : S3C2413 Define Address Register
// History
//   0.0 : Programming start (September 15,2005)
//=============================================================================

#ifndef __2413ADDR_H__
#define __2413ADDR_H__


//chapter2 EBI
#define rEBIPR      (*(volatile unsigned *)0x48800000)	//Bus priority decision
#define rBANK_CFG   (*(volatile unsigned *)0x48800004)	//Bank Configuration 


//chapter3 MOBILE DRAM CONTROLLER
#define rBANKCFG    (*(volatile unsigned *)0x48000000)	//Mobile DRAM configuration
#define rBANKCON1    (*(volatile unsigned *)0x48000004)	//Mobile DRAM control 
#define rBANKCON2    (*(volatile unsigned *)0x48000008)	//Mobile DRAM timing control 
#define rBANKCON3    (*(volatile unsigned *)0x4800000C)	//Mobile DRAM (E)MRS 
#define rREFRESH    (*(volatile unsigned *)0x48000010)	//Mobile DRAM refresh control
#define rTIMEOUT    (*(volatile unsigned *)0x48000014)	//Write Buffer Time out control 


//chapter4 SSMC
#define rSMBIDCYR0   (*(volatile unsigned *)0x4F000000)	//Bank0 idle cycle control 
#define rSMBIDCYR1   (*(volatile unsigned *)0x4F000020)	//Bank1 idle cycle control 
#define rSMBIDCYR2   (*(volatile unsigned *)0x4F000040)	//Bank2 idle cycle control 
#define rSMBIDCYR3   (*(volatile unsigned *)0x4F000060)	//Bank3 idle cycle control 
#define rSMBIDCYR4   (*(volatile unsigned *)0x4F000080)	//Bank0 idle cycle control 
#define rSMBIDCYR5   (*(volatile unsigned *)0x4F0000A0)	//Bank5 idle cycle control 
#define rSMBWSTRDR0  (*(volatile unsigned *)0x4F000004)	//Bank0 read wait state control 
#define rSMBWSTRDR1  (*(volatile unsigned *)0x4F000024)	//Bank1 read wait state control
#define rSMBWSTRDR2  (*(volatile unsigned *)0x4F000044)	//Bank2 read wait state control
#define rSMBWSTRDR3  (*(volatile unsigned *)0x4F000064)	//Bank3 read wait state control 
#define rSMBWSTRDR4  (*(volatile unsigned *)0x4F000084)	//Bank4 read wait state control 
#define rSMBWSTRDR5  (*(volatile unsigned *)0x4F0000A4)	//Bank5 read wait state control 
#define rSMBWSTWRR0  (*(volatile unsigned *)0x4F000008)	//Bank0 write wait state control 
#define rSMBWSTWRR1  (*(volatile unsigned *)0x4F000028)	//Bank1 write wait state control 
#define rSMBWSTWRR2  (*(volatile unsigned *)0x4F000048)	//Bank2 write wait state control 
#define rSMBWSTWRR3  (*(volatile unsigned *)0x4F000068)	//Bank3 write wait state control 
#define rSMBWSTWRR4  (*(volatile unsigned *)0x4F000088)	//Bank4 write wait state control 
#define rSMBWSTWRR5  (*(volatile unsigned *)0x4F0000A8)	//Bank5 write wait state control 
#define rSMBWSTOENR0 (*(volatile unsigned *)0x4F00000C)	//Bank0 output enable assertion delay control 
#define rSMBWSTOENR1 (*(volatile unsigned *)0x4F00002C)	//Bank1 output enable assertion delay control 
#define rSMBWSTOENR2 (*(volatile unsigned *)0x4F00004C)	//Bank2 output enable assertion delay control
#define rSMBWSTOENR3 (*(volatile unsigned *)0x4F00006C)	//Bank3 output enable assertion delay control 
#define rSMBWSTOENR4 (*(volatile unsigned *)0x4F00008C)	//Bank4 output enable assertion delay control
#define rSMBWSTOENR5 (*(volatile unsigned *)0x4F0000AC)	//Bank5 output enable assertion delay control 
#define rSMBWSTWENR0 (*(volatile unsigned *)0x4F000010)	//Bank0 write enable assertion delay control 
#define rSMBWSTWENR1 (*(volatile unsigned *)0x4F000030)	//Bank1 write enable assertion delay control 
#define rSMBWSTWENR2 (*(volatile unsigned *)0x4F000050)	//Bank2 write enable assertion delay control 
#define rSMBWSTWENR3 (*(volatile unsigned *)0x4F000070)	//Bank3 write enable assertion delay control 
#define rSMBWSTWENR4 (*(volatile unsigned *)0x4F000090)	//Bank4 write enable assertion delay control 
#define rSMBWSTWENR5 (*(volatile unsigned *)0x4F0000B0)	//Bank5 write enable assertion delay control 
#define rSMBCR0      (*(volatile unsigned *)0x4F000014)	//Bank0 control 
#define rSMBCR1      (*(volatile unsigned *)0x4F000034)	//Bank1 control 
#define rSMBCR2      (*(volatile unsigned *)0x4F000054)	//Bank2 control 
#define rSMBCR3      (*(volatile unsigned *)0x4F000074)	//Bank3 control 
#define rSMBCR4      (*(volatile unsigned *)0x4F000094)	//Bank4 control 
#define rSMBCR5      (*(volatile unsigned *)0x4F0000B4)	//Bank5 control 
#define rSMBSR0      (*(volatile unsigned *)0x4F000018)	//Bank0 status 
#define rSMBSR1      (*(volatile unsigned *)0x4F000038)	//Bank1 status 
#define rSMBSR2      (*(volatile unsigned *)0x4F000058)	//Bank2 status 
#define rSMBSR3      (*(volatile unsigned *)0x4F000078)	//Bank3 status 
#define rSMBSR4      (*(volatile unsigned *)0x4F000098)	//Bank4 status 
#define rSMBSR5      (*(volatile unsigned *)0x4F0000B8)	//Bank5 status 
#define rSMBWSTBRDR0 (*(volatile unsigned *)0x4F00001C)	//Bank0 burst read wait delay control 
#define rSMBWSTBRDR1 (*(volatile unsigned *)0x4F00003C)	//Bank1 burst read wait delay control 
#define rSMBWSTBRDR2 (*(volatile unsigned *)0x4F00005C)	//Bank2 burst read wait delay control 
#define rSMBWSTBRDR3 (*(volatile unsigned *)0x4F00007C)	//Bank3 burst read wait delay control 
#define rSMBWSTBRDR4 (*(volatile unsigned *)0x4F00009C)	//Bank4 burst read wait delay control 
#define rSMBWSTBRDR5 (*(volatile unsigned *)0x4F0000BC)	//Bank5 burst read wait delay control 
#define rSSMCSR      (*(volatile unsigned *)0x4F000200)	//SROMC status 
#define rSSMCCR      (*(volatile unsigned *)0x4F000204)	//SROMC control 


//chapter5 Nand Flash
#define rNFCONF		  (*(volatile unsigned *)0x4E000000)		  //NAND Flash configuration
#define rNFCONT		  (*(volatile unsigned *)0x4E000004)      //NAND Flash control
#define rNFCMD		  (*(volatile unsigned *)0x4E000008)      //NAND Flash command 
#define rNFADDR		  (*(volatile unsigned *)0x4E00000C)      //NAND Flash address
#define rNFDATA		  (*(volatile unsigned *)0x4E000010)      //NAND Flash data                         
#define rNFDATA8	  (*(volatile unsigned char *)0x4E000010)	  // NAND Flash data
#define rNFMECCD0	  (*(volatile unsigned *)0x4E000014)      //NAND Flash ECC for Main 
#define rNFMECCD1	  (*(volatile unsigned *)0x4E000018)      //NAND Flash ECC for Main 
#define rNFSECCD	  (*(volatile unsigned *)0x4E00001C)	  	//NAND Flash ECC for Spare Area
#define rNFSBLK 	  (*(volatile unsigned *)0x4E000020)		  //NAND Flash programmable start block address
#define rNFEBLK 	  (*(volatile unsigned *)0x4E000024) 	    //NAND Flash programmable end block address     
#define rNFSTAT 	  (*(volatile unsigned *)0x4E000028)      //NAND Flash operation status 
#define rNFECCERR0	(*(volatile unsigned *)0x4E00002C)      //NAND Flash ECC Error Status for I/O [7:0]
#define rNFECCERR1	(*(volatile unsigned *)0x4E000030)      //NAND Flash ECC Error Status for I/O [15:8]
#define rNFMECC0		(*(volatile unsigned *)0x4E000034)      //SLC or MLC NAND Flash ECC status
#define rNFMECC1		(*(volatile unsigned *)0x4E000038)	    //SLC or MLC NAND Flash ECC status	
#define rNFSECC 		(*(volatile unsigned *)0x4E00003C)  		//NAND Flash ECC for I/O[15:0]
#define rNFMLCBITPT	(*(volatile unsigned *)0x4E000040)  		//NAND Flash 4-bit ECC Error Pattern for data[7:0]

//chapter6 SYSEM CONTROLLER
#define rLOCKTIME 	(*(volatile unsigned *)0x4C000000)  		//MPLL/UPLL lock time conuter
#define rMPLLCON  	(*(volatile unsigned *)0x4C000004)  		//MPLL configuration
#define rUPLLCON  	(*(volatile unsigned *)0x4C000008)  		//UPLL configuration
#define rCLKCON   	(*(volatile unsigned *)0x4C00000C)  		//Clock generator control
#define rCLKDIVN  	(*(volatile unsigned *)0x4C000014)  		//Clock divider control
#define rOSCSET   	(*(volatile unsigned *)0x4C000018)  		//Oscillator stabilization time counter
#define rCLKSRC  	(*(volatile unsigned *)0x4C00001C)  		//Clock source control
#define rPWRMODECON (*(volatile unsigned *)0x4C000020)  		//Power management mode setting 
#define rPWRCFG     (*(volatile unsigned *)0x4C000024)  		//Power management configuration
#define rWKUPSTAT   (*(volatile unsigned *)0x4C000028)  		//Wakup status 
#define rENDIAN     (*(volatile unsigned *)0x4C00002C)  		//System endian control
#define rSWRSTCON   (*(volatile unsigned *)0x4C000030)  		//S/W reset control
#define rRSTCON     (*(volatile unsigned *)0x4C000034)  		//Reset control
#define rRSTSTAT    (*(volatile unsigned *)0x4C000038)  		//Reset status
#define rINFORM0    (*(volatile unsigned *)0x4C000070)  		//User defined informtion
#define rINFORM1    (*(volatile unsigned *)0x4C000074)  		//User defined informtion
#define rINFORM2    (*(volatile unsigned *)0x4C000078)  		//User defined informtion
#define rINFORM3    (*(volatile unsigned *)0x4C00007C)  		//User defined informtion


//chapter7 DMA
#define rDISRC0     (*(volatile unsigned *)0x4b000000)	//DMA 0 Initial source
#define rDISRCC0    (*(volatile unsigned *)0x4b000004)	//DMA 0 Initial source control
#define rDIDST0     (*(volatile unsigned *)0x4b000008)	//DMA 0 Initial Destination
#define rDIDSTC0    (*(volatile unsigned *)0x4b00000c)	//DMA 0 Initial Destination control
#define rDCON0      (*(volatile unsigned *)0x4b000010)	//DMA 0 Control
#define rDSTAT0     (*(volatile unsigned *)0x4b000014)	//DMA 0 Status (Read Only)
#define rDCSRC0     (*(volatile unsigned *)0x4b000018)	//DMA 0 Current source (Read Only)
#define rDCDST0     (*(volatile unsigned *)0x4b00001c)	//DMA 0 Current destination (Read Only)
#define rDMASKTRIG0 (*(volatile unsigned *)0x4b000020)	//DMA 0 Mask trigger
#define rDMAREQSEL0 (*(volatile unsigned *)0x4b000024)	//DMA 0 Request Selection register

#define rDISRC1     (*(volatile unsigned *)0x4b000040)	//DMA 1 Initial source
#define rDISRCC1    (*(volatile unsigned *)0x4b000044)	//DMA 1 Initial source control
#define rDIDST1     (*(volatile unsigned *)0x4b000048)	//DMA 1 Initial Destination
#define rDIDSTC1    (*(volatile unsigned *)0x4b00004c)	//DMA 1 Initial Destination control
#define rDCON1      (*(volatile unsigned *)0x4b000050)	//DMA 1 Control
#define rDSTAT1     (*(volatile unsigned *)0x4b000054)	//DMA 1 Status (Read Only)
#define rDCSRC1     (*(volatile unsigned *)0x4b000058)	//DMA 1 Current source (Read Only)
#define rDCDST1     (*(volatile unsigned *)0x4b00005c)	//DMA 1 Current destination (Read Only)
#define rDMASKTRIG1 (*(volatile unsigned *)0x4b000060)	//DMA 1 Mask trigger
#define rDMAREQSEL1 (*(volatile unsigned *)0x4b000064)	//DMA 1 Request Selection register

#define rDISRC2     (*(volatile unsigned *)0x4b000080)	//DMA 2 Initial source
#define rDISRCC2    (*(volatile unsigned *)0x4b000084)	//DMA 2 Initial source control
#define rDIDST2     (*(volatile unsigned *)0x4b000088)	//DMA 2 Initial Destination
#define rDIDSTC2    (*(volatile unsigned *)0x4b00008c)	//DMA 2 Initial Destination control
#define rDCON2      (*(volatile unsigned *)0x4b000090)	//DMA 2 Control
#define rDSTAT2     (*(volatile unsigned *)0x4b000094)	//DMA 2 Status (Read Only)
#define rDCSRC2     (*(volatile unsigned *)0x4b000098)	//DMA 2 Current source (Read Only)
#define rDCDST2     (*(volatile unsigned *)0x4b00009c)	//DMA 2 Current destination (Read Only)
#define rDMASKTRIG2 (*(volatile unsigned *)0x4b0000a0)	//DMA 2 Mask trigger
#define rDMAREQSEL2 (*(volatile unsigned *)0x4b0000a4)	//DMA 2 Request Selection register

#define rDISRC3     (*(volatile unsigned *)0x4b0000c0)	//DMA 3 Initial source
#define rDISRCC3    (*(volatile unsigned *)0x4b0000c4)	//DMA 3 Initial source control
#define rDIDST3     (*(volatile unsigned *)0x4b0000c8)	//DMA 3 Initial Destination
#define rDIDSTC3    (*(volatile unsigned *)0x4b0000cc)	//DMA 3 Initial Destination control
#define rDCON3      (*(volatile unsigned *)0x4b0000d0)	//DMA 3 Control
#define rDSTAT3     (*(volatile unsigned *)0x4b0000d4)	//DMA 3 Status (Read Only)
#define rDCSRC3     (*(volatile unsigned *)0x4b0000d8)	//DMA 3 Current source (Read Only)
#define rDCDST3     (*(volatile unsigned *)0x4b0000dc)	//DMA 3 Current destination (Read Only)
#define rDMASKTRIG3 (*(volatile unsigned *)0x4b0000e0)	//DMA 3 Mask trigger
#define rDMAREQSEL3 (*(volatile unsigned *)0x4b0000e4)	//DMA 3 Request Selection register


//chapter8 I/O PORT 
#define rGPACON    (*(volatile unsigned *)0x56000000)	//Configure the pins of port A
#define rGPADAT    (*(volatile unsigned *)0x56000004)	//The data for port A

#define rGPBCON    (*(volatile unsigned *)0x56000010)	//Configure the pins of port B
#define rGPBDAT    (*(volatile unsigned *)0x56000014)	//The data for port B
#define rGPBDN     (*(volatile unsigned *)0x56000018)	//Pull-down disable for port 

#define rGPBSLPCON (*(volatile unsigned *)0x5600001C)	//sleep mode configuration for port B

#define rGPCCON    (*(volatile unsigned *)0x56000020)	//Configure the pins of port C
#define rGPCDAT    (*(volatile unsigned *)0x56000024)	//The data for port C
#define rGPCDN     (*(volatile unsigned *)0x56000028)	//Pull-down disable for port C
#define rGPCSLPCON (*(volatile unsigned *)0x5600002C)	//sleep mode configuration for port C

#define rGPDCON    (*(volatile unsigned *)0x56000030)	//Configure the pins of port D
#define rGPDDAT    (*(volatile unsigned *)0x56000034)	//The data for port D
#define rGPDDN     (*(volatile unsigned *)0x56000038)	//Pull-down disable for port D
#define rGPDSLPCON (*(volatile unsigned *)0x5600003C)	//sleep mode configuration for port D

#define rGPECON    (*(volatile unsigned *)0x56000040)	//Configure the pins of port E
#define rGPEDAT    (*(volatile unsigned *)0x56000044)	//The data for port E
#define rGPEDN     (*(volatile unsigned *)0x56000048)	//Pull-down disable for port E
#define rGPESLPCON (*(volatile unsigned *)0x5600004C)	//sleep mode configuration for port E

#define rGPFCON    (*(volatile unsigned *)0x56000050)	//Configure the pins of port F
#define rGPFDAT    (*(volatile unsigned *)0x56000054)	//The data for port F
#define rGPFDN     (*(volatile unsigned *)0x56000058)	//Pull-down disable for port F

#define rGPGCON    (*(volatile unsigned *)0x56000060)	//Configure the pins of portt G 
#define rGPGDAT    (*(volatile unsigned *)0x56000064)	//The data for port G 
#define rGPGDN     (*(volatile unsigned *)0x56000068)	//Pull-down disable for port G
#define rGPGSLPCON (*(volatile unsigned *)0x5600006C)	//sleep mode configuration for port G

#define rGPHCON    (*(volatile unsigned *)0x56000070)	//Configure the pins of porttt H 
#define rGPHDAT    (*(volatile unsigned *)0x56000074)	//The data for port H 
#define rGPHDN     (*(volatile unsigned *)0x56000078)	//Pull-down disable for port H
#define rGPHSLPCON (*(volatile unsigned *)0x5600007C)	//sleep mode configuration for port H

#define rGPJCON    (*(volatile unsigned *)0x56000080)	//Configure the pins of portttt J
#define rGPJDAT    (*(volatile unsigned *)0x56000084)	//The data for port J 
#define rGPJDN     (*(volatile unsigned *)0x56000088)	//Pull-down disable for porl J
#define rGPJSLPCON (*(volatile unsigned *)0x5600008C)	//sleep mode configuration for port J

#define rMISCCR    (*(volatile unsigned *)0x56000090)	//Miscellaneous control
#define rDCLKCON   (*(volatile unsigned *)0x56000094)	//DCLK0/1 control
#define rEXTINT0   (*(volatile unsigned *)0x56000098)	//External interrupt control register 0
#define rEXTINT1   (*(volatile unsigned *)0x5600009C)	//External interrupt control register 1
#define rEXTINT2   (*(volatile unsigned *)0x560000A0)	//External interrupt control register 2
#define rEINTFLT0  (*(volatile unsigned *)0x560000A4)	//Reserved
#define rEINTFLT1  (*(volatile unsigned *)0x560000A8)	//Reserved
#define rEINTFLT2  (*(volatile unsigned *)0x560000Ac)	//External interrupt filter control register 2
#define rEINTFLT3  (*(volatile unsigned *)0x560000B0)	//External interrupt filter control register 3
#define rEINTMASK  (*(volatile unsigned *)0x560000B4)	//External interrupt mask
#define rEINTPEND  (*(volatile unsigned *)0x560000B8)	//External interrupt pending
#define rGSTATUS0  (*(volatile unsigned *)0x560000BC)	//External pin status
#define rGSTATUS1  (*(volatile unsigned *)0x560000C0)	//Chip ID(0x32440000)
#define rGSTATUS2  (*(volatile unsigned *)0x560000C4)	//Infrom
#define rGSTATUS3  (*(volatile unsigned *)0x560000C8)	//Infrom
#define rGSTATUS4  (*(volatile unsigned *)0x560000CC)	//Infrom
#define rGSTATUS5  (*(volatile unsigned *)0x560000D0)	//Infrom


// chapter9 PWM TIMER
#define rTCFG0  (*(volatile unsigned *)0x51000000)	//Configures the two 8-bit presclers
#define rTCFG1  (*(volatile unsigned *)0x51000004)	//5-MUX & DMA mode selecton
#define rTCON   (*(volatile unsigned *)0x51000008)	//Timer control
#define rTCNTB0 (*(volatile unsigned *)0x5100000c)	//Timer 0 count buffer 
#define rTCMPB0 (*(volatile unsigned *)0x51000010)	//Timer 0 compare buffer 
#define rTCNTO0 (*(volatile unsigned *)0x51000014)	//Timer 0 count observation 
#define rTCNTB1 (*(volatile unsigned *)0x51000018)	//Timer 1 count buffer 
#define rTCMPB1 (*(volatile unsigned *)0x5100001c)	//Timer 1 compare buffer 
#define rTCNTO1 (*(volatile unsigned *)0x51000020)	//Timer 1 count observation 
#define rTCNTB2 (*(volatile unsigned *)0x51000024)	//Timer 2 count buffer 
#define rTCMPB2 (*(volatile unsigned *)0x51000028)	//Timer 2 compare buffer 
#define rTCNTO2 (*(volatile unsigned *)0x5100002c)	//Timer 2 count observation 
#define rTCNTB3 (*(volatile unsigned *)0x51000030)	//Timer 3 count buffer 
#define rTCMPB3 (*(volatile unsigned *)0x51000034)	//Timer 3 compare buffer 
#define rTCNTO3 (*(volatile unsigned *)0x51000038)	//Timer 3 count observation 
#define rTCNTB4 (*(volatile unsigned *)0x5100003c)	//Timer 4 count buffer 
#define rTCNTO4 (*(volatile unsigned *)0x51000040)	//Timer 4 count observation


// chapter10 UART
#define rULCON0     (*(volatile unsigned *)0x50000000)	//UART channel 0 Line control
#define rUCON0      (*(volatile unsigned *)0x50000004)	//UART channel 0 Control
#define rUFCON0     (*(volatile unsigned *)0x50000008)	//UART channel 0 FIFO control
#define rUMCON0     (*(volatile unsigned *)0x5000000c)	//UART channel 0 Modem control
#define rUTRSTAT0   (*(volatile unsigned *)0x50000010)	//UART channel 0 Tx/Rx status
#define rUERSTAT0   (*(volatile unsigned *)0x50000014)	//UART channel 0 Rx error status
#define rUFSTAT0    (*(volatile unsigned *)0x50000018)	//UART channel 0 FIFO status
#define rUMSTAT0    (*(volatile unsigned *)0x5000001c)	//UART channel 0 Modem status
#define rUBRDIV0    (*(volatile unsigned *)0x50000028)	//UART Baud rate divisor 0
#define rUDIVSLOT0  (*(volatile unsigned *)0x5000002c)	//UART Baud rate dicisor 0

#define rULCON1     (*(volatile unsigned *)0x50004000)	//UART channel 1 Line control
#define rUCON1      (*(volatile unsigned *)0x50004004)	//UART channel 1 Control
#define rUFCON1     (*(volatile unsigned *)0x50004008)	//UART channel 1 FIFO control
#define rUMCON1     (*(volatile unsigned *)0x5000400c)	//UART channel 1 Modem control
#define rUTRSTAT1   (*(volatile unsigned *)0x50004010)	//UART channel 1 Tx/Rx status
#define rUERSTAT1   (*(volatile unsigned *)0x50004014)	//UART channel 1 Rx error status
#define rUFSTAT1    (*(volatile unsigned *)0x50004018)	//UART channel 1 FIFO status
#define rUMSTAT1    (*(volatile unsigned *)0x5000401c)	//UART channel 1 Modem status
#define rUBRDIV1    (*(volatile unsigned *)0x50004028)	//UART Baud rate divisor 1
#define rUDIVSLOT1  (*(volatile unsigned *)0x5000402c)	//UART Baud rate divisor 1

#define rULCON2     (*(volatile unsigned *)0x50008000)	//UART channel 2 Line control
#define rUCON2      (*(volatile unsigned *)0x50008004)	//UART channel 2 Control
#define rUFCON2     (*(volatile unsigned *)0x50008008)	//UART channel 2 FIFO control
#define rUTRSTAT2   (*(volatile unsigned *)0x50008010)	//UART channel 2 Tx/Rx status
#define rUERSTAT2   (*(volatile unsigned *)0x50008014)	//UART channel 2 Rx error status
#define rUFSTAT2    (*(volatile unsigned *)0x50008018)	//UART channel 2 FIFO status
#define rUBRDIV2    (*(volatile unsigned *)0x50008028)	//UART Baud rate divisor 2
#define rUDIVSLOT2  (*(volatile unsigned *)0x5000802c)	//UART Baud rate divisor 2

#ifdef __BIG_ENDIAN
#define rUTXH0      (*(volatile unsigned char *)0x50000023)	//UART channel 0 Transmit buffer
#define rURXH0      (*(volatile unsigned char *)0x50000027)	//UART channel 0 Receive buffer
#define rUTXH1      (*(volatile unsigned char *)0x50004023)	//UART channel 1 Transmit buffer
#define rURXH1      (*(volatile unsigned char *)0x50004027)	//UART channel 1 Receive buffer
#define rUTXH2      (*(volatile unsigned char *)0x50008023)	//UART channel 2 Transmit buffer
#define rURXH2      (*(volatile unsigned char *)0x50008027)	//UART channel 2 Receive buffer

#define WrUTXH0(ch) (*(volatile unsigned char *)0x50000023)=(unsigned char)(ch)
#define RdURXH0()   (*(volatile unsigned char *)0x50000027)
#define WrUTXH1(ch) (*(volatile unsigned char *)0x50004023)=(unsigned char)(ch)
#define RdURXH1()   (*(volatile unsigned char *)0x50004027)
#define WrUTXH2(ch) (*(volatile unsigned char *)0x50008023)=(unsigned char)(ch)
#define RdURXH2()   (*(volatile unsigned char *)0x50008027)


#else //Little Endian
#define rUTXH0 (*(volatile unsigned char *)0x50000020)	//UART channel 0 Transmit buffer
#define rURXH0 (*(volatile unsigned char *)0x50000024)	//UART channel 0 Receive buffer
#define rUTXH1 (*(volatile unsigned char *)0x50004020)	//UART channel 1 Transmit buffer
#define rURXH1 (*(volatile unsigned char *)0x50004024)	//UART channel 1 Receive buffer
#define rUTXH2 (*(volatile unsigned char *)0x50008020)	//UART 2channel 2 Transmit buffer
#define rURXH2 (*(volatile unsigned char *)0x50008024)	//UART channel 2 Receive buffer

#define WrUTXH0(ch) (*(volatile unsigned char *)0x50000020)=(unsigned char)(ch)
#define RdURXH0()   (*(volatile unsigned char *)0x50000024)
#define WrUTXH1(ch) (*(volatile unsigned char *)0x50004020)=(unsigned char)(ch)
#define RdURXH1()   (*(volatile unsigned char *)0x50004024)
#define WrUTXH2(ch) (*(volatile unsigned char *)0x50008020)=(unsigned char)(ch)
#define RdURXH2()   (*(volatile unsigned char *)0x50008024)


#endif	//UART

// chapter11 USB HOST CONTROLLER
#define rHcRevision            (*(volatile unsigned *)0x49000000)	//Control and status group
#define rHcControl             (*(volatile unsigned *)0x49000004)	//Control and status group
#define rHcCommonStatus        (*(volatile unsigned *)0x49000008)	//Control and status group
#define rHcInterruptStatus     (*(volatile unsigned *)0x4900000C)	//Control and status group
#define rHcInterruptEnable     (*(volatile unsigned *)0x49000010)	//Control and status group
#define rHcInterruptDisable    (*(volatile unsigned *)0x49000014)	//Control and status group
#define rHcHCCA                (*(volatile unsigned *)0x49000018)	//Memory pointer group
#define rHcPeridCuttentED      (*(volatile unsigned *)0x4900001C)	//Memory pointer group
#define rHcControlHeadED       (*(volatile unsigned *)0x49000020)	//Memory pointer group
#define rHcControlCurrentED    (*(volatile unsigned *)0x49000024)	//Memory pointer group
#define rHcBulkHeadED          (*(volatile unsigned *)0x49000028)	//Memory pointer group
#define rHcBulkCurrentED       (*(volatile unsigned *)0x4900002C)	//Memory pointer group
#define rHcDoneHead            (*(volatile unsigned *)0x49000030)	//Memory pointer group
#define rHcRmlnterval          (*(volatile unsigned *)0x49000034)	//frame counter group
#define rHcFmRemaining         (*(volatile unsigned *)0x49000038)	//frame counter group
#define rHcFmNumber            (*(volatile unsigned *)0x4900003C)	//frame counter group
#define rHcPeridicStart        (*(volatile unsigned *)0x49000040)	//frame counter group
#define rHcLSThreshold         (*(volatile unsigned *)0x49000044)	//frame counter group
#define rHcRhDescriptorA       (*(volatile unsigned *)0x49000048)	//Root hub group
#define rHcRhDescriptorB       (*(volatile unsigned *)0x4900004C)	//Root hub group
#define rHcRStatus             (*(volatile unsigned *)0x49000050)	//Root hub group
#define rHcRhPortStatus1       (*(volatile unsigned *)0x49000054)	//Root hub group
#define rHcRhPortStatus2       (*(volatile unsigned *)0x49000058)	//Root hub group


// chapter12 USB DEVICE
#ifdef __BIG_ENDIAN

#define rFUNC_ADDR_REG     (*(volatile unsigned char *)0x52000143)	//Function address
#define rPWR_REG           (*(volatile unsigned char *)0x52000147)	//Power management
#define rEP_INT_REG        (*(volatile unsigned char *)0x5200014b)	//EndPoint Interrupt 
#define rUSB_INT_REG       (*(volatile unsigned char *)0x5200015b)	//USB Interrupt 
#define rEP_INT_EN_REG     (*(volatile unsigned char *)0x5200015f)	//Endpoint Interrupt enable
#define rUSB_INT_EN_REG    (*(volatile unsigned char *)0x5200016f)  //USB Interrupt enable
#define rFRAME_NUM1_REG    (*(volatile unsigned char *)0x52000173)	//Frame number 1
#define rFRAME_NUM2_REG    (*(volatile unsigned char *)0x52000177)	//Frame number 2
#define rINDEX_REG         (*(volatile unsigned char *)0x5200017b)	//Register index
#define rMAXP_REG          (*(volatile unsigned char *)0x52000183)	//Endpoint max packet
#define rEP0_CSR           (*(volatile unsigned char *)0x52000187)	//Endpoint 0 status
#define rIN_CSR1_REG       (*(volatile unsigned char *)0x52000187)	//EP In control status 1/EP0 control status
#define rIN_CSR2_REG       (*(volatile unsigned char *)0x5200018b)  //EP In control status
#define rOUT_CSR1_REG      (*(volatile unsigned char *)0x52000193)	//EP Out control status 1
#define rOUT_CSR2_REG      (*(volatile unsigned char *)0x52000197)	//EP Out control status 2
#define rOUT_FIFO_CNT1_REG (*(volatile unsigned char *)0x5200019b)	//Endpoint out write count 1
#define rOUT_FIFO_CNT2_REG (*(volatile unsigned char *)0x5200019f)	//Endpoint out write count 2
#define rEP0_FIFO          (*(volatile unsigned char *)0x520001c3)	//Endpoint 0 FIFO
#define rEP1_FIFO          (*(volatile unsigned char *)0x520001c7)	//Endpoint 1 FIFO
#define rEP2_FIFO          (*(volatile unsigned char *)0x520001cb)	//Endpoint 2 FIFO
#define rEP3_FIFO          (*(volatile unsigned char *)0x520001cf)	//Endpoint 3 FIFO
#define rEP4_FIFO          (*(volatile unsigned char *)0x520001d3)	//Endpoint 4 FIFO
#define rEP1_DMA_CON       (*(volatile unsigned char *)0x52000203)	//EP1 DMA control
#define rEP1_DMA_UNIT      (*(volatile unsigned char *)0x52000207)	//EP1 DMA unit counter
#define rEP1_DMA_FIFO      (*(volatile unsigned char *)0x5200020b)	//EP1 DMA FIFO counter
#define rEP1_DMA_TTC_L     (*(volatile unsigned char *)0x5200020f)	//EP1 DMA Transfer counter low-byte
#define rEP1_DMA_TTC_M     (*(volatile unsigned char *)0x52000213)  //EP1 DMA Transfer counter middle-byte
#define rEP1_DMA_TTC_H     (*(volatile unsigned char *)0x52000217)  //EP1 DMA Transfer counter high-byte
#define rEP2_DMA_CON       (*(volatile unsigned char *)0x5200021b)	//EP2 DMA control
#define rEP2_DMA_UNIT      (*(volatile unsigned char *)0x5200021f)	//EP2 DMA unit counter
#define rEP2_DMA_FIFO      (*(volatile unsigned char *)0x52000223)	//EP2 DMA FIFO counter
#define rEP2_DMA_TTC_L     (*(volatile unsigned char *)0x52000227)	//EP2 DMA Transfer counter low-byte
#define rEP2_DMA_TTC_M     (*(volatile unsigned char *)0x5200022b)  //EP2 DMA Transfer counter middle-byte
#define rEP2_DMA_TTC_H     (*(volatile unsigned char *)0x5200022f)  //EP2 DMA Transfer counter high-byte
#define rEP3_DMA_CON       (*(volatile unsigned char *)0x52000243)	//EP3 DMA control
#define rEP3_DMA_UNIT      (*(volatile unsigned char *)0x52000247)	//EP3 DMA unit counter
#define rEP3_DMA_FIFO      (*(volatile unsigned char *)0x5200024b)	//EP3 DMA FIFO counter
#define rEP3_DMA_TTC_L     (*(volatile unsigned char *)0x5200024f)	//EP3 DMA Transfer counter low-byte
#define rEP3_DMA_TTC_M     (*(volatile unsigned char *)0x52000253)  //EP3 DMA Transfer counter middle-byte
#define rEP3_DMA_TTC_H     (*(volatile unsigned char *)0x52000257)  //EP3 DMA Transfer counter high-byte
#define rEP4_DMA_CON       (*(volatile unsigned char *)0x5200025b)	//EP4 DMA control
#define rEP4_DMA_UNIT      (*(volatile unsigned char *)0x5200025f)	//EP4 DMA unit counter
#define rEP4_DMA_FIFO      (*(volatile unsigned char *)0x52000263)	//EP4 DMA FIFO counter
#define rEP4_DMA_TTC_L     (*(volatile unsigned char *)0x52000267)	//EP4 DMA Transfer counter low-byte
#define rEP4_DMA_TTC_M     (*(volatile unsigned char *)0x5200026b)  //EP4 DMA Transfer counter middle-byte
#define rEP4_DMA_TTC_H     (*(volatile unsigned char *)0x5200026f)  //EP4 DMA Transfer counter high-byte

#else  // Little Endian
#define rFUNC_ADDR_REG     (*(volatile unsigned char *)0x52000140)	//Function address
#define rPWR_REG           (*(volatile unsigned char *)0x52000144)	//Power management
#define rEP_INT_REG        (*(volatile unsigned char *)0x52000148)	//EP Interrupt pending and clear
#define rUSB_INT_REG       (*(volatile unsigned char *)0x52000158)	//USB Interrupt pending and clear
#define rEP_INT_EN_REG     (*(volatile unsigned char *)0x5200015c)	//Interrupt enable
#define rUSB_INT_EN_REG    (*(volatile unsigned char *)0x5200016c)	//Interrupt enable
#define rFRAME_NUM1_REG    (*(volatile unsigned char *)0x52000170)	//Frame number lower byte
#define rFRAME_NUM2_REG    (*(volatile unsigned char *)0x52000174)	//Frame number higher byte
#define rINDEX_REG         (*(volatile unsigned char *)0x52000178)	//Register index
#define rMAXP_REG          (*(volatile unsigned char *)0x52000180)	//Endpoint max packet
#define rEP0_CSR           (*(volatile unsigned char *)0x52000184)	//Endpoint 0 status
#define rIN_CSR1_REG       (*(volatile unsigned char *)0x52000184)	//In endpoint control status 1
#define rIN_CSR2_REG       (*(volatile unsigned char *)0x52000188)	//In endpoint control status 2
#define rOUT_CSR1_REG      (*(volatile unsigned char *)0x52000190)	//Out endpoint control status 1
#define rOUT_CSR2_REG      (*(volatile unsigned char *)0x52000194)	//Out endpoint control status 2
#define rOUT_FIFO_CNT1_REG (*(volatile unsigned char *)0x52000198)	//Endpoint out write count 1
#define rOUT_FIFO_CNT2_REG (*(volatile unsigned char *)0x5200019c)	//Endpoint out write count 2
#define rEP0_FIFO          (*(volatile unsigned char *)0x520001c0)	//Endpoint 0 FIFO
#define rEP1_FIFO          (*(volatile unsigned char *)0x520001c4)	//Endpoint 1 FIFO
#define rEP2_FIFO          (*(volatile unsigned char *)0x520001c8)	//Endpoint 2 FIFO
#define rEP3_FIFO          (*(volatile unsigned char *)0x520001cc)	//Endpoint 3 FIFO
#define rEP4_FIFO          (*(volatile unsigned char *)0x520001d0)	//Endpoint 4 FIFO
#define rEP1_DMA_CON       (*(volatile unsigned char *)0x52000200)	//EP1 DMA interface control
#define rEP1_DMA_UNIT      (*(volatile unsigned char *)0x52000204)	//EP1 DMA transfer unit counter base 
#define rEP1_DMA_FIFO      (*(volatile unsigned char *)0x52000208)	//EP1 DMA transfer FIFO counter base
#define rEP1_DMA_TTC_L     (*(volatile unsigned char *)0x5200020c)	//EP1 DMA total transfer counter(lower byte)
#define rEP1_DMA_TTC_M     (*(volatile unsigned char *)0x52000210)	//EP1 DMA total transfer counter(middle byte)
#define rEP1_DMA_TTC_H     (*(volatile unsigned char *)0x52000214)	//EP1 DMA total transfer counter(high byte)
#define rEP2_DMA_CON       (*(volatile unsigned char *)0x52000218)	//EP2 DMA interface control
#define rEP2_DMA_UNIT      (*(volatile unsigned char *)0x5200021c)	//EP2 DMA transfer unit counter base 
#define rEP2_DMA_FIFO      (*(volatile unsigned char *)0x52000220)	//EP2 DMA transfer FIFO counter base
#define rEP2_DMA_TTC_L     (*(volatile unsigned char *)0x52000224)	//EP2 DMA total transfer counter(lower byte)
#define rEP2_DMA_TTC_M     (*(volatile unsigned char *)0x52000228)	//EP2 DMA total transfer counter(middle byte)
#define rEP2_DMA_TTC_H     (*(volatile unsigned char *)0x5200022c)	//EP2 DMA total transfer counter(high byte)
#define rEP3_DMA_CON       (*(volatile unsigned char *)0x52000240)	//EP3 DMA interface control
#define rEP3_DMA_UNIT      (*(volatile unsigned char *)0x52000244)	//EP3 DMA transfer unit counter base 
#define rEP3_DMA_FIFO      (*(volatile unsigned char *)0x52000248)	//EP3 DMA transfer FIFO counter base
#define rEP3_DMA_TTC_L     (*(volatile unsigned char *)0x5200024c)	//EP3 DMA total transfer counter(lower byte)
#define rEP3_DMA_TTC_M     (*(volatile unsigned char *)0x52000250)	//EP3 DMA total transfer counter(middle byte)
#define rEP3_DMA_TTC_H     (*(volatile unsigned char *)0x52000254)	//EP3 DMA total transfer counter(high byte)
#define rEP4_DMA_CON       (*(volatile unsigned char *)0x52000258)	//EP4 DMA interface control
#define rEP4_DMA_UNIT      (*(volatile unsigned char *)0x5200025c)	//EP4 DMA transfer unit counter base 
#define rEP4_DMA_FIFO      (*(volatile unsigned char *)0x52000260)	//EP4 DMA transfer FIFO counter base
#define rEP4_DMA_TTC_L     (*(volatile unsigned char *)0x52000264)	//EP4 DMA total transfer counter(lower byte)
#define rEP4_DMA_TTC_M     (*(volatile unsigned char *)0x52000268)	//EP4 DMA total transfer counter(middle byte)
#define rEP4_DMA_TTC_H     (*(volatile unsigned char *)0x5200026c)	//EP4 DMA total transfer counter(high byte)
#endif   // __BIG_ENDIAN


//chapter13 INTERRUPT 
#define rSRCPND     (*(volatile unsigned *)0x4a000000)	//Interrupt request status
#define rINTMOD     (*(volatile unsigned *)0x4a000004)	//Interrupt mode control
#define rINTMSK     (*(volatile unsigned *)0x4a000008)	//Interrupt mask control
#define rPRIORITY   (*(volatile unsigned *)0x4a00000c)	//IRQ priority control
#define rINTPND     (*(volatile unsigned *)0x4a000010)	//Interrupt request status
#define rINTOFFSET  (*(volatile unsigned *)0x4a000014)	//Interrupt request source offset
#define rSUBSRCPND  (*(volatile unsigned *)0x4a000018)	//Interrupt request status
#define rINTSUBMSK  (*(volatile unsigned *)0x4a00001c)	//Interrupt source mask


//chapter14 LCD CONTROLLER
#define rLCDCON1    (*(volatile unsigned *)0x4d000000)	//LCD control 1
#define rLCDCON2    (*(volatile unsigned *)0x4d000004)	//LCD control 2
#define rLCDCON3    (*(volatile unsigned *)0x4d000008)	//LCD control 3
#define rLCDCON4    (*(volatile unsigned *)0x4d00000c)	//LCD control 4
#define rLCDCON5    (*(volatile unsigned *)0x4d000010)	//LCD control 5
#define rLCDCON6    (*(volatile unsigned *)0x4d000034)	//LCD control 6
#define rLCDCON7    (*(volatile unsigned *)0x4d000038)	//LCD control 7
#define rLCDCON8    (*(volatile unsigned *)0x4d00003C)	//LCD control 8
#define rLCDCON9    (*(volatile unsigned *)0x4d000040)	//LCD control 9
#define rLCDSADDR1  (*(volatile unsigned *)0x4d000014)	//STN/TFT Frame buffer start address 1
#define rLCDSADDR2  (*(volatile unsigned *)0x4d000018)	//STN/TFT Frame buffer start address 2
#define rLCDSADDR3  (*(volatile unsigned *)0x4d00001c)	//STN/TFT Virtual screen address set
#define rTPAL       (*(volatile unsigned *)0x4d000020)	//TFT Temporary plette
//#define rGREENLUT   (*(volatile unsigned *)0x4d000024)	//STN Green lookup table 
//#define rBLUELUT    (*(volatile unsigned *)0x4d000028)	//STN Blue lookup table
//#define rDITHMODE   (*(volatile unsigned *)0x4d00004c)	//STN Dithering mode
//#define rTPAL       (*(volatile unsigned *)0x4d000050)	//TFT Temporary palette
#define rLCDINTPND  (*(volatile unsigned *)0x4d000024)	//LCD Interrupt pending
#define rLCDSRCPND  (*(volatile unsigned *)0x4d000028)	//LCD Interrupt source
#define rLCDINTMSK  (*(volatile unsigned *)0x4d00002c)	//LCD Interrupt mask
#define rTCONSEL    (*(volatile unsigned *)0x4d000030)	//LPC3600 Control --- edited by junon
#define rREDLUT0    (*(volatile unsigned *)0x4d000044)	//Red Lookup table[31:0]
#define rREDLUT1    (*(volatile unsigned *)0x4d000048)	//Red Lookup table[63:32]
#define rREDLUT2    (*(volatile unsigned *)0x4d00004C)	//Red Lookup table[95:64]
#define rREDLUT3    (*(volatile unsigned *)0x4d000050)	//Red Lookup table[127:96]
#define rREDLUT4    (*(volatile unsigned *)0x4d000054)	//Red Lookup table[159:128]
#define rREDLUT5    (*(volatile unsigned *)0x4d000058)	//Red Lookup table[191:160]
#define rREDLUT6    (*(volatile unsigned *)0x4d00005C)	//Red Lookup table[223:192]
#define rGREENLUT0  (*(volatile unsigned *)0x4d000060)	//GREEN Lookup table[31:0]
#define rGREENLUT1  (*(volatile unsigned *)0x4d000064)	//GREEN Lookup table[63:32]
#define rGREENLUT2  (*(volatile unsigned *)0x4d000068)	//GREEN Lookup table[95:64]
#define rGREENLUT3  (*(volatile unsigned *)0x4d00006C)	//GREEN Lookup table[127:96]
#define rGREENLUT4  (*(volatile unsigned *)0x4d000070)	//GREEN Lookup table[159:128]
#define rGREENLUT5  (*(volatile unsigned *)0x4d000074)	//GREEN Lookup table[191:160]
#define rGREENLUT6  (*(volatile unsigned *)0x4d000078)	//GREEN Lookup table[223:192]
#define rGREENLUT7  (*(volatile unsigned *)0x4d00007C)	//GREEN Lookup table[255:224]
#define rGREENLUT8  (*(volatile unsigned *)0x4d000080)	//GREEN Lookup table[287:256]
#define rGREENLUT9  (*(volatile unsigned *)0x4d000084)	//GREEN Lookup table[319:288]
#define rGREENLUT10 (*(volatile unsigned *)0x4d000088)	//GREEN Lookup table[351:320]
#define rGREENLUT11 (*(volatile unsigned *)0x4d00008C)	//GREEN Lookup table[383:352]
#define rGREENLUT12 (*(volatile unsigned *)0x4d000090)	//GREEN Lookup table[415:384]
#define rGREENLUT13 (*(volatile unsigned *)0x4d000094)	//GREEN Lookup table[447:416]
#define rBLUELUT0  (*(volatile unsigned *)0x4d000098)	//BLUE Lookup table[31:0]
#define rBLUELUT1  (*(volatile unsigned *)0x4d00009C)	//BLUE Lookup table[63:32]
#define rBLUELUT2  (*(volatile unsigned *)0x4d0000A0)	//BLUE Lookup table[95:64]
#define rBLUELUT3  (*(volatile unsigned *)0x4d0000A4)	//BLUE Lookup table[127:96]
#define rBLUELUT4  (*(volatile unsigned *)0x4d0000A8)	//BLUE Lookup table[159:128]
#define rBLUELUT5  (*(volatile unsigned *)0x4d0000AC)	//BLUE Lookup table[191:160]
#define rBLUELUT6  (*(volatile unsigned *)0x4d0000B0) 	//BLUE Lookup table[223:192]
#define rFRCPAT0    (*(volatile unsigned *)0x4d0000B4) 	//FRC Pattern
#define rFRCPAT1    (*(volatile unsigned *)0x4d0000B8) 	//FRC Pattern
#define rFRCPAT2    (*(volatile unsigned *)0x4d0000BC) 	//FRC Pattern
#define rFRCPAT3    (*(volatile unsigned *)0x4d0000C0) 	//FRC Pattern
#define rFRCPAT4    (*(volatile unsigned *)0x4d0000C4) 	//FRC Pattern
#define rFRCPAT5    (*(volatile unsigned *)0x4d0000C8) 	//FRC Pattern
#define rFRCPAT6    (*(volatile unsigned *)0x4d0000CC) 	//FRC Pattern
#define rFRCPAT7    (*(volatile unsigned *)0x4d0000D0) 	//FRC Pattern
#define rFRCPAT8    (*(volatile unsigned *)0x4d0000D4) 	//FRC Pattern
#define rFRCPAT9    (*(volatile unsigned *)0x4d0000D8) 	//FRC Pattern
#define rFRCPAT10   (*(volatile unsigned *)0x4d0000DC) 	//FRC Pattern
#define rFRCPAT11   (*(volatile unsigned *)0x4d0000E0) 	//FRC Pattern
#define rFRCPAT12   (*(volatile unsigned *)0x4d0000E4) 	//FRC Pattern
#define rFRCPAT13   (*(volatile unsigned *)0x4d0000E8) 	//FRC Pattern
#define rFRCPAT14   (*(volatile unsigned *)0x4d0000EC) 	//FRC Pattern
#define rFRCPAT15   (*(volatile unsigned *)0x4d0000F0) 	//FRC Pattern
#define rFRCPAT16   (*(volatile unsigned *)0x4d0000F4) 	//FRC Pattern
#define rFRCPAT17   (*(volatile unsigned *)0x4d0000F8) 	//FRC Pattern
#define rFRCPAT18   (*(volatile unsigned *)0x4d0000FC) 	//FRC Pattern
#define rFRCPAT19   (*(volatile unsigned *)0x4d000100) 	//FRC Pattern
#define rFRCPAT20   (*(volatile unsigned *)0x4d000104) 	//FRC Pattern
#define rFRCPAT21   (*(volatile unsigned *)0x4d000108) 	//FRC Pattern
#define rFRCPAT22   (*(volatile unsigned *)0x4d00010C) 	//FRC Pattern
#define rFRCPAT23   (*(volatile unsigned *)0x4d000110) 	//FRC Pattern
#define rFRCPAT24   (*(volatile unsigned *)0x4d000114) 	//FRC Pattern
#define rFRCPAT25   (*(volatile unsigned *)0x4d000118) 	//FRC Pattern
#define rFRCPAT26   (*(volatile unsigned *)0x4d00011C) 	//FRC Pattern
#define rFRCPAT27   (*(volatile unsigned *)0x4d000120) 	//FRC Pattern
#define rFRCPAT28   (*(volatile unsigned *)0x4d000124) 	//FRC Pattern
#define rFRCPAT29   (*(volatile unsigned *)0x4d000128) 	//FRC Pattern
#define rFRCPAT30   (*(volatile unsigned *)0x4d00012C) 	//FRC Pattern
#define rFRCPAT31   (*(volatile unsigned *)0x4d000130) 	//FRC Pattern
#define rFRCPAT32   (*(volatile unsigned *)0x4d000134) 	//FRC Pattern
#define rFRCPAT33   (*(volatile unsigned *)0x4d000138) 	//FRC Pattern
#define rFRCPAT34   (*(volatile unsigned *)0x4d00013C) 	//FRC Pattern
#define rFRCPAT35   (*(volatile unsigned *)0x4d000140) 	//FRC Pattern
#define rFRCPAT36   (*(volatile unsigned *)0x4d000144) 	//FRC Pattern
#define rFRCPAT37   (*(volatile unsigned *)0x4d000148) 	//FRC Pattern
#define rFRCPAT38   (*(volatile unsigned *)0x4d00014C) 	//FRC Pattern
#define rFRCPAT39   (*(volatile unsigned *)0x4d000150) 	//FRC Pattern
#define rFRCPAT40   (*(volatile unsigned *)0x4d000154) 	//FRC Pattern
#define rFRCPAT41   (*(volatile unsigned *)0x4d000158) 	//FRC Pattern
#define rFRCPAT42   (*(volatile unsigned *)0x4d00015C) 	//FRC Pattern
#define rFRCPAT43   (*(volatile unsigned *)0x4d000160) 	//FRC Pattern
#define rFRCPAT44   (*(volatile unsigned *)0x4d000164) 	//FRC Pattern
#define rFRCPAT45   (*(volatile unsigned *)0x4d000168) 	//FRC Pattern
#define rFRCPAT46   (*(volatile unsigned *)0x4d00016C) 	//FRC Pattern
#define rFRCPAT47   (*(volatile unsigned *)0x4d000170) 	//FRC Pattern
#define rFRCPAT48   (*(volatile unsigned *)0x4d000174) 	//FRC Pattern
#define rFRCPAT49   (*(volatile unsigned *)0x4d000178) 	//FRC Pattern
#define rFRCPAT50   (*(volatile unsigned *)0x4d00017C) 	//FRC Pattern
#define rFRCPAT51   (*(volatile unsigned *)0x4d000180) 	//FRC Pattern
#define rFRCPAT52   (*(volatile unsigned *)0x4d000184) 	//FRC Pattern
#define rFRCPAT53   (*(volatile unsigned *)0x4d000188) 	//FRC Pattern
#define rFRCPAT54   (*(volatile unsigned *)0x4d00018C) 	//FRC Pattern
#define rFRCPAT55   (*(volatile unsigned *)0x4d000190) 	//FRC Pattern
#define rFRCPAT56   (*(volatile unsigned *)0x4d000194) 	//FRC Pattern
#define rFRCPAT57   (*(volatile unsigned *)0x4d000198) 	//FRC Pattern
#define rFRCPAT58   (*(volatile unsigned *)0x4d00019C) 	//FRC Pattern
#define rFRCPAT59   (*(volatile unsigned *)0x4d0001A0) 	//FRC Pattern
#define rFRCPAT60   (*(volatile unsigned *)0x4d0001A4) 	//FRC Pattern
#define rFRCPAT61   (*(volatile unsigned *)0x4d0001A8) 	//FRC Pattern
#define rFRCPAT62   (*(volatile unsigned *)0x4d0001AC) 	//FRC Pattern
#define rFRCPAT63   (*(volatile unsigned *)0x4d0001B0) 	//FRC Pattern
#define rLCDTEST    (*(volatile unsigned *)0x4d0001B4) 	//FRC Pattern


// chapter15 ADC
#define rADCCON    (*(volatile unsigned *)0x58000000)	//ADC control
#define rADCTSC    (*(volatile unsigned *)0x58000004)	//ADC touch screen control
#define rADCDLY    (*(volatile unsigned *)0x58000008)	//ADC start or Interval Delay
#define rADCDAT0   (*(volatile unsigned *)0x5800000c)	//ADC conversion data 0
#define rADCDAT1   (*(volatile unsigned *)0x58000010)	//ADC conversion data 1
//#define rADCUPDN   (*(volatile unsigned *)0x58000014)	//Stylus Up/Down interrupt status	// Absence in 2413

// chapter16 RTC
#ifdef __BIG_ENDIAN
#define rRTCCON    (*(volatile unsigned char *)0x57000043)	//RTC control
#define rTICNT0    (*(volatile unsigned char *)0x57000047)	//Tick time count register 0
#define rTICNT1    (*(volatile unsigned char *)0x5700004F)	//Tick time count register 1
#define rRTCALM    (*(volatile unsigned char *)0x57000053)	//RTC alarm control
#define rALMSEC    (*(volatile unsigned char *)0x57000057)	//Alarm second
#define rALMMIN    (*(volatile unsigned char *)0x5700005b)	//Alarm minute
#define rALMHOUR   (*(volatile unsigned char *)0x5700005f)	//Alarm Hour
#define rALMDATE   (*(volatile unsigned char *)0x57000063)	//Alarm date   //edited by junon
#define rALMMON    (*(volatile unsigned char *)0x57000067)	//Alarm month
#define rALMYEAR   (*(volatile unsigned char *)0x5700006b)	//Alarm year
#define rRTCRST    (*(volatile unsigned char *)0x5700006f)	//RTC round reset
#define rBCDSEC    (*(volatile unsigned char *)0x57000073)	//BCD second
#define rBCDMIN    (*(volatile unsigned char *)0x57000077)	//BCD minute
#define rBCDHOUR   (*(volatile unsigned char *)0x5700007b)	//BCD hour
#define rBCDDATE   (*(volatile unsigned char *)0x5700007f)	//BCD date  //edited by junon
#define rBCDDAY    (*(volatile unsigned char *)0x57000083)	//BCD day   //edited by junon
#define rBCDMON    (*(volatile unsigned char *)0x57000087)	//BCD month
#define rBCDYEAR   (*(volatile unsigned char *)0x5700008b)	//BCD year

#else //Little Endian
#define rRTCCON    (*(volatile unsigned char *)0x57000040)	//RTC control
#define rTICNT0    (*(volatile unsigned char *)0x57000044)	//Tick time count register 0
#define rTICNT1    (*(volatile unsigned char *)0x5700004c)	//Tick time count register 1
#define rRTCALM    (*(volatile unsigned char *)0x57000050)	//RTC alarm control
#define rALMSEC    (*(volatile unsigned char *)0x57000054)	//Alarm second
#define rALMMIN    (*(volatile unsigned char *)0x57000058)	//Alarm minute
#define rALMHOUR   (*(volatile unsigned char *)0x5700005c)	//Alarm Hour
#define rALMDATE   (*(volatile unsigned char *)0x57000060)	//Alarm date  // edited by junon
#define rALMMON    (*(volatile unsigned char *)0x57000064)	//Alarm month
#define rALMYEAR   (*(volatile unsigned char *)0x57000068)	//Alarm year
#define rRTCRST    (*(volatile unsigned char *)0x5700006c)	//RTC round reset
#define rBCDSEC    (*(volatile unsigned char *)0x57000070)	//BCD second
#define rBCDMIN    (*(volatile unsigned char *)0x57000074)	//BCD minute
#define rBCDHOUR   (*(volatile unsigned char *)0x57000078)	//BCD hour
#define rBCDDATE   (*(volatile unsigned char *)0x5700007c)	//BCD date  //edited by junon
#define rBCDDAY    (*(volatile unsigned char *)0x57000080)	//BCD day   //edited by junon
#define rBCDMON    (*(volatile unsigned char *)0x57000084)	//BCD month
#define rBCDYEAR   (*(volatile unsigned char *)0x57000088)	//BCD year
#endif  //RTC

// chapter17 WATCH DOG TIMER
#define rWTCON   (*(volatile unsigned *)0x53000000)	//Watch-dog timer mode
#define rWTDAT   (*(volatile unsigned *)0x53000004)	//Watch-dog timer data
#define rWTCNT   (*(volatile unsigned *)0x53000008)	//Watch-dog timer count



//chapter18 MMC/SD/SDI CONTROLLER

//chapter18 MMC/SD/SDI CONTROLLER
#define rSDICON     (*(volatile unsigned *)0x5A000000)	//SDI Control
#define rSDIPRE     (*(volatile unsigned *)0x5A000004)	//SDI baud rate prescaler
#define rSDICmdARG  (*(volatile unsigned *)0x5A000008)	//SDI command argument
#define rSDICmdCON  (*(volatile unsigned *)0x5A00000c)	//SDI command control
#define rSDICmdSTA  (*(volatile unsigned *)0x5A000010)	//SDI command status
#define rSDIRSP0    (*(volatile unsigned *)0x5A000014)	//SDI response 0
#define rSDIRSP1    (*(volatile unsigned *)0x5A000018)	//SDI response 1
#define rSDIRSP2    (*(volatile unsigned *)0x5A00001c)	//SDI response 2
#define rSDIRSP3    (*(volatile unsigned *)0x5A000020)	//SDI response 3
#define rSDIDTIMER  (*(volatile unsigned *)0x5A000024)	//SDI data/busy timer
#define rSDIBSIZE   (*(volatile unsigned *)0x5A000028)	//SDI block size
#define rSDIDCON    (*(volatile unsigned *)0x5A00002c)	//SDI data control
#define rSDIDCNT    (*(volatile unsigned *)0x5A000030)	//SDI data remain counter
#define rSDIDSTA    (*(volatile unsigned *)0x5A000034)	//SDI data status
#define rSDIFSTA    (*(volatile unsigned *)0x5A000038)	//SDI FIFO status
#define rSDIINTMSK  (*(volatile unsigned *)0x5A00003c)	//SDI Interrupt Mask

#ifdef __BIG_ENDIAN  /* edited for 2413 */
#define rSDIDAT    (*(volatile unsigned *)0x5A00004c)	//SDI data
#define SDIDAT     0x5A00004c  
#else  // Little Endian
#define rSDIDAT    (*(volatile unsigned *)0x5A000040)	//SDI data 
#define SDIDAT     0x5A000040  
#endif   //SD Interface


//chapter19 IIC
#define rIICCON		(*(volatile unsigned *)0x54000000)	//IIC control
#define rIICSTAT	(*(volatile unsigned *)0x54000004)	//IIC control/status
#define rIICADD		(*(volatile unsigned *)0x54000008)	//IIC address
#define rIICDS		(*(volatile unsigned *)0x5400000c)	//IIC transmit/receive data shift
#define rIICLC		(*(volatile unsigned *)0x54000010)	//IIC multi-master line control

//chapter20 IIS
#define rIISCON  (*(volatile unsigned *)0x55000000)	//IIS Control
#define rIISMOD  (*(volatile unsigned *)0x55000004)	//IIS Mode
#define rIISFIC  (*(volatile unsigned *)0x55000008)	//IIS FIFO control
#define rIISPSR  (*(volatile unsigned *)0x5500000c)	//IIS clock divider control
#define rIISTXD  (*(volatile unsigned *)0x55000010)	//IIS tracsmit data
#define rIISRXD  (*(volatile unsigned *)0x55000014)	//IIS recelve data

//chapter21 SPI       
#define rSPCON0    (*(volatile unsigned *)0x59000000)	//SPI0 control
#define rSPSTA0    (*(volatile unsigned *)0x59000004)	//SPI0 status
#define rSPPIN0    (*(volatile unsigned *)0x59000008)	//SPI0 pin control
#define rSPPRE0    (*(volatile unsigned *)0x5900000c)	//SPI0 baud rate prescaler
#define rSPTDAT0   (*(volatile unsigned *)0x59000010)	//SPI0 Tx data
#define rSPRDAT0   (*(volatile unsigned *)0x59000014)	//SPI0 Rx data
#define rSPTXFIFO0 (*(volatile unsigned *)0x59000018)	//SPI0 Tx FIFO
#define rSPRXFIFO0 (*(volatile unsigned *)0x5900001C)	//SPI0 Rx FIFO
#define rSPRDATB0 (*(volatile unsigned *)0x59000020)	//SPI0 Rx Data
#define rSPFIC0    (*(volatile unsigned *)0x59000024)	//SPI0 Rx FIFO Interrupt and DMA control

#define WrSPTDAT0(ch) (*(volatile unsigned char*)0x59000010)=(unsigned char)(ch)
#define RdSPRDAT0()   (*(volatile unsigned char*)0x59000014)
#define RdSPRDATB0()   (*(volatile unsigned char*)0x59000020)
#define WrSPTXFIFO0(ch) (*(volatile unsigned char*)0x59000018)=(unsigned char)(ch)
#define RdSPRXFIFO0()   (*(volatile unsigned char*)0x5900001C)

#define rSPCON1    (*(volatile unsigned *)0x59000100)	//SPI1 control
#define rSPSTA1    (*(volatile unsigned *)0x59000104)	//SPI1 status
#define rSPPIN1    (*(volatile unsigned *)0x59000108)	//SPI1 pin control
#define rSPPRE1    (*(volatile unsigned *)0x5900010c)	//SPI1 baud rate prescaler
#define rSPTDAT1   (*(volatile unsigned *)0x59000110)	//SPI1 Tx data
#define rSPRDAT1   (*(volatile unsigned *)0x59000114)	//SPI1 Rx data
#define rSPTXFIFO1 (*(volatile unsigned *)0x59000118)	//SPI1 Tx FIFO
#define rSPRXFIFO1 (*(volatile unsigned *)0x5900011C)	//SPI1 Rx FIFO
#define rSPRDATB1 (*(volatile unsigned *)0x59000120)	//SPI1 Rx Data
#define rSPFIC1    (*(volatile unsigned *)0x59000124)	//SPI1 Rx FIFO Interrupt and DMA control

#define WrSPTDAT1(ch) (*(volatile unsigned char*)0x59000110)=(unsigned char)(ch)
#define RdSPRDAT1()   (*(volatile unsigned char*)0x59000114)
#define RdSPRDATB1()   (*(volatile unsigned char*)0x59000120)
#define WrSPTXFIFO1(ch) (*(volatile unsigned char*)0x59000118)=(unsigned char)(ch)
#define RdSPRXFIFO1()   (*(volatile unsigned char*)0x5900011C)

//chapter22 Camera Interface                              
#define rCISRCFMT           (*(volatile unsigned *)0x4D800000) //Input Source Format        
#define rCIWDOFST           (*(volatile unsigned *)0x4D800004) //Window offset       
#define rCIGCTRL            (*(volatile unsigned *)0x4D800008) //Global control        
#define rCIFCTRL1           (*(volatile unsigned *)0x4D80000C) //flash control 1
#define rCIFCTRL2           (*(volatile unsigned *)0x4D800010) //flash control 2
#define rCIDOWSFT2          (*(volatile unsigned *)0x4D800014) //Window option 2
#define rCICOYSA1           (*(volatile unsigned *)0x4D800018) //Y1 frame start address for codec DMA      
#define rCICOYSA2           (*(volatile unsigned *)0x4D80001C) //Y2 frame start address for codec DMA       
#define rCICOYSA3           (*(volatile unsigned *)0x4D800020) //Y3 frame start address for codec DMA        
#define rCICOYSA4           (*(volatile unsigned *)0x4D800024) //Y4 frame start address for codec DMA          
#define rCICOCBSA1          (*(volatile unsigned *)0x4D800028) //Cb1 frame start address for codec DMA 
#define rCICOCBSA2          (*(volatile unsigned *)0x4D80002C) //Cb2 frame start address for codec DMA        
#define rCICOCBSA3          (*(volatile unsigned *)0x4D800030) //Cb3 frame start address for codec DMA           
#define rCICOCBSA4          (*(volatile unsigned *)0x4D800034) //Cb4 frame start address for codec DMA   
#define rCICOCRSA1          (*(volatile unsigned *)0x4D800038) //Cr1 frame start address for codec DMA
#define rCICOCRSA2          (*(volatile unsigned *)0x4D80003C) //Cr2 frame start address for codec DMA
#define rCICOCRSA3          (*(volatile unsigned *)0x4D800040) //Cr3 frame start address for codec DMA
#define rCICOCRSA4          (*(volatile unsigned *)0x4D800044) //Cr4 frame start address for codec DMA
#define rCICOTRGFMT         (*(volatile unsigned *)0x4D800048) //Target image format of codex DMA
#define rCICOCTRL           (*(volatile unsigned *)0x4D80004C) //Codec DMA comtrol        
#define rCICOSCPRERATIO     (*(volatile unsigned *)0x4D800050) //Codec pre-scaler ratio control      
#define rCICOSCPREDST       (*(volatile unsigned *)0x4D800054) //Codec pre-scaler desitination format
#define rCICOSCCTRL         (*(volatile unsigned *)0x4D800058) //Codec main-scaler control
#define rCICOTAREA          (*(volatile unsigned *)0x4D80005C) //Codec pre-scaler desination format
#define rCICOSTATUS         (*(volatile unsigned *)0x4D800064) //Codec path status
#define rCIIMGCPT           (*(volatile unsigned *)0x4D8000A0) //Imahe capture enable command
#define rCICOCPTSEQ         (*(volatile unsigned *)0x4D8000A4) //Codec dma capture sequence related
#define rCICOSCOS           (*(volatile unsigned *)0x4D8000A8) //Codec scan line offset related
#define rCIIMGEFF           (*(volatile unsigned *)0x4D8000B0) //Imahe Effects related


//ATA register base
#define ATA_BASE           0x4b800000

/*--------------------------------------------------------------*/
/*	ATA Register						                        */
/*--------------------------------------------------------------*/
#define ATA_CONTROL        (*(volatile unsigned *)(ATA_BASE + 0x00))	//ATA0 enable and clock down status
#define ATA_STATUS         (*(volatile unsigned *)(ATA_BASE + 0x04))	//ATA0 status
#define ATA_COMMAND        (*(volatile unsigned *)(ATA_BASE + 0x08))	//ATA0 command
#define ATA_SWRST          (*(volatile unsigned *)(ATA_BASE + 0x0C))	//ATA0 software reset                  
#define ATA_IRQ            (*(volatile unsigned *)(ATA_BASE + 0x10))	//ATA0 interrupt sources
#define ATA_IRQ_MASK       (*(volatile unsigned *)(ATA_BASE + 0x14))	//ATA0 interrupt mask
#define ATA_CFG            (*(volatile unsigned *)(ATA_BASE + 0x18))	//ATA0 configuration for ATA interface               

#define ATA_PIO_TIME       (*(volatile unsigned *)(ATA_BASE + 0x2C))	//ATA0 PIO timing                                    
#define ATA_UDMA_TIME      (*(volatile unsigned *)(ATA_BASE + 0x30))	//ATA0 UDMA timing                                   
#define ATA_XFR_NUM        (*(volatile unsigned *)(ATA_BASE + 0x34))	//ATA0 transfer number                               
#define ATA_XFR_CNT        (*(volatile unsigned *)(ATA_BASE + 0x38))	//ATA0 current transfer count                        
#define ATA_TBUF_START     (*(volatile unsigned *)(ATA_BASE + 0x3C))	//ATA0 start address of track buffer                 
#define ATA_TBUF_SIZE      (*(volatile unsigned *)(ATA_BASE + 0x40))	//ATA0 size of track buffer                          
#define ATA_SBUF_START     (*(volatile unsigned *)(ATA_BASE + 0x44))	//ATA0 start address of Source buffer1               
#define ATA_SBUF_SIZE      (*(volatile unsigned *)(ATA_BASE + 0x48))	//ATA0 size of source buffer1                        
#define ATA_CADR_TBUF      (*(volatile unsigned *)(ATA_BASE + 0x4C))	//ATA0 current write address of track buffer         
#define ATA_CADR_SBUF      (*(volatile unsigned *)(ATA_BASE + 0x50))	//ATA0 current read address of source buffer         
#define ATA_PIO_DTR        (*(volatile unsigned *)(ATA_BASE + 0x54))	//ATA0 PIO device data register                      
#define ATA_PIO_FED        (*(volatile unsigned *)(ATA_BASE + 0x58))	//ATA0 PIO device Feature/Error register             
#define ATA_PIO_SCR        (*(volatile unsigned *)(ATA_BASE + 0x5C))	//ATA0 PIO sector count register                     
#define ATA_PIO_LLR        (*(volatile unsigned *)(ATA_BASE + 0x60))	//ATA0 PIO device LBA low register                   
#define ATA_PIO_LMR        (*(volatile unsigned *)(ATA_BASE + 0x64))	//ATA0 PIO device LBA middle register                
#define ATA_PIO_LHR        (*(volatile unsigned *)(ATA_BASE + 0x68))	//ATA0 PIO device LBA high register                  
#define ATA_PIO_DVR        (*(volatile unsigned *)(ATA_BASE + 0x6C))	//ATA0 PIO device register                           
#define ATA_PIO_CSD        (*(volatile unsigned *)(ATA_BASE + 0x70))	//ATA0 PIO device command/status register            
#define ATA_PIO_DAD        (*(volatile unsigned *)(ATA_BASE + 0x74))	//ATA0 PIO device control/alternate status register  
#define ATA_PIO_READY      (*(volatile unsigned *)(ATA_BASE + 0x78))	//ATA0 PIO data read/write ready                     
#define ATA_PIO_RDATA      (*(volatile unsigned *)(ATA_BASE + 0x7C))	//ATA0 PIO read data from device data register       
#define BUS_FIFO_STATUS    (*(volatile unsigned *)(ATA_BASE + 0x90))	//Internal AHBP fifo status                  
#define ATA_FIFO_STATUS    (*(volatile unsigned *)(ATA_BASE + 0x94))	//Internal ATA0  fifo status                 

/*=========================================================================
 *          	          ata Register Address
 *=========================================================================
 */

#define DEV_ERROR			(*(volatile unsigned *)(ATA_BASE + 0x58))
#define DEV_FEATURE		(*(volatile unsigned *)(ATA_BASE + 0x58))
#define DEV_SECTOR			(*(volatile unsigned *)(ATA_BASE + 0x5c))
#define DEV_LOWLBA			(*(volatile unsigned *)(ATA_BASE + 0x60))
#define DEV_MIDLBA			(*(volatile unsigned *)(ATA_BASE + 0x64))
#define DEV_HIGHLBA		(*(volatile unsigned *)(ATA_BASE + 0x68))
#define DEV_DEVICE			(*(volatile unsigned *)(ATA_BASE + 0x6c))
#define DEV_STATUS			(*(volatile unsigned *)(ATA_BASE + 0x70))
#define DEV_COMMAND		(*(volatile unsigned *)(ATA_BASE + 0x70))
#define DEV_ALTANATE		(*(volatile unsigned *)(ATA_BASE + 0x74))
#define DEV_CONTROL		(*(volatile unsigned *)(ATA_BASE + 0x74))

//Exception vector
#define pISR_RESET     			(*(unsigned *)(_ISR_STARTADDRESS+0x0))
#define pISR_UNDEF     			(*(unsigned *)(_ISR_STARTADDRESS+0x4))
#define pISR_SWI       			(*(unsigned *)(_ISR_STARTADDRESS+0x8))
#define pISR_PABORT    			(*(unsigned *)(_ISR_STARTADDRESS+0xc))
#define pISR_DABORT    			(*(unsigned *)(_ISR_STARTADDRESS+0x10))
#define pISR_RESERVED  			(*(unsigned *)(_ISR_STARTADDRESS+0x14))
#define pISR_IRQ       			(*(unsigned *)(_ISR_STARTADDRESS+0x18))
#define pISR_FIQ       			(*(unsigned *)(_ISR_STARTADDRESS+0x1c))

// Interrupt vector
#define pISR_EINT0		(*(unsigned *)(_ISR_STARTADDRESS+0x20))
#define pISR_EINT1		(*(unsigned *)(_ISR_STARTADDRESS+0x24))
#define pISR_EINT2		(*(unsigned *)(_ISR_STARTADDRESS+0x28))
#define pISR_EINT3		(*(unsigned *)(_ISR_STARTADDRESS+0x2c))
#define pISR_EINT4_7	(*(unsigned *)(_ISR_STARTADDRESS+0x30))
#define pISR_EINT8_23	(*(unsigned *)(_ISR_STARTADDRESS+0x34))
#define pISR_CAM		(*(unsigned *)(_ISR_STARTADDRESS+0x38))		// Added for 2442.
#define pISR_BAT_FLT	(*(unsigned *)(_ISR_STARTADDRESS+0x3c))
#define pISR_TICK		(*(unsigned *)(_ISR_STARTADDRESS+0x40))
#define pISR_WDT		(*(unsigned *)(_ISR_STARTADDRESS+0x44))   //Changed to pISR_WDT_AC97 for 2442A 
#define pISR_TIMER0	 	(*(unsigned *)(_ISR_STARTADDRESS+0x48))
#define pISR_TIMER1	 	(*(unsigned *)(_ISR_STARTADDRESS+0x4c))
#define pISR_TIMER2		(*(unsigned *)(_ISR_STARTADDRESS+0x50))
#define pISR_TIMER3		(*(unsigned *)(_ISR_STARTADDRESS+0x54))
#define pISR_TIMER4		(*(unsigned *)(_ISR_STARTADDRESS+0x58))
#define pISR_UART2		(*(unsigned *)(_ISR_STARTADDRESS+0x5c))
#define pISR_LCD		(*(unsigned *)(_ISR_STARTADDRESS+0x60))
#define pISR_DMA0		(*(unsigned *)(_ISR_STARTADDRESS+0x64))
#define pISR_DMA1		(*(unsigned *)(_ISR_STARTADDRESS+0x68))
#define pISR_DMA2		(*(unsigned *)(_ISR_STARTADDRESS+0x6c))
#define pISR_DMA3		(*(unsigned *)(_ISR_STARTADDRESS+0x70))
#define pISR_SDI		(*(unsigned *)(_ISR_STARTADDRESS+0x74))
#define pISR_SPI0		(*(unsigned *)(_ISR_STARTADDRESS+0x78))
#define pISR_UART1		(*(unsigned *)(_ISR_STARTADDRESS+0x7c))
#define pISR_NFCON		(*(unsigned *)(_ISR_STARTADDRESS+0x80))		// Added for 2442.
#define pISR_USBD		(*(unsigned *)(_ISR_STARTADDRESS+0x84))
#define pISR_USBH		(*(unsigned *)(_ISR_STARTADDRESS+0x88))
#define pISR_IIC		(*(unsigned *)(_ISR_STARTADDRESS+0x8c))
#define pISR_UART0		(*(unsigned *)(_ISR_STARTADDRESS+0x90))
#define pISR_SPI1		(*(unsigned *)(_ISR_STARTADDRESS+0x94))
#define pISR_RTC		(*(unsigned *)(_ISR_STARTADDRESS+0x98))
#define pISR_ADC		(*(unsigned *)(_ISR_STARTADDRESS+0x9c))


// PENDING BIT
#define BIT_EINT0		(0x1)
#define BIT_EINT1		(0x1<<1)
#define BIT_EINT2		(0x1<<2)
#define BIT_EINT3		(0x1<<3)
#define BIT_EINT4_7		(0x1<<4)
#define BIT_EINT8_23	(0x1<<5)
#define BIT_CAM			(0x1<<6)		
#define BIT_BAT_FLT		(0x1<<7)
#define BIT_TICK		(0x1<<8)
#define BIT_WDT			(0x1<<9)
#define BIT_TIMER0		(0x1<<10)
#define BIT_TIMER1		(0x1<<11)
#define BIT_TIMER2		(0x1<<12)
#define BIT_TIMER3		(0x1<<13)
#define BIT_TIMER4		(0x1<<14)
#define BIT_UART2		(0x1<<15)
#define BIT_LCD			(0x1<<16)
#define BIT_DMA0		(0x1<<17)
#define BIT_DMA1		(0x1<<18)
#define BIT_DMA2		(0x1<<19)
#define BIT_DMA3		(0x1<<20)
#define BIT_SDI			(0x1<<21)
#define BIT_SPI0		(0x1<<22)
#define BIT_UART1		(0x1<<23)
#define BIT_NFCON		(0x1<<24)	
#define BIT_USBD		(0x1<<25)
#define BIT_USBH		(0x1<<26)
#define BIT_IIC			(0x1<<27)
#define BIT_UART0		(0x1<<28)
#define BIT_SPI1		(0x1<<29)
#define BIT_RTC			(0x1<<30)
#define BIT_ADC			(0x1<<31)
#define BIT_ALLMSK		(0xffffffff)

#define BIT_SUB_ALLMSK	(0x7fff)			//Changed from 0x7ff to 0x7fff for 2442A 
#define BIT_SUB_CF		(0x1<<14)
#define BIT_SUB_SDI		(0x1<<13)
#define BIT_SUB_ADC		(0x1<<10)
#define BIT_SUB_TC		(0x1<<9)
#define BIT_SUB_ERR2	(0x1<<8)
#define BIT_SUB_TXD2	(0x1<<7)
#define BIT_SUB_RXD2	(0x1<<6)
#define BIT_SUB_ERR1	(0x1<<5)
#define BIT_SUB_TXD1	(0x1<<4)
#define BIT_SUB_RXD1	(0x1<<3)
#define BIT_SUB_ERR0	(0x1<<2)
#define BIT_SUB_TXD0	(0x1<<1)
#define BIT_SUB_RXD0	(0x1<<0)

#define ClearPending(bit) rSRCPND = bit;rINTPND = bit;



#define S3C2413_UART_CHANNELS	3
#define S3C2413_SPI_CHANNELS	2





                                                            
#endif 






