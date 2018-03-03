/*
 * fs/partitions/sdm.c
 *
 * SDM partiton table handler
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on fs/partitions/sun.c
 */
/*
 *  fs/partitions/sun.c
 *
 *  Code extracted from drivers/block/genhd.c
 *
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 */

#include <linux/hdreg.h>
#include <asm/uaccess.h>

#include "check.h"
#include "sdm.h"

#include "sdm_partition_table.h"

int
#if 0
sdm_partition(struct parsed_partitions *state, struct block_device *bdev)
#else
sdm_partition(struct parsed_partitions *state)
#endif
{
	struct block_device *bdev = state->bdev;
	int i, limit;
	Sector sector;
	unsigned int n_sectors;
	sdm_partition_tbl *label;
	struct sdm_partition *p;
	unsigned int start_sector, num_sectors;

	if (!bdev || !bdev->bd_inode)
		return 0;
	n_sectors = bdev->bd_inode->i_size >> SECTOR_SHIFT;
	if ((label = (void *)read_dev_sector(bdev, 0, &sector)) == NULL)
		return -1;
	if (label->magic != SDM_LABEL_MAGIC
	    || label->version != SDM_LABEL_VERSION
	    || label->n_partition > SDM_LABEL_N_PARTITION){
		put_dev_sector(sector);
		return 0;
	}
	limit = label->n_partition + 1;
	if (limit > state->limit)
		limit = state->limit;
	for (i = 1, p = label->partitions; i < limit; i++, p++){
		if (!(p->flag & SDM_LABEL_VALID) ||  p->size == 0)
			continue;
		if (p->start & (SECTOR_SIZE - 1))
			continue;
		start_sector = p->start >> SECTOR_SHIFT;
		if (p->size & (SECTOR_SIZE - 1))
			continue;
		num_sectors = p->size >> SECTOR_SHIFT;
		if (start_sector >= n_sectors)
			continue;
		if (num_sectors > n_sectors)
			continue;
		if (start_sector + num_sectors > n_sectors)
			continue;
		put_partition(state, i, start_sector, num_sectors);
	}
	put_dev_sector(sector);
	printk("\n");

	return 1;
}
