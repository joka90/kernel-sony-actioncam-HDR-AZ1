/* 2013-08-20: File changed by Sony Corporation */
#ifdef CONFIG_MMU

/* the upper-most page table pointer */
extern pmd_t *top_pmd;

#define TOP_PTE(x)	pte_offset_kernel(top_pmd, x)

static inline pmd_t *pmd_off_k(unsigned long virt)
{
	return pmd_offset(pud_offset(pgd_offset_k(virt), virt), virt);
}

struct mem_type {
	pteval_t prot_pte;
	unsigned int prot_l1;
	unsigned int prot_sect;
	unsigned int domain;
};

const struct mem_type *get_mem_type(unsigned int type);

extern void __flush_dcache_page(struct address_space *mapping, struct page *page);

struct map_desc;

#ifdef CONFIG_MEMORY_HOTPLUG
extern void __mhp_init create_mapping(struct map_desc *md);
#else
extern void __init create_mapping(struct map_desc *md);
#endif

#endif

#ifdef CONFIG_ZONE_DMA
extern u32 arm_dma_limit;
#else
#define arm_dma_limit ((u32)~0)
#endif

struct pglist_data;

#ifdef CONFIG_SNSC_SUPPORT_4KB_MAPPING
#ifdef CONFIG_MEMORY_HOTPLUG
void __mhp_init change_mapping(struct map_desc *md);
#else
void __init change_mapping(struct map_desc *md);
#endif
#endif
void __init change_sect_into_4k(struct map_desc *md);
void __init change_super_into_sect(struct map_desc *md);
void __init bootmem_init(void);
void arm_mm_memblock_reserve(void);
void reserve_node_zero(struct pglist_data *pgdat);
