/* 2011-09-04: File added by Sony Corporation */
/*
 *  File Name	    : arch/arm/mach-emxx/include/mach/pmu.h
 *  Function	    : pmu
 *  Release Version : Ver 1.01
 *  Release Date    : 2010/08/26
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

#ifndef __ASM_ARCH_PMU_H
#define __ASM_ARCH_PMU_H

#include <mach/smu.h>

#define SMU_REG_SHIFT(x)	(((x) & 0xffff) << 16)

/* clock */
#define EMXX_CLK_DCV		(SMU_REG_SHIFT(SMU_DSPGCLKCTRL) | 8)
#define EMXX_CLK_DCV_P		(SMU_REG_SHIFT(SMU_DSPGCLKCTRL) | 4)
#define EMXX_CLK_DSP_A		(SMU_REG_SHIFT(SMU_DSPGCLKCTRL) | 2)
#define EMXX_CLK_DSP		(SMU_REG_SHIFT(SMU_DSPGCLKCTRL) | 1)
#define EMXX_CLK_GIO_INT	(SMU_REG_SHIFT(SMU_GIOGCLKCTRL) | 2)
#define EMXX_CLK_GIO		(SMU_REG_SHIFT(SMU_GIOGCLKCTRL) | 1)
#define EMXX_CLK_INTA_T		(SMU_REG_SHIFT(SMU_INTAGCLKCTRL) | 4)
#define EMXX_CLK_INTA_		(SMU_REG_SHIFT(SMU_INTAGCLKCTRL) | 2)
#define EMXX_CLK_INTA_P		(SMU_REG_SHIFT(SMU_INTAGCLKCTRL) | 1)
#define EMXX_CLK_CHG1		(SMU_REG_SHIFT(SMU_CHGGCLKCTRL) | 2)
#define EMXX_CLK_CHG		(SMU_REG_SHIFT(SMU_CHGGCLKCTRL) | 1)
#define EMXX_CLK_P2M_T		(SMU_REG_SHIFT(SMU_P2MGCLKCTRL) | 4)
#define EMXX_CLK_P2M		(SMU_REG_SHIFT(SMU_P2MGCLKCTRL) | 2)
#define EMXX_CLK_P2M_P		(SMU_REG_SHIFT(SMU_P2MGCLKCTRL) | 1)
#define EMXX_CLK_M2P_T		(SMU_REG_SHIFT(SMU_M2PGCLKCTRL) | 4)
#define EMXX_CLK_M2P		(SMU_REG_SHIFT(SMU_M2PGCLKCTRL) | 2)
#define EMXX_CLK_M2P_P		(SMU_REG_SHIFT(SMU_M2PGCLKCTRL) | 1)
#define EMXX_CLK_M2M		(SMU_REG_SHIFT(SMU_M2MGCLKCTRL) | 2)
#define EMXX_CLK_M2M_P		(SMU_REG_SHIFT(SMU_M2MGCLKCTRL) | 1)
#define EMXX_CLK_PMU_32K	(SMU_REG_SHIFT(SMU_PMUGCLKCTRL) | 2)
#define EMXX_CLK_PMU		(SMU_REG_SHIFT(SMU_PMUGCLKCTRL) | 1)
#define EMXX_CLK_SRC		(SMU_REG_SHIFT(SMU_SRCGCLKCTRL) | 1)
#define EMXX_CLK_LCD_C		(SMU_REG_SHIFT(SMU_LCDGCLKCTRL) | 8)
#define EMXX_CLK_LCD_L		(SMU_REG_SHIFT(SMU_LCDGCLKCTRL) | 4)
#define EMXX_CLK_LCD_P		(SMU_REG_SHIFT(SMU_LCDGCLKCTRL) | 2)
#define EMXX_CLK_LCD		(SMU_REG_SHIFT(SMU_LCDGCLKCTRL) | 1)
#define EMXX_CLK_IMC		(SMU_REG_SHIFT(SMU_IMCGCLKCTRL) | 2)
#define EMXX_CLK_IMC_P		(SMU_REG_SHIFT(SMU_IMCGCLKCTRL) | 1)
#define EMXX_CLK_IMCW		(SMU_REG_SHIFT(SMU_IMCWGCLKCTRL) | 2)
#define EMXX_CLK_IMCW_P		(SMU_REG_SHIFT(SMU_IMCWGCLKCTRL) | 1)
#define EMXX_CLK_SIZ		(SMU_REG_SHIFT(SMU_SIZGCLKCTRL) | 2)
#define EMXX_CLK_SIZ_P		(SMU_REG_SHIFT(SMU_SIZGCLKCTRL) | 1)
#define EMXX_CLK_ROT		(SMU_REG_SHIFT(SMU_ROTGCLKCTRL) | 2)
#define EMXX_CLK_ROT_P		(SMU_REG_SHIFT(SMU_ROTGCLKCTRL) | 1)
#define EMXX_CLK_AVE_A		(SMU_REG_SHIFT(SMU_AVEGCLKCTRL) | 4)
#define EMXX_CLK_AVE_C		(SMU_REG_SHIFT(SMU_AVEGCLKCTRL) | 2)
#define EMXX_CLK_AVE_P		(SMU_REG_SHIFT(SMU_AVEGCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLK_A3D_CORE	(SMU_REG_SHIFT(SMU_A3DGCLKCTRL) | 4)
#define EMXX_CLK_A3D_MEM	(SMU_REG_SHIFT(SMU_A3DGCLKCTRL) | 2)
#define EMXX_CLK_A3D_SYS	(SMU_REG_SHIFT(SMU_A3DGCLKCTRL) | 1)
#endif
#define EMXX_CLK_DTV_S		(SMU_REG_SHIFT(SMU_DTVGCLKCTRL) | 4)
#define EMXX_CLK_DTV		(SMU_REG_SHIFT(SMU_DTVGCLKCTRL) | 2)
#define EMXX_CLK_DTV_P		(SMU_REG_SHIFT(SMU_DTVGCLKCTRL) | 1)
#define EMXX_CLK_NTS		(SMU_REG_SHIFT(SMU_NTSGCLKCTRL) | 2)
#define EMXX_CLK_NTS_P		(SMU_REG_SHIFT(SMU_NTSGCLKCTRL) | 1)
#define EMXX_CLK_CAM_S		(SMU_REG_SHIFT(SMU_CAMGCLKCTRL) | 4)
#define EMXX_CLK_CAM		(SMU_REG_SHIFT(SMU_CAMGCLKCTRL) | 2)
#define EMXX_CLK_CAM_P		(SMU_REG_SHIFT(SMU_CAMGCLKCTRL) | 1)
#define EMXX_CLK_IRR_S		(SMU_REG_SHIFT(SMU_IRRGCLKCTRL) | 2)
#define EMXX_CLK_IRR_P		(SMU_REG_SHIFT(SMU_IRRGCLKCTRL) | 1)
#define EMXX_CLK_PWM1		(SMU_REG_SHIFT(SMU_PWMGCLKCTRL) | 4)
#define EMXX_CLK_PWM0		(SMU_REG_SHIFT(SMU_PWMGCLKCTRL) | 2)
#define EMXX_CLK_PWM_P		(SMU_REG_SHIFT(SMU_PWMGCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLK_HSI_S		(SMU_REG_SHIFT(SMU_HSIGCLKCTRL) | 8)
#define EMXX_CLK_HSI_T		(SMU_REG_SHIFT(SMU_HSIGCLKCTRL) | 4)
#define EMXX_CLK_HSI_H		(SMU_REG_SHIFT(SMU_HSIGCLKCTRL) | 2)
#define EMXX_CLK_HSI_P		(SMU_REG_SHIFT(SMU_HSIGCLKCTRL) | 1)
#endif
#define EMXX_CLK_IIC0_S		(SMU_REG_SHIFT(SMU_IIC0GCLKCTRL) | 2)
#define EMXX_CLK_IIC0		(SMU_REG_SHIFT(SMU_IIC0GCLKCTRL) | 1)
#define EMXX_CLK_IIC1_S		(SMU_REG_SHIFT(SMU_IIC1GCLKCTRL) | 2)
#define EMXX_CLK_IIC1		(SMU_REG_SHIFT(SMU_IIC1GCLKCTRL) | 1)
#define EMXX_CLK_USB_PCI	(SMU_REG_SHIFT(SMU_USBGCLKCTRL) | 8)
#define EMXX_CLK_USB1		(SMU_REG_SHIFT(SMU_USBGCLKCTRL) | 2)
#define EMXX_CLK_USB0		(SMU_REG_SHIFT(SMU_USBGCLKCTRL) | 1)
#define EMXX_CLK_USIA_S0_H	(SMU_REG_SHIFT(SMU_USIAS0GCLKCTRL) | 4)
#define EMXX_CLK_USIA_S0_S	(SMU_REG_SHIFT(SMU_USIAS0GCLKCTRL) | 2)
#define EMXX_CLK_USIA_S0_P	(SMU_REG_SHIFT(SMU_USIAS0GCLKCTRL) | 1)
#define EMXX_CLK_USIA_S1_H	(SMU_REG_SHIFT(SMU_USIAS1GCLKCTRL) | 4)
#define EMXX_CLK_USIA_S1_S	(SMU_REG_SHIFT(SMU_USIAS1GCLKCTRL) | 2)
#define EMXX_CLK_USIA_S1_P	(SMU_REG_SHIFT(SMU_USIAS1GCLKCTRL) | 1)
#define EMXX_CLK_USIA_U0_S	(SMU_REG_SHIFT(SMU_USIAU0GCLKCTRL) | 2)
#define EMXX_CLK_USIA_U0_P	(SMU_REG_SHIFT(SMU_USIAU0GCLKCTRL) | 1)
#define EMXX_CLK_USIB_S2_H	(SMU_REG_SHIFT(SMU_USIBS2GCLKCTRL) | 4)
#define EMXX_CLK_USIB_S2_S	(SMU_REG_SHIFT(SMU_USIBS2GCLKCTRL) | 2)
#define EMXX_CLK_USIB_S2_P	(SMU_REG_SHIFT(SMU_USIBS2GCLKCTRL) | 1)
#define EMXX_CLK_USIB_S3_H	(SMU_REG_SHIFT(SMU_USIBS3GCLKCTRL) | 4)
#define EMXX_CLK_USIB_S3_S	(SMU_REG_SHIFT(SMU_USIBS3GCLKCTRL) | 2)
#define EMXX_CLK_USIB_S3_P	(SMU_REG_SHIFT(SMU_USIBS3GCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLK_USIB_S4_H	(SMU_REG_SHIFT(SMU_USIBS4GCLKCTRL) | 4)
#define EMXX_CLK_USIB_S4_S	(SMU_REG_SHIFT(SMU_USIBS4GCLKCTRL) | 2)
#define EMXX_CLK_USIB_S4_P	(SMU_REG_SHIFT(SMU_USIBS4GCLKCTRL) | 1)
#define EMXX_CLK_USIB_S5_H	(SMU_REG_SHIFT(SMU_USIBS5GCLKCTRL) | 4)
#define EMXX_CLK_USIB_S5_S	(SMU_REG_SHIFT(SMU_USIBS5GCLKCTRL) | 2)
#define EMXX_CLK_USIB_S5_P	(SMU_REG_SHIFT(SMU_USIBS5GCLKCTRL) | 1)
#endif
#define EMXX_CLK_USIB_U1_S	(SMU_REG_SHIFT(SMU_USIBU1GCLKCTRL) | 2)
#define EMXX_CLK_USIB_U1_P	(SMU_REG_SHIFT(SMU_USIBU1GCLKCTRL) | 1)
#define EMXX_CLK_USIB_U2_S	(SMU_REG_SHIFT(SMU_USIBU2GCLKCTRL) | 2)
#define EMXX_CLK_USIB_U2_P	(SMU_REG_SHIFT(SMU_USIBU2GCLKCTRL) | 1)
#define EMXX_CLK_USIB_U3_S	(SMU_REG_SHIFT(SMU_USIBU3GCLKCTRL) | 2)
#define EMXX_CLK_USIB_U3_P	(SMU_REG_SHIFT(SMU_USIBU3GCLKCTRL) | 1)
#define EMXX_CLK_SDIO0_H	(SMU_REG_SHIFT(SMU_SDIO0GCLKCTRL) | 4)
#define EMXX_CLK_SDIO0_S	(SMU_REG_SHIFT(SMU_SDIO0GCLKCTRL) | 2)
#define EMXX_CLK_SDIO0		(SMU_REG_SHIFT(SMU_SDIO0GCLKCTRL) | 1)
#define EMXX_CLK_SDIO1_H	(SMU_REG_SHIFT(SMU_SDIO1GCLKCTRL) | 4)
#define EMXX_CLK_SDIO1_S	(SMU_REG_SHIFT(SMU_SDIO1GCLKCTRL) | 2)
#define EMXX_CLK_SDIO1		(SMU_REG_SHIFT(SMU_SDIO1GCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLK_SDIO2_H	(SMU_REG_SHIFT(SMU_SDIO2GCLKCTRL) | 4)
#define EMXX_CLK_SDIO2_S	(SMU_REG_SHIFT(SMU_SDIO2GCLKCTRL) | 2)
#define EMXX_CLK_SDIO2		(SMU_REG_SHIFT(SMU_SDIO2GCLKCTRL) | 1)
#endif
#define EMXX_CLK_SDC_H		(SMU_REG_SHIFT(SMU_SDCGCLKCTRL) | 4)
#define EMXX_CLK_SDC		(SMU_REG_SHIFT(SMU_SDCGCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMGR
#define EMXX_CLK_USIB_U4_S	(SMU_REG_SHIFT(SMU_USIBU4GCLKCTRL) | 2)
#define EMXX_CLK_USIB_U4_P	(SMU_REG_SHIFT(SMU_USIBU4GCLKCTRL) | 1)
#endif
#define EMXX_CLK_CFI		(SMU_REG_SHIFT(SMU_CFIGCLKCTRL) | 4)
#define EMXX_CLK_CFI_H		(SMU_REG_SHIFT(SMU_CFIGCLKCTRL) | 2)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLK_CFI_S		(SMU_REG_SHIFT(SMU_CFIGCLKCTRL) | 1)
#endif
#define EMXX_CLK_MSP_R		(SMU_REG_SHIFT(SMU_MSPGCLKCTRL) | 4)
#define EMXX_CLK_MSP_S		(SMU_REG_SHIFT(SMU_MSPGCLKCTRL) | 2)
#define EMXX_CLK_MSP_P		(SMU_REG_SHIFT(SMU_MSPGCLKCTRL) | 1)
#define EMXX_CLK_TI0		(SMU_REG_SHIFT(SMU_TI0GCLKCTRL) | 1)
#define EMXX_CLK_TI1		(SMU_REG_SHIFT(SMU_TI1GCLKCTRL) | 1)
#define EMXX_CLK_TI2		(SMU_REG_SHIFT(SMU_TI2GCLKCTRL) | 1)
#define EMXX_CLK_TI3		(SMU_REG_SHIFT(SMU_TI3GCLKCTRL) | 1)
#define EMXX_CLK_TG0		(SMU_REG_SHIFT(SMU_TG0GCLKCTRL) | 1)
#define EMXX_CLK_TG1		(SMU_REG_SHIFT(SMU_TG1GCLKCTRL) | 1)
#define EMXX_CLK_TG2		(SMU_REG_SHIFT(SMU_TG2GCLKCTRL) | 1)
#define EMXX_CLK_TG3		(SMU_REG_SHIFT(SMU_TG3GCLKCTRL) | 1)
#define EMXX_CLK_TG4		(SMU_REG_SHIFT(SMU_TG4GCLKCTRL) | 1)
#define EMXX_CLK_TG5		(SMU_REG_SHIFT(SMU_TG5GCLKCTRL) | 1)
#define EMXX_CLK_TW0		(SMU_REG_SHIFT(SMU_TW0GCLKCTRL) | 1)
#define EMXX_CLK_TW1		(SMU_REG_SHIFT(SMU_TW1GCLKCTRL) | 1)
#define EMXX_CLK_TW2		(SMU_REG_SHIFT(SMU_TW2GCLKCTRL) | 1)
#define EMXX_CLK_TW3		(SMU_REG_SHIFT(SMU_TW3GCLKCTRL) | 1)
#define EMXX_CLK_TIM_P		(SMU_REG_SHIFT(SMU_TIMGCLKCTRL) | 1)
#define EMXX_CLK_STI_S		(SMU_REG_SHIFT(SMU_STIGCLKCTRL) | 2)
#define EMXX_CLK_STI_P		(SMU_REG_SHIFT(SMU_STIGCLKCTRL) | 1)
#define EMXX_CLK_CRP		(SMU_REG_SHIFT(SMU_CRPGCLKCTRL) | 1)
#define EMXX_CLK_AFS_32K	(SMU_REG_SHIFT(SMU_AFSGCLKCTRL) | 2)
#define EMXX_CLK_AFS_P		(SMU_REG_SHIFT(SMU_AFSGCLKCTRL) | 1)
#define EMXX_CLK_MMM_P		(SMU_REG_SHIFT(SMU_MMMGCLKCTRL) | 1)
#define EMXX_CLK_REF		(SMU_REG_SHIFT(SMU_REFGCLKCTRL) | 1)
#define EMXX_CLK_TW4		(SMU_REG_SHIFT(SMU_TW4GCLKCTRL) | 1)
#define EMXX_CLK_PDMA		(SMU_REG_SHIFT(SMU_PDMAGCLKCTRL) | 1)
#ifdef CONFIG_MACH_EMGR
#define EMXX_CLK_A2D_VG		(SMU_REG_SHIFT(SMU_A2DGCLKCTRL) | 2)
#define EMXX_CLK_A2D_SYS	(SMU_REG_SHIFT(SMU_A2DGCLKCTRL) | 1)
#define EMXX_CLK_USIB_U5_S	(SMU_REG_SHIFT(SMU_USIBU5GCLKCTRL) | 2)
#define EMXX_CLK_USIB_U5_P	(SMU_REG_SHIFT(SMU_USIBU5GCLKCTRL) | 1)
#endif

