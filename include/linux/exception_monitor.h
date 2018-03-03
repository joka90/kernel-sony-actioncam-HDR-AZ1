/* 2007-08-08: File added and changed by Sony Corporation */
/*
 * Copyright 2006,2007  Sony Corporation
 */
#ifndef EXCEPTION_MONITOR_H
#define EXCEPTION_MONITOR_H
#ifdef CONFIG_EXCEPTION_MONITOR

#include <linux/sched.h>

extern int exception_check_mode;
extern void (*exception_check)(int mode, struct pt_regs *regs);
extern int stop_all_threads_timeout(struct mm_struct *mm, long timeout);
extern void start_all_threads(struct mm_struct *mm);
extern int flush_serial_tty(int line);

static inline int  em_show_coredump(struct pt_regs *regs)
{
	if (exception_check_mode && exception_check) {
		int ret = stop_all_threads_timeout(current->mm,
		          CONFIG_EXCEPTION_MONITOR_STOP_SIBLING_TIMEOUT);
		if (ret == -ETIMEDOUT) {
			printk(KERN_WARNING
			       "Exception Monitor: waiting for "
			       "uninterruptible threads timedout.\n");
		}
		else if (ret != 0)
			return -1;
		exception_check(exception_check_mode, regs);
	/* Restart our siblings (so they can die, or whatever). */
		start_all_threads(current->mm);
	}
	return 0;
}

static inline int  em_show(struct pt_regs *regs)
{
	if (exception_check_mode && exception_check) {
		exception_check(exception_check_mode, regs);
		return 1;
	}
	return 0;
}

static inline void em_show_stack_overflow(struct pt_regs *regs)
{
	if (exception_check_mode && exception_check) {
#ifdef CONFIG_MIPS
		die("stack overflow", regs);
#else
		die("stack overflow", regs, SIGSTKFLT);
#endif
	}
}
#else
static inline int  em_show_coredump(struct pt_regs *regs) { return 0; }
static inline int  em_show(struct pt_regs *regs) { return 0; }
static inline void em_show_stack_overflow(struct pt_regs *regs) {}
#endif /* CONFIG_EXCEPTION_MONITOR */
#endif /* EXCEPTION_MONITOR_H */
