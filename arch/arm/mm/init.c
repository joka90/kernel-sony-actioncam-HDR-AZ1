/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/arch/arm/mm/init.c
 *
 *  Copyright (C) 1995-2005 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/swap.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/initrd.h>
#include <linux/of_fdt.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot.h>
#endif
#include <linux/memblock.h>
#include <linux/sort.h>

#ifdef CONFIG_MEMORY_HOTPLUG
#include <mach/hardware.h>
#endif
#include <asm/mach-types.h>
#include <asm/prom.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/tlb.h>
#include <asm/fixmap.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
#include <asm/memory.h>
#endif

#ifdef CONFIG_WARM_BOOT_IMAGE
#include <asm/mach/warmboot.h>
#endif

#include "mm.h"

static unsigned long phys_initrd_start __initdata = 0;
static unsigned long phys_initrd_size __initdata = 0;
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
static unsigned long type_specified_area_nr;
static unsigned long mapped_type_specified_area_nr;
void reserve_mem_memblock_reserve(void);
#ifdef CONFIG_SNSC_SSBOOT
static unsigned long ssboot_discard_specified_area_nr;
static unsigned long registered_ssboot_discard_specified_area_nr;
#endif
static struct {
	unsigned int type;
#ifdef CONFIG_SNSC_SSBOOT
	unsigned int ssboot_discard;
#endif
} reserved_mem_arm[MAX_RESERVE_AREAS];
#endif /* CONFIG_SNSC_MEMORY_RESERVE_SUPPORT */

static int __init early_initrd(char *p)
{
	unsigned long start, size;
	char *endp;

	start = memparse(p, &endp);
	if (*endp == ',') {
		size = memparse(endp + 1, NULL);

		phys_initrd_start = start;
		phys_initrd_size = size;
	}
	return 0;
}
early_param("initrd", early_initrd);

static int __init parse_tag_initrd(const struct tag *tag)
{
	printk(KERN_WARNING "ATAG_INITRD is deprecated; "
		"please update your bootloader.\n");
	phys_initrd_start = __virt_to_phys(tag->u.initrd.start);
	phys_initrd_size = tag->u.initrd.size;
	return 0;
}

__tagtable(ATAG_INITRD, parse_tag_initrd);

static int __init parse_tag_initrd2(const struct tag *tag)
{
	phys_initrd_start = tag->u.initrd.start;
	phys_initrd_size = tag->u.initrd.size;
	return 0;
}

__tagtable(ATAG_INITRD2, parse_tag_initrd2);

#ifdef CONFIG_OF_FLATTREE
void __init early_init_dt_setup_initrd_arch(unsigned long start, unsigned long end)
{
	phys_initrd_start = start;
	phys_initrd_size = end - start;
}
#endif /* CONFIG_OF_FLATTREE */

#ifdef CONFIG_SNSC_PAGES_ACCOUNTING
int is_free_initmem_running = 0;
#endif

/*
 * This keeps memory configuration data used by a couple memory
 * initialization functions, as well as show_mem() for the skipping
 * of holes in the memory map.  It is populated by arm_add_memory().
 */
struct meminfo meminfo;

#ifdef CONFIG_SNSC_GROUP_SPARSEMEM_SECTIONS
int __init is_range_overlap_with_meminfo(unsigned long start, unsigned long end)
{
	int i;
	for (i = 0; i < meminfo.nr_banks; i++) {
		if (end <= meminfo.bank[i].start ||
		    start >= (meminfo.bank[i].start + meminfo.bank[i].size))
			continue;
		else
			return 1;
	}
	return 0;
}
#endif

void show_mem(unsigned int filter)
{
	int free = 0, total = 0, reserved = 0;
	int shared = 0, cached = 0, slab = 0, node, i;
	struct meminfo * mi = &meminfo;

	printk("Mem-info:\n");
	show_free_areas(filter);
	for_each_online_node(node) {
		for_each_nodebank (i,mi,node) {
			struct membank *bank = &mi->bank[i];
			unsigned int pfn1, pfn2, pfn;
			struct page *page, *end;

			pfn1 = bank_pfn_start(bank);
			pfn2 = bank_pfn_end(bank);
			pfn = pfn1;

			page = pfn_to_page(pfn1);
			end  = pfn_to_page(pfn2 - 1) + 1;

			while(pfn < pfn2){
				BUG_ON(!pfn_valid(pfn));
				page = pfn_to_page(pfn);

				total++;
				if (PageReserved(page))
					reserved++;
				else if (PageSwapCache(page))
					cached++;
				else if (PageSlab(page))
					slab++;
				else if (!page_count(page))
					free++;
				else
					shared += page_count(page) - 1;

				pfn = page_to_pfn(page);
				pfn++;
			}
		}
	}

	printk("%d pages of RAM\n", total);
	printk("%d free pages\n", free);
	printk("%d reserved pages\n", reserved);
	printk("%d slab pages\n", slab);
	printk("%d pages shared\n", shared);
	printk("%d pages swap cached\n", cached);
}

static void __init find_node_limits(int node, struct meminfo *mi, 
	unsigned long *min, unsigned long *max_low, unsigned long *max_high)
{
	int i;

	*min = -1UL;
	*max_low = *max_high = 0;

	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];
		unsigned long start, end;

		start = bank_pfn_start(bank);
		end = bank_pfn_end(bank);

		if (*min > start)
			*min = start;
		if (*max_high < end)
			*max_high = end;
		if (bank->highmem)
			continue;
		if (*max_low < end)
			*max_low = end;
	}
}

/*
 * FIXME: We really want to avoid allocating the bootmap bitmap
 * over the top of the initrd.  Hopefully, this is located towards
 * the start of a bank, so if we allocate the bootmap bitmap at
 * the end, we won't clash.
 */
