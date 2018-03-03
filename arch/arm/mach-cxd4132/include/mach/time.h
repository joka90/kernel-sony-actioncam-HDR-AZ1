/*
 * include/asm-arm/arch-cxd4132/time.h
 *
 * OS timer definitions
 *
 * Copyright 2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/time.h.
 */
/*
 *  linux/include/asm-arm/arch-integrator/time.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ASM_ARCH_TIME_H__
#define __ASM_ARCH_TIME_H__

#include <asm/system.h>
#include <mach/timex.h>

#include <mach/regs-timer.h>

#define TIMER_FOR_TICK  (0)
#define TIMER_FOR_SCHED (1)


/*
 * How long is the timer interval?
 */

#define TIMER_RELOAD	(CLOCK_TICK_RATE/HZ)
#if CLOCK_TICK_RATE <= MHZ
# define TICKS2USECS(x) ((x) * (MHZ / CLOCK_TICK_RATE))
#elif CLOCK_TICK_RATE == 2000000
# define TICKS2USECS(x) (((x) + 1) >> 1)
#else
# error "Unexpected CLOCK_TICK_RATE"
#endif

#define FREERUN_TICK_RATE SCHED_TICK_RATE

#ifdef __ASSEMBLY__
	.macro	mach_freerun,rx
	ldr	\rx, =CXD4115_TIMER_BASE(TIMER_FOR_SCHED)
	.endm
	.macro	mach_read_cycles,rd,rx
	ldr	\rd, [\rx, #CXD4105_TIMERREAD]
	.endm
# if SCHED_TICK_RATE == 2000000
	.macro	mach_cycles_to_uses,rx
	lsr	\rx, \rx, #1
	.endm
	.macro	mach_usecs_to_cycles,rx
	lsl	\rx, \rx, #1
	.endm
# elif SCHED_TICK_RATE == 1000000
	.macro	mach_cycles_to_uses,rx
	.endm
	.macro	mach_usecs_to_cycles,rx
	.endm
# else
#  error "Unexpected SCHED_TICK_RATE"
# endif
#else /* __ASSEMBLY__ */
extern unsigned long long read_sched_clock(void);

static inline unsigned long read_freerun(void)
{
	return readl(VA_TIMER(TIMER_FOR_SCHED)+CXD4105_TIMERREAD);
}

extern unsigned long (*gettimeoffset)(void);
extern void cxd4115_timer_early_init(void);
#endif /* !__ASSEMBLY__ */
#endif /* __ASM_ARCH_TIME_H__ */
