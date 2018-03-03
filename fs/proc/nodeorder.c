/* 2010-10-15: File added by Sony Corporation */
#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <asm/atomic.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "internal.h"

extern int nodeorder_proc_show(struct seq_file *m, void *v);

static int nodeorder_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nodeorder_proc_show, NULL);
}

static const struct file_operations nodeorder_proc_fops = {
	.open		= nodeorder_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_nodeorder_init(void)
{
	proc_create("nodeorder", 0, NULL, &nodeorder_proc_fops);
	return 0;
}
module_init(proc_nodeorder_init);
