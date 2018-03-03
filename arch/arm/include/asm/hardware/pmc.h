#ifndef PMC_H
#define PMC_H

/*
 * arch/arm/include/asm/hardware/pmc.h
 *
 * MPCore Performance Monitor Register Access
 *
 * Copyright 2005,2006,2007,2008,2009,2010,2011,2012 Sony Corporation
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
#ifndef __ASM_HARDWARE_PMC_H__
#define __ASM_HARDWARE_PMC_H__

#define CP15_C12	((unsigned int)12)
#define CP15_C13	((unsigned int)13)
#define CP15_C14	((unsigned int)14)
#define CPREG_SHIFT	16

/*** cp15, 0, c9, c12 ***/
#define PMCR		(0 | (CP15_C12<<CPREG_SHIFT)) /* Control Register */
# define PMNC_E		(1 << 0)
# define PMNC_PMNX_CLR	(1 << 1)
# define PMNC_CCNT_CLR	(1 << 2)
# define __PMC_RESTART	(PMNC_E | PMNC_PMNX_CLR | PMNC_CCNT_CLR)

#define PMCNTENSET	(1 | (CP15_C12<<CPREG_SHIFT)) /* Count Enable Set Register */
#define PMCNTENCLR	(2 | (CP15_C12<<CPREG_SHIFT)) /* Count Enable Clear Register */
#define PMOVSR		(3 | (CP15_C12<<CPREG_SHIFT)) /* Overflow Flag Status Register */
#define PMSWINC		(4 | (CP15_C12<<CPREG_SHIFT)) /* Software Increment Register. WO */
#define PMSELR		(5 | (CP15_C12<<CPREG_SHIFT))
# define PMSEL_PMN0	(0x00)
# define PMSEL_PMN1	(0x01)
# define PMSEL_CCF	(0x1f) /* Select "Cycle Count Filter Control Register" */

/*** cp15, 0, c9, c13 ***/
#define PMCCNTR		(0 | (CP15_C13<<CPREG_SHIFT)) /* Cycle Count Register */
#define PMXEVTYPER	(1 | (CP15_C13<<CPREG_SHIFT)) /* Event Type Register (Use this register with PMSELR) */
#define PMXEVCNTR	(2 | (CP15_C13<<CPREG_SHIFT)) /* PM Counter Register */

/*** cp15, 0, c9, c14 ***/
#define PMUSERENR	(0 | (CP15_C14<<CPREG_SHIFT)) /* User Enable Register */
#define PMINTENSET	(1 | (CP15_C14<<CPREG_SHIFT)) /* Interrupt Enable Set Register */
#define PMINTENCLR	(2 | (CP15_C14<<CPREG_SHIFT)) /* Interrupt Enable Clear Register */

# define PMNC_PMN0	(1 << 0)
# define PMNC_PMN1	(1 << 1)
# define PMNC_CCNT	(1 << 31)

/* Not implemented */
#define PMCEID0 /* Common Event Identification Register 0. RO */
#define PMCEID1 /* Common Event Identification Register 1. RO */
#define PMCCFILTR /* Cycle Count Filter Control Register */
#define PMCFGR /* Configuration Register */
#define PMLAR /* Lock Access Register */
#define PMLSR /* Lock Status Register */
#define PMAUTHSTATUS /* Authentication Status Register */
#define PMDEVTYPE /* Device Type Register */
#define PMPERIPHERALID /*Identification Registers */
#define PMCOMPONENTID /* Identification Registers */

static inline unsigned int rdpmc(unsigned int id)
{
	unsigned int reg, val = 0;

	reg = (id >> CPREG_SHIFT) & 0x00ff;
	id  = id & 0x00ff;
	if (reg == CP15_C12)
		asm volatile ("mrc p15,0,%0,c9,c12,%1":"=r"(val):"i"(id):"cc");
	else if (reg == CP15_C13)
		asm volatile ("mrc p15,0,%0,c9,c13,%1":"=r"(val):"i"(id):"cc");
	else if (reg == CP15_C14)
		asm volatile ("mrc p15,0,%0,c9,c14,%1":"=r"(val):"i"(id):"cc");
	else ;

	return val;
}

static inline void wrpmc(unsigned int id, unsigned int val)
{
	unsigned int reg;

	reg = (id >> CPREG_SHIFT) & 0x00ff;
	id  = id & 0x00ff;
	if (reg == CP15_C12)
		asm volatile ("mcr p15,0,%0,c9,c12,%1"::"r"(val),"i"(id):"cc");
	else if (reg == CP15_C13)
		asm volatile ("mcr p15,0,%0,c9,c13,%1"::"r"(val),"i"(id):"cc");
	else if (reg == CP15_C14)
		asm volatile ("mcr p15,0,%0,c9,c14,%1"::"r"(val),"i"(id):"cc");
	else ;
}

#ifdef CONFIG_OPROFILE
static inline void pmc_restart(unsigned char ev1, unsigned char ev2)
{
}
#else
static inline void pmc_restart(unsigned char ev1, unsigned char ev2)
{
	register unsigned int pmnc;

	wrpmc(PMSELR, PMSEL_PMN0);
	wrpmc(PMXEVTYPER, ev1);

	wrpmc(PMSELR, PMSEL_PMN1);
	wrpmc(PMXEVTYPER, ev2);

	pmnc = __PMC_RESTART;
	asm volatile("mcr p15,0,%0,c9,c12,0" ::"r"(pmnc):"cc");
}
#endif /* CONFIG_OPROFILE */
#endif /* __ASM_HARDWARE_PMC_H__ */


#endif
