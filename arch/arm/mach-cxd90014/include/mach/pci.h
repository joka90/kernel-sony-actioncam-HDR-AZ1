#ifndef __ARCH_ARM_MACH_PCI_H__
#define __ARCH_ARM_MACH_PCI_H__
#ifdef __cplusplus
extern "C" {
#endif
/*
#define pcibios_assign_all_busses()     1
*/

#ifdef CONFIG_QEMU
#define QEMU_PCI_IRQBASE	242
#define QEMU_PCI_OFFSET 0x8a000000
#define QEMU_PCI_CORE_BASE	(0x38000000 + QEMU_PCI_OFFSET)
#define QEMU_PCI_VIRT_BASE	(0x39000000 + QEMU_PCI_OFFSET)
#define QEMU_PCI_CFG_VIRT_BASE  (0x3a000000 + QEMU_PCI_OFFSET)
#define QEMU_PCI_IO_VIRT_BASE   (0x40000000 + QEMU_PCI_OFFSET)

#define QEMU_PCI_IO_BASE0       0x40000000
#define QEMU_PCI_MEM_BASE0      0x41000000
#define QEMU_PCI_MEM_BASE1      0x42000000
#define QEMU_PCI_MEM_BASE2      0x43000000

#define QEMU_PCI_IO_BASE0_SIZE       0x01000000
#define QEMU_PCI_MEM_BASE0_SIZE      0x01000000
#define QEMU_PCI_MEM_BASE1_SIZE      0x01000000
#define QEMU_PCI_MEM_BASE2_SIZE      0x01000000

#define PCIBIOS_MIN_IO		QEMU_PCI_IO_BASE0
#define PCIBIOS_MIN_MEM		QEMU_PCI_MEM_BASE0

#else

#define pcibios_assign_all_busses()     1
#include <mach/hardware.h>
#define PCIBIOS_MIN_IO		CXD90014_PCIE_IO_VIRT_BASE
#define PCIBIOS_MIN_MEM		CXD90014_PCIE_MEM_NON_PREFETCH
#endif

#ifdef __cplusplus
}
#endif
#endif
