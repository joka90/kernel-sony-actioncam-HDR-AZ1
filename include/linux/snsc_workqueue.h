/* 2012-06-28: File added and changed by Sony Corporation */
/*
 * workqueue_sony.h --- sony's implementation of 'classic' work queues.
 */

#ifndef _LINUX_WORKQUEUE_SNSC_H
#define _LINUX_WORKQUEUE_SNSC_H

#include <linux/timer.h>
#include <linux/linkage.h>
#include <linux/bitops.h>
#include <linux/lockdep.h>
#include <asm/atomic.h>

struct snsc_workqueue_struct;

struct snsc_work_struct;

typedef void (*snsc_work_func_t)(struct snsc_work_struct *work);

struct snsc_work_struct {
	struct list_head entry;
	snsc_work_func_t func;
};

/*
 * initialize a work item's function pointer
 */
#define SNSC_PREPARE_WORK(_work, _func)	\
	do {	\
		(_work)->func = (_func);	\
	} while (0)

#define __SNSC_INIT_WORK(_work, _func, _onstack)	\
	do {	\
		INIT_LIST_HEAD(&(_work)->entry);	\
		SNSC_PREPARE_WORK((_work), (_func));	\
	} while (0)

#define SNSC_INIT_WORK(_work, _func)	\
	do {	\
		__SNSC_INIT_WORK((_work), (_func), 0);	\
	} while (0)

#define SNSC_INIT_WORK_ON_STACK(_work, _func)	\
	do {	\
		__SNSC_INIT_WORK((_work), (_func), 1);	\
	} while (0)



extern int snsc_queue_work_on(int cpu, struct snsc_workqueue_struct *wq,
						struct snsc_work_struct *work);
extern int snsc_queue_work(struct snsc_workqueue_struct *wq,
						struct snsc_work_struct *work);
extern void snsc_destroy_workqueue(struct snsc_workqueue_struct *wq);
extern struct snsc_workqueue_struct *snsc_create_workqueue(const char *name,
							int rt, int nice);

#ifdef CONFIG_EJ_ON_EACH_CPU_THREADED
extern int snsc_schedule_work_cpu(struct snsc_work_struct *work, int cpu);
#endif

#endif /* _LINUX_WORKQUEUE_SNSC_H */

