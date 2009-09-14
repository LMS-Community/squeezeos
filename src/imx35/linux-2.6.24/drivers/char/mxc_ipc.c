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
 * @file mxc_ipc.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for all kind of transfers between MCU and DSP core.
 *
 * The Interfaces are with respect to the MCU core. Any driver or user-space
 * application on the MCU side can transfer messages or data to the DSP side
 * using the interfaces implemented here.
 *
 * @ingroup IPC
 */

/*
 * Include Files
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/circ_buf.h>
#include <linux/uio.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <asm/arch/mxc_mu.h>
#include <asm/arch/mxc_ipc.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "mxc_mu_reg.h"

/*!
 * Automatic major assignment
 */
#define MXC_IPC_MAJOR                  0x0

/*! SDMA total receive buffer size, used by kernel space API */
#define MAX_SDMA_RX_BUF_SIZE           8192
/*! SDMA transmit buffer maximum size */
#define MAX_SDMA_TX_BUF_SIZE           8192
/*!
 * Number of buffers used for SDMA receive, used by user API. This should
 * be set to a power of 2.
 */
#define NUM_RX_SDMA_BUF                16
/*! Size of each SDMA receive buffer */
#define SDMA_RX_BUF_SIZE               512
/*! MU buffer maximum size */
#define MAX_MU_BUF_SIZE                512

/*!
 * Max virtual channels available for this implementation
 */
#define IPC_MAX_VIRTUAL_CHANNELS       32

/*!
 * Max ipc channels available for this implementation
 */
#define IPC_MAX_IPC_CHANNEL            10

/*!
 * This defines an unused virtual channel
 */
#define IPC_DUMMY_CHANNEL              IPC_MAX_VIRTUAL_CHANNELS + 1

/*!
 * Max config index allowed for IPC channels. There are three types:
 * read/write channel for messages
 * read/write channel for data
 * read only for data
 */
#define IPC_MAX_MU_CHANNEL_INDEX                3
#define IPC_MAX_SDMA_BIDI_CHANNEL_INDEX         1
#define IPC_MAX_SDMA_MONO_CHANNEL_INDEX         3

/*!
 * States allowed for an IPC channel
 */
#define CHANNEL_CLOSED                          0x0
#define CHANNEL_OPEN                            0x1
#define CHANNEL_READ_ONGOING                    0x2
#define CHANNEL_WRITE_ONGOING                   0x4

/*!
 * Physical channels used to implement IPC channels.
 * This defines depends on channels used on DSP side
 * i.e. if the DSP uses SDMA channel 1 to do writes,
 * we must use SDMA channel 1 to receive data.
 *
 * Currently we use 8 physical channels to implement the
 * IPC abstraction.
 *
 * - MU channel 0   --> IPC channel 0
 * - MU channel 1   --> IPC channel 1
 * - MU channel 2   --> IPC channel 2
 * - MU channel 3   --> IPC channel 3
 * - SDMA Log channel 0 --> IPC channel 4
 * - SDMA Log channel 1 --> IPC channel 5
 * - SDMA Log channel 2 --> IPC channel 6
 * - SDMA Log channel 3 --> IPC channel 7
 * - SDMA Data channel 0 --> IPC channel 8
 * - SDMA Data channel 1 --> IPC channel 9
 *
 * Note that IPC channels 8 and 9 uses two physical channels. This
 * is because SDMA channels are unidirectional and IPC
 * links must be bidirectional.
 *
 */
#define SHORT_MESSAGE_CHANNEL0                  0
#define SHORT_MESSAGE_CHANNEL1                  1
#define SHORT_MESSAGE_CHANNEL2                  2
#define SHORT_MESSAGE_CHANNEL3                  3
#define LOGGING_CHANNEL0                        4
#define LOGGING_CHANNEL1                        5
#define LOGGING_CHANNEL2                        6
#define LOGGING_CHANNEL3                        7
#define PACKET_DATA_CHANNEL0                    8
#define PACKET_DATA_CHANNEL1                    9

/*!
 * SDMA physical channels used for the different IPC channels
 */
#define PACKET_DATA0_SDMA_RD_CHNL               1
#define PACKET_DATA0_SDMA_WR_CHNL               2
#define PACKET_DATA1_SDMA_RD_CHNL               3
#define PACKET_DATA1_SDMA_WR_CHNL               4
#define LOG0_SDMA_RD_CHNL                       5
#define LOG1_SDMA_RD_CHNL                       6
#define LOG2_SDMA_RD_CHNL                       7
#define LOG3_SDMA_RD_CHNL                       8

/*!
 * Operations allowed on IPC channels. These macros are
 * used to tell the IPC driver-interface level which
 * operation to execute.
*/
#define IPC_READ                                0x1
#define IPC_WRITE                               0x2
#define IPC_RDWR                                IPC_READ | IPC_WRITE

#define IPC_WRITE_BYTE_INIT_VAL                 -4

#define IPC_DEFAULT_MAX_CTRL_STRUCT_NUM         50

typedef void (*read_callback_t) (HW_CTRL_IPC_READ_STATUS_T *);
typedef void (*write_callback_t) (HW_CTRL_IPC_WRITE_STATUS_T *);
typedef void (*notify_callback_t) (HW_CTRL_IPC_NOTIFY_STATUS_T *);

/*!
 * This structure represents the IPC channels SDMA receive buffer.
 */
struct sdma_rbuf {
	/*! Virtual addres of the read buffer */
	char *buf;

	/*! Physical address of the SDMA read buffers */
	dma_addr_t rpaddr;

	/*! Amount of data available to read */
	int count;

	/*! Offset within read buffer */
	int offset;
};

/*!
 * This structure represents the SDMA channels used
 * by the IPC abstraction.
 */
struct sdma_channel {
	/*! SDMA read channel number */
	int read_channel;

	/*! SDMA write channel number */
	int write_channel;

	/*! SDMA write buffer */
	char *wbuf;

	/*! Physical address of the SDMA write buffer */
	dma_addr_t wpaddr;

	/*! DMA read buffers, uses muti-buffering scheme */
	struct sdma_rbuf rbuf[NUM_RX_SDMA_BUF];

	/*! Head to keep track of DMA read buffers */
	int rbuf_head;

	/*! Tail to keep track of DMA read buffers */
	int rbuf_tail;

	/*! Head to keep track of DMA write buffers */
	int wbuf_head;

	/*! Tail to keep track of DMA write buffers */
	int wbuf_tail;

	/*! Tasklet to call SDMA read callback */
	struct tasklet_struct read_tasklet;

	/*! Tasklet to call SDMA write callback */
	struct tasklet_struct write_tasklet;
};

/*!
 * This structure represents the MU channels used
 * by the IPC abstraction.
 */
struct mu_channel {
	/*! MU channel number */
	int index;

	/*! MU read buffer */
	struct circ_buf *rbuf;

	/*! MU write buffer */
	struct circ_buf *wbuf;

	/*! Number of bytes writen */
	int bytes_written;
};

/*!
 * This structure represents an IPC channel. It is represented
 * by one or even two physical channels, plus a control structure
 */
struct ipc_channel {
	/*! Physical channel(s) used to implement an IPC channel */
	union {
		struct sdma_channel sdma;
		struct mu_channel mu;
	} ch;

	/*!
	 * Control structure. It handles access and transfers on this
	 * IPC channel
	 */
	struct ipc_priv_data *priv;

	/*! Lock to protect the channel buffers */
	spinlock_t channel_lock;
};

/*!
 * This structure represents a virtual channel. Multiple virtual channels
 * could be connected to an IPC channel. IPC abstraction supports up to 32
 * virtual channels.
 */
struct virtual_channel {
	/*! Controls access to the handle of this channel */
	struct semaphore sem;

	/*! Current state of the channel */
	atomic_t state;
	/*!Added state variables for IPCv2 Read-while -write functionality */
	atomic_t read_state_ipcv2;

	atomic_t write_state_ipcv2;

	/*!
	 *
	 */
	char *data;

	/*!
	 * Pointer to the control structure of the IPC channel mapped by
	 * this virtual channel
	 */
	struct ipc_channel *ipc;
};

/*!
 * Private data. Each time an IPC channel is opened, a private
 * structure is allocated and stored in the priv
 * field of the file descriptor. This data is
 * used for further operations on the recently opened
 * channel
 */
struct ipc_priv_data {
	/*! Blocking mode (non-blocking, blocking) */
	atomic_t blockmode;

	/*! Physical channel to which the virtual channel is mapped to */
	unsigned short pchannel;

	/*! Index of virtual channel */
	unsigned short vchannel;

	/*! This stores number of bytes received */
	unsigned int read_count;

	/*! This stores number of bytes transmitted */
	unsigned int write_count;

	/*! Write waitqueue */
	wait_queue_head_t wq;

	/*! Read waitqueue */
	wait_queue_head_t rq;

	/*! Store the task_struct structure of the calling process. */
	struct task_struct *owner;

	/*!
	 * Virtual channel assigned to the IPC channel storing
	 * this structure.
	 */
	struct virtual_channel *vc;

	/*!
	 * Callbacks used to signal the end of a operation
	 * on an IPC channel. Only for kernel modules.
	 */
	struct kernel_callbacks *k_callbacks;
};

/*!
 * This struct stores the callbacks pointers
 * used to communicate the end of an operation
 * on an IPC channel. These callbacks must be
 * provided by the caller. This feature is available
 * only for kernel modules.
 */
struct kernel_callbacks {
	read_callback_t read;
	write_callback_t write;
	notify_callback_t notify;
};

/*!
 * Array used to store the physical channels.
 */
static struct ipc_channel ipc_channels[IPC_MAX_IPC_CHANNEL];

/*!
 * Array used to store the pointers to the virtual channels.
 * These pointers will be allocated when trying to open a virtual channel.
 */
static struct virtual_channel virtual_channels[IPC_MAX_VIRTUAL_CHANNELS];

/*!
 * Array used to store the channel handlers, correlated to the virtual_channels array.
 * In future implementations, the handler could have been directly a
 * virtual_channel pointer, but then it would need an harmonization between AP and BP
 * structures to properly define what is a handler to a virtual channel on both sides.
 */
static HW_CTRL_IPC_CHANNEL_T channel_handlers[IPC_MAX_VIRTUAL_CHANNELS];

/*!
 * Status registers of the MU channels
 */
static unsigned int wbits[4] = { AS_MUMSR_MTE0, AS_MUMSR_MTE1, AS_MUMSR_MTE2,
	AS_MUMSR_MTE3
};
static unsigned int rbits[4] = { AS_MUMSR_MRF0, AS_MUMSR_MRF1, AS_MUMSR_MRF2,
	AS_MUMSR_MRF3
};

/*!
 * Major number
 */
static int major_num = 0;

static struct class *mxc_ipc_class;

static void sdma_user_readcb(void *args);
static void sdma_user_writecb(void *args);

/*!
 * Locks the virtual channel located on the index channel.
 * @param channel index of the virtual channel to lock
 */
