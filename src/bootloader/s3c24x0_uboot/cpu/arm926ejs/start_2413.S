/*
 *  2005 (C) Samsung Electronics 
 *          YONGKAL <yognkal.kim@samsung.com>
 *
 * start_2413.S: Initial codes are gotten from S3C24A0 firmware
 * ported by:	YONGKAL (Samsung Electronics.)
 * date:	November 11, 2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 *
 * $Date: 2006/02/08 08:02:58 $
 * $Revision: 1.6 $
 */

#include <config.h>
#include "linkage.h"
#include "armv.h"
#include "regs_2413.h"
#include "smdk2413.h"
	
	.text

ENTRY(_start)
ENTRY(ResetEntryPoint)		@ at 0x00000000
@ Exception vector table (located at physical address 0x00000000)

@ 0x00: Reset
	b	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_not_used:
	.word not_used
_irq:
	.word irq
_fiq:
	.word fiq

	.balignl 16,0xdeadbeef

/*
 * CFG_MEM_END is in the board dependent config-file (configs/config_BOARD.h)
 */
_TEXT_BASE:
        .word   TEXT_BASE

.globl _armboot_start
_armboot_start:
        .word _start

_start_armboot: 
		.word start_armboot
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

/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
        .word   0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
        .word 0x0badc0de

                .macro  adduart,rx
                mov     \rx, #0x50000000
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

			.align 2
str_fail:		.ascii	"fail\n\x00"
			.align 2
str_ok:			.ascii	"ok\n\x00"
			.align 2
str_copy:		.ascii	"copying ...\n\x00"
			.align 2
str_verify:		.ascii	"verifying ...\n\x00"
			.align 2
str_boot:		.ascii	"Booting from uboot ...\n\x00"

hexbuf:         .space 16

/*
 * the actual reset code
 */
reset:
	@ Set processor into Supervisior mode (SVC) and disable IRQ & FIQ
	mov	r0, #(SVC_MODE|F_BIT|I_BIT)
	msr	cpsr_c, r0	@ reset CPSR
	bl  InitGPIO
	bl	PreInit
	bl	InitClock
 	bl	InitMemCtl
@ 	bl	InitDDRCtl
	bl	InitUART

#ifdef CONFIG_DEBUG_LL
	/* for debugging */
        adr     r0, str_ok
        bl      printascii

	/* for debugging */
        adr     r0, str_boot
        bl      printascii
#endif

#ifndef CONFIG_S3C2413_JTAG_BOOT
	bl	copy_myself
#endif

	@ get ready to call C functions
	ldr	sp, uboot_stack_pointer	@ setup stack pointer
	mov	fp, #0					@ no previous frame, so fp=0
	mov	a2, #0					@ set argv to NULL

clear_bss:
        ldr     r0, _bss_start          /* find start of bss segment        */
        add     r0, r0, #4              /* start at first byte of bss       */
        ldr     r1, _bss_end            /* stop here                        */
        mov     r2, #0x00000000         /* clear                            */

clbss_l:str     r2, [r0]                /* clear loop...                    */
        add     r0, r0, #4
        cmp     r0, r1
        bne     clbss_l

        bl      start_armboot           @ call main

