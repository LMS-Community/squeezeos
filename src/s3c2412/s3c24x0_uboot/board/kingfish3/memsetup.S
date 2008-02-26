/*
 * Copyright (C) 2004 Samsung Electronics
 *            SW.LEE <hitchcar@samsung.com>
 *       I recommend that Memory initialization code be merged into start.S
 *
 * Memory Setup stuff - taken from blob memsetup.S
 *
 * Copyright (C) 1999 2000 2001 Erik Mouw (J.A.K.Mouw@its.tudelft.nl) and
 *                     Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *
 * Modified for the Samsung SMDK2410 by
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
 */


/* some parameters for the board */

/*
 *
 * Taken from linux/arch/arm/boot/compressed/head-s3c2410.S
 *
 * Copyright (C) 2002 Samsung Electronics SW.LEE  <hitchcar@sec.samsung.com>
 *
 */

#define BWSCON	0x48000000

/* BWSCON */
#define DW8		 	(0x0)
#define DW16		 	(0x1)
#define DW32		 	(0x2)
#define WAIT		 	(0x1<<2)
#define UBLB		 	(0x1<<3)

#define B1_BWSCON	  	(DW32)
#define B2_BWSCON	  	(DW16)
#define B3_BWSCON	  	(DW16 + WAIT + UBLB)
#define B4_BWSCON	  	(DW16)
#define B5_BWSCON	  	(DW16)
#define B6_BWSCON	  	(DW32)
#define B7_BWSCON	  	(DW32)

/* BANK0CON */
#define B0_Tacs		 	0x0	/*  0clk */
#define B0_Tcos		 	0x0	/*  0clk */
#define B0_Tacc		 	0x7	/* 14clk */
#define B0_Tcoh		 	0x0	/*  0clk */
#define B0_Tah		 	0x0	/*  0clk */
#define B0_Tacp		 	0x0
#define B0_PMC		 	0x0	/* normal */

/* BANK1CON */
#define B1_Tacs		 	0x0	/*  0clk */
#define B1_Tcos		 	0x0	/*  0clk */
#define B1_Tacc		 	0x7	/* 14clk */
#define B1_Tcoh		 	0x0	/*  0clk */
#define B1_Tah		 	0x0	/*  0clk */
#define B1_Tacp		 	0x0
#define B1_PMC		 	0x0

#define B2_Tacs		 	0x0
#define B2_Tcos		 	0x0
#define B2_Tacc		 	0x7
#define B2_Tcoh		 	0x0
#define B2_Tah		 	0x0
#define B2_Tacp		 	0x0
#define B2_PMC		 	0x0

#define B3_Tacs		 	0x0	/*  0clk */
#define B3_Tcos		 	0x0	/*  4clk */
#define B3_Tacc		 	0x7	/* 14clk */
#define B3_Tcoh		 	0x0	/*  1clk */
#define B3_Tah		 	0x0	/*  0clk */
#define B3_Tacp		 	0x0     /*  6clk */
#define B3_PMC		 	0x0	/*  16data */

#define B4_Tacs		 	0x0	/*  0clk */
#define B4_Tcos		 	0x0	/*  0clk */
#define B4_Tacc		 	0x7	/* 14clk */
#define B4_Tcoh		 	0x0	/*  0clk */
#define B4_Tah		 	0x0	/*  0clk */
#define B4_Tacp		 	0x0
#define B4_PMC		 	0x0	/* normal */

#define B5_Tacs		 	0x0	/*  0clk */
#define B5_Tcos		 	0x0	/*  0clk */
#define B5_Tacc		 	0x7	/* 14clk */
#define B5_Tcoh		 	0x0	/*  0clk */
#define B5_Tah		 	0x0	/*  0clk */
#define B5_Tacp		 	0x0
#define B5_PMC		 	0x0	/* normal */

#define B6_MT		 	0x3	/* SDRAM */
#define B6_Trcd	 	 	0x1
#define B6_SCAN		 	0x1	/* 8bit */

#define B7_MT		 	0x3	/* SDRAM */
#define B7_Trcd		 	0x1	/* 3clk */
#define B7_SCAN		 	0x1	/* 9bit */

/* REFRESH parameter */
#define REFEN		 	0x1	/* Refresh enable */
#define TREFMD		 	0x0	/* CBR(CAS before RAS)/Auto refresh */
#define Trp		 	0x1	/* 3clk */
#define Trc		 	0x2	/* 6clk */
#define Tchr		 	0x2	/* 3clk */
#define REFCNT		 	1269	/* period=15.6us, HCLK=60Mhz, (2048+1-15.6*60) */
/**************************************/
#define MEM_CTL_BASE		0x48000000
#define vBSWCON  (0+(B1_BWSCON<<4)+(B2_BWSCON<<8)+(B3_BWSCON<<12)+(B4_BWSCON<<16)+(B5_BWSCON<<20)+(B6_BWSCON<<24)+(B7_BWSCON<<28))
#define vBANKCON0 ((B0_Tacs<<13)+(B0_Tcos<<11)+(B0_Tacc<<8)+(B0_Tcoh<<6)+(B0_Tah<<4)+(B0_Tacp<<2)+(B0_PMC))
#define vBANKCON1 ((B1_Tacs<<13)+(B1_Tcos<<11)+(B1_Tacc<<8)+(B1_Tcoh<<6)+(B1_Tah<<4)+(B1_Tacp<<2)+(B1_PMC))
#define vBANKCON2 ((B2_Tacs<<13)+(B2_Tcos<<11)+(B2_Tacc<<8)+(B2_Tcoh<<6)+(B2_Tah<<4)+(B2_Tacp<<2)+(B2_PMC))
#define vBANKCON3 ((B3_Tacs<<13)+(B3_Tcos<<11)+(B3_Tacc<<8)+(B3_Tcoh<<6)+(B3_Tah<<4)+(B3_Tacp<<2)+(B3_PMC))
#define vBANKCON4 ((B4_Tacs<<13)+(B4_Tcos<<11)+(B4_Tacc<<8)+(B4_Tcoh<<6)+(B4_Tah<<4)+(B4_Tacp<<2)+(B4_PMC))
#define vBANKCON5 ((B5_Tacs<<13)+(B5_Tcos<<11)+(B5_Tacc<<8)+(B5_Tcoh<<6)+(B5_Tah<<4)+(B5_Tacp<<2)+(B5_PMC))
#define vBANKCON6 ((B6_MT<<15)+(B6_Trcd<<2)+(B6_SCAN))
#define vBANKCON7 ((B7_MT<<15)+(B7_Trcd<<2)+(B7_SCAN))
#define vREFRESH ((REFEN<<23)+(TREFMD<<22)+(Trp<<20)+(Trc<<18)+(Tchr<<16)+REFCNT)
#define vBANKSIZE 0xB1		/* BANKSIZE */
#define vMRSRB6 0x30		/* MRSRB6 */
#define vMRSRB7 0x30		/* MRSRB7 */