static unsigned int __init
find_bootmap_pfn(int node, struct meminfo *mi, unsigned int bootmap_pages)
{
	unsigned int start_pfn, i, bootmap_pfn;

	start_pfn   = PAGE_ALIGN(__pa(_end)) >> PAGE_SHIFT;
	bootmap_pfn = 0;

	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];
		unsigned int start, end;

		start = bank_pfn_start(bank);
		end   = bank_pfn_end(bank);

		if (end < start_pfn)
			continue;

		if (start < start_pfn)
			start = start_pfn;

		if (end <= start)
			continue;

		if (end - start >= bootmap_pages) {
			bootmap_pfn = start;
			break;
		}
	}

	if (bootmap_pfn == 0)
		BUG();

	return bootmap_pfn;
}

static int __init check_initrd(struct meminfo *mi)
{
	int initrd_node = -2;
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long end = phys_initrd_start + phys_initrd_size;

	/*
	 * Make sure that the initrd is within a valid area of
	 * memory.
	 */
	if (phys_initrd_size) {
		unsigned int i;

		initrd_node = -1;

		for (i = 0; i < mi->nr_banks; i++) {
			struct membank *bank = &mi->bank[i];
			if (bank_phys_start(bank) <= phys_initrd_start &&
			    end <= bank_phys_end(bank))
				initrd_node = bank->node;
		}
	}

	if (initrd_node == -1) {
		printk(KERN_ERR "INITRD: 0x%08lx+0x%08lx extends beyond "
		       "physical memory - disabling initrd\n",
		       phys_initrd_start, phys_initrd_size);
		phys_initrd_start = phys_initrd_size = 0;
	}
#endif

	return initrd_node;
}

static void __init bootmem_init_node(int node, struct meminfo *mi,
	unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned int boot_pfn;
	unsigned int boot_pages;
	pg_data_t *pgdat;
	int i;

	/*
	 * Allocate the bootmem bitmap page.  This must be in a region
	 * of memory which has already been mapped.
	 */
	boot_pages = bootmem_bootmap_pages(end_pfn - start_pfn);
#ifdef CONFIG_NODEZERO_ALLOCATION
	if (node > 0) {
		boot_pfn = __phys_to_pfn(virt_to_phys(alloc_bootmem_low_pages_node(NODE_DATA(0), boot_pages << PAGE_SHIFT)));
	} else
#endif /* CONFIG_NODEZERO_ALLOCATION */
	boot_pfn = find_bootmap_pfn(node, mi, boot_pages);

	/*
	 * Initialise the bootmem allocator for this node, handing the
	 * memory banks over to bootmem.
	 */
	node_set_online(node);
	pgdat = NODE_DATA(node);
	init_bootmem_node(pgdat, boot_pfn, start_pfn, end_pfn);

	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];
		if (!bank->highmem)
			free_bootmem_node(pgdat, bank_phys_start(bank), bank_phys_size(bank));
	}

#ifdef CONFIG_ARCH_POPULATES_NODE_MAP
	if (-1UL != start_pfn)
		add_active_range(node, start_pfn, end_pfn);
#endif

	/*
	 * Reserve the bootmem bitmap for this node.
	 */
#ifdef CONFIG_NODEZERO_ALLOCATION
	if (node == 0)
#endif
	reserve_bootmem_node(pgdat, boot_pfn << PAGE_SHIFT,
			     boot_pages << PAGE_SHIFT, BOOTMEM_DEFAULT);
}

#ifdef CONFIG_ZONE_DMA
unsigned long arm_dma_zone_size __read_mostly;
EXPORT_SYMBOL(arm_dma_zone_size);

/*
 * The DMA mask corresponding to the maximum bus address allocatable
 * using GFP_DMA.  The default here places no restriction on DMA
 * allocations.  This must be the smallest DMA mask in the system,
 * so a successful GFP_DMA allocation will always satisfy this.
 */
u32 arm_dma_limit;

static void __init arm_adjust_dma_zone(unsigned long *size, unsigned long *hole,
	unsigned long dma_size)
{
	if (size[0] <= dma_size)
		return;

	size[ZONE_NORMAL] = size[0] - dma_size;
	size[ZONE_DMA] = dma_size;
	hole[ZONE_NORMAL] = hole[0];
	hole[ZONE_DMA] = 0;
}
#endif

static void __init bootmem_reserve_initrd(int node)
{
#ifdef CONFIG_BLK_DEV_INITRD
	pg_data_t *pgdat = NODE_DATA(node);
	int res;

	res = reserve_bootmem_node(pgdat, phys_initrd_start,
			phys_initrd_size, BOOTMEM_EXCLUSIVE);

	if (res == 0) {
		initrd_start = __phys_to_virt(phys_initrd_start);
		initrd_end = initrd_start + phys_initrd_size;
	} else {
		printk(KERN_ERR
			"INITRD: 0x%08lx+0x%08lx overlaps in-use "
			"memory region - disabling initrd\n",
			phys_initrd_start, phys_initrd_size);
	}
#endif
}

static void __init bootmem_free_node(int node, struct meminfo *mi)
{
	unsigned long zone_size[MAX_NR_ZONES], zhole_size[MAX_NR_ZONES];
	unsigned long min, max_low, max_high;
	int i;

	find_node_limits(node, mi, &min, &max_low, &max_high);

	/*
	 * initialise the zones within this node.
	 */
	memset(zone_size, 0, sizeof(zone_size));

	/*
	 * The size of this node has already been determined.  If we need
	 * to do anything fancy with the allocation of this memory to the
	 * zones, now is the time to do it.
	 */
	zone_size[0] = max_low - min;
#ifdef CONFIG_HIGHMEM
	zone_size[ZONE_HIGHMEM] = max_high - max_low;
#endif

	/*
	 * For each bank in this node, calculate the size of the holes.
	 *  holes = node_size - sum(bank_sizes_in_node)
	 */
	memcpy(zhole_size, zone_size, sizeof(zhole_size));
	for_each_nodebank(i, mi, node) {
		int idx = 0;
#ifdef CONFIG_HIGHMEM
		if (mi->bank[i].highmem)
			idx = ZONE_HIGHMEM;
#endif
		zhole_size[idx] -= bank_pfn_size(&mi->bank[i]);
	}

#ifdef CONFIG_ZONE_DMA
	/*
	 * Adjust the sizes according to any special requirements for
	 * this machine type.
	 */
	if (arm_dma_zone_size) {
		arm_adjust_dma_zone(zone_size, zhole_size,
			arm_dma_zone_size >> PAGE_SHIFT);
		arm_dma_limit = PHYS_OFFSET + arm_dma_zone_size - 1;
	} else
		arm_dma_limit = 0xffffffff;
#endif

	free_area_init_node(node, zone_size, min, zhole_size);
}

