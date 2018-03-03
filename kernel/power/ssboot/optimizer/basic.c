/* 2012-08-28: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot - Basic image optimizer
 *
 *  Copyright 2012 Sony Corporation
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
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/vmalloc.h>
#include <linux/hash.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/mount.h>
#include <linux/ssboot.h>
#include <linux/delay.h>

#include <asm/uaccess.h>

#include "../internal.h"

struct proc_dir_entry *ssboot_proc_optimizer_basic;

#define PROFILE CONFIG_SNSC_SSBOOT_BASIC_OPTIMIZER_PROFILE_NAME
#define SSBOOT_BASIC_OPTIMIZER_PROF_MAGIC 0x0960f17e
#define SSBOOT_BASIC_OPTIMIZER_VER_MAJOR (0x0000)
#define SSBOOT_BASIC_OPTIMIZER_VER_MINOR (0x0001)
#define SSBOOT_BASIC_OPTIMIZER_PROF_VERSION \
         ((SSBOOT_BASIC_OPTIMIZER_VER_MAJOR << 16) + \
          (SSBOOT_BASIC_OPTIMIZER_VER_MINOR))

/* file name table hash */
#define hashfn(str)	\
  hash_long((unsigned long)strlen(str), fname_hash_shift)
static unsigned int fname_hash_shift = 4;

static char prof_path[SSBOOT_PATH_MAX];
module_param_string(prof_path, prof_path, SSBOOT_PATH_MAX, S_IRUGO | S_IWUSR);

static int dbg_enable = 0;
module_param_named(debug, dbg_enable, int, S_IRUGO | S_IWUSR);

