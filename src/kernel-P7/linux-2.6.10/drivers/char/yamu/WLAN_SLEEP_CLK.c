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
int WLAN_SLEEP_CLK_MAJOR = 231;

//===================================================================
int WLAN_SLEEP_CLK_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPHCON &= ~(0x3<<18));	//GPH 9
	(*(volatile unsigned int __force *)S3C2413_GPHCON |=(0x1<<18));	//GPH 9 Output
	(*(volatile unsigned int __force *)S3C2413_GPHDAT &=~(1<<9));		//GPH 9 High
	(*(volatile unsigned int __force *)S3C2413_GPHDN &=~(1<<9));		//GPH 9 Pull Down disable

   	return 0;          /* success */
}


int WLAN_SLEEP_CLK_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WLAN_SLEEP_CLK_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPHDAT &=~(1<<9));	//GPH 9 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPHDAT |=(1<<9));	//GPH 9 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations WLAN_SLEEP_CLK_fops = {
    	write:    WLAN_SLEEP_CLK_write,
    	open:    WLAN_SLEEP_CLK_open,
    	release:  	WLAN_SLEEP_CLK_release,
};

//===================================================================
static int __init WLAN_SLEEP_CLK_init(void)
{
    	int result;

    	result = register_chrdev(WLAN_SLEEP_CLK_MAJOR, "WLAN_SLEEP_CLK", &WLAN_SLEEP_CLK_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[WLAN_SLEEP_CLK]WLAN_SLEEP_CLK_MAJOR = %d\n", WLAN_SLEEP_CLK_MAJOR);
    	return 0;
}

static void __exit WLAN_SLEEP_CLK_exit(void)
{
	unregister_chrdev(WLAN_SLEEP_CLK_MAJOR, "WLAN_SLEEP_CLK");
}

//===================================================================
module_init(WLAN_SLEEP_CLK_init);
module_exit(WLAN_SLEEP_CLK_exit);

MODULE_LICENSE("GPL");
