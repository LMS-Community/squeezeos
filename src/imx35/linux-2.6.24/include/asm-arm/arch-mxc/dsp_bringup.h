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

#ifndef __ASM_ARCH_MXC_DSP_BRINGUP_H__
#define __ASM_ARCH_MXC_DSP_BRINGUP_H__

/*
 * Define a default start address for any DSP image
 */
#define IMAGE_START_ADDRESS      0x00008170

void dsp_startapp_request(void);
int dsp_memwrite_request(unsigned long addr, unsigned long word);
int dsp_parse_cmdline(const char *cmdline);

#endif				/* __ASM_ARCH_MXC_DSP_BRINGUP_H__ */
