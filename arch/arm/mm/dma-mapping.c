/* 2013-08-20: File changed by Sony Corporation */
/*
 *  linux/arch/arm/mm/dma-mapping.c
 *
 *  Copyright (C) 2000-2004 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  DMA uncached mapping support.
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_SMP
#include <linux/cpu.h>
#include <linux/hardirq.h>
#include <linux/stop_machine.h>
#endif



#include <asm/memory.h>
#include <asm/highmem.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/sizes.h>

#include "mm.h"

#ifdef CONFIG_EJ_DMA_CACHE_STAT
struct dma_cache_stat dma_cache_stats[DMA_CACHE_STAT_N];
EXPORT_SYMBOL(dma_cache_stats);
#endif /* CONFIG_EJ_DMA_CACHE_STAT */

static u64 get_coherent_dma_mask(struct device *dev)
{
	u64 mask = (u64)arm_dma_limit;

	if (dev) {
		mask = dev->coherent_dma_mask;

		/*
		 * Sanity check the DMA mask - it must be non-zero, and
		 * must be able to be satisfied by a DMA allocation.
		 */
		if (mask == 0) {
			dev_warn(dev, "coherent DMA mask is unset\n");
			return 0;
		}

		if ((~mask) & (u64)arm_dma_limit) {
			dev_warn(dev, "coherent DMA mask %#llx is smaller "
				 "than system GFP_DMA mask %#llx\n",
				 mask, (u64)arm_dma_limit);
			return 0;
		}
	}

	return mask;
}

/*
 * Allocate a DMA buffer for 'dev' of size 'size' using the
 * specified gfp mask.  Note that 'size' must be page aligned.
 */
static struct page *__dma_alloc_buffer(struct device *dev, size_t size, gfp_t gfp)
{
	unsigned long order = get_order(size);
	struct page *page, *p, *e;
	void *ptr;
	u64 mask = get_coherent_dma_mask(dev);

#ifdef CONFIG_DMA_API_DEBUG
	u64 limit = (mask + 1) & ~mask;
	if (limit && size >= limit) {
		dev_warn(dev, "coherent allocation too big (requested %#x mask %#llx)\n",
			size, mask);
		return NULL;
	}
#endif

	if (!mask)
		return NULL;

	if (mask < 0xffffffffULL)
		gfp |= GFP_DMA;

	page = alloc_pages(gfp, order);
	if (!page)
		return NULL;

	/*
	 * Now split the huge page and free the excess pages
	 */
	split_page(page, order);
	for (p = page + (size >> PAGE_SHIFT), e = page + (1 << order); p < e; p++)
		__free_page(p);

	/*
	 * Ensure that the allocated pages are zeroed, and that any data
	 * lurking in the kernel direct-mapped region is invalidated.
	 */
	ptr = page_address(page);
	memset(ptr, 0, size);
	dmac_flush_range(ptr, ptr + size);
	outer_flush_range(__pa(ptr), __pa(ptr) + size);

	return page;
}

/*
 * Free a DMA buffer.  'size' must be page aligned.
 */
static void __dma_free_buffer(struct page *page, size_t size)
{
	struct page *e = page + (size >> PAGE_SHIFT);

	while (page < e) {
		__free_page(page);
		page++;
	}
}

#ifdef CONFIG_MMU
/* Sanity check size */
#if (CONSISTENT_DMA_SIZE % SZ_2M)
#error "CONSISTENT_DMA_SIZE must be multiple of 2MiB"
#endif

#define CONSISTENT_OFFSET(x)	(((unsigned long)(x) - CONSISTENT_BASE) >> PAGE_SHIFT)
#define CONSISTENT_PTE_INDEX(x) (((unsigned long)(x) - CONSISTENT_BASE) >> PGDIR_SHIFT)
#define NUM_CONSISTENT_PTES (CONSISTENT_DMA_SIZE >> PGDIR_SHIFT)

/*
 * These are the page tables (2MB each) covering uncached, DMA consistent allocations
 */
static pte_t *consistent_pte[NUM_CONSISTENT_PTES];

#include "vmregion.h"

