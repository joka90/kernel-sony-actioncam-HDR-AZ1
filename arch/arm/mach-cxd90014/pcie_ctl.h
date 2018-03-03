/*
 * arch/arm/mach-cxd90014/pcie_ctl.h
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


#ifndef __PCIE_CTL_H__
#define __PCIE_CTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PCIE_ERRATA_5

#if 0
#define DEBUG_PCIE                                                          /**< debug print flag */
#endif

#define PCIE_SET_CRSR                           0x00000107U                 /**< +004H Command Register   */
#define PCIE_SET_MUA                            0x00000000U                 /**< +058H Message Upper Address */
#define PCIE_SET_MD                             0x00000000U                 /**< +05CH Message Data */
#define PCIE_SET_MB                             0xFFFFFFFFU                 /**< +060H Mask Bits */
#define PCIE_SET_RCCR                           0x0000001FU                 /**< +09CH Root Control Register   */
#define PCIE_SET_BARE                           0x00000003U                 /**< +820H BAR Enable Register */
#define PCIE_SET_BA0                            0xFFFF0001U                 /**< +804H Base Address 0 Resource Setting */
#define PCIE_SET_BA1                            0x00000000U                 /**< +808H Base Address 1 Resource Setting */
#define PCIE_SET_BAR0                           0x00000000U                 /**< +010H Base Address Register 0 */
#define PCIE_SET_BAR1                           0x00000000U                 /**< +014H Base Address Register 1 */

#define PCIE_COMP_MRRS                          0xFFFF8FFFU                 /**<  Read Request size (0x088h) */
#define PCIE_MRRS_BOUNDARY                      12U                         /**<  12bit shift  */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_128       0                           /**<  128Byte max Read Request size   */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_256       1U                          /**<  256Byte max Read Request size   */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_512       2U                          /**<  512Byte max Read Request size   */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_1024      3U                          /**<  1024Byte max Read Request size  */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_2048      4U                          /**<  2048Byte max Read Request size  */
#define PCIE_EPRC_EN_PCIECR_DCSR_MRRS_4096      5U                          /**<  4096Byte max Read Request size  */

#define PCIE_COMP_MPS                           0xFFFFFF1FU                 /**<  payload size      (0x088h) */
#define PCIE_MPS_BOUNDARY                       5U                          /**<  5bit shift  */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_128        0                           /**<  128Byte max payload size        */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_256        1U                          /**<  256Byte max payload size        */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_512        2U                          /**<  512Byte max payload size        */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_1024       3U                          /**<  1024Byte max payload size       */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_2048       4U                          /**<  2048Byte max payload size       */
#define PCIE_EPRC_EN_PCIECR_DCSR_MPS_4096       5U                          /**<  4096Byte max payload size       */

#define PCIE_PCIECR_LCSR_LABS                   (1U << 31)                  /**< +092H Link Status[15] Link Autonomous Bandwidth Status */
#define PCIE_PCIECR_LCSR_LBMS                   (1U << 30)                  /**< +092H Link Status[14] Link Bandwidth Manangement Status */
#define PCIE_PCIECR_LCSR_RL                     (1U << 5)                   /**< +090H Link Control[5] Retrain Link */

#define PCIE_PCIECR_LC2R_TLS                    (0x0000000FU)               /**<  +0B0H Link Control2[4:0] Target Link Speed */

#define PCIE_COMP_IP                            0x0000FF00U                 /**<  Interrupt Pin  (0x03DH) */

#define PCIE_ARB_PCS				0x408
#define PCIE_ARB_PCS_PERST                      (1U<<0)
#define PCIE_AXIBCSR_AXIBMR_DLWM                (1U<<3U)
#define PCIE_RCEP_TSSR_CS_BIT                   (16U)                       /**<  +844H Transfer Speed Status[19:16] Current Speed bit shift */
#define PCIE_RCEP_TSSR_CS                       (0x000F0000)                /**<  +844H Transfer Speed Status[19:16] Current Speed */
#define PCIE_RCEP_TSSR_DLA                      (1U<<6U)                    /**<  +844H Transfer Speed Status[6]     DL_Active */
#define PCIE_RCEP_TSSR_SCR                      (1U<<3U)                    /**<  +844H Transfer Speed Status[3]     Speed Change Ready */
#define PCIE_RCEP_TSSR_SCS                      (0x00000007U)               /**<  +844H Transfer Speed Status[2:0]   Speed Change Status */

