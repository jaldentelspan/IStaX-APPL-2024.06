/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _AFI_API_H_
#define _AFI_API_H_

/**
 * \file afi_api.h
 * \brief AFI API
 * \details This header file describes Automatic Frame Injector functions.
 * The module supports two different methods of injecting frames periodically:
 * 1) Single-frame flows. Here, one single frame is injected periodically at
 *    a certain period until stopped.
 *
 * 2) Multi-frame flows. Here, a sequence of frames is injected periodically
 *    at a certain bandwidth consumption until stopped.
 */

#include "microchip/ethernet/switch/api.h"   /* For VTSS_AFI_FAST_INJ_FRM_CNT_MAX */
#include "packet_api.h" /* For packet_tx_props_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AFI module error codes (mesa_rc)
 */
enum {
    AFI_RC_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_AFI), /**< Invalid parameter                                  */
    AFI_RC_FPH,                                            /**< Frames per hour too high or too low                */
    AFI_RC_BPS,                                            /**< Bits per second too high or too low                */
    AFI_RC_OUT_OF_SINGLE_ENTRIES,                          /**< No more free single-frame flow entries             */
    AFI_RC_OUT_OF_MULTI_ENTRIES,                           /**< No more free multi-frame flow entries              */
    AFI_RC_INVALID_DST_PORT_CNT,                           /**< Txing to more than one port                        */
    AFI_RC_NO_DST_AND_NO_MASQUERADE_PORT_SELECTED,         /**< No destination ports selected                      */
    AFI_RC_BOTH_DST_AND_MASQUERADE_PORT_SELECTED,          /**< Both masquerade and destination port selected      */
    AFI_RC_DOWN_PORT_OUT_OF_RANGE,                         /**< Invalid destination port                           */
    AFI_RC_UP_PORT_OUT_OF_RANGE,                           /**< Invalid masquerade port                            */
    AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_DOWN_PORT,   /**< All frames in sequence must use same down-port     */
    AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_UP_PORT,     /**< All frames in sequence must use same up-port       */
    AFI_RC_NO_FRAME_IN_SINGLE_FLOW,                        /**< No frame in single-frame flow                      */
    AFI_RC_NO_FRAMES_IN_MULTI_FLOW,                        /**< No frames in multi-frame flow                      */
    AFI_RC_PACKET_TX_ALLOC_FAILED,                         /**< packet_tx_alloc() failed                           */
    AFI_RC_PACKET_TX_FAILED,                               /**< packet_tx() failed                                 */
    AFI_RC_PACKET_TX_FDMA_FAILED,                          /**< The requested rate cannot be honored at the moment */
    AFI_RC_PACKET_CANCEL_TIMEOUT,                          /**< packet_tx_afi_cancel not called before timeout     */
    AFI_RC_INVALID_JITTER,                                 /**< Invalid jitter value                               */
    AFI_RC_HIJACK_FAILED,                                  /**< mesa_afi_hijack() failed                           */
    AFI_RC_SLOW_ALLOC_FAILED,                              /**< mesa_afi_slow_inj_alloc() failed                   */
    AFI_RC_SLOW_INJ_FREE,                                  /**< mesa_afi_slow_inj_free() failed                    */
    AFI_RC_SLOW_HIJACK_FAILED,                             /**< mesa_afi_slow_inj_frm_hijack() failed              */
    AFI_RC_SLOW_START_FAILED,                              /**< mesa_afi_slow_inj_start() failed                   */
    AFI_RC_SLOW_STOP_FAILED,                               /**< mesa_afi_slow_inj_stop() failed                    */
    AFI_RC_FAST_ALLOC_FAILED,                              /**< mesa_afi_fast_inj_alloc() failed                   */
    AFI_RC_FAST_INJ_FREE,                                  /**< mesa_afi_fast_inj_free() failed                    */
    AFI_RC_FAST_HIJACK_FAILED,                             /**< mesa_afi_fast_inj_frm_hijack() failed              */
    AFI_RC_FAST_START_FAILED,                              /**< mesa_afi_fast_inj_start() failed                   */
    AFI_RC_FAST_STOP_FAILED,                               /**< mesa_afi_fast_inj_stop() failed                    */
    AFI_RC_INVALID_SINGLE_ID,                              /**< Invalid single-frame flow ID                       */
    AFI_RC_INVALID_MULTI_ID,                               /**< Invalid multi-frame flow ID                        */
    AFI_RC_SINGLE_ID_NOT_IN_USE,                           /**< Single-frame flow ID not in use                    */
    AFI_RC_MULTI_ID_NOT_IN_USE,                            /**< Multi-frame flow ID not in use                     */
    AFI_RC_SINGLE_ALREADY_STARTED,                         /**< Single-frame flow already started, so unresumable  */
    AFI_RC_MULTI_ALREADY_STARTED,                          /**< Multi-frame flow already started, so unresumable   */
    AFI_RC_SINGLE_NOT_STARTED,                             /**< Single-frame flow not started, so unpausable       */
    AFI_RC_MULTI_NOT_STARTED,                              /**< Multi-frame flow not started, so unpausable        */
    AFI_RC_NO_MORE_SINGLE_ENTRIES,                         /**< No more single-table entries                       */
    AFI_RC_NO_MORE_MULTI_ENTRIES,                          /**< No more multi-table entries                        */
    AFI_RC_NO_SUCH_SINGLE_ENTRY,                           /**< No such single-table entry                         */
    AFI_RC_NO_SUCH_MULTI_ENTRY,                            /**< No such multi-table entry                          */
    AFI_RC_FPS_EQUAL_TO_ZERO,                              /**< AFI_V1: Resulting fps == 0                         */
    AFI_RC_MASQUERADING_REQUIRES_LOOP_PORT,                /**< Masquerading requires a loop port                  */
    AFI_RC_SWITCH_CORE_FRAME_RAM_EXHAUSTED,                /**< Used allocated quantum of switch-core RAM          */
    AFI_RC_AFI_ALLOC_FAILED,                               /**< mesa_afi_alloc() failed                            */
    AFI_RC_AFI_CANCEL_FAILED,                              /**< packet_tx_afi_cancel() failed                      */
    AFI_RC_NOT_SUPPORTED,                                  /**< Operation not supported on this platform           */
}; // Leave it anonymous

