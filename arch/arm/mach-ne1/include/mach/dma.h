/* 2008-09-01: File added by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/include/mach/dma.h
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on include/asm-arm/arch-realview/dma.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#define NE1_DMAC_MAX_P_CHANNELS	6

/* DMAC logical channel ID */

/* AHBDMAC0 */
#define NE1_DMAC0_LCH0	(0x00)
#define NE1_DMAC0_LCH1	(0x01)
#define NE1_DMAC0_LCH2	(0x02)	/* UART0 RX */
#define NE1_DMAC0_LCH3	(0x03)	/* UART0 TX */
#define NE1_DMAC0_LCH4	(0x04)	/* UART1 RX */
#define NE1_DMAC0_LCH5	(0x05)	/* UART1 TX */
#define NE1_DMAC0_LCH6	(0x06)	/* UART2 RX */
#define NE1_DMAC0_LCH7	(0x07)	/* UART2 TX */
#define NE1_DMAC0_MAX_L_CHANNELS	8

/* AHBDMAC1 */
#define NE1_DMAC1_LCH0	(0x10)	/* UART3 RX */
#define NE1_DMAC1_LCH1	(0x11)	/* UART3 TX */
#define NE1_DMAC1_LCH2	(0x12)	/* UART4 RX */
#define NE1_DMAC1_LCH3	(0x13)	/* UART4 TX */
#define NE1_DMAC1_LCH4	(0x14)	/* UART5 RX */
#define NE1_DMAC1_LCH5	(0x15)	/* UART5 TX */
#define NE1_DMAC1_LCH6	(0x16)	/* UART6 RX */
#define NE1_DMAC1_LCH7	(0x17)	/* UART6 TX */
#define NE1_DMAC1_MAX_L_CHANNELS	8

/* AHBDMAC2 */
#define NE1_DMAC2_LCH0	(0x20)	/* UART7 RX */
#define NE1_DMAC2_LCH1	(0x21)	/* UART7 TX */
#define NE1_DMAC2_LCH2	(0x22)	/* SD Read */
#define NE1_DMAC2_LCH3	(0x23)	/* SD Write */
#define NE1_DMAC2_LCH4	(0x24)
#define NE1_DMAC2_LCH5	(0x25)
#define NE1_DMAC2_LCH6	(0x26)	/* SPDIF RX */
#define NE1_DMAC2_LCH7	(0x27)	/* SPDIF TX */
#define NE1_DMAC2_MAX_L_CHANNELS	8

/* AHBDMAC3 */
#define NE1_DMAC3_LCH0	(0x30)	/* I2S0 RX */
#define NE1_DMAC3_LCH1	(0x31)	/* I2S0 TX */
#define NE1_DMAC3_LCH2	(0x32)	/* I2S1 RX */
#define NE1_DMAC3_LCH3	(0x33)	/* I2S1 TX */
#define NE1_DMAC3_LCH4	(0x34)	/* I2S2 RX */
#define NE1_DMAC3_LCH5	(0x35)	/* I2S2 TX */
#define NE1_DMAC3_LCH6	(0x36)	/* I2S3 RX */
#define NE1_DMAC3_LCH7	(0x37)	/* I2S3 TX */
#define NE1_DMAC3_MAX_L_CHANNELS	8

/* AHBDMAC4 */
#define NE1_DMAC4_LCH0	(0x40)	/* CSI0 RX */
#define NE1_DMAC4_LCH1	(0x41)	/* CSI0 TX */
#define NE1_DMAC4_LCH2	(0x42)	/* CSI1 RX */
#define NE1_DMAC4_LCH3	(0x43)	/* CSI1 TX */
#define NE1_DMAC4_LCH4	(0x44)
#define NE1_DMAC4_LCH5	(0x45)
#define NE1_DMAC4_LCH6	(0x46)
#define NE1_DMAC4_LCH7	(0x47)
#define NE1_DMAC4_MAX_L_CHANNELS	8

