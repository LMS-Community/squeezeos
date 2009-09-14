/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation
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
 * Authors: Artem Bityutskiy
 *          Adrian Hunter
 */

#include "ubifs.h"
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
#define BYTES_PER_LINE 32

/**
 * ubifs_hexdump - dump a buffer.
 * @ptr: the buffer to dump
 * @size: buffer size which must be multiple of 4 bytes
 */
void ubifs_hexdump(const void *ptr, int size)
{
	int i, k = 0, rows, columns;
	const uint8_t *p = ptr;

	rows = size / BYTES_PER_LINE + size % BYTES_PER_LINE;
	for (i = 0; i < rows; i++) {
		int j;

		cond_resched();
		columns = min(size - k, BYTES_PER_LINE) / 4;
		if (columns == 0)
			break;
		printk(KERN_DEBUG "%5d:  ", i * BYTES_PER_LINE);
		for (j = 0; j < columns; j++) {
			int n, N;

			N = size - k > 4 ? 4 : size - k;
			for (n = 0; n < N; n++)
				printk("%02x", p[k++]);
			printk(" ");
		}
		printk("\n");
	}
}
#endif /* LINUX_VERSION_CODE < 2.6.23 */

#ifdef UBIFS_COMPAT_USE_OLD_IGET
struct inode *ubifs_iget(struct super_block *sb, unsigned long inum)
{
	struct inode *inode;

	inode = iget(sb, inum);
	if (!inode) {
		make_bad_inode(inode);
		return ERR_PTR(-EINVAL);
	}

	return inode;
}

int validate_inode(struct ubifs_info *c, const struct inode *inode);

void ubifs_read_inode(struct inode *inode)
{
	int err;
	union ubifs_key key;
	struct ubifs_ino_node *ino;
	struct ubifs_info *c = inode->i_sb->s_fs_info;
	struct ubifs_inode *ui = ubifs_inode(inode);

	dbg_gen("inode %lu", inode->i_ino);
	ubifs_assert(inode->i_state & I_LOCK);

	ino = kmalloc(UBIFS_MAX_INO_NODE_SZ, GFP_NOFS);
	if (!ino) {
		err = -ENOMEM;
		goto out;
	}

	ino_key_init(c, &key, inode->i_ino);

	err = ubifs_tnc_lookup(c, &key, ino);
	if (err)
		goto out_ino;

	inode->i_flags |= (S_NOCMTIME | S_NOATIME);
	inode->i_nlink = le32_to_cpu(ino->nlink);
	inode->i_uid   = le32_to_cpu(ino->uid);
	inode->i_gid   = le32_to_cpu(ino->gid);
	inode->i_atime.tv_sec  = (int64_t)le64_to_cpu(ino->atime_sec);
	inode->i_atime.tv_nsec = le32_to_cpu(ino->atime_nsec);
	inode->i_mtime.tv_sec  = (int64_t)le64_to_cpu(ino->mtime_sec);
	inode->i_mtime.tv_nsec = le32_to_cpu(ino->mtime_nsec);
	inode->i_ctime.tv_sec  = (int64_t)le64_to_cpu(ino->ctime_sec);
	inode->i_ctime.tv_nsec = le32_to_cpu(ino->ctime_nsec);
	inode->i_mode = le32_to_cpu(ino->mode);
	inode->i_size = le64_to_cpu(ino->size);

	ui->data_len    = le32_to_cpu(ino->data_len);
	ui->flags       = le32_to_cpu(ino->flags);
	ui->compr_type  = le16_to_cpu(ino->compr_type);
	ui->creat_sqnum = le64_to_cpu(ino->creat_sqnum);
	ui->xattr_cnt   = le32_to_cpu(ino->xattr_cnt);
	ui->xattr_size  = le64_to_cpu(ino->xattr_size);
	ui->xattr_names = le32_to_cpu(ino->xattr_names);
	ui->synced_i_size = ui->ui_size = inode->i_size;

	ui->xattr = (ui->flags & UBIFS_XATTR_FL) ? 1 : 0;

	err = validate_inode(c, inode);
	if (err)
		goto out_invalid;

	switch (inode->i_mode & S_IFMT) {
	case S_IFREG:
		inode->i_mapping->a_ops = &ubifs_file_address_operations;
		inode->i_op = &ubifs_file_inode_operations;
		inode->i_fop = &ubifs_file_operations;
		if (ui->xattr) {
			ui->data = kmalloc(ui->data_len + 1, GFP_NOFS);
			if (!ui->data) {
				err = -ENOMEM;
				goto out_ino;
			}
			memcpy(ui->data, ino->data, ui->data_len);
			((char *)ui->data)[ui->data_len] = '\0';
		} else if (ui->data_len != 0) {
			err = 10;
			goto out_invalid;
		}
		break;
	case S_IFDIR:
		inode->i_op  = &ubifs_dir_inode_operations;
		inode->i_fop = &ubifs_dir_operations;
		if (ui->data_len != 0) {
			err = 11;
			goto out_invalid;
		}
		break;
	case S_IFLNK:
		inode->i_op = &ubifs_symlink_inode_operations;
		if (ui->data_len <= 0 || ui->data_len > UBIFS_MAX_INO_DATA) {
			err = 12;
			goto out_invalid;
		}
		ui->data = kmalloc(ui->data_len + 1, GFP_NOFS);
		if (!ui->data) {
			err = -ENOMEM;
			goto out_ino;
		}
		memcpy(ui->data, ino->data, ui->data_len);
		((char *)ui->data)[ui->data_len] = '\0';
		break;
	case S_IFBLK:
	case S_IFCHR:
	{
		dev_t rdev;
		union ubifs_dev_desc *dev;

		ui->data = kmalloc(sizeof(union ubifs_dev_desc), GFP_NOFS);
		if (!ui->data) {
			err = -ENOMEM;
			goto out_ino;
		}

		dev = (union ubifs_dev_desc *)ino->data;
		if (ui->data_len == sizeof(dev->new))
			rdev = new_decode_dev(le32_to_cpu(dev->new));
		else if (ui->data_len == sizeof(dev->huge))
			rdev = huge_decode_dev(le64_to_cpu(dev->huge));
		else {
			err = 13;
			goto out_invalid;
		}
		memcpy(ui->data, ino->data, ui->data_len);
		inode->i_op = &ubifs_file_inode_operations;
		init_special_inode(inode, inode->i_mode, rdev);
		break;
	}
	case S_IFSOCK:
	case S_IFIFO:
		inode->i_op = &ubifs_file_inode_operations;
		init_special_inode(inode, inode->i_mode, 0);
		if (ui->data_len != 0) {
			err = 14;
			goto out_invalid;
		}
		break;
	default:
		err = 15;
		goto out_invalid;
	}

	ubifs_set_inode_flags(inode);
	kfree(ino);
	return;

out_invalid:
	ubifs_err("inode %lu validation failed, error %d", inode->i_ino, err);
	dbg_dump_inode(c, inode);
	dbg_dump_node(c, ino);
	err = -EINVAL;
out_ino:
	kfree(ino);
out:
	ubifs_err("failed to read inode %lu, error %d", inode->i_ino, err);
	make_bad_inode(inode);
	return;
}
#endif /* UBIFS_COMPAT_USE_OLD_IGET */

