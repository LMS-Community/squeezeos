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

#include <linux/module.h>
#include <asm/arch/mxc_security_api.h>

static int __init mxc_sec_mod_init(void)
{
	printk(KERN_INFO "SEC: mxc_sec_mod_init() called \n");
	return 0;
}

static void __exit mxc_sec_mod_cleanup(void)
{
	printk(KERN_INFO "SEC: mxc_sec_mod_cleanup() called \n");
}

#ifndef        CONFIG_ARCH_MX3
#ifdef CONFIG_MXC_SECURITY_HAC
/* Export Symbol of HACC */
EXPORT_SYMBOL(hac_hash_data);
EXPORT_SYMBOL(hac_hashing_status);
EXPORT_SYMBOL(hac_get_status);
EXPORT_SYMBOL(hac_stop);
EXPORT_SYMBOL(hac_hash_result);
EXPORT_SYMBOL(hac_swrst);
EXPORT_SYMBOL(hac_burst_mode);
EXPORT_SYMBOL(hac_burst_read);
#ifdef CONFIG_PM
EXPORT_SYMBOL(hac_suspend);
EXPORT_SYMBOL(hac_resume);
#endif
#endif
#endif

#ifdef CONFIG_MXC_SECURITY_RTIC
/* Export Symbol of RTIC */
EXPORT_SYMBOL(rtic_init);
EXPORT_SYMBOL(rtic_configure_mode);
EXPORT_SYMBOL(rtic_configure_mem_blk);
EXPORT_SYMBOL(rtic_start_hash);
EXPORT_SYMBOL(rtic_get_status);
EXPORT_SYMBOL(rtic_get_control);
EXPORT_SYMBOL(rtic_configure_interrupt);
EXPORT_SYMBOL(rtic_hash_result);
EXPORT_SYMBOL(rtic_hash_write);
EXPORT_SYMBOL(rtic_get_faultaddress);
EXPORT_SYMBOL(rtic_dma_burst_read);
EXPORT_SYMBOL(rtic_hash_once_dma_throttle);
EXPORT_SYMBOL(rtic_dma_delay);
EXPORT_SYMBOL(rtic_wd_timer);
EXPORT_SYMBOL(rtic_sw_reset);
EXPORT_SYMBOL(rtic_clr_irq);
#endif

module_init(mxc_sec_mod_init);
module_exit(mxc_sec_mod_cleanup);
