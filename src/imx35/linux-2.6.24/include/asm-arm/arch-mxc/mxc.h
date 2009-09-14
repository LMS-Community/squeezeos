/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_H__
#define __ASM_ARCH_MXC_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

#ifndef __ASSEMBLY__
#include <linux/types.h>

/*!
 * @ingroup MSL_MX27 MSL_MX31 MSL_MXC91321  MSL_MX37
 */
/*!
 * gpio port structure
 */
struct mxc_gpio_port {
	u32 num;		/*!< gpio port number */
	u32 base;		/*!< gpio port base VA */
#ifdef MXC_GPIO_SPLIT_IRQ_2
	u16 irq_0_15, irq_16_31;
#else
	u16 irq;		/*!< irq number to the core */
#endif
	u16 virtual_irq_start;	/*!< virtual irq start number */
};
/*!
 * This structure is used to define the One wire platform data.
 * It includes search rom accelerator.
 */

struct mxc_w1_config {
	int search_rom_accelerator;
};
/*!
 * This structure is used to define the SPI master controller's platform
 * data. It includes the SPI  bus number and the maximum number of
 * slaves/chips it supports.
 */
struct mxc_spi_master {
	/*!
	 * SPI Master's bus number.
	 */
	unsigned int bus_num;
	/*!
	 * SPI Master's maximum number of chip selects.
	 */
	unsigned int maxchipselect;
	/*!
	 * CSPI Hardware Version.
	 */
	unsigned int spi_version;
};

struct mxc_ipu_config {
	int rev;
};

struct mxc_ir_platform_data {
	int uart_ir_mux;
	int ir_rx_invert;
	int ir_tx_invert;
	struct clk *uart_clk;
};

struct mxc_i2c_platform_data {
	u32 i2c_clk;
};
/*This struct is to define the number of SSIs on a platform,
DAM source port config, DAM external port config, and Regulator names*/
struct mxc_audio_platform_data {
	int ssi_num;
	int src_port;
	int ext_port;
	int intr_id_hp;
	struct clk *ssi_clk[2];
	char *regulator1;
	char *regulator2;
	void (*audio_set_clk) (int sample_rate);
};

struct mxc_spdif_platform_data {
	int spdif_tx;
	int spdif_rx;
	int spdif_clk_44100;
	int spdif_clk_48000;
	int spdif_clkid;
	struct clk *spdif_clk;
	struct clk *spdif_core_clk;
	void (*spdif_set_clk) (int sample_rate);
	void (*spdif_enable) (int enable);
};

struct mxc_asrc_platform_data {
	struct clk *asrc_core_clk;
	struct clk *asrc_audio_clk;
};

struct mxc_bt_platform_data {
	char *bt_vdd;
	char *bt_vdd_parent;
	char *bt_vusb;
	char *bt_vusb_parent;
	void (*bt_reset) (void);
};

struct mxc_lcd_platform_data {
	char *io_reg;
	char *core_reg;
	void (*reset) (void);
};

struct mxc_tsc_platform_data {
	char *vdd_reg;
	int penup_threshold;
	void (*active) (void);
	void (*inactive) (void);
};

struct mxc_tvout_platform_data {
	char *io_reg;
	char *core_reg;
	char *analog_reg;
	u32 detect_line;
};

/*! Platform data for the IDE drive structure. */
struct mxc_ide_platform_data {
	char *power_drive;	/*!< The power pointer */
	char *power_io;		/*!< The power pointer */
};

struct mxc_camera_platform_data {
	char *core_regulator;
	char *io_regulator;
	char *analog_regulator;
	char *gpo_regulator;
	u32 mclk;
};

/*gpo1-3 is in fixed state by hardware design,
 * only deal with reset pin and clock_enable pin
 * only poll mode can be used to control the chip,
 * interrupt mode is not supported by 3ds*/
struct mxc_fm_platform_data {
	char *reg_vio;
	char *reg_vdd;
	void (*gpio_get) (void);
	void (*gpio_put) (void);
	void (*reset) (void);
	void (*clock_ctl) (int flag);
};

struct mxc_mma7450_platform_data {
	char *reg_dvdd_io;
	char *reg_avdd;
	void (*gpio_pin_get) (void);
	void (*gpio_pin_put) (void);
	int int1;
	int int2;
};

struct mxc_keyp_platform_data {
	u16 *matrix;
	void (*active) (void);
	void (*inactive) (void);
};

struct mxc_unifi_platform_data {
	void (*hardreset) (void);
	void (*enable) (int en);
	/* power parameters */
	char *reg_gpo1;
	char *reg_gpo2;
	char *reg_1v5_ana_bb;
	char *reg_vdd_vpa;
	char *reg_1v5_dd;

	int host_id;

	void *priv;
};

struct mxc_gps_platform_data {
	char *core_reg;
	char *analog_reg;
};

struct mxc_mlb_platform_data {
	u32 buf_address;
	u32 phy_address;
	char *reg_nvcc;
	char *mlb_clk;
};

struct flexcan_platform_data {
	char *core_reg;
	char *io_reg;
	void (*xcvr_enable) (int id, int en);
	void (*active) (int id);
	void (*inactive) (int id);
};

extern void mxc_wd_reset(void);
extern void mxc_kick_wd(void);
unsigned long board_get_ckih_rate(void);

int mxc_snoop_set_config(u32 num, unsigned long base, int size);
int mxc_snoop_get_status(u32 num, u32 * statl, u32 * stath);

struct platform_device;
void mxc_pg_enable(struct platform_device *pdev);
void mxc_pg_disable(struct platform_device *pdev);

struct mxc_unifi_platform_data *get_unifi_plat_data(void);

#endif				/* __ASSEMBLY__ */

