
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

// Constants that define available card sizes, 8MB through 128MB
#define PS_8MB       8388608
#define PS_16MB      16777216
#define PS_32MB      33554432
#define PS_64MB      67108864
#define PS_128MB     134217728

#define ERROR_CODE	0xFFFF
#define BUFFER_SIZE 16

// Physical size in bytes of one MMC FLASH sector
#define PHYSICAL_BLOCK_SIZE     512

//#define NSSMD0 SLVSEL

// Erase group size = 16 MMC FLASH sectors
#define PHYSICAL_GROUP_SIZE     (PHYSICAL_BLOCK_SIZE * 16)

// Command table value definitions
// Used in the MMC_Command_Exec function to
// decode and execute MMC command requests
#define     EMPTY  0
#define     YES   1
#define     NO    0
#define     CMD   0
#define     RD    1
#define     WR    2
#define     R1    0
#define     R1b   1
#define     R2    2
#define     R3    3

// Start and stop data tokens for single and multiple
// block MMC data operations
#define     START_SBR      0xFE
#define     START_MBR      0xFE
#define     START_SBW      0xFE
#define     START_MBW      0xFC
#define     STOP_MBW       0xFD

// Mask for data response token after an MMC write
#define     DATA_RESP_MASK 0x11

// Mask for busy token in R1b response
#define     BUSY_BIT       0x80

#define DELAY(x)  {int i; for (i=0;i<x;i++);}

// Command Table Index Constants:
// Definitions for each table entry in the command table.
// These allow the MMC_Command_Exec function to be called with a
// meaningful parameter rather than a number.
#define     GO_IDLE_STATE            0
#define     SEND_OP_COND             1
#define     SEND_CSD                 2
#define     SEND_CID                 3
#define     STOP_TRANSMISSION        4
#define     SEND_STATUS              5
#define     SET_BLOCKLEN             6
#define     READ_SINGLE_BLOCK        7
#define     READ_MULTIPLE_BLOCK      8
#define     WRITE_BLOCK              9
#define     WRITE_MULTIPLE_BLOCK    10
#define     PROGRAM_CSD             11
#define     SET_WRITE_PROT          12
#define     CLR_WRITE_PROT          13
#define     SEND_WRITE_PROT         14
#define     TAG_SECTOR_START        15
#define     TAG_SECTOR_END          16
#define     UNTAG_SECTOR            17
#define     TAG_ERASE_GROUP_START   18
#define     TAG_ERASE_GROUP_END     19
#define     UNTAG_ERASE_GROUP       20
#define     ERASE                   21
#define     LOCK_UNLOCK             22
#define     READ_OCR                23
#define     CRC_ON_OFF              24

//-----------------------------------------------------------------------------
// UNIONs, STRUCTUREs, and ENUMs
//-----------------------------------------------------------------------------
typedef union LONG_mmc {                   // byte-addressable LONG
  long l;
  unsigned char b[4];
} LONG_mmc;

typedef union INT_mmc {                    // byte-addressable INT
  short i;
  unsigned char b[2];
} INT_mmc;

typedef union {                        // byte-addressable unsigned long
    unsigned long l;
    unsigned char b[4];
              } ULONG_mmc;

typedef union {                        // byte-addressable unsigned int
    unsigned short i;
    unsigned char b[2];
              } UINT_mmc;

// This structure defines entries into the command table;
typedef struct {
    unsigned char command_byte;      // OpCode;
    unsigned char arg_required;      // Indicates argument requirement;
    unsigned char CRC;               // Holds CRC for command if necessary;
    unsigned char trans_type;        // Indicates command transfer type;
    unsigned char response;          // Indicates expected response;
    unsigned char var_length;        // Indicates varialble length transfer;
               } COMMAND_MMC;

