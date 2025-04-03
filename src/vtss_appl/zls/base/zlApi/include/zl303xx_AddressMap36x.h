

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Values and constants for the ZLS3036X device internal address map
*
*******************************************************************************/

#ifndef ZL303XX_ADDRESS_MAP_36X_H_
#define ZL303XX_ADDRESS_MAP_36X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_Global.h"
#include "zl303xx_AddressMap.h"

/*****************   DEFINES   ************************************************/

#define ZLS3036X_DPLL_MAX                    4        /* DPLL 0-3 */
#define ZLS3036X_REF_MAX                     11       /* ref 0-10 */
#define ZLS3036X_NUM_SYNTHS                  4        /* synth 0-3 */
#define ZLS3036X_NUM_POST_DIVS               4        /* post_div A-D */
#define ZLS3036X_STICKY_UPDATE_DELAY_MS      25       /* Delay in ms between sticky register updates */
#define ZLS3036X_SYNTH_COUNT                (ZLS3036X_DPLL_MAX)

/* Mask to cover all possible size bits:
   - 6 bits allows a range of 0-63 (corresponds to size of 1-64 bytes) */
#define ZL303XX_MEM_SIZE_MASK_36X         (Uint32T)0x0003F000
#define ZL303XX_MEM_SIZE_SHIFT_36X        (Uint16T)12

/* Interrupt mask registers */
#define ZLS3036X_REF_FAIL_ISR_MASK_7_0   ZL303XX_MAKE_MEM_ADDR_36X(0x023, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_REF_FAIL_ISR_MASK_10_8  ZL303XX_MAKE_MEM_ADDR_36X(0x024, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_ISR_MASK           ZL303XX_MAKE_MEM_ADDR_36X(0x025, ZL303XX_MEM_SIZE_1_BYTE)

/* Page register */
#define ZL3036X_ADDR_PAGE_REG 0x7F


/* Macro to extract the encoded SIZE (in bytes) from a virtual address */
#define ZL303XX_MEM_SIZE_EXTRACT_36X(addr)   \
      (Uint8T)(((addr & ZL303XX_MEM_SIZE_MASK_36X) >> ZL303XX_MEM_SIZE_SHIFT_36X) + 1)

/* Macro to encode the register SIZE for insertion into a virtual address */
#define ZL303XX_MEM_SIZE_INSERT_36X(bytes)       \
      (Uint32T)((((Uint32T)(bytes) - 1) << ZL303XX_MEM_SIZE_SHIFT_36X)  \
                                                      & ZL303XX_MEM_SIZE_MASK_36X)

