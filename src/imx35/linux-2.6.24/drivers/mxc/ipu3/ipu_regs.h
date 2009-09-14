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

/*
 * @file ipu_regs.h
 *
 * @brief IPU Register definitions
 *
 * @ingroup IPU
 */
#ifndef __IPU_REGS_INCLUDED__
#define __IPU_REGS_INCLUDED__

#define IPU_CM_REG_BASE		0x1E000000
#define IPU_IDMAC_REG_BASE	0x1E008000
#define IPU_ISP_REG_BASE	0x1E010000
#define IPU_DP_REG_BASE		0x1E018000
#define IPU_IC_REG_BASE		0x1E020000
#define IPU_IRT_REG_BASE	0x1E028000
#define IPU_CSI0_REG_BASE	0x1E030000
#define IPU_CSI1_REG_BASE	0x1E038000
#define IPU_DI0_REG_BASE	0x1E040000
#define IPU_DI1_REG_BASE	0x1E048000
#define IPU_SMFC_REG_BASE	0x1E050000
#define IPU_DC_REG_BASE		0x1E058000
#define IPU_DMFC_REG_BASE	0x1E060000
#define IPU_CPMEM_REG_BASE	0x1F000000
#define IPU_LUT_REG_BASE	0x1F020000
#define IPU_SRM_REG_BASE	0x1F040000
#define IPU_TPM_REG_BASE	0x1F060000
#define IPU_DC_TMPL_REG_BASE	0x1F080000
#define IPU_ISP_TBPR_REG_BASE	0x1F0C0000

extern u32 *ipu_cm_reg;
extern u32 *ipu_idmac_reg;
extern u32 *ipu_dp_reg;
extern u32 *ipu_ic_reg;
extern u32 *ipu_dc_reg;
extern u32 *ipu_dc_tmpl_reg;
extern u32 *ipu_dmfc_reg;
extern u32 *ipu_di_reg[];
extern u32 *ipu_tpmem_base;

/* Register addresses */
/* IPU Common registers */
#define IPU_CONF		(ipu_cm_reg)

#define IPU_SRM_PRI1		(ipu_cm_reg + 0x00A0/4)
#define IPU_SRM_PRI2		(ipu_cm_reg + 0x00A4/4)
#define IPU_FS_PROC_FLOW1	(ipu_cm_reg + 0x00A8/4)
#define IPU_FS_PROC_FLOW2	(ipu_cm_reg + 0x00AC/4)
#define IPU_FS_PROC_FLOW3	(ipu_cm_reg + 0x00B0/4)
#define IPU_FS_DISP_FLOW1	(ipu_cm_reg + 0x00B4/4)
#define IPU_FS_DISP_FLOW2	(ipu_cm_reg + 0x00B8/4)
#define IPU_SKIP		(ipu_cm_reg + 0x00BC/4)
#define IPU_DISP_ALT_CONF	(ipu_cm_reg + 0x00C0/4)
#define IPU_DISP_GEN		(ipu_cm_reg + 0x00C4/4)
#define IPU_DISP_ALT1		(ipu_cm_reg + 0x00C8/4)
#define IPU_DISP_ALT2		(ipu_cm_reg + 0x00CC/4)
#define IPU_DISP_ALT3		(ipu_cm_reg + 0x00D0/4)
#define IPU_DISP_ALT4		(ipu_cm_reg + 0x00D4/4)
#define IPU_SNOOP		(ipu_cm_reg + 0x00D8/4)
#define IPU_MEM_RST		(ipu_cm_reg + 0x00DC/4)
#define IPU_PM			(ipu_cm_reg + 0x00E0/4)
#define IPU_GPR			(ipu_cm_reg + 0x00E4/4)
#define IPU_CHA_CUR_BUF(ch)	(ipu_cm_reg + 0x0124/4 + (ch / 32))
#define IPU_ALT_CUR_BUF0	(ipu_cm_reg + 0x012C/4)
#define IPU_ALT_CUR_BUF1	(ipu_cm_reg + 0x0130/4)
#define IPU_SRM_STAT		(ipu_cm_reg + 0x0134/4)
#define IPU_PROC_TASK_STAT	(ipu_cm_reg + 0x0138/4)
#define IPU_DISP_TASK_STAT	(ipu_cm_reg + 0x013C/4)
#define IPU_CHA_BUF0_RDY(ch)	(ipu_cm_reg + 0x0140/4 + (ch / 32))
#define IPU_CHA_BUF1_RDY(ch)	(ipu_cm_reg + 0x0148/4 + (ch / 32))
#define IPU_CHA_DB_MODE_SEL(ch)	(ipu_cm_reg + 0x0150/4 + (ch / 32))
#define IPU_ALT_CHA_BUF0_RDY(ch) (ipu_cm_reg + 0x0158/4 + (ch / 32))
#define IPU_ALT_CHA_BUF1_RDY(ch) (ipu_cm_reg + 0x0160/4 + (ch / 32))
#define IPU_ALT_CHA_DB_MODE_SEL(ch) (ipu_cm_reg + 0x0168/4 + (ch / 32))

