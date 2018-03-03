/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-007.c - Kprobes with probe points to the kernel functions __kmalloc and
 *  kfree.
 *
 *  Copyright 2006,2007 Sony Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

static struct kprobe k_007_kp, k_007_kp1;
int k_007_kmalloc_count = 0;
int k_007_kfree_count = 0;

static int k_count1 = 0;
static int k_count2 = 0;

static int k_007_kmalloc_hndlr(struct kprobe *kpr, struct pt_regs *p)
{
	k_007_kmalloc_count++;
	k_count1++;
	return 0;
}

static int k_007_kfree_hndlr(struct kprobe *kpr, struct pt_regs *p)
{
	k_007_kfree_count++;
	k_count2++;
	return 0;
}

static int __init k_007_kmf_init(void)
{
	k_007_kp.pre_handler = k_007_kmalloc_hndlr;
	k_007_kp1.pre_handler = k_007_kfree_hndlr;

	k_007_kp.symbol_name = "__kmalloc";
	k_007_kp1.symbol_name = "kfree";

	if ((register_kprobe(&k_007_kp) < 0) || (register_kprobe(&k_007_kp1) < 0)) {
		printk("k-007.c: register_kprobe is failed\n");
		return -1;
	}

	return 0;
}

static void __exit k_007_kmf_exit(void)
{
	printk("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n\n\n");
	printk("kmalloc count is %d \n", k_007_kmalloc_count);
	printk("kfree count is %d \n", k_007_kfree_count);
	printk("\n\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
	unregister_kprobe(&k_007_kp);
	unregister_kprobe(&k_007_kp1);
	printk(KERN_INFO "ktime: exiting...\n");

	if (k_count1++ > 0 && k_count2++ > 0)
		printk("Test k-007 PASS");
	else
		printk("Test k-007 FAIL");
}

module_init(k_007_kmf_init);
module_exit(k_007_kmf_exit);
MODULE_LICENSE("GPL");
