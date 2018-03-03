/* 2013-03-21: File added and changed by Sony Corporation */
/*
 *  stack-ext.c - A stack extender system for Linux
 *
 *  This is primarily targetted at embedded systems configured
 *  for small stack usage.
 *
 *  Copyright 2012 Sony Corporation
 */
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/thread_info.h>
#include <linux/stack-ext.h>

unsigned long stack_extension_failures;
unsigned long stack_extensions;

/* FIXTHIS - implement check_stack_depth() */
void *prep_new_stack(void)
{
	struct task_struct *tsk;
	int node;
	struct thread_info *new_ti, *real_ti;
	void *old_stack;
	register unsigned long sp asm ("sp");

	old_stack = (void *)sp;

	/* allocate a new stack page */
	tsk = current;
	real_ti = task_thread_info(tsk);
	node = tsk_fork_get_node(tsk);
	/* NOTE: new_ti is fake, on a non-primary stack.
	 * The only field used is ti_ptr
	 * Note also that we are not calling alloc_thread_info_node() here
	 * In order to keep extension stacks as lightweight as possible, we
	 * use the special alternate allocator (from a small, fixed pool of
	 * stack pages).
         */
	new_ti = alloc_alt_thread_info(0);
	if (!new_ti) {
		printk(KERN_CRIT "Could not extend stack!\n");
		stack_extension_failures++;
		return NULL;
	}
	stack_extensions++;

	/* set parent_stack in new stack thread_info */
	new_ti->ti_ptr = real_ti;

	/* DEFERRED prepare for overflow detection - is this needed?? */
	//stackend = end_of_stack(tsk);
	//*stackend = STACK_END_MAGIC;

	/* DEFERRED */
	//account_kernel_stack(new_ti, 1);

	return new_ti;
}

#if 0
/* ifdef out this code that implements a more general, 2-functon, solution
   to stack extension.  Originally, stack extensions were to be made only
   when the stack was getting low, and these routines were to be called from
   an mcount routine on entry to every kernel function.  However, that was
   deemed to have too much overhead.  Instead, the current stack extensions
   system only moves to a new stack inside handle_IRQ(), and it does it
   every time.  This remove one of the primary culprits of deep stack usage,
   and seems to be sufficient.  Possibly, other specific locations could
   be chosen to 'break' the stack onto different pages, to avoid overnesting.
   (One other spot might be in the NFS code, where the stack gets quite deep.)
*/
int extend_stack(void)
{
	void *new_stack_top;

	new_stack_top = prep_new_stack();
	if (!new_stack_top) {
		return -1;
	}
	/* FIXTHIS - should calculate the real top of stack rather
	 * than use a hardcoded page size value!
	 */
	new_stack_top = ((void *)new_stack_top) + 4096-4;

	/* move sp to new stack */
	/* top of new stack has old stack sp */
	asm(
		"pop {fp, lr}\n\t"	/* restore lr, for new frame */
		"mov ip, sp\n\t"
		"mov sp, %0\n\t"	/* move to new stack */
		"push {ip}\n\t"		/* save old sp to new stack */
		"push {fp, lr}"		/* create frame for local return */
		:
		: "Ir" (new_stack_top)
		: "ip");

	/* return to caller (on new stack) */
	return 0;
}

/* should be called when parent_stack != 0 and we're at top of stack */
void *unextend_stack(void)
{
	void *this_stack;
	struct thread_info *ti;
	void *previous_sp;
	register unsigned long sp asm ("sp");

	this_stack = (void *)((sp >> (STACK_SHIFT))<<(STACK_SHIFT));
	//printk("this_stack=%p\n", this_stack);
	ti = current_thread_info();
	//printk("ti=%p\n", ti);

	/* if on primary stack, return */
	if (this_stack == (void *)ti)
		return 0;

	/* top of new stack has old stack sp */
	/* move sp to previous stack */
	asm(
		"pop {r4, lr}\n\t"	/* clear stack frame */
		"mov r0, sp\n\t"	/* save current stack pointer */
		"pop {ip}\n\t"		/* get old stack pointer */
		"mov sp, ip\n\t"	/* switch stacks */
		"push {r4, lr}"		/* rebuild stack frame on old stack */
		: "=r" (previous_sp)
		:
		: "ip");

	/* free secondary stack */
	/* FIXTHIS - this is something of a misnomer now,
	 * we're just freeing the stack page
	 */
	previous_sp = (void *)(((unsigned)previous_sp >> (STACK_SHIFT))<<(STACK_SHIFT));
	free_alt_thread_info(previous_sp);
	return ti;
}
#endif

void __handle_irq_possibly_extending_stack(unsigned int irq)
{
	void *new_stack;
	void *extension_stack;

	new_stack = prep_new_stack();
	if (!new_stack) {
		__handle_irq(irq);
		return;
	}

	/* move sp to new stack and call __handle_irq()*/
	/* top of new stack has old stack sp */
	asm(
		"mov ip, sp\n\t"
		"mov sp, %1\n\t"	/* move to new stack */
		"push {ip}\n\t"		/* save old sp to new stack */
		"mov r0, %2\n\t"	/* set r0=irq */
		"bl __handle_irq\n\t"	/* call __handle_irq */
		"mov %0, sp\n\t"	/* save sp from stack extension */
		"pop {ip}\n\t"		/* get old stack pointer */
		"mov sp, ip\n\t"	/* switch stacks */
		: "=r" (extension_stack)
		: "r" (new_stack+4096-4), "r" (irq)
		: "r0", "ip");

	/* free extension stack */
	extension_stack = (void *)(((unsigned)extension_stack >> (STACK_SHIFT))<<(STACK_SHIFT));
	free_alt_thread_info(extension_stack);
	return;
}
