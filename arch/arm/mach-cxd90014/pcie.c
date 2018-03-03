/*
 * arch/arm/mach-cxd90014/pcie.c
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


#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/pcieport_if.h>
#include <linux/mbus.h>
#include <asm/mach/pci.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/delay.h>

#include <mach/irqs.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/snsc_boot_time.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <mach/pcie_export.h>
#include <mach/moduleparam.h>
#include <mach/cxd90014_cfg.h>
#include <mach/regs-gpio.h>

#include "pcie.h"
#include "pcie_ctl.h"
#include "pci_platform.h"

module_param_named(enable, cxd90014_pcie_enable, bool, S_IRUSR);

/* Override INTx trigger type */
static int pcie_irq_trigger[PCIE_INT_MASK+1]; /* zeroed */
#define PCIE_IRQ_TRIGGER_NONE	0 /* do not override */
#define PCIE_IRQ_TRIGGER_EDGE	1 /* edge  trigger */
#define PCIE_IRQ_TRIGGER_LEVEL	2 /* level trigger */
module_param_named(inta, pcie_irq_trigger[0], int, S_IRUSR);
module_param_named(intb, pcie_irq_trigger[1], int, S_IRUSR);
module_param_named(intc, pcie_irq_trigger[2], int, S_IRUSR);
module_param_named(intd, pcie_irq_trigger[3], int, S_IRUSR);

extern uint pci_n_slot; /* driver/pci/probe.c */
module_param_named(nslot, pci_n_slot, uint, S_IRUSR);

static int pcie_peri_eeprom;
module_param_named(peri_eeprom, pcie_peri_eeprom, bool, S_IRUSR);

static unsigned long pcie_prsnt_port = CXD90014_PORT_UNDEF;
module_param_named(prsnt, pcie_prsnt_port, port, S_IRUSR|S_IWUSR);
static int pcie_prsnt_pol = 1;
module_param_named(prsnt_pol, pcie_prsnt_pol, int, S_IRUSR|S_IWUSR);

static uint pcie_pon_delay;
module_param_named(pon_delay, pcie_pon_delay, uint, S_IRUSR|S_IWUSR);

static void                 __pci_addr(struct pci_bus *bus,unsigned int devfn, int offset);
static int          pcie2_dmx_read_config(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val);
static int          pcie2_dmx_write_config(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val);
static void         pcie_get_config_base( pcie_remap_config_t *pcie_config );
static int          __init  f_pcie2_dmx_setup_resources(struct resource **resource);
static int          __init  f_pcie2_dmx_setup(int nr, struct pci_sys_data *sys);
static int          f_pcie2_dmx_map_irq(struct pci_dev *dev, u8 slot, u8 pin);
static struct pci_bus *    __init  f_pcie2_dmx_scan_bus(int nr, struct pci_sys_data *sys);
static void         __init  f_pcie2_dmx_preinit(void);
static int          __init  f_pcie2_dmx_setup(int nr, struct pci_sys_data *sys);

static pcie_remap_config_t  pcie_config;

static int pcie_select = PCIE_MODE_SELECT;

static uint32_t  pcie_root_busnr;

static struct pci_ops f_pcie2_dmx_ops = {
	.read   = pcie2_dmx_read_config,
	.write  = pcie2_dmx_write_config,
};

static struct resource io_mem = {
	.name   = "PCI I/O space",
	.start  = PCIE_MEM_IO,
	.end    = PCIE_MEM_IO+PCIE_MEM_IO_SIZE-1,
	.flags  = IORESOURCE_IO,
};

static struct resource non_mem = {
	.name   = "PCI non-prefetchable",
	.start  = PCIE_MEM_NON_PREFETCH,
	.end    = UL(PCIE_MEM_NON_PREFETCH)+UL(PCIE_MEM_NON_PREFETCH_SIZE)-1,
	.flags  = IORESOURCE_MEM,
};

static struct hw_pci pcie2_dmx __initdata = {
	.swizzle        = pci_std_swizzle,
	.map_irq        = f_pcie2_dmx_map_irq,
	.nr_controllers = 1,
	.setup          = f_pcie2_dmx_setup,
	.scan           = f_pcie2_dmx_scan_bus,
	.preinit        = f_pcie2_dmx_preinit,
};


static void __pci_addr(struct pci_bus *bus,
		       unsigned int devfn, int offset)
{
	unsigned int busnr = bus->number;

	/*
	 * Trap out illegal values
	 */
	if (offset > PCIE_DEV_OFFSET_MAX) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: offset error = (%d)\n", offset);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
	if (busnr > PCIE_BUS_MAX) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: busnr error = (%d)\n", busnr);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
	if (devfn > PCIE_DEVFN_MAX) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: devfn error = (%d)\n", devfn);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
	return;
}

