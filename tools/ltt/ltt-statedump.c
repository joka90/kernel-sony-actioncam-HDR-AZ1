/* 2011-10-04: File added and changed by Sony Corporation */
/*
 * Linux Trace Toolkit Kernel State Dump
 *
 * Copyright 2005 -
 * Jean-Hugues Deschenes <jean-hugues.deschenes@polymtl.ca>
 *
 * Changes:
 *	Eric Clement:                   Add listing of network IP interface
 *	2006, 2007 Mathieu Desnoyers	Fix kernel threads
 *	                                Various updates
 *
 * Dual LGPL v2.1/GPL v2 license.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/irqnr.h>
#include <linux/cpu.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/marker.h>
#include <linux/fdtable.h>
#include <linux/swap.h>
#include <linux/wait.h>
#include <linux/mutex.h>

#include "ltt-tracer.h"

#ifdef CONFIG_GENERIC_HARDIRQS
#include <linux/irq.h>
#endif

#ifdef CONFIG_HAVE_KVM
#include <asm/vmx.h>
#endif

#define NB_PROC_CHUNK 20

/*
 * Protected by the trace lock.
 */
static struct delayed_work cpu_work[NR_CPUS];
static DECLARE_WAIT_QUEUE_HEAD(statedump_wq);
static atomic_t kernel_threads_to_run;

static void empty_cb(void *call_data)
{
}

static DEFINE_MUTEX(statedump_cb_mutex);
static void (*ltt_dump_kprobes_table_cb)(void *call_data) = empty_cb;

enum lttng_thread_type {
	LTTNG_USER_THREAD = 0,
	LTTNG_KERNEL_THREAD = 1,
};

enum lttng_execution_mode {
	LTTNG_USER_MODE = 0,
	LTTNG_SYSCALL = 1,
	LTTNG_TRAP = 2,
	LTTNG_IRQ = 3,
	LTTNG_SOFTIRQ = 4,
	LTTNG_MODE_UNKNOWN = 5,
};

enum lttng_execution_submode {
	LTTNG_NONE = 0,
	LTTNG_UNKNOWN = 1,
};

enum lttng_process_status {
	LTTNG_UNNAMED = 0,
	LTTNG_WAIT_FORK = 1,
	LTTNG_WAIT_CPU = 2,
	LTTNG_EXIT = 3,
	LTTNG_ZOMBIE = 4,
	LTTNG_WAIT = 5,
	LTTNG_RUN = 6,
	LTTNG_DEAD = 7,
};

struct trace_enum_map {
	long id;
	const char *symbol;
};

#ifdef CONFIG_HAVE_KVM
static const struct trace_enum_map vmx_kvm_exit_enum[] = {
	{ EXIT_REASON_EXCEPTION_NMI,            "exception" },
	{ EXIT_REASON_EXTERNAL_INTERRUPT,       "ext_irq" },
	{ EXIT_REASON_TRIPLE_FAULT,             "triple_fault" },
	{ EXIT_REASON_PENDING_INTERRUPT,        "interrupt_window" },
	{ EXIT_REASON_NMI_WINDOW,               "nmi_window" },
	{ EXIT_REASON_TASK_SWITCH,              "task_switch" },
	{ EXIT_REASON_CPUID,                    "cpuid" },
	{ EXIT_REASON_HLT,                      "halt" },
	{ EXIT_REASON_INVLPG,                   "invlpg" },
	{ EXIT_REASON_RDPMC,                    "rdpmc" },
	{ EXIT_REASON_RDTSC,                    "rdtsc" },
	{ EXIT_REASON_VMCALL,                   "hypercall" },
	{ EXIT_REASON_VMCLEAR,                  "vmclear" },
	{ EXIT_REASON_VMLAUNCH,                 "vmlaunch" },
	{ EXIT_REASON_VMPTRLD,                  "vmprtld" },
	{ EXIT_REASON_VMPTRST,                  "vmptrst" },
	{ EXIT_REASON_VMREAD,                   "vmread" },
	{ EXIT_REASON_VMRESUME,                 "vmresume" },
	{ EXIT_REASON_VMWRITE,                  "vmwrite" },
	{ EXIT_REASON_VMOFF,                    "vmoff" },
	{ EXIT_REASON_VMON,                     "vmon" },
	{ EXIT_REASON_CR_ACCESS,                "cr_access" },
	{ EXIT_REASON_DR_ACCESS,                "dr_access" },
	{ EXIT_REASON_IO_INSTRUCTION,           "io_instruction" },
	{ EXIT_REASON_MSR_READ,                 "rdmsr" },
	{ EXIT_REASON_MSR_WRITE,                "wrmsr" },
	{ EXIT_REASON_MWAIT_INSTRUCTION,        "mwait_instruction" },
	{ EXIT_REASON_MONITOR_INSTRUCTION,      "monitor_instruction" },
	{ EXIT_REASON_PAUSE_INSTRUCTION,        "pause_instruction" },
	{ EXIT_REASON_MCE_DURING_VMENTRY,       "mce_during_vmentry" },
	{ EXIT_REASON_TPR_BELOW_THRESHOLD,      "tpr_below_thres" },
	{ EXIT_REASON_APIC_ACCESS,              "apic_access" },
	{ EXIT_REASON_EPT_VIOLATION,            "ept_violation" },
	{ EXIT_REASON_EPT_MISCONFIG,            "epg_misconfig" },
	{ EXIT_REASON_WBINVD,                   "wbinvd" },
	{ -1, NULL }
};
#endif /* CONFIG_HAVE_KVM */

