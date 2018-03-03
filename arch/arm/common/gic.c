/* 2013-08-20: File changed by Sony Corporation */
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
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>

DEFINE_RAW_SPINLOCK(irq_controller_lock);

/* Address of GIC 0 CPU interface */
void __iomem *gic_cpu_base_addr __read_mostly;

/*
 * Supported arch specific GIC irq extension.
 * Default make them NULL.
 */
struct irq_chip gic_arch_extn = {
	.irq_eoi	= NULL,
	.irq_mask	= NULL,
	.irq_unmask	= NULL,
	.irq_retrigger	= NULL,
	.irq_set_type	= NULL,
	.irq_set_wake	= NULL,
};

#ifndef MAX_GIC_NR
#define MAX_GIC_NR	1
#endif

struct gic_chip_data gic_data[MAX_GIC_NR] __read_mostly;

inline void __iomem *gic_dist_base(struct irq_data *d)
{
	struct gic_chip_data *gic_data = irq_data_get_irq_chip_data(d);
	return gic_data->dist_base;
}

inline void __iomem *gic_cpu_base(struct irq_data *d)
{
	struct gic_chip_data *gic_data = irq_data_get_irq_chip_data(d);
	return gic_data->cpu_base;
}

inline unsigned int gic_irq(struct irq_data *d)
{
	struct gic_chip_data *gic_data = irq_data_get_irq_chip_data(d);
	return d->irq - gic_data->irq_offset;
}

/*
 * Routines to acknowledge, disable and enable interrupts
 */
void gic_ack_irq(struct irq_data *d)
{
	u32 mask = 1 << (d->irq % 32);

#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_lock(&irq_controller_lock);
#endif
	writel(mask, gic_dist_base(d) +
		GIC_DIST_ENABLE_CLEAR + (gic_irq(d) / 32) * 4);
	writel_relaxed(gic_irq(d), gic_cpu_base(d) + GIC_CPU_EOI);
#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_unlock(&irq_controller_lock);
#endif
}

void gic_mask_irq(struct irq_data *d)
{
	u32 mask = 1 << (d->irq % 32);

#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_lock(&irq_controller_lock);
#endif
	writel_relaxed(mask, gic_dist_base(d) + GIC_DIST_ENABLE_CLEAR + (gic_irq(d) / 32) * 4);
	if (gic_arch_extn.irq_mask)
		gic_arch_extn.irq_mask(d);
#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_unlock(&irq_controller_lock);
#endif
}

void gic_unmask_irq(struct irq_data *d)
{
	u32 mask = 1 << (d->irq % 32);

#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_lock(&irq_controller_lock);
#endif
	if (gic_arch_extn.irq_unmask)
		gic_arch_extn.irq_unmask(d);
	writel_relaxed(mask, gic_dist_base(d) + GIC_DIST_ENABLE_SET + (gic_irq(d) / 32) * 4);
#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_unlock(&irq_controller_lock);
#endif
}

void gic_eoi_irq(struct irq_data *d)
{
	if (gic_arch_extn.irq_eoi) {
		raw_spin_lock(&irq_controller_lock);
		gic_arch_extn.irq_eoi(d);
		raw_spin_unlock(&irq_controller_lock);
	}

	writel_relaxed(gic_irq(d), gic_cpu_base(d) + GIC_CPU_EOI);
}

int gic_set_type(struct irq_data *d, unsigned int type)
{
	void __iomem *base = gic_dist_base(d);
	unsigned int gicirq = gic_irq(d);
	u32 enablemask = 1 << (gicirq % 32);
	u32 enableoff = (gicirq / 32) * 4;
	u32 confmask = 0x2 << ((gicirq % 16) * 2);
	u32 confoff = (gicirq / 16) * 4;
	bool enabled = false;
	u32 val;

	/* Interrupt configuration for SGIs can't be changed */
	if (gicirq < 16)
		return -EINVAL;

	if (type != IRQ_TYPE_LEVEL_HIGH && type != IRQ_TYPE_EDGE_RISING)
		return -EINVAL;

	raw_spin_lock(&irq_controller_lock);

	if (gic_arch_extn.irq_set_type)
		gic_arch_extn.irq_set_type(d, type);

	val = readl_relaxed(base + GIC_DIST_CONFIG + confoff);
	if (type == IRQ_TYPE_LEVEL_HIGH)
		val &= ~confmask;
	else if (type == IRQ_TYPE_EDGE_RISING)
		val |= confmask;

	/*
	 * As recommended by the spec, disable the interrupt before changing
	 * the configuration
	 */
	if (readl_relaxed(base + GIC_DIST_ENABLE_SET + enableoff) & enablemask) {
		writel_relaxed(enablemask, base + GIC_DIST_ENABLE_CLEAR + enableoff);
		enabled = true;
	}

	writel_relaxed(val, base + GIC_DIST_CONFIG + confoff);

	if (enabled)
		writel_relaxed(enablemask, base + GIC_DIST_ENABLE_SET + enableoff);

	raw_spin_unlock(&irq_controller_lock);

	return 0;
}

