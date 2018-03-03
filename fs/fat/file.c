/* 2012-08-06: File changed by Sony Corporation */
/*
 *  linux/fs/fat/file.c
 *
 *  Written 1992,1993 by Werner Almesberger
 *
 *  regular file handling primitives for fat-based filesystems
 */

#include <linux/capability.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/backing-dev.h>
#include <linux/blkdev.h>
#include <linux/fsnotify.h>
#include <linux/security.h>
#ifdef CONFIG_SNSC_FS_FAT_IOCTL_CONCAT_DIRTY_PAGES
#include <linux/pagevec.h>
#endif
#include "fat.h"

#ifdef CONFIG_SNSC_FS_FAT_IOCTL_CONCAT_DIRTY_PAGES
static int fat_dirty_page_range(struct inode *inode, struct file *filp,
				pgoff_t from, pgoff_t to)
{
	struct address_space *mapping = inode->i_mapping;
	const struct address_space_operations *aops = mapping->a_ops;
	pgoff_t index;
	struct page *page;
	int ret = 0;

	for (index = from; index <= to; index++) {
		void *fsdata = NULL;
		loff_t pos = index << PAGE_CACHE_SHIFT;

		cond_resched();
try_again:
		ret = pagecache_write_begin(filp, mapping, pos, PAGE_CACHE_SIZE,
					    0, &page, &fsdata);
		if (ret)
			goto fail;

		if (!PageUptodate(page)) {
			ret = aops->readpage(filp, page);
			if (unlikely(ret)) {
				page_cache_release(page);
				if (ret == AOP_TRUNCATED_PAGE) {
					goto try_again;
				}
				goto fail;
			}

			ret = lock_page_killable(page);
			if (unlikely(ret))
				goto fail;
			if (!PageUptodate(page)) {
				if (page->mapping == NULL) {
					unlock_page(page);
					page_cache_release(page);
					goto try_again;
				}
				ret = -EIO;
				goto unlock;
			}
		}

		ret = pagecache_write_end(filp, mapping, pos, PAGE_CACHE_SIZE,
					  PAGE_CACHE_SIZE, page, fsdata);
		if (ret < 0 || ret != PAGE_CACHE_SIZE)
			goto fail;
	}
	ret = 0;
out:
	return ret;

unlock:
	unlock_page(page);
	page_cache_release(page);
fail:
	ret = -1;
	goto out;
}

static int fat_concat_dirty_pages(struct inode *inode, struct file *filp)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	struct address_space *mapping = inode->i_mapping;
	struct pagevec pvec;
	int nr_pages;
	int i, err = 0;
	pgoff_t lindex, last;
	pgoff_t from, to;
	int pages_per_clus;

	if (sbi->cluster_size <= PAGE_CACHE_SIZE)
		return 0;

	pages_per_clus = sbi->cluster_size >> PAGE_CACHE_SHIFT;
	lindex = 0;
	last = ALIGN(inode->i_size, PAGE_CACHE_SIZE) >> PAGE_CACHE_SHIFT;
	from = to = (pgoff_t)-1;
	while (lindex < last) {
		pagevec_init(&pvec, 0);
		nr_pages = pagevec_lookup_tag(&pvec, mapping, &lindex,
					      PAGECACHE_TAG_DIRTY,
					      min(last - lindex,
						  (pgoff_t)PAGEVEC_SIZE));
		if (nr_pages == 0)
			break;

		i = 0;
		if (from == (pgoff_t)-1)
			from = pvec.pages[i++]->index;
		for (; i < nr_pages; i++) {
			pgoff_t index = pvec.pages[i]->index;

			if (index >= last) /* beyond the EOF */
				break;
			if (index >= ALIGN(from + 1, pages_per_clus)) {
				/* found a dirty page in other cluster */
				if (to != (pgoff_t)-1) {
					err = fat_dirty_page_range(inode, filp,
								   from, to);
					if (err) {
						pagevec_release(&pvec);
						return err;
					}
				}
				from = index;
				to = (pgoff_t)-1;
			} else {
				to = index;
			}
		}

		pagevec_release(&pvec);
	}
	if (to != (pgoff_t)-1)
		err = fat_dirty_page_range(inode, filp, from, to);

	return err;
}
#endif

