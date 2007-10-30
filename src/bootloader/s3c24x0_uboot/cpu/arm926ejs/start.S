/*
 *  2004 (C) Samsung Electronics 
 *          SW.LEE  <hitchcar@samsung.com>
 *  	   - Add Interrupt ISR, Clock for S3C24A0A
 *
 * start.S: Initial codes are gotten from S3C24A0 firmware and vivi codes
 * ported by:	Woohyung Chun (Samsung Electronics.)
 * date:	March 15, 2004
 *
 * head.S: start-up codes for S3C24A0
 *
 *  Author:     Janghoon Lyu (Mizi Research)
 *  Created:    July 03, 2003
 *
 * Based on Memcfg.a 24a0init.s fw.s by SAMSUNG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 *
 * $Date: 2004/12/21 05:44:07 $
 * $Revision: 1.10 $
 */

#include <config.h>
#include "linkage.h"
#include "machine.h"
#include "memory.h"
#include "armv.h"

#include "regs.h"
#define CONFIG_SERIAL_DEFAULT_BAUDRATE 115200

	.text

ENTRY(_start)
ENTRY(ResetEntryPoint)		@ at 0x00000000

@ Exception vector table (located at physical address 0x00000000)

@ 0x00: Reset
	b	reset
        ldr     pc, _undefined_instruction
        ldr     pc, _software_interrupt
        ldr     pc, _prefetch_abort
        ldr     pc, _data_abort
        ldr     pc, _not_used
        ldr     pc, _irq
        ldr     pc, _fiq

_undefined_instruction: .word undefined_instruction
_software_interrupt:    .word software_interrupt
_prefetch_abort:        .word prefetch_abort
_data_abort:            .word data_abort
_not_used:              .word not_used
_irq:                   .word irq
_fiq:                   .word fiq

	.balignl 16,	UBOOT_MAGIC

/*
 * CFG_MEM_END is in the board dependent config-file (configs/config_BOARD.h)
 */
_TEXT_BASE:
        .word   TEXT_BASE

.globl _armboot_start
_armboot_start:
        .word _start


/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start
_bss_start:
        .word __bss_start

.globl _bss_end
_bss_end:
        .word _end


/*
 * Note: _armboot_end_data and _armboot_end are defined
 * by the (board-dependent) linker script.
 * _armboot_end_data is the first usable FLASH address after armboot
 */
.globl _armboot_end_data
_armboot_end_data:
        .word armboot_end_data
.globl _armboot_end
_armboot_end:
        .word armboot_end

/*
 * _armboot_real_end is the first usable RAM address behind armboot
 * and the various stacks
 */
.globl _armboot_real_end
_armboot_real_end:
        .word 0x0badc0de

#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
        .word   0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
        .word 0x0badc0de
