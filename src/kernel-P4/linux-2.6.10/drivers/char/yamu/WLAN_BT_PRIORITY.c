//===================================================================
// WLAN BT_PRIORITY 드라이버
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
int BT_PRIORITY_MAJOR = 221;

//===================================================================
int BT_PRIORITY_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPBCON &=0xFFCFFFFF);	//GPB 10
	(*(volatile unsigned int __force *)S3C2413_GPBCON |=0x00100000);	//GPB10 Output
	(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<10));		//GPB10 High
	(*(volatile unsigned int __force *)S3C2413_GPBDN &=~(1<<10));		//GPB10 Pull Down disable

   	return 0;          /* success */
}


int BT_PRIORITY_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t BT_PRIORITY_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPBDAT &=~(1<<10));	//GPB10 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<10));	//GPB10 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations BT_PRIORITY_fops = {
    	write:    BT_PRIORITY_write,
    	open:    BT_PRIORITY_open,
    	release:  	BT_PRIORITY_release,
};

//===================================================================
static int __init BT_PRIORITY_init(void)
{
    	int result;

    	result = register_chrdev(BT_PRIORITY_MAJOR, "BT_PRIORITY", &BT_PRIORITY_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[BT_PRIORITY]BT_PRIORITY_MAJOR = %d\n", BT_PRIORITY_MAJOR);
    	return 0;
}

static void __exit BT_PRIORITY_exit(void)
{
	unregister_chrdev(BT_PRIORITY_MAJOR, "BT_PRIORITY");
}

//===================================================================
module_init(BT_PRIORITY_init);
module_exit(BT_PRIORITY_exit);

MODULE_LICENSE("GPL");
