//===================================================================
// 무선랜 파워 컨트롤 드라이버
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
int WLENPwr_MAJOR = 240;

//===================================================================
int WLENPwr_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xfff3ffff);	//GPG9
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=0x00040000);	//GPG9 Output
	(*(volatile unsigned int __force *)S3C2413_GPGDN &=~(1<<9));		//GPG9 Pull Down disable
   	return 0;          /* success */
}

int WLENPwr_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WLENPwr_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);

	switch (data[0])
	{
		case 0x00:		// off
			(*(volatile unsigned int __force *)S3C2413_GPGDAT &=~(1<<9));
			break;
		case 0x01:		// on
			(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<9));
			break;
		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations WLENPwr_fops = {
    	write:    	WLENPwr_write,
    	open:    	WLENPwr_open,
    	release:  	WLENPwr_release,
};

//===================================================================
static int __init WLENPwr_init(void)
{
    	int result;

    	result = register_chrdev(WLENPwr_MAJOR, "WLANP", &WLENPwr_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}

    	printk("[WLANP]WLENPwr_MAJOR = %d\n", WLENPwr_MAJOR);
    	return 0;
}

static void __exit WLENPwr_exit(void)
{
	unregister_chrdev(WLENPwr_MAJOR, "WLANP");
}

//===================================================================
module_init(WLENPwr_init);
module_exit(WLENPwr_exit);

MODULE_LICENSE("GPL");

