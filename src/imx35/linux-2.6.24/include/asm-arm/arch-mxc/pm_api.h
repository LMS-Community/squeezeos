/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @defgroup PM_MX27 Dynamic Process Temperature Compensation (DPTC) for i.MX27
 */
/*!
 * @defgroup PM_MX31 Dynamic Process Temperature Compensation (DPTC) & Dynamic Voltage Frequency Scaling (DVFS) Drivers for i.MX31
 */
/*!
 * @defgroup PM_MXC91321 Dynamic Process Temperature Compensation (DPTC) for MXC91321
 */

/*!
 * @file arch-mxc/pm_api.h
 *
 * @brief MXC PM API header file.
 *
 * @ingroup PM_MX27 PM_MX31 PM_MXC91321
 */
#ifndef __ASM_ARCH_MXC_PM_API_H__
#define __ASM_ARCH_MXC_PM_API_H__

#include <linux/ioctl.h>

/*!
 * PM IOCTL Magic Number
 */
#define PM_MAGIC  0xDC

/*!
 * PM IOCTL update PM translation table command.\n
 * This command changes the current DPTC table used by the driver. \n
 * The command receives a string pointer to a new DPTC table, parses
 * it and swiches the table.
 */
#define PM_IOCSTABLE		         _IOW(PM_MAGIC, 0, char*)

/*!
 * PM IOCTL get DPTC table command.\n
 * This command returns DPTC table used by the driver. \n
 * The command receives a string pointer for DPTC table,
 * and dumps the current working table.
 */
#define PM_IOCGTABLE		        _IOR(PM_MAGIC, 1, char*)

/*!
 * DPTC IOCTL enable command.\n
 * This command enables the dptc module.
 */
#define DPTC_IOCTENABLE			_IO(PM_MAGIC, 2)

/*!
 * DPTC IOCTL disable command.\n
 * This command disables the dptc module.
 */
#define DPTC_IOCTDISABLE		 _IO(PM_MAGIC, 3)

/*!
 * DPTC IOCTL enable reference circuit command.\n
 * This command enables the dptc reference circuits.
 */
#define DPTC_IOCSENABLERC			_IOW(PM_MAGIC, 4, unsigned char)

/*!
 * DPTC IOCTL disable reference circuit command.\n
 * This command disables the dptc reference circuits.
 */
#define DPTC_IOCSDISABLERC			_IOW(PM_MAGIC, 5, unsigned char)

/*!
 * DPTC IOCTL get current state command.\n
 * This command returns the current dptc module state (Enabled/Disabled).
 */
#define DPTC_IOCGSTATE         		_IO(PM_MAGIC, 6)

/*!
 * DPTC IOCTL set working point command.\n
 * This command sets working point according to parameter.
 */
#define DPTC_IOCSWP         		_IOW(PM_MAGIC, 7, unsigned int)

/*!
 * DVFS IOCTL enable command.\n
 * This command enables the dvfs module.
 */
#define DVFS_IOCTENABLE			_IO(PM_MAGIC, 8)

/*!
 * DVFS IOCTL disable command.\n
 * This command disables the dvfs module.
 */
#define DVFS_IOCTDISABLE		 _IO(PM_MAGIC, 9)

/*!
 * DVFS IOCTL get current state command.\n
 * This command returns the current dvfs module state (Enabled/Disabled).
 */
#define DVFS_IOCGSTATE         		_IO(PM_MAGIC, 10)

/*!
 * DVFS IOCTL set SW general purpose bits
 */
#define DVFS_IOCSSWGP         		_IOW(PM_MAGIC, 11, unsigned char)

/*!
 * DVFS IOCTL set wait-for-interrupt state
 */
#define DVFS_IOCSWFI         		_IOW(PM_MAGIC, 12, unsigned char)

/*!
 * PM IOCTL get current frequency command.\n
 * This command returns the current frequency in Hz
 */
#define PM_IOCGFREQ         		_IO(PM_MAGIC, 13)

/*!
 * DVFS IOCTL set frequency command.\n
 * This command sets frequency according to parameter. The parameter
 * is the index of required frequency in current table.
 */
#define DVFS_IOCSFREQ         		_IOW(PM_MAGIC, 14, unsigned int)

/*!
 * DVFS IOCTL set mode command.\n
 * This command sets DVFS mode.
 * 0 - HW mode, 1 - SW predictive mode
 */