// Command table for MMC.  This table contains all commands available in SPI
// mode;  Format of command entries is described above in command structure
// definition;
COMMAND_MMC commandlist[25] = {
    { 0,NO ,0x95,CMD,R1 ,NO },    // CMD0;  GO_IDLE_STATE: reset card;
    { 1,NO ,0xFF,CMD,R1 ,NO },    // CMD1;  SEND_OP_COND: initialize card;
    { 9,NO ,0xFF,RD ,R1 ,NO },    // CMD9;  SEND_CSD: get card specific data;
    {10,NO ,0xFF,RD ,R1 ,NO },    // CMD10; SEND_CID: get card identifier;
    {12,NO ,0xFF,CMD,R1 ,NO },    // CMD12; STOP_TRANSMISSION: end read;
    {13,NO ,0xFF,CMD,R2 ,NO },    // CMD13; SEND_STATUS: read card status;
    {16,YES,0xFF,CMD,R1 ,NO },    // CMD16; SET_BLOCKLEN: set block size;
    {17,YES,0xFF,RD ,R1 ,NO },    // CMD17; READ_SINGLE_BLOCK: read 1 block;
    {18,YES,0xFF,RD ,R1 ,YES},    // CMD18; READ_MULTIPLE_BLOCK: read > 1;
    {24,YES,0xFF,WR ,R1 ,NO },    // CMD24; WRITE_BLOCK: write 1 block;
    {25,YES,0xFF,WR ,R1 ,YES},    // CMD25; WRITE_MULTIPLE_BLOCK: write > 1;
    {27,NO ,0xFF,CMD,R1 ,NO },    // CMD27; PROGRAM_CSD: program CSD;
    {28,YES,0xFF,CMD,R1b,NO },    // CMD28; SET_WRITE_PROT: set wp for group;
    {29,YES,0xFF,CMD,R1b,NO },    // CMD29; CLR_WRITE_PROT: clear group wp;
    {30,YES,0xFF,CMD,R1 ,NO },    // CMD30; SEND_WRITE_PROT: check wp status;
    {32,YES,0xFF,CMD,R1 ,NO },    // CMD32; TAG_SECTOR_START: tag 1st erase;
    {33,YES,0xFF,CMD,R1 ,NO },    // CMD33; TAG_SECTOR_END: tag end(single);
    {34,YES,0xFF,CMD,R1 ,NO },    // CMD34; UNTAG_SECTOR: deselect for erase;
    {35,YES,0xFF,CMD,R1 ,NO },    // CMD35; TAG_ERASE_GROUP_START;
    {36,YES,0xFF,CMD,R1 ,NO },    // CMD36; TAG_ERASE_GROUP_END;
    {37,YES,0xFF,CMD,R1 ,NO },    // CMD37; UNTAG_ERASE_GROUP;
    {38,YES,0xFF,CMD,R1b,NO },    // CMD38; ERASE: erase all tagged sectors;
    {42,YES,0xFF,CMD,R1b,NO },    // CMD42; LOCK_UNLOCK;
    {58,NO ,0xFF,CMD,R3 ,NO },    // CMD58; READ_OCR: read OCR register;
    {59,YES,0xFF,CMD,R1 ,NO }    // CMD59; CRC_ON_OFF: toggles CRC checking;
                              };

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

// Removed these. It doesn't work correctly on every MMC card, and we need
// all the resources we can get.
unsigned long PHYSICAL_SIZE;     // MMC size variable;  Set during
                                       // initialization;

unsigned long PHYSICAL_BLOCKS;   // MMC block number;  Computed during
                                       // initialization;

unsigned char IsInitialized;

char LOCAL_BLOCK[BUFFER_SIZE];
unsigned char test_data;


#define SEND__IN_FUNCTION



void Waitms(unsigned int count);
void Waitns(unsigned int count);

unsigned char scratch[PHYSICAL_BLOCK_SIZE];
//extern READ_BYTES(unsigned char* pchar,unsigned int len);
//extern WRITE_BYTES(unsigned char* pchar,unsigned int len);

