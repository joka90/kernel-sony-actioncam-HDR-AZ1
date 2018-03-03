/* 2012-02-16: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot Core - core interface
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/pfn.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/list.h>
#include <linux/parser.h>
#ifdef CONFIG_SNSC_PMMEM
#include <linux/snsc_pmmem.h>
#endif
#include "internal.h"

/* global settings */
ssboot_core_t ssboot = {
	.memmap		= {
		.num_region	= 0,
	},
	.imgmode	= SSBOOT_IMGMODE_MIN,
	.opmode		= SSBOOT_OPMODE_NORMAL,
	.resmode	= SSBOOT_RESMODE_NORMAL,
	.state		= SSBOOT_STATE_IDLE,
	.image		= {
		.target_id	= SSBOOT_TARGETID_UNKNOWN,
		.entry_addr	= 0,
		.num_section	= 0,
		.cmdline	= 0,
	},
	.writers	= LIST_HEAD_INIT(ssboot.writers),
	.optimizer	= NULL,
};

 /*
 * Cmdline parse operations
 */
enum {
	PARAM_RES_REWRITE,
	PARAM_RES_PROFILE,
	PARAM_RES_NORMAL,
	PARAM_IMG_MIN,
	PARAM_IMG_MAX,
	PARAM_IMG_OPTIMIZE,
	PARAM_ERR,
};

static const match_table_t cmdline_tokens = {
	{ PARAM_RES_REWRITE,	"ssboot_resmode=rewrite" },
	{ PARAM_RES_PROFILE,	"ssboot_resmode=profile" },
	{ PARAM_RES_NORMAL,	"ssboot_resmode=normal" },
	{ PARAM_IMG_MIN,	"ssboot_imgmode=min" },
	{ PARAM_IMG_MAX,	"ssboot_imgmode=max" },
	{ PARAM_IMG_OPTIMIZE,	"ssboot_imgmode=optimize" },
	{ PARAM_ERR,		NULL },
};

static int
parse_cmdline(void)
{
	substring_t args[MAX_OPT_ARGS];
	int token, ret = 0;
	char *p;
	char *param;

	param = kstrndup(ssboot.image.cmdline, SSBOOT_CMDLINE_SIZE, GFP_KERNEL);
	if (!param) {
		ret = -ENOMEM;
		goto out;
	}

	while ((p = strsep(&param, " ")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, cmdline_tokens, args);
		switch (token) {
		case PARAM_RES_REWRITE:
			ssboot.resmode = SSBOOT_RESMODE_REWRITE;
			break;
		case PARAM_RES_PROFILE:
			ssboot.resmode = SSBOOT_RESMODE_PROFILE;
			break;
		case PARAM_RES_NORMAL:
			ssboot.resmode = SSBOOT_RESMODE_NORMAL;
			break;
		case PARAM_IMG_MIN:
			ssboot.imgmode = SSBOOT_IMGMODE_MIN;
			break;
		case PARAM_IMG_MAX:
			ssboot.imgmode = SSBOOT_IMGMODE_MAX;
			break;
		case PARAM_IMG_OPTIMIZE:
			ssboot.imgmode = SSBOOT_IMGMODE_OPTIMIZE;
			break;
		default:
			break;
		}
	}

 out:
	kfree(param);
	return ret;
}

/*
 * Image writer operations
 */
static int
writer_prepare(void)
{
	int ret = 0;
	ssboot_writer_t *writer;

	/* check image writer */
	if (list_empty(&ssboot.writers)) {
		ssboot_err("image writer is not registered\n");
		return -ENODEV;
	}

	/* call prepare operation */
	list_for_each_entry(writer, &ssboot.writers, list) {
		if (writer->prepare == NULL)
			continue;

		ret = writer->prepare(writer->priv);
		if (ret < 0) {
			ssboot_err("failed to prepare image writer: %d\n", ret);
			break;
		}
	}
	return ret;
}

static int
writer_cleanup(void)
{
	int ret;
	int res = 0;
	ssboot_writer_t *writer;

	/* call cleanup operation */
	list_for_each_entry(writer, &ssboot.writers, list) {
		if (writer->cleanup == NULL)
			continue;

		ret = writer->cleanup(writer->priv);
		if (ret < 0) {
			ssboot_err("failed to cleanup image writer: %d\n", ret);
			res = ret;
		}
	}
	return res;
}

static int
writer_write(void)
{
	int ret = 0;
	ssboot_writer_t *writer;

	/* call write operation */
	list_for_each_entry(writer, &ssboot.writers, list) {
		ret = writer->write(&ssboot.image, writer->priv);
		if (ret < 0) {
			ssboot_err("failed to write image: %d\n", ret);
			break;
		}
	}
	return ret;
}

/*
 * Functions called from PM
 */