int gic_retrigger(struct irq_data *d)
{
	if (gic_arch_extn.irq_retrigger)
		return gic_arch_extn.irq_retrigger(d);

	return -ENXIO;
}

#ifdef CONFIG_SMP
int gic_set_affinity(struct irq_data *d, const struct cpumask *mask_val,
			    bool force)
{
	void __iomem *reg = gic_dist_base(d) + GIC_DIST_TARGET + (gic_irq(d) & ~3);
	unsigned int shift = (d->irq % 4) * 8;
	unsigned int cpu = cpumask_any_and(mask_val, cpu_online_mask);
	u32 val, mask, bit;

	if (cpu >= 8 || cpu >= nr_cpu_ids)
		return -EINVAL;

	mask = 0xff << shift;
	bit = 1 << (cpu + shift);

	raw_spin_lock(&irq_controller_lock);
	val = readl_relaxed(reg) & ~mask;
	writel_relaxed(val | bit, reg);
	raw_spin_unlock(&irq_controller_lock);

	return IRQ_SET_MASK_OK;
}
#endif

#ifdef CONFIG_PM
int gic_set_wake(struct irq_data *d, unsigned int on)
{
	int ret = -ENXIO;

	if (gic_arch_extn.irq_set_wake)
		ret = gic_arch_extn.irq_set_wake(d, on);

	return ret;
}

#else
#define gic_set_wake	NULL
#endif

static void gic_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct gic_chip_data *chip_data = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned int cascade_irq, gic_irq;
	unsigned long status;

	chained_irq_enter(chip, desc);

#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_lock(&irq_controller_lock);
#endif
	status = readl_relaxed(chip_data->cpu_base + GIC_CPU_INTACK);
#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_unlock(&irq_controller_lock);
#endif
	gic_irq = (status & 0x3ff);
	if (gic_irq == 1023)
		goto out;

	cascade_irq = gic_irq + chip_data->irq_offset;
	if (unlikely(gic_irq < 32 || gic_irq > 1020 || cascade_irq >= NR_IRQS))
		do_bad_IRQ(cascade_irq, desc);
	else
		generic_handle_irq(cascade_irq);

 out:
	chained_irq_exit(chip, desc);
}

#if defined (CONFIG_SNSC_SSBOOT) && !defined (CONFIG_WARM_BOOT_IMAGE)

#define MAX_GIC_NR_IRQ			1020

static struct gic_ctx {
	u_int32_t dist_config[DIV_ROUND_UP(MAX_GIC_NR_IRQ - 16, 16)];
	u_int32_t dist_target[DIV_ROUND_UP(MAX_GIC_NR_IRQ - 32, 4)];
	u_int32_t dist_pri[DIV_ROUND_UP(MAX_GIC_NR_IRQ, 4)];
	u_int32_t dist_enable[DIV_ROUND_UP(MAX_GIC_NR_IRQ, 32)];
	u_int32_t dist_ctrl;
	u_int32_t cpu_primask;
	u_int32_t cpu_binpoint;
	u_int32_t cpu_ctrl;
} gic_ctx[MAX_GIC_NR];

int
gic_suspend(void)
{
	struct gic_ctx *gic;
	void __iomem *dist_base;
	void __iomem *cpu_base;
	u_int32_t *p;
	unsigned int max_irq;
	int gic_nr, i;

	for (gic_nr = 0; gic_nr < MAX_GIC_NR; gic_nr++) {

		gic = &gic_ctx[gic_nr];

		dist_base = gic_data[gic_nr].dist_base;
		cpu_base  = gic_data[gic_nr].cpu_base;

		/*
		 * Find out how many interrupts are supported on this GIC.
		 */
		max_irq = readl_relaxed(dist_base + GIC_DIST_CTR) & 0x1f;
		max_irq = (max_irq + 1) * 32;

		/*
		 * The GIC only supports up to 1020 interrupt sources.
		 * Limit this to either the architected maximum, or the
		 * platform maximum.
		 */
		if (max_irq > max(MAX_GIC_NR_IRQ, NR_IRQS))
			max_irq = max(MAX_GIC_NR_IRQ, NR_IRQS);

		p = gic->dist_config;
		for (i = 16; i < max_irq; i += 16) {
			*p++ = readl_relaxed(dist_base + GIC_DIST_CONFIG + i * 4 / 16);
		}

		p = gic->dist_target;
		for (i = 32; i < max_irq; i += 4) {
			*p++ = readl_relaxed(dist_base + GIC_DIST_TARGET + i * 4 / 4);
		}

		p = gic->dist_pri;
		for (i = 0; i < max_irq; i += 4) {
			*p++ = readl_relaxed(dist_base + GIC_DIST_PRI + i * 4 / 4);
		}

		p = gic->dist_enable;
		for (i = 0; i < max_irq; i += 32) {
			*p++ = readl_relaxed(dist_base +
				     GIC_DIST_ENABLE_SET + i * 4 / 32);
		}

		gic->dist_ctrl    = readl_relaxed(dist_base + GIC_DIST_CTRL);
		gic->cpu_primask  = readl_relaxed(cpu_base + GIC_CPU_PRIMASK);
		gic->cpu_binpoint = readl_relaxed(cpu_base + GIC_CPU_BINPOINT);
		gic->cpu_ctrl     = readl_relaxed(cpu_base + GIC_CPU_CTRL);
	}

	return 0;
}

