/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-001.c - A Kprobe with probe point to the kernel function 'do_fork'.
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
#include "kprobes-test.h"

#if defined(CONFIG_ARM)
extern void show_allregs(struct pt_regs *regs);
#endif
static struct kprobe k_001_kpr;

static int k_count1 = 0;
static int k_count2 = 0;

static void __exit k_001_exit_probe(void)
{
	printk("\nModule exiting from do_fork \n");
	unregister_kprobe(&k_001_kpr);
	if (k_count1 > 0 && k_count2 > 0)
		printk("Test k001 PASS");
	else
		printk("Test k001 FAIL");
}

static int k_001_before_hook(struct kprobe *k_001_kpr, struct pt_regs *p)
{
	printk("\nBefore hook in do_fork");
	printk("\nThis is the Kprobe pre \n"
	       "handler for instruction at %p\n", k_001_kpr->addr);
	printk("Stack Dump :\n");
	dump_stack();
	printk("The Registers are:\n");
#if defined(CONFIG_ARM)
        show_allregs(p);
#endif
	k_count1++;
	return 0;
}

static int k_001_after_hook(struct kprobe *k_001_kpr,
			    struct pt_regs *p, unsigned long flags)
{
	printk("\nAfter hook in do_fork");
	printk("\nThis is the Kprobe post \n"
	       "handler for instruction at" " %p\n", k_001_kpr->addr);
	printk("Stack Dump :\n");
	dump_stack();
	printk("The Registers are:\n");
#if defined(CONFIG_ARM)
        show_allregs(p);
#endif
	k_count2++;
	return 0;
}

static int __init k_001_init_probe(void)
{
	printk("\nInserting the kprobe for do_fork\n");

	/* Registering a kprobe */
	k_001_kpr.pre_handler = (kprobe_pre_handler_t) k_001_before_hook;
	k_001_kpr.post_handler = (kprobe_post_handler_t) k_001_after_hook;
	k_001_kpr.symbol_name = "do_fork";
	if (register_kprobe (&k_001_kpr) < 0) {
		printk("k-001.c:register_kprobe is failed\n");
		return -1;
	}
	printk("register_kprobe is successful\n");
	return 0;
}

module_init(k_001_init_probe);
module_exit(k_001_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
