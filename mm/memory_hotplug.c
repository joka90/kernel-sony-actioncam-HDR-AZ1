/* 2010-10-15: File changed by Sony Corporation */
/*
 *  linux/mm/memory_hotplug.c
 *
 *  Copyright (C)
 */

#include <linux/stddef.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/bootmem.h>
#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/pagevec.h>
#include <linux/writeback.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/cpu.h>
#include <linux/memory.h>
#include <linux/memory_hotplug.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/migrate.h>
#include <linux/page-isolation.h>
#include <linux/pfn.h>
#include <linux/suspend.h>
#include <linux/mm_inline.h>
#include <linux/firmware-map.h>
#ifdef CONFIG_SNSC_FREEZE_PROCESSES_BEFORE_HOTREMOVE
#include <linux/freezer.h>
#endif

#include <asm/tlbflush.h>

#include "internal.h"

DEFINE_MUTEX(mem_hotplug_mutex);

#ifdef CONFIG_SNSC_PGSCAN
extern void show_pgscan(unsigned long, unsigned long);
#endif

void lock_memory_hotplug(void)
{
	mutex_lock(&mem_hotplug_mutex);

	/* for exclusive hibernation if CONFIG_HIBERNATION=y */
	lock_system_sleep();
}

void unlock_memory_hotplug(void)
{
	unlock_system_sleep();
	mutex_unlock(&mem_hotplug_mutex);
}


#ifndef CONFIG_SNSC_HOTREMOVE_RETRY_DELAY
#define CONFIG_SNSC_HOTREMOVE_RETRY_DELAY 40
#endif

#define MEMPLUG_HOTREMOVE_RETRY_DELAY CONFIG_SNSC_HOTREMOVE_RETRY_DELAY

/* add this memory to iomem resource */
static struct resource *register_memory_resource(u64 start, u64 size)
{
	struct resource *res;
	res = kzalloc(sizeof(struct resource), GFP_KERNEL);
	BUG_ON(!res);

	res->name = "System RAM";
	res->start = start;
	res->end = start + size - 1;
	res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	if (request_resource(&iomem_resource, res) < 0) {
		printk("System RAM resource %llx - %llx cannot be added\n",
		(unsigned long long)res->start, (unsigned long long)res->end);
		kfree(res);
		res = NULL;
	}
	return res;
}

static void release_memory_resource(struct resource *res)
{
	if (!res)
		return;
	release_resource(res);
	kfree(res);
	return;
}

#ifdef CONFIG_MEMORY_HOTPLUG_SPARSE
#ifndef CONFIG_SPARSEMEM_VMEMMAP
static void get_page_bootmem(unsigned long info,  struct page *page,
			     unsigned long type)
{
	page->lru.next = (struct list_head *) type;
	SetPagePrivate(page);
	set_page_private(page, info);
	atomic_inc(&page->_count);
}

/* reference to __meminit __free_pages_bootmem is valid
 * so use __ref to tell modpost not to generate a warning */
void __ref put_page_bootmem(struct page *page)
{
	unsigned long type;

	type = (unsigned long) page->lru.next;
	BUG_ON(type < MEMORY_HOTPLUG_MIN_BOOTMEM_TYPE ||
	       type > MEMORY_HOTPLUG_MAX_BOOTMEM_TYPE);

	if (atomic_dec_return(&page->_count) == 1) {
		ClearPagePrivate(page);
		set_page_private(page, 0);
		INIT_LIST_HEAD(&page->lru);
		__free_pages_bootmem(page, 0);
	}

}

static void register_page_bootmem_info_section(unsigned long start_pfn)
{
	unsigned long *usemap, mapsize, section_nr, i;
	struct mem_section *ms;
	struct page *page, *memmap;

	section_nr = pfn_to_section_nr(start_pfn);
	ms = __nr_to_section(section_nr);

	/* Get section's memmap address */
	memmap = sparse_decode_mem_map(ms->section_mem_map, section_nr);

	/*
	 * Get page for the memmap's phys address
	 * XXX: need more consideration for sparse_vmemmap...
	 */
	page = virt_to_page(memmap);
	mapsize = sizeof(struct page) * PAGES_PER_SECTION;
	mapsize = PAGE_ALIGN(mapsize) >> PAGE_SHIFT;

	/* remember memmap's page */
	for (i = 0; i < mapsize; i++, page++)
		get_page_bootmem(section_nr, page, SECTION_INFO);

	usemap = __nr_to_section(section_nr)->pageblock_flags;
	page = virt_to_page(usemap);

	mapsize = PAGE_ALIGN(usemap_size()) >> PAGE_SHIFT;

	for (i = 0; i < mapsize; i++, page++)
		get_page_bootmem(section_nr, page, MIX_SECTION_INFO);

}

void register_page_bootmem_info_node(struct pglist_data *pgdat)
{
	unsigned long i, pfn, end_pfn, nr_pages;
	int node = pgdat->node_id;
	struct page *page;
	struct zone *zone;

	nr_pages = PAGE_ALIGN(sizeof(struct pglist_data)) >> PAGE_SHIFT;
	page = virt_to_page(pgdat);

	for (i = 0; i < nr_pages; i++, page++)
		get_page_bootmem(node, page, NODE_INFO);

	zone = &pgdat->node_zones[0];
	for (; zone < pgdat->node_zones + MAX_NR_ZONES - 1; zone++) {
		if (zone->wait_table) {
			nr_pages = zone->wait_table_hash_nr_entries
				* sizeof(wait_queue_head_t);
			nr_pages = PAGE_ALIGN(nr_pages) >> PAGE_SHIFT;
			page = virt_to_page(zone->wait_table);

			for (i = 0; i < nr_pages; i++, page++)
				get_page_bootmem(node, page, NODE_INFO);
		}
	}

	pfn = pgdat->node_start_pfn;
	end_pfn = pfn + pgdat->node_spanned_pages;

	/* register_section info */
	for (; pfn < end_pfn; pfn += PAGES_PER_SECTION) {
		/*
		 * Some platforms can assign the same pfn to multiple nodes - on
		 * node0 as well as nodeN.  To avoid registering a pfn against
		 * multiple nodes we check that this pfn does not already
		 * reside in some other node.
		 */
		if (pfn_valid(pfn) && (pfn_to_nid(pfn) == node))
			register_page_bootmem_info_section(pfn);
	}
}
#endif /* !CONFIG_SPARSEMEM_VMEMMAP */

