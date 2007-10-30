/*
 * Common audio handling for the SA11x0
 *
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

/*
 * Buffer Management
 */

typedef struct {
	int offset;		/* current offset */
	char *data;		/* points to actual buffer */
	dma_addr_t dma_addr;	/* physical buffer address */
	int dma_ref;		/* DMA refcount */
	int master;		/* owner for buffer allocation, contain size when true */
} audio_buf_t;

typedef struct {
	char *id;		/* identification string */
	struct device *dev;	/* device */
	audio_buf_t *buffers;	/* pointer to audio buffer structures */
	u_int usr_head;		/* user fragment index */
	u_int dma_head;		/* DMA fragment index to go */
	u_int dma_tail;		/* DMA fragment index to complete */
	u_int fragsize;		/* fragment i.e. buffer size */
	u_int nbfrags;		/* nbr of fragments i.e. buffers */
	u_int pending_frags;	/* Fragments sent to DMA */
	dma_device_t dma_dev;	/* device identifier for DMA */
	dma_regs_t *dma_regs;	/* points to our DMA registers */
	int bytecount;		/* nbr of processed bytes */
	int fragcount;		/* nbr of fragment transitions */
	struct semaphore sem;	/* account for fragment usage */
	wait_queue_head_t wq;	/* for poll */
	int dma_spinref;	/* DMA is spinning */
	int mapped:1;		/* mmap()'ed buffers */
	int active:1;		/* actually in progress */
	int stopped:1;		/* might be active but stopped */
	int spin_idle:1;	/* have DMA spin on zeros when idle */
	int sa1111_dma:1;	/* DMA handled by SA1111 */
} audio_stream_t;

/*
 * State structure for one instance
 */

typedef struct {
	audio_stream_t *output_stream;
	audio_stream_t *input_stream;
	int rd_ref:1;		/* open reference for recording */
	int wr_ref:1;		/* open reference for playback */
	int need_tx_for_rx:1;	/* if data must be sent while receiving */
	void *data;
	void (*hw_init)(void *);
	void (*hw_shutdown)(void *);
	int (*client_ioctl)(struct inode *, struct file *, uint, ulong);
	struct semaphore sem;	/* to protect against races in attach() */
} audio_state_t;

/*
 * Functions exported by this module
 */
extern int sa1100_audio_attach( struct inode *inode, struct file *file,
				audio_state_t *state);
int sa1100_audio_suspend(audio_state_t *s, u32 state, u32 level);
int sa1100_audio_resume(audio_state_t *s, u32 level);

