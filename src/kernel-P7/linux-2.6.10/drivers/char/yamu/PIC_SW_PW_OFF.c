//===================================================================
// LCD reset 컨트롤 드라이버
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
extern void Init_LDI(void);

//===================================================================
int SW_PW_OFF_MAJOR = 236;

//===================================================================
int SW_PW_OFF_open (struct inode *inode, struct file *filp)
{
	(*(volatile unsigned int __force *)S3C2413_GPCCON &= ~(0x3<<10));	//GPC 5
	(*(volatile unsigned int __force *)S3C2413_GPCCON |= (0x1<<10));	//GPC 5 Output
	(*(volatile unsigned int __force *)S3C2413_GPCDAT |=(1<<5));		//GPC 5 High
	(*(volatile unsigned int __force *)S3C2413_GPCDN &=~(1<<5));		//GPC 5 Pull Down disable

   	return 0;          /* success */
}


int SW_PW_OFF_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t SW_PW_OFF_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:		// off
			(*(volatile unsigned int __force *)S3C2413_GPCDAT &=~(1<<5));	//GPC 5 Low
			break;
		case 0x01:		// on
			(*(volatile unsigned int __force *)S3C2413_GPCDAT |=(1<<5));	//GPC 5 High
			Init_LDI();
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations SW_PW_OFF_fops = {
    	write:    SW_PW_OFF_write,
    	open:    	SW_PW_OFF_open,
    	release:  	SW_PW_OFF_release,
};

//===================================================================
static int __init SW_PW_OFF_init(void)
{
    	int result;

    	result = register_chrdev(SW_PW_OFF_MAJOR, "SW_PW_OFF", &SW_PW_OFF_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[SW_PW_OFF]SW_PW_OFF_MAJOR = %d\n", SW_PW_OFF_MAJOR);
    	return 0;
}

static void __exit SW_PW_OFF_exit(void)
{
	unregister_chrdev(SW_PW_OFF_MAJOR, "SW_PW_OFF");
}

//===================================================================
module_init(SW_PW_OFF_init);
module_exit(SW_PW_OFF_exit);

MODULE_LICENSE("GPL");
