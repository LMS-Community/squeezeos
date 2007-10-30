/*
 *  armboot - Startup Code for ARM920 CPU-core
 *
 *  Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 *  Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 *  Copyright (c) 2002	Gary Jennejohn <gj@denx.de>
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


#include <config.h>
#include <version.h>

#define MISCCR                  0x56000080
#define MISCCR_nEN_SCKE         (1<<19)
#define MISCCR_nEN_SCLK1        (1<<18)
#define MISCCR_nEN_SCLK0        (1<<17)

#define S3C2410_NAND_BASE               0x4E000000
# define NFCONF_   0
# define NFCMD_    4
# define NFADDR_   8
# define NFDATA_   C
# define NFSTAT_   0x10
# define NFECC_    0x14

#include "memory.h"
#include <../board/smdk2440/memsetup.S>
#include <../board/smdk2410/memsetup.S>
#undef CONFIG_DEBUG_LL

/*
 *************************************************************************
 *
 * Jump vector table as in table 3.1 in [1]
 *
 *************************************************************************
 */


.globl _start
_start:	b       reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq
	
_undefined_instruction:	.word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

	.balignl 16,0xdeadbeef


/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * relocate armboot to ram
 * setup stack
 * jump to second stage
 *
 *************************************************************************
 */

_TEXT_BASE:
	.word	TEXT_BASE

.globl _armboot_start
_armboot_start:
	.word _start

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

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start
_bss_start:
        .word __bss_start

.globl _bss_end
_bss_end:
        .word _end


/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de

#ifdef CONFIG_SMDK2410
@ Processor clock values
#if (CONFIG_SYS_CLK_FREQ ==  12000000)
#define MDIV_406               195
#define PDIV_406               4
#define SDIV_406               1

#define MDIV_405               127
#define PDIV_405               2
#define SDIV_405               1

#define MDIV_96                56
#define PDIV_96                2
#define SDIV_96                1

#define MDIV_48                56
#define PDIV_48                2
#define SDIV_48                2

#define CLKDIVN_136            0xf  /* UCLK = UPLL/2 */
#define CLKDIVN_148            0x5  /* UCLK = UPLL/2 */

#define vUPLLCON_NOW_USER       ((MDIV_48 << 12) | (PDIV_48 << 4) | (SDIV_48))
#define vMPLLCON_NOW_USER       ((MDIV_406 << 12) | (PDIV_406 << 4) | (SDIV_406))
#define CLKDIVN_VAL		CLKDIVN_148

#else /* SYS_CLK_FREQ == 16.9344Mhz */ 
#define MDIV_533                118
#define PDIV_533                2
#define SDIV_533                1
/* In 533, you must use CLKDIVN_148 instead of CLKDIVN_136 */

#define MDIV_406                184
#define PDIV_406                2
#define SDIV_406                2
#define CLKDIVN_136             0xf /* UCLK = UPLL/2 */
#define vREFRESH_406            992
#define MDIV_96                 60
#define PDIV_96                 4
#define SDIV_96                 1
#define vMPLLCON_NOW_USER       ((MDIV_406 << 12) | (PDIV_406 << 4) | (SDIV_406))
#define vUPLLCON_NOW_USER       ((MDIV_96 << 12) | (PDIV_96 << 4) | (SDIV_96))
#define CLKDIVN_VAL		CLKDIVN_136
#endif

#define pWTCON          0x53000000
#define SRCPND		0x4A000000
#define INTMSK          0x4A000008
#define INTSUBMSK       0x4A00001C
#define LOCKTIME 	0x4C000000
#define MPLLCON		0x4C000004
#define UPLLCON 	0x4C000008
#define CLKCON		0x4C00000C
#define CLKSLOW		0x4C000010
#define CLKDIVN         0x4C000014
//#define CAMDIVN 	0x4C000018
#define GPFCON		0x56000050
#define GPFDAT		0x56000054
#define GPFUP		0x56000058



/*
 * the actual reset code
 */

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0

	ldr     r0, =pWTCON
	mov     r1, #0x0
	str     r1, [r0]

	/*
	 * mask all IRQs by setting all bits in the INTMR - default
	 */
	mov	r1, #0xffffffff
	ldr	r0, =INTMSK
	str	r1, [r0]

	ldr r2, =0x3ff
	ldr r0, =INTSUBMSK
	str r2, [r0]

	mov             r0, #0
                                                                                                                             
        mcr             p15, 0, r0, c8, c7, 0   @ Flush TLBs
        mcr             p15, 0, r0, c7, c5, 0   @ Flush I Cache
        mcr             p15, 0, r0, c7, c6, 0   @ Flush D Cache
        mcr             p15, 0, r0, c7, c10, 4  @ Drain WB

	
        @ initialize system clocks
	ldr	r0, =LOCKTIME
	ldr     r1, =0xffffff 
	str     r1, [r0]

        /* FCLK:HCLK:PCLK */
        ldr     r0, =CLKDIVN
        mov     r1, #0x3
