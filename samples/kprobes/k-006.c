/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-006.c - A Kprobe with probe point to the kernel function sys_write.
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

static struct kprobe k_006_kpr;
static int k_006_kr_kprobe_write_cnt;

static int k_count1 = 0;
static int k_count2 = 0;

static void __exit k_006_exit_probe(void)
{
	printk("kernel kprobe_write_cnt is %d \n", k_006_kr_kprobe_write_cnt);
	printk("\nModule exiting from sys_write \n");
	unregister_kprobe(&k_006_kpr);

	if (k_count1 > 0 && k_count2 > 0)
		printk("Test k-006 PASS");
	else
		printk("Test k-006 FAIL");
}

static int k_006_before_hook(struct kprobe *kpr, struct pt_regs *p)
{
	k_count1++;
	return 0;
}

static int k_006_after_hook(struct kprobe *kpr,
			    struct pt_regs *p, unsigned long flags)
{
	k_006_kr_kprobe_write_cnt++;
	k_count2++;
	return 0;
}

static int __init k_006_init_probe(void)
{
	printk("\nInserting the kprobe for sys_write\n");

	/* Registering a kprobe */
	k_006_kpr.pre_handler = (kprobe_pre_handler_t) k_006_before_hook;
	k_006_kpr.post_handler = (kprobe_post_handler_t) k_006_after_hook;

	k_006_kpr.symbol_name = "sys_write";

	if (register_kprobe(&k_006_kpr) < 0) {
		printk("k-006.c: register_kprobe is failed\n");
		return -1;
	}

	return 0;
}

module_init(k_006_init_probe);
module_exit(k_006_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