#define IPU_INT_CTRL(n)		(ipu_cm_reg + 0x003C/4 + ((n) - 1))
#define IPU_INT_CTRL_IRQ(irq)	IPU_INT_CTRL(((irq) / 32))
#define IPU_INT_STAT(n)		(ipu_cm_reg + 0x00E8/4 + ((n) - 1))
#define IPU_INT_STAT_IRQ(irq)	IPU_INT_STAT(((irq) / 32))

#define IPUIRQ_2_STATREG(irq)	(IPU_INT_STAT(1) + ((irq) / 32))
#define IPUIRQ_2_CTRLREG(irq)	(IPU_INT_CTRL(1) + ((irq) / 32))
#define IPUIRQ_2_MASK(irq)	(1UL << ((irq) & 0x1F))

/* CMOS Sensor Interface Registers */

/* Image Converter Registers */
#define IC_CONF			(ipu_ic_reg)
#define IC_PRP_ENC_RSC		(ipu_ic_reg + 0x0004/4)
#define IC_PRP_VF_RSC		(ipu_ic_reg + 0x0008/4)
#define IC_PP_RSC		(ipu_ic_reg + 0x000C/4)
#define IC_CMBP_1		(ipu_ic_reg + 0x0010/4)
#define IC_CMBP_2		(ipu_ic_reg + 0x0014/4)
#define IC_IDMAC_1		(ipu_ic_reg + 0x0018/4)
#define IC_IDMAC_2		(ipu_ic_reg + 0x001C/4)
#define IC_IDMAC_3		(ipu_ic_reg + 0x0020/4)
#define IC_IDMAC_4		(ipu_ic_reg + 0x0024/4)

#define IDMAC_CONF		(ipu_idmac_reg + 0x0000)
#define IDMAC_CHA_EN(ch)	(ipu_idmac_reg + 0x0004/4 + (ch/32))
#define IDMAC_SEP_ALPHA		(ipu_idmac_reg + 0x000C/4)
#define IDMAC_ALT_SEP_ALPHA	(ipu_idmac_reg + 0x0010/4)
#define IDMAC_CHA_PRI(ch)	(ipu_idmac_reg + 0x0014/4 + (ch/32))
#define IDMAC_WM_EN(ch)		(ipu_idmac_reg + 0x001C/4 + (ch/32))
#define IDMAC_CH_LOCK_EN_2	(ipu_idmac_reg + 0x0024/4)
#define IDMAC_SUB_ADDR_0	(ipu_idmac_reg + 0x0028/4)
#define IDMAC_SUB_ADDR_1	(ipu_idmac_reg + 0x002C/4)
#define IDMAC_SUB_ADDR_2	(ipu_idmac_reg + 0x0030/4)
#define IDMAC_BAND_EN(ch)	(ipu_idmac_reg + 0x0034/4 + (ch/32))
#define IDMAC_CHA_BUSY(ch)	(ipu_idmac_reg + 0x0040/4 + (ch/32))

