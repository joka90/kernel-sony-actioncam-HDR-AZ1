/*
 * arch/arm/mach-cxd90014/wdt.c
 *
 * Watchdog timer
 *
 * Copyright 2012,2013 Sony Corporation
 *
 * This code is based on kernel/softlockup.c
 */
/*
 * Detect Soft Lockups
 *
 * started by Ingo Molnar, Copyright (C) 2005, 2006 Red Hat, Inc.
 *
 * this code detects soft lockups: incidents in where on a CPU
 * the kernel does not reschedule for 10 seconds or more.
 */
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/lockdep.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <asm/hardware/arm_scu.h>
#include <asm/hardware/arm_twd.h>
#include <mach/regs-timer.h>
#include <mach/regs-misc.h>
#include <mach/pm_export.h>

/* WDT mode */
#define WDT_OFF  0
#define WDT_ON   1
static unsigned int wdt_mode = WDT_OFF;
module_param_named(mode, wdt_mode, uint, S_IRUSR);
static unsigned int wdt_timeout = 0;
module_param_named(timeout, wdt_timeout, uint, S_IRUSR);

/*---------- TIMER IP ----------*/
/* WD clock = 1MHz */
#define CXD90014_WDT_CFG (TMCK_DIV4|TMCS_ONESHOT|TMRST)
static unsigned int cxd90014_wdt_count; /* TIMER IP reload value */

static void cxd90014_wdt_setup(void)
{
	writel_relaxed(CXD90014_WDT_CFG, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCTL);
	writel_relaxed(cxd90014_wdt_count, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCMP);
}

static void cxd90014_wdt_start(void)
{
	writel_relaxed(CXD90014_WDT_CFG|TMST, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCTL);
}

static void cxd90014_wdt_stop(void)
{
	writel_relaxed(0, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCTL);
	writel_relaxed(TMCLR|TMINTCLR, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCLR);
}

static inline void cxd90014_wdt_restart(void)
{
	writel_relaxed(TMCLR, VA_WDT_TIMER(CXD90014_WDT)+CXD90014_TIMERCLR);
}

/*---------- MPCore WDT ----------*/
static unsigned int cpu_wdt_prescaler;
static unsigned int cpu_wdt_count;

#define WDT_CTRL (((cpu_wdt_prescaler-1)<<TWD_TIMER_CONTROL_PRESCALER_SHIFT)|TWD_WDOG_CONTROL_WDMODE)

static void cpu_wdt_setup(void)
{
	writel_relaxed(cpu_wdt_count, VA_LOCALTIMER + TWD_WDOG_COUNTER);
}
static void cpu_wdt_start(void)
{
	writel_relaxed(WDT_CTRL|TWD_TIMER_CONTROL_ENABLE, VA_LOCALTIMER + TWD_WDOG_CONTROL);
}
static void cpu_wdt_stop(void)
{
	writel_relaxed(TWD_WDOG_MAGIC1, VA_LOCALTIMER + TWD_WDOG_DISABLE);
	writel_relaxed(TWD_WDOG_MAGIC2, VA_LOCALTIMER + TWD_WDOG_DISABLE);
	writel_relaxed(0, VA_LOCALTIMER + TWD_WDOG_CONTROL);
}
static void cpu_wdt_restart(void)
{
	writel_relaxed(cpu_wdt_count, VA_LOCALTIMER + TWD_WDOG_LOAD);
}

#if 0
static int cpu_wdt_stat(void)
{
	return readl_relaxed(VA_LOCALTIMER + TWD_WDOG_RESETSTAT) & 1;
}
#endif

static void cpu_wdt_clear(void)
{
	writel_relaxed(1, VA_LOCALTIMER + TWD_WDOG_RESETSTAT);
}

/*-----------------------------------------------------------*/

/*
 * Touches watchdog from sleep.S
 */
void watchdog_touch(void)
{
	if (WDT_OFF == wdt_mode)
		return;

	cpu_wdt_restart();
	cxd90014_wdt_restart();
}

/*
 * This callback runs from the timer interrupt.
 */
