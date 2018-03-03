/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-009.c - A Kprobe with a probe point to the kernel function
 *  do_gettimeofday, with the empty pre-handler and post-handler
 *  which can be used to measure the Kprobes overhead.
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

static struct kprobe k_009_kp1;

static int k_count1 = 0;
static int k_count2 = 0;

static void __exit k_009_exit_probe(void)
{
	printk("\nModule exiting ");
	printk("from gettimeofday\n");
	unregister_kprobe(&k_009_kp1);

	if (k_count1 > 0 && k_count2 > 0)
		printk("Test k-009 PASS");
	else
		printk("Test k-009 FAIL");
}

static int k_009_pre_handler(struct kprobe *k_009_kp1, struct pt_regs *p)
{
	k_count1++;
	return 0;
}

static int k_009_post_handler(struct kprobe *k_009_kp1,
			      struct pt_regs *p, unsigned long flags)
{
	k_count2++;
	return 0;
}

static int __init k_009_init_probe(void)
{
	printk("\nInserting the kprobe at do_gettimeofday\n");

	/* Registering a kprobe */
	k_009_kp1.pre_handler = (kprobe_pre_handler_t) k_009_pre_handler;
	k_009_kp1.post_handler = (kprobe_post_handler_t) k_009_post_handler;

#if 0
	k_009_kp1.addr =
	    (kprobe_opcode_t *) kallsyms_lookup_name("do_gettimeofday");

	if (k_009_kp1.addr == NULL) {
		printk("kallsyms_lookup_name could not find address "
		       "for the specified symbol name\n");
		return 1;
	}

	printk("\nAddress where the kprobes are \n"
	       "going to be inserted - %p \n", k_009_kp1.addr);
#endif

	k_009_kp1.symbol_name = "do_gettimeofday";

	if (register_kprobe(&k_009_kp1) < 0) {
		printk("k-009.c: register_kprobe is failed\n");
		return -1;
	}

	register_kprobe(&k_009_kp1);

	return 0;
}

module_init(k_009_init_probe);
module_exit(k_009_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
