/*
 * drivers/tty/serial/amba2.c
 *
 * CXD90014 UART driver
 *
 * Copyright 2012 Sony Corporation
 *
 * This code is based on drivers/tty/serial/amba-pl011.c
 */
/*
 *  Driver for AMBA serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *  Copyright (C) 2010 ST-Ericsson SA
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
 * This is a generic driver for ARM AMBA-type serial ports.  They
 * have a lot of 16550-like features, but are not register compatible.
 * Note that although they do have CTS, DCD and DSR inputs, they do
 * not have an RI input, nor do they have DTR or RTS outputs.  If
 * required, these have to be supplied via some other means (eg, GPIO)
 * and hooked into this driver.
 */

#if defined(CONFIG_SERIAL_AMBA2_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/nmi.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>

#include <asm/io.h>

#include <linux/interrupt.h>
#ifdef CONFIG_KGDB
# include <linux/kgdb.h>
#endif /* CONFIG_KGDB */

#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/hardware/serial_amba2_ioctl.h>
#include <mach/uart.h>
#include "amba2.h"

#if defined(CONFIG_WBI_DEV_XMODEM)
#include <asm/mach/xmodem.h>
#endif /* CONFIG_WBI_DEV_XMODEM */

#define SERIAL_AMBA_MAJOR	204
#define SERIAL_AMBA_MINOR	16
#define SERIAL_AMBA_NR		UART_NR

#define AMBA_ISR_PASS_LIMIT	256

#define UART_DR_ERROR		(UART011_DR_OE|UART011_DR_BE|UART011_DR_PE|UART011_DR_FE)
#define UART_DUMMY_DR_RX	(1 << 16)

#define AMBAUART_CONSOLE_ENABLE_DEFAULT 0

/* module param */
static int console = AMBAUART_CONSOLE_ENABLE_DEFAULT;

/* initial setting */
static int ambauart_console_enable = AMBAUART_CONSOLE_ENABLE_DEFAULT;

#if defined(CONFIG_PREEMPT_RT_FULL) && defined(SUPPORT_SYSRQ)
static int ambauart_boost = 0;
module_param_named(boost, ambauart_boost, bool, S_IRUSR);
#endif /* CONFIG_PREEMPT_RT_FULL && SUPPORT_SYSRQ */

static void pl011_stop_tx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im &= ~UART011_TXIM;
	UART_PUT_IMSC(port, uap->im);
}

#ifdef AMBAUART_SOFTIRQ
static void pl011_softirq(struct uart_amba_port *uap)
{
	if (uap->tx_intr_mode == 0) {
		uap->tx_intr = 1;
		mb();
		AMBAUART_SOFTIRQ(uap->port.irq);
	}
}
#endif /* AMBAUART_SOFTIRQ */

static void pl011_start_tx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im |= UART011_TXIM;
	UART_PUT_IMSC(&uap->port, uap->im);
#ifdef AMBAUART_SOFTIRQ
	pl011_softirq(uap);
#endif /* AMBAUART_SOFTIRQ */
}

static void pl011_stop_rx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im &= ~(UART011_RXIM|UART011_RTIM|UART011_FEIM|
		     UART011_PEIM|UART011_BEIM|UART011_OEIM);
	UART_PUT_IMSC(port, uap->im);
}

static void pl011_enable_ms(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im |= UART011_RIMIM|UART011_CTSMIM|UART011_DCDMIM|UART011_DSRMIM;
	UART_PUT_IMSC(port, uap->im);
}

