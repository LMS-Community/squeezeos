/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
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
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <version.h>
#include <net.h>

#if defined (CONFIG_S3C2440A)
#include <s3c2440.h>
#elif defined (CONFIG_S3C2460x)
#include <asm/arch/s3c2460.h>
#elif defined (CONFIG_S3C2413)
#include <asm/arch/s3c2413.h>
#include <s3c2413.h>
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
void nand_init (void);
#endif
#include <../drivers/cs8900.h>

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

#ifndef CONFIG_IDENT_STRING
#define CONFIG_IDENT_STRING ""
#endif

const char version_string[] =
	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")"CONFIG_IDENT_STRING;

#ifdef CONFIG_DRIVER_CS8900
extern void cs8900_get_enetaddr (uchar * addr);
#endif

#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/lan91c96.h"
#endif
/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start = 0;
static ulong mem_malloc_end = 0;
static ulong mem_malloc_brk = 0;

static
void mem_malloc_init (ulong dest_addr)
{
	mem_malloc_start = dest_addr;
	mem_malloc_end = dest_addr + CFG_MALLOC_LEN;

	mem_malloc_brk = mem_malloc_start;

	memset ((void *) mem_malloc_start, 0,
			mem_malloc_end - mem_malloc_start);
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;

	return ((void *) old);
}

/************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

static int init_baudrate (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	uchar tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));
	
	gd->bd->bi_baudrate = gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

static int display_banner (void)
{
	int m,p,s,fout,fpclk,fhclk,farmclk;
	int *reg_val;
#ifdef CONFIG_SILENT_CONSOLE
	DECLARE_GLOBAL_DATA_PTR;

	if (gd->flags & GD_FLG_SILENT)
		return (0);
#endif

	printf ("\n\n%s\n\n", version_string);
	printf ("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
		_armboot_start, _armboot_end_data, _armboot_end);
#ifdef CONFIG_S3C2460x
	printf("*****-------------------------------------------------------------*****\n");
	printf("* MDIV: %d, PDIV: %d, SDIV: %d.\n", M_div , P_div, S_div);
	m = (M_div) + 8;
	p = (P_div)+2;
	s = (S_div);
	fout = (m*12)/(p * (1<<s));  //12 is Fin (MHz)
	//farmclk = (CLKDIV_VAL & (1<<16)) ? (fout/2) : fout;
	farmclk = (ARMCLK_div) ? (fout/2) : fout;
	//switch ( CLKDIV_VAL & 0x3 )
	switch ( HCLK_div )
	{
		case 0:
			fhclk = farmclk;
			break;
		case 1:
			fhclk = farmclk/2;
			break;
		case 2:
			fhclk = farmclk/3;
			break;
		case 3:
			fhclk = farmclk/4;
			break;
		default:
			break;
	}
	//fpclk = ( CLKDIV_VAL & (1<<2)) ? (fhclk/2) : fhclk; 
	fpclk = (PCLK_div) ? (fhclk/2) : fhclk; 
	printf("* Fout: %dMHz, ARMCLK: %dMHz, HCLK: %dMHz, PCLK: %dMHz.\n",fout,farmclk,fhclk,fpclk);
/*	reg_val = 0x40000030;
	printf("* HCLKCON is 0x%08x\n",*reg_val);
	reg_val = 0x40000034;
	printf("* PCLKCON is 0x%08x\n",*reg_val);
	reg_val = 0x40000038;
	printf("* SCLKCON is 0x%08x\n",*reg_val); */
	printf("*****-------------------------------------------------------------*****\n");
#endif /*CONFIG_S3C2460x*/
#ifdef CONFIG_MODEM_SUPPORT
	puts ("Modem Support enabled\n");
#endif
	
	printf ("IRQ Stack: %08lx\n", IRQ_STACK_START);
	printf ("FIQ Stack: %08lx\n", FIQ_STACK_START);
	
	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
static int display_dram_config (void)
{
	DECLARE_GLOBAL_DATA_PTR;
	int i;

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT)
		return (0);
#endif

	puts ("DRAM Configuration:\n");

	for(i=0; i<CONFIG_NR_DRAM_BANKS; i++) {
		printf ("Bank #%d: %08lx ", i, gd->bd->bi_dram[i].start);
		print_size (gd->bd->bi_dram[i].size, "\n");
	}

	return (0);
}

static void display_flash_config (ulong size)
{
#ifdef CONFIG_SILENT_CONSOLE
	DECLARE_GLOBAL_DATA_PTR;

	if (gd->flags & GD_FLG_SILENT)
		return;
#endif
	puts ("Flash: ");
	print_size (size, "\n");
}