//-----------------------------------------------------------------------------
// MMC_Command_Exec
//-----------------------------------------------------------------------------
//
// This function generates the necessary SPI traffic for all MMC SPI commands.
// The three parameters are described below:
//
// cmd:      This parameter is used to index into the command table and read
//           the desired command.  The Command Table Index Constants allow the
//           caller to use a meaningful constant name in the cmd parameter
//           instead of a simple index number.  For example, instead of calling
//           MMC_Command_Exec (0, argument, pchar) to send the MMC into idle
//           state, the user can call
//           MMC_Command_Exec (GO_IDLE_STATE, argument, pchar);
//
// argument: This parameter is used for MMC commands that require an argument.
//           MMC arguments are 32-bits long and can be values such as an
//           an address, a block length setting, or register settings for the
//           MMC.
//
// pchar:    This parameter is a pointer to the local data location for MMC
//           data operations.  When a read or write occurs, data will be stored
//           or retrieved from the location pointed to by pchar.
//
// The MMC_Command_Exec function indexes the command table using the cmd
// parameter. It reads the command table entry into memory and uses information
// from that entry to determine how to proceed.  Returns the 16-bit card
// response value;
//

unsigned int MMC_Command_Exec (unsigned char cmd, unsigned long argument,
                           unsigned char *pchar)
{
	unsigned char loopguard, i;
	unsigned char wlan_tx_buffer[2],wlan_tx_data[10]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};
	unsigned char wlan_rx_buffer[10], index_addr;
	COMMAND_MMC current_command;      // Local space for the command table
	           // entry;
	ULONG_mmc long_arg;               // Union variable for easy byte
	           // transfers of the argument;
	           // Static variable that holds the
	           // current data block length;
	static unsigned int current_blklen = 512;
	unsigned long old_blklen = 512;     // Temp variable to preserve data block
	           // length during temporary changes;
	unsigned int counter = 0;     // Byte counter for multi-byte fields;
	UINT_mmc card_response;           // Variable for storing card response;
	unsigned char data_resp;      // Variable for storing data response;
	unsigned char dummy_CRC;      // Dummy variable for storing CRC field;
	unsigned char *plast;
	unsigned int	byte_count;
	//   xdata unsigned char c;

	current_command = commandlist[cmd]; // Retrieve desired command table entry
				// from code space;
	card_response.i = 0;
	printf("     MMC_Command_Exec  0  \n");

	Master_nSS_Con0(1);                         // Select MMC by pulling CS low;
		DELAY(10);
	//Waitms(20);
	Master_nSS_Con0(0);                         // Select MMC by pulling CS low;

		DELAY(10);
	//Waitms(20);

	index_addr = 0;
	while(1)
	{
		//rx buffer clear;
		for(i=0;i<10;i++)
			wlan_rx_buffer[i] = 0xa5;

	  Waitms(20);
		Master_nSS_Con0(1);                         // CS High;
		DELAY(10);
		Master_nSS_Con0(0);                         // CS low;
		DELAY(10);

		SPI_Direction(TX_DIR);//--------------------------------------------------------------------Tx

	   wlan_tx_buffer[1] = 0x00;
	   wlan_tx_buffer[0] = 0x14;
		for(i=0;i<2;i++)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = wlan_tx_buffer[i];                     // Send another byte of SPI clocks;
			DELAY(10);
			//Waitms(1);
			while(!REDY0);
			DELAY(10);
			//printf("wlan tx ID address[%d] = 0x%x \n", i,wlan_tx_buffer[i]);
		}
		//for(i=0;i<2;i++)
		//	printf("wlan tx ID address[%d] = 0x%x \n", i,wlan_tx_buffer[i]);
	    // Issue command opcode;
	  //while(!REDY0);   //Check Tx ready state

		for(i=0;i<10;i++)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = wlan_tx_data[i];                     // Send another byte of SPI clocks;
			DELAY(20);
			//Waitms(1);
			while(!REDY0);
			DELAY(20);
			//printf("wlan tx ID address[%d] = 0x%x \n", i,wlan_tx_data[i]);
		}
		DELAY(200);

		//for(i=0;i<10;i++)
		//	printf("wlan tx data[%d] = 0x%x \n", i,wlan_tx_data[i]);
	    // Issue command opcode;
	  //while(!REDY0);   //Check Tx ready state

		Master_nSS_Con0(1);                         // Select MMC by pulling CS low;
		DELAY(10);
		//Waitms(1);
		Master_nSS_Con0(0);                         // Select MMC by pulling CS low;
		DELAY(10);
		Waitms(10);

		SPI_Direction(TX_DIR);//--------------------------------------------------------------------Tx

     wlan_tx_buffer[1] = 0x00;
     wlan_tx_buffer[0] = 0x94;//index_addr;
     wlan_tx_buffer[0] |= 0x80;
		for(i=0;i<2;i++)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = wlan_tx_buffer[i];                     // Send another byte of SPI clocks;
			DELAY(10);
			//Waitms(1);
			while(!REDY0);
			DELAY(10);
			//printf("wlan tx ID address[%d] = 0x%x \n", i,wlan_tx_buffer[i]);
		}
		//for(i=0;i<2;i++)
		//	printf("wlan tx ID address[%d] = 0x%x \n", i,wlan_tx_buffer[i]);
	   // Issue command opcode;
	  //     while(!REDY0);   //Check Tx ready state
		//Waitms(1);
		DELAY(10);

		SPI_Direction(RX_DIR); //--------------------------------------------------------------------Rx
		DELAY(10);
		Waitms(10);

		for(i=0;i<10;i++)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // Write dummy value to SPI so that
			DELAY(20);
			//Waitms(1);
			while(!REDY0);             // the response byte will be shifted in;

			/////card_response.b[0] = SPI0DAT; ////  ??????    // Save the response;
			wlan_rx_buffer[i] = rSPRDATB0; // Save the response;
			DELAY(20);
		}

		for(i=0;i<10;i++)
			printf("wlan rx data[%d] = 0x%x \n", i,wlan_rx_buffer[i]);

		DELAY(200);
		Master_nSS_Con0(1);                         // Select MMC by pulling CS low;
			DELAY(20);
		//Waitms(1);
		index_addr++;
		if(index_addr==0x80) index_addr=0;

	}

	SPI_Direction(TX_DIR);
	for(i=0;i<4;i++)
	{
		while(!REDY0);   //Check Tx ready state
		rSPTDAT0 = 0xFF;                     // Send another byte of SPI clocks;
		while(!REDY0_org){}
	}
    // Issue command opcode;
       while(!REDY0);   //Check Tx ready state
	rSPTDAT0 = (current_command.command_byte | 0x40);
	printf("     MMC_Command_Exec  1  \n");

	long_arg.l = argument;              // Make argument byte addressable;
	          // If current command changes block
	          // length, update block length variable
	          // to keep track;
	          // Command byte = 16 means that a set
	          // block length command is taking place
	          // and block length variable must be
	          // set;
	if(current_command.command_byte == 16)
	{
		current_blklen = argument;
	}
	           // Command byte = 9 or 10 means that a
	           // 16-byte register value is being read
	           // from the card, block length must be
	           // set to 16 bytes, and restored at the
	           // end of the transfer;
	if((current_command.command_byte == 9)||(current_command.command_byte == 10))
	{
		old_blklen = current_blklen;     // Command is a GET_CSD or GET_CID,
		current_blklen = 16;             // set block length to 16-bytes;
	}
    while(!REDY0_org){}                      // Wait for initial SPI transfer to end;
                          // Clear SPI Interrupt flag;

	           // If an argument is required, transmit
	           // one, otherwise transmit 4 bytes of
	           // 0x00;
	plast = &pchar[current_blklen];
	if(current_command.arg_required == YES)
	{
		counter = 0;
		while(counter <= 3)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = long_arg.b[counter];
			counter++;
			while(!REDY0_org){}
		}
	}
	else
	{
		counter = 0;
		while(counter <= 3)
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0x00;
			counter++;
			while(!REDY0_org){}
		}
	}
	printf("     MMC_Command_Exec  2  \n");
	while(!REDY0);   //Check Tx ready state
	rSPTDAT0 = current_command.CRC;      // Transmit CRC byte;  In all cases
