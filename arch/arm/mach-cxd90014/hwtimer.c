/*
 * hwtimer.c
 *
 * Hardware timer driver for CXD90014
 *
 * Copyright 2012 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/udif/module.h>
#include <linux/udif/io.h>

#include <mach/regs-timer.h>
#include <mach/hwtimer.h>

#define HWTIMER_DEBUG_PRINT

#if defined(HWTIMER_DEBUG) || defined(HWTIMER_DEBUG_PRINT)
#define DPRINTK(fmt ... ) printk(fmt)
#else
#define DPRINTK(fmt ... )
#endif

static struct hwtimer_info {
	UDIF_VA va;
	struct list_head list;
	raw_spinlock_t lock;
#ifdef HWTIMER_DEBUG
	unsigned int limit_over;
#endif
} hwtimer_info[UDIF_NR_HWTIMER] __cacheline_aligned;

#define HWTIMER_NAME "hwtimer"

#define HWTIMER_REG(va,x) ((va) + (x))

#define TMCK_1MHZ			TMCK_DIV4

#define HWTIMER_TMCTL_TIMER_STOP	(TMCS_FREERUN | TMCK_1MHZ)
#define HWTIMER_TMCTL_INT_OFF		(TMST | TMCS_FREERUN | TMCK_1MHZ)
#define HWTIMER_TMCTL_INT_ON		(TMST | TMINT | TMCS_FREERUN | TMCK_1MHZ)
#define HWTIMER_TMCTL_OFF		(TMCS_FREERUN | TMCK_1MHZ)
#define HWTIMER_TMCR_RESET		(TMCLR | TMINTCLR)
#define HWTIMER_TMCR_RESET_INT		(TMINTCLR)

#define HWTIMER_OFFSET 10 /* 10usec is the performance limits offset */

static int hwtimer_select_cpu(int cpu)
{
	if ((cpu < 0) || (UDIF_NR_HWTIMER <= cpu))
		cpu = raw_smp_processor_id();

	return cpu;
}

static unsigned long hwtimer_read_clock_ch(UDIF_VA va)
{
	return readl_relaxed(HWTIMER_REG(va, CXD90014_TIMERREAD));
}

unsigned long hwtimer_read_clock_cpu(int cpu)
{
	BUG_ON((cpu < 0) || (UDIF_NR_HWTIMER <= cpu));
	return readl_relaxed(HWTIMER_REG(hwtimer_info[cpu].va, CXD90014_TIMERREAD));
}

unsigned long hwtimer_read_clock(void)
{
	return readl_relaxed(HWTIMER_REG(hwtimer_info[raw_smp_processor_id()].va, CXD90014_TIMERREAD));
}

static void hwtimer_on(UDIF_VA va, unsigned long expire)
{
	do {
		unsigned long counter;

		writel_relaxed(expire, HWTIMER_REG(va, CXD90014_TIMERCMP));
		writel_relaxed(HWTIMER_TMCTL_INT_ON, HWTIMER_REG(va, CXD90014_TIMERCTL));

		counter = readl_relaxed(HWTIMER_REG(va, CXD90014_TIMERREAD));
		if (time_after(expire, counter)) {
			break;
		}

		DPRINTK("%s: counter(%ld) has caught up expire(%ld)!\n", __func__, counter, expire);
		counter = readl_relaxed(HWTIMER_REG(va, CXD90014_TIMERREAD));
		expire = counter + HWTIMER_OFFSET;
	} while (1);
}

static void hwtimer_clear(UDIF_VA va)
{
	writel_relaxed(HWTIMER_TMCR_RESET_INT, HWTIMER_REG(va, CXD90014_TIMERCLR));
	writel_relaxed(HWTIMER_TMCTL_INT_OFF, HWTIMER_REG(va, CXD90014_TIMERCTL));
	wmb();
}

static UDIF_ERR hwtimer_intr(UDIF_INTR *hndl)
{
	struct hwtimer_info *timer_info;
	struct list_head *timer_list;
	unsigned long flag;

	timer_info = udif_intr_data(hndl);
	hwtimer_clear(timer_info->va);
	timer_list = &(timer_info->list);
	raw_spin_lock_irqsave(&(timer_info->lock), flag);
	while (!list_empty(timer_list)) {
		struct hwtimer *t = list_entry(timer_list->next, struct hwtimer, entry);

		if (time_after_eq(hwtimer_read_clock_ch(timer_info->va), t->expires)) {
			list_del_init(&t->entry);

			raw_spin_unlock_irqrestore(&(timer_info->lock), flag);
			t->function(t->data);
			raw_spin_lock_irqsave(&(timer_info->lock), flag);
		}
		else {
			unsigned long expire = (t->expires + HWTIMER_OFFSET);
#ifdef HWTIMER_DEBUG
			t->start_time = hwtimer_read_clock_ch(timer_info->va);
			t->starter = 2;
#endif
			hwtimer_on(timer_info->va, expire);
			break;
		}
	}
	raw_spin_unlock_irqrestore(&(timer_info->lock), flag);
	return UDIF_ERR_OK;
}

