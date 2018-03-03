/* 2009-02-12: File changed by Sony Corporation */
/*
 *  linux/arch/arm/kernel/smp_tlb.c
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/preempt.h>
#include <linux/smp.h>

#include <asm/smp_plat.h>
#include <asm/tlbflush.h>
#include <linux/module.h>
#include <linux/snsc_workqueue.h>

#define dprintk pr_debug

#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED
struct cpu_work {
	struct snsc_work_struct work;
	void (*func)(void *);
	void *info;
	struct completion done;
};

static void cpu_work_thread(struct snsc_work_struct *work)
{
	struct cpu_work *cpu_work = container_of(work, struct cpu_work, work);
	dprintk("%s:cpu=%d\n", __func__, raw_smp_processor_id());
	cpu_work->func(cpu_work->info);
	complete(&cpu_work->done);
}


static int
__on_each_cpu_mask_thread(void (*func)(void *), void *info, int wait,
		 cpumask_t cpumask)
{
	int cpu;
	int this_cpu;
	int this_cpu_on = 0;
	struct cpu_work works[NR_CPUS];

	dprintk("%s:cpu=%d cpus=%d start\n", __func__, raw_smp_processor_id(), cpus_weight(cpumask));
	this_cpu = raw_smp_processor_id();
	if (cpu_isset(this_cpu, cpumask)) {
		this_cpu_on = 1;
		cpu_clear(this_cpu, cpumask);
	}

	for_each_cpu_mask(cpu, cpumask) {
		SNSC_INIT_WORK(&works[cpu].work, cpu_work_thread);
		init_completion(&works[cpu].done);
		works[cpu].func = func;
		works[cpu].info = info;
		snsc_schedule_work_cpu(&works[cpu].work, cpu);
	}

	if (this_cpu_on) {
		func(info);
	}

	for_each_cpu_mask(cpu, cpumask) {
		wait_for_completion(&works[cpu].done);
	}

	dprintk("%s:cpu=%d end\n", __func__, raw_smp_processor_id());

	return 0;
}

static int
__on_each_cpu_mask_atomic(void (*func)(void *), void *info, int wait,
		 cpumask_t cpumask)
{
	unsigned long flags;

	smp_call_function_many(&cpumask, func, info, wait);
	local_irq_save(flags);
	if (cpu_isset(smp_processor_id(), cpumask))
		func(info);
	local_irq_restore(flags);

	return 0;
}

static int
__on_each_cpu_mask(void (*func)(void *), void *info, int wait,
		 cpumask_t cpumask)
{
	int ret;

	if (in_atomic() || irqs_disabled() || system_state < SYSTEM_RUNNING) {
#ifndef CONFIG_EJ
		if (system_state == SYSTEM_RUNNING)
			WARN_ON_ONCE("from atomic\n");
#endif /* !CONFIG_EJ */
		ret = __on_each_cpu_mask_atomic(func, info, wait, cpumask);

	}
	else {
		migrate_disable();
#ifndef CONFIG_EJ_THREADED_CPU_BROADCAST
		ret = __on_each_cpu_mask_atomic(func, info, wait, cpumask);
#else
		ret = __on_each_cpu_mask_thread(func, info, wait, cpumask);
#endif
		migrate_enable();
	}
	return ret;
}

static int
on_each_cpu_mm(void (*func)(void *), void *info, int wait,
		 struct mm_struct *mm)
{
	cpumask_t cpumask;
	cpumask_copy(&cpumask, mm_cpumask(mm));

	if (unlikely(mm != current->active_mm))
		cpumask = cpu_online_map;
	else
		cpumask_and(&cpumask, &cpumask, cpu_online_mask);

	return __on_each_cpu_mask(func, info, wait, cpumask);
}

#ifndef CONFIG_EJ_THREADED_CPU_BROADCAST_ALL
static int
__on_each_cpu_mask_fast(void (*func)(void *), void *info, int wait,
			cpumask_t cpumask)
{
	int ret;

	migrate_disable();

	ret = __on_each_cpu_mask_atomic(func, info, wait, cpumask);

	migrate_enable();

	return ret;
}