static void grow_zone_span(struct zone *zone, unsigned long start_pfn,
			   unsigned long end_pfn)
{
	unsigned long old_zone_end_pfn;

	zone_span_writelock(zone);

	old_zone_end_pfn = zone->zone_start_pfn + zone->spanned_pages;
	if (start_pfn < zone->zone_start_pfn)
		zone->zone_start_pfn = start_pfn;

	zone->spanned_pages = max(old_zone_end_pfn, end_pfn) -
				zone->zone_start_pfn;

	zone_span_writeunlock(zone);
}

static void grow_pgdat_span(struct pglist_data *pgdat, unsigned long start_pfn,
			    unsigned long end_pfn)
{
	unsigned long old_pgdat_end_pfn =
		pgdat->node_start_pfn + pgdat->node_spanned_pages;

	if (start_pfn < pgdat->node_start_pfn)
		pgdat->node_start_pfn = start_pfn;

	pgdat->node_spanned_pages = max(old_pgdat_end_pfn, end_pfn) -
					pgdat->node_start_pfn;
}

static int __meminit __add_zone(struct zone *zone, unsigned long phys_start_pfn)
{
	struct pglist_data *pgdat = zone->zone_pgdat;
	int nr_pages = PAGES_PER_SECTION;
	int nid = pgdat->node_id;
	int zone_type;
	unsigned long flags;

	zone_type = zone - pgdat->node_zones;
	if (!zone->wait_table) {
		int ret;

		ret = init_currently_empty_zone(zone, phys_start_pfn,
						nr_pages, MEMMAP_HOTPLUG);
		if (ret)
			return ret;
	}
	pgdat_resize_lock(zone->zone_pgdat, &flags);
	grow_zone_span(zone, phys_start_pfn, phys_start_pfn + nr_pages);
	grow_pgdat_span(zone->zone_pgdat, phys_start_pfn,
			phys_start_pfn + nr_pages);
	pgdat_resize_unlock(zone->zone_pgdat, &flags);
	memmap_init_zone(nr_pages, nid, zone_type,
			 phys_start_pfn, MEMMAP_HOTPLUG);
	return 0;
}

static int __meminit __add_section(int nid, struct zone *zone,
					unsigned long phys_start_pfn)
{
	int nr_pages = PAGES_PER_SECTION;
	int ret;

	if (pfn_valid(phys_start_pfn))
		return -EEXIST;

	ret = sparse_add_one_section(zone, phys_start_pfn, nr_pages);

	if (ret < 0)
		return ret;

	ret = __add_zone(zone, phys_start_pfn);

	if (ret < 0)
		return ret;

	return register_new_memory(nid, __pfn_to_section(phys_start_pfn));
}

#ifdef CONFIG_SPARSEMEM_VMEMMAP
static int __remove_section(struct zone *zone, struct mem_section *ms)
{
	/*
	 * XXX: Freeing memmap with vmemmap is not implement yet.
	 *      This should be removed later.
	 */
	return -EBUSY;
}
#else
static int __remove_section(struct zone *zone, struct mem_section *ms)
{
	unsigned long flags;
	struct pglist_data *pgdat = zone->zone_pgdat;
	int ret = -EINVAL;

	if (!valid_section(ms))
		return ret;

	ret = unregister_memory_section(ms);
	if (ret)
		return ret;

	pgdat_resize_lock(pgdat, &flags);
	sparse_remove_one_section(zone, ms);
	pgdat_resize_unlock(pgdat, &flags);
	return 0;
}
#endif

#ifdef CONFIG_SNSC_GATHER_MEMMAP_FOR_SECTIONS
void *section_group_memmap;
int secgrp_memmap_start_sec_nr;
#endif

/*
 * Reasonably generic function for adding memory.  It is
 * expected that archs that support memory hotplug will
 * call this function after deciding the zone to which to
 * add the new pages.
 */
