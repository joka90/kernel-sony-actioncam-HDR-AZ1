/*
 * drivers/udif/bdev.c
 *
 * UDM
 *
 * Copyright 2012,2013 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/buffer_head.h>
#include <linux/loop.h>
#include <linux/udif/bdev.h>
#include <linux/udif/boottime.h>
#include <linux/udif/macros.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#define SECTOR_SHIFT 9

#define DISK_DATA(x)	((x)->private_data)
#define BDEV_DATA(x)	DISK_DATA((x)->bd_disk)
#define INODE_DATA(x)	BDEV_DATA((x)->i_bdev)
#define DISK_DEVNO(x)	MKDEV((x)->major, (x)->first_minor)

static unsigned int bdev_plcy= 0; /*0:Normal, 1:FIFO, 2:RR */
module_param_named(plcy, bdev_plcy, uint, S_IRUSR);
static unsigned int bdev_prio = 0;
module_param_named(prio, bdev_prio, uint, S_IRUSR);

#ifdef CONFIG_BLK_DEV_DECOMPRESS
# define UDIF_DECLARE_REQUEST(name, q, req, s) \
UDIF_REQUEST name = { \
	.write		= rq_data_dir(req), \
	.sector		= sector, \
	.nr_sectors	= nr_sectors, \
	.buffer		= (req)->buffer, \
	.sg		= s, \
	.rq		= req, \
	.que		= q, \
	.dec		= 0, \
}

static unsigned int udif_read_decompress(struct block_device *bdev,
					 sector_t sector, unsigned long nr_sectors,
					 struct disk_decompress *dec)
{
	UDIF_BDEV *dev = DISK_DATA(bdev->bd_disk);
	UDIF_DECLARE_FILE(fl, DISK_DEVNO(bdev->bd_disk), &dev->data);
	unsigned int xferred = 0;
	int ret;
	UDIF_REQUEST rq = {
		.write		= 0,
		.sector		= sector,
		.nr_sectors	= nr_sectors,
		.buffer		= 0,
		.sg		= 0,
		.rq		= 0,
		.dec		= dec,
	};

	ret = UDIF_CALL_FN(dev, request, int, -ENOSYS, &fl, &rq, &xferred);

	if (unlikely(ret))
		return 0;

	return dec->dstlen;
}

# else
#define UDIF_DECLARE_REQUEST(name, q, req, s) \
UDIF_REQUEST name = { \
	.write		= rq_data_dir(req), \
	.sector		= blk_rq_pos(req), \
	.nr_sectors	= blk_rq_sectors(req), \
	.buffer		= (req)->buffer, \
	.sg		= s, \
	.rq		= req, \
	.que		= q, \
}

#endif /* CONFIG_BLD_DEV_COMPRESS */

