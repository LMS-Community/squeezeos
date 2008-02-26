/*
 * 2004 (C) SAMSUNG ELECTRONICS 
 *       SW.LEE <hitchcar@samsung.com>
 *       ported to Uboot for S3C24A0A, S3C2440A
 *
 * usb_recv.c
 * 
 * S3C2410 USB receive function 
 * endpoint 1
 *
 * bushi <bushi@mizi.com>
 */

#include <common.h>
#include <command.h>

#include <s3c2440.h>
#include <s3c24x0_usb.h>
#include "usb_ctl.h"
#include <asm/sizes.h>

/* error number from linux */
#define EINTR            4	/* Interrupted system call */
#define EAGAIN          11	/* Try again */
#define EBUSY           16	/* Device or resource busy */
#define ENODEV          19	/* No such device */


//#define USB_DEBUG 1

#ifdef USB_DEBUG
#define LOG(arg...) printf(__FILE__":"__FUNCTION__"(): " ##arg)
#else
#define LOG(arg...) (void)(0)
#endif

static unsigned char *ep1_buf;
static unsigned int ep1_len;
static usb_callback_t ep1_callback;
static unsigned int ep1_curdmalen;
static unsigned int ep1_remain;
static unsigned int dmachn_rx;
static unsigned int rx_pktsize;
static unsigned int last_packet;

extern unsigned int usbd_dn_cnt;
extern unsigned int usbd_dn_addr;
extern int DNW;

int got_header = 0;
union {
	unsigned char data[9];
	unsigned int val[2];
} hdr;






void ep1_dma_callback(void *buf_id, int size)
{
	LOG("\n");

}

static void ep1_start(void)
{
	LOG("\n");
}

static void ep1_done(int flag)
{
	int size = ep1_len - ep1_remain;

	LOG(" start\n");

	if (!ep1_len)
		return;
	ep1_len = ep1_curdmalen = 0;

#ifndef CONFIG_S3C2410_OYSTER
	if (ep1_callback) {
		ep1_callback(flag, size);
	}
#endif
	LOG(" end\n");
}


void ep1_stall(void)
{
	LOG(" start\n");

	UD_INDEX = UD_INDEX_EP1;
	UD_OCSR1 |= UD_OCSR1_SENDSTL;	/* send stall, force stall */
	LOG(" end\n");
}

int ep1_init(int chn)
{
	desc_t *pd = s3c24x0_usb_get_descriptor_ptr();

	LOG(" start\n");

	rx_pktsize = __le16_to_cpu(pd->b.ep1.wMaxPacketSize);

	LOG("rx_pktsize = %d\n", rx_pktsize);

	dmachn_rx = chn;
	ep1_done(-EAGAIN);
	LOG(" end\n");
	return 0;
}

void ep1_reset(void)
{
	desc_t *pd = s3c24x0_usb_get_descriptor_ptr();
	rx_pktsize = __le16_to_cpu(pd->b.ep1.wMaxPacketSize);

	LOG("rx_pktsize = %d\n", rx_pktsize);

	UD_INDEX = UD_INDEX_EP1;
	UD_OCSR1 &= ~(UD_OCSR1_SENDSTL);

	ep1_done(-EINTR);
}


static void check_eof(int len)
{
	int unit;
	int index;
	unsigned short cal = 0, rec;
	unit = UD_MAXP * 8;
//	/* DEBUGGING: SW.LEE
//	if ( usbd_dn_cnt > 103500) printf("usbd_dn_cnt %d \n",usbd_dn_cnt);
//	*/
	if (DNW) {
		if ( (hdr.val[1] == (usbd_dn_cnt + 8)) ){
			rec=  *(unsigned short *)(usbd_dn_addr+usbd_dn_cnt-2);
			printf("Recevie CRC: Received 0x%x \n",rec);
			for (index = usbd_dn_addr + 8 ; index < hdr.val[1] ;index++)
				cal +=  *((unsigned char *)(index));
			printf("Calculated CRC: 0x%x bytes \n",cal);
		}
	}
	
	if (0 == (usbd_dn_cnt % SZ_64K))
		printf("*");

	if (len != unit) {
		printf("\nDownload done: Received %d(0x%x) bytes \n",
		       usbd_dn_cnt, usbd_dn_cnt);
		printf("Received data is located to 0x%08X \n",
		       usbd_dn_addr);
		got_header = 0;
		usbd_dn_cnt = 0;
	}
}


