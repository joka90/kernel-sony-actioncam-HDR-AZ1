/*
 * include/asm-arm/arch-cxd4115/regs-clk.h
 *
 * CXD4115 HSTCTRL registers
 *
 * Copyright 2007,2008,2009,2010 Sony Corporation
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
#define CLKRST_DATA		0x00
#define CLKRST_SET		0x04
#define CLKRST_CLR		0x08

#define VA_CLKRST_DATA(x)	(VA_CLKRST(x) + CLKRST_DATA)
#define VA_CLKRST_SET(x)	(VA_CLKRST(x) + CLKRST_SET)
#define VA_CLKRST_CLR(x)	(VA_CLKRST(x) + CLKRST_CLR)

#define CLKRST_SEL_CKEN		0x022
/* bit off == HIGH freq.,  bit on == LOW freq. */
# define SEL_CKEN_UART(x)	(0x10 << ((x)*4))
#define CXD4132_UART_CLK_HIGH  48000000
#define CXD4132_UART_CLK_LOW   12000000

/* FCS */
#define FCS_BASE		(CXD4132_CLKRST(0) + 0x600)
#define FCS_SIZE		0x200
#define VA_FCS(x)		(VA_CLKRST(0) + (x))
#define FCS_MASKED_STATUS	0x610
#define FCS_INT_ENABLE		0x620
# define FCS_INT_FIN		 0x00000001
#define FCS_CK_STATUS		0x730
# define FCS_BUSY		 0x00000100
# define FCS_AVBHOST_STAT 	 0x00000003
#  define FCS_FULL			  0
#  define FCS_HAVB_FHOST		  1
#  define FCS_HALF			  2

#endif /* __REGS_CLK_H__ */