static struct arm_vmregion_head consistent_head = {
	.vm_lock	= __SPIN_LOCK_UNLOCKED(&consistent_head.vm_lock),
	.vm_list	= LIST_HEAD_INIT(consistent_head.vm_list),
	.vm_start	= CONSISTENT_BASE,
	.vm_end		= CONSISTENT_END,
};

#ifdef CONFIG_HUGETLB_PAGE
#error ARM Coherent DMA allocator does not (yet) support huge TLB
#endif

/*
 * Initialise the consistent memory allocation.
 */
static int __init consistent_init(void)
{
	int ret = 0;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int i = 0;
	u32 base = CONSISTENT_BASE;

	do {
		pgd = pgd_offset(&init_mm, base);

		pud = pud_alloc(&init_mm, pgd, base);
		if (!pud) {
			printk(KERN_ERR "%s: no pud tables\n", __func__);
			ret = -ENOMEM;
			break;
		}

		pmd = pmd_alloc(&init_mm, pud, base);
		if (!pmd) {
			printk(KERN_ERR "%s: no pmd tables\n", __func__);
			ret = -ENOMEM;
			break;
		}
		WARN_ON(!pmd_none(*pmd));

		pte = pte_alloc_kernel(pmd, base);
		if (!pte) {
			printk(KERN_ERR "%s: no pte tables\n", __func__);
			ret = -ENOMEM;
			break;
		}

		consistent_pte[i++] = pte;
		base += (1 << PGDIR_SHIFT);
	} while (base < CONSISTENT_END);

	return ret;
}

core_initcall(consistent_init);

static void *
__dma_alloc_remap(struct page *page, size_t size, gfp_t gfp, pgprot_t prot)
{
	struct arm_vmregion *c;
	size_t align;
	int bit;

	if (!consistent_pte[0]) {
		printk(KERN_ERR "%s: not initialised\n", __func__);
		dump_stack();
		return NULL;
	}

	/*
	 * Align the virtual region allocation - maximum alignment is
	 * a section size, minimum is a page size.  This helps reduce
	 * fragmentation of the DMA space, and also prevents allocations
	 * smaller than a section from crossing a section boundary.
	 */
	bit = fls(size - 1);
	if (bit > SECTION_SHIFT)
		bit = SECTION_SHIFT;
	align = 1 << bit;

	/*
	 * Allocate a virtual address in the consistent mapping region.
	 */
	c = arm_vmregion_alloc(&consistent_head, align, size,
			    gfp & ~(__GFP_DMA | __GFP_HIGHMEM));
	if (c) {
		pte_t *pte;
		int idx = CONSISTENT_PTE_INDEX(c->vm_start);
		u32 off = CONSISTENT_OFFSET(c->vm_start) & (PTRS_PER_PTE-1);

		pte = consistent_pte[idx] + off;
		c->vm_pages = page;

		do {
			BUG_ON(!pte_none(*pte));

			set_pte_ext(pte, mk_pte(page, prot), 0);
			page++;
			pte++;
			off++;
			if (off >= PTRS_PER_PTE) {
				off = 0;
				pte = consistent_pte[++idx];
			}
		} while (size -= PAGE_SIZE);

		dsb();

		return (void *)c->vm_start;
	}
	return NULL;
}

static void __dma_free_remap(void *cpu_addr, size_t size)
{
	struct arm_vmregion *c;
	unsigned long addr;
	pte_t *ptep;
	int idx;
	u32 off;

	c = arm_vmregion_find_remove(&consistent_head, (unsigned long)cpu_addr);
	if (!c) {
		printk(KERN_ERR "%s: trying to free invalid coherent area: %p\n",
		       __func__, cpu_addr);
		dump_stack();
		return;
	}

	if ((c->vm_end - c->vm_start) != size) {
		printk(KERN_ERR "%s: freeing wrong coherent size (%ld != %d)\n",
		       __func__, c->vm_end - c->vm_start, size);
		dump_stack();
		size = c->vm_end - c->vm_start;
	}

	idx = CONSISTENT_PTE_INDEX(c->vm_start);
	off = CONSISTENT_OFFSET(c->vm_start) & (PTRS_PER_PTE-1);
	ptep = consistent_pte[idx] + off;
	addr = c->vm_start;
	do {
		pte_t pte = ptep_get_and_clear(&init_mm, addr, ptep);

		ptep++;
		addr += PAGE_SIZE;
		off++;
		if (off >= PTRS_PER_PTE) {
			off = 0;
			ptep = consistent_pte[++idx];
		}

		if (pte_none(pte) || !pte_present(pte))
			printk(KERN_CRIT "%s: bad page in kernel page table\n",
			       __func__);
	} while (size -= PAGE_SIZE);

	flush_tlb_kernel_range(c->vm_start, c->vm_end);

	arm_vmregion_free(&consistent_head, c);
}