int __ref __add_pages(int nid, struct zone *zone, unsigned long phys_start_pfn,
			unsigned long nr_pages)
{
	unsigned long i;
	int err = 0;
	int start_sec, end_sec;
#ifdef CONFIG_SNSC_GATHER_MEMMAP_FOR_SECTIONS
	struct page *page;
	unsigned long page_addr;
	unsigned long memmap_size = sizeof(struct page) * nr_pages;
	int order = get_order(memmap_size);
#endif
	/* during initialize mem_map, align hot-added range to section */
	start_sec = pfn_to_section_nr(phys_start_pfn);
	end_sec = pfn_to_section_nr(phys_start_pfn + nr_pages - 1);
#ifdef CONFIG_SNSC_GATHER_MEMMAP_FOR_SECTIONS
	page = alloc_pages(GFP_KERNEL|__GFP_NOWARN, order);
	if (page == NULL)
		panic("Not enough memory for memmap[].\n");

	/*
	 * Free unused pages.
	 *
	 * We use alloc_pages() to alloc memmap. This may lead to large
	 * memory waste if memmap's size is not power of 2.
	 *
	 * eg. On ARM, for 64M memory, with CONFIG_SNSC_SECTION_NR_OUT_OF_PAGE_FLAG==y (sizeof(struct page) == 36)
	 *
	 *   number of pages = 64M / 4K = 16K
	 *   memmap_size = 16K * 36 = 576K
	 *   alloc_pages() will get us 1M. Nearly 500K is wasted.
	 *
	 * Use the following dirty trick to free unused pages.
	 */
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING
	/*
	 * This free action should not be tracked by pageacct.
	 */
	{
		extern int is_free_initmem_running;
		is_free_initmem_running = 1;
	}
#endif
	for ( page_addr = ((unsigned long)page_address(page) + memmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	      page_addr < (unsigned long)page_address(page) + (1 << order) * PAGE_SIZE;
	      page_addr += PAGE_SIZE ) {
		struct page *_page = virt_to_page((void *)page_addr);
		ClearPageReserved(_page);
		init_page_count(_page);
		free_page(page_addr);
	}
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING
	{
		extern int is_free_initmem_running;
		is_free_initmem_running = 0;
	}
#endif

	section_group_memmap = page_address(page);
	memset(section_group_memmap, 0, memmap_size);
	secgrp_memmap_start_sec_nr = start_sec;
#endif
	for (i = start_sec; i <= end_sec; i++) {
		err = __add_section(nid, zone, i << PFN_SECTION_SHIFT);

		/*
		 * EEXIST is finally dealt with by ioresource collision
		 * check. see add_memory() => register_memory_resource()
		 * Warning will be printed if there is collision.
		 */
		if (err && (err != -EEXIST)){
#ifdef CONFIG_SNSC_GATHER_MEMMAP_FOR_SECTIONS
			__free_pages(page, order);
#endif
			break;
		}
		err = 0;
	}

	return err;
}
EXPORT_SYMBOL_GPL(__add_pages);

/**
 * __remove_pages() - remove sections of pages from a zone
 * @zone: zone from which pages need to be removed
 * @phys_start_pfn: starting pageframe (must be aligned to start of a section)
 * @nr_pages: number of pages to remove (must be multiple of section size)
 *
 * Generic helper function to remove section mappings and sysfs entries
 * for the section of the memory we are removing. Caller needs to make
 * sure that pages are marked reserved and zones are adjust properly by
 * calling offline_pages().
 */
int __remove_pages(struct zone *zone, unsigned long phys_start_pfn,
		 unsigned long nr_pages)
{
	unsigned long i, ret = 0;
	int sections_to_remove;

	/*
	 * We can only remove entire sections
	 */
	BUG_ON(phys_start_pfn & ~PAGE_SECTION_MASK);
	BUG_ON(nr_pages % PAGES_PER_SECTION);

	sections_to_remove = nr_pages / PAGES_PER_SECTION;
	for (i = 0; i < sections_to_remove; i++) {
		unsigned long pfn = phys_start_pfn + i*PAGES_PER_SECTION;
		release_mem_region(pfn << PAGE_SHIFT,
				   PAGES_PER_SECTION << PAGE_SHIFT);
		ret = __remove_section(zone, __pfn_to_section(pfn));
		if (ret)
			break;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(__remove_pages);

void online_page(struct page *page)
{
	unsigned long pfn = page_to_pfn(page);

	totalram_pages++;
	if (pfn >= num_physpages)
		num_physpages = pfn + 1;

#ifdef CONFIG_HIGHMEM
	if (PageHighMem(page))
		totalhigh_pages++;
#endif

	ClearPageReserved(page);
	init_page_count(page);
	__free_page(page);
}

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
extern int is_ext_reserved_area(unsigned long phys);
#endif

static int online_pages_range(unsigned long start_pfn, unsigned long nr_pages,
			void *arg)
{
	unsigned long i;
	unsigned long onlined_pages = *(unsigned long *)arg;
	struct page *page;
	if (PageReserved(pfn_to_page(start_pfn)))
		for (i = 0; i < nr_pages; i++) {
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
			if (is_ext_reserved_area((start_pfn + i) << PAGE_SHIFT))
				continue;
#endif
			page = pfn_to_page(start_pfn + i);
			online_page(page);
			onlined_pages++;
		}
	*(unsigned long *)arg = onlined_pages;
	return 0;
}

extern void set_range_migratetype(unsigned long , unsigned long, int);

int __ref online_pages(unsigned long pfn, unsigned long nr_pages)
{
	unsigned long onlined_pages = 0;
	struct zone *zone;
	int need_zonelists_rebuild = 0;
	int nid;
	int ret;
	struct memory_notify arg;
	pg_data_t *pgdat;
	const struct cpumask *mask;

	lock_memory_hotplug();
	arg.start_pfn = pfn;
	arg.nr_pages = nr_pages;
	arg.status_change_nid = -1;

	nid = page_to_nid(pfn_to_page(pfn));
	if (node_present_pages(nid) == 0)
		arg.status_change_nid = nid;

	ret = memory_notify(MEM_GOING_ONLINE, &arg);
	ret = notifier_to_errno(ret);
	if (ret) {
		memory_notify(MEM_CANCEL_ONLINE, &arg);
		unlock_memory_hotplug();
		return ret;
	}

	/*
	 * Set all pageblock in the range to MIGRATE_MOVABLE.  This has
	 * been done by __add_pages() when sections are hotplugged for
	 * the first time. But the hotplugged section will be set to
	 * MIGRATE_ISOLATE when hot removed. When the section is hotplugged
	 * for the second time (by just "echo online > memoryxxx/state,
	 * without "echo 0xXXXXXXX > probe"), __add_pages() won't be
	 * called. So we have to reset the section's migration type to
	 * MIGRATE_MOVABLE, or else the section will be hotplugged to
	 * ZONE_ISOLATION. This should be improved if physical memory
	 * remove is well implemented in main line kernel (see
	 * Documentation/memory-hotplug.txt).
	 */
	set_range_migratetype(pfn, pfn + nr_pages, MIGRATE_MOVABLE);

	/*
	 * This doesn't need a lock to do pfn_to_page().
	 * The section can't be removed here because of the
	 * memory_block->state_mutex.
	 */
	zone = page_zone(pfn_to_page(pfn));
	/*
	 * If this zone is not populated, then it is not in zonelist.
	 * This means the page allocator ignores this zone.
	 * So, zonelist must be updated after online.
	 */
	mutex_lock(&zonelists_mutex);
	if (!populated_zone(zone))
		need_zonelists_rebuild = 1;

	ret = walk_system_ram_range(pfn, nr_pages, &onlined_pages,
		online_pages_range);
	if (ret) {
		mutex_unlock(&zonelists_mutex);
		MEMPLUG_ERR_PRINTK(KERN_ERR "online_pages %lu at 0x%lx failed\n",
			nr_pages, pfn);
		memory_notify(MEM_CANCEL_ONLINE, &arg);
		unlock_memory_hotplug();
		return ret;
	}

#ifdef CONFIG_SNSC_MEMPLUG
 	MEMPLUG_VERBOSE_PRINTK(KERN_INFO "Onlined pages %lu\n", onlined_pages);
#endif
	zone->present_pages += onlined_pages;
	zone->zone_pgdat->node_present_pages += onlined_pages;
	if (onlined_pages) {
		node_set_state(zone_to_nid(zone), N_HIGH_MEMORY);
		if (need_zonelists_rebuild)
			build_all_zonelists(zone);
		else
			zone_pcp_update(zone);
	}

	mutex_unlock(&zonelists_mutex);

	init_per_zone_wmark_min();

	if (onlined_pages) {
		kswapd_run(zone_to_nid(zone));
		pgdat = NODE_DATA(nid);
		mask = cpumask_of_node(pgdat->node_id);
		if (cpumask_any_and(cpu_online_mask, mask) < nr_cpu_ids)
			if(pgdat->kswapd)
				set_cpus_allowed_ptr(pgdat->kswapd, mask);
	}
	vm_total_pages = nr_free_pagecache_pages();

	writeback_set_ratelimit();

	if (onlined_pages)
		memory_notify(MEM_ONLINE, &arg);
	unlock_memory_hotplug();

	return 0;
}
#endif /* CONFIG_MEMORY_HOTPLUG_SPARSE */

/* we are OK calling __meminit stuff here - we have CONFIG_MEMORY_HOTPLUG */
static pg_data_t __ref *hotadd_new_pgdat(int nid, u64 start)
{
	struct pglist_data *pgdat;
	unsigned long zones_size[MAX_NR_ZONES] = {0};
	unsigned long zholes_size[MAX_NR_ZONES] = {0};
	unsigned long start_pfn = start >> PAGE_SHIFT;

	pgdat = arch_alloc_nodedata(nid);
	if (!pgdat)
		return NULL;

	arch_refresh_nodedata(nid, pgdat);

	/* we can use NODE_DATA(nid) from here */

	/* init node's zones as empty zones, we don't have any present pages.*/
	free_area_init_node(nid, zones_size, start_pfn, zholes_size);

	/*
	 * The node we allocated has no zone fallback lists. For avoiding
	 * to access not-initialized zonelist, build here.
	 */
	mutex_lock(&zonelists_mutex);
	build_all_zonelists(NULL);
	mutex_unlock(&zonelists_mutex);

	return pgdat;
}

static void rollback_node_hotadd(int nid, pg_data_t *pgdat)
{
	arch_refresh_nodedata(nid, NULL);
	arch_free_nodedata(pgdat);
	return;
}


/*
 * called by cpu_up() to online a node without onlined memory.
 */
int mem_online_node(int nid)
{
	pg_data_t	*pgdat;
	int	ret;

	lock_memory_hotplug();
	pgdat = hotadd_new_pgdat(nid, 0);
	if (!pgdat) {
		ret = -ENOMEM;
		goto out;
	}
	node_set_online(nid);
	ret = register_one_node(nid);
	BUG_ON(ret);

out:
	unlock_memory_hotplug();
	return ret;
}

/* we are OK calling __meminit stuff here - we have CONFIG_MEMORY_HOTPLUG */
int __ref add_memory(int nid, u64 start, u64 size)
{
	pg_data_t *pgdat = NULL;
	int new_pgdat = 0;
	struct resource *res;
	int ret;

	lock_memory_hotplug();

	res = register_memory_resource(start, size);
	ret = -EEXIST;
	if (!res)
		goto out;

	if (!node_online(nid)) {
		pgdat = hotadd_new_pgdat(nid, start);
		ret = -ENOMEM;
		if (!pgdat)
			goto out;
		new_pgdat = 1;
	}

	/* call arch's memory hotadd */
	ret = arch_add_memory(nid, start, size);

	if (ret < 0)
		goto error;

	/* we online node here. we can't roll back from here. */
	node_set_online(nid);

    /* Atleast building fallback zone list is required.
	 * Note: We can not call build_all_zonelists() before
	 * node_set_online() as this node would be ignored
	 * by for_each_onine_node() loop in __build_all_zonelists().
	 * Current fix in mainline kernel does not work for us because
	 * it calls build_all_zonelists() in hotadd_new_pgdat()
	 */
	mutex_lock(&zonelists_mutex);
	build_all_zonelists(NULL);
	mutex_unlock(&zonelists_mutex);

	if (new_pgdat) {
		ret = register_one_node(nid);
		/*
		 * If sysfs file of new node can't create, cpu on the node
		 * can't be hot-added. There is no rollback way now.
		 * So, check by BUG_ON() to catch it reluctantly..
		 */
		BUG_ON(ret);
	}

	/* create new memmap entry */
	firmware_map_add_hotplug(start, start + size, "System RAM");

	goto out;

error:
	/* rollback pgdat allocation and others */
	if (new_pgdat)
		rollback_node_hotadd(nid, pgdat);
	if (res)
		release_memory_resource(res);

out:
	unlock_memory_hotplug();
	return ret;
}
EXPORT_SYMBOL_GPL(add_memory);

#ifdef CONFIG_MEMORY_HOTREMOVE
/*
 * A free page on the buddy free lists (not the per-cpu lists) has PageBuddy
 * set and the size of the free page is given by page_order(). Using this,
 * the function determines if the pageblock contains only free pages.
 * Due to buddy contraints, a free page at least the size of a pageblock will
 * be located at the start of the pageblock
 */
static inline int pageblock_free(struct page *page)
{
	return PageBuddy(page) && page_order(page) >= pageblock_order;
}

/* Return the start of the next active pageblock after a given page */
static struct page *next_active_pageblock(struct page *page)
{
	/* Ensure the starting page is pageblock-aligned */
	BUG_ON(page_to_pfn(page) & (pageblock_nr_pages - 1));

	/* If the entire pageblock is free, move to the end of free page */
	if (pageblock_free(page)) {
		int order;
		/* be careful. we don't have locks, page_order can be changed.*/
		order = page_order(page);
		if ((order < MAX_ORDER) && (order >= pageblock_order))
			return page + (1 << order);
	}

	return page + pageblock_nr_pages;
}

/* Checks if this range of memory is likely to be hot-removable. */
int is_mem_section_removable(unsigned long start_pfn, unsigned long nr_pages)
{
	struct page *page = pfn_to_page(start_pfn);
	struct page *end_page = page + nr_pages;

	/* Check the starting page of each pageblock within the range */
	for (; page < end_page; page = next_active_pageblock(page)) {
		if (!is_pageblock_removable_nolock(page))
			return 0;
		cond_resched();
	}

	/* All pageblocks in the memory block are likely to be hot-removable */
	return 1;
}

/*
 * Confirm all pages in a range [start, end) is belongs to the same zone.
 */
static int test_pages_in_a_zone(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn;
	struct zone *zone = NULL;
	struct page *page;
	int i;
	for (pfn = start_pfn;
	     pfn < end_pfn;
	     pfn += MAX_ORDER_NR_PAGES) {
		i = 0;
		/* This is just a CONFIG_HOLES_IN_ZONE check.*/
		while ((i < MAX_ORDER_NR_PAGES) && !pfn_valid_within(pfn + i))
			i++;
		if (i == MAX_ORDER_NR_PAGES)
			continue;
		page = pfn_to_page(pfn + i);
		if (zone && page_zone(page) != zone)
			return 0;
		zone = page_zone(page);
	}
	return 1;
}

/*
 * Scanning pfn is much easier than scanning lru list.
 * Scan pfn from start to end and Find LRU page.
 */
static unsigned long scan_lru_pages(unsigned long start, unsigned long end)
{
	unsigned long pfn;
	struct page *page;
	for (pfn = start; pfn < end; pfn++) {
		if (pfn_valid(pfn)) {
			page = pfn_to_page(pfn);
			if (PageLRU(page))
				return pfn;
		}
	}
	return 0;
}

static struct page *
hotremove_migrate_alloc(struct page *page, unsigned long private, int **x)
{
	/* This should be improooooved!! */
	return alloc_page(GFP_HIGHUSER_MOVABLE);
}

static void count_file_and_anon(struct list_head *page_list, unsigned long * anon, unsigned long * file)
{
        struct page *page;
        list_for_each_entry(page, page_list, lru) {
		if(page_is_file_cache(page))
			(*file)++;
		else
			(*anon)++;
        }
}

static int
do_migrate_range(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn;
	struct page *page;
	int not_managed = 0;
	int ret = 0;
	LIST_HEAD(source);

#ifdef CONFIG_CGROUP_MEM_RES_CTLR
	mutex_lock(&memcg_memplug_migration_mutex);
#endif
	for (pfn = start_pfn; pfn < end_pfn; pfn++) {
		if (!pfn_valid(pfn))
			continue;
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
		if (is_ext_reserved_area(pfn << PAGE_SHIFT))
		    continue;
#endif
		page = pfn_to_page(pfn);
		if (!get_page_unless_zero(page))
			continue;
		/*
		 * We can skip free pages. And we can only deal with pages on
		 * LRU.
		 */
		ret = isolate_lru_page(page);
		if (!ret) { /* Success */
			put_page(page);
			list_add_tail(&page->lru, &source);
			inc_zone_page_state(page, NR_ISOLATED_ANON +
					    page_is_file_cache(page));

		} else {
#ifdef CONFIG_DEBUG_VM
			printk(KERN_ALERT "removing pfn 0x%lx from LRU failed\n",
			       pfn);
			dump_page(page);
#endif
			put_page(page);
			/* Because we don't have big zone->lock. we should
			   check this again here. */
			if (page_count(page)) {
				not_managed++;
				ret = -EBUSY;
				MEMPLUG_VERBOSE_PRINTK(KERN_WARNING "memplug INFO :"
					"%s:%d: page frame [ 0x%lx ] not managed is %d,"
					" ret is %d \n",__func__,__LINE__,
					page_to_pfn(page),not_managed,ret);
				break;
			}else{
				MEMPLUG_VERBOSE_PRINTK(KERN_WARNING "memplug INFO :"
					"%s:%d: isolate lru failed but page frame "
					"[ 0x%lx ] is managed, ret is %d \n",__func__,
					__LINE__,page_to_pfn(page),ret);
			}

		}
	}
	if (!list_empty(&source)) {
		if (not_managed) {
			putback_lru_pages(&source);
			goto out;
		}

#ifdef CONFIG_SNSC_RELEASE_ON_HOTREMOVE
	{
		struct scan_control sc = {
			.gfp_mask = GFP_KERNEL,
			.may_writepage = !laptop_mode,
			.may_swap = 0,
		};
		struct zone * zone = page_zone(pfn_to_page(start_pfn));
		unsigned long nr_anon_before = 0, nr_file_before = 0;
		unsigned long nr_anon_after = 0, nr_file_after = 0;
		sc.nr_scanned = 0;

		set_reclaim_mode(DEF_PRIORITY, &sc, true);
#ifdef CONFIG_KSWAPD_SHOULD_ONLY_WAIT_ON_IO_IF_THERE_IS_IO
		sc.nr_io_pages = 0;
#endif
		count_file_and_anon(&source, &nr_anon_before, &nr_file_before);
#ifdef CONFIG_SNSC_MM_WAIT_PAGEWRITEBACK
		ret = shrink_page_list(&source, zone, &sc, DEF_PRIORITY);
#else
		ret = shrink_page_list(&source, zone, &sc);
#endif
		count_file_and_anon(&source, &nr_anon_after, &nr_file_after);

		mod_zone_page_state(zone, NR_ISOLATED_ANON,  (nr_anon_after - nr_anon_before));
		mod_zone_page_state(zone, NR_ISOLATED_FILE,  (nr_file_after - nr_file_before));
	}
#endif
		/* this function returns # of failed pages */
		ret = migrate_pages(&source, hotremove_migrate_alloc, 0,
							true, MIGRATE_SYNC);
		if (ret)
			putback_lru_pages(&source);
	}
out:
#ifdef CONFIG_CGROUP_MEM_RES_CTLR
	mutex_unlock(&memcg_memplug_migration_mutex);
#endif
	return ret;
}

/*
 * remove from free_area[] and mark all as Reserved.
 */
static int
offline_isolated_pages_cb(unsigned long start, unsigned long nr_pages,
			void *data)
{
	__offline_isolated_pages(start, start + nr_pages);
	return 0;
}

static void
offline_isolated_pages(unsigned long start_pfn, unsigned long end_pfn)
{
	walk_system_ram_range(start_pfn, end_pfn - start_pfn, NULL,
				offline_isolated_pages_cb);
}

/*
 * Check all pages in range, recoreded as memory resource, are isolated.
 */
static int
check_pages_isolated_cb(unsigned long start_pfn, unsigned long nr_pages,
			void *data)
{
	int ret;
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	ret = test_pages_isolated(start_pfn, start_pfn + nr_pages, (unsigned long *)data);
#else
	long offlined = *(long *)data;
	ret = test_pages_isolated(start_pfn, start_pfn + nr_pages);
	offlined = nr_pages;
	if (!ret)
		*(long *)data += offlined;
#endif
	return ret;
}

static long
check_pages_isolated(unsigned long start_pfn, unsigned long end_pfn)
{
	long offlined = 0;
	int ret;

	ret = walk_system_ram_range(start_pfn, end_pfn - start_pfn, &offlined,
			check_pages_isolated_cb);
	if (ret < 0)
		offlined = (long)ret;
	return offlined;
}

#ifndef CONFIG_SNSC_OOMK_ON_HOTREMOVE_FAILURE
/*
 * Return the number of available pages to page migration between start_pfn
 * and end_pfn.
 * It should include free pages and file backed page cache.
 */
unsigned long
available_pages_in_range(unsigned long start_pfn, unsigned long end_pfn)
{
	struct page *page;
	int free = 0;
	for (; start_pfn < end_pfn; start_pfn ++) {
		/*
		 * When memory are specified with holes in one zone,
		 * such as "mem=16M@0x00000000 mem=16M@0x02000000", we have
		 * to skip those holes.
		 */
		if (!pfn_valid(start_pfn))
		    continue;

		page =  pfn_to_page(start_pfn);
		if ((page_count(page) == 0 /* free pages */
		     || (page_count(page) > 0
			 && page->mapping != NULL
			 && !((unsigned long)page->mapping &
			      PAGE_MAPPING_ANON)
			 && !PageSwapBacked(page))) /* file backed page cache */
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
		    && !is_ext_reserved_area(start_pfn << PAGE_SHIFT) /* not in reserved area */
#endif
		    && get_pageblock_flags(page) != MIGRATE_ISOLATE)  { /* not isolated */

			free ++;
		}
	}
	return free;
}

#ifdef CONFIG_SNSC_USE_NODE_ORDER
extern struct zonelist *policy_zonelist(gfp_t gfp, struct mempolicy *policy, int nd);
static struct zonelist *get_zonelist(void)
{
	/* This gfp_mask should be the same as the one used in
	 * hotremove_migrate_alloc()*/
	gfp_t gfp_mask = GFP_HIGHUSER_MOVABLE;
	struct mempolicy *pol = current->mempolicy;
	return policy_zonelist(gfp_mask, pol, numa_node_id());
}

static int enough_mem_for_page_offlining(unsigned long start_pfn,
				  unsigned long end_pfn)
{
	int total_avail = 0, local_avail = -1, local_busy, left_avail = 0, tmp;
	int local_included = 0;
	int i;
	struct zone *z;
	struct zonelist *zlist = get_zonelist();

	for (i=0; zlist->_zonerefs[i].zone != NULL; i++) {
		z = zlist->_zonerefs[i].zone;

		/* Page range that we want to offline is included in one of
		 * the zone which pages will be migrated to. */
		if (start_pfn >= z->zone_start_pfn && end_pfn <= z->zone_start_pfn + z->spanned_pages)
			local_included = 1;

		tmp = available_pages_in_range(z->zone_start_pfn, z->zone_start_pfn + z->spanned_pages);

		if (tmp < 0)
			tmp = 0;
		total_avail += tmp;
	}

#define K(x) ((x) << (PAGE_SHIFT - 10))
	local_avail = available_pages_in_range(start_pfn, end_pfn);
	local_busy = end_pfn - start_pfn - local_avail;

	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "total_avail: %dKB\n", K(total_avail));
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "local_avail: %dKB\n", K(local_avail));
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "local_busy: %dKB\n", K(local_busy));
#undef K

	if (local_included == 1) {
		left_avail = total_avail - local_avail;
	} else {
		left_avail = total_avail;
	}

	if (left_avail < 0)
		left_avail = 0;
	if (left_avail < local_busy)
		return 0;
	return 1;
}

