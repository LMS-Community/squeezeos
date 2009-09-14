/*
 *  Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_MXC91321_H__
#define __ASM_ARCH_MXC_MXC91321_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

/*!
 * @file arch-mxc/mxc91321.h
 * @brief This file contains register definitions.
 *
 * @ingroup MSL_MXC91321
 */
/*!
 * defines the hardware clock tick rate
 */
#define CLOCK_TICK_RATE         16625000

#define MU_MUX_GN_INTS          0

/*
 * UART Chip level Configuration that a user may not have to edit. These
 * configuration vary depending on how the UART module is integrated with
 * the ARM core
 */
#define MXC_UART_NR 4
/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive Irda data.
 */
#define MXC_UART_IR_RXDMUX      0
/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive UART data.
 */
#define MXC_UART_RXDMUX         0

/*!
 * This option is used to set or clear the dspdma bit in the SDMA config
 * register.
 */
#define MXC_SDMA_DSPDMA         1

/*
 * IRAM
 */
#define IRAM_BASE_ADDR          0x1FFFC000
#define IRAM_BASE_ADDR_VIRT     0xF8000000
#define IRAM_SIZE               SZ_16K

/*
 * L2CC.
 */
#define L2CC_BASE_ADDR          0x30000000

/*
 * SMC
 */
#define SMC_BASE_ADDR          0x40000000
#define SMC_BASE_ADDR_VIRT     0xF5000000
#define SMC_SIZE               SZ_16M

/*
 * SiRF
 */
#define SIRF_BASE_ADDR          0x42000000
#define SIRF_BASE_ADDR_VIRT     0xF6000000
#define SIRF_SIZE               SZ_16M

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS1_BASE_ADDR_VIRT    0xFC000000
#define AIPS1_SIZE              SZ_1M

#define MAX_BASE_ADDR           0x43F04000
#define EVTMON_BASE_ADDR        0x43F08000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define ETB_SLOT4_BASE_ADDR     0x43F10000
#define ETB_SLOT5_BASE_ADDR     0x43F14000
#define ECT_CTIO_BASE_ADDR      0x43F18000
#define I2C_BASE_ADDR           0x43F80000
#define MU_BASE_ADDR            0x43F84000
#define WDOG2_BASE_ADDR         0x43F88000
#define UART1_BASE_ADDR         0x43F90000
#define UART2_BASE_ADDR         0x43F94000
#define OWIRE_BASE_ADDR         0x43F9C000
#define SSI1_BASE_ADDR          0x43FA0000
#define CSPI1_BASE_ADDR         0x43FA4000
#define KPP_BASE_ADDR           0x43FA8000

/*
 * SPBA
 */
#define SPBA0_BASE_ADDR         0x50000000
#define SPBA0_BASE_ADDR_VIRT    0xFC100000
#define SPBA0_SIZE              SZ_1M
#define IOMUXC_BASE_ADDR        0x50000000
#define MMC_SDHC1_BASE_ADDR     0x50004000
#define MMC_SDHC2_BASE_ADDR     0x50008000
#define UART3_BASE_ADDR         0x5000C000
#define CSPI2_BASE_ADDR         0x50010000
#define SSI2_BASE_ADDR          0x50014000
#define SIM1_BASE_ADDR          0x50018000
#define IIM_BASE_ADDR           0x5001C000
#define USBOTG_BASE_ADDR        0x50020000
#define HAC_BASE_ADDR           0x50024000
#define SAHARA_BASE_ADDR        0x50024000
#define UART4_BASE_ADDR         0x50028000
#define GPIO2_BASE_ADDR         0x5002C000
#define SIM2_BASE_ADDR          0x50000000

#define GEMK_BASE_ADDR          0x50034000
#define SDMA_CTI_BASE_ADDR      0x50038000
#define SPBA_CTRL_BASE_ADDR     0x5003C000

/*!
 * Defines for SPBA modules
 */
#define SPBA_SIM2               0x00
#define SPBA_SDHC1              0x04
#define SPBA_SDHC2              0x08
#define SPBA_UART3              0x0C
#define SPBA_CSPI2              0x10
#define SPBA_SSI2               0x14
#define SPBA_SIM1               0x18
#define SPBA_IIM                0x1C
#define SPBA_USB_OTG            0x20
#define SPBA_SAHARA             0x24
#define SPBA_HAC                0x24
#define SPBA_UART4              0x28
#define SPBA_GPIO_SDMA          0x2C
#define SPBA_IOMUX              0x30
#define SPBA_GEM                0x34