#else	/* !CONFIG_MMU */

#define __dma_alloc_remap(page, size, gfp, prot)	page_address(page)
#define __dma_free_remap(addr, size)			do { } while (0)

#endif	/* CONFIG_MMU */

static void *
__dma_alloc(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp,
	    pgprot_t prot)
{
	struct page *page;
	void *addr;

	*handle = ~0;
	size = PAGE_ALIGN(size);

	page = __dma_alloc_buffer(dev, size, gfp);
	if (!page)
		return NULL;

	if (!arch_is_coherent())
		addr = __dma_alloc_remap(page, size, gfp, prot);
	else
		addr = page_address(page);

	if (addr)
		*handle = pfn_to_dma(dev, page_to_pfn(page));
	else
		__dma_free_buffer(page, size);

	return addr;
}

/*
 * Allocate DMA-coherent memory space and return both the kernel remapped
 * virtual and bus address for that space.
 */
void *
dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
{
	void *memory;

	if (dma_alloc_from_coherent(dev, size, handle, &memory))
		return memory;

	return __dma_alloc(dev, size, handle, gfp,
			   pgprot_dmacoherent(pgprot_kernel));
}
EXPORT_SYMBOL(dma_alloc_coherent);

/*
 * Allocate a writecombining region, in much the same way as
 * dma_alloc_coherent above.
 */
void *
dma_alloc_writecombine(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
{
	return __dma_alloc(dev, size, handle, gfp,
			   pgprot_writecombine(pgprot_kernel));
}
EXPORT_SYMBOL(dma_alloc_writecombine);

static int dma_mmap(struct device *dev, struct vm_area_struct *vma,
		    void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	int ret = -ENXIO;
#ifdef CONFIG_MMU
	unsigned long user_size, kern_size;
	struct arm_vmregion *c;

	user_size = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	c = arm_vmregion_find(&consistent_head, (unsigned long)cpu_addr);
	if (c) {
		unsigned long off = vma->vm_pgoff;

		kern_size = (c->vm_end - c->vm_start) >> PAGE_SHIFT;

		if (off < kern_size &&
		    user_size <= (kern_size - off)) {
			ret = remap_pfn_range(vma, vma->vm_start,
					      page_to_pfn(c->vm_pages) + off,
					      user_size << PAGE_SHIFT,
					      vma->vm_page_prot);
		}
	}
#endif	/* CONFIG_MMU */

	return ret;
}

int dma_mmap_coherent(struct device *dev, struct vm_area_struct *vma,
		      void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	vma->vm_page_prot = pgprot_dmacoherent(vma->vm_page_prot);
	return dma_mmap(dev, vma, cpu_addr, dma_addr, size);
}
EXPORT_SYMBOL(dma_mmap_coherent);

int dma_mmap_writecombine(struct device *dev, struct vm_area_struct *vma,
			  void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	return dma_mmap(dev, vma, cpu_addr, dma_addr, size);
}
EXPORT_SYMBOL(dma_mmap_writecombine);

/*
 * free a page as defined by the above mapping.
 * Must not be called with IRQs disabled.
 */
void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle)
{
	WARN_ON(irqs_disabled());

	if (dma_release_from_coherent(dev, get_order(size), cpu_addr))
		return;

	size = PAGE_ALIGN(size);

	if (!arch_is_coherent())
		__dma_free_remap(cpu_addr, size);

	__dma_free_buffer(pfn_to_page(dma_to_pfn(dev, handle)), size);
}
EXPORT_SYMBOL(dma_free_coherent);