/* Reset */
#define EMXX_RST_DCV		(SMU_REG_SHIFT(SMU_DSP_RSTCTRL) | 8)
#define EMXX_RST_DSP_S		(SMU_REG_SHIFT(SMU_DSP_RSTCTRL) | 4)
#define EMXX_RST_DSP_A		(SMU_REG_SHIFT(SMU_DSP_RSTCTRL) | 2)
#define EMXX_RST_DSP_I		(SMU_REG_SHIFT(SMU_DSP_RSTCTRL) | 1)
#define EMXX_RST_GIO		(SMU_REG_SHIFT(SMU_GIO_RSTCTRL) | 1)
#define EMXX_RST_INTA		(SMU_REG_SHIFT(SMU_INTA_RSTCTRL) | 1)
#define EMXX_RST_CHG		(SMU_REG_SHIFT(SMU_CHG_RSTCTRL) | 1)
#define EMXX_RST_CHG1		(SMU_REG_SHIFT(SMU_CHG1_RSTCTRL) | 1)
#define EMXX_RST_P2M		(SMU_REG_SHIFT(SMU_P2M_RSTCTRL) | 1)
#define EMXX_RST_M2P		(SMU_REG_SHIFT(SMU_M2P_RSTCTRL) | 1)
#define EMXX_RST_M2M		(SMU_REG_SHIFT(SMU_M2M_RSTCTRL) | 1)
#define EMXX_RST_PMU		(SMU_REG_SHIFT(SMU_PMU_RSTCTRL) | 1)
#define EMXX_RST_SRC		(SMU_REG_SHIFT(SMU_SRC_RSTCTRL) | 1)
#define EMXX_RST_LCD_A		(SMU_REG_SHIFT(SMU_LCD_RSTCTRL) | 2)
#define EMXX_RST_LCD		(SMU_REG_SHIFT(SMU_LCD_RSTCTRL) | 1)
#define EMXX_RST_IMC		(SMU_REG_SHIFT(SMU_IMC_RSTCTRL) | 1)
#define EMXX_RST_IMCW		(SMU_REG_SHIFT(SMU_IMCW_RSTCTRL) | 1)
#define EMXX_RST_SIZ		(SMU_REG_SHIFT(SMU_SIZ_RSTCTRL) | 1)
#define EMXX_RST_ROT		(SMU_REG_SHIFT(SMU_ROT_RSTCTRL) | 1)
#define EMXX_RST_AVE_C		(SMU_REG_SHIFT(SMU_AVE_RSTCTRL) | 4)
#define EMXX_RST_AVE_A		(SMU_REG_SHIFT(SMU_AVE_RSTCTRL) | 2)
#define EMXX_RST_AVE_P		(SMU_REG_SHIFT(SMU_AVE_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_A3D		(SMU_REG_SHIFT(SMU_A3D_RSTCTRL) | 1)
#endif
#define EMXX_RST_DTV		(SMU_REG_SHIFT(SMU_DTV_RSTCTRL) | 1)
#define EMXX_RST_NTS		(SMU_REG_SHIFT(SMU_NTS_RSTCTRL) | 1)
#define EMXX_RST_CAM		(SMU_REG_SHIFT(SMU_CAM_RSTCTRL) | 1)
#define EMXX_RST_IRR		(SMU_REG_SHIFT(SMU_IRR_RSTCTRL) | 1)
#define EMXX_RST_PWM		(SMU_REG_SHIFT(SMU_PWM_RSTCTRL) | 1)
#define EMXX_RST_USIA_S0_A	(SMU_REG_SHIFT(SMU_USIAS0_RSTCTRL) | 2)
#define EMXX_RST_USIA_S0_S	(SMU_REG_SHIFT(SMU_USIAS0_RSTCTRL) | 1)
#define EMXX_RST_USIA_S1_A	(SMU_REG_SHIFT(SMU_USIAS1_RSTCTRL) | 2)
#define EMXX_RST_USIA_S1_S	(SMU_REG_SHIFT(SMU_USIAS1_RSTCTRL) | 1)
#define EMXX_RST_USIA_U0_A	(SMU_REG_SHIFT(SMU_USIAU0_RSTCTRL) | 2)
#define EMXX_RST_USIB_S2_A	(SMU_REG_SHIFT(SMU_USIBS2_RSTCTRL) | 2)
#define EMXX_RST_USIB_S2_S	(SMU_REG_SHIFT(SMU_USIBS2_RSTCTRL) | 1)
#define EMXX_RST_USIB_S3_A	(SMU_REG_SHIFT(SMU_USIBS3_RSTCTRL) | 2)
#define EMXX_RST_USIB_S3_S	(SMU_REG_SHIFT(SMU_USIBS3_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_USIB_S4_A	(SMU_REG_SHIFT(SMU_USIBS4_RSTCTRL) | 2)
#define EMXX_RST_USIB_S4_S	(SMU_REG_SHIFT(SMU_USIBS4_RSTCTRL) | 1)
#define EMXX_RST_USIB_S5_A	(SMU_REG_SHIFT(SMU_USIBS5_RSTCTRL) | 2)
#define EMXX_RST_USIB_S5_S	(SMU_REG_SHIFT(SMU_USIBS5_RSTCTRL) | 1)
#endif
#define EMXX_RST_USIB_U1_A	(SMU_REG_SHIFT(SMU_USIBU1_RSTCTRL) | 2)
#define EMXX_RST_USIB_U2_A	(SMU_REG_SHIFT(SMU_USIBU2_RSTCTRL) | 2)
#define EMXX_RST_USIB_U3_A	(SMU_REG_SHIFT(SMU_USIBU3_RSTCTRL) | 2)
#define EMXX_RST_SDIO0		(SMU_REG_SHIFT(SMU_SDIO0_RSTCTRL) | 1)
#define EMXX_RST_SDIO1		(SMU_REG_SHIFT(SMU_SDIO1_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_SDIO2		(SMU_REG_SHIFT(SMU_SDIO2_RSTCTRL) | 1)
#endif
#define EMXX_RST_SDC		(SMU_REG_SHIFT(SMU_SDC_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMGR
#define EMXX_RST_USIB_U4_A	(SMU_REG_SHIFT(SMU_USIBU4_RSTCTRL) | 2)
#endif
#define EMXX_RST_CFI		(SMU_REG_SHIFT(SMU_CFI_RSTCTRL) | 1)
#define EMXX_RST_MSP		(SMU_REG_SHIFT(SMU_MSP_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_HSI		(SMU_REG_SHIFT(SMU_HSI_RSTCTRL) | 1)
#endif
#define EMXX_RST_IIC0		(SMU_REG_SHIFT(SMU_IIC0_RSTCTRL) | 1)
#define EMXX_RST_IIC1		(SMU_REG_SHIFT(SMU_IIC1_RSTCTRL) | 1)
#define EMXX_RST_USB0		(SMU_REG_SHIFT(SMU_USB0_RSTCTRL) | 1)
#define EMXX_RST_USB1		(SMU_REG_SHIFT(SMU_USB1_RSTCTRL) | 1)
#define EMXX_RST_TI0		(SMU_REG_SHIFT(SMU_TI0_RSTCTRL) | 1)
#define EMXX_RST_TI1		(SMU_REG_SHIFT(SMU_TI1_RSTCTRL) | 1)
#define EMXX_RST_TI2		(SMU_REG_SHIFT(SMU_TI2_RSTCTRL) | 1)
#define EMXX_RST_TI3		(SMU_REG_SHIFT(SMU_TI3_RSTCTRL) | 1)
#define EMXX_RST_TW0		(SMU_REG_SHIFT(SMU_TW0_RSTCTRL) | 1)
#define EMXX_RST_TW1		(SMU_REG_SHIFT(SMU_TW1_RSTCTRL) | 1)
#define EMXX_RST_TW2		(SMU_REG_SHIFT(SMU_TW2_RSTCTRL) | 1)
#define EMXX_RST_TW3		(SMU_REG_SHIFT(SMU_TW3_RSTCTRL) | 1)
#define EMXX_RST_TG0		(SMU_REG_SHIFT(SMU_TG0_RSTCTRL) | 1)
#define EMXX_RST_TG1		(SMU_REG_SHIFT(SMU_TG1_RSTCTRL) | 1)
#define EMXX_RST_TG2		(SMU_REG_SHIFT(SMU_TG2_RSTCTRL) | 1)
#define EMXX_RST_TG3		(SMU_REG_SHIFT(SMU_TG3_RSTCTRL) | 1)
#define EMXX_RST_TG4		(SMU_REG_SHIFT(SMU_TG4_RSTCTRL) | 1)
#define EMXX_RST_TG5		(SMU_REG_SHIFT(SMU_TG5_RSTCTRL) | 1)
#define EMXX_RST_STI		(SMU_REG_SHIFT(SMU_STI_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_CRP		(SMU_REG_SHIFT(SMU_CRP_RSTCTRL) | 1)
#endif
#define EMXX_RST_AFS		(SMU_REG_SHIFT(SMU_AFS_RSTCTRL) | 1)
#define EMXX_RST_MMM		(SMU_REG_SHIFT(SMU_MMM_RSTCTRL) | 1)
#define EMXX_RST_TW4		(SMU_REG_SHIFT(SMU_TW4_RSTCTRL) | 1)
#define EMXX_RST_PDMA		(SMU_REG_SHIFT(SMU_PDMA_RSTCTRL) | 1)
#ifdef CONFIG_MACH_EMGR
#define EMXX_RST_A2D_PPF	(SMU_REG_SHIFT(SMU_A2D_RSTCTRL) | 4)
#define EMXX_RST_A2D_VG		(SMU_REG_SHIFT(SMU_A2D_RSTCTRL) | 2)
#define EMXX_RST_A2D		(SMU_REG_SHIFT(SMU_A2D_RSTCTRL) | 1)
#define EMXX_RST_USIB_U5_A	(SMU_REG_SHIFT(SMU_USIBU5_RSTCTRL) | 2)
#endif
#define EMXX_RST_DSP_SAFE_S	(SMU_REG_SHIFT(SMU_DSP_SAFE_RESET) | 4)
#define EMXX_RST_DSP_SAFE_A	(SMU_REG_SHIFT(SMU_DSP_SAFE_RESET) | 2)
#define EMXX_RST_DSP_SAFE_I	(SMU_REG_SHIFT(SMU_DSP_SAFE_RESET) | 1)
#define EMXX_RST_USB0_SAFE	(SMU_REG_SHIFT(SMU_USB0_SAFE_RESET) | 1)
#define EMXX_RST_USB1_SAFE	(SMU_REG_SHIFT(SMU_USB1_SAFE_RESET) | 1)
#define EMXX_RST_DTV_SAFE	(SMU_REG_SHIFT(SMU_DTV_SAFE_RESET) | 1)
#define EMXX_RST_CFI_SAFE	(SMU_REG_SHIFT(SMU_CFI_SAFE_RESET) | 1)
#define EMXX_RST_SDC_SAFE	(SMU_REG_SHIFT(SMU_SDC_SAFE_RESET) | 1)
#define EMXX_RST_SDIO0_SAFE	(SMU_REG_SHIFT(SMU_SDIO0_SAFE_RESET) | 1)
#define EMXX_RST_SDIO1_SAFE	(SMU_REG_SHIFT(SMU_SDIO1_SAFE_RESET) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_SDIO2_SAFE	(SMU_REG_SHIFT(SMU_SDIO2_SAFE_RESET) | 1)
#define EMXX_RST_USIA_SAFE	(SMU_REG_SHIFT(SMU_USIA_SAFE_RESET) | 1)
#define EMXX_RST_USIB_SAFE	(SMU_REG_SHIFT(SMU_USIB_SAFE_RESET) | 1)
#define EMXX_RST_HSI_SAFE	(SMU_REG_SHIFT(SMU_HSI_SAFE_RESET) | 1)
#endif
#define EMXX_RST_CAM_SAFE	(SMU_REG_SHIFT(SMU_CAM_SAFE_RESET) | 1)
#ifdef CONFIG_MACH_EMEV
#define EMXX_RST_A3D_SAFE	(SMU_REG_SHIFT(SMU_A3D_SAFE_RESET) | 1)
#endif
#define EMXX_RST_AVE_SAFE	(SMU_REG_SHIFT(SMU_AVE_SAFE_RESET) | 1)
#define EMXX_RST_SIZ_SAFE	(SMU_REG_SHIFT(SMU_SIZ_SAFE_RESET) | 1)
#define EMXX_RST_ROT_SAFE	(SMU_REG_SHIFT(SMU_ROT_SAFE_RESET) | 1)
#define EMXX_RST_IMC_SAFE	(SMU_REG_SHIFT(SMU_IMC_SAFE_RESET) | 1)
#define EMXX_RST_IMCW_SAFE	(SMU_REG_SHIFT(SMU_IMCW_SAFE_RESET) | 1)
#define EMXX_RST_M2M_SAFE	(SMU_REG_SHIFT(SMU_M2M_SAFE_RESET) | 1)
#define EMXX_RST_M2P_SAFE	(SMU_REG_SHIFT(SMU_M2P_SAFE_RESET) | 1)
#define EMXX_RST_P2M_SAFE	(SMU_REG_SHIFT(SMU_P2M_SAFE_RESET) | 1)
#ifdef CONFIG_MACH_EMGR
#define EMXX_RST_A2D_SAFE_PPF	(SMU_REG_SHIFT(SMU_A2D_SAFE_RESET) | 4)
#define EMXX_RST_A2D_SAFE_VG	(SMU_REG_SHIFT(SMU_A2D_SAFE_RESET) | 2)
#define EMXX_RST_A2D_SAFE	(SMU_REG_SHIFT(SMU_A2D_SAFE_RESET) | 1)
#endif

