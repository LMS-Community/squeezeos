/***********************************************************************
 *
 * linux/arch/arm/mach-s3c2413/mach-smdk2413.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 ***********************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <asm/arch/nand.h>
#include <asm/arch/smdk.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-ebi.h>
#include <asm/arch/regs-ssmc.h>

#include "devs.h"
#include "cpu.h"
#include "pm.h"


/* Jive flash assignment
 *
 * 0x00000000-0x00028000 : uboot
 * 0x00028000-0x0002c000 : uboot env
 * 0x0002c000-0x00030000 : uboot env
 * 0x00030000-0x00200000 : zimage A
 * 0x00200000-0x01600000 : cramfs A
 * 0x01600000-0x017d0000 : zimage B
 * 0x017d0000-0x02bd0000 : cramfs B
 * 0x02bd0000-0x03fd0000 : yaffs
 */

static struct mtd_partition smdk2413_imageA_nand_part[] = {

#if 0
	/* Don't allow access to the bootloader from linux */
	{
		.name           = "uboot",
		.offset         = 0,
		.size           = (160*SZ_1K),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},

	/* spare */
        {
                .name           = "spare",
                .offset         = (176*SZ_1K),
                .size           = (16*SZ_1K),
        },
#endif

	/* booted images */
        {
                .name           = "kernel (ro)",
                .offset         = (192*SZ_1K),
                .size           = (2*SZ_1M) - (192*SZ_1K),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
        },

        {
                .name           = "root (ro)",
                .offset         = (2*SZ_1M),
                .size           = (20*SZ_1M),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
        },

	/* yaffs */
        {
                .name           = "yaffs",
                .offset         = (44*SZ_1M),
                .size           = (20*SZ_1M),
        },

	/* bootloader environment */
        {
                .name           = "env",
                .offset         = (160*SZ_1K),
                .size           = 2 * (16*SZ_1K),
        },

	/* upgrade images */
        {
                .name           = "zimage",
                .offset         = (22*SZ_1M),
                .size           = (2*SZ_1M) - (192*SZ_1K),
        },

        {
                .name           = "cramfs",
                .offset         = (24*SZ_1M) - (192*SZ_1K),
                .size           = (20*SZ_1M),
        },
};

static struct mtd_partition smdk2413_imageB_nand_part[] = {

#if 0
	/* Don't allow access to the bootloader from linux */
	{
		.name           = "uboot",
		.offset         = 0,
		.size           = (160*SZ_1K),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},

	/* spare */
        {
                .name           = "spare",
                .offset         = (176*SZ_1K),
                .size           = (16*SZ_1K),
        },
#endif

	/* booted images */
        {
                .name           = "kernel (ro)",
                .offset         = (22*SZ_1M),
                .size           = (2*SZ_1M) - (192*SZ_1K),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
        },

        {
                .name           = "root (ro)",
                .offset         = (24*SZ_1M) - (192*SZ_1K),
                .size           = (20*SZ_1M),
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
        },

	/* yaffs */
        {
                .name           = "yaffs",
                .offset         = (44*SZ_1M),
                .size           = (20*SZ_1M),
        },

	/* bootloader environment */
        {
                .name           = "env",
                .offset         = (160*SZ_1K),
                .size           = 2 * (16*SZ_1K),
        },

	/* upgrade images */
        {
                .name           = "zimage",
                .offset         = (192*SZ_1K),
                .size           = (2*SZ_1M) - (192*SZ_1K),
        },

        {
                .name           = "cramfs",
                .offset         = (2*SZ_1M),
                .size           = (20*SZ_1M),
        },
};


static int smartmedia_map[] = { 0 };

static struct s3c2413_nand_set smdk2413_nand_sets[] = {
	[0] = {
	.name           = "SmartMedia",
	.devid		= 0x76,  		// 0x76 for 1208
	.nr_chips       = 1,
	.nr_map         = smartmedia_map,
	.nr_partitions  = ARRAY_SIZE(smdk2413_imageA_nand_part),
	.partitions     = smdk2413_imageA_nand_part,
	}
};

