//===================================================================
// Charger(LTC) ACPR 드라이버
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
int CHG_ACPR_MAJOR = 224;

//===================================================================
int CHG_ACPR_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFCFFFF);	//GPG8
	(*(volatile unsigned int __force *)S3C2413_GPGCON |= 0x00000000);	//GPG8  input
	(*(volatile unsigned int __force *)S3C2413_GPGDN &= ~(1<<8));		//GPG8 Pull Down Enable
   	return 0;          /* success */
}

int CHG_ACPR_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t CHG_ACPR_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPGDAT);
	data[0] = ((temp  & 0x100)>>8);	//GPG8

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations CHG_ACPR_fops = {
    	read:    	CHG_ACPR_read,
    	open:    	CHG_ACPR_open,
    	release:  	CHG_ACPR_release,
};

//===================================================================
static int __init CHG_ACPR_init(void)
{
    	int result;

    	result = register_chrdev(CHG_ACPR_MAJOR, "CHG_ACPR", &CHG_ACPR_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}
   /* if(SKELETON_MAJOR == 0)
		SKELETON_MAJOR = result;*/

    	printk("[CHG_ACPR]CHG_ACPR_MAJOR = %d\n", CHG_ACPR_MAJOR);
    	return 0;
}

static void __exit CHG_ACPR_exit(void)
{
	unregister_chrdev(CHG_ACPR_MAJOR, "CHG_ACPR");

}

//===================================================================
module_init(CHG_ACPR_init);
module_exit(CHG_ACPR_exit);

MODULE_LICENSE("GPL");
