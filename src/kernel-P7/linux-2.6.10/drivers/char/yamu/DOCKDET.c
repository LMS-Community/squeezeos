//===================================================================
// DOCK DETECTION 드라이버
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
int DOCKDET_MAJOR = 223;

//===================================================================
int DOCKDET_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFF3FFF);	//GPG7
	(*(volatile unsigned int __force *)S3C2413_GPGCON |= 0x00000000);	//GPG7  input
	(*(volatile unsigned int __force *)S3C2413_GPGDN &= ~(1<<7));		//GPG7 Pull Down Enable
   	return 0;          /* success */
}

int DOCKDET_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t DOCKDET_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPGDAT);
	data[0] = ((temp  & 0x80)>>7);	//GPG7

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations DOCKDET_fops = {
    	read:    	DOCKDET_read,
    	open:    	DOCKDET_open,
    	release:  	DOCKDET_release,
};

//===================================================================
static int __init DOCKDET_init(void)
{
    	int result;

    	result = register_chrdev(DOCKDET_MAJOR, "DOCKDET", &DOCKDET_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}
   /* if(SKELETON_MAJOR == 0)
		SKELETON_MAJOR = result;*/

    	printk("[DOCKDET]DOCKDET_MAJOR = %d\n", DOCKDET_MAJOR);
    	return 0;
}

static void __exit DOCKDET_exit(void)
{
	unregister_chrdev(DOCKDET_MAJOR, "DOCKDET");

}

//===================================================================
module_init(DOCKDET_init);
module_exit(DOCKDET_exit);

MODULE_LICENSE("GPL");
