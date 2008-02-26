#ifndef _HEAD_REGS_H_
#define _HEAD_REGS_H_

#define INT_BASE_ADDR		(0x40200000)
#define oINTMOD				(0x04)
#define oINTMSK				(0x08)
#define oINTSUBMSK			(0x1C)

#define LCD_BASE_ADDR		(0x4D000000)
#define oLCDINTMSK			(0x2C)

#define CLK_BASE_ADDR		(0x4C000000)
#define oLOCKTIME			(0x00)
#define oMPLLCON			(0x04)
#define oUPLLCON			(0x08)
#define oCLKCON				(0x0C)
#define oCLKDIV				(0x14)
#define oCLKSRC				(0x1C)

#define SDRAM_BASE_ADDR		(0x48000000)
#define oBANKCFG			(0x00)
#define oBANKCON1			(0x04)
#define oBANKCON2			(0x08)
#define oBANKCON3			(0x0C)
#define oREFRESH			(0x10)
#define oTIMEOUT			(0x14)

#define EBI_CTRL_ADDR		(0x48800000)
#define oBANK_CFG			(0x04)

#define UART_BASE_ADDR		(0x50000000)
#define oULCON				(0x00)
#define oUCON				(0x04)
#define oUFCON				(0x08)
#define oUMCON				(0x0C)
#define oUTRSTAT			(0x120)
#define oUERSTAT			(0x14)
#define oUFSTAT				(0x18)
#define oUMSTAT				(0x1C)
#define oUTXH				(0x20)
#define oURXH				(0x24)
#define oUBRDIV				(0x28)

#define WT_CTRL_BASE_ADDR	(0x53000000)

#define GPIO_BASE_ADDR		(0x56000000)
#define oGPACON          	(0x00)
#define oGPBCON          	(0x10)
#define	oGPBDAT				(0x14)
#define oGPFCON          	(0x50)
#define oGPFDAT          	(0x54)
#define oGPFDN           	(0x58)
#define oGPHCON          	(0x70)

#endif /* _HEAD_REGS_H_ */
