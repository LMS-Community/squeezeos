//===================================================================
// LCD 파워 컨트롤 드라이버
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
int NANDWP_MAJOR = 229;

//===================================================================
int NANDWP_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPCCON &=0xFFFFCFFF);	//GPC 6
	(*(volatile unsigned int __force *)S3C2413_GPCCON |=0x00001000);	//GPC6 Output
	(*(volatile unsigned int __force *)S3C2413_GPCDAT |=(1<<6));		//GPC6 High
	(*(volatile unsigned int __force *)S3C2413_GPCDN &=~(1<<6));		//GPC6 Pull Down disable	

   	return 0;          /* success */
}


int NANDWP_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t NANDWP_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:		// nWP ACTIVE
			(*(volatile unsigned int __force *)S3C2413_GPCDAT &=~(1<<6));	//GPC6 Low
			break;
		case 0x01:		// nWP Disable
			(*(volatile unsigned int __force *)S3C2413_GPCDAT |=(1<<6));	//GPC6 High
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations NANDWP_fops = {
    	write:    NANDWP_write,
    	open:    NANDWP_open,
    	release:  	NANDWP_release,
};

//===================================================================
static int __init NANDWP_init(void)
{
    	int result;

    	result = register_chrdev(NANDWP_MAJOR, "NANDWP", &NANDWP_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[NANDWP]NANDWP_MAJOR = %d\n", NANDWP_MAJOR);
    	return 0;
}

static void __exit NANDWP_exit(void)
{
	unregister_chrdev(NANDWP_MAJOR, "NANDWP");
}

//===================================================================
module_init(NANDWP_init);
module_exit(NANDWP_exit);

MODULE_LICENSE("GPL");