#else

int enough_mem_for_page_offlining(unsigned long start_pfn,
				  unsigned long end_pfn)
{
	struct sysinfo i;
	unsigned long total_avail, local_avail, local_busy, left_avail;
	long cached;
	si_meminfo(&i);
	si_swapinfo(&i);

	cached = global_page_state(NR_FILE_PAGES) -
			total_swapcache_pages - i.bufferram;
	if (cached < 0)
		cached = 0;

	total_avail = i.freeram + cached; /* total available page */
	local_avail = available_pages_in_range(start_pfn, end_pfn); /* available pages in specified range */

	local_busy = end_pfn - start_pfn - local_avail; /* busy pages in
							   specified range */

	left_avail = total_avail - local_avail; /* available pages out
						     side of the specified range */
#define K(x) ((x) << (PAGE_SHIFT - 10))
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "total_avail: %luKB\n", K(total_avail));
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "local_avail: %luKB\n", K(local_avail));
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "local_busy: %luKB\n", K(local_busy));
	MEMPLUG_VERBOSE_PRINTK(KERN_DEBUG "left_avail: %luKB\n", K(left_avail));
#undef K
	if (left_avail < 0)
		left_avail = 0;
	if (left_avail < local_busy)
		return 0;
	return 1;
}
#endif /* CONFIG_SNSC_USE_NODE_ORDER */
#endif /* !CONFIG_SNSC_OOMK_ON_HOTREMOVE_FAILURE */

