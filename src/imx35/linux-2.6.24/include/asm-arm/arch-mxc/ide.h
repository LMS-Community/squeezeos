/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_IDE_H__
#define __ASM_ARCH_MXC_IDE_H__

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>

/*
 * Set up a hw structure for a specified data port, control port and IRQ.
 * This should follow whatever the default interface uses.
 */
static __inline__ void
ide_init_hwif_ports(hw_regs_t * hw, int data_port, int ctrl_port, int *irq)
{
	ide_ioreg_t reg;
	int i;
	int regincr = 1;

	memset(hw, 0, sizeof(*hw));

	reg = (ide_ioreg_t) data_port;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += regincr;
	}

	hw->io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) ctrl_port;

	if (irq)
		*irq = 0;
}

/*
 * This registers the standard ports for this architecture with the IDE
 * driver.
 */
static __inline__ void ide_init_default_hwifs(void)
{

}

#endif
