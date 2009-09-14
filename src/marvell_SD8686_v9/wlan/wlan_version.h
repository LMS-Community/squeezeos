/** @file wlan_version.h
  * @brief This file contains wlan driver version number.
  * 
  * (c) Copyright © 2003-2006, Marvell International Ltd. 
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
	10/04/05: Add Doxygen format comments
	
********************************************************/
#include "../release_version.h"

const char driver_version[] =
    "sd8686-%s-" DRIVER_RELEASE_VERSION "-(" "FP" FPNUM ")"
#ifdef	DEBUG_LEVEL2
    "-dbg"
#endif
    " ";
