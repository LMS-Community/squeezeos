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
 * @file dvfs_dptc.c
 *
 * @brief Driver for the Freescale Semiconductor MXC DVFS & DPTC module.
 *
 * The DVFS & DPTC driver
 * driver is designed as a character driver which interacts with the MXC
 * DVFS & DPTC hardware. Upon initialization, the DVFS & DPTC driver initializes
 * the DVFS & DPTC hardware sets up driver nodes attaches to the DVFS & DPTC
 * interrupts and initializes internal data structures. When the DVFS or DPTC
 * interrupt occurs the driver checks the cause of the interrupt
 * (lower voltage/frequency, increase voltage/frequency or emergency) and changes
 * the CPU voltage and/or frequency according to translation table that is loaded
 * into the driver (the voltage changes are done by calling some routines
 * of the mc13783 driver).
 *
 * @ingroup PM_MX31 PM_MXC91321
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/arch/hardware.h>
#include <asm/arch/pmic_external.h>

/*
 * Module header files
 */
#include "dvfs_dptc.h"
#include <asm/arch/dptc.h>

#ifdef CONFIG_MXC_DVFS
#include <asm/arch/dvfs.h>
#endif

/*
 * Prototypes
 */
static int dvfs_dptc_open(struct inode *inode, struct file *filp);
static int dvfs_dptc_release(struct inode *inode, struct file *filp);
static int dvfs_dptc_ioctl(struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long arg);
#ifdef CONFIG_MXC_DVFS_SDMA
static ssize_t dvfs_dptc_read(struct file *filp, char __user * buf,
			      size_t count, loff_t * ppos);
#endif

#ifndef CONFIG_MXC_DVFS_SDMA
static irqreturn_t dvfs_dptc_irq(int irq, void *dev_id);
#else
static void dvfs_dptc_sdma_callback(dvfs_dptc_params_s * params);
#endif