void udif_bdev_map_rq(UDIF_REQUEST *urq)
{
	urq->sg_len = blk_rq_map_sg(urq->que, urq->rq, urq->sg);
	urq->sg_len = dma_map_sg(NULL, urq->sg, urq->sg_len, urq->write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}

void udif_bdev_unmap_rq(UDIF_REQUEST *urq)
{
	dma_unmap_sg(NULL, urq->sg, urq->sg_len, urq->write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}

void udif_do_request(struct request_queue *q, struct request *rq)
{
	UDIF_BDEV *dev = DISK_DATA(rq->rq_disk);
	UDIF_DECLARE_FILE(fl, DISK_DEVNO(rq->rq_disk), &dev->data);
	unsigned int xferred;
	int err;

	do {
		UDIF_DECLARE_REQUEST(urq, q, rq, dev->sg);
		xferred = 0;

		if (urq.sg && !(dev->flags & UDIF_BDEV_FLAGS_XMAPSG)) {
			udif_bdev_map_rq(&urq);
		}

		spin_unlock_irq(q->queue_lock);

		err = UDIF_CALL_FN(dev, request, int, -ENOSYS, &fl, &urq, &xferred);

		spin_lock_irq(q->queue_lock);

		if (urq.sg && !(dev->flags & UDIF_BDEV_FLAGS_XMAPSG))
			udif_bdev_unmap_rq(&urq);

		if (unlikely(err)) {
			if (err > 0)
				err = -EIO;
			__blk_end_request_all(rq, err);
			break;
		}

	} while (__blk_end_request(rq, err, xferred << SECTOR_SHIFT));
}

#ifdef CONFIG_OSAL_UDIF_BDEV_THREAD
static void udif_request(struct request_queue *q)
{
	struct request *rq = NULL;
	UDIF_BDEV *dev = q->queuedata;

	if (!dev || !dev->th) {
		while ((rq = blk_fetch_request(q)))
			__blk_end_request_all(rq, -ENODEV);

		return;
	}

	wake_up_process(dev->th);
}

static void __udif_request(struct request_queue *q)
#else
static void udif_request(struct request_queue *q)
#endif
{
	struct request *rq;

	while ((rq = blk_fetch_request(q))) {
		if (rq->cmd_type != REQ_TYPE_FS) {
			__blk_end_request_all(rq, -EIO);
			continue;
		}

		udif_do_request(q, rq);
	}
}

#ifdef CONFIG_OSAL_UDIF_BDEV_THREAD
static int udif_bdev_thread(void *arg)
{
	struct request_queue *rq = arg;

	if (set_thread_priority(bdev_plcy, bdev_prio) < 0) {
		printk("ERROR:%s:set_thread_priority(%d,%d) failed\n", __FUNCTION__, bdev_plcy, bdev_prio);
	}
	spin_lock_irq(rq->queue_lock);
	current->flags |= PF_NOFREEZE;

	while (!kthread_should_stop()) {
		/* must invoke with queue_lock held */
		__udif_request(rq);

		if (kthread_should_stop())
			break;

		set_current_state(TASK_UNINTERRUPTIBLE);

		spin_unlock_irq(rq->queue_lock);
		schedule();
		spin_lock_irq(rq->queue_lock);

		set_current_state(TASK_RUNNING);
	}

	spin_unlock_irq(rq->queue_lock);

	return 0;
}
#endif

static int udif_open(struct block_device *bdev, fmode_t mode)
{
	UDIF_BDEV *dev = BDEV_DATA(bdev);
	UDIF_DECLARE_FILE(fl, bdev->bd_dev, &dev->data);
	UDIF_ERR err;

	/* avoid to exec rmmod while openning driver */
	if (unlikely(!try_module_get(dev->owner))) {
		UDIF_PERR("Unable get module %s\n", dev->node->name);
		return UDIF_ERR_IO;
	}

	err = UDIF_CALL_FN(dev, open, UDIF_ERR, UDIF_ERR_OK, &fl);

	if (err != UDIF_ERR_OK) {
		module_put(dev->owner);
	}

	return err;
}

static int udif_close(struct gendisk *disk, fmode_t mode)
{
	UDIF_BDEV *dev = DISK_DATA(disk);
	UDIF_DECLARE_FILE(fl, DISK_DEVNO(disk), &dev->data);
	UDIF_ERR err;

	err = UDIF_CALL_FN(dev, close, UDIF_ERR, UDIF_ERR_OK, &fl);

	module_put(dev->owner);

	return err;
}

static int udif_getgeo(struct block_device *bdev, struct hd_geometry *hd_geo)
{
	UDIF_BDEV *dev = BDEV_DATA(bdev);
	UDIF_GEOMETRY geo;
	UDIF_DECLARE_FILE(fl, bdev->bd_dev, &dev->data);
	int ret;

	ret = UDIF_CALL_FN(dev, getgeo, UDIF_INT, -ENOSYS, &fl, &geo);
	if (!ret) {
		hd_geo->heads	= geo.heads;
		hd_geo->sectors	= geo.sectors;
		hd_geo->cylinders = geo.cylinders;
		if (geo.start == UDIF_GEOMETRY_START_DEFAULT)
			hd_geo->start = get_start_sect(bdev);
		else
			hd_geo->start = geo.start;
	}

	return ret;
}

static int udif_revalidate(struct gendisk *disk)
{
	UDIF_BDEV *dev = DISK_DATA(disk);
	UDIF_DECLARE_FILE(fl, DISK_DEVNO(disk), &dev->data);
	sector_t sectors = 0;
	int ret;

	ret = UDIF_CALL_FN(dev, revalidate, int, -ENOSYS, &fl, &sectors);
	if (!ret)
		set_capacity(disk, sectors);

	return ret;
}

static int udif_media_changed(struct gendisk *disk)
{
	UDIF_BDEV *dev = DISK_DATA(disk);
	UDIF_DECLARE_FILE(fl, DISK_DEVNO(disk), &dev->data);

	return UDIF_CALL_FN(dev, media_changed, int, -ENOSYS, &fl);
}

static int udif_ioctl(struct block_device *bdev, fmode_t mode,
		      unsigned int cmd, unsigned long arg)
{
	UDIF_BDEV *dev = BDEV_DATA(bdev);
	UDIF_DECLARE_FILE(fl, bdev->bd_dev, &dev->data);
	UDIF_DECLARE_IOCTL(ictl, cmd, arg);
	struct hd_geometry geo;
	int ret;

	if (cmd == LOOP_CLR_FD) {
		/* ignore */
		ret = 0;
	} else if (cmd == BLKFLSBUF) {
		fsync_bdev(bdev);
		invalidate_bdev(bdev);
		ret = 0;
	} else if (cmd == HDIO_GETGEO) {
		ret = udif_getgeo(bdev, &geo);
		if (!ret) {
			if (access_ok(VERIFY_WRITE, arg, sizeof(geo)) &&
			    copy_to_user((long *)arg, &geo, sizeof(geo))) {
				UDIF_PERR("%s: HDIO_GETGEO access error\n", dev->node->name);
				ret = -EFAULT;
			}
		}
	} else {
		ret = UDIF_CALL_FN(dev, ioctl, int, -ENOSYS, &fl, &ictl);
		if (!ret && ictl.disk_change) {
			check_disk_change(bdev);
		}
	}

	return ret;
}

static struct block_device_operations udif_fops = {
	.open	= udif_open,
	.release = udif_close,
	.ioctl	= udif_ioctl,
	.getgeo	= udif_getgeo,
	.revalidate_disk = udif_revalidate,
	.media_changed	= udif_media_changed,
};

#include <linux/udif/list.h>
#include <linux/udif/mutex.h>
#include <linux/udif/driver.h>
#include <linux/suspend.h>

static UDIF_LIST_HEAD(bdev_driver_list);
static UDIF_DECLARE_MUTEX(bdev_driver_mtx);

struct bdev_driver {
	UDIF_LIST list;
	UDIF_BDEV *dev;
	UDIF_DRIVER drv;
};

static UDIF_ERR udif_bdev_suspend(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	struct bdev_driver *bdrv = data;
	struct gendisk *disk = bdrv->dev->disk;
	int i;

	if (pm_get_state() != PM_SUSPEND_DISK)
		return UDIF_ERR_OK;

	for (i=0;i<30;i++) {
		struct block_device *bdev = bdget_disk(disk, i);
		struct super_block *sb;

		if (!bdev)
			continue;

		sb = get_super(bdev);
		if (!sb)
			continue;

		if (sb->s_flags & MS_RDONLY) {
			drop_super(sb);
			continue;
		}

		printk("sync %s partition %d\n", bdrv->drv.name, i);
		sync_filesystem(sb);
		drop_super(sb);
	}

	return UDIF_ERR_OK;
}

static UDIF_ERR udif_bdev_resume(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	struct bdev_driver *bdrv = data;
	struct gendisk *disk = bdrv->dev->disk;
	int i;

	if (pm_get_state() != PM_SUSPEND_DISK || pm_is_mem_alive())
		return UDIF_ERR_OK;

	for (i=0;i<30;i++) {
		struct block_device *bdev = bdget_disk(disk, i);
		struct super_block *sb;

		if (!bdev)
			continue;

		sb = get_super(bdev);
		if (!sb)
			continue;

		if (sb->s_flags & MS_RDONLY) {
			drop_super(sb);
			continue;
		}

		drop_super(sb);
		printk("invalidate %s partition %d\n", bdrv->drv.name, i);
		__invalidate_device(bdev, true);
	}

	return UDIF_ERR_OK;
}

static UDIF_DRIVER_OPS udif_bdev_ops = {
	.suspend = udif_bdev_suspend,
	.resume = udif_bdev_resume,
};

void udif_driver_add(UDIF_DRIVER *drv);
void udif_driver_del(UDIF_DRIVER *drv);
static void add_to_driver_list(UDIF_BDEV *dev)
{
	struct bdev_driver *bdrv = kzalloc(sizeof(*bdrv), GFP_KERNEL);
	struct gendisk *disk = dev->disk;

	bdrv->dev = dev;
	snprintf((void *)bdrv->drv.name, sizeof(bdrv->drv.name), "b-%d", disk->major);
	bdrv->drv.ops = &udif_bdev_ops;
	*(void **)(&bdrv->drv.data) = bdrv;

	udif_mutex_lock(&bdev_driver_mtx);
	udif_list_add_tail(&bdrv->list, &bdev_driver_list);
	udif_mutex_unlock(&bdev_driver_mtx);

	udif_driver_add(&bdrv->drv);
}

static void del_from_driver_list(UDIF_BDEV *dev)
{
	struct bdev_driver *bdrv;

	udif_mutex_lock(&bdev_driver_mtx);
	udif_list_for_each_entry(bdrv, &bdev_driver_list, list) {
		if (bdrv->dev == dev) {
			udif_list_del(&bdrv->list);
			udif_driver_del(&bdrv->drv);
			kfree(bdrv);
			break;
		}
	}
	udif_mutex_unlock(&bdev_driver_mtx);
}

#define BLK_SET_PARAM(disk, fn, param) \
({ \
	if (param) \
		fn((disk)->queue, param); \
})

UDIF_ERR udif_bdev_register(const UDIF_DEVNODE *node)
{
	UDIF_ERR ret;

	UDIF_PARM_CHK(!node, "invalid node", UDIF_ERR_PAR);
	UDIF_PARM_CHK(node->type != UDIF_TYPE_BDEV, "type is not block device", UDIF_ERR_PAR);

	ret = register_blkdev(node->major, node->name);
	if (unlikely(ret)) {
		UDIF_PERR("failed register_blkdev(): %s, major = %d\n", node->name, node->major);
		return UDIF_ERR_PAR;
	}
	return UDIF_ERR_OK;
}

void udif_bdev_unregister(const UDIF_DEVNODE *node)
{
	if (unlikely(!node)) {
		UDIF_PERR("invalid node\n");
		return;
	}

	if (unlikely(node->type != UDIF_TYPE_BDEV)) {
		UDIF_PERR("type is not block device\n");
		return;
	}

	unregister_blkdev(node->major, node->name);
}

UDIF_ERR __udif_bdev_add_disk(UDIF_BDEV *dev)
{
	struct gendisk *disk;

	UDIF_PARM_CHK(!dev, "invalid dev", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!dev->ops, "invalid ops", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!dev->node, "invalid node", UDIF_ERR_PAR);
	UDIF_PARM_CHK(dev->node->type != UDIF_TYPE_BDEV, "type is not block device", UDIF_ERR_PAR);
	UDIF_PARM_CHK(dev->disk, "already added disk", UDIF_ERR_PAR);

	dev->disk = disk = alloc_disk(dev->node->nr_minor);
	if (unlikely(!disk)) {
		UDIF_PERR("failed alloc_disk(): %s, nr_minor = %d\n", dev->node->name, dev->node->nr_minor);
		return UDIF_ERR_NOMEM;
	}

	spin_lock_init(&dev->lock);
	disk->queue = blk_init_queue(udif_request, &dev->lock);
	if (unlikely(!disk->queue)) {
		UDIF_PERR("failed blk_init_queue(): %s\n", dev->node->name);
		del_gendisk(disk);
		return UDIF_ERR_NOMEM;
	}

	disk->major = dev->node->major;
	disk->first_minor = dev->node->first_minor;
	disk->minors = dev->node->nr_minor;
	disk->fops = &udif_fops;
	disk->flags = dev->flags;
	disk->private_data = dev;

#ifdef CONFIG_BLK_DEV_DECOMPRESS
	disk->algo = dev->algo;
	if (disk->algo)
		disk->blk_read_decompress = udif_read_decompress;
#endif

	snprintf(disk->disk_name, sizeof(disk->disk_name), \
		 "%s%d", dev->node->name, disk->first_minor);

	BLK_SET_PARAM(disk, blk_queue_logical_block_size, dev->sector_size);
	BLK_SET_PARAM(disk, blk_queue_max_hw_sectors, dev->max_sectors);
	BLK_SET_PARAM(disk, blk_queue_max_segment_size, dev->max_segment_size);
	BLK_SET_PARAM(disk, blk_queue_max_segments, dev->max_hw_segments);
	BLK_SET_PARAM(disk, blk_queue_max_segments, dev->phys_segments);
	BLK_SET_PARAM(disk, blk_queue_dma_alignment, dev->dma_alignment);

	set_capacity(disk, dev->capacity);

#ifdef CONFIG_OSAL_UDIF_BDEV_THREAD
	disk->queue->queuedata = dev;
	dev->th = kthread_run(udif_bdev_thread, disk->queue, "bdev_%s", dev->node->name);
	if (IS_ERR(dev->th)) {
		blk_cleanup_queue(disk->queue);
		del_gendisk(disk);
		return PTR_ERR(dev->th);
	}
#endif

	add_disk(disk);

	add_to_driver_list(dev);

	return UDIF_ERR_OK;
}

UDIF_ERR udif_bdev_del_disk(UDIF_BDEV *dev)
{
	struct gendisk *disk;
	struct request_queue *q;

	UDIF_PARM_CHK(!dev, "invalid dev", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!dev->disk, "invalid disk", UDIF_ERR_PAR);

	del_from_driver_list(dev);

	disk = dev->disk;
	q = disk->queue;

	del_gendisk(disk);
#ifdef CONFIG_OSAL_UDIF_BDEV_THREAD
	kthread_stop(dev->th);
	q->queuedata = 0;
	dev->th = 0;
#endif

	if (q)
		blk_cleanup_queue(q);
	put_disk(disk);

	dev->disk = NULL;

	return UDIF_ERR_OK;
}

UDIF_ERR udif_bdev_fill_buffer(UDIF_REQUEST *req, UDIF_U8 data, UDIF_UINT nsect, UDIF_UINT *xferred)
{
	struct request *rq;
	struct bio_vec *bvec;
	struct req_iterator iter;

	UDIF_PARM_CHK(!req, "invalid req", UDIF_ERR_PAR);
	UDIF_PARM_CHK(req->write, "invalid req->write", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!xferred, "invalid xferred", UDIF_ERR_PAR);

	rq = req->rq;
	*xferred = 0;
	rq_for_each_segment(bvec, rq, iter) {
		void *buffer = __bio_kmap_atomic(iter.bio, iter.i, KM_USER0);

#if 0
		/* should use noncache area because of cache invalidation as udif_bdev_unmap_rq after request call back */
		memset((void *)VA_TO_NONCACHE((unsigned long)buffer), data, bvec->bv_len);
#else
		memset(buffer, data, bvec->bv_len);
		dmac_flush_range(buffer, buffer + bvec->bv_len);
		outer_flush_range(__pa(buffer), __pa(buffer) + bvec->bv_len);
#endif
		__bio_kunmap_atomic(iter.bio, KM_USER0);

		*xferred += bvec->bv_len >> 9;
		if (*xferred >= nsect) {
			*xferred = nsect;
			break;
		}
	}

	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(udif_bdev_register);
EXPORT_SYMBOL(udif_bdev_unregister);
EXPORT_SYMBOL(__udif_bdev_add_disk);
EXPORT_SYMBOL(udif_bdev_del_disk);
EXPORT_SYMBOL(udif_bdev_fill_buffer);
EXPORT_SYMBOL(udif_bdev_map_rq);
EXPORT_SYMBOL(udif_bdev_unmap_rq);
