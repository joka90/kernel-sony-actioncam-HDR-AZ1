/* 2012-08-29: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot Core - image creation
 *
 *  Copyright 2008,2009,2010 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/pfn.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/swap.h>
#include <linux/fs.h>
#include <linux/crc32.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/page.h>
#include <asm/sections.h>
#include <linux/pagemap.h>
#include "internal.h"

/* page attribute */
typedef enum ssboot_pgattr {
	SSBOOT_PAGE_NOSAVE,
	SSBOOT_PAGE_NORMAL,
	SSBOOT_PAGE_CRITICAL,
	SSBOOT_PAGE_FORCE
} ssboot_pgattr_t;

/* section search mode */
typedef enum ssboot_search {
	SSBOOT_SEARCH_COUNT,
	SSBOOT_SEARCH_FILL
} ssboot_search_t;

/* section search sequence */
typedef enum ssboot_seq {
	SSBOOT_SEQ_INIT,
	SSBOOT_SEQ_DOING,
	SSBOOT_SEQ_FINISH
} ssboot_seq_t;

/* index of page bitmaps */
typedef enum ssboot_pgbmp_item {
	SSBOOT_PGBMP_NORM,
	SSBOOT_PGBMP_CRIT,
	SSBOOT_PGBMP_SCOPY,
	SSBOOT_PGBMP_ECOPY,
	SSBOOT_PGBMP_NUM
} ssboot_pgbmp_item_t;

/* alias of page bitmaps */
#define pgbmp_norm	pgbmp[SSBOOT_PGBMP_NORM]
#define pgbmp_crit	pgbmp[SSBOOT_PGBMP_CRIT]
#define pgbmp_scopy	pgbmp[SSBOOT_PGBMP_SCOPY]
#define pgbmp_ecopy	pgbmp[SSBOOT_PGBMP_ECOPY]
#define pgbmp_nosave	pgbmp_ecopy

/* page bitmaps */
static ssboot_pgbmp_t *pgbmp[SSBOOT_PGBMP_NUM];

/* swapfile */
static char *swapfile;

const char * const ssboot_optimizer_event_name[SSBOOT_OPTEVT_MAX] = {
	[SSBOOT_OPTEVT_FILE] = "file",
	[SSBOOT_OPTEVT_ANON] = "anon",
};

static void __noreturn
panic_bad_page(struct page *page)
{
	ssboot_err("bad page status:\n");
	ssboot_err("  pfn=0x%lx, flags=0x%lx, mapcount=%d, count=%d\n",
		   page_to_pfn(page), page->flags, page_mapcount(page),
		   page_count(page));
	dump_stack();
	panic("fatal error occurs in ssboot core");
}

static void
free_all_memory(void)
{
	int force_writeback = swapfile ? 1 : 0;
	unsigned long free, progress = 0;

	/* free all memory */
	ssboot_info("Freeing all memory.");
	do {
		/* free 10MB per loop */
		free = __shrink_all_memory(10 * SSBOOT_PG_1MB,
					   force_writeback, 1);
		/*
		 * __shrink_all_memory() may return non 0 value even
		 * if no pages are reclaimed. Therefore summation of
		 * the return value does not indicate actual reclaimed
		 * pages, and a "." does not mean that 10MB memory is
		 * reclaimed.
		 */
		progress += free;
		if (progress >= (10 * SSBOOT_PG_1MB)) {
			printk(".");
			progress = 0;
		}
	} while (free > 0);
	printk("done\n");
}

static int
free_memory(unsigned long num_pages)
{
	unsigned long free, total = 0;

	/* free memory */
	ssboot_info("Freeing memory.");
	do {
		free = shrink_all_memory(num_pages - total);
		total += free;
		printk(".");
	} while (free > 0 && total < num_pages);
	printk("done (%ldMB)\n", total / SSBOOT_PG_1MB);

	if (total < num_pages) {
		ssboot_info("cannot free %ld pages\n", num_pages);
		return -ENOMEM;
	}
	return 0;
}

