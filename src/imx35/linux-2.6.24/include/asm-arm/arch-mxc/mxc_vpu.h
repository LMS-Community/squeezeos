/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @defgroup VPU Video Processor Unit Driver
 */

/*!
 * @file arch-mxc/mxc_vpu.h
 *
 * @brief VPU system initialization and file operation definition
 *
 * @ingroup VPU
 */

#ifndef __ASM_ARCH_MXC_VPU_H__
#define __ASM_ARCH_MXC_VPU_H__

#include <linux/fs.h>

typedef struct vpu_mem_desc {
	u32 size;
	dma_addr_t phy_addr;
	u32 cpu_addr;		/* cpu address to free the dma mem */
	u32 virt_uaddr;		/* virtual user space address */
} vpu_mem_desc;

#define VPU_IOC_MAGIC  'V'

#define VPU_IOC_PHYMEM_ALLOC	_IO(VPU_IOC_MAGIC, 0)
#define VPU_IOC_PHYMEM_FREE	_IO(VPU_IOC_MAGIC, 1)
#define VPU_IOC_WAIT4INT	_IO(VPU_IOC_MAGIC, 2)
#define VPU_IOC_PHYMEM_DUMP	_IO(VPU_IOC_MAGIC, 3)
#define VPU_IOC_REG_DUMP	_IO(VPU_IOC_MAGIC, 4)
#define VPU_IOC_VL2CC_FLUSH	_IO(VPU_IOC_MAGIC, 5)
#define VPU_IOC_IRAM_SETTING	_IO(VPU_IOC_MAGIC, 6)

int vl2cc_init(u32 vl2cc_hw_base);
void vl2cc_enable(void);
void vl2cc_flush(void);
void vl2cc_disable(void);
void vl2cc_cleanup(void);

#endif
