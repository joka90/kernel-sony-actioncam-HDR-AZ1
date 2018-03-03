/* 2009-02-24: File added and changed by Sony Corporation */
/*
 *  File Name		: linux/arch/arm/mach-ne1/dma.c
 *  Function		: dmac
 *  Release Version : Ver 1.00
 *  Release Date	: 2008/05/23
 *
 *  Copyright (C) NEC Electronics Corporation 2008
 *
 *
 *  This program is free software;you can redistribute it and/or modify it under the terms of
 *  the GNU General Public License as published by Free Softwere Foundation; either version 2
 *  of License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warrnty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program;
 *  If not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/dma.h>
#include <mach/hardware.h>

#include <linux/platform_device.h>
#include <linux/delay.h>

#include "dma.h"

/****  Prototype definition  ****/

static int setup_M2M(dma_regs_t *regs, dma_addr_t src_ptr, u_int size, dma_addr_t dst_ptr, u_int32_t mode);
static int setup_M2P(dma_regs_t *regs, dma_addr_t src_ptr, u_int size, dma_addr_t dst_ptr, u_int32_t mode);
static int setup_P2M(dma_regs_t *regs, dma_addr_t src_ptr, u_int size, dma_addr_t dst_ptr, u_int32_t mode);

/****  Structured data definition  ****/

ne1_dmal_t dmac0_lch[NE1_DMAC0_MAX_L_CHANNELS] = {
	{
		.name    = "DMAC0 0",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL0,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC0 1",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC0 UART0 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL2,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC0 UART0 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC0 UART1 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL4,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC0 UART1 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL5,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC0 UART2 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL6,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC0 UART2 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL7,
		.setup   = setup_M2P,
	},
};

ne1_dmal_t dmac1_lch[NE1_DMAC1_MAX_L_CHANNELS] = {
	{
		.name    = "DMAC1 UART3 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL0,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC1 UART3 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC1 UART4 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL2,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC1 UART4 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC1 UART5 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL4,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC1 UART5 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL5,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC1 UART6 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL6,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC1 UART6 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL7,
		.setup   = setup_M2P,
	},
};

ne1_dmal_t dmac2_lch[NE1_DMAC2_MAX_L_CHANNELS] = {
	{
		.name    = "DMAC2 UART7 RX",
		.defmode = NE1_DMAC_MODE_P2M_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL0,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC2 UART7 TX",
		.defmode = NE1_DMAC_MODE_M2P_8BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC2 SD RX",
		.defmode = NE1_DMAC_MODE_P2M_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_MASK |
		           NE1_DMAC_DCHC_SEL2,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC2 SD TX",
		.defmode = NE1_DMAC_MODE_M2P_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_MASK |
		           NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC2 4",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL4,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC2 5",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL5,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC2 SPDIF RX",
		.defmode = NE1_DMAC_MODE_P2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL6,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC2 SPDOF TX",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL7,
		.setup   = setup_M2P,
	},
};

ne1_dmal_t dmac3_lch[NE1_DMAC3_MAX_L_CHANNELS] = {
	{
		.name    = "DMAC3 I2S0 RX",
		.defmode = NE1_DMAC_MODE_P2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL0,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC3 I2S0 TX",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC3 I2S1 RX",
		.defmode = NE1_DMAC_MODE_P2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL2,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC3 I2S1 TX",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC3 I2S2 RX",
		.defmode = NE1_DMAC_MODE_P2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL4,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC3 I2S2 TX",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL5,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC3 I2S3 RX",
		.defmode = NE1_DMAC_MODE_P2M_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL6,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC3 I2S3 TX",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL7,
		.setup   = setup_M2P,
	},
};

