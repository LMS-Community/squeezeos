//===================================================================
// 가속도 센서  컨트롤 드라이버
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
#include <linux/delay.h>
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

void ACCEL_Reg_Init(void);
void ACCEL_Disable(void);
void ACCEL_Enable(void);
void ACCEL_Reg_Read(void);
unsigned char   __write_swiic(unsigned char slv_addr, unsigned char sub_addr, unsigned char *wr_ptr, unsigned char wr_length);
unsigned char   __read_swiic(unsigned char slv_addr, unsigned char sub_addr, unsigned char *rd_ptr, unsigned char wr_length);

#define	IIC_NG 1
#define	IIC_OK 0
#define	DELAY 10
#define	 SlaveADDr	0x38
// GPH4 : SDA
#define	ACCEL_SDA_HIGH	{ unsigned long gpdat;	\
													gpdat = __raw_readl(S3C2413_GPHDAT); \
					  							gpdat |= (0x1<<4); \
					  							__raw_writel(gpdat, S3C2413_GPHDAT); \
												}

#define	ACCEL_SDA_LOW		{ unsigned long gpdat;	\
													gpdat = __raw_readl(S3C2413_GPHDAT); \
					  							gpdat &= ~(0x1<<4); \
					  							__raw_writel(gpdat, S3C2413_GPHDAT); \
												}

// GPH5 : SCL
#define	ACCEL_SCL_HIGH		{ unsigned long gpdat;	\
													gpdat = __raw_readl(S3C2413_GPHDAT); \
					  							gpdat |= (0x1<<5); \
					  							__raw_writel(gpdat, S3C2413_GPHDAT); \
												}

#define	ACCEL_SCL_LOW		{ unsigned long gpdat;		\
													gpdat = __raw_readl(S3C2413_GPHDAT); \
					  							gpdat &= ~(0x1<<5); \
					  							__raw_writel(gpdat, S3C2413_GPHDAT); \
												}

//===================================================================
int ACCEL_MAJOR = 241;

//===================================================================
int ACCEL_open (struct inode *inode, struct file *filp)
{
	// SCL
	(*(volatile unsigned int __force *)S3C2413_GPHCON &= ~(0x3<<10));	//GPH5
	(*(volatile unsigned int __force *)S3C2413_GPHCON |=(0x1<<10));	//GPH5 Output
	(*(volatile unsigned int __force *)S3C2413_GPHDAT |=(1<<5));		//GPH5 High
	(*(volatile unsigned int __force *)S3C2413_GPHDN &= ~(1<<5));		//GPH5 Pull Down enable

	// SDA
	(*(volatile unsigned int __force *)S3C2413_GPHCON &= ~(0x3<<8));	//GPH4
	(*(volatile unsigned int __force *)S3C2413_GPHCON |=(0x1<<8));	//GPH4 Output
	(*(volatile unsigned int __force *)S3C2413_GPHDAT |=(1<<4));		//GPH4 High
	(*(volatile unsigned int __force *)S3C2413_GPHDN &= ~(1<<4));		//GPH4 Pull Down enable

	ACCEL_Reg_Init();
	//ACCEL_Reg_Read();


  return 0;          /* success */
}