static void pl011_rx_chars(struct uart_amba_port *uap)
{
	struct tty_struct *tty = uap->port.state->port.tty;
	unsigned int status, ch, flag, max_count = 256;

	while (max_count--) {
		status = UART_GET_FR(&uap->port);
		if (!UART_RX_DATA(status))
			break;

		/* Take chars from the FIFO and update status */
		ch = UART_GET_DR(&uap->port) | UART_DUMMY_DR_RX;
		flag = TTY_NORMAL;
		uap->port.icount.rx++;

		/*
		 * Note that the error handling code is
		 * out of the main execution path
		 */
		if (unlikely(!uap->txrx_enable)) {
			continue;
		}
		if (unlikely(ch & UART_DR_ERROR)) {
			if (ch & UART011_DR_BE) {
				ch &= ~(UART011_DR_FE | UART011_DR_PE);
				uap->port.icount.brk++;
				if (uart_handle_break(&uap->port))
					continue;
			} else if (ch & UART011_DR_PE)
				uap->port.icount.parity++;
			else if (ch & UART011_DR_FE)
				uap->port.icount.frame++;
			if (ch & UART011_DR_OE)
				uap->port.icount.overrun++;

			ch &= uap->port.read_status_mask;

			if (ch & UART011_DR_BE)
				flag = TTY_BREAK;
			else if (ch & UART011_DR_PE)
				flag = TTY_PARITY;
			else if (ch & UART011_DR_FE)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(&uap->port, ch & 255))
			continue;

		uart_insert_char(&uap->port, ch, UART011_DR_OE, ch, flag);
	}
	spin_unlock(&uap->port.lock);
	tty_flip_buffer_push(tty); /* DO NOT USE low_latency mode */
	spin_lock(&uap->port.lock);
}

static void pl011_tx_chars(struct uart_amba_port *uap)
{
	struct circ_buf *xmit = &uap->port.state->xmit;
	int count;

	if (uap->port.x_char) {
		UART_PUT_CHAR(&uap->port, uap->port.x_char);
		uap->port.icount.tx++;
		uap->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&uap->port)) {
		pl011_stop_tx(&uap->port);
		return;
	}

	if (unlikely(!uap->txrx_enable)) {
		xmit->tail = xmit->head; /* empty the xmit buffer */
	} else {
#ifdef AMBAUART_SOFTIRQ
		if (uap->tx_intr_mode == 0) { /* TX intr is not available */
			count = uap->port.fifosize + 2;
			do {
				unsigned int status;
				status = UART_GET_FR(&uap->port);
				if (!UART_TX_READY(status)) { /* FIFO full ? */
					/* TX intr will be available. */
					uap->tx_intr_mode = 1;
					break;
				}
				UART_PUT_CHAR(&uap->port, xmit->buf[xmit->tail]);
				xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
				uap->port.icount.tx++;
				if (uart_circ_empty(xmit))
					break;
			} while (--count > 0);
		} else {
#endif /* AMBAUART_SOFTIRQ */
	count = uap->port.fifosize >> 1;
	do {
		UART_PUT_CHAR(&uap->port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uap->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);
#ifdef AMBAUART_SOFTIRQ
		}
#endif /* AMBAUART_SOFTIRQ */
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&uap->port);

	if (uart_circ_empty(xmit))
		pl011_stop_tx(&uap->port);
#ifdef AMBAUART_SOFTIRQ
	else {
		pl011_softirq(uap);
	}
#endif /* AMBAUART_SOFTIRQ */
}

static void pl011_modem_status(struct uart_amba_port *uap)
{
	unsigned int status, delta;

	status = UART_GET_FR(&uap->port) & UART01x_FR_MODEM_ANY;

	delta = status ^ uap->old_status;
	uap->old_status = status;

	if (!delta)
		return;

	if (delta & UART01x_FR_DCD)
		uart_handle_dcd_change(&uap->port, status & UART01x_FR_DCD);

	if (delta & UART01x_FR_DSR)
		uap->port.icount.dsr++;

	if (delta & UART01x_FR_CTS)
		uart_handle_cts_change(&uap->port, status & UART01x_FR_CTS);

	wake_up_interruptible(&uap->port.state->port.delta_msr_wait);
}

