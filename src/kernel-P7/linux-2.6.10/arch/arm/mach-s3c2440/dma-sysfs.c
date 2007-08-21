/* linux/arch/arm/mach-bast/dma-sysfs.c
 *
 * (c) 2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2440 DMA core sysfs interface
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sysdev.h>
#include <linux/config.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>

#include <asm/mach/dma.h>
#include <asm/arch/map.h>


extern struct sysdev_class dma_sysclass;
extern s3c2440_dma_chan_t s3c2440_chans[S3C2440_DMA_CHANNELS];

#define dma_rdreg(chan, reg) readl((chan)->regs + (reg))


static const char *s3c2440_dma_decode_state(s3c2440_dma_state_t state)
{
	switch (state) {
	case S3C2440_DMA_IDLE:
		return "Idle";
	case S3C2440_DMA_RUNNING:
		return "Running";
	case S3C2440_DMA_PAUSED:
		return "Paused";
	default:
		return "Invalid";
	}

	return NULL;
}

static const char *s3c2440_dma_decode_loadstate(s3c2440_dma_loadst_t state)
{
	switch (state) {
	case S3C2440_DMALOAD_NONE:
		return "Nothing Loaded";
	case S3C2440_DMALOAD_1LOADED:
		return "1 Loaded";
	case S3C2440_DMALOAD_1RUNNING:
		return "1 Running";
	case S3C2440_DMALOAD_1LOADED_1RUNNING:
		return "1 Loaded, 1 Running";
	default:
		return "Invalid Load State";
	}

	return NULL;
}

static const char *s3c2440_dma_decode_source(s3c2440_dmasrc_t source)
{
	switch (source) {
	case S3C2440_DMASRC_HW:
		return "Hardware";
	case S3C2440_DMASRC_MEM:
		return "Memory";
	default:
		return "Unknown Source";
	}

	return NULL;
}


struct outbuff {
	char		*buff;
	int		 offset;
	int		 size;
};

static inline void outbuf_init(struct outbuff *ob, char *buf, int size)
{
	ob->buff   = buf;
	ob->offset = 0;
	ob->size   = size;
}

static void outbuf_pf(struct outbuff *ob, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);

	ob->offset += vsnprintf(ob->buff + ob->offset,
				ob->size - ob->offset,
				fmt, va);
	va_end(va);
}

static void dma_sysfs_showregs(s3c2440_dma_chan_t *chan, struct outbuff *buf)
{
	outbuf_pf(buf, "Hardware Registers:\n\n");

	outbuf_pf(buf, "  DISRC \t\t\t%08lx\n",  readl(chan->regs + 0x00));
	outbuf_pf(buf, "  DISRCC\t\t\t%08lx\n", readl(chan->regs + 0x04));
	outbuf_pf(buf, "  DIDST \t\t\t%08lx\n",  readl(chan->regs + 0x08));
	outbuf_pf(buf, "  DIDSTC\t\t\t%08lx\n", readl(chan->regs + 0x0C));
	outbuf_pf(buf, "  DCON  \t\t\t%08lx\n", readl(chan->regs + 0x10));
	outbuf_pf(buf, "  DSTAT \t\t\t%08lx\n", readl(chan->regs + 0x14));
	outbuf_pf(buf, "  DCSRC \t\t\t%08lx\n", readl(chan->regs + 0x18));
	outbuf_pf(buf, "  DCDST \t\t\t%08lx\n", readl(chan->regs + 0x1c));
	outbuf_pf(buf, "  DMTRG \t\t\t%08lx\n", readl(chan->regs + 0x20));
}

static void dma_sysfs_showbuf(s3c2440_dma_chan_t *chan,
			      struct outbuff *buf,
			      s3c2440_dma_buf_t *ptr)
{
	if (ptr == NULL)
		return;

	outbuf_pf(buf, "  buff %p: data=%08x, size=%08x, id=%08x\n",
		  ptr, ptr->data, ptr->size, ptr->id);
}

static void dma_sysfs_showbuffers(s3c2440_dma_chan_t *chan, struct outbuff *buf)
{
	s3c2440_dma_buf_t *ptr = chan->next;
	unsigned long flags;

	local_irq_save(flags);

	outbuf_pf(buf, "Current Buffer:\n");
	dma_sysfs_showbuf(chan, buf, chan->curr);

	outbuf_pf(buf, "\nBuffer Queue: %p -> %p\n", chan->next, chan->end);

	for (; ptr != NULL; ptr = ptr->next)
		dma_sysfs_showbuf(chan, buf, ptr);

	local_irq_restore(flags);
}

static void dma_sysfs_showstate(s3c2440_dma_chan_t *chan, struct outbuff *buf)
{
	outbuf_pf(buf, "IRQ:\t\t\t\t%d\n", chan->irq);
	outbuf_pf(buf, "Register Base:\t\t\t0x%08x\n", chan->regs);
	outbuf_pf(buf, "Address Register:\t\t0x%08lx\n", chan->addr_reg);
	outbuf_pf(buf, "Hardware Register:\t\t0x%08lx\n", chan->dev_addr);
	outbuf_pf(buf, "Load Timeout:\t\t\t0x%08x\n", chan->load_timeout);
	outbuf_pf(buf, "Data Source:\t\t\t%s\n", 
		     s3c2440_dma_decode_source(chan->source));

	outbuf_pf(buf, "Channel State:\t\t\t%s (%d)\n",
		     s3c2440_dma_decode_state(chan->state), chan->state);
	outbuf_pf(buf, "Load State:\t\t\t%s (%d)\n", 
		     s3c2440_dma_decode_loadstate(chan->load_state), 
		     chan->load_state);

	if (chan->client != NULL) {
		outbuf_pf(buf, "Claimed by:\t\t\t%s\n",
			     chan->client->name);
	} else {
		outbuf_pf(buf, "Claimed by:\t\t\tNo-one\n");
	}
}

static inline s3c2440_dma_chan_t *dev_to_chan(struct sys_device *dev)
{
	return &s3c2440_chans[dev->id];
}

static ssize_t dma_sysfs_show_regs(struct sys_device *dev, char *buf)
{
	s3c2440_dma_chan_t *cp = dev_to_chan(dev);
	struct outbuff buff;
	
	outbuf_init(&buff, buf, PAGE_SIZE);
	dma_sysfs_showregs(cp, &buff);	

	return buff.offset;
}

static ssize_t dma_sysfs_show_state(struct sys_device *dev, char *buf)
{
	s3c2440_dma_chan_t *cp = dev_to_chan(dev);
	struct outbuff buff;
	
	outbuf_init(&buff, buf, PAGE_SIZE);
	dma_sysfs_showstate(cp, &buff);

	return buff.offset;
}

static ssize_t dma_sysfs_show_buffers(struct sys_device *dev, char *buf)
{
	s3c2440_dma_chan_t *cp = dev_to_chan(dev);
	struct outbuff buff;
	
	outbuf_init(&buff, buf, PAGE_SIZE);
	dma_sysfs_showbuffers(cp, &buff);

	return buff.offset;
}

static ssize_t dma_sysfs_show_all(struct sys_device *dev, char *buf)
{
	s3c2440_dma_chan_t *cp = dev_to_chan(dev);
	struct outbuff buff;

	outbuf_init(&buff, buf, PAGE_SIZE);

	dma_sysfs_showstate(cp, &buff);
	dma_sysfs_showregs(cp, &buff);	
	dma_sysfs_showbuffers(cp, &buff);

	return buff.offset;
}

static SYSDEV_ATTR(all, S_IRUGO, dma_sysfs_show_all, NULL);
static SYSDEV_ATTR(regs, S_IRUGO, dma_sysfs_show_regs, NULL);
static SYSDEV_ATTR(state, S_IRUGO, dma_sysfs_show_state, NULL);
static SYSDEV_ATTR(buffers, S_IRUGO, dma_sysfs_show_buffers, NULL);

static struct sysdev_attribute *attrs[] = {
	&attr_all,
	&attr_regs,
	&attr_state,
	&attr_buffers,
};

static int dma_sysfs_attach(struct sys_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		sysdev_create_file(dev, attrs[i]);
	
	return 0;
}

static int dma_sysfs_remove(struct sys_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		sysdev_create_file(dev, attrs[i]);
	
	return 0;
}

static struct sysdev_driver dma_sysfs_driver = {
	.add	= dma_sysfs_attach,
	.remove	= dma_sysfs_remove,
};

static int __init s3c2440_init_dma_sysfs(void)
{
	printk("S3C24XX DMA sysfs support, (c) 2005 Simtec Electronics\n");

	return sysdev_driver_register(&dma_sysclass, &dma_sysfs_driver);
}

__initcall(s3c2440_init_dma_sysfs);