/*
 * Breath some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
	cpu_init,		/* basic cpu dependent setup */
	board_init,		/* basic board dependent setup */
	interrupt_init,		/* set up exceptions */
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
	dram_init,		/* configure available RAM banks */
	display_dram_config,
#if defined(CONFIG_VCMA9)
	checkboard,
#endif
	NULL,
};

void start_armboot (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	ulong size;
	gd_t gd_data;
	bd_t bd_data;
	init_fnc_t **init_fnc_ptr;
	char *s;
	char i;
	unsigned int matrix[3];
	
#if defined(CONFIG_VFD)
	unsigned long addr;
#endif

	/* Pointer is writable since we allocated a register for it */
	gd = &gd_data;
	memset ((void *)gd, 0, sizeof (gd_t));
	gd->bd = &bd_data;
	memset (gd->bd, 0, sizeof (bd_t));

	monitor_flash_len = _armboot_end_data - _armboot_start;
	
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

#if 0
	/* configure available FLASH banks */
	size = flash_init ();
	display_flash_config (size);
#endif

#ifdef CONFIG_VFD
#  ifndef PAGE_SIZE
#   define PAGE_SIZE 4096
#  endif
	/*
	 * reserve memory for VFD display (always full pages)
	 */
	/* armboot_real_end is defined in the board-specific linker script */
	addr = (_armboot_real_end + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	size = vfd_setmem (addr);
	gd->fb_base = addr;
	/* round to the next page boundary */
	addr += size;
	addr = (addr + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	mem_malloc_init (addr);
#else
	/* armboot_real_end is defined in the board-specific linker script */
	mem_malloc_init (_armboot_real_end);
#endif /* CONFIG_VFD */

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
	puts ("NAND:");
	nand_init();		/* go init the NAND */
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

	/* initialize environment */
	env_relocate ();

#ifdef CONFIG_VFD
	/* must do this after the framebuffer is allocated */
	drv_vfd_init();
#endif

	/* enable keypad backlights with PWM timer */
#define MAX_BRIGHTNESS 0xFFFF
	CLKCON |= (1 << 17); // Enable PWM timer block

	GPBCON &= ~(0x3 << 4);
	GPBCON |= (0x2 << 4); // GPB2 = TOUT2

	TCFG0 &= ~(255 << 8);
	TCFG0 |= ((6 - 1) / 2) << 8; // Same prescaler that is used by linux

	TCFG1 &= ~(0xF << 8);
	TCFG1 |= (0x0 << 8); // PCLK divide 1/2

	TCON |= (1 << 13); // Manual update

	TCNTB2 = MAX_BRIGHTNESS;
	TCMPB2 = (MAX_BRIGHTNESS / 4); // quarter brightness

	TCON |= (1 << 15); // Auto reload
	TCON |= (1 << 14); // Output inverted
	TCON |= (1 << 12); // Start timer

	TCON &= ~(1 << 13); // Turn off manual update


	/* IP Address */
	bd_data.bi_ip_addr = getenv_IPaddr ("ipaddr");

	/* MAC Address */
	{
		int i;
		ulong reg;
		char *s, *e;
		uchar tmp[64];

		i = getenv_r ("ethaddr", tmp, sizeof (tmp));
		s = (i > 0) ? tmp : NULL;

		for (reg = 0; reg < 6; ++reg) {
			bd_data.bi_enetaddr[reg] = s ? simple_strtoul (s, &e, 16) : 0;
			if (s)
				s = (*e) ? e + 1 : e;
		}
	}
	
	devices_init ();	/* get the devices list going. */
	
	jumptable_init ();

	console_init_r ();	/* fully init console as a device */

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r ();
#endif

	/* enable exceptions */
	enable_interrupts ();

#ifdef CONFIG_DRIVER_CS8900
	cs8900_get_enetaddr (gd->bd->bi_enetaddr);
#endif

#ifdef CONFIG_DRIVER_LAN91C96
	if (getenv ("ethaddr")) {
		smc_set_mac_addr(gd->bd->bi_enetaddr);
	}
	/* eth_hw_init(); */
#endif /* CONFIG_DRIVER_LAN91C96 */

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	if ((s = getenv ("bootfile")) != NULL) {
		copy_filename (BootFile, s, sizeof (BootFile));
	}
#endif	/* CFG_CMD_NET */

#ifdef BOARD_POST_INIT
	board_post_init ();
#endif

	/* nand nWP Disable */
	rGPCCON &=0xFFFFCFFF;	//GPC6
	rGPCCON |=0x00001000;	//GPC6 Output
	rGPCDAT |=(1<<6);	//GPC6 High
	rGPCDN &=~(1<<6);	//GPC6 Pull Down disable

#ifdef DETECT_KEY
	GPFDAT &= 0xF8;
	GPFDAT |= 0x06;
	matrix[0] = GPFDAT & 0xF8;

	GPFDAT &= 0xF8;
	GPFDAT |= 0x05;
	matrix[1] = GPFDAT & 0xF8;

	GPFDAT &= 0xF8;
	GPFDAT |= 0x03;
	matrix[2] = GPFDAT & 0xF8;

	//printf("matrix[0] %x\n", matrix[0]);
	//printf("matrix[1] %x\n", matrix[1]);
	//printf("matrix[2] %x\n", matrix[2]);

	if (matrix[1] == 0x20) {
		run_command("run sw6", 0);
	}
	if (matrix[1] == 0x10) {
		run_command("run sw5", 0);
	}
	if (matrix[0] == 0x20) {
		printf("sw2\n");
		run_command("run sw2", 0);
	}
	if (matrix[0] == 0x08) {
		run_command("run sw1", 0);
	}
	if (matrix[0] == 0x40) {
		run_command("run sw3", 0);
	}
	if (matrix[1] == 0x40) {
		run_command("run sw7", 0);
	}
	if (matrix[2] == 0x20) {
		run_command("run sw9", 0);
	}
	if (matrix[2] == 0x40) {
		run_command("run sw10", 0);
	}
	if (matrix[2] == 0x10) {
		run_command("run sw8", 0);
	}
#endif
	
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}


int do_blink (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned long delay;
	unsigned int val;
	
	if (argc < 2) {
		delay = 500;
	}
	else {
		delay = simple_strtoul(argv[1], NULL, 10);
	}

	val = TCMPB2;
	TCMPB2 = (MAX_BRIGHTNESS / 8); // dim

	while (delay && !ctrlc()) {
		udelay (1000);
		--delay;
	}

	TCMPB2 = val; // restore brightness
}


U_BOOT_CMD(
       blink,  CFG_MAXARGS,    1,      do_blink,
       "blink   - flash keyboard leds\n",
       "[delay]\n    - blink delay\n"
);



#ifdef CONFIG_MODEM_SUPPORT
/* called from main loop (common/main.c) */
extern void  dbg(const char *fmt, ...);
int mdm_init (void)
{
	char env_str[16];
	char *init_str;
	int i;
	extern char console_buffer[];
	static inline void mdm_readline(char *buf, int bufsiz);
	extern void enable_putc(void);
	extern int hwflow_onoff(int);

	enable_putc(); /* enable serial_putc() */

#ifdef CONFIG_HWFLOW
	init_str = getenv("mdm_flow_control");
	if (init_str && (strcmp(init_str, "rts/cts") == 0))
		hwflow_onoff (1);
	else
		hwflow_onoff(-1);
#endif

	for (i = 1;;i++) {
		sprintf(env_str, "mdm_init%d", i);
		if ((init_str = getenv(env_str)) != NULL) {
			serial_puts(init_str);
			serial_puts("\n");
			for(;;) {
				mdm_readline(console_buffer, CFG_CBSIZE);
				dbg("ini%d: [%s]", i, console_buffer);

				if ((strcmp(console_buffer, "OK") == 0) ||
					(strcmp(console_buffer, "ERROR") == 0)) {
					dbg("ini%d: cmd done", i);
					break;
				} else /* in case we are originating call ... */
					if (strncmp(console_buffer, "CONNECT", 7) == 0) {
						dbg("ini%d: connect", i);
						return 0;
					}
			}
		} else
			break; /* no init string - stop modem init */

		udelay(100000);
	}

	udelay(100000);

	/* final stage - wait for connect */
	for(;i > 1;) { /* if 'i' > 1 - wait for connection
				  message from modem */
		mdm_readline(console_buffer, CFG_CBSIZE);
		dbg("ini_f: [%s]", console_buffer);
		if (strncmp(console_buffer, "CONNECT", 7) == 0) {
			dbg("ini_f: connected");
			return 0;
		}
	}

	return 0;
}

/* 'inline' - We have to do it fast */
static inline void mdm_readline(char *buf, int bufsiz)
{
	char c;
	char *p;
	int n;

	n = 0;
	p = buf;
	for(;;) {
		c = serial_getc();

		/*		dbg("(%c)", c); */

		switch(c) {
		case '\r':
			break;
		case '\n':
			*p = '\0';
			return;

		default:
			if(n++ > bufsiz) {
				*p = '\0';
				return; /* sanity check */
			}
			*p = c;
			p++;
			break;
		}
	}
}
#endif	/* CONFIG_MODEM_SUPPORT */
