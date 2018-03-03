/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/fs/fat/inode.c
 *
 *  Written 1992,1993 by Werner Almesberger
 *  VFAT extensions by Gordon Chaffee, merged with msdos fs by Henrik Storner
 *  Rewritten for the constant inumbers support by Al Viro
 *
 *  Fixes:
 *
 *	Max Cohan: Fixed invalid FSINFO offset when info_sector is 0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/pagemap.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/mount.h>
#include <linux/vfs.h>
#include <linux/parser.h>
#include <linux/uio.h>
#include <linux/writeback.h>
#include <linux/log2.h>
#include <linux/hash.h>
#include <linux/blkdev.h>
#include <asm/unaligned.h>
#include "fat.h"

#ifndef CONFIG_FAT_DEFAULT_IOCHARSET
/* if user don't select VFAT, this is undefined. */
#define CONFIG_FAT_DEFAULT_IOCHARSET	""
#endif

static int fat_default_codepage = CONFIG_FAT_DEFAULT_CODEPAGE;
static char fat_default_iocharset[] = CONFIG_FAT_DEFAULT_IOCHARSET;


static int fat_add_cluster(struct inode *inode)
{
	int err, cluster;

	err = fat_alloc_clusters(inode, &cluster, 1);
	if (err)
		return err;
	/* FIXME: this cluster should be added after data of this
	 * cluster is writed */
	err = fat_chain_add(inode, cluster, 1);
	if (err)
		fat_free_clusters(inode, cluster);
	return err;
}

static inline int __fat_get_block(struct inode *inode, sector_t iblock,
				  unsigned long *max_blocks,
				  struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	unsigned long mapped_blocks;
	sector_t phys;
	int err, offset;

	err = fat_bmap(inode, iblock, &phys, &mapped_blocks, create);
	if (err)
		return err;
	if (phys) {
		map_bh(bh_result, sb, phys);
		*max_blocks = min(mapped_blocks, *max_blocks);
		return 0;
	}
	if (!create)
		return 0;

	if (iblock != MSDOS_I(inode)->mmu_private >> sb->s_blocksize_bits) {
		fat_fs_error(sb, "corrupted file size (i_pos %lld, %lld)",
			MSDOS_I(inode)->i_pos, MSDOS_I(inode)->mmu_private);
		return -EIO;
	}

	offset = (unsigned long)iblock & (sbi->sec_per_clus - 1);
	if (!offset) {
		/* TODO: multiple cluster allocation would be desirable. */
		err = fat_add_cluster(inode);
		if (err)
			return err;
	}
	/* available blocks on this cluster */
	mapped_blocks = sbi->sec_per_clus - offset;

	*max_blocks = min(mapped_blocks, *max_blocks);
	MSDOS_I(inode)->mmu_private += *max_blocks << sb->s_blocksize_bits;

	err = fat_bmap(inode, iblock, &phys, &mapped_blocks, create);
	if (err)
		return err;

	BUG_ON(!phys);
	BUG_ON(*max_blocks != mapped_blocks);
	set_buffer_new(bh_result);
	map_bh(bh_result, sb, phys);

	return 0;
}

static int fat_get_block(struct inode *inode, sector_t iblock,
			 struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	unsigned long max_blocks = bh_result->b_size >> inode->i_blkbits;
	int err;

	err = __fat_get_block(inode, iblock, &max_blocks, bh_result, create);
	if (err)
		return err;
	bh_result->b_size = max_blocks << sb->s_blocksize_bits;
	return 0;
}

static int fat_writepage(struct page *page, struct writeback_control *wbc)
{
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	struct inode *inode = page->mapping->host;
	if (fat_check_disk(inode->i_sb, 0)) {
		SetPageError(page);
		ClearPageUptodate(page);
		unlock_page(page);
		return -EIO;
	}
#endif
	return block_write_full_page(page, fat_get_block, wbc);
}

static int fat_writepages(struct address_space *mapping,
			  struct writeback_control *wbc)
{
	return mpage_writepages(mapping, wbc, fat_get_block);
}

static int fat_readpage(struct file *file, struct page *page)
{
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	struct inode *inode = page->mapping->host;
	if (fat_check_disk(inode->i_sb, 0)) {
		SetPageError(page);
		ClearPageUptodate(page);
		unlock_page(page);
		return -EIO;
	}
#endif
	return mpage_readpage(page, fat_get_block);
}

static int fat_readpages(struct file *file, struct address_space *mapping,
			 struct list_head *pages, unsigned nr_pages)
{
	return mpage_readpages(mapping, pages, nr_pages, fat_get_block);
}

static void fat_write_failed(struct address_space *mapping, loff_t to)
{
	struct inode *inode = mapping->host;

	if (to > inode->i_size) {
		truncate_pagecache(inode, to, inode->i_size);
		fat_truncate_blocks(inode, inode->i_size);
	}
}

static int fat_write_begin(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{
	int err;
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	struct inode *inode = mapping->host;
	if (fat_check_disk(inode->i_sb, 0))
		return -EIO;
#endif
	*pagep = NULL;
	err = cont_write_begin(file, mapping, pos, len, flags,
				pagep, fsdata, fat_get_block,
				&MSDOS_I(mapping->host)->mmu_private);
	if (err < 0)
		fat_write_failed(mapping, pos + len);
	return err;
}

static int fat_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *pagep, void *fsdata)
{
	struct inode *inode = mapping->host;
	int err;
	err = generic_write_end(file, mapping, pos, len, copied, pagep, fsdata);
	if (err < len)
		fat_write_failed(mapping, pos + len);
	if (!(err < 0) && !(MSDOS_I(inode)->i_attrs & ATTR_ARCH)) {
		inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
		MSDOS_I(inode)->i_attrs |= ATTR_ARCH;
		mark_inode_dirty(inode);
	}
	return err;
}

static ssize_t fat_direct_IO(int rw, struct kiocb *iocb,
			     const struct iovec *iov,
			     loff_t offset, unsigned long nr_segs)
{
	struct file *file = iocb->ki_filp;
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	ssize_t ret;

	if (rw == WRITE) {
		/*
		 * FIXME: blockdev_direct_IO() doesn't use ->write_begin(),
		 * so we need to update the ->mmu_private to block boundary.
		 *
		 * But we must fill the remaining area or hole by nul for
		 * updating ->mmu_private.
		 *
		 * Return 0, and fallback to normal buffered write.
		 */
		loff_t size = offset + iov_length(iov, nr_segs);
		if (MSDOS_I(inode)->mmu_private < size)
			return 0;
	}

	/*
	 * FAT need to use the DIO_LOCKING for avoiding the race
	 * condition of fat_get_block() and ->truncate().
	 */
	ret = blockdev_direct_IO(rw, iocb, inode, inode->i_sb->s_bdev,
				 iov, offset, nr_segs, fat_get_block, NULL);
	if (ret < 0 && (rw & WRITE))
		fat_write_failed(mapping, offset + iov_length(iov, nr_segs));

	return ret;
}

static sector_t _fat_bmap(struct address_space *mapping, sector_t block)
{
	sector_t blocknr;

	/* fat_get_cluster() assumes the requested blocknr isn't truncated. */
	anon_down_read(&mapping->host->i_alloc_sem);
	blocknr = generic_block_bmap(mapping, block, fat_get_block);
	anon_up_read(&mapping->host->i_alloc_sem);

	return blocknr;
}

static const struct address_space_operations fat_aops = {
	.readpage	= fat_readpage,
	.readpages	= fat_readpages,
	.writepage	= fat_writepage,
	.writepages	= fat_writepages,
	.write_begin	= fat_write_begin,
	.write_end	= fat_write_end,
	.direct_IO	= fat_direct_IO,
	.bmap		= _fat_bmap
};

/*
 * New FAT inode stuff. We do the following:
 *	a) i_ino is constant and has nothing with on-disk location.
 *	b) FAT manages its own cache of directory entries.
 *	c) *This* cache is indexed by on-disk location.
 *	d) inode has an associated directory entry, all right, but
 *		it may be unhashed.
 *	e) currently entries are stored within struct inode. That should
 *		change.
 *	f) we deal with races in the following way:
 *		1. readdir() and lookup() do FAT-dir-cache lookup.
 *		2. rename() unhashes the F-d-c entry and rehashes it in
 *			a new place.
 *		3. unlink() and rmdir() unhash F-d-c entry.
 *		4. fat_write_inode() checks whether the thing is unhashed.
 *			If it is we silently return. If it isn't we do bread(),
 *			check if the location is still valid and retry if it
 *			isn't. Otherwise we do changes.
 *		5. Spinlock is used to protect hash/unhash/location check/lookup
 *		6. fat_evict_inode() unhashes the F-d-c entry.
 *		7. lookup() and readdir() do igrab() if they find a F-d-c entry
 *			and consider negative result as cache miss.
 */

static void fat_hash_init(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	int i;

	spin_lock_init(&sbi->inode_hash_lock);
	for (i = 0; i < FAT_HASH_SIZE; i++)
		INIT_HLIST_HEAD(&sbi->inode_hashtable[i]);
}

static inline unsigned long fat_hash(loff_t i_pos)
{
	return hash_32(i_pos, FAT_HASH_BITS);
}

void fat_attach(struct inode *inode, loff_t i_pos)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	struct hlist_head *head = sbi->inode_hashtable + fat_hash(i_pos);

	spin_lock(&sbi->inode_hash_lock);
	MSDOS_I(inode)->i_pos = i_pos;
	hlist_add_head(&MSDOS_I(inode)->i_fat_hash, head);
	spin_unlock(&sbi->inode_hash_lock);
}
EXPORT_SYMBOL_GPL(fat_attach);

void fat_detach(struct inode *inode)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	spin_lock(&sbi->inode_hash_lock);
	MSDOS_I(inode)->i_pos = 0;
	hlist_del_init(&MSDOS_I(inode)->i_fat_hash);
	spin_unlock(&sbi->inode_hash_lock);
}
EXPORT_SYMBOL_GPL(fat_detach);

