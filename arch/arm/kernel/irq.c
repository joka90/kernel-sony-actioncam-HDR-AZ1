/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/arch/arm/kernel/irq.c
 *
 *  Copyright (C) 1992 Linus Torvalds
 *  Modifications for ARM processor Copyright (C) 1995-2000 Russell King.
 *
 *  Support for Dynamic Tick Timer Copyright (C) 2004-2005 Nokia Corporation.
 *  Dynamic Tick Timer written by Tony Lindgren <tony@atomide.com> and
 *  Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains the code used by various IRQ handling routines:
 *  asking for different IRQ's should be done through these routines
 *  instead of just grabbing them. Thus setups with different IRQ numbers
 *  shouldn't result in any weird surprises, and installing new handlers
 *  should be easier.
 *
 *  IRQ's are in fact implemented a bit like signal handlers for the kernel.
 *  Naturally it's not a 1:1 relation, but there are similarities.
 */
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/random.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/percpu.h>
#include <linux/ftrace.h>
#include <linux/stack-ext.h>

#include <asm/localtimer.h>
#include <asm/system.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <linux/rt_trace_lite.h>
#include <linux/rt_trace_lite_irq.h>
#ifdef CONFIG_SNSC_VFP_DETECTION_IRQ
#include <asm/vfp_detect.h>
#endif

/*
 * No architecture-specific irq_finish function defined in arm/arch/irqs.h.
 */
#ifndef irq_finish
#define irq_finish(irq) do { } while (0)
#endif

unsigned long irq_err_count;

#ifdef CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE

static DEFINE_PER_CPU(unsigned long, count_irq);
static DEFINE_PER_CPU(unsigned long, count_irq_enter);
#ifdef CONFIG_SMP
static DEFINE_PER_CPU(unsigned long, ipi_lt_without_irq);
static DEFINE_PER_CPU(unsigned long, in_ipi_count);
unsigned long get_ipi_count(int);
#endif

static void show_irq_ipi_lt(struct seq_file *p)
{
	int cpu;
#ifdef CONFIG_SMP
	unsigned long ipi_lt;
	unsigned long ipi_lt_no_irq;
#endif

	seq_putc(p, '\n');

#ifdef CONFIG_SMP
	seq_printf(p, "ipi/lt                         ");
	for_each_present_cpu(cpu) {
		ipi_lt = smp_irq_stat_cpu(cpu);
		seq_printf(p, " %10lu", ipi_lt);
	}
	seq_putc(p, '\n');

	seq_printf(p, "ipi/lt with no external irq    ");
	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu", per_cpu(ipi_lt_without_irq, cpu));
	seq_putc(p, '\n');

	seq_printf(p, "                               ");
	for_each_present_cpu(cpu) {
		ipi_lt = smp_irq_stat_cpu(cpu);
		ipi_lt_no_irq = per_cpu(ipi_lt_without_irq, cpu);
		seq_printf(p, " %9lu%%",
			(ipi_lt_no_irq * 100) / ((ipi_lt) ? ipi_lt : 1));
	}
	seq_putc(p, '\n');
	seq_printf(p, "(these ipi/lt have more overhead with CONFIG_SNSC_IRQ_OVERHEAD_ONCE)\n\n");

	seq_printf(p, "IPI pending on do_IPI() return ");
	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu", per_cpu(in_ipi_count, cpu));
	seq_putc(p, '\n');
	seq_printf(p, "(these would have less overhead if ack_IPI_irq() moved to do_IPI())\n\n");
#endif
	seq_printf(p, "external irq                   ");
	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu", per_cpu(count_irq, cpu));
	seq_putc(p, '\n');


	seq_printf(p, "irq_enter() calls              ");
	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu", per_cpu(count_irq_enter, cpu));
	seq_putc(p, '\n');


	seq_printf(p, "irq_enter() calls avoided:     ");
	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu",
			per_cpu(count_irq, cpu) -
			per_cpu(count_irq_enter, cpu));
	seq_putc(p, '\n');


	seq_printf(p, "                               ");
	for_each_present_cpu(cpu) {
		count_irq       = per_cpu(count_irq,       cpu);
		count_irq_enter = per_cpu(count_irq_enter, cpu);
		seq_printf(p, " %9lu%%",
			((count_irq - count_irq_enter) * 100) /
			((count_irq) ? count_irq : 1));
	}
	seq_putc(p, '\n');
}
#endif


void __handle_irq(unsigned int irq)
{
#ifdef CONFIG_THREAD_MONITOR
	s64 t1 = sched_clock();
#endif

	generic_handle_irq(irq);

#ifdef CONFIG_THREAD_MONITOR
	{
		int cpu = raw_smp_processor_id();

		if (trace_cpu & (1 << cpu)) {
			s64 t2 = sched_clock();
			u32 delta = t2 - t1;
			delta /= 1000;
			add_trace_entry(cpu, current, current, irq | (delta << 8));
		}
	}
#endif
}

