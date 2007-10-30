/*
	Hardware driver for Slim Devices JIVE
	(c) Copyright 2006 Slim Devices Inc <richard@slimdevices.com>

	----------------------------------------------------------
	This software may be used and distributed according to the terms
        of the GNU General Public License, incorporated herein by reference.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/pm.h>
#include <linux/clk.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-clock.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-timer.h>
#include <asm/arch/regs-irq.h>
#include <asm/arch/regs-dsc.h>
#include <asm/arch/regs-power.h>
//#include <asm/arch/regs-ssmc.h>

#include <asm/arch/map.h>

#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>

#define IRQF_TRIGGER_BOTH	(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

static struct clk *adc_clock;

static void __iomem *adc_regs;

static struct input_dev *bsp_dev;

struct timer_list acpr_timer;


extern void Init_LDI(void);
extern void LCD_CMD(unsigned short address, unsigned short data);

/* gone from input.h */
#define SW_0	(0)
#define SW_1	(1)
#define SW_2	(2)
#define SW_3	(3)


#define JIVE_MGMT_VERSION "1.0.0"
#define JIVE_MGMT_MODULE_NAME "jive_mgmt"
#define JIVE_MGMT_MISCDEV_MINOR MISC_DYNAMIC_MINOR
#define JIVE_MGMT_NAME   JIVE_MGMT_MODULE_NAME " hardware driver " JIVE_MGMT_VERSION
#define PFX JIVE_MGMT_MODULE_NAME ": "

#define JIVE_MGMT_MAGIC 'P'


#define JIVE_MGMT_IOCSLCDBACKLIGHT 	11
#define JIVE_MGMT_IOCGLCDBACKLIGHT 	12
#define JIVE_MGMT_IOCSLED		13
#define JIVE_MGMT_IOCGLED 		14
#define JIVE_MGMT_IOCGBATTERY 		17
#define JIVE_MGMT_IOCGPHONEDETECT	18

#define JIVE_MGMT_IOCSSWPWOFF		21
#define JIVE_MGMT_IOCSCHGSUSP		22
#define JIVE_MGMT_IOCGCHGACPR		23
#define JIVE_MGMT_IOCGUSBDETECT		24
#define JIVE_MGMT_IOCGCHGPPR		25
#define JIVE_MGMT_IOCSCHGHPWR		26
#define JIVE_MGMT_IOCGDOCKDETECT	27
#define JIVE_MGMT_IOCSWM8750		28
#define JIVE_MGMT_IOCSLCD		29

#define JIVE_MGMT_IOCGCLKCON		40	// printk current clock
#define JIVE_MGMT_IOCSCLKCONENABLE     	41	// enable clock X
#define JIVE_MGMT_IOCSCLKCONDISABLE    	42	// disable clock X
#define JIVE_MGMT_IOCGARMDIV		43	// get ARMDIV
#define JIVE_MGMT_IOCSARMDIV		44	// set ARMDIV
#define JIVE_MGMT_IOCGDSC0		45	// get DSC0 register
#define JIVE_MGMT_IOCGDSC1		46	// get DSC1 register


/*
 * Input subsystem switches:
 *
 * ACPOWER	=> SW0
 * HEADPHONE	=> SW1
 * DOCTDETECT	=> SW2
 * USBDETECT	=> SW3
 */



// When changing the pwm value, the led stays on for 1 full timer period.
// If this timer period is MAX_DIMMER the light on can be seen. If the
// period is 0 the light is off.
#define MAX_DIMMER 0xFFFF


// Number of battery samples
#define N_BATTERY_SAMPLES 6


#define RAW(var) (*(volatile unsigned int __force *)var)


