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
 * @file dptc_mx27.c
 *
 * @brief Driver for the Freescale Semiconductor MX27 DPTC module.
 *
 * The DPTC driver is designed as a character driver which interacts with the
 * MX27 DPTC hardware. Upon initialization, the DPTC driver initializes the
 * DPTC hardware sets up driver nodes attaches to the DPTC interrupt and
 * initializes internal data structures. When the DPTC interrupt occurs the
 * driver checks the cause of the interrupt (lower voltage, increase voltage or
 * emergency) and changes the CPU voltage according to translation table that
 * is loaded into the driver(the voltage changes are done by calling some
 * routines in the mc13783 driver). The driver read method is used to read the
 * currently loaded DPTC translation table and the write method is used
 * in-order to update the translation table. Driver ioctls are used to change
 * driver parameters and enable/disable the DPTC operation.
 *
 * @ingroup PM_MX27
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/arch/pmic_power.h>
#include <asm/arch/dvfs_dptc_struct.h>
#include "dvfs_dptc.h"

#define MX27_PMCR_BASE_ADDR          	(SYSCTRL_BASE_ADDR + 0x60)
#define MX27_DCVR_BASE_ADDR          	(SYSCTRL_BASE_ADDR + 0x64)
#define MX27_DCVR0_OFFSET               0
#define MX27_DCVR1_OFFSET               4
#define MX27_DCVR2_OFFSET               8
#define MX27_DCVR3_OFFSET               12

#define MX27_PMCR_MC_STATUS_OFFSET      31
#define MX27_PMCR_MC_STATUS_MASK        (1 << 31)
#define MX27_PMCR_EM_INTR_OFFSET        30
#define MX27_PMCR_EM_INTR_MASK          (1 << 30)
#define MX27_PMCR_UP_INTR_OFFSET        29
#define MX27_PMCR_UP_INTR_MASK          (1 << 29)
#define MX27_PMCR_LO_INTR_OFFSET        28
#define MX27_PMCR_LO_INTR_MASK          (1 << 28)
#define MX27_PMCR_REFCNT_OFFSET         16
#define MX27_PMCR_REFCNT_MASK           (0x7FF << 16)
#define MX27_PMCR_DCR_OFFSET            9
#define MX27_PMCR_DCR_MASK              (1 << 9)
#define MX27_PMCR_RCLKON_OFFSET         8
#define MX27_PMCR_RCLKON_MASK           (1 << 8)
#define MX27_PMCR_DRCE3_OFFSET          7
#define MX27_PMCR_DRCE3_MASK            (1 << 7)
#define MX27_PMCR_DRCE2_OFFSET          6
#define MX27_PMCR_DRCE2_MASK            (1 << 6)
#define MX27_PMCR_DRCE1_OFFSET          5
#define MX27_PMCR_DRCE1_MASK            (1 << 5)
#define MX27_PMCR_DRCE0_OFFSET          4
#define MX27_PMCR_DRCE0_MASK            (1 << 4)
#define MX27_PMCR_DIM_OFFSET            2
#define MX27_PMCR_DIM_MASK              (0x3 << 2)
#define MX27_PMCR_DIE_OFFSET            1
#define MX27_PMCR_DIE_MASK              (1 << 1)
#define MX27_PMCR_DPTEN_OFFSET          0
#define MX27_PMCR_DPTEN_MASK            (1 << 0)

/*!
 * DPTC proc file system entry name
 */
#define PROC_NODE_NAME	"dptc"

/*
 * Prototypes
 */
static int dptc_mx27_open(struct inode *inode, struct file *filp);
static int dptc_mx27_release(struct inode *inode, struct file *filp);
static int dptc_mx27_ioctl(struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long arg);
static irqreturn_t dptc_mx27_irq(int irq, void *dev_id);

/*
 * Global variables
 */

/*!
 * The dvfs_dptc_params structure holds all the internal DPTC driver parameters
 * (current working point, current frequency, translation table and DPTC
 * log buffer).
 */
static dvfs_dptc_params_s dptc_params;

static struct work_struct dptc_work;
static volatile u32 dptc_intr_status;
static u32 dptc_initial_wp;

/*!
 * Holds the automatically selected DPTC driver major number.
 */
static int major;

static struct class *mxc_dvfs_dptc_class;

/*
 * This mutex makes the Read,Write and IOCTL command mutual exclusive.
 */
DECLARE_MUTEX(access_mutex);

/*!
 * This structure contains pointers for device driver entry point.
 * The driver register function in init module will call this
 * structure.
 */
static struct file_operations fops = {
	.open = dptc_mx27_open,
	.release = dptc_mx27_release,
	.ioctl = dptc_mx27_ioctl,
};

static
void mx27_pmcr_modify_reg(u32 mask, u32 val)
{
	volatile u32 reg;
	reg = __raw_readl(IO_ADDRESS(MX27_PMCR_BASE_ADDR));
	reg = (reg & (~mask)) | val;
	__raw_writel(reg, IO_ADDRESS(MX27_PMCR_BASE_ADDR));
}

static
void mx27_dcvr_modify_reg(u32 offset, u32 val)
{
	__raw_writel(val, IO_ADDRESS(MX27_DCVR_BASE_ADDR) + offset);
}

/*!
 * Enable DPTC hardware
 */
static void dptc_enable_dptc(void)
{
	mx27_pmcr_modify_reg(MX27_PMCR_DPTEN_MASK, 1);
}