//	printf("command CRC = %x\n", current_command.CRC );
	while(!REDY0_org){}                      // except CMD0, this will be a dummy
                        // character;
	           // The command table entry will indicate
	           // what type of response to expect for
	           // a given command;  The following
	           // conditional handles the MMC response;

	SPI_Direction(RX_DIR);
	if(current_command.response == R1)
	{ // Read the R1 response from the card;
		loopguard=0;
		do
		{
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // Write dummy value to SPI so that
			while(!REDY0_org){}                // the response byte will be shifted in;

			/////card_response.b[0] = SPI0DAT; ////  ??????    // Save the response;
			card_response.b[0] = rSPRDATB0; // Save the response;

			if(!++loopguard)
				break;
			if(card_response.b[0] & BUSY_BIT)
			{
			//	printf("R1 response for busybit 0x%x\n",card_response.b[0]);
				Waitns(700);
			}

		}while((card_response.b[0] & BUSY_BIT));

		printf("==> card response 0x%x\n",card_response.b[0]);

		if(!loopguard)
		{
			printf("R1 response for loopguard 0x%x\n",card_response.b[0]);
			while(1);
			return card_response.b[0];}//{ return 0; }
		}                                     // Read the R1b response;
		else if(current_command.response == R1b)
		{
			printf("     MMC_Command_Exec  4  \n");
			loopguard = 0;
			do
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;               // Start SPI transfer;
				while(!REDY0_org){}

				//////card_response.b[0] = SPI0DAT;    /////  ?????   // Save card response
				card_response.b[0] = rSPRDATB0;

				if(!++loopguard)
					break;
			}while((card_response.b[0] & BUSY_BIT));
			loopguard = 0;
			do
			{                              // Wait for busy signal to end;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}

				if(!++loopguard)
					break;
			}while(rSPTDAT0 == 0x00);          // When byte from card is non-zero,
			if(!loopguard)
			{
				//BACK_FROM_ERROR;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				Master_nSS_Con0(1);
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				return ERROR_CODE;

			}
		}                                   // card is no longer busy;
	       else if(current_command.response == R2) 			   // Read R2 response
		{
			loopguard=0;
			printf("     MMC_Command_Exec  5  \n");
			do
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;               // Start SPI transfer;
				while(!REDY0_org){}
				//////card_response.b[0] = SPI0DAT;   ////  ?????  // Read first byte of response;
				card_response.b[0] = rSPRDATB0;


				if(!++loopguard)
					break;
			}while((card_response.b[0] & BUSY_BIT));
			if(!loopguard)
			{
				//BACK_FROM_ERROR;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				Master_nSS_Con0(1);
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				return ERROR_CODE;
			}
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;
			while(!REDY0_org){}
			//////card_response.b[1] = SPI0DAT;       //////   ????/   // Read second byte of response;
			card_response.b[1] = rSPRDATB0;
		   }
		else
		{                               // Read R3 response;
			loopguard=0;
			printf("     MMC_Command_Exec  6  \n");
			do
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;               // Start SPI transfer;
				while(!REDY0_org){}
				/////card_response.b[0] = SPI0DAT;   //// ????    // Read first byte of response;
				card_response.b[0] = rSPRDATB0;

				if(!++loopguard) break;
			} while((card_response.b[0] & BUSY_BIT));

			if(!loopguard)
			{
				//BACK_FROM_ERROR;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				Master_nSS_Con0(1);
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				return ERROR_CODE;
			}
			counter = 0;
			while(counter <= 3)              // Read next three bytes and store them
			{                                // in local memory;  These bytes make up
				counter++;                    // the Operating Conditions Register
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;               // (OCR);
				while(!REDY0_org){}
				/////*pchar++ = SPI0DAT;	////????
				*pchar++ = rSPRDATB0;
			}
		}
		printf("     MMC_Command_Exec  7  \n");