static irqreturn_t pl011_int(int irq, void *dev_id)
{
	struct uart_amba_port *uap = dev_id;
	unsigned long flags;
	unsigned int status, pass_counter = AMBA_ISR_PASS_LIMIT;
	int handled = 0;

#if defined(CONFIG_PREEMPT_RT_FULL) && defined(SUPPORT_SYSRQ)
	{
		if (unlikely(uap->is_console && !uap->boost)) {
			struct sched_param param = {
				.sched_priority = MAX_RT_PRIO-1
			};
			uap->boost = 1;
			if (ambauart_boost) {
			sched_setscheduler(current, SCHED_FIFO, &param);
			}
		}
	}
#endif /* CONFIG_PREEMPT_RT_FULL && SUPPORT_SYSRQ */
	spin_lock_irqsave(&uap->port.lock, flags);

	status = UART_GET_INT_STATUS(&uap->port);
#ifdef AMBAUART_SOFTIRQ
	if (uap->tx_intr_mode == 0) {
		if (UART_GET_RIS(&uap->port) & UART011_TXIS) {
			/* TX intr occur. */
			uap->tx_intr_mode = 1;
		} else {
			if (uap->tx_intr && (uap->im & UART011_TXIM)) {
				status |= UART011_TXIS; /* pseudo TX intr */
			}
		}
		uap->tx_intr = 0;
	}
#endif /* AMBAUART_SOFTIRQ */
	if (status) {
		do {
			UART_PUT_ICR(&uap->port, status & ~(UART011_TXIS|UART011_RTIS|UART011_RXIS));

			if (status & (UART011_RTIS|UART011_RXIS))
				pl011_rx_chars(uap);
			if (status & (UART011_DSRMIS|UART011_DCDMIS|
				      UART011_CTSMIS|UART011_RIMIS))
				pl011_modem_status(uap);
			if (status & UART011_TXIS)
				pl011_tx_chars(uap);

			if (pass_counter-- == 0)
				break;

			status = UART_GET_INT_STATUS(&uap->port);
		} while (status != 0);
		handled = 1;
	}

	spin_unlock_irqrestore(&uap->port.lock, flags);

	return IRQ_HANDLED;
}

static unsigned int pl01x_tx_empty(struct uart_port *port)
{
	unsigned int status = UART_GET_FR(port);
	return status & (UART01x_FR_BUSY|UART01x_FR_TXFF) ? 0 : TIOCSER_TEMT;
}

static unsigned int pl01x_get_mctrl(struct uart_port *port)
{
	unsigned int result = 0;
	unsigned int status = UART_GET_FR(port);

#define TIOCMBIT(uartbit, tiocmbit)		\
	if (status & (uartbit))		\
		result |= (tiocmbit)

	TIOCMBIT(UART01x_FR_DCD, TIOCM_CAR);
	TIOCMBIT(UART01x_FR_DSR, TIOCM_DSR);
	TIOCMBIT(UART01x_FR_CTS, TIOCM_CTS);
	TIOCMBIT(UART011_FR_RI, TIOCM_RNG);
#undef TIOCMBIT
	return result;
}

static void pl011_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	unsigned int cr;

	cr = UART_GET_CR(port);

#define	TIOCMBIT(tiocmbit, uartbit)		\
	if (mctrl & (tiocmbit))		\
		cr |= (uartbit);	\
	else				\
		cr &= ~(uartbit)

	TIOCMBIT(TIOCM_RTS, UART011_CR_RTS);
	TIOCMBIT(TIOCM_DTR, UART011_CR_DTR);
	TIOCMBIT(TIOCM_OUT1, UART011_CR_OUT1);
	TIOCMBIT(TIOCM_OUT2, UART011_CR_OUT2);
	TIOCMBIT(TIOCM_LOOP, UART011_CR_LBE);
#undef TIOCMBIT

	UART_PUT_CR(port, cr);
}

static void pl011_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long flags;
	unsigned int lcr_h;

	spin_lock_irqsave(&port->lock, flags);
	lcr_h = UART_GET_LCRH(port);
	if (break_state == -1)
		lcr_h |= UART01x_LCRH_BRK;
	else
		lcr_h &= ~UART01x_LCRH_BRK;
	UART_PUT_LCRH(port, lcr_h);
	spin_unlock_irqrestore(&port->lock, flags);
}

#ifdef CONFIG_CONSOLE_POLL
static int pl010_get_poll_char(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int status;

	status = UART_GET_FR(port);
	if (status & UART01x_FR_RXFE)
		return NO_POLL_CHAR;

	return UART_GET_DR(port);
}

static void pl010_put_poll_char(struct uart_port *port,
			 unsigned char ch)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	while (UART_GET_FR(port) & UART01x_FR_TXFF)
		barrier();

	UART_PUT_CHAR(port, ch);
}
#endif /* CONFIG_CONSOLE_POLL */