ne1_dmal_t dmac4_lch[NE1_DMAC4_MAX_L_CHANNELS] = {
	{
		.name    = "DMAC4 CSI0 RX",
		.defmode = NE1_DMAC_MODE_P2M_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL0,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC4 CSI0 TX",
		.defmode = NE1_DMAC_MODE_M2P_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC4 CSI1 RX",
		.defmode = NE1_DMAC_MODE_P2M_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL2,
		.setup   = setup_P2M,
	},
	{
		.name    = "DMAC4 CSI1 TX",
		.defmode = NE1_DMAC_MODE_M2P_16BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC4 4",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL4,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC4 5",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL5,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC4 6",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL6,
		.setup   = setup_M2P,
	},
	{
		.name    = "DMAC4 7",
		.defmode = NE1_DMAC_MODE_M2P_32BIT | NE1_DMAC_MODE_REQ_HIL | NE1_DMAC_DCHC_AM_PULSE |
		           NE1_DMAC_DCHC_SEL7,
		.setup   = setup_M2P,
	},
};

ne1_dmal_t exdmac_lch[NE1_DMAC4_MAX_L_CHANNELS] = {
	{
		.name    = "EXDMAC BUS0",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_DCHC_SEL0,
		.setup   = setup_M2M,
	},
	{
		.name    = "EXDMAC 1",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_DCHC_SEL1,
		.setup   = setup_M2M,
	},
	{
		.name    = "EXDMAC 2",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_DCHC_SEL2,
		.setup   = setup_M2M,
	},
	{
		.name    = "EXDMAC 3",
		.defmode = NE1_DMAC_MODE_M2M_32BIT | NE1_DMAC_DCHC_SEL3,
		.setup   = setup_M2M,
	},
};

/* for every physical channels (4ch) */
ne1_dmap_t dma_pch[NE1_DMAC_MAX_P_CHANNELS] = {
	{ /* DMAC0 */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_DMAC_0),
		.lch       = dmac0_lch,
		.lchno     = NE1_DMAC0_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC32_END_0,
		.err_irq   = IRQ_DMAC32_ERR_0,
	},
	{ /* DMAC1 */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_DMAC_1),
		.lch       = dmac1_lch,
		.lchno     = NE1_DMAC1_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC32_END_1,
		.err_irq   = IRQ_DMAC32_ERR_1,
	},
	{ /* DMAC2 */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_DMAC_2),
		.lch       = dmac2_lch,
		.lchno     = NE1_DMAC2_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC32_END_2,
		.err_irq   = IRQ_DMAC32_ERR_2,
	},
	{ /* DMAC3 */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_DMAC_3),
		.lch       = dmac3_lch,
		.lchno     = NE1_DMAC3_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC32_END_3,
		.err_irq   = IRQ_DMAC32_ERR_3,
	},
	{ /* DMAC4 */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_DMAC_4),
		.lch       = dmac4_lch,
		.lchno     = NE1_DMAC4_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC32_END_4,
		.err_irq   = IRQ_DMAC32_ERR_4,
	},
	{ /* EXDMAC */
		.parbase   = (dma_regs_t *)IO_ADDRESS(NE1_BASE_EXDMAC),
		.lch       = exdmac_lch,
		.lchno     = NE1_DMAC_EX_MAX_L_CHANNELS,
		.irq       = IRQ_DMAC_EXBUS_END,
		.err_irq   = IRQ_DMAC_EXBUS_ERR,
	},
};


/*
 * Check channel number
 */
static inline int check_validity_channel(int channel)
{
	unsigned int pchno;
	unsigned int lchno;

	pchno = NE1_DMAC_PCHNO(channel);
	lchno = NE1_DMAC_LCHNO(channel);

	if (
		     ((pchno == 0) && (lchno < NE1_DMAC0_MAX_L_CHANNELS))
		  || ((pchno == 1) && (lchno < NE1_DMAC1_MAX_L_CHANNELS))
		  || ((pchno == 2) && (lchno < NE1_DMAC2_MAX_L_CHANNELS))
		  || ((pchno == 3) && (lchno < NE1_DMAC3_MAX_L_CHANNELS))
		  || ((pchno == 4) && (lchno < NE1_DMAC4_MAX_L_CHANNELS))
		  || ((pchno == 5) && (lchno < NE1_DMAC_EX_MAX_L_CHANNELS))
	    ) {
			return 0;
	}

	return 1;
}

/*
 * Functions for default settings.
 */

