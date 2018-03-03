/* 2010-11-30: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/debug-macro.S
 *
 * Copyright (C) 1994-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef __ASM_ARCH_DEBUG_MACRO_S
#define __ASM_ARCH_DEBUG_MACRO_S


#if defined(CONFIG_MACH_NE1TB)
	.macro	addruart, rp, rv
	mov	\rp,      #0x18000000	@ physical base
	mov	\rv, \rp, lsr #4	@ virtual base
	orr	\rv, \rv, #0xf0000000
	orr	\rv, \rv, #0x00034000	@ offset
	orr	\rp, \rp, #0x00034000	@ offset
	.endm

#define UART_SHIFT			2
#endif

#include <asm/hardware/debug-8250.S>


#endif /* __ASM_ARCH_DEBUG_MACRO_S */