#endif


                .macro  _inituart,rx
                mov     r0, \rx
                mov     r1, #0x0
                str     r1, [r0, #0x08]
                str     r1, [r0, #0x0C]
                mov     r1, #0x3
                str     r1, [r0, #0x00]
                ldr     r1, ucon_val
                str     r1, [r0, #0x04]
                ldr     r1, ubrdiv_val
                str     r1, [r0, #0x28]
                .endm
                .align 2
ucon_val:       .long   0x245
ubrdiv_val:     .long   ((UART_REF_CLK / (CONFIG_SERIAL_DEFAULT_BAUDRATE * 16)) - 1)

                .macro  adduart,rx
                mov     \rx, #0x44000000
                orr     \rx, \rx, #0x00400000
                orr     \rx, \rx, #0x00004000
                .endm

                .macro  senduart,rd,rx
                str     \rd, [\rx, #0x20]
                .endm

                .macro  busyuart,rd,rx
                nop
                .endm

                .macro  waituart,rd,rx
                mov     \rd, \rx
1001:           ldr     \rd, [\rx, #0x10]
                and     \rd, \rd, #UTRSTAT_TX_EMPTY
                tst     \rd, #UTRSTAT_TX_EMPTY
                beq     1001b
                .endm

ENTRY(ledon)
                mov     r2, r0, lsl #4
                ldr     r0, gpio_base_addr
                ldr     r1, [r0, #0xc]
                bic     r1, r1, r2
                str     r1, [r0, #0xc]
                mov     pc, lr

ENTRY(ledoff)
                mov     r2, r0, lsl #4
                ldr     r0, gpio_base_addr
                ldr     r1, [r0, #0xc]
                orr     r1, r1, r2
                str     r1, [r0, #0xc]
                mov     pc, lr

gpio_base_addr: .long   0x44800000

ENTRY(printhex8)
                mov     r1, #8
                b       printhex

ENTRY(printhex4)
                mov     r1, #4
                b       printhex

ENTRY(printhex2)
                mov     r1, #2
printhex:       adr     r2, hexbuf
                add     r3, r2, r1
                mov     r1, #0
                strb    r1, [r3]
1:              and     r1, r0, #15
                mov     r0, r0, lsr #4
                cmp     r1, #10
                addlt   r1, r1, #'0'
                addge   r1, r1, #'a' - 10
                strb    r1, [r3, #-1]!
                teq     r3, r2
                bne     1b
                mov     r0, r2
                b       printascii

                .ltorg

ENTRY(printascii)
                adduart  r3
                b       2f
1:              waituart r2, r3
                senduart r1, r3
                busyuart r2, r3
                teq     r1, #'\n'
                moveq   r1, #'\r'
                beq     1b
2:              teq     r0, #0
                ldrneb  r1, [r0], #1
                teqne   r1, #0
                bne     1b
                mov     pc, lr

ENTRY(printch)
                adduart r3
                mov     r1, r0
                mov     r0, #0
                b       1b

ENTRY(inituart)
                _inituart r0
                mov     pc, lr

hexbuf:         .space 16




reset:
	@ Set processor into Supervisior mode (SVC) and disable IRQ & FIQ
	mov	r0, #(SVC_MODE|F_BIT|I_BIT)
	msr	cpsr_c, r0	@ reset CPSR
#ifndef CONFIG_S3C24A0A
	bl      InitGPIO
#endif
#ifdef OREIAS
	bl      InitGPIO
#endif
	bl	PreInit
	bl	InitClock
	bl	InitMemCtl
#ifdef CONFIG_PM
	@ Check if this is a wake-up from sleep
	ldr     r0, ALIVECON_ADDR
	ldr     r1, [r0]
	tst r1, #(1<<1)
	bne     WakeupStart
#endif
	bl	InitUART

#ifdef CONFIG_DEBUG_LL
	/* for debugging */
        adr     r0, str_ok
        bl      printascii

	/* for debugging */
        adr     r0, str_boot
        bl      printascii
#endif

#ifndef CONFIG_S3C24A0A_JTAG_BOOT
	bl	copy_myself
#endif

	@ get ready to call C functions
	ldr	sp, uboot_stack_pointer	@ setup stack pointer
	mov	fp, #0			@ no previous frame, so fp=0
	mov	a2, #0			@ set argv to NULL

clear_bss:
        ldr     r0, _bss_start          /* find start of bss segment        */
        add     r0, r0, #4              /* start at first byte of bss       */
        ldr     r1, _bss_end            /* stop here                        */
        mov     r2, #0x00000000         /* clear                            */

clbss_l:str     r2, [r0]                /* clear loop...                    */
        add     r0, r0, #4
        cmp     r0, r1
        bne     clbss_l


#if 0
	bl	main			@ call main
#else
 
        bl      start_armboot           @ call main
#endif
 
_start_armboot: .word start_armboot

#ifdef CONFIG_PM
WakeupStart:
	@Go ...
	ldr r0, GPRAM0_ADDR
	ldr r1, [r0]
	mov pc, r1
	nop
	nop
	
	1:  b   1b
#endif
													
@
@ PreInit: Inialize processors
@
PreInit:
	@ Disable watchdog timer
	ldr	r0, wt_ctrl_base_addr
	mov	r1, #0x0
	str	r1, [r0, #0x0]

	@ Disable all interrupts
	ldr	r0, int_ctrl_base_addr
	mvn	r1, #0x0
	str	r1, [r0, #oINTMSK]

	@ Disable all sub-interrupts
	mov	r1, #0x700
	orr	r0, r0, #0xff
	str	r1, [r0, #oINTSUBMSK]

	@ Set all interrupts as IRQ
	mov	r1, #0x0
	str	r1, [r0, #oINTMOD]

	@ Disable LCD sub interrupts
	ldr	r0, HD_LCD_BASE_ADDR
	mov	r1, #0x3
	str	r1, [r0, #oLCDINTMSK]
	
	mov	pc, lr


@
@ InitClock: Initialize core clock and bus clock.
@
InitClock:

	mov	r0, #CLK_BASE_ADDR

	ldr	r1, vLockTime
	str	r1, [r0, #oLOCKTIME]

	ldr	r1, HD_CLKDIV
	str	r1, [r0, #oCLKDIV]

	ldr	r1, HD_MPLLCON
	str	r1, [r0, #oMPLLCON]

	mov	pc, lr

@
@ InitMemCtl: Initialize memory controller
@
InitMemCtl:

	ldr	r0, HD_SROM_BASE_ADDR
	adrl	r1, HD_SROM_VAL
	ldmia	r1!, {r2-r5}
	stmia	r0!, {r2-r5}

	ldr	r0, HD_SDRAM1_BASE_ADDR

	@ Step 1: issue precharge all command
	mov	r1, #0x1
	str	r1, [r0, #oP1BANKCON]

	@ Step 2: make refresh cycle 15clk
@shaju for k2
#ifndef OREIAS
	mov	r1, #0xf
#else
	mov	r1, #0x20
#endif
	str	r1, [r0, #oP1REFRESH]

	@ Step 3: wait 120 clk
	mov	r1, #0x100
1:	subs	r1, r1, #1
	bne	1b

	@ Step 4: set normal operation refresh cycle
	ldr	r1,  REFRESHRATES
	str	r1, [r0, #oP1REFRESH]

	@ Step 5: set cfg/ctrl/timeout registers
	ldr	r1, HD_SDRAM1_VAL
	str	r1, [r0, #0x0]
	mov	r1, #0x0
	str	r1, [r0, #oP1BANKCON]	@ memcon. WB off, Normal
	str	r1, [r0, #oP1TIMEOUT]	@ disable
	@ Step 6: MRS command
	mov	r1, #0x2
	str	r1, [r0, #oP1BANKCON]
#ifdef OREIAS
	@ Step 7: issue EMRS command (only mobile)
	mov	r1, #0x3
	str	r1, [r0, #oP1BANKCON]
#endif
	@ Step 8: Normal operation
	mov	r1, #0x0
	str	r1, [r0, #oP1BANKCON]

	mov	pc, lr

/*
 * Initialize GPIO
 */
InitGPIO:

	ldr	r0, gpio_base_addr
#ifdef OREIAS
	mov 	r1,#0xffffffff
	str	r1, [r0, #0x0c]
	bic	r1, r1, #0x10000000
	str	r1, [r0, #0x0c]
#endif
	ldr	r1, gpcon_u_val
	str	r1, [r0, #0x00]
	ldr	r1, gpcon_m_val
	str	r1, [r0, #0x04]
	ldr	r1, gpcon_l_val
	str	r1, [r0, #0x08]
	ldr	r1, gpup_val
	str	r1, [r0, #0x10]

#ifdef CONFIG_DEBUG_LL
	@ LED on only #8
	ldr	r1, [r0, #0xc]
	orr	r1, r1, #0x000000f0
	bic	r1, r1, #0x00000080
	str	r1, [r0, #0xc]
#endif
	mov	pc, lr

/*
 * Initialize UART 0
 */
InitUART:
#ifndef OREIAS
	mov	r0, #0x44000000
	orr	r0, r0, #0x00800000
	mov	r1, #0x02A80000
	str	r1, [r0, #0x00]
	
#endif 
	mov	r0, #0x44000000
	orr	r0, r0, #0x00400000
	orr	r0, r0, #0x00004000
	mov	r1, #0x0
	str	r1, [r0, #0x08]
	str	r1, [r0, #0x0C]
	mov	r1, #0x3
	str	r1, [r0, #0x00]
	ldr	r1, ucon_val
	str	r1, [r0, #0x04]
	ldr	r1, ubrdiv_val
	str	r1, [r0, #0x28]
/*uart 0 init*/
	mov	r0, #0x44000000
	orr	r0, r0, #0x00400000
	mov	r1, #0x0
	str	r1, [r0, #0x08]
	str	r1, [r0, #0x0C]
	mov	r1, #0x3
	str	r1, [r0, #0x00]
	ldr	r1, ucon_val
	str	r1, [r0, #0x04]
	ldr	r1, ubrdiv_val
	str	r1, [r0, #0x28]

#ifdef CONFIG_DEBUG_LL
	mov	r10, lr
	mov	r0, #'\n'
	bl	printch
	mov	lr, r10
#endif
	mov	pc, lr


@
@ copy uboot to SDRAM and jump to ram
@
copy_myself:
	mov	r10, lr		@ save return address

	@ get ready to call C functions
	ldr	sp, uboot_stack_pointer	@ setup stack pointer
	mov	fp, #0			@ no previous frame, so fp=0

#ifdef CONFIG_DEBUG_LL
	adr	r0, str_copy
	bl	printascii
#endif
	bl	copy_uboot_to_ram

	tst 	r0, #0x0
	bne	copy_myself_failed

#ifdef CONFIG_DEBUG_LL
	adr	r0, str_ok
	bl	printascii
#endif

	@ verify
#ifdef CONFIG_DEBUG_LL
	adr	r0, str_verify
	bl	printascii
#endif

	mov	r0, #0
	ldr	r1, uboot_ram_base_addr
	mov	r2, #0x400	@ compare first 4-K bytes
1001:	ldr	r3, [r0], #4
	ldr	r4, [r1], #4
	teq	r3, r4
	bne	1002f		@ not matched
	subs	r2, r2, #4
	beq	1003f
	b	1001b
	
1002:
#ifdef CONFIG_DEBUG_LL
	sub	r8, r0, #4
	adr	r0, str_fail
	bl	printascii
	mov	r0, r8
	bl	printhex8
#endif
1:	b	1b

1003:
#ifdef CONFIG_DEBUG_LL
	adr	r0, str_ok
	bl	printascii
#endif
	
	@ jump to ram
	ldr	r0, uboot_ram_base_addr
	add	pc, r10, r0	/* fake ^^; */

copy_myself_failed:
#ifdef CONFIG_DEBUG_LL
	adr	r0, str_fail
	bl	printascii
#endif
1:	b	1b	


@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */

	.macro	bad_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	ldr	r2, _armboot_end
	add	r2, r2, #CONFIG_STACKSIZE
	sub	r2, r2, #8
	ldmia	r2, {r2 - r3}			@ get pc, cpsr
	add	r0, sp, #S_FRAME_SIZE		@ restore sp_SVC

	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}			@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp
	.endm

	.macro	irq_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	add     r8, sp, #S_PC
	stmdb   r8, {sp, lr}^                   @ Calling SP, LR
	str     lr, [r8, #0]                    @ Save calling PC
	mrs     r6, spsr
	str     r6, [r8, #4]                    @ Save CPSR
	str     r0, [r8, #8]                    @ Save OLD_R0
	mov	r0, sp
	.endm

	.macro	irq_restore_user_regs
	ldmia	sp, {r0 - lr}^			@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]			@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4			@ return & move spsr_svc into cpsr
	.endm

	.macro get_bad_stack
	ldr	r13, _armboot_end		@ setup our mode stack
	add	r13, r13, #CONFIG_STACKSIZE	@ resides at top of normal stack
	sub	r13, r13, #8

	str	lr, [r13]			@ save caller lr / spsr
	mrs	lr, spsr
	str     lr, [r13, #4]

	mov	r13, #MODE_SVC			@ prepare SVC-Mode
	@ msr	spsr_c, r13
	msr	spsr, r13
	mov	lr, pc
	movs	pc, lr
	.endm

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp, IRQ_STACK_START
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, FIQ_STACK_START
	.endm



 /* Exception handlers
 */
        .align  5
undefined_instruction:
        get_bad_stack
        bad_save_user_regs
        bl      do_undefined_instruction
                                                                                              
        .align  5
software_interrupt:
        get_bad_stack
        bad_save_user_regs
        bl      do_software_interrupt
                                                                                              
        .align  5
prefetch_abort:
        get_bad_stack
        bad_save_user_regs
        bl      do_prefetch_abort
                                                                                              
        .align  5
data_abort:
        get_bad_stack
        bad_save_user_regs
        bl      do_data_abort
                                                                                              
        .align  5
not_used:
        get_bad_stack
        bad_save_user_regs
        bl      do_not_used

        .align  5
irq:
        get_irq_stack
        irq_save_user_regs
        bl      do_irq
        irq_restore_user_regs
                                                                                              
        .align  5
fiq:
        get_fiq_stack
        /* someone ought to write a more effiction fiq_save_user_regs */
        irq_save_user_regs
        bl      do_fiq
        irq_restore_user_regs
                                                                                              
@ Data area for head.S
@

	.ltorg

	.align 2

@ addresses
uboot_stack_pointer:	.long	(STACK_BASE + STACK_SIZE - 4)
wt_ctrl_base_addr:	.long	0x44100000
int_ctrl_base_addr:	.long	0x40200000
HD_LCD_BASE_ADDR:	.long	LCD_BASE_ADDR
HD_SROM_BASE_ADDR:	.long	SROM_BASE_ADDR
HD_SDRAM1_BASE_ADDR:	.long	SDRAM1_BASE_ADDR
REFRESHRATES:		.long	REFRESHRATES_VAL
uboot_ram_base_addr:	.long	UBOOT_BASE

@ values
HD_CLKDIV:		.long	CLKDIV_VAL
HD_MPLLCON:		.long	MPLL_VAL
vLockTime:		.long	0x0fff0fff
HD_SROM_VAL:		.long	SROM_BW_VAL
			.long	SROM_BANK0_VAL
			.long	SROM_BANK1_VAL
			.long	SROM_BANK2_VAL
HD_SDRAM1_VAL:		.long	SDRAM_BANK1_VAL
gpcon_u_val:		.long	GPCON_U_VAL
gpcon_m_val:		.long	GPCON_M_VAL
gpcon_l_val:		.long	GPCON_L_VAL
gpup_val:		.long	GPUP_VAL

			.align 2
str_fail:		.ascii	"fail\n\x00"
			.align 2
str_ok:			.ascii	"ok\n\x00"
			.align 2
str_copy:		.ascii	"copying ... \x00"
			.align 2
str_verify:		.ascii	"verifying ... \x00"
			.align 2
str_boot:		.ascii	"Booting from uboot ... \x00"

#ifdef CONFIG_PM
.align 4
ALIVECON_ADDR:
	.long 0x44800044

GPRAM0_ADDR:
	.long 0x44800080
#endif
