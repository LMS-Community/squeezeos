//===================================================================
// Charger(LTC) HPWR 드라이버
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
int CHG_HPWR_MAJOR = 227;

//===================================================================
int CHG_HPWR_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPHCON &=0xFFFFCFFF);	//GPH 6
	(*(volatile unsigned int __force *)S3C2413_GPHCON |=0x00001000);	//GPH6 Output
	(*(volatile unsigned int __force *)S3C2413_GPHDAT |=(1<<6));		//GPH6 High
	(*(volatile unsigned int __force *)S3C2413_GPHDN &=~(1<<6));		//GPH6 Pull Down disable

   	return 0;          /* success */
}


int CHG_HPWR_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t CHG_HPWR_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPHDAT &=~(1<<6));	//GPH6 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPHDAT |=(1<<6));	//GPH6 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations CHG_HPWR_fops = {
    	write:    CHG_HPWR_write,
    	open:    CHG_HPWR_open,
    	release:  	CHG_HPWR_release,
};

//===================================================================
static int __init CHG_HPWR_init(void)
{
    	int result;

    	result = register_chrdev(CHG_HPWR_MAJOR, "CHG_HPWR", &CHG_HPWR_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[CHG_HPWR]CHG_HPWR_MAJOR = %d\n", CHG_HPWR_MAJOR);
    	return 0;
}

static void __exit CHG_HPWR_exit(void)
{
	unregister_chrdev(CHG_HPWR_MAJOR, "CHG_HPWR");
}

//===================================================================
module_init(CHG_HPWR_init);
module_exit(CHG_HPWR_exit);

MODULE_LICENSE("GPL");
