/*
 * wm8350.h  --  Power Managment Driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007 Wolfson Microelectronics PLC
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    8th Feb 2007   Initial version.
 *
 */

#ifndef __LINUX_REGULATOR_WM8350_H
#define __LINUX_REGULATOR_WM8350_H

/*
 * Register values.
 */
#define WM8350_RESET_ID                         0x00
#define WM8350_ID                               0x01
#define WM8350_SYSTEM_CONTROL_1                 0x03
#define WM8350_SYSTEM_CONTROL_2                 0x04
#define WM8350_SYSTEM_HIBERNATE                 0x05
#define WM8350_INTERFACE_CONTROL                0x06
#define WM8350_POWER_MGMT_1                     0x08
#define WM8350_POWER_MGMT_2                     0x09
#define WM8350_POWER_MGMT_3                     0x0A
#define WM8350_POWER_MGMT_4                     0x0B
#define WM8350_POWER_MGMT_5                     0x0C
#define WM8350_POWER_MGMT_6                     0x0D
#define WM8350_POWER_MGMT_7                     0x0E

#define WM8350_SYSTEM_INTERRUPTS                0x18
#define WM8350_INTERRUPT_STATUS_1               0x19
#define WM8350_INTERRUPT_STATUS_2               0x1A
#define WM8350_POWER_UP_INTERRUPT_STATUS        0x1B
#define WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS   0x1C
#define WM8350_OVER_CURRENT_INTERRUPT_STATUS    0x1D
#define WM8350_GPIO_INTERRUPT_STATUS            0x1E
#define WM8350_COMPARATOR_INTERRUPT_STATUS      0x1F
#define WM8350_SYSTEM_INTERRUPTS_MASK           0x20
#define WM8350_INTERRUPT_STATUS_1_MASK          0x21
#define WM8350_INTERRUPT_STATUS_2_MASK          0x22
#define WM8350_POWER_UP_INTERRUPT_STATUS_MASK   0x23
#define WM8350_UNDER_VOLTAGE_INTERRUPT_STATUS_MASK 0x24
#define WM8350_OVER_CURRENT_INTERRUPT_STATUS_MASK 0x25
#define WM8350_GPIO_INTERRUPT_STATUS_MASK       0x26
#define WM8350_COMPARATOR_INTERRUPT_STATUS_MASK 0x27

#define WM8350_MAX_REGISTER                     0xFF

/*
 * Field Definitions.
 */

/*
 * R0 (0x00) - Reset/ID
 */
#define WM8350_SW_RESET_CHIP_ID_MASK            0xFFFF  /* SW_RESET/CHIP_ID - [15:0] */

/*
 * R1 (0x01) - ID
 */
#define WM8350_CHIP_REV_MASK                    0x7000  /* CHIP_REV - [14:12] */
#define WM8350_CONF_STS_MASK                    0x0C00  /* CONF_STS - [11:10] */
#define WM8350_CUST_ID_MASK                     0x00FF  /* CUST_ID - [7:0] */

/*
 * R3 (0x03) - System Control 1
 */
#define WM8350_CHIP_ON                          0x8000  /* CHIP_ON */
#define WM8350_POWERCYCLE                       0x2000  /* POWERCYCLE */
#define WM8350_VCC_FAULT_OV                     0x1000  /* VCC_FAULT_OV */
#define WM8350_REG_RSTB_TIME_MASK               0x0C00  /* REG_RSTB_TIME - [11:10] */
#define WM8350_BG_SLEEP                         0x0200  /* BG_SLEEP */
#define WM8350_MEM_VALID                        0x0020  /* MEM_VALID */
#define WM8350_CHIP_SET_UP                      0x0010  /* CHIP_SET_UP */
#define WM8350_ON_DEB_T                         0x0008  /* ON_DEB_T */
#define WM8350_ON_POL                           0x0002  /* ON_POL */
#define WM8350_IRQ_POL                          0x0001  /* IRQ_POL */

/*
 * R4 (0x04) - System Control 2
 */
#define WM8350_USB_SUSPEND_8MA                  0x8000  /* USB_SUSPEND_8MA */
#define WM8350_USB_SUSPEND                      0x4000  /* USB_SUSPEND */
#define WM8350_USB_MSTR                         0x2000  /* USB_MSTR */
#define WM8350_USB_MSTR_SRC                     0x1000  /* USB_MSTR_SRC */
#define WM8350_USB_500MA                        0x0800  /* USB_500MA */
#define WM8350_USB_NOLIM                        0x0400  /* USB_NOLIM */
#define WM8350_WDOG_HIB_MODE                    0x0080  /* WDOG_HIB_MODE */
#define WM8350_WDOG_DEBUG                       0x0040  /* WDOG_DEBUG */
#define WM8350_WDOG_MODE_MASK                   0x0030  /* WDOG_MODE - [5:4] */
#define WM8350_WDOG_TO_MASK                     0x0007  /* WDOG_TO - [2:0] */

/*
 * R5 (0x05) - System Hibernate
 */
#define WM8350_HIBERNATE                        0x8000  /* HIBERNATE */
#define WM8350_WDOG_HIB_MODE                    0x0080  /* WDOG_HIB_MODE */
#define WM8350_REG_HIB_STARTUP_SEQ              0x0040  /* REG_HIB_STARTUP_SEQ */
#define WM8350_REG_RESET_HIB_MODE               0x0020  /* REG_RESET_HIB_MODE */
#define WM8350_RST_HIB_MODE                     0x0010  /* RST_HIB_MODE */
#define WM8350_IRQ_HIB_MODE                     0x0008  /* IRQ_HIB_MODE */
#define WM8350_MEMRST_HIB_MODE                  0x0004  /* MEMRST_HIB_MODE */
#define WM8350_PCCOMP_HIB_MODE                  0x0002  /* PCCOMP_HIB_MODE */
#define WM8350_TEMPMON_HIB_MODE                 0x0001  /* TEMPMON_HIB_MODE */

/*
 * R6 (0x06) - Interface Control
 */
#define WM8350_USE_DEV_PINS                     0x8000  /* USE_DEV_PINS */
#define WM8350_USE_DEV_PINS_MASK                0x8000  /* USE_DEV_PINS */
#define WM8350_USE_DEV_PINS_SHIFT                   15  /* USE_DEV_PINS */
#define WM8350_DEV_ADDR_MASK                    0x6000  /* DEV_ADDR - [14:13] */
#define WM8350_DEV_ADDR_SHIFT                       13  /* DEV_ADDR - [14:13] */
#define WM8350_CONFIG_DONE                      0x1000  /* CONFIG_DONE */
#define WM8350_CONFIG_DONE_MASK                 0x1000  /* CONFIG_DONE */
#define WM8350_CONFIG_DONE_SHIFT                    12  /* CONFIG_DONE */
#define WM8350_RECONFIG_AT_ON                   0x0800  /* RECONFIG_AT_ON */
#define WM8350_RECONFIG_AT_ON_MASK              0x0800  /* RECONFIG_AT_ON */
#define WM8350_RECONFIG_AT_ON_SHIFT                 11  /* RECONFIG_AT_ON */
#define WM8350_AUTOINC                          0x0200  /* AUTOINC */
#define WM8350_AUTOINC_MASK                     0x0200  /* AUTOINC */
#define WM8350_AUTOINC_SHIFT                         9  /* AUTOINC */
#define WM8350_ARA                              0x0100  /* ARA */
#define WM8350_ARA_MASK                         0x0100  /* ARA */
#define WM8350_ARA_SHIFT                             8  /* ARA */
#define WM8350_SPI_CFG                          0x0008  /* SPI_CFG */
#define WM8350_SPI_CFG_MASK                     0x0008  /* SPI_CFG */
#define WM8350_SPI_CFG_SHIFT                         3  /* SPI_CFG */
#define WM8350_SPI_4WIRE                        0x0004  /* SPI_4WIRE */
#define WM8350_SPI_4WIRE_MASK                   0x0004  /* SPI_4WIRE */
#define WM8350_SPI_4WIRE_SHIFT                       2  /* SPI_4WIRE */
#define WM8350_SPI_3WIRE                        0x0002  /* SPI_3WIRE */
#define WM8350_SPI_3WIRE_MASK                   0x0002  /* SPI_3WIRE */
#define WM8350_SPI_3WIRE_SHIFT                       1  /* SPI_3WIRE */

/* Bit values for R06 (0x06) */
#define WM8350_USE_DEV_PINS_PRIMARY                  0  /* Primary control interface */
#define WM8350_USE_DEV_PINS_DEV                      1  /* Secondary control interface */

#define WM8350_DEV_ADDR_34                           0  /* 2-wire address 0x1A/0x34 */
#define WM8350_DEV_ADDR_36                           1  /* 2-wire address 0x1B/0x36 */
#define WM8350_DEV_ADDR_3C                           2  /* 2-wire address 0x1E/0x3C */
#define WM8350_DEV_ADDR_3E                           3  /* 2-wire address 0x1F/0x3E */

#define WM8350_CONFIG_DONE_OFF                       0  /* Not programmed */
#define WM8350_CONFIG_DONE_DONE                      1  /* Programming complete */

#define WM8350_RECONFIG_AT_ON_OFF                    0  /* Don't reset registers on ON-event */
#define WM8350_RECONFIG_AT_ON_ON                     1  /* Reset registers on ON-event */

#define WM8350_AUTOINC_OFF                           0  /* No register auto-increment */
#define WM8350_AUTOINC_ON                            1  /* Enable register auto-increment */

#define WM8350_ARA_OFF                               0  /* Alert response address disabled */
#define WM8350_ARA_ON                                1  /* Alert response address enabled */

#define WM8350_SPI_CFG_CMOS                          0  /* SDOUT is CMOS */
#define WM8350_SPI_CFG_OD                            1  /* SDOUT is open-drain */

#define WM8350_SPI_4WIRE_3WIRE                       0  /* read data output on SDA */
#define WM8350_SPI_4WIRE_4WIRE                       1  /* read data output on SDOUT */

#define WM8350_SPI_3WIRE_I2C                         0  /* Select 2-wire interface */
#define WM8350_SPI_3WIRE_SPI                         1  /* Select 3-wire interface */

/*
 * R8 (0x08) - Power mgmt (1)
 */
#define WM8350_CODEC_ISEL_MASK                  0xC000  /* CODEC_ISEL - [15:14] */
#define WM8350_VBUFEN                           0x2000  /* VBUFEN */
#define WM8350_OUTPUT_DRAIN_EN                  0x0400  /* OUTPUT_DRAIN_EN */
#define WM8350_MIC_DET_ENA                      0x0100  /* MIC_DET_ENA */
#define WM8350_BIASEN                           0x0020  /* BIASEN */
#define WM8350_MICBEN                           0x0010  /* MICBEN */
#define WM8350_VMIDEN                           0x0004  /* VMIDEN */
#define WM8350_VMID_MASK                        0x0003  /* VMID - [1:0] */
#define WM8350_VMID_SHIFT                            0  /* VMID - [1:0] */

/* Bit values for R08 (0x08) */
#define WM8350_CODEC_ISEL_1_5                        0  /* 0 = x1.5 */
#define WM8350_CODEC_ISEL_1_0                        1  /* 0 = x1.0 */
#define WM8350_CODEC_ISEL_0_75                       2  /* 0 = x0.75 */
#define WM8350_CODEC_ISEL_0_5                        3  /* 0 = x0.5 */

#define WM8350_VMID_OFF                              0  /* 0 = 500k resistor string */
#define WM8350_VMID_500K                             1  /* 1 = 160k resistor string */
#define WM8350_VMID_100K                             2  /* 2 = 80k resistor string */
#define WM8350_VMID_10K                              3  /* 3 = 40k resistor string */

/*
 * R9 (0x09) - Power mgmt (2)
 */
#define WM8350_IN3R_ENA                         0x0800  /* IN3R_ENA */
#define WM8350_IN3L_ENA                         0x0400  /* IN3L_ENA */
#define WM8350_INR_ENA                          0x0200  /* INR_ENA */
#define WM8350_INL_ENA                          0x0100  /* INL_ENA */
#define WM8350_MIXINR_ENA                       0x0080  /* MIXINR_ENA */
#define WM8350_MIXINL_ENA                       0x0040  /* MIXINL_ENA */
#define WM8350_OUT4_ENA                         0x0020  /* OUT4_ENA */
#define WM8350_OUT3_ENA                         0x0010  /* OUT3_ENA */
#define WM8350_MIXOUTR_ENA                      0x0002  /* MIXOUTR_ENA */
#define WM8350_MIXOUTL_ENA                      0x0001  /* MIXOUTL_ENA */

/*
 * R10 (0x0A) - Power mgmt (3)
 */
#define WM8350_IN3R_TO_OUT2R                    0x0080  /* IN3R_TO_OUT2R */
#define WM8350_OUT2R_ENA                        0x0008  /* OUT2R_ENA */
#define WM8350_OUT2L_ENA                        0x0004  /* OUT2L_ENA */
#define WM8350_OUT1R_ENA                        0x0002  /* OUT1R_ENA */
#define WM8350_OUT1L_ENA                        0x0001  /* OUT1L_ENA */

/*
 * R11 (0x0B) - Power mgmt (4)
 */
#define WM8350_SYSCLK_ENA                       0x4000  /* SYSCLK_ENA */
#define WM8350_ADC_HPF_ENA                      0x2000  /* ADC_HPF_ENA */
#define WM8350_FLL_ENA                          0x0800  /* FLL_ENA */
#define WM8350_FLL_OSC_ENA                      0x0400  /* FLL_OSC_ENA */
#define WM8350_TOCLK_ENA                        0x0100  /* TOCLK_ENA */
#define WM8350_DACR_ENA                         0x0020  /* DACR_ENA */
#define WM8350_DACL_ENA                         0x0010  /* DACL_ENA */
#define WM8350_ADCR_ENA                         0x0008  /* ADCR_ENA */
#define WM8350_ADCL_ENA                         0x0004  /* ADCL_ENA */

/*
 * R12 (0x0C) - Power mgmt (5)
 */
