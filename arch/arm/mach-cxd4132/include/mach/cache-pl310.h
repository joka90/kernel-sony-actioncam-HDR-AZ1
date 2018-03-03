/*
 * arch/arm/include/asm/hardware/cache-pl310.h
 *
 * PL310 cache controller
 *
 * Copyright 2009 Sony Corporation
 *
 * This code is based on arch/arm/include/asm/hardware/cache-l2x0.h
 */
/*
 * arch/arm/include/asm/hardware/cache-l2x0.h
 *
 * Copyright (C) 2007 ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ASM_ARM_HARDWARE_PL310_H
#define __ASM_ARM_HARDWARE_PL310_H

#include <asm/hardware/cache-l2x0.h>

#define L2_CACHE_BYTES	32

/* CACHE ID */
# define PL310_CACHE_ID_MASK		0xff0003c0
# define PL310_CACHE_ID_VAL		0x410000c0

/* CACHE TYPE */
# define PL310_CTYPE_LOCKDOWN_MASTER	(1<<26)
# define PL310_CTYPE_LOCKDOWN_LINE	(1<<25)

/* AUX_CTRL */
# define PL310_AUX_BRESP		(1<<30)
# define PL310_AUX_I_PREFETCH		(1<<29)
# define PL310_AUX_D_PREFETCH		(1<<28)
# define PL310_AUX_NS_INTR		(1<<27)
# define PL310_AUX_NS_LOCK		(1<<26)
# define PL310_AUX_FC_WA_SHIFT		23
# define PL310_AUX_FC_WA_MASK		0x3
# define PL310_AUX_IGN_SH		(1<<22)
# define PL310_AUX_PARITY		(1<<21)
# define PL310_AUX_EVBUS		(1<<20)
# define PL310_AUX_WAYSIZE_SHIFT	17
# define PL310_AUX_WAYSIZE_MASK		0x7
# define PL310_AUX_WAY			(1<<16)
# define PL310_AUX_SHINV		(1<<13)
# define PL310_AUX_EX			(1<<12)
# define PL310_AUX_SOHI			(1<<10)
# define PL310_AUX_FULLZERO		(1<<0)

#define PL310_TAGRAM_CTRL		0x108
#define PL310_DATARAM_CTRL		0x10c

#define PL310_LOCKDOWN_WAY_D(x)		(0x900 + ((x)<<3))
#define PL310_LOCKDOWN_WAY_I(x)		(0x904 + ((x)<<3))
#define PL310_LOCKDOWN_LINE		0x950
#define PL310_UNLOCK_WAY		0x954
#define PL310_FILTER_START		0xc00
# define PL310_FILTER_ENABLE		0x1
# define PL310_FILTER_MASK		0xfff00000
#define PL310_FILTER_END		0xc04
#define PL310_PREFETCH_OFFSET		0xF60

/* DEBUG_CTRL */
# define PL310_DEBUG_DWB		(1<<1)
# define PL310_DEBUG_DCL		(1<<0)

#define PL310_REG(x)			(VA_PL310 + (x))

#ifndef __ASSEMBLY__
extern void pl310_setup(void);
extern void pl310_info(void);
extern void pl310_lockdown(uint *mask, int n);
#endif
#endif
