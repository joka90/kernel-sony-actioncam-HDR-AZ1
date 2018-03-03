/* 2011-03-07: File added and changed by Sony Corporation */
/*
 *  File Name       : arch/arm/mach-emxx/include/mach/memory.h
 *  Function        : memory
 *  Release Version : Ver 1.01
 *  Release Date    : 2010/09/01
 *
 * Copyright (C) 2010 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * Physical DRAM offset.
 */
#define PHYS_OFFSET	UL(0x40000000)

#ifdef CONFIG_MACH_EMEV
#define NODE_MEM_SIZE_BITS	29	/* 512M */
#else
#define NODE_MEM_SIZE_BITS	26	/* 64M */
#endif

#ifdef CONFIG_MACH_KZMA9
#define NODE_MEM_SIZE_BITS	28	/* 256M */
#endif

#ifdef CONFIG_SPARSEMEM
#define SECTION_SIZE_BITS	CONFIG_SNSC_SPARSE_SECTION_SHIFT
#define MAX_PHYSADDR_BITS	32
#define MAX_PHYSMEM_BITS	32
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
/*
 * EMEV is able to install 512MB memory. But current memory map cannot handle it.
 * Please see "arch/arm/mach-emxx/include/mach/vmalloc.h"
 */
#define MAX_HOTPLUG_MEM		(512 * 1024 * 1024)
#endif

#endif	/* __ASM_ARCH_MEMORY_H */
