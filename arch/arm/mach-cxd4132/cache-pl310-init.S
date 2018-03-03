/*
 *  linux/arch/arm/mm/cache-pl310-init.S
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
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <mach/platform.h>
#include <mach/cache-pl310.h>

#ifdef CONFIG_CACHE_PL310_INIT
@ parameter block
PARAM_AUX_MASK  = 0
PARAM_AUX_VAL	= 4
PARAM_LAT_TAG	= 8
PARAM_LAT_DATA	= 12

ENTRY(pl310_init)
	@ input: r0=PL310 phys address
	@ input: r1=parameter block phys adddress
	@ corrupt: r2, r3

	@ disable PL310
	mov	r2, #0
	str	r2, [r0, #L2X0_CTRL]

	@ set TagRAM latency
	ldr	r2, [r1, #PARAM_LAT_TAG]
	str	r2, [r0, #PL310_TAGRAM_CTRL]
	@ set DataRAM latency
	ldr	r2, [r1, #PARAM_LAT_DATA]
	str	r2, [r0, #PL310_DATARAM_CTRL]

	@ set AUX reg
	ldr	r2, [r0, #L2X0_AUX_CTRL]
	ldr	r3, [r1, #PARAM_AUX_MASK]
	and	r2, r2, r3
	ldr	r3, [r1, #PARAM_AUX_VAL]
	orr	r2, r2, r3
	str	r2, [r0, #L2X0_AUX_CTRL]
	@ Invalidate all ways
	ands	r2, #PL310_AUX_WAY
	moveq	r2, #0x100			@ 8way
	movne	r2, #0x10000			@ 16way
	sub	r2, r2, #1
	str	r2, [r0, #L2X0_INV_WAY]
1:
	ldr	r3, [r0, #L2X0_INV_WAY]
	ands	r3, r3, r2
	bne	1b

	@ cache sync
	mov	r2, #0
	str	r2, [r0, #L2X0_CACHE_SYNC]

	@ clear intr
	mov	r2, #-1
	str	r2, [r0, #L2X0_INTR_CLEAR]

	mov	pc, lr

ENTRY(pl310_enable)
	@ input: r0=PL310 phys address
	@ corrupt: r2

	mov	r2, #L2X0_ENABLE
	str	r2, [r0, #L2X0_CTRL]
	mov	pc, lr

#endif /* CONFIG_CACHE_PL310_INIT */

#ifdef CONFIG_PM
ENTRY(pl310_shutdown)
	@ input: r0=PL310 phys address
	@ corrupt: r1, r2

	@ PL310 is enabled ?
	ldr	r2, [r0, #L2X0_CTRL]
	ands	r2, #L2X0_ENABLE
	moveq	pc, lr

	@ Clean&Invalidate all ways
	ldr	r2, [r0, #L2X0_AUX_CTRL]
	ands	r2, #PL310_AUX_WAY
	moveq	r2, #0x100			@ 8way
	movne	r2, #0x10000			@ 16way
	sub	r2, r2, #1
	str	r2, [r0, #L2X0_CLEAN_INV_WAY]
1:
	ldr	r1, [r0, #L2X0_CLEAN_INV_WAY]
	ands	r1, r1, r2
	bne	1b

	@ disable PL310
	mov	r2, #0
	str	r2, [r0, #L2X0_CACHE_SYNC]
	str	r2, [r0, #L2X0_CTRL]

	mov	pc, lr
#endif /* CONFIG_PM */

ENTRY(pl310_read_config)
	@ input: r0=PL310 phys address
	@ return value:	r0
	@   bit 0: PL310 is enabled
	@   bit 1: Exclusive mode (only if PL310 is enabled)
	@ corrupt: r0,r1

	@ PL310 is enabled ?
	ldr	r1, [r0, #L2X0_CTRL]
	ands	r1, #L2X0_ENABLE
	moveq	r0, #0
	moveq	pc, lr

	@ Exclusive mode ?
	ldr	r1, [r0, #L2X0_AUX_CTRL]
	ands	r1, #PL310_AUX_EX
	mov	r0, #1		@ Enabled
	orrne	r0, r0, #2	@ Exclusive mode

	mov	pc, lr
