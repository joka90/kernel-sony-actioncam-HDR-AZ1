/* 2008-09-01: File added by Sony Corporation */
/*
 *  File Name	    : linux/arch/arm/mach-ne1/dma.h
 *  Function	    : dmac
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

#ifndef __ARCH_ARM_MACH_NE1_DMA_H
#define __ARCH_ARM_MACH_NE1_DMA_H


/****  Macro definition  ****/
#define NE1_DMAC_MASK_INT	(NE1_DMAC_INT_ERR | NE1_DMAC_INT_END)

#define NE1_DMAC_PCHNO(x)			(((x) & 0xf0) >> 4)
#define NE1_DMAC_LCHNO(x)			((x) & 0x0f)
#define NE1_DMAC_PARBASE2LCHNO(x)	(((long)(x) & 0xe0) >> 5)


//#define NE1_DMAC_PARBASE2CHNO(x) ((((long)((x & 0xf000) - 0xc000)) >> 8) | ((x & 0xe0) >> 5)))


/****  Structure definition  ****/

/* for `in_use' setting */
#define DMA_NO_USE	0
#define DMA_USE 	1

/*
 * Data for managing logical DMA channels
 */
typedef struct {
	const char		*name;		/* irq name of L-channel. */
	dma_callback_t	callback;	/* call-back function from interrupt. */
	void			*data;		/* argument of call-back function. */
	u_int32_t		defmode;	/* default transfer mode. */
	u_int8_t		in_use; 	/* DMA in use flag. */
	int				(*setup)(dma_regs_t *, dma_addr_t, u_int, dma_addr_t, u_int32_t);
	u_int8_t		int_mask; 	/* interrupt mask flag. */
} ne1_dmal_t;

/*
 * Data for managing physical DMA channels
 */
typedef struct {
	u_int32_t	control;	/* control/status regs. */
	u_int32_t	intstat;	/* interrupt control regs. */
	dma_regs_t	*parbase;	/* base of parameter registers array. */
	ne1_dmal_t	*lch;		/* control structures for logical CH. */
    u_int32_t	lchno;		/* No. of logical channel. */
	int irq;		/* Number of END interrupt */
	int err_irq;	/* Number of ERR interrupt */
} ne1_dmap_t;

#endif /* __ARCH_ARM_MACH_NE1_DMA_H */
