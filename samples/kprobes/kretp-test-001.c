/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  kretp-test-001.c - A Kretprobe with probe point to the kernel function 'sys_open'.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

static const char *probed_func = "sys_open";
static int k_count = 0;

/* Return-probe handler: Log the return value from the probed function. */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int retval = regs_return_value(regs);

	printk("%s %d, %s returns %d\n", __FILE__, __LINE__, probed_func, retval);
	printk("%s %d, Stack_dump :\n", __FILE__, __LINE__);
	dump_stack();
	k_count++;
	return 0;
}

static struct kretprobe my_kretprobe = {
	.handler = ret_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive = 20
};

static int __init kretprobe_init(void)
{
	int ret;
	my_kretprobe.kp.symbol_name = (char *)probed_func;

	if ((ret = register_kretprobe(&my_kretprobe)) < 0) {
		printk("%s, %d, register_kretprobe failed, returned %d\n", __FILE__,
		__LINE__, ret);
		return -1;
	}
	printk("%s %d, Planted return probe at %p\n", __FILE__, __LINE__,
		my_kretprobe.kp.addr);
	return 0;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&my_kretprobe);
	printk("%s %d, kretprobe unregistered\n", __FILE__, __LINE__);
	/* nmissed > 0 suggests that maxactive was set too low. */
	printk("%s %d, Missed probing %d instances of %s\n", __FILE__, __LINE__,
		my_kretprobe.nmissed, probed_func);
	if (k_count > 0)
		printk("kretp-test-001 PASS");
	else
		printk("kretp-test-001 FAIL");
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
