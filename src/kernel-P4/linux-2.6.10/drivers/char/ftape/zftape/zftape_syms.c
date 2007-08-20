/*
 *      Copyright (C) 1997 Claus-Justus Heine

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *
 * $Source: /home/cvs/s3c2413_kernel2.6.10_dev/drivers/char/ftape/zftape/zftape_syms.c,v $
 * $Revision: 1.1.1.1 $
 * $Date: 2006-09-26 00:11:10 +0100 (Tue, 26 Sep 2006) $
 *
 *      This file contains the symbols that the zftape frontend to 
 *      the ftape floppy tape driver exports 
 */		 

#include <linux/module.h>

#include <linux/zftape.h>

#include "../zftape/zftape-init.h"
#include "../zftape/zftape-read.h"
#include "../zftape/zftape-buffers.h"
#include "../zftape/zftape-ctl.h"

/* zftape-init.c */
EXPORT_SYMBOL(zft_cmpr_register);
EXPORT_SYMBOL(zft_cmpr_unregister);
/* zftape-read.c */
EXPORT_SYMBOL(zft_fetch_segment_fraction);
/* zftape-buffers.c */
EXPORT_SYMBOL(zft_vmalloc_once);
EXPORT_SYMBOL(zft_vmalloc_always);
EXPORT_SYMBOL(zft_vfree);
