/*
 * include/asm-arm/arch-cxd4132/uart.h
 *
 * UART definitions
 *
 * Copyright 2009,2010 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */
#ifndef __ARCH_CXD90014_UART_H
#define __ARCH_CXD90014_UART_H

#define UART_NR			3
#define UART_DEV_NAME          "cxd90014-uart"

/* register definitions */
#define AMBAUART_RTO		0x1c	 /*  Receive timeout register */
#define UART_RTO_CHAR(x)	((x) * 8 * 16 - 1) /* RTO is (x)*8 bits-time */
#define UART_RTO_DEFAULT	32

/* bit definitions */
#define AMBAUART_CR_SIRINV	0x08

struct uart_platform_data {
	int chan;
	int fifosize;
	char *clock;
};

#include <mach/gic_export.h>
#define AMBAUART_SOFTIRQ(x)	gic_setirq(x)

#endif /* __ARCH_CXD90014_UART_H */
