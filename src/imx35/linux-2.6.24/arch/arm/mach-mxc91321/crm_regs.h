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
#ifndef __ARCH_ARM_MACH_MXC91321_CRM_REGS_H__
#define __ARCH_ARM_MACH_MXC91321_CRM_REGS_H__

#include <asm/hardware.h>

#define MXC_CCM_BASE		IO_ADDRESS(CRM_MCU_BASE_ADDR)

/* Register addresses */
#define MXC_CCM_MCR		(MXC_CCM_BASE + 0x0)
#define MXC_CCM_MPDR0		(MXC_CCM_BASE + 0x4)
#define MXC_CCM_MPDR1		(MXC_CCM_BASE + 0x8)
#define MXC_CCM_RCSR		(MXC_CCM_BASE + 0xC)
#define MXC_CCM_MPCTL		(MXC_CCM_BASE + 0x10)
#define MXC_CCM_UPCTL		(MXC_CCM_BASE + 0x14)
#define MXC_CCM_COSR		(MXC_CCM_BASE + 0x18)
#define MXC_CCM_MCGR0		(MXC_CCM_BASE + 0x1C)
#define MXC_CCM_MCGR1		(MXC_CCM_BASE + 0x20)
#define MXC_CCM_MCGR2		(MXC_CCM_BASE + 0x24)
#define MXC_CCM_SDCR		(MXC_CCM_BASE + 0x3C)
#define MXC_CCM_CDTR		(MXC_CCM_BASE + 0x40)
/* Registers present only in MXC91321 */
#define MXC_CCM_DCVR0           (MXC_CCM_BASE + 0x28)
#define MXC_CCM_DCVR1           (MXC_CCM_BASE + 0x2c)
#define MXC_CCM_DCVR2           (MXC_CCM_BASE + 0x30)
#define MXC_CCM_DCVR3           (MXC_CCM_BASE + 0x34)
#define MXC_CCM_TPCTL		(MXC_CCM_BASE + 0x44)
#define MXC_CCM_UCDTR		(MXC_CCM_BASE + 0x48)
#define MXC_CCM_TCDTR		(MXC_CCM_BASE + 0x4C)
#define MXC_CCM_MPDR2		(MXC_CCM_BASE + 0x50)
#define MXC_CCM_DPTCDBG 	(MXC_CCM_BASE + 0x54)
#define MXC_CCM_PMCR0	 	(MXC_CCM_BASE + 0x38)
#define MXC_CCM_PMCR1		(MXC_CCM_BASE + 0x58)

/* Register bit definitions */
#define MXC_CCM_MCR_WBEN                    (0x1 << 31)
#define MXC_CCM_MCR_LPM_0                   (0x0 << 29)
#define MXC_CCM_MCR_LPM_1                   (0x1 << 29)
#define MXC_CCM_MCR_LPM_2                   (0x2 << 29)
#define MXC_CCM_MCR_LPM_3                   (0x3 << 29)
#define MXC_CCM_MCR_LPM_OFFSET              29
#define MXC_CCM_MCR_LPM_MASK                (0x3 << 29)
#define MXC_CCM_MCR_SSIS1                   (0x1 << 28)
#define MXC_CCM_MCR_SSIS2                   (0x1 << 27)
#define MXC_CCM_MCR_CSIS_OFFSET	             25
#define MXC_CCM_MCR_CSIS_MASK               (0x3 << 25)
#define MXC_CCM_MCR_SDHC1S_OFFSET           14
#define MXC_CCM_MCR_SDHC1S_MASK             (0x3 << 14)
#define MXC_CCM_MCR_SBYOS                   (0x1 << 13)
#define MXC_CCM_MCR_FIRS                    (0x1 << 11)
#define MXC_CCM_MCR_SDHC2S_OFFSET           9
#define MXC_CCM_MCR_SDHC2S_MASK             (0x3 << 9)
#define MXC_CCM_MCR_UPE                     (0x1 << 8)
#define MXC_CCM_MCR_UPL                     (0x1 << 7)
#define MXC_CCM_MCR_MPEN                    (0x1 << 6)
#define MXC_CCM_MCR_PLLBYP                  (0x1 << 5)
#define MXC_CCM_MCR_TPE                     (0x1 << 4)
#define MXC_CCM_MCR_TPL                     (0x1 << 3)
#define MXC_CCM_MCR_MPL                     (0x1 << 2)
#define MXC_CCM_MCR_DPE                     (0x1 << 1)
#define MXC_CCM_MCR_DPL                     0x1

