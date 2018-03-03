/* 2012-10-10: File added and changed by Sony Corporation */
/*
 * CompSwap device
 *
 *  Copyright 2012 Sony Corporation
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 *  CompSwap is based on "ramzswap" or "zram"
 *  Copyright (C) 2008, 2009, 2010  Nitin Gupta
 */

#define KMSG_COMPONENT "CompSwap"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/lzo.h>
#include <linux/string.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot.h>
#endif

#include <linux/compswap.h>
#include "internal.h"

static int major;
static struct compswap *devices;
static unsigned int num_devices;

extern struct attribute_group cs_disk_attr_group;

static struct compswap_compressor_ops   *compressor_ops;
static struct compswap_decompressor_ops *decompressor_ops;

static inline int compswap_compressor_init(void **priv)
{
	if (compressor_ops->init)
		return compressor_ops->init(priv);
	else
		return 0;
}

static inline void compswap_compressor_exit(void *priv)
{
	if (compressor_ops->exit)
		compressor_ops->exit(priv);
}

static inline int compswap_compressor_compress(
	const void *src, size_t src_len,
	void *dst, size_t *dst_len,
	void *priv)
{
	return compressor_ops->compress(src, src_len, dst, dst_len, priv);
}

static inline int compswap_decompressor_init(void **priv)
{
	if (decompressor_ops->init)
		return decompressor_ops->init(priv);
	else
		return 0;
}

static inline void compswap_decompressor_exit(void *priv)
{
	if (decompressor_ops->exit)
		decompressor_ops->exit(priv);
}

static inline int compswap_decompressor_decompress(
	const void *src, size_t src_len,
	void *dst, size_t *dst_len,
	void *priv)
{
	return decompressor_ops->decompress(src, src_len,
					dst, dst_len, priv);
}

static void compswap_add_bio(struct compswap *cs, struct bio *bio)
{
	spin_lock_irq(&cs->bio_list_lock);
	bio_list_add(&cs->bio_list, bio);
	spin_unlock_irq(&cs->bio_list_lock);
}

static struct bio *compswap_get_bio(struct compswap *cs)
{
	struct bio *bio;

	spin_lock_irq(&cs->bio_list_lock);
	bio = bio_list_pop(&cs->bio_list);
	spin_unlock_irq(&cs->bio_list_lock);

	return bio;
}

static ssize_t compswap_read_file(struct file *file, void *buf,
				size_t count, loff_t *pos)
{
	mm_segment_t old_fs;
	ssize_t ret;

	old_fs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, (char __user *)buf, count, pos);

	set_fs(old_fs);
	return ret;
}

static ssize_t compswap_write_file(struct file *file, void *buf,
				size_t count, loff_t *pos)
{
	mm_segment_t old_fs;
	ssize_t ret;

	old_fs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, (char __user *)buf, count, pos);

	set_fs(old_fs);
	return ret;
}

static int page_zero_filled(void *ptr)
{
	unsigned int pos;
	unsigned long *page;

	page = (unsigned long *)ptr;

	for (pos = 0; pos != PAGE_SIZE / sizeof(*page); pos++) {
		if (page[pos])
			return 0;
	}
	return 1;
}

static void setup_swap_header(struct compswap *cs, union swap_header *s)
{
	s->info.version = 1;
	s->info.last_page = (cs->disksize >> PAGE_SHIFT) - 1;
	s->info.nr_badpages = 0;
	memcpy(s->magic.magic, "SWAPSPACE2", 10);
}

static void compswap_free_page(struct compswap *cs, size_t index)
{
	if (cs->flush_on) {
		return;
	}

	mutex_lock(&cs->lock);

	if (unlikely(!cs->mtable) ||
	    unlikely(!cs->mtable[index].page)) {
		mutex_unlock(&cs->lock);
		return;
	}

	__free_page(cs->mtable[index].page);
	cs->mtable[index].page = NULL;

	mutex_unlock(&cs->lock);
}

static int handle_zero_page(struct bio *bio)
{
	void *user_mem;
	struct page *page = bio->bi_io_vec[0].bv_page;

	user_mem = kmap_atomic(page, KM_USER0);
	memset(user_mem, 0, PAGE_SIZE);
	kunmap_atomic(user_mem, KM_USER0);

	flush_dcache_page(page);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return 0;
}

