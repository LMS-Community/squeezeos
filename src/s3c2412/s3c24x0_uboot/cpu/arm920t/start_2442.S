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

#include "memory.h"
#include <../board/smdk2442/memsetup.S>

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
	b		sleep_setting

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

@ Processor clock values
#if (CONFIG_SYS_CLK_FREQ ==  12000000)
#define MDIV_126               52
#define PDIV_126               1
#define SDIV_126               2

#define MDIV_240               52
#define PDIV_240               1
#define SDIV_240               1

#define MDIV_250               117
#define PDIV_250               4 
#define SDIV_250               1

#define MDIV_260               57
#define PDIV_260               1
#define SDIV_260               1

#define MDIV_270               82
#define PDIV_270               2
#define SDIV_270               1

#define MDIV_271               173
#define PDIV_271               2
#define SDIV_271               2

#define MDIV_280               62
#define PDIV_280               1
#define SDIV_280               1

#define MDIV_300               67
#define PDIV_300               1
#define SDIV_300               1

#define MDIV_96			56
#define PDIV_96               2
#define SDIV_96               1

#define MDIV_48               56
#define PDIV_48               2
#define SDIV_48               2

#define CLKDIVN_124            0x3  /* UCLK = UPLL */
#define CLKDIVN_136            0x7  /* UCLK = UPLL */
#define CLKDIVN_148            0x5  /* UCLK = UPLL/2 */
#define vMPLLCON_NOW_USER       ((94<< 12) | (2<< 4) | (1))   /*for Sound codec 94:2:1*/
#define vUPLLCON_NOW_USER       ((56<< 12) | (2<< 4) | (1))
#define CLKDIVN_VAL    		0xf	



#else /* SYS_CLK_FREQ == 16.9344Mhz */ 

#define MDIV_296               97
#define PDIV_296               1
#define SDIV_296               2
#define MDIV_96               60
#define PDIV_96               4
#define SDIV_96               1
#define CLKDIVN_136            0xf  /* UCLK = UPLL/2 */
#define CLKDIVN_148            0xd  /* UCLK = UPLL/2 */
#define vMPLLCON_NOW_USER       ((MDIV_296 << 12) | (PDIV_296 << 4) | (SDIV_296))
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
#define CAMDIVN 	0x4C000018
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
	ldr r2, =0x7ff
	ldr r0, =INTSUBMSK
	str r2, [r0]
	
        @ initialize system clocks
	ldr	r0, =LOCKTIME
	ldr     r1, =0xffffff 
	str     r1, [r0]

#ifndef S3C2440_PLL_OFF
        /* FCLK:HCLK:PCLK */
	ldr	r0, =CAMDIVN
	mov	r1, #0
	str	r1, [r0]
        ldr     r0, =CLKDIVN
        ldr     r1, =CLKDIVN_VAL
        str     r1, [r0]
#endif

        mrc     p15, 0, r1, c1, c0, 0           @ read ctrl register
        orr     r1, r1, #0xc0000000             @ Asynchronous
        mcr     p15, 0, r1, c1, c0, 0           @ write ctrl register

#ifndef S3C2440_PLL_OFF
	/* UPLL setup */
        ldr     r0, =UPLLCON
        ldr	r1, upll_value_USER
        str     r1, [r0]
#endif
	nop
	nop
	nop
	nop	
	nop		@ wait until upll has the effect
	nop
	nop	
	nop	

#ifndef S3C2440_PLL_OFF
        /* PLL setup */
	ldr     r0, =MPLLCON
	ldr	r1, mpll_value_USER
	str     r1, [r0]
#endif

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
	ldr	r1, =0x70
	str	r1,[r0]

#ifndef CONFIG_S3C2440A_JTAG_BOOT

#ifdef CONFIG_S3C2440A_NAND_BOOT
	bl      copy_myself
#else
relocate:
	/*
	 * relocate armboot to RAM
	 */
	adr	r0, _start		/* r0 <- current position of code */
	ldr	r2, _armboot_start
	ldr	r3, _armboot_end
	sub	r2, r3, r2		/* r2 <- size of armboot */
	ldr	r1, _TEXT_BASE		/* r1 <- destination address */
	add	r2, r0, r2		/* r2 <- source end address */

	/*
	 * r0 = source address
	 * r1 = target address
	 * r2 = source end address
	 */
copy_loop:
	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r0, r2
	ble	copy_loop
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
#ifndef CONFIG_S3C2440A_JTAG_BOOT
	bl	memsetup
#endif
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
	.align 5
sleep_setting:
@ prepare the SDRAM self-refresh mode
	ldr r0, =0x48000024	@ REFRESH Register
	ldr r1, [r0]
	orr r1, r1,#(1<<22) @ self-refresh bit set

