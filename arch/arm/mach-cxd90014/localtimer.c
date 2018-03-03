/*
 * arch/arm/mach-cxd90014/localtimer.c
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on arch/arm/plat-realview/localtimer.c
 */
/*
 *  linux/arch/arm/plat-versatile/localtimer.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/clockchips.h>

#include <asm/smp_twd.h>
#include <asm/localtimer.h>
#include <mach/irqs.h>
#include <mach/time.h>

/*
 * Setup the local clock events for a CPU.
 */
#ifdef CONFIG_CXD90014_EXT_TIMER
int __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	extern void ext_timer_setup(struct clock_event_device *evt, unsigned int cpu);

	if (!cpu)
		return 0;
	ext_timer_setup(evt, cpu);
	return 0;
}

int local_timer_ack(void)
{
	return 0;
}

#else /* CONFIG_CXD90014_EXT_TIMER */

int __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	evt->irq = IRQ_LOCALTIMER;
	twd_timer_rate = CXD90014_TWD_CLK;
	twd_timer_setup(evt);
	return 0;
}
#endif /* CONFIG_CXD90014_EXT_TIMER */
