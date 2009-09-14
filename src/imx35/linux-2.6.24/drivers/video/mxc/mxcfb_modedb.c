/*
 * Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <asm/arch/mxcfb.h>

struct fb_videomode mxcfb_modedb[] = {
	{
		"Wintek-480x272", // name
		60, // refresh
		480, // xres
		272, // yres
		111111, // pixel clock period in pico-second; 9MHz
		2, // left_margin
		2, // right_margin
		2, // upper_margin
		2, // lower_margin
		41, // hsync_len
		10, // vsync_len
		FB_SYNC_OE_ACT_HIGH | FB_SYNC_CLK_INVERT, // sync;
//		FB_SYNC_OE_ACT_HIGH | FB_SYNC_CLK_IDLE_EN | FB_SYNC_CLK_INVERT, // sync;
		FB_VMODE_NONINTERLACED, // vmode
		0 // flag
	},
	{
	 /* 240x320 @ 60 Hz */
	 "Sharp-QVGA", 60, 240, 320, 185925, 9, 16, 7, 9, 1, 1,
	 FB_SYNC_HOR_HIGH_ACT | FB_SYNC_SHARP_MODE |
	 FB_SYNC_CLK_INVERT | FB_SYNC_DATA_INVERT | FB_SYNC_CLK_IDLE_EN,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* 240x33 @ 60 Hz */
	 "Sharp-CLI", 60, 240, 33, 185925, 9, 16, 7, 9 + 287, 1, 1,
	 FB_SYNC_HOR_HIGH_ACT | FB_SYNC_SHARP_MODE |
	 FB_SYNC_CLK_INVERT | FB_SYNC_DATA_INVERT | FB_SYNC_CLK_IDLE_EN,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* 640x480 @ 60 Hz */
	 "NEC-VGA", 60, 640, 480, 38255, 144, 0, 34, 40, 1, 1,
	 FB_SYNC_VERT_HIGH_ACT | FB_SYNC_OE_ACT_HIGH,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* NTSC TV output */
	 "TV-NTSC", 60, 640, 480, 37538,
	 38, 858 - 640 - 38 - 3,
	 36, 518 - 480 - 36 - 1,
	 3, 1,
	 0,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* PAL TV output */
	 "TV-PAL", 50, 640, 480, 37538,
	 38, 960 - 640 - 38 - 32,
	 32, 555 - 480 - 32 - 3,
	 32, 3,
	 0,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* TV output VGA mode, 640x480 @ 65 Hz */
	 "TV-VGA", 60, 640, 480, 40574, 35, 45, 9, 1, 46, 5,
	 0, FB_VMODE_NONINTERLACED, 0,
	 },
};

int mxcfb_modedb_sz = ARRAY_SIZE(mxcfb_modedb);
