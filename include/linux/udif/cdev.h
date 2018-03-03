/*
 * include/linux/udif/cdev.h
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
#ifndef __UDIF_CDEV_H__
#define __UDIF_CDEV_H__

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/udif/string.h>
#include <linux/udif/types.h>
#include <linux/udif/file.h>
#include <linux/udif/mmap.h>
#include <linux/udif/poll.h>
#include <linux/udif/devnode.h>

typedef struct UDIF_CDEV_READ {
	UDIF_USR_RW	uaddr;
	UDIF_SIZE	count;
	UDIF_LOFF	*pos;
} UDIF_CDEV_READ;

typedef struct UDIF_CDEV_WRITE {
	UDIF_USR_RO	uaddr;
	UDIF_SIZE	count;
	UDIF_LOFF	*pos;
} UDIF_CDEV_WRITE;

typedef enum {
	UDIF_SEEK_SET = 0,
	UDIF_SEEK_CUR = 1,
	UDIF_SEEK_END = 2,
} UDIF_WHENCE;

typedef struct UDIF_CDEV_LSEEK {
	UDIF_WHENCE	whence;
	UDIF_LOFF	offset;
	UDIF_LOFF	*pos;
} UDIF_CDEV_LSEEK;

typedef	UDIF_SSIZE (*UDIF_CDEV_READ_CB)(UDIF_FILE *, UDIF_CDEV_READ *);
typedef	UDIF_SSIZE (*UDIF_CDEV_WRITE_CB)(UDIF_FILE *, UDIF_CDEV_WRITE *);
typedef	UDIF_LOFF  (*UDIF_CDEV_LSEEK_CB)(UDIF_FILE *, UDIF_CDEV_LSEEK *);
typedef	UDIF_ERR   (*UDIF_CDEV_MMAP_CB)(UDIF_FILE *, UDIF_VMAREA *);
typedef	UDIF_UINT  (*UDIF_CDEV_POLL_CB)(UDIF_FILE *, UDIF_POLL *);

typedef struct UDIF_CDEV_OPS {
	UDIF_FILE_CB		open;
	UDIF_FILE_CB		close;
	UDIF_IOCTL_CB		ioctl;
	UDIF_CDEV_READ_CB	read;
	UDIF_CDEV_WRITE_CB	write;
	UDIF_CDEV_LSEEK_CB	lseek;
	UDIF_CDEV_MMAP_CB	mmap;
	UDIF_CDEV_POLL_CB	poll;
} UDIF_CDEV_OPS;

typedef struct UDIF_CDEV {
	const UDIF_DEVNODE *node;
	UDIF_CDEV_OPS	*ops;
	UDIF_VP		data;
	/* internal use */
	struct cdev	cdev;
	struct module	*owner;
} UDIF_CDEV;

#define UDIF_DECLARE_CDEV(dev, nd, op, dt) \
UDIF_CDEV dev = { \
	.owner	= THIS_MODULE, \
	.node	= nd, \
	.ops	= op, \
	.data	= dt, \
}

static inline void udif_cdev_init(UDIF_CDEV *dev, UDIF_DEVNODE *node, UDIF_CDEV_OPS *ops, UDIF_VP data)
{
	if (unlikely(!dev)) {
		return;
	}

	dev->owner = THIS_MODULE;
	dev->node = node;
	dev->ops = ops;
	dev->data = data;
}

UDIF_ERR udif_cdev_register(UDIF_CDEV *);
UDIF_ERR udif_cdev_unregister(UDIF_CDEV *);

#endif /* __UDIF_CDEV_H__ */
