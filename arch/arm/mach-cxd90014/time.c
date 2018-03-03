/*
 * arch/arm/mach-cxd90014/time.c
 *
 * timer driver
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on arch/arm/mach-integrator/core.c.
 */
/*
 *  linux/arch/arm/mach-integrator/core.c
 *
 *  Copyright (C) 2000-2003 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pm.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/snsc_boot_time.h>

#include <mach/hardware.h>
#include <asm/localtimer.h>
#include <asm/smp_twd.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/mach/time.h>
#include <mach/time.h>
#include <mach/regs-timer.h>

#include <asm/sched_clock.h>

#if SCHED_TICK_RATE == 1000000
# define SC_MULT 		4194304000UL
# define SC_SHIFT		22
#elif SCHED_TICK_RATE == 2000000
# define SC_MULT 		4194304000UL
# define SC_SHIFT		23
#else
# error "Unexpected SCHED_TICK_RATE"
#endif

/*
 * Observe tick and localtimer for debug
 */
int debug_tick_timing = 0;
module_param_named(tick_timing, debug_tick_timing, bool, S_IRUSR|S_IWUSR);

static DEFINE_CLOCK_DATA(cd);

unsigned long long notrace sched_clock(void)
{
	u32 cyc = read_freerun();
	return cyc_to_fixed_sched_clock(&cd, cyc, (u32)~0, SC_MULT, SC_SHIFT);
}

static void notrace cxd90014_update_sched_clock(void)
{
	u32 cyc = read_freerun();
	update_sched_clock(&cd, cyc, (u32)~0);
}

static void cxd90014_sched_clock_init(void)
{
	init_fixed_sched_clock(&cd, cxd90014_update_sched_clock,
			       32, SCHED_TICK_RATE, SC_MULT, SC_SHIFT);
	/* hand over */
	cd.epoch_ns = cyc_to_ns((u64)read_freerun(), SC_MULT, SC_SHIFT);
}

unsigned long cxd4115_read_cycles(void)
{
	return read_freerun();
}
EXPORT_SYMBOL(cxd4115_read_cycles);

static cycle_t cxd4115_get_cycles(struct clocksource *cs)
{
	return (cycle_t)read_freerun();
}

static struct clocksource clocksource_cxd4115 = {
	.name	= "cxd4105-freerun-timer",
	.rating	= 200,
	.read	= cxd4115_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int __init cxd4115_clocksource_init(void)
{
	cxd90014_sched_clock_init();
	clocksource_register_hz(&clocksource_cxd4115, SCHED_TICK_RATE);
	printk("%s: %u %u \n", __func__, clocksource_cxd4115.mult, clocksource_cxd4115.shift);
	return 0;
}


#ifndef CONFIG_EJ_LOCALTIMER_ONLY
static inline void ext_timer_clear(const void __iomem *base)
{
	/* Clear the local tick timer */
	writel_relaxed(0, base + CXD90014_TIMERCTL);
	writel_relaxed(TMCLR|TMINTCLR, base + CXD90014_TIMERCLR);
}

static inline void ext_timer_set(const void __iomem *base)
{
	/* Setup the local tick timer */
	writel_relaxed(CXD4115_TMCK_TICK_DIV|TMCS_PERIODIC|TMINT,
		       base + CXD90014_TIMERCTL);
	writel_relaxed(TIMER_RELOAD, base + CXD90014_TIMERCMP);
}

static inline void ext_timer_start(const void __iomem *base)
{
	/* Start the local tick timer */
	writel_relaxed(CXD4115_TMCK_TICK_DIV|TMCS_PERIODIC|TMINT|TMST,
		       base + CXD90014_TIMERCTL);
}

static inline void ext_timer_oneshot(const void __iomem *base)
{
	writel_relaxed(CXD4115_TMCK_TICK_DIV|TMCS_ONESHOT|TMINT,
		       base + CXD90014_TIMERCTL);
}

struct ext_timer {
	struct clock_event_device *evt;
	const unsigned int ch;
	const void __iomem *base;
	struct irqaction irq;
};
static struct ext_timer ext_timers[NR_CPUS] = {
	{
		.ch = TIMER_FOR_TICK,
		.base = VA_TIMER(TIMER_FOR_TICK),
		.irq = {
			.name		= "tick0",
			.flags		= IRQF_DISABLED | IRQF_TIMER,
		},
	},
	{
		.ch = TIMER_FOR_TICK1,
		.base = VA_TIMER(TIMER_FOR_TICK1),
		.irq = {
			.name		= "tick1",
			.flags		= IRQF_DISABLED | IRQF_TIMER,
		},
	},
	{
		.ch = TIMER_FOR_TICK2,
		.base = VA_TIMER(TIMER_FOR_TICK2),
		.irq = {
			.name		= "tick2",
			.flags		= IRQF_DISABLED | IRQF_TIMER,
		},
	},
	{
		.ch = TIMER_FOR_TICK3,
		.base = VA_TIMER(TIMER_FOR_TICK3),
		.irq = {
			.name		= "tick3",
			.flags		= IRQF_DISABLED | IRQF_TIMER,
		},
	},
};

static void ext_timer_set_mode(enum clock_event_mode mode,
			       struct clock_event_device *clk)
{
	struct ext_timer *t = &ext_timers[smp_processor_id()];

	switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		ext_timer_clear(t->base);
		ext_timer_set(t->base);
		ext_timer_start(t->base);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ext_timer_clear(t->base);
		ext_timer_oneshot(t->base);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		ext_timer_clear(t->base);
		break;
	}
}

static int ext_timer_set_next_event(unsigned long next,
				    struct clock_event_device *unused)
{
	struct ext_timer *t = &ext_timers[smp_processor_id()];
	unsigned long ctrl;

	ctrl = readl_relaxed(t->base + CXD90014_TIMERCTL);
	/*
	 * clear the timer before restart
	 */
	ext_timer_clear(t->base);

	writel_relaxed(next, t->base + CXD90014_TIMERCMP);
	writel_relaxed(ctrl | TMST, t->base + CXD90014_TIMERCTL);
	return 0;
}

/*
 * IRQ handler for the ext_timer
 */
static irqreturn_t ext_timer_interrupt(int irq, void *dev_id)
{
	struct ext_timer *t = dev_id;
	struct clock_event_device *evt = t->evt;

	/* clear the interrupt */
	writel_relaxed(TMINTCLR, t->base + CXD90014_TIMERCLR);
	wmb();

	if (debug_tick_timing & 1) {
		BOOT_TIME_ADD1((char *)(evt->name));
	}

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static void ext_timer_clockevents_init(struct clock_event_device *evt,
				       struct ext_timer *t,
				       unsigned int cpu)
{
	evt->name	= t->irq.name;
	evt->shift	= 32;
	evt->features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	evt->set_mode	= ext_timer_set_mode;
	evt->set_next_event = ext_timer_set_next_event;
	evt->rating	= 300;
	evt->irq	= IRQ_TIMER(t->ch);

	evt->mult =
		div_sc(CLOCK_TICK_RATE, NSEC_PER_SEC, evt->shift);
	evt->max_delta_ns =
		clockevent_delta2ns(0xffffffff, evt);
	evt->min_delta_ns =
		clockevent_delta2ns(0xf, evt);

	clockevents_register_device(evt);
}

void ext_timer_setup(struct clock_event_device *evt, unsigned int cpu)
{
	struct ext_timer *t = &ext_timers[cpu];

	/*
	 * Initialize to a known state (all timers off)
	 */
	/* (note) TIMER CLK and PCLK are enabled. (default) */
	ext_timer_clear(t->base);

	t->evt = evt;
	t->irq.dev_id = t;
	t->irq.handler = ext_timer_interrupt;
#ifdef CONFIG_MPCORE_GIC_INIT
	irq_set_affinity(IRQ_TIMER(t->ch), cpumask_of(cpu));
#endif /* CONFIG_MPCORE_GIC_INIT */
	setup_irq(IRQ_TIMER(t->ch), &t->irq);

	ext_timer_clockevents_init(evt, t, cpu);
}

static struct clock_event_device timer1_clockevent;
#endif /* !CONFIG_EJ_LOCALTIMER_ONLY */

/*
 * Setup sched clock(profile timer)
 *      Called when resume (cxd90014_pm_enter()).
 */
void cxd90014_timer_early_init(void)
{
	/* (note) TIMER CLK and PCLK are enabled. (default) */

	cd.epoch_ns       = 0;
	cd.epoch_cyc      = 0;
	cd.epoch_cyc_copy = 0;

	/* already running ? */
	if (readl_relaxed(VA_TIMER(TIMER_FOR_SCHED)+CXD90014_TIMERCTL)
	    == (CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST)) {
		return;
	}

	/* Clear the sched clock */
	writel_relaxed(0, VA_TIMER(TIMER_FOR_SCHED)+CXD90014_TIMERCTL);
	writel_relaxed(TMCLR|TMINTCLR, VA_TIMER(TIMER_FOR_SCHED)+CXD90014_TIMERCLR);
	/* Start the sched clock */
	writel_relaxed(CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST, VA_TIMER(TIMER_FOR_SCHED)+CXD90014_TIMERCTL);
}

static void __init cxd4115_init_timer(void)
{
#ifdef CONFIG_EJ_LOCALTIMER_ONLY
	extern void percpu_timer_setup(void);
#endif

	cxd4115_clocksource_init();

#if defined(CONFIG_LOCAL_TIMERS) && defined(CONFIG_HAVE_ARM_TWD)
	twd_base = VA_LOCALTIMER;
#endif
#ifdef CONFIG_EJ_LOCALTIMER_ONLY
	/* Can not calibrate localtimer w/o TIMER IP. */
	twd_timer_rate = CXD90014_TWD_CLK;
	percpu_timer_setup();
#else
	timer1_clockevent.cpumask = cpumask_of(0);
	ext_timer_setup(&timer1_clockevent, 0);
#endif
}

struct sys_timer cxd4115_timer __initdata_refok = {
	.init		= cxd4115_init_timer,
};