/* Clock control */
#define SMU_AHBCLKCTRL0_GROUP	(0 << 16)
#define SMU_AHBCLKCTRL1_GROUP	(1 << 16)
#define SMU_AHBCLKCTRL2_GROUP	(2 << 16)
#define SMU_AHBCLKCTRL3_GROUP	(3 << 16)
#define SMU_APBCLKCTRL0_GROUP	(4 << 16)
#define SMU_APBCLKCTRL1_GROUP	(5 << 16)
#define SMU_APBCLKCTRL2_GROUP	(6 << 16)
#define SMU_CLKCTRL_GROUP	(7 << 16)
#define SMU_AVECLKCTRL_GROUP	(8 << 16)

#define EMXX_CLKCTRL_MEMC		(SMU_AHBCLKCTRL0_GROUP | 25)
#define EMXX_CLKCTRL_SRC		(SMU_AHBCLKCTRL0_GROUP | 20)
#define EMXX_CLKCTRL_M2M		(SMU_AHBCLKCTRL0_GROUP | 18)
#define EMXX_CLKCTRL_M2P		(SMU_AHBCLKCTRL0_GROUP | 17)
#define EMXX_CLKCTRL_P2M		(SMU_AHBCLKCTRL0_GROUP | 16)
#define EMXX_CLKCTRL_DSP		(SMU_AHBCLKCTRL0_GROUP | 6)
#define EMXX_CLKCTRL_DSP_A		(SMU_AHBCLKCTRL0_GROUP | 5)
#define EMXX_CLKCTRL_DCV		(SMU_AHBCLKCTRL0_GROUP | 4)
#define EMXX_CLKCTRL_CPU		(SMU_AHBCLKCTRL0_GROUP | 0)

