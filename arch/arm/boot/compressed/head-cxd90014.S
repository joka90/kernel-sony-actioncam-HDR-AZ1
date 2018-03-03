/*
 * arch/arm/boot/compressed/head-cxd90014.S
 *
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
 */

#include <linux/linkage.h>
#include <asm/mach-types.h>
#include <mach/platform.h>

	.section        ".start", "ax"

	@ Preserve r7, r8, r9
	@ We can not preserve r7 and r9, but we do not use them later.
__cxd90014_start:
	@ turn off coherency
	mrc	p15, 0, r0, c1, c0, 1	@ aux
	bic	r0, r0, #0x40		@ clear SMP
	mcr	p15, 0, r0, c1, c0, 1	@
	@ turn MMU off
	mrc	p15, 0, r0, c1, c0, 0	@ ctrl register
	bic	r0, r0, #0x0005		@ .............c.m
	@bic	r0, r0, #0x1000		@ ...i............
	mcr	p15, 0, r0, c1, c0, 0	@ disable MMU and cache
	isb
	@ invalidate D-Cache
	dmb				@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1	@ read clidr
	ands	r3, r0, #0x7000000	@ extract loc from clidr
	mov	r3, r3, lsr #23		@ left align loc bit field
	beq	invfinished		@ if loc is 0, then no need to clean
	mov	r10, #0			@ start clean at cache level 0
invloop1:
	add	r2, r10, r10, lsr #1	@ work out 3x current cache level
	mov	r1, r0, lsr r2		@ extract cache type bits from clidr
	and	r1, r1, #7		@ mask of the bits for current cache only
	cmp	r1, #2			@ see what cache we have at this level
	blt	invskip			@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0	@ select current cache level in cssr
	isb				@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0	@ read the new csidr
	and	r2, r1, #7		@ extract the length of the cache lines
	add	r2, r2, #4		@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3	@ find maximum number on the way size
	clz	r5, r4			@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13	@ extract max number of the index size
invloop2:
	mov	r9, r4			@ create working copy of max way size
invloop3:
 ARM(	orr	r11, r10, r9, lsl r5	) @ factor way and cache number into r11
 THUMB(	lsl	r6, r9, r5		)
 THUMB(	orr	r11, r10, r6		) @ factor way and cache number into r11
 ARM(	orr	r11, r11, r7, lsl r2	) @ factor index number into r11
 THUMB(	lsl	r6, r7, r2		)
 THUMB(	orr	r11, r11, r6		) @ factor index number into r11
	mcr	p15, 0, r11, c7, c6, 2	@ invalidate by set/way
	subs	r9, r9, #1		@ decrement the way
	bge	invloop3
	subs	r7, r7, #1		@ decrement the index
	bge	invloop2
invskip:
	add	r10, r10, #2		@ increment cache number
	cmp	r3, r10
	bgt	invloop1
invfinished:
	mov	r10, #0			@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0	@ select current cache level in cssr
	dsb
	mcr	p15, 0, r10, c7, c5, 0	@ invalidate I-cache and BTC
	mcr	p15, 0, r10, c8, c7, 0	@ invalidate whole TLB
	dsb
	isb
	@
	b	__cxd90014_end
	.ltorg
__cxd90014_end:
	nop
