/* 2013-08-20: File changed by Sony Corporation */
/*
 * arch/arm/mm/cache-l2x0.c - L210/L220 cache controller support
 *
 * Copyright (C) 2007 ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#ifdef CONFIG_EMXX_L310
#include <mach/hardware.h>
#endif
#if defined (CONFIG_SNSC_SSBOOT) && !defined (CONFIG_WARM_BOOT_IMAGE)
#include <linux/ssboot.h>
#endif
#include <linux/rt_trace_lite.h>
#include <linux/snsc_boot_time.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#ifdef CONFIG_THREAD_MONITOR
#include <mach/tmonitor.h>
#endif /* CONFIG_THREAD_MONITOR */

#define CACHE_LINE_SIZE		32

#undef L2X0_STAT
#ifdef L2X0_STAT
# include <mach/time.h>
#endif /* L2X0_STAT */

#ifdef CONFIG_EJ_PL310_LOCKLESS
# define DECLARE_IRQFLAGS
# define L2X0_LOCK	do { } while (0)
# define L2X0_UNLOCK	do { } while (0)
#else /* CONFIG_EJ_PL310_LOCKLESS */
# define DECLARE_IRQFLAGS	unsigned long flags
# define L2X0_LOCK	raw_spin_lock_irqsave(&l2x0_lock, flags)
# define L2X0_UNLOCK	raw_spin_unlock_irqrestore(&l2x0_lock, flags)
#endif /* CONFIG_EJ_PL310_LOCKLESS */

/*
 * The sysfs interface for cache_lock_length is overkill for an embedded
 * application, but would probably be needed for submittal to mainline.
 * It also allows easy experimentation of various values without recompile
 * and reboot.  If the slight overhead of a variable instead of a constant
 * for CACHE_LOCK_LENGTH is a concern, then change "#if 0" to "#if 1".
 */
#if 0

#define CACHE_LOCK_LENGTH	(unsigned long)CONFIG_CACHE_L2X0_LOCK_LENGTH

#else

#include <linux/kobject.h>
#include <linux/sysfs.h>

static unsigned long cache_lock_length = CONFIG_CACHE_L2X0_LOCK_LENGTH;

#define CACHE_LOCK_LENGTH	cache_lock_length

static struct kobject *l2x0_kobj;


static ssize_t cache_lock_length_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld\n", cache_lock_length);
}

static ssize_t cache_lock_length_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (sscanf(buf, "%du", &value) != 1 ||
	    (value < CACHE_LINE_SIZE || value > 4096 ||
	     value != ((value / CACHE_LINE_SIZE) * CACHE_LINE_SIZE))
	   ) {
		printk(KERN_ERR "cache_lock_length_store: value must be multiple of %d in range %d..4096\n",
		       CACHE_LINE_SIZE, CACHE_LINE_SIZE);
		return -EINVAL;
	}

	/*
	 * This value can change while loops depending on the value are
	 * executing, with no ill effect.  So no lock to protect against
	 * simultaneous access.
	 */
	cache_lock_length = value;

	return count;
}



static struct kobj_attribute l2x0_sysfs_attr =
	__ATTR(cache_lock_length, 0644, cache_lock_length_show,
		cache_lock_length_store);

static int __init l2x0_sysfs_init(void)
{
	int ret;

	l2x0_kobj = kobject_create_and_add("l2x0", kernel_kobj);
	if (!l2x0_kobj)
		return -ENOMEM;

	ret = sysfs_create_file(l2x0_kobj, &l2x0_sysfs_attr.attr);
	if (ret)
		printk(KERN_ERR "l2x0_sysfs_init() subsys_create_file failed: %d\n", ret);

	return ret;
}
arch_initcall(l2x0_sysfs_init);

#endif


static void __iomem *l2x0_base;
#if !defined(CONFIG_EJ_PL310_LOCKLESS)
static DEFINE_RAW_SPINLOCK(l2x0_lock);
#endif /* !CONFIG_EJ_PL310_LOCKLESS */
static uint32_t l2x0_way_mask;	/* Bitmask of active ways */
static uint32_t l2x0_size;
static uint32_t outer_cache_way_size;
static int outer_cache_nways;

static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		;
}

#ifdef CONFIG_CACHE_PL310
static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* cache operations by line are atomic on PL310 */
}
#else
#define cache_wait	cache_wait_way
#endif