int
gic_resume(void)
{
	struct gic_ctx *gic;
	void __iomem *dist_base;
	void __iomem *cpu_base;
	u_int32_t *p;
	unsigned int max_irq;
	int gic_nr, i;

	for (gic_nr = 0; gic_nr < MAX_GIC_NR; gic_nr++) {

		gic = &gic_ctx[gic_nr];

		dist_base = gic_data[gic_nr].dist_base;
		cpu_base  = gic_data[gic_nr].cpu_base;

		/*
		 * Find out how many interrupts are supported on this GIC.
		 */
		max_irq = readl_relaxed(dist_base + GIC_DIST_CTR) & 0x1f;
		max_irq = (max_irq + 1) * 32;

		/*
		 * The GIC only supports up to 1020 interrupt sources.
		 * Limit this to either the architected maximum, or the
		 * platform maximum.
		 */
		if (max_irq > max(MAX_GIC_NR_IRQ, NR_IRQS))
			max_irq = max(MAX_GIC_NR_IRQ, NR_IRQS);

		for (i = 0; i < max_irq; i += 32) {
			/* disable interrupts at first */
			writel_relaxed(0xffffffff,
			       dist_base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);
		}

		p = gic->dist_config;
		for (i = 16; i < max_irq; i += 16) {
			writel_relaxed(*p++, dist_base + GIC_DIST_CONFIG + i * 4 / 16);
		}

		p = gic->dist_target;
		for (i = 32; i < max_irq; i += 4) {
			writel_relaxed(*p++, dist_base + GIC_DIST_TARGET + i * 4 / 4);
		}

		p = gic->dist_pri;
		for (i = 0; i < max_irq; i += 4) {
			writel_relaxed(*p++, dist_base + GIC_DIST_PRI + i * 4 / 4);
		}

		p = gic->dist_enable;
		for (i = 0; i < max_irq; i += 32) {
			/* try to clear pending interrupt */
			writel_relaxed(0xffffffff,
			       dist_base + GIC_DIST_PENDING_CLEAR + i * 4 /32);
			writel_relaxed(*p++,
			       dist_base + GIC_DIST_ENABLE_SET + i * 4 / 32);
		}

		writel_relaxed(gic->dist_ctrl,    dist_base + GIC_DIST_CTRL);
		writel_relaxed(gic->cpu_primask,  cpu_base + GIC_CPU_PRIMASK);
		writel_relaxed(gic->cpu_binpoint, cpu_base + GIC_CPU_BINPOINT);
		writel_relaxed(gic->cpu_ctrl,     cpu_base + GIC_CPU_CTRL);
	}

	return 0;
}
#endif /* CONFIG_SNSC_SSBOOT */

struct irq_chip gic_chip = {
	.name			= "GIC",
	.irq_ack		= gic_ack_irq,
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
	.irq_eoi		= gic_eoi_irq,
	.irq_set_type		= gic_set_type,
	.irq_retrigger		= gic_retrigger,
#ifdef CONFIG_SMP
	.irq_set_affinity	= gic_set_affinity,
#endif
	.irq_set_wake		= gic_set_wake,
};

void __init gic_cascade_irq(unsigned int gic_nr, unsigned int irq)
{
	if (gic_nr >= MAX_GIC_NR)
		BUG();
	if (irq_set_handler_data(irq, &gic_data[gic_nr]) != 0)
		BUG();
	irq_set_chained_handler(irq, gic_handle_cascade_irq);
}