static int
acquire_free_pages(unsigned long req_pages)
{
	struct zone *zone;
	unsigned long free = 0;

	/* calculate total free/required size */
	for_each_zone(zone) {
		if (populated_zone(zone)) {
			free += zone_page_state(zone, NR_FREE_PAGES);
			req_pages += zone->watermark[WMARK_HIGH];
		}
	}
	ssboot_dbg("acquire free memory (%ldMB/%ldMB)\n",
		   req_pages / SSBOOT_PG_1MB, free / SSBOOT_PG_1MB);

	/* free memory if necessary */
	if (free < req_pages) {
		return free_memory(req_pages - free);
	}
	return 0;
}

static int
is_page_reserved(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	if (PageReserved(page)) {
		return 1;
	}
	return 0;
}

static int
is_page_anon(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	if (PageAnon(page)) {
		return 1;
	}
	return 0;
}

static int
is_page_swapcache(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	if (PageSwapCache(page)) {
		return 1;
	}
	return 0;
}

static int
is_page_user(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	if (!PageSwapCache(page) &&
	    page_mapping(page) && mapping_mapped(page->mapping)) {
		return 1;
	}
	return 0;
}

static int
is_page_cache(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	if (!PageAnon(page) && PageLRU(page)) {
		return 1;
	}
	return 0;
}

static ssboot_pgattr_t
classify_page(unsigned long pfn, int force)
{
	unsigned long pfn_stext, pfn_etext;
	unsigned long pfn_srodata, pfn_erodata;
	unsigned long pfn_end;

	/* sanity check */
	if (!pfn_valid(pfn)) {
		goto nosave;
	}

	pfn_stext   = PFN_DOWN(__pa(&_stext));
	pfn_etext   = PFN_UP(__pa(&_etext));
	pfn_srodata = PFN_DOWN(__pa(&__start_rodata));
	pfn_erodata = PFN_UP(__pa(&__end_rodata));
	pfn_end     = PFN_UP(__pa(&_end));

	if (pfn >= pfn_stext) {

		/* kernel text / rodata */
		if ((pfn <= pfn_etext) ||
		    (pfn >= pfn_srodata && pfn <= pfn_erodata)) {
#ifdef CONFIG_SNSC_SSBOOT_NO_KERNEL
			goto nosave;
#else
			ssboot_pgstat_inc(SSBOOT_PGSTAT_KERNEL_RO);
			return SSBOOT_PAGE_NORMAL;
#endif
		}

		/* smaller than kernel end address */
		if (pfn <= pfn_end) {
			ssboot_pgstat_inc(SSBOOT_PGSTAT_KERNEL);
			return SSBOOT_PAGE_CRITICAL;
		}
	}

	/* reserved page */
	if (is_page_reserved(pfn)) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_RESERVED);
		return SSBOOT_PAGE_CRITICAL;
	}

	/* user page */
	if (is_page_user(pfn)) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_USER);
		return SSBOOT_PAGE_NORMAL;
	}

	/* anonymous page */
	if (is_page_anon(pfn)) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_ANON);
		return SSBOOT_PAGE_NORMAL;
	}

	/* swapcache */
	if (is_page_swapcache(pfn)) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_SWAPCACHE);
		return SSBOOT_PAGE_NORMAL;
	}

	/* page cache */
	if (is_page_cache(pfn)) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_CACHE);
		return SSBOOT_PAGE_NORMAL;
	}

	/* other kernel page */
	ssboot_pgstat_inc(SSBOOT_PGSTAT_KERNEL);
	return SSBOOT_PAGE_CRITICAL;

 nosave:
	/* forcibly saved page */
	if (force) {
		ssboot_pgstat_inc(SSBOOT_PGSTAT_OTHER);
		return SSBOOT_PAGE_FORCE;
	}

	/* not saved */
	return SSBOOT_PAGE_NOSAVE;
}

int
ssboot_free_page_bitmap(void)
{
	int i;

	ssboot_dbg("free page bitmaps (%ldKB)\n",
		   ssboot_pgbmp_size(pgbmp[0]) *
		   SSBOOT_PGBMP_NUM / SSBOOT_SZ_1KB);

	/* free page bitmaps */
	for (i = 0; i < SSBOOT_PGBMP_NUM; i++) {
		if (pgbmp[i] != NULL) {
			ssboot_pgbmp_free(pgbmp[i]);
			pgbmp[i] = NULL;
		}
	}
	return 0;
}

