/*
 * arch/arm/mach-cxd90014/pcie_ctl.c
 *
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
 * Copyright 2013 Sony Corporation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/snsc_boot_time.h>
#include <linux/pci.h>
#include "pcie.h"
#include "pcie_ctl.h"
#include "reg_PCIE.h"
#include "reg_PCIE_DMXHQ.h"
#include "reg_WPCIE.h"
#include "pci_platform.h"

static pcie_remap_reg_t     pcie_reg;

static irqreturn_t  pcie_err_interrupt      (int irq, void *dev_id);
static irqreturn_t  pcie_axi_interrupt      (int irq, void *dev_id);
static void         pcie_ctl_get_reg_base   ( pcie_remap_reg_t *pcie_reg );
static void         PCIeRC_Initialize       ( pcie_remap_reg_t *pcie_reg );
static void         PCIeEP_Initialize       ( pcie_remap_reg_t *pcie_reg );
static void         PCIe_RC_Configuration   ( pcie_remap_reg_t *pcie_reg );
static void         pcie_set_bit_w          ( uint16_t data, volatile void __iomem *addr );
static void         pcie_set_bit_l          ( uint32_t data, volatile void __iomem *addr );
static void         pcie_clr_bit_l          ( uint32_t data, volatile void __iomem *addr );
static void pcie_rc_setLimitLinkSpeed(reg_pcie_rcxh_t __iomem *reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed);
static void pcie_rc_setLinkSpeed(reg_pcie_rcxh_t __iomem *reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed);
static unsigned int pcie_rc_getSpeedChangeStatus(reg_pcie_rcxh_t __iomem *reg);
static unsigned int pcie_rc_getCurrentLinskSpeed(reg_pcie_rcxh_t __iomem *reg);
static void pcie_rc_changeLinkSpeed(pcie_remap_reg_t *pcie_reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed);

#ifdef PCIE_ERRATA_5
#define PCIE_DUMMY_READ readl_relaxed(pcie_reg.pcie_dmxhq_base)
#else /* PCIE_ERRATA_5 */
#define PCIE_DUMMY_READ do {} while (0)
#endif /* PCIE_ERRATA_5 */

void pcie_raw_writel(u32 val, volatile void __iomem *addr)
{
	writel_relaxed(val, addr);
}

void pcie_raw_writew(u16 val, volatile void __iomem *addr)
{
	writew_relaxed(val, addr);
}

void pcie_raw_writeb(u8 val, volatile void __iomem *addr)
{
	writeb_relaxed(val, addr);
}

u32 pcie_read(volatile void __iomem *addr)
{
	return readl_relaxed(addr);
}

void pcie_writel(u32 val, volatile void __iomem *addr)
{
	writel_relaxed(val, addr);
	PCIE_DUMMY_READ;
}

void pcie_writew(u16 val, volatile void __iomem *addr)
{
	writew_relaxed(val, addr);
	PCIE_DUMMY_READ;
}

void pcie_writeb(u8 val, volatile void __iomem *addr)
{
	writeb_relaxed(val, addr);
	PCIE_DUMMY_READ;
}

/* get config reg. base address */
volatile void __iomem *pcie_ctl_root_config(int slot)
{
	if (!slot) { /* DMXHQ Root Complex */
		return pcie_reg.pcie_base;
	}
	return NULL;
}

void pcie_ctl_cfg_io_set(uint8_t bus_num, uint8_t dev_num, uint8_t func_num )
{

	reg_pcie_dmxhq_t __iomem *dmxhq_reg;
	uint32_t wdata  = 0;

	dmxhq_reg = (reg_pcie_dmxhq_t __iomem *)( pcie_reg.pcie_dmxhq_base );
	pcie_assert_NULL( dmxhq_reg );


	/* XF00_0200h  - F00_023Ch  PCIe Destinasion Configration ID CFGn Register */
	wdata           |= ((bus_num << PCIE_BUS_BOUNDARY) | (dev_num << PCIE_DEV_BOUNDARY) | (func_num << PCIE_FUNC_BOUNDARY));
	pcie_writel( wdata , &dmxhq_reg->CTIDR[0].DATA );

	return;
}

