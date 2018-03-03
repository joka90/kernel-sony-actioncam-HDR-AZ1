/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/arch/arm/mm/cache-v7.S
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright (C) 2005 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is the "shell" of the ARMv7 processor support.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/unwind.h>

#include "proc-macros.S"

/*
 *	v7_flush_icache_all()
 *
 *	Flush the whole I-cache.
 *
 *	Registers:
 *	r0 - set to 0
 */
ENTRY(v7_flush_icache_all)
	mov	r0, #0
	ALT_SMP(mcr	p15, 0, r0, c7, c1, 0)		@ invalidate I-cache inner shareable
	ALT_UP(mcr	p15, 0, r0, c7, c5, 0)		@ I+BTB cache invalidate
	mov	pc, lr
ENDPROC(v7_flush_icache_all)

/*
 *	v7_dcache_all()
 *
 *	Do cache operation for the whole D-cache.
 *
 *	Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 *
 *	- mm    - mm_struct describing address space
 */
	.macro  v7_dcache_all, name, crm, hook

ENTRY(\name)
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	\name\()_finished		@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
\name\()_loop1:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt     \name\()_skip			@ skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
	save_and_disable_irqs_notrace r9	@ make cssr&csidr read atomic
#endif
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
#ifdef CONFIG_PREEMPT
	restore_irqs_notrace r9
#endif
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
	\hook
\name\()_loop2:
	mov	r9, r4				@ create working copy of max way size
\name\()_loop3:
 ARM(	orr	r11, r10, r9, lsl r5	)	@ factor way and cache number into r11
 THUMB(	lsl	r6, r9, r5		)
 THUMB(	orr	r11, r10, r6		)	@ factor way and cache number into r11
 ARM(	orr	r11, r11, r7, lsl r2	)	@ factor index number into r11
 THUMB(	lsl	r6, r7, r2		)
 THUMB(	orr	r11, r11, r6		)	@ factor index number into r11
	mcr	p15, 0, r11, c7, \crm, 2	@ cache maintenance by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	\name\()_loop3
	subs	r7, r7, #1			@ decrement the index
	bge	\name\()_loop2
\name\()_skip:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	\name\()_loop1
\name\()_finished:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
ENDPROC(\name)

	.endm

#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS
	.macro	hook_init_dcache_size
	add	r4, r4, #1
	add	r7, r7, #1
	mul	r4, r4, r7			@ way size * index size
	lsl	r2, r4, r2			@ line number << line size order
	ldr	r4, =v7_dcache_size
	ldr	r7, [r4]			@ load v7_dcache_size
	ldr	r5, =0xffffffff
	cmp	r5, r7
	ldreq	r7, =0				@ init v7_dcache_size at first time
	cmp	r2, r7
	strgt	r2, [r4]			@ store cache size to v7_dcache_size
	b	v7_init_dcache_size_skip
	.endm

	v7_dcache_all v7_init_dcache_size, c14 /* dummy */, hook_init_dcache_size

	.pushsection .data
ENTRY(v7_dcache_size)
	.long	0xffffffff
	.popsection
#endif /* CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS */

	v7_dcache_all v7_flush_dcache_all, c14
#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS
	v7_dcache_all v7_clean_dcache_all, c10
#endif

/*
 *	v7_inv_dcache_all()
 *
 *	Invalidate the whole D-cache.
 *
 *	Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 *
 *	- mm    - mm_struct describing address space
 */
	v7_dcache_all v7_inv_dcache_all,   c6

/*
 *	v7_flush_cache_all()
 *
 *	Flush the entire cache system.
 *  The data cache flush is now achieved using atomic clean / invalidates
 *  working outwards from L1 cache. This is done using Set/Way based cache
 *  maintenance instructions.
 *  The instruction cache can still be invalidated back to the point of
 *  unification in a single instruction.
 *
 */
ENTRY(v7_flush_kern_cache_all)
 ARM(	stmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	stmfd	sp!, {r4-r7, r9-r11, lr}	)
	bl	v7_flush_dcache_all
	ALT_SMP(mcr	p15, 0, r0, c7, c1, 0)	@ invalidate I-cache inner shareable
	ALT_UP(mcr	p15, 0, r0, c7, c5, 0)	@ I+BTB cache invalidate
 ARM(	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	ldmfd	sp!, {r4-r7, r9-r11, lr}	)
	mov	pc, lr
ENDPROC(v7_flush_kern_cache_all)