struct inode *fat_iget(struct super_block *sb, loff_t i_pos)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct hlist_head *head = sbi->inode_hashtable + fat_hash(i_pos);
	struct hlist_node *_p;
	struct msdos_inode_info *i;
	struct inode *inode = NULL;

	spin_lock(&sbi->inode_hash_lock);
	hlist_for_each_entry(i, _p, head, i_fat_hash) {
		BUG_ON(i->vfs_inode.i_sb != sb);
		if (i->i_pos != i_pos)
			continue;
		inode = igrab(&i->vfs_inode);
		if (inode)
			break;
	}
	spin_unlock(&sbi->inode_hash_lock);
	return inode;
}

static int is_exec(unsigned char *extension)
{
	unsigned char *exe_extensions = "EXECOMBAT", *walk;

	for (walk = exe_extensions; *walk; walk += 3)
		if (!strncmp(extension, walk, 3))
			return 1;
	return 0;
}

static int fat_calc_dir_size(struct inode *inode)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	int ret, fclus, dclus;

	inode->i_size = 0;
	if (MSDOS_I(inode)->i_start == 0)
		return 0;

	ret = fat_get_cluster(inode, FAT_ENT_EOF, &fclus, &dclus);
	if (ret < 0)
		return ret;
	inode->i_size = (fclus + 1) << sbi->cluster_bits;

	return 0;
}

/* doesn't deal with root inode */
static int fat_fill_inode(struct inode *inode, struct msdos_dir_entry *de)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	int error;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR_CREATE_FILE_WITH_DEFAULT_UID
	int px_uid, px_gid;
#endif

	MSDOS_I(inode)->i_pos = 0;
	inode->i_uid = sbi->options.fs_uid;
	inode->i_gid = sbi->options.fs_gid;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR_CREATE_FILE_WITH_DEFAULT_UID
	if (sbi->options.posix_attr) {
		px_uid = sbi->options.fs_uid ? sbi->options.fs_uid :
			CONFIG_SNSC_FS_VFAT_POSIX_ATTR_DEFAULT_UID_VALUE;
		px_gid = sbi->options.fs_gid ? sbi->options.fs_gid :
			CONFIG_SNSC_FS_VFAT_POSIX_ATTR_DEFAULT_UID_VALUE;
		inode->i_uid = current_fsuid() ? px_uid : 0;
		inode->i_gid = current_fsuid() ? px_gid : 0;
	}
#endif
	inode->i_version++;
	inode->i_generation = get_seconds();
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	MSDOS_I(inode)->i_last_dclus = 0;
	MSDOS_I(inode)->i_new = 0;
	MSDOS_I(inode)->i_new_dclus = 0;
#endif

	if ((de->attr & ATTR_DIR) && !IS_FREE(de->name)) {
		inode->i_generation &= ~1;
		inode->i_mode = fat_make_mode(sbi, de->attr, S_IRWXUGO);
		inode->i_op = sbi->dir_ops;
		inode->i_fop = &fat_dir_operations;

		MSDOS_I(inode)->i_start = le16_to_cpu(de->start);
		if (sbi->fat_bits == 32)
			MSDOS_I(inode)->i_start |= (le16_to_cpu(de->starthi) << 16);

		MSDOS_I(inode)->i_logstart = MSDOS_I(inode)->i_start;
		error = fat_calc_dir_size(inode);
		if (error < 0)
			return error;
		MSDOS_I(inode)->mmu_private = inode->i_size;

		inode->i_nlink = fat_subdirs(inode);
	} else { /* not a directory */
		inode->i_generation |= 1;
		inode->i_mode = fat_make_mode(sbi, de->attr,
			((sbi->options.showexec && !is_exec(de->name + 8))
			 ? S_IRUGO|S_IWUGO : S_IRWXUGO));
		MSDOS_I(inode)->i_start = le16_to_cpu(de->start);
		if (sbi->fat_bits == 32)
			MSDOS_I(inode)->i_start |= (le16_to_cpu(de->starthi) << 16);

		MSDOS_I(inode)->i_logstart = MSDOS_I(inode)->i_start;
		inode->i_size = le32_to_cpu(de->size);
		inode->i_op = &fat_file_inode_operations;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
		if (sbi->options.posix_attr
			&& sbi->posix_ops.is_symlink(inode, de)){
				inode->i_op = &fat_symlink_inode_operations;
		}
#endif
		inode->i_fop = &fat_file_operations;
		inode->i_mapping->a_ops = &fat_aops;
		MSDOS_I(inode)->mmu_private = inode->i_size;
	}
	if (de->attr & ATTR_SYS) {
		if (sbi->options.sys_immutable)
			inode->i_flags |= S_IMMUTABLE;
	}
	fat_save_attrs(inode, de->attr);

	inode->i_blocks = ((inode->i_size + (sbi->cluster_size - 1))
			   & ~((loff_t)sbi->cluster_size - 1)) >> 9;

	fat_time_fat2unix(sbi, &inode->i_mtime, de->time, de->date, 0);
	if (sbi->options.isvfat) {
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
		if (sbi->options.ignore_crtime)
			inode->i_ctime = inode->i_mtime;
		else
#endif
		fat_time_fat2unix(sbi, &inode->i_ctime, de->ctime,
				  de->cdate, de->ctime_cs);
		fat_time_fat2unix(sbi, &inode->i_atime, 0, de->adate, 0);
	} else
		inode->i_ctime = inode->i_atime = inode->i_mtime;

	return 0;
}

struct inode *fat_build_inode(struct super_block *sb,
			struct msdos_dir_entry *de, loff_t i_pos)
{
	struct inode *inode;
	int err;

	inode = fat_iget(sb, i_pos);
	if (inode)
		goto out;
	inode = new_inode(sb);
	if (!inode) {
		inode = ERR_PTR(-ENOMEM);
		goto out;
	}
	inode->i_ino = iunique(sb, MSDOS_ROOT_INO);
	inode->i_version = 1;
	err = fat_fill_inode(inode, de);
	if (err) {
		iput(inode);
		inode = ERR_PTR(err);
		goto out;
	}
	fat_attach(inode, i_pos);
	insert_inode_hash(inode);
out:
	return inode;
}

EXPORT_SYMBOL_GPL(fat_build_inode);

#ifdef CONFIG_SNSC_FS_FAT_LOOKUP_HINT
extern void fat_lkup_hint_inval(struct inode*);
#endif

static void fat_evict_inode(struct inode *inode)
{
	truncate_inode_pages(&inode->i_data, 0);
	if (!inode->i_nlink) {
		inode->i_size = 0;
		fat_truncate_blocks(inode, 0);
	}
	invalidate_inode_buffers(inode);
	end_writeback(inode);
	fat_cache_inval_inode(inode);
#ifdef CONFIG_SNSC_FS_FAT_LOOKUP_HINT
	fat_lkup_hint_inval(inode);
#endif
	fat_detach(inode);
}

static void fat_write_super(struct super_block *sb)
{
	lock_super(sb);
	sb->s_dirt = 0;

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (!fat_check_disk(sb, 0)) {
		if (!(sb->s_flags & MS_RDONLY))
			fat_clusters_flush(sb);
	}
#else
	if (!(sb->s_flags & MS_RDONLY))
		fat_clusters_flush(sb);
#endif
	unlock_super(sb);
}

static int fat_sync_fs(struct super_block *sb, int wait)
{
	int err = 0;

	if (sb->s_dirt) {
		lock_super(sb);
		sb->s_dirt = 0;
		err = fat_clusters_flush(sb);
		unlock_super(sb);
	}

	return err;
}

static void fat_put_super(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);

#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
	if (sbi->options.clnshutbit && (sbi->clnshutbit & 1)) {
		unsigned int cn;
		struct fat_entry fatent;

#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
		if (fat_check_disk(sb, 0))
			goto set_clnshutbit_failed;
#endif
		fatent_init(&fatent);
		cn = fat_ent_raw_read(sb, &fatent, 1);
		if (cn == 0)
			goto set_clnshutbit_failed;
		if (sbi->fat_bits == 32) {
			if (!(cn & 0x08000000) &&
			    fat_ent_raw_write(sb, &fatent, cn | 0x08000000) != 0)
				goto set_clnshutbit_failed;
		}
		else if (sbi->fat_bits == 16) {
			if (!(cn & 0x8000) &&
			    fat_ent_raw_write(sb, &fatent, cn | 0x8000) != 0)
				goto set_clnshutbit_failed;
		}
		if (0) {
set_clnshutbit_failed:
			printk("FAT: failed to set ClnShutBit\n");
		}
	}
#endif

	if (sb->s_dirt)
		fat_write_super(sb);

	iput(sbi->fat_inode);

	unload_nls(sbi->nls_disk);
	unload_nls(sbi->nls_io);

	if (sbi->options.iocharset != fat_default_iocharset)
		kfree(sbi->options.iocharset);

#ifdef CONFIG_SNSC_FS_FAT_GC
	fat_stop_gc(sb);
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	if (sbi->options.check_disk)
		k3d_put_disk(sb->s_bdev->bd_disk);
#endif

	sb->s_fs_info = NULL;
	kfree(sbi);
}

static struct kmem_cache *fat_inode_cachep;

static struct inode *fat_alloc_inode(struct super_block *sb)
{
	struct msdos_inode_info *ei;
	ei = kmem_cache_alloc(fat_inode_cachep, GFP_NOFS);
	if (!ei)
		return NULL;
	return &ei->vfs_inode;
}

static void fat_i_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
	INIT_LIST_HEAD(&inode->i_dentry);
	kmem_cache_free(fat_inode_cachep, MSDOS_I(inode));
}