#ifdef UBIFS_COMPAT_USE_OLD_PREPARE_WRITE
int ubifs_prepare_write(struct file *file, struct page *page, unsigned from,
			unsigned to)
{
	struct inode *inode = page->mapping->host;
	struct ubifs_info *c = inode->i_sb->s_fs_info;
	loff_t pos = ((loff_t)page->index << PAGE_CACHE_SHIFT) + to;
	struct ubifs_budget_req req;
	int err;

	ubifs_assert(!(inode->i_sb->s_flags & MS_RDONLY));

	if (unlikely(c->ro_media))
		return -EROFS;

	if (!PageUptodate(page)) {
		/*
		 * The page is not loaded from the flash and has to be loaded
		 * unless we are writing all of it.
		 */
		if (from == 0 && to == PAGE_CACHE_SIZE)
			/*
			 * Set the PG_checked flag to make the further code
			 * allocate full budget, because we do not know whether
			 * the page exists on the flash media or not.
			 */
			SetPageChecked(page);
		else {
			err = do_readpage(page);
			if (err)
				return err;
		}

		SetPageUptodate(page);
		ClearPageError(page);
	}

	memset(&req, 0, sizeof(struct ubifs_budget_req));
	if (!PagePrivate(page)) {
		/*
		 * If the PG_Checked flag is set, the page corresponds to a
		 * hole or to a place beyond the inode. In this case we have to
		 * budget for a new page, otherwise for a dirtied page.
		 */
		if (PageChecked(page))
			req.new_page = 1;
		else
			req.dirtied_page = 1;
	} else
		req.locked_pg = 1;

	if (pos > inode->i_size)
		/*
		 * We are writing beyond the file which means we are going to
		 * change inode size and make the inode dirty. And in turn,
		 * this means we have to budget for making the inode dirty.
		 */
		req.dirtied_ino = 1;

	err = ubifs_budget_space(c, &req);
	return err;
}

int ubifs_commit_write(struct file *file, struct page *page, unsigned from,
		       unsigned to)
{
	struct inode *inode = page->mapping->host;
	struct ubifs_inode *ui = ubifs_inode(inode);
	struct ubifs_info *c = inode->i_sb->s_fs_info;
	loff_t pos = ((loff_t)page->index << PAGE_CACHE_SHIFT) + to;

	dbg_gen("ino %lu, pg %lu, offs %lld-%lld (in pg: %u-%u, %u bytes) "
		"flags %#lx", inode->i_ino, page->index, pos - to + from,
		pos, from, to, to - from, page->flags);
	ubifs_assert(PageUptodate(page));
	ubifs_assert(mutex_is_locked(&inode->i_mutex));

	if (!PagePrivate(page)) {
		SetPagePrivate(page);
		atomic_long_inc(&c->dirty_pg_cnt);
		__set_page_dirty_nobuffers(page);
	}

	if (pos > inode->i_size) {
		int release;

		mutex_lock(&ui->ui_mutex);
		i_size_write(inode, pos);
		ui->ui_size = pos;
		release = ui->dirty;
		/*
		 * Note, we do not set @I_DIRTY_PAGES (which means that the
		 * inode has dirty pages), this has been done in
		 * '__set_page_dirty_nobuffers()'.
		 */
		__mark_inode_dirty(inode, I_DIRTY_DATASYNC);
		mutex_unlock(&ui->ui_mutex);

		/*
		 * We've marked the inode as dirty and we have allocated budget
		 * for this. However, the inode may had already be be dirty
		 * before, in which case we have to free the budget.
		 */
		if (release)
			ubifs_release_dirty_inode_budget(c, ui);
	}

	return 0;
}