/**
 * A value to use when starting iteration over single- or multi-frame
 * flows with calls to afi_single_status_get() or afi_multi_status_get().
 */
#define AFI_ID_NONE 0

/******************************************************************************/
//
// Single-frame flows (for periodical transmission of one frame)
//
/******************************************************************************/

/**
 * This defines the maximum number of simultaneous single-frame flows.
 */
#define AFI_SINGLE_CNT (fast_cap(MESA_CAP_AFI_V1) ? fast_cap(MESA_CAP_AFI_SLOT_CNT) : fast_cap(MESA_CAP_AFI_SLOW_INJ_CNT))

/**
 * This defines the minimum supported rate in frames per hour
 * for single-frame flows.
 */
// Serval AFI: API only supports 1 fps = 3600 fph
#define AFI_SINGLE_RATE_FPH_MIN (fast_cap(MESA_CAP_AFI_V1) ? 3600LLU : 1)

/**
 * This defines the maximum supported rate in frames per hour
 * for single-frame flows.
 * The actually achievable rate not only depends on the platform
 * we are running on, but also on the link speed of the destination
 * ports and other factors like whether other flows are currently active.
 */
#define AFI_SINGLE_RATE_FPH_MAX (fast_cap(MESA_CAP_AFI_V2) ? fast_cap(MESA_CAP_AFI_SLOW_INJ_FPH_MAX) : ((AFI_MULTI_RATE_BPS_MAX * 3600LLU) / ((64 + 20) * 8LLU)))

/**
 * Each single-frame flow can be configured to have jitter.
 * This enum contains the possible values for this.
 */
typedef enum {
    /**
     * No jitter. Frames are transmitted periodically at the
     * requested interval (afi_single_params_t::fph).
     */
    AFI_SINGLE_JITTER_NONE,

    /**
     * Transmit with jitter. The next frame is transmitted
     * randomly from 75% to 100% of the period (1 divided by
     * afi_single_params_t::fph) after the previous.
     */
    AFI_SINGLE_JITTER_75_TO_100_PERCENT,

    /**
     * Transmit with jitter. The next frame is transmitted
     * randomply from 50% to 100% of the period (1 divided by
     * afi_single_params_t::fph) after the previous.
     */
    AFI_SINGLE_JITTER_50_TO_100_PERCENT,

    /**
     * Transmit with jitter. The next frame is transmitted
     * randomly from 1 TICK to 100% of the period (1 divided by
     * afi_single_params_t::fph) after the previous.
     */
    AFI_SINGLE_JITTER_1_TICK_TO_100_PERCENT,

    /**
     * This must come last
     */
    AFI_SINGLE_JITTER_LAST
} afi_single_jitter_t;