@
@ Initialize GPIO
@
InitGPIO:
    @ LED on only #8
	ldr	r0, =GPIO_BASE_ADDR
    ldr r1, =0x55aa
    str r1, [r0, #oGPFCON]

    ldr r1, =0xff
    str r1, [r0, #oGPFDN]
	
	ldr r1, =0x000010			@ For LED BACKLIT
	str r1, [r0, #oGPBCON]
	ldr r1, =0x4				@ For LED BACKLIT
	str r1, [r0, #oGPBDAT]

	ldr r1, =0x3fffff			@ For NAND Setting
	str r1, [r0, #oGPACON]

	mov	pc, lr
 
@
@ PreInit: Inialize processors
@
PreInit:
	@ Disable watchdog timer
	ldr	r0, =WT_CTRL_BASE_ADDR
	mov	r1, #0x0
	str	r1, [r0, #0x0]

	@ Disable all interrupts
	ldr	r0, =INT_BASE_ADDR
	mvn	r1, #0x0
	str	r1, [r0, #oINTMSK]

	@ Disable all sub-interrupts
	ldr	r1, =0x700
	orr	r0, r0, #0xff
	str	r1, [r0, #oINTSUBMSK]

	@ Set all interrupts as IRQ
	mov	r1, #0x0
	str	r1, [r0, #oINTMOD]

	@ Disable LCD sub interrupts
	ldr	r0, =LCD_BASE_ADDR
	mov	r1, #0x3
	str	r1, [r0, #oLCDINTMSK]		@ 0x2C is off of oLCDINTMSK
	
	mov	pc, lr

@
@ InitClock: Initialize core clock and bus clock.
@
InitClock:
	ldr	r0, =CLK_BASE_ADDR
	ldr	r1, =LockTime
	str	r1, [r0, #oLOCKTIME]
	
	ldr	r1, HD_CLKDIV
	str	r1, [r0, #oCLKDIV]

	ldr	r1, HD_MPLLCON
	str	r1, [r0, #oMPLLCON]

	ldr	r1, HD_UPLLCON
	str	r1, [r0, #oUPLLCON]

	ldr r1, [r0, #oCLKSRC]
	orr	r1, r1, #0x30
	str r1, [r0, #oCLKSRC]

	mov	pc, lr

@
@ InitMemCtl: Initialize memory controller
@
InitMemCtl:
	@EBI Setting
	ldr	r0, =SDRAM_BASE_ADDR
	orr	r0, r0, #0x800000
	mov r1, #0xff
	str r1, [r0, #oBANK_CFG]
	
	ldr	r0, =SDRAM_BASE_ADDR
	adrl	r1,	 SDRAMDATA
	ldmia   r1!, {r2-r5}
    stmia   r0!, {r2-r5}

	@ Step 1: issue precharge all(PALL) command
	ldr	r0, =SDRAM_BASE_ADDR
	ldr	r1, [r0, #0x4]	
	bic r1,r1, #0x3
	mov	r1, #0x1
	str	r1, [r0, #oBANKCON1]

	@ Step 2: make refresh cycle 15clk
	mov	r1, #0xff
	str	r1, [r0, #oREFRESH]

	@ Step 3: wait 120 clk
	mov	r1, #0x100
1:	subs	r1, r1, #1
	bne	1b

	@ Step 4: MRS command
	mov	r1, #0x2
	str	r1, [r0, #oBANKCON1]
	
	@ Step 5: set normal operation refresh cycle
	ldr	r1, =REFRESHRATES_VAL
	str	r1, [r0, #oREFRESH]

	@ Step 6: EMRS command
	mov	r1, #0x3
	str	r1, [r0, #oBANKCON1]
	
	@ Step 7: Normal operation
	mov	r1, #0x0
	str	r1, [r0, #oBANKCON1]

	mov	pc, lr

@
@ InitDDRCtl: Initialize mDDR memory controller
@
InitDDRCtl:
	@EBI Setting
	ldr	r0, =SDRAM_BASE_ADDR
	orr	r0, r0, #0x800000
	mov r1, #0xff
	str r1, [r0, #oBANK_CFG]
	
	ldr	r0, =SDRAM_BASE_ADDR
	adrl	r1,	 DDRAMDATA
	ldmia   r1!, {r2-r5}
    stmia   r0!, {r2-r5}

	@ Step 1: issue precharge all(PALL) command
	ldr	r0, =SDRAM_BASE_ADDR
	ldr	r1, [r0, #oBANKCON1]	
	bic r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #oBANKCON1]

	@ Step 2: make refresh cycle 15clk
	mov	r1, #0xff
	str	r1, [r0, #oREFRESH]

	@ Step 3: wait 120 clk
	mov	r1, #0x100
1:	subs	r1, r1, #1
	bne	1b

	@ Step 4: MRS command
	ldr	r1, [r0, #oBANKCON1]	
	bic r1, r1, #0x3
	orr r1,	r1, #0x2
	str	r1, [r0, #oBANKCON1]
	
	@ Step 5: set normal operation refresh cycle
	ldr	r1, =REFRESHRATES_VAL
	str	r1, [r0, #oREFRESH]

	@ Step 6: EMRS command
	ldr	r1, [r0, #oBANKCON1]	
	bic r1, r1, #0x3
	orr r1,	r1, #0x3
	str	r1, [r0, #oBANKCON1]
	
	@ Step 7: Normal operation
	ldr	r1, [r0, #oBANKCON1]	
	bic r1, r1, #0x3
	orr r1, r1, #0x0
	str	r1, [r0, #oBANKCON1]

	mov	pc, lr

@
@ Initialize UART 0, 1
@
InitUART:
	@GPIO setting for UART
	ldr	r0, =GPIO_BASE_ADDR
	ldr r1, =0xaaaa
	str r1, [r0, #oGPHCON]
/*uart 1 init*/
	ldr	r0, =UART_BASE_ADDR
	orr	r0, r0, #0x00004000
	mov	r1, #0x0
	str	r1, [r0, #oUFCON]			@FIFO ctrl REG.
	str	r1, [r0, #oUMCON]			@Modem ctrl REG.
	mov	r1, #0x3
	str	r1, [r0, #oULCON]			@Line ctrl REG.
	ldr	r1, UCON_VAL
	str	r1, [r0, #oUCON]			@Control REG.
	ldr	r1, UBRDIV_VAL
	str	r1, [r0, #oUBRDIV]			@Baud Rate divisior REG.
/*uart 0 init*/
	ldr	r0, =UART_BASE_ADDR
	mov	r1, #0x0
	str	r1, [r0, #oUFCON]
	str	r1, [r0, #oUMCON]
	mov	r1, #0x3
	str	r1, [r0, #oULCON]
	ldr	r1, UCON_VAL
	str	r1, [r0, #oUCON]
	ldr	r1, UBRDIV_VAL
	str	r1, [r0, #oUBRDIV]

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

	tst r0, #0x0
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
	mov	r2, #0x1000	@ compare first 4-K bytes
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

	mov	r13, #SVC_MODE			@ prepare SVC-Mode
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
                                                                                              
	.ltorg

	.align 2
@ addresses
uboot_stack_pointer:	.long	(STACK_BASE + STACK_SIZE - 4)
uboot_ram_base_addr:	.long	UBOOT_BASE

SDRAMDATA:				.long	SDRAM_BANK_CFG
						.long	SDRAM_BANK_VAL1
						.long	SDRAM_BANK_VAL2
						.long	SDRAM_BANK_VAL3

DDRAMDATA:				.long	DDRAM_BANK_CFG
						.long	DDRAM_BANK_VAL1
						.long	DDRAM_BANK_VAL2
						.long	DDRAM_BANK_VAL3

@ values
UCON_VAL:       		.long   0x245	@TxINTtype=Level, 
										@RxErrStatusINTenble=GenerateReceiveErrorStatusINT
										@TxMode,RxMode=INT req. or PollingMode
UBRDIV_VAL:     		.long   ((UART_REF_CLK / (CONFIG_SERIAL_DEFAULT_BAUDRATE * 16)) - 1)
HD_CLKDIV:				.long	CLKDIV_VAL
HD_MPLLCON:				.long	MPLL_VAL
HD_UPLLCON:				.long	UPLL_VAL
