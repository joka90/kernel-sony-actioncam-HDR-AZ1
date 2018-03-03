/*
 * arch/arm/mach-cxd90014/core.c
 *
 * machine_desc and initialize function
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on arch/arm/mach-realview/realview_eb.c.
 */
/*
 *  linux/arch/arm/mach-realview/realview_eb.c
 *
 *  Copyright (C) 2004 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sysdev.h>
#include <linux/mman.h>

#ifdef CONFIG_PM
#include <mach/pm.h>
#include <linux/syscore_ops.h>
#endif /* CONFIG_PM */

#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include "../mm/mm.h"

#include <asm/io.h>
#include <asm/irq.h>

#include <mach/gic_export.h>
#include <mach/regs-misc.h>
#include <mach/cxd90014_cpuid.h>
#ifdef CONFIG_CACHE_PL310
#include <mach/pl310.h>
#endif
#include <mach/cxd90014_board.h>
#include "pcie.h"

/* memtype of I/O device */
static unsigned int mt_io = MT_DEVICE;
static int __init setup_io_memtype(char *str)
{
	unsigned int n;

	if (get_option(&str, &n) != 1) {
		return 1;
	}
	if (!get_mem_type(n)) {
		printk(KERN_ERR "ERROR: illegal mt_io:%u\n", n);
		return 1;
	}
	mt_io = n;
	return 0;
}
early_param("mt_io", setup_io_memtype);

/* memtype of uncached memory */
static unsigned int mt_uc = MT_DEVICE;
static int __init setup_uc_memtype(char *str)
{
	unsigned int n;

	if (get_option(&str, &n) != 1) {
		return 1;
	}
	if (!get_mem_type(n)) {
		printk(KERN_ERR "ERROR: illegal mt_uc:%u\n", n);
		return 1;
	}
	mt_uc = n;
	return 0;
}
early_param("mt_uc", setup_uc_memtype);

/* L2 cache enable/disable */
static int l2off = 0;
static int __init setup_l2off(char *str)
{
	l2off = 1;
	return 0;
}
early_param("l2off", setup_l2off);

/*---------------------------- I/O mapping --------------------------*/

static struct map_desc cxd90014_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(CXD90014_IO_BASE),
		.pfn		= __phys_to_pfn(CXD90014_IO_BASE),
		.length		= CXD90014_IO_SIZE,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD90014_HIO_BASE),
		.pfn		= __phys_to_pfn(CXD90014_HIO_BASE),
		.length		= CXD90014_HIO_SIZE,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD90014_SRAMC_CS_BASE),
		.pfn		= __phys_to_pfn(CXD90014_SRAMC_CS_BASE),
		.length		= CXD90014_SRAMC_CS_SIZE,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD90014_PCIE_IO_VIRT_BASE),
		.pfn		= __phys_to_pfn(CXD90014_PCIE_IO_PHYS_BASE),
		.length		= CXD90014_PCIE_IO_SIZE,
		.type		= MT_DEVICE },
};

static void __init cxd90014_map_ddr(void)
{
#if PAGE_OFFSET < PHYS_OFFSET
	struct map_desc map;

	/* DDR uncacheable */
	map.virtual = IO_ADDRESS(DDR_BASE);
	map.pfn     = __phys_to_pfn(DDR_BASE);
	map.length  = DDR_SIZE;
	map.type    = mt_uc;
	create_mapping(&map);
#endif /* PAGE_OFFSET < PHYS_OFFSET */
}

static void __init cxd90014_map_sram(void)
{
	struct map_desc map;

	map.pfn     = __phys_to_pfn(CXD90014_ESRAM_BASE);
	map.length  = CXD90014_ESRAM_SIZE;
	/* eSRAM uncacheable */
	map.virtual = VA_BOOTRAM;
	map.type    = mt_uc;
	create_mapping(&map);

	/* eSRAM cacheable */
	map.virtual = VA_BOOTRAM_CACHED;
	map.type    = MT_MEMORY;
	create_mapping(&map);
}

void __init cxd90014_map_io(void)
{
	int i;

	/* overwrite memtype */
	for (i = 0; i < ARRAY_SIZE(cxd90014_io_desc); i++) {
		cxd90014_io_desc[i].type = mt_io;
	}
	iotable_init(cxd90014_io_desc, ARRAY_SIZE(cxd90014_io_desc));

	/* I/O mapping is required. */
	cxd90014_read_cpuid();
#ifdef CONFIG_CACHE_PL310
	if (!l2off) {
		l2x0_init((void __iomem *)VA_PL310, AUX_VAL, AUX_MASK);
		l2x0_info();
	}
#endif /* CONFIG_CACHE_PL310 */

	cxd90014_map_ddr();
	cxd90014_map_sram();
}

