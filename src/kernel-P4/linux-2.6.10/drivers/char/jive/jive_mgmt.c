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
#include <linux/l3/WM8750.h>
#include <linux/pm.h>

#include <asm/arch/hardware.h>
#include <asm/arch-s3c2413/regs-adc.h>
#include <asm/arch-s3c2413/regs-clock.h>
#include <asm/arch-s3c2413/regs-gpio.h>
#include <asm/arch-s3c2413/regs-timer.h>
#include <asm/arch-s3c2413/regs-irq.h>

#include <asm/arch-s3c2413/map.h>
#include <asm/hardware/clock.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct clk *adc_clock;

extern void Init_LDI(void);
extern void LCD_CMD(unsigned short address, unsigned short data);


#define JIVE_MGMT_VERSION "1.0.0"
#define JIVE_MGMT_MODULE_NAME "jive_mgmt"
#define JIVE_MGMT_MISCDEV_MINOR MISC_DYNAMIC_MINOR
#define JIVE_MGMT_NAME   JIVE_MGMT_MODULE_NAME " hardware driver " JIVE_MGMT_VERSION
#define PFX JIVE_MGMT_MODULE_NAME ": "

#define JIVE_MGMT_MAGIC 'P'


#define JIVE_MGMT_IOCSLCDPOWER 		1
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
#define JIVE_MGMT_IOCPM_SUSPEND		30	// power management: suspend now (no args)
#define JIVE_MGMT_IOCPM_INT_ENABLE	31	// enable   an interrupt source for wakeup
#define JIVE_MGMT_IOCPM_INT_DISABLE	32	// disable  "   "          "     "     "



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

#define RAW(var) (*(volatile unsigned int __force *)var)


static struct input_dev bsp_dev;


