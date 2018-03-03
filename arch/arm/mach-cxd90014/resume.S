/*
 * arch/arm/mach-cxd90014/resume.S
 *
 * pm callback for cxd90014
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on arch/arm/mach-sa1100/sleep.S
 */
/*
 * SA11x0 Assembler Sleep/WakeUp Management Routines
 *
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2001-02-06: Cliff Brake         Initial code
 *
 * 2001-08-29:	Nicolas Pitre	Simplified.
 *
 * 2002-05-27:	Nicolas Pitre	Revisited, more cleanup and simplification.
 *				Storage is on the stack now.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>
#ifdef CONFIG_SMP
#include <asm/hardware/arm_scu.h>
#endif
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot_stat.h>
#endif
/*
 * cxd90014_cpu_resume()
 *
 * entry point from bootloader into kernel during resume
 *   Redboot does not have resume vector.
 *   Please jump to this vector by "go" command of RedBoot.
 */
/*
 * Execute only 1 CPU.
 * Because, other CPUs are offline before sleep.
 */

	.arm
#ifdef CONFIG_SNSC_SSBOOT
	.globl  cxd90014_ssboot_stat
cxd90014_ssboot_stat:	.long   SSBOOT_CREATE_MINSSBI

ENTRY(cxd90014_cpu_resume_profile)
	mov     r1, #SSBOOT_PROFILE
	b       cxd90014_cpu_resume_common
ENTRY(cxd90014_cpu_resume_optimize)
	mov     r1, #SSBOOT_CREATE_OPTSSBI
	b       cxd90014_cpu_resume_common
ENTRY(cxd90014_cpu_resume_ssboot_pre)
	mov     r1, #SSBOOT_SSBOOT_PRE
	b       cxd90014_cpu_resume_common
ENTRY(cxd90014_cpu_resume_ssboot)
	mov     r1, #SSBOOT_SSBOOT
	b       cxd90014_cpu_resume_common

ENTRY(cxd90014_cpu_resume_common)
	adr     r0, cxd90014_ssboot_stat
	str     r1, [r0]
#endif
ENTRY(cxd90014_cpu_resume)
 THUMB( adr     r9, BSYM(1f)    )       @ Kernel is always entered in ARM.
 THUMB( bx      r9              )       @ If this is a Thumb-2 kernel,
 THUMB( .thumb                  )       @ switch to Thumb now.
 THUMB(1:                       )

	setmode	PSR_F_BIT|PSR_I_BIT|SVC_MODE, r9

#if defined(CONFIG_DEBUG_LL) || defined(CONFIG_CXD90014_UART_IO_INIT)
	bl	uart_init
#endif /* CONFIG_DEBUG_LL || CONFIG_CXD90014_UART_IO_INIT */
#ifdef CONFIG_DEBUG_LL
	mov	r0, #'R'
	bl	printch				@ r1-r3 corrupted
#endif /* CONFIG_DEBUG_LL */

	@ clean & invalide D&I cache
	bl	v7_flush_kern_cache_all

	dsb
	dmb

	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0		@ invalidate I + D TLBs

	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x0007			@ .............cam
	bic	r0, r0, #0x1100			@ ...i...s........
	mcr	p15, 0, r0, c1, c0, 0		@ ctrl register

	nop
	nop

#if defined(CONFIG_CACHE_PL310)
	bl	pl310_init		@ r0-r3 corrupted
#endif

#if defined(CONFIG_SMP)
	bl	cxd90014_smp_init
#endif

	ldr	r0, sleep_save_sp_local		@ stack phys addr
	ldr	r1, =resume_after_mmu		@ its absolute virtual address
	ldmfd	r0!, {r2 - r12}			@ CP regs

	mcr p15, 0, r2, c3, c0, 0		@ domain ID

	mcr p15, 0, r3, c1, c0, 2		@ Coprocessor Access Control Register
	isb

	mcr	p15, 0, r4, c2, c0, 2		@ TTB control register
	mcr p15, 0, r5, c2, c0, 0		@ TTB0
	mcr	p15, 0, r6, c2, c0, 1		@ TTB1

	mcr	p15, 0, r7, c13, c0, 1		@ context ID
	mcr	p15, 0, r8, c13, c0, 2		@ thread ID
	mcr	p15, 0, r9, c13, c0, 3		@ thread ID
	mcr	p15, 0, r10, c13, c0, 4		@ thread ID

	mcr p15, 0, r11, c10, c2, 0		@ PRRR
	mcr p15, 0, r12, c10, c2, 1		@ NMRR

 ARM(	ldmfd	r0!, {r2 - r3, sp})		@ CP regs + virt stack ptr
 THUMB(	ldmfd	r0!, {r2 - r4})
 THUMB(	mov	sp, r4)

#ifdef CONFIG_SMP
	/* Set up the SCU */
	ldr	r0, =SCU_BASE
	ldr	r5, =SCU_INV_ALL
	str	r5, [r0, #SCU_INVALIDATE]
	ldr	r5, [r0, #SCU_CTRL]
	orr	r5, r5, #1
	str	r5, [r0, #SCU_CTRL]
# ifndef CONFIG_CPU_DCACHE_DISABLE
	orr	r3, r3, #0x20			@ Enable SMP/nAMP mode
# endif
#endif /* CONFIG_SMP */
	mcr p15, 0, r3, c1, c0, 1	@ aux control reg.
	b	resume_turn_on_mmu		@ cache align execution

	.align 5
resume_turn_on_mmu:
	mcr	p15, 0, r2, c1, c0, 0	@ turn on MMU, caches, etc.
	nop
	mov	pc, r1					@ jump to virtual addr
	nop
	nop
	nop
	nop

	.globl	sleep_save_sp
sleep_save_sp:
sleep_save_sp_local:
	.word	0					@ preserve stack phys ptr here

resume_after_mmu:

#ifdef CONFIG_DEBUG_LL
	mov	r0, #'S'
	bl	printch						@ r1-r3 corrupted
#endif /* CONFIG_DEBUG_LL */

	mov	r0, #0
	ldmfd	sp!, {r4 - r12, pc}		@ return to caller
ENDPROC(cxd90014_cpu_resume)