#define EMXX_CLKCTRL_CAM		(SMU_AHBCLKCTRL1_GROUP | 24)
#define EMXX_CLKCTRL_NTS		(SMU_AHBCLKCTRL1_GROUP | 21)
#define EMXX_CLKCTRL_DTV		(SMU_AHBCLKCTRL1_GROUP | 20)
#ifdef CONFIG_MACH_EMGR
#define EMXX_CLKCTRL_A2DSYS		(SMU_AHBCLKCTRL1_GROUP | 19)
#define EMXX_CLKCTRL_A2DVG		(SMU_AHBCLKCTRL1_GROUP | 18)
#endif
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_A3DSYS		(SMU_AHBCLKCTRL1_GROUP | 17)
#define EMXX_CLKCTRL_A3DMEM		(SMU_AHBCLKCTRL1_GROUP | 16)
#endif
#define EMXX_CLKCTRL_ROT		(SMU_AHBCLKCTRL1_GROUP | 9)
#define EMXX_CLKCTRL_SIZ		(SMU_AHBCLKCTRL1_GROUP | 8)
#define EMXX_CLKCTRL_IMCW		(SMU_AHBCLKCTRL1_GROUP | 5)
#define EMXX_CLKCTRL_IMC		(SMU_AHBCLKCTRL1_GROUP | 4)
#define EMXX_CLKCTRL_LCDC		(SMU_AHBCLKCTRL1_GROUP | 1)
#define EMXX_CLKCTRL_LCD		(SMU_AHBCLKCTRL1_GROUP | 0)