static int pl011_startup(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int cr;
	int retval;

	/*
	 * Try to enable the clock producer.
	 */
	retval = clk_enable(uap->clk);
	if (retval)
		goto out;

	uap->port.uartclk = clk_get_rate(uap->clk);

	// Disable Receiver and Transmitter (clears FIFOs)
	UART_PUT_CR(port, 0);
	{	/* disable FIFO to clear error in it */
		unsigned int val;
		val = UART_GET_LCRH(&uap->port);
		val &= ~UART01x_LCRH_FEN;
		UART_PUT_LCRH(&uap->port, val);
	}
	// Clear error
	UART_PUT_RSR(port, 0);
	/* Clear pending error and receive interrupts */
	UART_PUT_ICR(&uap->port, UART011_OEIS | UART011_BEIS | UART011_PEIS |
		     UART011_FEIS | UART011_RTIS | UART011_RXIS);

	/*
	 * Allocate the IRQ
	 */
	uap->boost = 0;
	retval = request_irq(uap->port.irq, pl011_int, uap->is_console?IRQF_NO_SOFTIRQ_CALL:0, UART_DEV_NAME, uap);
	if (retval)
		goto clk_dis;

	// Set FIFO level to 1/2
	UART_PUT_IFLS(&uap->port, UART011_IFLS_RX4_8|UART011_IFLS_TX4_8);

#ifdef AMBAUART_SOFTIRQ
	uap->tx_intr_mode = 0;
	uap->tx_intr = 0;
#else
	/*
	 * Provoke TX FIFO interrupt into asserting.
	 */
	cr = UART01x_CR_UARTEN | UART011_CR_TXE | UART011_CR_LBE;
	UART_PUT_CR(&uap->port, cr);
	UART_PUT_FBRD(&uap->port, 0);
	UART_PUT_IBRD(&uap->port, 1);
	UART_PUT_LCRH(&uap->port, 0);
	UART_PUT_CHAR(&uap->port, 0);
	/* about 8us */
	while (UART_GET_FR(&uap->port) & UART01x_FR_BUSY)
		barrier();
#endif /* AMBAUART_SOFTIRQ */

	// Enable receiver and transmitter.
	cr = UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE | UART011_CR_DTR | UART011_CR_OUT1;
	UART_PUT_CR(&uap->port, cr);

	/*
	 * initialise the old status of the modem signals
	 */
	uap->old_status = UART_GET_FR(&uap->port) & UART01x_FR_MODEM_ANY;

	/*
	 * Finally, enable interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	/* Clear out any spuriously appearing RX interrupts */
	UART_PUT_ICR(&uap->port, UART011_RTIS | UART011_RXIS);
	uap->im = UART011_RXIM | UART011_RTIM;
	UART_PUT_IMSC(&uap->port, uap->im);
	spin_unlock_irq(&uap->port.lock);

	return 0;

 clk_dis:
	clk_disable(uap->clk);
 out:
	return retval;
}

static void pl011_shutdown(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned long val;

	/*
	 * disable all interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	uap->im = 0;
	UART_PUT_IMSC(&uap->port, uap->im);
	UART_PUT_ICR(&uap->port, 0x0fff);
	spin_unlock_irq(&uap->port.lock);

	/*
	 * Free the interrupt
	 */
	free_irq(uap->port.irq, uap);

	/*
	 * disable the port
	 */
#if defined(CONFIG_DEBUG_LL) || defined(CONFIG_PM)
	/* we should not disable UART this case for _printch_ etc to work after SUSPEND*/
	UART_PUT_CR(port, UART01x_CR_UARTEN | UART011_CR_TXE | UART011_CR_RXE
		    | UART011_CR_DTR | UART011_CR_RTS | UART011_CR_OUT1);
#else
	UART_PUT_CR(port, UART01x_CR_UARTEN | UART011_CR_TXE);
#endif

	/*
	 * disable break condition and fifos
	 */
	val = UART_GET_LCRH(port);
	val &= ~(UART01x_LCRH_BRK | UART01x_LCRH_FEN);
	UART_PUT_LCRH(port, val);

	/*
	 * Shut down the clock producer
	 */
#ifdef CONFIG_SERIAL_AMBA2_CONSOLE_ALWAYS_ALIVE
	if (!uap->is_console)
#endif
		clk_disable(uap->clk);
}