#define WM8350_CODEC_ENA                        0x1000  /* CODEC_ENA */
#define WM8350_RTC_TICK_ENA                     0x0800  /* RTC_TICK_ENA */
#define WM8350_OSC32K_ENA                       0x0400  /* OSC32K_ENA */
#define WM8350_CHG_ENA                          0x0200  /* CHG_ENA */
#define WM8350_ACC_DET_ENA                      0x0100  /* ACC_DET_ENA */
#define WM8350_AUXADC_ENA                       0x0080  /* AUXADC_ENA */
#define WM8350_DCMP4_ENA                        0x0008  /* DCMP4_ENA */
#define WM8350_DCMP3_ENA                        0x0004  /* DCMP3_ENA */
#define WM8350_DCMP2_ENA                        0x0002  /* DCMP2_ENA */
#define WM8350_DCMP1_ENA                        0x0001  /* DCMP1_ENA */

/*
 * R13 (0x0D) - Power mgmt (6)
 */
#define WM8350_LS_ENA                           0x8000  /* LS_ENA */
#define WM8350_LDO4_ENA                         0x0800  /* LDO4_ENA */
#define WM8350_LDO3_ENA                         0x0400  /* LDO3_ENA */
#define WM8350_LDO2_ENA                         0x0200  /* LDO2_ENA */
#define WM8350_LDO1_ENA                         0x0100  /* LDO1_ENA */
#define WM8350_DC6_ENA                          0x0020  /* DC6_ENA */
#define WM8350_DC5_ENA                          0x0010  /* DC5_ENA */
#define WM8350_DC4_ENA                          0x0008  /* DC4_ENA */
#define WM8350_DC3_ENA                          0x0004  /* DC3_ENA */
#define WM8350_DC2_ENA                          0x0002  /* DC2_ENA */
#define WM8350_DC1_ENA                          0x0001  /* DC1_ENA */

/*
 * R14 (0x0E) - Power mgmt (7)
 */
#define WM8350_CS2_ENA                          0x0002  /* CS2_ENA */
#define WM8350_CS1_ENA                          0x0001  /* CS1_ENA */



/*
 * R24 (0x18) - System Interrupts
 */
#define WM8350_OC_INT                           0x2000  /* OC_INT */
#define WM8350_UV_INT                           0x1000  /* UV_INT */
#define WM8350_PUTO_INT                         0x0800  /* PUTO_INT */
#define WM8350_CS_INT                           0x0200  /* CS_INT */
#define WM8350_EXT_INT                          0x0100  /* EXT_INT */
#define WM8350_CODEC_INT                        0x0080  /* CODEC_INT */
#define WM8350_GP_INT                           0x0040  /* GP_INT */
#define WM8350_AUXADC_INT                       0x0020  /* AUXADC_INT */
#define WM8350_RTC_INT                          0x0010  /* RTC_INT */
#define WM8350_SYS_INT                          0x0008  /* SYS_INT */
#define WM8350_CHG_INT                          0x0004  /* CHG_INT */
#define WM8350_USB_INT                          0x0002  /* USB_INT */
#define WM8350_WKUP_INT                         0x0001  /* WKUP_INT */

/*
 * R25 (0x19) - Interrupt Status 1
 */
#define WM8350_CHG_BAT_HOT_EINT                 0x8000  /* CHG_BAT_HOT_EINT */
#define WM8350_CHG_BAT_COLD_EINT                0x4000  /* CHG_BAT_COLD_EINT */
#define WM8350_CHG_BAT_FAIL_EINT                0x2000  /* CHG_BAT_FAIL_EINT */
#define WM8350_CHG_TO_EINT                      0x1000  /* CHG_TO_EINT */
#define WM8350_CHG_END_EINT                     0x0800  /* CHG_END_EINT */
#define WM8350_CHG_START_EINT                   0x0400  /* CHG_START_EINT */
#define WM8350_CHG_FAST_RDY_EINT                0x0200  /* CHG_FAST_RDY_EINT */
#define WM8350_RTC_PER_EINT                     0x0080  /* RTC_PER_EINT */
#define WM8350_RTC_SEC_EINT                     0x0040  /* RTC_SEC_EINT */
#define WM8350_RTC_ALM_EINT                     0x0020  /* RTC_ALM_EINT */
#define WM8350_CHG_VBATT_LT_3P9_EINT            0x0004  /* CHG_VBATT_LT_3P9_EINT */
#define WM8350_CHG_VBATT_LT_3P1_EINT            0x0002  /* CHG_VBATT_LT_3P1_EINT */
#define WM8350_CHG_VBATT_LT_2P85_EINT           0x0001  /* CHG_VBATT_LT_2P85_EINT */

/*
 * R26 (0x1A) - Interrupt Status 2
 */
#define WM8350_CS1_EINT                         0x2000  /* CS1_EINT */
#define WM8350_CS2_EINT                         0x1000  /* CS2_EINT */
#define WM8350_USB_LIMIT_EINT                   0x0400  /* USB_LIMIT_EINT */
#define WM8350_AUXADC_DATARDY_EINT              0x0100  /* AUXADC_DATARDY_EINT */
#define WM8350_AUXADC_DCOMP4_EINT               0x0080  /* AUXADC_DCOMP4_EINT */
#define WM8350_AUXADC_DCOMP3_EINT               0x0040  /* AUXADC_DCOMP3_EINT */
#define WM8350_AUXADC_DCOMP2_EINT               0x0020  /* AUXADC_DCOMP2_EINT */
#define WM8350_AUXADC_DCOMP1_EINT               0x0010  /* AUXADC_DCOMP1_EINT */
#define WM8350_SYS_HYST_COMP_FAIL_EINT          0x0008  /* SYS_HYST_COMP_FAIL_EINT */
#define WM8350_SYS_CHIP_GT115_EINT              0x0004  /* SYS_CHIP_GT115_EINT */
#define WM8350_SYS_CHIP_GT140_EINT              0x0002  /* SYS_CHIP_GT140_EINT */
#define WM8350_SYS_WDOG_TO_EINT                 0x0001  /* SYS_WDOG_TO_EINT */

/*
 * R27 (0x1B) - Power Up Interrupt Status
 */
#define WM8350_PUTO_LDO4_EINT                   0x0800  /* PUTO_LDO4_EINT */
#define WM8350_PUTO_LDO3_EINT                   0x0400  /* PUTO_LDO3_EINT */
#define WM8350_PUTO_LDO2_EINT                   0x0200  /* PUTO_LDO2_EINT */
#define WM8350_PUTO_LDO1_EINT                   0x0100  /* PUTO_LDO1_EINT */
#define WM8350_PUTO_DC6_EINT                    0x0020  /* PUTO_DC6_EINT */
#define WM8350_PUTO_DC5_EINT                    0x0010  /* PUTO_DC5_EINT */
#define WM8350_PUTO_DC4_EINT                    0x0008  /* PUTO_DC4_EINT */
#define WM8350_PUTO_DC3_EINT                    0x0004  /* PUTO_DC3_EINT */
#define WM8350_PUTO_DC2_EINT                    0x0002  /* PUTO_DC2_EINT */
#define WM8350_PUTO_DC1_EINT                    0x0001  /* PUTO_DC1_EINT */

/*
 * R28 (0x1C) - Under Voltage Interrupt status
 */
#define WM8350_UV_LDO4_EINT                     0x0800  /* UV_LDO4_EINT */
#define WM8350_UV_LDO3_EINT                     0x0400  /* UV_LDO3_EINT */
#define WM8350_UV_LDO2_EINT                     0x0200  /* UV_LDO2_EINT */
#define WM8350_UV_LDO1_EINT                     0x0100  /* UV_LDO1_EINT */
#define WM8350_UV_DC6_EINT                      0x0020  /* UV_DC6_EINT */
#define WM8350_UV_DC5_EINT                      0x0010  /* UV_DC5_EINT */
#define WM8350_UV_DC4_EINT                      0x0008  /* UV_DC4_EINT */
#define WM8350_UV_DC3_EINT                      0x0004  /* UV_DC3_EINT */
#define WM8350_UV_DC2_EINT                      0x0002  /* UV_DC2_EINT */
#define WM8350_UV_DC1_EINT                      0x0001  /* UV_DC1_EINT */

/*
 * R29 (0x1D) - Over Current Interrupt status
 */
#define WM8350_OC_LS_EINT                       0x8000  /* OC_LS_EINT */

/*
 * R30 (0x1E) - GPIO Interrupt Status
 */
#define WM8350_GP12_EINT                        0x1000  /* GP12_EINT */
#define WM8350_GP11_EINT                        0x0800  /* GP11_EINT */
#define WM8350_GP10_EINT                        0x0400  /* GP10_EINT */
#define WM8350_GP9_EINT                         0x0200  /* GP9_EINT */
#define WM8350_GP8_EINT                         0x0100  /* GP8_EINT */
#define WM8350_GP7_EINT                         0x0080  /* GP7_EINT */
#define WM8350_GP6_EINT                         0x0040  /* GP6_EINT */
#define WM8350_GP5_EINT                         0x0020  /* GP5_EINT */
#define WM8350_GP4_EINT                         0x0010  /* GP4_EINT */
#define WM8350_GP3_EINT                         0x0008  /* GP3_EINT */
#define WM8350_GP2_EINT                         0x0004  /* GP2_EINT */
#define WM8350_GP1_EINT                         0x0002  /* GP1_EINT */
#define WM8350_GP0_EINT                         0x0001  /* GP0_EINT */

/*
 * R31 (0x1F) - Comparator Interrupt Status
 */
#define WM8350_EXT_USB_FB_EINT                  0x8000  /* EXT_USB_FB_EINT */
#define WM8350_EXT_WALL_FB_EINT                 0x4000  /* EXT_WALL_FB_EINT */
#define WM8350_EXT_BAT_FB_EINT                  0x2000  /* EXT_BAT_FB_EINT */
#define WM8350_CODEC_JCK_DET_L_EINT             0x0800  /* CODEC_JCK_DET_L_EINT */
#define WM8350_CODEC_JCK_DET_R_EINT             0x0400  /* CODEC_JCK_DET_R_EINT */
#define WM8350_CODEC_MICSCD_EINT                0x0200  /* CODEC_MICSCD_EINT */
#define WM8350_CODEC_MICD_EINT                  0x0100  /* CODEC_MICD_EINT */
#define WM8350_WKUP_OFF_STATE_EINT              0x0040  /* WKUP_OFF_STATE_EINT */
#define WM8350_WKUP_HIB_STATE_EINT              0x0020  /* WKUP_HIB_STATE_EINT */
#define WM8350_WKUP_CONV_FAULT_EINT             0x0010  /* WKUP_CONV_FAULT_EINT */
#define WM8350_WKUP_WDOG_RST_EINT               0x0008  /* WKUP_WDOG_RST_EINT */
#define WM8350_WKUP_GP_PWR_ON_EINT              0x0004  /* WKUP_GP_PWR_ON_EINT */
#define WM8350_WKUP_ONKEY_EINT                  0x0002  /* WKUP_ONKEY_EINT */
#define WM8350_WKUP_GP_WAKEUP_EINT              0x0001  /* WKUP_GP_WAKEUP_EINT */

/*
 * R32 (0x20) - System Interrupts Mask
 */
#define WM8350_IM_OC_INT                        0x2000  /* IM_OC_INT */
#define WM8350_IM_UV_INT                        0x1000  /* IM_UV_INT */
#define WM8350_IM_PUTO_INT                      0x0800  /* IM_PUTO_INT */
#define WM8350_IM_SPARE_INT                     0x0400  /* IM_SPARE_INT */
#define WM8350_IM_CS_INT                        0x0200  /* IM_CS_INT */
#define WM8350_IM_EXT_INT                       0x0100  /* IM_EXT_INT */
#define WM8350_IM_CODEC_INT                     0x0080  /* IM_CODEC_INT */
#define WM8350_IM_GP_INT                        0x0040  /* IM_GP_INT */
#define WM8350_IM_AUXADC_INT                    0x0020  /* IM_AUXADC_INT */
#define WM8350_IM_RTC_INT                       0x0010  /* IM_RTC_INT */
#define WM8350_IM_SYS_INT                       0x0008  /* IM_SYS_INT */
#define WM8350_IM_CHG_INT                       0x0004  /* IM_CHG_INT */
#define WM8350_IM_USB_INT                       0x0002  /* IM_USB_INT */
#define WM8350_IM_WKUP_INT                      0x0001  /* IM_WKUP_INT */

/*
 * R33 (0x21) - Interrupt Status 1 Mask
 */
#define WM8350_IM_CHG_BAT_HOT_EINT              0x8000  /* IM_CHG_BAT_HOT_EINT */
#define WM8350_IM_CHG_BAT_COLD_EINT             0x4000  /* IM_CHG_BAT_COLD_EINT */
#define WM8350_IM_CHG_BAT_FAIL_EINT             0x2000  /* IM_CHG_BAT_FAIL_EINT */
#define WM8350_IM_CHG_TO_EINT                   0x1000  /* IM_CHG_TO_EINT */
#define WM8350_IM_CHG_END_EINT                  0x0800  /* IM_CHG_END_EINT */
#define WM8350_IM_CHG_START_EINT                0x0400  /* IM_CHG_START_EINT */
#define WM8350_IM_CHG_FAST_RDY_EINT             0x0200  /* IM_CHG_FAST_RDY_EINT */
#define WM8350_IM_RTC_PER_EINT                  0x0080  /* IM_RTC_PER_EINT */
#define WM8350_IM_RTC_SEC_EINT                  0x0040  /* IM_RTC_SEC_EINT */
#define WM8350_IM_RTC_ALM_EINT                  0x0020  /* IM_RTC_ALM_EINT */
#define WM8350_IM_CHG_VBATT_LT_3P9_EINT         0x0004  /* IM_CHG_VBATT_LT_3P9_EINT */
#define WM8350_IM_CHG_VBATT_LT_3P1_EINT         0x0002  /* IM_CHG_VBATT_LT_3P1_EINT */
#define WM8350_IM_CHG_VBATT_LT_2P85_EINT        0x0001  /* IM_CHG_VBATT_LT_2P85_EINT */

