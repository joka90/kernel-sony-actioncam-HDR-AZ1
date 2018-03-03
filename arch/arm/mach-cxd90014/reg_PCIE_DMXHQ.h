/*
 * arch/arm/mach-cxd90014/reg_PCIE_DMXHQ.h
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


#ifndef     __REG_PCIE_DMXHQ_H__
#define     __REG_PCIE_DMXHQ_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TLP Header Register for PCIe Memory transaction (Offset F00_0010h)
*/
typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t               : 8;         /**< reserved */
        uint32_t       TC      : 4;         /**< TC */
        uint32_t               : 4;         /**< reserved */
        uint32_t               : 16;        /**< reserved */
    }   bit;
}   PCIE_DMXHQ_UN_TLPHR;

/**
 * @brief PCIe Configuration Target ID Register (CFG0-15) (Offset F00_0200h-F00_023Ch)
*/
typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       TFNCA   : 3;         /**< TFNCA */
        uint32_t       TDNCA   : 5;         /**< TDNCA */
        uint32_t       TBNCA   : 8;         /**< TBNCA */
        uint32_t               : 16;        /**< reserved */
    }   bit;
}   PCIE_DMXHQ_UN_CTIDR;

/**
 * @brief Secondary Bus Number for Configuration Access (Offset F00_0300h)
*/
typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       SBN     : 3;         /**< SBN */
        uint32_t               : 5;         /**< reserved */
        uint32_t               : 8;         /**< reserved */
        uint32_t               : 16;        /**< reserved */
    }   bit;
}   PCIE_DMXHQ_UN_SBNCA;

/**
 * @brief memory table access lower (Offset F00_0000Hh)
*/
typedef     uint32_t       PCIE_MTAOL;

/**
 * @brief memory table access higher (Offset F00_0004Hh)
*/
typedef     uint32_t       PCIE_MTAOH;


/* ==================================================================== */
#define __I     volatile const              /**< read only */
#define __O     volatile                    /**< write only */
#define __IO    volatile                    /**< read / write */

/* ==================================================================== */

#pragma pack(push,4)
/**
 * @brief DMXHQ (@0x0-0xFFF)
*/
typedef     struct      {
    __IO    PCIE_MTAOL              MTAOL;          /**< +000H */
    __IO    PCIE_MTAOH              MTAOH;          /**< +004H    */
    uint32_t                        __res1[2];      /**< +008,00Freserved */
    __IO    PCIE_DMXHQ_UN_TLPHR     TLPHR;          /**< +010H    */
    uint32_t                        __res2[((0x100-0x14)/4)];     /**< +014,0FFreserved */
    __IO    uint32_t                IOTAO;          /**< +100H    */
    uint32_t                        __res3[((0x200-0x104)/4)];     /**< +104,1FFreserved */
    __IO    PCIE_DMXHQ_UN_CTIDR     CTIDR[16];      /**< +200H-23FH  */
    uint32_t                        __res4[((0x300-0x240)/4)];     /**< +240,2FFreserved */
    __IO    PCIE_DMXHQ_UN_SBNCA     SBNCA;          /**< +300H    */
    uint32_t                        __res5[((0x1000-0x304)/4)];    /**< +304,FFFreserved */
}reg_pcie_dmxhq_t;

#pragma pack(pop)

#define PCIE_DMXHQ_INTSTAT	0x0800
# define PCIE_DMXHQ_INTSTAT_UNSUP (1 << 9)

#ifdef __cplusplus
}
#endif
#endif      /* __REG_PCIE_DMXHQ__ */
/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