#define ssboot_dbg_basic(format, arg...)				\
	do {								\
		if (dbg_enable)						\
			printk(KERN_INFO "ssboot: " format, ##arg);	\
	}								\
	while(0)

#define MAX_FILES 32
static char *drop_file_list[MAX_FILES] = { NULL };
module_param_array_named(ignore, drop_file_list, charp, NULL, S_IRUGO | S_IWUSR);

/* for file access */
static struct file *filp;
static mm_segment_t oldfs;

struct ssboot_bo_prof_file_name {
	char fname[SSBOOT_PATH_MAX];
	int index;
	struct hlist_node node;
};

struct ssboot_bo_prof_file_data {
	int index;
	pgoff_t offset;
};

struct ssboot_bo_prof_file {
	struct list_head list;
	struct list_head list_merge;
	struct ssboot_bo_prof_file_data data;
};

struct ssboot_bo_prof_anon_data {
	struct task_struct *tsk;
	unsigned long address;
};

struct ssboot_bo_prof_anon {
	struct list_head list;
	struct list_head list_merge;
	struct ssboot_bo_prof_anon_data data;
};

struct ssboot_bo_prof_header_event {
	uint32_t offset;
	uint32_t num;
};

struct ssboot_bo_prof_header_fname_tbl {
	uint32_t offset;
	uint32_t num;
};

struct ssboot_bo_prof_header {
	int magic;
	int version;
	struct ssboot_bo_prof_header_fname_tbl fname_tbl;
	struct ssboot_bo_prof_header_event h_ev[SSBOOT_OPTEVT_MAX];
};

struct ssboot_bo_info {
	int is_profiling;
	int stop_recording;
	struct ssboot_bo_prof_header header;
	struct hlist_head *fname_tbl_hlist;
	int fname_cnt;
	struct rw_semaphore fname_lock; /* for fname table */
};

struct ssboot_basic_optimizer {
	unsigned long stat[SSBOOT_OPTEVT_MAX];
	struct list_head prof_file_head;
	struct list_head prof_anon_head;
};

DEFINE_PER_CPU(struct ssboot_basic_optimizer, ssboot_basic_optimizers);

static inline void count_event(enum ssboot_optimizer_event ev)
{
	this_cpu_inc(ssboot_basic_optimizers.stat[ev]);
}

static inline struct ssboot_basic_optimizer *get_ssboot_basic_optimizer(void)
{
	return (&get_cpu_var(ssboot_basic_optimizers));
}

static inline void put_ssboot_basic_optimizer(void)
{
	put_cpu_var(ssboot_basic_optimizers);
}

static void init_ssboot_basic_optimizers(void)
{
	int cpu;

	get_online_cpus();
	for_each_cpu(cpu, cpu_online_mask) {
		struct ssboot_basic_optimizer *this =
			&per_cpu(ssboot_basic_optimizers, cpu);
		INIT_LIST_HEAD(&this->prof_file_head);
		INIT_LIST_HEAD(&this->prof_anon_head);
	}
	put_online_cpus();

}

static void init_ssboot_bo_info(struct ssboot_bo_info *info)
{
	int i, fname_hash_size;

	info->is_profiling = 0;
	info->stop_recording = 0;
	info->header.magic = SSBOOT_BASIC_OPTIMIZER_PROF_MAGIC;
	info->header.version = SSBOOT_BASIC_OPTIMIZER_PROF_VERSION;

	fname_hash_size = 1 << fname_hash_shift;
	info->fname_tbl_hlist =
		vmalloc(sizeof(struct hlist_head) * fname_hash_size);
	for (i = 0; i < fname_hash_size; i++) {
		INIT_HLIST_HEAD((info->fname_tbl_hlist + i));
	}
	info->fname_cnt = 0;
	init_rwsem(&info->fname_lock);

}

static void sum_profile_events(unsigned long *stat,
			const struct cpumask *cpumask)
{
	int cpu;
	int i;

	for_each_cpu(cpu, cpumask) {
		struct ssboot_basic_optimizer *this =
			&per_cpu(ssboot_basic_optimizers, cpu);

		for (i = 0; i < SSBOOT_OPTEVT_MAX; i++)
			*(stat+i) += this->stat[i];
	}
}

static void get_event_statistics(unsigned long *stat)
{
	get_online_cpus();
	sum_profile_events(stat, cpu_online_mask);
	put_online_cpus();
}

static int file_open(int flags)
{
	/* open file */
	filp = filp_open(prof_path, flags, 0644);
	if (IS_ERR(filp)) {
		ssboot_err("cannot open file: %s\n", prof_path);
		return PTR_ERR(filp);
	}
	get_file(filp);

	/* for kernel address */
	oldfs = get_fs();
	set_fs(get_ds());

	return 0;
}

static void file_close(void)
{
	/* close file */
	fput(filp);
	filp_close(filp, current->files);

	/* restore setting */
	set_fs(oldfs);
}

static int get_filename(struct path *path, char **buffer, int buflen, char **pathname)
{
	*pathname = d_path(path, *buffer, buflen);

	if (IS_ERR(*pathname))
		return -EINVAL;

	return strlen(*pathname);
}

static struct hlist_head *
get_fname_tbl_slot(struct hlist_head *head, char *str)
{
	return head + hashfn(str);
}

static struct ssboot_bo_prof_file_name *find_node(
	struct hlist_head *h, char *str)
{
	struct ssboot_bo_prof_file_name *tpos;
	struct hlist_node *pos;
	struct hlist_head *head;

	head = get_fname_tbl_slot(h, str);
	hlist_for_each_entry(tpos, pos, head, node) {
		if (strncmp(str, tpos->fname, SSBOOT_PATH_MAX) == 0) {
			return tpos;
		}
	}
	return NULL;
}


static int
get_fname_index(struct ssboot_bo_info *info, char *fname, int len)
{
	struct hlist_head *h = info->fname_tbl_hlist;
	struct hlist_head *head;
	struct ssboot_bo_prof_file_name *node;
	struct ssboot_bo_prof_file_name *new_node;

	char real_fname[SSBOOT_PATH_MAX];
	int  real_len;

	down_read(&info->fname_lock);
	real_len = snprintf(real_fname, SSBOOT_PATH_MAX, "/%s%s",
			    current->fs->root.mnt->mnt_mountpoint->d_iname,
			    fname);
	node = find_node(h, real_fname);
	if (node) {
		up_read(&info->fname_lock);
		return node->index;
	}
	up_read(&info->fname_lock);

	/* node is not found */
	down_write(&info->fname_lock);
	head = get_fname_tbl_slot(h, real_fname);
	new_node = vzalloc(sizeof(struct ssboot_bo_prof_file_name));
	if (new_node == NULL) {
		up_write(&info->fname_lock);
		return -1;
	}
	strncpy(new_node->fname, real_fname, real_len);
	new_node->index = info->fname_cnt++;
	hlist_add_head(&new_node->node, head);
	up_write(&info->fname_lock);

	return new_node->index;
}

static int
task_is_alive(struct task_struct *tsk)
{
	struct task_struct *p = NULL;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (p == tsk) {
			read_unlock(&tasklist_lock);
			return 1;
		}
	}
	read_unlock(&tasklist_lock);
	return 0;
}

