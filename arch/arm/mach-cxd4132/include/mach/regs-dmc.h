/*
 * include/asm-arm/arch-cxd4132/regs-dmc.h
 *
 * CXD4132 SDC
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
#ifndef __CXD4132_REGS_DMC_H__
#define __CXD4132_REGS_DMC_H__

#define DMC_MAX_RANK		2

#define DMC_DDRCTRL_NO		0
#define DMC_DDRCTRL_SREF	1
#define DMC_DDRCTRL_DPD		2

#define SDC_PARA1	0x000
#define SDC_PARA2	0x004
# define SDC_TMRW_MASK		0xff
# define SDC_TMRW_SHIFT		8
#define SDC_PARA3	0x008
#define SDC_PARA4	0x00c
#define SDC_MEMORY	0x100
# define SDC_MODE_DDR		(1 << 21)
# define SDC_MODE2RANK		(1 << 20)
#define SDC_REF_SET	0x104
# define SDC_REF_R1		(1 << 31)
# define SDC_REF_R0		(1 << 30)
#define SDC_COMMAND1	0x200   /* for LPDDR1 */
#define SDC_COMMAND2	0x204   /* for LPDDR2 */
# define SDC_DDR2_MRR_R0	(1 << 16)
# define SDC_DDR2_MRR_R1	(1 << 17)
# define SDC_DDR2_MRW_R0	(1 << 18)
# define SDC_DDR2_MRW_R1	(1 << 19)
# define SDC_DDR2_BUSY		(SDC_DDR2_MRR_R0|SDC_DDR2_MRR_R1|SDC_DDR2_MRW_R0|SDC_DDR2_MRW_R1)
# define SDC_DDR2_MA_SHIFT	8
#define SDC_COMMAND3	0x208	/* for LPDDR1 and LPDDR2 */
# define SDC_CMD_SREF_R1	(1 << 10)
# define SDC_CMD_SREX_R1	(1 << 9)
# define SDC_CMD_DPD_R1		(1 << 8)
# define SDC_CMD_SREF_R0	(1 << 2)
# define SDC_CMD_SREX_R0	(1 << 1)
# define SDC_CMD_DPD_R0		(1 << 0)
# define SDC_CMD_BUSY		(SDC_CMD_SREF_R1|SDC_CMD_DPD_R1|SDC_CMD_SREF_R0|SDC_CMD_DPD_R0)
#define SDC_CLK_GATE	0x300
# define SDC_CLK_GATE_MONPS	(1 << 16)

#endif /* __CXD4132_REGS_DMC_H__ */
