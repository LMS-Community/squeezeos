/*
 * Common audio handling for the SA11x0 processor
 *
 * Copyright (C) 2000, 2001 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 *
 * This module handles the generic buffering/DMA/mmap audio interface for
 * codecs connected to the SA1100 chip.  All features depending on specific
 * hardware implementations like supported audio formats or samplerates are
 * relegated to separate specific modules.
 *
 *
 * History:
 *
 * 2000-05-21	Nicolas Pitre	Initial release.
 *
 * 2000-06-10	Erik Bunce	Add initial poll support.
 *
 * 2000-08-22	Nicolas Pitre	Removed all DMA stuff. Now using the
 * 				generic SA1100 DMA interface.
 *
 * 2000-11-30	Nicolas Pitre	- Validation of opened instances;
 * 				- Power handling at open/release time instead
 * 				  of driver load/unload;
 *
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-uda1341.c driver.
 *
 * 2001-07-22	Nicolas Pitre	- added mmap() and realtime support
 * 				- corrected many details to better comply
 * 				  with the OSS API
 *
 * 2001-10-19	Nicolas Pitre	- brought DMA registration processing
 * 				  into this module for better ressource
 * 				  management.  This also fixes a bug
 * 				  with the suspend/resume logic.
 *
 * 2001-12-20	Nicolas Pitre	- moved DMA buffer handling out of the
 * 				  generic code (since only this module
 * 				  used it anyway) and reimplemented it
 * 				  locally for much better efficiency.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>

#include "sa1100-audio.h"


#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif


#define AUDIO_NAME		"sa1100-audio"
#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	8192


#define AUDIO_ACTIVE(state)	((state)->rd_ref || (state)->wr_ref)

#define SPIN_ADDR		(dma_addr_t)FLUSH_BASE_PHYS
#define SPIN_SIZE		2048


/*
 * DMA processing
 */

#ifndef CONFIG_SA1111

#define DMA_REQUEST(s, cb)	sa1100_request_dma(s->dma_dev, s->id, cb, s, &s->dma_regs)
#define DMA_FREE(s)		{sa1100_free_dma(s->dma_regs); s->dma_regs = 0;}
#define DMA_START(s, d, l)	sa1100_start_dma(s->dma_regs, d, l)
#define DMA_POS(s)		sa1100_get_dma_pos(s->dma_regs)
#define DMA_STOP(s)		sa1100_stop_dma(s->dma_regs)
#define DMA_CLEAR(s)		sa1100_clear_dma(s->dma_regs)
#define DMA_RESET(s)		sa1100_reset_dma(s->dma_regs)

#else

#include "sa1111-sac.h"

static int DMA_REQUEST(audio_stream_t *s, dma_callback_t cb)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		dma->callback = cb;
		dma->data = s;
		return 0;
	} else {
		return sa1100_request_dma(s->dma_dev, s->id, cb, s, &s->dma_regs);
	}
}

static void DMA_FREE(audio_stream_t *s)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		dma->reset(dma);
		return;
	} else {
		sa1100_free_dma(s->dma_regs);
		s->dma_regs = NULL;
	}
}

static int DMA_START(audio_stream_t *s, dma_addr_t data, int len)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		return dma->start(dma, data, len);
	} else {
		return sa1100_start_dma(s->dma_regs, data, len);
	}
}

static dma_addr_t DMA_POS(audio_stream_t *s)
{
	if (s->sa1111_dma) {
		/* the real thing is not implemented so... */
		return s->buffers[s->dma_tail].dma_addr;
	} else {
		return sa1100_get_dma_pos(s->dma_regs);
	}
}

static void DMA_STOP(audio_stream_t *s)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		dma->reset(dma);
		return;
	} else {
		sa1100_stop_dma(s->dma_regs);
	}
}

static void DMA_CLEAR(audio_stream_t *s)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		dma->reset(dma);
		return;
	} else {
		sa1100_clear_dma(s->dma_regs);
	}
}

static void DMA_RESET(audio_stream_t *s)
{
	if (s->sa1111_dma) {
		sa1111_dma_t *dma = (sa1111_dma_t *)s->dma_regs;
		dma->reset(dma);
		return;
	} else {
		sa1100_reset_dma(s->dma_regs);
	}
}

#endif


