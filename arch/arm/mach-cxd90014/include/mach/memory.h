/*
 * include/asm-arm/arch-cxd90014/memory.h
 *
 * memory address definitions
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/memory.h.
 */
/*
 *  linux/include/asm-arm/arch-integrator/mmu.h
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
#ifndef __ARCH_CXD90014_MEMORY_H
#define __ARCH_CXD90014_MEMORY_H
#include <mach/hardware.h>
#include <mach/vectors-layout.h>

#define PAGE_OFFSET		UL(CONFIG_PAGE_OFFSET)
#define TASK_SIZE		(UL(CONFIG_PAGE_OFFSET) - UL(0x01000000))
#if CONFIG_TASK_MMAP_OFFSET == 0
#define TASK_UNMAPPED_BASE	UL(CONFIG_PAGE_OFFSET / 2)
#else
#define TASK_UNMAPPED_BASE	UL(CONFIG_TASK_MMAP_OFFSET)
#endif

#ifdef __ASSEMBLY__
#define PLAT_PHYS_OFFSET	(DDR_BASE)
#else
#define PLAT_PHYS_OFFSET	UL(DDR_BASE)
#endif
#define BUS_OFFSET		PLAT_PHYS_OFFSET

#ifdef CONFIG_CXD90014_DDR512MB
#define ARM_DMA_ZONE_SIZE	SZ_512M
#else
#define ARM_DMA_ZONE_SIZE	SZ_1G
#endif /* CONFIG_CXD90014_DDR512MB */

#if defined(CONFIG_SPARSEMEM)

#define SECTION_SIZE_BITS	CONFIG_SNSC_SPARSE_SECTION_SHIFT
#define MAX_PHYSADDR_BITS	32
#define MAX_PHYSMEM_BITS	32
#ifdef CONFIG_CXD90014_DDR512MB
#define MAX_HOTPLUG_MEM		SZ_512M
#else
#define MAX_HOTPLUG_MEM		SZ_1G
#endif /* CONFIG_CXD90014_DDR512MB */

#endif /* CONFIG_SPARSEMEM */

#if defined(CONFIG_NUMA)
#define PFN_TO_NID(pfn)         pfn_to_nid(pfn)

#define pcibus_to_node(bus)	((void)(bus), -1)
#define pcibus_to_cpumask(bus)	(pcibus_to_node(bus) == -1 ? \
					CPU_MASK_ALL : \
					node_to_cpumask(pcibus_to_node(bus)) \
				)
#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))

#else
#define PFN_TO_NID(addr)	(0)
#endif /* CONFIG_NUMA */

#endif /* __ARCH_CXD90014_MEMORY_H */
