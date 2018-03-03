/*
 * arch/arm/mach-cxd90014/gic.c
 *
 * GIC extension
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on arch/arm/mach-integrator/core.c.
 */
/*
 *  arch/arm/mach-emxx/gic.c
 *
 *  Copyright (C) 2010 Renesas Electronics Corporation
 *
 *  This file is based on the arch/arm/common/gic.c
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
#include <mach/gic_export.h>
#include <mach/cxd90014_board.h>

#undef GIC_DEBUG

void __init gic_cxd90014_init(void)
{
	writel_relaxed(0, VA_GIC_CPU+GIC_CPU_CTRL);
	writel_relaxed(0, VA_GIC_DIST+GIC_DIST_CTRL);

	cxd90014_irq_type_init();

	writel_relaxed(1, VA_GIC_DIST+GIC_DIST_CTRL);
	writel_relaxed(1, VA_GIC_CPU+GIC_CPU_CTRL);
}

void gic_dump(void)
{
#ifdef GIC_DEBUG
	int i;

	printk(KERN_ERR "gic_dump\n");
	printk(KERN_ERR "Enable reg:\n");
	for (i = 0; i < NR_IRQS/8; i+=4) {
		printk("%08x ", readl_relaxed(VA_GIC_DIST + GIC_DIST_ENABLE_SET + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Priority reg:\n");
	for (i = 0; i < NR_IRQS; i+=4) {
		printk("%08x ", readl_relaxed(VA_GIC_DIST + GIC_DIST_PRI + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Target reg:\n");
	for (i = 0; i < NR_IRQS; i+=4) {
		printk("%08x ", readl_relaxed(VA_GIC_DIST + GIC_DIST_TARGET + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "Config reg:\n");
	for (i = 0; i < NR_IRQS/4; i+=4) {
		printk("%08x ", readl_relaxed(VA_GIC_DIST + GIC_DIST_CONFIG + i));
		if ((i & 0x1f) == 0x1c)
			printk("\n");
	}
	printk(KERN_ERR "primask=%08x\n", readl_relaxed(VA_GIC_CPU + GIC_CPU_PRIMASK));
#endif /* GIC_DEBUG */
}

void __init gic_config_edge(unsigned int irq)
{
	u32 offset, shift, tmp;

	offset = (irq >> GIC_CONFIG_BITS_SHIFT) << 2;
	shift = (irq & GIC_CONFIG_BITS_MASK) * GIC_CONFIG_BITS_WIDTH;
	tmp = readl_relaxed(VA_GIC_DIST+GIC_DIST_CONFIG + offset);
	tmp |= GIC_DIST_CONFIG_EDGE << shift;
	writel_relaxed(tmp, VA_GIC_DIST+GIC_DIST_CONFIG + offset);
}

#ifdef CONFIG_AUTO_IRQ_AFFINITY
int irq_select_affinity(unsigned int irq)
{
	struct irq_desc *desc;
	u32 map, cpu;
	extern void irq_set_thread_affinity(struct irq_desc *desc);

	if (!irq_can_set_affinity(irq))
		return 0;
	gic_lock();
	desc = irq_to_desc(irq);
	map = readb_relaxed(GIC_IRQ_TARGET(irq));
	cpu = ffs(map);
	if (cpu > 0) {
		cpu--;
		cpumask_copy(desc->irq_data.affinity, cpumask_of(cpu));
		irq_set_thread_affinity(desc);
	}
	gic_unlock();
	return 0;
}
#endif /* CONFIG_AUTO_IRQ_AFFINITY */

