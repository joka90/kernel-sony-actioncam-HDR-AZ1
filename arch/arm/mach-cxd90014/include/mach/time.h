/*
 * include/asm-arm/arch-cxd90014/time.h
 *
 * OS timer definitions
 *
 * Copyright 2011 Sony Corporation
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

#ifndef CONFIG_EJ_LOCALTIMER_ONLY
#define TIMER_FOR_TICK  (0)
#define TIMER_FOR_SCHED (1)
#define TIMER_FOR_TICK1	(2)
#define TIMER_FOR_TICK2	(3)
#define TIMER_FOR_TICK3	(4)


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
#endif /* !CONFIG_EJ_LOCALTIMER_ONLY */

#define FREERUN_TICK_RATE SCHED_TICK_RATE

#ifdef __ASSEMBLY__
# ifndef CONFIG_EJ_LOCALTIMER_ONLY
	.macro	mach_freerun,rx
	ldr	\rx, =CXD90014_TIMER_BASE(TIMER_FOR_SCHED)
	.endm
	.macro	mach_read_cycles,rd,rx
	ldr	\rd, [\rx, #CXD90014_TIMERREAD]
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
# endif /* !CONFIG_EJ_LOCALTIMER_ONLY */

#else /* __ASSEMBLY__ */

static inline unsigned long read_freerun(void)
{
# ifdef CONFIG_EJ_LOCALTIMER_ONLY
	return readl_relaxed(VA_GLOBALTIMER);
# else
	return readl_relaxed(VA_TIMER(TIMER_FOR_SCHED)+CXD90014_TIMERREAD);
# endif /* CONFIG_EJ_LOCALTIMER_ONLY */
}

extern unsigned long (*gettimeoffset)(void);
extern void cxd90014_timer_early_init(void);

#endif /* !__ASSEMBLY__ */
#endif /* __ASM_ARCH_TIME_H__ */