void ep1_int_hndlr(int udcsr)
{
	//dma_addr_t dma_addr;
	unsigned int len;
	int status;
	int recv_cnt;
	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP1;
	}
	status = UD_OCSR1;

	LOG("0x%02x\n", UD_OCSR1);
	LOG("0x%02x\n", UD_OCSR2);
	LOG("0x%02x\n", UD_ICSR1);
	LOG("0x%02x\n", UD_ICSR2);
	if (status & UD_OCSR1_SENTSTL) {
		if (DNW) {
			UD_INDEX = UD_INDEX_EP3;
		} else {
			UD_INDEX = UD_INDEX_EP1;
		}
		UD_OCSR1 &= ~UD_OCSR1_SENTSTL;
		LOG("UD_OCSR1_SENTSTL\n");
		return;
	}

	if (status & UD_OCSR1_PKTRDY) {
		LOG("ep1_len=%x\n", ep1_len);

#if 0
		if (!ep1_len) {
			printf("usb_recv: RPC for non-existent buffer\n");
			UD_INDEX = UD_INDEX_EP1;
			UD_OCSR1 |= UD_OCSR1_FFLUSH;
			//udelay(20);
			UD_INDEX = UD_INDEX_EP1;
			UD_OCSR1 &= ~UD_OCSR1_PKTRDY;
			return;
		}
#endif
		if (DNW) {
			UD_INDEX = UD_INDEX_EP3;
		} else {
			UD_INDEX = UD_INDEX_EP1;
		}
	
		recv_cnt = ((UD_OFCNTH << 8) | UD_OFCNTL) & 0xffff;

		if (DNW) {
			if (!got_header) {
				for (len = 0; len < 8; len++)
					hdr.data[len] = (u_char) UD_FIFO3;
				hdr.data[len] = '\0';
				usbd_dn_addr = hdr.val[0];
				last_packet = hdr.val[1]/64;
				printf("\nDownload address:: 0x%X Size:: %ubytes\n", usbd_dn_addr, hdr.val[1] - 10);
				//usbd_dn_cnt = 0;
			}
			for (len = 0;
			     len < recv_cnt - 8 * (1 - got_header);
			     len++) {
				//ep1_buf[len] = (u_char) UD_FIFO3;
				*(volatile unsigned char *) (usbd_dn_addr +
							     usbd_dn_cnt) =
				    (u_char) UD_FIFO3;
				usbd_dn_cnt++;
				LOG("ep1_buf[%d]=%x\n", len, ep1_buf[len]);
			}
			last_packet--;
			got_header = 1;
		} else {
			LOG("recv_count = %d\n", recv_cnt);
			for (len = 0; len < recv_cnt; len++) {
				//ep1_buf[len] = (u_char) UD_FIFO1;
				*(volatile unsigned char *) (usbd_dn_addr +
							     usbd_dn_cnt) =
				    (u_char) UD_FIFO1;
				usbd_dn_cnt++;
				LOG("ep1_buf[%d]=%x\n", len, ep1_buf[len]);
			}

			UD_INDEX = UD_INDEX_EP1;
			UD_OCSR1 &= ~(0xFF); //shaju to clear
			if(((UD_OCSR1&0x01) == 1) && ((UD_INT &0x8)==0) )
			{
			
			UD_INDEX = UD_INDEX_EP1;
			recv_cnt = ((UD_OFCNTH << 8) | UD_OFCNTL) & 0xffff;
				for (len = 0; len < recv_cnt; len++) {
				//ep1_buf[len] = (u_char) UD_FIFO1;
				*(volatile unsigned char *) (usbd_dn_addr +
							     usbd_dn_cnt) =
				    (u_char) UD_FIFO1;
				usbd_dn_cnt++;
				LOG("ep1_buf[%d]=%x\n", len, ep1_buf[len]);
			}

			UD_INDEX = UD_INDEX_EP1;
		UD_OCSR1 &= ~(0xFF); //shaju to clear

		}

		check_eof(recv_cnt);
			
		}


		//      ep1_remain -= recv_cnt;
		//      ep1_done(0);
		if (DNW) {
			UD_INDEX = UD_INDEX_EP3;
		UD_OCSR1 &= ~(0xFF); //shaju to clear
		}
		
	     if(DNW){
		/* H/w bug workaround */
		if(!last_packet){
                   if(hdr.val[1]%64){
		      UD_INDEX = UD_INDEX_EP3;
                      while(!(UD_OCSR1&0x1));
                      len = ((UD_OFCNTH << 8) | UD_OFCNTL) &0xffff;
                      if(len < hdr.val[1]%64)
                        printf("File Download Should Be Aborted !!!\n");
                   }

                   for(len = hdr.val[1]%64; len; len--){        //We read only what we need
                        *(volatile unsigned char *)(usbd_dn_addr +usbd_dn_cnt) = (u_char) UD_FIFO3;
                        usbd_dn_cnt++;
                   }

                   UD_INDEX = UD_INDEX_EP3;
                   UD_OCSR1 |= UD_OCSR1_FFLUSH;

                   if(usbd_dn_cnt != (hdr.val[1] - 8)){
                      printf("Size=%d usbd_dn_cnt=%d ", hdr.val[1], usbd_dn_cnt);
                   }else{
                      hdr.val[0] = 0;
                      hdr.val[1] = 0;
                      hdr.data[0] = *(volatile unsigned char *)(usbd_dn_addr + usbd_dn_cnt - 1);
                      hdr.data[1] = *(volatile unsigned char *)(usbd_dn_addr + usbd_dn_cnt - 2);
                      printf("Checksum of received file = 0x%X ", hdr.val[0]);
                   }
		     printf("usbd_dn_cnt :: 0x%X \n", usbd_dn_cnt);	   
                   usbd_dn_cnt = got_header = 0;
                }
	     }
	}
}

int s3c24x0_usb_recv(char *buf, int len, usb_callback_t callback)
{

	LOG("\n");

	if (ep1_len)
		return -EBUSY;

	//local_irq_save(flags);
	ep1_buf = buf;
	ep1_len = len;
	ep1_callback = callback;
	ep1_remain = len;

	ep1_start();
	// local_irq_restore(flags);

	return 0;
}


void s3c24x0_usb_recv_reset(void)
{
	LOG("\n");
	ep1_reset();
}


void s3c24x0_usb_recv_stall(void)
{
	LOG("\n");
	ep1_stall();
}