/*
 * R34 (0x22) - Interrupt Status 2 Mask
 */
#define WM8350_IM_SPARE2_EINT                   0x8000  /* IM_SPARE2_EINT */
#define WM8350_IM_SPARE1_EINT                   0x4000  /* IM_SPARE1_EINT */
#define WM8350_IM_CS1_EINT                      0x2000  /* IM_CS1_EINT */
#define WM8350_IM_CS2_EINT                      0x1000  /* IM_CS2_EINT */
#define WM8350_IM_USB_LIMIT_EINT                0x0400  /* IM_USB_LIMIT_EINT */
#define WM8350_IM_AUXADC_DATARDY_EINT           0x0100  /* IM_AUXADC_DATARDY_EINT */
#define WM8350_IM_AUXADC_DCOMP4_EINT            0x0080  /* IM_AUXADC_DCOMP4_EINT */
#define WM8350_IM_AUXADC_DCOMP3_EINT            0x0040  /* IM_AUXADC_DCOMP3_EINT */
#define WM8350_IM_AUXADC_DCOMP2_EINT            0x0020  /* IM_AUXADC_DCOMP2_EINT */
#define WM8350_IM_AUXADC_DCOMP1_EINT            0x0010  /* IM_AUXADC_DCOMP1_EINT */
#define WM8350_IM_SYS_HYST_COMP_FAIL_EINT       0x0008  /* IM_SYS_HYST_COMP_FAIL_EINT */
#define WM8350_IM_SYS_CHIP_GT115_EINT           0x0004  /* IM_SYS_CHIP_GT115_EINT */
#define WM8350_IM_SYS_CHIP_GT140_EINT           0x0002  /* IM_SYS_CHIP_GT140_EINT */
#define WM8350_IM_SYS_WDOG_TO_EINT              0x0001  /* IM_SYS_WDOG_TO_EINT */

/*
 * R35 (0x23) - Power Up Interrupt Status Mask
 */
#define WM8350_IM_PUTO_LDO4_EINT                0x0800  /* IM_PUTO_LDO4_EINT */
#define WM8350_IM_PUTO_LDO3_EINT                0x0400  /* IM_PUTO_LDO3_EINT */
#define WM8350_IM_PUTO_LDO2_EINT                0x0200  /* IM_PUTO_LDO2_EINT */
#define WM8350_IM_PUTO_LDO1_EINT                0x0100  /* IM_PUTO_LDO1_EINT */
#define WM8350_IM_PUTO_DC6_EINT                 0x0020  /* IM_PUTO_DC6_EINT */
#define WM8350_IM_PUTO_DC5_EINT                 0x0010  /* IM_PUTO_DC5_EINT */
#define WM8350_IM_PUTO_DC4_EINT                 0x0008  /* IM_PUTO_DC4_EINT */
#define WM8350_IM_PUTO_DC3_EINT                 0x0004  /* IM_PUTO_DC3_EINT */
#define WM8350_IM_PUTO_DC2_EINT                 0x0002  /* IM_PUTO_DC2_EINT */
#define WM8350_IM_PUTO_DC1_EINT                 0x0001  /* IM_PUTO_DC1_EINT */

/*
 * R36 (0x24) - Under Voltage Interrupt status Mask
 */
#define WM8350_IM_UV_LDO4_EINT                  0x0800  /* IM_UV_LDO4_EINT */
#define WM8350_IM_UV_LDO3_EINT                  0x0400  /* IM_UV_LDO3_EINT */
#define WM8350_IM_UV_LDO2_EINT                  0x0200  /* IM_UV_LDO2_EINT */
#define WM8350_IM_UV_LDO1_EINT                  0x0100  /* IM_UV_LDO1_EINT */
#define WM8350_IM_UV_DC6_EINT                   0x0020  /* IM_UV_DC6_EINT */
#define WM8350_IM_UV_DC5_EINT                   0x0010  /* IM_UV_DC5_EINT */
#define WM8350_IM_UV_DC4_EINT                   0x0008  /* IM_UV_DC4_EINT */
#define WM8350_IM_UV_DC3_EINT                   0x0004  /* IM_UV_DC3_EINT */
#define WM8350_IM_UV_DC2_EINT                   0x0002  /* IM_UV_DC2_EINT */
#define WM8350_IM_UV_DC1_EINT                   0x0001  /* IM_UV_DC1_EINT */

/*
 * R37 (0x25) - Over Current Interrupt status Mask
 */
#define WM8350_IM_OC_LS_EINT                    0x8000  /* IM_OC_LS_EINT */

/*
 * R38 (0x26) - GPIO Interrupt Status Mask
 */
#define WM8350_IM_GP12_EINT                     0x1000  /* IM_GP12_EINT */
#define WM8350_IM_GP11_EINT                     0x0800  /* IM_GP11_EINT */
#define WM8350_IM_GP10_EINT                     0x0400  /* IM_GP10_EINT */
#define WM8350_IM_GP9_EINT                      0x0200  /* IM_GP9_EINT */
#define WM8350_IM_GP8_EINT                      0x0100  /* IM_GP8_EINT */
#define WM8350_IM_GP7_EINT                      0x0080  /* IM_GP7_EINT */
#define WM8350_IM_GP6_EINT                      0x0040  /* IM_GP6_EINT */
#define WM8350_IM_GP5_EINT                      0x0020  /* IM_GP5_EINT */
#define WM8350_IM_GP4_EINT                      0x0010  /* IM_GP4_EINT */
#define WM8350_IM_GP3_EINT                      0x0008  /* IM_GP3_EINT */
#define WM8350_IM_GP2_EINT                      0x0004  /* IM_GP2_EINT */
#define WM8350_IM_GP1_EINT                      0x0002  /* IM_GP1_EINT */
#define WM8350_IM_GP0_EINT                      0x0001  /* IM_GP0_EINT */

/*
 * R39 (0x27) - Comparator Interrupt Status Mask
 */
#define WM8350_IM_EXT_USB_FB_EINT               0x8000  /* IM_EXT_USB_FB_EINT */
#define WM8350_IM_EXT_WALL_FB_EINT              0x4000  /* IM_EXT_WALL_FB_EINT */
#define WM8350_IM_EXT_BAT_FB_EINT               0x2000  /* IM_EXT_BAT_FB_EINT */
#define WM8350_IM_CODEC_JCK_DET_L_EINT          0x0800  /* IM_CODEC_JCK_DET_L_EINT */
#define WM8350_IM_CODEC_JCK_DET_R_EINT          0x0400  /* IM_CODEC_JCK_DET_R_EINT */
#define WM8350_IM_CODEC_MICSCD_EINT             0x0200  /* IM_CODEC_MICSCD_EINT */
#define WM8350_IM_CODEC_MICD_EINT               0x0100  /* IM_CODEC_MICD_EINT */
#define WM8350_IM_WKUP_OFF_STATE_EINT           0x0040  /* IM_WKUP_OFF_STATE_EINT */
#define WM8350_IM_WKUP_HIB_STATE_EINT           0x0020  /* IM_WKUP_HIB_STATE_EINT */
#define WM8350_IM_WKUP_CONV_FAULT_EINT          0x0010  /* IM_WKUP_CONV_FAULT_EINT */
#define WM8350_IM_WKUP_WDOG_RST_EINT            0x0008  /* IM_WKUP_WDOG_RST_EINT */
#define WM8350_IM_WKUP_GP_PWR_ON_EINT           0x0004  /* IM_WKUP_GP_PWR_ON_EINT */
#define WM8350_IM_WKUP_ONKEY_EINT               0x0002  /* IM_WKUP_ONKEY_EINT */
#define WM8350_IM_WKUP_GP_WAKEUP_EINT           0x0001  /* IM_WKUP_GP_WAKEUP_EINT */

/*
 * R220 (0xDC) - RAM BIST 1
 */
#define WM8350_READ_STATUS                      0x0800  /* READ_STATUS */
#define WM8350_TSTRAM_CLK                       0x0100  /* TSTRAM_CLK */
#define WM8350_TSTRAM_CLK_ENA                   0x0080  /* TSTRAM_CLK_ENA */
#define WM8350_STARTSEQ                         0x0040  /* STARTSEQ */
#define WM8350_READ_SRC                         0x0020  /* READ_SRC */
#define WM8350_COUNT_DIR                        0x0010  /* COUNT_DIR */
#define WM8350_TSTRAM_MODE_MASK                 0x000E  /* TSTRAM_MODE - [3:1] */
#define WM8350_TSTRAM_ENA                       0x0001  /* TSTRAM_ENA */

/*
 * R225 (0xE1) - DCDC/LDO status
 */
#define WM8350_LS_STS                           0x8000  /* LS_STS */
#define WM8350_LDO4_STS                         0x0800  /* LDO4_STS */
#define WM8350_LDO3_STS                         0x0400  /* LDO3_STS */
#define WM8350_LDO2_STS                         0x0200  /* LDO2_STS */
#define WM8350_LDO1_STS                         0x0100  /* LDO1_STS */
#define WM8350_DC6_STS                          0x0020  /* DC6_STS */
#define WM8350_DC5_STS                          0x0010  /* DC5_STS */
#define WM8350_DC4_STS                          0x0008  /* DC4_STS */
#define WM8350_DC3_STS                          0x0004  /* DC3_STS */
#define WM8350_DC2_STS                          0x0002  /* DC2_STS */
#define WM8350_DC1_STS                          0x0001  /* DC1_STS */

/*
 * Default values.
 */
#define WM8350_CONFIG_BANKS                     4