static inline int lock_virtual_channel(unsigned short channel)
{
	return (down_interruptible(&virtual_channels[channel].sem));
}

/*!
 * Try to locks the virtual channel located on the index channel.
 * @param channel index of the virtual channel to (try)lock
 */
static inline int trylock_virtual_channel(unsigned short channel)
{
	return down_trylock(&virtual_channels[channel].sem);
}

/*!
 * Unlocks the virtual channel located on the index channel
 * @param channel index of the virtual channel to unlock
 */
static inline void unlock_virtual_channel(unsigned short channel)
{
	up(&virtual_channels[channel].sem);
}

/*!
 * Finds an unused channel into the virtual channel array
 *
 * @return  The function returns teh index of the first unused
 *    channel found, IPC_DUMMY_CHANNEL otherwise.
 */
inline int get_free_virtual_channel(void)
{
	struct virtual_channel *vch = NULL;
	int i;

	for (i = 0; i < IPC_MAX_VIRTUAL_CHANNELS; i++) {
		if (trylock_virtual_channel(i) == 1) {
			continue;
		}

		vch = &virtual_channels[i];

		if (atomic_read(&vch->state) == CHANNEL_CLOSED) {
			return i;
		}

		unlock_virtual_channel(i);
	}

	return IPC_DUMMY_CHANNEL;
}

/*!
 * The MU interrupt handler schedules this tasklet for
 * execution to start processing a write request.
 * This function writes data to chnum MU register and signals
 * the end of the transfer to the servicing thread attached
 * to the IPC channel.
 *
 * @param chnum    MU channel number where the operation will occur
 */
static void mu_write_tasklet(int chnum)
{
	struct mu_channel *mu = NULL;
	struct ipc_priv_data *priv = NULL;
	char *p = NULL;
	int bytes, error = 0;

	priv = ipc_channels[chnum].priv;
	mu = &ipc_channels[chnum].ch.mu;

	/* Check for bytes in the IPC's MU channel write buffer */
	bytes = CIRC_CNT(mu->wbuf->head, mu->wbuf->tail, MAX_MU_BUF_SIZE);

	while ((bytes != 0) && (readl(AS_MUMSR) & wbits[chnum])) {

		pr_debug("remaining bytes = %d\n", bytes);

		p = &mu->wbuf->buf[mu->wbuf->tail];

		if (mxc_mu_mcuwrite(mu->index, p)) {
			pr_debug("Mu MCU write Failed\n");
			error = 1;
			break;
		}
		mu->wbuf->tail = (mu->wbuf->tail + 4) & (MAX_MU_BUF_SIZE - 1);
		bytes = CIRC_CNT(mu->wbuf->head, mu->wbuf->tail,
				 MAX_MU_BUF_SIZE);
	}

	spin_lock(&ipc_channels[chnum].channel_lock);
	bytes = CIRC_CNT(mu->wbuf->head, mu->wbuf->tail, MAX_MU_BUF_SIZE);
	/* Disable interrupts if buffer is empty */
	if ((bytes == 0) && (error == 0)) {
		pr_debug("(2)remaining bytes = %d\n", bytes);
		mxc_mu_intdisable(mu->index, TX);
	}
	spin_unlock(&ipc_channels[chnum].channel_lock);

	if (waitqueue_active(&priv->wq)) {
		wake_up_interruptible(&priv->wq);
	}
}

/*!
 * The MU interrupt handler schedules this tasklet for
 * execution to start processing a read request.
 * This function reads data from chnum MU register and signals
 * the end of the transfer to the servicing thread attached
 * to the IPC channel.
 *
 * @param   chnum    MU channel number where the operation will occur
 */
static void mu_read_tasklet(int chnum)
{
	struct mu_channel *mu = NULL;
	struct ipc_priv_data *priv = NULL;
	int bytes = 0;
	char p[4];
	int error = 0;

	priv = ipc_channels[chnum].priv;
	mu = &ipc_channels[chnum].ch.mu;

	bytes = CIRC_SPACE(mu->rbuf->head, mu->rbuf->tail, MAX_MU_BUF_SIZE);

	while ((bytes >= 4) && (readl(AS_MUMSR) & rbits[chnum])) {

		pr_debug("space available = %d\n", bytes);

		if (mxc_mu_mcuread(mu->index, p)) {
			pr_debug("MU MCU Read Failed\n");
			error = 1;
			break;
		}

		*((int *)(&(mu->rbuf->buf[mu->rbuf->head]))) = *((int *)p);
		mu->rbuf->head = (mu->rbuf->head + 4) & (MAX_MU_BUF_SIZE - 1);

		bytes = CIRC_SPACE(mu->rbuf->head, mu->rbuf->tail,
				   MAX_MU_BUF_SIZE);
	}

	/* Check if space available in read buffer */
	spin_lock(&ipc_channels[chnum].channel_lock);
	bytes = CIRC_SPACE(mu->rbuf->head, mu->rbuf->tail, MAX_MU_BUF_SIZE);
	if ((bytes < 4) && (error == 0)) {
		mxc_mu_intdisable(mu->index, RX);
	} else {
		pr_debug("(2)space available = %d\n", bytes);
		mxc_mu_intenable(mu->index, RX);
	}
	spin_unlock(&ipc_channels[chnum].channel_lock);
	if (waitqueue_active(&priv->rq)) {
		wake_up_interruptible(&priv->rq);
	}
}

/*!
 * The MU interrupt handler schedules this tasklet for
 * execution to start processing a read request.
 * This function reads data from chnum MU register and signals
 * the end of the transfer to the servicing thread attached
 * to the IPC channel.
 *
 * This function is used when the caller is a kernel module
 *
 * @param   chnum    MU channel number where the operation will occur
 */
static void mu_read_tasklet_kernel(int chnum)
{
	struct ipc_priv_data *priv = NULL;
	struct mu_channel *mu = NULL;
	char p[4];
	int bytes = 0;
	int error = 0;

	priv = ipc_channels[chnum].priv;
	mu = &ipc_channels[chnum].ch.mu;

	while ((bytes < priv->read_count) && (readl(AS_MUMSR) & rbits[chnum])) {
		pr_debug("space available = %d\n", bytes);
		if (mxc_mu_mcuread(mu->index, p)) {
			pr_debug("MU MCU Read Failed");
			error = 1;
			break;
		}

		*((int *)(&(priv->vc->data[bytes]))) = *((int *)p);
		bytes += 4;
	}

	if (error != 0) {
		mxc_mu_intenable(mu->index, RX);
	} else {
		mxc_mu_intdisable(mu->index, RX);
		mxc_mu_unbind(mu->index, RX);
		atomic_set(&priv->vc->state, CHANNEL_OPEN);

		if (priv->k_callbacks->read != NULL) {
			HW_CTRL_IPC_READ_STATUS_T status;

			status.channel = &channel_handlers[priv->vchannel];
			status.nb_bytes = bytes;
			priv->k_callbacks->read(&status);
		}
	}
}

/*!
 * The MU interrupt handler schedules this tasklet for
 * execution to start processing a write request.
 * This function write data to chnum MU register and signals
 * the end of the transfer to the servicing thread attached
 * to the IPC channel.
 *
 * This function is used when the caller is a kernel module
 *
 * @param   chnum    MU channel number where the operation will occur
 */
static void mu_write_tasklet_kernel(int chnum)
{
	struct ipc_priv_data *priv = NULL;
	struct mu_channel *mu = NULL;
	char *p = NULL;
	int error = 0;

	priv = ipc_channels[chnum].priv;
	mu = &ipc_channels[chnum].ch.mu;

	/* Indication that previous write was successful */
	mu->bytes_written += 4;

	while (mu->bytes_written < priv->write_count) {
		pr_debug("remaining bytes = %d\n", mu->bytes_written);
		p = &priv->vc->data[mu->bytes_written];
		if (mxc_mu_mcuwrite(mu->index, p) != 0) {
			error = 1;
			break;
		}
		if ((readl(AS_MUMSR) & wbits[chnum]) == 1) {
			mu->bytes_written += 4;
		} else {
			/*
			 * Data not yet read by DSP side, enable interrupts
			 * and exit
			 */
			break;
		}
	}

	/* Enable interrupts if there is data pending to be transmitted */
	if ((error == 0) && (mu->bytes_written < priv->write_count)) {
		mxc_mu_intenable(mu->index, TX);
	} else {
		mxc_mu_unbind(mu->index, TX);
		atomic_set(&priv->vc->state, CHANNEL_OPEN);

		if (priv->k_callbacks->write != NULL) {
			HW_CTRL_IPC_WRITE_STATUS_T status;

			status.channel = &channel_handlers[priv->vchannel];
			status.nb_bytes = mu->bytes_written;
			priv->k_callbacks->write(&status);
		}
	}
}

/*!
 * Enable MU channel interrupts
 *
 * @param   chnum    MU channel to enable
 * @return  returns 0 on success, negative value otherwise
 */
static int mxc_ipc_enable_muints(int chnum)
{
	int status = 0;
	struct mu_channel *mu = NULL;

	mu = &ipc_channels[chnum].ch.mu;
	/* Bind the default callback functions */
	status = mxc_mu_bind(mu->index, &mu_read_tasklet, RX);
	if (status == 0) {
		status = mxc_mu_bind(mu->index, &mu_write_tasklet, TX);
	}
	if (status == 0) {
		status = mxc_mu_intdisable(mu->index, TX);
	}
	/* Enable the RX interrupt to start receiving data */
	if (status == 0) {
		status = mxc_mu_intenable(mu->index, RX);
	}

	/* Cleanup if we had an error at any step */
	if (status != 0) {
		pr_debug("Error enabling MU channel %d interrupts \n", chnum);
		mxc_mu_unbind(mu->index, RX);
		mxc_mu_unbind(mu->index, TX);
	}

	return status;
}

/*!
 * Allocate a MU channel
 *
 * @param   chnum    MU channel to allocate
 * @return  returns 0 on success, negative value otherwise
 */
static int allocate_mu_channel(int chnum)
{
	struct mu_channel *mu = NULL;
	int num = 0, status = 0;

	num = mxc_mu_alloc_channel(chnum);
	if (num < 0) {
		pr_debug("Allocation of MU channel %d failed \n", chnum);
		return num;
	}
	mu = &ipc_channels[chnum].ch.mu;
	mu->index = num;

	status = mxc_mu_intdisable(mu->index, TX);
	if (status == 0) {
		status = mxc_mu_intdisable(mu->index, RX);
	}

	if (status != 0) {
		mxc_mu_dealloc_channel(mu->index);
		return status;
	}
	/* Allocate the read and write buffers */
	mu->rbuf = kmalloc(sizeof(struct circ_buf), GFP_KERNEL);
	if (mu->rbuf == NULL) {
		goto err_alloc_rbuf;
	}
	mu->rbuf->buf = kmalloc(MAX_MU_BUF_SIZE, GFP_KERNEL);
	if (mu->rbuf->buf == NULL) {
		goto err_alloc_rbuf_buf;
	}
	mu->wbuf = kmalloc(sizeof(struct circ_buf), GFP_KERNEL);
	if (mu->wbuf == NULL) {
		goto err_alloc_wbuf;
	}
	mu->wbuf->buf = kmalloc(MAX_MU_BUF_SIZE, GFP_KERNEL);
	if (mu->wbuf->buf == NULL) {
		goto err_alloc_wbuf_buf;
	}
	spin_lock_init(&ipc_channels[chnum].channel_lock);
	mu->wbuf->head = mu->wbuf->tail = 0;
	mu->rbuf->head = mu->rbuf->tail = 0;

	return 0;

      err_alloc_wbuf_buf:
	kfree(mu->wbuf);
      err_alloc_wbuf:
	kfree(mu->rbuf->buf);
      err_alloc_rbuf_buf:
	kfree(mu->rbuf);
      err_alloc_rbuf:
	mxc_mu_dealloc_channel(mu->index);
	return -ENOMEM;
}