#ifdef CONFIG_SNSC_FS_FAT_IOCTL_EXPAND_SIZE
static int fat_expand_size(struct inode *inode, loff_t size)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	const unsigned int cluster_size = sbi->cluster_size;
	int i, err = 0;
	int cluster, nr_clusters;

	if (IS_RDONLY(inode))
		return -EROFS;
	nr_clusters = (size + (cluster_size - 1)) >> sbi->cluster_bits;
	for (i = 0; i < nr_clusters; i++) {
		err = fat_alloc_clusters(inode, &cluster, 1);
		if (err)
			break;
		err = fat_chain_add(inode, cluster, 1);
		if (err)
			break;
	}
	if (err) {
		if (MSDOS_I(inode)->i_start != 0) {
			fat_free_clusters(inode, MSDOS_I(inode)->i_start);
			MSDOS_I(inode)->i_start = 0;
			MSDOS_I(inode)->i_logstart = 0;
			inode->i_blocks = 0;
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
			MSDOS_I(inode)->i_last_dclus = 0;
			MSDOS_I(inode)->i_new = 0;
			MSDOS_I(inode)->i_new_dclus = 0;
#endif
		}
	} else {
		MSDOS_I(inode)->mmu_private = size;
		i_size_write(inode, size);
		inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
		mark_inode_dirty(inode);
	}

	return err;
}
#endif

static int fat_ioctl_get_attributes(struct inode *inode, u32 __user *user_attr)
{
	u32 attr;

	mutex_lock(&inode->i_mutex);
	attr = fat_make_attrs(inode);
	mutex_unlock(&inode->i_mutex);

	return put_user(attr, user_attr);
}

static int fat_ioctl_set_attributes(struct file *file, u32 __user *user_attr)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	int is_dir = S_ISDIR(inode->i_mode);
	u32 attr, oldattr;
	struct iattr ia;
	int err;

	err = get_user(attr, user_attr);
	if (err)
		goto out;

	mutex_lock(&inode->i_mutex);
	err = mnt_want_write(file->f_path.mnt);
	if (err)
		goto out_unlock_inode;

	/*
	 * ATTR_VOLUME and ATTR_DIR cannot be changed; this also
	 * prevents the user from turning us into a VFAT
	 * longname entry.  Also, we obviously can't set
	 * any of the NTFS attributes in the high 24 bits.
	 */
	attr &= 0xff & ~(ATTR_VOLUME | ATTR_DIR);
	/* Merge in ATTR_VOLUME and ATTR_DIR */
	attr |= (MSDOS_I(inode)->i_attrs & ATTR_VOLUME) |
		(is_dir ? ATTR_DIR : 0);
	oldattr = fat_make_attrs(inode);

	/* Equivalent to a chmod() */
	ia.ia_valid = ATTR_MODE | ATTR_CTIME;
	ia.ia_ctime = current_fs_time(inode->i_sb);
	if (is_dir)
		ia.ia_mode = fat_make_mode(sbi, attr, S_IRWXUGO);
	else {
		ia.ia_mode = fat_make_mode(sbi, attr,
			S_IRUGO | S_IWUGO | (inode->i_mode & S_IXUGO));
	}

	/* The root directory has no attributes */
	if (inode->i_ino == MSDOS_ROOT_INO && attr != ATTR_DIR) {
		err = -EINVAL;
		goto out_drop_write;
	}

	if (sbi->options.sys_immutable &&
	    ((attr | oldattr) & ATTR_SYS) &&
	    !capable(CAP_LINUX_IMMUTABLE)) {
		err = -EPERM;
		goto out_drop_write;
	}

	/*
	 * The security check is questionable...  We single
	 * out the RO attribute for checking by the security
	 * module, just because it maps to a file mode.
	 */
	err = security_inode_setattr(file->f_path.dentry, &ia);
	if (err)
		goto out_drop_write;

	/* This MUST be done before doing anything irreversible... */
	err = fat_setattr(file->f_path.dentry, &ia);
	if (err)
		goto out_drop_write;

	fsnotify_change(file->f_path.dentry, ia.ia_valid);
	if (sbi->options.sys_immutable) {
		if (attr & ATTR_SYS)
			inode->i_flags |= S_IMMUTABLE;
		else
			inode->i_flags &= ~S_IMMUTABLE;
	}

	fat_save_attrs(inode, attr);
	mark_inode_dirty(inode);
