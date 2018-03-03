/* 2013-05-15: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot - Image Contents Debug Output writer
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

#include <linux/version.h>
#include <linux/module.h>
#include "internal.h"

#include <linux/mm.h>
#include <linux/pfn.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/rmap.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <asm/page.h>
#include <asm/sections.h>

static char page_attr_dump_fname[SSBOOT_PATH_MAX];
static char print_buf[1024];

/* for file access */
static struct file *filp;
static mm_segment_t oldfs;
static loff_t pos = 0;

static int file_open(void)
{
	/* open file */
	filp = filp_open(page_attr_dump_fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (IS_ERR(filp)) {
		ssboot_err("cannot open file: %s\n", page_attr_dump_fname);
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
	/* reset pos */
	pos = 0;

	/* close file */
	fput(filp);
	filp_close(filp, current->files);

	/* restore setting */
	set_fs(oldfs);
}

static int do_print(const char *fmt, ...)
{
	int r;
	va_list args;
	loff_t p = pos;

	memset(print_buf, 0, sizeof(print_buf));

	va_start(args, fmt);
	r = vsnprintf(print_buf, sizeof(print_buf), fmt, args);
	va_end(args);

	vfs_write(filp, print_buf, r, &p);
	pos += r;

	return r;
}

static void write_pid_from_mm(struct mm_struct *mm)
{
	struct task_struct *p;
	int once_find_pid = 0;


	for_each_process(p) {
		if (mm != p->mm)
			continue;

		/* matched */
		if (once_find_pid) {
			do_print(", ");
		} else {
			do_print(" pid: [");
			once_find_pid = 1;
		}

		do_print(" %u", p->pid);
		if (p->comm)
			do_print("(%s)", p->comm);
	}
	if (once_find_pid)
		do_print(" ]");
}

static inline void write_info_from_inode(struct inode *inode)
{
	struct dentry *dentry;

	do_print(", mode: 0%o", inode->i_mode);

	if (S_ISBLK(inode->i_mode) && inode->i_bdev)
		do_print(", dev: %d:%d",
			 MAJOR(inode->i_bdev->bd_dev),
			 MINOR(inode->i_bdev->bd_dev));

	/* TODO: extract file from only first dentry */
	list_for_each_entry(dentry, &inode->i_dentry, d_alias) {
		struct qstr *qstr = &dentry->d_name;

		do_print(", file: %s", qstr->name);
	}
}

static void write_pagecache_page(struct page *page)
{
	struct address_space *mapping = page->mapping;
	struct vm_area_struct *vma;
	struct prio_tree_iter iter;
	pgoff_t pgoff = page->index << (PAGE_CACHE_SHIFT - PAGE_SHIFT);
	int once_find_vma = 0;

	/*
	 * "mapping" of removed page cache is NULL. Removed page cache
	 * may still stay in LRU as the page is referenced.
	 */
	if (!mapping)
		return;

	/* filename, file offset and so on */
	write_info_from_inode(mapping->host);
	do_print(", offs: 0x%llx", (loff_t)(page->index << PAGE_SHIFT));

	/* pid, comm, virtual address */
	vma_prio_tree_foreach(vma, &iter, &mapping->i_mmap, pgoff, pgoff) {

		struct mm_struct *mm = vma->vm_mm;
		unsigned long address = page_address_in_vma(page, vma);

		/* pid */
		if (address == -EFAULT) {
			continue;
		}

		if (!once_find_vma) {
			do_print(", vma: [ {");
			once_find_vma = 1;
		} else {
			do_print(", {");
		}

		write_pid_from_mm(mm);
		do_print(", virt: 0x%08lx }", address);
	}
	if (once_find_vma)
		do_print(" ]");
}

static void write_anon_page(struct page *page)
{
	int once_find_vma = 0;
	struct anon_vma *anon_vma;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,33))
	struct anon_vma_chain *avc;
#else
	struct vm_area_struct *vma;
#endif

	unsigned long anon_mapping = (unsigned long) (page->mapping);
	anon_vma = (struct anon_vma *) (anon_mapping - PAGE_MAPPING_ANON);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,33))
	list_for_each_entry(avc, &anon_vma->head, same_anon_vma) {
		struct vm_area_struct *vma = avc->vma;
#else
 	list_for_each_entry(vma, &anon_vma->head, anon_vma_node) {
#endif

		unsigned long address = page_address_in_vma(page, vma);
		struct mm_struct *mm = vma->vm_mm;

		if (address == -EFAULT) {
			continue;
		}

		if (!once_find_vma) {
			do_print(", vma: [ {");
			once_find_vma = 1;
		} else {
			do_print(", {");
		}

		write_pid_from_mm(mm);
		do_print(", virt: %08lx }", address);
	}

	if (once_find_vma)
		do_print(" ]");
}

