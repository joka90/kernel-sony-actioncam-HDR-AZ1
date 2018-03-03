/* 2013-03-21: File added by Sony Corporation */
/*
 * include/linux/backtracetest.h
 *
 * macros and definitions for adding backtrace self-testing to
 * various kernel locations.
 */

#ifndef _LINUX_BACKTRACETEST_H
#define _LINUX_BACKTRACETEST_H

#ifdef CONFIG_BACKTRACE_SELF_TEST
extern int backtrace_regression_test(void);
extern int backtrace_test_irq_target;
extern int backtrace_test_irq_count;
extern void backtrace_test_hard_irq(void);
#define backtrace_self_test_irq() \
	if (unlikely(backtrace_test_irq_target) && \
		backtrace_test_irq_count < backtrace_test_irq_target) { \
		backtrace_test_irq_count++; \
		printk("=== [ Dumping stack in function: %s ]===\n", __func__); \
		backtrace_test_hard_irq(); \
	}
#else
#define backtrace_regression_test() do { } while (0)
#define backtrace_self_test_irq() do { } while(0)
#endif


#endif	/* _LINUX_BACKTRACE_SELF_TEST_H */
