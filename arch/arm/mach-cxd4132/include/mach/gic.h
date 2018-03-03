/*
 * include/asm-arm/arch-cxd4115/gic.h
 *
 * MPCore Distributed Interrupt Controller definitions
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/hardware/gic.h.
 */
/*
 *  linux/include/asm-arm/hardware/gic.h
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ARCH_CXD4115_GIC_H
#define __ARCH_CXD4115_GIC_H

#include <linux/compiler.h>

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
# define GIC_DIST_CONFIG_EDGE		  0x2
# define GIC_DIST_CONFIG_MODEL1N	  0x1
#define GIC_DIST_LINELEVEL		0xd00
#define GIC_DIST_SOFTINT		0xf00

/* Enable, Pending, Active, LineLevel reg. */
#define GIC_IRQ_BITS_SHIFT	5 /* packed 2^5 in u32 */
#define GIC_IRQ_BITS_MASK	((1 << GIC_IRQ_BITS_SHIFT) - 1)
#define GIC_IRQ_BITMASK(irq)	(1 << ((irq) & GIC_IRQ_BITS_MASK))
#define GIC_IRQ_OFFSET(irq)	(((irq) >> GIC_IRQ_BITS_SHIFT) << 2)

#define GIC_IRQ_PENDING(irq)	(readl(VA_GIC_DIST + GIC_DIST_PENDING_SET + GIC_IRQ_OFFSET(irq)) & GIC_IRQ_BITMASK(irq))

#define GIC_IRQ_RAWSTATUS(irq)	(readl(VA_GIC_DIST + GIC_DIST_LINELEVEL + GIC_IRQ_OFFSET(irq)) & GIC_IRQ_BITMASK(irq))

/* Priority reg. */
#define GIC_PRIO_BITS_SHIFT	2 /* packed 2^2 in u32 */
#define GIC_PRIO_OFFSET(irq)    (((irq) >> GIC_PRIO_BITS_SHIFT) << 2)
#define GIC_PRIO_BITS_MASK	((1 << GIC_PRIO_BITS_SHIFT) - 1)
#define GIC_PRIO_DATA_SHIFT(irq)	((irq & GIC_PRIO_BITS_MASK) << GIC_PRIO_BITS_MASK)

/* Configuration reg. */
#define GIC_CONFIG_BITS_WIDTH	2 /* 2bit field */
#define GIC_CONFIG_BITS_SHIFT	4 /* packed 2^4 in u32 */
#define GIC_CONFIG_BITS_MASK	((1 << GIC_CONFIG_BITS_SHIFT) - 1)

/* CPU targets reg. */
#define GIC_TARGET_BITS_SHIFT	2 /* packed 2^2 in u32 */
#define GIC_TARGET_BITS_MASK	((1 << GIC_TARGET_BITS_SHIFT) - 1)

#define gic_clearirq(irq)	writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_PENDING_CLEAR + GIC_IRQ_OFFSET(irq))
#define gic_pollirq(irq)	!!GIC_IRQ_RAWSTATUS(irq)
#define gic_maskirq(irq, enable) \
({ \
	if (enable) \
		writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_SET + GIC_IRQ_OFFSET(irq)); \
	else \
		writel(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_CLEAR + GIC_IRQ_OFFSET(irq)); \
})

#ifndef __ASSEMBLY__
void gic_dist_init(unsigned int gic_nr, const void __iomem *base, unsigned int irq_start, u8 *edge_irqs, int n_edge_irqs);
void gic_cpu_init(unsigned int gic_nr, const void __iomem *base);
void gic_cascade_irq(unsigned int gic_nr, unsigned int irq);
void gic_enable_ppi(unsigned int);
void gic_raise_softirq(cpumask_t cpumask, unsigned int irq);
void gic_suspend(void);
void gic_resume(void);
#endif

#endif /* __ARCH_CXD4115_GIC_H */
