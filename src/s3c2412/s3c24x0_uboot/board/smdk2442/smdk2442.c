/*
 * (C) Copyright 2004 Samsung Electronics
 *       SW.LEE <hitchcar@samsung.com> :Only For S3C2440
 *
 * 	 -added KingFish Port Setup
 *       -added MAX1718 Voltage Setup
 *       -added ref_port Setup
 * 
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <s3c2440.h>


static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n"
			  "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}


#ifdef KINGFISH
static void KingFish_port(void)
{
	GPACON = 0x7fffff;

	GPBCON = 0x055545;
	GPBUP = 0x7fb;		// The pull up function is disabled GPB[10:0], enabled for GPB[2] (enable backlignt)
	GPBDAT |= (1 << 1);	// IrDA shutdown (SD high)

#ifdef KINGFISH3
	GPBDAT |= (1 << 9);     // Regulator ON
#endif
	GPCCON = 0xaaa5aaaa;
	GPCUP = 0xffff;		// The pull up function is disabled GPC[15:0] , enabled GPC[8] 
	GPCDAT |= (1 << 9);	//USB charger SELI high(500mA)

	GPDDAT &= ~(1 << 8);	//USB charger disable
	GPDCON = 0xaaa5aaa0;
	GPDUP = 0xfffc;		// The pull up function is disabled GPD[15:2] , enabled GPD[1:0]

	GPECON = 0xaaaaaaaa;
	GPEUP = 0xffff;		// The pull up function is disabled GPE[15:0]

	GPFCON = 0xaaaa;
	GPFUP = 0xff;		// The pull up function is disabled GPF[7:0]

	GPGCON = 0xaaaaabba;
	GPGUP = 0xffff;		// The pull up function is disabled GPG[15:0]

	GPHCON = 0x29aaaa;
	GPHUP = 0x7ff;		// The pull up function is disabled GPH[10:0]

	GPJDAT = (1 << 12) | (0 << 11);
	GPJCON = 0x016aaaa;
	GPJUP = ~((0 << 12) | (1 << 11));

	GPJDAT = (0 << 12) | (0 << 11);
	GPJCON = 0x016aaaa;
	GPJUP = 0x1fff;

	DSC0 = 0;
	DSC1 = 0;

	/* USB Device Reset */
	CLKCON |= CLKCON_USBD;

#if 0
	GPBCON = (GPBCON & ~(3 << 20)) | (1 << 20);
	GPBUP |= (1 << 10);
	GPBDAT |= (1 << 10);
	GPBDAT &= ~(1 << 10);
	udelay(20000);
	GPBDAT |= (1 << 10);

	MISCCR = MISCCR & ~(1 << 3);
	MISCCR = MISCCR & ~(1 << 13);
#else

	GPBCON = (GPBCON & ~(3 << 20)) | (1 << 20);
	GPBDAT = (GPBDAT & ~(1 << 10)) | (0 << 10);	/* set the output to 0 */
	{
		int i;
		for (i = 0; i < 100; i++);
	}
	GPBCON = (GPBCON & ~(3 << 20)) | (0 << 20);	/* set to input mode */

#endif

}
#else
static void ref_port(void)
{
	/* set up the I/O ports */
	GPACON = 0x007FFFFF;
	GPBCON = 0x00044555;
	GPBUP = 0x000007FF;
	GPCCON = 0xAAAAAAAA;
	GPCUP = 0x0000FFFF;
	GPDCON = 0xAAAAAAAA;
	GPDUP = 0x0000FFFF;
	GPECON = 0xAAAAAAAA;
	GPEUP = 0x0000FFFF;
	GPFCON = 0x000055AA;
	GPFUP = 0x000000FF;
	GPGCON = 0xFF95FFBA;
	GPGUP = 0x0000FFFF;
	GPHCON = 0x002AFAAA;
	GPHUP = 0x000007FF;

#if 1				/* USB Device Part */
	/*GPGCON is reset for USB Device */
	GPGCON = (GPGCON & ~(3 << 24)) | (1 << 24);	/* Output Mode */
	GPGUP = GPGUP | (1 << 12);	/* Pull up disable */

	GPGDAT |= (1 << 12);
	GPGDAT &= ~(1 << 12);
	udelay(20000);
	GPGDAT |= (1 << 12);
#endif


}
#endif