#ifdef CONFIG_HAVE_ARCH_PFN_VALID
int pfn_valid(unsigned long pfn)
{
	return memblock_is_memory(pfn << PAGE_SHIFT);
}
EXPORT_SYMBOL(pfn_valid);
#endif

#ifndef CONFIG_SPARSEMEM
static void arm_memory_present(struct meminfo *mi, int node)
{
}
#else
static void arm_memory_present(struct meminfo *mi, int node)
{
	int i;
	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];
		memory_present(node, bank_pfn_start(bank), bank_pfn_end(bank));
	}
}
#endif

static int __init meminfo_cmp(const void *_a, const void *_b)
{
	const struct membank *a = _a, *b = _b;
	long cmp = bank_pfn_start(a) - bank_pfn_start(b);
	return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}

void __init arm_memblock_init(struct meminfo *mi, struct machine_desc *mdesc)
{
	int i;

	sort(&meminfo.bank, meminfo.nr_banks, sizeof(meminfo.bank[0]), meminfo_cmp, NULL);

	memblock_init();
	for (i = 0; i < mi->nr_banks; i++)
		memblock_add(mi->bank[i].start, mi->bank[i].size);

	arm_dt_memblock_reserve();
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	reserve_mem_memblock_reserve();
#endif

	/* reserve any platform specific memblock areas */
	if (mdesc->reserve)
		mdesc->reserve();

	memblock_analyze();
	memblock_dump_all();
}

#ifdef CONFIG_SNSC_SUPPORT_4KB_MAPPING
static void __init remap_memory_bank(struct membank *bank)
{
	struct map_desc map;

	map.pfn = bank_pfn_start(bank);
	map.virtual = __phys_to_virt(bank_phys_start(bank));
	map.length = bank_phys_size(bank);
	map.type = MT_MEMORY;

	change_mapping(&map);
}

static void __init remap_node(int node, struct meminfo *mi)
{
	int i;

	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];

		remap_memory_bank(bank);
	}
}
#endif

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
void reserve_mem_memblock_reserve(void)
{

	unsigned long i;
	phys_addr_t start, size;

	for (i = 0; i < reserve_mem_nr_map; i++) {
		start = reserved_mem[i].start;
		size = reserved_mem[i].size;
		if(memblock_reserve(start,size))
			panic("memblock: unable to reserve area: 0x%lx@0x%lx. Stop booting.\n",
				(unsigned long)size, (unsigned long)start);
	}

}
static __init void reserve_mem(int nid, pg_data_t *pgdat)
{
	unsigned long index;
	unsigned long start, size;
	unsigned long node_start, node_end;
	unsigned int type;
#ifdef CONFIG_SNSC_SSBOOT
	unsigned int ssboot_discard;
#endif
	struct map_desc md;

	node_start = (pgdat->bdata->node_min_pfn) << PAGE_SHIFT;
	node_end   = (pgdat->bdata->node_low_pfn) << PAGE_SHIFT;

	for (index = 0; index < reserve_mem_nr_map; index++) {
		start = reserved_mem[index].start;
		size = reserved_mem[index].size;
		type = reserved_mem_arm[index].type;
#ifdef CONFIG_SNSC_SSBOOT
		ssboot_discard = reserved_mem_arm[index].ssboot_discard;
#endif

		/* Reserve area is not in this node */
		if (start + size <= node_start || start >= node_end)
			continue;

		if ((start < node_start && start + size > node_start)
		    || (start < node_end && start + size > node_end)) {
			panic("reserve area: 0x%lx@0x%lx crossed node%d boundary. Stop booting.\n",
			      size, start, nid);
		}

		reserve_bootmem_node(pgdat, start, size, BOOTMEM_DEFAULT);

		if (type != MT_MEMORY) {
			if((start & ~SECTION_MASK) || (size & (SECTION_SIZE-1)))
				panic("reserve area: device area is not aligned with section size.\n");
			md.virtual = __phys_to_virt(start);
			md.pfn = start >> PAGE_SHIFT;
			md.length = size;
			md.type = type;
			create_mapping(&md);
			mapped_type_specified_area_nr++;
		}

#ifdef CONFIG_SNSC_SSBOOT
		if (ssboot_discard) {
			if (ssboot_region_register_discard(start, size) < 0)
				panic("reserve area: failed to register discard region.\n");
			registered_ssboot_discard_specified_area_nr++;
		}
#endif
	}
}