int
ssboot_alloc_page_bitmap(void)
{
	int i;

	/* allocate page bitmaps */
	for (i = 0; i < SSBOOT_PGBMP_NUM; i++) {
		pgbmp[i] = ssboot_pgbmp_alloc(&ssboot.memmap);
		if (pgbmp[i] == NULL) {
			ssboot_err("cannot allocate page bitmap #%d\n", i);
			ssboot_free_page_bitmap();
			return -ENOMEM;
		}
	}
	ssboot_dbg("allocate page bitmaps (%ldKB)\n",
		   ssboot_pgbmp_size(pgbmp[0]) *
		   SSBOOT_PGBMP_NUM / SSBOOT_SZ_1KB);

	return 0;
}

static void
mark_free_regions(ssboot_pgbmp_t *pgbmp_set)
{
	struct zone *zone;
	struct page *page;
	struct list_head *pos;
	int order;
	int type;

	/* mark page bitmap for free region */
	for_each_zone(zone) {
		if (is_highmem(zone) || !zone->spanned_pages) {
			continue;
		}
		for_each_migratetype_order(order, type) {
			list_for_each(pos,
				&zone->free_area[order].free_list[type]) {
				page = list_entry(pos, struct page, lru);
				ssboot_pgbmp_set_region(pgbmp_set,
							page_to_pfn(page),
							1 << order);
			}
		}
	}
}

static void
mark_extra_regions(unsigned long attr, ssboot_pgbmp_t *pgbmp_set,
				       ssboot_pgbmp_t *pgbmp_clear)
{
	unsigned long pfn, num;

	/* mark page bitmap for extra region */
	ssboot_exreg_find_first(attr, &pfn, &num);
	while (pfn != SSBOOT_PFN_NONE) {
		if (pgbmp_set != NULL) {
			ssboot_pgbmp_set_region(pgbmp_set, pfn, num);
		}
		if (pgbmp_clear != NULL) {
			ssboot_pgbmp_clear_region(pgbmp_clear, pfn, num);
		}
		ssboot_exreg_find_next(attr, &pfn, &num);
	}
}

int
ssboot_create_page_bitmap(void)
{
	struct zone *zone;
	unsigned long pfn;
	ssboot_pgattr_t attr;
	int force;

	/* initialize page statistics */
	ssboot_pgstat_init();

	/* initialize page bitmaps */
	ssboot_pgbmp_init(pgbmp_norm);
	ssboot_pgbmp_init(pgbmp_crit);
	ssboot_pgbmp_init(pgbmp_nosave);

	/* mark page bitmap for free region */
	mark_free_regions(pgbmp_nosave);

	/* mark page bitmaps for extra regions */
	mark_extra_regions(SSBOOT_EXREG_DISCARD,  pgbmp_nosave, NULL);
	mark_extra_regions(SSBOOT_EXREG_NORMAL,   pgbmp_norm,   pgbmp_nosave);
	mark_extra_regions(SSBOOT_EXREG_CRITICAL, pgbmp_crit,   pgbmp_nosave);
	mark_extra_regions(SSBOOT_EXREG_CRITICAL, NULL,         pgbmp_norm);
	mark_extra_regions(SSBOOT_EXREG_WORK,     pgbmp_nosave, NULL);

	/* find pages to save */
	for_each_zone(zone) {
		/* skip highmem/empty zone */
		if (is_highmem(zone) || !zone->spanned_pages) {
			continue;
		}

		/* search all pages in the zone */
		for (pfn = zone->zone_start_pfn;
		     pfn < zone->zone_start_pfn + zone->spanned_pages; pfn++) {

			/* skip free/discard/work region */
			if (ssboot_pgbmp_test(pgbmp_nosave, pfn)) {
				continue;
			}

			/* check if page is forcibly saved */
			force = (ssboot_pgbmp_test(pgbmp_norm, pfn) ||
				 ssboot_pgbmp_test(pgbmp_crit, pfn));

			/* classify page and update statistics */
			attr = classify_page(pfn, force);

			/* mark page bitmaps for pages to save */
			if (!force) {
				switch (attr) {
				case SSBOOT_PAGE_NORMAL:
					ssboot_pgbmp_set(pgbmp_norm, pfn);
					break;
				case SSBOOT_PAGE_CRITICAL:
					ssboot_pgbmp_set(pgbmp_crit, pfn);
					break;
				default:
					break;
				}
			}
		}
	}

	/* fixup page bitmaps for module R/O region */
	ssboot_pgbmp_find_first(pgbmp_crit, &pfn);
	while (pfn != SSBOOT_PFN_NONE) {
		if (module_text_pfn(pfn) ||
		    is_module_text_address((unsigned long)
					ssboot_pfn_to_virt(pfn))) {
			/* move page from critical to normal */
			ssboot_pgbmp_clear(pgbmp_crit, pfn);
			ssboot_pgbmp_set(pgbmp_norm, pfn);

			/* update statistics */
			ssboot_pgstat_dec(SSBOOT_PGSTAT_KERNEL);
			ssboot_pgstat_inc(SSBOOT_PGSTAT_MODULE_RO);
		}
		ssboot_pgbmp_find_next(pgbmp_crit, &pfn);
	}
	ssboot_dbg("create page bitmaps\n");

	return 0;
}

