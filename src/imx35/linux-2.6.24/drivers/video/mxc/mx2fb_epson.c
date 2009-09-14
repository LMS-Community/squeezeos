#error "please port this file to linux 2.6.18"
/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!@example slcdctest.c test driver using EPSON L2F50032T00 serial mode */

/**
 *
 * @defgroup SLCDC 			SLCDC driver
 **/
/**@{*/
/**
 * @file   mx2fb_epson.c
 * @brief   slcdc driver source code
 *
 * This is the basic release for SLCDC driver, which is to support EPSON
 * L2F50032T00 16bit seria mode5-6-5 and parallel mode. This driver acts as a
 * standard character device, which can be dinamically loaded.
 * For example, you can refer to slcdctest.c file.
 *
 * Modification History:
 * 15,Dec,2003 Karen Kang
 *
 * 21,Feb,2006 Vasanthan S
 *
 * @bug
 **/

#ifndef __KERNEL__
#  define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/pm.h>

#include "../console/fbcon.h"
#include "../../mxc/mempool/mempool.h"
#include <linux/timer.h>

#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>	/* get_user,copy_to_user */
#include <asm/irq.h>
#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/mx27.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <linux/devfs_fs_kernel.h>

#ifdef CONFIG_ARCH_MX2ADS
#include <asm/arch/mx2.h>
#endif

#define MODULE_NAME "slcdc"