static void hwtimer_dev_reset(UDIF_VA va)
{
	writel_relaxed(HWTIMER_TMCTL_TIMER_STOP, HWTIMER_REG(va ,CXD90014_TIMERCTL));
	writel_relaxed(HWTIMER_TMCR_RESET, HWTIMER_REG(va ,CXD90014_TIMERCLR));
}

static inline void hwtimer_dev_start(UDIF_VA va)
{
	writel_relaxed(HWTIMER_TMCTL_INT_OFF, HWTIMER_REG(va ,CXD90014_TIMERCTL));
}

static unsigned long hwtimer_sync_timers(void)
{
	int i;
	UDIF_VA va;
	unsigned long jitter;

	for (i=0; i<UDIF_NR_HWTIMER; i++) {
		va = hwtimer_info[i].va;
		hwtimer_dev_reset(va);
	}

	local_irq_disable();
	for (i=0; i<UDIF_NR_HWTIMER; i++) {
		va = hwtimer_info[i].va;
		hwtimer_dev_start(va);
	}
	wmb();
	jitter = readl_relaxed(HWTIMER_REG(hwtimer_info[0].va, CXD90014_TIMERREAD));
	local_irq_enable();

	return jitter;
}

static void hwtimer_dev_exit(UDIF_VA va)
{
	writel_relaxed(HWTIMER_TMCTL_OFF, HWTIMER_REG(va, CXD90014_TIMERCTL));
	wmb();
}

static int hwtimer_interrupt_register(const UDIF_DEVICE *dev, int i)
{
	if (udif_request_irq(dev, i, hwtimer_intr, &hwtimer_info[i])) {
		return -EBUSY;
	}

	return 0;
}

static void hwtimer_interrupt_unregister(const UDIF_DEVICE *dev, int i)
{
	udif_free_irq(dev, i);
}

static UDIF_ERR hwtimer_suspend(const UDIF_DEVICE *dev, UDIF_CH i, UDIF_VP unused)
{
	unsigned long flag;
	struct list_head *timer_list;
	struct hwtimer *t;

	udif_disable_irq(udif_devint_irq(dev, i));
	hwtimer_dev_exit(hwtimer_info[i].va);

	timer_list = &(hwtimer_info[i].list);

	raw_spin_lock_irqsave(&(hwtimer_info[i].lock), flag);
	list_for_each_entry(t, timer_list, entry) {
		printk("hwtimer(cpu:%d) warning:left entry in hwtimer_list - pid:%d expire:%lu\n",
		       i, t->pid, t->expires);
	}
	INIT_LIST_HEAD(&hwtimer_info[i].list);
	raw_spin_unlock_irqrestore(&(hwtimer_info[i].lock), flag);

	return UDIF_ERR_OK;
}

static UDIF_ERR hwtimer_resume(const UDIF_DEVICE *dev, UDIF_CH i, UDIF_VP unused)
{
	if (i == UDIF_NR_HWTIMER - 1)
		hwtimer_sync_timers();

	udif_enable_irq(udif_devint_irq(dev, i));

	return UDIF_ERR_OK;
}

static int hwtimer_debug_create_proc(UDIF_CH i);
static void hwtimer_debug_remove_proc(UDIF_CH i);

static UDIF_ERR hwtimer_probe(const UDIF_DEVICE *dev, UDIF_CH i, UDIF_VP unused)
{
	UDIF_ERR err=UDIF_ERR_OK;
	unsigned long jitter;

	hwtimer_debug_create_proc(i);

	hwtimer_info[i].va = udif_devio_virt(dev, i);

#ifdef HWTIMER_DEBUG
	hwtimer_info[i].limit_over = 0;
#endif
	raw_spin_lock_init(&hwtimer_info[i].lock);
	INIT_LIST_HEAD(&hwtimer_info[i].list);

	printk("hwtimer cpu%d ", i);

	err = hwtimer_interrupt_register(dev, i);
	if (err != UDIF_ERR_OK) {
		printk("error register interrupt cpu:%d (err:%d)\n", i, err);
		hwtimer_debug_remove_proc(i);
		goto out;
	}

	irq_set_affinity(udif_devint_irq(dev, i), cpumask_of(i));

	printk("OK\n");

	if (i == UDIF_NR_HWTIMER - 1) {
		jitter = hwtimer_sync_timers();
		printk("hwtimer jitter %lu\n", jitter);
	}

out:
	if (err != UDIF_ERR_OK) {
		hwtimer_debug_remove_proc(i);
	}

	return err;
}

