/*
 * arch/arm/mach-cxd90014/fiq_drv.S
 *
 * FIQ driver
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
#include <asm/domain.h>
#include <asm/assembler.h>
#include <asm/ptrace.h>
#include <mach/debug-macro.S>
#include <mach/memory.h>
#include <mach/regs-clk.h>
#include <mach/regs-misc.h>
#include <linux/amba/serial.h>

#define DETECT_COUNT 5000000

#define _DOMAIN_MANAGER 3

/* register save area (72bytes/CPU)
 * r0 - r7
 * r8 - r14
 * r14_fiq
 * spsr_fiq
 * CPU ID
 */
#define R_OFFS(x)	((x) * 4)

/* UART 115200 baud parametes @48MHz */
#define __UART_BAUD_F		0x03
#define __UART_BAUD_I		0x1A
#define __UART_8N1_NO_FIFO	0x70

	.text
	.globl	fiq_vec
fiq_vec:
	@ IRQ and FIQ are already disabled.
	setmode	PSR_F_BIT|PSR_I_BIT|FIQ_MODE, r8	@ ensure
	mov	r8, #(domain_val(DOMAIN_USER, _DOMAIN_MANAGER) | \
		      domain_val(DOMAIN_KERNEL, _DOMAIN_MANAGER) | \
		      domain_val(DOMAIN_TABLE, _DOMAIN_MANAGER) | \
		      domain_val(DOMAIN_IO, _DOMAIN_MANAGER))
	mcr	p15, 0, r8, c3, c0, 0		@ Set domain register
	mrc	p15, 0, r10, c0, c0, 5
	and	r10, r10, #0x0f			@ r10 = CPUID
	ldr	r8, =VA_VECTORS_REGSAVE
	mov	r9, #CXD90014_FIQ_REGSIZE
	mul	r9, r9, r10
	add	r8, r8, r9			@ r8 = save area
	@ save registers
	stmia	r8, {r0 - r7}			@ save r0-r7
	str	r14, [r8, #R_OFFS(15)]		@ save r14_fiq(pc+4) to R15
	mrs	r0, spsr
	str	r0, [r8, #R_OFFS(16)]		@ save spsr_fiq(cpsr)
	@ save r8-r14
	and	r0, r0, #MODE_MASK		@ r0 = Prev operating mode
	cmp	r0, #USR_MODE
	moveq	r0, #SYSTEM_MODE		@ USR_MODE-->SYSTEM_MODE
	orr	r0, r0, #PSR_F_BIT|PSR_I_BIT
	msr	cpsr_c, r0
	mov	r0, r8
	mov	r1, r9
	mov	r2, r10
	mov	r3, r11
	mov	r4, r12
	mov	r5, r13
	mov	r6, r14
	setmode	PSR_F_BIT|PSR_I_BIT|FIQ_MODE, r9	@ ensure
	add	r9, r8, #R_OFFS(8)
	stmia	r9, {r0 - r6}			@ r8-r14
	@ save CPUID
	str	r10, [r8, #R_OFFS(17)]

	cmp	r10, #0				@ CPU0 ?
	bne	stop

	@ disable watchdog reset
	ldr	r0, =VA_MISC
	mov	r1, #MISC_RSTREQ_EN
	str	r1, [r0, #MISC_RSTREQ_CLR]

	ldr	r0, =VA_VECTORS_DEBUG
	mov	lr, pc
	mov	pc, r0

#if 0
	@ XRESET_REQ
	ldr	r0, =VA_MISC
	ldr	r1, =MISC_RSTREQ_EN|MISC_RSTREQ_ASSERT
	str	r1, [r0, #MISC_RSTREQ_SET]
#endif

stop:
	wfi					@ DSB included
	b	stop
	.ltorg
	.globl fiq_vec_end
fiq_vec_end:
	.size fiq_vec, fiq_vec_end-fiq_vec

	.globl	fiq_debug
fiq_debug:
	mov	r11, lr				@ save lr

	bl	uart_init
	mov	r10, r0				@ r10 = UART address

	@ banner
	ldr	r0, =msg
	bl	printascii
	@ dump registers
	ldr	r0, =VA_VECTORS_REGSAVE
	bl	show_regs
	ldr	r0, =VA_VECTORS_REGSAVE+CXD90014_FIQ_REGSIZE
	bl	show_regs
	ldr	r0, =VA_VECTORS_REGSAVE+CXD90014_FIQ_REGSIZE*2
	bl	show_regs
	ldr	r0, =VA_VECTORS_REGSAVE+CXD90014_FIQ_REGSIZE*3
	bl	show_regs

	mov	pc, r11				@ return


	@ uart_init
	@  input : none
	@  output: r0=UART
uart_init:
	@ autops enable
	ldr	r0, =VA_CLKRST4(6)
	mov	r1, #CXD90014_UART_CLK_SHIFT(0)
	str	r1, [r0, #CLKRST_SET]
	@ reset UART0
	ldr	r0, =VA_CLKRST4(0xA)
	mov	r1, #00000200
	str	r1, [r0, #CLKRST_CLR]	@ assert
	str	r1, [r0, #CLKRST_SET]	@ negate
	ldr	r1, [r0, #CLKRST_DATA]	@ stat
	@ UART will be ready.
#if 0
	@ select UART pin function
	ldr	r0, =VA_GPIO(2)
	mov	r1, #0x00000300			@ UART0
	str	r1, [r0, #GPIO_PORTSELCLR]
#endif
	@ initialize UART0
	ldr	r0, =IO_ADDRESS(CXD90014_UART(0))
	@ Interrupt mask
	mov	r1, #0
	str	r1, [r0, #UART011_IMSC]
	@ 115200,8N1 (CLK=12MHz)
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

	@ show registers
show_regs:
	mov	r12, lr				@ save lr
	mov	r4, r0
	mov	r5, #0
1:
	ldr	r0, [r4], #4			@ r0 <- [r4++]
	bl	printhex
	ldr	r0, =SPC
	bl	printascii
	add	r5, r5, #1
	ands	r6, r5, #7
	bne	2f
	ldr	r0, =CRLF
	bl	printascii
2:
	cmp	r5, #CXD90014_FIQ_N_REGS
	blt	1b
	ldr	r0, =CRLF
	bl	printascii
	mov	pc, r12				@ return

	@ input : r0=data, r10=UART
	@ corrupt: r0-r3
printhex:
	adr	r2, hexbuf
	add	r3, r2, #8			@ LSB
	mov	r1, #0
	strb	r1, [r3]			@ '\0'
1:
	and	r1, r0, #0xf
	mov	r0, r0, lsr #4
	cmp	r1, #10
	addlt	r1, r1, #'0'
	addge	r1, r1, #'a' - 10
	strb	r1, [r3, #-1]!
	teq	r3, r2
	bne	1b
	mov	r0, r2
	b	printascii

	@ input : r0=string, r10=UART
	@ corrupt: r0-r2
printascii:
	ldrb	r1, [r0], #1
	cmp	r1, #0
	beq	1f
	waituart r2, r10
	senduart r1, r10
	b	printascii
1:
	busyuart r2, r10
	mov	pc, lr

hexbuf:	.space 9

msg:	.asciz	"FIQ\r\n"
CRLF:	.asciz	"\r\n"
SPC:	.asciz	" "
	.ltorg
	.globl	fiq_debug_end
fiq_debug_end:
	.size fiq_debug, fiq_debug_end-fiq_debug