/*! Deallocate a MU channel
 *
 * @param   chnum    MU channel to allocate
 * @return  returns 0 on success, negative value otherwise
 */
static int deallocate_mu_channel(int chnum)
{
	struct mu_channel *mu = NULL;
	int ret = 0;

	mu = &ipc_channels[chnum].ch.mu;

	ret = mxc_mu_intdisable(mu->index, TX);
	ret += mxc_mu_intdisable(mu->index, RX);
	ret += mxc_mu_unbind(mu->index, TX);
	ret += mxc_mu_unbind(mu->index, RX);
	ret += mxc_mu_dealloc_channel(mu->index);

	mu->index = -1;

	/* Free the read and write buffers */
	kfree(mu->rbuf->buf);
	kfree(mu->rbuf);
	kfree(mu->wbuf->buf);
	kfree(mu->wbuf);

	return ret;
}

/*!
 * Start the SDMA read channel accessed from kernel mode
 *
 * @param   ipc_chnl SDMA channel to start
 * @param   num_od_bd  number of BD's allocated for the channel
 *
 * @return 0 on succes, negative number if channel does not start
 */
static int initialize_sdma_read_channel_kernel(struct ipc_channel *ipc_chnl,
					       int num_of_bd)
{
	struct sdma_channel *read_chnl = NULL;
	dma_channel_params sdma_params;
	int result = 0;

	read_chnl = &ipc_chnl->ch.sdma;

	memset(&sdma_params, 0, sizeof(dma_channel_params));
	sdma_params.peripheral_type = DSP;
	sdma_params.transfer_type = dsp_2_emi;
	sdma_params.event_id = 0;
	sdma_params.callback = NULL;
	sdma_params.arg = 0;
	sdma_params.bd_number = num_of_bd;

	result = mxc_dma_setup_channel(read_chnl->read_channel, &sdma_params);
	if (result < 0) {
		return result;
	}

	return result;
}

/*!
 * Start the SDMA read channel accessed from user mode
 *
 * @param   ipc_chnl SDMA channel to start
 *
 * @return 0 on succes, negative number if channel does not start
 */
static int initialize_sdma_read_channel_user(int ipc_chnl)
{
	dma_request_t sdma_request;
	struct ipc_channel ipc_num;
	struct sdma_channel *read_chnl = NULL;
	dma_channel_params sdma_params;
	int result = 0;
	int i = 0;

	ipc_num = ipc_channels[ipc_chnl];
	read_chnl = &ipc_channels[ipc_chnl].ch.sdma;

	memset(&sdma_params, 0, sizeof(dma_channel_params));
	sdma_params.peripheral_type = DSP;
	sdma_params.transfer_type = dsp_2_emi;
	sdma_params.event_id = 0;
	sdma_params.callback = NULL;
	sdma_params.arg = 0;
	sdma_params.bd_number = NUM_RX_SDMA_BUF;

	result = mxc_dma_setup_channel(read_chnl->read_channel, &sdma_params);
	if (result < 0) {
		return result;
	} else {
		for (i = 0; i < NUM_RX_SDMA_BUF; i++) {
			/* Allocate the DMA'able buffers filled by SDMA engine */
			read_chnl->rbuf[i].buf =
			    dma_alloc_coherent(NULL, SDMA_RX_BUF_SIZE,
					       &read_chnl->rbuf[i].rpaddr,
					       GFP_DMA);
			if (read_chnl->rbuf[i].buf == NULL) {
				result = -1;
				break;
			}
			read_chnl->rbuf[i].count = 0;
			read_chnl->rbuf[i].offset = 0;
			memset(&sdma_request, 0, sizeof(dma_request_t));
			sdma_request.count = SDMA_RX_BUF_SIZE;
			sdma_request.destAddr =
			    (__u8 *) read_chnl->rbuf[i].rpaddr;
			sdma_request.bd_cont = 1;
			result = mxc_dma_set_config(read_chnl->read_channel,
						    &sdma_request, i);
			if (result != 0) {
				break;
			}
		}
	}

	if (result != 0) {
		return result;
	}

	read_chnl->rbuf_head = 0;
	read_chnl->rbuf_tail = 0;

	mxc_dma_set_callback(read_chnl->read_channel, sdma_user_readcb,
			     ipc_num.priv);
	mxc_dma_start(read_chnl->read_channel);
	return 0;
}

/*!
 * Setup the write channel for user mode and kernel mode
 *
 * @param   ipc_chnl      SDMA channel to start
 * @param   kernel_access flag indicates the access type, 1=kernel mode access.
 * @param   num_of_bd     number of BD's allocated for the channel
 *
 * @return 0 on succes, negative number if channel does not start
 */
static int initialize_sdma_write_channel(struct ipc_channel *ipc_chnl,
					 int kernel_access, int num_of_bd)
{
	struct sdma_channel *write_chnl = NULL;
	dma_channel_params sdma_params;
	int result = 0;

	write_chnl = &ipc_chnl->ch.sdma;
	memset(&sdma_params, 0, sizeof(dma_channel_params));
	sdma_params.peripheral_type = DSP;
	sdma_params.transfer_type = emi_2_dsp;
	sdma_params.event_id = 0;
	sdma_params.callback = NULL;
	sdma_params.arg = 0;
	sdma_params.bd_number = num_of_bd;

	result = mxc_dma_setup_channel(write_chnl->write_channel, &sdma_params);
	if (result < 0) {
		pr_debug("Failed Setting up SDMA channel %d\n",
			 write_chnl->write_channel);
		mxc_free_dma(write_chnl->write_channel);
		write_chnl->write_channel = -1;
		return result;
	}

	/*
	 * Allocate the write buffer for user space, used to copy user-space
	 * data for DMA.
	 */
	if (kernel_access == 0) {
		write_chnl->wbuf =
		    dma_alloc_coherent(NULL, MAX_SDMA_TX_BUF_SIZE,
				       &write_chnl->wpaddr, GFP_DMA);
		if (write_chnl->wbuf == NULL) {
			return -ENOMEM;
		}
	}

	return 0;
}

/*!
 * Allocate a SDMA channel
 *
 * @param   ipc_chnl  IPC channel to allocate
 * @param   chnum     SDMA channel to allocate
 * @param   mode      open mode for this channel (read, write, read/write)
 *
 * @return  returns 0 on success, negative number otherwise.
 */
static int allocate_sdma_channel(int ipc_chnl, int chnum, int mode)
{
	struct ipc_channel ipc_num;
	struct sdma_channel *sdma = NULL;
	int result = 0;

	ipc_num = ipc_channels[ipc_chnl];
	sdma = &ipc_channels[ipc_chnl].ch.sdma;
	if (mode == (IPC_RDWR)) {
		sdma->write_channel = chnum;
		sdma->read_channel = chnum - 1;
	} else {
		sdma->write_channel = -1;
		sdma->read_channel = chnum;
	}

	if (mode & IPC_WRITE) {
		result =
		    mxc_request_dma(&sdma->write_channel, "IPC WRITE SDMA");
		if (result < 0) {
			pr_debug("Failed Opening SDMA channel %d\n",
				 sdma->write_channel);
			sdma->write_channel = -1;
			return result;
		}
	}

	if (mode & IPC_READ) {
		result = mxc_request_dma(&sdma->read_channel, "IPC READ SDMA");
		if (result < 0) {
			pr_debug("Failed Opening SDMA channel %d\n",
				 sdma->read_channel);
			sdma->read_channel = -1;
			if (mode & IPC_WRITE) {
				mxc_free_dma(sdma->write_channel);
				sdma->write_channel = -1;
			}
			return result;
		}
	}

	spin_lock_init(&ipc_channels[chnum].channel_lock);
	return result;
}

static void mxc_ipc_free_readbuf_user(int chnum)
{
	int i = 0;
	struct sdma_channel *sdma;

	sdma = &ipc_channels[chnum].ch.sdma;

	/* Free the DMA'able receive buffers */
	for (i = 0; i < NUM_RX_SDMA_BUF; i++) {
		if (sdma->rbuf[i].buf != NULL) {
			dma_free_coherent(NULL, SDMA_RX_BUF_SIZE,
					  sdma->rbuf[i].buf,
					  sdma->rbuf[i].rpaddr);
			sdma->rbuf[i].buf = NULL;
		}
	}
}

/*!
 * Deallocate a SDMA channel
 *
 * @param   chnum     SDMA channel to allocate
 */
static void deallocate_sdma_channel(int chnum)
{
	struct sdma_channel *sdma;

	sdma = &ipc_channels[chnum].ch.sdma;
	if (sdma->write_channel != -1) {
		mxc_free_dma(sdma->write_channel);
	}
	if (sdma->read_channel != -1) {
		mxc_free_dma(sdma->read_channel);
	}
	sdma->write_channel = -1;
	sdma->read_channel = -1;

	/* Free the DMA'able transmit buffer */
	if (sdma->wbuf != NULL) {
		dma_free_coherent(NULL, MAX_SDMA_TX_BUF_SIZE,
				  sdma->wbuf, sdma->wpaddr);
		sdma->wbuf = NULL;
	}
}

/*!
 * Copy bytes from the circular buffer to the user buffer
 *
 * @param   buf          buffer to store data
 * @param   count        number of bytes to copy
 * @param   cbuf         circular_buffer to copy the data from
 * @param   buf_size     size of the circular buffer
 *
 * @return  number of bytes copied into the user buffer
 */
static int mxc_ipc_copy_from_mubuf(char *buf, int count, struct circ_buf *cbuf,
				   int buf_size)
{
	int n = 0, ret = 0, result = 0;

	while (1) {
		n = CIRC_CNT_TO_END(cbuf->head, cbuf->tail, buf_size);
		pr_debug("bytes available in circular buffer = %d\n", n);
		if (count < n) {
			n = count;
		}
		if (n <= 0) {
			break;
		}
		pr_debug("copying %d bytes into user's buffer\n", n);
		result = copy_to_user(buf, cbuf->buf + cbuf->tail, n);
		n -= result;
		cbuf->tail = (cbuf->tail + n) & (buf_size - 1);
		buf += n;
		count -= n;
		ret += n;
		/* Check if there is problem copying data */
		if (result != 0) {
			break;
		}
	}

	return ret;
}