out_drop_write:
	mnt_drop_write(file->f_path.mnt);
out_unlock_inode:
	mutex_unlock(&inode->i_mutex);
out:
	return err;
}

long fat_generic_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = filp->f_path.dentry->d_inode;
	u32 __user *user_attr = (u32 __user *)arg;

	switch (cmd) {
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	case FAT_IOCTL_CLOSE_NOSYNC:
	{
		filp->f_op = &fat_file_operations_nosync;
		return 0;
	}
#endif
	case FAT_IOCTL_GET_ATTRIBUTES:
		return fat_ioctl_get_attributes(inode, user_attr);
	case FAT_IOCTL_SET_ATTRIBUTES:
		return fat_ioctl_set_attributes(filp, user_attr);
#ifdef CONFIG_SNSC_FS_FAT_IOCTL_EXPAND_SIZE
	case FAT_IOCTL_EXPAND_SIZE:
	{
		int err;
		u32 size;

		if (!(filp->f_mode & FMODE_WRITE))
			return -EBADF;
		err = get_user(size, user_attr);
		if (err)
			return err;

		if (S_ISDIR(inode->i_mode) || (i_size_read(inode) != 0))
			return -EINVAL;

		mutex_lock(&inode->i_mutex);
		err = fat_expand_size(inode, size);
		mutex_unlock(&inode->i_mutex);
		return err;
	}
#endif
#ifdef CONFIG_SNSC_FS_FAT_IOCTL_CONCAT_DIRTY_PAGES
	case FAT_IOCTL_CONCAT_DIRTY_PAGES:
	{
		int err;

		if (!(filp->f_mode & FMODE_WRITE))
			return -EBADF;
		if (S_ISDIR(inode->i_mode))
			return -EINVAL;
		if (i_size_read(inode) == 0)
			return 0;

		mutex_lock(&inode->i_mutex);
		err = fat_concat_dirty_pages(inode, filp);
		mutex_unlock(&inode->i_mutex);
		return err;
	}
#endif
	default:
		return -ENOTTY;	/* Inappropriate ioctl for device */
	}
}

#ifdef CONFIG_COMPAT
static long fat_generic_compat_ioctl(struct file *filp, unsigned int cmd,
				      unsigned long arg)

{
	return fat_generic_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
static int fat_clean_inode(struct inode *inode)
{
	int ret = 0;

	if (!(inode->i_state & I_DIRTY))
		return 0;

	ret = fat_sync_inode(inode);
	if (!ret) {
		pr_debug("Sync inode ok, make it clean!\n");
		fat_mark_inode_clean(inode);
	}

	return ret;
}
#endif

static int fat_file_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	struct msdos_sb_info *sbi = MSDOS_SB(filp->f_mapping->host->i_sb);
	int ret = 0;
#endif

	if ((filp->f_mode & FMODE_WRITE) &&
#ifdef CONFIG_SNSC_FS_FAT_FLUSH_NO_WAIT_AT_CLOSING_CLEAN_FILES
	    (inode->i_state & I_DIRTY) &&
#endif
	     MSDOS_SB(inode->i_sb)->options.flush) {
		fat_flush_inodes(inode->i_sb, inode, NULL);
		congestion_wait(BLK_RW_SYNC, HZ/10);
	}
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (sbi->options.batch_sync) {
		mutex_lock(&inode->i_mutex);
		ret = fat_clean_inode(inode);
		mutex_unlock(&inode->i_mutex);
	}
	return ret;
#else
	return 0;
#endif
}

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
static int fat_file_release_nosync(struct inode *inode, struct file *filp)
{
	if ((filp->f_mode & FMODE_WRITE) &&
#ifdef CONFIG_SNSC_FS_FAT_FLUSH_NO_WAIT_AT_CLOSING_CLEAN_FILES
	    (inode->i_state & I_DIRTY) &&
#endif
	     MSDOS_SB(inode->i_sb)->options.flush) {
		fat_flush_inodes(inode->i_sb, inode, NULL);
		congestion_wait(BLK_RW_SYNC, HZ/10);
	}
	return 0;
}
#endif

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
static ssize_t fat_sync_read(struct file *filp, char __user *buf, size_t len,
			     loff_t *ppos)
{
	struct inode *inode = filp->f_mapping->host;
	if (fat_check_disk(inode->i_sb, 1))
		return -EIO;
	return do_sync_read(filp, buf, len, ppos);
}
#endif