static void
pl011_set_termios(struct uart_port *port, struct ktermios *termios,
		     struct ktermios *old)
{
	unsigned int lcr_h, old_cr;
	unsigned long flags;
	unsigned int baud, quot;

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
	quot = ((port->uartclk << 6) + 8 * baud)/(16 * baud);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr_h = UART01x_LCRH_WLEN_5;
		break;
	case CS6:
		lcr_h = UART01x_LCRH_WLEN_6;
		break;
	case CS7:
		lcr_h = UART01x_LCRH_WLEN_7;
		break;
	default: // CS8
		lcr_h = UART01x_LCRH_WLEN_8;
		break;
	}
	if (termios->c_cflag & CSTOPB)
		lcr_h |= UART01x_LCRH_STP2;
	if (termios->c_cflag & PARENB) {
		lcr_h |= UART01x_LCRH_PEN;
		if (!(termios->c_cflag & PARODD))
			lcr_h |= UART01x_LCRH_EPS;
	}
	if (port->fifosize > 1)
		lcr_h |= UART01x_LCRH_FEN;

	spin_lock_irqsave(&port->lock, flags);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask = UART011_DR_OE | 255;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART011_DR_FE | UART011_DR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UART011_DR_BE;

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART011_DR_FE | UART011_DR_PE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= UART011_DR_BE;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= UART011_DR_OE;
	}

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_DUMMY_DR_RX;

	if (UART_ENABLE_MS(port, termios->c_cflag))
		pl011_enable_ms(port);

	/* first, disable everything */
	old_cr = UART_GET_CR(port);
	UART_PUT_CR(port, 0);

	if (termios->c_cflag & CRTSCTS) {
		old_cr |= (UART011_CR_CTSEN | UART011_CR_RTSEN);
	} else {
		old_cr &= ~(UART011_CR_CTSEN | UART011_CR_RTSEN);
	}

	/* Set baud rate */
	UART_PUT_FBRD(port, quot & 0x3f);
	UART_PUT_IBRD(port, (quot & 0xfffc0) >> 6);

	/*
	 * ----------v----------v----------v----------v-----
	 * NOTE: MUST BE WRITTEN AFTER UARTLCR_M & UARTLCR_L
	 * ----------^----------^----------^----------^-----
	 */
	UART_PUT_LCRH(port, lcr_h);
	UART_PUT_CR(port, old_cr);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void pl011_pm(struct uart_port *port, unsigned int state,
		     unsigned int oldstate)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	if (0 == state) {
		clk_enable(uap->clk);
	} else {
#ifdef CONFIG_SERIAL_AMBA2_CONSOLE_ALWAYS_ALIVE
		if (!uap->is_console)
#endif
			clk_disable(uap->clk);
	}
}

static const char *pl011_type(struct uart_port *port)
{
	return port->type == PORT_AMBA ? "AMBA/PL011" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void pl010_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, SZ_4K);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int pl010_request_port(struct uart_port *port)
{
	return request_mem_region(port->mapbase, SZ_4K, UART_DEV_NAME)
			!= NULL ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void pl010_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_AMBA;
		pl010_request_port(port);
	}
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int pl010_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_AMBA)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static int amba_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	int ret, sw;

	switch (cmd) {
	case AMBAUART_IOC_GETMODE:
		if (copy_to_user((void __user *)arg, &uap->txrx_enable, sizeof (uap->txrx_enable)) != 0)
			return -EFAULT;
		ret = 0;
		break;
	case AMBAUART_IOC_SETMODE:
		if (copy_from_user((void *)&sw, (void __user *)arg, sizeof (int)) != 0)
			return -EFAULT;
		if (!uap->is_console)
			return -EPERM;
		ret = 0;
		if (sw == 0 || sw == 1) {
			uap->txrx_enable = sw;
		} else
			ret = -EINVAL;
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}
	return ret;
}

static struct uart_ops amba_pl011_pops = {
	.tx_empty	= pl01x_tx_empty,
	.set_mctrl	= pl011_set_mctrl,
	.get_mctrl	= pl01x_get_mctrl,
	.stop_tx	= pl011_stop_tx,
	.start_tx	= pl011_start_tx,
	.stop_rx	= pl011_stop_rx,
	.enable_ms	= pl011_enable_ms,
	.break_ctl	= pl011_break_ctl,
	.startup	= pl011_startup,
	.shutdown	= pl011_shutdown,
	.set_termios	= pl011_set_termios,
	.pm		= pl011_pm,
	.type		= pl011_type,
	.release_port	= pl010_release_port,
	.request_port	= pl010_request_port,
	.config_port	= pl010_config_port,
	.verify_port	= pl010_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = pl010_get_poll_char,
	.poll_put_char = pl010_put_poll_char,
#endif
	.ioctl		= amba_ioctl,
};

static struct uart_amba_port amba_ports[UART_NR];

