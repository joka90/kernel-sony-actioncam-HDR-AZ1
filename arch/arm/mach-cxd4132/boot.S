/*
 * arch/arm/mach-cxd4132/boot.S
 *
 *
 * Copyright 2009,2010 Sony Corporation
 *
 * This code is based on arch/arm/kernel/head.S.
 */
/*
 *  linux/arch/arm/kernel/head.S
 *
 *  Copyright (C) 1994-2002 Russell King
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Kernel startup code for all 32-bit CPUs
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/mach-types.h>
#include <asm/domain.h>
#include <asm/system.h>
#include <asm/assembler.h>
#include <mach/platform.h>
#include <mach/bootram.h>
#include <asm/hardware/gic.h>

#if defined(CONFIG_SMP)

#if 0 /* The code below must be within 1st MB. */
	__CPUINIT
#endif

	.align 5
ENTRY(cxd4115_secondary_entry)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I-cache
	mcr	p15, 0, r0, c7, c6, 0		@ invalidate D-cache
	mrc	p15, 0, r0, c0, c0, 5		@ CPUID
	ands	r0, r0, #0x0f
secondary_entry:
	ldr	r2, =GIC_CPU_BASE		@ GIC CPU Interface
	mov	r5, #1
	str	r5, [r2,#GIC_CPU_CTRL]		@ Enable CPU Interface
	mov	r5, #0xf0
	str	r5, [r2,#GIC_CPU_PRIMASK]	@ R0P0 workaround
	adr	r4, 1f
	ldmia	r4, {r5, r6}
	sub	r4, r4, r5
	add	r6, r6, r4
	ldr	r5, =0x3ff

pen:
	wfi
	ldr	r3, [r2,#GIC_CPU_INTACK]
	and	r4, r3, r5
	cmp	r4, r5
	strne	r3, [r2,#GIC_CPU_EOI]
	ldr	r7, [r6]                        @ busy wait
	cmp	r7, r0
	bne	pen

	b	secondary_startup
1:	.long	.
	.long	pen_release

	.type	__secondary_vector, %object
__secondary_vector:
	.long	.
	.long	cxd4115_secondary_entry

ENTRY(cxd4115_smp_init)
#ifndef CONFIG_CXD41XX_SIMPLE_LOADER
	adr	r2, __secondary_vector
	ldmia	r2, {r0, r1}
	sub	r2, r2, r0
	add	r1, r2

	ldr	r0, =(CXD4132_SRAM_BASE+CXD4132_BOOTRAM_WORKAREA)
	str	r1, [r0, #CXD4132_BOOTRAM_VECTOR+4] /* CPU#1 */
	str	r1, [r0, #CXD4132_BOOTRAM_VECTOR+8] /* CPU#2 */
	mov	r1, #0
	strb	r1, [r0, #CXD4132_BOOTRAM_FLAG+1] /* CPU#1 */
	strb	r1, [r0, #CXD4132_BOOTRAM_FLAG+2] /* CPU#2 */
	sev
#endif /* !CONFIG_CXD41XX_SIMPLE_LOADER */

	mov	pc, lr
#else /* CONFIG_SMP */
secondary_entry:
	wfi
	b	secondary_entry
#endif /* CONFIG_SMP */

ENTRY(ej_head_init)
	mov	r4, lr			@ save lr

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0   @ invalidate I-cache
	mcr	p15, 0, r0, c7, c6, 0   @ invalidate D-cache

 	mrc	p15, 0, r0, c1, c0, 0	@ ctrl register
 	bic	r0, r0, #0x0001		@ ...............m
 	mcr	p15, 0, r0, c1, c0, 0	@ disable MMU

#ifdef CONFIG_SMP_COMMON_ENTRY
	mrc	p15, 0, r0, c0, c0, 5		@ CPUID
	ands	r0, r0, #0x0f
	bne	secondary_entry
#endif /* CONFIG_SMP_COMMON_ENTRY */

	bl	uart_init
#if defined(CONFIG_DEBUG_EARLY)
	mov	r7, #'1'
	bl	_printch		@ r3, r6 corrupted
#endif
	bl	timer_init		@ start sched clock
#if defined(CONFIG_CACHE_PL310S)
	bl	cxd4132_pl310_setup
#endif
#if defined(CONFIG_SMP)
	bl	cxd4115_smp_init
#endif

	mov	r0, #0
	ldr	r1, =MACH_TYPE_CXD4132
	mov	r2, #0xffffffff		@ atags pointer

	mov	pc, r4

#if defined(CONFIG_SMP_COMMON_ENTRY) && !defined(CONFIG_SMP)
secondary:
#endif /* CONFIG_SMP_COMMON_ENTRY && !CONFIG_SMP */
