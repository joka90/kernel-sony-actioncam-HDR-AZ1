/*
 * arch/arm/mach-cxd90014/include/mach/entry-macro.h
 *
 * Low-level IRQ helpers
 *
 * C code version of arch/arm/mach-cxd90014/include/mach/entry-macro.S,
 * Any changes in this files must also be made in
 * arch/arm/mach-cxd90014/include/mach/entry-macro.S
 *
 * Copyright 2012,2013 Sony Corporation
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
#ifndef __ASM_ARCH_ENTRY_MACRO_H
#define __ASM_ARCH_ENTRY_MACRO_H

#include <asm/hardware/gic.h>
#include <mach/hardware.h>
#include <asm/io.h>

#define IRQ_MASK 0x3ff

#define IRQ_IS_DONE(irq)                ((irq) == 1023)
#define IRQ_IS_EXTERNAL_INTERRUPT(irq)  ((irq) > 29)
#define IRQ_IS_IPI(irq)                 ((irq) < 16)
#define IRQ_IS_LOCALTIMER(irq)          ((irq) == 29)

#define get_irq_preamble() do {} while (0)

static inline unsigned int get_irqstat(void)
{
	return readl_relaxed(VA_GIC_CPU + GIC_CPU_INTACK);
}

static inline void ack_local_timer_irq(unsigned int irqstat)
{
	writel_relaxed(irqstat, VA_GIC_CPU + GIC_CPU_EOI);
}

static inline void ack_IPI_irq(unsigned int irqstat)
{
	writel_relaxed(irqstat, VA_GIC_CPU + GIC_CPU_EOI);
}

#endif /* __ASM_ARCH_ENTRY_MACRO_H */
