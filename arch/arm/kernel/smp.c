/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/arch/arm/kernel/smp.c
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/cache.h>
#include <linux/profile.h>
#include <linux/errno.h>
#include <linux/ftrace.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/percpu.h>
#include <linux/clockchips.h>
#include <linux/completion.h>
#include <linux/rt_trace_lite.h>
#include <linux/rt_trace_lite_irq.h>

#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/processor.h>
#include <asm/sections.h>
#include <asm/tlbflush.h>
#include <asm/ptrace.h>
#include <asm/localtimer.h>
#include <linux/snsc_boot_time.h>

/*
 * as from 2.5, kernels no longer have an init_tasks structure
 * so we need some other way of telling a new secondary core
 * where to place its SVC stack
 */
struct secondary_data secondary_data;

enum ipi_msg_type {
	IPI_TIMER = 2,
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CALL_FUNC_SINGLE,
	IPI_CPU_STOP,
	IPI_BACKTRACE,
};

int __cpuinit __cpu_up(unsigned int cpu)
{
	struct cpuinfo_arm *ci = &per_cpu(cpu_data, cpu);
	struct task_struct *idle = ci->idle;
	pgd_t *pgd;
	int ret;

	/*
	 * Spawn a new process manually, if not already done.
	 * Grab a pointer to its task struct so we can mess with it
	 */
	if (!idle) {
		idle = fork_idle(cpu);
		if (IS_ERR(idle)) {
			printk(KERN_ERR "CPU%u: fork() failed\n", cpu);
			return PTR_ERR(idle);
		}
		ci->idle = idle;
	} else {
		/*
		 * Since this idle thread is being re-used, call
		 * init_idle() to reinitialize the thread structure.
		 */
		init_idle(idle, cpu);
	}

	/*
	 * Allocate initial page tables to allow the new CPU to
	 * enable the MMU safely.  This essentially means a set
	 * of our "standard" page tables, with the addition of
	 * a 1:1 mapping for the physical address of the kernel.
	 */
#ifndef CONFIG_EJ_SPLIT_PAGETABLE
	pgd = pgd_alloc(&init_mm);
#else
       pgd = init_mm.pgd;
#endif
	if (!pgd)
		return -ENOMEM;

	if (PHYS_OFFSET != PAGE_OFFSET) {
#ifndef CONFIG_HOTPLUG_CPU
		identity_mapping_add(pgd, __pa(__init_begin), __pa(__init_end));
#endif
		identity_mapping_add(pgd, __pa(_stext), __pa(_etext));
		identity_mapping_add(pgd, __pa(_sdata), __pa(_edata));
	}

	/*
	 * We need to tell the secondary core where to find
	 * its stack and the page tables.
	 */
	secondary_data.stack = task_stack_page(idle) + DEF_START_SP;
	secondary_data.pgdir = virt_to_phys(pgd);
	secondary_data.swapper_pg_dir = virt_to_phys(swapper_pg_dir);
	__cpuc_flush_dcache_area(&secondary_data, sizeof(secondary_data));
	outer_clean_range(__pa(&secondary_data), __pa(&secondary_data + 1));

	/*
	 * Now bring the CPU into our world.
	 */
	ret = boot_secondary(cpu, idle);
	if (ret == 0) {
		unsigned long timeout;

		/*
		 * CPU was successfully started, wait for it
		 * to come online or time out.
		 */
		timeout = jiffies + HZ;
		while (time_before(jiffies, timeout)) {
			if (cpu_online(cpu))
				break;
			udelay(10);
			barrier();
		}

		if (!cpu_online(cpu)) {
			pr_crit("CPU%u: failed to come online\n", cpu);
			ret = -EIO;
		}
	} else {
		pr_err("CPU%u: failed to boot: %d\n", cpu, ret);
	}

	secondary_data.stack = NULL;
	secondary_data.pgdir = 0;

	if (PHYS_OFFSET != PAGE_OFFSET) {
#ifndef CONFIG_HOTPLUG_CPU
		identity_mapping_del(pgd, __pa(__init_begin), __pa(__init_end));
#endif
		identity_mapping_del(pgd, __pa(_stext), __pa(_etext));
		identity_mapping_del(pgd, __pa(_sdata), __pa(_edata));
	}
#ifndef CONFIG_EJ_SPLIT_PAGETABLE
	pgd_free(&init_mm, pgd);
#endif

	return ret;
}

