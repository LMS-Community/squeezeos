/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int g_size = 4;
unsigned long g_paddr;
int g_is_write;
uint32_t g_value = 0;
uint32_t g_count = 1;

int parse_cmdline(int argc, char ** argv)
{
	int cur_arg = 0;
	char * str;

	if (argc < 2)
		return -1;

	cur_arg++;
        if (strcmp(argv[cur_arg], "-8") == 0) {
		cur_arg++;
                g_size = 1;
        }
	else if (strcmp(argv[cur_arg], "-16") == 0) {
		cur_arg++;
                g_size = 2;
        }
	else if (strcmp(argv[cur_arg], "-32") == 0) {
		cur_arg++;
                g_size = 4;
        }
	if (cur_arg >= argc)
		return -1;

	g_paddr = strtoul(argv[cur_arg], NULL, 16);
	if (!g_paddr)
		return -1;

	if ( str = strchr(argv[cur_arg], '=') ) {
		g_is_write = 1;
		if (strlen(str) > 1) {
			str++;
			g_value = strtoul(str, NULL, 16);
			return 0;
		}
	}
	if (++cur_arg >= argc)
		return -1;

	if ((argv[cur_arg])[0] == '=' ) {
		g_is_write = 1;
		if (strlen(argv[cur_arg]) > 1) {
			(argv[cur_arg])++;
		} else {
			if (++cur_arg >= argc)
				return -1;
		}
		g_value = strtoul(argv[cur_arg], NULL, 16);
	}
	else {
		if (g_is_write)
			g_value = strtoul(argv[cur_arg], NULL, 16);
		else
			g_count = strtoul(argv[cur_arg], NULL, 16);
	}
	return 0;
}

void read_mem(void * addr, uint32_t count, uint32_t size)
{
	int i;
	uint8_t * addr8 = addr;
	uint16_t * addr16 = addr;
	uint32_t * addr32 = addr;

	switch (size)
	{
		case 1:
			for (i = 0; i < count; i++) {
				if ( (i % 16) == 0 )
					printf("\n0x%08X: ", g_paddr);
				printf(" %02X", addr8[i]);
				g_paddr++;
			}
			break;
		case 2:
			for (i = 0; i < count; i++) {
				if ( (i % 8) == 0 )
					printf("\n0x%08X: ", g_paddr);
				printf(" %04X", addr16[i]);
				g_paddr += 2;
			}
			break;
		case 4:
			for (i = 0; i < count; i++) {
				if ( (i % 4) == 0 )
					printf("\n0x%08X: ", g_paddr);
				printf(" %08X", addr32[i]);
				g_paddr += 4;
			}
			break;
	}
	printf("\n\n");

}

void write_mem(void * addr, uint32_t value, uint32_t size)
{
	int i;
	uint8_t * addr8 = addr;
	uint16_t * addr16 = addr;
	uint32_t * addr32 = addr;

	switch (size)
	{
		case 1:
			*addr8 = value;
			break;
		case 2:
			*addr16 = value;
			break;
		case 4:
			*addr32 = value;
			break;
	}
}

int main(int argc, char **argv)
{
	int fd;
	void * mem;
	void * aligned_vaddr;
	unsigned long aligned_paddr;
	uint32_t aligned_size;

	if (parse_cmdline(argc, argv)) {
		printf("Usage:\n\n" \
		       "Read memory: memtool [-8 | -16 | -32] <phys addr> <count>\n" \
		       "Write memory: memtool [-8 | -16 | -32] <phys addr>=<value>\n\n" \
		       "Default access size is 32-bit.\n\nAddress, count and value are all in hex.\n");
		return 1;
	}

	/* Align address to access size */
	g_paddr &= ~(g_size - 1);

	aligned_paddr = g_paddr & ~(4096 - 1);
	aligned_size = g_paddr - aligned_paddr + (g_count * g_size);
	aligned_size = (aligned_size + 4096 - 1) & ~(4096 - 1);

	if (g_is_write)
		printf("Writing %d-bit value 0x%X to address 0x%08X\n", g_size*8, g_value, g_paddr);
	else
		printf("Reading 0x%X count starting at address 0x%08X\n", g_count, g_paddr);

	if ((fd = open("/dev/mem", O_RDWR, 0)) < 0)
		return 1;

	aligned_vaddr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, aligned_paddr);
	if (aligned_vaddr == NULL) {
		printf("Error mapping address\n");
		close(fd);
		return 1;
	}

	mem = (void *)((uint32_t)aligned_vaddr + (g_paddr - aligned_paddr));

	if (g_is_write) {
		write_mem(mem, g_value, g_size);
	}
	else {
		read_mem(mem, g_count, g_size);
	}

	munmap(aligned_vaddr, aligned_size);
	close(fd);
	return 0;
}