void watchdog_tick(void)
{
	int this_cpu;

	if (WDT_OFF == wdt_mode)
		return;

	this_cpu = smp_processor_id();
	cpu_wdt_restart();
	if (!this_cpu) {
		cxd90014_wdt_restart();
	}
}

static int __cpuinit
cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	switch (action) {
	case CPU_STARTING:
	case CPU_STARTING_FROZEN:
		cpu_wdt_stop();
		cpu_wdt_clear();
		cpu_wdt_setup();
		cpu_wdt_start();
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DYING:
	case CPU_DYING_FROZEN:
		cpu_wdt_stop();
		break;
#endif /* CONFIG_HOTPLUG_CPU */
	}
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata cpu_nfb = {
	.notifier_call = cpu_callback
};


#define WDOGINTR_NAME "rstreq"
static int wdog_info;

static irqreturn_t wdt_intr(int irq, void *dev_id)
{
	unsigned long stat;

	stat = readl_relaxed(VA_MISC_RSTREQ_STAT);
	printk(KERN_ERR "WDT:RSTREQ_STAT=0x%lx\n", stat);
	if (stat & (MISC_RSTREQ_ST_CPU3|MISC_RSTREQ_ST_CPU2|MISC_RSTREQ_ST_CPU1|MISC_RSTREQ_ST_CPU0)){
		/* clear CPU WDT events */
		writel_relaxed(MISC_RSTREQ_CPUCLR, VA_MISC_RSTREQ_SET);
		writel_relaxed(MISC_RSTREQ_CPUCLR, VA_MISC_RSTREQ_CLR);
		pm_machine_reset(0);
	}
	BUG();

	return IRQ_HANDLED;
}

#define MAX_PRESCALER 256

static void wdt_setup(void)
{
	unsigned long clk, msec;

	/* CPU WDT load value */
	clk = CXD90014_TWD_CLK;
	cpu_wdt_prescaler = clk / MHZ;
	if (cpu_wdt_prescaler > MAX_PRESCALER)
		cpu_wdt_prescaler = MAX_PRESCALER;
	msec = (clk / cpu_wdt_prescaler) / MSEC_PER_SEC;
	cpu_wdt_count = msec * wdt_timeout;

	/* TIMER WD load value */
	msec = CLOCK_TICK_RATE / MSEC_PER_SEC;
	cxd90014_wdt_count = (msec + 1) * wdt_timeout; /* to fire CPU WDT before TIMER WD */
	cxd90014_wdt_stop();
	cxd90014_wdt_setup();

	/* RSTREQ intr */
	if (request_irq(IRQ_WDOG, wdt_intr, IRQF_DISABLED, WDOGINTR_NAME,
			&wdog_info)) {
		printk(KERN_ERR "WDT:ERROR:request_irq %d failed.\n", IRQ_WDOG);
	}
}

static void wdt_show(void)
{
#if 0
	unsigned int cpu;

	/* CXD90014 does not support this function. */
	printk(KERN_ERR "WDT: checking previous status.\n");
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		if (cpu_wdt_stat(cpu)) {
			printk(KERN_ERR "WDT: CPU#%d occured.\n", cpu);
			/* clear RESET status */
			cpu_wdt_clear(cpu);
		}
	}
	printk(KERN_ERR "WDT: checking done.\n");
#endif
}

/* pre_smp_initcalls */
static int __init wdt_init(void)
{
	unsigned long modepin;

	if (WDT_OFF == wdt_mode) {
		return 0;
	}
	modepin = readl_relaxed(VA_MISC_DBGMODE);
	if (!(modepin & MISC_DBGMODE_DBGMODE)) {
		wdt_mode = WDT_OFF;
		return 0;
	}

	wdt_show();
	wdt_setup();

	cpu_callback(&cpu_nfb, CPU_STARTING, (void *)0);
	cxd90014_wdt_start();

	register_cpu_notifier(&cpu_nfb);

	return 0;
}
early_initcall(wdt_init);

void cxd90014_wdt_resume(void)
{
	if (WDT_OFF == wdt_mode) {
		return ;
	}
	cxd90014_wdt_stop();
	cxd90014_wdt_setup();
	cpu_callback(&cpu_nfb, CPU_STARTING, (void *)0);
	cxd90014_wdt_start();
}
