/* 2012-08-27: File added and changed by Sony Corporation */
#include <linux/rcupdate.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/kernel_stat.h>

atomic_t rcu_cnt ____cacheline_aligned_in_smp = ATOMIC_INIT(0);
struct rcu_data {
	struct rcu_head *wait_head;
	struct rcu_head *wait_tail;

	struct rcu_head *done_head;
	struct rcu_head *done_tail;
};
static DEFINE_PER_CPU(struct rcu_data, rcu_datas);

static void rcu_call_donelist(struct rcu_head *head)
{
	struct rcu_head *cur = head;
	int done = 0;

	BUG_ON(!cur);

	while (cur) {
		struct rcu_head *this = cur;
		cur = cur->next;
		__rcu_reclaim(this);
		done ++;
	}

	pr_debug(KERN_DEBUG "%s: cpu=%d, done=%d\n", __func__, raw_smp_processor_id(), done);
}

static inline void add_to_list(struct rcu_head **head,
			struct rcu_head **tail,
			struct rcu_head *head2,
			struct rcu_head *tail2)
{
	BUG_ON(!head2 || !tail2);
	if (!*head) {
		*head = head2;
	}
	else {
		(*tail)->next = head2;
	}
	*tail = tail2;
}

static void check_pending_rcu(void)
{
	unsigned long flags;
	int cpu;
	struct rcu_data *cpu_data;

	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	cpu_data = &per_cpu(rcu_datas, cpu);
	if (!atomic_read(&rcu_cnt) && cpu_data->wait_head) {
		add_to_list(&cpu_data->done_head,
			&cpu_data->done_tail,
			cpu_data->wait_head,
			cpu_data->wait_tail);
		cpu_data->wait_head = NULL;
	}
	local_irq_restore(flags);
}

#ifndef CONFIG_EJ_RCU_SONY_BATCH_MODE
void __rcu_read_unlock(void)
{
	if (per_cpu(rcu_datas, raw_smp_processor_id()).wait_head)
		check_pending_rcu();
}
EXPORT_SYMBOL_GPL(__rcu_read_unlock);
#endif

struct sync_data {
	struct completion com;
	struct rcu_head head;
};

static void wake_up_rcu(struct rcu_head *head)
{
	struct sync_data *data = container_of(head, struct sync_data, head);
	complete(&data->com);
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
#include <linux/kernel_stat.h>

/*
 * During boot, we forgive RCU lockdep issues.  After this function is
 * invoked, we start taking RCU lockdep issues seriously.
 */
void __init rcu_scheduler_starting(void)
{
	WARN_ON(nr_context_switches() > 0);
	rcu_scheduler_active = 1;
}

int rcu_scheduler_active __read_mostly;
EXPORT_SYMBOL_GPL(rcu_scheduler_active);
#endif /* #ifdef CONFIG_DEBUG_LOCK_ALLOC */

void synchronize_rcu(void)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	if (!rcu_scheduler_active)
		return;
#endif /* #ifdef CONFIG_DEBUG_LOCK_ALLOC */

	if (atomic_read(&rcu_cnt)) {
		struct sync_data data;
		init_completion(&data.com);
		call_rcu(&data.head, wake_up_rcu);
		wait_for_completion(&data.com);
	}
}

void call_rcu(struct rcu_head *head,
			       void (*func)(struct rcu_head *rcu))
{
	unsigned long flags;
	int cpu;
	struct rcu_data *cpu_data;

	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	cpu_data = &per_cpu(rcu_datas, cpu);

	head->func = func;

	head->next = NULL;
	add_to_list(&cpu_data->wait_head, &cpu_data->wait_tail, head, head);

	local_irq_restore(flags);
}

void rcu_in_timer(void)
{
	int cpu = raw_smp_processor_id();
	struct rcu_data *cpu_data = &per_cpu(rcu_datas, cpu);
	struct rcu_head *donelist = NULL;

	check_pending_rcu();

	local_irq_disable();
	if (cpu_data->done_head) {
		donelist = cpu_data->done_head;
		cpu_data->done_head = NULL;
	}
	local_irq_enable();

	if (donelist) {
		rcu_call_donelist(donelist);
	}
}