#define MXC_CCM_MPDR0_CSI_PDF_OFFSET        23
#define MXC_CCM_MPDR0_CSI_PDF_MASK          (0x1FF << 23)
#define MXC_CCM_MPDR0_CSI_FPDF              (0x1 << 22)
#define MXC_CCM_MPDR0_CSI_PRE               (0x1 << 21)
#define MXC_CCM_MPDR0_CSI_DIS               (0x1 << 19)
#define MXC_CCM_MPDR0_TPSEL                 (0x1 << 11)
#define MXC_CCM_MPDR0_TPSEL_0               0
#define MXC_CCM_MPDR0_TPSEL_1               (0x1 << 11)
#define MXC_CCM_MPDR0_TPSEL_OFFSET	    11
#define MXC_CCM_MPDR0_NFC_PDF_OFFSET        8
#define MXC_CCM_MPDR0_NFC_PDF_MASK          (0x7 << 8)
#define MXC_CCM_MPDR0_IPG_PDF_OFFSET        6
#define MXC_CCM_MPDR0_IPG_PDF_MASK          (0x3 << 6)
#define MXC_CCM_MPDR0_MAX_PDF_OFFSET        3
#define MXC_CCM_MPDR0_MAX_PDF_MASK          (0x7 << 3)
#define MXC_CCM_MPDR0_MAX_PDF_3		    0x00000010
#define MXC_CCM_MPDR0_MAX_PDF_4             0x00000018
#define MXC_CCM_MPDR0_MAX_PDF_5             0x00000020
#define MXC_CCM_MPDR0_BRMM_OFFSET           0
#define MXC_CCM_MPDR0_BRMM_MASK		    0x7
#define MXC_CCM_MPDR0_BRMM_0		0x00000000
#define MXC_CCM_MPDR0_BRMM_1		0x00000001
#define MXC_CCM_MPDR0_BRMM_2		0x00000002
#define MXC_CCM_MPDR0_BRMM_3	    	0x00000003
#define MXC_CCM_MPDR0_BRMM_4		0x00000004

#define MXC_CCM_MPDR1_USB_DIS               (0x1 << 30)
#define MXC_CCM_MPDR1_USB_PDF_OFFSET        27
#define MXC_CCM_MPDR1_USB_PDF_MASK          (0x7 << 27)
#define MXC_CCM_MPDR1_FIRI_PREPDF_OFFSET    24
#define MXC_CCM_MPDR1_FIRI_PREPDF_MASK      (0x7 << 24)
#define MXC_CCM_MPDR1_FIRI_PDF_OFFSET       19
#define MXC_CCM_MPDR1_FIRI_PDF_MASK         (0x1F << 19)
#define MXC_CCM_MPDR1_FIRI_DIS              (0x1 << 18)
#define MXC_CCM_MPDR1_SSI2_PREPDF_OFFSET    15
#define MXC_CCM_MPDR1_SSI2_PREPDF_MASK      (0x7 << 15)
#define MXC_CCM_MPDR1_SSI2_PDF_OFFSET       10
#define MXC_CCM_MPDR1_SSI2_PDF_MASK         (0x1F << 10)
#define MXC_CCM_MPDR1_SSI2_DIS              (0x1 << 9)
#define MXC_CCM_MPDR1_SSI1_PREPDF_OFFSET    6
#define MXC_CCM_MPDR1_SSI1_PREPDF_MASK      (0x7 << 6)
#define MXC_CCM_MPDR1_SSI1_PDF_OFFSET       1
#define MXC_CCM_MPDR1_SSI1_PDF_MASK         (0x1F << 1)
#define MXC_CCM_MPDR1_SSI1_DIS              0x1

