/* 2011-12-22: File added by Sony Corporation */
/*
 *  File Name       : arch/arm/mach-emxx/include/mach/kzm9d.h
 *  Function        : kzm9d
 *  Release Version : Ver 1.00
 *  Release Date    : 2010/02/05
 *
 * Copyright (C) 2010 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#ifndef __ASM_ARCH_KZM9D_H
#define __ASM_ARCH_KZM9D_H

#include <mach/hardware.h>

/* CS0 */
#define EMEV_FLASH_BASE		EMXX_BANK0_BASE
#define EMEV_FLASH_SIZE		(SZ_64M)

/* CS2 */
#define EMEV_ETHER_OFFSET	0x00000000
#define EMEV_ETHER_SIZE		SZ_4K
#define EMEV_ETHER_BASE		(EMXX_BANK1A_BASE + EMEV_ETHER_OFFSET)

#if 0
/* CS2 */
#define EMEV_NAND_OFFSET	0x10000000
#define EMEV_NAND_SIZE		SZ_128M
#define EMEV_NAND_BASE		(EMXX_BANK0_BASE + EMEV_NAND_OFFSET)

#define EMEV_NAND_DATA_BASE	(EMEV_NAND_BASE + 0x00000000)
#define EMEV_NAND_COMMAND_BASE	(EMEV_NAND_BASE + 0x00020000)
#define EMEV_NAND_ADDRESS_BASE	(EMEV_NAND_BASE + 0x00040000)
#endif

/* I2C name & slave address */
#define I2C_SLAVE_RTC_NAME	"rtc8564"
#define I2C_SLAVE_RTC_ADDR	0x51
#define I2C_SLAVE_EXTIO1_NAME	"max7318_1"
#define I2C_SLAVE_EXTIO1_ADDR	0x20
#define I2C_SLAVE_EXTIO2_NAME	"max7318_2"
#define I2C_SLAVE_EXTIO2_ADDR	0x21
#define I2C_SLAVE_CAM_NAME	"camera"
#define I2C_SLAVE_CAM_ADDR	0x50
#define I2C_SLAVE_CAM_AF_NAME	"camera_af"
#define I2C_SLAVE_CAM_AF_ADDR	0x0C
#define I2C_SLAVE_HDMI_NAME	"adv7523"
#define I2C_SLAVE_HDMI_ADDR	0x39
#define I2C_SLAVE_SPDIF_NAME	"cs8427"
#define I2C_SLAVE_SPDIF_ADDR	0x10
#define I2C_SLAVE_CODEC_NAME	"ak4648"
//koba #define I2C_SLAVE_CODEC_ADDR	0x13
#define I2C_SLAVE_CODEC_ADDR	0x12
#define I2C_SLAVE_NTSC_ENC_NAME	"adv7179"
#define I2C_SLAVE_NTSC_ENC_ADDR	0x2B
#define I2C_SLAVE_NTSC_DEC_NAME	"ad8856"
#define I2C_SLAVE_NTSC_DEC_ADDR	0x44

#ifndef __ASSEMBLY__
static inline int extio_reg_read(int addr, unsigned char *data){ return 0;}
static inline int extio_reg_write(int addr, unsigned char data){ return 0;}
static inline int extio_read(int addr, unsigned char *data){ return 0;}
static inline int extio_write(int addr, unsigned char data, unsigned char mask){ return 0;}
static inline int extio_set_direction(unsigned gpio, int is_input){ return 0;}
static inline int extio_get_value(unsigned int gpio){ return 0;}
static inline void extio_set_value(unsigned int gpio, int value) {}
#endif /*__ASSEMBLY__*/
#endif	/* __ASM_ARCH_KZM9D_H */