static int get_battery(void) {
	unsigned int i, max, min, maxindex, minindex, temp[5];
	unsigned int sum = 0x00;

	for(i=0; i<5; i++) {
		(*(volatile unsigned int __force *)S3C2413_ADCCON) = S3C2413_ADCCON_PRSCEN | S3C2413_ADCCON_PRSCVL(0x70) | S3C2413_ADCCON_SELMUX(0x02);
		udelay(10);
		(*(volatile unsigned int __force *)S3C2413_ADCTSC) &= 0xfb;
		(*(volatile unsigned int __force *)S3C2413_ADCCON) |= S3C2413_ADCCON_ENABLE_START;

		while((*(volatile unsigned int __force *)S3C2413_ADCCON) & S3C2413_ADCCON_ENABLE_START); //check if start bit is low
		while(!((*(volatile unsigned int __force *)S3C2413_ADCCON) & S3C2413_ADCCON_ECFLG)); //check if EC(End of Conversion) flag is high

		temp[i] = (((*(volatile unsigned int __force*)S3C2413_ADCDAT0)&0x3ff) >> 2);
		udelay(10);
	}

	(*(volatile unsigned int __force *)S3C2413_ADCCON) &= ~S3C2413_ADCCON_PRSCEN;


	// Discard maximum and minimum samples, average the rest
	// FIXME why do this?
	max = temp[0]; maxindex = 0;
	min = temp[0]; minindex = 0;
	
	for(i=0; i<5; i++) {
		if (temp[i] > max) {
			max = temp[i]; maxindex = i;
		}
		if (temp[i] < min) {
			min = temp[i]; minindex = i;
		}
	}

	for(i=0;i<5; i++) {
		if (i != maxindex && i != minindex) {
			sum += temp[i];
		}
	}
	
	if (maxindex == minindex) {
		return (unsigned char) (sum/4);
	}
	else {
		return (unsigned char) (sum/3);
	}
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
		RAW(S3C2413_GPBCON) &= ~(S3C2413_GPB0_TOUT0|S3C2413_GPB0_OUTP);
		RAW(S3C2413_GPBCON) |= S3C2413_GPB0_TOUT0;			     // GPB0 - TOUT0
		RAW(S3C2413_TCFG0) &= ~S3C2413_TCFG_PRESCALER0_MASK;		     // Prescale 0x00 for Timer 0 and 1
		RAW(S3C2413_TCFG1) &= ~S3C2413_TCFG1_MUX0_MASK;			     // PCLK divide 1/2
		RAW(S3C2413_TCFG1) |= S3C2413_TCFG1_MUX0_DIV2;

		RAW(S3C2413_TCON) |= S3C2413_TCON_T0MANUALUPD;                       // Manual update
		RAW(S3C2413_TCNTB(0)) = MAX_DIMMER;
		RAW(S3C2413_TCMPB(0)) = slide_pwm_value(val);
		RAW(S3C2413_TCON) |= (S3C2413_TCON_T0RELOAD | S3C2413_TCON_T0START); // Start timer 0 with Auto reload
		RAW(S3C2413_TCON) &= ~S3C2413_TCON_T0MANUALUPD;                      // Manual update mode disable
	}
	else if (pin==2) {
		RAW(S3C2413_GPBCON) &= ~(S3C2413_GPB2_TOUT2|S3C2413_GPB2_OUTP);
		RAW(S3C2413_GPBCON) |= S3C2413_GPB2_TOUT2;                           // GPB2 - TOUT2
// Following values are set in arch/arm/mach-s3c2413/time.c
// DO NOT CHANGE
// 		RAW(S3C2413_TCFG0) &= ~S3C2413_TCFG_PRESCALER1_MASK;
// 		RAW(S3C2413_TCFG0) |= (0x0F<<S3C2413_TCFG_PRESCALER1_SHIFT);	     // Prescale for Timer 2, 3 and 4
		RAW(S3C2413_TCFG1) &= ~S3C2413_TCFG1_MUX2_MASK;                      // PCLK divide 1/2
		RAW(S3C2413_TCFG1) |= S3C2413_TCFG1_MUX2_DIV2;

		RAW(S3C2413_TCON) |= S3C2413_TCON_T2MANUALUPD;	                     // Manual update
		RAW(S3C2413_TCNTB(2)) = MAX_DIMMER;		 
		RAW(S3C2413_TCMPB(2)) = slide_pwm_value(val);
		RAW(S3C2413_TCON) |= (S3C2413_TCON_T2RELOAD | S3C2413_TCON_T2START); // Start timer 2 with Auto reload
		RAW(S3C2413_TCON) &= ~S3C2413_TCON_T2MANUALUPD;	                     // Manual update mode disable
	}
}

static void set_pwm(int pin, int val) {
	if (pin==0) {	
		RAW(S3C2413_TCON) |= S3C2413_TCON_T0MANUALUPD; // Manual update
		RAW(S3C2413_TCMPB(0)) = slide_pwm_value(val);
		RAW(S3C2413_TCON) |= (S3C2413_TCON_T0RELOAD | S3C2413_TCON_T0START); // Start timer 0 with Auto reload
		RAW(S3C2413_TCON) &= ~S3C2413_TCON_T0MANUALUPD;	// Manual update mode disable
	}
	else if (pin==2) {
		RAW(S3C2413_TCON) |= S3C2413_TCON_T2MANUALUPD; // Manual update
		RAW(S3C2413_TCMPB(2)) = slide_pwm_value(val);
		RAW(S3C2413_TCON) |= (S3C2413_TCON_T2RELOAD | S3C2413_TCON_T2START); // Start timer 2 with Auto reload
		RAW(S3C2413_TCON) &= ~S3C2413_TCON_T2MANUALUPD;	// Manual update mode disable
	}
}

