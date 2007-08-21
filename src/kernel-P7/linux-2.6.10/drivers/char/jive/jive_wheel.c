
#include <linux/config.h>
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

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/delay.h>
#include <asm/hardware.h> 

#include <asm/arch-s3c2413/regs-gpio.h>
#include <asm/arch-s3c2413/regs-timer.h>
#include <asm/arch-s3c2413/map.h>

#include <asm/io.h>


#define PULSES_PER_DETENT 3

static int pos;
static int lastAB;
static struct input_dev wheel_dev;



static irqreturn_t Wheel_CWA(int irq, void *dev_id, struct pt_regs *regs) {		
	int val, dir, AB, ABCD;
	unsigned int control;

	// GPG[5:4] Normal Setting ( EINT Port Setting => Read Nothing)
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFFF0FF);	

	// XXXX without this delay we miss some key up events
	// XXXX FIXME experiment with different values here
	udelay(10);

	// read A and B
	val = __raw_readl(S3C2413_GPGDAT);
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
		input_report_rel(&wheel_dev, REL_Y, 1);
		input_sync(&wheel_dev);
		pos = 0;
	}

	if (pos < -PULSES_PER_DETENT) {
		input_report_rel(&wheel_dev, REL_Y, -1);
		input_sync(&wheel_dev);
		pos = 0;
	}


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
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 &=0xFF00FFFF);
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 |= control);


	// GPG13 INT Setting
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFFF0FF);
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=0x00000A00);


	return IRQ_HANDLED;
}

static int __init Wheel_init(void)
{
	// we need to register at least one button and two relative axes 
	// to emulate a mouse
	wheel_dev.evbit[0] = BIT(EV_KEY) | BIT(EV_REL);
	wheel_dev.keybit[LONG(BTN_MOUSE)] = BIT(BTN_LEFT) | BIT(BTN_RIGHT);
	wheel_dev.relbit[0] = BIT(REL_X) | BIT(REL_Y);
	wheel_dev.name = "Wheel";

	input_register_device(&wheel_dev);

	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFFF0FF);	// GPG[5:4] INT Setting
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=0x00000A00);	
	(*(volatile unsigned int __force *)S3C2413_GPGDN |=0x0030);		// GPG[5:4] disable pull-down	
	
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 &=0xFF00FFFF);	// EINT[13:12] both edge triggered
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 |=0x00FF0000);

	disable_irq(IRQ_EINT12);
        if (request_irq(IRQ_EINT12, Wheel_CWA, SA_SAMPLE_RANDOM,"CCW_B", NULL)) {
                 printk(KERN_ERR "Could not allocate CCW_B on IRQ_EINT12 !\n");
                 return -EIO;
	}

	disable_irq(IRQ_EINT13);
        if (request_irq(IRQ_EINT13, Wheel_CWA, SA_SAMPLE_RANDOM,"CW_A", NULL)) {
                 printk(KERN_ERR "Could not allocate CW_A on IRQ_EINT13 !\n");
                 return -EIO;
	}

    	return 0;
}

static void __exit Wheel_exit(void) {
	free_irq(IRQ_EINT12, NULL);
	free_irq(IRQ_EINT13, NULL);

	input_unregister_device(&wheel_dev);
}

module_init(Wheel_init);
module_exit(Wheel_exit);


MODULE_DESCRIPTION("Jive wheel driver");
MODULE_AUTHOR("Logitech");
MODULE_LICENSE("GPL");