static int pcie2_dmx_read_config(struct pci_bus *bus, unsigned int devfn, int where,
				 int size, u32 *val)
{
	uint32_t __iomem *config_reg;
	u32 v;
	volatile void __iomem *addr;
	int slot = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);

	__pci_addr(bus, devfn, where & ~PCIE_MASK_ACCESS);

	if (0 == bus->number) {
		/* BUS#0 is a virtual bus for connecting Root Complexes. */
		config_reg = (uint32_t __iomem *)pcie_ctl_root_config(slot);
		if (!config_reg) {
			*val = 0xFFFFFFFFU;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
	} else {
	/*PCIe config data read*/
	pcie_ctl_cfg_io_set((u8) bus->number,(u8) slot,(u8) func );

	config_reg = (uint32_t __iomem *)( pcie_config.pcie_config_base_0 );
	pcie_assert_NULL( config_reg );
	}

	addr = (volatile void __iomem *)((uint32_t __iomem *)config_reg + ( where / sizeof(uint32_t)));
	if (pcie_ctl_cfg_read(addr, &v) < 0) { /* ERROR ? */
		PCIE_PRINTK_DBG(KERN_INFO "%02x:%02x:%d: Unsupported Request Completion\n", bus->number, slot, func);
		*val = 0xFFFFFFFFU;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	switch( size ) {
	case PCIE_LONG_ACCESS:
		*val = v;
		break;
	case PCIE_SHORT_ACCESS:
		*val = (v >> (PCIE_ACCESS_BOUNDARY * (where & PCIE_MASK_ACCESS))) & 0xffff;
		break;
	case PCIE_BYTE_ACCESS:
		*val = (v >> (PCIE_ACCESS_BOUNDARY * (where & PCIE_MASK_ACCESS))) & 0xff;
		break;
	default:
		PCIE_PRINTK_ERR(KERN_ERR "PCI: size error = (%d)\n", size );
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	return PCIBIOS_SUCCESSFUL;
}

static int pcie2_dmx_write_config(struct pci_bus *bus, unsigned int devfn, int where,
				  int size, u32 val)
{
	void __iomem *config_reg;
	volatile void __iomem *addr;
	int slot = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);

	__pci_addr(bus, devfn, where );

	/* PCIe config data write */
	if (0 == bus->number) {
		/* BUS#0 is a virtual bus for connecting Root Complexes. */
		config_reg = (uint32_t __iomem *)pcie_ctl_root_config(slot);
		if (!config_reg) {
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		/* hook */
		pcie_ctl_cfg_root_hook(slot, where, size, val);
	} else {
	pcie_ctl_cfg_io_set((u8) bus->number,(u8) slot,(u8) func );

	config_reg = (uint32_t __iomem *)( pcie_config.pcie_config_base_0 );
	pcie_assert_NULL( config_reg );
	}

	switch( size ) {
	case PCIE_LONG_ACCESS:
		addr = (volatile void __iomem *)((uint32_t __iomem *)config_reg + ( where / sizeof(uint32_t)));
		pcie_writel( (u32)val , addr );
		break;
	case PCIE_SHORT_ACCESS:
		addr = (volatile void __iomem *)((uint16_t __iomem *)config_reg + ( where / sizeof(uint16_t)));
		pcie_writew( (u16)val , addr );
		break;
	case PCIE_BYTE_ACCESS:
		addr = (volatile void __iomem *)((uint8_t __iomem *)config_reg + ( where / sizeof(uint8_t)));
		pcie_writeb( (u8)val , addr );
		break;
	default:
		PCIE_PRINTK_ERR(KERN_ERR "PCI: size error = (%d)\n", size );
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	return PCIBIOS_SUCCESSFUL;
}

static void pcie_get_config_base( pcie_remap_config_t *pcie_config )
{

	pcie_assert_NULL( pcie_config );

	pcie_config->pcie_config_base_0   = ioremap( (unsigned long)PCIE_CONFIG_0, (size_t)PCIE_CONFIG_SIZE );
	if( pcie_config->pcie_config_base_0 == NULL ) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: pcie_config_base_0 error = (%p)\n", pcie_config->pcie_config_base_0);
		PCIE_PRINTK_ERR(KERN_ERR "BUG %s(%d)\n",__FUNCTION__,__LINE__);
		BUG();
	}
	return;
}

static int __init f_pcie2_dmx_setup_resources(struct resource **resource)
{
	int ret;

	ret = request_resource(&ioport_resource, &io_mem);
	if (ret) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: unable to allocate I/O "
		       "memory region (%d)\n", ret);
		goto _OUT;
	}
	ret = request_resource(&iomem_resource, &non_mem);
	if (ret) {
		PCIE_PRINTK_ERR(KERN_ERR "PCI: unable to allocate non-prefetchable "
		       "memory region (%d)\n", ret);
		goto _RELEASE_IO_MEM;
	}

	/*
	 * bus->resource[0] is the IO resource for this bus
	 * bus->resource[1] is the mem resource for this bus
	 * bus->resource[2] is none
	 */
	resource[0] = &io_mem;
	resource[1] = &non_mem;
	resource[2] = NULL;

	goto _OUT;

_RELEASE_IO_MEM:
	release_resource(&io_mem);
_OUT:
	return ret;
}

