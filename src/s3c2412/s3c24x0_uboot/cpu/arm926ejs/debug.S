/*
 *  vivi/arch/s3c24a0/debug.S
 *
 *  Based on linux/arch/arm/kernel/debug-armv.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  32-bit debugging code
 */

#include <config.h>
#include "smdk.h"



#define SYMBOL_NAME_STR(X) #X
#define SYMBOL_NAME(X) X
#ifdef __STDC__
#define SYMBOL_NAME_LABEL(X) X##:
#else
#define SYMBOL_NAME_LABEL(X) X/**/:
#endif

#define __ALIGN .align 0
#define __ALIGN_STR ".align 0"

#define ALIGN __ALIGN
#define ALIGN_STR __ALIGN_STR

#define ENTRY(name) \
  .globl SYMBOL_NAME(name); \
  ALIGN; \
  SYMBOL_NAME_LABEL(name)

#define CONFIG_SERIAL_DEFAULT_BAUDRATE 115200

		.text

#if defined(CONFIG_S3C24A0A_DEBUG)
		.macro	_inituart,rx
		mov	r0, \rx
		mov	r1, #0x0
		str	r1, [r0, #0x08]
		str	r1, [r0, #0x0C]
		mov	r1, #0x3
		str	r1, [r0, #0x00]
		ldr	r1, ucon_val
		str	r1, [r0, #0x04]
		ldr	r1, ubrdiv_val
		str	r1, [r0, #0x28]
		.endm
		.align 2
ucon_val:	.long	0x245
ubrdiv_val:	.long	((UART_REF_CLK / (CONFIG_SERIAL_DEFAULT_BAUDRATE * 16)) - 1)

		.macro	adduart,rx
		mov	\rx, #0x44000000
		orr	\rx, \rx, #0x00400000
		.endm

		.macro	senduart,rd,rx
		str	\rd, [\rx, #0x20]
		.endm

		.macro	busyuart,rd,rx
		nop
		.endm

		.macro	waituart,rd,rx
		mov	\rd, \rx
1001:		ldr	\rd, [\rx, #0x10]
		and	\rd, \rd, #UTRSTAT_TX_EMPTY
		tst	\rd, #UTRSTAT_TX_EMPTY
		beq	1001b
		.endm
#else
#error Unknown architecture
#endif

/*
 * Useful debugging routines
 */
#ifdef CONFIG_S3C24A0A_DEBUG
ENTRY(ledon)
		mov	r2, r0, lsl #4
		ldr	r0, gpio_base_addr
		ldr	r1, [r0, #0xc]
		bic	r1, r1, r2
		str	r1, [r0, #0xc]
		mov	pc, lr

ENTRY(ledoff)
		mov	r2, r0, lsl #4
		ldr	r0, gpio_base_addr
		ldr	r1, [r0, #0xc]
		orr	r1, r1, r2
		str	r1, [r0, #0xc]
		mov	pc, lr

gpio_base_addr:	.long	0x44800000
#endif

ENTRY(printhex8)
		mov	r1, #8
		b	printhex

ENTRY(printhex4)
		mov	r1, #4
		b	printhex

ENTRY(printhex2)
		mov	r1, #2
printhex:	adr	r2, hexbuf
		add	r3, r2, r1
		mov	r1, #0
		strb	r1, [r3]
1:		and	r1, r0, #15
		mov	r0, r0, lsr #4
		cmp	r1, #10
		addlt	r1, r1, #'0'
		addge	r1, r1, #'a' - 10
		strb	r1, [r3, #-1]!
		teq	r3, r2
		bne	1b
		mov	r0, r2
		b	printascii

		.ltorg

ENTRY(printascii)
		adduart  r3
		b	2f
1:		waituart r2, r3
		senduart r1, r3
		busyuart r2, r3
		teq	r1, #'\n'
		moveq	r1, #'\r'
		beq	1b
2:		teq	r0, #0
		ldrneb	r1, [r0], #1
		teqne	r1, #0
		bne	1b
		mov	pc, lr

ENTRY(printch)
		adduart r3
		mov	r1, r0
		mov	r0, #0
		b	1b

ENTRY(inituart)
		_inituart r0
		mov	pc, lr

hexbuf:		.space 16
