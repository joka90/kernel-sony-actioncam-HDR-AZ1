/*
 * arch/arm/mach-cxd90014/oom_kill.c
 *
 * for printing information during OOM
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on fs/proc/{meminfo.c, internal.h}
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/swap.h>
#include <linux/pagemap.h>
#include <linux/hugetlb.h>
#include <linux/oom_kill.h>

struct vmalloc_info {
	unsigned long	used;
	unsigned long	largest_chunk;
};

#ifdef CONFIG_MMU
#define VMALLOC_TOTAL (VMALLOC_END - VMALLOC_START)
extern void get_vmalloc_info(struct vmalloc_info *vmi);
#else

#define VMALLOC_TOTAL 0UL
#define get_vmalloc_info(vmi)			\
do {						\
	(vmi)->used = 0;			\
	(vmi)->largest_chunk = 0;		\
} while(0)
#endif

static char oom_buf[1024];
static int show_meminfo_and_panic(struct task_struct *target)
{
	struct sysinfo i;
	unsigned long committed;
	unsigned long allowed;
	struct vmalloc_info vmi;
	long cached;
	unsigned long pages[NR_LRU_LISTS];
	int lru;

#define K(x) ((x) << (PAGE_SHIFT - 10))
	si_meminfo(&i);
	si_swapinfo(&i);
	committed = percpu_counter_read_positive(&vm_committed_as);
	allowed = ((totalram_pages - hugetlb_total_pages())
		   * sysctl_overcommit_ratio / 100) + total_swap_pages;

	cached = global_page_state(NR_FILE_PAGES) -
		total_swapcache_pages - i.bufferram;
	if (cached < 0)
		cached = 0;

	get_vmalloc_info(&vmi);

	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_page_state(NR_LRU_BASE + lru);

	/*
	 * Tagged format, for easy grepping and expansion.
	 */
	snprintf(oom_buf, sizeof oom_buf,
		"Out Of Memory - pid(%d) to be killed\n"
		"MemTotal:       %8lu kB\n"
		"MemFree:        %8lu kB\n"
		"Buffers:        %8lu kB\n"
		"Cached:         %8lu kB\n"
		"SwapCached:     %8lu kB\n"
		"Active:         %8lu kB\n"
		"        anon:     %8lu kB\n"
		"        file:     %8lu kB\n"
		"Inactive:       %8lu kB\n"
		"        anon:     %8lu kB\n"
		"        file:     %8lu kB\n"
#ifdef CONFIG_UNEVICTABLE_LRU
		"Unevictable:    %8lu kB\n"
		"Mlocked:        %8lu kB\n"
#endif
#ifdef CONFIG_HIGHMEM
		"HighTotal:      %8lu kB\n"
		"HighFree:       %8lu kB\n"
		"LowTotal:       %8lu kB\n"
		"LowFree:        %8lu kB\n"
#endif
#ifndef CONFIG_MMU
		"MmapCopy:       %8lu kB\n"
#endif
		"SwapTotal:    %8lu kB\n"
		"SwapFree:     %8lu kB\n"
		"Dirty:        %8lu kB\n"
		"Writeback:    %8lu kB\n"
		"AnonPages:    %8lu kB\n"
		"Mapped:       %8lu kB\n"
		"Slab:         %8lu kB\n"
		"  Reclaimable:  %8lu kB\n"
		"  Unreclaim:    %8lu kB\n"
		"PageTables:   %8lu kB\n"
                "NFS_Unstable: %8lu kB\n"
                "Bounce:       %8lu kB\n"
		"WritebackTmp: %8lu kB\n"
		"CommitLimit:  %8lu kB\n"
		"Committed_AS: %8lu kB\n"
		"VmallocTotal: %8lu kB\n"
		"VmallocUsed:  %8lu kB\n"
		"VmallocChunk: %8lu kB\n",
		target->pid,
		K(i.totalram),
		K(i.freeram),
		K(i.bufferram),
		K(cached),
		K(total_swapcache_pages),
		K(pages[LRU_ACTIVE_ANON]   + pages[LRU_ACTIVE_FILE]),
                K(pages[LRU_ACTIVE_ANON]),
                K(pages[LRU_ACTIVE_FILE]),
                K(pages[LRU_INACTIVE_ANON] + pages[LRU_INACTIVE_FILE]),
                K(pages[LRU_INACTIVE_ANON]),
                K(pages[LRU_INACTIVE_FILE]),
#ifdef CONFIG_UNEVICTABLE_LRU
		K(pages[LRU_UNEVICTABLE]),
		K(global_page_state(NR_MLOCK)),
#endif
#ifdef CONFIG_HIGHMEM
		K(i.totalhigh),
		K(i.freehigh),
		K(i.totalram-i.totalhigh),
		K(i.freeram-i.freehigh),
#endif
#ifndef CONFIG_MMU
		K((unsigned long) atomic_read(&mmap_pages_allocated)),
#endif
		K(i.totalswap),
		K(i.freeswap),
		K(global_page_state(NR_FILE_DIRTY)),
		K(global_page_state(NR_WRITEBACK)),
		K(global_page_state(NR_ANON_PAGES)),
		K(global_page_state(NR_FILE_MAPPED)),
		K(global_page_state(NR_SLAB_RECLAIMABLE) +
		  global_page_state(NR_SLAB_UNRECLAIMABLE)),
		K(global_page_state(NR_SLAB_RECLAIMABLE)),
		K(global_page_state(NR_SLAB_UNRECLAIMABLE)),
		K(global_page_state(NR_PAGETABLE)),
		K(global_page_state(NR_UNSTABLE_NFS)),
		K(global_page_state(NR_BOUNCE)),
		K(global_page_state(NR_WRITEBACK_TEMP)),
		K(allowed),
		K(committed),
		(unsigned long)VMALLOC_TOTAL >> 10,
		vmi.used >> 10,
		vmi.largest_chunk >> 10
		);
#undef K

	printk(KERN_EMERG "%s", oom_buf);
	return SIGSEGV;
}

int before_oom_kill_task(struct task_struct *target)
{
	return show_meminfo_and_panic(target);
}

void after_oom_kill_task(struct task_struct *p)
{
	return ;
}