static void
free_copy_region(void)
{
	unsigned long start, end, free = 0;
	int order;

	/* free copied critical pages */
	ssboot_pgbmp_find_first(pgbmp_scopy, &start);
	ssboot_pgbmp_find_first(pgbmp_ecopy, &end);
	while (start != SSBOOT_PFN_NONE) {
		order = get_count_order(end - start + 1);
		__free_pages(pfn_to_page(start), order);
		free += 1 << order;
		ssboot_pgbmp_find_next(pgbmp_scopy, &start);
		ssboot_pgbmp_find_next(pgbmp_ecopy, &end);
	}
	ssboot_dbg("free copied critical pages (%ld pages)\n", free);
}

static int
alloc_copy_region(unsigned long num)
{
	struct page *page;
	unsigned long pfn, alloc = 0;
	int max, order;

	/* allocate copy region */
	order = MAX_ORDER;
	while (alloc < num) {
		max = get_bitmask_order(num - alloc) - 1;
		order = (order < max) ? order : max;
		while (order >= 0) {
			page = alloc_pages(GFP_ATOMIC | __GFP_NOWARN, order);
			if (page != NULL) {
				break;
			}
			order--;
		}
		if (order < 0) {
			ssboot_err("cannot allocate pages for copy\n");
			return -ENOMEM;
		}
		alloc += (1 << order);

		/* mark start and end pfn of allocated pages */
		pfn = page_to_pfn(page);
		ssboot_pgbmp_set(pgbmp_scopy, pfn);
		ssboot_pgbmp_set(pgbmp_ecopy, pfn + (1 << order) - 1);
	}
	ssboot_dbg("allocate pages for copy (%ld pages)\n", alloc);

	return 0;
}

void
ssboot_free_copied_pages(void)
{
	unsigned long pfn, num, remain;

	/* get number of critical pages */
	remain = ssboot_pgbmp_num_set(pgbmp_crit);

	/* unmark page bitmaps for work region */
	ssboot_exreg_find_first(SSBOOT_EXREG_WORK, &pfn, &num);
	while (pfn != SSBOOT_PFN_NONE && remain > 0) {
		num = (num < remain) ? num : remain;
		ssboot_pgbmp_clear(pgbmp_scopy, pfn);
		ssboot_pgbmp_clear(pgbmp_ecopy, pfn + num - 1);
		remain -= num;
		ssboot_exreg_find_next(SSBOOT_EXREG_WORK, &pfn, &num);
	}

	/* free copied critical pages */
	if (remain > 0) {
		free_copy_region();
	}
}

