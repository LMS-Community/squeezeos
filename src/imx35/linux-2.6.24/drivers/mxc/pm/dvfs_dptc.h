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
 * @file dvfs_dptc.h
 *
 * @brief MXC dvfs & dptc header file.
 *
 * @ingroup PM_MX27 PM_MX31 PM_MXC91321
 */
#ifndef __DVFS_DPTC_H__
#define __DVFS_DPTC_H__

#include <asm/arch/pm_api.h>
#include <asm/hardware.h>
#include <asm/arch/dvfs_dptc_struct.h>

#ifdef CONFIG_ARCH_MX27
#include <asm/arch/dma.h>
#else
#include <asm/arch/sdma.h>
#endif

#ifdef CONFIG_ARCH_MX3
#include "dvfs_dptc_table_mx31.h"
#include "dvfs_dptc_table_mx31_27ckih.h"
#include "dvfs_dptc_table_mx31_rev2.h"
#endif
#ifdef CONFIG_ARCH_MXC91321
#include "dvfs_dptc_table_mxc91321.h"
#endif
#ifdef CONFIG_ARCH_MX27
#include "dvfs_dptc_table_mx27.h"
#endif

#ifdef CONFIG_MXC_DVFS
#ifndef CONFIG_MXC_DPTC
/*!
 * DPTC Module Name
 */
#define DEVICE_NAME	"dvfs"

/*!
 * DPTC driver node Name
 */
#define NODE_NAME	"dvfs"
#endif				/* ifndef CONFIG_MXC_DPTC */
#ifdef CONFIG_MXC_DPTC
/*!
 * DPTC Module Name
 */
#define DEVICE_NAME	"dvfs_dptc"

/*!
 * DPTC driver node Name
 */
#define NODE_NAME	"dvfs_dptc"
#endif				/* ifdef CONFIG_MXC_DPTC */
#else				/* ifdef CONFIG_MXC_DVFS */
/*!
 * DPTC Module Name
 */
#define DEVICE_NAME	"dptc"

/*!
 * DPTC driver node Name
 */
#define NODE_NAME	"dptc"
#endif				/* ifdef CONFIG_MXC_DVFS */

#define MAX_TABLE_SIZE 8192

#endif				/* __DPTC_H__ */