/* Bank 0 */
#define WM8350_REGISTER_DEFAULTS_0 \
{ \
    0x17FF,     /* R0   - Reset/ID */ \
    0x1000,     /* R1   - ID */ \
    0x0000,     /* R2 */ \
    0x1002,     /* R3   - System Control 1 */ \
    0x0004,     /* R4   - System Control 2 */ \
    0x0000,     /* R5   - System Hibernate */ \
    0x8A00,     /* R6   - Interface Control */ \
    0x0000,     /* R7 */ \
    0x8000,     /* R8   - Power mgmt (1) */ \
    0x0000,     /* R9   - Power mgmt (2) */ \
    0x0000,     /* R10  - Power mgmt (3) */ \
    0x2000,     /* R11  - Power mgmt (4) */ \
    0x0E00,     /* R12  - Power mgmt (5) */ \
    0x0000,     /* R13  - Power mgmt (6) */ \
    0x0000,     /* R14  - Power mgmt (7) */ \
    0x0000,     /* R15 */ \
    0x0000,     /* R16  - RTC Seconds/Minutes */ \
    0x0100,     /* R17  - RTC Hours/Day */ \
    0x0101,     /* R18  - RTC Date/Month */ \
    0x1400,     /* R19  - RTC Year */ \
    0x0000,     /* R20  - Alarm Seconds/Minutes */ \
    0x0000,     /* R21  - Alarm Hours/Day */ \
    0x0000,     /* R22  - Alarm Date/Month */ \
    0x0320,     /* R23  - RTC Time Control */ \
    0x0000,     /* R24  - System Interrupts */ \
    0x0000,     /* R25  - Interrupt Status 1 */ \
    0x0000,     /* R26  - Interrupt Status 2 */ \
    0x0000,     /* R27  - Power Up Interrupt Status */ \
    0x0000,     /* R28  - Under Voltage Interrupt status */ \
    0x0000,     /* R29  - Over Current Interrupt status */ \
    0x0000,     /* R30  - GPIO Interrupt Status */ \
    0x0000,     /* R31  - Comparator Interrupt Status */ \
    0x3FFF,     /* R32  - System Interrupts Mask */ \
    0x0000,     /* R33  - Interrupt Status 1 Mask */ \
    0x0000,     /* R34  - Interrupt Status 2 Mask */ \
    0x0000,     /* R35  - Power Up Interrupt Status Mask */ \
    0x0000,     /* R36  - Under Voltage Interrupt status Mask */ \
    0x0000,     /* R37  - Over Current Interrupt status Mask */ \
    0x0000,     /* R38  - GPIO Interrupt Status Mask */ \
    0x0000,     /* R39  - Comparator Interrupt Status Mask */ \
    0x0040,     /* R40  - Clock Control 1 */ \
    0x0000,     /* R41  - Clock Control 2 */ \
    0x3B00,     /* R42  - FLL Control 1 */ \
    0x7086,     /* R43  - FLL Control 2 */ \
    0xC226,     /* R44  - FLL Control 3 */ \
    0x0000,     /* R45  - FLL Control 4 */ \
    0x0000,     /* R46 */ \
    0x0000,     /* R47 */ \
    0x0000,     /* R48  - DAC Control */ \
    0x0000,     /* R49 */ \
    0x00C0,     /* R50  - DAC Digital Volume L */ \
    0x00C0,     /* R51  - DAC Digital Volume R */ \
    0x0000,     /* R52 */ \
    0x0040,     /* R53  - DAC LR Rate */ \
    0x0000,     /* R54  - DAC Clock Control */ \
    0x0000,     /* R55 */ \
    0x0000,     /* R56 */ \
    0x0000,     /* R57 */ \
    0x4000,     /* R58  - DAC Mute */ \
    0x0000,     /* R59  - DAC Mute Volume */ \
    0x0000,     /* R60  - DAC Side */ \
    0x0000,     /* R61 */ \
    0x0000,     /* R62 */ \
    0x0000,     /* R63 */ \
    0x8000,     /* R64  - ADC Control */ \
    0x0000,     /* R65 */ \
    0x00C0,     /* R66  - ADC Digital Volume L */ \
    0x00C0,     /* R67  - ADC Digital Volume R */ \
    0x0000,     /* R68  - ADC Divider */ \
    0x0000,     /* R69 */ \
    0x0040,     /* R70  - ADC LR Rate */ \
    0x0000,     /* R71 */ \
    0x0303,     /* R72  - Input Control */ \
    0x0000,     /* R73  - IN3 Input Control */ \
    0x0000,     /* R74  - Mic Bias Control */ \
    0x0000,     /* R75 */ \
    0x0000,     /* R76  - Output Control */ \
    0x0000,     /* R77  - Jack Detect */ \
    0x0000,     /* R78  - Anti Pop Control */ \
    0x0000,     /* R79 */ \
    0x0040,     /* R80  - Left Input Volume */ \
    0x0040,     /* R81  - Right Input Volume */ \
    0x0000,     /* R82 */ \
    0x0000,     /* R83 */ \
    0x0000,     /* R84 */ \
    0x0000,     /* R85 */ \
    0x0000,     /* R86 */ \
    0x0000,     /* R87 */ \
    0x0800,     /* R88  - Left Mixer Control */ \
    0x1000,     /* R89  - Right Mixer Control */ \
    0x0000,     /* R90 */ \
    0x0000,     /* R91 */ \
    0x0000,     /* R92  - OUT3 Mixer Control */ \
    0x0000,     /* R93  - OUT4 Mixer Control */ \
    0x0000,     /* R94 */ \
    0x0000,     /* R95 */ \
    0x0000,     /* R96  - Output Left Mixer Volume */ \
    0x0000,     /* R97  - Output Right Mixer Volume */ \
    0x0000,     /* R98  - Input Mixer Volume L */ \
    0x0000,     /* R99  - Input Mixer Volume R */ \
    0x0000,     /* R100 - Input Mixer Volume */ \
    0x0000,     /* R101 */ \
    0x0000,     /* R102 */ \
    0x0000,     /* R103 */ \
    0x00E4,     /* R104 - LOUT1 Volume */ \
    0x00E4,     /* R105 - ROUT1 Volume */ \
    0x00E4,     /* R106 - LOUT2 Volume */ \
    0x02E4,     /* R107 - ROUT2 Volume */ \
    0x0000,     /* R108 */ \
    0x0000,     /* R109 */ \
    0x0000,     /* R110 */ \
    0x0000,     /* R111 - BEEP Volume */ \
    0x0A00,     /* R112 - AI Formating */ \
    0x0000,     /* R113 - ADC DAC COMP */ \
    0x0020,     /* R114 - AI ADC Control */ \
    0x0020,     /* R115 - AI DAC Control */ \
    0x0000,     /* R116 - AIF Test */ \
    0x0000,     /* R117 */ \
    0x0000,     /* R118 */ \
    0x0000,     /* R119 */ \
    0x0000,     /* R120 */ \
    0x0000,     /* R121 */ \
    0x0000,     /* R122 */ \
    0x0000,     /* R123 */ \
    0x0000,     /* R124 */ \
    0x0000,     /* R125 */ \
    0x0000,     /* R126 */ \
    0x0000,     /* R127 */ \
    0x1FFF,     /* R128 - GPIO Debounce */ \
    0x0000,     /* R129 - GPIO Pin pull up Control */ \
    0x03FC,     /* R130 - GPIO Pull down Control */ \
    0x0000,     /* R131 - GPIO Interrupt Mode */ \
    0x0000,     /* R132 */ \
    0x0000,     /* R133 - GPIO Control */ \
    0x0FFC,     /* R134 - GPIO Configuration (i/o) */ \
    0x0FFC,     /* R135 - GPIO Pin Polarity / Type */ \
    0x0000,     /* R136 */ \
    0x0000,     /* R137 */ \
    0x0000,     /* R138 */ \
    0x0000,     /* R139 */ \
    0x0013,     /* R140 - GPIO Function Select 1 */ \
    0x0000,     /* R141 - GPIO Function Select 2 */ \
    0x0000,     /* R142 - GPIO Function Select 3 */ \
    0x0003,     /* R143 - GPIO Function Select 4 */ \
    0x0000,     /* R144 - Digitiser Control (1) */ \
    0x0002,     /* R145 - Digitiser Control (2) */ \
    0x0000,     /* R146 */ \
    0x0000,     /* R147 */ \
    0x0000,     /* R148 */ \
    0x0000,     /* R149 */ \
    0x0000,     /* R150 */ \
    0x0000,     /* R151 */ \
    0x7000,     /* R152 - AUX1 Readback */ \
    0x7000,     /* R153 - AUX2 Readback */ \
    0x7000,     /* R154 - AUX3 Readback */ \
    0x7000,     /* R155 - AUX4 Readback */ \
    0x0000,     /* R156 - USB Voltage Readback */ \
    0x0000,     /* R157 - LINE Voltage Readback */ \
    0x0000,     /* R158 - BATT Voltage Readback */ \
    0x0000,     /* R159 - Chip Temp Readback */ \
    0x0000,     /* R160 */ \
    0x0000,     /* R161 */ \
    0x0000,     /* R162 */ \
    0x0000,     /* R163 - Generic Comparator Control */ \
    0x0000,     /* R164 - Generic comparator 1 */ \
    0x0000,     /* R165 - Generic comparator 2 */ \
    0x0000,     /* R166 - Generic comparator 3 */ \
    0x0000,     /* R167 - Generic comparator 4 */ \
    0xA00F,     /* R168 - Battery Charger Control 1 */ \
    0x0B06,     /* R169 - Battery Charger Control 2 */ \
    0x0000,     /* R170 - Battery Charger Control 3 */ \
    0x0000,     /* R171 */ \
    0x0000,     /* R172 - Current Sink Driver A */ \
    0x0000,     /* R173 - CSA Flash control */ \
    0x0000,     /* R174 - Current Sink Driver B */ \
    0x0000,     /* R175 - CSB Flash control */ \
    0x0000,     /* R176 - DCDC/LDO requested */ \
    0x002D,     /* R177 - DCDC Active options */ \
    0x0000,     /* R178 - DCDC Sleep options */ \
    0x0025,     /* R179 - Power-check comparator */ \
    0x000E,     /* R180 - DCDC1 Control */ \
    0x0000,     /* R181 - DCDC1 Timeouts */ \
    0x1006,     /* R182 - DCDC1 Low Power */ \
    0x0018,     /* R183 - DCDC2 Control */ \
    0x0000,     /* R184 - DCDC2 Timeouts */ \
    0x0000,     /* R185 */ \
    0x0000,     /* R186 - DCDC3 Control */ \
    0x0000,     /* R187 - DCDC3 Timeouts */ \
    0x0006,     /* R188 - DCDC3 Low Power */ \
    0x0000,     /* R189 - DCDC4 Control */ \
    0x0000,     /* R190 - DCDC4 Timeouts */ \
    0x0006,     /* R191 - DCDC4 Low Power */ \
    0x0008,     /* R192 - DCDC5 Control */ \
    0x0000,     /* R193 - DCDC5 Timeouts */ \
    0x0000,     /* R194 */ \
    0x0000,     /* R195 - DCDC6 Control */ \
    0x0000,     /* R196 - DCDC6 Timeouts */ \
    0x0006,     /* R197 - DCDC6 Low Power */ \
    0x0000,     /* R198 */ \
    0x0003,     /* R199 - Limit Switch Control */ \
    0x001C,     /* R200 - LDO1 Control */ \
    0x0000,     /* R201 - LDO1 Timeouts */ \
    0x001C,     /* R202 - LDO1 Low Power */ \
    0x001B,     /* R203 - LDO2 Control */ \
    0x0000,     /* R204 - LDO2 Timeouts */ \
    0x001C,     /* R205 - LDO2 Low Power */ \
    0x001B,     /* R206 - LDO3 Control */ \
    0x0000,     /* R207 - LDO3 Timeouts */ \
    0x001C,     /* R208 - LDO3 Low Power */ \
    0x001B,     /* R209 - LDO4 Control */ \
    0x0000,     /* R210 - LDO4 Timeouts */ \
    0x001C,     /* R211 - LDO4 Low Power */ \
    0x0000,     /* R212 */ \
    0x0000,     /* R213 */ \
    0x0000,     /* R214 */ \
    0x0000,     /* R215 - VCC_FAULT Masks */ \
    0x001F,     /* R216 - Main Bandgap Control */ \
    0x0000,     /* R217 - OSC Control */ \
    0x9000,     /* R218 - RTC Tick Control */ \
    0x0000,     /* R219 */ \
    0x4000,     /* R220 - RAM BIST 1 */ \
    0x0000,     /* R221 */ \
    0x0000,     /* R222 */ \
    0x0000,     /* R223 */ \
    0x0000,     /* R224 */ \
    0x0000,     /* R225 - DCDC/LDO status */ \
    0x0000,     /* R226 */ \
    0x0000,     /* R227 */ \
    0x0000,     /* R228 */ \
    0x0000,     /* R229 */ \
    0xE000,     /* R230 - GPIO Pin Status */ \
    0x0000,     /* R231 */ \
    0x0000,     /* R232 */ \
    0x0000,     /* R233 */ \
    0x0000,     /* R234 */ \
    0x0000,     /* R235 */ \
    0x0000,     /* R236 */ \
    0x0000,     /* R237 */ \
    0x0000,     /* R238 */ \
    0x0000,     /* R239 */ \
    0x0000,     /* R240 */ \
    0x0000,     /* R241 */ \
    0x0000,     /* R242 */ \
    0x0000,     /* R243 */ \
    0x0000,     /* R244 */ \
    0x0000,     /* R245 */ \
    0x0000,     /* R246 */ \
    0x0000,     /* R247 */ \
    0x0000,     /* R248 */ \
    0x0000,     /* R249 */ \
    0x0000,     /* R250 */ \
    0x0000,     /* R251 */ \
    0x0000,     /* R252 */ \
    0x0000,     /* R253 */ \
    0x0000,     /* R254 */ \
    0x0000,     /* R255 */ \
}

