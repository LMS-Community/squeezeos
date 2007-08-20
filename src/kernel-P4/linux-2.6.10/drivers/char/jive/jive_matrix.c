#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/i2o.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <asm/arch-s3c2413/regs-gpio.h>


#define COLUMNS 3
#define ROWS 5
#define KEYBIT(array, column, row) ((array)[(column)] & (0x08 << (row)))


static struct input_dev matrix_dev;
static unsigned char old_value[COLUMNS];
static int fixme_home = 0;


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



static irqreturn_t Matrix_int(int irq, void *dev_id, struct pt_regs *regs) {
	unsigned char line_value[ROWS];
	int i, j;

	disable_irq(IRQ_EINT3);
	disable_irq(IRQ_EINT4);
	disable_irq(IRQ_EINT5);
	disable_irq(IRQ_EINT6);
	disable_irq(IRQ_EINT7);


	// GPF[0:2] => output GPF[3:7] <= input
	__raw_writel(0x0015, S3C2413_GPFCON);


	// without this delay we miss some key up events
	// FIXME experiment with different values here
	udelay(10);

	
	// Read matrix
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xF8);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x06);
	line_value[0] = __raw_readl(S3C2413_GPFDAT)&0xF8;
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xF8);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x05);
	line_value[1] = __raw_readl(S3C2413_GPFDAT)&0xF8;
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xF8);
	(*(volatile unsigned int __force *)S3C2413_GPFDAT |= 0x03);
	line_value[2] = __raw_readl(S3C2413_GPFDAT)&0xF8;


	// FIXME on P4 board with no PIC we need to invert this
	switch (fixme_home) {
	case 0:
		if (line_value[0] & 0x08) {
			printk("WARNING PIC missing or not programmed\n");
			fixme_home = 1;
		}
		else {
			fixme_home = 2;
		}

	case 1:
		line_value[0] ^= 0x08;
	}


	// scan key matrix
	for (i=0; i<COLUMNS; i++) {
		for (j=0; j<ROWS; j++) {
			int key = keymap[ (i * ROWS) + j ];
			if (key != KEY_UNKNOWN
			    && (KEYBIT(line_value, i, j) ^ KEYBIT(old_value, i, j)) != 0) {
				input_report_key(&matrix_dev, key, (KEYBIT(line_value, i, j) > 0));
				input_sync(&matrix_dev);
			}
		}

		old_value[i] = line_value[i];
	}


	// set GPF[0:2] low
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xF8);

	// GPF[0:2] => output GPF[3:7] <= interrupt
	__raw_writel(0xAA95, S3C2413_GPFCON);

	enable_irq(IRQ_EINT3);
	enable_irq(IRQ_EINT4);
	enable_irq(IRQ_EINT5);
	enable_irq(IRQ_EINT6);
	enable_irq(IRQ_EINT7);

	return IRQ_HANDLED;
}		


static int __init matrix_init(void)
{
	int i;
	
	matrix_dev.evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
	matrix_dev.name = "Matrix";
	
	for (i=0; i<ROWS * COLUMNS; i++) {
		if (keymap[i] != KEY_UNKNOWN) {
			set_bit(keymap[i], matrix_dev.keybit);
		}
	}
	input_register_device(&matrix_dev);

      
	// GPF[0:2] => output GPF[3:7] <= interrupt
	__raw_writel(0xAA95, S3C2413_GPFCON);

	// GPF[3:7] pull-down disabled
	__raw_writel(0x00F8, S3C2413_GPFDN);


	// Set interrupts for EINT[3:7]
	(*(volatile unsigned int __force *)S3C2413_EXTINT0 &=0x00000FFF); // Both edges
	(*(volatile unsigned int __force *)S3C2413_EXTINT0 |=0x77677000);
	(*(volatile unsigned int __force *)S3C2413_EXTINT0 |=0x88888000); // Filtered
	

	// set GPF[0:2] low
	(*(volatile unsigned int __force *)S3C2413_GPFDAT &= 0xF8);


	disable_irq(IRQ_EINT3);
	disable_irq(IRQ_EINT4);
	disable_irq(IRQ_EINT5);
	disable_irq(IRQ_EINT6);
	disable_irq(IRQ_EINT7);


	if (request_irq(IRQ_EINT3, Matrix_int, SA_SAMPLE_RANDOM,"Matrix_int", NULL)) {
		printk(KERN_ERR "Could not allocate matrix IRQ !\n");
		return -EIO;
	}
	if (request_irq(IRQ_EINT4, Matrix_int, SA_SAMPLE_RANDOM,"Matrix_int", NULL)) {
		printk(KERN_ERR "Could not allocate matrix IRQ !\n");
		return -EIO;
	}
	if (request_irq(IRQ_EINT5, Matrix_int, SA_SAMPLE_RANDOM,"Matrix_int", NULL)) {
		printk(KERN_ERR "Could not allocate matrix IRQ !\n");
		return -EIO;
	}
	if (request_irq(IRQ_EINT6, Matrix_int, SA_SAMPLE_RANDOM,"Matrix_int", NULL)) {
		printk(KERN_ERR "Could not allocate matrix IRQ !\n");
		return -EIO;
	}
	if (request_irq(IRQ_EINT7, Matrix_int, SA_SAMPLE_RANDOM,"Matrix_int", NULL)) {
		printk(KERN_ERR "Could not allocate matrix IRQ !\n");
		return -EIO;
	}

  	return 0;
}


static void __exit matrix_exit(void)
{
	free_irq(IRQ_EINT3, NULL);
	free_irq(IRQ_EINT4, NULL);
	free_irq(IRQ_EINT5, NULL);
	free_irq(IRQ_EINT6, NULL);
	free_irq(IRQ_EINT7, NULL);

	input_unregister_device(&matrix_dev);
}


module_init(matrix_init);
module_exit(matrix_exit);

MODULE_DESCRIPTION("Jive matrix driver");
MODULE_AUTHOR("Logitech");
MODULE_LICENSE("GPL");
