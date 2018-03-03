/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  kretp-kp-test-001.c - A kprobe with probe point to the kernel function 'sys_open'.
 *
 *  Copyright 2007 Sony Corp.
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
static int k_count1 = 0;
static int k_count2 = 0;
static int k_count3 = 0;

static const char *probed_func = "sys_open";

/* Return-probe handler: Log the return value from the probed function. */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int retval = regs_return_value(regs);

	printk("%s %d, %s returns %d\n", __FILE__, __LINE__, probed_func, retval);
	printk("%s %d, Stack_dump :\n", __FILE__, __LINE__);
	dump_stack();

	k_count1++;
	return 0;
}

static struct kretprobe my_kretprobe = {
	.handler = ret_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive = 20
};

static struct kprobe k_001_kpr;

static int k_001_before_hook(struct kprobe *k_001_kpr, struct pt_regs *p)
{
	printk("\n%s %d, Stack dump for kprobe pre handler at %p\n", __FILE__, __LINE__, k_001_kpr->addr);
	dump_stack();
	printk("%s %d, The Registers are:\n", __FILE__, __LINE__);
	#if defined(CONFIG_ARM)
	show_allregs(p);
	#endif

	k_count2++;
	return 0;
}

static int k_001_after_hook(struct kprobe *k_001_kpr, struct pt_regs *p, unsigned long flags)
{
	printk("\n%s %d, Stack dump for kprobe post handler at %p\n", __FILE__, __LINE__, k_001_kpr->addr);
	dump_stack();
	printk("%s %d, The Registers are:\n", __FILE__, __LINE__);
	#if defined(CONFIG_ARM)
	show_allregs(p);
	#endif

	k_count3++;
	return 0;
}

static int __init k_001_init_probe(void)
{
	int ret;

	printk("\n%s %d, Inserting the kprobe for sys_open\n", __FILE__, __LINE__);
	/* Registering a kprobe */
	k_001_kpr.pre_handler = (kprobe_pre_handler_t) k_001_before_hook;
	k_001_kpr.post_handler = (kprobe_post_handler_t) k_001_after_hook;
	k_001_kpr.symbol_name = "sys_open";
	if (register_kprobe(&k_001_kpr) < 0) {
		printk("%s %d, k-001.c:register_kprobe is failed\n", __FILE__, __LINE__);
		return -1;
	}
	printk("%s %d, register_kprobe is successful\n", __FILE__, __LINE__);

	printk("\n%s %d, Inserting the kretprobe for sys_open\n", __FILE__, __LINE__);
	my_kretprobe.kp.symbol_name = (char *)probed_func;

	if ((ret = register_kretprobe(&my_kretprobe)) < 0) {
		printk("%s %d, register_kretprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d, Planted return probe for sys_open at %p\n", __FILE__, __LINE__, my_kretprobe.kp.addr);

	return 0;
}

static void __exit k_001_exit_probe(void)
{
	printk("\nModule exiting from sys_open \n");
	unregister_kprobe(&k_001_kpr);

	unregister_kretprobe(&my_kretprobe);
	/* nmissed > 0 suggests that maxactive was set too low. */
	printk("Missed probing %d instances of %s\n", my_kretprobe.nmissed, probed_func);

	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0)
		printk("kretp-kp-test-001 PASS");
	else
		printk("kretp-kp-test-001 FAIL");
}

module_init(k_001_init_probe);
module_exit(k_001_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