static UDIF_ERR hwtimer_remove(const UDIF_DEVICE *dev, UDIF_CH i, UDIF_VP unused)
{
	printk("hwtimer exit\n");

	hwtimer_interrupt_unregister(dev, i);
	hwtimer_dev_exit(hwtimer_info[i].va);
	hwtimer_debug_remove_proc(i);

	return UDIF_ERR_OK;
}

void hwtimer_init_timer(struct hwtimer *timer, void (*func)(unsigned long), unsigned long data, int cpu)
{
	cpu = hwtimer_select_cpu(cpu);
	BUG_ON((cpu < 0) || (UDIF_NR_HWTIMER <= cpu));
	timer->pid = task_pid_nr(current);
	timer->cpuid = cpu;
	timer->function = func;
	timer->data = data;

#ifdef HWTIMER_DEBUG
	timer->start_time = 0;
	timer->starter = 0;
#endif
	INIT_LIST_HEAD(&timer->entry);
}

void hwtimer_mod_timer(struct hwtimer *timer, unsigned long expire)
{
	struct list_head *timer_list;
	struct hwtimer *t;
	unsigned long flag;
	UDIF_VA va;

	if (!timer->function)
		printk("function pt error\n");
	BUG_ON(!timer->function);
	va = hwtimer_info[timer->cpuid].va;
	timer_list = &(hwtimer_info[timer->cpuid].list);
	raw_spin_lock_irqsave(&(hwtimer_info[timer->cpuid].lock), flag);
#ifdef HWTIMER_DEBUG
	timer->spinlock_get = hwtimer_read_clock_ch(va);
#endif
	if (time_after_eq((hwtimer_read_clock_ch(va) + HWTIMER_OFFSET), expire)) {
#ifdef HWTIMER_DEBUG
		hwtimer_info[timer->cpuid].limit_over++;
#endif
		expire = (hwtimer_read_clock_ch(va) + HWTIMER_OFFSET);
	}

	timer->expires = expire;
	if (!list_empty(&timer->entry)) {
		list_del_init(&timer->entry);
	}

	list_for_each_entry(t, timer_list, entry) {
		if (time_after(t->expires, timer->expires))
			break;
	}
	list_add_tail(&timer->entry, &t->entry);


	t = list_entry(timer_list->next, struct hwtimer, entry);

	if (t->expires == expire) {
		hwtimer_on(va, expire);
#ifdef HWTIMER_DEBUG
		t->start_time = hwtimer_read_clock_ch(va);
		t->starter = 1;
#endif
	}

	raw_spin_unlock_irqrestore(&(hwtimer_info[timer->cpuid].lock), flag);
}

void hwtimer_del_timer(struct hwtimer *timer)
{
	unsigned long flag;

	raw_spin_lock_irqsave(&(hwtimer_info[timer->cpuid].lock), flag);
	if (!list_empty(&timer->entry)) {
		list_del_init(&timer->entry);
	}
	raw_spin_unlock_irqrestore(&(hwtimer_info[timer->cpuid].lock), flag);
}

static void hwtimer_process_timeout(unsigned long __data)
{
	struct task_struct *task = (struct task_struct *)__data;
	wake_up_process(task);
}

#define MAX_TIMEOUT 0x7fffffff
signed long hwtimer_schedule_timeout(unsigned long timeout)
{
	struct hwtimer timer;
	unsigned long expire;

	if (timeout > MAX_TIMEOUT)
		return timeout;

	expire = timeout + hwtimer_read_clock();
	hwtimer_init_timer(&timer, hwtimer_process_timeout, (unsigned long)current, -1);

#ifdef HWTIMER_DEBUG
	timer.set_time = expire - timeout;
#endif
	hwtimer_mod_timer(&timer, expire);
	schedule();
	timeout = expire - hwtimer_read_clock();
	hwtimer_del_timer(&timer);

	return (signed long)timeout < HWTIMER_OFFSET ? 0 : timeout;
}

signed long hwtimer_usleep(unsigned long timeout)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	return hwtimer_schedule_timeout(timeout);
}


/* proc for debug */

#define HWTIMER_PROCDIR_NAME HWTIMER_NAME
#define HWTIMER_PROC_NAME "cpu"

