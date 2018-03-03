/*
 *  Trace to call kmalloc
 *
 *  Copyright 2012,2013 Sony Corporation
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
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
# define trace_lock_init(lock)			spin_lock_init(lock)
# define trace_lock_irqsave(lock, flags)	spin_lock_irqsave(lock, flags)
# define trace_unlock_irqrestore(lock, flags)	spin_unlock_irqrestore(lock, flags)
#else
# define trace_lock_init(lock)			raw_spin_lock_init(lock)
# define trace_lock_irqsave(lock, flags)	raw_spin_lock_irqsave(lock, flags)
# define trace_unlock_irqrestore(lock, flags)	raw_spin_unlock_irqrestore(lock, flags)
#endif

#define MODE_TRACE  0x1
#define MODE_POISON 0x2
# define POISON_UNINITIALIZE 0x55
# define POISON_AFTER_FREE   0x33
#define __MODE_MASK (MODE_TRACE | MODE_POISON)
#define MODE_ENABLE(mode) ((mode) & __MODE_MASK)

static int trace_mode = 0;

/* filtering caller address */
static ulong trace_min = MODULES_VADDR;
static ulong trace_max = MODULES_END;
static ulong trace_order = 11; /* 2048 */
static ulong caller_order = 7; /* 128 */

module_param_named(mode, trace_mode, int, S_IRUSR | S_IWUSR);
module_param_named(min, trace_min, ulongH, S_IRUSR | S_IWUSR);
module_param_named(max, trace_max, ulongH, S_IRUSR | S_IWUSR);
module_param_named(trace_order, trace_order, ulongH, S_IRUSR | S_IWUSR);
module_param_named(caller_order, caller_order, ulongH, S_IRUSR | S_IWUSR);

struct caller_t {
	const void *key;
	u32 size;
	u32 cnt;
	raw_spinlock_t lock;
};

struct addr_t {
	const void *key;
	u32 size;
	struct caller_t *entry;
	raw_spinlock_t lock;
};

struct counter_t {
	u32 cnt;
	u32 max;
	raw_spinlock_t lock;
};

static struct counter_t caller_cnt;
static struct counter_t trace_cnt;

#define TRACE_ENTRY (1 << trace_order)
#define CALLER_ENTRY (1 << caller_order)
static struct addr_t *entries = NULL;
static struct caller_t *callers = NULL;

static inline u32 hash(const void *key, int off, u32 num, u32 sft)
{
	return ((((u32)key >> sft) + off) & (num - 1));
}

#define FLG_FIND  0x1
#define FLG_ALLOC 0x2
#define get_entry(array, num, val, sft, irqs, flags)			\
({									\
	typeof(array) head, ent = NULL;					\
	int i;								\
	head = rcu_dereference(array);					\
	if (!head)							\
		goto end;						\
	for (i = 0; i < (num); i++) {					\
		u32 idx = hash(val, i, num, sft);			\
		ent = &(head)[idx];					\
		trace_lock_irqsave(&ent->lock, irqs);			\
		if (((flags) & FLG_FIND) && (ent->key == (val)))	\
			break;						\
		if (((flags) & FLG_ALLOC) && !ent->key)			\
			break;						\
		trace_unlock_irqrestore(&ent->lock, irqs);		\
		ent = NULL;						\
	}								\
end:									\
	ent;								\
})

#define __get_entry(ent, irqs)				\
({							\
	trace_lock_irqsave(&(ent)->lock, irqs);		\
})

#define put_entry(ent, irqs)				\
({							\
	trace_unlock_irqrestore(&(ent)->lock, irqs);	\
})

/* 5: kmalloc addr is 32 byte alligned */
#define __get_trace_entry(val, irqs, flags)	get_entry(entries, TRACE_ENTRY, val, 5, irqs, flags)
#define get_trace_entry(val, irqs)		__get_trace_entry(val, irqs, FLG_ALLOC)
#define find_trace_entry(val, irqs)		__get_trace_entry(val, irqs, FLG_FIND)
#define put_trace_entry(ent, irqs)		put_entry(ent, irqs)

/* 2: caller is 4 byte alligned */
#define find_caller_entry(val, irqs)		get_entry(callers, CALLER_ENTRY, val, 2, irqs, FLG_FIND | FLG_ALLOC)
#define get_caller_entry(ent, irqs)		__get_entry(ent, irqs)
#define put_caller_entry(ent, irqs)		put_entry(ent, irqs)

#define entry_init(ptr, num)				\
({							\
	int i;						\
	for (i = 0; i < (num); i++, (ptr)++) {		\
		(ptr)->key = NULL;				\
		trace_lock_init(&(ptr)->lock);	\
	}						\
})

static void caller_init(struct caller_t *p, int n)
{
	entry_init(p, n);
}

