/*
 * arch/arm/mach-cxd4115/timer.S
 *
 * TIMER initialize for CXD4115
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
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
#include <mach/regs-timer.h>

	.text
ENTRY(timer_init)
	@ TIMER CLK and PCLK are enabled. (default)

	ldr	r0, =CXD4115_TIMER_BASE(TIMER_FOR_SCHED)

	@ already running ?
	ldr	r1, [r0, #CXD4105_TIMERCTL]
	ldr	r2, =CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST
	cmp	r1, r2
	beq	end

	@ clear sched clock
	mov	r1, #0
	str	r1, [r0, #CXD4105_TIMERCTL]
	mov	r1, #TMCLR|TMINTCLR
	str	r1, [r0, #CXD4105_TIMERCLR]

	@ start sched clock
	ldr	r1, =CXD4115_TMCK_SCHED_DIV|TMCS_FREERUN|TMST
	str	r1, [r0, #CXD4105_TIMERCTL]

 end:
	mov	pc, lr