static void ltt_dump_enum_tables(struct ltt_probe_private_data *call_data)
{
#ifdef CONFIG_HAVE_KVM
	int i;
	/* KVM exit reasons for VMX */
	for(i = 0; vmx_kvm_exit_enum[i].symbol; i++) {
		__trace_mark(0, enum_tables, vmx_kvm_exit, call_data,
				"id %ld symbol %s", vmx_kvm_exit_enum[i].id,
				vmx_kvm_exit_enum[i].symbol);
	}
#endif /* CONFIG_HAVE_KVM */
}

#ifdef CONFIG_INET
static void ltt_enumerate_device(struct ltt_probe_private_data *call_data,
				 struct net_device *dev)
{
	struct in_device *in_dev;
	struct in_ifaddr *ifa;

	if (dev->flags & IFF_UP) {
		in_dev = in_dev_get(dev);
		if (in_dev) {
			for (ifa = in_dev->ifa_list; ifa != NULL;
			     ifa = ifa->ifa_next)
				__trace_mark(0, netif_state,
					     network_ipv4_interface,
					     call_data,
					     "name %s address #n4u%lu up %d",
					     dev->name,
					     (unsigned long)ifa->ifa_address,
					     0);
			in_dev_put(in_dev);
		}
	} else
		__trace_mark(0, netif_state, network_ip_interface,
			     call_data, "name %s address #n4u%lu up %d",
			     dev->name, 0UL, 0);
}

static inline int
ltt_enumerate_network_ip_interface(struct ltt_probe_private_data *call_data)
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	for_each_netdev(&init_net, dev)
		ltt_enumerate_device(call_data, dev);
	read_unlock(&dev_base_lock);

	return 0;
}
#else /* CONFIG_INET */
static inline int
ltt_enumerate_network_ip_interface(struct ltt_probe_private_data *call_data)
{
	return 0;
}
#endif /* CONFIG_INET */


static inline void
ltt_enumerate_task_fd(struct ltt_probe_private_data *call_data,
		      struct task_struct *t, char *tmp)
{
	struct fdtable *fdt;
	struct file *filp;
	unsigned int i;
	const unsigned char *path;

	if (!t->files)
		return;

	spin_lock(&t->files->file_lock);
	fdt = files_fdtable(t->files);
	for (i = 0; i < fdt->max_fds; i++) {
		filp = fcheck_files(t->files, i);
		if (!filp)
			continue;
		path = d_path(&filp->f_path, tmp, PAGE_SIZE);
		/* Make sure we give at least some info */
		__trace_mark(0, fd_state, file_descriptor, call_data,
			     "filename %s pid %d fd %u",
			     (IS_ERR(path))?(filp->f_dentry->d_name.name):(path),
			     t->pid, i);
	}
	spin_unlock(&t->files->file_lock);
}