#define DI_GENERAL(di)		(ipu_di_reg[di])
#define DI_BS_CLKGEN0(di)	(ipu_di_reg[di] + 0x0004/4)
#define DI_BS_CLKGEN1(di)	(ipu_di_reg[di] + 0x0008/4)

#define DI_SW_GEN0(di, gen)	(ipu_di_reg[di] + 0x000C/4 + (gen - 1))
#define DI_SW_GEN1(di, gen)	(ipu_di_reg[di] + 0x0030/4 + (gen - 1))
#define DI_STP_REP(di, gen)	(ipu_di_reg[di] + 0x0148/4 + (gen - 1)/2)
#define DI_SYNC_AS_GEN(di)	(ipu_di_reg[di] + 0x0054/4)
#define DI_DW_GEN(di, gen)	(ipu_di_reg[di] + 0x0058/4 + gen)
#define DI_DW_SET(di, gen, set)	(ipu_di_reg[di] + 0x0088/4 + gen + 0xC*set)
#define DI_SER_CONF(di)		(ipu_di_reg[di] + 0x015C/4)
#define DI_SSC(di)		(ipu_di_reg[di] + 0x0160/4)
#define DI_POL(di)		(ipu_di_reg[di] + 0x0164/4)
#define DI_AW0(di)		(ipu_di_reg[di] + 0x0168/4)
#define DI_AW1(di)		(ipu_di_reg[di] + 0x016C/4)
#define DI_SCR_CONF(di)		(ipu_di_reg[di] + 0x0170/4)
#define DI_STAT(di)		(ipu_di_reg[di] + 0x0174/4)

#define DMFC_RD_CHAN		(ipu_dmfc_reg)
#define DMFC_WR_CHAN		(ipu_dmfc_reg + 0x0004/4)
#define DMFC_WR_CHAN_DEF	(ipu_dmfc_reg + 0x0008/4)
#define DMFC_DP_CHAN		(ipu_dmfc_reg + 0x000C/4)
#define DMFC_DP_CHAN_DEF	(ipu_dmfc_reg + 0x0010/4)
#define DMFC_GENERAL1		(ipu_dmfc_reg + 0x0014/4)
#define DMFC_GENERAL2		(ipu_dmfc_reg + 0x0018/4)
#define DMFC_IC_CTRL		(ipu_dmfc_reg + 0x001C/4)

#define DC_MAP_CONF_PTR(n)	(ipu_dc_reg + 0x0108/4 + n/2)
#define DC_MAP_CONF_VAL(n)	(ipu_dc_reg + 0x0144/4 + n/2)

#define _RL_CH_2_OFFSET(ch)	((ch == 0) ? 8 : ( \
				 (ch == 1) ? 0x24 : ( \
				 (ch == 2) ? 0x40 : ( \
				 (ch == 5) ? 0x64 : ( \
				 (ch == 6) ? 0x80 : ( \
				 (ch == 8) ? 0x9C : ( \
				 (ch == 9) ? 0xBC : (-1))))))))
#define DC_RL_CH(ch, evt)	(ipu_dc_reg + _RL_CH_2_OFFSET(ch)/4 + evt/2)

#define DC_EVT_NF		0
#define DC_EVT_NL		1
#define DC_EVT_EOF		2
#define DC_EVT_NFIELD		3
#define DC_EVT_EOL		4
#define DC_EVT_EOFIELD		5
#define DC_EVT_NEW_ADDR		6
#define DC_EVT_NEW_CHAN		7
#define DC_EVT_NEW_DATA		8

#define dc_ch_offset(ch) \
({ \
	const char _offset[] = { \
		0, 0x1C, 0x38, 0x54, 0x58, 0x5C, 0x78, 0, 0x94, 0xB4}; \
	_offset[ch]; \
})
#define DC_WR_CH_CONF(ch)	(ipu_dc_reg + dc_ch_offset(ch)/4)
#define DC_WR_CH_ADDR(ch)	(ipu_dc_reg + dc_ch_offset(ch)/4 + 4/4)

