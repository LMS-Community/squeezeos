/*
 *  linux/include/linux/l3/pcm1770.h
 *
 * Philips PCM1770 mixer device driver
 *
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#define PCM1770_NAME "pcm1770"

struct pcm1770_cfg {
	unsigned int fs:16;
	unsigned int format:3;
};

#define FMT_I2S		0
#define FMT_LSB16	1
#define FMT_LSB18	2
#define FMT_LSB20	3
#define FMT_MSB		4
#define FMT_LSB16MSB	5
#define FMT_LSB18MSB	6
#define FMT_LSB20MSB	7

#define L3_PCM1770_CONFIGURE	0x13410001

struct l3_gain {
	unsigned int	left:8;
	unsigned int	right:8;
	unsigned int	unused:8;
	unsigned int	channel:8;
};

#define L3_SET_VOLUME		0x13410002
#define L3_SET_TREBLE		0x13410003
#define L3_SET_BASS		0x13410004
#define L3_SET_GAIN		0x13410005

struct l3_agc {
	unsigned int	level:8;
	unsigned int	enable:1;
	unsigned int	attack:7;
	unsigned int	decay:8;
	unsigned int	channel:8;
};

#define L3_INPUT_AGC		0x13410006

struct pcm1770;

int pcm1770_configure(struct pcm1770 *uda, struct pcm1770_cfg *conf);
int pcm1770_mixer_ctl(struct pcm1770 *uda, int cmd, void *arg);
int pcm1770_open(struct pcm1770 *uda);
void pcm1770_close(struct pcm1770 *uda);

struct pcm1770 *pcm1770_attach(const char *adapter);
void pcm1770_detach(struct pcm1770 *uda);