static __init int parse_memrsv(char *p)
{
	int i;
	unsigned long size,start;
	unsigned int type;
#ifdef CONFIG_SNSC_SSBOOT
	unsigned int ssboot_discard;
#endif
	char *endp;

	size = start = 0;
	size = memparse(p, &endp);
	if(*endp == '@' || *endp == '$' || *endp == '#'){
		start = memparse(endp + 1, &endp);
	}
	p = endp;

	type = MT_MEMORY;
#ifdef CONFIG_SNSC_SSBOOT
	ssboot_discard = 0;
#endif
	if(*p == ':') {
		++p;
	try_next_option:
		if ( 0 == strncmp(p, "device", i = strlen("device"))) {
			type = MT_DEVICE;
		} else if ( 0 == strncmp(p, "ssboot=discard",
					 i = strlen("ssboot=discard"))) {
#ifdef CONFIG_SNSC_SSBOOT
			ssboot_discard = 1;
#else
			printk(KERN_WARNING
			       "memrsv: \"ssboot=discard\" is ignored.\n");
#endif
		} else {
			printk(KERN_ERR "memrsv: memory attribute is invalid.\n");
			return -EINVAL;
		}
		p += i;
		if (*p == ',') {
			++p;
			goto try_next_option;
		}
	}
	if (*p != ' ' && *p != '\0') {
		printk(KERN_ERR "memrsv: parameter format is incorrect.\n");
		return -EINVAL;
	}

	if(size && start){
		if(reserve_mem_nr_map == MAX_RESERVE_AREAS){
			printk(KERN_ERR "too many reserved areas(max %d)!\n", MAX_RESERVE_AREAS);
			return -EINVAL;
		}

		printk("reserve area %lu: %lX @ %lX\n",reserve_mem_nr_map, size, start);
		reserved_mem[reserve_mem_nr_map].start = start;
		reserved_mem[reserve_mem_nr_map].size = size;
		reserved_mem_arm[reserve_mem_nr_map].type = type;
#ifdef CONFIG_SNSC_SSBOOT
		reserved_mem_arm[reserve_mem_nr_map].ssboot_discard = ssboot_discard;
#endif
		reserve_mem_nr_map++;
		if (type != MT_MEMORY)
			type_specified_area_nr++;
#ifdef CONFIG_SNSC_SSBOOT
		if (ssboot_discard)
			ssboot_discard_specified_area_nr++;
#endif
	}
	return 0;
}
#ifdef CONFIG_SNSC_USE_MEMMAP_AS_MEMRSV
early_param("memmap", parse_memrsv);
#else
early_param("memrsv", parse_memrsv);
#endif
#endif /* CONFIG_SNSC_MEMORY_RESERVE_SUPPORT */


#ifdef CONFIG_CXD90014_DDR_C2UNC
/*----------- for unified memory map --------------------*/
#ifndef MAX_C2UNC_AREAS
# define MAX_C2UNC_AREAS 8
#endif
struct c2unc_mem {
	unsigned long start;
	unsigned long size;
};

static struct c2unc_mem c2unc_mem[MAX_C2UNC_AREAS];
static unsigned long c2unc_mem_nr_map;

static __init int parse_mem_c2unc(char *p)
{
	unsigned long size,start;
	char *endp;

	size = start = 0;
	size = memparse(p, &endp);
	if(*endp == '@' || *endp == '$' || *endp == '#'){
		start = memparse(endp + 1, &endp);
	}
	p = endp;

	if(size && start){
		if(MAX_C2UNC_AREAS == c2unc_mem_nr_map){
			printk(KERN_ERR "ERROR: too many c2unc areas(max %d)!\n", MAX_C2UNC_AREAS);
			return -EINVAL;
		}
		if ((start | size) & ~PAGE_MASK) {
			printk(KERN_ERR "ERROR:mem_c2unc: not page aligned: %lX @ %lX\n", size, start);
			return -EINVAL;
		}

		printk(KERN_INFO "mem_c2unc: area %lu: %lX @ %lX\n",c2unc_mem_nr_map, size, start);
		c2unc_mem[c2unc_mem_nr_map].start = start;
		c2unc_mem[c2unc_mem_nr_map++].size = size;
	}
	return 0;
}
early_param("mem_c2unc", parse_mem_c2unc);
#endif

#ifdef CONFIG_SUBSYSTEM
unsigned long subsys_phys, subsys_virt, subsys_offs, subsys_size;

static __init int parse_mem_subsys(char *p)
{
	unsigned long size, phys, virt;
	char *endp;

	size = phys = virt = 0;
	size = memparse(p, &endp);
	if (*endp == '@' || *endp == '$' || *endp == '#') {
		phys = memparse(endp + 1, &endp);
	}
	if (*endp == ':') {
		virt = memparse(endp + 1, &endp);
	}
	p = endp;

	if (size && phys && virt) {
		if ((phys | virt | size) & ~PAGE_MASK) {
			printk(KERN_ERR "ERROR:mem_subsys: not page aligned: %lX @ %lX : %lX\n", size, phys, virt);
			return -EINVAL;
		}

		subsys_virt = virt;
		subsys_phys = phys;
		subsys_offs = virt - phys;
		subsys_size = size;
		printk("subsys: %lX @ %lX : %lX\n", size, phys, virt);
	}
	return 0;
}
early_param("mem_subsys", parse_mem_subsys);

static void map_subsys(void)
{
	struct map_desc map;
	unsigned long virt, phys, end, size;

	if (!subsys_virt)
		return;

	virt = subsys_virt & SECTION_MASK;
	end = (subsys_virt + subsys_size + (SECTION_SIZE - 1)) & SECTION_MASK;
	size = end - virt;
	phys = subsys_phys & SECTION_MASK;
	printk(KERN_INFO "map_subsys: virt=%lX,size=%lX,phys=%lX\n", virt, size, phys);

	map.virtual = virt;
	map.pfn     = PFN_DOWN(phys);
	map.length  = size;
	map.type    = MT_MEMORY;
	create_mapping(&map);
}
#endif /* CONFIG_SUBSYSTEM */

#ifdef CONFIG_CXD90014_DDR_C2UNC
/*
 * ddr_c2unc()
 *   Change cache attribute from cached to uncached.
 */