#ifdef CONFIG_EMXX_L310
static inline void cache_sync_from_idle(void)
{
	/* base is not yet initialized by l2x0_init(), So directly take the value */
	void __iomem *base = (void *)EMXX_L2CC_VIRT;

	trace_off_mark_l2x0(0x6901, 0);
#ifdef CONFIG_ARM_ERRATA_753970
	/* write to an unmmapped register */
	writel_relaxed(0, base + L2X0_DUMMY_REG);
#else
	writel_relaxed(0, base + L2X0_CACHE_SYNC);
#endif
	cache_wait(base + L2X0_CACHE_SYNC, 1);
	trace_off_mark_l2x0(0x6902, 0);
	dsb();
	trace_off_mark_l2x0(0x6903, 0);
}

void l2x0_cache_sync_idle(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	trace_off_mark_l2x0(0x6900, 0);
	cache_sync_from_idle();
	trace_off_mark_l2x0(0x690f, 0);
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}
#endif

static inline void cache_sync(void)
{
	void __iomem *base = l2x0_base;

#ifdef CONFIG_ARM_ERRATA_753970
	/* write to an unmmapped register */
	writel_relaxed(0, base + L2X0_DUMMY_REG);
#else
	trace_off_mark_l2x0(0x690, 0);
	writel_relaxed(0, base + L2X0_CACHE_SYNC);
#endif
	cache_wait(base + L2X0_CACHE_SYNC, 1);
	trace_off_mark_l2x0(0x691, 0);
	dsb();
	trace_off_mark_l2x0(0x69f, 0);
}

#ifdef CONFIG_EJ_PL310_ATOMIC
#ifdef CONFIG_EJ_PL310_LOCKLESS
#define OUTER_CACHE_LOOP_DIV	1
#else /* CONFIG_EJ_PL310_LOCKLESS */
#define OUTER_CACHE_LOOP_DIV	8
#endif /* CONFIG_EJ_PL310_LOCKLESS */

static void l2x0_all_op(unsigned long reg)
{
	void __iomem *base = l2x0_base;
	__u32 addr, start, end, wu, wend;
	int ways = 0;
	DECLARE_IRQFLAGS;
#ifdef L2X0_STAT
	u32 t1, tmax = 0;
	char msg[64];
#endif /* L2X0_STAT */

	wu = (outer_cache_way_size * SZ_1K) / CACHE_LINE_SIZE / OUTER_CACHE_LOOP_DIV;
	wend = (outer_cache_way_size * SZ_1K) / CACHE_LINE_SIZE;

	while (ways < outer_cache_nways) {
		start = 0;
		addr = ways << 28;
		for (end = wu; end <= wend; end += wu){
			L2X0_LOCK;
#if !defined(CONFIG_EJ_PL310_LOCKLESS)
#ifdef L2X0_STAT
			t1 = mach_read_cycles();
#endif /* L2X0_STAT */
#ifdef CONFIG_THREAD_MONITOR
			tmonitor_write("cstart");
#endif /* CONFIG_THREAD_MONITOR */
#endif /* !CONFIG_EJ_PL310_LOCKLESS */
			while (start < end) {
				writel_relaxed( (addr | (start << 5)),
						base + reg);
				start++;
			}
#if !defined(CONFIG_EJ_PL310_LOCKLESS)
#ifdef CONFIG_THREAD_MONITOR
			tmonitor_write("cend");
#endif /* CONFIG_THREAD_MONITOR */
#ifdef L2X0_STAT
			t1 = mach_read_cycles() - t1;
			if (t1 > tmax)
				tmax = t1;
#endif /* L2X0_STAT */
#endif /* !CONFIG_EJ_PL310_LOCKLESS */
			L2X0_UNLOCK;
		}
		ways++;
	}
#if !defined(CONFIG_EJ_PL310_LOCKLESS)
#ifdef L2X0_STAT
	scnprintf(msg, sizeof msg, "l2x0_all_op:%u", mach_cycles_to_usecs(tmax));
	BOOT_TIME_ADD1(msg);
#endif /* L2X0_STAT */
#endif /* !CONFIG_EJ_PL310_LOCKLESS */
	L2X0_LOCK;
	trace_off_mark_l2x0(0x6110, 0);
	cache_sync();
	trace_off_mark_l2x0(0x611f, 0);
	L2X0_UNLOCK;
}

