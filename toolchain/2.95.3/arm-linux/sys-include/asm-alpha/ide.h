/*
 *  linux/include/asm-alpha/ide.h
 *
 *  Copyright (C) 1994-1996  Linus Torvalds & authors
 */

/*
 *  This file contains the alpha architecture specific IDE code.
 */

#ifndef __ASMalpha_IDE_H
#define __ASMalpha_IDE_H

#ifdef __KERNEL__

#include <linux/config.h>

#ifndef MAX_HWIFS
#define MAX_HWIFS	4
#endif

#define ide__sti()	__sti()

#define ide_default_io_base(i)	((ide_ioreg_t)0)
#define ide_default_irq(b)	(0)

static __inline__ void ide_init_hwif_ports(hw_regs_t *hw, ide_ioreg_t data_port, ide_ioreg_t ctrl_port, int *irq)
{
	ide_ioreg_t reg = data_port;
	int i;

	memset(hw, 0, sizeof(*hw));
	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	if (ctrl_port) {
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
	} else {
		hw->io_ports[IDE_CONTROL_OFFSET] = data_port + 0x206;
	}
	if (irq != NULL)
		*irq = 0;
	hw->io_ports[IDE_IRQ_OFFSET] = 0;
}

/*
 * This registers the standard ports for this architecture with the IDE
 * driver.
 */
static __inline__ void ide_init_default_hwifs(void)
{
#ifndef CONFIG_BLK_DEV_IDEPCI
	hw_regs_t hw;

	ide_init_hwif_ports(&hw, 0x1f0, 0x3f6, NULL);
	hw.irq = 14;
	ide_register_hw(&hw, NULL);
	ide_init_hwif_ports(&hw, 0x170, 0x376, NULL);
	hw.irq = 15;
	ide_register_hw(&hw, NULL);
	ide_init_hwif_ports(&hw, 0x1e8, 0x3ee, NULL);
	hw.irq = 11;
	ide_register_hw(&hw, NULL);
	ide_init_hwif_ports(&hw, 0x168, 0x36e, NULL);
	hw.irq = 10;
	ide_register_hw(&hw, NULL);
#endif /* CONFIG_BLK_DEV_IDEPCI */
}

typedef union {
	unsigned all			: 8;	/* all of the bits together */
	struct {
		unsigned head		: 4;	/* always zeros here */
		unsigned unit		: 1;	/* drive select number, 0 or 1 */
		unsigned bit5		: 1;	/* always 1 */
		unsigned lba		: 1;	/* using LBA instead of CHS */
		unsigned bit7		: 1;	/* always 1 */
	} b;
	} select_t;

#define ide_request_irq(irq,hand,flg,dev,id)	request_irq((irq),(hand),(flg),(dev),(id))
#define ide_free_irq(irq,dev_id)		free_irq((irq), (dev_id))
#define ide_check_region(from,extent)		check_region((from), (extent))
#define ide_request_region(from,extent,name)	request_region((from), (extent), (name))
#define ide_release_region(from,extent)		release_region((from), (extent))

/*
 * The following are not needed for the non-m68k ports
 */
#define ide_ack_intr(hwif)		(1)
#define ide_fix_driveid(id)		do {} while (0)
#define ide_release_lock(lock)		do {} while (0)
#define ide_get_lock(lock, hdlr, data)	do {} while (0)

#endif /* __KERNEL__ */

#endif /* __ASMalpha_IDE_H */