#define DBMX_DEBUG 1
#ifdef DBMX_DEBUG
#define TRACE(fmt, args...) \
	{ \
		printk("\n %s:%d:%s:",__FILE__, __LINE__,__FUNCTION__); \
		printk(fmt, ## args);\
	}
#else
#define TRACE(fmt, args...)
#endif

#define FAILED(fmt, args...) \
	{ \
		printk("\n %s:%d:%s:",__FILE__, __LINE__,__FUNCTION__); \
		printk(fmt, ## args);\
	}

#define INFO(fmt, args...) \
	{ \
		printk("\n"); \
		printk(fmt, ## args);\
	}

/*@brief definition for SLCDC_LCD_TXCONFIG */
#define SLCDC_IMG_8L	0x00020000

/*@name SLCDC connection configuration
 *@brief config macro for serial mode or parallel mode. SLCDC can operate in
 *       serial or parallel mode. This macro reflects the hardware configuration
 *       and is not a software configuration.
 */
/**@{*/
#define SLCDC_SERIAL_MODE
/* #define SLCDC_PARALLE_MODE */
/**@}*/

/*@name SLCDC IO MUX configuration
 *@brief configuration macro for the pin mux detail. This tells which pins are
 *       configured for SLCDC. For more information refer processor data sheet.
 *       Most cases only one of the following macros should be enabled.
 */
/**@{*/
#define SLCDC_MUX_SSI3_SLCDC2	/*!< 1. SLCDC is mux.ed with SSI3 */
 //#define SLCDC_MUX_SD_SLCDC1  /*!< 2. SLCDC is mux.ed with SD2 */
 //#define SLCDC_MUX_LCD_SLCDC1 /*!< 3. SLCDC is mux.ed with LCD */
/**@}*/

/*@brief configuration macro which tells whether the interrupt should be used
 *        or not for SLCDC. SLCDC will provide an interrupt on a completed
 *        transfer, which can be used for refreshing the data.
 */
#define USING_INTERRUPT_SLCDC

/**
 *@name ioctl command macro definitions
 *@brief definition for SLCDC ioctl cmd,these command definition is different
 *       for serial \nmode and paralel mode ,in this driver, we have not
 *       supported all these commands, \n please check the ioctl function for
 *       details.
 **/
/**@{*/
/* definition for SLCDC cmd */
/* same as Epson 10043 & 50052 */
#ifdef SLCDC_SERIAL_MODE
#define SLCDC_CMD_DISON			0xaf00	/*!< 1. display on */
#define SLCDC_CMD_DISOFF		0xae00	/*!< 2. display off */
#define SLCDC_CMD_DISNOR		0xa600	/*!< 3. normal display */
#define SLCDC_CMD_DISINV		0xa700	/*!< 4. inverse display */
#define SLCDC_CMD_DISCTL		0xca00	/*!< 5. display control */
#define SLCDC_CMD_SLPIN			0x9500	/*!< 10.sleep in */
#define SLCDC_CMD_SLPOUT		0x9400	/*!< 11.sleep out */
#define SLCDC_CMD_SD_PSET		0x7500	/*!< 12.page address set */
#define SLCDC_CMD_SD_CSET		0x1500	/*!< 14.column address set */

#define SLCDC_CMD_DATCTL		0xbc	/*!< 16.data scan direction, etc. */
#define SLCDC_CMD_RAMWR			0x5c00	/*!< 17.writing to memory */
#define SLCDC_CMD_PTLIN			0xa800	/*!< 19.partial display in */
#define SLCDC_CMD_PTLOUT		0xa900	/*!< 20.partial display out */

/* value different from 10043 but same as 50052 */
#define SLCDC_CMD_VOLCTR		0xc600	/*!< 25.Electronic volume control */

/* commands not found in 10043 but in 50052*/
#define SLCDC_CMD_GCP64			0xcb00	/*!< 6. 64 grayscale pulse positon set */
#define SLCDC_CMD_GCP16			0xcc00	/*!< 7. 16 grayscale pulse positon set */
#define SLCDC_CMD_GSSET			0xcd00	/*!< 8. grayscale set */
#define SLCDC_CMD_RAMRD			0x5d00	/*!< 18.memory read */
#define SLCDC_CMD_ASCSET		0xaa00	/*!< 21.area scroll set */
#define SLCDC_CMD_SCSTART		0xab00	/*!< 22.scroll start set */
#define SLCDC_CMD_EPCTIN		0x6100	/*!< 26.Power IC control for EVR */
#define SLCDC_CMD_EPCTOUT		0x6200	/*!< 27.Power IC control for EVR */

#else
#define SLCDC_CMD_DISON			0xaf	/*!<1. display on */
#define SLCDC_CMD_DISOFF		0xae	/*!<2. display off */
#define SLCDC_CMD_DISNOR		0xa6	/*!<3. normal display */
#define SLCDC_CMD_DISINV		0xa7	/*!<4. inverse display */
#define SLCDC_CMD_DISCTL		0xca	/*!<5. display control */
#define SLCDC_CMD_SLPIN			0x95	/*!<10.sleep in */
#define SLCDC_CMD_SLPOUT		0x94	/*!<11.sleep out */
#define SLCDC_CMD_SD_PSET		0x75	/*!<12.page address set */
#define SLCDC_CMD_SD_CSET		0x15	/*!<14.column address set */

#define SLCDC_CMD_DATCTL		0xbc	/*!<16.data scan direction, etc. */
//#define SLCDC_CMD_DATCTL              0xbc00  /*!<16.data scan direction, etc.*/
#define SLCDC_CMD_RAMWR			0x5c	/*!<17.writing to memory */
#define SLCDC_CMD_PTLIN			0xa8	/*!<19.partial display in */
#define SLCDC_CMD_PTLOUT		0xa9	/*!<20.partial display out */

/* value different from 10043 but same as 50052 */
#define SLCDC_CMD_VOLCTR		0xc6	/*!<25.Electronic volume control */

/* commands not found in 10043 but in 50052*/
#define SLCDC_CMD_GCP64			0xcb	/*!<6. 64 grayscale pulse positon set */
#define SLCDC_CMD_GCP16			0xcc	/*!<7. 16 grayscale pulse positon set */
#define SLCDC_CMD_GSSET			0xcd	/*!<8. grayscale set */
#define SLCDC_CMD_RAMRD			0x5d	/*!<18.memory read */
#define SLCDC_CMD_ASCSET		0xaa	/*!<21.area scroll set */
#define SLCDC_CMD_SCSTART		0xab	/*!<22.scroll start set */
#define SLCDC_CMD_EPCTIN		0x61	/*!<26.Power IC control for EVR */
#define SLCDC_CMD_EPCTOUT		0x62	/*!<27.Power IC control for EVR */
#endif

/**@}*/

#define SLCDC_IRQ	INT_SLCDC
#define SLCDC_CMD_MEM_SIZE		4
#define SLCDC_WIDTH				176
#define SLCDC_HIGH				220
#define SLCDC_BPP				16
#define SLCDC_PIXEL_MEM_SIZE	(SLCDC_WIDTH*SLCDC_HIGH*SLCDC_BPP)/8
#define SLCDC_MEM_SIZE  (SLCDC_WIDTH*SLCDC_HIGH)
#define _SLCDC_DATA_SIZE_		(SLCDC_PIXEL_MEM_SIZE + 32)
#define SLCDC_DATA_MEM_SIZE	\
                    ((unsigned)(PAGE_ALIGN(_SLCDC_DATA_SIZE_ + PAGE_SIZE * 2)))

//bit mask definition in STAT/CTRL register
#define SLCDC_TRANSFER_BUSY		0x4
#define SLCDC_TRANSFER_ERROR	0x10

//<<<<<< Global Variable
/*used for SLCDC data buffer */
__attribute__ ((aligned(4)))
u16 *g_slcdc_dbuffer_address;

/* physical address for SLCDC data buffer*/
__attribute__ ((aligned(4)))
u16 *g_slcdc_dbuffer_phyaddress;

/* used for SLCDC command buffer */
__attribute__ ((aligned(4)))
u16 *g_slcdc_cbuffer_address;

/* physical address for SLCDC command buffer */
__attribute__ ((aligned(4)))
u16 *g_slcdc_cbuffer_phyaddress;

static wait_queue_head_t slcdc_wait;
static int slcdc_device_num;
static int g_slcdc_status;

static int slcdc_major;
module_param(slcdc_major, int, 0444);
MODULE_PARM_DESC(slcdc_major,
		 "slcdc char device major number. If this number is"
		 " set to zero, then automatic major number allocation will be done");

static int slcdc_minor;
module_param(slcdc_minor, int, 0444);
MODULE_PARM_DESC(slcdc_minor, "slcdc char device minor number");

#define SLCDC_OPEN_STATUS		0x0001
#define SLCDC_SUSPEND_STATUS	0x0002

#define SLCDC_MAJOR_NUM 10	/*!< SLCDC char dev default major number */
#define SLCDC_MINOR_NUM 156	/*!< SLCDC char dev default minor number */

#define FBCON_HAS_CFB4
#define FBCON_HAS_CFB8
#define FBCON_HAS_CFB16

static struct cdev slcdc_dev;

extern void gpio_slcdc_active(int type);
extern void gpio_slcdc_inactive(int type);

int slcdc_open(struct inode *inode, struct file *filp);
int slcdc_release(struct inode *inode, struct file *filp);
static int slcdc_ioctl(struct inode *inode, struct file *filp,
		       u_int cmd, u_long arg);
static int slcdc_mmap(struct file *filp, struct vm_area_struct *vma);
static void __init _init_fbinfo(void);

typedef struct {
	u16 *screen_start_address;
	u16 *v_screen_start_address;
} slcdc_par_t;

slcdc_par_t slcdc_par;

struct file_operations g_slcdc_fops = {
      open:slcdc_open,
      release:slcdc_release,
      ioctl:slcdc_ioctl,
      mmap:slcdc_mmap,
};

//>>>>>> Global Variable

#ifdef DBMX_DEBUG
#define FUNC_START	TRACE(KERN_ERR"start of %s\n", __FUNCTION__);
#define FUNC_END	TRACE(KERN_ERR"end of %s\n", __FUNCTION__);
#else
#define FUNC_START
#define FUNC_END
#endif

#define RED		0xf00
#define GREEN	0xf0
#define BLUE	0x0f

/**@brief Local LCD controller parameters*/
struct slcdcfb_par {
	u_char *screen_start_address;	/*!< Screen Start Address */
	u_char *v_screen_start_address;	/*!< Virtul Screen Start Address */
	unsigned long screen_memory_size;	/*!< Screen memory size */
	unsigned int palette_size;	/*!<Palette size */
	unsigned int max_xres;	/*!<Maximum x resolution */
	unsigned int max_yres;	/*!<Maximum x resolution */
	unsigned int xres;	/*!<X resolution */
	unsigned int yres;	/*!<Y resolution */
	unsigned int xres_virtual;	/*!<Vitual x resolution */
	unsigned int yres_virtual;	/*!<Vitual y resolution */
	unsigned int max_bpp;	/*!<Maximum bit per pixel */
	unsigned int bits_per_pixel;	/*!<Bits per pixel */
	unsigned int currcon;	/*!<Current console ID */
	unsigned int visual;	/*!<Vitual color type */
	unsigned int TFT:1;	/*!<TFT flag */
	unsigned int color:1;	/*!<Color flag */
	unsigned int sharp:1;	/*!< Sharp LCD flag */
};

/* Frame buffer device API */
static int slcdcfb_set_var(struct fb_info *info);
/* perform fb specific mmap */
static int slcdcfb_mmap(struct fb_info *info, struct file *file,
			struct vm_area_struct *vma);
/* perform fb specific ioctl (optional) */
static int slcdcfb_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg,
			 struct fb_info *info);
static int slcdcfb_open(struct fb_info *info, int user);
static int slcdcfb_release(struct fb_info *info, int user);

static int _check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int _decode_var(struct fb_info *info);
static int slcdcfb_blank(int blank, struct fb_info *info);

/*
 *  Framebuffer file operations
 */
static struct fb_ops slcdcfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = slcdcfb_open,
	.fb_release = slcdcfb_release,
	.fb_check_var = _check_var,
	.fb_set_par = _decode_var,
	.fb_blank = slcdcfb_blank,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_cursor = soft_cursor,
	.fb_ioctl = slcdcfb_ioctl,
	.fb_mmap = slcdcfb_mmap,
};

static struct display global_disp;	/* Initial (default) Display Settings */
static struct fb_info slcdc_fb_info;
static struct fb_var_screeninfo init_var;
static struct slcdcfb_par current_par;
static struct timer_list slcdc_timer;
static int start_fb_timer_flag = 0;	//0 is stop, 1 is start

#define MIN_XRES        64
#define MIN_YRES        64
#define LCD_MAX_BPP		SLCDC_BPP
#define MAX_PIXEL_MEM_SIZE SLCDC_PIXEL_MEM_SIZE
#define SLCDC_REFRESH_RATE (HZ / 100)

#define SLCDC2_CLK_PC31 31	/* refer iMX27 Pin Mux details */
#define SLCDC2_CS_PC30  30
#define SLCDC2_RS_PC29  29
#define SLCDC2_D0_PC28  28

#define SLCDC1_CLK_PB5 5
#define SLCDC1_CS_PB8  8
#define SLCDC1_RS_PB7  7
#define SLCDC1_D0_PB6  6

#define SLCDC1_DAT0_PA6 6

/* Fake monspecs to fill in fbinfo structure */
static struct fb_monspecs monspecs __initdata = {
	.hfmin = 30000,
	.hfmax = 70000,
	.vfmin = 50,
	.vfmax = 65,
	.dpms = 0		/* Generic */
};

void slcdc_delay(int num)
{
	udelay(num);
}

/**
 *@brief slcdc gpio configure routine for serial mode
 *
 * Function Name: slcdc_gpio_serial
 *
 *
 * Description:This routine will implement gpio configurations for serial mode
 *             both for\n LCDC mux and SDHC2 mux, you can use macro
 *             SLCDC_MUX_SSI3_SLCDC2 or SLCDC_MUX_SD_SLCDC1 \n to choose the
 *             right way according to your hardware configuration.
 *
 *
 *@return		None
 *
 * Modification History:
 * 	Dec,2003			Karen update for MX21 TO2
 *  Jun,2004			Shirley update for LCDC mux
 *  Mar,2006            Update for MX27 mux
 **/

void slcdc_gpio_serial(void)
{
#ifdef SLCDC_MUX_SSI3_SLCDC2
	/* we have to set SLCDC2_CLK, SLCDC2_CS, SLCDC2_RS and SLCDC2_D0 */
	//gpio_request_mux(MX27_PIN_SSI3_CLK,  GPIO_MUX_ALT); /* CLK */
	//gpio_request_mux(MX27_PIN_SSI3_TXDAT,GPIO_MUX_ALT); /* CS  */
	//gpio_request_mux(MX27_PIN_SSI3_RXDAT,GPIO_MUX_ALT); /* RS  */
	//gpio_request_mux(MX27_PIN_SSI3_FS,   GPIO_MUX_ALT); /* D0  */
	gpio_slcdc_active(0);

#endif
#ifdef SLCDC_MUX_SD_SLCDC1
	//gpio_request_mux(MX27_PIN_SD2_D1, GPIO_MUX_GPIO); /* CLK */
	//gpio_request_mux(MX27_PIN_SD2_D2, GPIO_MUX_GPIO); /* D0  */
	//gpio_request_mux(MX27_PIN_SD2_D3, GPIO_MUX_GPIO); /* RS  */
	//gpio_request_mux(MX27_PIN_SD2_CMD,GPIO_MUX_GPIO); /* CS  */
	gpio_slcdc_active(1);

#endif
}

/**
 *@brief slcdc gpio configure routine for parallel mode
 *
 * Function Name: slcdc_gpio_paralle
 *
 *
 * Description:This routine will implement gpio configurations for parralel
 *             mode both for\n LCDC mux and SDHC2 mux, you can use macro
 *             SLCDC_MUX_SSI3_SLCDC2 or SLCDC_MUX_SD_SLCDC1 \n to choose the
 *             right way according to your hardware configuration.
 *
 *
 *@return		None
 *
 * Modification History:
 *  Jun,2004			Shirley update for LCDC mux
 *  Mar,2006            Update for MX27 mux
 **/
#ifndef SLCDC_MUX_SSI3_SLCDC2
void slcdc_gpio_paralle(void)
{
#ifdef SLCDC_MUX_LCD_SLCDC1
	/* Make sure the actual hardware connection is based on this, if other port
	   is used then the following code has to be modified accordingly.
	   For further details refer Pin Mux details in iMX27 Spec. */
	/* gpio_request_mux(MX27_PIN_LD0,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD1,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD2,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD3,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD4,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD5,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD6,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD7,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD8,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD9,  GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD10, GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD11, GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD12, GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD13, GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD14, GPIO_MUX_GPIO);
	   gpio_request_mux(MX27_PIN_LD15, GPIO_MUX_GPIO);
	 */
	gpio_slcdc_active(2);

#endif
}
#endif

void slcdc_reset(int level)
{
	if (level == 0) {
#if 0
#ifdef SLCDC_MUX_SSI3_SLCDC2
		/* OE_ACD as a reset pin */
		_reg_GPIO_GIUS(GPIOA) |= 0x80000000;
		_reg_GPIO_OCR2(GPIOA) |= 0xc0000000;
		_reg_GPIO_DDIR(GPIOA) |= 0x80000000;

		/* set reset pin to low */
		_reg_GPIO_DR(GPIOA) &= 0x7fffffff;
#endif
#ifdef SLCDC_MUX_SD_SLCDC1
		/* SD2_D1 as reset pin */
		_reg_GPIO_GIUS(GPIOB) |= 0x00000020;
		_reg_GPIO_OCR1(GPIOB) |= 0x00000c00;
		_reg_GPIO_DDIR(GPIOB) |= 0x00000020;

		/* set reset pin to low */
		_reg_GPIO_DR(GPIOB) &= 0xffffffdf;
#endif
	} else {
#ifdef SLCDC_MUX_SSI3_SLCDC2
		/* set reset pin to high */
		_reg_GPIO_DR(GPIOA) |= 0x80000000;
#endif
#ifdef SLCDC_MUX_SD_SLCDC1
		/* set reset pin to high */
		_reg_GPIO_DR(GPIOB) |= 0x00000020;
#endif
#endif
	}

}

/**
 *@brief slcdc hardware initialization
 *
 * Function Name: slcdc_init_dev
 *
 * Description  : This routine will enable the SLCDC and the clock for it
 *
 **/
void slcdc_init_dev(void)
{
	volatile unsigned long reg;

	reg = __raw_readl(IO_ADDRESS(MAX_BASE_ADDR + 0x100 * 3 + 0x10));
	reg = reg | 0x00040000;
	__raw_writel(reg, IO_ADDRESS(MAX_BASE_ADDR + 0x100 * 3 + 0x10));

	reg = __raw_readl(IO_ADDRESS(SYSCTRL_BASE_ADDR + 0x58));	/* _reg_SYS_PCSR */
	reg = reg | 0x00000004;	/* set LCD/SLCD bus master high priority */
	__raw_writel(reg, IO_ADDRESS(SYSCTRL_BASE_ADDR + 0x58));

	/* enable the slcd clk */
	reg = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR + 0x20));	/* _reg_CRM_PCCR0 */
	reg |= 0x02200000;
	__raw_writel(reg, IO_ADDRESS(CCM_BASE_ADDR + 0x20));
}

