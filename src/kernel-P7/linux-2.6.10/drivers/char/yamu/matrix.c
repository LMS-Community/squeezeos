//===================================================================
// 매트릭스 키 드라이버
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
unsigned char MATRIX_MAJOR = 250;

//===================================================================
unsigned char key_code = 0, key_temp = 0, chat_count = 0;
static struct timer_list matrix_timer_list;

//===================================================================
void matrix_timer_func(unsigned long ptr);

//===================================================================
int matrix_open (struct inode *inode, struct file *filp)
{
	init_timer(&matrix_timer_list);
	matrix_timer_list.function = &matrix_timer_func;
	matrix_timer_list.expires = jiffies + 1; /* 10ms */
	add_timer(&matrix_timer_list);

	__raw_writel(0x0015, S3C2413_GPFCON);		//GF3 input : 2007.04.18
	__raw_writel(0x0F, S3C2413_GPFDN);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &=0x00);
	return 0;          /* success */
}

//===================================================================
int matrix_release (struct inode *inode, struct file *filp)
{
	return 0;
}

//===================================================================
ssize_t matrix_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char data[1];
	data[0] = key_code;

	copy_to_user(buf, data, count);
	return(count);
}

//===================================================================
struct file_operations matrix_fops = {
	read:     	matrix_read,
	open:    	matrix_open,
	release:  	matrix_release,
};

//===================================================================
static int __init matrix_init(void)
{
	int result;

  	result = register_chrdev(MATRIX_MAJOR, "MATRIX", &matrix_fops);
  	if (result < 0) {
   		printk(KERN_WARNING " can't get major \n");
   		return result;
  	}
  	printk("MATRIX_MAJOR = %d\n", MATRIX_MAJOR);
  	return 0;
}

static void __exit matrix_exit(void)
{
	unregister_chrdev(MATRIX_MAJOR, "MATRIX");
}

//===================================================================
void matrix_timer_func(unsigned long ptr)
{
	unsigned char key_count = 0;
	unsigned char line_value[5], temp = 0;

	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x0F);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xFE);
	line_value[0] = __raw_readl(S3C2413_GPFDAT);
	line_value[0] = (line_value[0]  & 0x70) | 0x01;

	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x0F);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xFD);
	line_value[1] = __raw_readl(S3C2413_GPFDAT);
	line_value[1] = (line_value[1]  & 0x70) | 0x02;

	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x0F);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xFB);
	line_value[2] = __raw_readl(S3C2413_GPFDAT);
	line_value[2] = (line_value[2]  & 0x70) | 0x04;

	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x0F);
	line_value[3] = __raw_readl(S3C2413_GPFDAT);
	line_value[3] = (line_value[3]  & 0x80) | 0x08;

	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x07);
	line_value[4] = __raw_readl(S3C2413_GPFDAT);
	line_value[4] = (line_value[4]  & 0x08);

	(*(volatile unsigned int __force *)S3C2413_GPFDAT  |= 0x0F);

	switch (line_value[0]){	case 0x11: temp = 0x11; key_count++; break;
						case 0x21: temp = 0x12; key_count++; break;
						case 0x41: temp = 0x13; key_count++; break;
						}
	switch (line_value[1]){	case 0x12: temp = 0x21; key_count++; break;
						case 0x22: temp = 0x22; key_count++; break;
						case 0x42: temp = 0x23; key_count++; break;
						}

	switch (line_value[2]){	case 0x14: temp = 0x31; key_count++; break;
						case 0x24: temp = 0x32; key_count++; break;
						case 0x44: temp = 0x33; key_count++; break;
						}

	switch (line_value[3]){	case 0x88: temp = 0x88; key_count++; break;
						}

	switch (line_value[4]){	case 0: temp = 0xAA; key_count++; break;
						}

	if (key_count != 1) temp = 0x00;
	key_count = 0;

	if (chat_count == 4) chat_count = 0;

	if (chat_count == 0) {key_temp = temp; chat_count++;}
	else if (chat_count == 3) {key_code = key_temp; chat_count = 0;}
	else if (key_temp != temp) {key_temp = 0x00; chat_count = 0;}
	else chat_count++;

	init_timer(&matrix_timer_list);
	matrix_timer_list.function = &matrix_timer_func;
	matrix_timer_list.expires = jiffies + 1; /* 10ms */
	add_timer(&matrix_timer_list);
}

//===================================================================
module_init(matrix_init);
module_exit(matrix_exit);

MODULE_LICENSE("GPL");
