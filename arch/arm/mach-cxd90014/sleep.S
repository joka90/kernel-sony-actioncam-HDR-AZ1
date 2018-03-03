/*
 * arch/arm/mach-cxd90014/sleep.S
 *
 * pm callback for cxd90014
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
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
#include <mach/debug-macro.S>
#include <mach/pm.h>
#include <mach/pm_macro.S>
#include <mach/time.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <mach/regs-misc.h>

		.data
/*
 * cxd90014_cpu_suspend()
 *
 * Causes cxd90014 to enter sleep state
 *
 */

ENTRY(cxd90014_cpu_suspend)
	@ r0: mode flag
	stmfd	sp!, {r4 - r12, lr}		@ save registers on stack
 ARM(	str	r0, modeflag)			@ save mode flag
 THUMB(	ldr	r1, =modeflag)
 THUMB(	str	r0, [r1])

	@ get coprocessor registers
	mrc 	p15, 0, r2, c1, c0, 0		@ control reg
	mrc 	p15, 0, r3, c1, c0, 1		@ aux control reg

	@ store them plus current virtual stack ptr on stack
	mov	r4, sp
	stmfd	sp!, {r2 - r4}

	@ get coprocessor registers
	mrc p15, 0, r2, c3, c0, 0		@ domain ID

	mrc p15, 0, r3, c1, c0, 2		@ Coprocessor Access Control Register

	mrc p15, 0, r4, c2, c0, 2		@ TTB control register
	mrc p15, 0, r5, c2, c0, 0		@ TTB0
	mrc p15, 0, r6, c2, c0, 1		@ TTB1

	mrc	p15, 0, r7, c13, c0, 1		@ context ID
	mrc	p15, 0, r8, c13, c0, 2		@ thread ID
	mrc	p15, 0, r9, c13, c0, 3		@ thread ID
	mrc	p15, 0, r10, c13, c0, 4		@ thread ID

	mrc p15, 0, r11, c10, c2, 0		@ PRRR
	mrc p15, 0, r12, c10, c2, 1		@ NMRR

	@ store them on stack
	stmfd	sp!, {r2 - r12}

	@ preserve phys address of stack
	mov	r0, sp
	bl	sleep_phys_sp
	ldr	r1, =sleep_save_sp
	str	r0, [r1]

	@ disable interrupt
	mov	r0, #PSR_F_BIT | PSR_I_BIT | SVC_MODE
	msr	cpsr_c, r0

	@ clean & invalide D&I cache
	bl	v7_flush_kern_cache_all

	ldr	r0, modeflag
	cmp	r0, #PM_ENTER_NORMAL
	bne	goto_suspend

	@ for creating Warm-Boot-Image
	bl	cxd90014_create_warmbootimage
	cmp	r0, #0
	beq	goto_suspend
	/*
		failure handling for warm_boot_image
	*/
	mov	r0, sp
	ldmfd	r0!, {r2 - r12}
 ARM(	ldmfd	r0!, {r2 - r3, sp})
 THUMB(	ldmfd	r0!, {r2 - r4})
 THUMB(	mov	sp, r4)
	ldmfd	sp!, {r4 - r12, pc}

goto_suspend:
	mov	r0, #1
	ldr	r1, =cxd90014_mem_alive
	str	r0, [r1]			@ save memory alive flag
#ifdef CONFIG_SNSC_DEBUG_PROFILE
	adr	r0, __printascii
	bl	profile_dump_record
#endif
	@ copy the standby operation code to the resume ram
	pm_workarea_va r0
	ldr	r1, =exec_in_resume_ram
	ldr	r2, =exec_in_resume_ram_end

code_copy:
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	teq	r1, r2
	bne	code_copy

	@ clean & invalide D&I cache
	bl	v7_flush_kern_cache_all

	mrc	p15, 0, r0, c1, c0, 0	@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x0006			@ .............ca.
	mcr	p15, 0, r0, c1, c0, 0	@ disable caches
	pm_outer_cache

	ldr r0, =0xffffffff
	mcr p15, 0, r0, c3, c0, 0		@ domain ID

	pm_stack_phys sp			@ Physical address
	pm_workarea_phys r1			@ Physical address

	@ jump to the resume ram(Physical address)
	mov	pc, r1
	nop
	nop

	.ltorg
	.align	5