#define MXC_CCM_RCSR_NF16B                  0x80000
#define MXC_CCM_RCSR_NFMS                   0x40000
#define MXC_CCM_RCSR_MPRES                  0x8000
#define MXC_CCM_RCSR_CLMREN                 0x100
#define MXC_CCM_RCSR_ARMCE                  0x80

/* Bit definitions for MCU, USB and Turbo PLL control registers */
#define MXC_CCM_PCTL_BRM                    0x80000000
#define MXC_CCM_PCTL_PDF_OFFSET             26
#define MXC_CCM_PCTL_PDF_MASK               (0xF << 26)
#define MXC_CCM_PCTL_MFD_OFFSET             16
#define MXC_CCM_PCTL_MFD_MASK               (0x3FF << 16)
#define MXC_CCM_MCUPCTL_MFI_OFFSET          11
#define MXC_CCM_MCUPCTL_MFI_MASK            (0xF << 11)
#define MXC_CCM_MCUPCTL_MFN_OFFSET          0
#define MXC_CCM_MCUPCTL_MFN_MASK            0x7FF
#define MXC_CCM_USBPCTL_MFI_OFFSET          10
#define MXC_CCM_USBPCTL_MFI_MASK            (0xF << 10)
#define MXC_CCM_USBPCTL_MFN_OFFSET          0
#define MXC_CCM_USBPCTL_MFN_MASK            0x3FF
#define MXC_CCM_TPCTL_MFI_OFFSET            11
#define MXC_CCM_TPCTL_MFI_MASK              (0xF << 11)
#define MXC_CCM_TPCTL_MFN_OFFSET            0
#define MXC_CCM_TPCTL_MFN_MASK              0x7FF

#define MXC_CCM_MCGR0_SSI1_OFFSET           2
#define MXC_CCM_MCGR0_SSI1_MASK             (0x3 << 2)
#define MXC_CCM_MCGR0_UART1_OFFSET          4
#define MXC_CCM_MCGR0_UART1_MASK            (0x3 << 4)
#define MXC_CCM_MCGR0_UART2_OFFSET          6
#define MXC_CCM_MCGR0_UART2_MASK            (0x3 << 6)
#define MXC_CCM_MCGR0_CSPI1_OFFSET          8
#define MXC_CCM_MCGR0_CSPI1_MASK            (0x3 << 8)
#define MXC_CCM_MCGR0_FIRI_OFFSET           10
#define MXC_CCM_MCGR0_FIRI_MASK             (0x3 << 10)
#define MXC_CCM_MCGR0_GPTMCU_OFFSET         12
#define MXC_CCM_MCGR0_GPTMCU_MASK           (0x3 << 12)
#define MXC_CCM_MCGR0_RTC_OFFSET            14
#define MXC_CCM_MCGR0_RTC_MASK              (0x3 << 14)
#define MXC_CCM_MCGR0_EPIT1MCU_OFFSET       16
#define MXC_CCM_MCGR0_EPIT1MCU_MASK         (0x3 << 16)
#define MXC_CCM_MCGR0_EPIT2MCU_OFFSET       18
#define MXC_CCM_MCGR0_EPIT2MCU_MASK         (0x3 << 18)
#define MXC_CCM_MCGR0_EDIO_OFFSET           20
#define MXC_CCM_MCGR0_EDIO_MASK             (0x3 << 20)
#define MXC_CCM_MCGR0_WDOGMCU_OFFSET        22
#define MXC_CCM_MCGR0_WDOGMCU_MASK          (0x3 << 22)
#define MXC_CCM_MCGR0_PWMMCU_OFFSET         26
#define MXC_CCM_MCGR0_PWMMCU_MASK           (0x3 << 26)
#define MXC_CCM_MCGR0_OWIRE_OFFSET          28
#define MXC_CCM_MCGR0_OWIRE_MASK            (0x3 << 28)
#define MXC_CCM_MCGR0_I2C_OFFSET            30
#define MXC_CCM_MCGR0_I2C_MASK              (0x3 << 30)