/**
 * Various parameters used to describe and configure a single-frame flow.
 * The destination port comes from the port defined in tx_props.tx_info.dst_port_mask.
 */
typedef struct {
    /**
     * Number of times this frame is sent per hour.
     */
    u64 fph;

    /**
     * Controls the jitter applied to the frame flow.
     *
     * This is useful for PTP delay requests (ref.
     * IEEE1588-2008, clause 9.5.11.2).
     *
     * Support for this feature is platform specific:
     *   Luton26: No
     *   Serval:  No
     *   Jaguar2: Yes
     *   Serval2: Yes
     *   ServalT: Yes
     */
    afi_single_jitter_t jitter;

    /**
     * If FALSE, first frame is transmitted a randomized number of timer ticks
     * after call to vtss_afi_slow_inj_start() to minimize risk of bursting in
     * case many flows are started "simultaneously".
     * If TRUE, first frame must be transmitted ASAP.
     *
     * Support for this feature is platform specific:
     *   Luton26: No
     *   Serval:  No
     *   Jaguar2: Yes
     *   Serval2: Yes
     *   ServalT: Yes
     */
    BOOL first_frame_urgent;
} afi_single_params_t;

/**
 * Structure for configuring an AFI single-frame flow.
 */
typedef struct {
    /**
     * Structure describing the frame in this single-frame flow.
     *
     * Once the call to afi_single_alloc() successfully returns,
     * a snapshot has been taken of the frame, and the caller
     * of afi_single_alloc() may safely free the frame pointer
     * in tx_props.packet_info.frm with a call to packet_tx_free().
     *
     * The packet_info.no_free is used by the AFI module and therefore has no
     * effect on the frame held in packet_info.frm. That frame will not be
     * freed by this module, even if set to false before the call to
     * afi_single_alloc().
     *
     * Restrictions on tx_props:
     *   tx_info.tx_vstax_hdr must be MESA_PACKET_TX_VSTAX_NONE.
     */
    packet_tx_props_t tx_props;

    /**
     * Parameters describing the flow.
     */
    afi_single_params_t params;
} afi_single_conf_t;

/**
 * Structure for holding the status of an AFI single-frame flow.
 */
typedef struct {
    /**
     * Various configuration parameters describing the single-frame flow.
     * It's a copy of the configuration passed to afi_single_alloc().
     */
    afi_single_params_t params;

    /**
     * Frame size including FCS, but excluding IFH, IFG, and preamble.
     */
    u32 frm_sz_bytes;

    /**
     * Destination port for this single-frame flow.
     * VTSS_PORT_NO_NONE if masquerading, that is, creating an up-flow.
     * See also \p masquerade_port.
     */
    u32 dst_port;

    /**
     * If \p dst_port is VTSS_PORT_NO_NONE, it is an up-flow, and this member
     * will hold the port that flow is supposed to ingress.
     */
    u32 masquerade_port;

    /**
     * Caller's module ID (taken from afi_single_conf_t::tx_props.packet_info.modid).
     */
    vtss_module_id_t module_id;

    /**
     * TRUE if user has started a single-frame flow.
     * FALSE if not yet started or she has paused the
     * flow.
     * See also #oper_up.
     */
    BOOL admin_up;

    /**
     * TRUE if the flow is actually running in the chip,
     * FALSE if not.
     *
     * If the user starts a flow, the admin_up will
     * be TRUE, but if there's no link on the destination
     * port, the flow will not actually get started. Only
     * when link comes up, will the flow (automatically)
     * get initiated.
     * See also #admin_up.
     */
    BOOL oper_up;

    /**
     * Actual bandwidth utilization for this single-frame flow,
     * measured in frames per hour.
     *
     * On AFI v1 and FDMA-based, this info is available after the call
     * to afi_single_alloc().
     *
     * On AFI v2, this info is not available until after the
     * very first call to afi_single_pause_resume(pause = FALSE).
     */
    u64 fph_act;

    /**
     * Number of milliseconds this flow has been active from
     * creation to pause or deletion.
     *
     * The value is only valid after a flow has been paused or freed (this
     * structure is passed to the caller of afi_multi_free()).
     *
     * The accuracy depends on many things, hereunder the time from this module
     * sends off a frame to the switch H/W until it actually gets there and gets
     * started, compared to the time it takes from this module asks to stop a
     * flow until it's actually stopped.
     *
     * On AFI v1 and FDMA-based, one single multi-flow may consist of several
     * identical lower-rated flows, because of the AFI's missing accuracy.
     * How to measure the time from start to stop is here even harder.
     *
     * Furthermore, on eCos-based platforms, the granularity of the time is 10
     * ms, and not 1 ms.
     */
    u64 active_time_ms;
} afi_single_status_t;

