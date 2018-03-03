/*
 * include/asm-arm/arch-cxd4115/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright 2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-integrator/debug-macro.S.
 */
/* linux/include/asm-arm/arch-integrator/debug-macro.S
 *
 * Debugging macro include header
 *
 *  Copyright (C) 1994-1999 Russell King
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/
#include <linux/amba/serial.h>

#define PORT_REG(x) (0x1000 * x)


		.macro	addruart, rp, rv
		mov	\rp, #0x00038000	@ offset of UART0
		orr	\rp, #PORT_REG(CONFIG_CXD4132_DEBUG_UART_NO)

		orr	\rp, #0xf2000000	@ PA
		mov	\rv, \rp		@ VA = PA
		.endm

		.macro	senduart,rd,rx
		str	\rd, [\rx, #UART01x_DR]
		.endm

		.macro	waituart,rd,rx
1001:		ldr	\rd, [\rx, #UART01x_FR]
		tst	\rd, #UART01x_FR_TXFF	@ UARTFLGUTXFF - 1 when full
		bne	1001b
		.endm

		.macro	busyuart,rd,rx
1001:		ldr	\rd, [\rx, #UART01x_FR]
		tst	\rd, #UART01x_FR_BUSY	@ UARTFLGUBUSY - 1 when busy
		bne	1001b
		.endm