static int compswap_read(struct compswap *cs, struct bio *bio)
{
	u32 index;
	struct page *page;
	unsigned char *user_mem, *mem;

	page = bio->bi_io_vec[0].bv_page;
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	mutex_lock(&cs->lock);

	BUG_ON(!cs->mtable);

	if (!cs->mtable[index].page) {
		mutex_unlock(&cs->lock);
		return handle_zero_page(bio);
	}

	user_mem = kmap_atomic(page, KM_USER0);
	mem      = kmap_atomic(cs->mtable[index].page, KM_USER1);

	memcpy(user_mem, mem, PAGE_SIZE);

	kunmap_atomic(user_mem, KM_USER0);
	kunmap_atomic(mem, KM_USER1);

	flush_dcache_page(page);

	mutex_unlock(&cs->lock);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return 0;
}

static int compswap_read_from_file(struct compswap *cs, struct bio *bio)
{
	int ret;
	u32 index, index_major, index_minor;
	size_t clen, ulen;
	struct page *page;
	unsigned char *user_mem, *mem;
	loff_t pos;
	ssize_t r_size, size;

	mutex_lock(&cs->lock);

	if (unlikely(cs->file == NULL)) {
		goto out;
	}

	if (cs->decompressor_init_done == 0) {
		ret = compswap_decompressor_init(&cs->dpriv);
		if (ret < 0) {
			pr_err("Error initializing decompress engine\n");
			goto out;
		}
		cs->decompressor_init_done = 1;
	}

	page  = bio->bi_io_vec[0].bv_page;
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;
	index_minor = index % Z_PAGES;
	index_major = index - index_minor;

	if (cs->ftable[index_major / Z_PAGES].flag &
				(1 << index_minor)) {
		mutex_unlock(&cs->lock);
		return handle_zero_page(bio);
	}

	if ((index_major / Z_PAGES) != cs->cached_block_pos) {

		if (cs->ftable[index_major / Z_PAGES].size == Z_SIZE) {
			r_size = Z_SIZE;
			mem = cs->temp_buffer;
		} else {
			clen = cs->ftable[index_major / Z_PAGES].size;
			r_size = W_ALIGN(clen);
			mem = cs->compress_buffer;
		}

		pos  = cs->ftable[index_major / Z_PAGES].offs;
		size = compswap_read_file(cs->file, mem, r_size, &pos);

		if (size != r_size) {
			pr_err("File read failed!\n");
			goto out;
		}

		if (r_size != Z_SIZE) {
			/* decompress */
			ulen = Z_SIZE;
			ret = compswap_decompressor_decompress(
				mem, clen,
				cs->temp_buffer, &ulen,
				cs->dpriv);

			/* should NEVER happen */
			if (unlikely(ret != 0)) {
				pr_err("Decompression failed! "
				       "err=%d, page=%u\n",
					ret, index);
				goto out;
			}
		}
		cs->cached_block_pos = (index_major / Z_PAGES);
	}

	user_mem = kmap_atomic(page, KM_USER0);
	memcpy(user_mem,
		(void *)(cs->temp_buffer + (PAGE_SIZE * index_minor)),
		 PAGE_SIZE);
	kunmap_atomic(user_mem, KM_USER0);
	flush_dcache_page(page);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	mutex_unlock(&cs->lock);
	return 0;

out:
	bio_io_error(bio);
	mutex_unlock(&cs->lock);
	return -EIO;
}

