/*
 * include/asm-arm/arch-cxd4115/system.h
 *
 * arch functions
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/system.h.
 */
/*
 *  linux/include/asm-arm/arch-integrator/system.h
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/proc-fns.h>
#include <mach/platform.h>
#if defined(CONFIG_PM)
# include <mach/pm.h>
#endif /* CONFIG_PM */

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset_core(char mode)
{
#if defined(CONFIG_PM)
	pm_machine_reset(1);
#endif
}

static inline void arch_reset(char mode, const char *cmd)
{
	arch_reset_core(mode);
}

#endif /* __ASM_ARCH_SYSTEM_H */
