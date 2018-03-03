/* 2009-02-24: File added and changed by Sony Corporation */
/*
 *  File Name		: linux/arch/arm/mach-ne1/usb.c
 *  Function		: Initialized usb
 *  Release Version : Ver 1.00
 *  Release Date	: 2008/05/26
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

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <mach/hardware.h>
#include <mach/pci.h>
#include <mach/ne1_sysctrl.h>
#include <mach/irqs.h>

#include "core.h"

static void ne1_internal_pci_bridge_init(void)
{
	// Disable all BAR
	writel(0x00000000, PCI_IBRIDGE_BASE + PCI_PCIBAREn);

	// Reset/Unreset Internal PCI
	writel(0x80000000, PCI_IBRIDGE_BASE + PCI_PCICTRL_H);
	mdelay(1);
	writel(0x00000000, PCI_IBRIDGE_BASE + PCI_PCICTRL_H);

	writel(0x00000016, PCI_IBRIDGE_BASE + PCI_PCIINIT1);	// CPU->PCI Window1
	writel(0x00000016, PCI_IBRIDGE_BASE + PCI_PCIINIT2);	// CPU->PCI Window2

	writel(0x00000040, PCI_IBRIDGE_BASE + PCI_ACR0);	// PCI->CPU Windows0
	writel(PHYS_OFFSET, PCI_IBRIDGE_BASE + PCI_BASE_ADDRESS_0);	// BAR0
	writel(0x00000001, PCI_IBRIDGE_BASE + PCI_PCIBAREn); // Enable BAR0

	writel(0x00000000, PCI_IBRIDGE_BASE + PCI_PCIARB); // Arbitration(Fair)

	// Enable PCI MEM & BUSMASTER
	writel(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, PCI_IBRIDGE_BASE + PCI_COMMAND);

	writel(0x00000000, PCI_IBRIDGE_BASE + PCI_PCICTRL_L);	// Retry infinity
	writel(0x10000000, PCI_IBRIDGE_BASE + PCI_PCICTRL_H);	// Accept PCI access
}

int ne1_internal_read_config(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val)
{
	u32 conf_val, rval;

	if (PCI_SLOT(devfn) > 31 || PCI_FUNC(devfn) > 7 || where > 255 ||
			(size != 1 && size != 2 && size != 4)) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	// type0 access
	conf_val = 0x80000000 | (devfn << 8) | (where & ~3);

	writel(conf_val, PCI_IBRIDGE_BASE + PCI_CNFIG_ADDR);
	rval = readl(PCI_IBRIDGE_BASE + PCI_CNFIG_DATA) >> ((where & 0x3)*8);

	switch (size) {
	case 1:
		*val = rval & 0xff;
		break;
	case 2:
		*val = rval & 0xffff;
		break;
	default:
		*val = rval;
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

int ne1_internal_write_config(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val)
{
	u32 conf_val, rval, wval = 0;

	if (PCI_SLOT(devfn) > 31 || PCI_FUNC(devfn) > 7 || where > 255 ||
			(size != 1 && size != 2 && size != 4)) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	// type0 access
	conf_val = 0x80000000 | (devfn << 8) | (where & ~3);

	if (where & 0x3 || size == 1 || size == 2) {
		writel(conf_val, PCI_IBRIDGE_BASE + PCI_CNFIG_ADDR);
		rval = readl(PCI_IBRIDGE_BASE + PCI_CNFIG_DATA);
		switch (where & 0x3) {
		case 0:
			if (size == 1)
				wval = (rval & 0xffffff00) | ((val & 0xff) << 0);
			else if (size == 2)
				wval = (rval & 0xffff0000) | ((val & 0xffff) << 0);
			else
				return PCIBIOS_DEVICE_NOT_FOUND;
			break;

		case 1:
			if (size == 1)
				wval = (rval & 0xffff00ff) | ((val & 0xff) << 8);
			else if (size == 2)
				wval = (rval & 0xff0000ff) | ((val & 0xffff) << 8);
			else
				return PCIBIOS_DEVICE_NOT_FOUND;
			break;
		case 2:
			if (size == 1)
				wval = (rval & 0xff00ffff) | ((val & 0xff) << 16);
			else if (size == 2)
				wval = (rval & 0x0000ffff) | ((val & 0xffff) << 16);
			else
				return PCIBIOS_DEVICE_NOT_FOUND;
			break;
		case 3:
			if (size == 1)
				wval = (rval & 0x00ffffff) | ((val & 0xff) << 24);
			else
				return PCIBIOS_DEVICE_NOT_FOUND;
			break;
		}
	} else {
		wval = val;
	}

	writel(conf_val, PCI_IBRIDGE_BASE + PCI_CNFIG_ADDR);
	writel(wval, PCI_IBRIDGE_BASE + PCI_CNFIG_DATA);

	return PCIBIOS_SUCCESSFUL;
}

static int ne1_ohci_init(void)
{
	u32 devfn, val;

	/* Initialized USB OHCI */
	devfn = PCI_DEVFN(19, 0);
	ne1_internal_read_config(NULL, devfn, PCI_VENDOR_ID, 4, &val);
	if (val == 0x00351033) {
		val = NE1_BASE_USBH;
		ne1_internal_write_config(NULL, devfn, PCI_BASE_ADDRESS_0, 4, val);
		val = IRQ_OHCI;
		ne1_internal_write_config(NULL, devfn, PCI_INTERRUPT_LINE, 1, val);
		val = PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
		ne1_internal_write_config(NULL, devfn, PCI_COMMAND, 2, val);

		ne1_internal_read_config(NULL, devfn, 0xE0, 4, &val);
		val |= (1 << 21);
		ne1_internal_write_config(NULL, devfn, 0xE0, 4, val);

#ifndef CONFIG_USB_EHCI_HCD
		ne1_internal_read_config(NULL, devfn, 0xE4, 4, &val);
		val |= 1;
		ne1_internal_write_config(NULL, devfn, 0xE4, 4, val);
#endif
		return 0;
	} else {
		printk("%s: Don't found OHCI controller. %x\n", __FUNCTION__, val);
		return -1;
	}
}