static void __l2x0_all_op(unsigned long reg)
{
	void __iomem *base = l2x0_base;
	__u32 addr, start, end;
	int ways = 0;

	while (ways < outer_cache_nways) {
		start = 0;
		end = (outer_cache_way_size * SZ_1K) / CACHE_LINE_SIZE;
		addr = ways << 28;
		while (start < end) {
			writel_relaxed( (addr | (start << 5)),
					base + reg);
			start++;
		}
		ways++;
	}
	trace_off_mark_l2x0(0x6100, 0);
	cache_sync();
	trace_off_mark_l2x0(0x610f, 0);
}
#endif /* CONFIG_EJ_PL310_ATOMIC */

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}

#if defined(CONFIG_PL310_ERRATA_588369) || defined(CONFIG_PL310_ERRATA_727915)

#define debug_writel(val)	outer_cache.set_debug(val)

static void l2x0_set_debug(unsigned long val)
{
	writel_relaxed(val, l2x0_base + L2X0_DEBUG_CTRL);
}
#else
/* Optimised out for non-errata case */
static inline void debug_writel(unsigned long val)
{
}

#define l2x0_set_debug	NULL
#endif

#ifdef CONFIG_PL310_ERRATA_588369
static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;

	/* Clean by PA followed by Invalidate by PA */
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}
#else

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_INV_LINE_PA);
}
#endif

static void l2x0_cache_sync(void)
{
	DECLARE_IRQFLAGS;

	debug_1_rt_trace_enter();
	L2X0_LOCK;
	trace_off_mark_l2x0(0x600, 0);
	debug_2_rt_trace_enter();
	cache_sync();
	debug_2_rt_trace_exit();
	trace_off_mark_l2x0(0x6ff, 0);
	L2X0_UNLOCK;
	debug_1_rt_trace_exit();
}

static void __l2x0_flush_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	__l2x0_all_op(L2X0_CLEAN_INV_LINE_IDX);
#else
#ifndef CONFIG_EMXX_L310_NORAM
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
#endif

	trace_off_mark_l2x0(0x610, 0);
	cache_sync();
	trace_off_mark_l2x0(0x61f, 0);
#ifndef CONFIG_EMXX_L310_NORAM
	debug_writel(0x00);
#endif
#endif /* CONFIG_EJ_PL310_ATOMIC */
}

#ifdef CONFIG_PL310_ERRATA_727915_R2P0
static void __l2x0_flush_all_727915_r2p0(void)
{
	void __iomem *base = l2x0_base;
	__u32 addr, start, end;
	int ways = 0;

	while (ways < outer_cache_nways) {
		start = 0;
		end = (outer_cache_way_size * SZ_1K) / CACHE_LINE_SIZE;
		addr = ways << 28;
		while (start < end) {
			writel_relaxed( (addr | (start << 5)) , base + L2X0_CLEAN_INV_LINE_IDX);
			start++;
		}
		ways++;
	}
	cache_sync();
}
#endif

static void l2x0_flush_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	l2x0_all_op(L2X0_CLEAN_INV_LINE_IDX);
#else
	unsigned long flags;

	/* clean all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_PL310_ERRATA_727915_R2P0
	__l2x0_flush_all_727915_r2p0();
#else
	__l2x0_flush_all();
#endif
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
#endif /* CONFIG_EJ_PL310_ATOMIC */
}

static void __l2x0_clean_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	__l2x0_all_op(L2X0_CLEAN_LINE_IDX);
#else
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_WAY, l2x0_way_mask);
	trace_off_mark_l2x0(0x620, 0);
	cache_sync();
	trace_off_mark_l2x0(0x62f, 0);
#endif /* CONFIG_EJ_PL310_ATOMIC */
}

static void l2x0_clean_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	l2x0_all_op(L2X0_CLEAN_LINE_IDX);
#else
	unsigned long flags;

	/* clean all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
#ifndef CONFIG_EMXX_L310_NORAM
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_WAY, l2x0_way_mask);
#endif
	trace_off_mark_l2x0(0x630, 0);
	cache_sync();
	trace_off_mark_l2x0(0x63f, 0);
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
#endif /* CONFIG_EJ_PL310_ATOMIC */
}

static void __l2x0_inv_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	__l2x0_all_op(L2X0_INV_LINE_IDX);
#else
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	trace_off_mark_l2x0(0x640, 0);
	cache_sync();
	trace_off_mark_l2x0(0x64f, 0);
