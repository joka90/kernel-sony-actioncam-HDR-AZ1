/*
 * arch/arm/mach-cxd90014/cpuid.c
 *
 * CPUID depend routines
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/regs-misc.h>
#include <mach/cxd90014_cpuid.h>

/*
 * PCIe
 */
int cxd90014_pcie_exists(void)
{
	u32 id;
	int ret = 0;

	if (IS_CXD90014_CPUID_UNDEF()) {
		printk(KERN_ERR "ERROR: cxd90014_pcie_exists: CPUID is undef.\n");
		BUG();
	}
	id = cxd90014_cpuid & TYPEID_IDMASK;

	switch (id) {
	case TYPEID_CXD90014:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

/*-------------------------------------------------------------------*/
void __init cxd90014_update_cpuid(void)
{
	/* nothing to do */
}

void __init cxd90014_read_cpuid(void)
{
	if (!IS_CXD90014_CPUID_UNDEF()) {
		return;
	}
#if defined(CONFIG_QEMU)
	cxd90014_cpuid = TYPEID_CXD90014;
#else
	cxd90014_cpuid = readl(VA_MISC + MISC_TYPEID);
#endif /* CONFIG_QEMU */
	printk(KERN_INFO "CPUID: 0x%08lx\n", cxd90014_cpuid);
	cxd90014_update_cpuid();
}