static int setup_M2M(dma_regs_t *regs, dma_addr_t src_ptr, u_int size,
		dma_addr_t dst_ptr, u_int32_t mode)
{
	// T.B.D
	return 0;
}

static int setup_M2P(dma_regs_t *regs, dma_addr_t src_ptr, u_int size,
		   dma_addr_t dst_ptr, u_int32_t mode)
{
	unsigned int dsize;

	regs->dsaw = src_ptr;
	regs->ddaw = dst_ptr;

	if (size == 0) {	/* short cut for special transfer mode */
		goto start;
	}

	dsize = mode & NE1_DMAC_DCHC_DSIZE_MASK;
	switch (dsize) {
	case NE1_DMAC_DCHC_DDS16:	size /= 2; break;
	case NE1_DMAC_DCHC_DDS32:	size /= 4; break;
	case NE1_DMAC_DCHC_DDS128:	size /= 16; break;
	case NE1_DMAC_DCHC_DDS256:	size /= 32; break;
	case NE1_DMAC_DCHC_DDS512:	size /= 64; break;
	case NE1_DMAC_DCHC_DDS8:
	default:
		break;
	}

	size--;

	regs->dtcw = size;
	regs->dchc = mode;

start:
	/* Start DMA. */
	regs->dchs = NE1_DMAC_DCHS_FCLR;
	regs->dchs = NE1_DMAC_DCHS_EN_EN | NE1_DMAC_DCHS_EN;

	return 0;
}

static int setup_P2M(dma_regs_t *regs, dma_addr_t src_ptr, u_int size,
		   dma_addr_t dst_ptr, u_int32_t mode)
{
	unsigned int ssize;

	regs->dsaw = src_ptr;
	regs->ddaw = dst_ptr;

	if (size == 0) {	/* short cut for special transfer mode */
		goto start;
	}

	ssize = mode & NE1_DMAC_DCHC_SSIZE_MASK;
	switch (ssize) {
	case NE1_DMAC_DCHC_SDS16:	size /= 2; break;
	case NE1_DMAC_DCHC_SDS32:	size /= 4; break;
	case NE1_DMAC_DCHC_SDS128:	size /= 16; break;
	case NE1_DMAC_DCHC_SDS256:	size /= 32; break;
	case NE1_DMAC_DCHC_SDS512:	size /= 64; break;
	case NE1_DMAC_DCHC_SDS8:
	default:
		break;
	}

	size--;

	regs->dtcw = size;
	regs->dchc = mode;

start:
	/* Start DMA. */
	regs->dchs = NE1_DMAC_DCHS_FCLR;
	regs->dchs = NE1_DMAC_DCHS_EN_EN | NE1_DMAC_DCHS_EN;

	return 0;
}

/*
 * Interrupt Handler
 */
static irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t *regs = dev_id;
	u_int32_t   intstat, endflag = 0;

	intstat = regs->dchs;
	if ((intstat & (NE1_DMAC_DCHS_END | NE1_DMAC_DCHS_ER)) == 0) {
		return IRQ_NONE;
	}

	/* clear interrupt status */
	regs->dchs = NE1_DMAC_DCHS_FCLR;

	switch (irq) {
	case INT_DMAC32_END_0:
		endflag = 1;
	case INT_DMAC32_ERR_0:
		pch = &dma_pch[0];
		break;
	case INT_DMAC32_END_1:
		endflag = 1;
	case INT_DMAC32_ERR_1:
		pch = &dma_pch[1];
		break;
	case INT_DMAC32_END_2:
		endflag = 1;
	case INT_DMAC32_ERR_2:
		pch = &dma_pch[2];
		break;
	case INT_DMAC32_END_3:
		endflag = 1;
	case INT_DMAC32_ERR_3:
		pch = &dma_pch[3];
		break;
	case INT_DMAC32_END_4:
		endflag = 1;
	case INT_DMAC32_ERR_4:
		pch = &dma_pch[4];
		break;
	case INT_DMAC_EXBUS_END:
		endflag = 1;
	case INT_DMAC_EXBUS_ERR:
		pch = &dma_pch[5];
		break;
	default:
		return IRQ_NONE;
	}

	lch = pch->lch + NE1_DMAC_PARBASE2LCHNO(regs);

	/* call client driver's interrupt handling routine */
	if (lch->callback != NULL) {
		if ((lch->int_mask & NE1_DMAC_INT_END) && (endflag == 1)) {
			lch->callback(lch->data, intstat);
		} else if ((lch->int_mask & NE1_DMAC_INT_ERR) && (endflag == 0)) {
			lch->callback(lch->data, intstat);
		}
	}

	return IRQ_HANDLED;
}