#ifdef CONFIG_HOTPLUG_CPU
static void percpu_timer_stop(void);

/*
 * __cpu_disable runs on the processor to be shutdown.
 */
int __cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();
	struct task_struct *p;
	int ret;

	ret = platform_cpu_disable(cpu);
	if (ret)
		return ret;

	/*
	 * Take this CPU offline.  Once we clear this, we can't return,
	 * and we must not schedule until we're ready to give up the cpu.
	 */
	set_cpu_online(cpu, false);

#ifndef CONFIG_EJ_DO_NOT_MIGRATE_IRQS
	/*
	 * OK - migrate IRQs away from this CPU
	 */
	migrate_irqs();
#endif

	/*
	 * Stop the local timer for this CPU.
	 */
	percpu_timer_stop();

	/*
	 * Flush user cache and TLB mappings, and then remove this CPU
	 * from the vm mask set of all processes.
	 */
	flush_cache_all();
	local_flush_tlb_all();

#ifndef CONFIG_PREEMPT_RT_FULL
	read_lock(&tasklist_lock);
#endif
	for_each_process(p) {
		if (p->mm)
			cpumask_clear_cpu(cpu, mm_cpumask(p->mm));
	}
#ifndef CONFIG_PREEMPT_RT_FULL
	read_unlock(&tasklist_lock);
#endif

	return 0;
}

#ifndef CONFIG_PREEMPT_RT_FULL
static DECLARE_COMPLETION(cpu_died);
#else
static DEFINE_PER_CPU(int, is_cpu_dead);
static DEFINE_PER_CPU(struct task_struct *, cpu_killer_thread);

extern int cpu_hotplug_active(void);

static int
hotplug_arch_sync_sanitizer(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	int cpu = (int)(long)hcpu;

	switch (action) {
	case CPU_DOWN_PREPARE:
			per_cpu(cpu_killer_thread, cpu) = NULL;
			per_cpu(is_cpu_dead, cpu) = 0;
	}

	return NOTIFY_DONE;
}

int __cpuinit hotplug_arch_sync_sanitizer_init(void){
	hotcpu_notifier(hotplug_arch_sync_sanitizer, 0);
	return 0;
}
core_initcall(hotplug_arch_sync_sanitizer_init);
#endif

/*
 * called on the thread which is asking for a CPU to be shutdown -
 * waits until shutdown has completed, or it is timed out.
 */
void __cpu_die(unsigned int cpu)
{

#ifndef CONFIG_PREEMPT_RT_FULL
	if (!wait_for_completion_timeout(&cpu_died, msecs_to_jiffies(5000))) {
		pr_err("CPU%u: cpu didn't die\n", cpu);
		return;
	}
#else
	int count = 0;

	per_cpu(cpu_killer_thread, cpu) = current;

	while(count++ < 17)
		if(!per_cpu(is_cpu_dead, cpu))
			schedule_timeout_uninterruptible(msecs_to_jiffies(300));

	if(!per_cpu(is_cpu_dead, cpu)){
		pr_err("CPU%u: cpu didn't die\n", cpu);
		return;
	}

	per_cpu(is_cpu_dead, cpu) = 0;
#endif

	printk(KERN_NOTICE "CPU%u: shutdown\n", cpu);

	if (!platform_cpu_kill(cpu))
		printk("CPU%u: unable to kill\n", cpu);
}

