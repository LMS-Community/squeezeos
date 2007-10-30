//===================================================================
// key 백라이트 컨트롤 드라이버
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

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>

#include <asm/arch-s3c2413/regs-gpio.h>
#include <asm/arch-s3c2413/regs-timer.h>
#include <asm/arch-s3c2413/regs-irq.h>
#include <asm/arch-s3c2413/map.h>
#include <asm/io.h>

//===================================================================
int KeyBackLightDIMM_MAJOR = 230;

//===================================================================
int KeyBackLightDIMM_open (struct inode *inode, struct file *filp)
{

	(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFCF);  	// GPB2 : 2
	(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000020);  	// GPB2 : 2 - TOUT2
	(*(volatile unsigned int __force *)S3C2413_TCFG0 &= 0x00000000);	// Prescale 0x00
	(*(volatile unsigned int __force *)S3C2413_TCFG1 &= 0xFFFFF0FF);	// PCLK devide 1/2

	/*
	(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFF3F);  	// GPB3 : 3
	(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000080);  	// GPB3 : 3 - TOUT3
	(*(volatile unsigned int __force *)S3C2413_TCFG0 &= 0x00000000);	// Prescale 0x00
	(*(volatile unsigned int __force *)S3C2413_TCFG1 &= 0xFFFF0FFF);	// PCLK devide 1/2
	*/
	(*(volatile unsigned int __force *)S3C2413_TCNTB(2) = 0x100);
	(*(volatile unsigned int __force *)S3C2413_TCMPB(2) = 0xFF);
	/*
	(*(volatile unsigned int __force *)S3C2413_TCNTB(3) = 0x100);
	(*(volatile unsigned int __force *)S3C2413_TCMPB(3) = 0xFF);
	*/


	(*(volatile unsigned int __force *)S3C2413_TCON &= 0xFFFF0FFF);
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00002000);		// Manual update
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00009000);		// Start timer2 with Auto reload
	(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00002000));	// Manual update mode disable

	/*
	(*(volatile unsigned int __force *)S3C2413_TCON &= 0xFFF0FFFF);
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00020000);		// Manual update
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00090000);		// Start timer3 with Auto reload
	(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00020000));	// Manual update mode disable
	*/
	return 0;          /* success */
}


int KeyBackLightDIMM_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t KeyBackLightDIMM_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	//unsigned short real_data=0;

	copy_from_user(data, buf, count);

	//real_data = (unsigned short)(data[0]<<8);
	//real_data |= data[1];

	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00002000); 	// Manual update
	(*(volatile unsigned int __force *)S3C2413_TCMPB(2) = data[0]);
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00009000);		// Start timer2 with Auto reload
	(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00002000));	// Manual update mode disable

	//(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00020000); 	// Manual update
	//(*(volatile unsigned int __force *)S3C2413_TCMPB(3) = data[0]);
	//(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00090000);		// Start timer3 with Auto reload
	//(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00020000));	// Manual update mode disable

	return(count);
}


//===================================================================
struct file_operations KeyBackLightDIMM_fops = {
    	write:    	KeyBackLightDIMM_write,
    	open:    	KeyBackLightDIMM_open,
    	release:  	KeyBackLightDIMM_release,
};

//===================================================================
static int __init KeyBackLightDIMM_init(void)
{
    	int result;

    	result = register_chrdev(KeyBackLightDIMM_MAJOR, "KeyBackLightDIMM", &KeyBackLightDIMM_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

    	printk("[KeyBackLightDIMM]KeyBackLightDIMM = %d\n", KeyBackLightDIMM_MAJOR);
    	return 0;
}

static void __exit KeyBackLightDIMM_exit(void)
{
	unregister_chrdev(KeyBackLightDIMM_MAJOR, "KeyBackLightDIMM");

}

//===================================================================
module_init(KeyBackLightDIMM_init);
module_exit(KeyBackLightDIMM_exit);

MODULE_LICENSE("GPL");

