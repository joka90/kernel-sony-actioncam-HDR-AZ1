/* 2013-08-20: File changed by Sony Corporation */
/*
 *  arch/arm/include/asm/thread_info.h
 *
 *  Copyright (C) 2002 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARM_THREAD_INFO_H
#define __ASM_ARM_THREAD_INFO_H

#ifdef __KERNEL__

#include <linux/compiler.h>
#include <asm/fpstate.h>
#include <asm/page.h>

#ifndef CONFIG_SNSC_DEBUG_STACK_LIMIT

#ifndef CONFIG_4KSTACKS
#define THREAD_SIZE_ORDER	1
#else
#define THREAD_SIZE_ORDER	0
#endif

#define THREAD_START_SP		(THREAD_SIZE - 8)

#else /* CONFIG_SNSC_DEBUG_STACK_LIMIT */

#define THREAD_SIZE_ORDER	2

#ifdef CONFIG_SNSC_DEBUG_STACK_LIMIT_MANUAL

#define __STACK_LIMIT_BYTES	(CONFIG_SNSC_DEBUG_STACK_LIMIT_BYTES & ~(8 - 1))
#ifndef __ASSEMBLY__
#define THREAD_START_SP		(PAGE_SIZE + __STACK_LIMIT_BYTES)
#else
#include <asm/asm-offsets.h>
#define THREAD_START_SP		(PAGE_SZ + __STACK_LIMIT_BYTES)
#if THREAD_START_SP > TU_THREAD_INFO - 8
#error CONFIG_SNSC_DEBUG_STACK_LIMIT_BYTES is too large
#endif
#endif

#else	/* !CONFIG_SNSC_DEBUG_STACK_LIMIT_MANUAL */

#ifndef __ASSEMBLY__
#define THREAD_START_SP		(offsetof(union thread_union, thread_info) - 8)
#else
#include <asm/asm-offsets.h>
#define THREAD_START_SP		(TU_THREAD_INFO -8)
#endif

#endif	/* CONFIG_SNSC_DEBUG_STACK_LIMIT_MANUAL */

#endif /* CONFIG_SNSC_DEBUG_STACK_LIMIT */

#define DEF_THREAD_SIZE_ORDER	THREAD_SIZE_ORDER
#define THREAD_SIZE		(4096 << DEF_THREAD_SIZE_ORDER)
#define STACK_SHIFT		PAGE_SHIFT + DEF_THREAD_SIZE_ORDER
#define ALT_STACK_SHIFT		PAGE_SHIFT

#define DEF_STACK_SIZE		THREAD_SIZE
#define ALT_STACK_SIZE		4096

#define DEF_START_SP		THREAD_START_SP
#define ALT_START_SP		ALT_STACK_SIZE-8

#ifdef CONFIG_SNSC_DEBUG_STACK_LIMIT
#define calc_start_sp(addr)	DEF_START_SP
#else
#define calc_start_sp(addr)	(calc_stack_size(addr)-8)
#endif

#ifndef __ASSEMBLY__

#if defined(CONFIG_SNSC_MULTI_STACK_SIZES) || defined(CONFIG_STACK_EXTENSIONS)
extern void *alt_stack_pool_start;
extern void *alt_stack_pool_end;
extern unsigned long get_alt_stack_count(void);
#else
#define alt_stack_pool_start 0
#define alt_stack_pool_end 0
#define get_alt_stack_count() (0)
#endif

#if defined(CONFIG_SNSC_MULTI_STACK_SIZES)
static inline unsigned int _calc_stack_size(void *addr)
{
	/* this calculation needs to be precise */
	if (addr >= alt_stack_pool_start && addr < alt_stack_pool_end)
		return ALT_STACK_SIZE;
	else
		return DEF_STACK_SIZE;
}
#define calc_stack_size(addr)	_calc_stack_size((void *)addr)
#else
#define calc_stack_size(p) (DEF_STACK_SIZE)
#endif

struct task_struct;
struct exec_domain;

#include <asm/types.h>
#include <asm/domain.h>

typedef unsigned long mm_segment_t;

struct cpu_context_save {
	__u32	r4;
	__u32	r5;
	__u32	r6;
	__u32	r7;
	__u32	r8;
	__u32	r9;
	__u32	sl;
	__u32	fp;
	__u32	sp;
	__u32	pc;
	__u32	extra[2];		/* Xscale 'acc' register, etc */
};

