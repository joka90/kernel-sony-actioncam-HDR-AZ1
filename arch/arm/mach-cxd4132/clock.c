/*
 * arch/arm/mach-cxd4132/clock.c
 *
 * clock manager
 *
 * Copyright 2009,2010 Sony Corporation
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
#include <mach/regs-permisc.h>

#include "clock.h"

#undef CXD4132_UART_SRST
#undef CXD4132_SRST_MEASURE

static int cxd4132_enable_uart(struct clk *clk, int enable)
{
#ifndef CXD4132_UART_SRST
	/*
	 *  Enable/Disable UART BAUD clock.
	 *    (note) UART PCLK should be always enabled.
	 */
	writel(enable, VA_PER_CLKRST_UART_CLK(clk->chan));
#else /* !CXD4132_UART_SRST */
	/*
	 *  Enable/Disable UART CLK(PCLK and BAUD clock)
	 *    by SoftReset. (auto power save feature)
	 */
	writel(enable, VA_PER_CLKRST_SRST_UART(clk->chan));
	if (enable) {
		int n;
# ifdef CXD4132_SRST_MEASURE
		static int n_min = 1000;
# endif
		/* Wait for negation of SoftReset */
		n = CXD4132_SRST_WAIT;
		while (!(readl(VA_PER_CLKRST_SRST_UART_STAT(clk->chan)) & 1)) {
			if (--n == 0)
				break;
		}
		if (n <= 0) {
			printk(KERN_ERR "ERROR:cxd4132_enable_uart:%d: SoftReset timeout\n", clk->chan);
		}
# ifdef CXD4132_SRST_MEASURE
		else {
			if (n < n_min) {
				n_min = n;
				printk(KERN_INFO "cxd4132_enable_uart: n=%d\n", n);
			}
		}
# endif
	}
#endif /* !CXD4132_UART_SRST */
	return 0;
}

static unsigned long cxd4132_getrate_uart(struct clk *clk)
{
	return (readl(VA_CLKRST_DATA(CLKRST_SEL_CKEN)) & SEL_CKEN_UART(clk->chan)) ? CXD4132_UART_CLK_LOW : CXD4132_UART_CLK_HIGH;
}

#if 0
#define CXD4132_CLKSTOP_WAIT 10
static int cxd4132_setrate_uart(struct clk *clk, unsigned long rate)
{
	int n;

	/* 1. Stop UART CLK */
	writel(0, VA_PER_CLKRST_UART_CLK(clk->chan));

	/* 2. Wait for clock stop */
	n = CXD4132_CLKSTOP_WAIT;
	while (readl(VA_PER_CLKRST_UART_CLK(clk->chan)+PER_CLKRST_STAT) & 1) {
		if (--n == 0)
			break;
	}

	/* 3. Select UARTCLK */
	if (rate == CXD4132_UART_CLK_HIGH) {
		writel(SEL_CKEN_UART(clk->chan), VA_CLKRST_CLR(CLKRST_SEL_CKEN));
	} else {
		writel(SEL_CKEN_UART(clk->chan), VA_CLKRST_SET(CLKRST_SEL_CKEN));
	}
	return 0;
}
#endif

static struct clk uart_clk[] = {
	/*-------------------- UART --------------------*/
	{
		.name		= "UARTCLK0",
		.chan		= 0,
		.get_rate       = cxd4132_getrate_uart,
		.enable		= cxd4132_enable_uart,
	},
	{
		.name		= "UARTCLK1",
		.chan		= 1,
		.get_rate       = cxd4132_getrate_uart,
		.enable		= cxd4132_enable_uart,
	},
	{
		.name		= "UARTCLK2",
		.chan		= 2,
		.get_rate       = cxd4132_getrate_uart,
		.enable		= cxd4132_enable_uart,
	},
	/*----------------------------------------------*/
};

#ifdef CONFIG_QEMU
static int qemu_clk_enable(struct clk *clk, int enable)
{
	return 0;
}

static unsigned long qemu_clk_getrate(struct clk *clk)
{
	return 1;
}

static struct clk qemu_clk = {
	.name		= "QEMU",
	.chan		= 2,
	.get_rate       = qemu_clk_getrate,
	.enable		= qemu_clk_enable,
};
#endif

static struct clk_lookup lookups[] = {
	{	/* UART0 */
		.dev_id		= "apb:050",
		.clk		= &uart_clk[0],
	},
	{	/* UART1 */
		.dev_id		= "apb:051",
		.clk		= &uart_clk[1],
	},
	{	/* UART2 */
		.dev_id		= "apb:052",
		.clk		= &uart_clk[2],
	},
#ifdef CONFIG_QEMU
	{	/* clcd */
		.dev_id		= "apb:clcd",
		.clk		= &qemu_clk,
	},
#endif
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
