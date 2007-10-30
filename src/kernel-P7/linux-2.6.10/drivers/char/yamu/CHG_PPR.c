//===================================================================
// Charger(LTC)  PPR 드라이버
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
int CHG_PPR_MAJOR = 225;

//===================================================================
int CHG_PPR_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPHCON &=0xFFFF3FFF);	//GPH7
	(*(volatile unsigned int __force *)S3C2413_GPHCON |= 0x00000000);	//GPH7  input
	(*(volatile unsigned int __force *)S3C2413_GPHDN &= ~(1<<7));		//GPH7 Pull Down Enable
   	return 0;          /* success */
}

int CHG_PPR_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t CHG_PPR_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPHDAT);
	data[0] = ((temp  & 0x80)>>7);	//GPH7

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations CHG_PPR_fops = {
    	read:    	CHG_PPR_read,
    	open:    	CHG_PPR_open,
    	release:  	CHG_PPR_release,
};

//===================================================================
static int __init CHG_PPR_init(void)
{
    	int result;

    	result = register_chrdev(CHG_PPR_MAJOR, "CHG_PPR", &CHG_PPR_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}
   /* if(SKELETON_MAJOR == 0)
		SKELETON_MAJOR = result;*/

    	printk("[CHG_PPR]CHG_PPR_MAJOR = %d\n", CHG_PPR_MAJOR);
    	return 0;
}

static void __exit CHG_PPR_exit(void)
{
	unregister_chrdev(CHG_PPR_MAJOR, "CHG_PPR");

}

//===================================================================
module_init(CHG_PPR_init);
module_exit(CHG_PPR_exit);

MODULE_LICENSE("GPL");
