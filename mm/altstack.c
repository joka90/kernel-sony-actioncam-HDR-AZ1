/* 2012-09-06: File added and changed by Sony Corporation */
/*
 *  altstack - An alternate-sized stack memory allocator
 *
 *  Copyright 2011 Sony Corporation
 */
#include <asm/types.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/thread_info.h>
#include <linux/mm.h>
#include <linux/atomic.h>
#include <linux/module.h>

#ifdef CONFIG_STACK_EXTENSIONS
#define MAX_ALT_STACKS		8
#endif

#ifdef CONFIG_SNSC_MAX_ALT_STACKS
/* override stack_extension size */
#define MAX_ALT_STACKS		CONFIG_SNSC_MAX_ALT_STACKS
#endif

/* leave a guard bit at the end */
#define ALT_STACK_MAP_SIZE	((MAX_ALT_STACKS+BITS_PER_LONG) / BITS_PER_LONG)

void *alt_stack_pool_start;
void *alt_stack_pool_end;

EXPORT_SYMBOL(alt_stack_pool_start);
EXPORT_SYMBOL(alt_stack_pool_end);

static unsigned long alt_stack_alloc_map[ALT_STACK_MAP_SIZE];
atomic_t alt_stack_alloc_count = ATOMIC_INIT(0);

#ifdef CONFIG_SNSC_ALT_STACKS_SHOW_MAX_USED
static unsigned long alt_stack_alloc_max;
#endif

static int alt_stack_debug = 0;

unsigned long get_alt_stack_count(void)
{
	return atomic_read(&alt_stack_alloc_count);
}

void dump_alt_stack_info(void)
{
	int i;

	printk("count=%d, map: ", atomic_read(&alt_stack_alloc_count));
	for (i=0; i<ALT_STACK_MAP_SIZE; i++) {
		printk("%08lx ", alt_stack_alloc_map[i]);
	}
	printk("\n");
}

static int __init alt_stack_debug_setup(char *buf)
{
	alt_stack_debug = 1;
	return 0;
}
early_param("alt_stack_debug", alt_stack_debug_setup);

#define asdebug(fmt, args...) ({				\
	if (unlikely(alt_stack_debug))			\
		printk(KERN_INFO			\
			"alt_stack::%s " fmt,		\
			__func__, ## args);		\
})

void __init init_alt_stack_pool(void)
{
	gfp_t mask = GFP_KERNEL;
	int order;
	struct page *page;

	/*
	 * Initially all stacks are free
	 */
	memset(alt_stack_alloc_map, 0, ALT_STACK_MAP_SIZE);

	/* allocate a contiguous block of physical memory for
 	 *  alternate-sized stacks
 	 */
	order = order_base_2(MAX_ALT_STACKS * (ALT_STACK_SIZE/PAGE_SIZE));
	page = alloc_pages(mask, order);
	if (!page) {
		printk(KERN_ERR "Alt-stacks: Could not allocate %d order "
			"pages for alternate stack pool\n", order);
		alt_stack_pool_start = NULL;
		alt_stack_pool_end = NULL;
		return;
	} else {
		alt_stack_pool_start = (void *)page_address(page);
		alt_stack_pool_end = alt_stack_pool_start +
			(ALT_STACK_SIZE<<order);
	}

	asdebug("MAX_ALT_STACKS=%d order=%d start=%p end=%p map=%p "
		"mapsize=%d\n",
		MAX_ALT_STACKS, order, alt_stack_pool_start,
		alt_stack_pool_end, alt_stack_alloc_map,
		ALT_STACK_MAP_SIZE);
}

struct thread_info *alloc_alt_thread_info(gfp_t mask)
{
	unsigned long idx;
	int already_alloced;
#ifdef CONFIG_SNSC_ALT_STACKS_SHOW_MAX_USED
	unsigned long max;
#endif

	if (alt_stack_pool_end==0)
		return NULL;	/* not initialized yet */

	if (atomic_read(&alt_stack_alloc_count) >= MAX_ALT_STACKS) {
		printk_once("Warning: alternate stack pool allocation failed;"
				"using normal kernel stack size.\n");
		return NULL;
	}

	/* set first free bit in map - repeating if we missed due to a race */
	do {
		idx = find_first_zero_bit(alt_stack_alloc_map, MAX_ALT_STACKS);
		if (idx >= MAX_ALT_STACKS) {
			asdebug("out of stack pages\n");
			return NULL;
		}
		already_alloced = test_and_set_bit(idx, alt_stack_alloc_map);
	} while(already_alloced);

	atomic_inc(&alt_stack_alloc_count);

#ifdef CONFIG_SNSC_ALT_STACKS_SHOW_MAX_USED
	/*
	 * since this is just used for tuning,
	 * I don't care about the small chance of a race here
	 */
	max = get_alt_stack_count();
	if (alt_stack_alloc_max < max) {
		alt_stack_alloc_max = max;
		printk("alt_stack: New max alt_stack count=%ld\n",
			alt_stack_alloc_max);
	}
#endif

	asdebug("allocating stack %lu at address %p\n", idx,
		alt_stack_pool_start + idx * ALT_STACK_SIZE);
#ifdef CONFIG_DEBUG_STACK_USAGE
	if (mask & __GFP_ZERO) {
		clear_page(alt_stack_pool_start + idx * ALT_STACK_SIZE);
	}
#endif
	return (struct thread_info *)
		(alt_stack_pool_start + idx * ALT_STACK_SIZE);
}

int free_alt_thread_info(struct thread_info *ti)
{
	unsigned long idx;
	int was_alloced;

	if (((void *)ti) < alt_stack_pool_start ||
	    ((void *)ti) > alt_stack_pool_end) {
		printk("altstack: tried to free wrong ti\n");
		return 0;
	}

	idx = ((void *)ti - alt_stack_pool_start) / ALT_STACK_SIZE;
	asdebug("freeing stack %lu at address %p\n", idx,
		(alt_stack_pool_start + idx * ALT_STACK_SIZE));

	was_alloced = test_and_clear_bit(idx, alt_stack_alloc_map);

	if (unlikely(!was_alloced))
		asdebug("tried to free unallocated stack %p\n", ti);
	else
		atomic_dec(&alt_stack_alloc_count);

	return was_alloced;
}