static void fat_destroy_inode(struct inode *inode)
{
	call_rcu(&inode->i_rcu, fat_i_callback);
}

static void init_once(void *foo)
{
	struct msdos_inode_info *ei = (struct msdos_inode_info *)foo;

	spin_lock_init(&ei->cache_lru_lock);
	ei->nr_caches = 0;
	ei->cache_valid_id = FAT_CACHE_VALID + 1;
	INIT_LIST_HEAD(&ei->cache_lru);
	INIT_HLIST_NODE(&ei->i_fat_hash);
	inode_init_once(&ei->vfs_inode);
}

static int __init fat_init_inodecache(void)
{
	fat_inode_cachep = kmem_cache_create("fat_inode_cache",
					     sizeof(struct msdos_inode_info),
					     0, (SLAB_RECLAIM_ACCOUNT|
						SLAB_MEM_SPREAD),
					     init_once);
	if (fat_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void __exit fat_destroy_inodecache(void)
{
	kmem_cache_destroy(fat_inode_cachep);
}

static int fat_remount(struct super_block *sb, int *flags, char *data)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);

#ifdef CONFIG_SNSC_FS_FAT_KEEP_CONSISTENCY_ON_SSBOOT
	/* To keep filesystem consistency without umount */
	if (*flags & MS_RDONLY) {
		/* invalidate block device related caches at R/O remount */
		__invalidate_device(sb->s_bdev, true);
	} else {
		/* update the number of free clusters at R/W remount */
		sbi->free_clusters = -1;
#ifdef CONFIG_SNSC_FS_UVFAT_RANDOMIZE_THE_FIRST_CLUSTER
		fat_find_free_cluster_at_random(sb);
#else
		fat_count_free_clusters(sb);
#endif
	}
#endif
	*flags |= MS_NODIRATIME | (sbi->options.isvfat ? 0 : MS_NOATIME);
	return 0;
}

static int fat_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	u64 id = huge_encode_dev(sb->s_bdev->bd_dev);

	/* If the count of free cluster is still unknown, counts it here. */
	if (sbi->free_clusters == -1 || !sbi->free_clus_valid) {
		int err = fat_count_free_clusters(dentry->d_sb);
		if (err)
			return err;
	}

	buf->f_type = dentry->d_sb->s_magic;
	buf->f_bsize = sbi->cluster_size;
	buf->f_blocks = sbi->max_cluster - FAT_START_ENT;
	buf->f_bfree = sbi->free_clusters;
	buf->f_bavail = sbi->free_clusters;
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen =
		(sbi->options.isvfat ? FAT_LFN_LEN : 12) * NLS_MAX_CHARSET_SIZE;

	return 0;
}

static inline loff_t fat_i_pos_read(struct msdos_sb_info *sbi,
				    struct inode *inode)
{
	loff_t i_pos;
#if BITS_PER_LONG == 32
	spin_lock(&sbi->inode_hash_lock);
#endif
	i_pos = MSDOS_I(inode)->i_pos;
#if BITS_PER_LONG == 32
	spin_unlock(&sbi->inode_hash_lock);
#endif
	return i_pos;
}

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC

#define FAT_START(sb)	(MSDOS_SB(sb)->fat_start << (sb)->s_blocksize_bits)
#define DIR_START(sb)	(MSDOS_SB(sb)->dir_start << (sb)->s_blocksize_bits)
#define FAT_END(sb)	(DIR_START(sb) - 1)
#define DIR_END(sb)	LLONG_MAX

static int fat_sync_meta(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;
	int err = 0;

	/* the FAT BHS update have to be ATOMIC*/
	err = filemap_write_and_wait_range(mapping, FAT_START(sb), FAT_END(sb));
	if (err)
		goto error;
#ifdef CONFIG_SNSC_FS_FAT_FLUSH_HARDWARE_CACHE
	blkdev_issue_flush(sb->s_bdev, GFP_KERNEL, NULL);
#endif

	/* wait BODY DATA to be finished*/
	err = filemap_write_and_wait(inode->i_mapping);
	if (err)
		goto error;

	if (!MSDOS_I(inode)->i_new
	    && MSDOS_I(inode)->i_last_dclus) {
		struct fat_entry fatent;
		int last = MSDOS_I(inode)->i_last_dclus;
		int new = MSDOS_I(inode)->i_new_dclus;

		pr_debug("%s - add chain now: %d %d\n",
			 __func__, last, new);

		fatent_init(&fatent);
		err = fat_ent_read(inode, &fatent, last);
		if (err < 0)
			goto out;

		err = fat_ent_write(inode, &fatent, new, 1);
		if (err < 0)
			goto out;

	out:
		fatent_brelse(&fatent);
		if (err < 0)
			goto error;
#ifdef CONFIG_SNSC_FS_FAT_FLUSH_HARDWARE_CACHE
		blkdev_issue_flush(sb->s_bdev, GFP_KERNEL, NULL);
#endif
	}

	MSDOS_I(inode)->i_new = 0;
	MSDOS_I(inode)->i_last_dclus = 0;

error:
	return err;
}
#endif

static int __fat_write_inode(struct inode *inode, int wait)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct buffer_head *bh;
	struct msdos_dir_entry *raw_entry;
	loff_t i_pos;
	int err;
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;

	if (sbi->options.batch_sync) {
		err = fat_sync_meta(inode);
		if (err) {
			printk(KERN_ERR "fat_sync_meta error: %d!\n", err);
			return err;
		}
	}
#endif
	if (inode->i_ino == MSDOS_ROOT_INO)
		return 0;

retry:
	i_pos = fat_i_pos_read(sbi, inode);
	if (!i_pos)
		return 0;

	bh = sb_bread(sb, i_pos >> sbi->dir_per_block_bits);
	if (!bh) {
		fat_msg(sb, KERN_ERR, "unable to read inode block "
		       "for updating (i_pos %lld)", i_pos);
		return -EIO;
	}
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (sbi->options.batch_sync)
		lock_buffer(bh);
#endif
	spin_lock(&sbi->inode_hash_lock);
	if (i_pos != MSDOS_I(inode)->i_pos) {
		spin_unlock(&sbi->inode_hash_lock);
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
		if (sbi->options.batch_sync)
			unlock_buffer(bh);
#endif
		brelse(bh);
		goto retry;
	}

	raw_entry = &((struct msdos_dir_entry *) (bh->b_data))
	    [i_pos & (sbi->dir_per_block - 1)];
	if (S_ISDIR(inode->i_mode))
		raw_entry->size = 0;
	else
		raw_entry->size = cpu_to_le32(inode->i_size);
	raw_entry->attr = fat_make_attrs(inode);
	raw_entry->start = cpu_to_le16(MSDOS_I(inode)->i_logstart);
	raw_entry->starthi = cpu_to_le16(MSDOS_I(inode)->i_logstart >> 16);
	fat_time_unix2fat(sbi, &inode->i_mtime, &raw_entry->time,
			  &raw_entry->date, NULL);
	if (sbi->options.isvfat) {
		__le16 atime;
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
		if (!sbi->options.ignore_crtime) {
#endif
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
		fat_time_unix2fat(sbi, &inode->i_ctime, &raw_entry->ctime,
				  &raw_entry->cdate, NULL);
 		if (sbi->posix_ops.set_attr(raw_entry, inode) == -1) {
 			raw_entry->ctime_cs
 				= (inode->i_ctime.tv_sec & 1) * 100 +
 					inode->i_ctime.tv_nsec / 10000000;
 		}
#else
		fat_time_unix2fat(sbi, &inode->i_ctime, &raw_entry->ctime,
				  &raw_entry->cdate, &raw_entry->ctime_cs);
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
		}
#endif
		fat_time_unix2fat(sbi, &inode->i_atime, &atime,
				  &raw_entry->adate, NULL);
	}
	spin_unlock(&sbi->inode_hash_lock);
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (sbi->options.batch_sync)
		unlock_buffer(bh);
#endif
	mark_buffer_dirty(bh);
	err = 0;
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	/* sync dirty direntries */
	if (sbi->options.batch_sync)
		err = filemap_write_and_wait_range(mapping, DIR_START(sb),
						   DIR_END(sb));
	else
#endif
	if (wait)
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
		err = sbi->options.relax_sync ?
			flush_dirty_buffer(bh) : sync_dirty_buffer(bh);
#else
		err = sync_dirty_buffer(bh);
#endif
	brelse(bh);
	return err;
}

static int fat_write_inode(struct inode *inode, struct writeback_control *wbc)
{
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	int ret = 0;

	if (sbi->options.batch_sync) {
		if (!mutex_trylock(&inode->i_mutex)) {
			/*
			 * if we failed to acquire the lock now
			 * then mark it dirty again.
			 * it never fail during umount, so it will
			 * be committed finally.
			 */
			pr_debug("Fail to acquire lock, mark it dirty again!\n");
			mark_inode_dirty(inode);
			goto out;
		}

		pr_debug("Acquire lock sucessfully!\n");

		ret = __fat_write_inode(inode, 1);

		mutex_unlock(&inode->i_mutex);
	} else {
		ret = __fat_write_inode(inode, wbc->sync_mode == WB_SYNC_ALL);
	}

out:
	return ret;
#else
	return __fat_write_inode(inode, wbc->sync_mode == WB_SYNC_ALL);
#endif
}

int fat_sync_inode(struct inode *inode)
{
	return __fat_write_inode(inode, 1);
}

EXPORT_SYMBOL_GPL(fat_sync_inode);

static int fat_show_options(struct seq_file *m, struct vfsmount *mnt);
static const struct super_operations fat_sops = {
	.alloc_inode	= fat_alloc_inode,
	.destroy_inode	= fat_destroy_inode,
	.write_inode	= fat_write_inode,
	.evict_inode	= fat_evict_inode,
	.put_super	= fat_put_super,
	.write_super	= fat_write_super,
	.sync_fs	= fat_sync_fs,
	.statfs		= fat_statfs,
	.remount_fs	= fat_remount,

	.show_options	= fat_show_options,
};