static int get_battery(void) {
	unsigned int i, max, min;
	unsigned int sum = 0x00;
	unsigned int adc_value;
	unsigned long con;

	con = readl(adc_regs + S3C2410_ADCCON);

	// take adc out of standby mode
	con &= ~S3C2410_ADCCON_STDBM;
	con |= S3C2410_ADCCON_SELMUX(0x02);
	con |= S3C2410_ADCCON_PRSCVL(0x70);
	writel(con, adc_regs + S3C2410_ADCCON);

	// enable pre-scaler
	con |= S3C2410_ADCCON_PRSCEN;
	writel(con, adc_regs + S3C2410_ADCCON);

	min = 0xffff;
	max = 0x0000;
	for(i=0; i<N_BATTERY_SAMPLES; i++) {
		con = readl(adc_regs + S3C2410_ADCCON);

		// start adc
		con |= S3C2410_ADCCON_ENABLE_START;
		writel(con, adc_regs + S3C2410_ADCCON);

		// wait for start bit is low
		while(readl(adc_regs + S3C2410_ADCCON) & S3C2410_ADCCON_ENABLE_START);

		// wait for EC(End of Conversion)
		while(!readl(adc_regs + S3C2410_ADCCON) & S3C2410_ADCCON_ECFLG);

		// read value
		adc_value = readl(adc_regs + S3C2410_ADCDAT0) & 0x3ff;

		sum += adc_value;
		if (adc_value < min)
			min = adc_value;
		if (adc_value > max)
			max = adc_value;
	}

	con = readl(adc_regs + S3C2410_ADCCON);
	con |= S3C2410_ADCCON_STDBM;
	con &= ~S3C2410_ADCCON_PRSCEN;
	writel(con, adc_regs + S3C2410_ADCCON);

	// throw out the max and min.
	sum -= max;
	sum -= min;
	sum = sum / (N_BATTERY_SAMPLES-2);

	return sum;
}


int slide_pwm_value(int _value)
{
	if (_value == 0)
		_value = MAX_DIMMER;
	else if (_value > MAX_DIMMER)
		_value = MAX_DIMMER-1;
	else _value--;
	return _value;
}

static void init_pwm(int pin, int val) {

	if (pin==0) {
	  //RAW(S3C2410_GPBCON) &= ~(S3C2410_GPB0_TOUT0|S3C2410_GPB0_OUTP);
	  //RAW(S3C2410_GPBCON) |= S3C2410_GPB0_TOUT0;			     // GPB0 - TOUT0
		RAW(S3C2410_TCFG0) &= ~S3C2410_TCFG_PRESCALER0_MASK;		     // Prescale 0x00 for Timer 0 and 1
		RAW(S3C2410_TCFG1) &= ~S3C2410_TCFG1_MUX0_MASK;			     // PCLK divide 1/2
		RAW(S3C2410_TCFG1) |= S3C2410_TCFG1_MUX0_DIV2;

		RAW(S3C2410_TCON) |= S3C2410_TCON_T0MANUALUPD;                       // Manual update
		RAW(S3C2410_TCNTB(0)) = MAX_DIMMER;
		RAW(S3C2410_TCMPB(0)) = slide_pwm_value(val);
		RAW(S3C2410_TCON) |= (S3C2410_TCON_T0RELOAD | S3C2410_TCON_T0START); // Start timer 0 with Auto reload
		RAW(S3C2410_TCON) &= ~S3C2410_TCON_T0MANUALUPD;                      // Manual update mode disable
	}
	else if (pin==2) {
		RAW(S3C2410_GPBCON) &= ~(S3C2410_GPB2_TOUT2|S3C2410_GPB2_OUTP);
		RAW(S3C2410_GPBCON) |= S3C2410_GPB2_TOUT2;                           // GPB2 - TOUT2
// Following values are set in arch/arm/mach-s3c2410/time.c
// DO NOT CHANGE
// 		RAW(S3C2410_TCFG0) &= ~S3C2410_TCFG_PRESCALER1_MASK;
// 		RAW(S3C2410_TCFG0) |= (0x0F<<S3C2410_TCFG_PRESCALER1_SHIFT);	     // Prescale for Timer 2, 3 and 4
		RAW(S3C2410_TCFG1) &= ~S3C2410_TCFG1_MUX2_MASK;                      // PCLK divide 1/2
		RAW(S3C2410_TCFG1) |= S3C2410_TCFG1_MUX2_DIV2;

		RAW(S3C2410_TCON) |= S3C2410_TCON_T2MANUALUPD;	                     // Manual update
		RAW(S3C2410_TCON) |= S3C2410_TCON_T2INVERT;	                     // Invert output

		RAW(S3C2410_TCNTB(2)) = MAX_DIMMER;		 
		RAW(S3C2410_TCMPB(2)) = slide_pwm_value(val);
		RAW(S3C2410_TCON) |= (S3C2410_TCON_T2RELOAD | S3C2410_TCON_T2START); // Start timer 2 with Auto reload
		RAW(S3C2410_TCON) &= ~S3C2410_TCON_T2MANUALUPD;	                     // Manual update mode disable
	}
}