/**
 *@brief slcdc register initialization
 *
 * Function Name: slcdc_init_reg
 *
 * Description:This routine will setup the SLCDC register for first time
 *
 **/
void slcdc_init_reg(void)
{
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x04));	/* _reg_SLCDC_DBUF_SIZE */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x08));	/* _reg_SLCDC_CBADDR */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x0C));	/* _reg_SLCDC_CBUF_SIZE */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x10));	/* _reg_SLCDC_CBUF_SSIZE */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x14));	/* _reg_SLCDC_FIFO_CONFIG */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x18));	/* _reg_SLCDC_LCD_CONFIG */
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x1C));
						     /*_reg_SLCDC_LCD_TXCONFIG*/
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	__raw_writel(0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x24));
						    /*_reg_SLCDC_LCD_CLKCONFIG*/
}

/**
 *@brief slcdc initial configuration
 *
 * Function Name: slcdc_config
 *
 * Description:This routine will fist time configuration like buffer address
 *             FIFO configuration etc.
 *
 **/
void slcdc_config(int datlen)
{
	u32 xfrmode, sckpol, worddefcom, imgend = 0, worddefwrite =
	    0, worddefdat = 0;
	volatile unsigned long reg;

	if (datlen == 8) {
		imgend = 0x2;	/*8-bit little endian; */
		worddefdat = 0;	/* 8-bit data */
		worddefwrite = 10;
	} else if (datlen == 16) {
		imgend = 0x1;	/* 16-bit little endian; */
		worddefdat = 1;	/* 16-bit data */
		worddefwrite = 1;
	} else {
		FAILED(":invaild parameter, 8 or 16 is the value required");
	}
	worddefcom = 1;
#ifdef SLCDC_SERIAL_MODE
	xfrmode = 0;		/* serial mode */
#else
	xfrmode = 1;		/* paralle mode */
#endif
	sckpol = 1;		/* falling edge */
	/* config to be little endian serial 16bit */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x1C));
						       /*_reg_SLCDC_LCD_TXCONFIG*/
	reg =
	    (imgend << 16) | (worddefdat << 4) | (worddefcom << 3) | (xfrmode <<
								      2) |
	    (sckpol);
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x1C));

	/* printk("SLCDC_TXCONFIG = %x \n",_reg_SLCDC_LCD_TXCONFIG); */
	/* config dma setting */
	__raw_writel(5, IO_ADDRESS(SLCDC_BASE_ADDR + 0x14));	/* _reg_SLCDC_FIFO_CONFIG,
								   burst length is 4 32-bit words */
	/* config buffer address setting */
	__raw_writel((u32) (slcdc_par.screen_start_address), IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */
	__raw_writel((u32) g_slcdc_cbuffer_phyaddress, IO_ADDRESS(SLCDC_BASE_ADDR + 0x08));	/* _reg_SLCDC_CBADDR */

	/* config clk setting */
	__raw_writel((u32) 0x3, IO_ADDRESS(SLCDC_BASE_ADDR + 0x24));
						   /*_reg_SLCDC_LCD_CLKCONFIG*/
	/* set GO 0 */
	__raw_writel((u32) 0x0, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						   /*_reg_SLCDC_LCD_CTRL_STAT*/

	slcdc_delay(5000);

}

/**
 *@brief slcdc send command routine
 *
 * Function Name: slcdc_send_cmd
 *
 * Description:This help routine sends command to the SLCD from SLCDC
 *
 *@return		0 on success, any other value otherwise
 **/
/* for command transfer, it is very short, will not use interrupt */
int slcdc_send_cmd(u32 length)
{
	u32 status;
	volatile unsigned long reg;

	/* disable interrupt */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
					       /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg = reg & 0xffffff7f;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	/* set length */
	__raw_writel(length, IO_ADDRESS(SLCDC_BASE_ADDR + 0x04));	/* _reg_SLCDC_DBUF_SIZE */

	/* set automode 00 for command */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
	reg = reg & 0x000001ff;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	/* set GO */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
	reg |= 0x1;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	/* polling for data transfer finish */

	status = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
	while ((!(status & SLCDC_TRANSFER_ERROR))
	       && (status & SLCDC_TRANSFER_BUSY)) {
		status = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
	}

	if (status & SLCDC_TRANSFER_ERROR) {
		TRACE("send cmd error status=0x%x \n", status);
		return 1;
	} else {
		reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
		reg |= 0x40;
		__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

		reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));	/* _reg_SLCDC_LCD_CTRL_STAT */
		reg |= 0x20;
		__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
		return 0;
	}
}