int pcie_ctl_cfg_read(volatile void __iomem *addr, u32 *val)
{
	volatile void __iomem *dmxhq;
	u32 stat;

	dmxhq = (volatile void __iomem *)pcie_reg.pcie_dmxhq_base;
	pcie_assert_NULL(dmxhq);

	/* clear Unsupported Request Completion status */
	pcie_writel(PCIE_DMXHQ_INTSTAT_UNSUP, dmxhq + PCIE_DMXHQ_INTSTAT);

	*val =pcie_read(addr);

	/* Unsupported Request Completion ? */
	stat = pcie_read(dmxhq + PCIE_DMXHQ_INTSTAT);
	if (stat & PCIE_DMXHQ_INTSTAT_UNSUP) {
		/* clear */
		pcie_writel(PCIE_DMXHQ_INTSTAT_UNSUP, dmxhq + PCIE_DMXHQ_INTSTAT);
		return -1;
	}
	return 0;
}

static void pcie_ctl_setup(uint32_t root_busnr)
{
	reg_pcie_dmxhq_t __iomem *dmxhq_reg;

	dmxhq_reg = (reg_pcie_dmxhq_t __iomem *)( pcie_reg.pcie_dmxhq_base );
	pcie_assert_NULL( dmxhq_reg );

	pcie_writel( root_busnr , &dmxhq_reg->SBNCA.DATA );

	return;
}

void pcie_ctl_cfg_root_hook(int slot, int where, int size, u32 val)
{
	if (slot)
		return;

	if (PCI_PRIMARY_BUS == where  &&  PCIE_LONG_ACCESS == size) {
		pcie_ctl_setup((val >> 8) & 0xff);
	}
}

#ifdef CONFIG_PM
void pcie_ctl_suspend(uint32_t pcie_select)
{
	volatile __iomem reg_pcie_dmxhq_t   *dmxhq_reg;

	dmxhq_reg = (volatile __iomem reg_pcie_dmxhq_t *)pcie_reg.pcie_dmxhq_base;
	pcie_assert_NULL(dmxhq_reg);
}

void pcie_ctl_resume(uint32_t pcie_select)
{
	volatile __iomem reg_pcie_dmxhq_t *dmxhq_reg;

	BOOT_TIME_ADD1("B pcie_ctl_resume");
	dmxhq_reg = (volatile __iomem reg_pcie_dmxhq_t *)pcie_reg.pcie_dmxhq_base;
	pcie_assert_NULL(dmxhq_reg);

	if (PCIE_MODE_ROOT == pcie_select) {
		PCIE_PRINTK_DBG(KERN_INFO " PCIe_Initialize start... \n");
		PCIeRC_Initialize(&pcie_reg);
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_config_Config start... \n");
		PCIe_RC_Configuration(&pcie_reg);
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_config_Config finish... \n");

		pcie_rc_changeLinkSpeed(&pcie_reg, PCIE_EPRC_EN_TSSR_TSCRR_RCS_5_0GT);
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_change_link_speed finish... \n");

		pcie_writel((uint32_t)PCIE_MEM_BASE_LOWER, &dmxhq_reg->MTAOL);
		pcie_writel((uint32_t)PCIE_MEM_BASE_UPPER, &dmxhq_reg->MTAOH);
		pcie_writel((uint32_t)PCIE_SET_DMXHQ_TLPHR, &dmxhq_reg->TLPHR.DATA);
		pcie_writel((uint32_t)PCIE_MEM_IO, &dmxhq_reg->IOTAO);
	} else {
		PCIE_PRINTK_DBG(KERN_INFO " PCIe_Initialize start... \n");
		PCIeEP_Initialize( &pcie_reg );

		pcie_writel((uint32_t)PCIE_MEM_BASE_LOWER_EP, &dmxhq_reg->MTAOL);
		pcie_writel((uint32_t)PCIE_MEM_BASE_UPPER_EP, &dmxhq_reg->MTAOH);
		pcie_writel((uint32_t)PCIE_SET_DMXHQ_TLPHR, &dmxhq_reg->TLPHR.DATA);
		pcie_writel((uint32_t)PCIE_MEM_IO, &dmxhq_reg->IOTAO);
	}
	BOOT_TIME_ADD1("E pcie_ctl_resume");
}
#endif /* CONFIG_PM */

