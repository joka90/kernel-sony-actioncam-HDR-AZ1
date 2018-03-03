/*
 * arch/arm/mach-cxd90014/include/mach/regs-clk.h
 *
 * CXD90014 CLKRST registers
 *
 * Copyright 2011,2012,2013 Sony Corporation
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
#ifndef __REGS_CLK_H__
#define __REGS_CLK_H__

/* CLKRST */
#define CLKRST_DATA	0x00
#define CLKRST_SET	0x04
#define CLKRST_CLR	0x08

/* IPCLKEN offset */
#define IPCLKEN0	2
#define IPCLKEN1	3
#define IPCLKEN2	4
#define IPCLKEN3	5
#define IPCLKEN4	6
#define IPCLKEN5	7
#define IPCLKEN6	8

/* UART baud clock */
#define CXD90014_UART_CLK_SHIFT(x)	(20 + ((x) << 2))

#define CLKRST_SEL_CKEN		0x022
/* bit off == HIGH freq.,  bit on == LOW freq. */
# define SEL_CKEN_UART(x)	(0x10 << ((x)*4))

#ifdef CONFIG_CXD90014_FPGA
#define CXD90014_UART_CLK_HIGH  24000000
#define CXD90014_UART_CLK_LOW   6000000
#else
#define CXD90014_UART_CLK_HIGH  48000000
#define CXD90014_UART_CLK_LOW   12000000
#endif

#define CLKRST_PLLSEL	0x10

#define CRG_CRCDC	0x30
#define CRG_CRDM(x)	(0x100 + ((x) << 4))

/* CLKRST3, IPCLKEN0 */
#define REG_XPS_CK_AVB_PCIE		(1 << 18)
/* CLKRST3, IPCLKEN1 */
#define REG_XPS_CK_AXI01_PCIE		(1 << 16)
#define REG_XPS_CK_AXI01_PCIE_H		(1 << 20)

#endif /* __REGS_CLK_H__ */