#ifdef CONFIG_PAGECACHE_PREALLOC
/*
 *	v7_flush_kern_dcache_all()
 *	Flush all dcache, not include icache.
 */
ENTRY(v7_flush_kern_dcache_all)
 ARM(	stmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	stmfd	sp!, {r4-r7, r9-r11, lr}	)
	bl	v7_flush_dcache_all
	mov	r0, #0
 ARM(	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	ldmfd	sp!, {r4-r7, r9-r11, lr}	)
	mov	pc, lr
ENDPROC(v7_flush_kern_dcache_all)
#endif

/*
 *	v7_flush_cache_all()
 *
 *	Flush all TLB entries in a particular address space
 *
 *	- mm    - mm_struct describing address space
 */
ENTRY(v7_flush_user_cache_all)
	/*FALLTHROUGH*/

/*
 *	v7_flush_cache_range(start, end, flags)
 *
 *	Flush a range of TLB entries in the specified address space.
 *
 *	- start - start address (may not be aligned)
 *	- end   - end address (exclusive, may not be aligned)
 *	- flags	- vm_area_struct flags describing address space
 *
 *	It is assumed that:
 *	- we have a VIPT cache.
 */
ENTRY(v7_flush_user_cache_range)
	mov	pc, lr
ENDPROC(v7_flush_user_cache_all)
ENDPROC(v7_flush_user_cache_range)

/*
 *	v7_coherent_kern_range(start,end)
 *
 *	Ensure that the I and D caches are coherent within specified
 *	region.  This is typically used when code has been written to
 *	a memory region, and will be executed.
 *
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 *
 *	It is assumed that:
 *	- the Icache does not read data from the write buffer
 */
ENTRY(v7_coherent_kern_range)
	/* FALLTHROUGH */

/*
 *	v7_coherent_user_range(start,end)
 *
 *	Ensure that the I and D caches are coherent within specified
 *	region.  This is typically used when code has been written to
 *	a memory region, and will be executed.
 *
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 *
 *	It is assumed that:
 *	- the Icache does not read data from the write buffer
 */
ENTRY(v7_coherent_user_range)
 UNWIND(.fnstart		)
	dcache_line_size r2, r3
	sub	r3, r2, #1
	bic	r12, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
1:
 USER(	mcr	p15, 0, r12, c7, c11, 1	)	@ clean D line to the point of unification
	add	r12, r12, r2
	cmp	r12, r1
	blo	1b
	dsb
	icache_line_size r2, r3
	sub	r3, r2, #1
	bic	r12, r0, r3
2:
 USER(	mcr	p15, 0, r12, c7, c5, 1	)	@ invalidate I line
	add	r12, r12, r2
	cmp	r12, r1
	blo	2b
3:
	mov	r0, #0
	ALT_SMP(mcr	p15, 0, r0, c7, c1, 6)	@ invalidate BTB Inner Shareable
	ALT_UP(mcr	p15, 0, r0, c7, c5, 6)	@ invalidate BTB
	dsb
	isb
	mov	pc, lr

/*
 * Fault handling for the cache operation above. If the virtual address in r0
 * isn't mapped, just try the next page.
 */
9001:
#ifdef CONFIG_ARM_ERRATA_775420
	dsb
#endif
	mov	r12, r12, lsr #12
	mov	r12, r12, lsl #12
	add	r12, r12, #4096
	b	3b
 UNWIND(.fnend		)
ENDPROC(v7_coherent_kern_range)
ENDPROC(v7_coherent_user_range)

/*
 *	v7_flush_kern_dcache_area(void *addr, size_t size)
 *
 *	Ensure that the data held in the page kaddr is written back
 *	to the page in question.
 *
 *	- addr	- kernel address
 *	- size	- region size
 */
ENTRY(v7_flush_kern_dcache_area)
	dcache_line_size r2, r3
	add	r1, r0, r1
	sub	r3, r2, #1
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
1:
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D line / unified line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr
ENDPROC(v7_flush_kern_dcache_area)

#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_UP
/*
 *	v7_dma_reduce_cache_ops
 *
 *	Do cache operation for the whole D-cache and return if specified
 *	region is larger than cache size.
 *
 *	- r0 - virtual start address of region
 *	- r1 - virtual end address of region
 */
	.macro v7_dma_reduce_cache_ops ops
	ldr	r2, =v7_dcache_size
	ldr	r3, [r2]
	sub	r2, r1, r0
	cmp	r2, r3
	blo	2f
 ARM(	stmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	stmfd	sp!, {r4-r7, r9-r11, lr}	)
	bl	v7_\ops\()_dcache_all
 ARM(	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	ldmfd	sp!, {r4-r7, r9-r11, lr}	)
	mov	pc, lr