static void set_pwm(int pin, int val) {
	RAW(S3C2410_TCMPB(pin)) = slide_pwm_value(val);
}

static int get_pwm(int pin) {
	return __raw_readl(S3C2410_TCMPB(pin));
}



static int jive_mgmt_open(struct inode *inode, struct file *filp) {
	return 0;
}

static int jive_mgmt_release(struct inode *inode, struct file *filp) {
	return 0;
}

static int jive_mgmt_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

	int val;
	get_user(val, (unsigned int __user *)arg);

	switch (cmd) {

	case JIVE_MGMT_IOCSLCDBACKLIGHT:
		set_pwm(0, val);
		s3c2410_gpio_setpin(S3C2410_GPB6, (val > 0));	
		break;

	case JIVE_MGMT_IOCGLCDBACKLIGHT:
		val = get_pwm(0);
		break;

	case JIVE_MGMT_IOCSLED:
		set_pwm(2, val);
		break;

	case JIVE_MGMT_IOCGLED:
		val = get_pwm(2);
		break;

	case JIVE_MGMT_IOCGBATTERY:
		val = get_battery();
		break;

	case JIVE_MGMT_IOCSSWPWOFF:
		s3c2410_gpio_setpin(S3C2410_GPC5, val);
		break;

	case JIVE_MGMT_IOCSCHGSUSP:
		s3c2410_gpio_setpin(S3C2410_GPG12, val);
		break;

	case JIVE_MGMT_IOCGCHGACPR:
		s3c2410_gpio_cfgpin(S3C2410_GPG3, S3C2410_GPG3_INP);
		val = (s3c2410_gpio_getpin(S3C2410_GPG3) > 0);
		s3c2410_gpio_cfgpin(S3C2410_GPG3, S3C2410_GPG3_EINT11);
		break;

	case JIVE_MGMT_IOCGUSBDETECT:
		s3c2410_gpio_cfgpin(S3C2410_GPG1, S3C2410_GPG1_INP);
		val = (s3c2410_gpio_getpin(S3C2410_GPG1) > 0);
		s3c2410_gpio_cfgpin(S3C2410_GPG1, S3C2410_GPG1_EINT9);
		break;

	case JIVE_MGMT_IOCGCHGPPR:
		val = (s3c2410_gpio_getpin(S3C2410_GPH7) > 0);
		break;

	case JIVE_MGMT_IOCSCHGHPWR:
		s3c2410_gpio_setpin(S3C2410_GPH6, val);
		break;

	case JIVE_MGMT_IOCGDOCKDETECT:
		val = (s3c2410_gpio_getpin(S3C2410_GPG7) > 0);
		break;

	case JIVE_MGMT_IOCGPHONEDETECT:
		s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_INP);
		val = (s3c2410_gpio_getpin(S3C2410_GPG14) > 0);
		s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_EINT22);
		break;