int
ssboot_prepare(void)
{
	int ret;

	/* prepare image writer */
	ret = writer_prepare();
	if (ret < 0) {
		return ret;
	}

	/* allocate cmdline info area */
	if (ssboot.image.cmdline == NULL) {
		ssboot.image.cmdline = kzalloc(SSBOOT_CMDLINE_SIZE, GFP_KERNEL);
		if (ssboot.image.cmdline == NULL) {
			ret = -ENOMEM;
			goto clean;
		}
	}

	/* allocate page bitmaps */
	ret = ssboot_alloc_page_bitmap();
	if (ret < 0) {
		goto clean;
	}

	/* enable swapfile */
	if (!ssboot_is_rewriting()) {
		ret = ssboot_swapon();
		if (ret < 0) {
			goto free;
		}
	}

	/* shrink image by freeing memory */
	ret = ssboot_shrink_image();
	if (ret < 0) {
		goto swapoff;
	}

	/* freeze swapfile */
	if (!ssboot_is_rewriting()) {
		ret = ssboot_swap_set_ro();
		if (ret < 0) {
			goto swapoff;
		}
	}

	/* wait for I/O completion */
	ret = ssboot_wait_io_completion();
	if (ret < 0) {
		goto swapoff;
	}

	/* set current state */
	ssboot.state = SSBOOT_STATE_PREPARE;

	return 0;

 swapoff:
	/* disable swapfile */
	ssboot_swapoff();
 free:
	/* free page bitmaps */
	ssboot_free_page_bitmap();
 clean:
	/* cleanup image writer */
	writer_cleanup();

	return ret;
}

int
ssboot_snapshot(void *entry_addr)
{
	int ret;
	ssboot_resmode_t orig_resmode = ssboot.resmode;

	/* check current state */
	if (ssboot.state != SSBOOT_STATE_PREPARE) {
		ret = -EPERM;
		goto out;
	}

	/* set current state for resume from image */
	ssboot.state = SSBOOT_STATE_SNAPSHOT;

	/* set resume mode for resume from image */
	ssboot.resmode = SSBOOT_RESMODE_NORMAL;

	/* create page bitmap */
	ret = ssboot_create_page_bitmap();
	if (ret < 0) {
		goto out;
	}

	/* copy critical pages */
	ret = ssboot_copy_critical_pages();
	if (ret < 0) {
		goto out;
	}

	/* lock page cache */
	ssboot_lock_page_cache();

	/* set entry address */
	ssboot.image.entry_addr = entry_addr;

 out:
	/* restore resume mode */
	ssboot.resmode = orig_resmode;

	/* set current state */
	if (ret < 0) {
		/* enter error state */
		ssboot.state = SSBOOT_STATE_ERROR;
	} else {
		/* for image creation */
		ssboot.state = SSBOOT_STATE_WRITING;
	}
	return ret;
}

int
ssboot_write(void)
{
	int ret;

	/* create section list */
	ret = ssboot_create_section_list();
	if (ret < 0) {
		ssboot_unlock_page_cache();
		goto out;
	}

	/* show page statistics */
	ssboot_pgstat_show();

	/* show image info */
	ssboot_show_image_info();

	/* write image */
	if (ssboot.imgmode != SSBOOT_IMGMODE_NULL) {
		ret = writer_write();
	}

	/* unlock page cache */
	ssboot_unlock_page_cache();

	/* free section list */
	ssboot_free_section_list();

 out:
	/* enter error state if failed */
	if (ret < 0) {
		ssboot.state = SSBOOT_STATE_ERROR;
	}
	return ret;
}

int
ssboot_finish(void)
{
	int err, ret = 0;

	/* check current state */
	if (ssboot.state == SSBOOT_STATE_PREPARE) {
		ssboot_err("image creation is not supported\n");
		ret = -ENOSYS;
	} else {
		/* free copied critical pages */
		ssboot_free_copied_pages();
	}

	/* free page bitmaps */
	ssboot_free_page_bitmap();

	/* cleanup image writer */
	err = writer_cleanup();
	if (err < 0) {
		ret = (ret < 0) ? ret : err;
		goto out;
	}

	/* switch operation after image creation */
	if (ssboot.state == SSBOOT_STATE_WRITING) {
		switch (ssboot.opmode) {
		case SSBOOT_OPMODE_SHUTDOWN:
			kernel_power_off();
			break;
		case SSBOOT_OPMODE_REBOOT:
			kernel_restart(NULL);
			break;
		case SSBOOT_OPMODE_NORMAL:
		default:
			break;
		}
	}

	/* parse snapshot cmdline information */
	ret = parse_cmdline();
	if (ret < 0) {
		goto out;
	}

	/* start profiling */
	if (ssboot.state == SSBOOT_STATE_SNAPSHOT &&
	    ssboot.resmode == SSBOOT_RESMODE_PROFILE) {
		ssboot_dbg("start profiling\n");
		ret = ssboot_optimizer_start_profiling();
		if (ret < 0)
			goto out;
	}

#ifdef CONFIG_SNSC_PMMEM
	if (ssboot_is_resumed() &&
	    ssboot.resmode == SSBOOT_RESMODE_NORMAL) {
		pmmem_free();
	}
#endif
 out:
	/* set current state */
	if (ssboot.state == SSBOOT_STATE_SNAPSHOT) {
		/* resumed from image */
		ssboot.state = SSBOOT_STATE_RESUMED;
	} else {
		/* after image creation */
		ssboot.state = SSBOOT_STATE_IDLE;
	}
	return ret;
}

