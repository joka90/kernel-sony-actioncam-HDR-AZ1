/*
 * arch/arm/mach-cxd90014/tmonitor.c
 *
 * Copyright 2012 Sony Corporation.
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
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <asm/io.h>
#include <mach/time.h>
#include <asm/uaccess.h>
#include <linux/snsc_boot_time.h>
#include <mach/tmonitor.h>
#include <linux/suspend.h>

#ifdef CONFIG_EJ_SCHED_DEBUG
#include <mach/kslog.h>
#endif /* CONFIG_EJ_SCHED_DEBUG */

#define GET_TIME() read_freerun()
#define t2u(t) (t)/(FREERUN_TICK_RATE/1000000)
#define u2t(u) (u)*(FREERUN_TICK_RATE/1000000)

int trace_cpu = 0;

static ulong buf_addr;
static ulong buf_size;
static ulong dfl_cpu;
module_param_named(addr, buf_addr, ulongH, S_IRUSR|S_IWUSR);
module_param_named(size, buf_size, ulongH, S_IRUSR|S_IWUSR);
module_param_named(mask, dfl_cpu, ulong, S_IRUSR|S_IWUSR);
/*
 * mode
 * 0 - ringbuffer mode
 * 1 - snapshot mode
 */
static int mode;
module_param_named(mode, mode, int, S_IRUSR|S_IWUSR);

/*
 * auto start mode
 * 0 - Do not start automatically.
 * 1 - boot_time_add() string match
 */
static int tmon_autostart = 0;
module_param_named(autostart, tmon_autostart, int, S_IRUSR|S_IWUSR);
static char *tmon_autostart_str;
module_param_named(autostart_str, tmon_autostart_str, charp, S_IRUSR|S_IWUSR);

/*
 * auto stop mode (bitmap)
 * 0 - Do not stop automatically.
 * 1 - boot_time_add() string match
 * 2 - N ticks elapsed after last boot_time_add().
 * 4 - boot_time_add() at an interval of N ticks or longer.
 */
int tmon_autostop = 0;
module_param_named(autostop, tmon_autostop, int, S_IRUSR|S_IWUSR);
static int tmon_autostop_retrigger = 0;
module_param_named(autostop_retrigger, tmon_autostop_retrigger, int, S_IRUSR|S_IWUSR);
static char *tmon_autostop_str;
module_param_named(autostop_str, tmon_autostop_str, charp, S_IRUSR|S_IWUSR);
static ulong tmon_autostop_ticks;
module_param_named(autostop_ticks, tmon_autostop_ticks, ulong, S_IRUSR|S_IWUSR);
static int tmon_autostop_segv;
module_param_named(autostop_segv, tmon_autostop_segv, bool, S_IRUSR|S_IWUSR);
static int tmon_autostop_tasklist;
module_param_named(autostop_tasklist, tmon_autostop_tasklist, bool, S_IRUSR|S_IWUSR);
static int tmon_autostop_sus;
module_param_named(autostop_sus, tmon_autostop_sus, bool, S_IRUSR|S_IWUSR);

static int tmon_autostopped = 0;

static u32 max_entry;
#define MAX_ENTRY max_entry

struct trace_entry {
	unsigned long time;
	short ppid, npid;
	int data;
	unsigned short user:1;
	signed short cpu:3;
	signed short pri:12;
	short state;
	char prev[TASK_COMM_LEN-1];
	char padding;
};

struct trace_data {
	int index;
	int overflow;
	struct trace_entry *entries;
};

#define MAX_HASH 0x100
struct name_hash {
	unsigned long addr;
	unsigned long offset;
	char name[32];
};

struct dump_cookie {
	struct name_hash hash[MAX_HASH];
	char namebuf[KSYM_SYMBOL_LEN];
};

static struct trace_data trace_datas;
static DEFINE_RAW_SPINLOCK(lock);
static int maxidx;
static int old_trace_cpu;

void
add_trace_entry(int cpu, struct task_struct *prev, struct task_struct *next, int data)
{
	struct trace_data *pdata = &trace_datas;
	struct trace_entry *entry;
	unsigned long flags;

	if (system_state != SYSTEM_RUNNING)
		return;

	if (!(trace_cpu & (1 << cpu))) {
		return;
	}

	raw_spin_lock_irqsave(&lock, flags);
	if (pdata->index < MAX_ENTRY) {
		entry = pdata->entries + pdata->index ++;
	}
	else {
		if (mode) {
			raw_spin_unlock_irqrestore(&lock, flags);
			return;
		}

		pdata->overflow = 1;
		pdata->index = 0;
		entry = pdata->entries + pdata->index ++;
	}
	raw_spin_unlock_irqrestore(&lock, flags);

	entry->time = GET_TIME();
	entry->ppid = prev->pid;
	entry->npid = next->pid;
	entry->data = data;
	entry->cpu = cpu;
	entry->user = 0;
	entry->pri = prev->rt_priority;
	entry->state = prev->state;
	memcpy(entry->prev, prev->comm, sizeof(entry->prev));
	entry->padding = 0;
}

