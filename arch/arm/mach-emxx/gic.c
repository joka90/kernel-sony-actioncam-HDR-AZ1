/* 2011-08-31: File added and changed by Sony Corporation */
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
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>
#include <mach/smp.h>


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
void gic_ack2_irq(struct irq_data *d)
{
	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(gic_irq(d), gic_cpu_base(d) + GIC_CPU_EOI);
	raw_spin_unlock(&irq_controller_lock);
}

struct irq_chip gic_chip_edge = {
	.name		= "GIC edge",
	.irq_ack		= gic_ack2_irq,
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= gic_set_affinity,
#endif
};

void __init gic_emxx_init(unsigned int gic_nr,
			  unsigned int irq_start)
{
	unsigned int max_irq, i;
	struct gic_chip_data *gic;
	void __iomem *base;


	gic = &gic_data[gic_nr];
	base = gic->dist_base;

	writel_relaxed(0, base + GIC_DIST_CTRL);
	writel_relaxed(0, gic->cpu_base + GIC_CPU_CTRL);
	/*
	 * Find out how many interrupts are supported.
	 */

	max_irq = (INT_LAST + 1);

	if (max_irq > max(1020, NR_IRQS))
		max_irq = max(1020, NR_IRQS);


	/* Set edge triggered */
	writel_relaxed(0x00FF0000, base + GIC_DIST_CONFIG + 0x08);	/* UARTs */
	writel_relaxed(0x0000000F, base + GIC_DIST_CONFIG + 0x10);	/* IICs */
	writel_relaxed(0xFFFFFF00, base + GIC_DIST_CONFIG + 0x14);	/* TIMERs */
	writel_relaxed(0x0000003F, base + GIC_DIST_CONFIG + 0x18);
	writel_relaxed(0x00000003, base + GIC_DIST_CONFIG + 0x28);

	/*
	 * Setup the Linux IRQ subsystem.
	 */
	for (i = irq_start; i < gic->irq_offset + max_irq; i++) {
		if ((i >= INT_TIMER0 && i <= INT_WDT4) ||
				(i == INT_IIC0) || (i == INT_IIC1) ||
				(i >= INT_UART0 && i <= INT_UART3)) {
			irq_set_chip_and_handler(i, &gic_chip_edge, handle_edge_irq);
		} else {
		  	irq_set_chip_and_handler(i, &gic_chip, handle_level_irq);
		}
		irq_set_chip_data(i, gic);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);

		if (i < 32)
			irq_desc[i].status_use_accessors |= IRQ_PER_CPU;
	}

	/*
	 * EMXX disable all SGI and PPI interrupts
	 */

	for (i = 0; i < 32; i += 32)
		writel_relaxed(0xffffffff, gic->dist_base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);



	writel_relaxed(1, base + GIC_DIST_CTRL);
	writel_relaxed(1, gic->cpu_base + GIC_CPU_CTRL);

}