static int __ref offline_pages(unsigned long start_pfn,
		  unsigned long end_pfn, unsigned long timeout)
{
	unsigned long pfn, nr_pages, expire;
	long offlined_pages;
	int ret, retry_max, retry_timeout, retry_isolation_max, retry_isolation_timeout, node;
	struct zone *zone;
	struct memory_notify arg;

	BUG_ON(start_pfn >= end_pfn);
	/* at least, alignment against pageblock is necessary */
	if (!IS_ALIGNED(start_pfn, pageblock_nr_pages)){
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: start_pfn: 0x%lx not aligned with pageblock boundary.\n",
		       start_pfn);
		return -EINVAL;
	}
	if (!IS_ALIGNED(end_pfn, pageblock_nr_pages)){
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: end_pfn: 0x%lx not aligned with pageblock boundary.\n",
		       end_pfn);
		return -EINVAL;
	}

#ifndef CONFIG_SNSC_OOMK_ON_HOTREMOVE_FAILURE
	/* If CONFIG_SNSC_OOMK_ON_HOTREMOVE_FAILURE is set, do nothing,
	 * just leave the handling to page migration. If memory is not
	 * enough, oom-killer will be invoded then.*/
	if (!enough_mem_for_page_offlining(start_pfn, end_pfn)) {
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: no enough memory"
		       "left for performing the page offlining operation.\n");
		return -ENOMEM;
	}
