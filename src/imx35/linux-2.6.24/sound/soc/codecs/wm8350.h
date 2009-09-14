/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _WM8350_H
#define _WM8350_H

#define WM8350_SYSCLK	0

#define WM8350_S_CURVE_NONE		0x0 /* will pop */
#define WM8350_S_CURVE_FAST		0x1
#define WM8350_S_CURVE_MEDIUM		0x2
#define WM8350_S_CURVE_SLOW		0x3

#define WM8350_DISCHARGE_OFF	0x0
#define WM8350_DISCHARGE_FAST	0x1
#define WM8350_DISCHARGE_MEDIUM	0x2
#define WM8350_DISCHARGE_SLOW	0x3

#define WM8350_TIE_OFF_500R		0x0
#define WM8350_TIE_OFF_30K		0x1

struct wm8350_platform_data {
	int vmid_discharge_msecs; /* VMID D3Cold discharge time */
	int drain_msecs; /* D3Cold drain time */
	int cap_discharge_msecs; /* Cap D3Hot (from off) discharge time */
	int vmid_charge_msecs; /* vmid power up time */ 
	u32 vmid_s_curve:2; /* vmid enable s curve speed */
	u32 dis_out4:2; /* out4 discharge speed */
	u32 dis_out3:2; /* out3 discharge speed */
	u32 dis_out2:2; /* out2 discharge speed */
	u32 dis_out1:2; /* out1 discharge speed */
	u32 vroi_out4:1; /* out4 tie off */
	u32 vroi_out3:1; /* out3 tie off */
	u32 vroi_out2:1; /* out2 tie off */
	u32 vroi_out1:1; /* out1 tie off */
	u32 vroi_enable:1; /* enable tie off */
	u32 codec_current_d0:2; /* current level D0*/
	u32 codec_current_d3:2; /* current level D3 */
	u32 codec_current_charge:2; /* codec current @ vmid charge */
};

extern const char wm8350_codec[SND_SOC_CODEC_NAME_SIZE];
extern const char wm8350_hifi_dai[SND_SOC_DAI_NAME_SIZE];

#endif
