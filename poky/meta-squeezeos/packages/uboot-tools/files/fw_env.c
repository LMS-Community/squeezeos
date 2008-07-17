/*
 * (C) Copyright 2000-2003
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

//#include <linux/compiler.h>
#include <mtd/mtd-user.h>
#include "fw_env.h"

typedef unsigned char uchar;

#define	CMD_GETENV	"fw_printenv"
#define	CMD_SETENV	"fw_setenv"

typedef struct envdev_s {
	uchar devname[16];		/* Device name */
	ulong devoff;			/* Device offset */
	ulong env_size;			/* environment size */
	ulong erase_size;		/* device erase size */
} envdev_t;

static envdev_t envdevices[2];
static int curdev;

#define DEVNAME(i)    envdevices[(i)].devname
#define DEVOFFSET(i)  envdevices[(i)].devoff
#define ENVSIZE(i)    envdevices[(i)].env_size
#define DEVESIZE(i)   envdevices[(i)].erase_size

#define CFG_ENV_SIZE ENVSIZE(curdev)

#define ENV_SIZE      getenvsize()

typedef struct environment_s {
	ulong crc;			/* CRC32 over data bytes    */
	uchar flags;			/* active or obsolete */
	uchar *data;
} env_t;

static env_t environment;

static int HaveRedundEnv = 0;


#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

static uchar default_environment[] = {
#if defined(CONFIG_BOOTARGS)
	"bootargs=" CONFIG_BOOTARGS "\0"
#endif
#if defined(CONFIG_BOOTCOMMAND)
	"bootcmd=" CONFIG_BOOTCOMMAND "\0"
#endif
#if defined(CONFIG_RAMBOOTCOMMAND)
	"ramboot=" CONFIG_RAMBOOTCOMMAND "\0"
#endif
#if defined(CONFIG_NFSBOOTCOMMAND)
	"nfsboot=" CONFIG_NFSBOOTCOMMAND "\0"
#endif
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	"bootdelay=" MK_STR (CONFIG_BOOTDELAY) "\0"
#endif
#if defined(CONFIG_BAUDRATE) && (CONFIG_BAUDRATE >= 0)
	"baudrate=" MK_STR (CONFIG_BAUDRATE) "\0"
#endif
#ifdef	CONFIG_LOADS_ECHO
	"loads_echo=" MK_STR (CONFIG_LOADS_ECHO) "\0"
#endif
#ifdef	CONFIG_ETHADDR
	"ethaddr=" MK_STR (CONFIG_ETHADDR) "\0"
#endif
#ifdef	CONFIG_ETH1ADDR
	"eth1addr=" MK_STR (CONFIG_ETH1ADDR) "\0"
#endif
#ifdef	CONFIG_ETH2ADDR
	"eth2addr=" MK_STR (CONFIG_ETH2ADDR) "\0"
#endif
#ifdef	CONFIG_ETHPRIME
	"ethprime=" CONFIG_ETHPRIME "\0"
#endif
#ifdef	CONFIG_IPADDR
	"ipaddr=" MK_STR (CONFIG_IPADDR) "\0"
#endif
#ifdef	CONFIG_SERVERIP
	"serverip=" MK_STR (CONFIG_SERVERIP) "\0"
#endif
#ifdef	CFG_AUTOLOAD
	"autoload=" CFG_AUTOLOAD "\0"
#endif
#ifdef	CONFIG_ROOTPATH
	"rootpath=" MK_STR (CONFIG_ROOTPATH) "\0"
#endif
#ifdef	CONFIG_GATEWAYIP
	"gatewayip=" MK_STR (CONFIG_GATEWAYIP) "\0"
#endif
#ifdef	CONFIG_NETMASK
	"netmask=" MK_STR (CONFIG_NETMASK) "\0"
#endif
#ifdef	CONFIG_HOSTNAME
	"hostname=" MK_STR (CONFIG_HOSTNAME) "\0"
#endif
#ifdef	CONFIG_BOOTFILE
	"bootfile=" MK_STR (CONFIG_BOOTFILE) "\0"
#endif
#ifdef	CONFIG_LOADADDR
	"loadaddr=" MK_STR (CONFIG_LOADADDR) "\0"
#endif
#ifdef	CONFIG_PREBOOT
	"preboot=" CONFIG_PREBOOT "\0"
#endif
#ifdef	CONFIG_CLOCKS_IN_MHZ
	"clocks_in_mhz=" "1" "\0"
#endif
#if defined(CONFIG_PCI_BOOTDELAY) && (CONFIG_PCI_BOOTDELAY > 0)
	"pcidelay=" MK_STR (CONFIG_PCI_BOOTDELAY) "\0"
#endif
#ifdef  CONFIG_EXTRA_ENV_SETTINGS
	CONFIG_EXTRA_ENV_SETTINGS
#endif
	"\0"			/* Termimate env_t data with 2 NULs */
};