/* Bank 1 */
#define WM8350_REGISTER_DEFAULTS_1 \
{ \
    0x17FF,     /* R0   - Reset/ID */ \
    0x1000,     /* R1   - ID */ \
    0x0000,     /* R2 */ \
    0x1002,     /* R3   - System Control 1 */ \
    0x0014,     /* R4   - System Control 2 */ \
    0x0000,     /* R5   - System Hibernate */ \
    0x8A00,     /* R6   - Interface Control */ \
    0x0000,     /* R7 */ \
    0x8000,     /* R8   - Power mgmt (1) */ \
    0x0000,     /* R9   - Power mgmt (2) */ \
    0x0000,     /* R10  - Power mgmt (3) */ \
    0x2000,     /* R11  - Power mgmt (4) */ \
    0x0E00,     /* R12  - Power mgmt (5) */ \
    0x0000,     /* R13  - Power mgmt (6) */ \
    0x0000,     /* R14  - Power mgmt (7) */ \
    0x0000,     /* R15 */ \
    0x0000,     /* R16  - RTC Seconds/Minutes */ \
    0x0100,     /* R17  - RTC Hours/Day */ \
    0x0101,     /* R18  - RTC Date/Month */ \
    0x1400,     /* R19  - RTC Year */ \
    0x0000,     /* R20  - Alarm Seconds/Minutes */ \
    0x0000,     /* R21  - Alarm Hours/Day */ \
    0x0000,     /* R22  - Alarm Date/Month */ \
    0x0320,     /* R23  - RTC Time Control */ \
    0x0000,     /* R24  - System Interrupts */ \
    0x0000,     /* R25  - Interrupt Status 1 */ \
    0x0000,     /* R26  - Interrupt Status 2 */ \
    0x0000,     /* R27  - Power Up Interrupt Status */ \
    0x0000,     /* R28  - Under Voltage Interrupt status */ \
    0x0000,     /* R29  - Over Current Interrupt status */ \
    0x0000,     /* R30  - GPIO Interrupt Status */ \
    0x0000,     /* R31  - Comparator Interrupt Status */ \
    0x3FFF,     /* R32  - System Interrupts Mask */ \
    0x0000,     /* R33  - Interrupt Status 1 Mask */ \
    0x0000,     /* R34  - Interrupt Status 2 Mask */ \
    0x0000,     /* R35  - Power Up Interrupt Status Mask */ \
    0x0000,     /* R36  - Under Voltage Interrupt status Mask */ \
    0x0000,     /* R37  - Over Current Interrupt status Mask */ \
    0x0000,     /* R38  - GPIO Interrupt Status Mask */ \
    0x0000,     /* R39  - Comparator Interrupt Status Mask */ \
    0x0040,     /* R40  - Clock Control 1 */ \
    0x0000,     /* R41  - Clock Control 2 */ \
    0x3B00,     /* R42  - FLL Control 1 */ \
    0x7086,     /* R43  - FLL Control 2 */ \
    0xC226,     /* R44  - FLL Control 3 */ \
    0x0000,     /* R45  - FLL Control 4 */ \
    0x0000,     /* R46 */ \
    0x0000,     /* R47 */ \
    0x0000,     /* R48  - DAC Control */ \
    0x0000,     /* R49 */ \
    0x00C0,     /* R50  - DAC Digital Volume L */ \
    0x00C0,     /* R51  - DAC Digital Volume R */ \
    0x0000,     /* R52 */ \
    0x0040,     /* R53  - DAC LR Rate */ \
    0x0000,     /* R54  - DAC Clock Control */ \
    0x0000,     /* R55 */ \
    0x0000,     /* R56 */ \
    0x0000,     /* R57 */ \
    0x4000,     /* R58  - DAC Mute */ \
    0x0000,     /* R59  - DAC Mute Volume */ \
    0x0000,     /* R60  - DAC Side */ \
    0x0000,     /* R61 */ \
    0x0000,     /* R62 */ \
    0x0000,     /* R63 */ \
    0x8000,     /* R64  - ADC Control */ \
    0x0000,     /* R65 */ \
    0x00C0,     /* R66  - ADC Digital Volume L */ \
    0x00C0,     /* R67  - ADC Digital Volume R */ \
    0x0000,     /* R68  - ADC Divider */ \
    0x0000,     /* R69 */ \
    0x0040,     /* R70  - ADC LR Rate */ \
    0x0000,     /* R71 */ \
    0x0303,     /* R72  - Input Control */ \
    0x0000,     /* R73  - IN3 Input Control */ \
    0x0000,     /* R74  - Mic Bias Control */ \
    0x0000,     /* R75 */ \
    0x0000,     /* R76  - Output Control */ \
    0x0000,     /* R77  - Jack Detect */ \
    0x0000,     /* R78  - Anti Pop Control */ \
    0x0000,     /* R79 */ \
    0x0040,     /* R80  - Left Input Volume */ \
    0x0040,     /* R81  - Right Input Volume */ \
    0x0000,     /* R82 */ \
    0x0000,     /* R83 */ \
    0x0000,     /* R84 */ \
    0x0000,     /* R85 */ \
    0x0000,     /* R86 */ \
    0x0000,     /* R87 */ \
    0x0800,     /* R88  - Left Mixer Control */ \
    0x1000,     /* R89  - Right Mixer Control */ \
    0x0000,     /* R90 */ \
    0x0000,     /* R91 */ \
    0x0000,     /* R92  - OUT3 Mixer Control */ \
    0x0000,     /* R93  - OUT4 Mixer Control */ \
    0x0000,     /* R94 */ \
    0x0000,     /* R95 */ \
    0x0000,     /* R96  - Output Left Mixer Volume */ \
    0x0000,     /* R97  - Output Right Mixer Volume */ \
    0x0000,     /* R98  - Input Mixer Volume L */ \
    0x0000,     /* R99  - Input Mixer Volume R */ \
    0x0000,     /* R100 - Input Mixer Volume */ \
    0x0000,     /* R101 */ \
    0x0000,     /* R102 */ \
    0x0000,     /* R103 */ \
    0x00E4,     /* R104 - LOUT1 Volume */ \
    0x00E4,     /* R105 - ROUT1 Volume */ \
    0x00E4,     /* R106 - LOUT2 Volume */ \
    0x02E4,     /* R107 - ROUT2 Volume */ \
    0x0000,     /* R108 */ \
    0x0000,     /* R109 */ \
    0x0000,     /* R110 */ \
    0x0000,     /* R111 - BEEP Volume */ \
    0x0A00,     /* R112 - AI Formating */ \
    0x0000,     /* R113 - ADC DAC COMP */ \
    0x0020,     /* R114 - AI ADC Control */ \
    0x0020,     /* R115 - AI DAC Control */ \
    0x0000,     /* R116 - AIF Test */ \
    0x0000,     /* R117 */ \
    0x0000,     /* R118 */ \
    0x0000,     /* R119 */ \
    0x0000,     /* R120 */ \
    0x0000,     /* R121 */ \
    0x0000,     /* R122 */ \
    0x0000,     /* R123 */ \
    0x0000,     /* R124 */ \
    0x0000,     /* R125 */ \
    0x0000,     /* R126 */ \
    0x0000,     /* R127 */ \
    0x1FFF,     /* R128 - GPIO Debounce */ \
    0x0000,     /* R129 - GPIO Pin pull up Control */ \
    0x03FC,     /* R130 - GPIO Pull down Control */ \
    0x0000,     /* R131 - GPIO Interrupt Mode */ \
    0x0000,     /* R132 */ \
    0x0000,     /* R133 - GPIO Control */ \
    0x00FB,     /* R134 - GPIO Configuration (i/o) */ \
    0x04FE,     /* R135 - GPIO Pin Polarity / Type */ \
    0x0000,     /* R136 */ \
    0x0000,     /* R137 */ \
    0x0000,     /* R138 */ \
    0x0000,     /* R139 */ \
    0x0312,     /* R140 - GPIO Function Select 1 */ \
    0x1003,     /* R141 - GPIO Function Select 2 */ \
    0x1331,     /* R142 - GPIO Function Select 3 */ \
    0x0003,     /* R143 - GPIO Function Select 4 */ \
    0x0000,     /* R144 - Digitiser Control (1) */ \
    0x0002,     /* R145 - Digitiser Control (2) */ \
    0x0000,     /* R146 */ \
    0x0000,     /* R147 */ \
    0x0000,     /* R148 */ \
    0x0000,     /* R149 */ \
    0x0000,     /* R150 */ \
    0x0000,     /* R151 */ \
    0x7000,     /* R152 - AUX1 Readback */ \
    0x7000,     /* R153 - AUX2 Readback */ \
    0x7000,     /* R154 - AUX3 Readback */ \
    0x7000,     /* R155 - AUX4 Readback */ \
    0x0000,     /* R156 - USB Voltage Readback */ \
    0x0000,     /* R157 - LINE Voltage Readback */ \
    0x0000,     /* R158 - BATT Voltage Readback */ \
    0x0000,     /* R159 - Chip Temp Readback */ \
    0x0000,     /* R160 */ \
    0x0000,     /* R161 */ \
    0x0000,     /* R162 */ \
    0x0000,     /* R163 - Generic Comparator Control */ \
    0x0000,     /* R164 - Generic comparator 1 */ \
    0x0000,     /* R165 - Generic comparator 2 */ \
    0x0000,     /* R166 - Generic comparator 3 */ \
    0x0000,     /* R167 - Generic comparator 4 */ \
    0xA00F,     /* R168 - Battery Charger Control 1 */ \
    0x0B06,     /* R169 - Battery Charger Control 2 */ \
    0x0000,     /* R170 - Battery Charger Control 3 */ \
    0x0000,     /* R171 */ \
    0x0000,     /* R172 - Current Sink Driver A */ \
    0x0000,     /* R173 - CSA Flash control */ \
    0x0000,     /* R174 - Current Sink Driver B */ \
    0x0000,     /* R175 - CSB Flash control */ \
    0x0000,     /* R176 - DCDC/LDO requested */ \
    0x002D,     /* R177 - DCDC Active options */ \
    0x0000,     /* R178 - DCDC Sleep options */ \
    0x0025,     /* R179 - Power-check comparator */ \
    0x0062,     /* R180 - DCDC1 Control */ \
    0x0400,     /* R181 - DCDC1 Timeouts */ \
    0x1006,     /* R182 - DCDC1 Low Power */ \
    0x0018,     /* R183 - DCDC2 Control */ \
    0x0000,     /* R184 - DCDC2 Timeouts */ \
    0x0000,     /* R185 */ \
    0x0026,     /* R186 - DCDC3 Control */ \
    0x0400,     /* R187 - DCDC3 Timeouts */ \
    0x0006,     /* R188 - DCDC3 Low Power */ \
    0x0062,     /* R189 - DCDC4 Control */ \
    0x0400,     /* R190 - DCDC4 Timeouts */ \
    0x0006,     /* R191 - DCDC4 Low Power */ \
    0x0008,     /* R192 - DCDC5 Control */ \
    0x0000,     /* R193 - DCDC5 Timeouts */ \
    0x0000,     /* R194 */ \
    0x0026,     /* R195 - DCDC6 Control */ \
    0x0800,     /* R196 - DCDC6 Timeouts */ \
    0x0006,     /* R197 - DCDC6 Low Power */ \
    0x0000,     /* R198 */ \
    0x0003,     /* R199 - Limit Switch Control */ \
    0x0006,     /* R200 - LDO1 Control */ \
    0x0400,     /* R201 - LDO1 Timeouts */ \
    0x001C,     /* R202 - LDO1 Low Power */ \
    0x0006,     /* R203 - LDO2 Control */ \
    0x0400,     /* R204 - LDO2 Timeouts */ \
    0x001C,     /* R205 - LDO2 Low Power */ \
    0x001B,     /* R206 - LDO3 Control */ \
    0x0000,     /* R207 - LDO3 Timeouts */ \
    0x001C,     /* R208 - LDO3 Low Power */ \
    0x001B,     /* R209 - LDO4 Control */ \
    0x0000,     /* R210 - LDO4 Timeouts */ \
    0x001C,     /* R211 - LDO4 Low Power */ \
    0x0000,     /* R212 */ \
    0x0000,     /* R213 */ \
    0x0000,     /* R214 */ \
    0x0000,     /* R215 - VCC_FAULT Masks */ \
    0x001F,     /* R216 - Main Bandgap Control */ \
    0x0000,     /* R217 - OSC Control */ \
    0x9000,     /* R218 - RTC Tick Control */ \
    0x0000,     /* R219 */ \
    0x4000,     /* R220 - RAM BIST 1 */ \
    0x0000,     /* R221 */ \
    0x0000,     /* R222 */ \
    0x0000,     /* R223 */ \
    0x0000,     /* R224 */ \
    0x0000,     /* R225 - DCDC/LDO status */ \
    0x0000,     /* R226 */ \
    0x0000,     /* R227 */ \
    0x0000,     /* R228 */ \
    0x0000,     /* R229 */ \
    0xE000,     /* R230 - GPIO Pin Status */ \
    0x0000,     /* R231 */ \
    0x0000,     /* R232 */ \
    0x0000,     /* R233 */ \
    0x0000,     /* R234 */ \
    0x0000,     /* R235 */ \
    0x0000,     /* R236 */ \
    0x0000,     /* R237 */ \
    0x0000,     /* R238 */ \
    0x0000,     /* R239 */ \
    0x0000,     /* R240 */ \
    0x0000,     /* R241 */ \
    0x0000,     /* R242 */ \
    0x0000,     /* R243 */ \
    0x0000,     /* R244 */ \
    0x0000,     /* R245 */ \
    0x0000,     /* R246 */ \
    0x0000,     /* R247 */ \
    0x0000,     /* R248 */ \
    0x0000,     /* R249 */ \
    0x0000,     /* R250 */ \
    0x0000,     /* R251 */ \
    0x0000,     /* R252 */ \
    0x0000,     /* R253 */ \
    0x0000,     /* R254 */ \
    0x0000,     /* R255 */ \
}

