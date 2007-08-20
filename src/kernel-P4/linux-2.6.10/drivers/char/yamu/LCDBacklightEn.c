//===================================================================
// LCD Backlight enable컨트롤 드라이버
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
#include <asm/arch-s3c2413/map.h>
#include <asm/io.h>

//===================================================================
int LCDBacklightEn_MAJOR = 232;

//===================================================================
int LCDBacklightEn_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPBCON &=0xFFFFCFFF);	//GPB6
	(*(volatile unsigned int __force *)S3C2413_GPBCON |=0x00001000);	//GPB6 Output
	(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<6));		//GPB6 High
	(*(volatile unsigned int __force *)S3C2413_GPBDN &=~(1<<6));		//GPB6 Pull Down disable	

   	return 0;          /* success */
}


int LCDBacklightEn_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t LCDBacklightEn_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:		// off
			(*(volatile unsigned int __force *)S3C2413_GPBDAT &=~(1<<6));	//GPG6 Low
			break;
		case 0x01:		// on
			(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<6));	//GPG6 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations LCDBacklightEn_fops = {
    	write:    LCDBacklightEn_write,
    	open:    	LCDBacklightEn_open,
    	release:  	LCDBacklightEn_release,
};

//===================================================================
static int __init LCDBacklightEn_init(void)
{
    	int result;

    	result = register_chrdev(LCDBacklightEn_MAJOR, "LCDBacklightEn", &LCDBacklightEn_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[LCDBacklightEn]LCDBacklightEn_MAJOR = %d\n", LCDBacklightEn_MAJOR);
    	return 0;
}

static void __exit LCDBacklightEn_exit(void)
{
	unregister_chrdev(LCDBacklightEn_MAJOR, "LCDBacklightEn");
}

//===================================================================
module_init(LCDBacklightEn_init);
module_exit(LCDBacklightEn_exit);

MODULE_LICENSE("GPL");