/*
 * Exported functions
 */
int
ssboot_is_writing(void)
{
	/* return true during image writing */
	return (ssboot.state == SSBOOT_STATE_PREPARE ||
		ssboot.state == SSBOOT_STATE_WRITING);
}
EXPORT_SYMBOL(ssboot_is_writing);

int
ssboot_is_resumed(void)
{
	/* return true if kernel resumed from image */
	return (ssboot.state == SSBOOT_STATE_SNAPSHOT ||
		ssboot.state == SSBOOT_STATE_RESUMED);
}
EXPORT_SYMBOL(ssboot_is_resumed);

int
ssboot_is_profiling(void)
{
	return (ssboot_is_resumed() &&
		ssboot.resmode == SSBOOT_RESMODE_PROFILE);
}
EXPORT_SYMBOL(ssboot_is_profiling);

int
ssboot_is_rewriting(void)
{
	return (ssboot_is_resumed() &&
		ssboot.resmode == SSBOOT_RESMODE_REWRITE);
}
EXPORT_SYMBOL(ssboot_is_rewriting);

void
ssboot_set_target_id(u_int16_t target_id)
{
	/* set target id to core */
	ssboot.image.target_id = target_id;
}
EXPORT_SYMBOL(ssboot_set_target_id);

int
ssboot_invalidate_page_cache(const char *filename)
{
	struct file *filp = NULL;
	struct block_device *bdev;
	unsigned long num;

	/* sanity check */
	if (filename == NULL) {
		return -EINVAL;
	}

	/* check current state */
	if (ssboot.state == SSBOOT_STATE_WRITING) {
		return -EPERM;
	}

	/* write back all dirty page caches */
	sys_sync();

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		return PTR_ERR(filp);
	}

	/* invalidate page caches associated with file */
	if (filp->f_mapping != NULL) {
		num = invalidate_mapping_pages(filp->f_mapping, 0, -1);
		ssboot_dbg("invalidate page cache (%ld pages)\n", num);
	}

	/* invalidate block device buffers */
	bdev = filp->f_path.dentry->d_inode->i_sb->s_bdev;
	if (bdev != NULL) {
		fsync_bdev(bdev);
		invalidate_bdev(bdev);
		ssboot_dbg("invalidate block device buffers\n");
	}

	filp_close(filp, current->files);

	return 0;
}
EXPORT_SYMBOL(ssboot_invalidate_page_cache);

int
ssboot_writer_register(ssboot_writer_t *writer)
{
	/* sanity check */
	if (writer == NULL || writer->write == NULL) {
		ssboot_err("cannot register invalid writer: %p\n", writer);
		return -EINVAL;
	}

	/* register to list */
	list_add_tail(&writer->list, &ssboot.writers);

	return 0;
}
EXPORT_SYMBOL(ssboot_writer_register);

int
ssboot_writer_unregister(ssboot_writer_t *writer)
{
	ssboot_writer_t *wptr;

	/* sanity check */
	if (writer == NULL) {
		ssboot_err("cannot unregister invalid writer: %p\n", writer);
		return -EINVAL;
	}

	/* check if writer is already unregistered */
	list_for_each_entry(wptr, &ssboot.writers, list) {
		if (wptr == writer)
			break;
	}
	if (wptr != writer) {
		ssboot_err("cannot unregister unknown writer: %p\n", writer);
		return -ENOENT;
	}

	/* unregister from list */
	list_del(&writer->list);

	return 0;
}
EXPORT_SYMBOL(ssboot_writer_unregister);

int
ssboot_optimizer_register(ssboot_optimizer_t *optimizer)
{
	/* sanity check */
	if (optimizer == NULL) {
		ssboot_err("cannot register invalid optimizer: %p\n",
			   optimizer);
		return -EINVAL;
	}

	/* check if optimizer is already registered */
	if (ssboot.optimizer != NULL) {
		ssboot_err("optimizer is already registered\n");
		return -EEXIST;
	}

	/* register optimizer */
	ssboot.optimizer = optimizer;

	return 0;
}
EXPORT_SYMBOL(ssboot_optimizer_register);

