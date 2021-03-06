#ifndef ASMARM_HARDWARE_ARM_SCU_H
#define ASMARM_HARDWARE_ARM_SCU_H

/*
 * SCU registers
 */
#define SCU_CTRL		0x00
# define SCU_ENABLE		 0x01
#define SCU_CONFIG		0x04
#define SCU_CPU_STATUS		0x08
#define SCU_INVALIDATE		0x0c
# define SCU_INV_ALL		 0x0000ffff
#define SCU_FPGA_REVISION	0x10

#endif
