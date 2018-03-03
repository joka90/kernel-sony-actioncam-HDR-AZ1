/*
 * arch/arm/mach-cxd4132/core.c
 *
 * machine_desc and initialize function
 *
 * Copyright 2009,2010 Sony Corporation
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
#endif /* CONFIG_PM */

#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include "../mm/mm.h"

#include <asm/io.h>
#include <asm/irq.h>

#include <mach/gic.h>
//#include <mach/regs-timer.h>
#include <mach/regs-permisc.h>
#include <mach/cxd4132_cpuid.h>
#ifdef CONFIG_CACHE_PL310S
#include <mach/cache-pl310.h>
#endif
#include <mach/cxd4132_board.h>

/*---------------------------- I/O mapping --------------------------*/

#ifdef CONFIG_CXD4132_USE_DEVICE_SO
#define MMU_ATTR1	MT_DEVICE_SO
#else /* CONFIG_CXD4132_USE_DEVICE_SO */
#define MMU_ATTR1	MT_DEVICE
#endif /* CONFIG_CXD4132_USE_DEVICE_SO */

static struct map_desc cxd4132_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(CXD4132_AHB_BASE),
		.pfn		= __phys_to_pfn(CXD4132_AHB_BASE),
		.length		= CXD4132_AHB_SIZE,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD4132_APB0_BASE),
		.pfn		= __phys_to_pfn(CXD4132_APB0_BASE),
		.length		= SZ_16M,
		.type		= MMU_ATTR1 },
	{
		.virtual	= IO_ADDRESS(CXD4132_APB1_BASE),
		.pfn		= __phys_to_pfn(CXD4132_APB1_BASE),
		.length		= SZ_16M,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD4132_APB2_BASE),
		.pfn		= __phys_to_pfn(CXD4132_APB2_BASE),
		.length		= SZ_16M,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD4132_APB3_BASE),
		.pfn		= __phys_to_pfn(CXD4132_APB3_BASE),
		.length		= SZ_16M,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(CXD4132_HIO_BASE),
		.pfn		= __phys_to_pfn(CXD4132_HIO_BASE),
		.length		= CXD4132_HIO_SIZE,
		.type		= MT_DEVICE },
	{
		.virtual	= IO_ADDRESS(FSP_CS1_BASE),
		.pfn		= __phys_to_pfn(FSP_CS1_BASE),
		.length		= FSP_CS1_SIZE,
		.type		= MMU_ATTR1 },
	{
		.virtual	= IO_ADDRESS(FSP_CS2_BASE),
		.pfn		= __phys_to_pfn(FSP_CS2_BASE),
		.length		= FSP_CS2_SIZE,
		.type		= MMU_ATTR1 },
};

static void __init cxd4132_map_ddr(void)
{
	struct map_desc map;

	/* DDR uncached */
	map.virtual = IO_ADDRESS(DDR_BASE);
	map.pfn     = __phys_to_pfn(DDR_BASE);
	map.length  = DDR_SIZE;
#ifdef CONFIG_CXD4132_USE_MEMORY_UC
	map.type    = MT_MEMORY_UC;
#else
	map.type    = MT_DEVICE;
#endif
	create_mapping(&map);
#ifdef CONFIG_EJ_RO_USER_TEXT
	change_mapping2(&map);
#endif /* CONFIG_EJ_RO_USER_TEXT */
}

static void __init cxd4132_map_sram(void)
{
	struct map_desc map;

	map.pfn     = __phys_to_pfn(CXD4132_SRAM_BASE);
	map.length  = cxd4132_get_sramsize();
	/* uncached */
	map.virtual = VA_BOOTRAM;
#ifdef CONFIG_CXD4132_USE_MEMORY_UC
	map.type    = MT_MEMORY_UC;
#else
	map.type    = MT_DEVICE;
#endif
	create_mapping(&map);

	/* cached */
	map.virtual = VA_BOOTRAM_CACHED;
	map.type    = MT_MEMORY;
	create_mapping(&map);
}

void __init cxd4132_map_io(void)
{
	iotable_init(cxd4132_io_desc, ARRAY_SIZE(cxd4132_io_desc));

	/* I/O mapping is required. */
	cxd4132_init_clk_table();
	cxd4132_read_cpuid();
#ifdef CONFIG_CACHE_PL310S
	pl310_setup();
	pl310_info();
#endif /* CONFIG_CACHE_PL310S */

	cxd4132_map_ddr();
	cxd4132_map_sram();
}

/*------------------------------ SYSDEV -----------------------------*/
#ifdef CONFIG_PM
static int cxd4132_suspend(struct sys_device *dev, pm_message_t state)
{
	gic_suspend();
	return 0;
}

static int cxd4132_resume(struct sys_device *dev)
{
	cxd4132_board_init();

	gic_resume();
	return 0;
}
#endif /* CONFIG_PM */

static struct sysdev_class cxd4132_sysclass = {
	.name		= "cxd4132",
#ifdef CONFIG_PM
	.suspend	= cxd4132_suspend,
	.resume		= cxd4132_resume,
#endif
};

static struct sys_device cxd4132_device = {
	.id		= 0,
	.cls		= &cxd4132_sysclass,
};

#ifdef CONFIG_PM
static void cxd4132_reset(int force)
{
	if (force) {
		local_irq_disable();
		/* assert XRESET_REQ */
		writel(PER_RSTREQ_EN|PER_RSTREQ_ASSERT, VA_PER_RSTREQ_SET);

		while (1) ;
	}
	writel(PER_RSTREQ_ASSERT, VA_PER_RSTREQ_SET);
}
void (*pm_machine_reset)(int) = cxd4132_reset;
EXPORT_SYMBOL(pm_machine_reset);

