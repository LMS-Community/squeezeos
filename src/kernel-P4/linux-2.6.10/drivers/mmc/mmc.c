/*
 *  linux/drivers/mmc/mmc.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *
 *  SD-Card support:
 *    Copyright (C) 2004 Thomas Kleffel, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol.h>

#include "mmc.h"

#ifdef CONFIG_MMC_DEBUG
#define DBG(x...)	printk(KERN_DEBUG x)
#else
#define DBG(x...)	do { } while (0)
#endif

#define CMD_RETRIES	3

/*
 * OCR Bit positions to 10s of Vdd mV.
 */
static const unsigned short mmc_ocr_bit_to_vdd[] = {
	150,	155,	160,	165,	170,	180,	190,	200,
	210,	220,	230,	240,	250,	260,	270,	280,
	290,	300,	310,	320,	330,	340,	350,	360
};

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};


/**
 *	mmc_request_done - finish processing an MMC command
 *	@host: MMC host which completed command
 *	@mrq: MMC request which completed
 *
 *	MMC drivers should call this function when they have completed
 *	their processing of a command.  This should be called before the
 *	data part of the command has completed.
 */
void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	int err = mrq->cmd->error;
	DBG("MMC: req done (%02x): %d: %08x %08x %08x %08x\n", cmd->opcode,
	    err, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

	if (err && cmd->retries) {
		cmd->retries--;
		cmd->error = 0;
		host->ops->request(host, mrq);
	} else if (mrq->done) {
		mrq->done(mrq);
	}
}

EXPORT_SYMBOL(mmc_request_done);

/**
 *	mmc_start_request - start a command on a host
 *	@host: MMC host to start command on
 *	@mrq: MMC request to start
 *
 *	Queue a command on the specified host.  We expect the
 *	caller to be holding the host lock with interrupts disabled.
 */
void
mmc_start_request(struct mmc_host *host, struct mmc_request *mrq)
{
	DBG("MMC: starting cmd %02x arg %08x flags %08x\n",
	    mrq->cmd->opcode, mrq->cmd->arg, mrq->cmd->flags);

	WARN_ON(host->card_busy == NULL);

	mrq->cmd->error = 0;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		mrq->cmd->data = mrq->data;
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
		if (mrq->stop) {
			mrq->data->stop = mrq->stop;
			mrq->stop->error = 0;
			mrq->stop->mrq = mrq;
		}
	}
	host->ops->request(host, mrq);
}

EXPORT_SYMBOL(mmc_start_request);

static void mmc_wait_done(struct mmc_request *mrq)
{
	complete(mrq->done_data);
}

int mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
	DECLARE_COMPLETION(complete);

	mrq->done_data = &complete;
	mrq->done = mmc_wait_done;

	mmc_start_request(host, mrq);

	wait_for_completion(&complete);

	return 0;
}

EXPORT_SYMBOL(mmc_wait_for_req);

/**
 *	mmc_wait_for_cmd - start a command and wait for completion
 *	@host: MMC host to start command
 *	@cmd: MMC command to start
 *	@retries: maximum number of retries
 *
 *	Start a new MMC command for a host, and wait for the command
 *	to complete.  Return any error that occurred while the command
 *	was executing.  Do not attempt to parse the response.
 */
int mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd, int retries)
{
	struct mmc_request mrq;

	BUG_ON(host->card_busy == NULL);

	memset(&mrq, 0, sizeof(struct mmc_request));

	memset(cmd->resp, 0, sizeof(cmd->resp));
	cmd->retries = retries;

	mrq.cmd = cmd;
	cmd->data = NULL;

	mmc_wait_for_req(host, &mrq);

	return cmd->error;
}

EXPORT_SYMBOL(mmc_wait_for_cmd);

static int mmc_send_app_cmd(struct mmc_host *host,  u16 rca)
{
	struct mmc_command cmd;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg =((u32)rca)<<16;
	cmd.flags = MMC_RSP_R1;

	return mmc_wait_for_cmd(host, &cmd, 0);
}