int arch_show_interrupts(struct seq_file *p, int prec)
{
#ifdef CONFIG_FIQ
	show_fiq_list(p, prec);
#endif
#ifdef CONFIG_SMP
	show_ipi_list(p, prec);
#endif
#ifdef CONFIG_LOCAL_TIMERS
	show_local_irqs(p, prec);
#endif

#ifdef CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE
 		show_irq_ipi_lt(p);
#endif
	seq_printf(p, "%*s: %10lu\n", prec, "Err", irq_err_count);
	return 0;
}

/*
 * handle_IRQ handles all hardware IRQ's.  Decoded IRQs should
 * not come via this function.  Instead, they should provide their
 * own 'handler'.  Used by platform code implementing C-based 1st
 * level decoding.
 */
void handle_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();

	/*
	 * Some hardware gives randomly wrong interrupts.  Rather
	 * than crashing, do something sensible.
	 */
	if (unlikely(irq >= nr_irqs)) {
		if (printk_ratelimit())
			printk(KERN_WARNING "Bad IRQ%u\n", irq);
		ack_bad_irq(irq);
	} else {
#ifdef CONFIG_SNSC_VFP_DETECTION_IRQ
		vfp_disable_hw();
#endif
		__handle_irq_possibly_extending_stack(irq);
	}

	/* AT91 specific workaround */
	irq_finish(irq);

	irq_exit();
	set_irq_regs(old_regs);
}


#ifdef CONFIG_SNSC_DEBUG_IRQ_DURATION
int show_irq_stat(struct seq_file *p, void *v)
{
	int i = *(loff_t *) v, cpu;
	struct irqaction * action;
	unsigned long flags;

	if (i == 0) {
		char cpuname[12];

		seq_printf(p, "  ");
		for_each_present_cpu(cpu) {
			sprintf(cpuname, "CPU%d", cpu);
			seq_printf(p, " %10s               ", cpuname);
		}

		seq_putc(p, '\n');
		seq_printf(p, "     ");
		for_each_present_cpu(cpu) {
			seq_printf(p, "      count  min  avg  max");
		}
		seq_putc(p, '\n');
	}

	if (i < NR_IRQS) {

		raw_spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (!action)
			goto unlock;

		seq_printf(p, "%3d: ", i);

		show_rt_trace_irq_stat(p, i);

		seq_printf(p, "  %s", action->name);
		for (action = action->next; action; action = action->next)
			seq_printf(p, ", %s", action->name);

		seq_putc(p, '\n');
unlock:
		raw_spin_unlock_irqrestore(&irq_desc[i].lock, flags);

	} else if (i == NR_IRQ_IPI) {

		seq_printf(p, "IPI: ");
		show_rt_trace_irq_stat(p, i);
		seq_printf(p, "  do_IPI");
		seq_putc(p, '\n');

	} else if (i == NR_IRQ_LOC) {

		seq_printf(p, "LOC: ");
		show_rt_trace_irq_stat(p, i);
		seq_printf(p, "  do_local_timer");
		seq_putc(p, '\n');

	} else if (i == NR_IRQ_INV) {

		seq_printf(p, "INV: ");
		show_rt_trace_irq_stat(p, i);
		seq_printf(p, "  invalid irq");
		seq_putc(p, '\n');

	}
	return 0;
}
#endif


#ifdef CONFIG_SNSC_IRQ_OVERHEAD_ONCE

#include <mach/entry-macro.h>

