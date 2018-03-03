/* 2013-08-20: File added and changed by Sony Corporation */
/*
 *  jp-test-002.c - A Jprobe with probe point to the kernel function
 *  'do_gettimeofday'.
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

/*
 * Jumper probe for do_gettimeofday.
 * Mirror principle enables access to arguments of the probed routine
 * from the probe handler.
 */

static int k_count = 0;

/* Proxy routine having the same arguments as actual do_gettimeofday() routine */
long jdo_gettimeofday(struct timeval *tv)
{
	printk("%s %d,jprobe: in do_gettimeofday\n", __FILE__, __LINE__);
	printk("%s %d,Stack dump : \n", __FILE__, __LINE__);
	dump_stack();
	/* Always end with a call to jprobe_return(). */
	k_count++;
	jprobe_return();
	/*NOTREACHED*/
	return 0;
}

static struct jprobe my_jprobe = {
	.entry = JPROBE_ENTRY(jdo_gettimeofday)
};

static int __init jprobe_init(void)
{
	int ret;
	my_jprobe.kp.symbol_name = "do_gettimeofday";

	if ((ret = register_jprobe(&my_jprobe)) < 0) {
		printk("%s %d,register_jprobe failed, returned %d\n", __FILE__, __LINE__, ret);
		return -1;
	}
	printk("%s %d,Planted jprobe at %p, handler addr %p\n", __FILE__, __LINE__,
		my_jprobe.kp.addr, my_jprobe.entry);
	return 0;
}

static void __exit jprobe_exit(void)
{
	unregister_jprobe(&my_jprobe);
	printk("%s %d,jprobe unregistered\n", __FILE__, __LINE__);

	if (k_count > 0)
		printk("jp-test-002 PASS");
	else
		printk("jp-test-002 FAIL");
}

module_init(jprobe_init)
module_exit(jprobe_exit)
MODULE_LICENSE("GPL");