/**
 *@brief slcdc send data routine
 *
 * Function Name: slcdc_send_data
 *
 * Description: This help routine sends data to the SLCD from SLCDC
 *
 *@return		0 on success, any other value otherwise
 **/
void slcdc_send_data(u32 length)
{
	volatile unsigned long reg;

	/* enable interrupt */
#ifdef USING_INTERRUPT_SLCDC
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= ~0xffffff7f;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
#endif
	/* set length */
	__raw_writel(length, IO_ADDRESS(SLCDC_BASE_ADDR + 0x04));	/* _reg_SLCDC_DBUF_SIZE */
	/* set automode 01 for data */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x00000800;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	/* set GO  */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x1;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

#ifdef USING_INTERRUPT_SLCDC
	interruptible_sleep_on(&slcdc_wait);
#else
	do {
		reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	} while ((reg & 0x00000004) != 0);

	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x40;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x20;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

#endif
	return;

}

/**
 *@brief slcdc isr
 *
 * Function Name: slcdc_isr
 *
 * Description: This ISR routine takes interrupt from SLCDC and does refresh of
 *              display data if necessary
 *
 *@return		0 on success, any other value otherwise
 **/
static irqreturn_t slcdc_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	volatile u32 reg;
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/

	/* clear interrupt */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x40;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	wake_up_interruptible(&slcdc_wait);

	if (start_fb_timer_flag == 1) {
		/*        while((_reg_SLCDC_LCD_CTRL_STAT &0x00000004)!=0);        */
		reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
		reg |= 0x40;
		__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

		reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
		reg |= 0x20;
		__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
		mod_timer(&slcdc_timer, jiffies + SLCDC_REFRESH_RATE);
		TRACE("slcdc_isr\n");
	}

	return IRQ_HANDLED;
}

/**
 *@brief slcdc buffer initialization
 *
 * Function Name: slcdc_init_buffer
 *
 *
 * Description:This routine will allocate physical memory for SLCDC data and
 *             for SLCDC command.
 *
 *@return		0 on success, appropriate error value on error
 **/
int slcdc_init_buffer(void)
{

	TRACE("slcdc data buffer size = %x \n",
	      (unsigned int)SLCDC_DATA_MEM_SIZE);

	if (g_slcdc_dbuffer_phyaddress != NULL)
		return -EINVAL;

	if (g_slcdc_cbuffer_phyaddress != NULL)
		return -EINVAL;

	g_slcdc_dbuffer_phyaddress = (u16 *) mxc_malloc(SLCDC_DATA_MEM_SIZE);
	g_slcdc_cbuffer_phyaddress = (u16 *) mxc_malloc(SLCDC_CMD_MEM_SIZE);

	if (!g_slcdc_dbuffer_phyaddress || !g_slcdc_cbuffer_phyaddress) {
		if (g_slcdc_dbuffer_phyaddress != (u16 *) 0)
			mxc_free((u32) g_slcdc_dbuffer_phyaddress);

		if (g_slcdc_cbuffer_phyaddress != (u16 *) 0)
			mxc_free((u32) g_slcdc_cbuffer_phyaddress);

		FAILED("can not allocated memory\n");
		return -ENOMEM;
	} else {

		TRACE
		    ("allocated cmd_buffer=0x%x size=%d, data_buffer=0x%x size=%d",
		     (int)g_slcdc_cbuffer_phyaddress, SLCDC_CMD_MEM_SIZE,
		     (int)g_slcdc_dbuffer_phyaddress, SLCDC_DATA_MEM_SIZE);

		if ((!request_mem_region((u32) g_slcdc_dbuffer_phyaddress,
					 SLCDC_DATA_MEM_SIZE,
					 slcdc_fb_info.fix.id))
		    ||
		    (!request_mem_region
		     ((u32) g_slcdc_cbuffer_phyaddress, SLCDC_CMD_MEM_SIZE,
		      slcdc_fb_info.fix.id))) {
			FAILED("request mem region failed.");
			return -EBUSY;
		}

		if (!(slcdc_fb_info.screen_base = ioremap((u32)
							  g_slcdc_dbuffer_phyaddress,
							  SLCDC_DATA_MEM_SIZE)))
		{
			release_mem_region((u32) g_slcdc_dbuffer_phyaddress,
					   SLCDC_DATA_MEM_SIZE);
			FAILED("Unable to map fb memory to virtual address");
			return -EIO;
		} else {
			g_slcdc_dbuffer_address =
			    (u16 *) slcdc_fb_info.screen_base;
		}

		if (!
		    (g_slcdc_cbuffer_address =
		     ioremap((u32) g_slcdc_cbuffer_phyaddress,
			     SLCDC_CMD_MEM_SIZE))) {
			release_mem_region((u32) g_slcdc_dbuffer_phyaddress,
					   SLCDC_DATA_MEM_SIZE);
			release_mem_region((u32) g_slcdc_cbuffer_phyaddress,
					   SLCDC_CMD_MEM_SIZE);
			FAILED("Unable to map fb memory to virtual address");
			return -EIO;
		}
	}

	TRACE("slcdc data buffer address = %x cmd buffer address= %x \n",
	      (unsigned int)slcdc_par.screen_start_address,
	      (unsigned int)g_slcdc_cbuffer_address);

	return 0;
}

/**
 *@brief slcdc buffer de initialization
 *
 * Function Name: slcdc_free_buffer
 *
 *
 * Description:This routine will deallocate the physical memory allocated by
 *             slcdc_init_buffer.
 *
 *@return		0 on success
 **/
int slcdc_free_buffer(void)
{

	FUNC_START;

	iounmap(g_slcdc_dbuffer_address);
	iounmap(g_slcdc_cbuffer_address);

	release_mem_region((unsigned long)g_slcdc_dbuffer_phyaddress,
			   SLCDC_DATA_MEM_SIZE);
	release_mem_region((unsigned long)g_slcdc_cbuffer_phyaddress,
			   SLCDC_CMD_MEM_SIZE);

	mxc_free((u32) g_slcdc_dbuffer_phyaddress);
	mxc_free((u32) g_slcdc_cbuffer_phyaddress);

	FUNC_END;
	return 0;
}

/**
 *@brief slcdc mmap function
 *
 * Function Name: slcdc_mmap
 *
 *
 * Description: This is the memory map routine for this driver,will setup the
 *              memory map used in this driver.
 *
 *@param	filp	the pointer to the file descripter
 *@param	vma		the pointer to the vma structure related to the driver
 *
 *@return	int   return status
 *			@li 0  sucessful
 *			@li other failed
 * Modification History:
 * 	Dec,2003,			Karen first version
 *
 **/

static int slcdc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long page, pos;
	unsigned long start = (unsigned long)vma->vm_start;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	if (size > SLCDC_DATA_MEM_SIZE)
		return -EINVAL;

	TRACE("slcdc_mmap is called 1\n");
	pos = (unsigned long)slcdc_par.v_screen_start_address;

	while (size > 0) {
		page = virt_to_phys((void *)pos);

		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		/* This is an IO map - tell maydump to skip this VMA  */
		vma->vm_flags |= VM_IO;

		if (remap_pfn_range
		    (vma, start, page >> PAGE_SHIFT, PAGE_SIZE, PAGE_SHARED))
			return -EAGAIN;
		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return 0;
}

/**
 *@brief slcdc mmap function called from framebuffer
 *
 * Function Name: slcdcfb_mmap
 *
 *
 * Description: This is the memory map routine for this driver,will setup the
 *              memory map used in this driver.
 *
 *@return		0 on success any other value for failure
 **/
static int slcdcfb_mmap(struct fb_info *info, struct file *file,
			struct vm_area_struct *vma)
{