/* PERI PCIe switch */
#define PCIE_PERI_ID	0x230312d8
#define PCIE_PERI_BUS	0 /* at this point "Secondary Bus Number" is 0. */
#define PCIE_PERI_SLOT	0
#define PCIE_PERI_FUNC	0
#define PCIE_PERI_EEPROM	0xbc
#define PCIE_PERI_EEPROM_DONE	0x08

static void f_pcie2_platform_init(void)
{
	volatile void __iomem *config;
	u32 v;
	int i;

	if (PCIE_MODE_ROOT != pcie_select)
		return;

	if (pcie_peri_eeprom) {
		/*
		 * Wait for loading EEPROM
		 * We can not use quirk, because VID:DID is wrong at this point.
		 *   Max time = 256*38 / (PEXCLK/1024) == 80[ms]
		 */
		pcie_ctl_cfg_io_set(PCIE_PERI_BUS, PCIE_PERI_SLOT, PCIE_PERI_FUNC);
		config = (volatile void __iomem *)pcie_config.pcie_config_base_0;
		pcie_ctl_cfg_read(config + PCI_VENDOR_ID, &v);
		if (PCIE_PERI_ID == v)
			return;
		for (i = 0; i < 800; i++) {
			if (pcie_ctl_cfg_read(config + PCIE_PERI_EEPROM, &v) < 0)
				break;
			if (v & PCIE_PERI_EEPROM_DONE)
				break;
			udelay(100);
		}
	}
}

static int __init f_pcie2_dmx_setup(int nr, struct pci_sys_data *sys)
{
	int ret = 0;

	if (nr == 0) {
		sys->mem_offset = 0;
		ret = f_pcie2_dmx_setup_resources(sys->resource);
		if (ret < 0) {
			PCIE_PRINTK_ERR(KERN_ERR "f_pcie2_dmx_setup: resources... oops?\n");
			goto _OUT;
		}
	} else {
		PCIE_PRINTK_ERR(KERN_ERR "f_pcie2_dmx_setup: resources... nr == 0??\n");
		goto _OUT;
	}

	pcie_get_config_base( &pcie_config );

	pcie_root_busnr = sys->busnr;

	ret = 1;

	f_pcie2_platform_init();

_OUT:
	return ret;
}

static struct pci_bus * __init f_pcie2_dmx_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_bus *bus;

	BOOT_TIME_ADD1("B f_pcie2_dmx_scan_bus");
	bus = pci_scan_bus(sys->busnr, &f_pcie2_dmx_ops, sys);
	BOOT_TIME_ADD1("E f_pcie2_dmx_scan_bus");
	return bus;
}

static void __init f_pcie2_dmx_preinit(void)
{
	pcie_ctl_start( PCIE_MODE_ROOT );
	return;
}

static int f_pcie2_dmx_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	u8 intx;
	int irq;

	/* slot,  pin,  irq
	 *  XX     1     PCIE_MSG_INTA_O
	 *  XX+1   1     PCIE_MSG_INTB_O
	 *  XX+2   1     PCIE_MSG_INTC_O
	 *  XX+3   1     PCIE_MSG_INTD_O
	 */
	intx = (slot + pin - 1) & PCIE_INT_MASK;
	irq = PCIE_MSG_INTA_O + intx;

	if (pcie_irq_trigger[intx]) { /* override trigger type */
		if (PCIE_IRQ_TRIGGER_EDGE == pcie_irq_trigger[intx]) {
			/* INTx: edge interrupt */
			irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
		} else {
			/* INTx: level interrupt */
			irq_set_irq_type(irq, IRQ_TYPE_LEVEL_HIGH);
		}
	}

	PCIE_PRINTK_INFO(KERN_INFO "PCI map irq: slot %d, pin %d, irq %d \n", slot, pin, irq);

	return irq;
}

#ifdef CONFIG_PM
void f_pcie2_dmx_suspend(void)
{
	if (!cxd90014_pcie_enable)
		return;

	pcie_ctl_suspend(pcie_select);
}

void f_pcie2_dmx_resume(void)
{
	if (!cxd90014_pcie_enable)
		return;

	if (pcie_pon_delay) {
		mdelay(pcie_pon_delay);
	}
	pcie_ctl_resume(pcie_select);
	f_pcie2_platform_init();
}
#endif /* CONFIG_PM */

