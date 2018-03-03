/* 2010-11-30: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/system.h
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/system.h
 *
 * Copyright (C) 2003 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/hardware.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <mach/ne1_sysctrl.h>


static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	void __iomem *rststs = __io_address(NE1_BASE_SYSCTRL + SYSCTRL_RSTSTS);
	unsigned int val;

	/*
	 * To reset the NaviEngine1, write 1 to SFTRSTP bit of
	 * the Reset Status register in the System Control unit.
	 */
	val = __raw_readl(rststs);
	val = ((val & 0x003f0000) | 0x1);
	__raw_writel(val, rststs);

	do {} while (1);
}

static inline void arch_power_off(void)
{
#if defined(CONFIG_MACH_NE1T)
	void __iomem *spdwn = __io_address(NE1TB_BASE_FPGA + NE1TB_FPGA_SPDWN);
	unsigned short val;

	/*
	 * To power off the NE1-TB, write 1 to the System
	 * Power-Down register in FPGA.
	 */
	val = 0x1;
	__raw_writes(val, spdwn);

	do {} while (1);

#endif
}


#endif /* __ASM_ARCH_SYSTEM_H */
