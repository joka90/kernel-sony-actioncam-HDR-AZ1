/* 2008-09-16: File added and changed by Sony Corporation */
/*
 *  linux/arch/arm/mach-ne1/platsmp.c
 *
 *  copied from
 *  arch/arm/mach-realview/platsmp.c
 *  then modified
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/smp_scu.h>
#include <asm/unified.h>

#include <mach/ne1_sysctrl.h>

#include "core.h"

extern void ne1_secondary_startup(void);

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 *
 * zzz Used by arch/arm/mach-ne1/hotplug.c: platform_do_lowpower()
 *
 * zzz Used by arch/arm/mach-ne1/headsmp.S: ne1_secondary_startup()
 */
#ifndef CONFIG_HOTPLUG_CPU
volatile int __cpuinitdata pen_release = -1;
#else
volatile int pen_release = -1;
#endif

void __cpuinit platform_secondary_init(unsigned int cpu)
{
	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	pen_release = -1;
	smp_wmb();
}

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	pen_release = cpu;
	flush_cache_all();

	/*
	 * XXX
	 *
	 * This is a later addition to the booting protocol: the
	 * bootMonitor now puts secondary cores into WFI, so
	 * poke_milo() no longer gets the cores moving; we need
	 * to send a soft interrupt to wake the secondary core.
	 * Use smp_cross_call() for this, since there's little
	 * point duplicating the code here
	 */
	__raw_writel(virt_to_phys(ne1_secondary_startup),
		 IO_ADDRESS(NE1_BASE_SYSCTRL + SYSCTRL_MEMO));
	mb();

	/* this sends the soft interrupt (IPI) */
	smp_send_reschedule(cpu);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		flush_cache_all();
		udelay(10);
	}

	return pen_release != -1 ? -ENOSYS : 0;
}

static void __iomem *scu_base_addr(void)
{
	return __io_address(NE1_BASE_SCU);
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores = scu_base ? scu_get_core_count(scu_base) : 1;

	/* sanity check */
	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "NPCore: no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	scu_enable(scu_base_addr());

	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The BootMonitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */
	__raw_writel(BSYM(virt_to_phys(ne1_secondary_startup)),
		     __io_address(NE1_BASE_SYSCTRL + SYSCTRL_MEMO));

	/* zzz 2.6.39 has after write to SYSCTRL_MEMO:
	mb();
	 * but also has smp_cross_call()...
	 * zzz where has the smp_cross_call() and wait for pen_release moved?
	 */
}
