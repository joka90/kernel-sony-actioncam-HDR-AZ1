/*
 * arch/arm/mach-cxd90014/uart.S
 *
 * UART initialize for cxd90014
 *
 * Copyright 2012 Sony Corporation
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
#include <mach/regs-gpio.h>
#include <mach/regs-octrl.h>
#include <linux/amba/serial.h>

#define CONSOLE_UART_NO 0
#define UART_PORT	2
#define UART0_TXD	(1 << 9)
#define UART0_RXD	(1 << 8)

/* setting values */

#ifdef CONFIG_CXD90014_FPGA
/* UART baudrate clock 24MHz, 115200baud */
#define __UART_BAUD_F	1
#define __UART_BAUD_I	0x0d
#else /* ASIC */
# if 0
/* UART baudrate clock 12MHz, 115200baud */
#define __UART_BAUD_F	0x21
#define __UART_BAUD_I	6
# else
/* UART baudrate clock 48MHz, 115200baud */
#define __UART_BAUD_F	3
#define __UART_BAUD_I	0x1a
# endif
#endif /* CONFIG_CXD90014_FPGA */

#define __UART_8N1_NO_FIFO    0x70

	.text
ENTRY(uart_init)
	@ Initialize UART
	@ Preserve r4
	mov	r3, lr

#ifdef CONFIG_CXD90014_UART_IO_INIT
	@ PORTSEL UART0 TXD,RXD
	ldr	r0, =CXD90014_GPIO(UART_PORT,PORT)
	mov	r1, #(UART0_TXD|UART0_RXD)
	str	r1, [r0, #GPIO_CLR]
	@ OEN
	ldr	r0, =CXD90014_OCTRL(UART_PORT) + OCTRL_OEN
	mov	r1, #UART0_TXD
	str	r1, [r0, #OCTRL_SET]
	@ IEN
	ldr	r0, =CXD90014_OCTRL(UART_PORT) + OCTRL_IEN
	mov	r1, #UART0_RXD
	str	r1, [r0, #OCTRL_SET]
	@ Pull OFF
	ldr	r0, =CXD90014_OCTRL(UART_PORT) + OCTRL_PUD1
	mov	r1, #(UART0_TXD|UART0_RXD)
	str	r1, [r0, #OCTRL_SET]
#endif /* CONFIG_CXD90014_UART_IO_INIT */

	ldr	r0, =CXD90014_UART(CONSOLE_UART_NO)
	bl	uart_setup

	mov	pc, r3
ENDPROC(uart_init)

uart_setup:
	@ Preserve r3,r4
	@ Interrupt mask
	mov	r1, #0
	str	r1, [r0, #UART011_IMSC]
	@ 115200bps,8N1
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