#endif
	/* This makes hotplug much easier...and readable.
	   we assume this for now. .*/
	if (!test_pages_in_a_zone(start_pfn, end_pfn)){
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: pfns between 0x%lx and 0x%lx are not in the same zone.\n",
		       start_pfn, end_pfn);
		return -EINVAL;
	}

	lock_memory_hotplug();

	zone = page_zone(pfn_to_page(start_pfn));
	node = zone_to_nid(zone);
	nr_pages = end_pfn - start_pfn;

	/* set above range as isolated */
	ret = start_isolate_page_range(start_pfn, end_pfn);
	if (ret) {
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: not all pfns between 0x%lx and 0x%lx can be isolated.\n",
		       start_pfn, end_pfn);
	}

	arg.start_pfn = start_pfn;
	arg.nr_pages = nr_pages;
	arg.status_change_nid = -1;
	if (nr_pages >= node_present_pages(node))
		arg.status_change_nid = node;

	ret = memory_notify(MEM_GOING_OFFLINE, &arg);
	ret = notifier_to_errno(ret);
	if (ret)
		goto failed_removal;

	pfn = start_pfn;
	expire = jiffies + timeout;
 	retry_isolation_max = 5;
 	retry_isolation_timeout = 0;
repeat_isolation:
	retry_max = 5;
	retry_timeout = 0;