int
ssboot_copy_critical_pages(void)
{
	unsigned long total, remain;
	unsigned long pfn, num, src, dst, end;
	int ret;

	/* initialize page bitmaps for copy */
	ssboot_pgbmp_init(pgbmp_scopy);
	ssboot_pgbmp_init(pgbmp_ecopy);

	/* get number of critical pages */
	total = remain = ssboot_pgbmp_num_set(pgbmp_crit);

	/* find work region to copy critical pages */
	ssboot_exreg_find_first(SSBOOT_EXREG_WORK, &pfn, &num);
	while (pfn != SSBOOT_PFN_NONE && remain > 0) {
		num = (num < remain) ? num : remain;
		ssboot_pgbmp_set(pgbmp_scopy, pfn);
		ssboot_pgbmp_set(pgbmp_ecopy, pfn + num - 1);
		remain -= num;
		ssboot_exreg_find_next(SSBOOT_EXREG_WORK, &pfn, &num);
	}
	if (remain < total) {
		ssboot_dbg("prepare work region for copy (%ld pages)\n",
			   total - remain);
	}

	/* allocate pages for shortage of work region */
	if (remain > 0) {
		ret = alloc_copy_region(remain);
		if (ret < 0) {
			ssboot_free_copied_pages();
			return ret;
		}
	}

	/* copy critical pages */
	ssboot_pgbmp_find_first(pgbmp_crit, &src);
	ssboot_pgbmp_find_first(pgbmp_scopy, &dst);
	ssboot_pgbmp_find_first(pgbmp_ecopy, &end);
	while (src != SSBOOT_PFN_NONE) {
		copy_page(ssboot_pfn_to_virt(dst), ssboot_pfn_to_virt(src));
		ssboot_pgbmp_find_next(pgbmp_crit, &src);
		if (dst++ == end) {
			/* find next allocated region */
			ssboot_pgbmp_find_next(pgbmp_scopy, &dst);
			ssboot_pgbmp_find_next(pgbmp_ecopy, &end);
		}
	}
	ssboot_dbg("copy critical pages (%ld pages)\n", total);

	return 0;
}

static int
ssboot_wait_on_page_cache_locked(unsigned long pfn)
{
	int retry = 0;
	struct page *page = pfn_to_page(pfn);

	/*
	 * We are outsider against page management system, we can't
	 * rely on wait_on_page_locked().
	 */
	while (PageLocked(page) && is_page_cache(pfn)) {
		schedule_timeout_uninterruptible(1);
		if (++retry % HZ == 0) {
			ssboot_err("page cache is locked for %d seconds...\n",
				   retry / HZ);
			ssboot_err("  pfn=0x%lx, flags=0x%lx\n",
				   page_to_pfn(page), page->flags);
		}
	}

	return retry;
}

int
ssboot_wait_io_completion(void)
{
	int ret;
	unsigned long pfn, num = 0;

	/* Make sure all page caches linked to LRU list. */
	ret = lru_add_drain_all();
	if (ret)
		return ret;

	/* create page bitmaps before refer to bitmaps */
	ret = ssboot_create_page_bitmap();
	if (ret < 0) {
		return ret;
	}

	/*
	 * At creating image, we assume that all I/O to/from page
	 * caches have been completed. As a page under I/O is locked,
	 * we wait for all page caches to be unlocked.
	 */
	ssboot_pgbmp_find_first(pgbmp_norm, &pfn);
	while (pfn != SSBOOT_PFN_NONE) {
		if (pfn_valid(pfn) && is_page_cache(pfn)) {
			ret = ssboot_wait_on_page_cache_locked(pfn);
			if (ret < 0)
				return ret;
			else if (ret > 0)
				num++;
		}
		ssboot_pgbmp_find_next(pgbmp_norm, &pfn);
	}
	ssboot_dbg("wait for I/O completion (%ld pages)\n", num);

	return 0;
}

void
ssboot_lock_page_cache(void)
{
	struct page *page;
	unsigned long pfn, num = 0;

	/* lock page cache */
	ssboot_pgbmp_find_first(pgbmp_norm, &pfn);
	while (pfn != SSBOOT_PFN_NONE) {
		if (pfn_valid(pfn) && is_page_cache(pfn)) {
			page = pfn_to_page(pfn);
			if (!trylock_page(page)) {
				panic_bad_page(page);
			}
			num++;
		}
		ssboot_pgbmp_find_next(pgbmp_norm, &pfn);
	}
	ssboot_dbg("lock page cache (%ld pages)\n", num);
}