#ifdef CONFIG_SERIAL_AMBA2_CONSOLE

void (*amba2_console_idle_callback)(void) = NULL;

#ifdef CONFIG_CONSOLE_READ
static int ambauart_console_read(struct console *co, char *s, unsigned count)
{
 	struct uart_port *port = &amba_ports[co->index].port;
 	int i;

	disable_irq(port->irq);
 	for (i = 0; i < count; i++) {
		while (UART_GET_FR(port) & UART01x_FR_RXFE) {
			if (amba2_console_idle_callback) {
				(*amba2_console_idle_callback)();
			}
			touch_nmi_watchdog();
 		}
 		*s++ = UART_GET_CHAR(port);
 	}
	enable_irq(port->irq);

 	return i;
}
#endif

static void pl011_console_putchar(struct uart_port *port, int ch)
{
	unsigned int status;

	do {
		status = UART_GET_FR(port);
		if (!UART_TX_READY(status)) {
			if (amba2_console_idle_callback) {
				(*amba2_console_idle_callback)();
			}
		}
	} while (!UART_TX_READY(status));
	UART_PUT_CHAR(port, ch);
}

static void
pl011_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_amba_port *uap = &amba_ports[co->index];
	unsigned int status, old_cr, new_cr;
	unsigned long flags;
	int locked = 1;

	if (unlikely(!uap->txrx_enable))
		return;

	clk_enable(uap->clk);

	if (uap->port.sysrq || oops_in_progress)
		locked = spin_trylock_irqsave(&uap->port.lock, flags);
	else
		spin_lock_irqsave(&uap->port.lock, flags);

	/*
	 *	First save the CR then disable the interrupts
	 */
	old_cr = UART_GET_CR(&uap->port);
	new_cr = old_cr & ~UART011_CR_CTSEN;
	new_cr |= UART01x_CR_UARTEN | UART011_CR_TXE;
	UART_PUT_CR(&uap->port, new_cr);
	UART_PUT_IMSC(&uap->port, 0);

	uart_console_write(&uap->port, s, count, pl011_console_putchar);

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore the TCR
	 */
	do {
		if (amba2_console_idle_callback) {
			(*amba2_console_idle_callback)();
		}
		status = UART_GET_FR(&uap->port);
	} while (status & UART01x_FR_BUSY);
	UART_PUT_CR(&uap->port, old_cr);
	UART_PUT_IMSC(&uap->port, uap->im);

	if (locked)
		spin_unlock_irqrestore(&uap->port.lock, flags);

#ifndef CONFIG_SERIAL_AMBA2_CONSOLE_ALWAYS_ALIVE
	clk_disable(uap->clk);
#endif
}

static void __init
pl011_console_get_options(struct uart_amba_port *uap, int *baud,
			     int *parity, int *bits)
{
	if (UART_GET_CR(&uap->port) & UART01x_CR_UARTEN) {
		unsigned int lcr_h, ibrd, fbrd;
		int b;

		lcr_h = UART_GET_LCRH(&uap->port);

		*parity = 'n';
		if (lcr_h & UART01x_LCRH_PEN) {
			if (lcr_h & UART01x_LCRH_EPS)
				*parity = 'e';
			else
				*parity = 'o';
		}

		if ((lcr_h & 0x60) == UART01x_LCRH_WLEN_7)
			*bits = 7;
		else
			*bits = 8;

		ibrd = UART_GET_IBRD(&uap->port);
		fbrd = UART_GET_FBRD(&uap->port) & 0x3f;
		b = uap->port.uartclk * 4 / (64 * ibrd + fbrd);
		b = (b + 50)/100;
		*baud = b * 100;
	}
}

static int __init pl011_console_setup(struct console *co, char *options)
{
	struct uart_amba_port *uap;
	int baud = 38400;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index < 0  ||  co->index >= UART_NR)
		co->index = 0;
	uap = &amba_ports[co->index];
	if (uap->port.dev == NULL)
		return -ENODEV;
	uap->is_console = 1;
	uap->txrx_enable = ambauart_console_enable;

	uap->port.uartclk = clk_get_rate(uap->clk);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		pl011_console_get_options(uap, &baud, &parity, &bits);

	return uart_set_options(&uap->port, co, baud, parity, bits, flow);
}