//        mov     r1, #0x5 //for 2440
        str     r1, [r0]

        mrc     p15, 0, r1, c1, c0, 0           @ read ctrl register
        orr     r1, r1, #0xc0000000             @ Asynchronous
        mcr     p15, 0, r1, c1, c0, 0           @ write ctrl register

	/* UPLL setup */

	/*
	 * we do sys-critical inits only at reboot,
	 * not when booting from ram!
	 */
#ifdef CONFIG_INIT_CRITICAL
	bl	cpu_init_crit
#endif

#ifdef CONFIG_PM
	@ Check if this is a wake-up from sleep
	ldr r1, PMST_ADDR
	ldr r0, [r1]
	tst r0, #0x2 @ PMST_SMR
	bne WakeupStart
#endif

	/* Led Configuration */
	ldr	r0, =GPFCON
	ldr	r1, =0x55aa
	str	r1,[r0]

	ldr	r0, =GPFUP
	ldr	r1, =0xff
	str	r1,[r0]

	ldr	r0, =GPFDAT
	ldr	r1, =0x60
	str	r1,[r0]

#ifndef CONFIG_S3C2410_JTAG_BOOT

#ifdef CONFIG_S3C2410_NAND_BOOT
/* release the SDRAM signal protection */
        ldr             r0, =MISCCR
        ldr             r1, [r0]
        bic             r1, r1, #(MISCCR_nEN_SCKE | MISCCR_nEN_SCLK1 | MISCCR_nEN_SCLK0)
        str             r1, [r0]
                                                                                                                             
        ldr             r0, =GPFDAT
        mov             r1, #0x10
        str             r1, [r0]

	bl      copy_myself
#endif
#endif

clear_bss:
        ldr     r0, _bss_start          /* find start of bss segment        */
        add     r0, r0, #4              /* start at first byte of bss       */
        ldr     r1, _bss_end            /* stop here                        */
        mov     r2, #0x00000000         /* clear                            */

clbss_l:str     r2, [r0]                /* clear loop...                    */
        add     r0, r0, #4
        cmp     r0, r1
        bne     clbss_l

	/* set up the stack */
	ldr	r0, _armboot_end
	add	r0, r0, #CONFIG_STACKSIZE
	sub	sp, r0, #12		/* leave 3 words for abort-stack */

	ldr	pc, _start_armboot

_start_armboot:	.word start_armboot



.align 4
mpll_value_USER:
        .long   vMPLLCON_NOW_USER

upll_value_USER:
        .long   vUPLLCON_NOW_USER
#endif


/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************
 */

cpu_init_crit:
	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	mcr	p15, 0, r0, c1, c0, 0


	/*
	 * before relocating, we have to setup RAM timing
	 * because memory timing is board-dependend, you will
	 * find a memsetup.S in your board directory.
	 */
	mov	ip, lr
	bl	memsetup
	mov	lr, ip

	mov	pc, lr

	
memsetup:
        @ initialise the static memory
                                                                                
        @ set memory control registers
        mov     r1, #MEM_CTL_BASE
        adrl    r2, mem_cfg_val
        add     r3, r1, #52
1:      ldr     r4, [r2], #4
        str     r4, [r1], #4
        cmp     r1, r3
        bne     1b
        mov     pc, lr




/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

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

/*
 * exception handlers
 */
	.align  5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl 	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl 	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl 	do_prefetch_abort

	.align	5
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl 	do_data_abort

	.align	5
not_used:
	get_bad_stack
	bad_save_user_regs
	bl 	do_not_used

	.align	5
irq:
	get_irq_stack
	irq_save_user_regs
	bl 	do_irq
	irq_restore_user_regs

	.align	5
fiq:
	get_fiq_stack
	/* someone ought to write a more effiction fiq_save_user_regs */
	irq_save_user_regs
	bl 	do_fiq
	irq_restore_user_regs

#ifdef CONFIG_PM
WakeupStart:	
	@ Clear sleep reset bit
	ldr r0, PMST_ADDR
	mov r1, #(1<<1) @ PMST_SMR
	str r1, [r0]
	
	@ Release the SDRAM signal protections
	ldr r0, PMCTL1_ADDR
	ldr r1, [r0]
	bic r1, r1, #((1<<17)|(1<<18)|(1<<19))  @ (SCLKE | SCLK1 | SCLK0)
	str r1, [r0]
	
	@ Go...
	ldr r0, PMSR0_ADDR  @ read a return address
	ldr r1, [r0]
	mov pc, r1
	nop
	nop
	1:  b   1b      @ infinite loop
