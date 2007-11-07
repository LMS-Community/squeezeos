/* drivers/char/jive/jive_matrix.c
 *
 * Copyright 2007 Logitech.
 *	Richard Titmuss <richard_titmuss@logitech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/i2o.h>
#include <linux/input.h>
#include <linux/interrupt.h>

#include <asm/hardware.h>
#include <asm/arch/regs-gpio.h>


#define DEBOUNCE_PERIOD_MS 5
#define PRESS_TIME_MS 15

static struct input_dev *matrix_dev;

struct timer_list matrix_timer;

static unsigned int last_state;

static unsigned int state[PRESS_TIME_MS / DEBOUNCE_PERIOD_MS];

static unsigned int index;


static unsigned int keymap[] = {
	// Columns are in this order:
	//  KEY_INPUT4
	//  KEY_INPUT0
	//  KEY_INPUT1
	//  KEY_INPUT2
	//  KEY_INPUT3

	// KEY_OUTPUT2
	KEY_UNKNOWN,
	KEY_B, // SW10 Fwd
	KEY_Z, // SW9 Rew
	KEY_C, // SW8 Pause
	KEY_UNKNOWN,

	// KEY_OUTPUT1
	KEY_UNKNOWN,
	KEY_X, // SW5 Play
	KEY_A, // SW6 Add
	KEY_EQUAL, // SW7 Volume Up
	KEY_UNKNOWN,

	// KEY_OUTPUT0
	KEY_H, // SW1 Home
	KEY_UNKNOWN,
	KEY_LEFT, // SW2 Back
	KEY_MINUS, // SW3 Volume Down
	KEY_RIGHT, // SW16 Go
};

#define IRQF_TRIGGER_BOTH (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

/* use IRQF_DISABLED to ensure that we aren't re-entered */
#define IRQ_FLAGS IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_BOTH | IRQF_DISABLED

static unsigned int matrix_irqs[] = {
	[0] = IRQ_EINT3,
	[1] = IRQ_EINT4,
	[2] = IRQ_EINT5,
	[3] = IRQ_EINT6,
	[4] = IRQ_EINT7,
};

static inline unsigned int matrix_read_gpio(void)
{
	unsigned int ret;

	/* invert home key */
	ret  = s3c2410_gpio_getpin(S3C2410_GPF3) ? 0 : (1 << 0);
	ret |= s3c2410_gpio_getpin(S3C2410_GPF4) ? (1 << 1) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF5) ? (1 << 2) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF6) ? (1 << 3) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF7) ? (1 << 4) : 0;

	return ret;
}

static void matrix_scan(unsigned long data)
{
	unsigned int debounced_state;
	int i;

	if (++index >= ARRAY_SIZE(state)) {
		index = 0;
	}

	state[index] = 0;
	for (i = 0; i < 3; i++) {
		/* drive lines Low or Hi-Z */
		s3c2410_gpio_cfgpin(S3C2410_GPF0, (i == 0) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);
		s3c2410_gpio_cfgpin(S3C2410_GPF1, (i == 1) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);
		s3c2410_gpio_cfgpin(S3C2410_GPF2, (i == 2) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);

		state[index] <<= 5;
		state[index] |= matrix_read_gpio();
	}

	/* ensure all lines are back to Low */
	s3c2410_gpio_cfgpin(S3C2410_GPF0, S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF1, S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF2, S3C2410_GPIO_OUTPUT);

	/* debounce switches */
	debounced_state = 0xFFFFFFFF;
	for (i = 0; i < ARRAY_SIZE(state); i++) {
		debounced_state &= state[i];
	}

	if (last_state != debounced_state) {
		for (i = 0; i < ARRAY_SIZE(keymap); i++) {
			unsigned int mask = (1 << i);

			if (keymap[i] != KEY_UNKNOWN
			    && ((debounced_state & mask) != (last_state & mask))) {
				input_report_key(matrix_dev, keymap[i], (debounced_state & mask) > 0);
			}
		}
		input_sync(matrix_dev);
		
		last_state = debounced_state;
	}

	if (state[index]) {
		/* keep scanning while keys are pressed */
		mod_timer(&matrix_timer, jiffies + (DEBOUNCE_PERIOD_MS * HZ/1000));
	}
	else {
		/* return the lines to interrupt */
		s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPIO_SFN2);
		s3c2410_gpio_cfgpin(S3C2410_GPF4, S3C2410_GPIO_SFN2);
		s3c2410_gpio_cfgpin(S3C2410_GPF5, S3C2410_GPIO_SFN2);
		s3c2410_gpio_cfgpin(S3C2410_GPF6, S3C2410_GPIO_SFN2);
		s3c2410_gpio_cfgpin(S3C2410_GPF7, S3C2410_GPIO_SFN2);
	}
}

