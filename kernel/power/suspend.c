/* 2013-08-20: File changed by Sony Corporation */
/*
 * kernel/power/suspend.c - Suspend to RAM and standby functionality.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * Copyright (c) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 *
 * This file is released under the GPLv2.
 */

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/syscalls.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/snsc_boot_time.h>
#include <linux/ftrace.h>
#include <trace/events/power.h>
#include <linux/reboot.h>
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot.h>
#endif

#ifdef	CONFIG_SNSC_SAFE_SUSPEND
extern int suspend_remount(void);
extern int resume_remount(void);
#endif

#include "power.h"

static volatile int pm_prevent_suspend = 0;

void pm_suspend_stop(int stop)
{
	pm_prevent_suspend = stop;
	smp_mb();
}

const char *const pm_states[PM_SUSPEND_MAX] = {
	[PM_SUSPEND_STANDBY]	= "standby",
	[PM_SUSPEND_MEM]	= "mem",
#if defined (CONFIG_SNSC_SSBOOT) && !defined(CONFIG_WARM_BOOT_IMAGE)
	[PM_SUSPEND_SNAPSHOT]   = "snapshot",
#endif
#ifdef CONFIG_WARM_BOOT_IMAGE
	[PM_SUSPEND_DISK]	= "warm",
#endif
};

static const struct platform_suspend_ops *suspend_ops;

/**
 *	suspend_set_ops - Set the global suspend method table.
 *	@ops:	Pointer to ops structure.
 */
void suspend_set_ops(const struct platform_suspend_ops *ops)
{
	mutex_lock(&pm_mutex);
	suspend_ops = ops;
	mutex_unlock(&pm_mutex);
}

bool valid_state(suspend_state_t state)
{
	/*
	 * All states need lowlevel support and need to be valid to the lowlevel
	 * implementation, no valid callback implies that none are valid.
	 */
	return suspend_ops && suspend_ops->valid && suspend_ops->valid(state);
}

/**
 * suspend_valid_only_mem - generic memory-only valid callback
 *
 * Platform drivers that implement mem suspend only and only need
 * to check for that in their .valid callback can use this instead
 * of rolling their own .valid callback.
 */
int suspend_valid_only_mem(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}

int resume_phase = 0;

static int suspend_test(int level)
{
#ifdef CONFIG_PM_DEBUG
	if (pm_test_level == level) {
		printk(KERN_INFO "suspend debug: Waiting for 5 seconds.\n");
		mdelay(5000);
		return 1;
	}
#endif /* !CONFIG_PM_DEBUG */
	return 0;
}

#ifdef CONFIG_SNSC_SSBOOT
static int suspend_prepare_ssboot(int kernel_task_only)
{
	int error;

	if (!kernel_task_only && freeze_tasks(FREEZER_USER_SPACE)) {
		error = -EAGAIN;
		return error;
	}

#ifdef CONFIG_WARM_BOOT_IMAGE
	if (get_ssboot_stat() == SSBOOT_CREATE_OPTSSBI) {
		error = ssboot_optimizer_optimize();
	} else {
		error = ssboot_prepare();
	}
#else
	error = ssboot_prepare();
#endif

	if (error) {
		return error;
	}

#ifdef CONFIG_SNSC_SAFE_SUSPEND
	/*
	 * Because sync_filesystem() called from
	 * suspend_remount() wakes up bdi threads, and wait
	 * for completion of it. Because bdi threads are
	 * freezable, we have to remount before freezing that
	 * tasks.
	 */
	if (suspend_remount()) {
		error = -EIO;
		return error;
	}
#endif

	if (freeze_tasks(FREEZER_KERNEL_THREADS)) {
		error = -EAGAIN;
		return error;
	}

	return 0;
}
#endif /* CONFIG_SNSC_SSBOOT */

/**
 *	suspend_prepare - Do prep work before entering low-power state.
 *
 *	This is common code that is called for each state that we're entering.
 *	Run suspend notifiers, allocate a console and stop all processes.
 */
#ifdef CONFIG_SNSC_SSBOOT
static int suspend_prepare(suspend_state_t state)
#else
static int suspend_prepare(void)
#endif
{
	int error;

	if (!suspend_ops || !suspend_ops->enter)
		return -EPERM;

	pm_prepare_console();

	error = pm_notifier_call_chain(PM_SUSPEND_PREPARE);
	if (error)
		goto Finish;

	error = usermodehelper_disable();
	if (error)
		goto Finish;

#if defined (CONFIG_SNSC_SSBOOT) && !defined (CONFIG_WARM_BOOT_IMAGE)
	if (state == PM_SUSPEND_SNAPSHOT) {
		error = suspend_prepare_ssboot(0);
		if (error)
			goto Thaw;
	} else
#endif
	error = suspend_freeze_processes();
	if (!error) {
#ifndef CONFIG_SNSC_SSBOOT
#ifdef	CONFIG_SNSC_SAFE_SUSPEND
#error		"This code may cause hang on v2.6.35 or later."
		(void) suspend_remount();
#endif
#endif /* CONFIG_SNSC_SSBOOT */
		return 0;
	}
	else {
#ifdef CONFIG_EJ_SUSPEND_FREEZE_ERROR
              printk(KERN_ERR "PM: suspend_freeze_processes failed\n");
              BUG();
#endif /* CONFIG_EJ_SUSPEND_FREEZE_ERROR */
	}

#ifdef CONFIG_SNSC_SSBOOT
 Thaw:
#endif
	suspend_thaw_processes();
	usermodehelper_enable();
 Finish:
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
	return error;
}

