/*
 * arch/arm/mach-cxd90014/clock.c
 *
 * clock manager
 *
 * Copyright 2011 Sony Corporation
 *
 * This code is based on arch/arm/mach-integrator/clock.c.
 */
/*
 *  linux/arch/arm/mach-integrator/clock.c
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clkdev.h>
#include <linux/mutex.h>

#include <asm/clkdev.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <mach/timex.h>
#include <mach/regs-clk.h>

#include "clock.h"

static int cxd90014_enable_uart(struct clk *clk, int enable)
{
#if !defined(CONFIG_CXD90014_FPGA) && !defined(CONFIG_QEMU)
	u32 bitmask;

	/*
	 *  Enable/Disable UART BAUD clock.
	 *    (note) UART PCLK should be always enabled.
	 */
	bitmask = 1 << CXD90014_UART_CLK_SHIFT(clk->chan);
	if (enable)
		writel_relaxed(bitmask, VA_CLKRST4(IPCLKEN4)+CLKRST_SET);
	else
		writel_relaxed(bitmask, VA_CLKRST4(IPCLKEN4)+CLKRST_CLR);
#endif /* !CONFIG_CXD90014_FPGA && !CONFIG_QEMU */

	return 0;
}

static unsigned long cxd90014_getrate_uart(struct clk *clk)
{
	return CXD90014_UART_CLK_HIGH;
}

static struct clk uart_clk[] = {
	/*-------------------- UART --------------------*/
	{
		.name		= "UARTCLK0",
		.chan		= 0,
		.get_rate       = cxd90014_getrate_uart,
		.enable		= cxd90014_enable_uart,
	},
	{
		.name		= "UARTCLK1",
		.chan		= 1,
		.get_rate       = cxd90014_getrate_uart,
		.enable		= cxd90014_enable_uart,
	},
	{
		.name		= "UARTCLK2",
		.chan		= 2,
		.get_rate       = cxd90014_getrate_uart,
		.enable		= cxd90014_enable_uart,
	},
	/*----------------------------------------------*/
};

static struct clk_lookup lookups[] = {
	{	/* UART0 */
		.dev_id		= "uart0",
		.clk		= &uart_clk[0],
	},
	{	/* UART1 */
		.dev_id		= "uart1",
		.clk		= &uart_clk[1],
	},
	{	/* UART2 */
		.dev_id		= "uart2",
		.clk		= &uart_clk[2],
	},
};

int clk_enable(struct clk *clk)
{
	(clk->enable)(clk, 1);
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	(clk->enable)(clk, 0);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (clk->rate != 0)
		return clk->rate;
	if (clk->get_rate != NULL)
		return (clk->get_rate)(clk);
	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	return rate;
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}
EXPORT_SYMBOL(clk_set_rate);

static int __init clk_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lookups); i++)
		clkdev_add(&lookups[i]);
	return 0;
}
arch_initcall(clk_init);
