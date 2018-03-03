/* 2012-06-22: File added by Sony Corporation */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/hardirq.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_SMP
DEFINE_PER_CPU(struct mm_struct *, current_mm);
#endif

#if 0
#undef pr_debug
#define pr_debug(fmt, arg...) printk("%s: " fmt, __func__, ##arg)
#endif

#define ASID_WORDS (((1<<ASID_BITS)+31)/32)
#define MAX_ASID ((1<<ASID_BITS) - 1)
static unsigned long asidmap[ASID_WORDS] = {1};
static atomic_t asid_left = ATOMIC_INIT(MAX_ASID);

#ifndef CONFIG_EJ_MAX_CONCURRENT_255

static LIST_HEAD(asid_mm_list);

static DEFINE_RAW_SPINLOCK(asid_lock);

#define RESERVE ((NR_CPUS * 2) + 4)

#define LOCK() raw_spin_lock(&asid_lock)
#define UNLOCK() raw_spin_unlock(&asid_lock)

static void del_asid_mm(struct mm_struct *mm)
{
	LOCK();
	list_del(&mm->context.list);
	UNLOCK();
}

static void add_asid_mm(struct mm_struct *mm)
{
	LOCK();
	list_add_tail(&mm->context.list, &asid_mm_list);
	UNLOCK();
}

static int grab_asid_mm(struct mm_struct *mm)
{
	mm_context_t *mmu;
	LOCK();
	list_for_each_entry(mmu, &asid_mm_list, list) {
		if (!atomic_read(&mmu->online)) {
			spin_lock(&mmu->lock);
			if (!atomic_read(&mmu->online) && mmu->id) {
				list_del(&mmu->list);

				mm->context.id = mmu->id;
				mmu->id = 0;

				list_add_tail(&mm->context.list, &asid_mm_list);

				spin_unlock(&mmu->lock);
				UNLOCK();
				pr_debug("%d grabed\n", mm->context.id);
				return 1;
			}
			spin_unlock(&mmu->lock);
		}
	}
	UNLOCK();
	return 0;
}
#else /*CONFIG_EJ_MAX_CONCURRENT_255*/

#define RESERVE (1)

static void del_asid_mm(struct mm_struct *mm)
{
	flush_tlb_mm(mm);
}

static void add_asid_mm(struct mm_struct *mm)
{
}

static int grab_asid_mm(struct mm_struct *mm)
{
	return 0;
}
#endif

static int alloc_asid(struct mm_struct *mm)
{
	int idx;
	int pos;

	if (atomic_read(&asid_left) < RESERVE) {
		return grab_asid_mm(mm);
	}

	atomic_dec(&asid_left);

	idx = (int)mm >> L1_CACHE_SHIFT;
	do {
		pos = (idx + 1) & MAX_ASID;
		idx = find_next_zero_bit(asidmap, MAX_ASID, pos);
	} while(idx > MAX_ASID || test_and_set_bit(idx, asidmap));

	mm->context.id = idx;

	add_asid_mm(mm);
	return 1;
}

static void free_asid(struct mm_struct *mm)
{
	del_asid_mm(mm);
	clear_bit(mm->context.id, asidmap);
	atomic_inc(&asid_left);
	pr_debug("free asid %d -> %d\n", mm->context.id, atomic_read(&asid_left));
	mm->context.id = 0;
}

#ifndef CONFIG_EJ_MAX_CONCURRENT_255
#define lock_context(mm) spin_lock(&mm->context.lock)
#define unlock_context(mm) spin_unlock(&mm->context.lock)

void __check_context(struct mm_struct *prev, struct mm_struct *next)
{
	atomic_dec(&prev->context.online);
	atomic_inc(&next->context.online);

	lock_context(next);
	if (unlikely(!next->context.id)) {
		alloc_asid(next);
		next->cpu_vm_mask = CPU_MASK_NONE;
		pr_debug("allocated asid %d left %d\n", next->context.id, atomic_read(&asid_left));
	}
	unlock_context(next);

	if (!cpu_test_and_set(smp_processor_id(), next->cpu_vm_mask)) {
		local_flush_tlb_mm(next);
	}
}
#endif

void destroy_context(struct mm_struct *mm)
{
	if (mm->context.id) {
		free_asid(mm);
	}
	else {
		pr_debug("strange id is zero!\n");
	}
}

int init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	mm->context.id = 0;
#ifndef CONFIG_EJ_MAX_CONCURRENT_255
	INIT_LIST_HEAD(&mm->context.list);
	atomic_set(&mm->context.online, 0);
	spin_lock_init(&mm->context.lock);
	alloc_asid(mm);
#else
	if (!alloc_asid(mm)) {
		printk(KERN_ERR "Cannot allocate ASID\n");
		return -1;
	}
#endif

	pr_debug("%s -> %d\n", tsk?tsk->comm:NULL, mm->context.id);
	return 0;
}
