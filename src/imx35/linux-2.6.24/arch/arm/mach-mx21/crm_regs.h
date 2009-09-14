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

#ifndef __ARCH_ARM_MACH_MX21_CRM_REGS_H__
#define __ARCH_ARM_MACH_MX21_CRM_REGS_H__

#include <asm/arch/hardware.h>

/* Register offsets */
#define CCM_CSCR                        0x0
#define CCM_MPCTL0                      0x4
#define CCM_MPCTL1                      0x8
#define CCM_SPCTL0                      0xC
#define CCM_SPCTL1                      0x10
#define CCM_OSC26MCTL                   0x14
#define CCM_PCDR0                       0x18
#define CCM_PCDR1                       0x1C
#define CCM_PCCR0                       0x20
#define CCM_PCCR1                       0x24
#define CCM_CCSR                        0x28
#define CCM_PMCTL                       0x2C
#define CCM_PMCOUNT                     0x30
#define CCM_WKGDCTL                     0x34

#define CCM_CSCR_PRESC_OFFSET           29
#define CCM_CSCR_PRESC_MASK             (0x7 << 29)
#define CCM_CSCR_USB_OFFSET             26
#define CCM_CSCR_USB_MASK               (0x7 << 26)
#define CCM_CSCR_SD_OFFSET              24
#define CCM_CSCR_SD_MASK                (0x3 << 24)
#define CCM_CSCR_SPLLRES                (1 << 22)
#define CCM_CSCR_MPLLRES                (1 << 21)
#define CCM_CSCR_SSI2                   (1 << 20)
#define CCM_CSCR_SSI2_OFFSET            20
#define CCM_CSCR_SSI1                   (1 << 19)
#define CCM_CSCR_SSI1_OFFSET            19
#define CCM_CSCR_SP                     (1 << 17)
#define CCM_CSCR_MCU                    (1 << 16)
#define CCM_CSCR_BCLK_OFFSET            10
#define CCM_CSCR_BCLK_MASK              (0xF << 10)
#define CCM_CSCR_IPDIV_OFFSET           9
#define CCM_CSCR_IPDIV                  (1 << 9)
#define CCM_CSCR_OSC26MDIV              (1 << 4)
#define CCM_CSCR_OSC26M                 (1 << 3)
#define CCM_CSCR_FPM                    (1 << 2)
#define CCM_CSCR_SPEN                   (1 << 1)
#define CCM_CSCR_MPEN                   (1 << 0)

#define CCM_MPCTL0_CPLM                 (1 << 31)
#define CCM_MPCTL0_PD_OFFSET            26
#define CCM_MPCTL0_PD_MASK              (0xF << 26)
#define CCM_MPCTL0_MFD_OFFSET           16
#define CCM_MPCTL0_MFD_MASK             (0x3FF << 16)
#define CCM_MPCTL0_MFI_OFFSET           10
#define CCM_MPCTL0_MFI_MASK             (0xF << 10)
#define CCM_MPCTL0_MFN_OFFSET           0
#define CCM_MPCTL0_MFN_MASK             0x3FF

#define CCM_MPCTL1_LF                   (1 << 15)
#define CCM_MPCTL1_BRMO                 (1 << 6)

#define CCM_SPCTL0_CPLM                 (1 << 31)
#define CCM_SPCTL0_PD_OFFSET            26
#define CCM_SPCTL0_PD_MASK              (0xF << 26)
#define CCM_SPCTL0_MFD_OFFSET           16
#define CCM_SPCTL0_MFD_MASK             (0x3FF << 16)
#define CCM_SPCTL0_MFI_OFFSET           10
#define CCM_SPCTL0_MFI_MASK             (0xF << 10)
#define CCM_SPCTL0_MFN_OFFSET           0
#define CCM_SPCTL0_MFN_MASK             0x3FF

#define CCM_SPCTL1_LF                   (1 << 15)
#define CCM_SPCTL1_BRMO                 (1 << 6)

#define CCM_OSC26MCTL_PEAK_OFFSET       16
#define CCM_OSC26MCTL_PEAK_MASK         (0x3 << 16)
#define CCM_OSC26MCTL_AGC_OFFSET        8
#define CCM_OSC26MCTL_AGC_MASK          (0x3F << 8)