static void trace_init(struct addr_t *p, int n)
{
	entry_init(p, n);
}

static void counter_init(struct counter_t *p)
{
	p->cnt = 0;
	p->max = 0;
	trace_lock_init(&p->lock);
}

static void counter_inc(struct counter_t *p)
{
	unsigned long flags;

	trace_lock_irqsave(&p->lock, flags);
	if (++p->cnt > p->max)
		p->max = p->cnt;
	trace_unlock_irqrestore(&p->lock, flags);
}

static void counter_dec(struct counter_t *p)
{
	unsigned long flags;

	trace_lock_irqsave(&p->lock, flags);
	--p->cnt;
	trace_unlock_irqrestore(&p->lock, flags);
}

static void counter_dump(struct seq_file *m, struct counter_t *p)
{
	unsigned long flags;
	u32 cnt, max;

	trace_lock_irqsave(&p->lock, flags);
	cnt = p->cnt;
	max = p->max;
	trace_unlock_irqrestore(&p->lock, flags);

	seq_printf(m, "current:%d, max:%d", cnt, max);
}

static struct caller_t *add_caller_entry(const void *caller, size_t size, int *alloced)
{
	unsigned long flags;
	struct caller_t *ent;

	*alloced = 0;

	ent = find_caller_entry(caller, flags);
	if (!ent)
		return NULL;

	if (!ent->key) {
		ent->key = caller;
		ent->size = 0;
		ent->cnt = 0;
		*alloced = 1;
	}

	ent->size += size;
	ent->cnt++;

	put_caller_entry(ent, flags);

	return ent;
}

static int del_caller_entry(struct caller_t *ent, u32 size)
{
	int ret = 0;
	unsigned long flags;

	get_caller_entry(ent, flags);

	ent->size -= size;

	if (!--ent->cnt) {
		ent->key = NULL;
		ent->size = 0;
		ret = 1;
	}

	put_caller_entry(ent, flags);

	return ret;
}

static int __kmalloc_add_trace_entry(const void *addr, size_t size, const void *caller)
{
	struct addr_t *ent;
	unsigned long flags;
	int alloced = 0;

	ent = get_trace_entry(addr, flags);
	if (!ent)
		return -ENOENT;

	ent->entry = add_caller_entry(caller, size, &alloced);
	if (!ent->entry) {
		put_trace_entry(ent, flags);
		return -ENOENT;
	}

	ent->key = addr;
	ent->size = size;

	put_trace_entry(ent, flags);

	if (alloced)
		counter_inc(&caller_cnt);

	counter_inc(&trace_cnt);

	return 0;
}

int kmalloc_add_trace_entry(void *addr, size_t size, gfp_t flags, const void *caller)
{
	int ret = 0;
	int mode = trace_mode;

	if (!MODE_ENABLE(mode))
		return 0;

	if ((ulong)caller < trace_min || (ulong)caller >= trace_max)
		return 0;

	if (!addr || !size)
		return -EINVAL;

	if (mode & MODE_TRACE) {
		rcu_read_lock();
		ret = __kmalloc_add_trace_entry(addr, size, caller);
		rcu_read_unlock();
	}

	if ((mode & MODE_POISON) && !(flags & __GFP_ZERO))
		memset(addr, POISON_UNINITIALIZE, size);

	return ret;
}

int kmalloc_add_trace(void *addr, size_t size, gfp_t flags)
{
	return kmalloc_add_trace_entry(addr, size, flags, __builtin_return_address(0));
}

EXPORT_SYMBOL(kmalloc_add_trace);

static int __kmalloc_del_trace_entry(const void *addr)
{
	struct addr_t *ent;
	unsigned long flags;
	int clear;
	int size;

	ent = find_trace_entry(addr, flags);
	if (!ent)
		return -ENOENT;

	clear = del_caller_entry(ent->entry, ent->size);

	size = ent->size;
	ent->size = 0;
	ent->entry = NULL;
	ent->key = NULL;

	put_trace_entry(ent, flags);

	if (clear)
		counter_dec(&caller_cnt);

	counter_dec(&trace_cnt);

	return size;
}

int kmalloc_del_trace_entry(void *addr)
{
	int mode = trace_mode;
	int size = 0;

	if (!MODE_ENABLE(mode))
		return 0;

	if (!addr)
		return -EINVAL;

	if (mode & MODE_TRACE) {
		rcu_read_lock();
		size = __kmalloc_del_trace_entry(addr);
		rcu_read_unlock();
	}

	if ((mode & MODE_POISON) && size > 0)
		memset(addr, POISON_AFTER_FREE, size);

	return (size < 0) ? size : 0;
}

