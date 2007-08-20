/*
 * linux/include/asm/ucb1200.h
 *
 * Copyright (C) 2000 G.Mate, Inc.
 *
 * Author: Tred Lim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __ASM_ARM_UCB1200_H
#define __ASM_ARM_UCB1200_H

#include <linux/types.h>

#define UCB1200_ADC_SYNC	0x00000001

struct ucb1200_arch_info {
	int	irq;			/* irq allocated for UCB1200 */
	int	flags;
};

struct ucb1200_low_level {
	int	(*arch_init)(struct ucb1200_arch_info *arch);
	void	(*arch_exit)(void);
	void	(*sib_enable)(void);
	void	(*sib_disable)(void);
	u16	(*sib_read_codec)(u16 addr);
	void	(*sib_write_codec)(u16 addr, u16 data);
	void	(*set_adc_sync)(int flag);
};


/* Make sure NR_IRQS to be defined in advance */
#include <asm/irq.h>
#include <asm/mach/irq.h>

#include <linux/sched.h>
#include <linux/interrupt.h>

#define UCB1200_IRQ(x)		(NR_IRQS + 1 + (x))

#define IRQ_UCB1200_IO0		UCB1200_IRQ(0)
#define IRQ_UCB1200_IO1		UCB1200_IRQ(1)
#define IRQ_UCB1200_IO2		UCB1200_IRQ(2)
#define IRQ_UCB1200_IO3		UCB1200_IRQ(3)
#define IRQ_UCB1200_IO4		UCB1200_IRQ(4)
#define IRQ_UCB1200_IO5		UCB1200_IRQ(5)
#define IRQ_UCB1200_IO6		UCB1200_IRQ(6)
#define IRQ_UCB1200_IO7		UCB1200_IRQ(7)
#define IRQ_UCB1200_IO8		UCB1200_IRQ(8)
#define IRQ_UCB1200_IO9		UCB1200_IRQ(9)
#define IRQ_UCB1200_ADC		UCB1200_IRQ(11)
#define IRQ_UCB1200_TSPX	UCB1200_IRQ(12)
#define IRQ_UCB1200_TSMX	UCB1200_IRQ(13)
#define IRQ_UCB1200_TCLIP	UCB1200_IRQ(14)
#define IRQ_UCB1200_ACLIP	UCB1200_IRQ(15)

#define UCB1200_IRQ_MAX		UCB1200_IRQ(15)
#define UCB1200_NR_IRQS		(UCB1200_IRQ(15) - UCB1200_IRQ(0) + 1)

#define UCB1200_IRQ_TO_IO(x)	((x) - UCB1200_IRQ(0))


/*
 * Codec registers 2, 3 and 4: interrupt related registers
 */
#define ADC_INT (1 << 11)
#define TSPX_INT (1 << 12)
#define TSMX_INT (1 << 13)

extern void
ucb1200_set_irq_edge(int irq_mask, int edge);

extern void
ucb1200_disable_irq(unsigned int irq);

extern void
ucb1200_enable_irq(unsigned int irq);

extern int
ucb1200_request_irq(unsigned int irq,
		    void (*handler)(int, void *, struct pt_regs *),
		    unsigned long irq_flags, const char *devname, void *dev_id);
extern void
ucb1200_free_irq(unsigned int irq, void *dev_id);


extern void
ucb1200_set_io_direction(int io, int dir);

extern int
ucb1200_test_io(int io);

extern void
ucb1200_set_io(int io, int level);


/*
 * UCB1200 registers
 */
#define UCB1200_REG_IO_DATA (0)
#define UCB1200_REG_IO_DIRECTION (1)
#define UCB1200_REG_RISE_INT_ENABLE (2)
#define UCB1200_REG_FALL_INT_ENABLE (3)
#define UCB1200_REG_INT_STATUS (4)
#define UCB1200_REG_TELECOM_CTL_A (5)
#define UCB1200_REG_TELECOM_CTL_B (6)
#define UCB1200_REG_AUDIO_CTL_A (7)
#define UCB1200_REG_AUDIO_CTL_B (8)
#define UCB1200_REG_TS_CTL (9)
#define UCB1200_REG_ADC_CTL (10)
#define UCB1200_REG_ADC_DATA (11)
#define UCB1200_REG_ID (12)
#define UCB1200_REG_MODE (13)

#define SIB_ZERO	0x8000

extern u16
ucb1200_read_reg(u16 reg);

extern void
ucb1200_write_reg(u16 reg, u16 data);


/*
 * UCB1200 register 10: ADC control register
 */
#define ADC_SYNC_ENA (1 << 0)
#define ADC_INPUT_MASK (7 << 2)
#define ADC_INPUT_TSPX (0 << 2)
#define ADC_INPUT_TSMX (1 << 2)
#define ADC_INPUT_TSPY (2 << 2)
#define ADC_INPUT_TSMY (3 << 2)
#define ADC_INPUT_AD0 (4 << 2)
#define ADC_INPUT_AD1 (5 << 2)
#define ADC_INPUT_AD2 (6 << 2)
#define ADC_INPUT_AD3 (7 << 2)
#define ADC_START (1 << 7)
#define ADC_ENA (1 << 15)

//extern int
//ucb1200_read_adc(int input, void *callback);


#endif	/* __ASM_ARM_UCB1200_H */
