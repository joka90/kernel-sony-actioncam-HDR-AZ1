/*
 * arch/arm/mach-cxd90014/subsystem.c
 *
 *
 * Copyright 2013 Sony Corporation
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <mach/subsystem.h>
#include <asm/irq.h>
#include <asm/mmu_context.h>

extern void irq_state_clr_disabled(struct irq_desc *desc);

struct mm_struct *subsys_new_mm(struct task_struct *tsk)
{
	struct mm_struct *mm, *oldmm;

	mm = mm_alloc();
	if (!mm)
		return mm;

	if (init_new_context(tsk, mm) < 0) {
		mmdrop(mm);
		return NULL;
	}

	task_lock(tsk);
	oldmm = current->active_mm;
	tsk->mm = mm;
	tsk->active_mm = mm;
	switch_mm(oldmm, mm, NULL);
	task_unlock(tsk);

	mmdrop(oldmm);

	return mm;
}
EXPORT_SYMBOL(subsys_new_mm);

#ifdef CONFIG_SMP
static void set_affinity(unsigned int irq, int cpu_id)
{
	irq_set_affinity(irq, cpumask_of(cpu_id));
}
#else
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <mach/gic.h>
#include <mach/hardware.h>

static void set_affinity(unsigned int irq, int cpu_id)
{
	const void __iomem *reg = VA_GIC_DIST + GIC_DIST_TARGET + (irq & ~GIC_TARGET_BITS_MASK);
	unsigned int shift = (irq & GIC_TARGET_BITS_MASK) << 3;
	u32 val;

	val = readl(reg) & ~(0xff << shift);
	val |= 1 << (cpu_id + shift);
	writel(val, reg);
}
#endif

int subsys_request_irq(unsigned int irq, irqreturn_t (*handler)(int, void *),
		unsigned long irq_flags, const char * devname, void *dev_id, int cpu_id)
{
	struct irq_desc *desc = irq_desc + irq;
	unsigned long flags;

	if (irq > NR_IRQS) {
		printk("%s: error %d\n", __func__, irq);
		return -1;
	}

	if (cpu_id != CPU_ID_FLEX)
		set_affinity(irq, cpu_id);

	set_irq_flags(irq, IRQF_VALID|IRQF_PROBE|IRQF_NOAUTOEN);
	irq_set_handler(irq, handle_simple_irq);

	raw_spin_lock_irqsave(&desc->lock, flags);
	irq_state_clr_disabled(desc);
	desc->depth = 0;
	raw_spin_unlock_irqrestore(&desc->lock, flags);

	return request_irq(irq, handler, irq_flags, devname, dev_id);
}
EXPORT_SYMBOL(subsys_request_irq);

void subsys_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
	set_irq_flags(irq, IRQF_VALID|IRQF_PROBE);
	irq_set_handler(irq, handle_level_irq);
}
EXPORT_SYMBOL(subsys_free_irq);

