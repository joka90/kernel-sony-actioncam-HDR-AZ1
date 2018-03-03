/*
 * arch/arm/mach-cxd4132/uart.S
 *
 * UART initialize for cxd4132
 *
 * Copyright 2010 Sony Corporation
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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gic.h>
#include <mach/regs-permisc.h>
#include <linux/amba/serial.h>

#define CONSOLE_UART_NO 0

/* setting values */

/* UART baudrate clock 12MHz, 115200baud */
#define __UART_BAUD_F         0x21
#define __UART_BAUD_I          0x6

#define __UART_8N1_NO_FIFO    0x70

	.text
ENTRY(uart_init)
	@ Initialize UART
	@ Preserve r4
	mov	r3, lr

	// select baudrate CLK
	//   12MHz is default.

	// enable baudrate CLK
	ldr	r1, =CXD4132_PER_CLKRST_UART_CLK(CONSOLE_UART_NO)
	mov	r0, #0x01
	str	r0, [r1]
#if CONFIG_CXD4132_DEBUG_UART_NO != CONSOLE_UART_NO
	ldr	r1, =CXD4132_PER_CLKRST_UART_CLK(CONFIG_CXD4132_DEBUG_UART_NO)
	mov	r0, #0x01
	str	r0, [r1]
	@ set IRQ affinity to CPU0
	ldr	r1, =CXD4132_GIC_DIST_BASE+GIC_DIST_TARGET+IRQ_UART(CONFIG_CXD4132_DEBUG_UART_NO)
	mov	r0, #0x01
	strb	r0, [r1]
#endif

	ldr	r0, =CXD4115_UART(CONSOLE_UART_NO)
	bl	uart_setup
#if CONFIG_CXD4132_DEBUG_UART_NO != CONSOLE_UART_NO
	ldr	r0, =CXD4115_UART(CONFIG_CXD4132_DEBUG_UART_NO)
	bl	uart_setup
#endif
	mov	pc, r3

uart_setup:
	@ Preserve r3,r4
	@ Interrupt mask
	mov	r1, #0
	str	r1, [r0, #UART011_IMSC]
	@ 115200bps,8N1 (CLK=12MHz)
	mov	r1, #__UART_BAUD_F
	str	r1, [r0, #UART011_FBRD]
	mov	r1, #__UART_BAUD_I
	str	r1, [r0, #UART011_IBRD]
	mov	r1, #__UART_8N1_NO_FIFO
	str	r1, [r0, #UART011_LCRH]
	@ clear error
	mov	r1, #0
	str	r1, [r0, #UART01x_ECR]
	@ TX enable
	mov	r1, #UART011_CR_TXE
	orr	r1, #UART01x_CR_UARTEN
	str	r1, [r0, #UART011_CR]

	mov	pc, lr