#define ZLS3036X_CHECK_DPLLID(dpllId) \
   ( ((dpllId) >= ZLS3036X_DPLL_MAX) ?  \
     (ZL303XX_ERROR_NOTIFY("Invalid dpllId: "#dpllId), ZL303XX_PARAMETER_INVALID) : \
     ZL303XX_OK    \
   )

#define ZLS3036X_CHECK_REF_ID(refId) \
           (((refId) >= ZLS3036X_REF_MAX) \
               ? ZL303XX_TRACE_ERROR("Invalid refId: %u", refId, 0,0,0,0,0), ZL303XX_PARAMETER_INVALID \
               : ZL303XX_OK)

#define ZLS3036X_CHECK_SYNTH_ID(synthId) \
           (((synthId) >= ZLS3036X_NUM_SYNTHS) \
                ? ZL303XX_TRACE_ERROR("Invalid synthId=%u", synthId, 0,0,0,0,0), ZL303XX_PARAMETER_INVALID \
                : ZL303XX_OK)
				
/* clear sync fail flag*/
#define ZLS3036X_CLEAR_SYNC_FAIL_FLAG_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x1B7, ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3036X_CLEAR_SYNC_FAIL_FLAG_MASK                (Uint32T)(0x01)

/* sync fail flag status */
#define ZLS3036X_SYNC_FAIL_FLAG_STATUS_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x1B6, ZL303XX_MEM_SIZE_1_BYTE)

/* Mask to cover all possible size bits:
   - 6 bits allows a range of 0-63 (corresponds to size of 1-64 bytes) */
#define ZL303XX_MEM_SIZE_MASK         (Uint32T)0x0003F000
#define ZL303XX_MEM_SIZE_SHIFT        (Uint16T)12

/* Macro to extract the encoded SIZE (in bytes) from a virtual address */
#define ZL303XX_MEM_SIZE_EXTRACT(addr)   \
      (Uint8T)(((addr & ZL303XX_MEM_SIZE_MASK) >> ZL303XX_MEM_SIZE_SHIFT) + 1)

/* address mask, extract & insert definitions */
#define ZL303XX_MEM_ADDR_MASK_36X           (Uint32T)0x0000007F
#define ZL303XX_MEM_ADDR_AND_PAGE_MASK_36X           (Uint32T)0x00000FFF

/* Macro to get the destination address from a given virtual register address. */
#define ZL303XX_MEM_ADDR_EXTRACT_36X(addr)                           \
      (Uint32T)((addr) & ZL303XX_MEM_ADDR_MASK_36X)

/* Macro to set the virtual register address into a virtual address  */
#define ZL303XX_MEM_ADDR_INSERT_36X(addr)                            \
      (Uint32T)((Uint32T)(addr) & ZL303XX_MEM_ADDR_MASK_36X)


/* PAGE extension */
/******************/

/* The page bits apply only to direct access registers. Overlay addresses do
   not have Pages associated with them, instead the Page bits are a part of the
   overlay address extension. */
#define ZL303XX_MEM_PAGE_MASK_36X            (Uint32T)0x00000F80
#define ZL303XX_MEM_PAGE_SHIFT_36X           (Uint16T)7

/* Macro to extract the encoded PAGE value from a virtual address */
#define ZL303XX_MEM_PAGE_EXTRACT_36X(addr)   \
      (Uint8T)(((addr) & ZL303XX_MEM_PAGE_MASK_36X) >> ZL303XX_MEM_PAGE_SHIFT_36X)

/* Macro to encode the PAGE value for insertion into a virtual address */
#define ZL303XX_MEM_PAGE_INSERT_36X(page)       \
      (Uint32T)(((Uint32T)(page) << ZL303XX_MEM_PAGE_SHIFT_36X) & ZL303XX_MEM_PAGE_MASK_36X)

/* Paged addresses are within the following range */
#define ZL303XX_PAGED_ADDR_MIN_36X     (Uint32T)0x00
#define ZL303XX_PAGED_ADDR_MAX_36X     (Uint32T)0x7E

/*****************   REGISTER ADDRESS CONSTRUCT   *****************************/

/* Construct a memory address for a register on a specific device.
   The device field is not filled in by this macro */

/* Page can be ignored, use virtual addresses and decode the page in the driver */
#define ZL303XX_MAKE_MEM_ADDR_36X(fulladdr,size)        \
      (Uint32T)(((fulladdr) & ZL303XX_MEM_ADDR_AND_PAGE_MASK_36X) |       \
                ZL303XX_MEM_SIZE_INSERT_36X(size))

/*****************   REGISTER ADDRESSES   *************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/* Sticky lock register options */
typedef enum
{
   ZLS3036X_STICKY_UNLOCK = 0,
   ZLS3036X_STICKY_LOCK = 1
} ZLS3036X_StickyLockE;

#define ZLS3036X_CHECK_STICKY_LOCK(s) \
           (((ZLS3036X_StickyLockE)(s) > ZLS3036X_STICKY_LOCK) \
               ? ZL303XX_PARAMETER_INVALID \
               : ZL303XX_OK)

/* Reference monitor fail bits */
typedef enum
{
   ZLS3036X_REF_MON_FAIL_LOS = 0x01,
   ZLS3036X_REF_MON_FAIL_SCM = 0x02,
   ZLS3036X_REF_MON_FAIL_CFM = 0x04,
   ZLS3036X_REF_MON_FAIL_GST = 0x08,
   ZLS3036X_REF_MON_FAIL_PFM = 0x10,

   ZLS3036X_REF_MON_FAIL_MASK = 0x1F
} ZLS3036X_RefMonFailE;

/* Dpll reference selection and mode register options */
typedef enum
{
   ZLS3036X_DPLL_MODE_FREERUN = 0x0,
   ZLS3036X_DPLL_MODE_HOLDOVER = 0x1,
   ZLS3036X_DPLL_MODE_REFLOCK = 0x2,
   ZLS3036X_DPLL_MODE_AUTO = 0x3,
   ZLS3036X_DPLL_MODE_NCO = 0x4,

   ZLS3036X_DPLL_MODE_MASK = 0x07,
   ZLS3036X_DPLL_REFMODE_MASK = 0xF7
} ZLS3036X_DpllModeE;

#define ZLS3036X_CHECK_DPLL_MODE(mode) \
           (((mode) > ZLS3036X_DPLL_MODE_NCO) ? ZL303XX_PARAMETER_INVALID : ZL303XX_OK)

/* Dpll holdover lock status register options */
typedef enum
{
   ZLS3036X_HOLDOVER_STATUS_DPLL0 = 0x01,
   ZLS3036X_HOLDOVER_STATUS_DPLL1 = 0x04,
   ZLS3036X_HOLDOVER_STATUS_DPLL2 = 0x10,
   ZLS3036X_HOLDOVER_STATUS_DPLL3 = 0x40,

   ZLS3036X_LOCK_STATUS_DPLL0 = 0x02,
   ZLS3036X_LOCK_STATUS_DPLL1 = 0x08,
   ZLS3036X_LOCK_STATUS_DPLL2 = 0x20,
   ZLS3036X_LOCK_STATUS_DPLL3 = 0x80,

   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL0 = 0x03,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL1 = 0x0C,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL2 = 0x30,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL3 = 0xC0
} ZLS3036X_DpllHoldoverLockStatusE;

typedef enum
{
   ZLS3036X_POST_DIV_A = 0,
   ZLS3036X_POST_DIV_B,
   ZLS3036X_POST_DIV_C,
   ZLS3036X_POST_DIV_D
} ZLS3036X_PostDivE;

#define ZLS3036X_POST_DIV_MASK  ((1 << ZLS3036X_NUM_POST_DIVS) - 1)

typedef enum
{
    ZLS3036X_AMEM = 0,
    ZLS3036X_BMEM = 1,
    ZLS3036X_CMEM = 2,
    ZLS3036X_DMEM = 3
} ZLS3036X_MemPartE;

typedef enum
{
   ZLS3036X_DPLL_TIE_COMPLETE = 0,
   ZLS3036X_DPLL_MTIE_SNAP = 1,
   ZLS3036X_DPLL_TIE_READ = 2,
   ZLS3036X_DPLL_TIE_WRITE = 3
} ZLS3036X_DpllTieCtrlE;

#define ZLS3036X_DPLL_TIE_CTRL_MASK   0x03
#define ZLS3036X_DPLL_TIE_CTRL_SHIFT  4

static const Uint32T ZLS3036X_HOLDOVER_STATUS_DPLLX[] = {
   ZLS3036X_HOLDOVER_STATUS_DPLL0,
   ZLS3036X_HOLDOVER_STATUS_DPLL1,
   ZLS3036X_HOLDOVER_STATUS_DPLL2,
   ZLS3036X_HOLDOVER_STATUS_DPLL3
};

static const Uint32T ZLS3036X_LOCK_STATUS_DPLLX[] = {
   ZLS3036X_LOCK_STATUS_DPLL0,
   ZLS3036X_LOCK_STATUS_DPLL1,
   ZLS3036X_LOCK_STATUS_DPLL2,
   ZLS3036X_LOCK_STATUS_DPLL3
};

static const Uint32T ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLLX[] = {
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL0,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL1,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL2,
   ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL3
};

/* Dpll delta frequency offset read register options */
typedef enum
{
   ZLS3036X_DF_RD_REQUEST_DPLL0 = 0x01,
   ZLS3036X_DF_RD_REQUEST_DPLL1 = 0x02,
   ZLS3036X_DF_RD_REQUEST_DPLL2 = 0x04,
   ZLS3036X_DF_RD_REQUEST_DPLL3 = 0x08,

   ZLS3036X_DF_RD_REQUEST_MASK_DPLL0 = 0x01,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL1 = 0x02,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL2 = 0x04,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL3 = 0x08
} ZLS3036X_DpllDfRdRequestE;

static const Uint32T ZLS3036X_DF_RD_REQUEST_MASK_DPLLX[] = {
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL0,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL1,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL2,
   ZLS3036X_DF_RD_REQUEST_MASK_DPLL3
};

static const Uint32T ZLS3036X_DF_RD_REQUEST_DPLLX[] = {
   ZLS3036X_DF_RD_REQUEST_DPLL0,
   ZLS3036X_DF_RD_REQUEST_DPLL1,
   ZLS3036X_DF_RD_REQUEST_DPLL2,
   ZLS3036X_DF_RD_REQUEST_DPLL3
};


/*****************   REGISTER DECLARATIONS   *************************/
/* Sticky register (for sync fail flag status) */
#define ZLS3036X_STICKY_SYNC_FAIL_STATUS ZL303XX_MAKE_MEM_ADDR_36X(0x1B6, ZL303XX_MEM_SIZE_1_BYTE)

/* Sticky register (for clear sync fail flag) */
#define ZLS3036X_STICKY_CLEAR_SYNC_FAIL ZL303XX_MAKE_MEM_ADDR_36X(0x1B7, ZL303XX_MEM_SIZE_1_BYTE)


/* Device id registers */
#define ZLS3036X_DEVICE_ID_REG ZL303XX_MAKE_MEM_ADDR_36X(0x01, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DEVICE_REV_REG ZL303XX_MAKE_MEM_ADDR_36X(0x02, ZL303XX_MEM_SIZE_1_BYTE)

/* Sticky lock register (for accessing sticky registers) */
#define ZLS3036X_STICKY_LOCK_REG ZL303XX_MAKE_MEM_ADDR_36X(0x011, ZL303XX_MEM_SIZE_1_BYTE)

/* Spurs suppression register */
#define ZLS3036X_SPURS_SUPPRESS_REG   ZL303XX_MAKE_MEM_ADDR_36X(0x010, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_SPURS_SUPPRESS_MAX   12

/* Value of GPIOs at startup */
#define ZLS3036X_GPIO_AT_STARTUP_REG  ZL303XX_MAKE_MEM_ADDR_36X(0x019, ZL303XX_MEM_SIZE_2_BYTE)

/* Reference monitor failure registers */
#define ZLS3036X_REF_MON_FAIL_REG(refId)   ZL303XX_MAKE_MEM_ADDR_36X(0x026 + (refId), ZL303XX_MEM_SIZE_1_BYTE)

/* Reference config registers */
#define ZLS3036X_GST_DISQUALIF_TIME_REG            ZL303XX_MAKE_MEM_ADDR_36X(0x046, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_GST_DISQUALIF_TIME(data, refId)   (((data) >> ((refId) * 2)) & 0x03)

#define ZLS3036X_GST_QUALIF_TIME_REG            ZL303XX_MAKE_MEM_ADDR_36X(0x04A, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_GST_QUALIF_TIME(data, refId)   (((data) >> ((refId) * 2)) & 0x03)

#define ZLS3036X_SCM_CFM_LIMIT_REG(refId)   ZL303XX_MAKE_MEM_ADDR_36X(0x050 + (refId), ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_SCM_LIMIT(data)            (((data) >> 4) & 0x07)
#define ZLS3036X_CFM_LIMIT(data)            ((data) & 0x07)

#define ZLS3036X_PFM_LIMIT_REG(refId)     ZL303XX_MAKE_MEM_ADDR_36X(0x060 + ((refId) / 2), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_PFM_LIMIT(data, refId)   (((data) >> (refId) % 2 * 4) & 0x07)

#define ZLS3036X_REF_CONFIG_REG            ZL303XX_MAKE_MEM_ADDR_36X(0x7A, ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_REF_CONFIG(data, refId)   (((data) >> (refId)) & 0x01)

#define ZLS3036X_PHASEMEM_LIMIT_REG(refId) ZL303XX_MAKE_MEM_ADDR_36X(0x06A + refId, ZL303XX_MEM_SIZE_1_BYTE)

/* Ref Freq */
#define ZLS3036X_REF_PRE_DIVIDE_REG(refId)    ZL303XX_MAKE_MEM_ADDR_36X(0x7C + ((refId) / 8), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_REF_PRE_DIVIDE(data, refId)  (((data) >> ((refId) % 8)) & 0x01)
#define ZLS3036X_REF_BASE_FREQ_REG(refId)  ZL303XX_MAKE_MEM_ADDR_36X(0x80 + ((refId) * 0x08), ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_REF_FREQ_MULT_REG(refId)  ZL303XX_MAKE_MEM_ADDR_36X(0x82 + ((refId) * 0x08), ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_REF_RATIO_M_REG(refId)    ZL303XX_MAKE_MEM_ADDR_36X(0x84 + ((refId) * 0x08), ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_REF_RATIO_N_REG(refId)    ZL303XX_MAKE_MEM_ADDR_36X(0x86 + ((refId) * 0x08), ZL303XX_MEM_SIZE_2_BYTE)

/* Reference clock-sync pair registers */
#define ZLS3036X_REF_CLK_SYNC_PAIR_REG(refId)         ZL303XX_MAKE_MEM_ADDR_36X(0x0EC + (refId) / 2, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_REF_CLK_SYNC_PAIR_GET(data, refId)   (((data) >> ((refId) % 2 * 4)) & 0x0F)
#define ZLS3036X_REF_CLK_SYNC_PAIR_SET(syncId, refId) ((syncId & 0x0F) << ((refId) % 2 * 4))

/* DPLL config registers */
#define ZLS3036X_DPLL_CTRL_REG(dpllId)    ZL303XX_MAKE_MEM_ADDR_36X(0x100 + ((dpllId) * 0x20), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_BANDWIDTH_CUSTOM_REG(dpllId)    ZL303XX_MAKE_MEM_ADDR_36X(0x100 + ((dpllId)*0x20) + 1, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_BANDWIDTH_SHIFT     5
#define ZLS3036X_DPLL_BANDWIDTH_MASK      0x07
#define ZLS3036X_DPLL_BANDWIDTH_CUSTOM    0x07
#define ZLS3036X_DPLL_BANDWIDTH(data)     (((data) >> ZLS3036X_DPLL_BANDWIDTH_SHIFT) & ZLS3036X_DPLL_BANDWIDTH_MASK)
#define ZLS3036X_DPLL_TIE_CLEAR_EN_SHIFT  4
#define ZLS3036X_DPLL_TIE_CLEAR_EN_MASK   0x01
#define ZLS3036X_DPLL_TIE_CLEAR_EN(data)  (((data) >> ZLS3036X_DPLL_TIE_CLEAR_EN_SHIFT) & ZLS3036X_DPLL_TIE_CLEAR_EN_MASK)
#define ZLS3036X_DPLL_PSL_SHIFT           0
#define ZLS3036X_DPLL_PSL_MASK            0x0F
#define ZLS3036X_DPLL_PSL(data)           (((data) >> ZLS3036X_DPLL_PSL_SHIFT) & ZLS3036X_DPLL_PSL_MASK)

#define ZLS3036X_DPLL_PULL_IN_REG(dpllId)  ZL303XX_MAKE_MEM_ADDR_36X(0x102 + ((dpllId) * 0x20), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_PULL_IN(data)        ((data) & 0x07)
#define ZLS3036X_DPLL_PULL_IN_MASK        0x0F

/* DPLL refsel registers */
#define ZLS3036X_DPLL_MODE_REFSEL_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X((0x0103 + ((dpllId) * 0x20)), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_REFSEL_FORCE_GET(data)    (((data) & 0xF0) >> 4)
#define ZLS3036X_DPLL_REFSEL_STAT_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X((0x0104 + ((dpllId) * 0x20)), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_REFSEL_AUTO_GET(data)     ((data) & 0x0F)
#define ZLS3036X_MODE_REF_SEL_MASK              0x7
#define ZLS3036X_NCO_MODE_REF_SEL_BITS          0x4
#define ZLS3036X_DPLL_REF_SEL_MASK              0xF
#define ZLS3036X_REF_SEL_OFFSET_SHIFT           4

/* DPLL reference priority registers */
#define ZLS3036X_DPLL_REF_PRIORITY_REG(dpllId, refId)   ZL303XX_MAKE_MEM_ADDR_36X((0x0105 + ((dpllId) * 0x20) + (refId / 2)), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_REF_PRIORITY_GET_UPPER(data)      ((data >> 4) & 0x0F)
#define ZLS3036X_DPLL_REF_PRIORITY_GET_LOWER(data)      ((data) & 0x0F)
#define ZLS3036X_DPLL_REF_PRIORITY_GET(data, refId)     ((refId % 2 == 0) ? ZLS3036X_DPLL_REF_PRIORITY_GET_LOWER(data) : \
                                                                            ZLS3036X_DPLL_REF_PRIORITY_GET_UPPER(data))
#define ZLS3036X_DPLL_REF_PRIORITY_NEVER_LOCK           (0xF)

#define ZLS3036X_DPLL_HO_EDGE_SEL_REG(dpllId)  ZL303XX_MAKE_MEM_ADDR_36X(0x10F + ((dpllId) * 0x20), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_EDGE_SEL(data)           (((data) >> 6) & 0x03)

/* Dpll holdover lock status registers */
#define ZLS3036X_HOLDOVER_LOCK_STATUS_REG                ZL303XX_MAKE_MEM_ADDR_36X(0x0180, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLL(dpllId)  ZLS3036X_HOLDOVER_LOCK_STATUS_MASK_DPLLX[dpllId]
#define ZLS3036X_HOLDOVER_STATUS_DPLL(dpllId)            ZLS3036X_HOLDOVER_STATUS_DPLLX[dpllId]
#define ZLS3036X_LOCK_STATUS_DPLL(dpllId)                ZLS3036X_LOCK_STATUS_DPLLX[dpllId]

/* Dpll TIE control registers */
#define ZLS3036X_DPLL_TIE_CTRL_REG      ZL303XX_MAKE_MEM_ADDR_36X(0x184, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_TIE_DATA_REG      ZL303XX_MAKE_MEM_ADDR_36X(0x185, ZL303XX_MEM_SIZE_4_BYTE)

/* Phase step control registers (per synthesizer) */
#define ZLS3036X_PHASE_STEP_CTRL_REG       ZL303XX_MAKE_MEM_ADDR_36X(0x01A1, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_PHASE_STEP_DATA_REG       ZL303XX_MAKE_MEM_ADDR_36X(0x01A2, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_PHASE_STEP_MAX_REG        ZL303XX_MAKE_MEM_ADDR_36X(0x01A6, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_PHASE_STEP_WRITE_SYNTH(synthId) (ZLS3036X_PHASE_STEP_CTRL_WRITE | (synthId))
#define ZLS3036X_PHASE_STEP_READ_SYNTH(synthId) (ZLS3036X_PHASE_STEP_CTRL_READ | (synthId))
#define ZLS3036X_HOLD_POST_DIV_SHIFT       4
#define ZLS3036X_PHASE_STEP_STATUS_REG     ZL303XX_MAKE_MEM_ADDR_36X(0x1A7, ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_PHASE_STEP_MARGIN_REG     ZL303XX_MAKE_MEM_ADDR_36X(0x1A9, ZL303XX_MEM_SIZE_1_BYTE)

/* Dpll delta frequency read request registers */
#define ZLS3036X_DPLL_RD_OPTION_REG                     ZL303XX_MAKE_MEM_ADDR_36X(0x18B, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_RD_OPTION_MEM_SEL(dpllId, sel)    ((sel) << (dpllId *2))
#define ZLS3036X_DF_RD_REQUEST_REG                      ZL303XX_MAKE_MEM_ADDR_36X(0x018C, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DF_RD_REQUEST_MASK_DPLL(dpllId)        ZLS3036X_DF_RD_REQUEST_MASK_DPLLX[dpllId]
#define ZLS3036X_DF_RD_REQUEST_DPLL(dpllId)             ZLS3036X_DF_RD_REQUEST_DPLLX[dpllId]

/* Dpll freq offset register when in NCO mode (5 bytes, high order byte) */
#define ZLS3036X_NCO_FREQ_OFFSET_WR_HI_DPLL(dpllId) ZL303XX_MAKE_MEM_ADDR_36X((0x018D + ((dpllId) * 0x5)), ZL303XX_MEM_SIZE_1_BYTE)
/* Dpll freq offset register when in NCO mode (5 bytes, low order bytes) */
#define ZLS3036X_NCO_FREQ_OFFSET_WR_LO_DPLL(dpllId) ZL303XX_MAKE_MEM_ADDR_36X((0x018E + ((dpllId) * 0x5)), ZL303XX_MEM_SIZE_4_BYTE)

/* Dpll post divider bitmaps */
#define ZLS3036X_SYNTH0_POST_DIV_A                 (1 << (4 * 0 + ZLS3036X_POST_DIV_A))
#define ZLS3036X_SYNTH0_POST_DIV_B                 (1 << (4 * 0 + ZLS3036X_POST_DIV_B))
#define ZLS3036X_SYNTH0_POST_DIV_C                 (1 << (4 * 0 + ZLS3036X_POST_DIV_C))
#define ZLS3036X_SYNTH0_POST_DIV_D                 (1 << (4 * 0 + ZLS3036X_POST_DIV_D))
#define ZLS3036X_SYNTH1_POST_DIV_A                 (1 << (4 * 1 + ZLS3036X_POST_DIV_A))
#define ZLS3036X_SYNTH1_POST_DIV_B                 (1 << (4 * 1 + ZLS3036X_POST_DIV_B))
#define ZLS3036X_SYNTH1_POST_DIV_C                 (1 << (4 * 1 + ZLS3036X_POST_DIV_C))
#define ZLS3036X_SYNTH1_POST_DIV_D                 (1 << (4 * 1 + ZLS3036X_POST_DIV_D))
#define ZLS3036X_SYNTH2_POST_DIV_A                 (1 << (4 * 2 + ZLS3036X_POST_DIV_A))
#define ZLS3036X_SYNTH2_POST_DIV_B                 (1 << (4 * 2 + ZLS3036X_POST_DIV_B))
#define ZLS3036X_SYNTH2_POST_DIV_C                 (1 << (4 * 2 + ZLS3036X_POST_DIV_C))
#define ZLS3036X_SYNTH2_POST_DIV_D                 (1 << (4 * 2 + ZLS3036X_POST_DIV_D))
#define ZLS3036X_SYNTH3_POST_DIV_A                 (1 << (4 * 3 + ZLS3036X_POST_DIV_A))
#define ZLS3036X_SYNTH3_POST_DIV_B                 (1 << (4 * 3 + ZLS3036X_POST_DIV_B))
#define ZLS3036X_SYNTH3_POST_DIV_C                 (1 << (4 * 3 + ZLS3036X_POST_DIV_C))
#define ZLS3036X_SYNTH3_POST_DIV_D                 (1 << (4 * 3 + ZLS3036X_POST_DIV_D))

/* Dpll synth */
#define ZLS3036X_SYNTH_DRIVE_PLL              ZL303XX_MAKE_MEM_ADDR_36X(0x01B0, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_SYNTH_ENABLE                 ZL303XX_MAKE_MEM_ADDR_36X(0x01B1, ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3036X_SYNTH_BASE_FREQ_REG(synth)   ZL303XX_MAKE_MEM_ADDR_36X(0x1B8 + (synth) * 8, ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_SYNTH_FREQ_MULT_REG(synth)   ZL303XX_MAKE_MEM_ADDR_36X(0x1BA + (synth) * 8, ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_SYNTH_RATIO_M_REG(synth)     ZL303XX_MAKE_MEM_ADDR_36X(0x1BC + (synth) * 8, ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3036X_SYNTH_RATIO_N_REG(synth)     ZL303XX_MAKE_MEM_ADDR_36X(0x1BE + (synth) * 8, ZL303XX_MEM_SIZE_2_BYTE)

#define ZLS3036X_SYNTH_POST_DIV_REG(synth, postDiv) \
   ZL303XX_MAKE_MEM_ADDR_36X(0x200 + synth * 0xC + postDiv * 3, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_SYNTH_POST_DIV_DATA(reg)  (((reg) >> 8) & 0x00FFFFFF)

#define ZLS3036X_DPLL_FAST_LOCK_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X(0x2C2 + ((dpllId) * 0x3), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_FAST_LOCK_EN_MASK       0x01
#define ZLS3036X_DPLL_FAST_LOCK_EN(data)      ((data) & ZLS3036X_DPLL_FAST_LOCK_EN_MASK)

#define ZLS3036X_PHASE_STEP_TIME_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X(0x3DC + (dpllId) * 4, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_INPUT_PHASE_STEP_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X(0x3EC + (dpllId) * 4, ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3036X_MISC_SW_HOST_REG(dpllId) ZL303XX_MAKE_MEM_ADDR_36X(0x3D0 + (dpllId), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_MISC_SW_HOST_HYBRID_TRANSIENT_MASK (0x3)

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