/* Bank 2 */
#define WM8350_REGISTER_DEFAULTS_2 \
{ \
    0x17FF,     /* R0   - Reset/ID */ \
    0x1000,     /* R1   - ID */ \
    0x0000,     /* R2 */ \
    0x1002,     /* R3   - System Control 1 */ \
    0x0014,     /* R4   - System Control 2 */ \
    0x0000,     /* R5   - System Hibernate */ \
    0x8A00,     /* R6   - Interface Control */ \
    0x0000,     /* R7 */ \
    0x8000,     /* R8   - Power mgmt (1) */ \
    0x0000,     /* R9   - Power mgmt (2) */ \
    0x0000,     /* R10  - Power mgmt (3) */ \
    0x2000,     /* R11  - Power mgmt (4) */ \
    0x0E00,     /* R12  - Power mgmt (5) */ \
    0x0000,     /* R13  - Power mgmt (6) */ \
    0x0000,     /* R14  - Power mgmt (7) */ \
    0x0000,     /* R15 */ \
    0x0000,     /* R16  - RTC Seconds/Minutes */ \
    0x0100,     /* R17  - RTC Hours/Day */ \
    0x0101,     /* R18  - RTC Date/Month */ \
    0x1400,     /* R19  - RTC Year */ \
    0x0000,     /* R20  - Alarm Seconds/Minutes */ \
    0x0000,     /* R21  - Alarm Hours/Day */ \
    0x0000,     /* R22  - Alarm Date/Month */ \
    0x0320,     /* R23  - RTC Time Control */ \
    0x0000,     /* R24  - System Interrupts */ \
    0x0000,     /* R25  - Interrupt Status 1 */ \
    0x0000,     /* R26  - Interrupt Status 2 */ \
    0x0000,     /* R27  - Power Up Interrupt Status */ \
    0x0000,     /* R28  - Under Voltage Interrupt status */ \
    0x0000,     /* R29  - Over Current Interrupt status */ \
    0x0000,     /* R30  - GPIO Interrupt Status */ \
    0x0000,     /* R31  - Comparator Interrupt Status */ \
    0x3FFF,     /* R32  - System Interrupts Mask */ \
    0x0000,     /* R33  - Interrupt Status 1 Mask */ \
    0x0000,     /* R34  - Interrupt Status 2 Mask */ \
    0x0000,     /* R35  - Power Up Interrupt Status Mask */ \
    0x0000,     /* R36  - Under Voltage Interrupt status Mask */ \
    0x0000,     /* R37  - Over Current Interrupt status Mask */ \
    0x0000,     /* R38  - GPIO Interrupt Status Mask */ \
    0x0000,     /* R39  - Comparator Interrupt Status Mask */ \
    0x0040,     /* R40  - Clock Control 1 */ \
    0x0000,     /* R41  - Clock Control 2 */ \
    0x3B00,     /* R42  - FLL Control 1 */ \
    0x7086,     /* R43  - FLL Control 2 */ \
    0xC226,     /* R44  - FLL Control 3 */ \
    0x0000,     /* R45  - FLL Control 4 */ \
    0x0000,     /* R46 */ \
    0x0000,     /* R47 */ \
    0x0000,     /* R48  - DAC Control */ \
    0x0000,     /* R49 */ \
    0x00C0,     /* R50  - DAC Digital Volume L */ \
    0x00C0,     /* R51  - DAC Digital Volume R */ \
    0x0000,     /* R52 */ \
    0x0040,     /* R53  - DAC LR Rate */ \
    0x0000,     /* R54  - DAC Clock Control */ \
    0x0000,     /* R55 */ \
    0x0000,     /* R56 */ \
    0x0000,     /* R57 */ \
    0x4000,     /* R58  - DAC Mute */ \
    0x0000,     /* R59  - DAC Mute Volume */ \
    0x0000,     /* R60  - DAC Side */ \
    0x0000,     /* R61 */ \
    0x0000,     /* R62 */ \
    0x0000,     /* R63 */ \
    0x8000,     /* R64  - ADC Control */ \
    0x0000,     /* R65 */ \
    0x00C0,     /* R66  - ADC Digital Volume L */ \
    0x00C0,     /* R67  - ADC Digital Volume R */ \
    0x0000,     /* R68  - ADC Divider */ \
    0x0000,     /* R69 */ \
    0x0040,     /* R70  - ADC LR Rate */ \
    0x0000,     /* R71 */ \
    0x0303,     /* R72  - Input Control */ \
    0x0000,     /* R73  - IN3 Input Control */ \
    0x0000,     /* R74  - Mic Bias Control */ \
    0x0000,     /* R75 */ \
    0x0000,     /* R76  - Output Control */ \
    0x0000,     /* R77  - Jack Detect */ \
    0x0000,     /* R78  - Anti Pop Control */ \
    0x0000,     /* R79 */ \
    0x0040,     /* R80  - Left Input Volume */ \
    0x0040,     /* R81  - Right Input Volume */ \
    0x0000,     /* R82 */ \
    0x0000,     /* R83 */ \
    0x0000,     /* R84 */ \
    0x0000,     /* R85 */ \
    0x0000,     /* R86 */ \
    0x0000,     /* R87 */ \
    0x0800,     /* R88  - Left Mixer Control */ \
    0x1000,     /* R89  - Right Mixer Control */ \
    0x0000,     /* R90 */ \
    0x0000,     /* R91 */ \
    0x0000,     /* R92  - OUT3 Mixer Control */ \
    0x0000,     /* R93  - OUT4 Mixer Control */ \
    0x0000,     /* R94 */ \
    0x0000,     /* R95 */ \
    0x0000,     /* R96  - Output Left Mixer Volume */ \
    0x0000,     /* R97  - Output Right Mixer Volume */ \
    0x0000,     /* R98  - Input Mixer Volume L */ \
    0x0000,     /* R99  - Input Mixer Volume R */ \
    0x0000,     /* R100 - Input Mixer Volume */ \
    0x0000,     /* R101 */ \
    0x0000,     /* R102 */ \
    0x0000,     /* R103 */ \
    0x00E4,     /* R104 - LOUT1 Volume */ \
    0x00E4,     /* R105 - ROUT1 Volume */ \
    0x00E4,     /* R106 - LOUT2 Volume */ \
    0x02E4,     /* R107 - ROUT2 Volume */ \
    0x0000,     /* R108 */ \
    0x0000,     /* R109 */ \
    0x0000,     /* R110 */ \
    0x0000,     /* R111 - BEEP Volume */ \
    0x0A00,     /* R112 - AI Formating */ \
    0x0000,     /* R113 - ADC DAC COMP */ \
    0x0020,     /* R114 - AI ADC Control */ \
    0x0020,     /* R115 - AI DAC Control */ \
    0x0000,     /* R116 - AIF Test */ \
    0x0000,     /* R117 */ \
    0x0000,     /* R118 */ \
    0x0000,     /* R119 */ \
    0x0000,     /* R120 */ \
    0x0000,     /* R121 */ \
    0x0000,     /* R122 */ \
    0x0000,     /* R123 */ \
    0x0000,     /* R124 */ \
    0x0000,     /* R125 */ \
    0x0000,     /* R126 */ \
    0x0000,     /* R127 */ \
    0x1FFF,     /* R128 - GPIO Debounce */ \
    0x0000,     /* R129 - GPIO Pin pull up Control */ \
    0x03FC,     /* R130 - GPIO Pull down Control */ \
    0x0000,     /* R131 - GPIO Interrupt Mode */ \
    0x0000,     /* R132 */ \
    0x0000,     /* R133 - GPIO Control */ \
    0x08FB,     /* R134 - GPIO Configuration (i/o) */ \
    0x0CFE,     /* R135 - GPIO Pin Polarity / Type */ \
    0x0000,     /* R136 */ \
    0x0000,     /* R137 */ \
    0x0000,     /* R138 */ \
    0x0000,     /* R139 */ \
    0x0312,     /* R140 - GPIO Function Select 1 */ \
    0x0003,     /* R141 - GPIO Function Select 2 */ \
    0x2331,     /* R142 - GPIO Function Select 3 */ \
    0x0003,     /* R143 - GPIO Function Select 4 */ \
    0x0000,     /* R144 - Digitiser Control (1) */ \
    0x0002,     /* R145 - Digitiser Control (2) */ \
    0x0000,     /* R146 */ \
    0x0000,     /* R147 */ \
    0x0000,     /* R148 */ \
    0x0000,     /* R149 */ \
    0x0000,     /* R150 */ \
    0x0000,     /* R151 */ \
    0x7000,     /* R152 - AUX1 Readback */ \
    0x7000,     /* R153 - AUX2 Readback */ \
    0x7000,     /* R154 - AUX3 Readback */ \
    0x7000,     /* R155 - AUX4 Readback */ \
    0x0000,     /* R156 - USB Voltage Readback */ \
    0x0000,     /* R157 - LINE Voltage Readback */ \
    0x0000,     /* R158 - BATT Voltage Readback */ \
    0x0000,     /* R159 - Chip Temp Readback */ \
    0x0000,     /* R160 */ \
    0x0000,     /* R161 */ \
    0x0000,     /* R162 */ \
    0x0000,     /* R163 - Generic Comparator Control */ \
    0x0000,     /* R164 - Generic comparator 1 */ \
    0x0000,     /* R165 - Generic comparator 2 */ \
    0x0000,     /* R166 - Generic comparator 3 */ \
    0x0000,     /* R167 - Generic comparator 4 */ \
    0xA00F,     /* R168 - Battery Charger Control 1 */ \
    0x0B06,     /* R169 - Battery Charger Control 2 */ \
    0x0000,     /* R170 - Battery Charger Control 3 */ \
    0x0000,     /* R171 */ \
    0x0000,     /* R172 - Current Sink Driver A */ \
    0x0000,     /* R173 - CSA Flash control */ \
    0x0000,     /* R174 - Current Sink Driver B */ \
    0x0000,     /* R175 - CSB Flash control */ \
    0x0000,     /* R176 - DCDC/LDO requested */ \
    0x002D,     /* R177 - DCDC Active options */ \
    0x0000,     /* R178 - DCDC Sleep options */ \
    0x0025,     /* R179 - Power-check comparator */ \
    0x000E,     /* R180 - DCDC1 Control */ \
    0x0400,     /* R181 - DCDC1 Timeouts */ \
    0x1006,     /* R182 - DCDC1 Low Power */ \
    0x0018,     /* R183 - DCDC2 Control */ \
    0x0000,     /* R184 - DCDC2 Timeouts */ \
    0x0000,     /* R185 */ \
    0x002E,     /* R186 - DCDC3 Control */ \
    0x0800,     /* R187 - DCDC3 Timeouts */ \
    0x0006,     /* R188 - DCDC3 Low Power */ \
    0x000E,     /* R189 - DCDC4 Control */ \
    0x0800,     /* R190 - DCDC4 Timeouts */ \
    0x0006,     /* R191 - DCDC4 Low Power */ \
    0x0008,     /* R192 - DCDC5 Control */ \
    0x0000,     /* R193 - DCDC5 Timeouts */ \
    0x0000,     /* R194 */ \
    0x0026,     /* R195 - DCDC6 Control */ \
    0x0C00,     /* R196 - DCDC6 Timeouts */ \
    0x0006,     /* R197 - DCDC6 Low Power */ \
    0x0000,     /* R198 */ \
    0x0003,     /* R199 - Limit Switch Control */ \
    0x001A,     /* R200 - LDO1 Control */ \
    0x0800,     /* R201 - LDO1 Timeouts */ \
    0x001C,     /* R202 - LDO1 Low Power */ \
    0x0010,     /* R203 - LDO2 Control */ \
    0x0800,     /* R204 - LDO2 Timeouts */ \
    0x001C,     /* R205 - LDO2 Low Power */ \
    0x000A,     /* R206 - LDO3 Control */ \
    0x0C00,     /* R207 - LDO3 Timeouts */ \
    0x001C,     /* R208 - LDO3 Low Power */ \
    0x001A,     /* R209 - LDO4 Control */ \
    0x0800,     /* R210 - LDO4 Timeouts */ \
    0x001C,     /* R211 - LDO4 Low Power */ \
    0x0000,     /* R212 */ \
    0x0000,     /* R213 */ \
    0x0000,     /* R214 */ \
    0x0000,     /* R215 - VCC_FAULT Masks */ \
    0x001F,     /* R216 - Main Bandgap Control */ \
    0x0000,     /* R217 - OSC Control */ \
    0x9000,     /* R218 - RTC Tick Control */ \
    0x0000,     /* R219 */ \
    0x4000,     /* R220 - RAM BIST 1 */ \
    0x0000,     /* R221 */ \
    0x0000,     /* R222 */ \
    0x0000,     /* R223 */ \
    0x0000,     /* R224 */ \
    0x0000,     /* R225 - DCDC/LDO status */ \
    0x0000,     /* R226 */ \
    0x0000,     /* R227 */ \
    0x0000,     /* R228 */ \
    0x0000,     /* R229 */ \
    0xE000,     /* R230 - GPIO Pin Status */ \
    0x0000,     /* R231 */ \
    0x0000,     /* R232 */ \
    0x0000,     /* R233 */ \
    0x0000,     /* R234 */ \
    0x0000,     /* R235 */ \
    0x0000,     /* R236 */ \
    0x0000,     /* R237 */ \
    0x0000,     /* R238 */ \
    0x0000,     /* R239 */ \
    0x0000,     /* R240 */ \
    0x0000,     /* R241 */ \
    0x0000,     /* R242 */ \
    0x0000,     /* R243 */ \
    0x0000,     /* R244 */ \
    0x0000,     /* R245 */ \
    0x0000,     /* R246 */ \
    0x0000,     /* R247 */ \
    0x0000,     /* R248 */ \
    0x0000,     /* R249 */ \
    0x0000,     /* R250 */ \
    0x0000,     /* R251 */ \
    0x0000,     /* R252 */ \
    0x0000,     /* R253 */ \
    0x0000,     /* R254 */ \
    0x0000,     /* R255 */ \
}