/*
 * Called from the idle thread for the CPU which has been shutdown.
 *
 * Note that we disable IRQs here, but do not re-enable them
 * before returning to the caller. This is also the behaviour
 * of the other hotplug-cpu capable cores, so presumably coming
 * out of idle fixes this.
 */
void __ref cpu_die(void)
{
	unsigned int cpu = smp_processor_id();

	idle_task_exit();

	local_irq_disable();
	mb();

	/* Tell __cpu_die() that this CPU is now safe to dispose of */
#ifndef CONFIG_PREEMPT_RT_FULL
	complete_nb(&cpu_died);
#else
	per_cpu(is_cpu_dead, cpu) = 1;
	while(1) {
		if (!cpu_hotplug_active())
			break;

		if(per_cpu(cpu_killer_thread, cpu)){
			wake_up_process(per_cpu(cpu_killer_thread, cpu));
			per_cpu(cpu_killer_thread, cpu) = NULL;
			break;
		}
    }
#endif

	/*
	 * actual CPU shutdown procedure is at least platform (if not
	 * CPU) specific.
	 */
	platform_cpu_die(cpu);

	/*
	 * Do not return to the idle loop - jump back to the secondary
	 * cpu initialisation.  There's some initialisation which needs
	 * to be repeated to undo the effects of taking the CPU offline.
	 */
	__asm__("mov	sp, %0\n"
	"	mov	fp, #0\n"
	"	b	secondary_start_kernel"
		:
		: "r" (task_stack_page(current) + calc_start_sp(current)));
}
#endif /* CONFIG_HOTPLUG_CPU */

/*
 * Called by both boot and secondaries to move global data into
 * per-processor storage.
 */
static void __cpuinit smp_store_cpu_info(unsigned int cpuid)
{
	struct cpuinfo_arm *cpu_info = &per_cpu(cpu_data, cpuid);

	cpu_info->loops_per_jiffy = loops_per_jiffy;
}

static DEFINE_RAW_SPINLOCK(calibrate_delay_lock);
/*
 * This is the secondary CPU boot entry.  We're using this CPUs
 * idle thread stack, but a set of temporary page tables.
 */
asmlinkage void __cpuinit secondary_start_kernel(void)
{
	struct mm_struct *mm = &init_mm;
	unsigned int cpu;

	/*
	 * The identity mapping is uncached (strongly ordered), so
	 * switch away from it before attempting any exclusive accesses.
	 */
	cpu_switch_mm(mm->pgd, mm);
	enter_lazy_tlb(mm, current);
	local_flush_tlb_all();

	/*
	 * All kernel threads share the same mm context; grab a
	 * reference and switch to it.
	 */
	cpu = smp_processor_id();
	atomic_inc(&mm->mm_count);
	current->active_mm = mm;
	cpumask_set_cpu(cpu, mm_cpumask(mm));

	printk("CPU%u: Booted secondary processor\n", cpu);

	cpu_init();
	preempt_disable();
	trace_hardirqs_off();

	/*
	 * Give the platform a chance to do its own initialisation.
	 */
	platform_secondary_init(cpu);

	/*
	 * Enable local interrupts.
	 */
	notify_cpu_starting(cpu);

	/*
	 * OK, now it's safe to let the boot CPU continue.  Wait for
	 * the CPU migration code to notice that the CPU is online
	 * before we continue. We need to do that before we enable
	 * interrupts otherwise a wakeup of a kernel thread affine to
	 * this CPU might break the affinity and let hell break lose.
	 */
	set_cpu_online(cpu, true);
	while (!cpu_active(cpu))
		cpu_relax();

	local_irq_enable();
	local_fiq_enable();

	/*
	 * Setup the percpu timer for this CPU.
	 */
	percpu_timer_setup();

	/*
	 * calibrate_delay() sets global loops_per_jiffy
	 * smp_store_cpu_info() saves a per cpu copy of if
	 */
	raw_spin_lock(&calibrate_delay_lock);

	calibrate_delay();

	smp_store_cpu_info(cpu);

	raw_spin_unlock(&calibrate_delay_lock);

	/*
	 * OK, it's off to the idle thread for us
	 */
	cpu_idle();
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	printk(KERN_INFO "SMP: Total of %d processors activated.\n",
	       num_online_cpus());
}

