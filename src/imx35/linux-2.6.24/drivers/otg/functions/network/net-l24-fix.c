/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * otg/functions/network/net-l24-fix.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/net-l24-fix.c|20070814002638|51510
 *
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */
/*!
 * @file otg/functions/network/net-l24-fix.c
 * @brief The Linux 2.4 OS specific upper edge (network interface)
 * implementation for the Network Function Driver.
 *
 * This file implements a standard Linux network driver interface and
 * the standard Linux 2.4 module init and exit functions.
 *
 * If compiled into the kernel, this driver can be used with NFSROOT to
 * provide the ROOT filesystem. Please note that the kernel NFSROOT support
 * (circa 2.4.20) can have problems if there are multiple interfaces. So
 * it is best to ensure that there are no other network interfaces compiled
 * in.
 *
 *
 * @ingroup NetworkFunction
 * @ingroup LINUXOS
 */

#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/bitops.h>
#include <linux/capability.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/inetdevice.h>
#include <linux/igmp.h>
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#endif
#include <linux/kmod.h>

#include <net/arp.h>
#include <net/ip.h>
#include <net/route.h>
#include <net/ip_fib.h>


#if defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG)
extern int devinet_ioctl(unsigned int , void __user *);

int local_devinet_ioctl(unsigned int cmd, void __user *data)
{
        return devinet_ioctl(cmd, data);
}

EXPORT_SYMBOL(local_devinet_ioctl);
#endif /* defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) || defined(CONFIG_OTG_NETWORK_RARPD_AUTO_CONFIG) */
