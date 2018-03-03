/* 2012-01-30: File added and changed by Sony Corporation */
/*
 *  Copyright 2011 Sony Corporation
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
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/sched.h>
#include <linux/cnt32_to_63.h>
#include <linux/clocksource.h>

#include <asm/mach/time.h>
#include <mach/timer.h>

#include "timer.h"

/*
 * This is the sched_clock implementation for EM/EV platform.
 *
 * EM/EV platform uses the onboard TIMER_TG0 which provides an 32 bit value.
 * The counter increments at 5.7344MHZ frequency so its
 * resolution of 174.3ns, and the 32-bit counter will overflow in 748.61 Sec.
 */
#define CYC2NS_SCALE_FACTOR 		10

/* make sure COUNTER_CLOCK_TICK_RATE % TICK_RATE_SCALE_FACTOR == 0 */
#define TICK_RATE_SCALE_FACTOR 		1000

#define COUNTER_CLOCK_TICK_RATE		(TIMER_CLOCK_TICK_RATE_PLL3)

/* cycles to nsec conversions taken from arch/arm/mach-omap1/time.c,
 * convert from cycles(64bits) => nanoseconds (64bits)
 */
#define CYC2NS_SCALE (((NSEC_PER_SEC / TICK_RATE_SCALE_FACTOR) << CYC2NS_SCALE_FACTOR) \
                      / (COUNTER_CLOCK_TICK_RATE / TICK_RATE_SCALE_FACTOR))

static inline unsigned long long notrace cycles_2_ns(unsigned long long cyc)
{
#if (CYC2NS_SCALE & 0x1)
	return (cyc * CYC2NS_SCALE << 1) >> (CYC2NS_SCALE_FACTOR + 1);
#else
	return (cyc * CYC2NS_SCALE) >> CYC2NS_SCALE_FACTOR;
#endif
}

unsigned long long notrace sched_clock(void)
{
	extern struct clocksource clocksource_sched_clk;
	struct clocksource *cs = &clocksource_sched_clk;
	cycle_t cyc;

	cyc = cnt32_to_63(cs->read(cs));
	return cycles_2_ns(cyc);
}
