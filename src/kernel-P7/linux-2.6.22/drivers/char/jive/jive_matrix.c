
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/i2o.h>
#include <linux/input.h>
#include <linux/interrupt.h>

#include <asm/hardware.h>
#include <asm/arch/regs-gpio.h>


#define COLUMNS 3
#define ROWS 5
#define KEYBIT(array, column, row) ((array)[(column)] & (0x08 << (row)))


static struct input_dev *matrix_dev;
static unsigned char old_value[COLUMNS];


static unsigned int keymap[] = {
	// Columns are in this order:
	//  KEY_INPUT4
	//  KEY_INPUT0
	//  KEY_INPUT1
	//  KEY_INPUT2
	//  KEY_INPUT3

	   
	// KEY_OUTPUT0
	KEY_H, // SW1 Home
	KEY_UNKNOWN,
	KEY_LEFT, // SW2 Back
	KEY_MINUS, // SW3 Volume Down
	KEY_RIGHT, // SW16 Go

	// KEY_OUTPUT1
	KEY_UNKNOWN,
	KEY_X, // SW5 Play
	KEY_A, // SW6 Add
	KEY_EQUAL, // SW7 Volume Up
	KEY_UNKNOWN,

	// KEY_OUTPUT2
	KEY_UNKNOWN,
	KEY_B, // SW10 Fwd
	KEY_Z, // SW9 Rew
	KEY_C, // SW8 Pause
	KEY_UNKNOWN,
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

	ret  = s3c2410_gpio_getpin(S3C2410_GPF3) ? (1 << 3) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF4) ? (1 << 4) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF5) ? (1 << 5) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF6) ? (1 << 6) : 0;
	ret |= s3c2410_gpio_getpin(S3C2410_GPF7) ? (1 << 7) : 0;

	return ret;
}

static irqreturn_t matrix_irq(int irq, void *dev_id) 
{
	unsigned char line_value[ROWS];
	int line;
	int i, j;

	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF4, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF5, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF6, S3C2410_GPIO_INPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF7, S3C2410_GPIO_INPUT);

	// without this delay we miss some key up events
	udelay(1);

	for (line = 0; line < 3; line++) {
		/* drive lines Low or Hi-Z */
		s3c2410_gpio_cfgpin(S3C2410_GPF0, (line == 0) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);
		s3c2410_gpio_cfgpin(S3C2410_GPF1, (line == 1) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);
		s3c2410_gpio_cfgpin(S3C2410_GPF2, (line == 2) ? S3C2410_GPIO_OUTPUT : S3C2410_GPIO_INPUT);

		line_value[line] = matrix_read_gpio();
	}

	/* ensure all lines are back to Low */
	s3c2410_gpio_cfgpin(S3C2410_GPF0, S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF1, S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_cfgpin(S3C2410_GPF2, S3C2410_GPIO_OUTPUT);

	/* return the lines to interrupt */
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPIO_SFN2);
	s3c2410_gpio_cfgpin(S3C2410_GPF4, S3C2410_GPIO_SFN2);
	s3c2410_gpio_cfgpin(S3C2410_GPF5, S3C2410_GPIO_SFN2);
	s3c2410_gpio_cfgpin(S3C2410_GPF6, S3C2410_GPIO_SFN2);
	s3c2410_gpio_cfgpin(S3C2410_GPF7, S3C2410_GPIO_SFN2);

	printk(KERN_DEBUG "%s: read %02x,%02x,%02x\n", __func__,
	       line_value[0], line_value[1], line_value[2]);
	
	// invert home key
	line_value[0] ^= 0x08;

	// scan key matrix
	for (i=0; i<COLUMNS; i++) {
		for (j=0; j<ROWS; j++) {
			int key = keymap[ (i * ROWS) + j ];
			if (key != KEY_UNKNOWN
			    && (KEYBIT(line_value, i, j) ^ KEYBIT(old_value, i, j)) != 0) {
				input_report_key(matrix_dev, key, (KEYBIT(line_value, i, j) > 0));
			}
		}

		old_value[i] = line_value[i];
	}

	input_sync(matrix_dev);

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
	
	for (i=0; i < (ROWS * COLUMNS); i++) {
		if (keymap[i] != KEY_UNKNOWN) {
			set_bit(keymap[i], matrix_dev->keybit);
		}
	}

	input_register_device(matrix_dev);


#if 0
	(*(volatile unsigned int __force *)S3C2413_EXTINT0 |=0x88888000); // Filtered
#endif	

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