#define EMXX_CLKCTRL_CFIH		(SMU_AHBCLKCTRL2_GROUP | 25)
#define EMXX_CLKCTRL_CFI		(SMU_AHBCLKCTRL2_GROUP | 24)
#define EMXX_CLKCTRL_SDCH		(SMU_AHBCLKCTRL2_GROUP | 21)
#define EMXX_CLKCTRL_SDC		(SMU_AHBCLKCTRL2_GROUP | 20)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_SDIO2H		(SMU_AHBCLKCTRL2_GROUP | 17)
#define EMXX_CLKCTRL_SDIO2		(SMU_AHBCLKCTRL2_GROUP | 16)
#endif
#define EMXX_CLKCTRL_SDIO1H		(SMU_AHBCLKCTRL2_GROUP | 13)
#define EMXX_CLKCTRL_SDIO1		(SMU_AHBCLKCTRL2_GROUP | 12)
#define EMXX_CLKCTRL_SDIO0H		(SMU_AHBCLKCTRL2_GROUP | 9)
#define EMXX_CLKCTRL_SDIO0		(SMU_AHBCLKCTRL2_GROUP | 8)
#define EMXX_CLKCTRL_USB1		(SMU_AHBCLKCTRL2_GROUP | 5)
#define EMXX_CLKCTRL_USB0		(SMU_AHBCLKCTRL2_GROUP | 4)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_HSIT		(SMU_AHBCLKCTRL2_GROUP | 1)
#define EMXX_CLKCTRL_HSI		(SMU_AHBCLKCTRL2_GROUP | 0)
#endif

