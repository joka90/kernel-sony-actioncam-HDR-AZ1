/* 2012-08-29: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot Core - proc interface
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
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/freezer.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include "internal.h"

/* iterator for 'section' */
typedef struct ssboot_sect_iter {
	loff_t           pos;
	loff_t           num;
	ssboot_section_t *sect;
	unsigned long    norm;
	unsigned long    crit;
} ssboot_sect_iter_t;

/* iterator for extra regions */
typedef struct ssboot_region_iter {
	loff_t        pos;
	unsigned long pfn;
	unsigned long num;
	unsigned long attr;
} ssboot_region_iter_t;

/* semaphore for proc interface access */
static DEFINE_SEMAPHORE(ssboot_proc_sem);

/* proc entry of 'ssboot' directory */
struct proc_dir_entry *ssboot_proc_root;
EXPORT_SYMBOL(ssboot_proc_root);

/* proc entry of 'ssboot/optimizer' directory */
struct proc_dir_entry *ssboot_proc_optimizer;
EXPORT_SYMBOL(ssboot_proc_optimizer);

/*
 * Wrapper functions for customized proc operations
 */
static int
ssboot_proc_open(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = (ssboot_proc_ops_t *)PDE(inode)->data;
	int ret;

	/* acquire semaphore */
	if (down_interruptible(&ssboot_proc_sem)) {
		return -ERESTARTSYS;
	}

	/* check if already opened */
	if (ops->opened) {
		ret = -EBUSY;
		goto out;
	}
	ops->opened = 1;

	/* call seq_file layer */
	ret = seq_open(file, &ops->seq_ops);
	if (ret < 0) {
		goto out;
	}
	((struct seq_file *)file->private_data)->private = (void *)ops;

	/* allocate write buffer */
	if (ops->write_max > 0) {
		ops->write_buf = kmalloc(ops->write_max, GFP_KERNEL);
		if (ops->write_buf == NULL) {
			ret = -ENOMEM;
			goto close;
		}
	}

	/* call open method */
	if (ops->open != NULL) {
		ret = ops->open(inode, file);
	}
 close:
	if (ret < 0) {
		seq_release(inode, file);
	}
 out:
	up(&ssboot_proc_sem);
	return ret;
}

static int
ssboot_proc_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	int ret;

	/* acquire semaphore */
	if (down_interruptible(&ssboot_proc_sem)) {
		return -ERESTARTSYS;
	}

	/* call release method */
	if (ops->release != NULL) {
		ops->release(inode, file);
	}

	/* free write buffer */
	if (ops->write_max > 0) {
		kfree(ops->write_buf);
		ops->write_buf = NULL;
	}
	ops->write_len = 0;

	/* clear open flag */
	ops->opened = 0;

	/* call seq_file layer */
	ret = seq_release(inode, file);

	up(&ssboot_proc_sem);
	return ret;
}

static ssize_t
ssboot_proc_write(struct file *file, const char __user *buffer, size_t len,
		  loff_t *offset)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	ssize_t ret = len;
	char *buf;

	/* acquire semaphore */
	if (down_interruptible(&ssboot_proc_sem)) {
		return -ERESTARTSYS;
	}

	/* sanity check */
	if (ops->write_buf == NULL) {
		ret = -EINVAL;
		goto out;
	}

	/* check offset and size */
	if (*offset + len >= ops->write_max) {
		ret = -EINVAL;
		goto out;
	}
	if (len == 0) {
		ret = 0;
		goto out;
	}

	/* get data from user */
	buf = ops->write_buf + *offset;
	if (copy_from_user(buf, buffer, len)) {
		ret = -EFAULT;
		goto out;
	}
	buf[len] = '\0';
	if (buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
		len--;
	}

	/* update offset and written size */
	*offset += len;
	if (ops->write_len < *offset) {
		ops->write_len = *offset;
	}
 out:
	up(&ssboot_proc_sem);
	return ret;
}

static void *
ssboot_proc_single_start(struct seq_file *m, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *
ssboot_proc_single_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void
ssboot_proc_stop(struct seq_file *m, void *v)
{
	return;
}

static struct file_operations wrapper_ops = {
	.open		= ssboot_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= ssboot_proc_write,
	.release	= ssboot_proc_release,
};

/*
 * mode
 */
static const char * const imgmode_str[SSBOOT_IMGMODE_NUM] = {
	[SSBOOT_IMGMODE_NULL]     = "null",
	[SSBOOT_IMGMODE_MIN]      = "min",
	[SSBOOT_IMGMODE_MAX]      = "max",
	[SSBOOT_IMGMODE_OPTIMIZE] = "optimize",
};

static int
mode_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
mode_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", imgmode_str[ssboot.imgmode]);

	return 0;
}