/*!
 * Disable DPTC hardware
 */
static void dptc_disable_dptc(void)
{
	mx27_pmcr_modify_reg(MX27_PMCR_DPTEN_MASK, 0);
}

/*!
 * Mask DPTC interrupt
 */
static void dptc_mask_dptc_int(void)
{
	mx27_pmcr_modify_reg(MX27_PMCR_DIE_MASK, (0 << MX27_PMCR_DIE_OFFSET));
}

/*!
 * Unmask DPTC interrupt
 */
static void dptc_unmask_dptc_int(void)
{
	mx27_pmcr_modify_reg(MX27_PMCR_DIE_MASK, (1 << MX27_PMCR_DIE_OFFSET));
}

/*!
 * Clear DCR bits of the CCM
 */
static void dptc_clear_dcr(void)
{
	mx27_pmcr_modify_reg(MX27_PMCR_DCR_MASK, (0 << MX27_PMCR_DCR_OFFSET));
}

/*!
 * This function enables the DPTC reference circuits.
 *
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to enable
 * @return   0 on success, error code on failure
 */
static int enable_ref_circuits(unsigned char rc_state)
{
	int ret_val;

	if (dptc_params.dptc_is_active == FALSE) {
		dptc_params.rc_state = rc_state;

		if (rc_state & 0x1) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE0_MASK,
					     (1 << MX27_PMCR_DRCE0_OFFSET));
		}
		if (rc_state & 0x2) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE1_MASK,
					     (1 << MX27_PMCR_DRCE1_OFFSET));
		}
		if (rc_state & 0x4) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE2_MASK,
					     (1 << MX27_PMCR_DRCE2_OFFSET));
		}
		if (rc_state & 0x8) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE3_MASK,
					     (1 << MX27_PMCR_DRCE3_OFFSET));
		}

		ret_val = 0;
	} else {
		ret_val = -EINVAL;
	}

	return ret_val;
}

/*!
 * This function disables the DPTC reference circuits.
 *
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to disable
 * @return   0 on success, error code on failure
 */
static int disable_ref_circuits(unsigned char rc_state)
{
	int ret_val;

	if (dptc_params.dptc_is_active == FALSE) {
		dptc_params.rc_state &= ~rc_state;

		if (rc_state & 0x1) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE0_MASK,
					     (0 << MX27_PMCR_DRCE0_OFFSET));
		}
		if (rc_state & 0x2) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE1_MASK,
					     (0 << MX27_PMCR_DRCE1_OFFSET));
		}
		if (rc_state & 0x4) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE2_MASK,
					     (0 << MX27_PMCR_DRCE2_OFFSET));
		}
		if (rc_state & 0x8) {
			mx27_pmcr_modify_reg(MX27_PMCR_DRCE3_MASK,
					     (0 << MX27_PMCR_DRCE3_OFFSET));
		}

		ret_val = 0;
	} else {
		ret_val = -EINVAL;
	}

	return ret_val;
}

/*!
 * This function updates the CPU voltage, produced by MC13783, by calling
 * MC13783 driver functions.
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 *
 */
static void set_pmic_voltage(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr, int wp)
{
	/* Call MC13783 functions */
	t_regulator_voltage volt;

	volt.sw1a = dvfs_dptc_tables_ptr->wp[wp].pmic_values[0];
	pmic_power_regulator_set_voltage(SW_SW1A, volt);
}

/*!
 * This function updates the DPTC threshold registers.
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 *
 */
static void update_dptc_thresholds(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr,
				   int wp)
{
	dcvr_state *dcvr;

	/* Calculate current table entry offset in the DPTC translation table */
	dcvr = &dvfs_dptc_tables_ptr->dcvr[0][wp];

	/* Update DPTC threshold registers */
	mx27_dcvr_modify_reg(MX27_DCVR0_OFFSET, dcvr->dcvr_reg[0].AsInt);
	mx27_dcvr_modify_reg(MX27_DCVR1_OFFSET, dcvr->dcvr_reg[1].AsInt);
	mx27_dcvr_modify_reg(MX27_DCVR2_OFFSET, dcvr->dcvr_reg[2].AsInt);
	mx27_dcvr_modify_reg(MX27_DCVR3_OFFSET, dcvr->dcvr_reg[3].AsInt);
}

/*!
 * This function increments a log buffer index (head or tail)
 * by the value of val.
 *
 * @param    index	pointer to the DPTC log buffer index that
 *			we wish to change.
 * @param    val	the value in which the index should be incremented.
 *
 */
static void inc_log_index(int *index, int val)
{
	*index = (*index + val) % LOG_ENTRIES;
}

/*!
 * This function adds a new entry to the DPTC log buffer.
 *
 * @param    dptc_log		pointer to the DPTC log buffer structure.
 * @param    wp			value of the working point index written
 *				to the log buffer.
 *
 * @return   number of log buffer entries.
 *
 */
static void add_dptc_log_entry(dptc_log_s * dptc_log, int wp)
{
	/*
	 * Down the log buffer mutex to exclude others from reading and
	 * writing to the log buffer.
	 */
	if (down_interruptible(&dptc_log->mutex)) {
		return;
	}

	/* Write values to log buffer */
	dptc_log->entries[dptc_log->head].jiffies = jiffies;
	dptc_log->entries[dptc_log->head].wp = wp;

	dptc_log->entries[dptc_log->head].voltage = wp;
	dptc_log->entries[dptc_log->head].freq = 0;

	/* Increment the head index by 1 */
	inc_log_index(&dptc_log->head, 1);
	/* If head index reaches the tail increment the tail by 1 */
	if (dptc_log->head == dptc_log->tail) {
		inc_log_index(&dptc_log->tail, 1);
	}

	/* Up the log buffer mutex to allow access to the log buffer */
	up(&dptc_log->mutex);
}

