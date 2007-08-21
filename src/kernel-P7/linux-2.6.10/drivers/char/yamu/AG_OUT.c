//===================================================================
// Codec AG_OUT 드라이버
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
int AG_OUT_MAJOR = 222;

//===================================================================
int AG_OUT_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFFCFFF);	//GPG6
	(*(volatile unsigned int __force *)S3C2413_GPGCON |= 0x00000000);	//GPG6  input
	(*(volatile unsigned int __force *)S3C2413_GPGDN &= ~(1<<6));		//GPG6 Pull Down Enable
   	return 0;          /* success */
}

int AG_OUT_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t AG_OUT_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPGDAT);

	data[0] = ((temp  & 0x40)>>6);	//GPG6
	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations AG_OUT_fops = {
    	read:    	AG_OUT_read,
    	open:    	AG_OUT_open,
    	release:  	AG_OUT_release,
};

//===================================================================
static int __init AG_OUT_init(void)
{
    	int result;

    	result = register_chrdev(AG_OUT_MAJOR, "AG_OUT", &AG_OUT_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

    	printk("[AG_OUT]AG_OUT_MAJOR = %d\n", AG_OUT_MAJOR);
    	return 0;
}

static void __exit AG_OUT_exit(void)
{
	unregister_chrdev(AG_OUT_MAJOR, "AG_OUT");

}

//===================================================================
module_init(AG_OUT_init);
module_exit(AG_OUT_exit);

MODULE_LICENSE("GPL");