#endif /* CONFIG_EJ_PL310_ATOMIC */
}

static void l2x0_inv_all(void)
{
#ifdef CONFIG_EJ_PL310_ATOMIC
	l2x0_all_op(L2X0_INV_LINE_IDX);
#else
	unsigned long flags;

	/* invalidate all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	/* Invalidating when L2 is enabled is a nono */
	BUG_ON(readl(l2x0_base + L2X0_CTRL) & 1);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	trace_off_mark_l2x0(0x650, 0);
	cache_sync();
	trace_off_mark_l2x0(0x65f, 0);
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
#endif
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
#ifndef CONFIG_EMXX_L310_NORAM
	void __iomem *base = l2x0_base;
#endif
	DECLARE_IRQFLAGS;

#ifdef CONFIG_L2_CACHE_UNALIGN_INV_BACKTRACE
 	if((start & (CACHE_LINE_SIZE -1)) || (end & (CACHE_LINE_SIZE -1))){
 		printk("Unalign Invalidate L2 Cache: "
 		       "start = 0x%lx end = 0x%lx size = %ld\n",
 		       start,end,end-start);
 		__backtrace();
 	}
#endif

	L2X0_LOCK;
#ifndef CONFIG_EMXX_L310_NORAM
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(start);
		debug_writel(0x00);
		start += CACHE_LINE_SIZE;
	}

	if ((end & (CACHE_LINE_SIZE - 1)) && (end > start)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(end);
		debug_writel(0x00);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, CACHE_LOCK_LENGTH);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
#ifdef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
			cache_wait(base + L2X0_INV_LINE_PA, 1);
			trace_off_mark_l2x0(0x660, 1);
			cache_sync();
			trace_off_mark_l2x0(0x66f, 1);
#endif /* CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
			L2X0_UNLOCK;
			L2X0_LOCK;
		}
	}
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	cache_wait(base + L2X0_INV_LINE_PA, 1);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
#endif
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	trace_off_mark_l2x0(0x660, 0);
	cache_sync();
	trace_off_mark_l2x0(0x66f, 0);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
	L2X0_UNLOCK;
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
#ifndef CONFIG_EMXX_L310_NORAM
	void __iomem *base = l2x0_base;
#endif
	DECLARE_IRQFLAGS;

	if ((end - start) >= l2x0_size) {
		l2x0_clean_all();
		return;
	}

	L2X0_LOCK;
#ifndef CONFIG_EMXX_L310_NORAM
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, CACHE_LOCK_LENGTH);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
#ifdef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
			cache_wait(base + L2X0_INV_LINE_PA, 1);
			trace_off_mark_l2x0(0x670, 1);
			cache_sync();
			trace_off_mark_l2x0(0x67f, 1);
#endif /* CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
			L2X0_UNLOCK;
			L2X0_LOCK;
		}
	}
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
#endif
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	trace_off_mark_l2x0(0x670, 0);
	cache_sync();
	trace_off_mark_l2x0(0x67f, 0);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
	L2X0_UNLOCK;
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
#ifndef CONFIG_EMXX_L310_NORAM
	void __iomem *base = l2x0_base;
#endif
	DECLARE_IRQFLAGS;

	if ((end - start) >= l2x0_size) {
		l2x0_flush_all();
		return;
	}

	L2X0_LOCK;
#ifndef CONFIG_EMXX_L310_NORAM
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, CACHE_LOCK_LENGTH);

		debug_writel(0x03);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
		debug_writel(0x00);

		if (blk_end < end) {
#ifdef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
			cache_wait(base + L2X0_INV_LINE_PA, 1);
			trace_off_mark_l2x0(0x680, 1);
			cache_sync();
			trace_off_mark_l2x0(0x68f, 1);
#endif /* CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
			L2X0_UNLOCK;
			L2X0_LOCK;
		}
	}
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
#endif
#ifndef CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC
	trace_off_mark_l2x0(0x680, 0);
	cache_sync();
	trace_off_mark_l2x0(0x68f, 0);
#endif /* !CONFIG_CACHE_L2X0_SPLIT_CACHE_SYNC */
	L2X0_UNLOCK;
}

static void l2x0_disable(void)
{
	DECLARE_IRQFLAGS;

	L2X0_LOCK;
	__l2x0_flush_all();
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
	dsb();
	L2X0_UNLOCK;
}