void
ssboot_unlock_page_cache(void)
{
	struct page *page;
	unsigned long pfn, num = 0;

	/* unlock page cache */
	ssboot_pgbmp_find_first(pgbmp_norm, &pfn);
	while (pfn != SSBOOT_PFN_NONE) {
		if (pfn_valid(pfn) && is_page_cache(pfn)) {
			page = pfn_to_page(pfn);
			if (!TestClearPageLocked(page)) {
				panic_bad_page(page);
			}
			num++;
		}
		ssboot_pgbmp_find_next(pgbmp_norm, &pfn);
	}
	ssboot_dbg("unlock page cache (%ld pages)\n", num);
}

static void
process_section(ssboot_search_t mode, ssboot_seq_t seq, unsigned long spfn,
		unsigned long wpfn, unsigned long attr)
{
	ssboot_image_t *image = &ssboot.image;
	static ssboot_section_t *curr_sect;
	static unsigned long curr_attr;
	static unsigned long next_spfn, next_wpfn;
	static unsigned long num_sect;

	switch (seq) {
	case SSBOOT_SEQ_INIT:
		num_sect  = 0;
		curr_sect = image->section - 1;
		curr_attr = 0;
		next_spfn = SSBOOT_PFN_NONE;
		next_wpfn = SSBOOT_PFN_NONE;
		break;
	case SSBOOT_SEQ_DOING:
		if (spfn != next_spfn || wpfn != next_wpfn ||
		    attr != curr_attr) {
			num_sect++;
			if (mode == SSBOOT_SEARCH_FILL) {
				BUG_ON(num_sect > image->num_section);
				curr_sect++;
				curr_sect->writer_pfn = wpfn;
				curr_sect->start_pfn  = spfn;
				curr_sect->num_pages  = 0;
				curr_sect->attr       = attr;
			}
			curr_attr = attr;
		}
		if (mode == SSBOOT_SEARCH_FILL) {
			curr_sect->num_pages++;
		}
		next_spfn = spfn + 1;
		next_wpfn = wpfn + 1;
		break;
	case SSBOOT_SEQ_FINISH:
		image->num_section = num_sect;
		break;
	default:
		BUG();
	}
}

static void
search_sections(ssboot_search_t mode)
{
	unsigned long src, dst, end;

	/* initialize section search */
	process_section(mode, SSBOOT_SEQ_INIT, 0, 0, 0);

	/* search critial section */
	ssboot_pgbmp_find_first(pgbmp_crit,  &src);
	ssboot_pgbmp_find_first(pgbmp_scopy, &dst);
	ssboot_pgbmp_find_first(pgbmp_ecopy, &end);
	while (src != SSBOOT_PFN_NONE) {
		process_section(mode, SSBOOT_SEQ_DOING, src, dst,
				SSBOOT_SECTION_CRITICAL);
		ssboot_pgbmp_find_next(pgbmp_crit, &src);
		if (dst++ == end) {
			/* find next copied region */
			ssboot_pgbmp_find_next(pgbmp_scopy, &dst);
			ssboot_pgbmp_find_next(pgbmp_ecopy, &end);
		}
	}

	/* search normal section */
	ssboot_pgbmp_find_first(pgbmp_norm, &src);
	while (src != SSBOOT_PFN_NONE) {
		process_section(mode, SSBOOT_SEQ_DOING, src, src,
				SSBOOT_SECTION_NORMAL);
		ssboot_pgbmp_find_next(pgbmp_norm, &src);
	}

	/* finalize section search */
	process_section(mode, SSBOOT_SEQ_FINISH, 0, 0, 0);
}

void
ssboot_free_section_list(void)
{
	ssboot_image_t *image = &ssboot.image;

	/* free section list */
	kfree(image->section);
	image->section = NULL;

	ssboot_dbg("free section list (%ldKB)\n",
		   sizeof(ssboot_section_t) *
		   image->num_section / SSBOOT_SZ_1KB);
}

int
ssboot_create_section_list(void)
{
	ssboot_image_t *image = &ssboot.image;

	/* count sections */
	search_sections(SSBOOT_SEARCH_COUNT);

	/* allocate section list */
	image->section = kmalloc(sizeof(ssboot_section_t) *
				 image->num_section, GFP_KERNEL);
	if(image->section == NULL){
		ssboot_err("cannot allocate section list\n");
		return -ENOMEM;
	}

	/* create section list */
	search_sections(SSBOOT_SEARCH_FILL);

	ssboot_dbg("create section list (%ldKB)\n",
		   sizeof(ssboot_section_t) *
		   image->num_section / SSBOOT_SZ_1KB);
	return 0;
}