repeat:
	/* start memory hot removal */
	ret = -EAGAIN;
	if (time_after(jiffies, expire)){
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: timeout.\n");
		goto failed_removal;
	}
	ret = -EINTR;
	if (signal_pending(current)){
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: interrupted by user.\n");
		goto failed_removal;
	}
	ret = 0;

#ifdef CONFIG_SMP
	/* drain all zone's lru pagevec, this is asyncronous... */
	lru_add_drain_all();
#else
	lru_add_drain();
#endif
	flush_scheduled_work();
	drain_all_pages();

	pfn = scan_lru_pages(start_pfn, end_pfn);
	if (pfn) { /* We have page on LRU */
		ret = do_migrate_range(pfn, end_pfn);
		if (!ret) {
			goto repeat;
		} else {
			MEMPLUG_ERR_PRINTK(KERN_ERR "memplug INFO:%s:%d:"
				" do_migrate_range() could not migrate pfn between"
				" 0x%lx and 0x%lx , retry_max is %d, ret is %d \n"
				,__func__,__LINE__,pfn,end_pfn,retry_max,ret);

			if (ret < 0) {
				/* ret < 0 mostly means page not yet in LRU */
				if (ret == -ENOMEM) {
					MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: "
					       "memory allocation failed when migrating pages.\n");
					goto failed_removal;
				}
				if (--retry_max == 0) {
					MEMPLUG_ERR_PRINTK(KERN_ERR "memplug INFO:%s:%d:"
						" max retry reached : do_migrate_range() could not"
						" migrate pfn between 0x%lx and 0x%lx , repeat is %d,"
						" ret is %d \n",__func__,__LINE__,pfn,end_pfn,retry_max,ret);
					MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: max retry reached.\n");
					goto failed_removal;
				}

				/* Betters our chances to find the page in LRU
				 * in the next iteration. Trade-off with performance.
				 * (yield() will not work if our priority is more)
				 */
				retry_timeout += MEMPLUG_HOTREMOVE_RETRY_DELAY;
				schedule_timeout_interruptible(msecs_to_jiffies(retry_timeout));
			}else
				/* Means could not migrate temprorarily,
				 * but scheduling out may be unnecessarily costly in this case */
				yield();

			goto repeat;
		}
	}