#if defined(CONFIG_SNSC_FS_FAT_BATCH_SYNC) || defined(CONFIG_SNSC_FS_VFAT_CHECK_DISK)
static ssize_t fat_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	struct inode *inode = filp->f_mapping->host;
#endif
	ssize_t ret;

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (fat_check_disk(inode->i_sb, 1))
		return -EIO;
#endif
	ret = do_sync_write(filp, buf, len, ppos);

	return ret;
}
#endif

int fat_file_fsync(struct file *filp, int datasync)
{
	struct inode *inode = filp->f_mapping->host;
	int res, err;

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (fat_check_disk(inode->i_sb, 1))
		return -EIO;
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (MSDOS_SB(inode->i_sb)->options.batch_sync) {
		res = fat_clean_inode(inode);
		err = 0;
	} else {
		res = generic_file_fsync(filp, datasync);
		err = sync_mapping_buffers(MSDOS_SB(inode->i_sb)->fat_inode->i_mapping);
	}
#else
	res = generic_file_fsync(filp, datasync);
	err = sync_mapping_buffers(MSDOS_SB(inode->i_sb)->fat_inode->i_mapping);
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (res == 0 && err == 0 && fat_check_disk(inode->i_sb, 1))
		return -EIO;
#endif
	return res ? res : err;
}


const struct file_operations fat_file_operations = {
	.llseek		= generic_file_llseek,
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	.read		= fat_sync_read,
#else
	.read		= do_sync_read,
#endif
#if defined(CONFIG_SNSC_FS_FAT_BATCH_SYNC) || defined(CONFIG_SNSC_FS_VFAT_CHECK_DISK)
	.write		= fat_sync_write,
#else
	.write		= do_sync_write,
#endif
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	.open		= generic_file_open,
#endif
	.release	= fat_file_release,
	.unlocked_ioctl	= fat_generic_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fat_generic_compat_ioctl,
#endif
	.fsync		= fat_file_fsync,
	.splice_read	= generic_file_splice_read,
};

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
const struct file_operations fat_file_operations_nosync = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= fat_sync_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.open           = generic_file_open,
	.release	= fat_file_release_nosync,
	.unlocked_ioctl	= fat_generic_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fat_generic_compat_ioctl,
#endif
	.fsync		= fat_file_fsync,
};
#endif

static int fat_cont_expand(struct inode *inode, loff_t size)
{
	struct address_space *mapping = inode->i_mapping;
	loff_t start = inode->i_size, count = size - inode->i_size;
	int err;

	err = generic_cont_expand_simple(inode, size);
	if (err)
		goto out;

	inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);
	if (IS_SYNC(inode)) {
		int err2;

		/*
		 * Opencode syncing since we don't have a file open to use
		 * standard fsync path.
		 */
		err = filemap_fdatawrite_range(mapping, start,
					       start + count - 1);
		err2 = sync_mapping_buffers(mapping);
		if (!err)
			err = err2;
		err2 = write_inode_now(inode, 1);
		if (!err)
			err = err2;
		if (!err) {
			err =  filemap_fdatawait_range(mapping, start,
						       start + count - 1);
		}
	}