static int
mode_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	ssboot_imgmode_t mode;
	const char *str;

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* parse string */
	for (mode = 0; mode < SSBOOT_IMGMODE_NUM; mode++) {
		str = imgmode_str[mode];
		if (strcmp(ops->write_buf, str) == 0) {
			break;
		}
	}
	if (mode == SSBOOT_IMGMODE_NUM) {
		ssboot_err("invalid image creation mode\n");
		return 0;
	}

	/* set image creation mode */
	ssboot.imgmode = mode;

	return 0;
}
ssboot_single_proc(mode, SSBOOT_PROC_RDWR, 16);

/*
 * resmode
 */
static const char * const resmode_str[SSBOOT_RESMODE_NUM] = {
	[SSBOOT_RESMODE_NORMAL]  = "normal",
	[SSBOOT_RESMODE_PROFILE] = "profile",
	[SSBOOT_RESMODE_REWRITE] = "rewrite",
};

static int
resmode_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
resmode_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", resmode_str[ssboot.resmode]);

	return 0;
}

static int
resmode_release(struct inode *inode, struct file *file)
{
	return 0;
}
ssboot_single_proc(resmode, SSBOOT_PROC_RDONLY, 16);

/*
 * operation
 */
static const char * const opmode_str[SSBOOT_OPMODE_NUM] = {
	[SSBOOT_OPMODE_NORMAL]   = "normal",
	[SSBOOT_OPMODE_SHUTDOWN] = "shutdown",
	[SSBOOT_OPMODE_REBOOT]   = "reboot",
};

static int
operation_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
operation_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", opmode_str[ssboot.opmode]);

	return 0;
}

static int
operation_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	ssboot_opmode_t mode;
	const char *str;

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* parse string */
	for (mode = 0; mode < SSBOOT_OPMODE_NUM; mode++) {
		str = opmode_str[mode];
		if (strcmp(ops->write_buf, str) == 0) {
			break;
		}
	}
	if (mode == SSBOOT_OPMODE_NUM) {
		ssboot_err("invalid operation mode\n");
		return 0;
	}

	/* set operation mode */
	ssboot.opmode = mode;

	return 0;
}
ssboot_single_proc(operation, SSBOOT_PROC_RDWR, 16);

/*
 * section
 */
static int
section_open(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	int ret;

	/* stop user processes */
	ret = freeze_tasks(FREEZER_USER_SPACE);
	if (ret < 0) {
		return ret;
	}

	/* allocate page bitmaps */
	ret = ssboot_alloc_page_bitmap();
	if (ret < 0) {
		goto thaw;
	}

	/* create page bitmap to estimate sections */
	ret = ssboot_create_page_bitmap();
	if (ret < 0) {
		goto free_pgbmp;
	}

	/* shrink image by freeing memory */
	ret = ssboot_shrink_image();
	if (ret < 0) {
		goto free_pgbmp;
	}

	/* stop kernel threads */
	ret = freeze_tasks(FREEZER_KERNEL_THREADS);
	if (ret < 0) {
		goto free_pgbmp;
	}

	/* create page bitmap with irqs disabled */
	local_irq_disable();
	ret = ssboot_create_page_bitmap();
	local_irq_enable();
	if (ret < 0) {
		goto free_pgbmp;
	}

	/* create section list */
	ret = ssboot_create_section_list();
	if (ret < 0) {
		goto free_pgbmp;
	}

	/* allocate iterator */
	ops->data = kmalloc(sizeof(ssboot_sect_iter_t), GFP_KERNEL);
	if (ops->data == NULL) {
		ret = -ENOMEM;
		goto free_list;
	}

	/* thaw processes */
	thaw_processes();
	return 0;

 free_list:
	ssboot_free_section_list();
 free_pgbmp:
	ssboot_free_page_bitmap();
 thaw:
	thaw_processes();
	return ret;
}

static void*
section_start(struct seq_file *m, loff_t *pos)
{
	ssboot_image_t *image = &ssboot.image;
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_sect_iter_t *iter = (ssboot_sect_iter_t *)ops->data;

	/* pos = 0 is for header */
	if (*pos == 0) {
		iter->pos = 0;
		return (void *)pos;
	}

	/* check end of list */
	if (iter->pos == -1UL) {
		return NULL;
	}

	/* get section info */
	if (*pos > image->num_section) {
		iter->num = iter->pos;
		iter->pos = -1UL;
	} else {
		iter->sect = &image->section[*pos - 1];
		iter->pos = *pos;
	}
	return (void *)pos;
}