/* default implementation */
void __attribute__ ((weak)) arch_suspend_disable_irqs(void)
{
	local_irq_disable();
}

/* default implementation */
void __attribute__ ((weak)) arch_suspend_enable_irqs(void)
{
	local_irq_enable();
}

/**
 *	suspend_enter - enter the desired system sleep state.
 *	@state:		state to enter
 *
 *	This function should be called after devices have been suspended.
 */
static int suspend_enter(suspend_state_t state)
{
	int error;

	if (suspend_ops->prepare) {
		error = suspend_ops->prepare();
		if (error)
			goto Platform_finish;
	}

	error = dpm_suspend_noirq(PMSG_SUSPEND);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to power down\n");
		goto Platform_finish;
	}

	if (suspend_ops->prepare_late) {
		error = suspend_ops->prepare_late();
		if (error)
			goto Platform_wake;
	}

	if (suspend_test(TEST_PLATFORM))
		goto Platform_wake;

#ifdef CONFIG_SNSC_SSBOOT
	/* Make sure all free pages linked to free list. */
	drain_all_pages();

	/* Make sure all page caches linked to LRU list. */
	error = lru_add_drain_all();
	if (error)
		goto Platform_wake;
#endif

	error = disable_nonboot_cpus();
	if (error || suspend_test(TEST_CPUS))
		goto Enable_cpus;

	arch_suspend_disable_irqs();
	BUG_ON(!irqs_disabled());

	system_state = SYSTEM_SUSPEND;

	BOOT_TIME_ADD1("PM: suspend enter");
	error = syscore_suspend();
	if (!error) {
		if (!(suspend_test(TEST_CORE) || pm_wakeup_pending())) {
			error = suspend_ops->enter(state);
			if (!error)
				resume_phase = 1;
			events_check_enabled = false;
		}
		syscore_resume();
	}
#ifdef CONFIG_SNSC_BOOT_TIME
      boot_time_resume();
#endif
	BOOT_TIME_ADD1("PM: resume start");

	system_state = SYSTEM_RUNNING;

	arch_suspend_enable_irqs();
	BUG_ON(irqs_disabled());

 Enable_cpus:
	enable_nonboot_cpus();

 Platform_wake:
	if (suspend_ops->wake)
		suspend_ops->wake();

	dpm_resume_noirq(PMSG_RESUME);
	BOOT_TIME_ADD1("PM: device resumed(noirq)");

 Platform_finish:
	if (suspend_ops->finish)
		suspend_ops->finish();

	return error;
}

/**
 *	suspend_devices_and_enter - suspend devices and enter the desired system
 *				    sleep state.
 *	@state:		  state to enter
 */
int suspend_devices_and_enter(suspend_state_t state)
{
	int error;

	if (!suspend_ops)
		return -ENOSYS;

	trace_machine_suspend(state);
#ifdef CONFIG_SNSC_SSBOOT
 Rewrite:
#endif
	if (suspend_ops->begin) {
		error = suspend_ops->begin(state);
		if (error)
			goto Close;
	}
	suspend_console();
	ftrace_stop();
	suspend_test_start();
	error = dpm_suspend_start(PMSG_SUSPEND);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to suspend\n");
		goto Recover_platform;
	}
	suspend_test_finish("suspend devices");
	if (suspend_test(TEST_DEVICES))
		goto Recover_platform;

#ifdef CONFIG_SNSC_SSBOOT
	error = suspend_enter(state);
#else
	suspend_enter(state);
#endif

 Resume_devices:
	suspend_test_start();
	dpm_resume_end(PMSG_RESUME);
	BOOT_TIME_ADD1("PM: device resumed");
	suspend_test_finish("resume devices");
	ftrace_start();
	resume_console();
 Close:
	if (suspend_ops->end)
		suspend_ops->end();
	resume_phase = 0;
	trace_machine_suspend(PWR_EVENT_EXIT);

#ifdef CONFIG_SNSC_SSBOOT
#ifdef CONFIG_WARM_BOOT_IMAGE
	if (!error && get_ssboot_stat() == SSBOOT_PROFILE) {
		error = ssboot_optimizer_start_profiling();
		if (error < 0) {
			printk("ssboot_optimizer_start_profiling() failed\n");
		}
	}
	else if (!error &&
		 get_ssboot_stat() == SSBOOT_CREATE_OPTSSBI) {
#ifdef	CONFIG_SNSC_SAFE_SUSPEND
		(void) resume_remount();
#endif
		/* thaw kernel threads only */
#ifdef CONFIG_SNSC_SAVE_AFFINITY
		printk("Restore affinity.\n");
		restore_affinity();
#endif /* CONFIG_SNSC_SAVE_AFFINITY */
		thaw_tasks(true);
		thaw_workqueues();

		/* prepare for suspend again */
		error = suspend_prepare_ssboot(1);
		if (!error) {
			goto Rewrite;
		}
	}
