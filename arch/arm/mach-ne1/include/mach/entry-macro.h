/* 2012-07-12: File added and changed by Sony Corporation */
/*
 * arch/arm/mach-ne1/include/mach/entry-macro.h
 *
 * Low-level IRQ helpers
 *
 * C code version of arch/arm/mach-ne1/include/mach/entry-macro.S,
 * Any changes in this files must also be made in
 * arch/arm/mach-ne1/include/mach/entry-macro.S
 *
 * Copyright 2009 Sony Corporation
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_ENTRY_MACRO_H
#define __ASM_ARCH_ENTRY_MACRO_H

#include <asm/hardware/gic.h>
#include <mach/hardware.h>

#define IRQ_MASK 0x3ff

#define IRQ_IS_DONE(irq)                ((irq) == 1023)
#define IRQ_IS_EXTERNAL_INTERRUPT(irq)  ((irq) > 29)
#define IRQ_IS_IPI(irq)                 ((irq) < 16)
#define IRQ_IS_LOCALTIMER(irq)          ((irq) == 29)

#define get_irq_preamble() do {} while (0)

static inline unsigned int get_irqstat(void)
{
	const void __iomem __force *gic_base = __io_address(NE1_GIC_CPU_BASE);

	return *(unsigned long *)(gic_base + GIC_CPU_INTACK);
}

static inline void ack_local_timer_irq(unsigned int irqstat)
{
	const void __iomem __force *gic_base = __io_address(NE1_GIC_CPU_BASE);

	*(unsigned long *)(gic_base + GIC_CPU_EOI) = irqstat;
}

static inline void ack_IPI_irq(unsigned int irqstat)
{
	const void __iomem __force *gic_base = __io_address(NE1_GIC_CPU_BASE);

	*(unsigned long *)(gic_base + GIC_CPU_EOI) = irqstat;
}

#endif
