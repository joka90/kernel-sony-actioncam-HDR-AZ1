/*
 * drivers/serial/amba2.h
 *
 * CXD4105 UART access macros
 *
 * Copyright 2005,2006,2007 Sony Corporation
 *
 * This code is based on drivers/serial/amba-pl011.c
 */
/*
 *  linux/drivers/char/amba.c
 *
 *  Driver for AMBA serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
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
 *
 *  $Id: amba.c,v 1.41 2002/07/28 10:03:27 rmk Exp $
 *
 * This is a generic driver for ARM AMBA-type serial ports.  They
 * have a lot of 16550-like features, but are not register compatible.
 * Note that although they do have CTS, DCD and DSR inputs, they do
 * not have an RI input, nor do they have DTR or RTS outputs.  If
 * required, these have to be supplied via some other means (eg, GPIO)
 * and hooked into this driver.
 */
#ifndef __AMBA2_UART_H__
#define __AMBA2_UART_H__

#undef DEBUG

/*
 * Access macros for the AMBA UARTs
 */
#define UART_GET_INT_STATUS(p)	readl_relaxed((p)->membase + UART011_MIS)
#define UART_PUT_ICR(p, c)	writel_relaxed((c), (p)->membase + UART011_ICR)
#define UART_GET_FR(p)		readl_relaxed((p)->membase + UART01x_FR)
/* UARTDR bit 8-11 is FE,PE,BE,OE */
#define UART_GET_DR(p)		readl_relaxed((p)->membase + UART01x_DR)
#define UART_GET_CHAR(p)	(readl_relaxed((p)->membase + UART01x_DR) & 0xff)
#define UART_PUT_CHAR(p, c)	writel_relaxed((c), (p)->membase + UART01x_DR)
#define UART_GET_RSR(p)		readl_relaxed((p)->membase + UART01x_RSR)
#define UART_PUT_RSR(p, c)	writel_relaxed((c), (p)->membase + UART01x_RSR)
#define UART_GET_CR(p)		readl_relaxed((p)->membase + UART011_CR)
#define UART_PUT_CR(p,c)	writel_relaxed((c), (p)->membase + UART011_CR)
#define UART_GET_IBRD(p)	readl_relaxed((p)->membase + UART011_IBRD)
#define UART_PUT_IBRD(p,c)	writel_relaxed((c), (p)->membase + UART011_IBRD)
#define UART_GET_FBRD(p)	readl_relaxed((p)->membase + UART011_FBRD)
#define UART_PUT_FBRD(p,c)	writel_relaxed((c), (p)->membase + UART011_FBRD)
#define UART_GET_LCRH(p)	readl_relaxed((p)->membase + UART011_LCRH)
#define UART_PUT_LCRH(p,c)	writel_relaxed((c), (p)->membase + UART011_LCRH)
#define UART_GET_IMSC(p)	readl_relaxed((p)->membase + UART011_IMSC)
#define UART_PUT_IMSC(p,c)	writel_relaxed((c), (p)->membase + UART011_IMSC)
#define UART_GET_RIS(p)		readl_relaxed((p)->membase + UART011_RIS)
#define UART_GET_IFLS(p)	readl_relaxed((p)->membase + UART011_IFLS)
#define UART_PUT_IFLS(p,c)	writel_relaxed((c), (p)->membase + UART011_IFLS)
#define UART_GET_DMACR(p)	readl_relaxed((p)->membase + UART011_DMACR)
#define UART_PUT_DMACR(p,c)	writel_relaxed((c), (p)->membase + UART011_DMACR)
#define UART_GET_RTO(p)		readl_relaxed((p)->membase + AMBAUART_RTO)
#define UART_PUT_RTO(p,c)	writel_relaxed((c), (p)->membase + AMBAUART_RTO)

#define UART_RX_DATA(s)		(((s) & UART01x_FR_RXFE) == 0)
#define UART_TX_READY(s)	(((s) & UART01x_FR_TXFF) == 0)

#define AMBAUART_ERRORS (UART011_OEIM|UART011_BEIM|UART011_PEIM|UART011_FEIM)

/*
 * We wrap our port structure around the generic uart_port.
 */
struct uart_amba_port {
	struct uart_port	port;
	struct clk		*clk;
	unsigned int		im;	/* interrupt mask */
	unsigned int		old_status;
	int			is_console;
	int			txrx_enable;
#ifdef AMBAUART_SOFTIRQ
	int			tx_intr_mode;
	int			tx_intr;
#endif /* AMBAUART_SOFTIRQ */
	int			boost;

	struct uart_platform_data *info;
};

extern void ambauart_start_tx(struct uart_amba_port *);

#endif /* __AMBA2_UART_H__ */