static void __init __ddr_c2unc(unsigned long addr, unsigned long size)
{
	struct map_desc map;

	map.virtual = __phys_to_virt(addr);
	map.pfn     = __phys_to_pfn(addr);
	map.length  = size;
#ifdef CONFIG_CXD4132_USE_MEMORY_UC
	map.type    = MT_MEMORY_UC;
#else
	map.type    = MT_DEVICE;
#endif

	if (PAGE_SIZE == size) {
#ifdef CONFIG_SNSC_SUPPORT_4KB_MAPPING
		change_sect_into_4k(&map);
#else
		printk(KERN_ERR "ERROR:%s:can not use 4K mapping\n", __func__);
		return;
#endif
	} else if (SECTION_SIZE == size) {
		change_super_into_sect(&map);
	}
	create_mapping(&map);
}

static void __init ddr_c2unc(void)
{
	unsigned long index, addr, rest, size;

	for ( index = 0; index < c2unc_mem_nr_map; index++ ) {
		addr = c2unc_mem[index].start;
		rest = c2unc_mem[index].size;

		while (rest > 0) {
			if (!(addr & ~SUPERSECTION_MASK)
			    && (rest >= SUPERSECTION_SIZE)) {
				size = SUPERSECTION_SIZE;
			} else if (!(addr & ~SECTION_MASK)
				   && rest >= SECTION_SIZE) {
				size = SECTION_SIZE;
			} else {
				size = PAGE_SIZE;
			}
			__ddr_c2unc(addr, size);
			addr += size;
			rest -= size;
		}
	}
}
/*-------------------------------------------------------*/
#endif

#ifdef CONFIG_EJ_CLEAR_MEMORY
static void __init bootmem_clear_node(pg_data_t *pgdat)
{
	bootmem_data_t *bdata = pgdat->bdata;
	unsigned long n, sidx, eidx, phys, len;

	if (!bdata->node_bootmem_map)
		return;

	n = bdata->node_low_pfn - bdata->node_min_pfn;
	for (sidx = 0; sidx < n; sidx = eidx + 1) {
		sidx = find_next_zero_bit(bdata->node_bootmem_map, n, sidx);
		if (sidx == n)
			break;
		eidx = find_next_bit(bdata->node_bootmem_map, n, sidx);
		phys = __pfn_to_phys(bdata->node_min_pfn + sidx);
		len = (eidx - sidx) << PAGE_SHIFT;
		printk(KERN_INFO "memclr: 0x%lx-0x%lx\n", phys, phys+len-1);
		memset(__va(phys), 0, len);
	}
}

static void __init bootmem_clear(void)
{
	int node;

#ifdef CONFIG_WARM_BOOT_IMAGE
	if (!wbi_memclr())
		return;
#endif /* CONFIG_WARM_BOOT_IMAGE */

	for_each_node(node) {
		bootmem_clear_node(NODE_DATA(node));
	}
}
#endif /* CONFIG_EJ_CLEAR_MEMORY */

void __init bootmem_init(void)
{
	struct meminfo *mi = &meminfo;
	unsigned long min, max_low, max_high;
	int node, initrd_node;
#ifdef CONFIG_ARCH_POPULATES_NODE_MAP
	unsigned long max_zone_pfns[MAX_NR_ZONES];
#endif
	struct memblock_region * reg;

	/*
	 * Locate which node contains the ramdisk image, if any.
	 */
	initrd_node = check_initrd(mi);

	max_low = max_high = 0;

	/*
	 * Run through each node initialising the bootmem allocator.
	 */
	for_each_node(node) {
		unsigned long node_low, node_high;

		find_node_limits(node, mi, &min, &node_low, &node_high);

		if (node_low > max_low)
			max_low = node_low;
		if (node_high > max_high)
			max_high = node_high;

		/*
		 * If there is no memory in this node, ignore it.
		 * (We can't have nodes which have no lowmem)
		 */
		if (node_low == 0)
			continue;

		bootmem_init_node(node, mi, min, node_low);

		/*
		 * Reserve any special node zero regions.
		 */
		if (node == 0)
			reserve_node_zero(NODE_DATA(node));

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
		reserve_mem(node, NODE_DATA(node));
#endif
		/*
		 * If the initrd is in this node, reserve its memory.
		 */
		if (node == initrd_node)
			bootmem_reserve_initrd(node);

		/*
		 * Sparsemem tries to allocate bootmem in memory_present(),
		 * so must be done after the fixed reservations
		 */
		arm_memory_present(mi, node);
	}

#if defined(CONFIG_ARCH_CXD90014BASED)
	printk("bootmem_init: max_low=0x%lu, max_high=0x%lu\n", max_low, max_high);
#endif
	/*
	 * Each chunk of allocated memblock
	 */
	for_each_memblock(reserved, reg) {
		unsigned long start = memblock_region_reserved_base_pfn(reg);
		unsigned long end = memblock_region_reserved_end_pfn(reg);
		unsigned long cur_start, cur_end, cur_size;
		unsigned long cur_min, cur_node_low, cur_node_high;
		pg_data_t *pgdat;
		int cur_node ;

		if (start >= end)
			break;

		/*
		 * For each chuck of allocated memblock we have to segregate it
		 * into corresponding nodes before passing the memblock chunks to
		 * the bootmem layer.
		 */
		for(cur_start = start; end > cur_start; cur_start += cur_size){
			if( (cur_node = __early_pfn_to_nid(cur_start)) == -1 ){
				cur_size = 1;
				continue;
			}

			pgdat = NODE_DATA(cur_node);

			find_node_limits(cur_node, mi, &cur_min,&cur_node_low,
				&cur_node_high);

			cur_end = (end > cur_node_low) ? cur_node_low : end;
			cur_size = cur_end - cur_start;
			reserve_bootmem_node(pgdat, __pfn_to_phys(cur_start),
				cur_size << PAGE_SHIFT, BOOTMEM_DEFAULT);
		}
	}

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	if (mapped_type_specified_area_nr != type_specified_area_nr)
		panic("memrsv: device attribute area is in secgrp.\n");

#ifdef CONFIG_SNSC_SSBOOT
	if (registered_ssboot_discard_specified_area_nr !=
					ssboot_discard_specified_area_nr)
		panic("memrsv: \"ssboot=discard\" area is in secgrp.\n");
#endif
#endif

#ifdef CONFIG_SUBSYSTEM
	map_subsys();
#endif /* CONFIG_SUBSYSTEM */

#ifdef CONFIG_EJ_CLEAR_MEMORY
	bootmem_clear();
#endif /* CONFIG_EJ_CLEAR_MEMORY */

#ifdef CONFIG_CXD90014_DDR_C2UNC
	/* change cached region to uncached */
	ddr_c2unc();
#endif

	/*
	 * sparse_init() needs the bootmem allocator up and running.
	 */
	sparse_init();

	/*
	 * Now free memory in each node - free_area_init_node needs
	 * the sparse mem_map arrays initialized by sparse_init()
	 * for memmap_init_zone(), otherwise all PFNs are invalid.
	 */
	for_each_node(node)
		bootmem_free_node(node, mi);

#ifdef CONFIG_MEMORY_HOTPLUG
	/*
 	 * to prevent vmalloc() allocate memory whose virt addr
 	 * occupies the address space which will be used by hotplugged
 	 * memory, we have to reserve the virt addr space for the max
 	 * amount of memory that may be hotplugged.
 	 */
	/* max amount of memory after all possible hotplug */
	max_low = max(max_low, (PHYS_OFFSET + MAX_HOTPLUG_MEM) >> PAGE_SHIFT);
#endif
	high_memory = __va((max_low << PAGE_SHIFT) - 1) + 1;


#ifdef CONFIG_SNSC_SUPPORT_4KB_MAPPING
	/* Remap each memory node */
	for_each_node(node)
		remap_node(node, mi);
#endif

#if defined(CONFIG_ARCH_CXD90014BASED)
	printk("high_memory: %p\n", high_memory);
#endif
	/*
	 * This doesn't seem to be used by the Linux memory manager any
	 * more, but is used by ll_rw_block.  If we can get rid of it, we
	 * also get rid of some of the stuff above as well.
	 *
	 * Note: max_low_pfn and max_pfn reflect the number of _pages_ in
	 * the system, not the maximum PFN.
	 */
	max_low_pfn = max_low - PHYS_PFN_OFFSET;
	max_pfn = max_high - PHYS_PFN_OFFSET;

#ifdef CONFIG_ARCH_POPULATES_NODE_MAP
#ifdef CONFIG_ZONE_DMA
	max_zone_pfns[ZONE_DMA] = virt_to_phys((char *)MAX_DMA_ADDRESS) >> PAGE_SHIFT;
#endif
	max_zone_pfns[ZONE_NORMAL] = max_pfn + (PHYS_OFFSET >> PAGE_SHIFT);
	free_area_init_nodes(max_zone_pfns);
#endif /* CONFIG_ARCH_POPULATES_NODE_MAP */
}