/*!
 * Write bytes to the circular buffer from the user buffer
 *
 *
 * @param   buf          buffer to copy data from
 * @param   count        number of bytes to copy
 * @param   cbuf         circular_buffer to copy the data to
 *
 * @return  number of bytes copied into the circular buffer
 */
static int mxc_ipc_copy_to_buf(char **buf, int *count, struct circ_buf *cbuf)
{
	int n = 0;
	int result;
	int ret = 0;

	while (1) {
		n = CIRC_SPACE_TO_END(cbuf->head, cbuf->tail, MAX_MU_BUF_SIZE);
		if (*count < n) {
			n = *count;
		}
		if (n < 4) {
			break;
		}
		result = copy_from_user(cbuf->buf + cbuf->head, *buf, n);
		if (result != 0) {
			pr_debug("EINVAL error\n");
			ret = -EFAULT;
			break;
		}
		n -= result;
		cbuf->head = (cbuf->head + n) & (MAX_MU_BUF_SIZE - 1);
		*buf += n;
		*count -= n;
	}

	return ret;
}

/*!
 * This function copies data from the MU buffer into the user read buffer.
 *
 * @param    priv       Private data. Used to synchronize the top half
 *                      and the botton half as well as to keep track of
 *                      the transfer
 * @param    count      number of bytes to be transferred
 * @param    buf        buffer where received bytes are saved
 *
 * @return              number of transferred bytes
 */
static int mu_start_dsp_transfer(struct ipc_priv_data *priv,
				 int count, char *buf)
{
	int bytes = 0;
	struct mu_channel *mu = &priv->vc->ipc->ch.mu;

	bytes = mxc_ipc_copy_from_mubuf(buf, count, mu->rbuf, MAX_MU_BUF_SIZE);
	pr_debug("bytes read = %d\n", bytes);

	spin_lock(&priv->vc->ipc->channel_lock);
	/* Enable intrs if space in receive buffer */
	if (CIRC_SPACE(mu->rbuf->head, mu->rbuf->tail, MAX_MU_BUF_SIZE)) {
		mxc_mu_intenable(mu->index, RX);
	} else {
		mxc_mu_intdisable(mu->index, RX);
	}
	spin_unlock(&priv->vc->ipc->channel_lock);

	if (bytes == 0) {
		pr_debug("Could not copy to user buffer error\n");
		return -EFAULT;
	}

	return bytes;
}

/*!
 * This function starts a copies data into MU buffer and starts the transfer.
 *
 * @param    priv       Private data. Used to synchronize the top half
 *                      and the botton half as well as to keep track of
 *                      the transfer
 * @param    count      number of bytes to be transferred
 * @param    buf        buffer containing the bytes to be transferred
 *
 * @return              number of transferred bytes.
 */
static int mu_start_mcu_transfer(struct ipc_priv_data *priv,
				 int count, const char *buf)
{
	struct mu_channel *mu = &priv->vc->ipc->ch.mu;
	int retval = 0, orig_cnt;

	orig_cnt = count;

	/* Copy all the data into write buffer */
	while (count > 0) {
		retval = mxc_ipc_copy_to_buf((char **)&buf, &count, mu->wbuf);
		if (retval < 0) {
			break;
		}

		/* Enable TX intr, data available in buffer */
		spin_lock(&priv->vc->ipc->channel_lock);
		mxc_mu_intenable(mu->index, TX);
		spin_unlock(&priv->vc->ipc->channel_lock);
		/* Check if non-blocking mode requested */
		if ((atomic_read(&priv->blockmode)) == 0) {
			break;
		}
		if (count == 0) {
			break;
		}
		if (wait_event_interruptible(priv->wq,
					     CIRC_SPACE(mu->wbuf->head,
							mu->wbuf->tail,
							MAX_MU_BUF_SIZE))) {
			break;
		}
	}

	pr_debug("bytes written = %d\n", count);

	return orig_cnt - count;
}

/*!
 * This function starts a MCU->DSP transfer requested by a kernel module.
 * This transfer uses a MU channel.
 *
 * @param    mu       the MU channel structure
 *
 * @return              Zero
 */
static int mu_start_mcu_transfer_kernel(struct mu_channel *mu)
{
	pr_debug("writing bytes to DSP\n");
	mxc_mu_bind(mu->index, &mu_write_tasklet_kernel, TX);
	mxc_mu_intenable(mu->index, TX);

	return 0;
}

/*!
 * This function starts a DSP->MCU transfer requested by a kernel module.
 * This transfer uses a MU channel.
 *
 * @param    priv       Private data. Used to synchronize and to keep track of
 *                      the transfer
 * @param    count      number of bytes to be transferred
 * @param    buf        buffer containing the bytes to be transferred
 *
 * @return              Zero
 */
static int mu_start_dsp_transfer_kernel(struct ipc_priv_data *priv,
					int count, char *buf)
{
	struct mu_channel *mu = &priv->vc->ipc->ch.mu;

	if (count > MAX_MU_BUF_SIZE) {
		count = MAX_MU_BUF_SIZE;
	}

	priv->read_count = count;
	priv->vc->data = buf;

	mxc_mu_bind(mu->index, &mu_read_tasklet_kernel, RX);
	mxc_mu_intenable(mu->index, RX);

	return 0;
}

/*!
 * This function is called by the SDMA's ISR whenever a MCU->DSP
 * transfer has finished. This callback is used whenever a
 * kernel module requests a transfer.
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_kernel_writecb(void *args)
{
	dma_request_t sdma_request;
	struct ipc_priv_data *priv = NULL;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	mxc_dma_get_config(sdma->write_channel, &sdma_request, 0);

	priv->write_count = sdma_request.count;

	atomic_set(&priv->vc->state, CHANNEL_OPEN);

	tasklet_schedule(&sdma->write_tasklet);
	pr_debug("Called SDMA write callback %d\n", priv->write_count);
}

/*!
 * This function is called by the SDMA's ISR whenever a MCU->DSP
 * transfer has finished. This callback is used whenever a
 * kernel module requests a transfer.
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_kernel_writecb_ipcv2(void *args)
{
	struct ipc_priv_data *priv = NULL;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	priv->write_count = 0;

	atomic_set(&priv->vc->write_state_ipcv2, CHANNEL_OPEN);

	tasklet_schedule(&sdma->write_tasklet);
	pr_debug("Called SDMA write callback\n");
}

/*!
 * This function starts a MCU->DSP transfer requested by a kernel module.
 * This transfer uses a SDMA channel.
 *
 * @param    ipc_chn    IPC channel data structure
 * @param    count      number of bytes to be transferred
 *
 * @return              Zero
 */
static int sdma_start_mcu_transfer_kernel(struct ipc_channel *ipc_chn,
					  int count)
{
	dma_request_t sdma_request;
	struct sdma_channel *sdma = NULL;

	sdma = &ipc_chn->ch.sdma;
	memset(&sdma_request, 0, sizeof(dma_request_t));

	pr_debug("writing %d bytes to DSP\n", count);

	sdma_request.sourceAddr = (__u8 *) sdma->wpaddr;
	sdma_request.count = count;
	sdma_request.bd_cont = 1;
	mxc_dma_set_config(sdma->write_channel, &sdma_request, sdma->wbuf_head);
	sdma->wbuf_head++;
	if (sdma->wbuf_head >= sdma->wbuf_tail) {
		sdma->wbuf_head = 0;
	}

	return 0;
}

/*!
 * This function is called by the SDMA's ISR whenever a MCU->DSP
 * transfer has finished. This callback is used whenever a
 * user space program requests a transfer.
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_user_writecb(void *args)
{
	dma_request_t sdma_request;
	struct ipc_priv_data *priv = NULL;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	mxc_dma_get_config(sdma->write_channel, &sdma_request, 0);

	pr_debug("Called SDMA write callback %d\n", sdma_request.count);
	atomic_set(&priv->vc->state, CHANNEL_OPEN);
	if (waitqueue_active(&priv->wq)) {
		wake_up_interruptible(&priv->wq);
	}
}

/*!
 * This function starts a MCU->DSP transfer requested by a user space
 * program. This transfer uses a SDMA channel.
 *
 * @param    priv       Private data. Useful to synchronize and to keep track of
 *                      the transfer
 * @param    count      number of bytes to be transferred
 * @param    buf        buffer containing the bytes to be transferred
 *
 * @return              Zero
 */
static int sdma_start_mcu_transfer(struct ipc_priv_data *priv,
				   int count, const char *buf)
{
	dma_request_t sdma_request;
	struct sdma_channel *sdma = NULL;
	int result = 0;

	sdma = &priv->vc->ipc->ch.sdma;

	/* Check if the Channel is in use */
	while ((atomic_read(&priv->vc->state)) == CHANNEL_WRITE_ONGOING) {
		pr_debug("Write Block till channel is available\n");
		if (wait_event_interruptible(priv->wq,
					     (atomic_read(&priv->vc->state) !=
					      CHANNEL_WRITE_ONGOING))) {
			return -ERESTARTSYS;
		}
	}

	result = copy_from_user(sdma->wbuf, buf, count);
	if (result != 0) {
		kfree(sdma->wbuf);
		return -EFAULT;
	}

	atomic_set(&priv->vc->state, CHANNEL_WRITE_ONGOING);
	memset(&sdma_request, 0, sizeof(dma_request_t));

	pr_debug("writing %d bytes to SDMA channel %d\n", count,
		 sdma->write_channel);

	sdma_request.sourceAddr = (__u8 *) sdma->wpaddr;
	sdma_request.count = count;

	mxc_dma_set_callback(sdma->write_channel, sdma_user_writecb, priv);
	mxc_dma_set_config(sdma->write_channel, &sdma_request, 0);
	mxc_dma_start(sdma->write_channel);

	return count;
}

/*!
 * This function is called by the SDMA ISR whenever a DSP->MCU
 * transfer has finished. This callback is used whenever a
 * kernel module requests a transfer.
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_kernel_readcb(void *args)
{
	dma_request_t sdma_request;
	struct ipc_priv_data *priv;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	mxc_dma_get_config(sdma->read_channel, &sdma_request, 0);

	priv->read_count = sdma_request.count;

	atomic_set(&priv->vc->state, CHANNEL_OPEN);

	tasklet_schedule(&sdma->read_tasklet);
	pr_debug("Called SDMA read callback %d\n", priv->read_count);
}

/*!
 * This function is called by the SDMA ISR whenever a DSP->MCU
 * transfer has finished. This callback is used whenever a
 * kernel module requests a transfer.
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_kernel_readcb_ipcv2(void *args)
{
	struct ipc_priv_data *priv;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	priv->read_count = 0;

	atomic_set(&priv->vc->read_state_ipcv2, CHANNEL_OPEN);

	tasklet_schedule(&sdma->read_tasklet);
	pr_debug("Called SDMA read callback\n");
}

/*!
 * This function starts a DSP->MCU transfer requested by a kernel module.
 * This transfer uses a SDMA channel.
 *
 * @param    priv       Private data. Used to synchronize and to keep track of
 *                      the transfer
 * @param    count      number of bytes to be transferred
 * @param    buf        buffer containing the bytes to be transferred
 *
 * @return              Zero
 */