/*
 * low level task data that entry.S needs immediate access to.
 * __switch_to() assumes cpu_context follows immediately after cpu_domain.
 */
struct thread_info {
	unsigned long		flags;		/* low level flags */
	int			preempt_count;	/* 0 => preemptable, <0 => bug */
#ifdef CONFIG_THREAD_INFO_INDIRECT
	struct thread_info	*ti_ptr;	/* pointer to real ti */
#endif
	mm_segment_t		addr_limit;	/* address limit */
	struct task_struct	*task;		/* main task structure */
	struct exec_domain	*exec_domain;	/* execution domain */
	__u32			cpu;		/* cpu */
	__u32			cpu_domain;	/* cpu domain */
	struct cpu_context_save	cpu_context;	/* cpu context */
	__u32			syscall;	/* syscall number */
	__u8			used_cp[16];	/* thread used copro */
	unsigned long		tp_value;
#if !defined(CONFIG_EJ_REDUCE_THREADINFO) || defined(CONFIG_CRUNCH)
	struct crunch_state	crunchstate;
#endif
#if !defined(CONFIG_EJ_REDUCE_THREADINFO) || defined(CONFIG_HAVE_FPSTATE)
	union fp_state		fpstate __attribute__((aligned(8)));
#endif
#if !defined(CONFIG_EJ_REDUCE_THREADINFO) || defined(CONFIG_VFP)
	union vfp_state		vfpstate;
#endif
#ifdef CONFIG_SNSC_VFP_DETECTION_IOCTL
	int			is_ioctl_context;
#endif
#ifdef CONFIG_ARM_THUMBEE
	unsigned long		thumbee_state;	/* ThumbEE Handler Base register */
#endif
	struct restart_block	restart_block;
};

#ifdef CONFIG_THREAD_INFO_INDIRECT
#define TI_PTR_INIT	.ti_ptr = &init_thread_union.thread_info,
#else
#define TI_PTR_INIT
#endif

#define INIT_THREAD_INFO(tsk)						\
{									\
	.task		= &tsk,						\
	.exec_domain	= &default_exec_domain,				\
	.flags		= 0,						\
	TI_PTR_INIT							\
	.preempt_count	= INIT_PREEMPT_COUNT,				\
	.addr_limit	= KERNEL_DS,					\
	.cpu_domain	= domain_val(DOMAIN_USER, DOMAIN_MANAGER) |	\
			  domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) |	\
			  domain_val(DOMAIN_IO, DOMAIN_CLIENT),		\
	.restart_block	= {						\
		.fn	= do_no_restart_syscall,			\
	},								\
}

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)

#define __HAVE_ARCH_THREAD_UNION
#ifdef CONFIG_EJ
# define __ARCH_THREAD_INFO_SIZE       (sizeof(struct thread_info)+sizeof(long))
#else
# define __ARCH_THREAD_INFO_SIZE       (sizeof(struct thread_info))
#endif /* CONFIG_EJ */

union thread_union {
	struct {
#ifdef CONFIG_SNSC_DEBUG_STACK_LIMIT
		unsigned long pad[(THREAD_SIZE-__ARCH_THREAD_INFO_SIZE)/sizeof(long)];
#endif
		struct thread_info thread_info;
#ifdef CONFIG_EJ
		/* for private thread_info space */
		unsigned long __pad2;
#endif /* CONFIG_EJ */
	} __attribute__((packed));
	unsigned long stack[DEF_STACK_SIZE/sizeof(long)];
};
/* static assertion */
extern char __ASSERTION_FAIL__size_of_thread_union_is_not_THREAD_SIZE[(sizeof (union thread_union) == THREAD_SIZE) ? 0:-1];

#ifdef CONFIG_SNSC_DEBUG_STACK_LIMIT
#define __HAVE_ARCH_THREAD_INFO_ALLOCATOR
struct thread_info *alloc_thread_info(struct task_struct *tsk);
struct thread_info *alloc_thread_info_node(struct task_struct *tsk, int node);
void free_thread_info(struct thread_info *ti);

void map_addr(unsigned long addr);
void unmap_addr(unsigned long addr);
int fault_on_stack(unsigned long addr);
#endif

/*
 * how to get the thread information struct from C
 */
static inline struct thread_info *current_thread_info(void) __attribute_const__;

