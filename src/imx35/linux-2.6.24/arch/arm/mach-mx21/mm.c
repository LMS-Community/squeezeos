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

/*!
 * @file mm.c
 *
 * @brief This file creates static mapping between physical to virtual memory.
 *
 * @ingroup Memory
 */

#include <linux/mm.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>

static struct map_desc mxc_io_desc[] __initdata = {
	{
	 .virtual = AIPI_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(AIPI_BASE_ADDR),
	 .length = AIPI_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = SAHB1_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(SAHB1_BASE_ADDR),
	 .length = SAHB1_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = X_MEMC_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(X_MEMC_BASE_ADDR),
	 .length = X_MEMC_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = CS1_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(CS1_BASE_ADDR),
	 .length = CS1_SIZE,
	 .type = MT_DEVICE}
};

void __init mxc_map_io(void)
{
	iotable_init(mxc_io_desc, ARRAY_SIZE(mxc_io_desc));
}
