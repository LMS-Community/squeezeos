//===================================================================
// 배터리 잔량 체크 드라이버
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

#include <asm/arch-s3c2413/regs-adc.h>
#include <asm/arch-s3c2413/map.h>
#include <asm/delay.h>
#include <asm/io.h>

//===================================================================
unsigned char BATTERY_MAJOR = 247;

//===================================================================
int battery_open (struct inode *inode, struct file *filp)
{
	return 0;
}

int battery_release (struct inode *inode, struct file *filp)
{
	return 0;
}

//===================================================================
ssize_t battery_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned int i, max, min, maxindex, minindex, temp[5];
	unsigned int sum = 0x00;
	char data[1];
		
	for(i=0; i<5; i++) {
		(*(volatile unsigned int __force *)S3C2413_ADCCON) =S3C2413_ADCCON_PRSCEN | S3C2413_ADCCON_PRSCVL(0x70)|S3C2413_ADCCON_SELMUX(0x02);
		udelay(10);
		(*(volatile unsigned int __force *)S3C2413_ADCTSC) &= 0xfb;
		(*(volatile unsigned int __force *)S3C2413_ADCCON) |= S3C2413_ADCCON_ENABLE_START;
		
	    	while((*(volatile unsigned int __force *)S3C2413_ADCCON) & S3C2413_ADCCON_ENABLE_START);	//check if start bit is low
			while(!((*(volatile unsigned int __force *)S3C2413_ADCCON) & S3C2413_ADCCON_ECFLG));        	//check if EC(End of Conversion) flag is high
	    	temp[i] = (((*(volatile unsigned int __force*)S3C2413_ADCDAT0)&0x3ff) >> 2);
		udelay(10);
	}

	//
	// 5회 측정후 중간 세개의 값을 평균내서 출력
	//
	max = temp[0]; maxindex = 0;
	min = temp[0]; minindex = 0;
	
	for(i=0; i<5; i++) {
		if (temp[i] > max) {max = temp[i]; maxindex = i;}
		if (temp[i] < min) {min = temp[i]; minindex = i;}
	}

	for(i=0;i<5; i++) 
		if (i != maxindex && i != minindex) sum += temp[i];
	
	if (maxindex == minindex) data[0] = (unsigned char) (sum/4);
	else data[0] = (unsigned char) (sum/3);
	
	copy_to_user(buf, data, count); 
	(*(volatile unsigned int __force *)S3C2413_ADCCON) &= ~S3C2413_ADCCON_PRSCEN;
	return(count);					
}

//===================================================================
struct file_operations battery_fops = {
	read:     	battery_read,
	open:    	battery_open,
	release:  	battery_release,
};

//===================================================================
static int __init battery_init(void)
{
	int result;

  	result = register_chrdev(BATTERY_MAJOR, "BATTERY", &battery_fops);
  	if (result < 0) {
   		printk(KERN_WARNING " can't get major \n");
   		return result;
  	}

  	printk("BATTERY_MAJOR = %d\n", BATTERY_MAJOR);
  	return 0;
}

static void __exit battery_exit(void)
{
	unregister_chrdev(BATTERY_MAJOR, "BATTERY");
}

//===================================================================

module_init(battery_init);
module_exit(battery_exit);

MODULE_LICENSE("GPL");

