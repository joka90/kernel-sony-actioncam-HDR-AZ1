/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  k-008.c - A Kprobe with a probe point to the kernel function
 *  do_gettimeofday, with an empty pre-handler which can be used
 *  to measure the Kprobes overhead.
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

static struct kprobe k_008_kp1;
static int k_count = 0;

static void __exit k_008_exit_probe(void)
{
	printk("\nModule exiting ");
	printk("from gettimeofday\n");
	unregister_kprobe(&k_008_kp1);

	if (k_count > 0)
		printk("k-008 PASS");
	else
		printk("k-008 FAIL");
}

static int k_008_pre_handler(struct kprobe *k_008_kp1, struct pt_regs *p)
{
	k_count++;
	return 0;
}

static int __init k_008_init_probe(void)
{
	printk("\nInserting the kprobe at do_gettimeofday\n");

	/* Registering a kprobe */
	k_008_kp1.pre_handler = (kprobe_pre_handler_t) k_008_pre_handler;

#if 0
	k_008_kp1.addr =
	    (kprobe_opcode_t *) kallsyms_lookup_name("do_gettimeofday");

	if (k_008_kp1.addr == NULL) {
		printk("kallsyms_lookup_name could not find address "
		       "for the specified symbol name\n");
		return 1;
	}

	printk("\nAddress where the kprobes are \n"
	       "going to be inserted - %p \n", k_008_kp1.addr);
#endif

	k_008_kp1.symbol_name = "do_gettimeofday";

	if (register_kprobe(&k_008_kp1) < 0) {
		printk("k-008.c: register_kprobe is failed\n");
		return -1;
	}

	register_kprobe(&k_008_kp1);

	return 0;
}

module_init(k_008_init_probe);
module_exit(k_008_exit_probe);

MODULE_DESCRIPTION("Kprobes test module");
MODULE_LICENSE("GPL");
