/*
 *  Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/errno.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/arch/common.h>

/*!
 * @file plat-mxc/irq.c
 *
 * @brief This file contains the AVIC implementation details.
 *
 * @ingroup Interrupt
 */

#define IRQ_BIT(irq)  (1 << (irq))

static uint32_t saved_wakeup_low, saved_wakeup_high;
static uint32_t suspend_wakeup_low, suspend_wakeup_high;

/*
 *****************************************
 * EDIO Registers                        *
 *****************************************
 */
#ifdef EDIO_BASE_ADDR

static const int mxc_edio_irq_map[] = {
	MXC_INT_EXT_INT0,
	MXC_INT_EXT_INT1,
	MXC_INT_EXT_INT2,
	MXC_INT_EXT_INT3,
	MXC_INT_EXT_INT4,
	MXC_INT_EXT_INT5,
	MXC_INT_EXT_INT6,
	MXC_INT_EXT_INT7,
};

static u32 edio_irq_type[MXC_MAX_EXT_LINES] = {
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
};

static int irq_to_edio(int irq)
{
	int i;
	int edio = -1;

	for (i = 0; i < MXC_MAX_EXT_LINES; i++) {
		if (mxc_edio_irq_map[i] == irq) {
			edio = i;
			break;
		}
	}
	return edio;
}

static void mxc_irq_set_edio_level(int irq, int trigger)
{
	int edio;
	unsigned short rval;

	edio = irq_to_edio(irq);
	rval = __raw_readw(EDIO_EPPAR);
	rval = (rval & (~(0x3 << (edio * 2)))) | (trigger << (edio * 2));
	__raw_writew(rval, EDIO_EPPAR);
}

static void mxc_irq_set_edio_dir(int irq, int dir)
{
	int edio;
	unsigned short rval;

	edio = irq_to_edio(irq);

	rval = __raw_readw(EDIO_EPDR);
	rval &= ~(1 << edio);
	rval |= (0 << edio);
	__raw_writew(rval, (EDIO_EPDR));

	/* set direction */
	rval = __raw_readw(EDIO_EPDDR);

	if (dir)
		/* out */
		rval |= (1 << edio);
	else
		/* in */
		rval &= ~(1 << edio);

	__raw_writew(rval, EDIO_EPDDR);

}

/*
 * Allows tuning the IRQ type , trigger and priority
 */
static void mxc_irq_set_edio(int irq, int fiq, int priority, int trigger)
{

	mxc_irq_set_edio_dir(irq, 0);
	/* set level */
	mxc_irq_set_edio_level(irq, trigger);
}
#endif

/*!
 * Disable interrupt number "irq" in the AVIC
 *
 * @param  irq          interrupt source number
 */
static void mxc_mask_irq(unsigned int irq)
{
	__raw_writel(irq, AVIC_INTDISNUM);
}

/*!
 * Enable interrupt number "irq" in the AVIC
 *
 * @param  irq          interrupt source number
 */
static void mxc_unmask_irq(unsigned int irq)
{
	__raw_writel(irq, AVIC_INTENNUM);
}

/*!
 * Set interrupt number "irq" in the AVIC as a wake-up source.
 *
 * @param  irq          interrupt source number
 * @param  enable       enable as wake-up if equal to non-zero
 * 			disble as wake-up if equal to zero
 *
 * @return       This function returns 0 on success.
 */
static int mxc_set_wake_irq(unsigned int irq, unsigned int enable)
{
	uint32_t *wakeup_intr;
	uint32_t irq_bit;

	if (irq < 32) {
		wakeup_intr = &suspend_wakeup_low;
		irq_bit = IRQ_BIT(irq);
	} else {
		wakeup_intr = &suspend_wakeup_high;
		irq_bit = IRQ_BIT(irq - 32);
	}

	if (enable) {
		*wakeup_intr |= irq_bit;
	} else {
		*wakeup_intr &= ~irq_bit;
	}

	return 0;
}

static struct irq_chip mxc_avic_chip = {
	.mask_ack = mxc_mask_irq,
	.mask = mxc_mask_irq,
	.unmask = mxc_unmask_irq,
	.set_wake = mxc_set_wake_irq,
};

#ifdef EDIO_BASE_ADDR
static void mxc_ack_edio(u32 irq)
{
	u16 edio = (u16) irq_to_edio(irq);
	if (edio_irq_type[edio] == IRQT_LOW) {
		/* Mask interrupt for level sensitive */
		mxc_mask_irq(irq);
	} else if (edio_irq_type[edio] == IRQT_HIGH) {
		/* clear edio interrupt */
		__raw_writew((1 << edio), EDIO_EPFR);
		/* dummy read for edio workaround */
		__raw_readw(EDIO_EPFR);
		/* Mask interrupt for level sensitive */
		mxc_mask_irq(irq);
	} else {
		/* clear edio interrupt */
		__raw_writew((1 << edio), EDIO_EPFR);
		/* dummy read for edio workaround */
		__raw_readw(EDIO_EPFR);
	}
}

static int mxc_edio_set_type(u32 irq, u32 type)
{
	edio_irq_type[irq_to_edio(irq)] = type;

	switch (type) {
	case IRQT_RISING:
		mxc_irq_set_edio_level(irq, 1);
		set_irq_handler(irq, handle_edge_irq);
		break;
	case IRQT_FALLING:
		mxc_irq_set_edio_level(irq, 2);
		set_irq_handler(irq, handle_edge_irq);
		break;
	case IRQT_BOTHEDGE:
		mxc_irq_set_edio_level(irq, 3);
		set_irq_handler(irq, handle_edge_irq);
		break;
	case IRQT_LOW:
		mxc_irq_set_edio_level(irq, 0);
		set_irq_handler(irq, handle_level_irq);
		break;
	case IRQT_HIGH:
		/* EDIO doesn't really support high-level interrupts,
		 * so we're faking it
		 */
		mxc_irq_set_edio_level(irq, 1);
		set_irq_handler(irq, handle_level_irq);
		break;
	default:
		return -EINVAL;
		break;
	}
	return 0;
}