/****  External functions  ****/

/*
 * Acquire DMA channel
 *   ne1_request_dma()
 */
int ne1_request_dma(int channel, const char *device_id,
				 dma_callback_t callback, void *data, dma_regs_t **dma_regs)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t *regs;
	int         err;

	if (check_validity_channel(channel) != 0) {
		return -ENODEV;
	}

	pch = &dma_pch[NE1_DMAC_PCHNO(channel)];
	if (pch->lch == NULL) {
		return -ENODEV;
	}
	lch = pch->lch + NE1_DMAC_LCHNO(channel);
	if (lch->in_use != DMA_NO_USE) {
		return -EBUSY;
	}
	regs = pch->parbase + NE1_DMAC_LCHNO(channel);

	err = request_irq(pch->irq, dma_irq_handler, (IRQF_SHARED | IRQF_DISABLED), device_id, regs);
	if (err != 0) {
		printk(KERN_INFO "%s(): unable to request IRQ %d for DMA channel (%s)\n",
			__FUNCTION__, pch->irq, device_id);
		return err;
	}
	err = request_irq(pch->err_irq, dma_irq_handler, (IRQF_SHARED | IRQF_DISABLED), device_id, regs);
	if (err != 0) {
		printk(KERN_INFO "%s(): unable to request IRQ %d for DMA channel (%s)\n",
			__FUNCTION__, pch->err_irq, device_id);
		return err;
	}

	/* Hold information for further DMAC operations. */
	lch->in_use = DMA_USE;
	lch->callback = callback;
	lch->data = data;

	/* return pointer to control registers. */
	if (dma_regs != NULL) {
		*dma_regs = regs;
	}

	return 0;
}

/*
 * Release DMA channel
 */
void ne1_free_dma(int channel)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t *regs;

	if (check_validity_channel(channel) != 0) {
		return;
	}

	pch = &dma_pch[NE1_DMAC_PCHNO(channel)];
	if (pch->lch == NULL) {
		return;
	}
	lch = pch->lch + NE1_DMAC_LCHNO(channel);
	if (lch->in_use != DMA_USE) {
		return;
	}

	/* Stop DMA */
	ne1_clear_dma(channel);

	/* Release interrupt */
	regs = pch->parbase + NE1_DMAC_LCHNO(channel);
	free_irq(pch->irq, regs);
	free_irq(pch->err_irq, regs);

	lch->in_use = DMA_NO_USE;
	lch->callback = NULL;
	lch->data = NULL;
}

/*
 * Setup and enable DMA.
 */
int ne1_start_dma(int channel, dma_addr_t src_ptr, u_int size,
			   dma_addr_t dst_ptr, int intmask)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t   *regs;
	int           err;

	if (check_validity_channel(channel) != 0) {
		return -ENODEV;
	}
	if ((intmask & ~NE1_DMAC_MASK_INT) != 0) {
		return -EINVAL;
	}

	pch = &dma_pch[NE1_DMAC_PCHNO(channel)];
	if (pch->lch == NULL) {
		return -ENODEV;
	}
	lch = pch->lch + NE1_DMAC_LCHNO(channel);
	if (lch->in_use != DMA_USE) {
		return -ENXIO;
	}
	if ((intmask != 0) && (lch->callback == NULL)) {
		return -EINVAL;
	}

	lch->int_mask = intmask;

	/* Set DMA channel up to appropriate default setting. */
	regs = pch->parbase + NE1_DMAC_LCHNO(channel);
	err = lch->setup(regs, src_ptr, size, dst_ptr, lch->defmode);
	if (err != 0) {
		return -EINVAL;
	}

	return 0;
}