static void*
section_next(struct seq_file *m, void *v, loff_t *pos)
{
	ssboot_image_t *image = &ssboot.image;
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_sect_iter_t *iter = (ssboot_sect_iter_t *)ops->data;

	++*pos;

	/* check end of list */
	if (iter->pos == -1UL) {
		return NULL;
	}

	/* get section info */
	if (*pos > image->num_section) {
		iter->num = iter->pos;
		iter->pos = -1UL;
	} else {
		iter->sect = &image->section[*pos - 1];
		iter->pos = *pos;
	}
	return (void *)pos;
}

static int
section_show(struct seq_file *m, void *v)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_sect_iter_t *iter = (ssboot_sect_iter_t *)ops->data;
	unsigned long pfn, num, total;

	/* print header */
	if (iter->pos == 0) {
		seq_printf(m, "Section  Physical Address          "
			      "KByte   Pages  Attribute\n");
		iter->num = 0;
		iter->norm = 0;
		iter->crit = 0;
		return 0;
	}

	/* print footer */
	if (iter->pos == -1UL) {
		total = iter->norm + iter->crit;
		seq_printf(m, "\n");
		seq_printf(m, "Section  : %5lld sections\n", iter->num);
		seq_printf(m, "Critical : %5ld pages (%3ldMB)\n",
			   iter->crit, iter->crit / SSBOOT_PG_1MB);
		seq_printf(m, "Normal   : %5ld pages (%3ldMB)\n",
			   iter->norm, iter->norm / SSBOOT_PG_1MB);
		seq_printf(m, "Total    : %5ld pages (%3ldMB)\n",
			   total, total / SSBOOT_PG_1MB);
		return 0;
	}

	/* print body */
	pfn = iter->sect->start_pfn;
	num = iter->sect->num_pages;
	seq_printf(m, "#%06lld  0x%08llx - 0x%08llx  %6ld  %6ld  ",
		   iter->pos, (u_int64_t)PFN_PHYS(pfn),
		   (u_int64_t)PFN_PHYS(pfn + num) - 1,
		   (unsigned long)PFN_PHYS(num) / SSBOOT_SZ_1KB, num);
	if (iter->sect->attr & SSBOOT_SECTION_CRITICAL) {
		seq_printf(m, "Critical\n");
		iter->crit += num;
	} else {
		seq_printf(m, "Normal\n");
		iter->norm += num;
	}
	return 0;
}

static int
section_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* free section list */
	ssboot_free_section_list();

	/* free page bitmaps */
	ssboot_free_page_bitmap();

	/* free iterator */
	kfree(ops->data);

	return 0;
}
ssboot_seq_proc(section, SSBOOT_PROC_RDONLY, 0);

/*
 * state
 */
static const char * const state_str[SSBOOT_STATE_NUM] = {
	[SSBOOT_STATE_IDLE]     = "idle",
	[SSBOOT_STATE_PREPARE]  = "prepare",
	[SSBOOT_STATE_SNAPSHOT] = "snapshot",
	[SSBOOT_STATE_WRITING]  = "writing",
	[SSBOOT_STATE_RESUMED]  = "resumed",
	[SSBOOT_STATE_ERROR]    = "error",
};

static int
state_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
state_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", state_str[ssboot.state]);

	return 0;
}

static int
state_release(struct inode *inode, struct file *file)
{
	return 0;
}
ssboot_single_proc(state, SSBOOT_PROC_RDONLY, 0);

/*
 * swapfile
 */
static int
swapfile_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
swapfile_show(struct seq_file *m, void *v)
{
	const char *str = ssboot_get_swapfile();

	seq_printf(m, "%s\n", str ? str : "null");
	return 0;
}

static int
swapfile_release(struct inode *inode, struct file *file)
{
	char *str;
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* set swapfile */
	if (!strcmp(ops->write_buf, "null"))
		str = NULL;
	else
		str = ops->write_buf;

	ssboot_set_swapfile(str);

	return 0;
}
ssboot_single_proc(swapfile, SSBOOT_PROC_RDWR, SSBOOT_PATH_MAX);

/*
 * optimizer
 */
