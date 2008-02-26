/*
 * (C) Copyleft 2004 Samsung Electronics 
 *         SW.LEE <hitchcar@samsung.com>
 *	   - S3C24A0A, S3C2440A Interrupt
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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

#include <common.h>

#if defined(CONFIG_S3C2460x)
#include <asm/arch/s3c2460.h>
#elif defined (CONFIG_S3C2413)
#include <asm/arch/s3c2413.h>
#else
#include <asm/arch/s3c24a0.h>
#endif

#include <asm/arch/irqs.h>
#include <asm/proc-armv/ptrace.h>


extern void reset_cpu(ulong addr);
int timer_load_val = 0;


inline void clear_pending(int irq)
{
        SRCPND = (1 << irq);
#if defined(CONFIG_S3C2460x)
	SUBSRCPND = SUBSRCPND;
#endif
        INTPND = INTPND;
        INTPND;
}



/* macro to read the 16 bit timer */
static inline ulong READ_TIMER(void)
{
 /* wjluv changes this for SMDK24A0 */
	return(TCNTO4 & 0xffff);

}

#ifdef CONFIG_USE_IRQ
/* enable IRQ interrupts */
void enable_interrupts (void)
{
	unsigned long temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "bic %0, %0, #0x80\n"
			     "msr cpsr_c, %0"
			     : "=r" (temp)
			     :
			     : "memory");
}


