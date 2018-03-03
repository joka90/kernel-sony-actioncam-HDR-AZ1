/*
 * arch/arm/mach-cxd4115/gic.c
 *
 * MPCore Distributed Interrupt Controller
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
 *
 * This code is based on arch/arm/common/gic.c.
 */
/*
 *  linux/arch/arm/common/gic.c
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Interrupt architecture for the GIC:
 *
 * o There is one Interrupt Distributor, which receives interrupts
 *   from system devices and sends them to the Interrupt Controllers.
 *
 * o There is one CPU Interface per CPU, which sends interrupts sent
 *   by the Distributor, and interrupts generated locally, to the
 *   associated CPU. The base address of the CPU interface is usually
 *   aliased so that the same address points to different chips depending
 *   on the CPU it is accessed from.
 *
 * Note that IRQs 0-31 are special - they are local to each CPU.
 * As such, the enable set/clear, pending set/clear and active bit
 * registers are banked per-cpu for these sources.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <mach/gic.h>

#undef GIC_DEBUG

static DEFINE_RAW_SPINLOCK(irq_controller_lock);

/*
 * Routines to acknowledge, disable and enable interrupts
 *
 * Linux assumes that when we're done with an interrupt we need to
 * unmask it, in the same way we need to unmask an interrupt when
 * we first enable it.
 *
 * The GIC has a separate notion of "end of interrupt" to re-enable
 * an interrupt after handling, in order to support hardware
 * prioritisation.
 *
 * We can make the GIC behave in the way that Linux expects by making
 * our "acknowledge" routine disable the interrupt, then mark it as
 * complete.
 */
static void gic_ack_irq(struct irq_data *d)
{
	uint irq = d->irq;

	writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_CLEAR + GIC_IRQ_OFFSET(irq));
	writel(irq, VA_GIC_CPU + GIC_CPU_EOI);
	mmiowb();
}

static void gic_mask_irq(struct irq_data *d)
{
	uint irq = d->irq;

	writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_CLEAR + GIC_IRQ_OFFSET(irq));
	mmiowb();
}

static void gic_unmask_irq(struct irq_data *d)
{
	uint irq = d->irq;

	writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_SET + GIC_IRQ_OFFSET(irq));
}

#ifdef CONFIG_SMP
static int gic_set_cpu(struct irq_data *d, const struct cpumask *mask_val,
			bool force)
{
	uint irq = d->irq;
	const void __iomem *reg = VA_GIC_DIST + GIC_DIST_TARGET + (irq & ~GIC_TARGET_BITS_MASK);
	unsigned int shift = (irq & GIC_TARGET_BITS_MASK) << 3;
	unsigned int cpu = cpumask_first(mask_val);
	u32 val;

	raw_spin_lock(&irq_controller_lock);
	d->node = cpu;
	val = readl(reg) & ~(0xff << shift);
	val |= 1 << (cpu + shift);
	writel(val, reg);
	raw_spin_unlock(&irq_controller_lock);
	return 0;
}
#endif