int mmc_wait_for_acmd(struct mmc_host *host, u16 rca, struct mmc_command *cmd, int retries)
{
	int err;

	do {
		err = mmc_send_app_cmd(host, rca);
		if(err != MMC_ERR_NONE) continue;

		err = mmc_wait_for_cmd(host, cmd, 0);
		if(err == MMC_ERR_NONE) break;

	} while (retries--);

	return err;
}




/**
 *	__mmc_claim_host - exclusively claim a host
 *	@host: mmc host to claim
 *	@card: mmc card to claim host for
 *
 *	Claim a host for a set of operations.  If a valid card
 *	is passed and this wasn't the last card selected, select
 *	the card before returning.
 *
 *	Note: you should use mmc_card_claim_host or mmc_claim_host.
 */
int __mmc_claim_host(struct mmc_host *host, struct mmc_card *card)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long flags;
	int err = 0;

	add_wait_queue(&host->wq, &wait);
	spin_lock_irqsave(&host->lock, flags);
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (host->card_busy == NULL)
			break;
		spin_unlock_irqrestore(&host->lock, flags);
		schedule();
		spin_lock_irqsave(&host->lock, flags);
	}
	set_current_state(TASK_RUNNING);
	host->card_busy = card;
	spin_unlock_irqrestore(&host->lock, flags);
	remove_wait_queue(&host->wq, &wait);

	if (card != (void *)-1 && host->card_selected != card) {
		struct mmc_command cmd;

		host->card_selected = card;

		cmd.opcode = MMC_SELECT_CARD;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	}

	return err;
}

EXPORT_SYMBOL(__mmc_claim_host);

/**
 *	mmc_release_host - release a host
 *	@host: mmc host to release
 *
 *	Release a MMC host, allowing others to claim the host
 *	for their operations.
 */
void mmc_release_host(struct mmc_host *host)
{
	unsigned long flags;

	BUG_ON(host->card_busy == NULL);

	spin_lock_irqsave(&host->lock, flags);
	host->card_busy = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	wake_up(&host->wq);
}

EXPORT_SYMBOL(mmc_release_host);

/*
 * Ensure that no card is selected.
 */
static void mmc_deselect_cards(struct mmc_host *host)
{
	struct mmc_command cmd;

	if (host->card_selected) {
		host->card_selected = NULL;

		cmd.opcode = MMC_SELECT_CARD;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_NONE;

		mmc_wait_for_cmd(host, &cmd, 0);
	}
}


