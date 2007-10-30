//===================================================================
// WLAN nRESET 드라이버
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
int WLAN_nRESET_MAJOR = 220;

//===================================================================
int WLAN_nRESET_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPGCON &= ~(0x3<<30));	//GPG 15
	(*(volatile unsigned int __force *)S3C2413_GPGCON |=(0x1<<30));	//GPG15 Output
	(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<15));		//GPG15 High
	(*(volatile unsigned int __force *)S3C2413_GPGDN &=~(1<<15));		//GPG15 Pull Down disable

   	return 0;          /* success */
}


int WLAN_nRESET_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WLAN_nRESET_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPGDAT &=~(1<<15));	//GPG15 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPGDAT |=(1<<15));	//GPG15 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations WLAN_nRESET_fops = {
    	write:    WLAN_nRESET_write,
    	open:    WLAN_nRESET_open,
    	release:  	WLAN_nRESET_release,
};

//===================================================================
static int __init WLAN_nRESET_init(void)
{
    	int result;

    	result = register_chrdev(WLAN_nRESET_MAJOR, "WLAN_nRESET", &WLAN_nRESET_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[WLAN_nRESET]WLAN_nRESET_MAJOR = %d\n", WLAN_nRESET_MAJOR);
    	return 0;
}

static void __exit WLAN_nRESET_exit(void)
{
	unregister_chrdev(WLAN_nRESET_MAJOR, "WLAN_nRESET");
}

//===================================================================
module_init(WLAN_nRESET_init);
module_exit(WLAN_nRESET_exit);

MODULE_LICENSE("GPL");