#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
static inline int is_outer_cacheable(const void *kaddr)
{
	unsigned long flags;
	unsigned long par;

	/*
	 * local_irq_save() instaed of spin_lock_irqsave() is enough
	 * here, because V2PCWPR and PAR are per-CPU register.
	 */
	local_irq_save(flags);

	/* V2PCWPR */
	asm volatile ( "mcr p15, 0, %0, c7, c8, 0" : : "r"(kaddr) );

	/*
	 * "isb" is required so that any change to CP15 registers is
	 * guaranteed to be visible to subsequent instructions. See
	 * ARMARM B3.12.4 for details.
	 */
	asm volatile ( "isb" );

	/* PAR */
	asm volatile ( "mrc p15, 0, %0, c7, c4, 0" : "=r"(par) );

	local_irq_restore(flags);

	/* For saftey, if translation failed, we do outer cache operations */
	if (par & 1)
		return 1;

	/* PAR[3:2] == 0 is outer noncacheable, otherwise something cacheable */
	return (par >> 2) & 0x3;
}

static inline int is_outer_cacheable_page(struct page *page)
{
	/* We always treat HighMem as outer cacheable */
	if (PageHighMem(page))
		return 1;

	return is_outer_cacheable(page_address(page));
}
#endif

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
/*
 * The threshold of the size to flush entire L2 cache.
 */
static unsigned long outer_cache_threshold = ~0UL;
module_param_named(l2th, outer_cache_threshold, ulong, S_IRUSR|S_IWUSR);

static void __outer_sync_all(enum dma_data_direction direction)
{
#ifdef CONFIG_OUTER_CACHE
	switch (direction) {
	case DMA_TO_DEVICE:		/* writeback only */
		outer_clean_all();
		break;
	case DMA_BIDIRECTIONAL:		/* writeback and invalidate */
	case DMA_FROM_DEVICE:
		outer_flush_all();
		break;
	default:
		BUG();
	}
#endif /* CONFIG_OUTER_CACHE */
}
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */

/*
 * Make an area consistent for devices.
 * Note: Drivers should NOT use this function directly, as it will break
 * platforms with CONFIG_DMABOUNCE.
 * Use the driver DMA support - see dma-mapping.h (dma_sync_*)
 */
void ___dma_single_cpu_to_dev(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	unsigned long paddr;

	BUG_ON(!virt_addr_valid(kaddr) || !virt_addr_valid(kaddr + size - 1));

	dmac_map_area(kaddr, size, dir);

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
#ifdef CONFIG_EJ_DMA_CACHE_STAT
	DMA_CACHE_STAT_INC(6, size);
#endif
	if (size > outer_cache_threshold) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(7, size);
#endif
		__outer_sync_all(dir);
		return;
	}
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
	if (is_outer_cacheable(kaddr)) {
#endif
	paddr = __pa(kaddr);
	if (dir == DMA_FROM_DEVICE) {
		outer_inv_range(paddr, paddr + size);
	} else {
		outer_clean_range(paddr, paddr + size);
	}
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
	} else { outer_sync(); }
#endif
	/* FIXME: non-speculating: flush on bidirectional mappings? */
}
EXPORT_SYMBOL(___dma_single_cpu_to_dev);

void ___dma_single_dev_to_cpu(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	BUG_ON(!virt_addr_valid(kaddr) || !virt_addr_valid(kaddr + size - 1));

	/* FIXME: non-speculating: not required */
	/* don't bother invalidating if DMA to device */
	if (dir != DMA_TO_DEVICE) {
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
		if (is_outer_cacheable(kaddr)) {
#endif
		unsigned long paddr = __pa(kaddr);
		outer_inv_range(paddr, paddr + size);
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
		} else { outer_sync(); }
#endif
	}

	dmac_unmap_area(kaddr, size, dir);
}
EXPORT_SYMBOL(___dma_single_dev_to_cpu);