#define DC_WR_CH_CONF_1		(ipu_dc_reg + 0x001C/4)
#define DC_WR_CH_ADDR_1		(ipu_dc_reg + 0x0020/4)
#define DC_WR_CH_CONF_5		(ipu_dc_reg + 0x005C/4)
#define DC_WR_CH_ADDR_5		(ipu_dc_reg + 0x0060/4)
#define DC_GEN			(ipu_dc_reg + 0x00D4/4)
#define DC_DISP_CONF1(disp)	(ipu_dc_reg + 0x00D8/4 + disp)
#define DC_DISP_CONF2(disp)	(ipu_dc_reg + 0x00E8/4 + disp)
#define DC_STAT			(ipu_dc_reg + 0x01C8/4)
#define DC_UGDE_0(evt)		(ipu_dc_reg + 0x0174/4 + evt*4)
#define DC_UGDE_1(evt)		(ipu_dc_reg + 0x0178/4 + evt*4)
#define DC_UGDE_2(evt)		(ipu_dc_reg + 0x017C/4 + evt*4)
#define DC_UGDE_3(evt)		(ipu_dc_reg + 0x0180/4 + evt*4)

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC
#define DP_COM_CONF(flow)	(ipu_dp_reg + flow/4)
#define DP_GRAPH_WIND_CTRL(flow) (ipu_dp_reg + 0x0004/4 + flow/4)
#define DP_FG_POS(flow)		(ipu_dp_reg + 0x0008/4 + flow/4)
#define DP_CSC_A_0(flow)	(ipu_dp_reg + 0x0044/4 + flow/4)
#define DP_CSC_A_1(flow)	(ipu_dp_reg + 0x0048/4 + flow/4)
#define DP_CSC_A_2(flow)	(ipu_dp_reg + 0x004C/4 + flow/4)
#define DP_CSC_A_3(flow)	(ipu_dp_reg + 0x0050/4 + flow/4)
#define DP_CSC_0(flow)		(ipu_dp_reg + 0x0054/4 + flow/4)
#define DP_CSC_1(flow)		(ipu_dp_reg + 0x0058/4 + flow/4)

enum {
	IPU_CONF_CSI0_EN = 0x00000001,
	IPU_CONF_CSI1_EN = 0x00000002,
	IPU_CONF_IC_EN = 0x00000004,
	IPU_CONF_ROT_EN = 0x00000008,
	IPU_CONF_DP_EN = 0x00000020,
	IPU_CONF_DI0_EN = 0x00000040,
	IPU_CONF_DI1_EN = 0x00000080,
	IPU_CONF_DMFC_EN = 0x00000400,
	IPU_CONF_DC_EN = 0x00000200,
	IPU_CONF_IDMAC_DIS = 0x00400000,
	IPU_CONF_IC_DMFC_SEL = 0x02000000,
	IPU_CONF_IC_DMFC_SYNC = 0x04000000,

	DI0_COUNTER_RELEASE = 0x01000000,
	DI1_COUNTER_RELEASE = 0x02000000,

	FS_PRPVF_ROT_SRC_SEL_MASK = 0x00000F00,
	FS_PRPVF_ROT_SRC_SEL_OFFSET = 8,
	FS_PRPENC_ROT_SRC_SEL_MASK = 0x0000000F,
	FS_PRPENC_ROT_SRC_SEL_OFFSET = 0,
	FS_PP_ROT_SRC_SEL_MASK = 0x000F0000,
	FS_PP_ROT_SRC_SEL_OFFSET = 16,
	FS_PP_SRC_SEL_MASK = 0x0000F000,
	FS_PP_SRC_SEL_OFFSET = 12,
	FS_PRP_SRC_SEL_MASK = 0x0F000000,
	FS_PRP_SRC_SEL_OFFSET = 24,
	FS_VF_IN_VALID = 0x80000000,
	FS_ENC_IN_VALID = 0x40000000,