/*!
 * This function updates the drivers current working point index. This index is
 * used for access the current DTPC table entry and it corresponds to the
 * current CPU working point measured by the DPTC hardware.
 *
 * @param    new_wp	New working point index value to be set.
 *
 */
static void set_dptc_wp(int new_wp)
{
	/*
	 * Check if new index is smaller than the maximal working point
	 * index in the DPTC translation table and larger that 0.
	 */
	if ((new_wp < dptc_params.dvfs_dptc_tables_ptr->wp_num)
	    && (new_wp >= 0)) {
		/* Set current working point index to new index */
		dptc_params.dvfs_dptc_tables_ptr->curr_wp = new_wp;
	}

	/*
	 * Check if new index is larger than the maximal working point index in
	 * the DPTC translation table.
	 */
	if (new_wp >= dptc_params.dvfs_dptc_tables_ptr->wp_num) {
		/*
		 * Set current working point index to maximal working point
		 * index in the DPTC translation table.
		 */
		dptc_params.dvfs_dptc_tables_ptr->curr_wp =
		    dptc_params.dvfs_dptc_tables_ptr->wp_num - 1;
	}

	/* Check if new index is smaller than 0. */
	if (new_wp < 0) {
		/* Set current working point index to 0 (minimal value) */
		dptc_params.dvfs_dptc_tables_ptr->curr_wp = 0;
	}

	/* Update the DPTC hardware thresholds */
	update_dptc_thresholds(dptc_params.dvfs_dptc_tables_ptr,
			       dptc_params.dvfs_dptc_tables_ptr->curr_wp);

	/* Update the MC13783 voltage */
	set_pmic_voltage(dptc_params.dvfs_dptc_tables_ptr,
			 dptc_params.dvfs_dptc_tables_ptr->curr_wp);

	/* Write DPTC changes in the DPTC log buffer */
	add_dptc_log_entry(&dptc_params.dptc_log_buffer,
			   dptc_params.dvfs_dptc_tables_ptr->curr_wp);

}

static void dptc_workqueue_handler(struct work_struct *work)
{
	if (dptc_intr_status & 0x4) {
		/* Chip working point has increased dramatically,
		 * raise working point to maximum */
		set_dptc_wp(dptc_params.dvfs_dptc_tables_ptr->curr_wp - 2);
	} else if (dptc_intr_status & 0x2) {
		/* Chip working point has increased, raise working point
		 * by one */
		set_dptc_wp(dptc_params.dvfs_dptc_tables_ptr->curr_wp + 1);
	} else {
		/* Chip working point has decreased, lower working point
		 * by one */
		set_dptc_wp(dptc_params.dvfs_dptc_tables_ptr->curr_wp - 1);
	}

	/*
	 * If the DPTC module is still active, re-enable
	 * the DPTC hardware
	 */
	if (dptc_params.dptc_is_active) {
		dptc_enable_dptc();
		dptc_unmask_dptc_int();
	}
}

/*!
 * This function enables the DPTC module. this function updates the DPTC
 * thresholds, updates the MC13783, unmasks the DPTC interrupt and enables
 * the DPTC module
 *
 * @return      0 if DPTC module was enabled else returns -EINVAL.
 */
static int start_dptc(void)
{
	/* Check if DPTC module isn't already active */
	if (dptc_params.dptc_is_active == FALSE) {

		enable_ref_circuits(dptc_params.rc_state);
		disable_ref_circuits(~dptc_params.rc_state);

		/*
		 * Set the DPTC thresholds and MC13783 voltage to
		 * correspond to the current working point and frequency.
		 */
		set_pmic_voltage(dptc_params.dvfs_dptc_tables_ptr,
				 dptc_params.dvfs_dptc_tables_ptr->curr_wp);

		update_dptc_thresholds(dptc_params.dvfs_dptc_tables_ptr,
				       dptc_params.dvfs_dptc_tables_ptr->
				       curr_wp);

		/* Mark DPTC module as active */
		dptc_params.dptc_is_active = TRUE;

		/* Enable the DPTC module and unmask the DPTC interrupt */
		dptc_enable_dptc();
		dptc_unmask_dptc_int();

		return 0;
	}

	/* DPTC module already active return error */
	return -EINVAL;
}

/*!
 * This function disables the DPTC module.
 *
 * @return      0 if DPTC module was disabled else returns -EINVAL.
 */
static int stop_dptc(void)
{
	/* Check if DPTC module isn't already disabled */
	if (dptc_params.dptc_is_active != FALSE) {

		/* Disable the DPTC module and mask the DPTC interrupt */
		dptc_disable_dptc();
		dptc_mask_dptc_int();

		/* Set working point to default */
		set_dptc_wp(dptc_initial_wp);

		/* Mark DPCT module as inactive */
		dptc_params.dptc_is_active = FALSE;

		return 0;
	}

	/* DPTC module already disabled, return error */
	return -EINVAL;
}