/*!
 * Defines for modules using static and dynamic DMA channels
 */
#define MXC_DMA_CHANNEL_UART1_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART1_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART2_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART2_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART3_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART3_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART4_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_UART4_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MMC1  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MMC2  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI1_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI1_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI2_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_SSI2_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_FIR_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_FIR_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI1_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI1_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI2_RX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_CSPI2_TX  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_MEMORY  MXC_DMA_DYNAMIC_CHANNEL
#define MXC_DMA_CHANNEL_DSP_PACKET_DATA0_RD  1
#define MXC_DMA_CHANNEL_DSP_PACKET_DATA0_WR  2
#define MXC_DMA_CHANNEL_DSP_PACKET_DATA1_RD  3
#define MXC_DMA_CHANNEL_DSP_PACKET_DATA1_WR  4
#define MXC_DMA_CHANNEL_DSP_LOG0_CHNL  5
#define MXC_DMA_CHANNEL_DSP_LOG1_CHNL  6
#define MXC_DMA_CHANNEL_DSP_LOG2_CHNL  7
#define MXC_DMA_CHANNEL_DSP_LOG3_CHNL  8
#define MXC_DMA_CHANNEL_GEM            11

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR         0x53F00000
#define AIPS2_BASE_ADDR_VIRT    0xFC200000
#define AIPS2_SIZE              SZ_1M
#define CRM_MCU_BASE_ADDR       0x53F80000
#define ECT_MCU_CTI_BASE_ADDR   0x53F84000
#define EDIO_BASE_ADDR          0x53F88000
#define FIRI_BASE_ADDR          0x53F8C000
#define GPT1_BASE_ADDR          0x53F90000
#define EPIT1_BASE_ADDR         0x53F94000
#define EPIT2_BASE_ADDR         0x53F98000
#define SCC_BASE_ADDR           0x53FAC000
#define RNGA_BASE_ADDR          0x53FB0000
#define RTR_BASE_ADDR           0x53FB4000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define AUDMUX_BASE_ADDR        0x53FC4000
#define MPEG4_ENC_BASE_ADDR     0x53FC8000
#define GPIO1_BASE_ADDR         0x53FCC000
#define SDMA_BASE_ADDR          0x53FD4000
#define RTC_BASE_ADDR           0x53FD8000
#define WDOG1_BASE_ADDR         0x53FDC000
#define PWM_BASE_ADDR           0x53FE0000
#define RTIC_BASE_ADDR          0x53FEC000

/*
 * ROMP and AVIC
 */
#define ROMP_BASE_ADDR          0x60000000
#define ROMP_BASE_ADDR_VIRT     0xFC500000
#define ROMP_SIZE               SZ_1M

#define AVIC_BASE_ADDR          0x68000000
#define AVIC_BASE_ADDR_VIRT     0xFC400000
#define AVIC_SIZE               SZ_1M

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define X_MEMC_BASE_ADDR        0xB8000000
#define X_MEMC_BASE_ADDR_VIRT   0xFC600000
#define X_MEMC_SIZE             SZ_1M

#define NFC_BASE_ADDR           X_MEMC_BASE_ADDR
#define ESDCTL_BASE_ADDR        0xB8001000
#define WEIM_BASE_ADDR          0xB8002000
#define M3IF_BASE_ADDR          0xB8003000

/*
 * Memory regions and CS
 */
#define IPU_MEM_BASE_ADDR       0x70000000
#define CSD0_BASE_ADDR          0x80000000
#define CSD1_BASE_ADDR          0x90000000
#define CS0_BASE_ADDR           0xA0000000
#define CS1_BASE_ADDR           0xA8000000
#define CS2_BASE_ADDR           0xB0000000
#define CS3_BASE_ADDR           0xB2000000
#define CS4_BASE_ADDR           0xB4000000
#define CS4_BASE_ADDR_VIRT      0xF4000000
#define CS4_SIZE                SZ_16M
#define CS5_BASE_ADDR           0xB6000000

/*!
 * This macro defines the physical to virtual address mapping for all the
 * peripheral modules. It is used by passing in the physical address as x
 * and returning the virtual address. If the physical address is not mapped,
 * it returns 0xDEADBEEF
 */