static int
optimizer_operation_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
optimizer_operation_show(struct seq_file *m, void *v)
{
	if (ssboot_optimizer_is_available()) {
		seq_printf(m, "%s\n", ssboot_optimizer_is_profiling() ?
			   "profiling" : "stopped");
	} else {
		seq_printf(m, "N/A\n");
	}

	return 0;
}

static int
optimizer_operation_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* check if data exists */
	if (ops->write_len == 0 || !ssboot_optimizer_is_available()) {
		return 0;
	}

	/* set optimizer */
	if (!strcmp(ops->write_buf, "start")) {
		ssboot_optimizer_start_profiling();
	} else if (!strcmp(ops->write_buf, "stop")) {
		ssboot_optimizer_stop_profiling();
	} else {
		ssboot_err("invalid optimizer operation\n");
	}

	return 0;
}
ssboot_single_proc(optimizer_operation, SSBOOT_PROC_RDWR, 16);

/*
 * region
 */
static int
region_open(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* allocate iterator */
	ops->data = kmalloc(sizeof(ssboot_region_iter_t), GFP_KERNEL);
	if (ops->data == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static void*
region_start(struct seq_file *m, loff_t *pos, unsigned long attr)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_region_iter_t *iter = (ssboot_region_iter_t *)ops->data;
	unsigned long pfn, num;
	loff_t i;

	/* pos = 0 is for header */
	if (*pos == 0) {
		iter->pos = 0;
		return (void *)pos;
	}

	/* get first extra region */
	iter->attr = ssboot_exreg_find_first(attr, &pfn, &num);
	if (pfn == SSBOOT_PFN_NONE) {
		return NULL;
	}

	/* seek for specified position */
	for (i = 1; i < *pos; i++) {
		iter->attr = ssboot_exreg_find_next(attr, &pfn, &num);
		if (pfn == SSBOOT_PFN_NONE) {
			return NULL;
		}
	}
	iter->pfn = pfn;
	iter->num = num;
	iter->pos = *pos;

	return (void *)pos;
}

static void*
region_next(struct seq_file *m, void *v, loff_t *pos, unsigned long attr)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_region_iter_t *iter = (ssboot_region_iter_t *)ops->data;
	unsigned long pfn, num;

	++*pos;

	if (*pos == 1) {
		/* get first extra region */
		iter->attr = ssboot_exreg_find_first(attr, &pfn, &num);
	} else {
		/* get next extra region */
		iter->attr = ssboot_exreg_find_next(attr, &pfn, &num);
	}
	if (pfn == SSBOOT_PFN_NONE) {
		return NULL;
	}
	iter->pfn = pfn;
	iter->num = num;
	iter->pos = *pos;

	return (void *)pos;
}

static int
region_show(struct seq_file *m, void *v)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_seq(m);
	ssboot_region_iter_t *iter = (ssboot_region_iter_t *)ops->data;

	/* print header */
	if (iter->pos == 0) {
		seq_printf(m, "Physical Address          "
			      "KByte   Pages  Attribute\n");
		return 0;
	}

	/* print body */
	seq_printf(m, "0x%08llx - 0x%08llx  %6ld  %6ld  ",
		   (u_int64_t)PFN_PHYS(iter->pfn),
		   (u_int64_t)PFN_PHYS(iter->pfn + iter->num) - 1,
		   (unsigned long)PFN_PHYS(iter->num) / SSBOOT_SZ_1KB,
		   iter->num);
	if (iter->attr & SSBOOT_EXREG_KERNEL) {
		seq_printf(m, "Kernel");
	} else {
		seq_printf(m, "User");
	}
	seq_printf(m, "\n");

	return 0;
}

