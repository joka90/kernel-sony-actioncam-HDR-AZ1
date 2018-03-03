/* 2011-02-22: File added by Sony Corporation */
/*
 * Declarations for power state transition code
 *
 * Copyright 2011 Sony Corporation
 *
 * This code is implemented based on following code:
 */
/*
 * arch/arm/mach-tegra/power.h
 *
 * Declarations for power state transition code
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MACH_EMXX_POWER_H
#define __MACH_EMXX_POWER_H

#include <asm/page.h>

/* CPU Context area (1KB per CPU) */
#define CONTEXT_SIZE_BYTES_SHIFT 10
#define CONTEXT_SIZE_BYTES (1<<CONTEXT_SIZE_BYTES_SHIFT)

#endif /* __MACH_EMXX_POWER_H */