int
ssboot_optimizer_unregister(ssboot_optimizer_t *optimizer)
{
	/* sanity check */
	if (optimizer == NULL) {
		ssboot_err("cannot unregister invalid optimizer: %p\n",
			   optimizer);
		return -EINVAL;
	}

	/* check if optimizer is already unregistered */
	if (ssboot.optimizer == NULL) {
		ssboot_err("optimizer is already unregistered\n");
		return -ENOENT;
	}

	/* check unknown optimizer */
	if (ssboot.optimizer != optimizer) {
		ssboot_err("cannot unregister unknown optimizer: %p\n",
			   optimizer);
		return -ENOENT;
	}

	/* unregister optimizer */
	ssboot.optimizer = NULL;

	return 0;
}
EXPORT_SYMBOL(ssboot_optimizer_unregister);

static int
region_register(int attr, u_int64_t phys_addr, size_t len)
{
	/* check alignment */
	if ((phys_addr | len) & ~PAGE_MASK) {
		ssboot_err("cannot register region (invalid align): "
			   "0x%08llx-0x%08llx\n",
			   phys_addr, phys_addr + len - 1);
		return -EINVAL;
	}

	/* register region */
	return ssboot_exreg_register(SSBOOT_EXREG_KERNEL | attr,
				     PFN_DOWN(phys_addr),
				     PFN_DOWN(len));
}

int
ssboot_region_register_normal(u_int64_t phys_addr, size_t len)
{
	return region_register(SSBOOT_EXREG_NORMAL, phys_addr, len);
}
EXPORT_SYMBOL(ssboot_region_register_normal);

int
ssboot_region_register_critical(u_int64_t phys_addr, size_t len)
{
	return region_register(SSBOOT_EXREG_CRITICAL, phys_addr, len);
}
EXPORT_SYMBOL(ssboot_region_register_critical);

int
ssboot_region_register_discard(u_int64_t phys_addr, size_t len)
{
	return region_register(SSBOOT_EXREG_DISCARD, phys_addr, len);
}
EXPORT_SYMBOL(ssboot_region_register_discard);

int
ssboot_region_register_work(u_int64_t phys_addr, size_t len)
{
	return region_register(SSBOOT_EXREG_WORK, phys_addr, len);
}
EXPORT_SYMBOL(ssboot_region_register_work);

int
ssboot_region_unregister(u_int64_t phys_addr, size_t len)
{
	/* check alignment */
	if ((phys_addr | len) & ~PAGE_MASK) {
		ssboot_err("cannot unregister region (invalid align): "
			   "0x%08llx-0x%08llx\n",
			   phys_addr, phys_addr + len - 1);
	}

	/* unregister region */
	return ssboot_exreg_unregister(SSBOOT_EXREG_KERNEL,
				       PFN_DOWN(phys_addr),
				       PFN_DOWN(len));
}
EXPORT_SYMBOL(ssboot_region_unregister);

/*
 * Function called from arch/arm/common/warm.c arch/arm/mach_cxd90014/pm.c
 */
void
ssboot_setmode(unsigned long stat)
{
	switch (stat) {
	case SSBOOT_CREATE_MINSSBI:
		ssboot.state   = SSBOOT_STATE_PREPARE;
		ssboot.resmode = SSBOOT_RESMODE_NORMAL;
		ssboot.imgmode = SSBOOT_IMGMODE_MIN;
		break;
	case SSBOOT_PROFILE:
		ssboot.state   = SSBOOT_STATE_SNAPSHOT;
		ssboot.resmode = SSBOOT_RESMODE_PROFILE;
		ssboot.imgmode = SSBOOT_IMGMODE_OPTIMIZE;
		break;
	case SSBOOT_CREATE_OPTSSBI:
		ssboot.state   = SSBOOT_STATE_SNAPSHOT;
		ssboot.resmode = SSBOOT_RESMODE_REWRITE;
		ssboot.imgmode = SSBOOT_IMGMODE_OPTIMIZE;
		break;
	case SSBOOT_SSBOOT_PRE:
	case SSBOOT_SSBOOT:
		ssboot.state   = SSBOOT_STATE_SNAPSHOT;
		ssboot.resmode = SSBOOT_RESMODE_NORMAL;
		ssboot.imgmode = SSBOOT_IMGMODE_OPTIMIZE;
		break;
	default:
		ssboot.state   = SSBOOT_STATE_IDLE;
		ssboot.resmode = SSBOOT_RESMODE_NORMAL;
		ssboot.imgmode = SSBOOT_IMGMODE_NULL;
		break;
	}
}

/*
 * Initialization
 */
void __init
ssboot_init(void)
{
	ssboot_memmap_init();
}
