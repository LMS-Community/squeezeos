//===================================================================
//IR 송신 드라이버
//===================================================================

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/clk.h>
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
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>

#include <asm/mach-types.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/delay.h>

#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-timer.h>
#include <asm/arch/map.h>


#define JIVE_IRTX_VERSION "1.0.0"
#define JIVE_IRTX_MODULE_NAME "irtx"
#define JIVE_IRTX_MISCDEV_MINOR MISC_DYNAMIC_MINOR
#define JIVE_IRTX_NAME   JIVE_MGMT_MODULE_NAME " hardware driver " JIVE_IRTX_VERSION
#define PFX JIVE_MGMT_MODULE_NAME ": "

#define RAW(var) (*(volatile unsigned int __force *)var)


//===================================================================
#define CarrierNothing	0
#define CarrierProcess	1

//===================================================================
static inline void DisableCarrierNothing(void);
static inline void EnableCarrierProcess(int v1, int v2);
static inline void EnableNoCarrier(void);


//===================================================================
static int IRTransmission_open (struct inode *inode, struct file *filp)
{	
   	return 0;
}

static int IRTransmission_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
static ssize_t IRTransmission_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	#define BUFFER_MAX		100
	#define NO_CARRIER_DELAY_MAX	20		// 20uS
	#define MARK_BIT		0x80000000
	#define OMIT_CARRIER_BIT	0x40000000

	#define CLOCK_FREQUENCY		12000000	//  12MHz
	#define CARRIER_FREQUENCY_MIN	20000		//  20kHz
	#define CARRIER_FREQUENCY_MAX	500000		// 500kHz

	int i;
	unsigned int TxData[100];
	unsigned long flags;
	int ret;
	unsigned int MarkBit;
	unsigned int OmitCarrierBit;
	unsigned int DelayTime;
	unsigned int CarrierFrequency;
	int v1, v2;

//	printk(KERN_WARNING "***** count: %d\n", count);

	// Sanity check
	if( count > BUFFER_MAX) {
		count = BUFFER_MAX;
	}

	ret = copy_from_user(TxData, buf, count*4);
	if (ret < 0) {
		return ret;
	}

	// First value in array is carrier frequency in Hz
	CarrierFrequency = TxData[0];
	// Sanity checks
	if( CarrierFrequency < CARRIER_FREQUENCY_MIN) {
		CarrierFrequency = CARRIER_FREQUENCY_MIN;
	}
	if( CarrierFrequency > CARRIER_FREQUENCY_MAX) {
		CarrierFrequency = CARRIER_FREQUENCY_MAX;
	}
	// Calculate timer values
	v1 = CLOCK_FREQUENCY / CarrierFrequency;
	v2 = v1 / 2;

	DisableCarrierNothing();

	local_irq_save(flags);

	for(i=1; i<count; i++) {
		MarkBit = TxData[i] & MARK_BIT;
		OmitCarrierBit = TxData[i] & OMIT_CARRIER_BIT;
		DelayTime = TxData[i] & ~( MARK_BIT | OMIT_CARRIER_BIT);

		// Mark
		if( MarkBit) {
			// Mark - omit carrier
			if( OmitCarrierBit) {
				// Limit to 20uS
				if( DelayTime > NO_CARRIER_DELAY_MAX) {
					DelayTime = NO_CARRIER_DELAY_MAX;
				}
				EnableNoCarrier();
			// Mark - w/ carrier
			} else {
				// Pass values for carrier frequency
				EnableCarrierProcess( v1, v2);
			}
		// Space
		} else {
			DisableCarrierNothing();
		}

//		printk(KERN_WARNING "***** TxData[%d]: %x\n", TxBitCnt, DelayTime);
		mdelay( DelayTime / 1000);
		udelay( DelayTime % 1000);
	}

	DisableCarrierNothing();

	local_irq_restore(flags);
	return 0;
}

//===================================================================
struct file_operations IRTransmission_fops = {
    	write:    	IRTransmission_write,
    	open:    	IRTransmission_open,
    	release:  	IRTransmission_release,
};

static struct miscdevice jive_irtx_miscdev = {
	JIVE_IRTX_MISCDEV_MINOR,
	JIVE_IRTX_MODULE_NAME,
	&IRTransmission_fops,
};

//===================================================================
static int __init IRTransmission_init(void)
{
	int result;

	/* For compatibility with the old bootloader allow s3c2413 machines
	 * to work here too. We can't use machine_is_s3c2413() here as that
	 * machine is not configured in the kernel.
	 *
	 * The S3C2413 hack should be removed when the old bootloader has
	 * been phased out.
	 */
	if (!(machine_is_jive() || machine_arch_type == MACH_TYPE_S3C2413))
		return 0;

	result = misc_register (&jive_irtx_miscdev);
	if (result) {
		printk(KERN_WARNING "misc device register failed\n");
		return result;
	}
    	return 0;
}

static void __exit IRTransmission_exit(void)
{
	misc_deregister(&jive_irtx_miscdev);
}

//===================================================================
static inline void DisableCarrierNothing(void)
{
	unsigned long flags;

	local_irq_save(flags);

	RAW(S3C2410_TCON) &= 0xFFFFF0FF;		

	local_irq_restore(flags);

	s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_OUTP);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
}				

static inline void EnableCarrierProcess(int v1, int v2)
{
	unsigned long flags;

	s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_TOUT1);

	local_irq_save(flags);

	RAW(S3C2410_TCFG0) &= ~S3C2410_TCFG_PRESCALER0_MASK;	// Prescale 0x00 for Timer 0 and 1
	RAW(S3C2410_TCFG1) &= ~S3C2410_TCFG1_MUX1_MASK;
	RAW(S3C2410_TCFG1) |= S3C2410_TCFG1_MUX1_DIV4;		// PCLK devide 1/4

	RAW(S3C2410_TCNTB(1)) = v1;				// 37.8Khz
	RAW(S3C2410_TCMPB(1)) = v2;

	RAW(S3C2410_TCON) &= 0xFFFFF0FF;
	RAW(S3C2410_TCON) |= S3C2410_TCON_T1MANUALUPD;		// Manual update
	RAW(S3C2410_TCON) |= S3C2410_TCON_T1RELOAD | S3C2410_TCON_T1START; // Start timer1 with Auto reload
	RAW(S3C2410_TCON) &= ~S3C2410_TCON_T1MANUALUPD;		// Manual update mode disable	

	local_irq_restore(flags);
}

static inline void EnableNoCarrier(void)
{
	unsigned long flags;

	local_irq_save(flags);

	RAW(S3C2410_TCON) &= 0xFFFFF0FF;		

	local_irq_restore(flags);

	s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_OUTP);
	s3c2410_gpio_setpin(S3C2410_GPB1, 1);
}
//===================================================================
module_init(IRTransmission_init);
module_exit(IRTransmission_exit);

MODULE_LICENSE("GPL");