void __init smp_prepare_boot_cpu(void)
{
	unsigned int cpu = smp_processor_id();

	per_cpu(cpu_data, cpu).idle = current;
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int ncores = num_possible_cpus();

	smp_store_cpu_info(smp_processor_id());

	/*
	 * are we trying to boot more cores than exist?
	 */
	if (max_cpus > ncores)
		max_cpus = ncores;
	if (ncores > 1 && max_cpus) {
		/*
		 * Enable the local timer or broadcast device for the
		 * boot CPU, but only if we have more than one CPU.
		 */
		percpu_timer_setup();

		/*
		 * Initialise the present map, which describes the set of CPUs
		 * actually populated at the present time. A platform should
		 * re-initialize the map in platform_smp_prepare_cpus() if
		 * present != possible (e.g. physical hotplug).
		 */
		init_cpu_present(&cpu_possible_map);

		/*
		 * Initialise the SCU if there are more than one CPU
		 * and let them know where to start.
		 */
		platform_smp_prepare_cpus(max_cpus);
	}
}

static void (*smp_cross_call)(const struct cpumask *, unsigned int);

void __init set_smp_cross_call(void (*fn)(const struct cpumask *, unsigned int))
{
	smp_cross_call = fn;
}

void arch_send_call_function_ipi_mask(const struct cpumask *mask)
{
	smp_cross_call(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_CALL_FUNC_SINGLE);
}

static const char *ipi_types[NR_IPI] = {
#define S(x,s)	[x - IPI_TIMER] = s
	S(IPI_TIMER, "Timer broadcast interrupts"),
	S(IPI_RESCHEDULE, "Rescheduling interrupts"),
	S(IPI_CALL_FUNC, "Function call interrupts"),
	S(IPI_CALL_FUNC_SINGLE, "Single function call interrupts"),
	S(IPI_CPU_STOP, "CPU stop interrupts"),
	S(IPI_BACKTRACE, "Trigger all cpu backtrace"),
};

void show_ipi_list(struct seq_file *p, int prec)
{
	unsigned int cpu, i;

	for (i = 0; i < NR_IPI; i++) {
		seq_printf(p, "%*s%u: ", prec - 1, "IPI", i);

		for_each_present_cpu(cpu)
			seq_printf(p, "%10u ",
				   __get_irq_stat(cpu, ipi_irqs[i]));

		seq_printf(p, " %s\n", ipi_types[i]);
	}
}

u64 smp_irq_stat_cpu(unsigned int cpu)
{
	u64 sum = 0;
	int i;

	for (i = 0; i < NR_IPI; i++)
		sum += __get_irq_stat(cpu, ipi_irqs[i]);

#ifdef CONFIG_LOCAL_TIMERS
	sum += __get_irq_stat(cpu, local_timer_irqs);
#endif

	return sum;
}

/*
 * Timer (local or broadcast) support
 */
static DEFINE_PER_CPU(struct clock_event_device, percpu_clockevent);

static void ipi_timer(void)
{
	struct clock_event_device *evt = &__get_cpu_var(percpu_clockevent);
	evt->event_handler(evt);
}

#ifdef CONFIG_LOCAL_TIMERS
#ifdef CONFIG_SNSC_IRQ_OVERHEAD_ONCE
           void __exception do_local_timer(struct pt_regs *regs)
