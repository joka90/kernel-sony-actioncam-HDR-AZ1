/*
 * arch/arm/mach-cxd4132/pm.c
 *
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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <mach/moduleparam.h>
#include <linux/init.h>
#include <mach/hardware.h>
#include <mach/regs-sio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-permisc.h>
#include <mach/regs-avbckg.h>
#include <mach/regs-dmc.h>
#include <mach/bootram.h>
#include <mach/powersave.h>

#undef ERROR_INJECTION

#ifdef CONFIG_PM
#ifndef CONFIG_CXD41XX_SIMPLE_SUSPEND
/*
 * DDR Error Detection
 */
extern int pm_chksum; /* sleep.S */
module_param_named(chksum, pm_chksum, int, S_IRUSR|S_IWUSR);
extern ulong pm_csumaddr; /* sleep.S */
module_param_named(csumaddr, pm_csumaddr, ulongH, S_IRUSR|S_IWUSR);
extern ulong pm_csumsize; /* sleep.S */
module_param_named(csumsize, pm_csumsize, ulongH, S_IRUSR|S_IWUSR);

/*
 * NVRAM config
 */
extern int pm_nvram; /* sleep.S */
module_param_named(nvram, pm_nvram, int, S_IRUSR|S_IWUSR);

/*
 * SIO config
 */
static int pm_sio_ch = -1;
module_param_named(sio_ch, pm_sio_ch, int, S_IRUSR);
static int pm_sio_port = -1;
module_param_named(sio_port, pm_sio_port, int, S_IRUSR);
static int pm_sio_xcs = -1;
module_param_named(sio_xcs, pm_sio_xcs, int, S_IRUSR);
static int pm_sio_baud = -1;
module_param_named(sio_baud, pm_sio_baud, int, S_IRUSR);
#endif /* !CONFIG_CXD41XX_SIMPLE_SUSPEND */

#ifdef ERROR_INJECTION
extern int pm_sio_rx_perr;	/* sleep.S */
module_param_named(sio_rx_perr, pm_sio_rx_perr, int, S_IRUSR|S_IWUSR);
extern int pm_sio_tx_perr;	/* sleep.S */
module_param_named(sio_tx_perr, pm_sio_tx_perr, int, S_IRUSR|S_IWUSR);
extern int pm_sio_rx_timeout;	/* sleep.S */
module_param_named(sio_rx_timeout, pm_sio_rx_timeout, int, S_IRUSR|S_IWUSR);
extern int pm_sio_tx_timeout;	/* sleep.S */
module_param_named(sio_tx_timeout, pm_sio_tx_timeout, int, S_IRUSR|S_IWUSR);
extern int pm_sio_data_error;	/* sleep.S */
module_param_named(sio_data_error, pm_sio_data_error, int, S_IRUSR|S_IWUSR);
#endif /* ERROR_INJECTION */


void cxd4132_pm_setup(void)
{
#ifndef CONFIG_CXD41XX_SIMPLE_SUSPEND
	extern unsigned long pm_sio_addr;     /* sleep.S */
	extern unsigned long pm_sio_gpio;     /* sleep.S */
	extern unsigned long pm_sio_clkrst;   /* sleep.S */
	extern unsigned long pm_sio_xcs_mask; /* sleep.S */
	extern unsigned long pm_sio_baud_l;   /* sleep.S */
	extern unsigned long pm_sio_baud_h;   /* sleep.S */
	extern unsigned long pm_sio_buf;      /* sleep.S */

	if (-1 != pm_sio_ch && -1 != pm_sio_port && -1 != pm_sio_xcs
	    && -1 != pm_sio_baud) {
		pm_sio_addr = CXD4132_SIO(pm_sio_ch);
		pm_sio_gpio = PA_GPIO(pm_sio_port);
		pm_sio_clkrst = CXD4132_PER_CLKRST_SIO_CLK(pm_sio_ch);
		pm_sio_xcs_mask = 1 << pm_sio_xcs;
		pm_sio_baud_l = (pm_sio_baud & 0x00ff) << 24;
		pm_sio_baud_h = (pm_sio_baud & 0xff00) << 16;
		pm_sio_buf = CXD4132_SRAM_BASE+CXD4132_SUSPEND_WORKAREA-1024;
	}
#endif /* CONFIG_CXD41XX_SIMPLE_SUSPEND */
}
#endif /* CONFIG_PM */

void bam_powersave(int save)
{
	unsigned long flags;
	u32 dat;

	local_irq_save(flags);
	if (save) {
		/* SDC_CLK_GATE */
		dat = readl(VA_SDC+SDC_CLK_GATE);
		dat &= ~SDC_CLK_GATE_MONPS;
		writel(dat, VA_SDC+SDC_CLK_GATE);

		/* CKG_ACLK_PS */
		dat = readl(VA_AVBCKG+AVBCKG_ACLK_PS);
		dat &= ~AVBCKG_XPS_A_BAM;
		writel(dat, VA_AVBCKG+AVBCKG_ACLK_PS);

		/* CKG_PCLK_PS */
		dat = readl(VA_AVBCKG+AVBCKG_PCLK_PS);
		dat &= ~AVBCKG_XPS_P_BAM;
		writel(dat, VA_AVBCKG+AVBCKG_PCLK_PS);
	} else {
		/* CKG_PCLK_PS */
		dat = readl(VA_AVBCKG+AVBCKG_PCLK_PS);
		dat |= AVBCKG_XPS_P_BAM;
		writel(dat, VA_AVBCKG+AVBCKG_PCLK_PS);

		/* CKG_ACLK_PS */
		dat = readl(VA_AVBCKG+AVBCKG_ACLK_PS);
		dat |= AVBCKG_XPS_A_BAM;
		writel(dat, VA_AVBCKG+AVBCKG_ACLK_PS);

		/* SDC_CLK_GATE */
		dat = readl(VA_SDC+SDC_CLK_GATE);
		dat |= SDC_CLK_GATE_MONPS;
		writel(dat, VA_SDC+SDC_CLK_GATE);
	}
	local_irq_restore(flags);
}