out:
	return err;
}

/* Free all clusters after the skip'th cluster. */
static int fat_free(struct inode *inode, int skip)
{
	struct super_block *sb = inode->i_sb;
	int err, wait, free_start, i_start, i_logstart;

	if (MSDOS_I(inode)->i_start == 0)
		return 0;

	fat_cache_inval_inode(inode);

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	wait = IS_DIRSYNC(inode) || MSDOS_SB(sb)->options.batch_sync;
#else
	wait = IS_DIRSYNC(inode);
#endif
	i_start = free_start = MSDOS_I(inode)->i_start;
	i_logstart = MSDOS_I(inode)->i_logstart;

	/* First, we write the new file size. */
	if (!skip) {
		MSDOS_I(inode)->i_start = 0;
		MSDOS_I(inode)->i_logstart = 0;
	}
	MSDOS_I(inode)->i_attrs |= ATTR_ARCH;
	inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
	if (wait) {
		err = fat_sync_inode(inode);
		if (err) {
			MSDOS_I(inode)->i_start = i_start;
			MSDOS_I(inode)->i_logstart = i_logstart;
			return err;
		}
	} else
		mark_inode_dirty(inode);

	/* Write a new EOF, and get the remaining cluster chain for freeing. */
	if (skip) {
		struct fat_entry fatent;
		int ret, fclus, dclus;

		ret = fat_get_cluster(inode, skip - 1, &fclus, &dclus);
		if (ret < 0)
			return ret;
		else if (ret == FAT_ENT_EOF)
			return 0;

		fatent_init(&fatent);
		ret = fat_ent_read(inode, &fatent, dclus);
		if (ret == FAT_ENT_EOF) {
			fatent_brelse(&fatent);
			return 0;
		} else if (ret == FAT_ENT_FREE) {
			fat_fs_error(sb,
				     "%s: invalid cluster chain (i_pos %lld)",
				     __func__, MSDOS_I(inode)->i_pos);
			ret = -EIO;
		} else if (ret > 0) {
			err = fat_ent_write(inode, &fatent, FAT_ENT_EOF, wait);
			if (err)
				ret = err;
		}
		fatent_brelse(&fatent);
		if (ret < 0)
			return ret;

		free_start = ret;
	}
	inode->i_blocks = skip << (MSDOS_SB(sb)->cluster_bits - 9);

	/* Freeing the remained cluster chain */
	return fat_free_clusters(inode, free_start);
}

void fat_truncate_blocks(struct inode *inode, loff_t offset)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	const unsigned int cluster_size = sbi->cluster_size;
	int nr_clusters;

	/*
	 * This protects against truncating a file bigger than it was then
	 * trying to write into the hole.
	 */
	if (MSDOS_I(inode)->mmu_private > offset)
		MSDOS_I(inode)->mmu_private = offset;

	nr_clusters = (offset + (cluster_size - 1)) >> sbi->cluster_bits;

	fat_free(inode, nr_clusters);
	fat_flush_inodes(inode->i_sb, inode, NULL);
}

int fat_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	generic_fillattr(inode, stat);
	stat->blksize = MSDOS_SB(inode->i_sb)->cluster_size;
	return 0;
}
EXPORT_SYMBOL_GPL(fat_getattr);

static int fat_sanitize_mode(const struct msdos_sb_info *sbi,
			     struct inode *inode, umode_t *mode_ptr)
{
	mode_t mask, perm;

	/*
	 * Note, the basic check is already done by a caller of
	 * (attr->ia_mode & ~FAT_VALID_MODE)
	 */

	if (S_ISREG(inode->i_mode))
		mask = sbi->options.fs_fmask;
	else
		mask = sbi->options.fs_dmask;

	perm = *mode_ptr & ~(S_IFMT | mask);