	FS_PRPENC_DEST_SEL_MASK = 0x0000000F,
	FS_PRPENC_DEST_SEL_OFFSET = 0,
	FS_PRPVF_DEST_SEL_MASK = 0x000000F0,
	FS_PRPVF_DEST_SEL_OFFSET = 4,
	FS_PRPVF_ROT_DEST_SEL_MASK = 0x000000F0,
	FS_PRPVF_ROT_DEST_SEL_OFFSET = 8,
	FS_PP_DEST_SEL_MASK = 0x0000F000,
	FS_PP_DEST_SEL_OFFSET = 12,
	FS_PP_ROT_DEST_SEL_MASK = 0x000F0000,
	FS_PP_ROT_DEST_SEL_OFFSET = 16,
	FS_PRPENC_ROT_DEST_SEL_MASK = 0x00F00000,
	FS_PRPENC_ROT_DEST_SEL_OFFSET = 20,

	FS_DC1_SRC_SEL_MASK = 0x00F00000,
	FS_DC1_SRC_SEL_OFFSET = 20,
	FS_DC2_SRC_SEL_MASK = 0x000F0000,
	FS_DC2_SRC_SEL_OFFSET = 16,
	FS_DP_SYNC0_SRC_SEL_MASK = 0x0000000F,
	FS_DP_SYNC0_SRC_SEL_OFFSET = 0,
	FS_DP_SYNC1_SRC_SEL_MASK = 0x000000F0,
	FS_DP_SYNC1_SRC_SEL_OFFSET = 4,
	FS_DP_ASYNC0_SRC_SEL_MASK = 0x00000F00,
	FS_DP_ASYNC0_SRC_SEL_OFFSET = 8,
	FS_DP_ASYNC1_SRC_SEL_MASK = 0x0000F000,
	FS_DP_ASYNC1_SRC_SEL_OFFSET = 12,

	FS_AUTO_REF_PER_MASK = 0,
	FS_AUTO_REF_PER_OFFSET = 16,

	TSTAT_VF_MASK = 0x0000000C,
	TSTAT_VF_OFFSET = 2,
	TSTAT_VF_ROT_MASK = 0x00000300,
	TSTAT_VF_ROT_OFFSET = 8,
	TSTAT_ENC_MASK = 0x00000003,
	TSTAT_ENC_OFFSET = 0,
	TSTAT_ENC_ROT_MASK = 0x000000C0,
	TSTAT_ENC_ROT_OFFSET = 6,
	TSTAT_PP_MASK = 0x00000030,
	TSTAT_PP_OFFSET = 4,
	TSTAT_PP_ROT_MASK = 0x00000C00,
	TSTAT_PP_ROT_OFFSET = 10,

	TASK_STAT_IDLE = 0,
	TASK_STAT_ACTIVE = 1,
	TASK_STAT_WAIT4READY = 2,

	/* Register bits */
	SDC_COM_TFT_COLOR = 0x00000001UL,
	SDC_COM_FG_EN = 0x00000010UL,
	SDC_COM_GWSEL = 0x00000020UL,
	SDC_COM_GLB_A = 0x00000040UL,
	SDC_COM_KEY_COLOR_G = 0x00000080UL,
	SDC_COM_BG_EN = 0x00000200UL,
	SDC_COM_SHARP = 0x00001000UL,

	SDC_V_SYNC_WIDTH_L = 0x00000001UL,

	ADC_CONF_PRP_EN = 0x00000001L,
	ADC_CONF_PP_EN = 0x00000002L,
	ADC_CONF_MCU_EN = 0x00000004L,

	ADC_DISP_CONF_SL_MASK = 0x00000FFFL,
	ADC_DISP_CONF_TYPE_MASK = 0x00003000L,
	ADC_DISP_CONF_TYPE_XY = 0x00002000L,

	ADC_DISP_VSYNC_D0_MODE_MASK = 0x00000003L,
	ADC_DISP_VSYNC_D0_WIDTH_MASK = 0x003F0000L,
	ADC_DISP_VSYNC_D12_MODE_MASK = 0x0000000CL,
	ADC_DISP_VSYNC_D12_WIDTH_MASK = 0x3F000000L,