static void init_dptc_wp(void)
{
	/* Call MC13783 functions */
	t_regulator_voltage volt;
	int i;

	/* Normal mode setting */
	pmic_power_regulator_get_voltage(SW_SW1A, &volt);
	for (i = 0; i < dptc_params.dvfs_dptc_tables_ptr->wp_num; i++) {
		if (volt.sw1a ==
		    dptc_params.dvfs_dptc_tables_ptr->wp[i].pmic_values[0]) {
			break;
		}
	}

	/*
	 * Check if new index is smaller than the maximal working point
	 * index in the DPTC translation table and larger that 0.
	 */
	if ((i < dptc_params.dvfs_dptc_tables_ptr->wp_num) && (i >= 0)) {
		/* Set current working point index to new index */
		dptc_params.dvfs_dptc_tables_ptr->curr_wp = i;
	}

	/*
	 * Check if new index is larger than the maximal working point index in
	 * the DPTC translation table.
	 */
	if (i >= dptc_params.dvfs_dptc_tables_ptr->wp_num) {
		/*
		 * Set current working point index to maximal working point
		 * index in the DPTC translation table.
		 */
		dptc_params.dvfs_dptc_tables_ptr->curr_wp =
		    dptc_params.dvfs_dptc_tables_ptr->wp_num - 1;
	}

	dptc_initial_wp = dptc_params.dvfs_dptc_tables_ptr->curr_wp;

	/* Initialize the log buffer */
	add_dptc_log_entry(&dptc_params.dptc_log_buffer,
			   dptc_params.dvfs_dptc_tables_ptr->curr_wp);
}

/*!
 * This function returns the number of entries in the DPTC log buffer.
 *
 * @param    dptc_log	pointer to the DPTC log buffer structure.
 *
 * @return   number of log buffer entries.
 *
 */
static int get_entry_count(dptc_log_s * dptc_log)
{
	return ((dptc_log->head - dptc_log->tail + LOG_ENTRIES) % LOG_ENTRIES);
}

/*!
 * This function is used by the proc file system to read the DPTC log buffer.
 * Each time the DPTC proc file system file is read this function is called
 * and returns the data written in the log buffer.
 *
 * @param    buf	pointer to the buffer the data should be written to.
 * @param    start	pointer to the pointer where the new data is
 *                      written to.
 *			procedure should update the start pointer to point to
 *			where in the buffer the data was written.
 * @param    offset	current offset in the DPTC proc file.
 * @param    count	number of bytes to read.
 * @param    eof	pointer to eof flag. should be set to 1 when
 *                      reaching eof.
 * @param    data	driver specific data pointer.
 *
 * @return   number byte read from the log buffer.
 *
 */
static int read_log(char *buf, char **start, off_t offset, int count,
		    int *eof, void *data)
{
	int entries_to_read;
	int num_of_entries;
	int entries_to_end_of_buffer, entries_left;
	void *entry_ptr;
	char *buf_ptr;
	dvfs_dptc_params_s *params;

	params = (dvfs_dptc_params_s *) data;

	/* Calculate number of log entries to read */
	entries_to_read = count / sizeof(dptc_log_entry_s);
	/* Get the number of current log buffer entries */
	num_of_entries = get_entry_count(&params->dptc_log_buffer);

	/*
	 * If number of entries to read is larger that the number of entries
	 * in the log buffer set number of entries to read to number of
	 * entries in the log buffer and set eof flag to 1
	 */
	if (num_of_entries < entries_to_read) {
		entries_to_read = num_of_entries;
		*eof = 1;
	}

	/*
	 * Down the log buffer mutex to exclude others from reading and
	 * writing to the log buffer.
	 */
	if (down_interruptible(&params->dptc_log_buffer.mutex)) {
		return -EAGAIN;
	}

	if (num_of_entries == 0 && offset == 0) {
		inc_log_index(&params->dptc_log_buffer.tail, -1);
		num_of_entries++;
		entries_to_read++;
	}

	/* get the pointer of the last (oldest) entry in the log buffer */
	entry_ptr = (void *)&params->dptc_log_buffer.
	    entries[params->dptc_log_buffer.tail];

	/* Check if tail index wraps during current read */
	if ((params->dptc_log_buffer.tail + entries_to_read) < LOG_ENTRIES) {
		/* No tail wrap around copy data from log buffer to buf */
		memcpy(buf, entry_ptr,
		       (entries_to_read * sizeof(dptc_log_entry_s)));
	} else {
		/*
		 * Tail wrap around.
		 * First copy data from current position until end of buffer,
		 * after that copy the rest from start of the log buffer.
		 */
		entries_to_end_of_buffer = LOG_ENTRIES -
		    params->dptc_log_buffer.tail;
		memcpy(buf, entry_ptr,
		       (entries_to_end_of_buffer * sizeof(dptc_log_entry_s)));

		entry_ptr = (void *)&params->dptc_log_buffer.entries[0];
		buf_ptr = buf +
		    (entries_to_end_of_buffer * sizeof(dptc_log_entry_s));
		entries_left = entries_to_read - entries_to_end_of_buffer;
		memcpy(buf_ptr, entry_ptr,
		       (entries_left * sizeof(dptc_log_entry_s)));
	}

	/* Increment the tail index by the number of entries read */
	inc_log_index(&params->dptc_log_buffer.tail, entries_to_read);

	/* Up the log buffer mutex to allow access to the log buffer */
	up(&params->dptc_log_buffer.mutex);

	/* set start of data to point to buf */
	*start = buf;

	return (entries_to_read * sizeof(dptc_log_entry_s));
}

