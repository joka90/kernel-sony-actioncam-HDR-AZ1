/* 2012-03-06: File added by Sony Corporation */
/*
 *  Momery Usage Tracer For Each Thread
 *
 *  Copyright 2010 Sony Corporation.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/thread_rss.h>

#define KB(x) ((x) >> (10))

static int is_kernel_thread(struct task_struct *tsk)
{
	return (tsk->flags & PF_KTHREAD);
}

static int __calculate_rss(u64 alloc, u64 free, u64 alloc_ex, u64 free_ex)
{
	int rss;
	s64 alloc_own = alloc - alloc_ex;
	s64 free_own  = free - free_ex;

	/*
	 * rss may be minus
	 */
	rss = (int) (alloc_own - free_own);

	return rss;
}

static int calculate_task_rss(struct task_struct *tsk)
{
	return __calculate_rss(tsk->thread_rss_alloc,
			      tsk->thread_rss_free,
			      tsk->thread_rss_alloc_excluded,
			      tsk->thread_rss_free_excluded);
}

static char *task_comm_display(char *buf, int kernel_thread, char *comm)
{
	int len;

	if (kernel_thread) {
		/* Show kernel thread command to format [comm] */

		len = strlen(comm);
		len = (len > (TASK_COMM_LEN-3)) ? (TASK_COMM_LEN-3) : len;

		/* clear buffer */
		memset(buf, 0, TASK_COMM_LEN);

		buf[0] = '[';
		strncpy(&buf[1], comm, len);
		buf[len+1] = ']';

		return buf;
	} else {
		/* userspace thread, does nothing */
		return comm;
	}
}

#ifdef CONFIG_SNSC_THREAD_RSS_HISTORY
#define NR_ENTRIES    10

int thread_rss_threshold = 8192;  /* 8KB */

struct thread_rss_entry {
	pid_t pid;
	pid_t tgid;
	int   flags;
	int   req;      /* Request Memory */
	u64   thread_rss_alloc;
	u64   thread_rss_free;
	u64   thread_rss_alloc_excluded;
	u64   thread_rss_free_excluded;
	char  comm[TASK_COMM_LEN];
};

struct thread_rss_data {
	int index;
	int count;
	int max;
	int over_threshold_count;
	struct thread_rss_entry tre[NR_ENTRIES];
};

static struct thread_rss_data *trd;

static int calculate_tre_rss(struct thread_rss_entry *tre)
{
	return __calculate_rss(tre->thread_rss_alloc,
			      tre->thread_rss_free,
			      tre->thread_rss_alloc_excluded,
			      tre->thread_rss_free_excluded);
}

static void thread_rss_history_add(int size)
{
	if (!trd) {
		/* Not initialized yet */
		return;
	}

	if (size >= thread_rss_threshold) {
		if (size > trd->max) {
			struct thread_rss_entry *tre = &(trd->tre[trd->index]);
			trd->max = size;

			tre->pid   = current->pid;
			tre->tgid  = current->tgid;
			tre->flags = is_kernel_thread(current);
			tre->req   = size;
			tre->thread_rss_alloc = current->thread_rss_alloc;
			tre->thread_rss_free  = current->thread_rss_free;
			tre->thread_rss_alloc_excluded = current->thread_rss_alloc_excluded;
			tre->thread_rss_free_excluded  = current->thread_rss_free_excluded;
			strncpy(tre->comm, current->comm, TASK_COMM_LEN);

			trd->index++;
			if (trd->index >= NR_ENTRIES) {
				trd->index = 0;
			}
			trd->count++;
		}
		trd->over_threshold_count++;
	}
}

static int thread_rss_history_show(struct seq_file *m, void *v)
{
	int start, end;
	char buf[TASK_COMM_LEN];

	end = (trd->count > NR_ENTRIES) ? NR_ENTRIES : trd->count;

	seq_printf(m, "------ The Maximum %d Records ------\n", end);
	seq_printf(m, "%5s %5s %8s %8s %8s %8s %8s %8s %2s %-16s\n",
		   "PID", "TGID", "ALLOC", "FREE", "ALLOC/EX", "FREE/EX", "RSS", "REQED", "KB", "COMMAND");

	for (start = 0; start < end; start++) {
		struct thread_rss_entry *tre = &(trd->tre[start]);
		int rss = calculate_tre_rss(tre);
		char *comm = task_comm_display(buf, tre->flags, tre->comm);

		seq_printf(m, "%5d %5d %8lld %8lld %8lld %8lld %8d %8d KB %-16s\n",
			   tre->pid, tre->tgid,
			   KB(tre->thread_rss_alloc),
			   KB(tre->thread_rss_free),
			   KB(tre->thread_rss_alloc_excluded),
			   KB(tre->thread_rss_free_excluded),
			   KB(rss),
			   KB(tre->req),
			   comm);
	}

	seq_printf(m, "Over Threshold Count: %d\n", trd->over_threshold_count);

	return 0;
}

#else  /* CONFIG_SNSC_THREAD_RSS_HISTORY */
#define thread_rss_history_add(size)    do { } while (0)
#define thread_rss_history_show(m, v)   do { } while (0)
#endif /* CONFIG_SNSC_THREAD_RSS_HISTORY */

/*
 * Alloc/Free is called in current thread context, but the
 * alloc/free is used by kernel, not current thread.
 */