static void write_normal_page(struct page *page)
{
	if (PageSwapCache(page)) {
		do_print(", type: swpcache");
	} else if (PageAnon(page)) {
		do_print(", type: anon");
		write_anon_page(page);
	} else if (PageLRU(page)) {
		do_print(", type: pgcache");
		write_pagecache_page(page);
	} else {
		do_print(", type: kernel");
	}
	return;
}

static int ssboot_page_attr_dump_prepare(void *priv)
{
	if (strlen(page_attr_dump_fname) == 0) {
		strncpy(page_attr_dump_fname,
			CONFIG_SNSC_SSBOOT_PAGE_ATTR_DUMP_FILE_NAME,
			SSBOOT_PATH_MAX);
		page_attr_dump_fname[SSBOOT_PATH_MAX - 1] = '\0';
	}

	ssboot_invalidate_page_cache(page_attr_dump_fname);
	return 0;
}

static int ssboot_page_attr_dump_cleanup(void *priv)
{
	return 0;
}


static int ssboot_page_attr_dump_write(ssboot_image_t *image, void *priv)
{
	unsigned long pfn;
	int i;

	unsigned long num_section;
	ssboot_section_t *section;

	num_section = image->num_section;
	section     = image->section;

	/* file open */
	if (file_open())
		return -EIO;

	for (i=0; i < num_section; i++) {

		/* check if section is in CRITICAL section */
		if (section->attr & SSBOOT_SECTION_CRITICAL) {
			for (pfn = section->start_pfn;
			     pfn < section->start_pfn + section->num_pages;
			     pfn++) {
				do_print("{ pfn: 0x%05lx, type: kernel }\n", pfn);
			}
			section++;
			continue;
		}

		/* here NORMAL page */
		for (pfn = section->start_pfn;
		     pfn < section->start_pfn + section->num_pages;
		     pfn++) {
			do_print("{ pfn: 0x%05lx", pfn);
			write_normal_page(pfn_to_page(pfn));
			do_print(" }\n");
		}
		section++;
	}

	/* file close */
	file_close();

	return 0;
}

/*
 * proc I/F
 */
static int
page_attr_dump_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
page_attr_dump_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", page_attr_dump_fname);
	return 0;
}

static int
page_attr_dump_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* set filename */
	strncpy(page_attr_dump_fname, ops->write_buf, SSBOOT_PATH_MAX);
	page_attr_dump_fname[SSBOOT_PATH_MAX - 1] = '\0';

	return 0;
}
ssboot_single_proc(page_attr_dump, SSBOOT_PROC_RDWR, SSBOOT_PATH_MAX);

/*
 * Initialization
 */
static ssboot_writer_t ssboot_page_attr_dump_writer = {
	.prepare	= ssboot_page_attr_dump_prepare,
	.write		= ssboot_page_attr_dump_write,
	.cleanup	= ssboot_page_attr_dump_cleanup,
};

static int __init
ssboot_page_attr_dump_init_module(void)
{
	int ret;

	strncpy(page_attr_dump_fname,
		CONFIG_SNSC_SSBOOT_PAGE_ATTR_DUMP_FILE_NAME,
		SSBOOT_PATH_MAX);
	page_attr_dump_fname[SSBOOT_PATH_MAX - 1] = '\0';

	ssboot_proc_create_entry("page_attr_dump",
		&page_attr_dump_ops, ssboot_proc_root);

	/* register page attr dump writer */
	ret = ssboot_writer_register(&ssboot_page_attr_dump_writer);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static void __exit
ssboot_page_attr_dump_cleanup_module(void)
{
	int ret;

	ret = ssboot_writer_unregister(&ssboot_page_attr_dump_writer);
	if (ret < 0) {
		return;
	}
}

module_init(ssboot_page_attr_dump_init_module);
module_exit(ssboot_page_attr_dump_cleanup_module);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("Image Contents Debug Output");
MODULE_LICENSE("GPL v2");