static u_int audio_get_dma_pos(audio_stream_t *s)
{
	audio_buf_t *b = &s->buffers[s->dma_tail];
	u_int offset;

	if (b->dma_ref) {
		offset = DMA_POS(s) - b->dma_addr;
		if (offset >= s->fragsize)
			offset = s->fragsize - 4;
	} else if (s->pending_frags) {
		offset = b->offset;
	} else {
		offset = 0;
	}
	return offset;
}


static void audio_stop_dma(audio_stream_t *s)
{
	u_int pos;
	unsigned long flags;
	audio_buf_t *b;

	if (s->dma_spinref > 0 || !s->buffers)
		return;

	local_irq_save(flags);
	s->stopped = 1;
	DMA_STOP(s);
	pos = audio_get_dma_pos(s);
	DMA_CLEAR(s);
	if (s->spin_idle) {
		DMA_START(s, SPIN_ADDR, SPIN_SIZE);
		DMA_START(s, SPIN_ADDR, SPIN_SIZE);
		s->dma_spinref = 2;
	} else
		s->dma_spinref = 0;
	local_irq_restore(flags);

	/* back up pointers to be ready to restart from the same spot */
	while (s->dma_head != s->dma_tail) {
		b = &s->buffers[s->dma_head];
		if (b->dma_ref) {
			b->dma_ref = 0;
			b->offset = 0;
		}
		s->pending_frags++;
		if (s->dma_head == 0)
			s->dma_head = s->nbfrags;
		s->dma_head--;
	}
	b = &s->buffers[s->dma_head];
	if (b->dma_ref) {
		b->offset = pos;
		b->dma_ref = 0;
	}
}


static void audio_reset(audio_stream_t *s)
{
	if (s->buffers) {
		audio_stop_dma(s);
		s->buffers[s->dma_head].offset = 0;
		s->buffers[s->usr_head].offset = 0;
		s->usr_head = s->dma_head;
		s->pending_frags = 0;
		sema_init(&s->sem, s->nbfrags);
	}
	s->active = 0;
	s->stopped = 0;
}


static void audio_process_dma(audio_stream_t *s)
{
	int ret;

	if (s->stopped)
		goto spin;

	if (s->dma_spinref > 0 && s->pending_frags) {
		s->dma_spinref = 0;
		DMA_CLEAR(s);
	}

	while (s->pending_frags) {
		audio_buf_t *b = &s->buffers[s->dma_head];
		u_int dma_size = s->fragsize - b->offset;
		if (dma_size > MAX_DMA_SIZE)
			dma_size = CUT_DMA_SIZE;
		ret = DMA_START(s, b->dma_addr + b->offset, dma_size);
		if (ret)
			return;
		b->dma_ref++;
		b->offset += dma_size;
		if (b->offset >= s->fragsize) {
			s->pending_frags--;
			if (++s->dma_head >= s->nbfrags)
				s->dma_head = 0;
		}
	}

spin:
	if (s->spin_idle) {
		int spincnt = 0;
		while (DMA_START(s, SPIN_ADDR, SPIN_SIZE) == 0)
			spincnt++;
		/*
		 * Note: if there is still a data buffer being
		 * processed then the ref count is negative.  This
		 * allows for the DMA termination to be accounted in
		 * the proper order.  Of course dma_spinref can't be
		 * greater than 0 if dma_ref is not 0 since we kill
		 * the spinning above as soon as there is real data to process.
		 */
		if (s->buffers && s->buffers[s->dma_tail].dma_ref)
			spincnt = -spincnt;
		s->dma_spinref += spincnt;
	}
}


static void audio_dma_callback(void *data)
{
	audio_stream_t *s = data;
	audio_buf_t *b = &s->buffers[s->dma_tail];

	if (s->dma_spinref > 0) {
		s->dma_spinref--;
	} else if (!s->buffers) {
		printk(KERN_CRIT "sa1100_audio: received DMA IRQ for non existent buffers!\n");
		return;
	} else if (b->dma_ref && --b->dma_ref == 0 && b->offset >= s->fragsize) {
		/* This fragment is done */
		b->offset = 0;
		s->bytecount += s->fragsize;
		s->fragcount++;
		s->dma_spinref = -s->dma_spinref;
		if (++s->dma_tail >= s->nbfrags)
			s->dma_tail = 0;
		if (!s->mapped)
			up(&s->sem);
		else
			s->pending_frags++;
		wake_up(&s->wq);
	}

	audio_process_dma(s);
}


