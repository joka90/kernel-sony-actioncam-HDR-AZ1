/* 2012-06-28: File added and changed by Sony Corporation */
/*
 * kernel/snsc_workqueue.c
 *
 */

#include <linux/snsc_workqueue.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/kthread.h>
#include <linux/hardirq.h>
#include <linux/mempolicy.h>


struct snsc_cpu_workqueue_struct {

	spinlock_t lock;

	struct snsc_workqueue_struct *wq;
	wait_queue_head_t work_wait;
	struct list_head worklist;

	struct snsc_work_struct *current_work;
	struct task_struct *thread;
} ____cacheline_aligned;

struct snsc_workqueue_struct {
	struct snsc_cpu_workqueue_struct *cpu_wq;
	struct list_head wqlist;
	const char *name;
	int rt;
	int nice;
};

struct wq_barrier {
	struct snsc_work_struct  work;
	struct completion   done;
};

static cpumask_var_t cpu_populated_map __read_mostly;
static DEFINE_SPINLOCK(workqueue_lock);
static LIST_HEAD(workqueues);

#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED

#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED_ON_KIPID
static struct snsc_workqueue_struct *snsc_kipid_wq __read_mostly;
#else
static struct snsc_workqueue_struct * snsc_keventd_wq;
#endif

int snsc_schedule_work_cpu(struct snsc_work_struct *work, int cpu)
{
#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED_ON_KIPID
	return snsc_queue_work_on(cpu, snsc_kipid_wq, work);
#else
	return snsc_queue_work_on(cpu, snsc_keventd_wq, work);
#endif
}
#endif

static void insert_work(struct snsc_cpu_workqueue_struct *cwq,
			struct snsc_work_struct *work, struct list_head *head)
{
	smp_wmb();
	list_add_tail(&work->entry, head);
	wake_up(&cwq->work_wait);
}

static void wq_barrier_func(struct snsc_work_struct *work)
{
	struct wq_barrier *barr = container_of(work, struct wq_barrier, work);
	complete(&barr->done);
}

static void insert_wq_barrier(struct snsc_cpu_workqueue_struct *cwq,
				struct wq_barrier *barr, struct list_head *head)
{
	SNSC_INIT_WORK_ON_STACK(&barr->work, wq_barrier_func);
	init_completion(&barr->done);
	insert_work(cwq, &barr->work, head);
}

static int flush_cpu_workqueue(struct snsc_cpu_workqueue_struct *cwq)
{
	int active = 0;
	struct wq_barrier barr;

	WARN_ON(cwq->thread == current);

	spin_lock_irq(&cwq->lock);
	if (!list_empty(&cwq->worklist) || cwq->current_work != NULL) {
		insert_wq_barrier(cwq, &barr, &cwq->worklist);
		active = 1;
	}
	spin_unlock_irq(&cwq->lock);

	if (active)
		wait_for_completion(&barr.done);

	return active;
}

static void cleanup_workqueue_thread(struct snsc_cpu_workqueue_struct *cwq)
{

	if (cwq->thread == NULL)
		return;

	flush_cpu_workqueue(cwq);
	kthread_stop(cwq->thread);
	cwq->thread = NULL;
}

static void __snsc_queue_work(struct snsc_cpu_workqueue_struct *cwq,
			 struct snsc_work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&cwq->lock, flags);
	BUG_ON(!list_empty(&work->entry));
	insert_work(cwq, work, &cwq->worklist);
	spin_unlock_irqrestore(&cwq->lock, flags);
}

static struct snsc_cpu_workqueue_struct *
wq_per_cpu(struct snsc_workqueue_struct *wq, int cpu)
{
	return per_cpu_ptr(wq->cpu_wq, cpu);
}

int snsc_queue_work(struct snsc_workqueue_struct *wq,
		struct snsc_work_struct *work)
{
	int ret;

	ret = snsc_queue_work_on(get_cpu(), wq, work);
	put_cpu();