	return slcdc_mmap(file, vma);

}

/**
 *@brief slcdc display-on function
 *
 * Function Name: slcdc_display_on
 *
 * Description:This helper routine will send the command to the SLCD for display
 *             on.
 *
 **/
void slcdc_display_on(void)
{
	u16 *databuffer = NULL;

	__raw_writel((u32) g_slcdc_cbuffer_phyaddress,
		     IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));

	/* put cmd into cmd buffer */
	databuffer = g_slcdc_cbuffer_address;
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_DISON;
	/* send cmd */
	slcdc_send_cmd(1);
	/* delay */
	slcdc_delay(0xffff);
	/* send cmd */
	slcdc_send_cmd(1);
	/* delay  */
	slcdc_delay(0xffff);
}

/**
 *@brief slcdc display-off function
 *
 * Function Name: slcdc_display_off
 *
 * Description:This helper routine will send the command to the SLCD for display
 *             off.
 *
 **/
void slcdc_display_off(void)
{
	u16 *databuffer = NULL;
	/* Cause LCD module to enter sleep mode.Bu sure to input display OFF command
	   to turn off the display before inputting the SLPIN command
	   Keep the logic power supply turned on for 40ms after the LCD module
	   enters the sleep mode.  */
	/* put cmd into cmd buffer */
	databuffer = slcdc_par.v_screen_start_address;
	/* send cmd SLPIN */
	*databuffer = SLCDC_CMD_SLPIN;
	slcdc_send_cmd(1);
	/* put reset high */

#if 0
#ifdef SLCDC_MUX_SSI3_SLCDC2
	/* set reset pin to high */
	_reg_GPIO_DR(GPIOA) |= 0x80000000;
#endif
#ifdef SLCDC_MUX_SD_SLCDC1
	/* set reset pin to high */
	_reg_GPIO_DR(GPIOB) |= 0x00000020;
#endif
#endif

	/* delay      for 50ms */
	slcdc_delay(0xffff);
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_DISOFF;
	/* send cmd DISOFF */
	slcdc_send_cmd(1);
	TRACE("disoff \n");
	slcdc_delay(0xffff);
}

/**
 *@brief slcdc display-off function
 *
 * Function Name: slcdc_display_normal
 *
 * Description:This helper routine will send the command to the SLCD for display
 *             normal.
 *
 **/
void slcdc_display_normal(void)
{
	u16 *databuffer = NULL;
	databuffer = slcdc_par.v_screen_start_address;
	*databuffer = SLCDC_CMD_DISNOR;
	/* send cmd */
	slcdc_send_cmd(1);
	/* delay */
	slcdc_delay(100);
}

/**
 *@brief slcdc display sleep out function
 *
 * Function Name: slcdc_sleep_out
 *
 * Description:This helper routine will send the command to the SLCD for wakeup
 *             from sleep.
 *
 **/
void slcdc_sleep_out(void)
{
	u16 *databuffer = NULL;
	databuffer = slcdc_par.v_screen_start_address;
	*databuffer = SLCDC_CMD_SLPOUT;
	/* send cmd */
	slcdc_send_cmd(1);
	/* delay */
	slcdc_delay(0xffff);
}

/** @brief sends control command to SLCD */
void slcdc_display_ctl(void)
{
	int i;
#ifdef SLCDC_SERIAL_MODE
	u16 disctl[11] = { 0x1c00, 0x0200, 0x8200, 0x0000,
		0x1e00, 0xe000, 0x0000, 0xdc00,
		0x0000, 0x0200, 0x0000
	};
#else
	u16 disctl[11] = { 0x1c, 0x02, 0x82, 0x00,
		0x1e, 0xe0, 0x00, 0xdc,
		0x00, 0x02, 0x00
	};
#endif
	u16 *databuffer = NULL;
	/* It make various display timing settings. */
	databuffer = slcdc_par.v_screen_start_address;
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_DISCTL;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(2000);
	/* put parameter into data buffer */
	for (i = 0; i < 11; i++) {
		*databuffer = disctl[i];
		databuffer++;
	}
	/* send data */
	slcdc_send_data(11);

}

/**
 *@brief slcd page set function
 *
 * Function Name: slcdc_page_set
 *
 * Description:This helper routine will send the command to the SLCD to set the
 *             page inside video ram
 *
 **/
void slcdc_page_set(void)
{
	/* It specify a page address area in order to access the display data
	   RAM from the MPU. Be sure to set both starting and ending pages. */
	int i;
	u16 *databuffer = NULL;
#ifdef SLCDC_SERIAL_MODE
	u16 pset[4] = { 0x0000, 0x0000, 0xb100, 0x0000 };
#else
	u16 pset[4] = { 0x00, 0x00, 0xb1, 0x00 };
#endif
	databuffer = (u16 *) (slcdc_par.v_screen_start_address);
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_SD_PSET;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(20000);

	/* put parameter into data buffer */
	for (i = 0; i < 4; i++) {
		*databuffer = pset[i];
		databuffer++;
	}
	/* send data */
	slcdc_send_data(4);

	slcdc_delay(20000);
}

/**
 *@brief slcd column set function
 *
 * Function Name: slcdc_col_set
 *
 * Description:This helper routine will send the command to the SLCD to set the
 *             column inside video ram
 *
 **/
void slcdc_col_set(void)
{
	/* It specify starting and ending collumns. */
	int i;
#ifdef SLCDC_SERIAL_MODE
	u16 cset[4] = { 0x0200, 0x0000, 0xb100, 0x0000 };
#else
	u16 cset[4] = { 0x02, 0x00, 0xb1, 0x00 };
#endif
	u16 *databuffer = NULL;
	databuffer = (u16 *) (slcdc_par.v_screen_start_address);
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_SD_CSET;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(20000);
	/* put parameter into data buffer */
	for (i = 0; i < 4; i++) {
		*databuffer = cset[i];
		databuffer++;
	}
	/* send data */
	slcdc_send_data(4);
	slcdc_delay(20000);

}

void slcdc_data_ctl(void)
{
	u8 *databuffer = NULL;
	databuffer = (u8 *) (slcdc_par.v_screen_start_address);
	*databuffer = SLCDC_CMD_DATCTL;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(20000);
	*databuffer = 0x28;
	/* send data */
	slcdc_send_data(1);
	slcdc_delay(20000);
}

void slcdc_volctl(void)
{
	u16 *databuffer = NULL;
	databuffer = slcdc_par.v_screen_start_address;
	*databuffer = SLCDC_CMD_VOLCTR;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(20000);
#ifdef SLCDC_SERIAL_MODE
	*databuffer = 0x9f00;
#else
	*databuffer = 0x9f;
#endif
	/* send data */
	slcdc_send_data(1);
	slcdc_delay(20000);
}

void slcdc_ascset(void)
{
	int i;
#ifdef SLCDC_SERIAL_MODE
	u16 lcd_para_ASCSET[7] = { 0x0000, 0x0000, 0x1f00, 0x0100,
		0x1f00, 0x0100, 0x0300
	};
#else
	u16 lcd_para_ASCSET[7] = { 0x00, 0x00, 0x1f, 0x01,
		0x1f, 0x01, 0x03
	};

#endif
	u16 *databuffer = NULL;
	databuffer = (u16 *) (slcdc_par.v_screen_start_address);
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_ASCSET;
	/* put parameter into data buffer */
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(2000);
	for (i = 0; i < 7; i++) {
		*databuffer = lcd_para_ASCSET[i];
		databuffer++;
	}
	/* send data */
	slcdc_send_data(7);
	slcdc_delay(20000);
}

void slcdc_scstart(void)
{
	int i;
	u16 lcd_para_SCSTART[2] = { 0x00, 0x00 };
	u16 *databuffer = NULL;
	databuffer = (u16 *) (slcdc_par.v_screen_start_address);
	/* put cmd into cmd buffer */
	*databuffer = SLCDC_CMD_SCSTART;
	/* put parameter into data buffer */
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(10000);
	for (i = 0; i < 2; i++) {
		*databuffer = lcd_para_SCSTART[i];
		databuffer++;
	}
	/* send data */
	slcdc_send_data(2);
	slcdc_delay(20000);

}