static int
region_release(struct inode *inode, struct file *file, unsigned long attr,
	       int remove)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	u_int64_t phys_addr;
	unsigned long len;
	char *start, *end;

	/* check if data exists */
	if (ops->write_len == 0) {
		goto free;
	}

	/* skip first spaces */
	start = ops->write_buf;
	while (isspace(*start)) {
		start++;
	}

	/* get first parameter */
	end = start;
	while (*end != '\0' && !isspace(*end)) {
		end++;
	}
	*end = '\0';

	/* abort if too short */
	if (end - ops->write_buf > ops->write_len) {
		goto free;
	}
	phys_addr = simple_strtoull(start, NULL, 0);

	/* skip spaces */
	start = end + 1;
	while (isspace(*start)) {
		start++;
	}

	/* get second parameter */
	end = start;
	while (*end != '\0' && !isspace(*end)) {
		end++;
	}
	*end = '\0';

	/* abort if too short */
	if (end - ops->write_buf > ops->write_len) {
		goto free;
	}
	len = simple_strtoul(start, NULL, 0);

	/* check alignlent */
	if (((phys_addr | len) & ~PAGE_MASK) != 0) {
		ssboot_err("cannot %s region (invalid align): "
			   "0x%08llx-0x%08llx\n",
			   remove ? "unregister" : "register",
			   phys_addr, phys_addr + len - 1);
	} else {
		if (remove) {
			/* unregister extra region */
			ssboot_exreg_unregister(SSBOOT_EXREG_USER,
						PFN_DOWN(phys_addr),
						PFN_DOWN(len));
		} else {
			/* register extra region */
			ssboot_exreg_register(SSBOOT_EXREG_USER | attr,
					      PFN_DOWN(phys_addr),
					      PFN_DOWN(len));
		}
	}
 free:
	/* free iterator */
	kfree(ops->data);

	return 0;
}

/*
 * region/normal
 */
static int
region_normal_open(struct inode *inode, struct file *file)
{
	return region_open(inode, file);
}

static void*
region_normal_start(struct seq_file *m, loff_t *pos)
{
	return region_start(m, pos, SSBOOT_EXREG_NORMAL);
}

static void*
region_normal_next(struct seq_file *m, void *v, loff_t *pos)
{
	return region_next(m, v, pos, SSBOOT_EXREG_NORMAL);
}

static int
region_normal_show(struct seq_file *m, void *v)
{
	return region_show(m, v);
}

static int
region_normal_release(struct inode *inode, struct file *file)
{
	return region_release(inode, file, SSBOOT_EXREG_NORMAL, 0);
}
ssboot_seq_proc(region_normal, SSBOOT_PROC_RDWR, PAGE_SIZE);

/*
 * region/critical
 */
static int
region_critical_open(struct inode *inode, struct file *file)
{
	return region_open(inode, file);
}

static void*
region_critical_start(struct seq_file *m, loff_t *pos)
{
	return region_start(m, pos, SSBOOT_EXREG_CRITICAL);
}

static void*
region_critical_next(struct seq_file *m, void *v, loff_t *pos)
{
	return region_next(m, v, pos, SSBOOT_EXREG_CRITICAL);
}

static int
region_critical_show(struct seq_file *m, void *v)
{
	return region_show(m, v);
}

static int
region_critical_release(struct inode *inode, struct file *file)
{
	return region_release(inode, file, SSBOOT_EXREG_CRITICAL, 0);
}
ssboot_seq_proc(region_critical, SSBOOT_PROC_RDWR, PAGE_SIZE);

/*
 * region/discard
 */
static int
region_discard_open(struct inode *inode, struct file *file)
{
	return region_open(inode, file);
}

static void*
region_discard_start(struct seq_file *m, loff_t *pos)
{
	return region_start(m, pos, SSBOOT_EXREG_DISCARD);
}

static void*
region_discard_next(struct seq_file *m, void *v, loff_t *pos)
{
	return region_next(m, v, pos, SSBOOT_EXREG_DISCARD);
}

static int
region_discard_show(struct seq_file *m, void *v)
{
	return region_show(m, v);
}

static int
region_discard_release(struct inode *inode, struct file *file)
{
	return region_release(inode, file, SSBOOT_EXREG_DISCARD, 0);
}
ssboot_seq_proc(region_discard, SSBOOT_PROC_RDWR, PAGE_SIZE);

/*
 * region/work
 */
static int
region_work_open(struct inode *inode, struct file *file)
{
	return region_open(inode, file);
}

static void*
region_work_start(struct seq_file *m, loff_t *pos)
{
	return region_start(m, pos, SSBOOT_EXREG_WORK);
}

static void*
region_work_next(struct seq_file *m, void *v, loff_t *pos)
{
	return region_next(m, v, pos, SSBOOT_EXREG_WORK);
}

static int
region_work_show(struct seq_file *m, void *v)
{
	return region_show(m, v);
}

static int
region_work_release(struct inode *inode, struct file *file)
{
	return region_release(inode, file, SSBOOT_EXREG_WORK, 0);
}
ssboot_seq_proc(region_work, SSBOOT_PROC_RDWR, PAGE_SIZE);

/*
 * region/remove
 */
static int
region_remove_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
region_remove_show(struct seq_file *m, void *v)
{
	return 0;
}

