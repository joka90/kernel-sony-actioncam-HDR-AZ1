/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  jp-kp-test-004.c - A Jprobe and a Kprobe with probe point to the kernel
 *  function 'sys_read'.
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
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/kprobes.h>
#include "kprobes-test.h"

/*
 * Jumper probe and kprobe for sys_read.
 * Mirror principle enables access to arguments of the probed routine
 * from the probe handler.
 */
static int k_count1 = 0;
static int k_count2 = 0;
static int k_count3 = 0;

#if defined(CONFIG_ARM)
extern void show_allregs(struct pt_regs *regs);
#endif

static struct kprobe kp;

int kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	printk("%s %d,kprobe pre-handler for sys_read \n", __FILE__, __LINE__);
	printk("%s %d,Stack dump : \n", __FILE__, __LINE__);
	dump_stack();
#if defined(CONFIG_ARM)
	printk("%s %d,The Registers are : \n", __FILE__, __LINE__);
	show_allregs(regs);
#endif
	k_count1++;
	return 0;
}

void kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	printk("%s %d,kprobe post-handler for sys_read \n", __FILE__, __LINE__);
	printk("%s %d,Stack dump : \n", __FILE__, __LINE__);
	dump_stack();
#if defined(CONFIG_ARM)
	printk("%s %d,The Registers are : \n", __FILE__, __LINE__);
	show_allregs(regs);
#endif
	k_count2++;
}

/* Proxy routine having the same arguments as actual sys_read() routine */
long jsys_read(unsigned int fd, char __user * buf, size_t count)
{
	/* Always end with a call to jprobe_return(). */
	k_count3++;
	jprobe_return();
	/* NOTREACHED */
	return 0;
}

static struct jprobe my_jprobe = {
	.entry = JPROBE_ENTRY(jsys_read)
};

static int __init jprobe_kprobe_init(void)
{
	int ret;
	my_jprobe.kp.symbol_name = "sys_read";

	if ((ret = register_jprobe(&my_jprobe)) <0) {
		printk("%s %d,register_jprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d,Planted jprobe at %p, handler addr %p\n", __FILE__, __LINE__,
		my_jprobe.kp.addr, my_jprobe.entry);

	kp.symbol_name = "sys_read";
	kp.pre_handler = kprobe_pre_handler;
	kp.post_handler = kprobe_post_handler;

	if ((ret = register_kprobe(&kp)) <0) {
		printk("%s %d,register_jprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}

	return 0;
}

static void __exit jprobe_kprobe_exit(void)
{
	unregister_jprobe(&my_jprobe);
	printk("%s %d,jprobe unregistered\n", __FILE__, __LINE__);
	unregister_kprobe(&kp);
	printk("%s %d,kprobe unregistered\n", __FILE__, __LINE__);

	if (k_count1 > 0 && k_count2 > 0 && k_count3 > 0)
		printk("jp-kp-test-004 PASS");
	else
		printk("jp-kp-test-004 FAIL");
}

module_init(jprobe_kprobe_init)
module_exit(jprobe_kprobe_exit)
MODULE_LICENSE("GPL");