void slcdc_config_panel(void)
{
	int i;
#if 1
	/* set data format */
	slcdc_config(8);
	slcdc_delay(0xffff);
	slcdc_reset(1);		/* pull reset signal high */

	/* set data format */
	slcdc_delay(20000);
	slcdc_data_ctl();
	slcdc_delay(1000);
	slcdc_config(16);

	/* sleep out */
	slcdc_sleep_out();
	slcdc_delay(20000);

	for (i = 0; i < 8; i++) {
		slcdc_volctl();
		slcdc_delay(2000);
	}
	slcdc_delay(0xffff);
	slcdc_delay(0xffff);
	slcdc_delay(0xffff);

	slcdc_display_on();
	slcdc_delay(0xffff);
	/* set col address */
	slcdc_col_set();
	/* set page address */
	slcdc_page_set();
	/* set area in screen to be used for scrolling */
	slcdc_ascset();
	slcdc_delay(20000);
	/* set top scroll page within the scroll area */
	slcdc_scstart();
	mdelay(4);
#endif
#if 0

	//set data format
	slcdc_config(8);
	slcdc_delay(0xffff);
	slcdc_reset(1);		//pull reset signal high

	//set data format
	slcdc_delay(20000);
	slcdc_data_ctl();
	slcdc_delay(1000);
	slcdc_config(16);
	//sleep out
	slcdc_sleep_out();
	slcdc_delay(0xffff);
	//set col address
	slcdc_col_set();
	//set page address
	slcdc_page_set();
	//set area in screen to be used for scrolling
	slcdc_ascset();
	slcdc_delay(20000);
	//set top scroll page within the scroll area
	slcdc_scstart();
	slcdc_delay(0xffff);
	//turn on slcd display
	slcdc_display_on();
	slcdc_delay(0xffff);
	slcdc_delay(0xffff);

	for (i = 0; i < 8; i++) {
		slcdc_volctl();
		slcdc_delay(2000);
	}
#endif
}

/**
 *@brief slcdc ioctl routine
 *
 * Function Name: slcdc_ioctl
 * Description:This routine will implement driver-specific functions
 *
 *@param 		inode	:	the pointer to driver-related inode.
 *@param 		filp	:	the pointer to driver-related file structure.
 *@param		cmd	:	the command number.
 *@param		arg:	argument which depends on command.
 *
 *@return	int   return status
 *			@li 0  sucess
 *			@li 1  failure
 *
 * Modification History:
 * 	Dec,2003			Karen first version for MX21 TO2
 *
 **/

static int slcdc_ioctl(struct inode *inode, struct file *filp,
		       u_int cmd, u_long arg)
{
	u16 *databuffer = NULL;

	switch (cmd) {
	case SLCDC_CMD_DISON:
		slcdc_display_on();
		break;

	case SLCDC_CMD_DISOFF:
		slcdc_display_off();
		break;

	case SLCDC_CMD_DISNOR:
		slcdc_display_normal();
		break;

	case SLCDC_CMD_DISINV:
		/* put cmd into cmd buffer */
		databuffer = slcdc_par.v_screen_start_address;
		*databuffer = SLCDC_CMD_DISINV;
		/* send cmd */
		slcdc_send_cmd(1);
		slcdc_delay(100);
		break;

	case SLCDC_CMD_DATCTL:
		break;

	case SLCDC_CMD_RAMWR:
		/* Causes the MPU to be a data entry mode,allowing it to serite data
		   in the display memory. Inputting any other cmds other than NOP
		   cancels the data entry mode. */

		__raw_writel((u32) g_slcdc_cbuffer_phyaddress, IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */

		/* put cmd into cmd buffer */
		databuffer = g_slcdc_cbuffer_address;
		/* put cmd into cmd buffer */
		*databuffer = SLCDC_CMD_RAMWR;
		/* send cmd */
		slcdc_send_cmd(1);
		slcdc_delay(2000);

		/* this is to display one data per time, it is ok.  */
		slcdc_delay(0xffff);
		__raw_writel((u32) slcdc_par.screen_start_address, IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */

		slcdc_delay(0xffff);
		slcdc_send_data(SLCDC_MEM_SIZE);
		slcdc_delay(0xffff);

		break;

	case SLCDC_CMD_RAMRD:
		break;

	case SLCDC_CMD_PTLIN:
		/* This command is used to display a partial screen for power saving. */
		break;

	case SLCDC_CMD_PTLOUT:
		/* This command is used to exit the partila diaplay mode. */
		break;

	case SLCDC_CMD_GCP64:
		/* make 63 pulse position settings of GCP for 64 gray scales. */
		/* send cmd into cmd buffer
		   send parameter into data buffer
		   send cmd
		   send data  */
		break;

	case SLCDC_CMD_GCP16:
		break;

	case SLCDC_CMD_GSSET:
		break;

	case SLCDC_CMD_ASCSET:
		/* make partial screen scroll settings. */
		/* send cmd into cmd buffer
		   send parameter into data buffer
		   send cmd
		   delay
		   send data
		   delay */
		break;

	case SLCDC_CMD_SCSTART:
		/* set a scroll starting page in the scrolling area.Be sure to send
		   this cmd after ASCSET . */
		/* send cmd into cmd buffer
		   send parameter into data buffer
		   send cmd
		   delay
		   send data
		   delay */
		break;
	default:
		break;
	}
	return 0;
}

static int slcdcfb_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg,
			 struct fb_info *info)
{
	return slcdc_ioctl(inode, file, cmd, arg);
}

/**
 *@brief slcdc close function
 *
 * Function Name: slcdc_release
 *
 *
 * Description: This is the release routine for the driver. And this function \n
 *	   will be called while the module being closed. In this function, it will\n
 *	   unregister the apm
 *
 *@param	inode the pointer to the inode descripter
 *@param	filp  the pointer to the file descripter
 *
 *@return	int   return status
 *			@li 0  sucessful
 *			@li other failed
 * Modification History:
 * 	Dec,2003			Karen first version
 *
 **/

int slcdc_release(struct inode *inode, struct file *filp)
{
	TRACE("slcdc_release: ----\n");
	del_timer_sync(&slcdc_timer);
	return 0;
}

/**
 *@brief slcdc timer call back function
 *
 * Function Name: slcdc_timer_func
 *
 * Description:This helper routine prepare the SLCDC for data transfer
 *
 **/
static void slcdc_timer_func(unsigned long args)
{
	volatile unsigned long reg;
	/* Causes the MPU to be a data entry mode,allowing it to serite data in the
	   display memory. Inputting any other cmds other than NOP cancels the data
	   entry mode. */

	__raw_writel((u32) g_slcdc_cbuffer_phyaddress, IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */

	/* put cmd into cmd buffer */
	*g_slcdc_cbuffer_address = SLCDC_CMD_RAMWR;
	/* send cmd */
	slcdc_send_cmd(1);
	slcdc_delay(2000);

	/* this is to display one data per time, it is ok. */
	slcdc_delay(0xffff);
	__raw_writel((u32) slcdc_par.screen_start_address, IO_ADDRESS(SLCDC_BASE_ADDR + 0x00));	/* _reg_SLCDC_DBADDR */

	slcdc_delay(0xffff);
	/* slcdc_send_data( SLCDC_MEM_SIZE); */

	/* enable interrupt */
#ifdef USING_INTERRUPT_SLCDC
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= ~0xffffff7f;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
#endif
	/* set length */
	__raw_writel(SLCDC_MEM_SIZE, IO_ADDRESS(SLCDC_BASE_ADDR + 0x04));	/* _reg_SLCDC_DBUF_SIZE */
	/* set automode 01 for data */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/
	reg |= 0x00000800;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));

	/* set GO  */
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
						    /*_reg_SLCDC_LCD_CTRL_STAT*/

	reg |= 0x1;
	__raw_writel(reg, IO_ADDRESS(SLCDC_BASE_ADDR + 0x20));
	slcdc_delay(0xffff);

	return;
}

/**
 *@brief slcdc timer initialization
 *
 * Function Name: init_slcdc_timer
 *
 * Description:This helper routine prepare the SLCDC timer for refreshing the
 *             screen
 *
 **/
static void init_slcdc_timer(void)
{

	init_timer(&slcdc_timer);
	slcdc_timer.expires = jiffies + SLCDC_REFRESH_RATE;
	slcdc_timer.data = 0;
	slcdc_timer.function = slcdc_timer_func;
	add_timer(&slcdc_timer);
	start_fb_timer_flag = 1;
}