#define CCM_PCDR0_SSI2BAUDDIV_OFFSET    26
#define CCM_PCDR0_SSI2BAUDDIV_MASK      (0x3F << 26)
#define CCM_PCDR0_SSI1BAUDDIV_OFFSET    16
#define CCM_PCDR0_SSI1BAUDDIV_MASK      (0x3F << 16)
#define CCM_PCDR0_NFCDIV_OFFSET         12
#define CCM_PCDR0_NFCDIV_MASK           (0xF << 12)
#define CCM_PCDR0_CLKO48MDIV_OFFSET     5
#define CCM_PCDR0_CLKO48MDIV_MASK       (0x7 << 5)
#define CCM_PCDR0_FIRIDIV_OFFSET        0
#define CCM_PCDR0_FIRIDIV_MASK          0x1F

#define CCM_PCDR1_PERDIV4_OFFSET        24
#define CCM_PCDR1_PERDIV4_MASK          (0x3F << 24)
#define CCM_PCDR1_PERDIV3_OFFSET        16
#define CCM_PCDR1_PERDIV3_MASK          (0x3F << 16)
#define CCM_PCDR1_PERDIV2_OFFSET        8
#define CCM_PCDR1_PERDIV2_MASK          (0x3F << 8)
#define CCM_PCDR1_PERDIV1_OFFSET        0
#define CCM_PCDR1_PERDIV1_MASK          0x3F

#define CCM_PCCR0_HCLK_CSI_OFFSET       31
#define CCM_PCCR0_HCLK_DMA_OFFSET       30
#define CCM_PCCR0_HCLK_BROM_OFFSET      28
#define CCM_PCCR0_HCLK_EMMA_OFFSET      27
#define CCM_PCCR0_HCLK_LCDC_OFFSET      26
#define CCM_PCCR0_HCLK_SLCDC_OFFSET     25
#define CCM_PCCR0_HCLK_USBOTG_OFFSET    24
#define CCM_PCCR0_HCLK_BMI_OFFSET       23
#define CCM_PCCR0_PERCLK4_OFFSET        22
#define CCM_PCCR0_SLCDC_OFFSET          21
#define CCM_PCCR0_FIRI_BAUD_OFFSET      20
#define CCM_PCCR0_NFC_OFFSET            19
#define CCM_PCCR0_PERCLK3_OFFSET        18
#define CCM_PCCR0_SSI1_BAUD_OFFSET      17
#define CCM_PCCR0_SSI2_BAUD_OFFSET      16
#define CCM_PCCR0_EMMA_OFFSET           15
#define CCM_PCCR0_USBOTG_OFFSET         14
#define CCM_PCCR0_DMA_OFFSET            13
#define CCM_PCCR0_I2C_OFFSET            12
#define CCM_PCCR0_GPIO_OFFSET           11
#define CCM_PCCR0_SDHC2_OFFSET          10
#define CCM_PCCR0_SDHC1_OFFSET          9
#define CCM_PCCR0_FIRI_OFFSET           8
#define CCM_PCCR0_SSI2_OFFSET           7
#define CCM_PCCR0_SSI1_OFFSET           6
#define CCM_PCCR0_CSPI2_OFFSET          5
#define CCM_PCCR0_CSPI1_OFFSET          4
#define CCM_PCCR0_UART4_OFFSET          3
#define CCM_PCCR0_UART3_OFFSET          2
#define CCM_PCCR0_UART2_OFFSET          1
#define CCM_PCCR0_UART1_OFFSET          0

#define CCM_PCCR1_OWIRE_OFFSET          31
#define CCM_PCCR1_KPP_OFFSET            30
#define CCM_PCCR1_RTC_OFFSET            29
#define CCM_PCCR1_PWM_OFFSET            28
#define CCM_PCCR1_GPT3_OFFSET           27
#define CCM_PCCR1_GPT2_OFFSET           26
#define CCM_PCCR1_GPT1_OFFSET           25
#define CCM_PCCR1_WDT_OFFSET            24
#define CCM_PCCR1_CSPI3_OFFSET          23
#define CCM_PCCR1_RTIC_OFFSET           22
#define CCM_PCCR1_RNGA_OFFSET           21

#define CCM_CCSR_32KSR                  (1 << 15)
#define CCM_CCSR_CLKOSEL_OFFSET         0
#define CCM_CCSR_CLKOSEL_MASK           0x1f

#define CKIH_CLK_FREQ                   26000000	/* 26M reference clk */
#define CKIL_CLK_FREQ                   (32768 * 512)	/* 32.768k oscillator in */

#define SYS_FMCR                        0x8	/*  Functional Muxing Control Reg */

#endif				/* __ARCH_ARM_MACH_MX21_CRM_REGS_H__ */