asmlinkage void __exception notrace asm_do_IRQ_IPI_LT(struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	unsigned int irq, irqstat;
	int in_irq = 0;
#if defined(CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE) && defined(CONFIG_SMP)
	int had_external_interrupt = 0;
	int in_ipi = 0;
#endif

	/*
	 * Why call irq_exit() in cases IRQ_IS_IP and IRQ_IS_LOCALTIMER?
	 * waiting until falling out of the while loop would allow a scenario:
	 *
	 *   irq A:
	 *     irq_enter()
	 *     handle irq A
	 *   IPI:
	 *     handle IPI
	 *   irq B:
	 *     handle irq B
	 *   irq_exit()
	 *
	 * instead of:
	 *
	 *   irq A:
	 *     irq_enter()
	 *     handle irq A
	 *   IPI:
	 *     irq_exit()
	 *     handle IPI
	 *   irq B:
	 *     irq_enter()
	 *     handle irq B
	 *   irq_exit()
	 *
	 * which would save an irq_exit() and an irq_enter().  But that would
	 * put the IPI handling between the irq_enter() and the irq_exit().
	 * That might work with the current code, but any changes to do_IPI()
	 * and do_localtimer() that does not expect a subsequent call to
	 * irq_exit() could cause problems.  I am being exceedingly cautious
	 * to minimize the amount of code flow change in
	 * CONFIG_SNSC_IRQ_OVERHEAD_ONCE.
	 */

	get_irq_preamble();
	do {
		irqstat = get_irqstat();
		irq = irqstat & IRQ_MASK;

		if (IRQ_IS_DONE(irq)) {

			/* done, no more irqs */
			irq = 0;

		} else if (IRQ_IS_EXTERNAL_INTERRUPT(irq)) {
#ifdef CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE
			per_cpu(count_irq, smp_processor_id())++;
#ifdef CONFIG_SMP
			had_external_interrupt = 1;
#endif
#endif
			if (!in_irq) {
				in_irq = 1;
#ifdef CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE
				per_cpu(count_irq_enter, smp_processor_id())++;
#endif
				irq_enter();
			}

			/*
			 * Some hardware gives randomly wrong interrupts.
			 * Rather than crashing, do something sensible.
			 */
			if (unlikely(irq >= NR_IRQS)) {
				if (printk_ratelimit())
					printk(KERN_WARNING "Bad IRQ%u\n", irq);
				ack_bad_irq(irq);
			} else {
#ifdef CONFIG_SNSC_VFP_DETECTION_IRQ
				vfp_disable_hw();
#endif
				__handle_irq_possibly_extending_stack(irq);
			}

			/* AT91 specific workaround */
			irq_finish(irq);

#ifdef CONFIG_SMP
		} else if (IRQ_IS_IPI(irq)) {

#ifdef CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE
			if (in_ipi)
				per_cpu(in_ipi_count, smp_processor_id())++;
			else
				in_ipi = 1;
#endif

			if (in_irq) {
				in_irq = 0;
				irq_exit();
			}
			ack_IPI_irq(irqstat);
			do_IPI(irq, regs);
#endif

#ifdef CONFIG_LOCAL_TIMERS
		} else if (IRQ_IS_LOCALTIMER(irq)) {
			if (in_irq) {
				in_irq = 0;
				irq_exit();
			}
			ack_local_timer_irq(irqstat);
			do_local_timer(regs);
#endif
		}
	} while (irq);

	if (in_irq)
		irq_exit();

#if defined(CONFIG_SNSC_DEBUG_IRQ_OVERHEAD_ONCE) && defined(CONFIG_SMP)
	if (!had_external_interrupt)
		per_cpu(ipi_lt_without_irq, smp_processor_id())++;
#endif

	set_irq_regs(old_regs);
}

#else /* !CONFIG_SNSC_IRQ_OVERHEAD_ONCE */

/*
 * asm_do_IRQ is the interface to be used from assembly code.
 */
asmlinkage void __exception_irq_entry
asm_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	handle_IRQ(irq, regs);
}

#endif

void set_irq_flags(unsigned int irq, unsigned int iflags)
{
	unsigned long clr = 0, set = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;

	if (irq >= nr_irqs) {
		printk(KERN_ERR "Trying to set irq flags for IRQ%d\n", irq);
		return;
	}

	if (iflags & IRQF_VALID)
		clr |= IRQ_NOREQUEST;
	if (iflags & IRQF_PROBE)
		clr |= IRQ_NOPROBE;
	if (!(iflags & IRQF_NOAUTOEN))
		clr |= IRQ_NOAUTOEN;
	/* Order is clear bits in "clr" then set bits in "set" */
	irq_modify_status(irq, clr, set & ~clr);
}

void __init init_IRQ(void)
{
	machine_desc->init_irq();
}

#ifdef CONFIG_SPARSE_IRQ
int __init arch_probe_nr_irqs(void)
{
	nr_irqs = machine_desc->nr_irqs ? machine_desc->nr_irqs : NR_IRQS;
	return nr_irqs;
}
#endif

#ifdef CONFIG_HOTPLUG_CPU

static bool migrate_one_irq(struct irq_desc *desc)
{
	struct irq_data *d = irq_desc_get_irq_data(desc);
	const struct cpumask *affinity = d->affinity;
	struct irq_chip *c;
	bool ret = false;

	/*
	 * If this is a per-CPU interrupt, or the affinity does not
	 * include this CPU, then we have nothing to do.
	 */
	if (irqd_is_per_cpu(d) || !cpumask_test_cpu(smp_processor_id(), affinity))
		return false;

	if (cpumask_any_and(affinity, cpu_online_mask) >= nr_cpu_ids) {
		affinity = cpu_online_mask;
		ret = true;
	}

	c = irq_data_get_irq_chip(d);
	if (c->irq_set_affinity)
		c->irq_set_affinity(d, affinity, true);
	else
		pr_debug("IRQ%u: unable to set affinity\n", d->irq);

	return ret;
}

/*
 * The current CPU has been marked offline.  Migrate IRQs off this CPU.
 * If the affinity settings do not allow other CPUs, force them onto any
 * available CPU.
 *
 * Note: we must iterate over all IRQs, whether they have an attached
 * action structure or not, as we need to get chained interrupts too.
 */
void migrate_irqs(void)
{
	unsigned int i;
	struct irq_desc *desc;
	unsigned long flags;

	local_irq_save(flags);

	for_each_irq_desc(i, desc) {
		bool affinity_broken = false;

		if (!desc)
			continue;

		raw_spin_lock(&desc->lock);
		affinity_broken = migrate_one_irq(desc);
		raw_spin_unlock(&desc->lock);

		if (affinity_broken && printk_ratelimit())
			pr_warning("IRQ%u no longer affine to CPU%u\n", i,
				smp_processor_id());
	}

	local_irq_restore(flags);
}
#endif /* CONFIG_HOTPLUG_CPU */