static struct uart_driver amba_reg;
static struct console amba_console = {
	.name		= "ttyAM",
	.write		= pl011_console_write,
#ifdef CONFIG_CONSOLE_READ
 	.read		= ambauart_console_read,
#endif
	.device		= uart_console_device,
	.setup		= pl011_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &amba_reg,
};

#define AMBA_CONSOLE	(&amba_console)
#else
#define AMBA_CONSOLE	NULL
#endif

#ifdef CONFIG_KGDB_AMBA2

static int kgdb_uart_port = 1;
static int kgdb_uart_baud = 115200;
static int kgdb_init(void)
{
	struct uart_port *port = &amba_ports[kgdb_uart_port].port;
	u_int baud = kgdb_uart_baud;
	u_int quot, lcr_h;

	// Disable Receiver and Transmitter (clears FIFOs)
	UART_PUT_CR(port, 0);
	// Clear sticky (writable) status bits.
	UART_PUT_CHAR(port, 0);
	UART_PUT_IMSC(port, 0);

	lcr_h = UART01x_LCRH_WLEN_8;
	quot = ((port->uartclk << 6) + 8 * baud)/(16 * baud);
	UART_PUT_IBRD(port, ((quot & 0xfffc0) >> 6));
	UART_PUT_FBRD(port, (quot & 0x3f));
	UART_PUT_LCRH(port, lcr_h);

	// Enable receiver and transmitter.
	UART_PUT_CR(port, UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE
		    | UART011_CR_DTR | UART011_CR_RTS | UART011_CR_OUT1);

	return 0;
}

static void kgdb_write_char(int ch)
{
	struct uart_port *port = &amba_ports[kgdb_uart_port].port;
	u_int status;

	do {
		status = UART_GET_FR(port);
	} while (!UART_TX_READY(status));

	UART_PUT_CHAR(port, ch);
}

static void kgdb_flush(void)
{
	struct uart_port *port = &amba_ports[kgdb_uart_port].port;
	u_int status;

	do {
		status = UART_GET_FR(port);
	} while (status & UART01x_FR_BUSY);
}

static int kgdb_read_char(void)
{
	struct uart_port *port = &amba_ports[kgdb_uart_port].port;
	u_int status;
	int ch;

	do {
		status = UART_GET_FR(port);
	} while (!UART_RX_DATA(status));

	ch = UART_GET_CHAR(port);
	return ch;
}

struct kgdb_io kgdb_io_ops = {
	.read_char  = kgdb_read_char,
	.write_char = kgdb_write_char,
	.flush      = kgdb_flush,
	.init       = kgdb_init,
};
#endif /* CONFIG_KGDB_AMBA2 */

static struct uart_driver amba_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "ttyAM",
	.dev_name		= "ttyAM",
	.major			= SERIAL_AMBA_MAJOR,
	.minor			= SERIAL_AMBA_MINOR,
	.nr			= UART_NR,
	.cons			= AMBA_CONSOLE,
};

static int pl011_probe(struct amba_device *dev, const struct amba_id *id)
{
	struct uart_amba_port *uap;
	int i, ret;

	for (i = 0, uap = amba_ports; i < UART_NR; i++, uap++) {
		if (uap->port.dev == NULL)
			break;
	}
	if (i == UART_NR)
		return -EBUSY;

	memset(uap, 0, sizeof(struct uart_amba_port));
	uap->info = dev->dev.platform_data;
	uap->clk = clk_get(&dev->dev, uap->info->clock);
	if (IS_ERR(uap->clk)) {
		return PTR_ERR(uap->clk);
	}

	uap->port.dev = &dev->dev;
	uap->port.mapbase = dev->res.start;
	uap->port.membase = (unsigned char __iomem *)IO_ADDRESSP(dev->res.start);
	uap->port.iotype = UPIO_MEM;
	uap->port.irq = dev->irq[0];
	uap->port.fifosize = uap->info->fifosize;
	uap->port.ops = &amba_pl011_pops;
	uap->port.flags = UPF_BOOT_AUTOCONF;
	uap->port.line = i;
	uap->txrx_enable = 1;
#ifdef AMBAUART_SOFTIRQ
	uap->tx_intr_mode = 0;
#endif /* AMBAUART_SOFTIRQ */

	/* Ensure interrupts from this UART are masked and cleared */
	UART_PUT_IMSC(&uap->port, 0);
	UART_PUT_ICR(&uap->port, 0x0fff);

	amba_set_drvdata(dev, uap);
	ret = uart_add_one_port(&amba_reg, &uap->port);
	if (ret) {
		amba_set_drvdata(dev, NULL);
		amba_ports[i].port.dev = NULL;
		clk_disable(uap->clk);
		clk_put(uap->clk);
	}

	return ret;
}

