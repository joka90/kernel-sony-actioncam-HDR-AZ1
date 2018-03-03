/* 2011-12-15: File added by Sony Corporation */
#ifndef _ASM_ARM_SYSCALL_H
#define _ASM_ARM_SYSCALL_H

extern const unsigned long sys_call_table[];

#include <linux/sched.h>

static inline long syscall_get_nr(struct task_struct *task,
				  struct pt_regs *regs)
{
	return regs->ARM_r7;
}

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->ARM_r0;
}

static inline void syscall_get_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned int i, unsigned int n,
					 unsigned long *args)
{
	BUG_ON(i);
	switch (n) {
	case 6:
		args[5] = regs->ARM_r5; /* fall through */
	case 5:
		args[4] = regs->ARM_r4;
	case 4:
		args[3] = regs->ARM_r3;
	case 3:
		args[2] = regs->ARM_r2;
	case 2:
		args[1] = regs->ARM_r1;
	case 1:
		args[0] = regs->ARM_r0;
	case 0:
		break;
	default:
		BUG();
	}
}
#endif	/* _ASM_ARM_SYSCALL_H */
