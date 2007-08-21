/*
 *  linux/include/linux/mmc/card.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 */
#ifndef LINUX_MMC_CARD_H
#define LINUX_MMC_CARD_H

#include <linux/mmc/mmc.h>

struct mmc_cid {
	unsigned int		manfid;
	char			prod_name[8];
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
};

struct mmc_csd {
	u8  sd;
	u8  csd_vers;
	u8  spec_vers;
	u8  taac;
	u8  nsac;
	u16 ccc;
	u8  tran_speed;
	u8  read_bl_len;
	u8  read_bl_partial;
	u8  write_blk_misalign;
	u8  read_blk_misalign;
	u8  dsr_imp;
	u16 c_size;
	u8  vdd_r_curr_min;
	u8  vdd_r_curr_max;
	u8  vdd_w_curr_min;
	u8  vdd_w_curr_max;
	u8  c_size_mult;
	union {
		struct { /* MMC system specification version 3.1 */
			u8  erase_grp_size;
			u8  erase_grp_mult;
		} mmc31;
		struct { /* MMC system specification version 2.2 */
			u8  sector_size;
			u8  erase_grp_size;
		} mmc22;
		struct { /* SD Version 1.0 */
			u8  erase_blk_en;
			u8  erase_blk_size;
		} sd10;
	} erase;

	u8  wp_grp_size;
	u8  wp_grp_enable;
	u8  default_ecc;
	u8  r2w_factor;
	u8  write_bl_len;
	u8  write_bl_partial;
	u8  file_format_grp;
	u8  copy;
	u8  perm_write_protect;
	u8  tmp_write_protect;
	u8  file_format;
	u8  ecc;

	unsigned short		tacc_clks;
	unsigned int		tacc_ns;
	unsigned int		max_dtr;
	unsigned int		capacity;
};

struct mmc_host;

/*
 * MMC device
 */
struct mmc_card {
	struct list_head	node;		/* node in hosts devices list */
	struct mmc_host		*host;		/* the host this device belongs to */
	struct device		dev;		/* the device */
	unsigned char		sd;
	unsigned int		rca;		/* relative card address of device */
	unsigned int		state;		/* (our) card state */
	u8			bus_width;	/* bus_width the card is set to */
#define MMC_STATE_PRESENT	(1<<0)		/* present in sysfs */
#define MMC_STATE_DEAD		(1<<1)		/* device no longer in stack */
#define MMC_STATE_BAD		(1<<2)		/* unrecognised device */
	u32			raw_cid[4];	/* raw card CID */
	u32			raw_csd[4];	/* raw card CSD */
	struct mmc_cid		cid;		/* card identification */
	struct mmc_csd		csd;		/* card specific */
};

#define mmc_card_present(c)	((c)->state & MMC_STATE_PRESENT)
#define mmc_card_dead(c)	((c)->state & MMC_STATE_DEAD)
#define mmc_card_bad(c)		((c)->state & MMC_STATE_BAD)

#define mmc_card_set_present(c)	((c)->state |= MMC_STATE_PRESENT)
#define mmc_card_set_dead(c)	((c)->state |= MMC_STATE_DEAD)
#define mmc_card_set_bad(c)	((c)->state |= MMC_STATE_BAD)

#define mmc_card_name(c)	((c)->cid.prod_name)
#define mmc_card_id(c)		((c)->dev.bus_id)

#define mmc_list_to_card(l)	container_of(l, struct mmc_card, node)
#define mmc_get_drvdata(c)	dev_get_drvdata(&(c)->dev)
#define mmc_set_drvdata(c,d)	dev_set_drvdata(&(c)->dev, d)

/*
 * MMC device driver (e.g., Flash card, I/O card...)
 */
struct mmc_driver {
	struct device_driver drv;
	int (*probe)(struct mmc_card *);
	void (*remove)(struct mmc_card *);
	int (*suspend)(struct mmc_card *, u32);
	int (*resume)(struct mmc_card *);
};

extern int mmc_register_driver(struct mmc_driver *);
extern void mmc_unregister_driver(struct mmc_driver *);

static inline int mmc_card_claim_host(struct mmc_card *card)
{
	return __mmc_claim_host(card->host, card);
}

#define mmc_card_release_host(c)	mmc_release_host((c)->host)

#endif