/*
 * Buffer creation/destruction
 */

static void audio_discard_buf(audio_stream_t * s)
{
	DPRINTK("audio_discard_buf\n");

	/* ensure DMA isn't using those buffers */
	audio_reset(s);

	if (s->buffers) {
		int frag;
		for (frag = 0; frag < s->nbfrags; frag++) {
			if (!s->buffers[frag].master)
				continue;
			dma_free_coherent(s->dev,
					s->buffers[frag].master,
					s->buffers[frag].data,
					s->buffers[frag].dma_addr);
		}
		kfree(s->buffers);
		s->buffers = NULL;
	}
}


static int audio_setup_buf(audio_stream_t * s)
{
	int frag;
	int dmasize = 0;
	char *dmabuf = NULL;
	dma_addr_t dmaphys = 0;

	if (s->buffers)
		return -EBUSY;

	s->buffers = kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL);
	if (!s->buffers)
		goto err;
	memset(s->buffers, 0, sizeof(audio_buf_t) * s->nbfrags);

	for (frag = 0; frag < s->nbfrags; frag++) {
		audio_buf_t *b = &s->buffers[frag];

		/*
		 * Let's allocate non-cached memory for DMA buffers.
		 * We try to allocate all memory at once.
		 * If this fails (a common reason is memory fragmentation),
		 * then we allocate more smaller buffers.
		 */
		if (!dmasize) {
			dmasize = (s->nbfrags - frag) * s->fragsize;
			do {
			  	dmabuf = dma_alloc_coherent(s->dev, dmasize, &dmaphys, GFP_KERNEL);
				if (!dmabuf)
					dmasize -= s->fragsize;
			} while (!dmabuf && dmasize);
			if (!dmabuf)
				goto err;
			b->master = dmasize;
			memzero(dmabuf, dmasize);
		}

		b->data = dmabuf;
		b->dma_addr = dmaphys;
		DPRINTK("buf %d: start %p dma %#08x\n", frag, b->data,
			b->dma_addr);

		dmabuf += s->fragsize;
		dmaphys += s->fragsize;
		dmasize -= s->fragsize;
	}

	s->usr_head = s->dma_head = s->dma_tail = 0;
	s->bytecount = 0;
	s->fragcount = 0;
	sema_init(&s->sem, s->nbfrags);

	return 0;

err:
	printk(AUDIO_NAME ": unable to allocate audio memory\n ");
	audio_discard_buf(s);
	return -ENOMEM;
}


/*
 * Driver interface functions
 */

static int audio_write(struct file *file, const char *buffer,
		       size_t count, loff_t * ppos)
{
	const char *buffer0 = buffer;
	audio_state_t *state = file->private_data;
	audio_stream_t *s = state->output_stream;
	int chunksize, ret = 0;
	unsigned long flags;

	DPRINTK("audio_write: count=%d\n", count);

	if (s->mapped)
		return -ENXIO;
	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;

	while (count > 0) {
		audio_buf_t *b = &s->buffers[s->usr_head];

		/* Wait for a buffer to become free */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&s->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&s->sem))
				break;
		}

		/* Feed the current buffer */
		chunksize = s->fragsize - b->offset;
		if (chunksize > count)
			chunksize = count;
		DPRINTK("write %d to %d\n", chunksize, s->usr_head);
		if (copy_from_user(b->data + b->offset, buffer, chunksize)) {
			up(&s->sem);
			return -EFAULT;
		}
		buffer += chunksize;
		count -= chunksize;
		b->offset += chunksize;
		if (b->offset < s->fragsize) {
			up(&s->sem);
			break;
		}

		/* Update pointers and send current fragment to DMA */
		b->offset = 0;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		s->active = 1;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;
	DPRINTK("audio_write: return=%d\n", ret);
	return ret;
}


static void audio_prime_rx(audio_state_t *state)
{
	audio_stream_t *is = state->input_stream;
	unsigned long flags;

	local_irq_save(flags);
	if (state->need_tx_for_rx) {
		/*
		 * With some codecs like the Philips UDA1341 we must ensure
		 * there is an output stream at any time while recording since
		 * this is how the UDA1341 gets its clock from the SA1100.
		 * So while there is no playback data to send, the output DMA
		 * will spin with all zeroes.  We use the cache flush special
		 * area for that.
		 */
		state->output_stream->spin_idle = 1;
		audio_process_dma(state->output_stream);
	}
	is->pending_frags = is->nbfrags;
	sema_init(&is->sem, 0);
	is->active = 1;
	audio_process_dma(is);
	local_irq_restore(flags);
}