	/* Image Converter Register bits */
	IC_CONF_PRPENC_EN = 0x00000001,
	IC_CONF_PRPENC_CSC1 = 0x00000002,
	IC_CONF_PRPENC_ROT_EN = 0x00000004,
	IC_CONF_PRPVF_EN = 0x00000100,
	IC_CONF_PRPVF_CSC1 = 0x00000200,
	IC_CONF_PRPVF_CSC2 = 0x00000400,
	IC_CONF_PRPVF_CMB = 0x00000800,
	IC_CONF_PRPVF_ROT_EN = 0x00001000,
	IC_CONF_PP_EN = 0x00010000,
	IC_CONF_PP_CSC1 = 0x00020000,
	IC_CONF_PP_CSC2 = 0x00040000,
	IC_CONF_PP_CMB = 0x00080000,
	IC_CONF_PP_ROT_EN = 0x00100000,
	IC_CONF_IC_GLB_LOC_A = 0x10000000,
	IC_CONF_KEY_COLOR_EN = 0x20000000,
	IC_CONF_RWS_EN = 0x40000000,
	IC_CONF_CSI_MEM_WR_EN = 0x80000000,

	IC_IDMAC_1_CB0_BURST_16 = 0x00000001,
	IC_IDMAC_1_CB1_BURST_16 = 0x00000002,
	IC_IDMAC_1_CB2_BURST_16 = 0x00000004,
	IC_IDMAC_1_CB3_BURST_16 = 0x00000008,
	IC_IDMAC_1_CB4_BURST_16 = 0x00000010,
	IC_IDMAC_1_CB5_BURST_16 = 0x00000020,
	IC_IDMAC_1_CB6_BURST_16 = 0x00000040,
	IC_IDMAC_1_CB7_BURST_16 = 0x00000080,
	IC_IDMAC_1_PRPENC_ROT_MASK = 0x00003800,
	IC_IDMAC_1_PRPENC_ROT_OFFSET = 11,
	IC_IDMAC_1_PRPVF_ROT_MASK = 0x0001C000,
	IC_IDMAC_1_PRPVF_ROT_OFFSET = 14,
	IC_IDMAC_1_PP_ROT_MASK = 0x000E0000,
	IC_IDMAC_1_PP_ROT_OFFSET = 17,
	IC_IDMAC_1_PP_FLIP_RS = 0x00400000,
	IC_IDMAC_1_PRPVF_FLIP_RS = 0x00200000,
	IC_IDMAC_1_PRPENC_FLIP_RS = 0x00100000,

	IC_IDMAC_2_PRPENC_HEIGHT_MASK = 0x000003FF,
	IC_IDMAC_2_PRPENC_HEIGHT_OFFSET = 0,
	IC_IDMAC_2_PRPVF_HEIGHT_MASK = 0x000FFC00,
	IC_IDMAC_2_PRPVF_HEIGHT_OFFSET = 10,
	IC_IDMAC_2_PP_HEIGHT_MASK = 0x3FF00000,
	IC_IDMAC_2_PP_HEIGHT_OFFSET = 20,

	IC_IDMAC_3_PRPENC_WIDTH_MASK = 0x000003FF,
	IC_IDMAC_3_PRPENC_WIDTH_OFFSET = 0,
	IC_IDMAC_3_PRPVF_WIDTH_MASK = 0x000FFC00,
	IC_IDMAC_3_PRPVF_WIDTH_OFFSET = 10,
	IC_IDMAC_3_PP_WIDTH_MASK = 0x3FF00000,
	IC_IDMAC_3_PP_WIDTH_OFFSET = 20,

	CSI_SENS_CONF_DATA_FMT_SHIFT = 8,
	CSI_SENS_CONF_DATA_FMT_RGB_YUV444 = 0x00000000L,
	CSI_SENS_CONF_DATA_FMT_YUV422 = 0x00000200L,
	CSI_SENS_CONF_DATA_FMT_BAYER = 0x00000300L,

