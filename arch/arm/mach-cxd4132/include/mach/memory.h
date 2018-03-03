/*
 * include/asm-arm/arch-cxd4132/memory.h
 *
 * memory address definitions
 *
 * Copyright 2009,2010 Sony Corporation
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
#ifndef __ARCH_CXD4132_MEMORY_H
#define __ARCH_CXD4132_MEMORY_H
#include <mach/hardware.h>

/*
 * Virtual    <---> Physical
 * 0xc0000000       0x80000000
 */
#ifdef CONFIG_CXD4132_DDR_RANK0_ABSENT
#define PAGE_OFFSET		UL(0xd0000000)
#else
#define PAGE_OFFSET		UL(0xc0000000)
#endif
#define TASK_SIZE		(UL(CONFIG_PAGE_OFFSET) - UL(0x01000000))
#define TASK_UNMAPPED_BASE	UL(CONFIG_PAGE_OFFSET / 2)

#define MODULES_VADDR		UL(CONFIG_MODULES_OFFSET)
#define MODULES_END		(MODULES_VADDR + SZ_16M)

#define CXD4132_PHYS_OFFSET	DDR_BASE
#ifdef __ASSEMBLY__
#define PHYS_OFFSET		(CXD4132_PHYS_OFFSET)
#else
#define PHYS_OFFSET		UL(CXD4132_PHYS_OFFSET)
#endif
#define BUS_OFFSET		PHYS_OFFSET
/* For fail-safe, accept both 0x80000000 and 0xc0000000 as input. */
#define CXD4132_VIRT_BIT	UL(0x40000000)
#define __virt_to_phys(x)	((x) & ~CXD4132_VIRT_BIT)
#define __phys_to_virt(x)	((x) | CXD4132_VIRT_BIT)

#define ARM_DMA_ZONE_SIZE	SZ_512M

#if defined(CONFIG_SPARSEMEM)

#define SECTION_SIZE_BITS	CONFIG_SNSC_SPARSE_SECTION_SHIFT
#define MAX_PHYSADDR_BITS	32
#define MAX_PHYSMEM_BITS	32
#define MAX_HOTPLUG_MEM		(512 * SZ_1M)

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

#endif /* __ARCH_CXD4132_MEMORY_H */