void pcie_ctl_start(uint32_t pcie_select)
{

	reg_pcie_dmxhq_t __iomem *dmxhq_reg;
	int                 err;

	BOOT_TIME_ADD1("B pcie_ctl_start");
	pcie_ctl_get_reg_base( &pcie_reg );

	dmxhq_reg = (reg_pcie_dmxhq_t __iomem *)( pcie_reg.pcie_dmxhq_base );
	pcie_assert_NULL( dmxhq_reg );


	if( pcie_select == PCIE_MODE_ROOT ) {
	    PCIE_PRINTK_DBG(KERN_INFO " PCIe_Initialize start... \n");
		PCIeRC_Initialize( &pcie_reg );
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_config_Config start... \n");
		PCIe_RC_Configuration( &pcie_reg );
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_config_Config finish... \n");

		pcie_rc_changeLinkSpeed( &pcie_reg, PCIE_EPRC_EN_TSSR_TSCRR_RCS_5_0GT );
		PCIE_PRINTK_DBG(KERN_INFO " Pcie_change_link_speed finish... \n");

		err = request_irq(PCIE_ST_ERR_COM_O, pcie_err_interrupt, IRQF_DISABLED,
				  "PCIe_COM_ERR", NULL);
		if (err) {
			PCIE_PRINTK_ERR(KERN_ERR "Failed to request interrupt IRQ: %d\n", PCIE_ST_ERR_COM_O );
			/* request_irq error */
			PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
			BUG();
		}

	} else {
		PCIE_PRINTK_DBG(KERN_INFO " PCIe_Initialize start... \n");
		PCIeEP_Initialize( &pcie_reg );
	}

	err = request_irq(PCIE_ST_ERR_AXI_O, pcie_axi_interrupt, IRQF_DISABLED,
			  "PCIe_AXI_ERR", NULL);
	if (err) {
		PCIE_PRINTK_ERR(KERN_ERR "Failed to request interrupt IRQ: %d\n", PCIE_ST_ERR_AXI_O );
		/* request_irq error */
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}

	if (pcie_select == PCIE_MODE_ROOT){
		pcie_writel( (uint32_t)(PCIE_MEM_BASE_LOWER), &dmxhq_reg->MTAOL );
		pcie_writel( (uint32_t)(PCIE_MEM_BASE_UPPER), &dmxhq_reg->MTAOH );
		pcie_writel( (uint32_t)(PCIE_SET_DMXHQ_TLPHR), &dmxhq_reg->TLPHR.DATA );
		pcie_writel( (uint32_t)(PCIE_MEM_IO), &dmxhq_reg->IOTAO );
	}else{
		pcie_writel( (uint32_t)(PCIE_MEM_BASE_LOWER_EP), &dmxhq_reg->MTAOL );
		pcie_writel( (uint32_t)(PCIE_MEM_BASE_UPPER_EP), &dmxhq_reg->MTAOH );
		pcie_writel( (uint32_t)(PCIE_SET_DMXHQ_TLPHR), &dmxhq_reg->TLPHR.DATA );
		pcie_writel( (uint32_t)(PCIE_MEM_IO), &dmxhq_reg->IOTAO );
	}

	BOOT_TIME_ADD1("E pcie_ctl_start");
	return;

}

