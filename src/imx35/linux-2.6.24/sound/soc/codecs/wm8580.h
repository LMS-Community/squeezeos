/*
 * wm8580.h  --  audio driver for WM8580
 *
 * Copyright 2008 Freescale Semiconductor, Inc.
 * Copyright 2008 Samsung Electronics.
 * Author: Ryu Euiyoul
 *         ryu.real@gmail.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _WM8580_H
#define _WM8580_H

#include <sound/soc.h>

#define WM8580_PLL_NONE   0
#define WM8580_PLLA       1
#define WM8580_PLLB       2

#define WM8580_MCLK       1
#define WM8580_DAC_CLKSEL 2
#define WM8580_CLKOUTSRC  3

#define WM8580_CLKSRC_MCLK 1
#define WM8580_CLKSRC_PLLA 2
#define WM8580_CLKSRC_PLLB 3
#define WM8580_CLKSRC_OSC  4
#define WM8580_CLKSRC_NONE 5

/*clock divider id's */
#define WM8580_BCLK_CLKDIV   0
#define WM8580_LRCLK_CLKDIV  1

typedef int (*hw_read_t) (void *control_data, long data, int length);
typedef int (*hw_write_t) (void *control_data, long data, int length);

extern const char wm8580_codec[SND_SOC_CODEC_NAME_SIZE];
extern const char wm8580_dai[SND_SOC_DAI_NAME_SIZE];

#endif