void ACCEL_Reg_Read(void)
{
	unsigned char data[1], i;

	__read_swiic(SlaveADDr,0x20,data,1);
	printk("cfg1 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x21,data,1);
	printk("cfg2 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x22,data,1);
	printk("cfg3 = 0x%x \n", data[0]);

	__read_swiic(SlaveADDr,0x30,data,1);
	printk("ff_wu_cfg1 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x32,data,1);
	printk("ff_wu_ths1 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x33,data,1);
	printk("ff_wu_duration1 = 0x%x \n", data[0]);

	__read_swiic(SlaveADDr,0x34,data,1);
	printk("ff_wu_cfg2 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x36,data,1);
	printk("ff_wu_ths2 = 0x%x \n", data[0]);
	__read_swiic(SlaveADDr,0x37,data,1);
	printk("ff_wu_duration2 = 0x%x \n", data[0]);

		__read_swiic(SlaveADDr,0x0f,data,1);
		printk("addr(0x%x) = 0x%x \n", 0x0f, data[0]);
	for(i=0x20;i< 0x40;i++)
	{
		__read_swiic(SlaveADDr,i,data,1);
		printk("addr(0x%x) = 0x%x \n", i, data[0]);

	}
}

int ACCEL_release (struct inode *inode, struct file *filp)
{
   	return 0;
}

//===================================================================
ssize_t ACCEL_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char data[1];

	copy_from_user(data, buf, count);


	switch (data[0])
	{
		case 0x00:			//  accel disable
			ACCEL_Disable();
			break;
		case 0x01:			// accel enable
 			ACCEL_Enable();
			break;

		default:
			break;
	}
	return(count);
}

//===================================================================
struct file_operations ACCEL_fops = {
    	write:    	ACCEL_write,
    	open:    	ACCEL_open,
    	release:  	ACCEL_release,
};

//===================================================================
static int __init ACCEL_init(void)
{
    	int result;

    	result = register_chrdev(ACCEL_MAJOR, "ACCEL", &ACCEL_fops);
    	if (result < 0) 	{
        	printk(KERN_WARNING " can't get major \n");
        	return result;
	}

    	printk("[ACCEL]ACCEL_MAJOR = %d\n", ACCEL_MAJOR);
    	return 0;
}

static void __exit ACCEL_exit(void)
{
	unregister_chrdev(ACCEL_MAJOR, "ACCEL");
}

// GPH4
void	SDAPinOutput(void)
{
	(*(volatile unsigned int __force *)S3C2413_GPHCON) &= ~(0x3<<8); 		//GPH4
	(*(volatile unsigned int __force *)S3C2413_GPHCON) |=(0x1<<8);			//GPH4 Output
}

void	SDAPinInput(void)
{
		(*(volatile unsigned int __force *)S3C2413_GPHCON) &= ~(0x3<<8);	//GPH4	Input
}


unsigned char	ACCEL_SDA_READ(void)
{
	unsigned long temp;

	temp = __raw_readl(S3C2413_GPHDAT);
	temp = ((temp  & 0x10)>>4);			//GPH4

	return ( (unsigned char)temp );
}

//------------------------------------------------------------------------------------------------------
void    _iic_start( void )
{

	SDAPinOutput();

	ACCEL_SDA_HIGH;
	ACCEL_SCL_HIGH;
	udelay(DELAY);

	ACCEL_SDA_LOW;
	udelay(DELAY);
	ACCEL_SCL_LOW;
	udelay(DELAY);
}

//------------------------------------------------------------------------------------------------------
void   _iic_stop( void )
{

	SDAPinOutput();
	ACCEL_SDA_LOW;
	udelay(DELAY);
	ACCEL_SCL_HIGH;
	udelay(DELAY);
	ACCEL_SDA_HIGH;
}

//------------------------------------------------------------------------------------------------------
void   _iic_ack( void )
{

	ACCEL_SCL_LOW;

	SDAPinOutput();
	ACCEL_SDA_LOW;

	ACCEL_SCL_HIGH;
	udelay(DELAY);
	ACCEL_SCL_LOW;
}


//------------------------------------------------------------------------------------------------------
void   _iic_nack( void )
{

	ACCEL_SCL_LOW;

	SDAPinOutput();
	ACCEL_SDA_HIGH;

	ACCEL_SCL_HIGH;
	udelay(DELAY);
	ACCEL_SCL_LOW;
}

//------------------------------------------------------------------------------------------------------
unsigned char   _iic_8bit_Tx( unsigned char iic_data)
{
    /* MSB */
	unsigned char  i = 0x80;
	unsigned char  iic_tmp;

	SDAPinOutput();

	do
	{
	/***************************************************
	*    Data 8Bits
	****************************************************/

		if(iic_data & i) 	{ACCEL_SDA_HIGH;}
		else							{ACCEL_SDA_LOW;}

		i = i>>1;
		udelay(DELAY);

		ACCEL_SCL_HIGH;
		udelay(DELAY);
		ACCEL_SCL_LOW;
		udelay(DELAY*2);
	}while(i);
      /***************************************************
      *    Ack Check
      ****************************************************/
	ACCEL_SDA_HIGH;
	SDAPinInput();
	udelay(DELAY);
	ACCEL_SCL_HIGH;
	udelay(DELAY);

	iic_tmp = ACCEL_SDA_READ();		//port_read(P_EEP_SDA);

	ACCEL_SCL_LOW;
	udelay(DELAY);

 	return(iic_tmp);
}

//------------------------------------------------------------------------------------------------------
unsigned char   _iic_8bit_Rx( void )
{
    /* MSB */
    unsigned char  i = 0x80;
    unsigned char  rxbuf = 0;


    SDAPinInput();

    do
    {

			ACCEL_SCL_HIGH;
			udelay(DELAY);

      if(ACCEL_SDA_READ())	rxbuf = rxbuf | i;

			ACCEL_SCL_LOW;
     	i = i>>1;
			udelay(DELAY);

    }while(i);

    return(rxbuf);
}

//------------------------------------------------------------------------------------------------------
unsigned char  __iic_page_read(unsigned char slv_addr, unsigned char sub_addr, unsigned char *wr_ptr, unsigned char wr_length)
{

	_iic_start();

	if(_iic_8bit_Tx( slv_addr ) == IIC_NG ) return(IIC_NG);

	if(_iic_8bit_Tx( sub_addr ) == IIC_NG ) return(IIC_NG);

	_iic_start();

	if(_iic_8bit_Tx( slv_addr+ 1 ) == IIC_NG ) return(IIC_NG);


	 while(wr_length--){

		*wr_ptr = _iic_8bit_Rx();
		(wr_length) ? _iic_ack(): _iic_nack();
		wr_ptr += 1 ;
	}

	_iic_stop();

	return(IIC_OK);

}

//------------------------- Driver -----------------------------------------------------------------------
unsigned char  __iic_page_write(unsigned char slv_addr, unsigned char sub_addr, unsigned char *wr_ptr, unsigned char wr_length)
{

	_iic_start();

	if(_iic_8bit_Tx( slv_addr ) == IIC_NG) return(IIC_NG);

	if(_iic_8bit_Tx( sub_addr ) == IIC_NG) return(IIC_NG);

	do{
	  	if(_iic_8bit_Tx(*wr_ptr ) == IIC_NG) return(IIC_NG);
	  	wr_ptr += 1 ;

	}while(--wr_length);

	_iic_stop();

	return(IIC_OK);
}

//------------------------------------------------------------------------------------------------------
unsigned char   __read_swiic(unsigned char slv_addr, unsigned char sub_addr, unsigned char *rd_ptr, unsigned char wr_length)
{
  	unsigned char  retryCnt = 3;
  	unsigned char  iic_tmp[16];
  	unsigned char  ack;

 	do{
	    	ack = __iic_page_read(slv_addr, sub_addr, iic_tmp, wr_length);

		switch(ack){
			case  IIC_OK:
				for( ;  0 < wr_length; wr_length--)		*(rd_ptr + (wr_length-1)) = iic_tmp[wr_length-1];
				return(ack);
			default:      		//IIC_NG,IIC_SHORT
				_iic_stop();
				break;
		}
  	}while(--retryCnt);

  	return( ack );
}

//----------------- Implementation ---------------------------------------------------------------------
unsigned char   __write_swiic(unsigned char slv_addr, unsigned char sub_addr, unsigned char *wr_ptr, unsigned char wr_length)
{
		unsigned char retryCnt = 3;
  	unsigned char ack;

  	do{
   	 		ack = __iic_page_write(slv_addr, sub_addr, wr_ptr, wr_length);

	    	switch(ack)
	    	{
	      		case  IIC_OK:
	        		return(ack);
	      		default:      //IIC_NG,IIC_SHORT
	        		_iic_stop();
	        		break;
	    	}
  	}while(--retryCnt);

  	return( ack );
}

void ACCEL_Reg_Init(void)
{
	unsigned char data[1], sub_addr;

	sub_addr = 0x20;	 // Ctrl_reg1
	data[0] = 0x47;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x21;		// Ctrl_reg2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x22;		// Ctrl_reg3
	data[0] = 0x11;
	__write_swiic(SlaveADDr,sub_addr , data, 1);


	sub_addr = 0x30;		// FF_WU_CFG_1
	//data[0] = 0x1a;
	data[0] = 0x05;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x32;		// FF_WU_SRC_1
	data[0] = 0xa;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x33;		// FF_WU_DURATION_1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x34;		// FF_WU_CFG_2
	//data[0] = 0x1a;
	data[0] = 0x05;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x36;		// FF_WU_SRC_2
	data[0] = 0xa;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x37;		// FF_WU_DURATION_2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);

}

void ACCEL_Enable(void)
{
	unsigned char data[1], sub_addr;

	sub_addr = 0x20;	 // Ctrl_reg1
	data[0] = 0x47;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x21;		// Ctrl_reg2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x22;		// Ctrl_reg3
	data[0] = 0x11;
	__write_swiic(SlaveADDr,sub_addr , data, 1);


	sub_addr = 0x30;		// FF_WU_CFG_1
	//data[0] = 0x1a;
	data[0] = 0x0a;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x32;		// FF_WU_THS_1
	data[0] = 0xa;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x33;		// FF_WU_DURATION_1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x34;		// FF_WU_CFG_2
	//data[0] = 0x1a;
	data[0] = 0x0a;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x36;		// FF_WU_THS_2
	data[0] = 0xa;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x37;		// FF_WU_DURATION_2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);



}

void ACCEL_Disable(void)
{
	unsigned char data[1], sub_addr;

	sub_addr = 0x20;	 // Ctrl_reg1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0;		// Ctrl_reg2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x22;		// Ctrl_reg3
	data[0] = 0x11;
	__write_swiic(SlaveADDr,sub_addr , data, 1);


	sub_addr = 0x30;		// FF_WU_CFG_1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x32;		// FF_WU_SRC_1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x33;		// FF_WU_DURATION_1
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x34;		// FF_WU_CFG_2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x36;		// FF_WU_SRC_2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);
	sub_addr = 0x37;		// FF_WU_DURATION_2
	data[0] = 0;
	__write_swiic(SlaveADDr,sub_addr , data, 1);


}
//===================================================================
module_init(ACCEL_init);
module_exit(ACCEL_exit);

MODULE_LICENSE("GPL");

