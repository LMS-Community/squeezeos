#ifndef _MMC_2413_H_
#define _MMC_2413_H_

// MMC_FLASH Functions

unsigned int MMC_Command_Exec (unsigned char cmd, unsigned long argument,
                           unsigned char *pchar);

void MMC_FLASH_Init (void);            // Initializes MMC and configures it to 
                                       // accept SPI commands;



unsigned int MMC_FLASH_Block_Read(unsigned long address, unsigned char *pchar);
unsigned char MMC_FLASH_Block_Write(unsigned long address,unsigned char *wdata);


#endif