static int is_unwanted_alloc(gfp_t flags)
{
	struct task_struct *tsk = current;

	if (is_kernel_thread(tsk)) {
		/*
		 * Kernel thread
		 *
		 * We can not fingre out the memory is REALLY allocated
		 * by kernel thread, because kernel thread usually allocate
		 * memory using same GFP_KERNEL as other kernel parts(page
		 * cache, slab allocator and so on...)
		 *
		 * So we do as much as we can:
		 *
		 * in_irq():     Hardware IRQ context
		 * in_softirq(): Softirq, for PREEMPT_RT, softirq kernel
		 *               threads always work in this mode.
		 *               We also record it as "unwanted" here
		 * GFP_USER, GFP_HIGHUSER and GFP_HIGHUSER_MOVABLE:
		 *               Kernel thread usually don't use this flag
		 *
		 */
		return (in_irq() || in_softirq() || \
			((flags & GFP_USER) == GFP_USER) || \
			((flags & GFP_HIGHUSER) == GFP_HIGHUSER) || \
			((flags & GFP_HIGHUSER_MOVABLE) == GFP_HIGHUSER_MOVABLE));
	} else {
		/*
		 * Userspace Thread
		 *
		 * Kernel won't use GFP_ATOMIC flags on page fault for
		 * userspace threads.
		 *
		 * A flag __GFP_ANONYMOUS is added to do_anonymous_page()
		 * handler. When userspace thread calls malloc() to allocate
		 * memory region and then access to that memory region, a
		 * page fault will happen, and kernel will call
		 * do_anonymous_page() to handle this kind of page fault.
		 */
		return (in_irq() || in_softirq() || \
			((flags & GFP_ATOMIC) == GFP_ATOMIC) || \
			!(flags & __GFP_ANONYMOUS));
	}
}

static int is_unwanted_free(void)
{
	return (in_interrupt());
}

/* API */
void trace_thread_rss_alloc(int size, gfp_t flags)
{
	current->thread_rss_alloc += size;

	if (is_unwanted_alloc(flags)) {
		current->thread_rss_alloc_excluded += size;
	}

	/* History */
	thread_rss_history_add(size);
}

void trace_thread_rss_free(int size)
{
	current->thread_rss_free += size;

	if (is_unwanted_free()) {
		current->thread_rss_free_excluded += size;
	}
}

/* Show */
static int thread_rss_proc_show(struct seq_file *m, void *v)
{
	struct task_struct *g, *t;
	u64 total_alloc = 0, total_free = 0;
	u64 total_alloc_ex = 0, total_free_ex = 0;
	int total_rss = 0;
	char buf[TASK_COMM_LEN];

	/* header */
	seq_printf(m, "%5s %5s %8s %8s %8s %8s %8s %2s %-16s\n",
		   "PID", "TGID", "ALLOC", "FREE", "ALLOC/EX", "FREE/EX", "RSS", "KB", "COMMAND");

	read_lock(&tasklist_lock);
	do_each_thread(g, t) {
		int rss = calculate_task_rss(t);
		char *comm = task_comm_display(buf, is_kernel_thread(t), t->comm);

		seq_printf(m, "%5d %5d %8lld %8lld %8lld %8lld %8d KB %-16s\n",
			   t->pid, t->tgid,
			   KB(t->thread_rss_alloc),
			   KB(t->thread_rss_free),
			   KB(t->thread_rss_alloc_excluded),
			   KB(t->thread_rss_free_excluded),
			   KB(rss),
			   comm);

		total_alloc += t->thread_rss_alloc;
		total_free  += t->thread_rss_free;
		total_alloc_ex += t->thread_rss_alloc_excluded;
		total_free_ex  += t->thread_rss_free_excluded;
		total_rss += rss;
	} while_each_thread(g, t);
	read_unlock(&tasklist_lock);

	seq_printf(m, "%-11s %8lld %8lld %8lld %8lld %8d KB\n",
		   "Total:",
		   KB(total_alloc),
		   KB(total_free),
		   KB(total_alloc_ex),
		   KB(total_free_ex),
		   KB(total_rss));

	/* Show History */
	thread_rss_history_show(m, v);

	return 0;
}

static int thread_rss_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, thread_rss_proc_show, NULL);
}

static int thread_rss_proc_write(struct file *file, const char *buffer,
				 size_t count, loff_t *ppos)
{
	struct task_struct *g, *t;

	read_lock(&tasklist_lock);
	do_each_thread(g, t) {
		t->thread_rss_alloc = 0;
		t->thread_rss_free = 0;
		t->thread_rss_alloc_excluded = 0;
		t->thread_rss_free_excluded = 0;
	} while_each_thread(g, t);
	read_unlock(&tasklist_lock);

#ifdef CONFIG_SNSC_THREAD_RSS_HISTORY
	memset(trd, 0, sizeof(struct thread_rss_data));
#endif
	return count;
}

static const struct file_operations thread_rss_proc_fops = {
	.open		= thread_rss_proc_open,
	.read		= seq_read,
	.write          = thread_rss_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_thread_rss_init(void)
{
	proc_create("thread_rss", 0, NULL, &thread_rss_proc_fops);

#ifdef CONFIG_SNSC_THREAD_RSS_HISTORY
	trd = kzalloc(sizeof(struct thread_rss_data), GFP_KERNEL);
	if (trd == NULL)
		return -ENOMEM;

	trd->max = thread_rss_threshold;
#endif

	return 0;
}
static void __exit proc_thread_rss_exit(void)
{
	remove_proc_entry("thread_rss", NULL);

#ifdef CONFIG_SNSC_THREAD_RSS_HISTORY
	if (trd) {
		kfree(trd);
	}
#endif
}

module_init(proc_thread_rss_init);
module_exit(proc_thread_rss_exit);

