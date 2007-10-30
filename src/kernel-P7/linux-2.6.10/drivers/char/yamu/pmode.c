//===================================================================
// 파워모드 컨트롤 드라이버
//===================================================================

#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/delay.h>
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
#include <asm/arch-s3c2413/regs-clock.h>
#include <asm/arch-s3c2413/regs-timer.h>
#include <asm/arch-s3c2413/map.h>
#include <asm/io.h>

extern void SPI_SCSB(int);
extern void SPI_SDA(int);
extern void SPI_SCLK(int);
extern void pcm1770_SEND_CMD(unsigned char, unsigned char);



//===================================================================
int PMODE_MAJOR = 234;

//===================================================================
int pmode_open (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
int pmode_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
int pmode_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
    {
		//---------------------------------------------------------------------------------------
       	case 0x01:
		//---------------------------------------------------------------------------------------
			//All power on
			//(*(volatile unsigned int __force *)S3C2413_PWRMODECON) = S3C2413_POWERMODECON_IDLE;
		//1. Wlan DC En(B10), pwr(G9), nRESET(G15)
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) |= (1<<10);
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) |= (1<<9);			
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) |= (1<<15);
		//2. key backlight - GPB2
			//pwm0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFCF);  	// GPB2 : 2
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000020);  	// GPB2 : 2 - TOUT2
			(*(volatile unsigned int __force *)S3C2413_TCFG0 &= 0x00000000);	// Prescale 0x00
			(*(volatile unsigned int __force *)S3C2413_TCFG1 &= 0xFFFFF0FF);	// PCLK devide 1/2

			(*(volatile unsigned int __force *)S3C2413_TCNTB(2) = 0x100);
			(*(volatile unsigned int __force *)S3C2413_TCMPB(2) = 0x0F);
			(*(volatile unsigned int __force *)S3C2413_TCON &= 0xFFFF0FFF);
			(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00002000);		// Manual update
			(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00009000);		// Start timer2 with Auto reload
			(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00002000));	// Manual update mode disable

		//3. lcd backlight max - GPB0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFFC);  	// GPB0 : 10
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000002);  	// GPB0 : 10 - TOUT0
			(*(volatile unsigned int __force *)S3C2413_TCFG0 &= 0x00000000);	// Prescale 0x00
			(*(volatile unsigned int __force *)S3C2413_TCFG1 &= 0xFFFFFFF0);	// PCLK devide 1/2

			(*(volatile unsigned int __force *)S3C2413_TCNTB(0) = 0x100);
			(*(volatile unsigned int __force *)S3C2413_TCMPB(0) = 0xFF);
			(*(volatile unsigned int __force *)S3C2413_TCON &= 0xFFFFFFF0);
			(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000002);		// Manual update
			(*(volatile unsigned int __force *)S3C2413_TCON |= 0x00000009);		// Start timer0 with Auto reload
			(*(volatile unsigned int __force *)S3C2413_TCON &= ~(0x00000002));	// Manual update mode disable

			//4.lcd backlight enable - GPG6
			(*(volatile unsigned int __force *)S3C2413_GPBDAT |=(1<<6));	//GPG6 high
			break;

		//---------------------------------------------------------------------------------------
		case 0x02:
		//---------------------------------------------------------------------------------------
			//LCD pwr,  backlight off, key backlight off
			//(*(volatile unsigned int __force *)S3C2413_PWRMODECON) = S3C2413_POWERMODECON_STOP;
			
			//1. Wlan DC En(B10), pwr(G9), nRESET(G15)
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) |= (1<<10);
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) |= (1<<9);			
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) |= (1<<15);
			//2. key backlight Off - GPB2
			//pwm0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFCF);  	// GPB2 : 2
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000010);  	// GPB2 : 2 - OUT
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) |= (1<<2);
			//3. lcd backlight min - GPB0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFFC);  	// GPB0 : 10
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000001);  	// GPB0 : 10 - OUT
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) &= ~1;
			//4.lcd backlight disable
			(*(volatile unsigned int __force *)S3C2413_GPBDAT &=~(1<<6));	//GPG6 Low

			break;

		//---------------------------------------------------------------------------------------
		case 0x03:
		//---------------------------------------------------------------------------------------
			//All power off(LCD pwr, DAC pwr, Amp pwr, mute tr control, wlan pwr, lcd backlight, key backlight)
			//(*(volatile unsigned int __force *)S3C2413_PWRMODECON) = S3C2413_POWERMODECON_SLEEP;
			//1. Wlan DC En(B10), pwr(G9), nRESET(G15)
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) &= ~(1<<10);
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) &= ~(1<<9);			
			(*(volatile unsigned int __force *)S3C2413_GPGDAT) &= ~(1<<15);
			//2. key backlight Off - GPB2
			//pwm0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFCF);  	// GPB2 : 2
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000010);  	// GPB2 : 2 - OUT
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) |= (1<<2);
			//3. lcd backlight min - GPB0
			(*(volatile unsigned int __force *)S3C2413_GPBCON &= 0xFFFFFFFC);  	// GPB0 : 10
			(*(volatile unsigned int __force *)S3C2413_GPBCON |= 0x00000001);  	// GPB0 : 10 - OUT
			(*(volatile unsigned int __force *)S3C2413_GPBDAT) &= ~1;
			//4.lcd backlight disable
			(*(volatile unsigned int __force *)S3C2413_GPBDAT &= ~(1<<6));	//GPG6 Low
			break;
	}

	return cmd;
}

//===================================================================
struct file_operations pmode_fops = {
    	ioctl:    	pmode_ioctl,
    	open:    	pmode_open,
    	release:  	pmode_release,
};

//===================================================================
static int __init pmode_init(void)
{
    	int result;

    	result = register_chrdev(PMODE_MAJOR, "PMODE", &pmode_fops);
    	if (result < 0) {
        	printk(KERN_WARNING " can't get major \n");
        	return result;
    	}
    	//printk("PMODE_MAJOR = %d\n", PMODE_MAJOR);
    	return 0;
}

static void __exit pmode_exit(void)
{
	unregister_chrdev(PMODE_MAJOR, "PMODE");
}

//===================================================================
module_init(pmode_init);
module_exit(pmode_exit);

MODULE_LICENSE("GPL");

