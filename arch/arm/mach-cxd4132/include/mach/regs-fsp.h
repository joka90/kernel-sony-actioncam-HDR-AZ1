/*
 * include/asm-arm/arch-cxd4132/regs-fsp.h
 *
 * CXD4132 FSP registers
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
#ifndef __MACH_CXD4132_REGS_FSP_H__
#define __MACH_CXD4132_REGS_FSP_H__

#define FSP_PHY_BASE(phy)	(0x400 + ((phy) << 8))

#define FSP_PHY_REG(reg)	FSP_PHY_REG_##reg
#define FSP_PHY_REG_CFG1	0x00
# define FSP_PHY_CFG1_MBSWP	(1<<13)
# define FSP_PHY_CFG1_DW16	(0x01)
#define FSP_PHY_REG_CFG2	0x04
#define FSP_PHY_REG_WBSETADR	0x08
#define FSP_PHY_REG_WBSETBIT	0x0c
#define FSP_PHY_REG_RBSETADR	0x10
#define FSP_PHY_REG_RBSETBIT	0x14
#define FSP_PHY_REG_PRM1	0x20
#define FSP_PHY_REG_PRM2	0x24
#define FSP_PHY_REG_PRM3	0x28
#define FSP_PHY_REG_PRM4	0x2c
#define FSP_PHY_REG_PRM5	0x30
#define FSP_PHY_REG_PRM6	0x34
#define FSP_PHY_REG_IOCTRL	0x38
#define FSP_PHY(phy,reg)	(FSP_PHY_BASE(phy) + FSP_PHY_REG(reg))

#define FSP_PHY_START(phy)	(0x780 + ((phy) << 3))
#define FSP_PHY_END(phy)	(FSP_PHY_START(phy) + 4)
#define FSP_PHY_BUSCLEAN	0x7a0

#endif /* __MACH_CXD4132_REGS_FSP_H__ */