/*
 * a FAT file handle with fhtype 3 is
 *  0/  i_ino - for fast, reliable lookup if still in the cache
 *  1/  i_generation - to see if i_ino is still valid
 *          bit 0 == 0 iff directory
 *  2/  i_pos(8-39) - if ino has changed, but still in cache
 *  3/  i_pos(4-7)|i_logstart - to semi-verify inode found at i_pos
 *  4/  i_pos(0-3)|parent->i_logstart - maybe used to hunt for the file on disc
 *
 * Hack for NFSv2: Maximum FAT entry number is 28bits and maximum
 * i_pos is 40bits (blocknr(32) + dir offset(8)), so two 4bits
 * of i_logstart is used to store the directory entry offset.
 */

static struct dentry *fat_fh_to_dentry(struct super_block *sb,
		struct fid *fid, int fh_len, int fh_type)
{
	struct inode *inode = NULL;
	u32 *fh = fid->raw;

	if (fh_len < 5 || fh_type != 3)
		return NULL;

	inode = ilookup(sb, fh[0]);
	if (!inode || inode->i_generation != fh[1]) {
		if (inode)
			iput(inode);
		inode = NULL;
	}
	if (!inode) {
		loff_t i_pos;
		int i_logstart = fh[3] & 0x0fffffff;

		i_pos = (loff_t)fh[2] << 8;
		i_pos |= ((fh[3] >> 24) & 0xf0) | (fh[4] >> 28);

		/* try 2 - see if i_pos is in F-d-c
		 * require i_logstart to be the same
		 * Will fail if you truncate and then re-write
		 */

		inode = fat_iget(sb, i_pos);
		if (inode && MSDOS_I(inode)->i_logstart != i_logstart) {
			iput(inode);
			inode = NULL;
		}
	}

	/*
	 * For now, do nothing if the inode is not found.
	 *
	 * What we could do is:
	 *
	 *	- follow the file starting at fh[4], and record the ".." entry,
	 *	  and the name of the fh[2] entry.
	 *	- then follow the ".." file finding the next step up.
	 *
	 * This way we build a path to the root of the tree. If this works, we
	 * lookup the path and so get this inode into the cache.  Finally try
	 * the fat_iget lookup again.  If that fails, then we are totally out
	 * of luck.  But all that is for another day
	 */
	return d_obtain_alias(inode);
}

static int
fat_encode_fh(struct dentry *de, __u32 *fh, int *lenp, int connectable)
{
	int len = *lenp;
	struct inode *inode =  de->d_inode;
	u32 ipos_h, ipos_m, ipos_l;

	if (len < 5) {
		*lenp = 5;
		return 255; /* no room */
	}

	ipos_h = MSDOS_I(inode)->i_pos >> 8;
	ipos_m = (MSDOS_I(inode)->i_pos & 0xf0) << 24;
	ipos_l = (MSDOS_I(inode)->i_pos & 0x0f) << 28;
	*lenp = 5;
	fh[0] = inode->i_ino;
	fh[1] = inode->i_generation;
	fh[2] = ipos_h;
	fh[3] = ipos_m | MSDOS_I(inode)->i_logstart;
	seq_spin_lock(&de->d_lock);
	fh[4] = ipos_l | MSDOS_I(de->d_parent->d_inode)->i_logstart;
	seq_spin_unlock(&de->d_lock);
	return 3;
}

static struct dentry *fat_get_parent(struct dentry *child)
{
	struct super_block *sb = child->d_sb;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	loff_t i_pos;
	struct dentry *parent;
	struct inode *inode;
	int err;

	lock_super(sb);

	err = fat_get_dotdot_entry(child->d_inode, &bh, &de, &i_pos);
	if (err) {
		parent = ERR_PTR(err);
		goto out;
	}
	inode = fat_build_inode(sb, de, i_pos);
	brelse(bh);

	parent = d_obtain_alias(inode);
out:
	unlock_super(sb);

	return parent;
}

static const struct export_operations fat_export_ops = {
	.encode_fh	= fat_encode_fh,
	.fh_to_dentry	= fat_fh_to_dentry,
	.get_parent	= fat_get_parent,
};

static int fat_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	struct msdos_sb_info *sbi = MSDOS_SB(mnt->mnt_sb);
	struct fat_mount_options *opts = &sbi->options;
	int isvfat = opts->isvfat;

	if (opts->fs_uid != 0)
		seq_printf(m, ",uid=%u", opts->fs_uid);
	if (opts->fs_gid != 0)
		seq_printf(m, ",gid=%u", opts->fs_gid);
	seq_printf(m, ",fmask=%04o", opts->fs_fmask);
	seq_printf(m, ",dmask=%04o", opts->fs_dmask);
	if (opts->allow_utime)
		seq_printf(m, ",allow_utime=%04o", opts->allow_utime);
	if (sbi->nls_disk)
		seq_printf(m, ",codepage=%s", sbi->nls_disk->charset);
	if (isvfat) {
		if (sbi->nls_io)
			seq_printf(m, ",iocharset=%s", sbi->nls_io->charset);

		switch (opts->shortname) {
		case VFAT_SFN_DISPLAY_WIN95 | VFAT_SFN_CREATE_WIN95:
			seq_puts(m, ",shortname=win95");
			break;
		case VFAT_SFN_DISPLAY_WINNT | VFAT_SFN_CREATE_WINNT:
			seq_puts(m, ",shortname=winnt");
			break;
		case VFAT_SFN_DISPLAY_WINNT | VFAT_SFN_CREATE_WIN95:
			seq_puts(m, ",shortname=mixed");
			break;
		case VFAT_SFN_DISPLAY_LOWER | VFAT_SFN_CREATE_WIN95:
			seq_puts(m, ",shortname=lower");
			break;
		default:
			seq_puts(m, ",shortname=unknown");
			break;
		}
	}
	if (opts->name_check != 'n')
		seq_printf(m, ",check=%c", opts->name_check);
	if (opts->usefree)
		seq_puts(m, ",usefree");
	if (opts->quiet)
		seq_puts(m, ",quiet");
	if (opts->showexec)
		seq_puts(m, ",showexec");
	if (opts->sys_immutable)
		seq_puts(m, ",sys_immutable");
#ifdef CONFIG_SNSC_FS_FAT_GC
	if (opts->gc)
		seq_puts(m, ",gc");
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (opts->batch_sync)
		seq_puts(m, ",batch_sync");
#endif
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
	if (opts->relax_sync)
		seq_puts(m, ",relax_sync");
#endif
#ifdef CONFIG_SNSC_FS_FAT12_NO_SECTOR_BOUNDARY
	if (opts->no_sect_bndry)
		seq_puts(m, ",no_sect_bndry");
#endif
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	if (opts->strict)
		seq_puts(m, ",strict");
#endif
	if (!isvfat) {
		if (opts->dotsOK)
			seq_puts(m, ",dotsOK=yes");
		if (opts->nocase)
			seq_puts(m, ",nocase");
	} else {
		if (opts->utf8)
			seq_puts(m, ",utf8");
		if (opts->unicode_xlate)
			seq_puts(m, ",uni_xlate");
		if (!opts->numtail)
			seq_puts(m, ",nonumtail");
		if (opts->rodir)
			seq_puts(m, ",rodir");
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
		if (opts->posix_attr)
			seq_puts(m, ",posix_attr");
#endif
#ifdef CONFIG_SNSC_FS_VFAT_COMPARE_UNICODE
		if (opts->compare_unicode)
			seq_puts(m, ",comp_uni");
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
		if (opts->clnshutbit)
			seq_puts(m, ",clnshutbit");
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
		if (opts->ignore_crtime)
			seq_puts(m, ",ignore_crtime");
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
		if (opts->check_disk)
			seq_puts(m, ",check_disk");
#endif
#ifdef CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK
		if (opts->avoid_dlink)
			seq_puts(m, ",avoid_dlink");
#endif
	}
	if (opts->flush)
		seq_puts(m, ",flush");
	if (opts->tz_utc)
		seq_puts(m, ",tz=UTC");
	if (opts->errors == FAT_ERRORS_CONT)
		seq_puts(m, ",errors=continue");
	else if (opts->errors == FAT_ERRORS_PANIC)
		seq_puts(m, ",errors=panic");
	else
		seq_puts(m, ",errors=remount-ro");
	if (opts->discard)
		seq_puts(m, ",discard");

	return 0;
}

enum {
#ifdef CONFIG_SNSC_FS_FAT_GC
	Opt_gc,
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	Opt_batch_dirsync, Opt_batch_sync,
#endif
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
	Opt_relax_sync,
#endif
	Opt_check_n, Opt_check_r, Opt_check_s, Opt_uid, Opt_gid,
	Opt_umask, Opt_dmask, Opt_fmask, Opt_allow_utime, Opt_codepage,
	Opt_usefree, Opt_nocase, Opt_quiet, Opt_showexec, Opt_debug,
	Opt_immutable, Opt_dots, Opt_nodots,
#ifdef CONFIG_SNSC_FS_FAT12_NO_SECTOR_BOUNDARY
	Opt_no_sect_bndry,
#endif
	Opt_charset, Opt_shortname_lower, Opt_shortname_win95,
	Opt_shortname_winnt, Opt_shortname_mixed, Opt_utf8_no, Opt_utf8_yes,
	Opt_uni_xl_no, Opt_uni_xl_yes, Opt_nonumtail_no, Opt_nonumtail_yes,
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	Opt_posix_attr_no, Opt_posix_attr_yes,
#endif
#ifdef CONFIG_SNSC_FS_VFAT_COMPARE_UNICODE
	Opt_comp_uni_no, Opt_comp_uni_yes,
#endif
#ifdef CONFIG_SNSC_FS_FAT_TIMEZONE
	Opt_timezone,
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
	Opt_clnshutbit_no, Opt_clnshutbit_yes,
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
	Opt_ignore_crtime_no, Opt_ignore_crtime_yes,
#endif
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	Opt_strict,
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	Opt_check_disk_no, Opt_check_disk_yes,
#endif
#ifdef CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK
	Opt_avoid_dlink_no, Opt_avoid_dlink_yes,
#endif
	Opt_obsolate, Opt_flush, Opt_tz_utc, Opt_rodir, Opt_err_cont,
	Opt_err_panic, Opt_err_ro, Opt_discard, Opt_err,
};