#define MXC_CCM_MCGR1_IPU_OFFSET            0
#define MXC_CCM_MCGR1_IPU_MASK              0x3
#define MXC_CCM_MCGR1_MPEG4_OFFSET          2
#define MXC_CCM_MCGR1_MPEG4_MASK            (0x3 << 2)
#define MXC_CCM_MCGR1_EMI_OFFSET            4
#define MXC_CCM_MCGR1_EMI_MASK              (0x3 << 4)
#define MXC_CCM_MCGR1_IIM_OFFSET            6
#define MXC_CCM_MCGR1_IIM_MASK              (0x3 << 6)
#define MXC_CCM_MCGR1_SIM1_OFFSET           8
#define MXC_CCM_MCGR1_SIM1_MASK             (0x3 << 8)
#define MXC_CCM_MCGR1_SIM2_OFFSET           10
#define MXC_CCM_MCGR1_SIM2_MASK             (0x3 << 10)
#define MXC_CCM_MCGR1_GEM_OFFSET            14
#define MXC_CCM_MCGR1_GEM_MASK              (0x3 << 14)
#define MXC_CCM_MCGR1_USBOTG_OFFSET         16
#define MXC_CCM_MCGR1_USBOTG_MASK           (0x3 << 16)
#define MXC_CCM_MCGR1_CSPI2_OFFSET          20
#define MXC_CCM_MCGR1_CSPI2_MASK            (0x3 << 20)
#define MXC_CCM_MCGR1_UART4_OFFSET          22
#define MXC_CCM_MCGR1_UART4_MASK            (0x3 << 22)
#define MXC_CCM_MCGR1_UART3_OFFSET          24
#define MXC_CCM_MCGR1_UART3_MASK            (0x3 << 24)
#define MXC_CCM_MCGR1_SDHC2_OFFSET          26
#define MXC_CCM_MCGR1_SDHC2_MASK            (0x3 << 26)
#define MXC_CCM_MCGR1_SDHC1_OFFSET          28
#define MXC_CCM_MCGR1_SDHC1_MASK            (0x3 << 28)
#define MXC_CCM_MCGR1_SSI2_OFFSET           30
#define MXC_CCM_MCGR1_SSI2_MASK             (0x3 << 30)

#define MXC_CCM_MCGR2_SDMA_OFFSET           0
#define MXC_CCM_MCGR2_SDMA_MASK             0x3
#define MXC_CCM_MCGR2_RTRMCU_OFFSET         2
#define MXC_CCM_MCGR2_RTRMCU_MASK           (0x3 << 2)
#define MXC_CCM_MCGR2_KPP_OFFSET            6
#define MXC_CCM_MCGR2_KPP_MASK              (0x3 << 6)
#define MXC_CCM_MCGR2_MU_OFFSET             8
#define MXC_CCM_MCGR2_MU_MASK               (0x3 << 8)
#define MXC_CCM_MCGR2_ECT_OFFSET            10
#define MXC_CCM_MCGR2_ECT_MASK              (0x3 << 10)
#define MXC_CCM_MCGR2_SMC_OFFSET            12
#define MXC_CCM_MCGR2_SMC_MASK              (0x3 << 12)
#define MXC_CCM_MCGR2_RTIC_OFFSET           14
#define MXC_CCM_MCGR2_RTIC_MASK             (0x3 << 14)
#define MXC_CCM_MCGR2_SPBA_OFFSET           16
#define MXC_CCM_MCGR2_SPBA_MASK             (0x3 << 16)
#define MXC_CCM_MCGR2_SAHARA_OFFSET         18
#define MXC_CCM_MCGR2_SAHARA_MASK           (0x3 << 18)