	CSI_SENS_CONF_VSYNC_POL_SHIFT = 0,
	CSI_SENS_CONF_HSYNC_POL_SHIFT = 1,
	CSI_SENS_CONF_DATA_POL_SHIFT = 2,
	CSI_SENS_CONF_PIX_CLK_POL_SHIFT = 3,
	CSI_SENS_CONF_SENS_PRTCL_SHIFT = 4,
	CSI_SENS_CONF_SENS_CLKSRC_SHIFT = 7,
	CSI_SENS_CONF_DATA_WIDTH_SHIFT = 10,
	CSI_SENS_CONF_EXT_VSYNC_SHIFT = 15,
	CSI_SENS_CONF_DIVRATIO_SHIFT = 16,

	PF_CONF_TYPE_MASK = 0x00000007,
	PF_CONF_TYPE_SHIFT = 0,
	PF_CONF_PAUSE_EN = 0x00000010,
	PF_CONF_RESET = 0x00008000,
	PF_CONF_PAUSE_ROW_MASK = 0x00FF0000,
	PF_CONF_PAUSE_ROW_SHIFT = 16,

	DI_DW_GEN_ACCESS_SIZE_OFFSET = 24,
	DI_DW_GEN_COMPONENT_SIZE_OFFSET = 16,

	DI_GEN_DI_CLK_EXT = 0x100000,
	DI_GEN_POLARITY_1 = 0x00000001,
	DI_GEN_POLARITY_2 = 0x00000002,
	DI_GEN_POLARITY_3 = 0x00000004,
	DI_GEN_POLARITY_4 = 0x00000008,
	DI_GEN_POLARITY_5 = 0x00000008,
	DI_GEN_POLARITY_6 = 0x00000008,
	DI_GEN_POLARITY_7 = 0x00000008,
	DI_GEN_POLARITY_8 = 0x00000008,

	DI_POL_DRDY_DATA_POLARITY = 0x00000080,
	DI_POL_DRDY_POLARITY_15 = 0x00000010,

	DI_VSYNC_SEL_OFFSET = 13,

	DC_WR_CH_CONF_FIELD_MODE = 0x00000200,
	DC_WR_CH_CONF_PROG_TYPE_OFFSET = 5,
	DC_WR_CH_CONF_PROG_TYPE_MASK = 0x000000E0,
	DC_WR_CH_CONF_PROG_DI_ID = 0x00000004,

	DC_UGDE_0_ODD_EN = 0x02000000,
	DC_UGDE_0_ID_CODED_MASK = 0x00000007,
	DC_UGDE_0_ID_CODED_OFFSET = 0,
	DC_UGDE_0_EV_PRIORITY_MASK = 0x00000078,
	DC_UGDE_0_EV_PRIORITY_OFFSET = 3,

	DP_COM_CONF_FG_EN = 0x00000001,
	DP_COM_CONF_GWSEL = 0x00000002,
	DP_COM_CONF_GWAM = 0x00000004,
	DP_COM_CONF_GWCKE = 0x00000008,
	DP_COM_CONF_CSC_DEF_MASK = 0x00000300,
	DP_COM_CONF_CSC_DEF_OFFSET = 8,
	DP_COM_CONF_CSC_DEF_FG = 0x00000300,
	DP_COM_CONF_CSC_DEF_BG = 0x00000200,
	DP_COM_CONF_CSC_DEF_BOTH = 0x00000100,
};

enum di_pins {
	DI_PIN11 = 0,
	DI_PIN12 = 1,
	DI_PIN13 = 2,
	DI_PIN14 = 3,
	DI_PIN15 = 4,
	DI_PIN16 = 5,
	DI_PIN17 = 6,
	DI_PIN_CS = 7,
};

enum di_sync_wave {
	DI_SYNC_NONE = -1,
	DI_SYNC_CLK = 0,
	DI_SYNC_INT_HSYNC = 1,
	DI_SYNC_HSYNC = 2,
	DI_SYNC_VSYNC = 3,
	DI_SYNC_DE = 5,
};

/* DC template opcodes */
#define WROD(lf)		(0x18 | (lf << 1))

#endif
