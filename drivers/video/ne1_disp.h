/* 2009-07-22: File added and changed by Sony Corporation */
/*
 *  File Name		: drivers/video/ne1_disp.c
 *  Function		: display controller
 *  Release Version : Ver 1.00
 *  Release Date	: 2008/07/05
 *
 *  Copyright (C) NEC Electronics Corporation 2007
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

/*!
    @file   ne1disp.h
    @brief  Definition of Display's Register
*/

#define	NE1_DISPLAY_TRACE_ON			//! Display Trace On/Off
#define	NE1_DISPLAY_TRACE_MODE		(1)	//! NE1     Trace On/Off
#define	NE1_DISPLAY_TRACE_CURSOR_MODE	(0)	//! Cursor  Trace On/Off
#define	NE1_DISPLAY_TRACE_SURFACE_MODE	(1)	//! Surface Trace On/Off
#define	NE1_DISPLAY_TRACE_HAL_MODE	(1)	//! HAL     Trace On/Off

#define	NE1_USE_SURFACEHEAP				0

#define NE1_DISPLAY_REGBASE_VA		(0x9FD10000)
#define NE1_DISPLAY_FRAMEBASE_VA	(0x8C000000)

#define NE1_DISPLAY_REGBASE_PHYS	(0x18010000)
#define NE1_DISPLAY_FRAMEBASE_PHYS	(0x8C000000)

#define	NE1_DISPLAY_REGBASE		(NE1_DISPLAY_REGBASE_PHYS)
#define NE1_DISPLAY_REGSIZE		(0x4000)

#define	NE1_REG_BASEOFS(b, o)		(b + ((o) & 0x000FFFFF))
#define	NE1_REG_OFFSET(b, o)		(b + ((o) & 0x0000FFFF))

#define IF_CNT_OTA       (0x0000)  //! Select Signal
#define INT_STAT_VPC     (0x0008)  //! Interrupt Status
#define INT_CLR          (0x0010)  //! Clear Interrupt
#define INT_PERM         (0x0018)  //! Enable Interrupt
#define INT_LINE_ALH     (0x0020)  //! HBlank Line
#define INT_SEL_FCT      (0x0028)  //! Select Interrupt
#define DS_RSLT_SYNC     (0x0030)  //! Resolution
#define DS_PLSW_BP       (0x0038)  //! Pulse width
#define DS_SET           (0x0040)  //! Visible ON/OFF
#define DS_DITHER        (0x0048)  //! Dither coefficient
#define DS_BRIGHT        (0x0050)  //! Brightness
#define MF_ESWP_BEX      (0x0058)  //! Convert Endian, Extended internal-color
#define MF_PIXFMT_03     (0x0060)  //! Memory Frame Format
#define MF_PIXFMT_46     (0x0068)  //! Memory Frame Format
#define MF_SADR_01       (0x0070)  //! Start Address
#define MF_SADR_23       (0x0078)  //! Start Address
#define MF_SADR_45       (0x0080)  //! Start Address
#define MF_SADR_6        (0x0088)  //! Start Address
#define MF_SIZEH_03      (0x00A8)  //! horizon size
#define MF_SIZEH_46      (0x00B0)  //! horizon size
#define MF_SIZEV_03      (0x00B8)  //! Virtical Size
#define MF_SIZEV_46      (0x00C0)  //! Virtical Size
#define MF_DFSPX_03      (0x00C8)  //! Start Position X-coordinate
#define MF_DFSPX_46      (0x00D0)  //! Start Position X-coordinate
#define MF_DFSPY_03      (0x00D8)  //! Start Position Y-coordinate
#define MF_DFSPY_46      (0x00E0)  //! Start Position Y-coordinate
#define MF_DFSPYF        (0x00E8)  //! Start Position Y-coordinate decimal position
#define MF_DFSIZEH_03    (0x00F0)  //! Display Frame horizon size
#define MF_DFSIZEH_46    (0x00F8)  //! Display Frame horizon size
#define MF_DFSIZEV_03    (0x0100)  //! Display Frame Virtical Size
#define MF_DFSIZEV_46    (0x0108)  //! Display Frame Virtical Size
#define MF_BDC_01        (0x0110)  //! Set Border Color RGB
#define MF_BDC_23        (0x0118)  //! Set Border Color RGB
#define MF_BDC_45        (0x0120)  //! Set Border Color RGB
#define MF_BDC_6         (0x0128)  //! Set Border Color RGB
#define CP_SADR_01       (0x0090)  //! Start Address
#define CP_SADR_2        (0x0098)  //! Start Address
#define CP_RENEW         (0x00A0)  //! Update
#define DF_CNT_03        (0x0130)  //! Set Display Layer
#define DF_CNT_46        (0x0138)  //! Set Display Layer
#define DF_SPX_03        (0x0140)  //! Horizon Start Position X-coordinate
#define DF_SPX_46        (0x0148)  //! Horizon Start Position X-coordinate
#define DF_SPY_03        (0x0150)  //! Vertical Start Position Y-coordinate
#define DF_SPY_46        (0x0158)  //! Vertical Start Position Y-coordinate
#define DF_DSIZEH_03     (0x0160)  //! Horizon Display Size
#define DF_DSIZEH_46     (0x0168)  //! Horizon Display Size
#define DF_DSIZEV_02     (0x0170)  //! Vertical Display Size
#define DF_DSIZEV_4      (0x0178)  //! Vertical Display Size
#define DF_RESIZEH_03    (0x0180)  //! Horizon Scale
#define DF_RESIZEH_46    (0x0188)  //! Horizon Scale
#define DF_RESIZEV_02    (0x0190)  //! Vertical Scale
#define DF_RESIZEV_4     (0x0198)  //! Vertical Scale
#define DF_PTRANS_03     (0x01A0)  //! when ARGB1555, transparent rate for A=1bit
#define DF_PTRANS_46     (0x01A8)  //! when ARGB1555, transparent rate for A=1bit
#define DF_LTRANS_03     (0x01B0)  //! transparent Pixel ON/OFF
#define DF_LTRANS_46     (0x01B8)  //! transparent Layer ON/OFF
#define DF_CKEY_CNT      (0x01C0)  //! Control Color Key
#define DF_CKEY_01       (0x01C8)  //! Color Key
#define DF_CKEY_23       (0x01D0)  //! Color Key
#define DF_CKEY_45       (0x01D8)  //! Color Key
#define DF_CKEY_6        (0x01E0)  //! Color Key
#define DF_CKEY_PER_03   (0x01E8)  //! B/V accepted range
#define DF_CKEY_PER_46   (0x01F0)  //! B/V accepted range
#define DF_CONC_01       (0x01F8)  //! Display Frame Constant Color
#define DF_CONC_23       (0x0200)  //! Display Frame Constant Color
#define DF_CONC_45       (0x0208)  //! Display Frame Constant Color
#define DF_CONC_6        (0x0210)  //! Display Frame Constant Color
#define DF_BGC           (0x0218)  //! Display Frame BackGround Color
#define DF_CDC_PER       (0x0220)  //! Display Frame Detect Color
#define HC_CNT_SPXY      (0x0228)  //! Coutrol H/W Cursor
#define VB_RNMASK        (0x0230)  //! VBlank Update Mask
#define DS_GMCTR_0       (0x1000)  //! Gamma Correction
#define DS_GMCTR_1       (0x1008)  //! Gamma Correction
#define DS_GMCTR_2       (0x1010)  //! Gamma Correction
#define DS_GMCTR_3       (0x1018)  //! Gamma Correction
#define DS_GMCTR_4       (0x1020)  //! Gamma Correction
#define DS_GMCTR_5       (0x1028)  //! Gamma Correction
#define DS_GMCTR_6       (0x1030)  //! Gamma Correction
#define DS_GMCTR_7       (0x1038)  //! Gamma Correction
#define DS_GMCTR_8       (0x1040)  //! Gamma Correction
#define DS_GMCTR_9       (0x1048)  //! Gamma Correction
#define DS_GMCTR_10      (0x1050)  //! Gamma Correction
#define DS_GMCTR_11      (0x1058)  //! Gamma Correction
#define DS_GMCTR_12      (0x1060)  //! Gamma Correction
#define DS_GMCTR_13      (0x1068)  //! Gamma Correction
#define DS_GMCTR_14      (0x1070)  //! Gamma Correction
#define DS_GMCTR_15      (0x1078)  //! Gamma Correction
#define DS_GMCTR_16      (0x1080)  //! Gamma Correction
#define DS_GMCTR_17      (0x1088)  //! Gamma Correction
#define DS_GMCTR_18      (0x1090)  //! Gamma Correction
#define DS_GMCTR_19      (0x1098)  //! Gamma Correction
#define DS_GMCTR_20      (0x10A0)  //! Gamma Correction
#define DS_GMCTR_21      (0x10A8)  //! Gamma Correction
#define DS_GMCTR_22      (0x10B0)  //! Gamma Correction
#define DS_GMCTR_23      (0x10B8)  //! Gamma Correction
#define DS_GMCTR_24      (0x10C0)  //! Gamma Correction
#define DS_GMCTR_25      (0x10C8)  //! Gamma Correction
#define DS_GMCTR_26      (0x10D0)  //! Gamma Correction
#define DS_GMCTR_27      (0x10D8)  //! Gamma Correction
#define DS_GMCTR_28      (0x10E0)  //! Gamma Correction
#define DS_GMCTR_29      (0x10E8)  //! Gamma Correction
#define DS_GMCTR_30      (0x10F0)  //! Gamma Correction
#define DS_GMCTR_31      (0x10F8)  //! Gamma Correction
#define DS_GMCTG_0       (0x1100)  //! Gamma Correction
#define DS_GMCTG_1       (0x1108)  //! Gamma Correction
#define DS_GMCTG_2       (0x1110)  //! Gamma Correction
#define DS_GMCTG_3       (0x1118)  //! Gamma Correction
#define DS_GMCTG_4       (0x1120)  //! Gamma Correction
#define DS_GMCTG_5       (0x1128)  //! Gamma Correction
#define DS_GMCTG_6       (0x1130)  //! Gamma Correction
#define DS_GMCTG_7       (0x1138)  //! Gamma Correction
#define DS_GMCTG_8       (0x1140)  //! Gamma Correction
#define DS_GMCTG_9       (0x1148)  //! Gamma Correction
#define DS_GMCTG_10      (0x1150)  //! Gamma Correction
#define DS_GMCTG_11      (0x1158)  //! Gamma Correction
#define DS_GMCTG_12      (0x1160)  //! Gamma Correction
#define DS_GMCTG_13      (0x1168)  //! Gamma Correction
#define DS_GMCTG_14      (0x1170)  //! Gamma Correction
#define DS_GMCTG_15      (0x1178)  //! Gamma Correction
#define DS_GMCTG_16      (0x1180)  //! Gamma Correction
#define DS_GMCTG_17      (0x1188)  //! Gamma Correction
#define DS_GMCTG_18      (0x1190)  //! Gamma Correction
#define DS_GMCTG_19      (0x1198)  //! Gamma Correction
#define DS_GMCTG_20      (0x11A0)  //! Gamma Correction
#define DS_GMCTG_21      (0x11A8)  //! Gamma Correction
#define DS_GMCTG_22      (0x11B0)  //! Gamma Correction
#define DS_GMCTG_23      (0x11B8)  //! Gamma Correction
#define DS_GMCTG_24      (0x11C0)  //! Gamma Correction
#define DS_GMCTG_25      (0x11C8)  //! Gamma Correction
#define DS_GMCTG_26      (0x11D0)  //! Gamma Correction
#define DS_GMCTG_27      (0x11D8)  //! Gamma Correction
#define DS_GMCTG_28      (0x11E0)  //! Gamma Correction
#define DS_GMCTG_29      (0x11E8)  //! Gamma Correction
#define DS_GMCTG_30      (0x11F0)  //! Gamma Correction
#define DS_GMCTG_31      (0x11F8)  //! Gamma Correction
#define DS_GMCTB_0       (0x1200)  //! Gamma Correction
#define DS_GMCTB_1       (0x1208)  //! Gamma Correction
#define DS_GMCTB_2       (0x1210)  //! Gamma Correction
#define DS_GMCTB_3       (0x1218)  //! Gamma Correction
#define DS_GMCTB_4       (0x1220)  //! Gamma Correction
#define DS_GMCTB_5       (0x1228)  //! Gamma Correction
#define DS_GMCTB_6       (0x1230)  //! Gamma Correction
#define DS_GMCTB_7       (0x1238)  //! Gamma Correction
#define DS_GMCTB_8       (0x1240)  //! Gamma Correction
#define DS_GMCTB_9       (0x1248)  //! Gamma Correction
#define DS_GMCTB_10      (0x1250)  //! Gamma Correction
#define DS_GMCTB_11      (0x1258)  //! Gamma Correction
#define DS_GMCTB_12      (0x1260)  //! Gamma Correction
#define DS_GMCTB_13      (0x1268)  //! Gamma Correction
#define DS_GMCTB_14      (0x1270)  //! Gamma Correction
#define DS_GMCTB_15      (0x1278)  //! Gamma Correction
#define DS_GMCTB_16      (0x1280)  //! Gamma Correction
#define DS_GMCTB_17      (0x1288)  //! Gamma Correction
#define DS_GMCTB_18      (0x1290)  //! Gamma Correction
#define DS_GMCTB_19      (0x1298)  //! Gamma Correction
#define DS_GMCTB_20      (0x12A0)  //! Gamma Correction
#define DS_GMCTB_21      (0x12A8)  //! Gamma Correction
#define DS_GMCTB_22      (0x12B0)  //! Gamma Correction
#define DS_GMCTB_23      (0x12B8)  //! Gamma Correction
#define DS_GMCTB_24      (0x12C0)  //! Gamma Correction
#define DS_GMCTB_25      (0x12C8)  //! Gamma Correction
#define DS_GMCTB_26      (0x12D0)  //! Gamma Correction
#define DS_GMCTB_27      (0x12D8)  //! Gamma Correction
#define DS_GMCTB_28      (0x12E0)  //! Gamma Correction
#define DS_GMCTB_29      (0x12E8)  //! Gamma Correction
#define DS_GMCTB_30      (0x12F0)  //! Gamma Correction
#define DS_GMCTB_31      (0x12F8)  //! Gamma Correction
#define HC_DATFRM_0      (0x2000)  //! HW CursorData Frame
#define HC_CLUT_0        (0x3000)  //! HW Cursor Lookup table


/********************************************************************
 *    Macro for MASK
 ********************************************************************/
#define NE1_MASK_IF_CNT_OTA      0x00000000003303FFLL  //!< Contorl Signal
#define NE1_MASK_IC_SS           0x0000000000000001LL  //!< Sync Signal
#define NE1_MASK_IC_CS           0x0000000000000002LL  //!< Dot Clock Signal
#define NE1_MASK_IC_SC           0x0000000000000004LL  //!< Interrupt Status
#define NE1_MASK_IC_CP           0x0000000000000008LL  //!< Dot Clock polarity
#define NE1_MASK_IC_HP           0x0000000000000010LL  //!< HSYNC polarity
#define NE1_MASK_IC_EHP          0x0000000000000020LL  //!< Ext HSYNC polarity
#define NE1_MASK_IC_VP           0x0000000000000040LL  //!< VSYNC polarity
#define NE1_MASK_IC_EVP          0x0000000000000080LL  //!< HSYNC polarity
#define NE1_MASK_IC_DEP          0x0000000000000100LL  //!< DE Signal polarity
#define NE1_MASK_IC_CDP          0x0000000000000200LL  //!< CDE Signal polarity
#define NE1_MASK_IC_DD           0x0000000000030000LL  //!< Data Signal Delay
#define NE1_MASK_IC_CDD          0x0000000000300000LL  //!< CDE Signal Delay
#define NE1_MASK_INT_STAT_VPC    0x0000000000C3FF7FLL  //!< Interrupt
#define NE1_MASK_IC_IS           0x000000000000007FLL  //!< Interrupt Status
#define NE1_MASK_IC_VCNT         0x000000000003FF00LL  //!< Interrupt Count V
#define NE1_MASK_IC_VSS          0x0000000000400000LL  //!< Interrupt VSYNC
#define NE1_MASK_IC_VBS          0x0000000000800000LL  //!< Interrupt VBlank
#define NE1_MASK_INT_CLR         0x000000000000007FLL  //!< Interrupt
#define NE1_MASK_IC_ICL          0x000000000000007FLL  //!< Interrupt Clear Interrupt
#define NE1_MASK_INT_PERM        0x000000000000007FLL  //!< Interrupt
#define NE1_MASK_IC_IP           0x000000000000007FLL  //!< Interrupt Enable Interrupt
#define NE1_MASK_INT_LINE_ALH    0x00000000000007FFLL  //!< Interrupt
#define NE1_MASK_IC_IL           0x00000000000003FFLL  //!< Interrupt HBlank Line
#define NE1_MASK_IC_AL_H         0x00000000000007FFLL  //!< Set Interrupt Almost h
#define NE1_MASK_INT_SEL_FCT     0x0037373737373737LL  //!< Interrupt
#define NE1_MASK_IC_ISEL0        0x0000000000000007LL  //!< Interrupt Select Interrupt0
#define NE1_MASK_IC_IF0          0x0000000000000030LL  //!< Interrupt factor0
#define NE1_MASK_IC_ISEL1        0x0000000000000700LL  //!< Interrupt Select Interrupt1
#define NE1_MASK_IC_IF1          0x0000000000003000LL  //!< Interrupt factor1
#define NE1_MASK_IC_ISEL2        0x0000000000070000LL  //!< Interrupt Select Interrupt2
#define NE1_MASK_IC_IF2          0x0000000000300000LL  //!< Interrupt factor2
#define NE1_MASK_IC_ISEL3        0x0000000007000000LL  //!< Interrupt Select Interrupt3
#define NE1_MASK_IC_IF3          0x0000000030000000LL  //!< Interrupt factor3
#define NE1_MASK_IC_ISEL4        0x0000000700000000LL  //!< Interrupt Select Interrupt4
#define NE1_MASK_IC_IF4          0x0000003000000000LL  //!< Interrupt factor4
#define NE1_MASK_IC_ISEL5        0x0000070000000000LL  //!< Interrupt Select Interrupt5
#define NE1_MASK_IC_IF5          0x0000300000000000LL  //!< Interrupt factor5
#define NE1_MASK_IC_ISEL6        0x0007000000000000LL  //!< Interrupt Select Interrupt6
#define NE1_MASK_IC_IF6          0x0030000000000000LL  //!< Interrupt factor6
#define NE1_MASK_DS_RSLT_SYNC    0x03FF07FF03FF07FFLL  //!< Display
#define NE1_MASK_DS_HR           0x00000000000007FFLL  //!< Display Horizontal Resolution
#define NE1_MASK_DS_VR           0x0000000003FF0000LL  //!< Display Vertical Resolution
#define NE1_MASK_DS_TH           0x000007FF00000000LL  //!< Display Horizontal Cycle
#define NE1_MASK_DS_TV           0x03FF000000000000LL  //!< Display Vertical Cycle
#define NE1_MASK_DS_PLSW_BP      0x03FF07FF03FF07FFLL  //!< Display
#define NE1_MASK_DS_THP          0x00000000000007FFLL  //!< Display Horizontal Pulse width
#define NE1_MASK_DS_TVP          0x0000000003FF0000LL  //!< Display Vertical Pulse width
#define NE1_MASK_DS_THB          0x000007FF00000000LL  //!< Display Horizontal Back porch
#define NE1_MASK_DS_TVB          0x03FF000000000000LL  //!< Display Vertical Back porch
#define NE1_MASK_DS_SET          0x00000000000080F1LL  //!< Display
#define NE1_MASK_DS_OC           0x0000000000000001LL  //!< Display ON/OFF
#define NE1_MASK_DS_DS           0x0000000000000030LL  //!< Display Dither ON/OFF,Output bit
#define NE1_MASK_DS_DM_H         0x0000000000000040LL  //!< Display Moving Horizontal Dither Pattern ON/OFF
#define NE1_MASK_DS_DM_V         0x0000000000000080LL  //!< Display Moving Vertical Dither Pattern ON/OFF
#define NE1_MASK_DS_GS           0x0000000000008000LL  //!< Display Gamma Correction ON/OFF
#define NE1_MASK_DS_DITHER       0xFFFFFFFFFFFFFFFFLL  //!< Display
#define NE1_MASK_DS_DC0          0x000000000000000FLL  //!< Display Dither coefficient 0
#define NE1_MASK_DS_DC1          0x00000000000000F0LL  //!< Display Dither coefficient 1
#define NE1_MASK_DS_DC2          0x0000000000000F00LL  //!< Display Dither coefficient 2
#define NE1_MASK_DS_DC3          0x000000000000F000LL  //!< Display Dither coefficient 3
#define NE1_MASK_DS_DC4          0x00000000000F0000LL  //!< Display Dither coefficient 4
#define NE1_MASK_DS_DC5          0x0000000000F00000LL  //!< Display Dither coefficient 5
#define NE1_MASK_DS_DC6          0x000000000F000000LL  //!< Display Dither coefficient 6
#define NE1_MASK_DS_DC7          0x00000000F0000000LL  //!< Display Dither coefficient 7
#define NE1_MASK_DS_DC8          0x0000000F00000000LL  //!< Display Dither coefficient 8
#define NE1_MASK_DS_DC9          0x000000F000000000LL  //!< Display Dither coefficient 9
#define NE1_MASK_DS_DC10         0x00000F0000000000LL  //!< Display Dither coefficient 10
#define NE1_MASK_DS_DC11         0x0000F00000000000LL  //!< Display Dither coefficient 11
#define NE1_MASK_DS_DC12         0x000F000000000000LL  //!< Display Dither coefficient 12
#define NE1_MASK_DS_DC13         0x00F0000000000000LL  //!< Display Dither coefficient 13
#define NE1_MASK_DS_DC14         0x0F00000000000000LL  //!< Display Dither coefficient 14
#define NE1_MASK_DS_DC15         0xF000000000000000LL  //!< Display Dither coefficient 15
#define NE1_MASK_DS_BRIGHT       0x00000000000000FFLL  //!< Display
#define NE1_MASK_DS_BC           0x00000000000000FFLL  //!< Display Brightness
#define NE1_MASK_MF_ESWP_BEX     0x00000000037FFFFFLL  //!< Memory Frame 0
#define NE1_MASK_MF_ES0          0x0000000000000003LL  //!< Memory Frame 0 Convert Endian
#define NE1_MASK_MF_ES1          0x000000000000000CLL  //!< Memory Frame 1 Convert Endian
#define NE1_MASK_MF_ES2          0x0000000000000030LL  //!< Memory Frame 2 Convert Endian
#define NE1_MASK_MF_ES3          0x00000000000000C0LL  //!< Memory Frame 3 Convert Endian
#define NE1_MASK_MF_ES4          0x0000000000000300LL  //!< Memory Frame 4 Convert Endian
#define NE1_MASK_MF_ES5          0x0000000000000C00LL  //!< Memory Frame 5 Convert Endian
#define NE1_MASK_MF_ES6          0x0000000000003000LL  //!< Memory Frame 6 Convert Endian
#define NE1_MASK_MF_BE0          0x0000000000010000LL  //!< Memory Frame 0 Extend Internal Color
#define NE1_MASK_MF_BE1          0x0000000000030000LL  //!< Memory Frame 1 Extend Internal Color
#define NE1_MASK_MF_BE2          0x0000000000040000LL  //!< Memory Frame 2 Extend Internal Color
#define NE1_MASK_MF_BE3          0x0000000000080000LL  //!< Memory Frame 3 Extend Internal Color
#define NE1_MASK_MF_BE4          0x0000000000100000LL  //!< Memory Frame 4 Extend Internal Color
#define NE1_MASK_MF_BE5          0x0000000000300000LL  //!< Memory Frame 5 Extend Internal Color
#define NE1_MASK_MF_BE6          0x0000000000400000LL  //!< Memory Frame 6 Extend Internal Color
#define NE1_MASK_MF_LBS          0x0000000003000000LL  //!< Set Layer BUF
#define NE1_MASK_MF_PIXFMT_03    0x951F9D1F951F9D1FLL  //!< Memory Frame 0
#define NE1_MASK_MF_PF0          0x000000000000000FLL  //!< Memory Frame 0 Format
#define NE1_MASK_MF_YC0          0x0000000000000010LL  //!< Memory Frame 0 Convert YC
#define NE1_MASK_MF_PS0          0x0000000000000100LL  //!< Memory Frame 0 Graphic/Natural
#define NE1_MASK_MF_MS_H0        0x0000000000000400LL  //!< Memory Frame 0 horizontal display magnification
#define NE1_MASK_MF_MS_V0        0x0000000000000800LL  //!< Memory Frame 0 vertical display magnification
#define NE1_MASK_MF_RL0          0x0000000000001000LL  //!< Memory Frame 0 Out of Frame
#define NE1_MASK_MF_AF0          0x0000000000008000LL  //!< Memory Frame 0 when argb1555, Select Transparent format
#define NE1_MASK_MF_PF1          0x00000000000F0000LL  //!< Memory Frame 1 Format
#define NE1_MASK_MF_YC1          0x0000000000100000LL  //!< Memory Frame 1 Convert YC
#define NE1_MASK_MF_PS1          0x0000000001000000LL  //!< Memory Frame 1 Graphic/Natural
#define NE1_MASK_MF_MS_H1        0x0000000004000000LL  //!< Memory Frame 1 horizontal display magnification
#define NE1_MASK_MF_RL1          0x0000000010000000LL  //!< Memory Frame 1 Out of Frame
#define NE1_MASK_MF_AF1          0x0000000080000000LL  //!< Memory Frame 1 when argb1555, Select Transparent format
#define NE1_MASK_MF_PF2          0x0000000F00000000LL  //!< Memory Frame 2 Format
#define NE1_MASK_MF_YC2          0x0000001000000000LL  //!< Memory Frame 2 Convert YC
#define NE1_MASK_MF_PS2          0x0000010000000000LL  //!< Memory Frame 2 Graphic/Natural
#define NE1_MASK_MF_MS_H2        0x0000040000000000LL  //!< Memory Frame 2 horizontal display magnification
#define NE1_MASK_MF_MS_V2        0x0000080000000000LL  //!< Memory Frame 2 vertical display magnification
#define NE1_MASK_MF_RL2          0x0000100000000000LL  //!< Memory Frame 2 Out of Frame
#define NE1_MASK_MF_AF2          0x0000800000000000LL  //!< Memory Frame 2 when argb1555, Select Transparent format
#define NE1_MASK_MF_PF3          0x000F000000000000LL  //!< Memory Frame 3 Format
#define NE1_MASK_MF_YC3          0x0010000000000000LL  //!< Memory Frame 3 Convert YC
#define NE1_MASK_MF_PS3          0x0100000000000000LL  //!< Memory Frame 3 Graphic/Natural
#define NE1_MASK_MF_MS_H3        0x0400000000000000LL  //!< Memory Frame 3 horizontal display magnification
#define NE1_MASK_MF_RL3          0x1000000000000000LL  //!< Memory Frame 3 Out of Frame
#define NE1_MASK_MF_AF3          0x8000000000000000LL  //!< Memory Frame 3 when argb1555, Select Transparent format
#define NE1_MASK_MF_PIXFMT_46    0x0000951F951F9D1FLL  //!< Memory Frame 4
#define NE1_MASK_MF_PF4          0x000000000000000FLL  //!< Memory Frame 4 Format
#define NE1_MASK_MF_YC4          0x0000000000000010LL  //!< Memory Frame 4 Convert YC
#define NE1_MASK_MF_PS4          0x0000000000000100LL  //!< Memory Frame 4 Graphic/Natural
#define NE1_MASK_MF_MS_H4        0x0000000000000400LL  //!< Memory Frame 4 horizontal display magnification
#define NE1_MASK_MF_MS_V4        0x0000000000000800LL  //!< Memory Frame 4 vertical display magnification
#define NE1_MASK_MF_RL4          0x0000000000001000LL  //!< Memory Frame 4 Out of Frame
#define NE1_MASK_MF_AF4          0x0000000000008000LL  //!< Memory Frame 4 when argb1555, Select Transparent format
#define NE1_MASK_MF_PF5          0x00000000000F0000LL  //!< Memory Frame 5 Format
#define NE1_MASK_MF_YC5          0x0000000000100000LL  //!< Memory Frame 5 Convert YC
#define NE1_MASK_MF_PS5          0x0000000001000000LL  //!< Memory Frame 5 Graphic/Natural
#define NE1_MASK_MF_MS_H5        0x0000000004000000LL  //!< Memory Frame 5 horizontal display magnification
#define NE1_MASK_MF_RL5          0x0000000010000000LL  //!< Memory Frame 5 Out of Frame
#define NE1_MASK_MF_AF5          0x0000000080000000LL  //!< Memory Frame 5 when argb1555, Select Transparent format
#define NE1_MASK_MF_PF6          0x0000000F00000000LL  //!< Memory Frame 6 Format
#define NE1_MASK_MF_YC6          0x0000001000000000LL  //!< Memory Frame 6 Convert YC
#define NE1_MASK_MF_PS6          0x0000010000000000LL  //!< Memory Frame 6 Graphic/Natural
#define NE1_MASK_MF_MS_H6        0x0000040000000000LL  //!< Memory Frame 6 horizontal display magnification
#define NE1_MASK_MF_RL6          0x0000100000000000LL  //!< Memory Frame 6 Out of Frame
#define NE1_MASK_MF_AF6          0x0000800000000000LL  //!< Memory Frame 6 when argb1555, Select Transparent format
#define NE1_MASK_MF_SADR_01      0xFFFFFFF0FFFFFFF0LL  //!< Memory Frame 0
#define NE1_MASK_MF_SA0          0x00000000FFFFFFF0LL  //!< Memory Frame 0 Start Address
#define NE1_MASK_MF_SA1          0xFFFFFFF000000000LL  //!< Memory Frame 1 Start Address
#define NE1_MASK_MF_SADR_23      0xFFFFFFF0FFFFFFF0LL  //!< Memory Frame 2
#define NE1_MASK_MF_SA2          0x00000000FFFFFFF0LL  //!< Memory Frame 2 Start Address
#define NE1_MASK_MF_SA3          0xFFFFFFF000000000LL  //!< Memory Frame 3 Start Address
#define NE1_MASK_MF_SADR_45      0xFFFFFFF0FFFFFFF0LL  //!< Memory Frame 4
#define NE1_MASK_MF_SA4          0x00000000FFFFFFF0LL  //!< Memory Frame 4 Start Address
#define NE1_MASK_MF_SA5          0xFFFFFFF000000000LL  //!< Memory Frame 5 Start Address
#define NE1_MASK_MF_SADR_6       0x00000000FFFFFFF0LL  //!< Memory Frame 6
#define NE1_MASK_MF_SA6          0x00000000FFFFFFF0LL  //!< Memory Frame 6 Start Address
#define NE1_MASK_MF_SIZEH_03     0x0FFF0FFF0FFF0FFFLL  //!< Memory Frame 0
#define NE1_MASK_MF_WS_H0        0x0000000000000FFFLL  //!< Memory Frame 0 horizon size
#define NE1_MASK_MF_WS_H1        0x000000000FFF0000LL  //!< Memory Frame 1 horizon size
#define NE1_MASK_MF_WS_H2        0x00000FFF00000000LL  //!< Memory Frame 2 horizon size
#define NE1_MASK_MF_WS_H3        0x0FFF000000000000LL  //!< Memory Frame 3 horizon size
#define NE1_MASK_MF_SIZEH_46     0x00000FFF0FFF0FFFLL  //!< Memory Frame 4
#define NE1_MASK_MF_WS_H4        0x0000000000000FFFLL  //!< Memory Frame 4 horizon size
#define NE1_MASK_MF_WS_H5        0x000000000FFF0000LL  //!< Memory Frame 5 horizon size
#define NE1_MASK_MF_WS_H6        0x00000FFF00000000LL  //!< Memory Frame 6 horizon size
#define NE1_MASK_MF_SIZEV_03     0x0FFF0FFF0FFF0FFFLL  //!< Memory Frame 0
#define NE1_MASK_MF_WS_V0        0x0000000000000FFFLL  //!< Memory Frame 0 Virtical Size
#define NE1_MASK_MF_WS_V1        0x000000000FFF0000LL  //!< Memory Frame 1 Virtical Size
#define NE1_MASK_MF_WS_V2        0x00000FFF00000000LL  //!< Memory Frame 2 Virtical Size
#define NE1_MASK_MF_WS_V3        0x0FFF000000000000LL  //!< Memory Frame 3 Virtical Size
#define NE1_MASK_MF_SIZEV_46     0x00000FFF0FFF0FFFLL  //!< Memory Frame 4
#define NE1_MASK_MF_WS_V4        0x0000000000000FFFLL  //!< Memory Frame 4 Virtical Size
#define NE1_MASK_MF_WS_V5        0x000000000FFF0000LL  //!< Memory Frame 5 Virtical Size
#define NE1_MASK_MF_WS_V6        0x00000FFF00000000LL  //!< Memory Frame 6 Virtical Size
#define NE1_MASK_MF_DFSPX_03     0x0FFF0FFF0FFF0FFFLL  //!< Memory Frame 0
#define NE1_MASK_MF_SP_X0        0x0000000000000FFFLL  //!< Memory Frame 0 Start Position X-coordinate
#define NE1_MASK_MF_SP_X1        0x000000000FFF0000LL  //!< Memory Frame 1 Start Position X-coordinate
#define NE1_MASK_MF_SP_X2        0x00000FFF00000000LL  //!< Memory Frame 2 Start Position X-coordinate
#define NE1_MASK_MF_SP_X3        0x0FFF000000000000LL  //!< Memory Frame 3 Start Position X-coordinate
#define NE1_MASK_MF_DFSPX_46     0x00000FFF0FFF0FFFLL  //!< Memory Frame 4
#define NE1_MASK_MF_SP_X4        0x0000000000000FFFLL  //!< Memory Frame 4 Start Position X-coordinate
#define NE1_MASK_MF_SP_X5        0x000000000FFF0000LL  //!< Memory Frame 5 Start Position X-coordinate
#define NE1_MASK_MF_SP_X6        0x00000FFF00000000LL  //!< Memory Frame 6 Start Position X-coordinate
#define NE1_MASK_MF_DFSPY_03     0x0FFF0FFF0FFF0FFFLL  //!< Memory Frame 0
#define NE1_MASK_MF_SP_Y0        0x0000000000000FFFLL  //!< Memory Frame 0 Start Position Y-coordinate
#define NE1_MASK_MF_SP_Y1        0x000000000FFF0000LL  //!< Memory Frame 1 Start Position Y-coordinate
#define NE1_MASK_MF_SP_Y2        0x00000FFF00000000LL  //!< Memory Frame 2 Start Position Y-coordinate
#define NE1_MASK_MF_SP_Y3        0x0FFF000000000000LL  //!< Memory Frame 3 Start Position Y-coordinate
#define NE1_MASK_MF_DFSPY_46     0x00000FFF0FFF0FFFLL  //!< Memory Frame 4
#define NE1_MASK_MF_SP_Y4        0x0000000000000FFFLL  //!< Memory Frame 4 Start Position Y-coordinate
#define NE1_MASK_MF_SP_Y5        0x000000000FFF0000LL  //!< Memory Frame 5 Start Position Y-coordinate
#define NE1_MASK_MF_SP_Y6        0x00000FFF00000000LL  //!< Memory Frame 6 Start Position Y-coordinate
#define NE1_MASK_MF_DFSPYF       0x0000000000000015LL  //!< Memory Frame 0
#define NE1_MASK_MF_SP_YF0       0x0000000000000001LL  //!< Memory Frame 0 Start Position Y-coordinate decimal position
#define NE1_MASK_MF_SP_YF2       0x0000000000000004LL  //!< Memory Frame 2 Start Position Y-coordinate decimal position
#define NE1_MASK_MF_SP_YF4       0x0000000000000010LL  //!< Memory Frame 4 Start Position Y-coordinate decimal position
#define NE1_MASK_MF_DFSIZEH_03   0x07FF07FF07FF07FFLL  //!< Memory Frame 0
#define NE1_MASK_MF_DS_H0        0x00000000000007FFLL  //!< Memory Frame 0 Display Frame horizon size
#define NE1_MASK_MF_DS_H1        0x0000000007FF0000LL  //!< Memory Frame 1 Display Frame horizon size
#define NE1_MASK_MF_DS_H2        0x000007FF00000000LL  //!< Memory Frame 2 Display Frame horizon size
#define NE1_MASK_MF_DS_H3        0x07FF000000000000LL  //!< Memory Frame 3 Display Frame horizon size
#define NE1_MASK_MF_DFSIZEH_46   0x000007FF07FF07FFLL  //!< Memory Frame 4
#define NE1_MASK_MF_DS_H4        0x00000000000007FFLL  //!< Memory Frame 4 Display Frame horizon size
#define NE1_MASK_MF_DS_H5        0x0000000007FF0000LL  //!< Memory Frame 5 Display Frame horizon size
#define NE1_MASK_MF_DS_H6        0x000007FF00000000LL  //!< Memory Frame 6 Display Frame horizon size
#define NE1_MASK_MF_DFSIZEV_03   0x03FF03FF03FF03FFLL  //!< Memory Frame 0
#define NE1_MASK_MF_DS_V0        0x00000000000003FFLL  //!< Memory Frame 0 Display Frame Virtical Size
#define NE1_MASK_MF_DS_V1        0x0000000003FF0000LL  //!< Memory Frame 1 Display Frame Virtical Size
#define NE1_MASK_MF_DS_V2        0x000003FF00000000LL  //!< Memory Frame 2 Display Frame Virtical Size
#define NE1_MASK_MF_DS_V3        0x03FF000000000000LL  //!< Memory Frame 3 Display Frame Virtical Size
#define NE1_MASK_MF_DFSIZEV_46   0x000003FF03FF03FFLL  //!< Memory Frame 4
#define NE1_MASK_MF_DS_V4        0x00000000000003FFLL  //!< Memory Frame 4 Display Frame Virtical Size
#define NE1_MASK_MF_DS_V5        0x0000000003FF0000LL  //!< Memory Frame 5 Display Frame Virtical Size
#define NE1_MASK_MF_DS_V6        0x000003FF00000000LL  //!< Memory Frame 6 Display Frame Virtical Size
#define NE1_MASK_MF_BDC_01       0x10FFFFFF10FFFFFFLL  //!< Layer0
#define NE1_MASK_MF_BC0          0x0000000000FFFFFFLL  //!< Layer0 Border Color RGB
#define NE1_MASK_MF_BC_S0        0x0000000010000000LL  //!< Layer0 Border Color RGB
#define NE1_MASK_MF_BC1          0x00FFFFFF00000000LL  //!< Layer1 Border Color RGB
#define NE1_MASK_MF_BC_S1        0x1000000000000000LL  //!< Layer1 Border Color RGB
#define NE1_MASK_MF_BDC_23       0x10FFFFFF10FFFFFFLL  //!< Layer2
#define NE1_MASK_MF_BC2          0x0000000000FFFFFFLL  //!< Layer2 Border Color RGB
#define NE1_MASK_MF_BC_S2        0x0000000010000000LL  //!< Layer2 Border Color RGB
#define NE1_MASK_MF_BC3          0x00FFFFFF00000000LL  //!< Layer3 Border Color RGB
#define NE1_MASK_MF_BC_S3        0x1000000000000000LL  //!< Layer3 Border Color RGB
#define NE1_MASK_MF_BDC_45       0x10FFFFFF10FFFFFFLL  //!< Layer4
#define NE1_MASK_MF_BC4          0x0000000000FFFFFFLL  //!< Layer4 Border Color RGB
#define NE1_MASK_MF_BC_S4        0x0000000010000000LL  //!< Layer4 Border Color RGB
#define NE1_MASK_MF_BC5          0x00FFFFFF00000000LL  //!< Layer5 Border Color RGB
#define NE1_MASK_MF_BC_S5        0x1000000000000000LL  //!< Layer5 Border Color RGB
#define NE1_MASK_MF_BDC_6        0x0000000010FFFFFFLL  //!< Layer6
#define NE1_MASK_MF_BC6          0x0000000000FFFFFFLL  //!< Layer6 Border Color RGB
#define NE1_MASK_MF_BC_S6        0x0000000010000000LL  //!< Layer6 Border Color RGB
#define NE1_MASK_CP_SADR_01      0xFFFFFFF0FFFFFFF0LL  //!< Color Palette 0
#define NE1_MASK_CP_SA0          0x00000000FFFFFFF0LL  //!< Color Palette 0 Start Address
#define NE1_MASK_CP_SA1          0xFFFFFFF000000000LL  //!< Color Palette 1 Start Address
#define NE1_MASK_CP_SADR_2       0x00000000FFFFFFF0LL  //!< Color Palette 2
#define NE1_MASK_CP_SA2          0x00000000FFFFFFF0LL  //!< Color Palette 2 Start Address
#define NE1_MASK_CP_RENEW        0x0000000000000111LL  //!< Color Palette 0
#define NE1_MASK_CP_RN0          0x0000000000000001LL  //!< Color Palette 0 Update
#define NE1_MASK_CP_RN1          0x0000000000000010LL  //!< Color Palette 1 Update
#define NE1_MASK_CP_RN2          0x0000000000000100LL  //!< Color Palette 2 Update
#define NE1_MASK_DF_CNT_03       0x1107110711071107LL  //!< Display Frame 0 Display Control
#define NE1_MASK_DF_DC0          0x0000000000000007LL  //!< Display Frame 0 Display Control Layer
#define NE1_MASK_DF_VS0          0x0000000000000100LL  //!< Display Frame 0 Display Control VRAM/Register
#define NE1_MASK_DF_LC0          0x0000000000001000LL  //!< Display Frame 0 Display Control Layer0 ON/OFF
#define NE1_MASK_DF_DC1          0x0000000000070000LL  //!< Display Frame 1 Display Control Layer
#define NE1_MASK_DF_VS1          0x0000000001000000LL  //!< Display Frame 1 Display Control VRAM/Register
#define NE1_MASK_DF_LC1          0x0000000010000000LL  //!< Display Frame 1 Display Control Layer1 ON/OFF
#define NE1_MASK_DF_DC2          0x0000000700000000LL  //!< Display Frame 2 Display Control Layer
#define NE1_MASK_DF_VS2          0x0000010000000000LL  //!< Display Frame 2 Display Control VRAM/Register
#define NE1_MASK_DF_LC2          0x0000100000000000LL  //!< Display Frame 2 Display Control Layer2 ON/OFF
#define NE1_MASK_DF_DC3          0x0007000000000000LL  //!< Display Frame 3 Display Control Layer
#define NE1_MASK_DF_VS3          0x0100000000000000LL  //!< Display Frame 3 Display Control VRAM/Register
#define NE1_MASK_DF_LC3          0x1000000000000000LL  //!< Display Frame 3 Display Control Layer3 ON/OFF
#define NE1_MASK_DF_CNT_46       0x0000110711071107LL  //!< Display Frame 4 Display Control
#define NE1_MASK_DF_DC4          0x0000000000000007LL  //!< Display Frame 4 Display Control Layer
#define NE1_MASK_DF_VS4          0x0000000000000100LL  //!< Display Frame 4 Display Control VRAM/Register
#define NE1_MASK_DF_LC4          0x0000000000001000LL  //!< Display Frame 4 Display Control Layer4 ON/OFF
#define NE1_MASK_DF_DC5          0x0000000000070000LL  //!< Display Frame 5 Display Control Layer
#define NE1_MASK_DF_VS5          0x0000000001000000LL  //!< Display Frame 5 Display Control VRAM/Register
#define NE1_MASK_DF_LC5          0x0000000010000000LL  //!< Display Frame 5 Display Control Layer5 ON/OFF
#define NE1_MASK_DF_DC6          0x0000000700000000LL  //!< Display Frame 6 Display Control Layer
#define NE1_MASK_DF_VS6          0x0000010000000000LL  //!< Display Frame 6 Display Control VRAM/Register
#define NE1_MASK_DF_LC6          0x0000100000000000LL  //!< Display Frame 6 Display Control Layer6 ON/OFF
#define NE1_MASK_DF_SPX_03       0x0FFF0FFF0FFF0FFFLL  //!< Display Frame 0
#define NE1_MASK_DF_SP_X0        0x0000000000000FFFLL  //!< Display Frame 0 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SP_X1        0x000000000FFF0000LL  //!< Display Frame 1 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SP_X2        0x00000FFF00000000LL  //!< Display Frame 2 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SP_X3        0x0FFF000000000000LL  //!< Display Frame 3 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SPX_46       0x00000FFF00000FFFLL  //!< Display Frame 4
#define NE1_MASK_DF_SP_X4        0x0000000000000FFFLL  //!< Display Frame 4 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SP_X5        0x000000000FFF0000LL  //!< Display Frame 5 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SP_X6        0x00000FFF00000000LL  //!< Display Frame 6 Horizon Start Position X-coordinate
#define NE1_MASK_DF_SPY_03       0x0FFF0FFF0FFF0FFFLL  //!< Display Frame 0
#define NE1_MASK_DF_SP_Y0        0x0000000000000FFFLL  //!< Display Frame 0 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SP_Y1        0x000000000FFF0000LL  //!< Display Frame 1 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SP_Y2        0x00000FFF00000000LL  //!< Display Frame 2 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SP_Y3        0x0FFF000000000000LL  //!< Display Frame 3 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SPY_46       0x00000FFF0FFF0FFFLL  //!< Display Frame 4
#define NE1_MASK_DF_SP_Y4        0x0000000000000FFFLL  //!< Display Frame 4 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SP_Y5        0x000000000FFF0000LL  //!< Display Frame 5 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_SP_Y6        0x00000FFF00000000LL  //!< Display Frame 6 Vertical Start Position Y-coordinate
#define NE1_MASK_DF_DSIZEH_03    0x07FF07FF07FF07FFLL  //!< Display Frame 0
#define NE1_MASK_DF_DS_H0        0x00000000000007FFLL  //!< Display Frame 0 Horizon Display Size
#define NE1_MASK_DF_DS_H1        0x0000000007FF0000LL  //!< Display Frame 1 Horizon Display Size
#define NE1_MASK_DF_DS_H2        0x000007FF00000000LL  //!< Display Frame 2 Horizon Display Size
#define NE1_MASK_DF_DS_H3        0x07FF000000000000LL  //!< Display Frame 3 Horizon Display Size
#define NE1_MASK_DF_DSIZEH_46    0x000007FF07FF07FFLL  //!< Display Frame 4
#define NE1_MASK_DF_DS_H4        0x00000000000007FFLL  //!< Display Frame 4 Horizon Display Size
#define NE1_MASK_DF_DS_H5        0x0000000007FF0000LL  //!< Display Frame 5 Horizon Display Size
#define NE1_MASK_DF_DS_H6        0x000007FF00000000LL  //!< Display Frame 6 Horizon Display Size
#define NE1_MASK_DF_DSIZEV_02    0x000003FF000003FFLL  //!< Display Frame 0
#define NE1_MASK_DF_DS_V0        0x00000000000003FFLL  //!< Display Frame 0 Vertical Display Size
#define NE1_MASK_DF_DS_V2        0x000003FF00000000LL  //!< Display Frame 2 Vertical Display Size
#define NE1_MASK_DF_DSIZEV_4     0x00000000000003FFLL  //!< Display Frame 4
#define NE1_MASK_DF_DS_V4        0x00000000000003FFLL  //!< Display Frame 4 Vertical Display Size
#define NE1_MASK_DF_RESIZEH_03   0x1FFF1FFF1FFF1FFFLL  //!< Display Frame 0
#define NE1_MASK_DF_RC_H0        0x0000000000001FFFLL  //!< Display Frame 0 Horizon Scale
#define NE1_MASK_DF_RC_H1        0x000000001FFF0000LL  //!< Display Frame 1 Horizon Scale
#define NE1_MASK_DF_RC_H2        0x00001FFF00000000LL  //!< Display Frame 2 Horizon Scale
#define NE1_MASK_DF_RC_H3        0x1FFF000000000000LL  //!< Display Frame 3 Horizon Scale
#define NE1_MASK_DF_RESIZEH_46   0x00001FFF1FFF1FFFLL  //!< Display Frame 4
#define NE1_MASK_DF_RC_H4        0x0000000000001FFFLL  //!< Display Frame 4 Horizon Scale
#define NE1_MASK_DF_RC_H5        0x000000001FFF0000LL  //!< Display Frame 5 Horizon Scale
#define NE1_MASK_DF_RC_H6        0x00001FFF00000000LL  //!< Display Frame 6 Horizon Scale
#define NE1_MASK_DF_RESIZEV_02   0x00001FFF00001FFFLL  //!< Display Frame 0
#define NE1_MASK_DF_RC_V0        0x0000000000001FFFLL  //!< Display Frame 0 Vertical Scale
#define NE1_MASK_DF_RC_V2        0x00001FFF00000000LL  //!< Display Frame 2 Vertical Scale
#define NE1_MASK_DF_RESIZEV_4    0x0000000000001FFFLL  //!< Display Frame 4
#define NE1_MASK_DF_RC_V4        0x0000000000001FFFLL  //!< Display Frame 4 Vertical Scale
#define NE1_MASK_DF_PTRANS_03    0xFFFFFFFFFFFFFFFFLL  //!< Display Frame 0 Pixel Rate
#define NE1_MASK_DF_PT_R00       0x00000000000000FFLL  //!< Display Frame 0 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R10       0x000000000000FF00LL  //!< Display Frame 0 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PT_R01       0x0000000000FF0000LL  //!< Display Frame 1 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R11       0x00000000FF000000LL  //!< Display Frame 1 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PT_R02       0x000000FF00000000LL  //!< Display Frame 2 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R12       0x0000FF0000000000LL  //!< Display Frame 2 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PT_R03       0x00FF000000000000LL  //!< Display Frame 3 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R13       0xFF00000000000000LL  //!< Display Frame 3 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PTRANS_46    0x0000FFFFFFFFFFFFLL  //!< Display Frame 4 Pixel Rate
#define NE1_MASK_DF_PT_R04       0x00000000000000FFLL  //!< Display Frame 4 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R14       0x000000000000FF00LL  //!< Display Frame 4 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PT_R05       0x0000000000FF0000LL  //!< Display Frame 5 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R15       0x00000000FF000000LL  //!< Display Frame 5 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_PT_R06       0x000000FF00000000LL  //!< Display Frame 6 Pixel Rate when argb1555, Rate(A=0)
#define NE1_MASK_DF_PT_R16       0x0000FF0000000000LL  //!< Display Frame 6 Pixel Rate when argb1555, Rate(A=1)
#define NE1_MASK_DF_LTRANS_03    0xFF03FF03FF03FF03LL  //!< Display Frame 0 Transparent ControlLayer Rate
#define NE1_MASK_DF_LT_C0        0x0000000000000001LL  //!< Display Frame 0 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C0        0x0000000000000002LL  //!< Display Frame 0 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R0        0x000000000000FF00LL  //!< Display Frame 0 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LT_C1        0x0000000000010000LL  //!< Display Frame 1 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C1        0x0000000000020000LL  //!< Display Frame 1 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R1        0x00000000FF000000LL  //!< Display Frame 1 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LT_C2        0x0000000100000000LL  //!< Display Frame 2 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C2        0x0000000200000000LL  //!< Display Frame 2 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R2        0x0000FF0000000000LL  //!< Display Frame 2 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LT_C3        0x0001000000000000LL  //!< Display Frame 3 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C3        0x0002000000000000LL  //!< Display Frame 3 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R3        0xFF00000000000000LL  //!< Display Frame 3 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LTRANS_46    0x0000FF03FF03FF03LL  //!< Display Frame 4 Transparent ControlLayer Rate
#define NE1_MASK_DF_LT_C4        0x0000000000000001LL  //!< Display Frame 4 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C4        0x0000000000000002LL  //!< Display Frame 4 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R4        0x000000000000FF00LL  //!< Display Frame 4 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LT_C5        0x0000000000010000LL  //!< Display Frame 5 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C5        0x0000000000020000LL  //!< Display Frame 5 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R5        0x00000000FF000000LL  //!< Display Frame 5 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_LT_C6        0x0000000100000000LL  //!< Display Frame 6 Transparent ControlLayer Rate Layer ON/OFF
#define NE1_MASK_DF_PT_C6        0x0000000200000000LL  //!< Display Frame 6 Transparent ControlLayer Rate Pixel ON/OFF
#define NE1_MASK_DF_LT_R6        0x0000FF0000000000LL  //!< Display Frame 6 Transparent ControlLayer Rate Display Frame Layer Rate
#define NE1_MASK_DF_CKEY_CNT     0x00000000007F7F7FLL  //!< Display Frame 0 Color KeyControl
#define NE1_MASK_DF_CK_C0        0x0000000000000001LL  //!< Display Frame 0 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C1        0x0000000000000002LL  //!< Display Frame 1 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C2        0x0000000000000004LL  //!< Display Frame 2 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C3        0x0000000000000008LL  //!< Display Frame 3 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C4        0x0000000000000010LL  //!< Display Frame 4 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C5        0x0000000000000020LL  //!< Display Frame 5 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_C6        0x0000000000000040LL  //!< Display Frame 6 Color KeyControl Color Key ON/OFF
#define NE1_MASK_DF_CK_S0        0x0000000000000100LL  //!< Display Frame 0 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S1        0x0000000000000200LL  //!< Display Frame 1 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S2        0x0000000000000400LL  //!< Display Frame 2 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S3        0x0000000000000800LL  //!< Display Frame 3 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S4        0x0000000000001000LL  //!< Display Frame 4 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S5        0x0000000000002000LL  //!< Display Frame 5 Color KeyRGBFormat
#define NE1_MASK_DF_CK_S6        0x0000000000004000LL  //!< Display Frame 6 Color KeyRGBFormat
#define NE1_MASK_DF_CK_A0        0x0000000000010000LL  //!< Display Frame 0 Color Key Compare with A
#define NE1_MASK_DF_CK_A1        0x0000000000020000LL  //!< Display Frame 1 Color Key Compare with A
#define NE1_MASK_DF_CK_A2        0x0000000000040000LL  //!< Display Frame 2 Color Key Compare with A
#define NE1_MASK_DF_CK_A3        0x0000000000080000LL  //!< Display Frame 3 Color Key Compare with A
#define NE1_MASK_DF_CK_A4        0x0000000000100000LL  //!< Display Frame 4 Color Key Compare with A
#define NE1_MASK_DF_CK_A5        0x0000000000200000LL  //!< Display Frame 5 Color Key Compare with A
#define NE1_MASK_DF_CK_A6        0x0000000000400000LL  //!< Display Frame 6 Color Key Compare with A
#define NE1_MASK_DF_CKEY_01      0xFFFFFFFFFFFFFFFFLL  //!< Display Frame 0
#define NE1_MASK_DF_CK0          0x00000000FFFFFFFFLL  //!< Display Frame 0 Color Key
#define NE1_MASK_DF_CK1          0xFFFFFFFF00000000LL  //!< Display Frame 1 Color Key
#define NE1_MASK_DF_CKEY_23      0xFFFFFFFFFFFFFFFFLL  //!< Display Frame 2
#define NE1_MASK_DF_CK2          0x00000000FFFFFFFFLL  //!< Display Frame 2 Color Key
#define NE1_MASK_DF_CK3          0xFFFFFFFF00000000LL  //!< Display Frame 3 Color Key
#define NE1_MASK_DF_CKEY_45      0xFFFFFFFFFFFFFFFFLL  //!< Display Frame 4
#define NE1_MASK_DF_CK4          0x00000000FFFFFFFFLL  //!< Display Frame 4 Color Key
#define NE1_MASK_DF_CK5          0xFFFFFFFF00000000LL  //!< Display Frame 5 Color Key
#define NE1_MASK_DF_CKEY_6       0x00000000FFFFFFFFLL  //!< Display Frame 6
#define NE1_MASK_DF_CK6          0x00000000FFFFFFFFLL  //!< Display Frame 6 Color Key
#define NE1_MASK_DF_CKEY_PER_03  0xFFFFFFFFFFFFFFFFLL  //!< Display Frame 0 Color KeyControl
#define NE1_MASK_DF_CK_PR_BV0    0x000000000000000FLL  //!< Display Frame 0 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY0    0x00000000000000F0LL  //!< Display Frame 0 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU0    0x0000000000000F00LL  //!< Display Frame 0 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A0     0x000000000000F000LL  //!< Display Frame 0 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CK_PR_BV1    0x00000000000F0000LL  //!< Display Frame 1 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY1    0x0000000000F00000LL  //!< Display Frame 1 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU1    0x000000000F000000LL  //!< Display Frame 1 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A1     0x00000000F0000000LL  //!< Display Frame 1 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CK_PR_BV2    0x0000000F00000000LL  //!< Display Frame 2 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY2    0x000000F000000000LL  //!< Display Frame 2 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU2    0x00000F0000000000LL  //!< Display Frame 2 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A2     0x0000F00000000000LL  //!< Display Frame 2 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CK_PR_BV3    0x000F000000000000LL  //!< Display Frame 3 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY3    0x00F0000000000000LL  //!< Display Frame 3 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU3    0x0F00000000000000LL  //!< Display Frame 3 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A3     0xF000000000000000LL  //!< Display Frame 3 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CKEY_PER_46  0x0000FFFFFFFFFFFFLL  //!< Display Frame 4 Color KeyControl
#define NE1_MASK_DF_CK_PR_BV4    0x000000000000000FLL  //!< Display Frame 4 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY4    0x00000000000000F0LL  //!< Display Frame 4 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU4    0x0000000000000F00LL  //!< Display Frame 4 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A4     0x000000000000F000LL  //!< Display Frame 4 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CK_PR_BV5    0x00000000000F0000LL  //!< Display Frame 5 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY5    0x0000000000F00000LL  //!< Display Frame 5 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU5    0x000000000F000000LL  //!< Display Frame 5 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A5     0x00000000F0000000LL  //!< Display Frame 5 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CK_PR_BV6    0x0000000F00000000LL  //!< Display Frame 6 Color KeyControl B/Vaccepted range
#define NE1_MASK_DF_CK_PR_GY6    0x000000F000000000LL  //!< Display Frame 6 Color KeyControl G/Yaccepted range
#define NE1_MASK_DF_CK_PR_RU6    0x00000F0000000000LL  //!< Display Frame 6 Color KeyControl R/Uaccepted range
#define NE1_MASK_DF_CK_PR_A6     0x0000F00000000000LL  //!< Display Frame 6 Color KeyControl Aaccepted range
#define NE1_MASK_DF_CONC_01      0x00FFFFFF00FFFFFFLL  //!< Display Frame 0 Constant Color
#define NE1_MASK_DF_CC_B0        0x00000000000000FFLL  //!< Display Frame 0 Constant Color B
#define NE1_MASK_DF_CC_G0        0x000000000000FF00LL  //!< Display Frame 0 Constant Color G
#define NE1_MASK_DF_CC_R0        0x0000000000FF0000LL  //!< Display Frame 0 Constant Color R
#define NE1_MASK_DF_CC_B1        0x000000FF00000000LL  //!< Display Frame 1 Constant Color B
#define NE1_MASK_DF_CC_G1        0x0000FF0000000000LL  //!< Display Frame 1 Constant Color G
#define NE1_MASK_DF_CC_R1        0x00FF000000000000LL  //!< Display Frame 1 Constant Color R
#define NE1_MASK_DF_CONC_23      0x00FFFFFF00FFFFFFLL  //!< Display Frame 2 Constant Color
#define NE1_MASK_DF_CC_B2        0x00000000000000FFLL  //!< Display Frame 2 Constant Color B
#define NE1_MASK_DF_CC_G2        0x000000000000FF00LL  //!< Display Frame 2 Constant Color G
#define NE1_MASK_DF_CC_R2        0x0000000000FF0000LL  //!< Display Frame 2 Constant Color R
#define NE1_MASK_DF_CC_B3        0x000000FF00000000LL  //!< Display Frame 3 Constant Color B
#define NE1_MASK_DF_CC_G3        0x0000FF0000000000LL  //!< Display Frame 3 Constant Color G
#define NE1_MASK_DF_CC_R3        0x00FF000000000000LL  //!< Display Frame 3 Constant Color R
#define NE1_MASK_DF_CONC_45      0x00FFFFFF00FFFFFFLL  //!< Display Frame 4 Constant Color
#define NE1_MASK_DF_CC_B4        0x00000000000000FFLL  //!< Display Frame 4 Constant Color B
#define NE1_MASK_DF_CC_G4        0x000000000000FF00LL  //!< Display Frame 4 Constant Color G
#define NE1_MASK_DF_CC_R4        0x0000000000FF0000LL  //!< Display Frame 4 Constant Color R
#define NE1_MASK_DF_CC_B5        0x000000FF00000000LL  //!< Display Frame 5 Constant Color B
#define NE1_MASK_DF_CC_G5        0x0000FF0000000000LL  //!< Display Frame 5 Constant Color G
#define NE1_MASK_DF_CC_R5        0x00FF000000000000LL  //!< Display Frame 5 Constant Color R
#define NE1_MASK_DF_CONC_6       0x0000000000FFFFFFLL  //!< Display Frame 6 Constant Color
#define NE1_MASK_DF_CC_B6        0x00000000000000FFLL  //!< Display Frame 6 Constant Color B
#define NE1_MASK_DF_CC_G6        0x000000000000FF00LL  //!< Display Frame 6 Constant Color G
#define NE1_MASK_DF_CC_R6        0x0000000000FF0000LL  //!< Display Frame 6 Constant Color R
#define NE1_MASK_DF_BGC          0x0000000000FFFFFFLL  //!< Display Frame  BackGround Color
#define NE1_MASK_DF_BC_B         0x00000000000000FFLL  //!< Display Frame  BackGround Color  B
#define NE1_MASK_DF_BC_G         0x000000000000FF00LL  //!< Display Frame  BackGround Color  G
#define NE1_MASK_DF_BC_R         0x0000000000FF0000LL  //!< Display Frame  BackGround Color  R
#define NE1_MASK_DF_CDC_PER      0x80000FFF00FFFFFFLL  //!< Display Frame  Detect Color
#define NE1_MASK_DF_CD_B         0x00000000000000FFLL  //!< Display Frame  Detect Color B
#define NE1_MASK_DF_CD_G         0x000000000000FF00LL  //!< Display Frame  Detect Color G
#define NE1_MASK_DF_CD_R         0x0000000000FF0000LL  //!< Display Frame  Detect Color R
#define NE1_MASK_DF_CD_PR_B      0x0000000F00000000LL  //!< Display Frame  Detect Color B accepted range
#define NE1_MASK_DF_CD_PR_G      0x000000F000000000LL  //!< Display Frame  Detect Color G accepted range
#define NE1_MASK_DF_CD_PR_R      0x00000F0000000000LL  //!< Display Frame  Detect Color R accepted range
#define NE1_MASK_DF_CD_C         0x8000000000000000LL  //!< Display Frame  Detect Color ON/OFF
#define NE1_MASK_HC_CNT_SPXY     0x00000FFF0FFF0007LL  //!< HW CursorControl
#define NE1_MASK_HC_DS           0x0000000000000001LL  //!< HW CursorControl ON/OFF
#define NE1_MASK_HC_FS           0x0000000000000002LL  //!< HW CursorControl Select Cursor Frame
#define NE1_MASK_HC_LC           0x0000000000000004LL  //!< HW CursorControl Access Control
#define NE1_MASK_HC_SP_X         0x000000000FFF0000LL  //!< HW CursorControl Start Position X
#define NE1_MASK_HC_SP_Y         0x00000FFF00000000LL  //!< HW CursorControl Start Position Y
#define NE1_MASK_VB_RNMASK       0x013F3F3F3F3F3F3FLL  //!< VBlank Mask Memory Frame 0
#define NE1_MASK_MF_WS_M0        0x0000000000000001LL  //!< VBlank Mask Memory Frame 0 Size
#define NE1_MASK_MF_SP_M0        0x0000000000000002LL  //!< VBlank Mask Memory Frame 0 Start Position
#define NE1_MASK_MF_DS_M0        0x0000000000000004LL  //!< VBlank Mask Memory Frame 0 Display Frame Size
#define NE1_MASK_DF_SP_M0        0x0000000000000008LL  //!< VBlank Mask Display Frame 0 Start Position
#define NE1_MASK_DF_DS_M0        0x0000000000000010LL  //!< VBlank Mask Display Frame 0 Display Size
#define NE1_MASK_DF_RC_M0        0x0000000000000020LL  //!< VBlank Mask Display Frame 0 Scale
#define NE1_MASK_MF_WS_M1        0x0000000000000100LL  //!< VBlank Mask Memory Frame 1 Size
#define NE1_MASK_MF_SP_M1        0x0000000000000200LL  //!< VBlank Mask Memory Frame 1 Start Position
#define NE1_MASK_MF_DS_M1        0x0000000000000400LL  //!< VBlank Mask Memory Frame 1 Display Frame Size
#define NE1_MASK_DF_SP_M1        0x0000000000000800LL  //!< VBlank Mask Display Frame 1 Start Position
#define NE1_MASK_DF_DS_M1        0x0000000000001000LL  //!< VBlank Mask Display Frame 1 Display Size
#define NE1_MASK_DF_RC_M1        0x0000000000002000LL  //!< VBlank Mask Display Frame 1 Scale
#define NE1_MASK_MF_WS_M2        0x0000000000010000LL  //!< VBlank Mask Memory Frame 2 Size
#define NE1_MASK_MF_SP_M2        0x0000000000020000LL  //!< VBlank Mask Memory Frame 2 Start Position
#define NE1_MASK_MF_DS_M2        0x0000000000040000LL  //!< VBlank Mask Memory Frame 2 Display Frame Size
#define NE1_MASK_DF_SP_M2        0x0000000000080000LL  //!< VBlank Mask Display Frame 2 Start Position
#define NE1_MASK_DF_DS_M2        0x0000000000100000LL  //!< VBlank Mask Display Frame 2 Display Size
#define NE1_MASK_DF_RC_M2        0x0000000000200000LL  //!< VBlank Mask Display Frame 2 Scale
#define NE1_MASK_MF_WS_M3        0x0000000001000000LL  //!< VBlank Mask Memory Frame 3 Size
#define NE1_MASK_MF_SP_M3        0x0000000002000000LL  //!< VBlank Mask Memory Frame 3 Start Position
#define NE1_MASK_MF_DS_M3        0x0000000004000000LL  //!< VBlank Mask Memory Frame 3 Display Frame Size
#define NE1_MASK_DF_SP_M3        0x0000000008000000LL  //!< VBlank Mask Display Frame 3 Start Position
#define NE1_MASK_DF_DS_M3        0x0000000010000000LL  //!< VBlank Mask Display Frame 3 Display Size
#define NE1_MASK_DF_RC_M3        0x0000000020000000LL  //!< VBlank Mask Display Frame 3 Scale
#define NE1_MASK_MF_WS_M4        0x0000000100000000LL  //!< VBlank Mask Memory Frame 4 Size
#define NE1_MASK_MF_SP_M4        0x0000000200000000LL  //!< VBlank Mask Memory Frame 4 Start Position
#define NE1_MASK_MF_DS_M4        0x0000000400000000LL  //!< VBlank Mask Memory Frame 4 Display Frame Size
#define NE1_MASK_DF_SP_M4        0x0000000800000000LL  //!< VBlank Mask Display Frame 4 Start Position
#define NE1_MASK_DF_DS_M4        0x0000001000000000LL  //!< VBlank Mask Display Frame 4 Display Size
#define NE1_MASK_DF_RC_M4        0x0000002000000000LL  //!< VBlank Mask Display Frame 4 Scale
#define NE1_MASK_MF_WS_M5        0x0000010000000000LL  //!< VBlank Mask Memory Frame 5 Size
#define NE1_MASK_MF_SP_M5        0x0000020000000000LL  //!< VBlank Mask Memory Frame 5 Start Position
#define NE1_MASK_MF_DS_M5        0x0000040000000000LL  //!< VBlank Mask Memory Frame 5 Display Frame Size
#define NE1_MASK_DF_SP_M5        0x0000080000000000LL  //!< VBlank Mask Display Frame 5 Start Position
#define NE1_MASK_DF_DS_M5        0x0000100000000000LL  //!< VBlank Mask Display Frame 5 Display Size
#define NE1_MASK_DF_RC_M5        0x0000200000000000LL  //!< VBlank Mask Display Frame 5 Scale
#define NE1_MASK_MF_WS_M6        0x0001000000000000LL  //!< VBlank Mask Memory Frame 6 Size
#define NE1_MASK_MF_SP_M6        0x0002000000000000LL  //!< VBlank Mask Memory Frame 6 Start Position
#define NE1_MASK_MF_DS_M6        0x0004000000000000LL  //!< VBlank Mask Memory Frame 6 Display Frame Size
#define NE1_MASK_DF_SP_M6        0x0008000000000000LL  //!< VBlank Mask Display Frame 6 Start Position
#define NE1_MASK_DF_DS_M6        0x0010000000000000LL  //!< VBlank Mask Display Frame 6 Display Size
#define NE1_MASK_DF_RC_M6        0x0020000000000000LL  //!< VBlank Mask Display Frame 6 Scale
#define NE1_MASK_HC_SP_M         0x0100000000000000LL  //!< VBlank Mask HW Cursor Start Position
#define NE1_MASK_DS_GMCTR_0      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R0        0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R1        0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R2        0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R3        0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R4        0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R5        0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R6        0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R7        0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_1      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R8        0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R9        0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R10       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R11       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R12       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R13       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R14       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R15       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_2      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R16       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R17       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R18       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R19       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R20       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R21       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R22       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R23       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_3      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R24       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R25       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R26       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R27       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R28       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R29       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R30       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R31       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_4      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R32       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R33       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R34       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R35       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R36       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R37       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R38       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R39       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_5      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R40       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R41       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R42       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R43       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R44       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R45       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R46       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R47       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_6      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R48       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R49       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R50       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R51       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R52       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R53       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R54       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R55       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_7      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R56       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R57       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R58       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R59       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R60       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R61       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R62       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R63       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_8      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R64       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R65       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R66       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R67       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R68       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R69       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R70       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R71       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_9      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R72       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R73       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R74       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R75       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R76       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R77       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R78       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R79       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_10     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R80       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R81       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R82       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R83       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R84       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R85       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R86       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R87       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_11     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R88       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R89       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R90       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R91       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R92       0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R93       0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R94       0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R95       0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_12     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R96       0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R97       0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R98       0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R99       0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R100      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R101      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R102      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R103      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_13     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R104      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R105      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R106      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R107      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R108      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R109      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R110      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R111      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_14     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R112      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R113      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R114      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R115      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R116      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R117      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R118      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R119      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_15     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R120      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R121      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R122      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R123      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R124      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R125      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R126      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R127      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_16     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R128      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R129      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R130      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R131      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R132      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R133      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R134      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R135      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_17     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R136      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R137      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R138      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R139      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R140      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R141      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R142      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R143      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_18     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R144      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R145      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R146      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R147      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R148      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R149      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R150      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R151      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_19     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R152      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R153      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R154      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R155      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R156      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R157      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R158      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R159      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_20     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R160      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R161      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R162      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R163      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R164      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R165      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R166      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R167      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_21     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R168      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R169      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R170      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R171      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R172      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R173      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R174      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R175      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_22     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R176      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R177      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R178      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R179      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R180      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R181      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R182      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R183      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_23     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R184      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R185      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R186      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R187      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R188      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R189      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R190      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R191      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_24     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R192      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R193      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R194      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R195      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R196      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R197      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R198      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R199      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_25     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R200      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R201      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R202      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R203      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R204      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R205      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R206      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R207      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_26     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R208      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R209      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R210      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R211      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R212      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R213      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R214      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R215      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_27     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R216      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R217      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R218      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R219      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R220      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R221      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R222      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R223      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_28     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R224      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R225      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R226      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R227      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R228      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R229      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R230      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R231      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_29     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R232      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R233      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R234      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R235      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R236      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R237      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R238      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R239      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_30     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R240      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R241      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R242      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R243      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R244      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R245      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R246      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R247      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTR_31     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_R248      0x00000000000000FFLL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R249      0x000000000000FF00LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R250      0x0000000000FF0000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R251      0x00000000FF000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R252      0x000000FF00000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R253      0x0000FF0000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R254      0x00FF000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GC_R255      0xFF00000000000000LL  //!< Gamma Correction R
#define NE1_MASK_DS_GMCTG_0      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G0        0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G1        0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G2        0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G3        0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G4        0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G5        0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G6        0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G7        0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_1      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G8        0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G9        0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G10       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G11       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G12       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G13       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G14       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G15       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_2      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G16       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G17       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G18       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G19       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G20       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G21       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G22       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G23       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_3      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G24       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G25       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G26       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G27       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G28       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G29       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G30       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G31       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_4      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G32       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G33       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G34       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G35       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G36       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G37       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G38       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G39       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_5      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G40       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G41       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G42       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G43       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G44       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G45       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G46       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G47       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_6      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G48       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G49       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G50       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G51       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G52       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G53       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G54       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G55       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_7      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G56       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G57       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G58       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G59       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G60       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G61       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G62       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G63       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_8      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G64       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G65       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G66       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G67       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G68       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G69       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G70       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G71       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_9      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G72       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G73       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G74       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G75       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G76       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G77       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G78       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G79       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_10     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G80       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G81       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G82       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G83       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G84       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G85       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G86       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G87       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_11     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G88       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G89       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G90       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G91       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G92       0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G93       0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G94       0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G95       0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_12     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G96       0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G97       0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G98       0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G99       0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G100      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G101      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G102      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G103      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_13     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G104      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G105      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G106      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G107      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G108      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G109      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G110      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G111      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_14     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G112      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G113      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G114      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G115      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G116      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G117      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G118      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G119      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_15     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G120      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G121      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G122      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G123      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G124      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G125      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G126      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G127      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_16     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G128      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G129      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G130      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G131      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G132      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G133      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G134      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G135      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_17     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G136      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G137      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G138      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G139      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G140      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G141      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G142      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G143      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_18     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G144      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G145      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G146      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G147      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G148      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G149      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G150      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G151      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_19     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G152      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G153      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G154      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G155      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G156      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G157      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G158      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G159      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_20     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G160      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G161      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G162      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G163      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G164      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G165      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G166      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G167      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_21     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G168      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G169      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G170      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G171      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G172      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G173      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G174      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G175      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_22     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G176      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G177      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G178      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G179      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G180      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G181      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G182      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G183      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_23     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G184      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G185      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G186      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G187      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G188      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G189      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G190      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G191      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_24     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G192      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G193      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G194      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G195      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G196      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G197      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G198      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G199      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_25     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G200      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G201      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G202      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G203      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G204      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G205      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G206      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G207      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_26     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G208      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G209      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G210      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G211      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G212      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G213      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G214      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G215      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_27     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G216      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G217      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G218      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G219      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G220      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G221      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G222      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G223      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_28     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G224      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G225      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G226      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G227      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G228      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G229      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G230      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G231      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_29     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G232      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G233      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G234      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G235      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G236      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G237      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G238      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G239      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_30     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G240      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G241      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G242      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G243      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G244      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G245      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G246      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G247      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTG_31     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_G248      0x00000000000000FFLL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G249      0x000000000000FF00LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G250      0x0000000000FF0000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G251      0x00000000FF000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G252      0x000000FF00000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G253      0x0000FF0000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G254      0x00FF000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GC_G255      0xFF00000000000000LL  //!< Gamma Correction G
#define NE1_MASK_DS_GMCTB_0      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B0        0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B1        0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B2        0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B3        0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B4        0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B5        0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B6        0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B7        0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_1      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B8        0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B9        0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B10       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B11       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B12       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B13       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B14       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B15       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_2      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B16       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B17       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B18       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B19       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B20       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B21       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B22       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B23       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_3      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B24       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B25       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B26       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B27       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B28       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B29       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B30       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B31       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_4      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B32       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B33       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B34       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B35       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B36       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B37       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B38       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B39       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_5      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B40       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B41       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B42       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B43       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B44       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B45       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B46       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B47       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_6      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B48       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B49       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B50       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B51       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B52       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B53       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B54       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B55       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_7      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B56       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B57       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B58       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B59       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B60       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B61       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B62       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B63       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_8      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B64       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B65       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B66       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B67       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B68       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B69       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B70       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B71       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_9      0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B72       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B73       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B74       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B75       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B76       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B77       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B78       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B79       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_10     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B80       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B81       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B82       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B83       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B84       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B85       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B86       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B87       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_11     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B88       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B89       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B90       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B91       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B92       0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B93       0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B94       0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B95       0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_12     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B96       0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B97       0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B98       0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B99       0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B100      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B101      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B102      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B103      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_13     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B104      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B105      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B106      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B107      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B108      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B109      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B110      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B111      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_14     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B112      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B113      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B114      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B115      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B116      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B117      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B118      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B119      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_15     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B120      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B121      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B122      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B123      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B124      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B125      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B126      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B127      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_16     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B128      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B129      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B130      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B131      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B132      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B133      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B134      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B135      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_17     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B136      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B137      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B138      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B139      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B140      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B141      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B142      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B143      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_18     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B144      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B145      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B146      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B147      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B148      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B149      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B150      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B151      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_19     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B152      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B153      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B154      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B155      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B156      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B157      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B158      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B159      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_20     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B160      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B161      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B162      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B163      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B164      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B165      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B166      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B167      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_21     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B168      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B169      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B170      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B171      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B172      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B173      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B174      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B175      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_22     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B176      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B177      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B178      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B179      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B180      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B181      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B182      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B183      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_23     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B184      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B185      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B186      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B187      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B188      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B189      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B190      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B191      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_24     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B192      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B193      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B194      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B195      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B196      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B197      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B198      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B199      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_25     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B200      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B201      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B202      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B203      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B204      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B205      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B206      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B207      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_26     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B208      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B209      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B210      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B211      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B212      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B213      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B214      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B215      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_27     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B216      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B217      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B218      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B219      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B220      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B221      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B222      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B223      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_28     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B224      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B225      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B226      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B227      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B228      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B229      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B230      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B231      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_29     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B232      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B233      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B234      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B235      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B236      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B237      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B238      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B239      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_30     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B240      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B241      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B242      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B243      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B244      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B245      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B246      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B247      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GMCTB_31     0xFFFFFFFFFFFFFFFFLL  //!< Gamma Correction
#define NE1_MASK_DS_GC_B248      0x00000000000000FFLL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B249      0x000000000000FF00LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B250      0x0000000000FF0000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B251      0x00000000FF000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B252      0x000000FF00000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B253      0x0000FF0000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B254      0x00FF000000000000LL  //!< Gamma Correction B
#define NE1_MASK_DS_GC_B255      0xFF00000000000000LL  //!< Gamma Correction B
#define NE1_MASK_HC_DATFRM_0     0xFFFFFFFFFFFFFFFFLL  //!< HW Cursor Data Frame
#define NE1_MASK_HC_DF           0xFFFFFFFFFFFFFFFFLL  //!< HW Cursor Data Frame  HW Cursor Data Frame
#define NE1_MASK_HC_CLUT_0       0xFFFFFFFFFFFFFFFFLL  //!< HW Cursor Lookup Table
#define NE1_MASK_HC_LUT          0xFFFFFFFFFFFFFFFFLL  //!< HW Cursor Lookup Table

/********************************************************************
 *    Macro for bit-shift
 ********************************************************************/
#define NE1_SHIFT_IC_SS          (0)
#define NE1_SHIFT_IC_CS          (1)
#define NE1_SHIFT_IC_SC          (2)
#define NE1_SHIFT_IC_CP          (3)
#define NE1_SHIFT_IC_HP          (4)
#define NE1_SHIFT_IC_EHP         (5)
#define NE1_SHIFT_IC_VP          (6)
#define NE1_SHIFT_IC_EVP         (7)
#define NE1_SHIFT_IC_DEP         (8)
#define NE1_SHIFT_IC_CDP         (9)
#define NE1_SHIFT_IC_DD          (16)
#define NE1_SHIFT_IC_CDD         (20)
#define NE1_SHIFT_IC_IS          (0)
#define NE1_SHIFT_IC_VCNT        (8)
#define NE1_SHIFT_IC_VSS         (22)
#define NE1_SHIFT_IC_VBS         (23)
#define NE1_SHIFT_IC_ICL         (0)
#define NE1_SHIFT_IC_IP          (0)
#define NE1_SHIFT_IC_IL          (0)
#define NE1_SHIFT_IC_AL_H        (0)
#define NE1_SHIFT_IC_ISEL0       (0)
#define NE1_SHIFT_IC_IF0         (4)
#define NE1_SHIFT_IC_ISEL1       (8)
#define NE1_SHIFT_IC_IF1         (12)
#define NE1_SHIFT_IC_ISEL2       (16)
#define NE1_SHIFT_IC_IF2         (20)
#define NE1_SHIFT_IC_ISEL3       (24)
#define NE1_SHIFT_IC_IF3         (28)
#define NE1_SHIFT_IC_ISEL4       (32)
#define NE1_SHIFT_IC_IF4         (36)
#define NE1_SHIFT_IC_ISEL5       (40)
#define NE1_SHIFT_IC_IF5         (44)
#define NE1_SHIFT_IC_ISEL6       (48)
#define NE1_SHIFT_IC_IF6         (52)
#define NE1_SHIFT_DS_HR          (0)
#define NE1_SHIFT_DS_VR          (16)
#define NE1_SHIFT_DS_TH          (32)
#define NE1_SHIFT_DS_TV          (48)
#define NE1_SHIFT_DS_THP         (0)
#define NE1_SHIFT_DS_TVP         (16)
#define NE1_SHIFT_DS_THB         (32)
#define NE1_SHIFT_DS_TVB         (48)
#define NE1_SHIFT_DS_OC          (0)
#define NE1_SHIFT_DS_DS          (4)
#define NE1_SHIFT_DS_DM_H        (6)
#define NE1_SHIFT_DS_DM_V        (7)
#define NE1_SHIFT_DS_GS          (15)
#define NE1_SHIFT_DS_DC0         (0)
#define NE1_SHIFT_DS_DC1         (4)
#define NE1_SHIFT_DS_DC2         (8)
#define NE1_SHIFT_DS_DC3         (12)
#define NE1_SHIFT_DS_DC4         (16)
#define NE1_SHIFT_DS_DC5         (20)
#define NE1_SHIFT_DS_DC6         (24)
#define NE1_SHIFT_DS_DC7         (28)
#define NE1_SHIFT_DS_DC8         (32)
#define NE1_SHIFT_DS_DC9         (36)
#define NE1_SHIFT_DS_DC10        (40)
#define NE1_SHIFT_DS_DC11        (44)
#define NE1_SHIFT_DS_DC12        (48)
#define NE1_SHIFT_DS_DC13        (52)
#define NE1_SHIFT_DS_DC14        (56)
#define NE1_SHIFT_DS_DC15        (60)
#define NE1_SHIFT_DS_BC          (0)
#define NE1_SHIFT_MF_ES0         (0)
#define NE1_SHIFT_MF_ES1         (2)
#define NE1_SHIFT_MF_ES2         (4)
#define NE1_SHIFT_MF_ES3         (6)
#define NE1_SHIFT_MF_ES4         (8)
#define NE1_SHIFT_MF_ES5         (10)
#define NE1_SHIFT_MF_ES6         (12)
#define NE1_SHIFT_MF_BE0         (16)
#define NE1_SHIFT_MF_BE1         (17)
#define NE1_SHIFT_MF_BE2         (18)
#define NE1_SHIFT_MF_BE3         (19)
#define NE1_SHIFT_MF_BE4         (20)
#define NE1_SHIFT_MF_BE5         (21)
#define NE1_SHIFT_MF_BE6         (22)
#define NE1_SHIFT_MF_LBS         (24)
#define NE1_SHIFT_MF_PF0         (0)
#define NE1_SHIFT_MF_YC0         (4)
#define NE1_SHIFT_MF_PS0         (8)
#define NE1_SHIFT_MF_MS_H0       (10)
#define NE1_SHIFT_MF_MS_V0       (11)
#define NE1_SHIFT_MF_RL0         (12)
#define NE1_SHIFT_MF_AF0         (15)
#define NE1_SHIFT_MF_PF1         (16)
#define NE1_SHIFT_MF_YC1         (20)
#define NE1_SHIFT_MF_PS1         (24)
#define NE1_SHIFT_MF_MS_H1       (26)
#define NE1_SHIFT_MF_RL1         (28)
#define NE1_SHIFT_MF_AF1         (31)
#define NE1_SHIFT_MF_PF2         (32)
#define NE1_SHIFT_MF_YC2         (36)
#define NE1_SHIFT_MF_PS2         (40)
#define NE1_SHIFT_MF_MS_H2       (42)
#define NE1_SHIFT_MF_MS_V2       (43)
#define NE1_SHIFT_MF_RL2         (44)
#define NE1_SHIFT_MF_AF2         (47)
#define NE1_SHIFT_MF_PF3         (48)
#define NE1_SHIFT_MF_YC3         (52)
#define NE1_SHIFT_MF_PS3         (56)
#define NE1_SHIFT_MF_MS_H3       (58)
#define NE1_SHIFT_MF_RL3         (60)
#define NE1_SHIFT_MF_AF3         (63)
#define NE1_SHIFT_MF_PF4         (0)
#define NE1_SHIFT_MF_YC4         (4)
#define NE1_SHIFT_MF_PS4         (8)
#define NE1_SHIFT_MF_MS_H4       (10)
#define NE1_SHIFT_MF_MS_V4       (11)
#define NE1_SHIFT_MF_RL4         (12)
#define NE1_SHIFT_MF_AF4         (15)
#define NE1_SHIFT_MF_PF5         (16)
#define NE1_SHIFT_MF_YC5         (20)
#define NE1_SHIFT_MF_PS5         (24)
#define NE1_SHIFT_MF_MS_H5       (26)
#define NE1_SHIFT_MF_RL5         (28)
#define NE1_SHIFT_MF_AF5         (31)
#define NE1_SHIFT_MF_PF6         (32)
#define NE1_SHIFT_MF_YC6         (36)
#define NE1_SHIFT_MF_PS6         (40)
#define NE1_SHIFT_MF_MS_H6       (42)
#define NE1_SHIFT_MF_RL6         (44)
#define NE1_SHIFT_MF_AF6         (47)
#define NE1_SHIFT_MF_SA0         (4)
#define NE1_SHIFT_MF_SA1         (36)
#define NE1_SHIFT_MF_SA2         (4)
#define NE1_SHIFT_MF_SA3         (36)
#define NE1_SHIFT_MF_SA4         (4)
#define NE1_SHIFT_MF_SA5         (36)
#define NE1_SHIFT_MF_SA6         (4)
#define NE1_SHIFT_MF_WS_H0       (0)
#define NE1_SHIFT_MF_WS_H1       (16)
#define NE1_SHIFT_MF_WS_H2       (32)
#define NE1_SHIFT_MF_WS_H3       (48)
#define NE1_SHIFT_MF_WS_H4       (0)
#define NE1_SHIFT_MF_WS_H5       (16)
#define NE1_SHIFT_MF_WS_H6       (32)
#define NE1_SHIFT_MF_WS_V0       (0)
#define NE1_SHIFT_MF_WS_V1       (16)
#define NE1_SHIFT_MF_WS_V2       (32)
#define NE1_SHIFT_MF_WS_V3       (48)
#define NE1_SHIFT_MF_WS_V4       (0)
#define NE1_SHIFT_MF_WS_V5       (16)
#define NE1_SHIFT_MF_WS_V6       (32)
#define NE1_SHIFT_MF_SP_X0       (0)
#define NE1_SHIFT_MF_SP_X1       (16)
#define NE1_SHIFT_MF_SP_X2       (32)
#define NE1_SHIFT_MF_SP_X3       (48)
#define NE1_SHIFT_MF_SP_X4       (0)
#define NE1_SHIFT_MF_SP_X5       (16)
#define NE1_SHIFT_MF_SP_X6       (32)
#define NE1_SHIFT_MF_SP_Y0       (0)
#define NE1_SHIFT_MF_SP_Y1       (16)
#define NE1_SHIFT_MF_SP_Y2       (32)
#define NE1_SHIFT_MF_SP_Y3       (48)
#define NE1_SHIFT_MF_SP_Y4       (0)
#define NE1_SHIFT_MF_SP_Y5       (16)
#define NE1_SHIFT_MF_SP_Y6       (32)
#define NE1_SHIFT_MF_SP_YF0      (0)
#define NE1_SHIFT_MF_SP_YF2      (2)
#define NE1_SHIFT_MF_SP_YF4      (4)
#define NE1_SHIFT_MF_DS_H0       (0)
#define NE1_SHIFT_MF_DS_H1       (16)
#define NE1_SHIFT_MF_DS_H2       (32)
#define NE1_SHIFT_MF_DS_H3       (48)
#define NE1_SHIFT_MF_DS_H4       (0)
#define NE1_SHIFT_MF_DS_H5       (16)
#define NE1_SHIFT_MF_DS_H6       (32)
#define NE1_SHIFT_MF_DS_V0       (0)
#define NE1_SHIFT_MF_DS_V1       (16)
#define NE1_SHIFT_MF_DS_V2       (32)
#define NE1_SHIFT_MF_DS_V3       (48)
#define NE1_SHIFT_MF_DS_V4       (0)
#define NE1_SHIFT_MF_DS_V5       (16)
#define NE1_SHIFT_MF_DS_V6       (32)
#define NE1_SHIFT_MF_BC0         (0)
#define NE1_SHIFT_MF_BC_S0       (28)
#define NE1_SHIFT_MF_BC1         (32)
#define NE1_SHIFT_MF_BC_S1       (60)
#define NE1_SHIFT_MF_BC2         (0)
#define NE1_SHIFT_MF_BC_S2       (28)
#define NE1_SHIFT_MF_BC3         (32)
#define NE1_SHIFT_MF_BC_S3       (60)
#define NE1_SHIFT_MF_BC4         (0)
#define NE1_SHIFT_MF_BC_S4       (28)
#define NE1_SHIFT_MF_BC5         (32)
#define NE1_SHIFT_MF_BC_S5       (60)
#define NE1_SHIFT_MF_BC6         (0)
#define NE1_SHIFT_MF_BC_S6       (28)
#define NE1_SHIFT_CP_SA0         (4)
#define NE1_SHIFT_CP_SA1         (36)
#define NE1_SHIFT_CP_SA2         (4)
#define NE1_SHIFT_CP_RN0         (0)
#define NE1_SHIFT_CP_RN1         (4)
#define NE1_SHIFT_CP_RN2         (8)
#define NE1_SHIFT_DF_DC0         (0)
#define NE1_SHIFT_DF_VS0         (8)
#define NE1_SHIFT_DF_LC0         (12)
#define NE1_SHIFT_DF_DC1         (16)
#define NE1_SHIFT_DF_VS1         (24)
#define NE1_SHIFT_DF_LC1         (28)
#define NE1_SHIFT_DF_DC2         (32)
#define NE1_SHIFT_DF_VS2         (40)
#define NE1_SHIFT_DF_LC2         (44)
#define NE1_SHIFT_DF_DC3         (48)
#define NE1_SHIFT_DF_VS3         (56)
#define NE1_SHIFT_DF_LC3         (60)
#define NE1_SHIFT_DF_DC4         (0)
#define NE1_SHIFT_DF_VS4         (8)
#define NE1_SHIFT_DF_LC4         (12)
#define NE1_SHIFT_DF_DC5         (16)
#define NE1_SHIFT_DF_VS5         (24)
#define NE1_SHIFT_DF_LC5         (28)
#define NE1_SHIFT_DF_DC6         (32)
#define NE1_SHIFT_DF_VS6         (40)
#define NE1_SHIFT_DF_LC6         (44)
#define NE1_SHIFT_DF_SP_X0       (0)
#define NE1_SHIFT_DF_SP_X1       (16)
#define NE1_SHIFT_DF_SP_X2       (32)
#define NE1_SHIFT_DF_SP_X3       (48)
#define NE1_SHIFT_DF_SP_X4       (0)
#define NE1_SHIFT_DF_SP_X5       (16)
#define NE1_SHIFT_DF_SP_X6       (32)
#define NE1_SHIFT_DF_SP_Y0       (0)
#define NE1_SHIFT_DF_SP_Y1       (16)
#define NE1_SHIFT_DF_SP_Y2       (32)
#define NE1_SHIFT_DF_SP_Y3       (48)
#define NE1_SHIFT_DF_SP_Y4       (0)
#define NE1_SHIFT_DF_SP_Y5       (16)
#define NE1_SHIFT_DF_SP_Y6       (32)
#define NE1_SHIFT_DF_DS_H0       (0)
#define NE1_SHIFT_DF_DS_H1       (16)
#define NE1_SHIFT_DF_DS_H2       (32)
#define NE1_SHIFT_DF_DS_H3       (48)
#define NE1_SHIFT_DF_DS_H4       (0)
#define NE1_SHIFT_DF_DS_H5       (16)
#define NE1_SHIFT_DF_DS_H6       (32)
#define NE1_SHIFT_DF_DS_V0       (0)
#define NE1_SHIFT_DF_DS_V2       (32)
#define NE1_SHIFT_DF_DS_V4       (0)
#define NE1_SHIFT_DF_RC_H0       (0)
#define NE1_SHIFT_DF_RC_H1       (16)
#define NE1_SHIFT_DF_RC_H2       (32)
#define NE1_SHIFT_DF_RC_H3       (48)
#define NE1_SHIFT_DF_RC_H4       (0)
#define NE1_SHIFT_DF_RC_H5       (16)
#define NE1_SHIFT_DF_RC_H6       (32)
#define NE1_SHIFT_DF_RC_V0       (0)
#define NE1_SHIFT_DF_RC_V2       (32)
#define NE1_SHIFT_DF_RC_V4       (0)
#define NE1_SHIFT_DF_PT_R00      (0)
#define NE1_SHIFT_DF_PT_R10      (8)
#define NE1_SHIFT_DF_PT_R01      (16)
#define NE1_SHIFT_DF_PT_R11      (24)
#define NE1_SHIFT_DF_PT_R02      (32)
#define NE1_SHIFT_DF_PT_R12      (40)
#define NE1_SHIFT_DF_PT_R03      (48)
#define NE1_SHIFT_DF_PT_R13      (56)
#define NE1_SHIFT_DF_PT_R04      (0)
#define NE1_SHIFT_DF_PT_R14      (8)
#define NE1_SHIFT_DF_PT_R05      (16)
#define NE1_SHIFT_DF_PT_R15      (24)
#define NE1_SHIFT_DF_PT_R06      (32)
#define NE1_SHIFT_DF_PT_R16      (40)
#define NE1_SHIFT_DF_LT_C0       (0)
#define NE1_SHIFT_DF_PT_C0       (1)
#define NE1_SHIFT_DF_LT_R0       (8)
#define NE1_SHIFT_DF_LT_C1       (16)
#define NE1_SHIFT_DF_PT_C1       (17)
#define NE1_SHIFT_DF_LT_R1       (24)
#define NE1_SHIFT_DF_LT_C2       (32)
#define NE1_SHIFT_DF_PT_C2       (33)
#define NE1_SHIFT_DF_LT_R2       (40)
#define NE1_SHIFT_DF_LT_C3       (48)
#define NE1_SHIFT_DF_PT_C3       (49)
#define NE1_SHIFT_DF_LT_R3       (56)
#define NE1_SHIFT_DF_LT_C4       (0)
#define NE1_SHIFT_DF_PT_C4       (1)
#define NE1_SHIFT_DF_LT_R4       (8)
#define NE1_SHIFT_DF_LT_C5       (16)
#define NE1_SHIFT_DF_PT_C5       (17)
#define NE1_SHIFT_DF_LT_R5       (24)
#define NE1_SHIFT_DF_LT_C6       (32)
#define NE1_SHIFT_DF_PT_C6       (33)
#define NE1_SHIFT_DF_LT_R6       (40)
#define NE1_SHIFT_DF_CK_C0       (0)
#define NE1_SHIFT_DF_CK_C1       (1)
#define NE1_SHIFT_DF_CK_C2       (2)
#define NE1_SHIFT_DF_CK_C3       (3)
#define NE1_SHIFT_DF_CK_C4       (4)
#define NE1_SHIFT_DF_CK_C5       (5)
#define NE1_SHIFT_DF_CK_C6       (6)
#define NE1_SHIFT_DF_CK_S0       (8)
#define NE1_SHIFT_DF_CK_S1       (9)
#define NE1_SHIFT_DF_CK_S2       (10)
#define NE1_SHIFT_DF_CK_S3       (11)
#define NE1_SHIFT_DF_CK_S4       (12)
#define NE1_SHIFT_DF_CK_S5       (13)
#define NE1_SHIFT_DF_CK_S6       (14)
#define NE1_SHIFT_DF_CK_A0       (16)
#define NE1_SHIFT_DF_CK_A1       (17)
#define NE1_SHIFT_DF_CK_A2       (18)
#define NE1_SHIFT_DF_CK_A3       (19)
#define NE1_SHIFT_DF_CK_A4       (20)
#define NE1_SHIFT_DF_CK_A5       (21)
#define NE1_SHIFT_DF_CK_A6       (22)
#define NE1_SHIFT_DF_CK0         (0)
#define NE1_SHIFT_DF_CK1         (32)
#define NE1_SHIFT_DF_CK2         (0)
#define NE1_SHIFT_DF_CK3         (32)
#define NE1_SHIFT_DF_CK4         (0)
#define NE1_SHIFT_DF_CK5         (32)
#define NE1_SHIFT_DF_CK6         (0)
#define NE1_SHIFT_DF_CK_PR_BV0   (0)
#define NE1_SHIFT_DF_CK_PR_GY0   (4)
#define NE1_SHIFT_DF_CK_PR_RU0   (8)
#define NE1_SHIFT_DF_CK_PR_A0    (12)
#define NE1_SHIFT_DF_CK_PR_BV1   (16)
#define NE1_SHIFT_DF_CK_PR_GY1   (20)
#define NE1_SHIFT_DF_CK_PR_RU1   (24)
#define NE1_SHIFT_DF_CK_PR_A1    (28)
#define NE1_SHIFT_DF_CK_PR_BV2   (32)
#define NE1_SHIFT_DF_CK_PR_GY2   (36)
#define NE1_SHIFT_DF_CK_PR_RU2   (40)
#define NE1_SHIFT_DF_CK_PR_A2    (44)
#define NE1_SHIFT_DF_CK_PR_BV3   (48)
#define NE1_SHIFT_DF_CK_PR_GY3   (52)
#define NE1_SHIFT_DF_CK_PR_RU3   (56)
#define NE1_SHIFT_DF_CK_PR_A3    (60)
#define NE1_SHIFT_DF_CK_PR_BV4   (0)
#define NE1_SHIFT_DF_CK_PR_GY4   (4)
#define NE1_SHIFT_DF_CK_PR_RU4   (8)
#define NE1_SHIFT_DF_CK_PR_A4    (12)
#define NE1_SHIFT_DF_CK_PR_BV5   (16)
#define NE1_SHIFT_DF_CK_PR_GY5   (20)
#define NE1_SHIFT_DF_CK_PR_RU5   (24)
#define NE1_SHIFT_DF_CK_PR_A5    (28)
#define NE1_SHIFT_DF_CK_PR_BV6   (32)
#define NE1_SHIFT_DF_CK_PR_GY6   (36)
#define NE1_SHIFT_DF_CK_PR_RU6   (40)
#define NE1_SHIFT_DF_CK_PR_A6    (44)
#define NE1_SHIFT_DF_CC_B0       (0)
#define NE1_SHIFT_DF_CC_G0       (8)
#define NE1_SHIFT_DF_CC_R0       (16)
#define NE1_SHIFT_DF_CC_B1       (32)
#define NE1_SHIFT_DF_CC_G1       (40)
#define NE1_SHIFT_DF_CC_R1       (48)
#define NE1_SHIFT_DF_CC_B2       (0)
#define NE1_SHIFT_DF_CC_G2       (8)
#define NE1_SHIFT_DF_CC_R2       (16)
#define NE1_SHIFT_DF_CC_B3       (32)
#define NE1_SHIFT_DF_CC_G3       (40)
#define NE1_SHIFT_DF_CC_R3       (48)
#define NE1_SHIFT_DF_CC_B4       (0)
#define NE1_SHIFT_DF_CC_G4       (8)
#define NE1_SHIFT_DF_CC_R4       (16)
#define NE1_SHIFT_DF_CC_B5       (32)
#define NE1_SHIFT_DF_CC_G5       (40)
#define NE1_SHIFT_DF_CC_R5       (48)
#define NE1_SHIFT_DF_CC_B6       (0)
#define NE1_SHIFT_DF_CC_G6       (8)
#define NE1_SHIFT_DF_CC_R6       (16)
#define NE1_SHIFT_DF_BC_B        (0)
#define NE1_SHIFT_DF_BC_G        (8)
#define NE1_SHIFT_DF_BC_R        (16)
#define NE1_SHIFT_DF_CD_B        (0)
#define NE1_SHIFT_DF_CD_G        (8)
#define NE1_SHIFT_DF_CD_R        (16)
#define NE1_SHIFT_DF_CD_PR_B     (32)
#define NE1_SHIFT_DF_CD_PR_G     (36)
#define NE1_SHIFT_DF_CD_PR_R     (40)
#define NE1_SHIFT_DF_CD_C        (63)
#define NE1_SHIFT_HC_DS          (0)
#define NE1_SHIFT_HC_FS          (1)
#define NE1_SHIFT_HC_LC          (2)
#define NE1_SHIFT_HC_SP_X        (16)
#define NE1_SHIFT_HC_SP_Y        (32)
#define NE1_SHIFT_MF_WS_M0       (0)
#define NE1_SHIFT_MF_SP_M0       (1)
#define NE1_SHIFT_MF_DS_M0       (2)
#define NE1_SHIFT_DF_SP_M0       (3)
#define NE1_SHIFT_DF_DS_M0       (4)
#define NE1_SHIFT_DF_RC_M0       (5)
#define NE1_SHIFT_MF_WS_M1       (8)
#define NE1_SHIFT_MF_SP_M1       (9)
#define NE1_SHIFT_MF_DS_M1       (10)
#define NE1_SHIFT_DF_SP_M1       (11)
#define NE1_SHIFT_DF_DS_M1       (12)
#define NE1_SHIFT_DF_RC_M1       (13)
#define NE1_SHIFT_MF_WS_M2       (16)
#define NE1_SHIFT_MF_SP_M2       (17)
#define NE1_SHIFT_MF_DS_M2       (18)
#define NE1_SHIFT_DF_SP_M2       (19)
#define NE1_SHIFT_DF_DS_M2       (20)
#define NE1_SHIFT_DF_RC_M2       (21)
#define NE1_SHIFT_MF_WS_M3       (24)
#define NE1_SHIFT_MF_SP_M3       (25)
#define NE1_SHIFT_MF_DS_M3       (26)
#define NE1_SHIFT_DF_SP_M3       (27)
#define NE1_SHIFT_DF_DS_M3       (28)
#define NE1_SHIFT_DF_RC_M3       (29)
#define NE1_SHIFT_MF_WS_M4       (32)
#define NE1_SHIFT_MF_SP_M4       (33)
#define NE1_SHIFT_MF_DS_M4       (34)
#define NE1_SHIFT_DF_SP_M4       (35)
#define NE1_SHIFT_DF_DS_M4       (36)
#define NE1_SHIFT_DF_RC_M4       (37)
#define NE1_SHIFT_MF_WS_M5       (40)
#define NE1_SHIFT_MF_SP_M5       (41)
#define NE1_SHIFT_MF_DS_M5       (42)
#define NE1_SHIFT_DF_SP_M5       (43)
#define NE1_SHIFT_DF_DS_M5       (44)
#define NE1_SHIFT_DF_RC_M5       (45)
#define NE1_SHIFT_MF_WS_M6       (48)
#define NE1_SHIFT_MF_SP_M6       (49)
#define NE1_SHIFT_MF_DS_M6       (50)
#define NE1_SHIFT_DF_SP_M6       (51)
#define NE1_SHIFT_DF_DS_M6       (52)
#define NE1_SHIFT_DF_RC_M6       (53)
#define NE1_SHIFT_HC_SP_M        (56)
#define NE1_SHIFT_DS_GC_R0       (0)
#define NE1_SHIFT_DS_GC_R1       (8)
#define NE1_SHIFT_DS_GC_R2       (16)
#define NE1_SHIFT_DS_GC_R3       (24)
#define NE1_SHIFT_DS_GC_R4       (32)
#define NE1_SHIFT_DS_GC_R5       (40)
#define NE1_SHIFT_DS_GC_R6       (48)
#define NE1_SHIFT_DS_GC_R7       (56)
#define NE1_SHIFT_DS_GC_R8       (0)
#define NE1_SHIFT_DS_GC_R9       (8)
#define NE1_SHIFT_DS_GC_R10      (16)
#define NE1_SHIFT_DS_GC_R11      (24)
#define NE1_SHIFT_DS_GC_R12      (32)
#define NE1_SHIFT_DS_GC_R13      (40)
#define NE1_SHIFT_DS_GC_R14      (48)
#define NE1_SHIFT_DS_GC_R15      (56)
#define NE1_SHIFT_DS_GC_R16      (0)
#define NE1_SHIFT_DS_GC_R17      (8)
#define NE1_SHIFT_DS_GC_R18      (16)
#define NE1_SHIFT_DS_GC_R19      (24)
#define NE1_SHIFT_DS_GC_R20      (32)
#define NE1_SHIFT_DS_GC_R21      (40)
#define NE1_SHIFT_DS_GC_R22      (48)
#define NE1_SHIFT_DS_GC_R23      (56)
#define NE1_SHIFT_DS_GC_R24      (0)
#define NE1_SHIFT_DS_GC_R25      (8)
#define NE1_SHIFT_DS_GC_R26      (16)
#define NE1_SHIFT_DS_GC_R27      (24)
#define NE1_SHIFT_DS_GC_R28      (32)
#define NE1_SHIFT_DS_GC_R29      (40)
#define NE1_SHIFT_DS_GC_R30      (48)
#define NE1_SHIFT_DS_GC_R31      (56)
#define NE1_SHIFT_DS_GC_R32      (0)
#define NE1_SHIFT_DS_GC_R33      (8)
#define NE1_SHIFT_DS_GC_R34      (16)
#define NE1_SHIFT_DS_GC_R35      (24)
#define NE1_SHIFT_DS_GC_R36      (32)
#define NE1_SHIFT_DS_GC_R37      (40)
#define NE1_SHIFT_DS_GC_R38      (48)
#define NE1_SHIFT_DS_GC_R39      (56)
#define NE1_SHIFT_DS_GC_R40      (0)
#define NE1_SHIFT_DS_GC_R41      (8)
#define NE1_SHIFT_DS_GC_R42      (16)
#define NE1_SHIFT_DS_GC_R43      (24)
#define NE1_SHIFT_DS_GC_R44      (32)
#define NE1_SHIFT_DS_GC_R45      (40)
#define NE1_SHIFT_DS_GC_R46      (48)
#define NE1_SHIFT_DS_GC_R47      (56)
#define NE1_SHIFT_DS_GC_R48      (0)
#define NE1_SHIFT_DS_GC_R49      (8)
#define NE1_SHIFT_DS_GC_R50      (16)
#define NE1_SHIFT_DS_GC_R51      (24)
#define NE1_SHIFT_DS_GC_R52      (32)
#define NE1_SHIFT_DS_GC_R53      (40)
#define NE1_SHIFT_DS_GC_R54      (48)
#define NE1_SHIFT_DS_GC_R55      (56)
#define NE1_SHIFT_DS_GC_R56      (0)
#define NE1_SHIFT_DS_GC_R57      (8)
#define NE1_SHIFT_DS_GC_R58      (16)
#define NE1_SHIFT_DS_GC_R59      (24)
#define NE1_SHIFT_DS_GC_R60      (32)
#define NE1_SHIFT_DS_GC_R61      (40)
#define NE1_SHIFT_DS_GC_R62      (48)
#define NE1_SHIFT_DS_GC_R63      (56)
#define NE1_SHIFT_DS_GC_R64      (0)
#define NE1_SHIFT_DS_GC_R65      (8)
#define NE1_SHIFT_DS_GC_R66      (16)
#define NE1_SHIFT_DS_GC_R67      (24)
#define NE1_SHIFT_DS_GC_R68      (32)
#define NE1_SHIFT_DS_GC_R69      (40)
#define NE1_SHIFT_DS_GC_R70      (48)
#define NE1_SHIFT_DS_GC_R71      (56)
#define NE1_SHIFT_DS_GC_R72      (0)
#define NE1_SHIFT_DS_GC_R73      (8)
#define NE1_SHIFT_DS_GC_R74      (16)
#define NE1_SHIFT_DS_GC_R75      (24)
#define NE1_SHIFT_DS_GC_R76      (32)
#define NE1_SHIFT_DS_GC_R77      (40)
#define NE1_SHIFT_DS_GC_R78      (48)
#define NE1_SHIFT_DS_GC_R79      (56)
#define NE1_SHIFT_DS_GC_R80      (0)
#define NE1_SHIFT_DS_GC_R81      (8)
#define NE1_SHIFT_DS_GC_R82      (16)
#define NE1_SHIFT_DS_GC_R83      (24)
#define NE1_SHIFT_DS_GC_R84      (32)
#define NE1_SHIFT_DS_GC_R85      (40)
#define NE1_SHIFT_DS_GC_R86      (48)
#define NE1_SHIFT_DS_GC_R87      (56)
#define NE1_SHIFT_DS_GC_R88      (0)
#define NE1_SHIFT_DS_GC_R89      (8)
#define NE1_SHIFT_DS_GC_R90      (16)
#define NE1_SHIFT_DS_GC_R91      (24)
#define NE1_SHIFT_DS_GC_R92      (32)
#define NE1_SHIFT_DS_GC_R93      (40)
#define NE1_SHIFT_DS_GC_R94      (48)
#define NE1_SHIFT_DS_GC_R95      (56)
#define NE1_SHIFT_DS_GC_R96      (0)
#define NE1_SHIFT_DS_GC_R97      (8)
#define NE1_SHIFT_DS_GC_R98      (16)
#define NE1_SHIFT_DS_GC_R99      (24)
#define NE1_SHIFT_DS_GC_R100     (32)
#define NE1_SHIFT_DS_GC_R101     (40)
#define NE1_SHIFT_DS_GC_R102     (48)
#define NE1_SHIFT_DS_GC_R103     (56)
#define NE1_SHIFT_DS_GC_R104     (0)
#define NE1_SHIFT_DS_GC_R105     (8)
#define NE1_SHIFT_DS_GC_R106     (16)
#define NE1_SHIFT_DS_GC_R107     (24)
#define NE1_SHIFT_DS_GC_R108     (32)
#define NE1_SHIFT_DS_GC_R109     (40)
#define NE1_SHIFT_DS_GC_R110     (48)
#define NE1_SHIFT_DS_GC_R111     (56)
#define NE1_SHIFT_DS_GC_R112     (0)
#define NE1_SHIFT_DS_GC_R113     (8)
#define NE1_SHIFT_DS_GC_R114     (16)
#define NE1_SHIFT_DS_GC_R115     (24)
#define NE1_SHIFT_DS_GC_R116     (32)
#define NE1_SHIFT_DS_GC_R117     (40)
#define NE1_SHIFT_DS_GC_R118     (48)
#define NE1_SHIFT_DS_GC_R119     (56)
#define NE1_SHIFT_DS_GC_R120     (0)
#define NE1_SHIFT_DS_GC_R121     (8)
#define NE1_SHIFT_DS_GC_R122     (16)
#define NE1_SHIFT_DS_GC_R123     (24)
#define NE1_SHIFT_DS_GC_R124     (32)
#define NE1_SHIFT_DS_GC_R125     (40)
#define NE1_SHIFT_DS_GC_R126     (48)
#define NE1_SHIFT_DS_GC_R127     (56)
#define NE1_SHIFT_DS_GC_R128     (0)
#define NE1_SHIFT_DS_GC_R129     (8)
#define NE1_SHIFT_DS_GC_R130     (16)
#define NE1_SHIFT_DS_GC_R131     (24)
#define NE1_SHIFT_DS_GC_R132     (32)
#define NE1_SHIFT_DS_GC_R133     (40)
#define NE1_SHIFT_DS_GC_R134     (48)
#define NE1_SHIFT_DS_GC_R135     (56)
#define NE1_SHIFT_DS_GC_R136     (0)
#define NE1_SHIFT_DS_GC_R137     (8)
#define NE1_SHIFT_DS_GC_R138     (16)
#define NE1_SHIFT_DS_GC_R139     (24)
#define NE1_SHIFT_DS_GC_R140     (32)
#define NE1_SHIFT_DS_GC_R141     (40)
#define NE1_SHIFT_DS_GC_R142     (48)
#define NE1_SHIFT_DS_GC_R143     (56)
#define NE1_SHIFT_DS_GC_R144     (0)
#define NE1_SHIFT_DS_GC_R145     (8)
#define NE1_SHIFT_DS_GC_R146     (16)
#define NE1_SHIFT_DS_GC_R147     (24)
#define NE1_SHIFT_DS_GC_R148     (32)
#define NE1_SHIFT_DS_GC_R149     (40)
#define NE1_SHIFT_DS_GC_R150     (48)
#define NE1_SHIFT_DS_GC_R151     (56)
#define NE1_SHIFT_DS_GC_R152     (0)
#define NE1_SHIFT_DS_GC_R153     (8)
#define NE1_SHIFT_DS_GC_R154     (16)
#define NE1_SHIFT_DS_GC_R155     (24)
#define NE1_SHIFT_DS_GC_R156     (32)
#define NE1_SHIFT_DS_GC_R157     (40)
#define NE1_SHIFT_DS_GC_R158     (48)
#define NE1_SHIFT_DS_GC_R159     (56)
#define NE1_SHIFT_DS_GC_R160     (0)
#define NE1_SHIFT_DS_GC_R161     (8)
#define NE1_SHIFT_DS_GC_R162     (16)
#define NE1_SHIFT_DS_GC_R163     (24)
#define NE1_SHIFT_DS_GC_R164     (32)
#define NE1_SHIFT_DS_GC_R165     (40)
#define NE1_SHIFT_DS_GC_R166     (48)
#define NE1_SHIFT_DS_GC_R167     (56)
#define NE1_SHIFT_DS_GC_R168     (0)
#define NE1_SHIFT_DS_GC_R169     (8)
#define NE1_SHIFT_DS_GC_R170     (16)
#define NE1_SHIFT_DS_GC_R171     (24)
#define NE1_SHIFT_DS_GC_R172     (32)
#define NE1_SHIFT_DS_GC_R173     (40)
#define NE1_SHIFT_DS_GC_R174     (48)
#define NE1_SHIFT_DS_GC_R175     (56)
#define NE1_SHIFT_DS_GC_R176     (0)
#define NE1_SHIFT_DS_GC_R177     (8)
#define NE1_SHIFT_DS_GC_R178     (16)
#define NE1_SHIFT_DS_GC_R179     (24)
#define NE1_SHIFT_DS_GC_R180     (32)
#define NE1_SHIFT_DS_GC_R181     (40)
#define NE1_SHIFT_DS_GC_R182     (48)
#define NE1_SHIFT_DS_GC_R183     (56)
#define NE1_SHIFT_DS_GC_R184     (0)
#define NE1_SHIFT_DS_GC_R185     (8)
#define NE1_SHIFT_DS_GC_R186     (16)
#define NE1_SHIFT_DS_GC_R187     (24)
#define NE1_SHIFT_DS_GC_R188     (32)
#define NE1_SHIFT_DS_GC_R189     (40)
#define NE1_SHIFT_DS_GC_R190     (48)
#define NE1_SHIFT_DS_GC_R191     (56)
#define NE1_SHIFT_DS_GC_R192     (0)
#define NE1_SHIFT_DS_GC_R193     (8)
#define NE1_SHIFT_DS_GC_R194     (16)
#define NE1_SHIFT_DS_GC_R195     (24)
#define NE1_SHIFT_DS_GC_R196     (32)
#define NE1_SHIFT_DS_GC_R197     (40)
#define NE1_SHIFT_DS_GC_R198     (48)
#define NE1_SHIFT_DS_GC_R199     (56)
#define NE1_SHIFT_DS_GC_R200     (0)
#define NE1_SHIFT_DS_GC_R201     (8)
#define NE1_SHIFT_DS_GC_R202     (16)
#define NE1_SHIFT_DS_GC_R203     (24)
#define NE1_SHIFT_DS_GC_R204     (32)
#define NE1_SHIFT_DS_GC_R205     (40)
#define NE1_SHIFT_DS_GC_R206     (48)
#define NE1_SHIFT_DS_GC_R207     (56)
#define NE1_SHIFT_DS_GC_R208     (0)
#define NE1_SHIFT_DS_GC_R209     (8)
#define NE1_SHIFT_DS_GC_R210     (16)
#define NE1_SHIFT_DS_GC_R211     (24)
#define NE1_SHIFT_DS_GC_R212     (32)
#define NE1_SHIFT_DS_GC_R213     (40)
#define NE1_SHIFT_DS_GC_R214     (48)
#define NE1_SHIFT_DS_GC_R215     (56)
#define NE1_SHIFT_DS_GC_R216     (0)
#define NE1_SHIFT_DS_GC_R217     (8)
#define NE1_SHIFT_DS_GC_R218     (16)
#define NE1_SHIFT_DS_GC_R219     (24)
#define NE1_SHIFT_DS_GC_R220     (32)
#define NE1_SHIFT_DS_GC_R221     (40)
#define NE1_SHIFT_DS_GC_R222     (48)
#define NE1_SHIFT_DS_GC_R223     (56)
#define NE1_SHIFT_DS_GC_R224     (0)
#define NE1_SHIFT_DS_GC_R225     (8)
#define NE1_SHIFT_DS_GC_R226     (16)
#define NE1_SHIFT_DS_GC_R227     (24)
#define NE1_SHIFT_DS_GC_R228     (32)
#define NE1_SHIFT_DS_GC_R229     (40)
#define NE1_SHIFT_DS_GC_R230     (48)
#define NE1_SHIFT_DS_GC_R231     (56)
#define NE1_SHIFT_DS_GC_R232     (0)
#define NE1_SHIFT_DS_GC_R233     (8)
#define NE1_SHIFT_DS_GC_R234     (16)
#define NE1_SHIFT_DS_GC_R235     (24)
#define NE1_SHIFT_DS_GC_R236     (32)
#define NE1_SHIFT_DS_GC_R237     (40)
#define NE1_SHIFT_DS_GC_R238     (48)
#define NE1_SHIFT_DS_GC_R239     (56)
#define NE1_SHIFT_DS_GC_R240     (0)
#define NE1_SHIFT_DS_GC_R241     (8)
#define NE1_SHIFT_DS_GC_R242     (16)
#define NE1_SHIFT_DS_GC_R243     (24)
#define NE1_SHIFT_DS_GC_R244     (32)
#define NE1_SHIFT_DS_GC_R245     (40)
#define NE1_SHIFT_DS_GC_R246     (48)
#define NE1_SHIFT_DS_GC_R247     (56)
#define NE1_SHIFT_DS_GC_R248     (0)
#define NE1_SHIFT_DS_GC_R249     (8)
#define NE1_SHIFT_DS_GC_R250     (16)
#define NE1_SHIFT_DS_GC_R251     (24)
#define NE1_SHIFT_DS_GC_R252     (32)
#define NE1_SHIFT_DS_GC_R253     (40)
#define NE1_SHIFT_DS_GC_R254     (48)
#define NE1_SHIFT_DS_GC_R255     (56)
#define NE1_SHIFT_DS_GC_G0       (0)
#define NE1_SHIFT_DS_GC_G1       (8)
#define NE1_SHIFT_DS_GC_G2       (16)
#define NE1_SHIFT_DS_GC_G3       (24)
#define NE1_SHIFT_DS_GC_G4       (32)
#define NE1_SHIFT_DS_GC_G5       (40)
#define NE1_SHIFT_DS_GC_G6       (48)
#define NE1_SHIFT_DS_GC_G7       (56)
#define NE1_SHIFT_DS_GC_G8       (0)
#define NE1_SHIFT_DS_GC_G9       (8)
#define NE1_SHIFT_DS_GC_G10      (16)
#define NE1_SHIFT_DS_GC_G11      (24)
#define NE1_SHIFT_DS_GC_G12      (32)
#define NE1_SHIFT_DS_GC_G13      (40)
#define NE1_SHIFT_DS_GC_G14      (48)
#define NE1_SHIFT_DS_GC_G15      (56)
#define NE1_SHIFT_DS_GC_G16      (0)
#define NE1_SHIFT_DS_GC_G17      (8)
#define NE1_SHIFT_DS_GC_G18      (16)
#define NE1_SHIFT_DS_GC_G19      (24)
#define NE1_SHIFT_DS_GC_G20      (32)
#define NE1_SHIFT_DS_GC_G21      (40)
#define NE1_SHIFT_DS_GC_G22      (48)
#define NE1_SHIFT_DS_GC_G23      (56)
#define NE1_SHIFT_DS_GC_G24      (0)
#define NE1_SHIFT_DS_GC_G25      (8)
#define NE1_SHIFT_DS_GC_G26      (16)
#define NE1_SHIFT_DS_GC_G27      (24)
#define NE1_SHIFT_DS_GC_G28      (32)
#define NE1_SHIFT_DS_GC_G29      (40)
#define NE1_SHIFT_DS_GC_G30      (48)
#define NE1_SHIFT_DS_GC_G31      (56)
#define NE1_SHIFT_DS_GC_G32      (0)
#define NE1_SHIFT_DS_GC_G33      (8)
#define NE1_SHIFT_DS_GC_G34      (16)
#define NE1_SHIFT_DS_GC_G35      (24)
#define NE1_SHIFT_DS_GC_G36      (32)
#define NE1_SHIFT_DS_GC_G37      (40)
#define NE1_SHIFT_DS_GC_G38      (48)
#define NE1_SHIFT_DS_GC_G39      (56)
#define NE1_SHIFT_DS_GC_G40      (0)
#define NE1_SHIFT_DS_GC_G41      (8)
#define NE1_SHIFT_DS_GC_G42      (16)
#define NE1_SHIFT_DS_GC_G43      (24)
#define NE1_SHIFT_DS_GC_G44      (32)
#define NE1_SHIFT_DS_GC_G45      (40)
#define NE1_SHIFT_DS_GC_G46      (48)
#define NE1_SHIFT_DS_GC_G47      (56)
#define NE1_SHIFT_DS_GC_G48      (0)
#define NE1_SHIFT_DS_GC_G49      (8)
#define NE1_SHIFT_DS_GC_G50      (16)
#define NE1_SHIFT_DS_GC_G51      (24)
#define NE1_SHIFT_DS_GC_G52      (32)
#define NE1_SHIFT_DS_GC_G53      (40)
#define NE1_SHIFT_DS_GC_G54      (48)
#define NE1_SHIFT_DS_GC_G55      (56)
#define NE1_SHIFT_DS_GC_G56      (0)
#define NE1_SHIFT_DS_GC_G57      (8)
#define NE1_SHIFT_DS_GC_G58      (16)
#define NE1_SHIFT_DS_GC_G59      (24)
#define NE1_SHIFT_DS_GC_G60      (32)
#define NE1_SHIFT_DS_GC_G61      (40)
#define NE1_SHIFT_DS_GC_G62      (48)
#define NE1_SHIFT_DS_GC_G63      (56)
#define NE1_SHIFT_DS_GC_G64      (0)
#define NE1_SHIFT_DS_GC_G65      (8)
#define NE1_SHIFT_DS_GC_G66      (16)
#define NE1_SHIFT_DS_GC_G67      (24)
#define NE1_SHIFT_DS_GC_G68      (32)
#define NE1_SHIFT_DS_GC_G69      (40)
#define NE1_SHIFT_DS_GC_G70      (48)
#define NE1_SHIFT_DS_GC_G71      (56)
#define NE1_SHIFT_DS_GC_G72      (0)
#define NE1_SHIFT_DS_GC_G73      (8)
#define NE1_SHIFT_DS_GC_G74      (16)
#define NE1_SHIFT_DS_GC_G75      (24)
#define NE1_SHIFT_DS_GC_G76      (32)
#define NE1_SHIFT_DS_GC_G77      (40)
#define NE1_SHIFT_DS_GC_G78      (48)
#define NE1_SHIFT_DS_GC_G79      (56)
#define NE1_SHIFT_DS_GC_G80      (0)
#define NE1_SHIFT_DS_GC_G81      (8)
#define NE1_SHIFT_DS_GC_G82      (16)
#define NE1_SHIFT_DS_GC_G83      (24)
#define NE1_SHIFT_DS_GC_G84      (32)
#define NE1_SHIFT_DS_GC_G85      (40)
#define NE1_SHIFT_DS_GC_G86      (48)
#define NE1_SHIFT_DS_GC_G87      (56)
#define NE1_SHIFT_DS_GC_G88      (0)
#define NE1_SHIFT_DS_GC_G89      (8)
#define NE1_SHIFT_DS_GC_G90      (16)
#define NE1_SHIFT_DS_GC_G91      (24)
#define NE1_SHIFT_DS_GC_G92      (32)
#define NE1_SHIFT_DS_GC_G93      (40)
#define NE1_SHIFT_DS_GC_G94      (48)
#define NE1_SHIFT_DS_GC_G95      (56)
#define NE1_SHIFT_DS_GC_G96      (0)
#define NE1_SHIFT_DS_GC_G97      (8)
#define NE1_SHIFT_DS_GC_G98      (16)
#define NE1_SHIFT_DS_GC_G99      (24)
#define NE1_SHIFT_DS_GC_G100     (32)
#define NE1_SHIFT_DS_GC_G101     (40)
#define NE1_SHIFT_DS_GC_G102     (48)
#define NE1_SHIFT_DS_GC_G103     (56)
#define NE1_SHIFT_DS_GC_G104     (0)
#define NE1_SHIFT_DS_GC_G105     (8)
#define NE1_SHIFT_DS_GC_G106     (16)
#define NE1_SHIFT_DS_GC_G107     (24)
#define NE1_SHIFT_DS_GC_G108     (32)
#define NE1_SHIFT_DS_GC_G109     (40)
#define NE1_SHIFT_DS_GC_G110     (48)
#define NE1_SHIFT_DS_GC_G111     (56)
#define NE1_SHIFT_DS_GC_G112     (0)
#define NE1_SHIFT_DS_GC_G113     (8)
#define NE1_SHIFT_DS_GC_G114     (16)
#define NE1_SHIFT_DS_GC_G115     (24)
#define NE1_SHIFT_DS_GC_G116     (32)
#define NE1_SHIFT_DS_GC_G117     (40)
#define NE1_SHIFT_DS_GC_G118     (48)
#define NE1_SHIFT_DS_GC_G119     (56)
#define NE1_SHIFT_DS_GC_G120     (0)
#define NE1_SHIFT_DS_GC_G121     (8)
#define NE1_SHIFT_DS_GC_G122     (16)
#define NE1_SHIFT_DS_GC_G123     (24)
#define NE1_SHIFT_DS_GC_G124     (32)
#define NE1_SHIFT_DS_GC_G125     (40)
#define NE1_SHIFT_DS_GC_G126     (48)
#define NE1_SHIFT_DS_GC_G127     (56)
#define NE1_SHIFT_DS_GC_G128     (0)
#define NE1_SHIFT_DS_GC_G129     (8)
#define NE1_SHIFT_DS_GC_G130     (16)
#define NE1_SHIFT_DS_GC_G131     (24)
#define NE1_SHIFT_DS_GC_G132     (32)
#define NE1_SHIFT_DS_GC_G133     (40)
#define NE1_SHIFT_DS_GC_G134     (48)
#define NE1_SHIFT_DS_GC_G135     (56)
#define NE1_SHIFT_DS_GC_G136     (0)
#define NE1_SHIFT_DS_GC_G137     (8)
#define NE1_SHIFT_DS_GC_G138     (16)
#define NE1_SHIFT_DS_GC_G139     (24)
#define NE1_SHIFT_DS_GC_G140     (32)
#define NE1_SHIFT_DS_GC_G141     (40)
#define NE1_SHIFT_DS_GC_G142     (48)
#define NE1_SHIFT_DS_GC_G143     (56)
#define NE1_SHIFT_DS_GC_G144     (0)
#define NE1_SHIFT_DS_GC_G145     (8)
#define NE1_SHIFT_DS_GC_G146     (16)
#define NE1_SHIFT_DS_GC_G147     (24)
#define NE1_SHIFT_DS_GC_G148     (32)
#define NE1_SHIFT_DS_GC_G149     (40)
#define NE1_SHIFT_DS_GC_G150     (48)
#define NE1_SHIFT_DS_GC_G151     (56)
#define NE1_SHIFT_DS_GC_G152     (0)
#define NE1_SHIFT_DS_GC_G153     (8)
#define NE1_SHIFT_DS_GC_G154     (16)
#define NE1_SHIFT_DS_GC_G155     (24)
#define NE1_SHIFT_DS_GC_G156     (32)
#define NE1_SHIFT_DS_GC_G157     (40)
#define NE1_SHIFT_DS_GC_G158     (48)
#define NE1_SHIFT_DS_GC_G159     (56)
#define NE1_SHIFT_DS_GC_G160     (0)
#define NE1_SHIFT_DS_GC_G161     (8)
#define NE1_SHIFT_DS_GC_G162     (16)
#define NE1_SHIFT_DS_GC_G163     (24)
#define NE1_SHIFT_DS_GC_G164     (32)
#define NE1_SHIFT_DS_GC_G165     (40)
#define NE1_SHIFT_DS_GC_G166     (48)
#define NE1_SHIFT_DS_GC_G167     (56)
#define NE1_SHIFT_DS_GC_G168     (0)
#define NE1_SHIFT_DS_GC_G169     (8)
#define NE1_SHIFT_DS_GC_G170     (16)
#define NE1_SHIFT_DS_GC_G171     (24)
#define NE1_SHIFT_DS_GC_G172     (32)
#define NE1_SHIFT_DS_GC_G173     (40)
#define NE1_SHIFT_DS_GC_G174     (48)
#define NE1_SHIFT_DS_GC_G175     (56)
#define NE1_SHIFT_DS_GC_G176     (0)
#define NE1_SHIFT_DS_GC_G177     (8)
#define NE1_SHIFT_DS_GC_G178     (16)
#define NE1_SHIFT_DS_GC_G179     (24)
#define NE1_SHIFT_DS_GC_G180     (32)
#define NE1_SHIFT_DS_GC_G181     (40)
#define NE1_SHIFT_DS_GC_G182     (48)
#define NE1_SHIFT_DS_GC_G183     (56)
#define NE1_SHIFT_DS_GC_G184     (0)
#define NE1_SHIFT_DS_GC_G185     (8)
#define NE1_SHIFT_DS_GC_G186     (16)
#define NE1_SHIFT_DS_GC_G187     (24)
#define NE1_SHIFT_DS_GC_G188     (32)
#define NE1_SHIFT_DS_GC_G189     (40)
#define NE1_SHIFT_DS_GC_G190     (48)
#define NE1_SHIFT_DS_GC_G191     (56)
#define NE1_SHIFT_DS_GC_G192     (0)
#define NE1_SHIFT_DS_GC_G193     (8)
#define NE1_SHIFT_DS_GC_G194     (16)
#define NE1_SHIFT_DS_GC_G195     (24)
#define NE1_SHIFT_DS_GC_G196     (32)
#define NE1_SHIFT_DS_GC_G197     (40)
#define NE1_SHIFT_DS_GC_G198     (48)
#define NE1_SHIFT_DS_GC_G199     (56)
#define NE1_SHIFT_DS_GC_G200     (0)
#define NE1_SHIFT_DS_GC_G201     (8)
#define NE1_SHIFT_DS_GC_G202     (16)
#define NE1_SHIFT_DS_GC_G203     (24)
#define NE1_SHIFT_DS_GC_G204     (32)
#define NE1_SHIFT_DS_GC_G205     (40)
#define NE1_SHIFT_DS_GC_G206     (48)
#define NE1_SHIFT_DS_GC_G207     (56)
#define NE1_SHIFT_DS_GC_G208     (0)
#define NE1_SHIFT_DS_GC_G209     (8)
#define NE1_SHIFT_DS_GC_G210     (16)
#define NE1_SHIFT_DS_GC_G211     (24)
#define NE1_SHIFT_DS_GC_G212     (32)
#define NE1_SHIFT_DS_GC_G213     (40)
#define NE1_SHIFT_DS_GC_G214     (48)
#define NE1_SHIFT_DS_GC_G215     (56)
#define NE1_SHIFT_DS_GC_G216     (0)
#define NE1_SHIFT_DS_GC_G217     (8)
#define NE1_SHIFT_DS_GC_G218     (16)
#define NE1_SHIFT_DS_GC_G219     (24)
#define NE1_SHIFT_DS_GC_G220     (32)
#define NE1_SHIFT_DS_GC_G221     (40)
#define NE1_SHIFT_DS_GC_G222     (48)
#define NE1_SHIFT_DS_GC_G223     (56)
#define NE1_SHIFT_DS_GC_G224     (0)
#define NE1_SHIFT_DS_GC_G225     (8)
#define NE1_SHIFT_DS_GC_G226     (16)
#define NE1_SHIFT_DS_GC_G227     (24)
#define NE1_SHIFT_DS_GC_G228     (32)
#define NE1_SHIFT_DS_GC_G229     (40)
#define NE1_SHIFT_DS_GC_G230     (48)
#define NE1_SHIFT_DS_GC_G231     (56)
#define NE1_SHIFT_DS_GC_G232     (0)
#define NE1_SHIFT_DS_GC_G233     (8)
#define NE1_SHIFT_DS_GC_G234     (16)
#define NE1_SHIFT_DS_GC_G235     (24)
#define NE1_SHIFT_DS_GC_G236     (32)
#define NE1_SHIFT_DS_GC_G237     (40)
#define NE1_SHIFT_DS_GC_G238     (48)
#define NE1_SHIFT_DS_GC_G239     (56)
#define NE1_SHIFT_DS_GC_G240     (0)
#define NE1_SHIFT_DS_GC_G241     (8)
#define NE1_SHIFT_DS_GC_G242     (16)
#define NE1_SHIFT_DS_GC_G243     (24)
#define NE1_SHIFT_DS_GC_G244     (32)
#define NE1_SHIFT_DS_GC_G245     (40)
#define NE1_SHIFT_DS_GC_G246     (48)
#define NE1_SHIFT_DS_GC_G247     (56)
#define NE1_SHIFT_DS_GC_G248     (0)
#define NE1_SHIFT_DS_GC_G249     (8)
#define NE1_SHIFT_DS_GC_G250     (16)
#define NE1_SHIFT_DS_GC_G251     (24)
#define NE1_SHIFT_DS_GC_G252     (32)
#define NE1_SHIFT_DS_GC_G253     (40)
#define NE1_SHIFT_DS_GC_G254     (48)
#define NE1_SHIFT_DS_GC_G255     (56)
#define NE1_SHIFT_DS_GC_B0       (0)
#define NE1_SHIFT_DS_GC_B1       (8)
#define NE1_SHIFT_DS_GC_B2       (16)
#define NE1_SHIFT_DS_GC_B3       (24)
#define NE1_SHIFT_DS_GC_B4       (32)
#define NE1_SHIFT_DS_GC_B5       (40)
#define NE1_SHIFT_DS_GC_B6       (48)
#define NE1_SHIFT_DS_GC_B7       (56)
#define NE1_SHIFT_DS_GC_B8       (0)
#define NE1_SHIFT_DS_GC_B9       (8)
#define NE1_SHIFT_DS_GC_B10      (16)
#define NE1_SHIFT_DS_GC_B11      (24)
#define NE1_SHIFT_DS_GC_B12      (32)
#define NE1_SHIFT_DS_GC_B13      (40)
#define NE1_SHIFT_DS_GC_B14      (48)
#define NE1_SHIFT_DS_GC_B15      (56)
#define NE1_SHIFT_DS_GC_B16      (0)
#define NE1_SHIFT_DS_GC_B17      (8)
#define NE1_SHIFT_DS_GC_B18      (16)
#define NE1_SHIFT_DS_GC_B19      (24)
#define NE1_SHIFT_DS_GC_B20      (32)
#define NE1_SHIFT_DS_GC_B21      (40)
#define NE1_SHIFT_DS_GC_B22      (48)
#define NE1_SHIFT_DS_GC_B23      (56)
#define NE1_SHIFT_DS_GC_B24      (0)
#define NE1_SHIFT_DS_GC_B25      (8)
#define NE1_SHIFT_DS_GC_B26      (16)
#define NE1_SHIFT_DS_GC_B27      (24)
#define NE1_SHIFT_DS_GC_B28      (32)
#define NE1_SHIFT_DS_GC_B29      (40)
#define NE1_SHIFT_DS_GC_B30      (48)
#define NE1_SHIFT_DS_GC_B31      (56)
#define NE1_SHIFT_DS_GC_B32      (0)
#define NE1_SHIFT_DS_GC_B33      (8)
#define NE1_SHIFT_DS_GC_B34      (16)
#define NE1_SHIFT_DS_GC_B35      (24)
#define NE1_SHIFT_DS_GC_B36      (32)
#define NE1_SHIFT_DS_GC_B37      (40)
#define NE1_SHIFT_DS_GC_B38      (48)
#define NE1_SHIFT_DS_GC_B39      (56)
#define NE1_SHIFT_DS_GC_B40      (0)
#define NE1_SHIFT_DS_GC_B41      (8)
#define NE1_SHIFT_DS_GC_B42      (16)
#define NE1_SHIFT_DS_GC_B43      (24)
#define NE1_SHIFT_DS_GC_B44      (32)
#define NE1_SHIFT_DS_GC_B45      (40)
#define NE1_SHIFT_DS_GC_B46      (48)
#define NE1_SHIFT_DS_GC_B47      (56)
#define NE1_SHIFT_DS_GC_B48      (0)
#define NE1_SHIFT_DS_GC_B49      (8)
#define NE1_SHIFT_DS_GC_B50      (16)
#define NE1_SHIFT_DS_GC_B51      (24)
#define NE1_SHIFT_DS_GC_B52      (32)
#define NE1_SHIFT_DS_GC_B53      (40)
#define NE1_SHIFT_DS_GC_B54      (48)
#define NE1_SHIFT_DS_GC_B55      (56)
#define NE1_SHIFT_DS_GC_B56      (0)
#define NE1_SHIFT_DS_GC_B57      (8)
#define NE1_SHIFT_DS_GC_B58      (16)
#define NE1_SHIFT_DS_GC_B59      (24)
#define NE1_SHIFT_DS_GC_B60      (32)
#define NE1_SHIFT_DS_GC_B61      (40)
#define NE1_SHIFT_DS_GC_B62      (48)
#define NE1_SHIFT_DS_GC_B63      (56)
#define NE1_SHIFT_DS_GC_B64      (0)
#define NE1_SHIFT_DS_GC_B65      (8)
#define NE1_SHIFT_DS_GC_B66      (16)
#define NE1_SHIFT_DS_GC_B67      (24)
#define NE1_SHIFT_DS_GC_B68      (32)
#define NE1_SHIFT_DS_GC_B69      (40)
#define NE1_SHIFT_DS_GC_B70      (48)
#define NE1_SHIFT_DS_GC_B71      (56)
#define NE1_SHIFT_DS_GC_B72      (0)
#define NE1_SHIFT_DS_GC_B73      (8)
#define NE1_SHIFT_DS_GC_B74      (16)
#define NE1_SHIFT_DS_GC_B75      (24)
#define NE1_SHIFT_DS_GC_B76      (32)
#define NE1_SHIFT_DS_GC_B77      (40)
#define NE1_SHIFT_DS_GC_B78      (48)
#define NE1_SHIFT_DS_GC_B79      (56)
#define NE1_SHIFT_DS_GC_B80      (0)
#define NE1_SHIFT_DS_GC_B81      (8)
#define NE1_SHIFT_DS_GC_B82      (16)
#define NE1_SHIFT_DS_GC_B83      (24)
#define NE1_SHIFT_DS_GC_B84      (32)
#define NE1_SHIFT_DS_GC_B85      (40)
#define NE1_SHIFT_DS_GC_B86      (48)
#define NE1_SHIFT_DS_GC_B87      (56)
#define NE1_SHIFT_DS_GC_B88      (0)
#define NE1_SHIFT_DS_GC_B89      (8)
#define NE1_SHIFT_DS_GC_B90      (16)
#define NE1_SHIFT_DS_GC_B91      (24)
#define NE1_SHIFT_DS_GC_B92      (32)
#define NE1_SHIFT_DS_GC_B93      (40)
#define NE1_SHIFT_DS_GC_B94      (48)
#define NE1_SHIFT_DS_GC_B95      (56)
#define NE1_SHIFT_DS_GC_B96      (0)
#define NE1_SHIFT_DS_GC_B97      (8)
#define NE1_SHIFT_DS_GC_B98      (16)
#define NE1_SHIFT_DS_GC_B99      (24)
#define NE1_SHIFT_DS_GC_B100     (32)
#define NE1_SHIFT_DS_GC_B101     (40)
#define NE1_SHIFT_DS_GC_B102     (48)
#define NE1_SHIFT_DS_GC_B103     (56)
#define NE1_SHIFT_DS_GC_B104     (0)
#define NE1_SHIFT_DS_GC_B105     (8)
#define NE1_SHIFT_DS_GC_B106     (16)
#define NE1_SHIFT_DS_GC_B107     (24)
#define NE1_SHIFT_DS_GC_B108     (32)
#define NE1_SHIFT_DS_GC_B109     (40)
#define NE1_SHIFT_DS_GC_B110     (48)
#define NE1_SHIFT_DS_GC_B111     (56)
#define NE1_SHIFT_DS_GC_B112     (0)
#define NE1_SHIFT_DS_GC_B113     (8)
#define NE1_SHIFT_DS_GC_B114     (16)
#define NE1_SHIFT_DS_GC_B115     (24)
#define NE1_SHIFT_DS_GC_B116     (32)
#define NE1_SHIFT_DS_GC_B117     (40)
#define NE1_SHIFT_DS_GC_B118     (48)
#define NE1_SHIFT_DS_GC_B119     (56)
#define NE1_SHIFT_DS_GC_B120     (0)
#define NE1_SHIFT_DS_GC_B121     (8)
#define NE1_SHIFT_DS_GC_B122     (16)
#define NE1_SHIFT_DS_GC_B123     (24)
#define NE1_SHIFT_DS_GC_B124     (32)
#define NE1_SHIFT_DS_GC_B125     (40)
#define NE1_SHIFT_DS_GC_B126     (48)
#define NE1_SHIFT_DS_GC_B127     (56)
#define NE1_SHIFT_DS_GC_B128     (0)
#define NE1_SHIFT_DS_GC_B129     (8)
#define NE1_SHIFT_DS_GC_B130     (16)
#define NE1_SHIFT_DS_GC_B131     (24)
#define NE1_SHIFT_DS_GC_B132     (32)
#define NE1_SHIFT_DS_GC_B133     (40)
#define NE1_SHIFT_DS_GC_B134     (48)
#define NE1_SHIFT_DS_GC_B135     (56)
#define NE1_SHIFT_DS_GC_B136     (0)
#define NE1_SHIFT_DS_GC_B137     (8)
#define NE1_SHIFT_DS_GC_B138     (16)
#define NE1_SHIFT_DS_GC_B139     (24)
#define NE1_SHIFT_DS_GC_B140     (32)
#define NE1_SHIFT_DS_GC_B141     (40)
#define NE1_SHIFT_DS_GC_B142     (48)
#define NE1_SHIFT_DS_GC_B143     (56)
#define NE1_SHIFT_DS_GC_B144     (0)
#define NE1_SHIFT_DS_GC_B145     (8)
#define NE1_SHIFT_DS_GC_B146     (16)
#define NE1_SHIFT_DS_GC_B147     (24)
#define NE1_SHIFT_DS_GC_B148     (32)
#define NE1_SHIFT_DS_GC_B149     (40)
#define NE1_SHIFT_DS_GC_B150     (48)
#define NE1_SHIFT_DS_GC_B151     (56)
#define NE1_SHIFT_DS_GC_B152     (0)
#define NE1_SHIFT_DS_GC_B153     (8)
#define NE1_SHIFT_DS_GC_B154     (16)
#define NE1_SHIFT_DS_GC_B155     (24)
#define NE1_SHIFT_DS_GC_B156     (32)
#define NE1_SHIFT_DS_GC_B157     (40)
#define NE1_SHIFT_DS_GC_B158     (48)
#define NE1_SHIFT_DS_GC_B159     (56)
#define NE1_SHIFT_DS_GC_B160     (0)
#define NE1_SHIFT_DS_GC_B161     (8)
#define NE1_SHIFT_DS_GC_B162     (16)
#define NE1_SHIFT_DS_GC_B163     (24)
#define NE1_SHIFT_DS_GC_B164     (32)
#define NE1_SHIFT_DS_GC_B165     (40)
#define NE1_SHIFT_DS_GC_B166     (48)
#define NE1_SHIFT_DS_GC_B167     (56)
#define NE1_SHIFT_DS_GC_B168     (0)
#define NE1_SHIFT_DS_GC_B169     (8)
#define NE1_SHIFT_DS_GC_B170     (16)
#define NE1_SHIFT_DS_GC_B171     (24)
#define NE1_SHIFT_DS_GC_B172     (32)
#define NE1_SHIFT_DS_GC_B173     (40)
#define NE1_SHIFT_DS_GC_B174     (48)
#define NE1_SHIFT_DS_GC_B175     (56)
#define NE1_SHIFT_DS_GC_B176     (0)
#define NE1_SHIFT_DS_GC_B177     (8)
#define NE1_SHIFT_DS_GC_B178     (16)
#define NE1_SHIFT_DS_GC_B179     (24)
#define NE1_SHIFT_DS_GC_B180     (32)
#define NE1_SHIFT_DS_GC_B181     (40)
#define NE1_SHIFT_DS_GC_B182     (48)
#define NE1_SHIFT_DS_GC_B183     (56)
#define NE1_SHIFT_DS_GC_B184     (0)
#define NE1_SHIFT_DS_GC_B185     (8)
#define NE1_SHIFT_DS_GC_B186     (16)
#define NE1_SHIFT_DS_GC_B187     (24)
#define NE1_SHIFT_DS_GC_B188     (32)
#define NE1_SHIFT_DS_GC_B189     (40)
#define NE1_SHIFT_DS_GC_B190     (48)
#define NE1_SHIFT_DS_GC_B191     (56)
#define NE1_SHIFT_DS_GC_B192     (0)
#define NE1_SHIFT_DS_GC_B193     (8)
#define NE1_SHIFT_DS_GC_B194     (16)
#define NE1_SHIFT_DS_GC_B195     (24)
#define NE1_SHIFT_DS_GC_B196     (32)
#define NE1_SHIFT_DS_GC_B197     (40)
#define NE1_SHIFT_DS_GC_B198     (48)
#define NE1_SHIFT_DS_GC_B199     (56)
#define NE1_SHIFT_DS_GC_B200     (0)
#define NE1_SHIFT_DS_GC_B201     (8)
#define NE1_SHIFT_DS_GC_B202     (16)
#define NE1_SHIFT_DS_GC_B203     (24)
#define NE1_SHIFT_DS_GC_B204     (32)
#define NE1_SHIFT_DS_GC_B205     (40)
#define NE1_SHIFT_DS_GC_B206     (48)
#define NE1_SHIFT_DS_GC_B207     (56)
#define NE1_SHIFT_DS_GC_B208     (0)
#define NE1_SHIFT_DS_GC_B209     (8)
#define NE1_SHIFT_DS_GC_B210     (16)
#define NE1_SHIFT_DS_GC_B211     (24)
#define NE1_SHIFT_DS_GC_B212     (32)
#define NE1_SHIFT_DS_GC_B213     (40)
#define NE1_SHIFT_DS_GC_B214     (48)
#define NE1_SHIFT_DS_GC_B215     (56)
#define NE1_SHIFT_DS_GC_B216     (0)
#define NE1_SHIFT_DS_GC_B217     (8)
#define NE1_SHIFT_DS_GC_B218     (16)
#define NE1_SHIFT_DS_GC_B219     (24)
#define NE1_SHIFT_DS_GC_B220     (32)
#define NE1_SHIFT_DS_GC_B221     (40)
#define NE1_SHIFT_DS_GC_B222     (48)
#define NE1_SHIFT_DS_GC_B223     (56)
#define NE1_SHIFT_DS_GC_B224     (0)
#define NE1_SHIFT_DS_GC_B225     (8)
#define NE1_SHIFT_DS_GC_B226     (16)
#define NE1_SHIFT_DS_GC_B227     (24)
#define NE1_SHIFT_DS_GC_B228     (32)
#define NE1_SHIFT_DS_GC_B229     (40)
#define NE1_SHIFT_DS_GC_B230     (48)
#define NE1_SHIFT_DS_GC_B231     (56)
#define NE1_SHIFT_DS_GC_B232     (0)
#define NE1_SHIFT_DS_GC_B233     (8)
#define NE1_SHIFT_DS_GC_B234     (16)
#define NE1_SHIFT_DS_GC_B235     (24)
#define NE1_SHIFT_DS_GC_B236     (32)
#define NE1_SHIFT_DS_GC_B237     (40)
#define NE1_SHIFT_DS_GC_B238     (48)
#define NE1_SHIFT_DS_GC_B239     (56)
#define NE1_SHIFT_DS_GC_B240     (0)
#define NE1_SHIFT_DS_GC_B241     (8)
#define NE1_SHIFT_DS_GC_B242     (16)
#define NE1_SHIFT_DS_GC_B243     (24)
#define NE1_SHIFT_DS_GC_B244     (32)
#define NE1_SHIFT_DS_GC_B245     (40)
#define NE1_SHIFT_DS_GC_B246     (48)
#define NE1_SHIFT_DS_GC_B247     (56)
#define NE1_SHIFT_DS_GC_B248     (0)
#define NE1_SHIFT_DS_GC_B249     (8)
#define NE1_SHIFT_DS_GC_B250     (16)
#define NE1_SHIFT_DS_GC_B251     (24)
#define NE1_SHIFT_DS_GC_B252     (32)
#define NE1_SHIFT_DS_GC_B253     (40)
#define NE1_SHIFT_DS_GC_B254     (48)
#define NE1_SHIFT_DS_GC_B255     (56)

/**
 *   Macro function
 */
#define NE1_GET_REG(r,m,s)        (((r) & (m)) >> (s))

#define NE1_GET_IC_SS(r)        (((r) & NE1_MASK_IC_SS) >> 0)
#define NE1_GET_IC_CS(r)        (((r) & NE1_MASK_IC_CS) >> 1)
#define NE1_GET_IC_SC(r)        (((r) & NE1_MASK_IC_SC) >> 2)
#define NE1_GET_IC_CP(r)        (((r) & NE1_MASK_IC_CP) >> 3)
#define NE1_GET_IC_HP(r)        (((r) & NE1_MASK_IC_HP) >> 4)
#define NE1_GET_IC_EHP(r)       (((r) & NE1_MASK_IC_EHP) >> 5)
#define NE1_GET_IC_VP(r)        (((r) & NE1_MASK_IC_VP) >> 6)
#define NE1_GET_IC_EVP(r)       (((r) & NE1_MASK_IC_EVP) >> 7)
#define NE1_GET_IC_DEP(r)       (((r) & NE1_MASK_IC_DEP) >> 8)
#define NE1_GET_IC_CDP(r)       (((r) & NE1_MASK_IC_CDP) >> 9)
#define NE1_GET_IC_DD(r)        (((r) & NE1_MASK_IC_DD) >> 16)
#define NE1_GET_IC_CDD(r)       (((r) & NE1_MASK_IC_CDD) >> 20)
#define NE1_GET_IC_IS(r)        (((r) & NE1_MASK_IC_IS) >> 0)
#define NE1_GET_IC_VCNT(r)      (((r) & NE1_MASK_IC_VCNT) >> 8)
#define NE1_GET_IC_VSS(r)       (((r) & NE1_MASK_IC_VSS) >> 22)
#define NE1_GET_IC_VBS(r)       (((r) & NE1_MASK_IC_VBS) >> 23)
#define NE1_GET_IC_ICL(r)       (((r) & NE1_MASK_IC_ICL) >> 0)
#define NE1_GET_IC_IP(r)        (((r) & NE1_MASK_IC_IP) >> 0)
#define NE1_GET_IC_IL(r)        (((r) & NE1_MASK_IC_IL) >> 0)
#define NE1_GET_IC_AL_H(r)      (((r) & NE1_MASK_IC_AL_H) >> 0)
#define NE1_GET_IC_ISEL0(r)     (((r) & NE1_MASK_IC_ISEL0) >> 0)
#define NE1_GET_IC_IF0(r)       (((r) & NE1_MASK_IC_IF0) >> 4)
#define NE1_GET_IC_ISEL1(r)     (((r) & NE1_MASK_IC_ISEL1) >> 8)
#define NE1_GET_IC_IF1(r)       (((r) & NE1_MASK_IC_IF1) >> 12)
#define NE1_GET_IC_ISEL2(r)     (((r) & NE1_MASK_IC_ISEL2) >> 16)
#define NE1_GET_IC_IF2(r)       (((r) & NE1_MASK_IC_IF2) >> 20)
#define NE1_GET_IC_ISEL3(r)     (((r) & NE1_MASK_IC_ISEL3) >> 24)
#define NE1_GET_IC_IF3(r)       (((r) & NE1_MASK_IC_IF3) >> 28)
#define NE1_GET_IC_ISEL4(r)     (((r) & NE1_MASK_IC_ISEL4) >> 32)
#define NE1_GET_IC_IF4(r)       (((r) & NE1_MASK_IC_IF4) >> 36)
#define NE1_GET_IC_ISEL5(r)     (((r) & NE1_MASK_IC_ISEL5) >> 40)
#define NE1_GET_IC_IF5(r)       (((r) & NE1_MASK_IC_IF5) >> 44)
#define NE1_GET_IC_ISEL6(r)     (((r) & NE1_MASK_IC_ISEL6) >> 48)
#define NE1_GET_IC_IF6(r)       (((r) & NE1_MASK_IC_IF6) >> 52)
#define NE1_GET_DS_HR(r)        (((r) & NE1_MASK_DS_HR) >> 0)
#define NE1_GET_DS_VR(r)        (((r) & NE1_MASK_DS_VR) >> 16)
#define NE1_GET_DS_TH(r)        (((r) & NE1_MASK_DS_TH) >> 32)
#define NE1_GET_DS_TV(r)        (((r) & NE1_MASK_DS_TV) >> 48)
#define NE1_GET_DS_THP(r)       (((r) & NE1_MASK_DS_THP) >> 0)
#define NE1_GET_DS_TVP(r)       (((r) & NE1_MASK_DS_TVP) >> 16)
#define NE1_GET_DS_THB(r)       (((r) & NE1_MASK_DS_THB) >> 32)
#define NE1_GET_DS_TVB(r)       (((r) & NE1_MASK_DS_TVB) >> 48)
#define NE1_GET_DS_OC(r)        (((r) & NE1_MASK_DS_OC) >> 0)
#define NE1_GET_DS_DS(r)        (((r) & NE1_MASK_DS_DS) >> 4)
#define NE1_GET_DS_DM_H(r)      (((r) & NE1_MASK_DS_DM_H) >> 6)
#define NE1_GET_DS_DM_V(r)      (((r) & NE1_MASK_DS_DM_V) >> 7)
#define NE1_GET_DS_GS(r)        (((r) & NE1_MASK_DS_GS) >> 15)
#define NE1_GET_DS_DC0(r)       (((r) & NE1_MASK_DS_DC0) >> 0)
#define NE1_GET_DS_DC1(r)       (((r) & NE1_MASK_DS_DC1) >> 4)
#define NE1_GET_DS_DC2(r)       (((r) & NE1_MASK_DS_DC2) >> 8)
#define NE1_GET_DS_DC3(r)       (((r) & NE1_MASK_DS_DC3) >> 12)
#define NE1_GET_DS_DC4(r)       (((r) & NE1_MASK_DS_DC4) >> 16)
#define NE1_GET_DS_DC5(r)       (((r) & NE1_MASK_DS_DC5) >> 20)
#define NE1_GET_DS_DC6(r)       (((r) & NE1_MASK_DS_DC6) >> 24)
#define NE1_GET_DS_DC7(r)       (((r) & NE1_MASK_DS_DC7) >> 28)
#define NE1_GET_DS_DC8(r)       (((r) & NE1_MASK_DS_DC8) >> 32)
#define NE1_GET_DS_DC9(r)       (((r) & NE1_MASK_DS_DC9) >> 36)
#define NE1_GET_DS_DC10(r)      (((r) & NE1_MASK_DS_DC10) >> 40)
#define NE1_GET_DS_DC11(r)      (((r) & NE1_MASK_DS_DC11) >> 44)
#define NE1_GET_DS_DC12(r)      (((r) & NE1_MASK_DS_DC12) >> 48)
#define NE1_GET_DS_DC13(r)      (((r) & NE1_MASK_DS_DC13) >> 52)
#define NE1_GET_DS_DC14(r)      (((r) & NE1_MASK_DS_DC14) >> 56)
#define NE1_GET_DS_DC15(r)      (((r) & NE1_MASK_DS_DC15) >> 60)
#define NE1_GET_DS_BC(r)        (((r) & NE1_MASK_DS_BC) >> 0)
#define NE1_GET_MF_ES0(r)       (((r) & NE1_MASK_MF_ES0) >> 0)
#define NE1_GET_MF_ES1(r)       (((r) & NE1_MASK_MF_ES1) >> 2)
#define NE1_GET_MF_ES2(r)       (((r) & NE1_MASK_MF_ES2) >> 4)
#define NE1_GET_MF_ES3(r)       (((r) & NE1_MASK_MF_ES3) >> 6)
#define NE1_GET_MF_ES4(r)       (((r) & NE1_MASK_MF_ES4) >> 8)
#define NE1_GET_MF_ES5(r)       (((r) & NE1_MASK_MF_ES5) >> 10)
#define NE1_GET_MF_ES6(r)       (((r) & NE1_MASK_MF_ES6) >> 12)
#define NE1_GET_MF_BE0(r)       (((r) & NE1_MASK_MF_BE0) >> 14)
#define NE1_GET_MF_BE1(r)       (((r) & NE1_MASK_MF_BE1) >> 16)
#define NE1_GET_MF_BE2(r)       (((r) & NE1_MASK_MF_BE2) >> 17)
#define NE1_GET_MF_BE3(r)       (((r) & NE1_MASK_MF_BE3) >> 18)
#define NE1_GET_MF_BE4(r)       (((r) & NE1_MASK_MF_BE4) >> 19)
#define NE1_GET_MF_BE5(r)       (((r) & NE1_MASK_MF_BE5) >> 20)
#define NE1_GET_MF_BE6(r)       (((r) & NE1_MASK_MF_BE6) >> 21)
#define NE1_GET_MF_LBS(r)       (((r) & NE1_MASK_MF_LBS) >> 24)
#define NE1_GET_MF_PF0(r)       (((r) & NE1_MASK_MF_PF0) >> 0)
#define NE1_GET_MF_YC0(r)       (((r) & NE1_MASK_MF_YC0) >> 4)
#define NE1_GET_MF_PS0(r)       (((r) & NE1_MASK_MF_PS0) >> 8)
#define NE1_GET_MF_MS_H0(r)     (((r) & NE1_MASK_MF_MS_H0) >> 10)
#define NE1_GET_MF_MS_V0(r)     (((r) & NE1_MASK_MF_MS_V0) >> 11)
#define NE1_GET_MF_RL0(r)       (((r) & NE1_MASK_MF_RL0) >> 12)
#define NE1_GET_MF_AF0(r)       (((r) & NE1_MASK_MF_AF0) >> 15)
#define NE1_GET_MF_PF1(r)       (((r) & NE1_MASK_MF_PF1) >> 16)
#define NE1_GET_MF_YC1(r)       (((r) & NE1_MASK_MF_YC1) >> 20)
#define NE1_GET_MF_PS1(r)       (((r) & NE1_MASK_MF_PS1) >> 24)
#define NE1_GET_MF_MS_H1(r)     (((r) & NE1_MASK_MF_MS_H1) >> 26)
#define NE1_GET_MF_RL1(r)       (((r) & NE1_MASK_MF_RL1) >> 28)
#define NE1_GET_MF_AF1(r)       (((r) & NE1_MASK_MF_AF1) >> 31)
#define NE1_GET_MF_PF2(r)       (((r) & NE1_MASK_MF_PF2) >> 32)
#define NE1_GET_MF_YC2(r)       (((r) & NE1_MASK_MF_YC2) >> 36)
#define NE1_GET_MF_PS2(r)       (((r) & NE1_MASK_MF_PS2) >> 40)
#define NE1_GET_MF_MS_H2(r)     (((r) & NE1_MASK_MF_MS_H2) >> 42)
#define NE1_GET_MF_MS_V2(r)     (((r) & NE1_MASK_MF_MS_V2) >> 43)
#define NE1_GET_MF_RL2(r)       (((r) & NE1_MASK_MF_RL2) >> 44)
#define NE1_GET_MF_AF2(r)       (((r) & NE1_MASK_MF_AF2) >> 47)
#define NE1_GET_MF_PF3(r)       (((r) & NE1_MASK_MF_PF3) >> 48)
#define NE1_GET_MF_YC3(r)       (((r) & NE1_MASK_MF_YC3) >> 52)
#define NE1_GET_MF_PS3(r)       (((r) & NE1_MASK_MF_PS3) >> 56)
#define NE1_GET_MF_MS_H3(r)     (((r) & NE1_MASK_MF_MS_H3) >> 58)
#define NE1_GET_MF_RL3(r)       (((r) & NE1_MASK_MF_RL3) >> 60)
#define NE1_GET_MF_AF3(r)       (((r) & NE1_MASK_MF_AF3) >> 63)
#define NE1_GET_MF_PF4(r)       (((r) & NE1_MASK_MF_PF4) >> 0)
#define NE1_GET_MF_YC4(r)       (((r) & NE1_MASK_MF_YC4) >> 4)
#define NE1_GET_MF_PS4(r)       (((r) & NE1_MASK_MF_PS4) >> 8)
#define NE1_GET_MF_MS_H4(r)     (((r) & NE1_MASK_MF_MS_H4) >> 10)
#define NE1_GET_MF_MS_V4(r)     (((r) & NE1_MASK_MF_MS_V4) >> 11)
#define NE1_GET_MF_RL4(r)       (((r) & NE1_MASK_MF_RL4) >> 12)
#define NE1_GET_MF_AF4(r)       (((r) & NE1_MASK_MF_AF4) >> 15)
#define NE1_GET_MF_PF5(r)       (((r) & NE1_MASK_MF_PF5) >> 16)
#define NE1_GET_MF_YC5(r)       (((r) & NE1_MASK_MF_YC5) >> 20)
#define NE1_GET_MF_PS5(r)       (((r) & NE1_MASK_MF_PS5) >> 24)
#define NE1_GET_MF_MS_H5(r)     (((r) & NE1_MASK_MF_MS_H5) >> 26)
#define NE1_GET_MF_RL5(r)       (((r) & NE1_MASK_MF_RL5) >> 28)
#define NE1_GET_MF_AF5(r)       (((r) & NE1_MASK_MF_AF5) >> 31)
#define NE1_GET_MF_PF6(r)       (((r) & NE1_MASK_MF_PF6) >> 32)
#define NE1_GET_MF_YC6(r)       (((r) & NE1_MASK_MF_YC6) >> 36)
#define NE1_GET_MF_PS6(r)       (((r) & NE1_MASK_MF_PS6) >> 40)
#define NE1_GET_MF_MS_H6(r)     (((r) & NE1_MASK_MF_MS_H6) >> 42)
#define NE1_GET_MF_RL6(r)       (((r) & NE1_MASK_MF_RL6) >> 44)
#define NE1_GET_MF_AF6(r)       (((r) & NE1_MASK_MF_AF6) >> 47)
#define NE1_GET_MF_SA0(r)       (((r) & NE1_MASK_MF_SA0) >> 4)
#define NE1_GET_MF_SA1(r)       (((r) & NE1_MASK_MF_SA1) >> 36)
#define NE1_GET_MF_SA2(r)       (((r) & NE1_MASK_MF_SA2) >> 4)
#define NE1_GET_MF_SA3(r)       (((r) & NE1_MASK_MF_SA3) >> 36)
#define NE1_GET_MF_SA4(r)       (((r) & NE1_MASK_MF_SA4) >> 4)
#define NE1_GET_MF_SA5(r)       (((r) & NE1_MASK_MF_SA5) >> 36)
#define NE1_GET_MF_SA6(r)       (((r) & NE1_MASK_MF_SA6) >> 4)
#define NE1_GET_MF_WS_H0(r)     (((r) & NE1_MASK_MF_WS_H0) >> 0)
#define NE1_GET_MF_WS_H1(r)     (((r) & NE1_MASK_MF_WS_H1) >> 16)
#define NE1_GET_MF_WS_H2(r)     (((r) & NE1_MASK_MF_WS_H2) >> 32)
#define NE1_GET_MF_WS_H3(r)     (((r) & NE1_MASK_MF_WS_H3) >> 48)
#define NE1_GET_MF_WS_H4(r)     (((r) & NE1_MASK_MF_WS_H4) >> 0)
#define NE1_GET_MF_WS_H5(r)     (((r) & NE1_MASK_MF_WS_H5) >> 16)
#define NE1_GET_MF_WS_H6(r)     (((r) & NE1_MASK_MF_WS_H6) >> 32)
#define NE1_GET_MF_WS_V0(r)     (((r) & NE1_MASK_MF_WS_V0) >> 0)
#define NE1_GET_MF_WS_V1(r)     (((r) & NE1_MASK_MF_WS_V1) >> 16)
#define NE1_GET_MF_WS_V2(r)     (((r) & NE1_MASK_MF_WS_V2) >> 32)
#define NE1_GET_MF_WS_V3(r)     (((r) & NE1_MASK_MF_WS_V3) >> 48)
#define NE1_GET_MF_WS_V4(r)     (((r) & NE1_MASK_MF_WS_V4) >> 0)
#define NE1_GET_MF_WS_V5(r)     (((r) & NE1_MASK_MF_WS_V5) >> 16)
#define NE1_GET_MF_WS_V6(r)     (((r) & NE1_MASK_MF_WS_V6) >> 32)
#define NE1_GET_MF_SP_X0(r)     (((r) & NE1_MASK_MF_SP_X0) >> 0)
#define NE1_GET_MF_SP_X1(r)     (((r) & NE1_MASK_MF_SP_X1) >> 16)
#define NE1_GET_MF_SP_X2(r)     (((r) & NE1_MASK_MF_SP_X2) >> 32)
#define NE1_GET_MF_SP_X3(r)     (((r) & NE1_MASK_MF_SP_X3) >> 48)
#define NE1_GET_MF_SP_X4(r)     (((r) & NE1_MASK_MF_SP_X4) >> 0)
#define NE1_GET_MF_SP_X5(r)     (((r) & NE1_MASK_MF_SP_X5) >> 16)
#define NE1_GET_MF_SP_X6(r)     (((r) & NE1_MASK_MF_SP_X6) >> 32)
#define NE1_GET_MF_SP_Y0(r)     (((r) & NE1_MASK_MF_SP_Y0) >> 0)
#define NE1_GET_MF_SP_Y1(r)     (((r) & NE1_MASK_MF_SP_Y1) >> 16)
#define NE1_GET_MF_SP_Y2(r)     (((r) & NE1_MASK_MF_SP_Y2) >> 32)
#define NE1_GET_MF_SP_Y3(r)     (((r) & NE1_MASK_MF_SP_Y3) >> 48)
#define NE1_GET_MF_SP_Y4(r)     (((r) & NE1_MASK_MF_SP_Y4) >> 0)
#define NE1_GET_MF_SP_Y5(r)     (((r) & NE1_MASK_MF_SP_Y5) >> 16)
#define NE1_GET_MF_SP_Y6(r)     (((r) & NE1_MASK_MF_SP_Y6) >> 32)
#define NE1_GET_MF_SP_YF0(r)    (((r) & NE1_MASK_MF_SP_YF0) >> 0)
#define NE1_GET_MF_SP_YF2(r)    (((r) & NE1_MASK_MF_SP_YF2) >> 2)
#define NE1_GET_MF_SP_YF4(r)    (((r) & NE1_MASK_MF_SP_YF4) >> 4)
#define NE1_GET_MF_DS_H0(r)     (((r) & NE1_MASK_MF_DS_H0) >> 0)
#define NE1_GET_MF_DS_H1(r)     (((r) & NE1_MASK_MF_DS_H1) >> 16)
#define NE1_GET_MF_DS_H2(r)     (((r) & NE1_MASK_MF_DS_H2) >> 32)
#define NE1_GET_MF_DS_H3(r)     (((r) & NE1_MASK_MF_DS_H3) >> 48)
#define NE1_GET_MF_DS_H4(r)     (((r) & NE1_MASK_MF_DS_H4) >> 0)
#define NE1_GET_MF_DS_H5(r)     (((r) & NE1_MASK_MF_DS_H5) >> 16)
#define NE1_GET_MF_DS_H6(r)     (((r) & NE1_MASK_MF_DS_H6) >> 32)
#define NE1_GET_MF_DS_V0(r)     (((r) & NE1_MASK_MF_DS_V0) >> 0)
#define NE1_GET_MF_DS_V1(r)     (((r) & NE1_MASK_MF_DS_V1) >> 16)
#define NE1_GET_MF_DS_V2(r)     (((r) & NE1_MASK_MF_DS_V2) >> 32)
#define NE1_GET_MF_DS_V3(r)     (((r) & NE1_MASK_MF_DS_V3) >> 48)
#define NE1_GET_MF_DS_V4(r)     (((r) & NE1_MASK_MF_DS_V4) >> 0)
#define NE1_GET_MF_DS_V5(r)     (((r) & NE1_MASK_MF_DS_V5) >> 16)
#define NE1_GET_MF_DS_V6(r)     (((r) & NE1_MASK_MF_DS_V6) >> 32)
#define NE1_GET_MF_BC0(r)       (((r) & NE1_MASK_MF_BC0) >> 0)
#define NE1_GET_MF_BC_S0(r)     (((r) & NE1_MASK_MF_BC_S0) >> 28)
#define NE1_GET_MF_BC1(r)       (((r) & NE1_MASK_MF_BC1) >> 32)
#define NE1_GET_MF_BC_S1(r)     (((r) & NE1_MASK_MF_BC_S1) >> 60)
#define NE1_GET_MF_BC2(r)       (((r) & NE1_MASK_MF_BC2) >> 0)
#define NE1_GET_MF_BC_S2(r)     (((r) & NE1_MASK_MF_BC_S2) >> 28)
#define NE1_GET_MF_BC3(r)       (((r) & NE1_MASK_MF_BC3) >> 32)
#define NE1_GET_MF_BC_S3(r)     (((r) & NE1_MASK_MF_BC_S3) >> 60)
#define NE1_GET_MF_BC4(r)       (((r) & NE1_MASK_MF_BC4) >> 0)
#define NE1_GET_MF_BC_S4(r)     (((r) & NE1_MASK_MF_BC_S4) >> 28)
#define NE1_GET_MF_BC5(r)       (((r) & NE1_MASK_MF_BC5) >> 32)
#define NE1_GET_MF_BC_S5(r)     (((r) & NE1_MASK_MF_BC_S5) >> 60)
#define NE1_GET_MF_BC6(r)       (((r) & NE1_MASK_MF_BC6) >> 0)
#define NE1_GET_MF_BC_S6(r)     (((r) & NE1_MASK_MF_BC_S6) >> 28)
#define NE1_GET_CP_SA0(r)       (((r) & NE1_MASK_CP_SA0) >> 4)
#define NE1_GET_CP_SA1(r)       (((r) & NE1_MASK_CP_SA1) >> 36)
#define NE1_GET_CP_SA2(r)       (((r) & NE1_MASK_CP_SA2) >> 4)
#define NE1_GET_CP_RN0(r)       (((r) & NE1_MASK_CP_RN0) >> 0)
#define NE1_GET_CP_RN1(r)       (((r) & NE1_MASK_CP_RN1) >> 4)
#define NE1_GET_CP_RN2(r)       (((r) & NE1_MASK_CP_RN2) >> 8)
#define NE1_GET_DF_DC0(r)       (((r) & NE1_MASK_DF_DC0) >> 0)
#define NE1_GET_DF_VS0(r)       (((r) & NE1_MASK_DF_VS0) >> 8)
#define NE1_GET_DF_LC0(r)       (((r) & NE1_MASK_DF_LC0) >> 12)
#define NE1_GET_DF_DC1(r)       (((r) & NE1_MASK_DF_DC1) >> 16)
#define NE1_GET_DF_VS1(r)       (((r) & NE1_MASK_DF_VS1) >> 24)
#define NE1_GET_DF_LC1(r)       (((r) & NE1_MASK_DF_LC1) >> 28)
#define NE1_GET_DF_DC2(r)       (((r) & NE1_MASK_DF_DC2) >> 32)
#define NE1_GET_DF_VS2(r)       (((r) & NE1_MASK_DF_VS2) >> 40)
#define NE1_GET_DF_LC2(r)       (((r) & NE1_MASK_DF_LC2) >> 44)
#define NE1_GET_DF_DC3(r)       (((r) & NE1_MASK_DF_DC3) >> 48)
#define NE1_GET_DF_VS3(r)       (((r) & NE1_MASK_DF_VS3) >> 56)
#define NE1_GET_DF_LC3(r)       (((r) & NE1_MASK_DF_LC3) >> 60)
#define NE1_GET_DF_DC4(r)       (((r) & NE1_MASK_DF_DC4) >> 0)
#define NE1_GET_DF_VS4(r)       (((r) & NE1_MASK_DF_VS4) >> 8)
#define NE1_GET_DF_LC4(r)       (((r) & NE1_MASK_DF_LC4) >> 12)
#define NE1_GET_DF_DC5(r)       (((r) & NE1_MASK_DF_DC5) >> 16)
#define NE1_GET_DF_VS5(r)       (((r) & NE1_MASK_DF_VS5) >> 24)
#define NE1_GET_DF_LC5(r)       (((r) & NE1_MASK_DF_LC5) >> 28)
#define NE1_GET_DF_DC6(r)       (((r) & NE1_MASK_DF_DC6) >> 32)
#define NE1_GET_DF_VS6(r)       (((r) & NE1_MASK_DF_VS6) >> 40)
#define NE1_GET_DF_LC6(r)       (((r) & NE1_MASK_DF_LC6) >> 44)
#define NE1_GET_DF_SP_X0(r)     (((r) & NE1_MASK_DF_SP_X0) >> 0)
#define NE1_GET_DF_SP_X1(r)     (((r) & NE1_MASK_DF_SP_X1) >> 16)
#define NE1_GET_DF_SP_X2(r)     (((r) & NE1_MASK_DF_SP_X2) >> 32)
#define NE1_GET_DF_SP_X3(r)     (((r) & NE1_MASK_DF_SP_X3) >> 48)
#define NE1_GET_DF_SP_X4(r)     (((r) & NE1_MASK_DF_SP_X4) >> 0)
#define NE1_GET_DF_SP_X5(r)     (((r) & NE1_MASK_DF_SP_X5) >> 16)
#define NE1_GET_DF_SP_X6(r)     (((r) & NE1_MASK_DF_SP_X6) >> 32)
#define NE1_GET_DF_SP_Y0(r)     (((r) & NE1_MASK_DF_SP_Y0) >> 0)
#define NE1_GET_DF_SP_Y1(r)     (((r) & NE1_MASK_DF_SP_Y1) >> 16)
#define NE1_GET_DF_SP_Y2(r)     (((r) & NE1_MASK_DF_SP_Y2) >> 32)
#define NE1_GET_DF_SP_Y3(r)     (((r) & NE1_MASK_DF_SP_Y3) >> 48)
#define NE1_GET_DF_SP_Y4(r)     (((r) & NE1_MASK_DF_SP_Y4) >> 0)
#define NE1_GET_DF_SP_Y5(r)     (((r) & NE1_MASK_DF_SP_Y5) >> 16)
#define NE1_GET_DF_SP_Y6(r)     (((r) & NE1_MASK_DF_SP_Y6) >> 32)
#define NE1_GET_DF_DS_H0(r)     (((r) & NE1_MASK_DF_DS_H0) >> 0)
#define NE1_GET_DF_DS_H1(r)     (((r) & NE1_MASK_DF_DS_H1) >> 16)
#define NE1_GET_DF_DS_H2(r)     (((r) & NE1_MASK_DF_DS_H2) >> 32)
#define NE1_GET_DF_DS_H3(r)     (((r) & NE1_MASK_DF_DS_H3) >> 48)
#define NE1_GET_DF_DS_H4(r)     (((r) & NE1_MASK_DF_DS_H4) >> 0)
#define NE1_GET_DF_DS_H5(r)     (((r) & NE1_MASK_DF_DS_H5) >> 16)
#define NE1_GET_DF_DS_H6(r)     (((r) & NE1_MASK_DF_DS_H6) >> 32)
#define NE1_GET_DF_DS_V0(r)     (((r) & NE1_MASK_DF_DS_V0) >> 0)
#define NE1_GET_DF_DS_V2(r)     (((r) & NE1_MASK_DF_DS_V2) >> 32)
#define NE1_GET_DF_DS_V4(r)     (((r) & NE1_MASK_DF_DS_V4) >> 0)
#define NE1_GET_DF_RC_H0(r)     (((r) & NE1_MASK_DF_RC_H0) >> 0)
#define NE1_GET_DF_RC_H1(r)     (((r) & NE1_MASK_DF_RC_H1) >> 16)
#define NE1_GET_DF_RC_H2(r)     (((r) & NE1_MASK_DF_RC_H2) >> 32)
#define NE1_GET_DF_RC_H3(r)     (((r) & NE1_MASK_DF_RC_H3) >> 48)
#define NE1_GET_DF_RC_H4(r)     (((r) & NE1_MASK_DF_RC_H4) >> 0)
#define NE1_GET_DF_RC_H5(r)     (((r) & NE1_MASK_DF_RC_H5) >> 16)
#define NE1_GET_DF_RC_H6(r)     (((r) & NE1_MASK_DF_RC_H6) >> 32)
#define NE1_GET_DF_RC_V0(r)     (((r) & NE1_MASK_DF_RC_V0) >> 0)
#define NE1_GET_DF_RC_V2(r)     (((r) & NE1_MASK_DF_RC_V2) >> 32)
#define NE1_GET_DF_RC_V4(r)     (((r) & NE1_MASK_DF_RC_V4) >> 0)
#define NE1_GET_DF_PT_R00(r)    (((r) & NE1_MASK_DF_PT_R00) >> 0)
#define NE1_GET_DF_PT_R10(r)    (((r) & NE1_MASK_DF_PT_R10) >> 8)
#define NE1_GET_DF_PT_R01(r)    (((r) & NE1_MASK_DF_PT_R01) >> 16)
#define NE1_GET_DF_PT_R11(r)    (((r) & NE1_MASK_DF_PT_R11) >> 24)
#define NE1_GET_DF_PT_R02(r)    (((r) & NE1_MASK_DF_PT_R02) >> 32)
#define NE1_GET_DF_PT_R12(r)    (((r) & NE1_MASK_DF_PT_R12) >> 40)
#define NE1_GET_DF_PT_R03(r)    (((r) & NE1_MASK_DF_PT_R03) >> 48)
#define NE1_GET_DF_PT_R13(r)    (((r) & NE1_MASK_DF_PT_R13) >> 56)
#define NE1_GET_DF_PT_R04(r)    (((r) & NE1_MASK_DF_PT_R04) >> 0)
#define NE1_GET_DF_PT_R14(r)    (((r) & NE1_MASK_DF_PT_R14) >> 8)
#define NE1_GET_DF_PT_R05(r)    (((r) & NE1_MASK_DF_PT_R05) >> 16)
#define NE1_GET_DF_PT_R15(r)    (((r) & NE1_MASK_DF_PT_R15) >> 24)
#define NE1_GET_DF_PT_R06(r)    (((r) & NE1_MASK_DF_PT_R06) >> 32)
#define NE1_GET_DF_PT_R16(r)    (((r) & NE1_MASK_DF_PT_R16) >> 40)
#define NE1_GET_DF_LT_C0(r)     (((r) & NE1_MASK_DF_LT_C0) >> 0)
#define NE1_GET_DF_PT_C0(r)     (((r) & NE1_MASK_DF_PT_C0) >> 1)
#define NE1_GET_DF_LT_R0(r)     (((r) & NE1_MASK_DF_LT_R0) >> 8)
#define NE1_GET_DF_LT_C1(r)     (((r) & NE1_MASK_DF_LT_C1) >> 16)
#define NE1_GET_DF_PT_C1(r)     (((r) & NE1_MASK_DF_PT_C1) >> 17)
#define NE1_GET_DF_LT_R1(r)     (((r) & NE1_MASK_DF_LT_R1) >> 24)
#define NE1_GET_DF_LT_C2(r)     (((r) & NE1_MASK_DF_LT_C2) >> 32)
#define NE1_GET_DF_PT_C2(r)     (((r) & NE1_MASK_DF_PT_C2) >> 33)
#define NE1_GET_DF_LT_R2(r)     (((r) & NE1_MASK_DF_LT_R2) >> 40)
#define NE1_GET_DF_LT_C3(r)     (((r) & NE1_MASK_DF_LT_C3) >> 48)
#define NE1_GET_DF_PT_C3(r)     (((r) & NE1_MASK_DF_PT_C3) >> 49)
#define NE1_GET_DF_LT_R3(r)     (((r) & NE1_MASK_DF_LT_R3) >> 56)
#define NE1_GET_DF_LT_C4(r)     (((r) & NE1_MASK_DF_LT_C4) >> 0)
#define NE1_GET_DF_PT_C4(r)     (((r) & NE1_MASK_DF_PT_C4) >> 1)
#define NE1_GET_DF_LT_R4(r)     (((r) & NE1_MASK_DF_LT_R4) >> 8)
#define NE1_GET_DF_LT_C5(r)     (((r) & NE1_MASK_DF_LT_C5) >> 16)
#define NE1_GET_DF_PT_C5(r)     (((r) & NE1_MASK_DF_PT_C5) >> 17)
#define NE1_GET_DF_LT_R5(r)     (((r) & NE1_MASK_DF_LT_R5) >> 24)
#define NE1_GET_DF_LT_C6(r)     (((r) & NE1_MASK_DF_LT_C6) >> 32)
#define NE1_GET_DF_PT_C6(r)     (((r) & NE1_MASK_DF_PT_C6) >> 33)
#define NE1_GET_DF_LT_R6(r)     (((r) & NE1_MASK_DF_LT_R6) >> 40)
#define NE1_GET_DF_CK_C0(r)     (((r) & NE1_MASK_DF_CK_C0) >> 0)
#define NE1_GET_DF_CK_C1(r)     (((r) & NE1_MASK_DF_CK_C1) >> 1)
#define NE1_GET_DF_CK_C2(r)     (((r) & NE1_MASK_DF_CK_C2) >> 2)
#define NE1_GET_DF_CK_C3(r)     (((r) & NE1_MASK_DF_CK_C3) >> 3)
#define NE1_GET_DF_CK_C4(r)     (((r) & NE1_MASK_DF_CK_C4) >> 4)
#define NE1_GET_DF_CK_C5(r)     (((r) & NE1_MASK_DF_CK_C5) >> 5)
#define NE1_GET_DF_CK_C6(r)     (((r) & NE1_MASK_DF_CK_C6) >> 6)
#define NE1_GET_DF_CK_S0(r)     (((r) & NE1_MASK_DF_CK_S0) >> 8)
#define NE1_GET_DF_CK_S1(r)     (((r) & NE1_MASK_DF_CK_S1) >> 9)
#define NE1_GET_DF_CK_S2(r)     (((r) & NE1_MASK_DF_CK_S2) >> 10)
#define NE1_GET_DF_CK_S3(r)     (((r) & NE1_MASK_DF_CK_S3) >> 11)
#define NE1_GET_DF_CK_S4(r)     (((r) & NE1_MASK_DF_CK_S4) >> 12)
#define NE1_GET_DF_CK_S5(r)     (((r) & NE1_MASK_DF_CK_S5) >> 13)
#define NE1_GET_DF_CK_S6(r)     (((r) & NE1_MASK_DF_CK_S6) >> 14)
#define NE1_GET_DF_CK_A0(r)     (((r) & NE1_MASK_DF_CK_A0) >> 16)
#define NE1_GET_DF_CK_A1(r)     (((r) & NE1_MASK_DF_CK_A1) >> 17)
#define NE1_GET_DF_CK_A2(r)     (((r) & NE1_MASK_DF_CK_A2) >> 18)
#define NE1_GET_DF_CK_A3(r)     (((r) & NE1_MASK_DF_CK_A3) >> 19)
#define NE1_GET_DF_CK_A4(r)     (((r) & NE1_MASK_DF_CK_A4) >> 20)
#define NE1_GET_DF_CK_A5(r)     (((r) & NE1_MASK_DF_CK_A5) >> 21)
#define NE1_GET_DF_CK_A6(r)     (((r) & NE1_MASK_DF_CK_A6) >> 22)
#define NE1_GET_DF_CK0(r)       (((r) & NE1_MASK_DF_CK0) >> 0)
#define NE1_GET_DF_CK1(r)       (((r) & NE1_MASK_DF_CK1) >> 32)
#define NE1_GET_DF_CK2(r)       (((r) & NE1_MASK_DF_CK2) >> 0)
#define NE1_GET_DF_CK3(r)       (((r) & NE1_MASK_DF_CK3) >> 32)
#define NE1_GET_DF_CK4(r)       (((r) & NE1_MASK_DF_CK4) >> 0)
#define NE1_GET_DF_CK5(r)       (((r) & NE1_MASK_DF_CK5) >> 32)
#define NE1_GET_DF_CK6(r)       (((r) & NE1_MASK_DF_CK6) >> 0)
#define NE1_GET_DF_CK_PR_BV0(r) (((r) & NE1_MASK_DF_CK_PR_BV0) >> 0)
#define NE1_GET_DF_CK_PR_GY0(r) (((r) & NE1_MASK_DF_CK_PR_GY0) >> 4)
#define NE1_GET_DF_CK_PR_RU0(r) (((r) & NE1_MASK_DF_CK_PR_RU0) >> 8)
#define NE1_GET_DF_CK_PR_A0(r)  (((r) & NE1_MASK_DF_CK_PR_A0) >> 12)
#define NE1_GET_DF_CK_PR_BV1(r) (((r) & NE1_MASK_DF_CK_PR_BV1) >> 16)
#define NE1_GET_DF_CK_PR_GY1(r) (((r) & NE1_MASK_DF_CK_PR_GY1) >> 20)
#define NE1_GET_DF_CK_PR_RU1(r) (((r) & NE1_MASK_DF_CK_PR_RU1) >> 24)
#define NE1_GET_DF_CK_PR_A1(r)  (((r) & NE1_MASK_DF_CK_PR_A1) >> 28)
#define NE1_GET_DF_CK_PR_BV2(r) (((r) & NE1_MASK_DF_CK_PR_BV2) >> 32)
#define NE1_GET_DF_CK_PR_GY2(r) (((r) & NE1_MASK_DF_CK_PR_GY2) >> 36)
#define NE1_GET_DF_CK_PR_RU2(r) (((r) & NE1_MASK_DF_CK_PR_RU2) >> 40)
#define NE1_GET_DF_CK_PR_A2(r)  (((r) & NE1_MASK_DF_CK_PR_A2) >> 44)
#define NE1_GET_DF_CK_PR_BV3(r) (((r) & NE1_MASK_DF_CK_PR_BV3) >> 48)
#define NE1_GET_DF_CK_PR_GY3(r) (((r) & NE1_MASK_DF_CK_PR_GY3) >> 52)
#define NE1_GET_DF_CK_PR_RU3(r) (((r) & NE1_MASK_DF_CK_PR_RU3) >> 56)
#define NE1_GET_DF_CK_PR_A3(r)  (((r) & NE1_MASK_DF_CK_PR_A3) >> 60)
#define NE1_GET_DF_CK_PR_BV4(r) (((r) & NE1_MASK_DF_CK_PR_BV4) >> 0)
#define NE1_GET_DF_CK_PR_GY4(r) (((r) & NE1_MASK_DF_CK_PR_GY4) >> 4)
#define NE1_GET_DF_CK_PR_RU4(r) (((r) & NE1_MASK_DF_CK_PR_RU4) >> 8)
#define NE1_GET_DF_CK_PR_A4(r)  (((r) & NE1_MASK_DF_CK_PR_A4) >> 12)
#define NE1_GET_DF_CK_PR_BV5(r) (((r) & NE1_MASK_DF_CK_PR_BV5) >> 16)
#define NE1_GET_DF_CK_PR_GY5(r) (((r) & NE1_MASK_DF_CK_PR_GY5) >> 20)
#define NE1_GET_DF_CK_PR_RU5(r) (((r) & NE1_MASK_DF_CK_PR_RU5) >> 24)
#define NE1_GET_DF_CK_PR_A5(r)  (((r) & NE1_MASK_DF_CK_PR_A5) >> 28)
#define NE1_GET_DF_CK_PR_BV6(r) (((r) & NE1_MASK_DF_CK_PR_BV6) >> 32)
#define NE1_GET_DF_CK_PR_GY6(r) (((r) & NE1_MASK_DF_CK_PR_GY6) >> 36)
#define NE1_GET_DF_CK_PR_RU6(r) (((r) & NE1_MASK_DF_CK_PR_RU6) >> 40)
#define NE1_GET_DF_CK_PR_A6(r)  (((r) & NE1_MASK_DF_CK_PR_A6) >> 44)
#define NE1_GET_DF_CC_B0(r)     (((r) & NE1_MASK_DF_CC_B0) >> 0)
#define NE1_GET_DF_CC_G0(r)     (((r) & NE1_MASK_DF_CC_G0) >> 8)
#define NE1_GET_DF_CC_R0(r)     (((r) & NE1_MASK_DF_CC_R0) >> 16)
#define NE1_GET_DF_CC_B1(r)     (((r) & NE1_MASK_DF_CC_B1) >> 32)
#define NE1_GET_DF_CC_G1(r)     (((r) & NE1_MASK_DF_CC_G1) >> 40)
#define NE1_GET_DF_CC_R1(r)     (((r) & NE1_MASK_DF_CC_R1) >> 48)
#define NE1_GET_DF_CC_B2(r)     (((r) & NE1_MASK_DF_CC_B2) >> 0)
#define NE1_GET_DF_CC_G2(r)     (((r) & NE1_MASK_DF_CC_G2) >> 8)
#define NE1_GET_DF_CC_R2(r)     (((r) & NE1_MASK_DF_CC_R2) >> 16)
#define NE1_GET_DF_CC_B3(r)     (((r) & NE1_MASK_DF_CC_B3) >> 32)
#define NE1_GET_DF_CC_G3(r)     (((r) & NE1_MASK_DF_CC_G3) >> 40)
#define NE1_GET_DF_CC_R3(r)     (((r) & NE1_MASK_DF_CC_R3) >> 48)
#define NE1_GET_DF_CC_B4(r)     (((r) & NE1_MASK_DF_CC_B4) >> 0)
#define NE1_GET_DF_CC_G4(r)     (((r) & NE1_MASK_DF_CC_G4) >> 8)
#define NE1_GET_DF_CC_R4(r)     (((r) & NE1_MASK_DF_CC_R4) >> 16)
#define NE1_GET_DF_CC_B5(r)     (((r) & NE1_MASK_DF_CC_B5) >> 32)
#define NE1_GET_DF_CC_G5(r)     (((r) & NE1_MASK_DF_CC_G5) >> 40)
#define NE1_GET_DF_CC_R5(r)     (((r) & NE1_MASK_DF_CC_R5) >> 48)
#define NE1_GET_DF_CC_B6(r)     (((r) & NE1_MASK_DF_CC_B6) >> 0)
#define NE1_GET_DF_CC_G6(r)     (((r) & NE1_MASK_DF_CC_G6) >> 8)
#define NE1_GET_DF_CC_R6(r)     (((r) & NE1_MASK_DF_CC_R6) >> 16)
#define NE1_GET_DF_BC_B(r)      (((r) & NE1_MASK_DF_BC_B) >> 0)
#define NE1_GET_DF_BC_G(r)      (((r) & NE1_MASK_DF_BC_G) >> 8)
#define NE1_GET_DF_BC_R(r)      (((r) & NE1_MASK_DF_BC_R) >> 16)
#define NE1_GET_DF_CD_B(r)      (((r) & NE1_MASK_DF_CD_B) >> 0)
#define NE1_GET_DF_CD_G(r)      (((r) & NE1_MASK_DF_CD_G) >> 8)
#define NE1_GET_DF_CD_R(r)      (((r) & NE1_MASK_DF_CD_R) >> 16)
#define NE1_GET_DF_CD_PR_B(r)   (((r) & NE1_MASK_DF_CD_PR_B) >> 32)
#define NE1_GET_DF_CD_PR_G(r)   (((r) & NE1_MASK_DF_CD_PR_G) >> 36)
#define NE1_GET_DF_CD_PR_R(r)   (((r) & NE1_MASK_DF_CD_PR_R) >> 40)
#define NE1_GET_DF_CD_C(r)      (((r) & NE1_MASK_DF_CD_C) >> 63)
#define NE1_GET_HC_DS(r)        (((r) & NE1_MASK_HC_DS) >> 0)
#define NE1_GET_HC_FS(r)        (((r) & NE1_MASK_HC_FS) >> 1)
#define NE1_GET_HC_LC(r)        (((r) & NE1_MASK_HC_LC) >> 2)
#define NE1_GET_HC_SP_X(r)      (((r) & NE1_MASK_HC_SP_X) >> 16)
#define NE1_GET_HC_SP_Y(r)      (((r) & NE1_MASK_HC_SP_Y) >> 32)
#define NE1_GET_MF_WS_M0(r)     (((r) & NE1_MASK_MF_WS_M0) >> 0)
#define NE1_GET_MF_SP_M0(r)     (((r) & NE1_MASK_MF_SP_M0) >> 1)
#define NE1_GET_MF_DS_M0(r)     (((r) & NE1_MASK_MF_DS_M0) >> 2)
#define NE1_GET_DF_SP_M0(r)     (((r) & NE1_MASK_DF_SP_M0) >> 3)
#define NE1_GET_DF_DS_M0(r)     (((r) & NE1_MASK_DF_DS_M0) >> 4)
#define NE1_GET_DF_RC_M0(r)     (((r) & NE1_MASK_DF_RC_M0) >> 5)
#define NE1_GET_MF_WS_M1(r)     (((r) & NE1_MASK_MF_WS_M1) >> 8)
#define NE1_GET_MF_SP_M1(r)     (((r) & NE1_MASK_MF_SP_M1) >> 9)
#define NE1_GET_MF_DS_M1(r)     (((r) & NE1_MASK_MF_DS_M1) >> 10)
#define NE1_GET_DF_SP_M1(r)     (((r) & NE1_MASK_DF_SP_M1) >> 11)
#define NE1_GET_DF_DS_M1(r)     (((r) & NE1_MASK_DF_DS_M1) >> 12)
#define NE1_GET_DF_RC_M1(r)     (((r) & NE1_MASK_DF_RC_M1) >> 13)
#define NE1_GET_MF_WS_M2(r)     (((r) & NE1_MASK_MF_WS_M2) >> 16)
#define NE1_GET_MF_SP_M2(r)     (((r) & NE1_MASK_MF_SP_M2) >> 17)
#define NE1_GET_MF_DS_M2(r)     (((r) & NE1_MASK_MF_DS_M2) >> 18)
#define NE1_GET_DF_SP_M2(r)     (((r) & NE1_MASK_DF_SP_M2) >> 19)
#define NE1_GET_DF_DS_M2(r)     (((r) & NE1_MASK_DF_DS_M2) >> 20)
#define NE1_GET_DF_RC_M2(r)     (((r) & NE1_MASK_DF_RC_M2) >> 21)
#define NE1_GET_MF_WS_M3(r)     (((r) & NE1_MASK_MF_WS_M3) >> 24)
#define NE1_GET_MF_SP_M3(r)     (((r) & NE1_MASK_MF_SP_M3) >> 25)
#define NE1_GET_MF_DS_M3(r)     (((r) & NE1_MASK_MF_DS_M3) >> 26)
#define NE1_GET_DF_SP_M3(r)     (((r) & NE1_MASK_DF_SP_M3) >> 27)
#define NE1_GET_DF_DS_M3(r)     (((r) & NE1_MASK_DF_DS_M3) >> 28)
#define NE1_GET_DF_RC_M3(r)     (((r) & NE1_MASK_DF_RC_M3) >> 29)
#define NE1_GET_MF_WS_M4(r)     (((r) & NE1_MASK_MF_WS_M4) >> 32)
#define NE1_GET_MF_SP_M4(r)     (((r) & NE1_MASK_MF_SP_M4) >> 33)
#define NE1_GET_MF_DS_M4(r)     (((r) & NE1_MASK_MF_DS_M4) >> 34)
#define NE1_GET_DF_SP_M4(r)     (((r) & NE1_MASK_DF_SP_M4) >> 35)
#define NE1_GET_DF_DS_M4(r)     (((r) & NE1_MASK_DF_DS_M4) >> 36)
#define NE1_GET_DF_RC_M4(r)     (((r) & NE1_MASK_DF_RC_M4) >> 37)
#define NE1_GET_MF_WS_M5(r)     (((r) & NE1_MASK_MF_WS_M5) >> 40)
#define NE1_GET_MF_SP_M5(r)     (((r) & NE1_MASK_MF_SP_M5) >> 41)
#define NE1_GET_MF_DS_M5(r)     (((r) & NE1_MASK_MF_DS_M5) >> 42)
#define NE1_GET_DF_SP_M5(r)     (((r) & NE1_MASK_DF_SP_M5) >> 43)
#define NE1_GET_DF_DS_M5(r)     (((r) & NE1_MASK_DF_DS_M5) >> 44)
#define NE1_GET_DF_RC_M5(r)     (((r) & NE1_MASK_DF_RC_M5) >> 45)
#define NE1_GET_MF_WS_M6(r)     (((r) & NE1_MASK_MF_WS_M6) >> 48)
#define NE1_GET_MF_SP_M6(r)     (((r) & NE1_MASK_MF_SP_M6) >> 49)
#define NE1_GET_MF_DS_M6(r)     (((r) & NE1_MASK_MF_DS_M6) >> 50)
#define NE1_GET_DF_SP_M6(r)     (((r) & NE1_MASK_DF_SP_M6) >> 51)
#define NE1_GET_DF_DS_M6(r)     (((r) & NE1_MASK_DF_DS_M6) >> 52)
#define NE1_GET_DF_RC_M6(r)     (((r) & NE1_MASK_DF_RC_M6) >> 53)
#define NE1_GET_HC_SP_M(r)      (((r) & NE1_MASK_HC_SP_M) >> 56)
#define NE1_GET_DS_GC_R0(r)     (((r) & NE1_MASK_DS_GC_R0) >> 0)
#define NE1_GET_DS_GC_R1(r)     (((r) & NE1_MASK_DS_GC_R1) >> 8)
#define NE1_GET_DS_GC_R2(r)     (((r) & NE1_MASK_DS_GC_R2) >> 16)
#define NE1_GET_DS_GC_R3(r)     (((r) & NE1_MASK_DS_GC_R3) >> 24)
#define NE1_GET_DS_GC_R4(r)     (((r) & NE1_MASK_DS_GC_R4) >> 32)
#define NE1_GET_DS_GC_R5(r)     (((r) & NE1_MASK_DS_GC_R5) >> 40)
#define NE1_GET_DS_GC_R6(r)     (((r) & NE1_MASK_DS_GC_R6) >> 48)
#define NE1_GET_DS_GC_R7(r)     (((r) & NE1_MASK_DS_GC_R7) >> 56)
#define NE1_GET_DS_GC_R8(r)     (((r) & NE1_MASK_DS_GC_R8) >> 0)
#define NE1_GET_DS_GC_R9(r)     (((r) & NE1_MASK_DS_GC_R9) >> 8)
#define NE1_GET_DS_GC_R10(r)    (((r) & NE1_MASK_DS_GC_R10) >> 16)
#define NE1_GET_DS_GC_R11(r)    (((r) & NE1_MASK_DS_GC_R11) >> 24)
#define NE1_GET_DS_GC_R12(r)    (((r) & NE1_MASK_DS_GC_R12) >> 32)
#define NE1_GET_DS_GC_R13(r)    (((r) & NE1_MASK_DS_GC_R13) >> 40)
#define NE1_GET_DS_GC_R14(r)    (((r) & NE1_MASK_DS_GC_R14) >> 48)
#define NE1_GET_DS_GC_R15(r)    (((r) & NE1_MASK_DS_GC_R15) >> 56)
#define NE1_GET_DS_GC_R16(r)    (((r) & NE1_MASK_DS_GC_R16) >> 0)
#define NE1_GET_DS_GC_R17(r)    (((r) & NE1_MASK_DS_GC_R17) >> 8)
#define NE1_GET_DS_GC_R18(r)    (((r) & NE1_MASK_DS_GC_R18) >> 16)
#define NE1_GET_DS_GC_R19(r)    (((r) & NE1_MASK_DS_GC_R19) >> 24)
#define NE1_GET_DS_GC_R20(r)    (((r) & NE1_MASK_DS_GC_R20) >> 32)
#define NE1_GET_DS_GC_R21(r)    (((r) & NE1_MASK_DS_GC_R21) >> 40)
#define NE1_GET_DS_GC_R22(r)    (((r) & NE1_MASK_DS_GC_R22) >> 48)
#define NE1_GET_DS_GC_R23(r)    (((r) & NE1_MASK_DS_GC_R23) >> 56)
#define NE1_GET_DS_GC_R24(r)    (((r) & NE1_MASK_DS_GC_R24) >> 0)
#define NE1_GET_DS_GC_R25(r)    (((r) & NE1_MASK_DS_GC_R25) >> 8)
#define NE1_GET_DS_GC_R26(r)    (((r) & NE1_MASK_DS_GC_R26) >> 16)
#define NE1_GET_DS_GC_R27(r)    (((r) & NE1_MASK_DS_GC_R27) >> 24)
#define NE1_GET_DS_GC_R28(r)    (((r) & NE1_MASK_DS_GC_R28) >> 32)
#define NE1_GET_DS_GC_R29(r)    (((r) & NE1_MASK_DS_GC_R29) >> 40)
#define NE1_GET_DS_GC_R30(r)    (((r) & NE1_MASK_DS_GC_R30) >> 48)
#define NE1_GET_DS_GC_R31(r)    (((r) & NE1_MASK_DS_GC_R31) >> 56)
#define NE1_GET_DS_GC_R32(r)    (((r) & NE1_MASK_DS_GC_R32) >> 0)
#define NE1_GET_DS_GC_R33(r)    (((r) & NE1_MASK_DS_GC_R33) >> 8)
#define NE1_GET_DS_GC_R34(r)    (((r) & NE1_MASK_DS_GC_R34) >> 16)
#define NE1_GET_DS_GC_R35(r)    (((r) & NE1_MASK_DS_GC_R35) >> 24)
#define NE1_GET_DS_GC_R36(r)    (((r) & NE1_MASK_DS_GC_R36) >> 32)
#define NE1_GET_DS_GC_R37(r)    (((r) & NE1_MASK_DS_GC_R37) >> 40)
#define NE1_GET_DS_GC_R38(r)    (((r) & NE1_MASK_DS_GC_R38) >> 48)
#define NE1_GET_DS_GC_R39(r)    (((r) & NE1_MASK_DS_GC_R39) >> 56)
#define NE1_GET_DS_GC_R40(r)    (((r) & NE1_MASK_DS_GC_R40) >> 0)
#define NE1_GET_DS_GC_R41(r)    (((r) & NE1_MASK_DS_GC_R41) >> 8)
#define NE1_GET_DS_GC_R42(r)    (((r) & NE1_MASK_DS_GC_R42) >> 16)
#define NE1_GET_DS_GC_R43(r)    (((r) & NE1_MASK_DS_GC_R43) >> 24)
#define NE1_GET_DS_GC_R44(r)    (((r) & NE1_MASK_DS_GC_R44) >> 32)
#define NE1_GET_DS_GC_R45(r)    (((r) & NE1_MASK_DS_GC_R45) >> 40)
#define NE1_GET_DS_GC_R46(r)    (((r) & NE1_MASK_DS_GC_R46) >> 48)
#define NE1_GET_DS_GC_R47(r)    (((r) & NE1_MASK_DS_GC_R47) >> 56)
#define NE1_GET_DS_GC_R48(r)    (((r) & NE1_MASK_DS_GC_R48) >> 0)
#define NE1_GET_DS_GC_R49(r)    (((r) & NE1_MASK_DS_GC_R49) >> 8)
#define NE1_GET_DS_GC_R50(r)    (((r) & NE1_MASK_DS_GC_R50) >> 16)
#define NE1_GET_DS_GC_R51(r)    (((r) & NE1_MASK_DS_GC_R51) >> 24)
#define NE1_GET_DS_GC_R52(r)    (((r) & NE1_MASK_DS_GC_R52) >> 32)
#define NE1_GET_DS_GC_R53(r)    (((r) & NE1_MASK_DS_GC_R53) >> 40)
#define NE1_GET_DS_GC_R54(r)    (((r) & NE1_MASK_DS_GC_R54) >> 48)
#define NE1_GET_DS_GC_R55(r)    (((r) & NE1_MASK_DS_GC_R55) >> 56)
#define NE1_GET_DS_GC_R56(r)    (((r) & NE1_MASK_DS_GC_R56) >> 0)
#define NE1_GET_DS_GC_R57(r)    (((r) & NE1_MASK_DS_GC_R57) >> 8)
#define NE1_GET_DS_GC_R58(r)    (((r) & NE1_MASK_DS_GC_R58) >> 16)
#define NE1_GET_DS_GC_R59(r)    (((r) & NE1_MASK_DS_GC_R59) >> 24)
#define NE1_GET_DS_GC_R60(r)    (((r) & NE1_MASK_DS_GC_R60) >> 32)
#define NE1_GET_DS_GC_R61(r)    (((r) & NE1_MASK_DS_GC_R61) >> 40)
#define NE1_GET_DS_GC_R62(r)    (((r) & NE1_MASK_DS_GC_R62) >> 48)
#define NE1_GET_DS_GC_R63(r)    (((r) & NE1_MASK_DS_GC_R63) >> 56)
#define NE1_GET_DS_GC_R64(r)    (((r) & NE1_MASK_DS_GC_R64) >> 0)
#define NE1_GET_DS_GC_R65(r)    (((r) & NE1_MASK_DS_GC_R65) >> 8)
#define NE1_GET_DS_GC_R66(r)    (((r) & NE1_MASK_DS_GC_R66) >> 16)
#define NE1_GET_DS_GC_R67(r)    (((r) & NE1_MASK_DS_GC_R67) >> 24)
#define NE1_GET_DS_GC_R68(r)    (((r) & NE1_MASK_DS_GC_R68) >> 32)
#define NE1_GET_DS_GC_R69(r)    (((r) & NE1_MASK_DS_GC_R69) >> 40)
#define NE1_GET_DS_GC_R70(r)    (((r) & NE1_MASK_DS_GC_R70) >> 48)
#define NE1_GET_DS_GC_R71(r)    (((r) & NE1_MASK_DS_GC_R71) >> 56)
#define NE1_GET_DS_GC_R72(r)    (((r) & NE1_MASK_DS_GC_R72) >> 0)
#define NE1_GET_DS_GC_R73(r)    (((r) & NE1_MASK_DS_GC_R73) >> 8)
#define NE1_GET_DS_GC_R74(r)    (((r) & NE1_MASK_DS_GC_R74) >> 16)
#define NE1_GET_DS_GC_R75(r)    (((r) & NE1_MASK_DS_GC_R75) >> 24)
#define NE1_GET_DS_GC_R76(r)    (((r) & NE1_MASK_DS_GC_R76) >> 32)
#define NE1_GET_DS_GC_R77(r)    (((r) & NE1_MASK_DS_GC_R77) >> 40)
#define NE1_GET_DS_GC_R78(r)    (((r) & NE1_MASK_DS_GC_R78) >> 48)
#define NE1_GET_DS_GC_R79(r)    (((r) & NE1_MASK_DS_GC_R79) >> 56)
#define NE1_GET_DS_GC_R80(r)    (((r) & NE1_MASK_DS_GC_R80) >> 0)
#define NE1_GET_DS_GC_R81(r)    (((r) & NE1_MASK_DS_GC_R81) >> 8)
#define NE1_GET_DS_GC_R82(r)    (((r) & NE1_MASK_DS_GC_R82) >> 16)
#define NE1_GET_DS_GC_R83(r)    (((r) & NE1_MASK_DS_GC_R83) >> 24)
#define NE1_GET_DS_GC_R84(r)    (((r) & NE1_MASK_DS_GC_R84) >> 32)
#define NE1_GET_DS_GC_R85(r)    (((r) & NE1_MASK_DS_GC_R85) >> 40)
#define NE1_GET_DS_GC_R86(r)    (((r) & NE1_MASK_DS_GC_R86) >> 48)
#define NE1_GET_DS_GC_R87(r)    (((r) & NE1_MASK_DS_GC_R87) >> 56)
#define NE1_GET_DS_GC_R88(r)    (((r) & NE1_MASK_DS_GC_R88) >> 0)
#define NE1_GET_DS_GC_R89(r)    (((r) & NE1_MASK_DS_GC_R89) >> 8)
#define NE1_GET_DS_GC_R90(r)    (((r) & NE1_MASK_DS_GC_R90) >> 16)
#define NE1_GET_DS_GC_R91(r)    (((r) & NE1_MASK_DS_GC_R91) >> 24)
#define NE1_GET_DS_GC_R92(r)    (((r) & NE1_MASK_DS_GC_R92) >> 32)
#define NE1_GET_DS_GC_R93(r)    (((r) & NE1_MASK_DS_GC_R93) >> 40)
#define NE1_GET_DS_GC_R94(r)    (((r) & NE1_MASK_DS_GC_R94) >> 48)
#define NE1_GET_DS_GC_R95(r)    (((r) & NE1_MASK_DS_GC_R95) >> 56)
#define NE1_GET_DS_GC_R96(r)    (((r) & NE1_MASK_DS_GC_R96) >> 0)
#define NE1_GET_DS_GC_R97(r)    (((r) & NE1_MASK_DS_GC_R97) >> 8)
#define NE1_GET_DS_GC_R98(r)    (((r) & NE1_MASK_DS_GC_R98) >> 16)
#define NE1_GET_DS_GC_R99(r)    (((r) & NE1_MASK_DS_GC_R99) >> 24)
#define NE1_GET_DS_GC_R100(r)   (((r) & NE1_MASK_DS_GC_R100) >> 32)
#define NE1_GET_DS_GC_R101(r)   (((r) & NE1_MASK_DS_GC_R101) >> 40)
#define NE1_GET_DS_GC_R102(r)   (((r) & NE1_MASK_DS_GC_R102) >> 48)
#define NE1_GET_DS_GC_R103(r)   (((r) & NE1_MASK_DS_GC_R103) >> 56)
#define NE1_GET_DS_GC_R104(r)   (((r) & NE1_MASK_DS_GC_R104) >> 0)
#define NE1_GET_DS_GC_R105(r)   (((r) & NE1_MASK_DS_GC_R105) >> 8)
#define NE1_GET_DS_GC_R106(r)   (((r) & NE1_MASK_DS_GC_R106) >> 16)
#define NE1_GET_DS_GC_R107(r)   (((r) & NE1_MASK_DS_GC_R107) >> 24)
#define NE1_GET_DS_GC_R108(r)   (((r) & NE1_MASK_DS_GC_R108) >> 32)
#define NE1_GET_DS_GC_R109(r)   (((r) & NE1_MASK_DS_GC_R109) >> 40)
#define NE1_GET_DS_GC_R110(r)   (((r) & NE1_MASK_DS_GC_R110) >> 48)
#define NE1_GET_DS_GC_R111(r)   (((r) & NE1_MASK_DS_GC_R111) >> 56)
#define NE1_GET_DS_GC_R112(r)   (((r) & NE1_MASK_DS_GC_R112) >> 0)
#define NE1_GET_DS_GC_R113(r)   (((r) & NE1_MASK_DS_GC_R113) >> 8)
#define NE1_GET_DS_GC_R114(r)   (((r) & NE1_MASK_DS_GC_R114) >> 16)
#define NE1_GET_DS_GC_R115(r)   (((r) & NE1_MASK_DS_GC_R115) >> 24)
#define NE1_GET_DS_GC_R116(r)   (((r) & NE1_MASK_DS_GC_R116) >> 32)
#define NE1_GET_DS_GC_R117(r)   (((r) & NE1_MASK_DS_GC_R117) >> 40)
#define NE1_GET_DS_GC_R118(r)   (((r) & NE1_MASK_DS_GC_R118) >> 48)
#define NE1_GET_DS_GC_R119(r)   (((r) & NE1_MASK_DS_GC_R119) >> 56)
#define NE1_GET_DS_GC_R120(r)   (((r) & NE1_MASK_DS_GC_R120) >> 0)
#define NE1_GET_DS_GC_R121(r)   (((r) & NE1_MASK_DS_GC_R121) >> 8)
#define NE1_GET_DS_GC_R122(r)   (((r) & NE1_MASK_DS_GC_R122) >> 16)
#define NE1_GET_DS_GC_R123(r)   (((r) & NE1_MASK_DS_GC_R123) >> 24)
#define NE1_GET_DS_GC_R124(r)   (((r) & NE1_MASK_DS_GC_R124) >> 32)
#define NE1_GET_DS_GC_R125(r)   (((r) & NE1_MASK_DS_GC_R125) >> 40)
#define NE1_GET_DS_GC_R126(r)   (((r) & NE1_MASK_DS_GC_R126) >> 48)
#define NE1_GET_DS_GC_R127(r)   (((r) & NE1_MASK_DS_GC_R127) >> 56)
#define NE1_GET_DS_GC_R128(r)   (((r) & NE1_MASK_DS_GC_R128) >> 0)
#define NE1_GET_DS_GC_R129(r)   (((r) & NE1_MASK_DS_GC_R129) >> 8)
#define NE1_GET_DS_GC_R130(r)   (((r) & NE1_MASK_DS_GC_R130) >> 16)
#define NE1_GET_DS_GC_R131(r)   (((r) & NE1_MASK_DS_GC_R131) >> 24)
#define NE1_GET_DS_GC_R132(r)   (((r) & NE1_MASK_DS_GC_R132) >> 32)
#define NE1_GET_DS_GC_R133(r)   (((r) & NE1_MASK_DS_GC_R133) >> 40)
#define NE1_GET_DS_GC_R134(r)   (((r) & NE1_MASK_DS_GC_R134) >> 48)
#define NE1_GET_DS_GC_R135(r)   (((r) & NE1_MASK_DS_GC_R135) >> 56)
#define NE1_GET_DS_GC_R136(r)   (((r) & NE1_MASK_DS_GC_R136) >> 0)
#define NE1_GET_DS_GC_R137(r)   (((r) & NE1_MASK_DS_GC_R137) >> 8)
#define NE1_GET_DS_GC_R138(r)   (((r) & NE1_MASK_DS_GC_R138) >> 16)
#define NE1_GET_DS_GC_R139(r)   (((r) & NE1_MASK_DS_GC_R139) >> 24)
#define NE1_GET_DS_GC_R140(r)   (((r) & NE1_MASK_DS_GC_R140) >> 32)
#define NE1_GET_DS_GC_R141(r)   (((r) & NE1_MASK_DS_GC_R141) >> 40)
#define NE1_GET_DS_GC_R142(r)   (((r) & NE1_MASK_DS_GC_R142) >> 48)
#define NE1_GET_DS_GC_R143(r)   (((r) & NE1_MASK_DS_GC_R143) >> 56)
#define NE1_GET_DS_GC_R144(r)   (((r) & NE1_MASK_DS_GC_R144) >> 0)
#define NE1_GET_DS_GC_R145(r)   (((r) & NE1_MASK_DS_GC_R145) >> 8)
#define NE1_GET_DS_GC_R146(r)   (((r) & NE1_MASK_DS_GC_R146) >> 16)
#define NE1_GET_DS_GC_R147(r)   (((r) & NE1_MASK_DS_GC_R147) >> 24)
#define NE1_GET_DS_GC_R148(r)   (((r) & NE1_MASK_DS_GC_R148) >> 32)
#define NE1_GET_DS_GC_R149(r)   (((r) & NE1_MASK_DS_GC_R149) >> 40)
#define NE1_GET_DS_GC_R150(r)   (((r) & NE1_MASK_DS_GC_R150) >> 48)
#define NE1_GET_DS_GC_R151(r)   (((r) & NE1_MASK_DS_GC_R151) >> 56)
#define NE1_GET_DS_GC_R152(r)   (((r) & NE1_MASK_DS_GC_R152) >> 0)
#define NE1_GET_DS_GC_R153(r)   (((r) & NE1_MASK_DS_GC_R153) >> 8)
#define NE1_GET_DS_GC_R154(r)   (((r) & NE1_MASK_DS_GC_R154) >> 16)
#define NE1_GET_DS_GC_R155(r)   (((r) & NE1_MASK_DS_GC_R155) >> 24)
#define NE1_GET_DS_GC_R156(r)   (((r) & NE1_MASK_DS_GC_R156) >> 32)
#define NE1_GET_DS_GC_R157(r)   (((r) & NE1_MASK_DS_GC_R157) >> 40)
#define NE1_GET_DS_GC_R158(r)   (((r) & NE1_MASK_DS_GC_R158) >> 48)
#define NE1_GET_DS_GC_R159(r)   (((r) & NE1_MASK_DS_GC_R159) >> 56)
#define NE1_GET_DS_GC_R160(r)   (((r) & NE1_MASK_DS_GC_R160) >> 0)
#define NE1_GET_DS_GC_R161(r)   (((r) & NE1_MASK_DS_GC_R161) >> 8)
#define NE1_GET_DS_GC_R162(r)   (((r) & NE1_MASK_DS_GC_R162) >> 16)
#define NE1_GET_DS_GC_R163(r)   (((r) & NE1_MASK_DS_GC_R163) >> 24)
#define NE1_GET_DS_GC_R164(r)   (((r) & NE1_MASK_DS_GC_R164) >> 32)
#define NE1_GET_DS_GC_R165(r)   (((r) & NE1_MASK_DS_GC_R165) >> 40)
#define NE1_GET_DS_GC_R166(r)   (((r) & NE1_MASK_DS_GC_R166) >> 48)
#define NE1_GET_DS_GC_R167(r)   (((r) & NE1_MASK_DS_GC_R167) >> 56)
#define NE1_GET_DS_GC_R168(r)   (((r) & NE1_MASK_DS_GC_R168) >> 0)
#define NE1_GET_DS_GC_R169(r)   (((r) & NE1_MASK_DS_GC_R169) >> 8)
#define NE1_GET_DS_GC_R170(r)   (((r) & NE1_MASK_DS_GC_R170) >> 16)
#define NE1_GET_DS_GC_R171(r)   (((r) & NE1_MASK_DS_GC_R171) >> 24)
#define NE1_GET_DS_GC_R172(r)   (((r) & NE1_MASK_DS_GC_R172) >> 32)
#define NE1_GET_DS_GC_R173(r)   (((r) & NE1_MASK_DS_GC_R173) >> 40)
#define NE1_GET_DS_GC_R174(r)   (((r) & NE1_MASK_DS_GC_R174) >> 48)
#define NE1_GET_DS_GC_R175(r)   (((r) & NE1_MASK_DS_GC_R175) >> 56)
#define NE1_GET_DS_GC_R176(r)   (((r) & NE1_MASK_DS_GC_R176) >> 0)
#define NE1_GET_DS_GC_R177(r)   (((r) & NE1_MASK_DS_GC_R177) >> 8)
#define NE1_GET_DS_GC_R178(r)   (((r) & NE1_MASK_DS_GC_R178) >> 16)
#define NE1_GET_DS_GC_R179(r)   (((r) & NE1_MASK_DS_GC_R179) >> 24)
#define NE1_GET_DS_GC_R180(r)   (((r) & NE1_MASK_DS_GC_R180) >> 32)
#define NE1_GET_DS_GC_R181(r)   (((r) & NE1_MASK_DS_GC_R181) >> 40)
#define NE1_GET_DS_GC_R182(r)   (((r) & NE1_MASK_DS_GC_R182) >> 48)
#define NE1_GET_DS_GC_R183(r)   (((r) & NE1_MASK_DS_GC_R183) >> 56)
#define NE1_GET_DS_GC_R184(r)   (((r) & NE1_MASK_DS_GC_R184) >> 0)
#define NE1_GET_DS_GC_R185(r)   (((r) & NE1_MASK_DS_GC_R185) >> 8)
#define NE1_GET_DS_GC_R186(r)   (((r) & NE1_MASK_DS_GC_R186) >> 16)
#define NE1_GET_DS_GC_R187(r)   (((r) & NE1_MASK_DS_GC_R187) >> 24)
#define NE1_GET_DS_GC_R188(r)   (((r) & NE1_MASK_DS_GC_R188) >> 32)
#define NE1_GET_DS_GC_R189(r)   (((r) & NE1_MASK_DS_GC_R189) >> 40)
#define NE1_GET_DS_GC_R190(r)   (((r) & NE1_MASK_DS_GC_R190) >> 48)
#define NE1_GET_DS_GC_R191(r)   (((r) & NE1_MASK_DS_GC_R191) >> 56)
#define NE1_GET_DS_GC_R192(r)   (((r) & NE1_MASK_DS_GC_R192) >> 0)
#define NE1_GET_DS_GC_R193(r)   (((r) & NE1_MASK_DS_GC_R193) >> 8)
#define NE1_GET_DS_GC_R194(r)   (((r) & NE1_MASK_DS_GC_R194) >> 16)
#define NE1_GET_DS_GC_R195(r)   (((r) & NE1_MASK_DS_GC_R195) >> 24)
#define NE1_GET_DS_GC_R196(r)   (((r) & NE1_MASK_DS_GC_R196) >> 32)
#define NE1_GET_DS_GC_R197(r)   (((r) & NE1_MASK_DS_GC_R197) >> 40)
#define NE1_GET_DS_GC_R198(r)   (((r) & NE1_MASK_DS_GC_R198) >> 48)
#define NE1_GET_DS_GC_R199(r)   (((r) & NE1_MASK_DS_GC_R199) >> 56)
#define NE1_GET_DS_GC_R200(r)   (((r) & NE1_MASK_DS_GC_R200) >> 0)
#define NE1_GET_DS_GC_R201(r)   (((r) & NE1_MASK_DS_GC_R201) >> 8)
#define NE1_GET_DS_GC_R202(r)   (((r) & NE1_MASK_DS_GC_R202) >> 16)
#define NE1_GET_DS_GC_R203(r)   (((r) & NE1_MASK_DS_GC_R203) >> 24)
#define NE1_GET_DS_GC_R204(r)   (((r) & NE1_MASK_DS_GC_R204) >> 32)
#define NE1_GET_DS_GC_R205(r)   (((r) & NE1_MASK_DS_GC_R205) >> 40)
#define NE1_GET_DS_GC_R206(r)   (((r) & NE1_MASK_DS_GC_R206) >> 48)
#define NE1_GET_DS_GC_R207(r)   (((r) & NE1_MASK_DS_GC_R207) >> 56)
#define NE1_GET_DS_GC_R208(r)   (((r) & NE1_MASK_DS_GC_R208) >> 0)
#define NE1_GET_DS_GC_R209(r)   (((r) & NE1_MASK_DS_GC_R209) >> 8)
#define NE1_GET_DS_GC_R210(r)   (((r) & NE1_MASK_DS_GC_R210) >> 16)
#define NE1_GET_DS_GC_R211(r)   (((r) & NE1_MASK_DS_GC_R211) >> 24)
#define NE1_GET_DS_GC_R212(r)   (((r) & NE1_MASK_DS_GC_R212) >> 32)
#define NE1_GET_DS_GC_R213(r)   (((r) & NE1_MASK_DS_GC_R213) >> 40)
#define NE1_GET_DS_GC_R214(r)   (((r) & NE1_MASK_DS_GC_R214) >> 48)
#define NE1_GET_DS_GC_R215(r)   (((r) & NE1_MASK_DS_GC_R215) >> 56)
#define NE1_GET_DS_GC_R216(r)   (((r) & NE1_MASK_DS_GC_R216) >> 0)
#define NE1_GET_DS_GC_R217(r)   (((r) & NE1_MASK_DS_GC_R217) >> 8)
#define NE1_GET_DS_GC_R218(r)   (((r) & NE1_MASK_DS_GC_R218) >> 16)
#define NE1_GET_DS_GC_R219(r)   (((r) & NE1_MASK_DS_GC_R219) >> 24)
#define NE1_GET_DS_GC_R220(r)   (((r) & NE1_MASK_DS_GC_R220) >> 32)
#define NE1_GET_DS_GC_R221(r)   (((r) & NE1_MASK_DS_GC_R221) >> 40)
#define NE1_GET_DS_GC_R222(r)   (((r) & NE1_MASK_DS_GC_R222) >> 48)
#define NE1_GET_DS_GC_R223(r)   (((r) & NE1_MASK_DS_GC_R223) >> 56)
#define NE1_GET_DS_GC_R224(r)   (((r) & NE1_MASK_DS_GC_R224) >> 0)
#define NE1_GET_DS_GC_R225(r)   (((r) & NE1_MASK_DS_GC_R225) >> 8)
#define NE1_GET_DS_GC_R226(r)   (((r) & NE1_MASK_DS_GC_R226) >> 16)
#define NE1_GET_DS_GC_R227(r)   (((r) & NE1_MASK_DS_GC_R227) >> 24)
#define NE1_GET_DS_GC_R228(r)   (((r) & NE1_MASK_DS_GC_R228) >> 32)
#define NE1_GET_DS_GC_R229(r)   (((r) & NE1_MASK_DS_GC_R229) >> 40)
#define NE1_GET_DS_GC_R230(r)   (((r) & NE1_MASK_DS_GC_R230) >> 48)
#define NE1_GET_DS_GC_R231(r)   (((r) & NE1_MASK_DS_GC_R231) >> 56)
#define NE1_GET_DS_GC_R232(r)   (((r) & NE1_MASK_DS_GC_R232) >> 0)
#define NE1_GET_DS_GC_R233(r)   (((r) & NE1_MASK_DS_GC_R233) >> 8)
#define NE1_GET_DS_GC_R234(r)   (((r) & NE1_MASK_DS_GC_R234) >> 16)
#define NE1_GET_DS_GC_R235(r)   (((r) & NE1_MASK_DS_GC_R235) >> 24)
#define NE1_GET_DS_GC_R236(r)   (((r) & NE1_MASK_DS_GC_R236) >> 32)
#define NE1_GET_DS_GC_R237(r)   (((r) & NE1_MASK_DS_GC_R237) >> 40)
#define NE1_GET_DS_GC_R238(r)   (((r) & NE1_MASK_DS_GC_R238) >> 48)
#define NE1_GET_DS_GC_R239(r)   (((r) & NE1_MASK_DS_GC_R239) >> 56)
#define NE1_GET_DS_GC_R240(r)   (((r) & NE1_MASK_DS_GC_R240) >> 0)
#define NE1_GET_DS_GC_R241(r)   (((r) & NE1_MASK_DS_GC_R241) >> 8)
#define NE1_GET_DS_GC_R242(r)   (((r) & NE1_MASK_DS_GC_R242) >> 16)
#define NE1_GET_DS_GC_R243(r)   (((r) & NE1_MASK_DS_GC_R243) >> 24)
#define NE1_GET_DS_GC_R244(r)   (((r) & NE1_MASK_DS_GC_R244) >> 32)
#define NE1_GET_DS_GC_R245(r)   (((r) & NE1_MASK_DS_GC_R245) >> 40)
#define NE1_GET_DS_GC_R246(r)   (((r) & NE1_MASK_DS_GC_R246) >> 48)
#define NE1_GET_DS_GC_R247(r)   (((r) & NE1_MASK_DS_GC_R247) >> 56)
#define NE1_GET_DS_GC_R248(r)   (((r) & NE1_MASK_DS_GC_R248) >> 0)
#define NE1_GET_DS_GC_R249(r)   (((r) & NE1_MASK_DS_GC_R249) >> 8)
#define NE1_GET_DS_GC_R250(r)   (((r) & NE1_MASK_DS_GC_R250) >> 16)
#define NE1_GET_DS_GC_R251(r)   (((r) & NE1_MASK_DS_GC_R251) >> 24)
#define NE1_GET_DS_GC_R252(r)   (((r) & NE1_MASK_DS_GC_R252) >> 32)
#define NE1_GET_DS_GC_R253(r)   (((r) & NE1_MASK_DS_GC_R253) >> 40)
#define NE1_GET_DS_GC_R254(r)   (((r) & NE1_MASK_DS_GC_R254) >> 48)
#define NE1_GET_DS_GC_R255(r)   (((r) & NE1_MASK_DS_GC_R255) >> 56)
#define NE1_GET_DS_GC_G0(r)     (((r) & NE1_MASK_DS_GC_G0) >> 0)
#define NE1_GET_DS_GC_G1(r)     (((r) & NE1_MASK_DS_GC_G1) >> 8)
#define NE1_GET_DS_GC_G2(r)     (((r) & NE1_MASK_DS_GC_G2) >> 16)
#define NE1_GET_DS_GC_G3(r)     (((r) & NE1_MASK_DS_GC_G3) >> 24)
#define NE1_GET_DS_GC_G4(r)     (((r) & NE1_MASK_DS_GC_G4) >> 32)
#define NE1_GET_DS_GC_G5(r)     (((r) & NE1_MASK_DS_GC_G5) >> 40)
#define NE1_GET_DS_GC_G6(r)     (((r) & NE1_MASK_DS_GC_G6) >> 48)
#define NE1_GET_DS_GC_G7(r)     (((r) & NE1_MASK_DS_GC_G7) >> 56)
#define NE1_GET_DS_GC_G8(r)     (((r) & NE1_MASK_DS_GC_G8) >> 0)
#define NE1_GET_DS_GC_G9(r)     (((r) & NE1_MASK_DS_GC_G9) >> 8)
#define NE1_GET_DS_GC_G10(r)    (((r) & NE1_MASK_DS_GC_G10) >> 16)
#define NE1_GET_DS_GC_G11(r)    (((r) & NE1_MASK_DS_GC_G11) >> 24)
#define NE1_GET_DS_GC_G12(r)    (((r) & NE1_MASK_DS_GC_G12) >> 32)
#define NE1_GET_DS_GC_G13(r)    (((r) & NE1_MASK_DS_GC_G13) >> 40)
#define NE1_GET_DS_GC_G14(r)    (((r) & NE1_MASK_DS_GC_G14) >> 48)
#define NE1_GET_DS_GC_G15(r)    (((r) & NE1_MASK_DS_GC_G15) >> 56)
#define NE1_GET_DS_GC_G16(r)    (((r) & NE1_MASK_DS_GC_G16) >> 0)
#define NE1_GET_DS_GC_G17(r)    (((r) & NE1_MASK_DS_GC_G17) >> 8)
#define NE1_GET_DS_GC_G18(r)    (((r) & NE1_MASK_DS_GC_G18) >> 16)
#define NE1_GET_DS_GC_G19(r)    (((r) & NE1_MASK_DS_GC_G19) >> 24)
#define NE1_GET_DS_GC_G20(r)    (((r) & NE1_MASK_DS_GC_G20) >> 32)
#define NE1_GET_DS_GC_G21(r)    (((r) & NE1_MASK_DS_GC_G21) >> 40)
#define NE1_GET_DS_GC_G22(r)    (((r) & NE1_MASK_DS_GC_G22) >> 48)
#define NE1_GET_DS_GC_G23(r)    (((r) & NE1_MASK_DS_GC_G23) >> 56)
#define NE1_GET_DS_GC_G24(r)    (((r) & NE1_MASK_DS_GC_G24) >> 0)
#define NE1_GET_DS_GC_G25(r)    (((r) & NE1_MASK_DS_GC_G25) >> 8)
#define NE1_GET_DS_GC_G26(r)    (((r) & NE1_MASK_DS_GC_G26) >> 16)
#define NE1_GET_DS_GC_G27(r)    (((r) & NE1_MASK_DS_GC_G27) >> 24)
#define NE1_GET_DS_GC_G28(r)    (((r) & NE1_MASK_DS_GC_G28) >> 32)
#define NE1_GET_DS_GC_G29(r)    (((r) & NE1_MASK_DS_GC_G29) >> 40)
#define NE1_GET_DS_GC_G30(r)    (((r) & NE1_MASK_DS_GC_G30) >> 48)
#define NE1_GET_DS_GC_G31(r)    (((r) & NE1_MASK_DS_GC_G31) >> 56)
#define NE1_GET_DS_GC_G32(r)    (((r) & NE1_MASK_DS_GC_G32) >> 0)
#define NE1_GET_DS_GC_G33(r)    (((r) & NE1_MASK_DS_GC_G33) >> 8)
#define NE1_GET_DS_GC_G34(r)    (((r) & NE1_MASK_DS_GC_G34) >> 16)
#define NE1_GET_DS_GC_G35(r)    (((r) & NE1_MASK_DS_GC_G35) >> 24)
#define NE1_GET_DS_GC_G36(r)    (((r) & NE1_MASK_DS_GC_G36) >> 32)
#define NE1_GET_DS_GC_G37(r)    (((r) & NE1_MASK_DS_GC_G37) >> 40)
#define NE1_GET_DS_GC_G38(r)    (((r) & NE1_MASK_DS_GC_G38) >> 48)
#define NE1_GET_DS_GC_G39(r)    (((r) & NE1_MASK_DS_GC_G39) >> 56)
#define NE1_GET_DS_GC_G40(r)    (((r) & NE1_MASK_DS_GC_G40) >> 0)
#define NE1_GET_DS_GC_G41(r)    (((r) & NE1_MASK_DS_GC_G41) >> 8)
#define NE1_GET_DS_GC_G42(r)    (((r) & NE1_MASK_DS_GC_G42) >> 16)
#define NE1_GET_DS_GC_G43(r)    (((r) & NE1_MASK_DS_GC_G43) >> 24)
#define NE1_GET_DS_GC_G44(r)    (((r) & NE1_MASK_DS_GC_G44) >> 32)
#define NE1_GET_DS_GC_G45(r)    (((r) & NE1_MASK_DS_GC_G45) >> 40)
#define NE1_GET_DS_GC_G46(r)    (((r) & NE1_MASK_DS_GC_G46) >> 48)
#define NE1_GET_DS_GC_G47(r)    (((r) & NE1_MASK_DS_GC_G47) >> 56)
#define NE1_GET_DS_GC_G48(r)    (((r) & NE1_MASK_DS_GC_G48) >> 0)
#define NE1_GET_DS_GC_G49(r)    (((r) & NE1_MASK_DS_GC_G49) >> 8)
#define NE1_GET_DS_GC_G50(r)    (((r) & NE1_MASK_DS_GC_G50) >> 16)
#define NE1_GET_DS_GC_G51(r)    (((r) & NE1_MASK_DS_GC_G51) >> 24)
#define NE1_GET_DS_GC_G52(r)    (((r) & NE1_MASK_DS_GC_G52) >> 32)
#define NE1_GET_DS_GC_G53(r)    (((r) & NE1_MASK_DS_GC_G53) >> 40)
#define NE1_GET_DS_GC_G54(r)    (((r) & NE1_MASK_DS_GC_G54) >> 48)
#define NE1_GET_DS_GC_G55(r)    (((r) & NE1_MASK_DS_GC_G55) >> 56)
#define NE1_GET_DS_GC_G56(r)    (((r) & NE1_MASK_DS_GC_G56) >> 0)
#define NE1_GET_DS_GC_G57(r)    (((r) & NE1_MASK_DS_GC_G57) >> 8)
#define NE1_GET_DS_GC_G58(r)    (((r) & NE1_MASK_DS_GC_G58) >> 16)
#define NE1_GET_DS_GC_G59(r)    (((r) & NE1_MASK_DS_GC_G59) >> 24)
#define NE1_GET_DS_GC_G60(r)    (((r) & NE1_MASK_DS_GC_G60) >> 32)
#define NE1_GET_DS_GC_G61(r)    (((r) & NE1_MASK_DS_GC_G61) >> 40)
#define NE1_GET_DS_GC_G62(r)    (((r) & NE1_MASK_DS_GC_G62) >> 48)
#define NE1_GET_DS_GC_G63(r)    (((r) & NE1_MASK_DS_GC_G63) >> 56)
#define NE1_GET_DS_GC_G64(r)    (((r) & NE1_MASK_DS_GC_G64) >> 0)
#define NE1_GET_DS_GC_G65(r)    (((r) & NE1_MASK_DS_GC_G65) >> 8)
#define NE1_GET_DS_GC_G66(r)    (((r) & NE1_MASK_DS_GC_G66) >> 16)
#define NE1_GET_DS_GC_G67(r)    (((r) & NE1_MASK_DS_GC_G67) >> 24)
#define NE1_GET_DS_GC_G68(r)    (((r) & NE1_MASK_DS_GC_G68) >> 32)
#define NE1_GET_DS_GC_G69(r)    (((r) & NE1_MASK_DS_GC_G69) >> 40)
#define NE1_GET_DS_GC_G70(r)    (((r) & NE1_MASK_DS_GC_G70) >> 48)
#define NE1_GET_DS_GC_G71(r)    (((r) & NE1_MASK_DS_GC_G71) >> 56)
#define NE1_GET_DS_GC_G72(r)    (((r) & NE1_MASK_DS_GC_G72) >> 0)
#define NE1_GET_DS_GC_G73(r)    (((r) & NE1_MASK_DS_GC_G73) >> 8)
#define NE1_GET_DS_GC_G74(r)    (((r) & NE1_MASK_DS_GC_G74) >> 16)
#define NE1_GET_DS_GC_G75(r)    (((r) & NE1_MASK_DS_GC_G75) >> 24)
#define NE1_GET_DS_GC_G76(r)    (((r) & NE1_MASK_DS_GC_G76) >> 32)
#define NE1_GET_DS_GC_G77(r)    (((r) & NE1_MASK_DS_GC_G77) >> 40)
#define NE1_GET_DS_GC_G78(r)    (((r) & NE1_MASK_DS_GC_G78) >> 48)
#define NE1_GET_DS_GC_G79(r)    (((r) & NE1_MASK_DS_GC_G79) >> 56)
#define NE1_GET_DS_GC_G80(r)    (((r) & NE1_MASK_DS_GC_G80) >> 0)
#define NE1_GET_DS_GC_G81(r)    (((r) & NE1_MASK_DS_GC_G81) >> 8)
#define NE1_GET_DS_GC_G82(r)    (((r) & NE1_MASK_DS_GC_G82) >> 16)
#define NE1_GET_DS_GC_G83(r)    (((r) & NE1_MASK_DS_GC_G83) >> 24)
#define NE1_GET_DS_GC_G84(r)    (((r) & NE1_MASK_DS_GC_G84) >> 32)
#define NE1_GET_DS_GC_G85(r)    (((r) & NE1_MASK_DS_GC_G85) >> 40)
#define NE1_GET_DS_GC_G86(r)    (((r) & NE1_MASK_DS_GC_G86) >> 48)
#define NE1_GET_DS_GC_G87(r)    (((r) & NE1_MASK_DS_GC_G87) >> 56)
#define NE1_GET_DS_GC_G88(r)    (((r) & NE1_MASK_DS_GC_G88) >> 0)
#define NE1_GET_DS_GC_G89(r)    (((r) & NE1_MASK_DS_GC_G89) >> 8)
#define NE1_GET_DS_GC_G90(r)    (((r) & NE1_MASK_DS_GC_G90) >> 16)
#define NE1_GET_DS_GC_G91(r)    (((r) & NE1_MASK_DS_GC_G91) >> 24)
#define NE1_GET_DS_GC_G92(r)    (((r) & NE1_MASK_DS_GC_G92) >> 32)
#define NE1_GET_DS_GC_G93(r)    (((r) & NE1_MASK_DS_GC_G93) >> 40)
#define NE1_GET_DS_GC_G94(r)    (((r) & NE1_MASK_DS_GC_G94) >> 48)
#define NE1_GET_DS_GC_G95(r)    (((r) & NE1_MASK_DS_GC_G95) >> 56)
#define NE1_GET_DS_GC_G96(r)    (((r) & NE1_MASK_DS_GC_G96) >> 0)
#define NE1_GET_DS_GC_G97(r)    (((r) & NE1_MASK_DS_GC_G97) >> 8)
#define NE1_GET_DS_GC_G98(r)    (((r) & NE1_MASK_DS_GC_G98) >> 16)
#define NE1_GET_DS_GC_G99(r)    (((r) & NE1_MASK_DS_GC_G99) >> 24)
#define NE1_GET_DS_GC_G100(r)   (((r) & NE1_MASK_DS_GC_G100) >> 32)
#define NE1_GET_DS_GC_G101(r)   (((r) & NE1_MASK_DS_GC_G101) >> 40)
#define NE1_GET_DS_GC_G102(r)   (((r) & NE1_MASK_DS_GC_G102) >> 48)
#define NE1_GET_DS_GC_G103(r)   (((r) & NE1_MASK_DS_GC_G103) >> 56)
#define NE1_GET_DS_GC_G104(r)   (((r) & NE1_MASK_DS_GC_G104) >> 0)
#define NE1_GET_DS_GC_G105(r)   (((r) & NE1_MASK_DS_GC_G105) >> 8)
#define NE1_GET_DS_GC_G106(r)   (((r) & NE1_MASK_DS_GC_G106) >> 16)
#define NE1_GET_DS_GC_G107(r)   (((r) & NE1_MASK_DS_GC_G107) >> 24)
#define NE1_GET_DS_GC_G108(r)   (((r) & NE1_MASK_DS_GC_G108) >> 32)
#define NE1_GET_DS_GC_G109(r)   (((r) & NE1_MASK_DS_GC_G109) >> 40)
#define NE1_GET_DS_GC_G110(r)   (((r) & NE1_MASK_DS_GC_G110) >> 48)
#define NE1_GET_DS_GC_G111(r)   (((r) & NE1_MASK_DS_GC_G111) >> 56)
#define NE1_GET_DS_GC_G112(r)   (((r) & NE1_MASK_DS_GC_G112) >> 0)
#define NE1_GET_DS_GC_G113(r)   (((r) & NE1_MASK_DS_GC_G113) >> 8)
#define NE1_GET_DS_GC_G114(r)   (((r) & NE1_MASK_DS_GC_G114) >> 16)
#define NE1_GET_DS_GC_G115(r)   (((r) & NE1_MASK_DS_GC_G115) >> 24)
#define NE1_GET_DS_GC_G116(r)   (((r) & NE1_MASK_DS_GC_G116) >> 32)
#define NE1_GET_DS_GC_G117(r)   (((r) & NE1_MASK_DS_GC_G117) >> 40)
#define NE1_GET_DS_GC_G118(r)   (((r) & NE1_MASK_DS_GC_G118) >> 48)
#define NE1_GET_DS_GC_G119(r)   (((r) & NE1_MASK_DS_GC_G119) >> 56)
#define NE1_GET_DS_GC_G120(r)   (((r) & NE1_MASK_DS_GC_G120) >> 0)
#define NE1_GET_DS_GC_G121(r)   (((r) & NE1_MASK_DS_GC_G121) >> 8)
#define NE1_GET_DS_GC_G122(r)   (((r) & NE1_MASK_DS_GC_G122) >> 16)
#define NE1_GET_DS_GC_G123(r)   (((r) & NE1_MASK_DS_GC_G123) >> 24)
#define NE1_GET_DS_GC_G124(r)   (((r) & NE1_MASK_DS_GC_G124) >> 32)
#define NE1_GET_DS_GC_G125(r)   (((r) & NE1_MASK_DS_GC_G125) >> 40)
#define NE1_GET_DS_GC_G126(r)   (((r) & NE1_MASK_DS_GC_G126) >> 48)
#define NE1_GET_DS_GC_G127(r)   (((r) & NE1_MASK_DS_GC_G127) >> 56)
#define NE1_GET_DS_GC_G128(r)   (((r) & NE1_MASK_DS_GC_G128) >> 0)
#define NE1_GET_DS_GC_G129(r)   (((r) & NE1_MASK_DS_GC_G129) >> 8)
#define NE1_GET_DS_GC_G130(r)   (((r) & NE1_MASK_DS_GC_G130) >> 16)
#define NE1_GET_DS_GC_G131(r)   (((r) & NE1_MASK_DS_GC_G131) >> 24)
#define NE1_GET_DS_GC_G132(r)   (((r) & NE1_MASK_DS_GC_G132) >> 32)
#define NE1_GET_DS_GC_G133(r)   (((r) & NE1_MASK_DS_GC_G133) >> 40)
#define NE1_GET_DS_GC_G134(r)   (((r) & NE1_MASK_DS_GC_G134) >> 48)
#define NE1_GET_DS_GC_G135(r)   (((r) & NE1_MASK_DS_GC_G135) >> 56)
#define NE1_GET_DS_GC_G136(r)   (((r) & NE1_MASK_DS_GC_G136) >> 0)
#define NE1_GET_DS_GC_G137(r)   (((r) & NE1_MASK_DS_GC_G137) >> 8)
#define NE1_GET_DS_GC_G138(r)   (((r) & NE1_MASK_DS_GC_G138) >> 16)
#define NE1_GET_DS_GC_G139(r)   (((r) & NE1_MASK_DS_GC_G139) >> 24)
#define NE1_GET_DS_GC_G140(r)   (((r) & NE1_MASK_DS_GC_G140) >> 32)
#define NE1_GET_DS_GC_G141(r)   (((r) & NE1_MASK_DS_GC_G141) >> 40)
#define NE1_GET_DS_GC_G142(r)   (((r) & NE1_MASK_DS_GC_G142) >> 48)
#define NE1_GET_DS_GC_G143(r)   (((r) & NE1_MASK_DS_GC_G143) >> 56)
#define NE1_GET_DS_GC_G144(r)   (((r) & NE1_MASK_DS_GC_G144) >> 0)
#define NE1_GET_DS_GC_G145(r)   (((r) & NE1_MASK_DS_GC_G145) >> 8)
#define NE1_GET_DS_GC_G146(r)   (((r) & NE1_MASK_DS_GC_G146) >> 16)
#define NE1_GET_DS_GC_G147(r)   (((r) & NE1_MASK_DS_GC_G147) >> 24)
#define NE1_GET_DS_GC_G148(r)   (((r) & NE1_MASK_DS_GC_G148) >> 32)
#define NE1_GET_DS_GC_G149(r)   (((r) & NE1_MASK_DS_GC_G149) >> 40)
#define NE1_GET_DS_GC_G150(r)   (((r) & NE1_MASK_DS_GC_G150) >> 48)
#define NE1_GET_DS_GC_G151(r)   (((r) & NE1_MASK_DS_GC_G151) >> 56)
#define NE1_GET_DS_GC_G152(r)   (((r) & NE1_MASK_DS_GC_G152) >> 0)
#define NE1_GET_DS_GC_G153(r)   (((r) & NE1_MASK_DS_GC_G153) >> 8)
#define NE1_GET_DS_GC_G154(r)   (((r) & NE1_MASK_DS_GC_G154) >> 16)
#define NE1_GET_DS_GC_G155(r)   (((r) & NE1_MASK_DS_GC_G155) >> 24)
#define NE1_GET_DS_GC_G156(r)   (((r) & NE1_MASK_DS_GC_G156) >> 32)
#define NE1_GET_DS_GC_G157(r)   (((r) & NE1_MASK_DS_GC_G157) >> 40)
#define NE1_GET_DS_GC_G158(r)   (((r) & NE1_MASK_DS_GC_G158) >> 48)
#define NE1_GET_DS_GC_G159(r)   (((r) & NE1_MASK_DS_GC_G159) >> 56)
#define NE1_GET_DS_GC_G160(r)   (((r) & NE1_MASK_DS_GC_G160) >> 0)
#define NE1_GET_DS_GC_G161(r)   (((r) & NE1_MASK_DS_GC_G161) >> 8)
#define NE1_GET_DS_GC_G162(r)   (((r) & NE1_MASK_DS_GC_G162) >> 16)
#define NE1_GET_DS_GC_G163(r)   (((r) & NE1_MASK_DS_GC_G163) >> 24)
#define NE1_GET_DS_GC_G164(r)   (((r) & NE1_MASK_DS_GC_G164) >> 32)
#define NE1_GET_DS_GC_G165(r)   (((r) & NE1_MASK_DS_GC_G165) >> 40)
#define NE1_GET_DS_GC_G166(r)   (((r) & NE1_MASK_DS_GC_G166) >> 48)
#define NE1_GET_DS_GC_G167(r)   (((r) & NE1_MASK_DS_GC_G167) >> 56)
#define NE1_GET_DS_GC_G168(r)   (((r) & NE1_MASK_DS_GC_G168) >> 0)
#define NE1_GET_DS_GC_G169(r)   (((r) & NE1_MASK_DS_GC_G169) >> 8)
#define NE1_GET_DS_GC_G170(r)   (((r) & NE1_MASK_DS_GC_G170) >> 16)
#define NE1_GET_DS_GC_G171(r)   (((r) & NE1_MASK_DS_GC_G171) >> 24)
#define NE1_GET_DS_GC_G172(r)   (((r) & NE1_MASK_DS_GC_G172) >> 32)
#define NE1_GET_DS_GC_G173(r)   (((r) & NE1_MASK_DS_GC_G173) >> 40)
#define NE1_GET_DS_GC_G174(r)   (((r) & NE1_MASK_DS_GC_G174) >> 48)
#define NE1_GET_DS_GC_G175(r)   (((r) & NE1_MASK_DS_GC_G175) >> 56)
#define NE1_GET_DS_GC_G176(r)   (((r) & NE1_MASK_DS_GC_G176) >> 0)
#define NE1_GET_DS_GC_G177(r)   (((r) & NE1_MASK_DS_GC_G177) >> 8)
#define NE1_GET_DS_GC_G178(r)   (((r) & NE1_MASK_DS_GC_G178) >> 16)
#define NE1_GET_DS_GC_G179(r)   (((r) & NE1_MASK_DS_GC_G179) >> 24)
#define NE1_GET_DS_GC_G180(r)   (((r) & NE1_MASK_DS_GC_G180) >> 32)
#define NE1_GET_DS_GC_G181(r)   (((r) & NE1_MASK_DS_GC_G181) >> 40)
#define NE1_GET_DS_GC_G182(r)   (((r) & NE1_MASK_DS_GC_G182) >> 48)
#define NE1_GET_DS_GC_G183(r)   (((r) & NE1_MASK_DS_GC_G183) >> 56)
#define NE1_GET_DS_GC_G184(r)   (((r) & NE1_MASK_DS_GC_G184) >> 0)
#define NE1_GET_DS_GC_G185(r)   (((r) & NE1_MASK_DS_GC_G185) >> 8)
#define NE1_GET_DS_GC_G186(r)   (((r) & NE1_MASK_DS_GC_G186) >> 16)
#define NE1_GET_DS_GC_G187(r)   (((r) & NE1_MASK_DS_GC_G187) >> 24)
#define NE1_GET_DS_GC_G188(r)   (((r) & NE1_MASK_DS_GC_G188) >> 32)
#define NE1_GET_DS_GC_G189(r)   (((r) & NE1_MASK_DS_GC_G189) >> 40)
#define NE1_GET_DS_GC_G190(r)   (((r) & NE1_MASK_DS_GC_G190) >> 48)
#define NE1_GET_DS_GC_G191(r)   (((r) & NE1_MASK_DS_GC_G191) >> 56)
#define NE1_GET_DS_GC_G192(r)   (((r) & NE1_MASK_DS_GC_G192) >> 0)
#define NE1_GET_DS_GC_G193(r)   (((r) & NE1_MASK_DS_GC_G193) >> 8)
#define NE1_GET_DS_GC_G194(r)   (((r) & NE1_MASK_DS_GC_G194) >> 16)
#define NE1_GET_DS_GC_G195(r)   (((r) & NE1_MASK_DS_GC_G195) >> 24)
#define NE1_GET_DS_GC_G196(r)   (((r) & NE1_MASK_DS_GC_G196) >> 32)
#define NE1_GET_DS_GC_G197(r)   (((r) & NE1_MASK_DS_GC_G197) >> 40)
#define NE1_GET_DS_GC_G198(r)   (((r) & NE1_MASK_DS_GC_G198) >> 48)
#define NE1_GET_DS_GC_G199(r)   (((r) & NE1_MASK_DS_GC_G199) >> 56)
#define NE1_GET_DS_GC_G200(r)   (((r) & NE1_MASK_DS_GC_G200) >> 0)
#define NE1_GET_DS_GC_G201(r)   (((r) & NE1_MASK_DS_GC_G201) >> 8)
#define NE1_GET_DS_GC_G202(r)   (((r) & NE1_MASK_DS_GC_G202) >> 16)
#define NE1_GET_DS_GC_G203(r)   (((r) & NE1_MASK_DS_GC_G203) >> 24)
#define NE1_GET_DS_GC_G204(r)   (((r) & NE1_MASK_DS_GC_G204) >> 32)
#define NE1_GET_DS_GC_G205(r)   (((r) & NE1_MASK_DS_GC_G205) >> 40)
#define NE1_GET_DS_GC_G206(r)   (((r) & NE1_MASK_DS_GC_G206) >> 48)
#define NE1_GET_DS_GC_G207(r)   (((r) & NE1_MASK_DS_GC_G207) >> 56)
#define NE1_GET_DS_GC_G208(r)   (((r) & NE1_MASK_DS_GC_G208) >> 0)
#define NE1_GET_DS_GC_G209(r)   (((r) & NE1_MASK_DS_GC_G209) >> 8)
#define NE1_GET_DS_GC_G210(r)   (((r) & NE1_MASK_DS_GC_G210) >> 16)
#define NE1_GET_DS_GC_G211(r)   (((r) & NE1_MASK_DS_GC_G211) >> 24)
#define NE1_GET_DS_GC_G212(r)   (((r) & NE1_MASK_DS_GC_G212) >> 32)
#define NE1_GET_DS_GC_G213(r)   (((r) & NE1_MASK_DS_GC_G213) >> 40)
#define NE1_GET_DS_GC_G214(r)   (((r) & NE1_MASK_DS_GC_G214) >> 48)
#define NE1_GET_DS_GC_G215(r)   (((r) & NE1_MASK_DS_GC_G215) >> 56)
#define NE1_GET_DS_GC_G216(r)   (((r) & NE1_MASK_DS_GC_G216) >> 0)
#define NE1_GET_DS_GC_G217(r)   (((r) & NE1_MASK_DS_GC_G217) >> 8)
#define NE1_GET_DS_GC_G218(r)   (((r) & NE1_MASK_DS_GC_G218) >> 16)
#define NE1_GET_DS_GC_G219(r)   (((r) & NE1_MASK_DS_GC_G219) >> 24)
#define NE1_GET_DS_GC_G220(r)   (((r) & NE1_MASK_DS_GC_G220) >> 32)
#define NE1_GET_DS_GC_G221(r)   (((r) & NE1_MASK_DS_GC_G221) >> 40)
#define NE1_GET_DS_GC_G222(r)   (((r) & NE1_MASK_DS_GC_G222) >> 48)
#define NE1_GET_DS_GC_G223(r)   (((r) & NE1_MASK_DS_GC_G223) >> 56)
#define NE1_GET_DS_GC_G224(r)   (((r) & NE1_MASK_DS_GC_G224) >> 0)
#define NE1_GET_DS_GC_G225(r)   (((r) & NE1_MASK_DS_GC_G225) >> 8)
#define NE1_GET_DS_GC_G226(r)   (((r) & NE1_MASK_DS_GC_G226) >> 16)
#define NE1_GET_DS_GC_G227(r)   (((r) & NE1_MASK_DS_GC_G227) >> 24)
#define NE1_GET_DS_GC_G228(r)   (((r) & NE1_MASK_DS_GC_G228) >> 32)
#define NE1_GET_DS_GC_G229(r)   (((r) & NE1_MASK_DS_GC_G229) >> 40)
#define NE1_GET_DS_GC_G230(r)   (((r) & NE1_MASK_DS_GC_G230) >> 48)
#define NE1_GET_DS_GC_G231(r)   (((r) & NE1_MASK_DS_GC_G231) >> 56)
#define NE1_GET_DS_GC_G232(r)   (((r) & NE1_MASK_DS_GC_G232) >> 0)
#define NE1_GET_DS_GC_G233(r)   (((r) & NE1_MASK_DS_GC_G233) >> 8)
#define NE1_GET_DS_GC_G234(r)   (((r) & NE1_MASK_DS_GC_G234) >> 16)
#define NE1_GET_DS_GC_G235(r)   (((r) & NE1_MASK_DS_GC_G235) >> 24)
#define NE1_GET_DS_GC_G236(r)   (((r) & NE1_MASK_DS_GC_G236) >> 32)
#define NE1_GET_DS_GC_G237(r)   (((r) & NE1_MASK_DS_GC_G237) >> 40)
#define NE1_GET_DS_GC_G238(r)   (((r) & NE1_MASK_DS_GC_G238) >> 48)
#define NE1_GET_DS_GC_G239(r)   (((r) & NE1_MASK_DS_GC_G239) >> 56)
#define NE1_GET_DS_GC_G240(r)   (((r) & NE1_MASK_DS_GC_G240) >> 0)
#define NE1_GET_DS_GC_G241(r)   (((r) & NE1_MASK_DS_GC_G241) >> 8)
#define NE1_GET_DS_GC_G242(r)   (((r) & NE1_MASK_DS_GC_G242) >> 16)
#define NE1_GET_DS_GC_G243(r)   (((r) & NE1_MASK_DS_GC_G243) >> 24)
#define NE1_GET_DS_GC_G244(r)   (((r) & NE1_MASK_DS_GC_G244) >> 32)
#define NE1_GET_DS_GC_G245(r)   (((r) & NE1_MASK_DS_GC_G245) >> 40)
#define NE1_GET_DS_GC_G246(r)   (((r) & NE1_MASK_DS_GC_G246) >> 48)
#define NE1_GET_DS_GC_G247(r)   (((r) & NE1_MASK_DS_GC_G247) >> 56)
#define NE1_GET_DS_GC_G248(r)   (((r) & NE1_MASK_DS_GC_G248) >> 0)
#define NE1_GET_DS_GC_G249(r)   (((r) & NE1_MASK_DS_GC_G249) >> 8)
#define NE1_GET_DS_GC_G250(r)   (((r) & NE1_MASK_DS_GC_G250) >> 16)
#define NE1_GET_DS_GC_G251(r)   (((r) & NE1_MASK_DS_GC_G251) >> 24)
#define NE1_GET_DS_GC_G252(r)   (((r) & NE1_MASK_DS_GC_G252) >> 32)
#define NE1_GET_DS_GC_G253(r)   (((r) & NE1_MASK_DS_GC_G253) >> 40)
#define NE1_GET_DS_GC_G254(r)   (((r) & NE1_MASK_DS_GC_G254) >> 48)
#define NE1_GET_DS_GC_G255(r)   (((r) & NE1_MASK_DS_GC_G255) >> 56)
#define NE1_GET_DS_GC_B0(r)     (((r) & NE1_MASK_DS_GC_B0) >> 0)
#define NE1_GET_DS_GC_B1(r)     (((r) & NE1_MASK_DS_GC_B1) >> 8)
#define NE1_GET_DS_GC_B2(r)     (((r) & NE1_MASK_DS_GC_B2) >> 16)
#define NE1_GET_DS_GC_B3(r)     (((r) & NE1_MASK_DS_GC_B3) >> 24)
#define NE1_GET_DS_GC_B4(r)     (((r) & NE1_MASK_DS_GC_B4) >> 32)
#define NE1_GET_DS_GC_B5(r)     (((r) & NE1_MASK_DS_GC_B5) >> 40)
#define NE1_GET_DS_GC_B6(r)     (((r) & NE1_MASK_DS_GC_B6) >> 48)
#define NE1_GET_DS_GC_B7(r)     (((r) & NE1_MASK_DS_GC_B7) >> 56)
#define NE1_GET_DS_GC_B8(r)     (((r) & NE1_MASK_DS_GC_B8) >> 0)
#define NE1_GET_DS_GC_B9(r)     (((r) & NE1_MASK_DS_GC_B9) >> 8)
#define NE1_GET_DS_GC_B10(r)    (((r) & NE1_MASK_DS_GC_B10) >> 16)
#define NE1_GET_DS_GC_B11(r)    (((r) & NE1_MASK_DS_GC_B11) >> 24)
#define NE1_GET_DS_GC_B12(r)    (((r) & NE1_MASK_DS_GC_B12) >> 32)
#define NE1_GET_DS_GC_B13(r)    (((r) & NE1_MASK_DS_GC_B13) >> 40)
#define NE1_GET_DS_GC_B14(r)    (((r) & NE1_MASK_DS_GC_B14) >> 48)
#define NE1_GET_DS_GC_B15(r)    (((r) & NE1_MASK_DS_GC_B15) >> 56)
#define NE1_GET_DS_GC_B16(r)    (((r) & NE1_MASK_DS_GC_B16) >> 0)
#define NE1_GET_DS_GC_B17(r)    (((r) & NE1_MASK_DS_GC_B17) >> 8)
#define NE1_GET_DS_GC_B18(r)    (((r) & NE1_MASK_DS_GC_B18) >> 16)
#define NE1_GET_DS_GC_B19(r)    (((r) & NE1_MASK_DS_GC_B19) >> 24)
#define NE1_GET_DS_GC_B20(r)    (((r) & NE1_MASK_DS_GC_B20) >> 32)
#define NE1_GET_DS_GC_B21(r)    (((r) & NE1_MASK_DS_GC_B21) >> 40)
#define NE1_GET_DS_GC_B22(r)    (((r) & NE1_MASK_DS_GC_B22) >> 48)
#define NE1_GET_DS_GC_B23(r)    (((r) & NE1_MASK_DS_GC_B23) >> 56)
#define NE1_GET_DS_GC_B24(r)    (((r) & NE1_MASK_DS_GC_B24) >> 0)
#define NE1_GET_DS_GC_B25(r)    (((r) & NE1_MASK_DS_GC_B25) >> 8)
#define NE1_GET_DS_GC_B26(r)    (((r) & NE1_MASK_DS_GC_B26) >> 16)
#define NE1_GET_DS_GC_B27(r)    (((r) & NE1_MASK_DS_GC_B27) >> 24)
#define NE1_GET_DS_GC_B28(r)    (((r) & NE1_MASK_DS_GC_B28) >> 32)
#define NE1_GET_DS_GC_B29(r)    (((r) & NE1_MASK_DS_GC_B29) >> 40)
#define NE1_GET_DS_GC_B30(r)    (((r) & NE1_MASK_DS_GC_B30) >> 48)
#define NE1_GET_DS_GC_B31(r)    (((r) & NE1_MASK_DS_GC_B31) >> 56)
#define NE1_GET_DS_GC_B32(r)    (((r) & NE1_MASK_DS_GC_B32) >> 0)
#define NE1_GET_DS_GC_B33(r)    (((r) & NE1_MASK_DS_GC_B33) >> 8)
#define NE1_GET_DS_GC_B34(r)    (((r) & NE1_MASK_DS_GC_B34) >> 16)
#define NE1_GET_DS_GC_B35(r)    (((r) & NE1_MASK_DS_GC_B35) >> 24)
#define NE1_GET_DS_GC_B36(r)    (((r) & NE1_MASK_DS_GC_B36) >> 32)
#define NE1_GET_DS_GC_B37(r)    (((r) & NE1_MASK_DS_GC_B37) >> 40)
#define NE1_GET_DS_GC_B38(r)    (((r) & NE1_MASK_DS_GC_B38) >> 48)
#define NE1_GET_DS_GC_B39(r)    (((r) & NE1_MASK_DS_GC_B39) >> 56)
#define NE1_GET_DS_GC_B40(r)    (((r) & NE1_MASK_DS_GC_B40) >> 0)
#define NE1_GET_DS_GC_B41(r)    (((r) & NE1_MASK_DS_GC_B41) >> 8)
#define NE1_GET_DS_GC_B42(r)    (((r) & NE1_MASK_DS_GC_B42) >> 16)
#define NE1_GET_DS_GC_B43(r)    (((r) & NE1_MASK_DS_GC_B43) >> 24)
#define NE1_GET_DS_GC_B44(r)    (((r) & NE1_MASK_DS_GC_B44) >> 32)
#define NE1_GET_DS_GC_B45(r)    (((r) & NE1_MASK_DS_GC_B45) >> 40)
#define NE1_GET_DS_GC_B46(r)    (((r) & NE1_MASK_DS_GC_B46) >> 48)
#define NE1_GET_DS_GC_B47(r)    (((r) & NE1_MASK_DS_GC_B47) >> 56)
#define NE1_GET_DS_GC_B48(r)    (((r) & NE1_MASK_DS_GC_B48) >> 0)
#define NE1_GET_DS_GC_B49(r)    (((r) & NE1_MASK_DS_GC_B49) >> 8)
#define NE1_GET_DS_GC_B50(r)    (((r) & NE1_MASK_DS_GC_B50) >> 16)
#define NE1_GET_DS_GC_B51(r)    (((r) & NE1_MASK_DS_GC_B51) >> 24)
#define NE1_GET_DS_GC_B52(r)    (((r) & NE1_MASK_DS_GC_B52) >> 32)
#define NE1_GET_DS_GC_B53(r)    (((r) & NE1_MASK_DS_GC_B53) >> 40)
#define NE1_GET_DS_GC_B54(r)    (((r) & NE1_MASK_DS_GC_B54) >> 48)
#define NE1_GET_DS_GC_B55(r)    (((r) & NE1_MASK_DS_GC_B55) >> 56)
#define NE1_GET_DS_GC_B56(r)    (((r) & NE1_MASK_DS_GC_B56) >> 0)
#define NE1_GET_DS_GC_B57(r)    (((r) & NE1_MASK_DS_GC_B57) >> 8)
#define NE1_GET_DS_GC_B58(r)    (((r) & NE1_MASK_DS_GC_B58) >> 16)
#define NE1_GET_DS_GC_B59(r)    (((r) & NE1_MASK_DS_GC_B59) >> 24)
#define NE1_GET_DS_GC_B60(r)    (((r) & NE1_MASK_DS_GC_B60) >> 32)
#define NE1_GET_DS_GC_B61(r)    (((r) & NE1_MASK_DS_GC_B61) >> 40)
#define NE1_GET_DS_GC_B62(r)    (((r) & NE1_MASK_DS_GC_B62) >> 48)
#define NE1_GET_DS_GC_B63(r)    (((r) & NE1_MASK_DS_GC_B63) >> 56)
#define NE1_GET_DS_GC_B64(r)    (((r) & NE1_MASK_DS_GC_B64) >> 0)
#define NE1_GET_DS_GC_B65(r)    (((r) & NE1_MASK_DS_GC_B65) >> 8)
#define NE1_GET_DS_GC_B66(r)    (((r) & NE1_MASK_DS_GC_B66) >> 16)
#define NE1_GET_DS_GC_B67(r)    (((r) & NE1_MASK_DS_GC_B67) >> 24)
#define NE1_GET_DS_GC_B68(r)    (((r) & NE1_MASK_DS_GC_B68) >> 32)
#define NE1_GET_DS_GC_B69(r)    (((r) & NE1_MASK_DS_GC_B69) >> 40)
#define NE1_GET_DS_GC_B70(r)    (((r) & NE1_MASK_DS_GC_B70) >> 48)
#define NE1_GET_DS_GC_B71(r)    (((r) & NE1_MASK_DS_GC_B71) >> 56)
#define NE1_GET_DS_GC_B72(r)    (((r) & NE1_MASK_DS_GC_B72) >> 0)
#define NE1_GET_DS_GC_B73(r)    (((r) & NE1_MASK_DS_GC_B73) >> 8)
#define NE1_GET_DS_GC_B74(r)    (((r) & NE1_MASK_DS_GC_B74) >> 16)
#define NE1_GET_DS_GC_B75(r)    (((r) & NE1_MASK_DS_GC_B75) >> 24)
#define NE1_GET_DS_GC_B76(r)    (((r) & NE1_MASK_DS_GC_B76) >> 32)
#define NE1_GET_DS_GC_B77(r)    (((r) & NE1_MASK_DS_GC_B77) >> 40)
#define NE1_GET_DS_GC_B78(r)    (((r) & NE1_MASK_DS_GC_B78) >> 48)
#define NE1_GET_DS_GC_B79(r)    (((r) & NE1_MASK_DS_GC_B79) >> 56)
#define NE1_GET_DS_GC_B80(r)    (((r) & NE1_MASK_DS_GC_B80) >> 0)
#define NE1_GET_DS_GC_B81(r)    (((r) & NE1_MASK_DS_GC_B81) >> 8)
#define NE1_GET_DS_GC_B82(r)    (((r) & NE1_MASK_DS_GC_B82) >> 16)
#define NE1_GET_DS_GC_B83(r)    (((r) & NE1_MASK_DS_GC_B83) >> 24)
#define NE1_GET_DS_GC_B84(r)    (((r) & NE1_MASK_DS_GC_B84) >> 32)
#define NE1_GET_DS_GC_B85(r)    (((r) & NE1_MASK_DS_GC_B85) >> 40)
#define NE1_GET_DS_GC_B86(r)    (((r) & NE1_MASK_DS_GC_B86) >> 48)
#define NE1_GET_DS_GC_B87(r)    (((r) & NE1_MASK_DS_GC_B87) >> 56)
#define NE1_GET_DS_GC_B88(r)    (((r) & NE1_MASK_DS_GC_B88) >> 0)
#define NE1_GET_DS_GC_B89(r)    (((r) & NE1_MASK_DS_GC_B89) >> 8)
#define NE1_GET_DS_GC_B90(r)    (((r) & NE1_MASK_DS_GC_B90) >> 16)
#define NE1_GET_DS_GC_B91(r)    (((r) & NE1_MASK_DS_GC_B91) >> 24)
#define NE1_GET_DS_GC_B92(r)    (((r) & NE1_MASK_DS_GC_B92) >> 32)
#define NE1_GET_DS_GC_B93(r)    (((r) & NE1_MASK_DS_GC_B93) >> 40)
#define NE1_GET_DS_GC_B94(r)    (((r) & NE1_MASK_DS_GC_B94) >> 48)
#define NE1_GET_DS_GC_B95(r)    (((r) & NE1_MASK_DS_GC_B95) >> 56)
#define NE1_GET_DS_GC_B96(r)    (((r) & NE1_MASK_DS_GC_B96) >> 0)
#define NE1_GET_DS_GC_B97(r)    (((r) & NE1_MASK_DS_GC_B97) >> 8)
#define NE1_GET_DS_GC_B98(r)    (((r) & NE1_MASK_DS_GC_B98) >> 16)
#define NE1_GET_DS_GC_B99(r)    (((r) & NE1_MASK_DS_GC_B99) >> 24)
#define NE1_GET_DS_GC_B100(r)   (((r) & NE1_MASK_DS_GC_B100) >> 32)
#define NE1_GET_DS_GC_B101(r)   (((r) & NE1_MASK_DS_GC_B101) >> 40)
#define NE1_GET_DS_GC_B102(r)   (((r) & NE1_MASK_DS_GC_B102) >> 48)
#define NE1_GET_DS_GC_B103(r)   (((r) & NE1_MASK_DS_GC_B103) >> 56)
#define NE1_GET_DS_GC_B104(r)   (((r) & NE1_MASK_DS_GC_B104) >> 0)
#define NE1_GET_DS_GC_B105(r)   (((r) & NE1_MASK_DS_GC_B105) >> 8)
#define NE1_GET_DS_GC_B106(r)   (((r) & NE1_MASK_DS_GC_B106) >> 16)
#define NE1_GET_DS_GC_B107(r)   (((r) & NE1_MASK_DS_GC_B107) >> 24)
#define NE1_GET_DS_GC_B108(r)   (((r) & NE1_MASK_DS_GC_B108) >> 32)
#define NE1_GET_DS_GC_B109(r)   (((r) & NE1_MASK_DS_GC_B109) >> 40)
#define NE1_GET_DS_GC_B110(r)   (((r) & NE1_MASK_DS_GC_B110) >> 48)
#define NE1_GET_DS_GC_B111(r)   (((r) & NE1_MASK_DS_GC_B111) >> 56)
#define NE1_GET_DS_GC_B112(r)   (((r) & NE1_MASK_DS_GC_B112) >> 0)
#define NE1_GET_DS_GC_B113(r)   (((r) & NE1_MASK_DS_GC_B113) >> 8)
#define NE1_GET_DS_GC_B114(r)   (((r) & NE1_MASK_DS_GC_B114) >> 16)
#define NE1_GET_DS_GC_B115(r)   (((r) & NE1_MASK_DS_GC_B115) >> 24)
#define NE1_GET_DS_GC_B116(r)   (((r) & NE1_MASK_DS_GC_B116) >> 32)
#define NE1_GET_DS_GC_B117(r)   (((r) & NE1_MASK_DS_GC_B117) >> 40)
#define NE1_GET_DS_GC_B118(r)   (((r) & NE1_MASK_DS_GC_B118) >> 48)
#define NE1_GET_DS_GC_B119(r)   (((r) & NE1_MASK_DS_GC_B119) >> 56)
#define NE1_GET_DS_GC_B120(r)   (((r) & NE1_MASK_DS_GC_B120) >> 0)
#define NE1_GET_DS_GC_B121(r)   (((r) & NE1_MASK_DS_GC_B121) >> 8)
#define NE1_GET_DS_GC_B122(r)   (((r) & NE1_MASK_DS_GC_B122) >> 16)
#define NE1_GET_DS_GC_B123(r)   (((r) & NE1_MASK_DS_GC_B123) >> 24)
#define NE1_GET_DS_GC_B124(r)   (((r) & NE1_MASK_DS_GC_B124) >> 32)
#define NE1_GET_DS_GC_B125(r)   (((r) & NE1_MASK_DS_GC_B125) >> 40)
#define NE1_GET_DS_GC_B126(r)   (((r) & NE1_MASK_DS_GC_B126) >> 48)
#define NE1_GET_DS_GC_B127(r)   (((r) & NE1_MASK_DS_GC_B127) >> 56)
#define NE1_GET_DS_GC_B128(r)   (((r) & NE1_MASK_DS_GC_B128) >> 0)
#define NE1_GET_DS_GC_B129(r)   (((r) & NE1_MASK_DS_GC_B129) >> 8)
#define NE1_GET_DS_GC_B130(r)   (((r) & NE1_MASK_DS_GC_B130) >> 16)
#define NE1_GET_DS_GC_B131(r)   (((r) & NE1_MASK_DS_GC_B131) >> 24)
#define NE1_GET_DS_GC_B132(r)   (((r) & NE1_MASK_DS_GC_B132) >> 32)
#define NE1_GET_DS_GC_B133(r)   (((r) & NE1_MASK_DS_GC_B133) >> 40)
#define NE1_GET_DS_GC_B134(r)   (((r) & NE1_MASK_DS_GC_B134) >> 48)
#define NE1_GET_DS_GC_B135(r)   (((r) & NE1_MASK_DS_GC_B135) >> 56)
#define NE1_GET_DS_GC_B136(r)   (((r) & NE1_MASK_DS_GC_B136) >> 0)
#define NE1_GET_DS_GC_B137(r)   (((r) & NE1_MASK_DS_GC_B137) >> 8)
#define NE1_GET_DS_GC_B138(r)   (((r) & NE1_MASK_DS_GC_B138) >> 16)
#define NE1_GET_DS_GC_B139(r)   (((r) & NE1_MASK_DS_GC_B139) >> 24)
#define NE1_GET_DS_GC_B140(r)   (((r) & NE1_MASK_DS_GC_B140) >> 32)
#define NE1_GET_DS_GC_B141(r)   (((r) & NE1_MASK_DS_GC_B141) >> 40)
#define NE1_GET_DS_GC_B142(r)   (((r) & NE1_MASK_DS_GC_B142) >> 48)
#define NE1_GET_DS_GC_B143(r)   (((r) & NE1_MASK_DS_GC_B143) >> 56)
#define NE1_GET_DS_GC_B144(r)   (((r) & NE1_MASK_DS_GC_B144) >> 0)
#define NE1_GET_DS_GC_B145(r)   (((r) & NE1_MASK_DS_GC_B145) >> 8)
#define NE1_GET_DS_GC_B146(r)   (((r) & NE1_MASK_DS_GC_B146) >> 16)
#define NE1_GET_DS_GC_B147(r)   (((r) & NE1_MASK_DS_GC_B147) >> 24)
#define NE1_GET_DS_GC_B148(r)   (((r) & NE1_MASK_DS_GC_B148) >> 32)
#define NE1_GET_DS_GC_B149(r)   (((r) & NE1_MASK_DS_GC_B149) >> 40)
#define NE1_GET_DS_GC_B150(r)   (((r) & NE1_MASK_DS_GC_B150) >> 48)
#define NE1_GET_DS_GC_B151(r)   (((r) & NE1_MASK_DS_GC_B151) >> 56)
#define NE1_GET_DS_GC_B152(r)   (((r) & NE1_MASK_DS_GC_B152) >> 0)
#define NE1_GET_DS_GC_B153(r)   (((r) & NE1_MASK_DS_GC_B153) >> 8)
#define NE1_GET_DS_GC_B154(r)   (((r) & NE1_MASK_DS_GC_B154) >> 16)
#define NE1_GET_DS_GC_B155(r)   (((r) & NE1_MASK_DS_GC_B155) >> 24)
#define NE1_GET_DS_GC_B156(r)   (((r) & NE1_MASK_DS_GC_B156) >> 32)
#define NE1_GET_DS_GC_B157(r)   (((r) & NE1_MASK_DS_GC_B157) >> 40)
#define NE1_GET_DS_GC_B158(r)   (((r) & NE1_MASK_DS_GC_B158) >> 48)
#define NE1_GET_DS_GC_B159(r)   (((r) & NE1_MASK_DS_GC_B159) >> 56)
#define NE1_GET_DS_GC_B160(r)   (((r) & NE1_MASK_DS_GC_B160) >> 0)
#define NE1_GET_DS_GC_B161(r)   (((r) & NE1_MASK_DS_GC_B161) >> 8)
#define NE1_GET_DS_GC_B162(r)   (((r) & NE1_MASK_DS_GC_B162) >> 16)
#define NE1_GET_DS_GC_B163(r)   (((r) & NE1_MASK_DS_GC_B163) >> 24)
#define NE1_GET_DS_GC_B164(r)   (((r) & NE1_MASK_DS_GC_B164) >> 32)
#define NE1_GET_DS_GC_B165(r)   (((r) & NE1_MASK_DS_GC_B165) >> 40)
#define NE1_GET_DS_GC_B166(r)   (((r) & NE1_MASK_DS_GC_B166) >> 48)
#define NE1_GET_DS_GC_B167(r)   (((r) & NE1_MASK_DS_GC_B167) >> 56)
#define NE1_GET_DS_GC_B168(r)   (((r) & NE1_MASK_DS_GC_B168) >> 0)
#define NE1_GET_DS_GC_B169(r)   (((r) & NE1_MASK_DS_GC_B169) >> 8)
#define NE1_GET_DS_GC_B170(r)   (((r) & NE1_MASK_DS_GC_B170) >> 16)
#define NE1_GET_DS_GC_B171(r)   (((r) & NE1_MASK_DS_GC_B171) >> 24)
#define NE1_GET_DS_GC_B172(r)   (((r) & NE1_MASK_DS_GC_B172) >> 32)
#define NE1_GET_DS_GC_B173(r)   (((r) & NE1_MASK_DS_GC_B173) >> 40)
#define NE1_GET_DS_GC_B174(r)   (((r) & NE1_MASK_DS_GC_B174) >> 48)
#define NE1_GET_DS_GC_B175(r)   (((r) & NE1_MASK_DS_GC_B175) >> 56)
#define NE1_GET_DS_GC_B176(r)   (((r) & NE1_MASK_DS_GC_B176) >> 0)
#define NE1_GET_DS_GC_B177(r)   (((r) & NE1_MASK_DS_GC_B177) >> 8)
#define NE1_GET_DS_GC_B178(r)   (((r) & NE1_MASK_DS_GC_B178) >> 16)
#define NE1_GET_DS_GC_B179(r)   (((r) & NE1_MASK_DS_GC_B179) >> 24)
#define NE1_GET_DS_GC_B180(r)   (((r) & NE1_MASK_DS_GC_B180) >> 32)
#define NE1_GET_DS_GC_B181(r)   (((r) & NE1_MASK_DS_GC_B181) >> 40)
#define NE1_GET_DS_GC_B182(r)   (((r) & NE1_MASK_DS_GC_B182) >> 48)
#define NE1_GET_DS_GC_B183(r)   (((r) & NE1_MASK_DS_GC_B183) >> 56)
#define NE1_GET_DS_GC_B184(r)   (((r) & NE1_MASK_DS_GC_B184) >> 0)
#define NE1_GET_DS_GC_B185(r)   (((r) & NE1_MASK_DS_GC_B185) >> 8)
#define NE1_GET_DS_GC_B186(r)   (((r) & NE1_MASK_DS_GC_B186) >> 16)
#define NE1_GET_DS_GC_B187(r)   (((r) & NE1_MASK_DS_GC_B187) >> 24)
#define NE1_GET_DS_GC_B188(r)   (((r) & NE1_MASK_DS_GC_B188) >> 32)
#define NE1_GET_DS_GC_B189(r)   (((r) & NE1_MASK_DS_GC_B189) >> 40)
#define NE1_GET_DS_GC_B190(r)   (((r) & NE1_MASK_DS_GC_B190) >> 48)
#define NE1_GET_DS_GC_B191(r)   (((r) & NE1_MASK_DS_GC_B191) >> 56)
#define NE1_GET_DS_GC_B192(r)   (((r) & NE1_MASK_DS_GC_B192) >> 0)
#define NE1_GET_DS_GC_B193(r)   (((r) & NE1_MASK_DS_GC_B193) >> 8)
#define NE1_GET_DS_GC_B194(r)   (((r) & NE1_MASK_DS_GC_B194) >> 16)
#define NE1_GET_DS_GC_B195(r)   (((r) & NE1_MASK_DS_GC_B195) >> 24)
#define NE1_GET_DS_GC_B196(r)   (((r) & NE1_MASK_DS_GC_B196) >> 32)
#define NE1_GET_DS_GC_B197(r)   (((r) & NE1_MASK_DS_GC_B197) >> 40)
#define NE1_GET_DS_GC_B198(r)   (((r) & NE1_MASK_DS_GC_B198) >> 48)
#define NE1_GET_DS_GC_B199(r)   (((r) & NE1_MASK_DS_GC_B199) >> 56)
#define NE1_GET_DS_GC_B200(r)   (((r) & NE1_MASK_DS_GC_B200) >> 0)
#define NE1_GET_DS_GC_B201(r)   (((r) & NE1_MASK_DS_GC_B201) >> 8)
#define NE1_GET_DS_GC_B202(r)   (((r) & NE1_MASK_DS_GC_B202) >> 16)
#define NE1_GET_DS_GC_B203(r)   (((r) & NE1_MASK_DS_GC_B203) >> 24)
#define NE1_GET_DS_GC_B204(r)   (((r) & NE1_MASK_DS_GC_B204) >> 32)
#define NE1_GET_DS_GC_B205(r)   (((r) & NE1_MASK_DS_GC_B205) >> 40)
#define NE1_GET_DS_GC_B206(r)   (((r) & NE1_MASK_DS_GC_B206) >> 48)
#define NE1_GET_DS_GC_B207(r)   (((r) & NE1_MASK_DS_GC_B207) >> 56)
#define NE1_GET_DS_GC_B208(r)   (((r) & NE1_MASK_DS_GC_B208) >> 0)
#define NE1_GET_DS_GC_B209(r)   (((r) & NE1_MASK_DS_GC_B209) >> 8)
#define NE1_GET_DS_GC_B210(r)   (((r) & NE1_MASK_DS_GC_B210) >> 16)
#define NE1_GET_DS_GC_B211(r)   (((r) & NE1_MASK_DS_GC_B211) >> 24)
#define NE1_GET_DS_GC_B212(r)   (((r) & NE1_MASK_DS_GC_B212) >> 32)
#define NE1_GET_DS_GC_B213(r)   (((r) & NE1_MASK_DS_GC_B213) >> 40)
#define NE1_GET_DS_GC_B214(r)   (((r) & NE1_MASK_DS_GC_B214) >> 48)
#define NE1_GET_DS_GC_B215(r)   (((r) & NE1_MASK_DS_GC_B215) >> 56)
#define NE1_GET_DS_GC_B216(r)   (((r) & NE1_MASK_DS_GC_B216) >> 0)
#define NE1_GET_DS_GC_B217(r)   (((r) & NE1_MASK_DS_GC_B217) >> 8)
#define NE1_GET_DS_GC_B218(r)   (((r) & NE1_MASK_DS_GC_B218) >> 16)
#define NE1_GET_DS_GC_B219(r)   (((r) & NE1_MASK_DS_GC_B219) >> 24)
#define NE1_GET_DS_GC_B220(r)   (((r) & NE1_MASK_DS_GC_B220) >> 32)
#define NE1_GET_DS_GC_B221(r)   (((r) & NE1_MASK_DS_GC_B221) >> 40)
#define NE1_GET_DS_GC_B222(r)   (((r) & NE1_MASK_DS_GC_B222) >> 48)
#define NE1_GET_DS_GC_B223(r)   (((r) & NE1_MASK_DS_GC_B223) >> 56)
#define NE1_GET_DS_GC_B224(r)   (((r) & NE1_MASK_DS_GC_B224) >> 0)
#define NE1_GET_DS_GC_B225(r)   (((r) & NE1_MASK_DS_GC_B225) >> 8)
#define NE1_GET_DS_GC_B226(r)   (((r) & NE1_MASK_DS_GC_B226) >> 16)
#define NE1_GET_DS_GC_B227(r)   (((r) & NE1_MASK_DS_GC_B227) >> 24)
#define NE1_GET_DS_GC_B228(r)   (((r) & NE1_MASK_DS_GC_B228) >> 32)
#define NE1_GET_DS_GC_B229(r)   (((r) & NE1_MASK_DS_GC_B229) >> 40)
#define NE1_GET_DS_GC_B230(r)   (((r) & NE1_MASK_DS_GC_B230) >> 48)
#define NE1_GET_DS_GC_B231(r)   (((r) & NE1_MASK_DS_GC_B231) >> 56)
#define NE1_GET_DS_GC_B232(r)   (((r) & NE1_MASK_DS_GC_B232) >> 0)
#define NE1_GET_DS_GC_B233(r)   (((r) & NE1_MASK_DS_GC_B233) >> 8)
#define NE1_GET_DS_GC_B234(r)   (((r) & NE1_MASK_DS_GC_B234) >> 16)
#define NE1_GET_DS_GC_B235(r)   (((r) & NE1_MASK_DS_GC_B235) >> 24)
#define NE1_GET_DS_GC_B236(r)   (((r) & NE1_MASK_DS_GC_B236) >> 32)
#define NE1_GET_DS_GC_B237(r)   (((r) & NE1_MASK_DS_GC_B237) >> 40)
#define NE1_GET_DS_GC_B238(r)   (((r) & NE1_MASK_DS_GC_B238) >> 48)
#define NE1_GET_DS_GC_B239(r)   (((r) & NE1_MASK_DS_GC_B239) >> 56)
#define NE1_GET_DS_GC_B240(r)   (((r) & NE1_MASK_DS_GC_B240) >> 0)
#define NE1_GET_DS_GC_B241(r)   (((r) & NE1_MASK_DS_GC_B241) >> 8)
#define NE1_GET_DS_GC_B242(r)   (((r) & NE1_MASK_DS_GC_B242) >> 16)
#define NE1_GET_DS_GC_B243(r)   (((r) & NE1_MASK_DS_GC_B243) >> 24)
#define NE1_GET_DS_GC_B244(r)   (((r) & NE1_MASK_DS_GC_B244) >> 32)
#define NE1_GET_DS_GC_B245(r)   (((r) & NE1_MASK_DS_GC_B245) >> 40)
#define NE1_GET_DS_GC_B246(r)   (((r) & NE1_MASK_DS_GC_B246) >> 48)
#define NE1_GET_DS_GC_B247(r)   (((r) & NE1_MASK_DS_GC_B247) >> 56)
#define NE1_GET_DS_GC_B248(r)   (((r) & NE1_MASK_DS_GC_B248) >> 0)
#define NE1_GET_DS_GC_B249(r)   (((r) & NE1_MASK_DS_GC_B249) >> 8)
#define NE1_GET_DS_GC_B250(r)   (((r) & NE1_MASK_DS_GC_B250) >> 16)
#define NE1_GET_DS_GC_B251(r)   (((r) & NE1_MASK_DS_GC_B251) >> 24)
#define NE1_GET_DS_GC_B252(r)   (((r) & NE1_MASK_DS_GC_B252) >> 32)
#define NE1_GET_DS_GC_B253(r)   (((r) & NE1_MASK_DS_GC_B253) >> 40)
#define NE1_GET_DS_GC_B254(r)   (((r) & NE1_MASK_DS_GC_B254) >> 48)
#define NE1_GET_DS_GC_B255(r)   (((r) & NE1_MASK_DS_GC_B255) >> 56)
#define NE1_GET_HC_DF(r)        (((r) & NE1_MASK_HC_DF) >> 0)
#define NE1_GET_HC_LUT(r)       (((r) & NE1_MASK_HC_LUT) >> 0)

#define NE1_SET_REG(r,m,s)      (((r) << (s) ) & (m))

#define NE1_SET_IC_SS(r)        (((r) <<0) & NE1_MASK_IC_SS)
#define NE1_SET_IC_CS(r)        (((r) <<1) & NE1_MASK_IC_CS)
#define NE1_SET_IC_SC(r)        (((r) <<2) & NE1_MASK_IC_SC)
#define NE1_SET_IC_CP(r)        (((r) <<3) & NE1_MASK_IC_CP)
#define NE1_SET_IC_HP(r)        (((r) <<4) & NE1_MASK_IC_HP)
#define NE1_SET_IC_EHP(r)       (((r) <<5) & NE1_MASK_IC_EHP)
#define NE1_SET_IC_VP(r)        (((r) <<6) & NE1_MASK_IC_VP)
#define NE1_SET_IC_EVP(r)       (((r) <<7) & NE1_MASK_IC_EVP)
#define NE1_SET_IC_DEP(r)       (((r) <<8) & NE1_MASK_IC_DEP)
#define NE1_SET_IC_CDP(r)       (((r) <<9) & NE1_MASK_IC_CDP)
#define NE1_SET_IC_DD(r)        (((r) <<16) & NE1_MASK_IC_DD)
#define NE1_SET_IC_CDD(r)       (((r) <<20) & NE1_MASK_IC_CDD)
#define NE1_SET_IC_IS(r)        (((r) <<0) & NE1_MASK_IC_IS)
#define NE1_SET_IC_VCNT(r)      (((r) <<8) & NE1_MASK_IC_VCNT)
#define NE1_SET_IC_VSS(r)       (((r) <<22) & NE1_MASK_IC_VSS)
#define NE1_SET_IC_VBS(r)       (((r) <<23) & NE1_MASK_IC_VBS)
#define NE1_SET_IC_ICL(r)       (((r) <<0) & NE1_MASK_IC_ICL)
#define NE1_SET_IC_IP(r)        (((r) <<0) & NE1_MASK_IC_IP)
#define NE1_SET_IC_IL(r)        (((r) <<0) & NE1_MASK_IC_IL)
#define NE1_SET_IC_AL_H(r)      (((r) <<0) & NE1_MASK_IC_AL_H)
#define NE1_SET_IC_ISEL0(r)     (((r) <<0) & NE1_MASK_IC_ISEL0)
#define NE1_SET_IC_IF0(r)       (((r) <<4) & NE1_MASK_IC_IF0)
#define NE1_SET_IC_ISEL1(r)     (((r) <<8) & NE1_MASK_IC_ISEL1)
#define NE1_SET_IC_IF1(r)       (((r) <<12) & NE1_MASK_IC_IF1)
#define NE1_SET_IC_ISEL2(r)     (((r) <<16) & NE1_MASK_IC_ISEL2)
#define NE1_SET_IC_IF2(r)       (((r) <<20) & NE1_MASK_IC_IF2)
#define NE1_SET_IC_ISEL3(r)     (((r) <<24) & NE1_MASK_IC_ISEL3)
#define NE1_SET_IC_IF3(r)       (((r) <<28) & NE1_MASK_IC_IF3)
#define NE1_SET_IC_ISEL4(r)     (((r) <<32) & NE1_MASK_IC_ISEL4)
#define NE1_SET_IC_IF4(r)       (((r) <<36) & NE1_MASK_IC_IF4)
#define NE1_SET_IC_ISEL5(r)     (((r) <<40) & NE1_MASK_IC_ISEL5)
#define NE1_SET_IC_IF5(r)       (((r) <<44) & NE1_MASK_IC_IF5)
#define NE1_SET_IC_ISEL6(r)     (((r) <<48) & NE1_MASK_IC_ISEL6)
#define NE1_SET_IC_IF6(r)       (((r) <<52) & NE1_MASK_IC_IF6)
#define NE1_SET_DS_HR(r)        (((r) <<0) & NE1_MASK_DS_HR)
#define NE1_SET_DS_VR(r)        (((r) <<16) & NE1_MASK_DS_VR)
#define NE1_SET_DS_TH(r)        (((r) <<32) & NE1_MASK_DS_TH)
#define NE1_SET_DS_TV(r)        (((r) <<48) & NE1_MASK_DS_TV)
#define NE1_SET_DS_THP(r)       (((r) <<0) & NE1_MASK_DS_THP)
#define NE1_SET_DS_TVP(r)       (((r) <<16) & NE1_MASK_DS_TVP)
#define NE1_SET_DS_THB(r)       (((r) <<32) & NE1_MASK_DS_THB)
#define NE1_SET_DS_TVB(r)       (((r) <<48) & NE1_MASK_DS_TVB)
#define NE1_SET_DS_OC(r)        (((r) <<0) & NE1_MASK_DS_OC)
#define NE1_SET_DS_DS(r)        (((r) <<4) & NE1_MASK_DS_DS)
#define NE1_SET_DS_DM_H(r)      (((r) <<6) & NE1_MASK_DS_DM_H)
#define NE1_SET_DS_DM_V(r)      (((r) <<7) & NE1_MASK_DS_DM_V)
#define NE1_SET_DS_GS(r)        (((r) <<15) & NE1_MASK_DS_GS)
#define NE1_SET_DS_DC0(r)       (((r) <<0) & NE1_MASK_DS_DC0)
#define NE1_SET_DS_DC1(r)       (((r) <<4) & NE1_MASK_DS_DC1)
#define NE1_SET_DS_DC2(r)       (((r) <<8) & NE1_MASK_DS_DC2)
#define NE1_SET_DS_DC3(r)       (((r) <<12) & NE1_MASK_DS_DC3)
#define NE1_SET_DS_DC4(r)       (((r) <<16) & NE1_MASK_DS_DC4)
#define NE1_SET_DS_DC5(r)       (((r) <<20) & NE1_MASK_DS_DC5)
#define NE1_SET_DS_DC6(r)       (((r) <<24) & NE1_MASK_DS_DC6)
#define NE1_SET_DS_DC7(r)       (((r) <<28) & NE1_MASK_DS_DC7)
#define NE1_SET_DS_DC8(r)       (((r) <<32) & NE1_MASK_DS_DC8)
#define NE1_SET_DS_DC9(r)       (((r) <<36) & NE1_MASK_DS_DC9)
#define NE1_SET_DS_DC10(r)      (((r) <<40) & NE1_MASK_DS_DC10)
#define NE1_SET_DS_DC11(r)      (((r) <<44) & NE1_MASK_DS_DC11)
#define NE1_SET_DS_DC12(r)      (((r) <<48) & NE1_MASK_DS_DC12)
#define NE1_SET_DS_DC13(r)      (((r) <<52) & NE1_MASK_DS_DC13)
#define NE1_SET_DS_DC14(r)      (((r) <<56) & NE1_MASK_DS_DC14)
#define NE1_SET_DS_DC15(r)      (((r) <<60) & NE1_MASK_DS_DC15)
#define NE1_SET_DS_BC(r)        (((r) <<0) & NE1_MASK_DS_BC)
#define NE1_SET_MF_ES0(r)       (((r) <<0) & NE1_MASK_MF_ES0)
#define NE1_SET_MF_ES1(r)       (((r) <<2) & NE1_MASK_MF_ES1)
#define NE1_SET_MF_ES2(r)       (((r) <<4) & NE1_MASK_MF_ES2)
#define NE1_SET_MF_ES3(r)       (((r) <<6) & NE1_MASK_MF_ES3)
#define NE1_SET_MF_ES4(r)       (((r) <<8) & NE1_MASK_MF_ES4)
#define NE1_SET_MF_ES5(r)       (((r) <<10) & NE1_MASK_MF_ES5)
#define NE1_SET_MF_ES6(r)       (((r) <<12) & NE1_MASK_MF_ES6)
#define NE1_SET_MF_BE0(r)       (((r) <<14) & NE1_MASK_MF_BE0)
#define NE1_SET_MF_BE1(r)       (((r) <<16) & NE1_MASK_MF_BE1)
#define NE1_SET_MF_BE2(r)       (((r) <<17) & NE1_MASK_MF_BE2)
#define NE1_SET_MF_BE3(r)       (((r) <<18) & NE1_MASK_MF_BE3)
#define NE1_SET_MF_BE4(r)       (((r) <<19) & NE1_MASK_MF_BE4)
#define NE1_SET_MF_BE5(r)       (((r) <<20) & NE1_MASK_MF_BE5)
#define NE1_SET_MF_BE6(r)       (((r) <<21) & NE1_MASK_MF_BE6)
#define NE1_SET_MF_LBS(r)       (((r) <<24) & NE1_MASK_MF_LBS)
#define NE1_SET_MF_PF0(r)       (((r) <<0) & NE1_MASK_MF_PF0)
#define NE1_SET_MF_YC0(r)       (((r) <<4) & NE1_MASK_MF_YC0)
#define NE1_SET_MF_PS0(r)       (((r) <<8) & NE1_MASK_MF_PS0)
#define NE1_SET_MF_MS_H0(r)     (((r) <<10) & NE1_MASK_MF_MS_H0)
#define NE1_SET_MF_MS_V0(r)     (((r) <<11) & NE1_MASK_MF_MS_V0)
#define NE1_SET_MF_RL0(r)       (((r) <<12) & NE1_MASK_MF_RL0)
#define NE1_SET_MF_AF0(r)       (((r) <<15) & NE1_MASK_MF_AF0)
#define NE1_SET_MF_PF1(r)       (((r) <<16) & NE1_MASK_MF_PF1)
#define NE1_SET_MF_YC1(r)       (((r) <<20) & NE1_MASK_MF_YC1)
#define NE1_SET_MF_PS1(r)       (((r) <<24) & NE1_MASK_MF_PS1)
#define NE1_SET_MF_MS_H1(r)     (((r) <<26) & NE1_MASK_MF_MS_H1)
#define NE1_SET_MF_RL1(r)       (((r) <<28) & NE1_MASK_MF_RL1)
#define NE1_SET_MF_AF1(r)       (((r) <<31) & NE1_MASK_MF_AF1)
#define NE1_SET_MF_PF2(r)       (((r) <<32) & NE1_MASK_MF_PF2)
#define NE1_SET_MF_YC2(r)       (((r) <<36) & NE1_MASK_MF_YC2)
#define NE1_SET_MF_PS2(r)       (((r) <<40) & NE1_MASK_MF_PS2)
#define NE1_SET_MF_MS_H2(r)     (((r) <<42) & NE1_MASK_MF_MS_H2)
#define NE1_SET_MF_MS_V2(r)     (((r) <<43) & NE1_MASK_MF_MS_V2)
#define NE1_SET_MF_RL2(r)       (((r) <<44) & NE1_MASK_MF_RL2)
#define NE1_SET_MF_AF2(r)       (((r) <<47) & NE1_MASK_MF_AF2)
#define NE1_SET_MF_PF3(r)       (((r) <<48) & NE1_MASK_MF_PF3)
#define NE1_SET_MF_YC3(r)       (((r) <<52) & NE1_MASK_MF_YC3)
#define NE1_SET_MF_PS3(r)       (((r) <<56) & NE1_MASK_MF_PS3)
#define NE1_SET_MF_MS_H3(r)     (((r) <<58) & NE1_MASK_MF_MS_H3)
#define NE1_SET_MF_RL3(r)       (((r) <<60) & NE1_MASK_MF_RL3)
#define NE1_SET_MF_AF3(r)       (((r) <<63) & NE1_MASK_MF_AF3)
#define NE1_SET_MF_PF4(r)       (((r) <<0) & NE1_MASK_MF_PF4)
#define NE1_SET_MF_YC4(r)       (((r) <<4) & NE1_MASK_MF_YC4)
#define NE1_SET_MF_PS4(r)       (((r) <<8) & NE1_MASK_MF_PS4)
#define NE1_SET_MF_MS_H4(r)     (((r) <<10) & NE1_MASK_MF_MS_H4)
#define NE1_SET_MF_MS_V4(r)     (((r) <<11) & NE1_MASK_MF_MS_V4)
#define NE1_SET_MF_RL4(r)       (((r) <<12) & NE1_MASK_MF_RL4)
#define NE1_SET_MF_AF4(r)       (((r) <<15) & NE1_MASK_MF_AF4)
#define NE1_SET_MF_PF5(r)       (((r) <<16) & NE1_MASK_MF_PF5)
#define NE1_SET_MF_YC5(r)       (((r) <<20) & NE1_MASK_MF_YC5)
#define NE1_SET_MF_PS5(r)       (((r) <<24) & NE1_MASK_MF_PS5)
#define NE1_SET_MF_MS_H5(r)     (((r) <<26) & NE1_MASK_MF_MS_H5)
#define NE1_SET_MF_RL5(r)       (((r) <<28) & NE1_MASK_MF_RL5)
#define NE1_SET_MF_AF5(r)       (((r) <<31) & NE1_MASK_MF_AF5)
#define NE1_SET_MF_PF6(r)       (((r) <<32) & NE1_MASK_MF_PF6)
#define NE1_SET_MF_YC6(r)       (((r) <<36) & NE1_MASK_MF_YC6)
#define NE1_SET_MF_PS6(r)       (((r) <<40) & NE1_MASK_MF_PS6)
#define NE1_SET_MF_MS_H6(r)     (((r) <<42) & NE1_MASK_MF_MS_H6)
#define NE1_SET_MF_RL6(r)       (((r) <<44) & NE1_MASK_MF_RL6)
#define NE1_SET_MF_AF6(r)       (((r) <<47) & NE1_MASK_MF_AF6)
#define NE1_SET_MF_SA0(r)       (((r) <<4) & NE1_MASK_MF_SA0)
#define NE1_SET_MF_SA1(r)       (((r) <<36) & NE1_MASK_MF_SA1)
#define NE1_SET_MF_SA2(r)       (((r) <<4) & NE1_MASK_MF_SA2)
#define NE1_SET_MF_SA3(r)       (((r) <<36) & NE1_MASK_MF_SA3)
#define NE1_SET_MF_SA4(r)       (((r) <<4) & NE1_MASK_MF_SA4)
#define NE1_SET_MF_SA5(r)       (((r) <<36) & NE1_MASK_MF_SA5)
#define NE1_SET_MF_SA6(r)       (((r) <<4) & NE1_MASK_MF_SA6)
#define NE1_SET_MF_WS_H0(r)     (((r) <<0) & NE1_MASK_MF_WS_H0)
#define NE1_SET_MF_WS_H1(r)     (((r) <<16) & NE1_MASK_MF_WS_H1)
#define NE1_SET_MF_WS_H2(r)     (((r) <<32) & NE1_MASK_MF_WS_H2)
#define NE1_SET_MF_WS_H3(r)     (((r) <<48) & NE1_MASK_MF_WS_H3)
#define NE1_SET_MF_WS_H4(r)     (((r) <<0) & NE1_MASK_MF_WS_H4)
#define NE1_SET_MF_WS_H5(r)     (((r) <<16) & NE1_MASK_MF_WS_H5)
#define NE1_SET_MF_WS_H6(r)     (((r) <<32) & NE1_MASK_MF_WS_H6)
#define NE1_SET_MF_WS_V0(r)     (((r) <<0) & NE1_MASK_MF_WS_V0)
#define NE1_SET_MF_WS_V1(r)     (((r) <<16) & NE1_MASK_MF_WS_V1)
#define NE1_SET_MF_WS_V2(r)     (((r) <<32) & NE1_MASK_MF_WS_V2)
#define NE1_SET_MF_WS_V3(r)     (((r) <<48) & NE1_MASK_MF_WS_V3)
#define NE1_SET_MF_WS_V4(r)     (((r) <<0) & NE1_MASK_MF_WS_V4)
#define NE1_SET_MF_WS_V5(r)     (((r) <<16) & NE1_MASK_MF_WS_V5)
#define NE1_SET_MF_WS_V6(r)     (((r) <<32) & NE1_MASK_MF_WS_V6)
#define NE1_SET_MF_SP_X0(r)     (((r) <<0) & NE1_MASK_MF_SP_X0)
#define NE1_SET_MF_SP_X1(r)     (((r) <<16) & NE1_MASK_MF_SP_X1)
#define NE1_SET_MF_SP_X2(r)     (((r) <<32) & NE1_MASK_MF_SP_X2)
#define NE1_SET_MF_SP_X3(r)     (((r) <<48) & NE1_MASK_MF_SP_X3)
#define NE1_SET_MF_SP_X4(r)     (((r) <<0) & NE1_MASK_MF_SP_X4)
#define NE1_SET_MF_SP_X5(r)     (((r) <<16) & NE1_MASK_MF_SP_X5)
#define NE1_SET_MF_SP_X6(r)     (((r) <<32) & NE1_MASK_MF_SP_X6)
#define NE1_SET_MF_SP_Y0(r)     (((r) <<0) & NE1_MASK_MF_SP_Y0)
#define NE1_SET_MF_SP_Y1(r)     (((r) <<16) & NE1_MASK_MF_SP_Y1)
#define NE1_SET_MF_SP_Y2(r)     (((r) <<32) & NE1_MASK_MF_SP_Y2)
#define NE1_SET_MF_SP_Y3(r)     (((r) <<48) & NE1_MASK_MF_SP_Y3)
#define NE1_SET_MF_SP_Y4(r)     (((r) <<0) & NE1_MASK_MF_SP_Y4)
#define NE1_SET_MF_SP_Y5(r)     (((r) <<16) & NE1_MASK_MF_SP_Y5)
#define NE1_SET_MF_SP_Y6(r)     (((r) <<32) & NE1_MASK_MF_SP_Y6)
#define NE1_SET_MF_SP_YF0(r)    (((r) <<0) & NE1_MASK_MF_SP_YF0)
#define NE1_SET_MF_SP_YF2(r)    (((r) <<2) & NE1_MASK_MF_SP_YF2)
#define NE1_SET_MF_SP_YF4(r)    (((r) <<4) & NE1_MASK_MF_SP_YF4)
#define NE1_SET_MF_DS_H0(r)     (((r) <<0) & NE1_MASK_MF_DS_H0)
#define NE1_SET_MF_DS_H1(r)     (((r) <<16) & NE1_MASK_MF_DS_H1)
#define NE1_SET_MF_DS_H2(r)     (((r) <<32) & NE1_MASK_MF_DS_H2)
#define NE1_SET_MF_DS_H3(r)     (((r) <<48) & NE1_MASK_MF_DS_H3)
#define NE1_SET_MF_DS_H4(r)     (((r) <<0) & NE1_MASK_MF_DS_H4)
#define NE1_SET_MF_DS_H5(r)     (((r) <<16) & NE1_MASK_MF_DS_H5)
#define NE1_SET_MF_DS_H6(r)     (((r) <<32) & NE1_MASK_MF_DS_H6)
#define NE1_SET_MF_DS_V0(r)     (((r) <<0) & NE1_MASK_MF_DS_V0)
#define NE1_SET_MF_DS_V1(r)     (((r) <<16) & NE1_MASK_MF_DS_V1)
#define NE1_SET_MF_DS_V2(r)     (((r) <<32) & NE1_MASK_MF_DS_V2)
#define NE1_SET_MF_DS_V3(r)     (((r) <<48) & NE1_MASK_MF_DS_V3)
#define NE1_SET_MF_DS_V4(r)     (((r) <<0) & NE1_MASK_MF_DS_V4)
#define NE1_SET_MF_DS_V5(r)     (((r) <<16) & NE1_MASK_MF_DS_V5)
#define NE1_SET_MF_DS_V6(r)     (((r) <<32) & NE1_MASK_MF_DS_V6)
#define NE1_SET_MF_BC0(r)       (((r) <<0) & NE1_MASK_MF_BC0)
#define NE1_SET_MF_BC_S0(r)     (((r) <<28) & NE1_MASK_MF_BC_S0)
#define NE1_SET_MF_BC1(r)       (((r) <<32) & NE1_MASK_MF_BC1)
#define NE1_SET_MF_BC_S1(r)     (((r) <<60) & NE1_MASK_MF_BC_S1)
#define NE1_SET_MF_BC2(r)       (((r) <<0) & NE1_MASK_MF_BC2)
#define NE1_SET_MF_BC_S2(r)     (((r) <<28) & NE1_MASK_MF_BC_S2)
#define NE1_SET_MF_BC3(r)       (((r) <<32) & NE1_MASK_MF_BC3)
#define NE1_SET_MF_BC_S3(r)     (((r) <<60) & NE1_MASK_MF_BC_S3)
#define NE1_SET_MF_BC4(r)       (((r) <<0) & NE1_MASK_MF_BC4)
#define NE1_SET_MF_BC_S4(r)     (((r) <<28) & NE1_MASK_MF_BC_S4)
#define NE1_SET_MF_BC5(r)       (((r) <<32) & NE1_MASK_MF_BC5)
#define NE1_SET_MF_BC_S5(r)     (((r) <<60) & NE1_MASK_MF_BC_S5)
#define NE1_SET_MF_BC6(r)       (((r) <<0) & NE1_MASK_MF_BC6)
#define NE1_SET_MF_BC_S6(r)     (((r) <<28) & NE1_MASK_MF_BC_S6)
#define NE1_SET_CP_SA0(r)       (((r) <<4) & NE1_MASK_CP_SA0)
#define NE1_SET_CP_SA1(r)       (((r) <<36) & NE1_MASK_CP_SA1)
#define NE1_SET_CP_SA2(r)       (((r) <<4) & NE1_MASK_CP_SA2)
#define NE1_SET_CP_RN0(r)       (((r) <<0) & NE1_MASK_CP_RN0)
#define NE1_SET_CP_RN1(r)       (((r) <<4) & NE1_MASK_CP_RN1)
#define NE1_SET_CP_RN2(r)       (((r) <<8) & NE1_MASK_CP_RN2)
#define NE1_SET_DF_DC0(r)       (((r) <<0) & NE1_MASK_DF_DC0)
#define NE1_SET_DF_VS0(r)       (((r) <<8) & NE1_MASK_DF_VS0)
#define NE1_SET_DF_LC0(r)       (((r) <<12) & NE1_MASK_DF_LC0)
#define NE1_SET_DF_DC1(r)       (((r) <<16) & NE1_MASK_DF_DC1)
#define NE1_SET_DF_VS1(r)       (((r) <<24) & NE1_MASK_DF_VS1)
#define NE1_SET_DF_LC1(r)       (((r) <<28) & NE1_MASK_DF_LC1)
#define NE1_SET_DF_DC2(r)       (((r) <<32) & NE1_MASK_DF_DC2)
#define NE1_SET_DF_VS2(r)       (((r) <<40) & NE1_MASK_DF_VS2)
#define NE1_SET_DF_LC2(r)       (((r) <<44) & NE1_MASK_DF_LC2)
#define NE1_SET_DF_DC3(r)       (((r) <<48) & NE1_MASK_DF_DC3)
#define NE1_SET_DF_VS3(r)       (((r) <<56) & NE1_MASK_DF_VS3)
#define NE1_SET_DF_LC3(r)       (((r) <<60) & NE1_MASK_DF_LC3)
#define NE1_SET_DF_DC4(r)       (((r) <<0) & NE1_MASK_DF_DC4)
#define NE1_SET_DF_VS4(r)       (((r) <<8) & NE1_MASK_DF_VS4)
#define NE1_SET_DF_LC4(r)       (((r) <<12) & NE1_MASK_DF_LC4)
#define NE1_SET_DF_DC5(r)       (((r) <<16) & NE1_MASK_DF_DC5)
#define NE1_SET_DF_VS5(r)       (((r) <<24) & NE1_MASK_DF_VS5)
#define NE1_SET_DF_LC5(r)       (((r) <<28) & NE1_MASK_DF_LC5)
#define NE1_SET_DF_DC6(r)       (((r) <<32) & NE1_MASK_DF_DC6)
#define NE1_SET_DF_VS6(r)       (((r) <<40) & NE1_MASK_DF_VS6)
#define NE1_SET_DF_LC6(r)       (((r) <<44) & NE1_MASK_DF_LC6)
#define NE1_SET_DF_SP_X0(r)     (((r) <<0) & NE1_MASK_DF_SP_X0)
#define NE1_SET_DF_SP_X1(r)     (((r) <<16) & NE1_MASK_DF_SP_X1)
#define NE1_SET_DF_SP_X2(r)     (((r) <<32) & NE1_MASK_DF_SP_X2)
#define NE1_SET_DF_SP_X3(r)     (((r) <<48) & NE1_MASK_DF_SP_X3)
#define NE1_SET_DF_SP_X4(r)     (((r) <<0) & NE1_MASK_DF_SP_X4)
#define NE1_SET_DF_SP_X5(r)     (((r) <<16) & NE1_MASK_DF_SP_X5)
#define NE1_SET_DF_SP_X6(r)     (((r) <<32) & NE1_MASK_DF_SP_X6)
#define NE1_SET_DF_SP_Y0(r)     (((r) <<0) & NE1_MASK_DF_SP_Y0)
#define NE1_SET_DF_SP_Y1(r)     (((r) <<16) & NE1_MASK_DF_SP_Y1)
#define NE1_SET_DF_SP_Y2(r)     (((r) <<32) & NE1_MASK_DF_SP_Y2)
#define NE1_SET_DF_SP_Y3(r)     (((r) <<48) & NE1_MASK_DF_SP_Y3)
#define NE1_SET_DF_SP_Y4(r)     (((r) <<0) & NE1_MASK_DF_SP_Y4)
#define NE1_SET_DF_SP_Y5(r)     (((r) <<16) & NE1_MASK_DF_SP_Y5)
#define NE1_SET_DF_SP_Y6(r)     (((r) <<32) & NE1_MASK_DF_SP_Y6)
#define NE1_SET_DF_DS_H0(r)     (((r) <<0) & NE1_MASK_DF_DS_H0)
#define NE1_SET_DF_DS_H1(r)     (((r) <<16) & NE1_MASK_DF_DS_H1)
#define NE1_SET_DF_DS_H2(r)     (((r) <<32) & NE1_MASK_DF_DS_H2)
#define NE1_SET_DF_DS_H3(r)     (((r) <<48) & NE1_MASK_DF_DS_H3)
#define NE1_SET_DF_DS_H4(r)     (((r) <<0) & NE1_MASK_DF_DS_H4)
#define NE1_SET_DF_DS_H5(r)     (((r) <<16) & NE1_MASK_DF_DS_H5)
#define NE1_SET_DF_DS_H6(r)     (((r) <<32) & NE1_MASK_DF_DS_H6)
#define NE1_SET_DF_DS_V0(r)     (((r) <<0) & NE1_MASK_DF_DS_V0)
#define NE1_SET_DF_DS_V2(r)     (((r) <<32) & NE1_MASK_DF_DS_V2)
#define NE1_SET_DF_DS_V4(r)     (((r) <<0) & NE1_MASK_DF_DS_V4)
#define NE1_SET_DF_RC_H0(r)     (((r) <<0) & NE1_MASK_DF_RC_H0)
#define NE1_SET_DF_RC_H1(r)     (((r) <<16) & NE1_MASK_DF_RC_H1)
#define NE1_SET_DF_RC_H2(r)     (((r) <<32) & NE1_MASK_DF_RC_H2)
#define NE1_SET_DF_RC_H3(r)     (((r) <<48) & NE1_MASK_DF_RC_H3)
#define NE1_SET_DF_RC_H4(r)     (((r) <<0) & NE1_MASK_DF_RC_H4)
#define NE1_SET_DF_RC_H5(r)     (((r) <<16) & NE1_MASK_DF_RC_H5)
#define NE1_SET_DF_RC_H6(r)     (((r) <<32) & NE1_MASK_DF_RC_H6)
#define NE1_SET_DF_RC_V0(r)     (((r) <<0) & NE1_MASK_DF_RC_V0)
#define NE1_SET_DF_RC_V2(r)     (((r) <<32) & NE1_MASK_DF_RC_V2)
#define NE1_SET_DF_RC_V4(r)     (((r) <<0) & NE1_MASK_DF_RC_V4)
#define NE1_SET_DF_PT_R00(r)    (((r) <<0) & NE1_MASK_DF_PT_R00)
#define NE1_SET_DF_PT_R10(r)    (((r) <<8) & NE1_MASK_DF_PT_R10)
#define NE1_SET_DF_PT_R01(r)    (((r) <<16) & NE1_MASK_DF_PT_R01)
#define NE1_SET_DF_PT_R11(r)    (((r) <<24) & NE1_MASK_DF_PT_R11)
#define NE1_SET_DF_PT_R02(r)    (((r) <<32) & NE1_MASK_DF_PT_R02)
#define NE1_SET_DF_PT_R12(r)    (((r) <<40) & NE1_MASK_DF_PT_R12)
#define NE1_SET_DF_PT_R03(r)    (((r) <<48) & NE1_MASK_DF_PT_R03)
#define NE1_SET_DF_PT_R13(r)    (((r) <<56) & NE1_MASK_DF_PT_R13)
#define NE1_SET_DF_PT_R04(r)    (((r) <<0) & NE1_MASK_DF_PT_R04)
#define NE1_SET_DF_PT_R14(r)    (((r) <<8) & NE1_MASK_DF_PT_R14)
#define NE1_SET_DF_PT_R05(r)    (((r) <<16) & NE1_MASK_DF_PT_R05)
#define NE1_SET_DF_PT_R15(r)    (((r) <<24) & NE1_MASK_DF_PT_R15)
#define NE1_SET_DF_PT_R06(r)    (((r) <<32) & NE1_MASK_DF_PT_R06)
#define NE1_SET_DF_PT_R16(r)    (((r) <<40) & NE1_MASK_DF_PT_R16)
#define NE1_SET_DF_LT_C0(r)     (((r) <<0) & NE1_MASK_DF_LT_C0)
#define NE1_SET_DF_PT_C0(r)     (((r) <<1) & NE1_MASK_DF_PT_C0)
#define NE1_SET_DF_LT_R0(r)     (((r) <<8) & NE1_MASK_DF_LT_R0)
#define NE1_SET_DF_LT_C1(r)     (((r) <<16) & NE1_MASK_DF_LT_C1)
#define NE1_SET_DF_PT_C1(r)     (((r) <<17) & NE1_MASK_DF_PT_C1)
#define NE1_SET_DF_LT_R1(r)     (((r) <<24) & NE1_MASK_DF_LT_R1)
#define NE1_SET_DF_LT_C2(r)     (((r) <<32) & NE1_MASK_DF_LT_C2)
#define NE1_SET_DF_PT_C2(r)     (((r) <<33) & NE1_MASK_DF_PT_C2)
#define NE1_SET_DF_LT_R2(r)     (((r) <<40) & NE1_MASK_DF_LT_R2)
#define NE1_SET_DF_LT_C3(r)     (((r) <<48) & NE1_MASK_DF_LT_C3)
#define NE1_SET_DF_PT_C3(r)     (((r) <<49) & NE1_MASK_DF_PT_C3)
#define NE1_SET_DF_LT_R3(r)     (((r) <<56) & NE1_MASK_DF_LT_R3)
#define NE1_SET_DF_LT_C4(r)     (((r) <<0) & NE1_MASK_DF_LT_C4)
#define NE1_SET_DF_PT_C4(r)     (((r) <<1) & NE1_MASK_DF_PT_C4)
#define NE1_SET_DF_LT_R4(r)     (((r) <<8) & NE1_MASK_DF_LT_R4)
#define NE1_SET_DF_LT_C5(r)     (((r) <<16) & NE1_MASK_DF_LT_C5)
#define NE1_SET_DF_PT_C5(r)     (((r) <<17) & NE1_MASK_DF_PT_C5)
#define NE1_SET_DF_LT_R5(r)     (((r) <<24) & NE1_MASK_DF_LT_R5)
#define NE1_SET_DF_LT_C6(r)     (((r) <<32) & NE1_MASK_DF_LT_C6)
#define NE1_SET_DF_PT_C6(r)     (((r) <<33) & NE1_MASK_DF_PT_C6)
#define NE1_SET_DF_LT_R6(r)     (((r) <<40) & NE1_MASK_DF_LT_R6)
#define NE1_SET_DF_CK_C0(r)     (((r) <<0) & NE1_MASK_DF_CK_C0)
#define NE1_SET_DF_CK_C1(r)     (((r) <<1) & NE1_MASK_DF_CK_C1)
#define NE1_SET_DF_CK_C2(r)     (((r) <<2) & NE1_MASK_DF_CK_C2)
#define NE1_SET_DF_CK_C3(r)     (((r) <<3) & NE1_MASK_DF_CK_C3)
#define NE1_SET_DF_CK_C4(r)     (((r) <<4) & NE1_MASK_DF_CK_C4)
#define NE1_SET_DF_CK_C5(r)     (((r) <<5) & NE1_MASK_DF_CK_C5)
#define NE1_SET_DF_CK_C6(r)     (((r) <<6) & NE1_MASK_DF_CK_C6)
#define NE1_SET_DF_CK_S0(r)     (((r) <<8) & NE1_MASK_DF_CK_S0)
#define NE1_SET_DF_CK_S1(r)     (((r) <<9) & NE1_MASK_DF_CK_S1)
#define NE1_SET_DF_CK_S2(r)     (((r) <<10) & NE1_MASK_DF_CK_S2)
#define NE1_SET_DF_CK_S3(r)     (((r) <<11) & NE1_MASK_DF_CK_S3)
#define NE1_SET_DF_CK_S4(r)     (((r) <<12) & NE1_MASK_DF_CK_S4)
#define NE1_SET_DF_CK_S5(r)     (((r) <<13) & NE1_MASK_DF_CK_S5)
#define NE1_SET_DF_CK_S6(r)     (((r) <<14) & NE1_MASK_DF_CK_S6)
#define NE1_SET_DF_CK_A0(r)     (((r) <<16) & NE1_MASK_DF_CK_A0)
#define NE1_SET_DF_CK_A1(r)     (((r) <<17) & NE1_MASK_DF_CK_A1)
#define NE1_SET_DF_CK_A2(r)     (((r) <<18) & NE1_MASK_DF_CK_A2)
#define NE1_SET_DF_CK_A3(r)     (((r) <<19) & NE1_MASK_DF_CK_A3)
#define NE1_SET_DF_CK_A4(r)     (((r) <<20) & NE1_MASK_DF_CK_A4)
#define NE1_SET_DF_CK_A5(r)     (((r) <<21) & NE1_MASK_DF_CK_A5)
#define NE1_SET_DF_CK_A6(r)     (((r) <<22) & NE1_MASK_DF_CK_A6)
#define NE1_SET_DF_CK0(r)       (((r) <<0) & NE1_MASK_DF_CK0)
#define NE1_SET_DF_CK1(r)       (((r) <<32) & NE1_MASK_DF_CK1)
#define NE1_SET_DF_CK2(r)       (((r) <<0) & NE1_MASK_DF_CK2)
#define NE1_SET_DF_CK3(r)       (((r) <<32) & NE1_MASK_DF_CK3)
#define NE1_SET_DF_CK4(r)       (((r) <<0) & NE1_MASK_DF_CK4)
#define NE1_SET_DF_CK5(r)       (((r) <<32) & NE1_MASK_DF_CK5)
#define NE1_SET_DF_CK6(r)       (((r) <<0) & NE1_MASK_DF_CK6)
#define NE1_SET_DF_CK_PR_BV0(r) (((r) <<0) & NE1_MASK_DF_CK_PR_BV0)
#define NE1_SET_DF_CK_PR_GY0(r) (((r) <<4) & NE1_MASK_DF_CK_PR_GY0)
#define NE1_SET_DF_CK_PR_RU0(r) (((r) <<8) & NE1_MASK_DF_CK_PR_RU0)
#define NE1_SET_DF_CK_PR_A0(r)  (((r) <<12) & NE1_MASK_DF_CK_PR_A0)
#define NE1_SET_DF_CK_PR_BV1(r) (((r) <<16) & NE1_MASK_DF_CK_PR_BV1)
#define NE1_SET_DF_CK_PR_GY1(r) (((r) <<20) & NE1_MASK_DF_CK_PR_GY1)
#define NE1_SET_DF_CK_PR_RU1(r) (((r) <<24) & NE1_MASK_DF_CK_PR_RU1)
#define NE1_SET_DF_CK_PR_A1(r)  (((r) <<28) & NE1_MASK_DF_CK_PR_A1)
#define NE1_SET_DF_CK_PR_BV2(r) (((r) <<32) & NE1_MASK_DF_CK_PR_BV2)
#define NE1_SET_DF_CK_PR_GY2(r) (((r) <<36) & NE1_MASK_DF_CK_PR_GY2)
#define NE1_SET_DF_CK_PR_RU2(r) (((r) <<40) & NE1_MASK_DF_CK_PR_RU2)
#define NE1_SET_DF_CK_PR_A2(r)  (((r) <<44) & NE1_MASK_DF_CK_PR_A2)
#define NE1_SET_DF_CK_PR_BV3(r) (((r) <<48) & NE1_MASK_DF_CK_PR_BV3)
#define NE1_SET_DF_CK_PR_GY3(r) (((r) <<52) & NE1_MASK_DF_CK_PR_GY3)
#define NE1_SET_DF_CK_PR_RU3(r) (((r) <<56) & NE1_MASK_DF_CK_PR_RU3)
#define NE1_SET_DF_CK_PR_A3(r)  (((r) <<60) & NE1_MASK_DF_CK_PR_A3)
#define NE1_SET_DF_CK_PR_BV4(r) (((r) <<0) & NE1_MASK_DF_CK_PR_BV4)
#define NE1_SET_DF_CK_PR_GY4(r) (((r) <<4) & NE1_MASK_DF_CK_PR_GY4)
#define NE1_SET_DF_CK_PR_RU4(r) (((r) <<8) & NE1_MASK_DF_CK_PR_RU4)
#define NE1_SET_DF_CK_PR_A4(r)  (((r) <<12) & NE1_MASK_DF_CK_PR_A4)
#define NE1_SET_DF_CK_PR_BV5(r) (((r) <<16) & NE1_MASK_DF_CK_PR_BV5)
#define NE1_SET_DF_CK_PR_GY5(r) (((r) <<20) & NE1_MASK_DF_CK_PR_GY5)
#define NE1_SET_DF_CK_PR_RU5(r) (((r) <<24) & NE1_MASK_DF_CK_PR_RU5)
#define NE1_SET_DF_CK_PR_A5(r)  (((r) <<28) & NE1_MASK_DF_CK_PR_A5)
#define NE1_SET_DF_CK_PR_BV6(r) (((r) <<32) & NE1_MASK_DF_CK_PR_BV6)
#define NE1_SET_DF_CK_PR_GY6(r) (((r) <<36) & NE1_MASK_DF_CK_PR_GY6)
#define NE1_SET_DF_CK_PR_RU6(r) (((r) <<40) & NE1_MASK_DF_CK_PR_RU6)
#define NE1_SET_DF_CK_PR_A6(r)  (((r) <<44) & NE1_MASK_DF_CK_PR_A6)
#define NE1_SET_DF_CC_B0(r)     (((r) <<0) & NE1_MASK_DF_CC_B0)
#define NE1_SET_DF_CC_G0(r)     (((r) <<8) & NE1_MASK_DF_CC_G0)
#define NE1_SET_DF_CC_R0(r)     (((r) <<16) & NE1_MASK_DF_CC_R0)
#define NE1_SET_DF_CC_B1(r)     (((r) <<32) & NE1_MASK_DF_CC_B1)
#define NE1_SET_DF_CC_G1(r)     (((r) <<40) & NE1_MASK_DF_CC_G1)
#define NE1_SET_DF_CC_R1(r)     (((r) <<48) & NE1_MASK_DF_CC_R1)
#define NE1_SET_DF_CC_B2(r)     (((r) <<0) & NE1_MASK_DF_CC_B2)
#define NE1_SET_DF_CC_G2(r)     (((r) <<8) & NE1_MASK_DF_CC_G2)
#define NE1_SET_DF_CC_R2(r)     (((r) <<16) & NE1_MASK_DF_CC_R2)
#define NE1_SET_DF_CC_B3(r)     (((r) <<32) & NE1_MASK_DF_CC_B3)
#define NE1_SET_DF_CC_G3(r)     (((r) <<40) & NE1_MASK_DF_CC_G3)
#define NE1_SET_DF_CC_R3(r)     (((r) <<48) & NE1_MASK_DF_CC_R3)
#define NE1_SET_DF_CC_B4(r)     (((r) <<0) & NE1_MASK_DF_CC_B4)
#define NE1_SET_DF_CC_G4(r)     (((r) <<8) & NE1_MASK_DF_CC_G4)
#define NE1_SET_DF_CC_R4(r)     (((r) <<16) & NE1_MASK_DF_CC_R4)
#define NE1_SET_DF_CC_B5(r)     (((r) <<32) & NE1_MASK_DF_CC_B5)
#define NE1_SET_DF_CC_G5(r)     (((r) <<40) & NE1_MASK_DF_CC_G5)
#define NE1_SET_DF_CC_R5(r)     (((r) <<48) & NE1_MASK_DF_CC_R5)
#define NE1_SET_DF_CC_B6(r)     (((r) <<0) & NE1_MASK_DF_CC_B6)
#define NE1_SET_DF_CC_G6(r)     (((r) <<8) & NE1_MASK_DF_CC_G6)
#define NE1_SET_DF_CC_R6(r)     (((r) <<16) & NE1_MASK_DF_CC_R6)
#define NE1_SET_DF_BC_B(r)      (((r) <<0) & NE1_MASK_DF_BC_B)
#define NE1_SET_DF_BC_G(r)      (((r) <<8) & NE1_MASK_DF_BC_G)
#define NE1_SET_DF_BC_R(r)      (((r) <<16) & NE1_MASK_DF_BC_R)
#define NE1_SET_DF_CD_B(r)      (((r) <<0) & NE1_MASK_DF_CD_B)
#define NE1_SET_DF_CD_G(r)      (((r) <<8) & NE1_MASK_DF_CD_G)
#define NE1_SET_DF_CD_R(r)      (((r) <<16) & NE1_MASK_DF_CD_R)
#define NE1_SET_DF_CD_PR_B(r)   (((r) <<32) & NE1_MASK_DF_CD_PR_B)
#define NE1_SET_DF_CD_PR_G(r)   (((r) <<36) & NE1_MASK_DF_CD_PR_G)
#define NE1_SET_DF_CD_PR_R(r)   (((r) <<40) & NE1_MASK_DF_CD_PR_R)
#define NE1_SET_DF_CD_C(r)      (((r) <<63) & NE1_MASK_DF_CD_C)
#define NE1_SET_HC_DS(r)        (((r) <<0) & NE1_MASK_HC_DS)
#define NE1_SET_HC_FS(r)        (((r) <<1) & NE1_MASK_HC_FS)
#define NE1_SET_HC_LC(r)        (((r) <<2) & NE1_MASK_HC_LC)
#define NE1_SET_HC_SP_X(r)      (((r) <<16) & NE1_MASK_HC_SP_X)
#define NE1_SET_HC_SP_Y(r)      (((r) <<32) & NE1_MASK_HC_SP_Y)
#define NE1_SET_MF_WS_M0(r)     (((r) <<0) & NE1_MASK_MF_WS_M0)
#define NE1_SET_MF_SP_M0(r)     (((r) <<1) & NE1_MASK_MF_SP_M0)
#define NE1_SET_MF_DS_M0(r)     (((r) <<2) & NE1_MASK_MF_DS_M0)
#define NE1_SET_DF_SP_M0(r)     (((r) <<3) & NE1_MASK_DF_SP_M0)
#define NE1_SET_DF_DS_M0(r)     (((r) <<4) & NE1_MASK_DF_DS_M0)
#define NE1_SET_DF_RC_M0(r)     (((r) <<5) & NE1_MASK_DF_RC_M0)
#define NE1_SET_MF_WS_M1(r)     (((r) <<8) & NE1_MASK_MF_WS_M1)
#define NE1_SET_MF_SP_M1(r)     (((r) <<9) & NE1_MASK_MF_SP_M1)
#define NE1_SET_MF_DS_M1(r)     (((r) <<10) & NE1_MASK_MF_DS_M1)
#define NE1_SET_DF_SP_M1(r)     (((r) <<11) & NE1_MASK_DF_SP_M1)
#define NE1_SET_DF_DS_M1(r)     (((r) <<12) & NE1_MASK_DF_DS_M1)
#define NE1_SET_DF_RC_M1(r)     (((r) <<13) & NE1_MASK_DF_RC_M1)
#define NE1_SET_MF_WS_M2(r)     (((r) <<16) & NE1_MASK_MF_WS_M2)
#define NE1_SET_MF_SP_M2(r)     (((r) <<17) & NE1_MASK_MF_SP_M2)
#define NE1_SET_MF_DS_M2(r)     (((r) <<18) & NE1_MASK_MF_DS_M2)
#define NE1_SET_DF_SP_M2(r)     (((r) <<19) & NE1_MASK_DF_SP_M2)
#define NE1_SET_DF_DS_M2(r)     (((r) <<20) & NE1_MASK_DF_DS_M2)
#define NE1_SET_DF_RC_M2(r)     (((r) <<21) & NE1_MASK_DF_RC_M2)
#define NE1_SET_MF_WS_M3(r)     (((r) <<24) & NE1_MASK_MF_WS_M3)
#define NE1_SET_MF_SP_M3(r)     (((r) <<25) & NE1_MASK_MF_SP_M3)
#define NE1_SET_MF_DS_M3(r)     (((r) <<26) & NE1_MASK_MF_DS_M3)
#define NE1_SET_DF_SP_M3(r)     (((r) <<27) & NE1_MASK_DF_SP_M3)
#define NE1_SET_DF_DS_M3(r)     (((r) <<28) & NE1_MASK_DF_DS_M3)
#define NE1_SET_DF_RC_M3(r)     (((r) <<29) & NE1_MASK_DF_RC_M3)
#define NE1_SET_MF_WS_M4(r)     (((r) <<32) & NE1_MASK_MF_WS_M4)
#define NE1_SET_MF_SP_M4(r)     (((r) <<33) & NE1_MASK_MF_SP_M4)
#define NE1_SET_MF_DS_M4(r)     (((r) <<34) & NE1_MASK_MF_DS_M4)
#define NE1_SET_DF_SP_M4(r)     (((r) <<35) & NE1_MASK_DF_SP_M4)
#define NE1_SET_DF_DS_M4(r)     (((r) <<36) & NE1_MASK_DF_DS_M4)
#define NE1_SET_DF_RC_M4(r)     (((r) <<37) & NE1_MASK_DF_RC_M4)
#define NE1_SET_MF_WS_M5(r)     (((r) <<40) & NE1_MASK_MF_WS_M5)
#define NE1_SET_MF_SP_M5(r)     (((r) <<41) & NE1_MASK_MF_SP_M5)
#define NE1_SET_MF_DS_M5(r)     (((r) <<42) & NE1_MASK_MF_DS_M5)
#define NE1_SET_DF_SP_M5(r)     (((r) <<43) & NE1_MASK_DF_SP_M5)
#define NE1_SET_DF_DS_M5(r)     (((r) <<44) & NE1_MASK_DF_DS_M5)
#define NE1_SET_DF_RC_M5(r)     (((r) <<45) & NE1_MASK_DF_RC_M5)
#define NE1_SET_MF_WS_M6(r)     (((r) <<48) & NE1_MASK_MF_WS_M6)
#define NE1_SET_MF_SP_M6(r)     (((r) <<49) & NE1_MASK_MF_SP_M6)
#define NE1_SET_MF_DS_M6(r)     (((r) <<50) & NE1_MASK_MF_DS_M6)
#define NE1_SET_DF_SP_M6(r)     (((r) <<51) & NE1_MASK_DF_SP_M6)
#define NE1_SET_DF_DS_M6(r)     (((r) <<52) & NE1_MASK_DF_DS_M6)
#define NE1_SET_DF_RC_M6(r)     (((r) <<53) & NE1_MASK_DF_RC_M6)
#define NE1_SET_HC_SP_M(r)      (((r) <<56) & NE1_MASK_HC_SP_M)
#define NE1_SET_DS_GC_R0(r)     (((r) <<0) & NE1_MASK_DS_GC_R0)
#define NE1_SET_DS_GC_R1(r)     (((r) <<8) & NE1_MASK_DS_GC_R1)
#define NE1_SET_DS_GC_R2(r)     (((r) <<16) & NE1_MASK_DS_GC_R2)
#define NE1_SET_DS_GC_R3(r)     (((r) <<24) & NE1_MASK_DS_GC_R3)
#define NE1_SET_DS_GC_R4(r)     (((r) <<32) & NE1_MASK_DS_GC_R4)
#define NE1_SET_DS_GC_R5(r)     (((r) <<40) & NE1_MASK_DS_GC_R5)
#define NE1_SET_DS_GC_R6(r)     (((r) <<48) & NE1_MASK_DS_GC_R6)
#define NE1_SET_DS_GC_R7(r)     (((r) <<56) & NE1_MASK_DS_GC_R7)
#define NE1_SET_DS_GC_R8(r)     (((r) <<0) & NE1_MASK_DS_GC_R8)
#define NE1_SET_DS_GC_R9(r)     (((r) <<8) & NE1_MASK_DS_GC_R9)
#define NE1_SET_DS_GC_R10(r)    (((r) <<16) & NE1_MASK_DS_GC_R10)
#define NE1_SET_DS_GC_R11(r)    (((r) <<24) & NE1_MASK_DS_GC_R11)
#define NE1_SET_DS_GC_R12(r)    (((r) <<32) & NE1_MASK_DS_GC_R12)
#define NE1_SET_DS_GC_R13(r)    (((r) <<40) & NE1_MASK_DS_GC_R13)
#define NE1_SET_DS_GC_R14(r)    (((r) <<48) & NE1_MASK_DS_GC_R14)
#define NE1_SET_DS_GC_R15(r)    (((r) <<56) & NE1_MASK_DS_GC_R15)
#define NE1_SET_DS_GC_R16(r)    (((r) <<0) & NE1_MASK_DS_GC_R16)
#define NE1_SET_DS_GC_R17(r)    (((r) <<8) & NE1_MASK_DS_GC_R17)
#define NE1_SET_DS_GC_R18(r)    (((r) <<16) & NE1_MASK_DS_GC_R18)
#define NE1_SET_DS_GC_R19(r)    (((r) <<24) & NE1_MASK_DS_GC_R19)
#define NE1_SET_DS_GC_R20(r)    (((r) <<32) & NE1_MASK_DS_GC_R20)
#define NE1_SET_DS_GC_R21(r)    (((r) <<40) & NE1_MASK_DS_GC_R21)
#define NE1_SET_DS_GC_R22(r)    (((r) <<48) & NE1_MASK_DS_GC_R22)
#define NE1_SET_DS_GC_R23(r)    (((r) <<56) & NE1_MASK_DS_GC_R23)
#define NE1_SET_DS_GC_R24(r)    (((r) <<0) & NE1_MASK_DS_GC_R24)
#define NE1_SET_DS_GC_R25(r)    (((r) <<8) & NE1_MASK_DS_GC_R25)
#define NE1_SET_DS_GC_R26(r)    (((r) <<16) & NE1_MASK_DS_GC_R26)
#define NE1_SET_DS_GC_R27(r)    (((r) <<24) & NE1_MASK_DS_GC_R27)
#define NE1_SET_DS_GC_R28(r)    (((r) <<32) & NE1_MASK_DS_GC_R28)
#define NE1_SET_DS_GC_R29(r)    (((r) <<40) & NE1_MASK_DS_GC_R29)
#define NE1_SET_DS_GC_R30(r)    (((r) <<48) & NE1_MASK_DS_GC_R30)
#define NE1_SET_DS_GC_R31(r)    (((r) <<56) & NE1_MASK_DS_GC_R31)
#define NE1_SET_DS_GC_R32(r)    (((r) <<0) & NE1_MASK_DS_GC_R32)
#define NE1_SET_DS_GC_R33(r)    (((r) <<8) & NE1_MASK_DS_GC_R33)
#define NE1_SET_DS_GC_R34(r)    (((r) <<16) & NE1_MASK_DS_GC_R34)
#define NE1_SET_DS_GC_R35(r)    (((r) <<24) & NE1_MASK_DS_GC_R35)
#define NE1_SET_DS_GC_R36(r)    (((r) <<32) & NE1_MASK_DS_GC_R36)
#define NE1_SET_DS_GC_R37(r)    (((r) <<40) & NE1_MASK_DS_GC_R37)
#define NE1_SET_DS_GC_R38(r)    (((r) <<48) & NE1_MASK_DS_GC_R38)
#define NE1_SET_DS_GC_R39(r)    (((r) <<56) & NE1_MASK_DS_GC_R39)
#define NE1_SET_DS_GC_R40(r)    (((r) <<0) & NE1_MASK_DS_GC_R40)
#define NE1_SET_DS_GC_R41(r)    (((r) <<8) & NE1_MASK_DS_GC_R41)
#define NE1_SET_DS_GC_R42(r)    (((r) <<16) & NE1_MASK_DS_GC_R42)
#define NE1_SET_DS_GC_R43(r)    (((r) <<24) & NE1_MASK_DS_GC_R43)
#define NE1_SET_DS_GC_R44(r)    (((r) <<32) & NE1_MASK_DS_GC_R44)
#define NE1_SET_DS_GC_R45(r)    (((r) <<40) & NE1_MASK_DS_GC_R45)
#define NE1_SET_DS_GC_R46(r)    (((r) <<48) & NE1_MASK_DS_GC_R46)
#define NE1_SET_DS_GC_R47(r)    (((r) <<56) & NE1_MASK_DS_GC_R47)
#define NE1_SET_DS_GC_R48(r)    (((r) <<0) & NE1_MASK_DS_GC_R48)
#define NE1_SET_DS_GC_R49(r)    (((r) <<8) & NE1_MASK_DS_GC_R49)
#define NE1_SET_DS_GC_R50(r)    (((r) <<16) & NE1_MASK_DS_GC_R50)
#define NE1_SET_DS_GC_R51(r)    (((r) <<24) & NE1_MASK_DS_GC_R51)
#define NE1_SET_DS_GC_R52(r)    (((r) <<32) & NE1_MASK_DS_GC_R52)
#define NE1_SET_DS_GC_R53(r)    (((r) <<40) & NE1_MASK_DS_GC_R53)
#define NE1_SET_DS_GC_R54(r)    (((r) <<48) & NE1_MASK_DS_GC_R54)
#define NE1_SET_DS_GC_R55(r)    (((r) <<56) & NE1_MASK_DS_GC_R55)
#define NE1_SET_DS_GC_R56(r)    (((r) <<0) & NE1_MASK_DS_GC_R56)
#define NE1_SET_DS_GC_R57(r)    (((r) <<8) & NE1_MASK_DS_GC_R57)
#define NE1_SET_DS_GC_R58(r)    (((r) <<16) & NE1_MASK_DS_GC_R58)
#define NE1_SET_DS_GC_R59(r)    (((r) <<24) & NE1_MASK_DS_GC_R59)
#define NE1_SET_DS_GC_R60(r)    (((r) <<32) & NE1_MASK_DS_GC_R60)
#define NE1_SET_DS_GC_R61(r)    (((r) <<40) & NE1_MASK_DS_GC_R61)
#define NE1_SET_DS_GC_R62(r)    (((r) <<48) & NE1_MASK_DS_GC_R62)
#define NE1_SET_DS_GC_R63(r)    (((r) <<56) & NE1_MASK_DS_GC_R63)
#define NE1_SET_DS_GC_R64(r)    (((r) <<0) & NE1_MASK_DS_GC_R64)
#define NE1_SET_DS_GC_R65(r)    (((r) <<8) & NE1_MASK_DS_GC_R65)
#define NE1_SET_DS_GC_R66(r)    (((r) <<16) & NE1_MASK_DS_GC_R66)
#define NE1_SET_DS_GC_R67(r)    (((r) <<24) & NE1_MASK_DS_GC_R67)
#define NE1_SET_DS_GC_R68(r)    (((r) <<32) & NE1_MASK_DS_GC_R68)
#define NE1_SET_DS_GC_R69(r)    (((r) <<40) & NE1_MASK_DS_GC_R69)
#define NE1_SET_DS_GC_R70(r)    (((r) <<48) & NE1_MASK_DS_GC_R70)
#define NE1_SET_DS_GC_R71(r)    (((r) <<56) & NE1_MASK_DS_GC_R71)
#define NE1_SET_DS_GC_R72(r)    (((r) <<0) & NE1_MASK_DS_GC_R72)
#define NE1_SET_DS_GC_R73(r)    (((r) <<8) & NE1_MASK_DS_GC_R73)
#define NE1_SET_DS_GC_R74(r)    (((r) <<16) & NE1_MASK_DS_GC_R74)
#define NE1_SET_DS_GC_R75(r)    (((r) <<24) & NE1_MASK_DS_GC_R75)
#define NE1_SET_DS_GC_R76(r)    (((r) <<32) & NE1_MASK_DS_GC_R76)
#define NE1_SET_DS_GC_R77(r)    (((r) <<40) & NE1_MASK_DS_GC_R77)
#define NE1_SET_DS_GC_R78(r)    (((r) <<48) & NE1_MASK_DS_GC_R78)
#define NE1_SET_DS_GC_R79(r)    (((r) <<56) & NE1_MASK_DS_GC_R79)
#define NE1_SET_DS_GC_R80(r)    (((r) <<0) & NE1_MASK_DS_GC_R80)
#define NE1_SET_DS_GC_R81(r)    (((r) <<8) & NE1_MASK_DS_GC_R81)
#define NE1_SET_DS_GC_R82(r)    (((r) <<16) & NE1_MASK_DS_GC_R82)
#define NE1_SET_DS_GC_R83(r)    (((r) <<24) & NE1_MASK_DS_GC_R83)
#define NE1_SET_DS_GC_R84(r)    (((r) <<32) & NE1_MASK_DS_GC_R84)
#define NE1_SET_DS_GC_R85(r)    (((r) <<40) & NE1_MASK_DS_GC_R85)
#define NE1_SET_DS_GC_R86(r)    (((r) <<48) & NE1_MASK_DS_GC_R86)
#define NE1_SET_DS_GC_R87(r)    (((r) <<56) & NE1_MASK_DS_GC_R87)
#define NE1_SET_DS_GC_R88(r)    (((r) <<0) & NE1_MASK_DS_GC_R88)
#define NE1_SET_DS_GC_R89(r)    (((r) <<8) & NE1_MASK_DS_GC_R89)
#define NE1_SET_DS_GC_R90(r)    (((r) <<16) & NE1_MASK_DS_GC_R90)
#define NE1_SET_DS_GC_R91(r)    (((r) <<24) & NE1_MASK_DS_GC_R91)
#define NE1_SET_DS_GC_R92(r)    (((r) <<32) & NE1_MASK_DS_GC_R92)
#define NE1_SET_DS_GC_R93(r)    (((r) <<40) & NE1_MASK_DS_GC_R93)
#define NE1_SET_DS_GC_R94(r)    (((r) <<48) & NE1_MASK_DS_GC_R94)
#define NE1_SET_DS_GC_R95(r)    (((r) <<56) & NE1_MASK_DS_GC_R95)
#define NE1_SET_DS_GC_R96(r)    (((r) <<0) & NE1_MASK_DS_GC_R96)
#define NE1_SET_DS_GC_R97(r)    (((r) <<8) & NE1_MASK_DS_GC_R97)
#define NE1_SET_DS_GC_R98(r)    (((r) <<16) & NE1_MASK_DS_GC_R98)
#define NE1_SET_DS_GC_R99(r)    (((r) <<24) & NE1_MASK_DS_GC_R99)
#define NE1_SET_DS_GC_R100(r)   (((r) <<32) & NE1_MASK_DS_GC_R100)
#define NE1_SET_DS_GC_R101(r)   (((r) <<40) & NE1_MASK_DS_GC_R101)
#define NE1_SET_DS_GC_R102(r)   (((r) <<48) & NE1_MASK_DS_GC_R102)
#define NE1_SET_DS_GC_R103(r)   (((r) <<56) & NE1_MASK_DS_GC_R103)
#define NE1_SET_DS_GC_R104(r)   (((r) <<0) & NE1_MASK_DS_GC_R104)
#define NE1_SET_DS_GC_R105(r)   (((r) <<8) & NE1_MASK_DS_GC_R105)
#define NE1_SET_DS_GC_R106(r)   (((r) <<16) & NE1_MASK_DS_GC_R106)
#define NE1_SET_DS_GC_R107(r)   (((r) <<24) & NE1_MASK_DS_GC_R107)
#define NE1_SET_DS_GC_R108(r)   (((r) <<32) & NE1_MASK_DS_GC_R108)
#define NE1_SET_DS_GC_R109(r)   (((r) <<40) & NE1_MASK_DS_GC_R109)
#define NE1_SET_DS_GC_R110(r)   (((r) <<48) & NE1_MASK_DS_GC_R110)
#define NE1_SET_DS_GC_R111(r)   (((r) <<56) & NE1_MASK_DS_GC_R111)
#define NE1_SET_DS_GC_R112(r)   (((r) <<0) & NE1_MASK_DS_GC_R112)
#define NE1_SET_DS_GC_R113(r)   (((r) <<8) & NE1_MASK_DS_GC_R113)
#define NE1_SET_DS_GC_R114(r)   (((r) <<16) & NE1_MASK_DS_GC_R114)
#define NE1_SET_DS_GC_R115(r)   (((r) <<24) & NE1_MASK_DS_GC_R115)
#define NE1_SET_DS_GC_R116(r)   (((r) <<32) & NE1_MASK_DS_GC_R116)
#define NE1_SET_DS_GC_R117(r)   (((r) <<40) & NE1_MASK_DS_GC_R117)
#define NE1_SET_DS_GC_R118(r)   (((r) <<48) & NE1_MASK_DS_GC_R118)
#define NE1_SET_DS_GC_R119(r)   (((r) <<56) & NE1_MASK_DS_GC_R119)
#define NE1_SET_DS_GC_R120(r)   (((r) <<0) & NE1_MASK_DS_GC_R120)
#define NE1_SET_DS_GC_R121(r)   (((r) <<8) & NE1_MASK_DS_GC_R121)
#define NE1_SET_DS_GC_R122(r)   (((r) <<16) & NE1_MASK_DS_GC_R122)
#define NE1_SET_DS_GC_R123(r)   (((r) <<24) & NE1_MASK_DS_GC_R123)
#define NE1_SET_DS_GC_R124(r)   (((r) <<32) & NE1_MASK_DS_GC_R124)
#define NE1_SET_DS_GC_R125(r)   (((r) <<40) & NE1_MASK_DS_GC_R125)
#define NE1_SET_DS_GC_R126(r)   (((r) <<48) & NE1_MASK_DS_GC_R126)
#define NE1_SET_DS_GC_R127(r)   (((r) <<56) & NE1_MASK_DS_GC_R127)
#define NE1_SET_DS_GC_R128(r)   (((r) <<0) & NE1_MASK_DS_GC_R128)
#define NE1_SET_DS_GC_R129(r)   (((r) <<8) & NE1_MASK_DS_GC_R129)
#define NE1_SET_DS_GC_R130(r)   (((r) <<16) & NE1_MASK_DS_GC_R130)
#define NE1_SET_DS_GC_R131(r)   (((r) <<24) & NE1_MASK_DS_GC_R131)
#define NE1_SET_DS_GC_R132(r)   (((r) <<32) & NE1_MASK_DS_GC_R132)
#define NE1_SET_DS_GC_R133(r)   (((r) <<40) & NE1_MASK_DS_GC_R133)
#define NE1_SET_DS_GC_R134(r)   (((r) <<48) & NE1_MASK_DS_GC_R134)
#define NE1_SET_DS_GC_R135(r)   (((r) <<56) & NE1_MASK_DS_GC_R135)
#define NE1_SET_DS_GC_R136(r)   (((r) <<0) & NE1_MASK_DS_GC_R136)
#define NE1_SET_DS_GC_R137(r)   (((r) <<8) & NE1_MASK_DS_GC_R137)
#define NE1_SET_DS_GC_R138(r)   (((r) <<16) & NE1_MASK_DS_GC_R138)
#define NE1_SET_DS_GC_R139(r)   (((r) <<24) & NE1_MASK_DS_GC_R139)
#define NE1_SET_DS_GC_R140(r)   (((r) <<32) & NE1_MASK_DS_GC_R140)
#define NE1_SET_DS_GC_R141(r)   (((r) <<40) & NE1_MASK_DS_GC_R141)
#define NE1_SET_DS_GC_R142(r)   (((r) <<48) & NE1_MASK_DS_GC_R142)
#define NE1_SET_DS_GC_R143(r)   (((r) <<56) & NE1_MASK_DS_GC_R143)
#define NE1_SET_DS_GC_R144(r)   (((r) <<0) & NE1_MASK_DS_GC_R144)
#define NE1_SET_DS_GC_R145(r)   (((r) <<8) & NE1_MASK_DS_GC_R145)
#define NE1_SET_DS_GC_R146(r)   (((r) <<16) & NE1_MASK_DS_GC_R146)
#define NE1_SET_DS_GC_R147(r)   (((r) <<24) & NE1_MASK_DS_GC_R147)
#define NE1_SET_DS_GC_R148(r)   (((r) <<32) & NE1_MASK_DS_GC_R148)
#define NE1_SET_DS_GC_R149(r)   (((r) <<40) & NE1_MASK_DS_GC_R149)
#define NE1_SET_DS_GC_R150(r)   (((r) <<48) & NE1_MASK_DS_GC_R150)
#define NE1_SET_DS_GC_R151(r)   (((r) <<56) & NE1_MASK_DS_GC_R151)
#define NE1_SET_DS_GC_R152(r)   (((r) <<0) & NE1_MASK_DS_GC_R152)
#define NE1_SET_DS_GC_R153(r)   (((r) <<8) & NE1_MASK_DS_GC_R153)
#define NE1_SET_DS_GC_R154(r)   (((r) <<16) & NE1_MASK_DS_GC_R154)
#define NE1_SET_DS_GC_R155(r)   (((r) <<24) & NE1_MASK_DS_GC_R155)
#define NE1_SET_DS_GC_R156(r)   (((r) <<32) & NE1_MASK_DS_GC_R156)
#define NE1_SET_DS_GC_R157(r)   (((r) <<40) & NE1_MASK_DS_GC_R157)
#define NE1_SET_DS_GC_R158(r)   (((r) <<48) & NE1_MASK_DS_GC_R158)
#define NE1_SET_DS_GC_R159(r)   (((r) <<56) & NE1_MASK_DS_GC_R159)
#define NE1_SET_DS_GC_R160(r)   (((r) <<0) & NE1_MASK_DS_GC_R160)
#define NE1_SET_DS_GC_R161(r)   (((r) <<8) & NE1_MASK_DS_GC_R161)
#define NE1_SET_DS_GC_R162(r)   (((r) <<16) & NE1_MASK_DS_GC_R162)
#define NE1_SET_DS_GC_R163(r)   (((r) <<24) & NE1_MASK_DS_GC_R163)
#define NE1_SET_DS_GC_R164(r)   (((r) <<32) & NE1_MASK_DS_GC_R164)
#define NE1_SET_DS_GC_R165(r)   (((r) <<40) & NE1_MASK_DS_GC_R165)
#define NE1_SET_DS_GC_R166(r)   (((r) <<48) & NE1_MASK_DS_GC_R166)
#define NE1_SET_DS_GC_R167(r)   (((r) <<56) & NE1_MASK_DS_GC_R167)
#define NE1_SET_DS_GC_R168(r)   (((r) <<0) & NE1_MASK_DS_GC_R168)
#define NE1_SET_DS_GC_R169(r)   (((r) <<8) & NE1_MASK_DS_GC_R169)
#define NE1_SET_DS_GC_R170(r)   (((r) <<16) & NE1_MASK_DS_GC_R170)
#define NE1_SET_DS_GC_R171(r)   (((r) <<24) & NE1_MASK_DS_GC_R171)
#define NE1_SET_DS_GC_R172(r)   (((r) <<32) & NE1_MASK_DS_GC_R172)
#define NE1_SET_DS_GC_R173(r)   (((r) <<40) & NE1_MASK_DS_GC_R173)
#define NE1_SET_DS_GC_R174(r)   (((r) <<48) & NE1_MASK_DS_GC_R174)
#define NE1_SET_DS_GC_R175(r)   (((r) <<56) & NE1_MASK_DS_GC_R175)
#define NE1_SET_DS_GC_R176(r)   (((r) <<0) & NE1_MASK_DS_GC_R176)
#define NE1_SET_DS_GC_R177(r)   (((r) <<8) & NE1_MASK_DS_GC_R177)
#define NE1_SET_DS_GC_R178(r)   (((r) <<16) & NE1_MASK_DS_GC_R178)
#define NE1_SET_DS_GC_R179(r)   (((r) <<24) & NE1_MASK_DS_GC_R179)
#define NE1_SET_DS_GC_R180(r)   (((r) <<32) & NE1_MASK_DS_GC_R180)
#define NE1_SET_DS_GC_R181(r)   (((r) <<40) & NE1_MASK_DS_GC_R181)
#define NE1_SET_DS_GC_R182(r)   (((r) <<48) & NE1_MASK_DS_GC_R182)
#define NE1_SET_DS_GC_R183(r)   (((r) <<56) & NE1_MASK_DS_GC_R183)
#define NE1_SET_DS_GC_R184(r)   (((r) <<0) & NE1_MASK_DS_GC_R184)
#define NE1_SET_DS_GC_R185(r)   (((r) <<8) & NE1_MASK_DS_GC_R185)
#define NE1_SET_DS_GC_R186(r)   (((r) <<16) & NE1_MASK_DS_GC_R186)
#define NE1_SET_DS_GC_R187(r)   (((r) <<24) & NE1_MASK_DS_GC_R187)
#define NE1_SET_DS_GC_R188(r)   (((r) <<32) & NE1_MASK_DS_GC_R188)
#define NE1_SET_DS_GC_R189(r)   (((r) <<40) & NE1_MASK_DS_GC_R189)
#define NE1_SET_DS_GC_R190(r)   (((r) <<48) & NE1_MASK_DS_GC_R190)
#define NE1_SET_DS_GC_R191(r)   (((r) <<56) & NE1_MASK_DS_GC_R191)
#define NE1_SET_DS_GC_R192(r)   (((r) <<0) & NE1_MASK_DS_GC_R192)
#define NE1_SET_DS_GC_R193(r)   (((r) <<8) & NE1_MASK_DS_GC_R193)
#define NE1_SET_DS_GC_R194(r)   (((r) <<16) & NE1_MASK_DS_GC_R194)
#define NE1_SET_DS_GC_R195(r)   (((r) <<24) & NE1_MASK_DS_GC_R195)
#define NE1_SET_DS_GC_R196(r)   (((r) <<32) & NE1_MASK_DS_GC_R196)
#define NE1_SET_DS_GC_R197(r)   (((r) <<40) & NE1_MASK_DS_GC_R197)
#define NE1_SET_DS_GC_R198(r)   (((r) <<48) & NE1_MASK_DS_GC_R198)
#define NE1_SET_DS_GC_R199(r)   (((r) <<56) & NE1_MASK_DS_GC_R199)
#define NE1_SET_DS_GC_R200(r)   (((r) <<0) & NE1_MASK_DS_GC_R200)
#define NE1_SET_DS_GC_R201(r)   (((r) <<8) & NE1_MASK_DS_GC_R201)
#define NE1_SET_DS_GC_R202(r)   (((r) <<16) & NE1_MASK_DS_GC_R202)
#define NE1_SET_DS_GC_R203(r)   (((r) <<24) & NE1_MASK_DS_GC_R203)
#define NE1_SET_DS_GC_R204(r)   (((r) <<32) & NE1_MASK_DS_GC_R204)
#define NE1_SET_DS_GC_R205(r)   (((r) <<40) & NE1_MASK_DS_GC_R205)
#define NE1_SET_DS_GC_R206(r)   (((r) <<48) & NE1_MASK_DS_GC_R206)
#define NE1_SET_DS_GC_R207(r)   (((r) <<56) & NE1_MASK_DS_GC_R207)
#define NE1_SET_DS_GC_R208(r)   (((r) <<0) & NE1_MASK_DS_GC_R208)
#define NE1_SET_DS_GC_R209(r)   (((r) <<8) & NE1_MASK_DS_GC_R209)
#define NE1_SET_DS_GC_R210(r)   (((r) <<16) & NE1_MASK_DS_GC_R210)
#define NE1_SET_DS_GC_R211(r)   (((r) <<24) & NE1_MASK_DS_GC_R211)
#define NE1_SET_DS_GC_R212(r)   (((r) <<32) & NE1_MASK_DS_GC_R212)
#define NE1_SET_DS_GC_R213(r)   (((r) <<40) & NE1_MASK_DS_GC_R213)
#define NE1_SET_DS_GC_R214(r)   (((r) <<48) & NE1_MASK_DS_GC_R214)
#define NE1_SET_DS_GC_R215(r)   (((r) <<56) & NE1_MASK_DS_GC_R215)
#define NE1_SET_DS_GC_R216(r)   (((r) <<0) & NE1_MASK_DS_GC_R216)
#define NE1_SET_DS_GC_R217(r)   (((r) <<8) & NE1_MASK_DS_GC_R217)
#define NE1_SET_DS_GC_R218(r)   (((r) <<16) & NE1_MASK_DS_GC_R218)
#define NE1_SET_DS_GC_R219(r)   (((r) <<24) & NE1_MASK_DS_GC_R219)
#define NE1_SET_DS_GC_R220(r)   (((r) <<32) & NE1_MASK_DS_GC_R220)
#define NE1_SET_DS_GC_R221(r)   (((r) <<40) & NE1_MASK_DS_GC_R221)
#define NE1_SET_DS_GC_R222(r)   (((r) <<48) & NE1_MASK_DS_GC_R222)
#define NE1_SET_DS_GC_R223(r)   (((r) <<56) & NE1_MASK_DS_GC_R223)
#define NE1_SET_DS_GC_R224(r)   (((r) <<0) & NE1_MASK_DS_GC_R224)
#define NE1_SET_DS_GC_R225(r)   (((r) <<8) & NE1_MASK_DS_GC_R225)
#define NE1_SET_DS_GC_R226(r)   (((r) <<16) & NE1_MASK_DS_GC_R226)
#define NE1_SET_DS_GC_R227(r)   (((r) <<24) & NE1_MASK_DS_GC_R227)
#define NE1_SET_DS_GC_R228(r)   (((r) <<32) & NE1_MASK_DS_GC_R228)
#define NE1_SET_DS_GC_R229(r)   (((r) <<40) & NE1_MASK_DS_GC_R229)
#define NE1_SET_DS_GC_R230(r)   (((r) <<48) & NE1_MASK_DS_GC_R230)
#define NE1_SET_DS_GC_R231(r)   (((r) <<56) & NE1_MASK_DS_GC_R231)
#define NE1_SET_DS_GC_R232(r)   (((r) <<0) & NE1_MASK_DS_GC_R232)
#define NE1_SET_DS_GC_R233(r)   (((r) <<8) & NE1_MASK_DS_GC_R233)
#define NE1_SET_DS_GC_R234(r)   (((r) <<16) & NE1_MASK_DS_GC_R234)
#define NE1_SET_DS_GC_R235(r)   (((r) <<24) & NE1_MASK_DS_GC_R235)
#define NE1_SET_DS_GC_R236(r)   (((r) <<32) & NE1_MASK_DS_GC_R236)
#define NE1_SET_DS_GC_R237(r)   (((r) <<40) & NE1_MASK_DS_GC_R237)
#define NE1_SET_DS_GC_R238(r)   (((r) <<48) & NE1_MASK_DS_GC_R238)
#define NE1_SET_DS_GC_R239(r)   (((r) <<56) & NE1_MASK_DS_GC_R239)
#define NE1_SET_DS_GC_R240(r)   (((r) <<0) & NE1_MASK_DS_GC_R240)
#define NE1_SET_DS_GC_R241(r)   (((r) <<8) & NE1_MASK_DS_GC_R241)
#define NE1_SET_DS_GC_R242(r)   (((r) <<16) & NE1_MASK_DS_GC_R242)
#define NE1_SET_DS_GC_R243(r)   (((r) <<24) & NE1_MASK_DS_GC_R243)
#define NE1_SET_DS_GC_R244(r)   (((r) <<32) & NE1_MASK_DS_GC_R244)
#define NE1_SET_DS_GC_R245(r)   (((r) <<40) & NE1_MASK_DS_GC_R245)
#define NE1_SET_DS_GC_R246(r)   (((r) <<48) & NE1_MASK_DS_GC_R246)
#define NE1_SET_DS_GC_R247(r)   (((r) <<56) & NE1_MASK_DS_GC_R247)
#define NE1_SET_DS_GC_R248(r)   (((r) <<0) & NE1_MASK_DS_GC_R248)
#define NE1_SET_DS_GC_R249(r)   (((r) <<8) & NE1_MASK_DS_GC_R249)
#define NE1_SET_DS_GC_R250(r)   (((r) <<16) & NE1_MASK_DS_GC_R250)
#define NE1_SET_DS_GC_R251(r)   (((r) <<24) & NE1_MASK_DS_GC_R251)
#define NE1_SET_DS_GC_R252(r)   (((r) <<32) & NE1_MASK_DS_GC_R252)
#define NE1_SET_DS_GC_R253(r)   (((r) <<40) & NE1_MASK_DS_GC_R253)
#define NE1_SET_DS_GC_R254(r)   (((r) <<48) & NE1_MASK_DS_GC_R254)
#define NE1_SET_DS_GC_R255(r)   (((r) <<56) & NE1_MASK_DS_GC_R255)
#define NE1_SET_DS_GC_G0(r)     (((r) <<0) & NE1_MASK_DS_GC_G0)
#define NE1_SET_DS_GC_G1(r)     (((r) <<8) & NE1_MASK_DS_GC_G1)
#define NE1_SET_DS_GC_G2(r)     (((r) <<16) & NE1_MASK_DS_GC_G2)
#define NE1_SET_DS_GC_G3(r)     (((r) <<24) & NE1_MASK_DS_GC_G3)
#define NE1_SET_DS_GC_G4(r)     (((r) <<32) & NE1_MASK_DS_GC_G4)
#define NE1_SET_DS_GC_G5(r)     (((r) <<40) & NE1_MASK_DS_GC_G5)
#define NE1_SET_DS_GC_G6(r)     (((r) <<48) & NE1_MASK_DS_GC_G6)
#define NE1_SET_DS_GC_G7(r)     (((r) <<56) & NE1_MASK_DS_GC_G7)
#define NE1_SET_DS_GC_G8(r)     (((r) <<0) & NE1_MASK_DS_GC_G8)
#define NE1_SET_DS_GC_G9(r)     (((r) <<8) & NE1_MASK_DS_GC_G9)
#define NE1_SET_DS_GC_G10(r)    (((r) <<16) & NE1_MASK_DS_GC_G10)
#define NE1_SET_DS_GC_G11(r)    (((r) <<24) & NE1_MASK_DS_GC_G11)
#define NE1_SET_DS_GC_G12(r)    (((r) <<32) & NE1_MASK_DS_GC_G12)
#define NE1_SET_DS_GC_G13(r)    (((r) <<40) & NE1_MASK_DS_GC_G13)
#define NE1_SET_DS_GC_G14(r)    (((r) <<48) & NE1_MASK_DS_GC_G14)
#define NE1_SET_DS_GC_G15(r)    (((r) <<56) & NE1_MASK_DS_GC_G15)
#define NE1_SET_DS_GC_G16(r)    (((r) <<0) & NE1_MASK_DS_GC_G16)
#define NE1_SET_DS_GC_G17(r)    (((r) <<8) & NE1_MASK_DS_GC_G17)
#define NE1_SET_DS_GC_G18(r)    (((r) <<16) & NE1_MASK_DS_GC_G18)
#define NE1_SET_DS_GC_G19(r)    (((r) <<24) & NE1_MASK_DS_GC_G19)
#define NE1_SET_DS_GC_G20(r)    (((r) <<32) & NE1_MASK_DS_GC_G20)
#define NE1_SET_DS_GC_G21(r)    (((r) <<40) & NE1_MASK_DS_GC_G21)
#define NE1_SET_DS_GC_G22(r)    (((r) <<48) & NE1_MASK_DS_GC_G22)
#define NE1_SET_DS_GC_G23(r)    (((r) <<56) & NE1_MASK_DS_GC_G23)
#define NE1_SET_DS_GC_G24(r)    (((r) <<0) & NE1_MASK_DS_GC_G24)
#define NE1_SET_DS_GC_G25(r)    (((r) <<8) & NE1_MASK_DS_GC_G25)
#define NE1_SET_DS_GC_G26(r)    (((r) <<16) & NE1_MASK_DS_GC_G26)
#define NE1_SET_DS_GC_G27(r)    (((r) <<24) & NE1_MASK_DS_GC_G27)
#define NE1_SET_DS_GC_G28(r)    (((r) <<32) & NE1_MASK_DS_GC_G28)
#define NE1_SET_DS_GC_G29(r)    (((r) <<40) & NE1_MASK_DS_GC_G29)
#define NE1_SET_DS_GC_G30(r)    (((r) <<48) & NE1_MASK_DS_GC_G30)
#define NE1_SET_DS_GC_G31(r)    (((r) <<56) & NE1_MASK_DS_GC_G31)
#define NE1_SET_DS_GC_G32(r)    (((r) <<0) & NE1_MASK_DS_GC_G32)
#define NE1_SET_DS_GC_G33(r)    (((r) <<8) & NE1_MASK_DS_GC_G33)
#define NE1_SET_DS_GC_G34(r)    (((r) <<16) & NE1_MASK_DS_GC_G34)
#define NE1_SET_DS_GC_G35(r)    (((r) <<24) & NE1_MASK_DS_GC_G35)
#define NE1_SET_DS_GC_G36(r)    (((r) <<32) & NE1_MASK_DS_GC_G36)
#define NE1_SET_DS_GC_G37(r)    (((r) <<40) & NE1_MASK_DS_GC_G37)
#define NE1_SET_DS_GC_G38(r)    (((r) <<48) & NE1_MASK_DS_GC_G38)
#define NE1_SET_DS_GC_G39(r)    (((r) <<56) & NE1_MASK_DS_GC_G39)
#define NE1_SET_DS_GC_G40(r)    (((r) <<0) & NE1_MASK_DS_GC_G40)
#define NE1_SET_DS_GC_G41(r)    (((r) <<8) & NE1_MASK_DS_GC_G41)
#define NE1_SET_DS_GC_G42(r)    (((r) <<16) & NE1_MASK_DS_GC_G42)
#define NE1_SET_DS_GC_G43(r)    (((r) <<24) & NE1_MASK_DS_GC_G43)
#define NE1_SET_DS_GC_G44(r)    (((r) <<32) & NE1_MASK_DS_GC_G44)
#define NE1_SET_DS_GC_G45(r)    (((r) <<40) & NE1_MASK_DS_GC_G45)
#define NE1_SET_DS_GC_G46(r)    (((r) <<48) & NE1_MASK_DS_GC_G46)
#define NE1_SET_DS_GC_G47(r)    (((r) <<56) & NE1_MASK_DS_GC_G47)
#define NE1_SET_DS_GC_G48(r)    (((r) <<0) & NE1_MASK_DS_GC_G48)
#define NE1_SET_DS_GC_G49(r)    (((r) <<8) & NE1_MASK_DS_GC_G49)
#define NE1_SET_DS_GC_G50(r)    (((r) <<16) & NE1_MASK_DS_GC_G50)
#define NE1_SET_DS_GC_G51(r)    (((r) <<24) & NE1_MASK_DS_GC_G51)
#define NE1_SET_DS_GC_G52(r)    (((r) <<32) & NE1_MASK_DS_GC_G52)
#define NE1_SET_DS_GC_G53(r)    (((r) <<40) & NE1_MASK_DS_GC_G53)
#define NE1_SET_DS_GC_G54(r)    (((r) <<48) & NE1_MASK_DS_GC_G54)
#define NE1_SET_DS_GC_G55(r)    (((r) <<56) & NE1_MASK_DS_GC_G55)
#define NE1_SET_DS_GC_G56(r)    (((r) <<0) & NE1_MASK_DS_GC_G56)
#define NE1_SET_DS_GC_G57(r)    (((r) <<8) & NE1_MASK_DS_GC_G57)
#define NE1_SET_DS_GC_G58(r)    (((r) <<16) & NE1_MASK_DS_GC_G58)
#define NE1_SET_DS_GC_G59(r)    (((r) <<24) & NE1_MASK_DS_GC_G59)
#define NE1_SET_DS_GC_G60(r)    (((r) <<32) & NE1_MASK_DS_GC_G60)
#define NE1_SET_DS_GC_G61(r)    (((r) <<40) & NE1_MASK_DS_GC_G61)
#define NE1_SET_DS_GC_G62(r)    (((r) <<48) & NE1_MASK_DS_GC_G62)
#define NE1_SET_DS_GC_G63(r)    (((r) <<56) & NE1_MASK_DS_GC_G63)
#define NE1_SET_DS_GC_G64(r)    (((r) <<0) & NE1_MASK_DS_GC_G64)
#define NE1_SET_DS_GC_G65(r)    (((r) <<8) & NE1_MASK_DS_GC_G65)
#define NE1_SET_DS_GC_G66(r)    (((r) <<16) & NE1_MASK_DS_GC_G66)
#define NE1_SET_DS_GC_G67(r)    (((r) <<24) & NE1_MASK_DS_GC_G67)
#define NE1_SET_DS_GC_G68(r)    (((r) <<32) & NE1_MASK_DS_GC_G68)
#define NE1_SET_DS_GC_G69(r)    (((r) <<40) & NE1_MASK_DS_GC_G69)
#define NE1_SET_DS_GC_G70(r)    (((r) <<48) & NE1_MASK_DS_GC_G70)
#define NE1_SET_DS_GC_G71(r)    (((r) <<56) & NE1_MASK_DS_GC_G71)
#define NE1_SET_DS_GC_G72(r)    (((r) <<0) & NE1_MASK_DS_GC_G72)
#define NE1_SET_DS_GC_G73(r)    (((r) <<8) & NE1_MASK_DS_GC_G73)
#define NE1_SET_DS_GC_G74(r)    (((r) <<16) & NE1_MASK_DS_GC_G74)
#define NE1_SET_DS_GC_G75(r)    (((r) <<24) & NE1_MASK_DS_GC_G75)
#define NE1_SET_DS_GC_G76(r)    (((r) <<32) & NE1_MASK_DS_GC_G76)
#define NE1_SET_DS_GC_G77(r)    (((r) <<40) & NE1_MASK_DS_GC_G77)
#define NE1_SET_DS_GC_G78(r)    (((r) <<48) & NE1_MASK_DS_GC_G78)
#define NE1_SET_DS_GC_G79(r)    (((r) <<56) & NE1_MASK_DS_GC_G79)
#define NE1_SET_DS_GC_G80(r)    (((r) <<0) & NE1_MASK_DS_GC_G80)
#define NE1_SET_DS_GC_G81(r)    (((r) <<8) & NE1_MASK_DS_GC_G81)
#define NE1_SET_DS_GC_G82(r)    (((r) <<16) & NE1_MASK_DS_GC_G82)
#define NE1_SET_DS_GC_G83(r)    (((r) <<24) & NE1_MASK_DS_GC_G83)
#define NE1_SET_DS_GC_G84(r)    (((r) <<32) & NE1_MASK_DS_GC_G84)
#define NE1_SET_DS_GC_G85(r)    (((r) <<40) & NE1_MASK_DS_GC_G85)
#define NE1_SET_DS_GC_G86(r)    (((r) <<48) & NE1_MASK_DS_GC_G86)
#define NE1_SET_DS_GC_G87(r)    (((r) <<56) & NE1_MASK_DS_GC_G87)
#define NE1_SET_DS_GC_G88(r)    (((r) <<0) & NE1_MASK_DS_GC_G88)
#define NE1_SET_DS_GC_G89(r)    (((r) <<8) & NE1_MASK_DS_GC_G89)
#define NE1_SET_DS_GC_G90(r)    (((r) <<16) & NE1_MASK_DS_GC_G90)
#define NE1_SET_DS_GC_G91(r)    (((r) <<24) & NE1_MASK_DS_GC_G91)
#define NE1_SET_DS_GC_G92(r)    (((r) <<32) & NE1_MASK_DS_GC_G92)
#define NE1_SET_DS_GC_G93(r)    (((r) <<40) & NE1_MASK_DS_GC_G93)
#define NE1_SET_DS_GC_G94(r)    (((r) <<48) & NE1_MASK_DS_GC_G94)
#define NE1_SET_DS_GC_G95(r)    (((r) <<56) & NE1_MASK_DS_GC_G95)
#define NE1_SET_DS_GC_G96(r)    (((r) <<0) & NE1_MASK_DS_GC_G96)
#define NE1_SET_DS_GC_G97(r)    (((r) <<8) & NE1_MASK_DS_GC_G97)
#define NE1_SET_DS_GC_G98(r)    (((r) <<16) & NE1_MASK_DS_GC_G98)
#define NE1_SET_DS_GC_G99(r)    (((r) <<24) & NE1_MASK_DS_GC_G99)
#define NE1_SET_DS_GC_G100(r)   (((r) <<32) & NE1_MASK_DS_GC_G100)
#define NE1_SET_DS_GC_G101(r)   (((r) <<40) & NE1_MASK_DS_GC_G101)
#define NE1_SET_DS_GC_G102(r)   (((r) <<48) & NE1_MASK_DS_GC_G102)
#define NE1_SET_DS_GC_G103(r)   (((r) <<56) & NE1_MASK_DS_GC_G103)
#define NE1_SET_DS_GC_G104(r)   (((r) <<0) & NE1_MASK_DS_GC_G104)
#define NE1_SET_DS_GC_G105(r)   (((r) <<8) & NE1_MASK_DS_GC_G105)
#define NE1_SET_DS_GC_G106(r)   (((r) <<16) & NE1_MASK_DS_GC_G106)
#define NE1_SET_DS_GC_G107(r)   (((r) <<24) & NE1_MASK_DS_GC_G107)
#define NE1_SET_DS_GC_G108(r)   (((r) <<32) & NE1_MASK_DS_GC_G108)
#define NE1_SET_DS_GC_G109(r)   (((r) <<40) & NE1_MASK_DS_GC_G109)
#define NE1_SET_DS_GC_G110(r)   (((r) <<48) & NE1_MASK_DS_GC_G110)
#define NE1_SET_DS_GC_G111(r)   (((r) <<56) & NE1_MASK_DS_GC_G111)
#define NE1_SET_DS_GC_G112(r)   (((r) <<0) & NE1_MASK_DS_GC_G112)
#define NE1_SET_DS_GC_G113(r)   (((r) <<8) & NE1_MASK_DS_GC_G113)
#define NE1_SET_DS_GC_G114(r)   (((r) <<16) & NE1_MASK_DS_GC_G114)
#define NE1_SET_DS_GC_G115(r)   (((r) <<24) & NE1_MASK_DS_GC_G115)
#define NE1_SET_DS_GC_G116(r)   (((r) <<32) & NE1_MASK_DS_GC_G116)
#define NE1_SET_DS_GC_G117(r)   (((r) <<40) & NE1_MASK_DS_GC_G117)
#define NE1_SET_DS_GC_G118(r)   (((r) <<48) & NE1_MASK_DS_GC_G118)
#define NE1_SET_DS_GC_G119(r)   (((r) <<56) & NE1_MASK_DS_GC_G119)
#define NE1_SET_DS_GC_G120(r)   (((r) <<0) & NE1_MASK_DS_GC_G120)
#define NE1_SET_DS_GC_G121(r)   (((r) <<8) & NE1_MASK_DS_GC_G121)
#define NE1_SET_DS_GC_G122(r)   (((r) <<16) & NE1_MASK_DS_GC_G122)
#define NE1_SET_DS_GC_G123(r)   (((r) <<24) & NE1_MASK_DS_GC_G123)
#define NE1_SET_DS_GC_G124(r)   (((r) <<32) & NE1_MASK_DS_GC_G124)
#define NE1_SET_DS_GC_G125(r)   (((r) <<40) & NE1_MASK_DS_GC_G125)
#define NE1_SET_DS_GC_G126(r)   (((r) <<48) & NE1_MASK_DS_GC_G126)
#define NE1_SET_DS_GC_G127(r)   (((r) <<56) & NE1_MASK_DS_GC_G127)
#define NE1_SET_DS_GC_G128(r)   (((r) <<0) & NE1_MASK_DS_GC_G128)
#define NE1_SET_DS_GC_G129(r)   (((r) <<8) & NE1_MASK_DS_GC_G129)
#define NE1_SET_DS_GC_G130(r)   (((r) <<16) & NE1_MASK_DS_GC_G130)
#define NE1_SET_DS_GC_G131(r)   (((r) <<24) & NE1_MASK_DS_GC_G131)
#define NE1_SET_DS_GC_G132(r)   (((r) <<32) & NE1_MASK_DS_GC_G132)
#define NE1_SET_DS_GC_G133(r)   (((r) <<40) & NE1_MASK_DS_GC_G133)
#define NE1_SET_DS_GC_G134(r)   (((r) <<48) & NE1_MASK_DS_GC_G134)
#define NE1_SET_DS_GC_G135(r)   (((r) <<56) & NE1_MASK_DS_GC_G135)
#define NE1_SET_DS_GC_G136(r)   (((r) <<0) & NE1_MASK_DS_GC_G136)
#define NE1_SET_DS_GC_G137(r)   (((r) <<8) & NE1_MASK_DS_GC_G137)
#define NE1_SET_DS_GC_G138(r)   (((r) <<16) & NE1_MASK_DS_GC_G138)
#define NE1_SET_DS_GC_G139(r)   (((r) <<24) & NE1_MASK_DS_GC_G139)
#define NE1_SET_DS_GC_G140(r)   (((r) <<32) & NE1_MASK_DS_GC_G140)
#define NE1_SET_DS_GC_G141(r)   (((r) <<40) & NE1_MASK_DS_GC_G141)
#define NE1_SET_DS_GC_G142(r)   (((r) <<48) & NE1_MASK_DS_GC_G142)
#define NE1_SET_DS_GC_G143(r)   (((r) <<56) & NE1_MASK_DS_GC_G143)
#define NE1_SET_DS_GC_G144(r)   (((r) <<0) & NE1_MASK_DS_GC_G144)
#define NE1_SET_DS_GC_G145(r)   (((r) <<8) & NE1_MASK_DS_GC_G145)
#define NE1_SET_DS_GC_G146(r)   (((r) <<16) & NE1_MASK_DS_GC_G146)
#define NE1_SET_DS_GC_G147(r)   (((r) <<24) & NE1_MASK_DS_GC_G147)
#define NE1_SET_DS_GC_G148(r)   (((r) <<32) & NE1_MASK_DS_GC_G148)
#define NE1_SET_DS_GC_G149(r)   (((r) <<40) & NE1_MASK_DS_GC_G149)
#define NE1_SET_DS_GC_G150(r)   (((r) <<48) & NE1_MASK_DS_GC_G150)
#define NE1_SET_DS_GC_G151(r)   (((r) <<56) & NE1_MASK_DS_GC_G151)
#define NE1_SET_DS_GC_G152(r)   (((r) <<0) & NE1_MASK_DS_GC_G152)
#define NE1_SET_DS_GC_G153(r)   (((r) <<8) & NE1_MASK_DS_GC_G153)
#define NE1_SET_DS_GC_G154(r)   (((r) <<16) & NE1_MASK_DS_GC_G154)
#define NE1_SET_DS_GC_G155(r)   (((r) <<24) & NE1_MASK_DS_GC_G155)
#define NE1_SET_DS_GC_G156(r)   (((r) <<32) & NE1_MASK_DS_GC_G156)
#define NE1_SET_DS_GC_G157(r)   (((r) <<40) & NE1_MASK_DS_GC_G157)
#define NE1_SET_DS_GC_G158(r)   (((r) <<48) & NE1_MASK_DS_GC_G158)
#define NE1_SET_DS_GC_G159(r)   (((r) <<56) & NE1_MASK_DS_GC_G159)
#define NE1_SET_DS_GC_G160(r)   (((r) <<0) & NE1_MASK_DS_GC_G160)
#define NE1_SET_DS_GC_G161(r)   (((r) <<8) & NE1_MASK_DS_GC_G161)
#define NE1_SET_DS_GC_G162(r)   (((r) <<16) & NE1_MASK_DS_GC_G162)
#define NE1_SET_DS_GC_G163(r)   (((r) <<24) & NE1_MASK_DS_GC_G163)
#define NE1_SET_DS_GC_G164(r)   (((r) <<32) & NE1_MASK_DS_GC_G164)
#define NE1_SET_DS_GC_G165(r)   (((r) <<40) & NE1_MASK_DS_GC_G165)
#define NE1_SET_DS_GC_G166(r)   (((r) <<48) & NE1_MASK_DS_GC_G166)
#define NE1_SET_DS_GC_G167(r)   (((r) <<56) & NE1_MASK_DS_GC_G167)
#define NE1_SET_DS_GC_G168(r)   (((r) <<0) & NE1_MASK_DS_GC_G168)
#define NE1_SET_DS_GC_G169(r)   (((r) <<8) & NE1_MASK_DS_GC_G169)
#define NE1_SET_DS_GC_G170(r)   (((r) <<16) & NE1_MASK_DS_GC_G170)
#define NE1_SET_DS_GC_G171(r)   (((r) <<24) & NE1_MASK_DS_GC_G171)
#define NE1_SET_DS_GC_G172(r)   (((r) <<32) & NE1_MASK_DS_GC_G172)
#define NE1_SET_DS_GC_G173(r)   (((r) <<40) & NE1_MASK_DS_GC_G173)
#define NE1_SET_DS_GC_G174(r)   (((r) <<48) & NE1_MASK_DS_GC_G174)
#define NE1_SET_DS_GC_G175(r)   (((r) <<56) & NE1_MASK_DS_GC_G175)
#define NE1_SET_DS_GC_G176(r)   (((r) <<0) & NE1_MASK_DS_GC_G176)
#define NE1_SET_DS_GC_G177(r)   (((r) <<8) & NE1_MASK_DS_GC_G177)
#define NE1_SET_DS_GC_G178(r)   (((r) <<16) & NE1_MASK_DS_GC_G178)
#define NE1_SET_DS_GC_G179(r)   (((r) <<24) & NE1_MASK_DS_GC_G179)
#define NE1_SET_DS_GC_G180(r)   (((r) <<32) & NE1_MASK_DS_GC_G180)
#define NE1_SET_DS_GC_G181(r)   (((r) <<40) & NE1_MASK_DS_GC_G181)
#define NE1_SET_DS_GC_G182(r)   (((r) <<48) & NE1_MASK_DS_GC_G182)
#define NE1_SET_DS_GC_G183(r)   (((r) <<56) & NE1_MASK_DS_GC_G183)
#define NE1_SET_DS_GC_G184(r)   (((r) <<0) & NE1_MASK_DS_GC_G184)
#define NE1_SET_DS_GC_G185(r)   (((r) <<8) & NE1_MASK_DS_GC_G185)
#define NE1_SET_DS_GC_G186(r)   (((r) <<16) & NE1_MASK_DS_GC_G186)
#define NE1_SET_DS_GC_G187(r)   (((r) <<24) & NE1_MASK_DS_GC_G187)
#define NE1_SET_DS_GC_G188(r)   (((r) <<32) & NE1_MASK_DS_GC_G188)
#define NE1_SET_DS_GC_G189(r)   (((r) <<40) & NE1_MASK_DS_GC_G189)
#define NE1_SET_DS_GC_G190(r)   (((r) <<48) & NE1_MASK_DS_GC_G190)
#define NE1_SET_DS_GC_G191(r)   (((r) <<56) & NE1_MASK_DS_GC_G191)
#define NE1_SET_DS_GC_G192(r)   (((r) <<0) & NE1_MASK_DS_GC_G192)
#define NE1_SET_DS_GC_G193(r)   (((r) <<8) & NE1_MASK_DS_GC_G193)
#define NE1_SET_DS_GC_G194(r)   (((r) <<16) & NE1_MASK_DS_GC_G194)
#define NE1_SET_DS_GC_G195(r)   (((r) <<24) & NE1_MASK_DS_GC_G195)
#define NE1_SET_DS_GC_G196(r)   (((r) <<32) & NE1_MASK_DS_GC_G196)
#define NE1_SET_DS_GC_G197(r)   (((r) <<40) & NE1_MASK_DS_GC_G197)
#define NE1_SET_DS_GC_G198(r)   (((r) <<48) & NE1_MASK_DS_GC_G198)
#define NE1_SET_DS_GC_G199(r)   (((r) <<56) & NE1_MASK_DS_GC_G199)
#define NE1_SET_DS_GC_G200(r)   (((r) <<0) & NE1_MASK_DS_GC_G200)
#define NE1_SET_DS_GC_G201(r)   (((r) <<8) & NE1_MASK_DS_GC_G201)
#define NE1_SET_DS_GC_G202(r)   (((r) <<16) & NE1_MASK_DS_GC_G202)
#define NE1_SET_DS_GC_G203(r)   (((r) <<24) & NE1_MASK_DS_GC_G203)
#define NE1_SET_DS_GC_G204(r)   (((r) <<32) & NE1_MASK_DS_GC_G204)
#define NE1_SET_DS_GC_G205(r)   (((r) <<40) & NE1_MASK_DS_GC_G205)
#define NE1_SET_DS_GC_G206(r)   (((r) <<48) & NE1_MASK_DS_GC_G206)
#define NE1_SET_DS_GC_G207(r)   (((r) <<56) & NE1_MASK_DS_GC_G207)
#define NE1_SET_DS_GC_G208(r)   (((r) <<0) & NE1_MASK_DS_GC_G208)
#define NE1_SET_DS_GC_G209(r)   (((r) <<8) & NE1_MASK_DS_GC_G209)
#define NE1_SET_DS_GC_G210(r)   (((r) <<16) & NE1_MASK_DS_GC_G210)
#define NE1_SET_DS_GC_G211(r)   (((r) <<24) & NE1_MASK_DS_GC_G211)
#define NE1_SET_DS_GC_G212(r)   (((r) <<32) & NE1_MASK_DS_GC_G212)
#define NE1_SET_DS_GC_G213(r)   (((r) <<40) & NE1_MASK_DS_GC_G213)
#define NE1_SET_DS_GC_G214(r)   (((r) <<48) & NE1_MASK_DS_GC_G214)
#define NE1_SET_DS_GC_G215(r)   (((r) <<56) & NE1_MASK_DS_GC_G215)
#define NE1_SET_DS_GC_G216(r)   (((r) <<0) & NE1_MASK_DS_GC_G216)
#define NE1_SET_DS_GC_G217(r)   (((r) <<8) & NE1_MASK_DS_GC_G217)
#define NE1_SET_DS_GC_G218(r)   (((r) <<16) & NE1_MASK_DS_GC_G218)
#define NE1_SET_DS_GC_G219(r)   (((r) <<24) & NE1_MASK_DS_GC_G219)
#define NE1_SET_DS_GC_G220(r)   (((r) <<32) & NE1_MASK_DS_GC_G220)
#define NE1_SET_DS_GC_G221(r)   (((r) <<40) & NE1_MASK_DS_GC_G221)
#define NE1_SET_DS_GC_G222(r)   (((r) <<48) & NE1_MASK_DS_GC_G222)
#define NE1_SET_DS_GC_G223(r)   (((r) <<56) & NE1_MASK_DS_GC_G223)
#define NE1_SET_DS_GC_G224(r)   (((r) <<0) & NE1_MASK_DS_GC_G224)
#define NE1_SET_DS_GC_G225(r)   (((r) <<8) & NE1_MASK_DS_GC_G225)
#define NE1_SET_DS_GC_G226(r)   (((r) <<16) & NE1_MASK_DS_GC_G226)
#define NE1_SET_DS_GC_G227(r)   (((r) <<24) & NE1_MASK_DS_GC_G227)
#define NE1_SET_DS_GC_G228(r)   (((r) <<32) & NE1_MASK_DS_GC_G228)
#define NE1_SET_DS_GC_G229(r)   (((r) <<40) & NE1_MASK_DS_GC_G229)
#define NE1_SET_DS_GC_G230(r)   (((r) <<48) & NE1_MASK_DS_GC_G230)
#define NE1_SET_DS_GC_G231(r)   (((r) <<56) & NE1_MASK_DS_GC_G231)
#define NE1_SET_DS_GC_G232(r)   (((r) <<0) & NE1_MASK_DS_GC_G232)
#define NE1_SET_DS_GC_G233(r)   (((r) <<8) & NE1_MASK_DS_GC_G233)
#define NE1_SET_DS_GC_G234(r)   (((r) <<16) & NE1_MASK_DS_GC_G234)
#define NE1_SET_DS_GC_G235(r)   (((r) <<24) & NE1_MASK_DS_GC_G235)
#define NE1_SET_DS_GC_G236(r)   (((r) <<32) & NE1_MASK_DS_GC_G236)
#define NE1_SET_DS_GC_G237(r)   (((r) <<40) & NE1_MASK_DS_GC_G237)
#define NE1_SET_DS_GC_G238(r)   (((r) <<48) & NE1_MASK_DS_GC_G238)
#define NE1_SET_DS_GC_G239(r)   (((r) <<56) & NE1_MASK_DS_GC_G239)
#define NE1_SET_DS_GC_G240(r)   (((r) <<0) & NE1_MASK_DS_GC_G240)
#define NE1_SET_DS_GC_G241(r)   (((r) <<8) & NE1_MASK_DS_GC_G241)
#define NE1_SET_DS_GC_G242(r)   (((r) <<16) & NE1_MASK_DS_GC_G242)
#define NE1_SET_DS_GC_G243(r)   (((r) <<24) & NE1_MASK_DS_GC_G243)
#define NE1_SET_DS_GC_G244(r)   (((r) <<32) & NE1_MASK_DS_GC_G244)
#define NE1_SET_DS_GC_G245(r)   (((r) <<40) & NE1_MASK_DS_GC_G245)
#define NE1_SET_DS_GC_G246(r)   (((r) <<48) & NE1_MASK_DS_GC_G246)
#define NE1_SET_DS_GC_G247(r)   (((r) <<56) & NE1_MASK_DS_GC_G247)
#define NE1_SET_DS_GC_G248(r)   (((r) <<0) & NE1_MASK_DS_GC_G248)
#define NE1_SET_DS_GC_G249(r)   (((r) <<8) & NE1_MASK_DS_GC_G249)
#define NE1_SET_DS_GC_G250(r)   (((r) <<16) & NE1_MASK_DS_GC_G250)
#define NE1_SET_DS_GC_G251(r)   (((r) <<24) & NE1_MASK_DS_GC_G251)
#define NE1_SET_DS_GC_G252(r)   (((r) <<32) & NE1_MASK_DS_GC_G252)
#define NE1_SET_DS_GC_G253(r)   (((r) <<40) & NE1_MASK_DS_GC_G253)
#define NE1_SET_DS_GC_G254(r)   (((r) <<48) & NE1_MASK_DS_GC_G254)
#define NE1_SET_DS_GC_G255(r)   (((r) <<56) & NE1_MASK_DS_GC_G255)
#define NE1_SET_DS_GC_B0(r)     (((r) <<0) & NE1_MASK_DS_GC_B0)
#define NE1_SET_DS_GC_B1(r)     (((r) <<8) & NE1_MASK_DS_GC_B1)
#define NE1_SET_DS_GC_B2(r)     (((r) <<16) & NE1_MASK_DS_GC_B2)
#define NE1_SET_DS_GC_B3(r)     (((r) <<24) & NE1_MASK_DS_GC_B3)
#define NE1_SET_DS_GC_B4(r)     (((r) <<32) & NE1_MASK_DS_GC_B4)
#define NE1_SET_DS_GC_B5(r)     (((r) <<40) & NE1_MASK_DS_GC_B5)
#define NE1_SET_DS_GC_B6(r)     (((r) <<48) & NE1_MASK_DS_GC_B6)
#define NE1_SET_DS_GC_B7(r)     (((r) <<56) & NE1_MASK_DS_GC_B7)
#define NE1_SET_DS_GC_B8(r)     (((r) <<0) & NE1_MASK_DS_GC_B8)
#define NE1_SET_DS_GC_B9(r)     (((r) <<8) & NE1_MASK_DS_GC_B9)
#define NE1_SET_DS_GC_B10(r)    (((r) <<16) & NE1_MASK_DS_GC_B10)
#define NE1_SET_DS_GC_B11(r)    (((r) <<24) & NE1_MASK_DS_GC_B11)
#define NE1_SET_DS_GC_B12(r)    (((r) <<32) & NE1_MASK_DS_GC_B12)
#define NE1_SET_DS_GC_B13(r)    (((r) <<40) & NE1_MASK_DS_GC_B13)
#define NE1_SET_DS_GC_B14(r)    (((r) <<48) & NE1_MASK_DS_GC_B14)
#define NE1_SET_DS_GC_B15(r)    (((r) <<56) & NE1_MASK_DS_GC_B15)
#define NE1_SET_DS_GC_B16(r)    (((r) <<0) & NE1_MASK_DS_GC_B16)
#define NE1_SET_DS_GC_B17(r)    (((r) <<8) & NE1_MASK_DS_GC_B17)
#define NE1_SET_DS_GC_B18(r)    (((r) <<16) & NE1_MASK_DS_GC_B18)
#define NE1_SET_DS_GC_B19(r)    (((r) <<24) & NE1_MASK_DS_GC_B19)
#define NE1_SET_DS_GC_B20(r)    (((r) <<32) & NE1_MASK_DS_GC_B20)
#define NE1_SET_DS_GC_B21(r)    (((r) <<40) & NE1_MASK_DS_GC_B21)
#define NE1_SET_DS_GC_B22(r)    (((r) <<48) & NE1_MASK_DS_GC_B22)
#define NE1_SET_DS_GC_B23(r)    (((r) <<56) & NE1_MASK_DS_GC_B23)
#define NE1_SET_DS_GC_B24(r)    (((r) <<0) & NE1_MASK_DS_GC_B24)
#define NE1_SET_DS_GC_B25(r)    (((r) <<8) & NE1_MASK_DS_GC_B25)
#define NE1_SET_DS_GC_B26(r)    (((r) <<16) & NE1_MASK_DS_GC_B26)
#define NE1_SET_DS_GC_B27(r)    (((r) <<24) & NE1_MASK_DS_GC_B27)
#define NE1_SET_DS_GC_B28(r)    (((r) <<32) & NE1_MASK_DS_GC_B28)
#define NE1_SET_DS_GC_B29(r)    (((r) <<40) & NE1_MASK_DS_GC_B29)
#define NE1_SET_DS_GC_B30(r)    (((r) <<48) & NE1_MASK_DS_GC_B30)
#define NE1_SET_DS_GC_B31(r)    (((r) <<56) & NE1_MASK_DS_GC_B31)
#define NE1_SET_DS_GC_B32(r)    (((r) <<0) & NE1_MASK_DS_GC_B32)
#define NE1_SET_DS_GC_B33(r)    (((r) <<8) & NE1_MASK_DS_GC_B33)
#define NE1_SET_DS_GC_B34(r)    (((r) <<16) & NE1_MASK_DS_GC_B34)
#define NE1_SET_DS_GC_B35(r)    (((r) <<24) & NE1_MASK_DS_GC_B35)
#define NE1_SET_DS_GC_B36(r)    (((r) <<32) & NE1_MASK_DS_GC_B36)
#define NE1_SET_DS_GC_B37(r)    (((r) <<40) & NE1_MASK_DS_GC_B37)
#define NE1_SET_DS_GC_B38(r)    (((r) <<48) & NE1_MASK_DS_GC_B38)
#define NE1_SET_DS_GC_B39(r)    (((r) <<56) & NE1_MASK_DS_GC_B39)
#define NE1_SET_DS_GC_B40(r)    (((r) <<0) & NE1_MASK_DS_GC_B40)
#define NE1_SET_DS_GC_B41(r)    (((r) <<8) & NE1_MASK_DS_GC_B41)
#define NE1_SET_DS_GC_B42(r)    (((r) <<16) & NE1_MASK_DS_GC_B42)
#define NE1_SET_DS_GC_B43(r)    (((r) <<24) & NE1_MASK_DS_GC_B43)
#define NE1_SET_DS_GC_B44(r)    (((r) <<32) & NE1_MASK_DS_GC_B44)
#define NE1_SET_DS_GC_B45(r)    (((r) <<40) & NE1_MASK_DS_GC_B45)
#define NE1_SET_DS_GC_B46(r)    (((r) <<48) & NE1_MASK_DS_GC_B46)
#define NE1_SET_DS_GC_B47(r)    (((r) <<56) & NE1_MASK_DS_GC_B47)
#define NE1_SET_DS_GC_B48(r)    (((r) <<0) & NE1_MASK_DS_GC_B48)
#define NE1_SET_DS_GC_B49(r)    (((r) <<8) & NE1_MASK_DS_GC_B49)
#define NE1_SET_DS_GC_B50(r)    (((r) <<16) & NE1_MASK_DS_GC_B50)
#define NE1_SET_DS_GC_B51(r)    (((r) <<24) & NE1_MASK_DS_GC_B51)
#define NE1_SET_DS_GC_B52(r)    (((r) <<32) & NE1_MASK_DS_GC_B52)
#define NE1_SET_DS_GC_B53(r)    (((r) <<40) & NE1_MASK_DS_GC_B53)
#define NE1_SET_DS_GC_B54(r)    (((r) <<48) & NE1_MASK_DS_GC_B54)
#define NE1_SET_DS_GC_B55(r)    (((r) <<56) & NE1_MASK_DS_GC_B55)
#define NE1_SET_DS_GC_B56(r)    (((r) <<0) & NE1_MASK_DS_GC_B56)
#define NE1_SET_DS_GC_B57(r)    (((r) <<8) & NE1_MASK_DS_GC_B57)
#define NE1_SET_DS_GC_B58(r)    (((r) <<16) & NE1_MASK_DS_GC_B58)
#define NE1_SET_DS_GC_B59(r)    (((r) <<24) & NE1_MASK_DS_GC_B59)
#define NE1_SET_DS_GC_B60(r)    (((r) <<32) & NE1_MASK_DS_GC_B60)
#define NE1_SET_DS_GC_B61(r)    (((r) <<40) & NE1_MASK_DS_GC_B61)
#define NE1_SET_DS_GC_B62(r)    (((r) <<48) & NE1_MASK_DS_GC_B62)
#define NE1_SET_DS_GC_B63(r)    (((r) <<56) & NE1_MASK_DS_GC_B63)
#define NE1_SET_DS_GC_B64(r)    (((r) <<0) & NE1_MASK_DS_GC_B64)
#define NE1_SET_DS_GC_B65(r)    (((r) <<8) & NE1_MASK_DS_GC_B65)
#define NE1_SET_DS_GC_B66(r)    (((r) <<16) & NE1_MASK_DS_GC_B66)
#define NE1_SET_DS_GC_B67(r)    (((r) <<24) & NE1_MASK_DS_GC_B67)
#define NE1_SET_DS_GC_B68(r)    (((r) <<32) & NE1_MASK_DS_GC_B68)
#define NE1_SET_DS_GC_B69(r)    (((r) <<40) & NE1_MASK_DS_GC_B69)
#define NE1_SET_DS_GC_B70(r)    (((r) <<48) & NE1_MASK_DS_GC_B70)
#define NE1_SET_DS_GC_B71(r)    (((r) <<56) & NE1_MASK_DS_GC_B71)
#define NE1_SET_DS_GC_B72(r)    (((r) <<0) & NE1_MASK_DS_GC_B72)
#define NE1_SET_DS_GC_B73(r)    (((r) <<8) & NE1_MASK_DS_GC_B73)
#define NE1_SET_DS_GC_B74(r)    (((r) <<16) & NE1_MASK_DS_GC_B74)
#define NE1_SET_DS_GC_B75(r)    (((r) <<24) & NE1_MASK_DS_GC_B75)
#define NE1_SET_DS_GC_B76(r)    (((r) <<32) & NE1_MASK_DS_GC_B76)
#define NE1_SET_DS_GC_B77(r)    (((r) <<40) & NE1_MASK_DS_GC_B77)
#define NE1_SET_DS_GC_B78(r)    (((r) <<48) & NE1_MASK_DS_GC_B78)
#define NE1_SET_DS_GC_B79(r)    (((r) <<56) & NE1_MASK_DS_GC_B79)
#define NE1_SET_DS_GC_B80(r)    (((r) <<0) & NE1_MASK_DS_GC_B80)
#define NE1_SET_DS_GC_B81(r)    (((r) <<8) & NE1_MASK_DS_GC_B81)
#define NE1_SET_DS_GC_B82(r)    (((r) <<16) & NE1_MASK_DS_GC_B82)
#define NE1_SET_DS_GC_B83(r)    (((r) <<24) & NE1_MASK_DS_GC_B83)
#define NE1_SET_DS_GC_B84(r)    (((r) <<32) & NE1_MASK_DS_GC_B84)
#define NE1_SET_DS_GC_B85(r)    (((r) <<40) & NE1_MASK_DS_GC_B85)
#define NE1_SET_DS_GC_B86(r)    (((r) <<48) & NE1_MASK_DS_GC_B86)
#define NE1_SET_DS_GC_B87(r)    (((r) <<56) & NE1_MASK_DS_GC_B87)
#define NE1_SET_DS_GC_B88(r)    (((r) <<0) & NE1_MASK_DS_GC_B88)
#define NE1_SET_DS_GC_B89(r)    (((r) <<8) & NE1_MASK_DS_GC_B89)
#define NE1_SET_DS_GC_B90(r)    (((r) <<16) & NE1_MASK_DS_GC_B90)
#define NE1_SET_DS_GC_B91(r)    (((r) <<24) & NE1_MASK_DS_GC_B91)
#define NE1_SET_DS_GC_B92(r)    (((r) <<32) & NE1_MASK_DS_GC_B92)
#define NE1_SET_DS_GC_B93(r)    (((r) <<40) & NE1_MASK_DS_GC_B93)
#define NE1_SET_DS_GC_B94(r)    (((r) <<48) & NE1_MASK_DS_GC_B94)
#define NE1_SET_DS_GC_B95(r)    (((r) <<56) & NE1_MASK_DS_GC_B95)
#define NE1_SET_DS_GC_B96(r)    (((r) <<0) & NE1_MASK_DS_GC_B96)
#define NE1_SET_DS_GC_B97(r)    (((r) <<8) & NE1_MASK_DS_GC_B97)
#define NE1_SET_DS_GC_B98(r)    (((r) <<16) & NE1_MASK_DS_GC_B98)
#define NE1_SET_DS_GC_B99(r)    (((r) <<24) & NE1_MASK_DS_GC_B99)
#define NE1_SET_DS_GC_B100(r)   (((r) <<32) & NE1_MASK_DS_GC_B100)
#define NE1_SET_DS_GC_B101(r)   (((r) <<40) & NE1_MASK_DS_GC_B101)
#define NE1_SET_DS_GC_B102(r)   (((r) <<48) & NE1_MASK_DS_GC_B102)
#define NE1_SET_DS_GC_B103(r)   (((r) <<56) & NE1_MASK_DS_GC_B103)
#define NE1_SET_DS_GC_B104(r)   (((r) <<0) & NE1_MASK_DS_GC_B104)
#define NE1_SET_DS_GC_B105(r)   (((r) <<8) & NE1_MASK_DS_GC_B105)
#define NE1_SET_DS_GC_B106(r)   (((r) <<16) & NE1_MASK_DS_GC_B106)
#define NE1_SET_DS_GC_B107(r)   (((r) <<24) & NE1_MASK_DS_GC_B107)
#define NE1_SET_DS_GC_B108(r)   (((r) <<32) & NE1_MASK_DS_GC_B108)
#define NE1_SET_DS_GC_B109(r)   (((r) <<40) & NE1_MASK_DS_GC_B109)
#define NE1_SET_DS_GC_B110(r)   (((r) <<48) & NE1_MASK_DS_GC_B110)
#define NE1_SET_DS_GC_B111(r)   (((r) <<56) & NE1_MASK_DS_GC_B111)
#define NE1_SET_DS_GC_B112(r)   (((r) <<0) & NE1_MASK_DS_GC_B112)
#define NE1_SET_DS_GC_B113(r)   (((r) <<8) & NE1_MASK_DS_GC_B113)
#define NE1_SET_DS_GC_B114(r)   (((r) <<16) & NE1_MASK_DS_GC_B114)
#define NE1_SET_DS_GC_B115(r)   (((r) <<24) & NE1_MASK_DS_GC_B115)
#define NE1_SET_DS_GC_B116(r)   (((r) <<32) & NE1_MASK_DS_GC_B116)
#define NE1_SET_DS_GC_B117(r)   (((r) <<40) & NE1_MASK_DS_GC_B117)
#define NE1_SET_DS_GC_B118(r)   (((r) <<48) & NE1_MASK_DS_GC_B118)
#define NE1_SET_DS_GC_B119(r)   (((r) <<56) & NE1_MASK_DS_GC_B119)
#define NE1_SET_DS_GC_B120(r)   (((r) <<0) & NE1_MASK_DS_GC_B120)
#define NE1_SET_DS_GC_B121(r)   (((r) <<8) & NE1_MASK_DS_GC_B121)
#define NE1_SET_DS_GC_B122(r)   (((r) <<16) & NE1_MASK_DS_GC_B122)
#define NE1_SET_DS_GC_B123(r)   (((r) <<24) & NE1_MASK_DS_GC_B123)
#define NE1_SET_DS_GC_B124(r)   (((r) <<32) & NE1_MASK_DS_GC_B124)
#define NE1_SET_DS_GC_B125(r)   (((r) <<40) & NE1_MASK_DS_GC_B125)
#define NE1_SET_DS_GC_B126(r)   (((r) <<48) & NE1_MASK_DS_GC_B126)
#define NE1_SET_DS_GC_B127(r)   (((r) <<56) & NE1_MASK_DS_GC_B127)
#define NE1_SET_DS_GC_B128(r)   (((r) <<0) & NE1_MASK_DS_GC_B128)
#define NE1_SET_DS_GC_B129(r)   (((r) <<8) & NE1_MASK_DS_GC_B129)
#define NE1_SET_DS_GC_B130(r)   (((r) <<16) & NE1_MASK_DS_GC_B130)
#define NE1_SET_DS_GC_B131(r)   (((r) <<24) & NE1_MASK_DS_GC_B131)
#define NE1_SET_DS_GC_B132(r)   (((r) <<32) & NE1_MASK_DS_GC_B132)
#define NE1_SET_DS_GC_B133(r)   (((r) <<40) & NE1_MASK_DS_GC_B133)
#define NE1_SET_DS_GC_B134(r)   (((r) <<48) & NE1_MASK_DS_GC_B134)
#define NE1_SET_DS_GC_B135(r)   (((r) <<56) & NE1_MASK_DS_GC_B135)
#define NE1_SET_DS_GC_B136(r)   (((r) <<0) & NE1_MASK_DS_GC_B136)
#define NE1_SET_DS_GC_B137(r)   (((r) <<8) & NE1_MASK_DS_GC_B137)
#define NE1_SET_DS_GC_B138(r)   (((r) <<16) & NE1_MASK_DS_GC_B138)
#define NE1_SET_DS_GC_B139(r)   (((r) <<24) & NE1_MASK_DS_GC_B139)
#define NE1_SET_DS_GC_B140(r)   (((r) <<32) & NE1_MASK_DS_GC_B140)
#define NE1_SET_DS_GC_B141(r)   (((r) <<40) & NE1_MASK_DS_GC_B141)
#define NE1_SET_DS_GC_B142(r)   (((r) <<48) & NE1_MASK_DS_GC_B142)
#define NE1_SET_DS_GC_B143(r)   (((r) <<56) & NE1_MASK_DS_GC_B143)
#define NE1_SET_DS_GC_B144(r)   (((r) <<0) & NE1_MASK_DS_GC_B144)
#define NE1_SET_DS_GC_B145(r)   (((r) <<8) & NE1_MASK_DS_GC_B145)
#define NE1_SET_DS_GC_B146(r)   (((r) <<16) & NE1_MASK_DS_GC_B146)
#define NE1_SET_DS_GC_B147(r)   (((r) <<24) & NE1_MASK_DS_GC_B147)
#define NE1_SET_DS_GC_B148(r)   (((r) <<32) & NE1_MASK_DS_GC_B148)
#define NE1_SET_DS_GC_B149(r)   (((r) <<40) & NE1_MASK_DS_GC_B149)
#define NE1_SET_DS_GC_B150(r)   (((r) <<48) & NE1_MASK_DS_GC_B150)
#define NE1_SET_DS_GC_B151(r)   (((r) <<56) & NE1_MASK_DS_GC_B151)
#define NE1_SET_DS_GC_B152(r)   (((r) <<0) & NE1_MASK_DS_GC_B152)
#define NE1_SET_DS_GC_B153(r)   (((r) <<8) & NE1_MASK_DS_GC_B153)
#define NE1_SET_DS_GC_B154(r)   (((r) <<16) & NE1_MASK_DS_GC_B154)
#define NE1_SET_DS_GC_B155(r)   (((r) <<24) & NE1_MASK_DS_GC_B155)
#define NE1_SET_DS_GC_B156(r)   (((r) <<32) & NE1_MASK_DS_GC_B156)
#define NE1_SET_DS_GC_B157(r)   (((r) <<40) & NE1_MASK_DS_GC_B157)
#define NE1_SET_DS_GC_B158(r)   (((r) <<48) & NE1_MASK_DS_GC_B158)
#define NE1_SET_DS_GC_B159(r)   (((r) <<56) & NE1_MASK_DS_GC_B159)
#define NE1_SET_DS_GC_B160(r)   (((r) <<0) & NE1_MASK_DS_GC_B160)
#define NE1_SET_DS_GC_B161(r)   (((r) <<8) & NE1_MASK_DS_GC_B161)
#define NE1_SET_DS_GC_B162(r)   (((r) <<16) & NE1_MASK_DS_GC_B162)
#define NE1_SET_DS_GC_B163(r)   (((r) <<24) & NE1_MASK_DS_GC_B163)
#define NE1_SET_DS_GC_B164(r)   (((r) <<32) & NE1_MASK_DS_GC_B164)
#define NE1_SET_DS_GC_B165(r)   (((r) <<40) & NE1_MASK_DS_GC_B165)
#define NE1_SET_DS_GC_B166(r)   (((r) <<48) & NE1_MASK_DS_GC_B166)
#define NE1_SET_DS_GC_B167(r)   (((r) <<56) & NE1_MASK_DS_GC_B167)
#define NE1_SET_DS_GC_B168(r)   (((r) <<0) & NE1_MASK_DS_GC_B168)
#define NE1_SET_DS_GC_B169(r)   (((r) <<8) & NE1_MASK_DS_GC_B169)
#define NE1_SET_DS_GC_B170(r)   (((r) <<16) & NE1_MASK_DS_GC_B170)
#define NE1_SET_DS_GC_B171(r)   (((r) <<24) & NE1_MASK_DS_GC_B171)
#define NE1_SET_DS_GC_B172(r)   (((r) <<32) & NE1_MASK_DS_GC_B172)
#define NE1_SET_DS_GC_B173(r)   (((r) <<40) & NE1_MASK_DS_GC_B173)
#define NE1_SET_DS_GC_B174(r)   (((r) <<48) & NE1_MASK_DS_GC_B174)
#define NE1_SET_DS_GC_B175(r)   (((r) <<56) & NE1_MASK_DS_GC_B175)
#define NE1_SET_DS_GC_B176(r)   (((r) <<0) & NE1_MASK_DS_GC_B176)
#define NE1_SET_DS_GC_B177(r)   (((r) <<8) & NE1_MASK_DS_GC_B177)
#define NE1_SET_DS_GC_B178(r)   (((r) <<16) & NE1_MASK_DS_GC_B178)
#define NE1_SET_DS_GC_B179(r)   (((r) <<24) & NE1_MASK_DS_GC_B179)
#define NE1_SET_DS_GC_B180(r)   (((r) <<32) & NE1_MASK_DS_GC_B180)
#define NE1_SET_DS_GC_B181(r)   (((r) <<40) & NE1_MASK_DS_GC_B181)
#define NE1_SET_DS_GC_B182(r)   (((r) <<48) & NE1_MASK_DS_GC_B182)
#define NE1_SET_DS_GC_B183(r)   (((r) <<56) & NE1_MASK_DS_GC_B183)
#define NE1_SET_DS_GC_B184(r)   (((r) <<0) & NE1_MASK_DS_GC_B184)
#define NE1_SET_DS_GC_B185(r)   (((r) <<8) & NE1_MASK_DS_GC_B185)
#define NE1_SET_DS_GC_B186(r)   (((r) <<16) & NE1_MASK_DS_GC_B186)
#define NE1_SET_DS_GC_B187(r)   (((r) <<24) & NE1_MASK_DS_GC_B187)
#define NE1_SET_DS_GC_B188(r)   (((r) <<32) & NE1_MASK_DS_GC_B188)
#define NE1_SET_DS_GC_B189(r)   (((r) <<40) & NE1_MASK_DS_GC_B189)
#define NE1_SET_DS_GC_B190(r)   (((r) <<48) & NE1_MASK_DS_GC_B190)
#define NE1_SET_DS_GC_B191(r)   (((r) <<56) & NE1_MASK_DS_GC_B191)
#define NE1_SET_DS_GC_B192(r)   (((r) <<0) & NE1_MASK_DS_GC_B192)
#define NE1_SET_DS_GC_B193(r)   (((r) <<8) & NE1_MASK_DS_GC_B193)
#define NE1_SET_DS_GC_B194(r)   (((r) <<16) & NE1_MASK_DS_GC_B194)
#define NE1_SET_DS_GC_B195(r)   (((r) <<24) & NE1_MASK_DS_GC_B195)
#define NE1_SET_DS_GC_B196(r)   (((r) <<32) & NE1_MASK_DS_GC_B196)
#define NE1_SET_DS_GC_B197(r)   (((r) <<40) & NE1_MASK_DS_GC_B197)
#define NE1_SET_DS_GC_B198(r)   (((r) <<48) & NE1_MASK_DS_GC_B198)
#define NE1_SET_DS_GC_B199(r)   (((r) <<56) & NE1_MASK_DS_GC_B199)
#define NE1_SET_DS_GC_B200(r)   (((r) <<0) & NE1_MASK_DS_GC_B200)
#define NE1_SET_DS_GC_B201(r)   (((r) <<8) & NE1_MASK_DS_GC_B201)
#define NE1_SET_DS_GC_B202(r)   (((r) <<16) & NE1_MASK_DS_GC_B202)
#define NE1_SET_DS_GC_B203(r)   (((r) <<24) & NE1_MASK_DS_GC_B203)
#define NE1_SET_DS_GC_B204(r)   (((r) <<32) & NE1_MASK_DS_GC_B204)
#define NE1_SET_DS_GC_B205(r)   (((r) <<40) & NE1_MASK_DS_GC_B205)
#define NE1_SET_DS_GC_B206(r)   (((r) <<48) & NE1_MASK_DS_GC_B206)
#define NE1_SET_DS_GC_B207(r)   (((r) <<56) & NE1_MASK_DS_GC_B207)
#define NE1_SET_DS_GC_B208(r)   (((r) <<0) & NE1_MASK_DS_GC_B208)
#define NE1_SET_DS_GC_B209(r)   (((r) <<8) & NE1_MASK_DS_GC_B209)
#define NE1_SET_DS_GC_B210(r)   (((r) <<16) & NE1_MASK_DS_GC_B210)
#define NE1_SET_DS_GC_B211(r)   (((r) <<24) & NE1_MASK_DS_GC_B211)
#define NE1_SET_DS_GC_B212(r)   (((r) <<32) & NE1_MASK_DS_GC_B212)
#define NE1_SET_DS_GC_B213(r)   (((r) <<40) & NE1_MASK_DS_GC_B213)
#define NE1_SET_DS_GC_B214(r)   (((r) <<48) & NE1_MASK_DS_GC_B214)
#define NE1_SET_DS_GC_B215(r)   (((r) <<56) & NE1_MASK_DS_GC_B215)
#define NE1_SET_DS_GC_B216(r)   (((r) <<0) & NE1_MASK_DS_GC_B216)
#define NE1_SET_DS_GC_B217(r)   (((r) <<8) & NE1_MASK_DS_GC_B217)
#define NE1_SET_DS_GC_B218(r)   (((r) <<16) & NE1_MASK_DS_GC_B218)
#define NE1_SET_DS_GC_B219(r)   (((r) <<24) & NE1_MASK_DS_GC_B219)
#define NE1_SET_DS_GC_B220(r)   (((r) <<32) & NE1_MASK_DS_GC_B220)
#define NE1_SET_DS_GC_B221(r)   (((r) <<40) & NE1_MASK_DS_GC_B221)
#define NE1_SET_DS_GC_B222(r)   (((r) <<48) & NE1_MASK_DS_GC_B222)
#define NE1_SET_DS_GC_B223(r)   (((r) <<56) & NE1_MASK_DS_GC_B223)
#define NE1_SET_DS_GC_B224(r)   (((r) <<0) & NE1_MASK_DS_GC_B224)
#define NE1_SET_DS_GC_B225(r)   (((r) <<8) & NE1_MASK_DS_GC_B225)
#define NE1_SET_DS_GC_B226(r)   (((r) <<16) & NE1_MASK_DS_GC_B226)
#define NE1_SET_DS_GC_B227(r)   (((r) <<24) & NE1_MASK_DS_GC_B227)
#define NE1_SET_DS_GC_B228(r)   (((r) <<32) & NE1_MASK_DS_GC_B228)
#define NE1_SET_DS_GC_B229(r)   (((r) <<40) & NE1_MASK_DS_GC_B229)
#define NE1_SET_DS_GC_B230(r)   (((r) <<48) & NE1_MASK_DS_GC_B230)
#define NE1_SET_DS_GC_B231(r)   (((r) <<56) & NE1_MASK_DS_GC_B231)
#define NE1_SET_DS_GC_B232(r)   (((r) <<0) & NE1_MASK_DS_GC_B232)
#define NE1_SET_DS_GC_B233(r)   (((r) <<8) & NE1_MASK_DS_GC_B233)
#define NE1_SET_DS_GC_B234(r)   (((r) <<16) & NE1_MASK_DS_GC_B234)
#define NE1_SET_DS_GC_B235(r)   (((r) <<24) & NE1_MASK_DS_GC_B235)
#define NE1_SET_DS_GC_B236(r)   (((r) <<32) & NE1_MASK_DS_GC_B236)
#define NE1_SET_DS_GC_B237(r)   (((r) <<40) & NE1_MASK_DS_GC_B237)
#define NE1_SET_DS_GC_B238(r)   (((r) <<48) & NE1_MASK_DS_GC_B238)
#define NE1_SET_DS_GC_B239(r)   (((r) <<56) & NE1_MASK_DS_GC_B239)
#define NE1_SET_DS_GC_B240(r)   (((r) <<0) & NE1_MASK_DS_GC_B240)
#define NE1_SET_DS_GC_B241(r)   (((r) <<8) & NE1_MASK_DS_GC_B241)
#define NE1_SET_DS_GC_B242(r)   (((r) <<16) & NE1_MASK_DS_GC_B242)
#define NE1_SET_DS_GC_B243(r)   (((r) <<24) & NE1_MASK_DS_GC_B243)
#define NE1_SET_DS_GC_B244(r)   (((r) <<32) & NE1_MASK_DS_GC_B244)
#define NE1_SET_DS_GC_B245(r)   (((r) <<40) & NE1_MASK_DS_GC_B245)
#define NE1_SET_DS_GC_B246(r)   (((r) <<48) & NE1_MASK_DS_GC_B246)
#define NE1_SET_DS_GC_B247(r)   (((r) <<56) & NE1_MASK_DS_GC_B247)
#define NE1_SET_DS_GC_B248(r)   (((r) <<0) & NE1_MASK_DS_GC_B248)
#define NE1_SET_DS_GC_B249(r)   (((r) <<8) & NE1_MASK_DS_GC_B249)
#define NE1_SET_DS_GC_B250(r)   (((r) <<16) & NE1_MASK_DS_GC_B250)
#define NE1_SET_DS_GC_B251(r)   (((r) <<24) & NE1_MASK_DS_GC_B251)
#define NE1_SET_DS_GC_B252(r)   (((r) <<32) & NE1_MASK_DS_GC_B252)
#define NE1_SET_DS_GC_B253(r)   (((r) <<40) & NE1_MASK_DS_GC_B253)
#define NE1_SET_DS_GC_B254(r)   (((r) <<48) & NE1_MASK_DS_GC_B254)
#define NE1_SET_DS_GC_B255(r)   (((r) <<56) & NE1_MASK_DS_GC_B255)
#define NE1_SET_HC_DF(r)        (((r) <<0) & NE1_MASK_HC_DF)
#define NE1_SET_HC_LUT(r)       (((r) <<0) & NE1_MASK_HC_LUT)

/**
 *		000b:NE1_DB	001b:NE1_TD
 */
#define	NE1_MASK_IDMODE_BOARDID	(0x07000000)
#define	NE1_MASK_IDMODE_FREQ	(0x00000800)

// kind of board
#define	NE1_IDMODE_NE1DB		(0x00000000)
#define	NE1_IDMODE_NE1TB		(0x01000000)

// frequency of system
#define	NE1_IDMODE_300MH		(0x00000000)
#define	NE1_IDMODE_400MH		(0x00000800)

/**
 *		Macro for Parameters
 */
#define	NE1_HWLAYER_NUM			(8)

#define	NE1_HWLAYER0_ID			(0)
#define	NE1_HWLAYER1_ID			(1)
#define	NE1_HWLAYER2_ID			(2)
#define	NE1_HWLAYER3_ID			(3)
#define	NE1_HWLAYER4_ID			(4)
#define	NE1_HWLAYER5_ID			(5)
#define	NE1_HWLAYER6_ID			(6)
#define	NE1_HWCURSOR_ID			(7)

/**
 *		Marco for Control
 */
#define	NE1_INTERNAL			(0)
#define	NE1_EXTERNAL			(1)

#define	NE1_OFF					(0)
#define	NE1_ON					(1)

#define	NE1_NORMAL				(0)
#define	NE1_INVERSE				(1)

#define	NE1_REVERSE_POLARITY	(0)
#define	NE1_STRAIGHT_POLARITY	(1)

#define	NE1_IN_VSYNC			(0)
#define	NE1_OUT_OF_VSYNC		(1)

#define	NE1_IN_VBLANK			(0)
#define	NE1_OUT_OF_VBLANK		(1)

#define	NE1_DISABLE				(0)
#define	NE1_ENABLE				(1)

#define	NE1_YC_COEFFICIENT_1	(0)		//! Y( 0 - 255),C( 0 - 255)
#define	NE1_YC_COEFFICIENT_2	(1)		//! Y(16 - 235),C(16 - 240)

#define	NE1_GRAPHICS_MODE		(0)		//! neighborhood operation
#define	NE1_NATURE_MODE			(1)		//! linear interpolation

#define	NE1_BORDER_COLOR_MODE	(0)
#define	NE1_ROUND_LAP_MODE		(1)

#define	NE1_ABIT_0_TO_255		(0)		//! 255 is percolation
#define	NE1_ABIT_255_TO_0		(1)		//! 0 is percolation

#define	NE1_SHOW_HWCURSOR0		(0)		//! show HW cursor0
#define	NE1_SHOW_HWCURSOR1		(1)		//! show HW cursor1

#define	NE1_UPDATE_HWCURSOR0	(1)		//! update HW cursor0
#define	NE1_UPDATE_HWCURSOR1	(0)		//! update HW cursor1

#define	NE1_HIDE_HWCURSOR0		(1)		//! hide HW cursor0
#define	NE1_HIDE_HWCURSOR1		(0)		//! hide HW cursor1

/**
 *		Display Pixel Format
 */
#define	NE1_PIXFMT_ARGB0565		(0)
#define	NE1_PIXFMT_ARGB1555		(1)
#define	NE1_PIXFMT_ARGB4444		(2)
#define	NE1_PIXFMT_ARGB0888		(3)
#define	NE1_PIXFMT_ARGB8888		(4)
#define	NE1_PIXFMT_PALETTE		(5)
#define	NE1_PIXFMT_YUYV			(8)
#define	NE1_PIXFMT_UYVY			(9)
#define	NE1_PIXFMT_YVYU			(10)
#define	NE1_PIXFMT_VYUY			(11)

#define	NE1_SET_ARGB8888(a,r,g,b)		(((a) << 24) | ((r) << 16) | ((g) << 8) | ((b) << 0))


/**
 *		Macro Definition for Interrupt
 */
#define	NE1_MASK_INT_VBLANK				(0x0000000000000001LL)
#define	NE1_MASK_INT_VSYNC				(0x0000000000000002LL)
#define	NE1_MASK_INT_ICIL				(0x0000000000000004LL)
#define	NE1_MASK_INT_ERR_EXTSYNC		(0x0000000000000008LL)
#define	NE1_MASK_INT_ERR_UNDERRUN_DISP	(0x0000000000000010LL)
#define	NE1_MASK_INT_ERR_UNDERRUN_LUT	(0x0000000000000020LL)
#define	NE1_MASK_INT_ERR_TIMEOUT_BUS	(0x0000000000000040LL)

/**
 *		Display Function Trace
 */
#ifdef NE1_DISPLAY_TRACE_ON
#define	NE1_DISPLAY_TRACE(f, m)			if((f) && RETAILMSG(1,(TEXT(m)))){}
#define	NE1_TRACEMSG		DEBUGMSG
#else
#define	NE1_DISPLAY_TRACE(f, m)
#define	NE1_DEBUG
#define	NE1_TRACEMSG		DEBUGMSG
#endif


