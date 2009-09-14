/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Artem Bityutskiy
 *         Adrian Hunter
 */

#ifndef __UBIFS_COMPAT_H__
#define __UBIFS_COMPAT_H__

#include <linux/version.h>

struct inode;
struct file;
struct page;
struct ubifs_info;
struct retries_info;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
/* iget() does not exist since 2.6.25 */
#define UBIFS_COMPAT_USE_OLD_IGET
void ubifs_read_inode(struct inode *inode);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
/*
 * We have write_begin() write_end() instead of prepare_write(), commit_write()
 * since 2.6.24.
 */
#define UBIFS_COMPAT_USE_OLD_PREPARE_WRITE
#define do_readpage(page) ubifs_do_readpage(page)
int ubifs_do_readpage(struct page *page);
int ubifs_prepare_write(struct file *file, struct page *page, unsigned from,
			unsigned to);
int ubifs_commit_write(struct file *file, struct page *page, unsigned from,
		       unsigned to);
int ubifs_make_free_space(struct ubifs_info *c, struct retries_info *ri,
			  int locked_pg);
#define bdi_init(x) 0
#define bdi_destroy(x)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
#define UBIFS_COMPAT_NO_SHRINKER
#define register_shrinker(x)
#define unregister_shrinker(x)
#define dbg_mempressure_init()
#define dbg_mempressure_exit()
#define set_freezable()
#define is_owner_or_cap(inode)  \
	((current->fsuid == (inode)->i_uid) || capable(CAP_FOWNER))
/* This is to hide slab cache interface changes - destructor was dropped */
#define UBIFSCOMPATNULL ,NULL
#else
#define UBIFSCOMPATNULL
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
#define uninitialized_var(x) x = x
/* print_hex_dump() did not exist in kernel prior to 2.6.22 */
#define print_hex_dump(a, b, c, f, e, buf, len, g) ubifs_hexdump(buf, len)
void ubifs_hexdump(const void *ptr, int size);
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
#define UBIFS_COMPAT_SUPPORT_NFS
#endif

#endif /* !__UBIFS_COMPAT_H__ */