static int audio_read(struct file *file, char *buffer,
		      size_t count, loff_t * ppos)
{
	char *buffer0 = buffer;
	audio_state_t *state = file->private_data;
	audio_stream_t *s = state->input_stream;
	int chunksize, ret = 0;
	unsigned long flags;

	DPRINTK("audio_read: count=%d\n", count);

	if (s->mapped)
		return -ENXIO;

	if (!s->active) {
		if (!s->buffers && audio_setup_buf(s))
			return -ENOMEM;
		audio_prime_rx(state);
	}

	while (count > 0) {
		audio_buf_t *b = &s->buffers[s->usr_head];

		/* Wait for a buffer to become full */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&s->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&s->sem))
				break;
		}

		/* Grab data from the current buffer */
		chunksize = s->fragsize - b->offset;
		if (chunksize > count)
			chunksize = count;
		DPRINTK("read %d from %d\n", chunksize, s->usr_head);
		if (copy_to_user(buffer, b->data+ b->offset, chunksize)) {
			up(&s->sem);
			return -EFAULT;
		}
		buffer += chunksize;
		count -= chunksize;
		b->offset += chunksize;
		if (b->offset < s->fragsize) {
			up(&s->sem);
			break;
		}

		/* Update pointers and return current fragment to DMA */
		b->offset = 0;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;
	DPRINTK("audio_read: return=%d\n", ret);
	return ret;
}

static int audio_sync(struct file *file)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *s = state->output_stream;
	audio_buf_t *b;
	u_int shiftval = 0;
	unsigned long flags;
	DECLARE_WAITQUEUE(wait, current);

	DPRINTK("audio_sync\n");

	if (!(file->f_mode & FMODE_WRITE) || !s->buffers || s->mapped)
		return 0;

	/*
	 * Send current buffer if it contains data.  Be sure to send
	 * a full sample count.
	 */
	b = &s->buffers[s->usr_head];
	if (b->offset &= ~3) {
		down(&s->sem);
		/*
		 * HACK ALERT !
		 * To avoid increased complexity in the rest of the code
		 * where full fragment sizes are assumed, we cheat a little
		 * with the start pointer here and don't forget to restore
		 * it later.
		 */
		shiftval = s->fragsize - b->offset;
		b->offset = shiftval;
		b->dma_addr -= shiftval;
		s->bytecount -= shiftval;
		if (++s->usr_head >= s->nbfrags)
			s->usr_head = 0;
		local_irq_save(flags);
		s->pending_frags++;
		audio_process_dma(s);
		local_irq_restore(flags);
	}

	/* Let's wait for all buffers to complete */
	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&s->wq, &wait);
	while (s->pending_frags &&
	       s->dma_tail != s->usr_head &&
	       !signal_pending(current)) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&s->wq, &wait);

	/* undo the pointer hack above */
	if (shiftval) {
		local_irq_save(flags);
		b->dma_addr += shiftval;
		/* ensure sane DMA code behavior if not yet processed */
		if (b->offset != 0)
			b->offset = s->fragsize;
		local_irq_restore(flags);
	}

	return 0;
}

static int audio_mmap(struct file *file, struct vm_area_struct *vma)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *s;
	unsigned long size, vma_addr;
	int i, ret;

	if (vma->vm_pgoff != 0)
		return -EINVAL;

	if (vma->vm_flags & VM_WRITE) {
		if (!state->wr_ref)
			return -EINVAL;;
		s = state->output_stream;
	} else if (vma->vm_flags & VM_READ) {
		if (!state->rd_ref)
			return -EINVAL;
		s = state->input_stream;
	} else return -EINVAL;

	if (s->mapped)
		return -EINVAL;
	size = vma->vm_end - vma->vm_start;
	if (size != s->fragsize * s->nbfrags)
		return -EINVAL;
	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;
	vma_addr = vma->vm_start;
	for (i = 0; i < s->nbfrags; i++) {
		audio_buf_t *buf = &s->buffers[i];
		if (!buf->master)
			continue;
		ret = remap_pfn_range(vma, vma_addr, buf->dma_addr, buf->master, vma->vm_page_prot);
		if (ret)
			return ret;
		vma_addr += buf->master;
	}
	s->mapped = 1;

	return 0;
}

