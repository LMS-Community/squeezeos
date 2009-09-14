/*
 *  Copyright (C) 2007 Lennert Buytenhek <buytenh@wantstofly.org>
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/hardware.h>

	@ this macro disables fast irq (not implemented)
	.macro	disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	@ this macro checks which interrupt occured
	@ and returns its number in irqnr
	@ and returns if an interrupt occured in irqstat
	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
	@ Can't use IO_ADDRESS macro. Otherwise compilation error
#ifdef CONFIG_MXC_TZIC
	ldr	\irqstat, =TZIC_IO_ADDRESS(TZIC_BASE_ADDR)
	@ Load offset & priority of the highest priority
	@ interrupt pending.
        @ 0xD80 is HIPND0 register
        ldr     \irqnr, =0
        ldr     \base, =0x0D80
1000:
        ldr     \tmp,   [\irqstat, \base]
        cmp     \tmp, #0
        bne     1001f
        addeq   \irqnr, \irqnr, #32
        addeq   \base, \base, #4
        cmp     \irqnr, #128
        blo     1000b
        b       2001f
1001:   ldr     \base, =1
1002:   tst     \tmp, \base
        bne     2002f
        movs    \base, \base, lsl #1
        addne   \irqnr, \irqnr, #1
        bne     1002b
2001:
        ldr  \irqnr, =0
2002:
        movs \irqnr, \irqnr
#else
	ldr	\base, =AVIC_IO_ADDRESS(AVIC_BASE_ADDR)
	@ Load offset & priority of the highest priority
	@ interrupt pending from AVIC_NIVECSR
	ldr	\irqstat, [\base, #0x40]
	@ Shift to get the decoded IRQ number, using ASR so
	@ 'no interrupt pending' becomes 0xffffffff
	mov	\irqnr, \irqstat, asr #16
	@ set zero flag if IRQ + 1 == 0
	adds	\tmp, \irqnr, #1
#endif  /*MXC_SUPPORT_TZIC*/
	.endm

	@ irq priority table (not used)
	.macro	irq_prio_table
	.endm
