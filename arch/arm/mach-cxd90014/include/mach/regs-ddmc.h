/*
 * include/asm-arm/arch-cxd90014/regs-ddmc.h
 *
 * CXD90014 DDMC
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
#ifndef __CXD90014_REGS_DDMC_H__
#define __CXD90014_REGS_DDMC_H__

#define CXD90014_DDMC_BASE 0xf0104000

//sref

#define DDMC_CTL_30 0x78
#define DDMC_CTL_31 0x7c

#define DDMC_LP_CMD_VAL     0x0a000000
#define DDMC_LP_CMD_MASK    0x00ffffff

#define DDMC_LP_STAT_VAL    0x00000025
#define DDMC_LP_STAT_MASK   0x000000ff

//pasr

#define DDMC_CTL_43 0xac	// mode reg write
#define DDMC_CTL_49 0xc4	// rank0
#define DDMC_CTL_54 0xd8	// rank1
#define DDMC_CTL_74 0x128	// int status
#define DDMC_CTL_75 0x12c	// int ack

#define DDMC_PASR_MODE1_R0	0x00800011
#define DDMC_PASR_MODE2_R0	0x02800011
#define DDMC_PASR_MODE1_R1	0x00800111
#define DDMC_PASR_MODE2_R1	0x02800111
#define DDMC_PASR_STST_CLR	0x1000
#define DDMC_PASR_STST_MASK	0x1000

#endif /* __CXD90014_REGS_DDMC_H__ */
