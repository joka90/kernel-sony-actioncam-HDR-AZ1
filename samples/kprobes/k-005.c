/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-005.c - A Kprobe with probe point to the kernel function sys_read.
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

static struct kprobe k_005_kpr;
static int k_005_kr_kprobe_read_cnt;

static int k_count1 = 0;
static int k_count2 = 0;

static void __exit k_005_exit_probe(void)
{
	printk("kernel kprobe_read_cnt is %d \n", k_005_kr_kprobe_read_cnt);
	printk("\nModule exiting from sys_read \n");
	unregister_kprobe(&k_005_kpr);
	if (k_count1 > 0 && k_count2 > 0)
		printk("Test k-005 PASS");
	else
		printk("Test k-005 FAIL");
}

static int k_005_before_hook(struct kprobe *kpr, struct pt_regs *p)
{
	k_count1++;
	return 0;
}

static int k_005_after_hook(struct kprobe *kpr,
			    struct pt_regs *p, unsigned long flags)
{
	k_005_kr_kprobe_read_cnt++;
	k_count2++;
	return 0;
}

static int __init k_005_init_probe(void)
{
	printk("\nInserting the kprobe for sys_read\n");

	/* Registering a kprobe */
	k_005_kpr.pre_handler = (kprobe_pre_handler_t) k_005_before_hook;
	k_005_kpr.post_handler = (kprobe_post_handler_t) k_005_after_hook;

	k_005_kpr.symbol_name = "sys_read";

	if (register_kprobe(&k_005_kpr) < 0 ) {
		printk("k-005.c: register_kprobe is failed\n");
		return -1;
	}

	return 0;
}

module_init(k_005_init_probe);
module_exit(k_005_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
