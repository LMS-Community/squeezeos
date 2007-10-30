/*
 * (C) Copyright 2001-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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

/* This code should work for both the S3C2400 and the S3C2410
 * as they seem to have the same PLL and clock machinery inside.
 * The different address mapping is handled by the s3c24xx.h files below.
 */

/*
 *
 * wjluv changes clock related function primitives for SMDK24A0
 *
 * date: Mach 15, 2004
 *
 * Modified functions: get_PLLCLK, get_HCLK, get_PCLK
 *
 */


#include <common.h>
#if defined (CONFIG_S3C2413)
#include <asm/arch/s3c2413.h>
#else
#include <asm/arch/s3c24a0.h>
#endif

#define MPLL 0
#define UPLL 1


#define GET_PCLK        0
#define GET_HCLK        1

#define FIN     12000000

#if defined (CONFIG_S3C2413)
#define CLKDIVN         __REG(0x4C000013)
#else
#define CLKDIVN         __REG(0x40000028)
#endif



/* ------------------------------------------------------------------------- */
/* NOTE: This describes the proper use of this file.
 *
 * CONFIG_SYS_CLK_FREQ should be defined as the input frequency of the PLL.
 *
 * get_FCLK(), get_HCLK(), get_PCLK() and get_UCLK() return the clock of
 * the specified bus in HZ.
 */
/* ------------------------------------------------------------------------- */

static ulong get_PLLCLK(int pllreg)
{

        unsigned long val = MPLLCON;
        return (((GET_MDIV(val) + 8) * FIN) / \
                ((GET_PDIV(val) + 2) * (1 << GET_SDIV(val))));

}

/* return FCLK frequency */
ulong get_FCLK(void)
{
    return(get_PLLCLK(MPLL));
}

/* return HCLK frequency */
ulong get_HCLK(void)
{
    return((CLKDIVN & 0x2) ? get_FCLK()/2 : get_FCLK());
}

/* return PCLK frequency */
ulong get_PCLK(void)
{
    return((CLKDIVN & 0x1) ? get_HCLK()/2 : get_HCLK());
}

/* return UCLK frequency */
ulong get_UCLK(void)
{
    return(get_PLLCLK(UPLL));
}