#else
asmlinkage void __exception_irq_entry do_local_timer(struct pt_regs *regs)
#endif
{
	struct pt_regs *old_regs = set_irq_regs(regs);
#ifdef CONFIG_SNSC_LOCAL_TIMER_AVOID_WAKEUP_SOFTIRQ
	struct clock_event_device *evt = &__get_cpu_var(percpu_clockevent);
#endif
	int cpu = smp_processor_id();

	trace_off_mark_irq(0x200, 0);

	irq_handler_rt_trace_enter(NR_IRQ_LOC);

	if (local_timer_ack()) {
#if defined(CONFIG_ARCH_CXD90014)
		extern int debug_tick_timing;
		if (debug_tick_timing & 1) {
			BOOT_TIME_ADD1("localtimer");
		}
#endif
		__inc_irq_stat(cpu, local_timer_irqs);
#ifdef CONFIG_SNSC_LOCAL_TIMER_AVOID_WAKEUP_SOFTIRQ
		/*
		 * wakeup_softirqd() will be called in this sequence:
		 *    local_timer_interrupt()
		 *       run_local_timers()
		 *          raise_softirq()
		 *             wakeup_softirqd()
		 *
		 * Avoid a second call to wakeup_softirq() from irq_exit().
		 */
		irq_enter();
		evt->event_handler(evt);
		irq_exit_no_invoke_softirq();
#else
		irq_enter();
		ipi_timer();
		irq_exit();
#endif
	}

	irq_handler_rt_trace_exit(NR_IRQ_LOC);

	trace_off_mark_irq(0x2ff, 0);

	set_irq_regs(old_regs);
}

void show_local_irqs(struct seq_file *p, int prec)
{
	unsigned int cpu;

	seq_printf(p, "%*s: ", prec, "LOC");

	for_each_present_cpu(cpu)
		seq_printf(p, "%10u ", __get_irq_stat(cpu, local_timer_irqs));

	seq_printf(p, " Local timer interrupts\n");
}
#endif

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
static void smp_timer_broadcast(const struct cpumask *mask)
{
	smp_cross_call(mask, IPI_TIMER);
}
#else
#define smp_timer_broadcast	NULL
#endif

static void broadcast_timer_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evt)
{
}

static void __cpuinit broadcast_timer_setup(struct clock_event_device *evt)
{
	evt->name	= "dummy_timer";
	evt->features	= CLOCK_EVT_FEAT_ONESHOT |
			  CLOCK_EVT_FEAT_PERIODIC |
			  CLOCK_EVT_FEAT_DUMMY;
	evt->rating	= 400;
	evt->mult	= 1;
	evt->set_mode	= broadcast_timer_set_mode;

	clockevents_register_device(evt);
}

void __cpuinit percpu_timer_setup(void)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(percpu_clockevent, cpu);

	evt->cpumask = cpumask_of(cpu);
	evt->broadcast = smp_timer_broadcast;

	if (local_timer_setup(evt))
		broadcast_timer_setup(evt);
}

#ifdef CONFIG_HOTPLUG_CPU
/*
 * The generic clock events code purposely does not stop the local timer
 * on CPU_DEAD/CPU_DEAD_FROZEN hotplug events, so we have to do it
 * manually here.
 */
static void percpu_timer_stop(void)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(percpu_clockevent, cpu);

	evt->set_mode(CLOCK_EVT_MODE_UNUSED, evt);
}
#endif

static DEFINE_RAW_SPINLOCK(stop_lock);

/*
 * ipi_cpu_stop - handle IPI from smp_send_stop()
 */
static void ipi_cpu_stop(unsigned int cpu)
{
	if (system_state == SYSTEM_BOOTING ||
	    system_state == SYSTEM_RUNNING) {
		raw_spin_lock(&stop_lock);
		printk(KERN_CRIT "CPU%u: stopping\n", cpu);
		dump_stack();
		raw_spin_unlock(&stop_lock);
	}

	set_cpu_online(cpu, false);

	local_fiq_disable();
	local_irq_disable();

	while (1)
		cpu_relax();
}

