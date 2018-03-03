/*
 * arch/arm/mach-cxd90014/fiq.c
 *
 * FIQ driver
 *
 * Copyright 2012 Sony Corporation
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
#include <asm/system.h>
#include <asm/io.h>
#include <mach/memory.h>
#include <mach/regs-misc.h>

static unsigned int fiq_enable_flag = 0;
module_param_named(enable, fiq_enable_flag, uint, S_IRUSR|S_IWUSR);

/* Called from early_trap_init() */
void mach_trap_init(unsigned long vectors)
{
	memcpy((void *)vectors + FIQ_VEC_OFFS, fiq_vec, fiq_vec_end - fiq_vec);
	memset((void *)vectors + CXD90014_FIQ_REGSAVE, 0, CXD90014_FIQ_REG_ALL);
	memcpy((void *)vectors + CXD90014_FIQ_DEBUG, fiq_debug, fiq_debug_end - fiq_debug);
	/* cache flush will be done in early_trap_init() */
}

void cxd90014_fiq_init(void)
{
	/* setup PER_FIQEN */
	if(fiq_enable_flag){
		writel(MISC_FIQ_FIQE_ALL, VA_MISC_FIQ);
	}

	local_fiq_enable();
}
