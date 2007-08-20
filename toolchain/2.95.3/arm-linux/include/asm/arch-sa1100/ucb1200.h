/*
 *  linux/include/asm-arm/arch-sa1100/ucb1200.h
 *
 *  Copyright (c) Compaq Computer Corporation, 1998, 1999
 *
 *  Authors: Carl Waldspurger and Larry Brakmo.
 *
 *  23 October 2000 - John Dorsey
 *                    Begin to generalize UCB1200/UCB1300 support
 *                    so that we can use Assabet switches.
 */

#if !defined(_ASM_ARCH_UCB1200_H)
#define _ASM_ARCH_UCB1200_H

#include <asm/hardware.h>

/*
 * Codec registers
 */
#define CODEC_REG_IO_DATA (0)
#define CODEC_REG_IO_DIRECTION (1)
#define CODEC_REG_RISE_INT_ENABLE (2)
#define CODEC_REG_FALL_INT_ENABLE (3)
#define CODEC_REG_INT_STATUS (4)
#define CODEC_REG_TELECOM_CTL_A (5)
#define CODEC_REG_TELECOM_CTL_B (6)
#define CODEC_REG_AUDIO_CTL_A (7)
#define CODEC_REG_AUDIO_CTL_B (8)
#define CODEC_REG_TS_CTL (9)
#define CODEC_REG_ADC_CTL (10)
#define CODEC_REG_ADC_DATA (11)
#define CODEC_REG_ID (12)
#define CODEC_REG_MODE (13)

/*
 * Codec registers 2, 3 and 4: interrupt related registers
 */
#define ADC_INT (1 << 11)
#define TSPX_INT (1 << 12)
#define TSMX_INT (1 << 13)

/*
 * Codec register 9: Touchscreen control register
 */
#define TSMX_POW (1 << 0)
#define TSPX_POW (1 << 1)
#define TSMY_POW (1 << 2)
#define TSPY_POW (1 << 3)
#define TSMX_GND (1 << 4)
#define TSPX_GND (1 << 5)
#define TSMY_GND (1 << 6)
#define TSPY_GND (1 << 7)
#define TSC_MODE_MASK (3 << 8)
#define TSC_MODE_INT (0 << 8)
#define TSC_MODE_PRESSURE (1 << 8)
#define TSC_MODE_POSITION (1 << 9)
#define TSC_BIAS_ENA (1 << 11)
#define TSPX_LOW (1 << 12)
#define TSMX_LOW (1 << 13)

/*
 * Codec register 10: ADC control register
 */
#define ADC_SYNC_ENA (1 << 0)
#define ADC_INPUT_MASK (7 << 2)
#define ADC_INPUT_TSPX (0 << 2)
#define ADC_INPUT_TSMX (1 << 2)
#define ADC_INPUT_TSPY (2 << 2)
#define ADC_INPUT_TSMY (3 << 2)
#define ADC_INPUT_AD0 (4 << 2)
#define ADC_INPUT_AD1 (5 << 2)
#define ADC_INPUT_AD2 (6 << 2)
#define ADC_INPUT_AD3 (7 << 2)
#define ADC_START (1 << 7)
#define ADC_ENA (1 << 15)

/*
 * Codec register 11: ADC data register
 */
#define ADC_DATA_SHIFT_VAL (5)
#define GET_ADC_DATA(x) (((x) >> ADC_DATA_SHIFT_VAL) & 0x3ff)
#define ADC_DAT_VAL (1 << 15)
#define ADC_ENA_TYPE (ADC_SYNC_ENA | ADC_ENA)


/* Useful operations */
static inline ushort codec_read(ushort addr)
{
    ulong flags;
    ushort data;

    save_flags_cli(flags);
    while (!(Ser4MCSR & MCSR_CWC));
    Ser4MCDR2 = ((addr & 0xf) << FShft(MCDR2_ADD)) | MCDR2_Rd;
    while (!(Ser4MCSR & MCSR_CRC));
    data = Ser4MCDR2 & 0xffff;
    restore_flags(flags);
    return data;
}

static inline void codec_write(ushort addr, ushort data)
{
    ulong flags;

    save_flags_cli(flags);
    while (!(Ser4MCSR & MCSR_CWC));
    Ser4MCDR2 = ((addr & 0xf) << FShft(MCDR2_ADD)) | MCDR2_Wr | (data & 0xffff);
    restore_flags(flags);
}

#endif  /* !defined(_ASM_ARCH_UCB1200_H) */