int
ssboot_shrink_image(void)
{
	unsigned long num_work, num_crit;
	unsigned long pfn, num;
	int ret;

	switch (ssboot.imgmode) {
	case SSBOOT_IMGMODE_MIN:
		/* free all freeable memory */
		free_all_memory();
		break;
	case SSBOOT_IMGMODE_OPTIMIZE:
		/* call image optimizer */
		ret = ssboot_optimizer_optimize();
		if (ret)
			return ret;
		break;
	case SSBOOT_IMGMODE_MAX:
	default:
		/* do nothing */
		break;
	}

	/* create page bitmaps to estimate sections */
	ret = ssboot_create_page_bitmap();
	if (ret < 0) {
		return ret;
	}

	/* calculate number of pages in work region */
	num_work = 0;
	ssboot_exreg_find_first(SSBOOT_EXREG_WORK, &pfn, &num);
	while (pfn != SSBOOT_PFN_NONE) {
		num_work += num;
		ssboot_exreg_find_next(SSBOOT_EXREG_WORK, &pfn, &num);
	}

	/* acquire free pages to create image */
	num_crit = ssboot_pgbmp_num_set(pgbmp_crit);
	num = (num_crit > num_work) ? (num_crit - num_work) : 0;
	ret = acquire_free_pages(num + SSBOOT_EXTRA_FREE_PAGES);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

void
ssboot_show_image_info(void)
{
	ssboot_image_t *image = &ssboot.image;
	unsigned long i, num, attr;
	unsigned long num_crit, num_norm, num_pages;

#ifdef CONFIG_SNSC_SSBOOT_DEBUG_SECTION
	/* show section info */
	ssboot_info("Section Information:\n");
	ssboot_info("  Section  Physical Address          "
		    "KByte   Pages  Attribute\n");
	for (i = 0; i < image->num_section; i++) {
		unsigned long pfn;
		pfn  = image->section[i].start_pfn;
		num  = image->section[i].num_pages;
		attr = image->section[i].attr;
		ssboot_info("  #%06ld  0x%08llx - 0x%08llx  %6ld  %6ld  ",
			    i + 1, (u_int64_t)PFN_PHYS(pfn),
			    (u_int64_t)PFN_PHYS(pfn + num) - 1,
			    (unsigned long)PFN_PHYS(num) / SSBOOT_SZ_1KB, num);
		if (attr & SSBOOT_SECTION_CRITICAL) {
			printk("Critical");
		} else {
			printk("Normal");
		}
		printk("\n");
	}
#endif

	/* take statistics */
	num_crit = num_norm = num_pages = 0;
	for (i = 0; i < image->num_section; i++) {
		num  = image->section[i].num_pages;
		attr = image->section[i].attr;
		if (attr & SSBOOT_SECTION_CRITICAL) {
			num_crit += num;
		} else {
			num_norm += num;
		}
	}
	num_pages = num_norm + num_crit;

	/* show image info */
	ssboot_info("Image Information:\n");
	ssboot_info("  Section    : %5ld sections\n", image->num_section);
	ssboot_info("  Critical   : %5ld pages (%3ldMB)\n",
		    num_crit, num_crit / SSBOOT_PG_1MB);
	ssboot_info("  Normal     : %5ld pages (%3ldMB)\n",
		    num_norm, num_norm / SSBOOT_PG_1MB);
	ssboot_info("  Total      : %5ld pages (%3ldMB)\n",
		    num_pages, num_pages / SSBOOT_PG_1MB);
}

int
ssboot_swapoff(void)
{
	int ret;
	mm_segment_t old_fs;

	if (!swapfile)
		return 0;

	/* Note that swapoff can disable writable swapfile only. */

	old_fs = get_fs();
	set_fs(get_ds());

	/* make swapfile writable */
	ret = swap_set_ro(swapfile, 0);
	if (ret < 0)
		goto out;

	/* disable swapfile */
	ret = sys_swapoff(swapfile);
	if (ret < 0)
		goto out;

out:
	set_fs(old_fs);

	return 0;
}

int
ssboot_swapon(void)
{
	int ret;
	mm_segment_t old_fs;

	if (!swapfile)
		return 0;

	old_fs = get_fs();
	set_fs(get_ds());

	/* disable current swapfile */
	(void)ssboot_swapoff();

	/* enable it again */
	ret = sys_swapon(swapfile, 0);
	if (ret < 0)
		goto out;

out:
	set_fs(old_fs);

	return 0;
}

int
ssboot_swap_set_ro(void)
{
	int ret;

	if (!swapfile)
		return 0;

	/* make swapfile read-only */
	ret = swap_set_ro(swapfile, 1);
	if (ret < 0)
		return ret;

	return 0;
}

void
ssboot_set_swapfile(const char *filename)
{
	/* disable current swapfile */
	(void)ssboot_swapoff();

	if (swapfile) {
		kfree(swapfile);
		swapfile = NULL;
	}

	if (filename)
		swapfile = kstrdup(filename, GFP_KERNEL);
}

const char*
ssboot_get_swapfile(void)
{
	return swapfile;
}

int
ssboot_optimizer_record(enum ssboot_optimizer_event ev, ...)
{
	int ret = 0;
	va_list args;
	ssboot_optimizer_t *optimizer = ssboot.optimizer;

	if (!ssboot_optimizer_is_profiling()) {
		return 0;
	}

	/* call optimize operation */
	if (optimizer->record != NULL) {
		va_start(args, ev);
		ret = optimizer->record(optimizer->priv, ev, args);
		va_end(args);
		if (ret < 0) {
			ssboot_err("failed to record optimizer event(%s): "
				"%d\n", ssboot_optimizer_event_name[ev], ret);
		}
	}
	return ret;
}

int
ssboot_optimizer_optimize(void)
{
	int ret = 0;
	ssboot_optimizer_t *optimizer = ssboot.optimizer;

	/* check image optimizer */
	if (optimizer == NULL) {
		ssboot_err("image optimizer is not registered\n");
		return -ENODEV;
	}

	/* call optimize operation */
	if (optimizer->optimize != NULL) {
		ret = optimizer->optimize(optimizer->priv);
		if (ret < 0) {
			ssboot_err("failed to optimize image: %d\n", ret);
		}
	}
	return ret;
}

int
ssboot_optimizer_is_available(void)
{
	return ssboot.optimizer ? 1 : 0;
}

int
ssboot_optimizer_start_profiling(void)
{
	int ret = 0;
	ssboot_optimizer_t *optimizer = ssboot.optimizer;

	/* sanity check */
	if (optimizer == NULL) {
		ssboot_err("image optimizer is not registered\n");
		return -ENODEV;
	}

	/* call start_profiling operation */
	if (optimizer->start_profiling != NULL) {
		ret = optimizer->start_profiling(optimizer->priv);
		if (ret < 0) {
			ssboot_err("failed to start profiling: %d\n", ret);
		}
	}
	return ret;
}
EXPORT_SYMBOL(ssboot_optimizer_start_profiling);

int
ssboot_optimizer_stop_profiling(void)
{
	int ret = 0;
	ssboot_optimizer_t *optimizer = ssboot.optimizer;

	/* sanity check */
	if (optimizer == NULL) {
		ssboot_err("image optimizer is not registered\n");
		return -ENODEV;
	}

	/* call stop_profiling operation */
	if (optimizer->stop_profiling != NULL) {
		ret = optimizer->stop_profiling(optimizer->priv);
		if (ret < 0) {
			ssboot_err("failed to stop profiling: %d\n", ret);
		}
	}
	return ret;
}
EXPORT_SYMBOL(ssboot_optimizer_stop_profiling);

int
ssboot_optimizer_is_profiling(void)
{
	int ret = 0;
	ssboot_optimizer_t *optimizer = ssboot.optimizer;

	if (optimizer == NULL) {
		return 0;
	}

	/* call is_profiling operation */
	if (optimizer->is_profiling != NULL) {
		ret = optimizer->is_profiling(optimizer->priv);
	}
	return ret;
}