static inline int
ltt_enumerate_file_descriptors(struct ltt_probe_private_data *call_data)
{
	struct task_struct *t = &init_task;
	char *tmp = (char *)__get_free_page(GFP_KERNEL);

	/* Enumerate active file descriptors */
	do {
		read_lock(&tasklist_lock);
		if (t != &init_task)
			atomic_dec(&t->usage);
		t = next_task(t);
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);
		task_lock(t);
		ltt_enumerate_task_fd(call_data, t, tmp);
		task_unlock(t);
	} while (t != &init_task);
	free_page((unsigned long)tmp);
	return 0;
}

static inline void
ltt_enumerate_task_vm_maps(struct ltt_probe_private_data *call_data,
		struct task_struct *t)
{
	struct mm_struct *mm;
	struct vm_area_struct *map;
	unsigned long ino;

	/* get_task_mm does a task_lock... */
	mm = get_task_mm(t);
	if (!mm)
		return;

	map = mm->mmap;
	if (map) {
		down_read(&mm->mmap_sem);
		while (map) {
			if (map->vm_file)
				ino = map->vm_file->f_dentry->d_inode->i_ino;
			else
				ino = 0;
			__trace_mark(0, vm_state, vm_map, call_data,
				     "pid %d start %lu end %lu flags %lu "
				     "pgoff %lu inode %lu",
				     t->pid, map->vm_start, map->vm_end,
				     map->vm_flags, map->vm_pgoff << PAGE_SHIFT,
				     ino);
			map = map->vm_next;
		}
		up_read(&mm->mmap_sem);
	}
	mmput(mm);
}

static inline int
ltt_enumerate_vm_maps(struct ltt_probe_private_data *call_data)
{
	struct task_struct *t = &init_task;

	do {
		read_lock(&tasklist_lock);
		if (t != &init_task)
			atomic_dec(&t->usage);
		t = next_task(t);
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);
		ltt_enumerate_task_vm_maps(call_data, t);
	} while (t != &init_task);
	return 0;
}

#ifdef CONFIG_GENERIC_HARDIRQS
static inline void list_interrupts(struct ltt_probe_private_data *call_data)
{
	unsigned int irq;
	unsigned long flags = 0;
	struct irq_desc *desc;

	/* needs irq_desc */
	for_each_irq_desc(irq, desc) {
		struct irqaction *action;
		const char *irq_chip_name =
			desc->irq_data.chip->name ? : "unnamed_irq_chip";

		local_irq_save(flags);
		raw_spin_lock(&desc->lock);
		for (action = desc->action; action; action = action->next)
			__trace_mark(0, irq_state, interrupt, call_data,
				     "name %s action %s irq_id %u",
				     irq_chip_name, action->name, irq);
		raw_spin_unlock(&desc->lock);
		local_irq_restore(flags);
	}
}
#else
static inline void list_interrupts(struct ltt_probe_private_data *call_data)
{
}
#endif

static inline int
ltt_enumerate_process_states(struct ltt_probe_private_data *call_data)
{
	struct task_struct *t = &init_task;
	struct task_struct *p = t;
	enum lttng_process_status status;
	enum lttng_thread_type type;
	enum lttng_execution_mode mode;
	enum lttng_execution_submode submode;

	do {
		mode = LTTNG_MODE_UNKNOWN;
		submode = LTTNG_UNKNOWN;

		read_lock(&tasklist_lock);
		if (t != &init_task) {
			atomic_dec(&t->usage);
			t = next_thread(t);
		}
		if (t == p) {
			p = next_task(t);
			t = p;
		}
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);

		task_lock(t);

		if (t->exit_state == EXIT_ZOMBIE)
			status = LTTNG_ZOMBIE;
		else if (t->exit_state == EXIT_DEAD)
			status = LTTNG_DEAD;
		else if (t->state == TASK_RUNNING) {
			/* Is this a forked child that has not run yet? */
			if (list_empty(&t->rt.run_list))
				status = LTTNG_WAIT_FORK;
			else
				/*
				 * All tasks are considered as wait_cpu;
				 * the viewer will sort out if the task was
				 * really running at this time.
				 */
				status = LTTNG_WAIT_CPU;
		} else if (t->state &
			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)) {
			/* Task is waiting for something to complete */
			status = LTTNG_WAIT;
		} else
			status = LTTNG_UNNAMED;
		submode = LTTNG_NONE;

		/*
		 * Verification of t->mm is to filter out kernel threads;
		 * Viewer will further filter out if a user-space thread was
		 * in syscall mode or not.
		 */
		if (t->mm)
			type = LTTNG_USER_THREAD;
		else
			type = LTTNG_KERNEL_THREAD;

		__trace_mark(0, task_state, process_state, call_data,
			     "pid %d parent_pid %d name %s type %d mode %d "
			     "submode %d status %d tgid %d",
			     t->pid, t->parent->pid, t->comm,
			     type, mode, submode, status, t->tgid);
		task_unlock(t);
	} while (t != &init_task);

	return 0;
}