static int ne1_ehci_init(void)
{
	u32 devfn, val;

	/* Initialized USB EHCI */
	devfn = PCI_DEVFN(19, 1);
	ne1_internal_read_config(NULL, devfn, PCI_VENDOR_ID, 4, &val);
	if (val == 0x00E01033) {
		val = NE1_BASE_USBH + 0x1000;
		ne1_internal_write_config(NULL, devfn, PCI_BASE_ADDRESS_0, 4, val);
		val = IRQ_EHCI;
		ne1_internal_write_config(NULL, devfn, PCI_INTERRUPT_LINE, 1, val);
		val = 0xF0;
		ne1_internal_write_config(NULL, devfn, PCI_LATENCY_TIMER, 1, val);
		val = PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
		ne1_internal_write_config(NULL, devfn, PCI_COMMAND, 2, val);
		return 0;
	} else {
		printk("%s: Don't found EHCI controller. %x\n", __FUNCTION__, val);
		return -1;
	}
}

static int __init ne1xb_usb_init(void)
{
	// Enable PCI clock
	writel(readl(SYSCTRL_BASE + SYSCTRL_CLKMSK) & ~SYSCTRL_CLKMSK_PCI,
			SYSCTRL_BASE + SYSCTRL_CLKMSK);

	ne1_internal_pci_bridge_init();
	if (ne1_ohci_init()) {
		platform_device_unregister(&ne1xb_ohci_device);
	}
	if (ne1_ehci_init()) {
		platform_device_unregister(&ne1xb_ehci_device);
	}

	return 0;
}

arch_initcall(ne1xb_usb_init);