/**
 *@brief slcdc open function
 *
 * Function Name: slcdc_open
 *
 *
 * Description: This is the open routine for the driver. And this function \n
 *	   will be called while the module being opened. In this function, it will\n
 *			@li	configure GPIO for serial/parallel
 *			@li	slcdc reset
 *			@li init slcd registers
 *			@li init waitqueue
 *			@li get rca, select the card
 *			@li send some command for panel configuration
 *
 *@param	inode the pointer to the inode descripter
 *@param	filp  the pointer to the file descripter
 *
 *@return	int   return status
 *			@li 0  sucessful
 *			@li other failed
 * Modification History:
 * 	Dec,2003,			Karen first version
 *  June,2004,			Shirley update for Parallel mode
 *
 **/

int slcdc_open(struct inode *inode, struct file *filp)
{
	volatile unsigned long reg;

	TRACE("slcdc_open: ----\n");

	/* init dev */
#ifdef SLCDC_SERIAL_MODE
	slcdc_gpio_serial();
#else
	slcdc_gpio_paralle();
#endif

	slcdc_init_dev();
	slcdc_reset(0);		/*pull reset low */
	/* init slcd registers */
	slcdc_init_reg();

	/* init waitqueue */
	init_waitqueue_head(&slcdc_wait);
	/* send some command for panel configuration */
	slcdc_config_panel();
	reg = __raw_readl(IO_ADDRESS(SLCDC_BASE_ADDR + 0x1C));
						    /*_reg_SLCDC_LCD_TXCONFIG*/
	TRACE("TRANS_CONFIG_REG=%x \n", (unsigned int)reg);

	/* init slcdc timer, and start timer */
	init_slcdc_timer();
	return 0;
}

/**
 *@brief slcdc init function
 *
 * Function Name: slcdc_init
 *
 *
 *@return	int   return status
 *			@li 0  sucess
 *			@li other failure
 *
 * Description: This is the initialization routine for the driver. And this
 *              function \n will be called while the module being installed.
 *              In this function, it will \n register char device,request
 *              slcdc irq, initialize the buffer,register to\npower management.
 *
 * Modification History:
 * 	Dec,2003,			Karen update for MX21 TO2
 *
 **/

int __init slcdc_init(void)
{
	int tmp, err;

	INFO("SLCDC Driver \n");
	INFO("Motorola SPS-SuZhou \n");

	_init_fbinfo();

	if (slcdcfb_set_var(&slcdc_fb_info)) ;	/* current_par.allow_modeset = 0; */

	register_framebuffer(&slcdc_fb_info);

	devfs_mk_cdev(slcdc_device_num, S_IFCHR | S_IRUGO | S_IWUSR, "slcdc");

	if (slcdc_major != 0) {
		/* use user supplied major number */
		slcdc_device_num = MKDEV(slcdc_major, slcdc_minor);

		err = register_chrdev_region(slcdc_device_num, 1, MODULE_NAME);
	} else {
		/* auto create device major number */
		err =
		    alloc_chrdev_region(&slcdc_device_num, slcdc_minor, 1,
					MODULE_NAME);
	}

	if (err < 0) {
		TRACE("%s driver: Unable to register chrdev region\n",
		      MODULE_NAME);
		return err;
	}

	cdev_init(&slcdc_dev, &g_slcdc_fops);
	slcdc_dev.owner = THIS_MODULE;
	slcdc_dev.ops = &g_slcdc_fops;

	if ((err = cdev_add(&slcdc_dev, slcdc_device_num, 1))) {
		TRACE
		    ("%s driver: Unable to create character device. Error code=%d\n",
		     MODULE_NAME, err);
		return -ENODEV;
	}

	/* init interrupt */
	tmp = request_irq(SLCDC_IRQ,
			  (void *)slcdc_isr,
			  SA_INTERRUPT | SA_SHIRQ, MODULE_NAME, MODULE_NAME);
	if (tmp) {
		printk("slcdc_init:cannot init major= %d irq=%d\n",
		       MAJOR(slcdc_device_num), SLCDC_IRQ);
		devfs_remove(MODULE_NAME);
		cdev_del(&slcdc_dev);
		return -1;
	}

	/* init buffer */
	/* initialize buffer address */
	g_slcdc_dbuffer_address = NULL;
	g_slcdc_dbuffer_phyaddress = NULL;
	g_slcdc_cbuffer_address = NULL;
	g_slcdc_cbuffer_phyaddress = NULL;
	slcdc_init_buffer();

	g_slcdc_status = 0;

	return 0;
}

/**
 *@brief slcdc cleanup function
 *
 * Function Name: slcdc_cleanup
 *
 *@return	None
 *
 * Description: This is the cleanup routine for the driver. And this function \n
 *	 will be called while the module being removed. In this function, it will \n
 *	 cleanup all the registered entries
 *
 * Modification History:
 * 	Dec 2003,			Karen update for MX21 TO2
 *
 **/

void __exit slcdc_cleanup(void)
{

	unregister_chrdev_region(slcdc_device_num, 1);

	devfs_remove(MODULE_NAME);

	/*Do some cleanup work */
	free_irq(SLCDC_IRQ, MODULE_NAME);

	unregister_framebuffer(&slcdc_fb_info);

	slcdc_free_buffer();

	cdev_del(&slcdc_dev);

	return;
}

module_init(slcdc_init);
module_exit(slcdc_cleanup);

/*////////////// Frame Buffer Suport /////////////////////////////////// */

/**
 * _check_var - Validates a var passed in.
 * @var: frame buffer variable screen structure
 * @info: frame buffer structure that represents a single frame buffer
 *
 * Checks to see if the hardware supports the state requested by var passed
 * in. This function does not alter the hardware state! If the var passed in
 * is slightly off by what the hardware can support then we alter the var
 * PASSED in to what we can do. If the hardware doesn't support mode change
 * a -EINVAL will be returned by the upper layers.
 *
 * Returns negative errno on error, or zero on success.
 */
static int _check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	const struct slcdcfb_par *par = (const struct slcdcfb_par *)info->par;

	if (var->xres > current_par.max_xres)
		var->xres = current_par.max_xres;
	if (var->yres > current_par.max_yres)
		var->yres = current_par.max_yres;

	var->xres_virtual = var->xres_virtual < par->xres
	    ? par->xres : var->xres_virtual;
	var->yres_virtual = var->yres_virtual < par->yres
	    ? par->yres : var->yres_virtual;
	var->bits_per_pixel = par->bits_per_pixel;

	switch (var->bits_per_pixel) {
	case 2:
	case 4:
	case 8:
		var->red.length = 4;
		var->green = var->red;
		var->blue = var->red;
		var->transp.length = 0;
		break;

	case 12:		/*  RGB 444 */
	case 16:		/* RGB 565 */
		TRACE("16->a\n");
		var->red.length = 4;
		var->blue.length = 4;
		var->green.length = 4;
		var->transp.length = 0;
#ifdef __LITTLE_ENDIAN
		TRACE("16->b\n");
		var->red.offset = 8;
		var->green.offset = 4;
		var->blue.offset = 0;
		var->transp.offset = 0;
#endif				/* __LITTLE_ENDIAN */
		break;

	default:
		return -EINVAL;
	}

	/* *var->screen_start_address=(u_char*)((u_long)g_slcdc_dbuffer_phyaddress );
	 *var->v_screen_start_address=(u_char*)((u_long)g_slcdc_dbuffer_address  ); */

	var->height = -1;
	var->width = -1;
	var->grayscale = 0;
	var->nonstd = 0;

	var->pixclock = -1;
	var->left_margin = -1;
	var->right_margin = -1;
	var->upper_margin = -1;
	var->lower_margin = -1;
	var->hsync_len = -1;
	var->vsync_len = -1;

	var->vmode = FB_VMODE_NONINTERLACED;
	var->sync = 0;

	return 0;
}

/**
 *@brief Use current_par to set a var structure
 *
 *@param 	var Input var data
 *@param	par LCD controller parameters
 *
 *@return  If no error, return 0
 *
 */