void ltt_statedump_register_kprobes_dump(void (*callback)(void *call_data))
{
	mutex_lock(&statedump_cb_mutex);
	ltt_dump_kprobes_table_cb = callback;
	mutex_unlock(&statedump_cb_mutex);
}
EXPORT_SYMBOL_GPL(ltt_statedump_register_kprobes_dump);

void ltt_statedump_unregister_kprobes_dump(void (*callback)(void *call_data))
{
	mutex_lock(&statedump_cb_mutex);
	ltt_dump_kprobes_table_cb = empty_cb;
	mutex_unlock(&statedump_cb_mutex);
}
EXPORT_SYMBOL_GPL(ltt_statedump_unregister_kprobes_dump);

void ltt_statedump_work_func(struct work_struct *work)
{
	if (atomic_dec_and_test(&kernel_threads_to_run))
		/* If we are the last thread, wake up do_ltt_statedump */
		wake_up(&statedump_wq);
}

static int do_ltt_statedump(struct ltt_probe_private_data *call_data)
{
	int cpu;
	struct module *cb_owner;

	printk(KERN_DEBUG "LTT state dump thread start\n");
	ltt_enumerate_process_states(call_data);
	ltt_enumerate_file_descriptors(call_data);
	list_modules(call_data);
	ltt_enumerate_vm_maps(call_data);
	list_interrupts(call_data);
	ltt_enumerate_network_ip_interface(call_data);
	ltt_dump_swap_files(call_data);
	ltt_dump_sys_call_table(call_data);
	ltt_dump_softirq_vec(call_data);
	ltt_dump_idt_table(call_data);
	ltt_dump_enum_tables(call_data);

	mutex_lock(&statedump_cb_mutex);

	cb_owner = __module_address((unsigned long)ltt_dump_kprobes_table_cb);
	__module_get(cb_owner);
	ltt_dump_kprobes_table_cb(call_data);
	module_put(cb_owner);

	mutex_unlock(&statedump_cb_mutex);

	/*
	 * Fire off a work queue on each CPU. Their sole purpose in life
	 * is to guarantee that each CPU has been in a state where is was in
	 * syscall mode (i.e. not in a trap, an IRQ or a soft IRQ).
	 */
	get_online_cpus();
	atomic_set(&kernel_threads_to_run, num_online_cpus());
	for_each_online_cpu(cpu) {
		INIT_DELAYED_WORK(&cpu_work[cpu], ltt_statedump_work_func);
		schedule_delayed_work_on(cpu, &cpu_work[cpu], 0);
	}
	/* Wait for all threads to run */
	__wait_event(statedump_wq, (atomic_read(&kernel_threads_to_run) != 0));
	put_online_cpus();
	/* Our work is done */
	printk(KERN_DEBUG "LTT state dump end\n");
	__trace_mark(0, global_state, statedump_end,
		     call_data, MARK_NOARGS);
	return 0;
}

/*
 * Called with trace lock held.
 */
int ltt_statedump_start(struct ltt_trace *trace)
{
	struct ltt_probe_private_data call_data;
	printk(KERN_DEBUG "LTT state dump begin\n");

	call_data.trace = trace;
	call_data.serializer = NULL;
	return do_ltt_statedump(&call_data);
}

static int __init statedump_init(void)
{
	int ret;
	printk(KERN_DEBUG "LTT : State dump init\n");
	ret = ltt_module_register(LTT_FUNCTION_STATEDUMP,
			ltt_statedump_start, THIS_MODULE);
	return ret;
}

static void __exit statedump_exit(void)
{
	printk(KERN_DEBUG "LTT : State dump exit\n");
	ltt_module_unregister(LTT_FUNCTION_STATEDUMP);
}

module_init(statedump_init)
module_exit(statedump_exit)

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Jean-Hugues Deschenes");
MODULE_DESCRIPTION("Linux Trace Toolkit Statedump");
