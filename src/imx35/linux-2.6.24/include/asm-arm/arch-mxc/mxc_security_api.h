
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

/*!
 * @defgroup MXCSECDRVRS Security Drivers
 */

/*!
 * @defgroup MXC_Security Security Drivers for HACC and RTIC
 * @ingroup MXCSECDRVRS
 */
/*!
 * @defgroup RNG RNG driver module for RNGA and RNGC
 * @ingroup MXCSECDRVRS
 */

/*!
 * @file arch-mxc/mxc_security_api.h
 *
 * @brief MXC Security user header file.
 *
 * This header file provides interface for accessing the Security module
 * mentioned below. It contains declarations of datatypes and functions. The
 * modules that it contains are:
 *
 * \b HAC is used to Hash the data stored in the memory. It can even Hash the
 * data in non-contiguous memory locations.
 *
 * \b RTIC is used to Hash the data stored in the memory during system boot
 * and during run-time execution.
 *
 * @ingroup MXC_Security
 */

#ifndef __ASM_ARCH_MXC_SECURITY_API_H__
#define __ASM_ARCH_MXC_SECURITY_API_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
typedef unsigned char unchar;
#endif
#include <linux/platform_device.h>

/*!
 * @name HAC module data types.
 */

/*! @{ */

/*!
 * These values are returned to indicate the status after the Hashing process
 * in the HAC module.
 */
typedef enum {
	/*!
	 * Hashing of data is completed.
	 */
	HAC_DONE,

	/*!
	 * Busy hashing the data.
	 */
	HAC_BUSY,

	/*!
	 * Hashing was aborted due to bus error during HAC access over
	 * the AHB (Advanced High-Performance Bus).
	 */
	HAC_ERR,

	/*!
	 * Hashing status unknown. This may be when the hashing process has not
	 * been initiated altleast once or 'ERROR' bit or 'DONE' bits were
	 * reset after the hashing process was completed.
	 */
	HAC_UNKNOWN
} hac_hash_status;

/*!
 * These parameters are used for controlling the Hashing action to be
 * performed.
 */
typedef enum {
	/*!
	 * Start the Hashing process.
	 */
	HAC_START,

	/*!
	 * Start the Hashing of data with last block.
	 */
	HAC_START_LAST,

	/*!
	 * Continue the Hashing process.
	 */
	HAC_CONTINUE,

	/*!
	 * Continue the Hashing of data with last block.
	 */
	HAC_LAST
} hac_hash;

/*!
 * These values are returned for HAC routines.
 */
typedef enum {
	/*!
	 * Successfully finished the routine.
	 */
	HAC_SUCCESS = 0,

	/*!
	 * Encountered some errors in the routine.
	 */
	HAC_FAILURE = -1,

	/*!
	 * HAC module is busy in Hashing process.
	 */
	HAC_HASH_BUSY = -2
} hac_ret;

/*!
 * These parameters are used to configure HAC Burst mode.
 */
typedef enum {
	/*!
	 * Configures Incremental Burst Mode.
	 */
	HAC_INR_BURST,

	/*!
	 * Configures 16WORD Burst Mode.
	 */
	HAC_16WORD_BURST
} hac_burst_mode_config;

/*!
 * These parameters are used to configure HAC Burst Read.
 */
typedef enum {
	/*!
	 * Configures 16 Word Burst Read.
	 */
	HAC_16WORD_BURST_READ,

	/*!
	 * Configures 8 Word Burst Read.
	 */
	HAC_8WORD_BURST_READ,

	/*!
	 * Configures 4 Word Burst Read.
	 */
	HAC_4WORD_BURST_READ,

	/*!
	 * Configures No Word Burst Read.
	 */
	HAC_NO_WORD_BURST_READ
} hac_burst_read_config;

/*!
 * Gets Hashing result value from HSH Registers.
 */
typedef struct {
	/*!
	 * Stores the hash result.
	 */
	ulong hash_result[8];
} hac_hash_rlt;

/*! @} */

/*!
 * @name RTIC module data types.
 */

/*! @{ */

/*!
 * These parameters are used for selecting RTIC mode of operation.
 */
typedef enum {
	/*!
	 * RTIC in ONE_TIME Hash Mode.
	 */
	RTIC_ONE_TIME,

	/*!
	 * RTIC in RUN_TIME Hash mode.
	 */
	RTIC_RUN_TIME
} rtic_mode;

/*!
 * These parameter are used to enable or disable the interrupt.
 */
typedef enum {
	/*!
	 * RTIC interrupt enable.
	 */
	RTIC_INTERRUPT_ENABLE,

	/*!
	 * RTIC interrupt disable.
	 */
	RTIC_INTERRUPT_DISABLE
} rtic_interrupt;

