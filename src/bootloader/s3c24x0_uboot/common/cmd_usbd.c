/*
 *  (C) Copyright SAMSUNG Electronics 
 *      SW.LEE  <hitchcar@samsung.com>
 *      - add USB device fo S3C2440A, S3C24A0A	
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>

#include <s3c24x0_usb.h>
#include	<board_test.h>

#if (CONFIG_COMMANDS & CFG_CMD_USBD)

#define CMD_USBD_DEBUG

#ifdef	CMD_USBD_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif


#define TX_PACKET_SIZE 64
#define RX_PACKET_SIZE 64

static const char pszMe[] = "usbd: ";

/* twiddle_descriptors()
 * It is between open() and start(). Setup descriptors.
 */
static void twiddle_descriptors( void )
{
         desc_t * pDesc = s3c24x0_usb_get_descriptor_ptr();

         pDesc->b.ep1.wMaxPacketSize = make_word_c( RX_PACKET_SIZE );
         pDesc->b.ep1.bmAttributes   = USB_EP_BULK;
         pDesc->b.ep2.wMaxPacketSize = make_word_c( TX_PACKET_SIZE );
         pDesc->b.ep2.bmAttributes   = USB_EP_BULK;
}


int
usbc_activate(void)
{
        int retval = 0;

        /* start usb core */
        retval = s3c24x0_usb_open( "usb-char" );

        twiddle_descriptors();

        retval = s3c24x0_usb_start();
        if ( retval ) {
                PRINTF( "%sAGHH! Could not USB core\n", pszMe );
                //free_txrx_buffers();
                return retval;
        }

        return 0;
}


extern int usbd_dn_addr;
extern int DNW;






int do_usbd_ud ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	DNW = 0;
	printf("usbd_ud check 1");
	switch (argc) {
		case 1 :
			usbd_dn_addr = USBD_DOWN_ADDR;/* Default Address */
			break;
		case 2 :
			usbd_dn_addr = simple_strtoul(argv[1], NULL, 16);
			break;
		default:
			     printf ("Usage:\n%s\n", cmdtp->usage);
				 return 1;
	}
	
	usbctl_init();
	usbc_activate();
	printf("Download address %d (0x%08x) \n",usbd_dn_addr, usbd_dn_addr);
	printf("Now, Plug USB cable into USB Host and Execute host cmd\n");	
	return 0;
}

extern int got_header;

#if (CONFIG_COMMANDS & CFG_CMD_BOARD_TEST)
int usb_upgrade_dnw ( char mode) 
{
	DNW = 1;
	got_header = 0;
	switch (mode) {
		case 0 :
			usbd_dn_addr = USBD_DOWN_ADDR;/* Default Address */
			break;
		case 1 :
			usbd_dn_addr = USBD_DOWN_ADDR;
			break;
		case 2 :
			usbd_dn_addr = USBD_DOWN_ADDR;
			break;
				
		default:
			printf ("usb_upgrade_dnw 4 \n");
			return 1;
	}
	usbctl_init();
	usbc_activate();
	printf("Download address %d (0x%08x) \n",usbd_dn_addr, usbd_dn_addr);
	return 0;
}
#endif

int do_usbd_dnw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) 
{
	DNW = 1;
	got_header = 0;
	printf("usbd_dnw check 1\n");
	switch (argc) {
		case 1 :
			usbd_dn_addr = USBD_DOWN_ADDR;/* Default Address */
			printf("usbd_dnw check 2\n");
			break;
		case 2 :
			usbd_dn_addr = simple_strtoul(argv[1], NULL, 16);
			printf("usbd_dnw check 3\n");
			break;
		default:
			printf ("Usage:\n%s\n", cmdtp->usage);
			return 1;
	}
	usbctl_init();
	usbc_activate();
	printf("Download address %d (0x%08x) \n",usbd_dn_addr, usbd_dn_addr);
	printf("Now, Waiting for DNW to transmit data \n");	
	return 0;
}
	


/**************************************************/
#if (CONFIG_COMMANDS & CFG_CMD_USBD)
U_BOOT_CMD(
	ud,     3,     1,      do_usbd_ud,
	"ud   - ud [download address]\n",
	"  initialize USB device and ready to receive\n"
);

U_BOOT_CMD(
	dnw,     3,     1,      do_usbd_dnw,
	"ud   - ud [download address]\n",
	"  initialize USB device and ready to receive\n"
);

#endif
#endif	/* CFG_CMD_USBD */