unsigned int gic_get_irq_priority(unsigned int irq)
{
	unsigned int prio;

	prio = readl_relaxed(VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	return ((prio >> GIC_PRIO_DATA_SHIFT(irq)) & 0xF0);
}

unsigned int gic_set_irq_priority(unsigned int irq, unsigned int prio)
{
	unsigned int prio_orig, temp;

	gic_lock();
	prio_orig = readl_relaxed(VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	temp = prio_orig & ~(0xff << GIC_PRIO_DATA_SHIFT(irq));
	temp |= ((prio & 0xf0) << GIC_PRIO_DATA_SHIFT(irq));

	writel_relaxed(temp, VA_GIC_DIST + GIC_DIST_PRI + GIC_PRIO_OFFSET(irq));
	gic_unlock();

	return temp;
}
EXPORT_SYMBOL(gic_set_irq_priority);

unsigned int gic_get_priority_mask(void)
{
	return readl_relaxed(VA_GIC_CPU + GIC_CPU_PRIMASK);
}

void gic_set_priority_mask(unsigned int prio_mask)
{
	writel_relaxed((prio_mask & 0xf0), VA_GIC_CPU + GIC_CPU_PRIMASK);
}

#ifdef CONFIG_EJ_EXPORT_RAISE_SOFT_INTERRUPT
void raise_irq(unsigned int cpu, unsigned int irq)
{
	dsb();
	writel_relaxed((1 << (16+cpu)) | irq, VA_GIC_DIST + GIC_DIST_SOFTINT);
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

void gic_cxd90014_suspend(void)
{
        u32 *p;
        int i;

        /* save Enable Regs. */
        p = gic_save.enable;
        for (i = 0; i < NR_IRQS/8; i+=4) {
                *p++ = readl_relaxed(VA_GIC_DIST + GIC_DIST_ENABLE_SET + i);
        }
#ifdef CONFIG_MPCORE_GIC_INIT
        /* save Priority Regs. */
        p = gic_save.prio;
        for (i = 0; i < NR_IRQS; i+=4) {
                *p++ = readl_relaxed(VA_GIC_DIST + GIC_DIST_PRI + i);
        }
        /* save Target Regs. */
        p = gic_save.target;
        for (i = 0; i < NR_IRQS; i+=4) {
                *p++ = readl_relaxed(VA_GIC_DIST + GIC_DIST_TARGET + i);
        }
        /* save Config Regs. */
        p = gic_save.config;
        for (i = 0; i < NR_IRQS/4; i+=4) {
                *p++ = readl_relaxed(VA_GIC_DIST + GIC_DIST_CONFIG + i);
        }
#endif /* CONFIG_MPCORE_GIC_INIT */
        gic_save.primask = readl_relaxed(VA_GIC_CPU + GIC_CPU_PRIMASK);
}

void gic_cxd90014_resume(void)
{
        u32 *p;
        int i;

#ifdef CONFIG_MPCORE_GIC_INIT
        /* restore Priority Regs. */
        p = gic_save.prio;
        for (i = 0; i < NR_IRQS; i+=4, p++) {
                writel_relaxed(*p, VA_GIC_DIST + GIC_DIST_PRI + i);
        }
        /* restore Target Regs. */
        p = gic_save.target;
        for (i = 0; i < NR_IRQS; i+=4, p++) {
                writel_relaxed(*p, VA_GIC_DIST + GIC_DIST_TARGET + i);
        }
        /* restore Config Regs. */
        p = gic_save.config;
        for (i = 0; i < NR_IRQS/4; i+=4, p++) {
                writel_relaxed(*p, VA_GIC_DIST + GIC_DIST_CONFIG + i);
        }
#endif /* CONFIG_MPCORE_GIC_INIT */
        /* restore Enable Regs. */
        p = gic_save.enable;
        for (i = 0; i < NR_IRQS/8; i+=4, p++) {
                writel_relaxed(*p, VA_GIC_DIST + GIC_DIST_ENABLE_SET + i);
        }
#ifdef GIC_DEBUG
        gic_dump();
#endif
        /* enable GIC */
        writel_relaxed(1, VA_GIC_DIST + GIC_DIST_CTRL);

        writel_relaxed(gic_save.primask, VA_GIC_CPU + GIC_CPU_PRIMASK);
        writel_relaxed(1, VA_GIC_CPU + GIC_CPU_CTRL);
}