static int
dump_trace_entry(struct seq_file *m, int i)
{
	struct dump_cookie *cookie = m->private;
	struct trace_data *pdata = &trace_datas;
	struct trace_entry *entry = pdata->entries + (i % MAX_ENTRY);
	u32 time = t2u(entry->time);
	u32 sec = time / 1000000;
	u32 us = time % 1000000;

#define CRLF "\r\n"
	/* user entry */
	if (entry->user) {
		seq_printf(m, "[ %5d.%06d ] -profile user %s cpu:%d" CRLF, sec, us, entry->prev, entry->cpu);
		return 0;
	}
	/* irq */
	else if (entry->npid == entry->ppid) {
		seq_printf(m, "[ %5d.%06d ] -profile user irq:%d,%d cpu:%d" CRLF, sec, us, entry->data&0xff, entry->data>>8, entry->cpu);
		return 0;
	}
	else {
		char *modname;
		const char *fname = NULL;
		unsigned long offset, size;
		unsigned long addr = entry->data;
		struct name_hash *hash = &cookie->hash[addr % MAX_HASH];

		if (addr && hash->addr == addr) {
			fname = hash->name;
			offset = hash->offset;
		}
		else if (addr) {
			fname = kallsyms_lookup(entry->data, &size, &offset, &modname, cookie->namebuf);
			if (fname) {
				memset(hash->name, 0, sizeof(hash->name));
				strncpy(hash->name, fname, sizeof(hash->name)-1);
				hash->addr = addr;
				hash->offset = offset;
			}
		}

		seq_printf(m, "[ %5d.%06d ] -sched next:%03d < prev:%03d state:%x wchan:%s%s%lx(%lx) task:%s cpu:%d prio:%d" CRLF,
				sec, us,
				entry->npid, entry->ppid,
				entry->state,
				fname?:"", fname?"+":"", fname?offset:addr, addr,
				entry->prev,
				entry->cpu, entry->pri
			  );
	}

	return 0;
}

static void set_trace_cpu(int cpus)
{
	if (!buf_addr || !buf_size)
		return;

	if (!trace_cpu) {
		struct trace_data *pdata = &trace_datas;

		pdata->entries = phys_to_virt(buf_addr);
		max_entry = buf_size / sizeof(struct trace_entry);

		pdata->index = 0;
		smp_mb();
		trace_cpu = cpus;
	}
	else {
		trace_cpu = cpus;
	}
}

void tmonitor_stop(void)
{
	trace_cpu = 0;
}
EXPORT_SYMBOL(tmonitor_stop);

void tmonitor_write(const char *buffer) {
	int count;
	struct trace_data *pdata = &trace_datas;
	struct trace_entry *entry;
	unsigned long flags;

	if ((unsigned long)buffer < 0x10) {
		unsigned long cpus = (unsigned long)buffer;
		set_trace_cpu(cpus);
		tmon_autostopped = 0;
		return;
	}

	if (!trace_cpu)
		return;

	raw_spin_lock_irqsave(&lock, flags);
	if (pdata->index < MAX_ENTRY) {
		entry = pdata->entries + pdata->index ++;
	}
	else {
		if (mode) {
			raw_spin_unlock_irqrestore(&lock, flags);
			return;
		}

		pdata->overflow = 1;
		pdata->index = 0;
		entry = pdata->entries + pdata->index ++;
	}
	raw_spin_unlock_irqrestore(&lock, flags);
	entry->cpu = raw_smp_processor_id();
	entry->state = 0;
	entry->time = GET_TIME();
	entry->user = 1;
	memset(entry->prev, 0, sizeof(entry->prev));
	count = strlen(buffer);
	count = sizeof(entry->prev) > count ? count : sizeof(entry->prev);
	memcpy(entry->prev, buffer, count);
	entry->padding = 0;
}
EXPORT_SYMBOL(tmonitor_write);

static int write_tmonitor(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	int i = 0;
	int cnt;
	char ubuf[TASK_COMM_LEN];

	if (!buf_addr || !buf_size)
		goto out;

	if (count > sizeof(ubuf) - 1)
		count = sizeof(ubuf) - 1;
	if (copy_from_user(ubuf, buffer, count)) {
		return -EFAULT;
	}
	ubuf[count] = 0;

	/* atoi() */
	for ( cnt = 0; cnt < count; cnt++ ) {
		if (isdigit(ubuf[cnt]))
			i = i*10 + ubuf[cnt] - '0';
		else {
			i = -1;
			break;
		}
	}

	if ( i < 0 || i >= (1 << NR_CPUS) ) {
		if (!trace_cpu)
			goto out;
		tmonitor_write(ubuf);
	}
	else {
		u8 cpus = (u8)i;
		set_trace_cpu(cpus);
		tmon_autostopped = 0;
#ifdef CONFIG_EJ_SCHED_DEBUG
		if (cpus) { /* start */
			kslog_init();
		} else { /* stop */
			kslog_show();
		}
#endif /* CONFIG_EJ_SCHED_DEBUG */
	}

out:
	return count;
}