static struct s3c2413_platform_nand smdk2413_nand_info = {
	.tacls		= 7,//0,
	.twrph0		= 7,//0,
	.twrph1		= 7,//0,
	.nr_sets	= ARRAY_SIZE(smdk2413_nand_sets),
	.sets		= smdk2413_nand_sets,
};


static struct map_desc smdk2413_iodesc[] __initdata = {
};

#define UCON S3C2413_UCON_DEFAULT
#define ULCON S3C2413_LCON_CS8 | S3C2413_LCON_PNONE
#define UFCON S3C2413_UFCON_RXTRIG8 | S3C2413_UFCON_FIFOMODE

static struct s3c2413_uartcfg smdk2413_uartcfgs[] = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	}
};

static struct platform_device *smdk2413_devices[] __initdata = {
	&s3c_device_usb,
	&s3c_device_udc,
	&s3c_device_lcd,
	&s3c_device_wdt,
	&s3c_device_i2c,
	&s3c_device_iis,
	&s3c_device_rtc,
	&s3c_device_nand,
	&s3c_device_adc,
	&s3c_device_sdi,
	&s3c_device_spi0,
	&s3c_device_spi1,
};

static struct s3c24xx_board smdk2413_board __initdata = {
	.devices       = smdk2413_devices,
	.devices_count = ARRAY_SIZE(smdk2413_devices)
};


/* modify active mtd partitions based on boot command line */
static int __init smdk2413_setup(char *options)
{
	unsigned long mtdset;

	if (!options || !*options) {
		return 0;
	}

	mtdset = simple_strtoul(options, &options, 10);
	
	switch (mtdset) {
	default:
	case 0:
		smdk2413_nand_sets[0].nr_partitions = ARRAY_SIZE(smdk2413_imageA_nand_part);
		smdk2413_nand_sets[0].partitions = smdk2413_imageA_nand_part;
		break;

	case 1:
		smdk2413_nand_sets[0].nr_partitions = ARRAY_SIZE(smdk2413_imageB_nand_part);
		smdk2413_nand_sets[0].partitions = smdk2413_imageB_nand_part;
		break;
	}

	return 0;
}

__setup("mtdset=", smdk2413_setup);


