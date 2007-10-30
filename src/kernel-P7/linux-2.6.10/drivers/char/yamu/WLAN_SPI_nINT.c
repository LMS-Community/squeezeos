//===================================================================
// WLAN SPI_nINT 드라이버
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
int WLAN_SPI_nINT_MAJOR = 219;

//===================================================================
int WLAN_SPI_nINT_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &=0xFFFFFFFC);	//GPG0
	(*(volatile unsigned int __force *)S3C2413_GPGCON |= 0x00000000);	//GPG0  input
	(*(volatile unsigned int __force *)S3C2413_GPGDN &= ~(1<<0));		//GPG0 Pull Down Enable
   	return 0;          /* success */
}

int WLAN_SPI_nINT_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WLAN_SPI_nINT_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];
	unsigned short temp;

	temp = __raw_readl(S3C2413_GPGDAT);
	
	data[0] = (temp  & 0x1);	//GPG0
	copy_to_user(buf, data, count); 
	return(count);					
}

//===================================================================
struct file_operations WLAN_SPI_nINT_fops = {
    	read:    	WLAN_SPI_nINT_read,
    	open:    	WLAN_SPI_nINT_open,
    	release:  	WLAN_SPI_nINT_release,
};

//===================================================================
static int __init WLAN_SPI_nINT_init(void)
{
    	int result;

    	result = register_chrdev(WLAN_SPI_nINT_MAJOR, "WLAN_SPI_nINT", &WLAN_SPI_nINT_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}
   /* if(SKELETON_MAJOR == 0)
		SKELETON_MAJOR = result;*/

    	printk("[WLAN_SPI_nINT]WLAN_SPI_nINT_MAJOR = %d\n", WLAN_SPI_nINT_MAJOR);
    	return 0;
}

static void __exit WLAN_SPI_nINT_exit(void)
{
	unregister_chrdev(WLAN_SPI_nINT_MAJOR, "WLAN_SPI_nINT");
		
}

//===================================================================
module_init(WLAN_SPI_nINT_init);
module_exit(WLAN_SPI_nINT_exit);

MODULE_LICENSE("GPL");
