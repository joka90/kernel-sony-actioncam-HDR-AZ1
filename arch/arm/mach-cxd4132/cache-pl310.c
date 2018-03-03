/*
 * arch/arm/mm/cache-pl310.c
 *
 * PL310 cache controller support
 *
 * Copyright 2009,2010,2011 Sony Corporation
 *
 * This code is based on arch/arm/mm/cache-l2x0.c.
 */
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

#include <asm/cacheflush.h>
#include <mach/cache-pl310.h>
#include <asm/cache.h>

#define CACHE_LINE_SIZE		L2_CACHE_BYTES

static inline void cache_sync(void)
{
	writel(0, PL310_REG(L2X0_CACHE_SYNC)); /* atomic cache operation */
	dsb();
}

static u32 pl310_waymask;

#ifdef CONFIG_PREEMPT_RT
static DEFINE_RAW_SPINLOCK(pl310_lock);
#else
static DEFINE_SPINLOCK(pl310_lock);
#endif /* CONFIG_PREEMPT_RT */

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
static void __pl310_all_op(const void __iomem __force *reg)
{
	u32 dat;

	writel(pl310_waymask, reg); /* background op. */
	do {
		dat = readl(reg);
	} while (dat & pl310_waymask);
	cache_sync();
}

static void pl310_all_op(const void __iomem __force *reg)
{
	u32 wmask;

	for (wmask=0x0001; wmask & pl310_waymask; wmask <<= 1) {
		unsigned long flags, dat;
		spin_lock_irqsave(&pl310_lock, flags);
		writel(wmask, reg); /* background op. */
		do {
			dat = readl(reg);
		} while (dat & pl310_waymask);
		spin_unlock_irqrestore(&pl310_lock, flags);
	}
	cache_sync();
}

static void __pl310_inv_all(void)
{
	__pl310_all_op(PL310_REG(L2X0_INV_WAY));
}

static void pl310_inv_all(void)
{
	pl310_all_op(PL310_REG(L2X0_INV_WAY));
}

static void __pl310_clean_all(void)
{
	__pl310_all_op(PL310_REG(L2X0_CLEAN_WAY));
}

static void pl310_clean_all(void)
{
	pl310_all_op(PL310_REG(L2X0_CLEAN_WAY));
}

static void __pl310_flush_all(void)
{
	__pl310_all_op(PL310_REG(L2X0_CLEAN_INV_WAY));
}

static void pl310_flush_all(void)
{
	pl310_all_op(PL310_REG(L2X0_CLEAN_INV_WAY));
}
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */

void pl310_lockdown(uint *mask, int n)
{
	unsigned long flags;
	u32 dat;
	int i;

	spin_lock_irqsave(&pl310_lock, flags);
	/* diable write back and line fill */
	writel(0x3, PL310_REG(L2X0_DEBUG_CTRL));
	/* clean and invalidate */
	writel(pl310_waymask, PL310_REG(L2X0_CLEAN_INV_WAY)); /* BG op.*/
	do {
		dat = readl(PL310_REG(L2X0_CLEAN_INV_WAY));
	} while (dat & pl310_waymask);

	/* lockdown */
	for (i = 0; i < n; i++, mask++) {
		writel(*mask, PL310_REG(PL310_LOCKDOWN_WAY_D(i)));
		writel(*mask, PL310_REG(PL310_LOCKDOWN_WAY_I(i)));
	}

	/* enable write back and line fill */
	writel(0x0, PL310_REG(L2X0_DEBUG_CTRL));
	cache_sync();
	spin_unlock_irqrestore(&pl310_lock, flags);
}