static int compswap_write(struct compswap *cs, struct bio *bio)
{
	int ret;
	u32 index;
	struct page *page, *page_store;
	unsigned char *user_mem, *mem;

	page  = bio->bi_io_vec[0].bv_page;
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	user_mem = kmap_atomic(page, KM_USER0);
	if (page_zero_filled(user_mem)) {

		kunmap_atomic(user_mem, KM_USER0);
		compswap_free_page(cs, index);

		set_bit(BIO_UPTODATE, &bio->bi_flags);
		bio_endio(bio, 0);
		return 0;
	}

	mutex_lock(&cs->lock);

	BUG_ON(!cs->mtable);

	if (cs->mtable[index].page) {
		page_store = cs->mtable[index].page;
	} else {
		page_store = alloc_page(GFP_NOIO | __GFP_HIGHMEM);
		if (unlikely(!page_store)) {
			mutex_unlock(&cs->lock);
			kunmap_atomic(user_mem, KM_USER0);
			pr_info("Error allocating memory for incompressible "
				"page: %u\n", index);
			ret = -ENOMEM;
			goto out;
		}
	}

	cs->mtable[index].page = page_store;

	mem = kmap_atomic(cs->mtable[index].page, KM_USER1);
	memcpy(mem, user_mem, PAGE_SIZE);
	kunmap_atomic(mem,      KM_USER1);
	kunmap_atomic(user_mem, KM_USER0);

	mutex_unlock(&cs->lock);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return 0;

out:
	bio_io_error(bio);
	return ret;
}

static inline int valid_swap_request(struct compswap *cs, struct bio *bio)
{
	if (unlikely(bio->bi_rw & REQ_FLUSH)) {
		return 1;
	}

	if (unlikely(
		(bio->bi_sector >= (cs->disksize >> SECTOR_SHIFT)) ||
		(bio->bi_sector & (SECTORS_PER_PAGE - 1)) ||
		(bio->bi_vcnt != 1) ||
		(bio->bi_size != PAGE_SIZE) ||
		(bio->bi_io_vec[0].bv_offset != 0))) {

		return 0;
	}

	return 1;
}

static int compswap_flush(struct compswap *cs, struct bio *bio)
{
	size_t num_pages;
	struct page *page;
	struct file *file;
	u32 index, index_major, index_minor;
	loff_t f_offs = 0;
	ssize_t w_size, size;
	void *buf;
	loff_t pos;
	int all_zero = 0;
	u32 clen;
	int ret;
	size_t total_orig_size = 0;
	size_t total_comp_size = 0;
	int progress_count = 0;

	mutex_lock(&cs->lock);

	if (unlikely((cs->file != NULL))) {
		ret = -EINVAL;
		goto fail1;
	}

	num_pages = cs->disksize >> PAGE_SHIFT;

	file = filp_open(cs->fname,
			O_CREAT | O_TRUNC | O_RDWR | O_LARGEFILE, 600);
	if (IS_ERR(file)) {
		pr_err("File %s open failed!\n", cs->fname);
		ret = -EIO;
		goto fail1;
	}
 	cs->file = file;

	cs->ftable = vmalloc((num_pages / Z_PAGES) * sizeof(*cs->ftable));
	if (!cs->ftable) {
		pr_err("Error allocating file table\n");
		ret = -ENOMEM;
		goto fail1;
	}
	memset(cs->ftable, 0,
		((num_pages / Z_PAGES) * sizeof(*cs->ftable)));

	ret = compswap_compressor_init(&cs->cpriv);
	if (ret < 0) {
		pr_err("Error initializing compress engine\n");
		ret = -EIO;
		goto fail2;
	}

	pr_info("Compressing");

	for (index = 0; index < num_pages; index++) {

		index_minor = index % Z_PAGES;
		index_major = index - index_minor;

		/* clear buffer */
		if (index_minor == 0) {
			memset(cs->temp_buffer, 0, Z_SIZE);
			all_zero = 1;
		}

		page = cs->mtable[index].page;
		if (!page) {
			cs->ftable[index_major / Z_PAGES].flag |=
				(1 << index_minor);
		} else {
			all_zero = 0;

			/* copy */
			buf = kmap_atomic(page, KM_USER1);
			memcpy(cs->temp_buffer + (PAGE_SIZE * index_minor),
				buf, PAGE_SIZE);
			kunmap_atomic(buf, KM_USER1);

			__free_page(page);
			cs->mtable[index].page = NULL;

			total_orig_size += PAGE_SIZE;
		}

		if (index_minor != (Z_PAGES - 1))
			continue;

		if (all_zero == 1) {
			continue;
		}

		clen = (Z_PAGES << 1) * PAGE_SIZE;
		ret = compswap_compressor_compress(
				cs->temp_buffer, Z_SIZE,
				cs->compress_buffer, &clen,
				cs->cpriv);

		if (unlikely(ret != 0)) {
			pr_err("Compression failed! err=%d\n", ret);
			ret = -EIO;
			goto fail3;
		}

		if (clen > COMPRESSION_THRESHOLD) {
			w_size = Z_SIZE;
			clen   = Z_SIZE;
			buf    = cs->temp_buffer;

		} else {
			w_size = W_ALIGN(clen);
			buf    = cs->compress_buffer;
		}

		total_comp_size += w_size;

		pos  = f_offs;
		size = compswap_write_file(file, buf, w_size, &pos);

		if (size != w_size) {
			pr_err("File write failed!\n");
			ret = -EIO;
			goto fail3;
		}

		cs->ftable[index_major / Z_PAGES].offs = f_offs;
		cs->ftable[index_major / Z_PAGES].size = clen;

		f_offs += size;

		if (progress_count++ % 32 == 0)
			pr_cont(".");
	}

	pr_cont("done! (%uk/%uk)\n",
			total_comp_size >> 10, total_orig_size >> 10);

	/* issue sync to file */
	vfs_fsync(file, 0);

#ifdef CONFIG_SNSC_SSBOOT
	/* invalidate page cache associated with image file */
	ssboot_invalidate_page_cache(cs->fname);
#endif

	/* finish compression */
	compswap_compressor_exit(cs->cpriv);

	/* free mtable */
	vfree(cs->mtable);
	cs->mtable = NULL;

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);

	mutex_unlock(&cs->lock);

	return 0;