static void tmonitor_autostart(char *str)
{
	if (!tmon_autostart || tmon_autostopped)
		return;
	if (!str || !tmon_autostart_str)
		return;
	if (strncmp(str, tmon_autostart_str, strlen(tmon_autostart_str)))
		return;
#ifdef CONFIG_EJ_SCHED_DEBUG
	kslog_init();
#endif /* CONFIG_EJ_SCHED_DEBUG */
	set_trace_cpu(TMON_TRACE_ALL);
}

static void tmonitor_autostop_timer(void)
{
#ifdef CONFIG_SNSC_BOOT_TIME
	extern unsigned long boot_time_elapse(void);
	extern struct task_struct *boot_time_last_task;

	if (time_after(boot_time_elapse(), tmon_autostop_ticks)) {
		tmon_autostopped = 1;
		tmonitor_stop();
		if (tmon_autostop_sus) {
			pm_suspend_stop(1);
		}
#ifdef CONFIG_EJ_SCHED_DEBUG
		kslog_show();
#endif /* CONFIG_EJ_SCHED_DEBUG */
		if (tmon_autostop_tasklist) {
			show_state();
		}
		printk(KERN_ERR "tmon:LONG_ELAPSE\n");
		if (tmon_autostop_segv && boot_time_last_task) {
			force_sig(SIGSEGV, boot_time_last_task);
		}
	}
#endif /* CONFIG_SNSC_BOOT_TIME */
}

static void tmonitor_autostop_str(char *str)
{
	if (!str || !tmon_autostop_str)
		return;
	if (strncmp(str, tmon_autostop_str, strlen(tmon_autostop_str)))
		return;
	tmon_autostopped = 1;
	tmonitor_stop();
	if (tmon_autostop_retrigger & TMON_AUTOSTOP_STR) {
		tmon_autostopped = 0;
	}
	if (tmon_autostopped  &&  tmon_autostop_sus) {
		pm_suspend_stop(1);
	}

	/* printk(KERN_ERR "tmon:STRING_MATCH\n"); */
}

static void tmonitor_autostop(char *str)
{
	if (!trace_cpu || !tmon_autostop || tmon_autostopped)
		return;

	if (tmon_autostop & TMON_AUTOSTOP_STR) {
		tmonitor_autostop_str(str);
	}
	if (tmon_autostop & TMON_AUTOSTOP_DELAY2) {
		tmonitor_autostop_timer();
	}
}

void tmonitor_boot_time_notify(char *comment)
{
	if (comment) {
		tmonitor_autostart(comment);
		tmonitor_write(comment);
	}
	tmonitor_autostop(comment);
}

void tmonitor_timer_notify(void)
{
	if (!trace_cpu || tmon_autostopped)
		return;
	tmonitor_autostop_timer();
}


static void *
tmonitor_seq_start(struct seq_file *m, loff_t *pos)
{
	int off = *pos;

	m->private = vmalloc(sizeof(struct dump_cookie));
	if (!m->private)
		return NULL;

	if (!off) {
		if (!(trace_datas.index | trace_datas.overflow))
			return NULL;

		/* stop tracing */
		old_trace_cpu = trace_cpu;
		trace_cpu = 0;
		msleep(10);
		maxidx = trace_datas.index;
		if (trace_datas.overflow)
			maxidx += MAX_ENTRY;
	}

	if (trace_datas.overflow)
		off += trace_datas.index;

	if (off >= maxidx)
		return NULL;

	return (void *)(off + 1);
}

static void
tmonitor_seq_stop(struct seq_file *m, void *v)
{
	vfree(m->private);
}

static void *
tmonitor_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	int off = (int)v;

	(*pos)++;

	off ++;

	if (off <= maxidx) {
		return (void *)(off);
	}
	else {
		/* resume tracing */
		maxidx = 0;
		trace_datas.index = 0;
		trace_datas.overflow = 0;
		smp_mb();
		trace_cpu = old_trace_cpu;
		old_trace_cpu = 0;
		return NULL;
	}
}

static int
tmonitor_seq_show(struct seq_file *m, void *v)
{
	int off = (int)v;
	dump_trace_entry(m, off-1);
	return 0;
}

static struct seq_operations tmonitor_seqop = {
	.start = tmonitor_seq_start,
	.next  = tmonitor_seq_next,
	.stop  = tmonitor_seq_stop,
	.show  = tmonitor_seq_show
};

static int open_tmonitor(struct inode *inode, struct file *file)
{
	return seq_open(file, &tmonitor_seqop);
}

static struct file_operations tmonitor_operations = {
	.open    = open_tmonitor,
	.read    = seq_read,
	.write   = write_tmonitor,
	.llseek  = seq_lseek,
	.release = seq_release,
};

static int __init trace_init(void)
{
	struct proc_dir_entry *entry;
	entry = create_proc_entry("tmonitor", S_IRUSR | S_IWUSR, NULL);
	if (entry) {
		entry->proc_fops = &tmonitor_operations;
	}

	set_trace_cpu(dfl_cpu);
	return 0;
}

module_init(trace_init);
