/*
 * Copyright (C) 2005 for Samsung Electronics 
 * 		- by Jaswinder Singh Brar <jassi@samsung.com>
 *
 * Thanx to many developers for the skeleton of this driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/regs-adc.h>
#include <asm/hardware/clock.h>

#define DELAY_JIFFIES		5		// 50ms
#define ADC_DELAY		20000
#define ADC_FREQ		2000000		// ADC Frequency is set to 2MHz

#define PEN_UP		0
#define PEN_DOWN	1
#define CALC_X		0
#define CALC_Y		1

#define S3C2440TSVERSION	0x0101

#define wait_down_int()								\
		do{ 	unsigned int val;					\
			val = DOWN_INT | XP_PULL_UP_EN | 			\
				   XP_AIN | XM_HIZ | YP_AIN | YM_GND |  	\
				   XP_PST(WAIT_INT_MODE); 			\
			__raw_writel(val, S3C2440_ADCTSC);			\
		}while(0);

#define wait_up_int()								\
		do{ 	unsigned int val;					\
			val = UP_INT | XP_PULL_UP_EN | XP_AIN | XM_HIZ | 	\
				   YP_AIN | YM_GND | XP_PST(WAIT_INT_MODE); 	\
			__raw_writel(val, S3C2440_ADCTSC);			\
		}while(0);

#define mode_x_axis()								\
		do{ 								\
			unsigned int val;					\
			val = XP_EXTVLT | XM_GND | YP_AIN | YM_HIZ | 		\
				   XP_PULL_UP_DIS | XP_PST(X_AXIS_MODE); 	\
			__raw_writel(val, S3C2440_ADCTSC);			\
		}while(0);

#define mode_y_axis()								\
		do{ 	unsigned int val;					\
			val = XP_AIN | XM_HIZ | YP_EXTVLT | YM_GND |		\
				   XP_PULL_UP_DIS | XP_PST(Y_AXIS_MODE);	\
			__raw_writel(val, S3C2440_ADCTSC);			\
		}while(0);

#define STOP_ADC()	__raw_writel(ADC_STANDBY_MODE, S3C2440_ADCCON)

#define START_ADC()	 							\
		do{ 	unsigned int val;					\
			val = __raw_readl(S3C2440_ADCCON);			\
			val &= ~ADC_STANDBY_MODE;				\
			val |= ADC_START;					\
			__raw_writel(val, S3C2440_ADCCON);			\
		}while(0);

#define start_adc_x()								\
		do{ 	unsigned int val;					\
			val = PRESCALE_EN | PRSCVL(prescalar) | 		\
				   ADC_INPUT(ADC_IN7) | ADC_START_START; 	\
			__raw_readl(S3C2440_ADCDAT0);				\
			__raw_writel(val, S3C2440_ADCCON);			\
		}while(0);							\

#define start_adc_y()								\
		do{ 	unsigned int val;					\
			val = PRESCALE_EN | PRSCVL(prescalar) | 		\
				   ADC_INPUT(ADC_IN5) | ADC_START_START; 	\
			__raw_readl(S3C2440_ADCDAT1);				\
			__raw_writel(val, S3C2440_ADCCON);			\
		}while(0);							\

#define freeze_ts_adc()								\
		do{ 	unsigned int val;					\
			val = __raw_readl(S3C2440_ADCCON);			\
			val &= ~(ADCCON_READ_START);				\
			__raw_writel(val, S3C2440_ADCCON);			\
		}while(0);

	/* Globals */

#ifdef CONFIG_S3C2440_TSHOOK
static void hook_timer_fire(unsigned long);
static struct timer_list hook_timer = TIMER_INITIALIZER(hook_timer_fire, 1, 0);
#endif

static struct clk	*adc_clock;
unsigned long prescalar;

static char *s3c2440ts_name = "s3c2440 TouchScreen";

struct s3c2440ts {
	struct input_dev dev;
	int penstatus;
	int adc_state;
	unsigned int xp;
	unsigned int yp;
	char phys[32];
};

static struct s3c2440ts ts;

static inline void elfin_get_xy(void);

static irqreturn_t ts_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	disable_irq(IRQ_TC);
	if(ts.penstatus == PEN_UP){	// Now its DOWN
		ts.adc_state = CALC_X;
		mode_x_axis();
		start_adc_x();

#ifdef CONFIG_S3C2440_ADC_POLLING
		while(!( __raw_readl(S3C2440_ADCCON) & (1<<15)));

#ifndef CONFIG_S3C2440_TSHOOK
		if(ts.penstatus == PEN_UP)
#endif
			elfin_get_xy();

#endif
	}else{
		ts.penstatus = PEN_UP;
		wait_down_int();
		//input_report_abs(&ts.dev, ABS_X, ts.xp);
		//input_report_abs(&ts.dev, ABS_Y, ts.yp);	// Shud we ?
		input_report_key(&ts.dev, BTN_TOUCH, 0);
		input_report_abs(&ts.dev, ABS_PRESSURE, 0);
		input_sync(&ts.dev);
	}
	enable_irq(IRQ_TC);
	return IRQ_HANDLED;
}

#ifdef CONFIG_S3C2440_TSHOOK
static void hook_timer_fire(unsigned long data)
{
	if( (ts.adc_state != CALC_X) || (ts.penstatus != PEN_DOWN) )
		return;

	disable_irq(IRQ_TC);
	ts.adc_state = CALC_X;
	mode_x_axis();
	start_adc_x();

#ifdef CONFIG_S3C2440_ADC_POLLING
	while(!( __raw_readl(S3C2440_ADCCON) & (1<<15)));

#ifndef CONFIG_S3C2440_TSHOOK
	if(ts.penstatus == PEN_UP)
#endif
		elfin_get_xy();

#endif
	enable_irq(IRQ_TC);
}
#endif

