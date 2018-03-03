/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-003.c - Multiple kprobes with the different probe point for the kernel
 *  function 'do_fork' and 'sys_gettimeofday'.
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

static struct kprobe k_003_kp1;
static struct kprobe k_003_kp2;

static int k_count1 = 0;
static int k_count2 = 0;
static int k_count3 = 0;
static int k_count4 = 0;

static void __exit k_003_exit_probe(void)
{
	printk("\nModule exiting");
	printk("\n2 kprobes from do-fork and sys_gettimeofday\n");
	unregister_kprobe(&k_003_kp2);
	unregister_kprobe(&k_003_kp1);

	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0 && k_count4 > 0)
		printk("test k-003 PASS");
	else
		printk("test k-003 FAIL");
}

static int k_003_before_hook(struct kprobe *k_003_kp1, struct pt_regs *p)
{
	printk("\nBefore hook in do_fork\n");
	k_count1++;
	return 0;
}

static int k_003_after_hook(struct kprobe *k_003_kp1,
			    struct pt_regs *p, unsigned long flags)
{
	printk("\nAfter hook in do_fork\n");
	k_count2++;
	return 0;
}

static int k_003_pre_handler(struct kprobe *k_003_kp2, struct pt_regs *p)
{
	printk("\nBefore hook in sys_gettimeofday\n");
	k_count3++;
	return 0;
}

static int k_003_post_handler(struct kprobe *k_003_kp2,
			      struct pt_regs *p, unsigned long flags)
{
	printk("\nAfter hook in sys_gettimeofday\n");
	k_count4++;
	return 0;
}

static int __init k_003_init_probe(void)
{
	printk("\nInserting two kprobes at differnt probe point\n");

	/* Registering a kprobe */
	k_003_kp1.pre_handler = (kprobe_pre_handler_t) k_003_before_hook;
	k_003_kp1.post_handler = (kprobe_post_handler_t) k_003_after_hook;

	k_003_kp2.pre_handler = (kprobe_pre_handler_t) k_003_pre_handler;
	k_003_kp2.post_handler = (kprobe_post_handler_t) k_003_post_handler;

	k_003_kp1.symbol_name = "do_fork";
	k_003_kp2.symbol_name = "sys_gettimeofday";

	if ((register_kprobe(&k_003_kp1) < 0) || (register_kprobe(&k_003_kp2) < 0)) {
 		printk("k-003: register_kprobe is failed\n");
		return -1;
	}

	return 0;
}

module_init(k_003_init_probe);
module_exit(k_003_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
