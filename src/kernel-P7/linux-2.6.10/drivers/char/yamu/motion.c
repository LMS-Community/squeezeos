//===================================================================
// 기울기 센서 드라이버
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


#define	MOVING			1
#define	NoMOVING		0
//===================================================================
int Motion_MAJOR = 239;
unsigned char MotionData=0;
//===================================================================

static irqreturn_t Motion_Interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
		disable_irq(IRQ_EINT14);

		MotionData = MOVING;

		enable_irq(IRQ_EINT14);
}


int Motion_open (struct inode *inode, struct file *filp)
{
	disable_irq(IRQ_EINT14);

	(*(volatile unsigned int __force *)S3C2413_GPGCON &=~(0x3<<12));	// GPG6 INT Setting
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=(0x2<<12));

	(*(volatile unsigned int __force *)S3C2413_GPGDN |=0x40);		// disable pull-down

	//(*(volatile unsigned int __force *)S3C2413_EINFLT3 &=0xFFFF00FF);	//filter clk(PCLK)
	//(*(volatile unsigned int __force *)S3C2413_EINFLT3 |=0x00003F00);	//filter clk(PCLK)

	(*(volatile unsigned int __force *)S3C2413_EXTINT1 &=0xF0FFFFFF);	//	GPG6 both edge triggered
	(*(volatile unsigned int __force *)S3C2413_EXTINT1 |=0x0F000000);

	disable_irq(IRQ_EINT14);
    if (request_irq(IRQ_EINT14, Motion_Interrupt, SA_SAMPLE_RANDOM,"Motion_Interrupt", NULL))
	{
       	printk(KERN_ERR "motion.c: Could not allocate ts IRQ_14_Motion !\n");
        	return -EIO;
	}

	//(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<12));	//GPG6 input
	//(*(volatile unsigned int __force *)S3C2413_GPGDN &= ~(1<<6));		//GPG6 Pull Down Enable
   	return 0;          /* success */
}

int Motion_release (struct inode *inode, struct file *filp)
{
	disable_irq(IRQ_EINT14);
	free_irq(IRQ_EINT14,NULL);
   	return 0;
}

//===================================================================
ssize_t Motion_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	//unsigned short temp;

	//temp = __raw_readl(S3C2413_GPGDAT);
	//	data[0] = ((temp  & 0x40)>>6);	//GPG6

	data[0] = MotionData;
	MotionData = NoMOVING;		// buffer clear

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations Motion_fops = {
    	read:    	Motion_read,
    	open:    	Motion_open,
    	release:  	Motion_release,
};

//===================================================================
static int __init Motion_init(void)
{
    	int result;

    	result = register_chrdev(Motion_MAJOR, "MOTION", &Motion_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

   /* if(SKELETON_MAJOR == 0)
		SKELETON_MAJOR = result;*/

    	printk("[MOTION]Motion_MAJOR = %d\n", Motion_MAJOR);
    	return 0;
}

static void __exit Motion_exit(void)
{
	unregister_chrdev(Motion_MAJOR, "MOTION");

}

//===================================================================
module_init(Motion_init);
module_exit(Motion_exit);

MODULE_LICENSE("GPL");