static inline void mmc_delay(unsigned int ms)
{
	if (ms < HZ / 1000) {
		yield();
		mdelay(ms);
	} else {
		msleep_interruptible (ms);
	}
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
static u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
	int bit;

	ocr &= host->ocr_avail;

	bit = ffs(ocr);
	if (bit) {
		bit -= 1;

		ocr = 3 << bit;

		host->ios.vdd = bit;
		host->ops->set_ios(host, &host->ios);
	} else {
		ocr = 0;
	}

	return ocr;
}

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const u32 __mask = (1 << (size)) - 1;			\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if ((size) + __shft >= 32)				\
			__res |= resp[__off-1] << (32 - __shft);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mmc_decode_cid(struct mmc_card *card)
{
	u32 *resp = card->raw_cid;

	memset(&card->cid, 0, sizeof(struct mmc_cid));

	if(card->sd) {
		if(card->csd.csd_vers != SD_CSDV_1) {
			printk("%s: card has unknown SD-MMCA version %d\n", card->host->host_name, card->csd.spec_vers);
			mmc_card_set_bad(card);
		} else {
			card->cid.manfid	= UNSTUFF_BITS(resp, 120,  8);
			card->cid.oemid		= UNSTUFF_BITS(resp, 104, 16);
			card->cid.prod_name[0]	= UNSTUFF_BITS(resp,  96,  8);
			card->cid.prod_name[1]	= UNSTUFF_BITS(resp,  88,  8);
			card->cid.prod_name[2]	= UNSTUFF_BITS(resp,  80,  8);
			card->cid.prod_name[3]	= UNSTUFF_BITS(resp,  72,  8);
			card->cid.prod_name[4]	= UNSTUFF_BITS(resp,  64,  8);
			card->cid.prod_name[5]  = 0;
			card->cid.prod_name[6]  = 0;
			card->cid.hwrev		= UNSTUFF_BITS(resp,  60,  4);
			card->cid.fwrev		= UNSTUFF_BITS(resp,  56,  4);
			card->cid.serial	= UNSTUFF_BITS(resp,  24, 32);
			card->cid.month		= UNSTUFF_BITS(resp,  12,  4);
			card->cid.year		= UNSTUFF_BITS(resp,   8,  4) + 2000;
		}
	} else {
		switch (card->csd.csd_vers) {
		case MMC_CSDV_1: /* MMC v1.? */
		case MMC_CSDV_14: /* MMC v1.4 */
			card->cid.manfid	= UNSTUFF_BITS(resp, 104, 24);
			card->cid.prod_name[0]	= UNSTUFF_BITS(resp,  96,  8);
			card->cid.prod_name[1]	= UNSTUFF_BITS(resp,  88,  8);
			card->cid.prod_name[2]	= UNSTUFF_BITS(resp,  80,  8);
			card->cid.prod_name[3]	= UNSTUFF_BITS(resp,  72,  8);
			card->cid.prod_name[4]	= UNSTUFF_BITS(resp,  64,  8);
			card->cid.prod_name[5]	= UNSTUFF_BITS(resp,  56,  8);
			card->cid.prod_name[6]	= UNSTUFF_BITS(resp,  48,  8);
			card->cid.hwrev		= UNSTUFF_BITS(resp,  44,  4);
			card->cid.fwrev		= UNSTUFF_BITS(resp,  40,  4);
			card->cid.serial	= UNSTUFF_BITS(resp,  16, 24);
			card->cid.month		= UNSTUFF_BITS(resp,  12,  4);
			card->cid.year		= UNSTUFF_BITS(resp,   8,  4) + 1997;
			break;

		case MMC_CSDV_2: /* MMC v2.x ? */
		case MMC_CSDV_3: /* MMC v3.x ? */
			card->cid.manfid	= UNSTUFF_BITS(resp, 120,  8);
			card->cid.oemid		= UNSTUFF_BITS(resp, 104, 16);
			card->cid.prod_name[0]	= UNSTUFF_BITS(resp,  96,  8);
			card->cid.prod_name[1]	= UNSTUFF_BITS(resp,  88,  8);
			card->cid.prod_name[2]	= UNSTUFF_BITS(resp,  80,  8);
			card->cid.prod_name[3]	= UNSTUFF_BITS(resp,  72,  8);
			card->cid.prod_name[4]	= UNSTUFF_BITS(resp,  64,  8);
			card->cid.prod_name[5]	= UNSTUFF_BITS(resp,  56,  8);
			card->cid.serial	= UNSTUFF_BITS(resp,  16, 32);
			card->cid.month		= UNSTUFF_BITS(resp,  12,  4);
			card->cid.year		= UNSTUFF_BITS(resp,   8,  4) + 1997;
			break;

		default:
			printk("%s: card has unknown MMC-MMCA version %d\n", card->host->host_name, card->csd.spec_vers);
			mmc_card_set_bad(card);
			break;
		}
	}
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static void mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m;
	u32 *resp = card->raw_csd;

	if(card->sd) {
		/*
		* We only understand SD-CSD structure v1.0.
		*/
		csd->sd=1;
		csd->csd_vers = UNSTUFF_BITS(resp, 126, 2);
		if(csd->csd_vers != SD_CSDV_1) {
			printk("%s: unrecognised SD-CSD structure version %d\n",
				card->host->host_name, csd->csd_vers);
			mmc_card_set_bad(card);
			return;
		}

		csd->spec_vers 		= 0;
		csd->taac		= UNSTUFF_BITS(resp, 112, 8);
		csd->nsac		= UNSTUFF_BITS(resp, 104, 8);
		csd->tran_speed		= UNSTUFF_BITS(resp,  96, 8);
		csd->ccc		= UNSTUFF_BITS(resp,  84,12);
		csd->read_bl_len	= UNSTUFF_BITS(resp,  80, 4);
		csd->read_bl_partial	= UNSTUFF_BITS(resp,  79, 1);
		csd->write_blk_misalign	= UNSTUFF_BITS(resp,  78, 1);
		csd->read_blk_misalign	= UNSTUFF_BITS(resp,  77, 1);
		csd->dsr_imp		= UNSTUFF_BITS(resp,  76, 1);
		csd->c_size		= UNSTUFF_BITS(resp,  62,12);
		csd->vdd_r_curr_min	= UNSTUFF_BITS(resp,  69, 3);
		csd->vdd_r_curr_max	= UNSTUFF_BITS(resp,  56, 3);
		csd->vdd_w_curr_min	= UNSTUFF_BITS(resp,  53, 3);
		csd->vdd_w_curr_max	= UNSTUFF_BITS(resp,  50, 3);
		csd->c_size_mult	= UNSTUFF_BITS(resp,  47, 3);
		csd->erase.sd10.erase_blk_en	= UNSTUFF_BITS(resp,  46, 1);
		csd->erase.sd10.erase_blk_size= UNSTUFF_BITS(resp,  39, 7);
		csd->wp_grp_size	= UNSTUFF_BITS(resp,  32, 7);
		csd->wp_grp_enable	= UNSTUFF_BITS(resp,  31, 1);
		csd->r2w_factor		= UNSTUFF_BITS(resp,  26, 3);
		csd->write_bl_len	= UNSTUFF_BITS(resp,  22, 4);
		csd->write_bl_partial	= UNSTUFF_BITS(resp,  21, 1);
		csd->file_format_grp	= UNSTUFF_BITS(resp,  15, 1);
		csd->copy		= UNSTUFF_BITS(resp,  14, 1);
		csd->perm_write_protect	= UNSTUFF_BITS(resp,  13, 1);
		csd->tmp_write_protect	= UNSTUFF_BITS(resp,  12, 1);
		csd->file_format	= UNSTUFF_BITS(resp,  10, 1);
		csd->ecc		= UNSTUFF_BITS(resp,   1, 7);

		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);
	} else {
		/*
		* We only understand MMC-CSD structure v1.1 and v2.
		* v2 has extra information in bits 15, 11 and 10.
		*/
		csd->sd=0;
		csd->csd_vers = UNSTUFF_BITS(resp, 126, 2);
		if (csd->csd_vers != MMC_CSDV_14 && csd->csd_vers != MMC_CSDV_2) {
			printk("%s: unrecognised MMC-CSD structure version %d\n",
				card->host->host_name, csd->csd_vers);
			mmc_card_set_bad(card);
			return;
		}

		csd->spec_vers	 = UNSTUFF_BITS(resp, 122, 4);
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->ccc	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_bl_len = UNSTUFF_BITS(resp, 80, 4);
	}
}

