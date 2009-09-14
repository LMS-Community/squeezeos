/*
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

#ifndef _OS_HEADER1_
#define _OS_HEADER1_

typedef char CHAR;
typedef char *PCHAR;
typedef u8 *PUCHAR;
typedef u16 *PUSHORT;
typedef long *PLONG;
typedef PLONG LONG_PTR;
typedef u32 *ULONG_PTR;
typedef u32 *Pu32;
typedef unsigned int UINT;
typedef UINT *PUINT;
typedef void VOID;
typedef VOID *PVOID;
typedef int WLAN_STATUS;
typedef u8 BOOLEAN;
typedef BOOLEAN *PBOOLEAN;
typedef PVOID PDRIVER_OBJECT;
typedef PUCHAR PUNICODE_STRING;
typedef long long LONGLONG;
typedef LONGLONG *PLONGLONG;
typedef unsigned long long *PULONGLONG;
typedef PUCHAR ANSI_STRING;
typedef ANSI_STRING *PANSI_STRING;
typedef unsigned short WCHAR;
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef struct semaphore SEMAPHORE;

#ifdef __KERNEL__
typedef irqreturn_t IRQ_RET_TYPE;
#define IRQ_RET		return IRQ_HANDLED
#endif /* __KERNEL__ */

#endif /* _OS_HEADER1 */