static int hwtimer_debug_proc_read(char *buffer, char **start, off_t offset,
		int count, int *peof, void *dat)
{
	struct list_head *timer_list;
	struct hwtimer *t;
	int len = 0;
	int i = 0;
	int cpuid = (int)dat;
	unsigned long check_time;
	unsigned long flag;

	timer_list = &(hwtimer_info[cpuid].list);

#ifdef HWTIMER_DEBUG
	len += snprintf((buffer + len), (count - len), "hwtimer debug mode\n");
#endif
	len += snprintf((buffer + len), (count - len),
			"cpu %d (callback cpu:%d)\n", cpuid, cpuid);
#ifdef HWTIMER_DEBUG
	len += snprintf((buffer + len), (count - len),
			"\nlimit over %u\n", hwtimer_info[cpuid].limit_over);
#else
	len += snprintf((buffer + len), (count - len), "\n");
#endif
	i=0;

	check_time = hwtimer_read_clock_ch(hwtimer_info[cpuid].va);
	len += snprintf((buffer + len), (count - len),
			"timer count %ld (%ld[s])\n", check_time, ((check_time / TIMER_1MSEC) / 1000));

	raw_spin_lock_irqsave(&(hwtimer_info[cpuid].lock), flag);
	list_for_each_entry(t, timer_list, entry) {
		struct hwtimer timer;

		timer = *t;
		if (len > (count - 256))
			break;
		raw_spin_unlock_irqrestore(&(hwtimer_info[cpuid].lock), flag);

		i++;

		check_time = hwtimer_read_clock_ch(hwtimer_info[cpuid].va);
#ifdef HWTIMER_DEBUG
		len += snprintf((buffer + len), (count - len),
				"%2d(%08x):cpuid:%d expire:%lu tid:%d"
				" set:%lu lock_get:%lu start:%lu(%lu)",
				i, (unsigned int)t,
				timer.cpuid, timer.expires, timer.pid,
				timer.set_time, timer.spinlock_get, timer.start_time, timer.starter
				);
#else
		len += snprintf((buffer + len), (count - len),
				"%2d(%08x):cpuid:%d expire:%lu tid:%d",
				i, (unsigned int)t,
				timer.cpuid, timer.expires, timer.pid
				);
#endif
		if (time_after_eq(timer.expires, check_time))
			len += snprintf((buffer + len), (count - len),
					" (check time %lu:remain %lu) OK\n",
					check_time, (timer.expires - check_time));
		else
			len += snprintf((buffer + len), (count - len),
					" (check time %lu:remain %lu) NG\n",
					check_time, (timer.expires - check_time));
		raw_spin_lock_irqsave(&(hwtimer_info[cpuid].lock), flag);
	}
	raw_spin_unlock_irqrestore(&(hwtimer_info[cpuid].lock), flag);

	return len;
}

static struct proc_dir_entry *hwtimer_procdir, *dirp[UDIF_NR_HWTIMER];

static int hwtimer_debug_create_proc(UDIF_CH i)
{
	char name[16];

	if (i == 0)
		hwtimer_procdir = proc_mkdir(HWTIMER_PROCDIR_NAME, NULL);

	snprintf(name, (sizeof(name) -1), HWTIMER_PROC_NAME "%01d", i);
	name[sizeof(name) - 1] = 0;

	dirp[i] = create_proc_entry(name, 0444, hwtimer_procdir);
	if (!dirp[i])
		return(-EINVAL);
	dirp[i]->read_proc = hwtimer_debug_proc_read;
	dirp[i]->data = (void *)i;

	return 0;
}

static void hwtimer_debug_remove_proc(UDIF_CH i)
{
	char name[16];

	snprintf(name, (sizeof(name) -1), HWTIMER_PROC_NAME "%01d", i);
	name[sizeof(name) - 1] = 0;

	remove_proc_entry(name, hwtimer_procdir);

	if (i == 0)
		remove_proc_entry(HWTIMER_PROCDIR_NAME, NULL);
}

EXPORT_SYMBOL(hwtimer_read_clock_cpu);
EXPORT_SYMBOL(hwtimer_read_clock);
EXPORT_SYMBOL(hwtimer_init_timer);
EXPORT_SYMBOL(hwtimer_mod_timer);
EXPORT_SYMBOL(hwtimer_del_timer);
EXPORT_SYMBOL(hwtimer_schedule_timeout);
EXPORT_SYMBOL(hwtimer_usleep);

static UDIF_DRIVER_OPS hwtimer_ops = {
	.probe		= hwtimer_probe,
	.remove		= hwtimer_remove,
	.suspend	= hwtimer_suspend,
	.resume		= hwtimer_resume,
};

UDIF_IDS(hwtimer_devs) = {
	UDIF_ID(UDIF_ID_HWTIMER, UDIF_CH_MASK_DEFAULT),
};
UDIF_DEPS(hwtimer_deps) = {};

UDIF_MODULE(hwtimer, "hwtimer", "1.1", hwtimer_ops, hwtimer_devs, hwtimer_deps, NULL);