static inline int free_area(unsigned long pfn, unsigned long end, char *s)
{
	unsigned int pages = 0, size = (end - pfn) << (PAGE_SHIFT - 10);

	for (; pfn < end; pfn++) {
		struct page *page = pfn_to_page(pfn);
		ClearPageReserved(page);
		init_page_count(page);
#ifdef CONFIG_SNSC_POISON_FREED_INIT_PAGES
		memset(phys_to_virt(pfn<<PAGE_SHIFT), POISON_FREE_INITMEM, PAGE_SIZE);
#endif
		__free_page(page);
		pages++;
	}

	if (size && s)
		printk(KERN_INFO "Freeing %s memory: %dK\n", s, size);

	return pages;
}

/*
 * Poison init memory with an undefined instruction (ARM) or a branch to an
 * undefined instruction (Thumb).
 */
static inline void poison_init_mem(void *s, size_t count)
{
	u32 *p = (u32 *)s;
	for (; count != 0; count -= 4)
		*p++ = 0xe7fddef0;
}

static inline void
free_memmap(int node, unsigned long start_pfn, unsigned long end_pfn)
{
	struct page *start_pg, *end_pg;
	unsigned long pg, pgend;

	/*
	 * Convert start_pfn/end_pfn to a struct page pointer.
	 */
	start_pg = pfn_to_page(start_pfn - 1) + 1;
	end_pg = pfn_to_page(end_pfn - 1) + 1;

	/*
	 * Convert to physical addresses, and
	 * round start upwards and end downwards.
	 */
	pg = (unsigned long)PAGE_ALIGN(__pa(start_pg));
	pgend = (unsigned long)__pa(end_pg) & PAGE_MASK;

	/*
	 * If there are free pages between these,
	 * free the section of the memmap array.
	 */
	if (pg < pgend)
		free_bootmem_node(NODE_DATA(node), pg, pgend - pg);
}

/*
 * The mem_map array can get very big.  Free the unused area of the memory map.
 */
static void __init free_unused_memmap_node(int node, struct meminfo *mi)
{
	unsigned long bank_start, prev_bank_end = 0;
	unsigned int i;

	/*
	 * This relies on each bank being in address order.
	 * The banks are sorted previously in bootmem_init().
	 */
	for_each_nodebank(i, mi, node) {
		struct membank *bank = &mi->bank[i];

		bank_start = bank_pfn_start(bank);

#ifdef CONFIG_SPARSEMEM
		/*
		 * Take care not to free memmap entries that don't exist
		 * due to SPARSEMEM sections which aren't present.
		 */
		bank_start = min(bank_start,
				 ALIGN(prev_bank_end, PAGES_PER_SECTION));
#else
		/*
		 * Align down here since the VM subsystem insists that the
		 * memmap entries are valid from the bank start aligned to
		 * MAX_ORDER_NR_PAGES.
		 */
		bank_start = round_down(bank_start, MAX_ORDER_NR_PAGES);
#endif
		/*
		 * If we had a previous bank, and there is a space
		 * between the current bank and the previous, free it.
		 */
		if (prev_bank_end && prev_bank_end != bank_start)
			free_memmap(node, prev_bank_end, bank_start);

		/*
		 * Align up here since the VM subsystem insists that the
		 * memmap entries are valid from the bank end aligned to
		 * MAX_ORDER_NR_PAGES.
		 */
		prev_bank_end = ALIGN(bank_pfn_end(bank), MAX_ORDER_NR_PAGES);
	}

#ifdef CONFIG_SPARSEMEM
	if (!IS_ALIGNED(prev_bank_end, PAGES_PER_SECTION))
		free_memmap(node, prev_bank_end,
			    ALIGN(prev_bank_end, PAGES_PER_SECTION));
#endif
}

