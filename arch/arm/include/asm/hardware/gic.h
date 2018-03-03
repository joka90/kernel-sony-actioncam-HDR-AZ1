/* 2013-08-20: File changed by Sony Corporation */
/*
 *  arch/arm/include/asm/hardware/gic.h
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARM_HARDWARE_GIC_H
#define __ASM_ARM_HARDWARE_GIC_H

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
#define GIC_DIST_SECURITY		0x080
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
# define GIC_DIST_CONFIG_EDGE		0x2
# define GIC_DIST_CONFIG_MODEL1N	0x1
#define GIC_DIST_LINELEVEL		0xd00
#define GIC_DIST_SOFTINT		0xf00

#ifndef __ASSEMBLY__
extern void __iomem *gic_cpu_base_addr;
extern struct irq_chip gic_arch_extn;

void gic_init(unsigned int, unsigned int, void __iomem *, void __iomem *);
void gic_secondary_init(unsigned int);
void gic_cascade_irq(unsigned int gic_nr, unsigned int irq);
void gic_raise_softirq(const struct cpumask *mask, unsigned int irq);
void gic_dist_clear_pending(unsigned int irq);
void gic_lock(void);
void gic_unlock(void);
void gic_enable_ppi(unsigned int);
#if defined (CONFIG_SNSC_SSBOOT) && !defined (CONFIG_WARM_BOOT_IMAGE)
int gic_suspend(void);
int gic_resume(void);
#endif /* CONFIG_SNSC_SSBOOT */

struct gic_chip_data {
	unsigned int irq_offset;
	void __iomem *dist_base;
	void __iomem *cpu_base;
};

struct irq_data;
extern struct gic_chip_data gic_data[];
extern struct irq_chip gic_chip_edge;
extern struct irq_chip gic_chip;
extern raw_spinlock_t irq_controller_lock;
void gic_emxx_init(unsigned int, unsigned int);
void gic_mask_irq(struct irq_data *);
void gic_unmask_irq(struct irq_data *);
void gic_ack_irq(struct irq_data *);
void gic_ack2_irq(struct irq_data *);
int gic_set_affinity(struct irq_data *, const struct cpumask *, bool);
inline void *gic_dist_base(struct irq_data *d);
inline void *gic_cpu_base(struct irq_data *d);
inline unsigned int gic_irq(struct irq_data *d);
void gic_eoi_irq(struct irq_data *);
int gic_set_type(struct irq_data *, unsigned int);
int gic_retrigger(struct irq_data *);
int gic_set_wake(struct irq_data *, unsigned int);
#endif

#endif
