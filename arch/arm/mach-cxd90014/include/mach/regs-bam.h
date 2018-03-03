/*
 * include/asm-arm/arch-cxd90014/regs-bam.h
 *
 * BAM definitions
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
#ifndef __ARCH_CXD90014_REGS_BAM_H
#define __ARCH_CXD90014_REGS_BAM_H

#define BAM_N_CH		3
#define BAM_MON_CTRL		0x0000
# define BAM_MON_START		1
# define BAM_MON_STOP		0
#define BAM_MODE_SET		0x0004
# define BAM_MODE_MEMCHK	(1 << 29)
# define BAM_CTRLMODE_CPU	(0 << 24)
#define BAM_MON_TGT		0x0008
# define BAM_MON_TGT_SHIFT	4
# define BAM_TGT_DDR		0
# define BAM_TGT_ESRAM0		1
# define BAM_TGT_ESRAM1		2
# define BAM_TGT_ESRAM2		3
# define BAM_TGT_ESRAM3		4
# define BAM_TGT_NONE		7
#define BAM_INT_STATUS		0x0040
#define BAM_INT_ENABLE		0x0048
# define BAM_INT_CH(x)		(1 << (x)*4)
# define BAM_INT_ALL		0x111

#define BAM_CH_BASE(x)		(0x200+(x)*0x300)
#define BAM_ID_SEL(x)		(0x20+(x)*4)
# define BAM_IFLT_EN			0x80000000
# define BAM_IFLT_INV			0x01000000
# define BAM_IFLT_DDR_MSK_NAND		(BAM_IFLT_EN|0xb30004)
#define BAM_MEMCHK_MSK_IP0	0x080
#define BAM_MEMCHK_MSK_IP1	0x084
# define BAM_MEMCHK_DDR_MSK_CPU		(3UL << 17) /* WIP49,50 */
# define BAM_MEMCHK_ESRAM_MSK_CPU	(3UL << 16) /* WIP48,49 */
#define BAM_MEMCHK_START	0x090
#define BAM_MEMCHK_END	       	0x094
#define BAM_MEMCHK_NG_ID       	0x220
#define BAM_MEMCHK_NG_ADDR     	0x224

#define BAM_MEMCHK_ALIGN	(16 - 1)

#endif /* __ARCH_CXD90014_REGS_BAM_H */
