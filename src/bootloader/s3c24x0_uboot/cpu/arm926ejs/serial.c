/*
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 *
 * wjluv changes serial function primitives for SMDK24A0
 *
 * date: Mach 15, 2004
 *
 * Modified functions: serial_setbrg, serial_getc, serial_putc, serial_tstc
 *
 */

#include <common.h>
#include <config.h>
#if defined(CONFIG_S3C2460x)
#include "asm/arch/s3c2460.h"
#elif defined(CONFIG_S3C2413)
#include "asm/arch/s3c2413.h"
#endif

#if defined(CONFIG_S3C2460x) || defined(CONFIG_S3C2413)

	#ifdef CONFIG_SERIAL_UART0
#define SERIAL_CHAR_READY()     (UTRSTAT0 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH0
#define SERIAL_READ_STATUS()    (UERSTAT0 & UART_ERR_MASK)

#define SERIAL_WRITE_STATUS()   (UTRSTAT0)
#define SERIAL_WRITE_READY()    ((UTRSTAT0) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)    ((UTXH0) = (c))

	#elif defined(CONFIG_SERIAL_UART1)
#define SERIAL_CHAR_READY()     (UTRSTAT1 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH1
#define SERIAL_READ_STATUS()    (UERSTAT1 & UART_ERR_MASK)

#define SERIAL_WRITE_STATUS()   (UTRSTAT1)
#define SERIAL_WRITE_READY()    ((UTRSTAT1) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)    ((UTXH1) = (c))

	#elif defined(CONFIG_SERIAL_UART2)
#define SERIAL_CHAR_READY()     (UTRSTAT2 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH2
#define SERIAL_READ_STATUS()    (UERSTAT2 & UART_ERR_MASK)

#define SERIAL_WRITE_STATUS()   (UTRSTAT2)
#define SERIAL_WRITE_READY()    ((UTRSTAT2) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)    ((UTXH2) = (c))

	#else
#error not support this serial port
	#endif

#else
#include "proc.h"
#endif

void serial_setbrg (void)
{
	int i;
	for (i = 0; i < 100; i++);
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
int serial_init (void)
{
	serial_setbrg ();

	return (0);
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
int serial_getc (void)
{
       char c;
        unsigned long rxstat;

        while (!SERIAL_CHAR_READY()) /* nothing */;

        c = SERIAL_READ_CHAR();

        /* FIXME: 여기서 에러 핸들링이 필요함 */
        rxstat = SERIAL_READ_STATUS();
        return c;

}

#ifdef CONFIG_HWFLOW
static int hwflow = 0; /* turned off by default */
int hwflow_onoff(int on)
{
	switch(on) {
	case 0:
	default:
		break; /* return current */
	case 1:
		hwflow = 1; /* turn on */
		break;
	case -1:
		hwflow = 0; /* turn off */
		break;
	}
	return hwflow;
}
#endif

#ifdef CONFIG_MODEM_SUPPORT
static int be_quiet = 0;
void disable_putc(void)
{
	be_quiet = 1;
}

void enable_putc(void)
{
	be_quiet = 0;
}
#endif


/*
 * Output a single byte to the serial port.
 */
void serial_putc (const char c)
{

	while (!SERIAL_WRITE_READY());
	SERIAL_WRITE_CHAR(c);

	/* If \n, also do \r */
	if (c == '\n')
		serial_putc ('\r');
}

/*
 * Test whether a character is in the RX buffer
 */
int serial_tstc (void)
{

        return SERIAL_CHAR_READY();
}

void
serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