/*!
 * This function initializes the DPTC log buffer.
 *
 * @param    dptc_log	pointer to the DPTC log buffer structure.
 *
 */
static void init_dptc_log(dptc_log_s * dptc_log)
{
	dptc_log->tail = 0;
	dptc_log->head = 0;

	/* initialize log buffer mutex used for accessing the log buffer */
	sema_init(&dptc_log->mutex, 1);
}

/*!
 * This function initializes the DPTC hardware
 *
 */
static int init_dptc_controller(void)
{
	INIT_WORK(&dptc_work, dptc_workqueue_handler);

	if (create_proc_read_entry(PROC_NODE_NAME, 0,
				   NULL, read_log, &dptc_params) == NULL) {
		/*
		 * Error creating proc file system entry.
		 * Exit and return error code
		 */
		printk(KERN_ERR "DPTC: Unable create proc entry");
		return -EFAULT;
	}

	/* Initialize the DPTC log buffer */
	init_dptc_log(&dptc_params.dptc_log_buffer);

	init_dptc_wp();

	/* By default all reference circuits are enabled */
	dptc_params.rc_state = DPTC_REF_CIRCUITS_STATUS;

	dptc_clear_dcr();

	/* Disable DPTC hardware and mask DPTC interrupt */
	dptc_disable_dptc();
	dptc_mask_dptc_int();

	printk(KERN_INFO "DPTC controller initialized\n");
	return 0;
}

/*!
 * This function is called to put the DPTC in a low power state.
 *
 * @param   pdev  the device structure used to give information on which
 *                device to suspend (not relevant for DPTC)
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
int mxc_dptc_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (dptc_params.dptc_is_active) {
		dptc_disable_dptc();
		set_dptc_wp(dptc_params.dvfs_dptc_tables_ptr->curr_wp - 1);
	}

	dptc_params.suspended = 1;

	return 0;
}

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 * @param   pdev  the device structure used to give information on which
 *                device to suspend (not relevant for DPTC)
 *
 * @return  The function always returns 0.
 */
int mxc_dptc_resume(struct platform_device *pdev)
{
	dptc_params.suspended = 0;

	if (dptc_params.dptc_is_active) {
		dptc_enable_dptc();
		dptc_unmask_dptc_int();
	}

	return 0;
}

/*!
 * This function frees power management table structures
 */
static void free_dvfs_dptc_table(void)
{
	int i;

	for (i = 0; i < dptc_params.dvfs_dptc_tables_ptr->dvfs_state_num; i++) {
		kfree(dptc_params.dvfs_dptc_tables_ptr->dcvr[i]);
	}

	kfree(dptc_params.dvfs_dptc_tables_ptr->dcvr);
	kfree(dptc_params.dvfs_dptc_tables_ptr->table);
	kfree(dptc_params.dvfs_dptc_tables_ptr->wp);

	kfree(dptc_params.dvfs_dptc_tables_ptr);

	dptc_params.dvfs_dptc_tables_ptr = 0;
}

/*
 * DVFS & DPTC table parsing function
 * reads the next line of the table in text format
 *
 * @param   str    pointer to the previous line
 *
 * @return  pointer to the next line
 */
static char *pm_table_get_next_line(char *str)
{
	char *line_ptr;
	int flag = 0;

	if (strlen(str) == 0)
		return str;

	line_ptr = strchr(str, '\n') + 1;

	while (!flag) {
		if (strlen(line_ptr) == 0) {
			flag = 1;
		} else if (line_ptr[0] == '\n') {
			line_ptr++;
		} else if (line_ptr[0] == '#') {
			line_ptr = pm_table_get_next_line(line_ptr);
		} else {
			flag = 1;
		}
	}

	return line_ptr;
}

/*
 * DVFS & DPTC table parsing function
 * sets the values of DVFS & DPTC tables from
 * table in text format
 *
 * @param   pm_table  pointer to the table in binary format
 * @param   pm_str    pointer to the table in text format
 *
 * @return  0 on success, error code on failure
 */