static void pl310_inv_range(unsigned long start, unsigned long end)
{
	unsigned long addr;

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	unsigned long flags;
	spin_lock_irqsave(&pl310_lock, flags);
#endif
#ifdef CONFIG_L2_CACHE_UNALIGN_INV_BACKTRACE
 	if((start & (CACHE_LINE_SIZE -1)) || (end & (CACHE_LINE_SIZE -1))){
 		printk("Unalign Invalidate L2 Cache: "
 		       "start = 0x%lx end = 0x%lx size = %ld\n",
 		       start,end,end-start);
 		__backtrace();
 	}
#endif

	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		writel(start, PL310_REG(L2X0_CLEAN_INV_LINE_PA)); /* atomic cache operation */
		start += CACHE_LINE_SIZE;
	}

	if ((end & (CACHE_LINE_SIZE - 1)) && (end > start)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		writel(end, PL310_REG(L2X0_CLEAN_INV_LINE_PA)); /* atomic cache operation */
	}

	for (addr = start; addr < end; addr += CACHE_LINE_SIZE) {
		writel(addr, PL310_REG(L2X0_INV_LINE_PA)); /* atomic cache operation */
	}
	cache_sync();
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	spin_unlock_irqrestore(&pl310_lock, flags);
#endif
}

static void pl310_clean_range(unsigned long start, unsigned long end)
{
	unsigned long addr;

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	unsigned long flags;
	spin_lock_irqsave(&pl310_lock, flags);
#endif
	start &= ~(CACHE_LINE_SIZE - 1);
	for (addr = start; addr < end; addr += CACHE_LINE_SIZE) {
		writel(addr, PL310_REG(L2X0_CLEAN_LINE_PA));
	}
	cache_sync();
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	spin_unlock_irqrestore(&pl310_lock, flags);
#endif
}

static void pl310_flush_range(unsigned long start, unsigned long end)
{
	unsigned long addr;

#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	unsigned long flags;
	spin_lock_irqsave(&pl310_lock, flags);
#endif
	start &= ~(CACHE_LINE_SIZE - 1);
	for (addr = start; addr < end; addr += CACHE_LINE_SIZE) {
		writel(addr, PL310_REG(L2X0_CLEAN_INV_LINE_PA));
	}
	cache_sync();
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	spin_unlock_irqrestore(&pl310_lock, flags);
#endif
}

