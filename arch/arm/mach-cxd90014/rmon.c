/*
 * arch/arm/mach-cxd90014/rmon.c
 *
 * Resource Monitor
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on fs/proc/meminfo.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/snsc_boot_time.h>

#define RMON_PROCNAME "rmon"

#define DEFAULT_PERIOD	1000 /* ms */

#undef CALC_BUFFERRAM

static int rmon_enable = 1;
module_param_named(en, rmon_enable, bool, S_IRUSR);
static uint rmon_period = DEFAULT_PERIOD;
module_param_named(period, rmon_period, uint, S_IRUSR|S_IWUSR);

/* Cached */
static ulong rmon_cached_thr_lh = 512; /* pages */
static ulong rmon_cached_thr_hl = 400; /* pages */
module_param_named(cachedH, rmon_cached_thr_lh, ulong, S_IRUSR|S_IWUSR);
module_param_named(cachedL, rmon_cached_thr_hl, ulong, S_IRUSR|S_IWUSR);

struct rmon_stat {
	int state;
	ulong *thr_lh, *thr_hl; /* hysterisis */
	ulong data;
};
/* state */
#define RMON_ST_INIT 0
#define RMON_ST_LH 1
#define RMON_ST_HL 2

static struct rmon_stat rmon_stats[] = {
	{ /* Cached */
		.state = RMON_ST_INIT,
		.thr_lh = &rmon_cached_thr_lh,
		.thr_hl = &rmon_cached_thr_hl,
	},
};
#define RMON_NSTATS	ARRAY_SIZE(rmon_stats)
#define RMON_ID_CACHED	0

static char rmon_msg[] = "rmon:-";
#define RMON_MSG_OFF	5
#define RMON_MSG_LEN	(sizeof rmon_msg - 1 - RMON_MSG_OFF)

static void rmon_set(uint id, char ch)
{
	rmon_msg[RMON_MSG_OFF + id] = ch;
}

static int rmon_update(uint id)
{
	struct rmon_stat *s;
	int update = 0;

	if (id >= RMON_NSTATS) {
		printk(KERN_ERR "ERROR:rmon:id=%u\n", id);
		return 0;
	}
	s = rmon_stats + id;
	if (!s->thr_lh || !s->thr_lh) {
		return 0;
	}
	/* state transision */
	switch (s->state) {
	case RMON_ST_INIT: /* wait for H level */
		if (s->data >= *s->thr_lh) {
			s->state = RMON_ST_HL;
			rmon_set(id, 'H');
		}
		break;
	case RMON_ST_LH: /* L->H transition */
		if (s->data >= *s->thr_lh) {
			s->state = RMON_ST_HL;
			rmon_set(id, 'H');
			update = 1;
		}
		break;
	case RMON_ST_HL: /* H->L transition */
		if (s->data < *s->thr_hl) {
			s->state = RMON_ST_LH;
			rmon_set(id, 'L');
			update = 1;
		}
		break;
	default:
		printk(KERN_ERR "ERROR:rmon:state=%d\n", s->state);
		break;
	}
	return update;
}

static int rmond(void *unused)
{
	signed long t;
#ifdef CALC_BUFFERRAM
	struct sysinfo si;
#endif
	long cached;
	uint id;
	int update;

	while (!kthread_should_stop() && rmon_period) {
		/* monitoring */
		cached = global_page_state(NR_FILE_PAGES)
			- total_swapcache_pages;
#ifdef CALC_BUFFERRAM
		si_meminfo(&si);
		cached -= si.bufferram;
#endif
		if (cached < 0)
			cached = 0;
		rmon_stats[RMON_ID_CACHED].data = cached;

		/* update */
		update = 0;
		for (id = 0; id < RMON_NSTATS; id++) {
			update |= rmon_update(id);
		}
		if (update) {
#if 0
			BOOT_TIME_ADD1(rmon_msg);
#else
			printk(KERN_ERR "%s\n", rmon_msg);
#endif
		}

		/* sleep */
		if ((t = msecs_to_jiffies(rmon_period)) == 0) {
			t = 1;
		}
		while (t && !kthread_should_stop() && rmon_period) {
			t = schedule_timeout_interruptible(t);
		}
	}
	return 0;
}

/*----------------------------------------------------------------*/
static int rmon_write(struct file *file, const char __user *buf,
		      size_t len, loff_t *ppos)
{
	char arg[16];
	int n, start;

	if (len > sizeof arg - 1) {
		printk(KERN_ERR "/proc/%s: too long.\n", RMON_PROCNAME);
		return -1;
	}
	if (copy_from_user(arg, buf, len)) {
		return -EFAULT;
	}
	arg[len] = '\0';

	n = sscanf(arg, "%d", &start);
	if (n != 1) {
		printk(KERN_ERR "/proc/%s: invalid format.\n", RMON_PROCNAME);
		return -1;
	}

	return len;
}

#define RMON_NR_SEQ 1

static void *rmon_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos < RMON_NR_SEQ)
		return pos;
	return NULL;
}

static void *rmon_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos < RMON_NR_SEQ)
		return pos;
	return NULL;
}

static void rmon_seq_stop(struct seq_file *seq, void *v)
{
}

static int rmon_seq_show(struct seq_file *seq, void *v)
{
	int i = *(loff_t *)v;

	if (i >= 0  &&  i < RMON_NSTATS) {
		seq_printf(seq, "%d: %d %lu", i, rmon_stats[i].state,
			   rmon_stats[i].data);
		if (rmon_stats[i].thr_hl && rmon_stats[i].thr_lh) {
			seq_printf(seq, "(%lu,%lu)", *rmon_stats[i].thr_hl,
				   *rmon_stats[i].thr_lh);
		}
		seq_printf(seq, "\n");
	}
	return 0;
}

static struct seq_operations rmon_seq_ops = {
	.start	= rmon_seq_start,
	.next	= rmon_seq_next,
	.stop	= rmon_seq_stop,
	.show	= rmon_seq_show,
};

static int rmon_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rmon_seq_ops);
}

static struct file_operations proc_rmon_ops = {
	.owner		= THIS_MODULE,
	.open		= rmon_seq_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
	.write		= rmon_write,
};

static struct proc_dir_entry *proc_rmon = NULL;
static struct task_struct *rmon_task;

static int __init rmon_init(void)
{
	int ret = 0;

	if (!rmon_enable)
		return 0;

	proc_rmon = create_proc_entry(RMON_PROCNAME, 0, NULL);
	if (!proc_rmon) {
		printk(KERN_ERR "ERROR:rmon:can not create proc entry\n");
		return -1;
	}
	proc_rmon->proc_fops = &proc_rmon_ops;

	rmon_task = kthread_run(rmond, NULL, "rmond");
	if (IS_ERR(rmon_task)) {
		printk(KERN_ERR "ERROR:rmon:can not create rmond.\n");
		ret = -EIO;
		goto err1;
	}
        return 0;
 err1:
	remove_proc_entry(RMON_PROCNAME, NULL);
	return ret;
}

module_init(rmon_init);
