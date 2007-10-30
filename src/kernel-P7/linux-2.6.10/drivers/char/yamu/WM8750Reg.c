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
#include <linux/l3/WM8750.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>

#include <asm/arch-s3c2413/regs-gpio.h>
#include <asm/arch-s3c2413/map.h>
#include <asm/io.h>

//===================================================================
int WM8750_REG_MAJOR = 228;

//===================================================================
int WM8750Reg_open (struct inode *inode, struct file *filp)
{
   	return 0;          /* success */
}


int WM8750Reg_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t WM8750Reg_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[2];

	copy_from_user(data, buf, count);

	WM8750_SEND_CMD(data[0],data[1]);

	return(count);
}

//===================================================================
struct file_operations WM8750Reg_fops = {
    	write:    	WM8750Reg_write,
    	open:    	WM8750Reg_open,
    	release:  	WM8750Reg_release,
};

//===================================================================
static int __init WM8750Reg_init(void)
{
	int result;

	result = register_chrdev(WM8750_REG_MAJOR, "WM8750_REG", &WM8750Reg_fops);
	if (result < 0)
	{
		printk(KERN_WARNING " can't get major \n");
		return result;
	}

	printk("[WM8750_REG]WM8750_REG_MAJOR = %d\n", WM8750_REG_MAJOR);
	return 0;
}

static void __exit WM8750Reg_exit(void)
{
	unregister_chrdev(WM8750_REG_MAJOR, "WM8750_REG");
}

//===================================================================
module_init(WM8750Reg_init);
module_exit(WM8750Reg_exit);

MODULE_LICENSE("GPL");