fail3:
	compswap_compressor_exit(cs->cpriv);
fail2:
	vfree(cs->ftable);
	cs->ftable = NULL;
fail1:
	bio_io_error(bio);
	mutex_unlock(&cs->lock);
	return ret;
}

static int compswap_thread(void *data)
{
	struct compswap *cs = data;
	struct bio *bio;

	set_user_nice(current, DEFAULT_THREAD_PRIORITY);

	while (!kthread_should_stop() || !bio_list_empty(&cs->bio_list)) {

		wait_event_interruptible(cs->event,
			!bio_list_empty(&cs->bio_list) || kthread_should_stop());

		if (bio_list_empty(&cs->bio_list)) {
			continue;
		}

		bio = compswap_get_bio(cs);
		BUG_ON(!bio);

		if (bio->bi_rw & REQ_FLUSH) {
			compswap_flush(cs, bio);
			continue;
		}

		switch (bio_data_dir(bio)) {
		case READ:
			compswap_read_from_file(cs, bio);
			break;
		case WRITE:
			bio_io_error(bio);
			break;
		}
	}
	return 0;
}

/*
 * Handler function for all compswap I/O requests.
 */
static int compswap_make_request(struct request_queue *queue,
				struct bio *bio)
{
	int ret = 0;
	struct compswap *cs = queue->queuedata;

	if (!valid_swap_request(cs, bio)) {
		bio_io_error(bio);
		return 0;
	}

	if (unlikely(!cs->init_done)) {
		bio_io_error(bio);
		return 0;
	}

	if ((bio->bi_rw & REQ_FLUSH) || cs->flush_on) {

		cs->flush_on = 1;
		compswap_add_bio(cs, bio);
		wake_up(&cs->event);

		return 0;
	}

	switch (bio_data_dir(bio)) {
	case READ:
		ret = compswap_read(cs, bio);
		break;
	case WRITE:
		ret = compswap_write(cs, bio);
		break;
	}

	return ret;
}

