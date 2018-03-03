/*
 * arch/arm/mach-cxd4132/pl310.S
 *
 * PL310 helper for CXD4132
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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/time.h>
#include <mach/hardware.h>

	.text
#ifdef CONFIG_CACHE_PL310_INIT
ENTRY(cxd4132_pl310_init)
	@ r0-r3,r5 corrupted
	@ preserve r4
	mov	r5, lr			@ save lr
	ldr	r0, =CXD4132_PL310_BASE
	adr	r1, pl310_param
	bl	pl310_init		@ r2,r3 corrupted
	bl	pl310_enable		@ r2 corrupted
	mov	pc, r5

pl310_param:
	.long	0xcfffebff		@ AUX mask
#ifdef CONFIG_CACHE_PL310_EXCLUSIVE
	.long	0x00001000		@ AUX val
					@   Exclusive cache
#else
	.long	0x00000000		@ AUX val
					@   Inclusive cache
#endif
	.long	0x000			@ TagRAM Latency(1/1/1)
	.long	0x000			@ DataRAM Latency(1/1/1)
#endif /* CONFIG_CACHE_PL310_INIT */

#ifdef CONFIG_PM
ENTRY(cxd4132_pl310_shutdown)
	@ r0-r2 corrupted
	ldr	r0, =CXD4132_PL310_BASE
	b	pl310_shutdown		@ r1,r2 corrupted
#endif


ENTRY(cxd4132_pl310_setup)
	@ r0-r3 corrupted
#ifdef CONFIG_CACHE_PL310_INIT
	b	cxd4132_pl310_init	@ r0-r3,r5 corrupted
#else /* CONFIG_CACHE_PL310_INIT */
	@ takeover PL310 setting
	mov	r3, lr			@ save lr
	ldr	r0, =CXD4132_PL310_BASE
	bl	pl310_read_config	@ r0,r1 corrupted
	ands	r0, #2			@ Exclusive mode ?
	moveq	pc, r3			@ return
	@ set exclusive_cache flag
	adr	r0, 1f
	ldmia	r0, {r1, r2}
	sub	r0, r0, r1
	add	r2, r2, r0
	mov	r0, #1
	str	r0, [r2]
	mov	pc, r3			@ return
#endif /* CONFIG_CACHE_PL310_INIT */
1:
	.long	.
	.long	exclusive_cache
	.ltorg

	.data
	.type	exclusive_cache, #object
ENTRY(exclusive_cache)
#ifdef CONFIG_CACHE_PL310_EXCLUSIVE
	.long	1
#else
	.long	0
#endif