	return ret;
}
EXPORT_SYMBOL(snsc_queue_work);

int snsc_queue_work_on(int cpu, struct snsc_workqueue_struct *wq,
		struct snsc_work_struct *work)
{

	__snsc_queue_work(wq_per_cpu(wq, cpu), work);
	return 1;
}
EXPORT_SYMBOL(snsc_queue_work_on);

static void run_workqueue(struct snsc_cpu_workqueue_struct *cwq)
{
	spin_lock_irq(&cwq->lock);
	while (!list_empty(&cwq->worklist)) {
		struct snsc_work_struct *work = list_entry(cwq->worklist.next,
						struct snsc_work_struct, entry);
		snsc_work_func_t f = work->func;

		cwq->current_work = work;
		list_del_init(cwq->worklist.next);
		spin_unlock_irq(&cwq->lock);
		f(work);
		spin_lock_irq(&cwq->lock);
		cwq->current_work = NULL;
	}
	spin_unlock_irq(&cwq->lock);
}

static int worker_thread(void *__cwq)
{
	struct snsc_cpu_workqueue_struct *cwq = __cwq;
	DEFINE_WAIT(wait);

	for (;;) {
		prepare_to_wait(&cwq->work_wait, &wait, TASK_INTERRUPTIBLE);

		if (!kthread_should_stop() &&
		    list_empty(&cwq->worklist))
			schedule();

		finish_wait(&cwq->work_wait, &wait);
		if (kthread_should_stop())
			break;

		run_workqueue(cwq);
	}
	return 0;
}

static struct snsc_cpu_workqueue_struct *
init_cpu_workqueue(struct snsc_workqueue_struct *wq, int cpu)
{
	struct snsc_cpu_workqueue_struct *cwq = per_cpu_ptr(wq->cpu_wq, cpu);

	cwq->wq = wq;
	spin_lock_init(&cwq->lock);
	INIT_LIST_HEAD(&cwq->worklist);
	init_waitqueue_head(&cwq->work_wait);

	return cwq;
}

static int
create_workqueue_thread(struct snsc_cpu_workqueue_struct *cwq, int cpu)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	struct snsc_workqueue_struct *wq = cwq->wq;
	const char *fmt = "%s/%u";
	struct task_struct *p;

	p = kthread_create(worker_thread, cwq, fmt, wq->name, cpu);

	if (IS_ERR(p))
		return PTR_ERR(p);

	if (cwq->wq->rt) {
		param.sched_priority = cwq->wq->rt;
		sched_setscheduler_nocheck(p, SCHED_FIFO, &param);
	}

	set_user_nice(p, wq->nice);
	cwq->thread = p;

	return 0;
}

static void
start_workqueue_thread(struct snsc_cpu_workqueue_struct *cwq, int cpu)
{
	struct task_struct *p = cwq->thread;
	if (p != NULL) {
		if (cpu >= 0)
			kthread_bind(p, cpu);
		wake_up_process(p);
	}
}


static int __devinit workqueue_cpu_callback(struct notifier_block *nfb,
						unsigned long action,
						void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct snsc_cpu_workqueue_struct *cwq;
	struct snsc_workqueue_struct *wq;
	int err = 0;

	action &= ~CPU_TASKS_FROZEN;

	switch (action) {
		case CPU_UP_PREPARE:
			cpumask_set_cpu(cpu, cpu_populated_map);
	}

undo:
	list_for_each_entry(wq, &workqueues, wqlist) {
		cwq = per_cpu_ptr(wq->cpu_wq, cpu);

		switch (action) {
			case CPU_UP_PREPARE:
				err = create_workqueue_thread(cwq, cpu);
				if (!err)
					break;
				printk(KERN_ERR "workqueue [%s] for %i failed\n",
					wq->name, cpu);
				action = CPU_UP_CANCELED;
				err = -ENOMEM;
				goto undo;

			case CPU_ONLINE:
				start_workqueue_thread(cwq, cpu);
				break;

			case CPU_UP_CANCELED:
				start_workqueue_thread(cwq, -1);
			case CPU_POST_DEAD:
				cleanup_workqueue_thread(cwq);
				break;
		}
	}

	switch (action) {
	case CPU_UP_CANCELED:
	case CPU_POST_DEAD:
		cpumask_clear_cpu(cpu, cpu_populated_map);
	}

	return notifier_from_errno(err);
}