static int
on_each_cpu_mm_fast(void (*func)(void *), void *info, int wait,
		    struct mm_struct *mm)
{
	cpumask_t cpumask;
	cpumask_copy(&cpumask, mm_cpumask(mm));

	if (unlikely(mm != current->active_mm))
		cpumask = cpu_online_map;
	else
		cpumask_and(&cpumask, &cpumask, cpu_online_mask);

	return __on_each_cpu_mask_fast(func, info, wait, cpumask);
}
#endif

/*
 * Call a function on all processors
 */
int on_each_cpu(void (*func) (void *info), void *info, int wait)
{
	return __on_each_cpu_mask(func, info, wait, cpu_online_map);
}
EXPORT_SYMBOL(on_each_cpu);

#else /* CONFIG_EJ_ON_EACH_CPU_THREADED */

static int
on_each_cpu_mm(void (*func)(void *), void *info, int wait,
		 struct mm_struct *mm)
{

	if (unlikely(mm != current->active_mm))
		return on_each_cpu(func, info, wait);

	preempt_disable();

	smp_call_function_many(mm_cpumask(mm), func, info, wait);
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(mm)))
		func(info);

	preempt_enable();
	return 0;
}
#endif /* CONFIG_EJ_ON_EACH_CPU_THREADED */

/**********************************************************************/

/*
 * TLB operations
 */
struct tlb_args {
	struct vm_area_struct *ta_vma;
	unsigned long ta_start;
	unsigned long ta_end;
};

static inline void ipi_flush_tlb_all(void *ignored)
{
	local_flush_tlb_all();
}

static inline void ipi_flush_tlb_mm(void *arg)
{
	struct mm_struct *mm = (struct mm_struct *)arg;

	local_flush_tlb_mm(mm);
}

static inline void ipi_flush_tlb_page(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_page(ta->ta_vma, ta->ta_start);
}

static inline void ipi_flush_tlb_kernel_page(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_kernel_page(ta->ta_start);
}

static inline void ipi_flush_tlb_range(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_range(ta->ta_vma, ta->ta_start, ta->ta_end);
}

static inline void ipi_flush_tlb_kernel_range(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_kernel_range(ta->ta_start, ta->ta_end);
}

void flush_tlb_all(void)
{
	if (tlb_ops_need_broadcast())
		on_each_cpu(ipi_flush_tlb_all, NULL, 1);
	else
		local_flush_tlb_all();
}

void flush_tlb_mm(struct mm_struct *mm)
{
	if (tlb_ops_need_broadcast())
#if defined(CONFIG_EJ_THREADED_CPU_BROADCAST_ALL) || !defined(CONFIG_EJ_ON_EACH_CPU_THREADED)
		on_each_cpu_mm(ipi_flush_tlb_mm, mm, 1, mm);
#else
		on_each_cpu_mm_fast(ipi_flush_tlb_mm, mm, 1, mm);
#endif
	else
		local_flush_tlb_mm(mm);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_vma = vma;
		ta.ta_start = uaddr;
#if defined(CONFIG_EJ_THREADED_CPU_BROADCAST_ALL) || !defined(CONFIG_EJ_ON_EACH_CPU_THREADED)
		on_each_cpu_mm(ipi_flush_tlb_page, &ta, 1, vma->vm_mm);
#else
		on_each_cpu_mm_fast(ipi_flush_tlb_page, &ta, 1, vma->vm_mm);
#endif
	} else
		local_flush_tlb_page(vma, uaddr);
}

void flush_tlb_kernel_page(unsigned long kaddr)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_start = kaddr;
		on_each_cpu(ipi_flush_tlb_kernel_page, &ta, 1);
	} else
		local_flush_tlb_kernel_page(kaddr);
}

void flush_tlb_range(struct vm_area_struct *vma,
                     unsigned long start, unsigned long end)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_vma = vma;
		ta.ta_start = start;
		ta.ta_end = end;
		on_each_cpu_mm(ipi_flush_tlb_range, &ta, 1, vma->vm_mm);
	} else
		local_flush_tlb_range(vma, start, end);
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_start = start;
		ta.ta_end = end;
		on_each_cpu(ipi_flush_tlb_kernel_range, &ta, 1);
	} else
		local_flush_tlb_kernel_range(start, end);
}

