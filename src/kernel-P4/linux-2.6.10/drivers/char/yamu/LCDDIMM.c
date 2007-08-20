//===================================================================
// LCD 백라이트 컨트롤 드라이버
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
int LCDDIMM_MAJOR = 238;

//===================================================================
int LCDDIMM_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFFC);  	// GPB0 : 10
	(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000002);  	// GPB0 : 10 - TOUT0
	(*(volatile unsigned int __force *)S3C2413_TCFG0 &= 0x00000000);	// Prescale 0x00
	(*(volatile unsigned int __force *)S3C2413_TCFG1 &= 0xFFFFFFF0);	// PCLK devide 1/2
	
	(*(volatile unsigned int __force *)S3C2413_TCNTB(0) = 0x100);		 
	(*(volatile unsigned int __force *)S3C2413_TCMPB(0) = 0xFF);		
	(*(volatile unsigned int __force *)S3C2413_TCON &= 0xFFFFFFF0);		
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000002);		// Manual update
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000009);		// Start timer0 with Auto reload
	(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00000002));	// Manual update mode disable
   	return 0;          /* success */
}


int LCDDIMM_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t LCDDIMM_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000002); 	// Manual update
	(*(volatile unsigned int __force *)S3C2413_TCMPB(0) = data[0]);		
	(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000009);		// Start timer0 with Auto reload
	(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00000002));	// Manual update mode disable
	return(count);
}


//===================================================================		
struct file_operations LCDDIMM_fops = {
    	write:    	LCDDIMM_write,
    	open:    	LCDDIMM_open,
    	release:  	LCDDIMM_release,
};

//===================================================================
static int __init LCDDIMM_init(void)
{
    	int result;

    	result = register_chrdev(LCDDIMM_MAJOR, "LCDDIMM", &LCDDIMM_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

    	printk("[LCDDIMM]LCDDIMM = %d\n", LCDDIMM_MAJOR);
    	return 0;
}

static void __exit LCDDIMM_exit(void)
{
	unregister_chrdev(LCDDIMM_MAJOR, "LCDDIMM");
		
}

//===================================================================
module_init(LCDDIMM_init);
module_exit(LCDDIMM_exit);

MODULE_LICENSE("GPL");