#define EMXX_CLKCTRL_INTA		(SMU_AHBCLKCTRL3_GROUP | 28)
#define EMXX_CLKCTRL_PDMA		(SMU_AHBCLKCTRL3_GROUP | 20)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_USIBS5		(SMU_AHBCLKCTRL3_GROUP | 7)
#define EMXX_CLKCTRL_USIBS4		(SMU_AHBCLKCTRL3_GROUP | 6)
#endif
#define EMXX_CLKCTRL_USIBS3		(SMU_AHBCLKCTRL3_GROUP | 5)
#define EMXX_CLKCTRL_USIBS2		(SMU_AHBCLKCTRL3_GROUP | 4)
#define EMXX_CLKCTRL_USIAS1		(SMU_AHBCLKCTRL3_GROUP | 1)
#define EMXX_CLKCTRL_USIAS0		(SMU_AHBCLKCTRL3_GROUP | 0)

#define EMXX_CLKCTRL_MEMCPCLK		(SMU_APBCLKCTRL0_GROUP | 28)
#define EMXX_CLKCTRL_PMUPCLK		(SMU_APBCLKCTRL0_GROUP | 24)
#define EMXX_CLKCTRL_M2MPCLK		(SMU_APBCLKCTRL0_GROUP | 22)
#define EMXX_CLKCTRL_M2PPCLK		(SMU_APBCLKCTRL0_GROUP | 21)
#define EMXX_CLKCTRL_P2MPCLK		(SMU_APBCLKCTRL0_GROUP | 20)
#define EMXX_CLKCTRL_GIOPCLK		(SMU_APBCLKCTRL0_GROUP | 5)
#define EMXX_CLKCTRL_INTAPCLK		(SMU_APBCLKCTRL0_GROUP | 4)
#define EMXX_CLKCTRL_DCVPCLK		(SMU_APBCLKCTRL0_GROUP | 0)

