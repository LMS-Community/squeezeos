
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h> 
#include <linux/init.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/init.h>

#include <asm/mach-types.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/delay.h>
#include <asm/hardware.h> 

#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-timer.h>
#include <asm/arch/map.h>

#include <asm/io.h>

#define PULSES_PER_DETENT 2

static int pos;
static int lastAB;
static struct input_dev *wheel_dev;

static inline void reg_orrl(void __iomem *reg, unsigned int val)
{
	__raw_writel(__raw_readl(reg) | val, reg);
}

static inline void reg_andl(void __iomem *reg, unsigned int val)
{
	__raw_writel(__raw_readl(reg) & val, reg);
}


static irqreturn_t Wheel_CWA(int irq, void *dev_id)
{		
	int val, dir, AB, ABCD;
	unsigned int control;
	unsigned long flags;

	s3c2410_gpio_cfgpin(S3C2410_GPG4, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPG5, S3C2410_GPIO_INPUT);

	// XXXX without this delay we miss some key up events
	// XXXX FIXME experiment with different values here
	udelay(1);

	// read A and B
	val = __raw_readl(S3C2410_GPGDAT);
	AB = ( (val & 0x0030) >> 4);

	/* CW rotation:
	 * A B  C D (CD are previous AB state)
	 * 0 1  1 1  => 0x7
	 * 0 0  0 1  => 0x1
	 * 1 0  0 0  => 0x8
	 * 1 1  1 0  => 0xE
	 *
	 * CCW rotation
	 * A B  C D
	 * 1 0  1 1  => 0xB
	 * 0 0  1 0  => 0x2
	 * 0 1  0 0  => 0x4
	 * 1 1  0 1  => 0xD
	 */


	// wheel state
	ABCD = (AB << 2) | lastAB;

	switch (ABCD) {
	case 0x1:
	case 0x8:
	case 0x7:
	case 0xE:
		// CW rotation
		dir = 1;
		lastAB = AB;
		//printk("\tAB %x ABCD %x  1\n", AB, ABCD);
		break;

	case 0xB:
	case 0x4:
	case 0x2:
	case 0xD:
		// CCW rotation
		dir = -1;
		lastAB = AB;
		//printk("\tAB %x ABCD %x  -1\n", AB, ABCD);
		break;

	case 0x0:
	case 0x5:
	case 0xA:
	case 0xF:
		// no change
		dir = 0;
		break;

	default:
		// impossible state
		dir = 0;
		printk("bad wheel state %x\n", ABCD);
	}


	// update position and detent
	if (dir != 0) {
		pos += dir;
	}

	if (pos > PULSES_PER_DETENT) {
		input_report_rel(wheel_dev, REL_WHEEL, 1);
		input_sync(wheel_dev);
		pos = 0;
	}

	if (pos < -PULSES_PER_DETENT) {
		input_report_rel(wheel_dev, REL_WHEEL, -1);
		input_sync(wheel_dev);
		pos = 0;
	}

#ifdef CONFIG_JIVE_WHEEL_LEVELIRQ
	// both edge interrupts seem unreliable with zippy, so modify the
	// interrupt control register to use low and high interrupt levels
	control = 0;
	if (AB & 0x02) {
		control |= 0x8 << 20; // EINT13 low level
	}
	else {
		control |= 0x9 << 20; // EINT13 high level
	}
	if (AB & 0x01) {
		control |= 0x8 << 16; // EINT12 low level 
	}
	else {
		control |= 0x9 << 16; // EINT12 high level
	}

	local_irq_save(flags);

	reg_andl(S3C2410_EXTINT1, 0xFF00FFFF);
	reg_orrl(S3C2410_EXTINT1, control);

	local_irq_restore(flags);
#endif

	s3c2410_gpio_cfgpin(S3C2410_GPG4, S3C2410_GPIO_SFN2);
	s3c2410_gpio_cfgpin(S3C2410_GPG5, S3C2410_GPIO_SFN2);

	return IRQ_HANDLED;
}

#ifdef CONFIG_JIVE_WHEEL_LEVELIRQ
#define IRQ_FLAGS IRQF_TRIGGER_HIGH | IRQF_SAMPLE_RANDOM
#else
#define IRQ_FLAGS IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_SAMPLE_RANDOM
#endif

static int __init Wheel_init(void)
{
	/* For compatibility with the old bootloader allow s3c2413 machines
	 * to work here too. We can't use machine_is_s3c2413() here as that
	 * machine is not configured in the kernel.
	 *
	 * The S3C2413 hack should be removed when the old bootloader has
	 * been phased out.
	 */
	if (!(machine_is_jive() || machine_arch_type == MACH_TYPE_S3C2413))
		return 0;

	// we need to register at least one button and two relative axes 
	// to emulate a mouse
	wheel_dev = input_allocate_device();
	if (wheel_dev == NULL) {
		printk(KERN_ERR "failed to allocate input device\n");
		return -ENOMEM;
	}

	wheel_dev->evbit[0] = BIT(EV_REL);
	wheel_dev->relbit[0] = BIT(REL_WHEEL);
	wheel_dev->name = "Wheel";

	input_register_device(wheel_dev);

        if (request_irq(IRQ_EINT12, Wheel_CWA, IRQ_FLAGS, "CCW_B", NULL)) {
                 printk(KERN_ERR "Could not allocate CCW_B on IRQ_EINT12 !\n");
                 return -EIO;
	}

        if (request_irq(IRQ_EINT13, Wheel_CWA, IRQ_FLAGS, "CW_A", NULL)) {
                 printk(KERN_ERR "Could not allocate CW_A on IRQ_EINT13 !\n");
                 return -EIO;
	}

	s3c2410_gpio_irqfilter(S3C2410_GPG4, 1, 0);
	s3c2410_gpio_irqfilter(S3C2410_GPG5, 1, 0);

    	return 0;
}

static void __exit Wheel_exit(void) {
	free_irq(IRQ_EINT12, NULL);
	free_irq(IRQ_EINT13, NULL);

	input_unregister_device(wheel_dev);
}

module_init(Wheel_init);
module_exit(Wheel_exit);


MODULE_DESCRIPTION("Jive wheel driver");
MODULE_AUTHOR("Logitech");
MODULE_LICENSE("GPL");