static const match_table_t fat_tokens = {
#ifdef CONFIG_SNSC_FS_FAT_GC
	{Opt_gc, "gc"},
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	{Opt_batch_dirsync, "batch_dirsync"},
	{Opt_batch_sync, "batch_sync"},
#endif
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
	{Opt_relax_sync, "relax_sync"},
#endif
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	{Opt_strict, "strict"},
#endif
	{Opt_check_r, "check=relaxed"},
	{Opt_check_s, "check=strict"},
	{Opt_check_n, "check=normal"},
	{Opt_check_r, "check=r"},
	{Opt_check_s, "check=s"},
	{Opt_check_n, "check=n"},
	{Opt_uid, "uid=%u"},
	{Opt_gid, "gid=%u"},
	{Opt_umask, "umask=%o"},
	{Opt_dmask, "dmask=%o"},
	{Opt_fmask, "fmask=%o"},
	{Opt_allow_utime, "allow_utime=%o"},
	{Opt_codepage, "codepage=%u"},
	{Opt_usefree, "usefree"},
	{Opt_nocase, "nocase"},
	{Opt_quiet, "quiet"},
	{Opt_showexec, "showexec"},
	{Opt_debug, "debug"},
	{Opt_immutable, "sys_immutable"},
#ifdef CONFIG_SNSC_FS_FAT12_NO_SECTOR_BOUNDARY
	{Opt_no_sect_bndry, "no_sect_bndry"},
#endif
	{Opt_flush, "flush"},
	{Opt_tz_utc, "tz=UTC"},
	{Opt_err_cont, "errors=continue"},
	{Opt_err_panic, "errors=panic"},
	{Opt_err_ro, "errors=remount-ro"},
	{Opt_discard, "discard"},
	{Opt_obsolate, "conv=binary"},
	{Opt_obsolate, "conv=text"},
	{Opt_obsolate, "conv=auto"},
	{Opt_obsolate, "conv=b"},
	{Opt_obsolate, "conv=t"},
	{Opt_obsolate, "conv=a"},
	{Opt_obsolate, "fat=%u"},
	{Opt_obsolate, "blocksize=%u"},
	{Opt_obsolate, "cvf_format=%20s"},
	{Opt_obsolate, "cvf_options=%100s"},
	{Opt_obsolate, "posix"},
	{Opt_err, NULL},
};
static const match_table_t msdos_tokens = {
	{Opt_nodots, "nodots"},
	{Opt_nodots, "dotsOK=no"},
	{Opt_dots, "dots"},
	{Opt_dots, "dotsOK=yes"},
	{Opt_err, NULL}
};
static const match_table_t vfat_tokens = {
	{Opt_charset, "iocharset=%s"},
	{Opt_shortname_lower, "shortname=lower"},
	{Opt_shortname_win95, "shortname=win95"},
	{Opt_shortname_winnt, "shortname=winnt"},
	{Opt_shortname_mixed, "shortname=mixed"},
	{Opt_utf8_no, "utf8=0"},		/* 0 or no or false */
	{Opt_utf8_no, "utf8=no"},
	{Opt_utf8_no, "utf8=false"},
	{Opt_utf8_yes, "utf8=1"},		/* empty or 1 or yes or true */
	{Opt_utf8_yes, "utf8=yes"},
	{Opt_utf8_yes, "utf8=true"},
	{Opt_utf8_yes, "utf8"},
	{Opt_uni_xl_no, "uni_xlate=0"},		/* 0 or no or false */
	{Opt_uni_xl_no, "uni_xlate=no"},
	{Opt_uni_xl_no, "uni_xlate=false"},
	{Opt_uni_xl_yes, "uni_xlate=1"},	/* empty or 1 or yes or true */
	{Opt_uni_xl_yes, "uni_xlate=yes"},
	{Opt_uni_xl_yes, "uni_xlate=true"},
	{Opt_uni_xl_yes, "uni_xlate"},
	{Opt_nonumtail_no, "nonumtail=0"},	/* 0 or no or false */
	{Opt_nonumtail_no, "nonumtail=no"},
	{Opt_nonumtail_no, "nonumtail=false"},
	{Opt_nonumtail_yes, "nonumtail=1"},	/* empty or 1 or yes or true */
	{Opt_nonumtail_yes, "nonumtail=yes"},
	{Opt_nonumtail_yes, "nonumtail=true"},
	{Opt_nonumtail_yes, "nonumtail"},
	{Opt_rodir, "rodir"},
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	{Opt_posix_attr_no, "posix_attr=0"},	/* 0 or no or false */
	{Opt_posix_attr_no, "posix_attr=no"},
	{Opt_posix_attr_no, "posix_attr=false"},
	{Opt_posix_attr_yes, "posix_attr=1"},	/* empty or 1 or yes or true */
	{Opt_posix_attr_yes, "posix_attr=yes"},
	{Opt_posix_attr_yes, "posix_attr=true"},
	{Opt_posix_attr_yes, "posix_attr"},
#endif
#ifdef CONFIG_SNSC_FS_VFAT_COMPARE_UNICODE
	{Opt_comp_uni_no, "comp_uni=0"},	/* 0 or no or false */
	{Opt_comp_uni_no, "comp_uni=no"},
	{Opt_comp_uni_no, "comp_uni=false"},
	{Opt_comp_uni_yes, "comp_uni=1"},	/* empty or 1 or yes or true */
	{Opt_comp_uni_yes, "comp_uni=yes"},
	{Opt_comp_uni_yes, "comp_uni=true"},
	{Opt_comp_uni_yes, "comp_uni"},
#endif
#ifdef CONFIG_SNSC_FS_FAT_TIMEZONE
	{Opt_timezone, "timezone=%s"},
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
	{Opt_clnshutbit_no, "clnshutbit=0"},	/* 0 or no or false */
	{Opt_clnshutbit_no, "clnshutbit=no"},
	{Opt_clnshutbit_no, "clnshutbit=false"},
	{Opt_clnshutbit_yes, "clnshutbit=1"},	/* empty or 1 or yes or true */
	{Opt_clnshutbit_yes, "clnshutbit=yes"},
	{Opt_clnshutbit_yes, "clnshutbit=true"},
	{Opt_clnshutbit_yes, "clnshutbit"},
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
	{Opt_ignore_crtime_no, "ignore_crtime=0"},	/* 0 or no or false */
	{Opt_ignore_crtime_no, "ignore_crtime=no"},
	{Opt_ignore_crtime_no, "ignore_crtime=false"},
	{Opt_ignore_crtime_yes, "ignore_crtime=1"},	/* empty or 1 or yes or true */
	{Opt_ignore_crtime_yes, "ignore_crtime=yes"},
	{Opt_ignore_crtime_yes, "ignore_crtime=true"},
	{Opt_ignore_crtime_yes, "ignore_crtime"},
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	{Opt_check_disk_no, "check_disk=0"},	/* 0 or no or false */
	{Opt_check_disk_no, "check_disk=no"},
	{Opt_check_disk_no, "check_disk=false"},
	{Opt_check_disk_yes, "check_disk=1"},	/* empty or 1 or yes or true */
	{Opt_check_disk_yes, "check_disk=yes"},
	{Opt_check_disk_yes, "check_disk=true"},
	{Opt_check_disk_yes, "check_disk"},
#endif
#ifdef CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK
	{Opt_avoid_dlink_no, "avoid_dlink=0"},	/* 0 or no or false */
	{Opt_avoid_dlink_no, "avoid_dlink=no"},
	{Opt_avoid_dlink_no, "avoid_dlink=false"},
	{Opt_avoid_dlink_yes, "avoid_dlink=1"},	/* empty or 1 or yes or true */
	{Opt_avoid_dlink_yes, "avoid_dlink=yes"},
	{Opt_avoid_dlink_yes, "avoid_dlink=true"},
	{Opt_avoid_dlink_yes, "avoid_dlink"},
#endif
	{Opt_err, NULL}
};