/*
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int disable_interrupts (void)
{
	unsigned long old,temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");
	return (old & 0x80) == 0;
}
#else
void enable_interrupts (void)
{
	return;
}
int disable_interrupts (void)
{
	return 0;
}
#endif


void bad_mode (void)
{
	panic ("Resetting CPU ...\n");
}

void show_regs (struct pt_regs *regs)
{
	unsigned long flags;
	const char *processor_modes[] = {
	"USER_26",	"FIQ_26",	"IRQ_26",	"SVC_26",
	"UK4_26",	"UK5_26",	"UK6_26",	"UK7_26",
	"UK8_26",	"UK9_26",	"UK10_26",	"UK11_26",
	"UK12_26",	"UK13_26",	"UK14_26",	"UK15_26",
	"USER_32",	"FIQ_32",	"IRQ_32",	"SVC_32",
	"UK4_32",	"UK5_32",	"UK6_32",	"ABT_32",
	"UK8_32",	"UK9_32",	"UK10_32",	"UND_32",
	"UK12_32",	"UK13_32",	"UK14_32",	"SYS_32",
	};

	flags = condition_codes (regs);

	printf ("pc : [<%08lx>]    lr : [<%08lx>]\n"
		"sp : %08lx  ip : %08lx  fp : %08lx\n",
		instruction_pointer (regs),
		regs->ARM_lr, regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	printf ("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9, regs->ARM_r8);
	printf ("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6, regs->ARM_r5, regs->ARM_r4);
	printf ("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2, regs->ARM_r1, regs->ARM_r0);
	printf ("Flags: %c%c%c%c",
		flags & CC_N_BIT ? 'N' : 'n',
		flags & CC_Z_BIT ? 'Z' : 'z',
		flags & CC_C_BIT ? 'C' : 'c', flags & CC_V_BIT ? 'V' : 'v');
	printf ("  IRQs %s  FIQs %s  Mode %s%s\n",
		interrupts_enabled (regs) ? "on" : "off",
		fast_interrupts_enabled (regs) ? "on" : "off",
		processor_modes[processor_mode (regs)],
		thumb_mode (regs) ? " (T)" : "");
}

void do_undefined_instruction (struct pt_regs *pt_regs)
{
	printf ("undefined instruction\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_software_interrupt (struct pt_regs *pt_regs)
{
	printf ("software interrupt\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_prefetch_abort (struct pt_regs *pt_regs)
{
	printf ("prefetch abort\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_data_abort (struct pt_regs *pt_regs)
{
	printf ("data abort\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_not_used (struct pt_regs *pt_regs)
{
	printf ("not used\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_fiq (struct pt_regs *pt_regs)
{
	printf ("fast interrupt request\n");
	show_regs (pt_regs);
	bad_mode ();
}

static inline void unmask(int irq)
{	
	INTMSK &=  ~(1 << irq);
	clear_pending(irq);

}
static inline void mask_ack(int irq)
{	
	INTMSK |= (1 << irq);
	clear_pending(irq);

}

#if defined(CONFIG_S3C24A0A) || defined(CONFIG_S3C2460x) 
void do_irq (struct pt_regs *pt_regs)
{
	int irq;

	//	show_regs (pt_regs);
	irq = INTOFFSET;
	mask_ack(irq);
	switch (irq) {
		case IRQ_USBD:
			udc_int_hndlr();
			break;
		default:
			printf("Unexpect IRQ %d \n",irq);
			break;	
	}
	unmask(irq);
}

#else
void do_irq (struct pt_regs *pt_regs)
{
	int irq;

	irq = INTOFFSET;
	mask_ack(irq);
	switch (irq) 
	{
		case	IRQ_EINT0_2:		/* External interrupt 0 ~ 2 */
			printf("IRQ_EINT0_2 IRQ %d \n",irq);
			break;	
		case	IRQ_EINT3_6:		/* External interrupt 3 ~ 6 */
			printf("IRQ_EINT3_6 IRQ %d \n",irq);
			break;
		case	IRQ_EINT7_10:	/* External interrupt 7 ~ 10 */
			printf("IRQ_EINT7_10 IRQ %d \n",irq);
			break;
		case	IRQ_EINT11_14:	/* External interrupt 11 ~ 14 */
			printf("IRQ_EINT11_14 IRQ %d \n",irq);
			break;
		case	IRQ_EINT15_18:	/* External interrupt 15 ~ 18 */
			printf("IRQ_EINT15_18 IRQ %d \n",irq);
			break;
		case	IRQ_TIC:			/* RTC time tick */
			printf("IRQ_TIC IRQ %d \n",irq);
			break;
		case	IRQ_DCTQ:		/* DCTQ */
			printf("IRQ_DCTQ IRQ %d \n",irq);
			break;
		case	IRQ_MC:			/* MC */
			printf("IRQ_MC IRQ %d \n",irq);
			break;	
		case	IRQ_KEYPAD:		/* Keypad */
			printf("IRQ_KEYPAD IRQ %d \n",irq);
			break;
		case IRQ_TIMER0:			/* Timer 0 */
			printf("IRQ_TIMER0 IRQ %d \n",irq);
			break;
		case IRQ_TIMER1:			/* Timer 1 */
			printf("IRQ_TIMER1 IRQ %d \n",irq);
			break;
		case IRQ_TIMER2:			/* Timer 2 */  
			printf("IRQ_TIMER2 IRQ %d \n",irq);
			break;
		case IRQ_TIMER3_4:		/* Timer 3, 4 */
			printf("IRQ_TIMER3 IRQ %d \n",irq);
			break;
		case	IRQ_LCD_POST:	/* LCD/POST */
			printf("IRQ_LCD_POST IRQ %d \n",irq);
			break;
		case	IRQ_CAM_C:		/* Camera Codec */
			printf("IRQ_CAM_C IRQ %d \n",irq);
			break;
		case	IRQ_WDT_BATFLT:	/* WDT/BATFLT */
			printf("IRQ_WDT_BATFLT IRQ %d \n",irq);
			break;
		case	IRQ_UART0:		/* UART 0 */
			printf("IRQ_UART0 IRQ %d \n",irq);
			break;
		case	IRQ_CAM_P:		/* Camera Preview */
			printf("IRQ_CAM_P IRQ %d \n",irq);
			break;
		case	IRQ_MODEM:		/* Modem */
			printf("IRQ_MODEM IRQ %d \n",irq);
			break;
		case	IRQ_DMA:		/* DMA channels for S-bus */
			printf("IRQ_DMA IRQ %d \n",irq);
			break;
		case	IRQ_SDI:		/* SDI MMC */
			printf("IRQ_SDI IRQ %d \n",irq);
			break;
		case	IRQ_SPI0:		/* SPI 0 */
			printf("IRQ_SPI0 IRQ %d \n",irq);
			break;
		case	IRQ_UART1:		/* UART 1 */
			printf("IRQ_UART1 IRQ %d \n",irq);
			break;
		case	IRQ_AC97_NFLASH:		/* AC97/NFALASH */
			printf("IRQ_AC97_NFLASH IRQ %d \n",irq);
			break;
		case	IRQ_USBH:		/* USB host */
			printf("IRQ_USBH IRQ %d \n",irq);
			break;
		case	IRQ_IIC:			/* IIC */
			printf("IRQ_IIC IRQ %d \n",irq);
			break;
		case	IRQ_IRDA_MSTICK:		/* IrDA/MSTICK */
			printf("IRQ_IRDA_MSTICK IRQ %d \n",irq);
			break;
		case	IRQ_VLX_SPI1:		/* SPI 1 */
			printf("IRQ_VLX_SPI1 IRQ %d \n",irq);
			break;
		case	IRQ_RTC:			/* RTC alaram */
			printf("IRQ_RTC IRQ %d \n",irq);
			break;	
		case	IRQ_ADC_PENUPDN:	/* ADC EOC/Pen up/Pen down */
			printf("IRQ_ADC_PENUPDN IRQ %d \n",irq);
			break;
		case	IRQ_USBD:			/* USB device */
			//printf("IRQ_USBD IRQ %d \n",irq);
			udc_int_hndlr();
			break;
			
		default:
			printf("other IRQ %d \n",irq);
			break;	
	}
	unmask(irq);
/*
	printf ("interrupt request\n");
	show_regs (pt_regs);
	bad_mode ();
*/
}
#endif
#if defined(CONFIG_S3C2460x)
void Isr_Init(void)
{
	rEINTCON0       = 0x22222222;                   //EINT2~0
        rEINTCON1       = 0x22222222;           //EINT10~3

}
#endif

