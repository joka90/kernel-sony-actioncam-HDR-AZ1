/*
 * arch/arm/mach-cxd90014/irqs.h
 *
 * IRQ definitions
 *
 * Copyright 2012,2013 Sony Corporation
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

#ifndef __ARCH_CXD90014_IRQS_H
#define __ARCH_CXD90014_IRQS_H

#define IRQ_IPI				1
#define IRQ_LOCALTIMER			29
#define IRQ_LOCALWDOG			30
#define IRQ_GIC_START			32

#define IRQ_AVB				34
#define IRQ_GPE				76
#define IRQ_MXX				87

#define IRQ_GPIO_BASE			121
#define IRQ_GPIO(x)			(IRQ_GPIO_BASE + (x))
#define NR_GPIO_S_IRQ			16
#define IRQ_GPIO_SB_BASE		137
#define NR_GPIO_SB_IRQ			7
#define IRQ_GPIO_S_16			144

#define IRQ_DFS				145

#define IRQ_PMU_BASE			146
#define IRQ_PMU(x)			(IRQ_PMU_BASE + (x))
#define IRQ_PMU_CPU0			IRQ_PMU(0)
#define IRQ_PMU_CPU1			IRQ_PMU(1)
#define IRQ_PMU_CPU2			IRQ_PMU(2)
#define IRQ_PMU_CPU3			IRQ_PMU(3)

#define IRQ_UART_BASE			150
#define IRQ_UART(x)			(IRQ_UART_BASE + (x))

#define IRQ_TIMER_BASE			153
#define IRQ_TIMER(x)			(IRQ_TIMER_BASE + (x))
#define IRQ_TIMERWDOG			IRQ_TIMER(16)

#define IRQ_BOSS			170
#define IRQ_LDEC			171
#define IRQ_NANDC			172

#define IRQ_DMAC0_BASE			173
#define IRQ_DMAC0(x)			(IRQ_DMAC0_BASE + (x))
#define IRQ_DMAC1_BASE			181
#define IRQ_DMAC1(x)			(IRQ_DMAC1_BASE + (x))
#define IRQ_XDMAC0_BASE			189
#define IRQ_XDMAC0(x)			(IRQ_XDMAC0_BASE + (x))
#define IRQ_XDMAC1_BASE			193
#define IRQ_XDMAC1(x)			(IRQ_XDMAC1_BASE + (x))

#define IRQ_SIRCS_BASE			197
#define IRQ_SIRCS_OVR			(IRQ_SIRCS_BASE + 0)
#define IRQ_SIRCS_RX			(IRQ_SIRCS_BASE + 1)

#define IRQ_I2C				199

#define IRQ_SIO_BASE			201
#define IRQ_SIO(x)			(IRQ_SIO_BASE + (x))

#define IRQ_MS0_BASE			206
#define IRQ_MS0				(IRQ_MS0_BASE + 0)
#define IRQ_MS0_INS_RISE		(IRQ_MS0_BASE + 1)
#define IRQ_MS0_INS_FALL		(IRQ_MS0_BASE + 2)

#define IRQ_TPU_BASE			209
#define IRQ_TPU(x)			(IRQ_TPU_BASE + (x))

#define IRQ_ADC_BASE			215
#define IRQ_ADC(x)			(IRQ_ADC_BASE + (x))

#define IRQ_HDMI			217

#define IRQ_SDIF_BASE			218
#define IRQ_SDIF(x)			(IRQ_SDIF_BASE + (x)*3)

#define IRQ_USB_BASE			227
#define IRQ_USB_CINTR			(IRQ_USB_BASE + 0)
#define IRQ_USB_DIRQ0			(IRQ_USB_BASE + 1)
#define IRQ_USB_DIRQ1			(IRQ_USB_BASE + 2)
#define IRQ_USB_EHCI			(IRQ_USB_BASE + 3)
#define IRQ_USB_OHCI			(IRQ_USB_BASE + 4)
#define IRQ_USB_VBUS			(IRQ_USB_BASE + 5)
#define IRQ_USB_ID			(IRQ_USB_BASE + 6)

#define IRQ_PCIE_BASE			235
#define IRQ_PCIE_EXPCAP			(IRQ_PCIE_BASE + 0)
#define IRQ_PCIE_ERR			(IRQ_PCIE_BASE + 1)
#define IRQ_PCIE_DMA			(IRQ_PCIE_BASE + 2)
#define IRQ_PCIE_AXIERR			(IRQ_PCIE_BASE + 3)
#define IRQ_PCIE_TRS			(IRQ_PCIE_BASE + 4)
#define IRQ_PCIE_INTA			(IRQ_PCIE_BASE + 5)
#define IRQ_PCIE_INTB			(IRQ_PCIE_BASE + 6)
#define IRQ_PCIE_INTC			(IRQ_PCIE_BASE + 7)
#define IRQ_PCIE_INTD			(IRQ_PCIE_BASE + 8)
#define IRQ_PCIE_PMETOFF		(IRQ_PCIE_BASE + 9)
#define IRQ_PCIE_PMERQ			(IRQ_PCIE_BASE + 10)
#define IRQ_PCIE_MSIABORT		(IRQ_PCIE_BASE + 11)
#define IRQ_PCIE_OTHER			(IRQ_PCIE_BASE + 12)

#define IRQ_WDOG			254

#define IRQ_L2				255

#define NR_IRQS				256

#ifndef __ASSEMBLY__
extern void raise_irq(unsigned int cpu, unsigned int irq);
extern int gpiopin_to_irq(unsigned int port, unsigned int bit);
#endif /* !__ASSEMBLY__ */
#endif /* __ARCH_CXD90014_IRQS_H */