#ifdef CONFIG_EJ_ARCH_HAVE_SMP_CALL_FUNCTION
/*
 * Bring smp_call_function() from 2.6.23
 */
struct smp_call_struct {
	void (*func)(void *info);
	void *info;
	int wait;
	cpumask_var_t pending;
	cpumask_var_t unfinished;
};

static struct smp_call_struct * volatile smp_call_function_data;
static DEFINE_RAW_SPINLOCK(smp_call_function_lock);

/*
 * You must not call this function with disabled interrupts, from a
 * hardware interrupt handler, nor from a bottom half handler.
 */
void smp_call_function_many(const struct cpumask *mask,
			    void (*func)(void *), void *info, bool wait)
{
	struct smp_call_struct data;
	unsigned long timeout;
	int ret = 0;
	cpumask_var_t callmap;

	if (!zalloc_cpumask_var(&callmap, GFP_KERNEL))
		return;

	if (!zalloc_cpumask_var(&data.pending, GFP_KERNEL))
		goto free_callmap;

	if (!zalloc_cpumask_var(&data.unfinished, GFP_KERNEL))
		goto free_pending;

	cpumask_copy(callmap, mask);

	data.func = func;
	data.info = info;
	data.wait = wait;

	cpumask_clear_cpu(smp_processor_id(), callmap);
	if (cpumask_empty(callmap))
		goto out;


	cpumask_copy(data.pending, callmap);
	if (wait)
		cpumask_copy(data.unfinished, callmap);

	BUG_ON(irqs_disabled());

	/*
	 * disable softirq to avoid deadlock
	 */
	local_bh_disable();

	/*
	 * try to get the mutex on smp_call_function_data
	 */
	raw_spin_lock(&smp_call_function_lock);
	smp_call_function_data = &data;

	smp_cross_call(callmap, IPI_CALL_FUNC);

	timeout = jiffies + HZ;
	while (!cpumask_empty(data.pending) && time_before(jiffies, timeout))
		barrier();

	/*
	 * did we time out?
	 */
	if (!cpumask_empty(data.pending)) {
		/*
		 * this may be causing our panic - report it
		 */
		printk(KERN_CRIT
		       "CPU%u: smp_call_function timeout for %p(%p)\n"
		       "      callmap %lx pending %lx, %swait\n",
		       smp_processor_id(), func, info, cpus_addr(*callmap),
		       cpus_addr(*data.pending), wait ? "" : "no ");

		/*
		 * TRACE
		 */
	retry:
		timeout = jiffies + (5 * HZ);
		while (!cpumask_empty(data.pending) && time_before(jiffies, timeout))
			barrier();

		if (cpumask_empty(data.pending))
			printk(KERN_CRIT "     RESOLVED\n");
		else {
			printk(KERN_CRIT "     STILL STUCK\n");
			printk(KERN_CRIT "     Try Again\n");
			goto retry;
		}
	}

	/*
	 * whatever happened, we're done with the data, so release it
	 */
	smp_call_function_data = NULL;
	raw_spin_unlock(&smp_call_function_lock);

	local_bh_enable();

	if (!cpumask_empty(data.pending)) {
		ret = -ETIMEDOUT;
		goto out;
	}

	if (wait)
		while (!cpumask_empty(data.unfinished))
			barrier();

 out:

 free_unfinished:
	free_cpumask_var(data.unfinished);
 free_pending:
	free_cpumask_var(data.pending);
 free_callmap:
	free_cpumask_var(callmap);


	return;
}
EXPORT_SYMBOL(smp_call_function_many);

int smp_call_function(void (*func)(void *info), void *info, int wait)
{
	smp_call_function_many(cpu_online_mask, func, info, wait);

	return 0;
}
EXPORT_SYMBOL_GPL(smp_call_function);