/*
 * Locate a MMC card on this MMC host given a raw CID.
 */
static struct mmc_card *mmc_find_card(struct mmc_host *host, u32 *raw_cid)
{
	struct mmc_card *card;

	list_for_each_entry(card, &host->cards, node) {
		if (memcmp(card->raw_cid, raw_cid, sizeof(card->raw_cid)) == 0)
			return card;
	}
	return NULL;
}

/*
 * Allocate a new MMC card, and assign a unique RCA.
 */
static struct mmc_card *
mmc_alloc_card(struct mmc_host *host, u32 *raw_cid, u8 sd, unsigned int *frca)
{
	struct mmc_card *card, *c;
	unsigned int rca = *frca;

	card = kmalloc(sizeof(struct mmc_card), GFP_KERNEL);
	if (!card)
		return ERR_PTR(-ENOMEM);

	mmc_init_card(card, host);
	memcpy(card->raw_cid, raw_cid, sizeof(card->raw_cid));
	card->sd = sd;

 again:
	list_for_each_entry(c, &host->cards, node)
		if (c->rca == rca) {
			rca++;
			goto again;
		}

	card->rca = rca;

	*frca = rca;

	return card;
}

/*
 * Tell attached cards to go to IDLE state
 */
static void mmc_idle_cards(struct mmc_host *host)
{
	struct mmc_command cmd;
	struct mmc_card *card;

	cmd.opcode = MMC_GO_IDLE_STATE;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_NONE;

	mmc_wait_for_cmd(host, &cmd, 0);

	//MMC_GO_IDLE_STATE resets all cards to bus width 1
	list_for_each_entry(card, &host->cards, node) {
		card->bus_width=1;
	}

	mmc_delay(100);
}