/*!
 * These parameter are used to enable or disable the RTIC SW Reset.
 */
typedef enum {
	/*!
	 * RTIC SW RESET enable.
	 */
	RTIC_RST_ENABLE,

	/*!
	 * RTIC SW RESET disable.
	 */
	RTIC_RST_DISABLE
} rtic_sw_rst;

/*!
 * These parameter are used to Clear IRQ.
 */
typedef enum {
	/*!
	 * RTIC CLR IRQ enable.
	 */
	RTIC_CLR_IRQ_ENABLE,

	/*!
	 * RTIC CLR IRQ disable.
	 */
	RTIC_CLR_IRQ_DISABLE
} rtic_clear_irq;

/*!
 * These parameters are used to select RTIC memory block for Hashing.
 */
typedef enum {
	/*!
	 * Select Memory Block A1.
	 */
	RTIC_A1,

	/*!
	 * Select Memory Block A2.
	 */
	RTIC_A2,

	/*!
	 * Select Memory Block B1.
	 */
	RTIC_B1,

	/*!
	 * Select Memory Block B2.
	 */
	RTIC_B2,

	/*!
	 * Select Memory Block C1.
	 */
	RTIC_C1,

	/*!
	 * Select Memory Block C2.
	 */
	RTIC_C2,

	/*!
	 * Select Memory Block D1.
	 */
	RTIC_D1,

	/*!
	 * Select Memory Block D2.
	 */
	RTIC_D2
} rtic_memblk;

/*!
 * These values are returned when queried for the status of the RTIC module.
 */
typedef enum {
	/*!
	 * Busy hashing the data.
	 */
	RTIC_STAT_HASH_BUSY = 0x0,

	/*!
	 * Hashing of data is completed.
	 */
	RTIC_STAT_HASH_DONE = 0x02,

	/*!
	 * Hashing was aborted due to corruption in memory block
	 * during RUN_TIME Hashing or address/length error has
	 * occurred.
	 */
	RTIC_STAT_HASH_ERR = 0x04,

	/*!
	 * Hashing status unknown. This may happen when: 1) Hashing process has
	 * not been initiated atleast once. 2) Error bit or Done bit was reset
	 * after the hashing process was completed.
	 */
	RTIC_UNKNOWN
} rtic_status;

/*!
 * These values are returned for RTIC routines.
 */
typedef enum {
	/*!
	 * Successfully finished the routine.
	 */
	RTIC_SUCCESS = 0,

	/*!
	 * Encountered some errors in the routine.
	 */
	RTIC_FAILURE = -1
} rtic_ret;

/*!
 * These parameters are used to configure RTIC DMA Burst size.
 */
typedef enum {
	/*!
	 * This parameter will configure DMA Burst read as 1 word.
	 */
	RTIC_DMA_1_WORD,
	/*!
	 * This parameter will configure DMA Burst read as 2 words.
	 */
	RTIC_DMA_2_WORD,
	/*!
	 * This parameter will configure DMA Burst read as 4 words.
	 */
	RTIC_DMA_4_WORD,
	/*!
	 * This parameter will configure DMA Burst read as 8 words.
	 */
	RTIC_DMA_8_WORD,
	/*!
	 * This parameter will configure DMA Burst read as 16 words.
	 */
	RTIC_DMA_16_WORD,
} rtic_dma_word;

/*!
 * Gets Hashing result value from RTIC Hash Result Registers.
 */
typedef struct {
	/*!
	 * Stores the hash result.
	 */
	ulong hash_result[8];
} rtic_hash_rlt;
/*! @} */

/*!
 * @name HAC module APIs.
 */

/*! @{ */

/*!
 * This API configures the start address and block length of the data that needs
 * to be hashed. Start address indicates starting location from where the data
 * in the flash memory is to be hashed. The number of blocks that needs to be
 * hashed is loaded in the block count register. This API also does the stating
 * of Hashing process or continue with Hashing of next block of data configured
 * in the START_ADDR, BLOCK_COUNT register depending on the hash parameter
 * passed.
 *
 * @param   start_address   starting address of the flash memory to be hashed.
 * @param   blk_len         number of blocks to be hashed.
 * @param   hash            mode of operation like Start or Continue hashing.
 *                          Following parameters are passed:
 *                          HAC_START       : Starts the Hashing process.
 *                          HAC_LAST_START  : Starts the Hashing process with
 *                                            last block of data.
 *                          HAC_CONTINUE    : Continue the Hashing process.
 *                          HAC_LAST        : Continue the Hashing process with
 *                                            last block of data.
 *
 * @return  HAC_SUCCESS    Success on completion of configuring HAC for
 *                         Hashing.\n
 *          HAC_FAILURE    Failure in completion.
 */
