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
 * @file mxc_rtic.c
 *
 * @brief APIs for RTIC Module.
 *
 * \b
 * This file provides the APIs for accessing Run-Time Integrity Checker (RTIC)
 * module. RTIC module accelerates the creation of a SHA-1 hash over
 * selected memory spaces. The RTIC has the ability to verify the memory
 * contents during system boot and during run-time execution.
 *
 * @ingroup MXC_Security
 */

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#ifdef CONFIG_MXC_RTIC_TEST_DEBUG
#include <linux/module.h>
#endif				/* CONFIG_MXC_RTIC_TEST_DEBUG */
#include <asm/io.h>
#include <asm/arch/clock.h>
#include "mxc_rtic.h"

/*!
 * The following api is used to enable the RTIC IP CLK and RTIC HCLK.
 * Before start using other APIs make sure that init is called.
 *
 * @param 	void
 *
 * return	void
 */

void rtic_init(void)
{
	struct clk *clk;

	clk = clk_get(NULL, "rtic_clk");
	clk_enable(clk);
}

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
rtic_ret rtic_configure_mode(rtic_mode mode, rtic_memblk mem_blk)
{
	ulong rtic_ctrl, rtic_sts, rtic_cmd;
	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	rtic_sts = __raw_readl(RTIC_STATUS);
	rtic_cmd = __raw_readl(RTIC_COMMAND);
	/*
	 * RTIC does not support enabling multiple memory blocks that aren't
	 * grouped together(i.e. enabling only memory blocks A & C without
	 * memory B enabled).
	 */
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (mode) {
	case RTIC_ONE_TIME:
		break;

	case RTIC_RUN_TIME:
#ifndef CONFIG_ARCH_MX27
		/* Check RUN Time Disable bit before enabling RUN Time memory
		   blocks. */
		if ((rtic_cmd & RTIC_RUN_TIME_DISABLE) == RTIC_RUN_TIME_DISABLE) {
			rtic_cmd &= ~RTIC_RUN_TIME_DISABLE;
			__raw_writel(rtic_cmd, RTIC_COMMAND);
		}
#endif				/* CONFIG_ARCH_MX27 */
		switch (mem_blk) {
		case RTIC_A1:
			pr_debug("RTIC Module:Memory Block A is enabled for"
				 "Run_Time Hashing.\n");
			rtic_ctrl &= ~RTIC_CTL_HASHONCE_MEMA_BLK_EN;
			rtic_ctrl |= RTIC_CTL_RUNTIME_MEMA_BLK_EN;
			__raw_writel(rtic_ctrl, RTIC_CONTROL);
			break;

		case RTIC_B1:
			pr_debug("RTIC Module:Memory Block B is "
				 "enabled for Run_Time Hashing.\n");
			rtic_ctrl &= ~RTIC_CTL_HASHONCE_MEMB_BLK_EN;
			rtic_ctrl |= RTIC_CTL_RUNTIME_MEMB_BLK_EN;
			__raw_writel(rtic_ctrl, RTIC_CONTROL);
			break;

		case RTIC_C1:
			pr_debug("RTIC Module:Memory Block C is "
				 "enabled for Run_Time Hashing.\n");
			rtic_ctrl &= ~RTIC_CTL_HASHONCE_MEMC_BLK_EN;
			rtic_ctrl |= RTIC_CTL_RUNTIME_MEMC_BLK_EN;
			__raw_writel(rtic_ctrl, RTIC_CONTROL);
			break;

		case RTIC_D1:
			pr_debug("RTIC Module:Memory Block D is "
				 "enabled for Run_Time Hashing.\n");
			rtic_ctrl &= ~RTIC_CTL_HASHONCE_MEMD_BLK_EN;
			rtic_ctrl |= RTIC_CTL_RUNTIME_MEMD_BLK_EN;
			__raw_writel(rtic_ctrl, RTIC_CONTROL);
			break;

		default:
			ret_val = RTIC_FAILURE;
			break;

		}
		break;
	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

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
				rtic_memblk mem_blk, int hash_select)
{
	rtic_ret ret_val = RTIC_SUCCESS;
	ulong rtic_sts, rtic_ctrl;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	/* Validating the parameters passed. Checking for start address is
	 * word boundary aligned. Checking for block length is multiple of
	 * 4 bytes. */
	if ((!start_addr) || (blk_len > RTIC_MAX_BLOCK_LENGTH) ||
	    (!((start_addr % 4) == 0)) || (!((blk_len % 4) == 0))) {
		return RTIC_FAILURE;
	}
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (mem_blk) {
	case RTIC_A1:
		pr_debug("RTIC Module: Mem Block A1 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMAADDR1);
		pr_debug("RTIC Module: Mem Block A1 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMALEN1);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMA_BLK_EN;
		if (hash_select) {
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMA_BLK_EN;
		}
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;

	case RTIC_A2:
		pr_debug("RTIC Module: Mem Block A2 start address"
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMAADDR2);
		pr_debug("RTIC Module: Mem Block A2 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMALEN2);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMA_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMA_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;

	case RTIC_B1:
		pr_debug("RTIC Module: Mem Block B1 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMBADDR1);
		pr_debug("RTIC Module: Mem Block B1 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMBLEN1);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMB_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMB_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);

		break;

	case RTIC_B2:
		pr_debug("RTIC Module: Mem Block B2 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMBADDR2);
		pr_debug("RTIC Module: Mem Block B2 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMBLEN2);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMB_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMB_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;

	case RTIC_C1:
		pr_debug("RTIC Module: Mem Block C1 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMCADDR1);
		pr_debug("RTIC Module: Mem Block C1 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMCLEN1);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMC_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMC_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);

		break;

	case RTIC_C2:
		pr_debug("RTIC Module: Mem Block C2 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMCADDR2);
		pr_debug("RTIC Module: Mem Block C2 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMCLEN2);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMC_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMC_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);

		break;

	case RTIC_D1:
		pr_debug("RTIC Module: Mem Block D1 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMDADDR1);
		pr_debug("RTIC Module: Mem Block D1 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMDLEN1);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMD_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMD_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);

		break;

	case RTIC_D2:
		pr_debug("RTIC Module: Mem Block D2 start address "
			 "0x%08lX\n", start_addr);
		__raw_writel(start_addr, RTIC_MEMDADDR2);
		pr_debug("RTIC Module: Mem Block D2 block len "
			 "0x%08lX\n", blk_len);
		__raw_writel(blk_len, RTIC_MEMDLEN2);
		rtic_ctrl |= RTIC_CTL_HASHONCE_MEMD_BLK_EN;
		if (hash_select)
			rtic_ctrl |= RTIC_CTL_HASHALGO_MEMD_BLK_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);

		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This API will configure to start the Hashing of data in memory
 * either in One-Time Hash mode or Run-Time Hash mode.
 *
 * @param   mode    RTIC mode of operation (ONE_TIME or RUN_TIME).
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */
rtic_ret rtic_start_hash(rtic_mode mode)
{
	rtic_ret ret_val = RTIC_SUCCESS;
	ulong rtic_cmd, rtic_sts;
	/* Read for RTIC Registers value. */
	rtic_cmd = __raw_readl(RTIC_COMMAND);
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}

	switch (mode) {
	case RTIC_ONE_TIME:
		pr_debug("RTIC Module: Starts the One_time hashing"
			 "process \n");
		rtic_cmd |= RTIC_CMD_HASH_ONCE;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	case RTIC_RUN_TIME:
		pr_debug("RTIC Module: Starts the Run_time hashing"
			 "process \n");
#ifndef CONFIG_ARCH_MX27
		/* Check RUN Time Disable bit before starting RUN Time
		   Hashing. */
		if ((rtic_cmd & RTIC_RUN_TIME_DISABLE) == RTIC_RUN_TIME_DISABLE) {
			rtic_cmd &= ~RTIC_RUN_TIME_DISABLE;
		}
#endif				/* CONFIG_ARCH_MX27 */
		rtic_cmd |= RTIC_CMD_RUN_TIME_CHK;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}

	return ret_val;
}

/*!
 * This API will read the RTIC status register.
 *
 * @return  Status of Hashing.
 */
ulong rtic_get_status(void)
{
	ulong rtic_sts;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	pr_debug("RTIC  Module: Hashing status register value 0x%08lX\n ",
		 rtic_sts);
	return rtic_sts;
}

/*!
 * This API will read the RTIC control register.
 *
 * @return  Control register value.
 */
ulong rtic_get_control(void)
{
	ulong rtic_ctrl;
	/* Read for RTIC Registers value. */
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	pr_debug("RTIC  Module: Hashing control register value 0x%08lX\n ",
		 rtic_ctrl);
	return rtic_ctrl;
}

/*!
 * This API enables or disables interrupt for RTIC module.
 *
 * @param  irq_en    To enable or disable interrupt.
 *
 * @return  RTIC_SUCCESS    RTIC Interrupt configured Successfully .\n
 *          RTIC_FAILURE    Error RTIC Interrupt configured.
 */
rtic_ret rtic_configure_interrupt(rtic_interrupt irq_en)
{
	rtic_ret ret_val = RTIC_SUCCESS;
	ulong rtic_ctrl, rtic_sts;
	/* Read for RTIC Registers value. */
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (irq_en) {
	case RTIC_INTERRUPT_ENABLE:
		pr_debug("RTIC Module: RTIC Interrupt enabled.\n");
		/* If in interrupt mode then, set the irq enable bit in
		   the RTIC control register */
		rtic_ctrl |= RTIC_CTL_IRQ_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;

	case RTIC_INTERRUPT_DISABLE:
		pr_debug("RTIC  Module: RTIC Interrupt Disabled.\n");
		/* If in polling mode, then disable the irq enable bit in
		   the RTIC Control register */
		rtic_ctrl &= ~RTIC_CTL_IRQ_EN;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This API reads the Fault address of the RTIC module.
 *
 * @return   Fault address register value.
 */
ulong rtic_get_faultaddress(void)
{
	ulong rtic_faultaddr;
	/* Read for RTIC Registers value. */
	rtic_faultaddr = __raw_readl(RTIC_FAULTADDR);
	pr_debug("RTIC  Module: Hashing fault register value 0x%08lX\n ",
		 rtic_faultaddr);
	return rtic_faultaddr;
}

/*!
 * This API reads 160/256 bit hash result from Hash result register for
 * SHA1/SHA256.The data is copied to the memory pointed by the input pointer.
 * SHA256 is supported only in RTIC2.
 *
 * @param    hash_result_reg    Hashed Value.
 * @param    hash_reg           Hash result register address.
 * @param    hash_num           Number of Hash Values.
 *
 * @return  RTIC_SUCCESS    Successfully hashed the data.\n
 *          RTIC_FAILURE    Error in the parameters passed.
 */

rtic_ret rtic_hash_read(rtic_hash_rlt * hash_result_reg, ulong hash_reg,
			int hash_num)
{
	int i;
	rtic_ret ret_val = RTIC_SUCCESS;
	for (i = 0; i < hash_num; i++) {
		hash_result_reg->hash_result[i] = __raw_readl(hash_reg);
		hash_reg += 4;
	}
	return ret_val;

}

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
			 int hash_num)
{
	int i;
	ulong hash_reg;
	rtic_ret ret_val = RTIC_SUCCESS;
	hash_reg = RTIC_MEMAHASHRES0 + (mem_blk * 20);
	for (i = 0; i < hash_num; i++) {
		__raw_writel(hash_result_reg->hash_result[i], hash_reg);
		hash_reg += 4;
	}
	return ret_val;
}

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
rtic_ret
rtic_hash_result(rtic_memblk mem_blk, rtic_mode mode,
		 rtic_hash_rlt * hash_result_reg)
{
	rtic_ret ret_val = RTIC_SUCCESS;
	ulong rtic_sts, rtic_ctrl;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	switch (mode) {
	case RTIC_ONE_TIME:
		/* In the one-time hash, Check Done interrupt before
		   reading hash results. */
		if ((rtic_sts & RTIC_DONE) == RTIC_DONE) {
			switch (mem_blk) {
			case RTIC_A1:
				pr_debug("RTIC Module: Read mem blk A hash"
					 "result\n");
				if ((rtic_ctrl & RTIC_CTL_HASHALGO_MEMA_BLK_EN)
				    == RTIC_CTL_HASHALGO_MEMA_BLK_EN) {
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMAHASHRES0, 8);
				} else
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMAHASHRES0, 5);
				break;

			case RTIC_B1:
				pr_debug("RTIC Module: Read mem blk B hash"
					 "result\n");
				if ((rtic_ctrl & RTIC_CTL_HASHALGO_MEMB_BLK_EN)
				    == RTIC_CTL_HASHALGO_MEMB_BLK_EN) {
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMBHASHRES0, 8);
				} else
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMBHASHRES0, 5);
				break;

			case RTIC_C1:
				pr_debug("RTIC Module: Read mem blk C hash"
					 "result\n");
				if ((rtic_ctrl & RTIC_CTL_HASHALGO_MEMC_BLK_EN)
				    == RTIC_CTL_HASHALGO_MEMC_BLK_EN) {
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMCHASHRES0, 8);
				} else
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMCHASHRES0, 5);
				break;

			case RTIC_D1:
				pr_debug("RTIC Module: Read mem blk D hash"
					 "result\n");
				if ((rtic_ctrl & RTIC_CTL_HASHALGO_MEMD_BLK_EN)
				    == RTIC_CTL_HASHALGO_MEMD_BLK_EN) {
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMDHASHRES0, 8);
				} else
					rtic_hash_read(hash_result_reg,
						       RTIC_MEMDHASHRES0, 5);
				break;

			default:
				ret_val = RTIC_FAILURE;
				break;
			}
		} else {
			pr_debug("RTIC Module: Memory blocks are not hashed "
				 "for boot authentication.\n");
			ret_val = RTIC_FAILURE;
		}
		break;

	case RTIC_RUN_TIME:
		switch (mem_blk) {
		case RTIC_A1:
			pr_debug("RTIC Module: run-time check doesn't update"
				 "Hash result value.\n");
			ret_val = RTIC_FAILURE;
			break;

		case RTIC_B1:
			pr_debug("RTIC Module: run-time check doesn't update"
				 "Hash result value.\n");
			ret_val = RTIC_FAILURE;
			break;

		case RTIC_C1:
			pr_debug("RTIC Module: run-time check doesn't update"
				 "Hash result value.\n");
			ret_val = RTIC_FAILURE;
			break;

		case RTIC_D1:
			pr_debug("RTIC Module: run-time check doesn't update"
				 "Hash result value.\n");
			ret_val = RTIC_FAILURE;
			break;

		default:
			ret_val = RTIC_FAILURE;
			break;
		}
		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This API configures RTIC DMA Burst Size. It configures maximum number of
 * words to burst read through DMA. This cannot be modified during run-time.
 *
 *      @param  dma_burst       Maximum DMA Burst Size.
 *
 *      @return  RTIC_SUCCESS    RTIC DMA successfully configured.\n
 *               RTIC_FAILURE    Error DMA configuration.
 */
rtic_ret rtic_dma_burst_read(rtic_dma_word dma_burst)
{
	ulong rtic_ctrl;
	ulong rtic_dma_delay, rtic_sts;
	rtic_ret ret_val = RTIC_SUCCESS;
	pr_debug("RTIC DMA burst read confirgured.\n");
	/* Read for RTIC Registers value. */
	rtic_ctrl = __raw_readl(RTIC_CONTROL);
	rtic_sts = __raw_readl(RTIC_STATUS);
	rtic_dma_delay = __raw_readl(RTIC_DMA);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (dma_burst) {
	case RTIC_DMA_1_WORD:
		rtic_ctrl &= ~DMA_BURST_CLR;
		rtic_ctrl |= DMA_1_WORD;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;
	case RTIC_DMA_2_WORD:
		rtic_ctrl &= ~DMA_BURST_CLR;
		rtic_ctrl |= DMA_2_WORD;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;
	case RTIC_DMA_4_WORD:
		rtic_ctrl &= ~DMA_BURST_CLR;
		rtic_ctrl |= DMA_4_WORD;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;
	case RTIC_DMA_8_WORD:
		rtic_ctrl &= ~DMA_BURST_CLR;
		rtic_ctrl |= DMA_8_WORD;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;
	case RTIC_DMA_16_WORD:
		rtic_ctrl &= ~DMA_BURST_CLR;
		rtic_ctrl |= DMA_16_WORD;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
		break;
	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This API sets DMA throttle register to 0x00 during boot time to minimize the
 * performance impact at startup.
 *
 *    @return  RTIC_SUCCESS    DMA throttle register set to 0x00 successfully.
 *             RTIC_FAILURE    Failure in programing DMA throttle register..
 */
rtic_ret rtic_hash_once_dma_throttle(void)
{
	ulong rtic_sts;
	ulong rtic_ctrl;

	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}

	if (cpu_is_mx27() || (cpu_is_mx31_rev(CHIP_REV_1_0) == 1)) {
		__raw_writel(0x00, RTIC_DMA);
	} else if (cpu_is_mx31_rev(CHIP_REV_2_0) >= 1) {
		/* Setting Hash Once DMA Throttle timer to Zero. */
		rtic_ctrl = __raw_readl(RTIC_CONTROL);
		rtic_ctrl &= ~HASH_ONCE_DMA_THROTTLE_CLR;
		__raw_writel(rtic_ctrl, RTIC_CONTROL);
	}

	return ret_val;
}

/*! This API programs the DMA Programmable Timer to set to specify how many
 * cycles to wait between DMA bus access.
 *
 *      @param  dma_delay       DMA Bus Duty Cycle Delay.
 *
 *      @return  RTIC_SUCCESS    RTIC DMA Duty Cycle delay set successfully.\n
 *               RTIC_FAILURE    Failure in programing DMA bus delay.
 */
rtic_ret rtic_dma_delay(ulong dma_delay)
{
	ulong rtic_sts;
	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	__raw_writel(dma_delay, RTIC_DMA);
	return ret_val;
}

/*!
 * This API Configures DMA Watchdog timer.
 *
 * @param       wd_timer        DMA Watchdog timer value.
 *
 * @return  RTIC_SUCCESS    RTIC DMA Watchdog Timer set successfully.\n
 *          RTIC_FAILURE    Failure in programing DMA Watchdog Timer.
 *
 */
rtic_ret rtic_wd_timer(ulong wd_timer)
{
	ulong rtic_sts;
	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	__raw_writel(wd_timer, RTIC_WDTIMER);
	return ret_val;
}

/*!
 * This API is used for Software reset RTIC Module.
 *
 * @param   rtic_rst        To Enable and Disable RTIC SW Reset.
 *
 * @return  RTIC_SUCCESS    RTIC SW Reset Configured successfully.\n
 *          RTIC_FAILURE    Failure in configuring.
 */
rtic_ret rtic_sw_reset(rtic_sw_rst rtic_rst)
{
	ulong rtic_cmd, rtic_sts;
	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_cmd = __raw_readl(RTIC_COMMAND);
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (rtic_rst) {
	case RTIC_RST_ENABLE:
		pr_debug("RTIC Module: RTIC SW Reset enabled.\n");
		rtic_cmd |= RTIC_CMD_SW_RESET;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	case RTIC_RST_DISABLE:
		pr_debug("RTIC  Module: RTIC SW Reset Disabled.\n");
		rtic_cmd &= ~RTIC_CMD_SW_RESET;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This API is used RTIC to clear IRQ.
 *
 * @param   rtic_irq_clr    To Clear RTIC IRQ.
 *
 * @return  RTIC_SUCCESS    Clear RTIC IRQ Successfully.\n
 *          RTIC_FAILURE    Failure in Clearing RTIC IRQ.
 */
rtic_ret rtic_clr_irq(rtic_clear_irq rtic_irq_clr)
{
	ulong rtic_cmd, rtic_sts;
	rtic_ret ret_val = RTIC_SUCCESS;
	/* Read for RTIC Registers value. */
	rtic_cmd = __raw_readl(RTIC_COMMAND);
	rtic_sts = __raw_readl(RTIC_STATUS);
	/* Check for RTIC Busy bit before writing into RTIC Registers. */
	if ((rtic_sts & RTIC_BUSY) != 0) {
		pr_debug("RTIC Module: RTIC is in BUSY in Hashing\n");
		return RTIC_FAILURE;
	}
	switch (rtic_irq_clr) {
	case RTIC_CLR_IRQ_ENABLE:
		pr_debug("RTIC Module: RTIC SW Reset enabled.\n");
		rtic_cmd |= RTIC_CMD_CLRIRQ;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	case RTIC_CLR_IRQ_DISABLE:
		pr_debug("RTIC  Module: RTIC SW Reset Disabled.\n");
		rtic_cmd &= ~RTIC_CMD_CLRIRQ;
		__raw_writel(rtic_cmd, RTIC_COMMAND);
		break;

	default:
		ret_val = RTIC_FAILURE;
		break;
	}
	return ret_val;
}