int sd_set_bus_width(struct mmc_host *host, u16 rca, u32 bus_width)
{
	struct mmc_command cmd;
	int err;

	cmd.opcode = MMC_ACMD_SD_SET_BUS_WIDTH;
	cmd.flags  = MMC_RSP_R1;

	switch(bus_width) {
		case 1:	 cmd.arg = 00; break;
		case 4:	 cmd.arg = 10; break;
		default: return -EINVAL;
	}

	err = mmc_wait_for_acmd(host, rca, &cmd, 3);

	if(err == MMC_ERR_TIMEOUT) {
		printk("MMC: sd_set_bus_width timed out.\n");
	} else if(err == MMC_ERR_BADCRC) {
		printk("MMC: sd_set_bus_width yielded crc error.\n");
	} else {
		printk("MMC: sd_app_op_cond done.\n");
	}

	return err;
}

EXPORT_SYMBOL(sd_set_bus_width);

static int sd_app_op_cond(struct mmc_host *host, u16 rca, u32 parameter, u32 *response)
{
	struct mmc_command cmd;
	int err;
	int retries = 100; //some cards take longer	-jassi

	cmd.opcode = MMC_ACMD_SD_APP_OP_COND;
	cmd.arg = parameter;
	cmd.flags = MMC_RSP_SHORT;

	printk("MMC: sd_app_op_cond to %08x\n",parameter);

	while(retries--) {
		err = mmc_wait_for_acmd(host, 0, &cmd, 0);
		if(0 == (cmd.resp[0] & MMC_CARD_BUSY)) {
			err=MMC_ERR_BUSY;
//			printk("MMC: sd_app_op_cond: at least one card is busy - trying again.\n");
			mmc_delay(10);
			continue;
		}

		if(err == MMC_ERR_NONE) {
			if(response)*response = cmd.resp[0];
			break;
		}
	}

	if(err == MMC_ERR_TIMEOUT) {
		printk("MMC: sd_app_op_cond timed out. Probably no SD-Card here.\n");
	} else if(err == MMC_ERR_BUSY) {
		printk("MMC: sd_app_op_cond locked busy. Probably have broken SD-Card.\n");
	} else {
		printk("MMC: sd_app_op_cond done. Results are: 0x%08x.\n",cmd.resp[0]);
	}

	return err;
}


/*
 * Apply power to the MMC stack.
 */
static void mmc_power_up(struct mmc_host *host)
{
	int bit = fls(host->ocr_avail) - 1;

	host->ios.vdd = bit;
	host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	host->ios.power_mode = MMC_POWER_UP;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(1);

	host->ios.clock = host->f_min;
	host->ios.power_mode = MMC_POWER_ON;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(2);
}

static void mmc_power_off(struct mmc_host *host)
{
	host->ios.clock = 0;
	host->ios.vdd = 0;
	host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	host->ios.power_mode = MMC_POWER_OFF;
	host->ops->set_ios(host, &host->ios);
}

static int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
	struct mmc_command cmd;
	int i, err = 0;

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_R3;

	for (i = 100; i; i--) {
		err = mmc_wait_for_cmd(host, &cmd, 0);
		if (err != MMC_ERR_NONE)
			break;

		if (cmd.resp[0] & MMC_CARD_BUSY || ocr == 0)
			break;

		err = MMC_ERR_TIMEOUT;

		mmc_delay(10);
	}

	if (rocr)
		*rocr = cmd.resp[0];

	return err;
}