/**
 * Initialize a single-frame configuration structure.
 *
 * For future compatibility, this must be called prior to
 * your own code filling in members of it.
 */
mesa_rc afi_single_conf_init(afi_single_conf_t *conf);

/**
 * Allocate a single-frame flow.
 *
 * Call afi_single_conf_init() prior to filling in #conf.
 *
 * The #conf structure itself may also be disposed off once this function
 * returns (that is, you may allocate it on the stack if you like).
 *
 * See details and restrictions under afi_single_conf_t.
 * The caller owns the frame pointer (conf->tx_props.packet_info.frm)
 * once this call completes, so you may safely call packet_tx_free()
 * on it after this call - or re-use it for another single-frame flow if needed.
 *
 * The flow can be started after successful allocation with a call
 * to afi_single_pause_resume() with #pause set to FALSE.
 */
mesa_rc afi_single_alloc(afi_single_conf_t *conf, u32 *single_id);

/**
 * Copy frame to H/W (AFI), but do not start the flow.
 * This is an optional step that may be inserted in
 * between afi_single_alloc() and afi_single_pause_resume().
 *
 * The function does not copy frames to H/W if it thinks
 * they are already there.
 *
 * It only works on some platforms, and is mainly provided
 * for a work-around of a chip-problem.
 */
mesa_rc afi_single_frm_to_hw(u32 single_id);

/**
 * Pause or resume a single-frame flow.
 *
 * Remember to call afi_single_free() even if this function
 * returns a non-successful value to free H/W and S/W resources.
 */
mesa_rc afi_single_pause_resume(u32 single_id, BOOL pause);

/**
 * Stop and free/delete a single-frame flow.
 *
 * This will automatically stop (if started) and remove
 * resources from both H/W and S/W (if in H/W).
 *
 * The \p active_time_ms is optional, but if non-NULL, it will receive the
 * number of milliseconds this flow has been active.
 */
mesa_rc afi_single_free(u32 single_id, u64 *active_time_ms);

/**
 * Get single-frame flow status
 *
 * This function is only meant for debugging.
 *
 * To get status for a particular single-frame flow ID:
 *   Set #*single_id == the ID to get and #next to FALSE.
 *
 * To get status for the first defined single-frame flow ID:
 *   Set #*single_id to AFI_ID_NONE and #next to TRUE
 *
 * To get status for the next defined single-frame flow ID:
 *   Leave #*single_id at the previous value and set #next to TRUE.
 *
 * As long as this function returns VTSS_RC_OK, #status will contain
 * valid info. Once it returns AFI_RC_NO_MORE_SINGLE_ENTRIES, iteration
 * is complete. Any other return value is a true error.
 */
mesa_rc afi_single_status_get(u32 *single_id, afi_single_status_t *status, BOOL next);

/**
 * Trace-print frame on afi's trace group called "print", which by default
 * prints out without changing trace levels.
 *
 * This function is only meant for debugging.
 *
 * To print frame for a particular single-frame flow ID:
 *   Set #*single_id == the ID to get and #next to FALSE.
 *
 * To print frame for the first defined single-frame flow ID:
 *   Set #*single_id to AFI_ID_NONE and #next to TRUE
 *
 * To print frame for the next defined single-frame flow ID:
 *   Leave #*single_id at the previous value and set #next to TRUE.
 *
 * As long as this function returns VTSS_RC_OK, a frame was printed.
 * Once it returns AFI_RC_NO_MORE_SINGLE_ENTRIES, iteration is complete.
 * Any other return value is a true error.
 */
mesa_rc afi_single_trace_frame(u32 *single_id, BOOL next);

/******************************************************************************/
//
// Multi-frame flows (e.g. for EMIX)
//
/******************************************************************************/