/*------------------------------ SYSCORE -----------------------------*/
#ifdef CONFIG_PM

static int cxd90014_suspend(void)
{
#ifdef CONFIG_CXD90014_PCIE2_DMX
	f_pcie2_dmx_suspend();
#endif /* CONFIG_CXD90014_PCIE2_DMX */
	gic_cxd90014_suspend();
        return 0;
}

static void cxd90014_resume(void)
{
	cxd90014_board_init();

	gic_cxd90014_resume();
#ifdef CONFIG_CXD90014_PCIE2_DMX
	f_pcie2_dmx_resume();
#endif /* CONFIG_CXD90014_PCIE2_DMX */
}

static struct syscore_ops cxd90014_syscore_ops = {

	.suspend        = cxd90014_suspend,
	.resume         = cxd90014_resume,
};

static void cxd90014_reset(int force)
{
        if (force) {
                local_irq_disable();
                /* assert XRESET_REQ */
                writel(MISC_RSTREQ_EN|MISC_RSTREQ_ASSERT, VA_MISC_RSTREQ_SET);
		wmb();

                while (1) ;
        }
        writel(MISC_RSTREQ_ASSERT, VA_MISC_RSTREQ_SET);
	wmb();
}
void (*pm_machine_reset)(int) = cxd90014_reset;
EXPORT_SYMBOL(pm_machine_reset);

static void cxd90014_poweroff(void)
{
        printk(KERN_ERR "not implemented yet.\n");
        local_irq_disable();
        while (1) ;
}
#endif /* CONFIG_PM */

/*------------------------------ SYSCORE -----------------------------*/

int __init cxd90014_core_init(void)
{
	int ret = 0;
#ifdef CONFIG_OVERCOMMIT_ALWAYS
	extern int sysctl_overcommit_memory;
#endif

#ifdef CONFIG_PM
	pm_power_off = cxd90014_poweroff;
	cxd90014_pm_setup();
#endif

#ifdef CONFIG_OVERCOMMIT_ALWAYS
	sysctl_overcommit_memory = OVERCOMMIT_ALWAYS;
#endif

#ifdef CONFIG_PM
	register_syscore_ops(&cxd90014_syscore_ops);
#endif

	return ret;
}

void __init cxd90014_init_irq(void)
{
#if !defined(CONFIG_QEMU)
	extern void cxd90014_fiq_init(void);
#endif /* !CONFIG_QEMU */

	gic_init(0, 29, (void __iomem *)VA_GIC_DIST, (void __iomem *)VA_GIC_CPU);
	gic_cxd90014_init();
	gic_dump();
#if !defined(CONFIG_QEMU)
	cxd90014_fiq_init();
#endif /* !CONFIG_QEMU */
}

static int xrstreq_mode = -1; /* do not touch RSTREQ register */

static void cxd90014_xrstreq_init(void)
{
	if (xrstreq_mode < 0)
		return;

	if (!xrstreq_mode) {
		writel_relaxed(MISC_RSTREQ_EN, VA_MISC_RSTREQ_CLR);
	} else {
		writel_relaxed(MISC_RSTREQ_EN, VA_MISC_RSTREQ_SET);
	}
}

static int __init cxd90014_xrstreq_setup(char *str)
{
	int val;

	get_option(&str, &val);
	if (!val) {
		xrstreq_mode = 0;
	} else {
		xrstreq_mode = 1;
	}
	cxd90014_xrstreq_init();

	return 1;
}
__setup("xrstreq=", cxd90014_xrstreq_setup);

/*
 * Called from mach-cxd90014/pm.c just after resume.
 * It is earlier than sysdev resume.
 */
#if !defined(CONFIG_QEMU)
void cxd90014_early_resume(void)
{
	extern void cxd90014_wdt_resume(void);

#ifdef CONFIG_CACHE_PL310
	if (!l2off) {
		extern void pl310_enable(void);
		pl310_enable();
	}
#endif /* CONFIG_CACHE_PL310 */
	cxd90014_xrstreq_init();
	cxd90014_wdt_resume();
	cxd90014_fiq_init();
}
#endif /* !CONFIG_QEMU */