/*
 * Acquire DMA channel status.
 */
unsigned int ne1_dma_status(int channel)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t *regs;
	int         stat;

	if (check_validity_channel(channel) != 0) {
		return -ENODEV;
	}

	pch = &dma_pch[NE1_DMAC_PCHNO(channel)];
	if (pch->lch == NULL) {
		return -ENODEV;
	}
	lch = pch->lch + NE1_DMAC_LCHNO(channel);
	regs = pch->parbase + NE1_DMAC_LCHNO(channel);

	stat = regs->dchs;

	return stat;
}

/*
 * Acquire all DMA channel status.
 * check all DMA channels,
 * return 1 if there is enabled channel.
 * Don't start DMA channel while calling this function.
 */
int ne1_dma_busy(void)
{
	int p;
	int l;
	ne1_dmal_t *lch;
	dma_regs_t *regs;

	for (p = 0; p < NE1_DMAC_MAX_P_CHANNELS; p++) {
		for (l = 0; l < dma_pch[p].lchno; l++) {
			lch = dma_pch[p].lch + l;
			if (lch->in_use == DMA_USE) {
				regs = dma_pch[p].parbase + l;
				if ((regs->dchs & NE1_DMAC_DCHS_ACT) != 0) {
					return 1;
				}
		   }
		}
	}

	return 0;
}

/*
 * Stop DMA channel.
 */
void ne1_clear_dma(int channel)
{
	ne1_dmap_t *pch;
	ne1_dmal_t *lch;
	dma_regs_t *regs;

	if (check_validity_channel(channel) != 0) {
		return;
	}

	pch = &dma_pch[NE1_DMAC_PCHNO(channel)];
	if (pch->lch == NULL) {
		return;
	}
	lch = pch->lch + NE1_DMAC_LCHNO(channel);
	if (lch->in_use != DMA_USE) {
		return;
	}
	regs = pch->parbase + NE1_DMAC_LCHNO(channel);

	while (regs->dchs & (NE1_DMAC_DCHS_RQST | NE1_DMAC_DCHS_ACT))

	/* stop DMA */
	regs->dchs = NE1_DMAC_DCHS_EN_EN;
	regs->dchs = NE1_DMAC_DCHS_FCLR;
}

/*
 * Stop DMA.
 */
void ne1_stop_dma(int channel)
{
	ne1_clear_dma(channel);
}

static int dma_suspend(struct platform_device *dev, pm_message_t state)
{
	switch (state.event) {
	case PM_EVENT_SUSPEND:
		if (ne1_dma_busy())
			return -EBUSY;
		break;
	default:
		break;
	}
	return 0;
}

static int dma_resume(struct platform_device *dev)
{
	return 0;
}

static int dma_probe(struct platform_device *dev)
{
	int p;
	int l;
	ne1_dmal_t *lch;

	for (p = 0; p < NE1_DMAC_MAX_P_CHANNELS; p++) {
		for (l = 0; l < dma_pch[p].lchno; l++) {
			lch = dma_pch[p].lch + l;
			lch->callback = NULL;
			lch->data     = NULL;
			lch->in_use   = DMA_NO_USE;
		}
	}

	return 0;
}

static struct platform_driver dma_driver = {
	.probe   = dma_probe,
	.suspend = dma_suspend,
	.resume  = dma_resume,
	.driver  = {
		.name  = "dma",
		.owner = THIS_MODULE,
	},
};


/*
 * Initialize
 */
static int __init ne1_dma_init(void)
{
	return platform_driver_register(&dma_driver);
}

arch_initcall(ne1_dma_init);

EXPORT_SYMBOL(ne1_request_dma);
EXPORT_SYMBOL(ne1_free_dma);
EXPORT_SYMBOL(ne1_start_dma);
EXPORT_SYMBOL(ne1_dma_status);
EXPORT_SYMBOL(ne1_dma_busy);
EXPORT_SYMBOL(ne1_clear_dma);
EXPORT_SYMBOL(ne1_stop_dma);