@ prepare MISCCR[19:17]=111b to make SDRAM signals(SCLK0,SCLK1,SCKE) protected
	ldr r2,=0x56000080	@ MISCCR Register
	ldr r3,[r2]
	orr r3,r3,#((1<<17)|(1<<18)|(1<<19))
             
@ prepare the Power_Off mode bit in CLKCON Register
	ldr r4,=0x4c00000c	@ CLKCON Register
	ldr r5,=(1<<3)
	b   set_sdram_refresh

	.align 5
set_sdram_refresh:
	str r1,[r0]             @ SDRAM self-refresh enable

@ wait until SDRAM into self-refresh
	mov r1, #64
1:  subs    r1, r1, #1
	bne 1b

@ set the MISCCR & CLKCON register for power off
	str r3,[r2]
	str r5,[r4]
	nop                 @ waiting for power off
	nop
	nop
	b   .

	.align 5
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
	
	@ Max1718_Set();  @for case 135 i.e 300MHz operation
@	GPBCON = (GPBCON & ~((3 << 20) | (3 << 16) | (3 << 14))) | (1 << 20) | (1 << 16) | (1 << 14);
	ldr r1, GPBCON
	ldr r0, [r1]
	bic r0, r0, #( (3 << 20) | (3 << 16) | (3 << 14) )
	orr r0, r0, #( (1 << 20) | (1 << 16) | (1 << 14) )
	str r0, [r1]
	// GPB7, 8, 10 : Output

@	GPFCON = (GPFCON & ~(0xff << 8)) | (0x55 << 8);	// GPF4~7: Output , shared with LED4~7
	ldr r1, GPFCON_reg
	ldr r0, [r1]
	bic r0, r0, #( (0xff << 8) )
	orr r0, r0, #( (0x55 << 8) )
	str r0, [r1]

@	GPBDAT = (GPBDAT & ~(1 << 7)) | (0 << 7);   //D4
	ldr r1, GPBDAT
	ldr r0, [r1]
	bic r0, r0, #( (1 << 7) )
	orr r0, r0, #( (0 << 7) )
	str r0, [r1]

@	GPFDAT = (GPFDAT & ~(0xf << 4)) | (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4);    //D3~0
	ldr r1, GPFDAT_reg
	ldr r0, [r1]
	bic r0, r0, #( (0xf << 4) )
	orr r0, r0, #( (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) )	@D3~0
	str r0, [r1]


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
	mov     ip, #0
	mcr     p15, 0, ip, c7, c7, 0           @ invalidate cache
	mcr     p15, 0, ip, c8, c7, 0           @ flush TLB (v4)
	mrc     p15, 0, ip, c1, c0, 0           @ get ctrl register
	bic     ip, ip, #0x000f                 @ ............wcam
	bic     ip, ip, #0x2100                 @ ..v....s........
	mcr     p15, 0, ip, c1, c0, 0           @ ctrl register
	mov     pc, r0


#ifndef CONFIG_S3C2440A_JTAG_BOOT

#ifdef CONFIG_S3C2440A_NAND_BOOT
copy_myself:    

	ldr	r0, =GPFDAT
	ldr	r1, =0x10
	str	r1,[r0]

        mov     r10, lr         @ save return address

        @ get ready to call C functions
        ldr     sp, uboot_stack_pointer @ setup stack pointer
        mov     fp, #0                  @ no previous frame, so fp=0

        bl      copy_uboot_to_ram

        tst     r0, #0x0
        bne     copy_myself_failed


	ldr	r0, =GPFDAT
	ldr	r1, =0x20
	str	r1,[r0]

        mov     r0, #0
        ldr     r1, uboot_ram_base_addr
        mov     r2, #0x400      @ compare first 4-K bytes
1001:   ldr     r3, [r0], #4
        ldr     r4, [r1], #4
        teq     r3, r4
        bne     1002f           @ not matched
        subs    r2, r2, #4
        beq     1003f
        b       1001b

1002:
	ldr	r0, =GPFDAT
	ldr	r1, =0x30
	str	r1,[r0]


1:      b       1b

1003:

	ldr	r0, =GPFDAT
	ldr	r1, =0x40
	str	r1,[r0]

        @ jump to ram
        ldr     r0, uboot_ram_base_addr
        add     pc, r10, r0     /* fake ^^;  return here. */

copy_myself_failed:
1:      b       1b

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
		GPBCON:
		.long 0x56000010
		GPBDAT:
		.long 0x56000014
		GPFCON_reg:
		.long 0x56000050
		GPFDAT_reg:
		.long 0x56000054
#endif
							

