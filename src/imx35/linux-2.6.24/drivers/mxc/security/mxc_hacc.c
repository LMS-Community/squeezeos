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
 * @file mxc_hacc.c
 *
 * @brief APIs for HAC Module.
 *
 * This file provides the APIs for accessing Hash Acceleration (HAC)
 * module. HAC module accelerates the creation of a SHA-1 hash over
 * selected memory spaces. The SHA-1 algorithm is a one-way hash
 * algorithm that creates 160 bit hash of any length input.
 *
 * @ingroup MXC_Security
 */

#include <linux/io.h>
#include <linux/clk.h>
#ifdef CONFIG_MXC_HAC_TEST_DEBUG
#include <linux/module.h>
#endif				/* CONFIG_MXC_HAC_TEST_DEBUG */
#include "mxc_hacc.h"

/*!
 * This variable indicates whether HAC module is in suspend or in resume
 * mode.
 */
static unsigned short hac_suspend_state = 0;

/*!
 * This API configures the start address and block length of the data that
 * needs to be hashed. Start address indicates starting location from where
 * the data in the flash memory is to be hashed. The number of blocks that
 * needs to be hashed is loaded in the block count register. This API does
 * the starting of Hashing process or continue with Hashing of next block of
 * data configured in the START_ADDR, BLOCK_COUNT register depending on the
 * hash parameter passed.
 *
 * @param   start_address   Starting address of the flash memory to be hashed.
 *                          user has to pass physical address here.
 * @param   blk_len         Number of blocks to be hashed.
 * @param   option          Mode of operation like Start or Continue hashing.\n
 *                          Following parameters are passed:
 *                          HAC_START       : Starts the Hashing process.
 *                          HAC_LAST_START  : Starts the Hashing process with
 *                                            last block of data.
 *                          HAC_CONTINUE    : Continue the Hashing process.
 *                          HAC_LAST        : Continue the Hashing process with
 *                                            last block of data.
 *
 *
 * @return  HAC_SUCCESS    Successfully hashed the data.\n
 *          HAC_FAILURE    Error in the parameters passed.
 *          HAC_BUSY       HAC module is busy in Hashing process.
 */
