/*
 * arch/arm/mach-cxd90014/pcie.h
 *
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
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


#ifndef __PCIE_H__
#define __PCIE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PCIE_DEV_OFFSET_MIN     0x000U
#define PCIE_DEV_OFFSET_MAX     0xfffU

#define PCIE_BUS_MIN            0
#define PCIE_BUS_MAX            255U

#define PCIE_DEVICE_MIN         0
#define PCIE_DEVICE_MAX         31U

#define PCIE_DEVFN_MIN          0
#define PCIE_DEVFN_MAX        255U

#define PCIE_ACCESS_BOUNDARY    8U
#define PCIE_BYTE_ACCESS        1U
#define PCIE_SHORT_ACCESS       2U
#define PCIE_LONG_ACCESS        4U

#define PCIE_MASK_ACCESS        3U

#define PCIE_INT_MASK           3U

#define PCIE_PRINT_ENABLE       0U

typedef struct pcie_remap_config{
    volatile void __iomem *pcie_config_base_0;
} pcie_remap_config_t;

#if PCIE_PRINT_ENABLE
#define PCIE_PRINTK_DBG     printk
#else
#define PCIE_PRINTK_DBG(...)
#endif

#define PCIE_PRINTK_ERR     printk
#define PCIE_PRINTK_INFO    printk

#ifdef CONFIG_PM
	extern void f_pcie2_dmx_suspend(void);
	extern void f_pcie2_dmx_resume(void);
#endif /* CONFIG_PM */

	extern int pcie_root_port_prsnt(void);
	extern int pcie_root_port_pdc(void);
	extern int pcie_port_perst(struct pci_dev *bridge, int assert);

#ifdef __cplusplus
}
#endif
#endif  /* __PCIE_H__ */
/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