static int sdma_start_dsp_transfer_kernel(struct ipc_priv_data *priv,
					  int count, char *buf)
{
	dma_request_t sdma_request;
	struct sdma_channel *sdma = NULL;

	sdma = &priv->vc->ipc->ch.sdma;

	priv->vc->data = buf;

	memset(&sdma_request, 0, sizeof(dma_request_t));

	pr_debug("writing %d bytes to DSP\n", count);

	sdma_request.destAddr = buf;
	sdma_request.count = count;

	mxc_dma_set_callback(sdma->read_channel, sdma_kernel_readcb, priv);
	mxc_dma_set_config(sdma->read_channel, &sdma_request, sdma->rbuf_head);
	sdma->rbuf_head++;
	if (sdma->rbuf_head >= sdma->rbuf_tail) {
		sdma->rbuf_head = 0;
	}
	mxc_dma_start(sdma->read_channel);

	return 0;
}

/*!
 * This function is called by the SDMA's ISR whenever a DSP->MCU
 * transfer has finished. This callback fills the SDMA channels read buffer
 *
 * @param    args       points to a priv structure. This structure contains
 *                      fields to synchronize the IPC channel and the top half
 *
 * @return              None
 */
static void sdma_user_readcb(void *args)
{
	dma_request_t sdma_request;
	struct ipc_priv_data *priv = NULL;
	struct sdma_channel *sdma = NULL;

	priv = (struct ipc_priv_data *)args;
	sdma = &priv->vc->ipc->ch.sdma;

	memset(&sdma_request, 0, sizeof(dma_request_t));
	mxc_dma_get_config(sdma->read_channel, &sdma_request, sdma->rbuf_head);

	while ((sdma_request.bd_done == 0)
	       && (sdma->rbuf[sdma->rbuf_head].count == 0)) {
		pr_debug("buf head=%d, cnt=%d\n", sdma->rbuf_head,
			 sdma_request.count);
		sdma->rbuf[sdma->rbuf_head].count = sdma_request.count;
		sdma->rbuf_head = (sdma->rbuf_head + 1) & (NUM_RX_SDMA_BUF - 1);
		memset(&sdma_request, 0, sizeof(dma_request_t));
		mxc_dma_get_config(sdma->read_channel, &sdma_request,
				   sdma->rbuf_head);
	}

	/* Data arrived in read chnl buffers, wake up any pending processes */
	if (waitqueue_active(&priv->rq)) {
		wake_up_interruptible(&priv->rq);
	}
}

/*!
 * Copy bytes from the SDMA read buffers to the user buffer
 *
 * @param   sdma         pointer to the sdma structure
 * @param   buf          buffer to store data
 * @param   count        number of bytes to copy
 *
 * @return  number of bytes copied into the user buffer
 */
static int mxc_ipc_copy_from_sdmabuf(struct sdma_channel *sdma, char *buf,
				     int count)
{
	int n = 0;
	int amt_cpy = 0;
	int offset = 0;
	int ret = 0;
	char *tmp_buf = NULL;
	dma_request_t sdma_request;
	int result;

	while ((sdma->rbuf[sdma->rbuf_tail].count > 0) && (count > 0)) {
		n = sdma->rbuf[sdma->rbuf_tail].count;
		offset = sdma->rbuf[sdma->rbuf_tail].offset;
		tmp_buf = sdma->rbuf[sdma->rbuf_tail].buf + offset;

		/* Use the minimum of the 2 counts to copy data */
		amt_cpy = (count < n) ? count : n;

		result = copy_to_user(buf, tmp_buf, amt_cpy);

		/* Check if there was problem copying data */
		if (result != 0) {
			pr_debug("problem copying into user's buffer\n");
			break;
		}
		pr_debug("copied %d bytes into user's buffer\n", amt_cpy);
		pr_debug("Buf Tail=%d\n", sdma->rbuf_tail);
		/* Check if we emptied the SDMA read buffer */
		if (count < n) {
			sdma->rbuf[sdma->rbuf_tail].count -= amt_cpy;
			sdma->rbuf[sdma->rbuf_tail].offset += amt_cpy;
		} else {
			sdma->rbuf[sdma->rbuf_tail].count = 0;
			sdma->rbuf[sdma->rbuf_tail].offset = 0;
			memset(&sdma_request, 0, sizeof(dma_request_t));
			sdma_request.count = SDMA_RX_BUF_SIZE;
			sdma_request.destAddr =
			    (__u8 *) sdma->rbuf[sdma->rbuf_tail].rpaddr;
			sdma_request.bd_cont = 1;
			result = mxc_dma_set_config(sdma->read_channel,
						    &sdma_request,
						    sdma->rbuf_tail);
			sdma->rbuf_tail =
			    (sdma->rbuf_tail + 1) & (NUM_RX_SDMA_BUF - 1);

			/* Restart Sdma channel after data copy */

			mxc_dma_start(sdma->read_channel);
		}
		buf += amt_cpy;
		count -= amt_cpy;
		ret += amt_cpy;
	}

	return ret;
}

/*!
 * This function allocates a virtual channel.
 *
 * @param index         index of virtual channel to be open
 * @param config        pointer to a structure containing the type
 *                      of channel to open.
 *
 * @return              0 on success, negative value otherwise
 */
static int allocate_virtual_channel(unsigned short index,
				    const HW_CTRL_IPC_OPEN_T * config)
{
	struct kernel_callbacks *k_callbacks;
	write_callback_t wcallback = NULL;
	struct ipc_priv_data *priv;
	unsigned short pchannel;
	int ret = 0, sdma_chnl = -1;

	priv = (struct ipc_priv_data *)
	    kmalloc(sizeof(struct ipc_priv_data), GFP_KERNEL);
	if (priv == NULL) {
		return -1;
	}

	k_callbacks = (struct kernel_callbacks *)
	    kmalloc(sizeof(struct kernel_callbacks), GFP_KERNEL);
	if (k_callbacks == NULL) {
		kfree(priv);
		return -1;
	}

	wcallback = config->write_callback;
	priv->vc = &virtual_channels[index];

	switch (config->type) {
	case HW_CTRL_IPC_SHORT_MSG:
		if (config->index > IPC_MAX_MU_CHANNEL_INDEX) {
			kfree(priv);
			kfree(k_callbacks);
			return -1;
		}

		pchannel = config->index;
		priv->vc->ipc = &ipc_channels[pchannel];
		ret = allocate_mu_channel(pchannel);
		if (ret < 0) {
			kfree(priv);
			kfree(k_callbacks);
			return ret;
		}
		break;
	case HW_CTRL_IPC_PACKET_DATA:
		if (config->index > IPC_MAX_SDMA_BIDI_CHANNEL_INDEX) {
			kfree(priv);
			kfree(k_callbacks);
			return -1;
		}
		if (config->index == 0) {
			pchannel = PACKET_DATA_CHANNEL0;
			sdma_chnl = PACKET_DATA0_SDMA_WR_CHNL;
		} else {
			pchannel = PACKET_DATA_CHANNEL1;
			sdma_chnl = PACKET_DATA1_SDMA_WR_CHNL;
		}
		priv->vc->ipc = &ipc_channels[pchannel];
		ret = allocate_sdma_channel(pchannel, sdma_chnl, IPC_RDWR);
		if (ret == 0) {
			ret =
			    initialize_sdma_write_channel(priv->vc->ipc, 1,
							  IPC_DEFAULT_MAX_CTRL_STRUCT_NUM);
		}
		if (ret == 0) {
			ret =
			    initialize_sdma_read_channel_kernel(priv->vc->ipc,
								IPC_DEFAULT_MAX_CTRL_STRUCT_NUM);
		}
		if (ret < 0) {
			deallocate_sdma_channel(pchannel);
			kfree(priv);
			kfree(k_callbacks);
			return ret;
		}
		break;
	case HW_CTRL_IPC_CHANNEL_LOG:
		if (config->index > IPC_MAX_SDMA_MONO_CHANNEL_INDEX) {
			kfree(priv);
			kfree(k_callbacks);
			return -1;
		}

		if (config->index == 0) {
			pchannel = LOGGING_CHANNEL0;
			sdma_chnl = LOG0_SDMA_RD_CHNL;
		} else if (config->index == 1) {
			pchannel = LOGGING_CHANNEL1;
			sdma_chnl = LOG1_SDMA_RD_CHNL;
		} else if (config->index == 2) {
			pchannel = LOGGING_CHANNEL2;
			sdma_chnl = LOG2_SDMA_RD_CHNL;
		} else {
			pchannel = LOGGING_CHANNEL3;
			sdma_chnl = LOG3_SDMA_RD_CHNL;
		}
		priv->vc->ipc = &ipc_channels[pchannel];
		ret = allocate_sdma_channel(pchannel, sdma_chnl, IPC_READ);
		if (ret == 0) {
			ret =
			    initialize_sdma_read_channel_kernel(priv->vc->ipc,
								IPC_DEFAULT_MAX_CTRL_STRUCT_NUM);
		}
		if (ret < 0) {
			deallocate_sdma_channel(pchannel);
			kfree(priv);
			kfree(k_callbacks);
			return ret;
		}
		wcallback = NULL;
		break;
	default:
		kfree(priv);
		kfree(k_callbacks);
		return -1;
	}

	k_callbacks->write = wcallback;
	k_callbacks->read = config->read_callback;
	k_callbacks->notify = config->notify_callback;

	/* always non-blocking mode for kernel modules */
	atomic_set(&priv->blockmode, 0);
	priv->vchannel = index;
	priv->pchannel = pchannel;
	priv->read_count = 0;
	priv->write_count = 0;
	priv->vc->ipc->priv = (void *)priv;
	priv->k_callbacks = k_callbacks;

	return 0;
}

/*!
 * Verify that the handler points to the correct handler in the handler array
 * if not, that mean either the passed handler is NULL, either it is not a correct handler
 *
 * @param channel       handler to the virtual channel
 *
 * @return              1 when handler valid, 0 otherwise
 */