static ulong timestamp;
static ulong lastdec;

static int s3c24x0_init_irq(void)
{
	/* disable all interrupts */
	INTSUBMSK = 0xffffffff;
	EINTMASK = 0xffffffff;
	INTMSK = 0xffffffff;

	/* clear status registers */
	EINTPEND = EINTPEND;
	SUBSRCPND = SUBSRCPND;
	SRCPND = SRCPND;
	INTPND = INTPND;

	/* all interrupts set as IRQ */
	INTMOD = 0x00000000;
#if defined(CONFIG_S3C2460x)
	Isr_Init();
#endif
	return 0;
}

/* Only for using NAND booting */
static void install_vector(void)
{
	int i = 0;
	unsigned long *dest = (unsigned long *)TEXT_BASE;
	unsigned long *src = 0;	/* Stepping Stone Start Address */

	for ( i = 0; i < SZ_1K ; i ++) {
		*src++ = *dest++;
	}
}

int interrupt_init (void)
{
#if 0
	S3C24X0_TIMERS * const timers = S3C24X0_GetBase_TIMERS();

	/* use PWM Timer 4 because it has no output */
	/* prescaler for Timer 4 is 16 */
	timers->TCFG0 = 0x0f00;
	if (timer_load_val == 0)
	{
		/*
		 * for 10 ms clock period @ PCLK with 4 bit divider = 1/2
		 * (default) and prescaler = 16. Should be 10390
		 * @33.25MHz and 15625 @ 50 MHz
		 */
		timer_load_val = get_PCLK()/(2 * 16 * 100);
	}
	/* load value for 10 ms timeout */
	lastdec = timers->TCNTB4 = timer_load_val;
	/* auto load, manual update of Timer 4 */
	timers->TCON = (timers->TCON & ~0x0700000) | 0x600000;
	/* auto load, start Timer 4 */
	timers->TCON = (timers->TCON & ~0x0700000) | 0x500000;
	timestamp = 0;
#else

	TCFG0 = 0x0f00;
	s3c24x0_init_irq();
#if (CONFIG_COMMANDS & CFG_CMD_USBD)
	enable_interrupts();
#endif

#if defined (CONFIG_S3C24A0A_JTAG_BOOT) || defined (CONFIG_S3C2413_JTAG_BOOT)
	install_vector();
#endif

        if (timer_load_val == 0)
        {
                /*
                 * for 10 ms clock period @ PCLK with 4 bit divider = 1/2
                 * (default) and prescaler = 16. Should be 10390
                 * @33.25MHz and 15625 @ 50 MHz
                 */
                timer_load_val = get_PCLK()/(2 * 16 * 100);
        }

        /* load value for 10 ms timeout */
        lastdec = TCNTB4 = timer_load_val;
        /* auto load, manual update of Timer 4 */
//      TCON = (TCON & ~0x0700000) | 0x600000;
        TCON = ((TCON & ~0x0700000) | TCON_4_AUTO | TCON_4_UPDATE);
        /* auto load, start Timer 4 */
//      TCON = (TCON & ~0x0700000) | 0x500000;
        TCON = ((TCON & ~0x0700000) | TCON_4_AUTO | COUNT_4_ON);
        timestamp = 0;


#endif

	return (0);
}

/*
 * timer without interrupts
 */

void reset_timer (void)
{
	reset_timer_masked ();
}

ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}

void set_timer (ulong t)
{
	timestamp = t;
}

void udelay (unsigned long usec)
{
	ulong tmo;

	tmo = usec / 1000;
	tmo *= (timer_load_val * 100);
	tmo /= 1000;

	tmo += get_timer (0);

	while (get_timer_masked () < tmo)
		/*NOP*/;
}

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER();
	timestamp = 0;
}

ulong get_timer_masked (void)
{
	ulong now = READ_TIMER();

	if (lastdec >= now) {
		/* normal mode */
		timestamp += lastdec - now;
	} else {
		/* we have an overflow ... */
		timestamp += lastdec + timer_load_val - now;
	}
	lastdec = now;

	return timestamp;
}

void udelay_masked (unsigned long usec)
{
	ulong tmo;

	tmo = usec / 1000;
	tmo *= (timer_load_val * 100);
	tmo /= 1000;

	reset_timer_masked ();

	while (get_timer_masked () < tmo)
		/*NOP*/;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk (void)
{
	ulong tbclk;

#if defined(CONFIG_SMDK2400) || defined(CONFIG_TRAB)
	tbclk = timer_load_val * 100;
#elif defined(CONFIG_S3C24A0A) || defined(CONFIG_VCMA9)
	tbclk = CFG_HZ;
#elif defined(CONFIG_S3C2460x) || defined(CONFIG_S3C2413) 
	tbclk = CFG_HZ;
#else
#	error "tbclk not configured"
#endif

	return tbclk;
}
