/*
 * include/asm-arm/arch-cxd4132/hardware.h
 *
 * I/O space definitions
 *
 * Copyright 2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/hardware.h.
 */
/*
 *  linux/include/asm-arm/arch-integrator/hardware.h
 *
 *  This file contains the hardware definitions of the Integrator.
 *
 *  Copyright (C) 1999 ARM Limited.
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
#ifndef __ARCH_CXD4132_HARDWARE_H
#define __ARCH_CXD4132_HARDWARE_H

#include <asm/sizes.h>
#include <mach/platform.h>

/*
 * Where in virtual memory the IO devices (timers, system controllers
 * and so on)
 */
#define PCIO_BASE		0

/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x)	(x)
#ifdef __ASSEMBLY__
#define IO_ADDRESSP(x)	IO_ADDRESS(x)
#else
#define IO_ADDRESSP(x)	(const void __iomem __force *)IO_ADDRESS(x)
#endif

/* obsolete */
#define HIO_ADDRESS(x)	(x)
#ifdef __ASSEMBLY__
#define HIO_ADDRESSP(x)	HIO_ADDRESS(x)
#else
#define HIO_ADDRESSP(x)	(const void __iomem __force *)HIO_ADDRESS(x)
#endif

#endif /* __ARCH_CXD4132_HARDWARE_H */