static void compswap_reset_device(struct compswap *cs)
{
	size_t index;

	mutex_lock(&cs->lock);

	if (!cs->init_done) {
		pr_info("Device already finalized!\n");
		return;
	}

	if (cs->decompressor_init_done) {
		compswap_decompressor_exit(&cs->dpriv);
		cs->decompressor_init_done = 0;
	}

	/* Do not accept any new I/O request */
	cs->init_done = 0;
	cs->flush_on  = 0;

	if (cs->thread) {
		kthread_stop(cs->thread);
		cs->thread = NULL;
	}

	if (cs->file) {
		filp_close(cs->file, NULL);
		cs->file = NULL;
	}

	/* Free various per-device buffers */
	free_pages((unsigned long)cs->compress_buffer, Z_PAGES_SHIFT + 1);
	free_pages((unsigned long)cs->temp_buffer,     Z_PAGES_SHIFT);

	cs->compress_buffer  = NULL;
	cs->temp_buffer      = NULL;

	/* Free all pages that are still in this compswap device */
	if (cs->mtable) {
		for (index = 0; index < cs->disksize >> PAGE_SHIFT;
			index++) {
			struct page *page;

			page = cs->mtable[index].page;
			if (page)
				__free_page(page);
		}
		vfree(cs->mtable);
		cs->mtable = NULL;
	}

	if (cs->ftable) {
		vfree(cs->ftable);
		cs->ftable = NULL;
	}

	cs->disksize = 0;
	set_capacity(cs->disk, 0);

	mutex_unlock(&cs->lock);
}

static int compswap_init_device(struct compswap *cs)
{
	int ret;
	size_t num_pages;
	struct page *page;
	union swap_header *swap_header;

	mutex_lock(&cs->lock);

	if (cs->init_done) {
		pr_info("Device already initialized!\n");
		mutex_unlock(&cs->lock);
		return 0;
	}

	if (!cs->disksize)
		cs->disksize = DEFAULT_DISKSIZE_KB << 10;

	/* Backing file name check */
	if (strlen(cs->fname) == 0) {
		pr_err("Backing file name is not specified\n");
		ret = -EIO;
		goto fail1;
	}

	/* Compressor/Decompressor plugin check */
	if (!compressor_ops || !decompressor_ops) {
		pr_err("Plugin is not registered\n");
		ret = -EIO;
		goto fail1;
	}

	cs->compress_buffer =
		 (void *)__get_free_pages(__GFP_ZERO, Z_PAGES_SHIFT + 1);
	if (!cs->compress_buffer) {
		pr_err("Error allocating compressor buffer space\n");
		ret = -ENOMEM;
		goto fail1;
	}

	cs->temp_buffer =
		(void *)__get_free_pages(__GFP_ZERO, Z_PAGES_SHIFT);
	if (!cs->temp_buffer) {
		pr_err("Error allocating temp buffer space\n");
		ret = -ENOMEM;
		goto fail2;
	}


	num_pages = cs->disksize >> PAGE_SHIFT;
	cs->mtable = vmalloc(num_pages * sizeof(*cs->mtable));
	if (!cs->mtable) {
		pr_err("Error allocating compswap address table\n");
		/* To prevent accessing table entries during cleanup */
		cs->disksize = 0;
		ret = -ENOMEM;
		goto fail3;
	}
	memset(cs->mtable, 0, num_pages * sizeof(*cs->mtable));

	page = alloc_page(__GFP_ZERO);
	if (!page) {
		pr_err("Error allocating swap header page\n");
		ret = -ENOMEM;
		goto fail4;
	}
	cs->mtable[0].page = page;

	swap_header = kmap(page);
	setup_swap_header(cs, swap_header);
	kunmap(page);

	set_capacity(cs->disk, cs->disksize >> SECTOR_SHIFT);

	/* compswap devices sort of resembles non-rotational disks */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, cs->disk->queue);

	bio_list_init(&cs->bio_list);
	cs->thread = kthread_create(compswap_thread, (void *) cs,
			"compswap%u", cs->disk->first_minor);
	wake_up_process(cs->thread);

	cs->cached_block_pos  = -1;

	cs->init_done = 1;
	mutex_unlock(&cs->lock);

	pr_debug("Initialization done!\n");
	return 0;

fail4:
	vfree(cs->mtable);
	cs->mtable = NULL;
fail3:
	free_pages((unsigned long)cs->temp_buffer,     Z_PAGES_SHIFT);
	cs->temp_buffer = NULL;
fail2:
	free_pages((unsigned long)cs->compress_buffer, Z_PAGES_SHIFT + 1);
	cs->compress_buffer = NULL;
