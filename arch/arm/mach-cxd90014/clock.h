/*
 * arch/arm/mach-cxd90014/clock.h
 *
 * clock manager header
 *
 * Copyright 2011 Sony Corporation
 *
 * This code is based on arch/arm/mach-integrator/clock.h.
 */
/*
 *  linux/arch/arm/mach-integrator/clock.h
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clkdev.h>

struct clk {
	const char	*name;
	unsigned long	rate;
	int		chan;
	unsigned long	(*get_rate)(struct clk *);
	int		(*set_rate)(struct clk *, unsigned long);
	int		(*enable)(struct clk *, int);
};

extern int clk_register(struct clk *clk);
extern void clk_unregister(struct clk *clk);