//	printf("     command trans_type : %x \n", current_command.trans_type);

	switch(current_command.trans_type)  // This conditional handles all data
	{                                   // operations;  The command entry
	           // determines what type, if any, data
	           // operations need to occur;
		case RD:                         // Read data from the MMC;
			loopguard = 0;
			printf("     MMC_Command_Exec  8  \n");
			do                            // Wait for a start read token from
			{                             // the MMC;
					   // Start a SPI transfer;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				if(!++loopguard) break;
				//printf("read token(start_sbr) = %x \n", rSPRDATB0);
				test_data = rSPRDATB0;
				printf("read token(start_sbr) = %x \n", rSPRDATB0);

			} while(test_data != START_SBR);  // Check for a start read token;
			if(!loopguard)
			{
				//BACK_FROM_ERROR;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				Master_nSS_Con0(1);
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				return ERROR_CODE;
			}

			counter = 0;                  // Reset byte counter;
			           // Read <current_blklen> bytes;
			//	 START_SPI_TIMEOUT;
			//// READ_BYTES(pchar,current_blklen);		/////???????
			for(byte_count = 0;byte_count < current_blklen;byte_count++)
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;               // (OCR);
				while(!REDY0_org){}
				*pchar++ = rSPRDATB0;
			}


			//	 STOP_SPI_TIME_OUT;
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // After all data is read, read the two
			while(!REDY0_org){}                // CRC bytes;  These bytes are not used
                   // in this mode, but the placeholders
			//dummy_CRC = SPI0DAT;       ///???   // must be read anyway;
			dummy_CRC = rSPRDATB0;

			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;
			while(!REDY0_org){}
			//////dummy_CRC = SPI0DAT;		///???
			dummy_CRC = rSPRDATB0;

			break;
		case WR:
			printf("     MMC_Command_Exec  9  \n");
			// Write data to the MMC;
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // Start by sending 8 SPI clocks so
			while(!REDY0_org){}                // the MMC can prepare for the write;

			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = START_SBW;          // Send the start write block token;
			while(!REDY0_org){}

				            // Reset byte counter;
			Waitns(700);
			//START_SPI_TIMEOUT;
			/////// WRITE_BYTES(pchar,current_blklen);    ////////???????????

			for(byte_count = 0;byte_count < current_blklen;byte_count++)
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = *pchar++;               // (OCR);
				while(!REDY0_org){}
			}




			// STOP_SPI_TIME_OUT;
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // Write CRC bytes (don't cares);
			while(!REDY0_org){}

			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;
			while(!REDY0_org){}

			loopguard = 0;
			do                            // Read Data Response from card;
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}

				////data_resp = SPI0DAT;			///????????
				data_resp = rSPRDATB0;
				if(!++loopguard)
					break;
			}while((data_resp & DATA_RESP_MASK) != 0x01);    // When bit 0 of the MMC response
			           										// is clear, a valid data response
			           										// has been received;
			if(!loopguard)
			{
				//BACK_FROM_ERROR;
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				Master_nSS_Con0(1);
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;
				while(!REDY0_org){}
				return ERROR_CODE;
			}
			do                            // Wait for end of busy signal;
			{
				while(!REDY0);   //Check Tx ready state
				rSPTDAT0 = 0xFF;            // Start SPI transfer to receive
				while(!REDY0_org){}             // busy tokens;
			} while(rSPTDAT0 == 0x00);       // When a non-zero token is returned,
			           // card is no longer busy;
			while(!REDY0);   //Check Tx ready state
			rSPTDAT0 = 0xFF;               // Issue 8 SPI clocks so that all card
			while(!REDY0_org){}                // operations can complete;
			break;
		default: break;
		}

	SPI_Direction(TX_DIR);
	while(!REDY0);   //Check Tx ready state
	rSPTDAT0 = 0xFF;
	while(!REDY0_org){}

	printf("     MMC_Command_Exec  10  \n");
	Master_nSS_Con0(1);                         // Deselect memory card;
	while(!REDY0);   //Check Tx ready state
	rSPTDAT0 = 0xFF;                     // Send 8 more SPI clocks to ensure
	while(!REDY0_org){}                      // the card has finished all necessary
                        // operations;
	           // Restore old block length if needed;
	if((current_command.command_byte == 9)||(current_command.command_byte == 10))
	{
		current_blklen = old_blklen;
	}
	return card_response.i;
}