#if defined(CONFIG_ARCH_EMXX) || defined(CONFIG_SNSC_SSBOOT) && !defined (CONFIG_WARM_BOOT_IMAGE)

#ifdef CONFIG_SNSC_SSBOOT
static __u32 saved_aux_ctrl;
#endif

void l2x0_suspend(void)
{
	if (l2x0_base != NULL) {
		if (readl(l2x0_base + L2X0_CTRL) & 1) {
#ifdef CONFIG_SNSC_SSBOOT
			if (ssboot_is_writing()) {
				/* save AUX_CTRL register */
				saved_aux_ctrl =
					readl(l2x0_base + L2X0_AUX_CTRL);
				return;
			}
 #endif
			/* disable L2X0 */
			flush_cache_all();
			asm("dsb");
			writel(0, l2x0_base + L2X0_CTRL);
			l2x0_clean_all();
		}
	}
}

void l2x0_resume(void)
{
#ifdef CONFIG_SNSC_SSBOOT
	if (ssboot_is_writing())
		return;
#endif

	if (l2x0_base != NULL) {
		if (!(readl(l2x0_base + L2X0_CTRL) & 1)) {
#ifdef CONFIG_SNSC_SSBOOT
			if (ssboot_is_resumed()) {
				writel(saved_aux_ctrl,
				       l2x0_base + L2X0_AUX_CTRL);
			}
#endif
			/* enable L2X0 */
			l2x0_inv_all();
			writel(1, l2x0_base + L2X0_CTRL);
		}
	}
}
#endif

void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 cache_id;
	__u32 way_size = 0;
	int ways;
	const char *type;

	l2x0_base = base;

	cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	rt_debug_1_init_name("l2x0_cache_sync");
	rt_debug_2_init_name("cache_sync");

	/* Determine the number of ways */
	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			ways = 16;
		else
			ways = 8;
		type = "L310";

		/*
		 * Set bit 22 in the auxiliary control register. If this bit
		 * is cleared, PL310 treats Normal Shared Non-cacheable
		 * accesses as Cacheable no-allocate.
		 */
		aux_val |= 1 << 22;
		break;
	case L2X0_CACHE_ID_PART_L210:
		ways = (aux >> 13) & 0xf;
		type = "L210";
		break;
	default:
		/* Assume unknown chips have 8 ways */
		ways = 8;
		type = "L2x0 series";
		break;
	}

	l2x0_way_mask = (1 << ways) - 1;

	/*
	 * L2 cache Size =  Way size * Number of ways
	 */
	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
	way_size = 1 << (way_size + 3);
	l2x0_size = ways * way_size * SZ_1K;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1)) {

		aux &= aux_mask;
		aux |= aux_val;

		/* l2x0 controller is disabled */
		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		l2x0_inv_all();

		/* enable L2X0 */
		writel_relaxed(1, l2x0_base + L2X0_CTRL);
	}

	outer_cache.inv_range = l2x0_inv_range;
	outer_cache.clean_range = l2x0_clean_range;
	outer_cache.flush_range = l2x0_flush_range;
	outer_cache.sync = l2x0_cache_sync;
	outer_cache.flush_all = l2x0_flush_all;
	outer_cache.inv_all = l2x0_inv_all;
	outer_cache.clean_all = l2x0_clean_all;
	outer_cache.disable = l2x0_disable;
	outer_cache.set_debug = l2x0_set_debug;

	outer_cache_nways = ways;
	outer_cache_way_size = way_size;

	/* cache operations without lock */
	outer_cache_unlocked.inv_range = l2x0_inv_range;
	outer_cache_unlocked.clean_range = l2x0_clean_range;
	outer_cache_unlocked.flush_range = l2x0_flush_range;
	outer_cache_unlocked.inv_all = __l2x0_inv_all;
	outer_cache_unlocked.clean_all = __l2x0_clean_all;
	outer_cache_unlocked.flush_all = __l2x0_flush_all;

	printk(KERN_INFO "%s cache controller enabled\n", type);
	printk(KERN_INFO "l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
			ways, cache_id, aux, l2x0_size);
}