static int pl011_remove(struct amba_device *dev)
{
	struct uart_amba_port *uap = amba_get_drvdata(dev);
	int i;

	amba_set_drvdata(dev, NULL);

	uart_remove_one_port(&amba_reg, &uap->port);

	for (i = 0; i < UART_NR; i++)
		if (amba_ports[i].port.line == uap->port.line)
			amba_ports[i].port.dev = NULL;

	clk_disable(uap->clk);
	clk_put(uap->clk);
	return 0;
}

#ifdef CONFIG_PM
static int pl011_suspend(struct amba_device *dev, pm_message_t state)
{
	struct uart_amba_port *uap = amba_get_drvdata(dev);

	if (!uap)
		return -EINVAL;

	return uart_suspend_port(&amba_reg, &uap->port);
}

static int pl011_resume(struct amba_device *dev)
{
	struct uart_amba_port *uap = amba_get_drvdata(dev);

	if (!uap)
		return -EINVAL;

	return uart_resume_port(&amba_reg, &uap->port);
}
#endif /* CONFIG_PM */

static struct amba_id pl011_ids[] = {
	{
		.id	= 0x00041011,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};

static struct amba_driver pl011_driver = {
	.drv = {
		.name	= "uart-pl011s",
	},
	.id_table	= pl011_ids,
	.probe		= pl011_probe,
	.remove		= pl011_remove,
#ifdef CONFIG_PM
	.suspend	= pl011_suspend,
	.resume		= pl011_resume,
#endif /* CONFIG_PM */
};

#if defined(CONFIG_WBI_DEV_XMODEM)
static void ambauart_xmodem_init(struct xmodem_device *dev)
{
	struct uart_port *port = dev->data;

	// Disable Receiver and Transmitter (clears FIFOs)
	UART_PUT_CR(port, 0);
	// Clear sticky (writable) status bits.
	UART_PUT_CHAR(port, 0);
	UART_PUT_IMSC(port, 0);

	UART_PUT_CR(port, UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE
		    | UART011_CR_DTR | UART011_CR_RTS | UART011_CR_OUT1);
}

static int ambauart_inb(struct xmodem_device *dev, unsigned char *c)
{
	struct uart_port *port = dev->data;
	u_int status;

	status = UART_GET_FR(port);
	if(UART_RX_DATA(status))
		*c = UART_GET_CHAR(port);
	else
		return 0;

	return 1;
}

static int ambauart_outb(struct xmodem_device *dev, unsigned char c)
{
	struct uart_port *port = dev->data;
	u_int status;

	do {
		status = UART_GET_FR(port);
	} while (!UART_TX_READY(status));

	UART_PUT_CHAR(port, c);
	return 1;
}

static struct xmodem_device ambauart_xmodem = {
	.name           = "ttyAM0",
	.open           = ambauart_xmodem_init,
	.inb            = ambauart_inb,
	.outb           = ambauart_outb,
	.close          = NULL,
	.data           = &amba_ports[0].port,
};
#endif /* CONFIG_WBI_DEV_XMODEM */

static int __init pl011_init(void)
{
	int ret;

	ambauart_console_enable = console;

#if defined(CONFIG_WBI_DEV_XMODEM)
	xmodem_register_device(&ambauart_xmodem);
#endif /* CONFIG_WBI_DEV_XMODEM */

	ret = uart_register_driver(&amba_reg);
	if (ret == 0) {
		ret = amba_driver_register(&pl011_driver);
		if (ret)
			uart_unregister_driver(&amba_reg);
	}
	return ret;
}

static void __exit pl011_exit(void)
{
	amba_driver_unregister(&pl011_driver);
	uart_unregister_driver(&amba_reg);
}

module_init(pl011_init);
module_exit(pl011_exit);

MODULE_AUTHOR("ARM Ltd/Deep Blue Solutions Ltd");
MODULE_DESCRIPTION("ARM AMBA serial port driver");
MODULE_LICENSE("GPL");

module_param(console, int, 0);
MODULE_PARM_DESC(console, "UART console enable/disable switch.");