static void cxd4132_poweroff(void)
{
	printk(KERN_ERR "not implemented yet.\n");
	local_irq_disable();
	while (1) ;
}
#endif /* CONFIG_PM */

int __init cxd4132_core_init(void)
{
	int ret;
#ifdef CONFIG_OVERCOMMIT_ALWAYS
	extern int sysctl_overcommit_memory;
#endif
#ifdef CONFIG_PM
	extern void cxd4132_pm_setup(void);
#endif

#ifdef CONFIG_PM
	pm_power_off = cxd4132_poweroff;
	cxd41xx_pm_setup();
	cxd4132_pm_setup();
#endif
#ifdef CONFIG_OVERCOMMIT_ALWAYS
	sysctl_overcommit_memory = OVERCOMMIT_ALWAYS;
#endif

	ret = sysdev_class_register(&cxd4132_sysclass);
	if (ret) {
		printk(KERN_ERR "cxd4132_core_init: sysdev_class_register failed.\n");
		return ret;
	}
	ret = sysdev_register(&cxd4132_device);
	if (ret) {
		printk(KERN_ERR "cxd4132_core_init: sysdev_register failed.\n");
		return ret;
	}
	return ret;
}

/* List of edge trigger IRQs */
static u8 edge_irqs[] __initdata = {
#ifdef CONFIG_MPCORE_GIC_INIT
	IRQ_GPIO_RISE(0), IRQ_GPIO_RISE(1), IRQ_GPIO_RISE(2), IRQ_GPIO_RISE(3),
	IRQ_GPIO_RISE(4), IRQ_GPIO_RISE(5), IRQ_GPIO_RISE(6), IRQ_GPIO_RISE(7),
	IRQ_GPIO_RISE(8), IRQ_GPIO_RISE(9), IRQ_GPIO_RISE(10),IRQ_GPIO_RISE(11),
	IRQ_GPIO_RISE(12),IRQ_GPIO_RISE(13),IRQ_GPIO_RISE(14),IRQ_GPIO_RISE(15),
	IRQ_GPIO_RISE(16),IRQ_GPIO_RISE(17),IRQ_GPIO_RISE(18),IRQ_GPIO_RISE(19),
	IRQ_GPIO_RISE(20),IRQ_GPIO_RISE(21),IRQ_GPIO_RISE(22),IRQ_GPIO_RISE(23),
	IRQ_GPIO_FALL(0), IRQ_GPIO_FALL(1), IRQ_GPIO_FALL(2), IRQ_GPIO_FALL(3),
	IRQ_GPIO_FALL(4), IRQ_GPIO_FALL(5), IRQ_GPIO_FALL(6), IRQ_GPIO_FALL(7),
	IRQ_GPIO_FALL(8), IRQ_GPIO_FALL(9), IRQ_GPIO_FALL(10),IRQ_GPIO_FALL(11),
	IRQ_GPIO_FALL(12),IRQ_GPIO_FALL(13),IRQ_GPIO_FALL(14),IRQ_GPIO_FALL(15),
	IRQ_GPIO_FALL(16),IRQ_GPIO_FALL(17),IRQ_GPIO_FALL(18),IRQ_GPIO_FALL(19),
	IRQ_GPIO_FALL(20),IRQ_GPIO_FALL(21),IRQ_GPIO_FALL(22),IRQ_GPIO_FALL(23),
	IRQ_USB_ID_RISE, IRQ_USB_ID_FALL, IRQ_USB_BVALID_RISE,IRQ_USB_BVALID_FALL,
	IRQ_FSP_XINT,
#endif /* CONFIG_MPCORE_GIC_INIT */
};

void __init cxd4132_init_irq(void)
{
	gic_dist_init(0, VA_GIC_DIST, 29, edge_irqs, sizeof edge_irqs);
	gic_cpu_init(0, VA_GIC_CPU);
#if !defined(CONFIG_QEMU)
	//cxd4132_fiq_init();
#endif /* !CONFIG_QEMU */
}

static int xrstreq_mode = -1; /* do not touch RSTREQ register */

static void cxd4132_xrstreq_init(void)
{
	if (xrstreq_mode < 0)
		return;

	if (!xrstreq_mode) {
		writel(PER_RSTREQ_EN, VA_PER_RSTREQ_CLR);
	} else {
		writel(PER_RSTREQ_EN, VA_PER_RSTREQ_SET);
	}
}

static int __init cxd4132_xrstreq_setup(char *str)
{
	int val;

	get_option(&str, &val);
	if (!val) {
		xrstreq_mode = 0;
	} else {
		xrstreq_mode = 1;
	}
	cxd4132_xrstreq_init();

	return 1;
}
__setup("xrstreq=", cxd4132_xrstreq_setup);

#ifdef CONFIG_PM
/*
 * Called from plat-cxd41xx/pm.c just after resume.
 * It is earlier than sysdev resume.
 */
#if !defined(CONFIG_QEMU)
void cxd41xx_early_resume(void)
{
	extern void cxd4132_wdt_resume(void);

	cxd4132_xrstreq_init();
	cxd4132_wdt_resume();
	//cxd4132_fiq_init();
}
#endif /* !CONFIG_QEMU */
#endif
