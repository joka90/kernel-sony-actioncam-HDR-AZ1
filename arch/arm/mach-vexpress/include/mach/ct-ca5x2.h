/* 2011-10-25: File added by Sony Corporation */
#ifndef __MACH_CT_CA5X2_H
#define __MACH_CT_CA5X2_H

/*
 * Physical base addresses
 */
#define CT_CA5X2_CLCDC		(0x10020000)
#define CT_CA5X2_DMC		(0x100e0000)
#define CT_CA5X2_SMC		(0x100e1000)
#define CT_CA5X2_SCC		(0x100e2000)
#define CT_CA5X2_CORESIGHT	(0x10200000)
#define CT_CA5X2_MPIC		(0x1e000000)
#define CT_CA5X2_SYSTIMER	(0x1e004000)
#define CT_CA5X2_SYSWDT		(0x1e007000)
#define CT_CA5X2_L2CC		(0x1e00a000)

#define A5_MPCORE_SCU		(CT_CA5X2_MPIC + 0x0000)
#define A5_MPCORE_GIC_CPU	(CT_CA5X2_MPIC + 0x0100)
#define A5_MPCORE_GIT		(CT_CA5X2_MPIC + 0x0200)
#define A5_MPCORE_TWD		(CT_CA5X2_MPIC + 0x0600)
#define A5_MPCORE_GIC_DIST	(CT_CA5X2_MPIC + 0x1000)

/*
 * Interrupts.  Those in {} are for AMBA devices
 */
#define IRQ_CT_CA5X2_CLCDC	{ 76 }
#define IRQ_CT_CA5X2_DMC	{ -1 }
#define IRQ_CT_CA5X2_SMC	{ 77, 78 }
#define IRQ_CT_CA5X2_TIMER0	80	/* Tied low. Timer0 not implemented */
#define IRQ_CT_CA5X2_TIMER1	81	/* Tied low. Timer1 not implemented */
#define IRQ_CT_CA5X2_GPIO	{ 82 }	/* Tied low. GPIO not implemented */
#define IRQ_CT_CA5X2_PMU_CPU0	92
#define IRQ_CT_CA5X2_PMU_CPU1	93
#define IRQ_CT_CA5X2_PMU_CPU2	94	/* PMUIRQ[2] (tied low) */
#define IRQ_CT_CA5X2_PMU_CPU3	95	/* PMUIRQ[3] (tied low) */

extern struct ct_desc ct_ca5x2_desc;

#endif
