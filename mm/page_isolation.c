/* 2009-07-06: File changed by Sony Corporation */
/*
 * linux/mm/page_isolation.c
 */

#include <linux/mm.h>
#include <linux/page-isolation.h>
#include <linux/pageblock-flags.h>
#include "internal.h"

static inline struct page *
__first_valid_page(unsigned long pfn, unsigned long nr_pages)
{
	int i;
	for (i = 0; i < nr_pages; i++)
		if (pfn_valid_within(pfn + i))
			break;
	if (unlikely(i == nr_pages))
		return NULL;
	return pfn_to_page(pfn + i);
}

/*
 * start_isolate_page_range() -- make page-allocation-type of range of pages
 * to be MIGRATE_ISOLATE.
 * @start_pfn: The lower PFN of the range to be isolated.
 * @end_pfn: The upper PFN of the range to be isolated.
 *
 * Making page-allocation-type to be MIGRATE_ISOLATE means free pages in
 * the range will never be allocated. Any free pages and pages freed in the
 * future will not be allocated again.
 *
 * start_pfn/end_pfn must be aligned to pageblock_order.
 * Returns 0 on success and -EBUSY if any part of range cannot be isolated.
 */
int
start_isolate_page_range(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn;
	unsigned long undo_pfn;
	struct page *page;

	BUG_ON((start_pfn) & (pageblock_nr_pages - 1));
	BUG_ON((end_pfn) & (pageblock_nr_pages - 1));

	for (pfn = start_pfn;
	     pfn < end_pfn;
	     pfn += pageblock_nr_pages) {
		page = __first_valid_page(pfn, pageblock_nr_pages);
		if (page && set_migratetype_isolate(page)) {
			undo_pfn = pfn;
			goto undo;
		}
	}
	return 0;
undo:
	for (pfn = start_pfn;
	     pfn < undo_pfn;
	     pfn += pageblock_nr_pages)
		unset_migratetype_isolate(pfn_to_page(pfn));

	return -EBUSY;
}

/*
 * Make isolated pages available again.
 */
int
undo_isolate_page_range(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn;
	struct page *page;
	BUG_ON((start_pfn) & (pageblock_nr_pages - 1));
	BUG_ON((end_pfn) & (pageblock_nr_pages - 1));
	for (pfn = start_pfn;
	     pfn < end_pfn;
	     pfn += pageblock_nr_pages) {
		page = __first_valid_page(pfn, pageblock_nr_pages);
		if (!page || get_pageblock_migratetype(page) != MIGRATE_ISOLATE)
			continue;
		unset_migratetype_isolate(page);
	}
	return 0;
}

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
extern int is_ext_reserved_area(unsigned long phys);
#endif

/*
 * Test all pages in the range is free(means isolated) or not.
 * all pages in [start_pfn...end_pfn) must be in the same zone.
 * zone->lock must be held before call this.
 *
 * Returns 1 if all pages in the range is isolated.
 */
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
static int
__test_page_isolated_in_pageblock(unsigned long pfn, unsigned long end_pfn,
				  unsigned long *data)
#else
static int
__test_page_isolated_in_pageblock(unsigned long pfn, unsigned long end_pfn)
#endif
{
	struct page *page;
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	unsigned long offlined_pages = 0;  /* Just for calculate the correct
					    * number of offlined pages. */
#endif

	while (pfn < end_pfn) {
		if (!pfn_valid_within(pfn)) {
			pfn++;
			continue;
		}
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
		if (is_ext_reserved_area(pfn << PAGE_SHIFT)) {
			pfn++;
			continue;
		}
		page = pfn_to_page(pfn);
		if (PageBuddy(page)) {
			pfn += 1 << page_order(page);
			offlined_pages += 1 << page_order(page);
		}
		else if (page_count(page) == 0 &&
			 page_private(page) == MIGRATE_ISOLATE) {
			pfn += 1;
			offlined_pages +=1;
		}

#else
		page = pfn_to_page(pfn);
		if (PageBuddy(page))
 			pfn += 1 << page_order(page);
 		else if (page_count(page) == 0 &&
				page_private(page) == MIGRATE_ISOLATE)
 			pfn += 1;
#endif
		else
			break;
	}
	if (pfn < end_pfn)
		return 0;

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	*data += offlined_pages;
#endif
	return 1;
}

#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
int test_pages_isolated(unsigned long start_pfn, unsigned long end_pfn,
			unsigned long *data)
#else
int test_pages_isolated(unsigned long start_pfn, unsigned long end_pfn)
#endif
{
	unsigned long pfn, flags;
	struct page *page;
	struct zone *zone;
	int ret;

	/*
	 * Note: pageblock_nr_page != MAX_ORDER. Then, chunks of free page
	 * is not aligned to pageblock_nr_pages.
	 * Then we just check pagetype fist.
	 */
	for (pfn = start_pfn; pfn < end_pfn; pfn += pageblock_nr_pages) {
		page = __first_valid_page(pfn, pageblock_nr_pages);
		if (page && get_pageblock_migratetype(page) != MIGRATE_ISOLATE)
			break;
	}
	page = __first_valid_page(start_pfn, end_pfn - start_pfn);
	if ((pfn < end_pfn) || !page)
		return -EBUSY;
	/* Check all pages are free or Marked as ISOLATED */
	zone = page_zone(page);
	spin_lock_irqsave(&zone->lock, flags);
#ifdef CONFIG_SNSC_MEMORY_RESERVE_SUPPORT
	ret = __test_page_isolated_in_pageblock(start_pfn, end_pfn, data);
#else
	ret = __test_page_isolated_in_pageblock(start_pfn, end_pfn);
#endif
	spin_unlock_irqrestore(&zone->lock, flags);
	return ret ? 0 : -EBUSY;
}
