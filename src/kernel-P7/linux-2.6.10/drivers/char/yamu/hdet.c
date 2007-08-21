//===================================================================
// 헤드폰 검출 드라이버
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
int Headphone_MAJOR = 222;

//===================================================================
int Headphone_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<28));	//GPG14 Input
	(*(volatile unsigned int __force *)S3C2413_GPGDN |= (1<<14));		//GPG14 Pull Down Disable
   	return 0;          /* success */
}

int Headphone_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t Headphone_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPGDAT);
	data[0] = ((temp  & 0x4000)>>14);	//GPG14

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations Headphone_fops = {
    	read:    	Headphone_read,
    	open:    	Headphone_open,
    	release:  	Headphone_release,
};

//===================================================================
static int __init Headphone_init(void)
{
    	int result;

    	result = register_chrdev(Headphone_MAJOR, "HDET", &Headphone_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

    	printk("[HDET]Headphone_MAJOR = %d\n", Headphone_MAJOR);
    	return 0;
}

static void __exit Headphone_exit(void)
{
	unregister_chrdev(Headphone_MAJOR, "HDET");
}

//===================================================================
module_init(Headphone_init);
module_exit(Headphone_exit);

MODULE_LICENSE("GPL");