void pl310_info(void)
{
	__u32 dat, attr, addr1, addr2;

	dat = readl(PL310_REG(L2X0_CACHE_ID));
	if ((dat & PL310_CACHE_ID_MASK) != PL310_CACHE_ID_VAL) {
		printk(KERN_ERR "ERROR: pl310_info: can not find PL310: CACHE_ID=0x%08x\n", dat);
		return;
	}

	dat = readl(PL310_REG(L2X0_CTRL));
	printk(KERN_INFO "PL310 cache controller: %s\n",
	       (dat & L2X0_ENABLE) ? "enabled" : "disabled");

	dat = readl(PL310_REG(L2X0_CACHE_TYPE));
	if (dat & PL310_CTYPE_LOCKDOWN_MASTER)
		printk(KERN_INFO "\tLockdown by master\n");
	if (dat & PL310_CTYPE_LOCKDOWN_LINE)
		printk(KERN_INFO "\tLockdown by line\n");

	dat = readl(PL310_REG(L2X0_AUX_CTRL));
	if (dat & PL310_AUX_FULLZERO)
		printk(KERN_INFO "\tFull line of Zero\n");
	if (dat & PL310_AUX_BRESP)
		printk(KERN_INFO "\tEarly BRESP\n");
	if (dat & PL310_AUX_I_PREFETCH)
		printk(KERN_INFO "\tInstruction prefetch\n");
	if (dat & PL310_AUX_D_PREFETCH)
		printk(KERN_INFO "\tData prefetch\n");
	if (dat & (PL310_AUX_I_PREFETCH|PL310_AUX_D_PREFETCH)) {
		attr = readl(PL310_REG(PL310_PREFETCH_OFFSET));
		printk(KERN_INFO "\tPrefetch offset=%d\n", attr);
	}
	if (dat & PL310_AUX_NS_INTR)
		printk(KERN_INFO "\tNon-secure interrupt access\n");
	if (dat & PL310_AUX_NS_LOCK)
		printk(KERN_INFO "\tNon-secure lockdown\n");
	attr = (dat >> PL310_AUX_FC_WA_SHIFT) & PL310_AUX_FC_WA_MASK;
	switch (attr) {
	case 0x1:
		printk(KERN_INFO "\tForce No write allocate\n");
		break;
	case 0x2:
		printk(KERN_INFO "\tForce write allocate\n");
		break;
	}
	if (dat & PL310_AUX_IGN_SH)
		printk(KERN_INFO "\tIgnore shared\n");
	if (dat & PL310_AUX_PARITY)
		printk(KERN_INFO "\tParity\n");
	if (dat & PL310_AUX_EVBUS)
		printk(KERN_INFO "\tEvent monitor bus\n");
	attr = (dat >> PL310_AUX_WAYSIZE_SHIFT) & PL310_AUX_WAYSIZE_MASK;
	if (attr == 0  ||  attr == 7)
		printk(KERN_INFO "\tWay-size = Unknown\n");
	else
		printk(KERN_INFO "\tWay-size = %dKB\n", 8*(1<<attr));
	printk(KERN_INFO "\t%d-way\n", dat & PL310_AUX_WAY ? 16 : 8);
	if (dat & PL310_AUX_SHINV)
		printk(KERN_INFO "\tShared invalidate\n");
	if (dat & PL310_AUX_EX)
		printk(KERN_INFO "\tExclusive cache\n");
	if (dat & PL310_AUX_SOHI)
		printk(KERN_INFO "\tHigh priority for S.O. and Device read\n");

	dat = readl(PL310_REG(PL310_TAGRAM_CTRL));
	printk(KERN_INFO "\tTag RAM latency = %03x\n", dat);
	dat = readl(PL310_REG(PL310_DATARAM_CTRL));
	printk(KERN_INFO "\tData RAM latency = %03x\n", dat);

	dat = readl(PL310_REG(PL310_FILTER_START));
	if (dat & PL310_FILTER_ENABLE) {

		addr1 = dat & PL310_FILTER_MASK;
		dat = readl(PL310_REG(PL310_FILTER_END));
		addr2 = dat & PL310_FILTER_MASK;
		printk(KERN_INFO "\tAddress Filter: 0x%08x-0x%08x\n", addr1, addr2);
	}

	dat = readl(PL310_REG(L2X0_DEBUG_CTRL));
	if (dat & PL310_DEBUG_DWB)
		printk(KERN_INFO "\tForce write through\n");
	if (dat & PL310_DEBUG_DCL)
		printk(KERN_INFO "\tDisable cache linefills\n");
}

void __init pl310_setup(void)
{
	__u32 dat;

	dat = readl(PL310_REG(L2X0_CTRL));
	if (!(dat & L2X0_ENABLE)) {
		return;
	}
	dat = readl(PL310_REG(L2X0_AUX_CTRL));
	pl310_waymask = (dat & PL310_AUX_WAY) ? 0xffff : 0x00ff;
	outer_cache.inv_range = pl310_inv_range;
	outer_cache.clean_range = pl310_clean_range;
	outer_cache.flush_range = pl310_flush_range;
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	outer_cache.inv_all = pl310_inv_all;
	outer_cache.clean_all = pl310_clean_all;
	outer_cache.flush_all = pl310_flush_all;
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */

	/* cache operations without lock */
	outer_cache_unlocked.inv_range = pl310_inv_range;
	outer_cache_unlocked.clean_range = pl310_clean_range;
	outer_cache_unlocked.flush_range = pl310_flush_range;
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	outer_cache_unlocked.inv_all = __pl310_inv_all;
	outer_cache_unlocked.clean_all = __pl310_clean_all;
	outer_cache_unlocked.flush_all = __pl310_flush_all;
#endif /* CONFIG_EJ_FLUSH_ENTIRE_L2CACHE */
}