#else /* !CONFIG_WARM_BOOT_IMAGE */
	if (state == PM_SUSPEND_SNAPSHOT) {
		int ret;
		if (!error && ssboot_is_writing()) {
#ifdef CONFIG_SNSC_SAFE_SUSPEND
			resume_remount();
#endif
			/* thaw kernel threads only */
			thaw_tasks(true);
			error = ssboot_write();
		}
		ret = ssboot_finish();
		if (!error) {
			error = ret;
		}
		if (!error && ssboot_is_rewriting()) {
#ifdef	CONFIG_SNSC_SAFE_SUSPEND
			(void) resume_remount();
#endif
			/* thaw kernel threads only */
			thaw_tasks(true);
			thaw_workqueues();

			/* prepare for suspend again */
			error = suspend_prepare_ssboot(1);
			if (!error) {
				goto Rewrite;
			}
		}
	}
#endif /* CONFIG_WARM_BOOT_IMAGE */
#endif /* CONFIG_SNSC_SSBOOT */
	return error;

 Recover_platform:
	if (suspend_ops->recover)
		suspend_ops->recover();
	goto Resume_devices;
}

/**
 *	suspend_finish - Do final work before exiting suspend sequence.
 *
 *	Call platform code to clean up, restart processes, and free the
 *	console that we've allocated. This is not called for suspend-to-disk.
 */
static void suspend_finish(void)
{
#ifdef	CONFIG_SNSC_SAFE_SUSPEND
	(void) resume_remount();
#endif
	suspend_thaw_processes();
	usermodehelper_enable();
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
}

#ifdef CONFIG_EJ_SUSPEND_FREEZE_ERROR
/* Action on suspend fail */
#define FAIL_ACTION_EXCEPTION	0
#define FAIL_ACTION_RESUME	1
#define FAIL_ACTION_REBOOT	2
#define FAIL_ACTION_HALT	3
#endif

/**
 *	enter_state - Do common work of entering low-power state.
 *	@state:		pm_state structure for state we're entering.
 *
 *	Make sure we're the only ones trying to enter a sleep state. Fail
 *	if someone has beat us to it, since we don't want anything weird to
 *	happen when we wake up.
 *	Then, do the setup for suspend, enter the state, and cleaup (after
 *	we've woken up).
 */
int enter_state(suspend_state_t state)
{
	int error;

#ifdef CONFIG_EJ_SUSPEND_FREEZE_ERROR
	extern int pm_action_on_suspend_fail;
#endif
	if (!valid_state(state))
		return -ENODEV;

	if (!mutex_trylock(&pm_mutex))
		return -EBUSY;

	printk(KERN_INFO "PM: Syncing filesystems ... ");
	sys_sync();
	printk("done.\n");

	if (pm_prevent_suspend) {
		printk(KERN_ERR "PM: suspend is inhibited.\n");
		while (pm_prevent_suspend) {
			msleep(1000);
		}
	}

	pr_debug("PM: Preparing system for %s sleep\n", pm_states[state]);
#ifdef CONFIG_SNSC_SSBOOT
	error = suspend_prepare(state);
#else
	error = suspend_prepare();
#endif
	if (error)
		goto Unlock;

	if (suspend_test(TEST_FREEZER))
		goto Finish;

	pr_debug("PM: Entering %s sleep\n", pm_states[state]);
	pm_restrict_gfp_mask();
	error = suspend_devices_and_enter(state);
	pm_restore_gfp_mask();

 Finish:
	pr_debug("PM: Finishing wakeup.\n");
	suspend_finish();
	BOOT_TIME_ADD1("PM: resume finished");
 Unlock:
	mutex_unlock(&pm_mutex);

#ifdef CONFIG_EJ_SUSPEND_FREEZE_ERROR
	if (error) {
		printk(KERN_ERR "ERROR:suspending kernel.\n");
		switch(pm_action_on_suspend_fail) {
		case FAIL_ACTION_EXCEPTION:
			BUG();
			break;
		case FAIL_ACTION_REBOOT:
			machine_restart(NULL);
			/* never return */
			break;
		case FAIL_ACTION_HALT:
			machine_halt();
			/* never return */
			break;
		case FAIL_ACTION_RESUME:
		default:
			break;
		}
	}
#endif

	return error;
}

/**
 *	pm_suspend - Externally visible function for suspending system.
 *	@state:		Enumerated value of state to enter.
 *
 *	Determine whether or not value is within range, get state
 *	structure, and enter (above).
 */
int pm_suspend(suspend_state_t state)
{
	if (state > PM_SUSPEND_ON && state < PM_SUSPEND_MAX)
		return enter_state(state);
	return -EINVAL;
}
EXPORT_SYMBOL(pm_suspend);