#ifndef CONFIG_THREAD_INFO_INDIRECT
static inline struct thread_info *current_thread_info(void)
{
	register unsigned long sp asm ("sp");
	if (calc_stack_size(sp) == DEF_STACK_SIZE)
		return &((union thread_union *)(sp & ~(DEF_STACK_SIZE - 1)))->thread_info;
	else
		return &((union thread_union *)(sp & ~(ALT_STACK_SIZE - 1)))->thread_info;
}

#else
static inline struct thread_info *current_thread_info(void)
{
	register unsigned long sp asm ("sp");
	if (calc_stack_size(sp) == DEF_STACK_SIZE)
		return ((union thread_union *)(sp & ~(DEF_STACK_SIZE - 1)))->thread_info.ti_ptr;
	else
		return ((union thread_union *)(sp & ~(ALT_STACK_SIZE - 1)))->thread_info.ti_ptr;
}
#endif

static inline unsigned long *current_stack_page(void)
{
	register unsigned long sp asm ("sp");
	if (calc_stack_size(sp) == DEF_STACK_SIZE)
		return (unsigned long *)(sp & ~(DEF_STACK_SIZE - 1));
	else
		return (unsigned long *)(sp & ~(ALT_STACK_SIZE - 1));
}

#define thread_saved_pc(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.pc))
#define thread_saved_sp(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.sp))
#define thread_saved_fp(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.fp))

extern void crunch_task_disable(struct thread_info *);
extern void crunch_task_copy(struct thread_info *, void *);
extern void crunch_task_restore(struct thread_info *, void *);
extern void crunch_task_release(struct thread_info *);

extern void iwmmxt_task_disable(struct thread_info *);
extern void iwmmxt_task_copy(struct thread_info *, void *);
extern void iwmmxt_task_restore(struct thread_info *, void *);
extern void iwmmxt_task_release(struct thread_info *);
extern void iwmmxt_task_switch(struct thread_info *);

extern void vfp_sync_hwstate(struct thread_info *);
extern void vfp_flush_hwstate(struct thread_info *);

struct user_vfp;
struct user_vfp_exc;

extern int vfp_preserve_user_clear_hwstate(struct user_vfp __user *,
					   struct user_vfp_exc __user *);
extern int vfp_restore_user_hwstate(struct user_vfp __user *,
				    struct user_vfp_exc __user *);
#endif

/*
 * We use bit 30 of the preempt_count to indicate that kernel
 * preemption is occurring.  See <asm/hardirq.h>.
 */
#define PREEMPT_ACTIVE	0x40000000

/*
 * thread information flags:
 *  TIF_SYSCALL_TRACE	- syscall trace active
 *  TIF_KERNEL_TRACE	- kernel trace active
 *  TIF_SIGPENDING	- signal pending
 *  TIF_NEED_RESCHED	- rescheduling necessary
 *  TIF_NOTIFY_RESUME	- callback before returning to user
 *  TIF_USEDFPU		- FPU was used by this task this quantum (SMP)
 *  TIF_POLLING_NRFLAG	- true if poll_idle() is polling TIF_NEED_RESCHED
 */
#define TIF_SIGPENDING		0
#define TIF_NEED_RESCHED	1
#define TIF_NOTIFY_RESUME	2	/* callback before returning to user */
#define TIF_KERNEL_TRACE	7
#define TIF_SYSCALL_TRACE	8
#define TIF_SYSCALL_TRACEPOINT	15
#define TIF_POLLING_NRFLAG	16
#define TIF_USING_IWMMXT	17
#define TIF_MEMDIE		18	/* is terminating due to OOM killer */
#define TIF_FREEZE		19
#define TIF_RESTORE_SIGMASK	20
#define TIF_SECCOMP		21

#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_KERNEL_TRACE	(1 << TIF_KERNEL_TRACE)
#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_SYSCALL_TRACEPOINT	(1 << TIF_SYSCALL_TRACEPOINT)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)
#define _TIF_USING_IWMMXT	(1 << TIF_USING_IWMMXT)
#define _TIF_FREEZE		(1 << TIF_FREEZE)
#define _TIF_RESTORE_SIGMASK	(1 << TIF_RESTORE_SIGMASK)
#define _TIF_SECCOMP		(1 << TIF_SECCOMP)
#define _TIF_SYSCALL_T_OR_A	(_TIF_SYSCALL_TRACE | _TIF_SYSCALL_TRACEPOINT)

/*
 * Change these and you break ASM code in entry-common.S
 */
#define _TIF_WORK_MASK		0x000000ff

#endif /* __KERNEL__ */
#endif /* __ASM_ARM_THREAD_INFO_H */