static unsigned int audio_poll(struct file *file,
			       struct poll_table_struct *wait)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *is = state->input_stream;
	audio_stream_t *os = state->output_stream;
	unsigned int mask = 0;

	DPRINTK("audio_poll(): mode=%s%s\n",
		(file->f_mode & FMODE_READ) ? "r" : "",
		(file->f_mode & FMODE_WRITE) ? "w" : "");

	if (file->f_mode & FMODE_READ) {
		/* Start audio input if not already active */
		if (!is->active) {
			if (!is->buffers && audio_setup_buf(is))
				return -ENOMEM;
			audio_prime_rx(state);
		}
		poll_wait(file, &is->wq, wait);
	}

	if (file->f_mode & FMODE_WRITE) {
		if (!os->buffers && audio_setup_buf(os))
			return -ENOMEM;
		poll_wait(file, &os->wq, wait);
	}

	if (file->f_mode & FMODE_READ)
		if (( is->mapped && is->bytecount > 0) ||
		    (!is->mapped && sema_count(&is->sem) > 0))
			mask |= POLLIN | POLLRDNORM;

	if (file->f_mode & FMODE_WRITE)
		if (( os->mapped && os->bytecount > 0) ||
		    (!os->mapped && sema_count(&os->sem) > 0))
			mask |= POLLOUT | POLLWRNORM;

	DPRINTK("audio_poll() returned mask of %s%s\n",
		(mask & POLLIN) ? "r" : "",
		(mask & POLLOUT) ? "w" : "");

	return mask;
}

static loff_t audio_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

static int audio_set_fragments(audio_stream_t *s, int val)
{
	if (s->active)
		return -EBUSY;
	if (s->buffers)
		audio_discard_buf(s);
	s->nbfrags = (val >> 16) & 0x7FFF;
	val &= 0xffff;
	if (val < 4)
		val = 4;
	if (s->sa1111_dma && val < 11)
		val = 11;
	if (val > 15)
		val = 15;
	s->fragsize = 1 << val;
	if (s->nbfrags < 2)
		s->nbfrags = 2;
	if (s->nbfrags * s->fragsize > 128 * 1024)
		s->nbfrags = 128 * 1024 / s->fragsize;
	if (audio_setup_buf(s))
		return -ENOMEM;
	return val|(s->nbfrags << 16);
}

