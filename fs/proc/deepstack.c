/* 2012-03-15: File added by Sony Corporation */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

#ifdef CONFIG_SNSC
extern int __init nlver_init(void);
#endif

/* use a value that would overrun a default stack size (8k) */
#define TEST_STACK_DEPTH 7129
/* use a value that will stretch a default stack to it's max */
//#define TEST_STACK_DEPTH 3973

/* define the following to avoid any calls to sub-routines
 * in deepstack_proc_show()
 * This is used to test whether the ftrace stack tracer can
 * correctly detect the size of stack usage for leaf functions
 *
 * Comment the following line out, to allow calling sub-routines
 * and to printk some extra debug information.
 */
//#define NO_SUBROUTINES 1

static int deepstack_proc_show(struct seq_file *m, void *v)
{
	int i;
	unsigned long sp;
	char volatile buffer[TEST_STACK_DEPTH];

	/*
	 * The volatile and the following assignment
	 * force the compiler to actually put the buffer and code in.
	 * Without these, the buffer and loop get optimized away
	 */
	v = (void *)buffer;

	/* walk down a huge data structure on stack
	* this should cause the kernel to fault, if
	* CONFIG_SNSC_STACK_LIMIT is set, and 'unmap_stack'
 	* is passed to the kernel on boot
  	*/
	asm("mov %0, sp" : "=r"(sp));
#ifndef NO_SUBROUTINES
	printk("sp=0x%08lx\n", sp);
	printk("current thread_info = 0x%08lx\n",
		(unsigned long)current_thread_info());
#endif

	for( i=TEST_STACK_DEPTH-1; i>0; i -= 1) {
#ifndef NO_SUBROUTINES
		if (i%100==0) {
			printk("&buffer[i]=%p\n", &buffer[i]);
		}
#endif
		buffer[i] = 'd'+i;
	}
#ifndef NO_SUBROUTINES
	printk("sp=0x%08lx\n", sp);
	printk("current thread_info = 0x%08lx\n",
		(unsigned long)current_thread_info());

	seq_printf(m, "how did I get here - I should have crashed!!\n"
	"Did you set CONFIG_SNSC_STACK_LIMIT and give the kernel the\n"
	"'unmap_stack' command?\n");
#endif
	return 0;
}

static int deepstack_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, deepstack_proc_show, NULL);
}

static const struct file_operations deepstack_proc_fops = {
	.open		= deepstack_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_deepstack_init(void)
{
	proc_create("deepstack", 0, NULL, &deepstack_proc_fops);
	return 0;
}
module_init(proc_deepstack_init);
