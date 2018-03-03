/*
 * arch/arm/mach-cxd90014/dap.c
 *
 * DAP
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
#include <linux/init.h>
#include <asm/io.h>
#include <mach/regs-dap.h>
#include <mach/dap_export.h>

static void __cxd90014_cti_unlock(int id)
{
	writel_relaxed(DAP_CTI_UNLOCK, VA_DAPCTI(id)+DAP_CTILOCK);
}

void cxd90014_cti_unlock(int id)
{
	if (id < 0 || id >= N_DAP_CTI) {
		printk(KERN_ERR "%s: illegal id: %d\n", __func__, id);
		return;
	}
	__cxd90014_cti_unlock(id);
}

static void __cxd90014_cti_ack(int id)
{
	writel_relaxed(1 << DAP_CTIOUT_NCTIRQ, VA_DAPCTI(id)+DAP_CTIINTACK);
	wmb();
}

void cxd90014_cti_ack(int id)
{
	if (id < 0 || id >= N_DAP_CTI) {
		printk(KERN_ERR "%s: illegal id: %d\n", __func__, id);
		return;
	}
	__cxd90014_cti_unlock(id);
	__cxd90014_cti_ack(id);
}

void cxd90014_cti_setup(void)
{
	int i;
	u32 dat;

	/* assume: NR_CPUS <= N_DAP_CTI */
	for (i = 0; i < NR_CPUS; i++) {
		__cxd90014_cti_unlock(i);

		dat = readl_relaxed(VA_DAPCTI(i)+DAP_CTIGATE);
		dat &= ~(1 << DAP_CTICH_PMUIRQ);
		writel_relaxed(dat, VA_DAPCTI(i)+DAP_CTIGATE);

		writel_relaxed(1 << DAP_CTICH_PMUIRQ,
			       VA_DAPCTI(i)+DAP_CTIINEN(DAP_CTIIN_PMUIRQ));
		writel_relaxed(1 << DAP_CTICH_PMUIRQ,
			       VA_DAPCTI(i)+DAP_CTIOUTEN(DAP_CTIOUT_NCTIRQ));
		__cxd90014_cti_ack(i); /* clear interrupt */

		writel_relaxed(0x1, VA_DAPCTI(i)+DAP_CTICONTROL);
	}
}