	/*
	 * Of the r and x bits, all (subject to umask) must be present. Of the
	 * w bits, either all (subject to umask) or none must be present.
	 *
	 * If fat_mode_can_hold_ro(inode) is false, can't change w bits.
	 */
	if ((perm & (S_IRUGO | S_IXUGO)) != (inode->i_mode & (S_IRUGO|S_IXUGO)))
		return -EPERM;
	if (fat_mode_can_hold_ro(inode)) {
		if ((perm & S_IWUGO) && ((perm & S_IWUGO) != (S_IWUGO & ~mask)))
			return -EPERM;
	} else {
		if ((perm & S_IWUGO) != (S_IWUGO & ~mask))
			return -EPERM;
	}

	*mode_ptr &= S_IFMT | perm;

	return 0;
}

static int fat_allow_set_time(struct msdos_sb_info *sbi, struct inode *inode)
{
	mode_t allow_utime = sbi->options.allow_utime;

	if (current_fsuid() != inode->i_uid) {
		if (in_group_p(inode->i_gid))
			allow_utime >>= 3;
		if (allow_utime & MAY_WRITE)
			return 1;
	}

	/* use a default check */
	return 0;
}

#define TIMES_SET_FLAGS	(ATTR_MTIME_SET | ATTR_ATIME_SET | ATTR_TIMES_SET)
/* valid file mode bits */
#define FAT_VALID_MODE	(S_IFREG | S_IFDIR | S_IRWXUGO)

int fat_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct msdos_sb_info *sbi = MSDOS_SB(dentry->d_sb);
	struct inode *inode = dentry->d_inode;
	unsigned int ia_valid;
	int error;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	int px_uid = sbi->options.fs_uid ? sbi->options.fs_uid :
		     CONFIG_SNSC_FS_VFAT_POSIX_ATTR_DEFAULT_UID_VALUE;
	int px_gid = sbi->options.fs_gid ? sbi->options.fs_gid :
		     CONFIG_SNSC_FS_VFAT_POSIX_ATTR_DEFAULT_UID_VALUE;
 	int need_size_change = 0;
#endif

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (fat_check_disk(inode->i_sb, 0))
		return -EIO;
#endif

	/* Check for setting the inode time. */
	ia_valid = attr->ia_valid;
	if (ia_valid & TIMES_SET_FLAGS) {
		if (fat_allow_set_time(sbi, inode))
			attr->ia_valid &= ~TIMES_SET_FLAGS;
	}

#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	if (sbi->options.posix_attr) {
		if (attr->ia_valid & ATTR_UID) {
			/* root can change uid to any value */
			if (capable(CAP_CHOWN) &&
			    (attr->ia_uid && attr->ia_uid != px_uid))
				attr->ia_uid = px_uid;
			/* change-uid affects gid */
			attr->ia_valid |= ATTR_GID;
			attr->ia_gid = attr->ia_uid ? px_gid : 0;
		} else {
			/* chown syscall sets both uid and gid */
			if (attr->ia_valid & ATTR_GID) {
				/* root can change gid to any value */
				if (capable(CAP_CHOWN) &&
				    (attr->ia_gid && attr->ia_gid != px_gid))
					attr->ia_gid = px_gid;
				/* change-gid affects uid */
				attr->ia_valid |= ATTR_UID;
				attr->ia_uid = attr->ia_gid ? px_uid : 0;
			}
		}
		/* change-group-mode affects on others-mode */
		if (attr->ia_valid & ATTR_MODE) {
			int others_mode = (attr->ia_mode & S_IRWXG) >> 3;
			attr->ia_mode &=  ~S_IRWXO;
			attr->ia_mode |=  others_mode;
		}
	}
#endif
#ifdef CONFIG_EJ_FS_IGNORE_UID_UNMATCH
	error = 0;
#else
	error = inode_change_ok(inode, attr);
#endif
	attr->ia_valid = ia_valid;
	if (error) {
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
		goto error_out;
#else
		if (sbi->options.quiet)
			error = 0;
		goto out;
#endif
	}

	/*
	 * Expand the file. Since inode_setattr() updates ->i_size
	 * before calling the ->truncate(), but FAT needs to fill the
	 * hole before it. XXX: this is no longer true with new truncate
	 * sequence.
	 */
	if (attr->ia_valid & ATTR_SIZE) {
		if (attr->ia_size > inode->i_size) {
			error = fat_cont_expand(inode, attr->ia_size);
			if (error || attr->ia_valid == ATTR_SIZE)
				goto out;
			attr->ia_valid &= ~ATTR_SIZE;
		}
	}

