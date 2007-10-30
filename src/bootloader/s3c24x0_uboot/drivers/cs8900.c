/*
 * Cirrus Logic CS8900A Ethernet
 *
 * (C) 2003 Wolfgang Denk, wd@denx.de
 *     Extension to synchronize ethaddr environment variable
 *     against value in EEPROM
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 1999 Ben Williamson <benw@pobox.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is loaded into SRAM in bootstrap mode, where it waits
 * for commands on UART1 to read and write memory, jump to code etc.
 * A design goal for this program is to be entirely independent of the
 * target board.  Anything with a CL-PS7111 or EP7211 should be able to run
 * this code in bootstrap mode.  All the board specifics can be handled on
 * the host.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <command.h>
#include "cs8900.h"
#include <net.h>
/* wjluv add this for SMDK24A0 */
#ifdef CONFIG_S3C24A0A_SMDK
#include <asm/arch/s3c24a0.h>
#elif defined (CONFIG_S3C2410)
#include <s3c2410.h>
#elif defined (CONFIG_S3C2440A_SMDK)
#include <s3c2440.h>
#elif defined (CONFIG_S3C2460x_SMDK)
#include <asm/arch/s3c2460.h>
#elif defined (CONFIG_S3C2413_SMDK)
#include <asm/arch/s3c2413.h>
#endif


#ifdef CONFIG_DRIVER_CS8900

#if (CONFIG_COMMANDS & CFG_CMD_NET)


/* packet page register access functions */

#ifdef CS8900_BUS32
/* we don't need 16 bit initialisation on 32 bit bus */
#define get_reg_init_bus(x) get_reg((x))
#else
static unsigned short get_reg_init_bus (int regno)
{
	/* force 16 bit busmode */
	volatile unsigned char c;

	c = CS8900_BUS16_0;
	c = CS8900_BUS16_1;
	c = CS8900_BUS16_0;
	c = CS8900_BUS16_1;
	c = CS8900_BUS16_0;

	CS8900_PPTR = regno;
	return (unsigned short) CS8900_PDATA;
}
#endif

unsigned short get_reg (int regno)
{
	CS8900_PPTR = regno;
	return (unsigned short) CS8900_PDATA;
}


static void put_reg (int regno, unsigned short val)
{  
	CS8900_PPTR = regno;
	CS8900_PDATA = val;
}

void eth_reset (void)
{
	int tmo;
	unsigned short us;

	/* reset NIC */
	put_reg (PP_SelfCTL, get_reg (PP_SelfCTL) | PP_SelfCTL_Reset);

	/* wait for 200ms */
	udelay (200000);
	/* Wait until the chip is reset */

	tmo = get_timer (0) + 1 * CFG_HZ;
	while ((((us = get_reg_init_bus (PP_SelfSTAT)) & PP_SelfSTAT_InitD) == 0)
		   && tmo < get_timer (0))
		/*NOP*/;
}

void cs8900_get_enetaddr (uchar * addr)
{
	int i;
	unsigned char env_enetaddr[6];
	char *tmp = getenv ("ethaddr");
	char *end;

	for (i=0; i<6; i++) {
		env_enetaddr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
		if (tmp)
			tmp = (*end) ? end+1 : end;
	}

	if (get_reg_init_bus (PP_ChipID) != 0x630e)
	{
//		printf("Ethernet CS8900 id 0x%x \n",get_reg_init_bus (PP_ChipID));
		return;
	}
	eth_reset ();
	if ((get_reg (PP_SelfST) & (PP_SelfSTAT_EEPROM | PP_SelfSTAT_EEPROM_OK)) ==
			(PP_SelfSTAT_EEPROM | PP_SelfSTAT_EEPROM_OK)) {

		/* Load the MAC from EEPROM */
		for (i = 0; i < 6 / 2; i++) {
			unsigned int Addr;

			Addr = get_reg (PP_IA + i * 2);
			addr[i * 2] = Addr & 0xFF;
			addr[i * 2 + 1] = Addr >> 8;
		}

		if (memcmp(env_enetaddr, "\0\0\0\0\0\0", 6) != 0 &&
		    memcmp(env_enetaddr, addr, 6) != 0) {
			printf ("\nWarning: MAC addresses don't match:\n");
			printf ("\tHW MAC address:  "
				"%02X:%02X:%02X:%02X:%02X:%02X\n",
				addr[0], addr[1],
				addr[2], addr[3],
				addr[4], addr[5] );
			printf ("\t\"ethaddr\" value: "
				"%02X:%02X:%02X:%02X:%02X:%02X\n",
				env_enetaddr[0], env_enetaddr[1],
				env_enetaddr[2], env_enetaddr[3],
				env_enetaddr[4], env_enetaddr[5]) ;
			debug ("### Set MAC addr from environment\n");
			memcpy (addr, env_enetaddr, 6);
		}
		if (!tmp) {
			char ethaddr[20];
			sprintf (ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
				 addr[0], addr[1],
				 addr[2], addr[3],
				 addr[4], addr[5]) ;
			debug ("### Set environment from HW MAC addr = \"%s\"\n",				ethaddr);
			setenv ("ethaddr", ethaddr);
		}

	}
}