static int dvfs_dptc_parse_table(dvfs_dptc_tables_s * pm_table, char *pm_str)
{
	char *pm_str_ptr;
	int i, j, n;
	dptc_wp *wp;

	pm_str_ptr = pm_str;

	n = sscanf(pm_str_ptr, "WORKING POINT %d\n", &pm_table->wp_num);

	if (n != 1) {
		printk(KERN_WARNING "Failed read WORKING POINT number\n");
		return -1;
	}

	pm_table->curr_wp = 0;

	pm_str_ptr = pm_table_get_next_line(pm_str_ptr);
	pm_table->dvfs_state_num = 1;

	pm_table->wp =
	    (dptc_wp *) kmalloc(sizeof(dptc_wp) * pm_table->wp_num, GFP_KERNEL);
	if (!pm_table->wp) {
		printk(KERN_ERR "Failed allocating memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < pm_table->wp_num; i++) {

		wp = &pm_table->wp[i];

		wp->wp_index = i;

		n = sscanf(pm_str_ptr, "WP 0x%x\n",
			   (unsigned int *)&wp->pmic_values[0]);

		if (n != 1) {
			printk(KERN_WARNING "Failed read WP %d\n", i);
			kfree(pm_table->wp);
			return -1;
		}

		pm_str_ptr = pm_table_get_next_line(pm_str_ptr);

	}

	pm_table->table =
	    (dvfs_state *) kmalloc(sizeof(dvfs_state) *
				   pm_table->dvfs_state_num, GFP_KERNEL);

	if (!pm_table->table) {
		printk(KERN_WARNING "Failed allocating memory\n");
		kfree(pm_table->wp);
		return -ENOMEM;
	}

	pm_table->dcvr =
	    (dcvr_state **) kmalloc(sizeof(dcvr_state *) *
				    pm_table->dvfs_state_num, GFP_KERNEL);

	if (!pm_table->dcvr) {
		printk(KERN_WARNING "Failed allocating memory\n");
		kfree(pm_table->table);
		kfree(pm_table->wp);
		return -ENOMEM;
	}

	for (i = 0; i < pm_table->dvfs_state_num; i++) {
		pm_table->dcvr[i] =
		    (dcvr_state *) kmalloc(sizeof(dcvr_state) *
					   pm_table->wp_num, GFP_KERNEL);

		if (!pm_table->dcvr[i]) {
			printk(KERN_WARNING "Failed allocating memory\n");

			for (j = i - 1; j >= 0; j--) {
				kfree(pm_table->dcvr[j]);
			}

			kfree(pm_table->dcvr);
			return -ENOMEM;
		}

		for (j = 0; j < pm_table->wp_num; j++) {

			n = sscanf(pm_str_ptr, "DCVR 0x%x 0x%x 0x%x 0x%x\n",
				   &pm_table->dcvr[i][j].dcvr_reg[0].AsInt,
				   &pm_table->dcvr[i][j].dcvr_reg[1].AsInt,
				   &pm_table->dcvr[i][j].dcvr_reg[2].AsInt,
				   &pm_table->dcvr[i][j].dcvr_reg[3].AsInt);

			if (n != 4) {
				printk(KERN_WARNING "Failed read FREQ %d\n", i);

				for (j = i; j >= 0; j--) {
					kfree(pm_table->dcvr[j]);
				}
				kfree(pm_table->dcvr);
				kfree(pm_table->table);
				kfree(pm_table->wp);
				return -1;
			}

			pm_str_ptr = pm_table_get_next_line(pm_str_ptr);
		}
	}

	return 0;
}

/*
 * Initializes the default values of DVFS & DPTC table
 *
 * @return  0 on success, error code on failure
 */
static int __init dvfs_dptc_init_default_table(void)
{
	int res = 0;
	char *table_str;

	dvfs_dptc_tables_s *default_table;

	default_table = kmalloc(sizeof(dvfs_dptc_tables_s), GFP_KERNEL);

	if (!default_table) {
		return -ENOMEM;
	}

	table_str = default_table_str;

	memset(default_table, 0, sizeof(dvfs_dptc_tables_s));
	res = dvfs_dptc_parse_table(default_table, table_str);

	if (res == 0) {
		dptc_params.dvfs_dptc_tables_ptr = default_table;
	}

	return res;
}

/*!
 * This function is called when the driver is opened. This function
 * checks if the user that open the device has root privileges.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int dptc_mx27_open(struct inode *inode, struct file *filp)
{
	/*
	 * check if the program that opened the driver has root
	 * privileges, if not return error.
	 */
	if (!capable(CAP_SYS_ADMIN)) {
		return -EACCES;
	}

	if (dptc_params.suspended) {
		return -EPERM;
	}

	return 0;
}

/*!
 * This function is called when the driver is close.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 *
 */
static int dptc_mx27_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*!
 * This function dumps dptc translation table into string pointer
 *
 * @param    str    string pointer
 */
static void dvfs_dptc_dump_table(char *str)
{
	int i, j;
	dcvr_state **dcvr_arr;
	dcvr_state *dcvr_row;

	memset(str, 0, MAX_TABLE_SIZE);

	sprintf(str, "WORKING POINT %d\n",
		dptc_params.dvfs_dptc_tables_ptr->wp_num);
	str += strlen(str);

	for (i = 0; i < dptc_params.dvfs_dptc_tables_ptr->wp_num; i++) {
		sprintf(str, "WP 0x%x\n", (unsigned int)
			dptc_params.dvfs_dptc_tables_ptr->wp[i].pmic_values[0]);
		str += strlen(str);
	}

	dcvr_arr = dptc_params.dvfs_dptc_tables_ptr->dcvr;
	for (i = 0; i < dptc_params.dvfs_dptc_tables_ptr->dvfs_state_num; i++) {
		dcvr_row = dcvr_arr[i];

		for (j = 0; j < dptc_params.dvfs_dptc_tables_ptr->wp_num; j++) {
			sprintf(str,
				"DCVR 0x%x 0x%x 0x%x 0x%x\n",
				dcvr_row[j].dcvr_reg[0].AsInt,
				dcvr_row[j].dcvr_reg[1].AsInt,
				dcvr_row[j].dcvr_reg[2].AsInt,
				dcvr_row[j].dcvr_reg[3].AsInt);

			str += strlen(str);
		}
	}
}

/*!
 * This function reads DVFS & DPTC translation table from user
 *
 * @param    user_table    pointer to user table
 * @return   0 on success, error code on failure
 */