#define PCIE_SET_RECR                           0x00000006U                 /**< +12CH Root Error Command Register */
#define PCIE_SET_BC                             (1U<<1)                     /**< +03EH Bridge Control Register */
#define PCIE_SET_DCSR                           0x0000000EU                 /**< +088H Device Control Register */
#define PCIE_RESR_FATAL                         0x00000040U                 /**< +130H Root Error Status Register */
#define PCIE_RESR_NON_FATAL                     0x00000020U                 /**< +130H Root Error Status Register */

#define PCIE_SET_CTRL_INIT                      0x00000000U
#define PCIE_SET_CTRL_PRV_RE_TYPE_RC            (1U<<31U)
#define PCIE_SET_CTRL_RST_PONX                  (1U<<0)
#define PCIE_SET_CTRL_ST_RE_TYPE                (1U<<30U)
#define PCIE_SET_CTRL_RST_PEX                   (1U<<1U)

#define PCIE_SET_EXTINT_SEL                     0x00000000U
#define PCIE_SET_EXTINT_CLR                     0xAA000000U
#define PCIE_SET_EXTINT_EMB                     0xAA000000U

#define PCIE_SET_DMXHQ_TLPHR                    0x00000000U                 /**< +010H PCIe memory TLP Header   */

#define PCIE_BUS_BOUNDARY                       8U                          /**<  8bit shift  */
#define PCIE_DEV_BOUNDARY                       3U                          /**<  3bit shift  */
#define PCIE_FUNC_BOUNDARY                      0U                          /**<  0bit shift  */

typedef struct pcie_remap_reg{
    volatile void __iomem *pcie_dmxhq_base;
    volatile void __iomem *pcie_base;
    volatile void __iomem *wpcie_base;
} pcie_remap_reg_t;



#ifdef DEBUG_PCIE
#define pcie_assert_NULL(b)                                                 \
    if ((b) == NULL){                                                       \
        PCIE_PRINTK_ERR(KERN_ERR " %s(%d) %p *** ASSERT \n",__FILE__,__LINE__,(b));  \
        BUG();                                                              \
    }
#else
#define pcie_assert_NULL(b)
#endif

extern void     pcie_ctl_start     ( uint32_t pcie_select );
extern void     pcie_ctl_cfg_io_set( uint8_t bus_num, uint8_t dev_num, uint8_t func_num );
extern int      pcie_ctl_cfg_read(volatile void __iomem *addr, u32 *val);
extern volatile void __iomem *pcie_ctl_root_config(int slot);
extern void	pcie_ctl_cfg_root_hook(int slot, int where, int size, u32 val);
#ifdef CONFIG_PM
	extern void pcie_ctl_suspend(uint32_t pcie_select);
	extern void pcie_ctl_resume(uint32_t pcie_select);
#endif /* CONFIG_PM */

/*--------------------------------------------------------------------------*/
/*  data access functions */
/*--------------------------------------------------------------------------*/
extern void pcie_raw_writel(u32 val, volatile void __iomem *addr);
extern void pcie_raw_writew(u16 val, volatile void __iomem *addr);
extern void pcie_raw_writeb(u8 val, volatile void __iomem *addr);
extern u32 pcie_read(volatile void __iomem *addr);
extern void pcie_writel(u32 val, volatile void __iomem *addr);
extern void pcie_writew(u16 val, volatile void __iomem *addr);
extern void pcie_writeb(u8 val, volatile void __iomem *addr);

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __PCI_CTL_H__ */

/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