static struct irq_chip mxc_edio_chip = {
	.name = "MXC_EDIO",
	.ack = mxc_ack_edio,
	.mask = mxc_mask_irq,
	.unmask = mxc_unmask_irq,
	.set_type = mxc_edio_set_type,
	.set_wake = mxc_set_wake_irq,
};

#endif

#ifdef CONFIG_PM
/*!
 * This function puts the AVIC in low-power mode/state.
 * All the interrupts that are enabled are first saved.
 * Only those interrupts which registers as a wake source by calling
 * enable_irq_wake are enabled. All other interrupts are disabled.
 *
 * @param   dev  the system device structure used to give information
 *                on AVIC to suspend
 * @param   mesg the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_avic_suspend(struct sys_device *dev, pm_message_t mesg)
{
	saved_wakeup_high = __raw_readl(AVIC_INTENABLEH);
	saved_wakeup_low = __raw_readl(AVIC_INTENABLEL);

	__raw_writel(suspend_wakeup_high, AVIC_INTENABLEH);
	__raw_writel(suspend_wakeup_low, AVIC_INTENABLEL);

	return 0;
}

/*!
 * This function brings the AVIC back from low-power state.
 * All the interrupts enabled before suspension are re-enabled from
 * the saved information.
 *
 * @param   dev  the system device structure used to give information
 *                on AVIC to resume
 *
 * @return  The function always returns 0.
 */
static int mxc_avic_resume(struct sys_device *dev)
{
	__raw_writel(saved_wakeup_high, AVIC_INTENABLEH);
	__raw_writel(saved_wakeup_low, AVIC_INTENABLEL);

	return 0;
}

#else
#define mxc_avic_suspend  NULL
#define mxc_avic_resume   NULL
#endif				/* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct sysdev_class mxc_avic_sysclass = {
	set_kset_name("mxc_irq"),
	.suspend = mxc_avic_suspend,
	.resume = mxc_avic_resume,
};

/*!
 * This structure represents AVIC as a system device.
 * System devices follow a slightly different driver model.
 * They don't need to do dynammic driver binding, can't be probed,
 * and don't reside on any type of peripheral bus.
 * So, it is represented and treated a little differently.
 */
static struct sys_device mxc_avic_device = {
	.id = 0,
	.cls = &mxc_avic_sysclass,
};

/*
 * This function is used to get the AVIC Lo and Hi interrupts
 * that are enabled as wake up sources to wake up the core from suspend
 */
void mxc_get_wake_irq(u32 * wake_src[])
{
	*wake_src[0] = __raw_readl(AVIC_INTENABLEL);
	*wake_src[1] = __raw_readl(AVIC_INTENABLEH);
}

/*!
 * This function initializes the AVIC hardware and disables all the
 * interrupts. It registers the interrupt enable and disable functions
 * to the kernel for each interrupt source.
 */
void __init mxc_init_irq(void)
{
	int i;
	u32 reg;

	/* put the AVIC into the reset value with
	 * all interrupts disabled
	 */
	__raw_writel(0, AVIC_INTCNTL);
	__raw_writel(0x1f, AVIC_NIMASK);

	/* disable all interrupts */
	__raw_writel(0, AVIC_INTENABLEH);
	__raw_writel(0, AVIC_INTENABLEL);

	/* all IRQ no FIQ */
	__raw_writel(0, AVIC_INTTYPEH);
	__raw_writel(0, AVIC_INTTYPEL);
	for (i = 0; i < MXC_MAX_INT_LINES; i++) {
#ifdef EDIO_BASE_ADDR
		if (irq_to_edio(i) != -1) {
			mxc_irq_set_edio(i, 0, 0, 0);
			set_irq_chip(i, &mxc_edio_chip);
		} else
#endif
		{
			set_irq_chip(i, &mxc_avic_chip);
		}
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID);
	}

	/* Set WDOG2's interrupt the highest priority level (bit 28-31) */
	reg = __raw_readl(AVIC_NIPRIORITY6);
	reg |= (0xF << 28);
	__raw_writel(reg, AVIC_NIPRIORITY6);

	if (MXC_INT_FORCE >= 32)
		__raw_writel(1 << (MXC_INT_FORCE & 31), AVIC_INTFRCH);
	else if (MXC_INT_FORCE >= 0)
		__raw_writel(1 << MXC_INT_FORCE, AVIC_INTFRCL);

	printk(KERN_INFO "MXC IRQ initialized\n");
}

/*!
 * This function registers AVIC hardware as a system device.
 * System devices will only be suspended with interrupts disabled, and
 * after all other devices have been suspended. On resume, they will be
 * resumed before any other devices, and also with interrupts disabled.
 *
 * @return       This function returns 0 on success.
 */
static int __init mxc_avic_sysinit(void)
{
	int ret = 0;

	ret = sysdev_class_register(&mxc_avic_sysclass);
	if (ret == 0) {
		ret = sysdev_register(&mxc_avic_device);
	}

	return ret;
}

arch_initcall(mxc_avic_sysinit);