#include <linux/writeback.h>

#define MAX_SHRINK_RETRIES 8
#define MAX_GC_RETRIES     4
#define MAX_CMT_RETRIES    2
#define MAX_NOSPC_RETRIES  1
#define NR_TO_WRITE 16

struct retries_info {
	long long prev_liability;
	unsigned int shrink_cnt;
	unsigned int shrink_retries:5;
	unsigned int try_gc:1;
	unsigned int gc_retries:4;
	unsigned int cmt_retries:3;
	unsigned int nospc_retries:1;
};

static int shrink_liability(struct ubifs_info *c, int nr_to_write,
			    int locked_pg)
{
	struct writeback_control wbc = {
		.sync_mode   = WB_SYNC_NONE,
		.range_end   = LLONG_MAX,
		.nr_to_write = nr_to_write,
		.skip_locked_pages = locked_pg,
	};

	writeback_inodes_sb(c->vfs_sb, &wbc);
	dbg_budg("%ld pages were written back", nr_to_write - wbc.nr_to_write);
	return nr_to_write - wbc.nr_to_write;
}

static int run_gc(struct ubifs_info *c)
{
	int err, lnum;

	/* Make some free space by garbage-collecting dirty space */
	down_read(&c->commit_sem);
	lnum = ubifs_garbage_collect(c, 1);
	up_read(&c->commit_sem);
	if (lnum < 0)
		return lnum;

	/* GC freed one LEB, return it to lprops */
	dbg_budg("GC freed LEB %d", lnum);
	err = ubifs_return_leb(c, lnum);
	if (err)
		return err;

	return 0;
}

int ubifs_make_free_space(struct ubifs_info *c, struct retries_info *ri,
			  int locked_pg)
{
	int err;

	/*
	 * If we have some dirty pages and inodes (liability), try to write
	 * them back unless this was tried too many times without effect
	 * already.
	 */
	if (ri->shrink_retries < MAX_SHRINK_RETRIES && !ri->try_gc) {
		long long liability;

		spin_lock(&c->space_lock);
		liability = c->budg_idx_growth + c->budg_data_growth +
			    c->budg_dd_growth;
		spin_unlock(&c->space_lock);

		if (ri->prev_liability >= liability) {
			/* Liability does not shrink, next time try GC then */
			ri->shrink_retries += 1;
			if (ri->gc_retries < MAX_GC_RETRIES)
				ri->try_gc = 1;
			dbg_budg("liability did not shrink: retries %d of %d",
				 ri->shrink_retries, MAX_SHRINK_RETRIES);
		}

		dbg_budg("force write-back (count %d)", ri->shrink_cnt);
		shrink_liability(c, NR_TO_WRITE + ri->shrink_cnt, locked_pg);

		ri->prev_liability = liability;
		ri->shrink_cnt += 1;
		return -EAGAIN;
	}

	/*
	 * Try to run garbage collector unless it was already tried too many
	 * times.
	 */
	if (ri->gc_retries < MAX_GC_RETRIES) {
		ri->gc_retries += 1;
		dbg_budg("run GC, retries %d of %d",
			 ri->gc_retries, MAX_GC_RETRIES);

		ri->try_gc = 0;
		err = run_gc(c);
		if (!err)
			return -EAGAIN;

		if (err == -EAGAIN) {
			dbg_budg("GC asked to commit");
			err = ubifs_run_commit(c);
			if (err)
				return err;
			return -EAGAIN;
		}

		if (err != -ENOSPC)
			return err;

		/*
		 * GC could not make any progress. If this is the first time,
		 * then it makes sense to try to commit, because it might make
		 * some dirty space.
		 */
		dbg_budg("GC returned -ENOSPC, retries %d",
			 ri->nospc_retries);
		if (ri->nospc_retries >= MAX_NOSPC_RETRIES)
			return err;
		ri->nospc_retries += 1;
	}

	/* Neither GC nor write-back helped, try to commit */
	if (ri->cmt_retries < MAX_CMT_RETRIES) {
		ri->cmt_retries += 1;
		dbg_budg("run commit, retries %d of %d",
			 ri->cmt_retries, MAX_CMT_RETRIES);
		err = ubifs_run_commit(c);
		if (err)
			return err;
		return -EAGAIN;
	}

	return -ENOSPC;
}

#endif /* UBIFS_COMPAT_USE_OLD_PREPARE_WRITE */
