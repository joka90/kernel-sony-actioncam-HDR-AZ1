/*
 * arch/arm/mach-cxd4115/time.c
 *
 * timer driver
 *
 * Copyright 2007,2008,2009,2010,2011 Sony Corporation
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
//#include <linux/snsc_boot_time.h>

#include <mach/hardware.h>
#include <asm/localtimer.h>
#include <asm/smp_twd.h>
#include <asm/io.h>
#include <asm/irq.h>

#ifdef CONFIG_LOCAL_TIMERS
#include <mach/localtimer.h>
#endif
#include <asm/mach/time.h>
#include <mach/time.h>
#include <mach/regs-timer.h>
#include <mach/gic.h>

#include <linux/cnt32_to_63.h>

#if SCHED_TICK_RATE < 1000000
# define CYC2NS_SCALE_FACTOR 		0
# define TICK_RATE_SCALE_FACTOR		1
#else
#define CYC2NS_SCALE_FACTOR 		10

/* make sure COUNTER_CLOCK_TICK_RATE % TICK_RATE_SCALE_FACTOR == 0 */
#define TICK_RATE_SCALE_FACTOR 		10000
#endif

#define COUNTER_CLOCK_TICK_RATE		SCHED_TICK_RATE

/* cycles to nsec conversions taken from arch/arm/mach-omap1/time.c,
 * convert from cycles(64bits) => nanoseconds (64bits)
 */
#define CYC2NS_SCALE (((NSEC_PER_SEC / TICK_RATE_SCALE_FACTOR) << CYC2NS_SCALE_FACTOR) \
                      / (COUNTER_CLOCK_TICK_RATE / TICK_RATE_SCALE_FACTOR))

static inline unsigned long long notrace cycles_2_ns(unsigned long long cyc)
{
#if (CYC2NS_SCALE & 0x1)
	return (cyc * CYC2NS_SCALE << 1) >> (CYC2NS_SCALE_FACTOR + 1);
#else
	return (cyc * CYC2NS_SCALE) >> CYC2NS_SCALE_FACTOR;
#endif
}

DECLARE_CNT32_TO_63_HI(__sched_clock_hi);
static void sched_clock_hi_init(void)
{
	__sched_clock_hi = 0;
}

unsigned long long notrace sched_clock(void)
{
	unsigned long long cyc;
	cyc = __cnt32_to_63(read_freerun(), __sched_clock_hi);
	return cycles_2_ns(cyc);
}

static inline void cxd4115_systimer_clear(void)
{
	/* Clear the system timer */
	writel(0, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCTL);
	writel(TMCLR|TMINTCLR, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCLR);
}

static inline void cxd4115_systimer_setup(void)
{
	/* Setup the system timer */
	writel(CXD4115_TMCK_TICK_DIV|TMCS_PERIODIC|TMINT, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCTL);
	writel(TIMER_RELOAD, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCMP);
}

static inline void cxd4115_systimer_start(void)
{
	/* Start the system timer */
	writel(CXD4115_TMCK_TICK_DIV|TMCS_PERIODIC|TMINT|TMST, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCTL);
}


static void timer_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *clk)
{
	switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:
                cxd4115_systimer_clear();
                cxd4115_systimer_setup();
                cxd4115_systimer_start();
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
                cxd4115_systimer_clear();
		writel(CXD4115_TMCK_TICK_DIV | TMCS_ONESHOT | TMINT,
		       VA_TIMER(TIMER_FOR_TICK) + CXD4105_TIMERCTL);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		cxd4115_systimer_clear();
                break;
	}
}

static int timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	unsigned long ctrl = readl(VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCTL);

	/*
	 * clear the timer before restart
	 */
	cxd4115_systimer_clear();

	writel(evt, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCMP);
	writel(ctrl | TMST, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCTL);

	return 0;
}

static struct clock_event_device timer1_clockevent =	 {
	.name		= "timer1",
	.shift		= 32,
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= timer_set_mode,
	.set_next_event	= timer_set_next_event,
	.rating		= 300,
	.cpumask	= cpu_all_mask,
};