void __init smdk2413_map_io(void)
{
	s3c_device_nand.dev.platform_data = &smdk2413_nand_info;
	s3c24xx_init_io(smdk2413_iodesc, ARRAY_SIZE(smdk2413_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdk2413_uartcfgs, ARRAY_SIZE(smdk2413_uartcfgs));
	s3c24xx_set_board(&smdk2413_board);
}

void smdk2411_cs89x0_set(void)
{
	unsigned long val;

	//Bank1 Idle cycle ctrl.
	val = readl(S3C2413_SSMC_SMBIDCYR1);
	val = 0;
	writel(val, S3C2413_SSMC_SMBIDCYR1);

	//Bank1 Write Wait State ctrl.
	val = readl(S3C2413_SSMC_SMBWSTWRR1);
	val = 14;
	writel(val, S3C2413_SSMC_SMBWSTWRR1);

	//Bank1 Output Enable Assertion Delay ctrl.
	val = readl(S3C2413_SSMC_SMBWSTOENR1);
	val = 4;
	writel(val, S3C2413_SSMC_SMBWSTOENR1);

	//Bank1 Write Enable Assertion Delay ctrl.
	val = readl(S3C2413_SSMC_SMBWSTWENR1);
	val = 4;
	writel(val, S3C2413_SSMC_SMBWSTWENR1);

	//Bank1 Read Wait State cont. = 14 clk
	val = readl(S3C2413_SSMC_SMBWSTRDR1);
	val = 14;
	writel(val, S3C2413_SSMC_SMBWSTRDR1);

	//SMWAIT active High, Read Byte Lane Enabl
	val = readl(S3C2413_SSMC_SMBCR1);
	val |=  ((1<<2)|(1<<0));  
	writel(val, S3C2413_SSMC_SMBCR1);

	val = readl(S3C2413_SSMC_SMBCR1);
	val |=  (1<<2);  
	writel(val, S3C2413_SSMC_SMBCR1);

	val = readl(S3C2413_SSMC_SMBCR1);
	val &= ~((3<<20)|(3<<12)); 
	writel(val, S3C2413_SSMC_SMBCR1);
	
	val = readl(S3C2413_SSMC_SMBCR1);
	val &= ~(3<<4);    
	writel(val, S3C2413_SSMC_SMBCR1);

	val = readl(S3C2413_SSMC_SMBCR1);
	val |= (1<<4);    
	writel(val, S3C2413_SSMC_SMBCR1);

	val = readl(S3C2413_SSMC_SMBCR1);
	val &= ~((1<<15)|(1<<7));    
	writel(val, S3C2413_SSMC_SMBCR1);
}

// rahul, for 2413
void smdk2413_cs89x0_set(void)
{
	unsigned long val;

	val = readl(S3C2413_BANK_CFG);
	val &= ~((1<<5)|(1<<4));
	writel(val, S3C2413_BANK_CFG);

	//Bank5 Idle cycle ctrl.
	val = readl(S3C2413_SSMC_SMBIDCYR5);
	val = 0;
	writel(val, S3C2413_SSMC_SMBIDCYR5);


	//Bank5 Write Wait State ctrl.
	val = readl(S3C2413_SSMC_SMBWSTWRR5);
	val = 14;
	writel(val, S3C2413_SSMC_SMBWSTWRR5);


	//Bank5 Output Enable Assertion Delay ctrl.     Tcho?
	val = readl(S3C2413_SSMC_SMBWSTOENR5);
	val = 2;
	writel(val, S3C2413_SSMC_SMBWSTOENR5);


	//Bank5 Write Enable Assertion Delay ctrl.
	val = readl(S3C2413_SSMC_SMBWSTWENR5);
	val = 2;
	writel(val, S3C2413_SSMC_SMBWSTWENR5);


	//Bank5 Read Wait State cont. = 14 clk          Tacc?
	val = readl(S3C2413_SSMC_SMBWSTRDR5);
	val = 14;
	writel(val, S3C2413_SSMC_SMBWSTRDR5);


	//SMWAIT active High, Read Byte Lane Enabl      WS1?
	val = readl(S3C2413_SSMC_SMBCR5);
	val |=  ((1<<2)|(1<<0));  
	writel(val, S3C2413_SSMC_SMBCR5);

	val = readl(S3C2413_SSMC_SMBCR5);
	val &= ~((3<<20)|(3<<12)); 
	writel(val, S3C2413_SSMC_SMBCR5);
	
	val = readl(S3C2413_SSMC_SMBCR5);
	val &= ~(3<<4);    
	writel(val, S3C2413_SSMC_SMBCR5);

	val = readl(S3C2413_SSMC_SMBCR5);
	val |= (1<<4);    
	writel(val, S3C2413_SSMC_SMBCR5);

}

void __init smdk2413_init_irq(void)
{
	s3c24xx_init_irq();
#if defined (CONFIG_ARCH_SMDK2411)	
	smdk2411_cs89x0_set(); //for 2411
#else
	smdk2413_cs89x0_set(); //rahul, for 2413
#endif
}

MACHINE_START(S3C2413, "SMDK2413")
     MAINTAINER("")
     BOOT_MEM(S3C2413_SDRAM_PA, S3C2413_PA_UART, S3C2413_VA_UART)
     BOOT_PARAMS(S3C2413_SDRAM_PA + 0x100)
     MAPIO(smdk2413_map_io)
     INIT_MACHINE(s3c2413_pm_init)
     INITIRQ(smdk2413_init_irq)
	.timer		= &s3c24xx_timer,
MACHINE_END


MACHINE_START(JIVE, "JIVE")
     MAINTAINER("")
     BOOT_MEM(S3C2413_SDRAM_PA, S3C2413_PA_UART, S3C2413_VA_UART)
     BOOT_PARAMS(S3C2413_SDRAM_PA + 0x100)
     MAPIO(smdk2413_map_io)
     INIT_MACHINE(s3c2413_pm_init)
     INITIRQ(smdk2413_init_irq)
	.timer		= &s3c24xx_timer,
MACHINE_END

