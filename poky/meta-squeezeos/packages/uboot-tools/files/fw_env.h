/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To build the utility with the run-time configuration
 * uncomment the next line.
 * See included "fw_env.config" sample file (TRAB board)
 * for notes on configuration.
 */
/*#define CONFIG_FILE     "/etc/fw_env.config"*/

#define HAVE_REDUND /* For systems with 2 env sectors */

#define DEVICE1_NAME      "/dev/mtd/3"
#define DEVICE1_OFFSET    0x0000
#define ENV1_SIZE         0x4000
#define DEVICE1_ESIZE     0x4000

#define DEVICE2_NAME      "/dev/mtd/3"
#define DEVICE2_OFFSET    0x4000
#define ENV2_SIZE         0x4000
#define DEVICE2_ESIZE     0x4000


/*
 * !!! WARNING !!!
 *
 * If you update these defines you MUST also update the values in
 * include/configs/smdk2413.h.
 */
#define CONFIG_BOOTDELAY	1
#define CONFIG_BOOTARGS    	"root=/dev/mtdblock1 console=ttySAC0,115200 mem=64M init=/linuxrc"
#define CONFIG_BOOTCOMMAND	"nandr $(kernelblock) 1d0000 30008000; setenv bootargs $(bootargs) mtdset=$(mtdset); bootm"

#define CONFIG_EXTRA_ENV_SETTINGS "sw6=echo Factory reset; blink; nande b00 1400000; blink\0" \
                                  "kernelblock=c\0" \
                                  "mtdset=0\0"


extern		void  fw_printenv(int argc, char *argv[]);
extern unsigned char *fw_getenv  (unsigned char *name);
extern		int   fw_setenvs  (int argc, char *argv[]);

extern unsigned	long  crc32	 (unsigned long, const unsigned char *, unsigned);