static int parse_options(struct super_block *sb, char *options, int is_vfat,
			 int silent, int *debug, struct fat_mount_options *opts)
{
	char *p;
	substring_t args[MAX_OPT_ARGS];
	int option;
	char *iocharset;
#ifdef CONFIG_SNSC_FS_FAT_TIMEZONE
	char *tz;
#endif

	opts->isvfat = is_vfat;

	opts->fs_uid = current_uid();
	opts->fs_gid = current_gid();
	opts->fs_fmask = opts->fs_dmask = current_umask();
	opts->allow_utime = -1;
	opts->codepage = fat_default_codepage;
	opts->iocharset = fat_default_iocharset;
	if (is_vfat) {
		opts->shortname = VFAT_SFN_DISPLAY_WINNT|VFAT_SFN_CREATE_WIN95;
		opts->rodir = 0;
	} else {
		opts->shortname = 0;
		opts->rodir = 1;
	}
#ifdef CONFIG_SNSC_FS_FAT_GC
	opts->gc = 0;
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	opts->batch_sync = 0;
#endif
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
	opts->relax_sync = 0;
#endif
	opts->name_check = 'n';
	opts->quiet = opts->showexec = opts->sys_immutable = opts->dotsOK =  0;
	opts->utf8 = opts->unicode_xlate = 0;
	opts->numtail = 1;
	opts->usefree = opts->nocase = 0;
	opts->tz_utc = 0;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
#ifdef CONFIG_SNSC_FS_ROOT_VFAT
	opts->posix_attr = 1;
	opts->quiet = 1;
#else
	opts->posix_attr = 0;
#endif
#endif
	opts->errors = FAT_ERRORS_RO;
#ifdef CONFIG_SNSC_FS_FAT12_NO
	opts->no_sect_bndry = 0;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_COMPARE_UNICODE
	opts->compare_unicode = 1;
#endif
#ifdef CONFIG_SNSC_FS_FAT_TIMEZONE
	fat_tz_set(&opts->timezone, NULL);
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
	opts->clnshutbit = 0;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
	opts->ignore_crtime = 0;
#endif
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	opts->strict = 0;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
	opts->check_disk = 0;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK
	opts->avoid_dlink = CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK_DEFAULT;
	if (opts->avoid_dlink != 0)
		opts->avoid_dlink = 1;
#endif
	*debug = 0;

	if (!options)
		goto out;

	while ((p = strsep(&options, ",")) != NULL) {
		int token;
		if (!*p)
			continue;

		token = match_token(p, fat_tokens, args);
		if (token == Opt_err) {
			if (is_vfat)
				token = match_token(p, vfat_tokens, args);
			else
				token = match_token(p, msdos_tokens, args);
		}
		switch (token) {
#ifdef CONFIG_SNSC_FS_FAT_GC
		case Opt_gc:
			opts->gc = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
		case Opt_batch_dirsync:
			opts->dirsync = 1;
		case Opt_batch_sync:
			opts->batch_sync = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
		case Opt_relax_sync:
			opts->relax_sync = 1;
			break;
#endif
		case Opt_check_s:
			opts->name_check = 's';
			break;
		case Opt_check_r:
			opts->name_check = 'r';
			break;
		case Opt_check_n:
			opts->name_check = 'n';
			break;
		case Opt_usefree:
			opts->usefree = 1;
			break;
		case Opt_nocase:
			if (!is_vfat)
				opts->nocase = 1;
			else {
				/* for backward compatibility */
				opts->shortname = VFAT_SFN_DISPLAY_WIN95
					| VFAT_SFN_CREATE_WIN95;
			}
			break;
		case Opt_quiet:
			opts->quiet = 1;
			break;
		case Opt_showexec:
			opts->showexec = 1;
			break;
		case Opt_debug:
			*debug = 1;
			break;
		case Opt_immutable:
			opts->sys_immutable = 1;
			break;
		case Opt_uid:
			if (match_int(&args[0], &option))
				return 0;
			opts->fs_uid = option;
			break;
		case Opt_gid:
			if (match_int(&args[0], &option))
				return 0;
			opts->fs_gid = option;
			break;
		case Opt_umask:
			if (match_octal(&args[0], &option))
				return 0;
			opts->fs_fmask = opts->fs_dmask = option;
			break;
		case Opt_dmask:
			if (match_octal(&args[0], &option))
				return 0;
			opts->fs_dmask = option;
			break;
		case Opt_fmask:
			if (match_octal(&args[0], &option))
				return 0;
			opts->fs_fmask = option;
			break;
		case Opt_allow_utime:
			if (match_octal(&args[0], &option))
				return 0;
			opts->allow_utime = option & (S_IWGRP | S_IWOTH);
			break;
		case Opt_codepage:
			if (match_int(&args[0], &option))
				return 0;
			opts->codepage = option;
			break;
		case Opt_flush:
			opts->flush = 1;
			break;
		case Opt_tz_utc:
			opts->tz_utc = 1;
			break;
		case Opt_err_cont:
			opts->errors = FAT_ERRORS_CONT;
			break;
		case Opt_err_panic:
			opts->errors = FAT_ERRORS_PANIC;
			break;
		case Opt_err_ro:
			opts->errors = FAT_ERRORS_RO;
			break;
#ifdef CONFIG_SNSC_FS_FAT12_NO_SECTOR_BOUNDARY
		case Opt_no_sect_bndry:
			opts->no_sect_bndry = 1;
			break;
#endif

		/* msdos specific */
		case Opt_dots:
			opts->dotsOK = 1;
			break;
		case Opt_nodots:
			opts->dotsOK = 0;
			break;
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
		case Opt_posix_attr_no:		/* 0 or no or false */
			opts->posix_attr = 0;
			break;
		case Opt_posix_attr_yes:	/* empty or 1 or yes or true */
			opts->posix_attr = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_COMPARE_UNICODE
		case Opt_comp_uni_no:		/* 0 or no or false */
			opts->compare_unicode = 0;
			break;
		case Opt_comp_uni_yes:		/* empty or 1 or yes or true */
			opts->compare_unicode = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_FAT_TIMEZONE
		case Opt_timezone:
			tz = match_strdup(&args[0]);
			if (!tz)
				return -ENOMEM;
			if (fat_tz_set(&opts->timezone, tz) < 0) {
				printk("VFAT: failed to parse mount option timezone=%s\n", tz);
				printk("VFAT: using default timezone\n");
				fat_tz_set(&opts->timezone, NULL);
			}
			kfree(tz);
			break;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
		case Opt_clnshutbit_no:		/* 0 or no or false */
			opts->clnshutbit = 0;
			break;
		case Opt_clnshutbit_yes:	/* empty or 1 or yes or true */
			opts->clnshutbit = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_IGNORE_CRTIME
		case Opt_ignore_crtime_no:	/* 0 or no or false */
			opts->ignore_crtime = 0;
			break;
		case Opt_ignore_crtime_yes:	/* empty or 1 or yes or true */
			opts->ignore_crtime = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
		case Opt_strict:
			opts->strict = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_CHECK_DISK
		case Opt_check_disk_no:		/* 0 or no or false */
			opts->check_disk = 0;
			break;
		case Opt_check_disk_yes:	/* empty or 1 or yes or true */
			opts->check_disk = 1;
			break;
#endif
#ifdef CONFIG_SNSC_FS_VFAT_AVOID_DOUBLE_LINK
		case Opt_avoid_dlink_no:	/* 0 or no or false */
			opts->avoid_dlink = 0;
			break;
		case Opt_avoid_dlink_yes:	/* empty or 1 or yes or true */
			opts->avoid_dlink = 1;
			break;
#endif

		/* vfat specific */
		case Opt_charset:
			if (opts->iocharset != fat_default_iocharset)
				kfree(opts->iocharset);
			iocharset = match_strdup(&args[0]);
			if (!iocharset)
				return -ENOMEM;
			opts->iocharset = iocharset;
			break;
		case Opt_shortname_lower:
			opts->shortname = VFAT_SFN_DISPLAY_LOWER
					| VFAT_SFN_CREATE_WIN95;
			break;
		case Opt_shortname_win95:
			opts->shortname = VFAT_SFN_DISPLAY_WIN95
					| VFAT_SFN_CREATE_WIN95;
			break;
		case Opt_shortname_winnt:
			opts->shortname = VFAT_SFN_DISPLAY_WINNT
					| VFAT_SFN_CREATE_WINNT;
			break;
		case Opt_shortname_mixed:
			opts->shortname = VFAT_SFN_DISPLAY_WINNT
					| VFAT_SFN_CREATE_WIN95;
			break;
		case Opt_utf8_no:		/* 0 or no or false */
			opts->utf8 = 0;
			break;
		case Opt_utf8_yes:		/* empty or 1 or yes or true */
			opts->utf8 = 1;
			break;
		case Opt_uni_xl_no:		/* 0 or no or false */
			opts->unicode_xlate = 0;
			break;
		case Opt_uni_xl_yes:		/* empty or 1 or yes or true */
			opts->unicode_xlate = 1;
			break;
		case Opt_nonumtail_no:		/* 0 or no or false */
			opts->numtail = 1;	/* negated option */
			break;
		case Opt_nonumtail_yes:		/* empty or 1 or yes or true */
			opts->numtail = 0;	/* negated option */
			break;
		case Opt_rodir:
			opts->rodir = 1;
			break;
		case Opt_discard:
			opts->discard = 1;
			break;

		/* obsolete mount options */
		case Opt_obsolate:
			fat_msg(sb, KERN_INFO, "\"%s\" option is obsolete, "
			       "not supported now", p);
			break;
		/* unknown option */
		default:
			if (!silent) {
				fat_msg(sb, KERN_ERR,
				       "Unrecognized mount option \"%s\" "
				       "or missing value", p);
			}
			return -EINVAL;
		}
	}

out:
	/* UTF-8 doesn't provide FAT semantics */
	if (!strcmp(opts->iocharset, "utf8")) {
		fat_msg(sb, KERN_ERR, "utf8 is not a recommended IO charset"
		       " for FAT filesystems, filesystem will be "
		       "case sensitive!\n");
	}

	/* If user doesn't specify allow_utime, it's initialized from dmask. */
	if (opts->allow_utime == (unsigned short)-1)
		opts->allow_utime = ~opts->fs_dmask & (S_IWGRP | S_IWOTH);
	if (opts->unicode_xlate)
		opts->utf8 = 0;

	return 0;
}

