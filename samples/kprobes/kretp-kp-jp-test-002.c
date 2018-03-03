/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  kretp-kp-jp-test-002.c - A kretprobe, kprobe and jprobe with probe point to the
 *  kernel function 'sys_close'.
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
static int k_count4 = 0;

/*
 * Jumper probe for sys_close.
 * Mirror principle enables access to arguments of the probed routine
 * from the probe handler.
 */

/* Proxy routine having the same arguments as actual sys_close() routine */
long jsys_close(unsigned int fd, char __user * buf, size_t count)
{
	printk("%s %d, Proxy sys_close, arguments are %d, %d\n", __FILE__, __LINE__, fd, count);
	printk("%s %d, Stack_dump :\n", __FILE__, __LINE__);
	dump_stack();
	/* Always end with a call to jprobe_return(). */
	k_count1++;
	jprobe_return();
	/* NOTREACHED */
	return 0;
}

static struct jprobe my_jprobe = {
	.entry = JPROBE_ENTRY(jsys_close)
};

static const char *probed_func = "sys_close";

/* Return-probe handler: Log the return value from the probed function. */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int retval = regs_return_value(regs);

	printk("%s %d, %s returns %d\n", __FILE__, __LINE__, probed_func, retval);
	printk("%s %d, Stack_dump :\n", __FILE__, __LINE__);
	dump_stack();

	k_count2++;
	return 0;
}

static struct kretprobe my_kretprobe = {
	.handler = ret_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive = 20
};

static struct kprobe k_002_kpr;

static int k_002_before_hook(struct kprobe *k_002_kpr, struct pt_regs *p)
{
	printk("%s %d\nStack dump for the kprobe pre handler for instruction at %p\n", __FILE__, __LINE__, k_002_kpr->addr);
	dump_stack();
	printk("%s %d, The Registers are:\n", __FILE__, __LINE__);
	#if defined(CONFIG_ARM)
	show_allregs(p);
	#endif

	k_count3++;
	return 0;
}

static int k_002_after_hook(struct kprobe *k_002_kpr, struct pt_regs *p, unsigned long flags)
{
	printk("%s %d\nStack dump for the kprobe post handler at %p\n", __FILE__, __LINE__, k_002_kpr->addr);
	dump_stack();
	printk("%s %d, The Registers are:\n", __FILE__, __LINE__);
	#if defined(CONFIG_ARM)
	show_allregs(p);
	#endif

	k_count4++;
	return 0;
}

static int __init k_002_init_probe(void)
{
	int ret;
	int retj;
	printk("%s %d\nInserting the kprobe for sys_close\n", __FILE__, __LINE__);

	/* Registering a kprobe */
	k_002_kpr.pre_handler = (kprobe_pre_handler_t) k_002_before_hook;
	k_002_kpr.post_handler = (kprobe_post_handler_t) k_002_after_hook;
	k_002_kpr.symbol_name = "sys_close", __FILE__, __LINE__;
	if (register_kprobe(&k_002_kpr) < 0) {
		printk("%s %dk-002.c:register_kprobe is failed\n", __FILE__, __LINE__);
		return -1;
	}
	printk("%s %d, register_kprobe is successful\n", __FILE__, __LINE__);

	printk("%s %d, Inserting the kretprobe for sys_close\n", __FILE__, __LINE__);
	my_kretprobe.kp.symbol_name = (char *)probed_func;

	if ((ret = register_kretprobe(&my_kretprobe)) < 0) {
		printk("%s %d, register_kretprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d, Planted return probe for sys_close at %p\n", __FILE__, __LINE__, my_kretprobe.kp.addr);

	my_jprobe.kp.symbol_name = "sys_close";

	if ((retj = register_jprobe(&my_jprobe)) < 0) {
		printk("%s %d,register_jprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d,Planted jprobe at %p, handler addr %p\n", __FILE__, __LINE__, my_jprobe.kp.addr, my_jprobe.entry);

	return 0;
}

static void __exit k_002_exit_probe(void)
{
	unregister_kprobe(&k_002_kpr);
	printk("%s %d\nkprobe unregistered from sys_close \n", __FILE__, __LINE__);

	unregister_kretprobe(&my_kretprobe);
	printk("%s %dkretprobe unregistered\n", __FILE__, __LINE__);
	/* nmissed > 0 suggests that maxactive was set too low. */
	printk("%s %dMissed probing %d instances of %s\n", __FILE__, __LINE__, my_kretprobe.nmissed, probed_func);

	unregister_jprobe(&my_jprobe);
	printk("%s %d,jprobe unregistered\n", __FILE__, __LINE__);

	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0 && k_count4 > 0)
		printk("kretp-kp-jp-test-002 PASS");
	else
		printk("kretp-kp-jp-test-002 FAIL");
}

module_init(k_002_init_probe);
module_exit(k_002_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
