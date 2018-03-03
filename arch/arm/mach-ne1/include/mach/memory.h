/* 2010-11-30: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/memory.h
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/memory.h
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

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H


/*
 * Physical DRAM offset.
 */
#define PHYS_OFFSET		UL(0x80000000)


#ifdef CONFIG_SPARSEMEM
#define SECTION_SIZE_BITS	CONFIG_SNSC_SPARSE_SECTION_SHIFT
#define MAX_PHYSADDR_BITS	32
#define MAX_PHYSMEM_BITS	32
#define MAX_HOTPLUG_MEM        256 * 1024 * 1024
#endif

#endif /* __ASM_ARCH_MEMORY_H */
