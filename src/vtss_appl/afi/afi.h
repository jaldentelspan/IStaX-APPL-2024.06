/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _AFI_H_
#define _AFI_H_

#include "critd_api.h"

/****************************************************************************/
//
// Common to both single- and multi-frame flows
//
/****************************************************************************/

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_AFI
#define VTSS_TRACE_GRP_DEFAULT 0

// Macros for accessing mutex functions
#define AFI_LOCK_ASSERT_LOCKED() critd_assert_locked(&AFI_crit, __FILE__, __LINE__)

extern critd_t AFI_crit;
struct AFI_Lock {
    AFI_Lock(const char *file, int line)
    {
        critd_enter(&AFI_crit, file, line);
    }

    ~AFI_Lock()
    {
        critd_exit(&AFI_crit, __FILE__, 0);
    }
};

#define AFI_LOCK_SCOPE() AFI_Lock __afi_lock_guard__(__FILE__, __LINE__)

// Internally, we use zero-based single- and multi-frame flow IDs, externally we use one-based.
#define AFI_U2I_ID(u_id) ((u_id) - 1)
#define AFI_I2U_ID(i_id) ((i_id) + 1)

/****************************************************************************/
//
// Single-frame flows
//
/****************************************************************************/

#define AFI_V1_SLOT_CNT_MAX  8 /**< The maximum number of AFI slots that one single frame
                                    may use. It is set to 8, because that's the burst size
                                    recommended in MEF 13, and the AFI may burst this number
                                    of frames. */

/**
 * Structure with per-slot state
 */
typedef struct {
    /**
     * Each frame may result in up to AFI_V1_SLOT_CNT_MAX
     * repetitions of the same frame to achieve a rate
     * as close to the desired as possible.
     * This in turn may cause the same flow to burst up
     * to AFI_V1_SLOT_CNT_MAX frames.
     */
    mesa_afi_id_t afi_id;

    /**
     * We need to keep track of whether we have allocated
     * an AFI ID or not, because when the multi-frame flow is paused,
     * we have to free the allocated AFI IDs, but keep the
     * #frm, and re-allocate new when the flow is resumed.
     */
    BOOL afi_id_allocated;

    /**
     * The number of frames per second that the AFI
     * tells us that this slot can provide.
     */
    u32 fps;

    /**
     * We must have our own frame copy per slot.
     */
    u8 *frm;

    /**
     * And each will have its own tx_props.
     */
    packet_tx_props_t tx_props;
} afi_slot_state_t;

/**
 * Platform-specific state
 */
typedef struct {
    /**
     * Number of slots used in #slots
     *
     * Used by:
     *   afi_v1.cxx, single and multi
     */
    u32 slot_cnt;

    /**
     * Per-slot state
     *
     * Used by:
     *   afi_v1.cxx, single and multi
     */
    afi_slot_state_t slots[AFI_V1_SLOT_CNT_MAX];

    /**
     * Indicates whether the frame has been sent to H/W or not.
     *
     * Used by:
     *   afi_v2.cxx, single
     */
    BOOL frame_in_hw;

    /**
     * Indicates whether the frames have been sent to H/W or not.
     *
     * Used by:
     *   afi_v2.cxx, multi
     */
    BOOL frames_in_hw;

    /**
     * The ID for this frame flow as provided by the API,
     * not to be confused with the ID that
     * this module provides to other application modules.
     *
     * Used by:
     *   afi_v2.cxx, single and multi
     *
     * Notice:
     *   It's also used as mesa_afi_slowid_t.
     */
    mesa_afi_fastid_t api_id;

    /**
     * Latest start time in ms since boot of this flow
     */
    u64 tx_time_start;

    /**
     * Time in ms that this flow has been active.
     *
     * Used by:
     *   All platforms
     */
    u64 active_time_ms;
} afi_platform_state_t;

/**
 * Platform-specific state.
 * Identical to the multi-frame flow version.
 */
typedef afi_platform_state_t afi_single_platform_state_t;

/**
 * Structure for holding the state of a single-frame flow.
 */
typedef struct {
    /**
     * This single-frame flow's status
     */
    afi_single_status_t status;

    /**
     * Copy of the user's frame.
     * The frame pointer kept inside #tx_props
     * are allocated and maintained by the AFI
     * module so that the caller of afi_single_alloc()
     * may free his once call returns.
     */
    packet_tx_props_t tx_props;

    /**
     * Indicates whether this entry is in use or not.
     */
    BOOL allocated;

    /**
     * State that depends on the AFI version.
     */
    afi_single_platform_state_t platform;
} afi_single_state_t;

/****************************************************************************/
//
// Multi-frame flows
//
/****************************************************************************/

/**
 * Platform-specific state.
 * Identical to the single-frame flow version.
 */
typedef afi_platform_state_t afi_multi_platform_state_t;

/**
 * Structure for holding the state of one multi-frame flow.
 */
typedef struct {
    /**
     * This multi-frame flow's status
     */
    afi_multi_status_t status;

    /**
     * Copy of the user's frames.
     * The frame pointers kept inside #tx_props
     * are allocated and maintained by the AFI
     * module so that the caller of afi_multi_alloc()
     * may free his once call returns.
     */
    CapArray<packet_tx_props_t, VTSS_APPL_CAP_AFI_MULTI_LEN_MAX> tx_props;

    /**
     * Indicates whether this entry is in use or not.
     */
    BOOL allocated;

    /**
     * Number of frames in this multi-frame flow.
     */
    u32 multi_len;

    /**
     * State that depends on the AFI version.
     */
    afi_multi_platform_state_t platform;

} afi_multi_state_t;

// Calls from common towards platform-specific
typedef struct {
    mesa_rc (*single_alloc)    (afi_single_state_t *single_state);
    mesa_rc (*single_free)     (afi_single_state_t *single_state);
    mesa_rc (*single_pause)    (afi_single_state_t *single_state);
    mesa_rc (*single_resume)   (afi_single_state_t *single_state);
    mesa_rc (*single_frm_to_hw)(afi_single_state_t *single_state);
    mesa_rc (*multi_alloc)     (afi_multi_state_t  *multi_state);
    mesa_rc (*multi_free)      (afi_multi_state_t  *multi_state);
    mesa_rc (*multi_pause)     (afi_multi_state_t  *multi_state);
    mesa_rc (*multi_resume)    (afi_multi_state_t  *multi_state);
    mesa_rc (*multi_frms_to_hw)(afi_multi_state_t  *multi_state);
    mesa_rc (*alloc_possible)  (u32 flow_cnt, u32 frame_size_bytes, BOOL alternate_resource_pool);
} afi_platform_func_t;

void afi_platform_v1_create(afi_platform_func_t *func);
void afi_platform_v2_create(afi_platform_func_t *func);

#endif /* !defined(_AFI_H_) */