void l2x0_info(void)
{
	__u32 dat, attr, addr1, addr2, flag;

	dat = readl(l2x0_base + L2X0_CTRL);
	printk(KERN_INFO "L2 cache controller: %s\n",
	       (dat & L2X0_ENABLE) ? "enabled" : "disabled");

	dat = readl(l2x0_base + L2X0_CACHE_TYPE);
	if (dat & L2X0_CTYPE_LOCKDOWN_MASTER)
		printk(KERN_INFO "\tLockdown by master\n");
	if (dat & L2X0_CTYPE_LOCKDOWN_LINE)
		printk(KERN_INFO "\tLockdown by line\n");

	dat = readl(l2x0_base + L2X0_AUX_CTRL);
	if (dat & (1 << L2X0_AUX_CTRL_FULLZERO_SHIFT))
		printk(KERN_INFO "\tFull line of Zero\n");
	if (dat & (1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT))
		printk(KERN_INFO "\tEarly BRESP\n");
	flag = 0;
	if (dat & (1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT)) {
		printk(KERN_INFO "\tInstruction prefetch\n");
		flag = 1;
	}
	if (dat & (1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT)) {
		printk(KERN_INFO "\tData prefetch\n");
		flag = 1;
	}
	if (flag) {
		attr = readl(l2x0_base + L2X0_PREFETCH_CTRL);
		printk(KERN_INFO "\tPrefetch offset=%d\n", attr);
	}
	if (dat & (1 << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT))
		printk(KERN_INFO "\tNon-secure interrupt access\n");
	if (dat & (1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT))
		printk(KERN_INFO "\tNon-secure lockdown\n");
	attr = (dat >> L2X0_AUX_CTRL_FCWA_SHIFT) & L2X0_AUX_CTRL_FCWA_MASK;
	switch (attr) {
	case 0x0:
		printk(KERN_INFO "\tUse AWCACHE\n");
		break;
	case 0x1:
		printk(KERN_INFO "\tForce No write allocate\n");
		break;
	case 0x2:
		printk(KERN_INFO "\tForce write allocate\n");
		break;
	case 0x3:
		printk(KERN_INFO "\tsame as Use AWCACHE\n");
		break;
	}
	if (dat & (1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT))
		printk(KERN_INFO "\tIgnore shared\n");
	if (dat & (1 << L2X0_AUX_CTRL_PARITY_SHIFT))
		printk(KERN_INFO "\tParity\n");
	if (dat & (1 << L2X0_AUX_CTRL_EVBUS_SHIFT))
		printk(KERN_INFO "\tEvent monitor bus\n");
	attr = (dat & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> L2X0_AUX_CTRL_WAY_SIZE_SHIFT;
	if (attr == 0  ||  attr == 7)
		printk(KERN_INFO "\tWay-size = Unknown\n");
	else
		printk(KERN_INFO "\tWay-size = %dKB\n", 8*(1<<attr));
	printk(KERN_INFO "\t%d-way\n",
	       dat & (1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) ? 16 : 8);
	if (dat & (1 << L2X0_AUX_CTRL_SHINV_SHIFT))
		printk(KERN_INFO "\tShared invalidate\n");
	if (dat & (1 << L2X0_AUX_CTRL_EXCL_SHIFT))
		printk(KERN_INFO "\tExclusive cache\n");
	if (dat & (1 << L2X0_AUX_CTRL_HIPRI_SHIFT))
		printk(KERN_INFO "\tHigh priority for S.O. and Device read\n");

	dat = readl(l2x0_base + L2X0_TAG_LATENCY_CTRL);
	printk(KERN_INFO "\tTag RAM latency = %03x\n", dat);
	dat = readl(l2x0_base + L2X0_DATA_LATENCY_CTRL);
	printk(KERN_INFO "\tData RAM latency = %03x\n", dat);

	dat = readl(l2x0_base + L2X0_FILTER_START);
	if (dat & L2X0_FILTER_ENABLE) {
		addr1 = dat & L2X0_FILTER_MASK;
		dat = readl(l2x0_base + L2X0_FILTER_END);
		addr2 = dat & L2X0_FILTER_MASK;
		printk(KERN_INFO "\tAddress Filter: 0x%08x-0x%08x\n", addr1, addr2);
	}

	dat = readl(l2x0_base + L2X0_DEBUG_CTRL);
	if (dat & L2X0_DEBUG_DWB)
		printk(KERN_INFO "\tForce write through\n");
	if (dat & L2X0_DEBUG_DCL)
		printk(KERN_INFO "\tDisable cache linefills\n");
}
