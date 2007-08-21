//===================================================================
// Charger(LTC) SUSP 드라이버
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
int CHG_SUSP_MAJOR = 226;

//===================================================================
int CHG_SUSP_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=~(0x3<<24));	//GPG 12
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=(0x1<<24));	//GPG 12 Output
	(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<12));		//GPG 12 High
	(*(volatile unsigned int __force *)S3C2413_GPGDN &=~(1<<12));		//GPG 12 Pull Down disable

   	return 0;          /* success */
}


int CHG_SUSP_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t CHG_SUSP_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPGDAT &=~(1<<12));	//GPG12 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<12));	//GPG12 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations CHG_SUSP_fops = {
    	write:    CHG_SUSP_write,
    	open:    CHG_SUSP_open,
    	release:  	CHG_SUSP_release,
};

//===================================================================
static int __init CHG_SUSP_init(void)
{
    	int result;

    	result = register_chrdev(CHG_SUSP_MAJOR, "CHG_SUSP", &CHG_SUSP_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[CHG_SUSP]CHG_SUSP_MAJOR = %d\n", CHG_SUSP_MAJOR);
    	return 0;
}

static void __exit CHG_SUSP_exit(void)
{
	unregister_chrdev(CHG_SUSP_MAJOR, "CHG_SUSP");
}

//===================================================================
module_init(CHG_SUSP_init);
module_exit(CHG_SUSP_exit);

MODULE_LICENSE("GPL");