#define DVFS_IOCSMODE         		_IOW(PM_MAGIC, 15, unsigned int)

#ifdef CONFIG_ARCH_MXC91321
/*!
 * This defines refercence circuits default status
 */
#define DPTC_REF_CIRCUITS_STATUS  0xE

/*!
 * DCVR register structure
 */
typedef struct {
	/*!
	 * Emergency limit threshold
	 */
	unsigned long EmergencyLimit:10;

	/*!
	 * Lower limit threshold
	 */
	unsigned long LowerLimit:11;

	/*!
	 * Upper limit threshold
	 */
	unsigned long UpperLimit:11;
} dcvr_reg_s;
#else
/*!
 * This defines refercence circuits default status
 */
#define DPTC_REF_CIRCUITS_STATUS  0xA

/*!
 * DCVR register structure
 */
typedef struct {
	/*!
	 * Reserved bits
	 */
	unsigned long Reserved:2;

	/*!
	 * Emergency limit threshold
	 */
	unsigned long EmergencyLimit:10;

	/*!
	 * Lower limit threshold
	 */
	unsigned long LowerLimit:10;

	/*!
	 * Upper limit threshold
	 */
	unsigned long UpperLimit:10;
} dcvr_reg_s;
#endif

/*!
 * DCVR register represented as a union between a 32 bit word and a bit field
 */
typedef union {

	/*!
	 * DCVR register as a 32 bit word
	 */
	unsigned int AsInt;

	/*!
	 * DCVR register as a bit field
	 */
	dcvr_reg_s AsStruct;
} dcvr_reg_u;

/*!
 * This struct represents DCVR registers state
 */
typedef struct {
	/*!
	 * dcvr registers values
	 */
	dcvr_reg_u dcvr_reg[4];
} dcvr_state;

/*!
 * This struct defines DPTC working point
 */
typedef struct {
	/*!
	 * Working point index
	 */
	int wp_index;
	/*!
	 * PMIC regulators values
	 */
	unsigned long pmic_values[4];
} dptc_wp;

/*!
 * This struct defines DVFS state
 */
typedef struct {
	/*!
	 * Flag for pll change when frequency should be decreased
	 */
	unsigned long pll_sw_down;
	/*!
	 * pdr0 register decrease value
	 */
	unsigned long pdr0_down;
	/*!
	 * pll register deccrease value
	 */
	unsigned long pll_down;
	/*!
	 * Flag for pll change when frequency should be increased
	 */
	unsigned long pll_sw_up;
	/*!
	 * pdr0 register increase value
	 */
	unsigned long pdr0_up;
	/*!
	 * pll register increase value
	 */
	unsigned long pll_up;
	/*!
	 * vscnt increase value
	 */
	unsigned long vscnt;
} dvfs_state;

/*!
 * This structure holds the dptc translation table.\n
 * This structure is used to translate a working point and frequency index to
 * voltage value and DPTC thresholds.\n
 * This structure is also used in the driver read and write operations.\n
 * During read operation this structure is received from the dptc driver,
 * and during write operation this structure should be sent to the driver.
 */
typedef struct {
	/*!
	 * Number of working points in dptc table
	 */
	int wp_num;

	/*!
	 * This variable holds the current working point
	 */
	int curr_wp;

	/*!
	 * DVFS translation table entries
	 */
	dvfs_state *table;

	/*!
	 * DCVR table
	 */
	dcvr_state **dcvr;

	/*!
	 * DPTC translation table entries
	 */
	dptc_wp *wp;

	/*
	 * Boolean flag. If it is 0 - uses 4 levels of frequency, 1 - uses 2 levels of frequency
	 */
	int use_four_freq;

	/*!
	 * Number of frequencies for each working point
	 */
	int dvfs_state_num;
} dvfs_dptc_tables_s;

/*!
 * DPTC log buffer entry structure
 */
typedef struct {

	/*!
	 * Log entry time in jiffies
	 */
	unsigned long jiffies;

	/*!
	 * Log entry working point value
	 */
	int wp;

	/*!
	 * Log entry voltage index
	 */
	int voltage;

	/*!
	 * Log entry frequency index
	 */
	int freq;
} dptc_log_entry_s;

/*!
 * This defines DVFS HW mode
 */
#define DVFS_HW_MODE   0

/*!
 * This defines DVFS predictive mode
 */
#define DVFS_PRED_MODE 1

#endif