#ifdef GIC_DEBUG
static void gic_dump(void)
{
	int i;

	printk(KERN_ERR "gic_dump\n");
	printk(KERN_ERR "Enable reg:\n");
	for (i = 0; i < NR_IRQS/8; i+=4) {
		printk("%08x ", readl(VA_GIC_DIST + GIC_DIST_ENABLE_SET + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Priority reg:\n");
	for (i = 0; i < NR_IRQS; i+=4) {
		printk("%08x ", readl(VA_GIC_DIST + GIC_DIST_PRI + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Target reg:\n");
	for (i = 0; i < NR_IRQS; i+=4) {
		printk("%08x ", readl(VA_GIC_DIST + GIC_DIST_TARGET + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Config reg:\n");
	for (i = 0; i < NR_IRQS/4; i+=4) {
		printk("%08x ", readl(VA_GIC_DIST + GIC_DIST_CONFIG + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "primask=%08x\n", readl(VA_GIC_CPU + GIC_CPU_PRIMASK));
}
#endif /* GIC_DEBUG */

static struct irq_chip gic_chip = {
	.name		= "GIC",
	.irq_ack		= gic_ack_irq,
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= gic_set_cpu,
#endif
};

void __init gic_dist_init(unsigned int gic_nr, const void __iomem *base, unsigned int irq_start, u8 *edge_irqs, int n_edge_irqs)
{
	unsigned int max_irq, i;
	u32 cpumask = 1 << raw_smp_processor_id();
#ifdef CONFIG_MPCORE_GIC_INIT
	u8 *p;
	unsigned int offset, shift, tmp;
#endif

	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

#ifdef CONFIG_MPCORE_GIC_INIT
	writel(0, base + GIC_DIST_CTRL);
#endif

	/*
	 * Find out how many interrupts are supported.
	 */
	max_irq = readl(base + GIC_DIST_CTR) & 0x1f;
	max_irq = (max_irq + 1) * 32;

	/*
	 * The GIC only supports up to 1020 interrupt sources.
	 * Limit this to either the architected maximum, or the
	 * platform maximum.
	 */
	if (max_irq > max(1020, NR_IRQS))
		max_irq = max(1020, NR_IRQS);

#ifdef CONFIG_MPCORE_GIC_INIT
	/*
	 * Set all global interrupts to be level triggered, N:N model
	 */
	offset = (IRQ_GIC_START >> GIC_CONFIG_BITS_SHIFT) << 2;
	for (i = 32; i < max_irq; i += 16, offset += 4)
		writel(0, base + GIC_DIST_CONFIG + offset);

	/*
         * Set edge trigger
         *  SLOW: set one by one. (read-modify-write)
	 */
	for (i = 0, p = edge_irqs; i < n_edge_irqs; i++, p++) {
		offset = (*p >> GIC_CONFIG_BITS_SHIFT) << 2;
		shift = (*p & GIC_CONFIG_BITS_MASK) * GIC_CONFIG_BITS_WIDTH;
		tmp = readl(base + GIC_DIST_CONFIG + offset);
		tmp |= GIC_DIST_CONFIG_EDGE << shift;
		writel(tmp, base + GIC_DIST_CONFIG + offset);
	}

	/*
	 * Set all global interrupts to this CPU only.
	 */
	offset = (IRQ_GIC_START >> GIC_TARGET_BITS_SHIFT) << 2;
	for (i = 32; i < max_irq; i += 4, offset += 4)
		writel(cpumask, base + GIC_DIST_TARGET + offset);

	/*
	 * Set priority on all interrupts.
	 *   INT0..31 are alias.
	 */
	for (i = 0, offset = 0; i < max_irq; i += 4, offset += 4)
		writel(0xa0a0a0a0, base + GIC_DIST_PRI + offset);

	/*
	 * Disable all interrupts.
	 *   INT29..31 are alias.
	 */
	for (i = 0, offset = 0; i < max_irq; i += 32, offset += 4)
		writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + offset);
#endif /* CONFIG_MPCORE_GIC_INIT */
	mmiowb();
#ifdef GIC_DEBUG
	gic_dump();
#endif

	/*
	 * Setup the Linux IRQ subsystem.
	 */
	for (i = irq_start; i < max_irq; i++) {
		irq_set_chip_and_handler(i, &gic_chip, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
#ifdef CONFIG_IRQ_PER_CPU
		if (i < 32) {
			irq_to_desc(i)->status |= IRQ_PER_CPU;
		}
#endif
	}

#ifdef CONFIG_MPCORE_GIC_INIT
	writel(1, base + GIC_DIST_CTRL);
#endif
}

void __cpuinit gic_enable_ppi(unsigned int irq)
{
	unsigned long flags;

	local_irq_save(flags);
	irq_set_status_flags(irq, IRQ_NOPROBE);
	gic_unmask_irq(irq_get_irq_data(irq));
	local_irq_restore(flags);
}

void __cpuinit gic_cpu_init(unsigned int gic_nr, const void __iomem *base)
{
#ifdef CONFIG_SMP
	int i;

	/*
	 * Set priority on INT0..31 for this CPU.
	 */
	for (i = 0; i < 32; i += 4)
		writel(0xa0a0a0a0, VA_GIC_DIST + GIC_DIST_PRI + i);
#endif

	writel(0xf0, base + GIC_CPU_PRIMASK);
	writel(1, base + GIC_CPU_CTRL);
}

#ifdef CONFIG_SMP
void gic_raise_softirq(cpumask_t cpumask, unsigned int irq)
{
	unsigned long map = *cpus_addr(cpumask);

	/*
	 * Ensure that stores to Normal memory are visible to the
	 * other CPUs before issuing the IPI.
	 */
	dsb();

	writel(map << 16 | irq, VA_GIC_DIST + GIC_DIST_SOFTINT);
}
#endif

#ifdef CONFIG_EJ_EXPORT_RAISE_SOFT_INTERRUPT
void raise_irq(unsigned int cpu, unsigned int irq)
{
	dsb();
	writel((1 << (16+cpu)) | irq, VA_GIC_DIST + GIC_DIST_SOFTINT);
}
EXPORT_SYMBOL(raise_irq);
#endif /* CONFIG_EJ_EXPORT_RAISE_SOFT_INTERRUPT */


struct gic_info {
	u32 enable[NR_IRQS/32];
	u32 prio[NR_IRQS/4];
	u32 target[NR_IRQS/4];
	u32 config[NR_IRQS/16];
	/* CPU I/F */
	u32 primask;
	u32 binary;
};
static struct gic_info gic_save;

void gic_suspend(void)
{
	u32 *p;
	int i;

	/* save Enable Regs. */
	p = gic_save.enable;
	for (i = 0; i < NR_IRQS/8; i+=4) {
		*p++ = readl(VA_GIC_DIST + GIC_DIST_ENABLE_SET + i);
	}
#ifdef CONFIG_MPCORE_GIC_INIT
	/* save Priority Regs. */
	p = gic_save.prio;
	for (i = 0; i < NR_IRQS; i+=4) {
		*p++ = readl(VA_GIC_DIST + GIC_DIST_PRI + i);
	}
	/* save Target Regs. */
	p = gic_save.target;
	for (i = 0; i < NR_IRQS; i+=4) {
		*p++ = readl(VA_GIC_DIST + GIC_DIST_TARGET + i);
	}
	/* save Config Regs. */
	p = gic_save.config;
	for (i = 0; i < NR_IRQS/4; i+=4) {
		*p++ = readl(VA_GIC_DIST + GIC_DIST_CONFIG + i);
	}
#endif /* CONFIG_MPCORE_GIC_INIT */
	gic_save.primask = readl(VA_GIC_CPU + GIC_CPU_PRIMASK);
}

void gic_resume(void)
{
	u32 *p;
	int i;

#ifdef CONFIG_MPCORE_GIC_INIT
	/* restore Priority Regs. */
	p = gic_save.prio;
	for (i = 0; i < NR_IRQS; i+=4, p++) {
		writel(*p, VA_GIC_DIST + GIC_DIST_PRI + i);
	}
	/* restore Target Regs. */
	p = gic_save.target;
	for (i = 0; i < NR_IRQS; i+=4, p++) {
		writel(*p, VA_GIC_DIST + GIC_DIST_TARGET + i);
	}
	/* restore Config Regs. */
	p = gic_save.config;
	for (i = 0; i < NR_IRQS/4; i+=4, p++) {
		writel(*p, VA_GIC_DIST + GIC_DIST_CONFIG + i);
	}
#endif /* CONFIG_MPCORE_GIC_INIT */
	/* restore Enable Regs. */
	p = gic_save.enable;
	for (i = 0; i < NR_IRQS/8; i+=4, p++) {
		writel(*p, VA_GIC_DIST + GIC_DIST_ENABLE_SET + i);
	}
#ifdef GIC_DEBUG
	gic_dump();
#endif
	/* enable GIC */
	writel(1, VA_GIC_DIST + GIC_DIST_CTRL);

	writel(gic_save.primask, VA_GIC_CPU + GIC_CPU_PRIMASK);
	writel(1, VA_GIC_CPU + GIC_CPU_CTRL);
}

#ifdef CONFIG_AUTO_IRQ_AFFINITY
int irq_select_affinity(unsigned int irq)
{
	const void __iomem *reg = VA_GIC_DIST + GIC_DIST_TARGET + (irq & ~GIC_TARGET_BITS_MASK);
	unsigned int shift = (irq & GIC_TARGET_BITS_MASK) << 3;
	u32 map, cpu;

	if (irq < 32)
		return 1;
	raw_spin_lock(&irq_controller_lock);
	map = (readl(reg) >> shift) & 0xff;
	cpu = ffs(map);
	if (cpu > 0) {
		cpu--;
		*(irq_to_desc(irq)->irq_data.affinity) = *cpumask_of(cpu);
	}
	raw_spin_unlock(&irq_controller_lock);
	return 0;
}
#endif /* CONFIG_AUTO_IRQ_AFFINITY */

#ifdef CONFIG_SNSC_PREEMPT_IRQS
unsigned int gic_get_irq_priority(unsigned int irq)
{
	unsigned int prio;

	prio = readl(VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	return ((prio >> GIC_PRIO_DATA_SHIFT(irq)) & 0xF0);
}

unsigned int gic_set_irq_priority(unsigned int irq, unsigned int prio)
{
	unsigned int prio_orig, temp;

	spin_lock(&irq_controller_lock);
	prio_orig = readl(VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	temp = prio_orig & ~(0xff << GIC_PRIO_DATA_SHIFT(irq));
	temp |= ((prio & 0xf0) << GIC_PRIO_DATA_SHIFT(irq));

	writel(temp, VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	spin_unlock(&irq_controller_lock);

	return temp;
}
EXPORT_SYMBOL(gic_set_irq_priority);

unsigned int gic_get_priority_mask(void)
{
	return readl(VA_GIC_CPU + GIC_CPU_PRIMASK);
}

void gic_set_priority_mask(unsigned int prio_mask)
{
	writel((prio_mask & 0xf0), VA_GIC_CPU + GIC_CPU_PRIMASK);
}
#endif /* CONFIG_SNSC_PREEMPT_IRQS */