fail1:
	mutex_unlock(&cs->lock);
	pr_err("Initialization failed: err=%d\n", ret);
	return ret;
}

void compswap_slot_free_notify(struct block_device *bdev,
				unsigned long index)
{
	struct compswap *cs;

	cs = bdev->bd_disk->private_data;
	compswap_free_page(cs, index);

	return;
}

static struct block_device_operations compswap_devops = {
	.swap_slot_free_notify = compswap_slot_free_notify,
	.owner = THIS_MODULE
};

static int create_device(struct compswap *cs, int device_id)
{
	int ret = 0;

	mutex_init(&cs->lock);

	cs->queue = blk_alloc_queue(GFP_KERNEL);
	if (!cs->queue) {
		pr_err("Error allocating disk queue for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	blk_queue_make_request(cs->queue, compswap_make_request);
	cs->queue->queuedata = cs;
	blk_queue_flush(cs->queue, REQ_FLUSH);

	 /* gendisk structure */
	cs->disk = alloc_disk(1);
	if (!cs->disk) {
		blk_cleanup_queue(cs->queue);
		pr_warning("Error allocating disk structure for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	cs->disk->major = major;
	cs->disk->first_minor = device_id;
	cs->disk->fops = &compswap_devops;
	cs->disk->queue = cs->queue;
	cs->disk->private_data = cs;
	scnprintf(cs->disk->disk_name, 16, "compswap%d", device_id);

	set_capacity(cs->disk, 0);

	blk_queue_physical_block_size(cs->disk->queue, PAGE_SIZE);
	blk_queue_logical_block_size(cs->disk->queue, PAGE_SIZE);

	blk_queue_io_min(cs->disk->queue, PAGE_SIZE);
	blk_queue_io_opt(cs->disk->queue, PAGE_SIZE);

	add_disk(cs->disk);

	cs->init_done = 0;

	ret = sysfs_create_group(&disk_to_dev(cs->disk)->kobj,
	                        &cs_disk_attr_group);
	if (ret < 0) {
	        pr_warning("Error creating sysfs group");
	        goto out;
	}

	init_waitqueue_head(&cs->event);

out:
	return ret;
}

static void destroy_device(struct compswap *cs)
{
	if (cs->disk) {
		del_gendisk(cs->disk);
		put_disk(cs->disk);
	}

	if (cs->queue)
		blk_cleanup_queue(cs->queue);
}

static int __init compswap_init(void)
{
	int ret, dev_id;

	if (num_devices > MAX_NUM_DEVICES) {
		pr_warning("Invalid value for num_devices: %u\n",
				num_devices);
		ret = -EINVAL;
		goto out;
	}

	/* if major is not specified by module param, use CONFIG */
	if (major == 0)
		major = MAJOR_NR;

	ret = register_blkdev(major, "compswap");
	if (ret < 0) {
		pr_warning("Unable to get major number\n");
		ret = -EBUSY;
		goto out;
	}

	/* major number is automatically selected by register_blkdev */
	if (major == 0)
		major = ret;

	if (!num_devices) {
		pr_info("num_devices not specified. Using default: 1\n");
		num_devices = 1;
	}

	/* Allocate the device array and initialize each one */
	pr_info("Creating %u devices ...\n", num_devices);
	devices = kzalloc(num_devices * sizeof(struct compswap),
					GFP_KERNEL);
	if (!devices) {
		ret = -ENOMEM;
		goto unregister;
	}

	for (dev_id = 0; dev_id < num_devices; dev_id++) {
		ret = create_device(&devices[dev_id], dev_id);
		if (ret)
			goto free_devices;
	}
	return 0;

free_devices:
	while (dev_id)
		destroy_device(&devices[--dev_id]);
unregister:
	unregister_blkdev(major, "compswap");
out:
	return ret;
}

static void __exit compswap_exit(void)
{
	int i;
	struct compswap *cs;

	for (i = 0; i < num_devices; i++) {
		cs = &devices[i];

		destroy_device(cs);
		if (cs->init_done)
			compswap_reset_device(cs);
	}

	unregister_blkdev(major, "compswap");

	kfree(devices);
	pr_debug("Cleanup done!\n");
}


int compswap_compressor_register(struct compswap_compressor_ops *ops)
{
	if (compressor_ops)
		return -EBUSY;

	if (!ops || !ops->compress)
		return -EINVAL;

	compressor_ops = ops;
	return 0;
}

int compswap_compressor_unregister(struct compswap_compressor_ops *ops)
{
	if (compressor_ops != ops)
		return -EINVAL;

	compressor_ops = NULL;
	return 0;
}

EXPORT_SYMBOL(compswap_compressor_register);
EXPORT_SYMBOL(compswap_compressor_unregister);

int compswap_decompressor_register(struct compswap_decompressor_ops *ops)
{
	if (decompressor_ops)
		return -EBUSY;

	if (!ops || !ops->decompress)
		return -EINVAL;

	decompressor_ops = ops;
	return 0;
}

int compswap_decompressor_unregister(struct compswap_decompressor_ops *ops)
{
	if (decompressor_ops != ops)
		return -EINVAL;

	decompressor_ops = NULL;
	return 0;
}

EXPORT_SYMBOL(compswap_decompressor_register);
EXPORT_SYMBOL(compswap_decompressor_unregister);

static struct compswap *dev_to_cs(struct device *dev)
{
        int i;
        struct compswap *cs = NULL;

        for (i = 0; i < num_devices; i++) {
                cs = &devices[i];
                if (disk_to_dev(cs->disk) == dev)
                        break;
        }
        return cs;
}

static ssize_t init_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	struct compswap *cs = dev_to_cs(dev);

	ret = compswap_init_device(cs);

	if (ret < 0)
		return -EINVAL;
	return len;
}

static ssize_t reset_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	struct compswap *cs = dev_to_cs(dev);

	compswap_reset_device(cs);

	return len;
}

static ssize_t backing_file_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct compswap *cs = dev_to_cs(dev);

	return scnprintf(buf, PAGE_SIZE, "%s\n", cs->fname);
}

static ssize_t backing_file_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	struct compswap *cs = dev_to_cs(dev);
	size_t filename_len = (len > MAX_FNAME_LEN) ? MAX_FNAME_LEN : len;

	if (cs->init_done) {
		pr_info("Cannot change filename for initialized device\n");
		return -EBUSY;
	}

	strncpy(cs->fname, buf, (filename_len - 1));
	cs->fname[filename_len - 1] = '\0';

	return len;
}