/* AHBEXDMAC */
#define NE1_DMAC_EX_LCH0	(0x50)	/* ExBUS0 */
#define NE1_DMAC_EX_LCH1	(0x51)
#define NE1_DMAC_EX_LCH2	(0x52)
#define NE1_DMAC_EX_LCH3	(0x53)
#define NE1_DMAC_EX_MAX_L_CHANNELS	4


typedef struct {
	volatile u32	dsab;
	volatile u32	ddab;
	volatile u32	dtcb;
	volatile u32	dsaw;
	volatile u32	ddaw;
	volatile u32	dtcw;
	volatile u32	dchc;
	volatile u32	dchs;
} dma_regs_t;

#define NE1_DMAC_DCHC_HP		(0xD << 28)
#define NE1_DMAC_DCHC_TM_SINGLE	(0 << 24)
#define NE1_DMAC_DCHC_TM_BLOCK	(1 << 24)
#define NE1_DMAC_DCHC_DAD		(1 << 23)
#define NE1_DMAC_DCHC_DDS8		(0 << 20)
#define NE1_DMAC_DCHC_DDS16		(1 << 20)
#define NE1_DMAC_DCHC_DDS32		(2 << 20)
#define NE1_DMAC_DCHC_DDS128	(4 << 20)
#define NE1_DMAC_DCHC_DDS256	(5 << 20)
#define NE1_DMAC_DCHC_DDS512	(6 << 20)
#define NE1_DMAC_DCHC_SAD		(1 << 19)
#define NE1_DMAC_DCHC_SDS8		(0 << 16)
#define NE1_DMAC_DCHC_SDS16		(1 << 16)
#define NE1_DMAC_DCHC_SDS32		(2 << 16)
#define NE1_DMAC_DCHC_SDS128	(4 << 16)
#define NE1_DMAC_DCHC_SDS256	(5 << 16)
#define NE1_DMAC_DCHC_SDS512	(6 << 16)
#define NE1_DMAC_DCHC_TCM_NOMASK (0 << 14)
#define NE1_DMAC_DCHC_TCM_MASK	(1 << 14)
#define NE1_DMAC_DCHC_AM_PULSE	(0 << 12)
#define NE1_DMAC_DCHC_AM_LEVEL	(1 << 12)
#define NE1_DMAC_DCHC_AM_MASK	(2 << 12)
#define NE1_DMAC_DCHC_EDGE		(1 << 10)
#define NE1_DMAC_DCHC_LVL		(1 << 10)
#define NE1_DMAC_DCHC_HIEN		(1 << 9)
#define NE1_DMAC_DCHC_LOEN		(1 << 8)
#define NE1_DMAC_DCHC_REQD_S	(0 << 4)
#define NE1_DMAC_DCHC_REQD_D	(1 << 4)
#define NE1_DMAC_DCHC_SEL0		(0 << 0)
#define NE1_DMAC_DCHC_SEL1		(1 << 0)
#define NE1_DMAC_DCHC_SEL2		(2 << 0)
#define NE1_DMAC_DCHC_SEL3		(3 << 0)
#define NE1_DMAC_DCHC_SEL4		(4 << 0)
#define NE1_DMAC_DCHC_SEL5		(5 << 0)
#define NE1_DMAC_DCHC_SEL6		(6 << 0)
#define NE1_DMAC_DCHC_SEL7		(7 << 0)

#define NE1_DMAC_DCHC_DSIZE_MASK	(7 << 20)
#define NE1_DMAC_DCHC_SSIZE_MASK	(7 << 16)

#define NE1_DMAC_MODE_M2P_32BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DAD | NE1_DMAC_DCHC_DDS32 | \
	NE1_DMAC_DCHC_SDS32 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_D \
	)
#define NE1_DMAC_MODE_M2P_16BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DAD | NE1_DMAC_DCHC_DDS16 | \
	NE1_DMAC_DCHC_SDS32 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_D \
	)
