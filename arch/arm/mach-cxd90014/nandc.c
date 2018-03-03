/*
 * arch/arm/mach-cxd90014/nandc.c
 *
 * Copyright 2012,2013,2014 Sony Corporation.
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
#include <asm/io.h>
#include <mach/regs-nandc.h>

static int ebi_en = 0;
static unsigned long ebi_burst_granularity;

module_param_named(ebi_en, ebi_en, bool, S_IRUSR | S_IWUSR);
module_param_named(ebi_burst_granularity, ebi_burst_granularity,
		   uint, S_IRUSR | S_IWUSR);

static unsigned long por_reset_count;

static void cxd90014_nandc_setparam(void)
{
	void *regparam_base = (ADDR_RAM_MISC_INFO + MISC_NAND_INIT_REGPARAM_OFFSET);

	if ( ebi_en ) {
		(*(unsigned long *)(regparam_base + REGPARAM_POR_RESET_COUNT_OFFSET)) = por_reset_count;
	}
	return ;
}

void cxd90014_nandc_setconfig(void)
{
	void __iomem *nandc;
	void __iomem *nandc_reg;

	if ( ebi_en ) {
		nandc = ioremap_nocache(NC_BASE_ADDR, NC_AREA_SIZE);
		if (!nandc) {
			printk("ERROR: nandc_set_config: ioremap failed\n");
			return;
		}

		por_reset_count = ((readl_relaxed(VA_NC_POR_RESET_COUNT(nandc)) & ~MASK_NC_EBI_BURST_GRANULARITY) |
				   ((ebi_burst_granularity << SHIFT_NC_EBI_BURST_GRANULARITY) & MASK_NC_EBI_BURST_GRANULARITY) |
				   NC_EBI_EN_ENABLE);
		writel_relaxed(por_reset_count, VA_NC_POR_RESET_COUNT(nandc));

		iounmap(nandc);
	}

	cxd90014_nandc_setparam();

	return ;
}