static int get_pwm(int pin) {
	return __raw_readl(S3C2413_TCMPB(pin));
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

	case JIVE_MGMT_IOCSLCDPOWER:
		// FIXME move to power management in video driver?
		if (val) {
			Init_LDI();
		}
		else {
			LCD_CMD(0x10, 0x0004); // Deep sleep
		}
		break;

	case JIVE_MGMT_IOCSLCDBACKLIGHT:
		set_pwm(0, val);
		s3c2413_gpio_setpin(S3C2413_GPB6, (val > 0));	
		break;

	case JIVE_MGMT_IOCGLCDBACKLIGHT:
		val = get_pwm(0);
		break;

	case JIVE_MGMT_IOCSLED:
		set_pwm(2, MAX_DIMMER - val);
		break;

	case JIVE_MGMT_IOCGLED:
		val = MAX_DIMMER - get_pwm(2);
		break;

	case JIVE_MGMT_IOCGBATTERY:
		val = get_battery();
		break;

	case JIVE_MGMT_IOCSSWPWOFF:
		s3c2413_gpio_setpin(S3C2413_GPC5, val);
		break;

	case JIVE_MGMT_IOCSCHGSUSP:
		s3c2413_gpio_setpin(S3C2413_GPG12, val);
		break;

	case JIVE_MGMT_IOCGCHGACPR:
		s3c2413_gpio_cfgpin(S3C2413_GPG8, S3C2413_GPG8_INP);
		val = (s3c2413_gpio_getpin(S3C2413_GPG8) > 0);
		s3c2413_gpio_cfgpin(S3C2413_GPG8, S3C2413_GPG8_EINT16);
		break;

	case JIVE_MGMT_IOCGUSBDETECT:
		s3c2413_gpio_cfgpin(S3C2413_GPG1, S3C2413_GPG1_INP);
		val = (s3c2413_gpio_getpin(S3C2413_GPG1) > 0);
		s3c2413_gpio_cfgpin(S3C2413_GPG1, S3C2413_GPG1_EINT9);
		break;

	case JIVE_MGMT_IOCGCHGPPR:
		val = (s3c2413_gpio_getpin(S3C2413_GPH7) > 0);
		break;

	case JIVE_MGMT_IOCSCHGHPWR:
		s3c2413_gpio_setpin(S3C2413_GPH6, val);
		break;

	case JIVE_MGMT_IOCGDOCKDETECT:
		val = (s3c2413_gpio_getpin(S3C2413_GPG7) > 0);
		break;

	case JIVE_MGMT_IOCGPHONEDETECT:
		s3c2413_gpio_cfgpin(S3C2413_GPG14, S3C2413_GPG14_INP);
		val = (s3c2413_gpio_getpin(S3C2413_GPG14) > 0);
		s3c2413_gpio_cfgpin(S3C2413_GPG14, S3C2413_GPG14_EINT22);
		break;

	case JIVE_MGMT_IOCSWM8750:
		printk("WM8750_SEND_CMD(%x, %x)\n", (val >> 8), (val & 0xFF));
		WM8750_SEND_CMD( (val >> 8), (val & 0xFF) );
		break;

	case JIVE_MGMT_IOCSLCD:
		LCD_CMD( (val >> 16), (val & 0xFFFF));
		break;

	case JIVE_MGMT_IOCPM_SUSPEND:
		printk("jive mgmt got suspend cmd\n");
		pm_suspend(PM_SUSPEND_MEM);
		break;

	case JIVE_MGMT_IOCPM_INT_ENABLE:
		printk("jive mgmt enabling wake on interrupt: %d\n", val);
		enable_irq_wake(val);
		break;

	case JIVE_MGMT_IOCPM_INT_DISABLE:
		printk("jive mgmt disabling wake on interrupt: %d\n", val);
		disable_irq_wake(val);
		break;

	default:
		return -EINVAL;
	}

	put_user(val, (unsigned int __user *)arg);
	return 0;
}

static irqreturn_t batf_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	printk("BATTERY FLAT!!\n");

	// FIXME suspend jive

	return IRQ_NONE;
}


static irqreturn_t acpower_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	int val;

	s3c2413_gpio_cfgpin(S3C2413_GPG8, S3C2413_GPG8_INP);
	val = (s3c2413_gpio_getpin(S3C2413_GPG8) > 0);

	input_report_switch(&bsp_dev, SW_0, val);
	input_sync(&bsp_dev);

	s3c2413_gpio_cfgpin(S3C2413_GPG8, S3C2413_GPG8_EINT16);

	return IRQ_HANDLED;
}