#define MUX_IO_P		29
#define MUX_IO_I		24
#define IOMUX_TO_GPIO(pin) 	((((unsigned int)pin >> MUX_IO_P) * GPIO_NUM_PIN) + ((pin >> MUX_IO_I) & ((1 << (MUX_IO_P - MUX_IO_I)) -1)))
#define IOMUX_TO_IRQ(pin)	(MXC_GPIO_INT_BASE + IOMUX_TO_GPIO(pin))
#define GPIO_TO_PORT(n)		(n / GPIO_NUM_PIN)
#define GPIO_TO_INDEX(n)	(n % GPIO_NUM_PIN)

/*
 *****************************************
 * AVIC Registers                        *
 *****************************************
 */
#define AVIC_BASE		IO_ADDRESS(AVIC_BASE_ADDR)
#define AVIC_INTCNTL		(AVIC_BASE + 0x00)	/* int control reg */
#define AVIC_NIMASK		(AVIC_BASE + 0x04)	/* int mask reg */
#define AVIC_INTENNUM		(AVIC_BASE + 0x08)	/* int enable number reg */
#define AVIC_INTDISNUM		(AVIC_BASE + 0x0C)	/* int disable number reg */
#define AVIC_INTENABLEH		(AVIC_BASE + 0x10)	/* int enable reg high */
#define AVIC_INTENABLEL		(AVIC_BASE + 0x14)	/* int enable reg low */
#define AVIC_INTTYPEH		(AVIC_BASE + 0x18)	/* int type reg high */
#define AVIC_INTTYPEL		(AVIC_BASE + 0x1C)	/* int type reg low */
#define AVIC_NIPRIORITY7	(AVIC_BASE + 0x20)	/* norm int priority lvl7 */
#define AVIC_NIPRIORITY6	(AVIC_BASE + 0x24)	/* norm int priority lvl6 */
#define AVIC_NIPRIORITY5	(AVIC_BASE + 0x28)	/* norm int priority lvl5 */
#define AVIC_NIPRIORITY4	(AVIC_BASE + 0x2C)	/* norm int priority lvl4 */
#define AVIC_NIPRIORITY3	(AVIC_BASE + 0x30)	/* norm int priority lvl3 */
#define AVIC_NIPRIORITY2	(AVIC_BASE + 0x34)	/* norm int priority lvl2 */
#define AVIC_NIPRIORITY1	(AVIC_BASE + 0x38)	/* norm int priority lvl1 */
#define AVIC_NIPRIORITY0	(AVIC_BASE + 0x3C)	/* norm int priority lvl0 */
#define AVIC_NIVECSR		(AVIC_BASE + 0x40)	/* norm int vector/status */
#define AVIC_FIVECSR		(AVIC_BASE + 0x44)	/* fast int vector/status */
#define AVIC_INTSRCH		(AVIC_BASE + 0x48)	/* int source reg high */
#define AVIC_INTSRCL		(AVIC_BASE + 0x4C)	/* int source reg low */
#define AVIC_INTFRCH		(AVIC_BASE + 0x50)	/* int force reg high */
#define AVIC_INTFRCL		(AVIC_BASE + 0x54)	/* int force reg low */
#define AVIC_NIPNDH		(AVIC_BASE + 0x58)	/* norm int pending high */
#define AVIC_NIPNDL		(AVIC_BASE + 0x5C)	/* norm int pending low */
#define AVIC_FIPNDH		(AVIC_BASE + 0x60)	/* fast int pending high */
#define AVIC_FIPNDL		(AVIC_BASE + 0x64)	/* fast int pending low */

/*
 *****************************************
 * EDIO Registers                        *
 *****************************************
 */
#ifdef EDIO_BASE_ADDR
#define EDIO_EPPAR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x00)
#define EDIO_EPDDR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x02)
#define EDIO_EPDR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x04)
#define EDIO_EPFR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x06)
#endif

/* DMA driver defines */
#define MXC_IDE_DMA_WATERMARK	32	/* DMA watermark level in bytes */
#define MXC_IDE_DMA_BD_NR	(512/3/4)	/* Number of BDs per channel */

#define DPTC_WP_SUPPORTED	17

#ifndef IS_MEM_DEVICE_NONSHARED
/* all peripherals on MXC so far are below 0x80000000 but leave L2CC alone */
#define IS_MEM_DEVICE_NONSHARED(x)  ((x) < 0x80000000 && (x) != L2CC_BASE_ADDR)
#endif

#ifndef __ASSEMBLY__
#include <linux/types.h>
struct dptc_wp {
	u32 dcvr0;
	u32 dcvr1;
	u32 dcvr2;
	u32 dcvr3;
	u32 regulator;
	u32 voltage;
};
struct cpu_wp {
	u32 pll_reg;
	u32 pll_rate;
	u32 cpu_rate;
	u32 pdr0_reg;
	u32 pdf;
	u32 mfi;
	u32 mfd;
	u32 mfn;
};

struct cpu_wp *get_cpu_wp(int *wp);

/*!
 * This function is called to put the DPTC in a low power state.
 *
 */
void dptc_disable(void);

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 */
void dptc_enable(void);

enum mxc_cpu_pwr_mode {
	WAIT_CLOCKED,		/* wfi only */
	WAIT_UNCLOCKED,		/* WAIT */
	WAIT_UNCLOCKED_POWER_OFF,	/* WAIT + SRPG */
	STOP_POWER_ON,		/* just STOP */
	STOP_POWER_OFF,		/* STOP + SRPG */
};

void mxc_cpu_lp_set(enum mxc_cpu_pwr_mode mode);
int tzic_enable_wake(int is_idle);

#endif

#endif				/*  __ASM_ARCH_MXC_H__ */
