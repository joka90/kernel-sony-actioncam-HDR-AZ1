/*
 * arch/arm/mach-cxd4132/include/mach/cmpr.h
 *
 * Compressor header
 *
 * Copyright 2011 Sony Corporation
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
 *
 */

#ifndef __CMPR_H__
#define __CMPR_H__

/* address */
#define CMPR1_ENV_CLK_ADDR	0xf34000b0
#define CMPR1_ENV_RST_ADDR	0xf34002b0
#define CMPR1_COMMON_ADDR	0xf34a0000
#define CMPR1_CORE_ADDR		(CMPR1_COMMON_ADDR+0x100)

#define CMPR2_ENV_CLK_ADDR	0xf34000c0
#define CMPR2_ENV_RST_ADDR	0xf34002c0
#define CMPR2_COMMON_ADDR	0xf34b0000
#define CMPR2_CORE_ADDR		(CMPR2_COMMON_ADDR+0x100)

#define CMPR3_ENV_CLK_ADDR	0xf3400130
#define CMPR3_ENV_RST_ADDR	0xf3400330
#define CMPR3_COMMON_ADDR	0xf34c0000
#define CMPR3_CORE_ADDR		(CMPR3_COMMON_ADDR+0x100)

#define CMPR_ADDR_CPU2CMPR(addr) (uint32_t)(0x3FFFFFFF & ((unsigned long)addr))
#define CMPR_ALIGN 128

/* CLKRST */
#define HTP_CLKRST_SET	0
#define HTP_CLKRST_CLR	4

#define CMPR_ENV_CLK_MASK 0x5
#define CMPR_ENV_RST_MASK 0x11

/* COMMON */
#define CMPR_CMN_CKEN	0x0
# define CMPR_CMN_CKEN_BIT	0x1
#define CMPR_CMN_XRST	0x4
# define CMPR_CMN_XRST_BIT	0x1
#define CMPR_CMN_IPC	0x8
#define CMPR_ADRS_OFST	0x30
#define REG_CMN(x)	(cmpr_common + (x))

#define CMPR_ADDR_BASE		(uint32_t)(0x80000000)
#define CMPR_ADDR_OFST(addr)	(uint32_t)(0xFFFFFF80 & (((unsigned long)addr) - (CMPR_ADDR_BASE)))
#define CMPR_MMU_TBL(addr)	(uint32_t)(0x3fffff80 & ((unsigned long)addr))

/* WORK VADR */
#define CMPR_VADR_ARG_STA	((unsigned long)0x00000000)
#define CMPR_VADR_ARG_END	((unsigned long)0x0000007F)

#define CMPR_VADR_SRC_STA	((unsigned long)0x01000000)
#define CMPR_VADR_SRC_END	((unsigned long)0x01FFFFFF)

#define CMPR_VADR_DST_STA	((unsigned long)0x02000000)
#define CMPR_VADR_DST_END	((unsigned long)0x02FFFFFF)

#define CMPR_VADR_WRK_STA	((unsigned long)0x03000000)
#define CMPR_VADR_WRK_END	((unsigned long)0x03FFFFFF)

#define CMPR_PADR(addr)		(uint32_t)(0x7FFFFF80 & (unsigned long)addr)
#define CMPR_VADR(addr)		(uint32_t)(0x1FFFFF80 & (unsigned long)addr)

/* CORE */
#define CMPR_COR_IMSR	4
# define CMPR_ENDFCT_DONE	0x01
# define CMPR_ENDFCT_INIT	0xff
#define CMPR_COR_IENR	8
# define CMPR_COR_IENR_BIT	0xff
#define CMPR_COR_MBX(x)	(16 + ((x) << 2))
# define CMPR_COR_MBX3_CANCEL	0x1
#define REG_COR(x)	(cmpr_core + (x))

struct cmpr_parg {
	uint32_t src_adr;
	uint32_t dst_adr;
	uint32_t work_adr;
	uint32_t input_size;
	uint16_t blk_bits;
#define CMP_DEFAULT_BLKBITS 17
	uint16_t reserved;
	uint16_t index_len;
#define CMP_DEFAULT_INDEX_LEN 10
	uint16_t reserved2;
} __attribute__ ((aligned(CMPR_ALIGN)));

#define CMPR_LZPART_MAGIC (('L'<<24)|('Z'<<16)|('P'<<8)|('T'))
#define CMPR_LZPART_BLKBITS (0x11)

#endif /* __CMPR_H__ */