static int is_channel_handler_valid(HW_CTRL_IPC_CHANNEL_T * channel)
{
	if (channel != NULL) {
		if (channel != &channel_handlers[channel->channel_nb]) {
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

static void mxc_ipc_sdma_readtasklet(unsigned long arg)
{
	struct ipc_priv_data *priv = (struct ipc_priv_data *)arg;

	if (priv->k_callbacks->read != NULL) {
		HW_CTRL_IPC_READ_STATUS_T status;

		status.channel = &channel_handlers[priv->vchannel];
		status.nb_bytes = priv->read_count;

		priv->k_callbacks->read(&status);
	}
}

static void mxc_ipc_sdma_writetasklet(unsigned long arg)
{
	struct ipc_priv_data *priv = (struct ipc_priv_data *)arg;

	if (priv->k_callbacks->write != NULL) {
		HW_CTRL_IPC_WRITE_STATUS_T status;

		status.channel = &channel_handlers[priv->vchannel];
		status.nb_bytes = priv->write_count;

		priv->k_callbacks->write(&status);
	}
}

/*!
 * Opens an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param config        Pointer to a struct containing configuration para
 *                      meters for the channel to open (type of channel,
 *                      callbacks, etc)
 *
 * @return              returns a virtual channel handler on success, a NULL
 *                      pointer otherwise.
 */
HW_CTRL_IPC_CHANNEL_T *hw_ctrl_ipc_open(const HW_CTRL_IPC_OPEN_T * config)
{
	struct virtual_channel *v;
	int channel_nb = 0;
	int status = 0;
	HW_CTRL_IPC_CHANNEL_T *channel;

	/* return a NULL handler if something goes wrong */
	channel = NULL;

	if (config == NULL) {
		return NULL;
	}

	/* Look for an empty virtual channel, if found, lock it */
	channel_nb = get_free_virtual_channel();

	/* if free virtual channel not found, return NULL handler */
	if (channel_nb == IPC_DUMMY_CHANNEL) {
		return NULL;
	}

	status = allocate_virtual_channel(channel_nb, config);
	if (status < 0) {
		goto cleanup;
	}

	v = &virtual_channels[channel_nb];
	atomic_set(&v->state, CHANNEL_OPEN);
	/*! set state to open for read-while-write IPcv2 functionality */
	atomic_set(&v->read_state_ipcv2, CHANNEL_OPEN);
	atomic_set(&v->write_state_ipcv2, CHANNEL_OPEN);

	channel = &channel_handlers[channel_nb];

	/*
	 * set the channel_nb in the handler struct. This allows to
	 * find back the corresponding index
	 * in both channel_handlers and virtual_channels arrays
	 */
	channel->channel_nb = channel_nb;
	if (v->ipc->priv->pchannel > 3) {
		struct sdma_channel *sdma_chnl;
		sdma_chnl = &v->ipc->ch.sdma;

		sdma_chnl->rbuf_head = 0;
		sdma_chnl->rbuf_tail = IPC_DEFAULT_MAX_CTRL_STRUCT_NUM;
		sdma_chnl->wbuf_head = 0;
		sdma_chnl->wbuf_tail = IPC_DEFAULT_MAX_CTRL_STRUCT_NUM;

		tasklet_init(&sdma_chnl->read_tasklet,
			     mxc_ipc_sdma_readtasklet,
			     (unsigned long)v->ipc->priv);

		tasklet_init(&sdma_chnl->write_tasklet,
			     mxc_ipc_sdma_writetasklet,
			     (unsigned long)v->ipc->priv);
	}

	pr_debug("Virtual channel %d opened\n", channel_nb);

      cleanup:
	unlock_virtual_channel(channel_nb);

	return channel;
}

/*!
 * Close an IPC link. This functions can be called directly by kernel
 * modules.
 *
 * @param channel       handler to the virtual channel to close.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_close(HW_CTRL_IPC_CHANNEL_T * channel)
{
	struct virtual_channel *v;
	int channel_nb;
	if (is_channel_handler_valid(channel) == 0) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	channel_nb = channel->channel_nb;
	v = &virtual_channels[channel_nb];
	if (atomic_read(&v->state) == CHANNEL_CLOSED ||
	    atomic_read(&v->read_state_ipcv2) == CHANNEL_CLOSED ||
	    atomic_read(&v->write_state_ipcv2) == CHANNEL_CLOSED) {
		return HW_CTRL_IPC_STATUS_OK;
	}
	if (lock_virtual_channel(channel_nb)) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}
	kfree(v->ipc->priv->k_callbacks);
	v->ipc->priv->k_callbacks = NULL;

	if (v->ipc->priv->pchannel > 3) {
		deallocate_sdma_channel(v->ipc->priv->pchannel);
	} else {
		deallocate_mu_channel(v->ipc->priv->pchannel);
	}
	kfree(v->ipc->priv);
	v->ipc->priv = NULL;
	v->ipc = NULL;
	unlock_virtual_channel(channel_nb);
	atomic_set(&v->state, CHANNEL_CLOSED);
	atomic_set(&v->read_state_ipcv2, CHANNEL_CLOSED);
	atomic_set(&v->write_state_ipcv2, CHANNEL_CLOSED);

	pr_debug("Virtual channel %d closed\n", channel_nb);

	return HW_CTRL_IPC_STATUS_OK;
}

/*!
 * Reads data from an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param channel       handler to the virtual channel where read has been requested
 * @param buf           physical address of DMA'able read buffer to store data read from
 *                      the channel.
 * @param nb_bytes      size of the buffer
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_read(HW_CTRL_IPC_CHANNEL_T * channel,
				      unsigned char *buf,
				      unsigned short nb_bytes)
{
	struct virtual_channel *v;
	int status = 0;
	int channel_nb;

	if (is_channel_handler_valid(channel) == 0) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	if (buf == NULL) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	channel_nb = channel->channel_nb;

	v = &virtual_channels[channel_nb];

	if (lock_virtual_channel(channel_nb)) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	status = atomic_read(&v->state);
	if (status == CHANNEL_READ_ONGOING) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_READ_ON_GOING;
	}

	if (status == CHANNEL_CLOSED) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
	}

	atomic_set(&v->state, CHANNEL_READ_ONGOING);

	v->ipc->priv->read_count = 0;
	if (v->ipc->priv->pchannel > 3) {
		struct sdma_channel *sdma_chnl;
		sdma_chnl = &v->ipc->ch.sdma;
		status =
		    sdma_start_dsp_transfer_kernel(v->ipc->priv, nb_bytes, buf);
	} else {
		status =
		    mu_start_dsp_transfer_kernel(v->ipc->priv, nb_bytes, buf);
	}

	unlock_virtual_channel(channel_nb);
	return (status == 0) ? HW_CTRL_IPC_STATUS_OK : HW_CTRL_IPC_STATUS_ERROR;
}

/*!
 * Writes data to an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param channel       handler to the virtual channel where read has been requested.
 * @param buf           physical address of DMA'able write buffer containing data to
 *                      be written on the channel.
 * @param nb_bytes      size of the buffer
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_write(HW_CTRL_IPC_CHANNEL_T * channel,
				       unsigned char *buf,
				       unsigned short nb_bytes)
{
	struct virtual_channel *v;
	struct sdma_channel *sdma_chnl;
	struct mu_channel *mu_chnl;
	int status = 0;
	int channel_nb;

	if (is_channel_handler_valid(channel) == 0) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	if (buf == NULL) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	channel_nb = channel->channel_nb;
	v = &virtual_channels[channel_nb];

	if (lock_virtual_channel(channel_nb)) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	status = atomic_read(&v->state);

	if (status == CHANNEL_CLOSED) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
	}

	if (status == CHANNEL_WRITE_ONGOING) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_WRITE_ON_GOING;
	}

	atomic_set(&v->state, CHANNEL_WRITE_ONGOING);

	v->ipc->priv->write_count = 0;

	if (v->ipc->priv->pchannel > 3) {
		sdma_chnl = &v->ipc->ch.sdma;
		sdma_chnl->wpaddr = (dma_addr_t) buf;
		status = sdma_start_mcu_transfer_kernel(v->ipc, nb_bytes);
		mxc_dma_set_callback(sdma_chnl->write_channel,
				     sdma_kernel_writecb, v->ipc->priv);
		mxc_dma_start(sdma_chnl->write_channel);
	} else {
		mu_chnl = &v->ipc->ch.mu;
		v->ipc->priv->vc->data = buf;
		mu_chnl->bytes_written = IPC_WRITE_BYTE_INIT_VAL;
		v->ipc->priv->write_count = nb_bytes;
		status = mu_start_mcu_transfer_kernel(mu_chnl);
	}

	pr_debug("transferred %d bytes on virtual channel %d\n", nb_bytes,
		 channel_nb);

	unlock_virtual_channel(channel_nb);
	return (status == 0) ? HW_CTRL_IPC_STATUS_OK : HW_CTRL_IPC_STATUS_ERROR;
}

/*!
 * Writes data to an IPC link. This function can be called directly by kernel
 * modules. It accepts a linked list or contiguous data.
 *
 * @param channel       handler to the virtual channel where read has
 *                      been requested.
 * @param mem_ptr       pointer of type HW_CTRL_IPC_WRITE_PARAMS_T. Each element
 *                      points to the physical address of a DMA'able buffer
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_write_ex(HW_CTRL_IPC_CHANNEL_T * channel,
					  HW_CTRL_IPC_WRITE_PARAMS_T * mem_ptr)
{
	struct virtual_channel *v;
	struct sdma_channel *sdma_chnl = NULL;
	int status = 0;
	int channel_nb;
	HW_CTRL_IPC_LINKED_LIST_T *elt_ptr;

	if (is_channel_handler_valid(channel) == 0) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	channel_nb = channel->channel_nb;

	v = &virtual_channels[channel_nb];
	if (lock_virtual_channel(channel_nb)) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	status = atomic_read(&v->state);
	if (status == CHANNEL_CLOSED) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
	}
	if (status == CHANNEL_WRITE_ONGOING) {
		unlock_virtual_channel(channel_nb);
		return HW_CTRL_IPC_STATUS_WRITE_ON_GOING;
	}
	atomic_set(&v->state, CHANNEL_WRITE_ONGOING);

	sdma_chnl = &v->ipc->ch.sdma;
	v->ipc->priv->write_count = 0;

	if (mem_ptr->ipc_memory_read_mode == HW_CTRL_IPC_MODE_CONTIGUOUS) {
		sdma_chnl->wpaddr =
		    (dma_addr_t) mem_ptr->read.cont_ptr->data_ptr;
		status =
		    sdma_start_mcu_transfer_kernel(v->ipc,
						   mem_ptr->read.cont_ptr->
						   length);
	} else {
		elt_ptr = mem_ptr->read.list_ptr;
		while (elt_ptr != NULL) {
			sdma_chnl->wpaddr = (dma_addr_t) elt_ptr->data_ptr;
			status =
			    sdma_start_mcu_transfer_kernel(v->ipc,
							   elt_ptr->length);
			elt_ptr = elt_ptr->next;
		}
	}
	mxc_dma_set_callback(sdma_chnl->write_channel, sdma_kernel_writecb,
			     v->ipc->priv);
	mxc_dma_start(sdma_chnl->write_channel);
	pr_debug("transferred bytes on virtual channel %d\n", channel_nb);

	unlock_virtual_channel(channel_nb);
	return (status == 0) ? HW_CTRL_IPC_STATUS_OK : HW_CTRL_IPC_STATUS_ERROR;
}

/*!
 * Used to set various channel parameters
 *
 * @param channel handler to the virtual channel where read has
 *                been requested.
 * @param action  IPC driver control action to perform.
 * @param param   parameters required to complete the requested action
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_ioctl(HW_CTRL_IPC_CHANNEL_T * channel,
				       HW_CTRL_IPC_IOCTL_ACTION_T action,
				       void *param)
{
	struct virtual_channel *v;
	struct sdma_channel *sdma_chnl = NULL;

	if (is_channel_handler_valid(channel) == 0) {
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	v = &virtual_channels[channel->channel_nb];
	switch (action) {
	case HW_CTRL_IPC_SET_READ_CALLBACK:
		v->ipc->priv->k_callbacks->read = param;
		break;
	case HW_CTRL_IPC_SET_WRITE_CALLBACK:
		v->ipc->priv->k_callbacks->write = param;
		break;
	case HW_CTRL_IPC_SET_NOTIFY_CALLBACK:
		v->ipc->priv->k_callbacks->notify = param;
		break;
	case HW_CTRL_IPC_SET_MAX_CTRL_STRUCT_NB:
		sdma_chnl = &v->ipc->ch.sdma;
		initialize_sdma_read_channel_kernel(v->ipc, (int)param);
		/* Reinitialize the buff pointers to start at zero BD */
		sdma_chnl->rbuf_tail = (int)param;
		sdma_chnl->rbuf_head = 0;
		if (sdma_chnl->write_channel != -1) {
			initialize_sdma_write_channel(v->ipc, 1, (int)param);
			sdma_chnl->wbuf_tail = (int)param;
			sdma_chnl->wbuf_head = 0;
		}
		break;
	default:
		return HW_CTRL_IPC_STATUS_ERROR;
	}

	return HW_CTRL_IPC_STATUS_OK;
}

/*!
 * This function is a variant on the write() function, and is used to send a
 * group of frames made of various pieces each to the IPC driver.
 * It is mandatory to allow high throughput on IPC while minimizing the time
 * spent in the drivers / interrupts.
 *
 * @param channel       handler to the virtual channel where read has
 *                      been requested.
 * @param ctrl_ptr      Pointer on the control structure.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_write_ex2(HW_CTRL_IPC_CHANNEL_T * channel,
					   HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *
					   ctrl_ptr)
{
	int status = 0;
	struct virtual_channel *v;
	struct sdma_channel *sdma_chnl = NULL;

	int channel_nb;

	channel_nb = channel->channel_nb;

	v = &virtual_channels[channel_nb];

	status = atomic_read(&v->write_state_ipcv2);
	if (status == CHANNEL_WRITE_ONGOING) {

		return HW_CTRL_IPC_STATUS_WRITE_ON_GOING;
	}

	if (status == CHANNEL_CLOSED) {

		return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
	}

	atomic_set(&v->write_state_ipcv2, CHANNEL_WRITE_ONGOING);

	v = &virtual_channels[channel->channel_nb];
	sdma_chnl = &v->ipc->ch.sdma;
	mxc_dma_set_callback(sdma_chnl->write_channel,
			     sdma_kernel_writecb_ipcv2, v->ipc->priv);
	status = mxc_sdma_write_ipcv2(sdma_chnl->write_channel, ctrl_ptr);
	return (status == 0) ? HW_CTRL_IPC_STATUS_OK : HW_CTRL_IPC_STATUS_ERROR;
}

/*!
 * This function is used to give a set of buffers to the IPC and enable data
 * transfers.
 *
 * @param channel       handler to the virtual channel where read has
 *                      been requested.
 * @param ctrl_ptr      Pointer on the control structure.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_read_ex2(HW_CTRL_IPC_CHANNEL_T * channel,
					  HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *
					  ctrl_ptr)
{
	int status = 0;
	struct virtual_channel *v;
	struct sdma_channel *sdma_chnl = NULL;

	int channel_nb;

	channel_nb = channel->channel_nb;

	v = &virtual_channels[channel_nb];

	status = atomic_read(&v->read_state_ipcv2);
	if (status == CHANNEL_READ_ONGOING) {

		return HW_CTRL_IPC_STATUS_READ_ON_GOING;
	}

	if (status == CHANNEL_CLOSED) {
		return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
	}

	atomic_set(&v->read_state_ipcv2, CHANNEL_READ_ONGOING);
	v = &virtual_channels[channel->channel_nb];
	sdma_chnl = &v->ipc->ch.sdma;
	mxc_dma_set_callback(sdma_chnl->read_channel, sdma_kernel_readcb_ipcv2,
			     v->ipc->priv);
	status = mxc_sdma_read_ipcv2(sdma_chnl->read_channel, ctrl_ptr);
	return (status == 0) ? HW_CTRL_IPC_STATUS_OK : HW_CTRL_IPC_STATUS_ERROR;
}

/*!
 * This function is called when an IPC channel is opened. This function does
 * the initialization of the device corresponding to the channel user want to
 * open.
 *
 * @param    inode    Pointer to device inode
 * @param    file     Pointer to device file structure
 *
 * @return    The function returns 0 on success, -1 otherwise
 */
static int mxc_ipc_open(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);
	struct ipc_priv_data *priv = NULL;
	unsigned short vch_index;
	int ret = 0, mode = IPC_READ, sdma_chnl = -1, ipc_chnl = 0;

	if (((minor == 4) || (minor == 5)) &&
	    (file->f_flags & (O_WRONLY | O_RDWR))) {
		return -ENXIO;
	}

	/*look for an empty virtual channel, if found, lock it */
	vch_index = get_free_virtual_channel();
	/*if free virtual channel not found, return ENODEV */
	if (vch_index == IPC_DUMMY_CHANNEL) {
		return -ENODEV;
	}

	priv = (struct ipc_priv_data *)
	    kmalloc(sizeof(struct ipc_priv_data), GFP_KERNEL);
	if (priv == NULL) {
		unlock_virtual_channel(vch_index);
		return -ENOMEM;
	}

	priv->vc = &virtual_channels[vch_index];
	if (minor < 3) {
		/* Allocate MU channel */
		ipc_chnl = minor;
		priv->vc->ipc = &ipc_channels[ipc_chnl];
		ret = allocate_mu_channel(ipc_chnl);
		if (ret < 0) {
			deallocate_mu_channel(ipc_chnl);
			kfree(priv);
			unlock_virtual_channel(vch_index);
			return ret;
		}
	} else {
		/* Allocate SDMA channel */
		if (minor == 3) {
			mode = IPC_RDWR;
			sdma_chnl = PACKET_DATA1_SDMA_WR_CHNL;
			ipc_chnl = PACKET_DATA_CHANNEL1;
		} else if (minor == 4) {
			mode = IPC_READ;
			sdma_chnl = LOG0_SDMA_RD_CHNL;
			ipc_chnl = LOGGING_CHANNEL0;
		} else if (minor == 5) {
			mode = IPC_READ;
			sdma_chnl = LOG1_SDMA_RD_CHNL;
			ipc_chnl = LOGGING_CHANNEL1;
		}
		ret = allocate_sdma_channel(ipc_chnl, sdma_chnl, mode);
		if (ret < 0) {
			deallocate_sdma_channel(ipc_chnl);
			kfree(priv);
			unlock_virtual_channel(vch_index);
			return ret;
		}
		priv->vc->ipc = &ipc_channels[ipc_chnl];
		if (mode & IPC_WRITE) {
			ret =
			    initialize_sdma_write_channel(priv->vc->ipc, 0, 1);
			if (ret < 0) {
				deallocate_sdma_channel(ipc_chnl);
				kfree(priv);
				unlock_virtual_channel(vch_index);
				return ret;
			}
		}
	}

	atomic_set(&priv->blockmode, ((file->f_flags & (O_NONBLOCK)) ? 0 : 1));
	priv->vchannel = vch_index;
	priv->pchannel = ipc_chnl;
	priv->owner = current;

	init_waitqueue_head(&priv->wq);
	init_waitqueue_head(&priv->rq);

	priv->vc->ipc->priv = (void *)priv;

	/* Start the SDMA read channel */
	if (minor > 2) {
		ret = initialize_sdma_read_channel_user(ipc_chnl);
		if (ret < 0) {
			deallocate_sdma_channel(ipc_chnl);
			mxc_ipc_free_readbuf_user(ipc_chnl);
			kfree(priv);
			unlock_virtual_channel(vch_index);
			return ret;
		}
	} else {
		/* Start the MU read channel */
		ret = mxc_ipc_enable_muints(ipc_chnl);
		if (ret < 0) {
			deallocate_mu_channel(ipc_chnl);
			kfree(priv);
			unlock_virtual_channel(vch_index);
			return ret;
		}
	}
	atomic_set(&priv->vc->state, CHANNEL_OPEN);

	file->private_data = (void *)priv;

	unlock_virtual_channel(priv->vchannel);

	pr_debug("Channel successfully opened\n");

	return 0;
}