static int is_ignored_file(char *fname)
{
	int i;
	char **fpts = drop_file_list;

	for (i = 0; i < MAX_FILES && *fpts; i++, fpts++) {
		if (strncmp(fname, *fpts, SSBOOT_PATH_MAX) == 0) {
			return 1;
		}
	}

	return 0;
}

static int
ssboot_basic_record_file(struct ssboot_bo_info *info,
			struct file *file, pgoff_t offset)
{
	struct ssboot_bo_prof_file *pf = NULL;
	struct ssboot_basic_optimizer *this;
	struct path *f_path;
	char *pathname;
	int ret = 0;
	int len;
	char *tmp = NULL;

	tmp = (char*)get_zeroed_page(GFP_KERNEL);
	if (tmp == NULL) {
		ssboot_err("%s(%d): no memory", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	pf = kzalloc(sizeof(struct ssboot_bo_prof_file), GFP_KERNEL);
	if (pf == NULL) {
		ret = -ENOMEM;
		ssboot_err("%s(%d): no memory", __FUNCTION__, __LINE__);
		goto alloc_err;
	}

	if (file == NULL) {
		ssboot_err("%s(%d): file is NULL",
			__FUNCTION__, __LINE__);
		goto err;
	}
	f_path = &file->f_path;
	len = get_filename(f_path, &tmp, PAGE_SIZE, &pathname);
	if (len < 0) {
		ret = len;
		ssboot_err("%s(%d): fault get_filename()",
			__FUNCTION__, __LINE__);
		goto err;
	}
	if (is_ignored_file(pathname)) {
		ssboot_dbg("%s(%d): %s ignored\n",
			__FUNCTION__, __LINE__, pathname);
		goto err;
	}
	pf->data.index = get_fname_index(info, pathname, len);
	if (pf->data.index == -1) {
		ret = -ENOENT;
		ssboot_err("%s(%d): fault to get index of %s\n",
			__FUNCTION__, __LINE__, pathname);
		goto err;
	}

	ssboot_dbg("Major page fault=%s", pathname);

	pf->data.offset = offset;
	INIT_LIST_HEAD(&pf->list);
	INIT_LIST_HEAD(&pf->list_merge);
	this = get_ssboot_basic_optimizer();
	list_add_tail(&pf->list, &this->prof_file_head);
	put_ssboot_basic_optimizer();

	if (tmp) {
		free_page((unsigned long)tmp);
	}

	return 0;

err:
	if (pf) {
		kfree(pf);
	}

alloc_err:
	if (tmp) {
		free_page((unsigned long)tmp);
	}

	return ret;
}

static int
ssboot_basic_record_anon(struct task_struct *tsk, unsigned long address)
{
	struct ssboot_bo_prof_anon *pa;
	struct ssboot_basic_optimizer *this;

	pa = kzalloc(sizeof(struct ssboot_bo_prof_anon), GFP_KERNEL);
	if (pa == NULL) {
		ssboot_err("%s(%d): no memory", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	pa->data.tsk = tsk->group_leader;
	pa->data.address = address;
	INIT_LIST_HEAD(&pa->list);
	INIT_LIST_HEAD(&pa->list_merge);
	this = get_ssboot_basic_optimizer();
	list_add_tail(&pa->list, &this->prof_anon_head);
	put_ssboot_basic_optimizer();

	return 0;
}

static int
ssboot_basic_merge_list(struct list_head *merged)
{
	struct ssboot_bo_prof_file *pf;
	int cpu;

	get_online_cpus();

	for_each_cpu(cpu, cpu_online_mask) {
		struct ssboot_basic_optimizer *this =
			&per_cpu(ssboot_basic_optimizers, cpu);
		list_for_each_entry(pf, &this->prof_file_head, list) {
			list_add_tail(&pf->list_merge, merged);
		}
	}
	put_online_cpus();

	return 0;
}

static int
ssboot_basic_merge_list_anon(struct list_head *merged)
{
	struct ssboot_bo_prof_anon *pa;
	int cpu;

	get_online_cpus();

	for_each_cpu(cpu, cpu_online_mask) {
		struct ssboot_basic_optimizer *this =
			&per_cpu(ssboot_basic_optimizers, cpu);
		list_for_each_entry(pa, &this->prof_anon_head, list) {
			list_add_tail(&pa->list_merge, merged);
		}
	}
	put_online_cpus();

	return 0;
}

static int
ssboot_basic_read_profile(char **ary,
			struct ssboot_bo_prof_file_data **data,
			uint32_t *num,
			struct ssboot_bo_prof_anon_data **a_data,
			uint32_t *anon_num)
{
	loff_t p = 0;
	loff_t p_tbl_start = 0;
	struct ssboot_bo_prof_header header;
	size_t count = 0;
	struct ssboot_bo_prof_file_data *file_data = NULL;
	struct ssboot_bo_prof_anon_data *anon_data = NULL;
	int i = 0;
	int j = 0;
	int tbl_num = 0;
	char *fname_ary;
	int ret = 0;

	ret = file_open(O_RDONLY);
	if (ret) {
		return ret;
	}

	count = sizeof(struct ssboot_bo_prof_header);
	vfs_read(filp, (char *)&header, count, &p);

	if (header.magic != SSBOOT_BASIC_OPTIMIZER_PROF_MAGIC) {
		ssboot_err("It is not profile data file(%d)\n", header.magic);
		return -EINVAL;
	}
	if (header.version != SSBOOT_BASIC_OPTIMIZER_PROF_VERSION) {
		ssboot_err("The version(0x%0X) is not supported\n",
			header.version);
		return -EINVAL;
	}
	p_tbl_start = header.fname_tbl.offset;
	tbl_num = header.fname_tbl.num;
	p = header.h_ev[SSBOOT_OPTEVT_FILE].offset;
	*num = header.h_ev[SSBOOT_OPTEVT_FILE].num;

	file_data = vmalloc(sizeof(struct ssboot_bo_prof_file_data) * (*num));
	if (file_data == NULL) {
		ssboot_err("fault vmalloc() for file_data\n");
		return -ENOMEM;
	}


	fname_ary = vmalloc(SSBOOT_PATH_MAX * tbl_num);
	if (fname_ary == NULL) {
		ssboot_err("fault vmalloc() for fname_ary tbl_num=%d\n", tbl_num);
		ret = -ENOMEM;
		goto fname_ary_err;
	}

	/* read file name table */
	count = SSBOOT_PATH_MAX * tbl_num;
	vfs_read(filp, (char *)fname_ary, count, &p_tbl_start);

	/* read file data */
	count = sizeof(struct ssboot_bo_prof_file_data);
	for (i = 0; i < (*num); i++) {
		vfs_read(filp, (char *)(file_data + i), count, &p);
		j = (file_data + i)->index;
		if (j > tbl_num) {
			ret = -EINVAL;
			ssboot_err("The index(%d) is larger than the number "
				"of file table index %d\n", j, tbl_num);
			goto read_file_data_err;
		}
	}

	p = header.h_ev[SSBOOT_OPTEVT_ANON].offset;
	*anon_num = header.h_ev[SSBOOT_OPTEVT_ANON].num;

	if (*anon_num != 0) {
		anon_data = vmalloc(
			sizeof(struct ssboot_bo_prof_anon_data) * (*anon_num));
		if (anon_data == NULL) {
			ssboot_err("fault vmalloc() for anon_data num=%d\n", (*anon_num));
			ret = -ENOMEM;
			goto anon_data_err;
		}

		/* read anon data */
		count = sizeof(struct ssboot_bo_prof_anon_data);
		for (i = 0; i < (*anon_num); i++) {
			vfs_read(filp, (char *)(anon_data + i), count, &p);
		}
	}

	file_close();

	*data = file_data;
	*a_data = anon_data;
	*ary = fname_ary;

	return 0;

anon_data_err:
read_file_data_err:
	if (fname_ary) {
		vfree(fname_ary);
	}

fname_ary_err:
	if (file_data) {
		vfree(file_data);
	}

	file_close();

	return ret;
}

static int
ssboot_basic_read_file_page(struct ssboot_bo_prof_file_data **data, int num, char **ary)
{
	struct ssboot_bo_prof_file_data *file_data = *data;
	char *fname_ary = *ary;
	int i = 0;
	struct file *file;
	char *fname;
	char *buf;
	loff_t pos;
	int ret = 0;

	buf = (char*)__get_free_page(GFP_KERNEL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		fname = fname_ary + SSBOOT_PATH_MAX * ((file_data+i)->index);
		ssboot_dbg_basic("Read:%s:%lx\n", fname,
			((file_data+i)->offset << PAGE_CACHE_SHIFT));
		file = filp_open(fname, O_RDONLY, 0444);
		if (IS_ERR(file)) {
			ssboot_err("cannot open file: %s\n", fname);
			continue;
		}
		get_file(file);

		pos = (file_data+i)->offset << PAGE_CACHE_SHIFT;
		kernel_read(file, pos, buf, PAGE_SIZE);

		/* close file */
		fput(file);
		filp_close(file, current->files);
	}

	if (fname_ary) {
		vfree(fname_ary);
	}

	if (file_data) {
		vfree(file_data);
	}

	if (buf) {
		free_page((unsigned long)buf);
	}

	return ret;
}

static int
ssboot_basic_read_anon_page(struct ssboot_bo_prof_anon_data **data, int num)
{
	struct ssboot_bo_prof_anon_data *anon_data = *data;
	int i = 0;
	struct task_struct *tsk = NULL;
	unsigned long address;
	int ret = 0;
	int err = 0;
	char *buf;

	buf = (char*)__get_free_page(GFP_KERNEL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		tsk = (anon_data+i)->tsk;
		address = (anon_data+i)->address;

		ssboot_dbg_basic("ReadSwap:%-16s:%lx\n", tsk->comm, address);
		/*
                 * If a task is generated and deleted during profiling,
                 * task_struct is recorded. It might be same address
                 * of task_struct when new process is generated after
                 * booting with optimized image. But it will be rare.
		 */
		if (task_is_alive(tsk)) {
			ret = access_process_vm(tsk, address,
						buf, PAGE_SIZE, 0);
			if (ret < 0) {
				err = ret;
				ssboot_err("No pages were pinned: %d\n", ret);
			}
		}
	}

	if (anon_data) {
		vfree(anon_data);
	}

	if (buf) {
		free_page((unsigned long)buf);
	}

	return err;
}


/*
 * Image optimizer operations
 */
static int
ssboot_basic_optimize(void *priv)
{
	unsigned long free, progress = 0;
	struct ssboot_bo_prof_file_data *file_data = NULL;
	struct ssboot_bo_prof_anon_data *anon_data = NULL;
	uint32_t file_num = 0;
	uint32_t anon_num = 0;
	char *fname_ary = NULL;
	int ret = 0;

	ssboot_info("optimizing image...\n");

	ret = ssboot_basic_read_profile(&fname_ary,
					&file_data, &file_num,
					&anon_data, &anon_num);
	if (ret < 0) {
		ssboot_err("failed ssboot_basic_read_profile() %d\n", ret);
		return ret;
	}

	/* free all memory */
	ssboot_info("Freeing all memory.");
	do {
		/* free 10MB per loop */
		free = __shrink_all_memory(10 * SSBOOT_PG_1MB, 1, 1);
		printk("(%s):Active(anon)%lu Inactive(anon)%lu\n",
		       __FUNCTION__, global_page_state(NR_ACTIVE_ANON), global_page_state(NR_INACTIVE_ANON));
		msleep(100);
		/*
		 * __shrink_all_memory() may return non 0 value even
		 * if no pages are reclaimed. Therefore summation of
		 * the return value does not indicate actual reclaimed
		 * pages, and a "." does not mean that 10MB memory is
		 * reclaimed.
		 */
		progress += free;
		if (progress >= (10 * SSBOOT_PG_1MB)) {
			progress = 0;
		}
	} while (free > 0);
	printk("done\n");

	/* Include anonymous page into image */
	ret = ssboot_basic_read_anon_page(&anon_data, anon_num);
	if (ret < 0) {
		return ret;
	}

	/* Include page cache into image */
	ret = ssboot_basic_read_file_page(&file_data, file_num, &fname_ary);
	if (ret < 0) {
		return ret;
	}

	ssboot_info("optimizing image done\n");

	return 0;
}

static int
ssboot_basic_start_profiling(void *priv)
{
	struct ssboot_bo_info *info = (struct ssboot_bo_info *)priv;
	int *is_profiling = &info->is_profiling;

	ssboot_info("basic: start profiling...\n");

	*is_profiling = 1;

	return 0;
}

static int
ssboot_basic_stop_profiling(void *priv)
{
	struct ssboot_bo_info *info = (struct ssboot_bo_info *)priv;
	int *is_profiling = &info->is_profiling;
	int *stop_recording = &info->stop_recording;
	struct ssboot_bo_prof_file *pf, *pf_next;
	struct ssboot_bo_prof_anon *pa, *pa_next;
	int i, fname_hash_size;
	struct ssboot_bo_prof_file_name *tpos;
	struct hlist_node *pos, *pos_next;
	loff_t p = 0;
	size_t count = 0;
	uint32_t num = 0;
	int open_flags = O_RDWR | O_CREAT | O_TRUNC;
	int ret = 0;

	LIST_HEAD(head);
	LIST_HEAD(anon_head);

	ssboot_info("stop profiling...");

	*stop_recording = 1;

	(void)ssboot_basic_merge_list(&head);
	(void)ssboot_basic_merge_list_anon(&anon_head);

	ret = file_open(open_flags);
	if (ret) {
		return ret;
	}

	/* write file name table */
	info->header.fname_tbl.offset = p + sizeof(struct ssboot_bo_prof_header);
	count = SSBOOT_PATH_MAX;
	num = 0;
	fname_hash_size = 1 << fname_hash_shift;
	for (i = 0; i < fname_hash_size; i++) {
		down_write(&info->fname_lock);
		hlist_for_each_entry_safe(tpos, pos, pos_next,
					(info->fname_tbl_hlist + i), node) {
			p = info->header.fname_tbl.offset +
				tpos->index * count;
			vfs_write(filp, (char *)&tpos->fname, count, &p);
			num++;
			hlist_del(&tpos->node);
			vfree(tpos);
		}
		up_write(&info->fname_lock);
	}
	p = info->header.fname_tbl.offset + num * count;
	info->header.fname_tbl.num = num;

	if (info->fname_tbl_hlist) {
		vfree(info->fname_tbl_hlist);
		info->fname_tbl_hlist = NULL;
	}

	/* write file information */
	info->header.h_ev[SSBOOT_OPTEVT_FILE].offset = p;
	count = sizeof(struct ssboot_bo_prof_file_data);
	num = 0;
	list_for_each_entry_safe(pf, pf_next, &head, list_merge) {
		vfs_write(filp, (char *)&pf->data, count, &p);
		num++;
		list_del(&pf->list_merge);
		kfree(pf);
	}
	info->header.h_ev[SSBOOT_OPTEVT_FILE].num = num;

	/* write anonymous page information */
	info->header.h_ev[SSBOOT_OPTEVT_ANON].offset = p;
	count = sizeof(struct ssboot_bo_prof_anon_data);
	num = 0;
	list_for_each_entry_safe(pa, pa_next, &anon_head, list_merge) {
		vfs_write(filp, (char *)&pa->data, count, &p);
		num++;
		list_del(&pa->list_merge);
		kfree(pa);
	}
	info->header.h_ev[SSBOOT_OPTEVT_ANON].num = num;

	/* update header */
	p = 0;
	count = sizeof(struct ssboot_bo_prof_header);
	vfs_write(filp, (char *)&info->header, count, &p);

	file_close();

	*stop_recording = 0;
	*is_profiling = 0;

	printk("done\n");

	/* Show profile file header information */
	ssboot_info("Profile File info\n");
	ssboot_info("fname_tbl: offset=%d, num=%d\n",
		info->header.fname_tbl.offset,
		info->header.fname_tbl.num);
	for (i = 0; i< SSBOOT_OPTEVT_MAX; i++) {
		ssboot_info("event[%d]: offset=%d, num=%d\n",
			i,
			info->header.h_ev[i].offset,
			info->header.h_ev[i].num);
	}

	return 0;

}

static int
ssboot_basic_is_profiling(void *priv)
{
	struct ssboot_bo_info *info = (struct ssboot_bo_info *)priv;
	int *is_profiling = &info->is_profiling;

	return *is_profiling;
}

static int
ssboot_basic_record(void *priv, enum ssboot_optimizer_event ev, va_list args)
{
	struct ssboot_bo_info *info = (struct ssboot_bo_info *)priv;
	int *stop_recording = &info->stop_recording;
	struct file *file;
	pgoff_t offset;
	struct task_struct *tsk;
	unsigned long address;
	int ret;

	if (*stop_recording == 1) {
		return 0;
	}

	count_event(ev);
	switch (ev) {
	case SSBOOT_OPTEVT_FILE:
		file = va_arg(args, struct file *);
		offset = va_arg(args, pgoff_t);
		ret = ssboot_basic_record_file(info, file, offset);
		if (ret < 0) {
			return ret;
		}
		break;
	case SSBOOT_OPTEVT_ANON:
		tsk = va_arg(args, struct task_struct *);
		address = va_arg(args, unsigned long);
		ret = ssboot_basic_record_anon(tsk, address);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		break;
	}

	return 0;
}

/*
 * Proc
 */

/*
 * optimizer
 */
static int
basic_estimation_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
basic_estimation_show(struct seq_file *m, void *v)
{
	int i = 0;
	unsigned long stat[SSBOOT_OPTEVT_MAX];

	if (!ssboot_is_profiling()) {
		return 0;
	}
	if (ssboot_optimizer_is_profiling()) {
		seq_printf(m, "status: profiling\n");
	} else {
		seq_printf(m, "status: profile done\n");
	}
	memset(stat, 0, sizeof(stat));
	get_event_statistics(stat);
	for (i = 0; i < SSBOOT_OPTEVT_MAX; i++) {
		seq_printf(m, "%s: %lu\n",
			ssboot_optimizer_event_name[i],
			stat[i]);
	}

	return 0;
}

static int
basic_estimation_release(struct inode *inode, struct file *file)
{
	return 0;
}
ssboot_single_proc(basic_estimation, SSBOOT_PROC_RDWR, 16);

static ssboot_proc_list_t basic_procs[] = {
	{ "estimation",	&basic_estimation_ops},
};


static int __init
ssboot_basic_proc_init(void)
{
	int i = 0;
	int ret = 0;

	/* create 'basic' directory under 'ssboot/optimizer' directory */
	ssboot_proc_optimizer_basic =
		proc_mkdir("basic", ssboot_proc_optimizer);
	if (ssboot_proc_optimizer_basic == NULL) {
		return -ENOMEM;
	}

	/* create entries under 'basic' directory */
	for (i = 0; i < ARRAY_SIZE(basic_procs); i++) {
		ret = ssboot_proc_create_entry(basic_procs[i].name,
					       basic_procs[i].ops,
					       ssboot_proc_optimizer_basic);
		if (ret < 0) {
			return ret;
		}
	}
	return ret;
}



/*
 * Initialization
 */
static ssboot_optimizer_t ssboot_basic_optimizer = {
	.optimize		= ssboot_basic_optimize,
	.start_profiling	= ssboot_basic_start_profiling,
	.stop_profiling		= ssboot_basic_stop_profiling,
	.is_profiling		= ssboot_basic_is_profiling,
	.record			= ssboot_basic_record,
};

static int __init
ssboot_basic_optimizer_init_module(void)
{
	int ret;
	struct ssboot_bo_info *info;

	info = kzalloc(sizeof(struct ssboot_bo_info), GFP_KERNEL);
	if (info == NULL) {
		return -ENOMEM;
	}
	ssboot_basic_optimizer.priv = (void*)info;

	/* register basic image optimizer */
	ret = ssboot_optimizer_register(&ssboot_basic_optimizer);
	if (ret < 0) {
		goto err_reg;
	}
	ret = ssboot_basic_proc_init();
	if (ret < 0) {
		goto err_proc;
	}

	init_ssboot_basic_optimizers();
	init_ssboot_bo_info(info);

	ssboot_dbg("registered basic image optimizer\n");

	return 0;

err_proc:
	(void)ssboot_optimizer_unregister(&ssboot_basic_optimizer);
err_reg:
	if (info != NULL) {
		kfree(info);
		info = NULL;
	}
	return ret;
}

static void __exit
ssboot_basic_optimizer_cleanup_module(void)
{
	int ret;

	/* unregister basic image optimizer */
	ret = ssboot_optimizer_unregister(&ssboot_basic_optimizer);
	if (ret < 0) {
		return;
	}
	ssboot_dbg("unregistered basic image optimizer\n");

	/* free memory to store profiling state */
	if (ssboot_basic_optimizer.priv != NULL) {
		kfree(ssboot_basic_optimizer.priv);
		ssboot_basic_optimizer.priv = NULL;
	}
}

module_init(ssboot_basic_optimizer_init_module);
module_exit(ssboot_basic_optimizer_cleanup_module);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("Basic image optimizer");
MODULE_LICENSE("GPL v2");