static int audio_ioctl(struct inode *inode, struct file *file,
		       uint cmd, ulong arg)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	long val;

	/* dispatch based on command */
	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);

	case SNDCTL_DSP_GETBLKSIZE:
		if (file->f_mode & FMODE_WRITE)
			return put_user(os->fragsize, (int *)arg);
		else
			return put_user(is->fragsize, (int *)arg);

	case SNDCTL_DSP_GETCAPS:
		val = DSP_CAP_REALTIME|DSP_CAP_TRIGGER|DSP_CAP_MMAP;
		if (is && os)
			val |= DSP_CAP_DUPLEX;
		return put_user(val, (int *)arg);

	case SNDCTL_DSP_SETFRAGMENT:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		if (file->f_mode & FMODE_READ) {
			int ret = audio_set_fragments(is, val);
			if (ret < 0)
				return ret;
			ret = put_user(ret, (int *)arg);
			if (ret)
				return ret;
		}
		if (file->f_mode & FMODE_WRITE) {
			int ret = audio_set_fragments(os, val);
			if (ret < 0)
				return ret;
			ret = put_user(ret, (int *)arg);
			if (ret)
				return ret;
		}
		return 0;

	case SNDCTL_DSP_SYNC:
		return audio_sync(file);

	case SNDCTL_DSP_SETDUPLEX:
		return 0;

	case SNDCTL_DSP_POST:
		return 0;

	case SNDCTL_DSP_GETTRIGGER:
		val = 0;
		if (file->f_mode & FMODE_READ && is->active && !is->stopped)
			val |= PCM_ENABLE_INPUT;
		if (file->f_mode & FMODE_WRITE && os->active && !os->stopped)
			val |= PCM_ENABLE_OUTPUT;
		return put_user(val, (int *)arg);

	case SNDCTL_DSP_SETTRIGGER:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		if (file->f_mode & FMODE_READ) {
			if (val & PCM_ENABLE_INPUT) {
				unsigned long flags;
				if (!is->active) {
					if (!is->buffers && audio_setup_buf(is))
						return -ENOMEM;
					audio_prime_rx(state);
				}
				local_irq_save(flags);
				is->stopped = 0;
				audio_process_dma(is);
				local_irq_restore(flags);
			} else {
				audio_stop_dma(is);
			}
		}
		if (file->f_mode & FMODE_WRITE) {
			if (val & PCM_ENABLE_OUTPUT) {
				unsigned long flags;
				if (!os->buffers && audio_setup_buf(os))
					return -ENOMEM;
				local_irq_save(flags);
				if (os->mapped && !os->pending_frags) {
					os->pending_frags = os->nbfrags;
					sema_init(&os->sem, 0);
					os->active = 1;
				}
				os->stopped = 0;
				audio_process_dma(os);
				local_irq_restore(flags);
			} else {
				audio_stop_dma(os);
			}
		}
		return 0;

	case SNDCTL_DSP_GETOPTR:
	case SNDCTL_DSP_GETIPTR:
	    {
		count_info inf = { 0, };
		audio_stream_t *s = (cmd == SNDCTL_DSP_GETOPTR) ? os : is;
		int bytecount, offset;
		unsigned long flags;

		if ((s == is && !(file->f_mode & FMODE_READ)) ||
		    (s == os && !(file->f_mode & FMODE_WRITE)))
			return -EINVAL;
		if (s->active) {
			local_irq_save(flags);
			offset = audio_get_dma_pos(s);
			inf.ptr = s->dma_tail * s->fragsize + offset;
			bytecount = s->bytecount + offset;
			s->bytecount = -offset;
			inf.blocks = s->fragcount;
			s->fragcount = 0;
			local_irq_restore(flags);
			if (bytecount < 0)
				bytecount = 0;
			inf.bytes = bytecount;
		}
		return copy_to_user((void *)arg, &inf, sizeof(inf));
	    }

	case SNDCTL_DSP_GETOSPACE:
	case SNDCTL_DSP_GETISPACE:
	    {
		audio_buf_info inf = { 0, };
		audio_stream_t *s = (cmd == SNDCTL_DSP_GETOSPACE) ? os : is;

		if ((s == is && !(file->f_mode & FMODE_READ)) ||
		    (s == os && !(file->f_mode & FMODE_WRITE)))
			return -EINVAL;
		if (!s->buffers && audio_setup_buf(s))
			return -ENOMEM;
		inf.bytes = sema_count(&s->sem) * s->fragsize;
		inf.bytes -= s->buffers[s->usr_head].offset;
		inf.fragments = inf.bytes / s->fragsize;
		inf.fragsize = s->fragsize;
		inf.fragstotal = s->nbfrags;
		return copy_to_user((void *)arg, &inf, sizeof(inf));
	    }

	case SNDCTL_DSP_NONBLOCK:
		file->f_flags |= O_NONBLOCK;
		return 0;

	case SNDCTL_DSP_RESET:
		if (file->f_mode & FMODE_READ) {
			audio_reset(is);
			if (state->need_tx_for_rx) {
				unsigned long flags;
				local_irq_save(flags);
				os->spin_idle = 0;
				local_irq_restore(flags);
			}
		}
		if (file->f_mode & FMODE_WRITE) {
			audio_reset(os);
		}
		return 0;

	default:
		/*
		 * Let the client of this module handle the
		 * non generic ioctls
		 */
		return state->client_ioctl(inode, file, cmd, arg);
	}

	return 0;
}

static int audio_release(struct inode *inode, struct file *file)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;

	DPRINTK("audio_release\n");

	down(&state->sem);

	if (file->f_mode & FMODE_READ) {
		audio_discard_buf(is);
		DMA_FREE(is);
		is->dma_spinref = 0;
		if (state->need_tx_for_rx) {
			os->spin_idle = 0;
			if (!state->wr_ref) {
				DMA_FREE(os);
				os->dma_spinref = 0;
			}
		}
		state->rd_ref = 0;
	}

	if (file->f_mode & FMODE_WRITE) {
		audio_sync(file);
		audio_discard_buf(os);
		if (!state->need_tx_for_rx || !state->rd_ref) {
			DMA_FREE(os);
			os->dma_spinref = 0;
		}
		state->wr_ref = 0;
	}

	if (!AUDIO_ACTIVE(state)) {
	       if (state->hw_shutdown)
		       state->hw_shutdown(state->data);
	}

	up(&state->sem);
	return 0;
}


