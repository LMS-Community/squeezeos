/*
 *  linux/drivers/mmc/s3c2413mci.h - Samsung S3C2413 SDI Interface driver
 *
 *  Copyright (C) 2004 Thomas Kleffel, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct clk;

//FIXME: DMA Resource management ?!
#define S3C2413SDI_DMA 0

enum s3c2413sdi_waitfor {
	COMPLETION_NONE,
	COMPLETION_CMDSENT,
	COMPLETION_RSPFIN,
	COMPLETION_XFERFINISH,
	COMPLETION_XFERFINISH_RSPFIN,
};

struct s3c2413sdi_host {
	struct mmc_host		*mmc;

	struct resource		*mem;
	struct clk		*clk;
	void __iomem		*base;
	int			irq;
	int			irq_cd;
	int			dma;
#ifdef S3C2413SDI_DMA_BACKBUF
	dma_addr_t		 dmabuf_phys;
	void			*dmabuf_log;
	unsigned int		 dmabuf_size;
#endif
	
	struct mmc_request	*mrq;

	spinlock_t		complete_lock;
	struct completion	complete_request;
	struct completion	complete_dma;
	enum s3c2413sdi_waitfor	complete_what;
	struct timer_list       timer;
};
