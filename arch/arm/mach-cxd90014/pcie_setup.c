/*
 * arch/arm/mach-cxd90014/pcie_setup.c
 *
 * PCIE setup
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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <mach/moduleparam.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/regs-pcie.h>
#include <mach/regs-clk.h>
#include <mach/cxd90014_cpuid.h>
#include <mach/pcie_export.h>

int cxd90014_pcie_enable = 0;

static void cxd90014_pcie_powersave(int save)
{
	unsigned long flags;

	if (!cxd90014_pcie_exists())
		return;

	local_irq_save(flags);
	if (save) {
		writel_relaxed(REG_XPS_CK_AVB_PCIE,     VA_CLKRST3(IPCLKEN0)+CLKRST_CLR);
		writel_relaxed(REG_XPS_CK_AXI01_PCIE,   VA_CLKRST3(IPCLKEN1)+CLKRST_CLR);
		writel_relaxed(REG_XPS_CK_AXI01_PCIE_H, VA_CLKRST3(IPCLKEN1)+CLKRST_CLR);
	} else {
		/* PCIe TxRx Resistance regulation: ON */
		writel_relaxed(PCIE_PHY_PD_TXRX, VA_PCIE_PHY + PCIE_PHY_REGULATION + PCIE_PHY_CLR);
		wmb();
		udelay(5); /* wait 5us */
	}
	local_irq_restore(flags);
}

void cxd90014_pcie_setup(void)
{
	if (!cxd90014_pcie_exists()) {
		cxd90014_pcie_enable = 0;
		return;
	}
#ifdef CONFIG_CXD90014_PCIE2_DMX
	cxd90014_pcie_powersave(!cxd90014_pcie_enable);
#else /* CONFIG_CXD90014_PCIE2_DMX */
	cxd90014_pcie_powersave(true);
#endif /* CONFIG_CXD90014_PCIE2_DMX */
}