#define EMXX_CLKCTRL_PWMPCLK		(SMU_APBCLKCTRL1_GROUP | 24)
#define EMXX_CLKCTRL_IRRPCLK		(SMU_APBCLKCTRL1_GROUP | 21)
#define EMXX_CLKCTRL_CAMPCLK		(SMU_APBCLKCTRL1_GROUP | 16)
#define EMXX_CLKCTRL_NTSPCLK		(SMU_APBCLKCTRL1_GROUP | 13)
#define EMXX_CLKCTRL_DTVPCLK		(SMU_APBCLKCTRL1_GROUP | 12)
#define EMXX_CLKCTRL_AVEPCLK		(SMU_APBCLKCTRL1_GROUP | 8)
#define EMXX_CLKCTRL_ROTPCLK		(SMU_APBCLKCTRL1_GROUP | 5)
#define EMXX_CLKCTRL_SIZPCLK		(SMU_APBCLKCTRL1_GROUP | 4)
#define EMXX_CLKCTRL_IMCWPCLK		(SMU_APBCLKCTRL1_GROUP | 2)
#define EMXX_CLKCTRL_IMCPCLK		(SMU_APBCLKCTRL1_GROUP | 1)
#define EMXX_CLKCTRL_LCDPCLK		(SMU_APBCLKCTRL1_GROUP | 0)

