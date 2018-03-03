/*
 * arch/arm/mach-cxd4132/cpuid.c
 *
 * CPUID depend routines
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
#include <linux/device.h>
#include <linux/init.h>
#include <mach/hardware.h>
#include <mach/regs-misc.h>
#include <mach/cxd4132_cpuid.h>
#include <mach/cxd4132_clock.h>
#include <asm/io.h>

/*
 * SRAM size
 */
unsigned int cxd4132_get_sramsize(void)
{
	int id;
	unsigned int size;

	if (IS_CXD41XX_CPUID_UNDEF()) {
		printk(KERN_ERR "ERROR: cxd4132_get_sramsize: CPUID is undef.\n");
		BUG();
	}
	id = cxd41xx_cpuid & TYPEID_IDMASK;

	/* determine SRAM size by CPUID */
	switch (id) {
	case TYPEID_CXD4133:
		size = SZ_1M * 3;
		break;
	default:
		size = SZ_1M * 4;
		break;
	}
	printk(KERN_INFO "SRAM: 0x%08x\n", size);
	return size;
}

/*
 * clock table
 */
#define CXD4132_DDR2_CLK_H 312000000 /* Hz */
#define CXD4132_DDR1_CLK_H 192000000 /* Hz */
#define CXD4132_CLK_L(x) ((x) / 2)
#define CXD4132_DDR2_CLK_L CXD4132_CLK_L(CXD4132_DDR2_CLK_H)
#define CXD4132_DDR1_CLK_L CXD4132_CLK_L(CXD4132_DDR1_CLK_H)
/* MPCore localtimer */
#define CXD4132_TWD_CLK(x) ((x) / 2)
#define CXD4132_DDR2_TWD_H CXD4132_TWD_CLK(CXD4132_DDR2_CLK_H)
#define CXD4132_DDR2_TWD_L CXD4132_TWD_CLK(CXD4132_DDR2_CLK_L)
#define CXD4132_DDR1_TWD_H CXD4132_TWD_CLK(CXD4132_DDR1_CLK_H)
#define CXD4132_DDR1_TWD_L CXD4132_TWD_CLK(CXD4132_DDR1_CLK_L)
/* FSP */
#define CXD4132_DDR2_FSP_H (CXD4132_DDR2_CLK_H / 2)
#define CXD4132_DDR2_FSP_L (CXD4132_DDR2_CLK_L / 2)
#define CXD4132_DDR1_FSP_H CXD4132_DDR1_CLK_H
#define CXD4132_DDR1_FSP_L CXD4132_DDR1_CLK_L

static struct cxd4132_clk_info cxd4132_clk_tables[][CXD4132_N_SPEED] = {
	{ /* DDR2 */
	/* HI */ { CXD4132_DDR2_CLK_H, CXD4132_DDR2_FSP_H,
		   CXD4132_DDR2_TWD_H/MHZ, CXD4132_DDR2_TWD_H/HZ },
	/* LO */ { CXD4132_DDR2_CLK_L, CXD4132_DDR2_FSP_L,
		   CXD4132_DDR2_TWD_L/MHZ, CXD4132_DDR2_TWD_L/HZ }
	},
	{ /* DDR1 */
	/* HI */ { CXD4132_DDR1_CLK_H, CXD4132_DDR1_FSP_H,
		   CXD4132_DDR1_TWD_H/MHZ, CXD4132_DDR1_TWD_H/HZ
	},
	/* LO */ { CXD4132_DDR1_CLK_L, CXD4132_DDR1_FSP_L,
		   CXD4132_DDR1_TWD_L/MHZ, CXD4132_DDR1_TWD_L/HZ }
	}
};
static struct cxd4132_clk_info *cxd4132_clk_table = NULL;
static unsigned int cxd4132_speed = 0;

/*
 * Call from BootCPU and interrupt disabled
 */
void cxd4132_set_speed(unsigned int speed)
{
	if (speed >= CXD4132_N_SPEED) {
		printk(KERN_ERR "ERROR: cxd4132_set_speed = %u\n",
		       cxd4132_speed);
		BUG();
	}
	cxd4132_speed = speed;
}

unsigned int cxd4132_get_speed(void)
{
	return cxd4132_speed;
}

static void cxd4132_verify_clk_table(void)
{
	if (!cxd4132_clk_table) {
		printk(KERN_ERR "ERROR: cxd4132_clk_table is not initialized yet.\n");
		BUG();
	}
	if (cxd4132_speed >= CXD4132_N_SPEED) {
		printk(KERN_ERR "ERROR: cxd4132_speed = %u\n",
		       cxd4132_speed);
		BUG();
	}
}

struct cxd4132_clk_info *cxd4132_get_clk_info(void)
{
	cxd4132_verify_clk_table();
	return &cxd4132_clk_table[cxd4132_speed];
}

unsigned long cxd4132_localtimer_rate(void)
{
	struct cxd4132_clk_info *clk;

	clk = cxd4132_get_clk_info();
	return clk->cpu >> 1;
}

void __init cxd4132_init_clk_table(void)
{
	unsigned long modepin, ddrcksel;
	struct cxd4132_clk_info *clk;

#ifdef CONFIG_QEMU
	modepin = DDRCKSEL_DDR2;
#else
	modepin = readl(VA_MISCCTRL(MISCCTRL_MODEREAD));
#endif /* CONFIG_QEMU */
	ddrcksel = modepin & DDRCKSEL_MASK;

	/* setup clk_table by DDRCKSEL */
	if (DDRCKSEL_DDR2 == ddrcksel) {
		cxd4132_clk_table = cxd4132_clk_tables[0];
	} else {
		cxd4132_clk_table = cxd4132_clk_tables[1];
	}
	clk = cxd4132_get_clk_info();
	printk(KERN_INFO "CLK: %lu, %lu\n", clk->cpu, clk->fsp);
}


/*-------------------------------------------------------------*/
void __init cxd41xx_update_cpuid(void)
{
	/* nothing to do */
}

void __init cxd4132_read_cpuid(void)
{
	if (!IS_CXD41XX_CPUID_UNDEF()) {
		return;
	}
#if defined(CONFIG_QEMU)
	cxd41xx_cpuid = TYPEID_CXD4133;
#else
	if (!(readl(VA_MISCCTRL(MISCCTRL_READDONE)) & FREAD_DONE)) {
		printk(KERN_ERR "ERROR: cxd4132_read_cpuid: not ready.\n");
		BUG();
	}
	cxd41xx_cpuid = readl(VA_MISCCTRL(MISCCTRL_TYPEID));
#endif /* CONFIG_QEMU */
	printk(KERN_INFO "CPUID: 0x%08lx\n", cxd41xx_cpuid);
	cxd41xx_update_cpuid();
}