/*
 * Discover cards by requesting their CID.  If this command
 * times out, it is not an error; there are no further cards
 * to be discovered.  Add new cards to the list.
 *
 * Create a mmc_card entry for each discovered card, assigning
 * it an RCA, and save the raw CID for decoding later.
 */
static void mmc_discover_cards(struct mmc_host *host, u8 sd)
{
	struct mmc_card *card;
	unsigned int first_rca = 1, err;

	while (1) {
		struct mmc_command cmd;

		cmd.opcode = MMC_ALL_SEND_CID;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_LONG; //HACK! CRC currently not implemented

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err == MMC_ERR_TIMEOUT) {
			err = MMC_ERR_NONE;
			break;
		}
		if (err != MMC_ERR_NONE) {
			printk(KERN_ERR "%s: error requesting CID: %d\n", host->host_name, err);
			break;
		}

		card = mmc_find_card(host, cmd.resp);
		if (!card) {
			card = mmc_alloc_card(host, cmd.resp, sd, &first_rca);
			if (IS_ERR(card)) {
				err = PTR_ERR(card);
				break;
			}
			list_add(&card->node, &host->cards);
		}

		card->state &= ~MMC_STATE_DEAD;

		cmd.opcode = MMC_SET_RELATIVE_ADDR;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err != MMC_ERR_NONE)
			mmc_card_set_dead(card);

		//SD-Cards choose their adresses themselfes (yuck!)
		if(card->sd) card->rca = (cmd.resp[0] >> 16);
	}
}

static void mmc_read_csds(struct mmc_host *host)
{
	struct mmc_card *card;

	list_for_each_entry(card, &host->cards, node) {
		struct mmc_command cmd;
		int err;

		if (card->state & (MMC_STATE_DEAD|MMC_STATE_PRESENT))
			continue;

		cmd.opcode = MMC_SEND_CSD;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_LONG; 	//HACK: No CRC check as s3c-Core is broken

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

		memcpy(card->raw_csd, cmd.resp, sizeof(card->raw_csd));

		mmc_decode_csd(card);
		mmc_decode_cid(card);
	}
}

static unsigned int mmc_calculate_clock(struct mmc_host *host)
{
	struct mmc_card *card;
	unsigned int max_dtr = host->f_max;

	list_for_each_entry(card, &host->cards, node)
		if (!mmc_card_dead(card) && max_dtr > card->csd.max_dtr)
			max_dtr = card->csd.max_dtr;

	DBG("MMC: selected %d.%03dMHz transfer rate\n",
	    max_dtr / 1000000, (max_dtr / 1000) % 1000);

	return max_dtr;
}

/*
 * Check whether cards we already know about are still present.
 * We do this by requesting status, and checking whether a card
 * responds.
 *
 * A request for status does not cause a state change in data
 * transfer mode.
 */
static void mmc_check_cards(struct mmc_host *host)
{
	struct list_head *l, *n;

	mmc_deselect_cards(host);

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);
		struct mmc_command cmd;
		int err;

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err == MMC_ERR_NONE)
			continue;

		mmc_card_set_dead(card);
	}
}


static void mmc_setup(struct mmc_host *host)
{
	mmc_power_up(host);
	mmc_idle_cards(host);

	// Wake and discover SD-Cards
	sd_app_op_cond(host, 0, host->ocr_avail, NULL);
	mmc_discover_cards(host, 1);

	// Wake and discover MMC-Cards
	mmc_send_op_cond(host, host->ocr_avail, NULL);
	mmc_discover_cards(host, 0);

	/*
	 * switch to push-pull mode.
	 */
	host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	host->ops->set_ios(host, &host->ios);

	mmc_read_csds(host);
}


/**
 *	mmc_detect_change - process change of state on a MMC socket
 *	@host: host which changed state.
 *
 *	All we know is that card(s) have been inserted or removed
 *	from the socket(s).  We don't know which socket or cards.
 */
void mmc_detect_change(struct mmc_host *host)
{
	schedule_work(&host->detect);
}

EXPORT_SYMBOL(mmc_detect_change);


