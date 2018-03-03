/*
 * arch/arm/mach-cxd90014/reg_WPCIE.h
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

#ifndef     __REG_WPCIE_H__
#define     __REG_WPCIE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       RST_PONX        : 1;
        uint32_t       RST_PEX         : 1;
        uint32_t                       : 28;
        uint32_t       ST_RE_TYPE      : 1;
        uint32_t       PRV_RE_TYPE     : 1;
    }   bit;
}   WPCIE_UN_CONTROL;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       FUNC0           : 1;
        uint32_t       FUNC1           : 1;
        uint32_t       FUNC2           : 1;
        uint32_t       FUNC3           : 1;
        uint32_t       FUNC4           : 1;
        uint32_t       FUNC5           : 1;
        uint32_t       FUNC6           : 1;
        uint32_t       FUNC7           : 1;
        uint32_t                       : 24;
    }   bit;
}   WPCIE_UN_FUNCTION;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       ST_LTSSM        : 4;
        uint32_t                       : 4;
        uint32_t       ST_LTSSM_SUB    : 3;
        uint32_t                       : 5;
        uint32_t                       : 16;
    }   bit;
}   WPCIE_UN_ST_LTSMM;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       PM_F0_PMST      : 2;
        uint32_t       PM_F1_PMST      : 2;
        uint32_t       PM_F2_PMST      : 2;
        uint32_t       PM_F3_PMST      : 2;
        uint32_t       PM_F4_PMST      : 2;
        uint32_t       PM_F5_PMST      : 2;
        uint32_t       PM_F6_PMST      : 2;
        uint32_t       PM_F7_PMST      : 2;
        uint32_t                       : 16;
    }   bit;
}   WPCIE_UN_ST_PWR_MNG;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       ST_DMA_INT_0    : 1;
        uint32_t       ST_DMA_INT_1    : 1;
        uint32_t       ST_DMA_INT_2    : 1;
        uint32_t       ST_DMA_INT_3    : 1;
        uint32_t       ST_DMA_INT_4    : 1;
        uint32_t       ST_DMA_INT_5    : 1;
        uint32_t       ST_DMA_INT_6    : 1;
        uint32_t       ST_DMA_INT_7    : 1;
        uint32_t       ST_DMA_INT_8    : 1;
        uint32_t       ST_DMA_INT_9    : 1;
        uint32_t       ST_DMA_INT_10   : 1;
        uint32_t       ST_DMA_INT_11   : 1;
        uint32_t       ST_DMA_INT_12   : 1;
        uint32_t       ST_DMA_INT_13   : 1;
        uint32_t       ST_DMA_INT_14   : 1;
        uint32_t       ST_DMA_INT_15   : 1;
        uint32_t                       : 16;
    }   bit;
}   WPCIE_UN_ST_DMA_INT;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       ST_AXI_ERR_0    : 1;
        uint32_t       ST_AXI_ERR_1    : 1;
        uint32_t       ST_AXI_ERR_2    : 1;
        uint32_t       ST_AXI_ERR_3    : 1;
        uint32_t       ST_AXI_ERR_4    : 1;
        uint32_t       ST_AXI_ERR_5    : 1;
        uint32_t       ST_AXI_ERR_6    : 1;
        uint32_t       ST_AXI_ERR_7    : 1;
        uint32_t                       : 8;
        uint32_t                       : 16;
    }   bit;
}   WPCIE_UN_ST_AXI_ERR;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       RST_PEX_NEG         : 1;    /**< B00  */
        uint32_t       RST_PEX_POS         : 1;    /**< B01  */
        uint32_t       ST_GRGMX_NEG        : 1;    /**< B02  */
        uint32_t       ST_GRGMX_POS        : 1;    /**< B03  */
        uint32_t       ST_DL_UP_NEG        : 1;    /**< B04  */
        uint32_t       ST_DL_UP_POS        : 1;    /**< B05  */
        uint32_t                           : 2;    /**< B07:06   */
        uint32_t       PM_AUXEN_EDGE       : 1;    /**< B08  */
        uint32_t       PM_TOFF_REQ_EDGE    : 1;    /**< B09  */
        uint32_t       PM_PMERQ_DET_EDGE   : 1;    /**< B10  */
        uint32_t                           : 5;    /**< B15:11   */
        uint32_t                           : 8;    /**< B23:16   */
        uint32_t       MSG_INTA_NEG        : 1;    /**< B24  */
        uint32_t       MSG_INTA_POS        : 1;    /**< B25  */
        uint32_t       MSG_INTB_NEG        : 1;    /**< B26  */
        uint32_t       MSG_INTB_POS        : 1;    /**< B27  */
        uint32_t       MSG_INTC_NEG        : 1;    /**< B28  */
        uint32_t       MSG_INTC_POS        : 1;    /**< B29  */
        uint32_t       MSG_INTD_NEG        : 1;    /**< B30  */
        uint32_t       MSG_INTD_POS        : 1;    /**< B31  */
    }   bit;
}   WPCIE_UN_EXTINT;

typedef     union       {
    uint32_t       DATA;
    struct      {
        uint32_t       FUNCTION        : 8;                /**< FUNCTION */
        uint32_t       DMACH           : 8;                /**< DMACH */
        uint32_t       VC              : 8;                /**< VC */
        uint32_t                       : 8;                /**< reserved */
    }   bit;
}   WPCIE_UN_CONFIG;

/* ==================================================================== */
#define __I     volatile const                                  /**< read only */
#define __O     volatile                                        /**< write only */
#define __IO    volatile                                        /**< read / write */

/* ==================================================================== */

#pragma pack(push,4)
/**
 * @brief WPCIE (@0x0-0xFFF)
*/
typedef     struct      {
    __IO     WPCIE_UN_CONTROL        CONTROL;                   /**< +00H */
    uint32_t                         __res1[1];                 /**< +004,007reserved */
    __IO     WPCIE_UN_FUNCTION       MSG_FN_REQ;                /**< +08H */
    __IO     WPCIE_UN_FUNCTION       MSG_FN_SEL;                /**< +0CH */
    __IO     WPCIE_UN_ST_LTSMM       ST_LTSMM;                  /**< +10H */
    __IO     WPCIE_UN_ST_PWR_MNG     ST_PWR_MNG;                /**< +14H */
    uint32_t                         __res2[2];                 /**< +018,01Freserved */
    __IO     WPCIE_UN_ST_DMA_INT     ST_DMA_INT;                /**< +20H */
    __IO     WPCIE_UN_ST_AXI_ERR     ST_AXI_ERR;                /**< +24H */
    uint32_t                         __res3[6];                 /**< +028,03Freserved */
    __IO     WPCIE_UN_EXTINT         EXTINT_STS;                /**< +40H */
    __IO     WPCIE_UN_EXTINT         EXTINT_CLR;                /**< +44H */
    __IO     WPCIE_UN_EXTINT         EXTINT_ENB;                /**< +48H */
    __IO     WPCIE_UN_EXTINT         EXTINT_RAW;                /**< +4CH */
    __IO     WPCIE_UN_EXTINT         EXTINT_SEL;                /**< +50H */
    uint32_t                         __res4[((0xFC-0x54)/4)];   /**< +054,0FBreserved */
    __IO     WPCIE_UN_CONFIG         CONFIG;                    /**< +FCH */
    uint32_t                         __res5[((0x1000-0x100)/4)]; /**< +100,FFFreserved */
}reg_wpcie_t;
#pragma pack(pop)


#ifdef __cplusplus
}
#endif
#endif      /* __REG_WPCIE_H__ */
/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