static irqreturn_t pcie_err_interrupt(int irq, void *dev_id)
{
	reg_pcie_rcxh_t __iomem *reg;
	volatile void __iomem *aer;
	uint32_t stat, src, id, multi;
	uint32_t cor = 0;
	uint32_t unc = 0;

	reg = (reg_pcie_rcxh_t __iomem *)( pcie_reg.pcie_base );
	pcie_assert_NULL( reg );

	PCIE_DUMMY_READ;
	aer = (volatile void __iomem *)reg + PCIE_RC_AER_OFFSET;
	stat = pcie_read(aer + PCI_ERR_ROOT_STATUS);
	src = pcie_read(aer + PCI_ERR_ROOT_ERR_SRC);
	/* clear */
	pcie_raw_writel(stat, aer + PCI_ERR_ROOT_STATUS);

	if (stat & PCI_ERR_ROOT_UNCOR_RCV) {
		unc = pcie_read(aer + PCI_ERR_UNCOR_STATUS);
		/* clear */
		pcie_raw_writel(unc, aer + PCI_ERR_UNCOR_STATUS);
	}
	if (stat & PCI_ERR_ROOT_COR_RCV) {
		cor = pcie_read(aer + PCI_ERR_COR_STATUS);
		/* clear */
		pcie_raw_writel(cor, aer + PCI_ERR_COR_STATUS);
	}
	wmb();

	PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:AER:status=0x%x\n", stat);

	if (stat & PCI_ERR_ROOT_COR_RCV) {
		multi = stat & PCI_ERR_ROOT_MULTI_COR_RCV;
		id = PCIE_RC_AER_SRC_COR(src);
		PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:Correctable%s:0x%x:id=0x%04x\n", multi?"(multi)":"", cor, id);
	}
	if (stat & PCI_ERR_ROOT_UNCOR_RCV) {
		multi = stat & PCI_ERR_ROOT_MULTI_UNCOR_RCV;
		id = PCIE_RC_AER_SRC_UNC(src);
		PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:UnCorrectable%s:0x%x:id=0x%04x\n", multi?"(multi)":"", unc, id);
	}

	if (stat & PCI_ERR_ROOT_NONFATAL_RCV) {
		PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:Non-Fatal\n");
	}
	if (stat & PCI_ERR_ROOT_FATAL_RCV) {
		PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:Fatal\n");
#if 0
		BUG();
#endif
	}

	return IRQ_HANDLED;
}

static irqreturn_t pcie_axi_interrupt(int irq, void *dev_id)
{
	reg_pcie_rcxh_t __iomem *reg;
	uint32_t stat;

	reg = (reg_pcie_rcxh_t __iomem *)( pcie_reg.pcie_base );
	pcie_assert_NULL( reg );

	PCIE_DUMMY_READ;
	stat = pcie_read(&reg->AXIBCSR.AXIEIS2R.DATA);
	/* clear */
	pcie_raw_writel(stat, &reg->AXIBCSR.AXIEIS2R.DATA);
	wmb();

	PCIE_PRINTK_ERR(KERN_ERR "PCIe:ERROR:AXI:status=0x%x\n", stat);
	BUG();

	return IRQ_HANDLED;
}

static void pcie_ctl_get_reg_base( pcie_remap_reg_t *pcie_reg )
{
	pcie_assert_NULL( pcie_reg );

	pcie_reg->pcie_dmxhq_base   = ioremap( (unsigned long)PCIE_DMXHQ_BASE, (size_t)PCIE_DMXHQ_SIZE );
	if( pcie_reg->pcie_dmxhq_base == NULL ) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: pcie_dmxhq_base = (%p)\n", pcie_reg->pcie_dmxhq_base);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}

#if 0
	pcie_reg->pcie_base         = (void *)ioremap( (unsigned long)PCIE_REG_BASE, (size_t)PCIE_REG_SIZE );
	if( pcie_reg->pcie_base == NULL ) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: pcie_base = (%p)\n", pcie_reg->pcie_base);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}

	pcie_reg->wpcie_base        = (void *)ioremap( (unsigned long)WPCIE_REG_BASE, (size_t)WPCIE_REG_SIZE );
	if( pcie_reg->wpcie_base == NULL ) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: wpcie_base error = (%p)\n", pcie_reg->wpcie_base);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
#else
	pcie_reg->pcie_base       = IO_ADDRESSP((unsigned long)PCIE_REG_BASE);
	pcie_reg->wpcie_base      = IO_ADDRESSP((unsigned long)WPCIE_REG_BASE);
#endif


	return;
}

