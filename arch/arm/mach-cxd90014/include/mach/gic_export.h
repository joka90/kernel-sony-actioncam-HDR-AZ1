/*
 * arch/arm/mach-cxd90014/include/mach/gic_export.h
 *
 * MPCore Distributed Interrupt Controller definitions
 *
 * Copyright 2012 Sony Corporation
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
#ifndef __ARCH_CXD90014_GIC_EXPORT_H
#define __ARCH_CXD90014_GIC_EXPORT_H

#include <linux/compiler.h>
#include <linux/cpumask.h>
#include <asm/hardware/gic.h>

/* Enable, Pending, Active, LineLevel reg. */
#define GIC_IRQ_BITS_SHIFT	5 /* packed 2^5 in u32 */
#define GIC_IRQ_BITS_MASK	((1 << GIC_IRQ_BITS_SHIFT) - 1)
#define GIC_IRQ_BITMASK(irq)	(1 << ((irq) & GIC_IRQ_BITS_MASK))
#define GIC_IRQ_OFFSET(irq)	(((irq) >> GIC_IRQ_BITS_SHIFT) << 2)

#define GIC_IRQ_PENDING(irq)	(readl_relaxed(VA_GIC_DIST + GIC_DIST_PENDING_SET + GIC_IRQ_OFFSET(irq)) & GIC_IRQ_BITMASK(irq))

#define GIC_IRQ_RAWSTATUS(irq)	(readl_relaxed(VA_GIC_DIST + GIC_DIST_LINELEVEL + GIC_IRQ_OFFSET(irq)) & GIC_IRQ_BITMASK(irq))

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

/* for byte access */
#define GIC_IRQ_TARGET(irq)	(VA_GIC_DIST + GIC_DIST_TARGET + (irq))

#define gic_setirq(irq)		writel_relaxed(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_PENDING_SET + GIC_IRQ_OFFSET(irq))
#define gic_clearirq(irq)	writel_relaxed(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_PENDING_CLEAR + GIC_IRQ_OFFSET(irq))
#define gic_pollirq(irq)	!!GIC_IRQ_RAWSTATUS(irq)
#define gic_maskirq(irq, enable) \
({ \
	if (enable) \
		writel_relaxed(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_SET + GIC_IRQ_OFFSET(irq)); \
	else \
		writel_relaxed(GIC_IRQ_BITMASK(irq), VA_GIC_DIST + GIC_DIST_ENABLE_CLEAR + GIC_IRQ_OFFSET(irq)); \
})

#ifndef __ASSEMBLY__
extern void gic_cxd90014_init(void);
extern void gic_dump(void);
extern void gic_config_edge(unsigned int irq);
extern unsigned int gic_get_irq_priority(unsigned int irq);
extern unsigned int gic_set_irq_priority(unsigned int irq, unsigned int prio);
extern unsigned int gic_get_priority_mask(void);
extern void gic_set_priority_mask(unsigned int prio_mask);
extern void gic_cxd90014_suspend(void);
extern void gic_cxd90014_resume(void);
#endif

#endif /* __ARCH_CXD90014_GIC_EXPORT_H */