static void mmc_rescan(void *data)
{
	struct mmc_host *host = data;
	struct list_head *l, *n;

	mmc_claim_host(host);

	if (host->ios.power_mode == MMC_POWER_ON)
		mmc_check_cards(host);

	mmc_setup(host);

	if (!list_empty(&host->cards)) {
		/*
		 * (Re-)calculate the fastest clock rate which the
		 * attached cards and the host support.
		 */
		host->ios.clock = mmc_calculate_clock(host);
		host->ops->set_ios(host, &host->ios);
	}

	mmc_release_host(host);

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);

		/*
		 * If this is a new and good card, register it.
		 */
		if (!mmc_card_present(card) && !mmc_card_dead(card)) {
			if (mmc_register_card(card))
				mmc_card_set_dead(card);
			else
				mmc_card_set_present(card);
		}

		/*
		 * If this card is dead, destroy it.
		 */
		if (mmc_card_dead(card)) {
			list_del(&card->node);
			mmc_remove_card(card);
		}
	}

	/*
	 * If we discover that there are no cards on the
	 * bus, turn off the clock and power down.
	 */
	if (list_empty(&host->cards))
		mmc_power_off(host);
}


/**
 *	mmc_alloc_host - initialise the per-host structure.
 *	@extra: sizeof private data structure
 *	@dev: pointer to host device model structure
 *
 *	Initialise the per-host structure.
 */
struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
{
	struct mmc_host *host;

	host = kmalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
	if (host) {
		memset(host, 0, sizeof(struct mmc_host) + extra);

		spin_lock_init(&host->lock);
		init_waitqueue_head(&host->wq);
		INIT_LIST_HEAD(&host->cards);
		INIT_WORK(&host->detect, mmc_rescan, host);

		host->dev = dev;

		/*
		 * By default, hosts do not support SGIO or large requests.
		 * They have to set these according to their abilities.
		 */
		host->max_hw_segs = 1;
		host->max_phys_segs = 1;
		host->max_sectors = 1 << (PAGE_CACHE_SHIFT - 9);
		host->max_seg_size = PAGE_CACHE_SIZE;
	}

	return host;
}

EXPORT_SYMBOL(mmc_alloc_host);

/**
 *	mmc_add_host - initialise host hardware
 *	@host: mmc host
 */
int mmc_add_host(struct mmc_host *host)
{
	static unsigned int host_num;

	snprintf(host->host_name, sizeof(host->host_name),
		 "mmc%d", host_num++);

	mmc_power_off(host);
	mmc_detect_change(host);

	return 0;
}

EXPORT_SYMBOL(mmc_add_host);

/**
 *	mmc_remove_host - remove host hardware
 *	@host: mmc host
 *
 *	Unregister and remove all cards associated with this host,
 *	and power down the MMC bus.
 */
void mmc_remove_host(struct mmc_host *host)
{
	struct list_head *l, *n;

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);

		mmc_remove_card(card);
	}

	mmc_power_off(host);
}

EXPORT_SYMBOL(mmc_remove_host);

/**
 *	mmc_free_host - free the host structure
 *	@host: mmc host
 *
 *	Free the host once all references to it have been dropped.
 */
void mmc_free_host(struct mmc_host *host)
{
	flush_scheduled_work();
	kfree(host);
}

EXPORT_SYMBOL(mmc_free_host);

#ifdef CONFIG_PM

/**
 *	mmc_suspend_host - suspend a host
 *	@host: mmc host
 *	@state: suspend mode (PM_SUSPEND_xxx)
 */
int mmc_suspend_host(struct mmc_host *host, u32 state)
{
	mmc_claim_host(host);
	mmc_deselect_cards(host);
	mmc_power_off(host);
	mmc_release_host(host);

	return 0;
}

EXPORT_SYMBOL(mmc_suspend_host);

/**
 *	mmc_resume_host - resume a previously suspended host
 *	@host: mmc host
 */
int mmc_resume_host(struct mmc_host *host)
{
	mmc_detect_change(host);

	return 0;
}

EXPORT_SYMBOL(mmc_resume_host);

#endif

MODULE_LICENSE("GPL");