#ifdef CONFIG_MXC_DPTC
/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxc_dptc_driver = {
	.driver = {
		   .name = "mxc_dptc",
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
#endif

/*
 * Global variables
 */

/*!
 * The dvfs_dptc_params structure holds all the internal DPTC driver parameters
 * (current working point, current frequency, translation table and DPTC
 * log buffer).
 */
static dvfs_dptc_params_s dvfs_dptc_params;

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
	.open = dvfs_dptc_open,
	.release = dvfs_dptc_release,
	.ioctl = dvfs_dptc_ioctl,
#ifdef CONFIG_MXC_DVFS_SDMA
	.read = dvfs_dptc_read,
#endif
};

#ifdef CONFIG_MXC_DVFS_SDMA
/*
 * Update pointers to physical addresses of DVFS & DPTC table
 * for SDMA usage
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DVFS &
 *                                   DPTC translation table.
 */
static void dvfs_dptc_virt_2_phys(dvfs_dptc_tables_s * dvfs_dptc_table)
{
	int i;

	/* Update DCVR pointers */
	for (i = 0; i < dvfs_dptc_table->dvfs_state_num; i++) {
		dvfs_dptc_table->dcvr[i] = (dcvr_state *)
		    sdma_virt_to_phys(dvfs_dptc_table->dcvr[i]);
	}
	dvfs_dptc_table->dcvr = (dcvr_state **)
	    sdma_virt_to_phys(dvfs_dptc_table->dcvr);
	dvfs_dptc_table->table = (dvfs_state *)
	    sdma_virt_to_phys(dvfs_dptc_table->table);
}

/*
 * Update pointers to virtual addresses of DVFS & DPTC table
 * for ARM usage
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DVFS &
 *                                   DPTC translation table.
 */
static void dvfs_dptc_phys_2_virt(dvfs_dptc_tables_s * dvfs_dptc_table)
{
	int i;

	dvfs_dptc_table->table = sdma_phys_to_virt
	    ((unsigned long)dvfs_dptc_table->table);
	dvfs_dptc_table->dcvr = sdma_phys_to_virt
	    ((unsigned long)dvfs_dptc_table->dcvr);

	/* Update DCVR pointers */
	for (i = 0; i < dvfs_dptc_table->dvfs_state_num; i++) {
		dvfs_dptc_table->dcvr[i] =
		    sdma_phys_to_virt((unsigned long)dvfs_dptc_table->dcvr[i]);
	}
}
#endif

/*!
 * This function frees power management table structures
 */
static void free_dvfs_dptc_table(void)
{
	int i;

#ifdef CONFIG_MXC_DVFS_SDMA
	dvfs_dptc_phys_2_virt(dvfs_dptc_params.dvfs_dptc_tables_ptr);
#endif

	for (i = 0;
	     i < dvfs_dptc_params.dvfs_dptc_tables_ptr->dvfs_state_num; i++) {
		sdma_free(dvfs_dptc_params.dvfs_dptc_tables_ptr->dcvr[i]);
	}

	sdma_free(dvfs_dptc_params.dvfs_dptc_tables_ptr->dcvr);
	sdma_free(dvfs_dptc_params.dvfs_dptc_tables_ptr->table);
	sdma_free(dvfs_dptc_params.dvfs_dptc_tables_ptr->wp);

	sdma_free(dvfs_dptc_params.dvfs_dptc_tables_ptr);

	dvfs_dptc_params.dvfs_dptc_tables_ptr = 0;
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
	dvfs_state *table;

	pm_str_ptr = pm_str;

	n = sscanf(pm_str_ptr, "WORKING POINT %d\n", &pm_table->wp_num);

	if (n != 1) {
		printk(KERN_WARNING "Failed read WORKING POINT number\n");
		return -1;
	}

	pm_table->curr_wp = 0;

	pm_str_ptr = pm_table_get_next_line(pm_str_ptr);

	if (cpu_is_mx31() || cpu_is_mx32()) {
		pm_table->dvfs_state_num = 4;
		pm_table->use_four_freq = 1;
	} else {
		pm_table->dvfs_state_num = 1;
	}

	pm_table->wp =
	    (dptc_wp *) sdma_malloc(sizeof(dptc_wp) * pm_table->wp_num);
	if (!pm_table->wp) {
		printk(KERN_ERR "Failed allocating memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < pm_table->wp_num; i++) {

		wp = &pm_table->wp[i];

		wp->wp_index = i;

		if (cpu_is_mx31() || cpu_is_mx32()) {
			n = sscanf(pm_str_ptr, "WP 0x%x 0x%x 0x%x 0x%x\n",
				   (unsigned int *)&wp->pmic_values[0],
				   (unsigned int *)&wp->pmic_values[1],
				   (unsigned int *)&wp->pmic_values[2],
				   (unsigned int *)&wp->pmic_values[3]);

			if (n != 4) {
				printk(KERN_WARNING "Failed read WP %d\n", i);
				sdma_free(pm_table->wp);
				return -1;
			}
		} else {
			n = sscanf(pm_str_ptr, "WP 0x%x\n",
				   (unsigned int *)&wp->pmic_values[0]);

			if (n != 1) {
				printk(KERN_WARNING "Failed read WP %d\n", i);
				sdma_free(pm_table->wp);
				return -1;
			}
		}

		pm_str_ptr = pm_table_get_next_line(pm_str_ptr);

	}

	pm_table->table =
	    (dvfs_state *) sdma_malloc(sizeof(dvfs_state) *
				       pm_table->dvfs_state_num);

	if (!pm_table->table) {
		printk(KERN_WARNING "Failed allocating memory\n");
		sdma_free(pm_table->wp);
		return -ENOMEM;
	}

	if (cpu_is_mx31() || cpu_is_mx32()) {
		for (i = 0; i < pm_table->dvfs_state_num; i++) {
			table = &pm_table->table[i];

			n = sscanf(pm_str_ptr,
				   "FREQ %d %d 0x%x 0x%x 0x%x 0x%x %d\n",
				   (unsigned int *)&table->pll_sw_up,
				   (unsigned int *)&table->pll_sw_down,
				   (unsigned int *)&table->pdr0_up,
				   (unsigned int *)&table->pdr0_down,
				   (unsigned int *)&table->pll_up,
				   (unsigned int *)&table->pll_down,
				   (unsigned int *)&table->vscnt);

			if (n != 7) {
				printk(KERN_WARNING "Failed read FREQ %d\n", i);
				sdma_free(pm_table->table);
				sdma_free(pm_table->wp);
				return -1;
			}

			pm_str_ptr = pm_table_get_next_line(pm_str_ptr);
		}
	}

	pm_table->dcvr =
	    (dcvr_state **) sdma_malloc(sizeof(dcvr_state *) *
					pm_table->dvfs_state_num);

	if (!pm_table->dcvr) {
		printk(KERN_WARNING "Failed allocating memory\n");
		sdma_free(pm_table->table);
		sdma_free(pm_table->wp);
		return -ENOMEM;
	}

	for (i = 0; i < pm_table->dvfs_state_num; i++) {
		pm_table->dcvr[i] =
		    (dcvr_state *) sdma_malloc(sizeof(dcvr_state) *
					       pm_table->wp_num);

		if (!pm_table->dcvr[i]) {
			printk(KERN_WARNING "Failed allocating memory\n");

			for (j = i - 1; j >= 0; j--) {
				sdma_free(pm_table->dcvr[j]);
			}

			sdma_free(pm_table->dcvr);
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
					sdma_free(pm_table->dcvr[j]);
				}
				sdma_free(pm_table->dcvr);
				sdma_free(pm_table->table);
				sdma_free(pm_table->wp);
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
	struct clk *clk;

	dvfs_dptc_tables_s *default_table;

	default_table = sdma_malloc(sizeof(dvfs_dptc_tables_s));

	if (!default_table) {
		return -ENOMEM;
	}

	table_str = default_table_str;
	if (cpu_is_mx31() || cpu_is_mx32()) {
		if (cpu_is_mx31_rev(CHIP_REV_2_0) < 0) {
			clk = clk_get(NULL, "ckih");
			if (clk_get_rate(clk) == 27000000) {
				printk(KERN_INFO
				       "DVFS & DPTC: using 27MHz CKIH table\n");
#ifdef CONFIG_ARCH_MX3
				table_str = default_table_str_27ckih;
#endif
			}
		} else {
#ifdef CONFIG_ARCH_MX3
			table_str = default_table_str_rev2;
#endif
		}
		clk_put(clk);
	}

	memset(default_table, 0, sizeof(dvfs_dptc_tables_s));
	res = dvfs_dptc_parse_table(default_table, table_str);

	if (res == 0) {
		dvfs_dptc_params.dvfs_dptc_tables_ptr = default_table;
	}

	return res;
}

#ifdef CONFIG_MXC_DVFS_SDMA
/*!
 * This function is called for SDMA channel initialization.
 *
 * @param    params    pointer to the DPTC driver parameters structure.
 *
 * @return   0 to indicate success else returns a negative number.
 */
static int init_sdma_channel(dvfs_dptc_params_s * params)
{
	dma_channel_params sdma_params;
	dma_request_t sdma_request;
	int i;
	int res = 0;

	params->sdma_channel = 0;
	res = mxc_request_dma(&params->sdma_channel, "DVFS_DPTC");
	if (res < 0) {
		printk(KERN_ERR "Failed allocate SDMA channel for DVFS_DPTC\n");
		return res;
	}

	memset(&sdma_params, 0, sizeof(dma_channel_params));
	sdma_params.peripheral_type = CCM;
	sdma_params.transfer_type = per_2_emi;
	sdma_params.event_id = DMA_REQ_CCM;
	sdma_params.callback = (dma_callback_t) dvfs_dptc_sdma_callback;
	sdma_params.arg = params;
	sdma_params.per_address = CCM_BASE_ADDR;
	sdma_params.watermark_level =
	    sdma_virt_to_phys(params->dvfs_dptc_tables_ptr);
	sdma_params.bd_number = 2;

	res = mxc_dma_setup_channel(params->sdma_channel, &sdma_params);

	if (res == 0) {
		memset(&sdma_request, 0, sizeof(dma_request_t));

		for (i = 0; i < DVFS_LB_SDMA_BD; i++) {
			sdma_request.destAddr = (__u8 *)
			    (params->dvfs_log_buffer_phys +
			     i * (DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8) /
			     DVFS_LB_SDMA_BD);
			sdma_request.count = DVFS_LB_SIZE / DVFS_LB_SDMA_BD;
			sdma_request.bd_cont = 1;

			mxc_dma_set_config(params->sdma_channel, &sdma_request,
					   i);
		}

		mxc_dma_start(params->sdma_channel);
	}

	return res;
}
#endif

/*!
 * This function is called for module initialization.
 * It initializes the driver data structures, sets up the DPTC hardware,
 * registers the DPTC driver, creates a proc file system read entry and
 * attaches the driver to the DPTC interrupt.
 *
 * @return   0 to indicate success else returns a negative number.
 *
 */
static int __init dvfs_dptc_init(void)
{
	int res = 0;
	struct class_device *temp_class;

	res = dvfs_dptc_init_default_table();

	if (res < 0) {
		printk(KERN_WARNING "Failed parsing default DPTC table\n");
		return res;
	}
#ifdef CONFIG_MXC_DPTC
	/* Initialize DPTC hardware */
	res = init_dptc_controller(&dvfs_dptc_params);
	if (res < 0) {
		free_dvfs_dptc_table();
		return res;
	}
#endif

#ifdef CONFIG_MXC_DVFS
	/* Initialize DVFS hardware */
	res = init_dvfs_controller(&dvfs_dptc_params);
	if (res < 0) {
		free_dvfs_dptc_table();
		return res;
	}

	/* Enable 4 mc13783 output voltages */
	pmic_write_reg(REG_ARBITRATION_SWITCHERS, 1, (1 << 5));

	/* Enable mc13783 voltage ready signal */
	pmic_write_reg(REG_INTERRUPT_MASK_1, 0, (1 << 11));

	/* Set mc13783 DVS speed 25mV each 4us */
	pmic_write_reg(REG_SWITCHERS_4, 1, (1 << 6));
	pmic_write_reg(REG_SWITCHERS_4, 0, (1 << 7));

	dvfs_update_freqs_table(dvfs_dptc_params.dvfs_dptc_tables_ptr);
#endif

#ifdef CONFIG_MXC_DVFS_SDMA
	/* Update addresses to physical */
	if (res == 0) {
		dvfs_dptc_virt_2_phys(dvfs_dptc_params.dvfs_dptc_tables_ptr);
	}

	res = init_sdma_channel(&dvfs_dptc_params);
	if (res < 0) {
		free_dvfs_dptc_table();
		return res;
	}
#endif

	/* Initialize internal driver structures */
	dvfs_dptc_params.dptc_is_active = FALSE;

#ifdef CONFIG_MXC_DVFS
	dvfs_dptc_params.dvfs_is_active = FALSE;
#endif

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
		res = PTR_ERR(mxc_dvfs_dptc_class);
		goto err_out1;
	}

	temp_class =
	    class_device_create(mxc_dvfs_dptc_class, NULL, MKDEV(major, 0),
				NULL, DEVICE_NAME);
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "DPTC: Error creating class device.\n");
		res = PTR_ERR(temp_class);
		goto err_out2;
	}
#ifndef CONFIG_MXC_DVFS_SDMA
	/* request the DPTC interrupt */
	res = request_irq(INT_CCM, dvfs_dptc_irq, 0, DEVICE_NAME, NULL);
	/*
	 * If res is not 0, then where was an error
	 * during attaching to DPTC interrupt.
	 * Exit and return error code.
	 */
	if (res) {
		printk(KERN_ERR "DPTC: Unable to attach to DPTC interrupt");
		goto err_out3;
	}
	/* request the DVFS interrupt */
	res = request_irq(INT_DVFS, dvfs_dptc_irq, 0, DEVICE_NAME, NULL);
	if (res) {
		printk(KERN_ERR "DVFS: Unable to attach to DVFS interrupt");
		goto err_out4;
	}
#endif

#ifdef CONFIG_MXC_DPTC
	/* Register low power modes functions */
	res = platform_driver_register(&mxc_dptc_driver);
	if (res == 0) {
		res = platform_device_register(&mxc_dptc_device);
		if (res != 0) {
			goto err_out5;
		}
	}
#endif
	dvfs_dptc_params.suspended = FALSE;

	return res;

      err_out5:
	free_irq(INT_DVFS, NULL);
#ifndef CONFIG_MXC_DVFS_SDMA
      err_out4:
#endif
	free_irq(INT_CCM, NULL);
#ifndef CONFIG_MXC_DVFS_SDMA
      err_out3:
#endif
	class_device_destroy(mxc_dvfs_dptc_class, MKDEV(major, 0));
      err_out2:
	class_destroy(mxc_dvfs_dptc_class);
      err_out1:
	unregister_chrdev(major, DEVICE_NAME);
	free_dvfs_dptc_table();
	printk(KERN_ERR "DVFS&DPTC driver was not initialized\n");
	return res;
}