static int flash_io_read ();
static int flash_io_write ();
static uchar *envmatch (uchar * s1, uchar * s2);
static int env_init (void);
static int parse_config (void);

#if defined(CONFIG_FILE)
static int get_config (char *);
#endif
static inline ulong getenvsize (void)
{
	ulong rc = CFG_ENV_SIZE - sizeof (long);

	//	if (HaveRedundEnv)
	//		rc -= sizeof (char);
	return rc;
}

/*
 * Search the environment for a variable.
 * Return the value, if found, or NULL, if not found.
 */
unsigned char *fw_getenv (unsigned char *name)
{
	uchar *env, *nxt;

	if (env_init ())
		return (NULL);

	for (env = environment.data; *env; env = nxt + 1) {
		uchar *val;

		for (nxt = env; *nxt; ++nxt) {
			if (nxt >= &environment.data[ENV_SIZE]) {
				fprintf (stderr, "## Error: "
					"environment not terminated\n");
				return (NULL);
			}
		}
		val = envmatch (name, env);
		if (!val)
			continue;
		return (val);
	}
	return (NULL);
}

/*
 * Print the current definition of one, or more, or all
 * environment variables
 */
void fw_printenv (int argc, char *argv[])
{
	uchar *env, *nxt;
	int i, n_flag;

	if (env_init ())
		return;

	if (argc == 1) {		/* Print all env variables  */
		for (env = environment.data; *env; env = nxt + 1) {
			for (nxt = env; *nxt; ++nxt) {
				if (nxt >= &environment.data[ENV_SIZE]) {
					fprintf (stderr, "## Error: "
						"environment not terminated\n");
					return;
				}
			}

			printf ("%s\n", env);
		}
		return;
	}

	if (strcmp (argv[1], "-n") == 0) {
		n_flag = 1;
		++argv;
		--argc;
		if (argc != 2) {
			fprintf (stderr, "## Error: "
				"`-n' option requires exactly one argument\n");
			return;
		}
	} else {
		n_flag = 0;
	}

	for (i = 1; i < argc; ++i) {	/* print single env variables   */
		uchar *name = (uchar *) argv[i];
		uchar *val = NULL;

		for (env = environment.data; *env; env = nxt + 1) {

			for (nxt = env; *nxt; ++nxt) {
				if (nxt >= &environment.data[ENV_SIZE]) {
					fprintf (stderr, "## Error: "
						"environment not terminated\n");
					return;
				}
			}
			val = envmatch (name, env);
			if (val) {
				if (!n_flag) {
					fputs ((char *)name, stdout);
					putc ('=', stdout);
				}
				puts ((char *)val);
				break;
			}
		}
		if (!val)
			fprintf (stderr, "## Error: \"%s\" not defined\n", name);
	}
}

int fw_setenvs(int argc, char *argv[])
{
	int argNb;
	int len;
	uchar *env, *nxt;
	uchar *oldval = NULL;
	uchar *name;
	uchar *value;

	if (argc < 2) {
		return (EINVAL);
	}

	if (env_init ())
		return (errno);

	for (argNb=1;argNb<argc;argNb+=2) {
		name = (uchar *)(argv[argNb]);
		value = (uchar *)(argv[argNb+1]);
		/*
		 * search if variable with this name already exists
		 */
		for (nxt = env = environment.data; *env; env = nxt + 1) {
			for (nxt = env; *nxt; ++nxt) {
				if (nxt >= &environment.data[ENV_SIZE]) {
					fprintf (stderr, "## Error: "
						"environment not terminated\n");
					return (EINVAL);
				}
			}
			if ((oldval = envmatch (name, env)) != NULL)
				break;
		}
	
		/*
		 * Delete any existing definition
		 */
		if (oldval) {
			/*
			 * Ethernet Address and serial# can be set only once
			 */
			if ((strcmp ((char *)name, "ethaddr") == 0) ||
				(strcmp ((char *)name, "serial#") == 0)) {
				fprintf (stderr, "Can't overwrite \"%s\"\n", name);
				return (EROFS);
			}
	
			if (*++nxt == '\0') {
				*env = '\0';
			} else {
				for (;;) {
					*env = *nxt++;
					if ((*env == '\0') && (*nxt == '\0'))
						break;
					++env;
				}
			}
			*++env = '\0';
		}
	
		/* Delete only ? */
		if (strlen((char *)value)!=0) {
			/*
			 * Append new definition at the end
			 */
			for (env = environment.data; *env || *(env + 1); ++env);
			if (env > environment.data)
				++env;
			/*
			 * Overflow when:
			 * "name" + "=" + "val" +"\0\0"  > CFG_ENV_SIZE - (env-environment)
			 */
			len = strlen ((char *)name) + 2;
			/* add '=' for first arg, ' ' for all others */
				len += strlen ((char *)value) + 1;
			if (len > (&environment.data[ENV_SIZE] - env)) {
				fprintf (stderr,
					"Error: environment overflow\n");
				return (-1);
			}
			while ((*env = *name++) != '\0')
				env++;
			uchar *val = (uchar *)argv[argNb+1];
			*env = '=';
			while ((*++env = *val++) != '\0');
			}
		
			/* end is marked with double '\0' */
			*++env = '\0';
	}
	
	/* Update CRC */
	environment.crc = crc32 (0, environment.data, ENV_SIZE);

	/* write environment back to flash */
	if (flash_io_write ()) {
		fprintf (stderr, "Error: can't write fw_env to flash\n");
		return (-1);
	}

	return (0);
	
}