hac_ret hac_hash_data(ulong start_address, ulong blk_len, hac_hash hash);

/*!
 * This API returns the status of the Hashing.
 *
 * @return      HAC_BUSY : Indicated HAC Module is busy with Hashing.\n
 *              HAC_DONE : Indicates Hashing of data is done.\n
 *              HAC_ERR  : Indicates error has occurred during Hashing.
 *              HAC_UNKNOWN: Hashing status unknown. This may be when the
 *                           hashing process has not been initiated atleast
 *                           once or 'ERROR' bit or 'DONE' bits were reset
 *                           after the hashing process was completed.
 */
hac_hash_status hac_hashing_status(void);

/*!
 * This API returns the status of the Hash module.
 *
 * @return      Value of the Hashing control register.
 */
ulong hac_get_status(void);

/*!
 * This API stops the Hashing process when the Hashing is in progress.
 */
hac_ret hac_stop(void);

/*!
 * This API reads 160/256 bit hash result from Hash result register for
 * SHA1/SHA256. The data is copied to the memory pointed by the input pointer.
 *
 * @param    hash_result_reg    structure Pointer where the hash result is
 *                              copied.
 */
hac_ret hac_hash_result(hac_hash_rlt * hash_result_reg);

/*!
 * This API will initiates software reset of the entire HAC module. It resets
 * all state machine to their default values. All status bits (BUSY/ERROR/DONE)
 * and any pending interrupts are cleared.
 *
 * @return  HAC_SUCCESS    Successfully in doing software reset.\n
 *          HAC_FAILURE    Error in doing software reset.
 */
hac_ret hac_swrst(void);

/*!
 * This API configures the burst mode of the HAC. When Burst mode set in HAC
 * Control register then ARM9 is configured for a 16-WORD burst, while Burst
 * mode is cleared then ARM9 is configured for a incremental burst.
 *
 * @param  burst_mode   Configures burst mode operations.
 *
 * @return  HAC_SUCCESS    Successfully in configuring burst mode.\n
 *          HAC_FAILURE    Error in configuring burst mode.
 */
hac_ret hac_burst_mode(hac_burst_mode_config burst_mode);

/*!
 * This API configures HAC burst read nature.
 *
 * @param  burst_read   Configures burst read.
 *
 * @return  HAC_SUCCESS    Successfully in configuring burst read.\n
 *          HAC_FAILURE    Error in configuring burst read.
 */
hac_ret hac_burst_read(hac_burst_read_config burst_read);

/*!
 * This function is called to put the HAC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on HAC
 *                to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
hac_ret hac_suspend(struct platform_device *pdev, pm_message_t state);

/*!
 * This function is called to bring the HAC back from a low power state.
 * Refer to the document driver-model/driver.txt in the kernel source tree
 * for more information.
 *
 * @param   dev   the device structure used to give information on HAC
 *                to resume.
 * @param   level the stage in device resumption process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
hac_ret hac_resume(struct platform_device *pdev);

/*! @} */

/*!
 * @name RTIC module APIs.
 */

/*! @{ */

/*!
 * The following api is used to enable the RTIC IP CLK and RTIC HCLK.
 * Before start using other APIs make sure that init is called.
 *
 * @param   void
 *
 * return   void
 */
void rtic_init(void);

/*!
 * This API configures the memory block (A, B, C, D) into
 * RUN_TIME Mode for Hashing of data in memory. RTIC does not support
 * enabling multiple memory blocks that aren't grouped together
 * (i.e. enabling only memory blocks A & C without memory B enabled).
 *
 * @param  mode         RTIC mode of operation (ONE_TIME or RUN_TIME).
 * @param  mem_blk      Memory block to be hashed.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */

rtic_ret rtic_configure_mode(rtic_mode mode, rtic_memblk mem_blk);

/*!
 * This API allows to configure start address,block length of the memory
 * and select (SHA1/SHA256) Hash algorithm to hash the data. Start address
 * indicates starting location from where the data in the memory is to
 * be hashed. The number of blocks that needs to be hashed is loaded in
 * the block count register. There are four memory blocks available. The
 * user can configure any one of these four memory blocks by passing their
 * appropriate address and block length to be Hashed. SHA 256 is supported
 * only in RTIC2. This API configures the memory block (A, B, C, D) into
 * ONE_TIME Mode for Hashing of data in memory.
 *
 * @param    start_addr       Starting address of the memory to be hashed.
 * @param    blk_len          Block length of data in memory.
 * @param    mem_blk          Memory block to be hashed.
 * @param    hash_select      Select the (SHA1/SHA256)Hash Algorithm.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */

