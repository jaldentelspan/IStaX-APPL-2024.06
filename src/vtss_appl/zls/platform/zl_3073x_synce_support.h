/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _ZL_3073X_SYNCE_SUPPORT_H_
#define _ZL_3073X_SYNCE_SUPPORT_H_

#include "zl_dpll_data_types_support.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_DataTypes.h"
#include "zl303xx_Error.h"
#include "zl303xx_RdWr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Range limit definitions & parameter checking */
#ifdef ZL303XX_REF_ID_MIN
#undef ZL303XX_REF_ID_MIN
#define ZL303XX_REF_ID_MIN       ZL303XX_REF_ID_0
#endif

#ifdef ZL303XX_REF_ID_MAX
#undef ZL303XX_REF_ID_MAX
#define ZL303XX_REF_ID_MAX       ZL303XX_REF_ID_9
#define ZL303XX_DPLL_NUM_REFS    (ZL303XX_REF_ID_MAX + 1)
#endif

#define ZLS3073X_NUM_SYNTHS                  3        /* synth 0-2 */
#define ZLS3073X_GP_SYNTHID                  0        /* General purpose synthesizer */

#define ZL303XX_CHECK_REF_ID(val)   \
            (((zl303xx_RefIdE)(val) > ZL303XX_REF_ID_MAX) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

#define ZL303XX_CHECK_REF_ID_0_TO_8(val)   \
            (((zl303xx_RefIdE)(val) > ZL303XX_REF_ID_8) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

#define ZLS3073X_CHECK_POST_DIV_ID(divId) \
           (((divId) > ZLS3073X_POST_DIV_D) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

#define ZLS3073X_CHECK_POST_DIV_ID_C_OR_D(divId) \
           (((divId) < ZLS3073X_POST_DIV_C || (divId) > ZLS3073X_POST_DIV_D) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

#define ZLS3073X_CHECK_OUT_ID(val)   \
            (((unsigned)(val) > 15) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

// typedef enum
// {
//    ZL303XX_UPDATE_NONE = 0,
//    ZL303XX_UPDATE_1HZ = 1,
//    ZL303XX_UPDATE_SYS_TIME = 2,
//    ZL303XX_UPDATE_SYS_INTRVL = 3
// } zl303xx_UpdateTypeE;

#define ZL303XX_CHECK_UPDATE_TYPE(X) \
      ((((zl303xx_UpdateTypeE)(X) > ZL303XX_UPDATE_SYS_INTRVL)) ? \
         (ZL303XX_ERROR_NOTIFY("Invalid Update Mode: " #X),ZL303XX_PARAMETER_INVALID) :  \
         ZL303XX_OK)

#define ZL303XX_CHECK_DPLL_ID(val)   \
            (((zl303xx_DpllIdE)(val) > ZL303XX_DPLL_ID_MAX) ?  \
            ZL303XX_PARAMETER_INVALID : \
            ZL303XX_OK)

#define ZLS3073X_CHECK_SYNTH_ID(synthId) \
           (((synthId) >= ZLS3073X_NUM_SYNTHS) \
                ? ZL303XX_TRACE_ERROR("Invalid synthId=%u", synthId, 0,0,0,0,0), ZL303XX_PARAMETER_INVALID \
                : ZL303XX_OK)

/* Ref Priority Register and Bitfield Attribute Definitions */
#define ZL303XX_DPLL_REF_PRIORITY_REG(refId)   \
                    ZL303XX_MAKE_MEM_ADDR_73X(0x614 + (refId / 2), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_REF_PRIORITY_MASK               (Uint32T)(0x0F)
#define ZL303XX_DPLL_REF_PRIORITY_SHIFT(refId)       ((refId % 2) * 4)

#define ZL303XX_DPLL_REF_PRIORITY_MAX   ZL303XX_DPLL_REF_PRIORITY_MASK

#define ZL303XX_CHECK_REF_PRIORITY(val)  \
            ((val > ZL303XX_DPLL_REF_PRIORITY_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref CFM Limits Attribute & Shift Definitions */

#define ZL303XX_REF_CFM_LIMIT_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x50 + refId), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_REF_CFM_LIMIT_MASK                (Uint32T)(0x07)
#define ZL303XX_REF_CFM_LIMIT_SHIFT               0

#define ZL303XX_REF_CFM_LIMIT_MAX   ZL303XX_REF_CFM_LIMIT_MASK

#define ZL303XX_CHECK_REF_CFM_LIMIT(val)  \
            ((val > ZL303XX_REF_CFM_LIMIT_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref SCM Limits Attribute & Shift Definitions */

#define ZL303XX_REF_SCM_LIMIT_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x50 + refId), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_REF_SCM_LIMIT_MASK                (Uint32T)(0x07)
#define ZL303XX_REF_SCM_LIMIT_SHIFT               4

#define ZL303XX_REF_SCM_LIMIT_MAX   ZL303XX_REF_SCM_LIMIT_MASK

#define ZL303XX_CHECK_REF_SCM_LIMIT(val)  \
            ((val > ZL303XX_REF_SCM_LIMIT_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Divide Bitfield Masks & Shift Definitions */
#define ZL303XX_REF_DIVIDE_MASK                     (Uint32T)(0x01)
#define ZL303XX_REF_DIVIDE_SHIFT                    (0x4)

/* Reference Config Register (Differential Inputs) Masks & Shift Definitions */
#define ZL303XX_REF_DIFF_INPUT_MASK                     (Uint32T)(0x01)
#define ZL303XX_REF_DIFF_INPUT_SHIFT                    (0x2)

/* Phase Acquisition Bitfield Masks & Shift Definitions */
#define ZL303XX_REF_PHASE_ACQUISITION_MASK                     (Uint32T)(0x01)
#define ZL303XX_REF_PHASE_ACQUISITION_SHIFT                    (0)


/* Ref Guard Soak Timer Disqualify Time Register and Bitfield Attribute Definitions */
#define ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_MASK               (Uint32T)(0xFFFF)
#define ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_SHIFT              (0)

#define ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_MAX   ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_MASK

#define ZL303XX_CHECK_REF_GST_DISQUALIFY_TIME(val)  \
            ((val > ZL303XX_DPLL_REF_GST_DISQUALIFY_TIME_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref Guard Soak Timer Qualify Time Register and Bitfield Attribute Definitions */
#define ZL303XX_DPLL_REF_GST_QUALIFY_TIME_MASK               (Uint32T)(0xFFFF)
#define ZL303XX_DPLL_REF_GST_QUALIFY_TIME_SHIFT              (0)

#define ZL303XX_DPLL_REF_GST_QUALIFY_TIME_MAX   ZL303XX_DPLL_REF_GST_QUALIFY_TIME_MASK

#define ZL303XX_CHECK_REF_GST_QUALIFY_TIME(val)  \
            ((val > ZL303XX_DPLL_REF_GST_QUALIFY_TIME_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref PFM Limits Attribute & Shift Definitions */
#define ZL303XX_REF_PFM_LIMIT_MASK               (Uint32T)(0xFF)
#define ZL303XX_REF_PFM_LIMIT_SHIFT              0

#define ZL303XX_REF_PFM_LIMIT_MAX   ZL303XX_REF_PFM_LIMIT_MASK

#define ZL303XX_CHECK_REF_PFM_LIMIT(val)  \
            ((val > ZL303XX_REF_PFM_LIMIT_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref Frame Sync Edge */
#define ZL303XX_REF_FRAME_SYNC_EDGE_MASK                (Uint32T)(0x01)
#define ZL303XX_REF_FRAME_SYNC_EDGE_SHIFT               0

#define ZL303XX_REF_FRAME_SYNC_EDGE_MAX   ZL303XX_REF_FRAME_SYNC_EDGE_MASK

#define ZL303XX_CHECK_REF_FRAME_SYNC_EDGE(val)  \
            ((val > ZL303XX_REF_FRAME_SYNC_EDGE_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Sync Pulse Input Register and Bitfield Attribute Definitions */

#define ZL303XX_REF_SYNC_PULSE_MODE_MASK               (Uint32T)(0xF)
#define ZL303XX_REF_SYNC_PULSE_MODE_SHIFT              (0)

#define ZL303XX_REF_SYNC_PULSE_INPUT_MASK               (Uint32T)(0xF)
#define ZL303XX_REF_SYNC_PULSE_INPUT_SHIFT              (4)
#define ZL303XX_REF_SYNC_PULSE_INPUT_MAX   ZL303XX_REF_SYNC_PULSE_INPUT_MASK

#define ZL303XX_CHECK_REF_SYNC_PULSE_INPUT(val)  \
            ((val > ZL303XX_REF_SYNC_PULSE_INPUT_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Ref Phase Memory Limit */
#define ZL303XX_REF_PHASE_MEMORY_LIMIT_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x6A + refId), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_REF_PHASE_MEMORY_LIMIT_MASK                (Uint32T)(0x08)
#define ZL303XX_REF_PHASE_MEMORY_LIMIT_SHIFT               0

#define ZL303XX_REF_PHASE_MEMORY_LIMIT_MAX   ZL303XX_REF_PHASE_MEMORY_LIMIT_MASK

#define ZL303XX_CHECK_REF_PHASE_MEMORY_LIMIT(val)  \
            ((val > ZL303XX_REF_PHASE_MEMORY_LIMIT_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Lock Delay */

#define ZL303XX_DPLL_LOCK_DELAY_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_LOCK_DELAY_SHIFT               0

#define ZL303XX_DPLL_LOCK_DELAY_MAX   ZL303XX_DPLL_LOCK_DELAY_MASK

#define ZL303XX_CHECK_DPLL_LOCK_DELAY(val)  \
            ((val > ZL303XX_DPLL_LOCK_DELAY_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Hitless Switching */

#define ZL303XX_DPLL_HITLESS_SWITCHING_MASK                (Uint32T)(0x01)
#define ZL303XX_DPLL_HITLESS_SWITCHING_SHIFT               0

#define ZL303XX_DPLL_HITLESS_SWITCHING_MAX   ZL303XX_DPLL_HITLESS_SWITCHING_MASK

/* DPLL ISR Mask */

#define ZL303XX_DPLL_IRQ_ID_MASK          (Uint32T)(0x01)
#define ZL303XX_DPLL_IRQ_ID_SHIFT(dpllId)  (dpllId % 4)

/* Dpll Monitor Failure Indicators */

#define ZLS3073X_DPLL_MON_TH_MASK_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x00D0 + (dpllId * 2),  ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3073X_DPLL_MON_TL_MASK_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x00D1 + (dpllId * 2),  ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3073X_DPLL_MON_MASK_LOCKED_MASK      (Uint32T)(0x01)
#define ZLS3073X_DPLL_MON_MASK_LOCKED_SHIFT     (0)

#define ZLS3073X_DPLL_MON_MASK_HOLDOVER_MASK      (Uint32T)(0x01)
#define ZLS3073X_DPLL_MON_MASK_HOLDOVER_SHIFT     (1)

/* DPLL Ref Fail SW Mask */

#define ZL303XX_DPLL_REF_SW_MASK_MASK                (Uint32T)(0x7F)
#define ZL303XX_DPLL_REF_SW_MASK_SHIFT               0

#define ZL303XX_DPLL_REF_SW_MASK_MAX   ZL303XX_DPLL_REF_SW_MASK_MASK

#define ZL303XX_CHECK_DPLL_REF_SW_MASK(val)  \
            ((val > ZL303XX_DPLL_REF_SW_MASK_MAX) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)


/* DPLL Ref Fail HO Mask */
#define ZL303XX_DPLL_REF_HO_MASK_MASK                (Uint32T)(0x7F)
#define ZL303XX_DPLL_REF_HO_MASK_SHIFT               0

#define ZL303XX_CHECK_DPLL_REF_HO_MASK(val)  \
            ((val != (val & ZL303XX_DPLL_REF_HO_MASK_MASK)) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Phase Slope Limit */
#define ZL303XX_DPLL_PHASE_SLOPE_LIMIT_MASK                (Uint32T)(0xFFFF)
#define ZL303XX_DPLL_PHASE_SLOPE_LIMIT_SHIFT               0

#define ZL303XX_CHECK_DPLL_PHASE_SLOPE_LIMIT(val)  \
            ((val > ZL303XX_DPLL_PHASE_SLOPE_LIMIT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Pull-In Range */
#define ZL303XX_DPLL_PULL_IN_RANGE_MASK                (Uint32T)(0xFFFF)
#define ZL303XX_DPLL_PULL_IN_RANGE_SHIFT               0

#define ZL303XX_CHECK_DPLL_PULL_IN_RANGE(val)  \
            ((val > ZL303XX_DPLL_PULL_IN_RANGE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Lock Selection */
#define ZL303XX_DPLL_LOCK_SELECTION_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x183, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_LOCK_SELECTION_MASK                (Uint32T)(0x03)
#define ZL303XX_DPLL_LOCK_SELECTION_SHIFT(dpllId)       (dpllId * 2)

#define ZL303XX_CHECK_DPLL_LOCK_SELECTION(val)  \
            ((val > ZL303XX_DPLL_LOCK_SELECTION_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Holdover Delay Storage */
#define ZL303XX_DPLL_HOLDOVER_DELAY_STORAGE_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_HOLDOVER_DELAY_STORAGE_SHIFT               0

#define ZL303XX_CHECK_DPLL_HOLDOVER_DELAY_STORAGE(val)  \
            ((val > ZL303XX_DPLL_HOLDOVER_DELAY_STORAGE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Holdover Filter Bandwidth */
#define ZL303XX_DPLL_HOLDOVER_FILTER_BANDWIDTH_MASK                (Uint32T)(0x0F)
#define ZL303XX_DPLL_HOLDOVER_FILTER_BANDWIDTH_SHIFT               0

#define ZL303XX_CHECK_DPLL_HOLDOVER_FILTER_BANDWIDTH(val)  \
            ((val > ZL303XX_DPLL_HOLDOVER_FILTER_BANDWIDTH_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Reference Edge Select */
#define ZL303XX_DPLL_REFERENCE_EDGE_SELECT_MASK                (Uint32T)(0x03)
#define ZL303XX_DPLL_REFERENCE_EDGE_SELECT_SHIFT               6

#define ZL303XX_CHECK_DPLL_REFERENCE_EDGE_SELECT(val)  \
            ((val > ZL303XX_DPLL_REFERENCE_EDGE_SELECT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL FCL Control */
#define ZL303XX_DPLL_FCL_CONTROL_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x2CF, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_FCL_CONTROL_MASK                (Uint32T)(0x03)
#define ZL303XX_DPLL_FCL_CONTROL_SHIFT(dpllId)       (dpllId * 2)

#define ZL303XX_CHECK_DPLL_FCL_CONTROL(val)  \
            ((val > ZL303XX_DPLL_FCL_CONTROL_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PSL Decay Time */
#define ZL303XX_DPLL_PSL_DECAY_TIME_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x0E3 + dpllId), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PSL_DECAY_TIME_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_PSL_DECAY_TIME_SHIFT               0

#define ZL303XX_CHECK_DPLL_PSL_DECAY_TIME(val)  \
            ((val > ZL303XX_DPLL_PSL_DECAY_TIME_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PSL Scaling */
#define ZL303XX_DPLL_PSL_SCALING_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x0E7 + dpllId), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PSL_SCALING_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_PSL_SCALING_SHIFT               0

#define ZL303XX_CHECK_DPLL_PSL_SCALING(val)  \
            ((val > ZL303XX_DPLL_PSL_SCALING_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PSL Max Phase */
#define ZL303XX_DPLL_PSL_MAX_PHASE_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x10B + dpllId * 0x20), ZL303XX_MEM_SIZE_2_BYTE)

#define ZL303XX_DPLL_PSL_MAX_PHASE_MASK                (Uint32T)(0xFFFF)
#define ZL303XX_DPLL_PSL_MAX_PHASE_SHIFT               0

#define ZL303XX_CHECK_DPLL_PSL_MAX_PHASE(val)  \
            ((val > ZL303XX_DPLL_PSL_MAX_PHASE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Loop Filter Corner Frequency */
#define ZL303XX_DPLL_LOOP_FILTER_CORNER_FREQ_MASK                (Uint32T)(0x07)
#define ZL303XX_DPLL_LOOP_FILTER_CORNER_FREQ_SHIFT               0

#define ZL303XX_CHECK_DPLL_LOOP_FILTER_CORNER_FREQ(val)  \
            ((val > ZL303XX_DPLL_LOOP_FILTER_CORNER_FREQ_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Variable Loop Bandwidth Select */
#define ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_SHIFT               0

#define ZL303XX_CHECK_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT(val)  \
            ((val > ZL303XX_DPLL_VARIABLE_LOW_BANDWIDTH_SELECT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Fast Lock Master Enable */
#define ZL303XX_DPLL_FAST_LOCK_MASTER_ENABLE_MASK                (Uint32T)(0x1)
#define ZL303XX_DPLL_FAST_LOCK_MASTER_ENABLE_SHIFT               0

#define ZL303XX_CHECK_DPLL_FAST_LOCK_MASTER_ENABLE(val)  \
            ((val > ZL303XX_DPLL_FAST_LOCK_MASTER_ENABLE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Fast Lock Force Enable */
#define ZL303XX_DPLL_FAST_LOCK_FORCE_ENABLE_MASK                (Uint32T)(0x1)
#define ZL303XX_DPLL_FAST_LOCK_FORCE_ENABLE_SHIFT               1

#define ZL303XX_CHECK_DPLL_FAST_LOCK_FORCE_ENABLE(val)  \
            ((val > ZL303XX_DPLL_FAST_LOCK_FORCE_ENABLE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Fast Lock Phase Error Threshold */
#define ZL303XX_DPLL_FAST_LOCK_PHASE_ERROR_THRESHOLD_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_FAST_LOCK_PHASE_ERROR_THRESHOLD_SHIFT               0

#define ZL303XX_CHECK_DPLL_FAST_LOCK_PHASE_ERROR_THRESHOLD(val)  \
            ((val > ZL303XX_DPLL_FAST_LOCK_PHASE_ERROR_THRESHOLD_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Fast Lock Frequency Error Threshold */
#define ZL303XX_DPLL_FAST_LOCK_FREQ_ERROR_THRESHOLD_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_FAST_LOCK_FREQ_ERROR_THRESHOLD_SHIFT               0

#define ZL303XX_CHECK_DPLL_FAST_LOCK_FREQ_ERROR_THRESHOLD(val)  \
            ((val > ZL303XX_DPLL_FAST_LOCK_FREQ_ERROR_THRESHOLD_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Phase Buildout Enable */
#define ZL303XX_DPLL_PHASE_BUILDOUT_ENABLE_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x110 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PHASE_BUILDOUT_ENABLE_MASK                (Uint32T)(0x01)
#define ZL303XX_DPLL_PHASE_BUILDOUT_ENABLE_SHIFT               0

#define ZL303XX_CHECK_DPLL_PHASE_BUILDOUT_ENABLE(val)  \
            ((val > ZL303XX_DPLL_PHASE_BUILDOUT_ENABLE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Reset PBO Count */
#define ZL303XX_DPLL_RESET_PBO_COUNT_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x110 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_RESET_PBO_COUNT_MASK                (Uint32T)(0x01)
#define ZL303XX_DPLL_RESET_PBO_COUNT_SHIFT               1

#define ZL303XX_CHECK_DPLL_RESET_PBO_COUNT(val)  \
            ((val > ZL303XX_DPLL_RESET_PBO_COUNT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Reset PBO Magnitude */
#define ZL303XX_DPLL_RESET_PBO_MAGNITUDE_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x110 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_RESET_PBO_MAGNITUDE_MASK                (Uint32T)(0x01)
#define ZL303XX_DPLL_RESET_PBO_MAGNITUDE_SHIFT               2

#define ZL303XX_CHECK_DPLL_RESET_PBO_MAGNITUDE(val)  \
            ((val > ZL303XX_DPLL_RESET_PBO_MAGNITUDE_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PBO Jitter Threshold Control */
#define ZL303XX_DPLL_PBO_JITTER_THRESHOLD_CTRL_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x111 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PBO_JITTER_THRESHOLD_CTRL_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_PBO_JITTER_THRESHOLD_CTRL_SHIFT               0

#define ZL303XX_CHECK_DPLL_PBO_JITTER_THRESHOLD_CTRL(val)  \
            ((val > ZL303XX_DPLL_PBO_JITTER_THRESHOLD_CTRL_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PBO Minimum Slope Threshold */
#define ZL303XX_DPLL_PBO_MIN_SLOPE_THRESHOLD_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x112 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PBO_MIN_SLOPE_THRESHOLD_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_PBO_MIN_SLOPE_THRESHOLD_SHIFT               0

#define ZL303XX_CHECK_DPLL_PBO_MIN_SLOPE_THRESHOLD(val)  \
            ((val > ZL303XX_DPLL_PBO_MIN_SLOPE_THRESHOLD_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL PBO End Interval */
#define ZL303XX_DPLL_PBO_END_INTERVAL_REG(dpllId) \
            ZL303XX_MAKE_MEM_ADDR_36X((0x113 + dpllId * 0x20), ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_PBO_END_INTERVAL_MASK                (Uint32T)(0xFF)
#define ZL303XX_DPLL_PBO_END_INTERVAL_SHIFT               0

#define ZL303XX_CHECK_DPLL_PBO_END_INTERVAL(val)  \
            ((val > ZL303XX_DPLL_PBO_END_INTERVAL_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL DAMPING factor */
#define ZL303XX_DPLL_DAMPING_FACTOR_MASK              (Uint32T)(0x1F)
#define ZL303XX_DPLL_DAMPING_FACTOR_SHIFT             0

#define ZL303XX_CHECK_DPLL_DAMPING_FACTOR(val)  \
            ((val > ZL303XX_DPLL_DAMPING_FACTOR_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* DPLL Config */
#define ZL303XX_DPLL_CONFIG_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x182, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_CONFIG_MASK                (Uint32T)(0x07)
#define ZL303XX_DPLL_CONFIG_SHIFT               0

#define ZL303XX_DPLL_CONFIG_MAX_   2

#define ZL303XX_CHECK_DPLL_CONFIG_(val)  \
            ((val > ZL303XX_DPLL_CONFIG_MAX_) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Synth Enable */
#define ZL303XX_SYNTH_ENABLE_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x1B1, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_SYNTH_ENABLE_MASK                (Uint32T)(0x01)
#define ZL303XX_SYNTH_ENABLE_SHIFT(synthId)      synthId

#define ZLS3073X_HP_HSDIV_REG(synthId) \
            ZL303XX_MAKE_MEM_ADDR_73X((0x0490 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3073X_HP_FDIV_BASE_REG(synthId) \
            ZL303XX_MAKE_MEM_ADDR_73X((0x0491 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3073X_HP_FDIV_NUM_REG(synthId)  \
            ZL303XX_MAKE_MEM_ADDR_73X((0x0495 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3073X_HP_FDIV_DEV_REG(synthId)  \
            ZL303XX_MAKE_MEM_ADDR_73X((0x0499 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)

/* Synth Drive DPLL */
#define ZLS3073X_HP_CTRL_REG(synthId) \
             ZL303XX_MAKE_MEM_ADDR_73X((0x0480 + 48 *(synthId - 1)), ZL303XX_MEM_SIZE_1_BYTE)
#define ZL303XX_HP_CTRL_DPLL_MASK                (Uint32T)(0x03)
#define ZL303XX_HP_CTRL_DPLL_SHIFT               (Uint32T)(0x04)

#define ZL303XX_HP_CTRL_FDIV_EN_MASK                (Uint32T)(0x1)
#define ZL303XX_HP_CTRL_FDIV_EN_SHIFT               (Uint32T)(0x1)

#define ZL303XX_HP_CTRL_SYNTH_EN_MASK                (Uint32T)(0x1)
#define ZL303XX_HP_CTRL_SYNTH_EN_SHIFT               (Uint32T)(0)

#define ZL303XX_HP_OUT_MUX                           (Uint32T)(0x4E1)
#define ZL303XX_HP_OUT_MUX_VAL(outId, divider)       (Uint32T)(divider << (2 * (outId/4)))

/* Synth Skew */
#define ZLS3073X_HP_FINE_SHIFT_REG(synthId)  \
               ZL303XX_MAKE_MEM_ADDR_73X((0x04A4 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)

#define ZL303XX_SYNTH_SKEW_MASK                (Uint32T)(0xFF)
#define ZL303XX_SYNTH_SKEW_SHIFT               0

#define ZL303XX_CHECK_SYNTH_SKEW(val)  \
            ((val > ZL303XX_SYNTH_SKEW_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)
#define ZLS3073X_HP_FREQ_BASE_REG(synthId) \
               ZL303XX_MAKE_MEM_ADDR_73X((0x0484 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3073X_HP_FREQ_M_REG(synthId) \
               ZL303XX_MAKE_MEM_ADDR_73X((0x0488 + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3073X_HP_FREQ_N_REG(synthId) \
               ZL303XX_MAKE_MEM_ADDR_73X((0x048C + 48 * (synthId - 1)), ZL303XX_MEM_SIZE_4_BYTE)

/* HP Differential Output Enable */
#define ZLS3073X_HP_OUT_CTRL_REG(outId)  \
               ZL303XX_MAKE_MEM_ADDR_73X(0x0505 + ((outId/2) * 16), ZL303XX_MEM_SIZE_1_BYTE)
#define ZL303XX_HP_OUT_FORMAT_MASK                 (Uint32T)(0xF)
#define ZL303XX_HP_OUT_FORMAT_SHIFT                (Uint32T)(0x0)

#define ZL303XX_CHECK_HP_OUT_FORMAT(val)  \
            ((val > ZL303XX_HP_OUT_FORMAT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)
#define ZLS3073X_HP_OUT_MSDIV_REG(outId)  \
             ZL303XX_MAKE_MEM_ADDR_73X(0x0500 +((outId/2) * 16), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3073X_HP_OUT_LSDIV_REG(outId)  \
             ZL303XX_MAKE_MEM_ADDR_73X(0x0501 +((outId/2) * 16), ZL303XX_MEM_SIZE_4_BYTE)

#define ZL303XX_HP_DIFF_ENABLE_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x261, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_HP_DIFF_ENABLE_MASK                (Uint32T)(0x01)
#define ZL303XX_HP_DIFF_ENABLE_SHIFT(outId)        outId

#define ZL303XX_CHECK_HP_DIFF_ENABLE(val)  \
            ((val > ZL303XX_DPLL_CONFIG_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* HP CMOS Output Enable */
#define ZL303XX_HP_CMOS_ENABLE_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x262, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_HP_CMOS_ENABLE_MASK                (Uint32T)(0x01)
#define ZL303XX_HP_CMOS_ENABLE_SHIFT(outId)        outId

#define ZL303XX_CHECK_HP_CMOS_ENABLE(val)  \
            ((val > ZL303XX_DPLL_CONFIG_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* HP Differential Power */
#define ZL303XX_HP_DIFF_POWER_REG \
            ZL303XX_MAKE_MEM_ADDR_36X(0x263, ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_HP_DIFF_POWER_MASK                (Uint32T)(0x01)
#define ZL303XX_HP_DIFF_POWER_SHIFT(outId)        outId

#define ZL303XX_CHECK_HP_DIFF_POWER(val)  \
            ((val > ZL303XX_DPLL_CONFIG_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Synthesizer Post Divider */

#define ZL303XX_SYNTH_POST_DIV_REG(synthId, divId) \
            ZL303XX_MAKE_MEM_ADDR_36X(0x200 + synthId * 0xC + divId * 3, ZL303XX_MEM_SIZE_4_BYTE)
#define ZL303XX_SYNTH_POST_DIV_MASK                   (Uint32T)(0xFFFFFF)
#define ZL303XX_SYNTH_POST_DIV_SHIFT                  8

#define ZL303XX_CHECK_SYNTH_POST_DIV(val)  \
            ((val > ZL303XX_SYNTH_POST_DIV_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Synthesizer Post Divider Quadrature Phase Shift */

#define ZL303XX_SYNTH_POST_DIV_QUADRATURE_PHASE_SHIFT_REG(synthId, divId)  \
            ZL303XX_MAKE_MEM_ADDR_36X(0x234 + synthId * 0xC + (divId - 2) * 2,  ZL303XX_MEM_SIZE_1_BYTE)
#define ZL303XX_SYNTH_POST_DIV_QUADRATURE_PHASE_SHIFT_MASK                   (Uint32T)(0x07)
#define ZL303XX_SYNTH_POST_DIV_QUADRATURE_PHASE_SHIFT_SHIFT                  5

#define ZL303XX_CHECK_SYNTH_POST_DIV_QUADRATURE_PHASE_SHIFT(val)  \
            ((val > ZL303XX_SYNTH_POST_DIV_QUADRATURE_PHASE_SHIFT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Synthesizer Post Divider Coarse Phase Shift */

#define ZL303XX_SYNTH_POST_DIV_COARSE_PHASE_SHIFT_REG(synthId, divId)  \
            ZL303XX_MAKE_MEM_ADDR_36X(0x234 + synthId * 0xC + (divId - 2) * 2,  ZL303XX_MEM_SIZE_2_BYTE)
#define ZL303XX_SYNTH_POST_DIV_COARSE_PHASE_SHIFT_MASK                   (Uint32T)(0x1FFF)
#define ZL303XX_SYNTH_POST_DIV_COARSE_PHASE_SHIFT_SHIFT                  0

#define ZL303XX_CHECK_SYNTH_POST_DIV_COARSE_PHASE_SHIFT(val)  \
            ((val > ZL303XX_SYNTH_POST_DIV_COARSE_PHASE_SHIFT_MASK) ?  \
            ZL303XX_PARAMETER_INVALID :  \
            ZL303XX_OK)

/* Reference Failure Interrupt Status Register */
#define ZLS3073X_REF_IRQ_ACTIVE_REG(refId) \
             ZL303XX_MAKE_MEM_ADDR_73X(0x1A8 + (refId / 8),  ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3073X_REF_IRQ_ACTIVE_MASK          (Uint32T)(0x01)
#define ZLS3073X_REF_IRQ_ACTIVE_SHIFT(refId)  (refId % 8)

/* Reference Failure Interrupt config Register */

#define  ZLS3073X_REF_IRQ_MASK_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x0A8 + (refId / 8),  ZL303XX_MEM_SIZE_1_BYTE)
#define ZL303XX_REF_IRQ_MASK_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_IRQ_MASK_SHIFT(refId)  (refId % 8)

/* Reference Monitor Failure Mask */

#define ZLS3073X_REF_MON_TH_MASK_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x0B0 + (refId * 2),  ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3073X_REF_MON_TL_MASK_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x0B1 + (refId * 2),  ZL303XX_MEM_SIZE_1_BYTE)

/* Reference Monitor Failure Indicators */
#define ZL303XX_REF_MON_LOS_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_MON_LOS_SHIFT         0

#define ZL303XX_REF_MON_SCM_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_MON_SCM_SHIFT         1

#define ZL303XX_REF_MON_CFM_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_MON_CFM_SHIFT         2

#define ZL303XX_REF_MON_GST_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_MON_GST_SHIFT         3

#define ZL303XX_REF_MON_PFM_MASK          (Uint32T)(0x01)
#define ZL303XX_REF_MON_PFM_SHIFT         4

/* DPLL ISR Status */
#define ZLS3073X_DPLL_ID_IRQ_ACTIVE_MASK          (Uint32T)(0x01)
#define ZLS3073X_DPLL_ID_IRQ_ACTIVE_SHIFT(dpllId) (dpllId)

/* DPLL Monitor failure Indicators */
#define ZLS3073X_DPLL_MON_STATUS_REG(dpllId) \
             ZL303XX_MAKE_MEM_ADDR_73X(0x0110 + (dpllId) , ZL303XX_MEM_SIZE_1_BYTE)

#define ZLS3073X_DPLL_MON_STICKY_HOLDOVER_MASK              (Uint32T)(0x01)
#define ZLS3073X_DPLL_MON_STICKY_HOLDOVER_SHIFT             (Uint32T)(0x01)
#define ZLS3073X_DPLL_MON_STICKY_LOCKED_MASK                (Uint32T)(0x01)
#define ZLS3073X_DPLL_MON_STICKY_LOCKED_SHIFT               (Uint32T)(0)

#define ZL303XX_DPLL_ISR_STATUS_HOLDOVER_MASK             (Uint32T)(0x01)
#define ZL303XX_DPLL_ISR_STATUS_HOLDOVER_SHIFT(dpllId)    (dpllId * 2)

#define ZL303XX_DPLL_ISR_STATUS_LOCK_MASK         (Uint32T)(0x01)
#define ZL303XX_DPLL_ISR_STATUS_LOCK_SHIFT(dpllId)        (1 + dpllId *2)

// #define ZL303XX_DPLL_ISR_STATUS_CUR_REF_FAIL_MASK (Uint32T)(0x01)

/* Reference Monitor Failure Masks */

#define ZLS3073X_REF_MON_STATUS_REG(refId) \
            ZL303XX_MAKE_MEM_ADDR_73X(0x0108 + refId ,  ZL303XX_MEM_SIZE_1_BYTE)

/* DPLL Hold Lock Status */

#define ZL303XX_DPLL_HOLD_LOCK_STATUS_REG \
            ZL303XX_MAKE_MEM_ADDR_73X(0x180,  ZL303XX_MEM_SIZE_1_BYTE)

#define ZL303XX_DPLL_HOLD_LOCK_STATUS_HOLDOVER_MASK             (Uint32T)(0x01)
#define ZL303XX_DPLL_HOLD_LOCK_STATUS_HOLDOVER_SHIFT(dpllId)    (dpllId * 2)

#define ZL303XX_DPLL_HOLD_LOCK_STATUS_LOCK_MASK                 (Uint32T)(0x01)
#define ZL303XX_DPLL_HOLD_LOCK_STATUS_LOCK_SHIFT(dpllId)        (1 + dpllId *2)


// typedef struct
// {
//    /* System timestamp: used for the Rx/Tx timestamps */
//    Uint64S localTs;
//
//    /* The insertTs when RTP protocol is in use */
//    /* Kept in SEC:TICKS format */
//    Uint64S rtpTs;
//
//    /* Units are protocol dependent:
//       PTP/NTP - units of seconds32:nanoseconds32.
//       RTP - upper byte = 0: lower byte in ticks of the RTP period  */
//    /* This is the raw value read from the register.               */
//    Uint64S rawInsertTs;
//
//    /* The insert timestamp converted to units of SEC:NANO   */
//    /* A more accurate name would be 'nsTs' or 'todTs'       */
//    Uint64S insertTs;
//
//    /* DCO */
//    Uint64S dcoTs;       /* DCO Ticks32:Phase32 => the raw sampled value    */
//    Uint64S dcoExtendTs; /* DCO Ticks64 (extended to 64-bit via software)   */
//
//    /* DCO offset at the time the sample was taken. */
//    Sint32T dcoFreq;
//
//    /* Predicted tick ratios for this sample offset. Used to convert between
//     * system clock counts and adjusted clock frequencies. */
//    Uint32T dcoDelta;    /* Predicted #DCO ticks in the next SYSTEM interval. */
//    Uint32T nsDelta;     /* Predicted #Nanoseconds in the next SYSTEM interval.*/
//    Uint32T insertDelta; /* Predicted #INSERT timestamp ticks in the next SYSTEM
//                          *    interval. */
//
//    /* CPU */
//    Uint32T osTimeTicks; /* Value of the OS tick counter at the sample point */
//    Uint64S cpuHwTime;   /* CPU HW timestamp at the sample point (if CPU HW
//                            time-stamping is enabled). */
//
// } zl303xx_TsSampleS;

// typedef struct
// {
//    zl303xx_BooleanE     lostLockIsrEn;
//    zl303xx_BooleanE     holdoverIsrEn;
//    zl303xx_DpllIdE      Id;
// } zl303xx_DpllIsrConfigS;

// typedef struct
// {
//    zl303xx_DpllHoldStateE holdover;
//    zl303xx_DpllLockStateE locked;
//    zl303xx_DpllIdE        Id;
// } zl303xx_DpllStatusS;

// typedef struct
// {
//    zl303xx_RefIdE       Id;
//    zl303xx_BooleanE     refIsrEn;
//    zl303xx_BooleanE     scmIsrEn;
//    zl303xx_BooleanE     cfmIsrEn;
//    zl303xx_BooleanE     pfmIsrEn;
//    zl303xx_BooleanE     gstIsrEn;
//    zl303xx_BooleanE     losIsrEn;
// } zl303xx_RefIsrConfigS;

// typedef struct
// {
//    zl303xx_BooleanE     refFail;  /* Logical OR of all unmasked monitor bits */
//
//    zl303xx_BooleanE     losFail;  /*  \                 */
//    zl303xx_BooleanE     scmFail;  /*   \                */
//    zl303xx_BooleanE     cfmFail;  /*    |   Sticky bits */
//    zl303xx_BooleanE     gstFail;  /*   /                */
//    zl303xx_BooleanE     pfmFail;  /*  /                 */
//
//    zl303xx_RefIdE       Id;
// } zl303xx_RefIsrStatusS;

typedef enum {
    HP_FORMAT_DISABLED     = 0x0,
    HP_FORMAT_DIFFERENTIAL = 0x2,
    HP_FORMAT_TWO_COMS     = 0x4,
    HP_FORMAT_P_ENABLED    = 0x5,
    HP_FORMAT_NONE         = 0xf,

}zl303xx_Ref73xHpFormat_t;

zlStatusE zl303xx_Ref73xBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T freqHz);
zlStatusE zl303xx_Ref73xFreqMultipleSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T multiple);
zlStatusE zl303xx_Ref73xRatioMRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T M);
zlStatusE zl303xx_Ref73xRatioNRegSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T N);
zlStatusE zl3073x_DpllRefPrioritySet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl3073x_DpllRefPriorityGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, zl303xx_RefIdE refId, Uint32T *val);
zlStatusE zl303xx_Ref73xCfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl303xx_Ref73xScmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl303xx_Ref73xGstDisqualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl303xx_Ref73xGstQualifyTimeSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl303xx_Ref73xPfmLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl303xx_Ref73xPhaseMemoryLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T val);
zlStatusE zl3073x_RefIsrConfigGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIsrConfigS *par);
zlStatusE zl3073x_RefIsrConfigSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIsrConfigS *par);
zlStatusE zl3073x_RefIsrStatusGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIsrStatusS *par);

zlStatusE zl303xx_Ref73xDpllIsrStatusGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllStatusS *par);
zlStatusE zl303xx_Ref73xDpllIsrMaskGet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIsrConfigS *par);
zlStatusE zl303xx_Ref73xDpllIsrMaskSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIsrConfigS *par);

zlStatusE zl303xx_Ref73xSynthEnableSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_BooleanE val);
zlStatusE zl303xx_Ref73xSynthDrivePll(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_DpllIdE dpllId);
zlStatusE zl303xx_Ref73xSynthBaseFreqSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T freqHz);
zlStatusE zl303xx_Ref73xSynthRatioMRegSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T M);
zlStatusE zl303xx_Ref73xSynthRatioNRegSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T N);

zlStatusE zl303xx_Ref73xHpFormatSet(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, zl303xx_Ref73xHpFormat_t format);
/* Helper functions to configure Integer and fractional divider of synthesizer 1,2 */
zlStatusE zl303xx_Ref73xSynthHsDivSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T val);
zlStatusE zl303xx_Ref73xSynthFracDivBaseSet(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, Uint32T val);
/* Helper functions to configure clock divider of synthesizer outputs out_0 - out_7 */
zlStatusE zl303xx_Ref73xHpMsDiv(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, Uint32T val);
zlStatusE zl303xx_Ref73xHpLsDiv(zl303xx_ParamsS *zl303xx_Params, Uint32T outId, Uint32T val);
zlStatusE zl303xx_Ref73xDpllVariableLowBandwidthSelectSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);
zlStatusE zl303xx_Ref73xDpllPhaseSlopeLimitSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);

zlStatusE zl3073x_DpllHoldoverDelayStorageSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);
zlStatusE zl3073x_DpllHoldoverFilterBandwidthSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);

zlStatusE zl303xx_Ref73xLosStateSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_RefIdE refId, Uint32T los);
zlStatusE zl303xx_Ref73xDpllFastLockMasterEnableSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);
zlStatusE zl303xx_Ref73xDpllFastLockFreqErrorThresholdSet(zl303xx_ParamsS *zl303xx_Params, zl303xx_DpllIdE dpllId, Uint32T val);

zlStatusE zl303xx_Ref73xDpllFeedbackRefInit(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_Dpll73xPullInRangeSet(zl303xx_ParamsS *zl303xx_Params, Uint32T PullInRange);

#ifdef __cplusplus
}
#endif

#endif // _ZL_3073X_SYNCE_SUPPORT_H_
