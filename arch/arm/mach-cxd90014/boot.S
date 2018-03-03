/*
 * arch/arm/mach-cxd90014/boot.S
 *
 *
 * Copyright 2012 Sony Corporation
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
#include <asm/hardware/arm_scu.h>

#if defined(CONFIG_SMP)

	.arm
#if 0 /* The code below must be within 1st MB. */
	__CPUINIT
#endif

ENTRY(cxd90014_secondary_entry)
 THUMB(	adr	r9, BSYM(1f)	)	@ Kernel is always entered in ARM.
 THUMB(	bx	r9		)	@ If this is a Thumb-2 kernel,
 THUMB(	.thumb			)	@ switch to Thumb now.
 THUMB(1:			)

	@ initialize caches
	bl	v7_inv_dcache_all		@ invalidate D-cache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I-cache and BTC
	mcr	p15, 0, r0, c8, c7, 0		@ invalidate whole TLB
	dsb
	isb
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

#ifndef CONFIG_CXD90014_SIMPLE_LOADER
	@ clear BOOT FLAG
	ldr	r10, =(CXD90014_ESRAM_BASE+CXD90014_BOOTRAM_WORK+CXD90014_BOOTRAM_FLAG)
	mov	r11, #0xff
	strb	r11, [r10, r0]
#endif /* !CONFIG_CXD90014_SIMPLE_LOADER */
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
	.align	2
1:	.long	.
	.long	pen_release
ENDPROC(cxd90014_secondary_entry)

	.align	2
	.type	__secondary_vector, %object
__secondary_vector:
	.long	.
	.long	cxd90014_secondary_entry

ENTRY(cxd90014_smp_init)
#ifndef CONFIG_CXD90014_SIMPLE_LOADER
	adr	r2, __secondary_vector
	ldmia	r2, {r0, r1}
	sub	r2, r2, r0
	add	r1, r2

	ldr	r0, =(CXD90014_ESRAM_BASE+CXD90014_BOOTRAM_WORK)
	str	r1, [r0, #CXD90014_BOOTRAM_VEC+0x04] /* CPU#1 */
	str	r1, [r0, #CXD90014_BOOTRAM_VEC+0x08] /* CPU#2 */
	str	r1, [r0, #CXD90014_BOOTRAM_VEC+0x0c] /* CPU#3 */
	mov	r1, #0
	strb	r1, [r0, #CXD90014_BOOTRAM_FLAG+1] /* CPU#1 */
	strb	r1, [r0, #CXD90014_BOOTRAM_FLAG+2] /* CPU#2 */
	strb	r1, [r0, #CXD90014_BOOTRAM_FLAG+3] /* CPU#3 */
	@ MMU=off (Strongly Ordered)
	sev
#endif /* !CONFIG_CXD90014_SIMPLE_LOADER */
	mov	pc, lr
ENDPROC(cxd90014_smp_init)

#else /* CONFIG_SMP */
secondary_entry:
	wfi
	b	secondary_entry
#endif /* CONFIG_SMP */

ENTRY(ej_head_init)
	mov	r8, lr			@ save lr

	@ turn off coherency
	mrc	p15, 0, r0, c1, c0, 1	@ aux
	bic	r0, r0, #0x40		@ clear SMP
	mcr	p15, 0, r0, c1, c0, 1	@
	@ turn MMU off
	mrc	p15, 0, r0, c1, c0, 0	@ ctrl register
	bic	r0, r0, #0x0005		@ .............c.m
	@bic	r0, r0, #0x1000		@ ...i............
	mcr	p15, 0, r0, c1, c0, 0	@ disable MMU and cache
	isb
	@ initialize caches
	bl	v7_inv_dcache_all	@ invalidate D-cache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate I-cache and BTC
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate whole TLB
	dsb
	isb

#ifdef CONFIG_SMP_COMMON_ENTRY
	mrc	p15, 0, r0, c0, c0, 5	@ CPUID
	ands	r0, r0, #0x0f
	bne	secondary_entry
#endif /* CONFIG_SMP_COMMON_ENTRY */

	@ CPU#0
	bl	uart_init
#if defined(CONFIG_DEBUG_EARLY)
	mov	r7, #'1'
	bl	_printch		@ r3, r6 corrupted
#endif
	bl	timer_init		@ start sched clock

	@ disable SCU
	ldr	r0, =SCU_BASE
	ldr	r1, [r0, #SCU_CTRL]
	bic	r1, r1, #SCU_ENABLE
	str	r1, [r0, #SCU_CTRL]
	@ Invalidate SCU duplicate tags
	ldr	r1, =SCU_INV_ALL
	str	r1, [r0, #SCU_INVALIDATE]
#if defined(CONFIG_CACHE_PL310)
	bl	pl310_init
#endif
	@ enable SCU
	ldr	r0, =SCU_BASE
	ldr	r1, [r0, #SCU_CTRL]
	orr	r1, r1, #SCU_ENABLE
	str	r1, [r0, #SCU_CTRL]

#if defined(CONFIG_SMP)
	bl	cxd90014_smp_init
#endif

	mov	r0, #0
	ldr	r1, =MACH_TYPE_CXD90014
	mov	r2, #0xffffffff		@ atags pointer

	mov	pc, r8
ENDPROC(ej_head_init)
