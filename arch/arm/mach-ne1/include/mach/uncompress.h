/* 2008-09-01: File added by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/uncompress.h
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/uncompress.h
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

#ifndef __ASM_ARCH_UNCOMPRESS_H
#define __ASM_ARCH_UNCOMPRESS_H

#include <asm/memory.h>
#include <mach/hardware.h>


#if defined(CONFIG_MACH_NE1TB)
#define BSP_UART_THR	(*(volatile unsigned char *)IO_ADDRESS(NE1_BASE_UART_0 + 0x00))
#define BSP_UART_LSR	(*(volatile unsigned char *)IO_ADDRESS(NE1_BASE_UART_0 + 0x14))
#endif


/*
 * This does not append a newline
 */
static inline void putc(int c)
{
	while ((BSP_UART_LSR & (1 << 5)) == 0)	/* THR "not" empty ? */
		barrier();

	BSP_UART_THR = c;
}

static inline void flush(void)
{
	while ((BSP_UART_LSR & (1 << 6)) == 0)	/* Tx section "not" empty ? */
		barrier();
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()


#endif /* __ASM_ARCH_UNCOMPRESS_H */