#define EMXX_CLKCTRL_MMMPCLK		(SMU_APBCLKCTRL2_GROUP | 28)
#define EMXX_CLKCTRL_AFSPCLK		(SMU_APBCLKCTRL2_GROUP | 24)
#define EMXX_CLKCTRL_STIPCLK		(SMU_APBCLKCTRL2_GROUP | 21)
#define EMXX_CLKCTRL_TIMPCLK		(SMU_APBCLKCTRL2_GROUP | 20)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_HSIPCLK		(SMU_APBCLKCTRL2_GROUP | 16)
#endif
#ifdef CONFIG_MACH_EMGR
#define EMXX_CLKCTRL_USIBU5PCLK		(SMU_APBCLKCTRL2_GROUP | 15)
#define EMXX_CLKCTRL_USIBU4PCLK		(SMU_APBCLKCTRL2_GROUP | 12)
#endif
#define EMXX_CLKCTRL_ICEPCLK		(SMU_APBCLKCTRL2_GROUP | 14)
#define EMXX_CLKCTRL_USIBU3PCLK		(SMU_APBCLKCTRL2_GROUP | 10)
#define EMXX_CLKCTRL_USIBU2PCLK		(SMU_APBCLKCTRL2_GROUP | 9)
#define EMXX_CLKCTRL_USIBU1PCLK		(SMU_APBCLKCTRL2_GROUP | 8)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_USIBS5PCLK		(SMU_APBCLKCTRL2_GROUP | 7)
#define EMXX_CLKCTRL_USIBS4PCLK		(SMU_APBCLKCTRL2_GROUP | 6)
#endif
#define EMXX_CLKCTRL_USIBS3PCLK		(SMU_APBCLKCTRL2_GROUP | 5)
#define EMXX_CLKCTRL_USIBS2PCLK		(SMU_APBCLKCTRL2_GROUP | 4)
#define EMXX_CLKCTRL_USIAU0PCLK		(SMU_APBCLKCTRL2_GROUP | 2)
#define EMXX_CLKCTRL_USIAS1PCLK		(SMU_APBCLKCTRL2_GROUP | 1)
#define EMXX_CLKCTRL_USIAS0PCLK		(SMU_APBCLKCTRL2_GROUP | 0)

#define EMXX_CLKCTRL_USBPCICLK		(SMU_CLKCTRL_GROUP | 21)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_HSISCLK		(SMU_CLKCTRL_GROUP | 16)
#define EMXX_CLKCTRL_USIBS5SCLK		(SMU_CLKCTRL_GROUP | 15)
#define EMXX_CLKCTRL_USIBS4SCLK		(SMU_CLKCTRL_GROUP | 14)
#endif
#define EMXX_CLKCTRL_USIBS3SCLK		(SMU_CLKCTRL_GROUP | 13)
#define EMXX_CLKCTRL_USIBS2SCLK		(SMU_CLKCTRL_GROUP | 12)
#define EMXX_CLKCTRL_USIAS1SCLK		(SMU_CLKCTRL_GROUP | 9)
#define EMXX_CLKCTRL_USIAS0SCLK		(SMU_CLKCTRL_GROUP | 8)
#define EMXX_CLKCTRL_MEMCCLK270		(SMU_CLKCTRL_GROUP | 5)
#define EMXX_CLKCTRL_AB0FLASHCLK	(SMU_CLKCTRL_GROUP | 4)
#ifdef CONFIG_MACH_EMEV
#define EMXX_CLKCTRL_A3DCORE		(SMU_CLKCTRL_GROUP | 2)
#endif
#define EMXX_CLKCTRL_QR			(SMU_CLKCTRL_GROUP | 0)

#define EMXX_CLKCTRL_AVEC		(SMU_AVECLKCTRL_GROUP | 4)
#define EMXX_CLKCTRL_AVEA		(SMU_AVECLKCTRL_GROUP | 0)


extern int emxx_open_clockgate(unsigned int clk);
extern int emxx_close_clockgate(unsigned int clk);
extern int emxx_get_clockgate(unsigned int clk);

extern int emxx_reset_device(unsigned int rst);
extern int emxx_unreset_device(unsigned int rst);
extern int emxx_get_reset(unsigned int rst);

extern int emxx_clkctrl_on(unsigned int dev);
extern int emxx_clkctrl_off(unsigned int dev);
extern int emxx_get_clkctrl(unsigned int dev);

#endif	/* __ASM_ARCH_PMU_H */
