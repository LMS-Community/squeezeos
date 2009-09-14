/** @file include.h
 * 
 * @brief This file contains all the necessary include file.
 *
 * (c) Copyright © 2003-2007, Marvell International Ltd. 
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */
/********************************************************
Change log:
	10/11/05: Add Doxygen format comments
	01/11/06: Conditional include file removal/addition
	01/30/06: Add kernel 2.6 support
	
********************************************************/

#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include    "os_headers.h"
#include    "wlan_types.h"
#include    "wlan_defs.h"
#include    "wlan_thread.h"

#include    "wlan_wmm.h"
#include    "wlan_11d.h"

#include    "os_timers.h"

#include    "host.h"
#include    "hostcmd.h"

#include    "wlan_scan.h"
#include    "wlan_join.h"

#include    "wlan_dev.h"
#include    "os_macros.h"
#include    "sbi.h"

/* masked by feng */
//#include    <sdio.h>

#include    "wlan_wext.h"
#include    "wlan_decl.h"
#endif /* _INCLUDE_H_ */