#endif

	.align	5
.globl reset_cpu
reset_cpu:
#ifdef CONFIG_S3C2400
	bl	disable_interrupts
# ifdef CONFIG_TRAB
	bl	disable_vfd
# endif
	ldr	r1, _rWTCON
	ldr	r2, _rWTCNT
	/* Disable watchdog */
	mov	r3, #0x0000
	str	r3, [r1]
	/* Initialize watchdog timer count register */
	mov	r3, #0x0001
	str	r3, [r2]
	/* Enable watchdog timer; assert reset at timer timeout */
	mov	r3, #0x0021
	str	r3, [r1]
_loop_forever:
	b	_loop_forever
_rWTCON:
	.word	0x15300000
_rWTCNT:
	.word	0x15300008
#else /* ! CONFIG_S3C2400 */
	mov     ip, #0
	mcr     p15, 0, ip, c7, c7, 0           @ invalidate cache
	mcr     p15, 0, ip, c8, c7, 0           @ flush TLB (v4)
	mrc     p15, 0, ip, c1, c0, 0           @ get ctrl register
	bic     ip, ip, #0x000f                 @ ............wcam
	bic     ip, ip, #0x2100                 @ ..v....s........
	mcr     p15, 0, ip, c1, c0, 0           @ ctrl register
	mov     pc, r0
#endif /* CONFIG_S3C2400 */



#ifndef CONFIG_S3C2410_JTAG_BOOT

#ifdef CONFIG_S3C2410_NAND_BOOT
copy_myself:    
        mov     r10, lr         @ save return address

        /* reset NAND */
        mov     r1, #S3C2410_NAND_BASE
        ldr     r2, =0xf830             /* initial value */
        str     r2, [r1, #NFCONF_]
        ldr     r2, [r1, #NFCONF_]
        bic     r2, r2, #0x800          /* enable chip   */
        str     r2, [r1, #NFCONF_]
        mov     r2, #0xff               /* RESET command */
        strb    r2, [r1, #NFCMD_]
        mov     r3, #0                  /* wait          */
1:      add     r3, r3, #0x1
        cmp     r3, #0xa
        blt     1b
2:      ldr     r2, [r1, #NFSTAT_]       /* wait ready    */
        tst     r2, #0x1
        beq     2b
        ldr     r2, [r1, #NFCONF_]
        orr     r2, r2, #0x800          /* disable chip  */
        str     r2, [r1, #NFCONF_]
                                                                                                                             
        /* get read to call C functions (for nand_cp()) */
        ldr     r0, _armboot_end
        add     r0, r0, #CONFIG_STACKSIZE
        sub     sp, r0, #12             /* leave 3 words for abort-stack */
        mov     fp, #0                  /* no previous frame, so fp=0 */

        /* copy uboot to RAM */
        ldr     r0, =_start
        mov     r1, #0x0
        ldr     r2, _armboot_end
        sub     r2, r2, r0
//        bl      nand_cp
        bl      copy_uboot_to_ram 
                                                                                                                             
        tst     r0, #0x0
        beq     ok_nand_read
3:      b       3b              /* infinite loop */
                                                                                                                             
ok_nand_read:
        /* verify */
        mov     r0, #0
        ldr     r1, =_start
        mov     r2, #0x400      /* 4 bytes * 1024 = 4K-bytes */
go_next:
        ldr     r3, [r0], #4
        ldr     r4, [r1], #4
        teq     r3, r4
        bne     notmatch
        subs    r2, r2, #4
        beq     done_nand_read
        bne     go_next
notmatch:
1:      b       1b              /* infinite loop */

done_nand_read:
        ldr             r0, =GPFDAT
        mov             r1, #0x70
        str             r1, [r0]

        mov     pc, r10
                                                                                                                             
#endif
#endif

	.ltorg

	.align 2

@ addresses
uboot_stack_pointer:    .long   (STACK_BASE + STACK_SIZE - 4)
uboot_ram_base_addr:    .long   UBOOT_BASE
	
	.align 4
mem_cfg_val:
	.long   vBSWCON
        .long   vBANKCON0
        .long   vBANKCON1
        .long   vBANKCON2
        .long   vBANKCON3
        .long   vBANKCON4
        .long   vBANKCON5
        .long   vBANKCON6
        .long   vBANKCON7
        .long   vREFRESH
        .long   vBANKSIZE
        .long   vMRSRB6
        .long   vMRSRB7
#ifdef CONFIG_PM
		.align 4
		PMCTL1_ADDR:
		.long 0x56000080
		PMST_ADDR:
		.long 0x560000B4
		PMSR0_ADDR:
		.long 0x560000B8
#endif