static irqreturn_t matrix_irq(int irq, void *dev_id) 
{
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF4, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF5, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF6, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF7, S3C2410_GPIO_INPUT);

	/* scan matrix */
	mod_timer(&matrix_timer, jiffies); // now

	return IRQ_HANDLED;
}		


static int __init matrix_init(void)
{
	int gpio;
	int ret;
	int i;

	matrix_dev = input_allocate_device();
	if (matrix_dev == NULL) {
		printk(KERN_ERR "matrix: Failed to allocate input device\n");
		return -ENOMEM;
	}
	
	matrix_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
	matrix_dev->name = "Matrix";
	
	for (i=0; i < ARRAY_SIZE(keymap); i++) {
		if (keymap[i] != KEY_UNKNOWN) {
			set_bit(keymap[i], matrix_dev->keybit);
		}
	}

	input_register_device(matrix_dev);

	/* Key debounce state */

	index = 0;
	last_state = 0;
	for (i = 0; i < ARRAY_SIZE(state); i++) {
		state[i] = 0;
	}

	init_timer(&matrix_timer);
	matrix_timer.function = matrix_scan;

	/* Set GPF0 through GPF2 low */

	for (gpio = S3C2410_GPF0; gpio <= S3C2410_GPF2; gpio++) {
		s3c2410_gpio_setpin(gpio, 0);
		s3c2410_gpio_cfgpin(gpio, S3C2410_GPIO_OUTPUT);
	}

	/* Remove pull-down on all pins, either outputs or are being
	 * driven from an external source. */

	for (gpio = S3C2410_GPF0; gpio <= S3C2410_GPF7; gpio++) {
		s3c2410_gpio_pullup(gpio, 1);
	}

	for (gpio = 0; gpio < ARRAY_SIZE(matrix_irqs); gpio++) {
		int irq = matrix_irqs[gpio];

		ret = request_irq(irq, matrix_irq, IRQ_FLAGS,
				  "keymatrix", NULL);
		if (ret < 0) {
			printk(KERN_ERR "Failed to request IRQ %d (error %d)\n", gpio, ret);
			goto err;
		}

		ret = set_irq_wake(irq, 1);
		if (ret < 0) {
			printk(KERN_ERR "Failed to set wake on IRQ %d (error %d)\n",
			       gpio, ret);
			goto err;
		}

		s3c2410_gpio_irqfilter(s3c2410_gpio_irq2pin(irq), 1, 0);
	}

	printk(KERN_INFO "%s: GPFCON=%08x\n", __func__, __raw_readl(S3C2410_GPFCON));
  	return 0;

 err:
	for (; gpio >= 0; gpio--) {
		int irq = matrix_irqs[gpio];

		set_irq_wake(irq, 0);
		free_irq(irq, NULL);
	}

	input_unregister_device(matrix_dev);
	return -EIO;
}


static void __exit matrix_exit(void)
{
	unsigned int gpio;

	for (gpio = 0; gpio < ARRAY_SIZE(matrix_irqs); gpio++) {
		int irq = matrix_irqs[gpio];

		set_irq_wake(irq, 0);
		free_irq(irq, NULL);
	}

	input_unregister_device(matrix_dev);
}


module_init(matrix_init);
module_exit(matrix_exit);

MODULE_DESCRIPTION("Jive matrix driver");
MODULE_AUTHOR("Logitech");
MODULE_LICENSE("GPL");