void snsc_destroy_workqueue(struct snsc_workqueue_struct *wq)
{
	int cpu;

	spin_lock(&workqueue_lock);
	list_del(&wq->wqlist);
	spin_unlock(&workqueue_lock);

	for_each_cpu(cpu, cpu_populated_map)
		cleanup_workqueue_thread(per_cpu_ptr(wq->cpu_wq, cpu));

	free_percpu(wq->cpu_wq);
	kfree(wq);
}
EXPORT_SYMBOL(snsc_destroy_workqueue);

static inline struct snsc_workqueue_struct *
alloc_workqueue_struct(const char *name, int rt, int nice)
{

	struct snsc_workqueue_struct *workq;

	workq = kzalloc(sizeof(struct snsc_workqueue_struct), GFP_KERNEL);
	if (!workq)
		return NULL;

	workq->cpu_wq = alloc_percpu(struct snsc_cpu_workqueue_struct);
	if (!workq->cpu_wq) {
		kfree(workq);
		return NULL;
	}

	workq->name = name;
	workq->rt = ((rt > (MAX_RT_PRIO - 1)) || rt < 0) ? 0 : rt;
	workq->nice = nice;
	INIT_LIST_HEAD(&workq->wqlist);


	return workq;
}

struct snsc_workqueue_struct *snsc_create_workqueue(const char *name,
						int rt, int nice)
{

	int err = 0, cpu;
	struct snsc_workqueue_struct *workq;
	struct snsc_cpu_workqueue_struct *cpu_wq;

	workq = alloc_workqueue_struct(name, rt, nice);
	if (!workq)
		return NULL;

	cpu_maps_update_begin();
	spin_lock(&workqueue_lock);
	list_add(&workq->wqlist, &workqueues);
	spin_unlock(&workqueue_lock);

	for_each_possible_cpu(cpu) {
	    cpu_wq = init_cpu_workqueue(workq, cpu);
	    if (err || !cpu_online(cpu))
			continue;
	    err = create_workqueue_thread(cpu_wq, cpu);
	    start_workqueue_thread(cpu_wq, cpu);
	}
	cpu_maps_update_done();

	if (err) {
		snsc_destroy_workqueue(workq);
		workq = NULL;
	}
	return workq;
}

#ifdef CONFIG_EJ_ON_EACH_CPU_THREAD_PRIORITY
# define WQ_PRIORITY CONFIG_EJ_ON_EACH_CPU_THREAD_PRIORITY
#else
# define WQ_PRIORITY 1
#endif
#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED_ON_KIPID
static unsigned int kipid_prio = WQ_PRIORITY;
module_param_named(kipid, kipid_prio, uint, S_IRUSR);
#endif /* CONFIG_EJ_ON_EACH_CPU_THREADED_ON_KIPID */

static int __init snsc_init_workqueues(void)
{

	alloc_cpumask_var(&cpu_populated_map, GFP_KERNEL);
	cpumask_copy(cpu_populated_map, cpu_online_mask);
	hotcpu_notifier(workqueue_cpu_callback, 0);
#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED

#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED_ON_KIPID
	if(!(snsc_kipid_wq = snsc_create_workqueue("kipid_wq", kipid_prio, -20)))
		panic("Can not create snsc_kipid_wq\n");
#else
	if(!(snsc_keventd_wq = snsc_create_workqueue("snsc_keventd_wq",1, -20)))
		panic("Can not create snsc_keventd_wq\n");
#endif

#endif

	return 0;
}
early_initcall(snsc_init_workqueues);
