/*
 * arch/arm/mach-cxd90014/pci_platform.h
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


#ifndef __PCI_PLATFORM_H__
#define __PCI_PLATFORM_H__


#ifdef __cplusplus
extern "C" {
#endif

#define PCIE_MODE_ENDPOINT      0
#define PCIE_MODE_ROOT          1

/* select_pcie_mode(0:endpoint / 1:rootcomplex */
#define PCIE_MODE_SELECT      PCIE_MODE_ROOT

/* PCI space */
#define PCIE_MEM_IO                 CXD90014_PCIE_IO_PHYS_BASE /* PCIe IO space base address */
#define PCIE_MEM_NON_PREFETCH       CXD90014_PCIE_MEM_NON_PREFETCH
#define PCIE_MEM_BASE_LOWER         PCIE_MEM_NON_PREFETCH
#define PCIE_MEM_BASE_UPPER         0x00000000U
#define PCIE_MEM_BASE_LOWER_EP      0x80000000U
#define PCIE_MEM_BASE_UPPER_EP      0x00000000U

/* Sizes of above maps */
#define PCIE_MEM_IO_SIZE            CXD90014_PCIE_IO_SIZE
#define PCIE_MEM_NON_PREFETCH_SIZE  CXD90014_PCIE_MEM_NON_PREFETCH_SIZE

/* Config offset */
#define PCIE_MAP_CONFIG_OFFSET      ( PCIE_MEM_IO + 0x04000000U )

#define PCIE_CONFIG_SIZE            0x1000U
#define PCIE_CONFIG_0               ( PCIE_MAP_CONFIG_OFFSET + (PCIE_CONFIG_SIZE * 0 ))

#define PCIE_DMXHQ_BASE             ((void*)0x3F000000U)
#define PCIE_DMXHQ_SIZE             0x1000U
#define PCIE_REG_BASE               ((void*)0xF0100000U)
#define PCIE_REG_SIZE               0x1000U
#define WPCIE_REG_BASE              ((void*)0xF0101000U)
#define WPCIE_REG_SIZE              0x1000U

#define PCIE_START_INT              235U
#define PCIE_ST_ERR_COM_O           ( PCIE_START_INT + 1U )
#define PCIE_ST_ERR_AXI_O           ( PCIE_START_INT + 3U )
#define PCIE_MSG_INTA_O             ( PCIE_START_INT + 5U )
#define PCIE_MSG_INTB_O             ( PCIE_START_INT + 6U )
#define PCIE_MSG_INTC_O             ( PCIE_START_INT + 7U )
#define PCIE_MSG_INTD_O             ( PCIE_START_INT + 8U )
#define PCIE_T0CSH_INT_O            1UL
#define PCIE_T0CSH_ILIPMGML_IP      (PCIE_T0CSH_INT_O << 8U)

#define PCIE_WAIT_MAX               1000000U

/* Root Port Physical Slot Number */
#define PCIE_ROOT_PSN               1U

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif /* #ifndef __PCI_PLATFORM_H__ */

/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
