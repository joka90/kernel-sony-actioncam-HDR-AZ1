/*
 * arch/arm/mach-cxd90014/pm_macro.S
 *
 * sleep macro
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
#include <mach/hardware.h>
#include <mach/bootram.h>
#include <mach/regs-ddmc.h>
#include <mach/regs-octrl.h>

	.macro	pm_workarea_va,rx
	ldr	\rx, =VA_BOOTRAM+CXD90014_SUSPEND_WORKAREA
	.endm

	.macro	pm_workarea_phys,rx
	ldr	\rx, =CXD90014_ESRAM_BASE+CXD90014_SUSPEND_WORKAREA
	.endm

	.macro	pm_stack_phys,rx
	ldr	\rx, =CXD90014_ESRAM_BASE+CXD90014_SUSPEND_WORKAREA
	.endm

	.macro	pm_outer_cache
#if defined(CONFIG_CACHE_PL310)
	bl	pl310_shutdown		@ r0-r2 corrupted
#endif

	.endm
	.macro	pm_memory
	ldr	r0, =CXD90014_DDMC_BASE

	@ RANK0 PASR
	ldrb	r1, cxd90014_ddr_pasr0_local	@ PC relative
	cmp	r1, #0
	beq	2f

	str	r1, [r0, #DDMC_CTL_49]

	ldr	r1, =DDMC_PASR_STST_CLR
	str	r1, [r0, #DDMC_CTL_75]

	ldr	r1, =DDMC_PASR_MODE1_R0
	str	r1, [r0, #DDMC_CTL_43]

	ldr	r1, =DDMC_PASR_MODE2_R0
	str	r1, [r0, #DDMC_CTL_43]

	ldr	r1, =DDMC_PASR_STST_MASK
1:
	ldr	r2, [r0, #DDMC_CTL_74]
	ands	r2, r2, r1
	beq	1b
2:
	@ RANK1 PASR
	ldrb	r1, cxd90014_ddr_pasr1_local	@ PC relative
	cmp	r1, #0
	beq	2f

	str	r1, [r0, #DDMC_CTL_54]

	ldr	r1, =DDMC_PASR_STST_CLR
	str	r1, [r0, #DDMC_CTL_75]

	ldr	r1, =DDMC_PASR_MODE1_R1
	str	r1, [r0, #DDMC_CTL_43]

	ldr	r1, =DDMC_PASR_MODE2_R1
	str	r1, [r0, #DDMC_CTL_43]

	ldr	r1, =DDMC_PASR_STST_MASK
1:
	ldr	r2, [r0, #DDMC_CTL_74]
	ands	r2, r2, r1
	beq	1b
2:
	@ Self Refresh
	ldr	r1, [r0, #DDMC_CTL_30]
	and	r1, r1, #DDMC_LP_CMD_MASK
	orr	r1, r1, #DDMC_LP_CMD_VAL
	str	r1, [r0, #DDMC_CTL_30]
	ldr	r2, =DDMC_LP_STAT_VAL
1:
	ldr	r1, [r0, #DDMC_CTL_31]
	and	r1, r1, #DDMC_LP_STAT_MASK
	cmp	r1, r2
	bne	1b
	.endm
