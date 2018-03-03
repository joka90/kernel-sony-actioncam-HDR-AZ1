/* 2008-09-01: File added by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/timex.h
 *
 * NE1-xB architecture timex specifications
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/timex.h
 *
 * Copyright (C) 2003 ARM Limited
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

#ifndef __ASM_ARCH_TIMEX_H
#define __ASM_ARCH_TIMEX_H


/*
 * Set 0x00 to the prescaler. If IDMODE11 (DIPSW[6]) is:
 *   ON .... base clock 33.333 MHz (CPU Clock is 399.996 MHz)
 *   OFF ... base clock 25.000 MHz (CPU Clock is 300.000 MHz)
 */
#define CLOCK_TICK_RATE		(((33333000 * 12) / 2) / (0x00 + 1))


#endif /* __ASM_ARCH_TIMEX_H */