static int _encode_var(struct fb_var_screeninfo *var, struct slcdcfb_par *par)
{
	FUNC_START
	    /* Don't know if really want to zero var on entry.
	       Look at set_var to see.  If so, may need to add extra params to par   */
	    memset(var, 0, sizeof(*var));
	var->xres = par->xres;
	TRACE("var->xress=%d\n", var->xres);
	var->yres = par->yres;
	TRACE("var->yres=%d\n", var->yres);
	var->xres_virtual = par->xres_virtual;
	TRACE("var->xres_virtual=%d\n", var->xres_virtual);
	var->yres_virtual = par->yres_virtual;
	TRACE("var->yres_virtual=%d\n", var->yres_virtual);

	var->bits_per_pixel = par->bits_per_pixel;
	TRACE("var->bits_per_pixel=%d\n", var->bits_per_pixel);

	switch (var->bits_per_pixel) {
	case 2:
	case 4:
	case 8:
		var->red.length = 4;
		var->green = var->red;
		var->blue = var->red;
		var->transp.length = 0;
		break;
	case 12:		/* This case should differ for Active/Passive mode */
	case 16:
		TRACE("16->a\n");
		var->red.length = 4;
		var->blue.length = 4;
		var->green.length = 4;
		var->transp.length = 0;
#ifdef __LITTLE_ENDIAN
		TRACE("16->b\n");
		var->red.offset = 8;
		var->green.offset = 4;
		var->blue.offset = 0;
		var->transp.offset = 0;
#endif				/* __LITTLE_ENDIAN */
		break;
	}

	FUNC_END return 0;
}

/**
 *@brief Get the video params out of 'var'. If a value doesn't fit,
 * 		round it up,if it's too big, return -EINVAL.
 *
 *@warning Round up in the following order: bits_per_pixel, xres,
 * 	yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 * 	bitfields, horizontal timing, vertical timing.
 *
 *@param 	var Input var data
 *@param	par LCD controller parameters
 *
 *@return If no error, return 0
 */
static int _decode_var(struct fb_info *info)
{
	struct fb_var_screeninfo *var = &info->var;
	struct slcdcfb_par par_var;
	struct slcdcfb_par *par = &par_var;

	FUNC_START *par = current_par;

	if ((par->xres = var->xres) < MIN_XRES)
		par->xres = MIN_XRES;
	if ((par->yres = var->yres) < MIN_YRES)
		par->yres = MIN_YRES;
	if (par->xres > current_par.max_xres)
		par->xres = current_par.max_xres;
	if (par->yres > current_par.max_yres)
		par->yres = current_par.max_yres;
	par->xres_virtual = var->xres_virtual < par->xres
	    ? par->xres : var->xres_virtual;
	par->yres_virtual = var->yres_virtual < par->yres
	    ? par->yres : var->yres_virtual;
	par->bits_per_pixel = var->bits_per_pixel;

	switch (par->bits_per_pixel) {

	case 4:
		par->visual = FB_VISUAL_PSEUDOCOLOR;
		par->palette_size = 16;
		break;

	case 8:
		par->visual = FB_VISUAL_PSEUDOCOLOR;
		par->palette_size = 256;
		break;

	case 12:		/* RGB 444 */
	case 16:		/* RGB 565 */
		par->visual = FB_VISUAL_TRUECOLOR;
		par->palette_size = 0;
		break;

	default:
		return -EINVAL;
	}

	par->screen_start_address =
	    (u_char *) ((u_long) g_slcdc_dbuffer_phyaddress);
	par->v_screen_start_address =
	    (u_char *) ((u_long) g_slcdc_dbuffer_address);

	/* update_lcd ? */

	FUNC_END return 0;
}

/**
 *@brief Set current_par by var, also set display data, specially the console
 * 	   related file operations, then enable the SLCD controller, and set cmap to
 * 	   hardware.
 *
 *@param	var	Iuput data pointer
 *@param	con	Console ID
 *@param	info	Frame buffer information
 *
 *@return   If no error, return 0.
 *
 **/
static int slcdcfb_set_var(struct fb_info *info)
{
	struct display *display;
	int err;
	struct slcdcfb_par par;
	struct fb_var_screeninfo *var = &info->var;

	FUNC_START;

	display = &global_disp;	/* Default display settings */

	/* Decode var contents into a par structure, adjusting any */
	/* out of range values. */
	if ((err = _decode_var(info))) {
		TRACE("decode var error!");
		return err;
	}

	/* Store adjusted par values into var structure */
	_encode_var(var, &par);

	if ((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_TEST)
		return 0;

	else if (((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NOW) &&
		 ((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NXTOPEN))
		return -EINVAL;

	display->inverse = 0;

	init_var = *var;	/* TODO:gcc support structure copy? */

	FUNC_END;
	return 0;
}

/**
 *@brief Blank the screen, if blank, disable LCD controller, while if no blank
 * 		set cmap and enable LCD controller
 *
 *@param	blank Blank flag
 *@param	info	Frame buffer database
 *
 *@return  VOID
 */
static int slcdcfb_blank(int blank, struct fb_info *info)
{

	if (blank) {
		slcdc_display_off();
	} else {
		slcdc_display_normal();
	}

	return 0;
}

/**
 *@brief Initialize frame buffer. While 16bpp is used to store a 12 bits pixels
 *      packet, it is not a really 16bpp system, maybe in-compatiable with
 * 		other system or GUI.There are some field in var which specify
 *		the red/green/blue offset in a 16bit word, just little endian is
 * 		concerned
 *
 *@return  VOID
 **/
static void __init _init_fbinfo(void)
{
	FUNC_START;

	slcdc_fb_info.node = -1;
	slcdc_fb_info.flags = 0;	/* Low-level driver is not a module */
	slcdc_fb_info.fbops = &slcdcfb_ops;
	slcdc_fb_info.monspecs = monspecs;

	strcpy(slcdc_fb_info.fix.id, "SLCDC");
	/* FIXME... set fix parameters */
	/*
	 * * setup initial parameters
	 * */
	memset(&init_var, 0, sizeof(init_var));
	memset(&current_par, 0, sizeof(current_par));

	init_var.transp.length = 0;
	init_var.nonstd = 0;
	init_var.activate = FB_ACTIVATE_NOW;
	init_var.xoffset = 0;
	init_var.yoffset = 0;
	init_var.height = -1;
	init_var.width = -1;
	init_var.vmode = FB_VMODE_NONINTERLACED;

	/*xres and yres might be set when loading the module, if this driver is
	   built as module */
	current_par.max_xres = SLCDC_WIDTH;
	current_par.max_yres = SLCDC_HIGH;

	current_par.max_bpp = SLCDC_BPP;
	init_var.red.length = 5;
	init_var.green.length = 6;
	init_var.blue.length = 5;
#ifdef __LITTLE_ENDIAN
	init_var.red.offset = 11;
	init_var.green.offset = 5;
	init_var.blue.offset = 0;
#endif				/* __LITTLE_ENDIAN */
	init_var.grayscale = 16;
	init_var.sync = 0;
	init_var.pixclock = 171521;

	current_par.screen_start_address = NULL;
	current_par.v_screen_start_address = NULL;
	current_par.screen_memory_size = MAX_PIXEL_MEM_SIZE;
	current_par.currcon = -1;

	init_var.xres = current_par.max_xres;
	init_var.yres = current_par.max_yres;
	init_var.xres_virtual = init_var.xres;
	init_var.yres_virtual = init_var.yres;
	init_var.bits_per_pixel = current_par.max_bpp;

	current_par.xres = init_var.xres;
	current_par.yres = init_var.yres;
	current_par.xres_virtual = init_var.xres;
	current_par.yres_virtual = init_var.yres;

	/* initialize current screen information */
	memcpy(&slcdc_fb_info.var, &init_var, sizeof(init_var));

	FUNC_END;
}

/**
 *@brief slcdc framebuffer open call
 *
 * Function Name: slcdcfb_open
 *
 * Description  : This function is called by the framebuffer when the
 *                application requests.
 *
 *@return		0 on success any other value for failure
 **/
static int slcdcfb_open(struct fb_info *info, int user)
{
	return slcdc_open(NULL, 0);
}

/**
 *@brief slcdc framebuffer release call
 *
 * Function Name: slcdcfb_release
 *
 * Description  : This function is called by the framebuffer when the
 *                application closes the link to framebuffer.
 *
 *@return		0 on success any other value for failure
 **/
static int slcdcfb_release(struct fb_info *info, int user)
{
	start_fb_timer_flag = 0;
	return slcdc_release(NULL, 0);
}

/*\@}*/

MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");
MODULE_LICENSE("GPL");