static void __init cxd4115_clockevents_init(unsigned int timer_irq)
{
	timer1_clockevent.irq = timer_irq;
	timer1_clockevent.mult =
		div_sc(CLOCK_TICK_RATE, NSEC_PER_SEC, timer1_clockevent.shift);
	timer1_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &timer1_clockevent);
	timer1_clockevent.min_delta_ns =
		clockevent_delta2ns(0xf, &timer1_clockevent);

	clockevents_register_device(&timer1_clockevent);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t cxd4115_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &timer1_clockevent;

	/* clear the interrupt */
        writel(TMINTCLR, VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERCLR);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction cxd4115_timer_irq = {
	.name		= "CXD4115 timer",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= cxd4115_timer_interrupt
};

unsigned long cxd4115_read_cycles(void)
{
	return read_freerun();
}
EXPORT_SYMBOL(cxd4115_read_cycles);

static cycle_t cxd4115_get_cycles(void)
{
	return read_freerun();
}

static struct clocksource clocksource_cxd4115 = {
	.name	= "cxd4105-freerun-timer",
	.rating	= 200,
	.read	= cxd4115_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
#if SCHED_TICK_RATE < 1000000
	.shift	= 10,
#else
	.shift	= 20,
#endif
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int __init cxd4115_clocksource_init(void)
{
	clocksource_cxd4115.mult =
		clocksource_hz2mult(SCHED_TICK_RATE,
                                     clocksource_cxd4115.shift);
	clocksource_register(&clocksource_cxd4115);

	printk("%s: %u %u \n", __func__, clocksource_cxd4115.mult, clocksource_cxd4115.shift);
	return 0;
}

/*
 * Setup sched clock(profile timer)
 * 	Called when resume (cxd4115_pm_enter()).
 */
static unsigned long sched_counter;

void cxd4115_timer_early_init(void)
{
	/* (note) TIMER CLK and PCLK are enabled. (default) */

	sched_clock_hi_init();

	/* already running ? */
	if (readl(VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERCTL)
	    == (CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST)) {
		return;
	}

	/* Clear the sched clock */
	writel(0, VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERCTL);
	writel(TMCLR|TMINTCLR, VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERCLR);

	writel(sched_counter, VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERLOAD);
	printk("%s: %ld %lx\n", __func__, sched_counter, read_freerun());

	/* Start the sched clock */
	writel(CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST, VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERCTL);
}

#ifdef CONFIG_EJ_LOCAL_TIMER_PHASE_SHIFT
/*
 * Returns number of us since last clock interrupt.
 */
static unsigned long cxd4115_getrawoffset(void)
{
	unsigned long ticks1;

	ticks1 = readl(VA_TIMER(TIMER_FOR_TICK)+CXD4105_TIMERREAD);
	return TICKS2USECS(ticks1);
}
#endif /* CONFIG_EJ_LOCAL_TIMER_PHASE_SHIFT */

static void __init cxd4115_init_timer(void)
{
#ifdef CONFIG_EJ_LOCALTIMER_ONLY
	extern void percpu_timer_setup(void);
#endif

	cxd4115_clocksource_init();

#ifdef CONFIG_LOCAL_TIMERS
	twd_base = VA_LOCALTIMER;
#endif
#ifdef CONFIG_EJ_LOCALTIMER_ONLY
	twd_timer_rate = 156000000;
	percpu_timer_setup();
#else
	/*
	 * Initialize to a known state (all timers off)
	 */
	/* (note) TIMER CLK and PCLK are enabled. (default) */
	cxd4115_systimer_clear();

	setup_irq(IRQ_TIMER(TIMER_FOR_TICK), &cxd4115_timer_irq);

	cxd4115_clockevents_init(IRQ_TIMER(TIMER_FOR_TICK));
#endif
}

struct sys_timer cxd4115_timer __initdata_refok = {
	.init		= cxd4115_init_timer,
#ifdef CONFIG_EJ_LOCAL_TIMER_PHASE_SHIFT
	.raw_offset	= cxd4115_getrawoffset,
#endif
};