exec_in_resume_ram:

/*--------------------- MMU OFF --------------------------------*/

 	@ disable MMU, the mapping is flat so
 	mrc	p15, 0, r0, c1, c0, 0	@ ctrl register
 	bic	r0, r0, #0x0001			@ ...............m
 	mcr	p15, 0, r0, c1, c0, 0	@ disable MMU

/*--------------------- Memory power down ----------------------*/
	pm_memory
/*--------------------------------------------------------------*/

	ldr r0, cxd90014_sus_end_msg_local
	cmp r0,	#0
	beq 1f

	mov	r0, #'E'
	bl	__printch
	mov	r0, #'\r'
	bl	__printch
	mov	r0, #'\n'
	bl	__printch
1:

/*--------------------- D-D Conv. control ----------------------*/
dd_off:

	ldr	r0, cxd90014_xpower_off_addr_local
	cmp	r0, #0xffffffff		@ not defined
	beq	1f
	ldr	r1, cxd90014_xpower_off_bitmask_local
	str	r1, [r0]			@ assert
1:
	@ disable XRSTREQ because WDT is still running.
	ldr	r0, =(CXD90014_MISC_BASE+MISC_RSTREQ_CLR)
	mov	r1, #MISC_RSTREQ_EN
	str	r1, [r0]
#if 0 /* XRSTREQ injection for TEST */
	ldr	r0, =(CXD90014_MISC_BASE+MISC_RSTREQ_SET)
	mov	r1, #MISC_RSTREQ_ASSERT
	str	r1, [r0]
#endif

	@ wait for interrupt
	mov	r0, #0
1:
	mcr	p15, 0, r0, c7, c0, 4
	b	1b
/*--------------------------------------------------------------*/

modeflag:	.long	0x00

		.globl	cxd90014_xpower_off_addr
cxd90014_xpower_off_addr:
cxd90014_xpower_off_addr_local:
		.long	0xffffffff

		.globl	cxd90014_xpower_off_bitmask
cxd90014_xpower_off_bitmask:
cxd90014_xpower_off_bitmask_local:
		.long	0xffffffff

		.globl	cxd90014_ddr_pasr0
cxd90014_ddr_pasr0:
cxd90014_ddr_pasr0_local:
		.long	0

		.globl	cxd90014_ddr_pasr1
cxd90014_ddr_pasr1:
cxd90014_ddr_pasr1_local:
		.long	0

		.globl	cxd90014_sus_end_msg
cxd90014_sus_end_msg:
cxd90014_sus_end_msg_local:
		.long	0

		.align	4
	@ for debug
__printhex8:
	mov	r1, #8
	b	__printhex

__printhex4:
	mov	r1, #4
	b	__printhex

__printhex2:
	mov	r1, #2
__printhex:
	adr	r2, hexbuf
	add	r3, r2, r1
	mov	r1, #0
	strb	r1, [r3]
1:	and	r1, r0, #15
	mov	r0, r0, lsr #4
	cmp	r1, #10
	addlt	r1, r1, #'0'
	addge	r1, r1, #'a' - 10
	strb	r1, [r3, #-1]!
	teq	r3, r2
	bne	1b
	mov	r0, r2
	b	__printascii

__printascii:
	addruart r2, r3
	b	2f
1:	waituart r2, r3
	senduart r1, r3
	busyuart r2, r3
	teq	r1, #'\n'
	moveq	r1, #'\r'
	beq	1b
2:	teq	r0, #0
	ldrneb	r1, [r0], #1
	teqne	r1, #0
	bne	1b
	mov	pc, lr

__printch:
	addruart r2, r3
	mov	r1, r0
	mov	r0, #0
	b	1b

hexbuf:		.space 16
	.ltorg
exec_in_resume_ram_end:

ENDPROC(cxd90014_cpu_suspend)