/*
 * mem_init() marks the free areas in the mem_map and tells us how much
 * memory is free.  This is done after various parts of the system have
 * claimed their memory after the kernel image.
 */
void __init mem_init(void)
{
	unsigned long reserved_pages, free_pages;
	int i, node;
#ifdef CONFIG_HAVE_TCM
	/* These pointers are filled in on TCM detection */
	extern u32 dtcm_end;
	extern u32 itcm_end;
#endif

#if !defined(CONFIG_DISCONTIGMEM) && !defined(CONFIG_SPARSEMEM)
	max_mapnr   = pfn_to_page(max_pfn + PHYS_PFN_OFFSET) - mem_map;
#endif

	/* this will put all unused low memory onto the freelists */
	for_each_online_node(node) {
		pg_data_t *pgdat = NODE_DATA(node);

		free_unused_memmap_node(node, &meminfo);
#ifdef CONFIG_NODEZERO_ALLOCATION
		if (node > 0) {
#endif
		if (pgdat->node_spanned_pages != 0)
			totalram_pages += free_all_bootmem_node(pgdat);
#ifdef CONFIG_NODEZERO_ALLOCATION
		}
#endif
	}
#ifdef CONFIG_NODEZERO_ALLOCATION
	totalram_pages += free_all_bootmem_node(NODE_DATA(0));
#endif

#ifdef CONFIG_SA1111
	/* now that our DMA memory is actually so designated, we can free it */
	totalram_pages += free_area(PHYS_PFN_OFFSET,
				    __phys_to_pfn(__pa(swapper_pg_dir)), NULL);
#endif

#ifdef CONFIG_HIGHMEM
	/* set highmem page free */
	for_each_online_node(node) {
		for_each_nodebank (i, &meminfo, node) {
			unsigned long start = bank_pfn_start(&meminfo.bank[i]);
			unsigned long end = bank_pfn_end(&meminfo.bank[i]);
			if (start >= max_low_pfn + PHYS_PFN_OFFSET)
				totalhigh_pages += free_area(start, end, NULL);
		}
	}
	totalram_pages += totalhigh_pages;
#endif

	reserved_pages = free_pages = 0;

	for_each_online_node(node) {
		for_each_nodebank(i, &meminfo, node) {
			struct membank *bank = &meminfo.bank[i];
			unsigned int pfn1, pfn2, pfn;
			struct page *page, *end;

			pfn1 = bank_pfn_start(bank);
			pfn2 = bank_pfn_end(bank);
			pfn = pfn1;

			page = pfn_to_page(pfn1);
			end  = pfn_to_page(pfn2 - 1) + 1;

			while(pfn < pfn2){
				/*
				 * Since the pfn is from within membank, It must be valid!!
				 */
				BUG_ON(!pfn_valid(pfn));

				page = pfn_to_page(pfn);
				if (PageReserved(page))
					reserved_pages++;
				else if (!page_count(page))
					free_pages++;

				pfn = page_to_pfn(page);
				pfn++;
			}

		}
	}

	/*
	 * Since our memory may not be contiguous, calculate the
	 * real number of pages we have in this system
	 */
	printk(KERN_INFO "Memory:");
	num_physpages = 0;
	for (i = 0; i < meminfo.nr_banks; i++) {
		num_physpages += bank_pfn_size(&meminfo.bank[i]);
		printk(" %ldMB", bank_phys_size(&meminfo.bank[i]) >> 20);
	}
	printk(" = %luMB total\n", num_physpages >> (20 - PAGE_SHIFT));

	printk(KERN_NOTICE "Memory: %luk/%luk available, %luk reserved, %luK highmem\n",
		nr_free_pages() << (PAGE_SHIFT-10),
		free_pages << (PAGE_SHIFT-10),
		reserved_pages << (PAGE_SHIFT-10),
		totalhigh_pages << (PAGE_SHIFT-10));

#define MLK(b, t) b, t, ((t) - (b)) >> 10
#define MLM(b, t) b, t, ((t) - (b)) >> 20
#define MLK_ROUNDUP(b, t) b, t, DIV_ROUND_UP(((t) - (b)), SZ_1K)

	printk(KERN_NOTICE "Virtual kernel memory layout:\n"
			"    vector  : 0x%08lx - 0x%08lx   (%4ld kB)\n"
#ifdef CONFIG_HAVE_TCM
			"    DTCM    : 0x%08lx - 0x%08lx   (%4ld kB)\n"
			"    ITCM    : 0x%08lx - 0x%08lx   (%4ld kB)\n"
#endif
			"    fixmap  : 0x%08lx - 0x%08lx   (%4ld kB)\n"
#ifdef CONFIG_MMU
			"    DMA     : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#endif
			"    vmalloc : 0x%08lx - 0x%08lx   (%4ld MB)\n"
			"    lowmem  : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#ifdef CONFIG_HIGHMEM
			"    pkmap   : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#endif
			"    modules : 0x%08lx - 0x%08lx   (%4ld MB)\n"
			"      .text : 0x%p" " - 0x%p" "   (%4d kB)\n"
			"      .init : 0x%p" " - 0x%p" "   (%4d kB)\n"
			"      .data : 0x%p" " - 0x%p" "   (%4d kB)\n"
			"       .bss : 0x%p" " - 0x%p" "   (%4d kB)\n",

			MLK(UL(CONFIG_VECTORS_BASE), UL(CONFIG_VECTORS_BASE) +
				(PAGE_SIZE)),
#ifdef CONFIG_HAVE_TCM
			MLK(DTCM_OFFSET, (unsigned long) dtcm_end),
			MLK(ITCM_OFFSET, (unsigned long) itcm_end),
#endif
			MLK(FIXADDR_START, FIXADDR_TOP),
#ifdef CONFIG_MMU
			MLM(CONSISTENT_BASE, CONSISTENT_END),
#endif
			MLM(VMALLOC_START, VMALLOC_END),
			MLM(PAGE_OFFSET, (unsigned long)high_memory),
#ifdef CONFIG_HIGHMEM
			MLM(PKMAP_BASE, (PKMAP_BASE) + (LAST_PKMAP) *
				(PAGE_SIZE)),
#endif
			MLM(MODULES_VADDR, MODULES_END),

			MLK_ROUNDUP(_text, _etext),
			MLK_ROUNDUP(__init_begin, __init_end),
			MLK_ROUNDUP(_sdata, _edata),
			MLK_ROUNDUP(__bss_start, __bss_stop));

#undef MLK
#undef MLM
#undef MLK_ROUNDUP

	/*
	 * Check boundaries twice: Some fundamental inconsistencies can
	 * be detected at build time already.
	 */
#ifdef CONFIG_MMU
	BUILD_BUG_ON(VMALLOC_END			> CONSISTENT_BASE);
	BUG_ON(VMALLOC_END				> CONSISTENT_BASE);

	BUILD_BUG_ON(TASK_SIZE				> MODULES_VADDR);
	BUG_ON(TASK_SIZE 				> MODULES_VADDR);
#endif

#ifdef CONFIG_HIGHMEM
	BUILD_BUG_ON(PKMAP_BASE + LAST_PKMAP * PAGE_SIZE > PAGE_OFFSET);
	BUG_ON(PKMAP_BASE + LAST_PKMAP * PAGE_SIZE	> PAGE_OFFSET);
#endif

	if (PAGE_SIZE >= 16384 && num_physpages <= 128) {
		extern int sysctl_overcommit_memory;
		/*
		 * On a machine this small we won't get
		 * anywhere without overcommit, so turn
		 * it on by default.
		 */
		sysctl_overcommit_memory = OVERCOMMIT_ALWAYS;
	}
}