static int dvfs_dptc_set_table(char *user_table)
{
	int ret_val = -ENOIOCTLCMD;
	char *tmp_str;
	char *tmp_str_ptr;
	dvfs_dptc_tables_s *dptc_table;

	if (dptc_params.dptc_is_active == TRUE) {
		ret_val = -EINVAL;
		return ret_val;
	}

	tmp_str = vmalloc(MAX_TABLE_SIZE);

	if (tmp_str < 0) {
		ret_val = (int)tmp_str;
	} else {
		memset(tmp_str, 0, MAX_TABLE_SIZE);
		tmp_str_ptr = tmp_str;

		/*
		 * read num_of_wp and dvfs_state_num
		 * parameters from new table
		 */
		while (tmp_str_ptr - tmp_str < MAX_TABLE_SIZE &&
		       (!copy_from_user(tmp_str_ptr, user_table, 1)) &&
		       tmp_str_ptr[0] != 0) {
			tmp_str_ptr++;
			user_table++;
		}
		if (tmp_str_ptr == tmp_str) {
			/* error reading from table */
			printk(KERN_ERR "Failed reading table from user, \
didn't copy a character\n");
			ret_val = -EFAULT;
		} else if (tmp_str_ptr - tmp_str == MAX_TABLE_SIZE) {
			/* error reading from table */
			printk(KERN_ERR "Failed reading table from user, \
read more than %d\n", MAX_TABLE_SIZE);
			ret_val = -EFAULT;
		} else {
			/*
			 * copy table from user and set it as
			 * the current DPTC table
			 */
			dptc_table = kmalloc(sizeof(dvfs_dptc_tables_s),
					     GFP_KERNEL);

			if (!dptc_table) {
				ret_val = -ENOMEM;
			} else {
				ret_val =
				    dvfs_dptc_parse_table(dptc_table, tmp_str);

				if (ret_val == 0) {
					free_dvfs_dptc_table();
					dptc_params.dvfs_dptc_tables_ptr =
					    dptc_table;

					set_dptc_wp(dptc_initial_wp);
				}
			}

		}

		vfree(tmp_str);
	}

	return ret_val;
}

/*!
 * This function is called when a ioctl call is made from user space.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 * @param    cmd      Ioctl command
 * @param    arg      Ioctl argument
 *
 *                    Following are the ioctl commands for user to use:\n
 *                    DPTC_IOCTENABLE : Enables the DPTC module.\n
 *                    DPTC_IOCTDISABLE : Disables the DPTC module.\n
 *                    DPTC_IOCSENABLERC : Enables DPTC reference circuits.\n
 *                    DPTC_IOCSDISABLERC : Disables DPTC reference circuits.\n
 *                    DPTC_IOCGETSTATE : Returns 1 if the DPTC module is enabled,
 *                    returns 0 if the DPTC module is disabled.\n
 *                    DPTC_IOCSWP : Sets working point.\n
 *                    PM_IOCSTABLE : Sets translation table.\n
 *                    PM_IOCGTABLE : Gets translation table.\n
 *                    PM_IOCGFREQ : Returns current CPU frequency in Hz
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int dptc_mx27_ioctl(struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	struct clk *clk;
	unsigned int tmp;
	int ret_val = -ENOIOCTLCMD;
	char *tmp_str;

	tmp = arg;

	if (dptc_params.suspended) {
		return -EPERM;
	}

	down(&access_mutex);

	pr_debug("DVFS_DPTC ioctl (%d)\n", cmd);

	switch (cmd) {
		/* Enable the DPTC module */
	case DPTC_IOCTENABLE:
		ret_val = start_dptc();
		break;

		/* Disable the DPTC module */
	case DPTC_IOCTDISABLE:
		ret_val = stop_dptc();
		break;

	case DPTC_IOCSENABLERC:
		ret_val = enable_ref_circuits(tmp);
		break;

	case DPTC_IOCSDISABLERC:
		ret_val = disable_ref_circuits(tmp);
		break;
		/*
		 * Return the DPTC module current state.
		 * Returns 1 if the DPTC module is enabled, else returns 0
		 */
	case DPTC_IOCGSTATE:
		ret_val = dptc_params.dptc_is_active;
		break;
	case DPTC_IOCSWP:
		if (dptc_params.dptc_is_active == FALSE) {
			if (arg >= 0 &&
			    arg < dptc_params.dvfs_dptc_tables_ptr->wp_num) {
				set_dptc_wp(arg);
				ret_val = 0;
			} else {
				ret_val = -EINVAL;
			}
		} else {
			ret_val = -EINVAL;
		}
		break;

		/* Update DPTC table */
	case PM_IOCSTABLE:
		ret_val = dvfs_dptc_set_table((char *)arg);
		break;

	case PM_IOCGTABLE:
		tmp_str = vmalloc(MAX_TABLE_SIZE);
		if (tmp_str < 0) {
			ret_val = (int)tmp_str;
		} else {
			dvfs_dptc_dump_table(tmp_str);
			if (copy_to_user((char *)tmp, tmp_str, strlen(tmp_str))) {
				printk(KERN_ERR
				       "Failed copy %d characters to 0x%x\n",
				       strlen(tmp_str), tmp);
				ret_val = -EFAULT;
			} else {
				ret_val = 0;
			}
			vfree(tmp_str);
		}
		break;

	case PM_IOCGFREQ:
		clk = clk_get(NULL, "cpu_clk");
		ret_val = clk_get_rate(clk);
		break;

		/* Unknown ioctl command -> return error */
	default:
		printk(KERN_ERR "Unknown ioctl command 0x%x\n", cmd);
		ret_val = -ENOIOCTLCMD;
	}

	up(&access_mutex);

	return ret_val;
}

