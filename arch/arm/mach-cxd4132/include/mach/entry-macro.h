/*
 * include/asm-arm/arch-cxd4115/entry-macro.h
 *
 * Low-level IRQ helpers
 *
 * C code version of include/asm-arm/arch-cxd4115/entry-macro.S
 * Any changes in this files must also be made in
 * include/asm-arm/arch-cxd4115/entry-macro.S
 *
 * Copyright 2008,2009,2010 Sony Corporation
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ARCH_CXD4115_ENTRY_MACRO_H
#define __ARCH_CXD4115_ENTRY_MACRO_H

#include <mach/hardware.h>
#include <mach/gic.h>
#include <asm/io.h>

#define IRQ_MASK 0x3ff

#define IRQ_IS_DONE(irq)                ((irq) == 1023)
#define IRQ_IS_EXTERNAL_INTERRUPT(irq)  ((irq) > 29)
#define IRQ_IS_IPI(irq)                 ((irq) < 16)
#define IRQ_IS_LOCALTIMER(irq)          ((irq) == 29)

#define get_irq_preamble() do {} while (0)

static inline int get_irq(void)
{
	return readl(VA_GIC_CPU + GIC_CPU_INTACK) & IRQ_MASK;
}

static inline void ack_local_timer_irq(unsigned int irq)
{
	writel(irq, VA_GIC_CPU + GIC_CPU_EOI);
}

static inline void ack_IPI_irq(unsigned int irq)
{
	writel(irq, VA_GIC_CPU + GIC_CPU_EOI);
}

#endif