static inline void elfin_get_xy(void)
{
	if(ts.adc_state == CALC_X){
		ts.adc_state = CALC_Y;
		freeze_ts_adc();
		ts.xp = __raw_readl(S3C2440_ADCDAT0) & 0x3FF;

		mode_y_axis();
		start_adc_y();
#ifdef CONFIG_S3C2440_ADC_POLLING
		while(!( __raw_readl(S3C2440_ADCCON) & (1<<15)));

		ts.adc_state = CALC_X;
		freeze_ts_adc();
		ts.yp = __raw_readl(S3C2440_ADCDAT1) & 0x3FF;
		ts.penstatus = PEN_DOWN;
		wait_up_int();

		//printk("(%d,%d) \n", ts.xp, ts.yp);
		input_report_abs(&ts.dev, ABS_X, ts.xp);
		input_report_abs(&ts.dev, ABS_Y, ts.yp);
		input_report_key(&ts.dev, BTN_TOUCH, 1);
		input_report_abs(&ts.dev, ABS_PRESSURE, 1);
		input_sync(&ts.dev);
		mod_timer(&hook_timer, jiffies + DELAY_JIFFIES);
#endif
	}else{
		ts.adc_state = CALC_X;
		freeze_ts_adc();
		ts.yp = __raw_readl(S3C2440_ADCDAT1) & 0x3FF;
		ts.penstatus = PEN_DOWN;
		wait_up_int();

		//printk("(%d,%d) \n", ts.xp, ts.yp);
		input_report_abs(&ts.dev, ABS_X, ts.xp);
		input_report_abs(&ts.dev, ABS_Y, ts.yp);
		input_report_key(&ts.dev, BTN_TOUCH, 1);
		input_report_abs(&ts.dev, ABS_PRESSURE, 1);
		input_sync(&ts.dev);
		mod_timer(&hook_timer, jiffies + DELAY_JIFFIES);
	}
}

#ifndef CONFIG_S3C2440_ADC_POLLING
static irqreturn_t adc_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	disable_irq(IRQ_ADC);
#ifndef CONFIG_S3C2440_TSHOOK
	if(ts.penstatus == PEN_UP)
#endif
		elfin_get_xy();

	enable_irq(IRQ_ADC);

	return IRQ_HANDLED;
}
#endif

static int __init s3c2440ts_probe(struct device *dev)
{
	unsigned long pclk;

	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}
	clk_use(adc_clock);
	clk_enable(adc_clock);

	pclk = clk_get_rate(adc_clock);
	prescalar = pclk/ADC_FREQ - 1;
	prescalar &= 0xff;

	__raw_writel(ADC_DELAY, S3C2440_ADCDLY);

	/* Initialise input stuff */
	memset(&ts, 0, sizeof(struct s3c2440ts));
	init_input_dev(&ts.dev);
	ts.dev.evbit[0] = BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	ts.dev.keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);
	input_set_abs_params(&ts.dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(&ts.dev, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(&ts.dev, ABS_PRESSURE, 0, 1, 0, 0);

	sprintf(ts.phys, "ts0");

	ts.penstatus = PEN_UP;
	ts.adc_state = CALC_X;
	ts.dev.private = &ts;
	ts.dev.name = s3c2440ts_name;
	ts.dev.phys = ts.phys;
	ts.dev.id.vendor = 0xDEAD;
	ts.dev.id.product = 0xBEEF;
	ts.dev.id.version = S3C2440TSVERSION;

	/* Get irqs */
#ifndef CONFIG_S3C2440_ADC_POLLING
	if (request_irq(IRQ_ADC, adc_handler, SA_SAMPLE_RANDOM,
		"s3c2440_adc", &ts.dev)) {
		printk(KERN_ERR "s3c2440_ts.c: Could not allocate ts IRQ_ADC !\n");
		return -EIO;
	}
#else
	printk(KERN_INFO "ADC In Polling Mode Selected.\n");
#endif
	if (request_irq(IRQ_TC, ts_handler, SA_SAMPLE_RANDOM,
			"s3c2440_ts", &ts.dev)) {
		printk(KERN_ERR "s3c2440_ts.c: Could not allocate ts IRQ_TC !\n");
		return -EIO;
	}

	printk(KERN_INFO "%s successfully loaded\n", s3c2440ts_name);

	/* Register to the input system */
	input_register_device(&ts.dev);

	wait_down_int();

	return 0;
}

static int s3c2440ts_remove(struct device *dev)
{
	del_timer(&hook_timer);
	free_irq(IRQ_TC,&ts.dev);
#ifndef CONFIG_S3C2440_ADC_POLLING
	free_irq(IRQ_ADC,&ts.dev);
#endif

	if (adc_clock) {
		clk_disable(adc_clock);
		clk_unuse(adc_clock);
		clk_put(adc_clock);
		adc_clock = NULL;
	}

	//wait_down_int();

	input_unregister_device(&ts.dev);

	return 0;
}

static struct device_driver s3c2440ts_driver = {
       .name           = "s3c2440-adcts",
       .bus            = &platform_bus_type,
       .probe          = s3c2440ts_probe,
       .remove         = s3c2440ts_remove,
};


int __init s3c2440ts_init(void)
{
	return driver_register(&s3c2440ts_driver);
}

void __exit s3c2440ts_exit(void)
{
	driver_unregister(&s3c2440ts_driver);
}

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("S3C24X0 TouchScreen driver");
MODULE_LICENSE("GPL");

module_init(s3c2440ts_init);
module_exit(s3c2440ts_exit);
