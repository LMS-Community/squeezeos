/* 
 * nand_cp.c: Copy from nand to destination
 *
 * Copyright (C) 2003	
 * Gianluca Costa <gcoata@libero.it>
 *
 * based on vivi boot (Mizi)
 *
 * Copyright (C) Samsung Electronics
 * Supporting and S3C24A0A
 *              SW.LEE <hitchcar@samsung.com> 
 *              - Don't use Reset func
 *              - 3 address cycle
 *              - add trans to software mode
 *              - Always make sure that nand_cp exist in 4 kbytes in linker script
 * Under GPLver2
 */

#include <common.h>
#ifdef CONFIG_S3C2460x
#include <asm/arch/s3c2460.h>
#elif defined (CONFIG_S3C2413)
#include <asm/arch/s3c2413.h>
#else
#include <asm/arch/s3c24a0.h>
#endif

#if defined(CONFIG_S3C2413)
#include <configs/smdk2413nand.h>
#include <linux/mtd/nand.h>
#define nand_clear_RnB()	  	(NFSTAT |= (1 << 4))
#define nand_wait()             { while(!(NFSTAT & (1 << 4))); }
void nand_select(void)
{
	if (NFCONF & (1<<31)) NFCONT &= ~(1<<1);
	else NFCONT &= ~(1<<2);
}

void nand_deselect(void)
{
	if (NFCONF & (1<<31)) NFCONT |= (1<<1);
	else NFCONT |= (1<<2);
}
#else
#include <configs/smdk24a0nand.h>
#include <linux/mtd/nand.h>
#define nand_select()           (NFCONT &= ~(1 << 7))
#define nand_deselect()         (NFCONT |= (1 << 7))
#define nand_clear_RnB()  		(NFSTAT |= (1 << 13))
#define nand_wait()             { while(!(NFSTAT & (1 << 13))); }
#endif

/*
 * address format
 * --------------------------------------------
 * | block(12bit) | page(5bit) | offset(9bit) |
 * --------------------------------------------
 */

int nandll_read_page(unsigned char *buf, unsigned long addr)
{
        int i;
#ifdef S3C24X0_16BIT_NAND
	unsigned short *ptr16 = (unsigned short *)buf;
#endif

        if (addr & NAND_PAGE_MASK) {
                return -1;      /* invalid alignment */
        }

        nand_select();
        nand_clear_RnB();

        NFCMD = NAND_CMD_READ0;

        /* Write Address */
        NFADDR = addr & 0xff;
        NFADDR = (addr >> 9) & 0xff;
        NFADDR = (addr >> 17) & 0xff;
#ifdef NAND_3_ADDR_CYCLE
#else
        NFADDR = (addr >> 25) & 0xff;
#endif
        nand_wait();

#ifdef S3C24X0_16BIT_NAND
        for(i=0; i < NAND_PAGE_SIZE/2; i++) {
                *ptr16 = NFDATA16;
                ptr16++;
        }
#else
	for(i=0; i < NAND_PAGE_SIZE; i++) {
                *buf = NFDATA8;
		buf++;
        }
#endif

        nand_deselect();
        return 0;
}

int nandll_read_oob(char *buf, unsigned long addr)
{
        int i;
#ifdef S3C24X0_16BIT_NAND
	unsigned short *ptr16 = (unsigned short *)buf;
#endif

        if ((addr & NAND_BLOCK_MASK)) {
                return -1;      /* invalid alignment */
        }

        /* Chip Enable */
        nand_select();
		nand_clear_RnB();
        /* READOOB */
        NFCMD = NAND_CMD_READOOB;

        /* Write Address */
        NFADDR = addr & 0xff;
        NFADDR = (addr >> 9) & 0xff;
        NFADDR = (addr >> 17) & 0xff;
#ifdef NAND_3_ADDR_CYCLE
#else
        NFADDR = (addr >> 25) & 0xff;
#endif
        nand_wait();
#ifdef S3C24X0_16BIT_NAND
        for(i=0; i < NAND_OOB_SIZE/2; i++) {
                *ptr16 = NFDATA16;
		ptr16++;
        }
#else
	for(i=0; i < NAND_OOB_SIZE; i++) {
                *buf = NFDATA8;
		buf++;
        }
#endif

        nand_deselect();
        return 0;
}

int is_bad_block(unsigned long addr)
{
	char oob_buf[16];

	nandll_read_oob((char *)oob_buf, addr);   /*culprit*/
#ifdef S3C24X0_16BIT_NAND
	if ( (oob_buf[0] != 0xFF) ||  (oob_buf[1] != 0xFF) )
		return 1;
#else
	if (oob_buf[5] != 0xFF)
		return 1;
#endif
	else
		return 0;
}

/*
 * Read data from NAND.
 */
int nandll_read_blocks(unsigned long dst_addr, unsigned long src_addr, int size)
{
        char *buf = (char *)dst_addr;
        char page_buf[NAND_PAGE_SIZE];
        int i, j;

        if ((src_addr & NAND_BLOCK_MASK) || (size & NAND_PAGE_MASK)) {
		printf("Alignment Error \n");
                return -1;      /* invalid alignment */
	}
        while (size > 0) {
                /* If this block is bad, go next block */
                if (is_bad_block(src_addr)) {
                        src_addr += NAND_BLOCK_SIZE;
                        continue;
                }
                /* Read block */
                for (i = 0; i < NAND_PAGES_IN_BLOCK; i++) {
                        nandll_read_page((char *)page_buf, src_addr);
                        for (j = 0; j < NAND_PAGE_SIZE; j++)
                                *buf++ = page_buf[j];
                        src_addr += NAND_PAGE_SIZE;
                        size -= NAND_PAGE_SIZE;
                        if (size == 0) break;
                }
        }

        return 0;
}


int copy_uboot_to_ram(void)
{
#if	defined(CONFIG_S3C2460x)
	NFCONF |= 0xFFF0; /* Max TACLS, TWRPH0, TWRPH1 */ /*Line taken from arm920t/nand_cp.c */
	/* Enter into NAND Software Controll Mode */
	NFCONT |= 0x3;
#elif defined (CONFIG_S3C2413)
	BANK_CFG |= (0x3<<2);
	GPACON &= ~(0x1f<<17);
    GPACON |= (0x1f<<17);

	NFCONF |= 0x7770;
	NFCONT |= 0xf7;
#else
	NFCONT |= 0x3;
#endif

	/* Copy uboot to ram. 128k is enough. */
	return nandll_read_blocks(UBOOT_BASE, 0x0, (8*NAND_BLOCK_SIZE));
}

/*
	__asm__ (
		"mov r2, #0x500;"
	"mov	r2, #0x600;"
	"mov	r0,#0x44000000;"
	"orr	r0, r0,#0x800000;"
	"orr	r0, r0,#0x94;"
	"ldr	r1, [r0, #0x0];"
	"bic	r1, r1, #0xf00;"
	"orr	r1, r1, r2;"
	"str	r1, [r0, #0x0];"
	"b	.;"
	);
*/
