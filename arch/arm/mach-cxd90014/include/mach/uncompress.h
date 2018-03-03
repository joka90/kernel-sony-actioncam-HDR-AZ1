/*
 * mach-cxd90014/include/mach/uncompress.h
 *
 * uncompress utility functions
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/uncompress.h.
 */
/*
 *  linux/include/asm-arm/arch-integrator/uncompress.h
 *
 *  Copyright (C) 1999 ARM Limited
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

#include <asm/sizes.h>
#include <mach/platform.h>
/* can not include <asm/io.h> here. */
#define readl(a)    (*(volatile u32 __force *)(a))
#define writel(v,a) (*(volatile u32 __force *)(a) = (v))

#define __ASSEMBLY__
#include <linux/amba/serial.h>
#undef __ASSEMBLY__

#define AMBA_UART_DR	(CXD90014_UART(0)+UART01x_DR)
#define AMBA_UART_FR	(CXD90014_UART(0)+UART01x_FR)
#define AMBA_UART_IBRD	(CXD90014_UART(0)+UART011_IBRD)
#define AMBA_UART_FBRD	(CXD90014_UART(0)+UART011_FBRD)
#define AMBA_UART_LCRH	(CXD90014_UART(0)+UART011_LCRH)
#define AMBA_UART_CR	(CXD90014_UART(0)+UART011_CR)

/*
 * This does not append a newline
 */
static void putc(int c)
{
	while (readl(AMBA_UART_FR) & UART01x_FR_TXFF)
		barrier();

	writel(c, AMBA_UART_DR);
}

static void flush(void)
{
	while (readl(AMBA_UART_FR) & UART01x_FR_BUSY)
		barrier();
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