#ifndef CONFIG_SMP
	/* drain all zone's lru pagevec, this is asyncronous... */
	lru_add_drain_all();
#endif
	/* drain pcp pages , this is synchrouns. */
	drain_all_pages();
	/* check again */
	offlined_pages = check_pages_isolated(start_pfn, end_pfn);
	if (offlined_pages < 0) {
  		MEMPLUG_ERR_PRINTK(KERN_ERR "memplug INFO:%s:%d: "
			"check_pages_isolated() returned negetive, range is"
			" 0x%lx and 0x%lx , retry_max is %d, offlined_pages is"
			" %ld, retry_isolation_max is %d \n",__func__,
			__LINE__,start_pfn,end_pfn,retry_max,offlined_pages,
			retry_isolation_max);

		if(retry_isolation_max-- > 0){
			retry_isolation_timeout += MEMPLUG_HOTREMOVE_RETRY_DELAY;
			schedule_timeout_interruptible(msecs_to_jiffies(retry_isolation_timeout));
			if(retry_isolation_max == 4)
				goto repeat_isolation;
			goto repeat;
		}
		MEMPLUG_ERR_PRINTK(KERN_ERR "memplug INFO:%s:%d: "
			"check_pages_isolated() failed, range is 0x%lx and "
			"0x%lx , retry_max is %d, offlined_pages is %ld,"
			"retry_isolation_max is %d \n",__func__,__LINE__,
			start_pfn,end_pfn,retry_max,offlined_pages,
			retry_isolation_max);
		MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining: page isolation check failed.\n");
		ret = -EBUSY;
		goto failed_removal;
	}
	MEMPLUG_VERBOSE_PRINTK(KERN_INFO "Offlined Pages %ld\n", offlined_pages);
	/* Ok, all of our target is islaoted.
	   We cannot do rollback at this point. */
	offline_isolated_pages(start_pfn, end_pfn);
	/* reset pagetype flags and makes migrate type to be MOVABLE */
	undo_isolate_page_range(start_pfn, end_pfn);
	/* removal success */
	zone->present_pages -= offlined_pages;
	zone->zone_pgdat->node_present_pages -= offlined_pages;
	totalram_pages -= offlined_pages;
	zone_pcp_update(zone);

	init_per_zone_wmark_min();

	if (!node_present_pages(node)) {
		node_set_offline(node);
		node_clear_state(node, N_HIGH_MEMORY);

/* If kswapd is frozen, thaw it */
#ifdef CONFIG_SNSC_FREEZE_PROCESSES_BEFORE_HOTREMOVE
		if (NODE_DATA(node)->kswapd && frozen(NODE_DATA(node)->kswapd))
			thaw_process(NODE_DATA(node)->kswapd);
#endif
		kswapd_stop(node);
		NODE_DATA(node)->kswapd = NULL;
	}

	vm_total_pages = nr_free_pagecache_pages();
	writeback_set_ratelimit();

	memory_notify(MEM_OFFLINE, &arg);
	unlock_memory_hotplug();
	return 0;

failed_removal:
	MEMPLUG_ERR_PRINTK(KERN_ERR "memory offlining 0x%lx to 0x%lx failed\n",
		start_pfn, end_pfn);
#ifdef CONFIG_SNSC_PGSCAN
	show_pgscan(start_pfn, end_pfn);
#endif
	memory_notify(MEM_CANCEL_OFFLINE, &arg);
	/* pushback to free area */
	undo_isolate_page_range(start_pfn, end_pfn);

	unlock_memory_hotplug();
	return ret;
}

int remove_memory(u64 start, u64 size)
{
	unsigned long start_pfn, end_pfn;

	start_pfn = PFN_DOWN(start);
	end_pfn = start_pfn + PFN_DOWN(size);
	return offline_pages(start_pfn, end_pfn, 120 * HZ);
}
#else
int remove_memory(u64 start, u64 size)
{
	return -EINVAL;
}
#endif /* CONFIG_MEMORY_HOTREMOVE */
EXPORT_SYMBOL_GPL(remove_memory);