int smp_call_function_single(int cpu, void (*func)(void *info), void *info,
			     int wait)
{
	/* prevent preemption and reschedule on another processor */
	int current_cpu = get_cpu();
	int ret = 0;

	if (cpu == current_cpu) {
		local_irq_disable();
		func(info);
		local_irq_enable();
	} else
		smp_call_function_many(get_cpu_mask(cpu),
				       func, info, wait);

	put_cpu();

	return ret;
}
EXPORT_SYMBOL_GPL(smp_call_function_single);

/*
 * for caller who has a pre-allocated data structure
 */
void __smp_call_function_single(int cpu, struct call_single_data *data,
				int wait)
{
	/* Can deadlock when called with interrupts disabled */
	WARN_ON_ONCE(wait && irqs_disabled() && !oops_in_progress);

	smp_call_function_single(cpu, data->func, data->info, wait);
}

/*
 * ipi_call_function - handle IPI from smp_call_function()
 *
 * Note that we copy data out of the cross-call structure and then
 * let the caller know that we're here and have done with their data
 */
static void ipi_call_function(unsigned int cpu)
{
	struct smp_call_struct *data = smp_call_function_data;
	void (*func)(void *info) = data->func;
	void *info = data->info;
	int wait = data->wait;

	cpumask_clear_cpu(cpu, data->pending);

	trace_off_mark_irq(0x300, (unsigned long)func);
	func(info);
	trace_off_mark_irq(0x3ff, 0);

	if (wait)
		cpumask_clear_cpu(cpu, data->unfinished);
}
#endif /* CONFIG_EJ_ARCH_HAVE_SMP_CALL_FUNCTION */

#ifdef arch_trigger_all_cpu_backtrace
/*
 * Based on arch/x86/kernel/apic/hw_nmi.c:
 *   arch_trigger_all_cpu_backtrace_handler()
 *   arch_trigger_all_cpu_backtrace()
 */

// xxx static cpumask_t backtrace_mask;
static struct cpumask backtrace_mask;

static unsigned long backtrace_flag;
static arch_spinlock_t smp_backtrace_lock = __ARCH_SPIN_LOCK_UNLOCKED;

/*
 * ipi_backtrace - handle IPI from smp_send_backtrace()
 */
static void ipi_backtrace(unsigned int cpu, struct pt_regs *regs)
{
	/*
	 * serialize cpus
	 */
	arch_spin_lock(&smp_backtrace_lock);

	pr_crit("\nCPU %u\n", cpu);

#if 1
	printk("\n");
	printk("Pid: %d, comm: %20s\n", task_pid_nr(current), current->comm);

	if (regs)
		__show_regs(regs);

	dump_stack();
#else
	/*
	 * 3.6.7-rt18 can use this form, because show_regs()
	 * calls dump_stack().
	 */
	if (regs)
		show_regs(regs);
	else
		dump_stack();
#endif

	cpumask_clear_cpu(cpu, &backtrace_mask);

	arch_spin_unlock(&smp_backtrace_lock);
}

void arch_trigger_all_cpu_backtrace(void)
{
	int i;

	if (test_and_set_bit(0, &backtrace_flag))
		/*
		 * If there is already a trigger_all_cpu_backtrace() in progress
		 * (backtrace_flag == 1), don't output double cpu dump infos.
		 */
		return;

	ipi_backtrace(smp_processor_id(), get_irq_regs());

	cpumask_copy(&backtrace_mask, cpu_online_mask);
	cpumask_clear_cpu(smp_processor_id(), &backtrace_mask);

	if (!cpumask_empty(&backtrace_mask))
		smp_cross_call(&backtrace_mask, IPI_BACKTRACE);

	/* Wait for up to 10 seconds for all CPUs to do the backtrace */
	for (i = 0; i < 10 * 1000; i++) {
		if (cpumask_empty(&backtrace_mask))
			break;
		mdelay(1);
	}

	clear_bit(0, &backtrace_flag);
}
#endif

/*
 * Main handler for inter-processor interrupts
 */