#define IO_ADDRESS(x)   \
        (((x >= IRAM_BASE_ADDR) && (x < (IRAM_BASE_ADDR + IRAM_SIZE))) ? IRAM_IO_ADDRESS(x):\
        ((x >= SMC_BASE_ADDR) && (x < (SMC_BASE_ADDR + SMC_SIZE))) ? SMC_IO_ADDRESS(x):\
        ((x >= SIRF_BASE_ADDR) && (x < (SIRF_BASE_ADDR + SIRF_SIZE))) ? SIRF_IO_ADDRESS(x):\
        ((x >= AIPS1_BASE_ADDR) && (x < (AIPS1_BASE_ADDR + AIPS1_SIZE))) ? AIPS1_IO_ADDRESS(x):\
        ((x >= SPBA0_BASE_ADDR) && (x < (SPBA0_BASE_ADDR + SPBA0_SIZE))) ? SPBA0_IO_ADDRESS(x):\
        ((x >= AIPS2_BASE_ADDR) && (x < (AIPS2_BASE_ADDR + AIPS2_SIZE))) ? AIPS2_IO_ADDRESS(x):\
        ((x >= ROMP_BASE_ADDR) && (x < (ROMP_BASE_ADDR + ROMP_SIZE))) ? ROMP_IO_ADDRESS(x):\
        ((x >= AVIC_BASE_ADDR) && (x < (AVIC_BASE_ADDR + AVIC_SIZE))) ? AVIC_IO_ADDRESS(x):\
        ((x >= CS4_BASE_ADDR) && (x < (CS4_BASE_ADDR + CS4_SIZE))) ? CS4_IO_ADDRESS(x):\
        ((x >= X_MEMC_BASE_ADDR) && (x < (X_MEMC_BASE_ADDR + X_MEMC_SIZE))) ? X_MEMC_IO_ADDRESS(x):\
        0xDEADBEEF)

/*
 * define the address mapping macros: in physical address order
 */

#define IRAM_IO_ADDRESS(x)  \
        (((x) - IRAM_BASE_ADDR) + IRAM_BASE_ADDR_VIRT)

#define SMC_IO_ADDRESS(x)   \
        (((x) - SMC_BASE_ADDR) + SMC_BASE_ADDR_VIRT)

#define SIRF_IO_ADDRESS(x)  \
        (((x) - SIRF_BASE_ADDR) + SIRF_BASE_ADDR_VIRT)

#define AIPS1_IO_ADDRESS(x) \
        (((x) - AIPS1_BASE_ADDR) + AIPS1_BASE_ADDR_VIRT)

#define SPBA0_IO_ADDRESS(x)  \
        (((x) - SPBA0_BASE_ADDR) + SPBA0_BASE_ADDR_VIRT)

#define AIPS2_IO_ADDRESS(x) \
        (((x) - AIPS2_BASE_ADDR) + AIPS2_BASE_ADDR_VIRT)

#define ROMP_IO_ADDRESS(x) \
        (((x) - ROMP_BASE_ADDR) + ROMP_BASE_ADDR_VIRT)

#define AVIC_IO_ADDRESS(x)  \
        (((x) - AVIC_BASE_ADDR) + AVIC_BASE_ADDR_VIRT)

#define CS4_IO_ADDRESS(x)   \
        (((x) - CS4_BASE_ADDR) + CS4_BASE_ADDR_VIRT)

#define X_MEMC_IO_ADDRESS(x)  \
        (((x) - X_MEMC_BASE_ADDR) + X_MEMC_BASE_ADDR_VIRT)

/*
 * DMA request assignments
 */
#define DMA_REQ_ECT        31
#define DMA_REQ_NFC        30
#define DMA_REQ_SSI1_TX1   29
#define DMA_REQ_SSI1_RX1   28
#define DMA_REQ_SSI1_TX2   27
#define DMA_REQ_SSI1_RX2   26
#define DMA_REQ_SSI2_TX1   25
#define DMA_REQ_SSI2_RX1   24
#define DMA_REQ_SSI2_TX2   23
#define DMA_REQ_SSI2_RX2   22
#define DMA_REQ_SDHC2      21
#define DMA_REQ_SDHC1      20
#define DMA_REQ_UART1_TX   19
#define DMA_REQ_UART1_RX   18
#define DMA_REQ_UART2_TX   17
#define DMA_REQ_UART2_RX   16
#define DMA_REQ_EXTREQ1    15
#define DMA_REQ_EXTREQ0    14
#define DMA_REQ_FIRI_TX    13
#define DMA_REQ_FIRI_RX    12
#define DMA_REQ_UART4_TX   13
#define DMA_REQ_UART4_RX   12
#define DMA_REQ_USB2       11
#define DMA_REQ_USB1       10
#define DMA_REQ_CSPI1_TX   9
#define DMA_REQ_CSPI1_RX   8
#define DMA_REQ_CSPI2_TX   7
#define DMA_REQ_CSPI2_RX   6
#define DMA_REQ_SIM1       5
#define DMA_REQ_SIM2       4
#define DMA_REQ_UART3_TX   3
#define DMA_REQ_UART3_RX   2
#define DMA_REQ_GEM        1
#define DMA_REQ_IPU        0