#define MXC_CCM_MPDR2_SDHC2_PDF_OFFSET      7
#define MXC_CCM_MPDR2_SDHC2_PDF_MASK        (0xF << 7)
#define MXC_CCM_MPDR2_SDHC2DIS              (0x1 << 6)
#define MXC_CCM_MPDR2_SDHC1_PDF_OFFSET      1
#define MXC_CCM_MPDR2_SDHC1_PDF_MASK        (0xF << 1)
#define MXC_CCM_MPDR2_SDHC1DIS              0x1

#define MXC_CCM_PMCR0_EMIHE                 0x80000000
#define MXC_CCM_PMCR0_PMICHE                0x40000000
#define MXC_CCM_PMCR0_DVSE                  0x04000000
#define MXC_CCM_PMCR0_DFSP                  0x02000000
#define MXC_CCM_PMCR0_DFSI                  0x01000000
#define MXC_CCM_PMCR0_DFSIM                 0x00800000
#define MXC_CCM_PMCR0_DRCE3                 0x00400000
#define MXC_CCM_PMCR0_DRCE1                 0x00200000
#define MXC_CCM_PMCR0_DRCE2                 0x00100000
#define MXC_CCM_PMCR0_DRCE0                 0x00080000
#define MXC_CCM_PMCR0_DCR                   0x00020000
#define MXC_CCM_PMCR0_PWTS                  0x00000100
#define MXC_CCM_PMCR0_NWTS                  0x00000080
#define MXC_CCM_PMCR0_CPFA                  0x00000040
#define MXC_CCM_PMCR0_DPVCR                 0x00000020
#define MXC_CCM_PMCR0_DPVV                  0x00000010
#define MXC_CCM_PMCR0_PTVAIM                0x00000008
#define MXC_CCM_PMCR0_PTVAI_OFFSET          1
#define MXC_CCM_PMCR0_PTVAI_MASK            (0x3 << 1)
#define MXC_CCM_PMCR0_DPTEN                 0x00000001

#define DVSUP_TURBO			    0	/* Dummy */
#define DVSUP_HIGH			    0	/* Dummy */
#define DVSUP_MEDIUM			    0	/* Dummy */
#define DVSUP_LOW			    0	/* Dummy */
#define MXC_CCM_PMCR0_PTVIS		    0	/* Dummy */

#define MXC_CCM_PMCR1_DVFS_STATE_OFFSET     22
#define MXC_CCM_PMCR1_DVFS_STATE_MASK       (0x7 << 22)
#define MXC_CCM_PMCR1_WFI_VAL               (0x1 << 21)
#define MXC_CCM_PMCR1_WFI_SEL               (0x1 << 20)
#define MXC_CCM_PMCR1_VOLT_HIGH             (0x1 << 19)
#define MXC_CCM_PMCR1_VOLT_LOW              (0x1 << 18)
#define MXC_CCM_PMCR1_PMIC_HS_CNT_OFFSET    0
#define MXC_CCM_PMCR1_PMIC_HS_CNT_MASK      0x3FFF

#define MXC_CCM_COSR_CKO2EN	            (0x1 << 13)
#define MXC_CCM_COSR_CKO2S_MASK	            (0x1F << 14)
#define MXC_CCM_COSR_CKO2S_OFFSET	    14
#define MXC_CCM_COSR_CKO2DV_MASK	    (0x7 << 10)
#define MXC_CCM_COSR_CKO2DV_OFFSET	    10

#define MXC_CCM_COSR_CKO1EN	            (0x1 << 6)
#define MXC_CCM_COSR_CKO1S_MASK	            0x7
#define MXC_CCM_COSR_CKO1S_OFFSET           0
#define MXC_CCM_COSR_CKO1DV_MASK	    (0x7 << 3)
#define MXC_CCM_COSR_CKO1DV_OFFSET	    3

#define DPTC_GET_PTVAI(pmcr0) \
	((pmcr0 & MXC_CCM_PMCR0_PTVAI_MASK) >> MXC_CCM_PMCR0_PTVAI_OFFSET)

enum mxc_clk_out {
	CKO2,
	CKO1,
};

#endif				/* __ARCH_ARM_MACH_MXC91321_CRM_REGS_H__ */
