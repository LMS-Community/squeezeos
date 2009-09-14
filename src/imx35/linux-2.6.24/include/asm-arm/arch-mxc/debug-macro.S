/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <asm/arch/hardware.h>

        .macro  addruart,rx
                mrc     p15, 0, \rx, c1, c0
                tst     \rx, #1                 /* MMU enabled? */
#ifdef CONFIG_SERIAL_8250_CONSOLE
                ldreq   \rx, =MXC_LL_EXTUART_PADDR /* Physical Addr of EXT UART A */
                ldrne   \rx, =MXC_LL_EXTUART_VADDR /* Virtual Addr */
#else
                ldreq   \rx, =MXC_LL_UART_PADDR /* Physical Addr of MXC UART */
                ldrne   \rx, =MXC_LL_UART_VADDR /* Virtual Addr */
#endif

        .endm

        .macro  senduart,rd,rx
#ifdef CONFIG_SERIAL_8250_CONSOLE
#ifdef MXC_LL_EXTUART_16BIT_BUS
                strh    \rd, [\rx]
#else
                strb    \rd, [\rx]
#endif
#else
                strh    \rd, [\rx, #0x40]
#endif
        .endm

        .macro  busyuart,rd,rx
        .endm

        .macro  waituart,rd,rx
#ifdef CONFIG_SERIAL_8250_CONSOLE
#ifdef MXC_LL_EXTUART_16BIT_BUS
1001:           ldrh    \rd, [\rx, #0xA]        /* LSR_TEMPT ? */
#else
1001:           ldrb    \rd, [\rx, #0x5]        /* LSR_TEMPT ? */
#endif
                tst     \rd, #0x40
#else
1001:           ldrh    \rd, [\rx, #0x98]       /* TXFE ? */
                tst     \rd, #0x4000
#endif
                beq     1001b
        .endm