static int fat_read_root(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	int error;

	MSDOS_I(inode)->i_pos = 0;
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	MSDOS_I(inode)->i_last_dclus = 0;
	MSDOS_I(inode)->i_new = 0;
	MSDOS_I(inode)->i_new_dclus = 0;
#endif
	inode->i_uid = sbi->options.fs_uid;
	inode->i_gid = sbi->options.fs_gid;
	inode->i_version++;
	inode->i_generation = 0;
	inode->i_mode = fat_make_mode(sbi, ATTR_DIR, S_IRWXUGO);
	inode->i_op = sbi->dir_ops;
	inode->i_fop = &fat_dir_operations;
	if (sbi->fat_bits == 32) {
		MSDOS_I(inode)->i_start = sbi->root_cluster;
		error = fat_calc_dir_size(inode);
		if (error < 0)
			return error;
	} else {
		MSDOS_I(inode)->i_start = 0;
		inode->i_size = sbi->dir_entries * sizeof(struct msdos_dir_entry);
	}
	inode->i_blocks = ((inode->i_size + (sbi->cluster_size - 1))
			   & ~((loff_t)sbi->cluster_size - 1)) >> 9;
	MSDOS_I(inode)->i_logstart = 0;
	MSDOS_I(inode)->mmu_private = inode->i_size;

	fat_save_attrs(inode, ATTR_DIR);
	inode->i_mtime.tv_sec = inode->i_atime.tv_sec = inode->i_ctime.tv_sec = 0;
	inode->i_mtime.tv_nsec = inode->i_atime.tv_nsec = inode->i_ctime.tv_nsec = 0;
	inode->i_nlink = fat_subdirs(inode)+2;

	return 0;
}

#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
static int noop_is_symlink(struct inode *inode, struct msdos_dir_entry *dentry)
{
	return 0;
}

static int noop_set_attr(struct msdos_dir_entry *dentry, struct inode *inode)
{
	return -1;
}

static int noop_get_attr(struct inode *inode, struct msdos_dir_entry *dentry)
{
	return -1;
}

static struct fat_posix_ops posix_noops = {
        .is_symlink = noop_is_symlink,
        .set_attr = noop_set_attr,
	.get_attr = noop_get_attr,
};
#endif

#ifdef CONFIG_SNSC_FS_FAT_LOOKUP_HINT
extern void fat_lkup_hint_init_sb(struct super_block *sb);
#endif

/*
 * Read the super block of an MS-DOS FS.
 */
int fat_fill_super(struct super_block *sb, void *data, int silent, int isvfat,
		   void (*setup)(struct super_block *))
{
	struct inode *root_inode = NULL, *fat_inode = NULL;
	struct buffer_head *bh;
	struct fat_boot_sector *b;
	struct msdos_sb_info *sbi;
	u16 logical_sector_size;
	u32 total_sectors, total_clusters, fat_clusters, rootdir_sectors;
	int debug;
	unsigned int media;
	long error;
	char buf[50];

	/*
	 * GFP_KERNEL is ok here, because while we do hold the
	 * supeblock lock, memory pressure can't call back into
	 * the filesystem, since we're only just about to mount
	 * it and have no inodes etc active!
	 */
	sbi = kzalloc(sizeof(struct msdos_sb_info), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	sb->s_fs_info = sbi;

	sb->s_flags |= MS_NODIRATIME;
	sb->s_magic = MSDOS_SUPER_MAGIC;
	sb->s_op = &fat_sops;
	sb->s_export_op = &fat_export_ops;
	ratelimit_state_init(&sbi->ratelimit, DEFAULT_RATELIMIT_INTERVAL,
			     DEFAULT_RATELIMIT_BURST);
#ifdef CONFIG_SNSC_FS_FAT_LOOKUP_HINT
	fat_lkup_hint_init_sb(sb);
#endif

	error = parse_options(sb, data, isvfat, silent, &debug, &sbi->options);
	if (error)
		goto out_fail;
#ifdef CONFIG_SNSC_FS_FAT_RELAX_SYNC
	if (sbi->options.relax_sync)
		sb->s_flags |= MS_SYNCHRONOUS;
#endif
#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
	if (sbi->options.dirsync)
		sb->s_flags |= MS_DIRSYNC;
	if (sbi->options.batch_sync) {
		sbi->options.relax_sync = 0;
		sb->s_flags &= ~MS_SYNCHRONOUS;
	}
#endif

	setup(sb); /* flavour-specific stuff that needs options */

	error = -EIO;
	sb_min_blocksize(sb, 512);
	bh = sb_bread(sb, 0);
	if (bh == NULL) {
		fat_msg(sb, KERN_ERR, "unable to read boot sector");
		goto out_fail;
	}

	b = (struct fat_boot_sector *) bh->b_data;
	if (!b->reserved) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus number of reserved sectors");
		brelse(bh);
		goto out_invalid;
	}
	if (!b->fats) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus number of FAT structure");
		brelse(bh);
		goto out_invalid;
	}

	/*
	 * Earlier we checked here that b->secs_track and b->head are nonzero,
	 * but it turns out valid FAT filesystems can have zero there.
	 */

#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	if (sbi->options.strict) {
		if (b->ignored[0] != 0xe9 && b->ignored[0] != 0xeb) {
			if (!silent)
				printk(KERN_ERR "FAT: invalid boot code"
				       " (0x%02x)\n", b->ignored[0]);
			brelse(bh);
			goto out_invalid;
		}
		if ((unsigned char)bh->b_data[0x1fe] != 0x55 ||
		    (unsigned char)bh->b_data[0x1ff] != 0xaa) {
			if (!silent)
				printk(KERN_ERR "FAT: invalid signature"
				       " (0x%02x%02x)\n", bh->b_data[0x1fe],
				       bh->b_data[0x1ff]);
			brelse(bh);
			goto out_invalid;
		}
	}
#endif
	media = b->media;
	if (!fat_valid_media(media)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "invalid media value (0x%02x)",
			       media);
		brelse(bh);
		goto out_invalid;
	}
	logical_sector_size = get_unaligned_le16(&b->sector_size);
	if (!is_power_of_2(logical_sector_size)
	    || (logical_sector_size < 512)
	    || (logical_sector_size > 4096)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus logical sector size %u",
			       logical_sector_size);
		brelse(bh);
		goto out_invalid;
	}
	sbi->sec_per_clus = b->sec_per_clus;
	if (!is_power_of_2(sbi->sec_per_clus)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus sectors per cluster %u",
			       sbi->sec_per_clus);
		brelse(bh);
		goto out_invalid;
	}

	if (logical_sector_size < sb->s_blocksize) {
		fat_msg(sb, KERN_ERR, "logical sector size too small for device"
		       " (logical sector size = %u)", logical_sector_size);
		brelse(bh);
		goto out_fail;
	}
	if (logical_sector_size > sb->s_blocksize) {
		brelse(bh);

		if (!sb_set_blocksize(sb, logical_sector_size)) {
			fat_msg(sb, KERN_ERR, "unable to set blocksize %u",
			       logical_sector_size);
			goto out_fail;
		}
		bh = sb_bread(sb, 0);
		if (bh == NULL) {
			fat_msg(sb, KERN_ERR, "unable to read boot sector"
			       " (logical sector size = %lu)",
			       sb->s_blocksize);
			goto out_fail;
		}
		b = (struct fat_boot_sector *) bh->b_data;
	}

	sbi->cluster_size = sb->s_blocksize * sbi->sec_per_clus;
	sbi->cluster_bits = ffs(sbi->cluster_size) - 1;
	sbi->fats = b->fats;
	sbi->fat_bits = 0;		/* Don't know yet */
	sbi->fat_start = le16_to_cpu(b->reserved);
	sbi->fat_length = le16_to_cpu(b->fat_length);
	sbi->root_cluster = 0;
	sbi->free_clusters = -1;	/* Don't know yet */
	sbi->free_clus_valid = 0;
	sbi->prev_free = FAT_START_ENT;
#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	sbi->dir_entries =
		le16_to_cpu(get_unaligned((__le16 *)&b->dir_entries));
#endif