#if 0
	case JIVE_MGMT_IOCSWM8750:
		printk("WM8750_SEND_CMD(%x, %x)\n", (val >> 8), (val & 0xFF));
		WM8750_SEND_CMD( (val >> 8), (val & 0xFF) );
		break;
#endif

#if 0
	case JIVE_MGMT_IOCSLCD:
		LCD_CMD( (val >> 16), (val & 0xFFFF));
		break;
#endif

	case JIVE_MGMT_IOCGCLKCON: {
		int i;
		unsigned int clkcon = RAW(S3C2410_CLKCON);

		for (i=0; i<=28; i++) {
			printk("CLKCON %d: %d\n", i, (clkcon & 1));
			clkcon >>= 1;
		}
		break;

	}
	case JIVE_MGMT_IOCSCLKCONENABLE: {
		unsigned int mask = 1 << val;
		RAW(S3C2410_CLKCON) |= mask;
		break;
	}
	case JIVE_MGMT_IOCSCLKCONDISABLE: {
		unsigned int mask = 1 << val;
		RAW(S3C2410_CLKCON) &= ~mask;
		break;
	}
	case JIVE_MGMT_IOCGARMDIV:
		val = (RAW(S3C2410_CLKDIVN) & S3C2412_CLKDIVN_ARMDIVN);
		break;

	case JIVE_MGMT_IOCSARMDIV:
		if (val) {
			RAW(S3C2410_CLKDIVN) |= S3C2412_CLKDIVN_ARMDIVN;
		}
		else {
			RAW(S3C2410_CLKDIVN) &= ~S3C2412_CLKDIVN_ARMDIVN;
		}
		break;

	case JIVE_MGMT_IOCGDSC0:
		val = RAW(S3C2412_DSC0);
		break;

	case JIVE_MGMT_IOCGDSC1:
		val = RAW(S3C2412_DSC1);
		break;

	default:
		return -EINVAL;
	}

	put_user(val, (unsigned int __user *)arg);
	return 0;
}


static void acpower_timer(unsigned long data)
{
	input_report_switch(bsp_dev, SW_0, data);
	input_sync(bsp_dev);
}


static irqreturn_t acpower_irq(int irq, void *dev_id)
{
	int val;

	s3c2410_gpio_cfgpin(S3C2410_GPG3, S3C2410_GPG3_INP);
	val = (s3c2410_gpio_getpin(S3C2410_GPG3) > 0);

	// debounce CHR_ACPR for half a second
	acpr_timer.data = val;
	mod_timer(&acpr_timer, jiffies + (HZ >> 2));

	s3c2410_gpio_cfgpin(S3C2410_GPG3, S3C2410_GPG3_EINT11);

	return IRQ_HANDLED;
}


static irqreturn_t headphone_irq(int irq, void *dev_id)
{
	int val;

	s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_INP);
	val = (s3c2410_gpio_getpin(S3C2410_GPG14) > 0);

	input_report_switch(bsp_dev, SW_1, val);
	input_sync(bsp_dev);

	s3c2410_gpio_cfgpin(S3C2410_GPG14, S3C2410_GPG14_EINT22);

	return IRQ_HANDLED;
}


static irqreturn_t dockdetect_irq(int irq, void *dev_id)
{
	int val;

	s3c2410_gpio_cfgpin(S3C2410_GPG7, S3C2410_GPG7_INP);
	val = (s3c2410_gpio_getpin(S3C2410_GPG7) > 0);

	input_report_switch(bsp_dev, SW_2, val);
	input_sync(bsp_dev);

	s3c2410_gpio_cfgpin(S3C2410_GPG7, S3C2410_GPG7_EINT15);

	return IRQ_HANDLED;
}