#ifdef CONFIG_PM

int sa1100_audio_suspend(audio_state_t *s, u32 state, u32 level)
{
	if (level == SUSPEND_DISABLE || level == SUSPEND_POWER_DOWN) {
		audio_stream_t *is = s->input_stream;
		audio_stream_t *os = s->output_stream;
		int stopstate;

		if (is && is->dma_regs) {
			stopstate = is->stopped;
			audio_stop_dma(is);
			DMA_CLEAR(is);
			is->dma_spinref = 0;
			is->stopped = stopstate;
		}
		if (os && os->dma_regs) {
			stopstate = os->stopped;
			audio_stop_dma(os);
			DMA_CLEAR(os);
			os->dma_spinref = 0;
			os->stopped = stopstate;
		}
		if (AUDIO_ACTIVE(s) && s->hw_shutdown)
			s->hw_shutdown(s->data);
	}
	return 0;
}

int sa1100_audio_resume(audio_state_t *s, u32 level)
{
	if (level == RESUME_ENABLE) {
		audio_stream_t *is = s->input_stream;
		audio_stream_t *os = s->output_stream;

		if (AUDIO_ACTIVE(s) && s->hw_init)
			s->hw_init(s->data);
		if (os && os->dma_regs) {
			DMA_RESET(os);
			audio_process_dma(os);
		}
		if (is && is->dma_regs) {
			DMA_RESET(is);
			audio_process_dma(is);
		}
	}
	return 0;
}

EXPORT_SYMBOL(sa1100_audio_suspend);
EXPORT_SYMBOL(sa1100_audio_resume);

#endif


int sa1100_audio_attach(struct inode *inode, struct file *file,
			audio_state_t *state)
{
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	int err, need_tx_dma;

	DPRINTK("audio_open\n");

	down(&state->sem);

	/* access control */
	err = -ENODEV;
	if ((file->f_mode & FMODE_WRITE) && !os)
		goto out;
	if ((file->f_mode & FMODE_READ) && !is)
		goto out;
	err = -EBUSY;
	if ((file->f_mode & FMODE_WRITE) && state->wr_ref)
		goto out;
	if ((file->f_mode & FMODE_READ) && state->rd_ref)
		goto out;
	err = -EINVAL;
	if ((file->f_mode & FMODE_READ) && state->need_tx_for_rx && !os)
		goto out;

	/* request DMA channels */
	need_tx_dma = ((file->f_mode & FMODE_WRITE) ||
		       ((file->f_mode & FMODE_READ) && state->need_tx_for_rx));
	if (state->wr_ref || (state->rd_ref && state->need_tx_for_rx))
		need_tx_dma = 0;
	if (need_tx_dma) {
		err = DMA_REQUEST(os, audio_dma_callback);
		if (err)
			goto out;
	}
	if (file->f_mode & FMODE_READ) {
		err = DMA_REQUEST(is, audio_dma_callback);
		if (err) {
			if (need_tx_dma)
				DMA_FREE(os);
			goto out;
		}
	}

	/* now complete initialisation */
	if (!AUDIO_ACTIVE(state)) {
		if (state->hw_init)
			state->hw_init(state->data);
	}

	if ((file->f_mode & FMODE_WRITE)) {
		state->wr_ref = 1;
		audio_reset(os);
		os->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		os->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		os->mapped = 0;
		init_waitqueue_head(&os->wq);
	}
	if (file->f_mode & FMODE_READ) {
		state->rd_ref = 1;
		audio_reset(is);
		is->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		is->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		is->mapped = 0;
		init_waitqueue_head(&is->wq);
	}

	file->private_data	= state;
	file->f_op->release	= audio_release;
	file->f_op->write	= audio_write;
	file->f_op->read	= audio_read;
	file->f_op->mmap	= audio_mmap;
	file->f_op->poll	= audio_poll;
	file->f_op->ioctl	= audio_ioctl;
	file->f_op->llseek	= audio_llseek;
	err = 0;

out:
	up(&state->sem);
	return err;
}

EXPORT_SYMBOL(sa1100_audio_attach);

MODULE_AUTHOR("Nicolas Pitre");
MODULE_DESCRIPTION("Common audio handling for the SA11x0 processor");
MODULE_LICENSE("GPL");