static void dma_cache_maint_page(struct page *page, unsigned long offset,
	size_t size, enum dma_data_direction dir,
	void (*op)(const void *, size_t, int))
{
	unsigned long pfn;
	size_t left = size;

	pfn = page_to_pfn(page) + offset / PAGE_SIZE;
	offset %= PAGE_SIZE;

	/*
	 * A single sg entry may refer to multiple physically contiguous
	 * pages.  But we still need to process highmem pages individually.
	 * If highmem is not configured then the bulk of this loop gets
	 * optimized out.
	 */
	do {
		size_t len = left;
		void *vaddr;

		page = pfn_to_page(pfn);

		if (PageHighMem(page)) {
			if (len + offset > PAGE_SIZE)
				len = PAGE_SIZE - offset;
			vaddr = kmap_high_get(page);
			if (vaddr) {
				vaddr += offset;
				op(vaddr, len, dir);
				kunmap_high(page);
			} else if (cache_is_vipt()) {
				/* unmapped pages might still be cached */
				vaddr = kmap_atomic(page);
				op(vaddr + offset, len, dir);
				kunmap_atomic(vaddr);
			}
		} else {
			vaddr = page_address(page) + offset;
			op(vaddr, len, dir);
		}
		offset = 0;
		pfn++;
		left -= len;
	} while (left);
}
#if defined(CONFIG_EJ_FLUSH_ENTIRE_L2CACHE) && defined(CONFIG_EJ_DMA_MAP_SG)
static void __dma_page_cpu_to_dev_inner(struct page *page, unsigned long off,
					size_t size, enum dma_data_direction dir)
{
	if (arch_is_coherent())
		return;
#ifdef CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE
#ifdef CONFIG_EJ_DMA_CACHE_STAT
	DMA_CACHE_STAT_INC(0, size);
#endif
	if (PageReserved(page) && PageUncached(page)) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(1, size);
#endif
		return;
	}
#endif
	dma_cache_maint_page(page, off, size, dir, dmac_map_area);
}
#endif
void ___dma_page_cpu_to_dev(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	unsigned long paddr;
#ifdef CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE
#ifdef CONFIG_EJ_DMA_CACHE_STAT
	DMA_CACHE_STAT_INC(2, size);
#endif
	if (PageReserved(page) && PageUncached(page)) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(3, size);
#endif
		return;
	}
#endif
	dma_cache_maint_page(page, off, size, dir, dmac_map_area);

#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
	if (is_outer_cacheable_page(page)) {
#endif
	paddr = page_to_phys(page) + off;
	if (dir == DMA_FROM_DEVICE) {
		outer_inv_range(paddr, paddr + size);
	} else {
		outer_clean_range(paddr, paddr + size);
	}
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
	} else { outer_sync(); }
#endif
	/* FIXME: non-speculating: flush on bidirectional mappings? */
}
EXPORT_SYMBOL(___dma_page_cpu_to_dev);

void ___dma_page_dev_to_cpu(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	unsigned long paddr = page_to_phys(page) + off;

	/* FIXME: non-speculating: not required */
	/* don't bother invalidating if DMA to device */
	if (dir != DMA_TO_DEVICE) {
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
		if (is_outer_cacheable_page(page)) {
#endif
		outer_inv_range(paddr, paddr + size);
#ifdef CONFIG_SNSC_ARM_DMA_SKIP_OUTER_CACHE_OPS_FOR_OUTER_NONCACHEABLE_REGION
		} else { outer_sync(); }
#endif
	}

#ifdef CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE
#ifdef CONFIG_EJ_DMA_CACHE_STAT
	DMA_CACHE_STAT_INC(4, size);
#endif
	if (PageReserved(page) && PageUncached(page)) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(5, size);
#endif
	} else
#endif
	dma_cache_maint_page(page, off, size, dir, dmac_unmap_area);

	/*
	 * Mark the D-cache clean for this page to avoid extra flushing.
	 */
	if (dir != DMA_TO_DEVICE && off == 0 && size >= PAGE_SIZE)
		set_bit(PG_dcache_clean, &page->flags);
}
EXPORT_SYMBOL(___dma_page_dev_to_cpu);

/**
 * dma_map_sg - map a set of SG buffers for streaming mode DMA
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map
 * @dir: DMA transfer direction
 *
 * Map a set of buffers described by scatterlist in streaming mode for DMA.
 * This is the scatter-gather version of the dma_map_single interface.
 * Here the scatter gather list elements are each tagged with the
 * appropriate dma address and length.  They are obtained via
 * sg_dma_{address,length}.
 *
 * Device ownership issues as mentioned for dma_map_single are the same
 * here.
 */