#define NE1_DMAC_MODE_M2P_8BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DAD | NE1_DMAC_DCHC_DDS8 | \
	NE1_DMAC_DCHC_SDS32 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_D \
	)

#define NE1_DMAC_MODE_P2M_32BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS32 | \
	NE1_DMAC_DCHC_SAD | NE1_DMAC_DCHC_SDS32 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_S \
	)
#define NE1_DMAC_MODE_P2M_16BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS32 | \
	NE1_DMAC_DCHC_SAD | NE1_DMAC_DCHC_SDS16 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_S \
	)
#define NE1_DMAC_MODE_P2M_8BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS32 | \
	NE1_DMAC_DCHC_SAD | NE1_DMAC_DCHC_SDS8 | \
	NE1_DMAC_DCHC_TCM_NOMASK | \
	NE1_DMAC_DCHC_REQD_S \
	)

#define NE1_DMAC_MODE_M2M_32BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS32 | \
	NE1_DMAC_DCHC_SDS32 | \
	NE1_DMAC_DCHC_TCM_MASK | \
	NE1_DMAC_DCHC_AM_MASK \
	)
#define NE1_DMAC_MODE_M2M_16BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS16 | \
	NE1_DMAC_DCHC_SDS16 | \
	NE1_DMAC_DCHC_TCM_MASK | \
	NE1_DMAC_DCHC_AM_MASK \
	)
#define NE1_DMAC_MODE_M2M_8BIT	\
	(NE1_DMAC_DCHC_HP | NE1_DMAC_DCHC_TM_SINGLE | \
	NE1_DMAC_DCHC_DDS8 | \
	NE1_DMAC_DCHC_SDS8 | \
	NE1_DMAC_DCHC_TCM_MASK | \
	NE1_DMAC_DCHC_AM_MASK \
	)

#define NE1_DMAC_MODE_REQ_LOL (NE1_DMAC_DCHC_LVL  | NE1_DMAC_DCHC_LOEN)
#define NE1_DMAC_MODE_REQ_HIL (NE1_DMAC_DCHC_LVL  | NE1_DMAC_DCHC_HIEN)
#define NE1_DMAC_MODE_REQ_LOE (NE1_DMAC_DCHC_EDGE | NE1_DMAC_DCHC_LOEN)
#define NE1_DMAC_MODE_REQ_HIE (NE1_DMAC_DCHC_EDGE | NE1_DMAC_DCHC_HIEN)


#define NE1_DMAC_DCHS_BVALID	(1 << 9)
#define NE1_DMAC_DCHS_TC		(1 << 8)
#define NE1_DMAC_DCHS_END		(1 << 7)
#define NE1_DMAC_DCHS_ER		(1 << 6)
#define NE1_DMAC_DCHS_ACT		(1 << 5)
#define NE1_DMAC_DCHS_RQST		(1 << 4)
#define NE1_DMAC_DCHS_FCLR		(1 << 3)
#define NE1_DMAC_DCHS_STG		(1 << 2)
#define NE1_DMAC_DCHS_EN_EN		(1 << 1)
#define NE1_DMAC_DCHS_EN		(1 << 0)

#define NE1_DMAC_INT_ERR	1
#define NE1_DMAC_INT_END	2

/*---------------------------------------------------------------------*/

typedef void	(*dma_callback_t)(void *data, int intsts);


extern int		ne1_request_dma(int channel, const char *device_id,
					  dma_callback_t callback, void *data,
					  dma_regs_t **dma_regs);
extern void		ne1_free_dma(int channel);
extern int		ne1_start_dma(int channel, dma_addr_t src_ptr, u_int size,
					dma_addr_t dst_ptr, int intmask);
extern unsigned int	ne1_dma_status(int channel);
extern int		ne1_dma_busy(void);
extern void		ne1_clear_dma(int channel);
extern void		ne1_stop_dma(int channel);

#endif /* __ASM_ARCH_DMA_H */