int rcu_needs_cpu(int cpu)
{
	struct rcu_data *cpu_data = &per_cpu(rcu_datas, cpu);

	if ((cpu_data->done_head) ||
	    (cpu_data->wait_head))
		return 1;
	else
		return 0;
}

static void __cpuinit rcu_offline_cpu(int cpu)
{
	unsigned long flags;
	struct rcu_data *dead_cpu_data = &per_cpu(rcu_datas, cpu);
	struct rcu_data *cpu_data;

	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	cpu_data = &per_cpu(rcu_datas, cpu);
	if(dead_cpu_data->done_head)
		add_to_list(&cpu_data->done_head,
			    &cpu_data->done_tail,
			    dead_cpu_data->done_head,
			    dead_cpu_data->done_tail);
	if(dead_cpu_data->wait_head)
		add_to_list(&cpu_data->wait_head,
			    &cpu_data->wait_tail,
			    dead_cpu_data->wait_head,
			    dead_cpu_data->wait_tail);
	dead_cpu_data->done_head = dead_cpu_data->wait_head = NULL;
	local_irq_restore(flags);
}

static void __cpuinit rcu_online_cpu(int cpu)
{
}

static int __cpuinit rcu_cpu_notify(struct notifier_block *self,
				unsigned long action, void *hcpu)
{
	long cpu = (long)hcpu;
	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		rcu_online_cpu(cpu);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		rcu_offline_cpu(cpu);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata rcu_nb = {
	.notifier_call	= rcu_cpu_notify,
};

void __init rcu_init(void)
{
	rcu_cpu_notify(&rcu_nb, CPU_ONLINE,
			(void *)(long)smp_processor_id());
	/* Register notifier for non-boot CPUs */
	register_cpu_notifier(&rcu_nb);
}

static DEFINE_PER_CPU(struct rcu_head, rcu_barrier_head) = {NULL};
static atomic_t rcu_barrier_cpu_count;
static DEFINE_MUTEX(rcu_barrier_mutex);
static struct completion rcu_barrier_completion;

static void rcu_barrier_callback(struct rcu_head *notused)
{
	if (atomic_dec_and_test(&rcu_barrier_cpu_count))
		complete(&rcu_barrier_completion);
}

/*
 * Called with preemption disabled, and from cross-cpu IRQ context.
 */
static void rcu_barrier_func(void *notused)
{
	int cpu = raw_smp_processor_id();
	struct rcu_head *head = &per_cpu(rcu_barrier_head, cpu);

	atomic_inc(&rcu_barrier_cpu_count);
	call_rcu(head, rcu_barrier_callback);
}

/**
 * rcu_barrier - Wait until all the in-flight RCUs are complete.
 */
void rcu_barrier(void)
{
	BUG_ON(in_interrupt());
	/* Take cpucontrol mutex to protect against CPU hotplug */
	mutex_lock(&rcu_barrier_mutex);
	init_completion(&rcu_barrier_completion);
	atomic_set(&rcu_barrier_cpu_count, 0);
	/*
	 * The queueing of callbacks in all CPUs must be atomic with
	 * respect to RCU, otherwise one CPU may queue a callback,
	 * wait for a grace period, decrement barrier count and call
	 * complete(), while other CPUs have not yet queued anything.
	 * So, we need to make sure that grace periods cannot complete
	 * until all the callbacks are queued.
	 */
	rcu_read_lock();
	on_each_cpu(rcu_barrier_func, NULL, 1);
	rcu_read_unlock();
	wait_for_completion(&rcu_barrier_completion);
	mutex_unlock(&rcu_barrier_mutex);
}

EXPORT_SYMBOL_GPL(synchronize_rcu);
EXPORT_SYMBOL_GPL(call_rcu);
EXPORT_SYMBOL_GPL(rcu_barrier);
EXPORT_SYMBOL_GPL(rcu_cnt);

