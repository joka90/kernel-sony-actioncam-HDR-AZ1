/* 2009-02-10: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/headsmp.S
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on arch/arm/mach-ne1/headsmp.S
 *
 * Copyright (c) 2003 ARM Limited
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/linkage.h>
#include <linux/init.h>


        __CPUINIT

/*
 * NaviEngine1 specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 */
ENTRY(ne1_secondary_startup)
	msr	cpsr_c,#0xd3

        mrc     p15, 0, r0, c0, c0, 5
        and     r0, r0, #15

#if defined(CONFIG_MACH_NE1TB)
	mov	r4, r0, LSL #8		@ calculate delay time from cpu id
9:	cmp	r4, #0			@ delay...
	sub	r4, r4, #1
	bne	9b

        adr     r4, 1f
        ldmia   r4, {r5, r6, r7}
        sub     r4, r4, r5		@ r6 = PA of pen_release
        add     r6, r6, r4		@ r7 = PA of NE1-TB FLED in FPGA

	mov	r5, #0x1
	mov	r4, r5, LSL r0
	ldrh	r5, [r7]
	orr	r5, r5, r4
	strh	r5, [r7]		@ turn on LED[cpu id]
#else
        adr     r4, 1f
        ldmia   r4, {r5, r6}
        sub     r4, r4, r5
        add     r6, r6, r4
#endif

pen:    ldr     r7, [r6]
        cmp     r7, r0
        bne     pen

        /*
         * we've been released from the holding pen: secondary_stack
         * should now contain the SVC stack for this core
         */
	mvn	r7, #0
	str	r7, [r6]
        b       secondary_startup

1:      .long   .
        .long   pen_release
#if defined(CONFIG_MACH_NE1TB)
	.long	(0x04010000 + 0x0a06)
#endif

