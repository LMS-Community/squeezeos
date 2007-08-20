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
#include <asm/arch/s3c2413.h>


static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n"
			  "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}


static void ref_port(void)
{
	/* set up the I/O ports */
	GPACON = 0x007FFFFF;
	GPBCON = 0x00044555;
	GPBUP = 0x000007FF;
	GPCCON = 0xAAAAAAAA;
	GPCUP = 0x0000FFFF;
	GPDCON = 0xAAAAAAAA;
	GPDDN = 0x0000FFFF;
	GPECON = 0xAAAAAAAA;
	GPEDN = 0x0000FFFF;
	GPFCON = 0x00000055; // GPF0->3 as OUTPUT GPF 4->GPF7 as INPUT
	GPFDN = 0x0000000F; // Pull down disabled on outputs
	GPGCON = 0xFF95FFBA;
	GPGDN = 0x0000FFFF;
	GPHCON = 0x002AFAAA;
	GPHDN = 0x000007FF;


#if 1				/* USB Device Part */
	/*GPGCON is reset for USB Device */
	GPGCON = (GPGCON & ~(3 << 24)) | (1 << 24);	/* Output Mode */
	GPGDN = GPGDN | (1 << 12);	/* Pull up disable */

	GPGDAT |= (1 << 12);
	GPGDAT &= ~(1 << 12);
	udelay(20000);
	GPGDAT |= (1 << 12);
					/* NAND Device Part */
	GPACON &= ~(0x1f<<17);
    GPACON |= (0x1f<<17);
#endif


    	GPGCON	= (GPGCON&0xfffffcff)|0x00000100;		//yamu board port G4 lcd power on 
    	GPGDN	|= (0x1<<4); // pull-down disable	
    	GPGDAT = (GPGDAT & ~(0x1<<4)) | ((1 & 0x1)<<4) ;	//port G4

}

/*
 * Miscellaneous platform dependent initialisations
 */

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
	ref_port();

	/* arch number of SMDK2440-Board */
	gd->bd->bi_arch_number = 193;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x30000100;

	icache_enable();
	dcache_enable();


	return 0;
}

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
extern ulong nand_probe(ulong physadr);

void nand_init(void)
{
	s3c24x0_nand_init();
}
#endif


int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}