static irqreturn_t headphone_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	int val;

	s3c2413_gpio_cfgpin(S3C2413_GPG14, S3C2413_GPG14_INP);
	val = (s3c2413_gpio_getpin(S3C2413_GPG14) > 0);

	input_report_switch(&bsp_dev, SW_1, val);
	input_sync(&bsp_dev);

	s3c2413_gpio_cfgpin(S3C2413_GPG14, S3C2413_GPG14_EINT22);

	return IRQ_HANDLED;
}


static irqreturn_t dockdetect_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	int val;

	s3c2413_gpio_cfgpin(S3C2413_GPG7, S3C2413_GPG7_INP);
	val = (s3c2413_gpio_getpin(S3C2413_GPG7) > 0);

	input_report_switch(&bsp_dev, SW_2, val);
	input_sync(&bsp_dev);

	s3c2413_gpio_cfgpin(S3C2413_GPG7, S3C2413_GPG7_EINT15);

	return IRQ_HANDLED;
}

static irqreturn_t usbdetect_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	int val;

	s3c2413_gpio_cfgpin(S3C2413_GPG1, S3C2413_GPG1_INP);
	val = (s3c2413_gpio_getpin(S3C2413_GPG1) > 0);

	input_report_switch(&bsp_dev, SW_3, val);
	input_sync(&bsp_dev);

	s3c2413_gpio_cfgpin(S3C2413_GPG1, S3C2413_GPG1_EINT9);

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
	int rc;

	printk("jive_mgmt_init\n");

	// make the flash writeable on boot
	// GPC6 => NAND_nWP
	s3c2413_gpio_cfgpin(S3C2413_GPC6, S3C2413_GPC6_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPC6, 0);
	s3c2413_gpio_setpin(S3C2413_GPC6, 1);	

	// TOUT0 => BACKLIGHT_PWM
	init_pwm(0, MAX_DIMMER);

	// GPB6 => BACKLIGHT_ENABLE
	s3c2413_gpio_cfgpin(S3C2413_GPB6, S3C2413_GPB6_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPB6, 0);
	s3c2413_gpio_setpin(S3C2413_GPB6, 1);	

	// TOUT2 => LED0
	init_pwm(2, MAX_DIMMER - (MAX_DIMMER / 16)); // XXXX so not to blind me

	// Set by the video driver, don't change them:
	// GPG13 => LCD_nRESET
	// GPC4 => LCD_ENABLE

	// GPC5 => SW_PW_OFF
	s3c2413_gpio_cfgpin(S3C2413_GPC5, S3C2413_GPC5_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPC5, 0);
	s3c2413_gpio_setpin(S3C2413_GPC5, 0);

	// GPG12 => CHG_SUSP
	s3c2413_gpio_cfgpin(S3C2413_GPG12, S3C2413_GPG12_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPG12, 0);
	s3c2413_gpio_setpin(S3C2413_GPG12, 0);

	// GPG8 <= CHG_ACPR
	s3c2413_gpio_cfgpin(S3C2413_GPG8, S3C2413_GPG8_EINT16);
	s3c2413_gpio_pullup(S3C2413_GPG8, 1);

	// EINT[16] both edge triggered
	*(volatile unsigned int __force *)S3C2413_EXTINT2 &= ~(0xF<<0);
	*(volatile unsigned int __force *)S3C2413_EXTINT2 |= (0xE<<0);

	disable_irq(IRQ_EINT16);
        if (request_irq(IRQ_EINT16, acpower_irq, SA_SAMPLE_RANDOM, "SW_0", NULL)) {
                 printk(KERN_ERR "Could not allocate acpower IRQ_EINT16\n");
	}


	// GPG1 <= USB_DET
	s3c2413_gpio_cfgpin(S3C2413_GPG1, S3C2413_GPG1_EINT9);
	s3c2413_gpio_pullup(S3C2413_GPG1, 1);

	// EINT[9] both edge triggered
	*(volatile unsigned int __force *)S3C2413_EXTINT1 &= ~(0xF<<4);
	*(volatile unsigned int __force *)S3C2413_EXTINT1 |= (0xE<<4);

	disable_irq(IRQ_EINT9);
        if (request_irq(IRQ_EINT9, usbdetect_irq, SA_SAMPLE_RANDOM, "SW_3", NULL)) {
                 printk(KERN_ERR "Could not allocate usbdetect IRQ_EINT9\n");
	}


	// GPH7 <= CHG_PPR
	s3c2413_gpio_cfgpin(S3C2413_GPH7, S3C2413_GPH7_INP);
	s3c2413_gpio_pullup(S3C2413_GPH7, 1);


	// GPH6 => CHG_HPWR
	s3c2413_gpio_cfgpin(S3C2413_GPH6, S3C2413_GPH6_OUTP);
	s3c2413_gpio_pullup(S3C2413_GPH6, 0);
	s3c2413_gpio_setpin(S3C2413_GPH6, 0);


	// GPG7 <= DOCK_DETECT (also on AIN0)
	s3c2413_gpio_cfgpin(S3C2413_GPG7, S3C2413_GPG7_EINT15);
	s3c2413_gpio_pullup(S3C2413_GPG7, 0);

	// EINT[15] both edge triggered
	*(volatile unsigned int __force *)S3C2413_EXTINT1 &= ~(0xF<<28);
	*(volatile unsigned int __force *)S3C2413_EXTINT1 |= (0xE<<28);

	disable_irq(IRQ_EINT15);
        if (request_irq(IRQ_EINT15, dockdetect_irq, SA_SAMPLE_RANDOM, "SW_2", NULL)) {
                 printk(KERN_ERR "Could not allocate dockdetect IRQ_EINT15\n");
	}


	// GPG14 <= HEADPHONE_DETECT
	s3c2413_gpio_cfgpin(S3C2413_GPG14, S3C2413_GPG14_EINT22);
	s3c2413_gpio_pullup(S3C2413_GPG14, 1);

	// EINT[22] both edge triggered
	*(volatile unsigned int __force *)S3C2413_EXTINT2 &= ~(0xF<<24);
	*(volatile unsigned int __force *)S3C2413_EXTINT2 |= (0xE<<24);

	disable_irq(IRQ_EINT22);
        if (request_irq(IRQ_EINT22, headphone_irq, SA_SAMPLE_RANDOM, "SW_1", NULL)) {
                 printk(KERN_ERR "Could not allocate headphone IRQ_EINT22\n");
	}


	// ADC clock, needed to read battery voltage
	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}
	clk_use(adc_clock);
	clk_enable(adc_clock);

	// Battery flat interrupt
	__raw_writel(__raw_readl(S3C2413_PWRCFG) | S3C2413_PWRCFG_BATF_CFG_INT, S3C2413_PWRCFG);
	disable_irq(IRQ_BATT_FLT);
        if (request_irq(IRQ_BATT_FLT, batf_irq, SA_SAMPLE_RANDOM,"batf", NULL)) {
                 printk(KERN_ERR "Could not allocate BATF IRQ_EINT12\n");
	}


	rc = misc_register (&jive_mgmt_miscdev);
	if (rc) {
		printk (KERN_ERR PFX "misc device register failed\n");
		return rc;
	}

	bsp_dev.evbit[0] = BIT(EV_SW);
	set_bit(SW_0, bsp_dev.swbit); // ac power
	set_bit(SW_1, bsp_dev.swbit); // headphone detect
	set_bit(SW_2, bsp_dev.swbit); // dock detect
	set_bit(SW_3, bsp_dev.swbit); // usb detect
	input_register_device(&bsp_dev);

	// set initial state
	acpower_irq(0, NULL, NULL);
	headphone_irq(0, NULL, NULL);
	dockdetect_irq(0, NULL, NULL);
	usbdetect_irq(0, NULL, NULL);

	return 0;
}

static void __exit jive_mgmt_exit(void) {
	misc_deregister(&jive_mgmt_miscdev);
}


module_init(jive_mgmt_init);
module_exit(jive_mgmt_exit);


MODULE_AUTHOR("richard@slimdevices.com");
MODULE_DESCRIPTION("JIVE device management");
MODULE_LICENSE("GPL");
