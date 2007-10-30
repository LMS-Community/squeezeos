/*
 *  linux/drivers/mmc/s3cmci.h - Samsung S3C MCI driver
 *
 *  Copyright (C) 2004-2006 Thomas Kleffel, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//FIXME: DMA Resource management ?!
#define S3CMCI_DMA 0

enum s3cmci_waitfor {
	COMPLETION_NONE,
	COMPLETION_FINALIZE,
	COMPLETION_CMDSENT,
	COMPLETION_RSPFIN,
	COMPLETION_XFERFINISH,
	COMPLETION_XFERFINISH_RSPFIN,
};

struct s3cmci_host {
	struct platform_device	*pdev;
	struct s3c24xx_mci_pdata *pdata;
	struct mmc_host		*mmc;
	struct resource		*mem;
	struct clk		*clk;
	void __iomem		*base;
	int			irq;
	int			irq_cd;
	int			dma;

	unsigned long		clk_rate;
	unsigned long		clk_div;
	unsigned long		real_rate;
	u8			prescaler;

	int			is2440;
	unsigned		sdiimsk;
	unsigned		sdidata;
	int			dodma;

	volatile int		dmatogo;

	struct mmc_request	*mrq;
	int			cmd_is_stop;

	spinlock_t		complete_lock;
	volatile enum s3cmci_waitfor
				complete_what;

	volatile int		dma_complete;

	volatile u32		pio_sgptr;
	volatile u32		pio_words;
	volatile u32		pio_count;
	volatile u32		*pio_ptr;
#define XFER_NONE 0
#define XFER_READ 1
#define XFER_WRITE 2
	volatile u32		pio_active;

	int			bus_width;

	char 			dbgmsg_cmd[301];
	char 			dbgmsg_dat[301];
	volatile char		*status;

	unsigned int		ccnt, dcnt;
	struct tasklet_struct	pio_tasklet;
};
