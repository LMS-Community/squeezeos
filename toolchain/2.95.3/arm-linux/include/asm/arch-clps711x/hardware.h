/*
 *  linux/include/asm-arm/arch-clps711x/hardware.h
 *
 *  This file contains the hardware definitions of the Prospector P720T.
 *
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#define CLPS7111_VIRT_BASE	0xff000000
#define CLPS7111_BASE		CLPS7111_VIRT_BASE

#define EP7212_VIRT_BASE	CLPS7111_VIRT_BASE
#define EP7212_BASE		CLPS7111_VIRT_BASE

#define SYSPLD_VIRT_BASE	0xfe000000
#define SYSPLD_BASE		SYSPLD_VIRT_BASE

#ifndef __ASSEMBLER__

#define PCIO_BASE		IO_BASE

#endif

#endif