static irqreturn_t usbdetect_irq(int irq, void *dev_id)
{
	int val;

	s3c2410_gpio_cfgpin(S3C2410_GPG1, S3C2410_GPG1_INP);
	val = (s3c2410_gpio_getpin(S3C2410_GPG1) > 0);

	input_report_switch(bsp_dev, SW_3, val);
	input_sync(bsp_dev);

	s3c2410_gpio_cfgpin(S3C2410_GPG1, S3C2410_GPG1_EINT9);

	return IRQ_HANDLED;
}


static struct file_operations jive_mgmt_fops = {
	owner:		THIS_MODULE,
	open:		jive_mgmt_open,
	release:	jive_mgmt_release,
	ioctl:		jive_mgmt_ioctl,
};

static struct miscdevice jive_mgmt_miscdev = {
	JIVE_MGMT_MISCDEV_MINOR,
	JIVE_MGMT_MODULE_NAME,
	&jive_mgmt_fops,
};

static int __init jive_mgmt_init(void) {
	unsigned long flags;
	int rc;

	/* For compatibility with the old bootloader allow s3c2413 machines
	 * to work here too. We can't use machine_is_s3c2413() here as that
	 * machine is not configured in the kernel.
	 *
	 * The S3C2413 hack should be removed when the old bootloader has
	 * been phased out.
	 */
	if (!(machine_is_jive() || machine_arch_type == MACH_TYPE_S3C2413))
		return 0;

	printk("jive_mgmt_init\n");


	// make the flash writeable on boot
	// GPC6 => NAND_nWP
	s3c2410_gpio_cfgpin(S3C2410_GPC6, S3C2410_GPC6_OUTP);
	s3c2410_gpio_pullup(S3C2410_GPC6, 0);
	s3c2410_gpio_setpin(S3C2410_GPC6, 1);	

	// TOUT0 => BACKLIGHT_PWM

	local_irq_save(flags);
	init_pwm(0, MAX_DIMMER);
	local_irq_restore(flags);

	// GPB6 => BACKLIGHT_ENABLE
	s3c2410_gpio_cfgpin(S3C2410_GPB6, S3C2410_GPB6_OUTP);
	s3c2410_gpio_pullup(S3C2410_GPB6, 0);
	s3c2410_gpio_setpin(S3C2410_GPB6, 1);	

	// TOUT2 => LED0
	local_irq_save(flags);
	init_pwm(2, MAX_DIMMER - (MAX_DIMMER / 16)); // XXXX so not to blind me
	local_irq_restore(flags);

	// Set by the video driver, don't change them:
	// GPG13 => LCD_nRESET
	// GPC4 => LCD_ENABLE

	// GPC5 => SW_PW_OFF
	s3c2410_gpio_cfgpin(S3C2410_GPC5, S3C2410_GPC5_OUTP);
	s3c2410_gpio_pullup(S3C2410_GPC5, 0);
	s3c2410_gpio_setpin(S3C2410_GPC5, 0);

	// GPG12 => CHG_SUSP
	s3c2410_gpio_cfgpin(S3C2410_GPG12, S3C2410_GPG12_OUTP);
	s3c2410_gpio_pullup(S3C2410_GPG12, 0);
	s3c2410_gpio_setpin(S3C2410_GPG12, 0);

	// GPG8 <= CHG_ACPR
	s3c2410_gpio_cfgpin(S3C2410_GPG3, S3C2410_GPG3_EINT11);
	s3c2410_gpio_pullup(S3C2410_GPG3, 1);
	s3c2410_gpio_irqfilter(S3C2410_GPG3, 1, 0);

	// must be before irq's are setup
	bsp_dev = input_allocate_device();
	if (bsp_dev) {
		struct input_dev *dev = bsp_dev;
		
		dev->evbit[0] = BIT(EV_SW);
		set_bit(SW_0, dev->swbit); // ac power
		set_bit(SW_1, dev->swbit); // headphone detect
		set_bit(SW_2, dev->swbit); // dock detect
		set_bit(SW_3, dev->swbit); // usb detect
		dev->name = "Switch";

		input_register_device(dev);
	}

	init_timer(&acpr_timer);
	acpr_timer.function = acpower_timer;

        if (request_irq(IRQ_EINT11, acpower_irq,
			IRQF_TRIGGER_BOTH | IRQF_SAMPLE_RANDOM,
			"SW_0", bsp_dev)) {
                 printk(KERN_ERR "Could not allocate acpower IRQ_EINT11\n");
	}


	// GPG1 <= USB_DET
	s3c2410_gpio_cfgpin(S3C2410_GPG1, S3C2410_GPG1_EINT9);
	s3c2410_gpio_pullup(S3C2410_GPG1, 1);
	s3c2410_gpio_irqfilter(S3C2410_GPG1, 1, 0);

        if (request_irq(IRQ_EINT9, usbdetect_irq,
			IRQF_TRIGGER_BOTH | IRQF_SAMPLE_RANDOM | 
			IRQF_SHARED | IRQF_DISABLED,
			"SW_3", bsp_dev)) {
		printk(KERN_ERR "Could not allocate usbdetect IRQ_EINT9\n");
	}


	// GPH7 <= CHG_PPR
	s3c2410_gpio_cfgpin(S3C2410_GPH7, S3C2410_GPH7_INP);
	s3c2410_gpio_pullup(S3C2410_GPH7, 1);


	// GPH6 => CHG_HPWR
	s3c2410_gpio_cfgpin(S3C2410_GPH6, S3C2410_GPH6_OUTP);
	s3c2410_gpio_pullup(S3C2410_GPH6, 0);
	s3c2410_gpio_setpin(S3C2410_GPH6, 0);


	// GPG7 <= DOCK_DETECT (also on AIN0)
	s3c2410_gpio_pullup(S3C2410_GPG7, 0);
	s3c2410_gpio_irqfilter(S3C2410_GPG7, 1, 0);
	
        if (request_irq(IRQ_EINT15, dockdetect_irq,
			IRQF_TRIGGER_BOTH | IRQF_SAMPLE_RANDOM,
			"SW_2", bsp_dev)) {
                 printk(KERN_ERR "Could not allocate dockdetect IRQ_EINT15\n");
	}


	// GPG14 <= HEADPHONE_DETECT
	s3c2410_gpio_pullup(S3C2410_GPG14, 1);

        if (request_irq(IRQ_EINT22, headphone_irq,
			IRQF_TRIGGER_BOTH | IRQF_SAMPLE_RANDOM,
			"SW_1", bsp_dev)) {
                 printk(KERN_ERR "Could not allocate headphone IRQ_EINT22\n");
	}


	// ADC clock, needed to read battery voltage
	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}

	clk_enable(adc_clock);
	adc_regs = ioremap(S3C2410_PA_ADC, S3C24XX_SZ_ADC);

	rc = misc_register (&jive_mgmt_miscdev);
	if (rc) {
		printk (KERN_ERR PFX "misc device register failed\n");
		return rc;
	}

	// set initial state
	acpower_irq(0, NULL);
	headphone_irq(0, NULL);
	dockdetect_irq(0, NULL);
	usbdetect_irq(0, NULL);

	return 0;
}

static void __exit jive_mgmt_exit(void) {
	free_irq(IRQ_EINT9, bsp_dev);
	free_irq(IRQ_EINT15, bsp_dev);
	free_irq(IRQ_EINT22, bsp_dev);
	free_irq(IRQ_BATT_FLT, bsp_dev);

	input_unregister_device(bsp_dev);
	misc_deregister(&jive_mgmt_miscdev);
}


module_init(jive_mgmt_init);
module_exit(jive_mgmt_exit);


MODULE_AUTHOR("richard@slimdevices.com");
MODULE_DESCRIPTION("JIVE device management");
MODULE_LICENSE("GPL");
