/*
 * arch/arm/mach-cxd90014/cfg.c
 *
 * config parameters
 *
 * Copyright 2009,2010 Sony Corporation
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
#include <mach/cxd90014_cpuid.h>
#include <mach/cxd90014_cfg.h>

/*
 * CPUID
 */
unsigned long cxd90014_cpuid = CXD90014_CPUID_UNDEF;

void __init __attribute__((weak)) cxd90014_update_cpuid(void)
{
	/* none */
}

/* Override CPUID by command line parameter */
static int __init set_cpuid(char *str)
{
	get_option(&str, (int *)&cxd90014_cpuid);
	printk(KERN_INFO "CPUID override: 0x%lx\n", cxd90014_cpuid);
	cxd90014_update_cpuid();
	return 0;
}
early_param("cpuid", set_cpuid);

/*
 * XPOWER_OFF (XPWR_OFF)
 */
unsigned long cxd90014_xpower_off = CXD90014_PORT_UNDEF;
module_param_named(poff, cxd90014_xpower_off, port, S_IRUSR);

/*
 * UART ports which is handled by tty driver.
 */
unsigned long cxd90014_uart_bitmap = 0x1UL; /* bit0=UART0 */
module_param_named(uart, cxd90014_uart_bitmap, ulong, S_IRUSR);

#if defined(CONFIG_SMC9118) || defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
/*
 * Ether IRQ
 */
unsigned long cxd90014_ether_irq = CONFIG_CXD90014_SMC911X_INT;
module_param_named(ether_irq, cxd90014_ether_irq, ulong, S_IRUSR);
#endif /* CONFIG_SMC9118 || CONFIG_SMSC911X */

/*
 * Observe tick and localtimer for debug
 */
unsigned long cxd90014_tick_timing = 0UL;
module_param_named(tick_timing, cxd90014_tick_timing, ulong, S_IRUSR|S_IWUSR);

/*
 * SMODE
 */
unsigned long cxd90014_smode_port = CXD90014_PORT_UNDEF;
module_param_named(smode, cxd90014_smode_port, port, S_IRUSR);
int cxd90014_smode = -1;
EXPORT_SYMBOL(cxd90014_smode);

/*
 * ABT_A1 IRQ
 */
int cxd90014_abt1_irq = -1;
module_param_named(abt1_irq, cxd90014_abt1_irq, int, S_IRUSR);

/*
 * ABT_A1 port
 */
unsigned long cxd90014_abt1_port = CXD90014_PORT_UNDEF;
module_param_named(abt1, cxd90014_abt1_port, port, S_IRUSR);
