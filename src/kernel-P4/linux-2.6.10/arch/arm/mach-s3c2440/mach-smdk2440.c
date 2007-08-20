/***********************************************************************
 *
 * linux/arch/arm/mach-s3c2440/mach-smdk2440.c
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
#include <asm/arch/S3C2440.h>
#include <asm/arch/regs-serial.h>

#include "devs.h"
#include "cpu.h"

static struct mtd_partition smdk2440_default_nand_part[] = {
        {
                .name           "NAND partition 0 : Bootloader",
                .offset         0,
                .size           (192*SZ_1K),
        },
        {
                .name           "NAND partition 1 : Kernel",
                .offset         (192*SZ_1K),
                .size           (2*SZ_1M) - (192*SZ_1K),
        },
        {
                .name           "NAND partition 2",
                .offset         (2*SZ_1M),
                .size           (50*SZ_1M) - (2*SZ_1M),
        },
        {
                .name           "NAND partition 3",
                .offset         (50*SZ_1M),
                .size           (14*SZ_1M),
        }
};

static int smartmedia_map[] = { 0 };

static struct s3c2440_nand_set smdk2440_nand_sets[] = {
	[0] = {
	.name           = "SmartMedia",
	.nr_chips       = 1,
	.nr_map         = smartmedia_map,
	.nr_partitions  = ARRAY_SIZE(smdk2440_default_nand_part),
	.partitions     = smdk2440_default_nand_part
	}
};

static struct s3c2440_platform_nand smdk2440_nand_info = {
	.tacls		= 8,//0,
	.twrph0		= 8,//0,
	.twrph1		= 8,//0,
	.nr_sets	= ARRAY_SIZE(smdk2440_nand_sets),
	.sets		= &smdk2440_nand_sets,
};


static struct map_desc smdk2440_iodesc[] __initdata = {
};

#define UCON S3C2440_UCON_DEFAULT
#define ULCON S3C2440_LCON_CS8 | S3C2440_LCON_PNONE
#define UFCON S3C2440_UFCON_RXTRIG8 | S3C2440_UFCON_FIFOMODE

static struct s3c2440_uartcfg smdk2440_uartcfgs[] = {
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

static struct platform_device *smdk2440_devices[] __initdata = {
	&s3c_device_usb,
	&s3c_device_udc,
	&s3c_device_lcd,
	&s3c_device_wdt,
	&s3c_device_i2c,
	&s3c_device_iis,
	&s3c_device_rtc,
	&s3c_device_nand,
};

static struct s3c24xx_board smdk2440_board __initdata = {
	.devices       = smdk2440_devices,
	.devices_count = ARRAY_SIZE(smdk2440_devices)
};

void __init smdk2440_map_io(void)
{
	s3c_device_nand.dev.platform_data = &smdk2440_nand_info;
	s3c24xx_init_io(smdk2440_iodesc, ARRAY_SIZE(smdk2440_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdk2440_uartcfgs, ARRAY_SIZE(smdk2440_uartcfgs));
	s3c24xx_set_board(&smdk2440_board);
}

void __init smdk2440_init_irq(void)
{
	s3c24xx_init_irq();
}

MACHINE_START(S3C2440, "SMDK2440")
     MAINTAINER("")
     BOOT_MEM(S3C2440_SDRAM_PA, S3C2440_PA_UART, S3C2440_VA_UART)
     BOOT_PARAMS(S3C2440_SDRAM_PA + 0x100)
     MAPIO(smdk2440_map_io)
     INITIRQ(smdk2440_init_irq)
	.timer		= &s3c24xx_timer,
MACHINE_END