/**
 * This defines the maximum number of frames in one multi-frame sequence (e.g. EMIX).
 */
#define AFI_MULTI_LEN_MAX (fast_cap(MESA_CAP_AFI_FAST_INJ_FRM_CNT) ? fast_cap(MESA_CAP_AFI_FAST_INJ_FRM_CNT) : 1)

/**
 * This defines the maximum number of simultaneous multi-frame flows.
 */
#define AFI_MULTI_CNT (fast_cap(MESA_CAP_AFI_V2) ? fast_cap(MESA_CAP_AFI_FAST_INJ_CNT) : 16)
/* Number 16 is arbitrarily chosen, but smaller than number of AFI slots divided by AFI_V1_SLOT_CNT_MAX, but still supporting 2 * 8 simultaneous flows */

/**
 * This defines the minimum supported rate in bits per second.
 */
// V2: Well, apart from [MIN; MAX], we could allow 0, which means wirespeed.
// V1: We use frames per second and at jumbo frame sizes
// (~10kbytes/frame), we need at least 80000 bits per second
// to reach at least one frame per second, hence 100kbps.
#define AFI_MULTI_RATE_BPS_MIN (fast_cap(MESA_CAP_AFI_V1) ? 100000LLU : (uint64_t)fast_cap(MESA_CAP_AFI_FAST_INJ_KBPS_MIN) * 1000LLU)

/**
 * This defines the maximum supported rate in bits per second
 * for multi-frame flows.
 */
#define AFI_MULTI_RATE_BPS_MAX (fast_cap(MESA_CAP_AFI_V1) ? 2500000000LLU : (uint64_t)fast_cap(MESA_CAP_AFI_FAST_INJ_KBPS_MAX) * 1000LLU)

/**
 * Various parameters used to describe and configure a multi-frame flow.
 */
typedef struct {
    /**
     * Requested bandwidth utilization for this multi-frame flow,
     * measured in bits per second.
     *
     * When computing the frame rate, each frame in the
     * multi-frame flow contributes with its own size (taken from
     * #tx_props[].packet_info.len). If #line_rate
     * is set to TRUE, 20 bytes for IFG + preamble is
     * automatically added when computing the frame rate.
     */
    u64 bps;

    /**
     * Controls whether the #bps is measured in line rate
     * (L1) or in date rate (L2).
     *
     * If measured in line rate (set #line_rate to TRUE),
     * 20 bytes is added per frame in the sequence when
     * computing the frame rate.
     *
     * If measured in data rate (set #line_rate to FALSE),
     * no additional bytes are added in the sequence when
     * computing the frame rate.
     */
    BOOL line_rate;

    /**
     * Controls the number of times the sequence is played.
     *
     * Set to 0 to keep it running until stopped.
     *
     * Support for this feature is platform specific:
     *   Luton26: No
     *   Serval:  No
     *   Jaguar2: Yes
     *   Serval2: Yes
     *   ServalT: Yes
     */
    u32 seq_cnt;

    /**
     * Set to TRUE if this flow should take from another pool of AFI resources
     * than other multi-flows. By convention, the flag should be set for SAT
     * tests and cleared for other flows.
     *
     * Setting this flag doesn't mean that it will not run out of
     * resources, but the resources will be taken from a different
     * pool than flows without this flag set.
     *
     *   Luton26: No
     *   Serval:  Yes
     *   Jaguar1: No
     *   Jaguar2: No
     *   Serval2: No
     *   ServalT: No
     */
    BOOL alternate_resource_pool;
} afi_multi_params_t;

/**
 * Structure for configuring a single AFI multi-frame flow.
 * The multi-frame flow may consist of one to AFI_MULTI_LEN_MAX
 * frames.
 */
typedef struct {
    /**
     * Frame sequence making up this multi-frame flow.
     * Each frame in the sequence has its own packet_tx_props_t
     * properties and may therefore have different size and
     * contents.
     *
     * The first entry in #tx_props[] that has packet_info.frm
     * set to NULL marks the end of the sequence.
     *
     * Once the call to afi_multi_alloc() successfully returns,
     * a snapshot has been taken of the frames, and the caller
     * of afi_multi_alloc() may safely free the frame pointers
     * in tx_props[].packet_info.frm with calls to packet_tx_free().
     *
     * The packet_info.no_free is used by the AFI module and therefore has no
     * effect on the frame held in packet_info.frm. That frame will not be
     * freed by this module, even if set to false before the call to
     * afi_multi_alloc().
     *
     * Restrictions on tx_props:
     *   tx_info.tx_vstax_hdr must be MESA_PACKET_TX_VSTAX_NONE.
     */
    CapArray<packet_tx_props_t, VTSS_APPL_CAP_AFI_MULTI_LEN_MAX> tx_props;

    /**
     * Parameters describing the flow.
     */
    afi_multi_params_t params;
} afi_multi_conf_t;