/*!
 * This function is called to close an IPC channel.
 * Any allocated buffers are freed
 *
 * @param    inode    Pointer to device inode
 * @param    file     Pointer to device file structure
 *
 * @return    The function returns 0 on success or
 *            returns NO_ACCESS if incorrect channel is accessed.
 */
static int mxc_ipc_close(struct inode *inode, struct file *file)
{
	struct ipc_priv_data *priv = NULL;
	unsigned int minor;
	int do_sleep = 0;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);
	priv = (struct ipc_priv_data *)file->private_data;

	if (lock_virtual_channel(priv->vchannel)) {
		return -EINTR;
	}

	if (minor < 3) {
		struct mu_channel *mu = NULL;
		mu = &priv->vc->ipc->ch.mu;

		/* Wait to complete any pending writes */
		/* Block until the write buffer is empty */
		while (CIRC_CNT(mu->wbuf->head, mu->wbuf->tail, MAX_MU_BUF_SIZE)
		       != 0) {
			if (wait_event_interruptible
			    (priv->wq,
			     (CIRC_CNT
			      (mu->wbuf->head, mu->wbuf->tail,
			       MAX_MU_BUF_SIZE) == 0))) {
				return -ERESTARTSYS;
			}
		}
		deallocate_mu_channel(priv->pchannel);
	} else {
		struct sdma_channel *sdma = NULL;
		sdma = &priv->vc->ipc->ch.sdma;

		/* Wait to complete any pending writes */
		/* Block until the write buffer is empty */
		while (atomic_read(&priv->vc->state) == CHANNEL_WRITE_ONGOING) {
			if (wait_event_interruptible(priv->wq,
						     (atomic_read
						      (&priv->vc->state) !=
						      CHANNEL_WRITE_ONGOING))) {
				return -ERESTARTSYS;
			}
		}
		deallocate_sdma_channel(priv->pchannel);
		mxc_ipc_free_readbuf_user(priv->pchannel);
	}

	/* Wake up all waiting processes on the wait queue */
	while (1) {
		do_sleep = 0;

		if (waitqueue_active(&priv->wq)) {
			wake_up(&priv->wq);
			do_sleep++;
		}
		if (waitqueue_active(&priv->rq)) {
			wake_up(&priv->rq);
			do_sleep++;
		}
		if (!do_sleep) {
			break;
		}
		schedule();
	}

	priv->vc->ipc = NULL;
	atomic_set(&priv->vc->state, CHANNEL_CLOSED);
	priv->vc = NULL;

	unlock_virtual_channel(priv->vchannel);

	kfree(file->private_data);
	file->private_data = NULL;

	pr_debug("Channel %d closed\n", minor);

	return 0;
}

