/*
 *  nanddump.c
 *
 *  Copyright (C) 2007 Richard Titmuss (richard_titmuss@logitech.com)
 *
 * $Id: $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This utility writes test patterns to flash trying to find blocks
 *   that fail when erasing, writing or in verifying.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "mtd/mtd-user.h"


int fd;
struct mtd_info_user meminfo;
int *badblocks;


void display_help(void)
{
	printf("Usage: nandtest MTD-device\n");
	exit(0);
}


int test_bad_blocks()
{
	unsigned long long offset;
	int badblock, erasesize;

	printf("check bad blocks...\n");

	erasesize = meminfo.erasesize;
	badblocks = malloc((meminfo.size / erasesize) * sizeof(int));

	for (offset = 0; offset < meminfo.size; offset += erasesize) {
		if ((badblock = ioctl(fd, MEMGETBADBLOCK, &offset)) < 0) {
			perror("ioctl(MEMGETBADBLOCK)");
			return 1;
		}

		badblocks[offset / erasesize] = badblock;
		if (badblock) {
			printf("bad block %llx\n", offset);
		}
	}

	return 0;
}


int test_oob()
{
	struct mtd_oob_buf oob;
	unsigned char oobbuf[64];
	unsigned long long offset;
	int i, writesize;

	printf("check oob...\n");

	oob.start = 0;
	oob.length = 16;
	oob.ptr = oobbuf;

	writesize = meminfo.writesize;

	for (offset = 0; offset < meminfo.size; offset += writesize) {
		if (ioctl(fd, MEMREADOOB, &oob) < 0) {
			perror("ioctl(MEMGETBADBLOCK)");
			return 1;
		}

		for (i = 0; i < 16; i++) {
			if (oobbuf[i] != 0xFF) {
				goto dump_oob;
			}
		}
		continue;

	dump_oob:
		printf("offset %llx: ", offset);
		for (i = 0; i < 16; i++) {
			printf("%x ", oobbuf[i]);
		}
		printf("\n");
	}

	return 0;
}

int test_erase()
{
	struct erase_info_user erase;
	unsigned long long offset;
	int erasesize;

	printf("erasing...\n");

	erasesize = meminfo.erasesize;

	for (offset = 0; offset < meminfo.size; offset += erasesize) {
		if (badblocks[offset / erasesize]) {
			continue;
		}

		erase.start = offset;
		erase.length = erasesize;

		if (ioctl(fd, MEMERASE, &erase) < 0) {
			fprintf(stderr, "erase failed %llx: %s\n", offset, strerror(errno));
			continue;
		}
	}

	return 0;
}


int test_write(int pattern)
{
	unsigned long long offset;
	int erasesize, writesize;
	unsigned char buf[meminfo.writesize];

	printf("writing %x ...\n", pattern);
	memset(buf, pattern, meminfo.writesize);

	erasesize = meminfo.erasesize;
	writesize = meminfo.writesize;

	for (offset = 0; offset < meminfo.size; offset += writesize) {
		if (badblocks[(offset & (~meminfo.erasesize + 1)) / erasesize]) {
			continue;
		}

		if (pwrite(fd, buf, writesize, offset) < 0) {
			fprintf(stderr, "write failed %llx: %s\n", offset, strerror(errno));
		}
	}

	return 0;
}


int test_verify(int pattern)
{
	unsigned long long offset;
	int i, erasesize, writesize;
	unsigned char buf[meminfo.writesize];

	printf("verify %x ...\n", pattern);

	erasesize = meminfo.erasesize;
	writesize = meminfo.writesize;

	for (offset = 0; offset < meminfo.size; offset += writesize) {
		if (badblocks[(offset & (~meminfo.erasesize + 1)) / erasesize]) {
			continue;
		}

		if (pread(fd, buf, writesize, offset) < 0) {
			fprintf(stderr, "read failed %llx: %s\n", offset, strerror(errno));
		}

		for (i = 0; i < writesize; i++) {
			if (buf[i] != pattern) {
				fprintf(stderr, "verify failed %llx: %x != %x\n", offset, buf[i], pattern);
				continue;
			}
		}
	}

	return 0;
}


int main(int argc, char **argv)
{

	if (argc <= 1) {
		display_help();
	}

	/* Open the device */
	if ((fd = open(argv[1], O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	if (meminfo.type != MTD_NANDFLASH) {
		fprintf(stderr, "Not NAND flash\n");
		exit(1);
	}

	printf("flags:\t\t0x%x\n", meminfo.flags);
	printf("size:\t\t0x%x\n", meminfo.size);
	printf("writesize:\t0x%x\n", meminfo.writesize);
	printf("erasesize:\t0x%x\n", meminfo.erasesize);
	printf("oobsize:\t0x%x\n", meminfo.oobsize);
	printf("ecctype:\t%d\n", meminfo.ecctype);
	printf("eccsize:\t0x%x\n", meminfo.eccsize);

	// tests
	test_bad_blocks();
	test_oob();

	test_erase();
	test_write(0x00);
	test_verify(0x00);

	test_erase();
	test_write(0xFF);
	test_verify(0xFF);

	test_erase();
	test_write(0xAA);
	test_verify(0xAA);

	test_erase();
	test_write(0x55);
	test_verify(0x55);

	close(fd);
	exit(0);
}
