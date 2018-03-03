/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  kretp-kp-test-004.c - A kretprobe and kprobe with probe point to the
 *  kernel function sys_write.
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

static const char *probed_func = "sys_write";
static struct kprobe k_004_kpr;
static int k_004_kr_kprobe_write_cnt;
static int k_004_kr_kretprobe_write_cnt;

static int k_count1 = 0;
static int k_count2 = 0;
static int k_count3 = 0;

/* Return-probe handler: Log the return value from the probed function. */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	k_004_kr_kretprobe_write_cnt++;
	k_count1++;
	return 0;
}

static struct kretprobe my_kretprobe = {
	.handler = ret_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive = 20
};

static void __exit k_004_exit_probe(void)
{
	printk("%s %d kernel kprobe_write_cnt is %d \n", __FILE__, __LINE__, k_004_kr_kprobe_write_cnt);
	printk("%s %d kernel kretprobe_write_cnt is %d \n", __FILE__, __LINE__, k_004_kr_kretprobe_write_cnt);
	printk("%s %d Module exiting from sys_write \n", __FILE__, __LINE__);
	unregister_kprobe(&k_004_kpr);
	printk("%s %d,kprobe unregistered\n", __FILE__, __LINE__);
	unregister_kretprobe(&my_kretprobe);
	printk("%s %d,kretprobe unregistered\n", __FILE__, __LINE__);

	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0)
		printk("kretp-kp-test-004 PASS");
	else
		printk("kretp-kp-test-004 FAIL");
}

static int k_004_before_hook(struct kprobe *kpr, struct pt_regs *p)
{
	k_count2++;
	return 0;
}

static int k_004_after_hook(struct kprobe *kpr,
		        struct pt_regs *p, unsigned long flags)
{
	k_004_kr_kprobe_write_cnt++;
	k_count3++;
	return 0;
}

static int __init k_004_init_probe(void)
{
	int ret;
	printk("%s %d, Inserting the kprobe for sys_write\n", __FILE__, __LINE__);

	/* Registering a kprobe */
	k_004_kpr.pre_handler = (kprobe_pre_handler_t) k_004_before_hook;
	k_004_kpr.post_handler = (kprobe_post_handler_t) k_004_after_hook;

	k_004_kpr.symbol_name = "sys_write";

	if (register_kprobe(&k_004_kpr) < 0) {
		printk("%s, %d, register_kprobe is failed\n", __FILE__, __LINE__);
		return -1;
	}

	my_kretprobe.kp.symbol_name = (char *)probed_func;

	if ((ret = register_kretprobe(&my_kretprobe)) < 0) {
		printk("%s %d,register_kretprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d,Planted return probe at %p\n", __FILE__, __LINE__, my_kretprobe.kp.addr);

	return 0;
}

module_init(k_004_init_probe);
module_exit(k_004_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