static int
region_remove_release(struct inode *inode, struct file *file)
{
	return region_release(inode, file, 0, 1);
}
ssboot_single_proc(region_remove, SSBOOT_PROC_WRONLY, PAGE_SIZE);

/*
 * region/init
 */
static int
region_init_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
region_init_show(struct seq_file *m, void *v)
{
	return 0;
}

static int
region_init_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	unsigned long pfn, num;
	int ret;

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* unregister all user entries */
	for (;;) {
		ssboot_exreg_find_first(SSBOOT_EXREG_USER, &pfn, &num);
		if (pfn == SSBOOT_PFN_NONE) {
			break;
		}
		ret = ssboot_exreg_unregister(SSBOOT_EXREG_USER, pfn, num);
		if (ret < 0) {
			BUG();
		}
	}
	return 0;
}
ssboot_single_proc(region_init, SSBOOT_PROC_WRONLY, PAGE_SIZE);

/*
 * Exported functions
 */
int
ssboot_proc_create_entry(const char *name, ssboot_proc_ops_t *ops,
			 struct proc_dir_entry *parent)
{
	struct proc_dir_entry *entry;

	/* fixup seq_ops */
	if (ops->seq_ops.start == NULL) {
		ops->seq_ops.start = ssboot_proc_single_start;
	}
	if (ops->seq_ops.next == NULL) {
		ops->seq_ops.next  = ssboot_proc_single_next;
	}
	ops->seq_ops.stop  = ssboot_proc_stop;

	/* create proc entry */
	entry = create_proc_entry(name, ops->mode, parent);
	if (entry == NULL) {
		return -ENOMEM;
	}
	entry->proc_fops = &wrapper_ops;
	entry->data = (void*)ops;

	return 0;
}
EXPORT_SYMBOL(ssboot_proc_create_entry);

void
ssboot_proc_remove_entry(const char *name, struct proc_dir_entry *parent)
{
	remove_proc_entry(name, parent);
}
EXPORT_SYMBOL(ssboot_proc_remove_entry);

/*
 * Initialization
 */
static ssboot_proc_list_t ssboot_procs[] = {
	{ "mode",	&mode_ops		},
	{ "resmode",	&resmode_ops		},
	{ "operation",	&operation_ops		},
	{ "section",	&section_ops		},
	{ "state",	&state_ops		},
	{ "swapfile",	&swapfile_ops		},
};

static ssboot_proc_list_t region_procs[] = {
	{ "normal",	&region_normal_ops	},
	{ "critical",	&region_critical_ops	},
	{ "discard",	&region_discard_ops	},
	{ "work",	&region_work_ops	},
	{ "remove",	&region_remove_ops	},
	{ "init",	&region_init_ops	},
};

static ssboot_proc_list_t optimizer_procs[] = {
	{ "operation",	&optimizer_operation_ops},
};

static int __init
ssboot_proc_init(void)
{
	struct proc_dir_entry *region_entry;
	int i, ret;

	/* create 'ssboot' directory */
	ssboot_proc_root = proc_mkdir("ssboot", NULL);
	if (ssboot_proc_root == NULL) {
		return -ENOMEM;
	}

	/* create entries under 'ssboot' directory */
	for (i = 0; i < ARRAY_SIZE(ssboot_procs); i++) {
		ret = ssboot_proc_create_entry(ssboot_procs[i].name,
					       ssboot_procs[i].ops,
					       ssboot_proc_root);
		if (ret < 0) {
			return ret;
		}
	}

	/* create 'region' directory under 'ssboot' directory */
	region_entry = proc_mkdir("region", ssboot_proc_root);
	if (region_entry == NULL) {
		return -ENOMEM;
	}

	/* create entries under 'region' directory */
	for (i = 0; i < ARRAY_SIZE(region_procs); i++) {
		ret = ssboot_proc_create_entry(region_procs[i].name,
					       region_procs[i].ops,
					       region_entry);
		if (ret < 0) {
			return ret;
		}
	}

	/* create 'optimizer' directory under 'ssboot' directory */
	ssboot_proc_optimizer = proc_mkdir("optimizer", ssboot_proc_root);
	if (ssboot_proc_optimizer == NULL) {
		return -ENOMEM;
	}

	/* create entries under 'optimizer' directory */
	for (i = 0; i < ARRAY_SIZE(optimizer_procs); i++) {
		ret = ssboot_proc_create_entry(optimizer_procs[i].name,
					       optimizer_procs[i].ops,
					       ssboot_proc_optimizer);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}
subsys_initcall(ssboot_proc_init);