/**
 * Structure for holding the status of a single AFI multi-frame flow.
 */
typedef struct {
    /**
     * Various configuration parameters describing the multi-frame flow.
     * It's a copy of the configuration passed to afi_multi_alloc().
     */
    afi_multi_params_t params;

    /**
     * Frame sizes including FCS, but excluding IFH, IFG, and preamble.
     * There is one such per frame in a sequence.
     * The first entry with value == 0 indicates the end of the
     * sequence.
     */
    CapArray<u32, VTSS_APPL_CAP_AFI_MULTI_LEN_MAX> frm_sz_bytes;

    /**
     * Destination port for this multi-frame flow.
     * VTSS_PORT_NO_NONE if masquerading, that is, creating an up-flow.
     * See also \p masquerade_port.
     */
    u32 dst_port;

    /**
     * If \p dst_port is VTSS_PORT_NO_NONE, it is an up-flow, and this member
     * will hold the port that flow is supposed to ingress.
     */
    u32 masquerade_port;

    /**
     * Caller's module ID (taken from afi_multi_conf_t::tx_props[0].packet_info.modid).
     */
    vtss_module_id_t module_id;

    /**
     * TRUE if user has started a multi-frame flow.
     * FALSE if not yet started or she has paused the
     * flow.
     * See also #oper_up.
     */
    BOOL admin_up;

    /**
     * TRUE if the flow is actually running in the chip,
     * FALSE if not.
     *
     * If the user starts a flow, the admin_up will
     * be TRUE, but if there's no link on the destination
     * port, the flow will not actually get started. Only
     * when link comes up, will the flow (automatically)
     * get initiated.
     * See also #admin_up.
     */
    BOOL oper_up;

    /**
     * Actual bandwidth utilization for this multi-frame flow,
     * measured in bits per second.
     *
     * On AFI v1 and FDMA-based, this info is available after the call
     * to afi_multi_alloc().
     *
     * On AFI v2, this info is not available until after the
     * very first call to afi_multi_pause_resume(pause = FALSE).
     */
    u64 bps_act;

    /**
     * Number of milliseconds this flow has been active from
     * creation to pause or deletion.
     *
     * The value is only valid after a flow has been paused or freed (this
     * structure is passed to the caller of afi_multi_free()).
     *
     * The accuracy depends on many things, hereunder the time from this module
     * sends off a frame to the switch H/W until it actually gets there and gets
     * started, compared to the time it takes from this module asks to stop a
     * flow until it's actually stopped.
     *
     * On AFI v1 and FDMA-based, one single multi-flow may consist of several
     * identical lower-rated flows, because of the AFI's missing accuracy.
     * How to measure the time from start to stop is here even harder.
     *
     * Furthermore, on eCos-based platforms, the granularity of the time is 10
     * ms, and not 1 ms.
     */
    u64 active_time_ms;
} afi_multi_status_t;

/**
 * Initialize a multi-frame configuration structure.
 *
 * For future compatibility, this must be called prior to
 * your own code filling in members of it.
 *
 * After the call, you MUST obtain new pointers to conf->tx_props[], because
 * that memory has been freed.
 *
 * \param conf [IN] Structure to initialize.
 *
 * \return VTSS_RC_OK on success, anythng else on error.
 */
mesa_rc afi_multi_conf_init(afi_multi_conf_t *conf);

/**
 * Allocate a multi-frame flow.
 *
 * Call afi_multi_conf_init() prior to filling in #conf.
 *
 * The #conf structure itself may also be disposed off once this function
 * returns (that is, you may allocate it on the stack if you like).
 *
 * See details and restrictions under afi_multi_conf_t.
 * The caller owns the frame pointers (conf->tx_props[].packet_info.frm)
 * once this call completes, so you may safely call packet_tx_free()
 * on them after this call - or re-use them for another multi-frame flow if needed.
 *
 * The flow can be started after successful allocation with a call
 * to afi_multi_pause_resume() with #pause set to FALSE.
 */
