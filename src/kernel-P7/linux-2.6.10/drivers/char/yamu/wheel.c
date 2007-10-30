//===================================================================
// »Ÿ µÂ∂Û¿Ãπˆ
//===================================================================

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

//===================================================================
int Wheel_MAJOR = 235;

//===================================================================
int FallingDrv, WheelData;

//===================================================================
static irqreturn_t Wheel_CWA(int irq, void *dev_id, struct pt_regs *regs)
{
	int Triger, Drv;
		disable_irq(IRQ_EINT13);

		// GPG5 Normal Setting ( EINT Port Setting => Read Nothing)
		(*(volatile unsigned int __force *)S3C2413_GPGCON &=~(0x3<<10));

		// INT Triger(F/R) Status Check & Condition Status => L/R Process update
		Drv = __raw_readl(S3C2413_GPGDAT);
		Triger=Drv;
		Drv = ((Drv & 0x10)>>4);			//GPG4 => L/R Status Check
		Triger = ((Triger & 0x20)>>5);	//GPG5 => Rising/Falling Status Check

		switch (Triger)
		{
			case 0: // Falling
				FallingDrv=Drv;
				break;
			case 1:	//Rising
				if (FallingDrv!=Drv) 		//(Drv <= RisingDrv)
					switch (Drv)
					{
						case 0:
							WheelData=0x02;	// Right
							break;
						case 1:
							WheelData=0x01;	// Left
							break;
						default:
							break;
					}
				break;
			default:
				break;
		}

		// GPG5 INT Setting
		(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<10));
		(*(volatile unsigned int __force *)S3C2413_GPGCON |=(0x2<<10));
		enable_irq(IRQ_EINT13);
   		return 0;
}

//===================================================================
int Wheel_open (struct inode *inode, struct file *filp)
{
	disable_irq(IRQ_EINT13);
	(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<8));	// GPG4 Normal Input

	(*(volatile unsigned int __force *)S3C2413_GPGCON &=~(0x3<<10));	// GPG5 INT Setting
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=(0x2<<10));

	(*(volatile unsigned int __force *)S3C2413_GPGDN |=0x30);		// disable pull-down

	//(*(volatile unsigned int __force *)S3C2413_EINFLT3 &=0xFFFF00FF);	//filter clk(PCLK)
	//(*(volatile unsigned int __force *)S3C2413_EINFLT3 |=0x00003F00);	//filter clk(PCLK)

	(*(volatile unsigned int __force *)S3C2413_EXTINT1 &=0xFF0FFFFF);	//	GPG5 both edge triggered
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 |=0x00F00000);

	disable_irq(IRQ_EINT13);
    if (request_irq(IRQ_EINT13, Wheel_CWA, SA_SAMPLE_RANDOM,"WheelCWA", NULL))
	{
       	printk(KERN_ERR "wheel.c: Could not allocate ts IRQ_13_wheel !\n");
        	return -EIO;
	}
   	return 0;

}

int Wheel_release (struct inode *inode, struct file *filp)
{
	disable_irq(IRQ_EINT13);
	free_irq(IRQ_EINT13,NULL);
   	return 0;
}

//===================================================================
ssize_t Wheel_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char data[1];

	data[0]=WheelData;
	copy_to_user(buf, data, count);

	WheelData = 0;
	return(count);
}

struct file_operations Wheel_fops = {
    	open:    	Wheel_open,
    	release:  	Wheel_release,
    	read:  	Wheel_read,
};

//===================================================================
static int __init Wheel_init(void)
{
    	int result;

    	result = register_chrdev(Wheel_MAJOR, "WHEEL", &Wheel_fops);
    	if (result < 0)
	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}
    	printk("[WHEEL]Wheel_MAJOR = %d\n", Wheel_MAJOR);
    	return 0;
}

static void __exit Wheel_exit(void)
{
	unregister_chrdev(Wheel_MAJOR, "WHEEL");
}

//===================================================================
module_init(Wheel_init);
module_exit(Wheel_exit);

MODULE_LICENSE("GPL");