#ifdef CONFIG_EJ_DMA_MAP_SG
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;
#ifdef CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE
	int all_uncached = 1;
#else
	int all_uncached = 0;
#endif
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	unsigned long size = 0;
#endif

	BUG_ON(!valid_dma_direction(dir));

	for_each_sg(sg, s, nents, i) {
		struct page *page = sg_page(s);
		s->dma_address = pfn_to_dma(dev, page_to_pfn(page)) + s->offset;
#ifdef CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE
		if (!(PageReserved(page) && PageUncached(page))) {
# ifdef CONFIG_PAGECACHE_PREALLOC
			if (!PageNotOnCache(page) || dir != DMA_FROM_DEVICE ) {
				all_uncached = 0;
			}
# else
			all_uncached = 0;
# endif
		}
#endif /* CONFIG_EJ_DONT_FLUSH_UNCACHED_PAGE */
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
		size += sg->length;
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */
	}

	if (arch_is_coherent() || all_uncached) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(10, size);
#endif
		goto end;
	}

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
#ifdef CONFIG_EJ_DMA_CACHE_STAT
	DMA_CACHE_STAT_INC(8, size);
#endif
	if (size > outer_cache_threshold) {
#ifdef CONFIG_EJ_DMA_CACHE_STAT
		DMA_CACHE_STAT_INC(9, size);
#endif
		for_each_sg(sg, s, nents, i) {
			__dma_page_cpu_to_dev_inner(sg_page(s), s->offset,
						    s->length, dir);
		}
		__outer_sync_all(dir);
		goto end;
	}
#endif
	for_each_sg(sg, s, nents, i) {
		__dma_page_cpu_to_dev(sg_page(s), s->offset, s->length, dir);
	}
end:
	debug_dma_map_sg(dev, sg, nents, nents, dir);
	return nents;
}
#else /* CONFIG_EJ_DMA_MAP_SG */
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i, j;

	BUG_ON(!valid_dma_direction(dir));

	for_each_sg(sg, s, nents, i) {
		s->dma_address = __dma_map_page(dev, sg_page(s), s->offset,
						s->length, dir);
		if (dma_mapping_error(dev, s->dma_address))
			goto bad_mapping;
	}
	debug_dma_map_sg(dev, sg, nents, nents, dir);
	return nents;

 bad_mapping:
	for_each_sg(sg, s, i, j)
		__dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
	return 0;
}
#endif /* CONFIG_EJ_DMA_MAP_SG */
EXPORT_SYMBOL(dma_map_sg);

/**
 * dma_unmap_sg - unmap a set of SG buffers mapped by dma_map_sg
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to unmap (same as was passed to dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 *
 * Unmap a set of streaming mode DMA translations.  Again, CPU access
 * rules concerning calls here are the same as for dma_unmap_single().
 */
void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	debug_dma_unmap_sg(dev, sg, nents, dir);

	for_each_sg(sg, s, nents, i)
		__dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
}
EXPORT_SYMBOL(dma_unmap_sg);

/**
 * dma_sync_sg_for_cpu
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map (returned from dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 */
void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (!dmabounce_sync_for_cpu(dev, sg_dma_address(s), 0,
					    sg_dma_len(s), dir))
			continue;

		__dma_page_dev_to_cpu(sg_page(s), s->offset,
				      s->length, dir);
	}

	debug_dma_sync_sg_for_cpu(dev, sg, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_cpu);

/**
 * dma_sync_sg_for_device
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map (returned from dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 */
void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (!dmabounce_sync_for_device(dev, sg_dma_address(s), 0,
					sg_dma_len(s), dir))
			continue;

		__dma_page_cpu_to_dev(sg_page(s), s->offset,
				      s->length, dir);
	}

	debug_dma_sync_sg_for_device(dev, sg, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_device);

#ifdef CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_SMP

struct dma_dcache_all_flags {
	unsigned int ready;
	unsigned int complete;
};

struct dma_dcache_all_priv {
	struct dma_dcache_all_flags *flags;
	void (*func)(unsigned int cpu,
		     unsigned int *ready, unsigned int *complete);
};

