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
int WLAN_DC_EN_MAJOR = 221;

//===================================================================
int WLAN_DC_EN_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPBCON &= ~(0x3<<20));	//GPB 10
	(*(volatile unsigned int __force *)S3C2413_GPBCON |= (0x1<<20));		//GPB 10 Output
	(*(volatile unsigned int __force *)S3C2413_GPBDAT |= (1<<10));		//GPB 10 Low
	(*(volatile unsigned int __force *)S3C2413_GPBDN &=~(1<<10));			//GPB 10 Pull Down disable

   	return 0;          /* success */
}


int WLAN_DC_EN_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WLAN_DC_EN_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:
			(*(volatile unsigned int __force *)S3C2413_GPBDAT &=~(1<<10));	//GPB 10 Low
			break;
		case 0x01:
			(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<10));	//GPB 10 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations WLAN_DC_EN_fops = {
    	write:    WLAN_DC_EN_write,
    	open:    WLAN_DC_EN_open,
    	release:  	WLAN_DC_EN_release,
};

//===================================================================
static int __init WLAN_DC_EN_init(void)
{
    	int result;

    	result = register_chrdev(WLAN_DC_EN_MAJOR, "WLAN_DC_EN", &WLAN_DC_EN_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[WLAN_DC_EN]WLAN_DC_EN_MAJOR = %d\n", WLAN_DC_EN_MAJOR);
    	return 0;
}

static void __exit WLAN_DC_EN_exit(void)
{
	unregister_chrdev(WLAN_DC_EN_MAJOR, "WLAN_DC_EN");
}

//===================================================================
module_init(WLAN_DC_EN_init);
module_exit(WLAN_DC_EN_exit);

MODULE_LICENSE("GPL");