void eth_halt (void)
{
	/* disable transmitter/receiver mode */
	put_reg (PP_LineCTL, 0);

	/* "shutdown" to show ChipID or kernel wouldn't find he cs8900 ... */
	get_reg_init_bus (PP_ChipID);
}

int eth_init (bd_t * bd)
{
	/*
#if defined(CONFIG_S3C24A0A_SMDK) || defined(CONFIG_S3C2460x)
SROM_BW &=~0x38;
SROM_BW |=0x38;
SROM_BC1 = 0x25a0; 
	 */
#if defined(CONFIG_S3C24A0A_SMDK)
	SROM_BW &=~0x38;
	SROM_BW |=0x38;
	SROM_BC1 = 0x25a0; 
#elif defined (CONFIG_S3C2460x)
	SROM_BW &=~0x1c0;
	SROM_BW |=0x1c0;
	SROM_BC2 = 0x25a0; 
#elif defined (CONFIG_S3C2440A_SMDK)||defined (CONFIG_S3C2410)
	BWSCON = ( BWSCON & ~(0xf<<12)) | (0xd << 12);
	BANKCON3 = ( 3<<11)|(0x7<<8)|(0x1<<6)|(0x3<<4)|(0x3<<2);
#elif defined (CONFIG_S3C2411_SMDK)
	BANK_CFG &= ~((1<<5)|(1<<4));   //EBI Setting for SROM
									//BANK4,5 conf. = one NAND/SRAM
	SMBIDCYR1 = 0;                  //Bank5 Idle cycle ctrl.
	SMBWSTWRR1 = 14;                //Bank5 Write Wait State ctrl.
	SMBWSTOENR1 = 2;                //Bank5 Output Enable Assertion Delay ctrl.     Tcho?
	SMBWSTWENR1 = 2;                //Bank5 Write Enable Assertion Delay ctrl.

	SMBWSTRDR1 = 14;                //Bank5 Read Wait State cont. = 14 clk          Tacc?
	SMBCR1 |=  ((1<<2)|(1<<0));     //SMWAIT active High, Read Byte Lane Enabl      WS1?
	SMBCR1 &= ~((3<<20)|(3<<12));   //SMADDRVALID = always High when Read/Write
	SMBCR1 &= ~(3<<4);          	//Clear Memory Width
	SMBCR1 |=  (1<<4);          	//Memory Width = 16bit
#elif defined (CONFIG_S3C2413_SMDK)
	BANK_CFG &= ~((1<<5)|(1<<4));   //EBI Setting for SROM
									//BANK4,5 conf. = one NAND/SRAM
	SMBIDCYR5 = 0;                  //Bank5 Idle cycle ctrl.
	SMBWSTWRR5 = 14;                //Bank5 Write Wait State ctrl.
	SMBWSTOENR5 = 2;                //Bank5 Output Enable Assertion Delay ctrl.     Tcho?
	SMBWSTWENR5 = 2;                //Bank5 Write Enable Assertion Delay ctrl.

	SMBWSTRDR5 = 14;                //Bank5 Read Wait State cont. = 14 clk          Tacc?
	SMBCR5 |=  ((1<<2)|(1<<0));     //SMWAIT active High, Read Byte Lane Enabl      WS1?
	SMBCR5 &= ~((3<<20)|(3<<12));   //SMADDRVALID = always High when Read/Write
	SMBCR5 &= ~(3<<4);          	//Clear Memory Width
	SMBCR5 |=  (1<<4);          	//Memory Width = 16bit
#endif


	/* verify chip id */
	if (get_reg_init_bus (PP_ChipID) != 0x630e) {
		printf ("CS8900 Ethernet chip not found?!\n");
		return 0;
	}

	eth_reset ();
	/* set the ethernet address */
	put_reg (PP_IA + 0, bd->bi_enetaddr[0] | (bd->bi_enetaddr[1] << 8));
	put_reg (PP_IA + 2, bd->bi_enetaddr[2] | (bd->bi_enetaddr[3] << 8));
	put_reg (PP_IA + 4, bd->bi_enetaddr[4] | (bd->bi_enetaddr[5] << 8));

	/* receive only error free packets addressed to this card */
	put_reg (PP_RxCTL, PP_RxCTL_IA | PP_RxCTL_Broadcast | PP_RxCTL_RxOK);

	/* do not generate any interrupts on receive operations */
	put_reg (PP_RxCFG, 0);

	/* do not generate any interrupts on transmit operations */
	put_reg (PP_TxCFG, 0);

	/* do not generate any interrupts on buffer operations */
	put_reg (PP_BufCFG, 0);

	/* enable transmitter/receiver mode */
	put_reg (PP_LineCTL, PP_LineCTL_Rx | PP_LineCTL_Tx);

	return 0;
}