rtic_ret rtic_configure_mem_blk(ulong start_addr, ulong blk_len,
				rtic_memblk mem_blk, int hash_select);

/*!
 * This API will configure to start the Hashing of data in memory
 * either in One-Time Hash mode or Run-Time Hash mode.
 *
 * @param       mode    Memory block to be hashed.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */
rtic_ret rtic_start_hash(rtic_mode mode);

/*!
 * This API will read the RTIC status register.
 *
 * @return  Status of Hashing.
 */
ulong rtic_get_status(void);

/*!
 * This API will read the RTIC control register.
 *
 * @return  Control register value.
 */
ulong rtic_get_control(void);

/*!
 * This API enables or disables interrupt for RTIC module.
 *
 * @param  irq_en    To enable or disable interrupt.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */
rtic_ret rtic_configure_interrupt(rtic_interrupt irq_en);

/*!
 * This API reads the Fault address of the RTIC module.
 *
 * @return   Fault address register value.
 */
ulong rtic_get_faultaddress(void);
/*!
 * This API writes 160/256 bit hash result from input pointer to Hash
 * result register.This is used after warm boot to restore the hash
 * Value and continue with Run Time Hashing.It avoid doing One time
 * Hashing again if it is done before Warm Boot.This feature is supported
 * only for RTIC2.
 *
 * @param    hash_result_reg    Hashed Value.
 * @param    hash_reg           Hash Register Address.
 * @param    hash_num           Number of 4 bytes for a Hash value.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */
rtic_ret rtic_hash_write(rtic_hash_rlt * hash_result_reg, rtic_memblk mem_blk,
			 int hash_num);
/*!
 * This API reads 160/256 bit hash result from Hash result register for
 * SHA1/SHA256.The data is copied to the memory pointed by the input pointer.
 * SHA256 is supported only in RTIC2.
 *
 * @param    mem_blk            Memory block to be hashed.
 * @param    mode               RTIC mode of operation (ONE_TIME or RUN_TIME).
 * @param    hash_result_reg    Hashed value.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */

rtic_ret rtic_hash_result(rtic_memblk mem_blk, rtic_mode mode,
			  rtic_hash_rlt * hash_result_reg);

/*!
 * This API configures RTIC DMA Burst Size. It configures maximum number of
 * words to burst read through DMA. This cannot be modified during run-time.
 *
 *      @param  dma_burst       Maximum DMA Burst Size.
 *
 *      @return  RTIC_SUCCESS    RTIC DMA successfully configured.\n
 *               RTIC_FAILURE    Error DMA configuration.
 */
rtic_ret rtic_dma_burst_read(rtic_dma_word dma_burst);

/*!
 * This API sets DMA throttle register to 0x00 during boot time to minimize the
 * performance impact at startup.
 *
 *    @return  RTIC_SUCCESS    DMA throttle register set to 0x00 successfully.
 *             RTIC_FAILURE    Failure in programing DMA throttle register..
 */
rtic_ret rtic_hash_once_dma_throttle(void);

/*! This API programs the DMA Programmable Timer to set to specify how many
 * cycles to wait between DMA bus access.
 *
 *      @param  dma_delay       DMA Bus Duty Cycle Delay.
 *
 *      @return  RTIC_SUCCESS    RTIC DMA Duty Cycle delay set successfully.\n
 *               RTIC_FAILURE    Failure in programing DMA bus delay.
 */
rtic_ret rtic_dma_delay(ulong dma_delay);

/*!
 * This API Configures DMA Watchdog timer.
 *
 * @param       wd_timer        DMA Watchdog timer value.
 *
 * @return  RTIC_SUCCESS    RTIC DMA Watchdog Timer set successfully.\n
 *          RTIC_FAILURE    Failure in programing DMA Watchdog Timer.
 *
 */
rtic_ret rtic_wd_timer(ulong wd_timer);

/*!
 * This API is used for Software reset RTIC Module.
 *
 * @param   rtic_rst        To Enable and Disable RTIC SW Reset.
 *
 * @return  RTIC_SUCCESS    RTIC SW Reset Configured successfully.\n
 *          RTIC_FAILURE    Failure in configuring.
 */
rtic_ret rtic_sw_reset(rtic_sw_rst rtic_rst);

/*!
 * This API is used RTIC to clear IRQ.
 *
 * @param   rtic_irq_clr    To Clear RTIC IRQ.
 *
 * @return  RTIC_SUCCESS    Clear RTIC IRQ Successfully.\n
 *          RTIC_FAILURE    Failure in Clearing RTIC IRQ.
 */
rtic_ret rtic_clr_irq(rtic_clear_irq rtic_irq_clr);

/*! @} */

#endif				/* __ASM_ARCH_MXC_SECURITY_API_H__ */