static void show_symbol(struct seq_file *m, const void *addr)
{
#ifdef CONFIG_KALLSYMS
	unsigned long offset, size;
	char modname[MODULE_NAME_LEN], name[KSYM_NAME_LEN];

	if (lookup_symbol_attrs((unsigned long)addr, &size, &offset, modname, name) == 0) {
		seq_printf(m, "%s+%#lx/%#lx", name, offset, size);
		if (modname[0])
			seq_printf(m, " [%s]", modname);
		return;
	}
#endif
	seq_printf(m, "%p", addr);
}

static void *trace_seq_start(struct seq_file *m, loff_t *pos)
{
	struct caller_t *head;

	rcu_read_lock();

	if (*pos >= CALLER_ENTRY)
		return NULL;

	head = rcu_dereference(callers);
	if (!head)
		return NULL;

	return &head[*pos];
}

static void *trace_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct caller_t *ent = v;

	if (++(*pos) >= CALLER_ENTRY)
		return NULL;

	return ent + 1;
}

static void trace_seq_stop(struct seq_file *m, void *v)
{
	rcu_read_unlock();
}

static int trace_seq_show(struct seq_file *m, void *v)
{
	struct caller_t *head = rcu_dereference(callers);
	struct caller_t *ent = v;
	const void *caller;
	u32 size, cnt;
	unsigned long flags;
	static u32 total;

	if (!head)
		return 0;

	if (ent == head) {
		seq_printf(m, "trace count(");
		counter_dump(m, &trace_cnt);
		seq_printf(m, "/%d), ", TRACE_ENTRY);

		seq_printf(m, "caller count(");
		counter_dump(m, &caller_cnt);
		seq_printf(m, "/%d)\n", CALLER_ENTRY);

		seq_printf(m, "count\tsize\tcaller\n");
		total = 0;
	}

	trace_lock_irqsave(&ent->lock, flags);
	caller = ent->key;
	size = ent->size;
	cnt = ent->cnt;
	trace_unlock_irqrestore(&ent->lock, flags);

	if (caller) {
		total += size;
		seq_printf(m, "%d\t%d\t", cnt, size);
		show_symbol(m, caller);
		seq_printf(m, "\n");
	}

	if (ent == &head[CALLER_ENTRY - 1]) {
		seq_printf(m, "total:\t%d byte\n", total);
	}

	return 0;
}

static struct seq_operations trace_seqop = {
	.start = trace_seq_start,
	.next  = trace_seq_next,
	.stop  = trace_seq_stop,
	.show  = trace_seq_show
};

static int proc_trace_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &trace_seqop);
}

static int __kmalloc_trace_init(int mode)
{
	struct caller_t *c = NULL;
	struct addr_t *e = NULL;

	if (!(mode & MODE_TRACE))
		return 0;

	counter_init(&caller_cnt);
	counter_init(&trace_cnt);

	c = vmalloc(sizeof(struct caller_t) * CALLER_ENTRY);
	if (!c)
		return -ENOMEM;

	caller_init(c, CALLER_ENTRY);

	e = vmalloc(sizeof(struct addr_t) * TRACE_ENTRY);
	if (!e) {
		vfree(c);
		return -ENOMEM;
	}

	trace_init(e, TRACE_ENTRY);

	rcu_assign_pointer(callers, c);
	rcu_assign_pointer(entries, e);

	return 0;
}

static void __kmalloc_trace_exit(void)
{
	struct addr_t *e;
	struct caller_t *c;

	rcu_read_lock();
	e = rcu_dereference(entries);
	c = rcu_dereference(callers);
	rcu_read_unlock();

	rcu_assign_pointer(entries, NULL);
	rcu_assign_pointer(callers, NULL);

	synchronize_rcu();

	if (e)
		vfree(e);
	if (c)
		vfree(c);
}

static int proc_trace_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char c;
	int mode, err = 0;

	if (copy_from_user(&c, buf, sizeof(c)))
		return -EFAULT;

	__kmalloc_trace_exit();

	mode = c - '0';
	err = __kmalloc_trace_init(mode);
	if (err) {
		printk("cannot change trace mode: %d, err = %d\n", mode, err);
		return err;
	}

	trace_mode = mode;

	return count;
}

static struct file_operations trace_operations = {
	.open    = proc_trace_open,
	.read    = seq_read,
	.write   = proc_trace_write,
	.llseek  = seq_lseek,
	.release = seq_release,
};

#define PROC_NAME "kmalloc_trace"
static int kmalloc_trace_init(void)
{
	struct proc_dir_entry *ent;
	int mode = trace_mode;

	ent = create_proc_entry(PROC_NAME, 0444, NULL);
	if (!ent) {
		trace_mode = 0;
		return 0;
	}

	ent->proc_fops = &trace_operations;

	if (__kmalloc_trace_init(mode)) {
		trace_mode = 0;
	}

	return 0;
}

early_initcall(kmalloc_trace_init);