mesa_rc afi_multi_alloc(afi_multi_conf_t *conf, u32 *multi_id);

/**
 * Copy frames to H/W (AFI), but do not start the flow.
 * This is an optional step that may be inserted in
 * between afi_multi_alloc() and afi_multi_pause_resume().
 *
 * The function does not copy frames to H/W if it thinks
 * they are already there.
 *
 * It only works on some platforms, and is mainly provided
 * for a work-around of a chip-problem.
 */
mesa_rc afi_multi_frms_to_hw(u32 multi_id);

/**
 * Pause or resume a multi-frame flow.
 *
 * Remember to call afi_multi_free() even if this function
 * returns a non-successful value to free H/W and S/W resources.
 */
mesa_rc afi_multi_pause_resume(u32 multi_id, BOOL pause);

/**
 * Stop and free/delete a multi-frame flow.
 *
 * This will automatically stop (if started) and remove
 * resources from both H/W and S/W (if in H/W).
 *
 * The \p active_time_ms is optional, but if non-NULL, it will receive the
 * number of milliseconds this flow has been active.
 */
mesa_rc afi_multi_free(u32 multi_id, u64 *active_time_ms);

/**
 * Get multi-frame flow status
 *
 * This function is only meant for debugging.
 *
 * To get status for a particular multi-frame flow ID:
 *   Set #*multi_id == the ID to get and #next to FALSE.
 *
 * To get status for the first defined multi-frame flow ID:
 *   Set #*multi_id to AFI_ID_NONE and #next to TRUE
 *
 * To get status for the next defined multi-frame flow ID:
 *   Leave #*multi_id at the previous value and set #next to TRUE.
 *
 * As long as this function returns VTSS_RC_OK, #status will contain
 * valid info. Once it returns AFI_RC_NO_MORE_MULTI_ENTRIES, iteration
 * is complete. Any other return value is a true error.
 */
mesa_rc afi_multi_status_get(u32 *multi_id, afi_multi_status_t *status, BOOL next);

/**
 * Trace-print frame on afi's trace group called "print", which by default
 * prints out without changing trace levels.
 *
 * This function is only meant for debugging.
 *
 * To print frame for a particular multi-frame flow ID:
 *   Set #*multi_id == the ID to get and #next to FALSE.
 *
 * To print frame for the first defined multi-frame flow ID:
 *   Set #*multi_id to AFI_ID_NONE and #next to TRUE
 *
 * To print frame for the next defined multi-frame flow ID:
 *   Leave #*multi_id at the previous value and set #next to TRUE.
 *
 * As long as this function returns VTSS_RC_OK, a frame was printed.
 * Once it returns AFI_RC_NO_MORE_MULTI_ENTRIES, iteration is complete.
 * Any other return value is a true error.
 */
mesa_rc afi_multi_trace_frame(u32 *multi_id, BOOL next);

/******************************************************************************/
//
// UTILITY FUNCTIONS
//
/******************************************************************************/

/**
 * Function for converting an AFI error (see AFI_RX_xxx above) to a textual string.
 * Only errors in the AFI module's range can be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *afi_error_txt(mesa_rc rc);

/**
 * Function for obtaining the current time since boot measured in milliseconds.
 *
 * \return Time since boot in ms.
 */
u64 afi_time_ms_get(void);

/**
 * Get to know whether it's possible to allocate a number of flows with the AFI
 * at hand, using a given resource pool.
 *
 * On Serval-1, there is a limit on the amount of switch core RAM, the AFI
 * may use. This function gives an indication on whether it's possible to
 * allocate N flows of each X bytes from a given resource pool.
 *
 * \return VTSS_RC_OK if it's possible, and VTSS_RC_ERROR if not.
 */
mesa_rc afi_alloc_possible(u32 flow_cnt, u32 frame_size_bytes, BOOL alternate_resource_pool);

/******************************************************************************/
//
// OTHER NON-MANAGEMENT FUNCTIONS
//
/******************************************************************************/

/**
 * Module initialization function.
 *
 * \param data [IN] Pointer to state
 *
 * \return VTSS_RC_OK unless something serious is wrong.
 */
mesa_rc afi_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* _AFI_API_H_ */

