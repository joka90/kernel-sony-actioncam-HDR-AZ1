/* 2011-10-03: File added and changed by Sony Corporation */
/*
 *  linux/fs/proc/kstack.c
 *
 *  proc interface to list kernel stack usage
 *
 *  Copyright 2010 Sony Corporation
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/nmi.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/semaphore.h>

static int __read_mostly __kstack_enabled = 0;

struct kstack_log_data {
	struct list_head list;
	pid_t pid;
	pid_t tgid;
	char *comm;
	unsigned long free;
	unsigned long long time;
};
static LIST_HEAD(kstack_log_head);

static DEFINE_SEMAPHORE(kstack_log_mutex);

/*
 * Call this function to log the free stack size when a process exits.
 * This function is called from check_stack_usage() in kernel/exit.c
 */
void kstack_log(const struct task_struct *p, unsigned long free)
{
	struct kstack_log_data *d;
	int comm_len;

	if (!__kstack_enabled)
		return;

	d = kmalloc(sizeof(*d), GFP_KERNEL);
	if (d == NULL)
		return;
	comm_len = strlen(p->comm);
	d->comm = kmalloc(comm_len + 1, GFP_KERNEL);
	if (d->comm == NULL)
	{
		kfree(d);
		return;
	}

	strncpy(d->comm, p->comm, comm_len + 1);
	d->pid = p->pid;
	d->tgid = p->tgid;
	d->free = free;
	d->time = sched_clock();

	/* add to list */
	down(&kstack_log_mutex);
	list_add_tail(&d->list, &kstack_log_head);
	up(&kstack_log_mutex);
}

static void kstack_clear(void)
{
	struct kstack_log_data *d, *n;

	down(&kstack_log_mutex);
	list_for_each_entry_safe(d, n, &kstack_log_head, list) {
		kfree(d->comm);
		kfree(d);
	}
	INIT_LIST_HEAD(&kstack_log_head);
	up(&kstack_log_mutex);
}

static int kstack_proc_show(struct seq_file *m, void *v)
{
	struct kstack_log_data *d;
	struct task_struct *g, *p;

	if (__kstack_enabled)
		seq_printf(m, "kstack is on\n\n");
	else
		seq_printf(m, "kstack is off\n\n");


	seq_printf(m, "exited processes:\n");
	seq_printf(m, "                 TIME   PID  TGID UNUSED-STACK COMMAND\n");

	down(&kstack_log_mutex);
	list_for_each_entry(d, &kstack_log_head, list) {
		unsigned long long t = d->time;
		unsigned long nsec = do_div(t, 1000000000);

		seq_printf(m, "%11llu.%09lu %5d %5d %12lu %-32s\n", t, nsec, d->pid, d->tgid, d->free, d->comm);
	}
	up(&kstack_log_mutex);


	seq_printf(m, "\n");
	seq_printf(m, "live processes:\n");
	seq_printf(m, "  PID  TGID UNUSED-STACK COMMAND\n");

	/* copied from kernel/sched.c */
	read_lock(&tasklist_lock);
	do_each_thread(g, p) {
		unsigned long free;
		unsigned long *stackend;
		int magic;

		stackend = end_of_stack(p);
		magic = (STACK_END_MAGIC == *stackend);
		free = stack_not_used(p);
		seq_printf(m, "%5d %5d %12lu %-32s %c\n", p->pid, p->tgid, free, p->comm, (magic)?' ':'*');
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);

	return 0;
}

extern int debug_stack_usage;

static int kstack_set(char opt)
{
	switch (opt) {
	case '0':
	case 'n': case 'N':
		__kstack_enabled = 0;
		break;
	case '1':
	case 'y': case 'Y':
		__kstack_enabled = 1;
		debug_stack_usage = 1;
		break;
	case 'c': case 'C':
	case 'r': case 'R':
		kstack_clear();
		break;
	default:
		return 0;
	}
	return 1;
}

static ssize_t kstack_proc_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	char c;

	if (!count)
		return 0;

	if (get_user(c, buf))
		return -EFAULT;
	if (!kstack_set(c))
		return -EFAULT;
	return count;
}

static int kstack_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, kstack_proc_show, NULL);
}

static const struct file_operations kstack_proc_fops = {
	.open		= kstack_proc_open,
	.read		= seq_read,
	.write		= kstack_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init kstack_init(void)
{
	proc_create("kstack", 0644, NULL, &kstack_proc_fops);
	return 0;
}

static int __init kstack_setup(char *line)
{
	kstack_set(*line);
	return 1;
}

late_initcall(kstack_init);
__setup("kstack=", kstack_setup);