static void Max1718_Set(int voltage)
{

	int vtg;
	//////////////////////////////////////////////
	//   D4    D3  D2   D1     D0
	//      0        1      0       0        0              // 1.35V
	//      0        1      0       0        1              // 1.30V
	//      0        1      0       1        0              // 1.25V
	//      0        1      0       1        1              // 1.20V
	//      0        1      1       0        0              // 1.15V
	//      0        1      1       0        1              // 1.10V
	//      0        1      1       1        0              // 1.05V
	//      0        1      1       1        1              // 1.00V
	//      1        0      0       0        1              // 0.95V
	//      1        0      0       1        1              // 0.90V
	//      1        0      1       0        1              // 0.85V
	//      1        0      1       1        1              // 0.80V

	vtg = voltage;
	GPBCON =
	    (GPBCON & ~((3 << 20) | (3 << 16) | (3 << 14))) | (1 << 20) |
	    (1 << 16) | (1 << 14);
	// GPB7, 8, 10 : Output
	GPFCON = (GPFCON & ~(0xff << 8)) | (0x55 << 8);	// GPF4~7: Output , shared with LED4~7

	switch (vtg) {
	case 135:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4);	//D3~0
		break;
	case 130:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (0 << 5) | (1 << 4);	//D3~0
		break;
	case 125:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (1 << 5) | (0 << 4);	//D3~0
		break;
	case 120:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (1 << 5) | (1 << 4);	//D3~0
		break;
	case 115:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (1 << 6) | (0 << 5) | (0 << 4);	//D3~0
		break;

	case 110:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (1 << 6) | (0 << 5) | (1 << 4);	//D3~0
		break;

	case 105:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (1 << 6) | (1 << 5) | (0 << 4);	//D3~0
		break;

	case 100:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4);	//D3~0
		break;

	case 95:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (1 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (0 << 7) | (0 << 6) | (0 << 5) | (1 << 4);	//D3~0
		break;

	case 90:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (1 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (0 << 7) | (0 << 6) | (1 << 5) | (1 << 4);	//D3~0
		break;
	case 85:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (1 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (0 << 7) | (1 << 6) | (0 << 5) | (1 << 4);	//D3~0
		break;
	case 80:
		GPBDAT = (GPBDAT & ~(1 << 7)) | (1 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (0 << 7) | (1 << 6) | (1 << 5) | (1 << 4);	//D3~0
		break;
	default:		// 1.2V
		GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);	//D4
		GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (1 << 5) | (1 << 4);	//D3~0
		break;

	}

	GPBDAT &= ~(1 << 8);	//Latch enable
	GPBDAT |= (1 << 10);	//Output enable
	GPBDAT |= (1 << 8);	//Latch disable
}


/*
 * Miscellaneous platform dependent initialisations
 */

extern void fill_nandinfo(void);
extern void nand_config(void); 
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	/* to reduce PLL lock time, adjust the LOCKTIME register */
	LOCKTIME = 0xFFFFFF;

	/* configure UPLL Before setting up MPLL */
//      clk_power->UPLLCON = ((U_M_MDIV << 12) + (U_M_PDIV << 4) + U_M_SDIV);

	/* some delay between MPLL and UPLL */
//      delay (8000);

	/* configure MPLL */
//        clk_power->MPLLCON = ((MDIV_406 << 12) + (PDIV_406 << 4) + SDIV_406);

	/* some delay between MPLL and UPLL */
	delay(4000);
#ifdef KINGFISH
	KingFish_port();
#else
	ref_port();

	Max1718_Set(135); /* 300Mhz default */
#endif
	/* arch number of SMDK2440-Board */
	gd->bd->bi_arch_number = 193;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x30000100;

	icache_enable();
	dcache_enable();

#ifdef CONFIG_S3C2440A_JTAG_BOOT
	nand_config();
#endif

	fill_nandinfo();

	return 0;
}

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
extern ulong nand_probe(ulong physadr);
unsigned long page_buf_addr;

void nand_init(void)
{

#if 0
	printf("%4lu MB\n", nand_probe((ulong) nand) >> 20);
#else
	printf("64 MB : \n");

#endif
}
#endif


int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}