static ssize_t disksize_kb_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct compswap *cs = dev_to_cs(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
		((u32)(cs->disksize >> 10)));
}

static ssize_t disksize_kb_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	struct compswap *cs = dev_to_cs(dev);
	u64 disksize_kb;

	if (cs->init_done) {
		pr_info("Cannot change disksize for initialized device\n");
		return -EBUSY;
	}

	ret = strict_strtoull(buf, 10, &disksize_kb);
	if (ret)
		return ret;

	cs->disksize = disksize_kb << 10;
	set_capacity(cs->disk, cs->disksize >> SECTOR_SHIFT);

	return len;
}

static DEVICE_ATTR(init, S_IWUSR, NULL, init_store);
static DEVICE_ATTR(reset, S_IWUSR, NULL, reset_store);
static DEVICE_ATTR(backing_file, S_IRUGO | S_IWUSR,
		backing_file_show, backing_file_store);
static DEVICE_ATTR(disksize_kb, S_IRUGO | S_IWUSR,
		disksize_kb_show, disksize_kb_store);

static struct attribute *cs_disk_attrs[] = {
        &dev_attr_init.attr,
        &dev_attr_reset.attr,
	&dev_attr_backing_file.attr,
	&dev_attr_disksize_kb.attr,
        NULL,
};

struct attribute_group cs_disk_attr_group = {
        .attrs = cs_disk_attrs,
};

module_param(num_devices, uint, 0);
MODULE_PARM_DESC(num_devices, "Number of CompSwap devices");
module_param(major, uint, 0);
MODULE_PARM_DESC(major, "Device major number");

module_init(compswap_init);
module_exit(compswap_exit);

MODULE_LICENSE("Dual BSD/GPL");

MODULE_DESCRIPTION("CompSwap Device");