void __init gic_dist_init(struct gic_chip_data *gic,
	unsigned int irq_start)
{
	unsigned int gic_irqs, irq_limit, i;
	void __iomem *base = gic->dist_base;
	u32 cpumask = 1 << smp_processor_id();

	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

	writel_relaxed(0, base + GIC_DIST_CTRL);

	/*
	 * Find out how many interrupts are supported.
	 * The GIC only supports up to 1020 interrupt sources.
	 */
	gic_irqs = readl_relaxed(base + GIC_DIST_CTR) & 0x1f;
	gic_irqs = (gic_irqs + 1) * 32;
	if (gic_irqs > 1020)
		gic_irqs = 1020;

#ifdef CONFIG_MPCORE_GIC_INIT
	/*
	 * Set all global interrupts to be level triggered, active high.
	 */
	for (i = 32; i < gic_irqs; i += 16)
		writel(0, base + GIC_DIST_CONFIG + i * 4 / 16);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel_relaxed(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

	/*
	 * Set priority on all global interrupts.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel_relaxed(0xa0a0a0a0, base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		writel_relaxed(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);
#endif /* CONFIG_MPCORE_GIC_INIT */

	/*
	 * Limit number of interrupts registered to the platform maximum
	 */
	irq_limit = gic->irq_offset + gic_irqs;
	if (WARN_ON(irq_limit > NR_IRQS))
		irq_limit = NR_IRQS;

	/*
	 * Setup the Linux IRQ subsystem.
	 */
	for (i = irq_start; i < irq_limit; i++) {
		irq_set_chip_and_handler(i, &gic_chip, handle_fasteoi_irq);
		irq_set_chip_data(i, gic);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
#ifdef CONFIG_IRQ_PER_CPU
		if (i < 32)
			irq_set_status_flags(i, IRQ_PER_CPU);
#endif
	}

	writel_relaxed(1, base + GIC_DIST_CTRL);
}

void __cpuinit gic_cpu_init(struct gic_chip_data *gic)
{
	void __iomem *dist_base = gic->dist_base;
	void __iomem *base = gic->cpu_base;
	int i;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel_relaxed(0xffff0000, dist_base + GIC_DIST_ENABLE_CLEAR);
	writel_relaxed(0x0000ffff, dist_base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel_relaxed(0xa0a0a0a0, dist_base + GIC_DIST_PRI + i * 4 / 4);

	writel_relaxed(0xf0, base + GIC_CPU_PRIMASK);
	writel_relaxed(1, base + GIC_CPU_CTRL);
}

void __init gic_init(unsigned int gic_nr, unsigned int irq_start,
	void __iomem *dist_base, void __iomem *cpu_base)
{
	struct gic_chip_data *gic;

	BUG_ON(gic_nr >= MAX_GIC_NR);

	gic = &gic_data[gic_nr];
	gic->dist_base = dist_base;
	gic->cpu_base = cpu_base;
	gic->irq_offset = (irq_start - 1) & ~31;

	if (gic_nr == 0)
		gic_cpu_base_addr = cpu_base;

	gic_dist_init(gic, irq_start);
	gic_cpu_init(gic);
}

void __cpuinit gic_secondary_init(unsigned int gic_nr)
{
	BUG_ON(gic_nr >= MAX_GIC_NR);

	gic_cpu_init(&gic_data[gic_nr]);
}

void __cpuinit gic_enable_ppi(unsigned int irq)
{
	unsigned long flags;

	local_irq_save(flags);
	irq_set_status_flags(irq, IRQ_NOPROBE);
	gic_unmask_irq(irq_get_irq_data(irq));
	local_irq_restore(flags);
}

#ifdef CONFIG_SMP
void gic_raise_softirq(const struct cpumask *mask, unsigned int irq)
{
	unsigned long map = *cpus_addr(*mask);

	/*
	 * Ensure that stores to Normal memory are visible to the
	 * other CPUs before issuing the IPI.
	 */
	dsb();

	/* this always happens on GIC0 */
	writel_relaxed(map << 16 | irq, gic_data[0].dist_base + GIC_DIST_SOFTINT);
}
#endif

void gic_dist_clear_pending(unsigned int irq)
{
	u32 mask = 1 << (irq % 32);

#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_lock(&irq_controller_lock);
#endif
	writel(mask, gic_dist_base(irq_get_irq_data(irq)) + GIC_DIST_PENDING_CLEAR + (gic_irq(irq_get_irq_data(irq)) / 32) * 4);
#ifndef CONFIG_SNSC_REMOVE_GIC_ACK_LOCK
	raw_spin_unlock(&irq_controller_lock);
#endif
}

void gic_lock(void)
{
	raw_spin_lock(&irq_controller_lock);
}

void gic_unlock(void)
{
	raw_spin_unlock(&irq_controller_lock);
}
