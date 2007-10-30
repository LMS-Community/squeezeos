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
 * $Date: 2006/01/17 06:08:05 $
 * $Revision: 1.8 $
 */

#include <config.h>
#include "linkage.h"
#include "machine.h"
@#include "memory.h"
#include "armv.h"


#include "regs.h"
#include <asm/arch/s3c2460.h>
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
#define m_temp (M_div+8)
#define p_temp (P_div+2)
#define s_temp (S_div)
#define fout_temp ( (m_temp*12)/(p_temp * (1<<s_temp)) )
#define	fpclk	((fout_temp)/4)  
	@assuming armclk:hclk:pclk = 1:2:4

ubrdiv_val:     .long   ( ((fpclk*1000000) / (CONFIG_SERIAL_DEFAULT_BAUDRATE * 16))-1)

                .macro  adduart,rx
                mov     \rx, #0x44000000
                orr     \rx, \rx, #0x00400000
                .endm

                .macro  senduart,rd,rx
                str     \rd, [\rx, #0x20]
                .endm

                .macro  busyuart,rd,rx
                nop
                .endm

                .macro  waituart,rd,rx
1001:           ldr     \rd, [\rx, #0x10]
                tst     \rd, #UTRSTAT_TX_EMPTY
                beq	1001b
		nop
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
1:     waituart r2, r3
		mov r1, #0x41   @filling 'A'
                senduart r1, r3
                busyuart r2, r3
                teq     r1, #'\n'
                moveq   r1, #'\r'
                beq     1b
2:              mov r1, #0x41
		teq     r0, #0
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
	mov	r0, #(SVC_MODE|F_BIT|I_BIT)
	msr	cpsr_c, r0	@ reset CPSR
@	bl	PreInit
	bl	InitClock
#if defined(CONFIG_SMDK2460_496_DDR) || defined(CONFIG_SMDK2460_416_DDR)
#ifdef CONFIG_SMDK2460_496_DDR)
	bl	InitMemDDR_B1   /*for DDR ram on port1*/
#endif
	bl	InitMemDDR_B2   /*for DDR ram on port2*/

#elif defined(CONFIG_SMDK2460_496_SDRAM) || defined(CONFIG_SMDK2460_416_SDRAM)
#ifdef CONFIG_SMDK2460_496_SDRAM
	bl	InitMemCtl1   /*for SDRAM ram on port1*/
#endif
	bl	InitMemCtl2   /*for SDRAM ram on port2*/
#endif
	bl	init_led
	mov	r2, #0x100
	bl	glow_led

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
	mov	r2, #0x200
	bl	glow_led
	
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

init_led:
	mov	r0,#0x44000000
	orr	r0, r0,#0x800000
	orr	r0, r0,#0x90
	ldr	r1, [r0, #0x0]
	mov	r2,#0xff000000
	orr	r2, r2,#0xff00
	orr	r2, r2,#0xff
	and	r1, r1, r2
	mov	r2,#0x550000
	orr	r1, r1, r2
	str	r1, [r0, #0x0]

	mov	pc, lr

glow_led:
	mov	r0,#0x44000000
	orr	r0, r0,#0x800000
	orr	r0, r0,#0x94
	ldr	r1, [r0, #0x0]
	bic	r1, r1, #0xf00
	orr	r1, r1, r2
	str	r1, [r0, #0x0]
	
	mov	pc, lr

led_init:
	mov	r0,#0x44000000
	orr	r0, r0,#0x800000
	orr	r0, r0,#0x90
	mov r1,#0x55
	orr r1,r1, #0x5500
	str	r1,[r0,#0x00]
	mov pc,lr

	ldr r1, [r0]
	and r1,r1,#0xff0000
	orr	r1, r1,#0x550000
	str	r1, [r0, #0x00]
	mov	pc, lr
													
led_glow:
	mov	r0,#0x44000000
	orr	r0, r0,#0x800000
	orr	r0, r0,#0x94
	mov	r1, #0xff
	orr	r1, r1,#0xff00
	str	r1,[r0, #0x00]
	mov pc,lr

	ldr r1, [r0]
	and r1,r1,#0xff
	orr	r1, r1,#0x180
	str	r1, [r0, #0x00]
	mov	pc, lr
													
@
@ PreInit: Inialize processors
@
PreInit:
;	@ Disable watchdog timer
;	ldr	r0, wt_ctrl_base_addr
;	mov	r1, #0x0
;	str	r1, [r0, #0x0]

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

	ldr	r0, WT_ADDR
	mov	r1, #0
	str	r1, [r0]         /*Disable watchdog*/

	ldr	r0, CLK_ADDR_BASE
@	mov	r1, #0x10
@	str	r1, [r0, #0x20]

	ldr     r1, [r0, #oCLKDIV]   /*Set Clock Divider*/
        bic     r1, r1, #0x10000
        bic     r1, r1, #0x7
        ldr     r2, =((ARMCLK_div<<16)|(PCLK_div<<2)|(HCLK_div))
        orr     r1, r1, r2
        str     r1, [r0, #oCLKDIV]


	ldr	r1, vLockTime0			/*Set MPLL Locktime*/
	str	r1, [r0, #oLOCKTIME0]
@	ldr	r1, vLockTime1
@	str	r1, [r0, #oLOCKTIME1]

	ldr	r1,=( (M_div<<16)|(P_div<<8)|(S_div) )  /*Set MPLL*/
	str	r1, [r0, #oMPLLCON]

	mov	r1, #0x10			/*Select MPLL clock out for SYSCLK*/
	str	r1, [r0, #0x20]

@	mov	r1,    #0x37000000
@	orr	r1, r1,#0x00000000
@	orr	r1, r1,#0x00000000
@	orr	r1, r1,#0x00000000
@	str	r1, [r0, #oHCLKCON]
	
@	mov	r1,    #0x00000000
@	mov	r1,    #0x00150000
@	orr	r1, r1,#0x00000c00
@	orr	r1, r1,#0x000000a1
@	str	r1, [r0, #oPCLKCON]
	
@	mov	r1, 	#0x1f000000
@	orr	r1, r1, #0x00900000
@	orr	r1, r1, #0x00003000
@	orr	r1, r1, #0x00000002
@	str	r1, [r0, #oSCLKCON]

@	ldr	r1, HD_MPLLCON
@	str	r1, [r0, #oMPLLCON]

	mov 	pc, lr
@--------------------------

@
@ InitMemCtl: Initialize memory controller
@
InitMemDDR_B1:
	@Step 1: Set Memory port1 configuration
	ldr	r0, PRIORITY0
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x10
	str	r1, [r0, #0x4]

	ldr	r0, HD_SROM_BASE_ADDR	@ Set SROM controller
	ldr r1, [r0]
	bic r1, r1, #0x1c0
	orr r1, r1, #0x1c0
	str	r1, [r0]

	@Step 2: Setting MEMSelect and CL
	ldr	r0, DDR_CONTROLLER
	ldr	r1, [r0, #0x0]
	bic r1, r1, #((0x3<<2) | (0x1<<0))
	orr	r1, r1, #(0x3<<21)					@Set MEMSelect to mDDR
	orr r1, r1, #((0x3<<2) | (0x1<<0))		@Set CL and Data Bus Width
	str	r1, [r0, #0x0]

	@Step 3: Issue a PALL command
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]

	@Step 4: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 5: Program the DLL Enable bit to low. Issue a EMRScommand
	ldr	r1, [r0, #0x0]
	bic r1, r1, #(0x1<<24)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	orr r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 6: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 7: Program the DLLReset bit to high. Issue a MRS command
	ldr	r1, [r0, #0x0]
	orr r1, r1, #(0x1<<23)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]    
	
	@Step 8: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 9: Issue a PALL command
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]    

	@Step 10: Write '0xf' into the refresh timer register
	mov	r1, #0xf
	str	r1, [r0, #0x8]

	@Step 11: Wait 200 clk(14 refresh cycles)
	mov	r1, #200
1:	subs	r1, r1, #1
	bne	1b

	@Step 12: Program the DLLReset bit to low. Issue a MRS command
	ldr	r1, [r0, #0x0]
	bic r1, r1, #(0x1<<23)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]
	
	@Step 13: Program the normal operational value into the refresh timer
	mov	r1, #0x30c
	str	r1, [r0, #0x8]

	@Step 14: Program INIT to nomal operation '00b'
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	str	r1, [r0, #0x4]
	
	@Step 15: Program the CFG register to their normal operation values.
	ldr	r1, [r0, #0x0]
	ldr	r2, =~(0xfe1fffa2)
	and	r1, r1, r2
	orr	r1, r1, #(0x1<<20)					@Power Down control
	orr	r1, r1, #((0x6<<16) | (0x9<<12))	@Set Tras, Trc value
	orr	r1, r1, #((0x3<<10) | (0x3<<8))		@Set Trcd, Trp value
	orr	r1, r1, #((0xa<<4) | (0x1<<1))		@Set Density, Disable AP
	str	r1, [r0, #0x0]
	
	ldr	r1, [r0, #0x4]
	orr	r1, r1, #((0x1<<6) | (0x1<<2))	@Power save mode, Write buffer enable
	str	r1, [r0, #0x4]        

	mov	pc, lr
@--------------------------

@
@ InitMemCtl: Initialize memory controller
@
InitMemDDR_B2:
	@Step 1: Set Memory port2 configuration
	ldr	r0, PRIORITY0
	ldr	r1, [r0, #0x8]
	bic	r1, r1, #0x10
	str	r1, [r0, #0x8]

	ldr	r0, HD_SROM_BASE_ADDR	@ Set SROM controller
	ldr r1, [r0]
	bic r1, r1, #0x1c0
	orr r1, r1, #0x1c0
	str	r1, [r0]

	@Step 2: Setting MEMSelect and CL
	ldr	r0, DDR_CONTROLLER_2
	ldr	r1, [r0, #0x0]
	bic r1, r1, #((0x3<<2) | (0x1<<0))
	orr	r1, r1, #(0x3<<21)					@Set MEMSelect to mDDR
	orr r1, r1, #((0x3<<2) | (0x1<<0))		@Set CL and Data Bus Width
	str	r1, [r0, #0x0]

	@Step 3: Issue a PALL command
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]

	@Step 4: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 5: Program the DLL Enable bit to low. Issue a EMRScommand
	ldr	r1, [r0, #0x0]
	bic r1, r1, #(0x1<<24)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	orr r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 6: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 7: Program the DLLReset bit to high. Issue a MRS command
	ldr	r1, [r0, #0x0]
	orr r1, r1, #(0x1<<23)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]    
	
	@Step 8: Wait minimum 2 cycle
	nop
	nop
	nop
	nop

	@Step 9: Issue a PALL command
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]    

	@Step 10: Write '0xf' into the refresh timer register
	mov	r1, #0xf
	str	r1, [r0, #0x8]

	@Step 11: Wait 200 clk(14 refresh cycles)
	mov	r1, #200
1:	subs	r1, r1, #1
	bne	1b

	@Step 12: Program the DLLReset bit to low. Issue a MRS command
	ldr	r1, [r0, #0x0]
	bic r1, r1, #(0x1<<23)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]
	
	@Step 13: Program the normal operational value into the refresh timer
	mov	r1, #0x30c
	str	r1, [r0, #0x8]

	@Step 14: Program INIT to nomal operation '00b'
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	str	r1, [r0, #0x4]
	
	@Step 15: Program the CFG register to their normal operation values.
	ldr	r1, [r0, #0x0]
	ldr	r2, =~(0xfe1fffa2)
	and	r1, r1, r2
	orr	r1, r1, #(0x1<<20)					@Power Down control
	orr	r1, r1, #((0x6<<16) | (0x9<<12))	@Set Tras, Trc value
	orr	r1, r1, #((0x3<<10) | (0x3<<8))		@Set Trcd, Trp value
	orr	r1, r1, #((0xa<<4) | (0x1<<1))		@Set Density, Disable AP
	str	r1, [r0, #0x0]
	
	ldr	r1, [r0, #0x4]
	orr	r1, r1, #((0x1<<6) | (0x1<<2))	@Power save mode, Write buffer enable
	str	r1, [r0, #0x4]        

	mov	pc, lr
@--------------------------

@
@ InitMemCtl: Initialize memory controller
@
InitMemCtl1:
	@Step 1: Set Memory port1 configuration
	ldr	r0, PRIORITY0
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x10
	str	r1, [r0, #0x4]

	ldr	r0, HD_SROM_BASE_ADDR	@ Set SROM controller
	ldr r1, [r0]
	bic r1, r1, #0x1c0
	orr r1, r1, #0x1c0
	str	r1, [r0]

	@Step 2: Issue a PALL command
	ldr	r0, HD_SDRAM1_BASE_ADDR
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]

	@Step 3: Write '0xf' into the refresh timer register
	mov	r1, #0xf
	str	r1, [r0, #0x8]                /*refresh */

	@Step 4: Wait 120 clk(8 refresh cycles)
	mov	r1, #120
11:	subs	r1, r1, #1
	bne	11b

	@Step 5: Program the normal operational value into the refresh timer.
	mov	r1, #0x30C
	str	r1, [r0, #0x8]                

	@Step 6: Program the MemSelect bits of configruation register.
	ldr	r1, [r0, #0x0]
	bic r1, r1, #0x3<<21
	bic r1, r1, #0x1
	orr	r1, r1, #0x1<<21	@Set MEMSelect to mSDRAM
	orr r1, r1, #0x1		@Set 16bit Data Bus Width

test_again11:	ldr	r3, [r0, #0x4]
	tst	r3, #0x1000
	beq	safe_to_change11
	b	test_again11
	
safe_to_change11:	str	r1, [r0, #0x0]   @Note: BANKCFG register should not be written when the SDRAM
					@Controller is busy. 12th bit in BANKCON register is used to
					@check if the controller is idle.
	
	@Step 7: Issue a MRS command.
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]

	@Step 8: Issue a EMRS command(Mobile only).
	orr	r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 9: program the INIT[1:0] to '00b'(normal operation).
	bic	r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 10: Program the configuration register to their normal operation values.
	ldr	r1, [r0, #0x0]
	ldr	r2, =~(0xfe1fffae)
	and	r1, r1, r2
	orr	r1, r1, #(0x1<<20)							@Power Down control
	orr	r1, r1, #((0x6<<16) | (0x9<<12))			@Set Tras, Trc value
	orr	r1, r1, #((0x3<<10) | (0x3<<8))				@Set Trcd, Trp value
	orr	r1, r1, #((0xa<<4) | (0x3<<2) | (0x1<<1))	@Set Density, CL, Disable AP

test_again12:	ldr	r3, [r0, #0x4]
	tst	r3, #0x1000
	beq	safe_to_change12
	b	test_again12
	
safe_to_change12:	str	r1, [r0, #0x0]   @Note: BANKCFG register should not be written when the SDRAM
					@Controller is busy. 12th bit in BANKCON register is used to
					@check if the controller is idle.
	
	ldr	r1, [r0, #0x4]
	orr	r1, r1, #((0x1<<6) | (0x1<<2))	@Support Power save mode, Write buffer enable
	str	r1, [r0, #0x4]        

	mov	pc, lr
@--------------------------

@
@ InitMemCtl: Initialize memory controller
@
InitMemCtl2:
	@Step 1: Set Memory port2 configuration
	ldr	r0, PRIORITY0
	ldr	r1, [r0, #0x8]
	bic	r1, r1, #0x10
	str	r1, [r0, #0x8]

	ldr	r0, HD_SROM_BASE_ADDR	@ Set SROM controller
	ldr r1, [r0]
	bic r1, r1, #0x1c0
	orr r1, r1, #0x1c0
	str	r1, [r0]

	@Step 2: Issue a PALL command
	ldr	r0, HD_SDRAM2_BASE_ADDR
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x1
	str	r1, [r0, #0x4]

	@Step 3: Write '0xf' into the refresh timer register
	mov	r1, #0xf
	str	r1, [r0, #0x8]                /*refresh */

	@Step 4: Wait 120 clk(8 refresh cycles)
	mov	r1, #120
11:	subs	r1, r1, #1
	bne	11b

	@Step 5: Program the normal operational value into the refresh timer.
	mov	r1, #0x30C
	str	r1, [r0, #0x8]                

	@Step 6: Program the MemSelect bits of configruation register.
	ldr	r1, [r0, #0x0]
	bic r1, r1, #0x3<<21
	bic r1, r1, #0x1
	orr	r1, r1, #0x1<<21	@Set MEMSelect to mSDRAM
#ifdef CONFIG_SMDK2460_496_SDRAM
	orr	r1, r1, #0x1 		@Set 16bit Data Bus Width
#endif

test_again21:	ldr	r3, [r0, #0x4]
	tst	r3, #0x1000
	beq	safe_to_change21
	b	test_again21
	
safe_to_change21:	str	r1, [r0, #0x0]   @Note: BANKCFG register should not be written when the SDRAM
					@Controller is busy. 12th bit in BANKCON register is used to
					@check if the controller is idle.
	
	@Step 7: Issue a MRS command.
	ldr	r1, [r0, #0x4]
	bic	r1, r1, #0x3
	orr	r1, r1, #0x2
	str	r1, [r0, #0x4]

	@Step 8: Issue a EMRS command(Mobile only).
	orr	r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 9: program the INIT[1:0] to '00b'(normal operation).
	bic	r1, r1, #0x3
	str	r1, [r0, #0x4]

	@Step 10: Program the configuration register to their normal operation values.
	ldr	r1, [r0, #0x0]
	ldr	r2, =~(0xfe1fffae)
	and	r1, r1, r2
	orr	r1, r1, #(0x1<<20)							@Power Down control
	orr	r1, r1, #((0x6<<16) | (0x9<<12))			@Set Tras, Trc value
	orr	r1, r1, #((0x3<<10) | (0x3<<8))				@Set Trcd, Trp value
	orr	r1, r1, #((0xa<<4) | (0x3<<2) | (0x1<<1))	@Set Density, CL, Disable AP

test_again22:	ldr	r3, [r0, #0x4]
	tst	r3, #0x1000
	beq	safe_to_change22
	b	test_again22
	
safe_to_change22:	str	r1, [r0, #0x0]   @Note: BANKCFG register should not be written when the SDRAM
					@Controller is busy. 12th bit in BANKCON register is used to
					@check if the controller is idle.
	
	ldr	r1, [r0, #0x4]
	orr	r1, r1, #((0x1<<6) | (0x1<<2))	@Support Power save mode, Write buffer enable
	str	r1, [r0, #0x4]        

	mov	pc, lr
@--------------------------

#ifdef CONFIG_DEBUG_LL
	@ LED on only #8
	ldr	r1, [r0, #0xc]
	orr	r1, r1, #0x000000f0
	bic	r1, r1, #0x00000080
	str	r1, [r0, #0xc]
#endif
	mov	pc, lr

/*
 * Initialise FPU
 */
InitFPU:
	ldr	r0,=(1<<30)
@	fmxr	fpexc, r0

	mov	r0, #0
@	fmxr	fpscr, r0

	mov pc, lr

/*
 * Initialize UART 0
 */
InitUART:
	mov	r0, #0x44000000
	orr	r0, r0, #0x00800000
	orr	r0, r0, #0x70
	mov	r1, #0xAA
	orr	r1, r1, #0xAA00
	str	r1, [r0, #0x0]    @GPIO

#ifdef CONFIG_SERIAL_UART0
	mov	r0, #0x44000000
	orr	r0, r0, #0x00400000
	mov	r1, #0x0
	str	r1, [r0, #0x8]
	str	r1, [r0, #0xC]

	mov	r1, #0x3                     @was 0.
	str	r1, [r0, #0x0]

	ldr	r1, ucon_val
	str	r1, [r0, #0x4]

	ldr	r1, ubrdiv_val
	str	r1, [r0, #0x28]
#endif /*CONFIG_SERIAL_UART0*/
#ifdef CONFIG_SERIAL_UART1
        mov     r0, #0x44000000
        orr     r0, r0, #0x00400000
        orr     r0, r0, #0x00004000
        mov     r1, #0x0
        str     r1, [r0, #0x8]
        str     r1, [r0, #0xC]

        mov     r1, #0x3
        str     r1, [r0, #0x0]

        ldr     r1, ucon_val
        str     r1, [r0, #0x4]

        ldr     r1, ubrdiv_val
        str     r1, [r0, #0x28]
#endif  /*CONFIG_SERIAL_UART1*/
#ifdef CONFIG_SERIAL_UART2
	mov	r0, #0x44000000
	orr	r0, r0, #0x00800000
	orr	r0, r0, #0x70
	ldr	r1, [r0, #0x0]			@ Get value of gphcon into r1.
	orr	r1, r1, #0x2A0000
	str	r1, [r0, #0x0]    @GPIO

        mov     r0, #0x44000000
        orr     r0, r0, #0x00400000
        orr     r0, r0, #0x00008000
        mov     r1, #0x0
        str     r1, [r0, #0x8]
        str     r1, [r0, #0xC]

        mov     r1, #0x3
        str     r1, [r0, #0x0]

        ldr     r1, ucon_val
        str     r1, [r0, #0x4]

        ldr     r1, ubrdiv_val
        str     r1, [r0, #0x28]

	mov	r0, #0x44000000
	orr	r0, r0, #0x00800000
	orr	r0, r0, #0x0000009c
	mov	r1, #0x0
	str	r1, [r0, #0x0]  @SPCON
#endif /*CONFIG_SERIAL_UART2*/

	mov pc, lr
@-------------


@
@ copy uboot to SDRAM and jump to ram
@
copy_myself:
	mov	r10, lr		@ save return address

	mov	r2, #0x300
	bl	glow_led

	@ get ready to call C functions
	ldr	sp, uboot_stack_pointer	@ setup stack pointer
	mov	fp, #0			@ no previous frame, so fp=0

	bl	copy_uboot_to_ram

	tst 	r0, #0x0
	bne	copy_myself_failed

	@ verify
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
	mov	r2, #0xf00
	bl	glow_led
	b	.

1:	b	1b

1003:
	@ jump to ram
	mov	r2, #0xb00
	bl	glow_led
	ldr	r0, uboot_ram_base_addr
	add	pc, r10, r0	/* fake ^^; */

copy_myself_failed:
1:	mov	r2, #0x500
	bl	glow_led
	b	1b	


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
CHECK_VAL:		.long	0xA5A5A5A5
HD_LCD_BASE_ADDR:	.long	LCD_BASE_ADDR
HD_SROM_BASE_ADDR:	.long	SROM_BASE_ADDR
HD_SDRAM1_BASE_ADDR:	.long	SDRAM1_BASE_ADDR
HD_SDRAM2_BASE_ADDR:	.long	SDRAM2_BASE_ADDR
DDR_CONTROLLER:		.long	0x40C40000 
DDR_CONTROLLER_2:	.long	0x40C80000 
PRIORITY0:	.long	0x40CE0000 
uboot_ram_base_addr:	.long	UBOOT_BASE

@ values

CLK_ADDR_BASE:		.long	0x40000000 
WT_ADDR:		.long	WT_BASE_ADDR 
@HD_CLKDIV:		.long	CLKDIV_VAL
@HD_MPLLCON:		.long	MPLLCON_VAL
HD_CLKSRC:		.long	0x10 
vLockTime0:		.long	0x0fff0fff 
vLockTime1:		.long	0xffff 
Refresh_and_val:	.long	0x9ffffe
@bank_cfg_and_val:	.long	0xfff000ff
bank_cfg_and_val:	.long	0xfe1fffae
bank_cfg_and_val1:	.long	0x01efff51
bank_cfg_and_val1_ddr:	.long	0x01efff5d

			.align 4
str_fail:		.ascii	"fail\n\x00"
			.align 4
str_ok:			.ascii	"ok\n\x00"
			.align 4
str_copy:		.ascii	"copying ... \x00"
			.align 4
str_verify:		.ascii	"verifying ... \x00"
			.align 4
str_boot:		.ascii	"Booting from uboot ... \x00"

#ifdef CONFIG_PM
.align 4
ALIVECON_ADDR:
	.long 0x44800044

GPRAM0_ADDR:
	.long 0x44800080
#endif
