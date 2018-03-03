/*
 * arch/arm/mach-cxd4132/speed.c
 *
 * Speed notify
 *
 * Copyright 2010 Sony Corporation
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/irqflags.h>
#include <mach/hardware.h>
#include <mach/localtimer.h>
#include <mach/pm.h>
#include <mach/cxd4132_clock.h>
#include <mach/regs-clk.h>
#include <asm/io.h>

void cxd41xx_speed_notify(void)
{
#ifdef CONFIG_EJ_USE_TICK_TIMER
	unsigned long flags;
	unsigned int stat, speed;
	struct cxd4132_clk_info *clk;

	local_irq_save(flags);

	/* new speed */
	stat = readl(VA_FCS(FCS_CK_STATUS));
	if (stat & FCS_BUSY) {
		printk(KERN_ERR "cxd41xx_speed_notify: FCS_BUSY\n");
		speed = CXD4132_SPEED_HI;
	} else {
		speed = ((stat & FCS_AVBHOST_STAT) == FCS_HALF) ? \
			CXD4132_SPEED_LO : CXD4132_SPEED_HI;
	}

	/* set new speed */
	cxd4132_set_speed(speed);

	/* Adjust localtimers */
	clk = cxd4132_get_clk_info();
	twd_calibrate_rate_all(clk->twd_MHz, clk->twd_per_tick);

	local_irq_restore(flags);
#endif /* CONFIG_EJ_USE_TICK_TIMER */
}


#include <mach/pm_speed.h>

#undef ELAPSE

/* Called from DFS interrupt handler */
void pm_speed_notify(void)
{
#ifdef ELAPSE
	unsigned long t1, t2;
	t1 = mach_read_cycles();
#endif /* ELAPSE */

	cxd41xx_speed_notify();
#ifdef ELAPSE
	t2 = mach_read_cycles();
	printk(KERN_ERR "pm_speed_notify: %lu [us]\n", mach_cycles_to_usecs(t2-t1));
#endif /* ELAPSE */
}
EXPORT_SYMBOL(pm_speed_notify);