/* Bank 3 */
#define WM8350_REGISTER_DEFAULTS_3 \
{ \
    0x17FF,     /* R0   - Reset/ID */ \
    0x1000,     /* R1   - ID */ \
    0x0000,     /* R2 */ \
    0x1000,     /* R3   - System Control 1 */ \
    0x0004,     /* R4   - System Control 2 */ \
    0x0000,     /* R5   - System Hibernate */ \
    0x8A00,     /* R6   - Interface Control */ \
    0x0000,     /* R7 */ \
    0x8000,     /* R8   - Power mgmt (1) */ \
    0x0000,     /* R9   - Power mgmt (2) */ \
    0x0000,     /* R10  - Power mgmt (3) */ \
    0x2000,     /* R11  - Power mgmt (4) */ \
    0x0E00,     /* R12  - Power mgmt (5) */ \
    0x0000,     /* R13  - Power mgmt (6) */ \
    0x0000,     /* R14  - Power mgmt (7) */ \
    0x0000,     /* R15 */ \
    0x0000,     /* R16  - RTC Seconds/Minutes */ \
    0x0100,     /* R17  - RTC Hours/Day */ \
    0x0101,     /* R18  - RTC Date/Month */ \
    0x1400,     /* R19  - RTC Year */ \
    0x0000,     /* R20  - Alarm Seconds/Minutes */ \
    0x0000,     /* R21  - Alarm Hours/Day */ \
    0x0000,     /* R22  - Alarm Date/Month */ \
    0x0320,     /* R23  - RTC Time Control */ \
    0x0000,     /* R24  - System Interrupts */ \
    0x0000,     /* R25  - Interrupt Status 1 */ \
    0x0000,     /* R26  - Interrupt Status 2 */ \
    0x0000,     /* R27  - Power Up Interrupt Status */ \
    0x0000,     /* R28  - Under Voltage Interrupt status */ \
    0x0000,     /* R29  - Over Current Interrupt status */ \
    0x0000,     /* R30  - GPIO Interrupt Status */ \
    0x0000,     /* R31  - Comparator Interrupt Status */ \
    0x3FFF,     /* R32  - System Interrupts Mask */ \
    0x0000,     /* R33  - Interrupt Status 1 Mask */ \
    0x0000,     /* R34  - Interrupt Status 2 Mask */ \
    0x0000,     /* R35  - Power Up Interrupt Status Mask */ \
    0x0000,     /* R36  - Under Voltage Interrupt status Mask */ \
    0x0000,     /* R37  - Over Current Interrupt status Mask */ \
    0x0000,     /* R38  - GPIO Interrupt Status Mask */ \
    0x0000,     /* R39  - Comparator Interrupt Status Mask */ \
    0x0040,     /* R40  - Clock Control 1 */ \
    0x0000,     /* R41  - Clock Control 2 */ \
    0x3B00,     /* R42  - FLL Control 1 */ \
    0x7086,     /* R43  - FLL Control 2 */ \
    0xC226,     /* R44  - FLL Control 3 */ \
    0x0000,     /* R45  - FLL Control 4 */ \
    0x0000,     /* R46 */ \
    0x0000,     /* R47 */ \
    0x0000,     /* R48  - DAC Control */ \
    0x0000,     /* R49 */ \
    0x00C0,     /* R50  - DAC Digital Volume L */ \
    0x00C0,     /* R51  - DAC Digital Volume R */ \
    0x0000,     /* R52 */ \
    0x0040,     /* R53  - DAC LR Rate */ \
    0x0000,     /* R54  - DAC Clock Control */ \
    0x0000,     /* R55 */ \
    0x0000,     /* R56 */ \
    0x0000,     /* R57 */ \
    0x4000,     /* R58  - DAC Mute */ \
    0x0000,     /* R59  - DAC Mute Volume */ \
    0x0000,     /* R60  - DAC Side */ \
    0x0000,     /* R61 */ \
    0x0000,     /* R62 */ \
    0x0000,     /* R63 */ \
    0x8000,     /* R64  - ADC Control */ \
    0x0000,     /* R65 */ \
    0x00C0,     /* R66  - ADC Digital Volume L */ \
    0x00C0,     /* R67  - ADC Digital Volume R */ \
    0x0000,     /* R68  - ADC Divider */ \
    0x0000,     /* R69 */ \
    0x0040,     /* R70  - ADC LR Rate */ \
    0x0000,     /* R71 */ \
    0x0303,     /* R72  - Input Control */ \
    0x0000,     /* R73  - IN3 Input Control */ \
    0x0000,     /* R74  - Mic Bias Control */ \
    0x0000,     /* R75 */ \
    0x0000,     /* R76  - Output Control */ \
    0x0000,     /* R77  - Jack Detect */ \
    0x0000,     /* R78  - Anti Pop Control */ \
    0x0000,     /* R79 */ \
    0x0040,     /* R80  - Left Input Volume */ \
    0x0040,     /* R81  - Right Input Volume */ \
    0x0000,     /* R82 */ \
    0x0000,     /* R83 */ \
    0x0000,     /* R84 */ \
    0x0000,     /* R85 */ \
    0x0000,     /* R86 */ \
    0x0000,     /* R87 */ \
    0x0800,     /* R88  - Left Mixer Control */ \
    0x1000,     /* R89  - Right Mixer Control */ \
    0x0000,     /* R90 */ \
    0x0000,     /* R91 */ \
    0x0000,     /* R92  - OUT3 Mixer Control */ \
    0x0000,     /* R93  - OUT4 Mixer Control */ \
    0x0000,     /* R94 */ \
    0x0000,     /* R95 */ \
    0x0000,     /* R96  - Output Left Mixer Volume */ \
    0x0000,     /* R97  - Output Right Mixer Volume */ \
    0x0000,     /* R98  - Input Mixer Volume L */ \
    0x0000,     /* R99  - Input Mixer Volume R */ \
    0x0000,     /* R100 - Input Mixer Volume */ \
    0x0000,     /* R101 */ \
    0x0000,     /* R102 */ \
    0x0000,     /* R103 */ \
    0x00E4,     /* R104 - LOUT1 Volume */ \
    0x00E4,     /* R105 - ROUT1 Volume */ \
    0x00E4,     /* R106 - LOUT2 Volume */ \
    0x02E4,     /* R107 - ROUT2 Volume */ \
    0x0000,     /* R108 */ \
    0x0000,     /* R109 */ \
    0x0000,     /* R110 */ \
    0x0000,     /* R111 - BEEP Volume */ \
    0x0A00,     /* R112 - AI Formating */ \
    0x0000,     /* R113 - ADC DAC COMP */ \
    0x0020,     /* R114 - AI ADC Control */ \
    0x0020,     /* R115 - AI DAC Control */ \
    0x0000,     /* R116 - AIF Test */ \
    0x0000,     /* R117 */ \
    0x0000,     /* R118 */ \
    0x0000,     /* R119 */ \
    0x0000,     /* R120 */ \
    0x0000,     /* R121 */ \
    0x0000,     /* R122 */ \
    0x0000,     /* R123 */ \
    0x0000,     /* R124 */ \
    0x0000,     /* R125 */ \
    0x0000,     /* R126 */ \
    0x0000,     /* R127 */ \
    0x1FFF,     /* R128 - GPIO Debounce */ \
    0x0000,     /* R129 - GPIO Pin pull up Control */ \
    0x03FC,     /* R130 - GPIO Pull down Control */ \
    0x0000,     /* R131 - GPIO Interrupt Mode */ \
    0x0000,     /* R132 */ \
    0x0000,     /* R133 - GPIO Control */ \
    0x0A7B,     /* R134 - GPIO Configuration (i/o) */ \
    0x06FE,     /* R135 - GPIO Pin Polarity / Type */ \
    0x0000,     /* R136 */ \
    0x0000,     /* R137 */ \
    0x0000,     /* R138 */ \
    0x0000,     /* R139 */ \
    0x1312,     /* R140 - GPIO Function Select 1 */ \
    0x1030,     /* R141 - GPIO Function Select 2 */ \
    0x2231,     /* R142 - GPIO Function Select 3 */ \
    0x0003,     /* R143 - GPIO Function Select 4 */ \
    0x0000,     /* R144 - Digitiser Control (1) */ \
    0x0002,     /* R145 - Digitiser Control (2) */ \
    0x0000,     /* R146 */ \
    0x0000,     /* R147 */ \
    0x0000,     /* R148 */ \
    0x0000,     /* R149 */ \
    0x0000,     /* R150 */ \
    0x0000,     /* R151 */ \
    0x7000,     /* R152 - AUX1 Readback */ \
    0x7000,     /* R153 - AUX2 Readback */ \
    0x7000,     /* R154 - AUX3 Readback */ \
    0x7000,     /* R155 - AUX4 Readback */ \
    0x0000,     /* R156 - USB Voltage Readback */ \
    0x0000,     /* R157 - LINE Voltage Readback */ \
    0x0000,     /* R158 - BATT Voltage Readback */ \
    0x0000,     /* R159 - Chip Temp Readback */ \
    0x0000,     /* R160 */ \
    0x0000,     /* R161 */ \
    0x0000,     /* R162 */ \
    0x0000,     /* R163 - Generic Comparator Control */ \
    0x0000,     /* R164 - Generic comparator 1 */ \
    0x0000,     /* R165 - Generic comparator 2 */ \
    0x0000,     /* R166 - Generic comparator 3 */ \
    0x0000,     /* R167 - Generic comparator 4 */ \
    0xA00F,     /* R168 - Battery Charger Control 1 */ \
    0x0B06,     /* R169 - Battery Charger Control 2 */ \
    0x0000,     /* R170 - Battery Charger Control 3 */ \
    0x0000,     /* R171 */ \
    0x0000,     /* R172 - Current Sink Driver A */ \
    0x0000,     /* R173 - CSA Flash control */ \
    0x0000,     /* R174 - Current Sink Driver B */ \
    0x0000,     /* R175 - CSB Flash control */ \
    0x0000,     /* R176 - DCDC/LDO requested */ \
    0x002D,     /* R177 - DCDC Active options */ \
    0x0000,     /* R178 - DCDC Sleep options */ \
    0x0025,     /* R179 - Power-check comparator */ \
    0x000E,     /* R180 - DCDC1 Control */ \
    0x0400,     /* R181 - DCDC1 Timeouts */ \
    0x1006,     /* R182 - DCDC1 Low Power */ \
    0x0018,     /* R183 - DCDC2 Control */ \
    0x0000,     /* R184 - DCDC2 Timeouts */ \
    0x0000,     /* R185 */ \
    0x000E,     /* R186 - DCDC3 Control */ \
    0x0400,     /* R187 - DCDC3 Timeouts */ \
    0x0006,     /* R188 - DCDC3 Low Power */ \
    0x0026,     /* R189 - DCDC4 Control */ \
    0x0400,     /* R190 - DCDC4 Timeouts */ \
    0x0006,     /* R191 - DCDC4 Low Power */ \
    0x0008,     /* R192 - DCDC5 Control */ \
    0x0000,     /* R193 - DCDC5 Timeouts */ \
    0x0000,     /* R194 */ \
    0x0026,     /* R195 - DCDC6 Control */ \
    0x0400,     /* R196 - DCDC6 Timeouts */ \
    0x0006,     /* R197 - DCDC6 Low Power */ \
    0x0000,     /* R198 */ \
    0x0003,     /* R199 - Limit Switch Control */ \
    0x001C,     /* R200 - LDO1 Control */ \
    0x0000,     /* R201 - LDO1 Timeouts */ \
    0x001C,     /* R202 - LDO1 Low Power */ \
    0x001C,     /* R203 - LDO2 Control */ \
    0x0400,     /* R204 - LDO2 Timeouts */ \
    0x001C,     /* R205 - LDO2 Low Power */ \
    0x001C,     /* R206 - LDO3 Control */ \
    0x0400,     /* R207 - LDO3 Timeouts */ \
    0x001C,     /* R208 - LDO3 Low Power */ \
    0x001F,     /* R209 - LDO4 Control */ \
    0x0400,     /* R210 - LDO4 Timeouts */ \
    0x001C,     /* R211 - LDO4 Low Power */ \
    0x0000,     /* R212 */ \
    0x0000,     /* R213 */ \
    0x0000,     /* R214 */ \
    0x0000,     /* R215 - VCC_FAULT Masks */ \
    0x001F,     /* R216 - Main Bandgap Control */ \
    0x0000,     /* R217 - OSC Control */ \
    0x9000,     /* R218 - RTC Tick Control */ \
    0x0000,     /* R219 */ \
    0x4000,     /* R220 - RAM BIST 1 */ \
    0x0000,     /* R221 */ \
    0x0000,     /* R222 */ \
    0x0000,     /* R223 */ \
    0x0000,     /* R224 */ \
    0x0000,     /* R225 - DCDC/LDO status */ \
    0x0000,     /* R226 */ \
    0x0000,     /* R227 */ \
    0x0000,     /* R228 */ \
    0x0000,     /* R229 */ \
    0xE000,     /* R230 - GPIO Pin Status */ \
    0x0000,     /* R231 */ \
    0x0000,     /* R232 */ \
    0x0000,     /* R233 */ \
    0x0000,     /* R234 */ \
    0x0000,     /* R235 */ \
    0x0000,     /* R236 */ \
    0x0000,     /* R237 */ \
    0x0000,     /* R238 */ \
    0x0000,     /* R239 */ \
    0x0000,     /* R240 */ \
    0x0000,     /* R241 */ \
    0x0000,     /* R242 */ \
    0x0000,     /* R243 */ \
    0x0000,     /* R244 */ \
    0x0000,     /* R245 */ \
    0x0000,     /* R246 */ \
    0x0000,     /* R247 */ \
    0x0000,     /* R248 */ \
    0x0000,     /* R249 */ \
    0x0000,     /* R250 */ \
    0x0000,     /* R251 */ \
    0x0000,     /* R252 */ \
    0x0000,     /* R253 */ \
    0x0000,     /* R254 */ \
    0x0000,     /* R255 */ \
}

/*
 * Access masks.
 */

struct wm8350_reg_access {
    u16  readable;   /* Mask of readable bits */
    u16  writable;   /* Mask of writable bits */
    u16  vol;        /* Mask of volatile bits */
};