void free_initmem(void)
{
#ifndef CONFIG_EJ_DO_NOT_FREE_INIT
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING
        is_free_initmem_running = 1;
#endif

#ifdef CONFIG_HAVE_TCM
	extern char __tcm_start, __tcm_end;

	poison_init_mem(&__tcm_start, &__tcm_end - &__tcm_start);
	totalram_pages += free_area(__phys_to_pfn(__pa(&__tcm_start)),
				    __phys_to_pfn(__pa(&__tcm_end)),
				    "TCM link");
#endif

	poison_init_mem(__init_begin, __init_end - __init_begin);
	if (!machine_is_integrator() && !machine_is_cintegrator())
		totalram_pages += free_area(__phys_to_pfn(__pa(__init_begin)),
					    __phys_to_pfn(__pa(__init_end)),
					    "init");
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING
	is_free_initmem_running = 0;
#endif
#endif /* !CONFIG_EJ_DO_NOT_FREE_INIT */
}

#ifdef CONFIG_BLK_DEV_INITRD

static int keep_initrd;

void free_initrd_mem(unsigned long start, unsigned long end)
{
	if (!keep_initrd) {
		poison_init_mem((void *)start, PAGE_ALIGN(end) - start);
		totalram_pages += free_area(__phys_to_pfn(__pa(start)),
					    __phys_to_pfn(__pa(end)),
					    "initrd");
	}
}

static int __init keepinitrd_setup(char *__unused)
{
	keep_initrd = 1;
	return 1;
}

__setup("keepinitrd", keepinitrd_setup);
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
extern  unsigned int target_get_mem_type(unsigned long);

#ifdef CONFIG_NUMA
int memory_add_physaddr_to_nid(u64 start)
{
#ifdef CONFIG_SNSC_MEMPLUG
	/*
	 * This function is mainly used when online pages with
	 * CONFIG_SNSC_GROUP_SPARSEMEM_SECTIONS=n, and not support any
	 * longer.
	 */
	return MEM_HOTPLUG_NID;
#else
	return 0;
#endif
}
#endif


int __devinit arch_add_memory(int nid, u64 start, u64 size)
{
	struct pglist_data *pgdata;
	struct zone *zone;
	unsigned long start_pfn = start >> PAGE_SHIFT;
	unsigned long nr_pages = size >> PAGE_SHIFT;
	struct map_desc md;
	if (start & ((PAGES_PER_SECTION << PAGE_SHIFT) - 1))
	{
		printk(KERN_WARNING "BUG: start address 0x%08lx not aligned with section size. "
				"ignoring.\n", (unsigned long)start);
		return -EINVAL;
	}

	pgdata = NODE_DATA(nid);

	md.virtual = __phys_to_virt(start);
	md.pfn = start_pfn;
	md.length = size;
	md.type = MT_MEMORY;
	create_mapping(&md);
#ifdef CONFIG_ARCH_POPULATES_NODE_MAP
	/* Put hotplugged memory to ZONE_MOVABLE */
	zone = &pgdata->node_zones[ZONE_MOVABLE];

	add_active_range(nid, __phys_to_pfn(start),
			 __phys_to_pfn(start + size));
#else
	/* this should work for most non-highmem platforms */
	zone = pgdata->node_zones;
#endif

	return __add_pages(nid, zone, start_pfn, nr_pages);
}
#endif