#ifdef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	if ((sbi->options.strict && sbi->dir_entries == 0) ||
	    (!sbi->options.strict && !sbi->fat_length && b->fat32_length)) {
#else
	if (!sbi->fat_length && b->fat32_length) {
#endif
		struct fat_boot_fsinfo *fsinfo;
		struct buffer_head *fsinfo_bh;

		/* Must be FAT32 */
		sbi->fat_bits = 32;
		sbi->fat_length = le32_to_cpu(b->fat32_length);
		sbi->root_cluster = le32_to_cpu(b->root_cluster);

		sb->s_maxbytes = 0xffffffff;

		/* MC - if info_sector is 0, don't multiply by 0 */
		sbi->fsinfo_sector = le16_to_cpu(b->info_sector);
		if (sbi->fsinfo_sector == 0)
			sbi->fsinfo_sector = 1;

		fsinfo_bh = sb_bread(sb, sbi->fsinfo_sector);
		if (fsinfo_bh == NULL) {
			fat_msg(sb, KERN_ERR, "bread failed, FSINFO block"
			       " (sector = %lu)", sbi->fsinfo_sector);
			brelse(bh);
			goto out_fail;
		}

		fsinfo = (struct fat_boot_fsinfo *)fsinfo_bh->b_data;
		if (!IS_FSINFO(fsinfo)) {
			fat_msg(sb, KERN_WARNING, "Invalid FSINFO signature: "
			       "0x%08x, 0x%08x (sector = %lu)",
			       le32_to_cpu(fsinfo->signature1),
			       le32_to_cpu(fsinfo->signature2),
			       sbi->fsinfo_sector);
		} else {
			if (sbi->options.usefree)
				sbi->free_clus_valid = 1;
			sbi->free_clusters = le32_to_cpu(fsinfo->free_clusters);
			sbi->prev_free = le32_to_cpu(fsinfo->next_cluster);
		}

		brelse(fsinfo_bh);
	}

	sbi->dir_per_block = sb->s_blocksize / sizeof(struct msdos_dir_entry);
	sbi->dir_per_block_bits = ffs(sbi->dir_per_block) - 1;

	sbi->dir_start = sbi->fat_start + sbi->fats * sbi->fat_length;
#ifndef CONFIG_SNSC_FS_FAT_STRICT_CHECK_ON_BOOT_SECTOR
	sbi->dir_entries = get_unaligned_le16(&b->dir_entries);
#endif
	if (sbi->dir_entries & (sbi->dir_per_block - 1)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus directroy-entries per block"
			       " (%u)", sbi->dir_entries);
		brelse(bh);
		goto out_invalid;
	}

	rootdir_sectors = sbi->dir_entries
		* sizeof(struct msdos_dir_entry) / sb->s_blocksize;
	sbi->data_start = sbi->dir_start + rootdir_sectors;
	total_sectors = get_unaligned_le16(&b->sectors);
	if (total_sectors == 0)
		total_sectors = le32_to_cpu(b->total_sect);

	total_clusters = (total_sectors - sbi->data_start) / sbi->sec_per_clus;

	if (sbi->fat_bits != 32)
		sbi->fat_bits = (total_clusters > MAX_FAT12) ? 16 : 12;

	/* check that FAT table does not overflow */
	fat_clusters = sbi->fat_length * sb->s_blocksize * 8 / sbi->fat_bits;
	total_clusters = min(total_clusters, fat_clusters - FAT_START_ENT);
	if (total_clusters > MAX_FAT(sb)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "count of clusters too big (%u)",
			       total_clusters);
		brelse(bh);
		goto out_invalid;
	}

	sbi->max_cluster = total_clusters + FAT_START_ENT;
	/* check the free_clusters, it's not necessarily correct */
	if (sbi->free_clusters != -1 && sbi->free_clusters > total_clusters)
		sbi->free_clusters = -1;
	/* check the prev_free, it's not necessarily correct */
	sbi->prev_free %= sbi->max_cluster;
	if (sbi->prev_free < FAT_START_ENT)
		sbi->prev_free = FAT_START_ENT;

	brelse(bh);

	/* set up enough so that it can read an inode */
	fat_hash_init(sb);
	fat_ent_access_init(sb);

	/*
	 * The low byte of FAT's first entry must have same value with
	 * media-field.  But in real world, too many devices is
	 * writing wrong value.  So, removed that validity check.
	 *
	 * if (FAT_FIRST_ENT(sb, media) != first)
	 */

	error = -EINVAL;
	sprintf(buf, "cp%d", sbi->options.codepage);
	sbi->nls_disk = load_nls(buf);
	if (!sbi->nls_disk) {
		fat_msg(sb, KERN_ERR, "codepage %s not found", buf);
		goto out_fail;
	}

	/* FIXME: utf8 is using iocharset for upper/lower conversion */
	if (sbi->options.isvfat) {
		sbi->nls_io = load_nls(sbi->options.iocharset);
		if (!sbi->nls_io) {
			fat_msg(sb, KERN_ERR, "IO charset %s not found",
			       sbi->options.iocharset);
			goto out_fail;
		}
	}

	error = -ENOMEM;
	fat_inode = new_inode(sb);
	if (!fat_inode)
		goto out_fail;
	MSDOS_I(fat_inode)->i_pos = 0;
	sbi->fat_inode = fat_inode;
	root_inode = new_inode(sb);
	if (!root_inode)
		goto out_fail;
	root_inode->i_ino = MSDOS_ROOT_INO;
	root_inode->i_version = 1;
	error = fat_read_root(root_inode);
	if (error < 0)
		goto out_fail;
	error = -ENOMEM;
	insert_inode_hash(root_inode);
	sb->s_root = d_alloc_root(root_inode);
	if (!sb->s_root) {
		fat_msg(sb, KERN_ERR, "get root inode failed");
		goto out_fail;
	}

#ifdef CONFIG_SNSC_FS_VFAT_CLEAN_SHUTDOWN_BIT
	if (sbi->options.clnshutbit) {
		int cn;
		int clnshutbit;
		struct fat_entry fatent;
		int count = 1;

		clnshutbit = 1;
		fatent_init(&fatent);
		fatent.fat_inode = sbi->fat_inode;
		cn = fat_ent_raw_read(sb, &fatent, 1);
		if (cn == -1) {
			printk("FAT: fail to read FAT\n");
			goto out_invalid;
		}
retry:
		switch (sbi->fat_bits) {
		case 32:
			if (((cn | 0x0c000000) & 0x0fffffff) < 0x0ffffff8) {
				printk("FAT: broken FAT (FAT[1]=%x)\n", cn);
				goto out_invalid;
			}
			clnshutbit = (cn & 0x08000000) ? 1 : 0;
			if (!(sb->s_flags & MS_RDONLY) && clnshutbit) {
				if (fat_ent_raw_write(sb, &fatent, cn & ~0x08000000) != 0) {
					if (count-- > 0)
						goto retry;
					printk("FAT: fail to clear ClnShutBit\n");
					goto out_invalid;
				}
			}
			break;
		case 16:
			if ((cn | 0xc000) < 0x0000fff8) {
				printk("FAT: broken FAT (FAT[1]=%x)\n", cn);
				goto out_invalid;
			}
			clnshutbit = (cn & 0x8000) ? 1 : 0;
			if (!(sb->s_flags & MS_RDONLY) && clnshutbit) {
				if (fat_ent_raw_write(sb, &fatent,
						       (cn & ~0x8000) & 0xffff) != 0) {
					if (count-- > 0)
						goto retry;
					printk("FAT: fail to clear ClnShutBit\n") ;
					goto out_invalid;
				}
			}
			break;
		case 12:
			if (cn < 0x00000ff8) {
				printk("FAT: broken FAT (FAT[1]=%x)\n", cn);
				goto out_invalid;
			}
			break;
		}
		if (!clnshutbit)
			printk("FAT: %s-fs: not clean shutdown\n",
			       sbi->fat_bits == 32 ? "FAT32" : "FAT16");
		sbi->clnshutbit = clnshutbit;
	}
#endif
#ifdef CONFIG_SNSC_FS_VFAT_POSIX_ATTR
	MSDOS_SB(sb)->posix_ops = posix_noops;
#endif
#ifdef CONFIG_SNSC_FS_FAT_GC
	fat_start_gc(sb);
#endif

	return 0;

out_invalid:
	error = -EINVAL;
	if (!silent)
		fat_msg(sb, KERN_INFO, "Can't find a valid FAT filesystem");

out_fail:
	if (fat_inode)
		iput(fat_inode);
	if (root_inode)
		iput(root_inode);
	unload_nls(sbi->nls_io);
	unload_nls(sbi->nls_disk);
	if (sbi->options.iocharset != fat_default_iocharset)
		kfree(sbi->options.iocharset);
	sb->s_fs_info = NULL;
	kfree(sbi);
	return error;
}

EXPORT_SYMBOL_GPL(fat_fill_super);

#ifdef CONFIG_SNSC_FS_FAT_BATCH_SYNC
/*
 * earse DIRTY bit of inode
 * In linux-3.0 kernel, "inode_in_use" is obseletted, therefore
 * move dirty bit from dirty list to inode_in_use is removed
 */
void fat_mark_inode_clean(struct inode *inode)
{
	if (!(inode->i_state & I_DIRTY))
		return;

	if (unlikely(block_dump)) {
		struct dentry *dentry = NULL;
		const char *name = "?";

		if (!list_empty(&inode->i_dentry)) {
			dentry = list_entry(inode->i_dentry.next,
						struct dentry, d_alias);
                        if (dentry && dentry->d_name.name)
				name = (const char *) dentry->d_name.name;
		}

		if (inode->i_ino || strcmp(inode->i_sb->s_id, "bdev"))
			printk(KERN_DEBUG
				"%s(%d): clean inode %lu (%s) on %s\n",
				current->comm, current->pid, inode->i_ino,
				name, inode->i_sb->s_id);
	}

	spin_lock(&inode->i_lock);

	if (inode->i_state & I_DIRTY) {
		inode->i_state &= ~I_DIRTY;

		/*
		 * If the inode is locked, just update its dirty state.
		 * The unlocker will place the inode on the appropriate
		 * superblock list, based upon its state.
		 */
		if (inode->i_state & I_NEW)
			goto out;

		if (inode->i_state & (I_FREEING|I_CLEAR))
			goto out;

	}
 out:
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(fat_mark_inode_clean);
#endif

/*
 * helper function for fat_flush_inodes.  This writes both the inode
 * and the file data blocks, waiting for in flight data blocks before
 * the start of the call.  It does not wait for any io started
 * during the call
 */
static int writeback_inode(struct inode *inode)
{

	int ret;
	struct address_space *mapping = inode->i_mapping;
	struct writeback_control wbc = {
	       .sync_mode = WB_SYNC_NONE,
	      .nr_to_write = 0,
	};
	/* if we used WB_SYNC_ALL, sync_inode waits for the io for the
	* inode to finish.  So WB_SYNC_NONE is sent down to sync_inode
	* and filemap_fdatawrite is used for the data blocks
	*/
	ret = sync_inode(inode, &wbc);
	if (!ret)
	       ret = filemap_fdatawrite(mapping);
	return ret;
}

/*
 * write data and metadata corresponding to i1 and i2.  The io is
 * started but we do not wait for any of it to finish.
 *
 * filemap_flush is used for the block device, so if there is a dirty
 * page for a block already in flight, we will not wait and start the
 * io over again
 */
int fat_flush_inodes(struct super_block *sb, struct inode *i1, struct inode *i2)
{
	int ret = 0;
	if (!MSDOS_SB(sb)->options.flush)
		return 0;
	if (i1)
		ret = writeback_inode(i1);
	if (!ret && i2)
		ret = writeback_inode(i2);
	if (!ret) {
		struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;
		ret = filemap_flush(mapping);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(fat_flush_inodes);

static int __init init_fat_fs(void)
{
	int err;

	err = fat_cache_init();
	if (err)
		return err;

	err = fat_init_inodecache();
	if (err)
		goto failed;

	return 0;

failed:
	fat_cache_destroy();
	return err;
}

static void __exit exit_fat_fs(void)
{
	fat_cache_destroy();
	fat_destroy_inodecache();
}

module_init(init_fat_fs)
module_exit(exit_fat_fs)

MODULE_LICENSE("GPL");
