/*
 *  linux/drivers/mmc/mmc_debug.c
 *
 *  Copyright (C) 2003 maintech GmbH, Thomas Kleffel <tk@maintech.de>
 *
 * This file contains debug helper functions for the MMC/SD stack
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include "mmc_debug.h"

char *mmc_cmd2str(int cmd)
{
	switch(cmd) {
		case  0: return "GO_IDLE_STATE";
		case  1: return "ALL_SEND_OCR";
		case  2: return "ALL_SEND_CID";
		case  3: return "ALL_SEND_RELATIVE_ADD";
		case  6: return "ACMD: SD_SET_BUSWIDTH";
		case  7: return "SEL_DESEL_CARD";
		case  9: return "SEND_CSD";
		case 10: return "SEND_CID";
		case 11: return "READ_UNTIL_STOP";
		case 12: return "STOP_TRANSMISSION";
		case 13: return "SEND_STATUS";
		case 15: return "GO_INACTIVE_STATE";
		case 16: return "SET_BLOCKLEN";
		case 17: return "READ_SINGLE_BLOCK";
		case 18: return "READ_MULTIPLE_BLOCK";
		case 24: return "WRITE_SINGLE_BLOCK";
		case 25: return "WRITE_MULTIPLE_BLOCK";
		case 41: return "ACMD: SD_APP_OP_COND";
		case 55: return "APP_CMD";
		default: return "UNKNOWN";
	}
}
EXPORT_SYMBOL(mmc_cmd2str);

char *mmc_err2str(int err)
{
	switch(err) {
		case MMC_ERR_NONE:	return "OK";
		case MMC_ERR_TIMEOUT:	return "TIMEOUT";
		case MMC_ERR_BADCRC: 	return "BADCRC";
		case MMC_ERR_FIFO: 	return "FIFO";
		case MMC_ERR_FAILED: 	return "FAILED";
		case MMC_ERR_INVALID: 	return "INVALID";
		case MMC_ERR_BUSY:	return "BUSY";
		case MMC_ERR_DMA:	return "DMA";
		case MMC_ERR_CANCELED:	return "CANCELED";
		default: return "UNKNOWN";
	}
}
EXPORT_SYMBOL(mmc_err2str);
