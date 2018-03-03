/*
 * arch/arm/mach-cxd90014/pl310.S
 *
 * PL310 helper for CXD90014
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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/pl310.h>

	.text
ENTRY(pl310_init)
	@ r0-r2 corrupted
	@ preserve r4
	ldr	r0, =CXD90014_PL310_BASE

	@ disable PL310
	mov	r1, #0
	str	r1, [r0, #L2X0_CTRL]

	@ set TagRAM latency
	ldr	r1, =TAGRAM_LATENCY
	str	r1, [r0, #L2X0_TAG_LATENCY_CTRL]
	@ set DataRAM latency
	ldr	r1, =DATARAM_LATENCY
	str	r1, [r0, #L2X0_DATA_LATENCY_CTRL]

	@ set AUX reg
	ldr	r1, [r0, #L2X0_AUX_CTRL]
	ldr	r2, =AUX_MASK
	and	r1, r1, r2
	ldr	r2, =AUX_VAL
	orr	r1, r1, r2
	str	r1, [r0, #L2X0_AUX_CTRL]

	@ Invalidate all ways
	ands	r1, #PL310_AUX_WAY
	moveq	r1, #0x100			@ 8way
	movne	r1, #0x10000			@ 16way
	sub	r1, r1, #1
	str	r1, [r0, #L2X0_INV_WAY]
1:
	ldr	r2, [r0, #L2X0_INV_WAY]
	ands	r2, r2, r1
	bne	1b

	@ cache sync
	mov	r1, #0
	str	r1, [r0, #L2X0_CACHE_SYNC]

	@ clear intr
	mov	r1, #-1
	str	r1, [r0, #L2X0_INTR_CLEAR]
	mov	pc, lr
ENDPROC(pl310_init)

ENTRY(pl310_enable)
	ldr	r0, =CXD90014_PL310_BASE
	mov	r1, #L2X0_ENABLE
	str	r1, [r0, #L2X0_CTRL]
	mov	pc, lr
ENDPROC(pl310_enable)

ENTRY(pl310_shutdown)
	@ corrupt: r1, r2
	ldr	r0, =CXD90014_PL310_BASE

	@ PL310 is enabled ?
	ldr	r1, [r0, #L2X0_CTRL]
	ands	r1, #L2X0_ENABLE
	moveq	pc, lr

	@ Clean&Invalidate all ways
	ldr	r1, [r0, #L2X0_AUX_CTRL]
	ands	r1, #PL310_AUX_WAY
	moveq	r1, #0x100			@ 8way
	movne	r1, #0x10000			@ 16way
	sub	r1, r1, #1
	str	r1, [r0, #L2X0_CLEAN_INV_WAY]
1:
	ldr	r2, [r0, #L2X0_CLEAN_INV_WAY]
	ands	r2, r2, r1
	bne	1b

	@ disable PL310
	mov	r1, #0
	str	r1, [r0, #L2X0_CACHE_SYNC]
	str	r1, [r0, #L2X0_CTRL]

	mov	pc, lr
ENDPROC(pl310_shutdown)
