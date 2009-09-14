/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __INCLUDE_IPU_PRV_H__
#define __INCLUDE_IPU_PRV_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/arch/hardware.h>

/* Globals */
extern struct device *g_ipu_dev;
extern spinlock_t ipu_lock;
extern bool g_ipu_clk_enabled;
extern struct clk *g_ipu_clk;
extern struct clk *g_di_clk[2];
extern struct clk *g_ipu_csi_clk;
extern ipu_channel_t g_ipu_di_channel[2];

#define IDMA_CHAN_INVALID	0xFF

struct ipu_channel {
	u8 video_in_dma;
	u8 alpha_in_dma;
	u8 graph_in_dma;
	u8 out_dma;
};

int register_ipu_device(void);
ipu_color_space_t format_to_colorspace(uint32_t fmt);

void ipu_dump_registers(void);

uint32_t _ipu_channel_status(ipu_channel_t channel);

void _ipu_init_dc_mappings(void);
int _ipu_dp_init(ipu_channel_t channel, uint32_t in_pixel_fmt,
		 uint32_t out_pixel_fmt);
void _ipu_dp_uninit(ipu_channel_t channel);
void _ipu_dc_init(int dc_chan, int di, bool interlaced);
void _ipu_dc_uninit(int dc_chan);
void _ipu_dp_dc_enable(ipu_channel_t channel);
void _ipu_dp_dc_disable(ipu_channel_t channel);
void _ipu_dmfc_init(void);
void _ipu_dmfc_set_wait4eot(int dma_chan, int width);
int _ipu_chan_is_interlaced(ipu_channel_t channel);

void _ipu_ic_enable_task(ipu_channel_t channel);
void _ipu_ic_disable_task(ipu_channel_t channel);
void _ipu_ic_init_prpvf(ipu_channel_params_t *params, bool src_is_csi);
void _ipu_ic_uninit_prpvf(void);
void _ipu_ic_init_rotate_vf(ipu_channel_params_t *params);
void _ipu_ic_uninit_rotate_vf(void);
void _ipu_ic_init_csi(ipu_channel_params_t *params);
void _ipu_ic_uninit_csi(void);
void _ipu_ic_init_prpenc(ipu_channel_params_t *params, bool src_is_csi);
void _ipu_ic_uninit_prpenc(void);
void _ipu_ic_init_rotate_enc(ipu_channel_params_t *params);
void _ipu_ic_uninit_rotate_enc(void);
void _ipu_ic_init_pp(ipu_channel_params_t *params);
void _ipu_ic_uninit_pp(void);
void _ipu_ic_init_rotate_pp(ipu_channel_params_t *params);
void _ipu_ic_uninit_rotate_pp(void);
int _ipu_ic_idma_init(int dma_chan, uint16_t width, uint16_t height,
		      int burst_size, ipu_rotate_mode_t rot);

#endif				/* __INCLUDE_IPU_PRV_H__ */