static void    PCIeRC_Initialize( pcie_remap_reg_t *pcie_reg )
{
	reg_pcie_rcxh_t __iomem *reg;
	reg_wpcie_t __iomem     *w_reg;
	volatile void __iomem   *p;
	uint32_t            count;

	pcie_assert_NULL( pcie_reg );

	reg = (reg_pcie_rcxh_t __iomem *)( pcie_reg->pcie_base );
	pcie_assert_NULL( reg );

	w_reg = (reg_wpcie_t __iomem *)( pcie_reg->wpcie_base );
	pcie_assert_NULL( w_reg );


	p = &w_reg->CONTROL.DATA;
	pcie_raw_writel( PCIE_SET_CTRL_INIT, p );

	pcie_set_bit_l( PCIE_SET_CTRL_PRV_RE_TYPE_RC, p );                         /* +000H RootComplex */

	pcie_set_bit_l( PCIE_SET_CTRL_RST_PONX, p );                               /* +000H Reset "10" */

	PCIE_PRINTK_DBG(KERN_INFO " WAIT Dualrole Link \n");
	for( count = 0; count < PCIE_WAIT_MAX; count++ ) {
		if( pcie_read( p ) & PCIE_SET_CTRL_ST_RE_TYPE ) {
			break;
		}
		udelay(1);
	}
	if( count == PCIE_WAIT_MAX ) {
		PCIE_PRINTK_ERR(KERN_ERR "PCIe timeout_error: %d\n", count );
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
	PCIE_PRINTK_DBG(KERN_INFO " Dualrole Link ACTIVE \n");

	pcie_rc_setLimitLinkSpeed(reg, PCIE_EPRC_EN_TSSR_TSCRR_RCS_2_5GT);

	pcie_raw_writel( 0x1d1e0A03U, pcie_reg->pcie_base + 0xB00U);

	pcie_raw_writel( PCIE_SET_EXTINT_CLR, &w_reg->EXTINT_CLR.DATA );
	pcie_raw_writel( PCIE_SET_EXTINT_EMB, &w_reg->EXTINT_ENB.DATA );

#if 0 /* moved to PCIe_RC_Configuration */
	pcie_clr_bit_l( PCIE_ARB_PCS_PERST, &reg->ARB.PCS.DATA );
#endif
	return;
}

static void PCIeEP_Initialize( pcie_remap_reg_t *pcie_reg )
{
	reg_pcie_epxh_t __iomem *reg;
	reg_wpcie_t     __iomem *w_reg;
	volatile void   __iomem *p;
	uint32_t            count;

	pcie_assert_NULL( pcie_reg );

	reg = (reg_pcie_epxh_t __iomem *)( pcie_reg->pcie_base );
	pcie_assert_NULL( reg );

	w_reg = (reg_wpcie_t __iomem *)( pcie_reg->wpcie_base );
	pcie_assert_NULL( w_reg );

	PCIE_PRINTK_DBG(KERN_INFO " PCIeEP_Initialize start... \n");
	p = &w_reg->CONTROL.DATA;
	pcie_raw_writel( PCIE_SET_CTRL_INIT, p );


	pcie_set_bit_l( PCIE_SET_CTRL_RST_PONX, p );                                /* +000H Reset "10" */

	pcie_set_bit_l( PCIE_AXIBCSR_AXIBMR_DLWM, &reg->AXIBCSR.AXIBMR.DATA );
	pcie_clr_bit_l( PCIE_COMP_IP, &reg->T0CSH.ILIPMGML.DATA );
	pcie_set_bit_l( PCIE_T0CSH_ILIPMGML_IP, &reg->T0CSH.ILIPMGML.DATA );
	pcie_clr_bit_l( PCIE_AXIBCSR_AXIBMR_DLWM, &reg->AXIBCSR.AXIBMR.DATA );

	pcie_raw_writel( 0x1d1e0A03U, pcie_reg->pcie_base + 0xB00U);

	pcie_set_bit_l( PCIE_SET_CTRL_RST_PEX, &w_reg->CONTROL.DATA );              /* +000H Reset "11" */

	PCIE_PRINTK_DBG(KERN_INFO " WAIT DL ACTIVE EP \n");

	for( count = 0; count < PCIE_WAIT_MAX; count++ ) {
		if( pcie_read( &reg->TSSR.TSSR.DATA ) & PCIE_RCEP_TSSR_DLA ) {
			break;
		}
		udelay(1);
	}
	if( count == PCIE_WAIT_MAX ) {
		PCIE_PRINTK_ERR(KERN_ERR "pcie_ctl_rc_wait_DLActive  TIME OUT ERROR\n");
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}

	PCIE_PRINTK_DBG(KERN_INFO " PCIeEP_Initialize end... \n");
	return;
}

static void  PCIe_RC_Configuration( pcie_remap_reg_t *pcie_reg )
{
	reg_pcie_rcxh_t __iomem *reg;
	uint32_t        count;
	uint32_t dat;

	pcie_assert_NULL( pcie_reg );

	reg = (reg_pcie_rcxh_t __iomem *)( pcie_reg->pcie_base );
	pcie_assert_NULL( reg );

	/*--- write enable ---*/
	pcie_set_bit_l( PCIE_AXIBCSR_AXIBMR_DLWM, &reg->AXIBCSR.AXIBMR.DATA );
	/* Set device class to PCI_CLASS_BRIDGE_PCI */
	writew_relaxed(PCI_CLASS_BRIDGE_PCI, (void __iomem *)&reg->T1CSH.CC+1);
	/* PCIe capabilities */
	dat = readw_relaxed((void __iomem *)&reg->PCIECR.CR+2);
	dat |= PCI_EXP_FLAGS_SLOT;
	writew_relaxed(dat, (void __iomem *)&reg->PCIECR.CR+2);
	/* Slot Capabilities */
	dat = PCIE_ROOT_PSN << PCI_EXP_SLTCAP_PSN_SHIFT;
	dat |= PCI_EXP_SLTCAP_HPC|PCI_EXP_SLTCAP_HPS;
	writel_relaxed(dat, (void __iomem *)&reg->PCIECR.SCR);
	/*--- write protect ---*/
	pcie_clr_bit_l( PCIE_AXIBCSR_AXIBMR_DLWM, &reg->AXIBCSR.AXIBMR.DATA );

	if (pcie_root_port_prsnt()) {
		/* negate PERST */
		pcie_clr_bit_l( PCIE_ARB_PCS_PERST, &reg->ARB.PCS.DATA );
		BOOT_TIME_ADD1("PCI:bus#1:PERST:negate");

	PCIE_PRINTK_DBG(KERN_INFO "WAIT DL ACTIVE \n");

	for( count = 0; count < PCIE_WAIT_MAX; count++ ) {
		if( pcie_read( &reg->TSSR.TSSR.DATA ) & PCIE_RCEP_TSSR_DLA ) {
			break;
		}
		udelay(1);
	}
	if( count == PCIE_WAIT_MAX ) {
		PCIE_PRINTK_ERR(KERN_ERR "ERROR: PCIe: pcie_ctl_rc_wait_DLActive  TIME OUT ERROR\n");
#if 0
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
#endif
	}

	}

#if !defined(CONFIG_PCIEAER)
	pcie_set_bit_l( PCIE_SET_RECR, &reg->AERC.RECR.DATA );              /* +12CH Root Error Command Register  */
	pcie_set_bit_w( PCIE_SET_BC,   &reg->T1CSH.BC );                    /* +03EH Root Error Command Register  */
	pcie_set_bit_l( PCIE_SET_DCSR, &reg->PCIECR.DCSR.DATA );            /* +088H Device Control Register  */
#endif /* !CONFIG_PCIEAER */

#if 0
	pcie_set_bit_l( PCIE_SET_CRSR, &reg->T1CSH.CRSR.DATA );            /* +004H Command Register  */
#endif

	return;
}

static void pcie_rc_setLimitLinkSpeed(reg_pcie_rcxh_t __iomem *reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed)
{
	uint16_t	wdata;

	wdata = pcie_read(&reg->PCIECR.LC2R.DATA);
	wdata &= ~PCIE_PCIECR_LC2R_TLS;			/* clear limit speed */
	wdata |= speed;					/* set limit speed */
	pcie_raw_writew(wdata, &reg->PCIECR.LC2R.DATA);
}

static void pcie_rc_setLinkSpeed(reg_pcie_rcxh_t __iomem *reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed)
{
	uint32_t	wdata;

	wdata = pcie_read(&reg->PCIECR.LCSR.DATA);
	wdata |= PCIE_PCIECR_LCSR_RL;		/* re-train link enable */
	wdata |= PCIE_PCIECR_LCSR_LABS;		/* clear link autonomous bandwidth status */
	wdata |= PCIE_PCIECR_LCSR_LBMS;		/* clear bandwidth manangement status */
	pcie_raw_writel(wdata , &reg->PCIECR.LCSR.DATA);
}

static unsigned int pcie_rc_getSpeedChangeStatus(reg_pcie_rcxh_t __iomem *reg)
{
	return pcie_read(&reg->TSSR.TSSR.DATA) & PCIE_RCEP_TSSR_SCS;
}

static unsigned int pcie_rc_getCurrentLinskSpeed(reg_pcie_rcxh_t __iomem *reg)
{
	return (pcie_read(&reg->PCIECR.LCSR.DATA) & PCIE_RCEP_TSSR_CS) >> PCIE_RCEP_TSSR_CS_BIT;
}

static void pcie_rc_changeLinkSpeed(pcie_remap_reg_t *pcie_reg, PCIE_EPRC_UN_PCIECR_LC2R_TLS speed)
{
	reg_pcie_rcxh_t	__iomem *reg;
	uint32_t		count;

	pcie_assert_NULL(pcie_reg);
	reg = (reg_pcie_rcxh_t __iomem *)(pcie_reg->pcie_base);
	pcie_assert_NULL(reg);

	PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  change rc's limit speed.\n");
	pcie_rc_setLimitLinkSpeed(reg, speed);

	if(pcie_read( &reg->TSSR.TSSR.DATA ) & PCIE_RCEP_TSSR_SCR) {
		/* link speed change available */
		;
	} else {
		/* link speed change not available */
        PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  can not change link speed.\n");
		return;
	}

	PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  change link speed start.\n");
	pcie_rc_setLinkSpeed(reg, speed);

	for(count=0; count<PCIE_WAIT_MAX; count++) {
		if(pcie_rc_getSpeedChangeStatus(reg) == PCIE_EPRC_EN_TSSR_TSSR_SCS_COMPLETED) {
			PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  change link speed completed.\n");
			break;
		} else {
			udelay(1);
		}
	}
	if(count == PCIE_WAIT_MAX) {
		PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  can not change link speed.\n");
	}

	switch(pcie_rc_getCurrentLinskSpeed(reg)) {
	case PCIE_EPRC_EN_TSSR_TSCRR_RCS_2_5GT:
		PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  current link speed : 2.5GT/s.\n");
		break;
	case PCIE_EPRC_EN_TSSR_TSCRR_RCS_5_0GT:
		PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  current link speed : 5.0GT/s.\n");
		break;
	default:
		PCIE_PRINTK_DBG(KERN_INFO "pcie_rc_changeLinkSpeed  current link speed : unknown.\n");
		break;
	}
}

static void pcie_set_bit_w( uint16_t data, volatile void __iomem *addr )
{
	uint16_t                wdata;

	pcie_assert_NULL( addr );

	wdata   =  readw_relaxed( addr );
	wdata   |= data;
	writew_relaxed( wdata, addr );
	return;
}

static void pcie_set_bit_l( uint32_t data, volatile void __iomem *addr )
{
	uint32_t                wdata;

	pcie_assert_NULL( addr );

	wdata   =  readl_relaxed( addr );
	wdata   |= data;
	writel_relaxed( wdata, addr );
	return;
}

static void pcie_clr_bit_l( uint32_t data, volatile void __iomem *addr )
{
	uint32_t                wdata;

	pcie_assert_NULL( addr );

	wdata   =  readl_relaxed( addr );
	wdata   &= ~data;
	writel_relaxed( wdata, addr );
	return;
}

/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