2:
	.endm
#endif

#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_SMP
/*
 *	v7_dma_clean_all(cpu, ready, complete)
 *	v7_dma_flush_all(cpu, ready, complete)
 *
 *	Do cache operation for the whole D-cache
 *	in synchronization with other cpus.
 *
 *	- cpu      - current cpu
 *	- ready    - flag to wait for other cpus to ready
 *	- complete - flag to wait for other cpus to complete
 */
	.macro v7_dma_dcache_all name, ops
\name:
	stmfd	sp!, {r4-r12, lr}
	dma_dcache_all_sync r0, r1, r3		@ wait for all cpus to ready
	mov	r8, r0				@ r8/r12 are kept across v7_dcache_all
	mov	r12, r2
	bl	v7_\ops\()_dcache_all		@ do cache operations for all lines
	dma_dcache_all_sync r8, r12, r3		@ wait for all cpus to complete
	ldmfd	sp!, {r4-r12, pc}
ENDPROC(\name)
	.endm

	v7_dma_dcache_all v7_dma_clean_all clean
	v7_dma_dcache_all v7_dma_flush_all flush

/*
 *	v7_dma_reduce_cache_ops
 *
 *	Call dma_dcache_all() and return if dma_dcache_all() does cache
 *	operation for specified region.
 *
 *	- r0 - virtual start address of region
 *	- r1 - virtual end address of region
 */
	.macro v7_dma_reduce_cache_ops ops
	stmfd	sp!, {r0-r1, lr}
	ldr	r2, =v7_dcache_size
	ldr	r3, =v7_dma_\ops\()_all
	bl	dma_dcache_all
	teq	r0, #0
	ldmfd	sp!, {r0-r1, lr}
	movne	pc, lr
	.endm
#endif /* CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_SMP */

/*
 *	v7_dma_inv_range(start,end)
 *
 *	Invalidate the data cache within the specified region; we will
 *	be performing a DMA operation in this region and we want to
 *	purge old data in the cache.
 *
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 */
v7_dma_inv_range:
#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS
	v7_dma_reduce_cache_ops flush
#endif
	dcache_line_size r2, r3
	sub	r3, r2, #1
	tst	r0, r3
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
	mcrne	p15, 0, r0, c7, c14, 1		@ clean & invalidate D / U line

	tst	r1, r3
	bic	r1, r1, r3
	mcrne	p15, 0, r1, c7, c14, 1		@ clean & invalidate D / U line
1:
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D / U line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr
ENDPROC(v7_dma_inv_range)

/*
 *	v7_dma_clean_range(start,end)
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 */
v7_dma_clean_range:
#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS
	v7_dma_reduce_cache_ops clean
#endif
	dcache_line_size r2, r3
	sub	r3, r2, #1
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
1:
	mcr	p15, 0, r0, c7, c10, 1		@ clean D / U line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr
ENDPROC(v7_dma_clean_range)

/*
 *	v7_dma_flush_range(start,end)
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 */
ENTRY(v7_dma_flush_range)
#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS
	v7_dma_reduce_cache_ops flush
#endif
	dcache_line_size r2, r3
	sub	r3, r2, #1
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
1:
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D / U line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr
ENDPROC(v7_dma_flush_range)

/*
 *	dma_map_area(start, size, dir)
 *	- start	- kernel virtual start address
 *	- size	- size of region
 *	- dir	- DMA direction
 */
ENTRY(v7_dma_map_area)
	add	r1, r1, r0
	teq	r2, #DMA_FROM_DEVICE
	beq	v7_dma_inv_range
	b	v7_dma_clean_range
ENDPROC(v7_dma_map_area)

/*
 *	dma_unmap_area(start, size, dir)
 *	- start	- kernel virtual start address
 *	- size	- size of region
 *	- dir	- DMA direction
 */
ENTRY(v7_dma_unmap_area)
	add	r1, r1, r0
	teq	r2, #DMA_TO_DEVICE
	bne	v7_dma_inv_range
	mov	pc, lr
ENDPROC(v7_dma_unmap_area)

	__INITDATA

	@ define struct cpu_cache_fns (see <asm/cacheflush.h> and proc-macros.S)
	define_cache_functions v7
