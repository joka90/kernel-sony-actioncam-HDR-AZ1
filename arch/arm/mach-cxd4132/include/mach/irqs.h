/*
 * include/asm-arm/arch-cxd4132/irqs.h
 *
 * IRQ definitions
 *
 * Copyright 2009,2010 Sony Corporation
 *
 * This code is based on include/asm-arm/arch-realview/irqs.h.
 */
/*
 *  linux/include/asm-arm/arch-realview/irqs.h
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ARCH_CXD4132_IRQS_H
#define __ARCH_CXD4132_IRQS_H

#define IRQ_IPI				1
#define IRQ_LOCALTIMER			29
#define IRQ_LOCALWDOG			30
#define IRQ_GIC_START			32

#define IRQ_AVB				35
#define IRQ_GPE				64

#define IRQ_GPIO_RISE_BASE		96
#define IRQ_GPIO_RISE(x)		(IRQ_GPIO_RISE_BASE + (x))

#define IRQ_GPIO_FALL_BASE		120
#define IRQ_GPIO_FALL(x)		(IRQ_GPIO_FALL_BASE + (x))

#define IRQ_SHM				IRQ_GPIO_RISE(20)
#define IRQ_IPC				IRQ_GPIO_RISE(15)

#define IRQ_DFS				144

#define IRQ_PMU_BASE			148
#define IRQ_PMU(x)			(IRQ_PMU_BASE + (x))
#define IRQ_PMU_CPU0			IRQ_PMU(0)
#define IRQ_PMU_CPU1			IRQ_PMU(1)
#define IRQ_PMU_CPU2			IRQ_PMU(2)
#define IRQ_PMU_CPU3			IRQ_PMU(3)
#define IRQ_PMU_SCU0			IRQ_PMU(4)
#define IRQ_PMU_SCU1			IRQ_PMU(5)
#define IRQ_PMU_SCU2			IRQ_PMU(6)
#define IRQ_PMU_SCU3			IRQ_PMU(7)
#define IRQ_PMU_SCU4			IRQ_PMU(8)
#define IRQ_PMU_SCU5			IRQ_PMU(9)
#define IRQ_PMU_SCU6			IRQ_PMU(10)
#define IRQ_PMU_SCU7			IRQ_PMU(11)

#define IRQ_UART_BASE			160
#define IRQ_UART(x)			(IRQ_UART_BASE + (x))

#define IRQ_TIMER_BASE			163
#define IRQ_TIMER(x)			(IRQ_TIMER_BASE + (x))
#define IRQ_TIMERWDOG			IRQ_TIMER(12)

#define IRQ_FSP_BASE			176
#define IRQ_FSP_DMAC(x)			(IRQ_FSP_BASE + (x))
#define IRQ_FSP_MENO			(IRQ_FSP_BASE + 4)
#define IRQ_FSP_LDEC			(IRQ_FSP_BASE + 5)
#define IRQ_FSP_SB			(IRQ_FSP_BASE + 6)
#define IRQ_FSP_XINT			(IRQ_FSP_BASE + 7)

#define IRQ_DMAC0_BASE			184
#define IRQ_DMAC0(x)			(IRQ_DMAC0_BASE + (x))

#define IRQ_SIRCS_BASE			192
#define IRQ_SIRCS_OVR			(IRQ_SIRCS_BASE + 0)
#define IRQ_SIRCS_RX			(IRQ_SIRCS_BASE + 2)

#define IRQ_I2C				195

#define IRQ_SIO_BASE			196
#define IRQ_SIO(x)			(IRQ_SIO_BASE + (x))

#define IRQ_MS0				201
#define IRQ_MS0_INS_RISE		202
#define IRQ_MS0_INS_FALL		203

#define IRQ_TPU_BASE			204
#define IRQ_TPU(x)			(IRQ_TPU_BASE + (x))

#define IRQ_ADC_BASE			210
#define IRQ_ADC(x)			(IRQ_ADC_BASE + (x))

#define IRQ_MXX				213

#define IRQ_SDIF_BASE			214
#define IRQ_SDIF(x)			(IRQ_SDIF_BASE + (x)*2)
#define IRQ_SDIF_INT(x)			(IRQ_SDIF(x) + 0)
#define IRQ_SDIF_WAKE(x)		(IRQ_SDIF(x) + 1)

#define IRQ_USB_BASE			222
#define IRQ_USB				(IRQ_USB_BASE + 0)
#define IRQ_USB_ID_RISE			(IRQ_USB_BASE + 1)
#define IRQ_USB_ID_FALL			(IRQ_USB_BASE + 2)
#define IRQ_USB_BVALID_RISE		(IRQ_USB_BASE + 3)
#define IRQ_USB_BVALID_FALL		(IRQ_USB_BASE + 4)
#define IRQ_USB_OVR_CURR		(IRQ_USB_BASE + 5)

#define IRQ_SATA			230
#define IRQ_SATA_PORT0			231
#define IRQ_SATA_CCC			232

#define IRQ_WDOG			234

#define IRQ_L2				235

#define NR_IRQS				256

#ifndef __ASSEMBLY__
extern void raise_irq(unsigned int cpu, unsigned int irq);
#endif /* !__ASSEMBLY__ */
#endif /* __ARCH_CXD4132_IRQS_H */