/*!
 * This function is called whenever the module is removed from the kernel. It
 * unregisters the DVFS & DPTC driver from kernel, frees the irq number
 * and removes the proc file system entry.
 */
static void __exit dvfs_dptc_cleanup(void)
{
#ifdef CONFIG_MXC_DPTC
	/* Unregister low power modes functions */
	platform_driver_unregister(&mxc_dptc_driver);
	platform_device_unregister(&mxc_dptc_device);
#endif

	free_dvfs_dptc_table();

	/* Un-register the driver and remove its node */
	class_device_destroy(mxc_dvfs_dptc_class, MKDEV(major, 0));
	class_destroy(mxc_dvfs_dptc_class);
	unregister_chrdev(major, DEVICE_NAME);

	/* release the DPTC interrupt */
	free_irq(INT_CCM, NULL);
	/* release the DVFS interrupt */
	free_irq(INT_DVFS, NULL);

	/* remove the DPTC proc file system entry */
	remove_proc_entry(PROC_NODE_NAME, NULL);
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
static int dvfs_dptc_open(struct inode *inode, struct file *filp)
{
	/*
	 * check if the program that opened the driver has root
	 * privileges, if not return error.
	 */
	if (!capable(CAP_SYS_ADMIN)) {
		return -EACCES;
	}

	if (dvfs_dptc_params.suspended) {
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
static int dvfs_dptc_release(struct inode *inode, struct file *filp)
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
	dvfs_state *table;

	memset(str, 0, MAX_TABLE_SIZE);

	sprintf(str, "WORKING POINT %d\n",
		dvfs_dptc_params.dvfs_dptc_tables_ptr->wp_num);
	str += strlen(str);

	for (i = 0; i < dvfs_dptc_params.dvfs_dptc_tables_ptr->wp_num; i++) {
		if (cpu_is_mx31() || cpu_is_mx32()) {
			sprintf(str, "WP 0x%x 0x%x 0x%x 0x%x\n", (unsigned int)
				dvfs_dptc_params.dvfs_dptc_tables_ptr->wp[i].
				pmic_values[0], (unsigned int)
				dvfs_dptc_params.dvfs_dptc_tables_ptr->wp[i].
				pmic_values[1], (unsigned int)
				dvfs_dptc_params.dvfs_dptc_tables_ptr->wp[i].
				pmic_values[2], (unsigned int)
				dvfs_dptc_params.dvfs_dptc_tables_ptr->wp[i].
				pmic_values[3]);
		} else {
			sprintf(str, "WP 0x%x\n", (unsigned int)
				dvfs_dptc_params.dvfs_dptc_tables_ptr->wp[i].
				pmic_values[0]);
		}

		str += strlen(str);
	}

	if (cpu_is_mx31() || cpu_is_mx32()) {
		for (i = 0;
		     i < dvfs_dptc_params.dvfs_dptc_tables_ptr->dvfs_state_num;
		     i++) {
			table = dvfs_dptc_params.dvfs_dptc_tables_ptr->table;
#ifdef CONFIG_MXC_DVFS_SDMA
			table = sdma_phys_to_virt((unsigned long)table);
#endif
			sprintf(str,
				"FREQ %d %d 0x%x 0x%x 0x%x 0x%x %d\n",
				(unsigned int)table[i].pll_sw_up,
				(unsigned int)table[i].pll_sw_down,
				(unsigned int)table[i].pdr0_up,
				(unsigned int)table[i].pdr0_down,
				(unsigned int)table[i].pll_up,
				(unsigned int)table[i].pll_down,
				(unsigned int)table[i].vscnt);

			str += strlen(str);
		}
	}

	for (i = 0;
	     i < dvfs_dptc_params.dvfs_dptc_tables_ptr->dvfs_state_num; i++) {
		dcvr_arr = dvfs_dptc_params.dvfs_dptc_tables_ptr->dcvr;
#ifdef CONFIG_MXC_DVFS_SDMA
		dcvr_arr = sdma_phys_to_virt((unsigned long)dcvr_arr);
#endif
		dcvr_row = dcvr_arr[i];
#ifdef CONFIG_MXC_DVFS_SDMA
		dcvr_row = sdma_phys_to_virt((unsigned long)dcvr_row);
#endif

		for (j = 0;
		     j < dvfs_dptc_params.dvfs_dptc_tables_ptr->wp_num; j++) {
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
int dvfs_dptc_set_table(char *user_table)
{
	int ret_val = -ENOIOCTLCMD;
	char *tmp_str;
	char *tmp_str_ptr;
	dvfs_dptc_tables_s *dptc_table;

	if ((cpu_is_mx31() || cpu_is_mx32()) &&
	    (dvfs_dptc_params.dptc_is_active == TRUE ||
	     dvfs_dptc_params.dvfs_is_active == TRUE)) {
		ret_val = -EINVAL;
		return ret_val;
	} else if (dvfs_dptc_params.dptc_is_active == TRUE) {
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
			dptc_table = sdma_malloc(sizeof(dvfs_dptc_tables_s));

			if (!dptc_table) {
				ret_val = -ENOMEM;
			} else {
				ret_val =
				    dvfs_dptc_parse_table(dptc_table, tmp_str);

				if (ret_val == 0) {
					free_dvfs_dptc_table();
					dvfs_dptc_params.dvfs_dptc_tables_ptr =
					    dptc_table;

#ifdef CONFIG_MXC_DVFS
					dvfs_update_freqs_table
					    (dvfs_dptc_params.
					     dvfs_dptc_tables_ptr);
#endif

#ifdef CONFIG_MXC_DVFS_SDMA
					/* Update addresses to physical */
					dvfs_dptc_virt_2_phys(dvfs_dptc_params.
							      dvfs_dptc_tables_ptr);
					mxc_free_dma(dvfs_dptc_params.
						     sdma_channel);
					init_sdma_channel(&dvfs_dptc_params);
#endif
#ifdef CONFIG_MXC_DPTC
					set_dptc_curr_freq(&dvfs_dptc_params,
							   0);
					set_dptc_wp(&dvfs_dptc_params, 0);
#endif
				}
			}

		}

		vfree(tmp_str);
	}

	return ret_val;
}

#ifdef CONFIG_MXC_DVFS_SDMA
static ssize_t dvfs_dptc_read(struct file *filp, char __user * buf,
			      size_t count, loff_t * ppos)
{
	size_t count0, count1;

	while (dvfs_dptc_params.chars_in_buffer < count) {
		//count = dvfs_dptc_params.chars_in_buffer;
		waitqueue_active(&dvfs_dptc_params.dvfs_pred_wait);
		wake_up(&dvfs_dptc_params.dvfs_pred_wait);
		schedule();
	}

	if (dvfs_dptc_params.read_ptr + count <
	    dvfs_dptc_params.dvfs_log_buffer +
	    DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8) {
		count0 = count;
		count1 = 0;
	} else {
		count0 =
		    dvfs_dptc_params.dvfs_log_buffer +
		    DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8 -
		    dvfs_dptc_params.read_ptr;
		count1 = count - count0;
	}

	copy_to_user(buf, dvfs_dptc_params.read_ptr, count0);
	copy_to_user(buf + count0, dvfs_dptc_params.dvfs_log_buffer, count1);

	if (count1 == 0) {
		dvfs_dptc_params.read_ptr += count;
	} else {
		dvfs_dptc_params.read_ptr =
		    dvfs_dptc_params.dvfs_log_buffer + count1;
	}

	if (dvfs_dptc_params.read_ptr ==
	    dvfs_dptc_params.dvfs_log_buffer +
	    DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8) {
		dvfs_dptc_params.read_ptr = dvfs_dptc_params.dvfs_log_buffer;
	}

	dvfs_dptc_params.chars_in_buffer -= count;

	return count;
}
#endif

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
 *                    DVFS_IOCTENABLE : Enables DVFS
 *                    DVFS_IOCTDISABLE : Disables DVFS
 *                    DVFS_IOCGSTATE : Returns 1 if the DVFS module is enabled,
 *                    returns 0 if the DVFS module is disabled.\n
 *                    DVFS_IOCSSWGP : Sets the value of DVFS SW general
 *                    purpose bits.\n
 *                    DVFS_IOCSWFI : Sets the status of WFI monitoring.\n
 *                    PM_IOCGFREQ : Returns current CPU frequency in Hz
 *                    DVFS_IOCSFREQ : Sets DVFS frequency when DVFS\n
 *                    HW is disabled.\n
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int dvfs_dptc_ioctl(struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	struct clk *clk;
	unsigned int tmp;
	int ret_val = -ENOIOCTLCMD;
	char *tmp_str;

	tmp = arg;

	if (dvfs_dptc_params.suspended) {
		return -EPERM;
	}

	down(&access_mutex);

	pr_debug("DVFS_DPTC ioctl (%d)\n", cmd);

	switch (cmd) {
#ifdef CONFIG_MXC_DPTC
		/* Enable the DPTC module */
	case DPTC_IOCTENABLE:
		ret_val = start_dptc(&dvfs_dptc_params);
		break;

		/* Disable the DPTC module */
	case DPTC_IOCTDISABLE:
		ret_val = stop_dptc(&dvfs_dptc_params);
		break;

	case DPTC_IOCSENABLERC:
		ret_val = enable_ref_circuits(&dvfs_dptc_params, tmp);
		break;

	case DPTC_IOCSDISABLERC:
		ret_val = disable_ref_circuits(&dvfs_dptc_params, tmp);
		break;
		/*
		 * Return the DPTC module current state.
		 * Returns 1 if the DPTC module is enabled, else returns 0
		 */
	case DPTC_IOCGSTATE:
		ret_val = dvfs_dptc_params.dptc_is_active;
		break;
	case DPTC_IOCSWP:
		if (dvfs_dptc_params.dptc_is_active == FALSE) {
			if (arg >= 0 &&
			    arg <
			    dvfs_dptc_params.dvfs_dptc_tables_ptr->wp_num) {
				set_dptc_wp(&dvfs_dptc_params, arg);
				ret_val = 0;
			} else {
				ret_val = -EINVAL;
			}
		} else {
			ret_val = -EINVAL;
		}
		break;

#endif				/* CONFIG_MXC_DPTC */

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

#ifdef CONFIG_MXC_DVFS
		/* Enable the DVFS module */
	case DVFS_IOCTENABLE:
		ret_val = start_dvfs(&dvfs_dptc_params);
		break;

		/* Disable the DVFS module */
	case DVFS_IOCTDISABLE:
		ret_val = stop_dvfs(&dvfs_dptc_params);
		break;
		/*
		 * Return the DVFS module current state.
		 * Returns 1 if the DPTC module is enabled, else returns 0
		 */
	case DVFS_IOCGSTATE:
		ret_val = dvfs_dptc_params.dvfs_is_active;
		break;
	case DVFS_IOCSSWGP:
		ret_val = set_sw_gp((unsigned char)arg);
		break;
	case DVFS_IOCSWFI:
		ret_val = set_wfi((unsigned char)arg);
		break;
	case DVFS_IOCSFREQ:
		if (dvfs_dptc_params.dvfs_is_active == FALSE ||
		    dvfs_dptc_params.dvfs_mode == DVFS_PRED_MODE) {
			if (arg >= 0 &&
			    arg <
			    dvfs_dptc_params.dvfs_dptc_tables_ptr->
			    dvfs_state_num) {
				ret_val = dvfs_set_state(arg);
			} else {
				ret_val = -EINVAL;
			}
		} else {
			ret_val = -EINVAL;
		}
		break;
	case DVFS_IOCSMODE:
#ifdef CONFIG_MXC_DVFS_SDMA
		if (dvfs_dptc_params.dvfs_is_active == FALSE) {
			if ((unsigned int)arg == DVFS_HW_MODE ||
			    (unsigned int)arg == DVFS_PRED_MODE) {
				dvfs_dptc_params.dvfs_mode = (unsigned int)arg;
				ret_val = 0;
			} else {
				ret_val = -EINVAL;
			}
		} else {
			ret_val = -EINVAL;
		}
#else
		/* Predictive mode is supported only in SDMA mode */
		ret_val = -EINVAL;
#endif
		break;
#endif				/* CONFIG_MXC_DVFS */
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

#ifndef CONFIG_MXC_DVFS_SDMA
/*!
 * This function is the DPTC & DVFS Interrupt handler.
 * This function wakes-up the dvfs_dptc_workqueue_handler function that handles the
 * DPTC interrupt.
 *
 * @param   irq      The Interrupt number
 * @param   dev_id   Driver private data
 *
 * @result    The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *            returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 *            \b IRQ_RETVAL is defined in include/linux/interrupt.h.
 */
static irqreturn_t dvfs_dptc_irq(int irq, void *dev_id)
{

#ifdef CONFIG_MXC_DPTC
	if (dvfs_dptc_params.dptc_is_active == TRUE) {
		dptc_irq();
	}
#endif

#ifdef CONFIG_MXC_DVFS
	if (dvfs_dptc_params.dvfs_is_active == TRUE) {
		dvfs_irq(&dvfs_dptc_params);
	}
#endif

	return IRQ_RETVAL(1);
}
#else
/*!
 * This function is the DPTC & DVFS SDMA callback.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 */
static void dvfs_dptc_sdma_callback(dvfs_dptc_params_s * params)
{
	dma_request_t sdma_request_params;
	int i;

	for (i = 0; i < DVFS_LB_SDMA_BD; i++) {
		mxc_dma_get_config(params->sdma_channel,
				   &sdma_request_params, i);

		if (sdma_request_params.bd_error == 1) {
			printk(KERN_WARNING
			       "Error in DVFS-DPTC buffer descriptor\n");
		}

		if (sdma_request_params.bd_done == 0) {
			params->chars_in_buffer +=
			    (DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8) /
			    DVFS_LB_SDMA_BD;

			if (params->chars_in_buffer >
			    (DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8)) {
				params->chars_in_buffer =
				    DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE / 8;
				params->read_ptr = params->dvfs_log_buffer;
			}

			sdma_request_params.destAddr =
			    (__u8 *) (params->dvfs_log_buffer_phys +
				      i * (DVFS_LB_SIZE * DVFS_LB_SAMPLE_SIZE /
					   8) / DVFS_LB_SDMA_BD);
			sdma_request_params.count =
			    DVFS_LB_SIZE / DVFS_LB_SDMA_BD;
			sdma_request_params.bd_cont = 1;
			mxc_dma_set_config(params->sdma_channel,
					   &sdma_request_params, i);

			if (params->dvfs_mode == DVFS_PRED_MODE) {
				wake_up_interruptible(&params->dvfs_pred_wait);
			}
		}
	}

#ifdef CONFIG_MXC_DPTC
	if (params->prev_wp != params->dvfs_dptc_tables_ptr->curr_wp) {
		dptc_irq();
	}
#endif
}
#endif

module_init(dvfs_dptc_init);
module_exit(dvfs_dptc_cleanup);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("DVFS & DPTC driver");
MODULE_LICENSE("GPL");