/*!
 * The write function is available to the user-space to perform
 * a write operation
 *
 *  @param  file        Pointer to device file structure
 *  @param  buf         User buffer, where write data is placed.
 *  @param  bytes       Size of the requested data transfer
 *  @param  off         File position where the user is accessing.
 *
 *  @return             Returns number of bytes written or
 *                      Returns COUNT_ESIZE if the number of bytes
 *                      requested is not a multiple of channel
 *                      receive or transmit size or
 *                      Returns -EAGAIN for a non-block write when there is
 *                      no data in the buffer or
 *                      Returns -EFAULT if copy_to_user failed
 */
static ssize_t mxc_ipc_write(struct file *file, const char *buf,
			     size_t bytes, loff_t * off)
{
	struct ipc_priv_data *priv;
	unsigned int minor;
	int count = 0;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	if ((minor <= 2) && ((bytes % 4) != 0)) {
		pr_debug("EINVAL error\n");
		return -EINVAL;
	}

	if (((minor == 4) || (minor == 5)) &&
	    (file->f_flags & (O_WRONLY | O_RDWR))) {
		pr_debug("ENXIO error\n");
		return -ENXIO;	/*log channel is read-only */
	}

	priv = (struct ipc_priv_data *)file->private_data;

	if (lock_virtual_channel(priv->vchannel)) {
		return -EINTR;
	}

	pr_debug("bytes to write = %d\n", bytes);

	if (minor > 2) {
		if (atomic_read(&priv->vc->state) == CHANNEL_WRITE_ONGOING) {
			/* If non-blocking mode return */
			if (atomic_read(&priv->blockmode) == 0) {
				unlock_virtual_channel(priv->vchannel);
				return -EAGAIN;
			}
		}
		if (bytes > MAX_SDMA_TX_BUF_SIZE) {
			bytes = MAX_SDMA_TX_BUF_SIZE;
		}
		count = sdma_start_mcu_transfer(priv, bytes, buf);
	} else {
		count = mu_start_mcu_transfer(priv, bytes, buf);
	}

	pr_debug("bytes written = %d\n", count);

	unlock_virtual_channel(priv->vchannel);

	return count;
}

/*!
 * Check to see if there is data available in the internal buffers
 *
 * @param  minor   minor number used to differentiate between MU and
 *                 SDMA buffers
 * @param  priv    private data. Useful to synchronize and to keep track of
 *                 the transfer
 * @return returns 0 if no data available or a positive value when data is
 * available
 */
static int mxc_ipc_data_available(int minor, struct ipc_priv_data *priv)
{
	int cnt = 0;

	if (minor > 2) {
		struct sdma_channel *sdma = NULL;

		sdma = &priv->vc->ipc->ch.sdma;
		cnt = sdma->rbuf[sdma->rbuf_tail].count;
		pr_debug("bytes in SDMA read buf=%d\n", cnt);
	} else {
		struct mu_channel *mu = NULL;
		mu = &priv->vc->ipc->ch.mu;
		cnt = CIRC_CNT(mu->rbuf->head, mu->rbuf->tail, MAX_MU_BUF_SIZE);
		pr_debug("bytes in MU read buf = %d\n", cnt);
	}

	return cnt;
}

/*!
 * The read function is available to the user-space to perform
 * a read operation
 *
 *  @param  file        Pointer to device file structure
 *  @param  buf         User buffer, where read data to be placed.
 *  @param  bytes       Size of the requested data transfer
 *  @param  off         File position where the user is accessing.
 *
 *  @return             Returns number of bytes read or
 *                      Returns COUNT_ESIZE if the number of bytes
 *                      requested is not a multiple of channel
 *                      receive or transmit size or
 *                      Returns -EAGAIN for a non-block read when no data
 *                      is ready in the buffer or in the register or
 *                      Returns -EFAULT if copy_to_user failed
 */
static ssize_t mxc_ipc_read(struct file *file, char *buf,
			    size_t bytes, loff_t * off)
{
	struct ipc_priv_data *priv;
	unsigned int minor;
	int blockmode;
	int status;
	int count = 0;

	minor = MINOR(file->f_dentry->d_inode->i_rdev);

	if ((minor <= 2) && ((bytes % 4) != 0)) {
		return -EINVAL;
	}

	priv = (struct ipc_priv_data *)file->private_data;

	status = atomic_read(&priv->vc->state);
	blockmode = atomic_read(&priv->blockmode);

	/*
	 * If non-blocking mode has been requested and no data is available
	 * read channel buffers, return -EAGAIN.
	 */
	if ((blockmode == 0) && (!mxc_ipc_data_available(minor, priv))) {
		return -EAGAIN;
	}

	if (lock_virtual_channel(priv->vchannel)) {
		return -EINTR;
	}

	pr_debug("bytes to read = %d\n", bytes);

	/* Block till data is available */
	while ((mxc_ipc_data_available(minor, priv)) == 0) {
		pr_debug("No data to read, BLOCK\n");
		if (wait_event_interruptible(priv->rq,
					     (mxc_ipc_data_available
					      (minor, priv) > 0))) {
			unlock_virtual_channel(priv->vchannel);
			return -ERESTARTSYS;
		}
	}

	if (minor > 2) {
		struct sdma_channel *sdma = NULL;
		sdma = &priv->vc->ipc->ch.sdma;

		count = mxc_ipc_copy_from_sdmabuf(sdma, buf, bytes);
	} else {
		count = mu_start_dsp_transfer(priv, bytes, buf);
	}

	pr_debug("bytes read = %d\n", count);

	atomic_set(&priv->vc->state, CHANNEL_OPEN);
	unlock_virtual_channel(priv->vchannel);

	return count;
}

/*!
 * This function allows the caller to determine whether it can
 * read from or write to one or more open files without blocking.
 *
 * @param    filp       Pointer to device file structure
 * @param    wait       used by poll_wait to indicate a change in the
 *                      poll status.
 *
 * @return              the bit mask describing which operations could
 *                      be completed immediately.
 */
unsigned int mxc_ipc_poll(struct file *filp, poll_table * wait)
{
	struct ipc_priv_data *priv;
	unsigned int minor;
	unsigned int mask = 0;
	int state = 0;

	minor = MINOR(filp->f_dentry->d_inode->i_rdev);
	priv = (struct ipc_priv_data *)filp->private_data;

	poll_wait(filp, &priv->wq, wait);
	poll_wait(filp, &priv->rq, wait);

	if (minor > 2) {
		struct sdma_channel *sdma;
		sdma = &priv->vc->ipc->ch.sdma;
		state = atomic_read(&priv->vc->state);
		/* Check if SDMA Write channel is available */
		if (state == CHANNEL_OPEN) {
			mask |= POLLOUT | POLLWRNORM;
		}
		/* Check if data is available in SDMA channel read buffer */
		if ((mxc_ipc_data_available(minor, priv)) > 0) {
			mask |= POLLIN | POLLRDNORM;
		}
	} else {
		struct mu_channel *mu;
		mu = &priv->vc->ipc->ch.mu;
		/* Check for space in MU channel write buffer */
		if (CIRC_SPACE(mu->wbuf->head, mu->wbuf->tail, MAX_MU_BUF_SIZE)) {
			mask |= POLLOUT | POLLWRNORM;
		}

		/* Check if data is available in MU channel read buffer */
		if (CIRC_CNT(mu->rbuf->head, mu->rbuf->tail, MAX_MU_BUF_SIZE)) {
			mask |= POLLIN | POLLRDNORM;
		}
	}
	pr_debug("Poll mask = %d\n", mask);
	return mask;
}

static struct file_operations mxc_ipc_fops = {
	.owner = THIS_MODULE,
	.read = mxc_ipc_read,
	.write = mxc_ipc_write,
//      .writev = mxc_ipc_writev,
	.poll = mxc_ipc_poll,
	.open = mxc_ipc_open,
	.release = mxc_ipc_close,
};

/*!
 * This function is used to unload the module.
 */
static void ipc_cleanup_module(void)
{
	int i;
	for (i = 0; i <= 5; i++) {
		class_device_destroy(mxc_ipc_class, MKDEV(major_num, i));
	}
	class_destroy(mxc_ipc_class);
	unregister_chrdev(major_num, "mxc_ipc");

	pr_debug("IPC Driver Module Unloaded\n");
}

/*!
 * This function is used to load the module. All initializations and
 * resources requesting is done here
 *
 * @return   Returns 0 on success, -1 otherwise
 */
int __init ipc_init_module(void)
{
	int res = 0;
	int i;
	struct class_device *temp_class;

	res = register_chrdev(MXC_IPC_MAJOR, "mxc_ipc", &mxc_ipc_fops);
	if (res < 0) {
		pr_debug("IPC Driver Module was not Loaded successfully\n");
		return res;
	}

	major_num = res;

	mxc_ipc_class = class_create(THIS_MODULE, "mxc_ipc");
	if (IS_ERR(mxc_ipc_class)) {
		printk(KERN_ERR "Error creating mxc_ipc class.\n");
		unregister_chrdev(major_num, "mxc_ipc");
		return PTR_ERR(mxc_ipc_class);
	}

	for (i = 0; i <= 5; i++) {
		temp_class =
		    class_device_create(mxc_ipc_class, NULL,
					MKDEV(major_num, i),
					NULL, "mxc_ipc%u", i);

		if (IS_ERR(temp_class))
			goto err_out;
	}

	for (i = 0; i < IPC_MAX_VIRTUAL_CHANNELS; i++) {
		virtual_channels[i].ipc = NULL;
		sema_init(&virtual_channels[i].sem, 1);
		atomic_set(&virtual_channels[i].state, CHANNEL_CLOSED);
		/* Initialize the channel_handlers array to map the virtual channel indexes */
		channel_handlers[i].channel_nb = i;
	}

	printk(KERN_INFO "IPC driver successfully loaded.\n");
	return major_num;

      err_out:
	printk(KERN_ERR "Error creating mxc_ipc class device.\n");
	for (i = 0; i <= 5; i++) {
		class_device_destroy(mxc_ipc_class, MKDEV(major_num, i));
	}
	class_destroy(mxc_ipc_class);
	unregister_chrdev(major_num, "mxc_ipc");
	return -1;
}

module_init(ipc_init_module);
module_exit(ipc_cleanup_module);
EXPORT_SYMBOL(hw_ctrl_ipc_open);
EXPORT_SYMBOL(hw_ctrl_ipc_close);
EXPORT_SYMBOL(hw_ctrl_ipc_read);
EXPORT_SYMBOL(hw_ctrl_ipc_write);
EXPORT_SYMBOL(hw_ctrl_ipc_write_ex);
EXPORT_SYMBOL(hw_ctrl_ipc_write_ex2);
EXPORT_SYMBOL(hw_ctrl_ipc_read_ex2);
EXPORT_SYMBOL(hw_ctrl_ipc_ioctl);
