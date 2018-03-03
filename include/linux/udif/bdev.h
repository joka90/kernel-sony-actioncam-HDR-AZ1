/*
 * include/linux/udif/bdev.h
 *
 *
 * Copyright 2010 Sony Corporation
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
#ifndef __UDIF_BDEV_H__
#define __UDIF_BDEV_H__

#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/genhd.h>
#include <linux/udif/types.h>
#include <linux/udif/file.h>
#include <linux/udif/devnode.h>
#include <linux/udif/sg.h>

#ifdef CONFIG_BLK_DEV_DECOMPRESS
typedef struct disk_decompress UDIF_DISK_DECOMPRESS;
#endif

#define UDIF_BDEV_ALGO_LZO	GENHD_ALGO_LZO
#define UDIF_BDEV_ALGO_LZ77	GENHD_ALGO_LZ77

#define UDIF_MAX_PHYS_SEGMENTS 128

typedef struct UDIF_REQUEST {
	UDIF_UINT	write;
	UDIF_SECTOR	sector;
	UDIF_UINT	nr_sectors;
	UDIF_SG		*sg;
	UDIF_UINT	sg_len;
	UDIF_U8		*buffer;
	UDIF_VP		rq;	/* struct request* */
	UDIF_VP		que;	/* struct request_queue* */
#ifdef CONFIG_BLK_DEV_DECOMPRESS
	UDIF_DISK_DECOMPRESS *dec;
#endif
} UDIF_REQUEST;

typedef struct UDIF_GEOMETRY {
      UDIF_U8		heads;
      UDIF_U8		sectors;
      UDIF_U16		cylinders;
      UDIF_ULONG	start;
} UDIF_GEOMETRY;

#define UDIF_GEOMETRY_START_DEFAULT (-1UL)

typedef	UDIF_ERR (*UDIF_REQUEST_CB)(UDIF_FILE *, UDIF_REQUEST *, UDIF_UINT *);
typedef	UDIF_ERR (*UDIF_MEDIA_CHANGED_CB)(UDIF_FILE *);
typedef	UDIF_ERR (*UDIF_GETGEO_CB)(UDIF_FILE *, UDIF_GEOMETRY *);
typedef	UDIF_ERR (*UDIF_REVALIDATE_CB)(UDIF_FILE *, UDIF_SECTOR *);

typedef struct UDIF_BDEV_OPS {
	const UDIF_FILE_CB		open;
	const UDIF_FILE_CB		close;
	const UDIF_IOCTL_CB		ioctl;
	const UDIF_REQUEST_CB		request;
	const UDIF_MEDIA_CHANGED_CB	media_changed;
	const UDIF_GETGEO_CB		getgeo;
	const UDIF_REVALIDATE_CB	revalidate;
} UDIF_BDEV_OPS;

typedef struct UDIF_BDEV {
	/* input from user */
	const UDIF_DEVNODE	*node;
	UDIF_BDEV_OPS		*ops;

	UDIF_SG			*sg;

	UDIF_SIZE		sector_size;
	UDIF_SECTOR		max_sectors;
	UDIF_UINT		max_hw_segments;
	UDIF_UINT		phys_segments;
	UDIF_SIZE		max_segment_size;

	UDIF_SECTOR		capacity;
	UDIF_UINT		dma_alignment;

	UDIF_VP			data;
#define UDIF_BDEV_FLAGS_XMAPSG 0x1
	UDIF_UINT		flags;
#ifdef CONFIG_BLK_DEV_DECOMPRESS
	UDIF_ULONG		algo;
#endif

	/* internal use */
	spinlock_t 		lock;
	UDIF_VP			disk;	/* struct gendisk */
	struct module		*owner;
#ifdef CONFIG_OSAL_UDIF_BDEV_THREAD
	struct task_struct	*th;
#endif
} UDIF_BDEV;

UDIF_ERR udif_bdev_register(const UDIF_DEVNODE *);
void udif_bdev_unregister(const UDIF_DEVNODE *);
UDIF_ERR __udif_bdev_add_disk(UDIF_BDEV *);
UDIF_ERR udif_bdev_del_disk(UDIF_BDEV *);
UDIF_ERR udif_bdev_fill_buffer(UDIF_REQUEST *, UDIF_U8, UDIF_UINT, UDIF_UINT *);
void udif_bdev_map_rq(UDIF_REQUEST *);
void udif_bdev_unmap_rq(UDIF_REQUEST *);

static inline UDIF_ERR udif_bdev_add_disk(UDIF_BDEV *dev)
{
	if (unlikely(!dev)) {
		return UDIF_ERR_PAR;
	}

	dev->owner = THIS_MODULE;
	return __udif_bdev_add_disk(dev);
}

#endif /* __UDIF_BDEV_H__ */