#define WM8350_ACCESS \
{  /*  read    write volatile */ \
    { 0xFFFF, 0xFFFF, 0xFFFF }, /* R0   - Reset/ID */ \
    { 0x7CFF, 0x0C00, 0x7FFF }, /* R1   - ID */ \
    { 0x0000, 0x0000, 0x0000 }, /* R2 */ \
    { 0xBE3B, 0xBE3B, 0x8000 }, /* R3   - System Control 1 */ \
    { 0xFCF7, 0xFCF7, 0xF800 }, /* R4   - System Control 2 */ \
    { 0x80FF, 0x80FF, 0x8000 }, /* R5   - System Hibernate */ \
    { 0xFB0E, 0xFB0E, 0x0000 }, /* R6   - Interface Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R7 */ \
    { 0xE537, 0xE537, 0xFFFF }, /* R8   - Power mgmt (1) */ \
    { 0x0FF3, 0x0FF3, 0xFFFF }, /* R9   - Power mgmt (2) */ \
    { 0x008F, 0x008F, 0xFFFF }, /* R10  - Power mgmt (3) */ \
    { 0x6D3C, 0x6D3C, 0xFFFF }, /* R11  - Power mgmt (4) */ \
    { 0x1F8F, 0x1F8F, 0xFFFF }, /* R12  - Power mgmt (5) */ \
    { 0x8F3F, 0x8F3F, 0xFFFF }, /* R13  - Power mgmt (6) */ \
    { 0x0003, 0x0003, 0xFFFF }, /* R14  - Power mgmt (7) */ \
    { 0x0000, 0x0000, 0x0000 }, /* R15 */ \
    { 0x7F7F, 0x7F7F, 0xFFFF }, /* R16  - RTC Seconds/Minutes */ \
    { 0x073F, 0x073F, 0xFFFF }, /* R17  - RTC Hours/Day */ \
    { 0x1F3F, 0x1F3F, 0xFFFF }, /* R18  - RTC Date/Month */ \
    { 0x3FFF, 0x00FF, 0xFFFF }, /* R19  - RTC Year */ \
    { 0x7F7F, 0x7F7F, 0x0000 }, /* R20  - Alarm Seconds/Minutes */ \
    { 0x0F3F, 0x0F3F, 0x0000 }, /* R21  - Alarm Hours/Day */ \
    { 0x1F3F, 0x1F3F, 0x0000 }, /* R22  - Alarm Date/Month */ \
    { 0xEF7F, 0xEA7F, 0xFFFF }, /* R23  - RTC Time Control */ \
    { 0x3BFF, 0x0000, 0xFFFF }, /* R24  - System Interrupts */ \
    { 0xFEE7, 0x0000, 0xFFFF }, /* R25  - Interrupt Status 1 */ \
    { 0x35FF, 0x0000, 0xFFFF }, /* R26  - Interrupt Status 2 */ \
    { 0x0F3F, 0x0000, 0xFFFF }, /* R27  - Power Up Interrupt Status */ \
    { 0x0F3F, 0x0000, 0xFFFF }, /* R28  - Under Voltage Interrupt status */ \
    { 0x8000, 0x0000, 0xFFFF }, /* R29  - Over Current Interrupt status */ \
    { 0x1FFF, 0x0000, 0xFFFF }, /* R30  - GPIO Interrupt Status */ \
    { 0xEF7F, 0x0000, 0xFFFF }, /* R31  - Comparator Interrupt Status */ \
    { 0x3FFF, 0x3FFF, 0x0000 }, /* R32  - System Interrupts Mask */ \
    { 0xFEE7, 0xFEE7, 0x0000 }, /* R33  - Interrupt Status 1 Mask */ \
    { 0xF5FF, 0xF5FF, 0x0000 }, /* R34  - Interrupt Status 2 Mask */ \
    { 0x0F3F, 0x0F3F, 0x0000 }, /* R35  - Power Up Interrupt Status Mask */ \
    { 0x0F3F, 0x0F3F, 0x0000 }, /* R36  - Under Voltage Interrupt status Mask */ \
    { 0x8000, 0x8000, 0x0000 }, /* R37  - Over Current Interrupt status Mask */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R38  - GPIO Interrupt Status Mask */ \
    { 0xEF7F, 0xEF7F, 0x0000 }, /* R39  - Comparator Interrupt Status Mask */ \
    { 0xC9F7, 0xC9F7, 0xFFFF }, /* R40  - Clock Control 1 */ \
    { 0x8001, 0x8001, 0x0000 }, /* R41  - Clock Control 2 */ \
    { 0xFFF7, 0xFFF7, 0xFFFF }, /* R42  - FLL Control 1 */ \
    { 0xFBFF, 0xFBFF, 0x0000 }, /* R43  - FLL Control 2 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R44  - FLL Control 3 */ \
    { 0x0033, 0x0033, 0x0000 }, /* R45  - FLL Control 4 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R46 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R47 */ \
    { 0x3033, 0x3033, 0x0000 }, /* R48  - DAC Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R49 */ \
    { 0x81FF, 0x81FF, 0xFFFF }, /* R50  - DAC Digital Volume L */ \
    { 0x81FF, 0x81FF, 0xFFFF }, /* R51  - DAC Digital Volume R */ \
    { 0x0000, 0x0000, 0x0000 }, /* R52 */ \
    { 0x0FFF, 0x0FFF, 0xFFFF }, /* R53  - DAC LR Rate */ \
    { 0x0017, 0x0017, 0x0000 }, /* R54  - DAC Clock Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R55 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R56 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R57 */ \
    { 0x4000, 0x4000, 0x0000 }, /* R58  - DAC Mute */ \
    { 0x7000, 0x7000, 0x0000 }, /* R59  - DAC Mute Volume */ \
    { 0x3C00, 0x3C00, 0x0000 }, /* R60  - DAC Side */ \
    { 0x0000, 0x0000, 0x0000 }, /* R61 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R62 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R63 */ \
    { 0x8303, 0x8303, 0xFFFF }, /* R64  - ADC Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R65 */ \
    { 0x81FF, 0x81FF, 0xFFFF }, /* R66  - ADC Digital Volume L */ \
    { 0x81FF, 0x81FF, 0xFFFF }, /* R67  - ADC Digital Volume R */ \
    { 0x0FFF, 0x0FFF, 0x0000 }, /* R68  - ADC Divider */ \
    { 0x0000, 0x0000, 0x0000 }, /* R69 */ \
    { 0x0FFF, 0x0FFF, 0xFFFF }, /* R70  - ADC LR Rate */ \
    { 0x0000, 0x0000, 0x0000 }, /* R71 */ \
    { 0x0707, 0x0707, 0xFFFF }, /* R72  - Input Control */ \
    { 0xC0C0, 0xC0C0, 0xFFFF }, /* R73  - IN3 Input Control */ \
    { 0xC09F, 0xC09F, 0xFFFF }, /* R74  - Mic Bias Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R75 */ \
    { 0x0F15, 0x0F15, 0xFFFF }, /* R76  - Output Control */ \
    { 0xC000, 0xC000, 0xFFFF }, /* R77  - Jack Detect */ \
    { 0x03FF, 0x03FF, 0x0000 }, /* R78  - Anti Pop Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R79 */ \
    { 0xE1FC, 0xE1FC, 0x8000 }, /* R80  - Left Input Volume */ \
    { 0xE1FC, 0xE1FC, 0x8000 }, /* R81  - Right Input Volume */ \
    { 0x0000, 0x0000, 0x0000 }, /* R82 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R83 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R84 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R85 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R86 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R87 */ \
    { 0x9807, 0x9807, 0xFFFF }, /* R88  - Left Mixer Control */ \
    { 0x980B, 0x980B, 0xFFFF }, /* R89  - Right Mixer Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R90 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R91 */ \
    { 0x8909, 0x8909, 0xFFFF }, /* R92  - OUT3 Mixer Control */ \
    { 0x9E07, 0x9E07, 0xFFFF }, /* R93  - OUT4 Mixer Control */ \
    { 0x0000, 0x0000, 0x0000 }, /* R94 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R95 */ \
    { 0x0EEE, 0x0EEE, 0x0000 }, /* R96  - Output Left Mixer Volume */ \
    { 0xE0EE, 0xE0EE, 0x0000 }, /* R97  - Output Right Mixer Volume */ \
    { 0x0E0F, 0x0E0F, 0x0000 }, /* R98  - Input Mixer Volume L */ \
    { 0xE0E1, 0xE0E1, 0x0000 }, /* R99  - Input Mixer Volume R */ \
    { 0x800E, 0x800E, 0x0000 }, /* R100 - Input Mixer Volume */ \
    { 0x0000, 0x0000, 0x0000 }, /* R101 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R102 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R103 */ \
    { 0xE1FC, 0xE1FC, 0xFFFF }, /* R104 - LOUT1 Volume */ \
    { 0xE1FC, 0xE1FC, 0xFFFF }, /* R105 - ROUT1 Volume */ \
    { 0xE1FC, 0xE1FC, 0xFFFF }, /* R106 - LOUT2 Volume */ \
    { 0xE7FC, 0xE7FC, 0xFFFF }, /* R107 - ROUT2 Volume */ \
    { 0x0000, 0x0000, 0x0000 }, /* R108 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R109 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R110 */ \
    { 0x80E0, 0x80E0, 0xFFFF }, /* R111 - BEEP Volume */ \
    { 0xBF00, 0xBF00, 0x0000 }, /* R112 - AI Formating */ \
    { 0x00F1, 0x00F1, 0x0000 }, /* R113 - ADC DAC COMP */ \
    { 0x00F8, 0x00F8, 0x0000 }, /* R114 - AI ADC Control */ \
    { 0x40FB, 0x40FB, 0x0000 }, /* R115 - AI DAC Control */ \
    { 0x7C30, 0x7C30, 0x0000 }, /* R116 - AIF Test */ \
    { 0x0000, 0x0000, 0x0000 }, /* R117 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R118 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R119 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R120 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R121 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R122 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R123 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R124 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R125 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R126 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R127 */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R128 - GPIO Debounce */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R129 - GPIO Pin pull up Control */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R130 - GPIO Pull down Control */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R131 - GPIO Interrupt Mode */ \
    { 0x0000, 0x0000, 0x0000 }, /* R132 */ \
    { 0x00C0, 0x00C0, 0x0000 }, /* R133 - GPIO Control */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R134 - GPIO Configuration (i/o) */ \
    { 0x1FFF, 0x1FFF, 0x0000 }, /* R135 - GPIO Pin Polarity / Type */ \
    { 0x0000, 0x0000, 0x0000 }, /* R136 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R137 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R138 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R139 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R140 - GPIO Function Select 1 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R141 - GPIO Function Select 2 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R142 - GPIO Function Select 3 */ \
    { 0x000F, 0x000F, 0x0000 }, /* R143 - GPIO Function Select 4 */ \
    { 0xF0FF, 0xF0FF, 0xA000 }, /* R144 - Digitiser Control (1) */ \
    { 0x3707, 0x3707, 0x0000 }, /* R145 - Digitiser Control (2) */ \
    { 0x0000, 0x0000, 0x0000 }, /* R146 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R147 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R148 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R149 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R150 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R151 */ \
    { 0x7FFF, 0x7000, 0xFFFF }, /* R152 - AUX1 Readback */ \
    { 0x7FFF, 0x7000, 0xFFFF }, /* R153 - AUX2 Readback */ \
    { 0x7FFF, 0x7000, 0xFFFF }, /* R154 - AUX3 Readback */ \
    { 0x7FFF, 0x7000, 0xFFFF }, /* R155 - AUX4 Readback */ \
    { 0x0FFF, 0x0000, 0xFFFF }, /* R156 - USB Voltage Readback */ \
    { 0x0FFF, 0x0000, 0xFFFF }, /* R157 - LINE Voltage Readback */ \
    { 0x0FFF, 0x0000, 0xFFFF }, /* R158 - BATT Voltage Readback */ \
    { 0x0FFF, 0x0000, 0xFFFF }, /* R159 - Chip Temp Readback */ \
    { 0x0000, 0x0000, 0x0000 }, /* R160 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R161 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R162 */ \
    { 0x000F, 0x000F, 0x0000 }, /* R163 - Generic Comparator Control */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R164 - Generic comparator 1 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R165 - Generic comparator 2 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R166 - Generic comparator 3 */ \
    { 0xFFFF, 0xFFFF, 0x0000 }, /* R167 - Generic comparator 4 */ \
    { 0xBFFF, 0xBFFF, 0x8000 }, /* R168 - Battery Charger Control 1 */ \
    { 0xFFFF, 0x4FFF, 0xB000 }, /* R169 - Battery Charger Control 2 */ \
    { 0x007F, 0x007F, 0x0000 }, /* R170 - Battery Charger Control 3 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R171 */ \
    { 0x903F, 0x903F, 0xFFFF }, /* R172 - Current Sink Driver A */ \
    { 0xE333, 0xE333, 0xFFFF }, /* R173 - CSA Flash control */ \
    { 0x903F, 0x903F, 0xFFFF }, /* R174 - Current Sink Driver B */ \
    { 0xE333, 0xE333, 0xFFFF }, /* R175 - CSB Flash control */ \
    { 0x8F3F, 0x8F3F, 0xFFFF }, /* R176 - DCDC/LDO requested */ \
    { 0x332D, 0x332D, 0x0000 }, /* R177 - DCDC Active options */ \
    { 0x002D, 0x002D, 0x0000 }, /* R178 - DCDC Sleep options */ \
    { 0x5177, 0x5177, 0x8000 }, /* R179 - Power-check comparator */ \
    { 0x047F, 0x047F, 0x0000 }, /* R180 - DCDC1 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R181 - DCDC1 Timeouts */ \
    { 0x737F, 0x737F, 0x0000 }, /* R182 - DCDC1 Low Power */ \
    { 0x535B, 0x535B, 0x0000 }, /* R183 - DCDC2 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R184 - DCDC2 Timeouts */ \
    { 0x0000, 0x0000, 0x0000 }, /* R185 */ \
    { 0x047F, 0x047F, 0x0000 }, /* R186 - DCDC3 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R187 - DCDC3 Timeouts */ \
    { 0x737F, 0x737F, 0x0000 }, /* R188 - DCDC3 Low Power */ \
    { 0x047F, 0x047F, 0x0000 }, /* R189 - DCDC4 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R190 - DCDC4 Timeouts */ \
    { 0x737F, 0x737F, 0x0000 }, /* R191 - DCDC4 Low Power */ \
    { 0x535B, 0x535B, 0x0000 }, /* R192 - DCDC5 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R193 - DCDC5 Timeouts */ \
    { 0x0000, 0x0000, 0x0000 }, /* R194 */ \
    { 0x047F, 0x047F, 0x0000 }, /* R195 - DCDC6 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R196 - DCDC6 Timeouts */ \
    { 0x737F, 0x737F, 0x0000 }, /* R197 - DCDC6 Low Power */ \
    { 0x0000, 0x0000, 0x0000 }, /* R198 */ \
    { 0xFFD3, 0xFFD3, 0x0000 }, /* R199 - Limit Switch Control */ \
    { 0x441F, 0x441F, 0x0000 }, /* R200 - LDO1 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R201 - LDO1 Timeouts */ \
    { 0x331F, 0x331F, 0x0000 }, /* R202 - LDO1 Low Power */ \
    { 0x441F, 0x441F, 0x0000 }, /* R203 - LDO2 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R204 - LDO2 Timeouts */ \
    { 0x331F, 0x331F, 0x0000 }, /* R205 - LDO2 Low Power */ \
    { 0x441F, 0x441F, 0x0000 }, /* R206 - LDO3 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R207 - LDO3 Timeouts */ \
    { 0x331F, 0x331F, 0x0000 }, /* R208 - LDO3 Low Power */ \
    { 0x441F, 0x441F, 0x0000 }, /* R209 - LDO4 Control */ \
    { 0xFFC0, 0xFFC0, 0x0000 }, /* R210 - LDO4 Timeouts */ \
    { 0x331F, 0x331F, 0x0000 }, /* R211 - LDO4 Low Power */ \
    { 0x0000, 0x0000, 0x0000 }, /* R212 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R213 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R214 */ \
    { 0x8F3F, 0x8F3F, 0x0000 }, /* R215 - VCC_FAULT Masks */ \
    { 0xFF3F, 0xE03F, 0x0000 }, /* R216 - Main Bandgap Control */ \
    { 0xEF2F, 0xE02F, 0x0000 }, /* R217 - OSC Control */ \
    { 0xF3FF, 0xB3FF, 0xc000 }, /* R218 - RTC Tick Control */ \
    { 0xFFFF, 0xFFFF, 0xFFFF }, /* R219 */ \
    { 0x09FF, 0x01FF, 0x0000 }, /* R220 - RAM BIST 1 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R221 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R222 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R223 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R224 */ \
    { 0x8F3F, 0x0000, 0xFFFF }, /* R225 - DCDC/LDO status */ \
    { 0x0000, 0x0000, 0x0000 }, /* R226 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R227 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R228 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R229 */ \
    { 0xFFFF, 0x1FFF, 0xFFFF }, /* R230 - GPIO Pin Status */ \
    { 0xFFFF, 0x1FFF, 0xFFFF }, /* R231 */ \
    { 0xFFFF, 0x1FFF, 0xFFFF }, /* R232 */ \
    { 0xFFFF, 0x1FFF, 0xFFFF }, /* R233 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R234 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R235 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R236 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R237 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R238 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R239 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R240 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R241 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R242 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R243 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R244 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R245 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R246 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R247 */ \
    { 0xFFFF, 0x0010, 0xFFFF }, /* R248 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R249 */ \
    { 0xFFFF, 0x0010, 0xFFFF }, /* R250 */ \
    { 0xFFFF, 0x0010, 0xFFFF }, /* R251 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R252 */ \
    { 0xFFFF, 0x0010, 0xFFFF }, /* R253 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R254 */ \
    { 0x0000, 0x0000, 0x0000 }, /* R255 */ \
}


#define WM8350_IRQ_WKUP_OFF_STATE		43
#define WM8350_IRQ_WKUP_HIB_STATE		44
#define WM8350_IRQ_WKUP_CONV_FAULT		45
#define WM8350_IRQ_WKUP_WDOG_RST		46
#define WM8350_IRQ_WKUP_GP_PWR_ON		47
#define WM8350_IRQ_WKUP_ONKEY		48
#define WM8350_IRQ_WKUP_GP_WAKEUP		49

#endif	/* __LINUX_PMIC_WM8350_H */