/* Get a data block via Ethernet */
extern int eth_rx (void)
{
	int i;
	unsigned short rxlen;
	unsigned short *addr;
	unsigned short status;

	status = get_reg (PP_RER);

	if ((status & PP_RER_RxOK) == 0)
		return 0;

	status = CS8900_RTDATA;		/* stat */
	rxlen = CS8900_RTDATA;		/* len */

	if (rxlen > PKTSIZE_ALIGN + PKTALIGN)
		printf ("packet too big!\n");

	for (addr = (unsigned short *) NetRxPackets[0], i = rxlen >> 1; i > 0;
		 i--)
		*addr++ = CS8900_RTDATA;
	if (rxlen & 1)
		*addr++ = CS8900_RTDATA;

	/* Pass the packet up to the protocol layers. */
	NetReceive (NetRxPackets[0], rxlen);

	return rxlen;
}

/* Send a data block via Ethernet. */
extern int eth_send (volatile void *packet, int length)
{
	volatile unsigned short *addr;
	int tmo;
	unsigned short s;

retry:
	/* initiate a transmit sequence */
	CS8900_TxCMD = PP_TxCmd_TxStart_Full;
	CS8900_TxLEN = length;

	/* Test to see if the chip has allocated memory for the packet */
	if ((get_reg (PP_BusSTAT) & PP_BusSTAT_TxRDY) == 0) {
		/* Oops... this should not happen! */
		printf ("cs: unable to send packet; retrying...\n");
		for (tmo = get_timer (0) + 5 * CFG_HZ; get_timer (0) < tmo;)
			/*NOP*/;
		eth_reset ();
		goto retry;
	}

	/* Write the contents of the packet */
	/* assume even number of bytes */
	for (addr = packet; length > 0; length -= 2)
		CS8900_RTDATA = *addr++;

	/* wait for transfer to succeed */
	tmo = get_timer (0) + 5 * CFG_HZ;
	while ((s = get_reg (PP_TER) & ~0x1F) == 0) {
		if (get_timer (0) >= tmo)
			break;
	}

	/* nothing */ ;
	if ((s & (PP_TER_CRS | PP_TER_TxOK)) != PP_TER_TxOK) {
		printf ("\ntransmission error %#x\n", s);
	}

	return 0;
}

static void cs8900_e2prom_ready(void)
{
	while(get_reg(PP_SelfST) & SI_BUSY);
}

/***********************************************************/
/* read a 16-bit word out of the EEPROM                    */
/***********************************************************/

int cs8900_e2prom_read(unsigned char addr, unsigned short *value)
{
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_READ_CMD | addr);
	cs8900_e2prom_ready();
	*value = get_reg(PP_EEData);

	return 0;
}


/***********************************************************/
/* write a 16-bit word into the EEPROM                     */
/***********************************************************/

int cs8900_e2prom_write(unsigned char addr, unsigned short value)
{
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_WRITE_EN);
	cs8900_e2prom_ready();
	put_reg(PP_EEData, value);
	put_reg(PP_EECMD, EEPROM_WRITE_CMD | addr);
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_WRITE_DIS);
	cs8900_e2prom_ready();

	return 0;
}

#endif	/* COMMANDS & CFG_NET */

#endif	/* CONFIG_DRIVER_CS8900 */