//-----------------------------------------------------------------------------
// MMC_FLASH_Init
//-----------------------------------------------------------------------------
//
// This function initializes the flash card, configures it to operate in SPI
// mode, and reads the operating conditions register to ensure that the device
// has initialized correctly.  It also determines the size of the card by
// reading the Card Specific Data Register (CSD).

void MMC_FLASH_Init(void)
{
	unsigned loopguard;
	int i;
	UINT_mmc card_status;             // Stores card status returned from
	// MMC function calls(MMC_Command_Exec);
	unsigned char counter = 0;    // SPI byte counter;
	unsigned char  *pchar;         // Xdata pointer for storing MMC
	// register values;
	// Transmit at least 64 SPI clocks
	// before any bus comm occurs.

	unsigned int c_size,bl_len;
	unsigned char c_mult;
	//	PHYSICAL_SIZE=0;
	//	PHYSICAL_BLOCKS=0;

	SPI_Init();
	Waitms(100);
	Master_nSS_Con0(1);
	pchar = (unsigned char *)LOCAL_BLOCK;

	SPI_Direction(TX_DIR);
	for(counter = 0; counter < 20; counter++) 	// min 80 clocks to get MMC ready
	{
		while(!REDY0);   //Check Tx ready state
		rSPTDAT0 = 0xFF;
		while(!REDY0_org){}
		Waitms(1);
	}
	// Send the GO_IDLE_STATE command with
	// CS driven low;  This causes the MMC
	// to enter SPI mode;
	Waitms(1);
	printf("MMC_FLASH_Init  0  \n");
	card_status.i = MMC_Command_Exec(GO_IDLE_STATE,EMPTY,EMPTY);

	loopguard=0;
	printf("MMC_FLASH_Init  1  \n");
	// Send the SEND_OP_COND command
	do                                  // until the MMC indicates that it is
	{
		Waitms(1);
		card_status.i = MMC_Command_Exec(SEND_OP_COND,EMPTY,EMPTY);
		if(!++loopguard)
		break;
	} while ((card_status.b[0] & 0x01));
	printf("count SEND_OP_COND: %d\n",loopguard);
	printf("     card status[b1] = %x \n", card_status.b[1]);
	printf("     card status[b0] = %x \n", card_status.b[0]);

	if(!loopguard)
		return;
	while(!REDY0);   //Check Tx ready state
	rSPTDAT0 = 0xFF;                     // Send 8 more SPI clocks to complete
	while(!REDY0_org){}                      // the initialization sequence;

	loopguard=0;
	printf("MMC_FLASH_Init  3  \n");
	do                                  // Read the Operating Conditions
	{                                   // Register (OCR);
		card_status.i = MMC_Command_Exec(READ_OCR,EMPTY,pchar);
		if(!++loopguard)
			break;
	} while(!(*pchar&0x80));              // Check the card busy bit of the OCR;
	if(!loopguard)
		return;
	printf("MMC_FLASH_Init  4  \n");
	card_status.i = MMC_Command_Exec(SEND_STATUS,EMPTY,EMPTY);
	// Get the Card Specific Data (CSD)
	// register to determine the size of the
	// MMC;
	for(i=0;i<4;i++)
	{
		printf("0x%x,  ",pchar[i]);
	}
	printf("MMC_FLASH_Init  5  \n");
	card_status.i = MMC_Command_Exec(SEND_CSD,EMPTY,pchar);

	if(card_status.i==0)
	{
		printf("Change speed");
		for(i=0;i<16;i++)
		{
			printf("0x%x,  ",pchar[i]);
		}
		//SPI0CKR = 1;
		Waitms(1);
	}
	else
	{
		printf("CARD STATUS 0x%x : \n",card_status.i);
		for(i=0;i<16;i++)
		{
			printf("0x%x,  ",pchar[i]);
		}
		PHYSICAL_BLOCKS = 0;
		PHYSICAL_SIZE = PHYSICAL_BLOCKS * bl_len;
		return;
	}
	printf("MMC_FLASH_Init  6  \n");
	card_status.i = MMC_Command_Exec(SET_BLOCKLEN,(unsigned long)PHYSICAL_BLOCK_SIZE,EMPTY);

	bl_len = 1 << (pchar[5] & 0x0f) ;
	c_size = ((pchar[6] & 0x03) << 10) |
	(pchar[7] << 2) | ((pchar[8] &0xc0) >> 6);
	c_mult = (((pchar[9] & 0x03) << 1) | ((pchar[10] & 0x80) >> 7));

	printf("bl_len 0x%x  \n",bl_len);
	printf("c_size 0x%x  \n",c_size);
	printf("c_mult 0x%x  \n",c_mult);

	// Determine the number of MMC sectors;
	PHYSICAL_BLOCKS = (unsigned long)(c_size+1)*(1 << (c_mult+2));
	PHYSICAL_SIZE = PHYSICAL_BLOCKS * bl_len;


	printf("PHYSICAL_BLOCKS 0x%x  \n",PHYSICAL_BLOCKS);
	printf("PHYSICAL_SIZE 0x%x  \n",PHYSICAL_SIZE);


	loopguard = 0;
	printf("MMC_FLASH_Init  7  \n");
	while((MMC_FLASH_Block_Read(0,scratch)!=0))
	{
		if(!++loopguard)
			break;
	}
	//printf("Wrong reads %d\n",loopguard);

	IsInitialized = 1;

}

