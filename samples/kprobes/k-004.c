/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-004.c - Kprobes with the fault handler for the kernel function
 *  'sys_gettimeofday'.
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
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include "kprobes-test.h"

#if defined(CONFIG_ARM)
extern void show_allregs(struct pt_regs *regs);
#endif
static struct kprobe k_004_kpr;

static int k_count1 = 0;
static int k_count2 = 0;
static int k_count3 = 0;

void k_004_CPY_FROM_USER(struct file *file, char *buf, int len)
{
	char x = 'a';

	if (put_user(x, buf)) {
	printk("put_user : -EFAULT\n");
	}

	printk("CPY_FROM_USER\n");
}

static void __exit k_004_exit_probe(void)
{
	printk("\nModule exiting from sys_gettimeofday \n");
	unregister_kprobe(&k_004_kpr);
	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0)
		printk("k-004 test PASS");
	else
		printk("k-004 test FAIL");
}

static int k_004_before_hook(struct kprobe *k_004_kpr, struct pt_regs *p)
{
	int len = 500;
	struct file *file = NULL;

	printk("\nBefore hook in sys_gettimeofday");
	printk("\nThis is the Kprobe pre \n"
	       "handler for instruction at" "%p\n", k_004_kpr->addr);
	printk("Stack Dump:\n");
	dump_stack();
	printk("The Registers are:\n");
#if defined(CONFIG_ARM)
        show_allregs(p);
#endif
	k_004_CPY_FROM_USER(file, NULL, len);
	k_count1++;
	return 0;
}

static int k_004_after_hook(struct kprobe *k_004_kpr,
			    struct pt_regs *p, unsigned long flags)
{
	printk("\nAfter hook in sys_gettimeofday");
	printk("\nThis is the Kprobe post \n"
	       "handler for instruction at" " %p\n", k_004_kpr->addr);
	printk("Stack Dump:\n");
	dump_stack();
	printk("The Registers are:\n");
#if defined(CONFIG_ARM)
        show_allregs(p);
#endif
	k_count2++;
	return 0;
}

int k_004_fault_probe(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk("\nThis is the Kprobe fault \n"
	       "handler for sys_gettimeodday\n");
	printk("fault_handler:p->addr=0x%p\n", p->addr);
	printk("Stack Dump:\n");
	dump_stack();
	printk("The Registers are:\n");
#if defined(CONFIG_ARM)
        show_allregs(regs);
#endif
	k_count3++;
	return 0;
}

static int __init k_004_init_probe(void)
{
	printk("\nInserting the kprobes  for sys_gettimeofday\n");

	/* Registering a kprobe */
	k_004_kpr.pre_handler = (kprobe_pre_handler_t) k_004_before_hook;
	k_004_kpr.post_handler = (kprobe_post_handler_t) k_004_after_hook;
	k_004_kpr.fault_handler = (kprobe_fault_handler_t) k_004_fault_probe;
	k_004_kpr.symbol_name = "sys_gettimeofday";
	if (register_kprobe(&k_004_kpr) < 0) {
		printk("k-004.c: register_kprobe is failed\n");
		return -1;
	}

	printk("register_kprobe is successful\n");
	printk("\nAddress where the kprobe is \n"
	       "going to be inserted - %p\n", k_004_kpr.addr);
	return 0;
}

module_init(k_004_init_probe);
module_exit(k_004_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
