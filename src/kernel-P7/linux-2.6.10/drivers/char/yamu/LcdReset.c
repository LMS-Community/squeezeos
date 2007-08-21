//===================================================================
// LCD reset 컨트롤 드라이버
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
extern void Init_LDI(void);

//===================================================================
int LcdReset_MAJOR = 233;

//===================================================================
int LcdReset_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<26));	//GPG13
	(*(volatile unsigned int __force *)S3C2413_GPGCON |= (0x1<<26));	//GPG13 Output
	(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<13));		//GPG13 High
	(*(volatile unsigned int __force *)S3C2413_GPGDN &=~(1<<13));		//GPG13 Pull Down disable

   	return 0;          /* success */
}


int LcdReset_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t LcdReset_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:		// off
			(*(volatile unsigned int __force *)S3C2413_GPGDAT &=~(1<<13));	//GPG13 Low
			break;
		case 0x01:		// on
			(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<13));	//GPG13 High
			Init_LDI();
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations LcdReset_fops = {
    	write:    LcdReset_write,
    	open:    	LcdReset_open,
    	release:  	LcdReset_release,
};

//===================================================================
static int __init LcdReset_init(void)
{
    	int result;

    	result = register_chrdev(LcdReset_MAJOR, "LcdReset", &LcdReset_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[LcdReset]LcdReset_MAJOR = %d\n", LcdReset_MAJOR);
    	return 0;
}

static void __exit LcdReset_exit(void)
{
	unregister_chrdev(LcdReset_MAJOR, "LcdReset");
}

//===================================================================
module_init(LcdReset_init);
module_exit(LcdReset_exit);

MODULE_LICENSE("GPL");