static struct dma_dcache_all_flags *dma_dcache_all_flags;
static DEFINE_MUTEX(dma_dcache_all_mutex);
static unsigned int dma_dcache_all_threshold;
EXPORT_SYMBOL(dma_dcache_all_threshold);

static int dma_dcache_all_handler(void *ptr)
{
	struct dma_dcache_all_priv *priv = ptr;

	priv->func(raw_smp_processor_id(),
		   &priv->flags->ready, &priv->flags->complete);

	return 0;
}

/**
 * dma_dcache_all
 * @start: virtual start address of region
 * @end: virtual end address of region
 * @dcache_size: pointer to actual dcache size
 * @func: pointer to function to do actual dcache ops
 */
int dma_dcache_all(unsigned long start, unsigned long end,
		   unsigned int *dcache_size,
		   void (*func)(unsigned int, unsigned int*, unsigned int*))
{
	int ret;
	unsigned int cpu;
	unsigned int threshold = dma_dcache_all_threshold;
	struct dma_dcache_all_priv priv;

	/* check to see if initialized */
	if (unlikely(!dma_dcache_all_flags))
		return 0;

	/* check to see if can sleep */
	if (in_atomic() || irqs_disabled())
		return 0;

	/* determine threshold */
	if (!threshold && dcache_size)
		threshold = *dcache_size;

	/* check to see if should do dcache all ops */
	if ((end - start) < threshold)
		return 0;

	/* acquire lock and cpus */
	mutex_lock(&dma_dcache_all_mutex);

	get_online_cpus();

	/* setup flags */
	dma_dcache_all_flags->ready    = 0;
	dma_dcache_all_flags->complete = 0;

	for_each_online_cpu(cpu) {
		BUG_ON(cpu >= sizeof(unsigned int));
		*((u_int8_t*)(&dma_dcache_all_flags->ready)    + cpu) = 1;
		*((u_int8_t*)(&dma_dcache_all_flags->complete) + cpu) = 1;
	}

	/* do dcache all ops on each online cpu */
	priv.flags = dma_dcache_all_flags;
	priv.func  = func;

	ret = stop_machine(dma_dcache_all_handler, &priv, &cpu_online_map);

	/* free lock and cpus */
	put_online_cpus();

	mutex_unlock(&dma_dcache_all_mutex);

	return !ret ? 1 : 0;
}

static int __init dma_dcache_all_init(void)
{
	dma_addr_t dummy;

	/* allocate struct dma_dcache_all_flags */
	dma_dcache_all_flags = dma_alloc_coherent(NULL,
		sizeof(struct dma_dcache_all_flags), &dummy, GFP_KERNEL);
	if (!dma_dcache_all_flags)
		return -ENOMEM;

	/* initialize threshold */
	dma_dcache_all_threshold =
		CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_THRESHOLD;

	pr_info("dma_dcache_all_threshold = %u\n", dma_dcache_all_threshold);

	return 0;
}
arch_initcall(dma_dcache_all_init);
#endif /* CONFIG_SNSC_ARM_DMA_REDUCE_CACHE_OPS_SMP */

/*
 * Return whether the given device DMA address mask can be supported
 * properly.  For example, if your device can only drive the low 24-bits
 * during bus mastering, then you would pass 0x00ffffff as the mask
 * to this function.
 */
int dma_supported(struct device *dev, u64 mask)
{
	if (mask < (u64)arm_dma_limit)
		return 0;
	return 1;
}
EXPORT_SYMBOL(dma_supported);

int dma_set_mask(struct device *dev, u64 dma_mask)
{
	if (!dev->dma_mask || !dma_supported(dev, dma_mask))
		return -EIO;

#ifndef CONFIG_DMABOUNCE
	*dev->dma_mask = dma_mask;
#endif

	return 0;
}
EXPORT_SYMBOL(dma_set_mask);

#define PREALLOC_DMA_DEBUG_ENTRIES	4096

static int __init dma_debug_do_init(void)
{
	dma_debug_init(PREALLOC_DMA_DEBUG_ENTRIES);
	return 0;
}
fs_initcall(dma_debug_do_init);