#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	if (sbi->options.posix_attr) {
		if (((attr->ia_valid & ATTR_UID) &&
		       ((attr->ia_uid) && (attr->ia_uid != px_uid))) ||
		    ((attr->ia_valid & ATTR_GID) &&
		       ((attr->ia_gid) && (attr->ia_gid != px_gid))) ||
		    ((attr->ia_valid & ATTR_MODE) &&
		     (attr->ia_mode & ~VFAT_POSIX_ATTR_VALID_MODE)))
			error = -EPERM;
	}
	else
#endif
	if (((attr->ia_valid & ATTR_UID) &&
	     (attr->ia_uid != sbi->options.fs_uid)) ||
	    ((attr->ia_valid & ATTR_GID) &&
	     (attr->ia_gid != sbi->options.fs_gid)) ||
	    ((attr->ia_valid & ATTR_MODE) &&
	     (attr->ia_mode & ~FAT_VALID_MODE)))
		error = -EPERM;

#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	if (error)
		goto error_out;

	if (attr->ia_valid & ATTR_SIZE) {
		/*
		 * Update i_size here, not in inode_setattr(),
		 * because inode_setattr() calls vmtruncate() and
		 * fat_truncate() consequently. This may release
		 * cluster chain, before updating size field of
		 * the dir entry on disk.
		 */
		if (IS_SWAPFILE(inode)){
			error = -ETXTBSY;
			goto out;
		}
		attr->ia_valid &= ~(ATTR_SIZE);
		if (attr->ia_size == inode->i_size) {
			attr->ia_valid |= ATTR_MTIME|ATTR_CTIME;
		} else {
			need_size_change = 1;
			i_size_write(inode, attr->ia_size);
		}
	}

	if (!sbi->options.posix_attr) {
		if (attr->ia_valid & ATTR_MODE) {
			if (fat_sanitize_mode(sbi, inode, &attr->ia_mode) < 0)
				attr->ia_valid &= ~ATTR_MODE;
		}
	}

	/*
	 * Here, no error returns from inode_setattr().
	 * Because ATTR_SIZE is cleared before calling it.
	 */
	setattr_copy(inode, attr);
	mark_inode_dirty(inode);

	if (need_size_change) {
		/* inode is already marked dirty in inode_setattr() */
		if (IS_SYNC(inode))
			generic_osync_inode_only(inode);
		truncate_setsize(inode, inode->i_size);
		fat_truncate_blocks(inode, attr->ia_size);
	}
#else
	if (error) {
		if (sbi->options.quiet)
			error = 0;
		goto out;
	}

	/*
	 * We don't return -EPERM here. Yes, strange, but this is too
	 * old behavior.
	 */
	if (attr->ia_valid & ATTR_MODE) {
		if (fat_sanitize_mode(sbi, inode, &attr->ia_mode) < 0)
			attr->ia_valid &= ~ATTR_MODE;
	}

	if (attr->ia_valid & ATTR_SIZE) {
		truncate_setsize(inode, attr->ia_size);
		fat_truncate_blocks(inode, attr->ia_size);
	}

	setattr_copy(inode, attr);
	mark_inode_dirty(inode);
#endif
out:
	return error;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
error_out:
	if (sbi->options.quiet)
		error = 0;
	return error;
#endif
}
EXPORT_SYMBOL_GPL(fat_setattr);

const struct inode_operations fat_file_inode_operations = {
	.setattr	= fat_setattr,
	.getattr	= fat_getattr,
};
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
struct inode_operations fat_symlink_inode_operations = {
       .readlink       = page_readlink,
       .follow_link    = page_follow_link_light,
       .setattr        = fat_setattr,
};

EXPORT_SYMBOL(fat_symlink_inode_operations);
#endif
