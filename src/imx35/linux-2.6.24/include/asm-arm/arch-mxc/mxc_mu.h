/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @defgroup MU Messaging Unit (MU) Driver
 */

/*!
 * @file arch-mxc/mxc_mu.h
 *
 * @brief This file contains the MU chip level configuration details and
 * public API declarations.
 *
 * @ingroup MU
 */

#ifndef __ASM_ARCH_MXC_MU_H__
#define __ASM_ARCH_MXC_MU_H__

#ifdef __KERNEL__

#include <asm/hardware.h>

/* MU configuration */

/*!
 * Number of channels in MU indicated using NUM_MU_CHANNELS
 */
#define NUM_MU_CHANNELS          4

/*!
 * Channel transmit/receive size indicated using CH_SIZE
 */
#define CH_SIZE                  4

/*!
 * Maximum Buffer Size is a sequence of 16, 32, 64, 128,256,...
 * ie., a value where only the MSB is 1. The value should be a
 * power of 2. It should be noted that the buffer is filled and
 * emptied as multiples of 4 bytes. Also, 4 bytes of the total allocated
 * space will not be used since empty space calculation always returns
 * available space minus 1 inorder to differentiate between full and empty
 * buffer.
 */
#define MAX_BUF_SIZE             1024
#endif				/* __KERNEL__ */

/*!
 * IOCTL commands
 */

/*!
 * SENDINT      : To issue general purpose interrupt to DSP
 */
#define SENDINT                  1

/*!
 * SENDNMI      : To issue NMI to DSP
 */
#define SENDNMI                  2

/*!
 * SENDDSPRESET : To issue DSP Hardware Reset
 */
#define SENDDSPRESET             3

/*!
 * SENDMURESET  : To issue MU Reset
 */
#define SENDMURESET              4

/*!
 * RXENABLE     : To enable receive interrupt
 */
#define RXENABLE                 5

/*!
 * TXENABLE     : To enable transmit interrupt
 */
#define TXENABLE                 6

/*!
 * GPENABLE     : To enable general purpose interrupt
 */
#define GPENABLE                 7

/*!
 * BYTESWAP_RD  : To enable byte swapping on read
 */
#define BYTESWAP_RD              8

/*!
 * BYTESWAP_WR  : To enable byte swapping on write
 */
#define BYTESWAP_WR               9

/*!
 * Error Indicators
 */
/*!
 * Error: NO_DATA: If no data is available in receive register or
 * if no transmit register is empty, this error is returned
 */
#define NO_DATA                  129

/*!
 * Error: NO_ACCESS: If user is accessing an incorrect register i.e., a register
 * not allocated to user, this error is returned
 */
#define NO_ACCESS                132

/*!
 * enum mu_oper
 * This datatype is to indicate whether the operation
 * related to receive, transmit or general purpose
 */
enum mu_oper {
	TX,
	RX,
	GP,
};

/*!
 * To declare array of function pointers of type callbackfn
 */
typedef void (*callbackfn) (int chnum);

#ifdef __KERNEL__
/*!
 * This function is called by other modules to bind their
 * call back functions.
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   callback      the caller's callback function
 * @param   muoper        The value passed is TX, RX, GP
 *
 * @return                Returns 0 on success or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_bind(int chand, callbackfn callback, enum mu_oper muoper);

/*!
 * This function is called by other modules to unbind their
 * call back functions
 *
 * @param   chand    The channel handler associated with the channel
 *                   to be accessed is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP, or
 *                   if disabling interrupt failed
 */
int mxc_mu_unbind(int chand, enum mu_oper muoper);

/*!
 * Allocating Resources:
 * Allocating channels required to the caller if they are free.
 * 'reg_allocated' variable holds all the registers that have been allocated
 *
 * @param   chnum   The channel number required is passed
 *
 * @return          Returns unique channel handler associated with channel. This
 *                  channel handler is used to validate the user accessing the
 *                  channel.
 *                  If invalid channel is passed, it returns REG_INVALID error.
 *                  If no channel is available, returns error.
 *                  Returns channel handler - on success
 *                  Returns -1 - on error
 */
int mxc_mu_alloc_channel(int chnum);

/*!
 * Deallocating Resources:
 *
 * @param   chand   The channel handler associated with the channel
 *                  that is to be freed is passed.
 *
 * @return          Returns 0 on success and -1 if channel was not
 *                  already allocated or REG_INVALID error if invalid
 *                  channel is obtained from the channel handler
 */
int mxc_mu_dealloc_channel(int chand);

/*!
 * This function is called by other modules to read from one of the
 * receive registers on the MCU side.
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   mu_msg        Buffer where the read data has to be stored
 *                        is passed by pointer as an argument
 *
 * @return                Returns 0 on success or
 *                        Returns NO_DATA error if there is no data to read or
 *                        Returns NO_ACCESS error if the incorrect channel
 *                        is accessed or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuread(int chand, char *mu_msg);

/*!
 * This function is called by other modules to write to one of the
 * transmit registers on the MCU side
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   mu_msg        Buffer where the write data has to be stored
 *                        is passed by pointer as an argument
 *
 * @return                Returns 0 on success or
 *                        Returns NO_DATA error if the register not empty or
 *                        Returns NO_ACCESS error if the incorrect channel
 *                        is accessed or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuwrite(int chand, char *mu_msg);

/*!
 * This function is called by the user and other modules to enable desired
 * interrupt
 *
 * @param   chand     The channel handler associated with the channel
 *                    to be accessed is passed
 * @param    muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intenable(int chand, enum mu_oper muoper);

/*!
 * This function is called by unbind function of this module and
 * can also be called from other modules to enable desired
 * receive Interrupt
 *
 * @param   chand    The channel handler associated with the channel
 *                   whose interrupt to be disabled is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intdisable(int chand, enum mu_oper muoper);

/*!
 * This function is used by other modules to reset the MU Unit. This would reset
 * both the MCU side and the DSP side
 */
int mxc_mu_reset(void);

/*!
 * This function is used by other modules to issue DSP hardware reset.
 */
int mxc_mu_dsp_reset(void);

/*!
 * This function is used by other modules to deassert DSP hardware reset.
 */
int mxc_mu_dsp_deassert(void);

/*!
 * This function is used by other modules to issue DSP Non-Maskable
 * Interrupt to the DSP
 */
int mxc_mu_dsp_nmi(void);

/*!
 * This function is used by other modules to retrieve the DSP reset state.
 *
 * @return   Returns 1 if DSP in reset state and 0 otherwise
 */
int mxc_mu_dsp_reset_status(void);

/*!
 * This function is used by other modules to retrieve the DSP Power Mode.
 *
 * @return   Returns a value from which power mode of the DSP side of
 *           of MU unit can be inferred
 *           0 - Run mode,  1 - Wait mode
 *           2 - Stop mode, 3 - DSM mode
 */
unsigned int mxc_mu_dsp_pmode_status(void);

#endif				/* __KERNEL__ */

#endif				/* __ASM_ARCH_MXC_MU_H__ */