//-----------------------------------------------------------------------------
// MMC_FLASH_Block_Read
//-----------------------------------------------------------------------------
//
// If you know beforehand that you'll read an entire 512-byte block, then
// this function has a smaller ROM footprint than MMC_FLASH_Read.
unsigned int MMC_FLASH_Block_Read(unsigned long address, unsigned char *pchar)
{
	unsigned int card_status;     // Stores MMC status after each MMC command;
	address *= PHYSICAL_BLOCK_SIZE;

	return card_status;
}

//-----------------------------------------------------------------------------
// MMC_FLASH_Block_Write
//-----------------------------------------------------------------------------
//
// If you know beforehand that you'll write an entire 512-byte block, then
// this function is more RAM-efficient than MMC_FLASH_Write because it
// doesn't require a 512-byte scratch buffer (and it's faster too, it doesn't
// require a read operation first). And it has a smaller ROM footprint too.
unsigned char MMC_FLASH_Block_Write(unsigned long address,unsigned char *wdata)
{
	unsigned int card_status;     // Stores status returned from MMC;

	address *= PHYSICAL_BLOCK_SIZE;

	card_status = MMC_Command_Exec(WRITE_BLOCK,address ,wdata);
	return card_status;
}



void Waitms(unsigned int count)
{
	int i,j;

	for(i=0;i<count;i++)
	{
		for(j=0;j<1000;j++)
		{
			Waitns(1000);
		}
	}
}

void Waitns(unsigned int count)
{
  count/=20;
   while(count--) ;
}


#endif







