/* 2012-05-10: File added by Sony Corporation */
/*
 *  CompSwap device
 *
 *  Copyright 2012 Sony Corporation
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 *  CompSwap is based on "ramzswap" or "zram"
 *  Copyright (C) 2008, 2009, 2010  Nitin Gupta
 */

#ifndef _COMPSWAP_DRV_H_
#define _COMPSWAP_DRV_H_

#include <linux/spinlock.h>
#include <linux/mutex.h>

/*-- Configurable parameters */
#define DEFAULT_DISKSIZE_KB	65536
#define DEFAULT_THREAD_PRIORITY	(-10)
#define COMPRESSION_THRESHOLD	(PAGE_SIZE * 7)
#define Z_PAGES_SHIFT		3		/* 3: 4096 * 2^3 = 32k */


#define Z_PAGES			(1 << Z_PAGES_SHIFT)
#define Z_SIZE			(Z_PAGES * PAGE_SIZE)

#define MAX_NUM_DEVICES		4

#define W_ALIGN_SIZE		16
#define W_ALIGN(size)		((size + W_ALIGN_SIZE - 1) \
				/ W_ALIGN_SIZE * W_ALIGN_SIZE)

#define MAX_FNAME_LEN		128

#define SECTOR_SHIFT		9
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)

#define MAJOR_NR		CONFIG_COMPSWAP_MAJOR_NR

struct cs_mtable_entry {
	struct page *page;
} __attribute__((aligned(4)));

struct cs_ftable_entry {
	loff_t offs;
	u16    size;
	u16    flag;
} __attribute__((aligned(4)));

struct compswap {
	unsigned char *compress_buffer;
	unsigned char *temp_buffer;
	int cached_block_pos;

	struct cs_mtable_entry *mtable;
	struct cs_ftable_entry *ftable;

	struct mutex lock;
	struct request_queue *queue;
	struct gendisk *disk;

	u64 disksize;

	char fname[MAX_FNAME_LEN];
	struct file *file;

	spinlock_t bio_list_lock;
	wait_queue_head_t event;
	struct bio_list bio_list;
	struct task_struct *thread;

	int init_done;
	int touched;
	int flush_on;

	struct completion complete;

	void *cpriv;
	void *dpriv;
	int decompressor_init_done;
};


#endif /* _COMPSWAP_DRV_H_ */