int pcie_root_port_prsnt(void)
{
	unsigned int ch, bit, dat;
	int ret;

	if (CXD90014_PORT_UNDEF == pcie_prsnt_port) {
		/* always present */
		return 1;
	}
	ch = pcie_prsnt_port & 0xff;
	bit = (pcie_prsnt_port >> 8) & 0xff;
	dat = readl_relaxed(VA_GPIO(ch,RDATA));
	dat = (dat >>bit) & 0x00000001;
	ret = (dat == pcie_prsnt_pol);
	PCIE_PRINTK_INFO(KERN_DEBUG "pcie_root_port_prsnt: %d\n", ret);
	return ret;
}

int pcie_root_port_pdc(void)
{
	if (CXD90014_PORT_UNDEF == pcie_prsnt_port) {
		return 0;
	}
	return 1;
}

int pcie_port_perst(struct pci_dev *bridge, int assert)
{
	u16 p2p_ctrl;
	int changed, now;

	changed = 0;
	pci_read_config_word(bridge, PCI_BRIDGE_CONTROL, &p2p_ctrl);
	now = !!(p2p_ctrl & PCI_BRIDGE_CTL_BUS_RESET);
	if (now != assert) {
		p2p_ctrl ^= PCI_BRIDGE_CTL_BUS_RESET;
		changed = 1;
		pci_write_config_word(bridge, PCI_BRIDGE_CONTROL, p2p_ctrl);
	}

	if (PCI_EXP_TYPE_ROOT_PORT == bridge->pcie_type) {
		u32 pwr_csr;

		changed = 0; /* override */
		pci_read_config_dword(bridge, PCIE_ARB_PCS, &pwr_csr);
		now = !!(pwr_csr & PCIE_ARB_PCS_PERST);
		if (now != assert) {
			pwr_csr ^= PCIE_ARB_PCS_PERST;
			changed = 1;
			pci_write_config_dword(bridge, PCIE_ARB_PCS, pwr_csr);
		}
	}
	if (changed) {
		char buf[64];
		scnprintf(buf, sizeof buf, "PCI:bus#%u:PERST:%s",
			  bridge->subordinate->number,
			  assert ? "assert":"negate");
		BOOT_TIME_ADD1(buf);
	}
	return changed;
}

void pcibios_configure_slot(struct pci_dev *dev)
{
	u8 pin, slot;
	struct pci_sys_data *sys = dev->sysdata;

	/* setup legacy IRQ */
	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &pin);
	if (pin && sys && sys->map_irq) {
		if (pin > 4)
			pin = 1;
		slot = 0;
		if (sys->swizzle)
			slot = sys->swizzle(dev, &pin);
		dev->irq = sys->map_irq(dev, slot, pin);
		pcibios_update_irq(dev, dev->irq);
	}
}

int arch_pcie_service_irqs(struct pci_dev *dev, int *irqs, int mask)
{
	int slot = PCI_SLOT(dev->devfn);

	if (PCI_EXP_TYPE_ROOT_PORT == dev->pcie_type && !slot) { /* DMXHQ */
		/* AER interrupt */
		irqs[PCIE_PORT_SERVICE_AER_SHIFT] = IRQ_PCIE_ERR;
		/* Hot-Plug */
		irqs[PCIE_PORT_SERVICE_HP_SHIFT] = IRQ_PCIE_EXPCAP;
		return 0;
	}
	return -1;
}

static int __init f_pcie2_dmx_init(void)
{
	if (!cxd90014_pcie_enable)
		return 0;

	pcibios_min_io = PCIE_MEM_IO;
	pcibios_min_mem = PCIE_MEM_NON_PREFETCH;


	if( pcie_select == PCIE_MODE_ROOT ) {
		pci_common_init(&pcie2_dmx);
	} else {
		pcie_ctl_start( PCIE_MODE_ENDPOINT );
	}
	return 0;
}

static int __init f_pcie2_dmx_mode_select(char *str)
{
	int   val;

	get_option( &str, &val);
	if ( val != 0 ) {
		/* Root complex */
		pcie_select = PCIE_MODE_ROOT;
	    PCIE_PRINTK_DBG(KERN_INFO "[%s] Root complex mode\n",__FUNCTION__);
	} else {
		/* Endpoint */
		pcie_select = PCIE_MODE_ENDPOINT;
		PCIE_PRINTK_DBG(KERN_INFO "[%s] Endpoint Mode\n",__FUNCTION__);
	}


	return 1;
}

__setup("pcie_select=", f_pcie2_dmx_mode_select);   /* select_pcie_mode(0:endpoint / 1:rootcomplex */
subsys_initcall(f_pcie2_dmx_init);
/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