#ifdef CONFIG_SNSC_IRQ_OVERHEAD_ONCE
           void __exception_irq_entry do_IPI(int ipinr, struct pt_regs *regs)
#else
asmlinkage void __exception_irq_entry do_IPI(int ipinr, struct pt_regs *regs)
#endif
{
	unsigned int cpu = smp_processor_id();
	struct pt_regs *old_regs = set_irq_regs(regs);

 	irq_handler_rt_trace_enter(NR_IRQ_IPI);
	if (ipinr >= IPI_TIMER && ipinr < IPI_TIMER + NR_IPI)
		__inc_irq_stat(cpu, ipi_irqs[ipinr - IPI_TIMER]);

	trace_off_mark_irq(0x400, ipinr);

	switch (ipinr) {
	case IPI_TIMER:
		trace_off_mark_irq_dtl(0x410, ipinr);
		irq_enter();
		ipi_timer();
		irq_exit();
		trace_off_mark_irq_dtl(0x41f, 0);
		break;

	case IPI_RESCHEDULE:
		trace_off_mark_irq_dtl(0x450, ipinr);
		scheduler_ipi();
		trace_off_mark_irq_dtl(0x45f, 0);
		break;

#ifndef CONFIG_EJ_ARCH_HAVE_SMP_CALL_FUNCTION
	case IPI_CALL_FUNC:
		trace_off_mark_irq_dtl(0x420, ipinr);
		irq_enter();
		generic_smp_call_function_interrupt();
		irq_exit();
		trace_off_mark_irq_dtl(0x42f, 0);
		break;

	case IPI_CALL_FUNC_SINGLE:
		trace_off_mark_irq_dtl(0x430, ipinr);
		irq_enter();
		generic_smp_call_function_single_interrupt();
		irq_exit();
		trace_off_mark_irq_dtl(0x43f, 0);
		break;

#else
	case IPI_CALL_FUNC:
		ipi_call_function(cpu);
		break;

	case IPI_CALL_FUNC_SINGLE:
		ipi_call_function(cpu);
		break;
#endif /* CONFIG_EJ_ARCH_HAVE_SMP_CALL_FUNCTION */

	case IPI_CPU_STOP:
		trace_off_mark_irq_dtl(0x440, ipinr);
		irq_enter();
		ipi_cpu_stop(cpu);
		irq_exit();
		trace_off_mark_irq_dtl(0x44f, 0);
		break;

	case IPI_BACKTRACE:
		trace_off_mark_irq_dtl(0x460, ipinr);
		ipi_backtrace(cpu, regs);
		trace_off_mark_irq_dtl(0x46f, 0);
		break;

	default:
		printk(KERN_CRIT "CPU%u: Unknown IPI message 0x%x\n",
		       cpu, ipinr);
		break;
	}

	irq_handler_rt_trace_exit(NR_IRQ_IPI);
	trace_off_mark_irq(0x4ff, ipinr);

	set_irq_regs(old_regs);
}

void smp_send_reschedule(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_RESCHEDULE);
}

/*
 * this function sends a 'reschedule' IPI to all other CPUs.
 * This is used when RT tasks are starving and other CPUs
 * might be able to run them:
 */
void smp_send_reschedule_allbutself(void)
{
        unsigned int cpu;

        for_each_online_cpu(cpu) {
                preempt_disable();
                if (cpu != smp_processor_id())
                        smp_send_reschedule(cpu);
                preempt_enable();
        }
}

void smp_send_stop(void)
{
	unsigned long timeout;

	if (num_online_cpus() > 1) {
		cpumask_t mask = cpu_online_map;
		cpu_clear(smp_processor_id(), mask);

		smp_cross_call(&mask, IPI_CPU_STOP);
	}

	/* Wait up to one second for other CPUs to stop */
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		pr_warning("SMP: failed to stop secondary CPUs\n");
}

/*
 * not supported here
 */
int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}