hac_ret hac_hash_data(ulong start_address, ulong blk_len, hac_hash option)
{
	struct clk *clk;
	ulong hac_start, hac_blk_cnt, hac_ctl;
	hac_ret ret_val = HAC_SUCCESS;

	clk = clk_get(NULL, "hac_clk");
	clk_enable(clk);
	hac_start = __raw_readl(HAC_START_ADDR);
	hac_blk_cnt = __raw_readl(HAC_BLK_CNT);
	hac_ctl = __raw_readl(HAC_CTL);
	if (hac_suspend_state == 1) {
		pr_debug("HAC Module: HAC Module is in suspend mode.\n");
		return -EPERM;
	}
	pr_debug("Function %s. HAC Module: Start address: 0x%08lX, "
		 "block length: 0x%08lX, hash option: 0x%08X\n",
		 __FUNCTION__, start_address, blk_len, option);
	/* Validating the parameters. Checking for start address to be in
	   512 bit boundary(64 byte) and block count value must not to be
	   zero. */
	if ((!start_address) || (blk_len > HAC_MAX_BLOCK_LENGTH) ||
	    (blk_len == 0) || (!((start_address % 64) == 0))) {
		pr_debug("HAC Module: Invalid parameters passed. \n");
		return HAC_FAILURE;
	}
	if ((hac_ctl & HAC_CTL_BUSY) == 0) {
		hac_start = start_address;
		__raw_writel(hac_start, HAC_START_ADDR);
		hac_blk_cnt = blk_len;
		__raw_writel(hac_blk_cnt, HAC_BLK_CNT);
		pr_debug("HAC Module: Hashing start address 0x%08lX\n ",
			 start_address);
		pr_debug("HAC Module: Hashing blk length 0x%08lX\n ", blk_len);
	} else {
		pr_debug("HAC Module: HAC module is busy in Hashing "
			 "process.\n");
		return HAC_HASH_BUSY;
	}

	switch (option) {
	case HAC_START:
		/*
		 * HAC_START will starts the Hashing of data in the memory.
		 * Before starting the Hashing process, it checks for 'STOP'
		 * bit, 'DONE' bit and 'ERROR' bit is set in the HAC Control
		 * register. If 'STOP' bit is set, it clears the 'STOP' bit in
		 * HAC Control register. If 'DONE' bit and  'ERROR' bit are
		 * set, they are cleared.
		 */
		pr_debug("HAC Module: Starts the hashing process \n");
		/* Checking if the Stop bit is been set. */
		if ((hac_ctl & HAC_CTL_STOP) == HAC_CTL_STOP) {
			pr_debug("HAC Module: STOP bit is set while"
				 "starting the Hashing\n");
			hac_ctl &= ~HAC_CTL_STOP;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if the 'DONE' bit and 'ERROR' bit is been set.
		   If they are set write to clear those bits */
		if (((hac_ctl & HAC_CTL_DONE) == HAC_CTL_DONE) ||
		    ((hac_ctl & HAC_CTL_ERROR) == HAC_CTL_ERROR)) {
			pr_debug("HAC Module: DONE and ERROR bit is set"
				 "while starting the Hashing\n");
			hac_ctl |= HAC_CTL_DONE;
			__raw_writel(hac_ctl, HAC_CTL);
			hac_ctl |= HAC_CTL_ERROR;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		hac_ctl |= HAC_CTL_START;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	case HAC_START_LAST:
		/*
		 * HAC_START_LAST will starts the Hashing of last block of data
		 * in the memory. Since this is last block of data in the
		 * memory 'PAD' bit in HAC control register is set to add
		 * appropriate padding to the end of the data structure.
		 * Before starting the Hashing process, it checks for 'STOP'
		 * bit, 'DONE' bit and 'ERROR' bit is set in the HAC Control
		 * register. If 'STOP' bit is set, it clears the 'STOP' bit in
		 * HAC Control register. If 'DONE' bit and  'ERROR' bit are
		 * set, they are cleared.
		 */
		pr_debug("HAC Module: Starts with last block"
			 "the hashing process \n");
		/* Checking if the Stop bit is been set. */
		if ((hac_ctl & HAC_CTL_STOP) == HAC_CTL_STOP) {
			pr_debug("HAC Module: STOP bit is set while"
				 "starting the Hashing\n");
			hac_ctl &= ~HAC_CTL_STOP;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if the 'DONE' bit and 'ERROR' bit is been set.
		   If they are set write to clear those bits */
		if (((hac_ctl & HAC_CTL_DONE) == HAC_CTL_DONE) ||
		    ((hac_ctl & HAC_CTL_ERROR) == HAC_CTL_ERROR)) {
			pr_debug(" HAC Module: DONE and ERROR bit is set"
				 "while  starting the Hashing\n");
			hac_ctl |= HAC_CTL_DONE;
			__raw_writel(hac_ctl, HAC_CTL);
			hac_ctl |= HAC_CTL_ERROR;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		hac_ctl |= HAC_CTL_START;
		__raw_writel(hac_ctl, HAC_CTL);
		/* Hash for the last block by padding it. */
		pr_debug("HAC Module: Setting the PAD bit while start"
			 "Hashing the last block\n");
		hac_ctl |= HAC_CTL_PAD;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	case HAC_CONTINUE:
		/*
		 * HAC_CONTINUE will continue the Hashing of data in the memory.
		 * This will continue the hashing processing by taking into
		 * consideration of the previous hash result and continues
		 * further hashing of the new data block. Before continue the
		 * Hashing process, it checks for 'STOP' bit, 'DONE' bit and
		 * 'ERROR' bit is set in the HAC Control register. If 'STOP'
		 * bit is set, it clears the 'STOP' bit in Control register.
		 * If 'DONE' bit is set, it clears the 'DONE' bit in control
		 * register. If 'ERROR' bit is set, then error message is
		 * indicated to the user.
		 */
		pr_debug("HAC Module: Continue hashing process. \n");
		/* Checking if the Stop bit is been set. */
		if ((hac_ctl & HAC_CTL_STOP) == HAC_CTL_STOP) {
			hac_ctl &= ~HAC_CTL_STOP;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if the 'DONE' bit is been set. If it is set write
		   one to clear the bit */
		if ((hac_ctl & HAC_CTL_DONE) == HAC_CTL_DONE) {
			hac_ctl |= HAC_CTL_DONE;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if 'ERROR' bit is been set. If it is set resturn
		   return back indicating error in Hashing porcess. */
		if ((hac_ctl & HAC_CTL_ERROR) == HAC_CTL_ERROR) {
			return HAC_FAILURE;
		}
		hac_ctl |= HAC_CTL_CONTINUE;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	case HAC_LAST:
		/*
		 * HAC_LAST will continue the Hashing of last block of data
		 * in the memory. Since this is last block of data in the
		 * memory 'PAD' bit in HAC control register is set to add
		 * appropriate padding to the end of the data structure.
		 * This will continue the hashing processing by taking into
		 * consideration of the previous hash result and continues
		 * further hashing of the new data block. Before continue the
		 * Hashing process, it checks for 'STOP' bit, 'DONE' bit and
		 * 'ERROR' bit is set in the HAC Control register. If 'STOP'
		 * bit is set, it clears the 'STOP' bit in Control register.
		 * If 'DONE' bit is set, it clears the 'DONE' bit in control
		 * register. If 'ERROR' bit is set, then error message is
		 * indicated to the user.
		 */
		pr_debug("HAC Module: Last block to hash. \n");
		/* Checking if the Stop bit is been set. */
		if ((hac_ctl & HAC_CTL_STOP) == HAC_CTL_STOP) {
			hac_ctl &= ~HAC_CTL_STOP;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if the 'DONE' bit is been set. If it is set write
		   one to clear the bit */
		if ((hac_ctl & HAC_CTL_DONE) == HAC_CTL_DONE) {
			hac_ctl |= HAC_CTL_DONE;
			__raw_writel(hac_ctl, HAC_CTL);
		}
		/* Checking if 'ERROR' bit is been set. If it is set resturn
		   return back indicating error in Hashing porcess. */
		if ((hac_ctl & HAC_CTL_ERROR) == HAC_CTL_ERROR) {
			return HAC_FAILURE;
		}
		hac_ctl |= HAC_CTL_CONTINUE;
		__raw_writel(hac_ctl, HAC_CTL);
		/* Continuing the hash for the last block by padding it. */
		hac_ctl |= HAC_CTL_PAD;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	default:
		ret_val = HAC_FAILURE;
		/* NOT RESPONDING */
		break;
	}
	return ret_val;
}

/*!
 * This API returns the status of the Hashing.
 *
 * @return      HAC_BUSY : Indicated HAC Module is busy with Hashing.\n
 *              HAC_DONE : Indicates Hashing of data is done.\n
 *              HAC_ERR  : Indicates error has occurred during Hashing.\n
 *              HAC_UNKNOWN: Hashing status unknown. This may be when the
 *                           hashing process has not been initiated atleast
 *                           once or 'ERROR' bit or 'DONE' bits were reset
 *                           after the hashing process was completed.
 */
hac_hash_status hac_hashing_status(void)
{
	ulong hac_ctl;
	hac_ctl = __raw_readl(HAC_CTL);
	if ((hac_ctl & HAC_CTL_BUSY) != 0) {
		pr_debug("HAC Module: Hash module is in busy state \n");
		return HAC_BUSY;
	} else if ((hac_ctl & HAC_CTL_DONE) != 0) {
		/* Clearing the done bit of the control register */
		pr_debug("HAC Module: Hashing of data is done \n");
		return HAC_DONE;
	} else if ((hac_ctl & HAC_CTL_ERROR) != 0) {
		/* Clearing the error bit of the control register */
		pr_debug("HAC Module: Error has occurred during hashing \n");
		return HAC_ERR;
	} else {
		return HAC_UNKNOWN;
	}
}

/*!
 * This API returns the status of the Hash module.
 *
 * @return      Value of the Hashing control register.
 */
ulong hac_get_status(void)
{
	ulong hac_ctl = __raw_readl(HAC_CTL);
	pr_debug("HAC Module: Hashing status register value 0x%08lX\n ",
		 hac_ctl);
	return hac_ctl;
}

/*!
 * This API stops the Hashing of data when the Hashing is in progress.
 */
hac_ret hac_stop(void)
{
	ulong hac_ctl;
	hac_ctl = __raw_readl(HAC_CTL);
	if (hac_suspend_state == 1) {
		pr_debug("HAC Module: HAC Module is in suspend mode.\n");
		return HAC_FAILURE;
	}
	pr_debug("HAC Module: Stop hashing process. \n");
	hac_ctl |= HAC_CTL_STOP;
	__raw_writel(hac_ctl, HAC_CTL);
	return HAC_SUCCESS;
}

/*!
 * This API reads 160 bit hash result from Hash result register. The data is
 * copied to the memory pointed by the input pointer.
 *
 * @param    hash_result_reg    structure Pointer where the hash result is
 *                              copied.
 */
hac_ret hac_hash_result(hac_hash_rlt * hash_result_reg)
{
	ulong hac_hsh4, hac_hsh3, hac_hsh2, hac_hsh1, hac_hsh0;
	struct clk *clk;

	clk = clk_get(NULL, "hac_clk");
	hac_hsh4 = __raw_readl(HAC_HSH4);
	hac_hsh3 = __raw_readl(HAC_HSH3);
	hac_hsh2 = __raw_readl(HAC_HSH2);
	hac_hsh1 = __raw_readl(HAC_HSH1);
	hac_hsh0 = __raw_readl(HAC_HSH0);
	clk_disable(clk);
	if (hac_suspend_state == 1) {
		pr_debug("HAC Module: HAC Module is in suspend mode.\n");
		return HAC_FAILURE;
	}
	pr_debug("HAC Module: Read hash result \n");
	hash_result_reg->hash_result[0] = hac_hsh4;
	hash_result_reg->hash_result[1] = hac_hsh3;
	hash_result_reg->hash_result[2] = hac_hsh2;
	hash_result_reg->hash_result[3] = hac_hsh1;
	hash_result_reg->hash_result[4] = hac_hsh0;
	return HAC_SUCCESS;
}

/*!
 * This API will initiates software reset of the entire HAC module. It resets
 * all state machine to their default values. All status bits (BUSY/ERROR/DONE)
 * and any pending interrupts are cleared.
 *
 * @return  HAC_SUCCESS    Successfully in doing software reset.\n
 *          HAC_FAILURE    Error in doing software reset.
 */
hac_ret hac_swrst(void)
{
	ulong hac_ctl;
	ulong hac_ret = HAC_SUCCESS;
	hac_ctl = __raw_readl(HAC_CTL);
	pr_debug("HAC Module: HAC Software reset function. \n");
	if (hac_suspend_state == 1) {
		pr_debug("HAC MODULE: HAC Module is in suspend mode.\n");
		return HAC_FAILURE;
	}
	hac_ctl |= HAC_CTL_SWRST;
	__raw_writel(hac_ctl, HAC_CTL);
	return hac_ret;
}

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
hac_ret hac_burst_mode(hac_burst_mode_config burst_mode)
{
	ulong hac_ctl;
	ulong hac_ret = HAC_SUCCESS;
	hac_ctl = __raw_readl(HAC_CTL);
	pr_debug("HAC Module: HAC Burst Mode function. \n");
	if (hac_suspend_state == 1) {
		pr_debug("HAC MODULE: HAC Module is in suspend mode.\n");
		return HAC_FAILURE;
	}
	switch (burst_mode) {
	case HAC_INR_BURST:
		hac_ctl |= HAC_CTL_BURST_MODE;
		break;

	case HAC_16WORD_BURST:
		hac_ctl &= ~HAC_CTL_BURST_MODE;
		break;

	default:
		hac_ret = HAC_FAILURE;
		break;
	}
	return hac_ret;
}

/*!
 * This API configures HAC burst read nature.
 *
 * @param  burst_read   Configures burst read.
 *
 * @return  HAC_SUCCESS    Successfully in configuring burst read.\n
 *          HAC_FAILURE    Error in configuring burst read.
 */
hac_ret hac_burst_read(hac_burst_read_config burst_read)
{
	ulong hac_ctl;
	ulong hac_ret = HAC_SUCCESS;
	hac_ctl = __raw_readl(HAC_CTL);
	pr_debug("HAC Module: HAC Burst Read function. \n");
	if (hac_suspend_state == 1) {
		pr_debug("HAC MODULE: HAC Module is in suspend mode.\n");
		return HAC_FAILURE;
	}
	switch (burst_read) {
	case HAC_16WORD_BURST_READ:
		__raw_writel(HAC_CTL_16WORD_BURST, HAC_CTL);
		break;

	case HAC_8WORD_BURST_READ:
		hac_ctl &= ~HAC_CTL_NO_BURST_READ;
		hac_ctl |= HAC_CTL_8WORD_BURST;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	case HAC_4WORD_BURST_READ:
		hac_ctl &= ~HAC_CTL_NO_BURST_READ;
		hac_ctl |= HAC_CTL_4WORD_BURST;
		__raw_writel(hac_ctl, HAC_CTL);
		break;

	case HAC_NO_WORD_BURST_READ:
		hac_ctl &= ~HAC_CTL_NO_BURST_READ;
		hac_ctl |= HAC_CTL_NO_BURST_READ;
		break;

	default:
		hac_ret = HAC_FAILURE;
		break;
	}
	return hac_ret;
}

#ifdef CONFIG_PM
/*!
 * This function is called to put the HAC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on HAC
 *                to suspend.
 * @param   state the power state the device is entering.
 *
 * @return  The function always returns HAC_SUCCESS.
 */
hac_ret hac_suspend(struct platform_device * pdev, pm_message_t state)
{
	ulong hac_ctl;
	struct clk *clk;

	hac_ctl = __raw_readl(HAC_CTL);
	clk = clk_get(NULL, "hac_clk");
	hac_suspend_state = 1;

	pr_debug("HAC Module: In suspend power down.\n");

	/* Enable stop bits in HAC Control Register. */
	hac_ctl |= HAC_CTL_STOP;
	__raw_writel(hac_ctl, HAC_CTL);
	clk_disable(clk);

	return HAC_SUCCESS;
}

/*!
 * This function is called to bring the HAC back from a low power state.
 * Refer to the document driver-model/driver.txt in the kernel source tree
 * for more information.
 *
 * @param   pdev  the device structure used to give information on HAC
 *                to resume.
 *
 * @return  The function always returns HAC_SUCCESS.
 */
hac_ret hac_resume(struct platform_device * pdev)
{
	ulong hac_ctl;
	struct clk *clk;

	clk = clk_get(NULL, "hac_clk");
	clk_enable(clk);
	hac_ctl = __raw_readl(HAC_CTL);

	pr_debug("HAC Module: Resume power on.\n");
	/* Disable stop bit in HAC Control register. */
	hac_ctl &= ~HAC_CTL_STOP;
	__raw_writel(hac_ctl, HAC_CTL);

	hac_suspend_state = 0;

	return HAC_SUCCESS;
}
#else
#define mxc_hac_suspend NULL
#define mxc_hac_resume  NULL
#endif				/* CONFIG_PM */