/*
 * Interrupt numbers
 */
#define MXC_INT_RESV0               0
#define MXC_INT_GEM                 1
#define MXC_INT_HAC                 2
#define MXC_INT_SAHARA              2
#define MXC_INT_MU3                 3
#define MXC_INT_MU2                 4
#define MXC_INT_MU1                 5
#define MXC_INT_MU0                 6
#define MXC_INT_ELIT2               7
#define MXC_INT_MMC_SDHC2           8
#define MXC_INT_MMC_SDHC1           9
#define MXC_INT_I2C                 10
#define MXC_INT_SSI2                11
#define MXC_INT_SSI1                12
#define MXC_INT_CSPI2               13
#define MXC_INT_CSPI1               14
#define MXC_INT_EXT_INT7            15
#define MXC_INT_UART3               16
#define MXC_INT_RESV17              17
#define MXC_INT_CCM_MCU_DVFS        17
#define MXC_INT_RESV18              18
#define MXC_INT_ECT                 19
#define MXC_INT_SIM_DATA            20
#define MXC_INT_SIM_GENERAL         21
#define MXC_INT_RNGA                22
#define MXC_INT_RTR                 23
#define MXC_INT_KPP                 24
#define MXC_INT_RTC                 25
#define MXC_INT_PWM                 26
#define MXC_INT_EPIT2               27
#define MXC_INT_EPIT1               28
#define MXC_INT_GPT                 29
#define MXC_INT_UART2               30
#define MXC_INT_DVFS                31
#define MXC_INT_RESV32              32
#define MXC_INT_NANDFC              33
#define MXC_INT_SDMA                34
#define MXC_INT_USBOTG_GRP_ASYNC    35
#define MXC_INT_USBOTG_MNP          36
#define MXC_INT_USBOTG_HOST         37
#define MXC_INT_USBOTG_FUNC         38
#define MXC_INT_USBOTG_DMA          39
#define MXC_INT_USBOTG_CTRL         40
#define MXC_INT_ELIT1               41
#define MXC_INT_IPU                 42
#define MXC_INT_UART1               43
#define MXC_INT_RESV44              44
#define MXC_INT_RESV45              45
#define MXC_INT_IIM                 46
#define MXC_INT_MU_RX_OR            47
#define MXC_INT_MU_TX_OR            48
#define MXC_INT_SCC_SCM             49
#define MXC_INT_EXT_INT6            50
#define MXC_INT_GPIOMCU             51
#define MXC_INT_GPIO1               MXC_INT_GPIOMCU
#define MXC_INT_GPIOSDMA            52
#define MXC_INT_GPIO2               MXC_INT_GPIOSDMA
#define MXC_INT_CCM                 53
#define MXC_INT_UART4_FIRI_OR       54
#define MXC_INT_WDOG2               55
#define MXC_INT_SIRF_EXT_INT5_OR    56
#define MXC_INT_EXT_INT5            56
#define MXC_INT_SIRF_EXT_INT4_OR    57
#define MXC_INT_EXT_INT4            57
#define MXC_INT_EXT_INT3            58
#define MXC_INT_RTIC                59
#define MXC_INT_MPEG4_ENC           60
#define MXC_INT_HANTRO              60
#define MXC_INT_EXT_INT0            61
#define MXC_INT_EXT_INT1            62
#define MXC_INT_EXT_INT2            63

#define MXC_MAX_INT_LINES       64
#define MXC_MAX_EXT_LINES       8

/*!
 * Interrupt Number for ARM11 PMU
 */
#define ARM11_PMU_IRQ           MXC_INT_RESV0

#define MXC_GPIO_INT_BASE	(MXC_MAX_INT_LINES)

/*!
 * Number of GPIO port as defined in the IC Spec
 */
#define GPIO_PORT_NUM           2
/*!
 * Number of GPIO pins per port
 */
#define GPIO_NUM_PIN            32

/*!
 * NFMS bit in CSCR register for pagesize of nandflash
 */
#define NFMS (*((volatile u32 *)IO_ADDRESS(CRM_MCU_BASE_ADDR+0xc)))
#define NFMS_BIT 18

#endif				/* __ASM_ARCH_MXC_MXC91321_H__ */