static int flash_io_write_part (int fd, int otherdev) {
	int len;
	erase_info_t erase;
	char *data = NULL;

	len = sizeof (environment.crc);

	/* erase existing environment */
	printf ("Erasing old environment %s:%lx\n", DEVNAME (otherdev), DEVOFFSET(otherdev));

	erase.length = DEVESIZE (otherdev);
	erase.start = DEVOFFSET (otherdev);
	if (ioctl (fd, MEMERASE, &erase) != 0) {
		fprintf (stderr, "MTD erase error on %s: %s\n",
			 DEVNAME (otherdev),
			 strerror (errno));
		return (-1);
	}

	printf ("Writing environment to %s:%lx\n", DEVNAME (otherdev), DEVOFFSET(otherdev));

	if ((data = malloc(DEVESIZE(otherdev))) == NULL) {
		fprintf(stderr, "Cannot malloc %ld bytes: %s\n",
			DEVESIZE(otherdev),
			strerror (errno));
		return (-1);
	}

	/* set environment data */
	memcpy(data, &environment, len);
	memcpy(data + len, environment.data, ENV_SIZE);
	
	/* write updated environment */
	if (lseek (fd, DEVOFFSET (otherdev), SEEK_SET) == -1) {
		fprintf (stderr,
			 "seek error on %s: %s\n",
			 DEVNAME (otherdev), strerror (errno));
		return (-1);
	}

	if (write (fd, data, DEVESIZE(otherdev)) != DEVESIZE(otherdev)) {
		fprintf (stderr,
			 "MTD write error on %s: %s\n",
			 DEVNAME (otherdev), strerror (errno));
		return (-1);
	}

	free(data);

	return 0;
}


static int flash_io_write ()
{
	int fd;

	if ((fd = open ((char *)DEVNAME (curdev), O_RDWR)) < 0) {
		fprintf (stderr,
			"Can't open %s: %s\n",
			DEVNAME (curdev), strerror (errno));
		return (-1);
	}

	if (HaveRedundEnv) {

		if (curdev == 0) {
			flash_io_write_part (fd, 1);
			flash_io_write_part (fd, 0);
		}
		else {
			flash_io_write_part (fd, 0);
			flash_io_write_part (fd, 1);
		}
	}
	else {
		flash_io_write_part (fd, 0);
	}

	if (close (fd)) {
		fprintf (stderr,
			"I/O error on %s: %s\n",
			DEVNAME (curdev), strerror (errno));
		return (-1);
	}

	/* everything ok */
	return (0);
}

static int flash_io_read()
{
	int fd, rc, len;

	//printf ("Reading environment to %s:%lx\n", DEVNAME (curdev), DEVOFFSET(curdev));

	if ((fd = open ((char *)DEVNAME (curdev), O_RDONLY)) < 0) {
		fprintf (stderr,
			"Can't open %s: %s\n",
			DEVNAME (curdev), strerror (errno));
		return (-1);
	}

	len = sizeof (environment.crc);
	//if (HaveRedundEnv) {
	//	len += sizeof (environment.flags);
	//}

	if (lseek (fd, DEVOFFSET (curdev), SEEK_SET) == -1) {
		fprintf (stderr,
			 "seek error on %s: %s\n",
			 DEVNAME (curdev), strerror (errno));
		return (-1);
	}
	if (read (fd, &environment, len) != len) {
		fprintf (stderr,
			 "CRC read error on %s: %s\n",
			 DEVNAME (curdev), strerror (errno));
		return (-1);
	}
	if ((rc = read (fd, environment.data, ENV_SIZE)) != ENV_SIZE) {
		fprintf (stderr,
			 "Read error on %s: %s\n",
			 DEVNAME (curdev), strerror (errno));
		return (-1);
	}

	if (close (fd)) {
		fprintf (stderr,
			"I/O error on %s: %s\n",
			DEVNAME (curdev), strerror (errno));
		return (-1);
	}

	/* everything ok */
	return (0);
}

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */

static uchar *envmatch (uchar * s1, uchar * s2)
{

	while (*s1 == *s2++)
		if (*s1++ == '=')
			return (s2);
	if (*s1 == '\0' && *(s2 - 1) == '=')
		return (s2);
	return (NULL);
}

/*
 * Prevent confusion if running from erased flash memory
 */
static int env_init (void)
{
	int crc1, crc1_ok;
	uchar *addr1;

	int crc2, crc2_ok;

	if (parse_config ())		/* should fill envdevices */
		return 1;

	if ((addr1 = calloc (1, ENV_SIZE)) == NULL) {
		fprintf (stderr,
			"Not enough memory for environment (%ld bytes)\n",
			ENV_SIZE);
		return (errno);
	}

	/* read environment block 0 from FLASH to local buffer */
	environment.data = addr1;
	curdev = 0;
	if (flash_io_read()) {
		return (errno);
	}

	crc1_ok = ((crc1 = crc32 (0, environment.data, ENV_SIZE))
			   == environment.crc);

	if (crc1_ok) {
		return 0;
	}

	if (!HaveRedundEnv) {
		if (!crc1_ok) {
			fprintf (stderr,
				"Warning: Bad CRC, using default environment\n");
			memcpy(environment.data, default_environment, sizeof(default_environment));
		}
	}

	memset(addr1, 0, ENV_SIZE);

	/* read environment block 1 from FLASH to local buffer */
	curdev = 1;
	if (flash_io_read()) {
		return (errno);
	}

	crc2_ok = ((crc2 = crc32 (0, environment.data, ENV_SIZE))
		   == environment.crc);

	if (crc2_ok) {
		return (0);
	}

	/* default environment */
	fprintf (stderr,
		 "Warning: Bad CRC, using default environment\n");
	memcpy(environment.data, default_environment, sizeof(default_environment));
	curdev = 0;

	return (0);
}


static int parse_config ()
{
	struct stat st;

#if defined(CONFIG_FILE)
	/* Fills in DEVNAME(), ENVSIZE(), DEVESIZE(). Or don't. */
	if (get_config (CONFIG_FILE)) {
		fprintf (stderr,
			"Cannot parse config file: %s\n", strerror (errno));
		return 1;
	}
#else
	strcpy ((char *)DEVNAME (0), (char *)DEVICE1_NAME);
	DEVOFFSET (0) = DEVICE1_OFFSET;
	ENVSIZE (0) = ENV1_SIZE;
	DEVESIZE (0) = DEVICE1_ESIZE;

#if defined HAVE_REDUND
	strcpy ((char *)DEVNAME (1), (char *)DEVICE2_NAME);
	DEVOFFSET (1) = DEVICE2_OFFSET;
	ENVSIZE (1) = ENV2_SIZE;
	DEVESIZE (1) = DEVICE2_ESIZE;
	HaveRedundEnv = 1;
#endif
#endif
	if (stat ((char *)DEVNAME (0), &st)) {
		fprintf (stderr,
			"Cannot access MTD device %s: %s\n",
			DEVNAME (0), strerror (errno));
		return 1;
	}

	if (HaveRedundEnv && stat ((char *)DEVNAME (1), &st)) {
		fprintf (stderr,
			"Cannot access MTD device %s: %s\n",
			DEVNAME (2), strerror (errno));
		return 1;
	}
	return 0;
}

#if defined(CONFIG_FILE)
static int get_config (char *fname)
{
	FILE *fp;
	int i = 0;
	int rc;
	char dump[128];

	if ((fp = fopen (fname, "r")) == NULL) {
		return 1;
	}

	while ((i < 2) && ((rc = fscanf (fp, "%s %lx %lx %lx",
				  DEVNAME (i),
				  &DEVOFFSET (i),
				  &ENVSIZE (i),
				  &DEVESIZE (i)  )) != EOF)) {

		/* Skip incomplete conversions and comment strings */
		if ((rc < 3) || (*DEVNAME (i) == '#')) {
			fgets (dump, sizeof (dump), fp);	/* Consume till end */
			continue;
		}

		i++;
	}
	fclose (fp);

	HaveRedundEnv = i - 1;
	if (!i) {			/* No valid entries found */
		errno = EINVAL;
		return 1;
	} else
		return 0;
}
#endif