/*!
 * This function is the DPTC Interrupt handler.
 * This function wakes-up the dptc_workqueue_handler function that handles the
 * DPTC interrupt.
 *
 * @param   irq      The Interrupt number
 * @param   dev_id   Driver private data
 *
 * @result    The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *            returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 *            \b IRQ_RETVAL is defined in include/linux/interrupt.h.
 */
static irqreturn_t dptc_mx27_irq(int irq, void *dev_id)
{
	if (dptc_params.dptc_is_active == TRUE) {
		dptc_intr_status = __raw_readl(IO_ADDRESS(MX27_PMCR_BASE_ADDR));

		/* Acknowledge the interrupt */
		__raw_writel(dptc_intr_status, IO_ADDRESS(MX27_PMCR_BASE_ADDR));

		/* Extract the interrupt cause */
		dptc_intr_status =
		    (dptc_intr_status >> MX27_PMCR_LO_INTR_OFFSET) & 0x7;

		if (dptc_intr_status != 0) {
			dptc_mask_dptc_int();
			dptc_disable_dptc();
			schedule_work(&dptc_work);
		}
	}

	return IRQ_RETVAL(1);
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxc_dptc_driver = {
	.driver = {
		   .name = "mxc_dptc",
		   .bus = &platform_bus_type,
		   },
	.suspend = mxc_dptc_suspend,
	.resume = mxc_dptc_resume,
};

/*!
 * This is platform device structure for adding MU
 */
static struct platform_device mxc_dptc_device = {
	.name = "mxc_dptc",
	.id = 0,
};

/*!
 * This function is called for module initialization.
 * It initializes the driver data structures, sets up the DPTC hardware,
 * registers the DPTC driver, creates a proc file system read entry and
 * attaches the driver to the DPTC interrupt.
 *
 * @return   0 to indicate success else returns a negative number.
 *
 */
static int __init dptc_mx27_init(void)
{
	int res;
	struct class_device *temp_class;

	res = dvfs_dptc_init_default_table();

	if (res < 0) {
		printk(KERN_WARNING "Failed parsing default DPTC table\n");
		return res;
	}

	/* Initialize DPTC hardware */
	res = init_dptc_controller();
	if (res < 0) {
		free_dvfs_dptc_table();
	}

	/* Initialize internal driver structures */
	dptc_params.dptc_is_active = FALSE;

	/*
	 * Register DPTC driver as a char driver with an automatically allocated
	 * major number.
	 */
	major = register_chrdev(0, DEVICE_NAME, &fops);

	/*
	 * Return error if a negative major number is returned.
	 */
	if (major < 0) {
		printk(KERN_ERR
		       "DPTC: Registering driver failed with %d\n", major);
		free_dvfs_dptc_table();
		return major;
	}

	mxc_dvfs_dptc_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mxc_dvfs_dptc_class)) {
		printk(KERN_ERR "DPTC: Error creating class.\n");
		goto err_out;
	}

	temp_class =
	    class_device_create(mxc_dvfs_dptc_class, NULL, MKDEV(major, 0),
				NULL, DEVICE_NAME);
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "DPTC: Error creating class device.\n");
		goto err_out;
	}

	/* request the DPTC interrupt */
	res = request_irq(MXC_INT_CCM, dptc_mx27_irq, 0, DEVICE_NAME, NULL);

	/*
	 * If res is not 0, then where was an error
	 * during attaching to DPTC interrupt.
	 * Exit and return error code.
	 */
	if (res) {
		printk(KERN_ERR "DPTC: Unable to attach to DPTC interrupt");
		free_dvfs_dptc_table();
		goto err_out;
	}

	/* Register low power modes functions */
	res = platform_driver_register(&mxc_dptc_driver);
	if (res == 0) {
		res = platform_device_register(&mxc_dptc_device);
		if (res != 0) {
			free_dvfs_dptc_table();
			goto err_out;
		}
	}

	dptc_params.suspended = FALSE;

	return 0;

      err_out:
	printk(KERN_WARNING "MX27 DPTC driver was not initialized\n");
	class_device_destroy(mxc_dvfs_dptc_class, MKDEV(major, 0));
	class_destroy(mxc_dvfs_dptc_class);
	unregister_chrdev(major, DEVICE_NAME);
	return -1;
}

/*!
 * This function is called whenever the module is removed from the kernel. It
 * unregisters the DVFS & DPTC driver from kernel, frees the irq number
 * and removes the proc file system entry.
 */
static void __exit dptc_mx27_cleanup(void)
{
	free_dvfs_dptc_table();

	/* Un-register the driver and remove its node */
	class_device_destroy(mxc_dvfs_dptc_class, MKDEV(major, 0));
	class_destroy(mxc_dvfs_dptc_class);
	unregister_chrdev(major, DEVICE_NAME);

	/* release the DPTC interrupt */
	free_irq(MXC_INT_CCM, NULL);

	/* Unregister low power modes functions */
	platform_driver_unregister(&mxc_dptc_driver);
	platform_device_unregister(&mxc_dptc_device);

	/* remove the DPTC proc file system entry */
	remove_proc_entry(PROC_NODE_NAME, NULL);
}

module_init(dptc_mx27_init);
module_exit(dptc_mx27_cleanup);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MX27 DPTC driver");
MODULE_LICENSE("GPL");
